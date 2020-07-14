// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "DisplaySystemComponent.h"
#include "StaticFastInstancedMeshesComp.h"
#include "Components/SkeletalMeshComponent.h"


struct FRotationInfo
{
	float rotationFloat = 0;
	float rotationSpeed = 0;
};


#include "UnitDisplayComponent.generated.h"

UCLASS( ClassGroup=(Custom), meta=(BlueprintSpawnableComponent) )
class UUnitDisplayComponent : public UDisplaySystemComponent
{
	GENERATED_BODY()
public:
	UUnitDisplayComponent()
	{
		LLM_SCOPE_(EPunSimLLMTag::PUN_DisplayUnit);
		
		_unitMeshes = CreateDefaultSubobject<UStaticFastInstancedMeshesComp>("_unitMeshes");
		_resourceMeshes = CreateDefaultSubobject<UStaticFastInstancedMeshesComp>("_resourceMeshes");
		_animatingModuleMeshes = CreateDefaultSubobject<UStaticFastInstancedMeshesComp>("_animatingModuleMeshes");

		//_testAnimatedMesh = CreateDefaultSubobject<USkeletalMeshComponent>("_testAnimatedMesh");
	}
	
	void Init(int size, TScriptInterface<IDisplaySystemDataSource> gameManager, UAssetLoaderComponent* assetLoader) override
	{
		LLM_SCOPE_(EPunSimLLMTag::PUN_DisplayUnit);
		
		UDisplaySystemComponent::Init(size, gameManager, assetLoader);

		//_unitMeshes->Init("Unit", this, 100, "Unit", 0, true);
		_unitMeshes->Init("Unit", this, 100, "", 0, true);
		for (int i = 0; i < UnitEnumCount; i++) 
		{
			UnitEnum unitEnum = static_cast<UnitEnum>(i);
			
			int32 variationCount = assetLoader->unitMeshCount(unitEnum);
			for (int32 j = 0; j < variationCount; j++) {
				_unitMeshes->AddProtoMesh(ToFString(UnitInfos[i].name) + FString::FromInt(j), assetLoader->unitMesh(unitEnum, j), nullptr);
			}
		}

		_resourceMeshes->Init("UnitResource", this, 100, "", 0, true);
		for (int i = 0; i < ResourceEnumCount; i++) {
			ResourceEnum resourceEnum = static_cast<ResourceEnum>(i);
			_resourceMeshes->AddProtoMesh(ToFString(ResourceName(resourceEnum)), assetLoader->resourceHandMesh(resourceEnum));
		}

		_animatingModuleMeshes->Init("BuildingAnim", this, 20, "", 0, true);
		TArray<FString>& animModuleNames = _assetLoader->animModuleNames();
		for (int i = 0; i < animModuleNames.Num(); i++) {
			UStaticMesh* protoMesh = _assetLoader->moduleMesh(animModuleNames[i]);
			if (protoMesh) {
				_animatingModuleMeshes->AddProtoMesh(animModuleNames[i], protoMesh);
			}
		}

		//_testAnimatedMesh->SetSkeletalMesh(assetLoader->unitSkelMesh(UnitEnum::Human));
	}

	int32 GetUnitTransformAndVariation(int32 unitId, FTransform& transform);

	void BeforeAdd() override {
		LLM_SCOPE_(EPunSimLLMTag::PUN_DisplayUnit);
		
		std::swap(_lastTransforms, _thisTransform);
		_thisTransform.clear();
	}
	void AfterAdd() override
	{
		LLM_SCOPE_(EPunSimLLMTag::PUN_DisplayUnit);
		
		_unitMeshes->AfterAdd();
		_resourceMeshes->AfterAdd();
		_animatingModuleMeshes->AfterAdd();

		// Clean up last rotator transforms that are out of sight
		auto lastRotatorTransformsCopy = _lastWorkRotatorRotation;
		for (auto it : lastRotatorTransformsCopy) {
			if (simulation().buildingIsAlive(it.Key) &&
				 _gameManager->IsInSampleRange(simulation().buildingCenter(it.Key)))
			{}
			else {
				_lastWorkRotatorRotation.Remove(it.Key);
			}
		}
	}

	// Hide displays when sampleIds.size() becomes zero (Switch to world map)
	void Display(std::vector<int>& sampleIds) override
	{
		LLM_SCOPE_(EPunSimLLMTag::PUN_DisplayUnit);
		
		bool shouldDisplay = sampleIds.size() != 0;
		_unitMeshes->SetActive(shouldDisplay);
		_resourceMeshes->SetActive(shouldDisplay);
		_animatingModuleMeshes->SetActive(shouldDisplay);
		UDisplaySystemComponent::Display(sampleIds);
	}

	void SetHighlight(UnitEnum unitEnum, int32 customDepthIndex)
	{
		LLM_SCOPE_(EPunSimLLMTag::PUN_DisplayUnit);
		
		int32 variationCount = _assetLoader->unitMeshCount(unitEnum);
		for (int32 i = 0; i < variationCount; i++) {
			_unitMeshes->SetCustomDepth(GetMeshName(GetUnitInfo(unitEnum), i), customDepthIndex);
		}
	}

protected:
	void UpdateDisplay(int regionId, int meshId, WorldAtom2 cameraAtom) override;

private:
	static FString GetMeshName(const UnitInfo& info, int32 variationIndex)
	{
		FString unitMeshName = ToFString(info.name);
		unitMeshName += FString::FromInt(variationIndex);
		return unitMeshName;
	}

private:
	UPROPERTY() UStaticFastInstancedMeshesComp* _unitMeshes;
	UPROPERTY() UStaticFastInstancedMeshesComp* _resourceMeshes;

	std::unordered_map<int32, FTransform> _lastTransforms;
	std::unordered_map<int32, FTransform> _thisTransform;

	UPROPERTY() UStaticFastInstancedMeshesComp* _animatingModuleMeshes;

	TMap<int32, FRotationInfo> _lastWorkRotatorRotation;

	//UPROPERTY() USkeletalMeshComponent* _testAnimatedMesh;
};
