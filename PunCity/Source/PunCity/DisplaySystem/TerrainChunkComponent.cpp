// Fill out your copyright notice in the Description page of Project Settings.


#include "TerrainChunkComponent.h"
#include "PunCity/PunUnrealUtils.h"
#include "PunCity/Simulation/GameSimulationCore.h"
#include <chrono>
#include "Kismet/GameplayStatics.h"

USTRUCT()
struct FVectorArray {
	UPROPERTY() TArray<FVector> Array;
};
USTRUCT()
struct FLinearColorArray {
	UPROPERTY() TArray<FLinearColor> Array;
};
USTRUCT()
struct FProcMeshTangentArray {
	UPROPERTY() TArray<FProcMeshTangent> Array;
};

using namespace std;
using namespace std::chrono;

#if TRAILER_MODE
static TArray<FVectorArray> verticesCache;
static TArray<FLinearColorArray> vertexColorsCache;
static TArray<FVectorArray> normalsCache;
static TArray<FProcMeshTangentArray> tangentsCache;
void UTerrainChunkComponent::ResetCache()
{
	verticesCache.Empty();
	vertexColorsCache.Empty();
	normalsCache.Empty();
	tangentsCache.Empty();
}
#endif


void TerrainChunkData::UpdateTerrainChunkMesh_Prepare1()
{
	//! Meshes Init

	if (meshHeightsTess.Num() == 0)
	{
		meshHeightsNoTess.Reserve(outerVertTotal);
		meshColorsNoTess.Reserve(outerVertTotal);

		meshHeightsTess.Reserve(tessOuterVertTotal);
		meshHeightsSmooth.Reserve(tessOuterVertTotal);
		meshColors.Reserve(tessOuterVertTotal);
		meshColorsSmooth.Reserve(tessOuterVertTotal);

		vertices.Reserve(tessVertTotal);
		vertexColors.Reserve(tessVertTotal);
		normals.Reserve(tessVertTotal);
		tangents.Reserve(tessVertTotal);

		// UV0 always same
		UV0.Reserve(tessVertTotal);
		for (int y = 0; y < tessVertSize; y++) {
			for (int x = 0; x < tessVertSize; x++)
			{
				UV0.Add(FVector2D(float(x) / tessTileSize, float(y) / tessTileSize)); // use tile size because for UV (since UV is length, not vert)
			}
		}

		// Tris is always the same, so we can cache it
		tris.Reserve(tessTileTotal * 6);
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
	}
}

void TerrainChunkData::UpdateTerrainChunkMesh_Prepare2(class GameSimulationCore& simulation, WorldRegion2 region, bool& containsWater)
{
	int tileDimX = GameMapConstants::TilesPerWorldX;
	int tileDimY = GameMapConstants::TilesPerWorldY;
	
	PunTerrainGenerator& terrainGenerator = simulation.terrainGenerator();
	std::vector<int16>& heightMap = terrainGenerator.heightMap;
	
	const int worldStartX = region.minXTile();
	const int worldStartY = region.minYTile();

	PunTerrainChanges& terrainChanges = simulation.terrainChanges();
	holes = terrainChanges.GetRegionHoles(region);

	

	
	meshHeightsNoTess.Empty(outerVertTotal);
	meshColorsNoTess.Empty(outerVertTotal);

	meshHeightsTess.Empty(tessOuterVertTotal);
	meshHeightsSmooth.Empty(tessOuterVertTotal);
	meshColors.Empty(tessOuterVertTotal);
	meshColorsSmooth.Empty(tessOuterVertTotal);

	vertices.Empty(tessVertTotal);
	//UV0.Empty(tessVertTotal);
	vertexColors.Empty(tessVertTotal);
	normals.Empty(tessVertTotal);
	tangents.Empty(tessVertTotal);


	//SCOPE_TIMER("Setup Mesh Arrays... aasff");

	containsWater = false;


	// 
	std::vector<uint32> sandTintData;

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
}

uint8 TerrainChunkData::UpdateTerrainChunkMesh_Prepare3(WorldRegion2 region)
{
	//PUN_LOG("Tick Region UpdateTerrainChunkMesh_Prepare3 Start region:%d", region.regionId());
	//SCOPE_TIMER_FILTER(1000, "Tick Region UpdateTerrainChunkMesh_Prepare3");
	
	// Tesselation expand
	for (int y = 0; y < tessOuterVertSize; y++) {
		for (int x = 0; x < tessOuterVertSize; x++)
		{
			int xx = x / 2;
			int yy = y / 2;

			// special case for outer rim...
			if (x == tessOuterVertSize - 1 || y == tessOuterVertSize - 1) {
				meshHeightsTess.Add(meshHeightsNoTess[xx + yy * outerVertSize]);
				meshColors.Add(meshColorsNoTess[xx + yy * outerVertSize]);
				continue;
			}

			// Use if-then instead of linear interpolation for speed
			float height00 = meshHeightsNoTess[xx + yy * outerVertSize];
			FLinearColor color00 = meshColorsNoTess[xx + yy * outerVertSize];

			if (x % 2 == 0) {
				if (y % 2 == 0) {
					meshHeightsTess.Add(height00);
					meshColors.Add(color00);
				}
				else {
					// Lerp on y
					meshHeightsTess.Add(0.5f * (height00 + meshHeightsNoTess[xx + (yy + 1) * outerVertSize]));
					meshColors.Add(0.5f * (color00 + meshColorsNoTess[xx + (yy + 1) * outerVertSize]));
				}
			}
			else {
				if (y % 2 == 0) {
					// Lerp on x
					meshHeightsTess.Add(0.5f *(height00 + meshHeightsNoTess[(xx + 1) + yy * outerVertSize]));
					meshColors.Add(0.5f *(color00 + meshColorsNoTess[(xx + 1) + yy * outerVertSize]));
				}
				else {
					// Lerp on x y
					meshHeightsTess.Add(0.25f * (height00 +
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
			float height = meshHeightsTess[x + y * tessOuterVertSize];
			FLinearColor color = meshColors[x + y * tessOuterVertSize];

			// Smooth where applicable
			if (x > 0 && y > 0 && x < (tessOuterVertSize - 1) && y < (tessOuterVertSize - 1))
			{
				height = height / 4;

				height += meshHeightsTess[(x - 1) + (y - 1) * tessOuterVertSize] / 16;
				height += meshHeightsTess[(x - 1) + (y + 1) * tessOuterVertSize] / 16;
				height += meshHeightsTess[(x + 1) + (y - 1) * tessOuterVertSize] / 16;
				height += meshHeightsTess[(x + 1) + (y + 1) * tessOuterVertSize] / 16;

				height += meshHeightsTess[x + (y - 1) * tessOuterVertSize] / 8;
				height += meshHeightsTess[x + (y + 1) * tessOuterVertSize] / 8;
				height += meshHeightsTess[(x - 1) + y * tessOuterVertSize] / 8;
				height += meshHeightsTess[(x + 1) + y * tessOuterVertSize] / 8;

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
	 * Holes
	 */
	
	for (size_t i = 0; i < holes.size(); i++)
	{
		WorldTile2 tile = holes[i];
		//WorldRegion2 tileRegion = tile.region();
		//if (region == tileRegion) {
		LocalTile2 localTile = tile.localTile(region);
		if (localTile.x >= -1 && localTile.y >= -1)
		{
			int32 tessTileX = localTile.x * tessMultiplier + tessMultiplier + 1;
			int32 tessTileY = localTile.y * tessMultiplier + tessMultiplier + 1;
			meshHeightsSmooth[tessTileX + tessTileY * tessOuterVertSize] = -CoordinateConstants::BelowWaterDisplayUnitHeight;
			meshHeightsSmooth[(tessTileX + 1) + tessTileY * tessOuterVertSize] = -CoordinateConstants::BelowWaterDisplayUnitHeight;
			meshHeightsSmooth[tessTileX + (tessTileY + 1) * tessOuterVertSize] = -CoordinateConstants::BelowWaterDisplayUnitHeight;
			meshHeightsSmooth[(tessTileX + 1) + (tessTileY + 1) * tessOuterVertSize] = -CoordinateConstants::BelowWaterDisplayUnitHeight;
		}
		//}
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
			//UV0.Add(FVector2D(float(x) / tessTileSize, float(y) / tessTileSize)); // use tile size because for UV (since UV is length, not vert)

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

	return 1;
}

void TerrainChunkData::UpdateTerrainChunkMesh_Prepare(GameSimulationCore& simulation, WorldRegion2 region, int tileDimX, int tileDimY,
													bool createMesh, bool& containsWater)
{
	LLM_SCOPE_(EPunSimLLMTag::PUN_DisplayTerrain);
	
	//! Meshes Init
	UpdateTerrainChunkMesh_Prepare1();
	

	// Use cache if possible
#if TRAILER_MODE

	if (verticesCache.Num() == 0) {
		verticesCache.SetNum(GameMapConstants::TotalRegions);
		vertexColorsCache.SetNum(GameMapConstants::TotalRegions);
		normalsCache.SetNum(GameMapConstants::TotalRegions);
		tangentsCache.SetNum(GameMapConstants::TotalRegions);
	}

	int32 regionId = region.regionId();
	if (verticesCache[regionId].Array.Num() > 0)
	{
		for (int32 i = 0; i < vertices.Num(); i++) {
			vertices[i] = verticesCache[regionId].Array[i];
			vertexColors[i] = vertexColorsCache[regionId].Array[i];
			normals[i] = normalsCache[regionId].Array[i];
			tangents[i] = tangentsCache[regionId].Array[i];
		}

		containsWater = true;
	}
	else
#endif
	{
		/*
		 * Note:
		 * - for TRAILER_MODE, we need these #if statement
		 * - for non TRAILER_MODE, UpdateTerrainChunkMesh_Prepare1/2/3 can be executed consecutively
		 */
		UpdateTerrainChunkMesh_Prepare2(simulation, region, containsWater);

		UpdateTerrainChunkMesh_Prepare3(region);

#if TRAILER_MODE
		static int32 count = 0;
		if (verticesCache[regionId].Array.Num() == 0)
		{
			count++;
			PUN_LOG("CACHE Vertices:%d", count);
			verticesCache[regionId].Array.SetNum(vertices.Num());
			vertexColorsCache[regionId].Array.SetNum(vertices.Num());
			normalsCache[regionId].Array.SetNum(vertices.Num());
			tangentsCache[regionId].Array.SetNum(vertices.Num());

			for (int32 i = 0; i < vertices.Num(); i++)
			{
				verticesCache[regionId].Array[i] = vertices[i];
				vertexColorsCache[regionId].Array[i] = vertexColors[i];
				normalsCache[regionId].Array[i] = normals[i];
				tangentsCache[regionId].Array[i] = tangents[i];
			}
		}
#endif
	}

	_lastHoleCount = holes.size();
}

void UTerrainChunkComponent::UpdateTerrainChunkMesh_UpdateMesh(bool createMesh, TerrainChunkData& terrainChunkData)
{
	//SCOPE_TIMER("Terrain Chunk Creation");

	TArray<FVector>& vertices = terrainChunkData.vertices;
	TArray<FLinearColor>& vertexColors = terrainChunkData.vertexColors;
	TArray<FVector>& normals = terrainChunkData.normals;
	TArray<FProcMeshTangent>& tangents = terrainChunkData.tangents;

	TArray<FVector2D>& UV0 = terrainChunkData.UV0;
	TArray<int32>& tris = terrainChunkData.tris;

	updatedTime = UGameplayStatics::GetTimeSeconds(this);

	if (createMesh) {
		CreateMeshSection_LinearColor(0, vertices, tris, normals, UV0, vertexColors, tangents, false);
	}
	else {
		UpdateMeshSection_LinearColor(0, vertices, normals, UV0, vertexColors, tangents);
	}
}