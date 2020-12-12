// Pun Dumnernchanvanit's


#include "TerrainLargeDisplayComponent.h"
#include "TerrainLargeChunkComponent.h"


int UTerrainLargeDisplayComponent::CreateNewDisplay(int objectId)
{
	LLM_SCOPE_(EPunSimLLMTag::PUN_DisplayTerrain);

	int meshId = _terrainLargeChunks.Num();
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
	_terrainLargeChunks.Add(terrainComp);

	bool containsWater = false; // if there is no water tile, don't enable waterMesh's visibility
	//_terrainLargeChunks[meshId]->UpdateLargeTerrainChunkMesh(simulation(), region, true);
	chunkInfosToUpdate.push_back({ meshId, objectId, true });

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
		_terrainLargeChunks[meshId]->alreadyUpdatedMesh = false;
		_terrainLargeChunks[meshId]->SetTerrainMaterial(_assetLoader); // One material for all...

		_terrainLargeChunks[meshId]->SetVisibility(true);
	}
}

void UTerrainLargeDisplayComponent::UpdateDisplay(int regionId, int meshId, WorldAtom2 cameraAtom, bool justSpawned, bool justCreated)
{
	LLM_SCOPE_(EPunSimLLMTag::PUN_DisplayTerrain);

	WorldRegion2 region(regionId);
	WorldAtom2 objectAtomLocation(region.x * CoordinateConstants::AtomsPerRegion,
		region.y * CoordinateConstants::AtomsPerRegion);

	FVector displayLocation = MapUtil::DisplayLocation(cameraAtom, objectAtomLocation);

	// PUN_LOG("region %d, %d ... %s", region.x, region.y, *displayLocation.ToString());

	_terrainLargeChunks[meshId]->SetRelativeLocation(displayLocation);

	// Update the terrain chunk if needed
	if (!_terrainLargeChunks[meshId]->alreadyUpdatedMesh)
	{
		//SCOPE_TIMER("Tick Region Terrain");

		//_terrainLargeChunks[meshId]->UpdateLargeTerrainChunkMesh(simulation(), region, false);
		if (!justCreated) {
			chunkInfosToUpdate.push_back({ meshId, regionId, false });
		}
		
		_terrainLargeChunks[meshId]->alreadyUpdatedMesh = true;
	}

	// Overlap Terrain should have its beach pushed down to hide seam...
	//_terrainChunks[meshId]->MaterialInstance->SetScalarParameterValue("IsOverlapTerrain", _isOverlapTerrain);

	//if (isMainMenuDisplay) {
	//	_terrainChunks[meshId]->MaterialInstance->SetScalarParameterValue("IsForestOnly", true);
	//}


	// Debug: Set cheap material if needed
	if (PunSettings::IsOn("PlainTerrain") != _terrainLargeChunks[meshId]->bIsPlainMaterial) {
		_terrainLargeChunks[meshId]->SetTerrainMaterial(_assetLoader, PunSettings::IsOn("PlainTerrain"));
	}

	// Trailer jitter fix
	if (PunSettings::TrailerSession) {
		_terrainLargeChunks[meshId]->SetVisibility(_gameManager->zoomDistance() > WorldZoomTransition_RegionToRegion4x4);
	}
}

void UTerrainLargeDisplayComponent::AfterAdd()
{
	// Prepare
	{
		SCOPE_TIMER_FILTER(1000, "Tick Region4x4 Test Prepare (%llu)", chunkInfosToUpdate.size());

		//if (PunSettings::Get(FString("TerrainLargeThread")) == 0)
		//{
		//	for (MeshChunkInfo& chunkInfo : chunkInfosToUpdate) {
		//		_terrainLargeChunks[chunkInfo.meshId]->UpdateLargeTerrainChunkMesh_Prepare(simulation(), chunkInfo.regionId, chunkInfo.createMesh);
		//	}
		//}
		//else
		//{
			ThreadedRun(chunkInfosToUpdate, 8, [&](MeshChunkInfo& chunkInfo) {
				_terrainLargeChunks[chunkInfo.meshId]->UpdateLargeTerrainChunkMesh_Prepare(simulation(), chunkInfo.regionId, chunkInfo.createMesh);
			});
		//}
	}

	// Update Mesh
	{
		SCOPE_TIMER_FILTER(1000, "Tick Region4x4 Test UpdateMesh (%llu)", chunkInfosToUpdate.size());
		for (MeshChunkInfo& chunkInfo : chunkInfosToUpdate) {
			_terrainLargeChunks[chunkInfo.meshId]->UpdateLargeTerrainChunkMesh_UpdateMesh(chunkInfo.createMesh);
		}
	}
}

void UTerrainLargeDisplayComponent::HideDisplay(int meshId, int32 regionId)
{
	LLM_SCOPE_(EPunSimLLMTag::PUN_DisplayTerrain);

	_terrainLargeChunks[meshId]->SetVisibility(false);
}