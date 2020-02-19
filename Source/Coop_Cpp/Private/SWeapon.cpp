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

	// ���ø�����Ʈ ���ͷ� ������.
	SetReplicates(true);

	NetUpdateFrequency = 66.0f;
	MinNetUpdateFrequency = 33.0f;
}

void ASWeapon::BeginPlay()
{
	Super::BeginPlay();

	TimeBetweenShots = 60 / RateOfFire; // RateOfFire�� �������� �߻� RPM�� ������.
}

void ASWeapon::Fire()
{
	// ���� the World, ���� ������ ũ�ν� �� ��ġ��!

	if (Role < ROLE_Authority)
	{
		Server_Fire(); // ������ ������ Fire�� ����
	}

	AActor* MyOwner = GetOwner();

	if (MyOwner)
	{
		// ���� ��ġ�� ȸ����
		FVector EyeLocation;
		FRotator EyeRotation;

		MyOwner->GetActorEyesViewPoint(EyeLocation, EyeRotation);

		FVector ShotDirection = EyeRotation.Vector();
		
		// ź����
		float HalfRaid = FMath::DegreesToRadians(BulletSpread);
		ShotDirection = FMath::VRandCone(ShotDirection, HalfRaid, HalfRaid);

		// ������ ��. ���� ��ġ���� ���� ȸ���������� ������ ����.
		FVector TraceEnd = EyeLocation + (EyeRotation.Vector() * 10000);
		
		// �浹 ���� ����
		FCollisionQueryParams QueryParams;
		QueryParams.AddIgnoredActor(MyOwner); // �÷��̾ �����Ѵ�.
		QueryParams.AddIgnoredActor(this); // ���� �ڽ��� �����Ѵ�.
		QueryParams.bTraceComplex = true;
		QueryParams.bReturnPhysicalMaterial = true;

		FVector TracerEndPoint;

		EPhysicalSurface SurfaceType = SurfaceType_Default;
		
		FHitResult Hit;
		if (GetWorld()->LineTraceSingleByChannel(Hit, EyeLocation, TraceEnd, COllISION_WEAPON, QueryParams))
		{
			// ���� �浹������ �������� �ش�.

			AActor* HitActor = Hit.GetActor();

			SurfaceType = UPhysicalMaterial::DetermineSurfaceType(Hit.PhysMaterial.Get());

			float ActualDamage = BaseDamage;

			// ���߿� �������� Ŭ���̾�Ʈ���� �Ǻ��ؼ� �Ѱ������� ó���ϵ�����.
			if (SurfaceType == SURFACE_FLESHVULNERABLE) // �浹 �����̽��� �Ӹ����
			{
				ActualDamage *= 4.0f; // ��弦 4�� ������
			}

			// � �������� ��Ȯ�� � ������ �ε��ƴ����� ���� �� ���� ������ ���� �� �� �ִٴ� ������ ����
			// DamageType : �������� �������� ���� �����ϴ� ������ Ŭ����.

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
	Fire(); // ���� ���޿��̱⿡ ���� ����� ����.
}

bool ASWeapon::Server_Fire_Validate()
{
	return true; // ������ ��ȿ�ϴٸ� Ʈ�縦 ������, �ƴϸ� ���� ����.
}

void ASWeapon::OnRep_HitScanTrace()
{
	// �÷����ϴµ��� ��� ��Ʈ��ũ�� ������ �ֵ��� ��.
	PlayFireEffect(HitScanTrace.TraceTo);
	PlayImpactEffect(HitScanTrace.Surface, HitScanTrace.TraceTo);
}

void ASWeapon::StartFire()
{
	float FirstDelay = FMath::Max(LastTimeFired + TimeBetweenShots - GetWorld()->TimeSeconds, 0.0f); // ù���� ������. ������ �� �߻�ð� + ���߰��ݿ��� �ð��� ����.

	GetWorldTimerManager().SetTimer(TimerHandle_TimeBetWeendShot, this, &ASWeapon::Fire, TimeBetweenShots, true, FirstDelay); //�� �߻簡 ���۵Ǹ� Ÿ�̸� ������ 1�ʸ��� FIRE
}

void ASWeapon::StopFire()
{
	GetWorldTimerManager().ClearTimer(TimerHandle_TimeBetWeendShot); // ����
}


void ASWeapon::PlayFireEffect(FVector TrandEnd)
{
	if (MuzzleEffect) // ����Ʈ�� ������ �����ϴ��� Ȯ����.
	{
		UGameplayStatics::SpawnEmitterAttached(MuzzleEffect, MeshComp, MuzzleSocketName);
	}

	FVector MuzzleLocation = MeshComp->GetSocketLocation(MuzzleSocketName);

	if (TraceEffect)
	{
		// ���� ����Ʈ��, ������ ���� ��ġ����, 
		UParticleSystemComponent* TraceComp = UGameplayStatics::SpawnEmitterAtLocation(GetWorld(), TraceEffect, MuzzleLocation);

		if (TraceComp)
		{
			// ��ǥ�� �֤��� ������ġ�� ������ �����ؾ��Ѵ�. �װ��� ��Ʈ����Ʈ ����Ʈ�� �ɰ��̴�.
			TraceComp->SetVectorParameter(TraceTargetName, TrandEnd);
		}
	}

	APawn* MyOnwer = Cast<APawn>(GetOwner());
	if (MyOnwer)
	{
		APlayerController* PC =  Cast<APlayerController>(MyOnwer->GetController());
		if (PC)
		{
			PC->ClientPlayCameraShake(FireCamShake); // Ŭ���̾�Ʈ�� ī�޶� ����.
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
		// ī�޸��ƿ��� ����ϴ� �������̽��� ���� ���� Ʈ���̽��� �ð������� ��Ÿ����.
		FVector MuzzleLocation = MeshComp->GetSocketLocation(MuzzleSocketName);

		// ����������� ������������ ���� ������ ���
		FVector ShotDirection = ImpactPoint - MuzzleLocation;
		// ��ֶ�����
		ShotDirection.Normalize();

		// �����, ����Ʈ ����Ʈ�� �߻���Ŵ, ������Ʈ����, ȸ������ �� ��ֿ���
		// �Ű������� ���� �����Ƿ� ��Ʈ ����Ʈ�� ������
		UGameplayStatics::SpawnEmitterAtLocation(GetWorld(), SelectedEffect, ImpactPoint, ShotDirection.Rotation());
	}
}

void ASWeapon::GetLifetimeReplicatedProps(TArray< FLifetimeProperty > & OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME_CONDITION(ASWeapon, HitScanTrace, COND_SkipOwner); // ������Ÿ���� ����ִµ��� ���ø�����Ʈ ������ �����´�. ���縦 �ϸ� ���� Ŭ���̾�Ʈ�� ȭ�鿡���� �������Ѵ�.
}