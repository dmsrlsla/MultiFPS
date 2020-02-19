// Fill out your copyright notice in the Description page of Project Settings.


#include "ProjectileWeapon.h"
#include "Components/SkeletalMeshComponent.h"
void AProjectileWeapon::Fire()
{

	// ���ʸ� �����´�.
	AActor* MyOwner = GetOwner();

	if (MyOwner && ProjectileClass)
	{
		// ���� ��ġ�� ȸ����
		FVector EyeLocation;
		FRotator EyeRotation;

		MyOwner->GetActorEyesViewPoint(EyeLocation, EyeRotation);

		FVector MuzzleLocation = MeshComp->GetSocketLocation(MuzzleSocketName);

		FActorSpawnParameters SpawnForms;
		SpawnForms.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;

		GetWorld()->SpawnActor<AActor>(ProjectileClass, MuzzleLocation, EyeRotation, SpawnForms);
	}
}