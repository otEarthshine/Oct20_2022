// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "IGameSimulationCore.h"
#include "Building.h"

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

struct TradeRouteResourcePair
{
	bool isTown1ToTown2 = true;
	ResourcePair resourcePair;

	//! Serialize
	FArchive& operator>>(FArchive &Ar) {
		Ar << isTown1ToTown2;
		resourcePair >> Ar;
		return Ar;
	}
};

struct TradeRoutePair
{
	int32 townId1;
	int32 townId2;

	int32 distance;

	std::vector<TradeRouteResourcePair> tradeResources;

	bool HasTownId(int32 townId) const {
		return townId == townId1 || 
				townId == townId2;
	}

	int32 GetCounterpartTownId(int32 townIdIn) const {
		return townIdIn == townId1 ? townId2 : townId1;
	}
	

	//! Serialize
	FArchive& operator>>(FArchive &Ar) {
		Ar << townId1;
		Ar << townId2;

		Ar << distance;

		SerializeVecObj(Ar, tradeResources);
		return Ar;
	}

	bool operator==(const TradeRoutePair& a) const {
		if (townId1 == a.townId2 &&
			townId2 == a.townId1) {
			return true;
		}
		return townId1 == a.townId1 &&
			townId2 == a.townId2;
	}
};



/**
 * 
 */
class WorldTradeSystem
{
public:
	void Init(IGameSimulationCore* simulation, int32 maxPlayers)
	{
		_simulation = simulation;
		_enumToSupplyValue100.resize(ResourceEnumCount);
		_resourceEnumToPrice100Vec.resize(ResourceEnumCount);
		_resourceEnumToPlayerSupplyChanges.resize(ResourceEnumCount);
	}

	void AddTown(int32 townId)
	{
		PUN_CHECK(townId == _intercityTradeOffers.size());
		_intercityTradeOffers.push_back(std::vector<IntercityTradeOffer>());
	}

	void Tick1Sec();

	/*
	 * Interface
	 */
	void ChangeSupply(int32 playerId, ResourceEnum resourceEnum, int32 quantity)
	{
		_enumToSupplyValue100[static_cast<int>(resourceEnum)] += quantity * GetResourceInfo(resourceEnum).basePrice * 100;

		_resourceEnumToPlayerSupplyChanges[static_cast<int>(resourceEnum)].push_back({ playerId, Time::Ticks(), quantity });
	}

	int64 price100(ResourceEnum resourceEnum)
	{
		int64 basePrice100 = GetResourceInfo(resourceEnum).basePrice * 100;
		int64 equilibriumSupplyValue100 = EquilibriumSupplyValue100(resourceEnum);
		check(equilibriumSupplyValue100 >= 0);
		
		int64 supplyValue100 = SupplyValue100(resourceEnum);
		check(supplyValue100 >= 0);
		
		if (equilibriumSupplyValue100 > supplyValue100) {
			return basePrice100 * equilibriumSupplyValue100 / supplyValue100;
		}
		return basePrice100 * equilibriumSupplyValue100 / supplyValue100 / 2 + basePrice100 / 2;
		//return GetResourceInfo(resourceEnum).basePrice * 100 * EquilibriumSupplyValue100(resourceEnum) / SupplyValue100(resourceEnum);
	}

	int64 SupplyValue100(ResourceEnum resourceEnum) {
		return _enumToSupplyValue100[static_cast<int>(resourceEnum)] + 1; // Note: can't be 0
	}

	void ChangePrice100_Percent(ResourceEnum resourceEnum, int32 percent) {
		_enumToSupplyValue100[static_cast<int>(resourceEnum)] = _enumToSupplyValue100[static_cast<int>(resourceEnum)] * 100 / (100 + percent); // more percent, less supply, more price
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

	// Trade Route
	void RefreshTradeRoutes();

	void SortTradeRoutes()
	{
		std::sort(_tradeRoutePairs.begin(), _tradeRoutePairs.end(), [&](const TradeRoutePair& a, const TradeRoutePair& b) {
			return a.distance < b.distance;
		});
	}


	
	bool HasTradeRoute(const TradeRoutePair& tradeRoutePairIn) const {
		for (const TradeRoutePair& tradeRoutePair : _tradeRoutePairs) {
			if (tradeRoutePair == tradeRoutePairIn) {
				return true;
			}
		}
		return false;
	}
	bool HasTradeRoute(int32 townId1, int32 townId2) const {
		TradeRoutePair route;
		route.townId1 = townId1;
		route.townId2 = townId2;
		return HasTradeRoute(route);
	}
	
	bool TryEstablishTradeRoute(const FGenericCommand& command);
	void TryCancelTradeRoute(const FGenericCommand& command);

	std::vector<TradeRoutePair>& tradeRoutePairs() {
		return _tradeRoutePairs;
	}

	
	std::vector<TradeRoutePair> GetTradeRoutesTo(int32 originTownId) {
		std::vector<TradeRoutePair> routes;
		for (const TradeRoutePair& route : _tradeRoutePairs) {
			if (route.HasTownId(originTownId)) {
				routes.push_back(route);
			}
		}
		return routes;
	}
	
	//std::vector<TradeRoutePair> GetTradeRouteTo(int32 originTownId)
	//{
	//	std::vector<TradeRoutePair> resultPairs;
	//	for (const TradeRoutePair& routePair : _tradeRoutePairs) {
	//		if (routePair.HasTownId(originTownId)) {
	//			resultPairs.push_back(routePair);
	//		}
	//	}
	//	return resultPairs;
	//}
	
	
	FText GetTradeRouteNodeName1(const TradeRoutePair& tradeRoutePair) {
		return _simulation->townNameT(tradeRoutePair.townId1);
	}
	FText GetTradeRouteNodeName2(const TradeRoutePair& tradeRoutePair) {
		return _simulation->townNameT(tradeRoutePair.townId2);
	}

	// Intercity Trade
	// TODO: REMOVE
	//const std::vector<IntercityTradeOffer>& GetIntercityTradeOffers(int32 townId) {
	//	return _intercityTradeOffers[townId];
	//}
	//IntercityTradeOffer GetIntercityTradeOffer(int32 townId, ResourceEnum resourceEnum) {
	//	std::vector<IntercityTradeOffer>& offers = _intercityTradeOffers[townId];
	//	for (IntercityTradeOffer& offer : offers) {
	//		if (offer.resourceEnum == resourceEnum) {
	//			return offer;
	//		}
	//	}
	//	return IntercityTradeOffer();
	//}
	//void SetIntercityTradeOffers(FSetIntercityTrade command)
	//{
	//	if (command.townId != -1 && command.townId < _intercityTradeOffers.size())
	//	{
	//		_intercityTradeOffers[command.townId].clear();
	//		for (int32 i = 0; i < command.resourceEnums.Num(); i++) {
	//			_intercityTradeOffers[command.townId].push_back({ static_cast<ResourceEnum>(command.resourceEnums[i]),  static_cast<IntercityTradeOfferEnum>(command.intercityTradeOfferEnum[i]), command.targetInventories[i] });
	//		}
	//	}
	//}
	

	// Serial
	void Serialize(FArchive& Ar)
	{
		Ar << _isInitialized;
		
		SerializeVecValue(Ar, _enumToSupplyValue100);
		SerializeVecVecValue(Ar, _resourceEnumToPrice100Vec);

		SerializeVecVecObj(Ar, _resourceEnumToPlayerSupplyChanges);
		
		SerializeVecObj(Ar, _tradeRoutePairs);

		SerializeVecVecObj(Ar, _intercityTradeOffers);
	}

	const std::vector<int32>& GetPlotStatVec(ResourceEnum resourceEnum) const {
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
	int64 worldPopulationWithBase() { return _simulation->worldTownPopulation() + BaseWorldPopulation; }

	// How fast price goes back to base price without tampering
	int64 EquilibriumSupplyValue100(ResourceEnum resourceEnum) {
		return EquilibriumSupplyValue100_PerPerson(resourceEnum) * worldPopulationWithBase();
	}

	static const int64 BaseWorldPopulation = 300;
	static const int64 MinSupplyValue100_PerPerson = 20 * 100; // also sort of max price % // May 10: 30 -> 50, Nov 14: 50 -> 20
	static const int64 WorldTradeFluctuationPercent = 30;
	// Also see SupplyMultiplier in SimInfo


private:
	IGameSimulationCore* _simulation = nullptr;

	/*
	 * Serialize
	 */
	bool _isInitialized = false;

	std::vector<int64> _enumToSupplyValue100;

	// Stats
	std::vector<std::vector<int32>> _resourceEnumToPrice100Vec;

	// Resource -> list of PlayerSupplyChanges
	std::vector<std::vector<PlayerSupplyChange>> _resourceEnumToPlayerSupplyChanges;


	// TradeRoute Clusters
	std::vector<TradeRoutePair> _tradeRoutePairs;
	//std::vector<std::vector<int32>> _playerIdToTradePartners;

	// Intercity Trade Offer
	std::vector<std::vector<IntercityTradeOffer>> _intercityTradeOffers;
};
