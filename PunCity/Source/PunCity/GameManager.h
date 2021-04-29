#pragma once

#include "GameFramework/Actor.h"
#include "PunCity/Simulation/GameSimulationCore.h"

#include "PunCity/DisplaySystem/UnitDisplayComponent.h"
#include "PunCity/DisplaySystem/BuildingDisplayComponent.h"
#include "PunCity/DisplaySystem/RegionDisplayComponent.h"
#include "PunCity/DisplaySystem/TileObjectDisplayComponent.h"
#include "PunCity/DisplaySystem/ResourceDisplayComponent.h"

#include "PunCity/DisplaySystem/TerritoryDisplayComponent.h"
#include "PunCity/DisplaySystem/TerrainMapComponent.h"

#include "PunCity/UI/GameUIDataSource.h"
#include "PunCity/MapUtil.h"

#include "PunCity/GameManagerInterface.h"
#include "PunCity/NetworkStructs.h"

#include <chrono>

#include "Engine/ExponentialHeightFog.h"
#include "PunCity/DisplaySystem/RegionLandmarkDisplayComponent.h"
#include "PunCity/DisplaySystem/RegionDecalDisplayComponent.h"
#include "PunCity/DisplaySystem/BuildingMeshesListComponent.h"
#include "PunCity/PunGameInstance.h"
#include "PunCity/Sound/SoundSystemComponent.h"
#include "PunCity/DisplaySystem/MiniBuildingDisplayComponent.h"
#include "PunCity/DisplaySystem/DebugDisplayComponent.h"
#include "PunCity/DisplaySystem/TerrainLargeDisplayComponent.h"
#include "PunCity/DisplaySystem/WorldMapMeshComponent.h"


DECLARE_CYCLE_STAT(TEXT("PUN: [Sound] Dropoff"), STAT_PunSoundDropoff, STATGROUP_Game);
DECLARE_CYCLE_STAT(TEXT("PUN: [Sound] Animal"), STAT_PunSoundAnimal, STATGROUP_Game);
DECLARE_CYCLE_STAT(TEXT("PUN: [Sound] 3D"), STAT_PunSound3D, STATGROUP_Game);
DECLARE_CYCLE_STAT(TEXT("PUN: [Sound] 2D"), STAT_PunSound2D, STATGROUP_Game);

#include "GameManager.generated.h"

UCLASS()
class AGameManager : public AActor, 
										public IGameManagerInterface,
										public IDisplaySystemDataSource,
										public IGameUIDataSource,
										public IGameSoundInterface
{
	GENERATED_BODY()
public:	
	// Sets default values for this actor's properties
	AGameManager();
	~AGameManager();

	template <typename T>
	void HideActorType() {
		for (TActorIterator<T> ActorItr(GetWorld()); ActorItr; ++ActorItr)
		{
			// Same as with the Object Iterator, access the subclass instance with the * or -> operators.
			auto component = *ActorItr;
			component->SetActorHiddenInGame(true);
			component->SetActorEnableCollision(false);
			component->SetActorTickEnabled(false);
		}
	}

	void Init(int32 playerId, bool isLoadingFromFile, IGameUIInterface* uiInterface, IGameNetworkInterface* networkInterface);

	void InitPhase1();
	void InitPhase2();
	void InitPhase3();

	bool isGameManagerInitialized() { return _initPhase > 3; }
	//bool isDisplayPracticeDone() { return _initPhase > 5; }

	int simulationTickCount() { return _simulation->tickCount(); }
	//int networkTickQueueCount() { return _gameTickQueue.size(); }


	void AddGameTick(NetworkTickInfo networkTick) {
		//PUN_LOG("TICK: gameManager->AddGameTick %d", networkTick.tickCount);
		_gameTickQueue.push_back(networkTick);
	}

	void TickNetworking();
	void TickDisplay(float DeltaTime, WorldAtom2 cameraAtom, float zoomDistance, float smoothZoomDistance, FVector cameraLocation, bool isPhotoMode, bool isBounceZooming);

	void UIMeshAfterAdd() {
		_buildingMeshesList.AfterAdd(); // This should be after all UI to prevent flash...	
	}

	IGameNetworkInterface* networkInterface() override { return _networkInterface; }

	void RefreshHeightForestColorTexture(TileArea area, bool isInstant) override {
		_terrainMap->RefreshHeightForestColorTexture(area, isInstant);
	}
	void SetRoadWorldTexture(WorldTile2 tile, bool isRoad, bool isDirtRoad) override {
		_terrainMap->SetRoadWorldTexture(tile, isRoad, isDirtRoad);
	}
	void RefreshHeightForestRoadTexture() override {
		_terrainMap->RefreshHeightForestRoadTexture();
	}

	void RefreshMapAnnotation() override {
		_terrainMap->RefreshAnnotations();
	}
	

	class ADirectionalLight* directionalLight() { return _directionalLight; }


	/*
	 * Debug
	 */
	void PrintMeshPoolCount() {
		PUN_LOG("PrintMeshPoolCount _buildingDisplaySystem:%d", _buildingDisplaySystem->meshPoolCount());
		PUN_LOG("PrintMeshPoolCount _miniBuildingDisplaySystem:%d", _miniBuildingDisplaySystem->meshPoolCount());
		PUN_LOG("PrintMeshPoolCount _regionDisplaySystem:%d", _regionDisplaySystem->meshPoolCount());
		PUN_LOG("PrintMeshPoolCount _terrainLargeDisplaySystem:%d", _terrainLargeDisplaySystem->meshPoolCount());
		PUN_LOG("PrintMeshPoolCount _buildingDisplaySystem:%d", _buildingDisplaySystem->meshPoolCount());

		PUN_LOG("PrintMeshPoolCount _debugDisplaySystem:%d", _debugDisplaySystem->meshPoolCount());
		PUN_LOG("PrintMeshPoolCount _decalDisplaySystem:%d", _decalDisplaySystem->meshPoolCount());
		PUN_LOG("PrintMeshPoolCount _resourceDisplaySystem:%d", _resourceDisplaySystem->meshPoolCount());
		PUN_LOG("PrintMeshPoolCount _worldMap:%d", _worldMap->meshPoolCount());
		PUN_LOG("PrintMeshPoolCount _tileDisplaySystem:%d", _tileDisplaySystem->meshPoolCount());
	}

	URegionDisplayComponent* regionDisplaySystem() { return _regionDisplaySystem;  }
	UTerrainLargeDisplayComponent* terrainLargeDisplaySystem() { return _terrainLargeDisplaySystem; }
	UUnitDisplayComponent* unitDisplaySystem() { return _unitDisplaySystem; }

public:
	UPROPERTY(EditAnywhere, BlueprintReadWrite) USceneComponent* _root;

	bool isGameTickQueueCloggedDirty = false; // If dirty, we need to notify server that _gameTickQueue is clogged, or no longer clogged
	bool isGameTickQueueClogged = false;

	UPROPERTY(EditAnywhere) bool PaintConstructionMesh;
	UPROPERTY(EditAnywhere) bool PrintConstructionMesh;
	UPROPERTY(EditAnywhere) bool RemoveVertexColor;

	// Trouble?: Don't forget to set viewport to realtime
	bool ShouldTickIfViewportsOnly() const override { return true; }
	void Tick(float DeltaSeconds) override {
		//PUN_LOG("Tick GameManager...");

		if (PaintConstructionMesh) {
			_assetLoader->PaintConstructionMesh();
			PaintConstructionMesh = false;
		}
		if (PrintConstructionMesh) {
			_assetLoader->PrintConstructionMesh();
			PrintConstructionMesh = false;
		}
		if (RemoveVertexColor) {
			_assetLoader->RemoveVertexColor();
			RemoveVertexColor = false;
		}
	}

	UPROPERTY(EditAnywhere) USceneComponent* worldWidgetParent;
	UPROPERTY(EditAnywhere, BlueprintReadWrite) UTerrainMapComponent* _terrainMap;
	UPROPERTY(EditAnywhere, BlueprintReadWrite) UWorldMapMeshComponent* _worldMap;

	// Photo taking
	float ShadowDistanceMultiplier = 1.0f;
	float MaxRegionCullDistance = 10 * 32;

private:
	UPunGameInstance* gameInstance() {
		return CastChecked<UPunGameInstance>(GetGameInstance());
	}
	
private:
	IGameUIInterface* _uiInterface = nullptr;
	IGameNetworkInterface* _networkInterface = nullptr;
	
	std::unique_ptr<GameSimulationCore> _simulation;

	/*
	 * Display Systems
	 */
	
	UPROPERTY(EditAnywhere) UUnitDisplayComponent* _unitDisplaySystem;
	UPROPERTY(EditAnywhere) UBuildingDisplayComponent* _buildingDisplaySystem;
	UPROPERTY(EditAnywhere) UMiniBuildingDisplayComponent* _miniBuildingDisplaySystem;
	UPROPERTY(EditAnywhere) URegionDisplayComponent* _regionDisplaySystem;
	UPROPERTY(EditAnywhere) UTerrainLargeDisplayComponent* _terrainLargeDisplaySystem;
	
	UPROPERTY(EditAnywhere) UDebugDisplayComponent* _debugDisplaySystem;
	
	UPROPERTY(EditAnywhere) URegionDecalDisplayComponent* _decalDisplaySystem;
	
	UPROPERTY(EditAnywhere) UTileObjectDisplayComponent* _tileDisplaySystem;
	UPROPERTY(EditAnywhere) UResourceDisplayComponent* _resourceDisplaySystem;

	UPROPERTY(EditAnywhere) UTerritoryDisplayComponent* _territoryDisplaySystem;
	
	UPROPERTY(EditAnywhere) class UParticleSystemComponent* _snowParticles;
	UPROPERTY(EditAnywhere) class UParticleSystemComponent* _rainParticles;
	UPROPERTY(EditAnywhere) UDecalComponent* _rainWetnessDecal;
	float rainIntensity = 0.0f;

	UPROPERTY(EditAnywhere) class ADirectionalLight* _directionalLight;
	UPROPERTY(EditAnywhere) class ASkyLight* _skyLight;
	UPROPERTY(EditAnywhere) UExponentialHeightFogComponent* _exponentialFogComponent;

	UPROPERTY(EditAnywhere) APostProcessVolume* _postProcessVolume;

	UPROPERTY(EditAnywhere) USoundSystemComponent* _soundSystem;
	

	BuildingMeshesListComponent _buildingMeshesList;

	/*
	 * 
	 */

	GameDisplayInfo _displayInfo;

	std::vector<int32> _sampleRegionIds;
	std::vector<int32> _lastSampleRegionIds;
	bool _samplesChanged = false; // TODO: remove??
	
	std::vector<int32> _sampleNearRegionIds; // RegionIds with zoom limit, this is used to pregenerate terrain chunks while zoomed out (before zooming in)
	std::vector<int32> _sampleTileObjCacheRegionIds;

	std::vector<int32> _sampleProvinceIds;
	
	void Sample();
	void SampleRegions();
	void SampleRegions(std::vector<int32>& sampleRegionIds, float customZoomDistance = -1);

	std::vector<NetworkTickInfo> _gameTickQueue;
	int _ue4TickCount = 0;

	UPROPERTY(EditAnywhere) UAssetLoaderComponent* _assetLoader;

	WorldAtom2 _cameraAtom = WorldAtom2::Zero;
	TileArea _sampleArea;
	float _zoomDistance = 0.0f;
	float _smoothZoomDistance = 0.0f;
	FVector _cameraLocation;

	bool _isPhotoMode = false;

	bool _isBounceZooming = false;

	std::chrono::steady_clock::time_point _tickStartTime;

	int32 _playerId = -1;

	int32 _displayTicks;

	std::vector<OverlayType> _overlaySetterTypeToType;
	WorldTile2 _overlayCenterTile = WorldTile2::Invalid;

	bool _isHidingTrees = false;
	bool _showProvinceOverlay = false;

	bool _isCtrlDown = false;
	bool _isShiftDown = false;
	bool _alwaysShowProvinceHover = false;

	int _initPhase = 0;
	bool _isLoadingFromFile = false;

	ResourceEnum _tipResourceEnum = ResourceEnum::None;


public:
	/** 
	 * IGameUIDataSource
	 */

	int32 playerId() override { return _playerId; }

	int32 GetResourceCount(int32 townId, ResourceEnum resourceEnum) final {
		return _simulation->resourceCountTown(townId, resourceEnum);
	}
	int32 GetResourceCount(int32 townId, const std::vector<ResourceEnum>& resourceEnums) final {
		int32 result = 0;
		for (ResourceEnum resourceEnum : resourceEnums) {
			result += _simulation->resourceCountTown(townId, resourceEnum);
		}
		return result;
	}

	FVector GetBuildingTrueCenterDisplayLocation(int objectId, WorldAtom2 cameraAtom) final {
		WorldAtom2 actualLocation = _simulation->buildingSystem().actualAtomLocation(objectId);
		FVector displayLocation = MapUtil::DisplayLocation(cameraAtom, actualLocation);

		Building& building = _simulation->building(objectId);
		AlgorithmUtils::ShiftDisplayLocationToTrueCenter(displayLocation, building.area(), building.faceDirection());
		return displayLocation;
	}
	

	Building& GetBuilding(int buildingId) final {
		return _simulation->buildingSystem().building(buildingId);
	}

	const std::vector<int32>& sampleRegionIds() final { return _sampleRegionIds; }
	const std::vector<int32>& sampleNearRegionIds() final { return _sampleNearRegionIds; }

	FVector GetUnitDisplayLocation(int objectId, WorldAtom2 cameraAtom) final {
		WorldAtom2 actualLocation = _simulation->unitSystem().actualAtomLocation(objectId);
		return MapUtil::DisplayLocation(cameraAtom, actualLocation);
	}

	UnitStateAI& GetUnitStateAI(int32 unitId) final {
		return _simulation->unitSystem().unitStateAI(unitId);
	}
	bool IsUnitValid(int32 unitId) final {
		return _simulation->unitSystem().IsUnitValid(unitId);
	}

	GameSimulationCore& simulation() final {
		PUN_CHECK(_simulation);
		return *_simulation;
	}
	GameSimulationCore* simulationPtr() final { return _simulation.get(); }
	bool HasSimulation() final { return _simulation != nullptr; }
	
	IUnitDataSource& unitDataSource() final { return _simulation->unitSystem(); }

	UnitDisplayState GetUnitTransformAndVariation(UnitStateAI& unit, FTransform& transform) final {
#if !DISPLAY_UNIT
		return { unit.unitEnum(), unit.animationEnum(), 0};
#endif
		return _unitDisplaySystem->GetUnitTransformAndVariation(unit, transform);
	}
	

	FVector DisplayLocation(WorldAtom2 atom) final { return MapUtil::DisplayLocation(_cameraAtom, atom); }

	FVector DisplayLocationTrueCenter(Building& building, bool withHeight = false) final
	{
		FVector displayLocationScope = DisplayLocation(building.centerTile().worldAtom2());
		AlgorithmUtils::ShiftDisplayLocationToTrueCenter(displayLocationScope, building.area(), building.faceDirection());

		if (withHeight) {
			displayLocationScope.Z = building.GetBuildingSelectorHeight();
		}
		return displayLocationScope;
	}

	int32 GetBuildingDisplayObjectId(int32 meshId, FString protoName, int32 instanceIndex) final {
#if !DISPLAY_BUILDING
		return 0;
#endif
		return _buildingDisplaySystem->GetObjectId(meshId, protoName, instanceIndex);
	}
	int32 GetTileObjDisplayObjectId(int32 meshId, FString protoName, int32 instanceIndex) final {
#if !DISPLAY_TILEOBJ
		return 0;
#endif
		return _tileDisplaySystem->GetObjectId(meshId, protoName, instanceIndex);
	}

	USoundSystemComponent* soundSystem() final { return _soundSystem; }
	
	float NearestSeaDistance() final
	{	
		auto& provinceSys = simulation().provinceSystem();
		float minSeaDistance = 100000.0f;
		for (int32 regionId : _sampleRegionIds) 
		{
			auto tryMinDist = [&](WorldTile2 tile) {
				float distance = WorldTile2::Distance(tile, _cameraAtom.worldTile2());
				if (distance < minSeaDistance) {
					minSeaDistance = distance;
				}
			};
			
			// Deep ocean
			if (provinceSys.GetProvinceIdRaw(WorldRegion2(regionId).centerTile()) == OceanProvinceId) {
				tryMinDist(WorldRegion2(regionId).centerTile());
			}
			
			const std::vector<int32>& provinceIds = provinceSys.GetProvinceIdsFromRegionId(regionId);
			for (int32 provinceId : provinceIds) {
				// make sea sound from coast
				if (provinceSys.provinceIsCoastal(provinceId)) {
					tryMinDist(provinceSys.GetProvinceCenterTile(provinceId));
				}
			}
		}
		return minSeaDistance;
	}

	/*
	 * SpawnBuildingMesh must be called every tick to show it
	 */
	void ShowBuildingMesh(Building& building, int customDepth = 0, bool receiveDecal = true) final {
		if (_isPhotoMode) {
			return;
		}
		_buildingMeshesList.ShowBuildingMesh(building, customDepth, receiveDecal);
	}
	void ShowBuildingMesh(WorldTile2 tile, Direction faceDirection, const std::vector<ModuleTransform>& modules, int32 customDepthIndex, bool receiveDecal = true) final {
		if (_isPhotoMode) {
			return;
		}
		_buildingMeshesList.ShowBuildingMesh(tile, faceDirection, modules, customDepthIndex, receiveDecal);
	}
	void ShowStorageMesh(TileArea area, WorldTile2 centerTile, int customDepth = 0) final {
		if (_isPhotoMode) {
			return;
		}
		_buildingMeshesList.ShowStorageMesh(area, centerTile, customDepth);
	}
	
	UDecalComponent* ShowDecal(TileArea area, UMaterial* material) final {
		return _buildingMeshesList.ShowDecal(area, material);
	}
	UDecalComponent* ShowDecal(TileArea area, UMaterial* material, TArray<UDecalComponent*>& decals, int32& decalCount, bool useMaterialInstance) final 	{
		return PunUnrealUtils::ShowDecal(area.trueCenterAtom(), area.size(), decals, decalCount, componentToAttach(), material, this, useMaterialInstance);
	}

	void ShowDeliveryArrow(FVector start, FVector end, bool isYellow = false, bool isSolid = false) final {
		_buildingMeshesList.ShowDeliveryArrow(start, end, isYellow, isSolid);
	}


	

	bool IsInSampleRange(WorldTile2 tile) final
	{
		if (!tile.isValid()) {
			return true;
		}
		
		WorldTile2 cameraTile = _cameraAtom.worldTile2();
		int32 tileRadius = GetTileRadius(_zoomDistance);
		return WorldTile2::Distance(cameraTile, tile) < tileRadius * 3 / 2;
	}


	/**
	 * IGameManagerInterface
	 */

	int32 playerCount() final {
		return gameInstance()->playerCount();
	}

	FString playerNameF(int32 playerId) final {
		return gameInstance()->playerNameF(playerId);
	}
	const TArray<FString>& playerNamesF() {
		return gameInstance()->playerNamesF();
	}

	std::vector<int32> allHumanPlayerIds() final {
		std::vector<int32> results = gameInstance()->allHumanPlayerIds();
		
		// Special case: Replay
		auto& replayPlayers = _simulation->replaySystem().replayPlayers;
		for (int32 i = 0; i < replayPlayers.size(); i++) {
			if (replayPlayers[i].isInitialize()) {
				CppUtils::TryAdd(results, i); // TryAdd because replay might be started on human, to edit the replay
			}
		}
		
		return results;
	}
	std::vector<int32> connectedPlayerIds(bool withReplayPlayers) final {
		std::vector<int32> results = gameInstance()->connectedPlayerIds();

		// Special case: Replay
		if (withReplayPlayers)
		{
			auto& replayPlayers = _simulation->replaySystem().replayPlayers;
			for (int32 i = 0; i < replayPlayers.size(); i++) {
				if (replayPlayers[i].isInitialize()) {
					CppUtils::TryAdd(results, i);
				}
			}
		}

		return results;
	}
	std::vector<int32> disconnectedPlayerIds() final {
		return gameInstance()->disconnectedPlayerIds();
	}

	bool isSinglePlayer() final {
		return CastChecked<UPunGameInstance>(GetGameInstance())->isSinglePlayer;
	}
	AutosaveEnum autosaveEnum() final {
		return gameInstance()->autosaveEnum;
	}
	

	// TODO: IsPlayerBuildable, IsPlayerFrontBuildable are relics that should go to simCore
	//bool IsPlayerBuildable(WorldTile2 tile) const final
	//{
	//	if (!GameMap::IsInGrid(tile)) return false;
	//	if (_simulation->tileOwnerPlayer(tile) != _playerId) return false;
	//	
	//	return _simulation->IsBuildable(tile) || _simulation->IsCritterBuildingIncludeFronts(tile);
	//}

	bool IsPlayerFrontBuildable(WorldTile2 tile) const final
	{
		if (!GameMap::IsInGrid(tile)) return false;
		if (_simulation->tileOwnerPlayer(tile) != _playerId) return false;

		return _simulation->IsFrontBuildable(tile) || _simulation->IsCritterBuilding(tile);
	}

	bool IsPlayerColonyBuildable(WorldTile2 tile) const {
		if (!GameMap::IsInGrid(tile)) return false;
		if (_simulation->tileOwnerTown(tile) != -1) return false; // Colony must be built on empty territory

		return _simulation->IsBuildable(tile) || _simulation->IsCritterBuildingIncludeFronts(tile);
	}
	bool IsPlayerColonyFrontBuildable(WorldTile2 tile) const {
		if (!GameMap::IsInGrid(tile)) return false;
		if (_simulation->tileOwnerTown(tile) != -1) return false;

		return _simulation->IsFrontBuildable(tile) || _simulation->IsCritterBuilding(tile);
	}

	bool IsPlayerRoadBuildable(WorldTile2 tile) const final {
		return GameMap::IsInGrid(tile) &&
				_simulation->IsFrontBuildableForPlayer(tile, _playerId) ||
				_simulation->IsCritterBuilding(tile);
	}
	bool IsIntercityRoadBuildable(WorldTile2 tile) const {
		return GameMap::IsInGrid(tile) &&
				_simulation->IsFrontBuildable(tile) ||
				_simulation->IsCritterBuilding(tile);
	}

	bool IsPlayerTunnelBuildable(WorldTile2 tile) const final {
		if (!GameMap::IsInGrid(tile)) {
			return false;
		}
		int32 tileOwnerPlayer = _simulation->tileOwnerPlayer(tile);
		if (tileOwnerPlayer != -1 && tileOwnerPlayer != _playerId) {
			return false;
		}
		return  _simulation->IsFrontBuildable(tile) ||
				_simulation->IsCritterBuilding(tile);
	}

	int32 GetMinEraDisplay(CardEnum buildingEnum) const final {
		return _assetLoader->GetMinEraDisplay(buildingEnum);
	}
	

	//bool IsGeobuildable(int32_t x, int32_t y) const final {
	//	return GameMap::IsInGrid(x, y) && GameMap::IsGeoResource(WorldTile2(x, y));
	//}

	GeoresourceNode RegionGeoresource(WorldRegion2 region) final {
		return _simulation->georesourceSystem().georesourceNode(region.regionId());
	}

	void ResizeDisplaySystemBuildingIds(int newSize) final {
#if DISPLAY_BUILDING
		_buildingDisplaySystem->ResizeObjectIds(newSize);
#endif
	}

	/*
	 * IGameSoundInterface
	 */
	void SpawnResourceDropoffAudio(ResourceEnum resourceEnum, WorldAtom2 worldAtom) override {
#if AUDIO_ALL
		SCOPE_CYCLE_COUNTER(STAT_PunSoundDropoff);
		if (IsInSampleRange(worldAtom.worldTile2())) {
			_soundSystem->SpawnResourceDropoffAudio(resourceEnum, worldAtom);
		}
#endif
	}
	void SpawnAnimalSound(UnitEnum unitEnum, bool isAngry, WorldAtom2 worldAtom, bool usePlayProbability) override {
#if AUDIO_ALL
		SCOPE_CYCLE_COUNTER(STAT_PunSoundAnimal);
		if (IsInSampleRange(worldAtom.worldTile2())) {
			_soundSystem->SpawnAnimalSound(unitEnum, isAngry, worldAtom, usePlayProbability);
		}
#endif
	}
	void Spawn3DSound(std::string groupName, std::string soundName, WorldAtom2 worldAtom, float height) override {
#if AUDIO_ALL
		SCOPE_CYCLE_COUNTER(STAT_PunSound3D);
		if (IsInSampleRange(worldAtom.worldTile2())) {
			_soundSystem->Spawn3DSound(groupName, soundName, worldAtom, height);
		}
#endif
	}
	void Spawn2DSound(std::string groupName, std::string soundName, int32 playerId, WorldTile2 tile) override {
		if (playerId == -1) {
			return;
		}
		if (playerId != _playerId) {
			return;
		}
		if (IsInSampleRange(tile)) {
			Spawn2DSound_Helper(groupName, soundName);
		}
	}
	void Spawn2DSound(std::string groupName, std::string soundName) override {
		Spawn2DSound_Helper(groupName, soundName);
	}
	void Spawn2DSoundAllPlayers(std::string groupName, std::string soundName, WorldTile2 tile) override {
		if (IsInSampleRange(tile)) {
			Spawn2DSound_Helper(groupName, soundName);
		}
	}

	void Spawn2DSound_Helper(const std::string& groupName, const std::string& soundName) {
#if AUDIO_ALL
		SCOPE_CYCLE_COUNTER(STAT_PunSound2D);
		if (_soundSystem) {
			_soundSystem->Spawn2DSound(groupName, soundName);
		}
#endif
	}
	
	//void SpawnBuildingWorkSound(Building& building) override
	//{
	//	if (IsInSampleRange(building.centerTile())) {
	//		_soundSystem->SpawnBuildingWorkSound(building.buildingEnum(), building.buildingId(), building.centerTile().worldAtom2());
	//	}
	//}
	void TryStartBuildingWorkSound(Building& building) override
	{
#if AUDIO_ALL
		if (IsInSampleRange(building.centerTile())) {
			_soundSystem->TryStartBuildingWorkSound(building.buildingEnum(), building.buildingId(), building.centerTile().worldAtom2());
		}
#endif
	}
	void TryStopBuildingWorkSound(Building& building) override
	{
#if AUDIO_ALL
		if (IsInSampleRange(building.centerTile())) {
			_soundSystem->TryStopBuildingWorkSound(building.buildingEnum(), building.buildingId(), building.centerTile().worldAtom2());
		}
#endif
	}

	float GetDisplayTime() override {
		return UGameplayStatics::GetTimeSeconds(this);
	}
	float GetTrailerTime() override {
		return networkInterface()->GetTrailerTime();
	}
	
	/*
	 * 
	 */

	// Overlay setting conflict resolution...
	void SetOverlayType(OverlayType overlayType, OverlaySetterType setterType) final {
		_overlaySetterTypeToType[static_cast<int>(setterType)] = overlayType;
	}
	void SetOverlayTile(WorldTile2 overlayCenterTile) final { _overlayCenterTile = overlayCenterTile; }
	WorldTile2 GetOverlayTile() final { return _overlayCenterTile; }
	OverlayType GetOverlayType() final {
		for (int i = 0; i < _overlaySetterTypeToType.size(); i++) {
			if (_overlaySetterTypeToType[i] != OverlayType::None) {
				return _overlaySetterTypeToType[i];
			}
		}
		return OverlayType::None;
	}

	bool isHidingTree() final {
		if (_isHidingTrees) {
			return true;
		}
		// Note: Add here, don't forget to add SetNeedDisplay to BuildingPlacementSystem too
		PlacementType placementType = networkInterface()->placementType();
		if (IsRoadPlacement(placementType)) {
			return true;
		}
		if (placementType == PlacementType::Building ||
			placementType == PlacementType::BuildingDrag) 
		{
			CardEnum buildingEnum = networkInterface()->placementBuildingEnum();
			if (IsPlacementHidingTree(buildingEnum)) {
				return true;
			}
		}
		return false;
	}
	void SetOverlayHideTree(bool isHiding) final {
		_isHidingTrees = isHiding;
		_simulation->SetNeedDisplayUpdate(DisplayClusterEnum::Trees, _sampleRegionIds);
	}
	void ToggleOverlayHideTree() final {
		_isHidingTrees = !_isHidingTrees;
		_simulation->SetNeedDisplayUpdate(DisplayClusterEnum::Trees, _sampleRegionIds);
	}

	bool isShowingProvinceOverlay() final { return _showProvinceOverlay; }
	void SetOverlayProvince(bool showProvinceOverlay) final {
		_showProvinceOverlay = showProvinceOverlay;
	}
	void ToggleOverlayProvince() final {
		_showProvinceOverlay = !_showProvinceOverlay;
	}
	

	bool isCtrlDown() final { return _isCtrlDown; }
	bool isShiftDown() final {
		const FModifierKeysState modifierKeys = FSlateApplication::Get().GetModifierKeys();
		return modifierKeys.IsShiftDown();
		//return _isShiftDown;
	}
	void SetCtrl(bool isDown) final { _isCtrlDown = isDown; }
	
	void SetShift(bool isDown) final {
		_LOG(PunInput, "SetShift %d", isDown);
		_isShiftDown = isDown;
	}

	bool alwaysShowProvinceHover() final { return _alwaysShowProvinceHover; }
	void SetAlwaysShowProvinceHover(bool alwaysShowProvinceHoverIn) {
		_alwaysShowProvinceHover = alwaysShowProvinceHoverIn;
	}

	ResourceEnum tipResourceEnum() final { return _tipResourceEnum; }
	void SetTipResourceEnum(ResourceEnum resourceEnum) final {
		_tipResourceEnum = resourceEnum;
	}


	void DrawLine(WorldAtom2 startAtom, FVector startShift, WorldAtom2 endAtom, FVector endShift, FLinearColor color, 
					float Thickness = 1.0f, float LifeTime = 10000) override 
	{
		if (PunSettings::IsOn("SimLine"))
		{
			FVector start = MapUtil::DisplayLocation(_cameraAtom, startAtom) + startShift;
			FVector end = MapUtil::DisplayLocation(_cameraAtom, endAtom) + endShift;
			if (auto batch = lineBatch()) {
				PUN_LOG("DrawLine GameManager %s %s", *FString(startAtom.ToString().c_str()), *FString(endAtom.ToString().c_str()))
						batch->DrawLine(start, end, color, 100.0f, Thickness, LifeTime);
			}
		}
	}

	float GetDisplayWorldTime() final {
		return UGameplayStatics::GetTimeSeconds(this);
	}

	FMapSettings GetMapSettings() final {
		return gameInstance()->GetMapSettings();
	}
	bool HasSavedMap(const FMapSettings& mapSettings) final {
		return gameInstance()->HasSavedMapWithSettings(mapSettings);
	}
	

	TArray<FString> GetReplayFileNames() final {
		return gameInstance()->replayFilesToLoad;
	}

	/**
	 * IDisplaySystemDataSource
	 */
	UAssetLoaderComponent* assetLoader() final { return _assetLoader; }
	
	AActor* actorToAttach() final { return Cast<AActor>(this);  }
	USceneComponent* componentToAttach() final { return _root; }

	ULineBatchComponent* lineBatch() final {
		if (UWorld* world = GetWorld()) {
			return world->LineBatcher;
		}
		return nullptr;
	}

	const GameDisplayInfo& displayInfo() final { return _displayInfo; }

	float zoomDistance() final { return _zoomDistance; }
	float smoothZoomDistance() final { return _smoothZoomDistance; }

	bool ZoomDistanceBelow(float threshold) final { return _zoomDistance < threshold; }
	bool SmoothZoomDistanceBelow(float threshold) { return _smoothZoomDistance < threshold; }
	
	FVector cameraLocation() final { return _cameraLocation; }

	int timeSinceTickStart() final {
		return std::chrono::duration_cast<std::chrono::microseconds>(std::chrono::high_resolution_clock::now() - _tickStartTime).count();
	}
	
	WorldAtom2 cameraAtom() final { return _cameraAtom; }
	TileArea sampleArea() final { return _sampleArea;  }

	const std::vector<int32>& sampleProvinceIds() final {
		return _sampleProvinceIds;
	}
	

	float GetTerrainDisplayHeight(WorldTile2 tile) final {
#if !DISPLAY_TERRAIN
		return 0.0f;
#endif
		return _regionDisplaySystem->GetTerrainDisplayHeight(tile);
	}

	bool isPhotoMode() final { return _isPhotoMode; }
};
