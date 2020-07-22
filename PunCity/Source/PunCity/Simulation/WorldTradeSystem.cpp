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
		PUN_CHECK(startTile != WorldTile2::Invalid);
		

		std::vector<WorldTile2> tileQueue;
		std::vector<bool> visitedTiles(GameMapConstants::TilesPerWorld, false);
		std::vector<int32> newClusterPlayerIds;

		tileQueue.push_back(startTile);

		for (int32 i = 0; i < 30000; i++)
		{
			WorldTile2 curTile = tileQueue.back();
			tileQueue.pop_back();
			
			if (visitedTiles[curTile.tileId()]) {
				continue;
			}
			visitedTiles[curTile.tileId()] = true;

			// Road, try queue more tiles
			if (pathAI->isRoad(curTile.x, curTile.y))
			{
				tileQueue.push_back(curTile + WorldTile2(1, 0));
				tileQueue.push_back(curTile + WorldTile2(-1, 0));
				tileQueue.push_back(curTile + WorldTile2(0, 1));
				tileQueue.push_back(curTile + WorldTile2(0, -1));
			}
			else
			{
				// Found townhall
				if (_simulation->tileHasBuilding(curTile) &&
					_simulation->buildingEnumAtTile(curTile) == CardEnum::Townhall)
				{
					CppUtils::TryAdd(newClusterPlayerIds, _simulation->tileOwner(curTile));
				}
			}
		}

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