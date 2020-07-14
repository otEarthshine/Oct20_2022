// Fill out your copyright notice in the Description page of Project Settings.


#include "TerrainChunkComponent.h"
#include "PunCity/PunUnrealUtils.h"
#include "PunCity/Simulation/GameSimulationCore.h"
#include <chrono>
#include "Kismet/GameplayStatics.h"

using namespace std;
using namespace std::chrono;

void UTerrainChunkComponent::UpdateTerrainChunkMesh(GameSimulationCore& simulation, WorldRegion2 region, int tileDimX, int tileDimY,
													bool createMesh, bool& containsWater)
{
	LLM_SCOPE_(EPunSimLLMTag::PUN_DisplayTerrain);
	
	updatedTime = UGameplayStatics::GetTimeSeconds(this);
	//PUN_LOG("UpdateTerrainChunkMesh: %f", updatedTime);
	
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


	//! Meshes Init
	static TArray<float> meshHeightsNoTess;
	static TArray<FLinearColor> meshColorsNoTess;

	static TArray<float> meshHeights;
	//static TArray<float> meshHeightsSmooth;
	static TArray<FLinearColor> meshColors;
	static TArray<FLinearColor> meshColorsSmooth;

	static TArray<FVector2D> UV0;
	static TArray<FLinearColor> vertexColors;
	static TArray<FVector> normals;
	static TArray<FProcMeshTangent> tangents;

	static TArray<int32> tris;

	if (meshHeights.Num() == 0)
	{
		meshHeightsNoTess.Reserve(outerVertTotal);
		meshColorsNoTess.Reserve(outerVertTotal);

		meshHeights.Reserve(tessOuterVertTotal);
		meshHeightsSmooth.Reserve(tessOuterVertTotal);
		meshColors.Reserve(tessOuterVertTotal);
		meshColorsSmooth.Reserve(tessOuterVertTotal);

		vertices.Reserve(tessVertTotal);
		UV0.Reserve(tessVertTotal);
		vertexColors.Reserve(tessVertTotal);
		normals.Reserve(tessVertTotal);
		tangents.Reserve(tessVertTotal);

		tris.Reserve(tessTileTotal * 6);
	}

	meshHeightsNoTess.Empty(outerVertTotal);
	meshColorsNoTess.Empty(outerVertTotal);

	meshHeights.Empty(tessOuterVertTotal);
	meshHeightsSmooth.Empty(tessOuterVertTotal);
	meshColors.Empty(tessOuterVertTotal);
	meshColorsSmooth.Empty(tessOuterVertTotal);

	vertices.Empty(tessVertTotal);
	UV0.Empty(tessVertTotal);
	vertexColors.Empty(tessVertTotal);
	normals.Empty(tessVertTotal);
	tangents.Empty(tessVertTotal);

	tris.Empty(tessTileTotal * 6);


	{
		//SCOPE_TIMER("Setup Mesh Arrays... aasff");

		containsWater = false;


		// 
		static std::vector<uint32> sandTintData;
		sandTintData.clear();

		// Find meshHeights/Color for the region mesh and the surrounding...
		// ... 0 to vertSizeX (exclusive)
		// ... -1 to vertSizeX + 1 (exclusive)
		// ... -1 to vertSizeX (inclusive)
		for (int y = -1; y <= vertSize; y++) {
			for (int x = -1; x <= vertSize; x++)
			{
				// If out of range, just pick the closest valid x, y
				int worldX = max(min(x + worldStartX, tileDimX - 1), 0);
				int worldY = max(min(y + worldStartY, tileDimY - 1), 0);

				FloatDet heightFD = heightMap[worldX + worldY * tileDimX];
				float height = FDToFloat(heightFD);

				// Transition fraction for nearWater
				//float transitionFraction = getTransitionFraction(x, y);

				WorldTile2 tile(worldX, worldY);
				float riverFraction = terrainGenerator.riverFraction(tile);
				int32 rainfall255 = terrainGenerator.rainfall255(tile);
				float rainfallFraction = FLinearColor::FromSRGBColor(FColor(rainfall255, 0, 0, 0)).R;

				// Moisture is rainfall+river
				// 0 - 0.8 is normal green... beyond that it is river green...
				//float moistureFraction = fmax(rainfallFraction * 0.8f, riverFraction);

				//transitionFraction = fmin(1.0f, 0.5 * transitionFraction + riverFraction * 0.5);

				FColor color = FColor(PunUnrealUtils::FractionTo255Color(riverFraction), 0, 0, 0);
				sandTintData.push_back(color.ToPackedARGB());


				// Alpha deals with beach color modification...

				if (heightFD >= MountainHeight) {
					meshHeightsNoTess.Add((height - flatHeight) / (1.0f - flatHeight) * CoordinateConstants::AboveFlatDisplayUnitHeight);

					meshColorsNoTess.Add(FLinearColor(rainfallFraction, 0, 0, riverFraction));
				}
				else if (heightFD >= FlatLandHeight)
				{
					meshHeightsNoTess.Add(0);
					meshColorsNoTess.Add(FLinearColor(rainfallFraction, 0, 0, riverFraction));
				}
				// Beach has the same level as flat land. Use the height info to calculate beachPaint instead.
				else if (heightFD >= BeachToWaterHeight)
				{
					float nearWaterPaint = 1.0f - (height - beachToWaterHeight) / (flatHeight - beachToWaterHeight);

					//meshHeights[outerIndex] = 0;
					meshHeightsNoTess.Add(0);

					// ... first 0.5 is used transition to beach... beyond 0.5 becomes river (darker)
					meshColorsNoTess.Add(FLinearColor(rainfallFraction, 0, nearWaterPaint, riverFraction));
				}
				else
				{
					// lowerDisplayHeight from 0 - BelowWaterDisplayUnitHeight
					float heightBelowWater = (beachToWaterHeight - height) / beachToWaterHeight * CoordinateConstants::BelowWaterDisplayUnitHeight;
					const float logisticMax = 4.0f;
					const float sigmoidHeight = 5.0f;


					// Logistic sigmoid slope that turns sigmoidHeight to logisticMax.0f height
					if (heightBelowWater < sigmoidHeight) {
						float logisticX = heightBelowWater * 2.0f - sigmoidHeight; // 0 to 5 turns into -5 to 5

						heightBelowWater += logisticMax / (1.0f + exp(-logisticX));
					}
					else {
						heightBelowWater += logisticMax;

						containsWater = true;
					}

					meshHeightsNoTess.Add(-heightBelowWater);

					meshColorsNoTess.Add(FLinearColor(rainfallFraction, 0, 1, riverFraction));
				}
			}
		}


		// Tesselation expand
		for (int y = 0; y < tessOuterVertSize; y++) {
			for (int x = 0; x < tessOuterVertSize; x++)
			{
				int xx = x / 2;
				int yy = y / 2;

				// special case for outer rim...
				if (x == tessOuterVertSize - 1 || y == tessOuterVertSize - 1) {
					meshHeights.Add(meshHeightsNoTess[xx + yy * outerVertSize]);
					meshColors.Add(meshColorsNoTess[xx + yy * outerVertSize]);
					continue;
				}

				// Use if-then instead of linear interpolation for speed
				float height00 = meshHeightsNoTess[xx + yy * outerVertSize];
				FLinearColor color00 = meshColorsNoTess[xx + yy * outerVertSize];

				if (x % 2 == 0) {
					if (y % 2 == 0) {
						meshHeights.Add(height00);
						meshColors.Add(color00);
					}
					else {
						// Lerp on y
						meshHeights.Add(0.5f * (height00 + meshHeightsNoTess[xx + (yy + 1) * outerVertSize]));
						meshColors.Add(0.5f * (color00 + meshColorsNoTess[xx + (yy + 1) * outerVertSize]));
					}
				}
				else {
					if (y % 2 == 0) {
						// Lerp on x
						meshHeights.Add(0.5f *(height00 + meshHeightsNoTess[(xx + 1) + yy * outerVertSize]));
						meshColors.Add(0.5f *(color00 + meshColorsNoTess[(xx + 1) + yy * outerVertSize]));
					}
					else {
						// Lerp on x y
						meshHeights.Add(0.25f * (height00 +
							meshHeightsNoTess[(xx + 1) + yy * outerVertSize] +
							meshHeightsNoTess[xx + (yy + 1) * outerVertSize] +
							meshHeightsNoTess[(xx + 1) + (yy + 1) * outerVertSize]));

						meshColors.Add(0.25f * (color00 +
							meshColorsNoTess[(xx + 1) + yy * outerVertSize] +
							meshColorsNoTess[xx + (yy + 1) * outerVertSize] +
							meshColorsNoTess[(xx + 1) + (yy + 1) * outerVertSize]));
					}
				}
			}
		}

		// Smoothing on meshHeights
		for (int y = 0; y < tessOuterVertSize; y++) {
			for (int x = 0; x < tessOuterVertSize; x++)
			{
				float height = meshHeights[x + y * tessOuterVertSize];
				FLinearColor color = meshColors[x + y * tessOuterVertSize];

				// Smooth where applicable
				if (x > 0 && y > 0 && x < (tessOuterVertSize - 1) && y < (tessOuterVertSize - 1))
				{
					height = height / 4;

					height += meshHeights[(x - 1) + (y - 1) * tessOuterVertSize] / 16;
					height += meshHeights[(x - 1) + (y + 1) * tessOuterVertSize] / 16;
					height += meshHeights[(x + 1) + (y - 1) * tessOuterVertSize] / 16;
					height += meshHeights[(x + 1) + (y + 1) * tessOuterVertSize] / 16;

					height += meshHeights[x + (y - 1) * tessOuterVertSize] / 8;
					height += meshHeights[x + (y + 1) * tessOuterVertSize] / 8;
					height += meshHeights[(x - 1) + y * tessOuterVertSize] / 8;
					height += meshHeights[(x + 1) + y * tessOuterVertSize] / 8;

					color = color / 4;

					color += meshColors[(x - 1) + (y - 1) * tessOuterVertSize] / 16;
					color += meshColors[(x - 1) + (y + 1) * tessOuterVertSize] / 16;
					color += meshColors[(x + 1) + (y - 1) * tessOuterVertSize] / 16;
					color += meshColors[(x + 1) + (y + 1) * tessOuterVertSize] / 16;

					color += meshColors[x + (y - 1) * tessOuterVertSize] / 8;
					color += meshColors[x + (y + 1) * tessOuterVertSize] / 8;
					color += meshColors[(x - 1) + y * tessOuterVertSize] / 8;
					color += meshColors[(x + 1) + y * tessOuterVertSize] / 8;
				}

				meshHeightsSmooth.Add(height);
				meshColorsSmooth.Add(color);
			}
		}

		
		/*
		 * Construct mesh
		 */

		// Fill verts
		for (int y = 0; y < tessVertSize; y++) {
			for (int x = 0; x < tessVertSize; x++)
			{
				int outerX = x + tessMultiplier; // outerX starts before x
				int outerY = y + tessMultiplier;

				float height = meshHeightsSmooth[outerX + outerY * tessOuterVertSize];

				vertices.Add(FVector(x * displayUnitPerTile + vertStart, y * displayUnitPerTile + vertStart, height));
				UV0.Add(FVector2D(float(x) / tessTileSize, float(y) / tessTileSize)); // use tile size because for UV (since UV is length, not vert)

				// Normal and Tangents
				float heightNegX = meshHeightsSmooth[(outerX - 1) + outerY * tessOuterVertSize];
				float heightPosX = meshHeightsSmooth[(outerX + 1) + outerY * tessOuterVertSize];
				float heightNegY = meshHeightsSmooth[outerX + (outerY - 1) * tessOuterVertSize];
				float heightPosY = meshHeightsSmooth[outerX + (outerY + 1) * tessOuterVertSize];

				FVector normal(heightNegX - heightPosX, heightNegY - heightPosY, displayUnitPerTile * 2);
				normal.Normalize();
				normals.Add(normal);

				FVector tangentX(displayUnitPerTile * 2, 0.0f, heightNegX - heightPosX);
				tangentX.Normalize();
				tangents.Add(FProcMeshTangent(tangentX, false));

				vertexColors.Add(meshColorsSmooth[outerX + outerY * tessOuterVertSize]);
			}
		}

		
		bool isCreatingMesh = createMesh || holes.size() != _lastHoleCount;
		if (isCreatingMesh)
		{
			for (int y = 0; y < tessTileSize; y++) {
				for (int x = 0; x < tessTileSize; x++)
				{
					int i = x + y * tessVertSize;
					if (((x & 1) + (y & 1)) & 1) {

						tris.Add(i);
						tris.Add(i + 1 + tessVertSize);
						tris.Add(i + 1);

						tris.Add(i);
						tris.Add(i + tessVertSize);
						tris.Add(i + 1 + tessVertSize);
					}
					else
					{
						tris.Add(i);
						tris.Add(i + tessVertSize);
						tris.Add(i + 1);

						tris.Add(i + 1);
						tris.Add(i + tessVertSize);
						tris.Add(i + 1 + tessVertSize);
					}
				}
			}

			// Dig hole
			// Do this by setting tris to same vertices to hide it.
			// tessOuterVertSize is 32*2 + 2*2 = 68
			auto hideTris = [&](int32 tessIndex)
			{
				tris[tessIndex * 6] = 0;
				tris[tessIndex * 6 + 1] = 0;
				tris[tessIndex * 6 + 2] = 0;

				tris[tessIndex * 6 + 3] = 0;
				tris[tessIndex * 6 + 4] = 0;
				tris[tessIndex * 6 + 5] = 0;
			};

			for (WorldTile2 tile : holes)
			{
				WorldRegion2 tileRegion = tile.region();
				if (region == tileRegion) {
					LocalTile2 localTile = tile.localTile();
					if (localTile.isValid()) {
						int32 tessTileX = localTile.x * tessMultiplier;
						int32 tessTileY = localTile.y * tessMultiplier;
						hideTris(tessTileX + tessTileY * tessTileSize);
						hideTris((tessTileX + 1) + tessTileY * tessTileSize);
						hideTris(tessTileX + (tessTileY + 1) * tessTileSize);
						hideTris((tessTileX + 1) + (tessTileY + 1) * tessTileSize);
					}
				}
			}
		}
		
	}

	{
		//SCOPE_TIMER("Terrain Chunk Creation");

		if (createMesh || holes.size() != _lastHoleCount) {
			CreateMeshSection_LinearColor(0, vertices, tris, normals, UV0, vertexColors, tangents, false);
		}
		else {
			UpdateMeshSection_LinearColor(0, vertices, normals, UV0, vertexColors, tangents);
		}

		_lastHoleCount = holes.size();

		//meshHeightsSmooth = meshHeightsSmooth;
	}

}
