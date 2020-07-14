// Pun Dumnernchanvanit's

#pragma once

#include "PunCity/Simulation//GameSimulationCore.h"
#include "ProceduralMeshComponent.h"
#include "Materials/MaterialInstanceDynamic.h"


#include "TerritoryMeshComponent.generated.h"

/**
 * 
 */
UCLASS()
class PROTOTYPECITY_API UTerritoryMeshComponent : public UProceduralMeshComponent
{
	GENERATED_BODY()
public:

	void UpdateMesh(bool createMesh, int32 provinceIdIn, int32 playerIdIn, bool isInnerMeshIn, 
					IGameSimulationCore* sim, int32 innerBorderWidth = 10)
	{
		SCOPE_TIMER("TerritoryMesh Update");

		ProvinceSystem& provinceSys = sim->provinceSystem();
		PunTerrainGenerator& terrainGen = sim->terrainGenerator();
		
		int32 outerBorderWidth = 0;
		TranslucencySortPriority = 99999;
		
		if (isInnerMeshIn) {
			TranslucencySortPriority++;
			outerBorderWidth = -5;
		}
		
		static TArray<FVector2D> UV0;
		static TArray<FLinearColor> vertexColors;
		static TArray<FVector> normals;
		static TArray<FProcMeshTangent> tangents;

		TArray<FVector> vertices;
		static TArray<int32> tris;

		UV0.Empty();
		vertexColors.Empty();
		normals.Empty();
		tangents.Empty();
		vertices.Empty();
		tris.Empty();

		provinceId = provinceIdIn;
		playerId = playerIdIn;
		
		isInnerMesh = isInnerMeshIn;
		WorldTile2 centerTile = provinceIdIn != -1 ? provinceSys.GetProvinceCenter(provinceId).worldTile2() : sim->townhall(playerId).centerTile();

		//const std::vector<WorldTile2>& edgeTiles = provinceSys.GetProvinceEdges(provinceId);
		const std::vector<WorldTile2x2>& edges1 = provinceIdIn != -1 ? provinceSys.GetProvinceEdges1(provinceId) : provinceSys.GetTerritoryEdges1(playerId);
		const std::vector<WorldTile2x2>& edges2 = provinceIdIn != -1 ? provinceSys.GetProvinceEdges2(provinceId) : provinceSys.GetTerritoryEdges2(playerId);

		//PUN_LOG("TerritoryMesh playerId:%d province:%d edges1:%d edges2:%d", playerIdIn, provinceIdIn, edges1.size(), edges2.size());
		
		size_t borderSize = edges1.size();

		//const std::vector<int32>& _provinceId2x2 = provinceSys.GetProvinceId2x2Vec();

		/*
		 * Determine vertices
		 */
		TArray<FVector> outerVertices;
		TArray<FVector> innerVertices;

		auto getHeight = [&](const WorldTile2& tile)
		{
			float height = terrainGen.GetTerrainDisplayHeight(tile);
			height += fmin(height, 15); // Enhance upward
			height = fmax(height, 5); // Slightly above ground...
			return height;
		};

		auto getMidPoint = [&](size_t i) -> FVector
		{
			WorldTile2 curWorldTilePreShift1 = edges1[i].worldTile2();
			WorldTile2 curWorldTilePreShift2 = edges2[i].worldTile2();
			WorldTile2 curWorldTile1 = curWorldTilePreShift1 - centerTile;
			WorldTile2 curWorldTile2 = curWorldTilePreShift2 - centerTile;

			// +5 is to shift curVec to center of 2x2 tile
			FVector curVec1(curWorldTile1.x * 10 + 5, curWorldTile1.y * 10 + 5, getHeight(curWorldTilePreShift1));
			FVector curVec2(curWorldTile2.x * 10 + 5, curWorldTile2.y * 10 + 5, getHeight(curWorldTilePreShift2));

			return (curVec1 + curVec2) / 2;
		};
		
		for (size_t i = 0; i < edges1.size(); i++)
		{
			FVector prevVec = getMidPoint((i - 1 + borderSize) % borderSize);
			FVector curVec = getMidPoint(i);
			FVector nextVec = getMidPoint((i + 1 + borderSize) % borderSize);
			
			FVector prevToCur = curVec - prevVec;
			FVector curToNext = nextVec - curVec;
			
			FVector bisectSource1 = -prevToCur.ToOrientationQuat().GetRightVector();
			FVector bisectSource2 = -curToNext.ToOrientationQuat().GetRightVector();
			
			FVector bisect = bisectSource1 * bisectSource2.Size() + bisectSource2 * bisectSource1.Size();
			bisect.Normalize();

			WorldTile2 curWorldTile = edges1[i].worldTile2();

			outerVertices.Add(curVec - bisect * outerBorderWidth);
			innerVertices.Add(curVec + bisect * innerBorderWidth);
		}
		
		for (size_t i = 0; i < edges1.size(); i++)
		{	
			UV0.Add(FVector2D(0, 0));
			UV0.Add(FVector2D(0, 1));
			UV0.Add(FVector2D(1, 1));
			UV0.Add(FVector2D(1, 0));

			float pathColor = static_cast<float>(i) / borderSize;
			vertexColors.Add(FLinearColor::White * pathColor);
			vertexColors.Add(FLinearColor::White * pathColor);
			vertexColors.Add(FLinearColor::White * pathColor);
			vertexColors.Add(FLinearColor::White * pathColor);

			normals.Add(FVector::UpVector);
			normals.Add(FVector::UpVector);
			normals.Add(FVector::UpVector);
			normals.Add(FVector::UpVector);

			tangents.Add(FProcMeshTangent());
			tangents.Add(FProcMeshTangent());
			tangents.Add(FProcMeshTangent());
			tangents.Add(FProcMeshTangent());

			/*
			 * Tris arrangement right-hand
			 */
			FVector curOuter = outerVertices[i];
			FVector curInner = innerVertices[i];
			FVector lastOuter = outerVertices[(i - 1 + borderSize) % borderSize];
			FVector lastInner = innerVertices[(i - 1 + borderSize) % borderSize];

			vertices.Add(lastInner);
			vertices.Add(lastOuter);
			vertices.Add(curOuter);
			vertices.Add(curInner);

			int32 trisStartIndex = 4 * i;
			tris.Add(0 + trisStartIndex);
			tris.Add(1 + trisStartIndex);
			tris.Add(2 + trisStartIndex);
			tris.Add(0 + trisStartIndex);
			tris.Add(2 + trisStartIndex);
			tris.Add(3 + trisStartIndex);
		}
		
		
		{
			//SCOPE_TIMER("Territory Mesh Creation");

			if (createMesh) {
				CreateMeshSection_LinearColor(0, vertices, tris, normals, UV0, vertexColors, tangents, false);
			}
			else {
				UpdateMeshSection_LinearColor(0, vertices, normals, UV0, vertexColors, tangents);
			}
		}
	}

	void SetTerritoryMaterial(UMaterial* protoMaterial, UMaterial* protoMaterial_Top)
	{
		if (!MaterialInstance) {
			MaterialInstance = UMaterialInstanceDynamic::Create(protoMaterial, this);
		}
		if (!MaterialInstance_Top) {
			MaterialInstance_Top = UMaterialInstanceDynamic::Create(protoMaterial_Top, this);
		}
		SetMaterial(0, MaterialInstance);
	}

	UPROPERTY() UMaterialInstanceDynamic* MaterialInstance = nullptr;
	UPROPERTY() UMaterialInstanceDynamic* MaterialInstance_Top = nullptr;
	
	int32 provinceId = -1;
	int32 playerId = -1; // Used for territory mesh
	int32 isInnerMesh = false;
};
