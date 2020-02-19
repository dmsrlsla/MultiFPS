// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameModeBase.h"
#include "SGameMode.generated.h"

// 유형의 이름, 파라미터의 갯수의 따라 몇단인지 설정함.
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

	// 블루프린트에 싱글 봇을 연결한다.
	UFUNCTION(BlueprintImplementableEvent, Category = "GameMode")
	void SpawnNewBot();

	void SpawnBotTimerElapsed();

	// Start Spawning Bots;
	void StartWave();

	// Stop 스폰 봇들
	void EndWave();

	//  다음 스폰 시작을 타이머를 설정한다.
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
