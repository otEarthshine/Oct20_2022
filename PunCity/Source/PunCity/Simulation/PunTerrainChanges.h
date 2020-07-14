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
		CppUtils::TryAdd(_regionsToHoles[tile.regionId()], tile);
		_simulation->SetNeedDisplayUpdate(DisplayClusterEnum::Terrain, tile.regionId(), true);
	}
	void RemoveHole(WorldTile2 tile) {
		CppUtils::TryRemove(_regionsToHoles[tile.regionId()], tile);
		_simulation->SetNeedDisplayUpdate(DisplayClusterEnum::Terrain, tile.regionId(), true);
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
