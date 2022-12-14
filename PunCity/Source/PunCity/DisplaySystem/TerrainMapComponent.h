// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "Components/SceneComponent.h"
//#include "SimpleInstancedMesh.h"
#include "DisplaySystemComponent.h"
#include "Materials/Material.h"
#include "Components/DecalComponent.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "StaticFastInstancedMeshesComp.h"

#include "TerrainMapComponent.generated.h"

UCLASS( ClassGroup=(Custom), meta=(BlueprintSpawnableComponent) )
class UTerrainMapComponent : public USceneComponent
{
	GENERATED_BODY()
public:	
	// Sets default values for this component's properties
	UTerrainMapComponent();

	void Deinit()
	{
		if (_assetLoader) {
			PunUnrealUtils::DestroyTexture2D(_assetLoader->heightTexture);
			PunUnrealUtils::DestroyTexture2D(_assetLoader->biomeTexture);
			PunUnrealUtils::DestroyTexture2D(_assetLoader->provinceTexture);
		}
	}

	//UPROPERTY(EditAnywhere, BlueprintReadWrite) UStaticMeshComponent* terrainMesh;
	UPROPERTY(EditAnywhere, BlueprintReadWrite) UStaticMeshComponent* terrainMeshWater;
	UPROPERTY(EditAnywhere, BlueprintReadWrite) UStaticMeshComponent* terrainMeshWaterDecal;

	UPROPERTY(EditAnywhere, BlueprintReadWrite) UStaticMeshComponent* terrainMeshWaterOutside;
	
	UPROPERTY() UMaterialInstanceDynamic* M_MapWater;

	UPROPERTY() UStaticFastInstancedMeshesComp* _georesourceEnumToMesh;
	UPROPERTY(EditAnywhere, BlueprintReadWrite) UStaticFastInstancedMeshesComp* _buildingsMeshes;

	void SetupWorldMapMesh(IDisplaySystemDataSource* dataSource, int tileDimXIn, int tileDimYIn, int worldMapSizeX, int worldMapSizeY, MapSizeEnum mapSizeEnum, UAssetLoaderComponent* assetLoader);
	static void SetupGlobalTextures(int tileDimXIn, int tileDimYIn, IGameSimulationCore* simulation, UAssetLoaderComponent* assetLoader);

	// Note that we use texture because mesh is too expensive. A single province mesh takes .2 ms to generate (~8k regions, so 1.6 sec to generate)
	void SetupProvinceTexture();
	
	void RefreshPartialProvinceTexture(TileArea area);
	void PaintProvinceTexture(ProvinceSystem& provinceSys, GameSimulationCore& sim, WorldTile2x2 provinceTile2x2, std::vector<uint8>& provinceDataPreBlur, WorldTile2x2 preBlur2x2);
	void SetBlurredProvinceArea(TileArea area2x2, std::vector<uint8>& provinceDataPreBlur);
	
	void UpdateTerrainMapDisplay(bool mapTerrainVisible, bool mapTerrainWaterVisible, bool tileDisplayNoRegionSkip);

	void InitAnnotations();
	void RefreshAnnotations();

	void RefreshHeightForestColorTexture(TileArea area, bool isInstant) {
		RefreshHeightForestColor(area, &(_dataSource->simulation()), false);
		if (isInstant) {
			PunUnrealUtils::SetTextureData(_assetLoader->heightTexture, heightForestColor);
		} else {
			isHeightForestColorDirty = true;
		}
	}
	static void SetRoadWorldTexture(WorldTile2 tile, bool isRoad, bool isDirtRoad)
	{
		//PUN_LOG("SetRoadWorldTexture %s", *tile.To_FString());
		
		isHeightForestColorDirty = true;

		if (PunSettings::TrailerSession) {
			isHeightForestColorDirty = false;
		}

		int32 tileId = tile.tileId();

		FColor color(heightForestColor[tileId]);
		if (isRoad) {
			// Use height as isDirtRoad for road
			color = FColor(color.R, 255, 0, isDirtRoad ? 255 : 0);
		} else {
			color = FColor(color.R, 0, 0, 0);
		}

		heightForestColor[tileId] = color.ToPackedARGB();
	}
	void RefreshHeightForestRoadTexture() {
		PunUnrealUtils::SetTextureData(_assetLoader->heightTexture, heightForestColor);
	}

private:
	static void RefreshHeightForestColor(TileArea area, IGameSimulationCore* simulation, bool useNewHeight);
	
private:
	static int tileDimX;
	static int tileDimY;
	static int tileDimX2;
	static int tileDimY2;
	float _tileToWorldMapX;
	float _tileToWorldMapY;
	MapSizeEnum _mapSizeEnum;

	IDisplaySystemDataSource* _dataSource;
	UPROPERTY() UAssetLoaderComponent* _assetLoader;

	UPROPERTY() TMap<int32, class UStaticMeshComponent*> _regionIdToStartingSpotMesh;
	UPROPERTY() TMap<int8, class UStaticMeshComponent*> _playerIdToFlagMesh;
	UPROPERTY() TMap<int8, class UMaterialInstanceDynamic*> _playerIdToFlagMaterial;

	UPROPERTY() UDecalComponent* _territoryMapDecal;
	UPROPERTY() UMaterialInstanceDynamic* _territoryMapDecalMaterial;

	bool _didFirstRefreshAnnotation = false;

	// Texture Data
	std::vector<uint32> provinceData;
	
	static std::vector<uint32> heightForestColor;
	static bool isHeightForestColorDirty;
	static float lastUpdatedHeightForestColor;
};
