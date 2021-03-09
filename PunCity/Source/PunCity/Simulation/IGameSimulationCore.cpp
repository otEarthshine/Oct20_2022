#include "IGameSimulationCore.h"


using namespace std;



//bool CanReserveSpot_Buildable(WorldRegion2 region, IGameSimulationCore* simulation)
//{
//	bool canPlant = true;
//	const int32 spotReserveDim = 24;
//	const WorldTile2 spotReserveSize(spotReserveDim, spotReserveDim);
//	TileArea area(region.centerTile() - spotReserveSize / 2, spotReserveSize);
//	area.ExecuteOnArea_WorldTile2([&](WorldTile2 tile) {
//		if (!simulation->IsBuildable(tile) &&
//			!simulation->IsCritterBuildingIncludeFronts(tile))
//		{
//			std::vector<int32> frontIds = simulation->frontBuildingIds(tile);
//			if (frontIds.size() > 0)
//			{
//				// If any front isn't regional building, we can't plant
//				for (int32 frontId : frontIds) {
//					if (!IsRegionalBuilding(simulation->buildingEnum(frontId))) {
//						canPlant = false;
//						return;
//					}
//				}
//				return;
//			}
//			else
//			{
//				CardEnum buildingEnum = simulation->buildingEnumAtTile(tile);
//
//				if (!IsRegionalBuilding(buildingEnum)) {
//					canPlant = false;
//				}
//			}
//		}
//	});
//
//	return canPlant;
//}

//bool CanReserveSpot_NotTooCloseToAnother(WorldRegion2 region, IGameSimulationCore* simulation, int spread)
//{
//	const int _startingSpotSpread = 2;
//
//	if (spread == -1) {
//		spread = _startingSpotSpread;
//	}
//
//	bool canPlant = true;
//	int minX = max(0, region.x - spread);
//	int maxX = min(GameMapConstants::RegionsPerWorldX - 1, region.x + spread);
//	int minY = max(0, region.y - spread);
//	int maxY = min(GameMapConstants::RegionsPerWorldY - 1, region.y + spread);
//	for (int xx = minX; xx <= maxX; xx++) {
//		for (int yy = minY; yy <= maxY; yy++)
//		{
//			int32_t regionId = xx + yy * GameMapConstants::RegionsPerWorldX;
//			//if (_regionIdToStartingSpots.find(regionId) != _regionIdToStartingSpots.end()) {
//			//	canPlant = false;
//			//	break;
//			//}
//			if (simulation->regionOwner(regionId) != -1) {
//				canPlant = false;
//				break;
//			}
//		}
//	}
//
//	return canPlant;
//}