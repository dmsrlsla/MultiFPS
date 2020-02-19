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
	Meshcomp->SetCanEverAffectNavigation(false); // �׺�޽�������Ʈ�� �׸��� ������� �ʰ���.
	Meshcomp->SetSimulatePhysics(true);
	RootComponent = Meshcomp;
	
	HealComp = CreateDefaultSubobject<UScHealthComponent>(TEXT("HealComp"));
	HealComp->OnHealChanged.AddDynamic(this, &ASTrackerBot::HandleTakeDamage);

	// �浹������ üũ�� ���Ǿ�������Ʈ
	SphereComp = CreateDefaultSubobject<USphereComponent>(TEXT("SphereComp"));
	SphereComp->SetSphereRadius(200);
	SphereComp->SetCollisionEnabled(ECollisionEnabled::QueryOnly); // ���ǻ���
	SphereComp->SetCollisionResponseToAllChannels(ECR_Ignore); // ��� ����
	SphereComp->SetCollisionResponseToChannel(ECC_Pawn, ECR_Overlap); // ���� �����.
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
	
	// ���������� Ž���Ҽ� �ֵ��� ����
	if (Role == ROLE_Authority)
	{
		// ù �̵������� �������ش�.
		NextPathPoint = GetNextPathPoint(); // ���۽� ���� �̵� ������ �޴´�.

		FTimerHandle TimerHandle_CheckPowerLevel;

		GetWorldTimerManager().SetTimer(TimerHandle_CheckPowerLevel, this, &ASTrackerBot::OnCheckNearBot, 1.0f, true); // 1�ʵ��� Ÿ�̸� ������
	}


	//OnActorBeginOverlap.AddDynamic(this, &ASTrackerBot::NotifyActorBeginOverlap);
}

FVector ASTrackerBot::GetNextPathPoint()
{
	// �÷��̾��� ��ġ�� �޾ƿ�
	AActor* BestTarget = nullptr;
	float NearestTargetDistance = FLT_MAX;

	// ���ݺ��ڱ��� �ִ� �����ι� ������� �ݺ���.
	for (FConstPawnIterator It = GetWorld()->GetPawnIterator(); It; ++It)
	{
		APawn* TestPawn = It->Get();
		// ���� �÷��̾ ��Ʈ�� �ǰ� �ִٸ�.
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

		// ���ο� ��ο���, ���� �� ������ �ִٸ� ���� �� ������ ��������.
		// Ŭ���̾�Ʈ�� �������� �α������� Ž����.
		if (NewPath && NewPath->PathPoints.Num() > 1)
		{
			return NewPath->PathPoints[1];
		}
	}
	// ���� ��ΰ� ���ٸ� ���� ��ġ�� �˷���
	return GetActorLocation();
}

void ASTrackerBot::HandleTakeDamage(UScHealthComponent * OwningHealthComp, float Health, float HeahthDelta, const UDamageType * DamageType, AController * InstigatedBy, AActor * DamageCauser)
{
	if (Matinst == nullptr)
	{
		// ���ο� ���׸��� �ν��Ͻ��� �����Ѵ�.
		Matinst = Meshcomp->CreateAndSetMaterialInstanceDynamicFromMaterial(0, Meshcomp->GetMaterial(0));
	}

	if (Matinst)
	{
		Matinst->SetScalarParameterValue("LastTimeDamageTaken", GetWorld()->TimeSeconds); // ���� �ð��� LastTimeDamageTaken �Ķ���Ϳ� �����־�, �ð����̿� ���� ���ڰŸ�(���׸��� Ÿ�� ����)�� �����Ѵ�.
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
	// ��ó ���� üũ�ϴ� �Ÿ�
	const float Radius = 600;

	// ���콺 �ݰ��� �ݶ��̴��� ������ ������ üũ
	FCollisionShape CollShape;
	CollShape.SetSphere(Radius);

	// ���� AI������ ã�´�.
	FCollisionObjectQueryParams QueryParams; // ��� �Ķ����
	QueryParams.AddObjectTypesToQuery(ECC_PhysicsBody);
	QueryParams.AddObjectTypesToQuery(ECC_Pawn);

	TArray<FOverlapResult> OverLaps; // ������ �������
	// �ݰ游ŭ���� �����ٵ�� �� ������ ã�� ���������� ����
	GetWorld()->OverlapMultiByObjectType(OverLaps, GetActorLocation(), FQuat::Identity, QueryParams, CollShape);
	if (DebugrackerBotDrawing)
	{
		DrawDebugSphere(GetWorld(), GetActorLocation(), Radius, 12, FColor::White, false, 0.1f);
	}
	int32 NrOfBots = 0;

	// ����� ���̿��� ����
	for (FOverlapResult Result : OverLaps)
	{
		// Ž���� ���͵��߿��� �� Ÿ���� ã�´�.
		ASTrackerBot* Bot = Cast<ASTrackerBot>(Result.GetActor());

		// ������ ���̰� �ڱ� �ڽ��� �ƴ϶��
		if (Bot && Bot != this)
		{
			NrOfBots++;
		}
	}

	const int32 MaxPowerLevel = 4;
	// 0~4���� �Ŀ�����
	PowerLevel = FMath::Clamp(NrOfBots, 0, MaxPowerLevel);
	// ���׸��� �ν��Ͻ��� ������
	if (Matinst == nullptr)
	{
		Matinst = Meshcomp->CreateAndSetMaterialInstanceDynamicFromMaterial(0, Meshcomp->GetMaterial(0));
	}
	// ����� ���İ� ����.
	if (Matinst)
	{
		float Alpha = PowerLevel / (float)MaxPowerLevel;

		Matinst->SetScalarParameterValue("PowerLevelAlpha", Alpha); // ���� ����
	}
}

// Called every frame
void ASTrackerBot::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);
	// �����̰� ����ó���� �ȵ��ִٸ�
	if (Role == ROLE_Authority && !bExploded)
	{
		OnCheckNearBot();
		FVector DistanceVector = (GetActorLocation() - NextPathPoint);

		float DistanceToTarget = DistanceVector.Size();

		// ���� �����̼��� ���� �̵������� ���� �ʴٸ�.
		if (DistanceToTarget <= RequiredDistanceToTarget)
		{
			NextPathPoint = GetNextPathPoint(); // ���۽� ���� �̵� ������ �޴´�.
		}
		else // �����ϰų� �ſ� ������ �ִٸ�.
		{
			FVector ForceDirection = NextPathPoint - GetActorLocation(); // ���� ġ���� ���� �̵������� ���� ���⺤�͸� ����.
			ForceDirection.Normalize();

			ForceDirection *= MovementForce;

			// ��� �̵��Ѵ� ���� ��������
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
	// ����� ������.
	if (!bStartedSelfDestroy)
	{
		ASCharacter* PlayerPawn = Cast<ASCharacter>(OtherActor);
		if (PlayerPawn && !UScHealthComponent::IsFriendly(OtherActor, this))
		{
			if (Role == ROLE_Authority) // ���� �������� ó���� Ÿ�̸� �߰�
			{
				// �÷��̾ ã������ �ڱ� �ڽſ��� Ÿ�̸Ӹ� �Ǵ�.
				GetWorldTimerManager().SetTimer(TimerHandle_SelfDamage, this, &ASTrackerBot::DamageSelf, SelfDamageInterval, true, 0.0f); // �ڱ⿡�� �������� �����ؼ� �ش�. �����غ���.
			}

				bStartedSelfDestroy = true;

				UGameplayStatics::SpawnSoundAttached(SelfDestructSound, RootComponent);

		}
	}
}

void ASTrackerBot::GetLifetimeReplicatedProps(TArray< FLifetimeProperty > & OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	//DOREPLIFETIME(ASTrackerBot, CurrentWeapon); // ������Ÿ���� ����ִµ��� ���ø�����Ʈ ������ �����´�. ���縦 �ϸ� ���� Ŭ���̾�Ʈ�� ȭ�鿡���� �������Ѵ�.
	//DOREPLIFETIME(ASTrackerBot, bDied);
}

// �� ���� ����. ���� �������� ����.
void ASTrackerBot::SetDestruct()
{
	if (bExploded)
	{
		return;
	}

	bExploded = true;

	UGameplayStatics::SpawnEmitterAtLocation(GetWorld(), ExplosionEffect, GetActorLocation());

	UGameplayStatics::PlaySoundAtLocation(this, ExploedSound, GetActorLocation());

	Meshcomp->SetVisibility(false, true);// ȭ��� �Ⱥ��̰� ��
	Meshcomp->SetCollisionEnabled(ECollisionEnabled::NoCollision);

	if (Role == ROLE_Authority) // ���� �������� ó���� �ѹ��� ó���ؾ��� �߿��� ����
	{
		// �ڱ��ڽ��� ���� ������ ������.
		TArray<AActor*> IgnoredActors;
		IgnoredActors.Add(this);

		float ActorDamage = ExplodeDamage + ExplodeDamage * PowerLevel;

		// ���� �ݰ����� �������� �ش�. ���� �ָ��� ��Ʈ�ѷ�����
		UGameplayStatics::ApplyRadialDamage(this, ExplodeDamage, GetActorLocation(), ExplodeRadius, nullptr, IgnoredActors, this, GetInstigatorController(), true);

		if (DebugrackerBotDrawing)
		{
			DrawDebugSphere(GetWorld(), GetActorLocation(), ExplodeRadius, 12, FColor::Red, false, 4.0f, 1.0f);
		}
		// �ٷ� �����ϸ� ����Ʈ�� �Ⱥ����� �����ð� 2�ʼ���
		SetLifeSpan(2.0f);
	}
}

void ASTrackerBot::RefreshPoff()
{
	NextPathPoint = GetNextPathPoint(); // ���۽� ���� �̵� ������ �޴´�.
}
