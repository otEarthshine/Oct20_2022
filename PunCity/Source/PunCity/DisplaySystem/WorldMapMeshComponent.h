// Pun Dumnernchanvanit's

#pragma once

#include "CoreMinimal.h"
#include "PunCity/DisplaySystem/DisplaySystemComponent.h"
#include "TerrainLargeChunkComponent.h"
#include "TerritoryMeshComponent.h"
#include "WorldMapMeshComponent.generated.h"

/**
 * 
 */
UCLASS()
class PROTOTYPECITY_API UWorldMapMeshComponent : public UDisplaySystemComponent
{
	GENERATED_BODY()
public:
	UWorldMapMeshComponent()
	{
		_terrainChunkParent = CreateDefaultSubobject<USceneComponent>("TerrainParent");
		_worldMapCollider = CreateDefaultSubobject<UStaticMeshComponent>("WorldMapMesh1");
	}
	
	void Init(int size, TScriptInterface<IDisplaySystemDataSource> gameManager, UAssetLoaderComponent* assetLoader) override
	{
		UDisplaySystemComponent::Init(size, gameManager, assetLoader);
		LLM_SCOPE_(EPunSimLLMTag::PUN_DisplayTerrain);

		_worldMapCollider->AttachToComponent(_terrainChunkParent, FAttachmentTransformRules::KeepRelativeTransform);
		_worldMapCollider->SetReceivesDecals(false);
		_worldMapCollider->SetCastShadow(false);
		_worldMapCollider->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
		_worldMapCollider->SetActive(true);
		_worldMapCollider->SetVisibility(false);
		_worldMapCollider->SetStaticMesh(assetLoader->WorldMapMesh);
		_worldMapCollider->ComponentTags.Empty();
		_worldMapCollider->ComponentTags.Add("WorldMap");

		FVector mapScale;
		MapSizeEnum mapSizeEnum = gameManager->simulation().mapSizeEnum();
		if (mapSizeEnum == MapSizeEnum::Large) {
			mapScale = FVector(32, 32, 32);
		}
		else if (mapSizeEnum == MapSizeEnum::Medium) {
			mapScale = FVector(16, 16, 16);
		}
		else if (mapSizeEnum == MapSizeEnum::Small) {
			mapScale = FVector(8, 8, 8);
		}
		else {
			UE_DEBUG_BREAK();
			mapScale = FVector(32, 32, 32);
		}
		_worldMapCollider->SetWorldScale3D(mapScale);

		lastVisible = false;

		Init2();
	}

	void Init2()
	{
		SCOPE_TIMER("Init UWorldMapMeshComponent")
		
		int32 tileSkipInt = 2;
		float displayUnitPerTile = CoordinateConstants::DisplayUnitPerTile * tileSkipInt;
		int32 tileSize = CoordinateConstants::TilesPerRegion * 4;
		int32 chunkRegionSize = tileSkipInt * 4;
		float chunkDisplaySize = tileSize * displayUnitPerTile;

		int32 totalChunksX = GameMapConstants::TilesPerWorldX / tileSize / tileSkipInt;
		int32 totalChunksY = GameMapConstants::TilesPerWorldY / tileSize / tileSkipInt;

		PUN_LOG("UWorldMapMeshComponent Init2 %d, %d, %d, %f", totalChunksX, totalChunksY, chunkRegionSize, chunkDisplaySize);

		for (int32 y = 0; y < totalChunksY; y++) {
			for (int32 x = 0; x < totalChunksX; x++)
			{
				auto terrainComp = NewObject<UTerrainLargeChunkComponent>(this);
				terrainComp->Rename(*FString(("WorldMapMesh" + to_string(x) + "_" + to_string(y)).c_str()));
				terrainComp->AttachToComponent(_terrainChunkParent, FAttachmentTransformRules::KeepRelativeTransform, NAME_None);
				terrainComp->SetCollisionEnabled(ECollisionEnabled::NoCollision);
				terrainComp->SetGenerateOverlapEvents(false);
				terrainComp->bAffectDistanceFieldLighting = false;
				terrainComp->RegisterComponent();
				terrainComp->Init(tileSize, displayUnitPerTile, tileSkipInt, false);
				terrainComp->SetRelativeLocation(FVector(x * chunkDisplaySize, y * chunkDisplaySize, 0));

				terrainComp->UpdateTerrainChunkMesh(simulation(), WorldRegion2(x * chunkRegionSize, y *chunkRegionSize), true);
				terrainComp->SetMaterial(0, _assetLoader->M_WorldMap);

				terrainComp->SetCastShadow(false);
				terrainComp->SetReceivesDecals(true);

				_terrainChunks.Add(terrainComp);
			}
		}
	}

	void InitProvinceMesh()
	{
		auto& provinceSys = simulation().provinceSystem();
		
		for (int32 i = 0; i < GameMapConstants::TotalRegions; i++)
		{
			if (simulation().IsProvinceValid(i)) 
			{
				auto comp = NewObject<UTerritoryMeshComponent>(this);
				comp->Rename(*FString(("ProvinceMesh_Map" + to_string(i)).c_str()));
				comp->AttachToComponent(_terrainChunkParent, FAttachmentTransformRules::KeepRelativeTransform, NAME_None);
				comp->SetCollisionEnabled(ECollisionEnabled::NoCollision);
				comp->SetGenerateOverlapEvents(false);
				comp->bAffectDistanceFieldLighting = false;
				comp->RegisterComponent();

				WorldTile2 provinceCenter = provinceSys.GetProvinceCenter(i).worldTile2();
				comp->SetRelativeLocation(FVector(provinceCenter.x * CoordinateConstants::DisplayUnitPerTile, 
													provinceCenter.y * CoordinateConstants::DisplayUnitPerTile, 0));

				//comp->UpdateMesh(true, i, );
				comp->SetTerritoryMaterial(_assetLoader->M_Province, _assetLoader->M_Province_Top);

				comp->SetCastShadow(false);
				comp->SetReceivesDecals(true);

				//_terrainChunks.Add(terrainComp);
				//_provinceChunks
			}
		}
	}


	void Update(bool mapVisible, bool colliderVisible)
	{
		LLM_SCOPE_(EPunSimLLMTag::PUN_DisplayTerrain);
		
		WorldAtom2 cameraAtom = gameManager()->cameraAtom();
		FVector displayLocation = MapUtil::DisplayLocation(cameraAtom, WorldAtom2(0, 0));
		_terrainChunkParent->SetRelativeLocation(displayLocation);

		//// PUN_LOG("region %d, %d ... %s", region.x, region.y, *displayLocation.ToString());

		if (lastVisible != mapVisible) {
			lastVisible = mapVisible;
			
			_terrainChunkParent->SetVisibility(mapVisible);
			for (auto chunk : _terrainChunks)
			{
				chunk->SetVisibility(mapVisible);
				chunk->SetActive(mapVisible);
			}
		}

		
		_worldMapCollider->SetCollisionEnabled(colliderVisible ? ECollisionEnabled::QueryOnly : ECollisionEnabled::NoCollision);
		_worldMapCollider->SetActive(colliderVisible);
	}

	bool isInitialized() {
		return _terrainChunks.Num();
	}

private:
	bool lastVisible = false;
	
	UPROPERTY() USceneComponent* _terrainChunkParent;
	UPROPERTY() TArray<UTerrainLargeChunkComponent*> _terrainChunks;
	UPROPERTY() TArray<UTerritoryMeshComponent*> _provinceChunks;

	UPROPERTY() UStaticMeshComponent* _worldMapCollider;
};
