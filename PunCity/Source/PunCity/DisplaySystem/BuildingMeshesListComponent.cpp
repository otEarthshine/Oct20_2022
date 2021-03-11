// Pun Dumnernchanvanit's


#include "BuildingMeshesListComponent.h"
#include "Components/DecalComponent.h"
#include "Materials/MaterialInstanceDynamic.h"

UBuildingMeshesComponent* BuildingMeshesListComponent::GetBuildingMeshes()
{
	PUN_CHECK(buildingMeshesIndex <= _buildingMeshesList.Num());
	if (buildingMeshesIndex == _buildingMeshesList.Num()) {
		auto buildingMeshes = NewObject<UBuildingMeshesComponent>(_dataSource->componentToAttach());
		buildingMeshes->AttachToComponent(_dataSource->componentToAttach(), FAttachmentTransformRules::KeepRelativeTransform, NAME_None);
		buildingMeshes->RegisterComponent();
		_buildingMeshesList.Add(buildingMeshes);
	}
	
	return _buildingMeshesList[buildingMeshesIndex++];
}

/*
 * For UI showing buildings
 *  Note: Bridge uses this..
 */
UBuildingMeshesComponent* BuildingMeshesListComponent::ShowBuildingMesh(Building& building, int customDepth, bool receiveDecal)
{
	UBuildingMeshesComponent* buildingMeshes = GetBuildingMeshes();

	auto& modules = _dataSource->displayInfo().GetDisplayModules(building.buildingEnum(), building.displayVariationIndex()).transforms;

	if (building.isEnum(CardEnum::Bridge) ||
		building.isEnum(CardEnum::IntercityBridge))
	{
		buildingMeshes->ShowBridge(building, _dataSource->assetLoader(), customDepth);
	}
	else if (building.isEnum(CardEnum::Tunnel)) {
		buildingMeshes->ShowTunnel(building, _dataSource->assetLoader(), customDepth);
	}
	else if (building.isEnum(CardEnum::StorageYard)) {
		buildingMeshes->ShowStorageMesh(building, _dataSource->assetLoader(), customDepth);
	}
	else {
		buildingMeshes->Show(building.faceDirection(), modules, _dataSource->assetLoader(), customDepth, receiveDecal);
	}

	buildingMeshes->SetWorldLocation(_dataSource->DisplayLocation(building.centerTile().worldAtom2()));

	return buildingMeshes;
}

UBuildingMeshesComponent* BuildingMeshesListComponent::ShowBuildingMesh(WorldTile2 tile, Direction faceDirection, const std::vector<ModuleTransform>& modules, int32 customDepthIndex, bool receiveDecal)
{
	UBuildingMeshesComponent* buildingMeshes = GetBuildingMeshes();
	
	buildingMeshes->Show(faceDirection, modules, _dataSource->assetLoader(), customDepthIndex, receiveDecal);
	buildingMeshes->SetWorldLocation(_dataSource->DisplayLocation(tile.worldAtom2()));

	return buildingMeshes;
}

UDecalComponent* BuildingMeshesListComponent::ShowDecal(TileArea area, UMaterial* material, bool useMaterialInstance)
{
	UDecalComponent* decal = PunUnrealUtils::ShowDecal(area.trueCenterAtom(), area.size(), _decals, decalIndex, _dataSource->componentToAttach(), material, _dataSource, useMaterialInstance);
	return decal;
}

void BuildingMeshesListComponent::AfterAdd()
{
	for (auto buildingMeshes : _buildingMeshesList) {
		buildingMeshes->AfterAdd();
	}
	buildingMeshesIndex = 0;

	PunUnrealUtils::UpdateDecals(_decals, decalIndex);

	AfterAdd_DeliveryArrow();
}