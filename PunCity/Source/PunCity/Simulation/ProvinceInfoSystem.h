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
	std::vector<WorldTile2> riverTiles;
	std::vector<WorldTile2> mountainTiles;
	
	BuildPlacement portSlot; // Slots are center tiles
	BuildPlacement portLandSlot;
	BuildPlacement largeLandSlot;
	BuildPlacement oasisSlot;

	int32 buildingId = -1;

	bool isValid() const { return provinceId != -1; }

	bool hasBuilding() const { return buildingId != -1; }
	
	void operator>>(FArchive& Ar)
	{
		Ar << provinceId;

		SerializeVecObj(Ar, coastalTiles);
		SerializeVecObj(Ar, riverTiles);
		SerializeVecObj(Ar, mountainTiles);
		
		portSlot >> Ar;
		portLandSlot >> Ar;
		largeLandSlot >> Ar;
		oasisSlot >> Ar;
		
		Ar << buildingId;
	}
};

struct ProvinceOwnerInfo
{
	int32 provinceId = -1;
	int32 townId = -1;
	int32 distanceFromTownhall = MAX_int32;
	
	bool isSafe = false;
	int32 lastRaidedTick = 0;
	
	std::vector<int32> fortIds;

	void operator>>(FArchive& Ar)
	{
		Ar << provinceId;
		Ar << townId;
		Ar << distanceFromTownhall;
		
		Ar << isSafe;
		Ar << lastRaidedTick;
		
		SerializeVecValue(Ar, fortIds);
	}
};


/**
 * 
 */
class ProvinceInfoSystem
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
		if (_simulation->isLoadingFromFile()) {
			return;
		}
		
		ProvinceSystem& provinceSys = _simulation->provinceSystem();
		PunTerrainGenerator& terrainGen = _simulation->terrainGenerator();

		/*
		 * Fill Coastal Tiles
		 */

		for (int32 provinceId = 0; provinceId < GameMapConstants::TotalRegions; provinceId++)
		{
			if (provinceSys.IsProvinceValid(provinceId))
			{
				ProvinceBuildingSlot& provinceBuildingSlot = _provinceBuildingSlots[provinceId];

				provinceBuildingSlot.provinceId = provinceId;

				auto isBuildable = [&](WorldTile2 tile) {
					return terrainGen.terrainTileType(tile) == TerrainTileType::None &&
						_simulation->GetProvinceIdClean(tile) == provinceId;
				};

				auto tryAddSpecialTiles = [&](std::vector<WorldTile2>& tiles, TerrainTileType tileType)
				{
					provinceSys.GetProvinceRectArea(provinceId).ExecuteOnArea_WorldTile2([&](WorldTile2 tile)
					{
						if (isBuildable(tile))
						{
							// Check found nearby tiles if any water
							auto checkAdjacentTile = [&](WorldTile2 shift) {
								if (terrainGen.terrainTileType(tile + shift) == tileType) {
									tiles.push_back(tile + shift);
								}
							};
							checkAdjacentTile(WorldTile2(1, 0));
							checkAdjacentTile(WorldTile2(-1, 0));
							checkAdjacentTile(WorldTile2(0, 1));
							checkAdjacentTile(WorldTile2(0, -1));
						}
					});
				};

				// Fill Coastal Tiles
				if (provinceSys.provinceOceanTileCount(provinceId) > 1)
				{
					tryAddSpecialTiles(provinceBuildingSlot.coastalTiles, TerrainTileType::Ocean);
					
					//provinceSys.GetProvinceRectArea(provinceId).ExecuteOnArea_WorldTile2([&](WorldTile2 tile)
					//{
					//	if (isBuildable(tile))
					//	{
					//		// Check found nearby tiles if any water
					//		auto checkAdjacentTile = [&](WorldTile2 shift) {
					//			if (terrainGen.terrainTileType(tile + shift) == TerrainTileType::Ocean) {
					//				provinceBuildingSlot.coastalTiles.push_back(tile + shift);
					//			}
					//		};
					//		checkAdjacentTile(WorldTile2(1, 0));
					//		checkAdjacentTile(WorldTile2(-1, 0));
					//		checkAdjacentTile(WorldTile2(0, 1));
					//		checkAdjacentTile(WorldTile2(0, -1));
					//	}
					//});
				}

				// Fill River Tiles
				if (provinceSys.provinceRiverTileCount(provinceId) > 1)
				{
					tryAddSpecialTiles(provinceBuildingSlot.riverTiles, TerrainTileType::River);
				}

				// Fill Mountain Tiles
				if (provinceSys.provinceMountainTileCount(provinceId) > 50)
				{
					tryAddSpecialTiles(provinceBuildingSlot.mountainTiles, TerrainTileType::Mountain);
					
					//provinceSys.GetProvinceRectArea(provinceId).ExecuteOnArea_WorldTile2([&](WorldTile2 tile)
					//{
					//	if (isBuildable(tile))
					//	{
					//		// Check found nearby tiles if any water
					//		auto checkAdjacentTile = [&](WorldTile2 shift) {
					//			if (terrainGen.terrainTileType(tile + shift) == TerrainTileType::Mountain) {
					//				provinceBuildingSlot.mountainTiles.push_back(tile + shift);
					//			}
					//		};
					//		checkAdjacentTile(WorldTile2(1, 0));
					//		checkAdjacentTile(WorldTile2(-1, 0));
					//		checkAdjacentTile(WorldTile2(0, 1));
					//		checkAdjacentTile(WorldTile2(0, -1));
					//	}
					//});
				}
			}
		}
		

		/*
		 * Oasis
		 */
		auto trySpawnOasis = [&](int32 nearbyOasisDistance, int32 riverDistance)
		{
			for (int32 provinceId = 0; provinceId < GameMapConstants::TotalRegions; provinceId++)
			{
				if (provinceSys.IsProvinceValid(provinceId))
				{
					// _provinceOwnerInfos init
					_provinceOwnerInfos[provinceId].provinceId = provinceId;


					bool canSpawnOasis = [&]()
					{
						if (_simulation->GetBiomeProvince(provinceId) != BiomeEnum::Desert) {
							return false;
						}

						// Check if nearby provinces are desert
						{
							const std::vector<ProvinceConnection>& connections = provinceSys.GetProvinceConnections(provinceId);
							for (const ProvinceConnection& connection : connections) {
								if (_simulation->GetBiomeProvince(connection.provinceId) != BiomeEnum::Desert) {
									return false;
								}
							}
						}

						// Check for nearby Oasis
						TSet<int32> visitedProvinceIds;
						std::function<bool(int32, int32, TerrainTileType)> hasNearbyOasisHelper = [&](int32 provinceIdTemp, int32 distance, TerrainTileType connectionType)
						{
							if (distance > nearbyOasisDistance) {
								return false;
							}

							if (visitedProvinceIds.Contains(provinceIdTemp)) {
								return false;
							}
							visitedProvinceIds.Add(provinceIdTemp);

							if (distance <= riverDistance && connectionType == TerrainTileType::River) {
								return true;
							}
							if (_provinceBuildingSlots[provinceIdTemp].oasisSlot.isValid()) {
								return true;
							}
							const std::vector<ProvinceConnection>& connections = provinceSys.GetProvinceConnections(provinceIdTemp);
							for (const ProvinceConnection& connection : connections) {
								if (hasNearbyOasisHelper(connection.provinceId, distance + 1, connection.tileType)) {
									return true;
								}
							}
							return false;
						};

						return !hasNearbyOasisHelper(provinceId, 0, TerrainTileType::None);
					}();

					if (canSpawnOasis)
					{
						WorldTile2 centerTile = provinceSys.GetProvinceCenterTile(provinceId);

						// Also need to be able to add the main Minor City Building
						WorldTile2 oasisSize = GetBuildingInfo(CardEnum::Oasis).baseBuildingSize;
						Direction faceDirection = static_cast<Direction>(GameRand::Rand(provinceId) % 4);
						TileArea largeLandSlotArea = BuildingArea(centerTile, oasisSize, faceDirection);

						bool isLandSlotNotBuildable = largeLandSlotArea.GetExpandedArea().ExecuteOnAreaWithExit_WorldTile2([&](WorldTile2 tile) {
							return !(terrainGen.terrainTileType(tile) == TerrainTileType::None &&
								_simulation->GetProvinceIdClean(tile) != -1); // Should exit
						});
						if (!isLandSlotNotBuildable && _provinceBuildingSlots[provinceId].coastalTiles.size() < 8)
						{
							PUN_LOG("Spawn Oasis provinceId:%d coastalTiles:%d", provinceId, _provinceBuildingSlots[provinceId].coastalTiles.size());

							_provinceBuildingSlots[provinceId].oasisSlot = { centerTile, oasisSize, faceDirection };
							_oasisSlotProvinceIds.push_back(provinceId);
						}
					}
				}
			}
			
		};

		trySpawnOasis(4, 2);
		trySpawnOasis(3, 1);

		auto isSlotEmpty = [&](const ProvinceBuildingSlot& provinceBuildingSlot)
		{
			return !provinceBuildingSlot.largeLandSlot.isValid() &&
				!provinceBuildingSlot.portSlot.isValid() &&
				!provinceBuildingSlot.portLandSlot.isValid() &&
				!provinceBuildingSlot.oasisSlot.isValid();
		};

		
		/*
		 * Land/Port Slots
		 */
		for (int32 provinceId = 0; provinceId < GameMapConstants::TotalRegions; provinceId++)
		{
			if (provinceSys.IsProvinceValid(provinceId))
			{	
				ProvinceBuildingSlot& provinceBuildingSlot = _provinceBuildingSlots[provinceId];

				auto isBuildable = [&](WorldTile2 tile) {
					return terrainGen.terrainTileType(tile) == TerrainTileType::None &&
						_simulation->GetProvinceIdClean(tile) == provinceId;
				};

				/*
				 * Don't put next to each other
				 */
				const std::vector<ProvinceConnection>& connections = provinceSys.GetProvinceConnections(provinceId);
				bool hasNearbyFilledSlot = false;
				for (const ProvinceConnection& connection : connections) {
					if (_provinceBuildingSlots[connection.provinceId].portLandSlot.isValid()) {
						hasNearbyFilledSlot = true;
						break;
					}
				}
				
				if (hasNearbyFilledSlot) {
					continue;
				}

				/*
				 * Large Land Slot
				 *  Low probabilility...
				 */
				auto findLargeLandSlot = [&]()
				{
					WorldTile2 centerTile = provinceSys.GetProvinceCenterTile(provinceId);

					// Also need to be able to add the main Minor City Building
					WorldTile2 largeLandSlotSize = GetBuildingInfo(CardEnum::MayanPyramid).baseBuildingSize;
					Direction faceDirection = Direction::S;//  static_cast<Direction>(GameRand::Rand(provinceId) % 4);
					TileArea largeLandSlotArea = BuildingArea(centerTile, largeLandSlotSize, faceDirection);

					bool isLandSlotNotBuildable = largeLandSlotArea.GetExpandedArea().ExecuteOnAreaWithExit_WorldTile2([&](WorldTile2 tile) {
						return !isBuildable(tile); // Should exit
					});
					if (!isLandSlotNotBuildable)
					{
						provinceBuildingSlot.largeLandSlot = { centerTile, largeLandSlotSize, faceDirection };
						_largeLandSlotProvinceIds.push_back(provinceId);
					}
				};
				if (provinceBuildingSlot.coastalTiles.size() == 0 &&
					GameRand::Rand(provinceId) % 5 == 0) 
				{
					findLargeLandSlot();
				}

				/*
				 * Port Slot
				 */
				if (isSlotEmpty(provinceBuildingSlot) &&
					provinceBuildingSlot.coastalTiles.size() > 0)
				{
					auto findPortSlotTile = [&]()
					{
						for (WorldTile2 coastTile : provinceBuildingSlot.coastalTiles)
						{
							auto checkPortSlotAvailable = [&](WorldTile2 centerTile, Direction faceDirection)
							{
								WorldTile2 portSize = GetBuildingInfo(CardEnum::MinorCityPort).baseBuildingSize;
								BuildPlacement portPlacement(centerTile, portSize, faceDirection);
								TileArea portArea = portPlacement.area();

								bool setDockInstruct;
								std::vector<PlacementGridInfo> grids;
								_simulation->CheckPortArea(portPlacement, CardEnum::MinorCityPort, grids, setDockInstruct, -1, 5);

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
								WorldTile2 minorCitySize = GetBuildingInfo(CardEnum::MinorCity).baseBuildingSize;
								int32 shift = (portSize.x / 2 + 1) + minorCitySize.x / 2;
								WorldTile2 minorCityCenterTile = centerTile + WorldTile2::DirectionTile(faceDirection) * shift;
								TileArea minorCityArea = BuildingArea(minorCityCenterTile, minorCitySize, OppositeDirection(faceDirection));

								bool isCityMainNotBuildable = minorCityArea.GetExpandedArea().ExecuteOnAreaWithExit_WorldTile2([&](WorldTile2 tile) {
									return !isBuildable(tile); // Should exit
								});
								if (isCityMainNotBuildable) {
									return false;
								}

								provinceBuildingSlot.portSlot = { centerTile, portSize, faceDirection };
								_portSlotProvinceIds.push_back(provinceId);

								provinceBuildingSlot.portLandSlot = { minorCityCenterTile, minorCitySize, faceDirection };
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
				 * Large Land Slot
				 */
				if (isSlotEmpty(provinceBuildingSlot) && 
					provinceBuildingSlot.coastalTiles.size() == 0)
				{
					findLargeLandSlot();
				}

				/*
				 * Land Slot
				 */
				if (isSlotEmpty(provinceBuildingSlot) &&
					provinceBuildingSlot.coastalTiles.size() == 0)
				{
					WorldTile2 centerTile = provinceSys.GetProvinceCenterTile(provinceId);
					
					// Also need to be able to add the main Minor City Building
					WorldTile2 minorCitySize = GetBuildingInfo(CardEnum::MinorCity).baseBuildingSize;
					Direction faceDirection = static_cast<Direction>(GameRand::Rand(provinceId) % 4);
					TileArea minorCityArea = BuildingArea(centerTile, minorCitySize, faceDirection);

					bool isCityNotBuildable = minorCityArea.GetExpandedArea().ExecuteOnAreaWithExit_WorldTile2([&](WorldTile2 tile) {
						return !isBuildable(tile); // Should exit
					});
					if (!isCityNotBuildable)
					{
						// TODO: smaller size for non Minor City
						//provinceBuildingSlot.portLandSlot = { centerTile, minorCitySize, faceDirection };
						//_landSlotProvinceIds.push_back(provinceId);
					}
				}
			}
		}

		PUN_LOG("ProvinceSlots: oasis:%d large:%d port:%d small:%d", 
			_oasisSlotProvinceIds.size(), _largeLandSlotProvinceIds.size(), 
			_portSlotProvinceIds.size(), _landSlotProvinceIds.size()
		);
	}

	const ProvinceBuildingSlot& provinceBuildingSlot(int32 provinceId) {
		return _provinceBuildingSlots[provinceId];;
	}

	const ProvinceOwnerInfo& provinceOwnerInfo(int32 provinceId) {
		return _provinceOwnerInfos[provinceId];
	}

	const std::vector<int32>& portSlotProvinceIds() { return _portSlotProvinceIds; }
	const std::vector<int32>& landSlotProvinceIds() { return _landSlotProvinceIds; }
	const std::vector<int32>& largeLandSlotProvinceIds() { return _largeLandSlotProvinceIds; }
	const std::vector<int32>& oasisSlotProvinceIds() { return _oasisSlotProvinceIds; }

	void SetSlotBuildingId(int32 provinceId, int32 buildingId) {
		//check(_provinceBuildingSlots[provinceId].portLandSlot.isValid() ||
		//	_provinceBuildingSlots[provinceId].largeLandSlot.isValid() ||
		//	_provinceBuildingSlots[provinceId].oasisSlot.isValid());
		
		_provinceBuildingSlots[provinceId].buildingId = buildingId;
	}
	void RemoveOasisSlot(int32 provinceId) {
		CppUtils::Remove(_oasisSlotProvinceIds, provinceId);
		_provinceBuildingSlots[provinceId].oasisSlot = BuildPlacement();
	}

	void RemoveTown(int32 provinceId) {
		_provinceOwnerInfos[provinceId].townId = -1;
		_provinceBuildingSlots[provinceId].buildingId = -1;
		check(provinceOwnerTown(provinceId) == -1);
	}

	/*
	 * Territory Owner
	 */

	void SetProvinceOwner(int32 provinceId, int32 townId, bool lightMode = false)
	{
		// Also update last owned town
		int32 lastTownId = _provinceOwnerInfos[provinceId].townId;
		if (IsValidMajorTown(lastTownId)) {
			_simulation->AddNeedDisplayUpdateId(DisplayGlobalEnum::Territory, lastTownId);
		}

		check(_provinceOwnerInfos[provinceId].provinceId == provinceId);
		_provinceOwnerInfos[provinceId].townId = townId;

		if (townId != -1 && !IsMinorTown(townId)) {
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

		if (IsValidMajorTown(townId)) {
			_simulation->AddNeedDisplayUpdateId(DisplayGlobalEnum::Territory, townId, true);
			PUN_LOG("SetProvinceOwner province:%d pid:%d", provinceId, townId);
		}
	}
	
	int32 provinceOwnerTown(int32 provinceId) { return _provinceOwnerInfos[provinceId].townId; }

	int32 provinceOwnerTownSafe(int32 provinceId)
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
	 * Raid
	 */
	void SetLastRaidTick(int32 provinceId, int32 lastRaidedTick)
	{
		check(_provinceOwnerInfos[provinceId].provinceId == provinceId);
		_provinceOwnerInfos[provinceId].lastRaidedTick = lastRaidedTick;
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
		SerializeVecValue(Ar, _largeLandSlotProvinceIds);
		SerializeVecValue(Ar, _oasisSlotProvinceIds);
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
	std::vector<int32> _largeLandSlotProvinceIds;
	std::vector<int32> _oasisSlotProvinceIds;
};