// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "Engine.h"
#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "ScHealthComponent.generated.h"

// 유형의 이름, 파라미터의 갯수의 따라 몇단인지 설정함.
DECLARE_DYNAMIC_MULTICAST_DELEGATE_SixParams(FOnHealChagneSignature, UScHealthComponent*, HealthComp, float, Health, float, HeahthDelta, const class UDamageType*, DamageType, class AController*, InstigatedBy, AActor*, DamageCauser);

UCLASS( ClassGroup=(Custom), meta=(BlueprintSpawnableComponent) )
class COOP_CPP_API UScHealthComponent : public UActorComponent
{
	GENERATED_BODY()

public:	
	// Sets default values for this component's properties
	UScHealthComponent();

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "HealthCompnent")
	uint8 TeamNum;

	UFUNCTION()
	void HandleTakeAnyDamage(AActor* DamagedActor, float Damage, const class UDamageType* DamageType, class AController* InstigatedBy, AActor* DamageCauser);

protected:
	// Called when the game starts
	virtual void BeginPlay() override;


	UPROPERTY(ReplicatedUsing = OnRepHealth, EditAnywhere, BlueprintReadWrite, Category = "HealthComponent")
	float Health;

	UFUNCTION()
	void OnRepHealth(float OldHealth);

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "HealthComponent")
	float DefaulteHealth;

public:

	float GetHealth() const;

	// 노출시킬 속성
	UPROPERTY(EditAnywhere, BlueprintAssignable, Category = "Events")
	FOnHealChagneSignature OnHealChanged;

	UFUNCTION(BlueprintCallable, Category = "HealthComponent")
	void Heal(float HealAmount);

	bool IsDead;

	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "HealthComponent")
	static bool IsFriendly(AActor* ActorA, AActor* ActorB);
};
