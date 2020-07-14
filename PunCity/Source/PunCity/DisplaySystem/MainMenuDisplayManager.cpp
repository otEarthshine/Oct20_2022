// Pun Dumnernchanvanit's


#include "MainMenuDisplayManager.h"

AMainMenuDisplayManager::AMainMenuDisplayManager()
{
#if !MAIN_MENU_DISPLAY
	return;
#endif
	
	PrimaryActorTick.bCanEverTick = true;
	PrimaryActorTick.bStartWithTickEnabled = true;
	bReplicates = false;

	_root = CreateDefaultSubobject<USceneComponent>("Root");
	SetRootComponent(_root);
	
	_buildingDisplaySystem = CreateDefaultSubobject<UBuildingDisplayComponent>("BuildingDisplay");
	_resourceDisplaySystem = CreateDefaultSubobject<UResourceDisplayComponent>("ResourceDisplay");
	
	_regionDisplaySystem = CreateDefaultSubobject<URegionDisplayComponent>("RegionDisplay");
	_decalDisplaySystem = CreateDefaultSubobject<URegionDecalDisplayComponent>("RegionDecalDisplay");
	
	_tileDisplaySystem = CreateDefaultSubobject<UTileObjectDisplayComponent>("TileObjectDisplay");
	

	_assetLoader = CreateDefaultSubobject<UAssetLoaderComponent>("AssetLoader");

	_displayInfo.LoadBuildingSets(_assetLoader);
}

AMainMenuDisplayManager::~AMainMenuDisplayManager() = default;


void AMainMenuDisplayManager::InitMainMenuDisplayManager(MapSizeEnum mapSizeEnum)
{
#if !MAIN_MENU_DISPLAY
	return;
#endif

	_postProcessVolume = CastChecked<APostProcessVolume>(PunUnrealUtils::FindWorldActor(GetWorld(), FName("GlobalPostProcessVolume")));
	
	// Hard code this for now... MainMenu should use small map for performance anyway.
	WorldRegion2 regionPerWorld = GetMapSize(MapSizeEnum::Large);
	
	_LOG(PunSaveLoad, "InitMainMenuDisplayManager mapSize(%d, %d)", regionPerWorld.x, regionPerWorld.y);
	
	GameMap::SetRegionsPerWorld(regionPerWorld.x, regionPerWorld.y);

	_simulation = std::make_unique<GameSimulationCore>();
	_simulation->MainMenuDisplayInit();
	
	_buildingDisplaySystem->Init(GameMapConstants::TotalRegions, this, _assetLoader);
	_buildingDisplaySystem->isMainMenuDisplay = true;
	
	_resourceDisplaySystem->Init(GameMapConstants::TotalRegions, this, _assetLoader);
	_resourceDisplaySystem->isMainMenuDisplay = true;
	
	_regionDisplaySystem->Init(GameMapConstants::TotalRegions, this, _assetLoader);
	_regionDisplaySystem->isMainMenuDisplay = true;

	_decalDisplaySystem->isMainMenuDisplay = true;

	_decalDisplaySystem->Init(GameMapConstants::TotalRegions, this, _assetLoader);
	_decalDisplaySystem->isMainMenuDisplay = true;
	
	_tileDisplaySystem->Init(GameMapConstants::TotalRegions, this, _assetLoader);
	_tileDisplaySystem->isMainMenuDisplay = true;

	TArray<AActor*> backgrounds = PunUnrealUtils::FindWorldActors(GetWorld(), "BlackBackground");
	for (AActor* actor : backgrounds) {
		actor->SetActorHiddenInGame(true);
	}
}

void AMainMenuDisplayManager::UpdateDisplay(WorldAtom2 cameraAtom, float zoomDistance, std::vector<int32> sampleRegionIds)
{
#if !MAIN_MENU_DISPLAY
	return;
#endif
	
	_LOG(PunSaveLoad, "AMainMenuDisplayManager::UpdateDisplay %s samples:%llu", ToTChar(cameraAtom.worldTile2().ToString()), sampleRegionIds.size());
	
	FLinearColor cameraOffsetColor(float(cameraAtom.x) / CoordinateConstants::AtomPerDisplayUnit,
		float(cameraAtom.y) / CoordinateConstants::AtomPerDisplayUnit, 0.0f, 0.0f);
	_assetLoader->SetMaterialCollectionParametersVector("CameraOffset", cameraOffsetColor);

	int32 tileRadius = GetTileRadius(zoomDistance);

	WorldTile2 centerTile = cameraAtom.worldTile2();

	_cameraAtom = cameraAtom;
	_zoomDistance = zoomDistance;
	

	/*
	 * TODO: Somehow need this...
	 */
	_buildingDisplaySystem->SetRelativeLocation(FVector::ZeroVector);
	_resourceDisplaySystem->SetRelativeLocation(FVector::ZeroVector);
	_regionDisplaySystem->SetRelativeLocation(FVector::ZeroVector);
	_tileDisplaySystem->SetRelativeLocation(FVector::ZeroVector);
	_decalDisplaySystem->SetRelativeLocation(FVector::ZeroVector);

	// Must be done to set heightTexture/biomeTexture
	UTerrainMapComponent::SetupGlobalTextures(GameMapConstants::TilesPerWorldX, GameMapConstants::TilesPerWorldY, _simulation.get(), _assetLoader);

	
	/*
	 * Display
	 */
	static std::vector<int> noSample;

	// Debug: Ensure TileObjFull/HideWater gets refreshed in main menu 
	{
		_regionDisplaySystem->BeforeDisplay(false);
		_regionDisplaySystem->Display(noSample);

		_tileDisplaySystem->RefreshDisplay(sampleRegionIds);
		_tileDisplaySystem->BeforeDisplay(true, sampleRegionIds, false);
		_tileDisplaySystem->Display(noSample);
		_tileDisplaySystem->AfterDisplay();
	}
	
	_regionDisplaySystem->BeforeDisplay(false);
	_regionDisplaySystem->Display(PunSettings::IsOn("DisplayRegions") ? sampleRegionIds : noSample);
	
	_decalDisplaySystem->Display(PunSettings::IsOn("DisplayDecals") ? sampleRegionIds : noSample);

	_tileDisplaySystem->RefreshDisplay(sampleRegionIds);
	_tileDisplaySystem->BeforeDisplay(PunSettings::IsOn("TileObjFull"), sampleRegionIds, false);
	_tileDisplaySystem->Display(PunSettings::IsOn("DisplayTiles") ? sampleRegionIds : noSample);
	_tileDisplaySystem->AfterDisplay();

	_buildingDisplaySystem->Display(PunSettings::IsOn("DisplayBuildings") ? sampleRegionIds : noSample);
	_resourceDisplaySystem->Display(PunSettings::IsOn("DisplayResources") ? sampleRegionIds : noSample);
}