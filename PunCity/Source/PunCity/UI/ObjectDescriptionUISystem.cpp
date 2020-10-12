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

DECLARE_CYCLE_STAT(TEXT("PUN: [UI]Select_Start"), STAT_PunUISelect_Start, STATGROUP_Game);

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
				// Highlight Hover if the province isn't owned or if the camera is zoomed out
				if (dataSource()->zoomDistance() > WorldZoomTransition_RegionToRegion4x4_Mid ||
					simulation().provinceOwner(provinceId) == -1 ||
					dataSource()->alwaysShowProvinceHover())
				{
					showHover = true;
					ShowRegionSelectionDecal(hitTile, true);
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
	if (InterfacesInvalid()) return;

	FHitResult hitResult;
	bool didHit = networkInterface()->ControllerGetMouseHit(hitResult, ECC_Pawn);

	// Don't hit if there is pointer on UI
	if (IsPointerOnUI()) {
		didHit = false;
	}

	if (dataSource()->isPhotoMode()) {
		dataSource()->simulation().SetDescriptionUIState(DescriptionUIState());
		UpdateDescriptionUI();
		return;
	}

	if (didHit)
	{
		UPrimitiveComponent* hitComponent = hitResult.GetComponent();
		int32 hitIndex = hitResult.Item;

		//PUN_LOG("First Hit: %s", *hitResult.ToString());

		GameSimulationCore& simulation = dataSource()->simulation();
		
		// Hit ground, create Surrounding colliders and redo the test..
		if (hitComponent && hitComponent->ComponentTags.Num())
		{
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
					if (TEXT("WorldMap") == typeName) {
						uiState.objectType = ObjectTypeEnum::Map;
						uiState.objectId = simulation.provinceSystem().GetProvinceIdRaw(worldTile);;

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

			simulation.SetDescriptionUIState(uiState);
		}

		UpdateDescriptionUI();
		return;
	}

	UpdateDescriptionUI();
}

void UObjectDescriptionUISystem::UpdateDescriptionUI()
{
	if (InterfacesInvalid()) return;

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
	// TODO: this seems to fire every tick...
	if (_objectDescriptionUI->state != uiState) {
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
			stringstream ss;

			/*
			 * Header
			 */
			if (buildingEnum == CardEnum::RegionTribalVillage) {
				ss << "<Header>" << GenerateTribeName(objectId) << " tribe</>";
			}
			else if (buildingEnum == CardEnum::Townhall) {
				ss << "<Header>" << TrimString_Dots(simulation.townName(building.playerId()), 15) << "</>";
			}
			else
			{
				ss << "<Header>" << building.buildingInfo().name << "</>";
			}
			
#if WITH_EDITOR || TRAILER_MODE
			ss << "[" << objectId << "]"; // ID
			ss << "\n" << building.centerTile().ToString();
			//ss << "GateTile:(" << building.gateTile().x << "," << building.gateTile().y << ")\n";
#endif
			SetText(_objectDescriptionUI->DescriptionUITitle, ss);
			_objectDescriptionUI->BuildingsStatOpener->SetVisibility(building.maxOccupants() > 0 && !IsHouse(building.buildingEnum()) ? ESlateVisibility::Visible : ESlateVisibility::Collapsed);
			_objectDescriptionUI->NameEditButton->SetVisibility((building.isEnum(CardEnum::Townhall) && building.playerId() == playerId()) ? ESlateVisibility::Visible : ESlateVisibility::Collapsed);
			
			//descriptionBox->AddRichText(ss);
			descriptionBox->AddLineSpacer(8);

			/*
			 * Body
			 */
			if (building.isOnFire())
			{
				ss << "<Red>Building is on fire.</>";
				descriptionBox->AddRichText(ss);
			}
			else if (building.isBurnedRuin())
			{
				ss << "<Red>Burned ruin.</>";
				descriptionBox->AddRichText(ss);
			}
			else
			{

				if (!building.isConstructed()) {
					ss << "Under construction\n";
					descriptionBox->AddRichText(ss);
				}

				// Occupants
				if (IsHouse(building.buildingEnum()) && building.isConstructed()) {
					ss << "<img id=\"House\"/>" << building.occupantCount() << "/" << building.allowedOccupants(); // House's allowedOccupants
					descriptionBox->AddRichText("Occupants", ss);

#if WITH_EDITOR
					ss << "Adjacent" << building.adjacentCount(CardEnum::House);
					descriptionBox->AddRichText("<Yellow>Occupants</>", ss);
#endif					
				}
				else if (building.maxOccupants() > 0) {
					ss << building.occupantCount() << "/" << building.maxOccupants();
					descriptionBox->AddRichText("Workers: ", ss);
				}

				// Upkeep
				int32 upkeep = building.upkeep();
				if (building.isConstructed() && upkeep > 0)
				{
					ss << "<img id=\"Coin\"/>" << upkeep;
					descriptionBox->AddRichText("Upkeep: ", ss);
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
						
						descriptionBox->AddRichText("Appeal", to_string(house->GetAppealPercent()) + "%");

						// Happiness
						{
							//UIconTextPairWidget* widget = descriptionBox->AddIconPair("", assetLoader->SmileIcon, to_string(house->totalHappiness()));
							//descriptionBox->AddSpacer();

							ss << house->housingHappiness() << "%";
							auto widget = descriptionBox->AddRichText("Housing quality", ss);
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
							ss << fixed << setprecision(1);
							ss << house->totalHouseIncome100() / 100.0f << "<img id=\"Coin\"/>";
							auto widget = descriptionBox->AddRichText("Income", ss);
							descriptionBox->AddSpacer();

							// Single house tax...
							ss << "Income: " << house->totalHouseIncome100() / 100.0f << "<img id=\"Coin\"/>\n";

							for (int32 i = 0; i < HouseIncomeEnumCount; i++) {
								int32 income100 = house->GetIncome100Int(i);
								if (income100 != 0) {
									ss << (income100 > 0 ? " +" : " ") << (income100 / 100.0f) << " " << IncomeEnumName[i] << "\n";
								}
							}

							AddToolTip(widget, ss);
						}

						// Science
						{
							ss << fixed << setprecision(1);
							ss << house->science100PerRound() / 100.0f << "<img id=\"Science\"/>";
							auto widget = descriptionBox->AddRichText("Science", ss);
							descriptionBox->AddSpacer();

							ss << "Science output: " << house->science100PerRound() / 100.0f << "<img id=\"Science\"/>\n";

							for (int32 i = 0; i < HouseScienceEnums.size(); i++) {
								int32 science100 = house->GetScience100(HouseScienceEnums[i]);
								if (science100 != 0) {
									ss << (science100 > 0 ? " +" : " ") << science100 / 100.0f << " " << ScienceEnumName[static_cast<int>(HouseScienceEnums[i])] << "\n";
								}
							}
							
							AddToolTip(widget, ss);
						}

						{
							descriptionBox->AddLineSpacer();
							descriptionBox->AddSpacer();
							ss << "<Header>Level " << houseLvl << "</>";
							descriptionBox->AddRichText(ss);

							if (houseLvl == house->GetMaxHouseLvl()) {
								ss << "Maximum lvl.";
								descriptionBox->AddRichText(ss);
							}
							else if (house->GetAppealPercent() < appealNeeded) {
								ss << "<Orange>Need " + to_string(appealNeeded) + " appeal to increase lvl.</>";
								descriptionBox->AddRichText(ss);
							}
							else {
								ss << "<Orange>" + house->HouseNeedDescription() + "</>";
								auto widget = descriptionBox->AddRichText(ss);
								AddToolTip(widget, house->HouseNeedTooltipDescription());
							}

							descriptionBox->AddSpacer();

							auto addCheckBoxIconPair = [&](ResourceEnum resourceEnum) {
								UIconTextPairWidget* widget = descriptionBox->AddIconPair("", resourceEnum, to_string(building.resourceCount(resourceEnum)));
								widget->UpdateAllowCheckBox(resourceEnum);
								widget->ObjectId = building.buildingId();
							};

							descriptionBox->AddRichText("<Subheader>Fuel:</>");
							addCheckBoxIconPair(ResourceEnum::Wood);
							addCheckBoxIconPair(ResourceEnum::Coal);

							descriptionBox->AddRichText("<Subheader>Luxury tier 1:</>");
							const std::vector<ResourceEnum>& luxury1 = GetLuxuryResourcesByTier(1);
							for (size_t i = 0; i < luxury1.size(); i++) {
								addCheckBoxIconPair(luxury1[i]);
							}

							if (houseLvl >= 3)
							{
								descriptionBox->AddRichText("<Subheader>Luxury tier 2:</>");
								const std::vector<ResourceEnum>& luxury2 = GetLuxuryResourcesByTier(2);
								for (size_t i = 0; i < luxury2.size(); i++) {
									addCheckBoxIconPair(luxury2[i]);
								}
							}
							if (houseLvl >= 5)
							{
								descriptionBox->AddRichText("<Subheader>Luxury tier 3:</>");
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
						auto& townhallPlayerOwned = simulation.playerOwned(townhall.playerId());
						int32 lvl = townhall.townhallLvl;

						descriptionBox->AddRichText("Player", TrimString_Dots(simulation.playerName(townhall.playerId()), 12));

#if WITH_EDITOR
						descriptionBox->AddRichText("<Yellow>PlayerId</>", to_string(townhall.playerId()));
#endif
						
						descriptionBox->AddRichText("Size", townhallPlayerOwned.GetTownSizeName());

						descriptionBox->AddRichText("Town Level", to_string(lvl));

						// Only show these if it is player's townhall
						if (townhall.playerId() == playerId()) {
							descriptionBox->AddRichText("Townhall Income", "<img id=\"Coin\"/>" + to_string(townhall.townhallIncome()));
						}

						// Show how much money he has..
						if (townhall.playerId() != playerId()) {
							ss << simulation.resourceSystem(townhall.playerId()).money() << "<img id=\"Coin\"/>";
							descriptionBox->AddRichText("Money", ss);

							ss << fixed << setprecision(1);
							ss << townhallPlayerOwned.totalIncome100() / 100.0f << "<img id=\"Coin\"/>";
							descriptionBox->AddRichText("Income", ss);

							ss << simulation.GetAverageHappiness(townhall.playerId()) << "<img id=\"Smile\"/>";
							descriptionBox->AddRichText("Happiness", ss);
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
								ss << simulation.townhall(allyPlayerIds[i]).townName() << "\n";
							}
							
							descriptionBox->AddRichText("Allies", simulation.townhall(allyPlayerIds[0]).townName(), ResourceEnum::None, ss.str());
						} else {
							descriptionBox->AddRichText("Allies", "None");
						}

						// Vassals
						const auto& vassalBuildingIds = townhallPlayerOwned.vassalBuildingIds();
						{
							std::stringstream vassalSS;

							auto getVassalName = [&](int32 vassalBuildingId)
							{
								Building& vassalBld = simulation.building(vassalBuildingId);
								if (vassalBld.isEnum(CardEnum::Townhall)) {
									return simulation.townhall(vassalBld.playerId()).townName();
								}
								else {
									return string("Non-player Vassal");
								}
							};
							
							for (int32 i = 1; i < vassalBuildingIds.size(); i++) {
								vassalSS << getVassalName(vassalBuildingIds[i]) << "\n";
							}
							
							if (vassalBuildingIds.size() > 0) {
								descriptionBox->AddRichText("Vassals", getVassalName(vassalBuildingIds[0]), ResourceEnum::None, vassalSS.str());
							} else {
								descriptionBox->AddRichText("Vassals", "None");
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
							auto button = descriptionBox->AddButtonRed("", "Abandon town", nullptr, "", this, CallbackEnum::AbandonTown);
							AddToolTip(button, "Abandon this town to build a new one. (Destroy this town)");
							descriptionBox->AddLineSpacer(10);
						}
						else
						{
							// Vassalize
							if (simulation.IsResearched(playerId(), TechEnum::Vassalize))
							{

								
							}
							//int32 startAttackPrice = simulation().GetProvinceAttackStartPrice(provinceId);
							//bool canClaim = simulation().influence(playerId()) >= startAttackPrice;

							//stringstream ss;
							//ss << "Conquer Province (Annex)\n";
							//ss << TextRed(to_string(startAttackPrice), !canClaim) << "<img id=\"Influence\"/>";

							//descriptionBox->AddSpacer();
							//descriptionBox->AddButton2Lines(ss.str(), this, CallbackEnum::StartAttackProvince, canClaim, false, provinceId);
						}

						//ss << "\nBirth: " << StatSystem::StatsToString(statSystem.GetStat(SeasonStatEnum::Birth));
						//ss << "Death(Age): " << StatSystem::StatsToString(statSystem.GetStat(SeasonStatEnum::DeathAge));
						//ss << "Death(Starve): " << StatSystem::StatsToString(statSystem.GetStat(SeasonStatEnum::DeathStarve));
						//ss << "Death(Cold): " << StatSystem::StatsToString(statSystem.GetStat(SeasonStatEnum::DeathCold));
						//statisticsBox->AddText(ss);

						//ss << "\nGarrisoned Player: " << to_string(townhall.garrisons.playerId()) << "\n";
						//if (townhall.garrisons.Count() > 0) {
						//	ss << "\nGarrisons:\n";
						//	for (int i = 0; i < townhall.garrisons.Count(); i++) {
						//		ss << townhall.garrisons.Get(i) << "\n";
						//	}
						//}

						//statisticsBox->AddText(ss);
						//statisticsBox->AfterAdd();
					}
					else if (building.isEnum(CardEnum::Bank) ||
							building.isEnum(CardEnum::InvestmentBank)) 
					{
						ss << WrapString(building.buildingInfo().description);
						descriptionBox->AddRichText(ss);
						descriptionBox->AddSpacer();
						
						ss << static_cast<Bank*>(&building)->roundProfit << "<img id=\"Coin\"/>";
						descriptionBox->AddRichText("Round profit", ss);
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
						ss << to_string(fertility) << "%";
						descriptionBox->AddRichText("Fertility", ss);

						AddEfficiencyText(farm, descriptionBox);

						descriptionBox->AddRichText("Work stage", farm.farmStageName());
						descriptionBox->AddRichText("Growth percent", to_string(farm.MinCropGrowthPercent()) + "%");

						//if (static_cast<Farm*>(&building)->hasAdjacencyBonus) {
						//	ss << "Bonus: +" << building.adjacentCount(BuildingEnum::Farm) * 10 << "% productivity\n\n";
						//}
#if WITH_EDITOR 
						ss << "(Editor)WorkedTiles: " << farm.workedTiles() << "\n";
						ss << "(Editor)adjacents: +" << building.adjacentCount() << "\n";
						descriptionBox->AddSpecialRichText("<Yellow>", ss);
#endif
						descriptionBox->AddRichText("Current", GetTileObjInfo(farm.currentPlantEnum).name);
					}
					// TODO: delete this??
					else if (building.isEnum(CardEnum::RanchBarn))
					{
						RanchBarn& barn = building.subclass<RanchBarn>();
						ss << "\n" << barn.inventory.ToString();
						ss << "\nAnimals: " << barn.animalOccupants().size() << "/" << barn.maxAnimals << "\n";
						descriptionBox->AddText(ss);

						// Selection Meshes
						const std::vector<int32_t>& occupants = barn.animalOccupants();
						for (int occupantId : occupants) {
							WorldAtom2 atom = dataSource()->unitDataSource().actualAtomLocation(occupantId);
							SpawnSelectionMesh(assetLoader->SelectionMaterialYellow, dataSource()->DisplayLocation(atom) + FVector(0, 0, 30));
						}
					}
					else if (IsRanch(building.buildingEnum()))
					{
						Ranch& ranch = building.subclass<Ranch>();
						ss << ranch.animalOccupants().size() << "/" << ranch.maxAnimals;
						descriptionBox->AddRichText("Animals:", ss);

						// Selection Meshes
						const std::vector<int32_t>& occupants = ranch.animalOccupants();
						for (int occupantId : occupants) {
							WorldAtom2 atom = dataSource()->unitDataSource().actualAtomLocation(occupantId);
							SpawnSelectionMesh(assetLoader->SelectionMaterialYellow, dataSource()->DisplayLocation(atom) + FVector(0, 0, 30));
						}
					}
					// TODO: Boar should just use the normal inventory?
					else if (building.isEnum(CardEnum::BoarBurrow)) {
						ss << WrapString(building.buildingInfo().description) << "\n";
						ss << building.subclass<BoarBurrow>().inventory.ToString();
						descriptionBox->AddText(ss);
					}
					//else if (IsBarrack(building.buildingEnum()))
					//{
					//	Barrack& barrack = building.subclass<Barrack>();
					//	ArmyInfo info = barrack.armyInfo();

					//	// The rest only show if it is the actual player
					//	if (barrack.playerId() == playerId())
					//	{
					//		descriptionBox->AddRichText("Trainings Queued", to_string(barrack.queueCount()));

					//		ss << std::fixed << std::showpoint << std::setprecision(1);
					//		ss << barrack.trainingPercent() << "%";
					//		descriptionBox->AddRichText("Training Progress", ss);

					//		ss << "Train " << info.name;
					//		ss << "\n";
					//		if (info.moneyCost > 0) {
					//			ss << MaybeRedText(to_string(info.moneyCost), simulation.money(playerId()) < info.moneyCost) << "<img id=\"Coin\"/> ";
					//		}

					//		for (const ResourcePair& pair : info.resourceCost) {
					//			if (pair.resourceEnum == ResourceEnum::Wood) {
					//				ss << MaybeRedText(to_string(pair.count), simulation.resourceCount(playerId(), pair.resourceEnum) < pair.count) << "<img id=\"Wood\"/>";
					//			}
					//			else if (pair.resourceEnum == ResourceEnum::Iron) {
					//				ss << MaybeRedText(to_string(pair.count), simulation.resourceCount(playerId(), pair.resourceEnum) < pair.count) << "<img id=\"Iron\"/>";
					//			}
					//			else if (pair.resourceEnum == ResourceEnum::Food) {
					//				ss << MaybeRedText(to_string(pair.count), simulation.foodCount(playerId()) < pair.count) << " food";
					//			}
					//		}

					//		bool isTrainable = true;

					//		descriptionBox->AddButton(ss.str(), "", nullptr, "", this, CallbackEnum::TrainUnit, isTrainable, false, objectId);

					//		// Show cancel button if there is a queued unit
					//		descriptionBox->AddButton("Cancel Training", "", nullptr, "", this, CallbackEnum::CancelTrainUnit, barrack.queueCount() > 0, false, objectId);


					//		ss.str(string());
					//	}
					//}
					else if (IsSpecialProducer(building.buildingEnum())) 
					{
						AddEfficiencyText(building, descriptionBox);

						if (building.isEnum(CardEnum::Mint)) {
							ss << "<img id=\"Coin\"/>" << building.seasonalProduction();
							descriptionBox->AddRichText("Income(per season)", ss);
						}
						if (building.isEnum(CardEnum::InventorsWorkshop)) {
							ss << "<img id=\"Science\"/>" << building.seasonalProduction();
							descriptionBox->AddRichText("Science(per season)", ss);
						}
						if (IsBarrack(building.buildingEnum())) {
							ss << "<img id=\"Influence\"/>" << building.seasonalProduction();
							descriptionBox->AddRichText("Influence(per season)", ss);
						}
					}
					else if (IsStorage(building.buildingEnum()))
					{
						StorageYard& storage = building.subclass<StorageYard>();
						ss << storage.tilesOccupied() << "/" << storage.storageSlotCount();
						descriptionBox->AddRichText("Slots", ss);

						descriptionBox->AddSpacer(12);
						
					}
					else if (building.isEnum(CardEnum::Tavern) || 
							building.isEnum(CardEnum::Theatre))
					{
						FunBuilding& funBuilding = building.subclass<FunBuilding>();
						ss << (funBuilding.guestCountLastRound() + funBuilding.guestCountThisRound());// << "/" << funBuilding.maxGuestsPerSeason();
						descriptionBox->AddRichText("Guests: ", ss);

						descriptionBox->AddRichText("Service Quality: ", std::to_string(funBuilding.serviceQuality()));
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
						ss << building.buildingInfo().description;
						descriptionBox->AddRichText(ss);
					}
					else if (building.isEnum(CardEnum::OreSupplier)) {
						ss << building.buildingInfo().description;
						descriptionBox->AddRichText(ss);
						descriptionBox->AddLineSpacer();

						int32_t targetBuyAmount = building.subclass<OreSupplier>().maxBuyAmount();
						descriptionBox->AddRichText("Resource Type", "Iron ore");
						descriptionBox->AddRichText("Target Buy Amount", to_string(targetBuyAmount));
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
							ss << "-- Ticks from last check: " << tradingPost->ticksFromLastCheck() << " ticks";
							descriptionBox->AddText(ss);
							ss << "-- lastCheckTick: " << tradingPost->lastCheckTick() << " secs";
							descriptionBox->AddText(ss);
#endif
							ss << countdown << " secs";
							descriptionBox->AddRichText("Trade complete in", ss);

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

								if (tradingCompany.isImport) {
									descriptionBox->AddRichText("Import", ToSignedNumber(tradingCompany.importMoney()) + "<img id=\"Coin\"/>");
								}
								else {
									descriptionBox->AddRichText("Export", ToSignedNumber(tradingCompany.exportMoney()) + "<img id=\"Coin\"/>");
								}

								descriptionBox->AddLineSpacer();
							}

							if (tradingCompany.activeResourceEnum == ResourceEnum::None) {
								descriptionBox->AddRichText("<Red>Setup automatic trade below.</>");
							}
							else {
								ss << (tradingCompany.isImport ? "Importing " : "Exporting ");
								ss << " until ";
								ss << tradingCompany.targetAmount;
								descriptionBox->AddIconPair(ss.str(), tradingCompany.activeResourceEnum, " target");
								ss.str("");

								if (tradingCompany.activeResourceEnum != ResourceEnum::None)
								{
									int32 target = tradingCompany.targetAmount;
									int32 count = resourceSys.resourceCount(tradingCompany.activeResourceEnum);

									if (tradingCompany.isImport) {
										if (target > count) {
											ss << "(import remaining: " << (target - count) << ")";
										}
										else {
											ss << "(import storage-target reached)";
										}
									}
									else {
										// Export
										if (target < count) {
											ss << "(export remaining: " << (count - target) << ")";
										}
										else {
											ss << "(resources in storage already below storage-target)";
										}
									}
									descriptionBox->AddRichText(ss.str());
									ss.str("");
								}
							}

							descriptionBox->AddSpacer(12);

							if (building.playerId() == playerId())
							{
								descriptionBox->AddDropdown(
									building.buildingId(),
									{ "Import", "Export" },
									tradingCompany.isImport ? "Import" : "Export",
									[](int32 objectId, FString sItem, IGameUIDataSource* dataSource, IGameNetworkInterface* networkInterface)
								{
									auto& trader = dataSource->simulation().building(objectId).subclass<TradingCompany>(CardEnum::TradingCompany);

									auto command = make_shared<FChangeWorkMode>();
									command->buildingId = objectId;
									command->intVar1 = static_cast<int32>(trader.activeResourceEnum);
									command->intVar2 = (sItem == FString("Import")) ? 1 : 0;
									command->intVar3 = static_cast<int32>(trader.targetAmount);
									networkInterface->SendNetworkCommand(command);
								});
							}

							descriptionBox->AddSpacer(12);

							// Show choose box
							descriptionBox->AddChooseResourceElement(tradingCompany.activeResourceEnum, this, CallbackEnum::OpenChooseResource);

							descriptionBox->AddSpacer(12);

							int32 targetAmount = tradingCompany.targetAmount;
							if (Time::Ticks() - tradingCompany.lastTargetSetTick < Time::TicksPerSecond * 3) {
								targetAmount = tradingCompany.lastTargetAmountSet;
							}
							descriptionBox->AddEditableNumberBox(this, CallbackEnum::EditNumberChooseResource, building.buildingId(), "Target: ", targetAmount);

							descriptionBox->AddLineSpacer(12);
							if (tradingCompany.HasPendingTrade()) {
								ss << tradingCompany.CountdownSecondsDisplay() << " secs";
								descriptionBox->AddRichText("Trade complete in", ss);
							}
							else 
							{
								if (tradingCompany.hoverWarning == HoverWarning::NotEnoughMoney) {
									descriptionBox->AddRichText("<Red>Import Failed</>", ss);
									descriptionBox->AddRichText("<Red>Not enough Money</>", ss);
								}
								else if (tradingCompany.hoverWarning == HoverWarning::AlreadyReachedTarget) {
									descriptionBox->AddRichText("<Red>Import Failed</>", ss);
									descriptionBox->AddRichText("<Red>Already reached import target</>", ss);
								}
								else if (tradingCompany.hoverWarning == HoverWarning::ResourcesBelowTarget) {
									descriptionBox->AddRichText("<Red>Export Failed</>", ss);
									descriptionBox->AddRichText("<Red>Resource count below storage target</>", ss);
								}
								
								ss << max(0, tradingCompany.TradeRetryCountDownTicks() / Time::TicksPerSecond) << " secs";
								descriptionBox->AddRichText("Retry trade in", ss);
							}

							// Dropdown / EditableNumberBox set in ObjectDescriptionUI.cpp
							//_objectDescriptionUI->SetEditableNumberBox(building.buildingId(), this, CallbackEnum::EditNumberChooseResource);

							/*
							 * Fill choose resource box
							 */
							UPunBoxWidget* chooseResourceBox = _objectDescriptionUI->ChooseResourceBox;
							FString searchString = _objectDescriptionUI->SearchBox->GetText().ToString();

							for (const ResourceInfo& info : SortedNameResourceEnum)
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
					else if (IsMountainMine(building.buildingEnum()))
					{
						int32 oreLeft = building.subclass<Mine>().oreLeft();
						if (oreLeft > 0) {
							descriptionBox->AddRichText("Resource left", to_string(oreLeft), building.product());
						} else {
							ss << "<Red>Mine Depleted</>";
							descriptionBox->AddRichText(ss);
						}
					}
					else if (IsRegionalBuilding(building.buildingEnum()))
					{
						descriptionBox->AddRichText(building.buildingInfo().description);
					}
					else if (building.isEnum(CardEnum::Colony))
					{
						auto& colony = building.subclass<Colony>();
						ss << colony.GetColonyUpkeep() << "<img id=\"Influence\"/>";
						descriptionBox->AddRichText("Upkeep: ", ss);

						descriptionBox->AddSpacer();

						ResourceEnum resourceEnum = colony.GetColonyResourceEnum();
						int32 resourceIncome = colony.GetColonyResourceIncome(resourceEnum);
						descriptionBox->AddRichText("Production (per round):", to_string(resourceIncome), resourceEnum);

						descriptionBox->AddSpacer();

						// Mine Resources
						if (IsOreEnum(resourceEnum))
						{
							int32 oreLeft = building.oreLeft();
							if (oreLeft > 0) {
								descriptionBox->AddRichText("Resource left", to_string(oreLeft), building.product());
							}
							else {
								ss << "<Red>Mine Depleted</>";
								descriptionBox->AddRichText(ss);
							}
						}
					}
					else if (building.isEnum(CardEnum::Fort))
					{
						descriptionBox->AddRichText("Attacking this province requires 100% more <img id=\"Influence\"/>.");
					}
					else {
						//ss << building.buildingInfo().description;
						//descriptionBox->AddRichText(ss);
					}
				}


				/*
				 * Special case: Manage Storage (Display regardless of if the storage is constructed
				 */
				if (IsStorage(building.buildingEnum()) &&
					building.ownedBy(playerId()))
				{
					descriptionBox->AddButton("Manage Storage", nullptr, "", this, CallbackEnum::OpenManageStorage, true, false, objectId);
					descriptionBox->AddLineSpacer(12);

					/*
					 * Fill ManageStorage
					 */
					StorageYard& storage = building.subclass<StorageYard>();
					UPunBoxWidget* manageStorageBox = _objectDescriptionUI->ManageStorageBox;

					std::vector<ResourceInfo> resourceEnums = SortedNameResourceEnum;

					auto tryAddManageStorageElement_UnderExpansion = [&](ResourceEnum resourceEnum, bool isShowing)
					{
						// Note: isShowing is needed since we should do RemoveIf even if we aren't showing the resource row 
						if (isShowing) {
							bool isAllowed = storage.ResourceAllowed(resourceEnum);
							ECheckBoxState checkState = (isAllowed ? ECheckBoxState::Checked : ECheckBoxState::Unchecked);
							manageStorageBox->AddManageStorageElement(resourceEnum, "", objectId, checkState, true);
						}
						CppUtils::RemoveIf(resourceEnums, [&](ResourceInfo resourceInfo) { return resourceInfo.resourceEnum == resourceEnum; });
					};

					// Food
					{
						int32 foodAllowCount = 0;
						for (ResourceEnum resourceEnum : FoodEnums) {
							if (storage.ResourceAllowed(resourceEnum)) {
								foodAllowCount++;
							}
						}
						ECheckBoxState checkState;
						if (foodAllowCount == _countof(FoodEnums)) {
							checkState = ECheckBoxState::Checked;
						}
						else if (foodAllowCount > 0) {
							checkState = ECheckBoxState::Undetermined;
						}
						else {
							checkState = ECheckBoxState::Unchecked;
						}

						auto element = manageStorageBox->AddManageStorageElement(ResourceEnum::Food, "Food", objectId, checkState, false);
						bool expanded = element->HasDelayOverride() ? element->expandedOverride : storage.expandedFood;
						for (ResourceEnum resourceEnum : FoodEnums) {
							tryAddManageStorageElement_UnderExpansion(resourceEnum, expanded);
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

						auto element = manageStorageBox->AddManageStorageElement(ResourceEnum::Luxury, "Luxury", objectId, checkState, false);
						bool expanded = element->HasDelayOverride() ? element->expandedOverride : storage.expandedLuxury;
						for (int32 i = 1; i < TierToLuxuryEnums.size(); i++) {
							for (ResourceEnum resourceEnum : TierToLuxuryEnums[i]) {
								tryAddManageStorageElement_UnderExpansion(resourceEnum, expanded);
							}
						}
					}

					// Other resources
					for (ResourceInfo resourceInfo : resourceEnums) {
						bool isAllowed = storage.ResourceAllowed(resourceInfo.resourceEnum);
						ECheckBoxState checkState = (isAllowed ? ECheckBoxState::Checked : ECheckBoxState::Unchecked);
						manageStorageBox->AddManageStorageElement(resourceInfo.resourceEnum, "", objectId, checkState, false);
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

					// Production stats
					if (IsProducer(building.buildingEnum()) ||
						building.buildingEnum() == CardEnum::Forester)
					{
						descriptionBox->AddLineSpacer();
						descriptionBox->AddRichText("Production (per season):", to_string(building.seasonalProduction()), building.product());

						PUN_DEBUG_EXPR(profitInternal += (building.seasonalProduction() * GetResourceInfo(building.product()).basePrice));
					}

					if (IsRanch(building.buildingEnum()) ||
						building.buildingEnum() == CardEnum::Farm ||
						building.buildingEnum() == CardEnum::HuntingLodge ||
						building.buildingEnum() == CardEnum::FruitGatherer)
					{
						std::vector<ResourcePair> pairs = building.seasonalProductionPairs();

						if (pairs.size() == 0) {
							descriptionBox->AddRichText("Production(per season):", "None");
						}
						else if (pairs.size() == 1) 
						{
							if (building.isEnum(CardEnum::Farm)) {
								descriptionBox->AddRichText("Production(per year):", to_string(pairs[0].count), pairs[0].resourceEnum);
							} else {
								descriptionBox->AddRichText("Production(per season):", to_string(pairs[0].count), pairs[0].resourceEnum);
							}
						}
						else {
							descriptionBox->AddRichText("Production(per season):");
							for (ResourcePair pair : pairs) {
								descriptionBox->AddIconPair(" ", pair.resourceEnum, to_string(pair.count));
							}
						}

						PUN_DEBUG_EXPR(
							if (building.product() != ResourceEnum::None) {
								profitInternal += (building.seasonalProduction() * GetResourceInfo(building.product()).basePrice);
							}
						);
					}

					// Consumption stat
					if (IsConsumerWorkplace(building.buildingEnum()))
					{
						if (hasInput1 && hasInput2)
						{
							descriptionBox->AddText("Consumption(per season): ");
							if (hasInput1) {
								descriptionBox->AddIconPair("", building.input1(), to_string(building.seasonalConsumption1()));

								PUN_DEBUG_EXPR(profitInternal -= (building.seasonalConsumption1() * GetResourceInfo(building.input1()).basePrice));
							}
							//if (hasInput1 && hasInput2) ss << ", ";
							if (hasInput2) {
								descriptionBox->AddIconPair("", building.input2(), to_string(building.seasonalConsumption2()));

								PUN_DEBUG_EXPR(profitInternal -= (building.seasonalConsumption2() * GetResourceInfo(building.input2()).basePrice));
							}
							//ss << "\n";
						}
						if (hasInput1)
						{
							descriptionBox->AddRichText("Consumption(per season):", to_string(building.seasonalConsumption1()), building.input1());
						}
					}
					// Mines consume deposits
					if (IsMountainMine(building.buildingEnum())) {
						descriptionBox->AddRichText("Depletion(per season):", to_string(building.seasonalConsumption1()), building.product());
					}

					PUN_DEBUG_EXPR(descriptionBox->AddRichText("<Yellow>PROFIT:</>", to_string(profitInternal)));
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
						[](int32 objectId, FString sItem, IGameUIDataSource* dataSource, IGameNetworkInterface* networkInterface)
					{
						auto command = make_shared<FChangeWorkMode>();
						command->buildingId = objectId;
						command->enumInt = dataSource->simulation().building(objectId).workModeIntFromString(sItem);
						networkInterface->SendNetworkCommand(command);
					});
					if (building.workMode().description != "") {
						descriptionBox->AddRichText(building.workMode().description);
					}
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
						//	descriptionBox->AddText("Input: ");
						//}
						//if (building.hasInput1()) ss << building.inputPerBatch() << " " << GetResourceInfo(building.input1()).name << " ";
						//if (building.hasInput2()) ss << ", " << building.inputPerBatch() << " " << GetResourceInfo(building.input2()).name;
						//ss << "\n";

						//ss << "Output: " << building.productPerBatch() << " " << GetResourceInfo(building.product()).name << "\n\n";

						ss << building.workPercent() << "% ";
						descriptionBox->AddRichText("Work done", ss);

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
								ss << "<Orange>Require ";
								if (building.needInput1()) ss << building.inputPerBatch() << " " << ResourceName(building.input1());
								if (building.needInput1() && building.needInput2()) ss << ", ";
								if (building.needInput2()) ss << building.inputPerBatch() << " " << ResourceName(building.input2());
								ss << "</>";
								descriptionBox->AddRichText(ss);
							}
						}

					}
					else if (IsSpecialProducer(building.buildingEnum()))
					{
#if WITH_EDITOR
						descriptionBox->AddRichText("-- workManSecPerBatch100: " + to_string(building.workManSecPerBatch100()));
						descriptionBox->AddRichText("-- workRevenuePerSec100_perMan: " + to_string(building.buildingInfo().workRevenuePerSec100_perMan));
						descriptionBox->AddRichText("-- baseInputValue: " + to_string(GetResourceInfo(building.input1()).basePrice * building.baseInputPerBatch()));
	
						switch (building.buildingEnum())
						{
						case CardEnum::BarrackArcher:
						case CardEnum::BarrackClubman:
						case CardEnum::Mint:
						case CardEnum::InventorsWorkshop:
						case CardEnum::CardMaker:
						case CardEnum::ImmigrationOffice: {
							auto& consumerIndustry = building.subclass<ConsumerIndustrialBuilding>();
							descriptionBox->AddRichText("-- baseInputValue: " + to_string(consumerIndustry.baseInputValue()));
							descriptionBox->AddRichText("-- baseOutputValue: " + to_string(consumerIndustry.baseOutputValue()));
							descriptionBox->AddRichText("-- baseProfitValue: " + to_string(consumerIndustry.baseProfitValue()));
							descriptionBox->AddRichText("-- workManSecPerBatch100(calc): " + to_string(consumerIndustry.baseProfitValue() * 100 * 100 / consumerIndustry.buildingInfo().workRevenuePerSec100_perMan));
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
						
						ss << building.workPercent() << "% ";
						descriptionBox->AddRichText("Work done", ss);
					}
				}
				else
				{		
					//resourceAndWorkSS << "constructReserved: " << (int)(100.0f * building.workReserved() / workCost) << "%\n";

					ss << static_cast<int>(100.0f * building.constructionFraction()) << "%";
					descriptionBox->AddRichText("Construct", ss);
					
#if WITH_EDITOR
					descriptionBox->AddRichText("-- buildManSecCost100", to_string(building.buildTime_ManSec100()));
#endif
				}

				// Work Reservers (Editor)
				//std::vector<int>& workReservers = building.workReservers();
				//for (int i = 0; i < workReservers.size(); i++) {
				//	ss << workReservers[i] << "\n";
				//}

				// Resources
				std::vector<ResourceHolderInfo>& holderInfos = building.holderInfos();
				if (!building.isConstructed()) {
					descriptionBox->AddLineSpacer();
					descriptionBox->AddRichText("<Subheader>Resources needed:</>");
					for (ResourceHolderInfo holderInfo : holderInfos) {
						ss << building.GetResourceCount(holderInfo) << "/" << building.GetResourceTarget(holderInfo);
						descriptionBox->AddIconPair("", holderInfo.resourceEnum, ss);
					}

#if WITH_EDITOR
					stringstream sst;
					sst << "hasNeededConstructionResource: " << (building.hasNeededConstructionResource() ? "Yes" : "No");
					descriptionBox->AddSpecialRichText("<Yellow>", sst);
					sst << "NeedConstruct: " << (building.NeedConstruct() ? "Yes" : "No");
					descriptionBox->AddSpecialRichText("<Yellow>", sst);
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
						int32_t resourceCount = building.GetResourceCount(holderInfo);

						// Don't display 0
						if (resourceCount == 0) {
							continue;
						}

						descriptionBox->AddIconPair("", holderInfo.resourceEnum, to_string(resourceCount));

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


				// Selection Meshes
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

				// Overlays
				switch (building.buildingEnum())
				{
				case CardEnum::Farm: overlayType = OverlayType::Farm; break;
				case CardEnum::FruitGatherer: overlayType = OverlayType::Gatherer; break;
				case CardEnum::Fisher: overlayType = OverlayType::Fish; break;
				case CardEnum::HuntingLodge: overlayType = OverlayType::Hunter; break;
				case CardEnum::Forester: overlayType = OverlayType::Forester; break;
				case CardEnum::Windmill: overlayType = OverlayType::Windmill; break;
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
							string moneyText = to_string(upgradeMoney);
							if (resourceSys.money() < upgradeMoney) {
								moneyText = "<Red>" + moneyText + "</>";
							}

							bool showExclamation = simulation.parameters(playerId())->NeedTownhallUpgradeNoticed;

							ss << "Upgrade townhall to lvl " << (townhall.townhallLvl + 1) << "\n";
							ss << "<img id=\"Coin\"/>" << moneyText;
							
							UPunButton* button = descriptionBox->AddButton2Lines(ss.str(), this, CallbackEnum::UpgradeBuilding, true, showExclamation, objectId, 0);
							ss.str(string());

							ss << "Upgrade Rewards<line>";
							ss << GetTownhallLvlToUpgradeBonusText(townhall.townhallLvl + 1);
							
							if (resourceSys.money() < upgradeMoney) {
								ss << "<space>";
								ss << "<Red>Not enough money to upgrade.</>";
							}
							AddToolTip(button, ss);
						}

						//if (townhall.wallLvl < TownHall::GetMaxUpgradeLvl())
						//{
						//	int32 upgradeStone = townhall.GetUpgradeStones();
						//	string resourceText = to_string(upgradeStone);
						//	if (resourceSys.resourceCount(ResourceEnum::Stone) < upgradeStone) {
						//		resourceText = "<Red>" + resourceText + "</>";
						//	}

						//	ss << "Upgrade wall to lvl " << (townhall.wallLvl + 1) << "\n";
						//	ss << resourceText << "<img id=\"Stone\"/>";

						//	bool showEnabled = townhall.wallLvl < townhall.townhallLvl;

						//	UPunButton* button = descriptionBox->AddButton2Lines(ss.str(), this, CallbackEnum::UpgradeBuilding, showEnabled, false, objectId, 1);
						//	ss.str(string());

						//	ss << "Upgrading the wall to next level will double its HP.";

						//	if (!showEnabled) {
						//		ss << "<space>";
						//		ss << "<Red>Wall level cannot exceed townhall level.</>";
						//	}
						//	else if (resourceSys.resourceCount(ResourceEnum::Stone) < upgradeStone) {
						//		ss << "<space>";
						//		ss << "<Red>Not enough stone to upgrade.</>";
						//	}
						//	AddToolTip(button, ss);
						//}
					}
					//! Upgrade Button Others
					else
					{
						auto setUpgradeButton = [&](BuildingUpgrade upgrade, int32 upgradeIndex)
						{
							ss << "Upgrade " << upgrade.name;
							
							if (upgrade.isUpgraded) {
								ss << "\nDone";
							}
							else {
								ResourceEnum resourceEnum = upgrade.resourceNeeded.resourceEnum;
								
								auto showResourceText = [&](std::string resourceString) {
									ss << "\n"
										<< "<img id=\"" << resourceString << "\"/>"
										<< TextRed(to_string(upgrade.resourceNeeded.count),
											resourceSys.resourceCount(resourceEnum) < upgrade.resourceNeeded.count);
								};
								
								if (resourceEnum == ResourceEnum::Stone) { showResourceText("Stone"); }
								else if (resourceEnum == ResourceEnum::Wood) { showResourceText("Wood"); }
								else if (resourceEnum == ResourceEnum::Iron) { showResourceText("IronBar"); }
								else if (resourceEnum == ResourceEnum::SteelTools) { showResourceText("SteelTools"); }
								else if (resourceEnum == ResourceEnum::Brick) { showResourceText("Brick"); }
								else if (resourceEnum == ResourceEnum::Paper) { showResourceText("Paper"); }
								//else if (upgrade.resourceNeeded.isValid()) {
								//	ss << " (" << upgrade.resourceNeeded.ToString() << ")";
								//}
								else {
									PUN_CHECK(resourceEnum == ResourceEnum::None);
									
									string moneyText = to_string(upgrade.moneyNeeded);
									if (resourceSys.money() < upgrade.moneyNeeded) {
										moneyText = "<Red>" + moneyText + "</>";
									}
									
									ss << "\n" << "<img id=\"Coin\"/>" << moneyText;
								}
							}

							bool isUpgradable = !upgrade.isUpgraded;

							UPunButton* button = descriptionBox->AddButton2Lines(ss.str(), this, CallbackEnum::UpgradeBuilding, isUpgradable, false,  objectId, upgradeIndex);
							ss.str("");
							ss << upgrade.name << "<space>" << upgrade.description;

							if (!_alreadyDidShiftDownUpgrade) {
								ss << "<line><space><Orange>Shift-click</> the button to try upgrading all same type buildings.";
							}
							
							AddToolTip(button, ss);
						};

						const std::vector<BuildingUpgrade>& upgrades = building.upgrades();
						for (size_t i = 0; i < upgrades.size(); i++) {
							setUpgradeButton(upgrades[i], i);
						}
					}
				}


				//! Drop Down
				if (building.playerId() == playerId()) {
					_objectDescriptionUI->SetDropDown(objectId);
				}
				
			} // Fire

			// Selection Mesh
			TileArea area = building.area();
			FVector displayLocation = dataSource()->DisplayLocation(building.centerTile().worldAtom2());
			AlgorithmUtils::ShiftDisplayLocationToTrueCenter(displayLocation, area, building.faceDirection());

			FVector selectionMeshLocation = displayLocation + FVector(0, 0, 30);
			if (building.isEnum(CardEnum::Bridge)) {
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
			check((int)unit.unitState() < sizeof(UnitStateString) / sizeof(UnitStateString[0]));

			stringstream ss;

			if (unit.isEnum(UnitEnum::Human)) {
				ss << "<Header>" << unit.GetUnitName() << "</>";
				SetText(_objectDescriptionUI->DescriptionUITitle, ss);

				ss << "<Subheader>" << unit.typeName() << "</>";
			} else {
				ss << "<Header>" << unit.typeName() << "</>";
				SetText(_objectDescriptionUI->DescriptionUITitle, ss);
			}
			
#if WITH_EDITOR
			ss << "[" << objectId << "]"; // ID
#endif
			descriptionBox->AddRichText(ss);
			descriptionBox->AddLineSpacer(8);

			if (!unit.isEnum(UnitEnum::Human)) {
				ss << "Gender: " << (unit.isMale() ? "male" : "female");
				descriptionBox->AddRichText(ss);
			}

			if (unit.playerId() != -1) {
				ss << "Player: " << simulation.playerName(unit.playerId());
				descriptionBox->AddRichText(ss);
			}
			
#if WITH_EDITOR
			ss << "-- Tile:(" << unit.unitTile().x << "," << unit.unitTile().y << ")";
			descriptionBox->AddRichText(ss);
			ss << "-- lastBirthTick:(" << unit.lastPregnantTick() << ")";
			descriptionBox->AddRichText(ss);
			ss << "animation: " << GetUnitAnimationName(unit.animationEnum());
			descriptionBox->AddRichText(ss);
#endif

			ss << "Age: " << unit.age() * PlayerParameters::PeopleAgeToGameYear / Time::TicksPerYear + 3 << " years"; // cheat 3 years older so no 0 years old ppl
			descriptionBox->AddTextWithSpacer(ss);

			// Show Food/Heat as percent
			//ss << "<space>Food: " << unit.foodActual() / 60 << "/" << unit.maxFood() / 60 << " secs";
			//ss << "<space>Heat: " << unit.heatActual() / 60 << "/" << unit.maxHeat() / 60 << " C*secs";
			ss << "<space>Food: " << (unit.foodActual() * 100 / unit.maxFood()) << "/100";
			ss << "<space>Heat: " << (unit.heatActual() * 100 / unit.maxHeat()) << "/100";
			ss << "<space>Health: " << unit.hp() << "/ 100";

			if (unit.isEnum(UnitEnum::Human)) {
				ss << "<space>Fun: " << unit.subclass<HumanStateAI>().funPercent() << " %";
#if WITH_EDITOR
				ss << "<space> -- Fun Sec: " << (unit.subclass<HumanStateAI>().funTicksActual() / Time::TicksPerSecond) << "s";
#endif
			}

			descriptionBox->AddRichTextParsed(ss);

			// Dying
			auto dyingMessage = [&](std::string dyingDescription) {
				ss << "<space><Red>" << dyingDescription << "</>";
				descriptionBox->AddRichTextParsed(ss);
				descriptionBox->AddSpacer();
			};
			if (unit.foodActual() <= 0) {
				dyingMessage("Dying from starvation");
			}
			else if (unit.heatActual() <= 0) {
				dyingMessage("Dying from the freezing cold");
			}
			else if (unit.hp() <= 0) {
				dyingMessage("Dying from sickness");
			}
			else if (unit.foodActual() <= unit.minWarnFood()) {
				dyingMessage("Starving");
			}
			else if (unit.heatActual() <= unit.minWarnHeat()) {
				dyingMessage("Freezing");
			}
			else if (unit.isSick()) {
				dyingMessage("Sick");
			}
			// Tools
			else if (unit.isEnum(UnitEnum::Human)) 
			{
				auto& human = unit.subclass<HumanStateAI>();
				if (human.needTools()) {
					dyingMessage("Cannot work properly without tools");
				}
#if WITH_EDITOR
				descriptionBox->AddRichText("-- Next tools sec", to_string((human.nextToolTick() - Time::Ticks()) / Time::TicksPerSecond));
#endif
			}

			if (unit.isEnum(UnitEnum::Human)) 
			{
				auto& human = unit.subclass<HumanStateAI>();

				{
					ss << human.workEfficiency100() << "%";
					auto widget = descriptionBox->AddRichText("Work efficiency", ss);

					ss << "Work efficiency: " << human.workEfficiency100() << "%\n";
					ss << " 100% base\n";
					ss << " penalties:\n";
					if (human.foodWorkPenalty100() < 0) {
						ss << "  " << human.foodWorkPenalty100() << "% starvation\n";
					}
					if (human.heatWorkPenalty100() < 0) {
						ss << "  " << human.heatWorkPenalty100() << "% cold\n";
					}
					if (human.toolPenalty100() < 0) {
						ss << "  " << human.toolPenalty100() << "% no tools\n";
					}
					if (human.sicknessPenalty100() < 0) {
						ss << "  " << human.sicknessPenalty100() << "% sick\n";
					}
					if (human.happinessPenalty100() < 0) {
						ss << "  " << human.happinessPenalty100() << "% happiness\n";
					}

					ss << " adjustments:\n";
					if (human.speedBoostEfficiency100() != 0) {
						ss << "  " << human.speedBoostEfficiency100() << "% leader speed boost\n";
					}
					
					AddToolTip(widget, ss);
				}
				
				descriptionBox->AddSpacer();

				{
					ss << human.happiness() << "<img id=\"Smile\"/>";
					auto widget = descriptionBox->AddRichText("Happiness", ss);

					ss << "Happiness: " << human.happiness() << "<img id=\"Smile\"/>\n";
					ss << "  Needs: " << human.baseHappiness() << "\n";
					ss << "   " << human.foodHappiness() << " food\n";
					ss << "   " << human.heatHappiness() << " heating\n";
					ss << "   " << human.housingHappiness() << " housing\n";
					ss << "   " << human.funHappiness() << " fun\n";
					ss << "  Modifiers: " << human.modifiersHappiness() << "\n";

					for (size_t i = 0; i < HappinessModifierEnumCount; i++) {
						int32 modifier = human.GetHappinessModifier(static_cast<HappinessModifierEnum>(i));
						if (modifier != 0) ss << "   " << modifier << " " << HappinessModifierName[i] << "\n";
					}
					
					AddToolTip(widget, ss);
				}
			}

			ss << "state: " << UnitStateString[(int)unit.unitState()] << "\n";

			if (unit.workplaceId() != -1) {
				auto workplaceName = simulation.building(unit.workplaceId()).buildingInfo().name;
				ss << "workplace: " << workplaceName << " (id: " << unit.workplaceId() << ")\n";
			} else {
				ss << "workplace: none \n";
			}

			if (unit.houseId() != -1) {
				auto houseName = simulation.building(unit.houseId()).buildingInfo().name;
				ss << "house: " << houseName << " (id: " << unit.houseId() << ")\n";
			} else {
				ss << "house: none \n";
			}

			//! Debug
#if WITH_EDITOR 
			if (PunSettings::IsOn("UIActions")) {
				ss << "-- nextActiveTick:" << unit.nextActiveTick() << "\n";
				ss << "speech:\n" << unit.debugSpeech(true);
				auto punText = descriptionBox->AddText(ss);
				
				FSlateFontInfo fontInfo = FSlateFontInfo(FPaths::EngineContentDir() / TEXT("Slate/Fonts/Roboto-Bold.ttf"), 7);
				punText->PunText->SetFont(fontInfo);
				//punText->PunText->SetRenderScale(FVector2D(1.0f, 0.5));
			}
#endif

			//if (unit.isEnum(UnitEnum::Infantry)) {
			//	ss << "target:" << unit.subclass<ArmyStateAI>().targetBuildingId() << "\n";
			//	ss << "garrison:" << unit.subclass<ArmyStateAI>().garrisonBuildingId() << "\n";
			//}

			descriptionBox->AddText(ss);

			// Inventory
			descriptionBox->AddLineSpacer();
			descriptionBox->AddRichText("Inventory:");
			unit.inventory().ForEachResource([&](ResourcePair resource) {
				descriptionBox->AddIconPair("", resource.resourceEnum, to_string(resource.count));
			});

			// Selection Meshes
			if (unit.houseId() != -1) {
				WorldAtom2 atom = simulation.buildingSystem().actualAtomLocation(unit.houseId());
				SpawnSelectionMesh(assetLoader->SelectionMaterialYellow, dataSource()->DisplayLocation(atom) + FVector(0, 0, 30));
			}
			if (unit.workplaceId() != -1) {
				WorldAtom2 atom = simulation.buildingSystem().actualAtomLocation(unit.workplaceId());
				SpawnSelectionMesh(assetLoader->SelectionMaterialYellow, dataSource()->DisplayLocation(atom) + FVector(0, 0, 30));

				if (GameRand::DisplayRand(Time::Ticks()) == 123456789) {
					SpawnSelectionMesh(assetLoader->ReferenceTerrainMaterial, dataSource()->DisplayLocation(atom) + FVector(0, 0, -30));
				}
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

			ss << WrapString(info.description);
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
				ss << " " << unit.unitInfo().name << " (id: " << objectId << ")";
				
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
			if (simulation.tileOwner(tile) != playerId()) {
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

				descriptionBox->AddText(ss);

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
			AddProvinceInfo(objectId, descriptionBox);

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
		
		if (!SimUtils::CanReserveSpot_NotTooCloseToAnother(provinceId, &simulation(), 1)) {
			descriptionBox->AddRichText("<Red>Too close to another town</>");
			canClaim = false;
		}

		TileArea area = SimUtils::FindStartSpot(provinceId, &simulation());
		
		if (!area.isValid()) {
			descriptionBox->AddRichText("<Red>Not enough buildable space.</>");
			canClaim = false;
		}


		int32 provincePrice = simulation().GetProvinceClaimPrice(provinceId, playerId());

		stringstream ss;
		ss << "Select Starting Location\n";
		if (area.isValid()) {
			ss << "<img id=\"Coin\"/>" << TextRed(to_string(provincePrice), !canClaim);
		} else {
			ss << "<Red>Not enough buildable space.</>";
		}
		descriptionBox->AddButton2Lines(ss.str(), this, CallbackEnum::SelectStartingLocation, canClaim, false, provinceId);
	}
}

void UObjectDescriptionUISystem::AddClaimLandButtons(int32 provinceId, UPunBoxWidget* descriptionBox)
{
	if (provinceId == -1) {
		return;
	}
	
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

				stringstream ss;
				ss << "Claim Province";
				AppendClaimConnectionString(ss, claimConnectionEnum);
				ss << "\n<img id=\"Influence\"/>" << TextRed(to_string(provincePrice), !canClaim);

				descriptionBox->AddSpacer();
				descriptionBox->AddButton2Lines(ss.str(), this, CallbackEnum::ClaimLandInfluence, canClaim, false, provinceId);
			}

			// Claim by money
			{
				bool canClaim = simulation().money(playerId()) >= provincePrice;

				stringstream ss;
				ss << "Claim Province";
				AppendClaimConnectionString(ss, claimConnectionEnum);
				ss << "\n<img id=\"Coin\"/>" << TextRed(to_string(provincePrice), !canClaim);

				descriptionBox->AddSpacer();
				descriptionBox->AddButton2Lines(ss.str(), this, CallbackEnum::ClaimLandMoney, canClaim, false, provinceId);
			}

			// Claim by food
			//if (simulation().IsResearched(playerId(), TechEnum::ClaimLandByFood))
			{
				int32 foodNeeded = provincePrice / FoodCost;
				bool canClaim = sim.foodCount(playerId()) >= foodNeeded;

				stringstream ss;
				ss << "Claim Province";
				AppendClaimConnectionString(ss, claimConnectionEnum);
				ss << "\n" << TextRed(to_string(foodNeeded), !canClaim) << " food";

				descriptionBox->AddSpacer();
				descriptionBox->AddButton2Lines(ss.str(), this, CallbackEnum::ClaimLandFood, canClaim, false, provinceId);
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

						stringstream ss;
						ss << "Conquer Province (Annex)";
						AppendClaimConnectionString(ss, claimConnectionEnum);
						ss << "\n" << TextRed(to_string(startAttackPrice), !canClaim) << "<img id=\"Influence\"/>";

						descriptionBox->AddSpacer();
						descriptionBox->AddButton2Lines(ss.str(), this, CallbackEnum::StartAttackProvince, canClaim, false, provinceId);
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

		// Delayed UI
		tradingCompany.lastTargetSetTick = Time::Ticks();
		tradingCompany.lastTargetAmountSet = numberBox->amount;
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
	
	std::stringstream ssTemp;
	int32 rainFall100 = terrainGenerator.GetRainfall100(tile);
	ssTemp << "Rainfall100" << rainFall100;
	descriptionBox->AddSpecialRichText("<Yellow>", ssTemp);
	ssTemp << "Temp10000: " << terrainGenerator.GetTemperatureFraction10000(tile.x, rainFall100);
	descriptionBox->AddSpecialRichText("-- ", ssTemp);
#endif
}

void UObjectDescriptionUISystem::AddTileInfo(WorldTile2 tile, UPunBoxWidget* descriptionBox)
{
	WorldRegion2 region = tile.region();
	auto& sim = simulation();

	stringstream ss;
	ss << "<Header>" << sim.terrainGenerator().GetBiomeName(tile) << " Tile</>\n";
	ss << "<Header>(" << tile.x << ", " << tile.y << ")</>";
	SetText(_objectDescriptionUI->DescriptionUITitle, ss);
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
	ss << "Owner: " << (ownerId == -1 ? "None" : sim.playerName(ownerId));
	descriptionBox->AddRichText(ss);


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
	ss << "Display (" << displayLocation.X << "," << displayLocation.Y << ")";
	descriptionBox->AddSpecialRichText("<Yellow>", ss);

	int32 provinceOwner = sim.provinceOwner(provinceId);
	if (provinceOwner != -1) {
		AIPlayerSystem& aiPlayer = sim.aiPlayerSystem(provinceOwner);

		if (aiPlayer.active()) {
			AIRegionStatus* regionStatus = aiPlayer.regionStatus(provinceId);
			if (regionStatus) {
				ss << "AIRegion (" << AIRegionPurposeName[static_cast<int>(regionStatus->currentPurpose)];
				ss << ", proposed:" << AIRegionProposedPurposeName[static_cast<int>(regionStatus->proposedPurpose)] << ")";
				descriptionBox->AddSpecialRichText("<Yellow>", ss);
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
	
	auto terrainGenerator = simulation().terrainGenerator();

	stringstream ss;
	
	if (provinceId == OceanProvinceId)
	{
		ss << "<Header>Deep Ocean</>\n";
		SetText(_objectDescriptionUI->DescriptionUITitle, ss);
		ss << "Large body of water.";
		descriptionBox->AddRichText(ss);
		return;
	}

	if (provinceId == MountainProvinceId)
	{
		ss << "<Header>High Mountain</>\n";
		SetText(_objectDescriptionUI->DescriptionUITitle, ss);
		ss << "Impassable unclaimed mountain.";
		descriptionBox->AddRichText(ss);
		return;
	}

	if (provinceId == RiverProvinceId ||
		provinceId == EmptyProvinceId)
	{
		ss << "<Header>Unusable Land</>\n";
		SetText(_objectDescriptionUI->DescriptionUITitle, ss);
		ss << "Land without any use that no one bothers with.";
		descriptionBox->AddRichText(ss);
		return;
	}

	provinceId = abs(provinceId);

	WorldTile2 provinceCenter = simulation().provinceSystem().GetProvinceCenterTile(provinceId);
	
	ss << "<Header>" << terrainGenerator.GetBiomeName(provinceCenter) << " Province</>\n";
	ss << "<Subheader>" <<  WorldRegion2(provinceId).ToString() << "</>\n";
	SetText(_objectDescriptionUI->DescriptionUITitle, ss);
	ss.str("");
	descriptionBox->AddSpacer(12);

	// Biome Description
	ss << WrapString(terrainGenerator.GetBiomeInfoFromTile(provinceCenter).description);
	descriptionBox->AddRichText(ss);

	descriptionBox->AddSpacer(12);

	// Biome number info
	AddBiomeInfo(provinceCenter, descriptionBox);
	
	descriptionBox->AddSpacer();

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
				descriptionBox->AddIconPair("", node.info().resourceEnum, to_string(node.depositAmount));
			}
			if (isMountain) {
				descriptionBox->AddIconPair("", ResourceEnum::Stone, to_string(node.stoneAmount));
			}
			
			descriptionBox->AddSpacer();
		}

	}
	else if (isMountain)
	{
		if (showTopLine) {
			descriptionBox->AddLineSpacer(15);
		}
		
		descriptionBox->AddRichText("Stone deposit:");
		descriptionBox->AddIconPair("", ResourceEnum::Stone, to_string(node.stoneAmount));
	}
}

void UObjectDescriptionUISystem::AddEfficiencyText(Building& building, UPunBoxWidget* descriptionBox)
{
	stringstream ss;

	if (building.isEnum(CardEnum::CardMaker))
	{
		ss << "Scholars Office's Efficiency increases work speed.";
		ss << "<space>";
	}
	
	ss << building.efficiency() << "%";
	auto widget = descriptionBox->AddRichText("Efficiency", ss);

	ss << "<Bold>Efficiency: " << building.efficiency() << "%</>";
	ss << "<space>";
	ss << " Base: " << building.efficiencyBeforeBonus() << "%";
	ss << "<space>";
	ss << " Bonuses:";

	{
		stringstream ss2;
		if (building.adjacentEfficiency() > 0) ss2 << "\n  +" << building.adjacentEfficiency() << "% adjacency bonus";
		if (building.levelEfficiency() > 0) ss2 << "\n  +" << building.levelEfficiency() << "% level";

		auto bonuses = building.GetBonuses();
		for (BonusPair bonus : bonuses) {
			if (bonus.value > 0) ss2 << "\n  +" << bonus.value << "% " << bonus.name;
		}

		if (ss2.str() == "") {
			ss2 << " None";
		}

		ss << ss2.str();
	}
	
	AddToolTip(widget, ss);
}

void UObjectDescriptionUISystem::AddTradeFeeText(TradeBuilding& building, UPunBoxWidget* descriptionBox)
{
	auto feeText = descriptionBox->AddRichText("Trade fee:", to_string(building.tradingFeePercent()) + "%");
	std::stringstream feeTip;
	feeTip << "<Bold>Trading fee:</>\n";
	feeTip << "base " << building.baseTradingFeePercent() << "%\n";
	std::vector<BonusPair> bonuses = building.GetTradingFeeBonuses();
	for (const auto& bonus : bonuses) {
		feeTip << bonus.value << "% " << bonus.name << "\n";
	}
	AddToolTip(feeText, feeTip);
}