// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "../Building.h"
#include "PunCity/Simulation/BuildingCardSystem.h"

class TempleGrograth final : public Building
{
public:
	void FinishConstruction() final
	{
		Building::FinishConstruction();

	}

	void OnDeinit() final
	{
	}
};

class Shrine : public Building
{
public:
	static const int32 Radius = 12;
};

class ShrineWisdom : public Building
{
public:
	void FinishConstruction() final {
		Building::FinishConstruction();
		_simulation->cardSystem(playerId()).AddDrawCards(CardEnum::WildCard);
	}

	void OnDeinit() final {
		//_simulation->cardSystem(playerId()).RemoveDrawCards(CardEnum::ConverterCard);
	}
};

class BlossomShrine final : public Building
{
public:
	void Tick1Sec() final;

	static const int Radius = 12;
};

class HellPortal final : public Building
{
public:
	void TickRound() final;
};

class AdventurersGuild final : public Building
{
public:
	void TickRound() final;
};