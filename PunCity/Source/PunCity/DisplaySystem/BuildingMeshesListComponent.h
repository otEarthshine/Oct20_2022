// Pun Dumnernchanvanit's

#pragma once

#include "DisplaySystemDataSource.h"
#include "PunCity/BuildingMeshesComponent.h"
#include "Kismet/KismetArrayLibrary.h"
#include "Kismet/KismetMathLibrary.h"

/**
 * For showing building mesh or decal
 * - for UI/temporary display such as IconBaker, UI highlight, UI delay filler
 */
class BuildingMeshesListComponent
{
public:
	void Init(IDisplaySystemDataSource* dataSource) {
		_dataSource = dataSource;
	}
	
	UBuildingMeshesComponent* ShowBuildingMesh(Building& building, int customDepth = 0, bool receiveDecal = true);
	UBuildingMeshesComponent* ShowBuildingMesh(WorldTile2 tile, Direction faceDirection, const std::vector<ModuleTransform>& modules, int32 customDepthIndex = 0, bool receiveDecal = true);

	void ShowStorageMesh(TileArea area, WorldTile2 centerTile, int customDepth = 0) {
		auto buildingMeshes = GetBuildingMeshes();
		buildingMeshes->ShowStorageMesh(area, centerTile, _dataSource->assetLoader(), customDepth);
		buildingMeshes->SetWorldLocation(_dataSource->DisplayLocation(centerTile.worldAtom2()));
	}

	UDecalComponent* ShowDecal(TileArea area, UMaterial* material, bool useMaterialInstance = false);
	
	void AfterAdd();

	int buildingMeshesIndex = 0;
	TArray<UBuildingMeshesComponent*> _buildingMeshesList;

	int32 decalIndex = 0;
	TArray<UDecalComponent*> _decals;

	/*
	 * Delivery Arrow
	 */
	void ShowDeliveryArrow(FVector start, FVector end, bool isYellow, bool isSolid)
	{
		if (_deliveryArrowIndex >= _deliveryArrowMeshes.Num()) {
			auto mesh = PunUnrealUtils::CreateStaticMesh(_dataSource->componentToAttach());
			mesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
			mesh->SetReceivesDecals(true);

			mesh->SetStaticMesh(_dataSource->assetLoader()->DeliveryArrowMesh);
			mesh->SetMaterial(0, UMaterialInstanceDynamic::Create(_dataSource->assetLoader()->M_DeliveryArrow, _dataSource->componentToAttach()));
			
			_deliveryArrowMeshes.Add(mesh);
		}
		auto mesh = _deliveryArrowMeshes[_deliveryArrowIndex];

		float distance = FVector::Distance(start, end);
		float baseArrowSize = CoordinateConstants::DisplayUnitPerTile;
		float scaleScalar = distance / baseArrowSize;
		FVector scale(scaleScalar, 0.2, 20);
		FRotator rotation = UKismetMathLibrary::FindLookAtRotation(start, end);

		mesh->SetVisibility(true);
		mesh->SetWorldTransform(FTransform(rotation, start, scale));

		auto material = CastChecked<UMaterialInstanceDynamic>(mesh->GetMaterial(0));
		material->SetScalarParameterValue("SectionCount", scaleScalar);
		material->SetScalarParameterValue("IsYellow", isYellow);
		material->SetScalarParameterValue("IsSolid", isSolid);

		_deliveryArrowIndex++;
	}
	void AfterAdd_DeliveryArrow()
	{
		for (int32 i = _deliveryArrowIndex; i < _deliveryArrowMeshes.Num(); i++) {
			_deliveryArrowMeshes[i]->SetVisibility(false);
		}
		_deliveryArrowIndex = 0;
	}

private:
	int32 _deliveryArrowIndex = 0;
	TArray<UStaticMeshComponent*> _deliveryArrowMeshes;
	
private:
	UBuildingMeshesComponent* GetBuildingMeshes();
	
private:
	IDisplaySystemDataSource* _dataSource = nullptr;
};
