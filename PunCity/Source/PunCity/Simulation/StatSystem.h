// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "IGameSimulationCore.h"
#include <sstream>
#include "PunCity/CppUtils.h"

enum class ResourceSeasonStatEnum
{
	Consumption,
	Production,
};
static const std::string ResourceSeasonStatName[] =
{
	"Consumption",
	"Production",
};
static const int32_t ResourceSeasonStatEnumCount = _countof(ResourceSeasonStatName);

/*
 * Note that Stat system is used for production/consumptions etc. rather than variables like population/food etc. that can be queried
 */
// Per player statSystem
class SubStatSystem
{
public:
	SubStatSystem() {
		_enumToSeasonToStat.resize(SeasonStatEnumCount, std::vector<int32_t>(4, 0));
		_enumToResourceToSeasonToStat.resize(ResourceSeasonStatEnumCount,
												std::vector<std::vector<int32_t>>(ResourceEnumCount, std::vector<int32_t>(4, 0))
											);
		_enumToAccumulatedStat.resize(AccumulatedStatEnumCount, 0);
	}

	void Tick(int32 playerId, IGameSimulationCore* simulation);
	//{
	//	if (Time::IsSeasonStart()) 
	//	{
	//		int32_t seasonInt = Time::SeasonMod();
	//		check(seasonInt < 4);
	//		
	//		// Reset Stat Entry
	//		for (int i = 0; i < _enumToSeasonToStat.size(); i++) {
	//			_enumToSeasonToStat[i][seasonInt] = 0;
	//		}
	//		for (int i = 0; i < _enumToResourceToSeasonToStat.size(); i++) {
	//			auto& resourceToSeasonToStat = _enumToResourceToSeasonToStat[i];
	//			for (int j = 0; j < resourceToSeasonToStat.size(); j++) {
	//				resourceToSeasonToStat[j][seasonInt] = 0;
	//			}
	//		}

	//	}
	//}

	/*
	 * Accumulated stats
	 */
	int32 GetStat(AccumulatedStatEnum statEnum) {
		return _enumToAccumulatedStat[static_cast<int32>(statEnum)];
	}
	void AddStat(AccumulatedStatEnum statEnum, int32 amount = 1) {
		_enumToAccumulatedStat[static_cast<int32>(statEnum)] += amount;
	}
	

	/*
	 * Enum to Season stats
	 */
	const std::vector<int32>& GetStat(SeasonStatEnum statEnum) {
		return _enumToSeasonToStat[(int32)statEnum];
	}

	void AddStat(SeasonStatEnum statEnum, int32 amount = 1) {
		_enumToSeasonToStat[(int32)statEnum][Time::SeasonMod()] += amount;
	}

	// return resourceToSeasonToStat
	const std::vector<std::vector<int32>>& GetResourceStat(ResourceSeasonStatEnum statEnum) {
		return _enumToResourceToSeasonToStat[(int)statEnum];
	}

	int32 GetYearlyResourceStat(ResourceSeasonStatEnum statEnum, ResourceEnum resourceEnum) {
		PUN_CHECK(static_cast<int32>(resourceEnum) < _enumToResourceToSeasonToStat[(int)statEnum].size())
		return CppUtils::Sum(_enumToResourceToSeasonToStat[(int)statEnum][static_cast<int32>(resourceEnum)]);
	}

	void AddResourceStat(ResourceSeasonStatEnum statEnum, ResourceEnum resourceEnum, int32_t amount) {
		_enumToResourceToSeasonToStat[(int)statEnum][(int)resourceEnum][Time::SeasonMod()] += amount;
	}
	int32 GetCurrentResourceStat(ResourceSeasonStatEnum statEnum, ResourceEnum resourceEnum, int32 seasonInt) {
		return _enumToResourceToSeasonToStat[static_cast<int>(statEnum)][static_cast<int>(resourceEnum)][seasonInt];
	}

	// return seasonToStat
	std::vector<int32_t> TotalRevenueStat() {
		const std::vector<std::vector<int32_t>>& resourceToSeasonToStat =  GetResourceStat(ResourceSeasonStatEnum::Production);
		std::vector<int32_t> seasonToRevenue(4, 0);
		for (int i = 0; i < resourceToSeasonToStat.size(); i++) {
			const std::vector<int32_t>& seasonToStat = resourceToSeasonToStat[i];
			for (int j = 0; j < 4; j++) {
				seasonToRevenue[j] += seasonToStat[j];
			}
		}
		return seasonToRevenue;
	}

	//
	int32 GetYearlyStat(SeasonStatEnum statEnum) {
		const std::vector<int32>& seasonToStat = GetStat(statEnum);
		int32 yearlyStat = 0;
		for (int i = 0; i < seasonToStat.size(); i++) {
			yearlyStat += seasonToStat[i];
		}
		return yearlyStat;
	}

	//! Serialize
	FArchive& operator>>(FArchive &Ar)
	{
		SerializeVecVecValue(Ar, _enumToSeasonToStat);
		SerializeVecLoop(Ar, _enumToResourceToSeasonToStat, [&](std::vector<std::vector<int32_t>>& vecVecValue) {
			SerializeVecVecValue(Ar, vecVecValue);
		});
		return Ar;
	}

private:
	std::vector<std::vector<int32>> _enumToSeasonToStat;
	std::vector<std::vector<std::vector<int32>>> _enumToResourceToSeasonToStat;
	std::vector<int32> _enumToAccumulatedStat;
};

/**
 * 
 */
class StatSystem
{
public:
	void Init() {}

	void AddPlayer(int32 playerId) {
		PUN_CHECK(playerId == _playerIdToSubstatSystem.size());
		_playerIdToSubstatSystem.push_back(SubStatSystem());
	}
	void ResetPlayer(int32 playerId) {
		_playerIdToSubstatSystem[playerId] = SubStatSystem();
	}

	void Tick(IGameSimulationCore* simulation) {
		for (int i = 0; i < _playerIdToSubstatSystem.size(); i++) {
			_playerIdToSubstatSystem[i].Tick(i, simulation);
		}
		_globalSubstatSystem.Tick(-1, simulation);
	}

	SubStatSystem& playerStatSystem(int32 playerId) {
		if (playerId == -1) {
			return _globalSubstatSystem;
		}
		return _playerIdToSubstatSystem[playerId]; 
	}

	std::vector<int32> GetStatAll(SeasonStatEnum statEnum) {
		std::vector<int32> statAmountAll(4, 0);
		for (int i = 0; i < _playerIdToSubstatSystem.size(); i++) {
			CombineSeasonStats(statAmountAll, _playerIdToSubstatSystem[i].GetStat(statEnum));
		}
		CombineSeasonStats(statAmountAll, _globalSubstatSystem.GetStat(statEnum));
		return statAmountAll;
	}

	void CombineSeasonStats(std::vector<int32_t>& main, const std::vector<int32_t>& add) {
		check(main.size() == 4);
		check(add.size() == 4);
		for (int i = 0; i < 4; i++) {
			main[i] += add[i];
		}
	}

	void Serialize(FArchive& Ar)
	{
		SerializeVecObj(Ar, _playerIdToSubstatSystem);
		_globalSubstatSystem >> Ar;
	}

private:
	std::vector<SubStatSystem> _playerIdToSubstatSystem;
	SubStatSystem _globalSubstatSystem; // SubStatSystem for no player (player == -1)

public:
	//! Print Season stats
	static std::string StatsToString(const std::vector<int32_t>& stats)
	{
		check(stats.size() == 4);
		std::stringstream ss;

		for (int i = 0; i < 4; i++) {
			ss << " _ " << std::to_string(stats[(i + Time::GraphBeginSeason()) % 4]);
		}
		ss << "\n";
		//ss << "spr " << std::to_string(stats[0]) << ", sum" << std::to_string(stats[1]) <<
		//	", aut " << std::to_string(stats[2]) << ", win " << std::to_string(stats[3]) << "\n";
		return ss.str();
	}

	static std::string ResourceStatsToString(const std::vector<std::vector<int32_t>>& stats)
	{
		check(stats.size() == ResourceEnumCount);
		std::stringstream ss;
		for (int i = 0; i < ResourceEnumCount; i++) {
			bool hasResource = false;
			check(stats[i].size() == 4);

			// If any season has this resource... display the graph
			for (int season = 0; season < stats[i].size(); season++) {
				if (stats[i][season] > 0) {
					hasResource = true;
				}
			}

			if (hasResource) {
				ss << ResourceName(ResourceEnum(i)) << ": " << StatsToString(stats[i]);
			}
		}
		return ss.str();
	}
};
