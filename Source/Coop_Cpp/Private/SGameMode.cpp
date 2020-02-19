// Fill out your copyright notice in the Description page of Project Settings.


#include "SGameMode.h"
#include "TimerManager.h"
#include "Engine.h"
#include "ScHealthComponent.h"
#include "SCharacter.h"
#include "SGameState.h"
#include "SPlayerState.h"

ASGameMode::ASGameMode()
{
	TimeBetWeenWaves = 2.0f;

	GameStateClass = ASGameState::StaticClass();
	PlayerStateClass = ASPlayerState::StaticClass();

	PrimaryActorTick.bCanEverTick = true; // 업데이트를 사용한다.
	// 1초의 한번
	PrimaryActorTick.TickInterval = 1.0f;
}

void ASGameMode::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);

	CheckWaveState(); // 업데이트 마다 체크함.
	CheckAnyPlayerAlive();
}

void ASGameMode::SpawnBotTimerElapsed()
{
	SpawnNewBot();

	NrOfBotsToSpawn--;

	if (NrOfBotsToSpawn <= 0)
	{
		EndWave();
	}
}

void ASGameMode::StartWave()
{
	WaveCount++;
	NrOfBotsToSpawn = 2 * WaveCount;

	GetWorldTimerManager().SetTimer(TimeHandle_BotSpawner, this, &ASGameMode::SpawnBotTimerElapsed, 1.0f, true, 0.0f); // 한 웨이브동안 봇 스폰

	SetWaveState(EWaveState::WaveInProgress);
}

void ASGameMode::EndWave()
{
	GetWorldTimerManager().ClearTimer(TimeHandle_BotSpawner);

	SetWaveState(EWaveState::WaittingToComplate);
}

void ASGameMode::PrepareForNectWave()
{
	GetWorldTimerManager().SetTimer(TimeHandle_NextWaveSpawner, this, &ASGameMode::StartWave, TimeBetWeenWaves, false); // 다음 웨이브 시작.

	SetWaveState(EWaveState::WaitingStart);
}

void ASGameMode::CheckWaveState()
{
	bool bIsPreparingForWave = GetWorldTimerManager().IsTimerActive(TimeHandle_NextWaveSpawner);

	// 아직 스폰이 덜 끝났다면, 체크는 하지 않는다.
	if (NrOfBotsToSpawn > 0 || bIsPreparingForWave)
	{
		return;
	}

	bool bIsAnyBootAlive = false;

	// 폰반복자까지 있는 등장인물 갯수대로 반복함.
	for (FConstPawnIterator It = GetWorld()->GetPawnIterator(); It; ++It)
	{
		APawn* TestPawn = It->Get();
		// 현재 플레이어가 컨트롤 되고 있다면.
		if (TestPawn == nullptr || TestPawn->IsPlayerControlled())
		{
			continue;
		}
		UScHealthComponent* HealthComp = Cast<UScHealthComponent>(TestPawn->GetComponentByClass(UScHealthComponent::StaticClass()));
		if (HealthComp && HealthComp->GetHealth() > 0.0f)
		{
			bIsAnyBootAlive = true; // 하나의 봇이라도 살아있다면, 트루처리.
			break;
		}
	}

	if (!bIsAnyBootAlive) // 살아있는 놈이 없다면
	{
		SetWaveState(EWaveState::WaveComplate);

		PrepareForNectWave(); // 초기화 실행한다.
	}
}

void ASGameMode::CheckAnyPlayerAlive()
{
	for (FConstPlayerControllerIterator It = GetWorld()->GetPlayerControllerIterator(); It; ++It)
	{
		APlayerController* PC = It->Get();
		if (PC && PC->GetPawn())
		{
			APawn* MyPawn = PC->GetPawn();
			UScHealthComponent* HealthComp = Cast<UScHealthComponent>(MyPawn->GetComponentByClass(UScHealthComponent::StaticClass()));

			if (ensure(HealthComp) && HealthComp->GetHealth() > 0.0f)
			{
				// 플레이어가 아직 살아있음
				return;
			}
		}
	}

	// 살아있는 플레이어가 없다면.
	GameOver();
}

void ASGameMode::GameOver()
{
	EndWave();

	UE_LOG(LogTemp, Log, TEXT("GameOver"));
	SetWaveState(EWaveState::GameOver);
}

void ASGameMode::SetWaveState(EWaveState NewState)
{
	ASGameState* GS = GetGameState<ASGameState>();
	if (ensureAlways(GS))
	{
		GS->SetWaveState(NewState);
	}
}

void ASGameMode::RestartDeadPlayer()
{
	for (FConstPlayerControllerIterator It = GetWorld()->GetPlayerControllerIterator(); It; ++It)
	{
		APlayerController* PC = It->Get();
		if (PC && PC->GetPawn() == nullptr)
		{
			// 게임모드 베이스에 리스타트 플레이어 있음
			RestartPlayer(PC);
		}
	}
}

void ASGameMode::StartPlay()
{
	Super::StartPlay();
	PrepareForNectWave();
}
