#include "BuildingPlacementSystem.h"
#include "PunCity/MapUtil.h"

#include "Materials/MaterialInstanceDynamic.h"
#include "CollisionQueryParams.h"
#include "PunCity/PunUtils.h"
#include "Components/DecalComponent.h"
#include "PunCity/Simulation/Buildings/GathererHut.h"
#include "PunCity/Simulation/Buildings/Temples.h"
#include "PunCity/Simulation/GameSimulationCore.h"
#include "PunCity/DisplaySystem/GameDisplayInfo.h"

#include <memory>
#include <algorithm>

DECLARE_CYCLE_STAT(TEXT("PUN: DragSpawn"), STAT_DragSpawnPlacement, STATGROUP_Game);
DECLARE_CYCLE_STAT(TEXT("PUN: DragAfterAdd"), STAT_DragAfterAdd, STATGROUP_Game);

using namespace std;
using namespace std::placeholders;

void PlacementGrid::Init(UMaterialInterface* material, UStaticMesh* mesh, UInstancedStaticMeshComponent* instancedMesh)
{
	_placementMesh = instancedMesh;
	_dynamicMaterial = UMaterialInstanceDynamic::Create(material, _placementMesh);
	_placementMesh->SetStaticMesh(mesh);
	_placementMesh->SetMaterial(0, _dynamicMaterial);
}

void PlacementGrid::SpawnGrid(WorldAtom2 cameraAtom, WorldTile2 location, Direction direction)
{
	PUN_CHECK(displayCount < 1000000);

	FVector displayLocation = MapUtil::DisplayLocation(cameraAtom, location.worldAtom2());
	FTransform transform(FRotator(0, RotationFromDirection(direction), 0), displayLocation, FVector(1.0f, 1.0f, 1.0f) * 5.0);

	if (displayCount >= _placementMesh->GetInstanceCount()) {
		_placementMesh->AddInstance(transform);
	} else {
		_placementMesh->UpdateInstanceTransform(displayCount, transform);
	}
	displayCount++;
}

void PlacementGrid::AfterAdd()
{
	SCOPE_CYCLE_COUNTER(STAT_DragAfterAdd);

	int instanceCount = _placementMesh->GetInstanceCount();
	for (int i = instanceCount - 1; i >= displayCount; i--) {
		_placementMesh->UpdateInstanceTransform(i, FTransform(FRotator::ZeroRotator, FVector::ZeroVector, FVector::ZeroVector));
	}

	// Refresh
	FTransform firstInstTransform;
	_placementMesh->GetInstanceTransform(0, firstInstTransform);
	_placementMesh->UpdateInstanceTransform(0, firstInstTransform, false, true, false);
}

UInstancedStaticMeshComponent * ABuildingPlacementSystem::CreateInstancedStaticMeshComponent(FString name)
{
	UInstancedStaticMeshComponent* newObject = NewObject<UInstancedStaticMeshComponent>(this, UInstancedStaticMeshComponent::StaticClass());
	newObject->Rename(*name);
	newObject->AttachToComponent(_root, FAttachmentTransformRules::KeepRelativeTransform, NAME_None);

	newObject->BodyInstance.SetCollisionEnabled(ECollisionEnabled::NoCollision, false);
	newObject->SetGenerateOverlapEvents(false);
	newObject->SetWorldLocation(FVector::ZeroVector);
	newObject->RegisterComponent();

	newObject->SetActive(false);
	return newObject;
}

/*
 * BuildingPlacementSystem
 */

ABuildingPlacementSystem::ABuildingPlacementSystem()
{
	PrimaryActorTick.bCanEverTick = false;

	_root = CreateDefaultSubobject<USceneComponent>("Root");
	SetActorLocationAndRotation(FVector::ZeroVector, FRotator(0.0f, 0.0f, 0.0f));

	_gridGuide = CreateDefaultSubobject<UDecalComponent>("GridGuide");
	_gridGuide->SetRelativeRotation(FVector(0, 0, -1).Rotation()); // Rotate to project the decal down
	_gridGuide->DecalSize = FVector(100, 160, 160);
	_gridGuide->SetActive(false);
	_gridGuide->SetVisibility(false);

	_radiusDecal = CreateDefaultSubobject<UDecalComponent>("RadiusDecal");
	_radiusDecal->SetRelativeRotation(FVector(0, 0, -1).Rotation()); // Rotate to project the decal down
	_radiusDecal->DecalSize = FVector(100, 160, 160);
	_radiusDecal->SetActive(false);
	_radiusDecal->SetVisibility(false);

	//_buildingMeshes = CreateDefaultSubobject<UBuildingMeshesComponent>("BuildingMeshes");
	//_delayFillerMeshes = CreateDefaultSubobject<UBuildingMeshesComponent>("DelayFillerMeshes");
}

void ABuildingPlacementSystem::Init(UAssetLoaderComponent* assetLoader)
{
	_assetLoader = assetLoader;

	auto initPlacementGrid = [&] (PlacementGrids& placementGrid, FString append)
	{
		auto instancedMesh = CreateInstancedStaticMeshComponent(FString("PlacementGreen") + append);
		placementGrid.Init(_assetLoader->PlacementMaterialGreen, _assetLoader->PlacementMesh, instancedMesh);

		//instancedMesh = CreateInstancedStaticMeshComponent(FString("PlacementRed") + append);
		//placementGrid.Init(_assetLoader->PlacementMaterialRed, _assetLoader->PlacementMesh, instancedMesh);

		instancedMesh = CreateInstancedStaticMeshComponent(FString("PlacementOrange") + append);
		placementGrid.Init(_assetLoader->PlacementMaterialRed, _assetLoader->PlacementMesh, instancedMesh);

		instancedMesh = CreateInstancedStaticMeshComponent(FString("PlacementGray") + append);
		placementGrid.Init(_assetLoader->PlacementMaterialGray, _assetLoader->PlacementMesh, instancedMesh);

		instancedMesh = CreateInstancedStaticMeshComponent(FString("PlacementArrowGreen") + append);
		placementGrid.Init(_assetLoader->PlacementArrowMaterialGreen, _assetLoader->PlacementMesh, instancedMesh);

		instancedMesh = CreateInstancedStaticMeshComponent(FString("PlacementArrowYellow") + append);
		placementGrid.Init(_assetLoader->PlacementArrowMaterialYellow, _assetLoader->PlacementMesh, instancedMesh);

		instancedMesh = CreateInstancedStaticMeshComponent(FString("PlacementArrowRed") + append);
		placementGrid.Init(_assetLoader->PlacementArrowMaterialRed, _assetLoader->PlacementMesh, instancedMesh);
	};

	initPlacementGrid(_placementGrid, FString(""));
	initPlacementGrid(_placementGridDelayed, FString("Delayed"));

	_placementType = PlacementType::None;
	_canPlace = false;
	_forceCannotPlace = false;

	_dragGatherSuccessCount = 0;
	_dragRoadSuccessCount = 0;
	_dragDemolishSuccessCount = 0;
	
	_dragState = DragState::None;

	_timesRotated = 0;
	_timesShifted = 0;

	_placementInstructions.resize(static_cast<int>(PlacementInstructionEnum::Count), { false, -1, -1 });

	_faceDirection = Direction::S;
}

PlacementInfo ABuildingPlacementSystem::GetPlacementInfo()
{
	if (!_gameInterface) return PlacementInfo();
	
	auto& sim = _gameInterface->simulation();
	int32 playerId = _gameInterface->playerId();
	
	/*
	 * Instructions
	 */
	if (_buildingEnum == CardEnum::StorageYard)
	{
		ClearInstructions();

		stringstream ss;
		if (_placementType != PlacementType::BuildingDrag) {
			ss << "Click and Drag Cursor";
			ss << "\nto specify area";
		}
		else
		{
			int32 sizeX = _area.sizeX();
			int32 sizeY = _area.sizeY();

			ss << "Drag cursor to resize the storage";

			if (sizeX > 1 || sizeY > 1) {
				int32 storageSpace = ((sizeX / 2) * (sizeY / 2));
				ss << "\n" << (sizeY / 2 * 2) << "x" << (sizeX / 2 * 2) << " (" << storageSpace << "<img id=\"Storage\"/>)";

				int32 woodNeeded = storageSpace * 5;
				ss << "\n" << MaybeRedText(to_string(woodNeeded), sim.resourceCountWithDrops(playerId, ResourceEnum::Wood) < woodNeeded) << "<img id=\"Wood\"/>";
			}

			if (IsStorageTooLarge(TileArea(WorldTile2(0, 0), WorldTile2(sizeX, sizeY)))) {
				ss << "\n" << "<Red>Width and Height must be less than 8</>";
			}
		}
		SetInstruction(PlacementInstructionEnum::DragStorageYard, true, ss.str());
	}
	else if (_buildingEnum == CardEnum::Farm)
	{
		PUN_CHECK(_placementType == PlacementType::BuildingDrag);
		ClearInstructions();

		stringstream ss;
		int32 fertility = _dragState == DragState::Dragging ? Farm::GetAverageFertility(_area, &sim) : sim.GetFertilityPercent(_mouseOnTile);
		ss << "Fertility: " << fertility << "%<space>";
		
		if (_dragState == DragState::NeedDragStart)
		{
			ss << "Click and Drag Cursor";
			ss << "\nto specify area";
		}
		else {
			int32 sizeX = _area.sizeX();
			int32 sizeY = _area.sizeY();

			ss << "Drag cursor to resize the farm";

			if (sizeX > 1 || sizeY > 1) {
				int32 areaSize = (sizeY * sizeX);
				ss << "\n" << sizeY << "x" << sizeX << " (Area:" << areaSize << ")";

				int32 woodNeeded = areaSize / 2;
				ss << "\n" << MaybeRedText(to_string(woodNeeded), sim.resourceCountWithDrops(playerId, ResourceEnum::Wood) < woodNeeded) << "<img id=\"Wood\"/>";
			}

			TileArea area(WorldTile2(0, 0), WorldTile2(sizeX, sizeY));
			if (IsFarmWidthTooHigh(area)) {
				ss << "\n<Red>Width or Length too large</>\n<Red>(Max Width/Length: 16)</>";
			}
			else if (IsFarmTooLarge(area)) {
				ss << "\n<Red>Area is too large (Max Area: 64)</>";
			}
			else if (IsFarmTooSmall(area)) {
				ss << "\n<Red>Area is too small (Min Area: 16)</>";
			}
		}
		SetInstruction(PlacementInstructionEnum::DragFarm, true, ss.str());
	}
	else if (_placementType == PlacementType::StoneRoad)
	{
		ClearInstructions();

		if (_dragState == DragState::Dragging && _canPlace) 
		{
			int32 stoneNeeded = 0;
			for (int32 tileId : _roadPathTileIds) {
				WorldTile2 tile(tileId);
				if (sim.buildingEnumAtTile(tile) == CardEnum::None && 
					!sim.IsRoadTile(tile)) 
				{
					stoneNeeded += 2;
				}
			}
			//int32 stoneNeeded = _roadPathTileIds.Num() * 2;
			SetInstruction(PlacementInstructionEnum::DragRoadStone, true, stoneNeeded);
		}
	}
	else if (_placementType == PlacementType::IntercityRoad)
	{
		ClearInstructions();

		if (_dragState == DragState::Dragging && _canPlace) {
			int32 goldNeeded = 0;
			for (int32 roadTileId : _roadPathTileIds) {
				if (!_gameInterface->simulation().IsRoadTile(WorldTile2(roadTileId))) {
					goldNeeded += IntercityRoadTileCost;
				}
			}
			
			SetInstruction(PlacementInstructionEnum::DragRoadIntercity, true, goldNeeded);
		}
	}
	else if (_buildingEnum == CardEnum::Kidnap)
	{
		if (_canPlace)
		{
			ClearInstructions();
			
			stringstream ss;
			//ss << "Spend " << 5 * sim.population(playerId) << "<img id=\"Coin\"/> to kidnap 3 people";
			ss << "Kidnap 3 people\n" << "Use on opponent's Townhall.";
			SetInstruction(PlacementInstructionEnum::Kidnap, true, ss.str());
		}
	}

	// Gather/Demolish Drag states
	else if (_dragState == DragState::NeedDragStart)
	{
		ClearInstructions();
		
		SetInstruction(PlacementInstructionEnum::DragGather, IsGatherPlacement(_placementType) && _dragGatherSuccessCount < 3);

		if (IsRoadPlacement(_placementType) && _dragRoadSuccessCount < 3) {
			SetInstruction(PlacementInstructionEnum::DragRoad1, true);
		}
		
		SetInstruction(PlacementInstructionEnum::DragDemolish, _placementType == PlacementType::Demolish && _dragDemolishSuccessCount < 3);
	}
	else if (_dragState == DragState::Dragging)
	{
		ClearInstructions();
		
		//if (_placementType == PlacementType::StoneRoad && _canPlace)
		//{
		//	int32 stoneNeeded = _roadPathTileIds.Num() * 2;
		//	SetInstruction(PlacementInstructionEnum::DragRoadStone, true, stoneNeeded);
		//}
	}
	else if (_buildingEnum == CardEnum::Colony) {
		SetInstruction(PlacementInstructionEnum::Colony, true);
	}
	else if (_buildingEnum == CardEnum::Fort) {
		SetInstruction(PlacementInstructionEnum::Fort, true);
	}
	// Buildings
	else
	{
		// For typical buildings, we don't clear instructions
		bool shouldShowRotate = _buildingEnum != CardEnum::Farm && 
								_buildingEnum != CardEnum::StorageYard;
		
		SetInstruction(PlacementInstructionEnum::Rotate, shouldShowRotate && _timesRotated < 3);

		bool isShiftableBuilding = _gameInterface && _gameInterface->simulation().IsPermanentBuilding(_networkInterface->playerId(), _buildingEnum);
		SetInstruction(PlacementInstructionEnum::Shift, isShiftableBuilding && _timesShifted < 2);
	}



	
	/*
	 * Display location
	 */
	FVector displayLocation;
	if (_dragState == DragState::NeedDragStart) {
		// Drag's center is the mouse tile
		displayLocation = MapUtil::DisplayLocation(_networkInterface->cameraAtom(), _mouseOnTile.worldAtom2());
	} else {
		displayLocation = _buildingLocation;
	}
	
	return PlacementInfo(_placementType, _buildingEnum, _placementInstructions, _mouseOnTile, displayLocation);
}

void ABuildingPlacementSystem::ShowRadius(int radius, OverlayType overlayType, bool isRed)
{
	_gameInterface->SetOverlayType(overlayType, OverlaySetterType::BuildingPlacement);
	
	//if (!_assetLoader->RadiusMaterialInstance) {
	//	_assetLoader->RadiusMaterialInstance = UMaterialInstanceDynamic::Create(_assetLoader->RadiusMaterial, this);
	//}
	//_assetLoader->MI_RedRadius->SetScalarParameterValue("IsRed", isRed);

	_radiusDecal->SetMaterial(0, isRed ? CastChecked<UMaterialInterface>(_assetLoader->MI_RedRadius) : CastChecked<UMaterialInterface>(_assetLoader->RadiusMaterial));
	_radiusDecal->SetVisibility(true);
	_radiusDecal->SetActive(true);
	_radiusDecal->DecalSize = FVector(100, 160, 160) * radius * 2 / 32;
}

void ABuildingPlacementSystem::StartBuildingPlacement(CardEnum buildingEnum, int32 buildingLvl, bool useBoughtCard, CardEnum useWildCard)
{
	if (!_gameInterface) return;

	CancelPlacement();


	/*
	 * Building starting with Drag
	 */
	if (buildingEnum == CardEnum::Farm)
	{
		_placementType = PlacementType::BuildingDrag;
		_buildingEnum = buildingEnum;
		StartDrag();

		_gridGuide->SetVisibility(false);
		_gridGuide->SetActive(false);
		_gameInterface->SetOverlayType(OverlayType::Farm, OverlaySetterType::BuildingPlacement);
		return;
	}

	
	
	PUN_LOG("StartBuildingPlacement: %s lvl:%d", ToTChar(GetBuildingInfo(buildingEnum).name), buildingLvl);

	_placementType = PlacementType::Building;
	_buildingEnum = buildingEnum;
	_buildingLvl = buildingLvl;
	//_faceDirection = Direction::S;

	_useBoughtCard = useBoughtCard;
	_useWildCard = useWildCard;
	
	//_lastPlacedInfo = PlacementInfo();

	_gridGuide->SetMaterial(0, _assetLoader->GridGuideMaterial);

	_placementGrid.SetActive(true);
	
	//! Radius and Overlay
	bool showGridGuide = true;

#define SHOW_RADIUS(CardEnumName) \
	else if (buildingEnum == CardEnum::CardEnumName) { \
		ShowRadius(CardEnumName::Radius, OverlayType::CardEnumName);\
	}
	
	//! Garden
	if (buildingEnum == CardEnum::Garden){
		ShowRadius(GetBuildingAppealInfo(buildingEnum).appealRadius, OverlayType::Appeal);
		showGridGuide = false;
		//_gameInterface->SetOverlayType(OverlayType::Appeal, OverlaySetterType::BuildingPlacement);
	}
	//! House
	else if (IsHouse(buildingEnum)) {
		showGridGuide = false;
		_gameInterface->SetOverlayType(OverlayType::Appeal, OverlaySetterType::BuildingPlacement);
	}
	//! Fisher
	else if (buildingEnum == CardEnum::Fisher) {
		ShowRadius(Fisher::Radius, OverlayType::Fish);
		showGridGuide = false;
	}
	//! BerryGatherer
	else if (buildingEnum == CardEnum::FruitGatherer) {
		ShowRadius(GathererHut::Radius, OverlayType::Gatherer);
	}
	else if (buildingEnum == CardEnum::HuntingLodge) {
		ShowRadius(HuntingLodge::Radius, OverlayType::Hunter);
		//_gameInterface->SetOverlayType(OverlayType::Hunter, OverlaySetterType::BuildingPlacement);
	}
	else if (buildingEnum == CardEnum::Forester) {
		ShowRadius(Forester::Radius, OverlayType::Forester);
		//_gameInterface->SetOverlayType(OverlayType::Forester, OverlaySetterType::BuildingPlacement);
	}
	else if (buildingEnum == CardEnum::Windmill) {
		ShowRadius(Windmill::Radius, OverlayType::Windmill);
	}
	SHOW_RADIUS(IrrigationReservoir)
	SHOW_RADIUS(Market)
	SHOW_RADIUS(ShippingDepot)
	
	else if (buildingEnum == CardEnum::Beekeeper) {
		ShowRadius(Beekeeper::Radius, OverlayType::Beekeeper);
		//_gameInterface->SetOverlayType(OverlayType::Forester, OverlaySetterType::BuildingPlacement);
	}
	
	//else if (buildingEnum == BuildingEnum::ConstructionOffice) {
	//	ShowRadius(ConstructionOffice::Radius);
	//	_gameInterface->SetOverlayType(OverlayType::ConstructionOffice, OverlaySetterType::BuildingPlacement);
	//}
	else if (buildingEnum == CardEnum::IndustrialistsGuild) {
		ShowRadius(10, OverlayType::Industrialist);
		//_gameInterface->SetOverlayType(OverlayType::Industrialist, OverlaySetterType::BuildingPlacement);
	}
	else if (buildingEnum == CardEnum::ConsultingFirm) {
		ShowRadius(10, OverlayType::Consulting);
		//_gameInterface->SetOverlayType(OverlayType::Consulting, OverlaySetterType::BuildingPlacement);
	}
	//else if (buildingEnum == CardEnum::Kidnap) {
	//	_gameInterface->SetOverlayType(OverlayType::Human, OverlaySetterType::BuildingPlacement);
	//}
	
	// Library/School ... Theatre/Tavern
	else if (buildingEnum == CardEnum::Library) {
		ShowRadius(Library::Radius, OverlayType::Library);
		//_gameInterface->SetOverlayType(OverlayType::Library, OverlaySetterType::BuildingPlacement);
	}
	else if (buildingEnum == CardEnum::School) {
		ShowRadius(School::Radius, OverlayType::School);
		//_gameInterface->SetOverlayType(OverlayType::School, OverlaySetterType::BuildingPlacement);
	}
	else if (buildingEnum == CardEnum::Bank) {
		ShowRadius(Bank::Radius, OverlayType::Bank);
		//_gameInterface->SetOverlayType(OverlayType::Bank, OverlaySetterType::BuildingPlacement);
	}
	
	else if (buildingEnum == CardEnum::Theatre) {
		ShowRadius(Theatre::Radius, OverlayType::Theatre);
		//_gameInterface->SetOverlayType(OverlayType::Theatre, OverlaySetterType::BuildingPlacement);
	}
	else if (buildingEnum == CardEnum::Tavern) {
		ShowRadius(Tavern::Radius, OverlayType::Tavern);
		//_gameInterface->SetOverlayType(OverlayType::Tavern, OverlaySetterType::BuildingPlacement);
	}
	// Shrine
	else if (buildingEnum == CardEnum::BlossomShrine) {
		//ShowRadius(BlossomShrine::Radius, OverlayType::);
	}
	// Bad Appeal
	else if (IsIndustryOrMine(buildingEnum)) {
		//_gameInterface->SetOverlayType(OverlayType::BadAppeal, OverlaySetterType::BuildingPlacement);
		ShowRadius(BadAppealRadius, OverlayType::BadAppeal, true);
	}

#undef SHOW_RADIUS

	// Spell
	if (IsAreaSpell(buildingEnum)) {
		showGridGuide = false;
	}

	//! Grid Guide
	if (showGridGuide) {
		_gridGuide->SetVisibility(true);
		_gridGuide->SetActive(true);
	}

	// Province Buildings always show province hover
	_gameInterface->SetAlwaysShowProvinceHover(buildingEnum == CardEnum::Fort || buildingEnum == CardEnum::Colony);
}

void ABuildingPlacementSystem::StartSetDeliveryTarget(int32 buildingId)
{
	_placementType = PlacementType::DeliveryTarget;
	_deliverySourceBuildingId = buildingId;
}

void ABuildingPlacementSystem::StartHarvestPlacement(bool isRemoving, ResourceEnum resourceEnum)
{
	_placementType = isRemoving ? PlacementType::GatherRemove : PlacementType::Gather;
	_harvestResourceEnum = resourceEnum;
	StartDrag();
}

void ABuildingPlacementSystem::StartDemolish()
{
	_placementType = PlacementType::Demolish;
	StartDrag();
}

void ABuildingPlacementSystem::StartRoad(bool isStoneRoad, bool isIntercity)
{
	if (isIntercity) {
		_placementType = PlacementType::IntercityRoad;
	} else {
		_placementType = isStoneRoad ? PlacementType::StoneRoad : PlacementType::DirtRoad;
	}
	StartDrag();
}

void ABuildingPlacementSystem::StartFence()
{
	_placementType = PlacementType::Fence;
	StartDrag();
}

void ABuildingPlacementSystem::StartBridge()
{
	_placementType = PlacementType::Bridge;
	StartDrag();
}
void ABuildingPlacementSystem::StartTunnel()
{
	_placementType = PlacementType::Tunnel;
	StartDrag();
}

void ABuildingPlacementSystem::StartDrag()
{	
	_dragState = DragState::NeedDragStart;
	_dragStartTile = WorldTile2::Invalid;

	_lastMouseOnTile = WorldTile2::Invalid;
	_lastDragState = DragState::None;
	_roadPathTileIds.Empty();
	
	_useBoughtCard = false;

	_placementGrid.SetActive(true);
}

void ABuildingPlacementSystem::TickLineDrag(WorldAtom2 cameraAtom, function<bool(WorldTile2)> isBuildableFunc)
{
	auto& simulation = _gameInterface->simulation();
	
	if (_dragState == DragState::NeedDragStart ||
		_dragState == DragState::LeftMouseDown)
	{
		if (simulation.tileOwner(_mouseOnTile) != _gameInterface->playerId()) {
			SetInstruction(PlacementInstructionEnum::OutsideTerritory, true);
		}
		_placementGrid.SpawnGrid(isBuildableFunc(_mouseOnTile) ? PlacementGridEnum::Green : PlacementGridEnum::Red, cameraAtom, _mouseOnTile);
	}
	else if (_dragState == DragState::Dragging) 
	{
		if (_placementType == PlacementType::Bridge ||
			_placementType == PlacementType::Tunnel)
		{
			CalculateBridgeLineDrag();

			// Invalid if 
			// When dragged across water. becomes valid...
			bool isValidBridgePlacement = true;
			int32 index = 0;
			int32 lastIndex = std::max(_area.sizeX(), _area.sizeY()) - 1;

			auto isSuitableTile = [&](WorldTile2 tile) {
				return (_placementType == PlacementType::Bridge) ? simulation.IsWater(tile) : simulation.IsMountain(tile);
			};
			

			if (lastIndex >= 2) { // bridge must be at least size 3
				int32_t waterTileCount = 0;
				
				_area.ExecuteOnArea_WorldTile2([&](WorldTile2 tile) {
					bool roadBuildable = isBuildableFunc(tile);
					if (!roadBuildable) {
						if (isSuitableTile(tile)) {
							waterTileCount++;
						} else {
							isValidBridgePlacement = false; // Not buildable, and not water, this is obstacle
						}
					}

					// End points must be land
					if (index == 0 || index == lastIndex) {
						if (!roadBuildable) {
							isValidBridgePlacement = false;
						}
					}
					index++;
				});

				if (waterTileCount == 0) {
					isValidBridgePlacement = false;
				}
				
			} else {
				isValidBridgePlacement = false;
			}

			_area.ExecuteOnArea_WorldTile2([&](WorldTile2 tile) {
				if (isValidBridgePlacement) {
					PUN_CHECK(isBuildableFunc(tile) || isSuitableTile(tile));
					if (simulation.tileHasBuilding(tile) && !IsRoad(simulation.buildingEnumAtTile(tile))) {
						_placementGrid.SpawnGrid(PlacementGridEnum::Red, cameraAtom, tile);
					} else {
						// Show gray on non water tiles
						_placementGrid.SpawnGrid(isSuitableTile(tile) ? PlacementGridEnum::Green : PlacementGridEnum::Gray, cameraAtom, tile);
					}
				} else {
					_placementGrid.SpawnGrid(PlacementGridEnum::Red, cameraAtom, tile);
				}
			});
			
		}
		else 
		{
			CalculateRoadLineDrag(isBuildableFunc);

			// Use roadPath if there is one.
			if (_roadPathTileIds.Num() > 0)
			{
				for (int32 i = 0; i < _roadPathTileIds.Num(); i++) {
					_placementGrid.SpawnGrid(_canPlaceRoad ? PlacementGridEnum::Green : PlacementGridEnum::Red, cameraAtom, WorldTile2(_roadPathTileIds[i]));
				}
			}
			//else
			//{
			//	// Extends isBuildable function to include territory checking
			//	auto isBuildableFunc2 = [&](WorldTile2 tile) {
			//		if (simulation.tileOwner(tile) != _gameInterface->playerId()) {
			//			SetInstruction(PlacementInstructionEnum::OutsideTerritory, true);
			//		}
			//		return isBuildableFunc(tile);
			//	};
			//	
			//	// Fallback to old method..
			//	_area.ExecuteOnArea_WorldTile2([&](WorldTile2 tile) {
			//		_placementGrid.SpawnGrid(isBuildableFunc2(tile) ? PlacementGridEnum::Green : PlacementGridEnum::Red, cameraAtom, tile);
			//	});
			//	if (!_area2.isInvalid()) {
			//		_area2.ExecuteOnArea_WorldTile2([&](WorldTile2 tile) {
			//			_placementGrid.SpawnGrid(isBuildableFunc2(tile) ? PlacementGridEnum::Green : PlacementGridEnum::Red, cameraAtom, tile);
			//		});
			//	}
			//}
		}
	}
	else {
		UE_DEBUG_BREAK();
	}
	
	_canPlace = _placementGrid.IsDisplayCountZero(PlacementGridEnum::Red) && 
				_placementGrid.IsDisplayCountZero(PlacementGridEnum::ArrowRed);

	_placementGrid.AfterAdd();
}

void ABuildingPlacementSystem::TickAreaDrag(WorldAtom2 cameraAtom, function<PlacementGridEnum(WorldTile2)> getPlacementGridEnum)
{
	_canPlace = true;

	if (_dragState == DragState::NeedDragStart ||
		_dragState == DragState::LeftMouseDown)
	{
		//PUN_DEBUG_FAST(FString::Printf(TEXT("Tick DragStart %d, %d"), _mouseOnTile.x, _mouseOnTile.y));
		_placementGrid.SpawnGrid(getPlacementGridEnum(_mouseOnTile), cameraAtom, _mouseOnTile);
	}
	else if (_dragState == DragState::Dragging)
	{
		CalculateDragArea();
		_area.ExecuteOnArea_WorldTile2([&](WorldTile2 tile) {
			SCOPE_CYCLE_COUNTER(STAT_DragSpawnPlacement);
			_placementGrid.SpawnGrid(getPlacementGridEnum(tile), cameraAtom, tile);
		});


		// Building area drag not always placable
		if (_placementType == PlacementType::BuildingDrag)
		{
			_canPlace = _placementGrid.IsDisplayCountZero(PlacementGridEnum::Red);

			if (_buildingEnum == CardEnum::Farm) {
				if (IsFarmSizeInvalid(_area)) {
					_canPlace = false;
				}
			}
			else if (_buildingEnum == CardEnum::StorageYard) {
				if (IsStorageTooLarge(_area)) {
					_canPlace = false;
				}
			}
		}
	}

	
	_placementGrid.AfterAdd();
}

void ABuildingPlacementSystem::FinishDrag(IGameNetworkInterface* networkInterface)
{
	// Reset the drag
	_dragState = DragState::NeedDragStart;
	_dragStartTile = WorldTile2::Invalid;
	_placementGrid.Clear();

	if (_placementType != PlacementType::None) { //TODO: find out why I need this...
		NetworkDragPlace(networkInterface, _placementType);
	}
}

void ABuildingPlacementSystem::LeftClickDown(IGameNetworkInterface* networkInterface)
{
	if (_placementType == PlacementType::Building)
	{
		if (_buildingEnum == CardEnum::StorageYard)
		{
			if (_canPlace)
			{
				StartDrag();

				_placementType = PlacementType::BuildingDrag;
				_area2 = _area;
				_dragState = DragState::LeftMouseDown;
				_dragStartTile = _mouseOnTile;
			}
			else {
				_gameInterface->Spawn2DSound("UI", "PopupCannot");
			}
			return;
		}
	}

	// TODO: use for ranch
	// Storage Transition to drag start once chosen the gate area...
	//if (_placementType == PlacementType::Building && 
	//	_buildingEnum == CardEnum::StorageYard)
	//{
	//	if (_canPlace)
	//	{
	//		StartDrag();

	//		_placementType = PlacementType::BuildingDrag;
	//		_area2 = _area;
	//		_dragState = DragState::LeftMouseDown;
	//		_dragStartTile = _mouseOnTile;
	//	}
	//	else {
	//		_gameInterface->Spawn2DSound("UI", "PopupCannot");
	//	}
	//	return;
	//}
	
	NetworkTryPlaceBuilding(networkInterface);

	// Drag can start by leftClick down/up on the same tile, this drag will end with another leftClickDown
	if (_dragState == DragState::NeedDragStart && _canPlace) {
		_dragState = DragState::LeftMouseDown;
		_dragStartTile = _mouseOnTile;
	}
	// Up Drag
	else if (_dragState == DragState::Dragging)  {
		if (_canPlace) {
			FinishDrag(networkInterface);
		} else {
			_gameInterface->Spawn2DSound("UI", "PopupCannot");
		}
	}
}

void ABuildingPlacementSystem::LeftClickUp(IGameNetworkInterface* networkInterface)
{
	// Up Drag
	if (_dragState == DragState::LeftMouseDown && _dragStartTile == _mouseOnTile && _canPlace)
	{
		// Special case for demolish which ppl expect it to destroy building on _mouseOnTile right away
		if (_placementType == PlacementType::Demolish) {
			_area = TileArea(_mouseOnTile, 0);
			FinishDrag(networkInterface);
		} else {
			_dragState = DragState::Dragging;
		}
	}
	// Down Drag
	else if (_dragState == DragState::Dragging && _canPlace) {
		FinishDrag(networkInterface);
	}
}

void ABuildingPlacementSystem::CancelPlacement()
{
	if (!_gameInterface) return;

	//_buildingMeshes->Hide();

	if (IsRoadPlacement(_placementType)) {
		// Ensure TileObj Refresh to hide trees
		_gameInterface->simulation().SetNeedDisplayUpdate(DisplayClusterEnum::Trees, _gameInterface->sampleRegionIds());
	}

	if (_dragState == DragState::Dragging) {
		if (_buildingEnum == CardEnum::StorageYard) {
			_placementType = PlacementType::Building;
		}
		StartDrag();
		return;
	}
	

	_placementType = PlacementType::None;
	_dragState = DragState::None;
	_buildingEnum = CardEnum::None;

	_deliverySourceBuildingId = -1;

	_placementGrid.SetActive(false);
	_placementGrid.Clear();

	_gridGuide->SetVisibility(false);
	_gridGuide->SetActive(false);

	_radiusDecal->SetVisibility(false);
	_radiusDecal->SetActive(false);

	if (_useWildCard != CardEnum::None) {
		_useWildCard = CardEnum::None;
		_gameInterface->simulation().cardSystem(_gameInterface->playerId()).converterCardState = ConverterCardUseState::None;
	}
	
	_gameInterface->SetOverlayType(OverlayType::None, OverlaySetterType::BuildingPlacement);
	_gameInterface->SetOverlayTile(WorldTile2::Invalid);

	_placementGridDelayed.SetActive(false);

	_networkInterface->SetCursor("Slate/MouseCursor");
}

void ABuildingPlacementSystem::RotatePlacement()
{
	if (_placementType == PlacementType::Building) {
		_faceDirection = RotateDirection(_faceDirection);

		_timesRotated++;
		_justRotated = true;
	}
}

void ABuildingPlacementSystem::CalculateDragArea() 
{
	_area = TileArea(min(_dragStartTile.x, _mouseOnTile.x),
					min(_dragStartTile.y, _mouseOnTile.y),
					max(_dragStartTile.x, _mouseOnTile.x),
					max(_dragStartTile.y, _mouseOnTile.y));

	_area.EnforceWorldLimit();

	// Ensure that the area isn't too large
	if (_area.sizeX() > 500) {
		_area.maxX = _area.minX + 499;
	}
	if (_area.sizeY() > 500) {
		_area.maxY = _area.minY + 499;
	}

	if (_placementType == PlacementType::BuildingDrag) 
	{
		// Storage is at least 2 tiles dimension
		if (_buildingEnum == CardEnum::StorageYard)
		{
			_area = TileArea(min(_area2.minX, _mouseOnTile.x),
						min(_area2.minY, _mouseOnTile.y),
						max(_area2.maxX, _mouseOnTile.x),
						max(_area2.maxY, _mouseOnTile.y));
		}

	}

	// TODO: use for ranch
	// Ranch start using the initial Barn placement...
	//if (_placementType == PlacementType::BuildingDrag &&
	//	_buildingEnum == CardEnum::StorageYard) 
	//{
	//	_area = TileArea(min(_area2.minX, _mouseOnTile.x),
	//				min(_area2.minY, _mouseOnTile.y),
	//				max(_area2.maxX, _mouseOnTile.x),
	//				max(_area2.maxY, _mouseOnTile.y));

	//	// Trim out area beyond building front
	//	if (_faceDirection == Direction::N) {
	//		_area.maxX = min(_area.maxX, _area2.maxX);
	//	}
	//	else if (_faceDirection == Direction::S) {
	//		_area.minX = max(_area.minX, _area2.minX);
	//	}
	//	else if (_faceDirection == Direction::E) {
	//		_area.maxY = min(_area.maxY, _area2.maxY);
	//	}
	//	else if (_faceDirection == Direction::W) {
	//		_area.minY = max(_area.minY, _area2.minY);
	//	}
	//}

	check(_area.minX > -1000);
	check(_area.minY > -1000);
}

void ABuildingPlacementSystem::CalculateRoadLineDrag(function<bool(WorldTile2)> isBuildableFunc)
{
	// Use BFS
	_roadPathTileIds.Empty();
	WorldTile2 start = _dragStartTile;
	WorldTile2 end = _mouseOnTile;

	unordered_map<int32, int32> tileIdToPrevTileId;

	std::vector<WorldTile2> queueTile;
	queueTile.push_back(start);

	auto addNeighbor = [&](WorldTile2 neighborTile, WorldTile2& curTile) {
		if (tileIdToPrevTileId.find(neighborTile.tileId()) == tileIdToPrevTileId.end()) {
			queueTile.push_back(neighborTile);
			tileIdToPrevTileId[neighborTile.tileId()] = curTile.tileId();

			//PUN_LOG("AddTile: %s", *neighborTile.To_FString());
		}
	};

	PUN_LOG("Start Pathing");

	int32 count = 0;
	while (!queueTile.empty())
	{
		count++;
		if (count > 30000) {
			break;
		}

		WorldTile2 curTile = queueTile.front();
		queueTile.erase(queueTile.begin());

		//PUN_LOG("CurTile: %s ... buildable:%d", *curTile.To_FString(), _gameInterface->IsPlayerRoadBuildable(curTile));

		if (!isBuildableFunc(curTile)) {
			continue;
		}

		if (curTile == end) {
			// reconstruct path and return
			int32_t curTileId = curTile.tileId();
			_roadPathTileIds.Add(curTileId);
			while (curTileId != start.tileId()) {
				curTileId = tileIdToPrevTileId[curTileId];
				_roadPathTileIds.Add(curTileId);
			}

			// Add corners to non-diagonal road.
			// When there are 2 tiles adjacent before and after the current tile, we fill in the corner
			if (_roadPathTileIds.Num() > 5) {
				for (int32 i = _roadPathTileIds.Num() - 2; i-- > 2;) {
					WorldTile2 prev1Tile(_roadPathTileIds[i + 1]);
					WorldTile2 prev2Tile(_roadPathTileIds[i + 2]);
					WorldTile2 forward0Tile(_roadPathTileIds[i]);
					WorldTile2 forward1Tile(_roadPathTileIds[i - 1]);
					
					bool previousAdjacent = prev1Tile.IsAdjacentTo(prev2Tile);
					bool midAdjacent = prev2Tile.IsAdjacentTo(forward0Tile);
					bool forwardAdjacent = forward0Tile.IsAdjacentTo(forward1Tile);
					if (!midAdjacent && previousAdjacent && forwardAdjacent)
					{
						WorldTile2 directionFromPrevious = prev1Tile - prev2Tile;
						WorldTile2 cornerTile = directionFromPrevious + prev1Tile;

						if (isBuildableFunc(cornerTile)) {
							_roadPathTileIds.Insert(cornerTile.tileId(), i + 1);
						}
					}
				}
			}

			_canPlaceRoad = true;
			
			PUN_LOG("CalculateRoadLineDrag FoundPath: %d", _roadPathTileIds.Num());
			return;
		}

		// Queue up nearby tiles
		// Prioritize up, right, left, down like banished
		addNeighbor(WorldTile2(curTile.x + 1, curTile.y), curTile); // up
		addNeighbor(WorldTile2(curTile.x, curTile.y + 1), curTile); // right
		addNeighbor(WorldTile2(curTile.x, curTile.y - 1), curTile); // left
		addNeighbor(WorldTile2(curTile.x - 1, curTile.y), curTile); // down

		addNeighbor(WorldTile2(curTile.x + 1, curTile.y + 1), curTile); // up+right
		addNeighbor(WorldTile2(curTile.x + 1, curTile.y - 1), curTile); // up+left
		addNeighbor(WorldTile2(curTile.x - 1, curTile.y + 1), curTile); // down+right
		addNeighbor(WorldTile2(curTile.x - 1, curTile.y - 1), curTile); // down+left
	}

	CalculateLineDrag();
}

void ABuildingPlacementSystem::CalculateLineDrag()
{
	// Backup
	_roadPathTileIds.Empty();
	WorldTile2 start = _dragStartTile;
	WorldTile2 end = _mouseOnTile;

	_roadPathTileIds.Add(start.tileId());
	WorldTile2 curTile = start;
	
	while (curTile != end)
	{
		int16 xDiff = end.x - curTile.x;
		int16 yDiff = end.y - curTile.y;
		WorldTile2 diff((xDiff == 0) ? 0 : xDiff / abs(xDiff), 
						(yDiff == 0) ? 0 : yDiff / abs(yDiff));
		curTile += diff;
		
		_roadPathTileIds.Add(curTile.tileId());
	}

	_canPlaceRoad = false;
	
	//_area = TileArea(min(_dragStartTile.x, _mouseOnTile.x),
	//	min(_dragStartTile.y, _mouseOnTile.y),
	//	max(_dragStartTile.x, _mouseOnTile.x),
	//	max(_dragStartTile.y, _mouseOnTile.y));
	//_area2 = TileArea::Invalid;// Clear to make sure
	//check(_area.minX > -1000);
	//check(_area.minY > -1000);

	//bool isLongX = _area.sizeX() >= _area.sizeY();
	//if (isLongX) {
	//	// LongX means that x goes between dragStartTile.x and mouseOnTile.x, when y is dragStartTile.y
	//	_area = TileArea(min(_dragStartTile.x, _mouseOnTile.x), _dragStartTile.y,
	//		max(_dragStartTile.x, _mouseOnTile.x), _dragStartTile.y);
	//	// Then y goes between dragStartTile.y and mouseOnTile.y, when x is mouseOnTile.x
	//	// -1 is to prevent double tile
	//	if (_dragStartTile.y > _mouseOnTile.y) {
	//		_area2 = TileArea(_mouseOnTile.x, _mouseOnTile.y,
	//			_mouseOnTile.x, _dragStartTile.y - 1);
	//	}
	//	else if (_dragStartTile.y < _mouseOnTile.y) {
	//		_area2 = TileArea(_mouseOnTile.x, _dragStartTile.y + 1,
	//			_mouseOnTile.x, _mouseOnTile.y);
	//	}
	//}
	//else {
	//	_area = TileArea(_dragStartTile.x, min(_dragStartTile.y, _mouseOnTile.y),
	//		_dragStartTile.x, max(_dragStartTile.y, _mouseOnTile.y));
	//	if (_dragStartTile.x > _mouseOnTile.x) {
	//		_area2 = TileArea(_mouseOnTile.x, _mouseOnTile.y,
	//			_dragStartTile.x - 1, _mouseOnTile.y);
	//	}
	//	else if (_dragStartTile.x < _mouseOnTile.x) {
	//		_area2 = TileArea(_dragStartTile.x + 1, _mouseOnTile.y,
	//			_mouseOnTile.x, _mouseOnTile.y);
	//	}
	//}
}

void ABuildingPlacementSystem::CalculateBridgeLineDrag()
{
	_area = TileArea(min(_dragStartTile.x, _mouseOnTile.x),
					min(_dragStartTile.y, _mouseOnTile.y),
					max(_dragStartTile.x, _mouseOnTile.x),
					max(_dragStartTile.y, _mouseOnTile.y));
	_area2 = TileArea::Invalid;// Clear to make sure
	check(_area.minX > -1000);
	check(_area.minY > -1000);

	bool isLongX = _area.sizeX() >= _area.sizeY();
	if (isLongX) {
		// LongX means that x goes between dragStartTile.x and mouseOnTile.x, when y is dragStartTile.y
		_area = TileArea(min(_dragStartTile.x, _mouseOnTile.x), _dragStartTile.y,
			max(_dragStartTile.x, _mouseOnTile.x), _dragStartTile.y);

		// Do not allow bridge longer than 100 tiles
		if (_area.sizeX() > 100) {
			// Shrink so that we keep _dragStartTile
			if (_area.minX == _dragStartTile.x) {
				_area.maxX = _area.minX + 100;
			} else {
				_area.minX = _area.maxX - 100;
			}
		}
	}
	else {
		_area = TileArea(_dragStartTile.x, min(_dragStartTile.y, _mouseOnTile.y),
			_dragStartTile.x, max(_dragStartTile.y, _mouseOnTile.y));

		// Do not allow bridge longer than 100 tiles
		if (_area.sizeY() > 100) {
			// Shrink so that we keep _dragStartTile
			if (_area.minY == _dragStartTile.y) {
				_area.maxY = _area.minY + 100;
			} else {
				_area.minY = _area.maxY - 100;
			}
		}
	}
}

void ABuildingPlacementSystem::HighlightDemolishAreaBuildingRed()
{
	// Highlight any building red
	std::vector<int32> buildingIds;
	_demolishHighlightArea.ExecuteOnArea_WorldTile2([&](WorldTile2 tile) 
	{
		if (tile.isValid()) 
		{
			int32 buildingId = _gameInterface->simulation().buildingIdAtTile(tile);
			if (buildingId != -1) {
				buildingIds.push_back(buildingId);
			}
		}
	});

	for (int32 buildingId : buildingIds) {
		Building& building = _gameInterface->simulation().building(buildingId);
		if (building.isConstructed()) {
			_gameInterface->ShowBuildingMesh(building, 3);
		}
	}
}

void ABuildingPlacementSystem::TickPlacement(AGameManager* gameInterface, IGameNetworkInterface* networkInterface,
											MouseInfo mouseInfo, WorldAtom2 cameraAtom)
{
	// TODO: find proper place to set this?
	_gameInterface = gameInterface;
	_networkInterface = networkInterface;

	_forceCannotPlace = false;

	if (gameInterface == nullptr) {
		return;
	}

	// Hide delay filler mesh if it exceeds the counter...
	if (_lastNetworkPlacementTime > 0.0f)
	{
		float time = UGameplayStatics::GetTimeSeconds(GetWorld());
		if (time > _lastNetworkPlacementTime + NetworkInputDelayTime) {
			_lastNetworkPlacementTime = -1.0f;
		}
		else
		{
			// Show construction right away (Fake responsiveness

			std::vector<ModuleTransform> modules;
			if (_delayFillerEnum == CardEnum::Fisher) {
				modules.insert(modules.begin(), ModuleTransform("FisherConstructionPoles", FTransform::Identity, 0.0f, ModuleTypeEnum::ConstructionOnly));
				modules.insert(modules.begin(), ModuleTransform("FisherConstructionPolesWater", FTransform::Identity, 0.0f, ModuleTypeEnum::ConstructionOnly));
				
				_gameInterface->ShowBuildingMesh(_delayFillerTile, _delayFillerFaceDirection, modules, 0);
				_gameInterface->ShowDecal(_delayFillerArea, _assetLoader->ConstructionBaseDecalMaterial);
			}
			else if (_delayFillerEnum == CardEnum::TradingPort) {
				modules.insert(modules.begin(), ModuleTransform("TradingPortConstructionPoles", FTransform::Identity, 0.0f, ModuleTypeEnum::ConstructionOnly));
				modules.insert(modules.begin(), ModuleTransform("TradingPortConstructionPolesWater", FTransform::Identity, 0.0f, ModuleTypeEnum::ConstructionOnly));

				_gameInterface->ShowBuildingMesh(_delayFillerTile, _delayFillerFaceDirection, modules, 0);
				_gameInterface->ShowDecal(_delayFillerArea, _assetLoader->ConstructionBaseDecalMaterial);
			}
			// Typical case
			else if (_delayFillerEnum != CardEnum::SpeedBoost &&
				!IsActionCard(_delayFillerEnum) &&
				!IsDecorativeBuilding(_delayFillerEnum))
			{
				// TODO: NO MORE CONSTRUCTION POLES?
				
				//WorldTile2 size = _delayFillerArea.size();// GetBuildingInfo(_delayFillerEnum).size;
				////FVector siteMarkingScale(size.x, size.y, 1);
				//FVector position(size.x % 2 == 0 ? 5 : 0, size.y % 2 == 0 ? 5 : 0, 0.0f);
				//FTransform siteMarkTransform(FRotator::ZeroRotator, position, siteMarkingScale);

				//GameDisplayUtils::GetConstructionPoles(_delayFillerArea, modules);

				//_gameInterface->ShowBuildingMesh(_delayFillerTile, Direction::S, modules, 0);
				//_gameInterface->ShowDecal(_delayFillerArea, _assetLoader->ConstructionBaseDecalMaterial);
			}


			if (_delayFillerEnum != CardEnum::SpeedBoost &&
				!IsActionCard(_delayFillerEnum) &&
				!IsDecorativeBuilding(_delayFillerEnum))
			{
				_gameInterface->ShowBuildingMesh(_delayFillerTile, _delayFillerFaceDirection, modules, 0);
				_gameInterface->ShowDecal(_delayFillerArea, _assetLoader->ConstructionBaseDecalMaterial);
			}
		}
	}

	// Highlight Demolish area
	{
		// Close down demolish highlight if no longer need it...
		bool showingConfirmation = networkInterface->IsShowingConfirmationUI("Are you sure you want to demolish?") ||
									networkInterface->IsShowingConfirmationUI("Are you sure you want to demolish?\nFort and Colony Cards will not be recovered.");
		bool isDemolishing = _placementType == PlacementType::Demolish || showingConfirmation;
		if (!isDemolishing) {
			_demolishHighlightArea = TileArea();
		}

		// During confirmation, still display the demolish area...
		if (showingConfirmation) {
			_placementGridDelayed.BeforeAdd();
			_demolishHighlightArea.ExecuteOnArea_WorldTile2([&](WorldTile2 tile) {
				SCOPE_CYCLE_COUNTER(STAT_DragSpawnPlacement);
				_placementGridDelayed.SpawnGrid(PlacementGridEnum::Red, cameraAtom, tile);
			});
			_placementGridDelayed.AfterAdd();

			HighlightDemolishAreaBuildingRed();
		} else {
			_placementGridDelayed.SetActive(false);
		}
	}
	
	// Don't tick if we are not placing anything...
	if (!mouseInfo.isValid || _placementType == PlacementType::None) {
		return;
	}
	
	_mouseOnTile = MapUtil::AtomLocation(cameraAtom, mouseInfo.mouseHitLocation()).worldTile2();
	int32 playerId = networkInterface->playerId();
	GameSimulationCore& simulation = _gameInterface->simulation();

	// TODO: might not need this anymore??
	//// Don't recalculate if there is no state change
	//if (_mouseOnTile == _lastMouseOnTile && 
	//	_dragState == _lastDragState && 
	//	!_justRotated && !justPlacedBuilding()) {
	//	return;
	//}

	// Ensure bought card is valid (in case of double click etc.)
	if (_placementType == PlacementType::Building && _useBoughtCard)
	{
		// If Card is no longer valid, stop placement
		if (!simulation.cardSystem(playerId).CanUseBoughtCard(_buildingEnum))
		{
			CancelPlacement();
			return;
		}
	}
	
	//if (_placementType == PlacementType::Building &&
	//	!_useBoughtCard &&
	//	!simulation.IsPermanentBuilding(playerId, _buildingEnum) &&
	//	!simulation.cardSystem(playerId).CanUseBoughtCard(_buildingEnum) &&
	//	!IsSkillEnum(_buildingEnum))
	//{
	//	CancelPlacement();
	//	return;
	//}

	// Play Drag Sound each time mouse move a tile during drag
	if (_dragState == DragState::Dragging && _mouseOnTile != _lastMouseOnTile) {
		_gameInterface->Spawn2DSound("UI", "PlacementDrag");
	}

	_lastMouseOnTile = _mouseOnTile;
	_lastDragState = _dragState;
	_justRotated = false;

	ClearInstructions();

	// Down Drag
	if (_dragState == DragState::LeftMouseDown && 
		_dragStartTile != _mouseOnTile) 
	{
		_dragState = DragState::Dragging;
	}

	GameRegionSystem& regionSystem = simulation.regionSystem();
	PunTerrainGenerator& terrainGenerator = simulation.terrainGenerator();

	_placementGrid.BeforeAdd();

	/*
	 * Gathering
	 */
	if (_placementType == PlacementType::Gather) 
	{
		TreeSystem& treeSystem = simulation.treeSystem();
		
		TickAreaDrag(cameraAtom, [&](WorldTile2 tile)
		{
			SCOPE_CYCLE_COUNTER(STAT_DragSpawnPlacement);
			//if (!regionSystem.IsOwnedByPlayer(tile.regionId(), playerId)) {
			if (simulation.tileOwner(tile) != playerId) {
				SetInstruction(PlacementInstructionEnum::OutsideTerritory, true);
				return PlacementGridEnum::Red;
			}

			const TileObjInfo& tileInfo = treeSystem.tileInfo(tile.tileId());
			ResourceTileType tileType = tileInfo.type;

			bool gatherable = false;
			if (_harvestResourceEnum == ResourceEnum::Wood) {
				gatherable = (tileType == ResourceTileType::Tree);
			}
			else if (_harvestResourceEnum == ResourceEnum::Orange) {
				gatherable = (tileType == ResourceTileType::Tree && (tileInfo.treeEnum != TileObjEnum::Orange && tileInfo.treeEnum != TileObjEnum::Papaya && tileInfo.treeEnum != TileObjEnum::Coconut));
			}
			else if (_harvestResourceEnum == ResourceEnum::Stone) {
				gatherable = (tileType == ResourceTileType::Deposit);
			}
			else if (_harvestResourceEnum == ResourceEnum::None) {
				gatherable = (tileType == ResourceTileType::Tree || tileType == ResourceTileType::Deposit);
			}

			//bool notGatherable = tileType != ResourceTileType::Tree && tileType != ResourceTileType::Deposit;
			
			return gatherable ? PlacementGridEnum::Green : PlacementGridEnum::Gray;
		});
		
		return;
	} 
	
	if (_placementType == PlacementType::GatherRemove) 
	{
		TreeSystem& treeSystem = simulation.treeSystem();
		
		TickAreaDrag(cameraAtom, [&](WorldTile2 tile)
		{
			SCOPE_CYCLE_COUNTER(STAT_DragSpawnPlacement);

			bool isRemovable = false;
			
			if (treeSystem.HasMark(playerId, tile.tileId())) 
			{
				ResourceTileType tileType = treeSystem.tileInfo(tile.tileId()).type;
				
				if (_harvestResourceEnum == ResourceEnum::Wood) {
					isRemovable = tileType == ResourceTileType::Tree;
				}
				else if (_harvestResourceEnum == ResourceEnum::Stone) {
					isRemovable = tileType == ResourceTileType::Deposit;
				}
				else if (_harvestResourceEnum == ResourceEnum::None) {
					isRemovable = tileType == ResourceTileType::Tree || tileType == ResourceTileType::Deposit;
				}
			}
			
			return isRemovable ? PlacementGridEnum::Red : PlacementGridEnum::Gray;
		});
		
		return;
	}

	/*
	 * Demolish
	 */
	if (_placementType == PlacementType::Demolish)
	{
		TickAreaDrag(cameraAtom, [&](WorldTile2 tile)
		{
			SCOPE_CYCLE_COUNTER(STAT_DragSpawnPlacement);
			return PlacementGridEnum::Red;
		});

		if (_dragState == DragState::Dragging) {
			_demolishHighlightArea = _area;
		}
		
		HighlightDemolishAreaBuildingRed();
		return;
	}

	/*
	 * Roads / Bridge
	 */
	if (_placementType == PlacementType::DirtRoad ||
		_placementType == PlacementType::StoneRoad ||
		_placementType == PlacementType::Fence ||
		_placementType == PlacementType::Bridge ||
		_placementType == PlacementType::Tunnel)
	{
		//TickLineDrag(cameraAtom, std::bind(&IGameManagerInterface::IsPlayerRoadBuildable, _gameInterface, _1));
		TickLineDrag(cameraAtom, [&](WorldTile2 tile) {
			return _gameInterface->IsPlayerRoadBuildable(tile);
		});
		return;
	}
	if (_placementType == PlacementType::IntercityRoad) 
	{
		TickLineDrag(cameraAtom, [&](WorldTile2 tile) {
			return _gameInterface->IsIntercityRoadBuildable(tile);
		});
		return;
	}

	/*
	 * Farms/Storage
	 */
	if (_placementType == PlacementType::BuildingDrag)
	{
		/*
		 * Farms Drag
		 */
		if (_buildingEnum == CardEnum::Farm)
		{
			TickAreaDrag(cameraAtom, [&](WorldTile2 tile)
			{
				if (IsPlayerBuildable(tile))
				{
					if (simulation.GetFertilityPercent(tile) < 20) {
						SetInstruction(PlacementInstructionEnum::FarmAndRanch, true);
						return PlacementGridEnum::Red;
					}

					// Any seed that can be planted here?
					std::vector<SeedInfo> seedsOwned = simulation.resourceSystem(_gameInterface->playerId()).seedsPlantOwned();
					GeoresourceEnum georesourceEnum = simulation.georesource(simulation.GetProvinceIdClean(tile)).georesourceEnum;
					
					bool hasValidSeed = false;
					for (SeedInfo seed : seedsOwned)
					{
						if (IsCommonSeedCard(seed.cardEnum)) {
							hasValidSeed = true;
							break;
						}
						if (IsSpecialSeedCard(seed.cardEnum) &&
							seed.georesourceEnum == georesourceEnum)
						{
							hasValidSeed = true;
							break;
						}
					}

					if (!hasValidSeed) {
						SetInstruction(PlacementInstructionEnum::FarmNoValidSeedForRegion, true);
						return PlacementGridEnum::Red;
					}
					return PlacementGridEnum::Green;
				}
				return PlacementGridEnum::Red;
			});
			
			return;
		}
		/*
		 * StorageYard Drag
		 */
		if (_buildingEnum == CardEnum::StorageYard)
		{
			bool isAllRed = false;

			// Gray on area not 2x2
			bool isOddX = _area.sizeX() % 2 == 1;
			bool isOddY = _area.sizeY() % 2 == 1;
			
			_area3 = _area;
			
			if (isOddX) {
				if (_area.minX == _area2.minX)	{
					_area3.maxX--;
				} else {
					_area3.minX++;
				}
			}
			if (isOddY) {
				if (_area.minY == _area2.minY) {
					_area3.maxY--;
				} else {
					_area3.minY++;
				}
			}
			
			TickAreaDrag(cameraAtom, [&](WorldTile2 tile)
			{
				if (IsPlayerBuildable(tile) && !isAllRed) {
					return _area3.HasTile(tile) ? PlacementGridEnum::Green : PlacementGridEnum::Gray;
				}
				return PlacementGridEnum::Red;
			});

			// TODO: for Ranch
			//// Show front road
			//TileArea frontArea = _area2.GetFrontArea(_faceDirection);
			//frontArea.ExecuteOnArea_WorldTile2([&](WorldTile2 tile) {
			//	_placementGrid.SpawnGrid(simulation.overlaySystem().IsRoad(tile) ? PlacementGridEnum::ArrowGreen : PlacementGridEnum::ArrowYellow, cameraAtom, tile, _faceDirection);
			//});

			// Show meshes
			if (_buildingEnum == CardEnum::StorageYard) {
				_gameInterface->ShowStorageMesh(_area3, _area3.centerTile());
			}

			return;
		}
		else {
			UE_DEBUG_BREAK();
			return;
		}
	}
	

	//*
	// * Fence
	// */
	//if (_placementType == PlacementType::Fence)
	//{
	//	TickLineDrag(cameraAtom, std::bind(&IGameManagerInterface::IsPlayerRoadBuildable, _gameInterface, _1));
	//	return;
	//}

	/*
	 * Delivery Point
	 */
	if (_placementType == PlacementType::DeliveryTarget)
	{
		_canPlace = false;
		
		if (_mouseOnTile.isValid())
		{
			// Show DeliveryArrow
			if (simulation.IsValidBuilding(_deliverySourceBuildingId))
			{
				Building& sourceBuilding = simulation.building(_deliverySourceBuildingId);
				Building* buildingAtTile = simulation.buildingAtTile(_mouseOnTile);
				if (buildingAtTile) 
				{
					WorldTile2 targetCenterTile = buildingAtTile->centerTile();
					
					if (IsStorage(buildingAtTile->buildingEnum())) 
					{
						if (sourceBuilding.isEnum(CardEnum::ShippingDepot) &&
							sourceBuilding.DistanceTo(targetCenterTile) <= ShippingDepot::Radius) {
							// don't allow setting target within shipping depot's radius
							SetInstruction(PlacementInstructionEnum::ShipperDeliveryPointShouldBeOutOfRadius, true);
							_placementGrid.SpawnGrid(PlacementGridEnum::Red, cameraAtom, targetCenterTile);
						}
						else {
							_placementGrid.SpawnGrid(PlacementGridEnum::Green, cameraAtom, targetCenterTile);
							_canPlace = true;
						}
					}
					else {
						SetInstruction(PlacementInstructionEnum::DeliveryPointMustBeStorage, true);
						_placementGrid.SpawnGrid(PlacementGridEnum::Red, cameraAtom, targetCenterTile);
					}
				}
				else {
					SetInstruction(PlacementInstructionEnum::DeliveryPointInstruction, true);
					_placementGrid.SpawnGrid(PlacementGridEnum::Red, cameraAtom, _mouseOnTile);
				}

				FVector mouseLocation;
				if (buildingAtTile) {
					mouseLocation = _gameInterface->DisplayLocationTrueCenter(*buildingAtTile);
				} else {
					mouseLocation = _gameInterface->DisplayLocation(_mouseOnTile.worldAtom2());
				}
				FVector sourceLocation = _gameInterface->DisplayLocationTrueCenter(sourceBuilding);

				_gameInterface->ShowDeliveryArrow(sourceLocation, mouseLocation);
			}
		}
		else {
			_placementGrid.SpawnGrid(PlacementGridEnum::Red, cameraAtom, _mouseOnTile);
		}

		_placementGrid.AfterAdd();
		
		return;
	}
	
	
	/*
	 * Building
	 */
	BldInfo buildingInfo = GetBuildingInfo(_buildingEnum);
	WorldTile2 size = buildingInfo.size;
	_area = BuildingArea(_mouseOnTile, size, _faceDirection);

	bool useNormalPlacement = true;
	bool isAllRed = false;

	FVector meshLocation = MapUtil::DisplayLocation(cameraAtom, _mouseOnTile.worldAtom2());

	_gridGuide->SetWorldLocation(meshLocation);
	_radiusDecal->SetWorldLocation(meshLocation);
	_gameInterface->SetOverlayTile(_mouseOnTile);


	/*
	 * Bad appeal only show red radius upon click
	 */
	if (IsIndustryOrMine(_buildingEnum))
	{
		const std::vector<int32>& houseIds = simulation.buildingIds(_gameInterface->playerId(), CardEnum::House);
		bool hasHouseAffectedByBadAppeal = false;
		for (int32 houseId : houseIds) {
			if (simulation.building(houseId).DistanceTo(_mouseOnTile) < BadAppealRadius) {
				hasHouseAffectedByBadAppeal = true;
				break;
			}
		}
		_radiusDecal->SetVisibility(hasHouseAffectedByBadAppeal);
	}

	/*
	 * Card Area effect such as FireStarter etc.
	 */
	if (!IsBuildingCard(_buildingEnum))
	{
		PUN_CHECK(IsAreaSpell(_buildingEnum));
		
		_area = TileArea(_mouseOnTile, AreaSpellRadius(_buildingEnum));
		_area.ExecuteOnArea_WorldTile2([&](WorldTile2 location) 
		{
			if (location.isValid())
			{
				if (WorldAtom2::DistanceLessThan(_mouseOnTile, location, AreaSpellRadius(_buildingEnum)))
				{
					int32 buildingId = simulation.buildingIdAtTileSafe(location);

					if (_buildingEnum == CardEnum::FireStarter) {
						// Casting on townhall requires additional influence
						if (buildingId != -1) {
							Building& building = simulation.building(buildingId);
							int32_t targetPlayerId = building.playerId();

							if (building.isEnum(CardEnum::Townhall)) {
								//SetInstruction(PlacementInstructionEnum::FireOnTownhall, true);
								_placementGrid.SpawnGrid(PlacementGridEnum::Red, cameraAtom, location);
							}
							else {
								_placementGrid.SpawnGrid(PlacementGridEnum::Green, cameraAtom, location);
							}
						}
						else {
							_placementGrid.SpawnGrid(PlacementGridEnum::Gray, cameraAtom, location);
						}
					}
					else if (_buildingEnum == CardEnum::Steal ||
						_buildingEnum == CardEnum::Snatch ||
						_buildingEnum == CardEnum::Kidnap ||
						_buildingEnum == CardEnum::SharingIsCaring)
					{
						if (buildingId != -1) {
							Building& building = simulation.building(buildingId);
							int32 targetPlayerId = building.playerId();

							if (building.isEnum(CardEnum::Townhall) && targetPlayerId != playerId) {
								_placementGrid.SpawnGrid(PlacementGridEnum::Green, cameraAtom, location);
								return;
							}
						}

						SetInstruction(PlacementInstructionEnum::TownhallTarget, true);

						_placementGrid.SpawnGrid(PlacementGridEnum::Red, cameraAtom, location);
						return;
					}
					else if (_buildingEnum == CardEnum::InstantBuild)
					{
						if (buildingId != -1) {
							Building& building = simulation.building(buildingId);

							if (!building.isConstructed()) {
								int32 cost = building.buildingInfo().constructionCostAsMoney() * 3;

								if (cost < simulation.money(playerId)) {
									//_buildingSpecificWarning = "Pay " + to_string(cost) + " <img id=\"Coin\"/> to instantly build this.";
									_placementGrid.SpawnGrid(PlacementGridEnum::Green, cameraAtom, location);
								}
								else {
									//_buildingSpecificWarning = "<Red>Not enough money to instant build (Require " + to_string(cost) + " </><img id=\"Coin\"/><Red>)</>";
									_placementGrid.SpawnGrid(PlacementGridEnum::Red, cameraAtom, location);
								}
								return;
							}
						}
						//_buildingSpecificWarning = "<Red>Must be used on a building under construction</>";
						_placementGrid.SpawnGrid(PlacementGridEnum::Red, cameraAtom, location);
						return;
					}
					/*
					 * Skills
					 */
					else if (_buildingEnum == CardEnum::SpeedBoost)
					{
						if (buildingId != -1)
						{
							Building& building = simulation.building(buildingId);


							// Show the building that has mouse over it.
							if (building.isConstructed()) {
								_gameInterface->ShowBuildingMesh(building, 2);
							}
							else {
								_gameInterface->ShowDecal(building.area(), _assetLoader->M_ConstructionHighlightDecal);
							}

							if (building.playerId() == -1) // Shouldn't be able to boost non-player buildings
							{
								SetInstruction(PlacementInstructionEnum::NotThisBuildingTarget, true);
							}
							else
							{
								bool isConstructed = building.isConstructed();
								CardEnum buildingEnum = building.buildingEnum();

								//if (building.isEnum(CardEnum::Townhall) ||
								//	building.isEnum(CardEnum::StorageYard) ||
								//	building.isEnum(CardEnum::Farm) ||
								//	IsDecorativeBuilding(buildingEnum) ||
								//	(IsHouse(buildingEnum) && isConstructed) ||
								//	(IsRanch(buildingEnum) && isConstructed))
								if (!CanGetSpeedBoosted(buildingEnum, isConstructed))
								{
									_networkInterface->SetCursor("Slate/MouseCursorSkillInvalid");
									SetInstruction(PlacementInstructionEnum::NotThisBuildingTarget, true);
									_forceCannotPlace = true;
									//_placementGrid.SpawnGrid(PlacementGridEnum::Red, cameraAtom, location);
									return;
								}

								_networkInterface->SetCursor("Slate/MouseCursorSkill");
								//_placementGrid.SpawnGrid(PlacementGridEnum::Green, cameraAtom, location);
							}
							return;
						}

						_networkInterface->SetCursor("Slate/MouseCursorSkillInvalid");
						SetInstruction(PlacementInstructionEnum::YourBuildingTarget, true);
						//_placementGrid.SpawnGrid(PlacementGridEnum::Red, cameraAtom, location);

						return;
					}
				}
			}
		});

		_canPlace = _placementGrid.IsDisplayCountZero(PlacementGridEnum::Red);

		if (_forceCannotPlace) {
			_forceCannotPlace = false;
			_canPlace = false;
		}
		
		_faceDirection = Direction::S;

		_placementGrid.AfterAdd();
		return;
	}
		
	/*
	 * Placement grids
	 */

	if (useNormalPlacement)
	{
		//// Building Mesh
		//meshLocation = MapUtil::DisplayLocation(cameraAtom, _mouseOnTile.worldAtom2());

		// Dock Grid
		if (_buildingEnum == CardEnum::Fisher ||
			_buildingEnum == CardEnum::TradingPort)
		{
			auto extraInfoPair = DockPlacementExtraInfo(_buildingEnum);
			int32 indexLandEnd = extraInfoPair.first;
			int32 minWaterCount = extraInfoPair.second;
			
			// Need to face water with overlapping at least 5 water tiles 
			int32 waterCount = 0;

			_area.ExecuteOnArea_WorldTile2([&](WorldTile2 tile) {
				if (GameMap::IsInGrid(tile.x, tile.y) && simulation.IsTileBuildableForPlayer(tile, playerId))
				{
					int steps = GameMap::GetFacingStep(_faceDirection, _area, tile);
					if (steps <= indexLandEnd) { // 0,1
						bool isGreen = IsPlayerBuildable(tile);
						_placementGrid.SpawnGrid(isGreen ? PlacementGridEnum::Green : PlacementGridEnum::Red, cameraAtom, tile);
					}
					else {
						if (simulation.IsWater(tile)) {
							waterCount++;
						}
					}
				}
			});

			// When there isn't enough water tiles, make the part facing water red...
			// Otherwise make it green on water, and gray on non-water
			_area.ExecuteOnArea_WorldTile2([&](WorldTile2 tile) {
				int steps = GameMap::GetFacingStep(_faceDirection, _area, tile);
				if (steps > indexLandEnd) {
					if (waterCount < minWaterCount) {
						_placementGrid.SpawnGrid(PlacementGridEnum::Red, cameraAtom, tile);

						SetInstruction(PlacementInstructionEnum::Dock, true);
					}
					else {
						if (simulation.tileHasBuilding(tile)) { // Maybe bridge
							_placementGrid.SpawnGrid(PlacementGridEnum::Red, cameraAtom, tile);
						}
						else {
							bool isGreen = simulation.IsWater(tile);
							_placementGrid.SpawnGrid(isGreen ? PlacementGridEnum::Green : PlacementGridEnum::Gray, cameraAtom, tile);
						}
					}
				}
			});
		}
		// Mine grid
		else if (IsMountainMine(_buildingEnum))
		{
			// Need to face mountain with overlapping at least 5 mountain tiles 
			int32 mountainCount = 0;

			GeoresourceEnum georesourceEnum = GeoresourceEnum::None;
			switch (_buildingEnum)
			{
			case CardEnum::CoalMine: georesourceEnum = GeoresourceEnum::CoalOre; break;
			case CardEnum::IronMine: georesourceEnum = GeoresourceEnum::IronOre; break;
			case CardEnum::GoldMine: georesourceEnum = GeoresourceEnum::GoldOre; break;
			case CardEnum::GemstoneMine: georesourceEnum = GeoresourceEnum::Gemstone; break;
			default: break;
			}

			GeoresourceSystem& georesourceSystem = simulation.georesourceSystem();
			auto isCorrectResource = [&](WorldTile2 tile)
			{
				// FastBuild doesn't need correct resource
				if (SimSettings::IsOn("CheatFastBuild")) {
					return true;
				}
				return _buildingEnum == CardEnum::Quarry || 
						georesourceSystem.georesourceNode(simulation.GetProvinceIdClean(tile)).info().georesourceEnum == georesourceEnum;
			};

			// First loop for
			// - (steps <= 1)
			// - count georesource
			_area.ExecuteOnArea_WorldTile2([&](WorldTile2 tile) {
				if (GameMap::IsInGrid(tile.x, tile.y) && simulation.IsTileBuildableForPlayer(tile, playerId))
				{
					int steps = GameMap::GetFacingStep(_faceDirection, _area, tile);
					if (steps <= 1) { // 0,1
						bool isGreen = IsPlayerBuildable(tile);
						_placementGrid.SpawnGrid(isGreen ? PlacementGridEnum::Green : PlacementGridEnum::Red, cameraAtom, tile);
					}
					else { // 2,3,4
						if (simulation.IsMountain(tile)) {
							mountainCount++;
						}
					}
				}
			});

			// When there isn't enough mountain tiles, make the part facing mountain red...
			// Otherwise make it green on mountain, and gray on non-mountain
			_area.ExecuteOnArea_WorldTile2([&](WorldTile2 tile) {
				int steps = GameMap::GetFacingStep(_faceDirection, _area, tile);
				if (steps > 1) {
					if (!isCorrectResource(tile)) 
					{
						GeoresourceInfo georesourceInfo = GetGeoresourceInfo(georesourceEnum);

						SetInstruction(PlacementInstructionEnum::NeedGeoresource, true, static_cast<int32>(georesourceEnum));

						_placementGrid.SpawnGrid(PlacementGridEnum::Red, cameraAtom, tile);
					}
					else if (mountainCount < 5) {
						SetInstruction(PlacementInstructionEnum::MountainMine, true);

						_placementGrid.SpawnGrid(PlacementGridEnum::Red, cameraAtom, tile);
					}
					else {
						bool isGreen = simulation.IsMountain(tile);
						_placementGrid.SpawnGrid(isGreen ? PlacementGridEnum::Green : PlacementGridEnum::Gray, cameraAtom, tile);
					}
				}
			});
		}
		// Claypit/Irrigation Reservoir grid
		else if (_buildingEnum == CardEnum::ClayPit ||
				_buildingEnum == CardEnum::IrrigationReservoir)
		{
			_area.ExecuteOnArea_WorldTile2([&](WorldTile2 tile) {
				if (IsPlayerBuildable(tile)) {
					if (terrainGenerator.riverFraction(tile) > GetRiverFractionPercentThreshold(_buildingEnum) / 100.0f) {
						_placementGrid.SpawnGrid(PlacementGridEnum::Green, cameraAtom, tile);
						return;
					}
					SetInstruction(PlacementInstructionEnum::MustBeNearRiver, true);
				}
				_placementGrid.SpawnGrid(PlacementGridEnum::Red, cameraAtom, tile);
			});
		}
		// Logistics Office
		else if (_buildingEnum == CardEnum::ShippingDepot)
		{
			vector<int32> storageIds = simulation.GetBuildingsWithinRadiusMultiple(_mouseOnTile, ShippingDepot::Radius, playerId, StorageEnums);
			for (int32 storageId : storageIds)
			{
				Building& buildingScope = simulation.buildingChecked(storageId);
				FVector displayLocationScope = gameInterface->DisplayLocationTrueCenter(buildingScope);
				gameInterface->ShowDeliveryArrow(displayLocationScope, meshLocation, true);
			}

			_area.ExecuteOnArea_WorldTile2([&](WorldTile2 location)
			{
				bool isGreen = IsPlayerBuildable(location) && !isAllRed;
				_placementGrid.SpawnGrid(isGreen ? PlacementGridEnum::Green : PlacementGridEnum::Red, cameraAtom, location);
			});

			SetInstruction(PlacementInstructionEnum::LogisticsOffice, true);
		}
		
		// Colony Grid
		else if (_buildingEnum == CardEnum::Colony)
		{
			auto& sim = _gameInterface->simulation();
			_area.ExecuteOnArea_WorldTile2([&](WorldTile2 tile) {
				if (IsPlayerBuildable(tile)) {
					if (sim.georesource(sim.GetProvinceIdClean(tile)).georesourceEnum != GeoresourceEnum::None) {
						_placementGrid.SpawnGrid(PlacementGridEnum::Green, cameraAtom, tile);
					} else {
						_placementGrid.SpawnGrid(PlacementGridEnum::Red, cameraAtom, tile);
						SetInstruction(PlacementInstructionEnum::ColonyNoGeoresource, true);
					}
				} else {
					_placementGrid.SpawnGrid(PlacementGridEnum::Red, cameraAtom, tile);
				}
			});
		}
		// Townhall Initial
		else if (_buildingEnum == CardEnum::Townhall)
		{
			auto tryPlaceBuilding = [&](WorldTile2 tile)
			{
				bool isGreen = IsPlayerBuildable(tile) && !isAllRed;
				_placementGrid.SpawnGrid(isGreen ? PlacementGridEnum::Green : PlacementGridEnum::Red, cameraAtom, tile);
			};

			auto tryPlaceRoad = [&](WorldTile2 tile, PlacementGridEnum redEnum, PlacementGridEnum greenEnum, PlacementGridEnum yellowEnum, Direction direction)
			{
				PlacementGridEnum gridEnum = redEnum;
				if (simulation.overlaySystem().IsRoad(tile)) {
					gridEnum = greenEnum;
				}
				else if (gameInterface->IsPlayerFrontBuildable(tile)) {
					gridEnum = yellowEnum;
				}

				_placementGrid.SpawnGrid(gridEnum, cameraAtom, tile, direction);

				if (simulation.tileOwner(tile) != playerId) {
					SetInstruction(PlacementInstructionEnum::OutsideTerritory, true);
				}
			};

			// Borders
			TileArea topRoad(_area.maxX + 1, _area.minY, _area.maxX + 1, _area.maxY);
			topRoad.ExecuteOnArea_WorldTile2([&](WorldTile2 tile) {
				tryPlaceRoad(tile, PlacementGridEnum::ArrowRed, PlacementGridEnum::ArrowGreen, PlacementGridEnum::ArrowYellow, Direction::S);
			});
			TileArea bottomRoad(_area.minX - 1, _area.minY, _area.minX - 1, _area.maxY);
			bottomRoad.ExecuteOnArea_WorldTile2([&](WorldTile2 tile) {
				tryPlaceRoad(tile, PlacementGridEnum::ArrowRed, PlacementGridEnum::ArrowGreen, PlacementGridEnum::ArrowYellow, Direction::S);
			});
			TileArea leftRoad(_area.minX, _area.minY - 1, _area.maxX, _area.minY - 1);
			leftRoad.ExecuteOnArea_WorldTile2([&](WorldTile2 tile) {
				tryPlaceRoad(tile, PlacementGridEnum::ArrowRed, PlacementGridEnum::ArrowGreen, PlacementGridEnum::ArrowYellow, Direction::E);
			});
			TileArea rightRoad(_area.minX, _area.maxY + 1, _area.maxX, _area.maxY + 1);
			rightRoad.ExecuteOnArea_WorldTile2([&](WorldTile2 tile) {
				tryPlaceRoad(tile, PlacementGridEnum::ArrowRed, PlacementGridEnum::ArrowGreen, PlacementGridEnum::ArrowYellow, Direction::E);
			});

			// Corners
			tryPlaceRoad(WorldTile2(_area.minX - 1, _area.minY - 1), PlacementGridEnum::ArrowRed, PlacementGridEnum::ArrowGreen, PlacementGridEnum::ArrowYellow, _faceDirection);
			tryPlaceRoad(WorldTile2(_area.minX - 1, _area.maxY + 1), PlacementGridEnum::ArrowRed, PlacementGridEnum::ArrowGreen, PlacementGridEnum::ArrowYellow, _faceDirection);
			tryPlaceRoad(WorldTile2(_area.maxX + 1, _area.minY - 1), PlacementGridEnum::ArrowRed, PlacementGridEnum::ArrowGreen, PlacementGridEnum::ArrowYellow, _faceDirection);
			tryPlaceRoad(WorldTile2(_area.maxX + 1, _area.maxY + 1), PlacementGridEnum::ArrowRed, PlacementGridEnum::ArrowGreen, PlacementGridEnum::ArrowYellow, _faceDirection);


			// Townhall
			_area.ExecuteOnArea_WorldTile2([&](WorldTile2 tile) {
				tryPlaceBuilding(tile);
			});
			
			// Storage
			WorldTile2 storageCenter1 = WorldTile2::EvenSizeRotationCenterShift(_area.centerTile(), _faceDirection) + WorldTile2::RotateTileVector(Storage1ShiftTileVec, _faceDirection);
			WorldTile2 storageCenter2 = storageCenter1 + WorldTile2::RotateTileVector(InitialStorage2Shift, _faceDirection);
			WorldTile2 storageCenter3 = storageCenter1 - WorldTile2::RotateTileVector(InitialStorage2Shift, _faceDirection);

			TileArea storageArea1 = BuildingArea(storageCenter1, InitialStorageTileSize, RotateDirection(Direction::E, _faceDirection));
			TileArea storageArea2 = BuildingArea(storageCenter2, InitialStorageTileSize, RotateDirection(Direction::E, _faceDirection));
			TileArea storageArea3 = BuildingArea(storageCenter3, InitialStorageTileSize, RotateDirection(Direction::E, _faceDirection));

			storageArea1.ExecuteOnArea_WorldTile2([&](WorldTile2 tile) {
				tryPlaceBuilding(tile);
			});
			storageArea2.ExecuteOnArea_WorldTile2([&](WorldTile2 tile) {
				tryPlaceBuilding(tile);
			});
			storageArea3.ExecuteOnArea_WorldTile2([&](WorldTile2 tile) {
				tryPlaceBuilding(tile);
			});
		}
		// Road Overlap Building
		else if (IsRoadOverlapBuilding(_buildingEnum))
		{
			_area.ExecuteOnArea_WorldTile2([&](WorldTile2 location) {
				bool isGreen = gameInterface->IsPlayerFrontBuildable(location) && !isAllRed;

				_placementGrid.SpawnGrid(isGreen ? PlacementGridEnum::Green : PlacementGridEnum::Red, cameraAtom, location);
			});
		}
		else if (IsRanch(_buildingEnum))
		{
		_area.ExecuteOnArea_WorldTile2([&](WorldTile2 tile) {
			if (IsPlayerBuildable(tile)) {
				if (simulation.GetBiomeEnum(tile) == BiomeEnum::Desert) {
					SetInstruction(PlacementInstructionEnum::FarmAndRanch, true);
					_placementGrid.SpawnGrid(PlacementGridEnum::Red, cameraAtom, tile);
					return;
				}
				_placementGrid.SpawnGrid(PlacementGridEnum::Green, cameraAtom, tile);
				return;
			}
			_placementGrid.SpawnGrid(PlacementGridEnum::Red, cameraAtom, tile);
		});
		}
		//else if (_buildingEnum == CardEnum::Farm)
		//{
		//	_area.ExecuteOnArea_WorldTile2([&](WorldTile2 tile) {
		//		if (IsPlayerBuildable(tile)) {
		//			if (simulation.GetFertilityPercent(tile) < 20) {
		//				SetInstruction(PlacementInstructionEnum::FarmAndRanch, true);
		//				_placementGrid.SpawnGrid(PlacementGridEnum::Red, cameraAtom, tile);
		//				return;
		//			}

		//			// Any seed that can be planted here?
		//			std::vector<SeedInfo> seedsOwned = simulation.resourceSystem(_gameInterface->playerId()).seedsPlantOwned();
		//			GeoresourceEnum georesourceEnum = simulation.georesource(tile.region()).georesourceEnum;
		//			bool hasValidSeed = false;
		//			for (SeedInfo seed : seedsOwned)
		//			{
		//				if (IsCommonSeedCard(seed.cardEnum)) {
		//					hasValidSeed = true;
		//					break;
		//				}
		//				if (IsSpecialSeedCard(seed.cardEnum) && 
		//					seed.georesourceEnum == georesourceEnum) 
		//				{
		//					hasValidSeed = true;
		//					break;
		//				}
		//			}
		//			
		//			if (!hasValidSeed) {
		//				SetInstruction(PlacementInstructionEnum::FarmNoValidSeedForRegion, true);
		//				_placementGrid.SpawnGrid(PlacementGridEnum::Red, cameraAtom, tile);
		//				return;
		//			}
		//			_placementGrid.SpawnGrid(PlacementGridEnum::Green, cameraAtom, tile);
		//			return;
		//		}
		//		_placementGrid.SpawnGrid(PlacementGridEnum::Red, cameraAtom, tile);
		//	});
		//}
		/*
		 * Foreign buildings
		 */
		else if (_buildingEnum == CardEnum::HumanitarianAidCamp)
		{
			_area.ExecuteOnArea_WorldTile2([&](WorldTile2 tile)
			{
				bool isForeign = simulation.tileOwner(tile) != playerId;
				if (!isForeign) {
					SetInstruction(PlacementInstructionEnum::ForeignBuilding, true);
				}
				
				bool isGreen = isForeign && GameMap::IsInGrid(tile) && simulation.IsBuildable(tile);
				_placementGrid.SpawnGrid(isGreen ? PlacementGridEnum::Green : PlacementGridEnum::Red, cameraAtom, tile);
			});
		}
		// Grid
		else
		{
			_area.ExecuteOnArea_WorldTile2([&](WorldTile2 location)
			{
				bool isGreen = IsPlayerBuildable(location) && !isAllRed;
				_placementGrid.SpawnGrid(isGreen ? PlacementGridEnum::Green : PlacementGridEnum::Red, cameraAtom, location);
			});
		}

		/*
		 * Front grid
		 */
		if (_buildingEnum == CardEnum::HumanitarianAidCamp)
		{
			TileArea frontArea = _area.GetFrontArea(_faceDirection);
			
			frontArea.ExecuteOnArea_WorldTile2([&](WorldTile2 tile)
			{
				PlacementGridEnum gridEnum = PlacementGridEnum::ArrowRed;
				if (gameInterface->simulation().overlaySystem().IsRoad(tile)) {
					gridEnum = PlacementGridEnum::ArrowGreen;
				}
				else if (simulation.IsFrontBuildable(tile)) {
					gridEnum = PlacementGridEnum::ArrowYellow;
				}

				_placementGrid.SpawnGrid(gridEnum, cameraAtom, tile, _faceDirection);
			});
		}
		else if (
			//_buildingEnum != BuildingEnum::Fence &&
			//_buildingEnum != BuildingEnum::Bridge &&
			!IsRoadOverlapBuilding(_buildingEnum) &&
			_buildingEnum != CardEnum::Townhall &&
			_buildingEnum != CardEnum::Farm &&
			_buildingEnum != CardEnum::StorageYard &&
			!IsDecorativeBuilding(_buildingEnum))
		{
			TileArea frontArea = _area.GetFrontArea(_faceDirection);

			frontArea.ExecuteOnArea_WorldTile2([&](WorldTile2 tile) 
			{
				PlacementGridEnum gridEnum = PlacementGridEnum::ArrowRed;
				if (simulation.overlaySystem().IsRoad(tile)) {
					gridEnum = PlacementGridEnum::ArrowGreen;
				} else if (gameInterface->IsPlayerFrontBuildable(tile)) {
					gridEnum = PlacementGridEnum::ArrowYellow;
				}
				
				_placementGrid.SpawnGrid(gridEnum, cameraAtom, tile, _faceDirection);

				//if (!regionSystem.IsOwnedByPlayer(tile.region(), playerId)) {
				if (simulation.tileOwner(tile) != playerId) {
					SetInstruction(PlacementInstructionEnum::OutsideTerritory, true);
				}
			});
		}
	}

	//if (GEngine) GEngine->AddOnScreenDebugMessage(-1, 10.0f, FColor::White, FString::Printf(TEXT("_instanceCount: %d , displayCount: %d"), _instanceCount, _instancedMesh->GetInstanceCount()));

	_placementGrid.AfterAdd();

	_canPlace = _placementGrid.IsDisplayCountZero(PlacementGridEnum::Red) && 
				_placementGrid.IsDisplayCountZero(PlacementGridEnum::ArrowRed);

	// If cannot place, check if this is caused by being outside territory.
	if (!_canPlace) {
		_area.ExecuteOnArea_WorldTile2([&](WorldTile2 tile) {
			//if (!regionSystem.IsOwnedByPlayer(tile.region(), playerId)) {
			if (simulation.tileOwner(tile) != playerId) {
				SetInstruction(PlacementInstructionEnum::OutsideTerritory, true);
			}
		});
	}


	// Refresh Building Mesh
	if (IsBuildingCard(_buildingEnum))
	{
		_buildingLocation = meshLocation;

		// Special case farm
		if (_buildingEnum == CardEnum::Farm) {
			//_gameInterface->ShowDecal(_area, _assetLoader->FarmDecalMaterial);
			return;
		}

		// Special case storage yard
		if (_buildingEnum == CardEnum::StorageYard) {
			return;
		}

		std::vector<ModuleTransform> modules = _gameInterface->displayInfo().GetDisplayModules(_buildingEnum, 0).transforms;
		_gameInterface->ShowBuildingMesh(_mouseOnTile, _faceDirection, modules, 0);
	}
}


void ABuildingPlacementSystem::NetworkDragPlace(IGameNetworkInterface* networkInterface, PlacementType placementType)
{
	auto& sim = _gameInterface->simulation();
	
	/*
	 * Farm
	 */
	if (_placementType == PlacementType::BuildingDrag)
	{
		if (_canPlace)
		{
			_gameInterface->Spawn2DSound("UI", "PlaceBuilding");
			
			if (_buildingEnum == CardEnum::Farm)
			{
				auto command = make_shared<FPlaceBuilding>();
				command->buildingEnum = buildingEnumInt();
				command->buildingLevel = _buildingLvl;
				command->area = _area;
				command->center = _area.centerTile();
				command->faceDirection = static_cast<uint8>(Direction::S);
				command->useBoughtCard = _useBoughtCard;
				command->useWildCard = _useWildCard;

				networkInterface->SendNetworkCommand(command);

				{
					// Show construction right away (Fake responsiveness)

					_lastNetworkPlacementTime = UGameplayStatics::GetTimeSeconds(GetWorld());
					_delayFillerEnum = _buildingEnum;
					_delayFillerTile = _area.centerTile();
					_delayFillerFaceDirection = Direction::S;
					_delayFillerArea = _area;
				}

				// Multiple placement for permanent buildings when holding shift
				if (_gameInterface->isShiftDown()) {
					return;
				}

				CancelPlacement();
			}
			else if (_buildingEnum == CardEnum::StorageYard)
			{
				auto command = make_shared<FPlaceBuilding>();
				command->buildingEnum = buildingEnumInt();
				command->buildingLevel = _buildingLvl;
				command->area = _area3; // Storage trimmed area
				//command->area2 = _area2; // TODO: Ranch use this
				command->center = _area3.centerTile();
				command->faceDirection = static_cast<uint8>(_faceDirection);
				command->useBoughtCard = _useBoughtCard;
				command->useWildCard = _useWildCard;

				networkInterface->SendNetworkCommand(command);

				{
					// Show construction right away (Fake responsiveness)

					_lastNetworkPlacementTime = UGameplayStatics::GetTimeSeconds(GetWorld());
					_delayFillerEnum = _buildingEnum;
					_delayFillerTile = _area3.centerTile();
					_delayFillerFaceDirection = _faceDirection;
					_delayFillerArea = _area3;
				}

				if (_gameInterface->isShiftDown()) {
					_placementType = PlacementType::Building;
					// TODO: use this for ranch
					//CancelPlacement();
					//StartBuildingPlacement(CardEnum::StorageYard, 0, false, CardEnum::None);
					return;
				}

				CancelPlacement();
			}
			// TODO: use old storage code for ranch...
			//else if (_buildingEnum == CardEnum::StorageYard)
			//{
			//	auto command = make_shared<FPlaceBuildingParameters>();
			//	command->buildingEnum = buildingEnumInt();
			//	command->buildingLevel = _buildingLvl;
			//	command->area = _area3; // Storage trimmed area
			//	command->area2 = _area2;
			//	command->center = _area3.centerTile();
			//	command->faceDirection = static_cast<uint8>(_faceDirection);
			//	command->useBoughtCard = _useBoughtCard;
			//	command->useWildCard = _useWildCard;

			//	networkInterface->SendNetworkCommand(command);

			//	{
			//		// Show construction right away (Fake responsiveness)

			//		_lastNetworkPlacementTime = UGameplayStatics::GetTimeSeconds(GetWorld());
			//		_delayFillerEnum = _buildingEnum;
			//		_delayFillerTile = _area3.centerTile();
			//		_delayFillerFaceDirection = _faceDirection;
			//		_delayFillerArea = _area3;
			//	}

			//	if (_gameInterface->isShiftDown()) {
			//		CancelPlacement();
			//		StartBuildingPlacement(CardEnum::StorageYard, 0, false, CardEnum::None);
			//		return;
			//	}

			//	CancelPlacement();
			//}
			else
			{
				UE_DEBUG_BREAK();
			}

			
			return;
		}
		
		_gameInterface->Spawn2DSound("UI", "PopupCannot");
		return;
	}
	
	/*
	 * Bridge
	 */
	if (_placementType == PlacementType::Bridge ||
		_placementType == PlacementType::Tunnel)
	{
		// Split into bridges and send separate commands
		int32_t lastIndex = std::max(_area.sizeX(), _area.sizeY()) - 1;

		// bridge must be at least size 3
		if (lastIndex >= 2) 
		{
			bool lastTileWasWater = false;
			WorldTile2 firstBridgeWaterTile = WorldTile2::Invalid;
			WorldTile2 lastBridgeWaterTile = WorldTile2::Invalid;

			auto& simulation = _gameInterface->simulation();
			
			_area.ExecuteOnArea_WorldTile2([&](WorldTile2 tile) 
			{
				bool tileIsSuitable = (_placementType == PlacementType::Bridge) ? simulation.IsWater(tile) : simulation.IsMountain(tile);

				if (!lastTileWasWater && tileIsSuitable) {
					PUN_CHECK(!firstBridgeWaterTile.isValid());
					firstBridgeWaterTile = tile;
				}

				if (tileIsSuitable) {
					lastBridgeWaterTile = tile;
				}
				
				// End of bridge issue PlaceBuilding command
				if (lastTileWasWater && !tileIsSuitable) {
					PUN_CHECK(firstBridgeWaterTile.isValid());
					
					int32 minX = min(firstBridgeWaterTile.x, lastBridgeWaterTile.x);
					int32 minY = min(firstBridgeWaterTile.y, lastBridgeWaterTile.y);
					int32 maxX = max(firstBridgeWaterTile.x, lastBridgeWaterTile.x);
					int32 maxY = max(firstBridgeWaterTile.y, lastBridgeWaterTile.y);
					TileArea bridgeArea(minX, minY, maxX, maxY);
					
					auto command = make_shared<FPlaceBuilding>();
					command->buildingEnum = static_cast<uint8>(_placementType == PlacementType::Bridge ? CardEnum::Bridge : CardEnum::Tunnel);
					command->buildingLevel = _buildingLvl;
					command->area = bridgeArea;
					command->center = bridgeArea.min();
					command->faceDirection = static_cast<uint8>(_faceDirection);
					command->useBoughtCard = false;

					networkInterface->SendNetworkCommand(command);

					_gameInterface->Spawn2DSound("UI", "PlaceBuilding");
					
					firstBridgeWaterTile = WorldTile2::Invalid;
				}

				lastTileWasWater = tileIsSuitable;
			});
		}

		if (!_gameInterface->isShiftDown()) {
			CancelPlacement();
			networkInterface->OnCancelPlacement();
		}
		_timesShifted++;
		return;
	}
	
	PUN_LOG("NetworkDragPlace: %d, %d, %d, %d", _area.minX, _area.minY, _area.maxX, _area.maxY);
	auto placeGatherCommand = make_shared<FPlaceDrag>();

	placeGatherCommand->area = _area;
	placeGatherCommand->area2 = _area2;
	placeGatherCommand->path = _roadPathTileIds;
	placeGatherCommand->placementType = static_cast<int8_t>(placementType);
	placeGatherCommand->harvestResourceEnum = _harvestResourceEnum;

	// Intercity Road, don't allow building if !canPlace or not enough money
	if (placementType == PlacementType::IntercityRoad) 
	{
		int32 playerId = _gameInterface->playerId();
		if (!_canPlace) {
			return;
		}
		
		int32 goldNeeded = 0;
		for (int32 roadTileId : _roadPathTileIds) {
			if (!_gameInterface->simulation().IsRoadTile(WorldTile2(roadTileId))) {
				goldNeeded += IntercityRoadTileCost;
			}
		}

		if (goldNeeded > 0 && goldNeeded > _gameInterface->simulation().money(playerId)) {
			_gameInterface->simulation().AddEventLog(playerId, "Not enough money.", true);
			return;
		}
	}

	// Demolish only after confirmation
	if (_placementType == PlacementType::Demolish) 
	{
		// Fort/Colony warn about card not being recoverable too
		bool hasFortOrColony = false;
		_area.ExecuteOnAreaWithExit_WorldTile2([&](WorldTile2 tile) {
			CardEnum buildingEnum = sim.buildingEnumAtTile(tile);
			if (buildingEnum == CardEnum::Fort ||
				buildingEnum == CardEnum::Colony) 
			{
				hasFortOrColony = true;
				return true;
			}
			return false;
		});
		
		if (hasFortOrColony) {
			_networkInterface->ShowConfirmationUI("Are you sure you want to demolish?\nFort and Colony Cards will not be recovered.", placeGatherCommand);
		} else {
			_networkInterface->ShowConfirmationUI("Are you sure you want to demolish?", placeGatherCommand);
		}

		CancelPlacement();
		networkInterface->OnCancelPlacement();
		
		_placementGridDelayed.SetActive(true); // still need _placementGrid active for Confirmation stage...
		return;
	}

	// Road/Gather after here
	networkInterface->SendNetworkCommand(placeGatherCommand);

	_gameInterface->Spawn2DSound("UI", "PlaceBuilding");

	if (_area.sizeX() > 1 || _area.sizeY() > 1) 
	{
		if (IsGatherPlacement(_placementType)) {
			_dragGatherSuccessCount++;
		}
		else if (_placementType == PlacementType::Demolish) {
			_dragDemolishSuccessCount++;
		}
	}

	if (_roadPathTileIds.Num() > 0 &&
		IsRoadPlacement(_placementType)) 
	{
		_dragRoadSuccessCount++;
	}

	if (!_gameInterface->isShiftDown() &&
		!IsRoadPlacement(_placementType))
	{
		CancelPlacement();
		networkInterface->OnCancelPlacement();
	}

	
	_timesShifted++;
}

void ABuildingPlacementSystem::NetworkTryPlaceBuilding(IGameNetworkInterface* networkInterface)
{
	PUN_LOG("NetworkTryPlaceBuilding");
	
	if (_placementType == PlacementType::Building) 
	{
		if (_canPlace)
		{
			auto& sim = _gameInterface->simulation();

			auto command = make_shared<FPlaceBuilding>();
			command->buildingEnum = buildingEnumInt();
			command->buildingLevel = _buildingLvl;
			command->area = _area;
			command->center = _mouseOnTile;
			command->faceDirection = static_cast<uint8_t>(_faceDirection);
			command->useBoughtCard = _useBoughtCard;
			command->useWildCard = _useWildCard;

			// Trailer Mode record house lvl
			command->buildingLevel = SimSettings::Get("CheatHouseLevel");

			//// Special cases
			//if (_buildingEnum == CardEnum::SpeedBoost)
			//{
			//	sim.buildingAtTile(_mouseOnTile);

			//	return;
			//}

			//_gameInterface->simulationCore()->DrawArea(_area, FLinearColor::Yellow, 0);

			networkInterface->SendNetworkCommand(command);

			{
				// Show construction right away (Fake responsiveness)

				_lastNetworkPlacementTime = UGameplayStatics::GetTimeSeconds(GetWorld());
				_delayFillerEnum = _buildingEnum;
				_delayFillerTile = _mouseOnTile;
				_delayFillerFaceDirection = _faceDirection;
				_delayFillerArea = _area;
			}

			_gameInterface->Spawn2DSound("UI", "PlaceBuilding");

			int32 playerId = networkInterface->playerId();

			// Multiple placement for permanent buildings when holding shift
			if (_gameInterface->isShiftDown()) 
			{
				if (sim.IsPermanentBuilding(playerId, _buildingEnum))
				{
					int32 money = sim.money(playerId);

					// Shift-Click only if there is enough money for buying 2 (current placement, and next placement)
					if (money >= 2 * sim.cardSystem(playerId).GetCardPrice(_buildingEnum)) {
						_timesShifted++;
						return;
					}
				}
				else if (_buildingEnum == CardEnum::SpeedBoost)
				{
					// Leader Skills
					// Shift-Click only if there is enough SP
					if (sim.playerOwned(playerId).GetSP() >= GetSkillManaCost(_buildingEnum)) {
						return;
					}
					//sim.AddPopupToFront(playerId(), "Not enough SP to use the leader skill.", ExclusiveUIEnum::None, "PopupCannot");
				}
			}

			CancelPlacement();
		}
		else
		{
			_gameInterface->Spawn2DSound("UI", "PopupCannot");
		}
	}

	else if (_placementType == PlacementType::DeliveryTarget)
	{
		if (_canPlace)
		{
			auto command = make_shared<FPlaceBuilding>();
			command->buildingIdToSetDelivery = _deliverySourceBuildingId;
			command->center = _mouseOnTile;

			networkInterface->SendNetworkCommand(command);

			// Select the source building
			_gameInterface->simulation().SetDescriptionUIState(DescriptionUIState(ObjectTypeEnum::Building, _deliverySourceBuildingId));
			
			CancelPlacement();
		}
		else {
			_gameInterface->Spawn2DSound("UI", "PopupCannot");
		}
	}
}
