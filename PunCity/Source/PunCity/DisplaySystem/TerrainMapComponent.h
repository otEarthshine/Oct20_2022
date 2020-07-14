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

	//UPROPERTY(EditAnywhere, BlueprintReadWrite) UStaticMeshComponent* terrainMesh;
	UPROPERTY(EditAnywhere, BlueprintReadWrite) UStaticMeshComponent* terrainMeshWater;
	UPROPERTY(EditAnywhere, BlueprintReadWrite) UStaticMeshComponent* terrainMeshWaterDecal;

	UPROPERTY(EditAnywhere, BlueprintReadWrite) UStaticMeshComponent* terrainMeshWaterOutside;
	
	UPROPERTY() UMaterialInstanceDynamic* M_MapWater;

	UPROPERTY() UStaticFastInstancedMeshesComp* _georesourceEnumToMesh;
	UPROPERTY(EditAnywhere, BlueprintReadWrite) UStaticFastInstancedMeshesComp* _buildingsMeshes;

	void SetupWorldMapMesh(IDisplaySystemDataSource* dataSource, int tileDimXIn, int tileDimYIn, int worldMapSizeX, int worldMapSizeY, MapSizeEnum mapSizeEnum, UAssetLoaderComponent* assetLoader);
	static void SetupGlobalTextures(int tileDimX, int tileDimY, IGameSimulationCore* simulation, UAssetLoaderComponent* assetLoader);

	void SetupProvinceTexture();
	void RefreshPartialProvinceTexture(TileArea area);
	void PaintProvinceTexture(ProvinceSystem& provinceSys, GameSimulationCore& sim, WorldTile2x2 provinceTile2x2, std::vector<uint8>& provinceDataPreBlur, WorldTile2x2 preBlur2x2);
	void SetBlurredProvinceArea(TileArea area2x2, std::vector<uint8>& provinceDataPreBlur);
	
	void UpdateTerrainMapDisplay(bool mapTerrainVisible, bool mapTerrainWaterVisible, bool tileDisplayNoRegionSkip);

	void InitAnnotations();
	void RefreshAnnotations();

	
private:
	int tileDimX;
	int tileDimY;
	int tileDimX2;
	int tileDimY2;
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
};
