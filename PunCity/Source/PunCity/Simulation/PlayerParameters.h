// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameSimulationInfo.h"
#include "IGameSimulationCore.h"

/**
 * 
 */
class PlayerParameters
{
public:

	PlayerParameters(int32 playerId, IGameSimulationCore* simulation) {
		_playerId = playerId;
		_simulation = simulation;
	}
	
	/*
		Banished pop by year (from youtuber)
		1) 10/7
		2) 15/5
		3) 17/5

		2.5 years to working/reproducing in banished

	
		KR:
		2 season to min breeding...

		Aim at population growth of 10%-20% per year??

		1 Child per year... first child at random time of the year.. 1 year recharge...
		10 adults, 5 children
		5 female

		4 children produced at normal rate...

		2 years to reach grown enough for breeding
		10 years lifespan...
		last 2 years no children...

		child at 20% of population... for a stable pop...
	 */
	static const int32 PeopleAgeToGameYear = 10;

	static const int32 BaseAdultTicks = Time::TicksPerYear * 3 / 2;
	
	/*
	 * Ages
	 *  - BeginBreedingAgeTicks 1.5 years (15 years)
	 *  - EndBreedingAgeTicks 5.5 years (55 years)
	 *  - DeathAgeTicks 8 years (80 years)
	 */
	int32 BeginBreedingAgeTicks() {
		int32 result = BaseAdultTicks; // 1.5 year
		if (_simulation->TownhallCardCount(_playerId, CardEnum::ChildMarriage) > 0) {
			result -= Time::TicksPerYear / 2;
		}
		return result;
	}
	int32 BeginWorkingAgeTicks()
	{
		return Time::TicksPerYear * 3 / 2; // 1.5 year
	}

	int32 EndBreedingAgeTicks()
	{
		return Time::TicksPerYear * 55 / 10;
	}

	int32 DeathAgeTicks()
	{
		int32 result = Time::TicksPerYear * 8;
		if (_simulation->TownhallCardCount(_playerId, CardEnum::ProlongLife) > 0) {
			result += Time::TicksPerYear;
		}
		return result;
	}

	/*
	 * Breeding chance
	 */
	int32 TicksBetweenPregnancy()
	{
		int32 result = Time::TicksPerYear * 5 / 4;
		if (_simulation->TownhallCardCount(_playerId, CardEnum::BirthControl) > 0) {
			result *= 2;
		}

		int32 population = _simulation->population(_playerId);
		int32 housingCapacity = _simulation->HousingCapacity(_playerId);
		if (population > housingCapacity) {
			result *= 2;
		}
		else if (population > housingCapacity + 5) {
			result *= 4;
		}

		if (_simulation->IsInDarkAge(_playerId)) { // Dark age baby boom
			result /= 3;
		}
		else if (population <= housingCapacity / 2) { // crashing economy
			result /= 2;
		}
		else if (population < housingCapacity - 10) { // house growth boost
			result = result * 3 / 4;
		}
		return result;
	}
	int32 TicksBetweenPregnancyRange() { return Time::TicksPerSeason * 3 / 4; }

	/*
	 * 
	 */
	
	int32 CutTreeTicks() { return CutTreeTicksBase + CutTreeTicksBonus; }
	int32 HarvestDepositTicks() { return HarvestDepositTicksBase; }
	


	/*
	 * Bonuses
	 */
	int32 CutTreeTicksBonus = 0;

	/*
	 * Statuses
	 */
	int32 MaxAchievedHouseLvl = 1;
	bool FarmNoticed = false;
	bool BridgeNoticed = false;
	bool NeedTownhallUpgradeNoticed = false;
	
	bool CannibalismOffered = false;
	bool DesertPilgrimOffered = false;

	void Serialize(FArchive& Ar)
	{
		Ar << CutTreeTicksBonus;

		// Statuses
		Ar << MaxAchievedHouseLvl;
		Ar << FarmNoticed;
		Ar << BridgeNoticed;
		Ar << NeedTownhallUpgradeNoticed;
		
		Ar << CannibalismOffered;
		Ar << DesertPilgrimOffered;
	}

private:
	int32 _playerId = -1;
	IGameSimulationCore* _simulation = nullptr;
};
