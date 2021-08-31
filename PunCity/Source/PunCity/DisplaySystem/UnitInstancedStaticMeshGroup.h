// Pun Dumnernchanvanit's

#pragma once

#include "GameDisplayInfo.h"
#include "UnitInstancedStaticMeshComponent.h"
#include "Components/SceneComponent.h"

#include "UnitInstancedStaticMeshGroup.generated.h"



UCLASS( ClassGroup=(Custom), meta=(BlueprintSpawnableComponent) )
class PROTOTYPECITY_API UUnitInstancedStaticMeshGroup : public USceneComponent
{
	GENERATED_BODY()
public:
	
	UUnitInstancedStaticMeshGroup()
	{
		//for (int32 i = 0; i < UnitDisplayEnumCount; i++) {
		//	meshes.Add(CreateDefaultSubobject<UUnitInstancedStaticMeshComponent>(FName(FString("UnitMeshesNew") + FString::FromInt(i))));
		//}
	}

	void Init(UAssetLoaderComponent* assetLoader) {
		_assetLoader = assetLoader;
		_meshes.SetNum(UnitEnumCount * _assetLoader->maxUnitMeshVariationCount());
	}

	int32 GetMeshIndex(UnitEnum unitEnum, int32 variationIndex) {
		return static_cast<int32>(unitEnum) * _assetLoader->maxUnitMeshVariationCount() + variationIndex;
	}

	void AddProtoMesh(UnitEnum unitEnum, int32 variationIndex, UStaticMesh* mesh)
	{
		int32 meshIndex = GetMeshIndex(unitEnum, variationIndex);

		if (_meshes[meshIndex] == nullptr)
		{
			auto unitMesh = NewObject<UUnitInstancedStaticMeshComponent>(this);
			unitMesh->Rename(*(FString("UnitMeshes") + FString::FromInt(meshIndex)));
			unitMesh->AttachToComponent(this, FAttachmentTransformRules::KeepRelativeTransform, NAME_None);
			unitMesh->RegisterComponent();
			unitMesh->SetStaticMesh(mesh);
			unitMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);

			PUN_LOG("AddProtoMesh unitEnum%d variation:%d mIndex:%d", unitEnum, variationIndex, meshIndex);

			_meshes[meshIndex] = unitMesh;
		}
	}

	void Add(UnitEnum unitEnum, int32 variationIndex, int32 unitId, FTransform& transform)
	{
		check(_meshes[GetMeshIndex(unitEnum, variationIndex)] != nullptr);
		_meshes[GetMeshIndex(unitEnum, variationIndex)]->Add(unitId, transform);
	}


	void AfterAdd() {
		for (int32 i = 0; i < _meshes.Num(); i++) {
			if (_meshes[i]) {
				_meshes[i]->AfterAdd();
			}
		}
	}

	void SetCustomDepth(UnitEnum unitEnum, int32 variationIndex, int32 customDepthIndex) {
		GameDisplayUtils::SetCustomDepth(_meshes[GetMeshIndex(unitEnum, variationIndex)], customDepthIndex);
	}

private:

	UPROPERTY() UAssetLoaderComponent* _assetLoader;
	UPROPERTY() TArray<UUnitInstancedStaticMeshComponent*> _meshes;
	
};
