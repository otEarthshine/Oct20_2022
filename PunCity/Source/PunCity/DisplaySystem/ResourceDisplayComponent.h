// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "DisplaySystemComponent.h"
#include "StaticFastInstancedMeshesComp.h"

#include "ResourceDisplayComponent.generated.h"

UCLASS( ClassGroup=(Custom), meta=(BlueprintSpawnableComponent) )
class UResourceDisplayComponent : public UDisplaySystemComponent
{
	GENERATED_BODY()
public:	

protected:
	int CreateNewDisplay(int objectId) override;
	void UpdateDisplay(int regionId, int meshId, WorldAtom2 cameraAtom) override;
	void HideDisplay(int meshId) override;

	void OnSpawnDisplay(int regionId, int meshId, WorldAtom2 cameraAtom) override {
		_meshes[meshId]->SetActive(true);

		// Refresh
		simulation().SetNeedDisplayUpdate(DisplayClusterEnum::Resource, regionId, true);
	}

	UPROPERTY() TArray<UStaticFastInstancedMeshesComp*> _meshes;
};
