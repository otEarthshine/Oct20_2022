// Fill out your copyright notice in the Description page of Project Settings.

#include "ObjectDescriptionUISystem.h"
#include "UnrealEngine.h"
#include "Engine/World.h"
#include "Components/StaticMeshComponent.h"
#include "Components/DecalComponent.h"
#include "Engine/StaticMesh.h"
#include "Components/CheckBox.h"
#include "Components/Button.h"
#include "../Simulation/Building.h"
#include "../Simulation/HumanStateAI.h"
#include "../Simulation/ArmyStateAI.h"
#include "../Simulation/UnitBase.h"
#include "../Simulation/Buildings/TownHall.h"
#include "../Simulation/Buildings/Temples.h"
#include "../Simulation/Buildings/House.h"
#include "../Simulation/Buildings/GathererHut.h"
#include "../Simulation/UnlockSystem.h"
#include "../Simulation/PlayerParameters.h"
#include "../Simulation/GeoresourceSystem.h"
#include "../AssetLoaderComponent.h"
#include "../Simulation/GameSimulationCore.h"
#include "../Simulation/BuildingSystem.h"
#include "../Simulation/StatSystem.h"
#include "Materials/Material.h"
#include "PunCity/PunUtils.h"
#include "PhysicsEngine/BodySetup.h"
#include "PhysicsEngine/AggregateGeom.h"

#include <string>
#include <sstream>
#include "PunCity/Simulation/Buildings/StorageYard.h"
#include "BuildingPlacementButton.h"
#include "ResourceStatTableRow.h"
#include "BuildingStatTableRow.h"
#include "CardSlot.h"
#include "PunCity/MapUtil.h"
#include "TownBonusIcon.h"

DECLARE_CYCLE_STAT(TEXT("PUN: [UI]All"), STAT_PunUIAll, STATGROUP_Game);

DECLARE_CYCLE_STAT(TEXT("PUN: [UI]UpdateDescriptionUI"), STAT_PunUIUpdateDescriptionUI, STATGROUP_Game);
DECLARE_CYCLE_STAT(TEXT("PUN: [UI]ProvinceFocusUI"), STAT_PunUIProvinceFocusUI, STATGROUP_Game);

DECLARE_CYCLE_STAT(TEXT("PUN: [UI]Select_Start"), STAT_PunUISelect_Start, STATGROUP_Game);
DECLARE_CYCLE_STAT(TEXT("PUN: [UI]FindStartSpot"), STAT_PunUIFindStartSpot, STATGROUP_Game);
DECLARE_CYCLE_STAT(TEXT("PUN: [UI]AddClaimLandButton"), STAT_PunUI_AddClaimLandButton, STATGROUP_Game);

DECLARE_CYCLE_STAT(TEXT("PUN: [UI]Province1"), STAT_PunUI_Province1, STATGROUP_Game);
DECLARE_CYCLE_STAT(TEXT("PUN: [UI]ProvinceAddBiomeInfo"), STAT_PunUI_ProvinceAddBiomeInfo, STATGROUP_Game);
DECLARE_CYCLE_STAT(TEXT("PUN: [UI]AddGeoresourceInfo"), STAT_PunUI_AddGeoresourceInfo, STATGROUP_Game);
DECLARE_CYCLE_STAT(TEXT("PUN: [UI]AddProvinceUpkeepInfo"), STAT_PunUI_AddProvinceUpkeepInfo, STATGROUP_Game);

DECLARE_CYCLE_STAT(TEXT("PUN: [UI]ShowRegionSelectionDecal"), STAT_PunUI_ShowRegionSelectionDecal, STATGROUP_Game);

#define LOCTEXT_NAMESPACE "ObjectDescriptionUISystem"

using namespace std;

static WorldTile2 FindGroundColliderHit(FHitResult& hitResult)
{
	UPrimitiveComponent* hitComponent = hitResult.GetComponent();
	PUN_CHECK(hitComponent);

	FVector relativeHitPoint = hitResult.ImpactPoint - hitComponent->GetComponentLocation();
	relativeHitPoint.X += CoordinateConstants::DisplayUnitPerRegion / 2;
	relativeHitPoint.Y += CoordinateConstants::DisplayUnitPerRegion / 2;
	LocalTile2 localTile(relativeHitPoint.X / CoordinateConstants::DisplayUnitPerTile,
		relativeHitPoint.Y / CoordinateConstants::DisplayUnitPerTile);

	int32 regionX = FCString::Atoi(*(hitComponent->ComponentTags[1].ToString()));
	int32 regionY = FCString::Atoi(*(hitComponent->ComponentTags[2].ToString()));

	WorldTile2 worldTile = localTile.worldTile2(WorldRegion2(regionX, regionY));
	PUN_LOG("GroundCollider region(%d, %d) localTile(%d, %d) world:%s... relative:%s ... componentLocation: %s ... impact:%s", regionX, regionY, localTile.x, localTile.y, *worldTile.To_FString(),
		*relativeHitPoint.ToCompactString(), *hitComponent->GetComponentLocation().ToString(), *hitResult.ImpactPoint.ToString());

	return worldTile;
}

void UObjectDescriptionUISystem::CreateWidgets()
{
	_objectDescriptionUI = AddWidgetToHUD<UObjectDescriptionUI>(UIEnum::ObjectDescription);
	_objectDescriptionUI->SetVisibility(ESlateVisibility::Collapsed);
	_objectDescriptionUI->Setup();
}

void UObjectDescriptionUISystem::Tick()
{
	LEAN_PROFILING_UI(TickObjDescriptionUI);
	SCOPE_CYCLE_COUNTER(STAT_PunUIAll);
	
	if (InterfacesInvalid()) return;

	_objectDescriptionUI->CheckPointerOnUI();

	UpdateDescriptionUI();

	if (dataSource()->simulation().tickCount() == 0) {
		_objectDescriptionUI->CloseAllSubUIs(true);
		_objectDescriptionUI->DescriptionPunBox->ResetBeforeFirstAdd();
		_objectDescriptionUI->DescriptionPunBoxScroll->ResetBeforeFirstAdd();
		_objectDescriptionUI->SetVisibility(ESlateVisibility::Collapsed);
	}

	// Didn't choose location yet, highlight regions the mouse hover over to hint it should be clicked...
	// if (!simulation().playerOwned(playerId()).isInitialized())
	{
		//FVector origin;
		//FVector direction;
		//UGameplayStatics::DeprojectScreenToWorld(CastChecked<APlayerController>(networkInterface()), networkInterface()->GetMousePositionPun(), origin, direction);

		//float traceDistanceToGround = fabs(origin.Z / direction.Z);
		//FVector groundPoint = direction * traceDistanceToGround + origin;

		//WorldTile2 hitTile = MapUtil::AtomLocation(dataSource()->cameraAtom(), groundPoint).worldTile2();

		WorldTile2 hitTile = networkInterface()->GetMouseGroundAtom().worldTile2();

		bool showHover = false;
		if (hitTile.isValid())
		{
			auto& provinceSys = simulation().provinceSystem();
			int32 provinceId = provinceSys.GetProvinceIdClean(hitTile);
			//provinceSys.highlightedProvince = provinceSys.GetProvinceIdRaw(hitTile); // TODO: remove this?


			if (provinceId != -1) 
			{
				bool isPlayerControlled = simulation().provinceOwnerPlayer(provinceId) == playerId();
				
				// Highlight Hover if the province isn't owned or if the camera is zoomed out
				if (dataSource()->zoomDistance() > WorldZoomTransition_RegionToRegion4x4_Mid ||
					dataSource()->alwaysShowProvinceHover())
				{
					if (!isPlayerControlled ||
						inputSystemInterface()->PlacementBuildingInfo().buildingEnum == CardEnum::Farm) 
					{
						showHover = true;
						ShowRegionSelectionDecal(hitTile, true);
					}
				}
			}
		}

		// Hide Hoover Decal
		if (!showHover) {
			if (_regionHoverMesh) {
				_regionHoverMesh->SetVisibility(false);
			}
		}
	}
}

bool UObjectDescriptionUISystem::IsShowingDescriptionUI()
{
	if (InterfacesInvalid()) return false;

	return dataSource()->simulation().descriptionUIState().isValid();
}
void UObjectDescriptionUISystem::CloseDescriptionUI() {
	if (InterfacesInvalid()) return;

	dataSource()->simulation().SetDescriptionUIState(DescriptionUIState());
	UpdateDescriptionUI();
}

void UObjectDescriptionUISystem::LeftMouseDown()
{
	PUN_LOG("!!! LeftMouseDown");
	
	if (InterfacesInvalid()) return;

	FHitResult hitResult;
	bool didHit = networkInterface()->ControllerGetMouseHit(hitResult, ECC_Pawn);

	// Don't hit if there is pointer on UI
	if (IsPointerOnUI()) {
		PUN_LOG("!!! LeftMouseDown: PointerOnUI");
		didHit = false;
	}

	if (dataSource()->isPhotoMode()) {
		dataSource()->simulation().SetDescriptionUIState(DescriptionUIState());
		UpdateDescriptionUI();

		PUN_LOG("!!! LeftMouseDown: isPhotoMode");
		return;
	}

	if (didHit)
	{
		/*
		 * !!! Hit issue could be because it is hitting some unwanted things (like skel mesh) !!!
		 */
		
		PUN_LOG("!!! LeftMouseDown: didHit");
		
		UPrimitiveComponent* hitComponent = hitResult.GetComponent();
		int32 hitIndex = hitResult.Item;

		//PUN_LOG("First Hit: %s", *hitResult.ToString());

		GameSimulationCore& simulation = dataSource()->simulation();
		
		// Hit ground, create Surrounding colliders and redo the test..
		if (hitComponent && hitComponent->ComponentTags.Num())
		{
			PUN_LOG("!!! LeftMouseDown: hitComponent");
			
			networkInterface()->ResetBottomMenuDisplay(); // Close the build menu/Gather etc if it was open...

			
			FName typeName = hitComponent->ComponentTags[0];

			DescriptionUIState uiState;
			float shortestHit = 100000.0f;

			/*
			 * Georesource
			 */
			// TODO: remove??
			//if (TEXT("Georesource") == typeName) {
			//	uiState.objectType = ObjectTypeEnum::Georesource;
			//	uiState.objectId = FCString::Atoi(*(hitComponent->ComponentTags[1].ToString()));
			//	simulation.SetDescriptionUIState(uiState);
			//	UpdateDescriptionUI();
			//	return;
			//}

			
			/*
			 * Tile + Objects with colliders made on the fly
			 */
			WorldTile2 worldTile = WorldTile2::Invalid;
			if (TEXT("WorldMap") == typeName) {
				FVector relativeHitPoint = hitResult.ImpactPoint - hitComponent->GetComponentLocation();
				worldTile = WorldTile2(relativeHitPoint.X / CoordinateConstants::DisplayUnitPerTile,
										relativeHitPoint.Y / CoordinateConstants::DisplayUnitPerTile);
			} else if (TEXT("GroundCollider") == typeName) {
				worldTile = FindGroundColliderHit(hitResult);
			}



			if (worldTile.isValid())
			{
				/*
				 * WorldMap
				 */
				if (!dataSource()->ZoomDistanceBelow(WorldZoomTransition_WorldSpaceUIHide))
				{
					if (TEXT("WorldMap") == typeName) 
					{

						Building* bld = simulation.buildingAtTile(worldTile);
						if (bld && dataSource()->ZoomDistanceBelow(WorldZoomTransition_Region4x4ToMap)) {
							uiState.objectType = ObjectTypeEnum::Building;
							uiState.objectId = bld->buildingId();;
						}
						else {
							uiState.objectType = ObjectTypeEnum::Map;
							uiState.objectId = worldTile.tileId();;
						}
						simulation.SetDescriptionUIState(uiState);
						UpdateDescriptionUI();
						return;
					}
				}
				
				/*
				 * TileObj
				 */
				TreeSystem& treeSystem = simulation.treeSystem();
				TileArea area(worldTile, 5);
				
				if (area.isValid()) {
					area.ExecuteOnArea_WorldTile2([&](WorldTile2 curTile)
					{
						int32_t curTileId = curTile.tileId();
						FVector displayLocation = dataSource()->DisplayLocation(curTile.worldAtom2());
						TileObjInfo info = treeSystem.tileInfo(curTileId);

						// If on construction site, select construction site instead
						auto selectTileObjOrConstruction = [&] () {
							int32 buildingId = simulation.buildingIdAtTile(worldTile);
							if (buildingId != -1 && !simulation.building(buildingId).isConstructed()) {
								uiState.objectType = ObjectTypeEnum::Building;
								uiState.objectId = buildingId;
							} else {
								uiState.objectType = ObjectTypeEnum::TileObject;
								uiState.objectId = curTileId;
							}
						};
						
						if (info.type == ResourceTileType::Tree)
						{
							if (!dataSource()->isHidingTree())
							{
								FTileMeshAssets assets = assetLoader()->tileMeshAsset(info.treeEnum);

								int32 ageTick = treeSystem.tileObjAge(curTileId);
								FTransform transform = GameDisplayUtils::GetTreeTransform(displayLocation, 0, curTileId, ageTick, info);

								if (TryMouseCollision(assets.assets[static_cast<int32>(TileSubmeshEnum::Leaf)], transform, shortestHit)) {
									selectTileObjOrConstruction();
								}
								else if (TryMouseCollision(assets.assets[static_cast<int32>(TileSubmeshEnum::Trunk)], transform, shortestHit)) {
									selectTileObjOrConstruction();
								}
							}
						}
						else if (info.type == ResourceTileType::Deposit)
						{
							FTileMeshAssets assets = assetLoader()->tileMeshAsset(info.treeEnum);
							
							int32 variationIndex;
							int32 variationCount = assets.assets.Num();
							FTransform transform = GameDisplayUtils::GetDepositTransform(curTileId, displayLocation, variationCount, variationIndex);

							if (TryMouseCollision(assets.assets[variationIndex], transform, shortestHit)) {
								selectTileObjOrConstruction();
								//uiState.objectType = ObjectTypeEnum::TileObject;
								//uiState.objectId = curTileId;
							}
						}
						else if (info.type == ResourceTileType::Bush)
						{
							if (!dataSource()->isHidingTree())
							{
								FTileMeshAssets assets = assetLoader()->tileMeshAsset(info.treeEnum);

								int32_t ageTick = treeSystem.tileObjAge(curTileId);
								FTransform transform = GameDisplayUtils::GetBushTransform(displayLocation, 0, curTileId, ageTick, info, simulation.terrainGenerator().GetBiome(curTile));

								// Plant uses the mesh array for multiple meshes (for example flower + its leaves)
								int32 submeshCount = assets.assets.Num();
								for (int32 i = 0; i < submeshCount; i++) {
									if (TryMouseCollision(assets.assets[i], transform, shortestHit))
									{
										// If bush is on farm or construction, show those instead...
										int32 buildingId = simulation.buildingIdAtTile(worldTile);
										if (buildingId != -1 &&
											(simulation.building(buildingId).isEnum(CardEnum::Farm) || !simulation.building(buildingId).isConstructed())
											)
										{
											uiState.objectType = ObjectTypeEnum::Building;
											uiState.objectId = buildingId;
										}
										else
										{
											uiState.objectType = ObjectTypeEnum::TileObject;
											uiState.objectId = curTileId;
										}
										break;
									}
								}
							}
						}
					});
				}

				/*
				 * Unit
				 */
				{
					const int unitCheckDistance = 3;
					
					UnitSystem& unitSystem = simulation.unitSystem();
					auto& unitList = unitSystem.unitSubregionLists();

					std::vector<WorldRegion2> regions = AlgorithmUtils::Get4NearbyRegions(worldTile);

					for (WorldRegion2& region : regions) 
					{
						unitList.ExecuteRegion(region, [&](int32 unitId)
						{
							if (!unitSystem.aliveUnsafe(unitId)) {
								return;
							}

							UnitStateAI& unit = unitSystem.unitStateAI(unitId);

							if (IsProjectile(unit.unitEnum()) ||
								WorldTile2::ManDistance(unit.unitAtom().worldTile2(), worldTile) > unitCheckDistance) {
								return;
							}

							FTransform transform;
							UnitDisplayState displayState = dataSource()->GetUnitTransformAndVariation(unit, transform);
							transform.SetScale3D(FVector::OneVector);

							UnitEnum unitEnum = unit.unitEnum();
							if (unitEnum == UnitEnum::Human)
							{
								if (IsHorseAnimation(unit.animationEnum()))
								{
									if (TryMouseCollision(assetLoader()->unitAsset(UnitEnum::HorseMarket, 0).staticMesh, transform, shortestHit)) {
										// Human uses USkeletonAsset, should just mark in sim for UnitDisplayComponent to adjust customDepth
										uiState.objectType = ObjectTypeEnum::Unit;
										uiState.objectId = unitId;
									}
								}
								else
								{
									// VariationIndex 0 for human.. Rough size is enough
									if (TryMouseCollision(assetLoader()->unitAsset(unitEnum, 0).staticMesh, transform, shortestHit)) {
										// Human uses USkeletonAsset, should just mark in sim for UnitDisplayComponent to adjust customDepth
										uiState.objectType = ObjectTypeEnum::Unit;
										uiState.objectId = unitId;
									}
								}
							}
							else if (unitEnum == UnitEnum::WildMan) 
							{
								// VariationIndex 0 for human.. Rough size is enough
								FUnitAsset unitAsset = assetLoader()->unitAsset(unitEnum, 0);
								if (TryMouseCollision(unitAsset.staticMesh, transform, shortestHit)) {
									// Human uses USkeletonAsset, should just mark in sim for UnitDisplayComponent to adjust customDepth
									uiState.objectType = ObjectTypeEnum::Unit;
									uiState.objectId = unitId;
								}
							}
							else {
								if (TryMouseCollision(assetLoader()->unitAsset(displayState.unitEnum, displayState.variationIndex).staticMesh, transform, shortestHit)) {
									uiState.objectType = ObjectTypeEnum::Unit;
									uiState.objectId = unitId;
								}
							}
						});
					}
				}

				/*
				 * Building
				 */
				{
					const GameDisplayInfo& displayInfo = dataSource()->displayInfo();
					BuildingSystem& buildingSystem = simulation.buildingSystem();
					auto& buildingList = buildingSystem.buildingSubregionList();

					std::vector<WorldRegion2> regions = AlgorithmUtils::Get4NearbyRegions(worldTile);
					
					for (WorldRegion2& region : regions)
					{
						buildingList.ExecuteRegion(region, [&](int32_t buildingId)
						{
							if (!buildingSystem.alive(buildingId)) {
								return;
							}

							Building& building = buildingSystem.building(buildingId);

							if (building.isConstructed())
							{
								const ModuleTransformGroup& modulePrototype = displayInfo.GetDisplayModules(building.factionEnum(), building.buildingEnum(), building.displayVariationIndex());
								std::vector<ModuleTransform> modules = modulePrototype.transforms;

								FTransform transform(FRotator(0, RotationFromDirection(building.displayFaceDirection()), 0),
															dataSource()->DisplayLocation(building.displayCenterTile().worldAtom2()));

								for (int i = 0; i < modules.size(); i++) 
								{
									UStaticMesh* protoMesh = assetLoader()->moduleMesh(modules[i].moduleName);

									if (protoMesh->BodySetup->AggGeom.GetElementCount() > 0) {
										FTransform finalTransform;
										FTransform::Multiply(&finalTransform, &modules[i].transform, &transform);

										if (TryMouseCollision(protoMesh, finalTransform, shortestHit)) {
											uiState.objectType = ObjectTypeEnum::Building;
											uiState.objectId = buildingId;
										}
									}
								}
							}
						});
					}

				}

				/*
				 * Tile
				 */
				if (!uiState.isValid())
				{
					int32 buildingId = simulation.buildingIdAtTile(worldTile);
					if (buildingId != -1) {
						PUN_LOG("GroundCollider got: %d", buildingId);

						uiState.objectType = ObjectTypeEnum::Building;
						uiState.objectId = buildingId;
					}
					else {
						// Drop
						std::vector<DropInfo> drops = simulation.dropSystem().GetDrops(worldTile);
						if (drops.size() > 0) {
							uiState.objectType = ObjectTypeEnum::Drop;
							uiState.objectId = worldTile.tileId();
							PUN_LOG("Drop clicked (%d, %d)", worldTile.x, worldTile.y);
						}
						// TileOfbject
						else if (!simulation.treeSystem().IsEmptyTile_WaterAsEmpty(worldTile.tileId()) &&
								 !dataSource()->isHidingTree())
						{
							uiState.objectType = ObjectTypeEnum::TileObject;
							uiState.objectId = worldTile.tileId();
							PUN_LOG("TileObjects clicked from ground (%d, %d)", worldTile.x, worldTile.y);
						}
						else {
							uiState.objectType = ObjectTypeEnum::EmptyTile;
							uiState.objectId = worldTile.tileId();
						}
					}
				}
			}


			PUN_LOG("!!! LeftMouseDown: SetDescriptionUIState type:%d id:%d", static_cast<int>(uiState.objectType), uiState.objectId);

			simulation.SetDescriptionUIState(uiState);
		}

		UpdateDescriptionUI();
		return;
	}

	PUN_LOG("!!! LeftMouseDown: did Not Hit");

	UpdateDescriptionUI();
}

void UObjectDescriptionUISystem::UpdateDescriptionUI()
{
	if (InterfacesInvalid()) return;

	SCOPE_CYCLE_COUNTER(STAT_PunUIUpdateDescriptionUI);

	UPunBoxWidget* focusBox = _objectDescriptionUI->DescriptionPunBox;
	UPunBoxWidget* descriptionBoxScrollable = _objectDescriptionUI->DescriptionPunBoxScroll;
	
	
	// DescriptionPunBoxScroll is used for House
	_objectDescriptionUI->DescriptionPunBoxScrollOuter->SetVisibility(ESlateVisibility::Collapsed);


	// Selection Meshes
	meshIndex = 0;

	if (!dataSource()->HasSimulation()) {
		return;
	}

	GameSimulationCore& sim = dataSource()->simulation();
	DescriptionUIState uiState = sim.descriptionUIState();

	auto assetLoader = dataSource()->assetLoader();
	
	// Reset the UI if it was swapped
	if (_objectDescriptionUI->state != uiState) {
		_objectDescriptionUI->state = uiState;

		if (uiState.isValid()) {
			_justOpenedDescriptionUI = true;
		}
		_objectDescriptionUI->CloseAllSubUIs(uiState.shouldCloseStatUI);
		focusBox->ResetBeforeFirstAdd();
		descriptionBoxScrollable->ResetBeforeFirstAdd();
	}

	if (_tileSelectionDecal) {
		_tileSelectionDecal->SetVisibility(false);
		_tileSelectionMesh->SetVisibility(false);
	}
	if (_regionSelectionMesh) {
		//_regionSelectionDecal->SetVisibility(false);
		_regionSelectionMesh->SetVisibility(false);
	}

	PunUnrealUtils::UpdateDecals(_groundBoxHighlightDecals, _groundBoxHighlightCount);

	

	OverlayType overlayType = OverlayType::None;

	auto& globalResourceSys = sim.globalResourceSystem(playerId());
	

	if (uiState.objectType == ObjectTypeEnum::None) {
		_objectDescriptionUI->SetVisibility(ESlateVisibility::Collapsed);
	}
	else
	{	
		_objectDescriptionUI->state = uiState; // For Click/Checkbox
		_objectDescriptionUI->parent = this;

		int32 objectId = uiState.objectId;

		//_objectDescriptionUI->BuildingsStatOpener->SetVisibility(ESlateVisibility::Collapsed);

		_objectDescriptionUI->TownBonusBox->SetVisibility(ESlateVisibility::Collapsed);
		_objectDescriptionUI->GlobalBonusBox->SetVisibility(ESlateVisibility::Collapsed);

		ResetIndent();

		//! TileBuilding (such as Building)
		// TODO: may be TileBuilding should just be TileObject..???
		if (uiState.objectType == ObjectTypeEnum::Building && objectId < 0) {
			// negative objectId in building means Fence or other TileObject
			WorldTile2 tile(-objectId);
			check(tile.isValid());

			//SetText(_objectDescriptionUI->DescriptionUITitle, "Fence");
			focusBox->AfterAdd();

			// Selection Mesh
			SpawnSelectionMesh(assetLoader->SelectionMaterialGreen, dataSource()->DisplayLocation(tile.worldAtom2()) + FVector(0, 0, 40));
		}
		//! Building
		else if (uiState.objectType == ObjectTypeEnum::Building)
		{
			PUN_CHECK(dataSource()->simulation().buildingIsAlive(objectId));

			Building& building = dataSource()->GetBuilding(objectId);
			CardEnum buildingEnum = building.buildingEnum();
			BldInfo buildingInfo = building.buildingInfo();

			auto& resourceSys = sim.resourceSystem(building.townId());

			bool showWhenOwnedByCurrentPlayer = building.playerId() == playerId() || PunSettings::IsOn("DebugFocusUI");

			TArray<FText> args;

			/*
			 * Header
			 */
			auto showBuildingSwapArrow = [&]() {
				if (playerId() == building.playerId()) {
					const std::vector<int32>& buildingIds = sim.buildingIds(building.townId(), buildingEnum);
					return buildingIds.size() > 1;
				}
				return false;
			};
			
			if (buildingEnum == CardEnum::RegionTribalVillage) {
				_objectDescriptionUI->DescriptionPunBox->AddWGT_ObjectFocus_Title(
					FText::Format(LOCTEXT("TribeName", "{0} tribe"), GenerateTribeName(objectId))
				);
			}
			/*
			 * Minor City
			 */
			else if (buildingEnum == CardEnum::MinorCity ||
					buildingEnum == CardEnum::MinorCityPort)
			{
				int32 townId = building.townId();
				TownManagerBase* townManagerBase = sim.townManagerBase(townId);
				MinorCity& minorCityBld = sim.building<MinorCity>(sim.GetTownhallId(townId));


				if (townManagerBase->relationship().isAlly(playerId())) {
					focusBox->AddRichTextCenter(FText::Format(INVTEXT("<FaintGreen12>{0}</>"), LOCTEXT("Ally", "Ally")));
				}
				else if (townManagerBase->relationship().isEnemy(playerId())) {
					focusBox->AddRichTextCenter(FText::Format(INVTEXT("<FaintRed12>{0}</>"), LOCTEXT("Enemy", "Enemy")));
				}
				else {
					focusBox->AddRichTextCenter(LOCTEXT("Neutral", "Neutral"));
				}
				
				focusBox->AddSpacer();
				

				int32 level;
				int32 currentMoney;
				int32 moneyToNextLevel;
				townManagerBase->GetMinorCityLevelAndCurrentMoney(level, currentMoney, moneyToNextLevel);
				
				if (level == 1) {
					_objectDescriptionUI->DescriptionPunBox->AddWGT_ObjectFocus_Title(
						FText::Format(LOCTEXT("TribeName", "{0} tribe"), sim.townNameT(townId))
					);
				}
				else {
					_objectDescriptionUI->DescriptionPunBox->AddWGT_ObjectFocus_Title(
						FText::Format(LOCTEXT("TribeName", "{0} (minor town)"), sim.townNameT(townId))
					);
				}

				//focusBox->AddRichText(FText::Format(INVTEXT("Level {0}"), minorCityBld.minorCityLevel()));

#if !UE_BUILD_SHIPPING
				//if (PunSettings::IsOn("DebugFocusUI")) {
					focusBox->AddRichText(FText::Format(INVTEXT("TownId {0}"), townId));
				//}
#endif

				focusBox->AddSpacer();

				focusBox->AddRichText(FText::Format(INVTEXT("TownhallId {0}"), townManagerBase->townhallId));

				focusBox->AddSpacer();
				
				const std::vector<AutoTradeElement>& autoExportElements = townManagerBase->autoExportElementsConst();
				for (const AutoTradeElement& element : autoExportElements) {
					focusBox->AddRichText(FText::Format(INVTEXT("Export {0}({1}) {2}"), 
						TEXT_NUM(element.calculatedFulfillmentLeft()), 
						TEXT_NUM(element.calculatedTradeAmountNextRound), 
						ResourceNameT(element.resourceEnum))
					);
				}

				const std::vector<AutoTradeElement>& autoImportElements = townManagerBase->autoImportElementsConst();
				for (const AutoTradeElement& element : autoImportElements) {
					focusBox->AddRichText(FText::Format(INVTEXT("Import {0}({1}) {2}"), 
						TEXT_NUM(element.calculatedFulfillmentLeft()),
						TEXT_NUM(element.calculatedTradeAmountNextRound),
						ResourceNameT(element.resourceEnum))
					);
				}

				focusBox->AddSpacer();

				focusBox->AddWGT_TextRow(UIEnum::WGT_ObjectFocus_TextRow,
					LOCTEXT("Level", "Level"),
					TEXT_NUM(level)
				);
				focusBox->AddWGT_TextRow(UIEnum::WGT_ObjectFocus_TextRow,
					LOCTEXT("Wealth", "Wealth"),
					FText::Format(
						INVTEXT("{0}/{1}"),
						TEXT_NUM(currentMoney),
						TEXT_NUM(moneyToNextLevel)
					),
					assetLoader->CoinIcon
				);

				focusBox->AddSpacer();

				const RelationshipModifiers& relationship = townManagerBase->relationship();
				int32 totalRelationshipToPlayer = townManagerBase->relationship().GetTotalRelationship(playerId());

				if (townManagerBase->lordPlayerId() != -1) {
					focusBox->AddWGT_TextRow(UIEnum::WGT_ObjectFocus_TextRow,
						LOCTEXT("Lord", "Lord"),
						sim.playerNameT(townManagerBase->lordPlayerId())
					);
				}
				else {
					focusBox->AddWGT_TextRow(UIEnum::WGT_ObjectFocus_TextRow,
						LOCTEXT("Relationship", "Relationship"),
						FText::Format(
							INVTEXT("{0}/{1}"),
							TEXT_NUM(totalRelationshipToPlayer),
							TEXT_NUM(townManagerBase->relationship().AllyRelationshipRequirement(playerId()))
						)
					);
				}

				focusBox->AddSpacer();
				focusBox->AddSpacer();

				//! Relationship Level
				focusBox->AddRichText(FText::Format(
					LOCTEXT("Relationship Tier 1", "{0}Relationship Lv 1 (exceed 30):</>\n{0}+30</><img id=\"Influence\"/> {0}per round</>"),
					totalRelationshipToPlayer > 30 ? INVTEXT("<Default>") : INVTEXT("<Gray>")
				));
				focusBox->AddSpacer();

				focusBox->AddRichText(FText::Format(
					LOCTEXT("Relationship Tier 2", "{0}Relationship Lv 2 (exceed 60):</>\n{0}+60</><img id=\"Influence\"/> {0}per round</>"),
					totalRelationshipToPlayer >60 ? INVTEXT("<Default>") : INVTEXT("<Gray>")
				));
				focusBox->AddSpacer();

				focusBox->AddRichText(FText::Format(
					LOCTEXT("Relationship Tier 3", "{0}Relationship Lv 3 (ally):</>\n{0}+150</><img id=\"Influence\"/> {0}per round</>"),
					relationship.isAlly(playerId()) ? INVTEXT("<Default>") : INVTEXT("<Gray>")
				));

				focusBox->AddSpacer();
				focusBox->AddSpacer();

				//! Highest 3 relationship
				{
					std::vector<std::pair<int32, int32>> playerIdAndRelationship;
					for (int32 i = 0; i < GameConstants::MaxPlayersAndAI; i++) {
						if (i != playerId()) {
							int32 totalRelationship = relationship.GetTotalRelationship(i);
							if (totalRelationship > 0) {
								playerIdAndRelationship.push_back({ i, totalRelationship });
							}
						}
					}

					std::sort(playerIdAndRelationship.begin(), playerIdAndRelationship.end(), [&](std::pair<int32, int32> a, std::pair<int32, int32> b) {
						return a.second > b.second;
					});

					int32 loopCount = std::min(3, static_cast<int>(playerIdAndRelationship.size()));
					if (loopCount > 0) {
						focusBox->AddRichText(LOCTEXT("Relationship with Others:", "Relationship with Others:"));
					}
					for (int32 i = 0; i < loopCount; i++) {
						focusBox->AddRichText(FText::Format(
							INVTEXT("{0} {1}"),
							sim.playerNameT(playerIdAndRelationship[i].first),
							playerIdAndRelationship[i].second
						));
					}
				}
			}
			
			else if (buildingEnum == CardEnum::Townhall) {
				_objectDescriptionUI->DescriptionPunBox->AddWGT_ObjectFocus_Title(
					FText::Format(LOCTEXT("Townhall Title", "Townhall Lv {0}"), TEXT_NUM(building.subclass<TownHall>().townhallLvl)),
					FText(), nullptr,
					(showBuildingSwapArrow() ? this : nullptr)
				);
			}
			else if (buildingEnum == CardEnum::House) {
				_objectDescriptionUI->DescriptionPunBox->AddWGT_ObjectFocus_Title(
					FText::Format(LOCTEXT("HouseName", "House Lv {0}"), FText::AsNumber(building.subclass<House>().houseLvl()))
				);
			}
			else
			{
				_objectDescriptionUI->DescriptionPunBox->AddWGT_ObjectFocus_Title(
					building.buildingInfo().GetName(),
					FText(), nullptr,
					(showBuildingSwapArrow() ? this : nullptr)
				);
			}


			//_objectDescriptionUI->DescriptionUITitle->SetVisibility(ESlateVisibility::Collapsed);
			//SetText(_objectDescriptionUI->DescriptionUITitle, FText::Join(FText(), args));
			//args.Empty();

			
			
			//_objectDescriptionUI->BuildingsStatOpener->SetVisibility(building.maxOccupants() > 0 && !IsHouse(building.buildingEnum()) ? ESlateVisibility::Visible : ESlateVisibility::Collapsed);
			//_objectDescriptionUI->BuildingsStatOpener->SetVisibility(ESlateVisibility::Collapsed); // TODO: Don't use it anymore?
			//_objectDescriptionUI->NameEditButton->SetVisibility((building.isEnum(CardEnum::Townhall) && building.playerId() == playerId()) ? ESlateVisibility::Visible : ESlateVisibility::Collapsed);

			//// Swap between same type of buildings
			//if (building.playerId() == playerId()) {
			//	const std::vector<int32>& buildingIds = sim.buildingIds(building.townId(), buildingEnum);
			//	_objectDescriptionUI->BuildingSwapArrows->SetVisibility(buildingIds.size() > 1 ? ESlateVisibility::Visible : ESlateVisibility::Collapsed);
			//} else {
			//	_objectDescriptionUI->BuildingSwapArrows->SetVisibility(ESlateVisibility::Collapsed);
			//}


#if !UE_BUILD_SHIPPING
			if (PunSettings::IsOn("DebugFocusUI")) 
			{
				focusBox->AddRichText(FText::Format(INVTEXT("[{0}]"), FText::AsNumber(objectId)));
				focusBox->AddRichText(FText::Format(INVTEXT("\n{0}"), FText::FromString(building.centerTile().To_FString())));

				if (building.playerId() != -1) {
					ADDTEXT_NUM(args, sim.IsConnectedBuilding(building.buildingId()));
					focusBox->AddRichText(FTEXT("<Yellow>IsConnectedBld</>"), args);
				}
			}
#endif


			// Show warning
			building.TryRefreshHoverWarning(UGameplayStatics::GetTimeSeconds(this));
			if (building.hoverWarning != HoverWarning::None)
			{
				FText description = GetHoverWarningDescription(building.hoverWarning);
				if (!description.IsEmpty()) {
					FString descriptionStr = description.ToString().Replace(TEXT("\n"), TEXT("</>\n<Red>"));
					descriptionStr = "<Red>" + descriptionStr + "</>";

					focusBox->AddWGT_PunRichText(UIEnum::WGT_ObjectFocus_FlavorText,
						FText::FromString(descriptionStr)
					);
					//focusBox->AddRichTextF("<Red>" + descriptionStr + "</>");
					focusBox->AddSpacer();
				}
			}
			

			// Show Description for some building
			if (building.isEnum(CardEnum::Bank) ||
				building.isEnum(CardEnum::Archives) ||
				building.isEnum(CardEnum::HaulingServices) ||
				building.isEnum(CardEnum::Granary) ||
				building.isEnum(CardEnum::ShippingDepot) || 
				building.isEnum(CardEnum::Market) || 
				building.isEnum(CardEnum::TradingCompany) ||
				building.isEnum(CardEnum::Hotel) ||
				IsDecorativeBuilding(building.buildingEnum()))
			{
				focusBox->AddWGT_PunRichText(UIEnum::WGT_ObjectFocus_FlavorText, building.buildingInfo().GetDescription()); // Autowrap off prevent flash
			}

			

			// Helpers:
			TileArea area = building.area();
			FVector displayLocation = dataSource()->DisplayLocation(building.centerTile().worldAtom2());
			
			AlgorithmUtils::ShiftDisplayLocationToTrueCenter(displayLocation, area, building.faceDirection());

			if (building.isEnum(CardEnum::Farm)) {
				displayLocation = dataSource()->DisplayLocation(building.subclass<Farm>().GetFarmDisplayCenter().worldAtom2());
			}

			/*
			 * Body
			 */
			if (building.isOnFire())
			{
				focusBox->AddRichText(LOCTEXT("BuildingOnFire", "<Red>Building is on fire.</>"));
			}
			else if (building.isBurnedRuin())
			{
				//ss << "<Red>Burned ruin.</>";
				focusBox->AddRichText(LOCTEXT("BurnedRuin", "<Red>Burned ruin.</>"));
			}
			else
			{

				if (!building.isConstructed()) {
					focusBox->AddRichText(LOCTEXT("UnderConstruction", "Under construction"));
					focusBox->AddSpacer();
				}

				// Occupants
				if (IsHouse(building.buildingEnum()) && building.isConstructed()) 
				{
					focusBox->AddWGT_TextRow(UIEnum::WGT_ObjectFocus_TextRow,
						LOCTEXT("Occupants", "Occupants"),
						FText::Format(
							INVTEXT("{0}/{1}"),
							FText::AsNumber(building.occupantCount()),
							FText::AsNumber(building.allowedOccupants())
						),
						assetLoader->HouseIcon
					);

#if !UE_BUILD_SHIPPING
					if (PunSettings::IsOn("DebugFocusUI")) {
						focusBox->AddRichText(FText::Join(FText(), INVTEXT("<Yellow>"), LOCTEXT("HouseAdjacent", "Adjacent"), INVTEXT("</>")));
					}
#endif					
				}
				else if (building.maxOccupants() > 0) 
				{
					focusBox->AddWGT_TextRow(UIEnum::WGT_ObjectFocus_TextRow, 
						LOCTEXT("Workers_C", "Workers: "), FText::Format(
						INVTEXT("{0}/{1}"),
						FText::AsNumber(building.occupantCount()),
						FText::AsNumber(building.maxOccupants())
					));
				}

				// Efficiency + Job Happiness
				//  ... exclude farm
				if (IsProducer(building.buildingEnum()) ||
					building.buildingEnum() == CardEnum::FruitGatherer ||
					building.buildingEnum() == CardEnum::Forester ||
					building.buildingEnum() == CardEnum::HuntingLodge ||
					building.buildingEnum() == CardEnum::Caravansary)
				{
					AddEfficiencyText(building, focusBox);
				}

				// Upkeep/Budget
				int32 baseUpkeep = building.baseUpkeep();
				if (building.isConstructed() && baseUpkeep > 0)
				{
					int32 upkeep = building.upkeep();

					if (showWhenOwnedByCurrentPlayer &&
						IsBudgetAdjustable(building.buildingEnum()) &&
						sim.IsResearched(playerId(), TechEnum::BudgetAdjustment))
					{
						if (_justOpenedDescriptionUI) {
							building.lastBudgetLevel = building.budgetLevel();
						}

						focusBox->AddSpacer();
						
						Indent(50);
						focusBox->AddRichText(
							TEXT_TAG("<Bold>", LOCTEXT("Budget_C", "Budget: ")),
							FText::Format(INVTEXT("<Bold>{0}</><img id=\"Coin\"/>"), FText::AsNumber(upkeep))
						);
						ResetIndent();
						
						auto widget = focusBox->AddBudgetAdjuster(this, building.buildingId(), true, building.lastBudgetLevel);
						AddToolTip(widget, LOCTEXT("Budget_Tip", "Increasing the Budget Level lead to higher Effectiveness and Job Happiness, but also increases the Building Upkeep."));
						focusBox->AddSpacer();
					}
					else {
						focusBox->AddWGT_TextRow(UIEnum::WGT_ObjectFocus_TextRow,
							LOCTEXT("Upkeep_C", "Upkeep"),
							FText::AsNumber(upkeep),
							assetLoader->CoinIcon
						);
					}
				}
				

				// Work Hour
				if (showWhenOwnedByCurrentPlayer &&
					IsBudgetAdjustable(building.buildingEnum()) &&
					sim.IsResearched(playerId(), TechEnum::WorkSchedule))
				{
					if (_justOpenedDescriptionUI) {
						building.lastWorkTimeLevel = building.workTimeLevel();
					}

					focusBox->AddSpacer();
					Indent(50);
					focusBox->AddRichText(
						TEXT_TAG("<Bold>", LOCTEXT("WorkHour_C", "Work Hours: "))
					);
					ResetIndent();
					
					auto widget = focusBox->AddBudgetAdjuster(this, building.buildingId(), false, building.lastWorkTimeLevel);
					AddToolTip(widget, LOCTEXT("Budget_Tip", "Increasing the Work Hours Level lead to higher Effectiveness, but lower Job Happiness."));
					focusBox->AddSpacer();
				}

				// Electricity
				if (building.IsElectricityUpgraded())
				{
					focusBox->AddSpacer();
					focusBox->AddRichText(
						TEXT_TAG("<GrayBold>", LOCTEXT("Electricity", "Electric Usage/Need")),
						FText::Format(INVTEXT("{0}/{1}kW"), TextRed(TEXT_NUM(building.ElectricityAmountUsage()), building.NotEnoughElectricity()), TEXT_NUM(building.ElectricityAmountNeeded()))
					);
				}
				

				// Upgrade Level / Appeal / tax
				if (building.isConstructed())
				{

					// Main
					if (IsHumanHouse(building.buildingEnum())) 
					{
						House* house = static_cast<House*>(&building);
						
						focusBox->AddSpacer(10);

						int32 houseLvl = house->houseLvl();

						// Heating
						{
							args.Empty();

							
							auto widgetCoal = focusBox->AddWGT_TextRow(UIEnum::WGT_ObjectFocus_TextRow,
								LOCTEXT("CoalHeatingEfficiency", "Coal Heating Efficiency"),
								TEXT_PERCENT(house->GetHeatingEfficiency(ResourceEnum::Coal))
							);
							house->GetHeatingEfficiencyTip(args, ResourceEnum::Coal);
							AddToolTip(widgetCoal, args);
							

							auto widgetWood = focusBox->AddWGT_TextRow(UIEnum::WGT_ObjectFocus_TextRow,
								LOCTEXT("WoodHeatingEfficiency", "Wood Heating Efficiency"), 
								TEXT_PERCENT(house->GetHeatingEfficiency(ResourceEnum::Wood))
							);
							house->GetHeatingEfficiencyTip(args, ResourceEnum::Wood);
							AddToolTip(widgetWood, args);
						}
						focusBox->AddSpacer(10);
						
						focusBox->AddWGT_TextRow(UIEnum::WGT_ObjectFocus_TextRow,
							LOCTEXT("Appeal", "Appeal"), 
							TEXT_PERCENT(house->GetAppealPercent())
						);

						// Happiness
						{
							auto widget = focusBox->AddWGT_TextRow(UIEnum::WGT_ObjectFocus_TextRow,
								LOCTEXT("Housingquality", "Housing quality"), TEXT_PERCENT(house->housingQuality())
							);
							focusBox->AddSpacer();
						}

						focusBox->AddSpacer(5);

						// Tax
						{
							auto widget = focusBox->AddWGT_TextRow(UIEnum::WGT_ObjectFocus_TextRow,
								LOCTEXT("Income", "Income"),
								TEXT_100(house->totalHouseIncome100()),
								assetLoader->CoinIcon
							);
							focusBox->AddSpacer();

							// Tooltip
							// Single house tax...
							args.Empty();
							ADDTEXT_(LOCTEXT("Income_C", "Income: {0}<img id=\"Coin\"/>\n"), TEXT_100(house->totalHouseIncome100()));

							for (int32 i = 0; i < HouseIncomeEnumCount; i++) {
								int32 income100 = house->GetIncome100Int(i);
								if (income100 != 0) {
									//ss << (income100 > 0 ? INVTEXT(" +") : INVTEXT(" ")) << (income100 / 100.0f) << " " << IncomeEnumName[i] << "\n";
									ADDTEXT_JOIN_(FText(), INVTEXT(" "), TEXT_100SIGNED(income100), INVTEXT(" "), GetIncomeEnumName(i), INVTEXT("\n"));
								}
							}

							AddToolTip(widget, args);
						}

						// Science
						{
							auto widget = focusBox->AddWGT_TextRow(UIEnum::WGT_ObjectFocus_TextRow,
								LOCTEXT("Science", "Science"),
								TEXT_100(house->science100PerRound()),
								assetLoader->ScienceIcon
							);
							focusBox->AddSpacer();

							// Tooltip
							ADDTEXT_(LOCTEXT("ScienceOutput_C", "Science output: {0}<img id=\"Science\"/>\n"), TEXT_100(house->science100PerRound()));

							int32 cumulative100 = 0;
							for (int32 i = 0; i < HouseScienceEnums.size(); i++) 
							{
								int32 science100 = house->GetScience100(HouseScienceEnums[i], 0);
								if (science100 != 0) {
									ADDTEXT_JOIN_(INVTEXT(" "), TEXT_100SIGNED(science100), INVTEXT(" "), ScienceEnumName(static_cast<int>(HouseScienceEnums[i])), INVTEXT("\n"));
								}
								cumulative100 += science100;
							}
							for (int32 i = 0; i < HouseScienceModifierEnums.size(); i++)
							{
								int32 science100 = house->GetScience100(HouseScienceModifierEnums[i], cumulative100);
								if (science100 != 0) {
									ADDTEXT_JOIN_(INVTEXT(" "), TEXT_100SIGNED(science100), INVTEXT(" "), ScienceEnumName(static_cast<int>(HouseScienceModifierEnums[i])), INVTEXT("\n"));
								}
							}

							AddToolTip(widget, args);
						}

						if (showWhenOwnedByCurrentPlayer)
						{
							focusBox->AddLineSpacer();

							// TODO: add appeal later?
							//int32 appealNeeded = house->GetAppealUpgradeRequirement(houseLvl + 1);

							if (houseLvl == house->GetMaxHouseLvl()) {
								focusBox->AddRichText(LOCTEXT("Maximum lvl", "Maximum lvl"));
							}
							else {
								ADDTEXT_(INVTEXT("<Orange>{0}</>\n"), house->HouseNeedDescription());
								ADDTEXT_LOCTEXT("HouseToIncreaseLevel", "<Orange>(to increase level)</>");
								auto widget = focusBox->AddRichText(args);
								AddToolTip(widget, house->HouseNeedTooltipDescription());
							}

							focusBox->AddSpacer();

							/*
							 * House Resource Checkboxes
							 */
							_objectDescriptionUI->DescriptionPunBoxScrollOuter->SetVisibility(ESlateVisibility::Visible);
							
							auto addCheckBoxIconPair = [&](ResourceEnum resourceEnum) {
								int32 resourceCount = building.resourceCount(resourceEnum);
								UIconTextPairWidget* widget = descriptionBoxScrollable->AddIconPair(FText(), resourceEnum, TEXT_NUM(resourceCount));
								widget->ObjectId = building.buildingId();
								widget->UpdateAllowCheckBox(resourceEnum);
								widget->SetTextColor(resourceCount > 0 ? FLinearColor(1, 1, 1) : FLinearColor(.15, .15, .15));
							};

							descriptionBoxScrollable->indentation = 8; // Offset Subheader negative indentation
							
							descriptionBoxScrollable->AddWGT_PunText(UIEnum::WGT_ObjectFocus_Subheader,
								LOCTEXT("Fuel_C", "Fuel:")
							);
							addCheckBoxIconPair(ResourceEnum::Wood);
							addCheckBoxIconPair(ResourceEnum::Coal);

							descriptionBoxScrollable->AddWGT_PunText(UIEnum::WGT_ObjectFocus_Subheader,
								LOCTEXT("LuxuryTier1_C", "Luxury tier 1:")
							);
							
							const std::vector<ResourceEnum>& luxury1 = GetLuxuryResourcesByTier(1);
							for (size_t i = 0; i < luxury1.size(); i++) {
								addCheckBoxIconPair(luxury1[i]);
							}


							auto hasAnyResource = [&](const std::vector<ResourceEnum>& resourceEnums) {
								for (ResourceEnum resourceEnum : resourceEnums) {
									if (house->resourceCountSafe(resourceEnum) > 0) {
										return true;
									}
								}
								return false;
							};

							const std::vector<ResourceEnum>& luxury2 = GetLuxuryResourcesByTier(2);
							const std::vector<ResourceEnum>& luxury3 = GetLuxuryResourcesByTier(3);
							
							if (houseLvl >= 4 || hasAnyResource(luxury2))
							{
								descriptionBoxScrollable->AddWGT_PunText(UIEnum::WGT_ObjectFocus_Subheader, 
									LOCTEXT("LuxuryTier2_C", "Luxury tier 2:")
								);
								for (size_t i = 0; i < luxury2.size(); i++) {
									addCheckBoxIconPair(luxury2[i]);
								}
							}
							if (houseLvl >= 6 || hasAnyResource(luxury3))
							{
								descriptionBoxScrollable->AddWGT_PunText(UIEnum::WGT_ObjectFocus_Subheader, 
									LOCTEXT("LuxuryTier3_C", "Luxury tier 3:")
								);
								for (size_t i = 0; i < luxury3.size(); i++) {
									addCheckBoxIconPair(luxury3[i]);
								}
							}

							descriptionBoxScrollable->indentation = 0;
							
							descriptionBoxScrollable->AfterAdd();
						}

						// Foreigners
						if (building.playerId() != playerId())
						{
							/*
							 * Spy Nest
							 */
							 // TODO: show more info than Townhall ???
							if (sim.IsResearched(playerId(), TechEnum::SpyCenter))
							{
								if (house->spyPlayerId() != playerId())
								{
									int32 spyNestPrice = sim.GetSpyNestPrice(playerId(), house->townId());

									UPunButton* button = focusBox->AddButton2Lines(
										FText::Format(
											LOCTEXT("EstablishSpyNest_Button", "Establish Spy Nest\n<img id=\"Coin\"/>{0}"),
											TextRed(TEXT_NUM(spyNestPrice), spyNestPrice > sim.moneyCap32(playerId()))
										),
										this, CallbackEnum::SpyEstablishNest, true, false, objectId
									);

									// Tooltip
									AddToolTip(button,
										LOCTEXT("EstablishSpyNest_Tip", "Establish a Spy Nest.<space>Spy Cards used in this City will not reveal your identity.<space>+30% Spy Bonus when using Spy Cards in this City.")
									);
								}
								else
								{
									FText anonymityText;
									bool showEnabled = true;

									if (house->isConcealed()) {
										anonymityText = LOCTEXT("EnsureAnonymity_Button", "Ensure Anonymity\n(Upgraded)");
										showEnabled = false;
									}
									else {
										int32 spyNestPrice = sim.GetSpyNestPrice(playerId(), house->townId());

										anonymityText = FText::Format(
											LOCTEXT("EnsureAnonymity_Button", "Ensure Anonymity\n<img id=\"Coin\"/>{0}"),
											TextRed(TEXT_NUM(spyNestPrice), spyNestPrice > sim.moneyCap32(playerId()))
										);
									}

									UPunButton* button = focusBox->AddButton2Lines(
										anonymityText,
										this, CallbackEnum::SpyEnsureAnonymity, showEnabled, false, objectId
									);

									// Tooltip
									AddToolTip(button,
										LOCTEXT("ConcealSpyNest_Tip", "Upgrade benefit: If your Spy Nest is found, you will not be revealed as the owner of this Spy Nest.")
									);
								}
							}

						}
					}
					else if (building.isEnum(CardEnum::Townhall))
					{
						TownHall& townhall = building.subclass<TownHall>();
						int32 townPlayerId = townhall.playerId();
						auto& townManager = sim.townManager(townhall.townId());
						auto& townhallPlayerOwned = sim.playerOwned(townPlayerId);
						
						FString townName = sim.townNameT(building.townId()).ToString();

						Indent(10);
						
						if (townPlayerId == playerId()) {
							focusBox->AddWGT_TownhallEditableTextRow(
								LOCTEXT("Town Name", "Town Name"),
								FText::FromString(TrimStringF_Dots(townName, 12)),
								this, CallbackEnum::TownNameEdit
							);
						}
						else {
							focusBox->AddWGT_TextRow(UIEnum::WGT_ObjectFocus_TextRow,
								LOCTEXT("Town Name", "Town Name"),
								FText::FromString(TrimStringF_Dots(townName, 15))
							);
						}

						FString playerNameTrimmed = TrimStringF_Dots(sim.playerNameF(townPlayerId), 12);
						focusBox->AddWGT_TextRow(UIEnum::WGT_ObjectFocus_TextRow,
							LOCTEXT("Player", "Player"),
							FText::FromString(playerNameTrimmed)
						);

#if !UE_BUILD_SHIPPING
						if (PunSettings::IsOn("DebugFocusUI")) {
							focusBox->AddRichText(TEXT_TAG("<Yellow>", INVTEXT("PlayerId")), FText::AsNumber(townPlayerId));
						}
#endif
						
						focusBox->AddWGT_TextRow(UIEnum::WGT_ObjectFocus_TextRow, 
							LOCTEXT("Size", "Size"), 
							townManager.GetTownSizeName()
						);

						//focusBox->AddWGT_TextRow(UIEnum::WGT_ObjectFocus_TextRow, 
						//	LOCTEXT("TownLevel", "Town Level"), 
						//	FText::AsNumber(lvl)
						//);

						// Only show these if it is player's townhall
						//if (townPlayerId == playerId()) {
						//	focusBox->AddWGT_TextRow(UIEnum::WGT_ObjectFocus_TextRow,
						//		LOCTEXT("TownhallIncome", "Townhall Income"),
						//		FText::AsNumber(townhall.townhallIncome()),
						//		assetLoader->CoinIcon
						//	);
						//}

						// Show how much money he has..
						if (townPlayerId != playerId())
						{
							//ADDTEXT_(INVTEXT("<img id=\"Coin\"/>{0}"), FText::AsNumber(sim.globalResourceSystem(townPlayerId).money()));
							//focusBox->AddRichText(LOCTEXT("Money", "Money"), args);
							focusBox->AddWGT_TextRow(UIEnum::WGT_ObjectFocus_TextRow,
								LOCTEXT("Money", "Money"),
								FText::AsNumber(sim.globalResourceSystem(townPlayerId).money()),
								assetLoader->CoinIcon
							);

							//ADDTEXT_(INVTEXT("<img id=\"Coin\"/>{0}"), TEXT_100(townManager.totalIncome100()));
							//focusBox->AddRichText(LOCTEXT("Money Income", "Money Income"), args);
							focusBox->AddWGT_TextRow(UIEnum::WGT_ObjectFocus_TextRow,
								LOCTEXT("Money Income", "Money Income"),
								TEXT_100(townManager.totalIncome100()),
								assetLoader->CoinIcon
							);
							
							focusBox->AddSpacer();

							if (sim.unlockedInfluence(townPlayerId)) 
							{
								//ADDTEXT_(INVTEXT("<img id=\"Influence\"/>{0}"), FText::AsNumber(sim.globalResourceSystem(townPlayerId).influence()));
								//focusBox->AddRichText(LOCTEXT("Influence", "Influence"), args);
								focusBox->AddWGT_TextRow(UIEnum::WGT_ObjectFocus_TextRow,
									LOCTEXT("Influence", "Influence"),
									FText::AsNumber(sim.globalResourceSystem(townPlayerId).influence()),
									assetLoader->InfluenceIcon
								);
								
								//ADDTEXT_(INVTEXT("<img id=\"Influence\"/>{0}"), TEXT_100(townManager.totalInfluenceIncome100()));
								//focusBox->AddRichText(LOCTEXT("Influence Income", "Influence Income"), args);
								focusBox->AddWGT_TextRow(UIEnum::WGT_ObjectFocus_TextRow,
									LOCTEXT("Influence Income", "Influence Income"),
									TEXT_100(townManager.totalInfluenceIncome100()),
									assetLoader->InfluenceIcon
								);

								
								focusBox->AddSpacer();
							}

							int32 science100PerRound = sim.GetScience100PerRound(townPlayerId);
							if (science100PerRound > 0) {
								focusBox->AddWGT_TextRow(UIEnum::WGT_ObjectFocus_TextRow,
									LOCTEXT("Science", "Science"),
									TEXT_100(science100PerRound),
									assetLoader->ScienceIcon
								);
								
								focusBox->AddSpacer();
							}
							

							focusBox->AddWGT_TextRow(UIEnum::WGT_ObjectFocus_TextRow,
								LOCTEXT("Happiness", "Happiness"),
								FText::AsNumber(sim.GetAverageHappiness(townhall.townId())),
								assetLoader->SmileIcon
							);
						}

						ResetIndent();
						//focusBox->AddSpacer(10);

						// Only show Independence/Allies status for other player's town
						// TODO: Diplomacy panel...
						if (townPlayerId != playerId())
						{
							focusBox->AddSpacer();
							
							// Lord
							if (townManager.lordPlayerId() != -1) {
								focusBox->AddWGT_TextRow(UIEnum::WGT_ObjectFocus_TextRow, 
									LOCTEXT("Lord", "Lord"), 
									sim.playerNameT(townManager.lordPlayerId())
								);
							}
							else {
								focusBox->AddWGT_PunText(UIEnum::WGT_ObjectFocus_TextLeft, 
									LOCTEXT("Independent State", "Independent State")
								); // TODO: many vassal => empire
							}

							// Allies
							const auto& allyPlayerIds = townManager.allyPlayerIds();
							if (allyPlayerIds.size() > 0)
							{
								// Expanded text part
								for (int32 i = 1; i < allyPlayerIds.size(); i++) {
									ADDTEXT_(INVTEXT("{0}\n"), sim.GetTownhallCapital(allyPlayerIds[i]).townNameT());
								}

								focusBox->AddRichText(LOCTEXT("Allies", "Allies"),
									sim.GetTownhallCapital(allyPlayerIds[0]).townNameT(), ResourceEnum::None, JOINTEXT(args)
								);
								args.Empty();
							}
							else {
								focusBox->AddRichText(LOCTEXT("Allies", "Allies"), LOCTEXT("None", "None"));
							}

							// Vassals
							const auto& vassalTownIds = townManager.vassalTownIds();
							{
								TArray<FText> vassalArgs;

								for (int32 i = 1; i < vassalTownIds.size(); i++) {
									ADDTEXT(vassalArgs, INVTEXT("{0}\n"), sim.townOrPlayerNameT(vassalTownIds[i]));
								}

								if (vassalTownIds.size() > 0) {
									focusBox->AddRichText(LOCTEXT("Vassals", "Vassals"), sim.townOrPlayerNameT(vassalTownIds[0]), ResourceEnum::None, JOINTEXT(vassalArgs));
								}
								else {
									focusBox->AddRichText(LOCTEXT("Vassals", "Vassals"), LOCTEXT("None", "None"));
								}

							}
						}

						// Note: Statistics replaced with Statistics Bureau
						//descriptionBox->AddButton("Show Statistics", nullptr, "", this, CallbackEnum::OpenStatistics, true, false, townhall.playerId());
						//focusBox->AddLineSpacer(10);

						if (building.playerId() == playerId())
						{
							//descriptionBox->AddButton("Set Trade Offers", nullptr, "", this, CallbackEnum::OpenSetTradeOffers, true, false, townhall.playerId());
							//descriptionBox->AddLineSpacer(10);

							// Abandon town
							//auto button = descriptionBox->AddButtonRed(FText(), LOCTEXT("AbandonTown", "Abandon Town"), nullptr, FText(), this, CallbackEnum::AbandonTown);
							//AddToolTip(button, LOCTEXT("AbandonTownTooltip", "Abandon this town to build a new one. (Destroy this town)"));
							//descriptionBox->AddLineSpacer(10);
						}
						else
						{
							// Vassalize
							if (sim.IsResearched(playerId(), TechEnum::Vassalize))
							{

								
							}
						}

					}
					else if (IsProfitBuilding(building.buildingEnum())) 
					{	
						//ADDTEXT_(INVTEXT("{0}<img id=\"Coin\"/>"), FText::AsNumber(static_cast<Bank*>(&building)->lastRoundProfit));
						focusBox->AddWGT_TextRow(UIEnum::WGT_ObjectFocus_TextRow,
							LOCTEXT("Round profit", "Round profit"),
							FText::AsNumber(static_cast<Bank*>(&building)->lastRoundProfit),
							assetLoader->CoinIcon
						);
					}
					else if (building.isEnum(CardEnum::Farm)) 
					{
						Farm& farm = building.subclass<Farm>();

						int32 fertility = farm.fertility();
						focusBox->AddWGT_TextRow(UIEnum::WGT_ObjectFocus_TextRow, 
							LOCTEXT("Fertility", "Fertility"), TEXT_PERCENT(fertility)
						);

						AddEfficiencyText(farm, focusBox);

						focusBox->AddWGT_TextRow(UIEnum::WGT_ObjectFocus_TextRow, 
							LOCTEXT("Work stage", "Work stage"), farm.farmStageName()
						);
						focusBox->AddWGT_TextRow(UIEnum::WGT_ObjectFocus_TextRow, 
							LOCTEXT("Growth percent", "Growth percent"), TEXT_PERCENT(farm.MinCropGrowthPercent())
						);

#if !UE_BUILD_SHIPPING
						if (PunSettings::IsOn("DebugFocusUI")) {
							ADDTEXT_(INVTEXT("(Editor)WorkedTiles: {0}\n"), TEXT_NUM(farm.workedTiles()));
							ADDTEXT_(INVTEXT("(Editor)adjacents: +{0}\n"), TEXT_NUM(building.adjacentCount()));
							focusBox->AddSpecialRichText(INVTEXT("<Yellow>"), args);
						}
#endif
						focusBox->AddRichText(LOCTEXT("Farm_CurrentPlant", "Current"), GetTileObjInfo(farm.currentPlantEnum).name);
					}
					else if (IsRanch(building.buildingEnum()))
					{
						Ranch& ranch = building.subclass<Ranch>();
						//ADDTEXT_(INVTEXT("{0}/{1}"), TEXT_NUM(ranch.animalOccupants().size()), TEXT_NUM(ranch.maxAnimals));
						//focusBox->AddRichText(LOCTEXT("Animals:", "Animals:"), args);
						focusBox->AddWGT_TextRow(UIEnum::WGT_ObjectFocus_TextRow,
							LOCTEXT("Animals:", "Animals:"),
							FText::Format(INVTEXT("{0}/{1}"), TEXT_NUM(ranch.animalOccupants().size()), TEXT_NUM(ranch.maxAnimals))
						);

						AddEfficiencyText(ranch, focusBox);

						// Add Animal Button
						if (building.playerId() == playerId())
						{
							int32 animalCost = ranch.animalCost();
							bool showEnabled = ranch.openAnimalSlots() > 0;
							
							TArray<FText> argsLocal;
							argsLocal.Add(LOCTEXT("Buy Animal", "Buy Animal"));
							ADDTEXT(argsLocal, INVTEXT("\n<img id=\"Coin\"/>{0}"), TextRed(FText::AsNumber(animalCost), animalCost > sim.moneyCap32(playerId())));

							focusBox->AddSpacer();
							focusBox->AddButton2Lines(JOINTEXT(argsLocal), this, CallbackEnum::AddAnimalRanch, showEnabled, false, objectId);
							focusBox->AddSpacer(8);
						}

						// Selection Meshes
						const std::vector<int32_t>& occupants = ranch.animalOccupants();
						for (int occupantId : occupants) {
							WorldAtom2 atom = dataSource()->unitDataSource().actualAtomLocation(occupantId);
							SpawnSelectionMesh(assetLoader->SelectionMaterialYellow, dataSource()->DisplayLocation(atom) + FVector(0, 0, 30));
						}
					}
					// TODO: Boar should just use the normal inventory?
					else if (building.isEnum(CardEnum::BoarBurrow)) 
					{
						focusBox->AddSpacer();
						focusBox->AddWGT_PunRichText(UIEnum::WGT_ObjectFocus_FlavorText,
							building.buildingInfo().GetDescription()
						);
						
						ADDTEXT__(building.subclass<BoarBurrow>().inventory.ToText());
						focusBox->AddRichText(args);
					}

					else if (IsSpecialProducer(building.buildingEnum())) 
					{
						AddEfficiencyText(building, focusBox);

						if (building.isEnum(CardEnum::Mint)) {
							focusBox->AddWGT_TextRow(UIEnum::WGT_ObjectFocus_TextRow, 
								LOCTEXT("Income(per season)", "Income(per season)"), 
								TEXT_NUM(building.seasonalProduction()),
								assetLoader->CoinIcon
							);
						}
						if (building.isEnum(CardEnum::ResearchLab)) {
							focusBox->AddWGT_TextRow(UIEnum::WGT_ObjectFocus_TextRow, 
								LOCTEXT("Science(per season)", "Science(per season)"), 
								TEXT_NUM(building.seasonalProduction()),
								assetLoader->ScienceIcon
							);
						}
						if (building.isEnum(CardEnum::RegionShrine)) {
							focusBox->AddWGT_TextRow(UIEnum::WGT_ObjectFocus_TextRow, 
								LOCTEXT("Science(per season)", "Science(per season)"), 
								TEXT_NUM(building.seasonalProduction()),
								assetLoader->ScienceIcon
							);
						}
						if (IsBarrack(building.buildingEnum())) {
							focusBox->AddWGT_TextRow(UIEnum::WGT_ObjectFocus_TextRow, 
								LOCTEXT("Influence(per season)", "Influence(per season)"), 
								TEXT_NUM(building.seasonalProduction()),
								assetLoader->InfluenceIcon
							);
						}
					}
					else if (IsStorage(building.buildingEnum()))
					{
						if (!building.isEnum(CardEnum::Market))
						{
							StorageYard& storage = building.subclass<StorageYard>();

							Indent(50);
							focusBox->AddWGT_TextRow(UIEnum::WGT_ObjectFocus_TextRow, 
								LOCTEXT("Slots", "Slots"),
								FText::Format(INVTEXT("{0}/{1}"), TEXT_NUM(storage.tilesOccupied()), TEXT_NUM(storage.storageSlotCount()))
							);
							ResetIndent();
							focusBox->AddSpacer();
						}
					}
					else if (IsFunServiceBuilding(building.buildingEnum()))
					{
						FunBuilding& funBuilding = building.subclass<FunBuilding>();
						int32 guestCount = funBuilding.guestCountLastRound() + funBuilding.guestCountThisRound();
						focusBox->AddWGT_TextRow(UIEnum::WGT_ObjectFocus_TextRow, 
							LOCTEXT("Guests: ", "Guests: "), TEXT_NUM(guestCount)
						);

						focusBox->AddWGT_TextRow(UIEnum::WGT_ObjectFocus_TextRow,
							LOCTEXT("Service Quality: ", "Service Quality: "), TEXT_NUM(funBuilding.serviceQuality())
						);
					}
					else if (building.isEnum(CardEnum::Library) ||
							building.isEnum(CardEnum::School)) 
					{
						focusBox->AddWGT_PunRichText(UIEnum::WGT_ObjectFocus_FlavorText,
							building.buildingInfo().GetDescription()
						);
					}
					//else if (building.isEnum(CardEnum::OreSupplier)) 
					//{
					//	descriptionBox->AddRichText(building.buildingInfo().GetDescription());
					//	descriptionBox->AddLineSpacer();

					//	int32 targetBuyAmount = building.subclass<OreSupplier>().maxBuyAmount();
					//	descriptionBox->AddRichText(LOCTEXT("Resource Type", "Resource Type"), LOCTEXT("Iron ore", "Iron ore"));
					//	descriptionBox->AddRichText(LOCTEXT("Target Buy Amount", "Target Buy Amount"), TEXT_NUM(targetBuyAmount));
					//}
					else if (IsTradingPostLike(building.buildingEnum()))
					{
						if (building.isConstructed())
						{
							auto tradingPost = static_cast<TradingPost*>(&building);
							int32 countdown = tradingPost->CountdownSecondsDisplay();

							AddTradeFeeText(building.subclass<TradeBuilding>(), focusBox);
							AddEfficiencyText(building, focusBox);

//#if WITH_EDITOR 
//							ADDTEXT_(INVTEXT("-- Ticks from last check: {0} ticks"), TEXT_NUM(tradingPost->ticksFromLastCheck()));
//							descriptionBox->AddRichText(args);
//							ADDTEXT_(INVTEXT("-- lastCheckTick: {0} secs"), TEXT_NUM(tradingPost->lastCheckTick()));
//							descriptionBox->AddRichText(args);
//#endif
							focusBox->AddWGT_TextRow(UIEnum::WGT_ObjectFocus_TextRow, 
								LOCTEXT("Trade complete in", "Trade complete in"), 
								FText::Format(LOCTEXT("{0} secs", "{0} secs"), TEXT_NUM(countdown))
							);

							bool showEnabled = (tradingPost->playerId() == playerId()) && tradingPost->CanTrade();
							focusBox->AddButton(LOCTEXT("Trade", "Trade"), nullptr, FText(), this, CallbackEnum::TradingPostTrade, showEnabled, false, objectId);
							
						}
					}
					else if (building.isEnum(CardEnum::TradingCompany))
					{
						auto& tradingCompany = building.subclass<TradingCompany>();

						// Shouldn't be able to tamper with other ppl's trade
						if (showWhenOwnedByCurrentPlayer)
						{
							
						}

						focusBox->AddWGT_TextRow(UIEnum::WGT_ObjectFocus_TextRow,
							LOCTEXT("Trade per Round", "Trade Quantity per Round"),
							TEXT_NUM(tradingCompany.tradeMaximumPerRound())
						);

					}
					else if (building.isEnum(CardEnum::TourismAgency))
					{
						int32 townId = building.townId();
						TownManager& townManager = sim.townManager(townId);
						int32 aveHappiness = townManager.aveHappinessByType(HappinessEnum::Tourism);
						
						focusBox->AddRichText(
							LOCTEXT("Tourism Happiness", "Tourism Happiness"), 
							FText::Format(
								INVTEXT("{0}{1}"),
								GetHappinessFace(aveHappiness),
								ColorHappinessText(aveHappiness, FText::Format(INVTEXT("{0}%"), TEXT_NUM(aveHappiness)))
							)
						);

						int32 hotelsOnTradeRoutes = 0;
						int32 aveHotelServiceQuality = 0;
						
						std::vector<TradeRoutePair> routes = sim.worldTradeSystem().GetTradeRoutesTo(townId);
						for (TradeRoutePair& route : routes) {
							const std::vector<int32>& hotelIds = sim.buildingIds(route.GetCounterpartTownId(townId), CardEnum::Hotel);
							hotelsOnTradeRoutes += hotelIds.size();
							for (int32 hotelId : hotelIds) {
								aveHotelServiceQuality += sim.building<Hotel>(hotelId).serviceQuality();
							}
						}

						aveHotelServiceQuality = aveHotelServiceQuality / std::max(1, hotelsOnTradeRoutes);

						focusBox->AddWGT_TextRow(UIEnum::WGT_ObjectFocus_TextRow, LOCTEXT("Trade Routes", "Trade Routes"), TEXT_NUM(routes.size()));
						focusBox->AddWGT_TextRow(UIEnum::WGT_ObjectFocus_TextRow, LOCTEXT("Hotels on Trade Routes", "Hotels on Trade Routes"), TEXT_NUM(hotelsOnTradeRoutes));
						focusBox->AddWGT_TextRow(UIEnum::WGT_ObjectFocus_TextRow, LOCTEXT("Average Hotel Service Quality", "Average Hotel Service Quality"), TEXT_PERCENT(aveHotelServiceQuality));
						
						focusBox->AddSpacer();
					}
					else if (building.isEnum(CardEnum::Hotel))
					{
						Hotel& hotel = building.subclass<Hotel>();
						
						focusBox->AddWGT_TextRow(UIEnum::WGT_ObjectFocus_TextRow, LOCTEXT("Service Quality", "Service Quality"), TEXT_PERCENT(hotel.serviceQuality()));

						focusBox->AddSpacer();
						UPunEditableNumberBox* numberBox = focusBox->AddEditableNumberBox(this, CallbackEnum::EditableNumberSetHotelFeePerVisitor, building.buildingId(),
							LOCTEXT("Fee per visitor", "Fee per visitor"), hotel.feePerVisitorDisplay()
						);
						numberBox->incrementMultiplier = 1;
						numberBox->shiftIncrementMultiplier = 10;
						numberBox->minAmount = 0;

						focusBox->AddSpacer();
						focusBox->AddWGT_TextRow(UIEnum::WGT_ObjectFocus_TextRow, LOCTEXT("Visitors", "Visitors"), FText::Format(
							INVTEXT("{0}/{1}"),
							TEXT_NUM(hotel.currentVisitorCount()),
							TEXT_NUM(hotel.maxVisitorCount())
						));

						focusBox->AddSpacer();

						args.Empty();
						TMap<int32, int32> townToVisitors = hotel.GetTownToVisitors();

						if (townToVisitors.Num() > 0)
						{
							args.Add(LOCTEXT("Visitors from:", "Visitors from:\n"));
							for (const auto& townToVisitor : townToVisitors) {
								args.Add(FText::Format(INVTEXT("  {0} {1}"), sim.townNameT(townToVisitor.Key), TEXT_NUM(townToVisitor.Value)));
							}
						}
						focusBox->AddRichText(JOINTEXT(args));

						focusBox->AddSpacer();
						std::vector<TradeRoutePair> routes = sim.worldTradeSystem().GetTradeRoutesTo(hotel.townId());
						focusBox->AddWGT_TextRow(UIEnum::WGT_ObjectFocus_TextRow, LOCTEXT("Trade Routes", "Trade Routes"), TEXT_NUM(routes.size()));
						
					}
					//! AncientWonder
					else if (IsAncientWonderCardEnum(building.buildingEnum()))
					{
						const ProvinceRuin& provinceRuin = building.subclass<ProvinceRuin>();

						focusBox->AddRichText(
							LOCTEXT("AncientWonder Desc", "+50<img id=\"Coin\"/> Province Base Income")
						);

						if (sim.IsResearched(playerId(), TechEnum::Museum))
						{
							int32 digDistance = provinceRuin.GetDigDistance(playerId());
							if (digDistance != INT32_MAX)
							{
								focusBox->AddWGT_TextRow(UIEnum::WGT_ObjectFocus_TextRow,
									LOCTEXT("Distance to your nearest Townhall", "Distance to Townhall"),
									TEXT_NUM(digDistance)
								);
							}

							if (sim.IsTileOwnedByForeignPlayer(playerId(), provinceRuin.centerTile())) {
								focusBox->AddWGT_WarningText(TEXT_TAG("<Red>", LOCTEXT("RuinFarFromTownhall_FocusUI", "Excavating sneakily in foreign territory incur cost penalty")));
							}
							else if (provinceRuin.GetDigDistanceFactor(playerId()) > 100) {
								focusBox->AddWGT_WarningText(TEXT_TAG("<Red>", LOCTEXT("RuinFarFromTownhall_FocusUI", "Excavating far from Townhall incur cost penalty")));
							}
						}
					}
					//! Diplomatic
					else if (IsForeignOnlyBuilding(building.buildingEnum()))
					{
						const DiplomaticBuilding& diplomaticBuilding = building.subclass<DiplomaticBuilding>();

						bool isHostOrForeignBuilder = building.playerId() == playerId() || building.foreignBuilderId() == playerId();
						if (isHostOrForeignBuilder) {
							focusBox->AddWGT_TextRow(UIEnum::WGT_ObjectFocus_TextRow, LOCTEXT("Influence Income", "Influence Income"), TEXT_100(diplomaticBuilding.influenceIncome100(playerId())), assetLoader->InfluenceIcon);
							focusBox->AddWGT_TextRow(UIEnum::WGT_ObjectFocus_TextRow, LOCTEXT("Money Income", "Money Income"), TEXT_100(diplomaticBuilding.moneyIncome100(playerId())), assetLoader->CoinIcon);
						}
						
					}
					//! Spy Center
					else if (building.isEnum(CardEnum::SpyCenter))
					{
						auto spyEffectivenessWidget = focusBox->AddWGT_TextRow(UIEnum::WGT_ObjectFocus_TextRow, LOCTEXT("Spy Effectiveness", "Spy Effectiveness"), TEXT_PERCENT(sim.GetSpyEffectiveness(building.playerId())));
						AddToolTip(spyEffectivenessWidget, LOCTEXT("Spy Effectiveness Tip", "Effectiveness of Spy Nest, Steal, Kidnap, Terrorism"));

						auto counterspyEffectivenessWidget = focusBox->AddWGT_TextRow(UIEnum::WGT_ObjectFocus_TextRow, LOCTEXT("Counterintelligence", "Counterintelligence"), TEXT_PERCENT(sim.GetSpyEffectiveness(building.playerId(), true)));
						AddToolTip(counterspyEffectivenessWidget, LOCTEXT("Counterintelligence Effectiveness Tip", "Counterintelligence Effectiveness nullify opponent's Spy Effectiveness"));

						focusBox->AddSpacer();
						int32 cardsReadyIn = building.subclass<SpyCenter>().secsToCardProduction();
						if (cardsReadyIn != -1) {
							focusBox->AddWGT_TextRow(UIEnum::WGT_ObjectFocus_TextRow, LOCTEXT("Card ready in", "Card ready in"), FText::Format(LOCTEXT("{0}seconds", "{0}s"), TEXT_NUM(cardsReadyIn)));
						}
					}
					
					else if (building.isEnum(CardEnum::ShippingDepot))
					{
						focusBox->AddWGT_PunRichText(UIEnum::WGT_ObjectFocus_FlavorText,
							LOCTEXT("Choose3Resources", "Choose 3 resources to haul to delivery target")
						);
						focusBox->AddSpacer(12);
						
						TArray<FText> options;
						options.Add(LOCTEXT("None", "None"));
						for (ResourceInfo info : SortedNameResourceInfo) {
							options.Add(info.GetName());
						}
						
						auto addDropDown = [&](int32 index)
						{
							ResourceEnum resourceEnum = building.subclass<ShippingDepot>().resourceEnums[index];
							
							focusBox->AddDropdown(
								building.buildingId(),
								options,
								ResourceName_WithNone(resourceEnum),
								[](int32 objectId, FString sItem, IGameUIDataSource* dataSource, IGameNetworkInterface* networkInterface, int32 dropdownIndex, int32 optionInt)
							{
								auto command = make_shared<FChangeWorkMode>();
								command->buildingId = objectId;
								command->intVar1 = dropdownIndex;
								std::wstring resourceName = ToWString(sItem);
								command->intVar2 = static_cast<int32>(FindResourceEnumByName(resourceName));
								networkInterface->SendNetworkCommand(command);
							}, index);
						};

						addDropDown(0);
						addDropDown(1);
						addDropDown(2);
					}
					else if (building.isEnum(CardEnum::IntercityLogisticsHub) ||
								building.isEnum(CardEnum::IntercityLogisticsPort))
					{
						if (showWhenOwnedByCurrentPlayer)
						{
							focusBox->AddWGT_PunRichText(UIEnum::WGT_ObjectFocus_FlavorText,
								LOCTEXT("IntercityLogisticsHub_ChooseResources", "Choose 4 resources to take from target town.")
							);
							focusBox->AddSpacer(18);

							auto& hub = building.subclass<IntercityLogisticsHub>();

							// targetAmount
							// - just opened UI, get it from targetAmount (actual value)
							// - after opened, we keep value in lastTargetAmountSet
							if (_justOpenedDescriptionUI) {
								hub.lastTargetTownId = hub.targetTownId;
								hub.lastResourceEnums = hub.resourceEnums;
								hub.lastResourceCounts = hub.resourceCounts;
							}
							int32 targetTownId = hub.lastTargetTownId;
							vector<ResourceEnum> resourceEnums = hub.lastResourceEnums;
							vector<int32> resourceCounts = hub.lastResourceCounts;

							const FText noneText = LOCTEXT("None", "None");

							int32 numberBoxStartIndex = resourceEnums.size();
							int32 targetTownDropdownIndex = resourceEnums.size() * 2; // Last Index

							// Target Town Dropdown
							{
								focusBox->AddWGT_PunText(UIEnum::WGT_ObjectFocus_Subheader, 
									LOCTEXT("TargetTownDropdown", "Town Target:")
								);
								
								TArray<FText> options;
								options.Add(noneText);

								std::vector<int32> tradableTownIds;
								if (building.isEnum(CardEnum::IntercityLogisticsHub)) {
									if (_justOpenedDescriptionUI) {
										tradableTownIds = hub.GetTradableTownIdsLand();
									}
									else {
										tradableTownIds = hub.cachedLandConnectedTownIds;
									}
								}
								else {
									tradableTownIds = hub.GetTradableTownIdsWater();
								}

								for (int32 townId : tradableTownIds) {
									if (townId != building.townId()) {
										options.Add(sim.townNameT(townId));
									}
								}

								focusBox->AddDropdown(
									building.buildingId(),
									options,
									sim.townNameT(targetTownId),
									[](int32 objectId, FString sItem, IGameUIDataSource* dataSource, IGameNetworkInterface* networkInterface, int32 dropdownIndex, int32 optionInt)
									{
										auto command = make_shared<FChangeWorkMode>();
										command->buildingId = objectId;
										command->intVar1 = dropdownIndex;
										command->intVar2 = dataSource->simulation().FindTownIdFromName(networkInterface->playerId(), sItem);
										networkInterface->SendNetworkCommand(command);
									},
									targetTownDropdownIndex
								);
								focusBox->AddSpacer(18);
							}


							// Resource Dropdown/Target Amount
							{
								TArray<FText> options;
								options.Add(noneText);
								options.Add(LOCTEXT("Food", "Food"));
								//options.Add(LOCTEXT("Fuel", "Fuel"));
								for (ResourceInfo info : SortedNameResourceInfo) {
									options.Add(info.GetName());
								}

								auto addDropDown = [&](int32 index)
								{
									ResourceEnum resourceEnum = resourceEnums[index];

									focusBox->AddDropdown(
										building.buildingId(),
										options,
										ResourceName_WithNone(resourceEnum),
										[](int32 objectId, FString sItem, IGameUIDataSource* dataSource, IGameNetworkInterface* networkInterface, int32 dropdownIndex, int32 optionInt)
									{
										std::wstring resourceName = ToWString(sItem);

										auto command = make_shared<FChangeWorkMode>();
										command->buildingId = objectId;
										command->intVar1 = dropdownIndex;
										command->intVar2 = static_cast<int32>(FindResourceEnumByName(resourceName));
										networkInterface->SendNetworkCommand(command);
									},
										index
										);

									focusBox->AddEditableNumberBox(
										building.buildingId(),
										index + numberBoxStartIndex, FText::Format(INVTEXT("{0}: "), LOCTEXT("Target", "Target")),
										resourceCounts[index],
										[numberBoxStartIndex](int32 objectId, int32 uiIndex, int32 amount, IGameNetworkInterface* networkInterface)
									{
										auto command = make_shared<FChangeWorkMode>();
										command->buildingId = objectId;
										command->intVar1 = uiIndex;
										command->intVar2 = amount;
										networkInterface->SendNetworkCommand(command);

										// Update Display
										auto& sim = networkInterface->dataSource()->simulation();
										if (sim.IsValidBuilding(objectId)) {
											Building& bld = sim.building(objectId);
											if (bld.isEnum(CardEnum::IntercityLogisticsHub)) {
												bld.subclass<IntercityLogisticsHub>().lastResourceCounts[uiIndex - numberBoxStartIndex] = amount;
											}
										}
									},
										FText(), false, resourceEnums[index]
										);

									focusBox->AddSpacer(12);
								};

								for (int32 i = 0; i < resourceEnums.size(); i++) {
									addDropDown(i);
								}
							}
						}
					}
					else if (building.isEnum(CardEnum::Caravansary))
					{
						if (showWhenOwnedByCurrentPlayer)
						{
							auto& caravansary = building.subclass<Caravansary>();
							

							// targetAmount
							// - just opened UI, get it from targetAmount (actual value)
							// - after opened, we keep value in lastTargetAmountSet
							if (_justOpenedDescriptionUI) {
								caravansary.lastTargetTownId = caravansary.targetTownId();
							}
							int32 targetTownId = caravansary.lastTargetTownId;

							const FText noneText = LOCTEXT("None", "None");

							// Target Town Dropdown
							{
								focusBox->AddWGT_PunText(UIEnum::WGT_ObjectFocus_Subheader,
									LOCTEXT("TargetTownDropdown", "Town Target:")
								);

								TArray<FText> options;
								TArray<int32> optionInts;
								options.Add(noneText);
								optionInts.Add(-1);

								std::vector<TradeRoutePair> tradeRoutes = sim.worldTradeSystem().GetTradeRoutesTo(caravansary.townId());

								for (TradeRoutePair tradeRoutePair : tradeRoutes) {
									int32 possibleTownId = tradeRoutePair.GetCounterpartTownId(building.townId());
									options.Add(sim.townNameT(possibleTownId));
									optionInts.Add(possibleTownId);
								}

							
								if (tradeRoutes.size() > 0)
								{
									focusBox->AddDropdown(
										building.buildingId(),
										options,
										targetTownId != -1 ? sim.townNameT(targetTownId) : noneText,
										[](int32 objectId, FString sItem, IGameUIDataSource* dataSource, IGameNetworkInterface* networkInterface, int32 dropdownIndex, int32 optionInt)
										{
											auto command = make_shared<FChangeWorkMode>();
											command->buildingId = objectId;
											command->intVar1 = optionInt;
											networkInterface->SendNetworkCommand(command);
										},
										0,
										optionInts
									);
								}
								//else {
								//	focusBox->AddRichText(TEXT_TAG("<Red>", LOCTEXT("Need Trade Route", "Need Trade Route")));
								//}
								focusBox->AddSpacer(18);
							}

							if (caravansary.targetTownId() != -1) {
								WorldTile2 targetTile = simulation().GetTownhallGate_All(caravansary.targetTownId());
								focusBox->AddWGT_TextRow(UIEnum::WGT_ObjectFocus_TextRow, LOCTEXT("Profit per trip", "Profit per trip"), TEXT_NUM(caravansary.GetTradeMoney(targetTile)));
							}
							
						}
					}
					else if (building.isEnum(CardEnum::WorldTradeOffice))
					{
						if (showWhenOwnedByCurrentPlayer)
						{
							auto& bld = building.subclass<WorldTradeOffice>();

							focusBox->AddRichText(LOCTEXT("WorldTradeOffice Description", "Every 30 secs:\n- spend 30<img id=\"Influence\"/>\n- increase price of goods by 1%"));
							focusBox->AddSpacer(24);
							
							// targetAmount
							// - just opened UI, get it from targetAmount (actual value)
							// - after opened, we keep value in lastTargetAmountSet
							if (_justOpenedDescriptionUI) {
								bld.lastResourceEnumToIncreasePrice = bld.resourceEnumToIncreasePrice;
								bld.lastResourceEnumToDecreasePrice = bld.resourceEnumToDecreasePrice;
							}

							const FText noneText = LOCTEXT("None", "None");

							TArray<FText> options;
							options.Add(noneText);
							for (ResourceInfo info : SortedNameResourceInfo) {
								options.Add(info.GetName());
							}

							auto addDropDown = [&](int32 index, ResourceEnum resourceEnum)
							{
								focusBox->AddDropdown(
									building.buildingId(),
									options,
									ResourceName_WithNone(resourceEnum),
									[](int32 objectId, FString sItem, IGameUIDataSource* dataSource, IGameNetworkInterface* networkInterface, int32 dropdownIndex, int32 optionInt)
									{
										std::wstring resourceName = ToWString(sItem);

										auto command = make_shared<FChangeWorkMode>();
										command->buildingId = objectId;
										command->intVar1 = dropdownIndex;
										command->intVar2 = static_cast<int32>(FindResourceEnumByName(resourceName));
										networkInterface->SendNetworkCommand(command);
									},
									index
								);

								focusBox->AddSpacer(24);
							};

							focusBox->AddRichText(LOCTEXT("Resource to increase price:", "Resource to increase price:"));
							addDropDown(0, bld.lastResourceEnumToIncreasePrice);

							focusBox->AddRichText(LOCTEXT("Resource to decrease price:", "Resource to decrease price:"));
							addDropDown(1, bld.lastResourceEnumToDecreasePrice);

						}
					}
					else if (IsMountainMine(building.buildingEnum()))
					{
						int32 oreLeft = building.subclass<Mine>().oreLeft();
						if (oreLeft > 0) {
							focusBox->AddWGT_TextRow_Resource(UIEnum::WGT_ObjectFocus_TextRow,
								LOCTEXT("Resource left", "Resource left"), 
								TEXT_NUM(oreLeft), 
								building.product()
							);
						} else {
							focusBox->AddRichText(TEXT_TAG("<Red>", LOCTEXT("Mine Depleted", "Mine Depleted")));
						}
					}
					else if (IsRegionalBuilding(building.buildingEnum()))
					{
						focusBox->AddWGT_PunRichText(UIEnum::WGT_ObjectFocus_FlavorText,
							building.buildingInfo().description
						);
					}
					else if (building.isEnum(CardEnum::ResourceOutpost))
					{
						auto& colony = building.subclass<ResourceOutpost>();
						focusBox->AddWGT_TextRow(UIEnum::WGT_ObjectFocus_TextRow,
							LOCTEXT("Upkeep: ", "Upkeep: "), 
							TEXT_NUM(colony.GetColonyUpkeep()),
							assetLoader->InfluenceIcon
						);

						focusBox->AddSpacer();

						ResourceEnum resourceEnum = colony.GetColonyResourceEnum();
						int32 resourceIncome = colony.GetColonyResourceIncome(resourceEnum);
						focusBox->AddWGT_TextRow_Resource(UIEnum::WGT_ObjectFocus_TextRow,
							LOCTEXT("Production (per round):", "Production (per round):"), 
							TEXT_NUM(resourceIncome), 
							resourceEnum
						);

						focusBox->AddSpacer();

						// Mine Resources
						if (IsOreEnum(resourceEnum))
						{
							int32 oreLeft = building.oreLeft();
							if (oreLeft > 0) {
								focusBox->AddWGT_TextRow_Resource(UIEnum::WGT_ObjectFocus_TextRow,
									LOCTEXT("Resource left", "Resource left"), 
									TEXT_NUM(oreLeft), 
									building.product()
								);
							}
							else {
								focusBox->AddWGT_PunRichText(UIEnum::WGT_ObjectFocus_FlavorText, 
									TEXT_TAG("<Red>", LOCTEXT("Mine Depleted", "Mine Depleted"))
								);
							}
						}
					}
					else if (building.isEnum(CardEnum::Fort))
					{
						focusBox->AddWGT_PunRichText(UIEnum::WGT_ObjectFocus_FlavorText,
							LOCTEXT("AttackRequires", "Attacking this province requires 100% more <img id=\"Influence\"/>.")
						);

						
					}
					else if (IsPowerPlant(buildingEnum))
					{
						focusBox->AddRichText(TEXT_TAG("<GrayBold>", LOCTEXT("Consumption (season):", "Consumption (per season):")),
							TEXT_NUM(building.seasonalConsumption1()), building.input1()
						);

						focusBox->AddLineSpacer();
						Indent(5);
						
						auto& powerPlant = building.subclass<PowerPlant>();

						auto addElectricityRow = [&](FText leftText, FText rightText, FText tipText)
						{
							auto electricityWidget = focusBox->AddWGT_TextRow(UIEnum::WGT_ObjectFocus_TextRow,
								leftText, rightText
							);

							args.Empty();
							args.Add(tipText);
							AddToolTip(electricityWidget, args);
						};
						
						auto electricityWidget = focusBox->AddWGT_PunText(UIEnum::WGT_ObjectFocus_Subheader,
							LOCTEXT("Electricity_FocusUI1", "Electricity Stats:")
						);
						args.Empty();
						args.Add(LOCTEXT("Electricity_Tip1", "Electricity production of 1 kW consumes 1 Coal per Round\n<Gray>(8 per year)</>"));
						AddToolTip(electricityWidget, args);
						

						addElectricityRow(
							LOCTEXT("Electricity_FocusUI2", "Building Production Capacity"),
							FText::Format(INVTEXT("{0}kW"), TEXT_NUM(powerPlant.ElectricityProductionCapacity())),
							LOCTEXT("Electricity_Tip2", "The maximum Electricity Production (kW) from this Building.")
						);

						auto& townManager = sim.townManager(powerPlant.playerId());
						addElectricityRow(
							LOCTEXT("Electricity_FocusUI3", "City Usage/Capacity"),
							FText::Format(INVTEXT("{0}/{1}kW"), TEXT_NUM(townManager.electricityConsumption()), TEXT_NUM(townManager.electricityProductionCapacity())),
							LOCTEXT("Electricity_Tip3", "City-wide Electricity:<space>[Consumption]/[Production Capacity].")
						);

						ResetIndent();
					}
					else if (IsWorldWonder(buildingEnum))
					{
					focusBox->AddWGT_TextRow(UIEnum::WGT_ObjectFocus_TextRow,
							LOCTEXT("WorldWonder_FocusUI_ThisWonderScore", "Wonder Score (This):"), 
							TEXT_NUM(building.subclass<WorldWonder>().WonderScore())
						);
					focusBox->AddWGT_TextRow(UIEnum::WGT_ObjectFocus_TextRow,
							LOCTEXT("WorldWonder_FocusUI_BuiltNumber", "Built Number:"), 
							TEXT_NUM(building.subclass<WorldWonder>().builtNumber)
						);

						focusBox->AddLineSpacer();
						
						focusBox->AddWGT_TextRow(UIEnum::WGT_ObjectFocus_TextRow,
							LOCTEXT("WorldWonder_FocusUI_PlayerScore", "Player's Total Score:"), 
							TEXT_NUM(sim.totalScore(building.playerId()))
						);

						ExecuteFillScoreBreakdownText([&](FText leftText, FText rightText) {
							return focusBox->AddWGT_TextRow(UIEnum::WGT_ObjectFocus_TextRow, leftText, rightText);
						}, building.playerId());
					}
					else {
						//ss << building.buildingInfo().description;
						//descriptionBox->AddRichText(ss);
					}
				}


				/*
				 * Special case: Manage Storage (Display regardless of if the storage is constructed
				 */
				bool isMarket = building.isEnum(CardEnum::Market);
				
				if ((IsStorage(building.buildingEnum()) || isMarket) &&
					building.ownedBy(playerId()))
				{
					const FText buttonName = isMarket ? LOCTEXT("Manage Market", "Manage Market") : LOCTEXT("Manage Storage", "Manage Storage");
					focusBox->AddButton(buttonName, nullptr, FText(), this, CallbackEnum::OpenManageStorage, true, false, objectId);

					/*
					 * Fill ManageStorage
					 */
					StorageBase& storage = building.subclass<StorageBase>();
					UPunBoxWidget* manageStorageBox = _objectDescriptionUI->ManageStorageBox;

					SetText(_objectDescriptionUI->ManageStorageTitle, buttonName);
					_objectDescriptionUI->ManageStorageWidthSizeBox->SetWidthOverride(isMarket ? 410 : 270);

					std::vector<ResourceInfo> resourceEnums = SortedNameResourceInfo;

					std::vector<int32> uiResourceTargetsToDisplay;
					if (isMarket) {
						Market& market = building.subclass<Market>();

						PUN_CHECK(market.lastUIResourceTargets.size() < 1000);
						
						if (_justOpenedDescriptionUI) {
							market.lastUIResourceTargets = market.GetMarketTargets();
						}
						uiResourceTargetsToDisplay = market.lastUIResourceTargets;

						//uiResourceTargetsToDisplay = market.GetMarketTargets();
					}
					
					auto tryAddManageStorageElement_UnderExpansion = [&](ResourceEnum resourceEnum, bool isShowing, bool isSection, bool shouldRemoveFromList = true)
					{
						// Note: isShowing is needed since we should do RemoveIf even if we aren't showing the resource row 
						if (isShowing) {
							bool isAllowed = storage.ResourceAllowed(resourceEnum);
							ECheckBoxState checkState = (isAllowed ? ECheckBoxState::Checked : ECheckBoxState::Unchecked);

							int32 showTarget = isMarket && building.subclass<StorageBase>().ResourceAllowed(resourceEnum);
							int32 target = showTarget ? uiResourceTargetsToDisplay[static_cast<int>(resourceEnum)] : -1;
							
							manageStorageBox->AddManageStorageElement(resourceEnum, FText(), objectId, checkState, isSection, _justOpenedDescriptionUI, showTarget, target);
						}
						if (shouldRemoveFromList) {
							CppUtils::RemoveIf(resourceEnums, [&](ResourceInfo resourceInfo) { return resourceInfo.resourceEnum == resourceEnum; });
						}
					};


					// Special Case: Granary
					if (storage.isEnum(CardEnum::Granary))
					{
						for (ResourceEnum resourceEnum : StaticData::FoodEnums) {
							tryAddManageStorageElement_UnderExpansion(resourceEnum, true, true, false);
						}
						tryAddManageStorageElement_UnderExpansion(ResourceEnum::Flour, true, true, false);
					}
					// Non-Granary
					else
					{
						// Food
						{
							int32 foodAllowCount = 0;
							for (ResourceEnum resourceEnum : StaticData::FoodEnums) {
								if (storage.ResourceAllowed(resourceEnum)) {
									foodAllowCount++;
								}
							}
							ECheckBoxState checkState;
							if (foodAllowCount == StaticData::FoodEnumCount) {
								checkState = ECheckBoxState::Checked;
							}
							else if (foodAllowCount > 0) {
								checkState = ECheckBoxState::Undetermined;
							}
							else {
								checkState = ECheckBoxState::Unchecked;
							}

							int32 target = 0;
							if (isMarket) {
								target = building.subclass<Market>().GetFoodTarget();
							}

							auto element = manageStorageBox->AddManageStorageElement(ResourceEnum::Food, LOCTEXT("Food", "Food"), objectId, checkState, true, _justOpenedDescriptionUI, isMarket, target);
							bool expanded = element->HasDelayOverride() ? element->expandedOverride : storage.expandedFood;
							for (ResourceEnum resourceEnum : StaticData::FoodEnums) {
								tryAddManageStorageElement_UnderExpansion(resourceEnum, expanded, false);
							}
						}

						// Luxury
						{
							int32 luxuryAllowCount = 0;
							for (int32 i = 1; i < TierToLuxuryEnums.size(); i++) {
								for (ResourceEnum resourceEnum : TierToLuxuryEnums[i]) {
									if (storage.ResourceAllowed(resourceEnum)) {
										luxuryAllowCount++;
									}
								}
							}
							ECheckBoxState checkState;
							if (luxuryAllowCount == LuxuryResourceCount()) {
								checkState = ECheckBoxState::Checked;
							}
							else if (luxuryAllowCount > 0) {
								checkState = ECheckBoxState::Undetermined;
							}
							else {
								checkState = ECheckBoxState::Unchecked;
							}

							auto element = manageStorageBox->AddManageStorageElement(ResourceEnum::Luxury, LOCTEXT("Luxury", "Luxury"), objectId, checkState, true, _justOpenedDescriptionUI, false);
							bool expanded = element->HasDelayOverride() ? element->expandedOverride : storage.expandedLuxury;
							for (int32 i = 1; i < TierToLuxuryEnums.size(); i++) {
								for (ResourceEnum resourceEnum : TierToLuxuryEnums[i]) {
									tryAddManageStorageElement_UnderExpansion(resourceEnum, expanded, false);
								}
							}
						}

						// Other resources
						if (building.isEnum(CardEnum::Market))
						{
							for (ResourceEnum resourceEnum : FuelEnums) {
								tryAddManageStorageElement_UnderExpansion(resourceEnum, true, true, false);
							}
							for (ResourceEnum resourceEnum : MedicineEnums) {
								tryAddManageStorageElement_UnderExpansion(resourceEnum, true, true, false);
							}
							for (ResourceEnum resourceEnum : ToolsEnums) {
								tryAddManageStorageElement_UnderExpansion(resourceEnum, true, true, false);
							}
						}
						else
						{
							for (ResourceInfo resourceInfo : resourceEnums)
							{
								tryAddManageStorageElement_UnderExpansion(resourceInfo.resourceEnum, true, true, false);
							}
						}

					}

					manageStorageBox->AfterAdd();
				}



				/*
				 * Dropdown
				 *   Comes just before "production (per season)
				 */
				if (building.hasWorkModes() &&
					showWhenOwnedByCurrentPlayer)
				{
					focusBox->AddDropdown(
						building.buildingId(),
						building.workModeNames(),
						building.workMode().name,
						[](int32 objectId, FString sItem, IGameUIDataSource* dataSource, IGameNetworkInterface* networkInterface, int32 dropdownIndex, int32 optionInt)
					{
						auto command = make_shared<FChangeWorkMode>();
						command->buildingId = objectId;
						command->enumInt = dataSource->simulation().building(objectId).workModeIntFromString(sItem);
						networkInterface->SendNetworkCommand(command);
					});
					if (!building.workMode().description.IsEmpty()) {
						focusBox->AddWGT_PunRichText(UIEnum::WGT_ObjectFocus_FlavorText,
							building.workMode().description
						);
					}
				}
				


				/*
				 *  Production/Consumption
				 */
				bool hasInput1 = building.hasInput1();
				bool hasInput2 = building.hasInput2();
				
				if (building.isConstructed())
				{
					Indent(10);

					PUN_DEBUG_EXPR(int32 profitInternal = 0);

					const FText perSeasonText = LOCTEXT("Production (per season):", "Production (per season):");
					const FText perYearText = LOCTEXT("Production (per year):", "Production (per year):");
					const FText noneText = LOCTEXT("None", "None");

					// Production stats
					if (IsProducer(building.buildingEnum()) ||
						building.buildingEnum() == CardEnum::Forester)
					{
						focusBox->AddLineSpacer();
						focusBox->AddWGT_TextRow_Resource(UIEnum::WGT_ObjectFocus_TextRow, 
							perSeasonText, 
							TEXT_NUM(building.seasonalProduction()), 
							building.product()
						);

						PUN_DEBUG_EXPR(profitInternal += (building.seasonalProduction() * GetResourceInfo(building.product()).basePrice));
					}

					if (IsRanch(building.buildingEnum()) ||
						building.buildingEnum() == CardEnum::Farm ||
						building.buildingEnum() == CardEnum::HuntingLodge ||
						building.buildingEnum() == CardEnum::FruitGatherer)
					{
						std::vector<ResourcePair> pairs = building.seasonalProductionPairs();

						if (pairs.size() == 0) 
						{
							if (building.isEnum(CardEnum::Farm)) {
								focusBox->AddWGT_TextRow(UIEnum::WGT_ObjectFocus_TextRow, perYearText, noneText);
							} else {
								focusBox->AddWGT_TextRow(UIEnum::WGT_ObjectFocus_TextRow, perSeasonText, noneText);
							}
						}
						else if (pairs.size() == 1) 
						{
							if (building.isEnum(CardEnum::Farm)) {
								focusBox->AddWGT_TextRow_Resource(UIEnum::WGT_ObjectFocus_TextRow,
									perYearText, 
									TEXT_NUM(pairs[0].count), 
									pairs[0].resourceEnum
								);
							} else {
								focusBox->AddWGT_TextRow_Resource(UIEnum::WGT_ObjectFocus_TextRow,
									perSeasonText, 
									TEXT_NUM(pairs[0].count), 
									pairs[0].resourceEnum
								);
							}
						}
						else {
							focusBox->AddRichText(perSeasonText);
							for (ResourcePair pair : pairs) {
								focusBox->AddIconPair(INVTEXT(" "), pair.resourceEnum, TEXT_NUM(pair.count));
							}
						}

						PUN_DEBUG_EXPR(
							if (building.product() != ResourceEnum::None) {
								profitInternal += (building.seasonalProduction() * GetResourceInfo(building.product()).basePrice);
							}
						);
					}


					const FText consumptionSeasonText = LOCTEXT("Consumption (season):", "Consumption(season):");
					const FText depletionSeasonText = LOCTEXT("Depletion (per season):", "Depletion(per season):");

					// Consumption stat
					if (IsConsumerWorkplace(buildingEnum) && !IsPowerPlant(buildingEnum))
					{
						if (hasInput1 && hasInput2)
						{
							focusBox->AddRichText(TEXT_TAG("<GrayBold>", consumptionSeasonText), 
								TEXT_NUM(building.seasonalConsumption1()), building.input1(), FText(),
								TEXT_NUM(building.seasonalConsumption2()), building.input2()
							);

							PUN_DEBUG_EXPR(profitInternal -= (building.seasonalConsumption1() * GetResourceInfo(building.input1()).basePrice));
							PUN_DEBUG_EXPR(profitInternal -= (building.seasonalConsumption2() * GetResourceInfo(building.input2()).basePrice));
							
						}
						else if (hasInput1)
						{
							focusBox->AddRichText(TEXT_TAG("<GrayBold>", consumptionSeasonText), 
								TEXT_NUM(building.seasonalConsumption1()), building.input1()
							);
						}
					}
					// Mines consume deposits
					if (IsMountainMine(building.buildingEnum())) {
						focusBox->AddRichText(TEXT_TAG("<GrayBold>", depletionSeasonText),
							TEXT_NUM(building.seasonalConsumption1()), building.product()
						);
					}

#if !UE_BUILD_SHIPPING
					if (PunSettings::IsOn("DebugFocusUI")) {
						PUN_DEBUG_EXPR(focusBox->AddRichText(INVTEXT("<Yellow>PROFIT:</>"), TEXT_NUM(profitInternal)));
					}
#endif

					ResetIndent();
				}
				//! End Production/Consumption

				

				///*
				// * Dropdown OLD SPOT
				// */
				//if (building.hasWorkModes() &&
				//	showWhenOwnedByCurrentPlayer)
				//{
				//	focusBox->AddDropdown(
				//		building.buildingId(),
				//		building.workModeNames(),
				//		building.workMode().name,
				//		[](int32 objectId, FString sItem, IGameUIDataSource* dataSource, IGameNetworkInterface* networkInterface, int32 dropdownIndex)
				//	{
				//		auto command = make_shared<FChangeWorkMode>();
				//		command->buildingId = objectId;
				//		command->enumInt = dataSource->simulation().building(objectId).workModeIntFromString(sItem);
				//		networkInterface->SendNetworkCommand(command);
				//	});
				//	if (!building.workMode().description.IsEmpty()) {
				//		focusBox->AddWGT_PunRichText(UIEnum::WGT_ObjectFocus_FlavorText,
				//			building.workMode().description
				//		);
				//	}
				//}

				// Forester
				if (building.isEnum(CardEnum::Forester) &&
					showWhenOwnedByCurrentPlayer)
				{
					Forester& forester = building.subclass<Forester>();
					
					auto addDropdown = [&](TArray<FText> options, FText defaultOption, int32 dropdownIndex)
					{
						focusBox->AddDropdown(
							building.buildingId(),
							options,
							defaultOption,
							[options](int32 objectId, FString sItem, IGameUIDataSource* dataSource, IGameNetworkInterface* networkInterface, int32 dropdownIndex, int32 optionInt)
						{
							auto FindCutTreeEnumFromName = [&](FString name) {
								for (int32 i = 0; i < options.Num(); i++) {
									if (name == options[i].ToString()) {
										return i;
									}
								}
								return 0;
							};
							
							auto command = make_shared<FChangeWorkMode>();
							command->buildingId = objectId;
							command->intVar1 = dropdownIndex;
							command->intVar2 = FindCutTreeEnumFromName(sItem);
							networkInterface->SendNetworkCommand(command);
						}, dropdownIndex);
					};

					TArray<FText> cutOptions {
						LOCTEXT("Cut Any Trees", "Cut Any Trees"),
						LOCTEXT("Cut Fruit Trees Only", "Cut Fruit Trees Only"),
						LOCTEXT("Cut Non-fruit Trees Only", "Cut Non-fruit Trees Only"),
					};
					TArray<FText> plantOptions {
						LOCTEXT("Plant Any Trees", "Plant Any Trees"),
						LOCTEXT("Prioritize Planting Fruit Trees", "Prioritize Planting Fruit Trees"),
						LOCTEXT("Prioritize Planting Non-fruit Trees", "Prioritize Planting Non-fruit Trees"),
					};

					FText workModeName = building.workMode().name;
					
					if (workModeName.IdenticalTo(CutAndPlantText) ||
						workModeName.IdenticalTo(PrioritizePlantText)) {
						addDropdown(plantOptions, plantOptions[static_cast<int>(forester.plantingEnum)], 0);
					}
					if (workModeName.IdenticalTo(CutAndPlantText) ||
						workModeName.IdenticalTo(PrioritizeCutText))  {
						addDropdown(cutOptions, cutOptions[static_cast<int>(forester.cuttingEnum)], 1);
					}
					focusBox->AddSpacer();
				}


				/*
				 * Work Status 2
				 */
				if (building.isConstructed()) 
				{

					// Work Reserved/Done
					if (IsProducer(building.buildingEnum()))
					{
#if !UE_BUILD_SHIPPING
						if (PunSettings::IsOn("DebugFocusUI")) {
							focusBox->AddRichText("-- workManSecPerBatch100: " + to_string(building.workManSecPerBatch100()));
							focusBox->AddRichText("-- workRevenuePerSec100_perMan_: " + to_string(building.workRevenuePerSec100_perMan_()));
							focusBox->AddRichText("-- batchCost: " + to_string(building.baseBatchCost()));
							focusBox->AddRichText("-- batchProfit: " + to_string(building.batchProfit()));
						}
#endif

						/*
						 * Work modes
						 */

						focusBox->AddWGT_TextRow(UIEnum::WGT_ObjectFocus_TextRow,
							LOCTEXT("Work done", "Work done"),
							TEXT_PERCENT(building.workPercent())
						);
						
						focusBox->AddSpacer();
						//focusBox->AddSpacer();

						//Indent(50);
						//focusBox->AddWGT_PunText(UIEnum::WGT_ObjectFocus_Subheader,
						//	LOCTEXT("Production batch:", "Production batch:")
						//);
						//ResetIndent();
						
						focusBox->AddProductionChain(
							{ building.input1(), building.inputPerBatch(building.input1()) },
							{ building.input2(), building.inputPerBatch(building.input2()) },
							{ building.product(), building.outputPerBatch() }
						);
						

						if (building.workPercent() > 0) {
							//ss << "(get " << building.outputPerBatch() << " " << ResourceName(building.product()) << ")\n";
						}
						else if (building.needInput1() ||
								 building.needInput2())
						{
							// Special case IsMountainMine
							if (IsMountainMine(building.buildingEnum()))
							{
							}
							else
							{
								bool needInput1 = building.needInput1();
								bool needInput2 = building.needInput2();
								
								ADDTEXT_LOCTEXT("Requires ", "Requires ");
								if (needInput1) ADDTEXT_(INVTEXT("{0} {1}"), TEXT_NUM(building.inputPerBatch(building.input1())), ResourceNameT(building.input1()));
								if (needInput1 && needInput2) ADDTEXT_INV_(", ");
								if (needInput2) ADDTEXT_(INVTEXT("{0} {1}"), TEXT_NUM(building.inputPerBatch(building.input2())), ResourceNameT(building.input2()));

								focusBox->AddSpacer();
								focusBox->AddSpacer();
								focusBox->AddRichText(TEXT_TAG("<Orange>", JOINTEXT(args)));
							}
						}
					}
					else if (IsSpecialProducer(building.buildingEnum()))
					{
#if !UE_BUILD_SHIPPING
						if (PunSettings::IsOn("DebugFocusUI")) {
							focusBox->AddRichText("-- workManSecPerBatch100: " + to_string(building.workManSecPerBatch100()));
							focusBox->AddRichText("-- workRevenuePerSec100_perMan_: " + to_string(building.workRevenuePerSec100_perMan_()));
							if (building.hasInput1()) {
								focusBox->AddRichText("-- baseInputValue: " + to_string(building.baseInputCost(building.input1())));
							}

							switch (building.buildingEnum())
							{
							case CardEnum::BarrackArcher:
							case CardEnum::BarrackClubman:
							case CardEnum::Mint:
							case CardEnum::ResearchLab:
							case CardEnum::RegionShrine:

							case CardEnum::CardMaker:
							case CardEnum::ImmigrationOffice: {
								auto& consumerIndustry = building.subclass<ConsumerIndustrialBuilding>();
								if (building.hasInput1()) {
									focusBox->AddRichText("-- baseInputValue: " + to_string(consumerIndustry.baseInputValue()));
									focusBox->AddRichText("-- baseOutputValue: " + to_string(consumerIndustry.baseOutputValue()));
									focusBox->AddRichText("-- baseProfitValue: " + to_string(consumerIndustry.baseProfitValue()));
									focusBox->AddRichText("-- workManSecPerBatch100(calc): " + to_string(consumerIndustry.baseProfitValue() * 100 * 100 / consumerIndustry.buildingInfo().workRevenuePerSec100_perMan()));
								}
								break;
							}
							default:
								break;
							}
						}
#endif
						
						UTexture2D* productTexture = nullptr;
						std::string productStr;

						auto setProduct = [&](UTexture2D* productTextureIn, std::string productStrIn) {
							productTexture = productTextureIn;
							productStr = productStrIn;
						};
						
						switch (buildingEnum)
						{
							case CardEnum::Mint: setProduct(assetLoader->CoinIcon, to_string(building.outputPerBatch()));  break;
							case CardEnum::ResearchLab: setProduct(assetLoader->ScienceIcon, to_string(building.outputPerBatch()));  break;
							case CardEnum::RegionShrine: setProduct(assetLoader->ScienceIcon, to_string(building.outputPerBatch()));  break;
							
							case CardEnum::BarrackArcher:
							case CardEnum::BarrackSwordman:
									setProduct(assetLoader->InfluenceIcon, to_string(building.outputPerBatch()));  break;
							
							case CardEnum::CardMaker: setProduct(assetLoader->CardBack, "1 card");  break;
							case CardEnum::ImmigrationOffice: break;
							default:
								UE_DEBUG_BREAK();
								break;
						}

						if (productTexture)
						{
							focusBox->AddSpacer();
							focusBox->AddWGT_PunText(UIEnum::WGT_ObjectFocus_Subheader, 
								LOCTEXT("Production batch:", "Production batch:")
							);
							focusBox->AddProductionChain(
								{ building.input1(), building.inputPerBatch(building.input1()) },
								{ building.input2(), building.inputPerBatch(building.input2()) },
								{}, productTexture, productStr
							);
						}

						focusBox->AddWGT_TextRow(UIEnum::WGT_ObjectFocus_TextRow, 
							LOCTEXT("Work done", "Work done"), 
							TEXT_PERCENT(building.workPercent())
						);
					}
				}
				else if (building.shouldDisplayConstructionUI())
				{	
					focusBox->AddWGT_TextRow(UIEnum::WGT_ObjectFocus_TextRow, 
						LOCTEXT("Construct", "Construct"), 
						TEXT_PERCENT(building.constructionPercent())
					);

					// Quick Build
					if (showWhenOwnedByCurrentPlayer &&
						(sim.IsResearched(playerId(), TechEnum::QuickBuild) || PunSettings::IsOn("ForceQuickBuild")))
					{
						if (IsRoad(building.buildingEnum()))
						{
							const std::vector<int32>& roadIds = sim.buildingIds(building.townId(), building.buildingEnum());
							int32 quickBuildAllCost = 0;
							for (int32 roadId : roadIds) {
								quickBuildAllCost += sim.building(roadId).GetQuickBuildCost();
							}

							TArray<FText> argsLocal;
							argsLocal.Add(LOCTEXT("Quick Build All", "Quick Build All"));
							ADDTEXT(argsLocal, INVTEXT("\n<img id=\"Coin\"/>{0}"), FText::AsNumber(quickBuildAllCost));

							focusBox->AddSpacer();
							focusBox->AddButton2Lines(JOINTEXT(argsLocal), this, CallbackEnum::QuickBuild, true, false, objectId);
						}
						else
						{
							int32 quickBuildCost = building.GetQuickBuildCost();
							bool canQuickBuild = sim.moneyCap32(playerId()) >= quickBuildCost;

							TArray<FText> argsLocal;
							argsLocal.Add(LOCTEXT("Quick Build <Orange>[Ctrl-B]</>", "Quick Build <Orange>[Ctrl-B]</>"));
							ADDTEXT(argsLocal, INVTEXT("\n<img id=\"Coin\"/>{0}"), TextRed(FText::AsNumber(quickBuildCost), !canQuickBuild));

							focusBox->AddSpacer();
							focusBox->AddButton2Lines(JOINTEXT(argsLocal), this, CallbackEnum::QuickBuild, true, false, objectId);
						}
					}
					
#if !UE_BUILD_SHIPPING
					if (PunSettings::IsOn("DebugFocusUI")) {
						focusBox->AddRichText("-- buildManSecCost100", to_string(building.buildTime_ManSec100()));
					}
#endif
				}


				/*
				 * Set Output Target
				 */
				ResourceEnum product = building.product();
				if (showWhenOwnedByCurrentPlayer &&
					product != ResourceEnum::None)
				{
					auto& townManager = sim.townManager(building.townId());
					if (_justOpenedDescriptionUI) {
						townManager.SetOutputTargetDisplay(product, townManager.GetOutputTarget(product));
					}
					int32 targetDisplay = townManager.GetOutputTargetDisplay(product);
					bool isChecked = (targetDisplay != -1);

					focusBox->AddSpacer();
					auto numberBox = focusBox->AddEditableNumberBox(this, CallbackEnum::EditableNumberSetOutputTarget, building.buildingId(),
						LOCTEXT("set produce target", "set produce target"), targetDisplay, 
						FText::Format(INVTEXT("{0} "), LOCTEXT("produce until", "produce until")),
						isChecked, product
					);
					numberBox->callbackVar1 = static_cast<int32>(product);
					numberBox->callbackVar2 = building.townId();
				}


				
				/*
				 * Inventory
				 */
				std::vector<ResourceHolderInfo>& holderInfos = building.holderInfos();
				if (building.shouldDisplayConstructionUI())
				{
					focusBox->AddLineSpacer();
					focusBox->AddWGT_PunText(UIEnum::WGT_ObjectFocus_Subheader, 
						LOCTEXT("Resources needed:", "Resources needed:")
					);


					std::vector<int32> constructionCosts = building.GetConstructionResourceCost();
					
					for (int32 i = 0; i < ConstructionResourceCount; i++)
					{
						ResourceEnum resourceEnum = ConstructionResources[i];
						ResourceHolderInfo holderInfo = building.holderInfo(resourceEnum);
						if (holderInfo.isValid()) {

							ADDTEXT_(INVTEXT("{0}/{1}"),
								TEXT_NUM(building.GetResourceCount(holderInfo)),
								TEXT_NUM(constructionCosts[i])
							);
							focusBox->AddIconPair(FText(), resourceEnum, args);
						}
					}

#if !UE_BUILD_SHIPPING
					if (PunSettings::IsOn("DebugFocusUI")) {
						TArray<FText> argsLocal;
						ADDTEXT(argsLocal, INVTEXT("hasNeededConstructionResource: {0}"), building.hasNeededConstructionResource() ? INVTEXT("Yes") : INVTEXT("No"));
						focusBox->AddSpecialRichText(INVTEXT("<Yellow>"), argsLocal);
						ADDTEXT(argsLocal, INVTEXT("NeedConstruct: {0}"), building.NeedConstruct() ? INVTEXT("Yes") : INVTEXT("No"));
						focusBox->AddSpecialRichText(INVTEXT("<Yellow>"), argsLocal);
					}
#endif
				}
				// Inventory
				else if (!holderInfos.empty() && buildingEnum != CardEnum::House)
				{
					focusBox->AddLineSpacer();

					const FText inventoryText = LOCTEXT("Inventory:", "Inventory:");
					
					if (holderInfos.size() > 0) {
						focusBox->AddWGT_PunText(UIEnum::WGT_ObjectFocus_Subheader, inventoryText);

						for (ResourceHolderInfo holderInfo : holderInfos)
						{
#if !UE_BUILD_SHIPPING
							if (PunSettings::IsOn("DebugFocusUI")) {
								ResourceHolder holder = building.resourceSystem().holder(holderInfo);

								int32 target = building.GetResourceTarget(holderInfo);
								int32 pop = holder.reservedPop();
								int32 push = holder.reservedPush();
								if (target != 0 || pop != 0 || push != 0) {
									stringstream sst;
									sst << "-- Target:" << target << ",Pop:" << pop << ",Push:" << push;
									focusBox->AddRichText(sst);
								}
							}
#endif

							int32 resourceCount = building.GetResourceCount(holderInfo);

							// Don't display 0
							if (resourceCount == 0) {
								continue;
							}

							focusBox->AddIconPair(FText(), holderInfo.resourceEnum, TEXT_NUM(resourceCount), false, false, focusBox->indentation);
						}
					}
					else {
						focusBox->AddWGT_PunText(UIEnum::WGT_ObjectFocus_Subheader, inventoryText);
						focusBox->AddRichText(
							TEXT_TAG("<Bold>", LOCTEXT("None", "None"))
						);
					}
				}


				/*
				 * Card slots
				 */
				auto updateCardSlotUI = [&](const std::vector<CardStatus>& cards, int32 maxCards, bool isTownhall)
				{
					if (_objectDescriptionUI->lastCards != cards ||
						_objectDescriptionUI->lastMaxCards != maxCards ||
						_objectDescriptionUI->lastIsTownhall != isTownhall)
					{
						_objectDescriptionUI->lastCards = cards;
						_objectDescriptionUI->lastMaxCards = maxCards;
						_objectDescriptionUI->lastIsTownhall = isTownhall;

						_objectDescriptionUI->CardSlots->ClearChildren();
						
						for (int32 i = 0; i < cards.size(); i++)
						{
							CardEnum cardEnum = cards[i].cardEnum;

							if (cardEnum != CardEnum::None)
							{
								UBuildingPlacementButton* cardButton = AddWidget<UBuildingPlacementButton>(UIEnum::CardMini);

								cardButton->PunInit(CardStatus(cardEnum, 1), i, this, CallbackEnum::SelectBuildingSlotCard, CardHandEnum::CardSlots);
								cardButton->callbackVar1 = building.buildingId();
								cardButton->cardAnimationOrigin = cards[i].lastPosition();
								cardButton->cardAnimationStartTime = cards[i].animationStartTime100 / 100.0f; // Animation start time is when FUseCard arrived

								//SetChildHUD(cardButton);

								cardButton->RefreshBuildingIcon(dataSource()->assetLoader());
								cardButton->SetCardStatus(CardHandEnum::CardSlots, false, false, IsRareCard(cardEnum));

								//cardButton->SellButton->SetVisibility(ESlateVisibility::Collapsed);

								_objectDescriptionUI->CardSlots->AddChild(cardButton);
							}
						}

						// card slots for the rest
						for (int32 i = cards.size(); i < maxCards; i++)
						{
							auto cardSlot = AddWidget<UCardSlot>(UIEnum::CardSlot);

							cardSlot->CardSlot->GetDynamicMaterial()->SetTextureParameterValue("CardSlot", isTownhall ? assetLoader->CardSlotRound : assetLoader->CardSlotBevel);
							cardSlot->CardSlot->GetDynamicMaterial()->SetScalarParameterValue("IsBuildingSlotCard", !isTownhall);
							
							_objectDescriptionUI->CardSlots->AddChild(cardSlot);
						}
					}
					_objectDescriptionUI->CardSlots->SetVisibility(ESlateVisibility::Visible);
				};

				if (building.playerId() == playerId())
				{
					if (building.isEnum(CardEnum::Townhall)) {
						auto& townManage = sim.townManager(building.townId());
						updateCardSlotUI(townManage.cardsInTownhall(), townManage.maxTownhallCards(), true);
					}
					else {
						updateCardSlotUI(building.slotCards(), building.maxCardSlots(), false);
					}
				}


				/*
				 * Town/Global Bonuses
				 */
				if (building.isEnum(CardEnum::Townhall) && showWhenOwnedByCurrentPlayer)
				{
					auto setBonusIcon = [&](UTownBonusIcon* bonusIcon, CardEnum bonusEnum)
					{
						bonusIcon->SetVisibility(ESlateVisibility::Visible);
						SetChildHUD(bonusIcon);
						bonusIcon->BuildingIcon->GetDynamicMaterial()->SetTextureParameterValue("ColorTexture", assetLoader->GetCardIcon(playerFactionEnum(), bonusEnum));
						
						const BldInfo& info = GetBuildingInfo(bonusEnum);
						UToolTipWidgetBase* tooltip = UPunBoxWidget::AddToolTip(bonusIcon, bonusIcon);

						auto tooltipBox = tooltip->TooltipPunBoxWidget;
						tooltipBox->AfterAdd();

						tooltipBox->AddRichText(TEXT_TAG("<TipHeader>", info.GetName()));
						tooltipBox->AddSpacer();
						tooltipBox->AddRichText(info.GetDescription());
					};

					auto fillBonusIcons = [&](UWrapBox* bonusSlots, UVerticalBox* bonusBox, const std::vector<CardEnum>& bonuses) {
						int32 bonusSlotCount = bonusSlots->GetChildrenCount();
						if (bonuses.size() > 0) {
							for (int32 i = 0; i < bonusSlotCount; i++) {
								auto bonusIcon = bonusSlots->GetChildAt(i);
								if (i < bonuses.size()) {
									setBonusIcon(CastChecked<UTownBonusIcon>(bonusIcon), bonuses[i]);
								} else {
									bonusIcon->SetVisibility(ESlateVisibility::Collapsed);
								}
							}
							bonusBox->SetVisibility(ESlateVisibility::Visible);
						}
					};

					const std::vector<CardEnum>& townBonuses = sim.townManager(building.townId()).townBonuses();
					fillBonusIcons(_objectDescriptionUI->TownBonusSlots, _objectDescriptionUI->TownBonusBox, townBonuses);

					const std::vector<CardEnum>& globalBonuses = sim.playerOwned(building.playerId()).globalBonuses();
					fillBonusIcons(_objectDescriptionUI->GlobalBonusSlots, _objectDescriptionUI->GlobalBonusBox, globalBonuses);
					
				}
				

				/*
				 * Selection Meshes (Related)
				 */
				std::vector<int>& occupants = building.occupants();
				for (int occupantId : occupants) {
					WorldAtom2 atom = dataSource()->unitDataSource().actualAtomLocation(occupantId);
					SpawnSelectionMesh(assetLoader->SelectionMaterialYellow, dataSource()->DisplayLocation(atom) + FVector(0, 0, 30));
				}

				// For road, we also spawn SelectionMaterialYellow for workReservers
				if (IsRoad(building.buildingEnum())) {
					std::vector<int32> workReserverIds = building.workReservers();
					for (int32 workReserverId : workReserverIds) {
						WorldAtom2 atom = dataSource()->unitDataSource().actualAtomLocation(workReserverId);
						SpawnSelectionMesh(assetLoader->SelectionMaterialYellow, dataSource()->DisplayLocation(atom) + FVector(0, 0, 30));
					}
				}

				// DeliveryArrow
				//   Others
				const std::vector<int32>& deliverySourceIds = building.deliverySourceIds();
				for (int32 sourceId : deliverySourceIds)
				{
					Building& buildingScope = sim.buildingChecked(sourceId);
					FVector displayLocationScope = dataSource()->DisplayLocationTrueCenter(buildingScope);
					dataSource()->ShowDeliveryArrow(displayLocationScope, displayLocation);
				}
				//   Self
				int32 deliveryTargetId = building.deliveryTargetId();
				if (deliveryTargetId != -1)
				{
					Building& buildingScope = sim.buildingChecked(deliveryTargetId);
					FVector displayLocationScope = dataSource()->DisplayLocationTrueCenter(buildingScope);
					dataSource()->ShowDeliveryArrow(displayLocation, displayLocationScope);
				}

				if (showWhenOwnedByCurrentPlayer)
				{
					// Special case: Logistics Office
					if (building.isEnum(CardEnum::ShippingDepot))
					{
						vector<int32> storageIds = sim.GetBuildingsWithinRadiusMultiple(building.centerTile(), ShippingDepot::Radius, building.townId(), StorageEnums);
						for (int32 storageId : storageIds)
						{
							Building& buildingScope = sim.buildingChecked(storageId);
							FVector displayLocationScope = dataSource()->DisplayLocationTrueCenter(buildingScope);
							dataSource()->ShowDeliveryArrow(displayLocationScope, displayLocation, true);
						}
					}

					// Special case: House
					if (building.isEnum(CardEnum::House))
					{
						// show arrows to every nearby houses
						House& house = building.subclass<House>();
						std::vector<int32>& occupantIds = house.occupants();
						for (int32 occupantId : occupantIds)
						{
							Building* workplace = sim.unitAI(occupantId).workplace();
							if (workplace && !workplace->isEnum(CardEnum::Townhall)) {
								FVector displayLocationScope = dataSource()->DisplayLocationTrueCenter(*workplace);
								dataSource()->ShowDeliveryArrow(displayLocation, displayLocationScope, true, true);
							}
						}
					}
					else if (building.isEnum(CardEnum::MinorCity)) {
						// Don't do anything for minor City
					}
					// Special case: Job Buildings
					else if (building.isConstructed() && 
						building.occupantCount() > 0 &&
						!IsRoad(buildingEnum))
					{
						// show arrows to every nearby houses
						std::vector<int32>& occupantIds = building.occupants();
						for (int32 occupantId : occupantIds)
						{
							int32 houseId = sim.unitAI(occupantId).houseId();
							if (Building* house = sim.buildingPtr(houseId))
							{
								check(IsHouse(house->buildingEnum()));
								dataSource()->ShowDeliveryArrow(displayLocation, dataSource()->DisplayLocationTrueCenter(*house), true, true);
							}
						}
					}
				}

				// Overlays
				switch (building.buildingEnum())
				{
					case CardEnum::Farm: overlayType = OverlayType::Farm; break;
					case CardEnum::FruitGatherer: overlayType = OverlayType::Gatherer; break;
					case CardEnum::Fisher: overlayType = OverlayType::Fish; break;
					case CardEnum::HuntingLodge: overlayType = OverlayType::Hunter; break;
					case CardEnum::Forester: overlayType = OverlayType::Forester; break;
					case CardEnum::Windmill: overlayType = OverlayType::Windmill; break;

					case CardEnum::IrrigationReservoir: overlayType = OverlayType::IrrigationReservoir; break;
					case CardEnum::Market: overlayType = OverlayType::Market; break;
					case CardEnum::ShippingDepot: overlayType = OverlayType::ShippingDepot; break;
					
					case CardEnum::Beekeeper: overlayType = OverlayType::Beekeeper; break;
						
					//case BuildingEnum::ConstructionOffice: overlayType = OverlayType::ConstructionOffice; break;
					case CardEnum::IndustrialistsGuild: overlayType = OverlayType::Industrialist; break;
					case CardEnum::ConsultingFirm: overlayType = OverlayType::Consulting; break;
						
					case CardEnum::Library: overlayType = OverlayType::Library; break;
					case CardEnum::School: overlayType = OverlayType::School; break;
					case CardEnum::Bank: overlayType = OverlayType::Bank; break;

					case CardEnum::HaulingServices: overlayType = OverlayType::HaulingServices; break;
					case CardEnum::Granary: overlayType = OverlayType::Granary; break;

					//case CardEnum::Museum: overlayType = OverlayType::Museum; break;
					//case CardEnum::Zoo: overlayType = OverlayType::Zoo; break;
					case CardEnum::Theatre: overlayType = OverlayType::Theatre; break;
					case CardEnum::Tavern: overlayType = OverlayType::Tavern; break;
				}

				//// Tool Checkbox
				//if (IsProducer(building.buildingEnum()) && building.isConstructed()) {
				//	_objectDescriptionUI->ToolCheckBox->SetVisibility(ESlateVisibility::Visible);

				//	int toolBonus = building.isEnum(BuildingEnum::Quarry) || building.isEnum(BuildingEnum::Mine) ? 50 : 20;
				//	_objectDescriptionUI->ToolCheckBoxText->SetText(FText::FromString(FString((" use tools (+" + to_string(toolBonus) + "% productivity)").c_str())));
				//	_objectDescriptionUI->ToolCheckBox->SetCheckedState(building.useTools ? ECheckBoxState::Checked : ECheckBoxState::Unchecked);
				//}
				//else {
					//_objectDescriptionUI->ToolCheckBox->SetVisibility(ESlateVisibility::Collapsed);
				//}

				bool isUpgraderPlayer;
				if (IsForeignOnlyBuilding(buildingEnum)) {
					isUpgraderPlayer = building.foreignBuilderId() == playerId();
				} else {
					isUpgraderPlayer = building.playerId() == playerId();
				}

				// Special case:
				if (IsAncientWonderCardEnum(buildingEnum) && 
					sim.IsResearched(playerId(), TechEnum::Museum)) 
				{
					isUpgraderPlayer = true;
				}
				
				if (isUpgraderPlayer)
				{
					const FText upgradeEraRequiresText = LOCTEXT("Upgrade Requires", "\n<Red>Requires {0}</>");
					
					//! Upgrade Button Townhall
					if (building.isEnum(CardEnum::Townhall))
					{
						TownHall& townhall = building.subclass<TownHall>();
						if (townhall.townhallLvl < TownHall::GetMaxUpgradeLvl()) 
						{
							int32 upgradeMoney = townhall.GetUpgradeMoney();
							FText moneyText = FText::AsNumber(upgradeMoney);
							if (globalResourceSys.moneyCap32() < upgradeMoney) {
								moneyText = TEXT_TAG("<Red>", moneyText);
							}

							bool showExclamation = sim.parameters(playerId())->NeedTownhallUpgradeNoticed;
							bool showEnabled = true;

							CLEARTEXT_();
							ADDTEXT_(
								LOCTEXT("TownhallUpgradeButton", "Upgrade Townhall to lvl {0}"),
								FText::AsNumber(townhall.townhallLvl + 1)
							);

							auto unlockSys = sim.unlockSystem(playerId());
							int32 nextTownhallLvl = townhall.townhallLvl + 1;
							if (nextTownhallLvl < unlockSys->GetEra() + 2) { // townhallLvl 1 ... Era 1, can upgrade
								ADDTEXT_(INVTEXT("\n<img id=\"Coin\"/>{0}"), moneyText);
							} else {
								ADDTEXT_(upgradeEraRequiresText, unlockSys->GetEraText(unlockSys->GetEra() + 1));
								showEnabled = false;
							}

							UPunButton* button = focusBox->AddButton2Lines(FText::Join(FText(), args), this, CallbackEnum::UpgradeBuilding, showEnabled, showExclamation, objectId, 0);
							args.Empty();

							ADDTEXT_LOCTEXT("Upgrade Rewards", "Upgrade Rewards");
							ADDTEXT_INV_("<line>");
							ADDTEXT__(GetTownhallLvlToUpgradeBonusText(townhall.townhallLvl + 1));

							//int32 capitalLvl = simulation.GetTownLvl(townhall.playerId());
							//if (townhall.townhallLvl + 1 < capitalLvl) {
							//	ADDTEXT_INV_("<space>");
							//	ADDTEXT_TAG_("<Red>", LOCTEXT("TownhallUpgrade_NeedCapitalUpgrade", "Require higher Capital's Townhall Level."));
							//}
							//else 
							if (globalResourceSys.moneyCap32() < upgradeMoney) {
								ADDTEXT_INV_("<space>");
								ADDTEXT_TAG_("<Red>", LOCTEXT("Not enough money to upgrade.", "Not enough money to upgrade."));
							}
							AddToolTip(button, args);
						}
					}
					//! Upgrade Button Others
					else
					{
						CLEARTEXT_();

						auto unlockSys = sim.unlockSystem(playerId());
						
						
						auto setUpgradeButton = [&](BuildingUpgrade upgrade, int32 upgradeIndex)
						{

							ResourcePair resourceNeededPair = upgrade.currentUpgradeResourceNeeded();

							// Special case: Ancient Wonders
							if (IsAncientWonderCardEnum(building.buildingEnum())) 
							{
								resourceNeededPair.count = resourceNeededPair.count * building.subclass<ProvinceRuin>().GetDigDistanceFactor(playerId()) / 100;
								
								if (sim.IsTileOwnedByForeignPlayer(playerId(), building.centerTile())) {
									ADDTEXT_LOCTEXT("Steal Artifact", "Steal Artifact");
									resourceNeededPair.count = resourceNeededPair.count * 3; // Stealing incur x3 cost
								} else {
									ADDTEXT_(INVTEXT("{0}"), building.GetUpgradeDisplayName(upgradeIndex));
								}
							}
							else {
								ADDTEXT_(LOCTEXT("Upgrade {0}", "Upgrade {0}"), building.GetUpgradeDisplayName(upgradeIndex));
							}

							
							ResourceEnum resourceEnum = resourceNeededPair.resourceEnum;
							bool showEnabled = true;
							
							if (upgrade.isUpgraded) {
								ADDTEXT_INV_("\n");
								ADDTEXT_LOCTEXT("Done", "Done");
								showEnabled = false;
							}
							else 
							{
								auto showResourceText = [&](FString resourceTagString)
								{
									bool isRed = resourceSys.resourceCount(resourceEnum) < resourceNeededPair.count;
									ADDTEXT_(INVTEXT("\n<img id=\"{0}\"/>{1}"), FText::FromString(resourceTagString), TextRed(TEXT_NUM(resourceNeededPair.count), isRed));
								};
								
								bool isEraUpgradedToFull = upgrade.isEraUpgrade() && !building.IsEraUpgradable();

								if (isEraUpgradedToFull) {
									ADDTEXT_(upgradeEraRequiresText, unlockSys->GetEraText(building.GetUpgradeEraLevel() + 1));
									showEnabled = false;
								}
								else if (resourceEnum == ResourceEnum::Stone) { showResourceText("Stone"); }
								else if (resourceEnum == ResourceEnum::Wood) { showResourceText("Wood"); }
								else if (resourceEnum == ResourceEnum::Iron) { showResourceText("IronBar"); }
								else if (resourceEnum == ResourceEnum::SteelTools) { showResourceText("SteelTools"); }
								else if (resourceEnum == ResourceEnum::Brick) { showResourceText("Brick"); }
								else if (resourceEnum == ResourceEnum::Paper) { showResourceText("Paper"); }
								else if (resourceEnum == ResourceEnum::Glass) { showResourceText("Glass"); }
								else if (resourceEnum == ResourceEnum::Concrete) { showResourceText("Concrete"); }
								else if (resourceEnum == ResourceEnum::Steel) { showResourceText("SteelBeam"); }
								else if (resourceEnum == ResourceEnum::Influence) {
									int32 influenceNeeded = resourceNeededPair.count;
									FText influenceText = TextRed(TEXT_NUM(influenceNeeded), globalResourceSys.influence() < influenceNeeded);

									ADDTEXT_(INVTEXT("\n<img id=\"Influence\"/>{0}"), influenceText);
								}
								else {
									PUN_CHECK(resourceEnum == ResourceEnum::Money);

									int32 moneyNeeded = resourceNeededPair.count;
									FText moneyText = TextRed(TEXT_NUM(moneyNeeded), globalResourceSys.moneyCap32() < moneyNeeded);
									
									ADDTEXT_(INVTEXT("\n<img id=\"Coin\"/>{0}"), moneyText);
								}
							}

							UPunButton* button = focusBox->AddButton2Lines(JOINTEXT(args), this, CallbackEnum::UpgradeBuilding, showEnabled, false,  objectId, upgradeIndex);

							// Tooltip
							args.Empty();
							ADDTEXT__(upgrade.name);
							ADDTEXT_INV_("<space>");

							if (!upgrade.isUpgraded) {
								const FText costText = LOCTEXT("cost", "cost");
								int32 resourceNeeded = resourceNeededPair.count;
								if (resourceEnum == ResourceEnum::Money) {
									ADDTEXT_(INVTEXT("{0}: <img id=\"Coin\"/>{1}"), costText, TEXT_NUM(resourceNeeded));
								}
								else if (resourceEnum == ResourceEnum::Influence) {
									ADDTEXT_(INVTEXT("{0}: <img id=\"Influence\"/>{1}"), costText, TEXT_NUM(resourceNeeded));
								}
								else {
									check(static_cast<int>(resourceEnum) < ResourceEnumCount);
									ADDTEXT_(INVTEXT("{0}: {1} {2}"), costText, TEXT_NUM(resourceNeeded), ResourceNameT(resourceEnum));
								}
								ADDTEXT_INV_("<space>");
							}
							
							ADDTEXT__(building.GetUpgradeDisplayDescription(upgradeIndex));

							if (!_alreadyDidShiftDownUpgrade) {
								ADDTEXT_INV_("<line><space>");
								ADDTEXT_LOCTEXT("ShiftDownUpgrade", "<Orange>Shift-click</> the button to try upgrading all same type buildings.");
							}
							
							AddToolTip(button, args);
						};

						
						const std::vector<BuildingUpgrade>& upgrades = building.upgrades();
						for (size_t i = 0; i < upgrades.size(); i++) {
							setUpgradeButton(upgrades[i], i);
						}
					}


					/*
					 * Delivery
					 */
					if ((building.product() != ResourceEnum::None && sim.unlockSystem(playerId())->unlockedSetDeliveryTarget) ||
						building.isEnum(CardEnum::ShippingDepot))
					{
						auto button = focusBox->AddButton(LOCTEXT("SetDeliveryTarget_Button", "Set Delivery Target"), nullptr, FText(), this, CallbackEnum::SetDeliveryTarget, true, false, objectId);
						AddToolTip(button, LOCTEXT("SetDeliveryTarget_Tip", "Set the Target Storage/Market where output resources would be delivered"));
						
						if (building.deliveryTargetId() != -1) {
							focusBox->AddButton(LOCTEXT("RemoveDeliveryTarget_Button", "Remove Delivery Target"), nullptr, FText(), this, CallbackEnum::RemoveDeliveryTarget, true, false, objectId);
						}
					}
				}


				//! Drop Down
				if (building.playerId() == playerId()) {
					_objectDescriptionUI->SetDropDown(objectId);
				}
				
			} // Fire

			/*
			 * Selection Mesh (Self)
			 */
			FVector selectionMeshLocation = displayLocation + FVector(0, 0, building.GetBuildingSelectorHeight());
			if (IsBridgeOrTunnel(building.buildingEnum()))
			{
				// Average using atom to get exact middle
				WorldAtom2 min = area.min().worldAtom2();
				WorldAtom2 max = area.max().worldAtom2();
				WorldAtom2 midAtom((min.x + max.x) / 2, (min.y + max.y) / 2);
				selectionMeshLocation = dataSource()->DisplayLocation(midAtom);
				selectionMeshLocation.Z = 30;
			}
			SpawnSelectionMesh(assetLoader->SelectionMaterialGreen, selectionMeshLocation);

			auto showGroundBoxHighlightDecal = [&]()
			{
				auto decal = dataSource()->ShowDecal(area, assetLoader->M_ConstructionHighlightDecal, _groundBoxHighlightDecals, _groundBoxHighlightCount, true);
				auto materialInst = CastChecked<UMaterialInstanceDynamic>(decal->GetDecalMaterial());
				materialInst->SetScalarParameterValue("TilesX", area.sizeX());
				materialInst->SetScalarParameterValue("TilesY", area.sizeY());
			};
			
			// Show Selection Highlight Mesh
			if (building.isEnum(CardEnum::Farm)) {
				// Farm is shown using the BuildingDisplayComponent
			}
			else if (building.isConstructed() && !building.isBurnedRuin()) {
				dataSource()->ShowBuildingMesh(building, 2);
			} else {
				showGroundBoxHighlightDecal();
			}

			focusBox->AfterAdd();
		}

		/*
		 * Unit
		 */
		else if (uiState.objectType == ObjectTypeEnum::Unit) 
		{
			UnitStateAI& unit = dataSource()->GetUnitStateAI(objectId);
			UnitEnum unitEnum = unit.unitEnum();

			// Make sure we don't forget to put in string
			check((int)unit.unitState() < UnitStateName.Num());

			if (unit.isEnum(UnitEnum::Human)) 
			{
				int32 age = unit.age() * PlayerParameters::PeopleAgeToGameYear / Time::TicksPerYear + 3; // cheat 3 years older so no 0 years old ppl
				
				focusBox->AddWGT_ObjectFocus_Title(
					unit.GetUnitNameT(),
					FText::Format(
						LOCTEXT("Human subtitle", "{0}, Age {1}"),
						(unit.isChild()) 
						? (unit.isMale() ? LOCTEXT("Boy", "Boy") : LOCTEXT("Girl", "Girl"))
						: (unit.isMale() ? LOCTEXT("Man", "Man") : LOCTEXT("Woman", "Woman")),
						TEXT_NUM(age)
					)
				);

			}
			else {
				focusBox->AddWGT_ObjectFocus_Title(unit.GetTypeName());
			}
			
#if !UE_BUILD_SHIPPING
			if (PunSettings::IsOn("DebugFocusUI")) {
				TArray<FText> args;
				ADDTEXT_(INVTEXT("[{0}]"), TEXT_NUM(objectId)); // ID
				focusBox->AddRichText(args);
			}
#endif

			if (unit.townId() != -1) 
			{
				if (unit.playerId() != -1) {
					focusBox->AddWGT_TextRow(UIEnum::WGT_ObjectFocus_TextRow, LOCTEXT("Player", "Player"), sim.playerNameT(unit.playerId()), nullptr);
				}
				focusBox->AddWGT_TextRow(UIEnum::WGT_ObjectFocus_TextRow, LOCTEXT("City", "City"), sim.townNameT(unit.townId()), nullptr);
				
				focusBox->AddLineSpacer();
			}
			
#if !UE_BUILD_SHIPPING
			if (PunSettings::IsOn("DebugFocusUI"))
			{
				std::stringstream ss;
				ss << "-- Tile:(" << unit.unitTile().x << "," << unit.unitTile().y << ")";
				focusBox->AddRichText(ss);
				ss << "-- lastBirthTick:(" << unit.lastPregnantTick() << ")";
				focusBox->AddRichText(ss);
				ss << "animation: " << GetUnitAnimationName(unit.animationEnum());
				focusBox->AddRichText(ss);
			}
#endif

			// Show Food/Heat as percent
			Indent(60);
			
			int32 foodPercent = unit.foodActual() * 100 / unit.maxFood();
			int32 heatPercent = unit.heatActual() * 100 / unit.maxHeat();
			focusBox->AddRichText(
				TEXT_TAG("<GrayBold>", LOCTEXT("Food", "Food")), 
				TextNumberColor(TEXT_PERCENT(foodPercent), foodPercent, unit.foodThreshold_Get2Percent(), unit.minWarnFoodPercent())
			);
			focusBox->AddRichText(
				TEXT_TAG("<GrayBold>", LOCTEXT("Heat", "Heat")),
				TextNumberColor(TEXT_PERCENT(heatPercent), heatPercent, unit.heatGetThresholdPercent(), unit.minWarnHeatPercent())
			);
			focusBox->AddRichText(
				TEXT_TAG("<GrayBold>", LOCTEXT("Health", "Health")),
				TextNumberColor(TEXT_PERCENT(unit.hp()), unit.hp(), 60, 40)
			);
			focusBox->AddLineSpacer();

			ResetIndent();
			
			/*
			 * Happiness
			 */
			if (unit.isEnum(UnitEnum::Human)) 
			{
				auto& human = unit.subclass<HumanStateAI>();
			
				int32 happinessOverall = human.happinessOverall();

				Indent(50);
				
				focusBox->AddRichText(
					FText::Format(INVTEXT("{0} {1}"), LOCTEXT("Happiness", "Happiness"), GetHappinessFace(happinessOverall)),
					ColorHappinessText(happinessOverall, TEXT_PERCENT(happinessOverall))
				);
				focusBox->AddSpacer();

				Indent(55);

				for (size_t i = 0; i < HappinessEnumCount; i++) 
				{
					int32 happiness = human.GetHappinessByType(static_cast<HappinessEnum>(i));

					auto widget = focusBox->AddRichText(
						FText::Format(
							INVTEXT("  {0} {1}"),
							ColorHappinessText(happiness, FText::Format(INVTEXT("{0}%"), TEXT_NUM(happiness))),
							HappinessEnumName[i]
						)
					);

					AddToolTip(widget, GetHappinessEnumTip(static_cast<HappinessEnum>(i)));
				}

				ResetIndent();

				focusBox->AddLineSpacer();
			}


			// Dying
			auto dyingMessage = [&](FText dyingDescription) {
				focusBox->AddWGT_PunRichText(UIEnum::WGT_ObjectFocus_FlavorText,
					TEXT_TAG("<Red>", dyingDescription)
				);
				focusBox->AddSpacer();
			};
			
			if (unit.foodActual() <= 0) {
				dyingMessage(LOCTEXT("Dying from starvation", "Dying from starvation"));
			}
			else if (unit.heatActual() <= 0) {
				dyingMessage(LOCTEXT("Dying from the freezing cold", "Dying from the freezing cold"));
			}
			else if (unit.hp() <= 0) {
				dyingMessage(LOCTEXT("Dying from sickness", "Dying from sickness"));
			}
			else if (unit.foodActual() <= unit.minWarnFood()) {
				dyingMessage(LOCTEXT("Starving", "Starving"));
			}
			else if (unit.heatActual() <= unit.minWarnHeat()) {
				dyingMessage(LOCTEXT("Freezing", "Freezing"));
			}
			else if (unit.isSick()) {
				dyingMessage(LOCTEXT("Sick", "Sick"));
			}
			// Tools
			else if (unit.isEnum(UnitEnum::Human)) 
			{
				auto& human = unit.subclass<HumanStateAI>();
				if (human.needTools()) {
					dyingMessage(LOCTEXT("Cannot work no tools", "Cannot work properly without tools"));
				}
#if !UE_BUILD_SHIPPING
				if (PunSettings::IsOn("DebugFocusUI")) {
					focusBox->AddRichText("-- Next tools sec", to_string((human.nextToolTick() - Time::Ticks()) / Time::TicksPerSecond));
				}
#endif
			}

			// Status (Human)
			if (unit.isEnum(UnitEnum::Human)) 
			{
				auto& human = unit.subclass<HumanStateAI>();

				{
					auto widget = focusBox->AddWGT_TextRow(UIEnum::WGT_ObjectFocus_TextRow,
						LOCTEXT("Work efficiency", "Work efficiency"),
						TEXT_PERCENT(human.workEfficiency100())
					);

					// Tip
					TArray<FText> args;
					ADDTEXT_(
						LOCTEXT("Work efficiency tip", "Work efficiency: {0}%\n 100% base\n penalties:\n"),
						TEXT_NUM(human.workEfficiency100())
					);

					auto addRow = [&](FText typeName, int32 percent) {
						ADDTEXT_(INVTEXT("  {0}% {1}\n"), TEXT_NUM(percent), typeName);
					};
					if (human.foodWorkPenalty100() < 0) {
						addRow(LOCTEXT("starvation", "starvation"), human.foodWorkPenalty100());
					}
					if (human.heatWorkPenalty100() < 0) {
						addRow(LOCTEXT("cold", "cold"), human.heatWorkPenalty100());
					}
					if (human.toolPenalty100() < 0) {
						addRow(LOCTEXT("no tools", "no tools"), human.toolPenalty100());
					}
					if (human.sicknessPenalty100() < 0) {
						addRow(LOCTEXT("sick", "sick"), human.sicknessPenalty100());
					}
					if (human.happinessPenalty100() < 0) {
						addRow(LOCTEXT("happiness", "happiness"), human.happinessPenalty100());
					}

					ADDTEXT_LOCTEXT(" adjustments:\n", " adjustments:\n");
					if (human.speedBoostEfficiency100() != 0) {
						addRow(LOCTEXT("leader speed boost", "leader speed boost"), human.speedBoostEfficiency100());
					}
					
					AddToolTip(widget, args);
				}
			}

			if (unit.isEnum(UnitEnum::Human))
			{
				if (unit.tryWorkFailEnum() != TryWorkFailEnum::None) {
					focusBox->AddWGT_PunRichText(UIEnum::WGT_ObjectFocus_FlavorText, 
						GetTryWorkFailEnumName(unit.tryWorkFailEnum())
					);
					focusBox->AddSpacer();
				}
			}
			
			//ADDTEXT_(INVTEXT("{0}: {1}\n"), LOCTEXT("Activity", "Activity"), UnitStateName[static_cast<int>(unit.unitState())]);
			focusBox->AddWGT_TextRow(UIEnum::WGT_ObjectFocus_TextRow,
				LOCTEXT("Activity", "Activity"), 
				UnitStateName[static_cast<int>(unit.unitState())]
			);

			// Workplace (Human)
			if (unit.isEnum(UnitEnum::Human))
			{
				if (unit.workplaceId() != -1)
				{
					Building& workplace = sim.building(unit.workplaceId());

					if (workplace.isConstructed()) {
						focusBox->AddWGT_TextRow(UIEnum::WGT_ObjectFocus_TextRow,
							LOCTEXT("Workplace", "Workplace"), 
							workplace.buildingInfo().GetName()
						);
					}
					else {
						focusBox->AddWGT_TextRow(UIEnum::WGT_ObjectFocus_TextRow,
							LOCTEXT("Workplace", "Workplace"),
							LOCTEXT("Construction Site", "Construction Site")
						);
					}

#if !UE_BUILD_SHIPPING
					if (PunSettings::IsOn("DebugFocusUI")) {
						focusBox->AddRichText(FText::Format(
							INVTEXT(" (id: {0})"), TEXT_NUM(unit.workplaceId())
						));
					}
#endif
				}
				else {
					focusBox->AddWGT_TextRow(UIEnum::WGT_ObjectFocus_TextRow,
						LOCTEXT("Workplace", "Workplace"),
						LOCTEXT("None", "None")
					);
				}

				focusBox->AddSpacer();
			}

			// House (Human)
			if (unit.houseId() != -1) {
#if !UE_BUILD_SHIPPING
				if (PunSettings::IsOn("DebugFocusUI")) 
				{
					TArray<FText> args;
					FText houseName = sim.building(unit.houseId()).buildingInfo().GetName();
					ADDTEXT_(LOCTEXT("House: X", "House: {0}"), houseName);
					ADDTEXT_(INVTEXT(" (id: {0})"), TEXT_NUM(unit.houseId()));
					args.Add(INVTEXT("\n"));
					focusBox->AddRichText(args);
				}
#endif
			}

#if !UE_BUILD_SHIPPING
			if (PunSettings::IsOn("DebugFocusUI")) {
				if (IsAnimal(unit.unitEnum())) {
					TArray<FText> args;
					ADDTEXT_(INVTEXT("Home province {0}"), TEXT_NUM(unit.homeProvinceId()));
					focusBox->AddRichText(args);
				}
			}
#endif

			//! Debug
#if !UE_BUILD_SHIPPING
			if (PunSettings::IsOn("DebugFocusUI")) {
				if (PunSettings::IsOn("UIActions")) {
					std::stringstream ss;
					ss << "-- nextActiveTick:" << unit.nextActiveTick() << "\n";
					ss << "speech:\n" << unit.debugSpeech(true);
					auto punText = focusBox->AddText(ss);
					
					FSlateFontInfo fontInfo = FSlateFontInfo(FPaths::EngineContentDir() / TEXT("Slate/Fonts/Roboto-Bold.ttf"), 7);
					punText->PunText->SetFont(fontInfo);
					//punText->PunText->SetRenderScale(FVector2D(1.0f, 0.5));
				}


				// Animal Arrow to Home Province
				if (!unit.isEnum(UnitEnum::Human) && unit.homeProvinceId() != -1)
				{
					WorldTile2 provinceCenter = sim.GetProvinceCenterTile(unit.homeProvinceId());
					dataSource()->ShowDeliveryArrow(dataSource()->DisplayLocation(unit.unitTile().worldAtom2()), 
													dataSource()->DisplayLocation(provinceCenter.worldAtom2()));
				}
			}
#endif

			/*
			 * Unit Capture Button
			 */
			if (IsWildAnimal(unit.unitEnum()) &&
				sim.IsResearched(playerId(), TechEnum::Zoo))
			{
				int32 animalPrice = sim.GetCaptureAnimalPrice(playerId(), unit.unitEnum(), unit.unitTile());

				if (sim.IsTileOwnedByForeignPlayer(playerId(), unit.unitTile())) {
					focusBox->AddWGT_WarningText(TEXT_TAG("<Red>", LOCTEXT("WildAnimalStealWarn_FocusUI", "Stealing animal from foreign territory incur extra cost")));
				}

				UPunButton* button = focusBox->AddButton2Lines(
					FText::Format(
						LOCTEXT("UnitCapture_Button", "Capture\n<img id=\"Coin\"/>{0}"), 
						TextRed(FText::AsNumber(animalPrice), animalPrice > sim.moneyCap32(playerId()))
					),
					this, CallbackEnum::CaptureUnit, true, false, objectId, unit.birthTicks()
				);

				// Tooltip
				AddToolTip(button, LOCTEXT("UnitCapture_Tip", "Captured animals can be placed in Zoo to increase its Service Quality and Town Attractiveness."));
			};
			

			/*
			 * Unit Inventory
			 */
			focusBox->AddLineSpacer();

			if (unit.inventory().Count() > 0)
			{
				focusBox->AddWGT_PunText(UIEnum::WGT_ObjectFocus_Subheader, LOCTEXT("Inventory:", "Inventory:"));

				unit.inventory().ForEachResource([&](ResourcePair resource) {
					focusBox->AddIconPair(FText(), resource.resourceEnum, TEXT_NUM(resource.count), false, false, focusBox->indentation);
				});
			}
			else
			{
				focusBox->AddWGT_PunText(UIEnum::WGT_ObjectFocus_Subheader, LOCTEXT("Inventory:", "Inventory:"));
				focusBox->AddRichText(
					TEXT_TAG("<Bold>", LOCTEXT("None", "None"))
				);
			}

			// Selection Meshes
			if (unit.houseId() != -1) {
				FVector selectorLocation = dataSource()->DisplayLocationTrueCenter(sim.building(unit.houseId()));
				SpawnSelectionMesh(assetLoader->SelectionMaterialYellow, selectorLocation);
			}
			if (unit.workplaceId() != -1) {
				FVector selectorLocation = dataSource()->DisplayLocationTrueCenter(sim.building(unit.workplaceId()));
				SpawnSelectionMesh(assetLoader->SelectionMaterialYellow, selectorLocation);
			}

			// Selection Mesh
			WorldAtom2 atom = dataSource()->unitDataSource().actualAtomLocation(objectId);
			SpawnSelectionMesh(assetLoader->SelectionMaterialGreen, dataSource()->DisplayLocation(atom) + FVector(0, 0, 30));

			//
			// Vertex Animated Meshes
			//  - For these, we can use the mesh directly without extra animation. (GetUnitTransformAndVariation gives proper animation state)
			//  - Skel meshes are highlighted in UnitDisplayComponent ... (search "targetCustomDepth" to find the code location)
			if (unitEnum != UnitEnum::Human &&
				unitEnum != UnitEnum::WildMan)
			{
				FTransform transform;
				UnitDisplayState displayState = dataSource()->GetUnitTransformAndVariation(unit, transform);
				UStaticMesh* unitMesh = assetLoader->unitAsset(displayState.unitEnum, displayState.variationIndex).staticMesh;
				if (IsValidPun(unitMesh)) {
					SpawnMesh(unitMesh, unitMesh->GetMaterial(0), transform, false, 2);
				}
			}

			focusBox->AfterAdd();
		}

		/*
		 * TileObject
		 */
		else if (uiState.objectType == ObjectTypeEnum::TileObject)
		{
			auto& treeSystem = sim.treeSystem();
			TileObjInfo info = treeSystem.tileInfo(objectId);
			WorldTile2 tile(objectId);

			int32 provinceId = sim.GetProvinceIdClean(tile);

			focusBox->AddWGT_ObjectFocus_Title(info.name);
			focusBox->AddWGT_PunRichText(UIEnum::WGT_ObjectFocus_FlavorText, info.description);
			
			
			if (info.IsPlant()) {
				Indent(20);
				focusBox->AddWGT_TextRow(UIEnum::WGT_ObjectFocus_TextRow, LOCTEXT("Growth percentage", "Growth percentage"), TEXT_PERCENT(treeSystem.growthPercent(objectId)));
				focusBox->AddWGT_TextRow(UIEnum::WGT_ObjectFocus_TextRow, LOCTEXT("Lifespan percentage", "Lifespan percentage"), TEXT_PERCENT(treeSystem.lifeSpanPercent(objectId)));
				focusBox->AddWGT_TextRow(UIEnum::WGT_ObjectFocus_TextRow, LOCTEXT("Bonus yield", "Bonus yield"), TEXT_PERCENT(treeSystem.bonusYieldPercent(objectId)));
				ResetIndent();
			}

			if (treeSystem.HasMark(playerId(), objectId))
			{
				focusBox->AddSpacer();
				focusBox->AddWGT_PunText(UIEnum::WGT_ObjectFocus_TextLeft, LOCTEXT("Marked for gather.", "Marked for gather."));
			}

#if !UE_BUILD_SHIPPING
			if (PunSettings::IsOn("DebugFocusUI"))
			{
				if (treeSystem.IsReserved(objectId))
				{
					int32_t reserverId = treeSystem.Reservation(objectId).unitId;
					UnitStateAI& unit = sim.unitAI(reserverId);

					TArray<FText> args;
					ADDTEXT_LOCTEXT("Reserver: \n", "Reserver: \n");
					if (unit.isEnum(UnitEnum::Human)) {
						ADDTEXT_(INVTEXT(" {0}\n"), unit.GetUnitNameT());
					}
					ADDTEXT_(INVTEXT(" {0} (id: {1})"), unit.unitInfo().name, TEXT_NUM(objectId));
					focusBox->AddRichText(args);
				}
			}
#endif


			focusBox->AddWGT_PunText(UIEnum::WGT_ObjectFocus_SectionTitle, LOCTEXT("Tile & Province Info", "Tile & Province Info"));
			
			{
				auto& terrainGen = sim.terrainGenerator();

				Indent(40);
				focusBox->AddWGT_TextRow(UIEnum::WGT_ObjectFocus_TextRow, LOCTEXT("Biome", "Biome"), terrainGen.GetBiomeNameT(tile));
				focusBox->AddWGT_TextRow(UIEnum::WGT_ObjectFocus_TextRow, LOCTEXT("Fertility", "Fertility"), TEXT_PERCENT(sim.GetFertilityPercent(tile)));
				focusBox->AddWGT_TextRow(UIEnum::WGT_ObjectFocus_TextRow, LOCTEXT("Appeal", "Appeal"), TEXT_PERCENT(terrainGen.GetAppealPercent(tile)));
				ResetIndent();
				
				focusBox->AddLineSpacer();
			}

			AddBiomeDebugInfo(tile, focusBox);


			// Georesource
			AddGeoresourceInfo(provinceId, focusBox);

			// Province Upkeep Info
			AddProvinceUpkeepInfo(provinceId, focusBox);

			/*
			 * 
			 */

			AddSelectStartLocationButton(provinceId, focusBox);
			AddClaimLandButtons(provinceId, focusBox);

			// Selection Mesh
			FVector displayLocation = dataSource()->DisplayLocation(tile.worldAtom2());

			float selectionHeight = GameDisplayUtils::TileObjHoverMeshHeight(info, objectId, treeSystem.tileObjAge(objectId));

			SpawnSelectionMesh(assetLoader->SelectionMaterialGreen, displayLocation + FVector(0, 0, selectionHeight));

			ShowTileSelectionDecal(dataSource()->DisplayLocation(tile.worldAtom2()));
			if (sim.tileOwnerPlayer(tile) == playerId()) {
				ShowRegionSelectionDecal(tile, true);
			} else {
				ShowRegionSelectionDecal(tile);
			}

			// Selection highlight
			if (info.type == ResourceTileType::Deposit) {
				TArray<UStaticMesh*> assets = assetLoader->tileMeshAsset(info.treeEnum).assets;
				int32 variationCount = assets.Num();
				int32 variationIndex;
				FTransform transform = GameDisplayUtils::GetDepositTransform(objectId, displayLocation, variationCount, variationIndex);
				SpawnMesh(assets[variationIndex], assets[variationIndex]->GetMaterial(0), transform, false, 2);
			}
			else if (info.type == ResourceTileType::Bush)
			{
				FTransform transform = GameDisplayUtils::GetBushTransform(displayLocation, 0, objectId, treeSystem.tileObjAge(objectId), info, sim.terrainGenerator().GetBiome(tile));

				// Plant uses the mesh array for multiple meshes (for example flower + its leaves)
				TArray<UStaticMesh*> assets = assetLoader->tileMeshAsset(info.treeEnum).assets;
				for (int32 i = 0; i < assets.Num(); i++) {
					UStaticMesh* mesh = assets[i];
					if (mesh) {
						SpawnMesh(mesh, mesh->GetMaterial(0), transform, false, 2, info.treeEnum != TileObjEnum::GrassGreen);
					}
				}
			}
			else if (info.type == ResourceTileType::Tree)
			{
				FTransform transform = GameDisplayUtils::GetTreeTransform(displayLocation, 0, objectId, treeSystem.tileObjAge(objectId), info);

				TArray<UStaticMesh*> assets = assetLoader->tileMeshAsset(info.treeEnum).assets;
				for (int32 i = 0; i < 2; i++) {
					UStaticMesh* mesh = assets[i];
					if (mesh) {
						SpawnMesh(mesh, mesh->GetMaterial(0), transform, false, 2);
					}
				}
			}

			focusBox->AfterAdd();
		}
		else if (uiState.objectType == ObjectTypeEnum::Drop)
		{
			auto& dropSystem = sim.dropSystem();
			WorldTile2 tile(objectId);
			std::vector<DropInfo> drops = dropSystem.GetDrops(tile);

			if (drops.size() > 0)
			{
				auto& resourceSystem = sim.resourceSystem(drops[0].townId);

				//SetText(_objectDescriptionUI->DescriptionUITitle, FText::Format(
				//	LOCTEXT("DroppedResourceObjUI", "<Header>Dropped Resource {0}</>"),
				//	tile.ToText()
				//));
				focusBox->AddWGT_ObjectFocus_Title(LOCTEXT("DroppedResourceObjUI", "Dropped Resource"));
				
				focusBox->AddLineSpacer();

				TArray<FText> args;
				for (int i = 0; i < drops.size(); i++) 
				{
					ADDTEXT_(INVTEXT("{0} {1}\n"),
						ResourceNameT(drops[i].holderInfo.resourceEnum),
						TEXT_NUM(resourceSystem.resourceCount(drops[i].holderInfo))
					);
				}
				focusBox->AddRichText(args);

				// Selection Mesh
				SpawnSelectionMesh(assetLoader->SelectionMaterialGreen, dataSource()->DisplayLocation(tile.worldAtom2()) + FVector(0, 0, 20));

				ShowTileSelectionDecal(dataSource()->DisplayLocation(tile.worldAtom2()));

				focusBox->AfterAdd();
			}
			else {
				sim.SetDescriptionUIState(DescriptionUIState(ObjectTypeEnum::None, -1));
			}
		}
		else if (uiState.objectType == ObjectTypeEnum::EmptyTile)
		{
			WorldTile2 tile(objectId);
			WorldRegion2 region = tile.region();

			if (tile.isValid())
			{
				RoadTile roadTile = sim.overlaySystem().GetRoad(tile);
				if (roadTile.isValid())
				{
					const BldInfo& info = GetBuildingInfo(roadTile.isDirt ? CardEnum::DirtRoad : CardEnum::StoneRoad);

					focusBox->AddWGT_ObjectFocus_Title(info.GetName());
					focusBox->AddWGT_PunRichText(UIEnum::WGT_ObjectFocus_FlavorText,
						info.GetDescription()
					);
					
					if (!roadTile.isConstructed) {
						focusBox->AddRichText(LOCTEXT("UnderConstruction", "Under construction"));
					}
				}
				else
				{

					int32 provinceId = sim.GetProvinceIdClean(tile);
					if (provinceId == -1) {
						AddImpassableTileInfo(tile, focusBox);
					}
					else if (sim.IsWater(tile)) {
						focusBox->AddWGT_ObjectFocus_Title(LOCTEXT("Water Tile", "Water Tile"));
					}
					else if (sim.IsMountain(tile)) {
						focusBox->AddWGT_ObjectFocus_Title(LOCTEXT("Mountain Tile", "Mountain Tile"));
					}
					else
					{
						BiomeEnum biomeEnum = sim.GetBiomeEnum(tile);
						BiomeInfo biomeInfo = GetBiomeInfo(biomeEnum);

						focusBox->AddWGT_ObjectFocus_ProvinceTitle(
							FText::Format(LOCTEXT("TileInfoHeader", "{0} Tile"), biomeInfo.name),
							biomeInfo.description,
							biomeEnum
						);
					}

#if !UE_BUILD_SHIPPING
					if (PunSettings::IsOn("DebugFocusUI")) {
						TArray<FText> args;
						ADDTEXT_(INVTEXT("--Province: {0}\n"), provinceId);
						ADDTEXT_(INVTEXT("--Region({0}, {1})\n"), region.x, region.y);
						ADDTEXT_(INVTEXT("--LocalTile({0}, {1})\n"), tile.localTile(region).x, tile.localTile(region).y)
							ADDTEXT_(INVTEXT("<Header>{0}</>"), tile.ToText());
						focusBox->AddSpecialRichText(INVTEXT("<Yellow>"), args);
					}
#endif

					// Biome
					AddBiomeInfo(tile, focusBox);
					focusBox->AddLineSpacer();


					// Region Owner
					int32 ownerPlayerId = sim.provinceOwnerPlayer(provinceId);
					focusBox->AddWGT_TextRow(UIEnum::WGT_ObjectFocus_TextRow,
						LOCTEXT("TileOwner", "Owner"),
						(ownerPlayerId == -1 ? INVTEXT("None") : sim.playerNameT(ownerPlayerId))
					);
					focusBox->AddLineSpacer();


					// Georesource
					AddGeoresourceInfo(provinceId, focusBox);

					// Province Upkeep Info
					AddProvinceUpkeepInfo(provinceId, focusBox);


					// Claim land
					AddSelectStartLocationButton(provinceId, focusBox);
					AddClaimLandButtons(provinceId, focusBox);

					// Extras
#if !UE_BUILD_SHIPPING
					if (PunSettings::IsOn("DebugFocusUI"))
					{
						TArray<FText> args;
						FVector displayLocation = dataSource()->DisplayLocation(tile.worldAtom2());
						ADDTEXT_(INVTEXT("Display ({0},{1})"), displayLocation.X, displayLocation.Y);
						focusBox->AddSpecialRichText(INVTEXT("<Yellow>"), args);

						int32 provincePlayerId = sim.provinceOwnerPlayer(provinceId);
						if (provincePlayerId != -1) {
							AIPlayerSystem& aiPlayer = sim.aiPlayerSystem(provincePlayerId);

							if (aiPlayer.active()) {
								AIRegionStatus* regionStatus = aiPlayer.regionStatus(provinceId);
								if (regionStatus) {
									ADDTEXT_(INVTEXT("AIRegion ({0}, proposed:{1})"),
										ToFText(AIRegionPurposeName[static_cast<int>(regionStatus->currentPurpose)]),
										ToFText(AIRegionProposedPurposeName[static_cast<int>(regionStatus->proposedPurpose)])
									);
									focusBox->AddSpecialRichText(INVTEXT("<Yellow>"), args);
								}
							}
						}
					}
#endif
				}

				ShowTileSelectionDecal(dataSource()->DisplayLocation(tile.worldAtom2()));
				if (sim.tileOwnerPlayer(tile) != playerId()) {
					ShowRegionSelectionDecal(tile);
				}
			}

			focusBox->AfterAdd();
		}
		else if (uiState.objectType == ObjectTypeEnum::Map)
		{
			SCOPE_CYCLE_COUNTER(STAT_PunUIProvinceFocusUI);

			WorldTile2 tile(objectId);

			if (tile.isValid()) {
				int32 provinceId = sim.GetProvinceIdClean(objectId);
				AddProvinceInfo(provinceId, tile, focusBox);
			}

			focusBox->AfterAdd();
		}

		_objectDescriptionUI->SetVisibility(ESlateVisibility::SelfHitTestInvisible);
	}

	dataSource()->SetOverlayType(overlayType, OverlaySetterType::ObjectDescriptionUI);

	// Clear unused Selection Meshes
	for (int32 i = meshIndex; i < _selectionMeshes.Num(); i++) {
		_selectionMeshes[i]->SetVisibility(false);
	}

	for (int32 i = skelMeshIndex; i < _selectionSkelMeshes.Num(); i++) {
		_selectionSkelMeshes[i]->SetVisibility(false);
	}

	_justOpenedDescriptionUI = false;

	//if (_buildingMesh) {
	//	_buildingMesh->AfterAdd();
	//}
}

bool UObjectDescriptionUISystem::TryMouseCollision(UStaticMesh* mesh, FTransform transform, float& shortestHit)
{
	if (!_collider) {
		_collider = NewObject<UStaticMeshComponent>(dataSource()->componentToAttach());
		_collider->AttachToComponent(dataSource()->componentToAttach(), FAttachmentTransformRules::KeepRelativeTransform, NAME_None);
		_collider->SetGenerateOverlapEvents(false);
		_collider->SetReceivesDecals(false);
		_collider->RegisterComponent();
		_collider->SetVisibility(false);
	}

	_collider->SetStaticMesh(mesh);
	_collider->SetWorldTransform(transform);
	_collider->SetCollisionEnabled(ECollisionEnabled::QueryOnly);

	auto controller = CastChecked<APlayerController>(networkInterface());
	FVector2D screenPosition;
	controller->GetMousePosition(screenPosition.X, screenPosition.Y);

	FVector WorldOrigin;
	FVector WorldDirection;
	if (UGameplayStatics::DeprojectScreenToWorld(controller, screenPosition, WorldOrigin, WorldDirection)) {
		FHitResult hitResult;
		_collider->LineTraceComponent(hitResult, WorldOrigin, WorldOrigin + WorldDirection * 100000.0f, FCollisionQueryParams());

		
		if (hitResult.GetComponent() && hitResult.Distance < shortestHit) {
			shortestHit = hitResult.Distance;
			
			//PUN_LOG("Transform %s", *transform.ToString());
			PUN_LOG("Hit: %s", *hitResult.ToString());
			_collider->SetCollisionEnabled(ECollisionEnabled::NoCollision);
			return true;
		}
	}
	_collider->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	return false;
}

void UObjectDescriptionUISystem::SpawnMesh(UStaticMesh* mesh, UMaterialInterface* material, FTransform transform, bool isRotating, int32 customDepth, bool castShadow)
{
	if (dataSource()->isPhotoMode()) {
		return;
	}
	
	// Create new mesh if neededs
	UStaticMeshComponent* meshComponent;
	if (_selectionMeshes.Num() <= meshIndex) {
		meshComponent = NewObject<UStaticMeshComponent>(dataSource()->componentToAttach());
		meshComponent->AttachToComponent(dataSource()->componentToAttach(), FAttachmentTransformRules::KeepRelativeTransform, NAME_None);
		meshComponent->SetGenerateOverlapEvents(false);
		meshComponent->SetCollisionEnabled(ECollisionEnabled::NoCollision);
		meshComponent->SetReceivesDecals(false);
		meshComponent->RegisterComponent();
		_selectionMeshes.Add(meshComponent);
	}
	else {
		meshComponent = _selectionMeshes[meshIndex];
	}

	meshComponent->SetStaticMesh(mesh);
	meshComponent->SetMaterial(0, material);
	meshComponent->SetWorldTransform(transform);
	meshComponent->SetCastShadow(castShadow);

	GameDisplayUtils::SetCustomDepth(meshComponent, customDepth);

	if (isRotating) {
		meshComponent->SetWorldRotation(FRotator(0, fmod(UGameplayStatics::GetTimeSeconds(GetWorld()), 1.0f) * 360, 0));
	}

	meshComponent->SetVisibility(true);
	meshIndex++;

}


void UObjectDescriptionUISystem::ShowTileSelectionDecal(FVector displayLocation, WorldTile2 size, UMaterial* material)
{
	auto assetLoader = dataSource()->assetLoader();
	if (!material) {
		material = assetLoader->M_TileHighlightDecal;
	}
	if (!_tileSelectionDecal) {
		_tileSelectionDecal = PunUnrealUtils::CreateDecal(dataSource()->componentToAttach(), material);

		_tileSelectionMesh = PunUnrealUtils::CreateStaticMesh(dataSource()->componentToAttach());
		_tileSelectionMesh->SetStaticMesh(assetLoader->PlacementMesh);
		_tileSelectionMesh->SetMaterial(0, assetLoader->M_TileHighlightForMesh);
	}
	_tileSelectionDecal->SetDecalMaterial(material);
	PunUnrealUtils::ShowComponent(_tileSelectionDecal, displayLocation, FVector(1, size.y, size.x));

	displayLocation.Z -= 3;
	_tileSelectionMesh->SetTranslucentSortPriority(20000);
	PunUnrealUtils::ShowComponent(_tileSelectionMesh, displayLocation, FVector(size.x, size.y, 1) * CoordinateConstants::DisplayUnitPerTile / 2);
}
void UObjectDescriptionUISystem::ShowRegionSelectionDecal(WorldTile2 tile, bool isHover)
{
	SCOPE_CYCLE_COUNTER(STAT_PunUI_ShowRegionSelectionDecal);
	
	if (dataSource()->zoomDistance() < 190 ||
		PunSettings::TrailerMode()) 
	{
		if (_regionHoverMesh) {
			_regionHoverMesh->SetVisibility(false);
		}
		return;
	}
	
	if (!tile.isValid()) {
		if (_regionHoverMesh) {
			_regionHoverMesh->SetVisibility(false);
		}
		return;
	}
	
	auto& provinceSys = dataSource()->simulation().provinceSystem();
	int32 provinceId = provinceSys.GetProvinceIdClean(tile);

	if (provinceId == -1) {
		if (_regionHoverMesh) {
			_regionHoverMesh->SetVisibility(false);
		}
		return;
	}

	//auto& decal = isHover ? _regionHoverDecal : _regionSelectionDecal;
	auto& mesh = isHover ? _regionHoverMesh : _regionSelectionMesh;
	
	if (!mesh) {
		auto assetLoader = dataSource()->assetLoader();
		//auto decalMaterial = isHover ? assetLoader->M_RegionHighlightDecalFaded : assetLoader->M_RegionHighlightDecal;
		auto meshMaterial = isHover ? assetLoader->M_TerritoryHighlightForMeshFaded : assetLoader->M_TerritoryHighlightForMesh;
		
		//decal = PunUnrealUtils::CreateDecal(dataSource()->componentToAttach(), decalMaterial);
		
		//mesh = PunUnrealUtils::CreateStaticMesh(dataSource()->componentToAttach());
		//mesh->SetStaticMesh(assetLoader->PlacementMesh);

		auto comp = NewObject<UTerritoryMeshComponent>(dataSource()->componentToAttach());
		comp->Rename(*(FString("TerritoryChunkHighlight") + FString::FromInt(isHover)));
		comp->AttachToComponent(dataSource()->componentToAttach(), FAttachmentTransformRules::KeepRelativeTransform, NAME_None);
		comp->SetCollisionEnabled(ECollisionEnabled::NoCollision);
		comp->SetGenerateOverlapEvents(false);
		comp->bAffectDistanceFieldLighting = false;
		comp->SetReceivesDecals(false);
		comp->SetCastShadow(false);
		comp->RegisterComponent();
		comp->SetTerritoryMaterial(meshMaterial, meshMaterial);
		mesh = comp;
	}

	if (mesh->provinceId != provinceId) {
		mesh->UpdateMesh(true, provinceId, -1, -1, false, &simulation(), 50);
	}

	WorldTile2 provinceCenter = provinceSys.GetProvinceCenter(provinceId).worldTile2();
	
	FVector displayLocation = dataSource()->DisplayLocation(provinceCenter.worldAtom2());

	mesh->SetWorldLocation(displayLocation);
	mesh->SetVisibility(true);
}

void UObjectDescriptionUISystem::AddSelectStartLocationButton(int32 provinceId, UPunBoxWidget* descriptionBox)
{
	if (provinceId == -1) {
		return;
	}
	SCOPE_CYCLE_COUNTER(STAT_PunUISelect_Start);
	
	bool hasChosenLocation = simulation().HasChosenLocation(playerId());
	// If player hasn't select starting location
	if (!hasChosenLocation)
	{
		descriptionBox->AddSpacer();
		
		bool canClaim = true;

		if (simulation().provinceOwnerTown_Major(provinceId) != -1) {
			descriptionBox->AddRichText(TEXT_TAG("<Red>", LOCTEXT("Already has owner.", "Already has owner.")));
			canClaim = false;
		}
		
		if (!SimUtils::CanReserveSpot_NotTooCloseToAnother(provinceId, &simulation(), 2)) {
			descriptionBox->AddRichText(TEXT_TAG("<Red>", LOCTEXT("Too close to another town", "Too close to another town")));
			canClaim = false;
		}

		TileArea area;
		{
			SCOPE_CYCLE_COUNTER(STAT_PunUIFindStartSpot);
			area = SimUtils::FindStartSpot(provinceId, &simulation());
		}
		
		if (!area.isValid()) {
			descriptionBox->AddRichText(TEXT_TAG("<Red>", LOCTEXT("Not enough buildable space.", "Not enough buildable space.")));
			canClaim = false;
		}


		int32 provincePriceMoney = simulation().GetProvinceClaimPrice(provinceId, playerId()) * GameConstants::ClaimProvinceByMoneyMultiplier;

		TArray<FText> args;
		ADDTEXT_LOCTEXT("SelectStart", "Select Starting Location");

		ADDTEXT_INV_("\n");
		switch (simulation().GetBiomeProvince(provinceId))
		{
		case BiomeEnum::Tundra:
			ADDTEXT_LOCTEXT("Difficulty: Extreme", "<Red>Extreme Difficulty</>");
			break;
		case BiomeEnum::Desert:
			ADDTEXT_LOCTEXT("Difficulty: Very Hard", "<Red>Very Hard Difficulty</>");
			break;
		case BiomeEnum::BorealForest:
		case BiomeEnum::Savanna:
		case BiomeEnum::GrassLand:
			ADDTEXT_LOCTEXT("Difficulty: Hard", "<Orange>Hard Difficulty</>");
			break;
		case BiomeEnum::Jungle:
			ADDTEXT_LOCTEXT("Difficulty: Somewhat Hard", "<Orange>Somewhat Hard Difficulty</>");
			break;
		case BiomeEnum::Forest:
			ADDTEXT_LOCTEXT("Difficulty: Normal", "<Gray>Normal Difficulty</>");
			break;
		}

		
		ADDTEXT_INV_("\n");
		if (area.isValid()) {
			ADDTEXT_(INVTEXT("<img id=\"Coin\"/>{0}"), TextRed(FText::AsNumber(provincePriceMoney), !canClaim));
		} else {
			ADDTEXT_TAG_("<Red>", LOCTEXT("NotEnoughBuildingSpace", "Not enough buildable space."));
		}
		
		descriptionBox->AddButton2Lines(JOINTEXT(args), this, CallbackEnum::SelectStartingLocation, canClaim, false, provinceId);
	}
}

void UObjectDescriptionUISystem::AddClaimLandButtons(int32 provinceId, UPunBoxWidget* descriptionBox)
{
	if (provinceId == -1) {
		return;
	}
	SCOPE_CYCLE_COUNTER(STAT_PunUI_AddClaimLandButton);
	
	auto& sim = simulation();

	/*
	 * Not owned by anyone
	 */
	int32 provinceTownId = sim.provinceOwnerTownSafe(provinceId);

	int32 allowResourceClaim = (provinceTownId == -1);

	// Allow claiming level 1 Minor City
	if (IsMinorTown(provinceTownId)) {
		allowResourceClaim = sim.townManagerBase(provinceTownId)->GetMinorCityLevel() <= 1;
	}
	
	if (allowResourceClaim)
	{	
		auto addClaimButtons = [&](ClaimConnectionEnum claimConnectionEnum)
		{	
			int32 provincePrice = sim.GetProvinceClaimPrice(provinceId, playerId(), claimConnectionEnum);
			
			if (simulation().GetBiomeProvince(provinceId) == BiomeEnum::Jungle) {
				descriptionBox->AddSpacer();
				descriptionBox->AddWGT_PunRichText(UIEnum::WGT_ObjectFocus_FlavorText,
					TEXT_TAG("<Red>", LOCTEXT("JungleDifficultToClear", "The difficulty in clearing jungle makes expansion more costly."))
				);
			}

			// Claim by influence
			if (sim.unlockedInfluence(playerId()))
			{
				bool canClaim = sim.influence(playerId()) >= provincePrice;

				TArray<FText> args;
				ADDTEXT_LOCTEXT("ClaimProvince", "Claim Province");
				AppendClaimConnectionString(args, claimConnectionEnum);
				ADDTEXT_(INVTEXT("\n<img id=\"Influence\"/>{0}"), TextRed(FText::AsNumber(provincePrice), !canClaim));

				descriptionBox->AddSpacer();
				descriptionBox->AddButton2Lines(JOINTEXT(args), this, CallbackEnum::ClaimLandInfluence, canClaim, false, provinceId);
			}

			// Claim by money
			{
				int32 provincePriceMoney = provincePrice * GameConstants::ClaimProvinceByMoneyMultiplier;
				bool canClaim = sim.moneyCap32(playerId()) >= provincePriceMoney;

				TArray<FText> args;
				ADDTEXT_LOCTEXT("ClaimProvince", "Claim Province");
				AppendClaimConnectionString(args, claimConnectionEnum);
				ADDTEXT_(INVTEXT("\n<img id=\"Coin\"/>{0}"), TextRed(TEXT_NUM(provincePriceMoney), !canClaim));

				descriptionBox->AddSpacer();
				UPunButton* button = descriptionBox->AddButton2Lines(JOINTEXT(args), this, CallbackEnum::ClaimLandMoney, canClaim, false, provinceId);

				{
					// Tip
					args.Empty();
					int32 claimPrice = sim.GetProvinceBaseClaimPrice(provinceId);
					args.Add(FText::Format(LOCTEXT("ClaimByMoneyButton_Tip", "<img id=\"Coin\"/>{0} base"), TEXT_NUM(claimPrice)));
					
					std::vector<BonusPair> pairs = sim.GetProvinceClaimPriceStructure(provinceId, playerId(), claimConnectionEnum, claimPrice);

					for (const BonusPair& pair : pairs) {
						args.Add(FText::Format(INVTEXT("\n  +<img id=\"Coin\"/>{0} {1}"), TEXT_NUM(pair.value), pair.name));
					}
					// Double with money claim
					args.Add(FText::Format(LOCTEXT("Claim by money", "\n  +<img id=\"Coin\"/>{0} claim by money (x2 price)"), TEXT_NUM(provincePrice)));

					args.Add(FText::Format(LOCTEXT("ClaimByMoneyButton_Tip", "\nTotal: {0}<img id=\"Coin\"/>"), TEXT_NUM(provincePriceMoney)));
					
					AddToolTip(button, JOINTEXT(args));
				}
			}

		};

		

		ClaimConnectionEnum claimConnectionEnum = sim.GetProvinceClaimConnectionEnumPlayer(provinceId, playerId());
		if (claimConnectionEnum != ClaimConnectionEnum::None)
		{
			//bool withShallowWater = sim.IsResearched(playerId(), TechEnum::ShallowWaterEmbark);
			int32 provinceDistance = sim.provinceInfoSystem().provinceDistanceToPlayer(provinceId, playerId(), false);
			if (provinceDistance != MAX_int32)
			{
				if (provinceDistance > 7) {
					descriptionBox->AddSpacer();
					descriptionBox->AddWGT_PunRichText(UIEnum::WGT_ObjectFocus_FlavorText,
						TEXT_TAG("<Red>", LOCTEXT("TooFarFromTownhallText", "Cannot claim a province more than 7 provinces from the Townhall (on land)"))
					);
				}
				else {
					addClaimButtons(claimConnectionEnum);
				}
			}
		}
		else if (sim.IsProvinceNextToPlayerIncludingNonFlatLand(provinceId, playerId()))
		{
			descriptionBox->AddSpacer();
			descriptionBox->AddWGT_PunRichText(UIEnum::WGT_ObjectFocus_FlavorText,
				TEXT_TAG("<Red>", LOCTEXT("CannotClaimThroughMtnSea", "Cannot claim province through mountain and sea"))
			);
		}

		descriptionBox->AddSpacer();
	}
	/*
	 * Own by someone
	 */
	else
	{
		TownManagerBase* townManagerBase = sim.townManagerBase(provinceTownId);

		// No townhall, don't do anything
		if (townManagerBase == nullptr || townManagerBase->townhallId == -1) {
			return;
		}

		// if this province overlaps with Townhall, act as if this is the home province
		bool isVassalizing = false;
		if (sim.IsTownhallOverlapProvince(provinceId, provinceTownId)) {
			isVassalizing = true;
			provinceId = sim.building(townManagerBase->townhallId).provinceId();
		}

		if (townManagerBase->playerIdForLogo() != playerId())
		{
			if (simulation().IsResearched(playerId(), TechEnum::Conquer))
			{
				auto addAttackButtons = [&](ClaimConnectionEnum claimConnectionEnum)
				{
					ProvinceClaimProgress claimProgress = townManagerBase->GetDefendingClaimProgress(provinceId);

					// Start a new claim (Claim or Conquer Province)
					if (!claimProgress.isValid())
					{
						int32 startAttackPrice = simulation().GetProvinceAttackStartPrice(provinceId, claimConnectionEnum);
						bool canClaim = simulation().influence(playerId()) >= startAttackPrice;

						TArray<FText> args;
						
						if (isVassalizing) 	{
							ADDTEXT_LOCTEXT("Vassalize", "Vassalize");
						}
						else {
							ADDTEXT_LOCTEXT("ConquerProvince", "Conquer Province (Annex)");
						}
						
						AppendClaimConnectionString(args, claimConnectionEnum);
						ADDTEXT_(INVTEXT("\n{0}<img id=\"Influence\"/>"), TextRed(FText::AsNumber(startAttackPrice), !canClaim));

						descriptionBox->AddSpacer();
						descriptionBox->AddButton2Lines(JOINTEXT(args), this, CallbackEnum::StartAttackProvince, canClaim, false, provinceId);
					}
				};

				ClaimConnectionEnum claimConnectionEnum = sim.GetProvinceClaimConnectionEnumPlayer(provinceId, playerId());
				if (claimConnectionEnum != ClaimConnectionEnum::None) {
					addAttackButtons(claimConnectionEnum);
				}
				else if (sim.IsProvinceNextToPlayerIncludingNonFlatLand(provinceId, playerId()))
				{
					descriptionBox->AddSpacer();
					descriptionBox->AddRichText(
						TEXT_TAG("<Red>", LOCTEXT("NoAttackThroughMtnSea", "Cannot attack through mountain and sea"))
					);
				}
			}
		
		}
	}
}



void UObjectDescriptionUISystem::CallBack1(UPunWidget* punWidgetCaller, CallbackEnum callbackEnum)
{
	if (callbackEnum == CallbackEnum::StartAttackProvince)
	{
		GetPunHUD()->OpenReinforcementUI(punWidgetCaller->callbackVar1, CallbackEnum::StartAttackProvince);
		return;
	}

	
	/*
	 * Regions
	 */
	if (callbackEnum == CallbackEnum::ClaimLandMoney ||
		callbackEnum == CallbackEnum::ClaimLandInfluence ||
		//callbackEnum == CallbackEnum::ClaimLandFood ||

		//callbackEnum == CallbackEnum::StartAttackProvince ||
		callbackEnum == CallbackEnum::ReinforceAttackProvince ||
		callbackEnum == CallbackEnum::ReinforceDefendProvince ||
		callbackEnum == CallbackEnum::DefendProvinceMoney ||

		callbackEnum == CallbackEnum::ClaimLandArmy ||
		callbackEnum == CallbackEnum::CancelClaimLandArmy ||
		callbackEnum == CallbackEnum::ClaimLandIndirect ||
		callbackEnum == CallbackEnum::BuildOutpost ||
		callbackEnum == CallbackEnum::DemolishOutpost
		)
	{
		// If there is a province Building and townhall hasn't been placed, don't claim the land
		int32 provinceId = punWidgetCaller->callbackVar1;

		bool hasProvinceBuilding = false;
		const std::vector<WorldRegion2>& regionOverlaps = simulation().provinceSystem().GetRegionOverlaps(provinceId);
		for (WorldRegion2 regionOverlap : regionOverlaps) {
			auto& buildingList = simulation().buildingSubregionList();
			buildingList.ExecuteRegion(regionOverlap, [&](int32 buildingId)
			{
				auto bld = simulation().building(buildingId);
				if (IsRegionalBuilding(bld.buildingEnum()) &&
					simulation().GetProvinceIdClean(bld.centerTile()) == provinceId)
				{
					hasProvinceBuilding = true;
				}
			});
		}

		
		if (!simulation().HasTownhall(playerId()) && hasProvinceBuilding) {
			simulation().AddPopupToFront(playerId(), 
				LOCTEXT("TownhallBeforeClaiming", "Please build the Townhall before claiming non-empty Province."), 
				ExclusiveUIEnum::None, "PopupCannot"
			);
			return;
		}

		
		PUN_LOG("Claim land");
		auto command = make_shared<FClaimLand>();
		command->claimEnum = callbackEnum;
		command->provinceId = punWidgetCaller->callbackVar1;
		PUN_CHECK(command->provinceId != -1);
		networkInterface()->SendNetworkCommand(command);

		dataSource()->Spawn2DSound("UI", "ClaimLand");

		CloseDescriptionUI();
	}
	//else if (callbackEnum == CallbackEnum::ClaimRuin)
	//{
	//	PUN_LOG("Claim Ruin");
	//	auto command = make_shared<FClaimLand>();
	//	command->provinceId = punWidgetCaller->callbackVar1;
	//	command->claimEnum = callbackEnum;
	//	PUN_CHECK(command->provinceId != -1);
	//	networkInterface()->SendNetworkCommand(command);

	//	CloseDescriptionUI();
	//}
	else if (callbackEnum == CallbackEnum::SelectStartingLocation)
	{
		auto command = make_shared<FChooseLocation>();
		command->provinceId = punWidgetCaller->callbackVar1;
		command->isChoosingOrReserving = true;
		networkInterface()->SendNetworkCommand(command);

		CloseDescriptionUI();
	}
	/*
	 * Buildings
	 */
	else if (callbackEnum == CallbackEnum::UpgradeBuilding)
	{
		// Warn if not enough money for townhall upgrade...
		Building& bld = simulation().building(punWidgetCaller->callbackVar1);
		if (bld.isEnum(CardEnum::Townhall))
		{
			TownHall& townhall = bld.subclass<TownHall>();
			//if (!townhall.isCapital() &&
			//	townhall.townhallLvl >= simulation().GetTownLvl(bld.playerId())) 
			//{
			//	simulation().AddPopupToFront(playerId(),
			//		LOCTEXT("RequireHigherCapitalTownhallLvl", "Upgrade failed. Require higher Capital's Townhall Level"),
			//		ExclusiveUIEnum::None, "PopupCannot"
			//	);
			//	return;
			//}
			
			if (punWidgetCaller->callbackVar2 == 0) {
				if (!townhall.HasEnoughUpgradeMoney()) {
					simulation().AddPopupToFront(playerId(), 
						LOCTEXT("NoUpgradeMoney", "Not enough money for upgrade."), 
						ExclusiveUIEnum::None, "PopupCannot"
					);
					return;
				}
			}
			else {
				if (!townhall.HasEnoughStoneToUpgradeWall()) {
					simulation().AddPopupToFront(playerId(), 
						LOCTEXT("NoUpgradeStone", "Not enough stone for upgrade."), 
						ExclusiveUIEnum::None, "PopupCannot"
					);
					return;
				}
			}
		}

		const FModifierKeysState modifierKeys = FSlateApplication::Get().GetModifierKeys();
		bool isShiftDown = modifierKeys.IsShiftDown();
		//bool isShiftDown = IsShiftDown(GEngine->GameViewport->Viewport);;
		if (isShiftDown) {
			_alreadyDidShiftDownUpgrade = true;
		}

		_LOG(LogNetworkInput, "[ObjDesc] UpgradeBuilding: isShiftDown:%d", isShiftDown);
		
		auto command = make_shared<FUpgradeBuilding>();
		command->upgradeType = punWidgetCaller->callbackVar2;
		command->buildingId = punWidgetCaller->callbackVar1;
		command->isShiftDown = isShiftDown;

		PUN_ENSURE(bld.isEnum(CardEnum::Townhall) || command->upgradeType < bld.upgrades().size(), return);
		
		networkInterface()->SendNetworkCommand(command);

		// Townhall noticed
		if (bld.isEnum(CardEnum::Townhall)) {
			simulation().parameters(playerId())->NeedTownhallUpgradeNoticed = false;
		}
	}
	
	else if (callbackEnum == CallbackEnum::SetDeliveryTarget)
	{
		inputSystemInterface()->StartSetDeliveryTarget(punWidgetCaller->callbackVar1);
	}
	else if (callbackEnum == CallbackEnum::RemoveDeliveryTarget)
	{
		auto command = make_shared<FPlaceBuilding>();
		command->buildingIdToSetDelivery = punWidgetCaller->callbackVar1;
		command->center = WorldTile2::Invalid;

		networkInterface()->SendNetworkCommand(command);
	}

	else if (callbackEnum == CallbackEnum::TradingPostTrade)
	{
		int32 buildingId = punWidgetCaller->callbackVar1;
		Building& building = dataSource()->simulation().building(buildingId);
		PUN_CHECK(IsTradingPostLike(building.buildingEnum()));

		if (static_cast<TradingPost*>(&building)->CanTrade()) {
			GetPunHUD()->OpenTradeUI(buildingId);
		}
	}
	
	else if (callbackEnum == CallbackEnum::OpenStatistics)
	{
		GetPunHUD()->OpenStatisticsUI(punWidgetCaller->callbackVar1);
		//ESlateVisibility visibility = _objectDescriptionUI->StatisticsPanel->GetVisibility();
		//_objectDescriptionUI->StatisticsPanel->SetVisibility(visibility == ESlateVisibility::Visible ? ESlateVisibility::Collapsed : ESlateVisibility::Visible);
	}
	else if (callbackEnum == CallbackEnum::OpenSetTradeOffers)
	{
		GetPunHUD()->OpenIntercityTradeUI(punWidgetCaller->callbackVar1);
	}
	else if (callbackEnum == CallbackEnum::AbandonTown)
	{
		PopupInfo popup(playerId(), 
			LOCTEXT("AskAbandonSettlement_Pop", "Would you like to abandon this settlement to build a new one?"),
			{
				LOCTEXT("YesAbandonSettlement", "Yes, we will abandon this settlement"),
				LOCTEXT("No", "No")
			}, PopupReceiverEnum::Approve_AbandonTown1, true, "PopupBad"
		);
		popup.forcedSkipNetworking = true;
		simulation().AddPopupToFront(popup);
	}
	// ChooseResource
	else if (callbackEnum == CallbackEnum::OpenChooseResource)
	{
		ESlateVisibility visibility = _objectDescriptionUI->ChooseResourceOverlay->GetVisibility();
		_objectDescriptionUI->SearchBox->SetText(FText());
		_objectDescriptionUI->ChooseResourceOverlay->SetVisibility(visibility == ESlateVisibility::Visible ? ESlateVisibility::Collapsed : ESlateVisibility::Visible);
	}
	else if (callbackEnum == CallbackEnum::PickChooseResource)
	{
		auto chooseButton = CastChecked<UChooseResourceElement>(punWidgetCaller);
		auto& tradingCompany = simulation().building(punWidgetCaller->punId).subclass<TradingCompany>();

		auto command = make_shared<FChangeWorkMode>();
		command->buildingId = tradingCompany.buildingId();
		command->intVar1 = static_cast<int32>(chooseButton->resourceEnum);
		command->intVar2 = static_cast<int32>(tradingCompany.isImport);
		command->intVar3 = static_cast<int32>(tradingCompany.targetAmount);
		networkInterface()->SendNetworkCommand(command);

		_objectDescriptionUI->ChooseResourceOverlay->SetVisibility(ESlateVisibility::Collapsed);
	}
	else if (callbackEnum == CallbackEnum::EditNumberChooseResource)
	{
		auto numberBox = CastChecked<UPunEditableNumberBox>(punWidgetCaller);
		auto& tradingCompany = simulation().building(numberBox->punId).subclass<TradingCompany>();
		
		numberBox->amount = std::max(0, numberBox->amount);
		numberBox->UpdateText();

		auto command = make_shared<FChangeWorkMode>();
		command->buildingId = tradingCompany.buildingId();
		command->intVar1 = static_cast<int32>(tradingCompany.activeResourceEnum);
		command->intVar2 = static_cast<int32>(tradingCompany.isImport);
		command->intVar3 = static_cast<int32>(numberBox->amount);
		networkInterface()->SendNetworkCommand(command);

		// For UI
		tradingCompany.lastTargetAmountSet = numberBox->amount;
	}
	else if (callbackEnum == CallbackEnum::EditableNumberSetOutputTarget)
	{
		auto editableBox = CastChecked<UPunEditableNumberBox>(punWidgetCaller);
		
		auto command = make_shared<FGenericCommand>();
		command->callbackEnum = callbackEnum;
		command->intVar1 = editableBox->callbackVar1;

		int32 amount = editableBox->isEditableNumberActive ? editableBox->amount : -1;;
		command->intVar2 = amount;

		int32 townId = editableBox->callbackVar2;
		command->townId = townId;

		networkInterface()->SendNetworkCommand(command);

		auto& townManager = simulation().townManager(townId);
		townManager.SetOutputTargetDisplay(static_cast<ResourceEnum>(editableBox->callbackVar1), amount);
	}
	else if (callbackEnum == CallbackEnum::EditableNumberSetHotelFeePerVisitor)
	{
		auto editableBox = CastChecked<UPunEditableNumberBox>(punWidgetCaller);

		auto command = make_shared<FGenericCommand>();
		command->callbackEnum = callbackEnum;
		command->intVar1 = editableBox->punId;
		command->intVar2 = editableBox->amount;

		networkInterface()->SendNetworkCommand(command);

		if (Building* bld = simulation().buildingPtr(editableBox->punId)) {
			bld->subclass<Hotel>(CardEnum::Hotel).SetPendingFeePerVisitor(editableBox->amount);
		}
	}

	else if (callbackEnum == CallbackEnum::OpenManageStorage)
	{
		auto overlay = _objectDescriptionUI->ManageStorageOverlay;
		overlay->SetVisibility(overlay->GetVisibility() == ESlateVisibility::Visible ? ESlateVisibility::Collapsed : ESlateVisibility::Visible);
	}
	
	else if (callbackEnum == CallbackEnum::SelectBuildingSlotCard)
	{
		auto cardMini = CastChecked<UBuildingPlacementButton>(punWidgetCaller);
		
		auto command = make_shared<FUnslotCard>();
		command->buildingId = cardMini->callbackVar1;
		command->unslotIndex = cardMini->cardHandIndex;
		networkInterface()->SendNetworkCommand(command);
	}
	else if (callbackEnum == CallbackEnum::QuickBuild)
	{
		auto command = make_shared<FGenericCommand>();
		command->callbackEnum = callbackEnum;
		command->intVar1 = punWidgetCaller->callbackVar1;

		networkInterface()->SendNetworkCommand(command);
	}
	else if (callbackEnum == CallbackEnum::AddAnimalRanch)
	{
		auto command = make_shared<FGenericCommand>();
		command->callbackEnum = callbackEnum;
		command->intVar1 = punWidgetCaller->callbackVar1;

		networkInterface()->SendNetworkCommand(command);
	}
	else if (callbackEnum == CallbackEnum::BudgetAdjust)
	{
		auto adjuster = CastChecked<UPunBudgetAdjuster>(punWidgetCaller);

		if (Building* bld = simulation().buildingPtr(adjuster->buildingId))
		{
			if (adjuster->isBudgetOrTime) {
				bld->lastBudgetLevel = adjuster->level;
			} else {
				bld->lastWorkTimeLevel = adjuster->level;
			}
			
			auto command = make_shared<FGenericCommand>();
			command->callbackEnum = CallbackEnum::BudgetAdjust;
			command->intVar1 = adjuster->buildingId;
			command->intVar2 = adjuster->isBudgetOrTime;
			command->intVar3 = adjuster->level;
			command->intVar4 = dataSource()->isShiftDown();

			networkInterface()->SendNetworkCommand(command);
		}
	}
	else if (callbackEnum == CallbackEnum::TownNameEdit) {
		_objectDescriptionUI->OpenNameEditPopup();
	}
	else if (callbackEnum == CallbackEnum::BuildingSwapArrow) {
		SwitchToNextBuilding(punWidgetCaller->callbackVar1);
	}

	/*
	 * Spy
	 */
	else if (IsSpyCallback(callbackEnum))
	{
		auto command = make_shared<FGenericCommand>();
		command->callbackEnum = callbackEnum;
		command->intVar1 = punWidgetCaller->callbackVar1;

		networkInterface()->SendNetworkCommand(command);
	}

	/*
	 * Unit
	 */
	else if (callbackEnum == CallbackEnum::CaptureUnit)
	{
		auto command = make_shared<FGenericCommand>();
		command->callbackEnum = CallbackEnum::CaptureUnit;
		command->intVar1 = punWidgetCaller->callbackVar1; // objectId
		command->intVar2 = punWidgetCaller->callbackVar2; // birthTicks

		networkInterface()->SendNetworkCommand(command);
	}
}

void UObjectDescriptionUISystem::AddBiomeInfo(WorldTile2 tile, UPunBoxWidget* focusBox)
{
	auto& sim = simulation();
	auto& provinceSys = sim.provinceSystem();
	auto& terrainGenerator = sim.terrainGenerator();

	BiomeEnum biomeEnum = terrainGenerator.GetBiome(tile);

	TArray<FText> args;

	// Don't display Fertility on water
	if (terrainGenerator.terrainTileType(tile) == TerrainTileType::ImpassableFlat) {
		//ADDTEXT_(INVTEXT("<Red>{0}</>\n"), LOCTEXT("Impassable Terrain", "Impassable Terrain"));
		focusBox->AddWGT_PunText(UIEnum::WGT_ObjectFocus_TextLeft, 
			FText::Format(INVTEXT("<Red>{0}</>\n"), LOCTEXT("Impassable Terrain", "Impassable Terrain"))
		);
	}
	else if (IsWaterTileType(terrainGenerator.terrainTileType(tile))) {
		
	}
	else {
		//ADDTEXT_(LOCTEXT("BiomeInfoFertility", "<Bold>Fertility:</> {0}\n"), TEXT_PERCENT(terrainGenerator.GetFertilityPercent(tile)));
		focusBox->AddWGT_TextRow(UIEnum::WGT_ObjectFocus_TextRow,
			LOCTEXT("BiomeInfoFertility", "Fertility"), TEXT_PERCENT(sim.GetFertilityPercent(tile))
		);

		int32 provinceId = provinceSys.GetProvinceIdClean(tile);
		if (provinceId != -1) 
		{
			// Province flat area
			//ADDTEXT_(LOCTEXT("BiomeInfoProvinceFlatArea", "<Bold>Province flat area:</> {0} tiles\n"),
			//	TEXT_NUM(provinceSys.provinceFlatTileCount(provinceSys.GetProvinceIdClean(tile)))
			//);

			// Distance from Townhall
			//int32 provinceDistance = sim.regionSystem().provinceDistanceToPlayer(provinceId, playerId());
			//if (provinceDistance != MAX_int32) {
			//	ADDTEXT_(LOCTEXT("BiomeInfoDescription_Distance", "<Bold>Distance from Townhall:</> {0} provinces\n"),
			//		TEXT_NUM(provinceDistance)
			//	);
			//}
			//ADDTEXT_INV_("\n");
		}
	}

	FloatDet maxCelsius = sim.MaxCelsius(tile);
	FloatDet minCelsius = sim.MinCelsius(tile);


	//ADDTEXT_(INVTEXT("<Bold>{0}</>\n"), LOCTEXT("Temperature", "Temperature"));
	//
	//ADDTEXT_(INVTEXT("  Summer: {0}°C ({1}°F)\n"), TEXT_NUMINT(FDToFloat(maxCelsius)), TEXT_NUMINT(FDToFloat(CelsiusToFahrenheit(maxCelsius))));
	//ADDTEXT_(INVTEXT("  Winter: {0}°C ({1}°F)"), TEXT_NUMINT(FDToFloat(minCelsius)), TEXT_NUMINT(FDToFloat(CelsiusToFahrenheit(minCelsius))));
	//focusBox->AddRichText(args);

	focusBox->AddWGT_TextRow(UIEnum::WGT_ObjectFocus_TextRow,
		LOCTEXT("Temperature", "Temperature"),
		FText::Format(INVTEXT("{0} to {1}°C"), TEXT_NUMINT(FDToFloat(minCelsius)), TEXT_NUMINT(FDToFloat(maxCelsius)))
	);
	focusBox->AddWGT_TextRow(UIEnum::WGT_ObjectFocus_TextRow,
		FText(),
		FText::Format(INVTEXT("{0} to {1}°F"), TEXT_NUMINT(FDToFloat(CelsiusToFahrenheit(minCelsius))), TEXT_NUMINT(FDToFloat(CelsiusToFahrenheit(maxCelsius))))
	);
	
	//ADDTEXT_(
	//	//LOCTEXT("BiomeInfoTemperature", "<Bold>Temperature:</> {0}-{1}°C ({2}-{3}°F)\n"),
	//	INVTEXT(": {0} to {1}°C ({2} to {3}°F)\n"),
	//	TEXT_NUMINT(FDToFloat(minCelsius)),
	//	TEXT_NUMINT(FDToFloat(maxCelsius)),
	//	TEXT_NUMINT(FDToFloat(CelsiusToFahrenheit(minCelsius))),
	//	TEXT_NUMINT(FDToFloat(CelsiusToFahrenheit(maxCelsius)))
	//);
	focusBox->AddSpacer(5);

	if (biomeEnum == BiomeEnum::Jungle) {
		const int32 jungleDiseaseFactor100 = 300;
		focusBox->AddRichText(
			TEXT_TAG("<Disease>", LOCTEXT("Disease Frequency", "Disease Frequency")), 
			TEXT_TAG("<Disease>", FText::Format(LOCTEXT("X per year", "{0} per year"), TEXT_100_2(jungleDiseaseFactor100)))
		);
	} else {
		focusBox->AddWGT_TextRow(UIEnum::WGT_ObjectFocus_TextRow, LOCTEXT("Disease Frequency", "Disease Frequency"), LOCTEXT("1.0 per year", "1.0 per year"));
	}
	
	focusBox->AddRichText(args);
	
	AddBiomeDebugInfo(tile, focusBox);
}

void UObjectDescriptionUISystem::AddBiomeDebugInfo(WorldTile2 tile, UPunBoxWidget* descriptionBox)
{
#if WITH_EDITOR
	if (PunSettings::IsOn("DebugFocusUI"))
	{
		auto& terrainGenerator = simulation().terrainGenerator();

		TArray<FText> args;
		int32 rainFall100 = terrainGenerator.GetRainfall100(tile);
		ADDTEXT_(INVTEXT("Rainfall100 {0}"), TEXT_NUM(rainFall100));
		descriptionBox->AddSpecialRichText(INVTEXT("<Yellow>"), args);
		ADDTEXT_(INVTEXT("Temp10000: {0}"), TEXT_NUM(terrainGenerator.GetTemperatureFraction10000(tile.x, rainFall100)));
		descriptionBox->AddSpecialRichText(INVTEXT("-- "), args);
	}
#endif
}

//void UObjectDescriptionUISystem::AddTileInfo(WorldTile2 tile, UPunBoxWidget* descriptionBox)
//{
//	WorldRegion2 region = tile.region();
//	auto& sim = simulation();
//
//	TArray<FText> args;
//	ADDTEXT_(LOCTEXT("TileInfoHeader", "<Header>{0} Tile</>\n"), sim.terrainGenerator().GetBiomeNameT(tile));
//#if WITH_EDITOR
//	ADDTEXT_(INVTEXT("--Region({0}, {1})\n"), region.x, region.y);
//	ADDTEXT_(INVTEXT("--LocalTile({0}, {1})\n"), tile.localTile(region).x, tile.localTile(region).y)
//#endif
//	ADDTEXT_(INVTEXT("<Header>{0}</>"), tile.ToText());
//	SetText(_objectDescriptionUI->DescriptionUITitle, args);
//	descriptionBox->AddSpacer(12);
//
//	// Biome
//	AddBiomeInfo(tile, descriptionBox);
//
//
//	int32 provinceId = simulation().GetProvinceIdClean(tile);
//	if (provinceId == -1) {
//		return;
//	}
//	
//	descriptionBox->AddSpacer(12);
//
//	// Region
//	//ss << "Region (" << region.x << ", " << region.y << ")";
//	//descriptionBox->AddRichText(ss);
//
//	// Region Owner
//	int32 ownerPlayerId = sim.provinceOwnerPlayer(provinceId);
//	ADDTEXT_(LOCTEXT("TileOwner", "Owner: {0}"), (ownerPlayerId == -1 ? INVTEXT("None") : sim.playerNameT(ownerPlayerId)));
//	descriptionBox->AddRichText(args);
//
//
//	// - Spacer: Tile, Georesource
//	descriptionBox->AddSpacer(12);
//
//	// Georesource
//	AddGeoresourceInfo(provinceId, descriptionBox);
//
//	// Province Upkeep Info
//	AddProvinceUpkeepInfo(provinceId, descriptionBox);
//
//	// - Spacer: Georesource, Claim
//	if (ownerPlayerId == -1) {
//		descriptionBox->AddSpacer();
//		descriptionBox->AddLineSpacer(15);
//	}
//	
//	// Claim land
//	AddSelectStartLocationButton(provinceId, descriptionBox);
//	AddClaimLandButtons(provinceId, descriptionBox);
//
//	// Extras
//#if WITH_EDITOR
//	FVector displayLocation = dataSource()->DisplayLocation(tile.worldAtom2());
//	ADDTEXT_(INVTEXT("Display ({0},{1})"), displayLocation.X, displayLocation.Y);
//	descriptionBox->AddSpecialRichText(INVTEXT("<Yellow>"), args);
//
//	int32 provincePlayerId = sim.provinceOwnerPlayer(provinceId);
//	if (provincePlayerId != -1) {
//		AIPlayerSystem& aiPlayer = sim.aiPlayerSystem(provincePlayerId);
//
//		if (aiPlayer.active()) {
//			AIRegionStatus* regionStatus = aiPlayer.regionStatus(provinceId);
//			if (regionStatus) {
//				ADDTEXT_(INVTEXT("AIRegion ({0}, proposed:{1})"),
//					ToFText(AIRegionPurposeName[static_cast<int>(regionStatus->currentPurpose)]),
//					ToFText(AIRegionProposedPurposeName[static_cast<int>(regionStatus->proposedPurpose)])
//				);
//				descriptionBox->AddSpecialRichText(INVTEXT("<Yellow>"), args);
//			}
//		}
//	}
//#endif
//
//	ShowTileSelectionDecal(dataSource()->DisplayLocation(tile.worldAtom2()));
//	if (sim.tileOwnerPlayer(tile) != playerId()) {
//		ShowRegionSelectionDecal(tile);
//	}
//}

void UObjectDescriptionUISystem::AddImpassableTileInfo(WorldTile2 tile, UPunBoxWidget* focusBox)
{
	if (simulation().IsMountain(tile))
	{
		focusBox->AddWGT_ObjectFocus_Title(LOCTEXT("High Mountain", "High Mountain"));
		focusBox->AddWGT_PunRichText(UIEnum::WGT_ObjectFocus_FlavorText, LOCTEXT("High Mountain Desc", "Impassable unclaimed mountain."));
	}
	else if (simulation().IsWater(tile)) {
		focusBox->AddWGT_ObjectFocus_Title(LOCTEXT("Deep Ocean", "Deep Ocean"));
		focusBox->AddWGT_PunRichText(UIEnum::WGT_ObjectFocus_FlavorText, LOCTEXT("Deep Ocean Desc", "Large body of water."));
	}
	else {
		focusBox->AddWGT_ObjectFocus_Title(LOCTEXT("Unusable Land", "Unusable Land"));
		focusBox->AddWGT_PunRichText(UIEnum::WGT_ObjectFocus_FlavorText, LOCTEXT("Unusable Land Desc", "Land without any use that no one bothers with."));
	}
}

void UObjectDescriptionUISystem::AddProvinceInfo(int32 provinceId, WorldTile2 tile, UPunBoxWidget* focusBox)
{
	if (provinceId == -1)
	{
		SCOPE_CYCLE_COUNTER(STAT_PunUI_Province1);
		AddImpassableTileInfo(tile, focusBox);
		
		return;
	}

	auto& terrainGenerator = simulation().terrainGenerator();

	TArray<FText> args;

	provinceId = abs(provinceId);

	WorldTile2 provinceCenter = simulation().provinceSystem().GetProvinceCenterTile(provinceId);

	// Biome
	{
		SCOPE_CYCLE_COUNTER(STAT_PunUI_ProvinceAddBiomeInfo);

		//ADDTEXT_(INVTEXT("<Header>{0} {1}</>"), terrainGenerator.GetBiomeNameT(provinceCenter), provinceText);
		//SetText(_objectDescriptionUI->DescriptionUITitle, args);
		//focusBox->AddSpacer(12);

		//focusBox->AddWGT_ObjectFocus_Title(FText::Format(
		//	INVTEXT("{0} {1}"),
		//	terrainGenerator.GetBiomeNameT(provinceCenter), provinceText
		//));

		BiomeEnum biomeEnum = simulation().GetBiomeEnum(provinceCenter);
		const BiomeInfo& info = GetBiomeInfo(biomeEnum);
		
		focusBox->AddWGT_ObjectFocus_ProvinceTitle(
			FText::Format(INVTEXT("{0} {1}"), info.name, LOCTEXT("Province", "Province")),
			info.description,
			biomeEnum
		);

		// Biome Description
		//FString wrappedDescription = WrapStringF(terrainGenerator.GetBiomeInfoFromTile(provinceCenter).description.ToString());
		//descriptionBox->AddRichText(FText::FromString(wrappedDescription));

		//focusBox->AddWGT_PunText(
		//	UIEnum::WGT_ObjectFocus_FlavorText, 
		//	terrainGenerator.GetBiomeInfoFromTile(provinceCenter).description
		//);

		//focusBox->AddSpacer(12);

		// Biome number info
		AddBiomeInfo(provinceCenter, focusBox);

		focusBox->AddLineSpacer();
	}

	// Georesource
	AddGeoresourceInfo(provinceId, focusBox);

	// Province Upkeep Info
	AddProvinceUpkeepInfo(provinceId, focusBox);

	// Claim land
	AddSelectStartLocationButton(provinceId, focusBox);
	AddClaimLandButtons(provinceId, focusBox);

	// If no longer in map mode, don't display the selections
	ShowRegionSelectionDecal(provinceCenter);
}

void UObjectDescriptionUISystem::AddProvinceUpkeepInfo(int32 provinceIdClean, UPunBoxWidget* focusBox)
{
	SCOPE_CYCLE_COUNTER(STAT_PunUI_AddProvinceUpkeepInfo);
	
	if (provinceIdClean == -1) {
		return;
	}

	auto& sim = simulation();
	int32 provincePlayerId = sim.provinceOwnerPlayer(provinceIdClean);
	bool unlockedInfluence = sim.unlockedInfluence(playerId());

	Indent(15);
	focusBox->AddWGT_PunText(UIEnum::WGT_ObjectFocus_Subheader, LOCTEXT("Province Info:", "Province Info:"));

	// Already own this province, Show real income/upkeep
	if (provincePlayerId == playerId())
	{	
		focusBox->AddWGT_TextRow(UIEnum::WGT_ObjectFocus_TextRow,
			LOCTEXT("Income", "Income"),
			TEXT_100(sim.GetProvinceIncome100(provinceIdClean)),
			assetLoader()->CoinIcon
		);
		
		
		if (unlockedInfluence) {
			//focusBox->AddRichText(FText::Format(
			//	LOCTEXT("Upkeep: InfluenceX", "Upkeep: <img id=\"Influence\"/>{0}"), 
			//	TEXT_100(sim.GetProvinceUpkeep100(provinceIdClean, provincePlayerId))
			//));

			focusBox->AddWGT_TextRow(UIEnum::WGT_ObjectFocus_TextRow,
				LOCTEXT("Upkeep", "Upkeep"),
				TEXT_100(sim.GetProvinceUpkeep100(provinceIdClean, provincePlayerId)),
				assetLoader()->InfluenceIcon
			);

			if (!sim.provinceInfoSystem().provinceOwnerInfo(provinceIdClean).isSafe) 
			{
				focusBox->AddWGT_TextRow(UIEnum::WGT_ObjectFocus_TextRow,
					GetInfluenceIncomeName(InfluenceIncomeEnum::UnsafeProvinceUpkeep),
					TEXT_100(1000),
					assetLoader()->InfluenceIcon
				);
			}
		}

		// Defense Bonus
		int32 defenseBonus = sim.provinceInfoSystem().provinceOwnerInfo(provinceIdClean).isSafe ? 200 : 0;
		auto widget = focusBox->AddWGT_TextRow(UIEnum::WGT_ObjectFocus_TextRow,
			LOCTEXT("Defense Bonus", "Defense Bonus"),
			TEXT_PERCENT(defenseBonus)
		);
	}
	// Other player's Home Province
	else if (provincePlayerId != -1 && sim.homeProvinceId(provincePlayerId) == provinceIdClean)
	{
		// Defense Bonus
		int32 defenseBonus = sim.provinceInfoSystem().provinceOwnerInfo(provinceIdClean).isSafe ? 200 : 0;
		auto widget = focusBox->AddWGT_TextRow(UIEnum::WGT_ObjectFocus_TextRow,
			LOCTEXT("Defense Bonus", "Defense Bonus"),
			TEXT_PERCENT(defenseBonus)
		);
	}
	else 
	{
		focusBox->AddWGT_TextRow(UIEnum::WGT_ObjectFocus_TextRow,
			LOCTEXT("BaseIncome", "Base Income"),
			TEXT_100(sim.GetProvinceIncome100(provinceIdClean)),
			assetLoader()->CoinIcon
		);
		
		if (unlockedInfluence) {
			focusBox->AddWGT_TextRow(UIEnum::WGT_ObjectFocus_TextRow,
				LOCTEXT("BaseUpkeep", "Base Upkeep"),
				TEXT_100(sim.GetProvinceBaseUpkeep100(provinceIdClean)),
				assetLoader()->InfluenceIcon
			);
		}
		
		// Defense Bonus
		int32 defenseBonus = sim.provinceInfoSystem().provinceOwnerInfo(provinceIdClean).isSafe ? 200 : 0;
		auto widget = focusBox->AddWGT_TextRow(UIEnum::WGT_ObjectFocus_TextRow,
			LOCTEXT("Defense Bonus", "Defense Bonus"),
			TEXT_PERCENT(defenseBonus)
		);
	}


	// Distance from Townhall
	int32 provinceDistance = sim.provinceInfoSystem().provinceDistanceToPlayer(provinceIdClean, playerId());
	if (provinceDistance != MAX_int32) {
		focusBox->AddWGT_TextRow(UIEnum::WGT_ObjectFocus_TextRow,
			LOCTEXT("BiomeInfoDescription_Distance", "Distance to Townhall"),
			FText::Format(LOCTEXT("{0} provinces", "{0} {0}|plural(one=province,other=provinces)"), TEXT_NUM(provinceDistance))
		);
	}

	// Flat Area
	focusBox->AddWGT_TextRow(UIEnum::WGT_ObjectFocus_TextRow,
		LOCTEXT("BiomeInfoDescription_FlatArea", "Flat Area"),
		FText::Format(LOCTEXT("{0} tiles", "{0} tiles"), TEXT_NUM(simulation().provinceSystem().provinceFlatTileCount(provinceIdClean)))
	);

#if !UE_BUILD_SHIPPING
	if (PunSettings::IsOn("DebugFocusUI")) 
	{
		TArray<FText> args;
		ADDTEXT_(INVTEXT("ProvinceId: {0}\n"), TEXT_NUM(provinceIdClean));
		ADDTEXT_(INVTEXT("GetProvinceBaseClaimPrice: {0}\n"), TEXT_NUM(simulation().GetProvinceBaseClaimPrice(provinceIdClean)));
		ADDTEXT_(INVTEXT("GetProvinceClaimCostPenalty_DistanceFromTownhall: {0}\n"), TEXT_NUM(simulation().GetProvinceClaimCostPenalty_DistanceFromTownhall(provinceIdClean, playerId())));
		ADDTEXT_(INVTEXT("GetProvinceClaimPrice: {0}"), TEXT_NUM(simulation().GetProvinceClaimPrice(provinceIdClean, playerId(), ClaimConnectionEnum::Flat)));
		focusBox->AddRichText(JOINTEXT(args));
	}
#endif
	
	ResetIndent();
	
	focusBox->AddSpacer(12);
}

void UObjectDescriptionUISystem::AddGeoresourceInfo(int32 provinceId, UPunBoxWidget* focusBox)
{
	SCOPE_CYCLE_COUNTER(STAT_PunUI_AddGeoresourceInfo);
	
	if (provinceId == -1) {
		return;
	}
	provinceId = abs(provinceId);
	
	auto& simulation = dataSource()->simulation();
	
	// Show georesource for province
	GeoresourceSystem& georesourceSys = simulation.georesourceSystem();
	GeoresourceNode node = georesourceSys.georesourceNode(provinceId);
	ProvinceSystem& provinceSys = simulation.provinceSystem();
	bool isMountain = provinceSys.provinceMountainTileCount(provinceId) > 0;

	bool shouldShowResource = node.HasResource();
	if (node.georesourceEnum == GeoresourceEnum::Oil) {
		shouldShowResource = simulation.IsResearched(playerId(), TechEnum::Petroleum);
	}

	
	// No Georesource and mountain for stone
	if (!shouldShowResource && !isMountain) {
		return;
	}

	focusBox->AddWGT_PunText(UIEnum::WGT_ObjectFocus_Subheader, LOCTEXT("Resources:", "Resources:"));
	
	if (shouldShowResource)
	{
		if (IsFarmGeoresource(node.georesourceEnum))  
		{
			focusBox->AddWGT_TextRow(UIEnum::WGT_ObjectFocus_InventoryEntry,
				GetResourceInfo(node.info().resourceEnum).name,
				LOCTEXT("Available", "Available"),
				assetLoader()->GetGeoresourceIcon(node.info().resourceEnum)
			);
		}
		else {
			focusBox->AddWGT_TextRow_Resource(UIEnum::WGT_ObjectFocus_InventoryEntry,
				GetResourceInfo(node.info().resourceEnum).name,
				TEXT_NUM(node.depositAmount),
				node.info().resourceEnum
			);
			
			//descriptionBox->AddIconPair(FText(), node.info().resourceEnum, TEXT_NUM(node.depositAmount));
		}

		//descriptionBox->AddSpacer();
	}

	if (isMountain)
	{
		focusBox->AddWGT_TextRow_Resource(UIEnum::WGT_ObjectFocus_InventoryEntry,
			GetResourceInfo(ResourceEnum::Stone).name,
			TEXT_NUM(node.stoneAmount),
			ResourceEnum::Stone
		);
	}

	focusBox->AddLineSpacer();
}

void UObjectDescriptionUISystem::AddEfficiencyText(Building& building, UPunBoxWidget* focusBox)
{
	const FText efficiencyText = LOCTEXT("Efficiency", "Efficiency");
	const FText baseText = LOCTEXT("Base", "Base");
	
	auto widget = focusBox->AddWGT_TextRow(UIEnum::WGT_ObjectFocus_TextRow, 
		efficiencyText, TEXT_PERCENT(building.efficiency())
	);

	if (focusBox->IsHovered())
	{
		TArray<FText> args;

		if (building.isEnum(CardEnum::CardMaker))
		{
			ADDTEXT_LOCTEXT("Scholars Office's Efficiency", "Scholars Office's Efficiency increases work speed.");
			ADDTEXT_INV_("<space>");
		}

		ADDTEXT_TAG_("<Bold>", FText::Format(INVTEXT("{0}: {1}%"), efficiencyText, TEXT_NUM(building.efficiency())));
		ADDTEXT_INV_("<space>");
		ADDTEXT_(INVTEXT(" {0}: {1}%"), baseText, TEXT_NUM(building.efficiencyBeforeBonus()));
		ADDTEXT_INV_("<space>");
		ADDTEXT_(INVTEXT(" {0}:"), LOCTEXT("Bonuses", "Bonuses"));

		{
			TArray<FText> args2;
			if (building.adjacentEfficiency() > 0) {
				ADDTEXT(args2, INVTEXT("\n  +{0}% Adjacency Bonus"), TEXT_NUM(building.adjacentEfficiency()));
			}
			if (building.levelEfficiency() > 0) {
				ADDTEXT(args2, INVTEXT("\n  +{0}% Combo Level {1}"), TEXT_NUM(building.levelEfficiency()), TEXT_NUM(building.level()));
			}

			auto bonuses = building.GetBonuses();
			for (BonusPair bonus : bonuses) {
				if (bonus.value > 0) {
					ADDTEXT(args2, INVTEXT("\n  +{0}% {1}"), TEXT_NUM(bonus.value), bonus.name);
				}
			}

			if (args2.Num() == 0) {
				ADDTEXT(args2, INVTEXT(" {0}"), LOCTEXT("None", "None"));
			}

			//ss << ss2.str();
			args.Add(JOINTEXT(args2));
		}

		AddToolTip(widget, args);
	}


	// Job Happiness
	if (building.maxOccupants() > 0) {
		int32 jobHappiness = building.GetJobHappiness();
		focusBox->AddRichText(
			TEXT_TAG("<GrayBold>", LOCTEXT("Job Happiness", "Job Happiness")), ColorHappinessText(jobHappiness, TEXT_PERCENT(jobHappiness))
		);
	}
}

void UObjectDescriptionUISystem::AddTradeFeeText(TradeBuilding& building, UPunBoxWidget* descriptionBox)
{
	const FText tradingFeeText = LOCTEXT("Trade Fee:", "Trade Fee:");
	
	auto feeText = descriptionBox->AddRichText(tradingFeeText, TEXT_PERCENT(building.baseTradingFeePercent()));

	// Tip
	TArray<FText> args;
	ADDTEXT_(INVTEXT("<Bold>{0}</>\n"), tradingFeeText);
	ADDTEXT_(LOCTEXT("base {0}%\n", "base {0}%\n"), TEXT_NUM(building.baseFixedTradingFeePercent()));
	std::vector<BonusPair> bonuses = building.GetTradingFeeBonuses();
	for (const auto& bonus : bonuses) {
		ADDTEXT_(INVTEXT("{0}% {1}\n"), TEXT_NUM(bonus.value), bonus.name);
	}
	AddToolTip(feeText, args);

	// TODO: Resource Fee
	if (simulation().HasTownBonus(building.townId(), CardEnum::DesertTradeForALiving)) {
		descriptionBox->AddRichText(LOCTEXT("DesertTradeForALiving Fee Bonus", "<Gray>0% Fee when trading Food/Wood.</>"));
		descriptionBox->AddSpacer();
	}
	if (simulation().HasTownBonus(building.townId(), CardEnum::DesertOreTrade)) {
		descriptionBox->AddRichText(LOCTEXT("DesertOreTrade Fee Bonus", "<Gray>0% Fee when trading Ores.</>"));
		descriptionBox->AddSpacer();
	}
	descriptionBox->AddSpacer(12);
}


#undef LOCTEXT_NAMESPACE