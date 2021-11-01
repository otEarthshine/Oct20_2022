#pragma once

#include "UnrealEngine.h"
#include "GameFramework/Actor.h"

#include "Components/HierarchicalInstancedStaticMeshComponent.h"
#include "PunCity/Simulation/GameSimulationInfo.h"
#include "PunCity/AssetLoaderComponent.h"
#include "PunCity/GameManagerInterface.h"
#include "PunCity/GameNetworkInterface.h"
#include "BuildingMeshesComponent.h"
#include "Kismet/GameplayStatics.h"
#include "GameManager.h"

#include <functional>

struct MouseInfo
{
	bool isValid = true;
	FVector mouseLocation, mouseDirection;

	FVector mouseHitLocation() {
		// Get mouse hit (ground position)
		// 0.0f = mouseDirection.Z * t + mouseLocation.Z
		float t = -mouseLocation.Z / mouseDirection.Z;
		return mouseLocation + mouseDirection * t;
	}
};

// Manages the placement grid mesh
class PlacementGrid
{
public:
	int displayCount = 0;

	void Init(UMaterialInterface* material, UStaticMesh* mesh, UInstancedStaticMeshComponent* instancedMesh);
	void SetActive(bool active) {
		_placementMesh->TranslucencySortPriority = 999;
		_placementMesh->SetActive(active);
		_placementMesh->SetVisibility(active);
		//if (!active) _placementMesh->ClearInstances();
	}

	void BeforeAdd() { displayCount = 0; }
	void SpawnGrid(WorldAtom2 cameraAtom, WorldTile2 location, Direction direction);
	void AfterAdd();

	void Clear() { _placementMesh->ClearInstances(); }

private:
	UMaterialInstanceDynamic* _dynamicMaterial = nullptr;
	UInstancedStaticMeshComponent* _placementMesh = nullptr;
};

class PlacementGrids
{
public:
	void Init(UMaterialInterface* material, UStaticMesh* mesh, UInstancedStaticMeshComponent* instancedMesh) {
		_placementGrids.push_back(PlacementGrid());
		_placementGrids.back().Init(material, mesh, instancedMesh);
	}
	void SetActive(bool active) {
		for (PlacementGrid& grid : _placementGrids) {
			grid.SetActive(active);
		}
	}

	void BeforeAdd() {
		for (PlacementGrid& grid : _placementGrids) {
			grid.BeforeAdd();
		}
	}
	void SpawnGrid(PlacementGridEnum gridEnum, WorldAtom2 cameraAtom, WorldTile2 location, Direction direction = Direction::S) {
		_placementGrids[static_cast<int>(gridEnum)].SpawnGrid(cameraAtom, location, direction);
	}
	void AfterAdd() {
		for (PlacementGrid& grid : _placementGrids) {
			grid.AfterAdd();
		}
	}

	void Clear()
	{
		for (PlacementGrid& grid : _placementGrids) {
			grid.Clear();
		}
	}

	bool IsDisplayCountZero(PlacementGridEnum gridEnum) {
		return _placementGrids[static_cast<int>(gridEnum)].displayCount == 0;
	}

	int32 GetDisplayGridCount(PlacementGridEnum gridEnum) {
		return _placementGrids[static_cast<int>(gridEnum)].displayCount;
	}

private:
	std::vector<PlacementGrid> _placementGrids;
};


#include "BuildingPlacementSystem.generated.h"

UCLASS()
class ABuildingPlacementSystem : public AActor
{
	GENERATED_BODY()
	
public:	
	ABuildingPlacementSystem();

public:	
	void Init(UAssetLoaderComponent* assetLoader);

	void StartBuildingPlacement(CardStatus cardStatus, bool useBoughtCard, CardEnum useWildCard);
	void StartSetDeliveryTarget(int32 buildingId);
	void StartRevealSpyNest();
	
	void StartHarvestPlacement(bool isRemoving, ResourceEnum resourceEnum);
	void StartDemolish();
	void StartRoad(bool isStoneRoad, bool isIntercity);
	void StartIrrigationDitch();
	void StartFence();
	void StartBridge(bool isIntercity);
	void StartTunnel();

	void LeftClickDown(IGameNetworkInterface* networkInterface);
	void LeftClickUp(IGameNetworkInterface* networkInterface);

	void CancelPlacement();
	void RotatePlacement();
	void TickPlacement(AGameManager* gameInterface, IGameNetworkInterface* networkInterface, 
						MouseInfo mouseInfo, WorldAtom2 cameraAtom);

	// Determine instructions for display
	PlacementInfo GetPlacementInfo();

	PlacementType placementState() { return _placementType; }
	CardEnum placementBuildingEnum() { return _buildingEnum; }

	//int32_t GetCardHandIndexBeingPlaced() {
	//	// After building is placed, info needs to travel in network before coming back to update cardIndexInUse
	//	// Allow 2s for network travel.. During this time, give old _cardHandIndex if asked (so UI won't display the card)
	//	bool justPlaced = _lastPlacedTime > 0 && UGameplayStatics::GetRealTimeSeconds(GetWorld()) - _lastPlacedTime < 2.0f;
	//	if (_placementType == PlacementType::None && !justPlaced) {
	//		return -1;
	//	}
	//	return _cardHandIndex;
	//}

	CardEnum GetBuildingEnumBeingPlaced() {
		if (_placementType == PlacementType::Building) {
			return _buildingEnum;
		}
		if (_lastNetworkPlacementTime > 0) {
			return _delayFillerEnum;
		}
		return CardEnum::None;
	}

	TileArea GetDemolishHighlightArea() {
		return _demolishHighlightArea;
	}


	void RefreshTreesNearMouseOnTile()
	{
		_mouseOnTile.region().ExecuteOnNearbyRegions([&](WorldRegion2 region) {
			_gameInterface->simulation().SetNeedDisplayUpdate(DisplayClusterEnum::Trees, region.regionId(), true);
		});
	}
	
	
public:
	PlacementGrids _placementGrid;
	PlacementGrids _placementGridDelayed;
	
	int32 _timesRotated = 0;
	int32 _timesShifted = 0;

	std::vector<PlacementInstructionInfo> _placementInstructions; // Not using bool because you cannot get ref from vector
	void SetInstruction(PlacementInstructionEnum instructionEnum, bool shouldShow, int32 intVar1 = -1, int32 intVar2 = -1) {
		_placementInstructions[static_cast<int>(instructionEnum)].shouldShow = shouldShow;
		_placementInstructions[static_cast<int>(instructionEnum)].intVar1 = intVar1;
		_placementInstructions[static_cast<int>(instructionEnum)].intVar2 = intVar2;
		_placementInstructions[static_cast<int>(instructionEnum)].instruction = FText();
	}
	void SetInstruction(PlacementInstructionEnum instructionEnum, bool shouldShow, FText instruction) {
		_placementInstructions[static_cast<int>(instructionEnum)] = { shouldShow, -1, -1, instruction };
	}

	void ClearInstructions() {
		for (auto& placementInstruction : _placementInstructions) {
			placementInstruction.shouldShow = false;
			placementInstruction.intVar1 = -1;
			placementInstruction.intVar2 = -1;
		}
	}

private:
	UInstancedStaticMeshComponent* CreateInstancedStaticMeshComponent(FString name);

	uint16 buildingEnumInt() { return static_cast<uint16>(_buildingEnum); }

	void ShowRadius(int radius, OverlayType overlayType, bool isRed = false);

	void StartDrag();

	void TickLineDrag(WorldAtom2 cameraAtom, std::function<bool(WorldTile2)> isBuildableFunc, bool checkTerritory, bool hasDiagonal = true);
	void TickAreaDrag(WorldAtom2 cameraAtom, std::function<PlacementGridEnum(WorldTile2)> getPlacementGridEnum);
	void FinishDrag(IGameNetworkInterface* networkInterface);

	void CalculateDragArea();
	void CalculateRoadLineDrag(std::function<bool(WorldTile2)> isBuildableFunc, bool hasDiagonal);
	void CalculateLineDrag();
	void CalculateBridgeLineDrag();

	void NetworkDragPlace(IGameNetworkInterface* networkInterface, PlacementType placementType);
	void NetworkTryPlaceBuilding(IGameNetworkInterface* networkInterface);
	

	bool IsPlayerBuildable(WorldTile2 tile) {
		if (!_gameInterface->simulation().IsPlayerBuildable(tile, _gameInterface->playerId())) {
			return false;
		}
		if (justPlacedBuilding() && _delayFillerArea.HasTile(tile)) {
			return false;
		}
		return true;
	}

	bool IsPlayerFrontBuildable(WorldTile2 tile)
	{
		if (!GameMap::IsInGrid(tile)) return false;
		if (_simulation->tileOwnerPlayer(tile) != _gameInterface->playerId()) return false;

		return _simulation->IsFrontBuildable(tile) || _simulation->IsCritterBuilding(tile);
	}

	bool IsForeignFrontBuildable(WorldTile2 tile)
	{
		if (!GameMap::IsInGrid(tile)) return false;

		int32 tileOwner = _simulation->tileOwnerPlayer(tile);
		if (tileOwner != -1 && tileOwner != _gameInterface->playerId()) {
			return _simulation->IsFrontBuildable(tile) || _simulation->IsCritterBuilding(tile);
		}
		return false;
	}

	

	void HighlightDemolishAreaBuildingRed();

	
private:
	UPROPERTY() UAssetLoaderComponent* _assetLoader;
	UPROPERTY() USceneComponent* _root = nullptr;
	
	UPROPERTY() AGameManager* _gameInterface;
	GameSimulationCore* _simulation = nullptr;
	class IGameNetworkInterface* _networkInterface = nullptr;

	
	float _lastNetworkPlacementTime = -1.0f; // For delay filler mesh...
	CardEnum _delayFillerEnum;
	WorldTile2 _delayFillerTile;
	Direction _delayFillerFaceDirection;
	TileArea _delayFillerArea;
	bool justPlacedBuilding() { return _lastNetworkPlacementTime >= 0; }
	
	UPROPERTY() UDecalComponent* _gridGuide;
	UPROPERTY() TArray<UDecalComponent*> _gridGuideLines;
	
	//class UDecalComponent* _radiusDecal;
	UPROPERTY() UStaticMeshComponent* _radiusMesh;

	// Demolish confirmation show...
	TileArea _demolishHighlightArea;
	

private:
	PlacementType _placementType = PlacementType::None;
	ResourceEnum _harvestResourceEnum = ResourceEnum::None;
	CardEnum _buildingEnum = CardEnum::None;
	
	int32 _intVar1 = 0;
	int32 _intVar2 = 0;
	int32 _intVar3 = 0;
	
	Direction _faceDirection;

	FVector _buildingLocation;

	//int32_t _cardHandIndex = -1;
	bool _useBoughtCard = false;
	CardEnum _useWildCard = CardEnum::None;

	int32 _deliverySourceBuildingId = -1;

	bool _canPlace = false;
	bool _forceCannotPlace = false;

	int32 _dragGatherSuccessCount = 0;
	int32 _dragRoadSuccessCount = 0;
	int32 _dragDemolishSuccessCount = 0;
	
	TileArea _area;
	TileArea _area2; // For road, and farm's actual area and TODO: Ranch start
	TileArea _area3; // Storage trimmed TODO: Ranch final form
	TArray<int32> _roadPathTileIds;
	bool _canPlaceRoad = false;

	WorldTile2 _mouseOnTile;

	DragState _dragState;
	WorldTile2 _dragStartTile;

	// Special
	std::vector<FarmTile> _farmWorldTiles;
	WorldTile2 _farmStartTile;

	// Don't recalculate if there is no state change
	WorldTile2 _lastMouseOnTile = WorldTile2::Invalid;
	DragState _lastDragState = DragState::None;
	bool _justRotated = false;

};
