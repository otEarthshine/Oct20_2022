// Fill out your copyright notice in the Description page of Project Settings.

#include "GeoresourceSystem.h"
#include "../GameRand.h"
#include "PunCity/DisplaySystem/PunTerrainGenerator.h"
#include <algorithm>
#include "PunCity/NetworkStructs.h"
#include "Building.h"
#include "PunCity/Simulation/ProvinceSystem.h"

using namespace std;

void GeoresourceSystem::InitGeoresourceSystem(IGameSimulationCore* simulation, bool isFullInit)
{
	_simulation = simulation;
	_provinceToGeoresource.reserve(GameMapConstants::TotalRegions);

	auto& terrainGenerator = simulation->terrainGenerator();
	auto& provinceSys = simulation->provinceSystem();

	for (int i = 0; i < GameMapConstants::TotalRegions; i++) {
		WorldTile2 centerTile = isFullInit ? provinceSys.GetProvinceCenterTile(i) : WorldTile2::Invalid;
		_provinceToGeoresource.push_back(GeoresourceNode::Create(GeoresourceEnum::None, i, centerTile));
	}

	if (!isFullInit) {
		return;
	}

	auto isGeoresourceBuildable = [&](int32 provinceIdIn) {
		WorldTile2 center = provinceSys.GetProvinceCenterTile(provinceIdIn);
		TileArea area(center, 5);
		area.EnforceWorldLimit();

		bool isNotBuildable = area.ExecuteOnAreaWithExit_WorldTile2([&](WorldTile2 tileIn)
		{
			if (!_simulation->IsBuildable(tileIn)) {
				return true;
			}
			if (provinceSys.GetProvinceIdRaw(tileIn) != provinceIdIn) {
				return true;
			}
			return false;
		});

		return !isNotBuildable;
	};


	/*
	 * Start by generating usable slots
	 */
	TMap<int32, bool> usableHeavyMountainProvinceIds;
	TMap<int32, bool> usableLightMountainProvinceIds;
	TMap<int32, bool> usableFlatProvinceIds;

	auto hasExistingResource = [&](int32 provinceId) {
		return (usableHeavyMountainProvinceIds.Contains(provinceId) && usableHeavyMountainProvinceIds[provinceId]) ||
			(usableLightMountainProvinceIds.Contains(provinceId) && usableLightMountainProvinceIds[provinceId]) ||
			(usableFlatProvinceIds.Contains(provinceId) && usableFlatProvinceIds[provinceId]);
	};
	auto hasNearbyExistingResource = [&](int32 provinceId) {
		if (hasExistingResource(provinceId)) {
			return true;
		}
		return provinceSys.ExecuteAdjacentProvincesWithExitTrue(provinceId, [&](const ProvinceConnection& connection) -> bool {
			return hasExistingResource(connection.provinceId);
		});
	};

	// Find all usable Mineral slots
	for (int i = 0; i < GameMapConstants::TotalRegions; i++)
	{
		int32 provinceId = i;

		if (provinceSys.IsProvinceValid(provinceId) &&
			provinceSys.provinceFlatTileCount(provinceId) > CoordinateConstants::TileIdsPerRegion / 2)
		{
			if (!hasNearbyExistingResource(provinceId) &&
				provinceSys.provinceMountainTileCount(provinceId) > provinceSys.provinceTileCount(provinceId) / 3)
			{
				usableHeavyMountainProvinceIds.Add(provinceId, true);
			}
		}
	}

	for (int i = 0; i < GameMapConstants::TotalRegions; i++)
	{
		int32 provinceId = i;

		if (provinceSys.IsProvinceValid(provinceId) &&
			provinceSys.provinceFlatTileCount(provinceId) > CoordinateConstants::TileIdsPerRegion / 2)
		{
			if (!hasNearbyExistingResource(provinceId) &&
				provinceSys.provinceMountainTileCount(provinceId) > CoordinateConstants::TileIdsPerRegion / 2)
			{
				usableLightMountainProvinceIds.Add(provinceId, true);
			}
		}
	}

	// Find all usable Flat slots
	for (int i = 0; i < GameMapConstants::TotalRegions; i++)
	{
		int32 provinceId = i;
		if (provinceSys.IsProvinceValid(provinceId) &&
			provinceSys.provinceFlatTileCount(provinceId) >= CoordinateConstants::TileIdsPerRegion / 2)
		{
			BiomeEnum biomeEnum = _simulation->GetBiomeProvince(provinceId);
			
			if (biomeEnum == BiomeEnum::Forest || biomeEnum == BiomeEnum::Jungle) 
			{
				bool hasNearbyFlatResource = provinceSys.ExecuteAdjacentProvincesWithExitTrue(provinceId, [&](const ProvinceConnection& connection) -> bool {
					return usableFlatProvinceIds.Contains(connection.provinceId);
				});

				if (!hasNearbyFlatResource) {
					usableFlatProvinceIds.Add(provinceId, true);
				}
			}
		}
	}

	int32 totalUsableProvinces = usableHeavyMountainProvinceIds.Num() +
		usableLightMountainProvinceIds.Num() +
		usableFlatProvinceIds.Num();

	/*
	 * Ratio of different resource type
	 */

	std::pair<GeoresourceEnum, int32> pairGem = { GeoresourceEnum::Gemstone, 3 };
	std::pair<GeoresourceEnum, int32> pairGoldOre = { GeoresourceEnum::GoldOre, 4 };
	std::pair<GeoresourceEnum, int32> pairOil = { GeoresourceEnum::Oil, 7 };

	std::vector<std::pair<GeoresourceEnum, int32>> mountainGeoresourceToSpawnRatio
	{
		{ GeoresourceEnum::IronOre, 10 },
		{ GeoresourceEnum::CoalOre, 4 },
	};

	std::vector<std::pair<GeoresourceEnum, int32>> jungleGeoresourceToSpawnRatio{
		{ GeoresourceEnum::CannabisFarm, 8 },
		{ GeoresourceEnum::CocoaFarm, 8 },
		{ GeoresourceEnum::CoffeeFarm, 5 },
	};
	std::vector<std::pair<GeoresourceEnum, int32>> forestGeoresourceToSpawnRatio{
		{ GeoresourceEnum::GrapeFarm, 8 },
		{ GeoresourceEnum::TulipFarm, 5 },
	};
	std::vector<std::pair<GeoresourceEnum, int32>> flatGeoresourceToSpawnRatio{
		{ GeoresourceEnum::CottonFarm, 8 },
		{ GeoresourceEnum::DyeFarm, 8 },
	};
	

	int32 totalRatio = 0;
	auto addRatio = [&](std::vector<std::pair<GeoresourceEnum, int32>>& georesourceToSpawnRatio) {
		for (int32 i = 0; i < georesourceToSpawnRatio.size(); i++) {
			totalRatio += georesourceToSpawnRatio[i].second;
		}
	};
	addRatio(mountainGeoresourceToSpawnRatio);
	addRatio(jungleGeoresourceToSpawnRatio);
	addRatio(forestGeoresourceToSpawnRatio);
	addRatio(flatGeoresourceToSpawnRatio);

	totalRatio += pairGoldOre.second;
	totalRatio += pairGem.second;
	totalRatio += pairOil.second;


	// Check mountain first
	//  Mountain should fill with ratio that combines both mountain+flat
	auto createGeoresourceToFill = [&](std::vector<GeoresourceEnum>& georesourceToFill, std::vector<std::pair<GeoresourceEnum, int32>>& georesourceToSpawnRatio) {
		for (int32 i = 0; i < georesourceToSpawnRatio.size(); i++) {
			int32 count = georesourceToSpawnRatio[i].second * totalUsableProvinces / totalRatio;
			PUN_LOG("createGeoresourceToFill %s count:%d", *(GetGeoresourceInfo(georesourceToSpawnRatio[i].first).name.ToString()), count);
			for (int32 j = 0; j < count; j++) {
				georesourceToFill.push_back(georesourceToSpawnRatio[i].first);
			}
		}
	};
	
	std::vector<GeoresourceEnum> mountainGeoresourceToFill;
	createGeoresourceToFill(mountainGeoresourceToFill, mountainGeoresourceToSpawnRatio);

	std::vector<GeoresourceEnum> jungleGeoresourceToFill;
	createGeoresourceToFill(jungleGeoresourceToFill, jungleGeoresourceToSpawnRatio);

	std::vector<GeoresourceEnum> forestGeoresourceToFill;
	createGeoresourceToFill(forestGeoresourceToFill, forestGeoresourceToSpawnRatio);
	
	std::vector<GeoresourceEnum> flatGeoresourceToFill;
	createGeoresourceToFill(flatGeoresourceToFill, flatGeoresourceToSpawnRatio);
	
	
	// Fill ores from heavy
	// ... Fill like this to make sure it is deterministic
	// - Start by filling Gemstone/Gold Ore/Oil first

	auto ensureAtLeastOneNode = [&](int32& fillIndex, GeoresourceEnum georesourceEnumIn, int32 depositAmount = 50000)
	{
		if (fillIndex == 0)
		{
			ExecuteRegionsWithJumpAndExit([&](int32 provinceId)
			{
				if (usableFlatProvinceIds.Contains(provinceId) &&
					usableFlatProvinceIds[provinceId]) {
					usableFlatProvinceIds[provinceId] = false;
					PlantResource(provinceId, georesourceEnumIn, depositAmount);
					fillIndex++;
					return true;
				}
				if (usableLightMountainProvinceIds.Contains(provinceId) &&
					usableLightMountainProvinceIds[provinceId]) {
					usableLightMountainProvinceIds[provinceId] = false;
					PlantResource(provinceId, georesourceEnumIn, depositAmount);
					fillIndex++;
					return true;
				}
				if (usableHeavyMountainProvinceIds.Contains(provinceId) &&
					usableHeavyMountainProvinceIds[provinceId]) {
					usableHeavyMountainProvinceIds[provinceId] = false;
					PlantResource(provinceId, georesourceEnumIn, depositAmount);
					fillIndex++;
					return true;
				}
				return false;
			});
			PUN_CHECK(fillIndex == 1);
		}
	};

	// Desert Gem
	int32 totalFill = 0;
	int32 fillIndex = 0;
	int32 gemCount = pairGem.second * totalUsableProvinces / totalRatio;
	ExecuteRegionsWithJumpAndExit([&](int32 provinceId) {
		if (_simulation->GetBiomeProvince(provinceId) == BiomeEnum::Desert &&
			usableHeavyMountainProvinceIds.Contains(provinceId) &&
			usableHeavyMountainProvinceIds[provinceId])
		{
			usableHeavyMountainProvinceIds[provinceId] = false;
			PlantResource(provinceId, GeoresourceEnum::Gemstone, 50000);
			fillIndex++;
		}
		return fillIndex >= gemCount;
	});
	ExecuteRegionsWithJumpAndExit([&](int32 provinceId) {
		if (_simulation->GetBiomeProvince(provinceId) == BiomeEnum::Desert &&
			usableLightMountainProvinceIds.Contains(provinceId) &&
			usableLightMountainProvinceIds[provinceId])
		{
			usableLightMountainProvinceIds[provinceId] = false;
			PlantResource(provinceId, GeoresourceEnum::Gemstone, 50000);
			fillIndex++;
		}
		return fillIndex >= gemCount;
	});
	ensureAtLeastOneNode(fillIndex, GeoresourceEnum::Gemstone);

	totalFill += fillIndex;
	PUN_LOG("totalFill:%d fillIndex:%d", totalFill, fillIndex);

	// Gold (Tundra/Boreal/Desert)
	fillIndex = 0;
	int32 goldCount = pairGoldOre.second * totalUsableProvinces / totalRatio;
	ExecuteRegionsWithJumpAndExit([&](int32 provinceId) {
		BiomeEnum biomeEnum = _simulation->GetBiomeProvince(provinceId);
		if ((biomeEnum == BiomeEnum::Tundra || biomeEnum == BiomeEnum::BorealForest || biomeEnum == BiomeEnum::Desert) &&
			usableHeavyMountainProvinceIds.Contains(provinceId) &&
			usableHeavyMountainProvinceIds[provinceId])
		{
			usableHeavyMountainProvinceIds[provinceId] = false;
			PlantResource(provinceId, GeoresourceEnum::GoldOre, 50000);
			fillIndex++;
		}
		return fillIndex >= goldCount;
	});
	ExecuteRegionsWithJumpAndExit([&](int32 provinceId) {
		BiomeEnum biomeEnum = _simulation->GetBiomeProvince(provinceId);
		if ((biomeEnum == BiomeEnum::Tundra || biomeEnum == BiomeEnum::BorealForest || biomeEnum == BiomeEnum::Desert) &&
			usableLightMountainProvinceIds.Contains(provinceId) &&
			usableLightMountainProvinceIds[provinceId])
		{
			usableLightMountainProvinceIds[provinceId] = false;
			PlantResource(provinceId, GeoresourceEnum::GoldOre, 50000);
			fillIndex++;
		}
		return fillIndex >= goldCount;
	});
	ensureAtLeastOneNode(fillIndex, GeoresourceEnum::GoldOre);

	totalFill += fillIndex;
	PUN_LOG("totalFill:%d fillIndex:%d", totalFill, fillIndex);

	// Oil (Tundra/Boreal/Desert)
	fillIndex = 0;
	int32 oilCount = pairOil.second * totalUsableProvinces / totalRatio;
	ExecuteRegionsWithJumpAndExit([&](int32 provinceId) {
		BiomeEnum biomeEnum = _simulation->GetBiomeProvince(provinceId);
		if (biomeEnum == BiomeEnum::Tundra || 
			biomeEnum == BiomeEnum::BorealForest || 
			biomeEnum == BiomeEnum::Desert)
		{
			int32 oilDepositAmount = (biomeEnum == BiomeEnum::BorealForest) ? 20000 : 50000;
			
			if (usableFlatProvinceIds.Contains(provinceId) &&
				usableFlatProvinceIds[provinceId]) {
				usableFlatProvinceIds[provinceId] = false;
				PlantResource(provinceId, GeoresourceEnum::Oil, oilDepositAmount);
				fillIndex++;
			}
			if (usableLightMountainProvinceIds.Contains(provinceId) &&
				usableLightMountainProvinceIds[provinceId]) {
				usableLightMountainProvinceIds[provinceId] = false;
				PlantResource(provinceId, GeoresourceEnum::Oil, oilDepositAmount);
				fillIndex++;
			}
			if (usableHeavyMountainProvinceIds.Contains(provinceId) &&
				usableHeavyMountainProvinceIds[provinceId]) {
				usableHeavyMountainProvinceIds[provinceId] = false;
				PlantResource(provinceId, GeoresourceEnum::Oil, oilDepositAmount);
				fillIndex++;
			}
		}
		return fillIndex >= oilCount;
	});
	ensureAtLeastOneNode(fillIndex, GeoresourceEnum::Oil);

	PUN_CHECK(fillIndex > 0);
	totalFill += fillIndex;
	PUN_LOG("totalFill:%d fillIndex:%d", totalFill, fillIndex);

	/*
	 * Farms
	 */
	auto fillFlatGeoResourcesBase = [&](std::vector<GeoresourceEnum>& georesourceToFill, bool useAnyBiome, BiomeEnum biomeEnumIn)
	{
		PUN_LOG(".. georesourceToFill:%d biome:%s", georesourceToFill.size(), (useAnyBiome ? TEXT("None"): *GetBiomeInfo(biomeEnumIn).name.ToString()));
		ExecuteRegionsWithJumpAndExit([&](int32 provinceId) {
			BiomeEnum biomeEnum = _simulation->GetBiomeProvince(provinceId);
			bool isCorrectBiome = useAnyBiome ? (biomeEnum == BiomeEnum::Jungle || biomeEnum == BiomeEnum::Forest) : biomeEnum == biomeEnumIn;

			if (isCorrectBiome &&
				usableFlatProvinceIds.Contains(provinceId) &&
				usableFlatProvinceIds[provinceId]) {
				usableFlatProvinceIds[provinceId] = false;
				PUN_LOG("Flat fillFlatGeoResources_Flat %d fillIndex:%d", provinceId, fillIndex);
				PlantResource(provinceId, georesourceToFill[fillIndex], 50000);
				fillIndex++;
			}
			return fillIndex >= georesourceToFill.size();
		});
		ExecuteRegionsWithJumpAndExit([&](int32 provinceId) {
			BiomeEnum biomeEnum = _simulation->GetBiomeProvince(provinceId);
			bool isCorrectBiome = useAnyBiome ? (biomeEnum == BiomeEnum::Jungle || biomeEnum == BiomeEnum::Forest) : biomeEnum == biomeEnumIn;

			if (isCorrectBiome &&
				usableLightMountainProvinceIds.Contains(provinceId) &&
				usableLightMountainProvinceIds[provinceId]) {
				usableLightMountainProvinceIds[provinceId] = false;
				PUN_LOG("Light fillFlatGeoResources_Light %d fillIndex:%d", provinceId, fillIndex);
				PlantResource(provinceId, georesourceToFill[fillIndex], 50000);
				fillIndex++;
			}
			return fillIndex >= georesourceToFill.size();
		});
		ExecuteRegionsWithJumpAndExit([&](int32 provinceId) {
			BiomeEnum biomeEnum = _simulation->GetBiomeProvince(provinceId);
			bool isCorrectBiome = useAnyBiome ? (biomeEnum == BiomeEnum::Jungle || biomeEnum == BiomeEnum::Forest) : biomeEnum == biomeEnumIn;
			
			if (isCorrectBiome &&
				usableHeavyMountainProvinceIds.Contains(provinceId) &&
				usableHeavyMountainProvinceIds[provinceId]) {
				usableHeavyMountainProvinceIds[provinceId] = false;
				PUN_LOG("Heavy fillFlatGeoResources_Heavy %d fillIndex:%d", provinceId, fillIndex);
				PlantResource(provinceId, georesourceToFill[fillIndex], 50000);
				fillIndex++;
			}
			return fillIndex >= georesourceToFill.size();
		});
	};

	auto fillFlatGeoResources = [&](std::vector<GeoresourceEnum>& georesourceToFill, bool useAnyBiome, BiomeEnum biomeEnumIn)
	{
		PUN_LOG("!! fillFlatGeoResourcesBase");
		fillIndex = 0;
		
		fillFlatGeoResourcesBase(georesourceToFill, useAnyBiome, biomeEnumIn);
		
		if (!useAnyBiome) {
			fillFlatGeoResourcesBase(georesourceToFill, true, biomeEnumIn);
		}

		totalFill += fillIndex;
		PUN_LOG("totalFill:%d fillIndex:%d", totalFill, fillIndex);
	};
	

	fillFlatGeoResources(jungleGeoresourceToFill, false, BiomeEnum::Jungle);
	fillFlatGeoResources(forestGeoresourceToFill, false, BiomeEnum::Forest);
	fillFlatGeoResources(flatGeoresourceToFill, true, BiomeEnum::Forest);


	/*
	 * All other minerals
	 *  Comes after so it is more likely to fill into places that aren't fertile
	 */
	fillIndex = 0;
	ExecuteRegionsWithJumpAndExit([&](int32 provinceId) {
		if (usableHeavyMountainProvinceIds.Contains(provinceId) &&
			usableHeavyMountainProvinceIds[provinceId])
		{
			usableHeavyMountainProvinceIds[provinceId] = false;
			PlantResource(provinceId, mountainGeoresourceToFill[fillIndex], 50000);
			fillIndex++;
		}
		return fillIndex >= mountainGeoresourceToFill.size();
	});
	ExecuteRegionsWithJumpAndExit([&](int32 provinceId) {
		if (usableLightMountainProvinceIds.Contains(provinceId) &&
			usableLightMountainProvinceIds[provinceId])
		{
			usableLightMountainProvinceIds[provinceId] = false;
			PlantResource(provinceId, mountainGeoresourceToFill[fillIndex], 50000);
			fillIndex++;
		}
		return fillIndex >= mountainGeoresourceToFill.size();
	});
	totalFill += fillIndex;
	PUN_LOG("ORE totalFill:%d fillIndex:%d", totalFill, fillIndex);

	

	// Fill leftover Farms
	fillFlatGeoResources(jungleGeoresourceToFill, false, BiomeEnum::Jungle);
	fillFlatGeoResources(forestGeoresourceToFill, false, BiomeEnum::Forest);
	fillFlatGeoResources(flatGeoresourceToFill, true, BiomeEnum::Forest);

	// LAST farm
	fillIndex = 0;
	ExecuteRegionsWithJumpAndExit([&](int32 provinceId) {
		if (usableFlatProvinceIds.Contains(provinceId) &&
			usableFlatProvinceIds[provinceId])
		{
			usableFlatProvinceIds[provinceId] = false;
			PlantResource(provinceId, GeoresourceEnum::DyeFarm, 50000);
			fillIndex++;
		}
		return false;
	});
	totalFill += fillIndex;
	PUN_LOG("LAST FARM totalFill:%d fillIndex:%d", totalFill, fillIndex);

	// LAST ore
	ExecuteRegionsWithJumpAndExit([&](int32 provinceId) {
		if (usableLightMountainProvinceIds.Contains(provinceId) &&
			usableLightMountainProvinceIds[provinceId])
		{
			usableLightMountainProvinceIds[provinceId] = false;
			PlantResource(provinceId, GeoresourceEnum::CoalOre, 50000);
			fillIndex++;
		}
		return false;
	});
	totalFill += fillIndex;
	PUN_LOG("LAST ORE totalFill:%d fillIndex:%d", totalFill, fillIndex);
	

	PUN_LOG("totalFill:%d usableHeavyMountainProvinceIds:%d usableLightMountainProvinceIds:%d usableFarmProvinceIds:%d",
			totalFill, usableHeavyMountainProvinceIds.Num(), usableLightMountainProvinceIds.Num(), usableFlatProvinceIds.Num()
	);
	
	return;

	//// Nested helpers
	//// Note that Province and WorldRegion2 has similar size, Just that provinceId isn't always valid
	//auto hasNearbyGeoresource = [&](int32 provinceId)
	//{
	//	return provinceSys.ExecuteAdjacentProvincesWithExitTrue(provinceId, [&](const ProvinceConnection& connection) -> bool {
	//		return _provinceToGeoresource[connection.provinceId].HasResource();
	//	});
	//};

	// Note: IronOre/Gold Ore also spawns coal next to it

	//for (int i = 0; i < GameMapConstants::TotalRegions; i++)
	//{
	//	int32 provinceId = i;

	//	if (!provinceSys.IsProvinceValid(provinceId)) {
	//		continue;
	//	}

	//	if (provinceSys.provinceFlatTileCount(provinceId) < CoordinateConstants::TileIdsPerRegion / 2) {
	//		continue;
	//	}

	//	if (hasNearbyGeoresource(provinceId)) {
	//		continue;
	//	}

	//	WorldTile2 provinceCenter = provinceSys.GetProvinceCenterTile(provinceId);
	//	
	//	BiomeEnum biomeEnum = terrainGenerator.GetBiome(provinceCenter);
	//	
	//	// Mountain Ore
	//	if (provinceSys.CanPlantMountainGeoresource(provinceId))
	//	{
	//		if (GameRand::RandChance(2)) 
	//		{
	//			int32 desertCount = 0;
	//			int32 taigaTundraCount = 0;
	//			int32 connectionCount = 0;

	//			provinceSys.ExecuteAdjacentProvinces(provinceId, [&](ProvinceConnection connection)
	//			{
	//				BiomeEnum biome = simulation->GetBiomeProvince(connection.provinceId);
	//				if (biome == BiomeEnum::Desert) {
	//					desertCount++;
	//				}
	//				else if (biome == BiomeEnum::BorealForest ||
	//					biome == BiomeEnum::Tundra)
	//				{
	//					taigaTundraCount++;
	//				}

	//				connectionCount++;
	//			});

	//			connectionCount = max(1, connectionCount);
	//			int32 desertPercent = 100 * desertCount / connectionCount;
	//			int32 taigaTundraPercent = 100 * taigaTundraCount / connectionCount;
	//			
	//			if (desertPercent >= 80) {
	//				if (GameRand::RandChance(3)) {
	//					PlantResource(provinceId, GeoresourceEnum::GoldOre, 16000);
	//				}
	//				else if (GameRand::RandChance(3)) {
	//					PlantResource(provinceId, GeoresourceEnum::Oil, 50000);
	//				}
	//				PlantResource(provinceId, GeoresourceEnum::Gemstone, 16000);
	//				continue;
	//			}
	//			if (desertPercent >= 60) {
	//				PlantResource(provinceId, GeoresourceEnum::GoldOre, 16000);
	//				continue;
	//			}
	//			if (taigaTundraPercent >= 80) {
	//				if (GameRand::RandChance(2)) {
	//					PlantResource(provinceId, GeoresourceEnum::GoldOre, 16000);
	//				}
	//				else if (GameRand::RandChance(3)) {
	//					PlantResource(provinceId, GeoresourceEnum::Oil, 50000);
	//				}
	//				continue;
	//			}
	//		}
	//		
	//		if (GameRand::RandChance(3)) {
	//			PlantResource(provinceId, GeoresourceEnum::CoalOre, 16000);

	//			const std::vector<ProvinceConnection>& connections = provinceSys.GetProvinceConnections(provinceId);
	//			
	//			// Plant adjacent resource
	//			for (ProvinceConnection connection : connections) 
	//			{
	//				bool canPlantResource = provinceSys.CanPlantMountainGeoresource(connection.provinceId);
	//				if (canPlantResource) {
	//					PlantResource(connection.provinceId, GeoresourceEnum::IronOre, 24000);
	//					break;
	//				}
	//			}
	//			
	//			continue;
	//		}
	//		if (GameRand::RandChance(15)) {
	//			PlantResource(provinceId, GeoresourceEnum::CoalOre, 40000);
	//			continue;
	//		}

	//		if (GameRand::RandChance(15)) {
	//			PlantResource(provinceId, GeoresourceEnum::Oil, 30000);
	//			continue;
	//		}
	//		//if (GameRand::RandChance(15)) {
	//		//	PlantResource(region, GeoresourceEnum::IronOre, 3000);
	//		//	continue;
	//		//}
	//	}

	//	// Plantation
	//	if (provinceSys.CanPlantFarmGeoresource(provinceId))
	//	{
	//		if (biomeEnum == BiomeEnum::Jungle) {
	//			if (GameRand::RandChance(4)) {
	//				PlantResource(provinceId, GeoresourceEnum::CocoaFarm, 0);
	//				continue;
	//			}
	//		}
	//		
	//		if (biomeEnum == BiomeEnum::Jungle || 
	//			biomeEnum == BiomeEnum::Forest) 
	//		{
	//			if (GameRand::RandChance(12)) {
	//				PlantResource(provinceId, GeoresourceEnum::CannabisFarm, 0);
	//				continue;
	//			}

	//			if (GameRand::RandChance(12)) {
	//				PlantResource(provinceId, GeoresourceEnum::CottonFarm, 0);
	//				continue;
	//			}
	//			if (GameRand::RandChance(12)) {
	//				PlantResource(provinceId, GeoresourceEnum::DyeFarm, 0);
	//				continue;
	//			}
	//			
	//			if (GameRand::RandChance(20)) {
	//				PlantResource(provinceId, GeoresourceEnum::CoffeeFarm, 0);
	//				continue;
	//			}
	//		}

	//		if (biomeEnum == BiomeEnum::Forest)
	//		{
	//			if (GameRand::RandChance(12)) {
	//				PlantResource(provinceId, GeoresourceEnum::GrapeFarm, 0);
	//				continue;
	//			}
	//			if (GameRand::RandChance(20)) {
	//				PlantResource(provinceId, GeoresourceEnum::TulipFarm, 0);
	//				continue;
	//			}
	//		}
	//	}
	//	
	//	//
	//	if (isGeoresourceBuildable(provinceId))
	//	{
	//		//if (biomeEnum == BiomeEnum::Forest && GameRand::RandChance(100)) {
	//		//	PlantResource(region, GeoresourceEnum::GiantTree);
	//		//	continue;
	//		//}
	//		//if (biomeEnum == BiomeEnum::Forest && GameRand::RandChance(50)) {
	//		//	PlantResource(provinceId, GeoresourceEnum::GiantMushroom);
	//		//	continue;
	//		//}
	//		//if (biomeEnum == BiomeEnum::Forest && GameRand::RandChance(50)) {
	//		//	PlantResource(region, GeoresourceEnum::CherryBlossom);
	//		//	continue;
	//		//}
	//		//if (GameRand::RandChance(500)) {
	//		//	PlantResource(region, GeoresourceEnum::Hotspring);
	//		//	continue;
	//		//}
	//		//if (GameRand::RandChance(50)) {
	//		//	PlantResource(region, GeoresourceEnum::Ruin);
	//		//	continue;
	//		//}
	//		
	//		//TerrainRegionInfo terrainInfo = terrainGenerator.GetTerrainRegionInfo(region);

	//	}

	//}


}

void GeoresourceSystem::PlantResource(int32 provinceId, GeoresourceEnum georesourceEnum, int32 depositAmount)
{
	PUN_LOG(" - PlantResource %d %s", provinceId, *(GetGeoresourceInfo(georesourceEnum).name.ToString()));
	
	if (IsFarmGeoresource(georesourceEnum)) {
		depositAmount = 0;
	}
	
	TreeSystem& treeSystem = _simulation->treeSystem();
	
	// Put resource in the middle of the region
	WorldTile2 size(8, 8);
	//WorldTile2 tile(region.minXTile() + CoordinateConstants::TilesPerRegion / 2 - (size.x - 1) / 2, 
	//				region.minYTile() + CoordinateConstants::TilesPerRegion / 2 - (size.y - 1) / 2);

	ProvinceSystem& provinceSys = _simulation->provinceSystem();
	WorldTile2 centerTile = provinceSys.GetProvinceCenter(provinceId).worldTile2();

	TileArea georesourceArea = BuildingArea(centerTile, size, Direction::S);

	GeoresourceNode node = GeoresourceNode::Create(georesourceEnum, provinceId, centerTile, georesourceArea);
	


	if (node.info().isLandmark()) 
	{
		// Make sure we can plant this
		bool canPlant = true;
		node.area.ExecuteOnArea_WorldTile2([&](WorldTile2 areaTile) {
			if (!_simulation->IsBuildable(areaTile)) {
				canPlant = false;
			}
		});
		
		if (canPlant)
		{
			_provinceToGeoresource[provinceId] = node;
			_georesourcesProvinceIds.push_back(provinceId);
			
			//georesourceArea.ExecuteOnArea_Tile([&](int16_t x, int16_t y) {
			//	WorldTile2 curTile(x, y);
			//	if (WorldTile2::Distance(curTile, node.region.centerTile()) <= 6) treeSystem.ForceRemoveTileObj(curTile);
			//});

			node.area.ExecuteOnArea_WorldTile2([&](WorldTile2 curTile) {
				_simulation->SetWalkableSkipFlood(curTile, false);
				//GameMap::SetTerrainTile(curTile, TerrainTileType::Georesource);
			});
		}
	}
	else
	{
		_provinceToGeoresource[provinceId] = node;
		_georesourcesProvinceIds.push_back(provinceId);
	}

	if (depositAmount > 0)
	{
		int32 fluctuationPercent = 50;
		int32 fluctuationAmount = depositAmount * fluctuationPercent / 100;
		_provinceToGeoresource[provinceId].depositAmount = depositAmount + GameRand::Rand(centerTile.tileId()) % fluctuationAmount - fluctuationAmount / 2;
	}

	// Special case Iron/Gold ... Also spawn Coal
	if (georesourceEnum == GeoresourceEnum::GoldOre ||
		georesourceEnum == GeoresourceEnum::IronOre)
	{
		const std::vector<ProvinceConnection>& connections = _simulation->GetProvinceConnections(provinceId);
		for (const ProvinceConnection& connection : connections) {
			if (_simulation->IsProvinceValid(connection.provinceId) &&
				_simulation->provinceSystem().provinceMountainTileCount(connection.provinceId) > CoordinateConstants::TileIdsPerRegion / 3 &&
				_provinceToGeoresource[connection.provinceId].georesourceEnum == GeoresourceEnum::None)
			{
				PlantResource(connection.provinceId, GeoresourceEnum::CoalOre, 50000);
				break;
			}
		}
	}

	//if (canPlant)
	//{
	//	_regionToGeoresource[region.regionId()] = node;
	//	_georesourcesRegions.push_back(region);

	//	if (!node.info().isLandmark())
	//	{
	//		georesourceArea.ExecuteOnArea_Tile([&](int16_t x, int16_t y) {
	//			WorldTile2 curTile(x, y);
	//			if (WorldTile2::Distance(curTile, node.region.centerTile()) <= 6) treeSystem.ForceRemoveTileObj(curTile);
	//		});

	//		node.area.ExecuteOnArea_WorldTile2([&](WorldTile2 curTile) {
	//			_simulation->SetWalkableSkipFlood(curTile, false);
	//			GameMap::SetTerrainTile(curTile, TerrainTileType::Georesource);
	//		});
	//	}
	//}
}