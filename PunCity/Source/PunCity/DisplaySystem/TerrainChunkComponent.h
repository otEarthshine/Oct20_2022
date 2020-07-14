// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "ProceduralMeshComponent.h"
#include "PunCity/Simulation/GameSimulationConstants.h"
#include "PunCity/Simulation/GameRegionSystem.h"
#include "PunCity/AssetLoaderComponent.h"
#include "Materials/MaterialInstanceDynamic.h"

#include "TerrainChunkComponent.generated.h"


struct WorldRegion2;
class PunTerrainGenerator;

UCLASS( ClassGroup=(Custom), meta=(BlueprintSpawnableComponent) )
class UTerrainChunkComponent : public UProceduralMeshComponent
{
	GENERATED_BODY()
public:

	bool GetShadowIndirectOnly() const override {
		return true;
	}

	void UpdateTerrainChunkMesh(class GameSimulationCore& simulation, WorldRegion2 region, int tileDimX, int tileDimY, 
								bool createMesh, bool& containsWater);

	UPROPERTY() TArray<float> meshHeightsSmooth;

	UPROPERTY() TArray<FVector> vertices;

	float GetTerrainDisplayHeight(LocalTile2 localTile) {
		// 69 .. border size 2.. so 0,1,2,3 ... 3 is the first...
		int32 outerX = (localTile.x * 2 + 3);
		int32 outerY = (localTile.y * 2 + 3);
		return meshHeightsSmooth[outerX + outerY * tessOuterVertSize];
	}

	//UPROPERTY() UTexture2D* BeachSandTintTexture = nullptr;
	
	//UPROPERTY() TArray<UMaterialInstanceDynamic*> MaterialInstances;

	UPROPERTY() UMaterialInstanceDynamic* MaterialInstance = nullptr;

	void SetTerrainMaterial(UAssetLoaderComponent* assetLoader, bool isPlainMaterial = false)
	{
		LLM_SCOPE_(EPunSimLLMTag::PUN_DisplayTerrain);

		bIsPlainMaterial = isPlainMaterial;

		if (bIsPlainMaterial) {
			SetMaterial(0, assetLoader->M_PlainMaterial);
		}
		else 
		{
			if (!MaterialInstance) {
				MaterialInstance = UMaterialInstanceDynamic::Create(assetLoader->GetTerrainMaterial(), this);
			}
			SetMaterial(0, MaterialInstance);
		}
	}

	float updatedTime = 0;

	bool bIsPlainMaterial = false;

private:
	int _lastHoleCount = 0;

private:
	// Higher poly for height displacement...
	const int tessMultiplier = 2;
	const float displayUnitPerTile = CoordinateConstants::DisplayUnitPerTile / tessMultiplier;

	const int tileSize = CoordinateConstants::TilesPerRegion;
	const int vertSize = tileSize + 1;
	const int outerVertSize = vertSize + 2;

	const int outerVertTotal = outerVertSize * outerVertSize;

	const int tessTileSize = CoordinateConstants::TilesPerRegion * tessMultiplier;
	const int tessVertSize = tessTileSize + 1;
	const int tessOuterVertSize = tessVertSize + 2 * tessMultiplier;

	const int tessTileTotal = tessTileSize * tessTileSize;
	const int tessVertTotal = tessVertSize * tessVertSize;
	const int tessOuterVertTotal = tessOuterVertSize * tessOuterVertSize;
};
