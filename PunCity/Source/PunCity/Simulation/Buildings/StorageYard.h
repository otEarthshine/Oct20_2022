// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "../Building.h"
#include "../Resource/ResourceSystem.h"

class StorageYard : public Building
{
public:
	void FinishConstruction() override {
		Building::FinishConstruction();

		for (ResourceInfo info : ResourceInfos) {
			AddResourceHolder(info.resourceEnum, ResourceHolderType::Storage, 0);
		}
	}

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
	

	void Serialize(FArchive& Ar) override {
		Building::Serialize(Ar);
		Ar << _tilesOccupied;

		// TODO: Reuse this for ranch ..... remove once new save... UPDATE MAINMENU
		//if (!_simulation->isLoadingForMainMenu()) {
		//	_miniArea >> Ar;
		//}
	}

private:
	// This includes push
	int32 _tilesOccupied = 0;

	// Manipulate what goes into this storage yard.
	//std::vector<int32>;
};