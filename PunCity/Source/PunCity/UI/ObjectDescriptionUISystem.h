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
		return _objectDescriptionUI->ChooseResourceOverlay->IsHovered() ||
			_objectDescriptionUI->ManageStorageOverlay->IsHovered();
	}

	TArray<UWidget*> GetEmptyCardSlot() {
		return _objectDescriptionUI->EmptyCardSlots;
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
	
	void AddBiomeInfo(WorldTile2 tile, UPunBoxWidget* descriptionBox);
	void AddBiomeDebugInfo(WorldTile2 tile, UPunBoxWidget* descriptionBox);
	
	void AddTileInfo(WorldTile2 tile, UPunBoxWidget* descriptionBox);
	void AddProvinceInfo(int32 provinceId, UPunBoxWidget* descriptionBox);

	void AddGeoresourceInfo(int32 provinceId, UPunBoxWidget* descriptionBox, bool showTopLine = false);

	void AddProvinceUpkeepInfo(int32 provinceIdClean, UPunBoxWidget* descriptionBox);
	
	void AddEfficiencyText(Building& building, UPunBoxWidget* descriptionBox);
	void AddTradeFeeText(class TradeBuilding& building, UPunBoxWidget* descriptionBox);

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
};
