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

	PrimaryActorTick.bCanEverTick = true; // ������Ʈ�� ����Ѵ�.
	// 1���� �ѹ�
	PrimaryActorTick.TickInterval = 1.0f;
}

void ASGameMode::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);

	CheckWaveState(); // ������Ʈ ���� üũ��.
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

	GetWorldTimerManager().SetTimer(TimeHandle_BotSpawner, this, &ASGameMode::SpawnBotTimerElapsed, 1.0f, true, 0.0f); // �� ���̺굿�� �� ����

	SetWaveState(EWaveState::WaveInProgress);
}

void ASGameMode::EndWave()
{
	GetWorldTimerManager().ClearTimer(TimeHandle_BotSpawner);

	SetWaveState(EWaveState::WaittingToComplate);
}

void ASGameMode::PrepareForNectWave()
{
	GetWorldTimerManager().SetTimer(TimeHandle_NextWaveSpawner, this, &ASGameMode::StartWave, TimeBetWeenWaves, false); // ���� ���̺� ����.

	SetWaveState(EWaveState::WaitingStart);
}

void ASGameMode::CheckWaveState()
{
	bool bIsPreparingForWave = GetWorldTimerManager().IsTimerActive(TimeHandle_NextWaveSpawner);

	// ���� ������ �� �����ٸ�, üũ�� ���� �ʴ´�.
	if (NrOfBotsToSpawn > 0 || bIsPreparingForWave)
	{
		return;
	}

	bool bIsAnyBootAlive = false;

	// ���ݺ��ڱ��� �ִ� �����ι� ������� �ݺ���.
	for (FConstPawnIterator It = GetWorld()->GetPawnIterator(); It; ++It)
	{
		APawn* TestPawn = It->Get();
		// ���� �÷��̾ ��Ʈ�� �ǰ� �ִٸ�.
		if (TestPawn == nullptr || TestPawn->IsPlayerControlled())
		{
			continue;
		}
		UScHealthComponent* HealthComp = Cast<UScHealthComponent>(TestPawn->GetComponentByClass(UScHealthComponent::StaticClass()));
		if (HealthComp && HealthComp->GetHealth() > 0.0f)
		{
			bIsAnyBootAlive = true; // �ϳ��� ���̶� ����ִٸ�, Ʈ��ó��.
			break;
		}
	}

	if (!bIsAnyBootAlive) // ����ִ� ���� ���ٸ�
	{
		SetWaveState(EWaveState::WaveComplate);

		PrepareForNectWave(); // �ʱ�ȭ �����Ѵ�.
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
				// �÷��̾ ���� �������
				return;
			}
		}
	}

	// ����ִ� �÷��̾ ���ٸ�.
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
			// ���Ӹ�� ���̽��� ����ŸƮ �÷��̾� ����
			RestartPlayer(PC);
		}
	}
}

void ASGameMode::StartPlay()
{
	Super::StartPlay();
	PrepareForNectWave();
}
