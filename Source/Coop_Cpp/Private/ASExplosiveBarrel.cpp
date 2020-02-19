// Fill out your copyright notice in the Description page of Project Settings.


#include "ASExplosiveBarrel.h"
#include "ScHealthComponent.h"
#include "Kismet/GameplayStatics.h"
#include "PhysicsEngine/RadialForceComponent.h"
#include "Net/UnrealNetwork.h"

// Sets default values
AASExplosiveBarrel::AASExplosiveBarrel()
{
	HealthComp = CreateDefaultSubobject<UScHealthComponent>(TEXT("HealthComp"));
	HealthComp->OnHealChanged.AddDynamic(this, &AASExplosiveBarrel::OnHealthChanged);

	MeshComp = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("MeshComp"));
	// 물리 시물레이션 활성화
	MeshComp->SetSimulatePhysics(true);
	// 물리 타입은 피지컬 바디
	MeshComp->SetCollisionObjectType(ECC_PhysicsBody);
	RootComponent = MeshComp;

	RadialForceComp = CreateDefaultSubobject<URadialForceComponent>(TEXT("RadialForceComp"));
	RadialForceComp->SetupAttachment(MeshComp);
	RadialForceComp->Radius = 250; // 라디안 250 설정함
	RadialForceComp->bImpulseVelChange = true; // 충격변화 트루 설정
	RadialForceComp->bAutoActivate = false; // 자동활성화 취소
	RadialForceComp->bIgnoreOwningActor = true; // 자기자신은 무시한다.

	ExplosionImpulse = 400;

	SetReplicates(true);
	SetReplicateMovement(true); // 이동량이 서버에 복사 설정
}

void AASExplosiveBarrel::OnRep_Explanded()
{
	// 서버에서 작동하는 함수!
	// 이펙트 호출
	UGameplayStatics::SpawnEmitterAtLocation(GetWorld(), ExplosionEffect, GetActorLocation());
	// 컴포넌트색깔을 바꿈
	MeshComp->SetMaterial(0, ExplosionMaterial);
}

void AASExplosiveBarrel::OnHealthChanged(UScHealthComponent * HealthComps, float Health, float HeahthDelta, const UDamageType * DamageType, AController * InstigatedBy, AActor * DamageCauser)
{
	if (bExploded)
	{
		return;
	}

	if (Health <= 0.0f)
	{
		bExploded = true;
		// 위로 튕기는 충격량
		OnRep_Explanded(); // 서버에서 충돌을 알려주고 이펙트를 발생시킴.
		FVector BoostIntersity = FVector::UpVector * ExplosionImpulse;
		// 위로 팅김.
		MeshComp->AddImpulse(BoostIntersity, NAME_None, true);


		// 주위에 충격을 줌.
		RadialForceComp->FireImpulse();
	}
}

void AASExplosiveBarrel::GetLifetimeReplicatedProps(TArray< FLifetimeProperty > & OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME(AASExplosiveBarrel, bExploded); // 라이프타임이 살아있는동안 리플리케이트 변수를 가져온다. 복사를 하면 각자 클라이언트상 화면에서는 정상동작한다.
}