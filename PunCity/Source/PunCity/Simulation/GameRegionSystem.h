// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "GameSimulationConstants.h"
#include "IGameSimulationCore.h"
#include "ProvinceSystem.h"
#include "PunCity/DisplaySystem/PunTerrainGenerator.h"

struct AnimalColony
{
	UnitEnum unitEnum = UnitEnum::Human;
	int32 provinceId = -1;
	WorldTile2 center = WorldTile2::Invalid;
	int32 radius = -1;
	std::vector<UnitFullId> units;

	void operator>>(FArchive& Ar)
	{
		Ar << unitEnum;
		Ar << provinceId;
		center >> Ar;
		Ar << radius;
		SerializeVecObj(Ar, units);
	}
};

struct ProvinceBuildingSlot
{
	int32 provinceId = -1;

	std::vector<WorldTile2> coastalTiles;
	std::vector<WorldTile2> mountainTiles;
	
	WorldTile2 portSlot = WorldTile2::Invalid;
	WorldTile2 landSlot = WorldTile2::Invalid;
	Direction portSlotFaceDirection = Direction::E;
	Direction landSlotFaceDirection = Direction::E;

	int32 townId = -1;

	bool isValid() const { return provinceId != -1; }
	
	void operator>>(FArchive& Ar)
	{
		Ar << provinceId;

		SerializeVecObj(Ar, coastalTiles);
		SerializeVecObj(Ar, mountainTiles);
		
		portSlot >> Ar;
		landSlot >> Ar;
		Ar << portSlotFaceDirection;
		Ar << landSlotFaceDirection;
		
		Ar << townId;
	}
};

struct ProvinceOwnerInfo
{
	int32 provinceId = -1;
	int32 townId = -1;
	int32 distanceFromTownhall = MAX_int32;
	bool isSafe = false;
	
	std::vector<int32> fortIds;

	void operator>>(FArchive& Ar)
	{
		Ar << provinceId;
		Ar << townId;
		Ar << distanceFromTownhall;
		Ar << isSafe;
		
		SerializeVecValue(Ar, fortIds);
	}
};


/**
 * 
 */
class GameRegionSystem
{
public:
	void InitProvinceInfoSystem(IGameSimulationCore* simulation)
	{
		_simulation = simulation;

		provinceToTreeCountCache.resize(GameMapConstants::TotalRegions, -1);
		
		_provinceOwnerInfos.resize(GameMapConstants::TotalRegions);
		
		_boarBurrowsToProvince.resize(GameMapConstants::TotalRegions);
		_provinceToAnimalIds.resize(GameMapConstants::TotalRegions);

		_provinceBuildingSlots.resize(GameMapConstants::TotalRegions);

		InitProvinceBuildingSlots();
	}

	/*
	 * Province Building Slot
	 */
	
	void InitProvinceBuildingSlots()
	{
		ProvinceSystem& provinceSys = _simulation->provinceSystem();
		PunTerrainGenerator& terrainGen = _simulation->terrainGenerator();

		for (int32 provinceId = 0; provinceId < GameMapConstants::TotalRegions; provinceId++)
		{
			if (provinceSys.IsProvinceValid(provinceId))
			{
				// _provinceOwnerInfos init
				_provinceOwnerInfos[provinceId].provinceId = provinceId;
				
				ProvinceBuildingSlot& provinceBuildingSlot = _provinceBuildingSlots[provinceId];

				provinceBuildingSlot.provinceId = provinceId;

				auto isBuildable = [&](WorldTile2 tile) {
					return terrainGen.terrainTileType(tile) == TerrainTileType::None &&
						_simulation->GetProvinceIdClean(tile) == provinceId;
				};
				
				
				// Fill Coastal Tiles
				if (provinceSys.provinceOceanTileCount(provinceId) > 1)
				{
					provinceSys.GetProvinceRectArea(provinceId).ExecuteOnArea_WorldTile2([&](WorldTile2 tile)
					{
						if (isBuildable(tile))
						{
							// Check found nearby tiles if any water
							auto checkAdjacentTile = [&](WorldTile2 shift) {
								if (terrainGen.terrainTileType(tile + shift) == TerrainTileType::Ocean) {
									provinceBuildingSlot.coastalTiles.push_back(tile + shift);
								}
							};
							checkAdjacentTile(WorldTile2(1, 0));
							checkAdjacentTile(WorldTile2(-1, 0));
							checkAdjacentTile(WorldTile2(0, 1));
							checkAdjacentTile(WorldTile2(0, -1));
						}
					});
				}

				// Fill Mountain Tiles
				if (provinceSys.provinceMountainTileCount(provinceId) > 50)
				{
					provinceSys.GetProvinceRectArea(provinceId).ExecuteOnArea_WorldTile2([&](WorldTile2 tile)
					{
						if (isBuildable(tile))
						{
							// Check found nearby tiles if any water
							auto checkAdjacentTile = [&](WorldTile2 shift) {
								if (terrainGen.terrainTileType(tile + shift) == TerrainTileType::Mountain) {
									provinceBuildingSlot.mountainTiles.push_back(tile + shift);
								}
							};
							checkAdjacentTile(WorldTile2(1, 0));
							checkAdjacentTile(WorldTile2(-1, 0));
							checkAdjacentTile(WorldTile2(0, 1));
							checkAdjacentTile(WorldTile2(0, -1));
						}
					});
				}

				/*
				 * Don't put next to each other
				 */
				const std::vector<ProvinceConnection>& connections = provinceSys.GetProvinceConnections(provinceId);
				bool hasNearbyFilledSlot = false;
				for (const ProvinceConnection& connection : connections) {
					if (_provinceBuildingSlots[connection.provinceId].landSlot.isValid()) {
						hasNearbyFilledSlot = true;
						break;
					}
				}
				
				if (hasNearbyFilledSlot) {
					continue;
				}
				

				/*
				 * Port Slot
				 */
				if (provinceBuildingSlot.coastalTiles.size() > 0)
				{
					auto findPortSlotTile = [&]()
					{
						for (WorldTile2 coastTile : provinceBuildingSlot.coastalTiles)
						{
							auto checkPortSlotAvailable = [&](WorldTile2 centerTile, Direction faceDirection)
							{
								WorldTile2 portSize = GetBuildingInfo(CardEnum::TradingPort).size;
								TileArea portArea = BuildingArea(centerTile, portSize, faceDirection);

								bool setDockInstruct;
								std::vector<PlacementGridInfo> grids;
								_simulation->CheckPortArea(portArea, faceDirection, CardEnum::TradingPort, grids, setDockInstruct, -1, 10);

								// Main Port Area not buildable
								for (PlacementGridInfo& gridInfo : grids) {
									if (gridInfo.gridEnum == PlacementGridEnum::Red) {
										return false;
									}
								}

								bool isPortMainNotInProvince = portArea.ExecuteOnAreaWithExit_WorldTile2([&](WorldTile2 tile) {
									return _simulation->GetProvinceIdClean(tile) != provinceId; // Should exit
								});
								if (isPortMainNotInProvince) {
									return false;
								}
								
								bool isPortFrontNotBuildable = portArea.GetFrontArea(faceDirection).ExecuteOnAreaWithExit_WorldTile2([&](WorldTile2 tile) {
									return !isBuildable(tile); // Should exit
								});
								if (isPortFrontNotBuildable) {
									return false;
								}


								// Also need to be able to add the main Minor City Building
								WorldTile2 minorCitySize = GetBuildingInfo(CardEnum::MinorCity).size;
								int32 shift = (portSize.x / 2 + 1) + minorCitySize.x / 2;
								WorldTile2 minorCityCenterTile = centerTile + WorldTile2::DirectionTile(faceDirection) * shift;
								TileArea minorCityArea = BuildingArea(minorCityCenterTile, minorCitySize, OppositeDirection(faceDirection));

								bool isCityMainNotBuildable = minorCityArea.GetExpandedArea().ExecuteOnAreaWithExit_WorldTile2([&](WorldTile2 tile) {
									return !isBuildable(tile); // Should exit
								});
								if (isCityMainNotBuildable) {
									return false;
								}

								provinceBuildingSlot.portSlot = centerTile;
								provinceBuildingSlot.portSlotFaceDirection = faceDirection;
								_portSlotProvinceIds.push_back(provinceId);

								provinceBuildingSlot.landSlot = minorCityCenterTile;
								provinceBuildingSlot.landSlotFaceDirection = faceDirection;
								_landSlotProvinceIds.push_back(provinceId);

								return true;
							};

							// Center Tile
							if (checkPortSlotAvailable(coastTile, Direction::N)) return;
							if (checkPortSlotAvailable(coastTile, Direction::S)) return;
							if (checkPortSlotAvailable(coastTile, Direction::E)) return;
							if (checkPortSlotAvailable(coastTile, Direction::W)) return;

							// Surrounding tiles
							for (int32 i = 1; i <= 2; i++) {
								if (checkPortSlotAvailable(coastTile + WorldTile2(i, 0), Direction::N)) return;
								if (checkPortSlotAvailable(coastTile + WorldTile2(i, 0), Direction::S)) return;
								if (checkPortSlotAvailable(coastTile + WorldTile2(-i, 0), Direction::N)) return;
								if (checkPortSlotAvailable(coastTile + WorldTile2(-i, 0), Direction::S)) return;
								
								if (checkPortSlotAvailable(coastTile + WorldTile2(0, i), Direction::E)) return;
								if (checkPortSlotAvailable(coastTile + WorldTile2(0, i), Direction::W)) return;
								if (checkPortSlotAvailable(coastTile + WorldTile2(0, -i), Direction::E)) return;
								if (checkPortSlotAvailable(coastTile + WorldTile2(0, -i), Direction::W)) return;
							}
						}
					};

					findPortSlotTile();
				}

				/*
				 * Land Slot
				 */
				if (!provinceBuildingSlot.portSlot.isValid())
				{
					WorldTile2 centerTile = provinceSys.GetProvinceCenterTile(provinceId);
					
					// Also need to be able to add the main Minor City Building
					WorldTile2 minorCitySize = GetBuildingInfo(CardEnum::MinorCity).size;
					Direction faceDirection = static_cast<Direction>(GameRand::Rand(provinceId) % 4);
					TileArea minorCityArea = BuildingArea(centerTile, minorCitySize, faceDirection);

					bool isCityNotBuildable = minorCityArea.GetExpandedArea().ExecuteOnAreaWithExit_WorldTile2([&](WorldTile2 tile) {
						return !isBuildable(tile); // Should exit
					});
					if (!isCityNotBuildable)
					{
						provinceBuildingSlot.landSlot = centerTile;
						provinceBuildingSlot.landSlotFaceDirection = faceDirection;
						_landSlotProvinceIds.push_back(provinceId);
					}
				}
			}
		}
	}

	const ProvinceBuildingSlot& provinceBuildingSlot(int32 provinceId) {
		return _provinceBuildingSlots[provinceId];;
	}

	const ProvinceOwnerInfo& provinceOwnerInfo(int32 provinceId) {
		return _provinceOwnerInfos[provinceId];
	}

	const std::vector<int32>& portSlotProvinceIds() { return _portSlotProvinceIds; }
	const std::vector<int32>& landSlotProvinceIds() { return _landSlotProvinceIds; }

	void SetSlotTownId(int32 provinceId, int32 townId) {
		check(_provinceBuildingSlots[provinceId].landSlot.isValid());
		_provinceBuildingSlots[provinceId].townId = townId;
	}

	/*
	 * Territory Owner
	 */

	void SetProvinceOwner(int32 provinceId, int32 townId, bool lightMode = false)
	{
		// Also update last owned town
		int32 lastTownId = _provinceOwnerInfos[provinceId].townId;
		if (lastTownId != -1) {
			_simulation->AddNeedDisplayUpdateId(DisplayGlobalEnum::Territory, lastTownId);
		}

		check(_provinceOwnerInfos[provinceId].provinceId == provinceId);
		_provinceOwnerInfos[provinceId].townId = townId;

		if (townId != -1) {
			RefreshFlood_IsSafe(provinceId);
		}

		if (PunSettings::TrailerMode()) {
			return;
		}
		if (lightMode) {
			return;
		}
		
		_simulation->SetNeedDisplayUpdate(DisplayGlobalEnum::Province, true);
		_simulation->AddNeedDisplayUpdateId(DisplayGlobalEnum::Province, provinceId);

		_simulation->SetNeedDisplayUpdate(DisplayGlobalEnum::MapDefenseNode, true);

		if (townId != -1) {
			_simulation->AddNeedDisplayUpdateId(DisplayGlobalEnum::Territory, townId, true);
			PUN_LOG("SetProvinceOwner province:%d pid:%d", provinceId, townId);
		}
	}
	
	int32 provinceOwner(int32 provinceId) { return _provinceOwnerInfos[provinceId].townId; }

	int32 provinceOwnerSafe(int32 provinceId)
	{
		if (_provinceOwnerInfos.size() > provinceId && provinceId >= 0) {
			return _provinceOwnerInfos[provinceId].townId;
		}
		return -1;
	}

	/*
	 * Province isSafe Flood
	 */

	void AddFortToProvince(int32 provinceId, int32 fortId)
	{
		check(!CppUtils::Contains(_provinceOwnerInfos[provinceId].fortIds, fortId));
		_provinceOwnerInfos[provinceId].fortIds.push_back(fortId);
		RefreshFlood_IsSafe(provinceId);
	}
	void TryRemoveFortFromProvince(int32 provinceId, int32 fortId)
	{
		CppUtils::TryRemove(_provinceOwnerInfos[provinceId].fortIds, fortId);
		check(!CppUtils::Contains(_provinceOwnerInfos[provinceId].fortIds, fortId));
		RefreshFlood_IsSafe(provinceId);
	}

	void RefreshFlood_IsSafe(int32 provinceIdIn)
	{
		int32 townId = _provinceOwnerInfos[provinceIdIn].townId;
		check(townId != -1);

		// Set all initially to isSafe
		// Find unsafe border provinces
		TSet<int32> unsafeBorderProvinceIds;
		
		std::vector<int32> provincesClaimed = _simulation->GetTownProvincesClaimed(townId);

		// provinceIdIn isn't added yet
		provincesClaimed.push_back(provinceIdIn);
		
		for (int32 provinceId : provincesClaimed) 
		{
			check(_provinceOwnerInfos[provinceId].townId == townId);
			_provinceOwnerInfos[provinceId].isSafe = true;

			if (_provinceOwnerInfos[provinceId].fortIds.size() == 0)
			{
				const auto& connections = _simulation->GetProvinceConnections(provinceId);
				for (const ProvinceConnection& connection : connections)
				{
					if (connection.tileType == TerrainTileType::None &&
						_provinceOwnerInfos[connection.provinceId].townId != townId)
					{
						unsafeBorderProvinceIds.Add(provinceId);
					}
				}
			}
		}

		// Flood from border provinces as far as possible
		TSet<int32> visitedProvinceIds;
		
		for (int32 unsafeBorderProvinceId : unsafeBorderProvinceIds)
		{
			std::function<void(int32)> flood = [&](int32 provinceId)
			{	
				if (!visitedProvinceIds.Contains(provinceId))  {
					visitedProvinceIds.Add(provinceId);

					if (_provinceOwnerInfos[provinceId].fortIds.size() > 0) {
						return;
					}
					_provinceOwnerInfos[provinceId].isSafe = false;
					PUN_LOG("Province Unsafe:%d", provinceId);

					// Flood to same town's flat or river connections
					const auto& connections = _simulation->GetProvinceConnections(provinceId);
					for (const ProvinceConnection& connection : connections) {
						if (connection.tileType == TerrainTileType::None ||
							connection.tileType == TerrainTileType::River)
						{
							if (_provinceOwnerInfos[connection.provinceId].townId == townId) {
								flood(connection.provinceId);
							}
						}
					}
				}
			};

			flood(unsafeBorderProvinceId);
		}
	}

	/*
	 * Province Distance Map
	 */
	// provinceDistanceMap does not take into account bridge etc.
	int32 provinceDistanceMap(int32 provinceId) { return _provinceOwnerInfos[provinceId].distanceFromTownhall; }
	void SetProvinceDistanceMap(int32 provinceId, int32 provinceDistance) {
		_provinceOwnerInfos[provinceId].distanceFromTownhall = provinceDistance;
	}

	int32 provinceDistanceToPlayer(int32 provinceId, int32 playerId, bool withShallowWater = false)
	{
		int32 cachedDist = provinceDistanceMap(provinceId);
		if (cachedDist != MAX_int32) {
			return cachedDist;
		}
		
		int32 minProvinceDistance = MAX_int32;
		const std::vector<ProvinceConnection>& connections = _simulation->GetProvinceConnections(provinceId);
		for (const ProvinceConnection& connection : connections) {
			if (connection.isConnectedTileType(withShallowWater) &&
				_simulation->provinceOwnerPlayer(connection.provinceId) == playerId)
			{
				int32 connectedProvinceDist = provinceDistanceMap(connection.provinceId);
				if (connectedProvinceDist != MAX_int32) {
					minProvinceDistance = std::min(minProvinceDistance, connectedProvinceDist + 1);
				}
			}
		}
		return minProvinceDistance;
	}
	
	/*
	 * Burrows
	 */

	const std::vector<int32>& boarBurrows(int32 provinceId) {
		return _boarBurrowsToProvince[provinceId];
	}
	void AddBoarBurrow(int32 provinceId, int32 buildingId) {
		_boarBurrowsToProvince[provinceId].push_back(buildingId);
	}
	void RemoveBoarBurrow(int32 provinceId, int32 buildingId);


	const std::vector<int32>& provinceAnimals(int32 provinceId) {
		return _provinceToAnimalIds[provinceId];
	}
	void AddProvinceAnimals(int32 provinceId, int32 animalId)
	{
		PUN_CHECK(_simulation->IsProvinceValid(provinceId));
		PUN_CHECK(!CppUtils::Contains(_provinceToAnimalIds[provinceId], animalId));
		
		_provinceToAnimalIds[provinceId].push_back(animalId);

#if !UE_BUILD_SHIPPING
		debugTotalProvinceAnimalCount++;
		if (_provinceToAnimalIds[provinceId].size() > debugMaxAnimalCount) {
			debugMaxAnimalCount = _provinceToAnimalIds[provinceId].size();
			debugMaxAnimalCountProvinceId = provinceId;
		}
#endif
	}
	void RemoveProvinceAnimals(int32 provinceId, int32 animalId)
	{
		PUN_CHECK(_simulation->IsProvinceValid(provinceId));
		
		CppUtils::Remove(_provinceToAnimalIds[provinceId], animalId);

		PUN_CHECK(!CppUtils::Contains(_provinceToAnimalIds[provinceId], animalId));

#if !UE_BUILD_SHIPPING
		debugTotalProvinceAnimalCount--;
		if (provinceId == debugMaxAnimalCountProvinceId) {
			debugMaxAnimalCount = _provinceToAnimalIds[provinceId].size();
		}
#endif
	}

	void AddAnimalColony(UnitEnum unitEnum, WorldTile2 center, int32 radius, int32 chancePercentMultiplier);
	void RemoveAnimalColony(UnitEnum unitEnum);
	


	void Serialize(FArchive &Ar)
	{
		SerializeVecValue(Ar, provinceToTreeCountCache);
		
		SerializeVecObj(Ar, _provinceOwnerInfos);
		
		SerializeVecVecValue(Ar, _boarBurrowsToProvince);
		SerializeVecVecValue(Ar, _provinceToAnimalIds);
		SerializeVecObj(Ar, _animalColonies);

		SerializeVecObj(Ar, _provinceBuildingSlots);
		SerializeVecValue(Ar, _portSlotProvinceIds);
		SerializeVecValue(Ar, _landSlotProvinceIds);
	}


	int32 debugMaxAnimalCount = 0;
	int32 debugMaxAnimalCountProvinceId = -1;
	int32 debugTotalProvinceAnimalCount = 0;

public:
	std::vector<int32> provinceToTreeCountCache;

private:
	IGameSimulationCore* _simulation = nullptr;

	std::vector<ProvinceOwnerInfo> _provinceOwnerInfos;
	
	std::vector<std::vector<int32>> _boarBurrowsToProvince;
	std::vector<std::vector<int32>> _provinceToAnimalIds;
	std::vector<AnimalColony> _animalColonies;

	std::vector<ProvinceBuildingSlot> _provinceBuildingSlots;
	std::vector<int32> _portSlotProvinceIds;
	std::vector<int32> _landSlotProvinceIds;
};