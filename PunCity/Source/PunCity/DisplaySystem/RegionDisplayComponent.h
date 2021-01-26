// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "DisplaySystemComponent.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "TerritoryMeshComponent.h"
#include "TerrainChunkComponent.h"

#include "RegionDisplayComponent.generated.h"

UCLASS( ClassGroup=(Custom), meta=(BlueprintSpawnableComponent) )
class URegionDisplayComponent : public UDisplaySystemComponent
{
	GENERATED_BODY()
public:
	URegionDisplayComponent();

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

	void BeforeAdd() override {
		chunkInfosToUpDate.clear();
	}

	void AfterAdd() override;


	// Debug:
	TerrainChunkData GetTerrainChunkData(int32 meshId) { return _terrainChunkData[meshId]; }
	UTerrainChunkComponent* GetTerrainChunk(int32 meshId) { return _terrainChunks[meshId]; }
		
protected:
	int CreateNewDisplay(int objectId) override;
	
	void OnSpawnDisplay(int objectId, int meshId, WorldAtom2 cameraAtom) override;
	void UpdateDisplay(int objectId, int meshId, WorldAtom2 cameraAtom, bool justSpawned, bool justCreated) override;
	void HideDisplay(int32 meshId, int32 regionId) override;

//private:
//	void UpdateOverlayDisplay(int regionId, int meshId, OverlaySystem& overlaySystem);

private:
	bool _isOverlapTerrain = false;

	std::vector<MeshChunkInfo> chunkInfosToUpDate;
	
	UPROPERTY() TArray<class UStaticMeshComponent*> _waterMeshes;
	UPROPERTY() TArray<UMaterialInstanceDynamic*> _waterMaterials;
	UPROPERTY() TArray<class UStaticMeshComponent*> _waterDecalMeshes;

	TArray<TerrainChunkData> _terrainChunkData;
	UPROPERTY() TArray<class UTerrainChunkComponent*> _terrainChunks;
	UPROPERTY() TArray<class UStaticMeshComponent*> _groundColliderMeshes;
};
