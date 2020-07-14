// Pun Dumnernchanvanit's

#pragma once

#include "DisplaySystemDataSource.h"
#include "PunCity/BuildingMeshesComponent.h"

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
	
	UBuildingMeshesComponent* ShowBuildingMesh(Building& building, int customDepth = 0);
	UBuildingMeshesComponent* ShowBuildingMesh(WorldTile2 tile, Direction faceDirection, const std::vector<ModuleTransform>& modules, int32 customDepthIndex = 0);

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
	
private:
	UBuildingMeshesComponent* GetBuildingMeshes();
	
private:
	IDisplaySystemDataSource* _dataSource = nullptr;
};
