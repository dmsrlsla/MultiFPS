// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "ScHealthComponent.h"
#include "ASExplosiveBarrel.generated.h"

class UScHealthComponent;
class UStaticMeshComponent;
class URadialForceComponent;
class UParticleSystem;

UCLASS()
class COOP_CPP_API AASExplosiveBarrel : public AActor
{
	GENERATED_BODY()
	
public:	
	// Sets default values for this actor's properties
	AASExplosiveBarrel();

protected:

	UPROPERTY(VisibleAnywhere, Category = "Componenets")
	UScHealthComponent* HealthComp;

	UPROPERTY(VisibleAnywhere, Category = "Componenets")
	UStaticMeshComponent* MeshComp;

	UPROPERTY(VisibleAnywhere, Category = "Componenets")
	URadialForceComponent* RadialForceComp;

	UPROPERTY(ReplicatedUsing = OnRep_Explanded)
	bool bExploded;

	UFUNCTION()
	void OnRep_Explanded();

	UPROPERTY(EditDefaultsOnly, Category = "FX")
	float ExplosionImpulse = 400;

	UPROPERTY(EditDefaultsOnly, Category = "FX")
	UParticleSystem* ExplosionEffect;

	UPROPERTY(EditDefaultsOnly, Category = "FX")
	UMaterialInterface* ExplosionMaterial;

public:	
	UFUNCTION()
	void OnHealthChanged(UScHealthComponent* HealthComps, float Health, float HeahthDelta, const class UDamageType* DamageType, class AController* InstigatedBy, AActor* DamageCauser);

};
