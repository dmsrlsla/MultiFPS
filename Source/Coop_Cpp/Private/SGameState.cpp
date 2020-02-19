// Fill out your copyright notice in the Description page of Project Settings.


#include "SGameState.h"
#include "Net/UnrealNetwork.h"

void ASGameState::OnRep_WaveState(EWaveState OldState)
{
	WaveStateChange(WaveState, OldState);
}

void ASGameState::GetLifetimeReplicatedProps(TArray< FLifetimeProperty > & OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME(ASGameState, WaveState);
}

void ASGameState::SetWaveState(EWaveState NewState)
{
	EWaveState OldState = WaveState;
	if (Role == ROLE_Authority)
	{
		WaveState = NewState;
		OnRep_WaveState(OldState);
	}
}
