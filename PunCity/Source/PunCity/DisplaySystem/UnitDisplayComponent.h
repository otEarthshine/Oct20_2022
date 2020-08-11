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

		// TODO: find a way to swap TMap
		//std::swap(_lastTransforms, _thisTransform);
		_lastTransforms = _thisTransform;
		_thisTransform.Empty();
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

		SkelMeshAfterAdd();
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
	/*
	 * Skel Mesh
	 */
	void AddUnit(int32 unitId , UnitStateAI& unit)
	{
		FTransform transform;
		int32 variationIndex = GetUnitTransformAndVariation(unitId, transform);
		
		int32 index = -1;
		if (_lastUnitIdToSkelMeshIndex.Contains(unitId)) {
			index = _lastUnitIdToSkelMeshIndex[unitId];
			_lastUnitIdToSkelMeshIndex.Remove(unitId);
		}
		else {
			for (int32 i = 0; i < _unitSkelMeshes.Num(); i++) {
				if (!_unitSkelMeshes[i]->IsVisible()) {
					index = i;
					break;
				}
			}
			if (index == -1) {
				index = _unitSkelMeshes.Num();
				auto skelMesh = NewObject<USkeletalMeshComponent>(this);
				skelMesh->AttachToComponent(this, FAttachmentTransformRules::KeepRelativeTransform);
				skelMesh->RegisterComponent();
				skelMesh->SetSkeletalMesh(_assetLoader->unitSkelMesh(UnitEnum::Human, 0));
				//skelMesh->SetAnimation(_assetLoader->unitAnimation(UnitAnimationEnum::Walk));
				//skelMesh->SetPlayRate();
				_unitSkelMeshes.Add(skelMesh);
				_unitAnimationEnum.push_back(UnitAnimationEnum::None);
			}
		}
		_unitIdToSkelMeshIndex.Add(unitId, index);

		// Setup the mesh
		USkeletalMeshComponent* skelMesh = _unitSkelMeshes[index];
		skelMesh->SetWorldTransform(transform);
		skelMesh->SetVisibility(true);

		//
		UnitAnimationEnum animationEnum = UnitAnimationEnum::Waiting;
		// TODO: need animationEnum in UnitEnum for this...

		if (_unitAnimationEnum[index] != animationEnum) {
			skelMesh->SetPlayRate(1.0f);
			skelMesh->PlayAnimation(_assetLoader->unitAnimation(animationEnum), true);
		}

		_thisTransform.Add(unitId, transform);
	}

	void SkelMeshAfterAdd()
	{
		// Despawn all unused meshes
		for (auto it : _lastUnitIdToSkelMeshIndex) {
			_unitSkelMeshes[it.Value]->SetVisibility(false);
		}
		_lastUnitIdToSkelMeshIndex = _unitIdToSkelMeshIndex;
		_unitIdToSkelMeshIndex.Empty(); // _unitIdToSkelMeshIndex.Num());
	}

private:
	UPROPERTY() UStaticFastInstancedMeshesComp* _unitMeshes;
	UPROPERTY() UStaticFastInstancedMeshesComp* _resourceMeshes;

	TMap<int32, FTransform> _lastTransforms;
	TMap<int32, FTransform> _thisTransform;

	UPROPERTY() UStaticFastInstancedMeshesComp* _animatingModuleMeshes;

	TMap<int32, FRotationInfo> _lastWorkRotatorRotation;

	//UPROPERTY() USkeletalMeshComponent* _testAnimatedMesh;

	// Skel
	UPROPERTY() TArray<USkeletalMeshComponent*> _unitSkelMeshes;
	std::vector<UnitAnimationEnum> _unitAnimationEnum;
	TMap<int32, int32> _unitIdToSkelMeshIndex;
	TMap<int32, int32> _lastUnitIdToSkelMeshIndex;
};
