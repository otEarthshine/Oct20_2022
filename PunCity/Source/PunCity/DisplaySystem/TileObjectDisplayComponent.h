// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "DisplaySystemComponent.h"
#include "StaticFastInstancedMesh.h"
#include "StaticFastInstancedMeshesComp.h"

#include "TileObjectDisplayComponent.generated.h"

#define TILE_OBJ_CACHE TRAILER_MODE

UCLASS( ClassGroup=(Custom), meta=(BlueprintSpawnableComponent) )
class UTileObjectDisplayComponent : public UDisplaySystemComponent
{
	GENERATED_BODY()
public:
	UTileObjectDisplayComponent() {
		LLM_SCOPE_(EPunSimLLMTag::PUN_DisplayTileObj);
		
		_fallingMeshes = CreateDefaultSubobject<UStaticFastInstancedMeshesComp>("_fallingMeshes");
	}
	
	void Init(int size, TScriptInterface<IDisplaySystemDataSource> gameManager, UAssetLoaderComponent* assetLoader, int32 initialPoolSize) override
	{
		LLM_SCOPE_(EPunSimLLMTag::PUN_DisplayTileObj);
		
		UDisplaySystemComponent::Init(size, gameManager, assetLoader, initialPoolSize);

		_fallingMeshes->Init("FallenTree", this, 50, "FallenTree", 0, true);
		for (int i = 0; i < TreeEnumSize; i++) 
		{
			TileObjInfo info = GetTileObjInfoInt(i);
			FTileMeshAssets tileProtoMeshes = _assetLoader->tileMeshAsset(info.treeEnum);

			for (int j = 0; j < tileProtoMeshes.assets.Num(); j++) {
				if (tileProtoMeshes.assets[j]) {
					FString meshName = GetFallingMeshName(static_cast<TileSubmeshEnum>(j), info.treeEnum); // TODO: Can this cause problem with translation?
					//PUN_LOG("FALLING mesh: j=%d, %s", j, *meshName);
					
					_fallingMeshes->AddProtoMesh(meshName, tileProtoMeshes.assets[j]);
					//_fallingMeshes->GetMesh(meshName)->SetRenderCustomDepth(false);
				}
			}
		}

#if TRAILER_MODE
		_objectIdToMeshes.Empty();
#endif
	}

	int32 GetObjectId(int32 meshId, FString protoName, int32 instanceIndex) {
#if TILE_OBJ_CACHE
		return meshId;
#else
		return _meshIdToMeshes[meshId]->GetObjectId(protoName, instanceIndex);
#endif
	}

	void RefreshDisplay(const std::vector<int>& sampleIds)
	{
#if TILE_OBJ_CACHE
		return;
#endif
		
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
		
		_noRegionSkipThisTickYet = true;
	}
	void BeforeAdd() override
	{
		_chunkInfosToUpdate.clear();
		_chunkInfosAll.clear();
	}
	void AfterAdd() override
	{
		_displayStateChanged = false;

		// Note: TileObj Threading doesn't work well...
		{
			SCOPE_TIMER_FILTER(5000, "TileObj Prepare size:%llu", _chunkInfosToUpdate.size());

			if (PunSettings::Get(FString("TileObjThread")) == 0)
			{
				for (MeshChunkInfo& chunkInfo : _chunkInfosToUpdate) {
					UpdateDisplay_PrepareReset(chunkInfo); // QueueAdd
				}
			}
			else
			{
				ThreadedRun(_chunkInfosToUpdate, PunSettings::Get("TileObjThreadCount"), [&](MeshChunkInfo& chunkInfo)
				{
					UpdateDisplay_PrepareReset(chunkInfo); // QueueAdd
				});
			}
		}

		{
			SCOPE_TIMER_FILTER(1000, "TileObj UpdateMesh");
			for (MeshChunkInfo& chunkInfo : _chunkInfosToUpdate) {
				UpdateDisplay_UpdateMesh(chunkInfo); // ExecuteAddQueue
			}
		}

		for (MeshChunkInfo& chunkInfo : _chunkInfosAll) {
			UpdateDisplay_FinishAll(chunkInfo);
		}

		_fallingMeshes->AfterAdd();
	}
	//void AfterAdd() override {
	//	LLM_SCOPE_(EPunSimLLMTag::PUN_DisplayTileObj);

	//	_fallingMeshes->AfterAdd();
	//}

	void UpdateDisplay_PrepareReset(MeshChunkInfo& chunkInfo);
	void UpdateDisplay_UpdateMesh(MeshChunkInfo& chunkInfo);
	
	void UpdateDisplay_FinishAll(MeshChunkInfo& chunkInfo);
	

	bool NoRegionSkipThisTick() {
		return _noRegionSkipThisTickYet;
	}

protected:
	int CreateNewDisplay(int32 objectId) override;
	void OnSpawnDisplay(int32 objectId, int32 meshId, WorldAtom2 cameraAtom) override;
	void UpdateDisplay(int32 regionId, int32 meshId, WorldAtom2 cameraAtom, bool justSpawned, bool justCreated) override;
	void HideDisplay(int32 meshId, int32 regionId) override;

private:
	static FString GetMeshName(TileObjEnum tileObjEnum, int32 variationIndex) {
		return "TileObj" + FString::FromInt(static_cast<int>(tileObjEnum)) + "_" + FString::FromInt(variationIndex);
	}
	static FString GetFallingMeshName(TileSubmeshEnum submeshEnum, TileObjEnum tileObjEnum) {
		return ToFString(TileSubmeshName[static_cast<int>(submeshEnum)]) + FString::FromInt(static_cast<int>(tileObjEnum));
	}
	
private:
	bool _isFullDisplay = true;
	bool _isHiddenDisplay = false; // Hidden, but still being generated
	bool _displayStateChanged = true;
	bool _noRegionSkipThisTickYet = true;


	UPROPERTY() TArray<UStaticFastInstancedMeshesComp*> _objectIdToMeshes; // For TRAILER_MODE

	TArray<FastInstancedMeshesData> _meshIdToMeshData;
	UPROPERTY() TArray<UStaticFastInstancedMeshesComp*> _meshIdToMeshes;

	
	//UPROPERTY() TArray<UStaticFastInstancedMesh*> _meshIdToGatherMarks;

	
	UPROPERTY() UStaticFastInstancedMeshesComp* _fallingMeshes;


	std::vector<MeshChunkInfo> _chunkInfosToUpdate;
	std::vector<MeshChunkInfo> _chunkInfosAll;
};
