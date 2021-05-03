#include "GameManager.h"

//#include "CameraPawn.h"

#include "PunCity/MapUtil.h"
#include "PunCity/AssetLoaderComponent.h"

#include "Particles/ParticleSystemComponent.h"
#include "Kismet/GameplayStatics.h"
#include "PunCity/PunUtils.h"
#include "PunCity/PunGameSettings.h"

#include "PunCity/Simulation/Buildings/Townhall.h"

#include "Engine/DirectionalLight.h"
#include "Engine/SkyLight.h"
#include "Components/DirectionalLightComponent.h"
#include "Components/ExponentialHeightFogComponent.h"
#include "Engine/PostProcessVolume.h"
#include "Materials/MaterialInstance.h"

#include "Sound/SoundCue.h"
#include "Components/AudioComponent.h"
#include "Engine/StaticMeshActor.h"
#include "EngineUtils.h"
#include "PunCity/PunTimer.h"
#include "PunCity/PunGameInstance.h"

//LLM_SCOPE(TEXT("PunTest"));


using namespace std;
using namespace std::chrono;

DECLARE_CYCLE_STAT(TEXT("PUN: [Display] Core [0]"), STAT_PunDisplayTickCore, STATGROUP_Game);
DECLARE_CYCLE_STAT(TEXT("PUN: [Display] TileObjects"), STAT_PunDisplayTree, STATGROUP_Game);
DECLARE_CYCLE_STAT(TEXT("PUN: [Display] Resource"), STAT_PunDisplayResource, STATGROUP_Game);
DECLARE_CYCLE_STAT(TEXT("PUN: [Display] Building"), STAT_PunDisplayBuilding, STATGROUP_Game);
DECLARE_CYCLE_STAT(TEXT("PUN: [Display] MiniBuilding"), STAT_PunDisplayMiniBuilding, STATGROUP_Game);
DECLARE_CYCLE_STAT(TEXT("PUN: [Display] Unit"), STAT_PunDisplayUnit, STATGROUP_Game);
DECLARE_CYCLE_STAT(TEXT("PUN: [Display] Region"), STAT_PunDisplayRegion, STATGROUP_Game);
DECLARE_CYCLE_STAT(TEXT("PUN: [Display] Landmark"), STAT_PunDisplayLandmark, STATGROUP_Game);

DECLARE_CYCLE_STAT(TEXT("PUN: [Display] Misc [1]"), STAT_PunDisplayTickMisc, STATGROUP_Game);
DECLARE_CYCLE_STAT(TEXT("PUN: [Display] Misc-Territory"), STAT_PunDisplayTerritory, STATGROUP_Game);
DECLARE_CYCLE_STAT(TEXT("PUN: [Display] Misc-Sample"), STAT_PunDisplaySample, STATGROUP_Game);


#define LOCTEXT_NAMESPACE "GameManager"


// Sets default values
AGameManager::AGameManager()
{
	LLM_SCOPE_(EPunSimLLMTag::PUN_GameManager);
	
	INIT_LOG("Construct AGameManager");
	
	PrimaryActorTick.bCanEverTick = true;
	PrimaryActorTick.bStartWithTickEnabled = true;
	bReplicates = false;

	_root = CreateDefaultSubobject<USceneComponent>("Root");
	SetRootComponent(_root);

#if DISPLAY_WORLDMAP
	_terrainMap = CreateDefaultSubobject<UTerrainMapComponent>("WorldMap");
	_worldMap = CreateDefaultSubobject<UWorldMapMeshComponent>("WorldMap2");
#endif
	
#if DISPLAY_BUILDING
	_buildingDisplaySystem = CreateDefaultSubobject<UBuildingDisplayComponent>("BuildingDisplay");
#endif

#if DISPLAY_MINIBUILDING
	_miniBuildingDisplaySystem = CreateDefaultSubobject<UMiniBuildingDisplayComponent>("MiniBuildingDisplay");
#endif

#if DISPLAY_TERRAIN
	_regionDisplaySystem = CreateDefaultSubobject<URegionDisplayComponent>("RegionDisplay");
	_terrainLargeDisplaySystem = CreateDefaultSubobject<UTerrainLargeDisplayComponent>("TerrainLargeDisplay");
#endif

	_debugDisplaySystem = CreateDefaultSubobject<UDebugDisplayComponent>("DebugDisplay");

#if DISPLAY_REGIONDECAL
	_decalDisplaySystem = CreateDefaultSubobject<URegionDecalDisplayComponent>("RegionDecalDisplay");
#endif

#if DISPLAY_TILEOBJ
	_tileDisplaySystem = CreateDefaultSubobject<UTileObjectDisplayComponent>("TileObjectDisplay");
#endif

#if DISPLAY_UNIT
	_unitDisplaySystem = CreateDefaultSubobject<UUnitDisplayComponent>("UnitDisplay");
#endif

#if DISPLAY_RESOURCE
	_resourceDisplaySystem = CreateDefaultSubobject<UResourceDisplayComponent>("ResourceByRegionDisplay");
#endif

#if DISPLAY_TERRITORY
	_territoryDisplaySystem = CreateDefaultSubobject<UTerritoryDisplayComponent>("TerritoryDisplay");
#endif

	
	worldWidgetParent = CreateDefaultSubobject<USceneComponent>("WorldWidgetParent");

	_snowParticles = CreateDefaultSubobject<UParticleSystemComponent>("Snow");
	_rainParticles = CreateDefaultSubobject<UParticleSystemComponent>("Rain");
	
	_rainWetnessDecal = CreateDefaultSubobject<UDecalComponent>("RainWetnessDecal");
	_rainWetnessDecal->SetRelativeRotation(FVector(0, 0, -1).Rotation()); // Rotate to project the decal down
	_rainWetnessDecal->DecalSize = FVector(512, 1024, 1024);

	_overlaySetterTypeToType.resize(static_cast<int>(OverlaySetterType::Count));

	//_moduleInfo.LoadJson();

	_assetLoader = CreateDefaultSubobject<UAssetLoaderComponent>("AssetLoader");
	_displayInfo.LoadBuildingSets(_assetLoader);

	_assetLoader->CheckMeshesAvailable();

	PaintConstructionMesh = false;
	PrintConstructionMesh = false;
	//UpdateRHIConstructionMesh = false;


	//PUN_CHECK(assetLoader()->moduleNames().Num() == ModuleMeshCount);
}

void AGameManager::Init(int32 playerId, bool isLoadingFromFile, IGameUIInterface* uiInterface, IGameNetworkInterface* networkInterface)
{
	LLM_SCOPE_(EPunSimLLMTag::PUN_GameManager);
	
	//PUN_CHECK(assetLoader()->moduleNames().Num() == ModuleMeshCount);
	
	PUN_DEBUG(FString::Printf(TEXT("GameManager Init playerId:%d"), playerId));

	_uiInterface = uiInterface;
	_networkInterface = networkInterface;
	
	_playerId = playerId;
	_initPhase = 1;
	_isLoadingFromFile = isLoadingFromFile;

	_isPhotoMode = false;

	{ // Hide test scene items
		HideActorType<AStaticMeshActor>();
		HideActorType<AEmitter>();
		HideActorType<ADecalActor>();
		HideActorType<APointLight>();
	}

	_assetLoader->CheckMeshesAvailable();

	// Loading from file means we need _simulation right from the start??
	if (_isLoadingFromFile) {
		InitPhase1();
		_initPhase++;

		// Set game speed to lowest after load...
		networkInterface->SetGameSpeed(GameSpeedHalf);
	}

	//PUN_CHECK(assetLoader()->moduleNames().Num() == ModuleMeshCount);
}

void AGameManager::InitPhase1()
{
	LLM_SCOPE_(EPunSimLLMTag::PUN_GameManager);
	
	SCOPE_TIMER("InitPhase1");

	WorldRegion2 mapSize = GetMapSize(GetMapSettings().mapSizeEnumInt);

	int regionPerWorldX = mapSize.x;
	int regionPerWorldY = mapSize.y;

	_assetLoader->SetMaterialCollectionParametersScalar("TotalRegionX", regionPerWorldX);
	_assetLoader->SetMaterialCollectionParametersScalar("TotalRegionY", regionPerWorldY);

	_assetLoader->SetMaterialCollectionParametersScalar("TundraTemperatureStart", tundraTemperatureStart100 / 100.0f);
	_assetLoader->SetMaterialCollectionParametersScalar("BorealTemperatureStart", borealTemperatureStart100 / 100.0f);
	_assetLoader->SetMaterialCollectionParametersScalar("ForestTemperatureStart", forestTemperatureStart100 / 100.0f);

	GameMap::SetRegionsPerWorld(regionPerWorldX, regionPerWorldY);

	_simulation = make_unique<GameSimulationCore>();
	_simulation->Init(this, this, _uiInterface, _isLoadingFromFile);

	// Note!!! Don't try to move _simulation->Init to phase2 ... it will cause endless prblems

	// Next Phase:
	_uiInterface->SetLoadingText(LOCTEXT("Loading_AddingLife", "Adding Life..."));

	//PUN_CHECK(assetLoader()->moduleNames().Num() == ModuleMeshCount);
}
void AGameManager::InitPhase2()
{
	LLM_SCOPE_(EPunSimLLMTag::PUN_GameManager);
	
	SCOPE_TIMER("InitPhase2");
	
	UnitSystem& unitSystem = _simulation->unitSystem();

	if (!_isLoadingFromFile)
	{
		int32 initialAnimals = PunSettings::Get("InitialAnimals");
		switch (_simulation->mapSizeEnum()) {
		case MapSizeEnum::Large: break;
		case MapSizeEnum::Medium: initialAnimals /= 4; break;
		case MapSizeEnum::Small: initialAnimals /= 16; break;
		}
		unitSystem.AddAnimals(initialAnimals);
	}

	/*
	 * Init display systems
	 */
	_assetLoader->CheckMeshesAvailable();

#if DISPLAY_UNIT
	_unitDisplaySystem->Init(unitSystem.unitCount(), this, _assetLoader, 0);
#endif
	
#if DISPLAY_BUILDING
	_LOG(PunInit, "_buildingDisplaySystem Init");
	_buildingDisplaySystem->Init(GameMapConstants::TotalRegions, this, _assetLoader, 150);
#endif

#if DISPLAY_MINIBUILDING
	_LOG(PunInit, "_miniBuildingDisplaySystem Init");
	_miniBuildingDisplaySystem->Init(GameMapConstants::TotalRegions, this, _assetLoader, 206);
#endif

#if DISPLAY_TERRAIN
	_regionDisplaySystem->Init(GameMapConstants::TotalRegions, this, _assetLoader, 75);
	_terrainLargeDisplaySystem->Init(GameMapConstants::TotalRegions, this, _assetLoader, 22); // 22
#endif

	_debugDisplaySystem->Init(GameMapConstants::TotalRegions, this, _assetLoader, 390);
	//_landmarkDisplaySystem->Init(GameMapConstants::TotalRegions, this, _assetLoader);

#if DISPLAY_REGIONDECAL
	_decalDisplaySystem->Init(GameMapConstants::TotalRegions, this, _assetLoader, 206);
#endif

#if DISPLAY_RESOURCE
	_resourceDisplaySystem->Init(GameMapConstants::TotalRegions, this, _assetLoader, 150);
#endif

#if DISPLAY_TERRITORY
	_territoryDisplaySystem->Init(0, this, _assetLoader, 0);
#endif

	_snowParticles->SetTemplate(_assetLoader->snowParticles); // Trailer blizzard
	_rainParticles->SetTemplate(_assetLoader->rainParticles);
	_rainWetnessDecal->SetDecalMaterial(UMaterialInstanceDynamic::Create(_assetLoader->M_RainWetness, this));

#if DISPLAY_WORLDMAP
	float worldMapSizeX = CoordinateConstants::DisplayUnitPerTile * CoordinateConstants::TilesPerRegion * 4;
	
	_terrainMap->SetupWorldMapMesh(this, GameMapConstants::TilesPerWorldX, GameMapConstants::TilesPerWorldY,
												worldMapSizeX, worldMapSizeX * 2, GetMapSettings().mapSizeEnum(), _assetLoader);
	_terrainMap->InitAnnotations();

	_worldMap->Init(GameMapConstants::TotalRegions, this, _assetLoader, 0);
#endif

#if DISPLAY_TILEOBJ
	_tileDisplaySystem->Init(GameMapConstants::TotalRegions, this, _assetLoader, 206);
#endif

	_buildingMeshesList.Init(this);

	_ue4TickCount = 0;
	_displayTicks = 0;

	/*
	 * SaveCheck
	 */
	auto gameInstance = CastChecked<UPunGameInstance>(GetGameInstance());
	if (gameInstance->saveCheckTransitioning) {
		_LOG(PunSaveCheck, "MapTransitioned, new sim %p", _simulation.get());

		gameInstance->saveCheckTransitioning = false;
		_simulation->saveCheckState = SaveCheckState::SerializeAfterSave;
		_simulation->saveCheckStartTick = Time::Ticks();
	}

	// Next Phase:
	_uiInterface->SetLoadingText(LOCTEXT("Loading_MakingSound", "Making sound..."));

	//PUN_CHECK(assetLoader()->moduleNames().Num() == ModuleMeshCount);
}
AGameManager::~AGameManager()
{
	// Deinit Textures
	if (IsValidPun(_territoryDisplaySystem)) _territoryDisplaySystem->Deinit();
	if (IsValidPun(_decalDisplaySystem)) _decalDisplaySystem->Deinit();
	if (IsValidPun(_terrainMap)) _terrainMap->Deinit();
}

void AGameManager::InitPhase3()
{
	LLM_SCOPE_(EPunSimLLMTag::PUN_GameManager);
	
	SCOPE_TIMER("InitPhase3");

	if (!_isLoadingFromFile)
	{
		// Initial Text
		std::stringstream ss;
		//ss << "The last apocalyptic winter wiped out all our civilizations. ";
		//ss << "The freezing cold and plague reclaimed the world from us.";
		//ss << "<space>";
		//ss << "Long time has passed since, and the ice finally melted...";
		//ss << "<space>";
		//ss << "The task is upon us, surviving few, to rebuild a flourishing civilization.";

		PopupInfo popup(_playerId, {
				LOCTEXT("BeginGameFirstPopup1", "The Great Freeze left civilization in ruin, very few were strong enough to withstand the cold and plague that claimed the earth."),
				LOCTEXT("BeginGameFirstPopup2", "<space>Decades have passed since the frost took our lands, and our courage and hope have returned with the melting of the ice. "),
				LOCTEXT("BeginGameFirstPopup3", "<space>It falls to us, we surviving few, to rebuild a flourishing civilization."),
			},
			{ LOCTEXT("CheersBeginning", "Cheers to the new beginning!") },
			PopupReceiverEnum::StartGame_Story, false, ""
		);
		_simulation->AddPopup(popup);
	}

	/*
	 * Light and sound...
	 */
	auto gameInstance = CastChecked<UPunGameInstance>(GetGameInstance());

	{
		_directionalLight = CastChecked<ADirectionalLight>(PunUnrealUtils::FindWorldActor(GetWorld(), FName("DirectionalLight")));
		_skyLight = CastChecked<ASkyLight>(PunUnrealUtils::FindWorldActor(GetWorld(), FName("SkyLight")));
		AActor* fogActor = CastChecked<AExponentialHeightFog>(PunUnrealUtils::FindWorldActor(GetWorld(), FName("ExponentialHeightFog")));
		_exponentialFogComponent = PunUnrealUtils::GetComponent<UExponentialHeightFogComponent>(fogActor);
		_exponentialFogComponent->SetVisibility(true);
		
		_postProcessVolume = CastChecked<APostProcessVolume>(PunUnrealUtils::FindWorldActor(GetWorld(), FName("GlobalPostProcessVolume")));

#if AUDIO_ALL
		_soundSystem = NewObject<USoundSystemComponent>(this);
		_soundSystem->gameInstance = gameInstance;
		
		_soundSystem->RegisterComponentWithWorld(GetWorld());
		_soundSystem->AttachToComponent(RootComponent, FAttachmentTransformRules::KeepRelativeTransform);
		_soundSystem->RegisterComponent();
		_soundSystem->Init(this, _uiInterface, gameInstance, gameInstance);
#endif
	}
	
	// Stop Menu sound
	gameInstance->StopMenuSound();

	// Load the settings...
	gameInstance->RefreshSoundSettings();

	gameInstance->RefreshOtherSettings();

	// Next Phase:
	_uiInterface->SetLoadingText(NSLOCTEXT("GameManager", "Loading_Completed", "Loading Completed..."));

	PUN_LOG("Module names %d", assetLoader()->moduleNames().Num());
	//PUN_CHECK(assetLoader()->moduleNames().Num() == ModuleMeshCount);
}


static int timerTickCount = 0;
static long timerTimeSpan = 0;

void AGameManager::TickNetworking()
{
	LLM_SCOPE_(EPunSimLLMTag::PUN_GameManager);
	
	auto t1 = high_resolution_clock::now();

	GameRand::SetRandUsageValid(true);

	//// Init phases
	//int32 lastPhase = _initPhase;
	//_initPhase++;
	//switch(lastPhase)
	//{
	//case 1: InitPhase1(); return;
	//case 2: InitPhase2(); return;
	//case 3: InitPhase3(); return;
	//}
	if (_initPhase == 1) {
		InitPhase1();
		_initPhase++;
		return;
	}
	if (_initPhase == 2)
	{
		InitPhase2();
		_initPhase++;
		return;
	}
	if (_initPhase == 3)
	{
		InitPhase3();
		_initPhase++;
		return;
	}

	
	/*
	 * Single Player Does not need any built-in delay
	 */
	if (!gameInstance()->shouldDelayInput()) {
		//PUN_LOG("TICK: gameManager->TickNetworking()");
		_ue4TickCount++;

		for (int32 i = 0; i < _gameTickQueue.size(); i++) {
			//PUN_LOG("TICK: _simulation->Tick()");
			_simulation->Tick(_gameTickQueue.size(), _gameTickQueue[i]);
		}
		_gameTickQueue.clear();
		return;
	}

	
	// Latency adjustment
	int32 gameTickQuarter = 5;
	int32 gameTickHalf = 10;
	int32 gameTickNormal = 50;
	int32 gameTickDouble = 100;

	int32 gameTickClogged = 120;

	// Lower latency for single player mode
	if (isSinglePlayer()) {
		//gameTickQuarter = 1;
		//gameTickHalf = 2;
		//gameTickNormal = 5;
		//gameTickDouble = 10;

		gameTickQuarter = 2;
		gameTickHalf = 4;
		gameTickNormal = 20;
		gameTickDouble = 40;
	}	
	
	// Network design notes:
	// Game tick is already optimized by only waking up unit when needed, 60 fps for game tick will do just fine + make things simpler
	// Each network tick (5 fps) will push 12 game ticks

	// battle.net servers have a built in 250 ms latency.
	// 250ms latency is 4 packets per sec
	// only need to send networkTick only every 15 gameTicks (4/60 = 1/15)
	// Turns only happen 

	// Tick simulation to be inline with server
	_ue4TickCount++;
	if (_gameTickQueue.size() == 0) {
		if (_ue4TickCount % 60 == 0) {
			GEngine->AddOnScreenDebugMessage(-1, 1.0f, FColor::White, FString::Printf(TEXT("No Tick:%d"), _gameTickQueue.size()));
		}
	}
	else if (_gameTickQueue.size() < gameTickQuarter) {
		// Tick at 1/4 the speed
		if (_ue4TickCount % 4 == 0) {
			_simulation->Tick(_gameTickQueue.size(), _gameTickQueue[0]);
			_gameTickQueue.erase(_gameTickQueue.begin());
		}
	}
	else if (_gameTickQueue.size() < gameTickHalf) {
		// Tick at half the speed
		if (_ue4TickCount % 2 == 0) {
			_simulation->Tick(_gameTickQueue.size(), _gameTickQueue[0]);
			_gameTickQueue.erase(_gameTickQueue.begin());
		}
	}
	else if (_gameTickQueue.size() < gameTickNormal) {
		// Tick at normal speed
		_simulation->Tick(_gameTickQueue.size(), _gameTickQueue[0]);
		_gameTickQueue.erase(_gameTickQueue.begin());
	}
	else if (_gameTickQueue.size() < gameTickDouble) {
		// Tick double speed
		_simulation->Tick(_gameTickQueue.size(), _gameTickQueue[0]);
		_gameTickQueue.erase(_gameTickQueue.begin());
		_simulation->Tick(_gameTickQueue.size(), _gameTickQueue[0]);
		_gameTickQueue.erase(_gameTickQueue.begin());
	}
	else {
		// Tick quad speed
		_simulation->Tick(_gameTickQueue.size(), _gameTickQueue[0]);
		_gameTickQueue.erase(_gameTickQueue.begin());
		_simulation->Tick(_gameTickQueue.size(), _gameTickQueue[0]);
		_gameTickQueue.erase(_gameTickQueue.begin());
		_simulation->Tick(_gameTickQueue.size(), _gameTickQueue[0]);
		_gameTickQueue.erase(_gameTickQueue.begin());
		_simulation->Tick(_gameTickQueue.size(), _gameTickQueue[0]);
		_gameTickQueue.erase(_gameTickQueue.begin());
	}

	auto t2 = high_resolution_clock::now();
	auto time_span = duration_cast<microseconds>(t2 - t1);
	timerTimeSpan += time_span.count();

	timerTickCount++;
	if (timerTickCount == 60) 
	{
		//if (GEngine) GEngine->AddOnScreenDebugMessage(-1, 10.0f, FColor::White, FString::Printf(TEXT("Time: %d , Disp: %d"), timerTimeSpan / timerTickCount, timerTimeSpan2 / timerTickCount));
		timerTickCount = 0;
		timerTimeSpan = 0;
	}

	// Update if gameTickQueue is clogged
	bool newCloggedState = _gameTickQueue.size() > gameTickClogged;
	if (newCloggedState != isGameTickQueueClogged) {
		isGameTickQueueClogged = newCloggedState;
		isGameTickQueueCloggedDirty = true;
	}

	GameRand::SetRandUsageValid(false);
}

void AGameManager::Sample()
{
	LLM_SCOPE_(EPunSimLLMTag::PUN_GameManager);
	SCOPE_CYCLE_COUNTER(STAT_PunDisplaySample);

	int32 tileRadius = GetTileRadius(zoomDistance());

	// When this is map-mode, move cameraAtom faraway to despawn all meshes
	WorldTile2 centerTile = ZoomDistanceBelow(WorldToMapZoomAmount) ? cameraAtom().worldTile2() : WorldTile2(-500, -500);
	
	// Debug with single sample
	if (centerTile.isValid() && PunSettings::Get("SingleSample")) 
	{
		_sampleArea = TileArea(centerTile, 1);
		swap(_lastSampleRegionIds, _sampleRegionIds);
		_samplesChanged = _lastSampleRegionIds.size() != _sampleRegionIds.size();
		
		_sampleRegionIds.clear();
		_sampleRegionIds.push_back(centerTile.regionId());
		return;
	}

	// Regions within radius
	_sampleArea = TileArea(centerTile, tileRadius);
	_sampleArea.EnforceWorldLimit();

	swap(_lastSampleRegionIds, _sampleRegionIds);
	_samplesChanged = _lastSampleRegionIds.size() != _sampleRegionIds.size();


	
	/*
	 * Old Sampple Regions
	 */
	//int minRegionX = _sampleArea.minX / CoordinateConstants::TilesPerRegion;
	//int minRegionY = _sampleArea.minY / CoordinateConstants::TilesPerRegion;
	//int maxRegionX = _sampleArea.maxX / CoordinateConstants::TilesPerRegion;
	//int maxRegionY = _sampleArea.maxY / CoordinateConstants::TilesPerRegion;
	//
	//_sampleRegionIds.clear();
	//int i = 0;
	//for (int x = minRegionX; x <= maxRegionX; x++) {
	//	for (int y = minRegionY; y <= maxRegionY; y++) {
	//		int32 regionId = WorldRegion2(x, y).regionId();
	//		_sampleRegionIds.push_back(regionId);

	//		if (i >= _lastSampleRegionIds.size() || regionId != _lastSampleRegionIds[i]) {
	//			_samplesChanged = true;
	//		}
	//		i++;
	//	}
	//}

	/*
	 * Sample regions
	 */
	SampleRegions();
}

void AGameManager::SampleRegions()
{
	LLM_SCOPE_(EPunSimLLMTag::PUN_GameManager);

	// Zoom distance is above threshold, don't sample any region...
	if (PunSettings::TrailerSession ||
		ZoomDistanceBelow(WorldToMapZoomAmount)) 
	{
		SampleRegions(_sampleRegionIds);

		if (SmoothZoomDistanceBelow(WorldZoomTransition_RegionToRegion4x4)) {
			// Below WorldZoomTransition_Region, just use _sampleRegionIds for everything
			_sampleNearRegionIds.clear();
		} else {
			// Above WorldZoomTransition_Region, we need to sample for terrainChunk cache in _sampleNearRegionIds
			SampleRegions(_sampleNearRegionIds, WorldZoomTransition_RegionToRegion4x4);
		}

		_sampleTileObjCacheRegionIds.clear();
	}
	else {
		_sampleRegionIds.clear();
		_sampleNearRegionIds.clear();

		SampleRegions(_sampleTileObjCacheRegionIds, WorldToMapZoomAmount);
	}

	/*
	 * Get _sampleProvinceIds
	 */
	{
		auto& provinceSys = simulation().provinceSystem();
		_sampleProvinceIds.clear();
		for (int32 regionId : _sampleRegionIds) {
			const std::vector<int32>& provinceIds = provinceSys.GetProvinceIdsFromRegionId(regionId);
			for (int32 provinceId : provinceIds) {
				if (provinceSys.IsProvinceValid(provinceId)) {
					CppUtils::TryAdd(_sampleProvinceIds, provinceId);
				}
			}
		}
	}

	//PUN_LOG("Sample regions:%llu nearRegions:%llu", _sampleRegionIds.size(), _sampleNearRegionIds.size());
}

void AGameManager::SampleRegions(std::vector<int32>& sampleRegionIds, float customZoomDistance)
{
	LLM_SCOPE_(EPunSimLLMTag::PUN_GameManager);
	
	sampleRegionIds.clear();
	
	// Get top 2 corners
	auto viewport = GEngine->GameViewport->Viewport;
	//PUN_LOG("viewport %s ... %s", *viewport->GetInitialPositionXY().ToString(), *viewport->GetSizeXY().ToString());
	FVector2D pos00(viewport->GetInitialPositionXY());
	FVector2D pos11(pos00 + FVector2D(viewport->GetSizeXY()));


	FVector origin;
	FVector direction;

	auto addHitRegion = [&](FVector2D screenPos, bool showLine = false)
	{
		UGameplayStatics::DeprojectScreenToWorld(CastChecked<APlayerController>(networkInterface()), screenPos, origin, direction);

		float traceDistance = fabs(origin.Z / direction.Z);
		FVector groundPoint = direction * traceDistance + origin;

		if (showLine) {
			//lineBatch()->DrawLine(FVector(0, 0, 0), groundPoint, FLinearColor::Red, 100.0f, 5.0f);
		}

		return groundPoint;
	};


	// Ground corners
	FVector ground00;
	FVector ground01;
	FVector ground10;

	if (customZoomDistance > 0)
	{
		float customZoomDistanceFraction = customZoomDistance / smoothZoomDistance();
		if (customZoomDistanceFraction < 1.0f)
		{
			FVector2D pos10(pos11.X, pos00.Y);
			FVector2D pos01(pos00.X, pos11.Y);

			ground00 = addHitRegion(pos00 + (pos11 - pos00) * (1.0f - customZoomDistanceFraction) / 2, true);
			ground01 = addHitRegion(pos10 + (pos01 - pos10) * (1.0f - customZoomDistanceFraction) / 2, true);
			ground10 = addHitRegion(pos01 + (pos10 - pos01) * (1.0f - customZoomDistanceFraction) / 2, true);
		}
	}
	else
	{
		// Zoom distance retrieved from the camera
		ground00 = addHitRegion(pos00);
		ground01 = addHitRegion(FVector2D(pos00.X, pos11.Y));
		ground10 = addHitRegion(FVector2D(pos11.X, pos00.Y));
	}

	// ERROR CHECK
	const float maxVecLength = GameMapConstants::TilesPerWorldX * CoordinateConstants::DisplayUnitPerTile * 100;
	if (ground00.Size() > maxVecLength ||
		ground01.Size() > maxVecLength ||
		ground10.Size() > maxVecLength) {
		return;
	}

	// Make a grid out of the 4 ground points with spacing equals to half the region size
	FVector axisX(ground10 - ground00);
	float maxDistanceX = axisX.Size();
	FVector axisDirectionX = axisX.GetSafeNormal();

	FVector axisY(ground01 - ground00);
	float maxDistanceY = axisY.Size();
	FVector axisDirectionY = axisY.GetSafeNormal();

	// ERROR CHECK
	if (axisX.ContainsNaN() || axisY.ContainsNaN() ||
		!FMath::IsFinite(maxDistanceX) || !FMath::IsFinite(maxDistanceY)) {
		return;
	}

	float gridSpacing = CoordinateConstants::DisplayUnitPerRegion / 2;

	auto checkGridPoint = [&](float x, float y)
	{
		FVector checkPoint = ground00 + axisDirectionX * x + axisDirectionY * y;
		//lineBatch()->DrawLine(checkPoint, checkPoint + FVector(0, 0, 30), FLinearColor::Blue, 100.0f, 10.0f);

		WorldRegion2 region = MapUtil::AtomLocation(cameraAtom(), checkPoint).region();
		if (region.IsValid() && WorldTile2::Distance(cameraAtom().worldTile2(), region.centerTile()) < MaxRegionCullDistance) {
			int32 regionId = region.regionId();
			CppUtils::TryAdd(sampleRegionIds, regionId);
		}
	};

	//for (float y = gridSpacing; y < maxDistanceY; y += gridSpacing) { // TODO: changed y = 0 to detect beyond screen top...
	for (float y = 0; y < maxDistanceY; y += gridSpacing) {
		for (float x = gridSpacing; x < maxDistanceX; x += gridSpacing) {
			checkGridPoint(x, y);
		}
	}

	// Note rims get checked with less spacing to prevent holes
	float rimGridSpacing = gridSpacing / 8;

	// Apply tilt to grid check so that chunks don't all spawn at the same time when moving exactly NSEW
	float gridCount = maxDistanceY / rimGridSpacing;
	float targetMaxTilt = rimGridSpacing;
	float tiltIncrement = targetMaxTilt / gridCount;

	// Rim X
	for (float y = 0; y < maxDistanceY; y += rimGridSpacing) {
		checkGridPoint(-rimGridSpacing - y * tiltIncrement, y);
		checkGridPoint(0, y);
		checkGridPoint(maxDistanceX, y);
		checkGridPoint(maxDistanceX + rimGridSpacing + y * tiltIncrement, y);
	}

	// Rim Y
	for (float x = 0; x < maxDistanceX; x += rimGridSpacing) {
		checkGridPoint(x, -rimGridSpacing - x * tiltIncrement);
		checkGridPoint(x, 0);
		checkGridPoint(x, maxDistanceY);
		checkGridPoint(x, maxDistanceY + rimGridSpacing + x * tiltIncrement);
	}

	checkGridPoint(maxDistanceX, maxDistanceY);
}


void AGameManager::TickDisplay(float DeltaTime, WorldAtom2 cameraAtom, float zoomDistance, float smoothZoomDistance, FVector cameraLocation, bool isPhotoMode, bool isBounceZooming)
{
	LLM_SCOPE_(EPunSimLLMTag::PUN_GameManager);
	
	if (!isGameManagerInitialized()) {
		return;
	}

	// Practice Tick Display
	//if (_initPhase <= 7)
	//{
	//	INIT_LOG("Practice Tick Display");
	//	cameraAtom = WorldTile2(GameMapConstants::TilesPerWorldX / 2, GameMapConstants::TilesPerWorldY / 2).worldAtom2();
	//	zoomDistance = WorldZoomTransition_Tree - 10;
	//	_initPhase++;
	//}

	_isPhotoMode = isPhotoMode;
	
	
	_tickStartTime = high_resolution_clock::now();
	
	{
		SCOPE_CYCLE_COUNTER(STAT_PunDisplayTickMisc);
		//SCOPE_TIMER("Tick Sample Outer");
		
		// CameraOffset
		FLinearColor cameraOffsetColor(float(cameraAtom.x) / CoordinateConstants::AtomPerDisplayUnit,
										float(cameraAtom.y) / CoordinateConstants::AtomPerDisplayUnit, 0.0f, 0.0f);
		_assetLoader->SetMaterialCollectionParametersVector("CameraOffset", cameraOffsetColor);

		_cameraAtom = cameraAtom;
		_zoomDistance = zoomDistance;
		_smoothZoomDistance = smoothZoomDistance;
		
		_cameraLocation = cameraLocation;

		_isBounceZooming = isBounceZooming;
		
		// Determine what to display
		Sample();
	}

	std::vector<int> noSample;
	
	{
		SCOPE_CYCLE_COUNTER(STAT_PunDisplayTickCore);
		//SCOPE_TIMER_("--- Tick Camera zoom:%f", zoomDistance);

		{
#if DISPLAY_UNIT
			SCOPE_CYCLE_COUNTER(STAT_PunDisplayUnit);
			SCOPE_TIMER_FILTER(5000, "** Tick Unit -");
			
			bool displayUnits = PunSettings::IsOn("DisplayUnits") && zoomDistance < WorldZoomTransition_Unit;
			_unitDisplaySystem->Display(displayUnits ? _sampleRegionIds : noSample);
#endif
		}

		{
			bool buildingDisplayOn = PunSettings::IsOn("DisplayBuildings");
			
			bool displayBuildings = buildingDisplayOn && zoomDistance < WorldZoomTransition_Buildings;

#if DISPLAY_BUILDING
			{
				SCOPE_CYCLE_COUNTER(STAT_PunDisplayBuilding);
				SCOPE_TIMER_FILTER(5000, "** Tick Building -");

				_buildingDisplaySystem->Display(displayBuildings ? _sampleRegionIds : noSample);
			}
#endif

#if DISPLAY_MINIBUILDING
			bool miniBuildingDisplayOn = PunSettings::IsOn("DisplayMiniBuildings");
			bool displayMiniBuildings = miniBuildingDisplayOn && !displayBuildings && zoomDistance < WorldZoomTransition_BuildingsMini;
			
			if (PunSettings::TrailerMode()) {
				displayMiniBuildings = miniBuildingDisplayOn && !displayBuildings;
			}
			
			{
				SCOPE_CYCLE_COUNTER(STAT_PunDisplayMiniBuilding);
				SCOPE_TIMER_FILTER(5000, "** Tick MiniBuilding -");
				
				_miniBuildingDisplaySystem->Display(displayMiniBuildings ? _sampleRegionIds : noSample);
			}
#endif
		}

		{
#if DISPLAY_TERRAIN
			if (PunSettings::IsOn("DisplayRegions"))
			{
				SCOPE_CYCLE_COUNTER(STAT_PunDisplayRegion);
				SCOPE_TIMER_FILTER(5000, "** Tick Region - zoom:%f trans:%f bouncing:%d", smoothZoomDistance, WorldZoomTransition_RegionToRegion4x4, isBounceZooming);
				
				if (smoothZoomDistance < WorldZoomTransition_RegionToRegion4x4_Mid ||
					PunSettings::TrailerSession)
				{
					_regionDisplaySystem->BeforeDisplay(false);

					{
						//SCOPE_TIMER_FILTER(1000, "Tick Region Display");
						_regionDisplaySystem->Display(_sampleRegionIds);
					}
					
					//_regionDisplaySystem->AfterDisplay();
				}
				else {
					_regionDisplaySystem->Display(noSample);
				}

			}
			else {
				_regionDisplaySystem->Display(noSample);
			}

			if (PunSettings::IsOn("DisplayRegions4x4"))
			{
				SCOPE_CYCLE_COUNTER(STAT_PunDisplayRegion);
				SCOPE_TIMER_FILTER(5000, "** Tick Region4x4 - zoom:%f trans:%f bouncing:%d", smoothZoomDistance, WorldZoomTransition_RegionToRegion4x4, isBounceZooming);

				// Terrain large is 4x4 regions
				auto displayNormally = [&]()
				{
					std::vector<int32> sampleNearRegionIds4x4;
					for (int32 regionId : _sampleRegionIds) {
						WorldRegion2 region(regionId);
						CppUtils::TryAdd(sampleNearRegionIds4x4, WorldRegion2(region.x / 4 * 4, region.y / 4 * 4).regionId());
					}

					_terrainLargeDisplaySystem->Display(sampleNearRegionIds4x4);

					
				};
				
				if (PunSettings::TrailerSession) {
					displayNormally();
				}
				else
				{
					if (smoothZoomDistance < WorldZoomTransition_RegionToRegion4x4) {
						_terrainLargeDisplaySystem->Display(noSample);
					}
					else if (smoothZoomDistance < WorldZoomTransition_Region4x4ToMap) {
						displayNormally();
					}
					else {
						_terrainLargeDisplaySystem->Display(noSample);
					}
				}
			}
			else {
				_terrainLargeDisplaySystem->Display(noSample);
			}
#endif
		}

		{
			if (smoothZoomDistance < WorldZoomTransition_Tree) {
				_debugDisplaySystem->Display(_sampleRegionIds);
			}
		}

		//{
		//	SCOPE_CYCLE_COUNTER(STAT_PunDisplayLandmark);
		//	SCOPE_TIMER_FILTER(5000, "Tick Landmark -");

		//	bool displayLandmark = zoomDistance < WorldZoomAmountStage3;
		//	_landmarkDisplaySystem->UpdateLandmarkDisplay(displayLandmark ? _sampleRegionIds : noSample);
		//}

		{
#if DISPLAY_REGIONDECAL
			SCOPE_CYCLE_COUNTER(STAT_PunDisplayLandmark);
			SCOPE_TIMER_FILTER(5000, "** Tick Decals -");
			
			bool displayRegionDecal = PunSettings::IsOn("DisplayDecals") && zoomDistance < WorldToMapZoomAmount;
			if (PunSettings::TrailerMode()) {
				displayRegionDecal = true;
			}
			_decalDisplaySystem->Display(displayRegionDecal ? _sampleRegionIds : noSample);
#endif
		}

		{
#if DISPLAY_TILEOBJ
			SCOPE_CYCLE_COUNTER(STAT_PunDisplayTree);
			SCOPE_TIMER_FILTER(5000, "** Tick TileObj -");

			bool isOn = PunSettings::IsOn("DisplayTiles");
			bool isZoomedAllTheWayOut = zoomDistance > WorldZoomTransition_Tree;

			// Note: TODO: turn this on for Non-Trailer
			if (isZoomedAllTheWayOut && !PunSettings::TrailerSession) {
				// TODO: Is this cache working?
				// Zoomed all the way out, cache tileObj for when we zoom in...
				_tileDisplaySystem->BeforeDisplay(false, (isOn ? _sampleTileObjCacheRegionIds : noSample), true, zoomDistance > WorldZoomTransition_TreeHidden);
				_tileDisplaySystem->Display(isOn ? _sampleTileObjCacheRegionIds : noSample);
			}
			else {
				bool displayTileObj = isOn;
				bool displayFull = PunSettings::IsOn("TileObjFull") && smoothZoomDistance < WorldZoomTransition_Bush;

				if (PunSettings::TrailerSession) {
					displayFull = true;
				}

				_tileDisplaySystem->BeforeDisplay(displayFull, _sampleRegionIds, isZoomedAllTheWayOut);
				_tileDisplaySystem->Display(displayTileObj ? _sampleRegionIds : noSample);
			}
			
			//_tileDisplaySystem->AfterDisplay();
#endif
		}

		{
#if DISPLAY_RESOURCE
			SCOPE_CYCLE_COUNTER(STAT_PunDisplayResource);
			SCOPE_TIMER_FILTER(5000, "Tick Resource -");
			
			bool displayResources = PunSettings::IsOn("DisplayResources") && ZoomDistanceBelow(WorldZoomAmountStage3);
			_resourceDisplaySystem->Display(displayResources ? _sampleRegionIds : noSample);
#endif
		}

		// Don't display shadow in map mode...
		_directionalLight->SetCastShadows(ZoomDistanceBelow(WorldToMapZoomAmount));
	}

	{
		SCOPE_CYCLE_COUNTER(STAT_PunDisplayTickMisc);

		{
#if DISPLAY_TERRITORY
			SCOPE_CYCLE_COUNTER(STAT_PunDisplayTerritory);
			SCOPE_TIMER_FILTER(5000, "Tick Territory -");
			
			//SCOPE_TIMER("Tick -- Territory");
			bool displayTerritory = PunSettings::IsOn("DisplayTerritory") && 
									!PunSettings::TrailerSession;
			_territoryDisplaySystem->Display(displayTerritory ? _sampleProvinceIds : noSample);
#endif
		}

		{
			//SCOPE_TIMER("Tick -- Snow/Rain");
			
			_snowParticles->bSuppressSpawning = !Time::IsSnowing();
			_rainParticles->bSuppressSpawning = !Time::IsRaining();

			// Rain fog
			const float fogFadeInSeconds = 3.0f;
			const float maxFogDensity = 0.2f; // 0.3f
			const float fogFadeSpeed = maxFogDensity / fogFadeInSeconds;
			float fogDensity = _exponentialFogComponent->FogDensity;
			if (Time::IsRaining() || Time::IsFogging()) {
				fogDensity = fmin(maxFogDensity, fogDensity + fogFadeSpeed * DeltaTime);
			} else {
				fogDensity = fmax(0.0f, fogDensity - fogFadeSpeed * DeltaTime);
			}
			
			if (PunSettings::IsOn("ForceNoFog")) {
				fogDensity = 0;
			}
			
			_exponentialFogComponent->SetFogDensity(fogDensity);

			// Wetness
			const float wetFadeSpeed = 1.0f / fogFadeInSeconds;
			if (Time::IsRaining()) {
				if (!_rainWetnessDecal->IsVisible()) {
					_rainWetnessDecal->SetVisibility(true);
				}
				rainIntensity = fmin(1.0f, rainIntensity + wetFadeSpeed * DeltaTime);
				auto materialInst = CastChecked<UMaterialInstanceDynamic>(_rainWetnessDecal->GetDecalMaterial());
				materialInst->SetScalarParameterValue("RainIntensity", rainIntensity);
				_rainWetnessDecal->SetWorldScale3D(FVector::OneVector * MapUtil::GlobalDecalZoomFactor(zoomDistance));
			}
			else {
				if (_rainWetnessDecal->IsVisible()) {
					rainIntensity = fmax(0.0f, rainIntensity - wetFadeSpeed * DeltaTime);
					auto materialInst = CastChecked<UMaterialInstanceDynamic>(_rainWetnessDecal->GetDecalMaterial());
					materialInst->SetScalarParameterValue("RainIntensity", rainIntensity);
					_rainWetnessDecal->SetWorldScale3D(FVector::OneVector * MapUtil::GlobalDecalZoomFactor(zoomDistance));

					if (rainIntensity == 0.0f) {
						_rainWetnessDecal->SetVisibility(false);
					}
				}
			}
		}

		{
			SCOPE_TIMER_FILTER(5000, "Tick -- Map");
			bool isVisible = PunSettings::IsOn("DisplayTerrainMap") && (smoothZoomDistance >= WorldZoomTransition_Region4x4ToMap_SlightBelow);
			bool isWaterVisible = PunSettings::IsOn("DisplayTerrainMap") && (smoothZoomDistance >= WorldZoomTransition_RegionToRegion4x4);
			bool isColliderVisible = (smoothZoomDistance >= WorldZoomTransition_RegionToRegion4x4_Mid);

#if DISPLAY_TILEOBJ
			bool tileDisplayNoRegionSkip = _tileDisplaySystem->NoRegionSkipThisTick();
#else
			bool tileDisplayNoRegionSkip = true;
#endif

#if DISPLAY_WORLDMAP
			_terrainMap->UpdateTerrainMapDisplay(isVisible, isWaterVisible, tileDisplayNoRegionSkip);

			_worldMap->Update(isVisible, isColliderVisible);
#endif
		}
	}

	{
		//SCOPE_TIMER("Tick -- Misc");
		
		auto directionalLight = CastChecked<UDirectionalLightComponent>(_directionalLight->GetLightComponent());
		auto skyLight = CastChecked<USkyLightComponent>(_skyLight->GetLightComponent());

		/*
		 * Shadow Distance
		 */
		{
			// Shadow distance
			// From plotting, we get this equation
			// As we get close to MinZoomAmount, the camera pitch up... Hence, we should keep the shadow distance farther
			float cappedZoomDistance = std::max(_smoothZoomDistance, MinZoomAmount * 3);
			float shadowDistance = 1.728 * cappedZoomDistance + 58.674;
			//shadowDistance *= 1.2; // Adjustment...

			//if (isPhotoMode) {
			//	shadowDistance *= 2;
			//}

			//PUN_LOG("shadowDistance %f", shadowDistance);

			shadowDistance *= ShadowDistanceMultiplier;

			// Shadow distance multiplier for Low GameSettings which adjusted the shadow distance to too low
			if (GEngine && GEngine->GameUserSettings) {
				int32 shadowQuality = GEngine->GameUserSettings->ScalabilityQuality.ShadowQuality;
				if (shadowQuality == 1) {
					shadowDistance *= 1.25;
				}
				if (shadowQuality == 2) {
					shadowDistance *= 1.15;
				}
			}

			directionalLight->SetDynamicShadowDistanceMovableLight(shadowDistance);

			// Dim winter light
			const float defaultLightIntensity = 2.5;
			const float winterLightIntensity = 2.0;
			float lightIntensity = defaultLightIntensity + (winterLightIntensity - defaultLightIntensity) * simulation().snowHeightForestStart();
			directionalLight->SetIntensity(lightIntensity);

			skyLight->SetIntensity(0.5);

			//PUN_LOG("directionalLight %f sky %f", directionalLight->Intensity, skyLight->Intensity);
		}


		{
			SetPostProcessVolume(_postProcessVolume, gameInstance());

			/*
			 * Ambient Occlusion turn off in outer map.
			 */
			float zoomAmountLerpLow = GetCameraZoomAmount(zoomStepBounceLow);
			float zoomAmountLerpHigh = GetCameraZoomAmount(zoomStepBounceHigh);
			float maxIntensity = 0.75f;

			float intensity01 = 1.0f - Clamp01((smoothZoomDistance - zoomAmountLerpLow) / (zoomAmountLerpHigh - zoomAmountLerpLow));
			_postProcessVolume->Settings.AmbientOcclusionIntensity = maxIntensity * intensity01;
		}
		

		{
#if AUDIO_ALL
			_soundSystem->UpdateParameters(playerId(), DeltaTime);
#endif
		}
		
		_assetLoader->SetMaterialCollectionParametersScalar("ZoomDistance", smoothZoomDistance);

		// Weather...
		_assetLoader->SetMaterialCollectionParametersScalar("SnowTundraStart", simulation().snowHeightTundraStart());
		_assetLoader->SetMaterialCollectionParametersScalar("SnowBorealStart", simulation().snowHeightBorealStart());
		_assetLoader->SetMaterialCollectionParametersScalar("SnowForestStart", simulation().snowHeightForestStart());

		//PUN_LOG("Snow %f, %f, %f", simulation().snowHeightTundraStart(), simulation().snowHeightBorealStart(), simulation().snowHeightForestStart());
		
		_assetLoader->SetMaterialCollectionParametersScalar("FallSeason", Time::FallSeason());
		_assetLoader->SetMaterialCollectionParametersScalar("SpringSeason", Time::SpringSeason());
		_assetLoader->SetMaterialCollectionParametersScalar("FloweringIntensity", Time::FloweringIntensity());


		//FLinearColor cameraOffsetColor(float(cameraAtom.x) / CoordinateConstants::AtomPerDisplayUnit,
		//								float(cameraAtom.y) / CoordinateConstants::AtomPerDisplayUnit, 0.0f, 0.0f);
		//_assetLoader->SetMaterialCollectionParametersVector("CameraOffset", cameraOffsetColor);
		_assetLoader->SetMaterialCollectionParametersVector("LightDirection", directionalLight->GetDirection());
	}

	_displayTicks++;
	TimeDisplay::SetTicks(_displayTicks);
}


#undef LOCTEXT_NAMESPACE