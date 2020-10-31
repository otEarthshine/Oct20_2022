// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "PunCity/Simulation/GameSimulationInfo.h"
#include "Particles/ParticleSystemComponent.h"
#include "TerrainMapComponent.h"
#include "StaticParticleSystemsComponent.generated.h"


UCLASS( ClassGroup=(Custom), meta=(BlueprintSpawnableComponent) )
class UStaticParticleSystemsComponent : public USceneComponent
{
	GENERATED_BODY()
public:
	UStaticParticleSystemsComponent() {
		PrimaryComponentTick.bCanEverTick = false;
	}

	UPROPERTY() TMap<int32, UParticleSystemComponent*> _lastParticles;
	UPROPERTY() TMap<int32, UParticleSystemComponent*> _newParticles;

	UPROPERTY() USceneComponent* parent;

	void AfterAdd()
	{
		// Particles Check Map to see what is no longer in use..
		for (auto pair : _lastParticles) {
			if (!_newParticles.Contains(pair.Key)) {
				pair.Value->ReleaseToPool();
			}
		}
		_lastParticles = _newParticles;
		_newParticles.Empty();
	}

	void Add(int32 particleKey, const FTransform& finalTransform, ParticleEnum particleEnum, UAssetLoaderComponent* assetLoader)
	{
		// Check _lastParticles for reuse
		UParticleSystemComponent* comp = nullptr;
		if (_lastParticles.Contains(particleKey)) {
			comp = _lastParticles[particleKey];
			if (!comp->IsValidLowLevel() || !IsValid(comp)) {
				comp = nullptr;
			}
		}

		// Create a new particle if needed
		if (comp == nullptr)
		{
			//comp = UGameplayStatics::SpawnEmitterAtLocation(this, _assetLoader->particleSystem(ParticleEnum::DemolishDust),
			//																		finalTransform, false, EPSCPoolMethod::ManualRelease);
			UParticleSystem* particleSys = assetLoader->particleSystem(particleEnum);
			//comp = UGameplayStatics::SpawnEmitterAtLocation(GetWorld(), particleSys, FTransform::Identity, false, EPSCPoolMethod::ManualRelease);
			comp = UGameplayStatics::SpawnEmitterAttached(particleSys, this, NAME_None,
				FVector::ZeroVector, FRotator::ZeroRotator, FVector(1),
				EAttachLocation::KeepRelativeOffset, false, EPSCPoolMethod::ManualRelease);
			
			//comp->AttachToComponent(this, FAttachmentTransformRules::KeepRelativeTransform);
			//comp->AttachToComponent(parent, FAttachmentTransformRules::KeepRelativeTransform);
		}

		// Move the particle to the correct location
		comp->SetRelativeTransform(finalTransform);
		//comp->SetWorldTransform(finalTransform);

		_newParticles.Add(particleKey, comp);
	}

	void SetClusterActive(bool isActive)
	{
		if (!isActive) {
			for (auto pair : _lastParticles) {
				pair.Value->Deactivate();
				pair.Value->ReleaseToPool();
			}
			_lastParticles.Empty();
			_newParticles.Empty();
		}
	}
	
//	struct InstanceInfo
//	{
//		int16_t instanceIndex;
//		int8_t stateMod; // state modded to fit in int8_t .. modding is fine here since we just want to know if the state changed
//	};
//
//	UStaticParticleSystemsComponent() {
//		PrimaryComponentTick.bCanEverTick = false;
//	}
//
//	void Init(FString name, USceneComponent* parent, int tileIdCount);
//
//	// Add if necessary. 
//	// -tileId that was already added won't be touched
//	// -tileId that wasn't added for an update will get despawned automatically
//	void Add(int32_t key, UParticleSystem* protoParticle, FTransform transform, int32_t state);
//	void AfterAdd();
//
//	void SetActive(bool bNewActive, bool bReset = false) override;
//
//	// Debug...
//	PUN_DEBUG_EXPR(
//		bool NoBug() {
//			return _keyToInstanceInfo.count() + _disabledInstanceIndices.Num() == _particleSystems.Num();
//		}
//		bool NoBug2() {
//			return _keysThisTick.size() <= _keyToInstanceInfo.count();
//		}
//		std::vector<int32_t> _keysThisTick;
//	);
//
//private:
//	void Despawn(int instanceIndex);
//
//	static void SetParticleActive(UParticleSystemComponent* particleComponent, bool bNewActive) {
//		particleComponent->bSuppressSpawning = !bNewActive;
//
//		// TODO: move particles doesn't work either
//		//particleComponent->SetRelativeLocation(particleComponent->GetRelativeLocation() + FVector(0, 0, -1000)); // move it to another location...
//	}
//
//
//private:
//	UPROPERTY() TArray<int32> _disabledInstanceIndices;
//	CycleMap<InstanceInfo> _keyToInstanceInfo;
//
//	UPROPERTY() TArray<UParticleSystemComponent*> _particleSystems;
};