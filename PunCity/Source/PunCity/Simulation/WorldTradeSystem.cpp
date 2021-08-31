// Fill out your copyright notice in the Description page of Project Settings.


#include "WorldTradeSystem.h"
#include "PunCity/CppUtils.h"
#include "PunCity/PunTimer.h"
#include "Building.h"

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
	tradeRoutePair.buildingId1 = command.intVar2;
	tradeRoutePair.townId2 = command.intVar3;
	tradeRoutePair.buildingId2 = command.intVar4;

	const Building& building1 = _simulation->building(tradeRoutePair.buildingId1);
	const Building& building2 = _simulation->building(tradeRoutePair.buildingId2);

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

	WorldTile2 startTile = findNearestRoadTile(_simulation->building(tradeRoutePair.buildingId1).gateTile());
	WorldTile2 targetTile = findNearestRoadTile(_simulation->building(tradeRoutePair.buildingId2).gateTile());

	if (!startTile.isValid() ||
		!targetTile.isValid()) 
	{
		_simulation->AddPopupToFront(playerId1,
			LOCTEXT("ConnectTownhallsToEstablishTradePop", "Connect your Townhall to target Townhall with Intercity Road to establish a trade route."), 
			ExclusiveUIEnum::None, "PopupCannot"
		);
		return;
	}

	std::vector<uint32_t> path;
	bool succeed = _simulation->pathAI()->FindPathRoadOnly(startTile.x, startTile.y, targetTile.x, targetTile.y, path);

	if (succeed)
	{
		// Connect both players
		_tradeRoutePairs.push_back(tradeRoutePair);

		_simulation->RecalculateTaxDelayedPlayer(playerId1);
		if (playerId2 != -1) {
			_simulation->RecalculateTaxDelayedPlayer(playerId2);
		}

		// TODO: Get Proper name for single building that can be traded with
		FText text = FText::Format(
			LOCTEXT("TradeRouteEstablish_Pop", "Trade Route was established between {0} and {1}!\nTrade Route Income varies with the population of both cities."),
			GetTradeRouteNodeName1(tradeRoutePair),
			GetTradeRouteNodeName2(tradeRoutePair)
		);
		_simulation->AddPopup(playerId1, text);
		if (playerId2 != -1) {
			_simulation->AddPopup(playerId2, text);
		}

		SortTradeRoutes();
	}
	else {
		_simulation->AddPopupToFront(playerId1,
			LOCTEXT("NeedIntercityToMakeTradeRoute", "Need intercity road to establish a trade route. Connect your Townhall to target Townhall with Road."),
			ExclusiveUIEnum::None, "PopupCannot"
		);
	}
	
}
void WorldTradeSystem::TryCancelTradeRoute(const FGenericCommand& command)
{
	TradeRoutePair tradeRoutePair;
	tradeRoutePair.townId1 = command.intVar1;
	tradeRoutePair.buildingId1 = command.intVar2;
	tradeRoutePair.townId2 = command.intVar3;
	tradeRoutePair.buildingId2 = command.intVar4;
	
	bool succeed = CppUtils::TryRemove(_tradeRoutePairs, tradeRoutePair);
	if (succeed)
	{
		int32 playerId1 = _simulation->building(tradeRoutePair.buildingId1).playerId();
		check(playerId1 != -1);
		int32 playerId2 = _simulation->building(tradeRoutePair.buildingId2).playerId();
		
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