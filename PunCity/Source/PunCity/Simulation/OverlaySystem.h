#pragma once

#include "GameSimulationInfo.h"
#include <algorithm>
#include "IGameSimulationCore.h"
#include "PunCity/PunUtils.h"
#include "ProvinceSystem.h"
#include "PunCity/DisplaySystem/PunTerrainGenerator.h"

struct RoadTile
{
	WorldTile2 tile;
	bool isDirt;
	bool isConstructed;
	int32 buildingId;

	RoadTile() : tile(0), isDirt(false), isConstructed(false), buildingId(-1) {}
	RoadTile(WorldTile2 tile, bool isDirt, float isConstructed, int32 buildingId)
			: tile(tile), isDirt(isDirt), isConstructed(isConstructed), buildingId(buildingId) {}
	
	bool isValid() { return tile.isValid(); }
	bool operator==(const RoadTile& a) {
		return tile == a.tile && isDirt == a.isDirt;
	}

	//! Serialize
	FArchive& operator>>(FArchive &Ar)
	{
		tile >> Ar;
		Ar << isDirt << isConstructed;
		Ar << buildingId;
		return Ar;
	}
};

/**
 * 
 */
class OverlaySystem
{
public:
	void Init(IGameSimulationCore* simulation) {
		
		// TODO: don't use _appeal... use buildingSubRegion list to get buildings nearby and calc appeal from there instead...
		//_manmadeAppeal.resize(GameMapConstants::TilesPerWorldX * GameMapConstants::TilesPerWorldY, 0);

		_regionToRoad.resize(GameMapConstants::TotalRegions);
		//_regionToFence.resize(GameMapConstants::TotalRegions);
		_simulation = simulation;
	}

	//std::vector<uint8_t>& appeal() { return _manmadeAppeal; }
	int32 GetAppealPercent(WorldTile2 tile);

	// TODO: move Road/Fence out???
	bool IsRoad(WorldTile2 tile) const {
		auto &roads = _regionToRoad[tile.regionId()];
		auto found = std::find_if(roads.begin(), roads.end(), [&](const RoadTile& roadTile) { return roadTile.tile == tile; });
		return found != roads.end();
	}
	RoadTile GetRoad(WorldTile2 tile) const {
		auto &roads = _regionToRoad[tile.regionId()];
		auto found = std::find_if(roads.begin(), roads.end(), [&](const RoadTile& roadTile) { return roadTile.tile == tile; });
		if (found != roads.end()) {
			return *found;
		}
		return RoadTile(WorldTile2::Invalid, false, false, -1);
	}

	// This is for initial roads (like that around townhall)
	void AddRoad(WorldTile2 tile, bool isDirtRoad, bool isConstructed, int32 buildingId = -1) {
		if (isConstructed) {
			_simulation->SetRoadPathAI(tile, true);
		}
		auto& roads = _regionToRoad[tile.regionId()];
		auto found = std::find_if(roads.begin(), roads.end(), [&](const RoadTile& roadTile) { return roadTile.tile == tile; });
		check(found == roads.end());
		roads.push_back(RoadTile(tile, isDirtRoad, isConstructed, buildingId));

		_simulation->SetNeedDisplayUpdate(DisplayClusterEnum::Road, tile.regionId(), true);

		_simulation->SetRoadWorldTexture(tile, isConstructed, isDirtRoad);
	}

	bool RemoveRoad(WorldTile2 tile) {
		// TODO: should SetRoad be here?
		_simulation->SetRoadPathAI(tile, false);
		auto &roads = _regionToRoad[tile.regionId()];
		auto found = std::find_if(roads.begin(), roads.end(), [&](const RoadTile& roadTile) { return roadTile.tile == tile; });
		if (found != roads.end()) {
			roads.erase(found);
			_simulation->SetNeedDisplayUpdate(DisplayClusterEnum::Road, tile.regionId(), true);

			_simulation->SetRoadWorldTexture(tile, false, true);
			//PUN_LOG("Overlay RemoveRoad True");
			return true;
		}
		//PUN_LOG("Overlay RemoveRoad False");
		return false;
	}

	void ClearRoadInRegion(int32 regionId)
	{
		std::vector<RoadTile> roadTiles = _regionToRoad[regionId];
		for (RoadTile roadTile : roadTiles) {
			_simulation->SetRoadPathAI(roadTile.tile, false);
		}
		_regionToRoad[regionId].clear();
		_simulation->SetNeedDisplayUpdate(DisplayClusterEnum::Road, regionId, true);
	}

	void ClearRoadInProvince(int32 provinceId)
	{
		const std::vector<WorldRegion2>& overlapRegions = _simulation->provinceSystem().GetRegionOverlaps(provinceId);
		for (WorldRegion2 region : overlapRegions) 
		{
			int32 regionId = region.regionId();
			std::vector<RoadTile>& roadTiles = _regionToRoad[regionId];
			for (size_t i = roadTiles.size(); i-- > 0;) 
			{
				if (_simulation->GetProvinceIdClean(roadTiles[i].tile) == provinceId) {
					_simulation->SetRoadPathAI(roadTiles[i].tile, false);
					roadTiles.erase(roadTiles.begin() + i);
				}
			}
			_simulation->SetNeedDisplayUpdate(DisplayClusterEnum::Road, regionId, true);
		}
	}

	const std::vector<RoadTile>& roads(int32 regionId) {
		return _regionToRoad[regionId];
	}

	void Serialize(FArchive& Ar)
	{
		SerializeVecVecObj(Ar, _regionToRoad);
	}

private:
	std::vector<std::vector<RoadTile>> _regionToRoad;

	IGameSimulationCore* _simulation = nullptr;
};
