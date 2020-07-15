// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameCoordinate.h"
#include "GameSimulationConstants.h"
#include "../GameConstants.h"
#include <memory>
#include "IGameSimulationCore.h"
#include "ProvinceSystem.h"

/**
 * 
 */
class GameRegionSystem
{
public:
	void Init(IGameSimulationCore* simulation) {
		_simulation = simulation;
		_territoryOwnerMap.resize(GameMapConstants::TotalRegions, -1);
		_isDirectControl.resize(GameMapConstants::TotalRegions, false);
		
		_boarBurrowsToRegion.resize(GameMapConstants::TotalRegions);
	}

	std::vector<int32>& territoryOwnerMap() { return _territoryOwnerMap; }

	void SetProvinceOwner(int32 provinceId, int32 playerId, bool isDirectControl = true)
	{
		// Also update last owned player
		int32 lastPlayerId = _territoryOwnerMap[provinceId];
		if (lastPlayerId != -1) {
			_simulation->AddNeedDisplayUpdateId(DisplayGlobalEnum::Territory, lastPlayerId);
		}
		
		_territoryOwnerMap[provinceId] = playerId;
		_isDirectControl[provinceId] = isDirectControl;
		_simulation->SetNeedDisplayUpdate(DisplayGlobalEnum::Province, true);

		_simulation->AddNeedDisplayUpdateId(DisplayGlobalEnum::Province, provinceId);

		if (playerId != -1) {
			_simulation->AddNeedDisplayUpdateId(DisplayGlobalEnum::Territory, playerId);
			PUN_LOG("SetProvinceOwner province:%d pid:%d", provinceId, playerId);
		}
	}
	
	int32 provinceOwner(int32 provinceId) { return _territoryOwnerMap[provinceId]; }
	bool isDirectControl(int32 provinceId) { return _isDirectControl[provinceId]; }

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


	void Serialize(FArchive &Ar)
	{
		SerializeVecValue(Ar, _territoryOwnerMap);
		SerializeVecVecValue(Ar, _boarBurrowsToRegion);
	}

private:
	IGameSimulationCore* _simulation = nullptr;
	
	std::vector<int32> _territoryOwnerMap;
	std::vector<bool> _isDirectControl;
	
	std::vector<std::vector<int32>> _boarBurrowsToRegion;
};