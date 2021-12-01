// Pun Dumnernchanvanit's


#include "MiniBuildingDisplayComponent.h"

using namespace std;

int UMiniBuildingDisplayComponent::CreateNewDisplay(int regionId)
{
	int meshId = _moduleMeshes.Num();

	// Modules
	{
		_moduleMeshes.Add(NewObject<UStaticFastInstancedMeshesComp>(this));
		_moduleMeshes[meshId]->Init("MiniBuildingModules" + to_string(meshId) + "_", this, 20, "", meshId);

		const TArray<FName>& moduleNames = _assetLoader->moduleNames();
		for (int i = 0; i < moduleNames.Num(); i++) 
		{
			// Only keep body and roof
			if (GameDisplayInfo::IsMiniModule(moduleNames[i]))
			{
				UStaticMesh* protoMesh = _assetLoader->moduleMesh(moduleNames[i]);
				if (protoMesh) {
					_moduleMeshes[meshId]->AddProtoMesh(moduleNames[i], protoMesh, nullptr);
				}
			}
		}
	}

	return meshId;
}

void UMiniBuildingDisplayComponent::OnSpawnDisplay(int regionId, int meshId, WorldAtom2 cameraAtom)
{
	_moduleMeshes[meshId]->SetActive(true);

	simulation().SetNeedDisplayUpdate(DisplayClusterEnum::Building, WorldRegion2(regionId).regionId(), true);
}

void UMiniBuildingDisplayComponent::UpdateDisplay(int regionId, int meshId, WorldAtom2 cameraAtom, bool justSpawned, bool justCreated)
{
	auto& buildingList = simulation().buildingSystem().buildingSubregionList();
	auto& sim = simulation();
	BuildingSystem& buildingSystem = sim.buildingSystem();
	const GameDisplayInfo& displayInfo = gameManager()->displayInfo();
	
	WorldRegion2 region(regionId);
	FVector regionDisplayLocation = gameManager()->DisplayLocation(region.worldAtom2()); //MapUtil::DisplayLocation(cameraAtom, region.worldAtom2());

	{
		//SCOPE_TIMER_("Tick Building Move %d", _moduleMeshes[meshId]->meshPool.Num());
		_moduleMeshes[meshId]->SetRelativeLocation(regionDisplayLocation + FVector(5, 5, 0));
	}
	
	if (simulation().NeedDisplayUpdate(DisplayClusterEnum::Building, regionId))
	{
		buildingList.ExecuteRegion(region, [&](int32_t buildingId)
		{
			Building& building = buildingSystem.building(buildingId);
			CardEnum buildingEnum = building.buildingEnum();
			FactionEnum factionEnum = building.factionEnum();
			const std::vector<BuildingUpgrade>& upgrades = building.upgrades();

			// Don't display road
			if (IsRoad(buildingEnum)) {
				return;
			}

			// Special case bridge
			if (IsBridgeOrTunnel(buildingEnum)) 
			{
				return;
			}
			if (buildingEnum == CardEnum::BoarBurrow) {
				return;
			}

			float displayVariationIndex = building.displayVariationIndex();
			int32_t buildingRotation = RotationFromDirection(building.displayFaceDirection());

			WorldTile2 centerTile = building.displayCenterTile();
			FTransform transform(FRotator(0, buildingRotation, 0), centerTile.localTile().localDisplayLocation());

			const ModuleTransformGroup& modulePrototype = displayInfo.GetDisplayModules(factionEnum, buildingEnum, displayVariationIndex);
			std::vector<ModuleTransform> modules = modulePrototype.miniModules;

			if (building.isBurnedRuin())
			{
			}
			else if (building.isConstructed())
			{
				for (int i = 0; i < modules.size(); i++)
				{
					int32_t instanceKey = centerTile.tileId() + i * GameMapConstants::TilesPerWorld;

					FTransform finalTransform;
					FTransform::Multiply(&finalTransform, &modules[i].transform, &transform);

					//PUN_LOG("ModuleDisplay %s, %s", *finalTransform.GetRotation().Rotator().ToString(), *modules[i].transform.GetRotation().Rotator().ToString());
					_moduleMeshes[meshId]->Add(modules[i].moduleName, instanceKey, finalTransform, 0, buildingId);
				}

			}
		});

		//! Oasis
		{
			const ProvinceBuildingSlot& slot = sim.provinceInfoSystem().provinceBuildingSlot(regionId);
			if (slot.isValid() && slot.oasisSlot.isValid())
			{
				WorldTile2 centerTile = slot.oasisSlot.centerTile;
				LocalTile2 localTile = centerTile.localTile(region);
				
				const ModuleTransformGroup& modulePrototype = displayInfo.GetDisplayModules(FactionEnum::Arab, CardEnum::Oasis, 0);
				FTransform transform(FRotator::ZeroRotator, localTile.localDisplayLocation());

				std::vector<ModuleTransform> modules = modulePrototype.transforms;
				_moduleMeshes[meshId]->Add(modules[0].moduleName, centerTile.tileId(), transform, 0); // Oasis MiniMesh
				_moduleMeshes[meshId]->Add(modules[1].moduleName, centerTile.tileId(), transform, 0); // Oasis MiniMeshWater
			}
		}

		_moduleMeshes[meshId]->AfterAdd();
	}

	// Farm display
	//buildingList.ExecuteRegion(WorldRegion2(regionId), [&](int32_t buildingId)
	//{
	//	Building& building = simulation().buildingSystem().building(buildingId);

	//	// Special case farm
	//	if (building.isEnum(CardEnum::Farm) && building.isConstructed()) {
	//		PunUnrealUtils::ShowDecal(building.area().trueCenterAtom(), building.buildingSize(), _farmDecals, _farmCount, this, _assetLoader->FarmDecalMaterial, gameManager());
	//	}
	//});
}

void UMiniBuildingDisplayComponent::HideDisplay(int meshId, int32 regionId)
{
	_moduleMeshes[meshId]->SetActive(false);
}