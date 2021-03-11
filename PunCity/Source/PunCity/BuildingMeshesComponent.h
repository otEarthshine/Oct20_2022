// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Components/SceneComponent.h"
#include "Components/StaticMeshComponent.h"
#include "PunCity/DisplaySystem/GameDisplayInfo.h"
#include "PunCity/Simulation/Building.h"
#include "BuildingMeshesComponent.generated.h"

/*
 * Manages reusable StaticMeshComponents used to display many modules (one building = many modules)
 *  (for UI/temporary display such as IconBaker, UI highlight, UI delay filler)
 */

UCLASS( ClassGroup=(Custom), meta=(BlueprintSpawnableComponent) )
class UBuildingMeshesComponent : public USceneComponent
{
	GENERATED_BODY()
public:
	void AfterAdd() {
		// Remove Unused
		for (int32 i = _meshIndexIterator; i < meshes.Num(); i++) {
			meshes[i]->SetVisibility(false);
			meshes[i]->SetActive(false);
		}
		_meshIndexIterator = 0;
	}
	
	void Hide()
	{
		for (int i = 0; i < meshes.Num(); i++) {
			meshes[i]->SetVisibility(false);
			meshes[i]->SetActive(false);
		}
		_meshIndexIterator = 0;
	}

	void Show(Direction faceDirection, const std::vector<ModuleTransform>& modules, UAssetLoaderComponent* assetLoader, int32 customDepthIndex = 0, bool receiveDecal = true)
	{
		PUN_CHECK(assetLoader);

		//while (meshes.Num() < modules.size() + _meshIndexIterator) {
		for (int32 i = meshes.Num(); i < modules.size() + _meshIndexIterator; i++) 
		{
			auto buildingMesh = NewObject<UStaticMeshComponent>(this);
			buildingMesh->AttachToComponent(this, FAttachmentTransformRules::KeepRelativeTransform, NAME_None);
			buildingMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
			buildingMesh->SetGenerateOverlapEvents(false);
			buildingMesh->SetReceivesDecals(receiveDecal);
			buildingMesh->RegisterComponent();
			
			meshes.Add(buildingMesh);
		}

		for (int i = 0; i < modules.size(); i++)
		{
			int32 meshI = i + _meshIndexIterator;
			UStaticMesh* mesh = assetLoader->moduleMesh(modules[i].moduleName);
			PUN_CHECK(mesh || modules[i].moduleName == "StoneRoad" || modules[i].moduleName == "DirtRoad");
			
			meshes[meshI]->SetStaticMesh(mesh);
			meshes[meshI]->SetVisibility(true);
			meshes[meshI]->SetActive(true);

			GameDisplayUtils::SetCustomDepth(meshes[meshI], customDepthIndex);

			// Set Transform
			FString moduleName = modules[i].moduleName;
			FTransform moduleTransform = modules[i].transform;
			
			//if (moduleName.Equals(FString("DecorativeBasketBox"))) {
			//	PUN_LOG("%s transform: %s, %s, %s", *moduleName, *moduleTransform.GetTranslation().ToCompactString(),
			//		*moduleTransform.GetRotation().Rotator().ToString(),
			//		*moduleTransform.GetScale3D().ToCompactString());
			//}
			
			meshes[i]->SetRelativeTransform(moduleTransform);
		}

		_meshIndexIterator += modules.size();
		
		SetWorldRotation(FRotator(0.0f, RotationFromDirection(faceDirection), 0.0f));
	}

	void ShowBridge(Building& building, UAssetLoaderComponent* assetLoader, int32 customDepthIndex = 0)
	{
		std::vector<GameDisplayUtils::BridgeModule> bridgeModules = GameDisplayUtils::GetBridgeModules(building.area());

		std::vector<ModuleTransform> modules;
		for (int i = 0; i < bridgeModules.size(); i++)
		{
			FVector displayLocation = (bridgeModules[i].tile - building.centerTile()).displayLocation();
			FTransform transform(FRotator(0, bridgeModules[i].rotation, 0), displayLocation);

			modules.push_back(ModuleTransform(bridgeModules[i].moduleName, transform));
		}

		Show(Direction::S, modules, assetLoader, customDepthIndex);
	}

	void ShowTunnel(Building& building, UAssetLoaderComponent* assetLoader, int32 customDepthIndex = 0)
	{
		std::vector<ModuleTransform> modules;
		
		auto spawnEntrance = [&](WorldTile2 tile, int32 rotationInt) {
			FVector displayLocation = (tile - building.centerTile()).displayLocation();
			FTransform transform(FRotator(0, rotationInt, 0), displayLocation);
			modules.push_back(ModuleTransform(FString("Tunnel"), transform));
		};

		TileArea area = building.area();
		int32 rotationShift = (area.sizeX() > 1) ? 0 : 90;

		spawnEntrance(area.min(), rotationShift);
		spawnEntrance(area.max(), rotationShift + 180);
		

		Show(Direction::S, modules, assetLoader, customDepthIndex);
	}

	/*
	 * Storage
	 */
	void ShowStorageMesh(TileArea area, WorldTile2 centerTile, UAssetLoaderComponent* assetLoader, int32 customDepthIndex = 0)
	{
		std::vector<ModuleTransform> modules;
		for (int32 x = 0; x < area.sizeX(); x += 2) {
			for (int32 y = 0; y < area.sizeY(); y += 2)
			{
				WorldTile2 tile(x + area.minX, y + area.minY);
				// Only display the even tiles ,shifted by 0.5

				FVector displayLocation = (tile - centerTile).displayLocation(); // ??
				FTransform storageTransform(FRotator(0, 0, 0), displayLocation + FVector(5, 5, 0));
				modules.push_back(ModuleTransform("StorageTile", storageTransform));
			}
		}

		Show(Direction::S, modules, assetLoader, customDepthIndex);
	}
	void ShowStorageMesh(Building& building, UAssetLoaderComponent* assetLoader, int32 customDepthIndex = 0) {
		ShowStorageMesh(building.area(), building.centerTile(), assetLoader, customDepthIndex);
	}

private:
	int32 _meshIndexIterator = 0;
	UPROPERTY() TArray<UStaticMeshComponent*> meshes;
};
