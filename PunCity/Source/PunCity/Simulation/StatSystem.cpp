// Fill out your copyright notice in the Description page of Project Settings.

#include "StatSystem.h"
#include "PlayerOwnedManager.h"

void SubStatSystem::Tick(int32 townId, IGameSimulationCore* simulation)
{
	if (Time::IsSeasonStart())
	{
		int32 seasonInt = Time::SeasonMod();
		check(seasonInt < 4);

		int32 lastSeasonInt = (Time::Seasons() + 4 - 1) % 4;

		// Add to Graph just before resetting
		if (townId != -1)
		{
			int32 foodProduction = 0;
			int32 foodConsumption = 0;
			for (ResourceEnum foodEnum : StaticData::FoodEnums) {
				foodProduction += GetCurrentResourceStat(ResourceSeasonStatEnum::Production, foodEnum, lastSeasonInt);
				foodConsumption += GetCurrentResourceStat(ResourceSeasonStatEnum::Consumption, foodEnum, lastSeasonInt);
			}
			auto& townManager = simulation->townManager(townId);
			townManager.AddDataPoint(PlotStatEnum::FoodProduction, foodProduction, Time::TicksPerSeason);
			townManager.AddDataPoint(PlotStatEnum::FoodConsumption, foodConsumption, Time::TicksPerSeason);
		}


		// Reset Stat Entry
		for (int i = 0; i < _enumToSeasonToStat.size(); i++) {
			_enumToSeasonToStat[i][seasonInt] = 0;
		}
		for (int i = 0; i < _enumToResourceToSeasonToStat.size(); i++) {
			auto& resourceToSeasonToStat = _enumToResourceToSeasonToStat[i];
			for (int j = 0; j < resourceToSeasonToStat.size(); j++) {
				resourceToSeasonToStat[j][seasonInt] = 0;
			}
		}

	}
}