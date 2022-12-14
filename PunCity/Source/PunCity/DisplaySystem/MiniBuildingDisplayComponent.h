// Pun Dumnernchanvanit's

#pragma once

#include "CoreMinimal.h"
#include "PunCity/DisplaySystem/DisplaySystemComponent.h"
#include "StaticFastInstancedMeshesComp.h"
#include "MiniBuildingDisplayComponent.generated.h"

/**
 * 
 */
UCLASS()
class PROTOTYPECITY_API UMiniBuildingDisplayComponent : public UDisplaySystemComponent
{
	GENERATED_BODY()
public:
	UMiniBuildingDisplayComponent() {
		PrimaryComponentTick.bCanEverTick = false;
	}

protected:
	int CreateNewDisplay(int regionId) override;
	void OnSpawnDisplay(int regionId, int meshId, WorldAtom2 cameraAtom) override;
	void UpdateDisplay(int regionId, int meshId, WorldAtom2 cameraAtom, bool justSpawned, bool justCreated) override;
	void HideDisplay(int meshId, int32 regionId) override;

	UPROPERTY() TArray<UStaticFastInstancedMeshesComp*> _moduleMeshes;
};
