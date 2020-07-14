// Fill out your copyright notice in the Description page of Project Settings.

#include "StaticFastInstancedMesh.h"

#include "Materials/MaterialInstanceDynamic.h"
#include "Components/LineBatchComponent.h"
#include "Engine/StaticMesh.h"

DECLARE_CYCLE_STAT(TEXT("PUN: [Display]FastMesh.BeforeAdd"), STAT_PunDisplayFastMeshBeforeAdd, STATGROUP_Game);
DECLARE_CYCLE_STAT(TEXT("PUN: [Display]FastMesh.Add"), STAT_PunDisplayFastAdd, STATGROUP_Game);
DECLARE_CYCLE_STAT(TEXT("PUN: [Display]FastMesh.AfterAdd"), STAT_PunDisplayFastMeshAfterAdd, STATGROUP_Game);
DECLARE_CYCLE_STAT(TEXT("PUN: [Display]FastMesh.SetActive"), STAT_PunDisplayFastMeshSetActive, STATGROUP_Game);

using namespace std;

//TODO: get rid of FStaticFastInstancedMeshState parameters???
void UStaticFastInstancedMesh::Init(USceneComponent* parent, int tileIdCount, bool forceUpdateTransform, bool hasCollision)
{
	// PUN_LOG("Init %s", *name);
	//check(mesh);

	AttachToComponent(parent, FAttachmentTransformRules::KeepRelativeTransform, NAME_None);

	SetMobility(EComponentMobility::Movable);

	// MoveComp/TransformRenderData low performance diff turning on QueryOnly vs NoCollision (~2.7ms to 3ms)
	// Therefore this is worth it for simplicity
	BodyInstance.SetCollisionEnabled(hasCollision ? ECollisionEnabled::QueryOnly : ECollisionEnabled::NoCollision, false);

	SetGenerateOverlapEvents(false);
	
	SetReceivesDecals(false);
	//SetRenderCustomDepth(true);
	//SetCustomDepthStencilValue(1);
	
	RegisterComponent();

	// Test if this fix crash  in UpdateGlobalDistanceField ... seems to do so???
	bAffectDistanceFieldLighting = false;
	SetDistanceFieldSelfShadowBias(0);

	//if (collisionOnly) {
	//	SetMaterial(0, nullptr);
	//	SetVisibility(false);
	//} else {
	//	SetMaterial(0, material ? material : mesh->GetMaterial(0));
	//}

	//SetStaticMesh(mesh);

	//SetMeshState({ mesh, material, hasCollision, collisionOnly });
	
	SetRelativeLocation(FVector::ZeroVector);


	_keyToInstanceInfo.Init(tileIdCount);
	_instanceIndexToObjectId.Init(tileIdCount);

	_forceUpdateTransform = forceUpdateTransform;
	
	//_hasCollision = hasCollision;
	//_collisionOnly = collisionOnly;

	//PUN_LOG("Init tree %s, %d", *_instancedMesh->GetFName().ToString(), _instancedMesh);

	PUN_DEBUG_EXPR(_keysThisTick.clear());
}


void UStaticFastInstancedMesh::Add(int32_t key, FTransform transform, int32_t state, int32_t objectId)
{
	SCOPE_CYCLE_COUNTER(STAT_PunDisplayFastAdd);


	PUN_CHECK(GetStaticMesh());
	PUN_CHECK(key >= 0);
	PUN_CHECK_EDITOR(NoBug());
	PUN_CHECK_EDITOR(NoBug2());
	PUN_CHECK(!CppUtils::Contains(_keysThisTick, key));
	//PUN_CHECK(find(_keysThisTick.begin(), _keysThisTick.end(), key) == _keysThisTick.end());
	PUN_DEBUG_EXPR(_keysThisTick.push_back(key));

	int8_t stateMod = static_cast<int8_t>(state % 127);

	InstanceInfo info;
	if (!_keyToInstanceInfo.TryGet(key, info))
	{
		int instanceIndex;
		if (_disabledInstanceIndices.Num() > 0) {
			// Reuse
			instanceIndex = _disabledInstanceIndices.Last();
			_disabledInstanceIndices.RemoveAt(_disabledInstanceIndices.Num() - 1);
			UpdateInstanceTransform(instanceIndex, transform);
			//UE_LOG(LogTemp, Error, TEXT("Reuse tileId %d"), tileId);
			
			check(GetInstanceCount() > instanceIndex);
			//PUN_ALOG("FastMesh", key, ".FastMesh.add_FromTrash id:%d tick%d", key, TimeDisplay::Ticks());
		}
		else
		{
			// Make new instance
			instanceIndex = GetInstanceCount();
			AddInstance(transform);

			check(GetInstanceCount() > instanceIndex);
			//PUN_ALOG("FastMesh", key, ".FastMesh.add_NewCount id:%d tick%d", key, TimeDisplay::Ticks());
		}
		//UE_LOG(LogTemp, Error, TEXT("Add tileId %d, %d, name:%s"), tileId, instanceIndex, *_instancedMesh->GetFName().ToString());
		check(instanceIndex >= 0);
		check(instanceIndex < 50000); //TODO:

		info.instanceIndex = instanceIndex;
		info.stateMod = stateMod;
		_keyToInstanceInfo.Add(key, info);
		_needUpdate = true;

		if (_hasCollision) {
			check(objectId >= 0);
			_instanceIndexToObjectId.Add(instanceIndex, objectId);
			//UE_LOG(LogTemp, Error, TEXT("_instanceIndexToObjectId %s Add %d objId:%d"), *GetFName().ToString(), instanceIndex, objectId);
		}
	}
	else {
		//PUN_ALOG("FastMesh", key, ".FastMesh.add_ExistingCount id:%d tick%d", key, TimeDisplay::Ticks());
	}

	PUN_CHECK_EDITOR(NoBug2());
	PUN_CHECK_EDITOR(NoBug());
	PUN_CHECK(GetInstanceCount() > info.instanceIndex);

	// If this tile's age is some threshold older than display age, we need to update the display's transform
	if (_forceUpdateTransform || info.stateMod != stateMod) {
		UpdateInstanceTransform(info.instanceIndex, transform);
		info.stateMod = stateMod;
		_needUpdate = true;
	}

	_keyToInstanceInfo.SetInUse(key, info);

	PUN_CHECK((_keyToInstanceInfo.InUseTick(key) & 1) == (TimeDisplay::Ticks() & 1));

	if (_hasCollision) {
		_instanceIndexToObjectId.SetInUse(info.instanceIndex, objectId);
		//UE_LOG(LogTemp, Error, TEXT("_instanceIndexToObjectId %s Set %d objId:%d"), *GetFName().ToString(), info.instanceIndex, objectId);
	}
}

void UStaticFastInstancedMesh::AfterAdd()
{
	SCOPE_CYCLE_COUNTER(STAT_PunDisplayFastMeshAfterAdd);

	PUN_CHECK_EDITOR(NoBug());
	PUN_CHECK_EDITOR(NoBug2());

	//UE_LOG(LogTemp, Error, TEXT("_keysThisTick clear %d"), _keysThisTick.size());

	static std::vector<InstanceInfo> unusedList;
	unusedList.clear();

	PUN_DEBUG_EXPR(int beforeRemoveCount = _keyToInstanceInfo.count());

	_keyToInstanceInfo.RemoveUnused(unusedList);


	// Compare _keyToInstanceInfo to _keysThisTick to make sure they match.
	PUN_DEBUG_EXPR(
	std::vector<int32_t> keysLeft = _keyToInstanceInfo.GetAll();
	// Check that keys added this ticks are still alive in _keyToInstanceInfo
	for (int i = 0; i < _keysThisTick.size(); i++) {
		PUN_ACHECK(std::find(keysLeft.begin(), keysLeft.end(), _keysThisTick[i]) != keysLeft.end(), "FastMesh", _keysThisTick[i]);
	}
	// **Note that it is ok for _keyToInstanceInfo to have no _keysThisTick to match. This is because of how despawning works (using even/odd tick check)
	);
	PUN_DEBUG_EXPR(_keysThisTick.clear());

	for (const InstanceInfo& info : unusedList) {
		//PUN_ALOG_ALL("FastMesh", "Despawn ticks:%d index:%d", TimeDisplay::Ticks(), info.instanceIndex);
		Despawn(info.instanceIndex);
	}
	if (!unusedList.empty()) {
		_needUpdate = true;
	}

	if (_hasCollision) {
		static std::vector<int32> unusedList2;
		_instanceIndexToObjectId.RemoveUnused(unusedList2);
	}

	PUN_CHECK_EDITOR(NoBug());

	if (!_needUpdate) {
		return;
	}

	MarkRenderStateDirty();
	//PUN_LOG("after tree: %lu", _instancedMesh->GetInstanceCount());

	//// Turn off the instance
	//int32 instanceCount = GetInstanceCount();
	//Super::SetActive(instanceCount > 0);
	//if (instanceCount > 0 && !bVisible) {
	//	SetVisibility(true);
	//} else if(instanceCount == 0 && bVisible) {
	//	SetVisibility(false);
	//}

	_needUpdate = false;
}

void UStaticFastInstancedMesh::Despawn(int instanceIndex)
{
	FTransform transform(FRotator::ZeroRotator, FVector::ZeroVector, FVector::ZeroVector);
	//FTransform oldTransform;
	//GetInstanceTransform(instanceIndex, oldTransform);
	//FTransform transform(FRotator::ZeroRotator, oldTransform.GetLocation() + FVector(0 , 0, 100), FVector::OneVector * 0.1);

	UpdateInstanceTransform(instanceIndex, transform);
	_disabledInstanceIndices.Add(instanceIndex);
}

void UStaticFastInstancedMesh::SetActive(bool bNewActive, bool bReset)
{
	SCOPE_CYCLE_COUNTER(STAT_PunDisplayFastMeshSetActive);

	PUN_CHECK_EDITOR(NoBug2());
	PUN_CHECK_EDITOR(NoBug());

	if (!_noBasePass) {
		SetVisibility(bNewActive);
	}
	

	Super::SetActive(bNewActive, bReset);

	if (!bNewActive) {
		ClearInstances();
		PUN_CHECK(GetInstanceCount() == 0);
		_disabledInstanceIndices.Empty();

		_keyToInstanceInfo.Clear();
		_instanceIndexToObjectId.Clear();

		PUN_DEBUG_EXPR(_keysThisTick.clear());
	}

	PUN_CHECK_EDITOR(NoBug());
	PUN_CHECK_EDITOR(NoBug2());
}