// Pun Dumnernchanvanit's

#pragma once

#include "UnitInstancedStaticMeshComponent.h"
#include "Components/SceneComponent.h"
#include "PunCity/Simulation/GameSimulationInfo.h"

#include "UnitInstancedMeshComponent.generated.h"


UCLASS( ClassGroup=(Custom), meta=(BlueprintSpawnableComponent) )
class PROTOTYPECITY_API UUnitInstancedMeshComponent : public USceneComponent
{
	GENERATED_BODY()
public:

	//UUnitInstancedMeshComponent()
	//{
	//	for (int32 i = 0; i < UnitDisplayEnumCount; i++) {
	//		meshes.Add(CreateDefaultSubobject<UUnitInstancedStaticMeshComponent>(FName(FString("UnitMeshesNew") + FString::FromInt(i))));
	//	}
	//}

	//UPROPERTY() TArray<UUnitInstancedStaticMeshComponent*> meshes;

	//void AddProtoMesh(UnitDisplayEnum displayEnum, UStaticMesh* mesh) {
	//	meshes[static_cast<int32>(displayEnum)]->SetStaticMesh(mesh);
	//}

	//void Add(UnitDisplayEnum displayEnum, int32 unitId, FTransform& transform) {
	//	meshes[static_cast<int32>(displayEnum)]->Add(unitId, transform);
	//}


	//void AfterAdd() {
	//	for (int32 i = 0; i < meshes.Num(); i++) {
	//		meshes[i]->AfterAdd();
	//	}
	//}
	//

	//
};
