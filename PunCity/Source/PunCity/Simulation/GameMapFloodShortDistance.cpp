//// Pun Dumnernchanvanit's
//
//
//#include "GameMapFloodShortDistance.h"
//
//DECLARE_CYCLE_STAT(TEXT("PUN: Flood.IsConnectedShort"), STAT_PunIsConnectedShort, STATGROUP_Game);
//DECLARE_CYCLE_STAT(TEXT("PUN: Flood.floodingShort"), STAT_PunFloodingShort, STATGROUP_Game);
//
//using namespace std;
//
//
//int32 GameMapFloodShortDistance::FindRootId(int32 currentId, std::vector<std::vector<int32>>& connectedIds, std::vector<bool>& visited)
//{
//	if (!visited[currentId]) {
//		visited[currentId] = true;
//		int32 rootId = currentId; //  If no connection, this is the root
//
//		// Find lowest connected id
//		auto& ids = connectedIds[currentId];
//		for (int i = 0; i < ids.size(); i++) {
//			int32 possibleRootId = FindRootId(ids[i], connectedIds, visited);
//			if (possibleRootId < rootId) {
//				rootId = possibleRootId;
//			}
//		}
//		return rootId;
//	}
//	return currentId;
//}
//
//void GameMapFloodShortDistance::SetFloodRootIds(int32 rootId, int32 currentId, std::vector<int32>& rootIds,
//												std::vector<std::vector<int32>>& connectedIds, std::vector<bool>& visited)
//{
//	if (!visited[currentId]) {
//		visited[currentId] = true;
//		rootIds[currentId] = rootId; // Set self
//
//		auto& ids = connectedIds[currentId];
//		for (int i = 0; i < ids.size(); i++) {
//			SetFloodRootIds(rootId, ids[i], rootIds, connectedIds, visited); // Set children
//		}
//	}
//}
//
//
//void GameMapFloodShortDistance::FillRegion(int32 region64Id)
//{
//	SCOPE_CYCLE_COUNTER(STAT_PunFloodingShort);
//
//	int32 minX = FloodChunkIdToMinTileX(region64Id);
//	int32 minY = FloodChunkIdToMinTileY(region64Id);
//
//	std::vector<int32>& floods = _region64ToFloods[region64Id];
//
//	// vector to store how tiles are connected
//	// connectedIndices[3] = 0 ... means that 3 is connected to 0
//	static std::vector<std::vector<int32>> connectedIds;
//	connectedIds.clear();
//
//	for (int32 y = 0; y < FloodChunkSize; y++)
//	{
//		int32 yShift = y * FloodChunkSize;
//
//		// Other tiles, check left tile first.
//		// If both tiles has indices connect the indices
//		for (int32 x = 0; x < FloodChunkSize; x++)
//		{
//			// TODO: optimize this out?
//			if (!WorldTile2(minX + x, minY + y).isValid()) {
//				floods[x + yShift] = -1;
//				continue;
//			}
//			
//			if (!_pathAI->isWalkable(minX + x, minY + y)) {
//				floods[x + yShift] = -1;
//				continue;
//			}
//
//			int32 leftIndex = x > 0 ? floods[(x - 1) + yShift] : -1;
//			int32 topIndex = y > 0 ? floods[x + (yShift - FloodChunkSize)] : -1;
//
//			if (leftIndex != -1) {
//				floods[x + yShift] = leftIndex;
//			}
//			else if (topIndex != -1) {
//				floods[x + yShift] = topIndex;
//			}
//			else {
//				// New Id
//				floods[x + yShift] = connectedIds.size();
//				connectedIds.push_back(std::vector<int32>());
//			}
//
//			// Connect left/topIndex in the case that they not the same
//			if (leftIndex != -1 && topIndex != -1 && leftIndex != topIndex)
//			{
//				auto& ids1 = connectedIds[leftIndex];
//				if (std::find(ids1.begin(), ids1.end(), topIndex) == ids1.end()) {
//					ids1.push_back(topIndex);
//				}
//				auto& ids2 = connectedIds[topIndex];
//				if (std::find(ids2.begin(), ids2.end(), leftIndex) == ids2.end()) {
//					ids2.push_back(leftIndex);
//				}
//			}
//		}
//	}
//
//	// Find the root floodId for all ids
//	// as we go further up floodIds, tie them together using connection information
//	// Lets say floodId 3 connects to 0,1,2 ... we find the lowest id (0) and set rootId of 0,1,2,3 to 0
//	static std::vector<int32> rootIds;
//	static std::vector<bool> visited;
//	rootIds.resize(connectedIds.size());
//	visited.resize(connectedIds.size(), false);
//
//	for (int i = 0; i < rootIds.size(); i++) {
//		fill(visited.begin(), visited.end(), false);
//		int32 rootId = FindRootId(i, connectedIds, visited);
//
//		// Also need to change children's rootIds as well
//		// Make sure all connected ids have rootIndex set...
//		fill(visited.begin(), visited.end(), false);
//		SetFloodRootIds(rootId, i, rootIds, connectedIds, visited);
//	}
//
//	// Set rootIds to the flood grid
//	for (int32 y = 0; y < Region64Size; y++) {
//		for (int32 x = 0; x < Region64Size; x++) {
//			int32 floodId = floods[x + y * Region64Size];
//			if (floodId != -1) {
//				floods[x + y * Region64Size] = rootIds[floodId];
//			}
//		}
//	}
//}