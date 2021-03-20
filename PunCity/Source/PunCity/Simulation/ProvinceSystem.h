// Pun Dumnernchanvanit's

#pragma once

#include "IGameSimulationCore.h"
#include "PunCity/DisplaySystem/PunTerrainGenerator.h"
#include "PunCity/PunUtils.h"
#include "PunCity/PunTimer.h"
#include "PunCity/CppUtils.h"

// ClockwiseShift
const std::vector<WorldTile2x2> shift8 = {
		WorldTile2x2(0, -1),
		WorldTile2x2(1, -1),
		WorldTile2x2(1, 0),
		WorldTile2x2(1, 1),
		WorldTile2x2(0, 1),
		WorldTile2x2(-1, 1),
		WorldTile2x2(-1, 0),
		WorldTile2x2(-1, -1),
};


/**
 * 
 */
class PROTOTYPECITY_API ProvinceSystem
{
public:

	void InitProvince(IGameSimulationCore* simulation)
	{
		SCOPE_TIMER("InitProvince");
		_simulation = simulation;

		if (simulation->isLoadingFromFile()) {
			return;
		}

		auto& terrainGen = simulation->terrainGenerator();

		_provinceCenters.resize(GameMapConstants::TotalRegions, WorldTile2x2());

		std::vector<std::vector<WorldTile2x2>> floodStackToProvinceId;
		floodStackToProvinceId.resize(GameMapConstants::TotalRegions);

		_provinceMountainTileCount.resize(GameMapConstants::TotalRegions);
		_provinceRiverTileCount.resize(GameMapConstants::TotalRegions);
		_provinceOceanTileCount.resize(GameMapConstants::TotalRegions);
		_provinceFlatTileCount.resize(GameMapConstants::TotalRegions);

		int32 totalTiles2x2 = GameMapConstants::TilesPerWorld / 4;
		_provinceId2x2.resize(totalTiles2x2, EmptyProvinceId);
		for (size_t i = 0; i < totalTiles2x2; i++)
		{
			TerrainTileType tileType = simulation->terraintileType(WorldTile2x2(i).worldTile2().tileId());
			if (tileType == TerrainTileType::Mountain) {
				_provinceId2x2[i] = MountainProvinceId;
			}
			else if (tileType == TerrainTileType::River) {
				_provinceId2x2[i] = RiverProvinceId;
			}
			else if (tileType == TerrainTileType::Ocean) {
				_provinceId2x2[i] = OceanProvinceId;
			}
			else {
				_provinceId2x2[i] = EmptyProvinceId;
			}
		}

		// Start filling the stack
		for (int32 provinceId = 0; provinceId < GameMapConstants::TotalRegions; provinceId++)
		{
			WorldRegion2 region(provinceId);
			WorldTile2 centerTile = region.centerTile();
			// Turn into hex
			if (region.y % 2 == 0) {
				centerTile.x += CoordinateConstants::TilesPerRegion / 2 - 1;
			}

			int32 randX = GameRand::DisplayRand(provinceId);
			int32 randY = GameRand::DisplayRand(randX);

			auto tryFillFirstItem = [&](int32 randSize) -> bool
			{
				// Randomize first time in small area around center
				randX = GameRand::DisplayRand(randY);
				randY = GameRand::DisplayRand(randX);
				centerTile.x += (randX % randSize) - randSize / 2;
				centerTile.y += (randY % randSize) - randSize / 2;

				if (centerTile.isValid())
				{
					WorldTile2x2 tile2x2Origin = WorldTile2x2(centerTile);


					_provinceCenters[provinceId] = tile2x2Origin;

					int32 tile2x2Id = tile2x2Origin.tile2x2Id();
					if (_provinceId2x2[tile2x2Id] == EmptyProvinceId) {
						_provinceId2x2[tile2x2Id] = provinceId;
						floodStackToProvinceId[provinceId].push_back(tile2x2Origin);
						_provinceFlatTileCount[provinceId] += 4;
						return true;
					}
				}
				return false;
			};

			// Got a good tile the first sample, continue to next provinceId
			if (tryFillFirstItem(16)) {
				continue;
			}

			// If first time random doesn't work, brute force random to find an empty tile
			for (int32 i = 0; i < 20; i++) {
				if (tryFillFirstItem(16)) {
					break;
				}
			}
		}

		// Flood X levels on flat ground
		FloodBreadthFirst(floodStackToProvinceId, 64, [&](WorldTile2x2 tile2x2, int32& provinceId, std::vector<WorldTile2x2>& newStack, int32 floodLevel) {
			int32 tile2x2Id = tile2x2.tile2x2Id();
			int32 curProvinceId = _provinceId2x2[tile2x2Id];
			if (curProvinceId == EmptyProvinceId) {
				_provinceId2x2[tile2x2Id] = provinceId;
				newStack.push_back(tile2x2);

				_provinceFlatTileCount[provinceId] += 4;
			}
		});


		int32 proviceIdsSize = floodStackToProvinceId.size(); // New provinceId that won't overlap with the previous
#if WITH_EDITOR
		for (size_t i = 0; i < totalTiles2x2; i++) {
			int32 provinceId = _provinceId2x2[i];
			if (IsValidRawProvinceId(provinceId)) {
				PUN_CHECK(provinceId < proviceIdsSize);
				PUN_CHECK(provinceId >= 0);
			}
		}
#endif

		/*
		 * Flood more level to fill in water and mountain
		 */
		for (int32 provinceId = 0; provinceId < proviceIdsSize; provinceId++) {
			floodStackToProvinceId[provinceId].clear();
		}

		// Get edge tile2x2 for further flooding
		for (size_t i = 0; i < totalTiles2x2; i++)
		{
			int32 provinceId = _provinceId2x2[i];
			if (IsValidRawProvinceId(provinceId)) {
				// Get edges
				WorldTile2x2 tile2x2(i);
				if (IsMountainOrWaterProvinceId(_provinceId2x2[WorldTile2x2(tile2x2.x + 1, tile2x2.y).tile2x2Id()]) ||
					IsMountainOrWaterProvinceId(_provinceId2x2[WorldTile2x2(tile2x2.x - 1, tile2x2.y).tile2x2Id()]) ||
					IsMountainOrWaterProvinceId(_provinceId2x2[WorldTile2x2(tile2x2.x, tile2x2.y + 1).tile2x2Id()]) ||
					IsMountainOrWaterProvinceId(_provinceId2x2[WorldTile2x2(tile2x2.x, tile2x2.y - 1).tile2x2Id()]))
				{
					PUN_CHECK(provinceId < proviceIdsSize);
					PUN_CHECK(provinceId >= 0);
					floodStackToProvinceId[provinceId].push_back(tile2x2);
				}
			}
		}

		/*
		 * Flood into Mountain and Water
		 */
		FloodBreadthFirst(floodStackToProvinceId, 16, [&](WorldTile2x2 tile2x2, int32& provinceId, std::vector<WorldTile2x2>& newStack, int32 floodLevel)
		{
			int32 tile2x2Id = tile2x2.tile2x2Id();
			int32 curProvinceId = _provinceId2x2[tile2x2Id];

			auto floodProvinceId = [&]() {
				_provinceId2x2[tile2x2Id] = provinceId;
				newStack.push_back(tile2x2);
			};

			if (curProvinceId == MountainProvinceId) {
				floodProvinceId();
				_provinceMountainTileCount[provinceId] += 4;
			}
			else if (curProvinceId == EmptyProvinceId) {
				floodProvinceId();
				_provinceFlatTileCount[provinceId] += 4;

				// Flood the whole empty part, this prevents having flat land connection in weird spots.

				std::vector<WorldTile2x2> flatStack;
				flatStack.push_back(tile2x2);

				auto tryFlood = [&](WorldTile2x2 tile2x2In) {
					int32 tile2x2IdIn = tile2x2In.tile2x2Id();
					if (_provinceId2x2[tile2x2IdIn] == EmptyProvinceId)
					{
						_provinceId2x2[tile2x2IdIn] = provinceId;
						newStack.push_back(tile2x2);
						_provinceFlatTileCount[provinceId] += 4;

						flatStack.push_back(tile2x2In);
					}
				};

				while (flatStack.size() > 0)
				{
					// Look around the tile to find the tile that has not been filled yet
					WorldTile2x2 lastTile2x2 = flatStack.back();
					flatStack.pop_back();

					tryFlood(WorldTile2x2(lastTile2x2.x + 1, lastTile2x2.y));
					tryFlood(WorldTile2x2(lastTile2x2.x - 1, lastTile2x2.y));
					tryFlood(WorldTile2x2(lastTile2x2.x, lastTile2x2.y + 1));
					tryFlood(WorldTile2x2(lastTile2x2.x, lastTile2x2.y - 1));
				}
			}
			else if (curProvinceId == RiverProvinceId) {
				floodProvinceId();
				_provinceRiverTileCount[provinceId] += 4;
			}
			else if (curProvinceId == OceanProvinceId) {
				floodProvinceId();
				_provinceOceanTileCount[provinceId] += 4;
			}
		});


		/*
		 * Generate Edge
		 */
		auto tryFloodEdge = [&](const WorldTile2x2& localTile2x2, const WorldTile2x2& localPrevTile2x2,
			const int32& curProvinceId, std::vector<WorldTile2x2>& stack, std::vector<WorldTile2x2>& prevStack)
		{
			int32 localTile2x2Id = localTile2x2.tile2x2Id();
			int32 localProvinceId = abs(_provinceId2x2[localTile2x2Id]);

			if (localProvinceId == curProvinceId ) //&&
				//_provinceId2x2[localTile2x2Id] >= 0) // If already pushed on to stack, don't look it up again
			{
				int32 invalidSides = (abs(_provinceId2x2[WorldTile2x2(localTile2x2.x + 1, localTile2x2.y).tile2x2Id()]) != localProvinceId) +
					(abs(_provinceId2x2[WorldTile2x2(localTile2x2.x - 1, localTile2x2.y).tile2x2Id()]) != localProvinceId) +
					(abs(_provinceId2x2[WorldTile2x2(localTile2x2.x, localTile2x2.y + 1).tile2x2Id()]) != localProvinceId) +
					(abs(_provinceId2x2[WorldTile2x2(localTile2x2.x, localTile2x2.y - 1).tile2x2Id()]) != localProvinceId);
				bool isEdge = (3 >= invalidSides && invalidSides >= 1);

				if (isEdge) {
					stack.push_back(localTile2x2);
					prevStack.push_back(localPrevTile2x2);
					_provinceId2x2[localTile2x2Id] = -localProvinceId;
				}
			}
		};
		
		{
			_regionToProvinceIds.resize(GameMapConstants::TotalRegions);
			_provinceRectAreas.resize(GameMapConstants::TotalRegions, TileArea::GetLoopMinMaxInitial());
			_provinceToRegionsOverlap.resize(GameMapConstants::TotalRegions);
			PUN_CHECK(floodStackToProvinceId.size() == GameMapConstants::TotalRegions);

			_provinceEdges1.resize(GameMapConstants::TotalRegions);
			_provinceEdges2.resize(GameMapConstants::TotalRegions);

			for (size_t i = 0; i < totalTiles2x2; i++)
			{
				WorldTile2x2 curTile2x2(i);
				int32 curProvinceId = _provinceId2x2[i];

				// Fill _regionToProvinceIds for display
				if (IsValidRawProvinceId(curProvinceId)) {
					int32 absCurProvinceId = abs(curProvinceId);
					CppUtils::TryAdd(_regionToProvinceIds[curTile2x2.worldTile2().regionId()], absCurProvinceId);
				}

				if (curProvinceId >= 0 &&
					_provinceEdges1[curProvinceId].empty())
				{
					//auto insertTile2x2 = [&](const WorldTile2x2& tile2x2, const WorldTile2x2& tile2x2Shifted)
					//{
					//	// If going in the same direction as last edge
					//	size_t vecSize = _provinceEdges1[curProvinceId].size();
					//	if (vecSize >= 2) 
					//	{
					//		WorldTile2x2 prevTile = _provinceEdges1[curProvinceId][vecSize - 1];
					//		if (tile2x2 - prevTile == _provinceEdges1[curProvinceId][vecSize - 2] - prevTile) 
					//		{
					//			_provinceEdges1[curProvinceId][vecSize - 1] = tile2x2;
					//			_provinceEdges2[curProvinceId][vecSize - 1] = tile2x2Shifted;
					//			return;
					//		}
					//	}
					//	
					//	_provinceEdges1[curProvinceId].push_back(tile2x2);
					//	_provinceEdges2[curProvinceId].push_back(tile2x2Shifted);
					//};
					
					auto isInArea = [&](int32 provinceRawId) -> bool {
						return abs(provinceRawId) == curProvinceId;
					};
					
					bool succeed = GenerateEdge(i, curProvinceId, curTile2x2, _provinceEdges1[curProvinceId], _provinceEdges2[curProvinceId], isInArea, tryFloodEdge);

					// Remove the provinceId
					if (!succeed) {
						int32 absCurProvinceId = abs(curProvinceId);
						CppUtils::TryRemove(_regionToProvinceIds[curTile2x2.worldTile2().regionId()], absCurProvinceId);
						_provinceId2x2[i] = EmptyProvinceId;
					}
				}
			}
		}
		

		// TODO: Remove
		displayedProvinceThisTick.resize(_provinceEdges1.size(), false);

		/*
		 * Merge small provinces
		 */
		const int32 mergeThresholdSize = CoordinateConstants::TileIdsPerRegion * 4 / 5;

		for (int32 provinceId = 0; provinceId < proviceIdsSize; provinceId++)
		{
			// Only use valid provinces
			if (_provinceFlatTileCount[provinceId] == 0) {
				continue;
			}

			if (_provinceFlatTileCount[provinceId] < mergeThresholdSize)
			{
				//PUN_LOG("Try Merge oldProvinceId:%d flat:%d total:%d", provinceId, _provinceFlatTileCount[provinceId], provinceTileCount(provinceId));

				// Get the nearby province with the most connected tiles
				std::vector<int32> provinceIds;
				std::vector<int32> connectedTileCounts;

				std::vector<WorldTile2x2>& outerVerts = _provinceEdges2[provinceId];
				for (size_t i = 0; i < outerVerts.size(); i++)
				{
					WorldTile2x2 neighborTile2x2 = outerVerts[i];

					int32 neighborProvinceId = _provinceId2x2[neighborTile2x2.tile2x2Id()];
					if (IsValidRawProvinceId(neighborProvinceId))
					{
						neighborProvinceId = abs(neighborProvinceId);
						PUN_CHECK(IsValidNonEdgeProvinceId(neighborProvinceId));

						TerrainTileType tileType = simulation->terraintileType(neighborTile2x2.worldTile2().tileId());

						// Only merge across land
						if (tileType == TerrainTileType::None)
						{
							size_t provinceIndex = -1;
							for (size_t j = 0; j < provinceIds.size(); j++) {
								if (provinceIds[j] == neighborProvinceId) {
									provinceIndex = j;
									break;
								}
							}
							if (provinceIndex == -1) {
								provinceIndex = provinceIds.size();
								provinceIds.push_back(neighborProvinceId);
								connectedTileCounts.push_back(0);
							}

							connectedTileCounts[provinceIndex]++;
						}
					}
				}

				int32 provinceIdToMergeTo = -1;
				int32 maxConnectedTileCount = 0;

				for (size_t i = 0; i < provinceIds.size(); i++) {
					if (connectedTileCounts[i] > maxConnectedTileCount) {
						maxConnectedTileCount = connectedTileCounts[i];
						provinceIdToMergeTo = provinceIds[i];
					}
				}

				//PUN_LOG("provinceIdToMergeTo %d flat:%d total:%d maxConnectedTileCount:%d", provinceIdToMergeTo, _provinceFlatTileCount[provinceIdToMergeTo], provinceTileCount(provinceIdToMergeTo), maxConnectedTileCount);

				if (provinceIdToMergeTo != -1)
				{
					MergeProvince(provinceId, provinceIdToMergeTo, tryFloodEdge);
				}
			}
		}
		
		/*
		 * Province Connection
		 */
		_provinceConnections.resize(_provinceEdges1.size());

		for (size_t j = 0; j < _provinceEdges1.size(); j++)
		{
			std::vector<WorldTile2x2>& edges = _provinceEdges1[j];
			int32 provinceId = j;

			if (IsProvinceValid(provinceId))
			{
				auto checkConnection = [&](WorldTile2x2 tile2x2In, TerrainTileType prevTileTypeIn)
				{
					int32 neighborProvinceId = _provinceId2x2[tile2x2In.tile2x2Id()];
					if (IsValidRawProvinceId(neighborProvinceId) &&
						abs(neighborProvinceId) != provinceId)
					{
						WorldTile2 tileIn = tile2x2In.worldTile2();

						// For Flat connection, also check ifself is flat... if not it would be the other type
						TerrainTileType tileType = terrainGen.terrainTileType(tileIn);
						if (tileType == TerrainTileType::None &&
							prevTileTypeIn != TerrainTileType::None)
						{
							tileType = prevTileTypeIn;
						}

						std::vector<ProvinceConnection>& connections = _provinceConnections[provinceId];
						bool addedConnection = false;
						for (size_t i = connections.size(); i-- > 0;) {
							if (connections[i].provinceId == abs(neighborProvinceId) &&
								connections[i].tileType == tileType) 
							{
								connections[i].connectedTiles++;
								addedConnection = true;
								break;
							}
						}
						if (!addedConnection) {
							connections.push_back(ProvinceConnection(abs(neighborProvinceId), tileType));
						}
						//ProvinceConnection connection(abs(neighborProvinceId), tileType);
						//CppUtils::TryAdd(_provinceConnections[provinceId], connection);
					}
				};

				for (size_t i = 0; i < edges.size(); i++)
				{
					WorldTile2x2 tile2x2 = edges[i];
					TerrainTileType tileType = terrainGen.terrainTileType(edges[i].worldTile2());

					checkConnection(WorldTile2x2(tile2x2.x + 1, tile2x2.y), tileType);
					checkConnection(WorldTile2x2(tile2x2.x - 1, tile2x2.y), tileType);
					checkConnection(WorldTile2x2(tile2x2.x, tile2x2.y + 1), tileType);
					checkConnection(WorldTile2x2(tile2x2.x, tile2x2.y - 1), tileType);
				}
			}
		}

		/*
		 * Get rid of any tiny provinces
		 */
		const int32 tinyProvinceThreshold = CoordinateConstants::TileIdsPerRegion / 8;

		for (int32 provinceId = 0; provinceId < proviceIdsSize; provinceId++)
		{
			if (0 < _provinceFlatTileCount[provinceId] && 
				_provinceFlatTileCount[provinceId] < tinyProvinceThreshold)
			{
				ExecuteOnProvinceTiles(provinceId, [&](WorldTile2& tile)
				{
					TerrainTileType tileType = terrainGen.terrainTileType(tile);
					if (tileType == TerrainTileType::None) {
						_provinceId2x2[WorldTile2x2(tile).tile2x2Id()] = EmptyProvinceId;

						// Ensure no entry
						terrainGen.SetImpassableFlatTile(tile);
					}
					else if (tileType == TerrainTileType::Mountain) {
						_provinceId2x2[WorldTile2x2(tile).tile2x2Id()] = MountainProvinceId;
					}
					else if (tileType == TerrainTileType::Ocean) {
						_provinceId2x2[WorldTile2x2(tile).tile2x2Id()] = OceanProvinceId;
					}
					else if (tileType == TerrainTileType::River) {
						_provinceId2x2[WorldTile2x2(tile).tile2x2Id()] = RiverProvinceId;
					}
				});

				_provinceCenters[provinceId] = WorldTile2x2();
				_provinceEdges1[provinceId].clear();
				_provinceEdges2[provinceId].clear();
				
				for (const WorldRegion2& region : _provinceToRegionsOverlap[provinceId]) {
					CppUtils::TryRemove(_regionToProvinceIds[region.regionId()], region.regionId());
				}

				_provinceMountainTileCount[provinceId] = 0;
				_provinceRiverTileCount[provinceId] = 0;
				_provinceOceanTileCount[provinceId] = 0;
				_provinceFlatTileCount[provinceId] = 0;

				_provinceConnections[provinceId].clear();
				_provinceRectAreas[provinceId] = TileArea::GetLoopMinMaxInitial();
				_provinceToRegionsOverlap[provinceId].clear();
			}
		}


		_totalFlatTiles = 0;
		for (size_t i = 0; i < _provinceFlatTileCount.size(); i++) {
			_totalFlatTiles += _provinceFlatTileCount[i];
		}

		/*
		 * Set any leftover EmptyProvinceId tile to mountain so nothing uses it
		 */
		//auto pathAI = _simulation->pathAI(true);
		//for (int32 i = 0; i < GameMapConstants::TilesPerWorld; i++) 
		//{
		//	WorldTile2 tile(i);
		//	if (pathAI->isWalkable(tile.x, tile.y)) 
		//	{
		//		int32 provinceId = _provinceId2x2[WorldTile2x2(tile).tile2x2Id()];
		//		if (provinceId == MountainProvinceId || 
		//			provinceId == EmptyProvinceId)
		//		{
		//			//PUN_LOG("SetMountainTile %s", *tile.To_FString());
		//			_simulation->terrainGenerator().SetMountainTile(tile);
		//		}
		//		else if (IsWaterProvinceId(provinceId))
		//		{
		//			//PUN_LOG("SetWaterTile %s", *tile.To_FString());
		//			_simulation->terrainGenerator().SetWaterTile(tile);
		//		}
		//	}
		//}
	}

	/*
	 * Territory
	 */
	void RefreshTerritoryEdge(int32 townId, const std::vector<int32>& provinceIds)
	{
		SCOPE_TIMER("RefreshTerritoryEdge");

		if (PunSettings::TrailerMode()) {
			return;
		}
		
		if (_territoryEdges1.size() <= townId) {
			_territoryEdges1.resize(townId + 1);
			_territoryEdges2.resize(townId + 1);
			_territoryClusterProvinceIds.resize(townId + 1);
		}

		PUN_LOG("RefreshTerritoryEdge townId:%d provinceIds:%d", townId, provinceIds.size());


		if (provinceIds.size() == 0) {
			return;
		}


		/*
		 * Determine territory clusters (Oversea etc.)
		 */
		std::unordered_map<int32, std::vector<int32>> parentProvinceIdToClusterProvinceIds;
		{
			TSet<int32> floodedProvinceIds;
			for (int32 provinceId : provinceIds)
			{
				if (!floodedProvinceIds.Contains(provinceId))
				{
					// flood the part
					std::vector<int32> floodQueue;
					floodQueue.push_back(provinceId);

					while (floodQueue.size() > 0)
					{
						int32 curProvinceId = floodQueue.back();
						floodedProvinceIds.Add(curProvinceId);
						parentProvinceIdToClusterProvinceIds[provinceId].push_back(curProvinceId);

						floodQueue.pop_back();
						auto& connections = GetProvinceConnections(curProvinceId);
						for (const ProvinceConnection& connection : connections) 
						{
							// Flood into owned province
							if (_simulation->provinceOwnerTown(connection.provinceId) == townId &&
								!floodedProvinceIds.Contains(connection.provinceId)) 
							{
								floodQueue.push_back(connection.provinceId);
							}
						}

					}
				}
			}
		}
		

		/*
		 * Flood Territory Edge for each cluster
		 */
		_territoryEdges1[townId].clear();
		_territoryEdges2[townId].clear();
		_territoryClusterProvinceIds[townId].clear();
		
		for (const auto& it : parentProvinceIdToClusterProvinceIds)
		{
			const std::vector<int32>& clusterProvinceIds = it.second;

			_territoryClusterProvinceIds[townId].push_back(clusterProvinceIds);

			TileArea combinedArea = TileArea::GetLoopMinMaxInitial();
			for (int32 provinceId : clusterProvinceIds) {
				combinedArea = TileArea::CombineAreas(combinedArea, _provinceRectAreas[provinceId]);
			}

			PUN_LOG("RefreshTerritoryEdge townId:%d clusterId:%d provinceIds:%d combinedArea:%s", townId, clusterProvinceIds[0], clusterProvinceIds.size(), ToTChar(combinedArea.ToString()));

			auto tryFloodTerritoryEdge = [&](WorldTile2x2 localTile2x2, const WorldTile2x2& localPrevTile2x2,
				const int32& curProvinceId, std::vector<WorldTile2x2>& stack, std::vector<WorldTile2x2>& prevStack)
			{
				//PUN_LOG("tryFloodTerritoryEdge %d %d stack:%d", localTile2x2.x, localTile2x2.y, stack.size());

				int32 localTile2x2Id = localTile2x2.tile2x2Id();
				int32 localProvinceId = _provinceId2x2[localTile2x2Id];

				if (!IsValidRawProvinceId(localProvinceId)) {
					return;
				}
				localProvinceId = abs(localProvinceId);

				// Instead of checking for same provinceId, we check for same player owner
				if (_simulation->provinceOwnerTown(localProvinceId) == townId)
				{
					auto isInvalidSide = [&](int16 x2, int16 y2)
					{
						int32 lambdaProvinceIdId = _provinceId2x2[WorldTile2x2(x2, y2).tile2x2Id()];
						if (!IsValidRawProvinceId(lambdaProvinceIdId)) {
							return true;
						}
						lambdaProvinceIdId = abs(lambdaProvinceIdId);
						return _simulation->provinceOwnerTown(lambdaProvinceIdId) != townId;
					};

					int32 invalidSides = isInvalidSide(localTile2x2.x + 1, localTile2x2.y) +
						isInvalidSide(localTile2x2.x - 1, localTile2x2.y) +
						isInvalidSide(localTile2x2.x, localTile2x2.y + 1) +
						isInvalidSide(localTile2x2.x, localTile2x2.y - 1);
					bool isEdge = (3 >= invalidSides && invalidSides >= 1);

					if (isEdge) {
						stack.push_back(localTile2x2);
						prevStack.push_back(localPrevTile2x2);
					}
				}
			};

			_territoryEdges1[townId].push_back({});
			_territoryEdges2[townId].push_back({});

			// Loop through combinedArea to hit the start spot for edge, and generate it
			combinedArea.ExecuteOnAreaWithExit_WorldTile2x2([&](int16 x2, int16 y2)
			{
				WorldTile2x2 tile2x2(x2, y2);
				size_t tile2x2Id = tile2x2.tile2x2Id();
				int32 curProvinceId = _provinceId2x2[tile2x2Id];

				//PUN_LOG("Loop %d %d curProvinceId:%d targetTownId:%d", x2, y2, curProvinceId, townId);

				if (!IsValidRawProvinceId(curProvinceId)) {
					return false;
				}
				curProvinceId = abs(curProvinceId);

				// Ensure this is the correct cluster
				if (!CppUtils::Contains(clusterProvinceIds, curProvinceId)) {
					return false;
				}

				// Loop until we hit the territory edge
				if (_simulation->provinceOwnerTown(curProvinceId) == townId)
				{
					auto isInArea = [&](int32 provinceRawId) -> bool {
						if (!IsValidRawProvinceId(provinceRawId)) {
							return false;
						}
						return _simulation->provinceOwnerTown(abs(provinceRawId)) == townId;
					};

					GenerateEdge(tile2x2Id, curProvinceId, tile2x2, 
								_territoryEdges1[townId].back(),
								_territoryEdges2[townId].back(),
								isInArea, tryFloodTerritoryEdge, false);
					return true;
				}
				return false;
			});
		}

		PUN_LOG("RefreshTerritoryEdge edge1:%d edge2:%d", GetTerritoryEdges1(townId, 0).size(), GetTerritoryEdges2(townId, 0).size());
	}

	const std::vector<std::vector<int32>>& GetTerritoryClusters(int32 townId)
	{
		return _territoryClusterProvinceIds[townId];
	}
	int32 GetTerritoryClusterId(int32 townId, int32 provinceIdIn)
	{
		if (townId >= _territoryEdges1.size()) {
			return -1;
		}
		const auto& clusters = GetTerritoryClusters(townId);
		for (int32 i = 0; i < clusters.size(); i++)
		{
			const auto& cluster = clusters[i];
			for (int32 j = 0; j < cluster.size(); j++) {
				if (cluster[j] == provinceIdIn) {
					return i;
				}
			}
		}
		return -1;
	}
	const std::vector<WorldTile2x2>& GetTerritoryEdges1(int32 townId, int32 clusterId)
	{
		//if (_territoryEdges1.size() <= townId) {
		//	_territoryEdges1.resize(townId + 1);
		//	_territoryEdges2.resize(townId + 1);
		//	_territoryClusterProvinceIds.resize(townId + 1);
		//}
		
		return _territoryEdges1[townId][clusterId];
	}
	const std::vector<WorldTile2x2>& GetTerritoryEdges2(int32 townId, int32 clusterId)
	{
		return _territoryEdges2[townId][clusterId];
	}

	/*
	 * Get
	 */

	int32 GetProvinceIdRaw(WorldTile2 tile) {
		return _provinceId2x2[WorldTile2x2(tile).tile2x2Id()];
	}

	// Only positive id or -1 otherwise
	int32 GetProvinceIdClean(WorldTile2 tile) {
		if (!tile.isValid()) {
			return -1;
		}
		int32 provinceId = _provinceId2x2[WorldTile2x2(tile).tile2x2Id()];
		if (IsValidRawProvinceId(provinceId)) {
			return abs(provinceId);
		}
		return -1;
	}

	int32 GetProvinceId2x2(WorldTile2x2 tile2x2) {
		return _provinceId2x2[tile2x2.tile2x2Id()];
	}

	const std::vector<int32>& GetProvinceId2x2Vec() {
		return _provinceId2x2;
	}


	const std::vector<WorldTile2x2>& GetProvinceEdges1(int32 provinceId) const {
		return _provinceEdges1[provinceId];
	}
	const std::vector<WorldTile2x2>& GetProvinceEdges2(int32 provinceId) const {
		return _provinceEdges2[provinceId];
	}

	const std::vector<int32>& GetProvinceIdsFromRegionId(int32 regionId) {
		return _regionToProvinceIds[regionId];
	}
	std::vector<int32> GetProvinceIdsFromArea(TileArea area, bool smallArea)
	{
		std::vector<int32> provinceIds;

		std::vector<WorldRegion2> regions = smallArea ? area.GetOverlapRegionsSmallArea() : area.GetOverlapRegions();
		for (WorldRegion2 region : regions) {
			const std::vector<int32>& curProvinceIds = _regionToProvinceIds[region.regionId()];
			for (int32 provinceId : curProvinceIds) {
				CppUtils::TryAdd(provinceIds, provinceId);
			}
		}
		return provinceIds;
	}

	WorldTile2x2 GetProvinceCenter(int32 provinceId) {
		return _provinceCenters[provinceId];
	}
	WorldTile2 GetProvinceCenterTile(int32 provinceId) {
		return _provinceCenters[provinceId].worldTile2();
	}

	bool IsProvinceValid(int32 provinceId) {
		if (0 <= provinceId && provinceId < GameMapConstants::TotalRegions) {
			return !_provinceCenters[provinceId].IsInvalid();
		}
		return false;
	}

	/*
	 * Connections
	 */
	const std::vector<ProvinceConnection>& GetProvinceConnections(int32 provinceId) {
		return _provinceConnections[provinceId];
	}

	
	void AddTunnelProvinceConnection(int32 provinceId1, int32 provinceId2, TerrainTileType tileType)
	{
		auto changeConnectionType = [&](int32 provinceId1In, int32 provinceId2In) {
			std::vector<ProvinceConnection>& connections = _provinceConnections[provinceId1In];
			for (ProvinceConnection& connection : connections) {
				if (connection.provinceId == provinceId2In) {
					connection.tileType = tileType;
				}
			}
		};

		changeConnectionType(provinceId1, provinceId2);
		changeConnectionType(provinceId2, provinceId1);
	}
	

	bool AreAdjacentProvinces(int32 provinceId1, int32 provinceId2) {
		const std::vector<ProvinceConnection>& connections1 = _provinceConnections[provinceId1];
		for (ProvinceConnection connection : connections1) {
			if (connection.provinceId == provinceId2) {
				return true;
			}
		}
		return false;
	}


	template <typename Func>
	void ExecuteAdjacentProvinces(int32 provinceId, Func func) {
		const std::vector<ProvinceConnection>& connections = _provinceConnections[provinceId];
		for (int32 i = 0; i < connections.size(); i++) {
			func(connections[i]);
		}
	}

	template <typename Func>
	bool ExecuteAdjacentProvincesWithExitTrue(int32 provinceId, Func shouldExit) {
		const std::vector<ProvinceConnection>& connections = _provinceConnections[provinceId];
		for (int32 i = 0; i < connections.size(); i++) {
			if (shouldExit(connections[i])) {
				return true;
			}
		}
		return false;
	}

	template <typename Func>
	void ExecuteNearbyProvinces(int32 provinceId, int32 level, Func func)
	{
		std::vector<int32> visitedProvinces;
		ExecuteNearbyProvincesHelper(provinceId, level, func, visitedProvinces);
	}

	template <typename Func>
	void ExecuteNearbyProvincesHelper(int32 provinceId, int32 level, Func func, std::vector<int32>& visitedProvinces)
	{
		visitedProvinces.push_back(provinceId);

		const std::vector<ProvinceConnection>& connections = _provinceConnections[provinceId];
		for (int32 i = 0; i < connections.size(); i++)
		{
			const ProvinceConnection& connection = connections[i];

			if (CppUtils::Contains(visitedProvinces, connection.provinceId)) {
				continue;
			}

			func(connection);

			if (level > 1) {
				ExecuteNearbyProvincesHelper(connection.provinceId, level - 1, func, visitedProvinces);
			}
		}
	}

	template <typename Func>
	bool ExecuteNearbyProvincesWithExitTrue(int32 provinceId, int32 level, Func shouldExitFunc)
	{
		std::vector<int32> visitedProvinces;
		return ExecuteNearbyProvincesWithExitTrue_Helper(provinceId, level, shouldExitFunc, visitedProvinces);
	}

	template <typename Func>
	bool ExecuteNearbyProvincesWithExitTrue_Helper(int32 provinceId, int32 level, Func shouldExitFunc, std::vector<int32>& visitedProvinces)
	{
		visitedProvinces.push_back(provinceId);

		const std::vector<ProvinceConnection>& connections = _provinceConnections[provinceId];
		for (int32 i = 0; i < connections.size(); i++)
		{
			ProvinceConnection connection = connections[i];

			if (CppUtils::Contains(visitedProvinces, connection.provinceId)) {
				continue;
			}

			if (shouldExitFunc(connection)) {
				return true;
			}

			if (level > 1) {
				bool shouldExit = ExecuteNearbyProvincesWithExitTrue_Helper(connection.provinceId, level - 1, shouldExitFunc, visitedProvinces);
				if (shouldExit) {
					return true;
				}
			}
		}

		return false;
	}

	/*
	 *
	 */
	template <typename Func>
	void ExecuteOnProvinceTiles(int32 provinceId, Func func)
	{
		PUN_CHECK(IsProvinceValid(provinceId));
		TileArea area = _provinceRectAreas[provinceId];
		area.ExecuteOnArea_WorldTile2([&](WorldTile2 tile) {
			if (abs(GetProvinceIdRaw(tile)) == provinceId) {
				func(tile);
			}
		});
	}

	TileArea GetProvinceRectArea(int32 provinceId) {
		return _provinceRectAreas[provinceId];
	}

	WorldTile2 GetProvinceRandomTile(int32 provinceId, WorldTile2 floodOrigin, int32 maxRegionDist = 1, bool isIntelligent = false, int32 tries = 100)
	{
		PUN_CHECK(IsProvinceValid(provinceId));
		TileArea area = _provinceRectAreas[provinceId];

		for (int32 i = 0; i < tries; i++) 
		{
			WorldTile2 tile = area.RandomTile();
			DEBUG_ISCONNECTED_VAR(GetProvinceRandomTile);
			
			if (abs(GetProvinceIdRaw(tile)) == provinceId &&
				_simulation->IsConnected(floodOrigin, tile, maxRegionDist)) 
			{
				return tile;
			}
		}

		return WorldTile2::Invalid;
	}
	WorldTile2 GetProvinceRandomTile_NoFlood(int32 provinceId, int32 tries)
	{
		PUN_CHECK(IsProvinceValid(provinceId));
		TileArea area = _provinceRectAreas[provinceId];

		if (area.isValid())
		{
			for (int32 i = 0; i < tries; i++)
			{
				WorldTile2 tile = area.RandomTile();
				DEBUG_ISCONNECTED_VAR(GetProvinceRandomTile);

				if (abs(GetProvinceIdRaw(tile)) == provinceId) {
					return tile;
				}
			}
		}

		return WorldTile2::Invalid;
	}
	WorldTile2 GetProvinceAnyTile(int32 provinceId)
	{
		PUN_CHECK(IsProvinceValid(provinceId));
		TileArea area = _provinceRectAreas[provinceId];
		
		WorldTile2 anyTile = WorldTile2::Invalid;
		area.ExecuteOnArea_WorldTile2([&](WorldTile2 tile) {
			if (abs(GetProvinceIdRaw(tile)) == provinceId) {
				anyTile = tile;
			}
		});
		PUN_CHECK(anyTile.isValid());
		return anyTile;
	}
	


	const std::vector<WorldRegion2>& GetRegionOverlaps(int32 provinceId) {
		return _provinceToRegionsOverlap[provinceId];
	}

	std::vector<WorldRegion2> GetRegionOverlaps(const std::vector<int32>& provinceIds)
	{
		std::vector<WorldRegion2> results;
		for (int32 provinceId : provinceIds)
		{
			const std::vector<WorldRegion2>& overlapRegions = _provinceToRegionsOverlap[provinceId];
			for (WorldRegion2 region : overlapRegions) {
				CppUtils::TryAdd(results, region);
			}
		}
		return results;
	}

	/*
	 * Terrain
	 */
	int16 provinceMountainTileCount(int32 provinceId) const { return _provinceMountainTileCount[provinceId]; }
	int16 provinceRiverTileCount(int32 provinceId) const { return _provinceRiverTileCount[provinceId]; }
	int16 provinceOceanTileCount(int32 provinceId) const { return _provinceOceanTileCount[provinceId]; }
	int32 provinceFlatTileCount(int32 provinceId) const { return _provinceFlatTileCount[provinceId]; };
	int32 provinceTileCount(int32 provinceId) const { return _provinceFlatTileCount[provinceId] + 
															_provinceMountainTileCount[provinceId] + 
															_provinceRiverTileCount[provinceId] + 
															_provinceOceanTileCount[provinceId]; };

	int32 totalFlatTiles() { return _totalFlatTiles; }
	

	bool provinceIsMountain(int32 provinceId) const { return provinceMountainTileCount(provinceId) > provinceTileCount(provinceId) / 3; }
	bool provinceIsCoastal(int32 provinceId) const { return provinceOceanTileCount(provinceId) > 3; }
	

	static int32 MinTileCountForGeoresource() { return CoordinateConstants::TilesPerRegion * 3 / 4; }
	
	bool CanPlantMountainGeoresource(int32 provinceId) const {
		int32 tileCount = provinceTileCount(provinceId);
		return tileCount > MinTileCountForGeoresource()  && provinceIsMountain(provinceId) && provinceFlatTileCount(provinceId) > tileCount / 4;
	}

	bool CanPlantFarmGeoresource(int32 provinceId) const {
		return provinceFlatTileCount(provinceId) > provinceFlatTileCount(provinceId) * 3 / 4;
	}

	
public:
	// For display
	//int32 highlightedProvince = -1;
	
	// Debug
	std::vector<bool> displayedProvinceThisTick;
	
private:

	template<typename Func>
	static void FloodBreadthFirst(std::vector<std::vector<WorldTile2x2>>& floodStackToProvinceId, int32 floodLevels, Func tryFlood)
	{
		// Flood X levels
		for (int32 j = 0; j < floodLevels; j++)
		{
			for (int32 provinceId = 0; provinceId < floodStackToProvinceId.size(); provinceId++)
			{
				std::vector<WorldTile2x2> newStack;
				std::vector<WorldTile2x2>& lastStack = floodStackToProvinceId[provinceId];

				if (lastStack.size() > 0)
				{
					for (size_t k = 0; k < lastStack.size(); k++) {
						// Look around the tile to find the tile that has not been filled yet
						WorldTile2x2 lastTile2x2 = lastStack[k];
						tryFlood(WorldTile2x2(lastTile2x2.x + 1, lastTile2x2.y), provinceId, newStack, j);
						tryFlood(WorldTile2x2(lastTile2x2.x - 1, lastTile2x2.y), provinceId, newStack, j);
						tryFlood(WorldTile2x2(lastTile2x2.x, lastTile2x2.y + 1), provinceId, newStack, j);
						tryFlood(WorldTile2x2(lastTile2x2.x, lastTile2x2.y - 1), provinceId, newStack, j);
					}

					floodStackToProvinceId[provinceId] = newStack;
				}
			}
		}
	}

	void FloodRecurse(WorldTile2x2 previousTile2x2, WorldTile2x2 tile2x2)
	{
		int32 tile2x2Id = tile2x2.tile2x2Id();
		int32 localProvinceId = _provinceId2x2[tile2x2Id];
		WorldTile2 tile = tile2x2.worldTile2();

		if (IsValidNonEdgeProvinceId(localProvinceId))
		{
			int32 invalidSides = (abs(_provinceId2x2[WorldTile2x2(tile2x2.x + 1, tile2x2.y).tile2x2Id()]) != localProvinceId) +
				(abs(_provinceId2x2[WorldTile2x2(tile2x2.x - 1, tile2x2.y).tile2x2Id()]) != localProvinceId) +
				(abs(_provinceId2x2[WorldTile2x2(tile2x2.x, tile2x2.y + 1).tile2x2Id()]) != localProvinceId) +
				(abs(_provinceId2x2[WorldTile2x2(tile2x2.x, tile2x2.y - 1).tile2x2Id()]) != localProvinceId);
			bool isEdge = (3 >= invalidSides && invalidSides >= 1);
			
			if (isEdge &&
				!CppUtils::Contains(_provinceEdges1[localProvinceId], tile2x2))
			{
				_provinceEdges1[localProvinceId].push_back(tile2x2Id);
				//_provinceId2x2[tile2x2Id] = localProvinceId; // Negative for edge


				// Loop starting from previous tile going clockwise
				// TODO: Change to lookup
				size_t startShift8 = 0;
				for (size_t k = 0; k < 8; k++) {
					if (tile2x2 + shift8[k] == previousTile2x2) {
						startShift8 = k - 1;
						break;
					}
				}

				for (size_t k = 0; k < 8; k++) {
					FloodRecurse(tile2x2, tile2x2 + shift8[(k + startShift8) % 8]);
				}
			}
		}

	}

	/*
	 * Problems:
	 * - Sample IsInProvince
	 * - World map light region cut decal (also mesh+inset but lower poly?)
	 * - 
	 */

	/*
	 * Usage
	 * - Block choke point with fortress
	 */

	/*
	 * Military
	 * - Shogun feels better than Shogun 2
	 */

	/*
	 * 2x2
	 * - 1/4 less memory
	 * -1/4 faster computation
	 */


	template <typename Func1, typename Func2>
	bool GenerateEdge(size_t& curTile2x2Id, int32& curProvinceId, WorldTile2x2& curTile2x2, 
						std::vector<WorldTile2x2>& edges1, std::vector<WorldTile2x2>& edges2, 
						Func1 isInArea, Func2 tryFloodEdge, bool isProvince = true)
	{
		auto insertTile2x2 = [&](const WorldTile2x2& tile2x2, const WorldTile2x2& tile2x2Shifted)
		{
			edges1.push_back(tile2x2);
			edges2.push_back(tile2x2Shifted);
		};

		
		/*
		 * Use stack so it is depth first search
		 *
		 */
		std::vector<WorldTile2x2> stack;
		std::vector<WorldTile2x2> prevStack;

		stack.push_back(curTile2x2);

		//prevStack.push_back(WorldTile2x2());
		{
			// Calculate the prevTile
			// Loop clockwise.. Previous tile would be when the last loop tile is outside, but the current tile is inside
			bool lastTileIsInside = isInArea(_provinceId2x2[(curTile2x2 + shift8[0]).tile2x2Id()]);
			for (size_t k = 1; k < 9; k++) {
				WorldTile2x2 shiftedTile2x2 = curTile2x2 + shift8[k % 8];
				bool curTileInside = isInArea(_provinceId2x2[shiftedTile2x2.tile2x2Id()]);
				
				if (!isProvince) {
					PUN_LOG("lastTileIsInside:%d curTileInside:%d (%d,%d)", lastTileIsInside, curTileInside, shiftedTile2x2.x, shiftedTile2x2.y);
				}
				
				if (!lastTileIsInside && curTileInside) {
					prevStack.push_back(curTile2x2 + shift8[k % 8]);
					break;
				}
				lastTileIsInside = curTileInside;
			}
		}

		// Failed to create edge (because flood area is 1 tiles?)
		//PUN_CHECK(prevStack.size() == 1);
		if (prevStack.size() != 1) {
			return false;
		}

		WorldTile2x2 startTile2x2 = curTile2x2;
		WorldTile2x2 startPrevTile2x2 = prevStack[0];
		bool passedFirstTile = false;
		
		if (isProvince) {
			_provinceId2x2[curTile2x2Id] = -curProvinceId;
		}
		

		while (stack.size() > 0)
		{
			WorldTile2x2 tile2x2 = stack.back();
			WorldTile2x2 prevTile2x2 = prevStack.back();

			//PUN_LOG("while %d %d", tile2x2.x, tile2x2.y);

			// Passing startTile2x2 two times means we circled back to the starting point
			if (tile2x2 == startTile2x2 && 
				prevTile2x2 == startPrevTile2x2)
			{
				if (passedFirstTile) {
					break;
				} else {
					passedFirstTile = true;
				}
			}

			stack.pop_back();
			prevStack.pop_back();

			int32 tile2x2Id = tile2x2.tile2x2Id();

			if (isProvince) {
				PUN_CHECK(abs(_provinceId2x2[tile2x2Id]) == curProvinceId);
			}

			WorldTile2 tile = tile2x2.worldTile2();


			size_t startShift8 = 0;
			{
				// Loop starting from previous tile going clockwise
				// TODO: Change to lookup
				for (size_t k = 0; k < 8; k++) {
					if (tile2x2 + shift8[k] == prevTile2x2) {
						startShift8 = k;
						break;
					}
				}
			}

			/*
			 * Fill _provinceEdges2 and _provinceEdges2
			 */
			{
				size_t startShift8Local = 0;
				for (size_t k = 0; k < 8; k++) {
					if (tile2x2 + shift8[k] == prevTile2x2) {
						startShift8Local = k;
						break;
					}
				}

				for (size_t k = 8; k-- > 0;) {
					int32 shiftIndex = (k + startShift8Local) % 8;
					if (shiftIndex % 2 == 0) // This leave out the vertex that will make the province edge sharp
					{
						WorldTile2x2 tile2x2Shifted = tile2x2 + shift8[(k + startShift8Local) % 8];
						
						if (!isInArea(_provinceId2x2[tile2x2Shifted.tile2x2Id()])) {
							insertTile2x2(tile2x2, tile2x2Shifted);
						}
					}
				}
			}



			if (isProvince)
			{
				// Expand the _provinceRectAreas
				TileArea& area = _provinceRectAreas[curProvinceId];
				area.minX = std::min(area.minX, tile.x);
				area.minY = std::min(area.minY, tile.y);

				int16 tileXShifted = tile.x + 1; //  +1 is since tile is from tile2x2 mod
				int16 tileYShifted = tile.y + 1;
				area.maxX = std::max(area.maxX, tileXShifted);
				area.maxY = std::max(area.maxY, tileYShifted);

				// Fill _provinceRegionsOverlap
				WorldRegion2 overlapRegion = tile.region();
				CppUtils::TryAdd(_provinceToRegionsOverlap[curProvinceId], overlapRegion);
			}

			// Loop counter-clockwise around starting from prevTile..
			//  When the last loop tile is outside, but the current tile is inside, we found the next tile
			//  TODO: no longer need stack...
			bool lastTileIsInside = isInArea(_provinceId2x2[(tile2x2 + shift8[(0 + startShift8) % 8]).tile2x2Id()]);
			for (size_t k = 8; k-- > 0;) 
			{
				WorldTile2x2 shiftedTile2x2 = tile2x2 + shift8[(k + startShift8) % 8];
				bool curTileInside = isInArea(_provinceId2x2[shiftedTile2x2.tile2x2Id()]);
				//if (!isProvince) PUN_LOG("lastTileIsInside:%d curTileInside:%d (%d,%d)", lastTileIsInside, curTileInside, shiftedTile2x2.x, shiftedTile2x2.y);

				if (!lastTileIsInside && curTileInside) {
					tryFloodEdge(shiftedTile2x2, tile2x2, curProvinceId, stack, prevStack);
					break;
				}
				lastTileIsInside = curTileInside;
			}
		}

		return true;
	}

	/*
	 * 
	 */
	
	/*
	 * Doesn't include _provinceConnection
	 */
	template <typename Func>
	void MergeProvince(int32 oldProvinceId, int32 newProvinceId, Func tryFloodEdge)
	{
		PUN_CHECK(oldProvinceId >= 0);
		PUN_CHECK(newProvinceId >= 0);
		
		// Set the provinceId to provinceIdToMergeTo on _provinceId2x2
		TileArea oldArea = _provinceRectAreas[oldProvinceId];

		//TileArea area(0, 0, GameMapConstants::TilesPerWorldX - 1, GameMapConstants::TilesPerWorldY - 1);

		// Merge Rect Area
		TileArea combinedArea = TileArea::CombineAreas(_provinceRectAreas[newProvinceId], _provinceRectAreas[oldProvinceId]);
		_provinceRectAreas[newProvinceId] = combinedArea;
		_provinceRectAreas[oldProvinceId] = TileArea::Invalid;

		// DEBUG
		int32 changedTileCount = 0;
		int32 totalTilesAfter = 0;
		// DEBUG_END
		
		combinedArea.ExecuteOnArea_WorldTile2x2([&](int16 x2, int16 y2)
		{
			WorldTile2x2 localTile2x2(x2, y2);
			int32 tile2x2Id = localTile2x2.tile2x2Id();
			WorldTile2 tile = localTile2x2.worldTile2();
			
			if (abs(_provinceId2x2[tile2x2Id]) == oldProvinceId)  {
				_provinceId2x2[tile2x2Id] = newProvinceId;

				CppUtils::TryAdd(_regionToProvinceIds[tile.regionId()], newProvinceId);
				changedTileCount += 4;
			}

			// Clear from _regionToProvinceIds
			CppUtils::TryRemove(_regionToProvinceIds[tile.regionId()], oldProvinceId);
		});
		
		int32 debugTileCount = provinceTileCount(oldProvinceId);
		//PUN_LOG("OLD mtn:%d river:%d ocean:%d flat:%d total:%d", _provinceMountainTileCount[oldProvinceId], 
		//	_provinceRiverTileCount[oldProvinceId],
		//	_provinceOceanTileCount[oldProvinceId],
		//	_provinceFlatTileCount[oldProvinceId], provinceTileCount(oldProvinceId));
		//PUN_LOG("NEW mtn:%d river:%d ocean:%d flat:%d total:%d", _provinceMountainTileCount[newProvinceId],
		//	_provinceRiverTileCount[newProvinceId],
		//	_provinceOceanTileCount[newProvinceId],
		//	_provinceFlatTileCount[newProvinceId], provinceTileCount(newProvinceId));
		PUN_CHECK(changedTileCount == debugTileCount);

		// Refresh to mark all tiles positive (GenerateEdge will mark them negative)
		combinedArea.ExecuteOnArea_WorldTile2x2([&](int16 x2, int16 y2)
		{
			WorldTile2x2 localTile2x2(x2, y2);
			int32 tile2x2Id = localTile2x2.tile2x2Id();

			if (abs(_provinceId2x2[tile2x2Id]) == newProvinceId) 
			{
				_provinceId2x2[tile2x2Id] = newProvinceId;
				totalTilesAfter += 4;
			}
		});

		//DEBUG
		//PUN_LOG("changedTileCount:%d totalTilesAfter:%d", changedTileCount, totalTilesAfter);
		PUN_CHECK(totalTilesAfter == (changedTileCount + provinceTileCount(newProvinceId)));

		_provinceCenters[oldProvinceId] = WorldTile2x2();

		// Merge TileCount
		_provinceMountainTileCount[newProvinceId] += _provinceMountainTileCount[oldProvinceId];
		_provinceRiverTileCount[newProvinceId] += _provinceRiverTileCount[oldProvinceId];
		_provinceOceanTileCount[newProvinceId] += _provinceOceanTileCount[oldProvinceId];
		_provinceFlatTileCount[newProvinceId] += _provinceFlatTileCount[oldProvinceId];

		_provinceMountainTileCount[oldProvinceId] = 0;
		_provinceRiverTileCount[oldProvinceId] = 0;
		_provinceOceanTileCount[oldProvinceId] = 0;
		_provinceFlatTileCount[oldProvinceId] = 0;


		// Merge RegionOverlap
		for (WorldRegion2 region : _provinceToRegionsOverlap[oldProvinceId]) {
			CppUtils::TryAdd(_provinceToRegionsOverlap[newProvinceId], region);
		}
		_provinceToRegionsOverlap[oldProvinceId].clear();


		/*
		 * Reconstruct Edges on new province
		 */
		_provinceEdges1[oldProvinceId].clear();
		_provinceEdges2[oldProvinceId].clear();
		_provinceEdges1[newProvinceId].clear();
		_provinceEdges2[newProvinceId].clear();
		
		combinedArea.ExecuteOnArea_WorldTile2([&](WorldTile2 tile)
		{
			WorldTile2x2 curTile2x2(tile);
			size_t curTile2x2Id = curTile2x2.tile2x2Id();
			int32 curProvinceId = _provinceId2x2[curTile2x2Id];

			if (curProvinceId >= 0 &&
				_provinceEdges1[curProvinceId].empty())
			{
				auto insertTile2x2 = [&](const WorldTile2x2& tile2x2, const WorldTile2x2& tile2x2Shifted) {
					_provinceEdges1[curProvinceId].push_back(tile2x2);
					_provinceEdges2[curProvinceId].push_back(tile2x2Shifted);
				};
				
				auto isInArea = [&](int32 provinceRawId) -> bool {
					return abs(provinceRawId) == curProvinceId;
				};
				
				GenerateEdge(curTile2x2Id, curProvinceId, curTile2x2, _provinceEdges1[curProvinceId], _provinceEdges2[curProvinceId], isInArea, tryFloodEdge);
			}
		});

	}

public:
	/*
	 * Serialize
	 */
	void Serialize(FArchive &Ar)
	{
		SerializeVecValue(Ar, _provinceId2x2);
		SerializeVecObj(Ar, _provinceCenters);
		
		SerializeVecVecObj(Ar, _provinceEdges1);
		SerializeVecVecObj(Ar, _provinceEdges2);

		SerializeVecLoop(Ar, _territoryEdges1, [&](std::vector<std::vector<WorldTile2x2>>& vecVecObj) {
			SerializeVecVecObj(Ar, vecVecObj);
		});
		SerializeVecLoop(Ar, _territoryEdges2, [&](std::vector<std::vector<WorldTile2x2>>& vecVecObj) {
			SerializeVecVecObj(Ar, vecVecObj);
		});
		SerializeVecLoop(Ar, _territoryClusterProvinceIds, [&](std::vector<std::vector<int32>>& vecVecValue) {
			SerializeVecVecValue(Ar, vecVecValue);
		});
		//SerializeVecVecObj(Ar, _territoryEdges1);
		//SerializeVecVecObj(Ar, _territoryEdges2);

		SerializeVecVecValue(Ar, _regionToProvinceIds);

		SerializeVecValue(Ar, _provinceMountainTileCount);
		SerializeVecValue(Ar, _provinceRiverTileCount);
		SerializeVecValue(Ar, _provinceOceanTileCount);
		SerializeVecValue(Ar, _provinceFlatTileCount);

		Ar << _totalFlatTiles;

		SerializeVecVecObj(Ar, _provinceConnections);
		SerializeVecObj(Ar, _provinceRectAreas);
		SerializeVecVecObj(Ar, _provinceToRegionsOverlap);
	}
	
private:
	IGameSimulationCore* _simulation = nullptr;
	
	std::vector<int32> _provinceId2x2; // negative represents edge
	std::vector<WorldTile2x2> _provinceCenters;

	std::vector<std::vector<WorldTile2x2>> _provinceEdges1;
	std::vector<std::vector<WorldTile2x2>> _provinceEdges2; // the pair of province edges used to get the mid point for building mesh

	std::vector<std::vector<std::vector<WorldTile2x2>>> _territoryEdges1; // townId->cluster->edges
	std::vector<std::vector<std::vector<WorldTile2x2>>> _territoryEdges2;
	std::vector<std::vector<std::vector<int32>>> _territoryClusterProvinceIds;

	// Used to quickly query what provinces are in particular region
	std::vector<std::vector<int32>> _regionToProvinceIds;

	std::vector<int16> _provinceMountainTileCount;
	std::vector<int16> _provinceRiverTileCount;
	std::vector<int16> _provinceOceanTileCount;
	std::vector<int16> _provinceFlatTileCount;

	int32 _totalFlatTiles = 0;

	std::vector<std::vector<ProvinceConnection>> _provinceConnections;
	std::vector<TileArea> _provinceRectAreas;
	std::vector<std::vector<WorldRegion2>> _provinceToRegionsOverlap;
};
