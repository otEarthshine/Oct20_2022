// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"

class GameConstants
{
public:
	static const int MaxPlayers = 30;
	static const int MaxAIs = 15;
	static const int MaxPlayersAndAI = MaxPlayers + MaxAIs;

	static bool IsAI(int32 playerId) { return playerId >= MaxPlayers; }

	//static const int InitialTownhallUnits = 8;

#if WITH_EDITOR
	static const int IdealWorldPop = 5000;
	static const int InitialWorldPop = 4000; // 10
#else
	static const int IdealWorldPop = 20000;
	static const int InitialWorldPop = IdealWorldPop / 2;
#endif
	//static const int32_t BaseHappiness = 50;

	static const int32 MaxFloodDistance_Human = 3;
	static const int32 MaxFloodDistance_HumanIdle = 3;
	static const int32 MaxFloodDistance_Animal = 3;

	static const int32 MaxFloodDistance_Army = 10;

	static const int WorkerEmptyBuildingInventoryAmount = 80;

	static const int32 StorageCountPerTile = 120;
	static const int32 StorageSlotCount = 4;

	static const int32 InitialMoney = 2000;
};

static const int32 ModuleMeshCount = 416; // 407