// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "IGameSimulationCore.h"
#include "PunCity/CppUtils.h"

// TODO: remove struct constructor...
struct DropInfo
{
	int32 playerId;
	WorldTile2 tile;
	ResourceHolderInfo holderInfo;

	DropInfo() : playerId(-1), tile(WorldTile2::Invalid), holderInfo(ResourceHolderInfo::Invalid()) {}
	DropInfo(int32 playerId, WorldTile2 tile, ResourceHolderInfo holderInfo) : playerId(playerId), tile(tile), holderInfo(holderInfo) {}

	static DropInfo Invalid() { return DropInfo(-1, WorldTile2::Invalid, ResourceHolderInfo::Invalid()); }

	bool isValid() { return tile.isValid(); }

	bool operator==(const DropInfo& a) const {
		return a.playerId == playerId && a.holderInfo == holderInfo;
	}

	//! Serialize
	FArchive& operator>>(FArchive &Ar) {
		Ar << playerId;
		tile >> Ar;
		holderInfo >> Ar;
		return Ar;
	}
};

/**
 * 
 */
class ResourceDropSystem
{
public:
	void Init(IGameSimulationCore* simulation) {
		_simulation = simulation;
		_provinceIdToDrops.resize(GameMapConstants::TotalRegions);
		_regionIdToDrops.resize(GameMapConstants::TotalRegions);
	}

	void AddDrop(int32 playerId, ResourceHolderInfo holderInfo, WorldTile2 tile)
	{
		int32 provinceId = _simulation->GetProvinceIdClean(tile);
		PUN_CHECK(provinceId != -1);
		int32 provinceOwnerId = _simulation->provinceOwner(provinceId);
		PUN_CHECK(provinceOwnerId == playerId); // No dropping outside territory (SpawnDrop filtered this)

		//UE_LOG(LogTemp, Error, TEXT("AddDrop player:%d, %s, id:%d"), playerId, *ToFString(holderInfo.resourceName()), holderInfo.holderId);

		DropInfo dropInfo(playerId, tile, holderInfo);
		_provinceIdToDrops[provinceId].push_back(dropInfo);
		check(CppUtils::Contains(_provinceIdToDrops[provinceId], dropInfo));

		_regionIdToDrops[tile.regionId()].push_back(dropInfo);

#if WITH_EDITOR
		// Check if player id is the same for all drops
		for (DropInfo& info : _provinceIdToDrops[provinceId]) {
			PUN_CHECK(info.playerId == playerId);
		}
#endif
	}
	void RemoveDrop(int32 playerId, ResourceHolderInfo holderInfo, WorldTile2 tile) // normal removal from resourceSystem
	{
		int32 provinceId = _simulation->GetProvinceIdClean(tile);
		PUN_CHECK(provinceId != -1);

		//UE_LOG(LogTemp, Error, TEXT("RemoveDrop player:%d, %s, id:%d"), playerId, *ToFString(holderInfo.resourceName()), holderInfo.holderId);

		DropInfo dropInfo(playerId, tile, holderInfo);
		check(CppUtils::Contains(_provinceIdToDrops[provinceId], dropInfo));

		CppUtils::Remove(_regionIdToDrops[tile.regionId()], dropInfo);
		
		return CppUtils::Remove(_provinceIdToDrops[provinceId], dropInfo);
	}

	std::vector<DropInfo> GetDrops(WorldTile2 tile)
	{
		// TODO: could be changed to regionId?
		int32 provinceId = _simulation->GetProvinceIdClean(tile);
		if (provinceId == -1) {
			return {};
		}
		std::vector<DropInfo>& drops = _provinceIdToDrops[provinceId];
		std::vector<DropInfo> results;
		for (DropInfo& drop : drops) {
			if (drop.tile == tile) {
				results.push_back(drop);
			}
		}
		return results;
	}

	// Force removal by any non-resourceSystem caller
	void ForceRemoveDrop(WorldTile2 tile) {
		std::vector<DropInfo> drops = GetDrops(tile);
		for (DropInfo& drop : drops) {
			_simulation->DespawnResourceHolder(drop.holderInfo, drop.playerId);
			RemoveDrop(drop.playerId, drop.holderInfo, tile);
		}
	}

	// Needed this to delete drops by province (province changed hand etc.)
	void ResetProvinceDrop(int32 provinceId) {
		std::vector<DropInfo>& drops = _provinceIdToDrops[provinceId];
		for (DropInfo drop : drops) {
			CppUtils::Remove(_regionIdToDrops[drop.tile.regionId()], drop);
		}
		_provinceIdToDrops[provinceId].clear();
	}

	const std::vector<DropInfo>& Drops(int32 provinceId) {
		return _provinceIdToDrops[provinceId];
	}

	const std::vector<DropInfo>& DropsInRegion(WorldRegion2 region) {
		return _regionIdToDrops[region.regionId()];
	}

	void Serialize(FArchive& Ar)
	{
		SerializeVecVecObj(Ar, _provinceIdToDrops);

		if (Ar.IsLoading())
		{
			_regionIdToDrops.clear();
			_regionIdToDrops.resize(GameMapConstants::TotalRegions);
			
			// Get regionIdToDrop from _provinceIdToDrops
			for (std::vector<DropInfo>& drops : _provinceIdToDrops) {
				for (const DropInfo& dropInfo : drops) {
					_regionIdToDrops[dropInfo.tile.regionId()].push_back(dropInfo);
				}
			}
		}
	}

private:
	IGameSimulationCore* _simulation = nullptr;

	std::vector<std::vector<DropInfo>> _provinceIdToDrops;
	std::vector<std::vector<DropInfo>> _regionIdToDrops;
};
