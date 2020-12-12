// Pun Dumnernchanvanit's

#pragma once

#include "CoreMinimal.h"
#include "PunCity/DisplaySystem/DisplaySystemComponent.h"

#include "TerrainLargeDisplayComponent.generated.h"

/**
 * 
 */
UCLASS()
class PROTOTYPECITY_API UTerrainLargeDisplayComponent : public UDisplaySystemComponent
{
	GENERATED_BODY()
public:

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
