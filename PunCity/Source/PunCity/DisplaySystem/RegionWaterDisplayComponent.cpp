// Pun Dumnernchanvanit's


#include "RegionWaterDisplayComponent.h"
#include "Engine/StaticMesh.h"
//
//int32 URegionWaterDisplayComponent::CreateNewDisplay(int objectId)
//{
//	int meshId = _waterMeshes.Num();
//	float centerShift = CoordinateConstants::RegionCenterDisplayUnits + 5;
//
//	UStaticMeshComponent* waterComp = CreateMeshComponent(this, "TerrainWater" + to_string(meshId));
//	waterComp->SetCollisionEnabled(ECollisionEnabled::NoCollision);
//	waterComp->SetStaticMesh(_assetLoader->WaterMesh);
//	UMaterialInstanceDynamic* waterMaterial = UMaterialInstanceDynamic::Create(_assetLoader->WaterMesh->GetMaterial(0), waterComp);
//	_waterMaterials.Add(waterMaterial);
//	waterComp->SetMaterial(0, waterMaterial);
//	waterComp->SetRelativeLocation(FVector(centerShift, centerShift, -8));
//	waterComp->SetReceivesDecals(false);
//	waterComp->SetCastShadow(false);
//	waterComp->bAffectDistanceFieldLighting = false;
//	waterComp->SetDistanceFieldSelfShadowBias(0);
//	_waterMeshes.Add(waterComp);
//
//	// Need separate water decal since decal needs opaque (to write to depth)
//	UStaticMeshComponent* waterDecalComp = CreateMeshComponent(this, "TerrainWaterDecal" + to_string(meshId));
//	waterDecalComp->SetCollisionEnabled(ECollisionEnabled::NoCollision);
//	waterDecalComp->SetStaticMesh(_assetLoader->WaterMesh);
//	waterDecalComp->SetMaterial(0, _assetLoader->SelectionMaterialGreen); // ...
//	waterDecalComp->SetRelativeLocation(FVector(centerShift, centerShift, -8));
//	waterDecalComp->SetReceivesDecals(true);
//	waterDecalComp->SetCastShadow(false);
//	waterDecalComp->bAffectDistanceFieldLighting = false;
//	waterDecalComp->SetDistanceFieldSelfShadowBias(0);
//	_waterDecalMeshes.Add(waterDecalComp);
//
//	return meshId;
//}
//
//void URegionWaterDisplayComponent::OnSpawnDisplay(int regionId, int meshId, WorldAtom2 cameraAtom)
//{
//	float centerShift = CoordinateConstants::RegionCenterDisplayUnits;
//
//	WorldRegion2 region(regionId);
//	bool containsWater = false; // if there is no water tile, don't enable waterMesh's visibility
//
//	// Water
//	_waterMeshes[meshId]->SetVisibility(containsWater);
//	_waterMaterials[meshId]->SetTextureParameterValue("HeightMap", _assetLoader->heightTexture);
//	_waterMaterials[meshId]->SetTextureParameterValue("BiomeMap", _assetLoader->biomeTexture);
//}
//
//void URegionWaterDisplayComponent::UpdateDisplay(int regionId, int meshId, WorldAtom2 cameraAtom)
//{
//	WorldRegion2 region(regionId);
//	WorldAtom2 objectAtomLocation(region.x * CoordinateConstants::AtomsPerRegion,
//		region.y * CoordinateConstants::AtomsPerRegion);
//
//	FVector displayLocation = MapUtil::DisplayLocation(cameraAtom, objectAtomLocation);
//
//	OverlayType overlayType = gameManager()->GetOverlayType();
//
//	// PUN_LOG("region %d, %d ... %s", region.x, region.y, *displayLocation.ToString());
//
//	_waterDecalMeshes[meshId]->SetVisibility(overlayType == OverlayType::Fish);
//
//	_terrainChunks[meshId]->SetRelativeLocation(displayLocation);
//
//	_groundColliderMeshes[meshId]->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
//
//	// Update the terrain chunk if needed
//	if (simulation().NeedDisplayUpdate(DisplayClusterEnum::Terrain, regionId))
//	{
//		//SCOPE_TIMER("Tick Region Terrain");
//
//		bool containsWater;
//		_terrainChunks[meshId]->UpdateTerrainChunkMesh(simulation(), region,
//			GameMapConstants::TilesPerWorldX, GameMapConstants::TilesPerWorldY, false,
//			containsWater);
//		simulation().SetNeedDisplayUpdate(DisplayClusterEnum::Terrain, regionId, false);
//	}
//
//}
//
//void URegionWaterDisplayComponent::HideDisplay(int meshId)
//{
//	_waterMeshes[meshId]->SetVisibility(false);
//	_waterDecalMeshes[meshId]->SetVisibility(false);
//}