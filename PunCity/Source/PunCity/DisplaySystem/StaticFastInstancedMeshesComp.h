// Pun Dumnernchanvanit's

#pragma once

#include "CoreMinimal.h"
#include "Components/SceneComponent.h"
#include "StaticFastInstancedMesh.h"

#include "StaticFastInstancedMeshesComp.generated.h"


UCLASS( ClassGroup=(Custom), meta=(BlueprintSpawnableComponent) )
class UStaticFastInstancedMeshesComp : public USceneComponent
{
	GENERATED_BODY()
public:

	UPROPERTY(BlueprintReadOnly) TMap<FString, UStaticFastInstancedMesh*> meshes;
	UPROPERTY() TMap<FString, FStaticFastInstancedMeshState> meshStates;

	// Mesh pool
	//  Contains all meshes, both active/inactive
	//  Go through the pool searching for inactive to reuse meshes
	UPROPERTY() TArray<UStaticFastInstancedMesh*> meshPool;

	UStaticFastInstancedMeshesComp() { PrimaryComponentTick.bCanEverTick = false; }

	int32 poolInUseNumber() {
		int32 count = 0;
		for (int32 i = 0; i < meshPool.Num(); i++) {
			if (!meshPool[i]->meshName.IsEmpty()) {
				count++;
			}
		}
		return count;
	}
	

	void Init(std::string nameSuffix, USceneComponent* parent, int32 bucketCount, std::string collisionTag = "", int32 meshId = -1, bool forceUpdateTransform = false)
	{
		check(bucketCount >= 20);
		_nameSuffix = nameSuffix;
		AttachToComponent(parent, FAttachmentTransformRules::KeepRelativeTransform, NAME_None);
		RegisterComponent();
		_bucketCount = bucketCount;
		_forceUpdateTransform = forceUpdateTransform;

		_collisionTag = collisionTag; 
		_meshId = meshId;

		_activeState = true;
	}
	
	void AddProtoMesh(FString meshName, UStaticMesh* protoMesh, UMaterialInterface* protoMaterial = nullptr, bool hasCollision = false, bool collisionOnly = false)
	{
		meshes.Add(meshName, nullptr);
		meshStates.Add(meshName, { ToFString(_nameSuffix) + meshName, protoMesh, protoMaterial, hasCollision, collisionOnly }); // Here hasCollision is local to this mesh (required global collisionTag)
		
		//_castShadow = castShadow;
	}

	void Add(FString meshName, int32 key, FTransform transform, int32 state, int32 objectId = -1, bool castShadow = true, bool isShadowOnly = false) {
		// PUN_LOG("FFastMesh.Add.Begin ticks:%d id:%d", TimeDisplay::Ticks(), key);
		UStaticFastInstancedMesh* mesh = GetMesh(meshName);

		mesh->SetCastShadow(castShadow);

		//if (isShadowOnly) {
		//	mesh->bCastHiddenShadow = true;
		//	//mesh->SetVisibility(visibility);
		//}
		mesh->bCastHiddenShadow = isShadowOnly;
		mesh->bHiddenInGame = isShadowOnly;
		
		//PUN_LOG("AddMesh: %s", *meshName);
		mesh->Add(key, transform, state, objectId);
		// PUN_LOG("FFastMesh.Add.End ticks:%d id:%d", TimeDisplay::Ticks(), key);
	}

	bool ContainsKey(FString meshName, int32 key) {
		return GetMesh(meshName)->ContainsKey(key);
	}

	void SetActive(bool active, bool bReset = false) override {
		if (_activeState != active || bReset) {
			if (!active) {
				for (auto mesh : meshPool) {
					if (!mesh->meshName.IsEmpty()) {
						DespawnMesh(mesh->meshName);
					}
				}
			}
			_activeState = active;
			USceneComponent::SetActive(active);
		}
	}

	void ClearMeshes()
	{
		for (auto mesh : meshPool) {
			if (!mesh->meshName.IsEmpty()) {
				DespawnMesh(mesh->meshName);
			}
		}
	}

	// SetVisibilility without clearing instances
	void SetVisibilityQuick(bool active)
	{
		for (auto mesh : meshPool) {
			if (!mesh->meshName.IsEmpty()) {
				mesh->SetVisibility(active);
			}
		}
	}
	

	void AfterAdd() {
		for (auto& mesh : meshes) {
			if (mesh.Value) {
				mesh.Value->AfterAdd();

				// Despawn the mesh if it is not used...
				if (mesh.Value->GetInstanceCount() == 0) {
					DespawnMesh(mesh.Key);
				}
			}
		}
	}

	UStaticFastInstancedMesh* GetMesh(FString meshName) {
		check(meshes.Contains(meshName));
		// If hit here, forgot to change TreeEnumSize???
		// Or forgot to add building???

		if (!meshes[meshName]) {
			return SpawnMeshFromPool(meshName);
		}
		return meshes[meshName];
	}

	// For Collision
	int32 GetObjectId(FString meshName, int32 instanceIndex) {
		return meshes[meshName]->GetObjectId(instanceIndex);
	}

	void SetCustomDepth(FString meshName, int32 customDepthIndex) {
		if (meshes.Contains(meshName) && meshes[meshName]) {
			GameDisplayUtils::SetCustomDepth(meshes[meshName], customDepthIndex);
		}
	}
	void SetReceivesDecals(FString meshName, bool receivesDecal) {
		if (meshes.Contains(meshName) && meshes[meshName]) {
			meshes[meshName]->SetReceivesDecals(receivesDecal);
		}
	}

	// TODO: brute set cast shadow...
	void SetCastShadow(FString meshName, bool castShadow) {
		//_castShadow = castShadow;
		if (meshes.Contains(meshName) && meshes[meshName]) {
			meshes[meshName]->SetCastShadow(castShadow);
		}
	}

private:
	bool hasCollision() { return _collisionTag != ""; }
	
	/*
	 * Pooling
	 */
	UStaticFastInstancedMesh* SpawnMeshFromPool(FString meshName)
	{
		PUN_CHECK(!meshes[meshName]);
		//PUN_LOG("SpawnMeshFromPool %s", *meshName);

		UStaticFastInstancedMesh* mesh = nullptr;
		for (int32 i = 0; i < meshPool.Num(); i++) {
			if (meshPool[i]->meshName.IsEmpty()) {
				mesh = meshPool[i];
				mesh->SetActive(true, true);
				mesh->AttachToComponent(this, FAttachmentTransformRules::KeepRelativeTransform);
			}
		}

		if (!mesh) {
			mesh = NewObject<UStaticFastInstancedMesh>(this);
			mesh->Init(this, _bucketCount, _forceUpdateTransform, hasCollision());
			mesh->poolIndex = meshPool.Num();
			meshPool.Add(mesh);
		}

		mesh->meshName = meshName;
		mesh->SetMeshState(meshStates[meshName]);

		mesh->ComponentTags.Empty(3);
		if (hasCollision()) {
			mesh->ComponentTags.Add(ToFName(_collisionTag));
			mesh->ComponentTags.Add(FName(*FString::FromInt(_meshId)));
			mesh->ComponentTags.Add(FName(*meshName));
		}

		//mesh->SetCastShadow(_castShadow);

		meshes[meshName] = mesh;

		return mesh;
	}

	void DespawnMesh(FString meshName)
	{
		meshes[meshName]->SetActive(false, true);
		meshes[meshName]->meshName.Empty();
		meshes[meshName]->Rename();
		GameDisplayUtils::SetCustomDepth(meshes[meshName], 0);
		meshes[meshName]->SetReceivesDecals(false); // Just in case this is constructionBaseHighlight
		
		meshes[meshName]->DetachFromComponent(FDetachmentTransformRules::KeepRelativeTransform);
		meshes[meshName] = nullptr;
	}

private:
	std::string _nameSuffix;
	int32 _bucketCount;
	bool _forceUpdateTransform;

	// Collision
	std::string _collisionTag; // having empty collisionTag disables collision and decrease performance impact. Required in init to set the body instance.
	int32 _meshId;

	bool _activeState;

	//bool _castShadow;
};
