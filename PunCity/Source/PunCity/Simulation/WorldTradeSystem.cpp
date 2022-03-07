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

bool WorldTradeSystem::CanEstablishTradeRoute(const FGenericCommand& command)
{
	// TODO: this is for demolish.. check if trade routes are still valid
	return true;
}

void WorldTradeSystem::TryEstablishTradeRoute(const FGenericCommand& command)
{
	// Trade Route always connect capital to capital
	TradeRoutePair tradeRoutePair;
	tradeRoutePair.townId1 = command.intVar1;
	tradeRoutePair.townId2 = command.intVar2;

	if (IsMajorTown(tradeRoutePair.townId1)) {
		tradeRoutePair.townId1 = _simulation->townPlayerId(tradeRoutePair.townId1);
	}
	if (IsMajorTown(tradeRoutePair.townId2)) {
		tradeRoutePair.townId2 = _simulation->townPlayerId(tradeRoutePair.townId2);
	}

	Building& building1 = _simulation->building(_simulation->GetTownhallId(tradeRoutePair.townId1));
	Building& building2 = _simulation->building(_simulation->GetTownhallId(tradeRoutePair.townId2));

	tradeRoutePair.distance = WorldTile2::Distance(building1.centerTile(), building2.centerTile());
	
	int32 playerId1 = building1.playerId();
	check(building1.townId() != -1);
	int32 playerId2 = building2.playerId();

	TryConnectTradeEnum connectTradeEnum = TryEstablishTradeRoute_Helper(tradeRoutePair);
	
	if (connectTradeEnum == TryConnectTradeEnum::CanConnectLand ||
		connectTradeEnum == TryConnectTradeEnum::CanConnectWater)
	{
		// Connect both players
		_tradeRoutePairs.push_back(tradeRoutePair);

		if (playerId1 != -1) _simulation->RecalculateTaxDelayedPlayer(playerId1);
		if (playerId2 != -1) _simulation->RecalculateTaxDelayedPlayer(playerId2);


		// TODO: Get Proper name for single building that can be traded with
		FText text = FText::Format(
			LOCTEXT("TradeRouteEstablish_Pop", "Trade Route was established between {0} and {1}!<space>Trade Route Income varies with the population of both cities."),
			GetTradeRouteNodeName1(tradeRoutePair),
			GetTradeRouteNodeName2(tradeRoutePair)
		);
		_simulation->AddPopupToFront(playerId1, text, ExclusiveUIEnum::TownAutoTradeUI, "");
		_simulation->AddPopupToFront(playerId2, text, ExclusiveUIEnum::TownAutoTradeUI, "");

		SortTradeRoutes();


		/*
		 * Unlocks Caravansary
		 */
		if (connectTradeEnum == TryConnectTradeEnum::CanConnectLand &&
			_simulation->TryDoCallOnceAction(command.playerId, PlayerCallOnceActionEnum::UnlockCaravan))
		{
			_simulation->AddDrawCards(command.playerId, CardEnum::Caravansary);

			int32 buildingEnumInt = static_cast<int32>(CardEnum::Caravansary);

			_simulation->AddPopup(
				PopupInfo(command.playerId,
					FText::Format(
						LOCTEXT("UnlockedBuilding_Pop", "Unlocked Caravansary.<space>Trade Caravans bring profit from Land Trade Route.<space>Would you like to buy a Caravansary card for {0}<img id=\"Coin\"/>."),
						TEXT_NUM(_simulation->GetCardPrice(command.playerId, CardEnum::Caravansary))
					),
					{ LOCTEXT("Buy", "Buy"), LOCTEXT("Refuse", "Refuse") },
					PopupReceiverEnum::DoneResearchBuyCardEvent, false, "ResearchComplete", buildingEnumInt
				)
			);
		}
	}
	else
	{
		TryEstablishTradeFailPopup(connectTradeEnum, command);
	}
	
}

void WorldTradeSystem::TryEstablishTradeFailPopup(TryConnectTradeEnum connectTradeEnum, const FGenericCommand& command)
{
	if (connectTradeEnum == TryConnectTradeEnum::AlreadyConnected)
	{
		_simulation->AddPopupToFront(command.playerId,
			LOCTEXT("AlreadyHasTradeRoute", "Already establish the trade route."),
			ExclusiveUIEnum::None, "PopupCannot"
		);
	}
	else if (connectTradeEnum == TryConnectTradeEnum::NeedLandRoute)
	{
		// Complain about needing road to trade
		_simulation->AddPopupToFront(command.playerId,
			LOCTEXT("NeedIntercityRoadToMakeTradeRoute", "Need intercity road to establish the trade route. Connect your Townhall to target Townhall with Road."),
			ExclusiveUIEnum::TownAutoTradeUI, "PopupCannot"
		);
	}
	else if (connectTradeEnum == TryConnectTradeEnum::NeedPort)
	{
		// Complain about needing port
		_simulation->AddPopupToFront(command.playerId,
			LOCTEXT("NeedPortToMakeTradeRoute", "Need Trading Port to establish the trade route."),
			ExclusiveUIEnum::TownAutoTradeUI, "PopupCannot"
		);
	}
}

TryConnectTradeEnum WorldTradeSystem::TryEstablishTradeRoute_Helper(const TradeRoutePair& tradeRoutePair)
{
	SCOPE_TIMER("TryEstablishTradeRoute");
	
	auto pathAI = _simulation->pathAI();
	
	// Already established
	if (HasTradeRoute(tradeRoutePair)) 
	{
		return TryConnectTradeEnum::AlreadyConnected;
	}
	

	/*
	 * Land Trade
	 * If any pair of colony/capital can be connected, establish the trade route between capitals
	 */
	std::vector<int32> townIds1;
	std::vector<int32> townIds2;

	if (IsMajorTown(tradeRoutePair.townId1)) {
		int32 townPlayerId = _simulation->townPlayerId(tradeRoutePair.townId1);
		townIds1 = _simulation->GetTownIds(townPlayerId);
	} else {
		townIds1 = { tradeRoutePair.townId1 };
	}
	if (IsMajorTown(tradeRoutePair.townId2)) {
		int32 townPlayerId = _simulation->townPlayerId(tradeRoutePair.townId2);
		townIds2 = _simulation->GetTownIds(townPlayerId);
	} else {
		townIds2 = { tradeRoutePair.townId2 };
	}

	for (int32 townId1 : townIds1) {
		for (int32 townId2 : townIds2)
		{
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

			Building& building1 = _simulation->building(_simulation->GetTownhallId(townId1));
			Building& building2 = _simulation->building(_simulation->GetTownhallId(townId2));

			WorldTile2 startTile = findNearestRoadTile(building1.gateTile());
			WorldTile2 targetTile = findNearestRoadTile(building2.gateTile());

			if (startTile.isValid() && targetTile.isValid())
			{
				std::vector<uint32_t> path;
				bool succeed = _simulation->pathAI()->FindPathRoadOnly(startTile.x, startTile.y, targetTile.x, targetTile.y, path);

				if (succeed) {
					return TryConnectTradeEnum::CanConnectLand;
				}
			}
		}
	}


	/*
	 * Water Trade
	 */

	//! Not Possible to Water Trade? This is because the target has no port
	bool targetHasPort = false;
	for (int32 townId1 : townIds1)
	{
		std::vector<int32> targetTownPortIds = _simulation->GetPortIds(townId1);
		if (targetTownPortIds.size() > 0) {
			targetHasPort = true;
		}
	}
	if (!targetHasPort) {
		return TryConnectTradeEnum::NeedLandRoute;
	}

	bool selfHasPort = false;
	for (int32 townId2 : townIds2)
	{
		std::vector<int32> selfTownPortIds = _simulation->GetPortIds(townId2);
		if (selfTownPortIds.size() > 0) {
			selfHasPort = true;
		}
	}

	return selfHasPort ? TryConnectTradeEnum::CanConnectWater : TryConnectTradeEnum::NeedPort;

	

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
		//check(playerId1 != -1);
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

void WorldTradeSystem::RemoveTradeRouteNode(int32 townId)
{
	for (int32 i = _tradeRoutePairs.size(); i-- > 0;) {
		if (_tradeRoutePairs[i].HasTownId(townId)) {
			_tradeRoutePairs.erase(_tradeRoutePairs.begin() + i);

			int32 playerId1 = _simulation->building(_simulation->GetTownhallId(_tradeRoutePairs[i].townId1)).playerId();
			int32 playerId2 = _simulation->building(_simulation->GetTownhallId(_tradeRoutePairs[i].townId2)).playerId();

			_simulation->RecalculateTaxDelayedPlayer(playerId1);
			_simulation->RecalculateTaxDelayedPlayer(playerId2);
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

		_enumToSupplyValue100[i] = std::max(_enumToSupplyValue100[i], GetMinSupplyValue100());

		check(_enumToSupplyValue100[i] >= 0);
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