// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "IGameSimulationCore.h"

struct PlayerSupplyChange
{
	int32 playerId = -1;
	int32 tick = -1;
	int32 amount = 0;

	//! Serialize
	FArchive& operator>>(FArchive &Ar) {
		Ar << playerId;
		Ar << tick;
		Ar << amount;
		return Ar;
	}
};

/**
 * 
 */
class WorldTradeSystem
{
public:
	void Init(IGameSimulationCore* simulation) {
		_simulation = simulation;
		_enumToSupplyValue100.resize(ResourceEnumCount);
		_resourceEnumToPrice100Vec.resize(ResourceEnumCount);
		_resourceEnumToPlayerSupplyChanges.resize(ResourceEnumCount);
	}

	void Tick1Sec()
	{
		//PUN_LOG("WorldTrade Tick1Sec");
		// Initialize after allplayers started
		if (!_isInitialized && _simulation->AllPlayerChoseLocationAfterInitialTicks())
		{
			for (size_t i = 0; i < _enumToSupplyValue100.size(); i++) {
				_enumToSupplyValue100[i] = EquilibriumSupplyValue100_PerPerson(static_cast<ResourceEnum>(i)) * worldPopulationWithBase();
			}
			_isInitialized = true;
		}

		// Non-Food
		for (int32 i = 0; i < ResourceEnumCount; i++) 
		{
			ResourceEnum resourceEnum = static_cast<ResourceEnum>(i);
			int64 buyValue100PerSec = EquilibriumSupplyValue100(resourceEnum) / Time::SecondsPerYear; // Assumes total bought/sold per year is equal to EquilibriumSupply
			int64 buyValue100TimesSupplyValue100 = buyValue100PerSec * SupplyValue100(resourceEnum);
			buyValue100PerSec = buyValue100TimesSupplyValue100 / EquilibriumSupplyValue100(resourceEnum); // Buy more when Supply goes up
			buyValue100PerSec = GameRand::RandFluctuate(buyValue100PerSec, WorldTradeFluctuationPercent);
			_enumToSupplyValue100[i] -= buyValue100PerSec;

			int64 sellValue100PerSec = EquilibriumSupplyValue100(resourceEnum) / Time::SecondsPerYear;
			sellValue100PerSec = GameRand::RandFluctuate(sellValue100PerSec, WorldTradeFluctuationPercent);
			_enumToSupplyValue100[i] += sellValue100PerSec;

			_enumToSupplyValue100[i] = std::max(_enumToSupplyValue100[i], MinSupplyValue100_PerPerson * worldPopulationWithBase());

			//PUN_LOG("Resource:%s, buy:%d sell:%d supply:%d eq:%d", *ResourceNameF(resourceEnum), buyValue100PerSec, sellValue100PerSec, _enumToSupplyValue100[i], EquilibriumSupplyValue100());
		}

		/*
		 * Stat: Add Data
		 */
		if (Time::Ticks() % TicksPerStatInterval == 0)
		{
			for (int32 i = 0; i < ResourceEnumCount; i++) {
				int32 price100 = _simulation->price100(static_cast<ResourceEnum>(i));
				std::vector<int32>& statVec = _resourceEnumToPrice100Vec[i];
				statVec.push_back(price100);
			}
		}

		/*
		 * 
		 */
		if (Time::IsSeasonStart())
		{
			// Remove trade record that are more than 1 years old
			for (std::vector<PlayerSupplyChange>& vecPlayerSupplyChanges : _resourceEnumToPlayerSupplyChanges) {
				for (size_t i = vecPlayerSupplyChanges.size(); i-- > 0;) {
					if (Time::Ticks() - vecPlayerSupplyChanges[i].tick >= Time::TicksPerYear) {
						vecPlayerSupplyChanges.erase(vecPlayerSupplyChanges.begin() + i);
					}
				}
			}
		}
	}

	/*
	 * Interface
	 */
	void ChangeSupply(int32 playerId, ResourceEnum resourceEnum, int32 quantity)
	{
		_enumToSupplyValue100[static_cast<int>(resourceEnum)] += quantity * GetResourceInfo(resourceEnum).basePrice * 100;

		_resourceEnumToPlayerSupplyChanges[static_cast<int>(resourceEnum)].push_back({playerId, Time::Ticks(), quantity});
	}

	int32 price100(ResourceEnum resourceEnum) {
		return GetResourceInfo(resourceEnum).basePrice * 100 * EquilibriumSupplyValue100(resourceEnum) / SupplyValue100(resourceEnum);
	}

	int64 SupplyValue100(ResourceEnum resourceEnum) {
		return _enumToSupplyValue100[static_cast<int>(resourceEnum)] + 1; // Note: can't be 0
	}

	const std::vector<PlayerSupplyChange>& GetSupplyChanges(ResourceEnum resourceEnum) {
		return _resourceEnumToPlayerSupplyChanges[static_cast<int>(resourceEnum)];
	}
	int32 GetNetPlayerSupplyChange(int32 playerId, ResourceEnum resourceEnum) {
		std::vector<PlayerSupplyChange>& supplyChanges = _resourceEnumToPlayerSupplyChanges[static_cast<int>(resourceEnum)];
		int32 netSupplyChange = 0;
		for (const PlayerSupplyChange& supplyChange : supplyChanges) {
			if (supplyChange.playerId == playerId) {
				netSupplyChange += supplyChange.amount;
			}
		}
		return netSupplyChange;
	}
	std::vector<std::pair<int32, int32>> GetTwoMainTraders(ResourceEnum resourceEnum, bool isGettingImporter)
	{
		std::vector<PlayerSupplyChange>& supplyChanges = _resourceEnumToPlayerSupplyChanges[static_cast<int>(resourceEnum)];
		std::unordered_map<int32, int32> playerToSupplyChanges;
		for (const PlayerSupplyChange& supplyChange : supplyChanges) {
			playerToSupplyChanges[supplyChange.playerId] += supplyChange.amount;
		}

		
		if (isGettingImporter) {
			return GetTwoMainTradersHelper(playerToSupplyChanges, [&](int32 a, int32 b) { return a < b; });
		}
		
		return GetTwoMainTradersHelper(playerToSupplyChanges, [&](int32 a, int32 b) { return a > b; });
	}
	

	// Serial
	void Serialize(FArchive& Ar)
	{
		SerializeVecValue(Ar, _enumToSupplyValue100);
		SerializeVecVecValue(Ar, _resourceEnumToPrice100Vec);

		SerializeVecVecObj(Ar, _resourceEnumToPlayerSupplyChanges);
	}

	const std::vector<int32>& GetStatVec(ResourceEnum resourceEnum) const {
		return _resourceEnumToPrice100Vec[static_cast<int>(resourceEnum)];
	}

	// Resource Stats
	ResourceEnum resourceEnumToShowStat = ResourceEnum::Wood;

private:
	/*
	 * Helpers
	 */
	template <typename Func>
	static std::vector<std::pair<int32, int32>> GetTwoMainTradersHelper(std::unordered_map<int32, int32>& playerToSupplyChanges, Func compareFunc)
	{
		std::vector<std::pair<int32, int32>> result;
		for (auto it : playerToSupplyChanges)
		{
			if (compareFunc(it.second, 0))
			{
				// Replace the result vec if we get lower value... (higher net import)
				bool isAdded = false;
				for (size_t i = 0; i < result.size(); i++) {
					if (compareFunc(it.second, result[i].second)) {
						result.insert(result.begin() + i, std::make_pair(it.first, it.second));
						isAdded = true;
						break;
					}
				}
				if (!isAdded) {
					result.push_back(std::make_pair(it.first, it.second));
				}
				if (result.size() > 2) {
					result.pop_back();
				}
			}
		}
		return result;
	}
	
private:
	int64 worldPopulationWithBase() { return _simulation->worldPlayerPopulation() + BaseWorldPopulation; }

	// How fast price goes back to base price without tampering
	int64 EquilibriumSupplyValue100(ResourceEnum resourceEnum) {
		return EquilibriumSupplyValue100_PerPerson(resourceEnum) * worldPopulationWithBase();
	}

	static const int64 BaseWorldPopulation = 100;
	static const int64 MinSupplyValue100_PerPerson = 30 * 100; // also sort of min price %
	static const int64 WorldTradeFluctuationPercent = 30;


private:
	IGameSimulationCore* _simulation = nullptr;
	bool _isInitialized = false;

	std::vector<int64> _enumToSupplyValue100;

private:
	// Stats
	std::vector<std::vector<int32>> _resourceEnumToPrice100Vec;

	// Resource -> list of PlayerSupplyChanges
	std::vector<std::vector<PlayerSupplyChange>> _resourceEnumToPlayerSupplyChanges;
};
