// Fill out your copyright notice in the Description page of Project Settings.


#include "ScHealthComponent.h"
#include "Components/ActorComponent.h"
#include "Net/UnrealNetwork.h"
#include "SGameMode.h"

// Sets default values for this component's properties
UScHealthComponent::UScHealthComponent()
{
	DefaulteHealth = 100;
	IsDead = false;

	TeamNum = 255;

	SetIsReplicated(true);
}


// Called when the game starts
void UScHealthComponent::BeginPlay()
{
	Super::BeginPlay();

	// ���������� �������� �ټ��ֵ��� �߰��Ѵ�.

	//if (Role == ROLE_Authority) // ���͸��� Role�� ������
	if(GetOwnerRole() == ROLE_Authority)
	{
		AActor* MyOwner = GetOwner();

		if (MyOwner)
		{
			MyOwner->OnTakeAnyDamage.AddDynamic(this, &UScHealthComponent::HandleTakeAnyDamage);
		}

		Health = DefaulteHealth;
	}


}

void UScHealthComponent::OnRepHealth(float OldHealth)
{
	float Damage = Health - OldHealth;
	OnHealChanged.Broadcast(this, Health, Damage, nullptr, nullptr, nullptr);
}
// �б����� ������. �ƹ��� ���� �ǵ帮�� �ʴ´�.
float UScHealthComponent::GetHealth() const
{
	return Health;
}

void UScHealthComponent::Heal(float HealAmount)
{
	// ������ 0���ϰų�, ����ü���� 0�ΰ��
	if(HealAmount <= 0.0f || Health <= 0.0f)
	{
		return;
	}

	Health = FMath::Clamp(Health + HealAmount, 0.0f, DefaulteHealth);

	UE_LOG(LogTemp, Log, TEXT("Health Changeed : %s (%s)"), *FString::SanitizeFloat(Health), *FString::SanitizeFloat(DefaulteHealth));

	OnHealChanged.Broadcast(this, Health, -HealAmount, nullptr, nullptr, nullptr); // �����뺸. ���� ������ �Լ����� -�ؼ� �������� ó����.
}

bool UScHealthComponent::IsFriendly(AActor* ActorA, AActor* ActorB)
{
	if (ActorA == nullptr || ActorB == nullptr)
	{
		return true;
	}

	UScHealthComponent* HealthA = Cast<UScHealthComponent>(ActorA->GetComponentByClass(UScHealthComponent::StaticClass()));
	UScHealthComponent* HealthB = Cast<UScHealthComponent>(ActorB->GetComponentByClass(UScHealthComponent::StaticClass()));

	if (HealthA == nullptr || HealthB == nullptr)
	{
		return true;
	}

	return HealthA->TeamNum == HealthB->TeamNum;
}

void UScHealthComponent::HandleTakeAnyDamage(AActor* DamagedActor, float Damage, const class UDamageType* DamageType, class AController* InstigatedBy, AActor* DamageCauser)
{
	if (Damage <= 0.0f || IsDead)
	{
		return;
	}

	if (DamageCauser != DamagedActor && IsFriendly(DamagedActor, DamageCauser))
	{
		return;
	}

	// Update Health Clamp
	Health = FMath::Clamp(Health - Damage, 0.0f, DefaulteHealth);

	IsDead = Health <= 0.0f;

	UE_LOG(LogTemp, Log, TEXT("Health Changeed : %s"), *FString::SanitizeFloat(Health));
	// DamageType ��������, DamageCauser ���ؿ���
	OnHealChanged.Broadcast(this, Health, Damage, DamageType, InstigatedBy, DamageCauser);

	if (IsDead)
	{
		ASGameMode* GM = Cast<ASGameMode>(GetWorld()->GetAuthGameMode());
		if (GM)
		{
			GM->OnActorKilled.Broadcast(GetOwner(), DamageCauser, InstigatedBy);
		}
	}
}

void UScHealthComponent::GetLifetimeReplicatedProps(TArray< FLifetimeProperty > & OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME(UScHealthComponent, Health); // ������Ÿ���� ����ִµ��� ���ø�����Ʈ ������ �����´�. ���縦 �ϸ� ���� Ŭ���̾�Ʈ�� ȭ�鿡���� �������Ѵ�.
}

