// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "DisplaySystemComponent.h"
#include "../AssetLoaderComponent.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "Components/DecalComponent.h"
#include "Engine/Texture2D.h"
#include "TerritoryMeshComponent.h"

#include "TerritoryDisplayComponent.generated.h"

USTRUCT()
struct FTerritoryMeshes
{
	GENERATED_BODY();
	UPROPERTY() TArray<UTerritoryMeshComponent*> TerritoryMeshes;
};

USTRUCT()
struct FTerritoryDecals
{
	GENERATED_BODY()

	UPROPERTY() TArray<UDecalComponent*> decals;
	UPROPERTY() TArray<UMaterialInstanceDynamic*> decalMaterials;

	UPROPERTY() TMap<int32, int32> playerIdToDecalIndex;

	void AddNewDecal(int32_t playerId, USceneComponent* parent, UAssetLoaderComponent* assetLoader);

	int32 Contains(int32_t playerId) {
		return playerIdToDecalIndex.Contains(playerId);
	}

	UDecalComponent* GetDecal(int32_t playerId) { return decals[playerIdToDecalIndex[playerId]]; }
	UMaterialInstanceDynamic* GetDecalMaterial(int32_t playerId) { return decalMaterials[playerIdToDecalIndex[playerId]]; }
	UTexture2D* adjacentTerritoriesTexture(int32_t playerId) { return _adjacentTerritoriesTextures[playerIdToDecalIndex[playerId]]; }
	UTexture2D* adjacentTerritoriesTextures2(int32_t playerId) { return _adjacentTerritoriesTextures2[playerIdToDecalIndex[playerId]]; }
	
private:
	UPROPERTY() TArray<UTexture2D*> _adjacentTerritoriesTextures;
	UPROPERTY() TArray<UTexture2D*> _adjacentTerritoriesTextures2;
};

UCLASS( ClassGroup=(Custom), meta=(BlueprintSpawnableComponent) )
class UTerritoryDisplayComponent : public UDisplaySystemComponent
{
	GENERATED_BODY()
public:
	void Init(int size, TScriptInterface<IDisplaySystemDataSource> gameManager, UAssetLoaderComponent* assetLoader, int32 initialPoolSize) final
	{
		UDisplaySystemComponent::Init(size, gameManager, assetLoader, initialPoolSize);

		// Make region border decal
		//auto decal = NewObject<UDecalComponent>(this);
		//decal->AttachToComponent(this, FAttachmentTransformRules::KeepRelativeTransform, NAME_None);
		//decal->RegisterComponent();
		//_regionBorderDecalMaterial = UMaterialInstanceDynamic::Create(_assetLoader->RegionBorderMaterial, this);
		//decal->SetMaterial(0, _regionBorderDecalMaterial);
		//decal->DecalSize = FVector(512, 1024, 1024);
		//decal->SetRelativeRotation(FVector(0, 0, -1).Rotation()); // Rotate to project the decal down
		//_regionBorderDecal = decal;
	}

	void Display(std::vector<int>& sampleProvinceIds) override;

private:
	UTerritoryMeshComponent* CreateTerritoryMeshComponent(bool isProvince)
	{
		auto comp = NewObject<UTerritoryMeshComponent>(this);
		comp->Rename(*FString(("TerritoryChunk" + to_string(territoryNameCounter++)).c_str()));
		comp->AttachToComponent(this, FAttachmentTransformRules::KeepRelativeTransform, NAME_None);
		comp->SetCollisionEnabled(ECollisionEnabled::NoCollision);
		comp->SetGenerateOverlapEvents(false);
		comp->bAffectDistanceFieldLighting = false;
		comp->SetReceivesDecals(false);
		comp->SetCastShadow(false);
		comp->RegisterComponent();
		if (isProvince) {
			comp->SetTerritoryMaterial(_assetLoader->M_Province, _assetLoader->M_Province_Top);
		} else {
			comp->SetTerritoryMaterial(_assetLoader->M_Territory, _assetLoader->M_Territory_Top);
		}
		return comp;
	}
	
	void DisplayPlayerId(int32 playerId, std::vector<int32>& territoryOwnerMap, FTerritoryDecals& decals);

private:
	UPROPERTY() FTerritoryDecals playerDecals;

	//UPROPERTY() UDecalComponent* _regionBorderDecal;
	//UPROPERTY() UMaterialInstanceDynamic* _regionBorderDecalMaterial;


	UPROPERTY() TArray<UTerritoryMeshComponent*> _provinceMeshes;
	UPROPERTY() TArray<UTerritoryMeshComponent*> _territoryMeshesInner;
	UPROPERTY() TArray<UTerritoryMeshComponent*> _provinceMeshesPool;

	UPROPERTY() TArray<FTerritoryMeshes> _playerIdToTerritoryMesh;
	
	
	int32 territoryNameCounter = 0;
};
