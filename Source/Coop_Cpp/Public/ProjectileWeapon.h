// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "SWeapon.h"
#include "ProjectileWeapon.generated.h"

/**
 * 
 */
UCLASS()
class COOP_CPP_API AProjectileWeapon : public ASWeapon
{
	GENERATED_BODY()
	
protected:

	virtual void Fire() override;

	// EditDefaultsOnly 초기값만 바꿔줄수 있다.
	UPROPERTY(EditDefaultsOnly, Category = "Projectile")
	TSubclassOf<AActor> ProjectileClass;
};
