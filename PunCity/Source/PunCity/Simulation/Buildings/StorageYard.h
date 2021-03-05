// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "../Building.h"
#include "../Resource/ResourceSystem.h"
#include "../PlayerOwnedManager.h"
#include "PunCity/MapUtil.h"

class StorageBase : public Building
{
public:
	void OnInit() override
	{
		// queuedResourceAllowed must be here to allow managing storage before it is finished
		queuedResourceAllowed.clear();
		queuedResourceAllowed.resize(ResourceEnumCount, true);
	}

	virtual ResourceHolderType defaultHolderType() { return ResourceHolderType::Storage; }

	void FinishConstruction() override
	{
		Building::FinishConstruction();

		for (ResourceInfo info : ResourceInfos) {
			ResourceHolderType holderType = queuedResourceAllowed[info.resourceEnumInt()] ? defaultHolderType() : ResourceHolderType::Provider;
			AddResourceHolder(info.resourceEnum, holderType, 0);
		}
		queuedResourceAllowed.clear();

		TrailerAddResource();
	}

	bool ResourceAllowed(ResourceEnum resourceEnum)
	{
		if (isConstructed()) {
			return holder(resourceEnum).type != ResourceHolderType::Provider;
		}
		return queuedResourceAllowed[static_cast<int>(resourceEnum)];
	}

	int32 GetFoodCount() {
		int count = 0;
		for (ResourceEnum foodEnum : StaticData::FoodEnums) {
			count += resourceCount(foodEnum);
		}
		return count;
	}

	void Serialize(FArchive& Ar) override {
		Building::Serialize(Ar);
		Ar << expandedFood;
		Ar << expandedLuxury;

		SerializeVecValue(Ar, queuedResourceAllowed);
	}

public:
	bool expandedFood = true;
	bool expandedLuxury = true;

	std::vector<uint8> queuedResourceAllowed;
};


class StorageYard : public StorageBase
{
public:
	int32 storageSlotCount() override {
		return (_area.sizeX() / 2) * (_area.sizeY() / 2);
	}
	int32 stackPerSide() override {
		return (_area.sizeX() / 2);
	}

	// Gate on the S in the middle.
	WorldTile2 gateTile() override {
		return WorldTile2(_area.minX, _area.minY / 2 + _area.maxY / 2);
	}

	void SetAreaWalkable() override
	{
		// Make the area on which this was built walkable.
		_area.ExecuteOnArea_WorldTile2([&](WorldTile2 tile) {
			bool isBorder = (tile.x == _area.minX) ||
				(tile.x == _area.maxX) ||
				(tile.y == _area.minY) ||
				(tile.y == _area.maxY);
			_simulation->SetWalkable(tile, isBorder);
		});

		_simulation->ResetUnitActionsInArea(_area);

		_didSetWalkable = true;
	}


	// TODO: reuse this for ranch
	//void SetMiniArea(TileArea miniArea) {
	//	_miniArea = miniArea;
	//}
	//WorldTile2 gateTile() override
	//{
	//	return Building::gateTile();
	//	
	//	// case for initial/AI's storage that didn't specify miniArea
	//	if (_miniArea == _area) {
	//		return Building::gateTile();
	//	}
	//	
	//	if (_faceDirection == Direction::N) {
	//		return WorldTile2(_miniArea.minX + 1, _miniArea.minY);
	//	}
	//	if (_faceDirection == Direction::S) {
	//		return WorldTile2(_miniArea.minX - 1, _miniArea.minY);
	//	}
	//	if (_faceDirection == Direction::E) {
	//		return WorldTile2(_miniArea.minX, _miniArea.minY + 1);
	//	}
	//	if (_faceDirection == Direction::W) {
	//		return WorldTile2(_miniArea.minX, _miniArea.minY - 1);
	//	}
	//	UE_DEBUG_BREAK();
	//	return WorldTile2::Invalid;
	//}


	void RefreshStorage()
	{
		_tilesOccupied = StorageTilesOccupied(holderInfos(), resourceSystem());

		// TODO: wtf?
		//PUN_CHECK(occupied <= storageSlotCount());
		if (_tilesOccupied > storageSlotCount()) {
			_tilesOccupied = storageSlotCount();
		}
	}

	// This includes push
	int32 tilesOccupied() { return _tilesOccupied; }

	bool isFull() {
		return tilesOccupied() >= storageSlotCount();
	}

	int32 SpaceLeftFor(ResourceEnum resourceEnum)
	{
		PUN_CHECK(resourceEnum != ResourceEnum::None);
		
		const int32 countPerTile = GameConstants::StorageCountPerTile;
		
		int32 spaceLeft = (storageSlotCount() - _tilesOccupied) * GameConstants::StorageCountPerTile;

		// If there is existing resource, we can also fit more into existing tile
		int32 resourceCountWithPush = resourceSystem().resourceCountWithPush(_holderInfos[static_cast<int>(resourceEnum)]);
		if (resourceCountWithPush > 0) {
			int32 tilesOccupiedByThisResource = (countPerTile - 1 + resourceCountWithPush) / countPerTile;
			spaceLeft += (tilesOccupiedByThisResource * countPerTile) - resourceCountWithPush;
		}
		return spaceLeft;
	}

	int32 maxCardSlots() override { return 0; }


	bool RefreshHoverWarning() override
	{
		if (_simulation->isStorageAllFull(_townId)) {
			hoverWarning = HoverWarning::StorageFull;
			return true;
		}

		hoverWarning = HoverWarning::None;
		return false;
	}
	

	void Serialize(FArchive& Ar) override {
		StorageBase::Serialize(Ar);
		Ar << _tilesOccupied;
	}

private:
	// This includes push
	int32 _tilesOccupied = 0;

	// Manipulate what goes into this storage yard.
	//std::vector<int32>;
};

class Warehouse : public StorageYard
{
public:
	int32 storageSlotCount() override {
		return 30;
	}

	WorldTile2 gateTile() override {
		return Building::gateTile();
	}

	void SetAreaWalkable() override {
		Building::SetAreaWalkable();
	}
};

class Market : public StorageBase
{
public:
	static const int32 Radius = 34;

	void OnInit() override
	{
		queuedResourceAllowed.clear();
		queuedResourceAllowed.resize(ResourceEnumCount, false);

		_resourceTargets.clear();
		_resourceTargets.resize(ResourceEnumCount, 0);

		for (int32 i = 0; i < _resourceTargets.size(); i++) 
		{
			ResourceEnum resourceEnum = static_cast<ResourceEnum>(i);
			if (IsFoodEnum(resourceEnum) ||
				IsFuelEnum(resourceEnum)) {
				_resourceTargets[i] = 150;
				queuedResourceAllowed[i] = true;
			}
			else if (IsMedicineEnum(resourceEnum) ||
				IsToolsEnum(resourceEnum) ||
				IsLuxuryEnum(resourceEnum)) {
				_resourceTargets[i] = 70;
				queuedResourceAllowed[i] = true;
			}
			else {
				queuedResourceAllowed[i] = false;
			}
		}
		
		_foodTarget = 500;
	}


	void FinishConstruction() override {
		StorageBase::FinishConstruction();

		_upgrades = {
			MakeWorkerSlotUpgrade(50, 2),
		};

		//PUN_CHECK(lastUIResourceTargets.size() < 1000);
	}

	// TEST
#if WITH_EDITOR
	void OnTick1Sec() override {
		PUN_CHECK(lastUIResourceTargets.size() < 1000);
	}
#endif

	/*
	 * Market Targets
	 */
	bool IsMarketResource(ResourceEnum resourceEnum) {
		return  IsFoodEnum(resourceEnum) ||
			IsFuelEnum(resourceEnum) ||
			IsMedicineEnum(resourceEnum) || 
			IsToolsEnum(resourceEnum) || 
			IsLuxuryEnum(resourceEnum);
	}

	int32 GetMarketTarget(ResourceEnum resourceEnum) {
		return _resourceTargets[static_cast<int>(resourceEnum)];
	}
	const std::vector<int32>& GetMarketTargets() { return _resourceTargets; }
	
	void SetMarketTarget(ResourceEnum resourceEnum, int32 amount) {
		if (resourceEnum == ResourceEnum::Food) {
			_foodTarget = amount;
			return;
		}
		PUN_CHECK(IsResourceValid(resourceEnum));
		_resourceTargets[static_cast<int>(resourceEnum)] = amount;
	}

	int32 GetFoodTarget() {
		return _foodTarget;
	}


	ResourceHolderType defaultHolderType() override { return ResourceHolderType::Market; }
	
	void Serialize(FArchive& Ar) override {
		StorageBase::Serialize(Ar);
		SerializeVecValue(Ar, _resourceTargets);
		Ar << _foodTarget;
	}
	
	// Non-Serialize
	//std::vector<int32> lastUIResourceTargets;

	
	int32 lastUIFoodTarget = -1; // TODO: seems like something is writing into this part of memory...
	std::vector<int32> lastUIResourceTargets;
private:
	std::vector<int32> _resourceTargets;
	int32 _foodTarget = 0;
};



class IntercityLogisticsHub : public Building
{
public:
	void FinishConstruction() override
	{
		Building::FinishConstruction();
		resourceEnums.resize(4, ResourceEnum::None);
		resourceCounts.resize(4, 0);

		lastResourceEnums.resize(4, ResourceEnum::None);
		lastResourceCounts.resize(4, 0);

		// Grab any existing town as townID
		targetTownId = -1;
		std::vector<int32> townIds = isEnum(CardEnum::IntercityLogisticsHub) ? GetTradableTownIdsLand() : GetTradableTownIdsWater();
		for (int32 townId : townIds) {
			if (townId != _townId) {
				targetTownId = townId;
				break;
			}
		}
		lastTargetTownId = -1;

		// Holders
		for (ResourceInfo info : ResourceInfos) {
			AddResourceHolder(info.resourceEnum, ResourceHolderType::Provider, 0);
		}
	}

	//!
	std::vector<int32> GetTradableTownIdsLand()
	{
		cachedLandConnectedTownIds.clear();

		WorldTile2 startTile = gateTile();

		const auto& tradableTownIds = GetApproximateTradableTownIdsByDistance(gateTile(), _playerId, _buildingEnum, _townId, _simulation);
		for (int32 tradableTownId : tradableTownIds) {
			// Ensure the town can be connected by land
			WorldTile2 targetTile = _simulation->GetTownhallGate(tradableTownId);
			if (targetTile.isValid()) {
				//std::vector<uint32_t> path;
				//bool succeed = _simulation->pathAI(true)->FindPathRoadOnly(hubGate.x, hubGate.y, townGateTile.x, townGateTile.y, path);
				//if (succeed) {
				//	cachedLandConnectedTownIds.push_back(townId);
				//}

				std::vector<uint32_t> path;
				bool succeed = _simulation->pathAI()->FindPathRoadOnly(startTile.x, startTile.y, targetTile.x, targetTile.y, path);
				if (!succeed) {
					// Try to go to townhall first
					if (!_simulation->IsConnectedBuilding(buildingId())) {
						continue;
					}

					// Might be able to use townhall as startTile instead
					startTile = _simulation->GetTownhallGate(townId());
					succeed = _simulation->pathAI()->FindPathRoadOnly(startTile.x, startTile.y, targetTile.x, targetTile.y, path);
					if (!succeed) {
						continue;
					}
				}

				cachedLandConnectedTownIds.push_back(tradableTownId);
			}
		}
		
		return cachedLandConnectedTownIds;
	}
	std::vector<int32> GetTradableTownIdsWater()
	{
		std::vector<int32> tradableTownIds;

		const auto& townIds = GetApproximateTradableTownIdsByDistance(centerTile(), _playerId, _buildingEnum, _townId, _simulation);
		for (int32 townId : townIds) {
			// Ensure the town has port
			if (_simulation->GetPortIds(townId).size() > 0) {
				tradableTownIds.push_back(townId);
			}
		}
		return tradableTownIds;
	}

	//!
	static std::vector<int32> GetApproximateTradableTownIdsByDistance(WorldTile2 originTile, int32 playerId, CardEnum buildingEnum, int32 townToOmit, IGameSimulationCore* sim)
	{
		int32 maxTownDistance = (buildingEnum == CardEnum::IntercityLogisticsHub) ? MaxLogisticsHubDistance : MaxLogisticsPortDistance;

		std::vector<int32> tradableTownIds;

		const auto& townIds = sim->GetTownIds(playerId);
		for (int32 townId : townIds) {
			if (townId != townToOmit)
			{
				// Ensure the town isn't too far
				WorldTile2 townGateTile = sim->GetTownhallGate(townId);
				if (townGateTile.isValid() && WorldTile2::Distance(townGateTile, originTile) <= maxTownDistance) {
					tradableTownIds.push_back(townId);
				}
			}
		}
		return tradableTownIds;
	}
	

	bool needSetup() {
		for (ResourceEnum resourceEnum : resourceEnums) {
			if (resourceEnum != ResourceEnum::None) {
				return false;
			}
		}
		return true;
	}

	bool RefreshHoverWarning() override
	{
		if (isConstructed())
		{
			if (targetTownId == -1) {
				hoverWarning = HoverWarning::IntercityLogisticsNeedTargetTown;
				return true;
			}
			if (needSetup()) {
				hoverWarning = HoverWarning::IntercityLogisticsNeedTargetResource;
				return true;
			}
		}

		return Building::RefreshHoverWarning();
	}

	virtual void SetTargetTownId(int32 targetTownIdIn) {
		targetTownId = targetTownIdIn;
	}

	void ChangeTownOwningPlayer(int32 playerId) override
	{
		Building::ChangeTownOwningPlayer(playerId);

		targetTownId = -1;
		lastTargetTownId = -1;
	}

	void Serialize(FArchive& Ar) override {
		Building::Serialize(Ar);
		Ar << targetTownId;
		SerializeVecValue(Ar, resourceEnums);
		SerializeVecValue(Ar, resourceCounts);
	}

	static const int32 MaxLogisticsHubDistance = 500;
	static const int32 MaxLogisticsPortDistance = 1000;

public:
	int32 targetTownId;
	std::vector<ResourceEnum> resourceEnums;
	std::vector<int32> resourceCounts;

	// Display
	int32 lastTargetTownId;
	std::vector<ResourceEnum> lastResourceEnums;
	std::vector<int32> lastResourceCounts;

	std::vector<int32> cachedLandConnectedTownIds;
};

class IntercityLogisticsPort final : public IntercityLogisticsHub
{
public:
	void SetTargetTownId(int32 targetTownIdIn) override {
		IntercityLogisticsHub::SetTargetTownId(targetTownIdIn);

		//// Try cache the trade route
		//if (targetTownIdIn != -1)
		//{
		//	int32 minPortDist = MAX_int32;
		//	int32 nearestPortId = _simulation->FindNearestBuildingId(gateTile(), CardEnum::TradingPort, targetTownIdIn, minPortDist);

		//	if (nearestPortId != -1 && minPortDist < MaxLogisticsPortDistance)
		//	{
		//		std::vector<WorldTile2> resultPath;
		//		_simulation->FindPathWater(buildingId(), nearestPortId, resultPath);
		//	}
		//}
	}

public:
	// Const
	static const int32 MaxLogisticsDistance = 1500;
};