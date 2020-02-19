// Fill out your copyright notice in the Description page of Project Settings.


#include "SPickUpActor.h"
#include "Components/SphereComponent.h"
#include "Components/DecalComponent.h"
#include "SPowerUpActor.h"
#include "TimerManager.h"
#include "Net/UnrealNetwork.h"

// Sets default values
ASPickUpActor::ASPickUpActor()
{
	SphereComp = CreateDefaultSubobject<USphereComponent>(TEXT("SphereComp"));
	SphereComp->SetSphereRadius(75.0f);
	RootComponent = SphereComp;

	DecalComp = CreateDefaultSubobject<UDecalComponent>(TEXT("DecalComp"));
	DecalComp->SetRelativeRotation(FRotator(90, 0.0f, 0.0f));
	DecalComp->DecalSize = FVector(64, 75, 75);
	DecalComp->SetupAttachment(RootComponent);

	CooldownDuration = 10.0f;

	SetReplicates(true);
}

// Called when the game starts or when spawned
void ASPickUpActor::BeginPlay()
{
	Super::BeginPlay();

	if (Role == ROLE_Authority) // ���������� �����մϴ�. �ߺ����� ����.
	{
		Respawn();
	}
}

void ASPickUpActor::Respawn()
{
	if (PowerUpClass == nullptr)
	{
		UE_LOG(LogTemp, Warning, TEXT("PowerUpClass is nullptr in %s. Please update your Blueprint"), *GetName());
		return;
	}

	FActorSpawnParameters SpawnParams;
	SpawnParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;

	PowerUpInstance = GetWorld()->SpawnActor<ASPowerUpActor>(PowerUpClass, GetTransform(), SpawnParams);
}

// ������ �̺�Ʈ�� �˷��ݴϴ�. �ش� ���ǿ� �ٸ� ���Ͱ� ���������
void ASPickUpActor::NotifyActorBeginOverlap(AActor * OtherActor)
{
	Super::NotifyActorBeginOverlap(OtherActor);

	// @TODO: �Ŀ��� Ȱ��ȭ �մϴ�.
	if (Role == ROLE_Authority && PowerUpInstance)
	{
		// ������ ���� ���͸� �������ݴϴ�.
		PowerUpInstance->ActivatedPowerup(OtherActor);
		PowerUpInstance = nullptr;

		// ���� �ð��� ������ ������ �ϰ� �մϴ�.
		GetWorldTimerManager().SetTimer(TimerHandle_RespawnTimer, this, &ASPickUpActor::Respawn, CooldownDuration);
	}
}
