// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "DisplaySystemComponent.h"
#include "StaticFastInstancedMesh.h"
#include "StaticFastInstancedMeshesComp.h"

#include "TileObjectDisplayComponent.generated.h"

UCLASS( ClassGroup=(Custom), meta=(BlueprintSpawnableComponent) )
class UTileObjectDisplayComponent : public UDisplaySystemComponent
{
	GENERATED_BODY()
public:
	UTileObjectDisplayComponent() {
		LLM_SCOPE_(EPunSimLLMTag::PUN_DisplayTileObj);
		
		_fallingMeshes = CreateDefaultSubobject<UStaticFastInstancedMeshesComp>("_fallingMeshes");
	}
	
	void Init(int size, TScriptInterface<IDisplaySystemDataSource> gameManager, UAssetLoaderComponent* assetLoader) override 
	{
		LLM_SCOPE_(EPunSimLLMTag::PUN_DisplayTileObj);
		
		UDisplaySystemComponent::Init(size, gameManager, assetLoader);

		_fallingMeshes->Init("FallenTree", this, 50, "FallenTree", 0, true);
		for (int i = 0; i < TreeEnumSize; i++) 
		{
			TileObjInfo info = GetTileObjInfoInt(i);
			FTileMeshAssets tileProtoMeshes = _assetLoader->tileMeshAsset(info.treeEnum);

			for (int j = 0; j < tileProtoMeshes.assets.Num(); j++) {
				if (tileProtoMeshes.assets[j]) {
					FString meshName = ToFString(TileSubmeshName[j] + info.name);
					//PUN_LOG("Falling mesh: j=%d, %s", j, *meshName);
					
					_fallingMeshes->AddProtoMesh(meshName, tileProtoMeshes.assets[j]);
					//_fallingMeshes->GetMesh(meshName)->SetRenderCustomDepth(false);
				}
			}
		}
	}

	void AfterAdd() override {
		LLM_SCOPE_(EPunSimLLMTag::PUN_DisplayTileObj);
		
		_fallingMeshes->AfterAdd();
	}

	int32 GetObjectId(int32 meshId, FString protoName, int32 instanceIndex) {
		return _meshIdToMeshes[meshId]->GetObjectId(protoName, instanceIndex);
	}

	void RefreshDisplay(const std::vector<int>& sampleIds)
	{
		_displayStateChanged = true;
		for (int32 sampleId : sampleIds)
		{
			//if (sampleId < _meshIdByObjectId.size()) {
			//	int32 meshId = _meshIdByObjectId[sampleId];
			//	if (meshId != -1 && _meshIdToMeshes[meshId] != nullptr) {
			//		_meshIdToMeshes[meshId]->AfterAdd(); // Note: Forced refresh...
			//		simulation().NeedDisplayUpdate(DisplayClusterEnum::Trees, sampleId);
			//	}
			//}
			simulation().SetNeedDisplayUpdate(DisplayClusterEnum::Trees, sampleId);

			for (auto mesh : _meshIdToMeshes) {
				mesh->ClearMeshes();
			}
		}
	}

	void BeforeDisplay(bool isFullDisplay, const std::vector<int>& sampleIds, bool isZoomedAllTheWayOut, bool hideTreeDisplay = false)
	{
		LLM_SCOPE_(EPunSimLLMTag::PUN_DisplayTileObj);
		
		if (_isFullDisplay != isFullDisplay) {
			PUN_LOG("SetDisplayState %d", isFullDisplay);
			_isFullDisplay = isFullDisplay;

			RefreshDisplay(sampleIds);
		}

		_maxSpawnPerTick = isZoomedAllTheWayOut ? 3 : 999;

		_isHiddenDisplay = hideTreeDisplay;
		
		_noRegionSkipThisTick = true;
	}
	void AfterDisplay() {
		_displayStateChanged = false;
	}

	bool NoRegionSkipThisTick() {
		return _noRegionSkipThisTick;
	}


protected:
	int CreateNewDisplay(int32 objectId) override;
	void OnSpawnDisplay(int32 objectId, int32 meshId, WorldAtom2 cameraAtom) override;
	void UpdateDisplay(int32 regionId, int32 meshId, WorldAtom2 cameraAtom) override;
	void HideDisplay(int32 meshId) override;

private:
	bool _isFullDisplay = true;
	bool _isHiddenDisplay = false; // Hidden, but still being generated
	bool _displayStateChanged = true;
	bool _noRegionSkipThisTick = true;
	
	UPROPERTY() TArray<UStaticFastInstancedMeshesComp*> _meshIdToMeshes;
	UPROPERTY() TArray<UStaticFastInstancedMesh*> _meshIdToGatherMarks;

	UPROPERTY() UStaticFastInstancedMeshesComp* _fallingMeshes;
};
