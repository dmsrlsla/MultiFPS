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

	// 서버에서만 데미지를 줄수있도록 추가한다.

	//if (Role == ROLE_Authority) // 액터만이 Role을 가진다
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
// 읽기전용 엑세스. 아무런 값도 건드리지 않는다.
float UScHealthComponent::GetHealth() const
{
	return Health;
}

void UScHealthComponent::Heal(float HealAmount)
{
	// 힐량이 0이하거나, 현재체력이 0인경우
	if(HealAmount <= 0.0f || Health <= 0.0f)
	{
		return;
	}

	Health = FMath::Clamp(Health + HealAmount, 0.0f, DefaulteHealth);

	UE_LOG(LogTemp, Log, TEXT("Health Changeed : %s (%s)"), *FString::SanitizeFloat(Health), *FString::SanitizeFloat(DefaulteHealth));

	OnHealChanged.Broadcast(this, Health, -HealAmount, nullptr, nullptr, nullptr); // 힐량통보. 기존 데미지 함수에서 -해서 힐량으로 처리함.
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
	// DamageType 피해유형, DamageCauser 피해원인
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

	DOREPLIFETIME(UScHealthComponent, Health); // 라이프타임이 살아있는동안 리플리케이트 변수를 가져온다. 복사를 하면 각자 클라이언트상 화면에서는 정상동작한다.
}

