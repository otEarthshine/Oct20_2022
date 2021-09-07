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



#undef LOCTEXT_NAMESPACE