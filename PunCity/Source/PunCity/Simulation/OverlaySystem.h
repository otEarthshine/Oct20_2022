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



struct DitchTile
{
	WorldTile2 tile;
	bool isVisible = false;

	//! Serialize
	FArchive& operator>>(FArchive &Ar)
	{
		tile >> Ar;
		Ar << isVisible;
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
		_regionToIrrigationDitch.resize(GameMapConstants::TotalRegions);
		//_regionToFence.resize(GameMapConstants::TotalRegions);
		
		_simulation = simulation;
	}

	//std::vector<uint8_t>& appeal() { return _manmadeAppeal; }
	int32 GetAppealPercent(WorldTile2 tile);

	/*
	 * Road
	 */
	bool IsRoad(WorldTile2 tile) const
	{
		PUN_ENSURE(tile.isValid(), return false);
		
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
		if (isConstructed || !isDirtRoad) {
			_simulation->SetRoadPathAI(tile, true);
		}
		auto& roads = _regionToRoad[tile.regionId()];
		auto found = std::find_if(roads.begin(), roads.end(), [&](const RoadTile& roadTile) { return roadTile.tile == tile; });
		if (found == roads.end()) {
			roads.push_back(RoadTile(tile, isDirtRoad, isConstructed, buildingId));

			_simulation->SetNeedDisplayUpdate(DisplayClusterEnum::Road, tile.regionId(), true);

			_simulation->SetRoadWorldTexture(tile, isConstructed, isDirtRoad);
		}
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

			_simulation->AddFireOnceParticleInfo(ParticleEnum::OnPlacement, TileArea(tile, WorldTile2(1, 1)));
			
			//PUN_LOG("Overlay RemoveRoad True");
			return true;
		}
		//PUN_LOG("Overlay RemoveRoad False");
		return false;
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
					_simulation->SetRoadWorldTexture(roadTiles[i].tile, false, true);
					roadTiles.erase(roadTiles.begin() + i);
				}
			}
			_simulation->SetNeedDisplayUpdate(DisplayClusterEnum::Road, regionId, true);
		}
	}

	const std::vector<RoadTile>& roads(int32 regionId) {
		return _regionToRoad[regionId];
	}

	/*
	 * Irrigation
	 */

	const std::vector<DitchTile>& irrigationDitches(int32 regionId) {
		return _regionToIrrigationDitch[regionId];
	}

	bool IsIrrigationDitch(WorldTile2 tile) const {
		return CppUtils::Contains(_regionToIrrigationDitch[tile.regionId()], [&](const DitchTile& ditchTile) {
			return ditchTile.tile == tile;
		});
	}

	void AddIrrigationDitch(WorldTile2 tile, bool isVisible = true) {
		DitchTile newDitchTile;
		newDitchTile.tile = tile;
		newDitchTile.isVisible = isVisible;
		
		CppUtils::TryAdd_If(_regionToIrrigationDitch[tile.regionId()], newDitchTile, [&](const DitchTile& ditchTileTemp) {
			return ditchTileTemp.tile == tile;
		});
	}
	bool RemoveIrrigationDitch(WorldTile2 tile) {
		return CppUtils::TryRemoveIf(_regionToIrrigationDitch[tile.regionId()], [&](const DitchTile& ditchTile) {
			return ditchTile.tile == tile;
		});
	}

	
	

	/*
	 * Serialize
	 */
	void Serialize(FArchive& Ar)
	{
		SerializeVecVecObj(Ar, _regionToRoad);
		SerializeVecVecObj(Ar, _regionToIrrigationDitch);
	}

private:
	std::vector<std::vector<RoadTile>> _regionToRoad;

	std::vector<std::vector<DitchTile>> _regionToIrrigationDitch;

	IGameSimulationCore* _simulation = nullptr;
};
