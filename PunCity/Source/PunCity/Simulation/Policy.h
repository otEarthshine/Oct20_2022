// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameSimulationInfo.h"
#include "IGameSimulationCore.h"
#include "PunCity/PunUtils.h"
#include "UnlockSystem.h"
#include "PlayerParameters.h"

enum class PolicyEnum : uint8_t
{
	None,
	ChildMarriage,
	CutItAllDown,
	ChildLabor, // Children are playing while people are dying left and right.
	//SmeltingExperiment, // 
	Trade,
	SeedExtraction,
	SmelterBonus,
	FarmBonus,

	Forage,
	ColdResist,
	MoreTax,

	HundredHourWorkWeek,
	RapidIndustrialization,
	StayHungryStayFoolish,
	GrumpyCatBreeding,

	OneWithNature,
	NaturalResourceConservation,
};

/**
 * 
 */
struct Policy
{
	//! These are set in PolicySysetm
	PolicyEnum policyEnum = PolicyEnum::None;
	int32_t playerId = -1;
	IGameSimulationCore* simulation = nullptr;

	virtual std::string name() { return "None"; }
	virtual std::string description() { return "None"; }
	virtual void OnSetActive(bool active) = 0;
};

// This can be used by policy to add other policies
class IPolicySystem
{
public:
	virtual void MoveCardFromLockedToAvailable(PolicyEnum policyEnum) = 0;
	virtual void MoveCardFromAvailableToInUse(PolicyEnum policyEnum) = 0;
	virtual void MoveCardFromInUseToBin(PolicyEnum policyEnum) = 0;

	virtual const std::unordered_map<PolicyEnum, std::shared_ptr<Policy>>& cardsAvailable() = 0;
	virtual std::unordered_map<PolicyEnum, std::shared_ptr<Policy>>& cardsInUse() = 0;
};

//Quest system.
// GamePlayerState stores PolicySystem with policy implemented
// Resources 
// Policy decided are sent across as enum
// Productivity change: Global Static Bonus//Bonus applied directly, array of bonus?
// 

struct CutItAllDown : Policy
{
	std::string name() override { return "Cut it all down"; }
	std::string description() override { return "Increase tree cut speed by 30%"; }

	void OnSetActive(bool active) override
	{
		simulation->parameters(playerId)->CutTreeTicksBonus = active ? -90 : 0;
	}
};

struct ChildMarriage : Policy
{
	std::string name() override { return "Child marriage"; }
	std::string description() override { return "Decrease the age which citizens can give birth."; }

	void OnSetActive(bool active) override
	{
	}
};

struct TradeCard : Policy
{
	std::string name() override { return "Trade"; }
	std::string description() override { return "Allows trade after setting policies."; }

	void OnSetActive(bool active) override{}
};

// TODO: query card directly... no need for playerParameters
struct SeedExtraction : Policy
{
	std::string name() override { return "Seed extraction"; }
	std::string description() override { return "Farms generate x2 more seeds. -25% Farm productivity"; }

	void OnSetActive(bool active) override
	{
		//simulation->parameters(playerId)->SeedExtraction = true;
	}
};


struct ForageCard : Policy
{
	std::string name() override { return "Gatherer society"; }
	std::string description() override { return "Increase gatherer yield by 30%"; }

	void OnSetActive(bool active) override
	{
		//simulation->playerParameters(playerId)->GatheringBonus = active ? 30 : 0;
	}
};

struct ColdResist : Policy
{
	std::string name() override { return "Cold resist"; }
	std::string description() override { return "Heat reduces 30% slower"; }

	void OnSetActive(bool active) override
	{
		//simulation->parameters(playerId)->HeatResistBonus = active ? -30 : 0;
	}
};

struct MoreTax : Policy
{
	std::string name() override { return "House Tax"; }
	std::string description() override { return "House gets +10% tax for each adjacent house."; }

	void OnSetActive(bool active) override
	{
		//simulation->playerParameters(playerId)->HouseAdjacencyBonus = active;
	}
};

struct SmelterBonus : Policy
{
	std::string name() override { return "Smelter Bonus"; }
	std::string description() override { return "Smelter gets +10% production for each adjacent smelter."; }

	void OnSetActive(bool active) override
	{
		//simulation->playerParameters(playerId)->SmelterAdjacencyBonus = active;
	}
};

//------------


//! Heighten productivity but kills population growth rate/happiness
struct HundredHourWorkWeek : Policy
{
	void OnSetActive(bool active) override
	{

	}
};

//! Build industrial buildings for bonus iron
struct RapidIndustrialization : Policy
{
	void OnSetActive(bool active) override
	{

	}
};

//! Starvation has no effect on work productivity
struct StayHungryStayFoolish : Policy
{
	void OnSetActive(bool active) override
	{

	}
};

//! Expand 3 more regions this year

/*
* Lazy
*/

//! Grumpy cat may spawn in city
struct GrumpyCatBreeding : Policy
{
	void OnSetActive(bool active) override
	{

	}
};

//! Increase tree growth/farm productivity. Industrial productivity %50
struct OneWithNature : Policy
{
	void OnSetActive(bool active) override
	{

	}
};

/*
* Survival
*/

//! Deplete natural resources slower but also slows down production
struct NaturalResourceConservation : Policy
{
	void OnSetActive(bool active) override
	{

	}
};

// Isolationist, LetThemEatCake, FiveYearPlan