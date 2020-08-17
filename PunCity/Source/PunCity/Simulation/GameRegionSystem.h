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
	void Init(IGameSimulationCore* simulation) {
		_simulation = simulation;
		_territoryOwnerMap.resize(GameMapConstants::TotalRegions, -1);
		//_isDirectControl.resize(GameMapConstants::TotalRegions, false);
		
		_boarBurrowsToRegion.resize(GameMapConstants::TotalRegions);
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
			_simulation->AddNeedDisplayUpdateId(DisplayGlobalEnum::Territory, playerId);
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

	const std::vector<int32>& boarBurrows(int32 regionId) {
		return _boarBurrowsToRegion[regionId];
	}
	void AddBoarBurrow(int32 regionId, int32 buildingId) {
		_boarBurrowsToRegion[regionId].push_back(buildingId);
	}
	void RemoveBoarBurrow(int32 regionId, int32 buildingId);


	const std::vector<int32>& provinceAnimals(int32 provinceId) {
		return _provinceToAnimalIds[provinceId];
	}
	void AddProvinceAnimals(int32 provinceId, int32 animalId)
	{
//		PUN_CHECK(_simulation->IsProvinceValid(provinceId));
//		debugTotalProvinceAnimalCount++;
//		_provinceToAnimalIds[provinceId].push_back(animalId);
//
//#if !UE_BUILD_SHIPPING
//		if (_provinceToAnimalIds[provinceId].size() > debugMaxAnimalCount) {
//			debugMaxAnimalCount = _provinceToAnimalIds[provinceId].size();
//			debugMaxAnimalCountProvinceId = provinceId;
//		}
//#endif
	}
	void RemoveProvinceAnimals(int32 provinceId, int32 animalId)
	{
//		PUN_CHECK(_simulation->IsProvinceValid(provinceId));
//		
//		debugTotalProvinceAnimalCount--;
//		CppUtils::Remove(_provinceToAnimalIds[provinceId], animalId);
//
//#if !UE_BUILD_SHIPPING
//		if (provinceId == debugMaxAnimalCountProvinceId) {
//			debugMaxAnimalCount = _provinceToAnimalIds[provinceId].size();
//		}
//#endif
	}
	int32 RefreshAnimalHomeProvince(int32 provinceIdIn, int32 animalId)
	{
		// Find a nearby province with least animals
		const std::vector<ProvinceConnection>& connections = _simulation->GetProvinceConnections(provinceIdIn);
		int32 minAnimalCount = _provinceToAnimalIds[provinceIdIn].size();
		int32 minAnimalProvinceId = provinceIdIn;

		//for (const ProvinceConnection& connection : connections) {
		//	if (connection.tileType == TerrainTileType::None) {
		//		int32 animalCount = _provinceToAnimalIds[connection.provinceId].size();
		//		if (animalCount < minAnimalCount) {
		//			minAnimalCount = animalCount;
		//			minAnimalProvinceId = connection.provinceId;
		//		}
		//	}
		//}

		//// Move the animal
		//if (minAnimalProvinceId != provinceIdIn)
		//{
		//	RemoveProvinceAnimals(provinceIdIn, animalId);
		//	AddProvinceAnimals(minAnimalProvinceId, animalId);
		//}
		
		return minAnimalProvinceId;
	}

	void AddAnimalColony(UnitEnum unitEnum, WorldTile2 center, int32 radius, int32 chancePercentMultiplier);
	void RemoveAnimalColony(UnitEnum unitEnum);
	


	void Serialize(FArchive &Ar)
	{
		SerializeVecValue(Ar, _territoryOwnerMap);
		SerializeVecVecValue(Ar, _boarBurrowsToRegion);
		SerializeVecVecValue(Ar, _provinceToAnimalIds);
		SerializeVecObj(Ar, _animalColonies);
	}

#if !UE_BUILD_SHIPPING
	int32 debugMaxAnimalCount = 0;
	int32 debugMaxAnimalCountProvinceId = -1;
	int32 debugTotalProvinceAnimalCount = 0;
#endif

private:
	IGameSimulationCore* _simulation = nullptr;
	
	std::vector<int32> _territoryOwnerMap;
	//std::vector<bool> _isDirectControl;
	
	std::vector<std::vector<int32>> _boarBurrowsToRegion;
	std::vector<std::vector<int32>> _provinceToAnimalIds;
	std::vector<AnimalColony> _animalColonies;
};