// Pun Dumnernchanvanit's

#pragma once

#include "Components/InstancedStaticMeshComponent.h"
#include "PunCity/Simulation/GameSimulationInfo.h"

#include "UnitInstancedStaticMeshComponent.generated.h"

/**
 * 
 */
UCLASS()
class PROTOTYPECITY_API UUnitInstancedStaticMeshComponent : public UInstancedStaticMeshComponent
{
	GENERATED_BODY()
public:
	UPROPERTY() TMap<int32, int32> unitIdToInstanceIndex;
	UPROPERTY() TSet<int32> unitIdAddedThisTick;
	UPROPERTY() TArray<int32> disabledIndices;

	UUnitInstancedStaticMeshComponent() {
		SetReceivesDecals(false);
	}

	void Add(int32 unitId, FTransform& transform)
	{
		unitIdAddedThisTick.Add(unitId);

		// Get Existing Unit
		if (unitIdToInstanceIndex.Contains(unitId)) {
			UpdateInstanceTransform(unitIdToInstanceIndex[unitId], transform);
			return;
		}

		// Spawn Unit from disabledIndices
		if (disabledIndices.Num() > 0) {
			int32 disabledIndex = disabledIndices.Pop(false);
			unitIdToInstanceIndex.Add(unitId, disabledIndex);
			UpdateInstanceTransform(disabledIndex, transform);
			return;
		}

		unitIdToInstanceIndex.Add(unitId, GetInstanceCount());
		AddInstance(transform);
	}


	void AfterAdd()
	{
		TMap<int32, int32> unitIdToInstanceIndexCopy = unitIdToInstanceIndex;
		for (auto pair : unitIdToInstanceIndexCopy) {
			if (!unitIdAddedThisTick.Contains(pair.Key)) {
				unitIdToInstanceIndex.Remove(pair.Key);
				disabledIndices.Add(pair.Value);
				UpdateInstanceTransform(pair.Value, FTransform(FVector(0, 0, -5000)));
			}
		}

		unitIdAddedThisTick.Empty();
		MarkRenderStateDirty();
	}

	
};
