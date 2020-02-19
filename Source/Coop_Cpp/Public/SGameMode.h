// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameModeBase.h"
#include "SGameMode.generated.h"

// ������ �̸�, �Ķ������ ������ ���� ������� ������.
DECLARE_DYNAMIC_MULTICAST_DELEGATE_ThreeParams(FOnActorKilled, AActor*, VictimActor, AActor*, KillerActor, AController*, KillerController);
enum class EWaveState : uint8;


/**
 * 
 */
UCLASS()
class COOP_CPP_API ASGameMode : public AGameModeBase
{
	GENERATED_BODY()

protected:

	// �������Ʈ�� �̱� ���� �����Ѵ�.
	UFUNCTION(BlueprintImplementableEvent, Category = "GameMode")
	void SpawnNewBot();

	void SpawnBotTimerElapsed();

	// Start Spawning Bots;
	void StartWave();

	// Stop ���� ����
	void EndWave();

	//  ���� ���� ������ Ÿ�̸Ӹ� �����Ѵ�.
	void PrepareForNectWave();

	void CheckWaveState();

	void CheckAnyPlayerAlive();

	void GameOver();

	void SetWaveState(EWaveState NewState);

	void RestartDeadPlayer();

protected:

	FTimerHandle TimeHandle_BotSpawner;

	FTimerHandle TimeHandle_NextWaveSpawner;
	
	int32 NrOfBotsToSpawn;

	int32 WaveCount;

	UPROPERTY(EditDefaultsOnly, Category ="GameMode")
	float TimeBetWeenWaves;

public:

	ASGameMode();

	virtual void Tick(float DeltaSeconds) override;

	virtual void StartPlay() override;

	UPROPERTY(BlueprintAssignable, Category ="GameMode")
	FOnActorKilled OnActorKilled;
};
