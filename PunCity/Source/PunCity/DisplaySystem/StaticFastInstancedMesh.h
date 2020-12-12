// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "GameDisplayContainers.h"
#include "Engine/StaticMesh.h"
#include "PunCity/CppUtils.h"
#include "PunCity/PunUtils.h"

#include "StaticFastInstancedMesh.generated.h"

//struct MeshInstanceInfoBase
//{
//	int32 key;
//	FTransform transform;
//	int32 state;
//	int32 objectId = -1;
//};



USTRUCT(BlueprintType)
struct FStaticFastInstancedMeshState
{
	GENERATED_BODY();

	FString name;
	UPROPERTY() UStaticMesh* protoMesh = nullptr;
	UPROPERTY() UMaterialInterface* protoMaterial = nullptr;
	bool hasCollision = false;
	bool noBasePass = false;
	
};

// Keep track of mesh reuse for static transforms (only set transform when spawning)
// Can use of region/tileId/static assumption to avoid having to do UpdateInstance every updates
// Disabled meshes are also pooled instead of removing

UCLASS( ClassGroup=(Custom), meta=(BlueprintSpawnableComponent) )
class UStaticFastInstancedMesh : public UInstancedStaticMeshComponent
{
	GENERATED_BODY()
public:
	struct InstanceInfo
	{
		int16 instanceIndex = -1;
		int8 stateMod = -1; // state modded to fit in int8_t .. modding is fine here since we just want to know if the state changed
	};

	UStaticFastInstancedMesh() { PrimaryComponentTick.bCanEverTick = false; }

	void Init(USceneComponent* parent, int maxTileId, bool forceUpdateTransform = false, bool hasCollision = false);

	// BatchAdd used for spawning new region
	void BeforeBatchAdd();
	void BatchAdd(int32 key, FTransform transform, int32 state, int32 objectId = -1);
	
	// Add if necessary. 
	// -tileId that was already added won't be touched
	// -tileId that wasn't added for an update will get despawned automatically
	void Add(int32 key, FTransform transform, int32 state, int32 objectId = -1);
	void AfterAdd();

	bool ContainsKey(int32 key) { return CppUtils::Contains(_keysThisTick, key); }

	void SetActive(bool bNewActive, bool bReset = false) override;
	
	// For collision
	int32_t GetObjectId(int32 instanceIndex) {
		return _instanceIndexToObjectId.Get(instanceIndex);
	}

	// Debug...
	PUN_DEBUG_EXPR(
	bool NoBug() {
		return _keyToInstanceInfo.count() + _disabledInstanceIndices.Num() == GetInstanceCount();
	}
	bool NoBug2() {
		return _keysThisTick.size() <= _keyToInstanceInfo.count();
	}
	);

	void SetMeshState(FStaticFastInstancedMeshState meshState) {
		_hasCollision = meshState.hasCollision;
		_noBasePass = meshState.noBasePass;
		
		if (meshState.noBasePass) {
			SetMaterial(0, nullptr);
			SetVisibility(false);
		}
		else {
			SetMaterial(0, meshState.protoMaterial ? meshState.protoMaterial : meshState.protoMesh->GetMaterial(0));
		}

		SetStaticMesh(meshState.protoMesh);
		Rename(*meshState.name);
	}


	///*
	// * Queue for threading
	// */
	//TArray<MeshInstanceInfoBase> meshesToAdd;

	//void ClearAddQueue() { meshesToAdd.Empty(); }
	//void QueueAdd(int32 key, FTransform transform, int32 state, int32 objectId = -1) {
	//	meshesToAdd.Add({ key, transform, state, objectId });
	//}
	//void ExecuteAddQueue()
	//{
	//	for (MeshInstanceInfoBase& meshInfo : meshesToAdd) {
	//		Add(meshInfo.key, meshInfo.transform, meshInfo.state, meshInfo.objectId);
	//	}
	//}

public:
	// For its parent
	FString meshName;
	int32 poolIndex = -1;

	bool shouldBatchAdd = false;

private:
	void Despawn(int instanceIndex);

private:
	UPROPERTY() TArray<int32> _disabledInstanceIndices;

	CycleMap<InstanceInfo> _keyToInstanceInfo;
	CycleMap<int32> _instanceIndexToObjectId;

	std::vector<int32> _keysThisTick;

	bool _needUpdate; // We only need to update if we have some change in instanceMesh's transform
	bool _forceUpdateTransform;
	bool _hasCollision;
	bool _noBasePass;

	TArray<FTransform> _instancesToBatchAdd;
};
