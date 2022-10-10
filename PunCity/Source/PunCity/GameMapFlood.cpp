// Fill out your copyright notice in the Description page of Project Settings.


#include "GameMapFlood.h"
#include "PunCity/CppUtils.h"
#include "PunCity/PunTimer.h"

using namespace std;

DECLARE_CYCLE_STAT(TEXT("PUN: Flood.IsConnected.Light"), STAT_PunIsConnectedLight, STATGROUP_Game);
DECLARE_CYCLE_STAT(TEXT("PUN: Flood.IsConnected.Deep"), STAT_PunIsConnectedDeep, STATGROUP_Game);
DECLARE_CYCLE_STAT(TEXT("PUN: Flood.flooding"), STAT_PunFlooding, STATGROUP_Game);
DECLARE_CYCLE_STAT(TEXT("PUN: Flood.connectedFlood"), STAT_PunFloodConnectedFloods, STATGROUP_Game);

//! RegionFloodConnections
std::vector<FloodInfo> RegionFloodConnections::connectedFloods(FloodInfo info)
{
	//SCOPE_CYCLE_COUNTER(STAT_PunFloodConnectedFloods);
	// TODO: this is not used anymore??

	check(info.region64Id == _region64Id);
	static std::vector<FloodInfo> connectedFloodsResults;
	connectedFloodsResults.clear();

	for (int i = 0; i < DirectionCount; i++)
	{
		// Check region validity first
		int32 neighborRegion64Id = Region64Neighbor(info.region64Id, static_cast<Direction>(i));
		if (neighborRegion64Id == -1) {
			continue;
		}

		std::vector<FloodConnection>& connections = _directionToConnections[i];
		for (const FloodConnection& connection : connections) {
			if (connection.originId == info.floodId) {
				connectedFloodsResults.push_back(FloodInfo(neighborRegion64Id, connection.neighborId));
			}
		}
	}
	return connectedFloodsResults;
}


//! GameMapFlood

void GameMapFlood::ResetRegionFloodDelayed(int32 region64Id)
{
	CppUtils::TryAdd(_regions64ToReset, region64Id);
}

void GameMapFlood::RefreshRegionConnections(int32 region64Id)
{
	//UE_LOG(LogTemp, Error, TEXT("RefreshRegionConnections (%d,%d)"), region.x, region.y);

	std::vector<int32>& floods = _region64ToFloods[region64Id];
	RegionFloodConnections& regionConnections = _region64ToConnections[region64Id];

	for (int i = 0; i < DirectionCount; i++) {
		Direction direction = static_cast<Direction>(i);
		int32 neighborRegion64Id = Region64Neighbor(region64Id, direction);

		if (neighborRegion64Id != -1) 
		{
			std::vector<int32>& neighborFloods = _region64ToFloods[neighborRegion64Id];
			RegionFloodConnections& neighborConnections = _region64ToConnections[neighborRegion64Id];

			if (direction == Direction::E) {
				regionConnections.ClearConnections(Direction::E);
				neighborConnections.ClearConnections(Direction::W);

				// E (+y), loop top tiles of current region, and bottom tiles of neighbor
				for (int x = 0; x < Region64Size; x++) {
					int32 originFloodId = floods[x + (Region64Size - 1) * Region64Size];
					int32 neighborFloodId = neighborFloods[x];

					// Add connections to both this/neighbor
					if (originFloodId != -1 && neighborFloodId != -1) {
						regionConnections.AddConnection(Direction::E, originFloodId, neighborFloodId);
						neighborConnections.AddConnection(Direction::W, neighborFloodId, originFloodId);
					}
				}
			}
			else if (direction == Direction::W) {
				regionConnections.ClearConnections(Direction::W);
				neighborConnections.ClearConnections(Direction::E);

				// W (-y)
				for (int x = 0; x < Region64Size; x++) {
					int32 originFloodId = floods[x];
					int32 neighborFloodId = neighborFloods[x + (Region64Size - 1) * Region64Size];

					// Add connections to both this/neighbor
					if (originFloodId != -1 && neighborFloodId != -1) {
						regionConnections.AddConnection(Direction::W, originFloodId, neighborFloodId);
						neighborConnections.AddConnection(Direction::E, neighborFloodId, originFloodId);
					}
				}
			}
			else if (direction == Direction::N) {
				regionConnections.ClearConnections(Direction::N);
				neighborConnections.ClearConnections(Direction::S);

				// N (+x)
				for (int y = 0; y < Region64Size; y++) {
					int32 originFloodId = floods[(Region64Size - 1) + y * Region64Size];
					int32 neighborFloodId = neighborFloods[y * Region64Size];

					// Add connections to both this/neighbor
					if (originFloodId != -1 && neighborFloodId != -1) {
						regionConnections.AddConnection(Direction::N, originFloodId, neighborFloodId);
						neighborConnections.AddConnection(Direction::S, neighborFloodId, originFloodId);
					}
				}
			}
			else if (direction == Direction::S) {
				regionConnections.ClearConnections(Direction::S);
				neighborConnections.ClearConnections(Direction::N);

				// S (-x)
				for (int y = 0; y < Region64Size; y++) {
					int32 originFloodId = floods[y * Region64Size];
					int32 neighborFloodId = neighborFloods[(Region64Size - 1) + y * Region64Size];

					// Add connections to both this/neighbor
					if (originFloodId != -1 && neighborFloodId != -1) {
						regionConnections.AddConnection(Direction::S, originFloodId, neighborFloodId);
						neighborConnections.AddConnection(Direction::N, neighborFloodId, originFloodId);
					}
				}
			}
			else {
				UE_DEBUG_BREAK();
			}
		}
	}

	// Check connections...
	//for (int i = 0; i < DirectionCount; i++) {
	//	Direction direction = static_cast<Direction>(i);
	//	int32_t neighborRegion64Id = Region64Neighbor(region64Id, direction);

	//	if (neighborRegion64Id != -1) {
	//		RegionFloodConnections& neighborConnections = _region64ToConnections[neighborRegion64Id];

	//		size_t selfSize = regionConnections.directionToConnections()[(int)direction].size();
	//		size_t neighborSize = neighborConnections.directionToConnections()[(int)OppositeDirection(direction)].size();
	//		check(selfSize == neighborSize);
	//	}
	//}
}

void GameMapFlood::Tick()
{
	//check(_pathAI->walkableGridSize() > 0);

	if (_regions64ToReset.size() > 0)
	{
		for (int i = 0; i < _regions64ToReset.size(); i++) {
			//SCOPE_TIMER("FillRegion");
			FillRegion(_regions64ToReset[i]);
		}

		for (int i = 0; i < _regions64ToReset.size(); i++) {
			//SCOPE_TIMER("RefreshRegionConnections");
			RefreshRegionConnections(_regions64ToReset[i]);
		}

		for (int i = 0; i < _regions64ToReset.size(); i++) {
			int32 region64Id = _regions64ToReset[i];
			WorldRegion2 region00(Region64X(region64Id) * 2, Region64Y(region64Id) * 2);
			_simulation->OnRefreshFloodGrid(region00);
			_simulation->OnRefreshFloodGrid(WorldRegion2(region00.x + 1, region00.y));
			_simulation->OnRefreshFloodGrid(WorldRegion2(region00.x, region00.y + 1));
			_simulation->OnRefreshFloodGrid(WorldRegion2(region00.x + 1, region00.y + 1));
		}

		_regions64ToReset.clear();
	}
}

void GameMapFlood::FillRegion(int32 region64Id)
{
	SCOPE_CYCLE_COUNTER(STAT_PunFlooding);

	std::vector<int32>& floods = _region64ToFloods[region64Id];

	int32 minX = Region64XToMinTileX(region64Id);
	int32 minY = Region64YToMinTileY(region64Id);


	// vector to store how tiles are connected
	// connectedIndices[3] = 0 ... means that 3 is connected to 0
	static std::vector<std::vector<int32>> connectedIds;
	connectedIds.clear();

	for (int32 y = 0; y < Region64Size; y++)
	{
		int32 yShift = y * Region64Size;

		// Other tiles, check left tile first.
		// If both tiles has indices connect the indices
		for (int32 x = 0; x < Region64Size; x++)
		{
			//if (!GameMap::PathAI->isWalkable(minX + x, minY + y)) {
			if (!_pathAI->isWalkable(minX + x, minY + y)) {
				floods[x + yShift] = -1;
				continue;
			}

			int32 leftIndex = x > 0 ? floods[(x - 1) + yShift] : -1;
			int32 topIndex = y > 0 ? floods[x + (yShift - Region64Size)] : -1;

			// TODO: Diagonal check: left-top and right-top?
			

			if (leftIndex != -1) {
				floods[x + yShift] = leftIndex;
			}
			else if (topIndex != -1) {
				floods[x + yShift] = topIndex;
			}
			else {
				// New Id
				floods[x + yShift] = connectedIds.size();
				connectedIds.push_back(std::vector<int32_t>());
			}

			// Connect left/topIndex in the case that they not the same
			if (leftIndex != -1 && topIndex != -1 && leftIndex != topIndex)
			{
				auto& ids1 = connectedIds[leftIndex];
				if (std::find(ids1.begin(), ids1.end(), topIndex) == ids1.end()) {
					ids1.push_back(topIndex);
				}
				auto& ids2 = connectedIds[topIndex];
				if (std::find(ids2.begin(), ids2.end(), leftIndex) == ids2.end()) {
					ids2.push_back(leftIndex);
				}
			}
		}
	}

	// Find the root floodId for all ids
	// as we go further up floodIds, tie them together using connection information
	// Lets say floodId 3 connects to 0,1,2 ... we find the lowest id (0) and set rootId of 0,1,2,3 to 0
	static std::vector<int32> rootIds;
	static std::vector<bool> visited;
	rootIds.resize(connectedIds.size());
	visited.resize(connectedIds.size(), false);

	for (int i = 0; i < rootIds.size(); i++) {
		fill(visited.begin(), visited.end(), false);
		int32 rootId = FindRootId(i, connectedIds, visited);

		// Also need to change children's rootIds as well
		// Make sure all connected ids have rootIndex set...
		fill(visited.begin(), visited.end(), false);
		SetFloodRootIds(rootId, i, rootIds, connectedIds, visited);
	}

	// Set rootIds to the flood grid
	for (int32 y = 0; y < Region64Size; y++) {
		for (int32 x = 0; x < Region64Size; x++) {
			int32 floodId = floods[x + y * Region64Size];
			if (floodId != -1) {
				floods[x + y * Region64Size] = rootIds[floodId];
			}
		}
	}
}

int32 GameMapFlood::FindRootId(int32 currentId, std::vector<std::vector<int32>>& connectedIds, std::vector<bool>& visited)
{
	if (!visited[currentId]) {
		visited[currentId] = true;
		int32 rootId = currentId; //  If no connection, this is the root

		// Find lowest connected id
		auto& ids = connectedIds[currentId];
		for (int i = 0; i < ids.size(); i++) {
			int32 possibleRootId = FindRootId(ids[i], connectedIds, visited);
			if (possibleRootId < rootId) {
				rootId = possibleRootId;
			}
		}
		return rootId;
	}
	return currentId;
}

void GameMapFlood::SetFloodRootIds(int32 rootId, int32 currentId, std::vector<int32>& rootIds, 
									std::vector<std::vector<int32>>& connectedIds, std::vector<bool>& visited)
{
	if (!visited[currentId]) {
		visited[currentId] = true;
		rootIds[currentId] = rootId; // Set self

		auto& ids = connectedIds[currentId];
		for (int i = 0; i < ids.size(); i++) {
			SetFloodRootIds(rootId, ids[i], rootIds, connectedIds, visited); // Set children
		}
	}
}

bool GameMapFlood::IsConnected(WorldTile2 start, WorldTile2 end, int maxRegionDistance)
{
	int32 startRegion64Id;
	int32 endRegion64Id;
	int32 startFloodId;
	int32 endFloodId;
	{
		//SCOPE_CYCLE_COUNTER(STAT_PunIsConnectedLight); // Note: Turning on affect overall speed

		check(start.isValid());
		check(end.isValid());

		startRegion64Id = TileToRegion64Id(start);
		endRegion64Id = TileToRegion64Id(end);

		startFloodId = _region64ToFloods[startRegion64Id][TileToLocal64Id(start)];
		endFloodId = _region64ToFloods[endRegion64Id][TileToLocal64Id(end)];
		if (startFloodId == -1 || endFloodId == -1) {
			return false;
		}
	}
	
	return IsConnected(FloodInfo(startRegion64Id, startFloodId),
		FloodInfo(endRegion64Id, endFloodId), maxRegionDistance);
}

bool GameMapFlood::IsConnected(FloodInfo startFlood, FloodInfo endFlood, int maxRegionDistance)
{
	// Fast track special case...
	if (startFlood == endFlood) {
		return true;
	}

	SCOPE_CYCLE_COUNTER(STAT_PunIsConnectedDeep);

	static std::vector<FloodInfo> queue;
	static std::vector<int8> regionDist; // ManDistance to end point
	queue.clear();
	regionDist.clear();

	static std::vector<FloodInfo> visited;
	visited.clear();

	// Need arrange queue so that closest element is close to findIndexToAdd
	//const int32 endRegion64Id = endFlood.region64Id;
	//auto addToQueue = [&](FloodInfo& connectedInfo)
	//{
	//	int32 manDist = Region64ManDistance(connectedInfo.region64Id, endRegion64Id);
	//	for (size_t i = 0; i < queue.size(); i++) {
	//		if (manDist <= regionDist[i]) {
	//			queue.insert(queue.begin() + i, connectedInfo);
	//			regionDist.insert(regionDist.begin() + i, manDist);
	//			return;
	//		}
	//	}

	//	// most manDist is whole queue, push_back
	//	queue.push_back(connectedInfo);
	//	regionDist.push_back(manDist);
	//};

	queue.push_back(startFlood);
	regionDist.push_back(0);

	//int32 loopCount = 0;
	//const int32 maxLoopCount = maxRegionDistance * 5;

	int32 loop = 0;
	while (!queue.empty()) 
	{
		// Get current FloodInfo
		FloodInfo info = queue[0];
		int32 regionDistance = regionDist[0];

		// If at the end, return
		if (info == endFlood) {
			return true;
		}

		queue.erase(queue.begin());
		regionDist.erase(regionDist.begin());

		visited.push_back(info);

		if (regionDistance >= maxRegionDistance) {
			continue;
		}

		// Expand and queue more FloodInfo
		RegionFloodConnections& regionConnections = _region64ToConnections[info.region64Id];
		auto& directionToConnections = regionConnections.directionToConnections();
		//std::vector<FloodInfo> connectedInfos = regionConnections.connectedFloods(info);

		// NOTE!!!!
		// - Greedy search is actually slower... tested...

		{
			//SCOPE_CYCLE_COUNTER(STAT_PunFloodConnectedFloods); // Note: this is in range of 10k at x5 speed.. affect overall performance

			check(info.region64Id == regionConnections.region64Id());

			for (int i = 0; i < DirectionCount; i++)
			{
				// Check region validity first
				int32_t neighborRegion64Id = Region64Neighbor(info.region64Id, static_cast<Direction>(i));
				if (neighborRegion64Id == -1) {
					continue;
				}

				const std::vector<FloodConnection>& connections = directionToConnections[i];
				for (const FloodConnection& connection : connections) {
					if (connection.originId == info.floodId) {
						FloodInfo connectedInfo(neighborRegion64Id, connection.neighborId);

						if (find(visited.begin(), visited.end(), connectedInfo) == visited.end()) {
							queue.push_back(connectedInfo);
							regionDist.push_back(regionDistance + 1);

							//addToQueue(connectedInfo);
						}
					}
				}
			}
		}

		// SLower too??
		//// Queue only unvisited node
		//for (int i = 0; i < connectedInfos.size(); i++) {
		//	if (find(visited.begin(), visited.end(), connectedInfos[i]) == visited.end()) {
		//		queue.push_back(connectedInfos[i]);
		//		regionDist.push_back(regionDistance + 1);
		//	}
		//}

		loop++;
		if (loop > 100000) {
			UE_DEBUG_BREAK();
			break;
		}
	}

	return false;
}

std::vector<PathFindingFloodInfo> GameMapFlood::FindPath_CheckFloodRegionHelper(WorldTile2 start, WorldTile2 end, int maxRegionDistance) const
{
	int32 startRegion64Id;
	int32 endRegion64Id;
	int32 startFloodId;
	int32 endFloodId;
	{
		//SCOPE_CYCLE_COUNTER(STAT_PunIsConnectedLight); // Note: Turning on affect overall speed

		check(start.isValid());
		check(end.isValid());

		startRegion64Id = TileToRegion64Id(start);
		endRegion64Id = TileToRegion64Id(end);

		startFloodId = _region64ToFloods[startRegion64Id][TileToLocal64Id(start)];
		endFloodId = _region64ToFloods[endRegion64Id][TileToLocal64Id(end)];
		if (startFloodId == -1 || endFloodId == -1) {
			return {};
		}
	}

	return FindPath_FloodRegionHelper(FloodInfo(startRegion64Id, startFloodId), FloodInfo(endRegion64Id, endFloodId), maxRegionDistance);
}


std::vector<WorldTile2> GameMapFlood::FindPath_FloodRegionHelper(WorldTile2 start, WorldTile2 end, int maxRegionDistance) const
{
	int32 startRegion64Id;
	int32 endRegion64Id;
	int32 startFloodId;
	int32 endFloodId;
	{
		//SCOPE_CYCLE_COUNTER(STAT_PunIsConnectedLight); // Note: Turning on affect overall speed

		check(start.isValid());
		check(end.isValid());

		startRegion64Id = TileToRegion64Id(start);
		endRegion64Id = TileToRegion64Id(end);

		startFloodId = _region64ToFloods[startRegion64Id][TileToLocal64Id(start)];
		endFloodId = _region64ToFloods[endRegion64Id][TileToLocal64Id(end)];
		if (startFloodId == -1 || endFloodId == -1) {
			return {};
		}
	}

	std::vector<PathFindingFloodInfo> infos = FindPath_FloodRegionHelper(FloodInfo(startRegion64Id, startFloodId), FloodInfo(endRegion64Id, endFloodId), maxRegionDistance);

	std::vector<WorldTile2> rawPath;

	auto isValidConnectingTile = [&](const WorldTile2& currentTile, const FloodInfo& currentFloodInfo, const FloodInfo& lastFloodInfo)
	{
		if (GetFloodId(currentTile) != currentFloodInfo.floodId) {
			return false;
		}

		// TODO Optimize checking only neighbor in direction
		return GetFloodId(currentTile + WorldTile2(-1, -1)) == lastFloodInfo.floodId ||
			GetFloodId(currentTile + WorldTile2(-1, 0)) == lastFloodInfo.floodId ||
			GetFloodId(currentTile + WorldTile2(-1, 1)) == lastFloodInfo.floodId ||
			GetFloodId(currentTile + WorldTile2(0, -1)) == lastFloodInfo.floodId ||
			GetFloodId(currentTile + WorldTile2(0, 1)) == lastFloodInfo.floodId ||
			GetFloodId(currentTile + WorldTile2(1, -1)) == lastFloodInfo.floodId ||
			GetFloodId(currentTile + WorldTile2(1, 0)) == lastFloodInfo.floodId ||
			GetFloodId(currentTile + WorldTile2(1, 1)) == lastFloodInfo.floodId;
	};

	/**
	 * Calculate
	 */
	auto checkDistance1 = [&](int32 index, const WorldTile2& currentTile, const FloodInfo& currentFloodInfo, const FloodInfo& lastFloodInfo, WorldTile2& nearestTile, int32& nearestDistance)
	{
		if (isValidConnectingTile(currentTile, currentFloodInfo, lastFloodInfo))
		{
			int32 distanceToStart = WorldTile2::Distance(currentTile, start);
			if (nearestDistance > distanceToStart) {
				nearestDistance = distanceToStart;
				nearestTile = currentTile;
			}
			int32 distanceToEnd = WorldTile2::Distance(currentTile, end);
			if (nearestDistance > distanceToEnd) {
				nearestDistance = distanceToEnd;
				nearestTile = currentTile;
			}
		}
	};

	auto addNearestTile1 = [&](const WorldTile2& nearestTile)
	{
		rawPath.push_back(nearestTile);
	};

	LoopThrough(infos, checkDistance1, addNearestTile1);

	rawPath.insert(rawPath.begin(), start);
	rawPath.push_back(end);

	//for (int32 i = rawPath.size(); i-- > 1;) {
	//	WorldTile2 tile1 = rawPath[i - 1];
	//	WorldTile2 tile2 = rawPath[i];
	//	_simulation->DrawLine(tile1.worldAtom2(), FVector(0, 0, 10), tile2.worldAtom2(), FVector(0, 0, 15), FLinearColor::Gray, 1.0f, 10000);
	//}

	check(rawPath.size() == infos.size() + 2);

	/**
	 * Refine
	 */
	std::vector<WorldTile2> refinedPath;
	
	auto checkDistance2 = [&](int32 infosIndex, const WorldTile2& currentTile, const FloodInfo& currentFloodInfo, const FloodInfo& lastFloodInfo, WorldTile2& nearestTile, int32& nearestDistance)
	{
		if (isValidConnectingTile(currentTile, currentFloodInfo, lastFloodInfo))
		{
			// index is reverse for path vs infos
			int32 index = infos.size() - 1 - infosIndex;
			
			WorldTile2 previousTile = rawPath[index];
			WorldTile2 nextTile = rawPath[index + 2];
			
			int32 distance = WorldTile2::Distance(previousTile, currentTile) + WorldTile2::Distance(currentTile, nextTile);
			if (nearestDistance > distance) {
				nearestDistance = distance;
				nearestTile = currentTile;
			}
		}
	};

	auto addNearestTile2 = [&](const WorldTile2& nearestTile)
	{
		refinedPath.push_back(nearestTile);
	};

	LoopThrough(infos, checkDistance2, addNearestTile2);

	refinedPath.insert(refinedPath.begin(), start);
	refinedPath.push_back(end);
	
	// Note: high level path is "start first"
	// low level path is "end first"
	return refinedPath;
}


std::vector<PathFindingFloodInfo> GameMapFlood::FindPath_FloodRegionHelper(FloodInfo startFlood, FloodInfo endFlood, int32 maxRegionDistance) const
{
	//SCOPE_TIMER("FindPath_FloodRegionHelper2");
	
	// Fast track special case...
	if (startFlood == endFlood) {
		return {};
	}

	static std::vector<PathFindingFloodInfo> queue;
	queue.clear();

	static TMap<int64, PathFindingFloodInfo> visited;
	visited.Empty();

	
	queue.push_back({ startFlood, FloodInfo(-1, -1), 0});
	

	int32 loop = 0;
	while (!queue.empty())
	{
		// Get current FloodInfo
		PathFindingFloodInfo info = queue[0];
		queue.erase(queue.begin());

		// If at the end, return
		if (info.currentFloodInfo == endFlood)
		{
			// reconstruct path
			PathFindingFloodInfo currentInfo = info;
			
			std::vector<PathFindingFloodInfo> path;
			path.push_back(currentInfo);

			while (currentInfo.lastFloodInfo != startFlood &&
				currentInfo.lastFloodInfo.region64Id != -1)
			{
				currentInfo = visited[currentInfo.lastFloodInfo.GetHash()];
				path.push_back(currentInfo);
			}
			
			return path;
		}

		visited.Add(info.currentFloodInfo.GetHash(), info);

		if (info.regionDistance >= maxRegionDistance) {
			continue;
		}

		// Expand and queue more FloodInfo
		const RegionFloodConnections& regionConnections = _region64ToConnections[info.currentFloodInfo.region64Id];
		auto& directionToConnections = regionConnections.directionToConnections();

		{
			check(info.currentFloodInfo.region64Id == regionConnections.region64Id());

			for (int i = 0; i < DirectionCount; i++)
			{
				// Check region validity first
				int32_t neighborRegion64Id = Region64Neighbor(info.currentFloodInfo.region64Id, static_cast<Direction>(i));
				if (neighborRegion64Id == -1) {
					continue;
				}

				const std::vector<FloodConnection>& connections = directionToConnections[i];
				for (const FloodConnection& connection : connections) {
					if (connection.originId == info.currentFloodInfo.floodId) 
					{
						if (!visited.Contains(connection.neighborId + Region64Tiles * neighborRegion64Id))
						{
							queue.push_back({ FloodInfo(neighborRegion64Id, connection.neighborId), info.currentFloodInfo, info.regionDistance + 1 });
						}
					}
				}
			}
		}

		loop++;
		if (loop > 100000) {
			UE_DEBUG_BREAK();
			break;
		}
	}

	
	//for (const auto& info : visited) {
	//	if (info.Value.lastFloodInfo.region64Id != -1)
	//	{
	//		WorldTile2 start = Region64IdToTile(info.Value.currentFloodInfo.region64Id);
	//		WorldTile2 last = Region64IdToTile(info.Value.lastFloodInfo.region64Id);
	//		start += WorldTile2(32, 32);
	//		last += WorldTile2(32, 32);
	//		_simulation->DrawLine(start, FVector::ZeroVector, last, FVector(0, 0, 3), FLinearColor::Red, 1.0f, 10000);
	//	}
	//}

	return {};
}