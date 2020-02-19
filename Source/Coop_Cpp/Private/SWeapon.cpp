// Fill out your copyright notice in the Description page of Project Settings.


#include "SWeapon.h"
#include "Components/SkeletalMeshComponent.h"
#include "DrawDebugHelpers.h"
#include "Kismet/GameplayStatics.h"
#include "Particles/ParticleSystem.h"
#include "Components/SkeletalMeshComponent.h"
#include "Particles/ParticleSystemComponent.h"
#include "PhysicalMaterials/PhysicalMaterial.h"
#include "Coop_Cpp.h"
#include "TimerManager.h"
#include "Net/UnrealNetwork.h"

// Sets default values

static int32 DebugWeaponDrawing = 0;
FAutoConsoleVariableRef CVARDebugWeaponLine(
	TEXT("COOP.DebugWeapons"), 
	DebugWeaponDrawing, 
	TEXT("Draw Debug Line For Weapons"), 
	ECVF_Cheat);

ASWeapon::ASWeapon()
{

	MeshComp = CreateDefaultSubobject<USkeletalMeshComponent>(TEXT("MeshComp"));
	RootComponent = MeshComp;

	MuzzleSocketName = "MuzzleSocket";
	TraceTargetName = "Target";

	BaseDamage = 20.0f;
	BulletSpread = 2.0f;
	RateOfFire = 600.0f;

	// 리플리케이트 액터로 설정함.
	SetReplicates(true);

	NetUpdateFrequency = 66.0f;
	MinNetUpdateFrequency = 33.0f;
}

void ASWeapon::BeginPlay()
{
	Super::BeginPlay();

	TimeBetweenShots = 60 / RateOfFire; // RateOfFire가 빠를수록 발사 RPM이 빨라짐.
}

void ASWeapon::Fire()
{
	// 추적 the World, 폰의 눈에서 크로스 헤어에 위치로!

	if (Role < ROLE_Authority)
	{
		Server_Fire(); // 서버에 전달할 Fire를 보냄
	}

	AActor* MyOwner = GetOwner();

	if (MyOwner)
	{
		// 눈의 위치와 회전값
		FVector EyeLocation;
		FRotator EyeRotation;

		MyOwner->GetActorEyesViewPoint(EyeLocation, EyeRotation);

		FVector ShotDirection = EyeRotation.Vector();
		
		// 탄퍼짐
		float HalfRaid = FMath::DegreesToRadians(BulletSpread);
		ShotDirection = FMath::VRandCone(ShotDirection, HalfRaid, HalfRaid);

		// 추적의 끝. 눈의 위치부터 눈의 회전방향으로 직선인 지점.
		FVector TraceEnd = EyeLocation + (EyeRotation.Vector() * 10000);
		
		// 충돌 판정 기준
		FCollisionQueryParams QueryParams;
		QueryParams.AddIgnoredActor(MyOwner); // 플레이어를 무시한다.
		QueryParams.AddIgnoredActor(this); // 무기 자신을 무시한다.
		QueryParams.bTraceComplex = true;
		QueryParams.bReturnPhysicalMaterial = true;

		FVector TracerEndPoint;

		EPhysicalSurface SurfaceType = SurfaceType_Default;
		
		FHitResult Hit;
		if (GetWorld()->LineTraceSingleByChannel(Hit, EyeLocation, TraceEnd, COllISION_WEAPON, QueryParams))
		{
			// 뭔가 충돌됬으면 데미지를 준다.

			AActor* HitActor = Hit.GetActor();

			SurfaceType = UPhysicalMaterial::DetermineSurfaceType(Hit.PhysMaterial.Get());

			float ActualDamage = BaseDamage;

			// 나중에 서버인지 클라이언트인지 판별해서 한곳에서만 처리하도록함.
			if (SurfaceType == SURFACE_FLESHVULNERABLE) // 충돌 서페이스가 머리라면
			{
				ActualDamage *= 4.0f; // 헤드샷 4배 데미지
			}

			// 어떤 방향으로 정확히 어떤 지점에 부딪쳤는지에 대해 더 많은 정보를 전달 할 수 있다는 점에서 유리
			// DamageType : 데미지가 어떤방식으로 들어갈지 결정하는 데이터 클래스.

			UGameplayStatics::ApplyPointDamage(HitActor, ActualDamage, ShotDirection, Hit, MyOwner->GetInstigatorController(), MyOwner, DamageType);

			PlayImpactEffect(SurfaceType, Hit.ImpactPoint);

			TracerEndPoint = Hit.ImpactPoint;

		}

		if (DebugWeaponDrawing > 0 )
		{
			DrawDebugLine(GetWorld(), EyeLocation, TraceEnd, FColor::White, false, 1.8f, 0, 1.0f);
		}
	
		PlayFireEffect(TracerEndPoint);

		if (Role == ROLE_Authority)
		{
			HitScanTrace.TraceTo = TracerEndPoint;
			HitScanTrace.Surface = SurfaceType;
		}

		LastTimeFired = GetWorld()->TimeSeconds;
	}
}

void ASWeapon::Server_Fire_Implementation()
{
	Fire(); // 서버 전달용이기에 실제 내용과 동일.
}

bool ASWeapon::Server_Fire_Validate()
{
	return true; // 서버가 유효하다면 트루를 보내고, 아니면 연결 끊김.
}

void ASWeapon::OnRep_HitScanTrace()
{
	// 플레이하는동안 계속 네트워크에 받을수 있도록 함.
	PlayFireEffect(HitScanTrace.TraceTo);
	PlayImpactEffect(HitScanTrace.Surface, HitScanTrace.TraceTo);
}

void ASWeapon::StartFire()
{
	float FirstDelay = FMath::Max(LastTimeFired + TimeBetweenShots - GetWorld()->TimeSeconds, 0.0f); // 첫발의 딜레이. 마지막 샷 발사시간 + 샷발간격에서 시간을 뺀것.

	GetWorldTimerManager().SetTimer(TimerHandle_TimeBetWeendShot, this, &ASWeapon::Fire, TimeBetweenShots, true, FirstDelay); //총 발사가 시작되면 타이머 설정해 1초마다 FIRE
}

void ASWeapon::StopFire()
{
	GetWorldTimerManager().ClearTimer(TimerHandle_TimeBetWeendShot); // 멈춤
}


void ASWeapon::PlayFireEffect(FVector TrandEnd)
{
	if (MuzzleEffect) // 이펙트가 실제로 존재하는지 확인함.
	{
		UGameplayStatics::SpawnEmitterAttached(MuzzleEffect, MeshComp, MuzzleSocketName);
	}

	FVector MuzzleLocation = MeshComp->GetSocketLocation(MuzzleSocketName);

	if (TraceEffect)
	{
		// 라인 이펙트는, 머즐의 소켓 위치부터, 
		UParticleSystemComponent* TraceComp = UGameplayStatics::SpawnEmitterAtLocation(GetWorld(), TraceEffect, MuzzleLocation);

		if (TraceComp)
		{
			// 목표가 있ㄱ도 최종위치에 변수를 전달해야한다. 그것은 히트임팩트 포인트가 될것이다.
			TraceComp->SetVectorParameter(TraceTargetName, TrandEnd);
		}
	}

	APawn* MyOnwer = Cast<APawn>(GetOwner());
	if (MyOnwer)
	{
		APlayerController* PC =  Cast<APlayerController>(MyOnwer->GetController());
		if (PC)
		{
			PC->ClientPlayCameraShake(FireCamShake); // 클라이언트의 카메라를 흔든다.
		}
	}
}

void ASWeapon::PlayImpactEffect(EPhysicalSurface SurfaceType, FVector ImpactPoint)
{
	UParticleSystem* SelectedEffect = nullptr;

	switch (SurfaceType)
	{
	case SURFACE_FLESHDEFAULT:
	case SURFACE_FLESHVULNERABLE:
		SelectedEffect = FleshImpactEffect;
		break;
	default:
		SelectedEffect = DefaultImpactEffect;
		break;
	}

	if (SelectedEffect)
	{
		// 카메리아에서 재생하는 인터페이스와 실제 라인 트레이스를 시각적으로 나타낸다.
		FVector MuzzleLocation = MeshComp->GetSocketLocation(MuzzleSocketName);

		// 충격지점부터 시작지점까지 빼서 방향을 계산
		FVector ShotDirection = ImpactPoint - MuzzleLocation;
		// 노멀라이즈
		ShotDirection.Normalize();

		// 월드상에, 임팩트 이펙트를 발생시킴, 힛포인트에서, 회전률은 힛 노멀에서
		// 매개변수를 직접 받으므로 히트 포인트는 제거함
		UGameplayStatics::SpawnEmitterAtLocation(GetWorld(), SelectedEffect, ImpactPoint, ShotDirection.Rotation());
	}
}

void ASWeapon::GetLifetimeReplicatedProps(TArray< FLifetimeProperty > & OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME_CONDITION(ASWeapon, HitScanTrace, COND_SkipOwner); // 라이프타임이 살아있는동안 리플리케이트 변수를 가져온다. 복사를 하면 각자 클라이언트상 화면에서는 정상동작한다.
}