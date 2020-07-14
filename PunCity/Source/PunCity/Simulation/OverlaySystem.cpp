// Fill out your copyright notice in the Description page of Project Settings.

#include "OverlaySystem.h"
#include "BuildingSystem.h"

using namespace std;

int32 OverlaySystem::GetAppealPercent(WorldTile2 tile)
{
	//int32_t addedAppeal = _manmadeAppeal[tile.x + tile.y * GameMapConstants::TilesPerWorldX];
	auto& buildingSystem =_simulation->buildingSystem();

	std::vector<WorldRegion2> regions = tile.nearby4Regions();
	
	int32 manmadeAppeal = 0;
	auto changeAppeal = [&](int32_t appealChange, int32_t distAtom, int32_t radiusAtom) {
		const int32_t lerpDivisions = 10;
		int32_t percentToFull = 100 * (radiusAtom - distAtom) / radiusAtom * lerpDivisions;
		percentToFull = min(100, percentToFull);
		manmadeAppeal += percentToFull * appealChange / 100;
	};
	
	for (const WorldRegion2& region : regions) 
	{
		if (region.IsValid()) 
		{
			// Put in appeal from forest by checking treeShades in 5x5 area?? 
			
			buildingSystem.buildingSubregionList().ExecuteRegion(region, [&](int32 buildingId)
			{
				Building& building = buildingSystem.building(buildingId);
				CardEnum buildingEnum = building.buildingEnum();

				if (IsAppealAffectingBuilding(buildingEnum))
				{
					int32 distAtoms = WorldAtom2::Distance(tile.worldAtom2(), building.centerTile().worldAtom2());
					AppealInfo info = GetBuildingAppealInfo(buildingEnum);
					int32 radiusAtoms = info.appealRadius * CoordinateConstants::AtomsPerTile;
					
					if (distAtoms < radiusAtoms) {
						changeAppeal(info.appealIncrease, distAtoms, radiusAtoms);
					}
					
					//if (IsDecorationBuilding(buildingEnum)) {
					//	changeAppeal(GetDecorationAppealIncrease(buildingEnum), distAtom, radiusAtom);
					//}
					//else if (IsHeavyIndustrialBuilding(buildingEnum) ||
					//		 IsMountainMine(buildingEnum)) 
					//{
					//	changeAppeal(-20, distAtom, radiusAtom);
					//}
					//else if (IsIndustrialBuilding(buildingEnum)) {
					//	changeAppeal(-10, distAtom, radiusAtom);
					//}
				}
			});
		}
	}

	int32_t naturalAppeal = _simulation->terrainGenerator().GetAppealPercent(tile);
	return manmadeAppeal + naturalAppeal;
}