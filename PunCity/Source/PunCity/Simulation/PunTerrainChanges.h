// Pun Dumnernchanvanit's

#pragma once

#include "GameSimulationInfo.h"
#include "PunCity/CppUtils.h"
#include "IGameSimulationCore.h"

/**
 * 
 */
class PunTerrainChanges
{
public:
	void Init(IGameSimulationCore* simulation) {
		_simulation = simulation;
		_regionsToHoles.resize(GameMapConstants::TotalRegions);
	}
	
	void AddHole(WorldTile2 tile) {
		std::vector<int32> regionIds = GetAffectedRegions(tile);
		for (int32 regionId : regionIds) {
			CppUtils::TryAdd(_regionsToHoles[regionId], tile);
			_simulation->SetNeedDisplayUpdate(DisplayClusterEnum::Terrain, regionId, true);
		}
	}
	void RemoveHole(WorldTile2 tile) {
		std::vector<int32> regionIds = GetAffectedRegions(tile);
		for (int32 regionId : regionIds) {
			CppUtils::TryRemove(_regionsToHoles[regionId], tile);
			_simulation->SetNeedDisplayUpdate(DisplayClusterEnum::Terrain, regionId, true);
		}
	}

	static std::vector<int32> GetAffectedRegions(WorldTile2 tile)
	{
		std::vector<int32> regionIds;
		CppUtils::TryAdd(regionIds, tile.regionId());
		CppUtils::TryAdd(regionIds, WorldTile2(tile.x + 1, tile.y).regionId());
		CppUtils::TryAdd(regionIds, WorldTile2(tile.x - 1, tile.y).regionId());
		CppUtils::TryAdd(regionIds, WorldTile2(tile.x, tile.y + 1).regionId());
		CppUtils::TryAdd(regionIds, WorldTile2(tile.x, tile.y - 1).regionId());
		return regionIds;
	}

	std::vector<WorldTile2> GetRegionHoles(WorldRegion2 region) {
		return _regionsToHoles[region.regionId()];
	}

	void Serialize(FArchive &Ar)
	{
		SerializeVecVecObj(Ar, _regionsToHoles);
	}

private:
	IGameSimulationCore* _simulation = nullptr;
	std::vector<std::vector<WorldTile2>> _regionsToHoles;
};
