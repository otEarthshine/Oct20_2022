// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "GameSimulationInfo.h"
#include "IGameSimulationCore.h"
#include "PunCity/PunUtils.h"

/**
 * 
 */
class GeoresourceSystem
{
public:
	void InitGeoresourceSystem(IGameSimulationCore* simulation, bool isFullInit);

	GeoresourceNode georesourceNode(int32 provinceId) const {
		if (provinceId == -1) {
			return GeoresourceNode(); // invalid province, return null
		}
		return _provinceToGeoresource[provinceId];
	}
	
	const std::vector<GeoresourceNode>& provinceToGeoresource() { return _provinceToGeoresource; }
	const std::vector<int32>& georesourceRegions() { return _georesourcesProvinceIds; }


	//const int32 ruinCost = 800;
	//bool CanClaimRuin(int32_t playerId, int32_t regionId) {
	//	PUN_CHECK(_regionToGeoresource[regionId].georesourceEnum == GeoresourceEnum::Ruin);
	//	PUN_CHECK(_simulation->regionOwner(regionId) == -1 || _simulation->regionOwner(regionId) == playerId);
	//	return !_regionToGeoresource[regionId].isUsedUp;
	//}
	//void ClaimRuin(int32 playerId, int32 regionId) {
	//	PUN_CHECK(CanClaimRuin(playerId, regionId));
	//	if (!_regionToGeoresource[regionId].isUsedUp) {
	//		_simulation->GenerateRareCardSelection(playerId, RareHandEnum::RareCards, "You found something interesting in the ruin!");
	//		_regionToGeoresource[regionId].isUsedUp = true;
	//	}

	//	// TODO: Destroy ruin option...
	//	// isDirty = true;
	//}

	void MineStone(int32 regionId, int32 amount)
	{
		int32& depositAmount = _provinceToGeoresource[regionId].stoneAmount;
		depositAmount = std::max(0, depositAmount - amount);
	}
	void MineOre(int32 regionId, int32 amount)
	{
		int32& depositAmount = _provinceToGeoresource[regionId].depositAmount;
		depositAmount = std::max(0, depositAmount - amount);
	}

	void Serialize(FArchive &Ar)
	{
		SerializeVecObj(Ar, _provinceToGeoresource);
		SerializeVecValue(Ar, _georesourcesProvinceIds);
	}

public:
	bool isDirty = false;

private:
	void PlantResource(int32 provinceId, GeoresourceEnum georesourceEnum, int32 depositAmount = 0);

private:
	IGameSimulationCore* _simulation = nullptr;

	std::vector<GeoresourceNode> _provinceToGeoresource;
	std::vector<int32> _georesourcesProvinceIds;
};
