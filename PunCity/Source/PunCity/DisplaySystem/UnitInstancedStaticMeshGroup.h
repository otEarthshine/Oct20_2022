// Pun Dumnernchanvanit's

#pragma once

#include "GameDisplayInfo.h"
#include "UnitInstancedStaticMeshComponent.h"
#include "Components/SceneComponent.h"

#include "UnitInstancedStaticMeshGroup.generated.h"


UCLASS( ClassGroup=(Custom), meta=(BlueprintSpawnableComponent) )
class PROTOTYPECITY_API UUnitInstancedStaticMeshGroup : public USceneComponent
{
	GENERATED_BODY()
public:
	
	UUnitInstancedStaticMeshGroup()
	{
		for (int32 i = 0; i < UnitDisplayEnumCount; i++) {
			meshes.Add(CreateDefaultSubobject<UUnitInstancedStaticMeshComponent>(FName(FString("UnitMeshesNew") + FString::FromInt(i))));
		}
	}

	void Init() {
		for (int32 i = 0; i < UnitDisplayEnumCount; i++) {
			meshes[i]->SetCollisionEnabled(ECollisionEnabled::NoCollision);
		}
	}

	UPROPERTY() TArray<UUnitInstancedStaticMeshComponent*> meshes;

	void AddProtoMesh(UnitDisplayEnum displayEnum, UStaticMesh* mesh) {
		meshes[static_cast<int32>(displayEnum)]->SetStaticMesh(mesh);
	}

	void Add(UnitDisplayEnum displayEnum, int32 unitId, FTransform& transform) {
		meshes[static_cast<int32>(displayEnum)]->Add(unitId, transform);
	}


	void AfterAdd() {
		for (int32 i = 0; i < meshes.Num(); i++) {
			meshes[i]->AfterAdd();
		}
	}

	void SetCustomDepth(UnitDisplayEnum displayEnum, int32 customDepthIndex) {
		GameDisplayUtils::SetCustomDepth(meshes[static_cast<int32>(displayEnum)], customDepthIndex);
	}

	
};
