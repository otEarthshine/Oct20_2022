// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "ObjectDescriptionUI.h"
#include "PunCity/BuildingMeshesComponent.h"
#include "PunCity/DisplaySystem/TerritoryMeshComponent.h"

#include "ObjectDescriptionUISystem.generated.h"


/**
 * 
 */
UCLASS()
class UObjectDescriptionUISystem : public UPunWidget, public UObjectDescriptionUIParent
{
	GENERATED_BODY()
public:
	void CreateWidgets();

	void Tick();

	void LeftMouseDown();

	void UpdateDescriptionUI();

	bool IsShowingDescriptionUI();
	void CloseDescriptionUI() override;

	bool IsHoveredOnScrollUI() {
		UVerticalBox* punBox = _objectDescriptionUI->DescriptionPunBox->PunVerticalBox;
		for (int32 i = 0; i < punBox->GetChildrenCount(); i++) {
			auto dropdown = Cast<UPunDropdown>(punBox->GetChildAt(i));
			if (dropdown && dropdown->IsHovered()) {
				return true;
			}
		}
		
		return _objectDescriptionUI->ChooseResourceOverlay->IsHovered() ||
			_objectDescriptionUI->ManageStorageOverlay->IsHovered() ||
			_objectDescriptionUI->DescriptionPunBoxScrollOuter->IsHovered();
	}

	TArray<UWidget*> GetEmptyCardSlot() {
		return _objectDescriptionUI->EmptyCardSlots;
	}

	void SwitchToNextBuilding(bool isLeft)
	{
		DescriptionUIState uiState = simulation().descriptionUIState();

		if (uiState.objectType == ObjectTypeEnum::Building)
		{
			CardEnum buildingEnum = simulation().buildingEnum(uiState.objectId);
			const std::vector<int32>& buildingIds = simulation().buildingIds(playerId(), buildingEnum);
			if (buildingIds.size() > 1) {
				// Find the index of the current building in the array and increment it by 1
				int32 index = -1;
				for (size_t i = 0; i < buildingIds.size(); i++) {
					if (buildingIds[i] == uiState.objectId) {
						index = i;
						break;
					}
				}
				if (index != -1) {
					int32 nextIndex = (index + 1) % buildingIds.size();
					if (isLeft) {
						nextIndex = (index - 1 + buildingIds.size()) % buildingIds.size();
					}

					int32 buildingId = buildingIds[nextIndex];
					simulation().SetDescriptionUIState({ ObjectTypeEnum::Building, buildingId });
					networkInterface()->SetCameraAtom(simulation().building(buildingId).centerTile().worldAtom2());
				}
			}
		}
	}

	void HideForPhotoMode()
	{
		dataSource()->simulation().SetDescriptionUIState(DescriptionUIState());
		UpdateDescriptionUI();
	}

private:
	void SpawnSelectionMesh(UMaterialInterface* material, FVector location) {
		SpawnMesh(dataSource()->assetLoader()->SelectionMesh, material, FTransform(location), true, 0, false);
	}
	void SpawnMesh(UStaticMesh* mesh, UMaterialInterface* material, FTransform transform, bool isRotating = false, int32 customDepth = 0, bool castShadow = true);
	bool TryMouseCollision(UStaticMesh* mesh, FTransform transform, float& shortestHit);

	void ShowTileSelectionDecal(FVector displayLocation, WorldTile2 size = WorldTile2(1, 1), UMaterial* material = nullptr);
	void ShowRegionSelectionDecal(WorldTile2 tile, bool isHover = false);

	void AddSelectStartLocationButton(int32 provinceId, UPunBoxWidget* descriptionBox);
	void AddClaimLandButtons(int32 provinceId, UPunBoxWidget* descriptionBox);
	
	void CallBack1(UPunWidget* punWidgetCaller, CallbackEnum callbackEnum) final;

	// Helper
	
	void AddBiomeInfo(WorldTile2 tile, UPunBoxWidget* focusBox);
	void AddBiomeDebugInfo(WorldTile2 tile, UPunBoxWidget* descriptionBox);
	
	//void AddTileInfo(WorldTile2 tile, UPunBoxWidget* descriptionBox);
	void AddImpassableTileInfo(WorldTile2 tile, UPunBoxWidget* focusBox);
	void AddProvinceInfo(int32 provinceId, WorldTile2 tile, UPunBoxWidget* focusBox);

	void AddGeoresourceInfo(int32 provinceId, UPunBoxWidget* focusBox);

	void AddProvinceUpkeepInfo(int32 provinceIdClean, UPunBoxWidget* focusBox);
	
	void AddEfficiencyText(Building& building, UPunBoxWidget* focusBox);
	void AddTradeFeeText(class TradeBuilding& building, UPunBoxWidget* descriptionBox);


	void Indent(float amount) {
		_objectDescriptionUI->DescriptionPunBox->indentation = amount;
	}
	void ResetIndent() {
		_objectDescriptionUI->DescriptionPunBox->indentation = 20;
	}

private:
	UPROPERTY() class UObjectDescriptionUI* _objectDescriptionUI;

	int meshIndex = 0;
	UPROPERTY() TArray<UStaticMeshComponent*> _selectionMeshes;

	int32 skelMeshIndex = 0;
	UPROPERTY() TArray<USkeletalMeshComponent*> _selectionSkelMeshes;
	
	//UPROPERTY() UBuildingMeshesComponent* _buildingMesh = nullptr;

	UPROPERTY() UStaticMeshComponent* _collider; // Collider used for mouseHitTesting

	UPROPERTY() UDecalComponent* _tileSelectionDecal = nullptr;
	UPROPERTY() UStaticMeshComponent* _tileSelectionMesh = nullptr;

	UPROPERTY() UTerritoryMeshComponent* _regionSelectionMesh = nullptr;
	UPROPERTY() UTerritoryMeshComponent* _regionHoverMesh = nullptr;


	UPROPERTY() TArray<class UDecalComponent*> _groundBoxHighlightDecals;
	int32 _groundBoxHighlightCount = 0;


	bool _alreadyDidShiftDownUpgrade = false;

	bool _justOpenedDescriptionUI = false;
};
