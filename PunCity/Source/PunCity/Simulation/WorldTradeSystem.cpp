// Fill out your copyright notice in the Description page of Project Settings.


#include "WorldTradeSystem.h"
#include "PunCity/CppUtils.h"
#include "PunCity/PunTimer.h"
#include "Building.h"

#define LOCTEXT_NAMESPACE "WorldTradeSystem"

void WorldTradeSystem::RefreshTradeClusters()
{
	//SCOPE_TIMER("RefreshTradeClusters");
	//
	//auto pathAI = _simulation->pathAI(true);

	//_tradeClusterToPlayerIds.clear();

	//_simulation->ExecuteOnPlayersAndAI([&](int32 playerId) 
	//{
	//	if (!_simulation->IsPlayerInitialized(playerId)) {
	//		return;
	//	}
	//	
	//	// if already in cluster, ignore
	//	for (auto& playerIds : _tradeClusterToPlayerIds) {
	//		if (CppUtils::Contains(playerIds, playerId)) {
	//			return;
	//		}
	//	}

	//	WorldTile2 gateTile = _simulation->townhallGateTile(playerId);
	//	WorldTile2 startTile = WorldTile2::Invalid;

	//	auto tryFindStartTile = [&](WorldTile2 shift) {
	//		if (startTile == WorldTile2::Invalid) {
	//			WorldTile2 testTile = gateTile + shift;
	//			if (pathAI->isRoad(testTile.x, testTile.y)) {
	//				startTile = testTile;
	//			}
	//		}
	//	};
	//	tryFindStartTile(WorldTile2(1, 0));
	//	tryFindStartTile(WorldTile2(-1, 0));
	//	tryFindStartTile(WorldTile2(0, 1));
	//	tryFindStartTile(WorldTile2(0, -1));
	//	PUN_CHECK(startTile.isValid());
	//	

	//	std::vector<WorldTile2> tileQueue;
	//	std::vector<bool> visitedTiles(GameMapConstants::TilesPerWorld, false);
	//	std::vector<int32> newClusterPlayerIds;

	//	tileQueue.push_back(startTile);

	//	for (int32 i = 0; i < 30000; i++)
	//	{
	//		if (tileQueue.empty()) {
	//			PUN_LOG("RefreshTradeClusters Flood %d", i);
	//			break;
	//		}
	//		
	//		WorldTile2 curTile = tileQueue.back();
	//		tileQueue.pop_back();

	//		int32 curTileId = curTile.tileId();
	//		if (visitedTiles[curTileId]) {
	//			continue;
	//		}
	//		visitedTiles[curTileId] = true;

	//		auto tryQueue = [&](WorldTile2 shift) {
	//			WorldTile2 tile = curTile + shift;
	//			if (pathAI->isRoad(tile.x, tile.y)) {
	//				tileQueue.push_back(tile);
	//			}
	//		};

	//		tryQueue(WorldTile2(1, 0));
	//		tryQueue(WorldTile2(-1, 0));
	//		tryQueue(WorldTile2(0, 1));
	//		tryQueue(WorldTile2(0, -1));

	//		//// Found townhall
	//		//if (_simulation->tileHasBuilding(curTile) &&
	//		//	_simulation->buildingEnumAtTile(curTile) == CardEnum::Townhall)
	//		//{
	//		//	CppUtils::TryAdd(newClusterPlayerIds, _simulation->tileOwner(curTile));
	//		//}
	//	}

	//	PUN_CHECK(tileQueue.empty());
	//	

	//	// TODO: remove
	//	newClusterPlayerIds.push_back(playerId);
	//	
	//	PUN_CHECK(newClusterPlayerIds.size() > 0);

	//	_tradeClusterToPlayerIds.push_back(newClusterPlayerIds);
	//});
}
//std::vector<int32> WorldTradeSystem::GetTradePartners(int32 playerId)
//{
//	//for (auto& playerIds :_tradeClusterToPlayerIds) {
//	//	if (CppUtils::Contains(playerIds, playerId)) {
//	//		return playerIds;
//	//	}
//	//}
//	return std::vector<int32>();
//}
std::vector<int32> WorldTradeSystem::GetTradePartners(int32 playerId) {
	return _playerIdToTradePartners[playerId];
}

void WorldTradeSystem::TryEstablishTradeRoute(FSetIntercityTrade command)
{
	SCOPE_TIMER("TryEstablishTradeRoute");
	
	auto pathAI = _simulation->pathAI(true);
	
	int32 playerId = command.playerId;
	int32 targetPlayerId = _simulation->building(command.buildingIdToEstablishTradeRoute).playerId();

	// Already established
	if (CppUtils::Contains(_playerIdToTradePartners[playerId], targetPlayerId)) {
		_simulation->AddPopupToFront(playerId, 
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

	WorldTile2 startTile = findNearestRoadTile(_simulation->townhallGateTile(playerId));
	WorldTile2 targetTile = findNearestRoadTile(_simulation->building(command.buildingIdToEstablishTradeRoute).gateTile());
	
	if (!startTile.isValid() ||
		!targetTile.isValid()) 
	{
		_simulation->AddPopupToFront(playerId, 
			LOCTEXT("ConnectTownhallsToEstablishTradePop", "Connect your Townhall to target Townhall with Intercity Road to establish a trade route."), 
			ExclusiveUIEnum::None, "PopupCannot"
		);
		return;
	}

	std::vector<WorldTile2> tileQueue;
	std::vector<bool> visitedTiles(GameMapConstants::TilesPerWorld, false);

	tileQueue.push_back(startTile);

	for (int32 i = 0; i < 30000; i++)
	{
		if (tileQueue.empty()) {
			_simulation->AddPopupToFront(playerId, 
				LOCTEXT("NeedIntercityToMakeTradeRoute", "Need intercity road to establish a trade route. Connect your Townhall to target Townhall with Road."), 
				ExclusiveUIEnum::None, "PopupCannot"
			);
			PUN_LOG("RefreshTradeClusters Flood %d", i);
			return;
		}

		WorldTile2 curTile = tileQueue.back();
		tileQueue.pop_back();

		int32 curTileId = curTile.tileId();
		if (visitedTiles[curTileId]) {
			continue;
		}
		visitedTiles[curTileId] = true;

		if (curTile == targetTile)
		{
			// Connect both players
			_playerIdToTradePartners[playerId].push_back(targetPlayerId);
			_playerIdToTradePartners[targetPlayerId].push_back(playerId);
			_simulation->RecalculateTaxDelayed(playerId);
			_simulation->RecalculateTaxDelayed(targetPlayerId);

			std::stringstream ss;
			ss << "Trade Route was established between " << _simulation->townName(playerId) << " and " << _simulation->townName(targetPlayerId);
			ss << "!\nTrade Route Income varies with the population of both cities.";
			_simulation->AddPopup(playerId, ss.str());
			_simulation->AddPopup(targetPlayerId, ss.str());

			return;
		}

		auto tryQueue = [&](WorldTile2 shift) {
			WorldTile2 tile = curTile + shift;
			if (pathAI->isRoad(tile.x, tile.y)) {
				tileQueue.push_back(tile);
			}
		};

		tryQueue(WorldTile2(1, 0));
		tryQueue(WorldTile2(-1, 0));
		tryQueue(WorldTile2(0, 1));
		tryQueue(WorldTile2(0, -1));

		tryQueue(WorldTile2(1, 1));
		tryQueue(WorldTile2(1, -1));
		tryQueue(WorldTile2(-1, 1));
		tryQueue(WorldTile2(-1, -1));
	}

	UE_DEBUG_BREAK();
}
void WorldTradeSystem::TryCancelTradeRoute(FSetIntercityTrade command)
{
	int32 playerId = command.playerId;
	int32 targetPlayerId = _simulation->building(command.buildingIdToEstablishTradeRoute).playerId();
	
	bool succeed = CppUtils::TryRemove(_playerIdToTradePartners[playerId], targetPlayerId);
	if (succeed)
	{
		CppUtils::TryRemove(_playerIdToTradePartners[targetPlayerId], playerId);
		_simulation->RecalculateTaxDelayed(playerId);
		_simulation->RecalculateTaxDelayed(targetPlayerId);

		std::stringstream ss;
		ss << "Trade Route between " << _simulation->townName(playerId) << " and " << _simulation->townName(targetPlayerId) << " was removed.";
		_simulation->AddPopup(playerId, ss.str());
		_simulation->AddPopup(targetPlayerId, ss.str());
	}
}
void WorldTradeSystem::RemoveAllTradeRoutes(int32 playerId)
{
	std::vector<int32>& partners = _playerIdToTradePartners[playerId];
	for (int32 partnerId : partners) {
		CppUtils::TryRemove(_playerIdToTradePartners[partnerId], playerId);
	}
	partners.clear();
}


#undef LOCTEXT_NAMESPACE