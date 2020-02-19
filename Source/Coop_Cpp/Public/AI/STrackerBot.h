// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Pawn.h"
#include "STrackerBot.generated.h"

class UScHealthComponent;
class USphereComponent;
class USoundCue;
UCLASS()
class COOP_CPP_API ASTrackerBot : public APawn
{
	GENERATED_BODY()

public:
	// Sets default values for this pawn's properties
	ASTrackerBot();

protected:
	// Called when the game starts or when spawned
	virtual void BeginPlay() override;

	UPROPERTY(VisibleDefaultsOnly, Category = "Component")
	UStaticMeshComponent* Meshcomp;

	UPROPERTY(VisibleDefaultsOnly, Category = "Component")
	UScHealthComponent* HealComp;

	UPROPERTY(VisibleDefaultsOnly, Category = "Component")
	USphereComponent* SphereComp;

	FVector GetNextPathPoint();

	// 네비게이션의 다음 이동지점
	FVector NextPathPoint;

	UPROPERTY(EditDefaultsOnly, Category = "TrackingBot")
	float MovementForce;
	UPROPERTY(EditDefaultsOnly, Category = "TrackingBot")
	bool bUseVelocityChange;
	UPROPERTY(EditDefaultsOnly, Category = "TrackingBot")
	float RequiredDistanceToTarget;

	UFUNCTION()
	void HandleTakeDamage(UScHealthComponent* OwningHealthComp, float Health, float HeahthDelta, const class UDamageType* DamageType, class AController* InstigatedBy, AActor* DamageCauser);

	UMaterialInstanceDynamic* Matinst;

	virtual void GetLifetimeReplicatedProps(TArray< FLifetimeProperty > & OutLifetimeProps) const override;

	void SetDestruct();

	UPROPERTY(EditDefaultsOnly, Category = "FX")
	UParticleSystem* ExplosionEffect;

	UPROPERTY(EditDefaultsOnly, Category = "FX")
	USoundCue* SelfDestructSound;

	UPROPERTY(EditDefaultsOnly, Category = "FX")
	USoundCue* ExploedSound;

	bool bExploded;

	bool bStartedSelfDestroy;

	UPROPERTY(EditDefaultsOnly, Category = "TrackingBot")
	float ExplodeDamage;

	UPROPERTY(EditDefaultsOnly, Category = "TrackingBot")
	float SelfDamageInterval;

	UPROPERTY(EditDefaultsOnly, Category = "TrackingBot")
	float ExplodeRadius;



	FTimerHandle TimerHandle_SelfDamage;

	void DamageSelf();

	void OnCheckNearBot();

	int32 PowerLevel;

	FTimerHandle TimerHandle_Refresh;
public:	
	// Called every frame
	virtual void Tick(float DeltaTime) override;

	virtual void NotifyActorBeginOverlap(AActor* OtherActor) override;

	void RefreshPoff();
};
