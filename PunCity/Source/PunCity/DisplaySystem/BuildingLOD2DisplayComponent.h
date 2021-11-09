// Pun Dumnernchanvanit's

#pragma once

#include "DisplaySystemComponent.h"
#include "StaticFastInstancedMeshesComp.h"


#include "BuildingLOD2DisplayComponent.generated.h"

/**
 * 
 */
UCLASS()
class PROTOTYPECITY_API UBuildingLOD2DisplayComponent : public UDisplaySystemComponent
{
	GENERATED_BODY()
public:
	UBuildingLOD2DisplayComponent()
	{
		// Note: Don't parent these to terrainMesh since they are scaled....
		_buildingsMeshes = CreateDefaultSubobject<UStaticFastInstancedMeshesComp>("Map_BuildingMeshes");
		//_buildingsMeshes->AttachToComponent(terrainMesh, FAttachmentTransformRules::KeepRelativeTransform);
		//_buildingsMeshes->SetRelativeLocationAndRotation(FVector::ZeroVector, FRotator::ZeroRotator);
	}
	

	void InitAnnotations();
	void RefreshAnnotations();

	void TickBuildingLOD2()
	{
		/*
		 * Buildings/Georesource
		 */
		bool mapCityVisible = !gameManager()->ZoomDistanceBelow(WorldZoomTransition_BuildingsMini);
		if (mapCityVisible && !_buildingsMeshes->IsActive())
		{
			_buildingsMeshes->SetActive(true, true);
			RefreshAnnotations();
		}
		else if (!mapCityVisible && _buildingsMeshes->IsActive())
		{
			_buildingsMeshes->SetActive(false, true);
		}

		// Initial map refresh happens after 3 sec
		if (Time::Ticks() > 3 * Time::TicksPerSecond &&
			!_didFirstRefreshAnnotation)
		{
			_didFirstRefreshAnnotation = true;

			if (mapCityVisible) {
				//_georesourceEnumToMesh->SetActive(true, true);
				_buildingsMeshes->SetActive(true, true);
				RefreshAnnotations();
			}
			else {
				//_georesourceEnumToMesh->SetActive(false, true);
				_buildingsMeshes->SetActive(false, true);
			}
		}

		// Building Meshes
		{
			_buildingsMeshes->SetWorldLocation(MapUtil::DisplayLocation(gameManager()->cameraAtom(), WorldAtom2::Zero));
		}
	}


	

	UPROPERTY(EditAnywhere, BlueprintReadWrite) UStaticFastInstancedMeshesComp* _buildingsMeshes;
	// TODO: later on, can change building by its type??? not refresh the whole thing? (buildings not shared anyway)

	bool _didFirstRefreshAnnotation = false;
};
