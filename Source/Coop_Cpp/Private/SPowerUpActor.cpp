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
		// 만료 단계 호출
		OnExpired();

		bIsPowerActive = false; // 비활성화 상태이면 파워를 비 활성화 상태라고 알려주도록 변경한다. Rep에  전달

		OnRep_PowerupActive();// 서버에는 알림기능이 없으므로 Rep을 해제해 전원을 킨다. // bIsPowerActive 변동사항 적용.

		// 타이머를 클리어 합니다.
		GetWorldTimerManager().ClearTimer(TimerHandle_PowerUp);
	}
}

// 서버로 전달하는 Rep 함수
void ASPowerUpActor::OnRep_PowerupActive()
{
	OnPowerUpStateChanged(bIsPowerActive); // 블루프린트에서 실물을 구현해서 해당 항목을 구현한다.
}

// 해당 함수들은 형태만 유지가 되고, 실제 구현은 블루프린트에서 상세적으로 구현함. 추상화.
void ASPowerUpActor::ActivatedPowerup(AActor* ActiveFor)
{
	OnActivated(ActiveFor);

	bIsPowerActive = true; // 활성화 하는동안, 서버에 현재 파워가 발동중이라고 트루상태를 보냄. 블루프린트에서는 메시를 꺼버림. 활성화 상태일때.

	OnRep_PowerupActive();// 서버에는 알림기능이 없으므로 Rep을 해제해 전원을 킨다. // bIsPowerActive 변동사항 적용.

	// 블루프린트에서 설정한 파워업 인터벌 값이 0보다 크면 타이머를 설정하여 해당 간격만큼 파워업을 시켜줌.
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
	DOREPLIFETIME(ASPowerUpActor, bIsPowerActive); // 활성화하고 다른사람에게 복제
}

