// Pun Dumnernchanvanit's

#pragma once

#include "CoreMinimal.h"
#include "PunCity/DisplaySystem/DisplaySystemComponent.h"
#include "Components/StaticMeshComponent.h"

#include "RegionLandmarkDisplayComponent.generated.h"


USTRUCT()
struct FLandmarkMesh
{
	GENERATED_BODY();

	UPROPERTY() UStaticMeshComponent* mesh = nullptr;
	UPROPERTY() bool justUpdated;

	FLandmarkMesh() : justUpdated(false) {}
	FLandmarkMesh(UStaticMeshComponent* mesh) : mesh(mesh), justUpdated(true) {}
};

/**
 * 
 */
UCLASS()
class URegionLandmarkDisplayComponent : public UDisplaySystemComponent
{
	GENERATED_BODY()
public:

	void UpdateLandmarkDisplay(std::vector<int32_t> sampleRegionIds)
	{
		GeoresourceSystem& georesourceSystem = simulation().georesourceSystem();
		const float centerShift = CoordinateConstants::RegionCenterDisplayUnits;

		// Prepare
		for (auto& it : _regionIdToMesh) {
			it.Value.justUpdated = false;
		}

		// Set Meshes
		for (int32 regionId: sampleRegionIds)
		{
			GeoresourceNode node = georesourceSystem.georesourceNode(regionId);
			if (node.georesourceEnum != GeoresourceEnum::None &&
				node.info().isLandmark())
			{
				WorldRegion2 region(regionId);
				WorldAtom2 objectAtomLocation(region.x * CoordinateConstants::AtomsPerRegion,
												region.y * CoordinateConstants::AtomsPerRegion);

				FVector displayLocation = MapUtil::DisplayLocation(gameManager()->cameraAtom(), objectAtomLocation);
				displayLocation += FVector(centerShift + 5, centerShift + 5, 0);
				
				FLandmarkMesh& landmark = GetMesh(regionId);
				landmark.mesh->SetRelativeLocation(displayLocation);
				
				landmark.mesh->ComponentTags.Empty();
				landmark.mesh->ComponentTags.Add(TEXT("Georesource"));
				landmark.mesh->ComponentTags.Add(*FString::FromInt(regionId));
				
				landmark.justUpdated = true;
			}
		}

		// Clear any unused meshes
		TArray<int32> regionIds;
		_regionIdToMesh.GetKeys(regionIds);
		for (int32 curRegionId : regionIds) {
			FLandmarkMesh landmark = _regionIdToMesh[curRegionId];
			if (!landmark.justUpdated) {
				landmark.mesh->DestroyComponent();
				_regionIdToMesh.Remove(curRegionId);
			}
		}
	
	}

private:
	FLandmarkMesh& GetMesh(int32_t regionId)
	{
		if (_regionIdToMesh.Contains(regionId)) {
			return _regionIdToMesh[regionId];
		}
		return SpawnMesh(regionId);
	}

	FLandmarkMesh& SpawnMesh(int32 regionId)
	{
		//SCOPE_TIMER("Region - Georesource");

		GeoresourceSystem& georesourceSystem = simulation().georesourceSystem();
		GeoresourceNode node = georesourceSystem.georesourceNode(regionId);

		TArray<UStaticMesh*> meshes = _assetLoader->georesourceMesh(node.georesourceEnum).meshes;

		UStaticMeshComponent* meshComp = PunUnrealUtils::CreateStaticMesh(this);
		meshComp->SetStaticMesh(meshes[0]);
		meshComp->SetVisibility(true);
		meshComp->SetCollisionEnabled(ECollisionEnabled::QueryOnly);

		_regionIdToMesh.Add(regionId, FLandmarkMesh(meshComp));

		return _regionIdToMesh[regionId];
	}

private:
	UPROPERTY() TMap<int32, FLandmarkMesh> _regionIdToMesh;
};
