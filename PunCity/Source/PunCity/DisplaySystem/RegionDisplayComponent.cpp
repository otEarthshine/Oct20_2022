#include "RegionDisplayComponent.h"

#include "Components/LineBatchComponent.h"
#include "Components/TextRenderComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/StaticMesh.h"
#include "PunTerrainGenerator.h"
#include "PunCity/PunGameSettings.h"
#include "PunCity/PunTimer.h"

#include <string>

using namespace std;

URegionDisplayComponent::URegionDisplayComponent()
{
	PrimaryComponentTick.bCanEverTick = false;
	_maxSpawnPerTick = 3;

#if TRAILER_MODE
	UTerrainChunkComponent::ResetCache();
#endif
}

float URegionDisplayComponent::GetTerrainDisplayHeight(WorldTile2 tile)
{
	int32 meshId = GetMeshId(tile.regionId());
	if (meshId == -1) {
		return 0;
	}
	return _terrainChunkData[meshId].GetTerrainDisplayHeight(tile.localTile()); //_terrainChunks[meshId]->meshHeightsSmooth[/*(localTile.x * 2 + 1) +*/ (localTile.y * 2 + 1) * sizeX];
}

int URegionDisplayComponent::CreateNewDisplay(int objectId)
{
	LLM_SCOPE_(EPunSimLLMTag::PUN_DisplayTerrain);
	
	int meshId = _terrainChunks.Num();
	float centerShift = CoordinateConstants::RegionCenterDisplayUnits + 5;
	WorldRegion2 region(objectId);

	UTerrainChunkComponent* terrainComp = NewObject<UTerrainChunkComponent>(this);
	terrainComp->Rename(*FString(("TerrainChunk" + to_string(meshId)).c_str()));
	terrainComp->AttachToComponent(this, FAttachmentTransformRules::KeepRelativeTransform, NAME_None);
	terrainComp->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	terrainComp->SetGenerateOverlapEvents(false);
	terrainComp->bAffectDistanceFieldLighting = false;
	terrainComp->SetReceivesDecals(true);
	terrainComp->RegisterComponent();
	terrainComp->SetTerrainMaterial(_assetLoader); // One material for all...
	
	_terrainChunks.Add(terrainComp);
	_terrainChunkData.Add(TerrainChunkData());

	//bool containsWater = false; // if there is no water tile, don't enable waterMesh's visibility
	//_terrainChunkData[meshId].UpdateTerrainChunkMesh_Prepare(simulation(), region,
	//						GameMapConstants::TilesPerWorldX, GameMapConstants::TilesPerWorldY, true, containsWater);
	//_terrainChunks[meshId]->UpdateTerrainChunkMesh_UpdateMesh(true, _terrainChunkData[meshId]);
	chunkInfosToUpDate.push_back({ meshId, objectId, true });

	UStaticMeshComponent* waterComp = CreateMeshComponent(terrainComp, "TerrainWater" + to_string(meshId));
	waterComp->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	waterComp->SetStaticMesh(_assetLoader->WaterMesh);
	UMaterialInstanceDynamic* waterMaterial = UMaterialInstanceDynamic::Create(_assetLoader->WaterMesh->GetMaterial(0), waterComp);
	_waterMaterials.Add(waterMaterial);
	waterComp->SetMaterial(0, waterMaterial);
	waterComp->SetRelativeLocation(FVector(centerShift, centerShift, -8));
	waterComp->SetReceivesDecals(false);
	waterComp->SetCastShadow(false);
	waterComp->bAffectDistanceFieldLighting = false;
	waterComp->SetDistanceFieldSelfShadowBias(0);
	_waterMeshes.Add(waterComp);

	// Need separate water decal since decal needs opaque (to write to depth)
	UStaticMeshComponent* waterDecalComp = CreateMeshComponent(terrainComp, "TerrainWaterDecal" + to_string(meshId));
	waterDecalComp->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	waterDecalComp->SetStaticMesh(_assetLoader->WaterMesh);
	waterDecalComp->SetMaterial(0, _assetLoader->SelectionMaterialGreen); // ...
	waterDecalComp->SetRelativeLocation(FVector(centerShift, centerShift, -8));
	waterDecalComp->SetReceivesDecals(true);
	waterDecalComp->SetCastShadow(false);
	waterDecalComp->bAffectDistanceFieldLighting = false;
	waterDecalComp->SetDistanceFieldSelfShadowBias(0);
	_waterDecalMeshes.Add(waterDecalComp);

	// GroundCollision
	UStaticMeshComponent* groundColliderMesh = CreateMeshComponent(terrainComp, "GroundCollider" + to_string(meshId));
	groundColliderMesh->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
	groundColliderMesh->SetStaticMesh(_assetLoader->WaterMesh);
	groundColliderMesh->SetRelativeLocation(FVector(centerShift, centerShift, 0));
	groundColliderMesh->SetReceivesDecals(false);
	groundColliderMesh->SetVisibility(false);
	groundColliderMesh->SetCastShadow(false);
	groundColliderMesh->bAffectDistanceFieldLighting = false;
	groundColliderMesh->SetDistanceFieldSelfShadowBias(0);
	groundColliderMesh->SetCollisionObjectType(ECC_Pawn);
	_groundColliderMeshes.Add(groundColliderMesh);

	return meshId;
}

void URegionDisplayComponent::OnSpawnDisplay(int regionId, int meshId, WorldAtom2 cameraAtom)
{
	LLM_SCOPE_(EPunSimLLMTag::PUN_DisplayTerrain);
	
	float centerShift = CoordinateConstants::RegionCenterDisplayUnits;

	WorldRegion2 region(regionId);
	bool containsWater = false; // if there is no water tile, don't enable waterMesh's visibility

	// Terrain
	{
		//SCOPE_TIMER("Region - Update Terrain Chunk");

		//PUN_LOG("OnSpawnDisplay regionId:%d meshId:%d", regionId, meshId);
		
		//_terrainChunkData[meshId].UpdateTerrainChunkMesh_Prepare1();
		//_terrainChunkData[meshId].UpdateTerrainChunkMesh_Prepare2(simulation(), region, containsWater);
		//_terrainChunkData[meshId].UpdateTerrainChunkMesh_Prepare3(region);
		//_terrainChunks[meshId]->UpdateTerrainChunkMesh_UpdateMesh(false, _terrainChunkData[meshId]);
		
		//_terrainChunks[meshId]->SetTerrainMaterial(_assetLoader); // One material for all...

		_terrainChunks[meshId]->SetVisibility(true);
	}

	{
		//SCOPE_TIMER("Region - Others");
		
		// Ground Collider
		_groundColliderMeshes[meshId]->ComponentTags.Empty();
		_groundColliderMeshes[meshId]->ComponentTags.Add("GroundCollider");
		_groundColliderMeshes[meshId]->ComponentTags.Add(FName(*FString::FromInt(region.x)));
		_groundColliderMeshes[meshId]->ComponentTags.Add(FName(*FString::FromInt(region.y)));

		// Water
		//_waterMeshes[meshId]->SetVisibility(containsWater);
		_waterMaterials[meshId]->SetTextureParameterValue("HeightMap", _assetLoader->heightTexture);
		_waterMaterials[meshId]->SetTextureParameterValue("BiomeMap", _assetLoader->biomeTexture);
		//_waterMaterials[meshId]->SetTextureParameterValue(TEXT("RiverMap"), _assetLoader->riverTexture);

	}
}

void URegionDisplayComponent::UpdateDisplay(int regionId, int meshId, WorldAtom2 cameraAtom, bool justSpawned, bool justCreated)
{
	LLM_SCOPE_(EPunSimLLMTag::PUN_DisplayTerrain);
	
	WorldRegion2 region(regionId);
	WorldAtom2 objectAtomLocation(region.x * CoordinateConstants::AtomsPerRegion,
		region.y * CoordinateConstants::AtomsPerRegion);

	FVector displayLocation = MapUtil::DisplayLocation(cameraAtom, objectAtomLocation);

	OverlayType overlayType = gameManager()->GetOverlayType();

	// PUN_LOG("region %d, %d ... %s", region.x, region.y, *displayLocation.ToString());

	_waterDecalMeshes[meshId]->SetVisibility(overlayType == OverlayType::Fish);

	_terrainChunks[meshId]->SetRelativeLocation(displayLocation);

	_groundColliderMeshes[meshId]->SetCollisionEnabled(ECollisionEnabled::QueryOnly);

	// Update the terrain chunk if needed
	if (simulation().NeedDisplayUpdate(DisplayClusterEnum::Terrain, regionId) ||
		justSpawned)
	{
		if (!justCreated) {
			chunkInfosToUpDate.push_back({ meshId, regionId, false });
		}
		
		simulation().SetNeedDisplayUpdate(DisplayClusterEnum::Terrain, regionId, false);
	}

	// Overlap Terrain should have its beach pushed down to hide seam...
	_terrainChunks[meshId]->MaterialInstance->SetScalarParameterValue("IsOverlapTerrain", _isOverlapTerrain);

	if (isMainMenuDisplay) {
		_terrainChunks[meshId]->MaterialInstance->SetScalarParameterValue("IsForestOnly", true);
	}

	
	// Debug: Set cheap material if needed
	if (PunSettings::IsOn("PlainTerrain") != _terrainChunks[meshId]->bIsPlainMaterial) {
		_terrainChunks[meshId]->SetTerrainMaterial(_assetLoader, PunSettings::IsOn("PlainTerrain"));
	}
	if (PunSettings::IsOn("HideWater")) {
		_waterMeshes[meshId]->SetVisibility(false);
	}
	
}

void URegionDisplayComponent::AfterAdd()
{
	SCOPE_TIMER_FILTER(5000, "Tick Region AfterDisplay");

	// Execute all the needed
	{
		//SCOPE_TIMER_FILTER(1000, "Tick Region Test Prepare1");
		for (MeshChunkInfo& chunkInfo : chunkInfosToUpDate) {
			_terrainChunkData[chunkInfo.meshId].UpdateTerrainChunkMesh_Prepare1();
		}
	}

	//{
	//	SCOPE_TIMER_FILTER(1000, "Tick Region Test Prepare2");
	//	for (MeshChunkInfo& chunkInfo : chunkInfosToUpDate)
	//	{
	//		bool containsWater;
	//		_terrainChunkData[chunkInfo.meshId].UpdateTerrainChunkMesh_Prepare2(simulation(), chunkInfo.region(), containsWater);
	//		_waterMeshes[chunkInfo.meshId]->SetVisibility(containsWater);
	//	}
	//}

	{
		SCOPE_TIMER_FILTER(5000, "Tick Region Test Prepare3");

		//if (PunSettings::Get("TerrainThread") == 0)
		//{
		//	for (MeshChunkInfo& chunkInfo : chunkInfosToUpDate)
		//	{
		//		bool containsWater;
		//		_terrainChunkData[chunkInfo.meshId].UpdateTerrainChunkMesh_Prepare2(simulation(), chunkInfo.region(), containsWater);
		//		_waterMeshes[chunkInfo.meshId]->SetVisibility(containsWater);

		//		_terrainChunkData[chunkInfo.meshId].UpdateTerrainChunkMesh_Prepare3(chunkInfo.region());
		//	}
		//}
		//else
		//{
			ThreadedRun(chunkInfosToUpDate, 8, [&](MeshChunkInfo& chunkInfo)
			{
				bool containsWater;
				_terrainChunkData[chunkInfo.meshId].UpdateTerrainChunkMesh_Prepare2(simulation(), chunkInfo.region(), containsWater);
				_waterMeshes[chunkInfo.meshId]->SetVisibility(containsWater);

				_terrainChunkData[chunkInfo.meshId].UpdateTerrainChunkMesh_Prepare3(chunkInfo.region());
			});
		//}
	}


	

	{
		//SCOPE_TIMER_FILTER(1000, "Tick Region AfterDisplay UpdateMesh");

		for (MeshChunkInfo& chunkInfo : chunkInfosToUpDate) {
			_terrainChunks[chunkInfo.meshId]->UpdateTerrainChunkMesh_UpdateMesh(chunkInfo.createMesh, _terrainChunkData[chunkInfo.meshId]);
		}
	}
}

void URegionDisplayComponent::HideDisplay(int meshId, int32 regionId)
{
	LLM_SCOPE_(EPunSimLLMTag::PUN_DisplayTerrain);
	
	_waterMeshes[meshId]->SetVisibility(false);
	_waterDecalMeshes[meshId]->SetVisibility(false);
	
	_terrainChunks[meshId]->SetVisibility(false);

	_groundColliderMeshes[meshId]->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	_groundColliderMeshes[meshId]->ComponentTags.Empty();
}