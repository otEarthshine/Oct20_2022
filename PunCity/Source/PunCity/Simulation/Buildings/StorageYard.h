// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "../Building.h"
#include "../Resource/ResourceSystem.h"
#include "../PlayerOwnedManager.h"

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
		if (_simulation->isStorageAllFull(_playerId)) {
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
		PUN_CHECK(IsResourceValid(resourceEnum));
		if (resourceEnum == ResourceEnum::Food) {
			_foodTarget = amount;
			return;
		}
		_resourceTargets[static_cast<int>(resourceEnum)] = amount;

		//PUN_CHECK(lastUIResourceTargets.size() < 1000);
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



class IntercityLogisticsHub final : public Building
{
public:
	void OnInit() override {
		resourcePairs.resize(4, { ResourceEnum::None, 0 });
	}

	bool needSetup() {
		for (const ResourcePair& pair : resourcePairs) {
			if (pair.resourceEnum != ResourceEnum::None) {
				return false;
			}
		}
		return true;
	}

	bool RefreshHoverWarning() override
	{
		if (Building::RefreshHoverWarning()) {
			return true;
		}

		if (targetTownId == -1) {
			hoverWarning = HoverWarning::IntercityLogisticsNeedTargetTown;
			return true;
		}
		if (needSetup()) {
			hoverWarning = HoverWarning::IntercityLogisticsNeedTargetResource;
			return true;
		}

		hoverWarning = HoverWarning::None;
		return true;
	}

	void Serialize(FArchive& Ar) override {
		Building::Serialize(Ar);
		SerializeVecObj(Ar, resourcePairs);
	}

	int32 targetTownId = -1;
	std::vector<ResourcePair> resourcePairs;

	// Display
	int32 lastTargetTownId = -1;
	std::vector<ResourcePair> lastResourcePairs;
};