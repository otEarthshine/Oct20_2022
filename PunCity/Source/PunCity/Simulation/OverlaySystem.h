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
	bool isFilled = false;

	//! Serialize
	FArchive& operator>>(FArchive &Ar)
	{
		tile >> Ar;
		Ar << isVisible;
		Ar << isFilled;
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
		_regionToHumanFertility.resize(GameMapConstants::TotalRegions);
		//_regionToFence.resize(GameMapConstants::TotalRegions);
		
		_simulation = simulation;
	}

	void Tick()
	{
		for (int32 townId : _townToRefreshIrrigation) {
			RefreshIrrigationFill(townId);
		}
		_townToRefreshIrrigation.clear();

		RefreshHumanFertility();
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

	void AddIrrigationDitch(WorldTile2 tile, bool isVisible = true)
	{
		AddTileForHumanFertilityRefresh(tile);
		RefreshIrrigationFillDelayed(_simulation->tileOwnerTown(tile));
		
		DitchTile newDitchTile;
		newDitchTile.tile = tile;
		newDitchTile.isVisible = isVisible;
		
		CppUtils::TryAdd_If(_regionToIrrigationDitch[tile.regionId()], newDitchTile, [&](const DitchTile& ditchTileTemp) {
			return ditchTileTemp.tile == tile;
		});
	}
	bool RemoveIrrigationDitch(WorldTile2 tile)
	{
		if (!IsValidMajorTown(_simulation->tileOwnerTown(tile))) {
			return false;
		}
		
		AddTileForHumanFertilityRefresh(tile);
		RefreshIrrigationFillDelayed(_simulation->tileOwnerTown(tile));
		
		return CppUtils::TryRemoveIf(_regionToIrrigationDitch[tile.regionId()], [&](const DitchTile& ditchTile) {
			return ditchTile.tile == tile;
		});
	}

	void RefreshIrrigationFillDelayed(int32 townId) {
		if (townId != -1) {
			CppUtils::TryAdd(_townToRefreshIrrigation, townId);
		}
	}
	void RefreshIrrigationFill(int32 townId);

	
	//

	uint8 GetHumanFertility(WorldTile2 tile)
	{
		std::vector<uint8>& humanFertility = _regionToHumanFertility[tile.regionId()];
		if (humanFertility.size() == 0) {
			return 0;
		}
		return humanFertility[tile.localTileId()];
	}

	void AddTileForHumanFertilityRefresh(WorldTile2 tile) {
		tile.region().ExecuteOnNearbyRegions([&](WorldRegion2 region) {
			CppUtils::TryAdd(_regionsToRefreshHumanFertility, region);
		});
	}
	void RefreshHumanFertility()
	{
		for (WorldRegion2 region : _regionsToRefreshHumanFertility)
		{
			if (_regionToHumanFertility[region.regionId()].size() == 0) {
				_regionToHumanFertility[region.regionId()].resize(CoordinateConstants::TileIdsPerRegion);
			}
			
			region.ExecuteOnRegion_WorldTile([&](WorldTile2 tile) 
			{
				int32 fertility = 0;
				
				region.ExecuteOnNearbyRegions([&](WorldRegion2 nearbyRegions) {
					const std::vector<DitchTile>& ditchTiles = _regionToIrrigationDitch[nearbyRegions.regionId()];
					for (const DitchTile& ditchTile : ditchTiles) {
						if (ditchTile.isFilled) {
							fertility = std::max(fertility, (10 - WorldTile2::Distance(ditchTile.tile, tile)) * 100 / 5);
						}
					}
				});

				fertility = std::min(fertility, 95);
				_regionToHumanFertility[region.regionId()][tile.localTileId()] = static_cast<uint8>(fertility);
			});
		}
		_regionsToRefreshHumanFertility.clear();
	}
	

	/*
	 * Serialize
	 */
	void Serialize(FArchive& Ar)
	{
		SerializeVecVecObj(Ar, _regionToRoad);
		SerializeVecVecObj(Ar, _regionToIrrigationDitch);
		
		SerializeVecVecValue(Ar, _regionToHumanFertility);
		SerializeVecObj(Ar, _regionsToRefreshHumanFertility);

		SerializeVecValue(Ar, _townToRefreshIrrigation);
	}

private:
	DitchTile* GetIrrigationDitch(WorldTile2 tile) {
		std::vector<DitchTile>& ditchTiles = _regionToIrrigationDitch[tile.regionId()];
		for (DitchTile& ditchTile : ditchTiles) {
			if (ditchTile.tile == tile) {
				return &ditchTile;
			}
		}
		return nullptr;
	}

private:
	std::vector<std::vector<RoadTile>> _regionToRoad;

	std::vector<std::vector<DitchTile>> _regionToIrrigationDitch;
	
	std::vector<std::vector<uint8>> _regionToHumanFertility;
	std::vector<WorldRegion2> _regionsToRefreshHumanFertility;

	std::vector<int32> _townToRefreshIrrigation;

	//! Not Serialized
	IGameSimulationCore* _simulation = nullptr;
};
