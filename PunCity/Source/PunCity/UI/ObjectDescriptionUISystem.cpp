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
	SCOPE_CYCLE_COUNTER(STAT_PunUIAll);
	
	if (InterfacesInvalid()) return;

	_objectDescriptionUI->CheckPointerOnUI();

	UpdateDescriptionUI();

	if (dataSource()->simulation().tickCount() == 0) {
		_objectDescriptionUI->CloseAllSubUIs(true);
		_objectDescriptionUI->DescriptionPunBox->ResetBeforeAdd();
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
				bool isPlayerControlled = simulation().provinceOwner(provinceId) == playerId();
				
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
							int32 variationIndex = dataSource()->GetUnitTransformAndVariation(unitId, transform);
							transform.SetScale3D(FVector::OneVector);


							if (unit.unitEnum() == UnitEnum::Human)
							{
								// VariationIndex 0 for human.. TODO: proper code here?
								if (TryMouseCollision(assetLoader()->unitMesh(unit.unitEnum(), 0), transform, shortestHit)) {
									// Human uses USkeletonAsset, should just mark in sim for UnitDisplayComponent to adjust customDepth
									uiState.objectType = ObjectTypeEnum::Unit;
									uiState.objectId = unitId;
								}
							}
							else
							{
								if (TryMouseCollision(assetLoader()->unitMesh(unit.unitEnum(), variationIndex), transform, shortestHit)) {
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
								const ModuleTransforms& modulePrototype = displayInfo.GetDisplayModules(building.buildingEnum(), building.displayVariationIndex());
								std::vector<ModuleTransform> modules = modulePrototype.transforms;

								FTransform transform(FRotator(0, RotationFromDirection(building.faceDirection()), 0), 
															dataSource()->DisplayLocation(building.centerTile().worldAtom2()));

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

	UPunBoxWidget* descriptionBox = _objectDescriptionUI->DescriptionPunBox;

	// Selection Meshes
	meshIndex = 0;

	if (!dataSource()->HasSimulation()) {
		return;
	}

	GameSimulationCore& simulation = dataSource()->simulation();
	DescriptionUIState uiState = simulation.descriptionUIState();

	auto assetLoader = dataSource()->assetLoader();
	
	// Reset the UI if it was swapped
	if (_objectDescriptionUI->state != uiState) {
		_objectDescriptionUI->state = uiState;

		if (uiState.isValid()) {
			_justOpenedDescriptionUI = true;
		}
		_objectDescriptionUI->CloseAllSubUIs(uiState.shouldCloseStatUI);
		descriptionBox->ResetBeforeAdd();
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

	auto& resourceSys = simulation.resourceSystem(playerId());
	

	if (uiState.objectType == ObjectTypeEnum::None) {
		_objectDescriptionUI->SetVisibility(ESlateVisibility::Collapsed);
	}
	else
	{	
		_objectDescriptionUI->state = uiState; // For Click/Checkbox
		_objectDescriptionUI->parent = this;

		int32 objectId = uiState.objectId;

		_objectDescriptionUI->BuildingsStatOpener->SetVisibility(ESlateVisibility::Collapsed);
		_objectDescriptionUI->NameEditButton->SetVisibility(ESlateVisibility::Collapsed);


		//! TileBuilding (such as Building)
		// TODO: may be TileBuilding should just be TileObject..???
		if (uiState.objectType == ObjectTypeEnum::Building && objectId < 0) {
			// negative objectId in building means Fence or other TileObject
			WorldTile2 tile(-objectId);
			check(tile.isValid());

			SetText(_objectDescriptionUI->DescriptionUITitle, "Fence");
			descriptionBox->AfterAdd();

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
			
			//stringstream ss;
			TArray<FText> args;
			
			/*
			 * Header
			 */
			if (buildingEnum == CardEnum::RegionTribalVillage) {
				//ss << "<Header>" << GenerateTribeName(objectId) << " tribe</>";
				ADDTEXT_(LOCTEXT("TribeName", "<Header>{0} tribe</>"), ToFText(GenerateTribeName(objectId)));
			}
			else if (buildingEnum == CardEnum::Townhall) {
				//ss << "<Header>" << TrimString_Dots(simulation.townName(building.playerId()), 15) << "</>";
				//ADDINVTEXT(args, "<Header>{0}</>", ToFText(TrimString_Dots(simulation.townName(building.playerId()), 15)));
				ADDTEXT_(INVTEXT("<Header>{0}</>"), ToFText(TrimString_Dots(simulation.townName(building.playerId()), 15)));
			}
			else if (buildingEnum == CardEnum::House) {
				//ss << "<Header>House Level " << building.subclass<House>().houseLvl() << "</>";
				ADDTEXT_(LOCTEXT("HouseName", "<Header>House Level {0}</>"), FText::AsNumber(building.subclass<House>().houseLvl()));
			}
			else
			{
				//ss << "<Header>" << building.buildingInfo().name << "</>";
				//ADDINVTEXT(args, "<Header>{buildingName}</>", building.buildingInfo().GetName());
				ADDTEXT_(INVTEXT("<Header>{buildingName}</>"), building.buildingInfo().GetName());
			}
			
#if WITH_EDITOR || TRAILER_MODE
			ADDTEXT_(INVTEXT("[{objectId}]"), FText::AsNumber(objectId));
			ADDTEXT_(INVTEXT("\n{centerTile}"), FText::FromString(building.centerTile().To_FString()));
			//ss << "[" << objectId << "]"; // ID
			//ss << "\n" << building.centerTile().ToString();
#endif

			
			//SetText(_objectDescriptionUI->DescriptionUITitle, ss);
			SetText(_objectDescriptionUI->DescriptionUITitle, FText::Join(FText(), args));
			args.Empty();

			
			
			_objectDescriptionUI->BuildingsStatOpener->SetVisibility(building.maxOccupants() > 0 && !IsHouse(building.buildingEnum()) ? ESlateVisibility::Visible : ESlateVisibility::Collapsed);
			_objectDescriptionUI->NameEditButton->SetVisibility((building.isEnum(CardEnum::Townhall) && building.playerId() == playerId()) ? ESlateVisibility::Visible : ESlateVisibility::Collapsed);
			
			//descriptionBox->AddRichText(ss);
			descriptionBox->AddLineSpacer(8);

			
#if WITH_EDITOR
			// IsConnectedBuilding
			if (playerId() != -1) {
				//ss << simulation.IsConnectedBuilding(building.buildingId(), playerId());
				ADDTEXT_NUMBER(args, simulation.IsConnectedBuilding(building.buildingId(), playerId()))
				descriptionBox->AddRichText(FTEXT("<Yellow>IsConnectedBld</>"), args);
			}
#endif


			// Show warning
			building.TryRefreshHoverWarning(UGameplayStatics::GetTimeSeconds(this));
			if (building.hoverWarning != HoverWarning::None)
			{
				//std::string description = GetHoverWarningDescription(building.hoverWarning);
				//if (description.size() > 0) {
				//	FString descriptionText = ToFString(description);
				//	descriptionText = descriptionText.Replace(TEXT("\n"), TEXT("</>\n<Red>"));
				//	std::string descriptionStr = ToStdString(descriptionText);
				//	descriptionBox->AddRichText("<Red>" + descriptionStr + "</>");
				//}
				FText description = GetHoverWarningDescriptionText(building.hoverWarning);
				if (!description.IsEmpty()) {
					FString descriptionStr = description.ToString().Replace(TEXT("\n"), TEXT("</>\n<Red>"));
					descriptionBox->AddRichTextF("<Red>" + descriptionStr + "</>");
				}
			}
			

			// Show Description for some building
			if (building.isEnum(CardEnum::Bank) ||
				building.isEnum(CardEnum::ShippingDepot) || 
				building.isEnum(CardEnum::Market) || 
				building.isEnum(CardEnum::TradingCompany))
			{
				//ss << WrapString(building.buildingInfo().description);
				descriptionBox->AddRichTextF(WrapStringF(building.buildingInfo().GetDescription().ToString())); // WrapString helps prevent flash
				descriptionBox->AddSpacer(8);
			}

			

			// Helpers:
			TileArea area = building.area();
			FVector displayLocation = dataSource()->DisplayLocation(building.centerTile().worldAtom2());
			AlgorithmUtils::ShiftDisplayLocationToTrueCenter(displayLocation, area, building.faceDirection());

			/*
			 * Body
			 */
			if (building.isOnFire())
			{
				//ss << "<Red>Building is on fire.</>";
				//descriptionBox->AddRichText(ss);
				descriptionBox->AddRichText(LOCTEXT("BuildingOnFire", "<Red>Building is on fire.</>"));
			}
			else if (building.isBurnedRuin())
			{
				//ss << "<Red>Burned ruin.</>";
				descriptionBox->AddRichText(LOCTEXT("BurnedRuin", "<Red>Burned ruin.</>"));
			}
			else
			{

				if (!building.isConstructed()) {
					//ss << "Under construction\n";
					descriptionBox->AddRichText(LOCTEXT("UnderConstruction", "Under construction\n"));
				}

				// Occupants
				if (IsHouse(building.buildingEnum()) && building.isConstructed()) 
				{
					//ss << "<img id=\"House\"/>" << building.occupantCount() << "/" << building.allowedOccupants(); // House's allowedOccupants
					//descriptionBox->AddRichText("Occupants", ss);
					
					ADDTEXT(args, INVTEXT("<img id=\"House\"/>{0}/{1}"),
						FText::AsNumber(building.occupantCount()), 
						FText::AsNumber(building.allowedOccupants())
					);
					descriptionBox->AddRichText(LOCTEXT("Occupants", "Occupants"), args);

#if WITH_EDITOR
					//ss << building.adjacentCount(CardEnum::House);
					//descriptionBox->AddRichText("<Yellow>Adjacent</>", ss);
					descriptionBox->AddRichText(FText::Join(FText(), INVTEXT("<Yellow>"), LOCTEXT("HouseAdjacent", "Adjacent"), INVTEXT("</>")));
#endif					
				}
				else if (building.maxOccupants() > 0) {
					//ss << building.occupantCount() << "/" << building.maxOccupants();
					ADDTEXT(args, INVTEXT("{0}/{1}"),
						FText::AsNumber(building.occupantCount()),
						FText::AsNumber(building.maxOccupants())
					);
					descriptionBox->AddRichText(LOCTEXT("Workers_C", "Workers: "), args);
				}

				// Upkeep
				int32 upkeep = building.upkeep();
				if (building.isConstructed() && upkeep > 0)
				{
					//ss << "<img id=\"Coin\"/>" << upkeep;
					descriptionBox->AddRichText(
						LOCTEXT("Upkeep_C", "Upkeep: "), FText::Join(FText(), INVTEXT("<img id=\"Coin\"/>"), FText::AsNumber(upkeep))
					);
				}

				

				// Upgrade Level / Appeal / tax
				if (building.isConstructed())
				{

					// Main
					if (IsHumanHouse(building.buildingEnum())) 
					{
						House* house = static_cast<House*>(&building);
						
						descriptionBox->AddSpacer(10);

						int32 houseLvl = house->houseLvl();
						int32 appealNeeded = house->GetAppealUpgradeRequirement(houseLvl + 1);

						// Heating
						{
							args.Empty();
							
							//auto widgetCoal = descriptionBox->AddRichText("Coal Heating Efficiency", to_string(house->GetHeatingEfficiency(ResourceEnum::Coal)) + "%");
							//house->GetHeatingEfficiencyTip(ss, ResourceEnum::Coal);
							//AddToolTip(widgetCoal, ss);

							auto widgetCoal = descriptionBox->AddRichText(
								LOCTEXT("CoalHeatingEfficiency", "Coal Heating Efficiency"), TEXT_PERCENT(house->GetHeatingEfficiency(ResourceEnum::Coal))
							);
							house->GetHeatingEfficiencyTip(args, ResourceEnum::Coal);
							AddToolTip(widgetCoal, args);
							
							//auto widgetWood = descriptionBox->AddRichText("Wood Heating Efficiency", to_string(house->GetHeatingEfficiency(ResourceEnum::Wood)) + "%");
							//house->GetHeatingEfficiencyTip(args, ResourceEnum::Wood);
							//AddToolTip(widgetWood, ss);

							auto widgetWood = descriptionBox->AddRichText(
								LOCTEXT("WoodHeatingEfficiency", "Wood Heating Efficiency"), TEXT_PERCENT(house->GetHeatingEfficiency(ResourceEnum::Wood))
							);
							house->GetHeatingEfficiencyTip(args, ResourceEnum::Wood);
							AddToolTip(widgetWood, args);
						}
						descriptionBox->AddSpacer(10);
						
						descriptionBox->AddRichText(LOCTEXT("Appeal", "Appeal"), TEXT_PERCENT(house->GetAppealPercent()));

						// Happiness
						{
							//ss << house->housingHappiness() << "%";
							auto widget = descriptionBox->AddRichText(
								LOCTEXT("Housingquality", "Housing quality"), TEXT_PERCENT(house->housingHappiness())
							);
							descriptionBox->AddSpacer();

							//ss << "Happiness: " << house->totalHappiness() << "<img id=\"Smile\"/>\n";
							//ss << " +" << GameConstants::BaseHappiness << " New city optimism\n";
							//ss << " +" << house->appealHappiness() << " Appeal\n";
							//ss << " +" << house->luxuryHappiness() << " Luxury\n";
							//ss << " +" << house->tavernHappiness() << " Tavern";
							//AddToolTip(widget, ss);
						}

						descriptionBox->AddSpacer(5);

						// Tax
						{
							ADDTEXT_(INVTEXT("{0}<img id=\"Coin\"/>\n"), TEXT_100(house->totalHouseIncome100()));
							auto widget = descriptionBox->AddRichText(LOCTEXT("Income", "Income"), args);
							descriptionBox->AddSpacer();

							// Tooltip
							// Single house tax...
							ADDTEXT_(LOCTEXT("Income_C", "Income: {0}<img id=\"Coin\"/>\n"), TEXT_100(house->totalHouseIncome100()));

							for (int32 i = 0; i < HouseIncomeEnumCount; i++) {
								int32 income100 = house->GetIncome100Int(i);
								if (income100 != 0) {
									//ss << (income100 > 0 ? INVTEXT(" +") : INVTEXT(" ")) << (income100 / 100.0f) << " " << IncomeEnumName[i] << "\n";
									ADDTEXT_JOIN_(INVTEXT(" "), TEXT_100SIGNED(income100), INVTEXT(" "), GetIncomeEnumName(i), INVTEXT("\n"));
								}
							}

							AddToolTip(widget, args);


							// TODO: OLD
							//ss << fixed << setprecision(1);
							//ss << house->totalHouseIncome100() / 100.0f << "<img id=\"Coin\"/>";
							//auto widget = descriptionBox->AddRichText("Income", ss);
							//descriptionBox->AddSpacer();

							// Tooltip
							// Single house tax...
							//ss << "Income: " << house->totalHouseIncome100() / 100.0f << "<img id=\"Coin\"/>\n";

							//for (int32 i = 0; i < HouseIncomeEnumCount; i++) {
							//	int32 income100 = house->GetIncome100Int(i);
							//	if (income100 != 0) {
							//		ss << (income100 > 0 ? " +" : " ") << (income100 / 100.0f) << " " << IncomeEnumName[i] << "\n";
							//	}
							//}

							//AddToolTip(widget, ss);
						}

						// Science
						{
							ADDTEXT_(INVTEXT("{0}<img id=\"Science\"/>\n"), TEXT_100(house->science100PerRound()));
							auto widget = descriptionBox->AddRichText(LOCTEXT("Science", "Science"), args);
							descriptionBox->AddSpacer();

							// Tooltip
							ADDTEXT_(LOCTEXT("ScienceOutput_C", "Science output: {0}<img id=\"Science\"/>\n"), TEXT_100(house->science100PerRound()));

							for (int32 i = 0; i < HouseScienceEnums.size(); i++) {
								int32 science100 = house->GetScience100(HouseScienceEnums[i]);
								if (science100 != 0) {
									//ss << (science100 > 0 ? " +" : " ") << science100 / 100.0f << " " << ScienceEnumName[static_cast<int>(HouseScienceEnums[i])] << "\n";
									ADDTEXT_JOIN_(INVTEXT(" "), TEXT_100SIGNED(science100), INVTEXT(" "), ScienceEnumName(static_cast<int>(HouseScienceEnums[i])), INVTEXT("\n"));
								}
							}

							AddToolTip(widget, args);
							
							//ss << fixed << setprecision(1);
							//ss << house->science100PerRound() / 100.0f << "<img id=\"Science\"/>";
							//auto widget = descriptionBox->AddRichText("Science", ss);
							//descriptionBox->AddSpacer();

							//ss << "Science output: " << house->science100PerRound() / 100.0f << "<img id=\"Science\"/>\n";

							//for (int32 i = 0; i < HouseScienceEnums.size(); i++) {
							//	int32 science100 = house->GetScience100(HouseScienceEnums[i]);
							//	if (science100 != 0) {
							//		ss << (science100 > 0 ? " +" : " ") << science100 / 100.0f << " " << ScienceEnumName[static_cast<int>(HouseScienceEnums[i])] << "\n";
							//	}
							//}
							//
							//AddToolTip(widget, ss);
						}

						if (house->ownedBy(playerId()))
						{
							descriptionBox->AddLineSpacer();
							descriptionBox->AddSpacer();
							//ss << "<Header>Level " << houseLvl << "</>";
							//descriptionBox->AddRichText(ss);

							if (houseLvl == house->GetMaxHouseLvl()) {
								//ss << "Maximum lvl";
								descriptionBox->AddRichText(LOCTEXT("Maximum lvl", "Maximum lvl"));
							}
							else if (house->GetAppealPercent() < appealNeeded) {
								//ss << "<Orange>Need " + to_string(appealNeeded) + " appeal to increase lvl</>";
								ADDTEXT_(LOCTEXT("HouseNeedAppeal", "<Orange>Need {0} appeal to increase lvl</>"), FText::AsNumber(appealNeeded));
								descriptionBox->AddRichText(args);
							}
							else {
								//ss << "<Orange>" + house->HouseNeedDescription() + "</>\n<Orange>(to increase level)</>";
								ADDTEXT_(INVTEXT("<Orange>{0}</>\n"), FText::AsNumber(appealNeeded));
								ADDTEXT_LOCTEXT("HouseToIncreaseLevel", "<Orange>(to increase level)</>");
								auto widget = descriptionBox->AddRichText(args);
								AddToolTip(widget, house->HouseNeedTooltipDescription());
							}

							descriptionBox->AddSpacer();

							auto addCheckBoxIconPair = [&](ResourceEnum resourceEnum) {
								UIconTextPairWidget* widget = descriptionBox->AddIconPair(FText(), resourceEnum, TEXT_NUM(building.resourceCount(resourceEnum)));
								widget->UpdateAllowCheckBox(resourceEnum);
								widget->ObjectId = building.buildingId();
							};

							//ADDTEXT_(INVTEXT("<Bold>{0}</>"), LOCTEXT("Fuel_C", "Fuel:"))
							ADDTEXT_TAG_("<Bold>", LOCTEXT("Fuel_C", "Fuel:"));
							descriptionBox->AddRichText(args);
							addCheckBoxIconPair(ResourceEnum::Wood);
							addCheckBoxIconPair(ResourceEnum::Coal);

							ADDTEXT_TAG_("<Bold>", LOCTEXT("LuxuryTier1_C", "Luxury tier 1:"));
							descriptionBox->AddRichText(args);
							const std::vector<ResourceEnum>& luxury1 = GetLuxuryResourcesByTier(1);
							for (size_t i = 0; i < luxury1.size(); i++) {
								addCheckBoxIconPair(luxury1[i]);
							}

							if (houseLvl >= 3)
							{
								ADDTEXT_TAG_("<Bold>", LOCTEXT("LuxuryTier2_C", "Luxury tier 2:"));
								descriptionBox->AddRichText(args);
								const std::vector<ResourceEnum>& luxury2 = GetLuxuryResourcesByTier(2);
								for (size_t i = 0; i < luxury2.size(); i++) {
									addCheckBoxIconPair(luxury2[i]);
								}
							}
							if (houseLvl >= 5)
							{
								ADDTEXT_TAG_("<Bold>", LOCTEXT("LuxuryTier3_C", "Luxury tier 3:"));
								descriptionBox->AddRichText(args);
								const std::vector<ResourceEnum>& luxury = GetLuxuryResourcesByTier(3);
								for (size_t i = 0; i < luxury.size(); i++) {
									addCheckBoxIconPair(luxury[i]);
								}
							}
						}
					}
					else if (building.isEnum(CardEnum::Townhall))
					{
						TownHall& townhall = building.subclass<TownHall>();
						int32 townPlayerId = townhall.playerId();
						auto& townhallPlayerOwned = simulation.playerOwned(townPlayerId);
						int32 lvl = townhall.townhallLvl;

						FString nameTrimmed = TrimStringF_Dots(simulation.playerNameF(townhall.playerId()), 12);
						descriptionBox->AddRichText(LOCTEXT("Player", "Player"), FText::FromString(nameTrimmed));

#if WITH_EDITOR
						descriptionBox->AddRichText(TEXT_TAG("<Yellow>", INVTEXT("PlayerId")), FText::AsNumber(townhall.playerId()));
#endif
						
						descriptionBox->AddRichText(LOCTEXT("Size", "Size"), townhallPlayerOwned.GetTownSizeName());

						descriptionBox->AddRichText(LOCTEXT("TownLevel", "Town Level"), FText::AsNumber(lvl));

						// Only show these if it is player's townhall
						if (townPlayerId == playerId()) {
							descriptionBox->AddRichText(
								LOCTEXT("TownhallIncome", "Townhall Income"), 
								FText::Join(FText(), INVTEXT("<img id=\"Coin\"/>"), FText::AsNumber(townhall.townhallIncome())) // TODO:
							);
						}

						// Show how much money he has..
						if (townPlayerId != playerId())
						{
							ADDTEXT_(INVTEXT("<img id=\"Coin\"/>{0}"), FText::AsNumber(simulation.resourceSystem(townPlayerId).money()));
							descriptionBox->AddRichText(LOCTEXT("Money", "Money"), args);

							ADDTEXT_(INVTEXT("<img id=\"Coin\"/>{0}"), TEXT_100(townhallPlayerOwned.totalIncome100()));
							descriptionBox->AddRichText(LOCTEXT("Money Income", "Money Income"), args);
							descriptionBox->AddSpacer();

							ADDTEXT_(INVTEXT("<img id=\"Influence\"/>{0}"), FText::AsNumber(simulation.resourceSystem(townPlayerId).influence()));
							descriptionBox->AddRichText(LOCTEXT("Influence", "Influence"), args);
							ADDTEXT_(INVTEXT("<img id=\"Influence\"/>{0}"), TEXT_100(townhallPlayerOwned.totalInfluenceIncome100()));
							descriptionBox->AddRichText(LOCTEXT("Influence Income", "Influence Income"), args);
							descriptionBox->AddSpacer();
							
							ADDTEXT_(INVTEXT("<img id=\"Smile\"/>{0}"), FText::AsNumber(simulation.GetAverageHappiness(townPlayerId)));
							descriptionBox->AddRichText(LOCTEXT("Happiness", "Happiness"), args);
						}

						descriptionBox->AddSpacer(10);

						// Lord
						if (townhallPlayerOwned.lordPlayerId() != -1) {
							descriptionBox->AddRichText("Lord", simulation.playerName(townhallPlayerOwned.lordPlayerId()));
						} else {
							descriptionBox->AddRichText("Independent State"); // TODO: many vassal => empire
						}

						// Allies
						const auto& allyPlayerIds = townhallPlayerOwned.allyPlayerIds();
						if (allyPlayerIds.size() > 0)
						{
							// Expanded text part
							for (int32 i = 1; i < allyPlayerIds.size(); i++) {
								ADDTEXT_(INVTEXT("{0}\n"), FText::FromString(simulation.townhall(allyPlayerIds[i]).townFName()));
							}
							
							descriptionBox->AddRichText(LOCTEXT("Allies", "Allies"), 
								simulation.townhall(allyPlayerIds[0]).townTName(), ResourceEnum::None, JOINTEXT(args)
							);
							args.Empty();
						} else {
							descriptionBox->AddRichText(LOCTEXT("Allies", "Allies"), LOCTEXT("None", "None"));
						}

						// Vassals
						const auto& vassalBuildingIds = townhallPlayerOwned.vassalBuildingIds();
						{
							TArray<FText> vassalArgs;

							auto getVassalName = [&](int32 vassalBuildingId)
							{
								Building& vassalBld = simulation.building(vassalBuildingId);
								if (vassalBld.isEnum(CardEnum::Townhall)) {
									return simulation.townhall(vassalBld.playerId()).townTName();
								}
								else {
									return LOCTEXT("Non-player Vassal", "Non-player Vassal");
								}
							};
							
							for (int32 i = 1; i < vassalBuildingIds.size(); i++) {
								ADDTEXT(vassalArgs, INVTEXT("{0}\n"), getVassalName(vassalBuildingIds[i]));
							}
							
							if (vassalBuildingIds.size() > 0) {
								descriptionBox->AddRichText(LOCTEXT("Vassals", "Vassals"), getVassalName(vassalBuildingIds[0]), ResourceEnum::None, JOINTEXT(vassalArgs));
							} else {
								descriptionBox->AddRichText(LOCTEXT("Vassals", "Vassals"), LOCTEXT("None", "None"));
							}
							
						}

						// Note: Statistics replaced with Statistics Bureau
						//descriptionBox->AddButton("Show Statistics", nullptr, "", this, CallbackEnum::OpenStatistics, true, false, townhall.playerId());
						descriptionBox->AddLineSpacer(10);

						if (building.playerId() == playerId())
						{
							//descriptionBox->AddButton("Set Trade Offers", nullptr, "", this, CallbackEnum::OpenSetTradeOffers, true, false, townhall.playerId());
							//descriptionBox->AddLineSpacer(10);

							// Abandon town
							auto button = descriptionBox->AddButtonRed(FText(), LOCTEXT("AbandonTown", "Abandon Town"), nullptr, FText(), this, CallbackEnum::AbandonTown);
							AddToolTip(button, LOCTEXT("AbandonTownTooltip", "Abandon this town to build a new one. (Destroy this town)"));
							descriptionBox->AddLineSpacer(10);
						}
						else
						{
							// Vassalize
							if (simulation.IsResearched(playerId(), TechEnum::Vassalize))
							{

								
							}
						}

					}
					else if (building.isEnum(CardEnum::Bank) ||
							building.isEnum(CardEnum::InvestmentBank)) 
					{	
						ADDTEXT_(INVTEXT("{0}<img id=\"Coin\"/>"), FText::AsNumber(static_cast<Bank*>(&building)->lastRoundProfit));
						descriptionBox->AddRichText(LOCTEXT("Round profit", "Round profit"), args);
					}
					else if (building.isEnum(CardEnum::IronSmelter)) {
						//if (simulation.playerParameters(playerId())->SmelterAdjacencyBonus) {
						//	ss << "\nBonus: +" << building.adjacentCount(BuildingEnum::IronSmelter) * 10 << "% productivity\n\n";
						//}
					}
					else if (building.isEnum(CardEnum::Farm)) 
					{
						Farm& farm = building.subclass<Farm>();

						int32 fertility = farm.fertility();
						descriptionBox->AddRichText(LOCTEXT("Fertility", "Fertility"), TEXT_PERCENT(fertility));

						AddEfficiencyText(farm, descriptionBox);

						descriptionBox->AddRichText(LOCTEXT("Work stage", "Work stage"), farm.farmStageName());
						descriptionBox->AddRichText(LOCTEXT("Growth percent", "Growth percent"), TEXT_PERCENT(farm.MinCropGrowthPercent()));

						//if (static_cast<Farm*>(&building)->hasAdjacencyBonus) {
						//	ss << "Bonus: +" << building.adjacentCount(BuildingEnum::Farm) * 10 << "% productivity\n\n";
						//}
#if WITH_EDITOR
						ADDTEXT_(INVTEXT("(Editor)WorkedTiles: {0}\n"), TEXT_NUM(farm.workedTiles()));
						ADDTEXT_(INVTEXT("(Editor)adjacents: +{0}\n"), TEXT_NUM(building.adjacentCount()));
						descriptionBox->AddSpecialRichText(INVTEXT("<Yellow>"), args);
#endif
						descriptionBox->AddRichText("Current", GetTileObjInfo(farm.currentPlantEnum).name);
					}
					//// TODO: delete this??
					//else if (building.isEnum(CardEnum::RanchBarn))
					//{
					//	RanchBarn& barn = building.subclass<RanchBarn>();
					//	ss << "\n" << barn.inventory.ToString();
					//	ss << "\nAnimals: " << barn.animalOccupants().size() << "/" << barn.maxAnimals << "\n";
					//	descriptionBox->AddRichText(ss);

					//	// Selection Meshes
					//	const std::vector<int32_t>& occupants = barn.animalOccupants();
					//	for (int occupantId : occupants) {
					//		WorldAtom2 atom = dataSource()->unitDataSource().actualAtomLocation(occupantId);
					//		SpawnSelectionMesh(assetLoader->SelectionMaterialYellow, dataSource()->DisplayLocation(atom) + FVector(0, 0, 30));
					//	}
					//}
					else if (IsRanch(building.buildingEnum()))
					{
						Ranch& ranch = building.subclass<Ranch>();
						ADDTEXT_(INVTEXT("{0}/{1}"), TEXT_NUM(ranch.animalOccupants().size()), TEXT_NUM(ranch.maxAnimals));
						descriptionBox->AddRichText(LOCTEXT("Animals:", "Animals:"), args);

						// Selection Meshes
						const std::vector<int32_t>& occupants = ranch.animalOccupants();
						for (int occupantId : occupants) {
							WorldAtom2 atom = dataSource()->unitDataSource().actualAtomLocation(occupantId);
							SpawnSelectionMesh(assetLoader->SelectionMaterialYellow, dataSource()->DisplayLocation(atom) + FVector(0, 0, 30));
						}
					}
					// TODO: Boar should just use the normal inventory?
					else if (building.isEnum(CardEnum::BoarBurrow)) {
						ADDTEXT_(INVTEXT("{0}\n"), FText::FromString(WrapStringF(building.buildingInfo().GetDescription().ToString())));
						ADDTEXT__(building.subclass<BoarBurrow>().inventory.ToText());
						descriptionBox->AddRichText(args);
					}

					else if (IsSpecialProducer(building.buildingEnum())) 
					{
						AddEfficiencyText(building, descriptionBox);

						if (building.isEnum(CardEnum::Mint)) {
							ADDTEXT_(INVTEXT("<img id=\"Coin\"/>{0}"), TEXT_NUM(building.seasonalProduction()));
							descriptionBox->AddRichText(LOCTEXT("Income(per season)", "Income(per season)"), args);
						}
						if (building.isEnum(CardEnum::InventorsWorkshop)) {
							ADDTEXT_(INVTEXT("<img id=\"Science\"/>{0}"), TEXT_NUM(building.seasonalProduction()));
							descriptionBox->AddRichText(LOCTEXT("Science(per season)", "Science(per season)"), args);
						}
						if (building.isEnum(CardEnum::RegionShrine)) {
							ADDTEXT_(INVTEXT("<img id=\"Science\"/>{0}"), TEXT_NUM(building.seasonalProduction()));
							descriptionBox->AddRichText(LOCTEXT("Science(per season)", "Science(per season)"), args);
						}
						if (IsBarrack(building.buildingEnum())) {
							ADDTEXT_(INVTEXT("<img id=\"Influence\"/>{0}"), TEXT_NUM(building.seasonalProduction()));
							descriptionBox->AddRichText(LOCTEXT("Influence(per season)", "Influence(per season)"), args);
						}
					}
					else if (IsStorage(building.buildingEnum()))
					{
						if (!building.isEnum(CardEnum::Market))
						{
							StorageYard& storage = building.subclass<StorageYard>();
							ADDTEXT_(INVTEXT("{0}/{1}"), TEXT_NUM(storage.tilesOccupied()), TEXT_NUM(storage.storageSlotCount()));
							descriptionBox->AddRichText(LOCTEXT("Slots", "Slots"), args);

							descriptionBox->AddSpacer(12);
						}
					}
					else if (building.isEnum(CardEnum::Tavern) || 
							building.isEnum(CardEnum::Theatre))
					{
						FunBuilding& funBuilding = building.subclass<FunBuilding>();
						int32 guestCount = funBuilding.guestCountLastRound() + funBuilding.guestCountThisRound();
						descriptionBox->AddRichText(LOCTEXT("Guests: ", "Guests: "), TEXT_NUM(guestCount));

						descriptionBox->AddRichText(LOCTEXT("Service Quality: ", "Service Quality: "), TEXT_NUM(funBuilding.serviceQuality()));
//#if WITH_EDITOR 
//						ss << "guestCountInternal: " << funBuilding.guestCountInternal() << "\n";
//						ss << "guestCountLastRound: " << funBuilding.guestCountLastRound() << "\n";
//						ss << "guestCountLastLastRound: " << funBuilding.guestCountLastLastRound() << "\n";
//						ss << "guestReservations: " << funBuilding.guestReservations() << "\n";
//						descriptionBox->AddSpecialRichText("<Yellow>", ss);
//#endif
					}
					else if (building.isEnum(CardEnum::Library) ||
							building.isEnum(CardEnum::School)) 
					{
						descriptionBox->AddSpacer();
						descriptionBox->AddRichText(building.buildingInfo().GetDescription());
					}
					else if (building.isEnum(CardEnum::OreSupplier)) 
					{
						descriptionBox->AddRichText(building.buildingInfo().GetDescription());
						descriptionBox->AddLineSpacer();

						int32 targetBuyAmount = building.subclass<OreSupplier>().maxBuyAmount();
						descriptionBox->AddRichText(LOCTEXT("Resource Type", "Resource Type"), LOCTEXT("Iron ore", "Iron ore"));
						descriptionBox->AddRichText(LOCTEXT("Target Buy Amount", "Target Buy Amount"), TEXT_NUM(targetBuyAmount));
					}
					else if (IsTradingPostLike(building.buildingEnum()))
					{
						if (building.isConstructed())
						{
							auto tradingPost = static_cast<TradingPost*>(&building);
							int32 countdown = tradingPost->CountdownSecondsDisplay();

							//descriptionBox->AddRichText("Trade fee:", to_string(tradingPost->tradingFeePercent()) + "%");

							AddTradeFeeText(building.subclass<TradeBuilding>(), descriptionBox);
							AddEfficiencyText(building, descriptionBox);

#if WITH_EDITOR 
							ADDTEXT_(INVTEXT("-- Ticks from last check: {0} ticks"), TEXT_NUM(tradingPost->ticksFromLastCheck()));
							descriptionBox->AddRichText(args);
							ADDTEXT_(INVTEXT("-- lastCheckTick: {0} secs"), TEXT_NUM(tradingPost->lastCheckTick()));
							descriptionBox->AddRichText(args);
#endif
							ADDTEXT_(LOCTEXT("{0} secs", "{0} secs"), TEXT_NUM(countdown));
							descriptionBox->AddRichText(LOCTEXT("Trade complete in", "Trade complete in"), args);

							ButtonStateEnum buttonState = ButtonStateEnum::Enabled;
							if (building.playerId() != playerId()) {
								buttonState = ButtonStateEnum::Hidden;
							}
							else if (!tradingPost->CanTrade()) {
								buttonState = ButtonStateEnum::Disabled;
							}
							SetButtonEnabled(_objectDescriptionUI->TradeButton, buttonState);
						}
					}
					else if (building.isEnum(CardEnum::TradingCompany))
					{
						auto& tradingCompany = building.subclass<TradingCompany>();

						// Shouldn't be able to tamper with other ppl's trade
						if (building.ownedBy(playerId()))
						{

							// Display status only if the trade type was chosen...
							if (tradingCompany.activeResourceEnum != ResourceEnum::None)
							{
								descriptionBox->AddRichText("Maximum trade per round", to_string(tradingCompany.tradeMaximumPerRound()));

								//auto feeText = descriptionBox->AddRichText("Trade fee:", to_string(tradingCompany.tradingFeePercent()) + "%");
								//std::stringstream feeTip;
								//feeTip << "Trading fee: " << tradingCompany.tradingFeePercent() << "%\n";
								//feeTip << "base " << tradingCompany.baseTradingFeePercent() << "%\n";
								//std::vector<BonusPair> bonuses = tradingCompany.GetTradingFeeBonuses();
								//for (const auto& bonus : bonuses) {
								//	feeTip << bonus.name << " " << bonus.value;
								//}
								//AddToolTip(feeText, feeTip);
								AddTradeFeeText(building.subclass<TradeBuilding>(), descriptionBox);

								AddEfficiencyText(building, descriptionBox);

								descriptionBox->AddRichText("Profit", ToSignedNumber((tradingCompany.exportMoney100() - tradingCompany.importMoney100()) / 100) + "<img id=\"Coin\"/>");

								descriptionBox->AddLineSpacer();
							}

							if (tradingCompany.activeResourceEnum == ResourceEnum::None) {
								descriptionBox->AddRichText("<Red>Setup automatic trade below.</>");
							}
							else {
								FText importExportText = tradingCompany.isImport ? LOCTEXT("Importing", "Importing") : LOCTEXT("Exporting", "Exporting");
								ADDTEXT_(LOCTEXT("ImportUntil", "{0} until {1}"), importExportText, TEXT_NUM(tradingCompany.targetAmount));
								
								descriptionBox->AddIconPair(JOINTEXT(args), tradingCompany.activeResourceEnum, LOCTEXT("ImportTarget", " target"));
								args.Empty();

								if (tradingCompany.activeResourceEnum != ResourceEnum::None)
								{
									int32 target = tradingCompany.targetAmount;
									int32 count = resourceSys.resourceCount(tradingCompany.activeResourceEnum);

									if (tradingCompany.isImport) {
										if (target > count) {
											ADDTEXT_(LOCTEXT("ImportRemaining", "(import remaining: {0})"), TEXT_NUM(target - count));
										}
										else {
											ADDTEXT_LOCTEXT("ImportTarget", "(import storage-target reached)");
										}
									}
									else {
										// Export
										if (target < count) {
											ADDTEXT_(LOCTEXT("ExportRemaining", "(export remaining: {0})"), TEXT_NUM(count - target));
										}
										else {
											ADDTEXT_LOCTEXT("StorageBelowTarget", "(resources in storage already below storage-target)");
										}
									}
									descriptionBox->AddRichText(args);
								}
							}

							descriptionBox->AddSpacer(12);

							if (building.playerId() == playerId())
							{
								FText importText = LOCTEXT("Import", "Import");
								FText exportText = LOCTEXT("Export", "Export");
								
								descriptionBox->AddDropdown(
									building.buildingId(),
									{ importText, exportText },
									tradingCompany.isImport ? importText : exportText,
									[&](int32 objectId, FString sItem, IGameUIDataSource* dataSource, IGameNetworkInterface* networkInterface, int32 dropdownIndex)
								{
									auto& trader = dataSource->simulation().building(objectId).subclass<TradingCompany>(CardEnum::TradingCompany);

									auto command = make_shared<FChangeWorkMode>();
									command->buildingId = objectId;
									command->intVar1 = static_cast<int32>(trader.activeResourceEnum);
									command->intVar2 = (sItem == importText.ToString()) ? 1 : 0;
									command->intVar3 = static_cast<int32>(trader.targetAmount);
									networkInterface->SendNetworkCommand(command);
								});
							}

							descriptionBox->AddSpacer(12);

							// Show choose box
							descriptionBox->AddChooseResourceElement(tradingCompany.activeResourceEnum, this, CallbackEnum::OpenChooseResource);

							descriptionBox->AddSpacer(12);

							// targetAmount
							// - just opened UI, get it from targetAmount (actual value)
							// - after opened, we keep value in lastTargetAmountSet
							if (_justOpenedDescriptionUI) {
								tradingCompany.lastTargetAmountSet = tradingCompany.targetAmount;
							}
							int32 targetAmount = tradingCompany.lastTargetAmountSet;
							
							descriptionBox->AddEditableNumberBox(this, CallbackEnum::EditNumberChooseResource, building.buildingId(), "Target: ", targetAmount);

							descriptionBox->AddLineSpacer(12);
							if (tradingCompany.HasPendingTrade()) {
								ADDTEXT_(LOCTEXT("{0} secs", "{0} secs"), TEXT_NUM(tradingCompany.CountdownSecondsDisplayInt()));
								descriptionBox->AddRichText(LOCTEXT("Trade complete in", "Trade complete in"), args);
							}
							else 
							{
								// TODO: do we need args on the right?
								if (tradingCompany.hoverWarning == HoverWarning::NotEnoughMoney) {
									descriptionBox->AddRichText(TEXT_TAG("<Red>", LOCTEXT("Import Failed", "Import Failed")), args);
									descriptionBox->AddRichText(TEXT_TAG("<Red>", LOCTEXT("Not enough Money", "Not enough Money")), args);
								}
								else if (tradingCompany.hoverWarning == HoverWarning::AlreadyReachedTarget) {
									descriptionBox->AddRichText(TEXT_TAG("<Red>", LOCTEXT("Import Failed", "Import Failed")), args);
									descriptionBox->AddRichText(TEXT_TAG("<Red>", LOCTEXT("Already reached import target", "Already reached import target")), args);
								}
								else if (tradingCompany.hoverWarning == HoverWarning::ResourcesBelowTarget) {
									descriptionBox->AddRichText(TEXT_TAG("<Red>", LOCTEXT("Export Failed", "Export Failed")), args);
									descriptionBox->AddRichText(TEXT_TAG("<Red>", LOCTEXT("Resource below target", "Resource count below storage target")), args);
								}

								int32 tradeRetryCountdown = max(0, tradingCompany.TradeRetryCountDownTicks() / Time::TicksPerSecond);
								ADDTEXT_(LOCTEXT("{0} secs", "{0} secs"), TEXT_NUM(tradeRetryCountdown));
								descriptionBox->AddRichText(LOCTEXT("Retry trade in", "Retry trade in"), args);
							}

							// Dropdown / EditableNumberBox set in ObjectDescriptionUI.cpp
							//_objectDescriptionUI->SetEditableNumberBox(building.buildingId(), this, CallbackEnum::EditNumberChooseResource);

							/*
							 * Fill choose resource box
							 */
							UPunBoxWidget* chooseResourceBox = _objectDescriptionUI->ChooseResourceBox;
							FString searchString = _objectDescriptionUI->SearchBox->GetText().ToString();

							for (const ResourceInfo& info : SortedNameResourceInfo)
							{
								FString name = ToFString(info.name);

								if (IsTradeResource(info.resourceEnum))
								{
									if (searchString.IsEmpty() ||
										name.Find(searchString, ESearchCase::Type::IgnoreCase, ESearchDir::FromStart) != INDEX_NONE)
									{
										auto widget = chooseResourceBox->AddChooseResourceElement(info.resourceEnum, this, CallbackEnum::PickChooseResource);
										widget->punId = building.buildingId();
									}
								}
							}
							chooseResourceBox->AfterAdd();

						}

						
					}
					else if (building.isEnum(CardEnum::ShippingDepot))
					{
						descriptionBox->AddRichText(LOCTEXT("Choose3Resources", "Choose 3 resources to haul to delivery target"));
						descriptionBox->AddSpacer(12);
						
						TArray<FText> options;
						options.Add(LOCTEXT("None", "None"));
						for (ResourceInfo info : SortedNameResourceInfo) {
							options.Add(info.GetName());
						}
						
						auto addDropDown = [&](int32 index)
						{
							ResourceEnum resourceEnum = building.subclass<ShippingDepot>().resourceEnums[index];
							
							descriptionBox->AddDropdown(
								building.buildingId(),
								options,
								ResourceName_WithNone(resourceEnum),
								[](int32 objectId, FString sItem, IGameUIDataSource* dataSource, IGameNetworkInterface* networkInterface, int32 dropdownIndex)
							{
								auto command = make_shared<FChangeWorkMode>();
								command->buildingId = objectId;
								command->intVar1 = dropdownIndex;
								std::string resourceName = ToStdString(sItem);
								command->intVar2 = static_cast<int32>(FindResourceEnumByName(resourceName));
								networkInterface->SendNetworkCommand(command);
							}, index);
						};

						addDropDown(0);
						addDropDown(1);
						addDropDown(2);
					}
					else if (IsMountainMine(building.buildingEnum()))
					{
						int32 oreLeft = building.subclass<Mine>().oreLeft();
						if (oreLeft > 0) {
							descriptionBox->AddRichText(LOCTEXT("Resource left", "Resource left"), TEXT_NUM(oreLeft), building.product());
						} else {
							descriptionBox->AddRichText(TEXT_TAG("<Red", LOCTEXT("Mine Depleted", "Mine Depleted")));
						}
					}
					else if (IsRegionalBuilding(building.buildingEnum()))
					{
						descriptionBox->AddRichText(building.buildingInfo().description);
					}
					else if (building.isEnum(CardEnum::Colony))
					{
						auto& colony = building.subclass<Colony>();
						descriptionBox->AddRichText(LOCTEXT("Upkeep: ", "Upkeep: "), FText::Format(INVTEXT("{0}<img id=\"Influence\"/>"), TEXT_NUM(colony.GetColonyUpkeep())));

						descriptionBox->AddSpacer();

						ResourceEnum resourceEnum = colony.GetColonyResourceEnum();
						int32 resourceIncome = colony.GetColonyResourceIncome(resourceEnum);
						descriptionBox->AddRichText(LOCTEXT("Production (per round):", "Production (per round):"), TEXT_NUM(resourceIncome), resourceEnum);

						descriptionBox->AddSpacer();

						// Mine Resources
						if (IsOreEnum(resourceEnum))
						{
							int32 oreLeft = building.oreLeft();
							if (oreLeft > 0) {
								descriptionBox->AddRichText(LOCTEXT("Resource left", "Resource left"), TEXT_NUM(oreLeft), building.product());
							}
							else {
								descriptionBox->AddRichText(TEXT_TAG("<Red>", LOCTEXT("Mine Depleted", "Mine Depleted")));
							}
						}
					}
					else if (building.isEnum(CardEnum::Fort))
					{
						descriptionBox->AddRichText(LOCTEXT("AttackRequires", "Attacking this province requires 100% more <img id=\"Influence\"/>."));
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
					descriptionBox->AddButton(buttonName, nullptr, FText(), this, CallbackEnum::OpenManageStorage, true, false, objectId);
					//descriptionBox->AddLineSpacer(12);

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

					manageStorageBox->AfterAdd();
				}
				


				/*
				 * Work Status 1
				 */
				bool hasInput1 = building.hasInput1();
				bool hasInput2 = building.hasInput2();
				
				if (building.isConstructed())
				{
					// Productivity ... exclude farm
					if (IsProducer(building.buildingEnum()) ||
						building.buildingEnum() == CardEnum::FruitGatherer ||
						building.buildingEnum() == CardEnum::Forester ||
						building.buildingEnum() == CardEnum::HuntingLodge)
					{
						AddEfficiencyText(building, descriptionBox);
					}

					PUN_DEBUG_EXPR(int32 profitInternal = 0);

					const FText perSeasonText = LOCTEXT("Production (per season):", "Production (per season):");
					const FText perYearText = LOCTEXT("Production (per year):", "Production (per year):");
					const FText noneText = LOCTEXT("None", "None");

					// Production stats
					if (IsProducer(building.buildingEnum()) ||
						building.buildingEnum() == CardEnum::Forester)
					{
						descriptionBox->AddLineSpacer();
						descriptionBox->AddRichText(perSeasonText, TEXT_NUM(building.seasonalProduction()), building.product());

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
								descriptionBox->AddRichText(perYearText, noneText);
							} else {
								descriptionBox->AddRichText(perSeasonText, noneText);
							}
						}
						else if (pairs.size() == 1) 
						{
							if (building.isEnum(CardEnum::Farm)) {
								descriptionBox->AddRichText(perYearText, TEXT_NUM(pairs[0].count), pairs[0].resourceEnum);
							} else {
								descriptionBox->AddRichText(perSeasonText, TEXT_NUM(pairs[0].count), pairs[0].resourceEnum);
							}
						}
						else {
							descriptionBox->AddRichText(perSeasonText);
							for (ResourcePair pair : pairs) {
								descriptionBox->AddIconPair(INVTEXT(" "), pair.resourceEnum, TEXT_NUM(pair.count));
							}
						}

						PUN_DEBUG_EXPR(
							if (building.product() != ResourceEnum::None) {
								profitInternal += (building.seasonalProduction() * GetResourceInfo(building.product()).basePrice);
							}
						);
					}


					const FText consumptionSeasonText = LOCTEXT("Consumption(per season):", "Consumption(per season):");
					const FText depletionSeasonText = LOCTEXT("Depletion(per season):", "Depletion(per season):");

					// Consumption stat
					if (IsConsumerWorkplace(building.buildingEnum()))
					{
						if (hasInput1 && hasInput2)
						{
							descriptionBox->AddRichText(consumptionSeasonText);
							if (hasInput1) {
								descriptionBox->AddIconPair(FText(), building.input1(), TEXT_NUM(building.seasonalConsumption1()));

								PUN_DEBUG_EXPR(profitInternal -= (building.seasonalConsumption1() * GetResourceInfo(building.input1()).basePrice));
							}
							//if (hasInput1 && hasInput2) ss << ", ";
							if (hasInput2) {
								descriptionBox->AddIconPair(FText(), building.input2(), TEXT_NUM(building.seasonalConsumption2()));

								PUN_DEBUG_EXPR(profitInternal -= (building.seasonalConsumption2() * GetResourceInfo(building.input2()).basePrice));
							}
							//ss << "\n";
						}
						if (hasInput1)
						{
							descriptionBox->AddRichText(consumptionSeasonText, TEXT_NUM(building.seasonalConsumption1()), building.input1());
						}
					}
					// Mines consume deposits
					if (IsMountainMine(building.buildingEnum())) {
						descriptionBox->AddRichText(depletionSeasonText, TEXT_NUM(building.seasonalConsumption1()), building.product());
					}

					PUN_DEBUG_EXPR(descriptionBox->AddRichText(INVTEXT("<Yellow>PROFIT:</>"), TEXT_NUM(profitInternal)));
				}

				/*
				 * Dropdown
				 */
				if (building.hasWorkModes() &&
					building.playerId() == playerId())
				{
					descriptionBox->AddDropdown(
						building.buildingId(),
						building.workModeNames(),
						building.workMode().name,
						[](int32 objectId, FString sItem, IGameUIDataSource* dataSource, IGameNetworkInterface* networkInterface, int32 dropdownIndex)
					{
						auto command = make_shared<FChangeWorkMode>();
						command->buildingId = objectId;
						command->enumInt = dataSource->simulation().building(objectId).workModeIntFromString(sItem);
						networkInterface->SendNetworkCommand(command);
					});
					if (!building.workMode().description.IsEmpty()) {
						descriptionBox->AddRichText(building.workMode().description);
					}
				}

				// Forester
				if (building.isEnum(CardEnum::Forester) &&
					building.playerId() == playerId())
				{
					Forester& forester = building.subclass<Forester>();
					
					auto addDropdown = [&](TArray<FText> options, FText defaultOption, int32 dropdownIndex)
					{
						descriptionBox->AddDropdown(
							building.buildingId(),
							options,
							defaultOption,
							[options](int32 objectId, FString sItem, IGameUIDataSource* dataSource, IGameNetworkInterface* networkInterface, int32 dropdownIndex)
						{
							auto FindCutTreeEnumFromName = [&](FText name) {
								for (int32 i = 0; i < options.Num(); i++) {
									if (name.EqualTo(options[i])) {
										return i;
									}
								}
								return 0;
							};
							
							auto command = make_shared<FChangeWorkMode>();
							command->buildingId = objectId;
							command->intVar1 = dropdownIndex;
							command->intVar2 = FindCutTreeEnumFromName(FText::FromString(sItem));
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
					
					if (workModeName.EqualTo(CutAndPlantText) ||
						workModeName.EqualTo(PrioritizePlantText)) {
						addDropdown(plantOptions, plantOptions[static_cast<int>(forester.plantingEnum)], 0);
					}
					if (workModeName.EqualTo(CutAndPlantText) ||
						workModeName.EqualTo(PrioritizeCutText))  {
						addDropdown(cutOptions, cutOptions[static_cast<int>(forester.cuttingEnum)], 1);
					}
					descriptionBox->AddSpacer();
				}


				/*
				 * Work Status 2
				 */
				if (building.isConstructed()) 
				{

					// Work Reserved/Done
					if (IsProducer(building.buildingEnum()))
					{
#if WITH_EDITOR
						descriptionBox->AddRichText("-- workManSecPerBatch100: " + to_string(building.workManSecPerBatch100()));
						descriptionBox->AddRichText("-- workRevenuePerSec100_perMan: " + to_string(building.buildingInfo().workRevenuePerSec100_perMan));
						descriptionBox->AddRichText("-- batchCost: " + to_string(building.batchCost()));
						descriptionBox->AddRichText("-- batchProfit: " + to_string(building.batchProfit()));
#endif

						/*
						 * Work modes
						 */
						//if (BuildingHasDropdown(building.buildingEnum())) {
						//	addDropDown();
						//}

						descriptionBox->AddSpacer();
						descriptionBox->AddRichText("<Subheader>Production batch:</>");
						descriptionBox->AddProductionChain({ building.input1(), building.inputPerBatch() },
							{ building.input2(), building.inputPerBatch() },
							{ building.product(), building.productPerBatch() });

						//if (building.hasInput1() || building.hasInput2()) {
						//	descriptionBox->AddRichText("Input: ");
						//}
						//if (building.hasInput1()) ss << building.inputPerBatch() << " " << GetResourceInfo(building.input1()).name << " ";
						//if (building.hasInput2()) ss << ", " << building.inputPerBatch() << " " << GetResourceInfo(building.input2()).name;
						//ss << "\n";

						//ss << "Output: " << building.productPerBatch() << " " << GetResourceInfo(building.product()).name << "\n\n";

						descriptionBox->AddRichText(LOCTEXT("Work done", "Work done"), TEXT_PERCENT(building.workPercent()));

						if (building.workPercent() > 0) {
							//ss << "(get " << building.productPerBatch() << " " << ResourceName(building.product()) << ")\n";
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
								
								ADDTEXT_LOCTEXT("Require {0}", "Require {0}");
								if (needInput1) ADDTEXT_(INVTEXT("{0} {1}"), TEXT_NUM(building.inputPerBatch()), ResourceNameT(building.input1()));
								if (needInput1 && needInput2) ADDTEXT_INV_(", ");
								if (needInput2) ADDTEXT_(INVTEXT("{0} {1}"), TEXT_NUM(building.inputPerBatch()), ResourceNameT(building.input2()));

								descriptionBox->AddRichText(TEXT_TAG("<Orange>", JOINTEXT(args)));
							}
						}
					}
					else if (IsSpecialProducer(building.buildingEnum()))
					{
#if WITH_EDITOR
						descriptionBox->AddRichText("-- workManSecPerBatch100: " + to_string(building.workManSecPerBatch100()));
						descriptionBox->AddRichText("-- workRevenuePerSec100_perMan: " + to_string(building.buildingInfo().workRevenuePerSec100_perMan));
						if (building.hasInput1()) {
							descriptionBox->AddRichText("-- baseInputValue: " + to_string(GetResourceInfo(building.input1()).basePrice * building.baseInputPerBatch()));
						}
	
						switch (building.buildingEnum())
						{
						case CardEnum::BarrackArcher:
						case CardEnum::BarrackClubman:
						case CardEnum::Mint:
						case CardEnum::InventorsWorkshop:
						case CardEnum::RegionShrine:
							
						case CardEnum::CardMaker:
						case CardEnum::ImmigrationOffice: {
							auto& consumerIndustry = building.subclass<ConsumerIndustrialBuilding>();
							if (building.hasInput1()) {
								descriptionBox->AddRichText("-- baseInputValue: " + to_string(consumerIndustry.baseInputValue()));
								descriptionBox->AddRichText("-- baseOutputValue: " + to_string(consumerIndustry.baseOutputValue()));
								descriptionBox->AddRichText("-- baseProfitValue: " + to_string(consumerIndustry.baseProfitValue()));
								descriptionBox->AddRichText("-- workManSecPerBatch100(calc): " + to_string(consumerIndustry.baseProfitValue() * 100 * 100 / consumerIndustry.buildingInfo().workRevenuePerSec100_perMan));
							}
							break;
						}
						default:
							break;
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
							case CardEnum::Mint: setProduct(assetLoader->CoinIcon, to_string(building.productPerBatch()));  break;
							case CardEnum::InventorsWorkshop: setProduct(assetLoader->ScienceIcon, to_string(building.productPerBatch()));  break;
							case CardEnum::RegionShrine: setProduct(assetLoader->ScienceIcon, to_string(building.productPerBatch()));  break;
							
							case CardEnum::BarrackArcher:
							case CardEnum::BarrackSwordman:
									setProduct(assetLoader->InfluenceIcon, to_string(building.productPerBatch()));  break;
							
							case CardEnum::CardMaker: setProduct(assetLoader->CardBack, "1 card");  break;
							case CardEnum::ImmigrationOffice: break;
							default:
								UE_DEBUG_BREAK();
								break;
						}

						if (productTexture)
						{
							descriptionBox->AddSpacer();
							descriptionBox->AddRichText("<Subheader>Production batch:</>");
							descriptionBox->AddProductionChain({ building.input1(), building.inputPerBatch() },
								{ building.input2(), building.inputPerBatch() },
								{}, productTexture, productStr);
						}

						//PUN_LOG("_workDone100:%d workManSecPerBatch100:%d batchProfit:%d baseInputPerBatch:%d efficiency:%d workRevenuePerSec100_perMan:%d", 
						//	building.workDone100(), building.workManSecPerBatch100(), building.batchProfit(), building.baseInputPerBatch(), building.efficiency(), building.buildingInfo().workRevenuePerSec100_perMan);
						
						descriptionBox->AddRichText(LOCTEXT("Work done", "Work done"), TEXT_PERCENT(building.workPercent()));
					}
				}
				else
				{		
					//resourceAndWorkSS << "constructReserved: " << (int)(100.0f * building.workReserved() / workCost) << "%\n";

					descriptionBox->AddRichText(LOCTEXT("Construct", "Construct"), TEXT_PERCENT(building.constructionPercent()));
					
#if WITH_EDITOR
					descriptionBox->AddRichText("-- buildManSecCost100", to_string(building.buildTime_ManSec100()));
#endif
				}

				// Work Reservers (Editor)
				//std::vector<int>& workReservers = building.workReservers();
				//for (int i = 0; i < workReservers.size(); i++) {
				//	ss << workReservers[i] << "\n";
				//}

				/*
				 * Set Output Target
				 */
				ResourceEnum product = building.product();
				if (building.playerId() == playerId() &&
					product != ResourceEnum::None)
				{
					auto& playerOwned = simulation.playerOwned(playerId());
					if (_justOpenedDescriptionUI) {
						playerOwned.SetOutputTargetDisplay(product, playerOwned.GetOutputTarget(product));
					}
					int32 targetDisplay = playerOwned.GetOutputTargetDisplay(product);
					bool isChecked = (targetDisplay != -1);

					descriptionBox->AddLineSpacer();
					auto numberBox = descriptionBox->AddEditableNumberBox(this, CallbackEnum::EditableNumberSetOutputTarget, building.buildingId(),
																"set produce target", targetDisplay, "produce until ", isChecked, product);
					numberBox->callbackVar1 = static_cast<int32>(product);
				}


				
				/*
				 * Inventory
				 */
				std::vector<ResourceHolderInfo>& holderInfos = building.holderInfos();
				if (!building.isConstructed()) 
				{
					descriptionBox->AddLineSpacer();
					descriptionBox->AddRichText(TEXT_TAG("<Subheader>", LOCTEXT("Resources needed:", "Resources needed:")));
					for (ResourceHolderInfo holderInfo : holderInfos) {
						ADDTEXT_(INVTEXT("{0}/{1}"), TEXT_NUM(building.GetResourceCount(holderInfo)), TEXT_NUM(building.GetResourceTarget(holderInfo)))
						descriptionBox->AddIconPair(FText(), holderInfo.resourceEnum, args);
					}

#if WITH_EDITOR
					TArray<FText> argsLocal;
					ADDTEXT(argsLocal, INVTEXT("hasNeededConstructionResource: {0}"), building.hasNeededConstructionResource() ? INVTEXT("Yes") : INVTEXT("No"));
					descriptionBox->AddSpecialRichText(INVTEXT("<Yellow>"), argsLocal);
					ADDTEXT(argsLocal, INVTEXT("NeedConstruct: {0}"), building.NeedConstruct() ? INVTEXT("Yes") : INVTEXT("No"));
					descriptionBox->AddSpecialRichText(INVTEXT("<Yellow>"), argsLocal);
#endif
				}
				// Inventory
				else if (!holderInfos.empty() && buildingEnum != CardEnum::House)
				{
					descriptionBox->AddLineSpacer();

					if (holderInfos.size() > 0) {
						descriptionBox->AddRichText("<Subheader>Inventory:</>");
					}
					else {
						descriptionBox->AddRichText("<Subheader>Inventory:</>", "None");
					}

					for (ResourceHolderInfo holderInfo : holderInfos)
					{
						int32 resourceCount = building.GetResourceCount(holderInfo);

						// Don't display 0
						if (resourceCount == 0) {
							continue;
						}

						descriptionBox->AddIconPair(FText(), holderInfo.resourceEnum, TEXT_NUM(resourceCount));

						//ss << ResourceName(holderInfo.resourceEnum) << ": " << resourceCount;
#if WITH_EDITOR 
						ResourceHolder holder = building.resourceSystem().holder(holderInfo);

						stringstream sst;
						sst << "-- Target:" << building.GetResourceTarget(holderInfo) << ",Pop:" << holder.reservedPop() << ",Push:" << holder.reservedPush();
						descriptionBox->AddRichText(sst);
#endif
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
						
						for (int32 i = 0; i < _objectDescriptionUI->lastCards.size(); i++)
						{
							UBuildingPlacementButton* cardButton = AddWidget<UBuildingPlacementButton>(UIEnum::CardMini);
							CardEnum cardEnum = _objectDescriptionUI->lastCards[i].cardEnum;

							cardButton->PunInit(cardEnum, i, 0, 1, this, CallbackEnum::SelectBuildingSlotCard);
							cardButton->callbackVar1 = building.buildingId();
							cardButton->cardAnimationOrigin = cards[i].lastPosition();
							cardButton->cardAnimationStartTime = cards[i].animationStartTime100 / 100.0f; // Animation start time is when FUseCard arrived
							
							SetChildHUD(cardButton);

							cardButton->SetupMaterials(dataSource()->assetLoader());
							cardButton->SetCardStatus(CardHandEnum::CardSlots, false, false, IsRareCard(cardEnum));

							cardButton->SellButton->SetVisibility(ESlateVisibility::Collapsed);

							_objectDescriptionUI->CardSlots->AddChild(cardButton);
						}

						// card slots for the rest
						for (int32 i = _objectDescriptionUI->lastCards.size(); i < _objectDescriptionUI->lastMaxCards; i++)
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
						auto& cardSys = simulation.cardSystem(playerId());
						updateCardSlotUI(cardSys.cardsInTownhall(), cardSys.maxTownhallCards(), true);
					}
					else {
						updateCardSlotUI(building.slotCards(), building.maxCardSlots(), false);
					}
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
					Building& buildingScope = simulation.buildingChecked(sourceId);
					FVector displayLocationScope = dataSource()->DisplayLocationTrueCenter(buildingScope);
					dataSource()->ShowDeliveryArrow(displayLocationScope, displayLocation);
				}
				//   Self
				int32 deliveryTargetId = building.deliveryTargetId();
				if (deliveryTargetId != -1)
				{
					Building& buildingScope = simulation.buildingChecked(deliveryTargetId);
					FVector displayLocationScope = dataSource()->DisplayLocationTrueCenter(buildingScope);
					dataSource()->ShowDeliveryArrow(displayLocation, displayLocationScope);
				}

				if (building.playerId() == playerId())
				{
					// Special case: Logistics Office
					if (building.isEnum(CardEnum::ShippingDepot))
					{
						vector<int32> storageIds = simulation.GetBuildingsWithinRadiusMultiple(building.centerTile(), ShippingDepot::Radius, playerId(), StorageEnums);
						for (int32 storageId : storageIds)
						{
							Building& buildingScope = simulation.buildingChecked(storageId);
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
							Building* workplace = simulation.unitAI(occupantId).workplace();
							if (workplace && !workplace->isEnum(CardEnum::Townhall)) {
								FVector displayLocationScope = dataSource()->DisplayLocationTrueCenter(*workplace);
								dataSource()->ShowDeliveryArrow(displayLocation, displayLocationScope, true, true);
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

				if (building.playerId() == playerId())
				{
					//! Upgrade Button Townhall
					if (building.isEnum(CardEnum::Townhall))
					{
						TownHall& townhall = building.subclass<TownHall>();
						if (townhall.townhallLvl < TownHall::GetMaxUpgradeLvl()) 
						{
							int32 upgradeMoney = townhall.GetUpgradeMoney();
							FText moneyText = FText::AsNumber(upgradeMoney);
							if (resourceSys.money() < upgradeMoney) {
								moneyText = TEXT_TAG("<Red>", moneyText);
							}

							bool showExclamation = simulation.parameters(playerId())->NeedTownhallUpgradeNoticed;

							//ss << "Upgrade Townhall to lvl " << (townhall.townhallLvl + 1) << "\n";
							//ss << "<img id=\"Coin\"/>" << moneyText;

							ADDTEXT_(
								LOCTEXT("TownhallUpgradeButton",
									"Upgrade Townhall to lvl {0}\n"
									"<img id=\"Coin\"/>{1}"
								),
								FText::AsNumber(townhall.townhallLvl + 1),
								moneyText
							);
							
							UPunButton* button = descriptionBox->AddButton2Lines(FText::Join(FText(), args), this, CallbackEnum::UpgradeBuilding, true, showExclamation, objectId, 0);
							args.Empty();

							ADDTEXT_LOCTEXT("Upgrade Rewards", "Upgrade Rewards");
							ADDTEXT_INV_("<line>");
							ADDTEXT__(GetTownhallLvlToUpgradeBonusText(townhall.townhallLvl + 1));
							
							if (resourceSys.money() < upgradeMoney) {
								ADDTEXT_INV_("<space>");
								ADDTEXT_TAG_("<Red>", LOCTEXT("Not enough money to upgrade.", "Not enough money to upgrade."));
							}
							AddToolTip(button, args);
						}
					}
					//! Upgrade Button Others
					else
					{
						auto setUpgradeButton = [&](BuildingUpgrade upgrade, int32 upgradeIndex)
						{
							ADDTEXT_(LOCTEXT("Upgrade {0}", "Upgrade {0}"), upgrade.name);

							ResourceEnum resourceEnum = upgrade.resourceNeeded.resourceEnum;
							
							if (upgrade.isUpgraded) {
								ADDTEXT_INV_("\n");
								ADDTEXT_LOCTEXT("Done", "Done");
							}
							else {
								auto showResourceText = [&](FText resourceText)
								{
									bool isRed = resourceSys.resourceCount(resourceEnum) < upgrade.resourceNeeded.count;
									
									ADDTEXT_(INVTEXT("\n<img id=\"{0}\"/>{1}"), resourceText, TextRed(TEXT_NUM(upgrade.resourceNeeded.count), isRed));
								};
								
								if (resourceEnum == ResourceEnum::Stone) { showResourceText(LOCTEXT("Stone", "Stone")); }
								else if (resourceEnum == ResourceEnum::Wood) { showResourceText(LOCTEXT("Wood", "Wood")); }
								else if (resourceEnum == ResourceEnum::Iron) { showResourceText(LOCTEXT("IronBar", "IronBar")); }
								else if (resourceEnum == ResourceEnum::SteelTools) { showResourceText(LOCTEXT("SteelTools", "SteelTools")); }
								else if (resourceEnum == ResourceEnum::Brick) { showResourceText(LOCTEXT("Brick", "Brick")); }
								else if (resourceEnum == ResourceEnum::Paper) { showResourceText(LOCTEXT("Paper", "Paper")); }
								//else if (upgrade.resourceNeeded.isValid()) {
								//	ss << " (" << upgrade.resourceNeeded.ToString() << ")";
								//}
								else {
									PUN_CHECK(resourceEnum == ResourceEnum::None);
									
									FText moneyText = TextRed(TEXT_NUM(upgrade.moneyNeeded), resourceSys.money() < upgrade.moneyNeeded);
									
									ADDTEXT_(INVTEXT("\n<img id=\"Coin\"/>{0}"), moneyText);
								}
							}

							bool isUpgradable = !upgrade.isUpgraded;

							UPunButton* button = descriptionBox->AddButton2Lines(JOINTEXT(args), this, CallbackEnum::UpgradeBuilding, isUpgradable, false,  objectId, upgradeIndex);

							// Tooltip
							args.Empty();
							ADDTEXT__(upgrade.name);
							ADDTEXT_INV_("<space>");

							const FText costText = LOCTEXT("cost", "cost");
							if (resourceEnum == ResourceEnum::None) {
								ADDTEXT_(INVTEXT("{0}: <img id=\"Coin\"/>{1}"), costText, TEXT_NUM(upgrade.moneyNeeded));
							} else {
								ADDTEXT_(INVTEXT("{0}: {1} {2}"), costText, TEXT_NUM(upgrade.resourceNeeded.count), ResourceNameT(resourceEnum));
							}
							ADDTEXT_INV_("<space>");
							
							ADDTEXT__(upgrade.description);

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
					if ((building.product() != ResourceEnum::None && simulation.unlockSystem(playerId())->unlockedSetDeliveryTarget) ||
						building.isEnum(CardEnum::ShippingDepot))
					{
						auto button = descriptionBox->AddButton(LOCTEXT("Set Delivery Target", "Set Delivery Target"), nullptr, FText(), this, CallbackEnum::SetDeliveryTarget, true, false, objectId);
						AddToolTip(button, LOCTEXT("Set Delivery Target Tip", "Set the Target Storage/Market where output resources would be delivered"));
						
						if (building.deliveryTargetId() != -1) {
							descriptionBox->AddButton(LOCTEXT("Remove Delivery Target", "Remove Delivery Target"), nullptr, FText(), this, CallbackEnum::RemoveDeliveryTarget, true, false, objectId);
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
			if (building.isEnum(CardEnum::Bridge) ||
				building.isEnum(CardEnum::Tunnel)) 
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
				showGroundBoxHighlightDecal();
			}
			else if (building.isConstructed() && !building.isBurnedRuin()) {
				dataSource()->ShowBuildingMesh(building, 2);
			} else {
				showGroundBoxHighlightDecal();
			}

			descriptionBox->AfterAdd();
		}

		/*
		 * Unit
		 */
		else if (uiState.objectType == ObjectTypeEnum::Unit) 
		{
			UnitStateAI& unit = dataSource()->GetUnitStateAI(objectId);

			// Make sure we don't forget to put in string
			check((int)unit.unitState() < UnitStateName.Num());

			TArray<FText> args;

			if (unit.isEnum(UnitEnum::Human)) {
				SetText(_objectDescriptionUI->DescriptionUITitle, TEXT_TAG("<Header>", unit.GetUnitNameT()));

				ADDTEXT_TAG_("<Subheader>", unit.GetTypeName());
			} else {
				SetText(_objectDescriptionUI->DescriptionUITitle, TEXT_TAG("<Header>", unit.GetTypeName()));
			}
			
#if WITH_EDITOR
			ADDTEXT_(INVTEXT("[{0}]"), TEXT_NUM(objectId)); // ID
#endif
			descriptionBox->AddRichText(args);
			descriptionBox->AddLineSpacer(8);

			if (!unit.isEnum(UnitEnum::Human)) {
				ADDTEXT_(INVTEXT("{0}: {1}"), 
					LOCTEXT("Gender", "Gender"),
					unit.isMale() ? LOCTEXT("male", "male") : LOCTEXT("female", "female"));
				
				descriptionBox->AddRichText(args);
			}

			if (unit.playerId() != -1) {
				const FText playerText = LOCTEXT("Player", "Player");
				ADDTEXT_(INVTEXT("{0}: {1}"),
					LOCTEXT("Player", "Player"), 
					simulation.playerNameT(unit.playerId()));
				descriptionBox->AddRichText(args);
			}
			
#if WITH_EDITOR
			{
				std::stringstream ss;
				ss << "-- Tile:(" << unit.unitTile().x << "," << unit.unitTile().y << ")";
				descriptionBox->AddRichText(ss);
				ss << "-- lastBirthTick:(" << unit.lastPregnantTick() << ")";
				descriptionBox->AddRichText(ss);
				ss << "animation: " << GetUnitAnimationName(unit.animationEnum());
				descriptionBox->AddRichText(ss);
			}
#endif

			int32 age = unit.age() * PlayerParameters::PeopleAgeToGameYear / Time::TicksPerYear + 3; // cheat 3 years older so no 0 years old ppl
			ADDTEXT_(LOCTEXT("Age: {0} years", "Age: {0} years"), TEXT_NUM(age));
			
			descriptionBox->AddRichText(args);
			descriptionBox->AddSpacer();

			// Show Food/Heat as percent
			ADDTEXT_(INVTEXT("<space>{0}: {1}/100"), LOCTEXT("Food", "Food"), TEXT_NUM(unit.foodActual() * 100 / unit.maxFood()));
			ADDTEXT_(INVTEXT("<space>{0}: {1}/100"), LOCTEXT("Heat", "Heat"), TEXT_NUM(unit.heatActual() * 100 / unit.maxHeat()));
			ADDTEXT_(INVTEXT("<space>{0}: {1}/100"), LOCTEXT("Health", "Health"), TEXT_NUM(unit.hp()));

			if (unit.isEnum(UnitEnum::Human)) {
				ADDTEXT_(INVTEXT("<space>{0}: {1}%"), LOCTEXT("Fun", "Fun"), TEXT_NUM(unit.subclass<HumanStateAI>().funPercent()));
#if WITH_EDITOR
				ADDTEXT_(INVTEXT("<space> -- Fun Sec: {0}s"), TEXT_NUM(unit.subclass<HumanStateAI>().funTicksActual() / Time::TicksPerSecond));
#endif
			}

			descriptionBox->AddRichTextParsed(args);
			descriptionBox->AddSpacer(8);

			// Dying
			auto dyingMessage = [&](FText dyingDescription) {
				ADDTEXT_(INVTEXT("<space><Red>{0}</>"), dyingDescription);
				descriptionBox->AddRichTextParsed(args);
				descriptionBox->AddSpacer();
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
#if WITH_EDITOR
				descriptionBox->AddRichText("-- Next tools sec", to_string((human.nextToolTick() - Time::Ticks()) / Time::TicksPerSecond));
#endif
			}

			// Status (Human)
			if (unit.isEnum(UnitEnum::Human)) 
			{
				auto& human = unit.subclass<HumanStateAI>();

				{
					auto widget = descriptionBox->AddRichText(LOCTEXT("Work efficiency", "Work efficiency"), TEXT_PERCENT(human.workEfficiency100()));

					ADDTEXT_(LOCTEXT("Work efficiency tip",
						"Work efficiency: {0}%\n"
						" 100% base\n"
						" penalties:\n"),
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
				
				descriptionBox->AddSpacer();

				{
					ADDTEXT_(INVTEXT("{0}<img id=\"Smile\"/>"), TEXT_NUM(human.happiness()));
					auto widget = descriptionBox->AddRichText(LOCTEXT("Happiness", "Happiness"), args);

					ADDTEXT_(LOCTEXT("Happiness Tip",
						"Happiness: {0}<img id=\"Smile\"/>\n"
						"  Needs: {1}\n"), 
						TEXT_NUM(human.happiness()), TEXT_NUM(human.baseHappiness())
					);

					auto addRow = [&](FText typeName, int32 value) {
						ADDTEXT_(INVTEXT("   {0} {1}\n"), TEXT_NUM(value), typeName);
					};
					
					addRow(LOCTEXT("food", "food"), human.foodHappiness());
					addRow(LOCTEXT("heating", "heating"), human.heatHappiness());
					addRow(LOCTEXT("housing", "housing"), human.housingHappiness());
					addRow(LOCTEXT("fun", "fun"), human.funHappiness());
					
					ADDTEXT_(INVTEXT("  {0}: {1}\n"), LOCTEXT("Modifiers", "Modifiers"), TEXT_NUM(human.modifiersHappiness()));

					for (size_t i = 0; i < HappinessModifierName.Num(); i++) {
						int32 modifier = human.GetHappinessModifier(static_cast<HappinessModifierEnum>(i));
						if (modifier != 0) {
							addRow(HappinessModifierName[i], modifier);
						}
					}
					
					AddToolTip(widget, args);
				}
			}

			ADDTEXT_(INVTEXT("{0}: {1}\n"), LOCTEXT("state", "state"), UnitStateName[static_cast<int>(unit.unitState())]);

			// Workplace (Human)
			if (unit.isEnum(UnitEnum::Human))
			{
				if (unit.workplaceId() != -1)
				{
					Building& workplace = simulation.building(unit.workplaceId());

					if (workplace.isConstructed()) {
						ADDTEXT_(INVTEXT("{0}: {1}"), LOCTEXT("workplace", "workplace"), workplace.buildingInfo().GetName());
					}
					else {
						ADDTEXT_LOCTEXT("workplace: Construction Site", "workplace: Construction Site");
					}

#if WITH_EDITOR 
					ADDTEXT_(INVTEXT(" (id: {0})\n"), TEXT_NUM(unit.workplaceId()));
#endif
				}
				else {
					ADDTEXT_LOCTEXT("workplace: none \n", "workplace: none \n");
				}
			}

			if (unit.houseId() != -1) {
				FText houseName = simulation.building(unit.houseId()).buildingInfo().GetName();
				ADDTEXT_(LOCTEXT("house: (id: )", "house: {0} (id: {1})\n"), houseName, TEXT_NUM(unit.houseId()));
			} else {
				ADDTEXT_LOCTEXT("house: none \n", "house: none \n");
			}

#if WITH_EDITOR
			if (IsAnimal(unit.unitEnum())) {
				ADDTEXT_(INVTEXT("home province {0}"), TEXT_NUM(unit.homeProvinceId()));
			}
#endif

			//! Debug
#if WITH_EDITOR 
			if (PunSettings::IsOn("UIActions")) {
				std::stringstream ss;
				ss << "-- nextActiveTick:" << unit.nextActiveTick() << "\n";
				ss << "speech:\n" << unit.debugSpeech(true);
				auto punText = descriptionBox->AddText(ss);
				
				FSlateFontInfo fontInfo = FSlateFontInfo(FPaths::EngineContentDir() / TEXT("Slate/Fonts/Roboto-Bold.ttf"), 7);
				punText->PunText->SetFont(fontInfo);
				//punText->PunText->SetRenderScale(FVector2D(1.0f, 0.5));
			}


			// Animal Arrow to Home Province
			if (!unit.isEnum(UnitEnum::Human) && unit.homeProvinceId() != -1)
			{
				WorldTile2 provinceCenter = simulation.GetProvinceCenterTile(unit.homeProvinceId());
				dataSource()->ShowDeliveryArrow(dataSource()->DisplayLocation(unit.unitTile().worldAtom2()), 
												dataSource()->DisplayLocation(provinceCenter.worldAtom2()));
			}
#endif

			//if (unit.isEnum(UnitEnum::Infantry)) {
			//	ss << "target:" << unit.subclass<ArmyStateAI>().targetBuildingId() << "\n";
			//	ss << "garrison:" << unit.subclass<ArmyStateAI>().garrisonBuildingId() << "\n";
			//}

			descriptionBox->AddRichText(args);

			// Inventory
			descriptionBox->AddLineSpacer();
			descriptionBox->AddRichText("Inventory:");
			unit.inventory().ForEachResource([&](ResourcePair resource) {
				descriptionBox->AddIconPair(FText(), resource.resourceEnum, TEXT_NUM(resource.count));
			});
			descriptionBox->AddLineSpacer();

			// Selection Meshes
			if (unit.houseId() != -1) {
				//WorldAtom2 atom = simulation.buildingSystem().actualAtomLocation(unit.houseId());
				//SpawnSelectionMesh(assetLoader->SelectionMaterialYellow, dataSource()->DisplayLocation(atom) + FVector(0, 0, 30));

				FVector selectorLocation = dataSource()->DisplayLocationTrueCenter(simulation.building(unit.houseId()));
				SpawnSelectionMesh(assetLoader->SelectionMaterialYellow, selectorLocation);
			}
			if (unit.workplaceId() != -1) {
				//WorldAtom2 atom = simulation.buildingSystem().actualAtomLocation(unit.workplaceId());
				//SpawnSelectionMesh(assetLoader->SelectionMaterialYellow, dataSource()->DisplayLocation(atom) + FVector(0, 0, 30));

				FVector selectorLocation = dataSource()->DisplayLocationTrueCenter(simulation.building(unit.workplaceId()));
				SpawnSelectionMesh(assetLoader->SelectionMaterialYellow, selectorLocation);

				// TODO: why this??
				//if (GameRand::DisplayRand(Time::Ticks()) == 123456789) {
				//	SpawnSelectionMesh(assetLoader->ReferenceTerrainMaterial, dataSource()->DisplayLocation(atom) + FVector(0, 0, -30));
				//}
			}

			// Selection Mesh
			WorldAtom2 atom = dataSource()->unitDataSource().actualAtomLocation(objectId);
			SpawnSelectionMesh(assetLoader->SelectionMaterialGreen, dataSource()->DisplayLocation(atom) + FVector(0, 0, 30));

			if (unit.unitEnum() != UnitEnum::Human)
			{
				FTransform transform;
				int32 variationIndex = dataSource()->GetUnitTransformAndVariation(objectId, transform);
				UStaticMesh* unitMesh = assetLoader->unitMesh(unit.unitEnum(), variationIndex);
				SpawnMesh(unitMesh, unitMesh->GetMaterial(0), transform, false, 2);
			}

			descriptionBox->AfterAdd();
		}

		/*
		 * Georesource
		 */
		else if (uiState.objectType == ObjectTypeEnum::Georesource)
		{
			// TODO: remove???
			
			//WorldRegion2 region(objectId);
			//
			//auto& georesourceSystem = simulation.georesourceSystem();
			//GeoresourceNode node = georesourceSystem.georesourceNode(objectId);
			////GeoresourceInfo info = node.info();
			////
			////stringstream ss;
			////ss << info.name << "\n";
			////descriptionBox->AddTextWithSpacer(ss);
			////
			////ss << info.description << "\n\n";

			////descriptionBox->AddTextWithSpacer(ss);

			//AddGeoresourceInfo(objectId, descriptionBox);

			//AddSelectStartLocationButton(region, descriptionBox);
			//AddClaimLandButtons(region, descriptionBox);
			//AddClaimRuinButton(WorldRegion2(objectId), descriptionBox);

			//// Selection Mesh
			//FVector displayLocation = dataSource()->DisplayLocation(node.centerTile.worldAtom2());
			//SpawnSelectionMesh(assetLoader->SelectionMaterialGreen, displayLocation + FVector(0, 0, 30));

			//ShowTileSelectionDecal(displayLocation, node.area.size());
			//if (simulation.provinceOwner(region.regionId()) != playerId()) {
			//	ShowRegionSelectionDecal(node.centerTile);
			//}

			//descriptionBox->AfterAdd();
		}

		/*
		 * TileObject
		 */
		else if (uiState.objectType == ObjectTypeEnum::TileObject)
		{
			auto& treeSystem = simulation.treeSystem();
			TileObjInfo info = treeSystem.tileInfo(objectId);
			WorldTile2 tile(objectId);

			int32 provinceId = simulation.GetProvinceIdClean(tile);

			stringstream ss;
			ss << "<Header>" << info.name << "</> " << tile.ToString();
			SetText(_objectDescriptionUI->DescriptionUITitle, ss);
			descriptionBox->AddSpacer(12);

			ss << WrapString(info.description); // WrapString Help Prevent flash
			descriptionBox->AddRichText(ss);
			
			if (info.IsPlant()) {
				descriptionBox->AddSpacer(12);
				ss << "<Bold>Growth percentage:</> " << treeSystem.growthPercent(objectId) << "%\n";
				ss << "<Bold>Lifespan percentage:</> " << treeSystem.lifeSpanPercent(objectId) << "%\n";
				ss << "<Bold>Bonus yield:</> " << treeSystem.bonusYieldPercent(objectId) << "%";
				descriptionBox->AddRichText(ss);
			}

			if (treeSystem.HasMark(playerId(), objectId))
			{
				ss << "Marked for gather.";
				descriptionBox->AddTextWithSpacer(ss);
			}

			if (treeSystem.IsReserved(objectId))
			{
				int32_t reserverId = treeSystem.Reservation(objectId).unitId;
				UnitStateAI& unit = simulation.unitAI(reserverId);

				ss << "Reserver: \n";
				if (unit.isEnum(UnitEnum::Human)) {
					ss << " " << unit.GetUnitName() << "\n";
				}
				ss << " " << unit.unitInfo().nameStr() << " (id: " << objectId << ")";
				
				descriptionBox->AddTextWithSpacer(ss);
			}

			descriptionBox->AddRichText(ss);
			descriptionBox->AddSpacer(12);

			ss << "<Bold>Biome:</> " << simulation.terrainGenerator().GetBiomeName(tile) << "\n";
			ss << "<Bold>Fertility:</> " << simulation.terrainGenerator().GetFertilityPercent(tile) << "%\n";
			ss << "<Bold>Province flat area:</> " << simulation.provinceSystem().provinceFlatTileCount(provinceId) << " tiles\n";
			//ss << "<Bold>Region:</> (" << region.x << ", " << region.y << ")";
			descriptionBox->AddRichText(ss);

			AddBiomeDebugInfo(tile, descriptionBox);


			descriptionBox->AddSpacer();

			// Georesource (automatically add top line)
			AddGeoresourceInfo(provinceId, descriptionBox);

			// Province Upkeep Info
			AddProvinceUpkeepInfo(provinceId, descriptionBox);

			/*
			 * 
			 */

			AddSelectStartLocationButton(provinceId, descriptionBox);
			AddClaimLandButtons(provinceId, descriptionBox);

			// Selection Mesh
			FVector displayLocation = dataSource()->DisplayLocation(tile.worldAtom2());

			float selectionHeight = GameDisplayUtils::TileObjHoverMeshHeight(info, objectId, treeSystem.tileObjAge(objectId));

			SpawnSelectionMesh(assetLoader->SelectionMaterialGreen, displayLocation + FVector(0, 0, selectionHeight));

			ShowTileSelectionDecal(dataSource()->DisplayLocation(tile.worldAtom2()));
			if (simulation.tileOwner(tile) == playerId()) {
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
				FTransform transform = GameDisplayUtils::GetBushTransform(displayLocation, 0, objectId, treeSystem.tileObjAge(objectId), info, simulation.terrainGenerator().GetBiome(tile));

				// Plant uses the mesh array for multiple meshes (for example flower + its leaves)
				TArray<UStaticMesh*> assets = assetLoader->tileMeshAsset(info.treeEnum).assets;
				for (int32 i = 0; i < assets.Num(); i++) {
					SpawnMesh(assets[i], assets[i]->GetMaterial(0), transform, false, 2, info.treeEnum != TileObjEnum::GrassGreen);
				}
			}
			else if (info.type == ResourceTileType::Tree)
			{
				FTransform transform = GameDisplayUtils::GetTreeTransform(displayLocation, 0, objectId, treeSystem.tileObjAge(objectId), info);

				TArray<UStaticMesh*> assets = assetLoader->tileMeshAsset(info.treeEnum).assets;
				for (int32 i = 0; i < 2; i++) {
					SpawnMesh(assets[i], assets[i]->GetMaterial(0), transform, false, 2);
				}
			}

			descriptionBox->AfterAdd();
		}
		else if (uiState.objectType == ObjectTypeEnum::Drop)
		{
			auto& dropSystem = simulation.dropSystem();
			WorldTile2 tile(objectId);
			std::vector<DropInfo> drops = dropSystem.GetDrops(tile);

			if (drops.size() > 0)
			{
				auto& resourceSystem = simulation.resourceSystem(drops[0].playerId);

				stringstream ss;
				ss << "<Header>" << "Dropped Resource " << tile.ToString() << "</>";
				SetText(_objectDescriptionUI->DescriptionUITitle, ss);
				_objectDescriptionUI->BuildingsStatOpener->SetVisibility(ESlateVisibility::Collapsed);
				
				descriptionBox->AddLineSpacer(8);
				
				for (int i = 0; i < drops.size(); i++) {
					ss << resourceSystem.resourceCount(drops[i].holderInfo) << " " << ResourceName(drops[i].holderInfo.resourceEnum) << "\n";
				}

				descriptionBox->AddRichText(ss);

				// Selection Mesh
				SpawnSelectionMesh(assetLoader->SelectionMaterialGreen, dataSource()->DisplayLocation(tile.worldAtom2()) + FVector(0, 0, 20));

				ShowTileSelectionDecal(dataSource()->DisplayLocation(tile.worldAtom2()));

				descriptionBox->AfterAdd();
			}
			else {
				simulation.SetDescriptionUIState(DescriptionUIState(ObjectTypeEnum::None, -1));
			}
		}
		else if (uiState.objectType == ObjectTypeEnum::EmptyTile)
		{	
			WorldTile2 tile(objectId);

			AddTileInfo(tile, descriptionBox);

			descriptionBox->AfterAdd();
		}
		else if (uiState.objectType == ObjectTypeEnum::Map)
		{
			SCOPE_CYCLE_COUNTER(STAT_PunUIProvinceFocusUI);

			WorldTile2 tile(objectId);

			if (tile.isValid()) {
				int32 provinceId = simulation.GetProvinceIdClean(objectId);
				AddProvinceInfo(provinceId, descriptionBox);
			}

			descriptionBox->AfterAdd();
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
	
	bool hasChosenLocation = simulation().playerOwned(playerId()).hasChosenLocation();
	// If player hasn't select starting location
	if (!hasChosenLocation)
	{	
		bool canClaim = true;

		if (simulation().provinceOwner(provinceId) != -1) {
			descriptionBox->AddRichText("<Red>Already has owner.</>");
			canClaim = false;
		}
		
		if (!SimUtils::CanReserveSpot_NotTooCloseToAnother(provinceId, &simulation(), 2)) {
			descriptionBox->AddRichText("<Red>Too close to another town</>");
			canClaim = false;
		}

		TileArea area;
		{
			SCOPE_CYCLE_COUNTER(STAT_PunUIFindStartSpot);
			area = SimUtils::FindStartSpot(provinceId, &simulation());
		}
		
		if (!area.isValid()) {
			descriptionBox->AddRichText("<Red>Not enough buildable space.</>");
			canClaim = false;
		}


		int32 provincePrice = simulation().GetProvinceClaimPrice(provinceId, playerId());

		TArray<FText> args;
		ADDTEXT_LOCTEXT("SelectStart", "Select Starting Location\n");
		if (area.isValid()) {
			ADDTEXT_(INVTEXT("<img id=\"Coin\"/>{0}"), TextRed(FText::AsNumber(provincePrice), !canClaim));
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
	auto& playerOwned = sim.playerOwned(playerId());

	/*
	 * Not owned by anyone
	 */
	int32 provinceOwnerId = sim.provinceOwner(provinceId);
	
	if (provinceOwnerId == -1)
	{
		auto addClaimButtons = [&](ClaimConnectionEnum claimConnectionEnum)
		{
			int32 provincePrice = sim.GetProvinceClaimPrice(provinceId, playerId(), claimConnectionEnum);
			
			if (simulation().GetBiomeProvince(provinceId) == BiomeEnum::Jungle) {
				descriptionBox->AddSpacer();
				descriptionBox->AddRichText(WrapString("The difficulty in clearing jungle makes expansion more costly."));
			}

			// Claim by influence
			if (simulation().unlockedInfluence(playerId()))
			{
				bool canClaim = simulation().influence(playerId()) >= provincePrice;

				TArray<FText> args;
				AppendClaimConnectionString(args, false, claimConnectionEnum);
				ADDTEXT_(INVTEXT("\n<img id=\"Influence\"/>{0}"), TextRed(FText::AsNumber(provincePrice), !canClaim));

				descriptionBox->AddSpacer();
				descriptionBox->AddButton2Lines(JOINTEXT(args), this, CallbackEnum::ClaimLandInfluence, canClaim, false, provinceId);
			}

			// Claim by money
			{
				bool canClaim = simulation().money(playerId()) >= provincePrice;

				TArray<FText> args;
				AppendClaimConnectionString(args, false, claimConnectionEnum);
				ADDTEXT_(INVTEXT("\n<img id=\"Coin\"/>{0}"), TextRed(FText::AsNumber(provincePrice), !canClaim));

				descriptionBox->AddSpacer();
				descriptionBox->AddButton2Lines(JOINTEXT(args), this, CallbackEnum::ClaimLandMoney, canClaim, false, provinceId);
			}

			// Claim by food
			//if (simulation().IsResearched(playerId(), TechEnum::ClaimLandByFood))
			{
				int32 foodNeeded = provincePrice / FoodCost;
				bool canClaim = sim.foodCount(playerId()) >= foodNeeded;

				TArray<FText> args;
				AppendClaimConnectionString(args, false, claimConnectionEnum);
				ADDTEXT_(LOCTEXT("ClaimFood", "\n{0} food"), TextRed(FText::AsNumber(foodNeeded), !canClaim));

				descriptionBox->AddSpacer();
				descriptionBox->AddButton2Lines(JOINTEXT(args), this, CallbackEnum::ClaimLandFood, canClaim, false, provinceId);
			}
		};

		

		ClaimConnectionEnum claimConnectionEnum = sim.GetProvinceClaimConnectionEnum(provinceId, playerId());
		if (claimConnectionEnum != ClaimConnectionEnum::None) {
			addClaimButtons(claimConnectionEnum);
		}
		else if (sim.IsProvinceNextToPlayerIncludingNonFlatLand(provinceId, playerId()))
		{
			descriptionBox->AddSpacer();
			descriptionBox->AddRichText("<Red>Cannot claim province through mountain and sea</>");
		}

		descriptionBox->AddSpacer();

		//// Claim by army
		//int32 totalArmyCount = CppUtils::Sum(simulation().GetTotalArmyCounts(playerId(), true));
		//if (totalArmyCount > 0 &&
		//	playerOwned.IsProvinceClaimQueuable(provinceId))
		//{
		//	std::vector<RegionClaimProgress> claimQueue = playerOwned.armyRegionsClaimQueue();
		//	
		//	if (CppUtils::Contains(claimQueue, [&](RegionClaimProgress& claimProgress) { return claimProgress.provinceId == provinceId; }))
		//	{
		//		descriptionBox->AddButton("Cancel claim using army", nullptr, "",
		//									this, CallbackEnum::CancelClaimLandArmy, true, false, provinceId);
		//	}
		//	else if (claimQueue.size() > 0)
		//	{
		//		descriptionBox->AddButton("Claim Land with Army (Queue)", nullptr, "",
		//			this, CallbackEnum::ClaimLandArmy, true, false, provinceId);
		//	}
		//	else
		//	{
		//		descriptionBox->AddButton("Claim Land with Army", nullptr, "",
		//			this, CallbackEnum::ClaimLandArmy, true, false, provinceId);
		//	}
		//}
	}
	// Other player
	// Conquer
	else if (provinceOwnerId != playerId())
	{
		// Conquer
		/*if (simulation().unlockedInfluence(playerId()))*/
		if (sim.HasTownhall(provinceOwnerId) &&
			simulation().townhall(provinceOwnerId).provinceId() != provinceId)
		{
			if (simulation().IsResearched(playerId(), TechEnum::Conquer))
			{
				auto addAttackButtons = [&](ClaimConnectionEnum claimConnectionEnum)
				{
					ProvinceClaimProgress claimProgress = simulation().playerOwned(provinceOwnerId).GetDefendingClaimProgress(provinceId);

					// Already a claim, reinforce
					if (claimProgress.isValid())
					{
						// TODO: don't need buttons since there is already a battle UI?
						//int32 attackReinforcePrice = simulation().GetProvinceAttackReinforcePrice(provinceId, claimConnectionEnum);
						//bool canClaim = simulation().influence(playerId()) >= attackReinforcePrice;

						//stringstream ss;
						//ss << "Reinforce (Annex)\n";
						//ss << TextRed(to_string(attackReinforcePrice), !canClaim) << "<img id=\"Influence\"/>";

						//descriptionBox->AddSpacer();
						//descriptionBox->AddButton2Lines(ss.str(), this, CallbackEnum::ReinforceAttackProvince, canClaim, false, provinceId);
					}
					// Start a new claim
					else
					{
						int32 startAttackPrice = simulation().GetProvinceAttackStartPrice(provinceId, claimConnectionEnum);
						bool canClaim = simulation().influence(playerId()) >= startAttackPrice;

						TArray<FText> args;
						AppendClaimConnectionString(args, true, claimConnectionEnum);
						ADDTEXT_(INVTEXT("\n{0}<img id=\"Influence\"/>"), TextRed(FText::AsNumber(startAttackPrice), !canClaim));

						descriptionBox->AddSpacer();
						descriptionBox->AddButton2Lines(JOINTEXT(args), this, CallbackEnum::StartAttackProvince, canClaim, false, provinceId);
					}
				};

				ClaimConnectionEnum claimConnectionEnum = sim.GetProvinceClaimConnectionEnum(provinceId, playerId());
				if (claimConnectionEnum != ClaimConnectionEnum::None) {
					addAttackButtons(claimConnectionEnum);
				}
				else if (sim.IsProvinceNextToPlayerIncludingNonFlatLand(provinceId, playerId()))
				{
					descriptionBox->AddSpacer();
					descriptionBox->AddRichText("<Red>Cannot attack through mountain and sea</>");
				}
			}
		}
		
	}
	// Self, check if this province is being conquered
	else
	{
		ProvinceClaimProgress claimProgress = simulation().playerOwned(provinceOwnerId).GetDefendingClaimProgress(provinceId);

		// Already a claimProgress, and isn't the attacking player, allow anyone to help defend
		if (claimProgress.isValid() &&
			claimProgress.attackerPlayerId != playerId())
		{
			// TODO: don't need buttons since there is already a battle UI?
			//// Defend by Influence
			//if (simulation().unlockedInfluence(playerId()))
			//{
			//	bool canClaim = simulation().influence(playerId()) >= BattleInfluencePrice;

			//	std::stringstream ss;
			//	ss << "Defend Province\n";
			//	ss << TextRed(to_string(BattleInfluencePrice), !canClaim) << "<img id=\"Influence\"/>";

			//	descriptionBox->AddSpacer();
			//	descriptionBox->AddButton2Lines(ss.str(), this, CallbackEnum::DefendProvinceInfluence, canClaim, false, provinceId);
			//}

			//// Defend by money
			//{
			//	bool canClaim = simulation().money(playerId()) >= BattleInfluencePrice;

			//	std::stringstream ss;
			//	ss << "Defend Province\n";
			//	ss << TextRed(to_string(BattleInfluencePrice), !canClaim) << "<img id=\"Coin\"/>";

			//	descriptionBox->AddSpacer();
			//	descriptionBox->AddButton2Lines(ss.str(), this, CallbackEnum::DefendProvinceMoney, canClaim, false, provinceId);
			//}
		}
	}
}



void UObjectDescriptionUISystem::CallBack1(UPunWidget* punWidgetCaller, CallbackEnum callbackEnum)
{
	/*
	 * Regions
	 */
	if (callbackEnum == CallbackEnum::ClaimLandMoney ||
		callbackEnum == CallbackEnum::ClaimLandInfluence ||
		callbackEnum == CallbackEnum::ClaimLandFood ||

		callbackEnum == CallbackEnum::StartAttackProvince ||
		callbackEnum == CallbackEnum::ReinforceAttackProvince ||
		callbackEnum == CallbackEnum::DefendProvinceInfluence ||
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
			simulation().AddPopupToFront(playerId(), { "Please build the Townhall before claiming non-empty Province." }, ExclusiveUIEnum::None, "PopupCannot");
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
	else if (callbackEnum == CallbackEnum::ClaimRuin)
	{
		PUN_LOG("Claim Ruin");
		auto command = make_shared<FClaimLand>();
		command->provinceId = punWidgetCaller->callbackVar1;
		command->claimEnum = callbackEnum;
		PUN_CHECK(command->provinceId != -1);
		networkInterface()->SendNetworkCommand(command);

		CloseDescriptionUI();
	}
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
			if (punWidgetCaller->callbackVar2 == 0) {
				if (!bld.subclass<TownHall>().HasEnoughUpgradeMoney()) {
					simulation().AddPopupToFront(playerId(), { "Not enough money for upgrade." }, ExclusiveUIEnum::None, "PopupCannot");
					return;
				}
			}
			else {
				if (!bld.subclass<TownHall>().HasEnoughStoneToUpgradeWall()) {
					simulation().AddPopupToFront(playerId(), { "Not enough stone for upgrade." }, ExclusiveUIEnum::None, "PopupCannot");
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
		std::stringstream ss;
		ss << "Would you like to abandon this settlement to build a new one?";
		PopupInfo popup(playerId(), ss.str(),
			{
				"Yes, we will abandon this settlement",
				"No"
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

		networkInterface()->SendNetworkCommand(command);

		auto& playerOwned = simulation().playerOwned(playerId());
		playerOwned.SetOutputTargetDisplay(static_cast<ResourceEnum>(editableBox->callbackVar1), amount);
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
	//else if (callbackEnum == CallbackEnum::TrainUnit)
	//{
	//	if (simulation().CanTrainUnit(punWidgetCaller->callbackVar1))
	//	{
	//		auto command = make_shared<FTrainUnit>();
	//		command->buildingId = punWidgetCaller->callbackVar1;
	//		networkInterface()->SendNetworkCommand(command);
	//	}
	//}
	//else if (callbackEnum == CallbackEnum::CancelTrainUnit)
	//{
	//	int32 buildingId = punWidgetCaller->callbackVar1;
	//	if (simulation().building(buildingId).subclass<Barrack>().queueCount() > 0)
	//	{
	//		auto command = make_shared<FTrainUnit>();
	//		command->buildingId = buildingId;
	//		command->isCancel = true;
	//		networkInterface()->SendNetworkCommand(command);
	//	}
	//}
}

void UObjectDescriptionUISystem::AddBiomeInfo(WorldTile2 tile, UPunBoxWidget* descriptionBox)
{
	std::wstringstream ss;
	auto& sim = simulation();
	auto& provinceSys = sim.provinceSystem();
	auto& terrainGenerator = sim.terrainGenerator();

	BiomeEnum biomeEnum = terrainGenerator.GetBiome(tile);
	
	ss << fixed << setprecision(0);

	// Don't display Fertility on water
	if (terrainGenerator.terrainTileType(tile) == TerrainTileType::ImpassableFlat) {
		ss << "<Red>Impassable Terrain</>\n";
	}
	else if (IsWaterTileType(terrainGenerator.terrainTileType(tile))) {
		
	} else {
		ss << "<Bold>Fertility:</> " << terrainGenerator.GetFertilityPercent(tile) << "%\n";

		int32 provinceId = provinceSys.GetProvinceIdClean(tile);
		if (provinceId != -1) {
			ss << "<Bold>Province flat area:</> " << provinceSys.provinceFlatTileCount(provinceSys.GetProvinceIdClean(tile)) << " tiles\n";
			//ss << "Defense Bonus:" << (simulation().provinceSystem().provinceIsMountain(provinceId) ? "50%(Mountain)" : "0%");
			ss << "\n";
		}
	}

	FloatDet maxCelsius = sim.MaxCelsius(tile);
	FloatDet minCelsius = sim.MinCelsius(tile);
	ss << "<Bold>Temperature:</> " << FDToFloat(minCelsius) << "-" << FDToFloat(maxCelsius) << "°C ("
		<< FDToFloat(CelsiusToFahrenheit(minCelsius)) << "-" << FDToFloat(CelsiusToFahrenheit(maxCelsius)) << "°F)\n";
	//ss << "<Bold>Winter Temperature:</> " << FDToFloat(minCelsius) << "°C (" << FDToFloat(CelsiusToFahrenheit(minCelsius)) << "°F)\n";

	if (biomeEnum == BiomeEnum::Jungle) {
		ss << "<OrangeRed>Disease Frequency: 2.0 per year</>";
	} else {
		ss << "<Bold>Disease Frequency:</> 1.0 per year";
	}
	
	descriptionBox->AddRichText(ss);
	
	descriptionBox->AddSpacer(5);
	
	AddBiomeDebugInfo(tile, descriptionBox);
}

void UObjectDescriptionUISystem::AddBiomeDebugInfo(WorldTile2 tile, UPunBoxWidget* descriptionBox)
{
#if WITH_EDITOR
	auto& terrainGenerator = simulation().terrainGenerator();
	
	TArray<FText> args;
	int32 rainFall100 = terrainGenerator.GetRainfall100(tile);
	ADDTEXT_(INVTEXT("Rainfall100 {0}"), TEXT_NUM(rainFall100));
	descriptionBox->AddSpecialRichText(INVTEXT("<Yellow>"), args);
	ADDTEXT_(INVTEXT("Temp10000: {0}"), TEXT_NUM(terrainGenerator.GetTemperatureFraction10000(tile.x, rainFall100)));
	descriptionBox->AddSpecialRichText(INVTEXT("-- "), args);
#endif
}

void UObjectDescriptionUISystem::AddTileInfo(WorldTile2 tile, UPunBoxWidget* descriptionBox)
{
	WorldRegion2 region = tile.region();
	auto& sim = simulation();

	TArray<FText> args;
	ADDTEXT_(LOCTEXT("TileInfoHeader", "<Header>{0} Tile</>\n"), sim.terrainGenerator().GetBiomeNameT(tile));
	ADDTEXT_(INVTEXT("<Header>({0}, {1})</>"), tile.x, tile.y);
	SetText(_objectDescriptionUI->DescriptionUITitle, args);
	descriptionBox->AddSpacer(12);

	// Biome
	AddBiomeInfo(tile, descriptionBox);


	int32 provinceId = simulation().GetProvinceIdClean(tile);
	if (provinceId == -1) {
		return;
	}
	
	descriptionBox->AddSpacer(12);

	// Region
	//ss << "Region (" << region.x << ", " << region.y << ")";
	//descriptionBox->AddRichText(ss);

	// Region Owner
	int32 ownerId = sim.provinceOwner(provinceId);
	ADDTEXT_(LOCTEXT("TileOwner", "Owner: {0}"), (ownerId == -1 ? INVTEXT("None") : sim.playerNameT(ownerId)));
	descriptionBox->AddRichText(args);


	// - Spacer: Tile, Georesource
	descriptionBox->AddSpacer(12);

	// Georesource
	AddGeoresourceInfo(provinceId, descriptionBox, true);

	// Province Upkeep Info
	AddProvinceUpkeepInfo(provinceId, descriptionBox);

	// - Spacer: Georesource, Claim
	if (ownerId == -1) {
		descriptionBox->AddSpacer();
		descriptionBox->AddLineSpacer(15);
	}
	
	// Claim land
	AddSelectStartLocationButton(provinceId, descriptionBox);
	AddClaimLandButtons(provinceId, descriptionBox);

	// Extras
#if WITH_EDITOR
	FVector displayLocation = dataSource()->DisplayLocation(tile.worldAtom2());
	ADDTEXT_(INVTEXT("Display ({0},{1})"), displayLocation.X, displayLocation.Y);
	descriptionBox->AddSpecialRichText(INVTEXT("<Yellow>"), args);

	int32 provinceOwner = sim.provinceOwner(provinceId);
	if (provinceOwner != -1) {
		AIPlayerSystem& aiPlayer = sim.aiPlayerSystem(provinceOwner);

		if (aiPlayer.active()) {
			AIRegionStatus* regionStatus = aiPlayer.regionStatus(provinceId);
			if (regionStatus) {
				ADDTEXT_(INVTEXT("AIRegion ({0}, proposed:{1})"),
					ToFText(AIRegionPurposeName[static_cast<int>(regionStatus->currentPurpose)]),
					ToFText(AIRegionProposedPurposeName[static_cast<int>(regionStatus->proposedPurpose)])
				);
				descriptionBox->AddSpecialRichText(INVTEXT("<Yellow>"), args);
			}
		}
	}
#endif

	ShowTileSelectionDecal(dataSource()->DisplayLocation(tile.worldAtom2()));
	if (sim.tileOwner(tile) != playerId()) {
		ShowRegionSelectionDecal(tile);
	}
}

void UObjectDescriptionUISystem::AddProvinceInfo(int32 provinceId, UPunBoxWidget* descriptionBox)
{
	if (provinceId == -1) {
		return;
	}

	auto& terrainGenerator = simulation().terrainGenerator();

	TArray<FText> args;

	{
		SCOPE_CYCLE_COUNTER(STAT_PunUI_Province1);
		
		if (provinceId == OceanProvinceId)
		{
			SetText(_objectDescriptionUI->DescriptionUITitle, TEXT_TAG("<Header>", LOCTEXT("Deep Ocean", "Deep Ocean")));
			descriptionBox->AddRichText(LOCTEXT("Deep Ocean Desc", "Large body of water."));
			return;
		}

		if (provinceId == MountainProvinceId)
		{
			SetText(_objectDescriptionUI->DescriptionUITitle, TEXT_TAG("<Header>", LOCTEXT("High Mountain", "High Mountain")));
			descriptionBox->AddRichText(LOCTEXT("High Mountain Desc", "Impassable unclaimed mountain."));
			return;
		}

		if (provinceId == RiverProvinceId ||
			provinceId == EmptyProvinceId)
		{
			SetText(_objectDescriptionUI->DescriptionUITitle, TEXT_TAG("<Header>", LOCTEXT("Unusable Land", "Unusable Land")));
			descriptionBox->AddRichText(LOCTEXT("Unusable Land Desc", "Land without any use that no one bothers with."));
			return;
		}
	}

	provinceId = abs(provinceId);

	WorldTile2 provinceCenter = simulation().provinceSystem().GetProvinceCenterTile(provinceId);

	// Biome
	{
		SCOPE_CYCLE_COUNTER(STAT_PunUI_ProvinceAddBiomeInfo);

		const FText provinceText = LOCTEXT("Province", "Province");

		ADDTEXT_(INVTEXT("<Header>{0} {1}</>\n"), terrainGenerator.GetBiomeNameT(provinceCenter), provinceText);
		SetText(_objectDescriptionUI->DescriptionUITitle, args);
		descriptionBox->AddSpacer(12);

		// Biome Description
		FString wrappedDescription = WrapStringF(terrainGenerator.GetBiomeInfoFromTile(provinceCenter).description.ToString());
		descriptionBox->AddRichText(FText::FromString(wrappedDescription));

		descriptionBox->AddSpacer(12);

		// Biome number info
		AddBiomeInfo(provinceCenter, descriptionBox);

		descriptionBox->AddSpacer();
	}

	// Georesource
	AddGeoresourceInfo(provinceId, descriptionBox);

	// Province Upkeep Info
	AddProvinceUpkeepInfo(provinceId, descriptionBox);

	// Claim land
	AddSelectStartLocationButton(provinceId, descriptionBox);
	AddClaimLandButtons(provinceId, descriptionBox);

	// If no longer in map mode, don't display the selections
	ShowRegionSelectionDecal(provinceCenter);
}

void UObjectDescriptionUISystem::AddProvinceUpkeepInfo(int32 provinceIdClean, UPunBoxWidget* descriptionBox)
{
	SCOPE_CYCLE_COUNTER(STAT_PunUI_AddProvinceUpkeepInfo);
	
	if (provinceIdClean == -1) {
		return;
	}

	descriptionBox->AddLineSpacer(15);

	auto& sim = simulation();
	int32 provinceOwnerId = sim.provinceOwner(provinceIdClean);
	bool unlockedInfluence = sim.unlockedInfluence(playerId());
	
	stringstream ss;
	ss << fixed << setprecision(1);
	ss << "<Subheader>Province Info:</>\n";

	// Already own this province, Showr real income/upkeep
	if (provinceOwnerId == playerId())
	{
		ss << "Income: <img id=\"Coin\"/>" << sim.GetProvinceIncome100(provinceIdClean) / 100.0f;
		descriptionBox->AddRichText(ss);
		
		if (unlockedInfluence) {
			ss << "Upkeep: <img id=\"Influence\"/>" << sim.GetProvinceUpkeep100(provinceIdClean, provinceOwnerId) / 100.0f;
			descriptionBox->AddRichText(ss);

			if (sim.IsBorderProvince(provinceIdClean)) {
				ss << "Border Upkeep: <img id=\"Influence\"/>5";
				descriptionBox->AddRichText(ss);
			}
		}
		
		ss << "Defense Bonus: " << sim.GetProvinceAttackCostPercent(provinceIdClean) << "%";
		auto widget = descriptionBox->AddRichText(ss);
		AddToolTip(widget, sim.GetProvinceDefenseBonusTip(provinceIdClean));
	}
	// Other player's Home Province
	else if (provinceOwnerId != -1 && sim.homeProvinceId(provinceOwnerId) == provinceIdClean)
	{
		ss << "Home Province of " << sim.playerName(provinceOwnerId);
		descriptionBox->AddRichText(ss);
		
		ss << "Vassalize Defense Bonus: " << sim.GetProvinceVassalizeDefenseBonus(provinceIdClean) << "%";
		auto widget = descriptionBox->AddRichText(ss);
		AddToolTip(widget, sim.GetProvinceVassalizeDefenseBonusTip(provinceIdClean));
	}
	else 
	{
		ss << "Base Income: <img id=\"Coin\"/>" << sim.GetProvinceIncome100(provinceIdClean) / 100.0f;
		descriptionBox->AddRichText(ss);
		
		if (unlockedInfluence) {
			ss << "Base Upkeep: <img id=\"Influence\"/>" << sim.GetProvinceBaseUpkeep100(provinceIdClean) / 100.0f;
			descriptionBox->AddRichText(ss);
		}
		
		ss << "Defense Bonus: " << sim.GetProvinceAttackCostPercent(provinceIdClean) << "%";
		auto widget = descriptionBox->AddRichText(ss);
		AddToolTip(widget, sim.GetProvinceDefenseBonusTip(provinceIdClean));
	}
	
	descriptionBox->AddSpacer(12);
}

void UObjectDescriptionUISystem::AddGeoresourceInfo(int32 provinceId, UPunBoxWidget* descriptionBox, bool showTopLine)
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
	
	if (node.HasResource())
	{
		if (showTopLine) {
			descriptionBox->AddLineSpacer(15);
		}
		
		stringstream ss;
		ss << "<Header>" << node.info().name << "</>";
		descriptionBox->AddRichText(ss);
		descriptionBox->AddSpacer(8);
		
		ss << node.info().description;
		descriptionBox->AddRichText(ss);
		descriptionBox->AddSpacer();

		if (node.depositAmount > 0 || isMountain) {
			descriptionBox->AddRichText("Resource amount:");

			if (node.depositAmount > 0) {
				descriptionBox->AddIconPair(FText(), node.info().resourceEnum, TEXT_NUM(node.depositAmount));
			}
			if (isMountain) {
				descriptionBox->AddIconPair(FText(), ResourceEnum::Stone, TEXT_NUM(node.stoneAmount));
			}
			
			descriptionBox->AddSpacer();
		}

	}
	else if (isMountain)
	{
		if (showTopLine) {
			descriptionBox->AddLineSpacer(15);
		}
		
		descriptionBox->AddRichText(LOCTEXT("Stone deposit:", "Stone deposit:"));
		descriptionBox->AddIconPair(FText(), ResourceEnum::Stone, TEXT_NUM(node.stoneAmount));
	}
}

void UObjectDescriptionUISystem::AddEfficiencyText(Building& building, UPunBoxWidget* descriptionBox)
{
	const FText efficiencyText = LOCTEXT("Efficiency", "Efficiency");
	const FText baseText = LOCTEXT("Base", "Base");
	
	auto widget = descriptionBox->AddRichText(efficiencyText, TEXT_PERCENT(building.efficiency()));

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

void UObjectDescriptionUISystem::AddTradeFeeText(TradeBuilding& building, UPunBoxWidget* descriptionBox)
{
	const FText tradingFeeText = LOCTEXT("Trade Fee:", "Trade Fee:");
	
	auto feeText = descriptionBox->AddRichText(tradingFeeText, TEXT_PERCENT(building.tradingFeePercent()));

	// Tip
	TArray<FText> args;
	ADDTEXT_(INVTEXT("<Bold>{0}</>\n"), tradingFeeText);
	ADDTEXT_(LOCTEXT("base {0}%\n", "base {0}%\n"), TEXT_NUM(building.baseTradingFeePercent()));
	std::vector<BonusPair> bonuses = building.GetTradingFeeBonuses();
	for (const auto& bonus : bonuses) {
		ADDTEXT_(INVTEXT("{0}% {1}\n"), TEXT_NUM(bonus.value), bonus.name);
	}
	AddToolTip(feeText, args);
}


#undef LOCTEXT_NAMESPACE