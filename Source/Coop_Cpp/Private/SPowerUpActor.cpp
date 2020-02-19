// Fill out your copyright notice in the Description page of Project Settings.


#include "SPowerUpActor.h"
#include "TimerManager.h"
#include "Net/UnrealNetwork.h"

// Sets default values
ASPowerUpActor::ASPowerUpActor()
{
	PowerUpInterval = 0.0f;
	TotalNrofTicks = 0;

	bIsPowerActive = false;

	SetReplicates(true);
}

// Called when the game starts or when spawned
void ASPowerUpActor::BeginPlay()
{
	Super::BeginPlay();
}

void ASPowerUpActor::OnTickPowerup()
{
	Ticksprossed++;

	OnPowerUpTicked();

	if (Ticksprossed >= TotalNrofTicks)
	{
		// ���� �ܰ� ȣ��
		OnExpired();

		bIsPowerActive = false; // ��Ȱ��ȭ �����̸� �Ŀ��� �� Ȱ��ȭ ���¶�� �˷��ֵ��� �����Ѵ�. Rep��  ����

		OnRep_PowerupActive();// �������� �˸������ �����Ƿ� Rep�� ������ ������ Ų��. // bIsPowerActive �������� ����.

		// Ÿ�̸Ӹ� Ŭ���� �մϴ�.
		GetWorldTimerManager().ClearTimer(TimerHandle_PowerUp);
	}
}

// ������ �����ϴ� Rep �Լ�
void ASPowerUpActor::OnRep_PowerupActive()
{
	OnPowerUpStateChanged(bIsPowerActive); // �������Ʈ���� �ǹ��� �����ؼ� �ش� �׸��� �����Ѵ�.
}

// �ش� �Լ����� ���¸� ������ �ǰ�, ���� ������ �������Ʈ���� �������� ������. �߻�ȭ.
void ASPowerUpActor::ActivatedPowerup(AActor* ActiveFor)
{
	OnActivated(ActiveFor);

	bIsPowerActive = true; // Ȱ��ȭ �ϴµ���, ������ ���� �Ŀ��� �ߵ����̶�� Ʈ����¸� ����. �������Ʈ������ �޽ø� ������. Ȱ��ȭ �����϶�.

	OnRep_PowerupActive();// �������� �˸������ �����Ƿ� Rep�� ������ ������ Ų��. // bIsPowerActive �������� ����.

	// �������Ʈ���� ������ �Ŀ��� ���͹� ���� 0���� ũ�� Ÿ�̸Ӹ� �����Ͽ� �ش� ���ݸ�ŭ �Ŀ����� ������.
	if (PowerUpInterval > 0.0f)
	{
		GetWorldTimerManager().SetTimer(TimerHandle_PowerUp, this, &ASPowerUpActor::OnTickPowerup, PowerUpInterval, true);
	}
	else
	{
		OnTickPowerup();
	}
}

void ASPowerUpActor::GetLifetimeReplicatedProps(TArray< FLifetimeProperty > & OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);
	DOREPLIFETIME(ASPowerUpActor, bIsPowerActive); // Ȱ��ȭ�ϰ� �ٸ�������� ����
}

