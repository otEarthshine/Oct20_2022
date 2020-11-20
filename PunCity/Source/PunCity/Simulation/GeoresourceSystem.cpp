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

	// Nested helpers
	// Note that Province and WorldRegion2 has similar size, Just that provinceId isn't always valid
	auto hasNearbyGeoresource = [&](int32 provinceId)
	{
		return provinceSys.ExecuteAdjacentProvincesWithExitTrue(provinceId, [&](const ProvinceConnection& connection) -> bool {
			return _provinceToGeoresource[connection.provinceId].HasResource();
		});
	};


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

	for (int i = 0; i < GameMapConstants::TotalRegions; i++)
	{
		int32 provinceId = i;

		if (!provinceSys.IsProvinceValid(provinceId)) {
			continue;
		}

		if (provinceSys.provinceFlatTileCount(provinceId) < CoordinateConstants::TileIdsPerRegion / 2) {
			continue;
		}

		if (hasNearbyGeoresource(provinceId)) {
			continue;
		}

		WorldTile2 provinceCenter = provinceSys.GetProvinceCenterTile(provinceId);
		
		BiomeEnum biomeEnum = terrainGenerator.GetBiome(provinceCenter);
		
		// Mountain Ore
		if (provinceSys.CanPlantMountainGeoresource(provinceId))
		{
			if (GameRand::RandChance(2)) 
			{
				int32 desertCount = 0;
				int32 taigaTundraCount = 0;
				int32 connectionCount = 0;

				provinceSys.ExecuteAdjacentProvinces(provinceId, [&](ProvinceConnection connection)
				{
					BiomeEnum biome = simulation->GetBiomeProvince(connection.provinceId);
					if (biome == BiomeEnum::Desert) {
						desertCount++;
					}
					else if (biome == BiomeEnum::BorealForest ||
						biome == BiomeEnum::Tundra)
					{
						taigaTundraCount++;
					}

					connectionCount++;
				});

				connectionCount = max(1, connectionCount);
				int32 desertPercent = 100 * desertCount / connectionCount;
				int32 taigaTundraPercent = 100 * taigaTundraCount / connectionCount;
				
				if (desertPercent >= 80) {
					if (GameRand::RandChance(3)) {
						PlantResource(provinceId, GeoresourceEnum::GoldOre, 16000);
					}
					PlantResource(provinceId, GeoresourceEnum::Gemstone, 16000);
					continue;
				}
				if (desertPercent >= 60) {
					PlantResource(provinceId, GeoresourceEnum::GoldOre, 16000);
					continue;
				}
				if (taigaTundraPercent >= 80) {
					if (GameRand::RandChance(2)) {
						PlantResource(provinceId, GeoresourceEnum::GoldOre, 16000);
					}
					continue;
				}
			}
			
			if (GameRand::RandChance(3)) {
				PlantResource(provinceId, GeoresourceEnum::CoalOre, 16000);

				const std::vector<ProvinceConnection>& connections = provinceSys.GetProvinceConnections(provinceId);
				
				// Plant adjacent resource
				for (ProvinceConnection connection : connections) 
				{
					bool canPlantResource = provinceSys.CanPlantMountainGeoresource(connection.provinceId);
					if (canPlantResource) {
						PlantResource(connection.provinceId, GeoresourceEnum::IronOre, 24000);
						break;
					}
				}
				
				continue;
			}
			if (GameRand::RandChance(15)) {
				PlantResource(provinceId, GeoresourceEnum::CoalOre, 40000);
				continue;
			}
			//if (GameRand::RandChance(15)) {
			//	PlantResource(region, GeoresourceEnum::IronOre, 3000);
			//	continue;
			//}
		}

		// Plantation
		if (provinceSys.CanPlantFarmGeoresource(provinceId))
		{
			if (biomeEnum == BiomeEnum::Jungle) {
				if (GameRand::RandChance(4)) {
					PlantResource(provinceId, GeoresourceEnum::CocoaFarm, 0);
					continue;
				}
			}
			
			if (biomeEnum == BiomeEnum::Jungle || 
				biomeEnum == BiomeEnum::Forest) 
			{
				if (GameRand::RandChance(10)) {
					PlantResource(provinceId, GeoresourceEnum::CannabisFarm, 0);
					continue;
				}
				if (GameRand::RandChance(10)) {
					PlantResource(provinceId, GeoresourceEnum::GrapeFarm, 0);
					continue;
				}

				if (GameRand::RandChance(10)) {
					PlantResource(provinceId, GeoresourceEnum::CottonFarm, 0);
					continue;
				}
				if (GameRand::RandChance(10)) {
					PlantResource(provinceId, GeoresourceEnum::DyeFarm, 0);
					continue;
				}
			}
		}
		
		//
		if (isGeoresourceBuildable(provinceId))
		{
			//if (biomeEnum == BiomeEnum::Forest && GameRand::RandChance(100)) {
			//	PlantResource(region, GeoresourceEnum::GiantTree);
			//	continue;
			//}
			//if (biomeEnum == BiomeEnum::Forest && GameRand::RandChance(50)) {
			//	PlantResource(provinceId, GeoresourceEnum::GiantMushroom);
			//	continue;
			//}
			//if (biomeEnum == BiomeEnum::Forest && GameRand::RandChance(50)) {
			//	PlantResource(region, GeoresourceEnum::CherryBlossom);
			//	continue;
			//}
			//if (GameRand::RandChance(500)) {
			//	PlantResource(region, GeoresourceEnum::Hotspring);
			//	continue;
			//}
			//if (GameRand::RandChance(50)) {
			//	PlantResource(region, GeoresourceEnum::Ruin);
			//	continue;
			//}
			
			//TerrainRegionInfo terrainInfo = terrainGenerator.GetTerrainRegionInfo(region);

		}

	}


}

void GeoresourceSystem::PlantResource(int32 provinceId, GeoresourceEnum georesourceEnum, int32 depositAmount)
{
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