// Fill out your copyright notice in the Description page of Project Settings.


#include "WorldTradeSystem.h"
#include "PunCity/CppUtils.h"
#include "PunCity/PunTimer.h"
#include "Building.h"
#include "Buildings/GathererHut.h"

#define LOCTEXT_NAMESPACE "WorldTradeSystem"

void WorldTradeSystem::RefreshTradeRoutes()
{
	// TODO: this is for demolish.. check if trade routes are still valid



	
}

void WorldTradeSystem::TryEstablishTradeRoute(const FGenericCommand& command)
{
	SCOPE_TIMER("TryEstablishTradeRoute");
	
	auto pathAI = _simulation->pathAI();

	TradeRoutePair tradeRoutePair;
	tradeRoutePair.townId1 = command.intVar1;
	tradeRoutePair.townId2 = command.intVar2;

	Building& building1 = _simulation->building(_simulation->GetTownhallId(tradeRoutePair.townId1));
	Building& building2 = _simulation->building(_simulation->GetTownhallId(tradeRoutePair.townId2));

	tradeRoutePair.distance = WorldTile2::Distance(building1.centerTile(), building2.centerTile());
	
	int32 playerId1 = building1.playerId();
	check(playerId1 != -1);
	int32 playerId2 = building2.playerId();
	
	// Already established
	if (HasTradeRoute(tradeRoutePair)) 
	{
		_simulation->AddPopupToFront(playerId1,
			LOCTEXT("AlreadyHasTradeRoute","Already establish the trade route."), 
			ExclusiveUIEnum::None, "PopupCannot"
		);
		return;
	}

	auto connectTradeRoute = [&]()
	{
		// Connect both players
		_tradeRoutePairs.push_back(tradeRoutePair);

		_simulation->RecalculateTaxDelayedPlayer(playerId1);
		if (playerId2 != -1) {
			_simulation->RecalculateTaxDelayedPlayer(playerId2);
		}

		// TODO: Get Proper name for single building that can be traded with
		FText text = FText::Format(
			LOCTEXT("TradeRouteEstablish_Pop", "Trade Route was established between {0} and {1}!<space>Trade Route Income varies with the population of both cities."),
			GetTradeRouteNodeName1(tradeRoutePair),
			GetTradeRouteNodeName2(tradeRoutePair)
		);
		_simulation->AddPopupToFront(playerId1, text, ExclusiveUIEnum::TownAutoTradeUI, "");
		if (playerId2 != -1) {
			_simulation->AddPopupToFront(playerId2, text, ExclusiveUIEnum::TownAutoTradeUI, "");
		}

		SortTradeRoutes();
	};

	/*
	 * Land Trade
	 */

	auto findNearestRoadTile = [&](WorldTile2 gateTileIn) -> WorldTile2
	{
		WorldTile2 result = WorldTile2::Invalid;
		auto tryFindStartTile = [&](WorldTile2 shift) {
			if (result == WorldTile2::Invalid) {
				WorldTile2 testTile = gateTileIn + shift;
				if (pathAI->isRoad(testTile.x, testTile.y)) {
					result = testTile;
				}
			}
		};
		tryFindStartTile(WorldTile2(1, 0));
		tryFindStartTile(WorldTile2(-1, 0));
		tryFindStartTile(WorldTile2(0, 1));
		tryFindStartTile(WorldTile2(0, -1));
		return result;
	};

	WorldTile2 startTile = findNearestRoadTile(building1.gateTile());
	WorldTile2 targetTile = findNearestRoadTile(building2.gateTile());

	if (startTile.isValid() && targetTile.isValid())
	{
		std::vector<uint32_t> path;
		bool succeed = _simulation->pathAI()->FindPathRoadOnly(startTile.x, startTile.y, targetTile.x, targetTile.y, path);

		if (succeed) {
			connectTradeRoute();
			return;
		}
	}

	/*
	 * Water Trade
	 */

	//! Not Possible to Water Trade?
	std::vector<int32> portIds2 = _simulation->GetPortIds(tradeRoutePair.townId2);
	if (portIds2.size() == 0)
	{
		// Complain about needing road to trade
		_simulation->AddPopupToFront(playerId1,
			LOCTEXT("NeedIntercityRoadToMakeTradeRoute", "Need intercity road to establish the trade route. Connect your Townhall to target Townhall with Road."),
			ExclusiveUIEnum::TownAutoTradeUI, "PopupCannot"
		);
		return;
	}


	std::vector<int32> portIds1 = _simulation->GetPortIds(tradeRoutePair.townId1);
	if (portIds1.size() == 0)
	{
		// Complain about needing port
		_simulation->AddPopupToFront(playerId1,
			LOCTEXT("NeedPortToMakeTradeRoute", "Need Trading Port to establish the trade route."),
			ExclusiveUIEnum::TownAutoTradeUI, "PopupCannot"
		);
		return;
	}


	connectTradeRoute();
	

	// TODO: Province flood to see if the water tiles are connected
	
}
void WorldTradeSystem::TryCancelTradeRoute(const FGenericCommand& command)
{
	TradeRoutePair tradeRoutePair;
	tradeRoutePair.townId1 = command.intVar1;
	tradeRoutePair.townId2 = command.intVar2;
	
	bool succeed = CppUtils::TryRemove(_tradeRoutePairs, tradeRoutePair);
	if (succeed)
	{
		int32 playerId1 = _simulation->building(_simulation->GetTownhallId(tradeRoutePair.townId1)).playerId();
		check(playerId1 != -1);
		int32 playerId2 = _simulation->building(_simulation->GetTownhallId(tradeRoutePair.townId2)).playerId();
		
		_simulation->RecalculateTaxDelayedPlayer(playerId1);
		_simulation->RecalculateTaxDelayedPlayer(playerId2);

		FText text = FText::Format(
			LOCTEXT("CancelTradeRoute_Pop", "Trade Route between {0} and {1} was removed."),
			GetTradeRouteNodeName1(tradeRoutePair),
			GetTradeRouteNodeName2(tradeRoutePair)
		);
		_simulation->AddPopup(playerId1, text);
		if (playerId2 != -1) {
			_simulation->AddPopup(playerId2, text);
		}
	}
}

void WorldTradeSystem::Tick1Sec()
{
	//PUN_LOG("WorldTrade Tick1Sec");
	// Initialize after allplayers started
	if (!_isInitialized && _simulation->AllPlayerHasTownhallAfterInitialTicks())
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
			_resourceEnumToPrice100Vec[i].push_back(price100(static_cast<ResourceEnum>(i)));
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

#undef LOCTEXT_NAMESPACE