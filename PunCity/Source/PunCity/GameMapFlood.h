// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "UnrealEngine.h"
#include "PunCity/Simulation/GameSimulationInfo.h"
#include "PunCity/Simulation/GameMapFloodShortDistance.h"
#include "PunCity/DisplaySystem/PunTerrainGenerator.h"
#include <array>

/*
 * Hard-code 64x64 flood region for speed
 *
 * Moving to a new file shouldn't affect speed????
 */
static const int32 Region64Size = 64;
static const int32 Region64SizeShift = 6; // 2^6 is 64
static const int32 Region64SizeMask = 0b111111; // Note that 8 shift (half of 16) is not used since it will be harder to read values in debugger...

static const int32 Region64Tiles = Region64Size * Region64Size;
static const int32 Region64PerWorldX = 64; // 128 for normal 32x32....  64 for this.... 64x64
static const int32 Region64PerWorldXShift = 6;
static const int32 Region64PerWorldXMask = 0b111111;

static const int32 Region64PerWorldY = 128;
static const int32 Region64Total = Region64PerWorldX * Region64PerWorldY;

static int32 TileToRegion64Id(WorldTile2 tile) {
	return (tile.x >> Region64SizeShift) | ((tile.y >> Region64SizeShift) << Region64PerWorldXShift);
}
static int32 TileToLocal64Id(WorldTile2 tile) {
	return (tile.x & Region64SizeMask) | ((tile.y & Region64SizeMask) << Region64SizeShift);
}

static int32 Region64X(int32 region64Id) {
	return region64Id & Region64PerWorldXMask;
}
static int32 Region64Y(int32 region64Id) {
	return region64Id >> Region64PerWorldXShift;
}

static int32 Region64XToMinTileX(int32 region64Id) {
	return (region64Id & Region64PerWorldXMask) << Region64SizeShift; // Extract X, then shift by Region64SizeShift (or multiply by 64) to get the minX 
}
static int32 Region64YToMinTileY(int32 region64Id) {
	return (region64Id >> Region64PerWorldXShift) << Region64SizeShift;
}

static int32 Region64ManDistance(int32 region64Id1, int32 region64Id2)
{
	return abs(Region64X(region64Id1) + Region64X(region64Id2)) +
		abs(Region64Y(region64Id1) + Region64Y(region64Id2));
}

static int32 Region64Neighbor(int32 region64Id, Direction direction)
{
	int32 neighborRegion64Id = -1;
	switch (direction) {
	case Direction::N: {
		if ((region64Id & Region64PerWorldXMask) < Region64PerWorldX - 1) {
			neighborRegion64Id = region64Id + 1;
		}
		break;
	}
	case Direction::S: {
		if ((region64Id & Region64PerWorldXMask) > 0) {
			neighborRegion64Id = region64Id - 1;
		}
		break;
	}
	case Direction::E: {
		if ((region64Id >> Region64PerWorldXShift) < Region64PerWorldY - 1) {
			neighborRegion64Id = region64Id + Region64PerWorldX;
		}
		break;
	}
	case Direction::W: {
		if ((region64Id >> Region64PerWorldXShift) > 0) {
			neighborRegion64Id = region64Id - Region64PerWorldX;
		}
		break;
	}
	}
	return neighborRegion64Id;
}

struct FloodInfo
{
	int32 region64Id;
	int32 floodId;
	
	FloodInfo(int32_t region64Id, int32_t floodId)
		: region64Id(region64Id), floodId(floodId) {}

	bool operator==(const FloodInfo& a) const {
		return a.region64Id == region64Id && a.floodId == floodId;
	}

	//! Serialize
	FArchive& operator>>(FArchive &Ar) {
		Ar << region64Id << floodId;
		return Ar;
	}
};

struct FloodConnection
{
	int32 originId;
	int32 neighborId;

	FloodConnection() : originId(0), neighborId(0) {}
	
	FloodConnection(int32_t originId, int32_t neighborId)
		: originId(originId), neighborId(neighborId) {}

	bool operator==(const FloodConnection& a) const {
		return originId == a.originId && neighborId == a.neighborId;
	}

	//! Serialize
	FArchive& operator>>(FArchive &Ar) {
		Ar << originId << neighborId;
		return Ar;
	}
};

class RegionFloodConnections
{
public:
	void Init(int32_t region64Id) {
		_region64Id = region64Id;
		//UE_LOG(LogTemp, Error, TEXT("Init RegionFloodConnections (%d,%d)"), _region.x, _region.y);
	}

	void AddConnection(Direction direction, int32_t originFloodId, int32_t neighborFloodId) 
	{
		check(originFloodId != -1);
		check(neighborFloodId != -1);
		FloodConnection newConnection(originFloodId, neighborFloodId);

		auto& connections = _directionToConnections[static_cast<int32_t>(direction)];

		//if (direction == Direction::E || direction == Direction::W) {
		//	UE_LOG(LogTemp, Error, TEXT("AddConnection1 (%d,%d) origin:%d neighbor:%d"), _region.x, _region.y, originFloodId, neighborFloodId);
		//}

		// If connection is already listed, don't add more
		for (int i = 0; i < connections.size(); i++) {
			if (connections[i] == newConnection) {
				return;
			}
		}
		//if (direction == Direction::E || direction == Direction::W) {
		//	UE_LOG(LogTemp, Error, TEXT(" --- Done (%d,%d) origin:%d neighbor:%d"), _region.x, _region.y, originFloodId, neighborFloodId);
		//}

		connections.push_back(newConnection);
	}

	void ClearConnections(Direction direction) {
		_directionToConnections[static_cast<int32_t>(direction)].clear();
	}
	
	std::vector<FloodInfo> connectedFloods(FloodInfo info);

	std::array<std::vector<FloodConnection>, 4>& directionToConnections() {
		return _directionToConnections;
	}

	int32 region64Id() { return _region64Id; }

	//! Serialize
	FArchive& operator>>(FArchive &Ar) {
		Ar << _region64Id;
		for (int i = 0; i < 4; i++) {
			SerializeVecObj(Ar, _directionToConnections[i]);
		}
		return Ar;
	}

private:
	int32 _region64Id = -1;
	std::array<std::vector<FloodConnection>, 4> _directionToConnections;
};

/**
 * 
 */
class GameMapFlood
{
public:
	void Init(PunAStar128x256* pathAI, PunTerrainGenerator* terrainGenerator, bool isLoadingFromFile)
	{
		_terrainGenerator = terrainGenerator;
		_pathAI = pathAI;

		_region64ToFloods.resize(Region64Total);
		_region64ToConnections.resize(Region64Total);

		if (isLoadingFromFile) {
			return;
		}

		for (int32 i = 0; i < Region64Total; i++) {
			_region64ToFloods[i].resize(Region64Tiles);
			FillRegion(i);
		}

		for (int32 i = 0; i < Region64Total; i++) {
			_region64ToConnections[i].Init(i);
			RefreshRegionConnections(i);
		}
	}

	void Tick();

	int32 GetFloodId(WorldTile2 tile) {
		return _region64ToFloods[TileToRegion64Id(tile)][TileToLocal64Id(tile)];
	}

	void ResetRegionFloodDelayed(int32 region64Id);

	void ResetAllRegionFlood() {
		for (int32 i = 0; i < Region64Total; i++) {
			FillRegion(i);
		}
		for (int32 i = 0; i < Region64Total; i++) {
			RefreshRegionConnections(i);
		}
	}

	bool IsConnected(WorldTile2 start, WorldTile2 end, int maxRegionDistance);
	bool IsConnected(FloodInfo startFlood, FloodInfo endFlood, int maxRegionDistance);

	// Can go within 1 tile of non-walkable tile such as trees/stones etc.
	// For these cases, we try to find any nearby tile that is within the same flood
	NonWalkableTileAccessInfo TryAccessNonWalkableTile(WorldTile2 start, WorldTile2 nonwalkableTile, int maxRegionDistance)
	{
		FloodInfo startFlood = GetFloodInfo(start);

		WorldTile2 curTile = nonwalkableTile;

		NonWalkableTileAccessInfo nonWalkableTileInfo;
		int32 minDistance = 9999;

		auto checkAdjacentTile = [&]() {
			if (curTile.isValid() && IsConnected(startFlood, GetFloodInfo(curTile), maxRegionDistance)) {
				int32 distance = WorldTile2::ManDistance(start, curTile);
				if (distance < minDistance) {
					nonWalkableTileInfo = NonWalkableTileAccessInfo(nonwalkableTile, curTile);
					minDistance = distance;
				}
			}
		};

		curTile.x++;
		checkAdjacentTile();
		
		curTile.x -= 2;
		checkAdjacentTile();
		
		curTile.x++;
		curTile.y++;
		checkAdjacentTile();
		
		curTile.y -= 2;
		checkAdjacentTile();
		
		//if (curTile.isValid() && IsConnected(startFlood, GetFloodInfo(curTile), maxRegionDistance)) {
		//	return NonWalkableTileAccessInfo(nonwalkableTile, curTile);
		//}

		return nonWalkableTileInfo;
	}

	std::vector<RegionFloodConnections>& region64ToConnections() { 
		return _region64ToConnections;
	}

	// Debug
	PunAStar128x256* pathAI() { return _pathAI; }

	void Serialize(FArchive &Ar)
	{
		SerializeVecVecValue(Ar, _region64ToFloods);
		SerializeVecObj(Ar, _region64ToConnections);
		SerializeVecValue(Ar, _regions64ToReset);
	}

private:
	FloodInfo GetFloodInfo(WorldTile2 tile) {
		int32 region64Id = TileToRegion64Id(tile);
		return FloodInfo(region64Id, _region64ToFloods[region64Id][TileToLocal64Id(tile)]);
	}

	void RefreshRegionConnections(int32 region64Id);

	void FillRegion(int32 region64Id);

	int32 FindRootId(int32 currentId, std::vector<std::vector<int32>>& connectedIds, std::vector<bool>& visited);
	void SetFloodRootIds(int32 rootId, int32 currentId, std::vector<int32>& rootIds, std::vector<std::vector<int32>>& connectedIds, std::vector<bool>& visited);


private:
	PunTerrainGenerator* _terrainGenerator = nullptr;
	PunAStar128x256* _pathAI = nullptr;

	// Flood map for regions
	std::vector<std::vector<int32>> _region64ToFloods; //std::vector<std::vector<int32>> _region64ToFloods;
	std::vector<RegionFloodConnections> _region64ToConnections;

	std::vector<int32> _regions64ToReset;
};
