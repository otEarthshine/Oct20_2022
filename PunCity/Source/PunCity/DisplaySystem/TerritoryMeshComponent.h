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
		ProvinceSystem& provinceSys = sim->provinceSystem();
		
		SCOPE_TIMER_("TerritoryMesh Update flatTiles:%d", (playerIdIn != -1 ? playerIdIn : provinceSys.provinceFlatTileCount(provinceIdIn)));

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
		WorldTile2 centerTile;
		if (provinceIdIn != -1) {
			centerTile = provinceSys.GetProvinceCenter(provinceId).worldTile2();
		} else {
			centerTile = sim->townhall(playerId).centerTile();

			PUN_LOG("Update TerritoryMesh2 %d", playerId);
		}
		
		//const std::vector<WorldTile2>& edgeTiles = provinceSys.GetProvinceEdges(provinceId);
		const std::vector<WorldTile2x2>& edges1 = provinceIdIn != -1 ? provinceSys.GetProvinceEdges1(provinceId) : provinceSys.GetTerritoryEdges1(playerId);
		const std::vector<WorldTile2x2>& edges2 = provinceIdIn != -1 ? provinceSys.GetProvinceEdges2(provinceId) : provinceSys.GetTerritoryEdges2(playerId);

		//PUN_LOG("TerritoryMesh playerId:%d province:%d edges1:%d edges2:%d", playerIdIn, provinceIdIn, edges1.size(), edges2.size());
		
		size_t edges1Size = edges1.size();

		//const std::vector<int32>& _provinceId2x2 = provinceSys.GetProvinceId2x2Vec();

		/*
		 * Determine vertices
		 */
		TArray<FVector> outerVertices1;
		TArray<FVector> innerVertices1;

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
			FVector prevVec = getMidPoint((i - 1 + edges1Size) % edges1Size);
			FVector curVec = getMidPoint(i);
			FVector nextVec = getMidPoint((i + 1 + edges1Size) % edges1Size);
			
			FVector prevToCur = curVec - prevVec;
			FVector curToNext = nextVec - curVec;
			
			FVector bisectSource1 = -prevToCur.ToOrientationQuat().GetRightVector();
			FVector bisectSource2 = -curToNext.ToOrientationQuat().GetRightVector();
			
			FVector bisect = bisectSource1 * bisectSource2.Size() + bisectSource2 * bisectSource1.Size();
			bisect.Normalize();

			//WorldTile2 curWorldTile = edges1[i].worldTile2();

			outerVertices1.Add(curVec - bisect * outerBorderWidth);
			innerVertices1.Add(curVec + bisect * innerBorderWidth);
		}

		

		/*
		 * Break each edge point into two 
		 */
		TArray<FVector> outerVertices;
		TArray<FVector> innerVertices;

		int32 vertices1Num = outerVertices1.Num();
		
		if (vertices1Num > 5)
		{
			for (int32 i = 0; i < vertices1Num; i++)
			{
				int32 leftVecIndex = (i - 1 + vertices1Num) % vertices1Num;
				int32 rightVecIndex = (i + 1) % vertices1Num;
				
				FVector leftVec = outerVertices1[leftVecIndex];
				FVector midVec = outerVertices1[i];
				FVector rightVec = outerVertices1[rightVecIndex];

				FVector leftVecInner = innerVertices1[leftVecIndex];
				FVector midVecInner = innerVertices1[i];
				FVector rightVecInner = innerVertices1[rightVecIndex];

				FVector midToLeft = (leftVec - midVec).GetSafeNormal();
				FVector midToRight = (rightVec - midVec).GetSafeNormal();
				

				float dot = FVector::DotProduct(midToLeft, midToRight);
				// if this is corner is sharp enough, replace the midVec with 2 points instead
				if (dot > -0.8)
				{
					outerVertices.Add(midToLeft * 5.0f + midVec);
					outerVertices.Add(midToRight * 5.0f + midVec); // 5.0f for .5 tile moved

					innerVertices.Add((leftVecInner - midVecInner).GetSafeNormal() * 5.0f + midVecInner);
					innerVertices.Add((rightVecInner - midVecInner).GetSafeNormal() * 5.0f + midVecInner);
				}
				// not as sharp, put point in as is
				else if (dot > -0.99)
				{
					outerVertices.Add(midVec);
					innerVertices.Add(midVecInner);
				}
				// Straight line, don't add any point

				
				////
				//else if (dot < -0.8)
				//{
				//	
				//}
				//// if this is corner is sharp enough, replace the midVec with 2 points instead
				//else if (dot < -0.8)
				//{
				//	outerVertices[i] = (rightVec - midVec).GetSafeNormal() * 5.0f + midVec; // 5.0f for .5 tile moved
				//	outerVertices.Insert((leftVec - midVec).GetSafeNormal() * 5.0f + midVec, i);

				//	innerVertices[i] = (rightVecInner - midVecInner).GetSafeNormal() * 5.0f + midVecInner;
				//	innerVertices.Insert((leftVecInner - midVecInner).GetSafeNormal() * 5.0f + midVecInner, i);
				//}

				//leftVec = midVec;
				//leftVecInner = midVecInner;
			}
		}
		else
		{
			outerVertices = outerVertices1;
			innerVertices = innerVertices1;
		}

		/*
		 * Generate Mesh
		 */
		size_t outerVerticesSize = outerVertices.Num();
		
		for (size_t i = 0; i < outerVerticesSize; i++)
		{
			UV0.Add(FVector2D(0, 0));
			UV0.Add(FVector2D(0, 1));
			UV0.Add(FVector2D(1, 1));
			UV0.Add(FVector2D(1, 0));

			float pathColor = static_cast<float>(i) / outerVerticesSize;
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
			FVector lastOuter = outerVertices[(i - 1 + outerVerticesSize) % outerVerticesSize];
			FVector lastInner = innerVertices[(i - 1 + outerVerticesSize) % outerVerticesSize];

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
