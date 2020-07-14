//// Pun Dumnernchanvanit's
//
//#pragma once
//
//#include "PunCity/Simulation/GameSimulationInfo.h"
//#include "PunCity/PunTimer.h"
//
///*
// * Hard-code 64x64 flood region for speed
// */
//static const int32 Region64Size = 64;
//static const int32 Region64SizeShift = 6; // 2^6 is 64
//static const int32 Region64SizeMask = 0b111111; // Note that 8 shift (half of 16) is not used since it will be harder to read values in debugger...
//
//static const int32 Region64Tiles = Region64Size * Region64Size;
//static const int32 Region64PerWorldX = 64; // 128 for normal 32x32....  64 for this.... 64x64
//static const int32 Region64PerWorldXShift = 6;
//static const int32 Region64PerWorldXMask = 0b111111;
//
//static const int32 Region64PerWorldY = 128;
//static const int32 Region64Total = Region64PerWorldX * Region64PerWorldY;
//
//static int32 TileToRegion64Id(WorldTile2 tile) {
//	return (tile.x >> Region64SizeShift) | ((tile.y >> Region64SizeShift) << Region64PerWorldXShift);
//}
//static int32 TileToLocal64Id(WorldTile2 tile) {
//	return (tile.x & Region64SizeMask) | ((tile.y & Region64SizeMask) << Region64SizeShift);
//}
//
//static int32 Region64X(int32 region64Id) {
//	return region64Id & Region64PerWorldXMask;
//}
//static int32 Region64Y(int32 region64Id) {
//	return region64Id >> Region64PerWorldXShift;
//}
//
//static int32 Region64XToMinTileX(int32 region64Id) {
//	return (region64Id & Region64PerWorldXMask) << Region64SizeShift; // Extract X, then shift by Region64SizeShift (or multiply by 64) to get the minX 
//}
//static int32 Region64YToMinTileY(int32 region64Id) {
//	return (region64Id >> Region64PerWorldXShift) << Region64SizeShift;
//}
//
//static int32 Region64ManDistance(int32 region64Id1, int32 region64Id2)
//{
//	return abs(Region64X(region64Id1) + Region64X(region64Id2)) +
//		abs(Region64Y(region64Id1) + Region64Y(region64Id2));
//}
//
//static int32 Region64Neighbor(int32 region64Id, Direction direction)
//{
//	int32 neighborRegion64Id = -1;
//	switch (direction) {
//	case Direction::N: {
//		if ((region64Id & Region64PerWorldXMask) < Region64PerWorldX - 1) {
//			neighborRegion64Id = region64Id + 1;
//		}
//		break;
//	}
//	case Direction::S: {
//		if ((region64Id & Region64PerWorldXMask) > 0) {
//			neighborRegion64Id = region64Id - 1;
//		}
//		break;
//	}
//	case Direction::E: {
//		if ((region64Id >> Region64PerWorldXShift) < Region64PerWorldY - 1) {
//			neighborRegion64Id = region64Id + Region64PerWorldX;
//		}
//		break;
//	}
//	case Direction::W: {
//		if ((region64Id >> Region64PerWorldXShift) > 0) {
//			neighborRegion64Id = region64Id - Region64PerWorldX;
//		}
//		break;
//	}
//	}
//	return neighborRegion64Id;
//}
//
///*
// * Flood extends to 2 regions (64 tiles) outside of 64x64 grid
// */
//static const int32 FloodChunkSize = 256;
//static const int32 FloodChunkSizeShift = 8; // 2^8 is 256
//static const int32 FloodChunkSizeMask = 0b11111111; // Note that 8 shift (half of 16) is not used since it will be harder to read values in debugger...
//
//static const int32 FloodChunkTiles = FloodChunkSize * FloodChunkSize;
//
//static int32 FloodChunkIdToMinTileX(int32 floodChunkId)
//{
//	int32 minX = (floodChunkId & Region64PerWorldXMask) << Region64SizeShift; // Extract X, then shift by Region64SizeShift (or multiply by 64) to get the minX
//	return minX - (32 * 3); // shift back 32*3 more tiles to get to the actual min
//}
//static int32 FloodChunkIdToMinTileY(int32 floodChunkId)
//{
//	int32 minY = (floodChunkId >> Region64PerWorldXShift) << Region64SizeShift;
//	return minY - (32 * 3);
//}
//
///**
// * 
// */
//class PROTOTYPECITY_API GameMapFloodShortDistance
//{
//public:
//	void Init(PunAStar128x256* pathAI, bool isLoadingFromFile)
//	{
//		SCOPE_TIMER("FloodShortDistance Init");
//		
//		_pathAI = pathAI;
//
//		_region64ToFloods.resize(Region64Total);
//
//		if (isLoadingFromFile) {
//			return;
//		}
//
//		//for (int32 i = 0; i < Region64Total; i++) {
//		//	_region64ToFloods[i].resize(FloodChunkTiles);
//		//	FillRegion(i);
//		//}
//	}
//
//	void Tick() {}
//
//	void FillRegion(int32 region64Id);
//
//	void Serialize(FArchive &Ar)
//	{
//		SerializeVecVecValue(Ar, _region64ToFloods);
//		SerializeVecValue(Ar, _regions64ToReset);
//	}
//	
//private:
//	static int32 FindRootId(int32 currentId, std::vector<std::vector<int32>>& connectedIds, std::vector<bool>& visited);
//	static void SetFloodRootIds(int32 rootId, int32 currentId, std::vector<int32>& rootIds, std::vector<std::vector<int32>>& connectedIds, std::vector<bool>& visited);
//	
//private:
//	PunAStar128x256* _pathAI = nullptr;
//
//	std::vector<std::vector<int32>> _region64ToFloods;
//	std::vector<int32> _regions64ToReset;
//};
