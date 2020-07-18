// Pun Dumnernchanvanit's

#pragma once

#include "IGameSimulationCore.h"
#include "ProvinceSystem.h"
#include "Misc/AES.h"

/**
 * 
 */
class SimUtils
{
public:
	static bool CanReserveSpot_Buildable2(WorldRegion2 region, IGameSimulationCore* simulation)
	{
		bool canPlant = true;
		const int32 spotReserveDim = 24;
		const WorldTile2 spotReserveSize(spotReserveDim, spotReserveDim);
		TileArea area(region.centerTile() - spotReserveSize / 2, spotReserveSize);
		area.ExecuteOnArea_WorldTile2([&](WorldTile2 tile) {
			if (!simulation->IsBuildable(tile) &&
				!simulation->IsCritterBuildingIncludeFronts(tile))
			{
				std::vector<int32> frontIds = simulation->frontBuildingIds(tile);
				if (frontIds.size() > 0)
				{
					// If any front isn't regional building, we can't plant
					for (int32 frontId : frontIds) {
						if (!IsRegionalBuilding(simulation->buildingEnum(frontId))) {
							canPlant = false;
							return;
						}
					}
					return;
				}
				
				CardEnum buildingEnum = simulation->buildingEnumAtTile(tile);

				if (!IsRegionalBuilding(buildingEnum)) {
					canPlant = false;
				}
			}
		});

		return canPlant;
	}

	static bool CanReserveSpot_NotTooCloseToAnother(int32 provinceId, IGameSimulationCore* simulation, int spread = 2)
	{
		// Also do not allow reserveSpot
		//if (!region.IsInnerRegion()) {
		//	return false;
		//}
		
		//const int _startingSpotSpread = (spread != -1) ? spread : 2;

		//if (spread == -1) {
		//	spread = _startingSpotSpread;
		//}

		//PUN_LOG("CanReserveSpot_NotTooCloseToAnother spread:%d", spread);
		
		bool cannotPlant = simulation->provinceSystem().ExecuteNearbyProvincesWithExitTrue(provinceId, spread, [&](ProvinceConnection connection)
		{
			return simulation->provinceOwner(connection.provinceId) != -1;
		});

		return !cannotPlant;

		//bool canPlant = true;
		//int minX = std::max(0, region.x - spread);
		//int maxX = std::min(GameMapConstants::RegionsPerWorldX - 1, region.x + spread);
		//int minY = std::max(0, region.y - spread);
		//int maxY = std::min(GameMapConstants::RegionsPerWorldY - 1, region.y + spread);
		//for (int xx = minX; xx <= maxX; xx++) {
		//	for (int yy = minY; yy <= maxY; yy++)
		//	{
		//		int32_t regionId = xx + yy * GameMapConstants::RegionsPerWorldX;
		//		//if (_regionIdToStartingSpots.find(regionId) != _regionIdToStartingSpots.end()) {
		//		//	canPlant = false;
		//		//	break;
		//		//}
		//		if (simulation->regionOwner(regionId) != -1) {
		//			canPlant = false;
		//			break;
		//		}
		//	}
		//}

		//return canPlant;
	}

	static TileArea FindStartSpot(int32 provinceId, IGameSimulationCore* simulation)
	{
		auto& provinceSys = simulation->provinceSystem();
		
		PUN_CHECK(provinceSys.IsProvinceValid(provinceId));

		// Spiral from the middle of province's rectArea
		TileArea provinceRectArea = provinceSys.GetProvinceRectArea(provinceId);
		
		WorldTile2 townhallSize = GetBuildingInfo(CardEnum::Townhall).size;
		TileArea area = BuildingArea(provinceRectArea.centerTile(), townhallSize, Direction::S);

		// Add Road...
		area.minX = area.minX - 1;
		area.minY = area.minY - 1;
		area.maxX = area.maxX + 1;
		area.maxY = area.maxY + 1;

		// Add Storages
		area.minY = area.minY - 4;

		if (!area.isValid()) {
			return TileArea::Invalid;
		}
		
		return SpiralFindAvailableBuildArea(area, [&](WorldTile2 tile)
		{
			// Out of province, can't build...
			if (abs(provinceSys.GetProvinceIdRaw(tile)) != provinceId) {
				return false;
			}
			
			// Not buildable and not critter building
			if (!simulation->IsBuildable(tile) &&
				!simulation->IsCritterBuildingIncludeFronts(tile))
			{
				std::vector<int32> frontIds = simulation->frontBuildingIds(tile);
				if (frontIds.size() > 0)
				{
					// If any front isn't regional building, we can't plant
					for (int32 frontId : frontIds) {
						if (!IsRegionalBuilding(simulation->buildingEnum(frontId))) {
							return false;
						}
					}
					return true;
				}

				CardEnum buildingEnum = simulation->buildingEnumAtTile(tile);
				return IsRegionalBuilding(buildingEnum);
			}
			return true;
		},
		[&](WorldTile2 tile) {
			return !provinceRectArea.HasTile(tile);
		});
	}

	template <typename Func, typename Func2>
	static TileArea SpiralFindAvailableBuildArea(TileArea startArea, Func isValidTile, Func2 isOutOfBound)
	{
		// Just spiral out trying to find isWalkable tile...
		int32 x = 0;
		int32 y = 0;
		int32 dx = 0;
		int32 dy = -1;

		int32 badTileCount = 0;
		startArea.ExecuteOnArea_WorldTile2([&](WorldTile2 tile) {
			if (!isValidTile(tile)) {
				badTileCount++;
			}
		});


		WorldTile2 minTile = startArea.min();
		WorldTile2 size = startArea.size();
		
		int32 loop;
		for (loop = 20000; loop-- > 0;)
		{
			TileArea area = TileArea(minTile + WorldTile2(x, y), size);

			// If area is out of bound, exit from it
			if (isOutOfBound(area.corner00()) ||
				isOutOfBound(area.corner01()) ||
				isOutOfBound(area.corner10()) ||
				isOutOfBound(area.corner11())) 
			{
				return TileArea::Invalid;
			}

			if (badTileCount == 0) {
				return area;
			}
			check(badTileCount > 0);
		
			

			// Spiral
			if ((x == y) || ((x < 0) && (x == -y)) || ((x > 0) && (x == 1 - y)))
			{
				int32 temp = dx;
				dx = -dy;
				dy = temp;
			}


			// Moving in x direction: remove -x, add +x
			if (dx > 0) {
				int32 xRemove = x;
				int32 xAdd = x + size.x;
				for (int32 y_ = 0; y_ < size.y; y_++) {
					if (!isValidTile(minTile + WorldTile2(xRemove, y_ + y))) {
						badTileCount--;
					}
					if (!isValidTile(minTile + WorldTile2(xAdd, y_ + y))) {
						badTileCount++;
					}
				}
			}
			else if (dx < 0) {
				int32 xRemove = x + size.x - 1;
				int32 xAdd = x - 1;
				for (int32 y_ = 0; y_ < size.y; y_++) {
					if (!isValidTile(minTile + WorldTile2(xRemove, y_ + y))) {
						badTileCount--;
					}
					if (!isValidTile(minTile + WorldTile2(xAdd, y_ + y))) {
						badTileCount++;
					}
				}
			}
			else if (dy > 0) {
				int32 yRemove = y;
				int32 yAdd = y + size.y;
				for (int32 x_ = 0; x_ < size.x; x_++) {
					if (!isValidTile(minTile + WorldTile2(x_ + x, yRemove))) {
						badTileCount--;
					}
					if (!isValidTile(minTile + WorldTile2(x_ + x, yAdd))) {
						badTileCount++;
					}
				}
			}
			else if (dy < 0) {
				int32 yRemove = y + size.y - 1;
				int32 yAdd = y - 1;
				for (int32 x_ = 0; x_ < size.x; x_++) {
					if (!isValidTile(minTile + WorldTile2(x_ + x, yRemove))) {
						badTileCount--;
					}
					if (!isValidTile(minTile + WorldTile2(x_ + x, yAdd))) {
						badTileCount++;
					}
				}
			}
			else {
				UE_DEBUG_BREAK();
			}
			
			

			x += dx;
			y += dy;
			
		}

		check(loop > 1);

		return TileArea::Invalid;
	}





	
};
