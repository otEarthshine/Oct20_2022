// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "DisplaySystemComponent.h"
#include "StaticFastInstancedMesh.h"
#include "StaticFastInstancedMeshesComp.h"

#include "ResourceDisplayComponent.generated.h"

UCLASS( ClassGroup=(Custom), meta=(BlueprintSpawnableComponent) )
class UResourceDisplayComponent : public UDisplaySystemComponent
{
	GENERATED_BODY()
public:	

protected:
	int CreateNewDisplay(int objectId) override;
	void OnSpawnDisplay(int objectId, int meshId, WorldAtom2 cameraAtom) override;
	void UpdateDisplay(int regionId, int meshId, WorldAtom2 cameraAtom) override;
	void HideDisplay(int meshId) override;

private:
	UPROPERTY() TArray<UStaticFastInstancedMeshesComp*> _meshes;
};
