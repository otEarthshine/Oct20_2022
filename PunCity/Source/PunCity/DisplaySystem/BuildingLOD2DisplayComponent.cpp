// Pun Dumnernchanvanit's


#include "BuildingLOD2DisplayComponent.h"

void UBuildingLOD2DisplayComponent::InitAnnotations()
{
	_buildingsMeshes->Init("BuildingModules_Map", this, 100, "", 0);

	const TArray<FName>& moduleNames = _assetLoader->moduleNames();
	for (int i = 0; i < moduleNames.Num(); i++) {
		UStaticMesh* protoMesh = _assetLoader->moduleMesh(moduleNames[i]);
		if (protoMesh) {
			if (GameDisplayInfo::IsMiniModule(moduleNames[i])) {
				_buildingsMeshes->AddProtoMesh(moduleNames[i], protoMesh, nullptr);
			}
		}
	}
}

void UBuildingLOD2DisplayComponent::RefreshAnnotations()
{
	_LOG(PunDisplay, "RefreshAnnotations");
	SCOPE_TIMER("RefreshAnnotations");

	/*
	 * Buildings
	 */
	const GameDisplayInfo& displayInfo = gameManager()->displayInfo();

	auto& sim = gameManager()->simulation();

#if DISPLAY_WORLDMAP_BUILDING
	
	int32 buildingCount = sim.buildingSystem().buildingCount();

	for (int32 buildingId = 0; buildingId < buildingCount; buildingId++)
	{
		if (sim.IsValidBuilding(buildingId))
		{
			Building& building = sim.building(buildingId);
			FactionEnum factionEnum = building.factionEnum();
			CardEnum buildingEnum = building.buildingEnum();

			if (building.isConstructed() && displayInfo.GetVariationCount(factionEnum, buildingEnum) > 0)
			{
				// Building mesh
				int32 displayVariationIndex = building.displayVariationIndex();
				float buildingRotation = RotationFromDirection(building.displayFaceDirection());


				WorldTile2 centerTile = building.displayCenterTile();
				FVector displayLocation(centerTile.x * CoordinateConstants::DisplayUnitPerTile + 5.0f,
										centerTile.y * CoordinateConstants::DisplayUnitPerTile + 5.0f, 0);
				//FVector displayLocation(centerTile.x * _tileToWorldMapX, centerTile.y * _tileToWorldMapY, 0);

				//FVector displayLocation(0, 0, 0);

				FTransform transform(FRotator(0, buildingRotation, 0), displayLocation);
				//_dataSource->DisplayLocation(centerTile.worldAtom2())

				const ModuleTransformGroup& modulePrototype = displayInfo.GetDisplayModules(factionEnum, buildingEnum, displayVariationIndex);
				std::vector<ModuleTransform> modules = modulePrototype.miniModules;

				// Special buildings
				for (int32 i = 0; i < modules.size(); i++)
				{
					int32 instanceKey = centerTile.tileId() + i * GameMapConstants::TilesPerWorld;

					FTransform finalTransform;
					FTransform::Multiply(&finalTransform, &modules[i].transform, &transform);

					//PUN_LOG("ModuleDisplay %s, %s", *finalTransform.GetRotation().Rotator().ToString(), *modules[i].transform.GetRotation().Rotator().ToString());
					_buildingsMeshes->Add(modules[i].moduleName, instanceKey, finalTransform, 0, buildingId);
				}

			}
		}
	}

	
	//sim.ExecuteOnPlayersAndAI([&](int32 playerId)
	//{
	//	for (int32 j = 0; j < BuildingEnumCount; j++)
	//	{
	//		CardEnum buildingEnum = static_cast<CardEnum>(j);

	//		// Don't display road
	//		if (IsRoad(buildingEnum)) {
	//			return;
	//		}

	//		const std::vector<int32> buildingIds = sim.buildingIds(playerId, buildingEnum);

	//		//PUN_LOG("RefreshAnnotations count:%d", buildingIds.size());

	//		for (int32 buildingId : buildingIds)
	//		{
	//			Building& building = sim.building(buildingId);
	//			FactionEnum factionEnum = building.factionEnum();

	//			if (building.isConstructed() && displayInfo.GetVariationCount(factionEnum, buildingEnum) > 0)
	//			{
	//				// Building mesh
	//				int32 displayVariationIndex = building.displayVariationIndex();
	//				float buildingRotation = RotationFromDirection(building.displayFaceDirection());


	//				WorldTile2 centerTile = building.displayCenterTile();
	//				FVector displayLocation(centerTile.x * CoordinateConstants::DisplayUnitPerTile,
	//					centerTile.y * CoordinateConstants::DisplayUnitPerTile, 0);
	//				//FVector displayLocation(centerTile.x * _tileToWorldMapX, centerTile.y * _tileToWorldMapY, 0);

	//				//FVector displayLocation(0, 0, 0);

	//				FTransform transform(FRotator(0, buildingRotation, 0), displayLocation);
	//				//_dataSource->DisplayLocation(centerTile.worldAtom2())

	//				const ModuleTransformGroup& modulePrototype = displayInfo.GetDisplayModules(factionEnum, buildingEnum, displayVariationIndex);
	//				std::vector<ModuleTransform> modules = modulePrototype.miniModules;

	//				auto showMesh = [&](int32 i) {
	//					int32 instanceKey = centerTile.tileId() + i * GameMapConstants::TilesPerWorld;

	//					FTransform finalTransform;
	//					FTransform::Multiply(&finalTransform, &modules[i].transform, &transform);

	//					//PUN_LOG("ModuleDisplay %s, %s", *finalTransform.GetRotation().Rotator().ToString(), *modules[i].transform.GetRotation().Rotator().ToString());
	//					_buildingsMeshes->Add(modules[i].moduleName, instanceKey, finalTransform, 0, buildingId);
	//				};

	//				// Special buildings
	//				for (int32 i = 0; i < modules.size(); i++)
	//				{
	//					showMesh(i);

	//					//if (modules[i].moduleName.Right(4) == FString("Body") ||
	//					//	modules[i].moduleName.Right(4) == FString("Roof") ||
	//					//	FStringCompareRight(modules[i].moduleName, FString("AlwaysOn")))
	//					//{
	//					//	showMesh(i);
	//					//}
	//				}

	//			}
	//		}
	//	}


	//});


	// Oasis
	const std::vector<int32>& oasisSlotProvinceIds = sim.provinceInfoSystem().oasisSlotProvinceIds();
	for (int32 oasisProvinceId : oasisSlotProvinceIds)
	{
		const ProvinceBuildingSlot& slot = sim.provinceInfoSystem().provinceBuildingSlot(oasisProvinceId);
		if (slot.isValid() && slot.oasisSlot.isValid())
		{
			WorldTile2 centerTile = slot.oasisSlot.centerTile;
			FVector displayLocation(centerTile.x * CoordinateConstants::DisplayUnitPerTile, centerTile.y * CoordinateConstants::DisplayUnitPerTile, 10);

			const ModuleTransformGroup& modulePrototype = displayInfo.GetDisplayModules(FactionEnum::Arab, CardEnum::Oasis, 0);
			FTransform transform(FRotator::ZeroRotator, displayLocation);

			std::vector<ModuleTransform> modules = modulePrototype.transforms;
			_buildingsMeshes->Add(modules[0].moduleName, centerTile.tileId(), transform, 0); // Oasis MiniMesh
			_buildingsMeshes->Add(modules[1].moduleName, centerTile.tileId(), transform, 0); // Oasis MiniMeshWater
		}
	}

#endif

	_buildingsMeshes->AfterAdd();
	bool mapCityVisible = !gameManager()->ZoomDistanceBelow(WorldZoomTransition_BuildingsMini);
	_buildingsMeshes->SetActive(mapCityVisible, true);



}
