// Pun Dumnernchanvanit's

#pragma once

#include "CoreMinimal.h"
#include "ResourceDisplayComponent.h"

#include "ResourceByRegionDisplayComponent.generated.h"

/**
 * 
 */
UCLASS()
class PROTOTYPECITY_API UResourceByRegionDisplayComponent : public UResourceDisplayComponent
{
	GENERATED_BODY()
protected:
	//void UpdateDisplay(int regionId, int meshId, WorldAtom2 cameraAtom) override;

	//void OnSpawnDisplay(int regionId, int meshId, WorldAtom2 cameraAtom) override {
	//	_meshes[meshId]->SetActive(true);

	//	// Refresh
	//	simulation().SetNeedDisplayUpdate(DisplayClusterEnum::Resource, regionId, true);
	//}
};
