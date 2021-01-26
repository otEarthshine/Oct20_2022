// Pun Dumnernchanvanit's

#pragma once

#include "CoreMinimal.h"
#include "PunCity/DisplaySystem/DisplaySystemComponent.h"
#include "TerrainLargeChunkComponent.h"

#include "TerrainLargeDisplayComponent.generated.h"

/**
 * 
 */
UCLASS()
class PROTOTYPECITY_API UTerrainLargeDisplayComponent : public UDisplaySystemComponent
{
	GENERATED_BODY()
public:

	/*
	 * Debug
	 */
	UTerrainLargeChunkComponent* GetTerrainChunk(int32 meshId) { return _terrainLargeChunks[meshId]; }
	void HideAllChunks() {
		for (int32 i = 0; i < _terrainLargeChunks.Num(); i++) {
			HideDisplay(i, _terrainLargeChunks[i]->lastRegionId);
		}
	}
	
protected:
	void BeforeAdd() override {
		chunkInfosToUpdate.clear();
	}
	
	int CreateNewDisplay(int objectId) override;
	void OnSpawnDisplay(int objectId, int meshId, WorldAtom2 cameraAtom) override;
	void UpdateDisplay(int objectId, int meshId, WorldAtom2 cameraAtom, bool justSpawned, bool justCreated) override;
	void HideDisplay(int meshId, int32 regionId) override;

	void AfterAdd() override;
	
private:
	std::vector<MeshChunkInfo> chunkInfosToUpdate;
	
	UPROPERTY() TArray<class UTerrainLargeChunkComponent*> _terrainLargeChunks;
};