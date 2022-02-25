// Fill out your copyright notice in the Description page of Project Settings.

#include "OverlaySystem.h"
#include "BuildingSystem.h"
#include "Buildings/GathererHut.h"

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

void OverlaySystem::RefreshIrrigationFill(int32 townId)
{
	// Reset town's irrigation
	const std::vector<int32>& provincesClaimed = _simulation->GetTownProvincesClaimed(townId);
	std::vector<WorldRegion2> overlapRegions = _simulation->provinceSystem().GetRegionOverlaps(provincesClaimed);
	for (WorldRegion2 region : overlapRegions)
	{
		std::vector<DitchTile>& ditchTiles = _regionToIrrigationDitch[region.regionId()];
		for (DitchTile& ditchTile : ditchTiles) {
			ditchTile.isFilled = false;
			AddTileForHumanFertilityRefresh(ditchTile.tile);
		}

		_simulation->SetNeedDisplayUpdate(DisplayClusterEnum::Building, region.regionId());
	}

	// Might not need to flood
	std::vector<int32> bldIds = _simulation->buildingIdsFiltered(townId, CardEnum::IrrigationPump, [&](Building& bld) {
		return !bld.isConstructed();
	});

	if (bldIds.size() == 0) {
		return;
	}
	
	// Flood
	std::vector<std::vector<WorldTile2>> floodQueues;
	std::vector<int32> bldIndexToQueueIndex;
	std::vector<int32> waterLeft;

	floodQueues.resize(bldIds.size());
	bldIndexToQueueIndex.resize(bldIds.size());
	
	for (int32 i = 0; i < bldIds.size(); i++) {
		IrrigationPump& pump = _simulation->building<IrrigationPump>(bldIds[i]);
		floodQueues[i].push_back(pump.GetFirstIrrigationDitchTile(true));
		bldIndexToQueueIndex[i] = i;
		waterLeft.push_back(pump.trueTotalWater());
	}
	

	TMap<int32, int32> visitedTilesToBldIndex;

	int32 floodCompleted;
	do
	{
		floodCompleted = 0;

		for (int32 i = 0; i < bldIds.size(); i++)
		{
			int32 totalQueuedTiles = 0;
			
			if (waterLeft[i] > 0)
			{
				PUN_LOG("Flood INDEX:%d waterLeft:%d", i, waterLeft[i]);

				int32 queueIndex = bldIndexToQueueIndex[i];
				std::vector<WorldTile2>& floodQueue = floodQueues[queueIndex];

				if (floodQueue.size() > 0)
				{
					totalQueuedTiles += floodQueue.size();

					WorldTile2 curTile = floodQueue[0];
					floodQueue.erase(floodQueue.begin());

					PUN_LOG("Flood %s queueLeft:%d queueIndex:%d", *curTile.To_FString(), floodQueue.size(), queueIndex);

					int32 tileId = curTile.tileId();
					if (visitedTilesToBldIndex.Contains(tileId))
					{
						int32 tileQueueIndex = bldIndexToQueueIndex[visitedTilesToBldIndex[tileId]];
						if (tileQueueIndex != queueIndex) 
						{
							PUN_LOG("Flood Merge %d %d", queueIndex, tileQueueIndex);
							
							// merge the queues
							std::vector<WorldTile2>& tileFloodQueue = floodQueues[tileQueueIndex];
							tileFloodQueue.insert(tileFloodQueue.end(), floodQueue.begin(), floodQueue.end());
							floodQueue.clear();

							// anything pointing to queueIndex should be redirected to tileFloodQueue
							for (int32 j = 0; j < bldIndexToQueueIndex.size(); j++) {
								if (bldIndexToQueueIndex[j] == queueIndex) {
									bldIndexToQueueIndex[j] = tileQueueIndex;
								}
							}
						}
					}
					else
					{
						visitedTilesToBldIndex.Add(tileId, i);
						waterLeft[i]--;

						GetIrrigationDitch(curTile)->isFilled = true;

						PUN_LOG(" - Flood Good");

						// Flood nearby
						auto checkNearby = [&](WorldTile2 tile) {
							if (!visitedTilesToBldIndex.Contains(tile.tileId())) {
								DitchTile* ditchTile = GetIrrigationDitch(tile);
								if (ditchTile) {
									floodQueue.push_back(tile);
								}
							}
						};

						checkNearby(curTile + WorldTile2(1, 0));
						checkNearby(curTile + WorldTile2(-1, 0));
						checkNearby(curTile + WorldTile2(0, 1));
						checkNearby(curTile + WorldTile2(0, -1));
					}
				}
				
			}

			if (waterLeft[i] <= 0 || totalQueuedTiles <= 0) {
				floodCompleted++;
			}
		}
	}
	while (floodCompleted < bldIds.size());

	for (int32 i = 0; i < bldIds.size(); i++)
	{
		IrrigationPump& pump = _simulation->building<IrrigationPump>(bldIds[i]);
		pump.trueWaterLeft = waterLeft[i];
	}
	
}