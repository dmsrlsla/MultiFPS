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
	// ���� �ù����̼� Ȱ��ȭ
	MeshComp->SetSimulatePhysics(true);
	// ���� Ÿ���� ������ �ٵ�
	MeshComp->SetCollisionObjectType(ECC_PhysicsBody);
	RootComponent = MeshComp;

	RadialForceComp = CreateDefaultSubobject<URadialForceComponent>(TEXT("RadialForceComp"));
	RadialForceComp->SetupAttachment(MeshComp);
	RadialForceComp->Radius = 250; // ���� 250 ������
	RadialForceComp->bImpulseVelChange = true; // ��ݺ�ȭ Ʈ�� ����
	RadialForceComp->bAutoActivate = false; // �ڵ�Ȱ��ȭ ���
	RadialForceComp->bIgnoreOwningActor = true; // �ڱ��ڽ��� �����Ѵ�.

	ExplosionImpulse = 400;

	SetReplicates(true);
	SetReplicateMovement(true); // �̵����� ������ ���� ����
}

void AASExplosiveBarrel::OnRep_Explanded()
{
	// �������� �۵��ϴ� �Լ�!
	// ����Ʈ ȣ��
	UGameplayStatics::SpawnEmitterAtLocation(GetWorld(), ExplosionEffect, GetActorLocation());
	// ������Ʈ������ �ٲ�
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
		// ���� ƨ��� ��ݷ�
		OnRep_Explanded(); // �������� �浹�� �˷��ְ� ����Ʈ�� �߻���Ŵ.
		FVector BoostIntersity = FVector::UpVector * ExplosionImpulse;
		// ���� �ñ�.
		MeshComp->AddImpulse(BoostIntersity, NAME_None, true);


		// ������ ����� ��.
		RadialForceComp->FireImpulse();
	}
}

void AASExplosiveBarrel::GetLifetimeReplicatedProps(TArray< FLifetimeProperty > & OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME(AASExplosiveBarrel, bExploded); // ������Ÿ���� ����ִµ��� ���ø�����Ʈ ������ �����´�. ���縦 �ϸ� ���� Ŭ���̾�Ʈ�� ȭ�鿡���� �������Ѵ�.
}