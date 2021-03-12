#include "BuildingDisplayComponent.h"

#include "Materials/MaterialInstanceDynamic.h"
//#include "Components/StaticMeshComponent.h"
//#include "PunCity/Simulation/Resource/ResourceSystem.h"
//#include "PunCity/Simulation/Buildings/TownHall.h"
//#include "PunCity/Simulation/Buildings/House.h"
#include "PunCity/Simulation/Buildings/GathererHut.h"
#include "PunCity/PunGameSettings.h"
#include "Components/LineBatchComponent.h"
#include "Components/BoxComponent.h"
#include "Components/PointLightComponent.h"
#include "Components/DecalComponent.h"
#include "PunCity/Simulation/OverlaySystem.h"
#include "Materials/MaterialInterface.h"

#include "Components/HierarchicalInstancedStaticMeshComponent.h"
#include "Engine/StaticMesh.h"
#include "PunCity/PunUtils.h"

#include <string>
#include "PunCity/PunTimer.h"

DECLARE_CYCLE_STAT(TEXT("PUN: [Display]Building_CoreSpawn"), STAT_PunDisplayBuilding_CoreSpawn, STATGROUP_Game);

DECLARE_CYCLE_STAT(TEXT("PUN: [Display]Building_Core"), STAT_PunDisplayBuilding_Core, STATGROUP_Game);
DECLARE_CYCLE_STAT(TEXT("PUN: [Display]Building_Particles"), STAT_PunDisplayBuilding_Particles, STATGROUP_Game);
DECLARE_CYCLE_STAT(TEXT("PUN: [Display]BuildingAnim_Core"), STAT_PunDisplayBuildingAnim_Core, STATGROUP_Game);
DECLARE_CYCLE_STAT(TEXT("PUN: [Display]Building_Farm"), STAT_PunDisplayBuilding_Farm, STATGROUP_Game);

using namespace std;

void UBuildingDisplayComponent::ShowRadius(int32 radius, WorldAtom2 centerAtom, Building& building)
{
	LLM_SCOPE_(EPunSimLLMTag::PUN_DisplayBuilding);
	
	int currentIndex = _radiusCount;
	_radiusCount++;
	//PUN_LOG("BldDisp ShowRadius %d", radius);

	//for (int32 i = _radiusDecals.Num(); i < _radiusCount; i++) {
	//	_radiusDecals.Add(PunUnrealUtils::CreateDecal(this, _assetLoader->RadiusMaterial));
	//}
	for (int32 i = _radiusMeshes.Num(); i < _radiusCount; i++) {
		UStaticMeshComponent* meshComp = PunUnrealUtils::CreateStaticMesh(this);
		meshComp->SetStaticMesh(_assetLoader->RadiusMesh);
		_radiusMeshes.Add(meshComp);
	}
	
	FVector displayLocation = gameManager()->DisplayLocation(centerAtom); //MapUtil::DisplayLocation(_cameraAtom, centerAtom);
	//PUN_LOG("displayLocation %s", *displayLocation.ToString());

	//PunUnrealUtils::SetActive(_radiusDecals[currentIndex], true);
	//_radiusDecals[currentIndex]->SetWorldLocation(displayLocation);
	//_radiusDecals[currentIndex]->SetWorldScale3D(FVector(1, 2 * radius, 2 * radius));

	PunUnrealUtils::SetActive(_radiusMeshes[currentIndex], true);
	_radiusMeshes[currentIndex]->SetWorldLocation(displayLocation);
	_radiusMeshes[currentIndex]->SetWorldScale3D(FVector::OneVector * radius);
	
	_radiusCount++;
}

void UBuildingDisplayComponent::AfterAdd()
{
	LLM_SCOPE_(EPunSimLLMTag::PUN_DisplayBuilding);
	
	PunUnrealUtils::UpdateMeshes(_radiusMeshes, _radiusCount);
	
	PunUnrealUtils::UpdateDecals(_farmDecals, _farmCount);
	PunUnrealUtils::UpdateDecals(_constructionBaseDecals, _constructionBaseCount);

	PunUnrealUtils::UpdateLights(_pointLights, _pointLightCount);


	for (int32 i = 0; i < _demolishInfos.size(); i++) {
		if (_demolishInfos[i].isInUse()) 
		{
			//float displayX = (area.maxX + area.minX) / 2.0f * CoordinateConstants::DisplayUnitPerTile - CoordinateConstants::DisplayUnitPerRegion * region.x;
			//float displayY = (area.maxY + area.minY) / 2.0f * CoordinateConstants::DisplayUnitPerTile - CoordinateConstants::DisplayUnitPerRegion * region.y;
			TileArea area = _demolishInfos[i].area;
			FVector displayLocation = MapUtil::DisplayLocation(_gameManager->cameraAtom(), area.centerTile().worldAtom2());
			_demolishParticlePool[i]->SetWorldLocation(displayLocation);

			if (_demolishParticlePool[i]->bWasCompleted) {
				_demolishParticlePool[i]->SetVisibility(false);
				_demolishInfos[i] = DemolishDisplayInfo();
			}
		}
	}

}

/*
 * Building meshes
 */

int UBuildingDisplayComponent::CreateNewDisplay(int objectId)
{
	LLM_SCOPE_(EPunSimLLMTag::PUN_DisplayBuilding);
	
	int meshId = _moduleMeshes.Num();

	// Modules
	{
		_moduleMeshes.Add(NewObject<UStaticFastInstancedMeshesComp>(this));
		_moduleMeshes[meshId]->Init("BuildingModules" + to_string(meshId) + "_", this, 20, "", meshId);

		_lastModuleMeshesOverlayType.push_back(OverlayType::None);

		const TArray<FString>& moduleNames = _assetLoader->moduleNames();
		
		//PUN_CHECK(_assetLoader->moduleMesh("Townhall3Special2"));
		//PUN_LOG("CreateNewDisplay _moduleNames %d", moduleNames.Num());
		//PUN_CHECK(moduleNames.Num() == ModuleMeshCount);
		
		for (int i = 0; i < moduleNames.Num(); i++) {
			UStaticMesh* protoMesh = _assetLoader->moduleMesh(moduleNames[i]);
			PUN_CHECK(protoMesh);
			if (protoMesh) {
				_moduleMeshes[meshId]->AddProtoMesh(moduleNames[i], protoMesh, nullptr);

				// Add burnt frame
				if (moduleNames[i].Len() > 5 && moduleNames[i].Right(5) == FString("Frame")) {
					_moduleMeshes[meshId]->AddProtoMesh(moduleNames[i] + FString("Burnt"), protoMesh, _assetLoader->M_WoodFrameBurned);
				}
			}
		}

	}

	{
		_togglableModuleMeshes.Add(NewObject<UStaticFastInstancedMeshesComp>(this));
		_togglableModuleMeshes[meshId]->Init("BuildingTogglableModules" + to_string(meshId) + "_", _moduleMeshes[meshId], 20, "", meshId);

		const TArray<FString>& moduleNames = _assetLoader->togglableModuleNames();
		for (int i = 0; i < moduleNames.Num(); i++) {
			UStaticMesh* protoMesh = _assetLoader->moduleMesh(moduleNames[i]);
			PUN_CHECK(protoMesh);
			_togglableModuleMeshes[meshId]->AddProtoMesh(moduleNames[i], protoMesh, nullptr);
		}
	}

	//_particles.Add(NewObject<UStaticParticleSystemsComponent>(this));
	//_particles[meshId]->Init(FString("Particles") + FString::FromInt(meshId), _moduleMeshes[meshId], 50);

	_particles.Add(NewObject<UStaticParticleSystemsComponent>(this));
	//_particles[meshId]->AttachToComponent(_moduleMeshes[meshId], FAttachmentTransformRules::KeepRelativeTransform, NAME_None);
	_particles[meshId]->AttachToComponent(this, FAttachmentTransformRules::KeepRelativeTransform, NAME_None);
	_particles[meshId]->parent = _moduleMeshes[meshId];

	//// Attach Light
	//UPointLightComponent* light = NewObject<UPointLightComponent>(this, UPointLightComponent::StaticClass());
	//light->AttachToComponent(meshComp, FAttachmentTransformRules::KeepRelativeTransform, NAME_None);
	//light->IntensityUnits = ELightUnits::Candelas;
	//light->RegisterComponent();
	//_lights.Add(light);

	return meshId;
}

void UBuildingDisplayComponent::OnSpawnDisplay(int objectId, int meshId, WorldAtom2 cameraAtom)
{
	LLM_SCOPE_(EPunSimLLMTag::PUN_DisplayBuilding);
	
	//	UPointLightComponent* light = _lights[meshId];
	//	if (building.isEnum(BuildingEnum::IronSmelter)) {
	//		light->SetIntensity(1.0f);
	//		light->SetRelativeLocation(FVector(-3, -6, 7));
	//		light->SetLightColor(FLinearColor(1, .14f, 0));
	//		light->SetAttenuationRadius(10);
	//		light->SetSourceRadius(5);
	//		light->SetCastShadows(true);
	//	}
	//	else {
	//		light->SetIntensity(0.02f);
	//		light->SetRelativeLocation(FVector(-12, 10, 10));
	//		light->SetLightColor(FLinearColor(1, 0.283f, 0));
	//		light->SetAttenuationRadius(20);
	//		light->SetSourceRadius(0);
	//		light->SetCastShadows(false);
	//	}
	//	light->SetVisibility(false);
	//}

	_moduleMeshes[meshId]->SetActive(true);
	_togglableModuleMeshes[meshId]->SetActive(true);

	_particles[meshId]->SetClusterActive(true);

	simulation().SetNeedDisplayUpdate(DisplayClusterEnum::Building, WorldRegion2(objectId).regionId(), true);
	simulation().SetNeedDisplayUpdate(DisplayClusterEnum::BuildingAnimation, WorldRegion2(objectId).regionId(), true);
}

void UBuildingDisplayComponent::UpdateDisplay(int regionId, int meshId, WorldAtom2 cameraAtom, bool justSpawned, bool justCreated)
{
	LLM_SCOPE_(EPunSimLLMTag::PUN_DisplayBuilding);
	
	auto& buildingList = simulation().buildingSystem().buildingSubregionList();
	BuildingSystem& buildingSystem = simulation().buildingSystem();
	WorldRegion2 region(regionId);
	FVector regionDisplayLocation = gameManager()->DisplayLocation(region.worldAtom2()); //MapUtil::DisplayLocation(cameraAtom, region.worldAtom2());
	
	{
		//SCOPE_TIMER_("Tick Building Move %d", _moduleMeshes[meshId]->meshPool.Num());
		_moduleMeshes[meshId]->SetRelativeLocation(regionDisplayLocation + FVector(5, 5, 0));


		// TODO: Why need this, and not just attach to _moduleMeshes?
		_particles[meshId]->SetRelativeLocation(regionDisplayLocation + FVector(5, 5, 0));
	}
	
	OverlayType overlayType = gameManager()->GetOverlayType();
	const GameDisplayInfo& displayInfo = gameManager()->displayInfo();

	if (simulation().NeedDisplayUpdate(DisplayClusterEnum::Building, regionId) || isMainMenuDisplay)
	{
		//PUN_LOG("--- Display Construction NeedDisplayUpdate regionId:%d", regionId);
		
		//SCOPE_TIMER("Tick Building: Building");
		SCOPE_CYCLE_COUNTER(STAT_PunDisplayBuilding_Core);

		// Ignore selected building
		int32 selectedBuildingId = -1;
		if (!gameManager()->isPhotoMode() &&
			simulation().descriptionUIState().objectType == ObjectTypeEnum::Building) 
		{
			selectedBuildingId = simulation().descriptionUIState().objectId;
		}

		buildingList.ExecuteRegion(region, [&](int32 buildingId)
		{
			Building& building = buildingSystem.building(buildingId);
			CardEnum buildingEnum = building.buildingEnum();
			const std::vector<BuildingUpgrade>& upgrades = building.upgrades();

			//PUN_LOG(" -- Display Construction[%d] (1) %s construct:%d", building.buildingId(), ToTChar(building.buildingInfo().name), building.constructionPercent());

			// Don't display road
			if (IsRoad(buildingEnum)) {
				return;
			}

			// Special case bridge
			if (buildingEnum == CardEnum::Bridge ||
				buildingEnum == CardEnum::IntercityBridge)
			{
				std::vector<GameDisplayUtils::BridgeModule> bridgeModules = GameDisplayUtils::GetBridgeModules(building.area());

				for (int i = 0; i < bridgeModules.size(); i++) {
					int32 instanceKey = building.centerTile().tileId() + i * GameMapConstants::TilesPerWorld;
					FTransform transform(FRotator(0, bridgeModules[i].rotation, 0), bridgeModules[i].tile.localTile(region).localDisplayLocation());
					_moduleMeshes[meshId]->Add(bridgeModules[i].moduleName, instanceKey, transform, 0, buildingId);
				}
				
				return;
			}

			// Special case Tunnel
			if (buildingEnum == CardEnum::Tunnel)
			{
				TileArea area = building.area();

				auto spawnEntrance = [&](WorldTile2 tile, int32 rotationInt) {
					int32 instanceKey = tile.tileId();
					FTransform transform(FRotator(0, rotationInt, 0), tile.localTile(region).localDisplayLocation());
					_moduleMeshes[meshId]->Add(FString("Tunnel"), instanceKey, transform, 0, buildingId);
				};

				int32 rotationShift = (area.sizeX() > 1) ? 0 : 90;
				
				spawnEntrance(area.min(), rotationShift);
				spawnEntrance(area.max(), rotationShift + 180);
				
				return;
			}

			// Building mesh
			float buildingRotation;
			int32 displayVariationIndex;
			if (buildingEnum == CardEnum::Fence) {
				pair<GridConnectType, int8_t> connectInfo = GetGridConnectType(building.centerTile());
				displayVariationIndex = static_cast<int32_t>(connectInfo.first);
				buildingRotation = connectInfo.second * 90;
			}
			else if (buildingEnum == CardEnum::FenceGate) {
				displayVariationIndex = building.displayVariationIndex();
				buildingRotation = GetGridConnectType(building.centerTile(), true).second * 90 - 90; // Wall has 0 rotation as up/down. Gate must be adjusted since 0 rotation is right/left.
			}
			else {
				displayVariationIndex = building.displayVariationIndex();
				buildingRotation = RotationFromDirection(building.faceDirection());
			}

			WorldTile2 centerTile = building.centerTile();
			FTransform transform(FRotator(0, buildingRotation, 0), centerTile.localTile().localDisplayLocation());

			//PUN_LOG("GetModules %s %d", *ToFString(building.buildingInfo().name), building.displayVariationIndex());

			const ModuleTransforms& modulePrototype = displayInfo.GetDisplayModules(buildingEnum, displayVariationIndex);
			std::vector<ModuleTransform> modules = modulePrototype.transforms;

			if (building.isBurnedRuin())
			{
				WorldTile2 size = building.buildingSize();
				FVector siteMarkingScale(size.x, size.y, 1);

				FVector position(size.x % 2 == 0 ? 5 : 0, size.y % 2 == 0 ? 5 : 0, 0.0f);
				FTransform siteMarkTransform(FRotator::ZeroRotator, position, siteMarkingScale);
				
				float constructionFraction = 0.5f;
				int32 constructionPercent = constructionFraction * 100;
				int32_t instanceKey = centerTile.tileId();

				// Add burned ground
				{
					FTransform finalTransform;
					FTransform::Multiply(&finalTransform, &siteMarkTransform, &transform);
					_moduleMeshes[meshId]->Add("BuildingBaseBurnt", instanceKey, finalTransform, constructionPercent, buildingId);
					instanceKey += GameMapConstants::TilesPerWorld;
				}
				
				// Add burned frame
				for (size_t i = 0; i < modules.size(); i++) {
					FString& moduleName = modules[i].moduleName;
					if (moduleName.Len() > 5 && moduleName.Right(5) == FString("Frame")) {
						_moduleMeshes[meshId]->Add(moduleName + FString("Burnt"), instanceKey, transform, constructionPercent, buildingId);
						break;
					}
				}
			}
			else if (building.isConstructed())
			{
				// If it is selected, ObjectDescriptionUI would display it instead...
				if (buildingId == selectedBuildingId) {
					return;
				}

				// Special case: Storage
				if (buildingEnum == CardEnum::StorageYard)
				{
					TileArea area = building.area();

					for (int32 x = 0; x < area.sizeX(); x += 2) {
						for (int32 y = 0; y < area.sizeY(); y += 2)
						{
							WorldTile2 tile(x + area.minX, y + area.minY);
							// Only display the even tiles ,shifted by 0.5

							int32 instanceKey = tile.tileId();
							FTransform storageTransform(FRotator(0, 0, 0), tile.localTile(region).localDisplayLocation() + FVector(5, 5, 0));
							_moduleMeshes[meshId]->Add("StorageTile", instanceKey, storageTransform, 0, buildingId);
						}
					}

					return;
				}
				
				for (int i = 0; i < modules.size(); i++) 
				{
					if (modules[i].moduleTypeEnum == ModuleTypeEnum::ConstructionOnly) {
						continue;
					}

					// if model requires an upgrade, don't show if that upgrade wasn't done...
					if (modules[i].upgradeStates > 0) {
						int32 showWhenUpgradedIndex = modules[i].upgradeStates - 1;
						if (!upgrades[showWhenUpgradedIndex].isUpgraded) {
							continue;
						}
					}
					// vice versa..
					else if (modules[i].upgradeStates < 0) {
						int32 dontShowWhenUpgradedIndex = -modules[i].upgradeStates - 1;
						if (upgrades[dontShowWhenUpgradedIndex].isUpgraded) {
							continue;
						}
					}

					//int32_t instanceKey = localTile.tileId() + i * CoordinateConstants::TileIdsPerRegion + i; // +i so it doesn't overlap
					int32_t instanceKey = centerTile.tileId() + i * GameMapConstants::TilesPerWorld;

					//float scaleScalar = modules[i].transform.GetScale3D().X;
					//if (modules[i].moduleTypeEnum == ModuleTypeEnum::Window && building.shouldDisplayAsWorking()) {
					//	// If there is no one in the building, shut the building's light
					//	scaleScalar = 0.5f;
					//}
					//
					//FTransform localTransform = FTransform(modules[i].transform.GetRotation(), 
					//										modules[i].transform.GetTranslation(), FVector(scaleScalar));

					FTransform finalTransform;
					FTransform::Multiply(&finalTransform, &modules[i].transform, &transform);

					//PUN_CHECK(_assetLoader->moduleMesh("Townhall3Special2"));
					//PUN_CHECK(_moduleMeshes[meshId]->GetMesh("Townhall3Special2"));

					//PUN_LOG("ModuleDisplay %s, %s", *finalTransform.GetRotation().Rotator().ToString(), *modules[i].transform.GetRotation().Rotator().ToString());
					_moduleMeshes[meshId]->Add(modules[i].moduleName, instanceKey, finalTransform, 0, buildingId);
				}
				
			}
			else 
			{
				float constructionFraction = building.constructionFraction();

				// Storage yard, don't display special
				if (building.isEnum(CardEnum::StorageYard)) {
					modules.clear();
				}

				// Construction site should have four poles and base marking the area
				{

					// make the site marking 1 tile, centered... so the shift can be done correctly...
					if (!IsPortBuilding(building.buildingEnum()) &&
						!building.isEnum(CardEnum::ClayPit) &&
						!building.isEnum(CardEnum::IrrigationReservoir) &&
						!building.isEnum(CardEnum::Farm) &&
						!building.isEnum(CardEnum::StorageYard)) 
					{
						WorldTile2 size = building.buildingSize();
						FVector siteMarkingScale(size.x, size.y, 1);

						FVector position(size.x % 2 == 0 ? 5 : 0, size.y % 2 == 0 ? 5 : 0, 0.0f);
						FTransform siteMarkTransform(FRotator::ZeroRotator, position, siteMarkingScale);
						
						// Outline the construction base before the land is cleared
						//  Note that this must be displayed for the depth-based outline to work.
						//  So we just need to make sure that the color of ConstructionBaseHighlight is as similar as possible to the decal
						//  Note also that if we always leave this on, the construction frame will always have weird outlines
						if (building.playerId() != -1 &&
							!simulation().IsLandCleared_SmallOnly(building.townId(), building.area())) 
						{
							modules.insert(modules.begin(), ModuleTransform("ConstructionBaseHighlight", siteMarkTransform, 0.0f, ModuleTypeEnum::ConstructionBaseHighlight));
						}
					}

					if (building.isEnum(CardEnum::Fisher)) {
						modules.insert(modules.begin(), ModuleTransform("FisherConstructionPoles", FTransform::Identity, 0.0f, ModuleTypeEnum::ConstructionOnly));
						modules.insert(modules.begin(), ModuleTransform("FisherConstructionPolesWater", FTransform::Identity, 0.0f, ModuleTypeEnum::ConstructionOnly));
					}
					else if (building.isEnum(CardEnum::TradingPort)) {
						modules.insert(modules.begin(), ModuleTransform("TradingPortConstructionPoles", FTransform::Identity, 0.0f, ModuleTypeEnum::ConstructionOnly));
						modules.insert(modules.begin(), ModuleTransform("TradingPortConstructionPolesWater", FTransform::Identity, 0.0f, ModuleTypeEnum::ConstructionOnly));
					}
					else {
						//GameDisplayUtils::GetConstructionPoles(building.area(), modules);
						
						//modules.insert(modules.begin(), ModuleTransform("ConstructionPoles", siteMarkTransform, 0.0f, ModuleTypeEnum::ConstructionOnly));
					}
				}

				// For buildings with only Special module (such as storage yard)
				//  Don't display the Special during construction
				//  Don't display the Special during construction
				//  TODO: remove this?
				if (modules.size() == 3 && modules[2].moduleTypeEnum != ModuleTypeEnum::Frame) {
					modules.pop_back();
				}

				//PUN_LOG("  - Display Construction[%d] (2) %s construct:%d", building.buildingId(), ToTChar(building.buildingInfo().name), building.constructionPercent());

				for (int i = 0; i < modules.size(); i++)
				{
					// Check if this mesh should be displayed during this construction phase
					if (modules[i].constructionFractionBegin <= constructionFraction)
					{
						FString moduleName = modules[i].moduleName;
						//int32_t instanceKey = localTile.tileId() + i * (CoordinateConstants::TileIdsPerRegion + 1); // +1 so it doesn't overlap (in hash?)
						int32_t instanceKey = centerTile.tileId() + i * GameMapConstants::TilesPerWorld;

						FTransform moduleTransform;
						FTransform::Multiply(&moduleTransform, &modules[i].transform, &transform);

						//if (moduleName.Equals(FString("DecorativeBasketBox"))) {
						//	PUN_LOG("%s transform: %s,  module: %s", *moduleName, *transform.ToString(), *modules[i].transform.ToString());
						//}

						// TODO: remove this???
						if (_assetLoader->HasConstructionMesh(moduleName)) {
							// Use construction mesh (construction poly + vertex color representing when the piece should appear)
							float moduleFraction = modules[i].moduleConstructionFraction(constructionFraction);
							moduleTransform.SetScale3D(FVector(1, 1, moduleFraction * 0.99f + 0.01f));
							int32 constructionPercent = constructionFraction * 100;
							_moduleMeshes[meshId]->Add(moduleName + "Construction", instanceKey, moduleTransform, constructionPercent, buildingId);

							//PUN_LOG("Construction Z:%f age:%d", transform.GetScale3D().Z, static_cast<int32>(constructionFraction * 100));
						}
						// This mesh has no pairing construction mesh and will just appear
						else 
						{
							if (modules[i].moduleTypeEnum == ModuleTypeEnum::Frame) {
								float moduleFraction = modules[i].moduleConstructionFraction(constructionFraction);
								moduleTransform.SetScale3D(FVector(1, 1, moduleFraction * 0.99f + 0.01f));
								int32 modulePercent = moduleFraction * 100;
								_moduleMeshes[meshId]->Add(moduleName, instanceKey, moduleTransform, modulePercent, buildingId);

								//PUN_LOG("    Display FrameConstruction[%d] module:%d construct:%d", building.buildingId(), modulePercent, building.constructionPercent());
								//PUN_LOG("FrameConstruction Z:%f age:%d", transform.GetScale3D().Z, constructionPercent);
							}
							//else if (modules[i].moduleTypeEnum == ModuleTypeEnum::ConstructionBaseHighlight) {
							//	_moduleMeshes[meshId]->Add(moduleName, instanceKey, moduleTransform, 0, buildingId, false, true);
							//}
							else {
								_moduleMeshes[meshId]->Add(moduleName, instanceKey, moduleTransform, 0, buildingId);
							}

						}
					}
					else
					{
						//if (modules[i].moduleTypeEnum == ModuleTypeEnum::Frame) {
							//PUN_LOG("    Display FrameConstruction[%d] (not ready) construct:%d", building.buildingId(), building.constructionPercent());
						//}
					}
				}
				
			}

		});

		// Hidden part highlight on base
		_moduleMeshes[meshId]->SetCustomDepth("ConstructionBaseHighlight", 4);
		_moduleMeshes[meshId]->SetReceivesDecals("ConstructionBaseHighlight", true);

		// TODO: TEMP test fisher
		_moduleMeshes[meshId]->SetCustomDepth("FisherConstructionPoles", 4);
		_moduleMeshes[meshId]->SetReceivesDecals("FisherConstructionPoles", true);
		
		_moduleMeshes[meshId]->SetCustomDepth("TradingPortConstructionPoles", 4);
		_moduleMeshes[meshId]->SetReceivesDecals("TradingPortConstructionPoles", true);

		_moduleMeshes[meshId]->AfterAdd();

		simulation().SetNeedDisplayUpdate(DisplayClusterEnum::Building, regionId, false);
	}
	

	/*
	 * Building Animation
	 */
	bool needBuildingAnimationUpdate = simulation().NeedDisplayUpdate(DisplayClusterEnum::BuildingAnimation, regionId) || 
										isMainMenuDisplay ||
										PunSettings::TrailerSession;

	// TODO: This is a hack to check every sec anyway
	//needBuildingAnimationUpdate = needBuildingAnimationUpdate || Time::Tick

	std::vector<DemolishDisplayInfo>* demolishInfosPtr = nullptr;
	if (!isMainMenuDisplay) {
		demolishInfosPtr = &(simulation().GetDemolishDisplayInfo(regionId));
	}

	// Note: Need this outer since particles are shared by 
	if (needBuildingAnimationUpdate || (demolishInfosPtr && demolishInfosPtr->size() > 0))
	{	
		// Building Animations (Smoke, flame etc.)
		//  Smoke rise when building has occupants
		//  State change when .. worker change
		// 
		//  For production building also need...
		//   - filled inputs (work on buildings without input too??)
		//  State change when .. just filled input, just finish batch
		if (needBuildingAnimationUpdate)
		{
			//SCOPE_TIMER("Tick Building: Animation");

			SCOPE_CYCLE_COUNTER(STAT_PunDisplayBuildingAnim_Core);

			buildingList.ExecuteRegion(WorldRegion2(regionId), [&](int32 buildingId)
			{
				Building& building = buildingSystem.building(buildingId);
				CardEnum buildingEnum = building.buildingEnum();

				// Don't display road
				if (IsRoad(buildingEnum)) {
					return;
				}

				bool shouldDisplayParticles = building.shouldDisplayParticles() || isMainMenuDisplay || PunSettings::TrailerSession;

				// Building mesh
				float buildingRotation = RotationFromDirection(building.faceDirection());
				int32 displayVariationIndex = building.displayVariationIndex();

				WorldTile2 centerTile = building.centerTile();
				FTransform transform(FRotator(0, buildingRotation, 0), centerTile.localTile().localDisplayLocation());

				//PUN_LOG("GetModules %s %d", *ToFString(building.buildingInfo().name), building.displayVariationIndex());

				const ModuleTransforms& modulePrototype = displayInfo.GetDisplayModules(buildingEnum, displayVariationIndex);

				/*
				 * Particles
				 */
				// TODO: Bring this back
				//{
				//	// Burning
				//	int32 startIndex = modulePrototype.particleInfos.size();
				//	if (building.isOnFire())
				//	{
				//		FTransform finalTransform;

				//		// scale based on 5x5 ranch
				//		{
				//			FVector scale = FVector(.4 * (building.buildingSize().x / 5.0f), .3 * (building.buildingSize().y / 5.0f), .4);
				//			FTransform fireTransform(FQuat::Identity, FVector::ZeroVector, scale);
				//			FTransform::Multiply(&finalTransform, &fireTransform, &transform);
				//			_particles[meshId]->Add(centerTile.tileId() + (startIndex)* GameMapConstants::TilesPerWorld, _assetLoader->particleSystem(ParticleEnum::BuildingFire), finalTransform, 0);
				//		}
				//		{
				//			FVector scale = FVector(3.2 * (building.buildingSize().x / 5.0f), 2.4 * (building.buildingSize().y / 5.0f), .5);
				//			FTransform fireSmokeTransform(FQuat::Identity, FVector::ZeroVector, scale);
				//			FTransform::Multiply(&finalTransform, &fireSmokeTransform, &transform);
				//			_particles[meshId]->Add(centerTile.tileId() + (startIndex + 1)* GameMapConstants::TilesPerWorld, _assetLoader->particleSystem(ParticleEnum::BuildingFireSmoke), finalTransform, 0);
				//		}
				//	}
				//}

				if (building.isConstructed() && shouldDisplayParticles)
				{
					const std::vector<ParticleInfo>& particleInfos = modulePrototype.particleInfos;
					for (int32 i = 0; i < particleInfos.size(); i++)
					{
						FTransform finalTransform;
						FTransform::Multiply(&finalTransform, &particleInfos[i].transform, &transform);

						// need to take into account variationIndex change... Assume max particleInfos of 5
						int32 particleKey = centerTile.tileId() + (i + 5 * displayVariationIndex) * GameMapConstants::TilesPerWorld;

						//PUN_CHECK(_particles[meshId] != nullptr);
						//_particles[meshId]->Add(particleKey, _assetLoader->particleSystem(particleInfos[i].particleEnum), finalTransform, 0);

						

						//// Determine transform
						//FVector buildingDisplayLocation = MapUtil::DisplayLocation(_gameManager->cameraAtom(), centerTile.worldAtom2());
						//FTransform buildingDisplayTransform(FRotator(0, buildingRotation, 0), buildingDisplayLocation);
						//FTransform finalTransform;
						//FTransform::Multiply(&finalTransform, &particleInfos[i].transform, &buildingDisplayTransform);


						// Move the particle to the correct location
						//comp->SetWorldTransform(finalTransform);

						_particles[meshId]->Add(particleKey, finalTransform, particleInfos[i].particleEnum, _assetLoader);
					}
				}

				/*
				 * Togglables
				 */
				if (building.isConstructed())
				{
					// Example, planks or mushroom log should be shown after work began, even if work was halted
					const std::vector<ModuleTransform>& modules = modulePrototype.togglableTransforms;
					for (int i = 0; i < modules.size(); i++)
					{
						int32 instanceKey = centerTile.tileId() + i * GameMapConstants::TilesPerWorld;

						// TODO: Window scale adjustment scheme doesn't work.. Solution: Need two type of instancedStaticMesh light/dark
						if (modules[i].moduleTypeEnum == ModuleTypeEnum::Window)
						{
							float scale = 1;

							//// Close the light when there is no occupant
							//if (building.occupantCount() == 0) {
							//	scale = 0.5f;
							//}

							FTransform localTransform(modules[i].transform.GetRotation(), modules[i].transform.GetTranslation(), FVector(scale));
							FTransform finalTransform;
							FTransform::Multiply(&finalTransform, &localTransform, &transform);

							_togglableModuleMeshes[meshId]->Add(modules[i].moduleName, instanceKey, finalTransform, scale > 0.7 ? 1 : 0, buildingId);
						}
						else
						{
							FTransform finalTransform;
							FTransform::Multiply(&finalTransform, &modules[i].transform, &transform);

							if (shouldDisplayParticles) {
								_togglableModuleMeshes[meshId]->Add(modules[i].moduleName, instanceKey, finalTransform, 0, buildingId);
							}
						}
					}
				}

				/*
				 * Sound: here so it sync with animation
				 * Note: Performance tested to have 0.0ms when separated
				 */
				if (!isMainMenuDisplay)
				{
					if (building.isConstructed() && shouldDisplayParticles) {
						simulation().soundInterface()->TryStartBuildingWorkSound(building);
					}
					else {
						simulation().soundInterface()->TryStopBuildingWorkSound(building);
					}
				}
			});

			// Note: _particles->AfterAdd() is quite costly even without particles, but after check, it seems related to DEBUG_EXPR
			_togglableModuleMeshes[meshId]->AfterAdd();
			_particles[meshId]->AfterAdd();

			simulation().SetNeedDisplayUpdate(DisplayClusterEnum::BuildingAnimation, regionId, false);
		}
		
		if (demolishInfosPtr && demolishInfosPtr->size() > 0)
		{
			SCOPE_TIMER_FILTER(5000, "Tick Building: Demolish count:%llu", demolishInfosPtr->size());
			
			auto& demolishInfos = *demolishInfosPtr;
			
			for (size_t i = demolishInfos.size(); i-- > 0;) 
			{
				// Trailer only show dust on DirtRoad
				if (PunSettings::TrailerSession && 
					demolishInfos[i].buildingEnum != CardEnum::DirtRoad)
				{
					demolishInfos.erase(demolishInfos.begin() + i);
					continue;
				}

				// Find and spawn
				for (int32 j = 0; j < _demolishInfos.size(); j++) {
					if (!_demolishInfos[j].isInUse()) {
						_demolishInfos[j] = demolishInfos[i];
						_demolishParticlePool[j]->SetVisibility(true);
						_demolishParticlePool[j]->Activate(true);
						
						FVector scale = FVector(demolishInfos[i].area.sizeX() / 4.0f, demolishInfos[i].area.sizeY() / 4.0f, 1.0f);
						_demolishParticlePool[j]->SetWorldScale3D(scale);
						break;
					}
				}
				
				//TileArea area = demolishInfos[i].area;
				//int32 demolishId = area.min().tileId();
				
				{
					//FVector scale = FVector(demolishInfos[i].area.sizeX() / 4.0f, demolishInfos[i].area.sizeY() / 4.0f, 1.0f);
					//{
					//	SCOPE_TIMER_FILTER(5000, "Tick Building _demolishParticles %d", _demolishParticles.Num());
					//	particleComp = UGameplayStatics::SpawnEmitterAtLocation(this, _assetLoader->particleSystem(ParticleEnum::DemolishDust),
					//													FVector::ZeroVector, FRotator::ZeroRotator, scale, false, EPSCPoolMethod::AutoRelease);
					//}
					//particleComp->AttachToComponent(_moduleMeshes[meshId], FAttachmentTransformRules::KeepRelativeTransform);
					//_demolishParticles.Add(demolishId, particleComp);
				}
				
				// Display within the region
				////  Note: somehow couldn't do this without SetWorldLocation()
				//float displayX = (area.maxX + area.minX) / 2.0f * CoordinateConstants::DisplayUnitPerTile - CoordinateConstants::DisplayUnitPerRegion * region.x;
				//float displayY = (area.maxY + area.minY) / 2.0f * CoordinateConstants::DisplayUnitPerTile - CoordinateConstants::DisplayUnitPerRegion * region.y;

				//particleComp->SetWorldLocation(FVector(displayX, displayY, 0) + regionDisplayLocation + FVector(5, 5, 0));
			}

			demolishInfos.clear();
		}
	}


	{
		//SCOPE_TIMER("Tick Building: Decals");
		
		// Decals, always update
		buildingList.ExecuteRegion(WorldRegion2(regionId), [&](int32_t buildingId)
		{
			Building& building = simulation().buildingSystem().building(buildingId);

			if (!building.isConstructed() &&
				!IsRoad(building.buildingEnum()))
			{
				if (!building.isEnum(CardEnum::ClayPit))
				{
					TileArea area = building.area();
					PunUnrealUtils::ShowDecal(area.trueCenterAtom(), area.size(), _constructionBaseDecals, _constructionBaseCount, this, _assetLoader->ConstructionBaseDecalMaterial, gameManager());
				}
			}

			// Special case farm
			if (building.isEnum(CardEnum::Farm))
			{
				if (building.isConstructed())
				{
					TileArea area = building.area();

					UDecalComponent* decals = PunUnrealUtils::ShowDecal(building.area().trueCenterAtom(), area.size(), _farmDecals, _farmCount, 
						this, _assetLoader->FarmDecalMaterial, gameManager(), true, 0.01f);
					auto farmMaterial = CastChecked<UMaterialInstanceDynamic>(decals->GetDecalMaterial());
					farmMaterial->SetScalarParameterValue("FarmTilesX", area.sizeX());
					farmMaterial->SetScalarParameterValue("FarmTilesY", area.sizeY());
				}
				return;
			}


			// Overlay
			{
				UpdateDisplayOverlay(building, overlayType);
			}

			// Light
			if (building.isConstructed())
			{
				UpdateDisplayLight(building);
				//const ModuleTransforms& modulePrototype = displayInfo.GetDisplayModules(building.buildingEnum(), building.displayVariationIndex());
				//const std::vector<PointLightInfo>& lightInfos = modulePrototype.lightInfos;
				//if (lightInfos.size() > 0)
				//{
				//	float buildingRotation = RotationFromDirection(building.faceDirection());
				//	FVector displayLocation = gameManager()->DisplayLocation(building.centerTile().worldAtom2());
				//	FTransform transform(FRotator(0, buildingRotation, 0), displayLocation);

				//	for (const PointLightInfo& lightInfo : lightInfos)
				//	{
				//		FTransform localLightTransform(FRotator::ZeroRotator, lightInfo.position, lightInfo.scale);

				//		FTransform finalTransform;
				//		FTransform::Multiply(&finalTransform, &localLightTransform, &transform);

				//		UPointLightComponent* pointLight = PunUnrealUtils::ShowLight(finalTransform, _pointLights, _pointLightCount, this);

				//		pointLight->SetIntensityUnits(ELightUnits::Candelas);
				//		pointLight->SetIntensity(lightInfo.intensity);
				//		pointLight->SetLightColor(lightInfo.color);
				//		pointLight->SetAttenuationRadius(lightInfo.attenuationRadius);

				//		//pointLight->SetIntensity(100);
				//		//pointLight->SetAttenuationRadius(100);
				//	}
				//}
			}
			
			// Debug
			if (PunSettings::Settings["BuildingLines"])
			{
				FVector start = gameManager()->DisplayLocation(building.centerTile().worldAtom2());
				lineBatch()->DrawLine(start, start + FVector(0, 0, 100), FLinearColor(1, 0, 0), 100.0f, 1.0f, 10000);

				start = gameManager()->DisplayLocation(building.gateTile().worldAtom2());
				lineBatch()->DrawLine(start, start + FVector(0, 0, 100), FLinearColor(0, 0, 1), 100.0f, 1.0f, 10000);
			}
		});
	}

	// Highlight all object of type
	if (_lastModuleMeshesOverlayType[meshId] != overlayType)
	{
		SCOPE_TIMER("Tick Building: SetHighlight");
		
		SetHighlight(_moduleMeshes[meshId], CardEnum::Fisher, overlayType == OverlayType::Fish);
		SetHighlight(_moduleMeshes[meshId], CardEnum::FruitGatherer, overlayType == OverlayType::Gatherer);
		SetHighlight(_moduleMeshes[meshId], CardEnum::HuntingLodge, overlayType == OverlayType::Hunter);
		
		SetHighlight(_moduleMeshes[meshId], CardEnum::Forester, overlayType == OverlayType::Forester);
		SetHighlight(_moduleMeshes[meshId], CardEnum::Windmill, overlayType == OverlayType::Windmill);

		SetHighlight(_moduleMeshes[meshId], CardEnum::IrrigationReservoir, overlayType == OverlayType::IrrigationReservoir);
		SetHighlight(_moduleMeshes[meshId], CardEnum::Market, overlayType == OverlayType::Market);
		SetHighlight(_moduleMeshes[meshId], CardEnum::ShippingDepot, overlayType == OverlayType::ShippingDepot);
		
		SetHighlight(_moduleMeshes[meshId], CardEnum::Beekeeper, overlayType == OverlayType::Beekeeper);

		SetHighlight(_moduleMeshes[meshId], CardEnum::Library, overlayType == OverlayType::Library);
		SetHighlight(_moduleMeshes[meshId], CardEnum::School, overlayType == OverlayType::School);
		SetHighlight(_moduleMeshes[meshId], CardEnum::Bank, overlayType == OverlayType::Bank);

		SetHighlight(_moduleMeshes[meshId], CardEnum::Granary, overlayType == OverlayType::Granary);
		SetHighlight(_moduleMeshes[meshId], CardEnum::HaulingServices, overlayType == OverlayType::HaulingServices);

		SetHighlight(_moduleMeshes[meshId], CardEnum::Theatre, overlayType == OverlayType::Theatre);
		SetHighlight(_moduleMeshes[meshId], CardEnum::Tavern, overlayType == OverlayType::Tavern);

		_lastModuleMeshesOverlayType[meshId] = overlayType;
	}
}

pair<GridConnectType, int8_t> UBuildingDisplayComponent::GetGridConnectType(WorldTile2 tile, bool isGate)
{
	bool top = false;
	bool bottom = false;
	bool right = false;
	bool left = false;
	int connectCount = 0;

	WorldTile2 topTile = tile + WorldTile2(1, 0);
	WorldTile2 rightTile = tile + WorldTile2(0, 1);
	WorldTile2 leftTile = tile + WorldTile2(0, -1);
	WorldTile2 bottomTile = tile + WorldTile2(-1, 0);

	// Gate's road special case
	if (isGate) {
		const auto& overlaySys = simulation().overlaySystem();
		if (overlaySys.IsRoad(topTile) || overlaySys.IsRoad(bottomTile)) {
			return { GridConnectType::Opposite, 1 };
		}
		if (overlaySys.IsRoad(rightTile) || overlaySys.IsRoad(leftTile)) {
			return { GridConnectType::Opposite, 0 };
		}
	}

	// Calc bools
	if (topTile.x < GameMapConstants::TilesPerWorldX) {
		Building* bld = simulation().buildingAtTile(topTile);
		top = (bld != nullptr) && (bld->isEnum(CardEnum::Fence) || bld->isEnum(CardEnum::FenceGate));
		connectCount += top;
	}
	if (rightTile.y < GameMapConstants::TilesPerWorldY) {
		Building* bld = simulation().buildingAtTile(rightTile);
		right = (bld != nullptr) && (bld->isEnum(CardEnum::Fence) || bld->isEnum(CardEnum::FenceGate));
		connectCount += right;
	}
	if (leftTile.y >= 0) {
		Building* bld = simulation().buildingAtTile(leftTile);
		left = (bld != nullptr) && (bld->isEnum(CardEnum::Fence) || bld->isEnum(CardEnum::FenceGate));
		connectCount += left;
	}
	if (bottomTile.y >= 0) {
		Building* bld = simulation().buildingAtTile(bottomTile);
		bottom = (bld != nullptr) && (bld->isEnum(CardEnum::Fence) || bld->isEnum(CardEnum::FenceGate));
		connectCount += bottom;
	}

	// Each GridConnectType's default... Clockwise, first arm start on +x
	// { GridConnectType::Three, 2 } means it rotates 2 times from default

	if (connectCount == 4) {
		return { GridConnectType::Four, 0 };
	}
	if (connectCount == 3) {
		if (!left) {
			return { GridConnectType::Three, 0 };
		}
		if (!top) {
			return { GridConnectType::Three, 1 };
		}
		if (!right) {
			return { GridConnectType::Three, 2 };
		}
		else {
			check(!bottom);
			return { GridConnectType::Three, 3 };
		}
	}
	if (connectCount == 2) {
		// Opposite
		if (top && bottom) {
			return { GridConnectType::Opposite, 0 };
		}
		if (left && right) {
			return { GridConnectType::Opposite, 1 };
		}
		// Adjacent
		if (top && right) {
			return { GridConnectType::Adjacent, 0 };
		}
		if (right && bottom) {
			return { GridConnectType::Adjacent, 1 };
		}
		if (bottom && left) {
			return { GridConnectType::Adjacent, 2 };
		}
		if (left && top) {
			return { GridConnectType::Adjacent, 3 };
		}
		else {
			UE_DEBUG_BREAK();
		}
	}
	else if (connectCount == 1) {
		if (top || bottom) {
			return { GridConnectType::Opposite, 0 };
		}
		return { GridConnectType::Opposite, 1 };
	}
	else if (connectCount == 0) {
		return { GridConnectType::Opposite, 1 };
	}

	UE_DEBUG_BREAK();
	return pair<GridConnectType, int8_t>();
}


void UBuildingDisplayComponent::UpdateDisplayBuilding(int objectId, int meshId, WorldAtom2 cameraAtom)
{
	LLM_SCOPE_(EPunSimLLMTag::PUN_DisplayBuilding);
	
	//BuildingSystem& buildingSystem = simulation()->buildingSystem();
	//Building& building = buildingSystem.building(objectId);
	//uint8_t buildingEnumInt = building.buildingEnumInt();

	//// Location and rotation
	//WorldAtom2 actualLocation = buildingSystem.actualAtomLocation(objectId);
	//FVector displayLocation = MapUtil::DisplayLocation(cameraAtom, actualLocation);

	//bool isWall = building.buildingEnum() != BuildingEnum::Fence ||
	//	building.buildingEnum() != BuildingEnum::FenceGate;

	//{
	//	if (building.isConstructed()) 
	//	{
	//		// IronSmelter/Other turning on and off
	//		UPointLightComponent* light = _lights[meshId];
	//		if (building.isEnum(BuildingEnum::IronSmelter)) {
	//			light->SetVisibility(Time::Ticks() - building.lastWorkedOn < 900);
	//			light->SetIntensity(max(sinf((float)(Time::Ticks() % 220) / 220.0f),
	//				sinf((float)(Time::Ticks() % 370) / 370.0f)));
	//		}
	//		else if (building.isEnum(BuildingEnum::BoarBurrow)) {
	//			light->SetVisibility(false);
	//		}
	//		else {
	//			light->SetVisibility(building.occupantCount() > 0);
	//		}
	//	}
	//}
}


void UBuildingDisplayComponent::UpdateDisplayOverlay(Building& building, OverlayType overlayType)
{
	WorldAtom2 centerAtom = building.centerTile().worldAtom2();

#define SHOW_RADIUS(CardEnumName) \
	else if (overlayType == OverlayType::CardEnumName && building.isEnum(CardEnum::CardEnumName)) { \
		ShowRadius(CardEnumName::Radius, centerAtom, building); \
	}
	
	if (overlayType == OverlayType::Fish && building.isEnum(CardEnum::Fisher)) {
		//if (!building.IsUpgraded(0)) { // Skip whale
			//PUN_LOG("BldDisp Fish Overlay: %s", *ToFString(building.debugStr()));
			ShowRadius(Fisher::Radius, centerAtom, building);
		//}
	}
	else if (overlayType == OverlayType::Gatherer && building.isEnum(CardEnum::FruitGatherer)) {
		//PUN_LOG("BldDisp Gatherer Overlay: %d", buildingId);
		ShowRadius(GathererHut::Radius, centerAtom, building);
	}
	else if (overlayType == OverlayType::Hunter && building.isEnum(CardEnum::HuntingLodge)) {
		ShowRadius(HuntingLodge::Radius, centerAtom, building);
	}
	else if (overlayType == OverlayType::Forester && building.isEnum(CardEnum::Forester)) {
		ShowRadius(Forester::Radius, centerAtom, building);
	}
	else if (overlayType == OverlayType::Windmill && building.isEnum(CardEnum::Windmill)) {
		ShowRadius(Windmill::Radius, centerAtom, building);
	}
	
	SHOW_RADIUS(IrrigationReservoir)
	SHOW_RADIUS(Market)
	SHOW_RADIUS(ShippingDepot)
	
	else if (overlayType == OverlayType::Beekeeper && building.isEnum(CardEnum::Beekeeper)) {
		ShowRadius(Beekeeper::Radius, centerAtom, building);
	}
	//else if (overlayType == OverlayType::ConstructionOffice && building.isEnum(BuildingEnum::ConstructionOffice)) {
	//	ShowRadius(ConstructionOffice::Radius, centerAtom, building);
	//}
	else if (overlayType == OverlayType::Industrialist && building.isEnum(CardEnum::IndustrialistsGuild)) {
		ShowRadius(10, centerAtom, building);
	}
	else if (overlayType == OverlayType::Consulting && building.isEnum(CardEnum::ConsultingFirm)) {
		ShowRadius(10, centerAtom, building);
	}

	else if (overlayType == OverlayType::Library && building.isEnum(CardEnum::Library)) {
		ShowRadius(Library::Radius, centerAtom, building);
	}
	else if (overlayType == OverlayType::School && building.isEnum(CardEnum::School)) {
		ShowRadius(School::Radius, centerAtom, building);
	}
	else if (overlayType == OverlayType::Bank && building.isEnum(CardEnum::Bank)) {
		ShowRadius(Bank::Radius, centerAtom, building);
	}

	else if (overlayType == OverlayType::Granary && building.isEnum(CardEnum::Granary)) {
		ShowRadius(Granary::Radius, centerAtom, building);
	}
	else if (overlayType == OverlayType::HaulingServices && building.isEnum(CardEnum::HaulingServices)) {
		ShowRadius(HaulingServices::Radius, centerAtom, building);
	}

	else if (overlayType == OverlayType::Theatre && building.isEnum(CardEnum::Theatre)) {
		ShowRadius(Theatre::Radius, centerAtom, building);
	}
	else if (overlayType == OverlayType::Tavern && building.isEnum(CardEnum::Tavern)) {
		ShowRadius(Tavern::Radius, centerAtom, building);
	}
	
#undef SHOW_RADIUS

}

void UBuildingDisplayComponent::UpdateDisplayLight(Building& building)
{
	const GameDisplayInfo& displayInfo = gameManager()->displayInfo();
	
	const ModuleTransforms& modulePrototype = displayInfo.GetDisplayModules(building.buildingEnum(), building.displayVariationIndex());
	const std::vector<PointLightInfo>& lightInfos = modulePrototype.lightInfos;
	if (lightInfos.size() > 0)
	{
		float buildingRotation = RotationFromDirection(building.faceDirection());
		FVector displayLocation = gameManager()->DisplayLocation(building.centerTile().worldAtom2());
		FTransform transform(FRotator(0, buildingRotation, 0), displayLocation);

		for (const PointLightInfo& lightInfo : lightInfos)
		{
			FTransform localLightTransform(FRotator::ZeroRotator, lightInfo.position, lightInfo.scale);

			FTransform finalTransform;
			FTransform::Multiply(&finalTransform, &localLightTransform, &transform);

			UPointLightComponent* pointLight = PunUnrealUtils::ShowLight(finalTransform, _pointLights, _pointLightCount, this);

			pointLight->SetIntensityUnits(ELightUnits::Candelas);
			pointLight->SetIntensity(lightInfo.intensity);
			pointLight->SetLightColor(lightInfo.color);
			pointLight->SetAttenuationRadius(lightInfo.attenuationRadius);

			//pointLight->SetIntensity(100);
			//pointLight->SetAttenuationRadius(100);
		}
	}
}

void UBuildingDisplayComponent::HideDisplay(int meshId, int32 regionId)
{
	LLM_SCOPE_(EPunSimLLMTag::PUN_DisplayBuilding);
	
	//_lights[meshId]->SetVisibility(false);

	_moduleMeshes[meshId]->SetActive(false);
	_togglableModuleMeshes[meshId]->SetActive(false);

	_particles[meshId]->SetClusterActive(false);
}