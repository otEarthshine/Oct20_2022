// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "DisplaySystemComponent.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "TerritoryMeshComponent.h"

#include "RegionDisplayComponent.generated.h"

UCLASS( ClassGroup=(Custom), meta=(BlueprintSpawnableComponent) )
class URegionDisplayComponent : public UDisplaySystemComponent
{
	GENERATED_BODY()
public:
	URegionDisplayComponent()
	{
		PrimaryComponentTick.bCanEverTick = false;
		_maxSpawnPerTick = 3;
	}

	float GetTerrainDisplayHeight(WorldTile2 tile);

	void BeforeDisplay(bool isOverlapTerrain) {
		_isOverlapTerrain = isOverlapTerrain;
		
		if (isOverlapTerrain) {
			_maxSpawnPerTick = 1;
		} else {
			_maxSpawnPerTick = 999;
		}

		std::vector<bool>& displayedProvinceThisTick = simulation().provinceSystem().displayedProvinceThisTick;
		for (size_t i = 0; i < displayedProvinceThisTick.size(); i++) {
			displayedProvinceThisTick[i] = false;
		}
	}
		
protected:
	int CreateNewDisplay(int objectId) override;
	void OnSpawnDisplay(int objectId, int meshId, WorldAtom2 cameraAtom) override;
	void UpdateDisplay(int objectId, int meshId, WorldAtom2 cameraAtom) override;
	void HideDisplay(int meshId) override;

//private:
//	void UpdateOverlayDisplay(int regionId, int meshId, OverlaySystem& overlaySystem);

private:
	bool _isOverlapTerrain = false;
	
	UPROPERTY() TArray<class UStaticMeshComponent*> _waterMeshes;
	UPROPERTY() TArray<UMaterialInstanceDynamic*> _waterMaterials;
	UPROPERTY() TArray<class UStaticMeshComponent*> _waterDecalMeshes;

	UPROPERTY() TArray<class UTerrainChunkComponent*> _terrainChunks;
	UPROPERTY() TArray<class UStaticMeshComponent*> _groundColliderMeshes;
};
