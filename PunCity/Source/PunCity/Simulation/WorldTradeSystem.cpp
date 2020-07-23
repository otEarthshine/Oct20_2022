// Fill out your copyright notice in the Description page of Project Settings.


#include "WorldTradeSystem.h"
#include "PunCity/CppUtils.h"
#include "PunCity/PunTimer.h"

void WorldTradeSystem::RefreshTradeClusters()
{
	SCOPE_TIMER("RefreshTradeClusters");
	
	auto pathAI = _simulation->pathAI(true);

	_tradeClusterToPlayerIds.clear();

	_simulation->ExecuteOnPlayersAndAI([&](int32 playerId) 
	{
		if (!_simulation->IsPlayerInitialized(playerId)) {
			return;
		}
		
		// if already in cluster, ignore
		for (auto& playerIds : _tradeClusterToPlayerIds) {
			if (CppUtils::Contains(playerIds, playerId)) {
				return;
			}
		}

		WorldTile2 gateTile = _simulation->townhallGateTile(playerId);
		WorldTile2 startTile = WorldTile2::Invalid;

		auto tryFindStartTile = [&](WorldTile2 shift) {
			if (startTile == WorldTile2::Invalid) {
				WorldTile2 testTile = gateTile + shift;
				if (pathAI->isRoad(testTile.x, testTile.y)) {
					startTile = testTile;
				}
			}
		};
		tryFindStartTile(WorldTile2(1, 0));
		tryFindStartTile(WorldTile2(-1, 0));
		tryFindStartTile(WorldTile2(0, 1));
		tryFindStartTile(WorldTile2(0, -1));
		PUN_CHECK(startTile.isValid());
		

		std::vector<WorldTile2> tileQueue;
		std::vector<bool> visitedTiles(GameMapConstants::TilesPerWorld, false);
		std::vector<int32> newClusterPlayerIds;

		tileQueue.push_back(startTile);

		for (int32 i = 0; i < 30000; i++)
		{
			if (tileQueue.empty()) {
				PUN_LOG("RefreshTradeClusters Flood %d", i);
				break;
			}
			
			WorldTile2 curTile = tileQueue.back();
			tileQueue.pop_back();

			int32 curTileId = curTile.tileId();
			if (visitedTiles[curTileId]) {
				continue;
			}
			visitedTiles[curTileId] = true;

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

			//// Found townhall
			//if (_simulation->tileHasBuilding(curTile) &&
			//	_simulation->buildingEnumAtTile(curTile) == CardEnum::Townhall)
			//{
			//	CppUtils::TryAdd(newClusterPlayerIds, _simulation->tileOwner(curTile));
			//}
		}

		PUN_CHECK(tileQueue.empty());
		

		// TODO: remove
		newClusterPlayerIds.push_back(playerId);
		
		PUN_CHECK(newClusterPlayerIds.size() > 0);

		_tradeClusterToPlayerIds.push_back(newClusterPlayerIds);
	});
}
std::vector<int32> WorldTradeSystem::GetTradePartners(int32 playerId)
{
	for (auto& playerIds :_tradeClusterToPlayerIds) {
		if (CppUtils::Contains(playerIds, playerId)) {
			return playerIds;
		}
	}
	return std::vector<int32>();
}