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
		}

		_simulation->SetNeedDisplayUpdate(DisplayClusterEnum::Building, region.regionId());
	}

	// Might not need to flood
	std::vector<int32> bldIds = _simulation->buildingIds(townId, CardEnum::IrrigationPump);
	if (bldIds.size() == 0) {
		return;
	}
	
	// Flood
	std::vector<std::vector<WorldTile2>> floodQueues;
	std::vector<std::vector<int32>> connectedIndices; // when hitting other visited tile indices, we can flood using queues from other indices
	std::vector<int32> waterLeft;

	floodQueues.resize(bldIds.size());
	connectedIndices.resize(bldIds.size());
	
	for (int32 i = 0; i < bldIds.size(); i++) {
		IrrigationPump& pump = _simulation->building<IrrigationPump>(bldIds[i]);
		floodQueues[i].push_back(pump.GetFirstIrrigationDitchTile());
		connectedIndices[i].push_back(i); // connected to its own floodQueue
		waterLeft.push_back(pump.efficiency());
	}
	

	TMap<int32, int32> visitedTilesToIndex;

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

				// go through all flood queues that are connected
				for (int32 j = connectedIndices[i].size(); j-- > 0;)
				{
					std::vector<WorldTile2>& floodQueue = floodQueues[connectedIndices[i][j]];

					if (floodQueue.size() > 0)
					{
						totalQueuedTiles += floodQueue.size();

						WorldTile2 curTile = floodQueue[0];
						floodQueue.erase(floodQueue.begin());

						PUN_LOG("Flood %s queueLeft:%d connectedIndex:%d", *curTile.To_FString(), floodQueue.size(), connectedIndices[i][j]);

						int32 tileId = curTile.tileId();
						if (visitedTilesToIndex.Contains(tileId))
						{
							CppUtils::TryAdd(connectedIndices[i], visitedTilesToIndex[tileId]);
						}
						else
						{
							visitedTilesToIndex.Add(tileId, i);
							waterLeft[i]--;

							GetIrrigationDitch(curTile)->isFilled = true;

							PUN_LOG(" - Flood Good");

							// Flood nearby
							auto checkNearby = [&](WorldTile2 tile) {
								if (!visitedTilesToIndex.Contains(tile.tileId())) {
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
		pump.waterLeft = waterLeft[i];
	}
	
}