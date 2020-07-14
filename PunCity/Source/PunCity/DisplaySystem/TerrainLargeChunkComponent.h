// Pun Dumnernchanvanit's

#pragma once

#include "CoreMinimal.h"
#include "ProceduralMeshComponent.h"
#include "PunCity/Simulation/GameSimulationConstants.h"
#include "PunCity/Simulation/GameSimulationCore.h"
#include "Materials/MaterialInstanceDynamic.h"

#include "TerrainLargeChunkComponent.generated.h"

/**
 * 
 */
UCLASS()
class PROTOTYPECITY_API UTerrainLargeChunkComponent : public UProceduralMeshComponent
{
	GENERATED_BODY()
public:
	bool GetShadowIndirectOnly() const override {
		return true;
	}

	void Init(int32 tileSizeIn, float displayUnitPerTileIn, int32 tileSkipIn = 1, bool isNotWorldMapIn = true) {
		tileSize = tileSizeIn;
		displayUnitPerTile = displayUnitPerTileIn;
		tileSkip = tileSkipIn;
		isNotWorldMap = isNotWorldMapIn;
	}

	void UpdateTerrainChunkMesh(GameSimulationCore& simulation, WorldRegion2 region, bool createMesh)
	{
		int innerVertSize = tileSize + 1;
		int outerVertSize = tileSize + 3;

		int tileDimX = GameMapConstants::TilesPerWorldX;
		int tileDimY = GameMapConstants::TilesPerWorldY;

		PunTerrainGenerator& terrainGenerator = simulation.terrainGenerator();
		std::vector<int16_t>& heightMap = terrainGenerator.heightMap;

		PunTerrainChanges& terrainChanges = simulation.terrainChanges();
		std::vector<WorldTile2> holes = terrainChanges.GetRegionHoles(region);

		//UE_LOG(LogTemp, Error, TEXT("Setup chunk: %d, %d"), region.minXTile(), region.minYTile());
		const int worldStartX = region.minXTile();
		const int worldStartY = region.minYTile();

		// Vertices are placed shifted 0.5, out of tile center
		const float vertStart = -0.5f;

		const float flatHeight = FDToFloat(FlatLandHeight);
		const float beachToWaterHeight = FDToFloat(BeachToWaterHeight);


		static TArray<float> meshHeights;
		static TArray<FLinearColor> meshColors;
		
		//! Meshes Init
		static TArray<FVector2D> UV0;
		static TArray<FLinearColor> vertexColors;
		static TArray<FVector> normals;
		static TArray<FProcMeshTangent> tangents;
		
		static TArray<int32> tris;

		if (meshHeights.Num() == 0)
		{
			meshHeights.Reserve(outerVertSize);
			meshColors.Reserve(outerVertSize);

			vertices.Reserve(innerVertSize);
			UV0.Reserve(innerVertSize);
			vertexColors.Reserve(innerVertSize);
			normals.Reserve(innerVertSize);
			tangents.Reserve(innerVertSize);

			tris.Reserve(tileSize * 6);
		}

		meshHeights.Empty(outerVertSize);
		meshColors.Empty(outerVertSize);

		vertices.Empty(innerVertSize);
		UV0.Empty(innerVertSize);
		vertexColors.Empty(innerVertSize);
		normals.Empty(innerVertSize);
		tangents.Empty(innerVertSize);

		tris.Empty(tileSize * 6);
		

		// Find meshHeights/Color for the region mesh and the surrounding...
		// ... 0 to vertSizeX (exclusive)
		// ... -1 to vertSizeX + 1 (exclusive)
		// ... -1 to vertSizeX (inclusive)
		for (int y = -1; y <= innerVertSize; y++) {
			for (int x = -1; x <= innerVertSize; x++)
			{
				// If out of range, just pick the closest valid x, y
				int worldX = max(min(x * tileSkip + worldStartX, tileDimX - 1), 0);
				int worldY = max(min(y * tileSkip + worldStartY, tileDimY - 1), 0);

				FloatDet heightFD = heightMap[worldX + worldY * tileDimX];
				float height = FDToFloat(heightFD);

				// Transition fraction for nearWater
				// float transitionFraction = getTransitionFraction(x, y);

				WorldTile2 tile(worldX, worldY);
				float riverFraction = terrainGenerator.riverFraction(tile);
				int32 rainfall255 = terrainGenerator.rainfall255(tile);
				float rainfallFraction = FLinearColor::FromSRGBColor(FColor(rainfall255, 0, 0, 0)).R;

				// Moisture is rainfall+river
				// 0 - 0.8 is normal green... beyond that it is river green...
				//float moistureFraction = fmax(rainfallFraction * 0.8f, riverFraction);

				//transitionFraction = fmin(1.0f, 0.5 * transitionFraction + riverFraction * 0.5);

				//FColor color = FColor(PunUnrealUtils::FractionTo255Color(riverFraction), 0, 0, 0);
				//sandTintData.push_back(color.ToPackedARGB());


				// Alpha deals with beach color modification...

				if (heightFD >= MountainHeight) {
					meshHeights.Add((height - flatHeight) / (1.0f - flatHeight) * CoordinateConstants::AboveFlatDisplayUnitHeight);

					meshColors.Add(FLinearColor(rainfallFraction, 0, 0, riverFraction));
				}
				else if (heightFD >= FlatLandHeight)
				{
					meshHeights.Add(0);
					meshColors.Add(FLinearColor(rainfallFraction, 0, 0, riverFraction));
				}
				// Beach has the same level as flat land. Use the height info to calculate beachPaint instead.
				else if (heightFD >= BeachToWaterHeight)
				{
					float nearWaterPaint = 1.0f - (height - beachToWaterHeight) / (flatHeight - beachToWaterHeight);
					
					meshHeights.Add(0);

					// ... first 0.5 is used transition to beach... beyond 0.5 becomes river (darker)
					meshColors.Add(FLinearColor(rainfallFraction, 0, nearWaterPaint, riverFraction));
				}
				else
				{
					// lowerDisplayHeight from 0 - BelowWaterDisplayUnitHeight
					float heightBelowWater = (beachToWaterHeight - height) / beachToWaterHeight * CoordinateConstants::BelowWaterDisplayUnitHeight;
					const float logisticMax = 4.0f;
					const float sigmoidHeight = 5.0f;


					//// Logistic sigmoid slope that turns sigmoidHeight to logisticMax.0f height
					//if (heightBelowWater < sigmoidHeight) {
					//	float logisticX = heightBelowWater * 2.0f - sigmoidHeight; // 0 to 5 turns into -5 to 5

					//	heightBelowWater += logisticMax / (1.0f + exp(-logisticX));
					//}
					//else {
					//	heightBelowWater += logisticMax;

					//	containsWater = true;
					//}

					meshHeights.Add(-heightBelowWater);

					meshColors.Add(FLinearColor(rainfallFraction, 0, 1, riverFraction));
				}
			}
		}


		/*
		 * Construct mesh
		 */
		
		// Fill verts
		for (int y = 0; y < innerVertSize; y++) {
			for (int x = 0; x < innerVertSize; x++)
			{
				int outerX = x + 1; // outerX starts before x
				int outerY = y + 1;
				
				float height = meshHeights[outerX + outerY * outerVertSize];

				vertices.Add(FVector(x * displayUnitPerTile + vertStart,
											y * displayUnitPerTile + vertStart, height));

				if (isNotWorldMap) {
					UV0.Add(FVector2D(float(x) / tileSize, float(y) / tileSize)); // use tile size because for UV (since UV is length, not vert)	
				} else {
					int worldX = max(min(x * tileSkip + worldStartX, tileDimX - 1), 0);
					int worldY = max(min(y * tileSkip + worldStartY, tileDimY - 1), 0);
					UV0.Add(FVector2D(float(worldX) / tileDimX, float(worldY) / tileDimY));
				}

				// Normal and Tangents
				float heightNegX = meshHeights[(outerX - 1) + outerY * outerVertSize];
				float heightPosX = meshHeights[(outerX + 1) + outerY * outerVertSize];
				float heightNegY = meshHeights[outerX + (outerY - 1) * outerVertSize];
				float heightPosY = meshHeights[outerX + (outerY + 1) * outerVertSize];

				FVector normal(heightNegX - heightPosX, heightNegY - heightPosY, displayUnitPerTile * 2);
				normal.Normalize();
				normals.Add(normal);

				FVector tangentX(displayUnitPerTile * 2, 0.0f, heightNegX - heightPosX);
				tangentX.Normalize();
				tangents.Add(FProcMeshTangent(tangentX, false));

				vertexColors.Add(meshColors[outerX + outerY * outerVertSize]);
			}
		}

		if (createMesh)
		{
			if (isNotWorldMap)
			{
				for (int y = 0; y < tileSize; y++) {
					for (int x = 0; x < tileSize; x++)
					{
						int i = x + y * innerVertSize;

						if (((x & 1) + (y & 1)) & 1)
						{
							tris.Add(i);
							tris.Add(i + 1 + innerVertSize);
							tris.Add(i + 1);

							tris.Add(i);
							tris.Add(i + innerVertSize);
							tris.Add(i + 1 + innerVertSize);
						}
						else
						{
							tris.Add(i);
							tris.Add(i + innerVertSize);
							tris.Add(i + 1);

							tris.Add(i + 1);
							tris.Add(i + innerVertSize);
							tris.Add(i + 1 + innerVertSize);
						}
					}
				}
			}
			else
			{
				for (int y = 0; y < tileSize; y++) {
					for (int x = 0; x < tileSize; x++)
					{	
						int i = x + y * innerVertSize;

						const float depthToCull = -70.0f;
						if (vertices[i].Z < depthToCull &&
							vertices[i + 1].Z < depthToCull &&
							vertices[i + innerVertSize].Z < depthToCull &&
							vertices[i + 1 + innerVertSize].Z < depthToCull) {
							continue;
						}

						if (((x & 1) + (y & 1)) & 1)
						{
							tris.Add(i);
							tris.Add(i + 1 + innerVertSize);
							tris.Add(i + 1);

							tris.Add(i);
							tris.Add(i + innerVertSize);
							tris.Add(i + 1 + innerVertSize);
						}
						else
						{
							tris.Add(i);
							tris.Add(i + innerVertSize);
							tris.Add(i + 1);

							tris.Add(i + 1);
							tris.Add(i + innerVertSize);
							tris.Add(i + 1 + innerVertSize);
						}
					}
				}
			}
		}
		

		{
			if (createMesh) {
				SCOPE_TIMER("Terrain Large Chunk Creation");
				CreateMeshSection_LinearColor(0, vertices, tris, normals, UV0, vertexColors, tangents, false);
			}
			else {
				SCOPE_TIMER("Terrain Large Chunk Update");
				UpdateMeshSection_LinearColor(0, vertices, normals, UV0, vertexColors, tangents);
			}
		}
	}

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

	UPROPERTY() TArray<FVector> vertices;
	UPROPERTY() UMaterialInstanceDynamic* MaterialInstance = nullptr;

	bool bIsPlainMaterial = false;

	bool alreadyUpdatedMesh = false;

	

private:
	int32 tileSize = 0;
	int32 tileSkip = 0;
	float displayUnitPerTile = 0;

	bool isNotWorldMap = true;
	
	//const int tileSize = CoordinateConstants::TilesPerRegion * 4;
//	const int innerVertSize = tileSize + 1; 
//	const int outerVertSize = tileSize + 3;
};
