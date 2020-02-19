// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "SPowerUpActor.generated.h"

UCLASS()
class COOP_CPP_API ASPowerUpActor : public AActor
{
	GENERATED_BODY()
	
public:	
	// Sets default values for this actor's properties
	ASPowerUpActor();

protected:
	// Called when the game starts or when spawned
	virtual void BeginPlay() override;

	UPROPERTY(EditDefaultsOnly, Category="PowerUps")
	float PowerUpInterval;

	/* �Ŀ��� ���� �Դϴ�. */
	UPROPERTY(EditDefaultsOnly, Category = "PowerUps")
	int32 TotalNrofTicks;

	int32 Ticksprossed;

	FTimerHandle TimerHandle_PowerUp;

	void OnTickPowerup();
	// ���ø�����Ʈ �Լ� ���.
	UPROPERTY(ReplicatedUsing=OnRep_PowerupActive)
	bool bIsPowerActive;

	UFUNCTION()
	void OnRep_PowerupActive();
	UFUNCTION(BlueprintImplementableEvent, Category = "PowerUps")
	void OnPowerUpStateChanged(bool bNewIsActive);

public:	

	void ActivatedPowerup(AActor* ActiveFor);

	// �������Ʈ���� ȣ���ؼ� ������ �Լ��� �����.
	
	UFUNCTION(BlueprintImplementableEvent, Category = "PowerUps")
	void OnActivated(AActor* ActiveFor);

	UFUNCTION(BlueprintImplementableEvent, Category = "PowerUps")
	void OnPowerUpTicked();

	UFUNCTION(BlueprintImplementableEvent, Category = "PowerUps")
	void OnExpired();

	virtual void GetLifetimeReplicatedProps(TArray< FLifetimeProperty > & OutLifetimeProps) const override;
};
