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


class TerrainChunkData
{
public:
	// TODO: might not need this?
	void UpdateTerrainChunkMesh_Prepare(class GameSimulationCore& simulation, WorldRegion2 region, int tileDimX, int tileDimY, bool createMesh);

	void UpdateTerrainChunkMesh_Prepare1();
	void UpdateTerrainChunkMesh_Prepare2(class GameSimulationCore& simulation, WorldRegion2 region);
	uint8 UpdateTerrainChunkMesh_Prepare3(WorldRegion2 region);

	float GetTerrainDisplayHeight(LocalTile2 localTile) {
		// 69 .. border size 2.. so 0,1,2,3 ... 3 is the first...
		int32 outerX = (localTile.x * 2 + 3);
		int32 outerY = (localTile.y * 2 + 3);
		return meshHeightsSmooth[outerX + outerY * tessOuterVertSize];
	}

	std::vector<WorldTile2> holes;

	TArray<float> meshHeightsNoTess;
	TArray<FLinearColor> meshColorsNoTess;

	TArray<float> meshHeightsTess;
	TArray<float> meshHeightsSmooth;
	TArray<FLinearColor> meshColors;
	TArray<FLinearColor> meshColorsSmooth;

	TArray<FVector> vertices;
	TArray<FLinearColor> vertexColors;
	TArray<FVector> normals;
	TArray<FProcMeshTangent> tangents;

	TArray<FVector2D> UV0;

	TArray<int32> tris;

	bool containsWater = false;

	/*
	 * Debug
	 */
	FVector GetVerticesAt(LocalTile2 localTile)
	{
		int32 tessTileX = localTile.x * tessMultiplier;
		int32 tessTileY = localTile.y * tessMultiplier;
		return vertices[tessTileX + tessTileY * tessVertSize];
	}

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

	// Vertices are placed shifted 0.5, out of tile center
	const float vertStart = -0.5f;

	const float flatHeight = FDToFloat(FlatLandHeight);
	const float beachToWaterHeight = FDToFloat(BeachToWaterHeight);
};


UCLASS( ClassGroup=(Custom), meta=(BlueprintSpawnableComponent) )
class UTerrainChunkComponent : public UProceduralMeshComponent
{
	GENERATED_BODY()
public:
#if TRAILER_MODE
	static void ResetCache();
#endif
	
	bool GetShadowIndirectOnly() const override {
		return true;
	}
	
	void UpdateTerrainChunkMesh_UpdateMesh(bool createMesh, TerrainChunkData& terrainChunkData);

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

	bool bIsPlainMaterial = false;

	float updatedTime = 0;
	bool containsWater = false;
};
