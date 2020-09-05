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
	int CreateNewDisplay(int objectId) override;
	void OnSpawnDisplay(int objectId, int meshId, WorldAtom2 cameraAtom) override;
	void UpdateDisplay(int objectId, int meshId, WorldAtom2 cameraAtom) override;
	void HideDisplay(int meshId, int32 regionId) override;

private:
	UPROPERTY() TArray<class UTerrainLargeChunkComponent*> _terrainChunks;
};
