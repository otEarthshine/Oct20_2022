// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "GameSimulationConstants.h"
#include "IGameSimulationCore.h"
#include "ProvinceSystem.h"

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

/**
 * 
 */
class GameRegionSystem
{
public:
	void Init(IGameSimulationCore* simulation)
	{
		_simulation = simulation;

		provinceToTreeCountCache.resize(GameMapConstants::TotalRegions, -1);
		
		_provinceOwnerMap.resize(GameMapConstants::TotalRegions, -1);
		_provinceDistanceMap.resize(GameMapConstants::TotalRegions, MAX_int32);
		
		_boarBurrowsToProvince.resize(GameMapConstants::TotalRegions);
		_provinceToAnimalIds.resize(GameMapConstants::TotalRegions);
	}

	std::vector<int32>& territoryOwnerMap() { return _provinceOwnerMap; }

	void SetProvinceOwner(int32 provinceId, int32 playerId, bool lightMode = false)
	{
		// Also update last owned player
		int32 lastPlayerId = _provinceOwnerMap[provinceId];
		if (lastPlayerId != -1) {
			_simulation->AddNeedDisplayUpdateId(DisplayGlobalEnum::Territory, lastPlayerId);
		}
		
		_provinceOwnerMap[provinceId] = playerId;
		//_isDirectControl[provinceId] = isDirectControl;

		if (PunSettings::TrailerMode()) {
			return;
		}
		if (lightMode) {
			return;
		}
		
		_simulation->SetNeedDisplayUpdate(DisplayGlobalEnum::Province, true);
		_simulation->AddNeedDisplayUpdateId(DisplayGlobalEnum::Province, provinceId);

		if (playerId != -1) {
			_simulation->AddNeedDisplayUpdateId(DisplayGlobalEnum::Territory, playerId, true);
			PUN_LOG("SetProvinceOwner province:%d pid:%d", provinceId, playerId);
		}
	}
	
	int32 provinceOwner(int32 provinceId) { return _provinceOwnerMap[provinceId]; }
	
	int32 provinceDistanceMap(int32 provinceId) { return _provinceDistanceMap[provinceId]; }
	void SetProvinceDistanceMap(int32 provinceId, int32 provinceDistance) {
		_provinceDistanceMap[provinceId] = provinceDistance;
	}

	int32 provinceDistanceToPlayer(int32 provinceId, int32 playerId)
	{
		int32 cachedDist = provinceDistanceMap(provinceId);
		if (cachedDist != MAX_int32) {
			return cachedDist;
		}
		
		int32 minProvinceDistance = MAX_int32;
		const std::vector<ProvinceConnection>& connections = _simulation->GetProvinceConnections(provinceId);
		for (const ProvinceConnection& connection : connections) {
			if (connection.isConnectedTileType() &&
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
	
	//bool IsOwnedByPlayer(WorldRegion2 region, int32 playerId) {
	//	return region.IsValid() && _territoryOwnerMap[region.regionId()] == playerId;
	//}
	
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
		
		SerializeVecValue(Ar, _provinceOwnerMap);
		SerializeVecValue(Ar, _provinceDistanceMap);
		
		SerializeVecVecValue(Ar, _boarBurrowsToProvince);
		SerializeVecVecValue(Ar, _provinceToAnimalIds);
		SerializeVecObj(Ar, _animalColonies);
	}


	int32 debugMaxAnimalCount = 0;
	int32 debugMaxAnimalCountProvinceId = -1;
	int32 debugTotalProvinceAnimalCount = 0;

public:
	std::vector<int32> provinceToTreeCountCache;

private:
	IGameSimulationCore* _simulation = nullptr;
	
	std::vector<int32> _provinceOwnerMap;
	std::vector<int32> _provinceDistanceMap;
	
	std::vector<std::vector<int32>> _boarBurrowsToProvince;
	std::vector<std::vector<int32>> _provinceToAnimalIds;
	std::vector<AnimalColony> _animalColonies;
};