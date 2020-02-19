// Fill out your copyright notice in the Description page of Project Settings.


#include "STrackerBot.h"
#include "Components/StaticMeshComponent.h"
#include "Components/SphereComponent.h"
#include "NavigationSystem/Public/NavigationSystem.h"
#include "NavigationSystem/Public/NavigationPath.h"
#include "Kismet/GameplayStatics.h"
#include "GameFramework/Character.h"
#include "ScHealthComponent.h"
#include "DrawDebugHelpers.h"
#include "SCharacter.h"
#include "TimerManager.h"
#include "Sound/SoundCue.h"

static int32 DebugrackerBotDrawing = 0;
FAutoConsoleVariableRef CVARDebugTrackerBot(
	TEXT("COOP.DebugTrackerBot"),
	DebugrackerBotDrawing,
	TEXT("Draw Debug Line For TrackerBot"),
	ECVF_Cheat);

// Sets default values
ASTrackerBot::ASTrackerBot()
{
 	// Set this pawn to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = true;

	Meshcomp = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("MeshComp"));
	Meshcomp->SetCanEverAffectNavigation(false); // 네브메시컴포넌트를 그릴때 영향받지 않게함.
	Meshcomp->SetSimulatePhysics(true);
	RootComponent = Meshcomp;
	
	HealComp = CreateDefaultSubobject<UScHealthComponent>(TEXT("HealComp"));
	HealComp->OnHealChanged.AddDynamic(this, &ASTrackerBot::HandleTakeDamage);

	// 충돌판정을 체크할 스피어컴포넌트
	SphereComp = CreateDefaultSubobject<USphereComponent>(TEXT("SphereComp"));
	SphereComp->SetSphereRadius(200);
	SphereComp->SetCollisionEnabled(ECollisionEnabled::QueryOnly); // 질의상태
	SphereComp->SetCollisionResponseToAllChannels(ECR_Ignore); // 모두 무시
	SphereComp->SetCollisionResponseToChannel(ECC_Pawn, ECR_Overlap); // 폰만 등록함.
	SphereComp->SetupAttachment(Meshcomp);

	MovementForce = 1000;

	bUseVelocityChange = false;

	RequiredDistanceToTarget = 100;

	bExploded = false;

	bStartedSelfDestroy = false;

	ExplodeDamage = 60;

	ExplodeRadius = 350;

	SelfDamageInterval = 0.25f;

	PowerLevel = 0;
}

// Called when the game starts or when spawned
void ASTrackerBot::BeginPlay()
{
	Super::BeginPlay();
	
	// 서버에서만 탐색할수 있도록 변경
	if (Role == ROLE_Authority)
	{
		// 첫 이동지점을 설정해준다.
		NextPathPoint = GetNextPathPoint(); // 시작시 다음 이동 정보를 받는다.

		FTimerHandle TimerHandle_CheckPowerLevel;

		GetWorldTimerManager().SetTimer(TimerHandle_CheckPowerLevel, this, &ASTrackerBot::OnCheckNearBot, 1.0f, true); // 1초동안 타이머 설정함
	}


	//OnActorBeginOverlap.AddDynamic(this, &ASTrackerBot::NotifyActorBeginOverlap);
}

FVector ASTrackerBot::GetNextPathPoint()
{
	// 플레이어의 위치를 받아옴
	AActor* BestTarget = nullptr;
	float NearestTargetDistance = FLT_MAX;

	// 폰반복자까지 있는 등장인물 갯수대로 반복함.
	for (FConstPawnIterator It = GetWorld()->GetPawnIterator(); It; ++It)
	{
		APawn* TestPawn = It->Get();
		// 현재 플레이어가 컨트롤 되고 있다면.
		if (TestPawn == nullptr || UScHealthComponent::IsFriendly(TestPawn,this))
		{
			continue;
		}
		UScHealthComponent* TestPawnHealthComp = Cast<UScHealthComponent>(TestPawn->GetComponentByClass(UScHealthComponent::StaticClass()));
		if (TestPawnHealthComp && TestPawnHealthComp->GetHealth() > 0.0f)
		{
			float Distance = (TestPawn->GetActorLocation() - GetActorLocation()).Size();
			if (Distance < NearestTargetDistance)
			{
				BestTarget = TestPawn;
				NearestTargetDistance = Distance;

			}
			break;
		}
	}

	if (BestTarget)
	{

		UNavigationPath* NewPath = UNavigationSystemV1::FindPathToActorSynchronously(this, GetActorLocation(), Cast<AActor>(BestTarget));
		GetWorldTimerManager().ClearTimer(TimerHandle_Refresh);
		GetWorldTimerManager().SetTimer(TimerHandle_Refresh, this, &ASTrackerBot::RefreshPoff, 5.0f, false);

		// 새로운 경로에서, 다음 점 정보가 있다면 다음 점 정보를 전달해줌.
		// 클라이언트와 서버에서 두군데에서 탐색함.
		if (NewPath && NewPath->PathPoints.Num() > 1)
		{
			return NewPath->PathPoints[1];
		}
	}
	// 다음 경로가 없다면 현재 위치를 알랴줌
	return GetActorLocation();
}

void ASTrackerBot::HandleTakeDamage(UScHealthComponent * OwningHealthComp, float Health, float HeahthDelta, const UDamageType * DamageType, AController * InstigatedBy, AActor * DamageCauser)
{
	if (Matinst == nullptr)
	{
		// 새로운 메테리얼 인스턴스를 생성한다.
		Matinst = Meshcomp->CreateAndSetMaterialInstanceDynamicFromMaterial(0, Meshcomp->GetMaterial(0));
	}

	if (Matinst)
	{
		Matinst->SetScalarParameterValue("LastTimeDamageTaken", GetWorld()->TimeSeconds); // 현재 시간을 LastTimeDamageTaken 파라미터에 보내주어, 시간차이에 의한 깜박거림(메테리얼 타임 세팅)을 유도한다.
	}

	if (Health <= 0.0f)
	{
		SetDestruct();
	}
}

void ASTrackerBot::DamageSelf()
{
	UGameplayStatics::ApplyDamage(this, 20, GetInstigatorController(), this, nullptr);
}

void ASTrackerBot::OnCheckNearBot()
{
	// 근처 봇을 체크하는 거리
	const float Radius = 600;

	// 라디우스 반경의 콜라이더를 생성해 오버랩 체크
	FCollisionShape CollShape;
	CollShape.SetSphere(Radius);

	// 폰과 AI봇들을 찾는다.
	FCollisionObjectQueryParams QueryParams; // 물어볼 파라미터
	QueryParams.AddObjectTypesToQuery(ECC_PhysicsBody);
	QueryParams.AddObjectTypesToQuery(ECC_Pawn);

	TArray<FOverlapResult> OverLaps; // 오버랩 결과물들
	// 반경만큼에서 물리바디와 폰 정보를 찾아 오버랩스에 저장
	GetWorld()->OverlapMultiByObjectType(OverLaps, GetActorLocation(), FQuat::Identity, QueryParams, CollShape);
	if (DebugrackerBotDrawing)
	{
		DrawDebugSphere(GetWorld(), GetActorLocation(), Radius, 12, FColor::White, false, 0.1f);
	}
	int32 NrOfBots = 0;

	// 결과물 사이에서 루프
	for (FOverlapResult Result : OverLaps)
	{
		// 탐색된 액터들중에서 봇 타입을 찾는다.
		ASTrackerBot* Bot = Cast<ASTrackerBot>(Result.GetActor());

		// 유형이 봇이고 자기 자신이 아니라면
		if (Bot && Bot != this)
		{
			NrOfBots++;
		}
	}

	const int32 MaxPowerLevel = 4;
	// 0~4까지 파워레벨
	PowerLevel = FMath::Clamp(NrOfBots, 0, MaxPowerLevel);
	// 메테리얼 인스턴스를 가져옴
	if (Matinst == nullptr)
	{
		Matinst = Meshcomp->CreateAndSetMaterialInstanceDynamicFromMaterial(0, Meshcomp->GetMaterial(0));
	}
	// 밝아질 알파값 설정.
	if (Matinst)
	{
		float Alpha = PowerLevel / (float)MaxPowerLevel;

		Matinst->SetScalarParameterValue("PowerLevelAlpha", Alpha); // 알파 설정
	}
}

// Called every frame
void ASTrackerBot::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);
	// 서버이고 폭팔처리가 안되있다면
	if (Role == ROLE_Authority && !bExploded)
	{
		OnCheckNearBot();
		FVector DistanceVector = (GetActorLocation() - NextPathPoint);

		float DistanceToTarget = DistanceVector.Size();

		// 현재 로케이션이 다음 이동지점과 같지 않다면.
		if (DistanceToTarget <= RequiredDistanceToTarget)
		{
			NextPathPoint = GetNextPathPoint(); // 시작시 다음 이동 정보를 받는다.
		}
		else // 동일하거나 매우 가까이 있다면.
		{
			FVector ForceDirection = NextPathPoint - GetActorLocation(); // 현재 치에서 다음 이동지역을 빼서 방향벡터를 만듬.
			ForceDirection.Normalize();

			ForceDirection *= MovementForce;

			// 계속 이동한다 다음 지점으로
			Meshcomp->AddForce(ForceDirection, NAME_None, bUseVelocityChange);
			if (DebugrackerBotDrawing)
			{
				DrawDebugDirectionalArrow(GetWorld(), GetActorLocation(), GetActorLocation() + ForceDirection, 32, FColor::Yellow, false, 0.0f, 0, 1.0f);
			}
		}
		if (DebugrackerBotDrawing)
		{
			DrawDebugSphere(GetWorld(), NextPathPoint, 20, 12, FColor::Yellow, false, 4.0f, 1.0f);
		}
	}
}

void ASTrackerBot::NotifyActorBeginOverlap(AActor * OtherActor)
{
	Super::NotifyActorBeginOverlap(OtherActor);
	// 재시작 방지함.
	if (!bStartedSelfDestroy)
	{
		ASCharacter* PlayerPawn = Cast<ASCharacter>(OtherActor);
		if (PlayerPawn && !UScHealthComponent::IsFriendly(OtherActor, this))
		{
			if (Role == ROLE_Authority) // 이제 서버에서 처리함 타이머 추가
			{
				// 플레이어를 찾았으면 자기 자신에게 타이머를 건다.
				GetWorldTimerManager().SetTimer(TimerHandle_SelfDamage, this, &ASTrackerBot::DamageSelf, SelfDamageInterval, true, 0.0f); // 자기에게 데미지를 연속해서 준다. 폭발준비함.
			}

				bStartedSelfDestroy = true;

				UGameplayStatics::SpawnSoundAttached(SelfDestructSound, RootComponent);

		}
	}
}

void ASTrackerBot::GetLifetimeReplicatedProps(TArray< FLifetimeProperty > & OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	//DOREPLIFETIME(ASTrackerBot, CurrentWeapon); // 라이프타임이 살아있는동안 리플리케이트 변수를 가져온다. 복사를 하면 각자 클라이언트상 화면에서는 정상동작한다.
	//DOREPLIFETIME(ASTrackerBot, bDied);
}

// 볼 폭팔 연출. 실제 데미지도 전달.
void ASTrackerBot::SetDestruct()
{
	if (bExploded)
	{
		return;
	}

	bExploded = true;

	UGameplayStatics::SpawnEmitterAtLocation(GetWorld(), ExplosionEffect, GetActorLocation());

	UGameplayStatics::PlaySoundAtLocation(this, ExploedSound, GetActorLocation());

	Meshcomp->SetVisibility(false, true);// 화면상에 안보이게 함
	Meshcomp->SetCollisionEnabled(ECollisionEnabled::NoCollision);

	if (Role == ROLE_Authority) // 이제 서버에서 처리함 한번만 처리해야할 중요한 일임
	{
		// 자기자신은 폭발 데미지 무시함.
		TArray<AActor*> IgnoredActors;
		IgnoredActors.Add(this);

		float ActorDamage = ExplodeDamage + ExplodeDamage * PowerLevel;

		// 주위 반경으로 데미지를 준다. 현재 휘말린 컨트롤러에게
		UGameplayStatics::ApplyRadialDamage(this, ExplodeDamage, GetActorLocation(), ExplodeRadius, nullptr, IgnoredActors, this, GetInstigatorController(), true);

		if (DebugrackerBotDrawing)
		{
			DrawDebugSphere(GetWorld(), GetActorLocation(), ExplodeRadius, 12, FColor::Red, false, 4.0f, 1.0f);
		}
		// 바로 삭제하면 이펙트가 안보여서 생존시간 2초설정
		SetLifeSpan(2.0f);
	}
}

void ASTrackerBot::RefreshPoff()
{
	NextPathPoint = GetNextPathPoint(); // 시작시 다음 이동 정보를 받는다.
}
