// Fill out your copyright notice in the Description page of Project Settings.


#include "StaticParticleSystemsComponent.h"

using namespace std;

void UStaticParticleSystemsComponent::Init(FString name, USceneComponent* parent, int tileIdCount)
{
	Rename(*name);
	AttachToComponent(parent, FAttachmentTransformRules::KeepRelativeTransform, NAME_None);

	SetMobility(EComponentMobility::Movable);

	RegisterComponent();

	SetRelativeLocation(FVector::ZeroVector);

	_keyToInstanceInfo.Init(tileIdCount);

	PUN_DEBUG_EXPR(_keysThisTick.clear());
}


void UStaticParticleSystemsComponent::Add(int32_t key, UParticleSystem* protoParticle, FTransform transform, int32_t state)
{
	PUN_CHECK(protoParticle);
	
	PUN_CHECK(key >= 0);
	PUN_CHECK_EDITOR(NoBug());
	PUN_CHECK_EDITOR(NoBug2());
	PUN_CHECK_EDITOR(find(_keysThisTick.begin(), _keysThisTick.end(), key) == _keysThisTick.end());
	PUN_DEBUG_EXPR(_keysThisTick.push_back(key));

	int8_t stateMod = static_cast<int8_t>(state % 127);

	InstanceInfo info;
	if (_keyToInstanceInfo.TryGet(key, info))
	{
		//_particleSystems[info.instanceIndex]->SetRelativeTransform(transform);
		SetParticleActive(_particleSystems[info.instanceIndex], true);
	}
	else
	{
		// Create New Instance
		int instanceIndex;
		if (_disabledInstanceIndices.Num() > 0) {
			// Reuse
			instanceIndex = _disabledInstanceIndices.Last();
			_disabledInstanceIndices.RemoveAt(_disabledInstanceIndices.Num() - 1);

			//PUN_LOG("Reuse tileId %d", tileId);

			PUN_CHECK(_particleSystems.Num() > instanceIndex);
		}
		else
		{
			// Make new instance
			instanceIndex = _particleSystems.Num();

			auto particleComp = NewObject<UParticleSystemComponent>(this);
			particleComp->Rename(*(GetName() + FString::FromInt(instanceIndex)));
			particleComp->AttachToComponent(this, FAttachmentTransformRules::KeepRelativeTransform, NAME_None);
			particleComp->RegisterComponent();
			_particleSystems.Add(particleComp);

			PUN_CHECK(_particleSystems.Num() > instanceIndex);
		}

		_particleSystems[instanceIndex]->SetRelativeTransform(transform);
		_particleSystems[instanceIndex]->SetTemplate(protoParticle);
		SetParticleActive(_particleSystems[instanceIndex], true);

		//PUN_LOG("Adding Smoke:%d  local:%s, world:%s", instanceIndex, *transform.GetLocation().ToString(), *_particleSystems[instanceIndex]->GetComponentToWorld().GetLocation().ToString());

		//PUN_CHECK("Add tileId %d, %d, name:%s", tileId, instanceIndex, *_instancedMesh->GetFName().ToString());
		PUN_CHECK(instanceIndex >= 0);
		PUN_CHECK(instanceIndex < 50000); //TODO:

		info.instanceIndex = instanceIndex;
		info.stateMod = stateMod;
		_keyToInstanceInfo.Add(key, info);
	}

	PUN_CHECK_EDITOR(NoBug2());
	PUN_CHECK_EDITOR(NoBug());
	PUN_CHECK(_particleSystems.Num() > info.instanceIndex);

	_keyToInstanceInfo.SetInUse(key, info);

	PUN_CHECK((_keyToInstanceInfo.InUseTick(key) & 1) == (TimeDisplay::Ticks() & 1));
}

void UStaticParticleSystemsComponent::AfterAdd()
{
	//SCOPE_CYCLE_COUNTER(STAT_PunDisplayFastMeshAfterAdd);

	PUN_CHECK_EDITOR(NoBug());
	PUN_CHECK_EDITOR(NoBug2());

	//PUN_LOG("_keysThisTick clear %d", _keysThisTick.size());

	static std::vector<InstanceInfo> unusedList;
	unusedList.clear();

	PUN_DEBUG_EXPR(int beforeRemoveCount = _keyToInstanceInfo.count());

	{
		// Note: Tested 0.0ms
		//SCOPE_TIMER("Particle->AfterAdd: _keyToInstanceInfo RemoveUnused");
		_keyToInstanceInfo.RemoveUnused(unusedList);
	}

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

	{
		// Note: Tested 0.0ms
		//SCOPE_TIMER("Particle->AfterAdd: Despawn");
		
		for (const InstanceInfo& info : unusedList) {
			//PUN_ALOG_ALL("FastMesh", "Despawn ticks:%d index:%d", TimeDisplay::Ticks(), info.instanceIndex);
			Despawn(info.instanceIndex);
		}
	}

	PUN_CHECK_EDITOR(NoBug());
}

void UStaticParticleSystemsComponent::Despawn(int instanceIndex)
{
	SetParticleActive(_particleSystems[instanceIndex], false);
	_disabledInstanceIndices.Add(instanceIndex);

	PUN_LOG("Remove Smoke: %d", instanceIndex);
}

void UStaticParticleSystemsComponent::SetActive(bool bNewActive, bool bReset)
{
	PUN_CHECK_EDITOR(NoBug2());
	PUN_CHECK_EDITOR(NoBug());

	Super::SetActive(bNewActive, bReset);

	// On disable.
	if (!bNewActive) {
		_disabledInstanceIndices.Empty();
		for (int32 i = 0; i < _particleSystems.Num(); i++) {
			SetParticleActive(_particleSystems[i], false);
			_disabledInstanceIndices.Add(i);

			// TODO: instant hide ????
			//_particleSystems[i]->SetVisibility(bNewActive);
		}

		_keyToInstanceInfo.Clear();
	}

	PUN_CHECK_EDITOR(NoBug());
	PUN_CHECK_EDITOR(NoBug2());
}