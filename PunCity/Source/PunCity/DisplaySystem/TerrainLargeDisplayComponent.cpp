// Pun Dumnernchanvanit's


#include "TerrainLargeDisplayComponent.h"
#include "TerrainLargeChunkComponent.h"


int UTerrainLargeDisplayComponent::CreateNewDisplay(int objectId)
{
	LLM_SCOPE_(EPunSimLLMTag::PUN_DisplayTerrain);

	int meshId = _terrainChunks.Num();
	float centerShift = CoordinateConstants::RegionCenterDisplayUnits + 5;
	WorldRegion2 region(objectId);

	UTerrainLargeChunkComponent* terrainComp = NewObject<UTerrainLargeChunkComponent>(this);
	terrainComp->Rename(*FString(("TerrainChunk" + to_string(meshId)).c_str()));
	terrainComp->AttachToComponent(this, FAttachmentTransformRules::KeepRelativeTransform, NAME_None);
	terrainComp->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	terrainComp->SetGenerateOverlapEvents(false);
	terrainComp->bAffectDistanceFieldLighting = false;
	terrainComp->SetReceivesDecals(true);
	terrainComp->RegisterComponent();
	terrainComp->Init(CoordinateConstants::TilesPerRegion * 4, CoordinateConstants::DisplayUnitPerTile);
	_terrainChunks.Add(terrainComp);

	bool containsWater = false; // if there is no water tile, don't enable waterMesh's visibility
	_terrainChunks[meshId]->UpdateTerrainChunkMesh(simulation(), region, true);


	return meshId;
}

void UTerrainLargeDisplayComponent::OnSpawnDisplay(int regionId, int meshId, WorldAtom2 cameraAtom)
{
	LLM_SCOPE_(EPunSimLLMTag::PUN_DisplayTerrain);

	float centerShift = CoordinateConstants::RegionCenterDisplayUnits;

	WorldRegion2 region(regionId);
	bool containsWater = false; // if there is no water tile, don't enable waterMesh's visibility

	// Terrain
	{
		//SCOPE_TIMER("Region - Update Terrain Chunk");
		_terrainChunks[meshId]->alreadyUpdatedMesh = false;
		//_terrainChunks[meshId]->UpdateTerrainChunkMesh(simulation(), WorldRegion2(regionId),
		//	GameMapConstants::TilesPerWorldX, GameMapConstants::TilesPerWorldY, false,
		//	containsWater);
		_terrainChunks[meshId]->SetTerrainMaterial(_assetLoader); // One material for all...

		_terrainChunks[meshId]->SetVisibility(true);
	}
}

void UTerrainLargeDisplayComponent::UpdateDisplay(int regionId, int meshId, WorldAtom2 cameraAtom)
{
	LLM_SCOPE_(EPunSimLLMTag::PUN_DisplayTerrain);

	WorldRegion2 region(regionId);
	WorldAtom2 objectAtomLocation(region.x * CoordinateConstants::AtomsPerRegion,
		region.y * CoordinateConstants::AtomsPerRegion);

	FVector displayLocation = MapUtil::DisplayLocation(cameraAtom, objectAtomLocation);

	// PUN_LOG("region %d, %d ... %s", region.x, region.y, *displayLocation.ToString());

	_terrainChunks[meshId]->SetRelativeLocation(displayLocation);

	// Update the terrain chunk if needed
	if (!_terrainChunks[meshId]->alreadyUpdatedMesh)
	{
		//SCOPE_TIMER("Tick Region Terrain");

		_terrainChunks[meshId]->UpdateTerrainChunkMesh(simulation(), region, false);

		_terrainChunks[meshId]->alreadyUpdatedMesh = true;
	}

	// Overlap Terrain should have its beach pushed down to hide seam...
	//_terrainChunks[meshId]->MaterialInstance->SetScalarParameterValue("IsOverlapTerrain", _isOverlapTerrain);

	//if (isMainMenuDisplay) {
	//	_terrainChunks[meshId]->MaterialInstance->SetScalarParameterValue("IsForestOnly", true);
	//}


	// Debug: Set cheap material if needed
	if (PunSettings::IsOn("PlainTerrain") != _terrainChunks[meshId]->bIsPlainMaterial) {
		_terrainChunks[meshId]->SetTerrainMaterial(_assetLoader, PunSettings::IsOn("PlainTerrain"));
	}
}

void UTerrainLargeDisplayComponent::HideDisplay(int meshId)
{
	LLM_SCOPE_(EPunSimLLMTag::PUN_DisplayTerrain);

	_terrainChunks[meshId]->SetVisibility(false);
}