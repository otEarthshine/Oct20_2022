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
	struct InstanceInfo
	{
		int16_t instanceIndex;
		int8_t stateMod; // state modded to fit in int8_t .. modding is fine here since we just want to know if the state changed
	};

	UStaticParticleSystemsComponent() {
		PrimaryComponentTick.bCanEverTick = false;
	}

	void Init(FString name, USceneComponent* parent, int tileIdCount);

	// Add if necessary. 
	// -tileId that was already added won't be touched
	// -tileId that wasn't added for an update will get despawned automatically
	void Add(int32_t key, UParticleSystem* protoParticle, FTransform transform, int32_t state);
	void AfterAdd();

	void SetActive(bool bNewActive, bool bReset = false) override;

	// Debug...
	PUN_DEBUG_EXPR(
		bool NoBug() {
			return _keyToInstanceInfo.count() + _disabledInstanceIndices.Num() == _particleSystems.Num();
		}
		bool NoBug2() {
			return _keysThisTick.size() <= _keyToInstanceInfo.count();
		}
		std::vector<int32_t> _keysThisTick;
	);

private:
	void Despawn(int instanceIndex);

	static void SetParticleActive(UParticleSystemComponent* particleComponent, bool bNewActive) {
		particleComponent->bSuppressSpawning = !bNewActive;
	}


private:
	UPROPERTY() TArray<int32> _disabledInstanceIndices;
	CycleMap<InstanceInfo> _keyToInstanceInfo;

	UPROPERTY() TArray<UParticleSystemComponent*> _particleSystems;
};

//USTRUCT()
//struct FStaticParticleSystemsComponent_AllTypes
//{
//	GENERATED_BODY()
//
//	UPROPERTY() TArray<UStaticParticleSystemsComponent*> particlesByEnum;
//
//	void SetActive(bool active) {
//		for (int32 i = 0; i < particlesByEnum.Num(); i++) {
//			particlesByEnum[i]->SetActive(true);
//		}
//	}
//
//	void SetRelativeLocation(FVector location) {
//		for (int32 i = 0; i < particlesByEnum.Num(); i++) {
//			particlesByEnum[i]->SetRelativeLocation(location);
//		}
//	}
//
//	void AfterAdd() {
//		for (int32 i = 0; i < particlesByEnum.Num(); i++) {
//			particlesByEnum[i]->AfterAdd();
//		}
//	}
//};