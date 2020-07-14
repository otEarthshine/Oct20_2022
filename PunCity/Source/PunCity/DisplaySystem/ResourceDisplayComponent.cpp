// Fill out your copyright notice in the Description page of Project Settings.


#include "ResourceDisplayComponent.h"

DECLARE_CYCLE_STAT(TEXT("PUN: [Display]Drops"), STAT_PunDisplayDropTick, STATGROUP_Game);
DECLARE_CYCLE_STAT(TEXT("PUN: [Display]Building_Resource"), STAT_PunDisplayBuilding_Resource, STATGROUP_Game);

using namespace std;

static const int32_t maxStack = 1;

int UResourceDisplayComponent::CreateNewDisplay(int objectId)
{
	int meshId = _meshes.Num();

	_meshes.Add(NewObject<UStaticFastInstancedMeshesComp>(this));
	_meshes[meshId]->Init("ResourceDrop" + to_string(meshId), this, CoordinateConstants::TileIdsPerRegion, "", -1, true);

	for (int i = 0; i < ResourceEnumCount; i++) {
		ResourceEnum resourceEnum = static_cast<ResourceEnum>(i);
		_meshes[meshId]->AddProtoMesh(ResourceNameF(resourceEnum), _assetLoader->resourceMesh(resourceEnum));
		_meshes[meshId]->AddProtoMesh(ResourceNameF(resourceEnum) + FString("Hand"), _assetLoader->resourceHandMesh(resourceEnum));
	}

	return meshId;
}

void UResourceDisplayComponent::OnSpawnDisplay(int regionId, int meshId, WorldAtom2 cameraAtom)
{
	_meshes[meshId]->SetActive(true);

	// Refresh
	simulation().SetNeedDisplayUpdate(DisplayClusterEnum::Resource, regionId, true);
}

static std::vector<bool> _tileIdToInUse;

void UResourceDisplayComponent::UpdateDisplay(int regionId, int meshId, WorldAtom2 cameraAtom)
{
	auto& dropSystem = simulation().dropSystem();
	auto& buildingSystem = simulation().buildingSystem();

	// Set region location
	WorldRegion2 region(regionId);
	FVector regionDisplayLocation = MapUtil::DisplayLocation(cameraAtom, region.worldAtom2());
	_meshes[meshId]->SetRelativeLocation(regionDisplayLocation + FVector(5, 5, 0));

	if (simulation().NeedDisplayUpdate(DisplayClusterEnum::Resource, regionId) || isMainMenuDisplay)
	{
		//! Drops
		if (!isMainMenuDisplay)
		{
			SCOPE_CYCLE_COUNTER(STAT_PunDisplayDropTick);

			const std::vector<DropInfo>& drops = dropSystem.Drops(regionId);

			_tileIdToInUse.resize(maxStack * CoordinateConstants::TileIdsPerRegion);
			fill(_tileIdToInUse.begin(), _tileIdToInUse.end(), false);

			for (int j = 0; j < drops.size(); j++) {
				DropInfo dropInfo = drops[j];

				WorldTile2 tile = dropInfo.tile;
				LocalTile2 localTile = tile.localTile();

				// Increment the tileId if it was already in used
				for (int k = 0; k < maxStack; k++) {
					int32_t tileIdWithStack = localTile.tileId() + k * CoordinateConstants::TileIdsPerRegion;
					if (!_tileIdToInUse[tileIdWithStack])
					{
						FTransform transform(FRotator::ZeroRotator, localTile.localDisplayLocation() + FVector(0, 0, 4 * k + 2), FVector(0.8f, 0.8f, 0.8f));
						_meshes[meshId]->Add(ResourceNameF(dropInfo.holderInfo.resourceEnum) + FString("Hand"), tileIdWithStack, transform, 0);
						_tileIdToInUse[tileIdWithStack] = true;
						break;
					}
				}
			}
		}

		//! Building's stacks
		{
			SCOPE_CYCLE_COUNTER(STAT_PunDisplayBuilding_Resource);

			auto& buildingList = buildingSystem.buildingSubregionList();

			buildingList.ExecuteRegion(region, [&](int32_t buildingId)
			{
				Building& building = buildingSystem.building(buildingId);
				std::vector<ResourceHolderInfo>& holderInfos = building.holderInfos();
				TileArea area = building.area();
				WorldTile2 minTile = area.min();

				// relativeMinTile can be negative since minTile can be out of region..
				WorldTile2 relativeMinTileWorld = minTile - region.minTile();
				LocalTile2 relativeMinTile(relativeMinTileWorld.x, relativeMinTileWorld.y);

				LocalTile2 localCenter = building.centerTile().localTile();
				check(localCenter.isValid());
				ResourceSystem& resourceSystem = simulation().resourceSystem(building.playerId());

				const int resourcePerDisplay = 10;

				if (building.isEnum(CardEnum::StorageYard) ||
					building.isEnum(CardEnum::TradingPost) ||
					building.isEnum(CardEnum::TradingPort) ||
					building.isEnum(CardEnum::HumanitarianAidCamp))
				{
					int32 currentTileIndex = 0;

					FVector resourceShift = FVector::ZeroVector;
					if (building.isEnum(CardEnum::TradingPost)) {
						switch (building.faceDirection()) {
							case Direction::S: resourceShift = FVector(0, 20, 0); break;
							case Direction::N: resourceShift = FVector(40, 20, 0); break;
							case Direction::E: resourceShift = FVector(20, 40, 0); break;
							case Direction::W: resourceShift = FVector(20, 0, 0); break;
							default: break;
						}
						//resourceShift = FVector(0, 20, 0);
						//resourceShift = resourceShift.RotateAngleAxis(RotationFromDirection(building.faceDirection()), FVector::UpVector);
					}
					else if (building.isEnum(CardEnum::TradingPort)) {
						switch (building.faceDirection()) {
						case Direction::S: resourceShift = FVector(0, 0, 0); break;
						case Direction::N: resourceShift = FVector(60, 50, 0); break;
						case Direction::E: resourceShift = FVector(0, 60, 0); break;
						case Direction::W: resourceShift = FVector(50, 0, 0); break;
						default: break;
						}
						//resourceShift = FVector(0, 20, 0);
						//resourceShift = resourceShift.RotateAngleAxis(RotationFromDirection(building.faceDirection()), FVector::UpVector);
					}

					for (size_t j = 0; j < holderInfos.size(); j++)
					{
						check(holderInfos[j].isValid());

						int resourceCount = resourceSystem.resourceCount(holderInfos[j]);
						int tileCount = (resourceCount + GameConstants::StorageCountPerTile - 1) / GameConstants::StorageCountPerTile;
						check(tileCount >= 0);
						check(tileCount <= building.storageSlotCount());

						FString resourceName = ToFString(holderInfos[j].resourceName());

						const int stacksPerSide = building.stackPerSide();

						for (int i = 0; i < tileCount; i++)
						{
							int x = currentTileIndex % stacksPerSide;
							int y = currentTileIndex / stacksPerSide;
							currentTileIndex++;

							FVector resourceLocation = FVector(x * 20 + 5, y * 20 + 5, 0) + relativeMinTile.localDisplayLocation() + resourceShift;

							// Scale is used to show resource fraction.
							int resourceInStack = std::min(GameConstants::StorageCountPerTile, resourceCount - i * GameConstants::StorageCountPerTile);
							int unitsInStack = (resourceInStack + resourcePerDisplay - 1) / resourcePerDisplay;
							PUN_CHECK(unitsInStack > 0);
							float scale = unitsInStack * 0.05f + 0.01;
							
							FTransform transform(FRotator(0, 0, 0), resourceLocation, FVector(1, 1, scale));

							// TODO: resourceTile could be out of range...
							//  Handle by having putting it in tileId beyond max reserved for this?? reserve.. complex..
							//  Or just be lazy and 5000 * 16 tileIds per region, too much???
							// Or use TileIdHashMap for this?? unlimited tileIds... less code!

							_meshes[meshId]->Add(resourceName, localCenter.tileId() + i * CoordinateConstants::TileIdsPerRegion, transform, 0);
						}
					}
				}

			});
		}


		// After
		_meshes[meshId]->AfterAdd();

		simulation().SetNeedDisplayUpdate(DisplayClusterEnum::Resource, regionId, false);
	}

	if (isMainMenuDisplay) {
		return;
	}

	// Debug lines
	if (PunSettings::Settings["DropLines"])
	{
		const std::vector<DropInfo>& drops = dropSystem.Drops(regionId);
		for (int j = 0; j < drops.size(); j++) {
			DropInfo dropInfo = drops[j];

			WorldTile2 tile = dropInfo.tile;
			FVector start = MapUtil::DisplayLocation(cameraAtom, tile.worldAtom2());
			lineBatch()->DrawLine(start, start + FVector(0, 0, 100), FLinearColor::White, 100.0f, 1.0f, 10000);
		}
	}
}

void UResourceDisplayComponent::HideDisplay(int meshId)
{
	_meshes[meshId]->SetActive(false);
}