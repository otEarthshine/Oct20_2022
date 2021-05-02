// Fill out your copyright notice in the Description page of Project Settings.

#include "TerrainMapComponent.h"
#include "Engine/StaticMesh.h"
#include "Components/StaticMeshComponent.h"
#include "Components/DecalComponent.h"
#include "PunCity/PunTimer.h"
#include "../MapUtil.h"
#include "DisplaySystemDataSource.h"

using namespace std;

bool UTerrainMapComponent::isHeightForestColorDirty = false;
float UTerrainMapComponent::lastUpdatedHeightForestColor = 0.0f;

// Sets default values for this component's properties
UTerrainMapComponent::UTerrainMapComponent()
{
	INIT_LOG("Construct UTerrainMapComponent");
	
	// Set this component to be initialized when the game starts, and to be ticked every frame.  You can turn these features
	// off to improve performance if you don't need them.
	PrimaryComponentTick.bCanEverTick = true;
	SetIsReplicated(false);

	//terrainMesh = CreateDefaultSubobject<UProceduralMeshComponent>(TEXT("TerrainMeshTestChild"));

	//terrainMesh = CreateDefaultSubobject<UStaticMeshComponent>("WorldMapMesh1");
	//terrainMesh->AttachToComponent(this, FAttachmentTransformRules::KeepRelativeTransform);
	//terrainMesh->SetReceivesDecals(true);
	//terrainMesh->SetCastShadow(false);
	//PUN_CHECK(terrainMesh);

	terrainMeshWater = CreateDefaultSubobject<UStaticMeshComponent>("WorldMapMeshWater");
	terrainMeshWater->AttachToComponent(this, FAttachmentTransformRules::KeepRelativeTransform);
	terrainMeshWater->SetReceivesDecals(true);
	terrainMeshWater->SetCastShadow(false);
	PUN_CHECK(terrainMeshWater);

	terrainMeshWaterDecal = CreateDefaultSubobject<UStaticMeshComponent>("WorldMapMeshWaterDecal");
	terrainMeshWaterDecal->AttachToComponent(terrainMeshWater, FAttachmentTransformRules::KeepRelativeTransform);
	terrainMeshWaterDecal->SetReceivesDecals(true);
	terrainMeshWaterDecal->SetCastShadow(false);
	terrainMeshWaterDecal->SetCustomDepthStencilValue(1);
	PUN_CHECK(terrainMeshWaterDecal);



	terrainMeshWaterOutside = CreateDefaultSubobject<UStaticMeshComponent>("WorldMapMeshWaterOutside");
	terrainMeshWaterOutside->AttachToComponent(terrainMeshWater, FAttachmentTransformRules::KeepRelativeTransform);
	terrainMeshWaterOutside->SetReceivesDecals(false);
	terrainMeshWaterOutside->SetCastShadow(false);
	PUN_CHECK(terrainMeshWaterOutside);


	// Note: Don't parent these to terrainMesh since they are scaled....
	_buildingsMeshes = CreateDefaultSubobject<UStaticFastInstancedMeshesComp>("Map_BuildingMeshes");
	//_buildingsMeshes->AttachToComponent(terrainMesh, FAttachmentTransformRules::KeepRelativeTransform);
	//_buildingsMeshes->SetRelativeLocationAndRotation(FVector::ZeroVector, FRotator::ZeroRotator);
	
	_georesourceEnumToMesh = CreateDefaultSubobject<UStaticFastInstancedMeshesComp>("_georesourceEnumToMesh");

	// Territory Decal
	auto decal = CreateDefaultSubobject<UDecalComponent>("_territoryMapDecal");
	decal->AttachToComponent(this, FAttachmentTransformRules::KeepRelativeTransform, NAME_None);
	decal->DecalSize = FVector(160, 160, 160); // 160 is region sized decal
	decal->SetRelativeRotation(FVector(0, 0, -1).Rotation()); // Rotate to project the decal down
	_territoryMapDecal = decal;

}

void UTerrainMapComponent::UpdateTerrainMapDisplay(bool mapTerrainVisible, bool mapTerrainWaterVisible, bool tileDisplayNoRegionSkip)
{
	FVector mapScale;
	if (_mapSizeEnum == MapSizeEnum::Large) {
		mapScale = FVector(32, 32, 32);
	}
	else if (_mapSizeEnum == MapSizeEnum::Medium) {
		mapScale = FVector(16, 16, 16);
	}
	else if (_mapSizeEnum == MapSizeEnum::Small) {
		mapScale = FVector(8, 8, 8);
	}
	else {
		UE_DEBUG_BREAK();
		mapScale = FVector(32, 32, 32);
	}

	// Province Texture
	{
		auto& sim = _dataSource->simulation();
		std::vector<int32> provinceIds = sim.GetNeedDisplayUpdateIds(DisplayGlobalEnum::Province);
		for (int32 provinceId : provinceIds) {
			RefreshPartialProvinceTexture(sim.GetProvinceRectArea(provinceId));
		}
	}
	
	// Terrain Mesh
	{
		//if (mapTerrainVisible) {

		//	terrainMesh->SetVisibility(true);
		//	terrainMesh->SetActive(true);
		//	terrainMesh->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
		//}
		//else
		//{
		//	terrainMesh->SetVisibility(false);
		//	terrainMesh->SetActive(false);
		//	terrainMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
		//	terrainMesh->SetWorldLocation(FVector(0, 0, 1000000));
		//	terrainMesh->SetWorldScale3D(FVector::ZeroVector);
		//}
	}

	if (mapTerrainWaterVisible) {
		//terrainMesh->SetWorldLocation(MapUtil::DisplayLocationMapMode(MapUtil::GetCamShiftLocation(_dataSource->cameraAtom()), WorldAtom2::Zero));
		terrainMeshWater->SetWorldScale3D(mapScale); // Enlarge to match worldSize..
	}

	// Always updating waterMesh for terrainMeshWaterOutside to be placed in proper place
	terrainMeshWater->SetWorldLocation(MapUtil::DisplayLocation(_dataSource->cameraAtom(), WorldAtom2::Zero));

	// Water Mesh
	{
		terrainMeshWater->SetWorldScale3D(mapScale);
		terrainMeshWater->SetVisibility(mapTerrainWaterVisible);
		terrainMeshWater->SetActive(mapTerrainWaterVisible);
		terrainMeshWater->SetCollisionEnabled(ECollisionEnabled::NoCollision);

		terrainMeshWaterDecal->SetWorldScale3D(mapScale);
		//terrainMeshWaterDecal->SetVisibility(mapVisible);
		//terrainMeshWaterDecal->SetActive(mapVisible);
		terrainMeshWaterDecal->SetRenderInMainPass(false);
		terrainMeshWaterDecal->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	}

	// Water Outside mesh
	{
		terrainMeshWaterOutside->SetWorldScale3D(mapScale);
		terrainMeshWaterOutside->SetVisibility(true); // always visible even when zoomed
		terrainMeshWaterOutside->SetActive(mapTerrainVisible);
		terrainMeshWaterOutside->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	}

	bool mapTreeVisible = (!_dataSource->ZoomDistanceBelow(WorldZoomTransition_Tree)) && tileDisplayNoRegionSkip;

	// Forest Mesh
	{
		if (!PunSettings::IsOn("MapTree")) {
			mapTreeVisible = false;
		}
		
		_assetLoader->M_WorldMap->SetScalarParameterValue("ShowTrees", mapTreeVisible);
	}

	// Camera Position
	//{
	//	PUN_CHECK(_mapCrosshairDecal);
	//	_mapCrosshairDecal->SetDecalMaterial(_assetLoader->MapCrosshairDecalMaterial);
	//	_mapCrosshairDecal->SetActive(mapTreeVisible);
	//	_mapCrosshairDecal->SetVisibility(mapTreeVisible);
	//}

	{
		if (!_territoryMapDecalMaterial) {
			_territoryMapDecalMaterial = UMaterialInstanceDynamic::Create(_assetLoader->M_TerritoryMapDecal, this);
			_territoryMapDecalMaterial->SetTextureParameterValue("ProvinceMap", _assetLoader->provinceTexture);
		}

		// Decal at center
		FVector displayLocation = MapUtil::DisplayLocation(_dataSource->cameraAtom(), WorldTile2(GameMapConstants::TilesPerWorldX / 2, GameMapConstants::TilesPerWorldY / 2).worldAtom2());
		
		_territoryMapDecal->SetDecalMaterial(_territoryMapDecalMaterial);
		_territoryMapDecal->SetActive(mapTreeVisible);
		_territoryMapDecal->SetVisibility(mapTreeVisible);
		_territoryMapDecal->SetWorldScale3D(FVector(256, GameMapConstants::RegionsPerWorldY, GameMapConstants::RegionsPerWorldX));
		_territoryMapDecal->SetWorldLocation(displayLocation);
	}

	/*
	 * Buildings/Georesource
	 */
	bool mapCityVisible = !_dataSource->ZoomDistanceBelow(WorldZoomTransition_BuildingsMini);
	if (mapCityVisible && !_buildingsMeshes->IsActive()) {
		_georesourceEnumToMesh->SetActive(true, true);
		_buildingsMeshes->SetActive(true, true);
		RefreshAnnotations();
	} else if (!mapCityVisible && _buildingsMeshes->IsActive()) {
		_georesourceEnumToMesh->SetActive(false, true);
		_buildingsMeshes->SetActive(false, true);
	}

	// Initial map refresh happens after 3 sec
	if (Time::Ticks() > 3 * Time::TicksPerSecond && 
		!_didFirstRefreshAnnotation) 
	{
		_didFirstRefreshAnnotation = true;

		if (mapCityVisible) {
			_georesourceEnumToMesh->SetActive(true, true);
			_buildingsMeshes->SetActive(true, true);
			RefreshAnnotations();
		} else {
			_georesourceEnumToMesh->SetActive(false, true);
			_buildingsMeshes->SetActive(false, true);
		}
	}

	// Building Meshes
	{
		_buildingsMeshes->SetWorldLocation(MapUtil::DisplayLocation(_dataSource->cameraAtom(), WorldAtom2::Zero));
	}

	// HeightForestColorUpdate
	{
		float minUpdateInterval = Time::Ticks() > Time::TicksPerSecond ? 20.0f : 3.0f;

		if (mapTerrainVisible &&
			UGameplayStatics::GetTimeSeconds(GetWorld()) - lastUpdatedHeightForestColor > minUpdateInterval &&
			isHeightForestColorDirty)
		{
			isHeightForestColorDirty = false;
			lastUpdatedHeightForestColor = UGameplayStatics::GetTimeSeconds(GetWorld());

			SCOPE_TIMER("SetTextureData heightForestColor Texture");
			PunUnrealUtils::SetTextureData(_assetLoader->heightTexture, heightForestColor);
		}
	}
}


void UTerrainMapComponent::SetupWorldMapMesh(IDisplaySystemDataSource* dataSource, int tileDimXIn, int tileDimYIn, int worldMapSizeX, int worldMapSizeY, MapSizeEnum mapSizeEnum, UAssetLoaderComponent* assetLoader)
{
	//PUN_CHECK(terrainMesh);
	_dataSource = dataSource;
	_assetLoader = assetLoader;

	tileDimX = tileDimXIn;
	tileDimY = tileDimYIn;
	tileDimX2 = tileDimX / 2;
	tileDimY2 = tileDimY / 2;
	
	auto& terrainGenerator = dataSource->simulation().terrainGenerator();

	//! Prepare
	_tileToWorldMapX = float(worldMapSizeX) / tileDimX;
	_tileToWorldMapY = float(worldMapSizeY) / tileDimY;
	_mapSizeEnum = mapSizeEnum;

	PUN_CHECK(assetLoader);
	//terrainMesh->SetStaticMesh(assetLoader->WorldMapMesh);
	//terrainMesh->ComponentTags.Empty();
	//terrainMesh->ComponentTags.Add("WorldMap");

	//terrainMeshLayer2->SetStaticMesh(assetLoader->WorldMapMeshSubdivide);
	terrainMeshWater->SetStaticMesh(assetLoader->WorldMapMeshWater);
	
	terrainMeshWaterDecal->SetStaticMesh(assetLoader->WorldMapMeshWater);
	terrainMeshWaterDecal->SetMaterial(0, _assetLoader->M_HiddenInMainPass);

	terrainMeshWaterOutside->SetStaticMesh(assetLoader->WorldMapMeshWaterOutside);

	// Create terrain material if there isn't one.
	// !!! Can't use constructor because of landscape layer blend crash
	if (!_assetLoader->M_WorldMap) {
		auto worldMapMaterialProto = Cast<UMaterial>(StaticLoadObject(UMaterial::StaticClass(), NULL, *FString("/Game/Models/WorldMap/M_WorldMap")));
		_assetLoader->M_WorldMap = UMaterialInstanceDynamic::Create(worldMapMaterialProto, this);
	}

	if (!M_MapWater) {
		M_MapWater = UMaterialInstanceDynamic::Create(assetLoader->WorldMapMeshWater->GetMaterial(0), terrainMeshWater);
		terrainMeshWater->SetMaterial(0, M_MapWater);
	}

	SetupGlobalTextures(tileDimX, tileDimY, &(_dataSource->simulation()), assetLoader);
	SetupProvinceTexture();

	{
		_assetLoader->M_WorldMap->SetTextureParameterValue("HeightMap", _assetLoader->heightTexture);

		M_MapWater->SetTextureParameterValue("BiomeMap", _assetLoader->biomeTexture);
		_assetLoader->M_WorldMap->SetTextureParameterValue("BiomeMap", _assetLoader->biomeTexture);

		_assetLoader->M_WorldMap->SetTextureParameterValue("ProvinceMap", _assetLoader->provinceTexture);
	}
}

std::vector<uint32> UTerrainMapComponent::heightForestColor;
int UTerrainMapComponent::tileDimX;
int UTerrainMapComponent::tileDimY;
int UTerrainMapComponent::tileDimX2;
int UTerrainMapComponent::tileDimY2;

void UTerrainMapComponent::RefreshHeightForestColor(TileArea area, IGameSimulationCore* simulation, bool useNewHeight)
{
	//SCOPE_TIMER("Refresh HeightForestColor");

	isHeightForestColorDirty = true;
	
	PunTerrainGenerator& terrainGenerator = simulation->terrainGenerator();
	const std::vector<int16>& heightMap = terrainGenerator.heightMap;
	TreeSystem& treeSystem = simulation->treeSystem();

	area.ExecuteOnArea_Tile([&](int16_t x, int16_t y)
	{
		WorldTile2 tile(x, y);
		int32 tileId = tile.tileId();

		int32 heightColor;
		uint8 isRoad = 0;
		uint8 isDirtRoad = 0;
		if (useNewHeight)
		{
			// Height
			float height = FDToFloat(heightMap[tileId]);
			heightColor = static_cast<int32_t>(255.0f * height);
			heightColor = min(255, max(0, heightColor));
		}
		else
		{
			FColor oldColor(heightForestColor[tileId]);
			heightColor = oldColor.R;
			isRoad = oldColor.G;
			isDirtRoad = oldColor.A;
		}

		// Forest shade
		uint8 forestColor = 0;
		for (int32_t yy = y - 1; yy <= y + 1; yy++) {
			for (int32_t xx = x - 1; xx <= x + 1; xx++) {
				forestColor += treeSystem.treeShade(tileId) ? 28 : 0; // 28 = 255/9
			}
		}

		// Water
		FColor color = FColor(static_cast<uint8>(heightColor), isRoad, forestColor, isDirtRoad);
		heightForestColor[tileId] = color.ToPackedARGB();
	});
	
	//for (int y = 0; y < tileDimY; y++) {
	//	for (int x = 0; x < tileDimX; x++)
	//	{
	//		WorldTile2 tile(x, y);
	//		int32 tileId = tile.tileId();

	//		// Height
	//		float height = FDToFloat(heightMap[tileId]);
	//		int32 heightColor = static_cast<int32_t>(255.0f * height);
	//		heightColor = min(255, max(0, heightColor));

	//		// Forest shade
	//		uint8 forestColor = 0;
	//		if (x >= 1 && y >= 1 && x < (tileDimX - 1) && y < (tileDimY - 1))
	//		{
	//			for (int32_t yy = y - 1; yy <= y + 1; yy++) {
	//				for (int32_t xx = x - 1; xx <= x + 1; xx++) {
	//					forestColor += treeSystem.treeShade(tileId) ? 28 : 0; // 28 = 255/9
	//				}
	//			}
	//		}


	//		// Water
	//		FColor color = FColor(static_cast<uint8>(heightColor), 0, forestColor, forestColor);
	//		heightForestColor[tileId] = color.ToPackedARGB();
	//	}
	//}

}

void UTerrainMapComponent::SetupGlobalTextures(int tileDimXIn, int tileDimYIn, IGameSimulationCore* simulation, UAssetLoaderComponent* assetLoader)
{
	PunTerrainGenerator& terrainGenerator = simulation->terrainGenerator();

	tileDimX = tileDimXIn;
	tileDimY = tileDimYIn;
	
	//! Textures
	const int biomeSteps = 16;
	{
		assetLoader->heightTexture = PunUnrealUtils::CreateTexture2D(tileDimX, tileDimY);
		assetLoader->heightTexture->LODBias = -5;

		//_assetLoader->biomeTexture = PunUnrealUtils::CreateTexture2D(tileDimX / biomeSteps, tileDimY / biomeSteps);

		assetLoader->biomeTexture = PunUnrealUtils::CreateTexture2D(tileDimX / 4, tileDimY / 4);
	}

	const std::vector<int16>& heightMap = terrainGenerator.heightMap;
	//std::vector<TerrainRegionInfo>& regionToInfo = terrainGenerator.regionToInfo;

	{
		heightForestColor.clear();
		heightForestColor.resize(heightMap.size(), 0);

		TileArea area(1, 1, tileDimX - 1, tileDimY - 1);
		RefreshHeightForestColor(area, simulation, true);

		// Just in-case this is loaded from file, ensure all the roads are recorded in the texture
		auto& overlaySys = simulation->overlaySystem();
		for (int32 i = 0; i < GameMapConstants::TotalRegions; i++) {
			const std::vector<RoadTile>& roads = overlaySys.roads(i);
			for (const auto& road : roads) {
				SetRoadWorldTexture(road.tile, road.isConstructed, road.isDirt);
			}
		}

		SCOPE_TIMER("SetTextureData heightForestColor Texture");

		PunUnrealUtils::SetTextureData(assetLoader->heightTexture, heightForestColor);
		lastUpdatedHeightForestColor = UGameplayStatics::GetTimeSeconds(assetLoader->GetWorld());
		
		//SCOPE_TIMER("Update heightForestColor");
		//
		//for (int y = 0; y < tileDimY; y++) {
		//	for (int x = 0; x < tileDimX; x++)
		//	{
		//		WorldTile2 tile(x, y);
		//		int32 tileId = tile.tileId();

		//		// Height
		//		float height = FDToFloat(heightMap[tileId]);
		//		int32 heightColor = static_cast<int32_t>(255.0f * height);
		//		heightColor = min(255, max(0, heightColor));

		//		// Forest shade
		//		uint8 forestColor = 0;
		//		if (x >= 1 && y >= 1 && x < (tileDimX - 1) && y < (tileDimY - 1))
		//		{
		//			for (int32_t yy = y - 1; yy <= y + 1; yy++) {
		//				for (int32_t xx = x - 1; xx <= x + 1; xx++) {
		//					forestColor += treeSystem.treeShade(tileId) ? 28 : 0; // 28 = 255/9
		//				}
		//			}
		//		}


		//		// Water
		//		FColor color = FColor(static_cast<uint8>(heightColor), 0, forestColor, forestColor);
		//		heightForestColor[tileId] = color.ToPackedARGB();
		//	}
		//}

		//PunUnrealUtils::SetTextureData(assetLoader->heightTexture, heightForestColor);
	}

	{
		SCOPE_TIMER("Setup WorldMap Biome Texture")
		
		static std::vector<uint32> biomeData;
		biomeData.clear();
		biomeData.resize(heightMap.size() / 4 / 4, 0);

		const std::vector<uint8>& riverMap = terrainGenerator.river4x4Map();
		const std::vector<uint8>& rainfallMap = terrainGenerator.rainfall4x4Map();
		const std::vector<uint8>& mountainMap = terrainGenerator.isMountain4x4Map();

		int32 tileDimY4 = tileDimY / 4;
		int32 tileDimX4 = tileDimX / 4;
		for (int y4 = 0; y4 < tileDimY4; y4++) {
			for (int x4 = 0; x4 < tileDimX4; x4++)
			{
				int tile4x4Id = x4 + y4 * tileDimX4;

				int latitude = x4 * 100 / tileDimX4;

				// TODO: Temperature range becomes wider as we go away from equator
				int equatorMaxtemperature = 30;
				int poleMaxTemperature = 0;

				int absLatitude = abs(latitude - 50);

				int temperature = equatorMaxtemperature + (poleMaxTemperature - equatorMaxtemperature) * absLatitude / 50;
				temperature *= 3; // temp since temperature is max at only 30

				uint8 mountain = mountainMap[tile4x4Id];
				uint8 rainfall = rainfallMap[tile4x4Id];

				//uint8 trimmedRainfall = static_cast<int32_t>(rainfallMap[tile4x4Id]) * 8 / 10;
				//uint8 moisture = max(trimmedRainfall, riverMap[tile4x4Id]); // 0 - 0.8 is normal green... beyond that it is river green...

				biomeData[tile4x4Id] = FColor(temperature, rainfall, mountain, riverMap[tile4x4Id]).ToPackedARGB();
			}
		}
		

		// Copy to texture
		PunUnrealUtils::SetTextureData(assetLoader->biomeTexture, biomeData);
	}

}


void UTerrainMapComponent::PaintProvinceTexture(ProvinceSystem& provinceSys, GameSimulationCore& sim, WorldTile2x2 provinceTile2x2, std::vector<uint8>& provinceDataPreBlur, WorldTile2x2 preBlur2x2)
{
	int32 provinceId = provinceSys.GetProvinceId2x2(provinceTile2x2);

	auto paintTile2x2 = [&](uint8 color) {
		provinceDataPreBlur[preBlur2x2.tile2x2Id()] = color;
		provinceDataPreBlur[WorldTile2x2(preBlur2x2.x + 1, preBlur2x2.y).tile2x2Id()] = color;
		provinceDataPreBlur[WorldTile2x2(preBlur2x2.x, preBlur2x2.y + 1).tile2x2Id()] = color;
		provinceDataPreBlur[WorldTile2x2(preBlur2x2.x + 1, preBlur2x2.y + 1).tile2x2Id()] = color;
	};

	// Negative provinceId is the edge
	if (IsEdgeProvinceId(provinceId))
	{
		//int32 topProvinceId = provinceSys.GetProvinceId2x2(WorldTile2x2(x2 + 1, y2));
		//int32 rightProvinceId = provinceSys.GetProvinceId2x2(WorldTile2x2(x2, y2 - 1));
		//int32 right2ProvinceId = provinceSys.GetProvinceId2x2(WorldTile2x2(x2, y2 - 2));
		//int32 leftProvinceId = provinceSys.GetProvinceId2x2(WorldTile2x2(x2, y2 + 1));
		//int32 left2ProvinceId = provinceSys.GetProvinceId2x2(WorldTile2x2(x2, y2 + 2));

		// Horizontal line looks thick, If detect top along with right/left, don't draw
		// Detect 2 right/left so it doesn't look weird on intersections
		//if (IsEdgeProvinceId(topProvinceId) &&
		//	IsEdgeProvinceId(rightProvinceId) && IsEdgeProvinceId(right2ProvinceId) &&
		//	IsEdgeProvinceId(leftProvinceId) && IsEdgeProvinceId(left2ProvinceId))
		//{
		//	return;
		//}

		//WorldTile2 tile(x2 * 2, y2 * 2);
		//provinceDataPreBlur[tile.tileId()] = 255;
		//provinceDataPreBlur[WorldTile2(tile.x + 1, tile.y).tileId()] = 255;
		//provinceDataPreBlur[WorldTile2(tile.x, tile.y + 1).tileId()] = 255;
		//provinceDataPreBlur[WorldTile2(tile.x + 1, tile.y + 1).tileId()] = 255;
		paintTile2x2(255);

		//const int32 kernel1 = 255;
		//const int32 kernel2 = 255 / 2;
		//const int32 kernel3 = 255 / 4;
		//const int32 kernalColor1 = FColor(kernel1, kernel1, kernel1, 255).ToPackedARGB();
		//const int32 kernalColor2 = FColor(kernel2, kernel2, kernel2, 255).ToPackedARGB();
		//const int32 kernalColor3 = FColor(kernel3, kernel3, kernel3, 255).ToPackedARGB();

		//
		//auto tryColor = [&](int32 x2_, int32 y2_, int32 kernel, int32 kernelColor) {
		//	if (kernel > FColor(provinceData[x2_ + y2_ * tileDimX2]).A) 
		//	{
		//		WorldTile2 tile(x2_ * 2, y2_ * 2);
		//		provinceData[tile.tileId()] = kernelColor;
		//		provinceData[WorldTile2(tile.x + 1, tile.y).tileId()] = kernelColor;
		//		provinceData[WorldTile2(tile.x, tile.y + 1).tileId()] = kernelColor;
		//		provinceData[WorldTile2(tile.x + 1, tile.y + 1).tileId()] = kernelColor;
		//	}
		//};

		//tryColor(x2, y2, kernel1, kernalColor1);

		//tryColor(x2 - 1, y2, kernel2, kernalColor2);
		//tryColor(x2 + 1, y2, kernel2, kernalColor2);
		//tryColor(x2, y2 - 1, kernel2, kernalColor2);
		//tryColor(x2, y2 + 1, kernel2, kernalColor2);

		//tryColor(x2 - 1, y2 - 1, kernel3, kernalColor3);
		//tryColor(x2 + 1, y2 - 1, kernel3, kernalColor3);
		//tryColor(x2 - 1, y2 + 1, kernel3, kernalColor3);
		//tryColor(x2 + 1, y2 + 1, kernel3, kernalColor3);
	}
	else if (IsValidNonEdgeProvinceId(provinceId))
	{
		int32 townId = sim.provinceOwnerTown(provinceId);

		if (townId != -1)
		{
			//FColor color = PlayerColor1(sim.provinceOwner(provinceId)).ToFColor(true);
			//color.A = 130;
			//uint32 packedColor = color.ToPackedARGB();

			//WorldTile2 tile(x2 * 2, y2 * 2);
			//provinceData[tile.tileId()] = packedColor;
			//provinceData[WorldTile2(tile.x + 1, tile.y).tileId()] = packedColor;
			//provinceData[WorldTile2(tile.x, tile.y + 1).tileId()] = packedColor;
			//provinceData[WorldTile2(tile.x + 1, tile.y + 1).tileId()] = packedColor;
			paintTile2x2(130);
		}
		else {
			paintTile2x2(0);
		}
	}
	else {
		paintTile2x2(0);
	}
}

void UTerrainMapComponent::SetupProvinceTexture()
{
	SCOPE_TIMER("Setup World Map Province Texture");
	_assetLoader->provinceTexture = PunUnrealUtils::CreateTexture2D(tileDimX2, tileDimY2);

	std::vector<uint8> provinceDataPreBlur(tileDimX2 * tileDimY2, 0);

	provinceData.clear();
	provinceData.resize(tileDimX2 * tileDimY2, FColor(0, 0, 0, 0).ToPackedARGB());

	auto& sim = _dataSource->simulation();
	auto& provinceSys = sim.provinceSystem();

	for (int y2 = 0; y2 < tileDimY2; y2++) {
		for (int x2 = 0; x2 < tileDimX2; x2++)
		{
			PaintProvinceTexture(provinceSys, sim, WorldTile2x2(x2, y2), provinceDataPreBlur, WorldTile2x2(x2, y2));
		}
	}

	TileArea area2x2(0, 0, tileDimX2 - 1, tileDimY2 - 1);
	SetBlurredProvinceArea(area2x2, provinceDataPreBlur);

	PunUnrealUtils::SetTextureData(_assetLoader->provinceTexture, provinceData);
}

void UTerrainMapComponent::RefreshPartialProvinceTexture(TileArea area)
{
	SCOPE_TIMER("RefreshPartial Province Texture");
	auto& sim = _dataSource->simulation();
	auto& provinceSys = sim.provinceSystem();

	/*
	 * Bugged???
	 */

	//area.minX = max(0, area.minX - 5);
	//area.minY = max(0, area.minY - 5);
	//area.maxX = min(tileDimX, area.maxX + 5);
	//area.maxY = min(tileDimY, area.maxY + 5);

	//TileArea area2x2 = area.tileArea2x2();

	//std::vector<uint8> provinceDataPreBlur(area2x2.sizeX() * area2x2.sizeY(), 0);

	//// Still bugged....
	//
	//for (int y2 = 0; y2 < area2x2.sizeX(); y2++) {
	//	for (int x2 = 0; x2 < area2x2.sizeY(); x2++)
	//	{
	//		// this paint on provinceDataPreBlur only
	//		PaintProvinceTexture(provinceSys, sim, WorldTile2x2(area2x2.minX + x2, area2x2.minY + y2), provinceDataPreBlur, WorldTile2x2(x2, y2));
	//	}
	//}

	//SetBlurredProvinceArea(area2x2, provinceDataPreBlur);

	//PunUnrealUtils::SetTextureData(_assetLoader->provinceTexture, provinceData);
}

void UTerrainMapComponent::SetBlurredProvinceArea(TileArea area2x2, std::vector<uint8>& provinceDataPreBlur)
{
	for (int y = 1; y < area2x2.sizeY() - 1; y++) {
		for (int x = 1; x < area2x2.sizeX() - 1; x++)
		{
			int32 average = provinceDataPreBlur[x + y * tileDimX2];
			average += provinceDataPreBlur[(x - 1) + (y - 1) * tileDimX2]
				+ provinceDataPreBlur[(x - 1) + (y)* tileDimX2]
				+ provinceDataPreBlur[(x - 1) + (y + 1) * tileDimX2]

				+ provinceDataPreBlur[(x + 1) + (y - 1) * tileDimX2]
				+ provinceDataPreBlur[(x + 1) + (y)* tileDimX2]
				+ provinceDataPreBlur[(x + 1) + (y + 1) * tileDimX2]

				+ provinceDataPreBlur[(x)+(y + 1) * tileDimX2]
				+ provinceDataPreBlur[(x)+(y - 1) * tileDimX2];
			average /= 9;

			provinceData[(x + area2x2.minX) + (y + area2x2.minY) * tileDimX2] = FColor(average, average, average, average).ToPackedARGB();
		}
	}
}

void UTerrainMapComponent::InitAnnotations()
{	
	_georesourceEnumToMesh->Init("MapResourceNode1", terrainMeshWater, 200, "", 0);
	//_georesourceEnumToMesh2->Init("MapResourceNode2", terrainMesh, 200, "", 0);
	for (int32_t i = GeoresourceEnumCount; i-- > 0;) {
		GeoresourceInfo info = GetGeoresourceInfo(i);
		TArray<UStaticMesh*> miniMeshes = _assetLoader->georesourceMesh(info.georesourceEnum).miniMeshes;
		for (int32 j = 0; j < miniMeshes.Num(); j++) {
			_georesourceEnumToMesh->AddProtoMesh((*info.GetDisplayName()) + FString::FromInt(j), miniMeshes[j]);
		}
	}

	/*
	 * Buildings
	 */
	{
		_buildingsMeshes->Init("BuildingModules_Map", this, 100, "", 0);

		const TArray<FString>& moduleNames = _assetLoader->moduleNames();
		for (int i = 0; i < moduleNames.Num(); i++) {
			UStaticMesh* protoMesh = _assetLoader->moduleMesh(moduleNames[i]);
			if (protoMesh) {
				if (moduleNames[i].Right(4) == FString("Body") ||
					moduleNames[i].Right(4) == FString("Roof"))
				{
					_buildingsMeshes->AddProtoMesh(moduleNames[i], protoMesh, nullptr);
				}
			}
		}
	}
}

void UTerrainMapComponent::RefreshAnnotations()
{
	_LOG(PunDisplay, "RefreshAnnotations");
	SCOPE_TIMER("RefreshAnnotations");

	/*
	 * Buildings
	 */
	const GameDisplayInfo& displayInfo = _dataSource->displayInfo();

	auto& simulation = _dataSource->simulation();

#if DISPLAY_WORLDMAP_BUILDING
	simulation.ExecuteOnPlayersAndAI([&](int32 playerId)
	{
		for (int32 j = 0; j < BuildingEnumCount; j++)
		{
			CardEnum buildingEnum = static_cast<CardEnum>(j);
			
			// Don't display road
			if (IsRoad(buildingEnum)) {
				return;
			}
			
			const std::vector<int32> buildingIds = simulation.buildingIds(playerId, buildingEnum);

			//PUN_LOG("RefreshAnnotations count:%d", buildingIds.size());
			
			for (int32 buildingId : buildingIds) 
			{
				Building& building = simulation.building(buildingId);
				
				if (building.isConstructed() && displayInfo.GetVariationCount(buildingEnum) > 0)
				{
					// Building mesh
					int32 displayVariationIndex = building.displayVariationIndex();
					float buildingRotation = RotationFromDirection(building.faceDirection());


					WorldTile2 centerTile = building.centerTile();
					FVector displayLocation(centerTile.x * CoordinateConstants::DisplayUnitPerTile,
						centerTile.y * CoordinateConstants::DisplayUnitPerTile, 0);
					//FVector displayLocation(centerTile.x * _tileToWorldMapX, centerTile.y * _tileToWorldMapY, 0);

					//FVector displayLocation(0, 0, 0);

					FTransform transform(FRotator(0, buildingRotation, 0), displayLocation);
					//_dataSource->DisplayLocation(centerTile.worldAtom2())

					const ModuleTransformGroup& modulePrototype = displayInfo.GetDisplayModules(buildingEnum, displayVariationIndex);
					std::vector<ModuleTransform> modules = modulePrototype.transforms;

					auto showMesh = [&](int32 i) {
						int32 instanceKey = centerTile.tileId() + i * GameMapConstants::TilesPerWorld;

						FTransform finalTransform;
						FTransform::Multiply(&finalTransform, &modules[i].transform, &transform);

						//PUN_LOG("ModuleDisplay %s, %s", *finalTransform.GetRotation().Rotator().ToString(), *modules[i].transform.GetRotation().Rotator().ToString());
						_buildingsMeshes->Add(modules[i].moduleName, instanceKey, finalTransform, 0, buildingId);
					};

					// Special buildings
					if (buildingEnum == CardEnum::RegionCrates ||
						buildingEnum == CardEnum::RegionShrine)
					{
						for (int32 i = 0; i < modules.size(); i++) {
							if (modules[i].moduleName.Right(4) == FString("Special")) {
								showMesh(i);
							}
						}
					}
					else
					{
						for (int32 i = 0; i < modules.size(); i++)
						{
							if (modules[i].moduleName.Right(4) == FString("Body") ||
								modules[i].moduleName.Right(4) == FString("Roof"))
							{
								showMesh(i);
								//int32 instanceKey = centerTile.tileId() + i * GameMapConstants::TilesPerWorld;

								//FTransform finalTransform;
								//FTransform::Multiply(&finalTransform, &modules[i].transform, &transform);

								////PUN_LOG("ModuleDisplay %s, %s", *finalTransform.GetRotation().Rotator().ToString(), *modules[i].transform.GetRotation().Rotator().ToString());
								//_buildingsMeshes->Add(modules[i].moduleName, instanceKey, finalTransform, 0, buildingId);
							}
						}
					}

				}
			}
		}

		
	});
#endif

	_buildingsMeshes->AfterAdd();
	bool mapCityVisible = !_dataSource->ZoomDistanceBelow(WorldZoomTransition_BuildingsMini);
	_buildingsMeshes->SetActive(mapCityVisible, true);

	// No showing Georesource for trailer
	if (PunSettings::TrailerSession) {
		_georesourceEnumToMesh->AfterAdd();
		return;
	}
	

	/*
	 * Georesources
	 */
	GeoresourceSystem& georesourceSystem = _dataSource->simulation().georesourceSystem();
	const std::vector<GeoresourceNode>& georesources = georesourceSystem.provinceToGeoresource();

	for (const GeoresourceNode& node : georesources)
	{
		if (node.HasResource())
		{
			GeoresourceInfo info = node.info();
			// Add
			WorldTile2 centerTile = node.centerTile;
			FVector displayLocation(centerTile.x * _tileToWorldMapX, centerTile.y * _tileToWorldMapY, 0);
			
			FVector size = FVector::OneVector * 0.5f; // From 2x2 tile to 1x1 sized...

			FTransform transform(FRotator::ZeroRotator, displayLocation, size);

			int32 miniMeshesCount = _assetLoader->georesourceMesh(info.georesourceEnum).miniMeshes.Num();
			for (int32 j = 0; j < miniMeshesCount; j++) {
				// TODO: later standardize this...
				if (info.georesourceEnum == GeoresourceEnum::Ruin) {
					transform.SetScale3D(FVector::OneVector * 0.1f);
				}
				
				_georesourceEnumToMesh->Add(*info.GetDisplayName() + FString::FromInt(j), centerTile.tileId(), transform, 0);
			}
			//_georesourceEnumToMesh2->Add(ToFString(info.name), centerTile.tileId(), transform, 0);
		}
	}

	_georesourceEnumToMesh->AfterAdd();

}