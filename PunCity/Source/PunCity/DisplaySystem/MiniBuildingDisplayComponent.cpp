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

		const TArray<FString>& moduleNames = _assetLoader->moduleNames();
		for (int i = 0; i < moduleNames.Num(); i++) 
		{
			// Only keep body and roof
			if (
				//moduleNames[i].Right(4) == FString("Body") ||
				//moduleNames[i].Right(4) == FString("Roof") ||
				// Special case for regional buildings
				//moduleNames[i] == FString("AncientShrineSpecial") ||
				//moduleNames[i] == FString("RegionCratePileSpecial")

				GameDisplayInfo::IsMiniModule(moduleNames[i]))
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

void UMiniBuildingDisplayComponent::UpdateDisplay(int regionId, int meshId, WorldAtom2 cameraAtom)
{
	auto& buildingList = simulation().buildingSystem().buildingSubregionList();
	BuildingSystem& buildingSystem = simulation().buildingSystem();
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
			const std::vector<BuildingUpgrade>& upgrades = building.upgrades();

			// Don't display road
			if (IsRoad(buildingEnum)) {
				return;
			}

			// Special case bridge
			if (buildingEnum == CardEnum::Bridge ||
				buildingEnum == CardEnum::Tunnel) 
			{
				return;
			}
			if (buildingEnum == CardEnum::BoarBurrow) {
				return;
			}

			float displayVariationIndex = building.displayVariationIndex();
			int32_t buildingRotation = RotationFromDirection(building.faceDirection());

			WorldTile2 centerTile = building.centerTile();
			FTransform transform(FRotator(0, buildingRotation, 0), centerTile.localTile().localDisplayLocation());

			const ModuleTransforms& modulePrototype = displayInfo.GetDisplayModules(buildingEnum, displayVariationIndex);
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