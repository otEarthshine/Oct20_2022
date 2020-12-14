// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "Components/SceneComponent.h"
#include "UnrealEngine.h"
#include "DisplaySystemDataSource.h"
#include "../AssetLoaderComponent.h"
#include "../MapUtil.h"

#include "DisplaySystemComponent.generated.h"

/**
 * Display specific type of object within the area where the camera can see.
 *  With this class, only unused mesh gets disabled preventing flashing.
 *  Subclass/Implement the virtual functions to make this workd
 */

struct MeshChunkInfo
{
	int32 meshId;
	int32 regionId;
	bool createMesh = false;
	bool justSpawned = false;

	WorldRegion2 region() { return WorldRegion2(regionId); }
};

UCLASS( ClassGroup=(Custom), meta=(BlueprintSpawnableComponent) )
class UDisplaySystemComponent : public USceneComponent
{
	GENERATED_BODY()
public:
	UDisplaySystemComponent() { PrimaryComponentTick.bCanEverTick = false; }

	virtual void Init(int size, TScriptInterface<IDisplaySystemDataSource> gameManager, UAssetLoaderComponent* assetLoader, int32 initialPoolSize);

	// Called to update vector size when simulation changed (like adding units/buildings)
	void ResizeObjectIds(int newSize)
	{
		PUN_CHECK(newSize >= 0);
		for (size_t i = _meshIdByObjectId.size(); i < newSize; i++) {
			_meshIdByObjectId.push_back(-1);
		}
		
		//while (_meshIdByObjectId.size() < newSize) {
		//	_meshIdByObjectId.push_back(-1);
		//}
	}

	virtual void Display(std::vector<int>& sampleIds);

	
	int32 GetMeshId(int32 objectId) {
		return _meshIdByObjectId[objectId];
	}

	int32 meshPoolCount() { return _objectIdByMeshId.size(); }

protected:
	virtual int CreateNewDisplay(int objectId) { 
		_meshId++;
		return _meshId - 1;
	}

	virtual void OnSpawnDisplay(int32 objectId, int32 meshId, WorldAtom2 cameraAtom) {}
	virtual void UpdateDisplay(int32 objectId, int32 meshId, WorldAtom2 cameraAtom, bool justSpawned, bool justCreated) {}
	
	virtual void HideDisplay(int32 meshId, int32 regionId) {}
	
	virtual void BeforeAdd() {}
	virtual void AfterAdd() {}

	class UStaticMeshComponent* CreateMeshComponent(USceneComponent* parent, std::string name);

	class GameSimulationCore& simulation() { return gameManager()->simulation(); }
	ULineBatchComponent* lineBatch() { return gameManager()->lineBatch(); }
	int32 playerId() { return gameManager()->playerId(); }

	IDisplaySystemDataSource* gameManager() {
		PUN_CHECK(_gameManager);
		return CastChecked<IDisplaySystemDataSource>(_gameManager.GetObject());
	}

	//
	template <typename Func>
	void ThreadedRun(std::vector<MeshChunkInfo>& chunkInfos, int32 threadCount, Func func)
	{
		if (threadCount == 0) {
			threadCount = 8;
		}
		if (chunkInfos.size() >= threadCount)
		{
			PUN_LOG("Tick Region Test Prepare3: chunkInfos:%d", chunkInfos.size());

			// Split into threads
			TArray<TFuture<uint8>> futures;

			for (int32 i = 0; i < threadCount; i++)
			{
				futures.Add(
					Async(EAsyncExecution::Thread, [&, i]()
					{
						for (int32 j = i; j < chunkInfos.size(); j += threadCount) {
							//PUN_LOG("Thread i:%d regionId:%d", i, chunkInfos[j].regionId);
							func(chunkInfos[j]);
						}
						//PUN_LOG("DoneThread i:%d", i);
						return static_cast<uint8>(1);
					})
				);
			}

			// Pause while all the thread works
			for (int32 i = 0; i < futures.Num(); i++) {
				futures[i].Wait();
			}

		}
		else
		{
			// No thread
			for (MeshChunkInfo& chunkInfo : chunkInfos) {
				func(chunkInfo);
			}
		}
	}
	

public:
	bool isMainMenuDisplay = false;
	
protected:
	UPROPERTY() UAssetLoaderComponent* _assetLoader;
	UPROPERTY() TScriptInterface<IDisplaySystemDataSource> _gameManager;

	std::vector<int> _meshIdByObjectId; // index is objectId, value is corresponding the meshId

	std::vector<int> _objectIdByMeshId; // From meshId, we can get the objectId that the mesh displayed, objectId of -1 means the mesh is not used
	std::vector<bool> _isMeshesInUse; // Temporary, reset every frame: Mark regionMesh whether it is being use (size == regionMeshes.size)

	int32 _meshId;

	int32 _spawnedThisTick = 0;
	int32 _maxSpawnPerTick = 99999;
	int32 _lastSampleSize = 0;
	WorldTile2 _lastCameraTile = WorldTile2::Invalid;
};
