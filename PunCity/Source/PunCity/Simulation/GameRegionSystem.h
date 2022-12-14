// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameCoordinate.h"
#include "GameSimulationConstants.h"
#include "../GameConstants.h"
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
		
		_territoryOwnerMap.resize(GameMapConstants::TotalRegions, -1);
		
		_boarBurrowsToProvince.resize(GameMapConstants::TotalRegions);
		_provinceToAnimalIds.resize(GameMapConstants::TotalRegions);
	}

	std::vector<int32>& territoryOwnerMap() { return _territoryOwnerMap; }

	void SetProvinceOwner(int32 provinceId, int32 playerId, bool lightMode = false)
	{
		// Also update last owned player
		int32 lastPlayerId = _territoryOwnerMap[provinceId];
		if (lastPlayerId != -1) {
			_simulation->AddNeedDisplayUpdateId(DisplayGlobalEnum::Territory, lastPlayerId);
		}
		
		_territoryOwnerMap[provinceId] = playerId;
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
	
	int32 provinceOwner(int32 provinceId) { return _territoryOwnerMap[provinceId]; }
	//bool isDirectControl(int32 provinceId) { return _isDirectControl[provinceId]; }

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
		
		SerializeVecValue(Ar, _territoryOwnerMap);
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
	
	std::vector<int32> _territoryOwnerMap;
	//std::vector<bool> _isDirectControl;
	
	std::vector<std::vector<int32>> _boarBurrowsToProvince;
	std::vector<std::vector<int32>> _provinceToAnimalIds;
	std::vector<AnimalColony> _animalColonies;
};