#include "GameSimulationCore.h"
#include "UnitStateAI.h"

#include "PunCity/GameRand.h"
#include "UnrealEngine.h"
#include "Buildings/TownHall.h"
#include "Buildings/GathererHut.h"
#include "Buildings/House.h"
#include <chrono>
#include <functional>
#include "Buildings/StorageYard.h"

using namespace std;
using namespace std::chrono;


DEFINE_LOG_CATEGORY(LogNetworkInput);

DECLARE_CYCLE_STAT(TEXT("PUN: [Sim]Total"), STAT_PunSimTotal, STATGROUP_Game);

#define LOCTEXT_NAMESPACE "GameSimulationCore"

void GameSimulationCore::Init(IGameManagerInterface* gameManager, IGameSoundInterface* soundInterface, IGameUIInterface* uiInterface, bool isLoadingFromFile)
{
	PUN_LLM_ONLY(PunScopeLLM::InitPunLLM());

	PUN_LLM(PunSimLLMTag::Simulation);
	
	_CrtSetDbgFlag(_CRTDBG_ALLOC_MEM_DF | _CRTDBG_LEAK_CHECK_DF);

	GameRand::SetRandUsageValid(true);
	
	_gameManager = gameManager;
	_soundInterface = soundInterface;
	_uiInterface = uiInterface;
	
	_tickCount = 0;
	//_playerCount = 0;
	
	GameRand::ResetStateToTickCount(_tickCount);
	Time::ResetTicks();

	StaticData::ResetFoodEnums();

	_isLoadingFromFile = isLoadingFromFile;

	{
		PUN_LLM(PunSimLLMTag::PathAI);
		//_pathAIHuman = make_unique<PunAStar128x256>();
		_pathAI = make_unique<PunAStar128x256>();
	}

	_mapSettings = gameManager->GetMapSettings();
	//std::string mapSeed = ToStdString(_mapSettings.mapSeed);

	// Terrain Gen
	{
		PUN_LLM(PunSimLLMTag::Terrain);

		_terrainGenerator = make_unique<PunTerrainGenerator>();
		_terrainGenerator->Init(this, GameMapConstants::TilesPerWorldX, GameMapConstants::TilesPerWorldY, 0, mapSizeEnum(), gameManager->lineBatch());

		// TODO: check mapSeed to see if there is a cached map...

		if (!isLoadingFromFile)
		{
#define FORCE_GENERATION 0

#if FORCE_GENERATION
			_terrainGenerator->CalculateWorldTerrain(_mapSettings);
			_terrainGenerator->SetGameMap();
#else
			if (!gameManager->HasSavedMap(_mapSettings)) {
				INIT_LOG("CalculateWorldTerrain %s", ToTChar(_mapSettings.ToString()));
				_terrainGenerator->CalculateWorldTerrain(_mapSettings);
				
				_terrainGenerator->SetGameMap();

				//INIT_LOG("SaveTerrain");
				//_terrainGenerator->SaveOrLoad(true);
			}
			else {
				INIT_LOG("LoadTerrain");
				_terrainGenerator->SaveOrLoad(false);
				
				_terrainGenerator->SetGameMap();
			}
#endif
		}

		// Water for pathAI
		const std::vector<TerrainTileType>& terrainMap = _terrainGenerator->terrainMap();
		for (int32 yy = 0; yy < GameMapConstants::Tiles4x4PerWorldY; yy++) {
			for (int32 xx = 0; xx < GameMapConstants::Tiles4x4PerWorldX; xx++) {
				TerrainTileType tileType = terrainMap[WorldTile2(xx * 4, yy * 4).tileId()];
				PunAStar128x256::SetWater(xx, yy, tileType == TerrainTileType::Ocean || tileType == TerrainTileType::River);
			}
		}
	}

	_LOG(PunInit, "GAME_VERSION %s", *FString::FromInt(GAME_VERSION));

	_uiInterface->SetLoadingText(NSLOCTEXT("PunPlayerController", "Loading_PlantingTrees", "Planting Trees..."));

	_gameEventSystem.Init();
	_overlaySystem.Init(this); // Needs to come first for road.

	_provinceSystem.InitProvince(this);

	_unitSystem = make_unique<UnitSystem>();
	_unitSystem->Init(this);

	_buildingSystem = make_unique<BuildingSystem>();
	_buildingSystem->Init(this);

	_regionSystem = make_unique<ProvinceInfoSystem>();
	_regionSystem->InitProvinceInfoSystem(this);

	_georesourceSystem = make_unique<GeoresourceSystem>();
	_georesourceSystem->InitGeoresourceSystem(this, !isLoadingFromFile);

	{
		PUN_LLM(PunSimLLMTag::Trees);
		_treeSystem = make_unique<TreeSystem>();
		_treeSystem->Init(GameMapConstants::TilesPerWorldX, GameMapConstants::TilesPerWorldY, this);
	}

	_statSystem.Init();

	PUN_CHECK(_pathAI->walkableGridSize() > 0);

	{
		PUN_LLM(PunSimLLMTag::Flood);
		_floodSystem.Init(_pathAI.get(), _terrainGenerator.get(), this, isLoadingFromFile);
		//_floodSystemHuman.Init(_pathAI.get(), _terrainGenerator.get(), this, isLoadingFromFile); // TODO: Remove once save is ok
	}

	//_floodSystem2.Init(_pathAI.get(), isLoadingFromFile);
	//_floodSystem2Human.Init(_pathAIHuman.get(), isLoadingFromFile);

	_dropSystem.Init(this);

	_debugLineSystem.Init();

	_worldTradeSystem.Init(this, GameConstants::MaxPlayersAndAI);

	_terrainChanges.Init(this);

	_replaySystem.Init(this);


	// Just create all players' system even if there isn't that many
	// TODO: don't create all?
	//while (_resourceSystems.size() < GameConstants::MaxPlayersAndAI)
	PUN_CHECK(_resourceSystems.size() == 0);
	for (int32 i = 0; i < GameConstants::MaxPlayersAndAI; i++) 
	{
		int32 playerId = _resourceSystems.size();

		_playerOwnedManagers.push_back(PlayerOwnedManager(playerId, this));

		// Add town
		int32 townId = _resourceSystems.size();
		_resourceSystems.push_back(ResourceSystem(townId, this));
		_statSystem.AddTown(townId);
		_worldTradeSystem.AddTown(townId);
		_townManagers.push_back(make_unique<TownManager>(playerId, townId, this));
		_playerOwnedManagers[playerId].AddTownId(townId);

		
		_globalResourceSystems.push_back(GlobalResourceSystem());
		_unlockSystems.push_back(UnlockSystem(playerId, this));
		_questSystems.push_back(QuestSystem(playerId, this));
		_playerParameters.push_back(PlayerParameters(playerId, this));
		_popupSystems.push_back(PopupSystem(playerId, this));
		_cardSystem.push_back(BuildingCardSystem(playerId, this));

		

		_aiPlayerSystem.push_back(AIPlayerSystem(playerId, this, this));
		
		_eventLogSystem.AddPlayer();
		_replaySystem.AddPlayer();

		_playerIdToNonRepeatActionToAvailableTick.push_back(std::vector<int32>(static_cast<int>(NonRepeatActionEnum::Count), 0));
		_playerIdToCallOnceActionWasCalled.push_back(std::vector<int32>(CallOnceEnumCount, false));
	}

	// Only set the "InitialAIs" active
	PUN_CHECK(_mapSettings.aiCount <= GameConstants::MaxAIs);
	for (int i = 0; i < _mapSettings.aiCount; i++) {
		_aiPlayerSystem[GameConstants::MaxPlayersPossible + i].SetActive(true);
	}

	// Replay is fixed for 1 player for now
	int32 firstReplayPlayer = 1;
	TArray<FString> replayNames = gameManager->GetReplayFileNames();
	for (int32 i = 0; i < replayNames.Num(); i++) {
		_replaySystem.LoadPlayerActions(firstReplayPlayer, replayNames[i]);
	}


#if CHECK_TICKHASH
	_tickHashes.Clear();
	_serverTickHashes.Clear();
	_currentInputHashes = 0;
#endif
	
#if KEEP_ACTION_HISTORY
	_commandsExecuted.clear();
	_commandsTickExecuted.clear();
#endif

	/*
	 * Displays
	 */
	_displayEnumToRegionToNeedUpdate.resize(static_cast<int>(DisplayClusterEnum::Count));
	for (int i = 0; i < _displayEnumToRegionToNeedUpdate.size(); i++) {
		_displayEnumToRegionToNeedUpdate[i].resize(GameMapConstants::TotalRegions, true);
	}

	_displayEnumToNeedUpdate.resize(static_cast<int>(DisplayGlobalEnum::Count), true);
	_displayEnumToNeedUpdateIds.resize(static_cast<int>(DisplayGlobalEnum::Count));

	_regionToDemolishDisplayInfos.resize(GameMapConstants::TotalRegions);
	_regionToFireOnceParticleInfo.resize(GameMapConstants::TotalRegions);

	if (!isLoadingFromFile) {
		InitProvinceBuildings();
	}
	
	
	_descriptionUIState = DescriptionUIState();

	//PUN_LOG("Done Sim Init %d", _tickCount);

	// Snow
	_snowAccumulation3 = 0;
	_snowAccumulation2 = 0;
	_snowAccumulation1 = 0;
	_lastTickSnowAccumulation3 = 0;
	_lastTickSnowAccumulation2 = 0;
	_lastTickSnowAccumulation1 = 0;

	// Integrity check
	TileObjInfosIntegrityCheck();

	check(static_cast<int32>(TryWorkFailEnum::Count) == TryWorkFailEnumName.Num());
	

	GameRand::SetRandUsageValid(false);
}

void GameSimulationCore::InitProvinceBuildings()
{
	std::vector<CardEnum> provinceBuildings(GameMapConstants::TotalRegions, CardEnum::None);

#if TRAILER_MODE
	return;
#endif
	
	auto& provinceSys = provinceSystem();

	/*
	 * Process:
	 * - Find possible slots first
	 * - Distribute from fixed count
	 */

	ProvinceInfoSystem& provinceInfoSys = provinceInfoSystem();

	const int32 targetMinorPortCityCount = 10;
	const int32 targetMinorCityCount = 20;

	std::vector<int32> portSlotProvinceIds = provinceInfoSys.portSlotProvinceIds();

	// Fill Port first
	const int32 skipCount = 20; // Skip to try to distribute evenly
	int32 minorPortCityCount = 0;
	int32 minorCityCount = 0;

	auto createBuilding = [&](int32 townId, CardEnum buildingEnum, WorldTile2 centerTile, Direction faceDirection, int32 radiusToClearTrees)
	{
		// Create building
		FPlaceBuilding parameters;
		parameters.buildingEnum = static_cast<uint8>(buildingEnum);
		parameters.faceDirection = static_cast<uint8>(faceDirection);
		parameters.center = centerTile;
		parameters.area = BuildingArea(parameters.center, GetBuildingInfo(buildingEnum).size, faceDirection);
		parameters.playerId = -1;
		parameters.townId = townId;

		int32 buildingId = PlaceBuilding(parameters);
		if (buildingId != -1)
		{
			Building& bld = building(buildingId);

			bld.TryInstantFinishConstruction();

			// Clear the trees
			TileArea(parameters.center, radiusToClearTrees).ExecuteOnArea_WorldTile2([&](WorldTile2 tile)
			{
				if (WorldTile2::Distance(parameters.center, tile) <= radiusToClearTrees) {
					_treeSystem->ForceRemoveTileObj(tile, false);
				}
			});

			return buildingId;
		}
		
		return -1;
	};

	auto createMinorCity = [&](int32 provinceId)
	{
		check(provinceSys.IsProvinceValid(provinceId));
		const ProvinceBuildingSlot& slot = provinceInfoSys.provinceBuildingSlot(provinceId);
		check(slot.landSlot.isValid());

		int32 townId = AddMinorTown(provinceId);
		provinceInfoSys.SetSlotTownId(provinceId, townId);
		SetProvinceOwner(provinceId, townId, true);

		int32 minorCityBuildingId = createBuilding(townId, CardEnum::MinorCity, slot.landSlot, slot.landSlotFaceDirection, 7);
		if (minorCityBuildingId != -1) {
			townManagerBase(townId)->townhallId = minorCityBuildingId;
		}
		else {
			// Failed to create building, remove Town
			RemoveMinorTown(townId);
			provinceInfoSys.SetSlotTownId(provinceId, -1);
			SetProvinceOwner(provinceId, -1, true);
		}
		
		return minorCityBuildingId;
	};

	for (int32 i = 0; i < skipCount; i++) {
		for (int32 j = i; j < portSlotProvinceIds.size(); j += skipCount)
		{
			check(j < portSlotProvinceIds.size());
			int32 provinceId = portSlotProvinceIds[j];

			// Create Minor City
			int32 minorCityBuildingId = createMinorCity(provinceId);

			PUN_LOG("portSlotProvinceIds provinceId:%d minorCityBuildingId:%d", provinceId, minorCityBuildingId);

			if (minorCityBuildingId != -1)
			{
				const ProvinceBuildingSlot& slot = provinceInfoSys.provinceBuildingSlot(provinceId);

				// Create Port
				int32 minorCityPortId = createBuilding(slot.townId, CardEnum::MinorCityPort, slot.portSlot, slot.portSlotFaceDirection, 7);

				MinorCity& minorCity = building<MinorCity>(minorCityBuildingId);
				MinorCityChild& minorCityPort = building<MinorCityChild>(minorCityPortId);
				
				townManagerBase(minorCity.townId())->AddChildBuilding(minorCityPort);

				PUN_LOG("portSlotProvinceIds2 provinceId:%d minorCityPortId:%d", provinceId, minorCityPortId);

				minorPortCityCount++;
				minorCityCount++;
				if (minorPortCityCount >= targetMinorPortCityCount) {
					goto endPortLoop;
				}
			}
		}
	}
	endPortLoop:

	const std::vector<int32>& landSlotProvinceIds = provinceInfoSys.landSlotProvinceIds();

	for (int32 i = 0; i < skipCount; i++) {
		for (int32 j = i; j < landSlotProvinceIds.size(); j += skipCount)
		{
			check(j < landSlotProvinceIds.size());
			int32 minorCityBuildingId = createMinorCity(landSlotProvinceIds[j]);

			PUN_LOG("landSlotProvinceIds provinceId:%d minorCityBuildingId:%d", landSlotProvinceIds[j], minorCityBuildingId);

			minorCityCount++;
			if (minorCityCount >= targetMinorCityCount) {
				goto endLandLoop;
			}
		}
	}
	endLandLoop:

	PUN_LOG("InitProvinceBuildings port:%d/%d all:%d/%d", minorPortCityCount, portSlotProvinceIds.size(), minorCityCount, landSlotProvinceIds.size());

	return;
	
	for (int y = 0; y < GameMapConstants::RegionsPerWorldY; y++) {
		for (int x = 0; x < GameMapConstants::RegionsPerWorldX; x++)
		{
			if (GameRand::Rand() % 3 != 0) {
				continue;
			}
			
			WorldRegion2 region(x, y);
			int32 provinceId = region.regionId();

			WorldTile2 provinceCenter = provinceSys.GetProvinceCenterTile(provinceId);
			if (!provinceCenter.isValid()) {
				continue;
			}

			BiomeEnum biomeEnum = terrainGenerator().GetBiome(provinceCenter);

			auto hasNearbyBuilding = [&](CardEnum cardEnum) {
				bool hasNearby = false;
				// TODO: faster if check only 4 regions before loop reach this
				region.ExecuteOnNearbyRegions([&](WorldRegion2 curRegion) {
					if (curRegion.IsValid() && provinceBuildings[curRegion.regionId()] == cardEnum) {
						hasNearby = true;
					}
				});
				return hasNearby;
			};

			/*
			 * Province Buildings
			 */
			CardEnum buildingEnum = CardEnum::None;

			if (GameRand::Rand() % 3 == 0 && !hasNearbyBuilding(CardEnum::MinorCity)) {
				buildingEnum = CardEnum::MinorCity;
			}
			else if (GameRand::Rand() % 10 == 0 && !hasNearbyBuilding(CardEnum::RegionShrine)) {
				buildingEnum = CardEnum::RegionShrine;
			}
			else if (biomeEnum == BiomeEnum::Desert)
			{
				if (GameRand::Rand() % 5 == 0 && !hasNearbyBuilding(CardEnum::RegionCrates)) {
					buildingEnum = CardEnum::RegionCrates;
				}
				else if (GameRand::Rand() % 25 == 0 && !hasNearbyBuilding(CardEnum::RegionTribalVillage)) {
					buildingEnum = CardEnum::RegionTribalVillage;
				}
			}
			else
			{
				if (GameRand::Rand() % 3 == 0 && !hasNearbyBuilding(CardEnum::RegionCrates)) {
					buildingEnum = CardEnum::RegionCrates;
				}
				else if (GameRand::Rand() % 3 == 0 && !hasNearbyBuilding(CardEnum::RegionTribalVillage)) {
					buildingEnum = CardEnum::RegionTribalVillage;
				}
			}

			if (buildingEnum == CardEnum::None) {
				continue;
			}

			// Random X times in a region trying to find a suitable spot
			const int32 tries = 1;
			const int32 rangeFromCenter = 7;
			const int32 randSize = rangeFromCenter * 2 + 1;

			for (int32 i = 0; i < tries; i++)
			{
				Direction faceDirection = Direction::S;
				//WorldTile2 centerTile(provinceCenter.x + (GameRand::Rand() % randSize) - rangeFromCenter,
				//						provinceCenter.y + (GameRand::Rand() % randSize) - rangeFromCenter); // current unit's tile is gate tile.

				WorldTile2 centerTile = provinceCenter;
				TileArea area = BuildingArea(centerTile, GetBuildingInfo(buildingEnum).size, faceDirection);

				bool canPlace = true;
				if (area.isInMap()) {
					area.ExecuteOnArea_WorldTile2([&](WorldTile2 areaTile) {
						if (!IsBuildable(areaTile)) canPlace = false;
					});
				}
				else {
					canPlace = false;
				}

				// Front Grid
				TileArea frontArea = area.GetFrontArea(faceDirection);
				if (frontArea.isInMap()) {
					frontArea.ExecuteOnArea_WorldTile2([&](WorldTile2 areaTile) {
						if (!IsBuildable(areaTile)) canPlace = false;
					});
				}
				else {
					canPlace = false;
				}

				if (canPlace)
				{
					FPlaceBuilding parameters;
					parameters.buildingEnum = static_cast<uint8>(buildingEnum);
					parameters.faceDirection = static_cast<uint8>(faceDirection);
					parameters.area = area;
					parameters.center = centerTile;
					parameters.playerId = -1;

					int32 buildingId = PlaceBuilding(parameters);
					Building& bld = building(buildingId);
					
					bld.TryInstantFinishConstruction();

					// Clear the trees
					const int32 radiusToClearTrees = 7;
					TileArea(centerTile, radiusToClearTrees).ExecuteOnArea_WorldTile2([&](WorldTile2 tile)
					{
						if (WorldTile2::Distance(centerTile, tile) <= radiusToClearTrees) {
							_treeSystem->ForceRemoveTileObj(tile, false);
						}
					});

					provinceBuildings[region.regionId()] = buildingEnum;

					break;
				}
			}

		}
	}
}

/*
 * Tick
 */
void GameSimulationCore::Tick(int bufferCount, NetworkTickInfo& tickInfo, bool tickOnce)
{
	LEAN_PROFILING_D(TickSim);
	PUN_LLM(PunSimLLMTag::Simulation);

#if PUN_LLM_ON
	if (Time::Ticks() > Time::TicksPerSecond * 5 && Time::Ticks() % Time::TicksPerMinute == 0) {
		PunScopeLLM::Print();
	}
#endif
	
	SCOPE_CYCLE_COUNTER(STAT_PunSimTotal);
	//_LOG(PunTick, "GameSimulationCore::Tick tickCount:%d tickCountSim:%d", tickInfo.tickCount, tickInfo.tickCountSim);

	if (GEngine &&  tickInfo.commands.size() > 0) {
		GEngine->AddOnScreenDebugMessage(-1, 10.0f, FColor::White, 
										FString::Printf(TEXT("Tick:%d, buffer:%d, commands:%zu"), _tickCount, bufferCount, tickInfo.commands.size()));
	}


	//PUN_LOG("TickSim UnitSystem unitCount():%d _unitLeans:%d", unitSystem().unitCount(), unitCount() - unitSystem().deadCount());

	

	CheckIntegrity();

	// Issue Commands
	std::vector<shared_ptr<FNetworkCommand>> commands = tickInfo.commands;

	if (tickOnce) { // Skip commands precessing for StepSimulation
		commands.clear();
	}
	

	vector<bool> commandSuccess(commands.size(), true);

	if (commands.size() > 0) {
		_LOG(LogNetworkInput, "!!!ExecuteNetworkCommands  TickCount:%d commands:%d", Time::Ticks(), commands.size());
	}

	for (size_t i = 0; i < commands.size(); i++) 
	{
		PUN_CHECK(IsValidPlayer(commands[i]->playerId));

		// Player not initialized, only allow choosing location
		// This check prevent the case where commands arrived after abandoning town (such as commands from leftover popups etc.)
		if (!HasChosenLocation(commands[i]->playerId))
		{
			if (commands[i]->commandType() != NetworkCommandEnum::ChooseLocation &&
				commands[i]->commandType() != NetworkCommandEnum::AddPlayer)
			{
				continue;
			}
		}

		tempVariable = i;
		commandSuccess[i] = ExecuteNetworkCommand(commands[i]);


		// Special case: Townhall after refresh and put in animation
		if (commands[i]->commandType() == NetworkCommandEnum::PlaceBuilding)
		{
			auto placeCommand = static_pointer_cast<FPlaceBuilding>(commands[i]);
			if (placeCommand->buildingEnum == static_cast<int>(CardEnum::Townhall))
			{
				_gameManager->RefreshMapAnnotation();

				// Ensure Map was refreshed (In case AI built city at tick=0)
				RefreshHeightForestColorTexture(TileArea(placeCommand->center, 12), true);
				RefreshHeightForestRoadTexture();

			}
		}

	}

	if (commands.size() > 0) {
		_LOG(LogNetworkInput, "!!!ExecuteNetworkCommands End----");
	}

	// Server tick is not the same as SimulationTick
	//   Therefore we need to set SimulationTick before saving it into replay.
	tickInfo.tickCountSim = _tickCount;


	/*
	 * Replays
	 */
	// Remove unsuccessful commands
	for (size_t i = commands.size(); i-- > 0;) {
		if (!commandSuccess[i]) {
			commands.erase(commands.begin() + i);
		}
	}
	
	// Record tickInfo
	_replaySystem.AddTrailerCommands(commands);
	_replaySystem.AddNetworkTickInfo(tickInfo, commands);

	// AutoSave Replay
	if (_tickCount % Time::TicksPerSeason == 0) {
		// TODO: for now hard code player action saving (Save client1)
		_replaySystem.SavePlayerActions(0, "ReplaySave0");
	}



	/*
	 * Tick simulation 
	 */

	// Note: _tickCount contains simulation local tick as oppose to server's tick

	int32 gameSpeed = tickInfo.gameSpeed;
	if (!AllPlayerHasTownhallAfterInitialTicks()) {
		gameSpeed = 0; // Pause while players are still choosing location.
	}

	if (tickOnce) {
		gameSpeed = 1;
	}
	
	_lastGameSpeed = gameSpeed;

	// Special -12 game speed which means 1/2 discard a tick
	int32 simTicksForThisControllerTick = gameSpeed;
	if (gameSpeed == -12) {
		simTicksForThisControllerTick = tickInfo.proxyControllerTick % 2;
	}

	for (size_t localTickIndex = 0; localTickIndex < simTicksForThisControllerTick; localTickIndex++)
	{
		//_LOG(PunTick, "[%d] TickCore %d", _gameManager->playerId(), _tickCount);

		GameRand::ResetStateToTickCount(_tickCount);
		Time::SetTickCount(_tickCount);

		_floodSystem.Tick();


		if (Time::Ticks() % 60 == 0)
		{
			//if (isConnectedNotCached || isConnectedCached) {
			//	PUN_LOG("isConnected notCached:%d cached:%d", isConnectedNotCached, isConnectedCached);
			//}
			//isConnectedNotCached = 0;
			//isConnectedCached = 0;
		}
		

		if (PunSettings::IsOn("TickStats")) {
			_statSystem.Tick(this);
		}

		if (PunSettings::IsOn("TickPlayerOwned"))
		{
			//! Tick Round
			if (Time::Ticks() % Time::TicksPerRound == 0) 
			{
				// AutoTrade
				RefreshAutoTradeAmount();
				RefreshAutoTradeFulfillment();
				
				ExecuteOnPlayersAndAI([&] (int32 playerId) 
				{
					if (_playerOwnedManagers[playerId].hasChosenLocation()) 
					{
						//! Tick Round Town
						const std::vector<int32>& townIds = _playerOwnedManagers[playerId].townIds();
						for (int32 townId : townIds) {
							_townManagers[townId]->TickRound();
						}
						
						_cardSystem[playerId].TickRound();
						_playerOwnedManagers[playerId].TickRound();

						_popupSystems[playerId].TickRound();


						// Autosave
						if (Time::Seconds() > 0)
						{
							AutosaveEnum autosaveEnum = _gameManager->autosaveEnum();
							if (autosaveEnum == AutosaveEnum::HalfYear && Time::Ticks() % (Time::TicksPerYear / 2) == 0) {
								_uiInterface->AutosaveGame();
							}
							else if (autosaveEnum == AutosaveEnum::Year && Time::Ticks() % Time::TicksPerYear == 0) {
								_uiInterface->AutosaveGame();
							}
							else if (autosaveEnum == AutosaveEnum::TwoYears && Time::Ticks() % (Time::TicksPerYear * 2) == 0) {
								_uiInterface->AutosaveGame();
							}
						}
					}
				});


			}

			// Event ticks
			if (Time::IsAutumnStart() && Time::Years() > 0) 
			{
				// Randomize an event depending on the status of the town...

			}

			// Check for events every 60 sec
			if (_tickCount % (Time::TicksPerSecond * 60) == 0)
			{
				// Event log check
				ExecuteOnConnectedPlayers([&](int32 playerId)
				{
					if (_playerOwnedManagers[playerId].hasCapitalTownhall())
					{
						if (isStorageAllFull(playerId)) 
						{
							_eventLogSystem.AddEventLog(playerId, 
								LOCTEXT("NeedStorage_Event", "Need more storage space."),
								true
							);

							_soundInterface->Spawn2DSound("UI", "NeedStorageBell", playerId);

							// Trigger storage quest if not yet done...
							TryAddQuest(playerId, std::make_shared<BuildStorageQuest>());
						}
					}
				});
			}

			/*
			 * Tick 1 sec
			 */
			if (_tickCount % Time::TicksPerSecond == 0) 
			{
				ExecuteOnPlayersAndAI([&](int32 playerId)
				{
					if (_playerOwnedManagers[playerId].hasCapitalTownhall()) 
					{
						const std::vector<int32>& townIds = _playerOwnedManagers[playerId].townIds();
						for (int32 townId : townIds) {
							_townManagers[townId]->Tick1Sec();
						}
						
						//_playerOwnedManagers[playerId].RefreshHousing();
						_playerOwnedManagers[playerId].Tick1Sec();

						// Reset jobs once iin a while to bring citizens back to work if needed
						if (Time::Seconds() % 20 == 0) {
							for (int32 townId : townIds) {
								_townManagers[townId]->RefreshJobDelayed();
							}
						}
						

						/*
						 * Special
						 */
						auto unlockSys = unlockSystem(playerId);
						if (!unlockSys->didFirstTimeMedicineLowPopup && GetResourceCount(playerId, MedicineEnums) < 20) {
							unlockSys->didFirstTimeMedicineLowPopup = true;
							AddPopup(playerId, {
								LOCTEXT("MedicineFirstWarn1_Pop", "Your Medicine/Medicinal Herb count is low.<space>If you run out of both Medicine and Medicinal Herb, sickness will spread killing your citizens.<space>"),
								LOCTEXT("MedicineFirstWarn2_Pop", "To produce medicinal herb:<bullet>Unlock Medicinal Herb Seeds Technology</><bullet>Build Farms, and change its Workmode to Medicinal Herb</><space>"),
								LOCTEXT("MedicineFirstWarn3_Pop", "Alternatively, you can also import Medicine/Medicinal Herb from Trading Post/Port/Company.")
							});
						}
						if (!unlockSys->didFirstTimeToolsLowPopup && GetResourceCount(playerId, ToolsEnums) < 10) {
							unlockSys->didFirstTimeToolsLowPopup = true;
							AddPopup(playerId, {
								LOCTEXT("ToolsFirstWarn1_Pop", "Your Tools count is low.<space>If you run out of Tools, your citizens' work efficiency will drop.<space>"),
								LOCTEXT("ToolsFirstWarn2_Pop", "The easiest way to acquire Tools is by importing Steel Tools from Trading Post/Port/Company.<space>"),
								LOCTEXT("ToolsFirstWarn3_Pop", "Steel Tools are produced from Blacksmith requiring Iron Bars and Wood.")
							});
						}

						if (!unlockSys->didFirstTimeLaborer0 && _townManagers[playerId]->laborerCount() == 0) {
							unlockSys->didFirstTimeLaborer0 = true;
							AddPopup(playerId, {
								FText::Format(
									LOCTEXT("Laborer0FirstWarn1_Pop", "Your Laborer count is now 0 at {0}.<space>Every citizen is employed in a building. There is no free Laborer left to Haul and Gather Resources full-time.<space>"),
									townNameT(playerId)
								),
								LOCTEXT("Laborer0FirstWarn2_Pop", "This can cause logistics issues resulting in production slow-down or resources not being picked up.<space>"),
								LOCTEXT("Laborer0FirstWarn3_Pop", "To increase your Laborer count, either expel workers from buildings, or manually set the Laborer count from the Townhall or Employment Bureau."),
								}
							);
						}

						if (!unlockSys->didFirstTimeLowHappiness && _townManagers[playerId]->aveOverallHappiness() < HumanStateAI::minWarnHappiness()) {
							unlockSys->didFirstTimeLowHappiness = true;

							AddPopup(playerId, 
								{
									LOCTEXT("LowHappinessWarn1_Pop", "Your happiness is low.<space>Below 65% Happiness, Citizen's work efficiency will decrease.<space>"),
									LOCTEXT("LowHappinessWarn2_Pop", "Below 50% Happiness, Citizens will start leaving your City.<space>"),
									ToFText(TutorialLinkString(TutorialLinkEnum::Happiness))
								}
							);
						}

						///*
						// * ClaimProgress
						// */
						//// Conquer Province
						//std::vector<ProvinceClaimProgress> claimProgresses = _playerOwnedManagers[playerId].defendingClaimProgress();
						//for (const ProvinceClaimProgress& claimProgress : claimProgresses) 
						//{
						//	// One season to conquer in normal case
						//	//if (claimProgress.ticksElapsed > BattleClaimTicks)
						//	if (claimProgress.attackerWon())
						//	{
						//		ProvinceAttackEnum provinceAttackEnum = _playerOwnedManagers[playerId].GetProvinceAttackEnum(claimProgress.provinceId, claimProgress.attackerPlayerId);
						//		int32 provinceOwnerTownId = provinceOwnerTown_Major(claimProgress.provinceId);
						//		
						//		_playerOwnedManagers[playerId].EndConquer(claimProgress.provinceId);
						//		_playerOwnedManagers[claimProgress.attackerPlayerId].EndConquer_Attacker(claimProgress.provinceId);

						//		// Capital
						//		if (homeProvinceId(playerId) == claimProgress.provinceId)
						//		{
						//			// Home town, vassalize/declare independence instead
						//			//  For Declare Independence, the attacker is the town owner
						//			if (claimProgress.attackerPlayerId == playerId)
						//			{
						//				// Declare Independence
						//				int32 oldLordPlayerId = lordPlayerId(playerId);

						//				LoseVassalHelper(oldLordPlayerId, playerId);

						//				AddPopupAll(PopupInfo(playerId,
						//					FText::Format(LOCTEXT("IndependenceAll_Pop", "{0} declared independence from {1}."), playerNameT(playerId), playerNameT(oldLordPlayerId))
						//				), -1);
						//			}
						//			else
						//			{
						//				// Player getting vassalized lose all of its own vassals (player vassal only)
						//				const std::vector<int32>& vassalBuildingIds = _playerOwnedManagers[playerId].vassalBuildingIds();
						//				for (int32 vassalBuildingId : vassalBuildingIds) {
						//					if (building(vassalBuildingId).isEnum(CardEnum::Townhall)) {
						//						LoseVassalHelper(playerId, building(vassalBuildingId).playerId());
						//					}
						//				}

						//				// If there is an existing lord, the lord lose the vassal
						//				int32 oldLordPlayerId = _playerOwnedManagers[playerId].lordPlayerId();
						//				if (oldLordPlayerId != -1) {
						//					LoseVassalHelper(oldLordPlayerId, playerId);
						//				}

						//				// Vassalize
						//				int32 lordId = claimProgress.attackerPlayerId;
						//				_playerOwnedManagers[lordId].GainVassal(GetTownhallCapital(playerId).buildingId());
						//				_playerOwnedManagers[playerId].SetLordPlayerId(lordId);

						//				_LOG(PunNetwork, "Vassalize [sim] pid:%d lordId:%d", playerId, _playerOwnedManagers[playerId].lordPlayerId());
						//				
						//				AddPopupAll(PopupInfo(lordId, 
						//					FText::Format(LOCTEXT("XhasConqueredY", "{0} has conquered {1}."), playerNameT(lordId), playerNameT(playerId))
						//				), -1);
						//				AddPopup(playerId, 
						//					FText::Format(LOCTEXT("NewLord_Pop", "<Bold>You became {0}'s vassal.</><space><bullet>As a vassal, you pay your lord 5% <img id=\"Coin\"/> revenue as a tribute each round.</><bullet>Half of your <img id=\"Influence\"/> income goes to your lord.</><bullet>If your lord is ahead of you in science, you gain +20% <img id=\"Science\"/> from knowledge transfer.</>"),
						//						playerNameT(lordId)
						//					)
						//				);

						//				// Recalculate Tax
						//				const std::vector<int32>& lordTownIds = _playerOwnedManagers[lordId].townIds();
						//				for (int32 townId : lordTownIds) {
						//					_townManagers[townId]->RecalculateTaxDelayed();
						//				}
						//				
						//				const std::vector<int32>& playerTownIds = _playerOwnedManagers[playerId].townIds();
						//				for (int32 townId : playerTownIds) {
						//					_townManagers[townId]->RecalculateTaxDelayed();
						//				}
						//				//_playerOwnedManagers[lordId].RecalculateTaxDelayed();
						//				//_playerOwnedManagers[playerId].RecalculateTaxDelayed();

						//				// CheckDominationVictory(lordPlayerId);
						//			}
						//		}
						//		// ConquerColony
						//		else if (provinceAttackEnum == ProvinceAttackEnum::ConquerColony)
						//		{
						//			// town swap hands
						//			ChangeTownOwningPlayer(provinceOwnerTownId, claimProgress.attackerPlayerId);

						//			AddPopupAll(PopupInfo(playerId,
						//				FText::Format(LOCTEXT("ConquerColonyAll_Pop", "{0} has taken control of {1} from {2}."), playerNameT(claimProgress.attackerPlayerId), townNameT(provinceOwnerTownId), playerNameT(playerId))
						//			), -1);

						//			// If this is AI player's capital, set the AI inactive
						//			if (IsAIPlayer(playerId)) {
						//				_aiPlayerSystem[playerId].SetActive(false);

						//				AddPopupAll(PopupInfo(playerId,
						//					FText::Format(LOCTEXT("AIEliminated_Pop", "{0} is eliminated."), playerNameT(playerId))
						//				), -1);
						//			}
						//		}
						//		// ConquerProvince
						//		else
						//		{
						//			// Attacker now owns the province
						//			// Destroy any leftover building owned by player
						//			ClearProvinceBuildings(claimProgress.provinceId);

						//			SetProvinceOwner_Popup(claimProgress.provinceId, claimProgress.attackerPlayerId, false);
						//			
						//			//SetProvinceOwner(claimProgress.provinceId, attackerTownId);
						//		}
						//	}
						//	// Failed to conquer
						//	//else if (claimProgress.ticksElapsed <= 0)
						//	else if (claimProgress.attackerLost()) 
						//	{
						//		ProvinceAttackEnum provinceAttackEnum = _playerOwnedManagers[playerId].GetProvinceAttackEnum(claimProgress.provinceId, claimProgress.attackerPlayerId);
						//		int32 townId = provinceOwnerTown_Major(claimProgress.provinceId);
						//		
						//		_playerOwnedManagers[playerId].EndConquer(claimProgress.provinceId);
						//		_playerOwnedManagers[claimProgress.attackerPlayerId].EndConquer_Attacker(claimProgress.provinceId);
						//		
						//		// Note: no point in trying to pipe claimProgress here, too confusing...
						//		if (homeProvinceId(playerId) == claimProgress.provinceId)
						//		{
						//			// Home town, vassalize instead
						//			AddPopupToFront(playerId, FText::Format(LOCTEXT("DefendSuccessfulMain", "You successfully defend your independence against {0}"), playerNameT(claimProgress.attackerPlayerId)));
						//			AddPopupToFront(claimProgress.attackerPlayerId, FText::Format(LOCTEXT("VassalizeNotSuccessful", "Your attempt to vassalize {0} was not successful."), playerNameT(playerId)));
						//		}
						//		else if (provinceAttackEnum == ProvinceAttackEnum::ConquerColony)
						//		{
						//			AddPopupToFront(playerId, FText::Format(LOCTEXT("DefendSuccessful_Colony", "You successfully defend your town against {0}"), playerNameT(claimProgress.attackerPlayerId)));
						//			AddPopupToFront(claimProgress.attackerPlayerId, FText::Format(LOCTEXT("ConquerNotSuccessful_Colony", "Your attempt to gain control of {0} was not successful."), townNameT(townId)));
						//		}
						//		// ConquerProvince
						//		else {
						//			AddPopupToFront(playerId, FText::Format(LOCTEXT("DefendSuccessfulProvince", "You successfully defended your Province against {0}"), playerNameT(claimProgress.attackerPlayerId)));
						//			AddPopupToFront(claimProgress.attackerPlayerId, FText::Format(LOCTEXT("ConquerNotSuccessful", "Your attempt to conquer {0}'s Province was not successful."), playerNameT(playerId)));
						//		}
						//	}
						//}


						/*
						 * Cards
						 */
						_cardSystem[playerId].TryRefreshRareHand();


						// Spawn rare hand after 1 sec... Delay so that the ChooseRareCard sound will appear in sync when menu after systemMoveCamera
						{
							auto& townhallCapital = GetTownhallCapital(playerId);

							if (!townhallCapital.alreadyGotInitialCard && townhallCapital.townAgeTicks() >= Time::TicksPerSecond)
							{
								GenerateRareCardSelection(playerId, RareHandEnum::InitialCards1, FText());
								//GenerateRareCardSelection(playerId, RareHandEnum::InitialCards2, FText());

								townhallCapital.alreadyGotInitialCard = true;
							}
						}

						// Biome Bonus
						for (int32 townId : townIds)
						{
							if (TownHall* townhal = GetTownhallPtr(townId))
							{
								if (!townhal->alreadyGotBiomeCards1 && townhal->townAgeTicks() >= Time::TicksPerSecond) {
									townhal->alreadyGotBiomeCards1 = true;
									
									switch (GetBiomeEnum(townhal->centerTile()))
									{
									case BiomeEnum::BorealForest:
									case BiomeEnum::Tundra:
										GenerateRareCardSelection(playerId, RareHandEnum::BorealCards, FText(), townId);
										break;
									case BiomeEnum::Desert:
										GenerateRareCardSelection(playerId, RareHandEnum::DesertCards, FText(), townId);
										break;
									case BiomeEnum::Savanna:
									case BiomeEnum::GrassLand:
										GenerateRareCardSelection(playerId, RareHandEnum::SavannaCards, FText(), townId);
										break;
									case BiomeEnum::Jungle:
										GenerateRareCardSelection(playerId, RareHandEnum::JungleCards, FText(), townId);
										break;
									case BiomeEnum::Forest:
										GenerateRareCardSelection(playerId, RareHandEnum::ForestCards, FText(), townId);
										break;
									default:
										UE_DEBUG_BREAK();
										break;
									}
								}
							}
						}

						//// Check for owner change, and update vassals accordingly...
						//ArmyNode& armyNode = townhall(playerId).armyNode;
						//int32 lastLordPlayerId = armyNode.lastLordPlayerId;
						//int32 lordPlayerId = armyNode.lordPlayerId;
						//if (lastLordPlayerId != lordPlayerId)
						//{
						//	// If the last lord wasn't the original owner, that player lose vassalage
						//	if (lastLordPlayerId != armyNode.originalPlayerId) {
						//		_playerOwnedManagers[lastLordPlayerId].LoseVassal(armyNode.nodeId);
						//	}

						//	// Attacker made node owner's a vassal
						//	if (lordPlayerId != armyNode.originalPlayerId) {
						//		_playerOwnedManagers[lordPlayerId].GainVassal(armyNode.nodeId);
						//		AddPopupAll(PopupInfo(lordPlayerId, playerName(lordPlayerId) + " has conquered " + armyNodeName(armyNode.nodeId) + "."), -1);
						//		AddPopup(armyNode.originalPlayerId, 
						//			"<Bold>You became " + playerName(lordPlayerId) + "'s vassal.</>"
						//					 "<space>"
						//					 "<bullet>As a vassal, you pay your lord 5% <img id=\"Coin\"/> revenue as a tribute each round.</>"
						//					 "<bullet>If your lord is ahead of you in science, you gain +20% <img id=\"Science\"/> from knowledge transfer.</>");
						//		_playerOwnedManagers[lordPlayerId].RecalculateTaxDelayed();
						//		_playerOwnedManagers[armyNode.originalPlayerId].RecalculateTaxDelayed();

						//		CheckDominationVictory(lordPlayerId);
						//	}
						//	// lord title returns to its original owner, liberated!
						//	else
						//	{
						//		AddPopupAll(PopupInfo(lordPlayerId, playerName(armyNode.originalPlayerId) + " has declared independence from " + playerName(lastLordPlayerId) + "."), -1);
						//	}
						//	
						//	armyNode.lastLordPlayerId = lordPlayerId;
						//}

						/*
						 * Economic Victory
						 */
						//const int32 thousandsToWarn1 = 500;
						//const int32 thousandsToWarn2 = 800;

						//auto warnEconVictoryPopup = [&](int32 thousandsToWarn)
						//{
						//	AddPopup(playerId, FText::Format(
						//		LOCTEXT("WarnEcon_Pop", "You accumulated {0},000<img id=\"Coin\"/>!<space>You will achieve economic victory once you accumulate 1,000,000<img id=\"Coin\"/>."),
						//		TEXT_NUM(thousandsToWarn)
						//	));
						//	AddPopupAll(PopupInfo(-1,
						//		FText::Format(
						//			LOCTEXT("WarnEconAll_Pop", "{0} accumulated {1},000<img id=\"Coin\"/>.<space>At 1,000,000<img id=\"Coin\"/> {0} will achieve the economic victory."),
						//			playerNameT(playerId),
						//			TEXT_NUM(thousandsToWarn)
						//		)
						//	), playerId);
						//};
						//
						//if (_playerOwnedManagers[playerId].economicVictoryPhase == 0 && money(playerId) > (thousandsToWarn1 * 1000)) {
						//	_playerOwnedManagers[playerId].economicVictoryPhase = 1;
						//	warnEconVictoryPopup(thousandsToWarn1);
						//}
						//else if (_playerOwnedManagers[playerId].economicVictoryPhase == 1 && money(playerId) > (thousandsToWarn2 * 1000)) {
						//	_playerOwnedManagers[playerId].economicVictoryPhase = 2;
						//	warnEconVictoryPopup(thousandsToWarn2);
						//}
						//else if (_playerOwnedManagers[playerId].economicVictoryPhase == 2 && money(playerId) > (1000000)) 	{
						//	_endStatus.victoriousPlayerId = playerId;
						//	_endStatus.gameEndEnum = GameEndEnum::EconomicVictory;
						//}
						
					}
				});
				
				for (int j = GameConstants::MaxPlayersPossible; j < GameConstants::MaxPlayersAndAI; j++) {
					_aiPlayerSystem[j].Tick1Sec();
				}

				_eventLogSystem.Tick1Sec();
				_worldTradeSystem.Tick1Sec();


				// Delete old particles
				for (int32 i = 0; i < GameMapConstants::TotalRegions; i++) {
					std::vector<FireOnceParticleInfo>& particles = _regionToFireOnceParticleInfo[i];
					for (int32 j = particles.size(); j-- > 0;) {
						if (Time::Ticks() - particles[j].startTick > Time::TicksPerSecond) {
							particles.erase(particles.begin() + j);
						}
					}
				}

				/*
				 * Tick 1 Sec TownBase
				 */
				auto tickTownBase1Sec = [&](TownManagerBase* provinceTownManager)
				{
					provinceTownManager->Tick1Sec_TownBase();

					/*
					 * ClaimProgress
					 */
					int32 provinceTownId = provinceTownManager->townId();
					int32 provincePlayerId = provinceTownManager->playerId();
					
					 // Conquer Province
					std::vector<ProvinceClaimProgress> claimProgresses = provinceTownManager->defendingClaimProgress();
					for (const ProvinceClaimProgress& claimProgress : claimProgresses)
					{
						/*
						 * Attacker Won
						 */
						if (claimProgress.attackerWon())
						{
							ProvinceAttackEnum provinceAttackEnum = provinceTownManager->GetProvinceAttackEnum(claimProgress.provinceId, claimProgress.attackerPlayerId);
							int32 provinceOwnerTownId = provinceOwnerTown_Major(claimProgress.provinceId);

							/*
							 * Military
							 */
							provinceTownManager->EndConquer(claimProgress.provinceId);
							townManagerBase(claimProgress.attackerPlayerId)->EndConquer_Attacker(claimProgress.provinceId);


							/*
							 * Non-Military
							 */

							//! Raid
							if (claimProgress.attackEnum == ProvinceAttackEnum::RaidBattle)
							{
								CompleteRaid(claimProgress.provinceId, claimProgress.attackerPlayerId, provinceTownId);
							}
							//! Minor Town
							else if (provincePlayerId == -1)
							{
								// If there is an existing lord, the lord lose the vassal
								int32 oldLordPlayerId = provinceTownManager->lordPlayerId();
								if (oldLordPlayerId != -1) {
									LoseVassalHelper(oldLordPlayerId, provincePlayerId);
								}

								
								// Vassalize
								int32 lordId = claimProgress.attackerPlayerId;
								townManagerBase(lordId)->GainVassal(provinceTownManager->townId());
								provinceTownManager->SetLordPlayerId(lordId);

								
								AddPopupAll(PopupInfo(lordId,
									FText::Format(LOCTEXT("XhasConqueredMinorTownY", "{0} has conquered {1}."), playerNameT(lordId), playerNameT(provincePlayerId))
								), -1);
								
							}
							//! Capital
							else if (homeProvinceId(provincePlayerId) == claimProgress.provinceId)
							{
								// Home town, vassalize/declare independence instead
								//  For Declare Independence, the attacker is the town owner
								if (claimProgress.attackerPlayerId == provincePlayerId)
								{
									// Declare Independence
									int32 oldLordPlayerId = lordPlayerId(provincePlayerId);

									LoseVassalHelper(oldLordPlayerId, provincePlayerId);

									AddPopupAll(PopupInfo(provincePlayerId,
										FText::Format(LOCTEXT("IndependenceAll_Pop", "{0} declared independence from {1}."), playerNameT(provincePlayerId), playerNameT(oldLordPlayerId))
									), -1);
								}
								else
								{
									// Player getting vassalized lose all of its own vassals (player vassal only)
									const std::vector<int32>& vassalTownIds = provinceTownManager->vassalTownIds();
									for (int32 vassalTownId : vassalTownIds) {
										LoseVassalHelper(provincePlayerId, vassalTownId);
									}

									// If there is an existing lord, the lord lose the vassal
									int32 oldLordPlayerId = provinceTownManager->lordPlayerId();
									if (oldLordPlayerId != -1) {
										LoseVassalHelper(oldLordPlayerId, provincePlayerId);
									}

									// Vassalize
									int32 lordId = claimProgress.attackerPlayerId;
									townManagerBase(lordId)->GainVassal(provinceTownManager->townId());
									provinceTownManager->SetLordPlayerId(lordId);

									_LOG(PunNetwork, "Vassalize [sim] pid:%d lordId:%d", provincePlayerId, provinceTownManager->lordPlayerId());

									AddPopupAll(PopupInfo(lordId,
										FText::Format(LOCTEXT("XhasConqueredY", "{0} has conquered {1}."), playerNameT(lordId), playerNameT(provincePlayerId))
									), -1);
									AddPopup(provincePlayerId,
										FText::Format(LOCTEXT("NewLord_Pop", "<Bold>You became {0}'s vassal.</><space><bullet>As a vassal, you pay your lord 5% <img id=\"Coin\"/> revenue as a tribute each round.</><bullet>Half of your <img id=\"Influence\"/> income goes to your lord.</><bullet>If your lord is ahead of you in science, you gain +20% <img id=\"Science\"/> from knowledge transfer.</>"),
											playerNameT(lordId)
										)
									);

									// Recalculate Tax
									const std::vector<int32>& lordTownIds = _playerOwnedManagers[lordId].townIds();
									for (int32 townId : lordTownIds) {
										_townManagers[townId]->RecalculateTaxDelayed();
									}

									const std::vector<int32>& playerTownIds = _playerOwnedManagers[provincePlayerId].townIds();
									for (int32 townId : playerTownIds) {
										_townManagers[townId]->RecalculateTaxDelayed();
									}

									// CheckDominationVictory(lordPlayerId);
								}
							}
							//! ConquerColony
							else if (provinceAttackEnum == ProvinceAttackEnum::ConquerColony)
							{
								// town swap hands
								ChangeTownOwningPlayer(provinceOwnerTownId, claimProgress.attackerPlayerId);

								AddPopupAll(PopupInfo(provincePlayerId,
									FText::Format(LOCTEXT("ConquerColonyAll_Pop", "{0} has taken control of {1} from {2}."), playerNameT(claimProgress.attackerPlayerId), townNameT(provinceOwnerTownId), playerNameT(provincePlayerId))
								), -1);

								//// If this is AI player's capital, set the AI inactive
								//if (IsAIPlayer(playerId)) {
								//	_aiPlayerSystem[playerId].SetActive(false);

								//	AddPopupAll(PopupInfo(playerId,
								//		FText::Format(LOCTEXT("AIEliminated_Pop", "{0} is eliminated."), playerNameT(playerId))
								//	), -1);
								//}
							}
							//! ConquerProvince
							else
							{
								// Attacker now owns the province
								// Destroy any leftover building owned by player
								ClearProvinceBuildings(claimProgress.provinceId);

								SetProvinceOwner_Popup(claimProgress.provinceId, claimProgress.attackerPlayerId, false);

								//SetProvinceOwner(claimProgress.provinceId, attackerTownId);
							}
						}
						/*
						 * Attacker Lost
						 */
						else if (claimProgress.attackerLost())
						{
							ProvinceAttackEnum provinceAttackEnum = provinceTownManager->GetProvinceAttackEnum(claimProgress.provinceId, claimProgress.attackerPlayerId);

							/*
							 * Military
							 */
							provinceTownManager->EndConquer(claimProgress.provinceId);
							townManagerBase(claimProgress.attackerPlayerId)->EndConquer_Attacker(claimProgress.provinceId);

							// Note: no point in trying to pipe claimProgress here, too confusing...
							if (provincePlayerId == -1) // Minor Town
							{
								AddPopupToFront(claimProgress.attackerPlayerId, FText::Format(LOCTEXT("VassalizeNotSuccessful", "Your attempt to vassalize {0} was not successful."), townNameT(provinceTownId)));
							}
							else if (homeProvinceId(provincePlayerId) == claimProgress.provinceId)
							{
								// Home town, vassalize instead
								AddPopupToFront(provincePlayerId, FText::Format(LOCTEXT("DefendSuccessfulMain", "You successfully defend your independence against {0}"), playerNameT(claimProgress.attackerPlayerId)));
								AddPopupToFront(claimProgress.attackerPlayerId, FText::Format(LOCTEXT("VassalizeNotSuccessful", "Your attempt to vassalize {0} was not successful."), playerNameT(provincePlayerId)));
							}
							else if (provinceAttackEnum == ProvinceAttackEnum::ConquerColony)
							{
								AddPopupToFront(provincePlayerId, FText::Format(LOCTEXT("DefendSuccessful_Colony", "You successfully defend your town against {0}"), playerNameT(claimProgress.attackerPlayerId)));
								AddPopupToFront(claimProgress.attackerPlayerId, FText::Format(LOCTEXT("ConquerNotSuccessful_Colony", "Your attempt to gain control of {0} was not successful."), townNameT(provinceTownId)));
							}
							// ConquerProvince
							else {
								AddPopupToFront(provincePlayerId, FText::Format(LOCTEXT("DefendSuccessfulProvince", "You successfully defended your Province against {0}"), playerNameT(claimProgress.attackerPlayerId)));
								AddPopupToFront(claimProgress.attackerPlayerId, FText::Format(LOCTEXT("ConquerNotSuccessful", "Your attempt to conquer {0}'s Province was not successful."), playerNameT(provincePlayerId)));
							}
						}
					}
				};
				
				//! Major Towns
				ExecuteOnPlayersAndAI([&](int32 playerId)
				{
					if (_playerOwnedManagers[playerId].hasCapitalTownhall())
					{
						const std::vector<int32>& townIds = _playerOwnedManagers[playerId].townIds();
						for (int32 townId : townIds) {
							tickTownBase1Sec(_townManagers[townId].get());
						}
					}
				});
				
				//! Minor Towns
				for (const std::unique_ptr<TownManagerBase>& minorTown : _minorTownManagers)
				{
					if (minorTown->isValid()) {
						tickTownBase1Sec(minorTown.get());
					}
				}


				/*
				 * CityNetwork
				 */
				TestCityNetworkStage();
				
			}

			// Tick quarter sec
			if (_tickCount % (Time::TicksPerSecond / 4) == 0)
			{
				ExecuteOnPlayersAndAI([&](int32 playerId) 
				{
					/*
					 * Science
					 */
					if (_playerOwnedManagers[playerId].hasChosenLocation()) {
						unlockSystem(playerId)->Research(GetScience100PerRound(playerId), 4);
					}
				});
			}


			// Show happiness message in the middle of the round, twice a year
			int32 tickModYear = _tickCount % Time::TicksPerYear;
			if (tickModYear == (Time::TicksPerYear / 4) ||
				tickModYear == (Time::TicksPerYear * 3 / 4))
			{
				ExecuteOnPlayersAndAI([&](int32 playerId)
				{
					int32 happiness = townManager(playerId).aveOverallHappiness();
					
					FText eventText;

					if (happiness >= 95) {
						eventText = LOCTEXT("HappinessIdolize_Event", "People idolize you as god.");
					}
					else if (happiness >= 85) {
						eventText = LOCTEXT("HappinessLove_Event", "People love you.");
					}
					else if (happiness >= 70) {
						eventText = LOCTEXT("HappinessOk_Event", "People think you are ok.");
					}
					else if (happiness >= 55) {
						eventText = LOCTEXT("HappinessHate_Event", "People hate you.");
					}
					else {
						eventText = LOCTEXT("HappinessFurious_Event", "People are furious at you.");
					}
					
					_eventLogSystem.AddEventLog(playerId,
						eventText,
						false
					);
				});
			}

			ExecuteOnPlayersAndAI([&](int32 playerId) 
			{
				if (_playerOwnedManagers[playerId].hasCapitalTownhall()) 
				{
					_playerOwnedManagers[playerId].Tick();

					//! Tick Round Town
					const std::vector<int32>& townIds = _playerOwnedManagers[playerId].townIds();
					for (int32 townId : townIds) {
						_townManagers[townId]->Tick();
					}
					

					questSystem(playerId)->Tick();

					//townhall(playerId).armyNode.Tick();
				}
			});
		}

		if (PunSettings::IsOn("TickUnits")) {
			_unitSystem->Tick();
		}

		if (PunSettings::IsOn("TickBuildings")) {
			_buildingSystem->Tick();
		}

		if (PunSettings::IsOn("TickTiles")) 
		{
			//PUN_LLM(PunSimLLMTag::Trees);
			_treeSystem->Tick();
		}

		if (PunSettings::IsOn("TickUnlocks"))
		{

		}

		
		// Hashes
#if CHECK_TICKHASH
		{
			AddTickHash(_tickHashes, _tickCount, TickHashEnum::Input, _currentInputHashes);
			AddTickHash(_tickHashes, _tickCount, TickHashEnum::RandCount, GameRand::RandUsageCount());
			AddTickHash(_tickHashes, _tickCount, TickHashEnum::Rand, GameRand::RandState());
			AddTickHash(_tickHashes, _tickCount, TickHashEnum::Unit, _unitSystem->GetSyncHash());
			AddTickHash(_tickHashes, _tickCount, TickHashEnum::Building, _buildingSystem->GetSyncHash());

			int32 hash = 0;
			ExecuteOnPlayersAndAI([&](int32 playerId) {
				hash += populationPlayer(playerId);
			});
			AddTickHash(_tickHashes, _tickCount, TickHashEnum::GlobalStats, hash);
			
			AddTickHash(_tickHashes, _tickCount, TickHashEnum::UnitActions, _unitSystem->GetSyncHash_Actions());

			int32 hashResources = 0;
			ExecuteOnPlayersAndAI([&](int32 playerId) {
				hashResources += resourceSystem(playerId).GetSyncHash();
			});
			AddTickHash(_tickHashes, _tickCount, TickHashEnum::Resources, hashResources);
		}
#endif

#if USE_LEAN_PROFILING
		LeanProfiler::FinishTick(static_cast<int32>(LeanProfilerEnum::R_resourceCount), static_cast<int32>(LeanProfilerEnum::DropoffFoodAnimal));
		if (Time::Ticks() % (PunSettings::Get("LeanProfilingTicksInterval") * gameSpeed) == 0)
		{
			LeanProfiler::FinishInterval(static_cast<int32>(LeanProfilerEnum::R_resourceCount), static_cast<int32>(LeanProfilerEnum::DropoffFoodAnimal));
		}
#endif
		

		// Snow
		{
			auto snowAccumulate = [&](FloatDet& lastSnow, FloatDet& snow, int32 temperatureFraction100)
			{
				lastSnow = snow;

				FloatDet minCelsius = ModifyCelsiusByBiome(temperatureFraction100, Time::MinCelsiusBase());
				FloatDet maxCelsius = ModifyCelsiusByBiome(temperatureFraction100, Time::MaxCelsiusBase(), maxCelsiusDivider);

				FloatDet currentCelsius = Time::CelsiusHelper(minCelsius, maxCelsius);

				const FloatDet snowAccumulationStarts = -FD0_X(21);// FD0_X(59); // Starts at 5.9 C
				snow += (-currentCelsius + snowAccumulationStarts) / Time::TicksPerSeason * PunSettings::Get("ForceSnowSpeed") / 100;
				snow = FDMin(FDMax(snow, 0), FDOne);
				//PUN_LOG("SnowAccumulation snow:%f celsius:%f", FDToFloat(snow), FDToFloat(currentCelsius));
			};

			snowAccumulate(_lastTickSnowAccumulation3, _snowAccumulation3, tundraTemperatureStart100);
			snowAccumulate(_lastTickSnowAccumulation2, _snowAccumulation2, borealTemperatureStart100);
			snowAccumulate(_lastTickSnowAccumulation1, _snowAccumulation1, forestTemperatureStart100);
		}
		

		_tickCount++;


		
		// SaveCheck
		if (saveCheckActive())
		{
			PUN_CHECK(saveCheckState == SaveCheckState::SerializeBeforeSave ||
					  saveCheckState == SaveCheckState::SerializeAfterSave);
			
			auto saveCrc = [&](int32 stage)
			{
				_LOG(PunSaveCheck, "Serializing CRC %d - ticks:%d randState:%d", stage, Time::Ticks(), GameRand::RandState());
				FBufferArchive SaveArchive;
				SaveArchive.SetIsSaving(true);
				SaveArchive.SetIsLoading(false);

				saveCheckCrcsToTick.push_back(std::vector<int32>());
				saveCheckCrcLabels.clear();
				std::vector<int32>* crcs = &(saveCheckCrcsToTick.back());
				std::vector<std::string>* crcLabels = &saveCheckCrcLabels;
				Serialize(SaveArchive, SaveArchive, GameSaveChunkEnum::All, crcs, crcLabels);
			};

			auto end = [&]() {
				saveCheckStartTick = -1;

				if (saveCheckState == SaveCheckState::SerializeBeforeSave) {
					_LOG(PunSaveCheck, "SerializeBeforeSave - complete");
					saveCheckState = SaveCheckState::SerializeBeforeSave_Done;
				}
				else if (saveCheckState == SaveCheckState::SerializeAfterSave) {
					_LOG(PunSaveCheck, "SerializeAfterSave - complete");
					saveCheckState = SaveCheckState::SerializeAfterSave_Done;
				}
			};
			
			if (Time::Ticks() == saveCheckStartTick + 1) {
				saveCrc(1);
				end();
			}
			//else if (Time::Ticks() == saveCheckStartTick + Time::TicksPerSecond) {
			//	saveCrc(2);
			//}
			//else if (Time::Ticks() == saveCheckStartTick + Time::TicksPerMinute) {
			//	saveCrc(3);
			//}
		}
	}

	CheckIntegrity();
}

/**
 * IGameSimulationCore
 */

void GameSimulationCore::ResetUnitActions(int id, int32 waitTicks)
{
	_unitSystem->ResetActions_SystemPart(id, waitTicks);
}

int32 GameSimulationCore::AddUnit(UnitEnum unitEnum, int32 townId, WorldAtom2 location, int32 ageTicks)
{
	int objectId = _unitSystem->AddUnit(unitEnum, townId, location, ageTicks);

	//PlayerOwned
	if (unitEnum == UnitEnum::Human && townId != -1) {
		_townManagers[townId]->PlayerAddHuman(objectId);
	}
	return objectId;
}
void GameSimulationCore::RemoveUnit(int id)
{
	// PlayerOwned
	UnitStateAI& unit = unitAI(id);
	int32 townId = unit.townId();
	if (_unitSystem->unitEnum(id) == UnitEnum::Human && townId != -1) {
		PUN_DEBUG_EXPR(unit.AddDebugSpeech("RemoveUnit Player"));

		Building* workplace = unit.workplace();
		if (workplace) {
			workplace->RemoveOccupant(id);
		}
		unit.SetWorkplaceId(-1);
		unit.SetHouseId(-1);
		_townManagers[townId]->PlayerRemoveHuman(id);
	}

	_unitSystem->RemoveUnit(id);

	TryRemoveDescriptionUI(ObjectTypeEnum::Unit, id);
}

/**
 * Player interactions
 */

bool GameSimulationCore::ExecuteNetworkCommand(std::shared_ptr<FNetworkCommand> command)
{
	switch (command->commandType())
	{
	case NetworkCommandEnum::PlaceBuilding: {
		int32 buildingId = PlaceBuilding(*static_pointer_cast<FPlaceBuilding>(command));
		return (buildingId != -1);
	}
	case NetworkCommandEnum::PlaceDrag:			PlaceDrag(*static_pointer_cast<FPlaceDrag>(command)); break;
	case NetworkCommandEnum::JobSlotChange:		JobSlotChange(*static_pointer_cast<FJobSlotChange>(command)); break;
	case NetworkCommandEnum::SetAllowResource:	SetAllowResource(*static_pointer_cast<FSetAllowResource>(command)); break;
	case NetworkCommandEnum::SetPriority:		SetPriority(*static_pointer_cast<FSetPriority>(command)); break;
	case NetworkCommandEnum::SetTownPriority:	SetTownPriority(*static_pointer_cast<FSetTownPriority>(command)); break;
	case NetworkCommandEnum::SetGlobalJobPriority:SetGlobalJobPriority(*static_pointer_cast<FSetGlobalJobPriority>(command)); break;
	case NetworkCommandEnum::GenericCommand:	GenericCommand(*static_pointer_cast<FGenericCommand>(command)); break;

	case NetworkCommandEnum::TradeResource:		TradeResource(*static_pointer_cast<FTradeResource>(command)); break;
	case NetworkCommandEnum::SetIntercityTrade:	SetIntercityTrade(*static_pointer_cast<FSetIntercityTrade>(command)); break;
	case NetworkCommandEnum::UpgradeBuilding:	UpgradeBuilding(*static_pointer_cast<FUpgradeBuilding>(command)); break;
	case NetworkCommandEnum::ChangeWorkMode:	ChangeWorkMode(*static_pointer_cast<FChangeWorkMode>(command)); break;
	case NetworkCommandEnum::ChooseLocation:	ChooseLocation(*static_pointer_cast<FChooseLocation>(command)); break;
	case NetworkCommandEnum::ChooseInitialResources:ChooseInitialResources(*static_pointer_cast<FChooseInitialResources>(command)); break;
		
	case NetworkCommandEnum::Cheat:				Cheat(*static_pointer_cast<FCheat>(command)); break;
	case NetworkCommandEnum::PopupDecision:		PopupDecision(*static_pointer_cast<FPopupDecision>(command)); break;

	case NetworkCommandEnum::RerollCards:		RerollCards(*static_pointer_cast<FRerollCards>(command)); break;

	case NetworkCommandEnum::SelectRareCard:	SelectRareCard(*static_pointer_cast<FSelectRareCard>(command)); break;
	case NetworkCommandEnum::BuyCard:			BuyCards(*static_pointer_cast<FBuyCard>(command)); break;
	case NetworkCommandEnum::SellCards:			SellCards(*static_pointer_cast<FSellCards>(command)); break;
	case NetworkCommandEnum::UseCard:			UseCard(*static_pointer_cast<FUseCard>(command)); break;
	case NetworkCommandEnum::UnslotCard:		UnslotCard(*static_pointer_cast<FUnslotCard>(command)); break;

	case NetworkCommandEnum::Attack:			Attack(*static_pointer_cast<FAttack>(command)); break;

	case NetworkCommandEnum::ClaimLand:			ClaimLand(*static_pointer_cast<FClaimLand>(command)); break;
	case NetworkCommandEnum::ChooseResearch:	ChooseResearch(*static_pointer_cast<FChooseResearch>(command)); break;

	case NetworkCommandEnum::ChangeName:		ChangeName(*static_pointer_cast<FChangeName>(command)); break;
	case NetworkCommandEnum::SendChat:			SendChat(*static_pointer_cast<FSendChat>(command)); break;
	default: UE_DEBUG_BREAK();
	}

#if CHECK_TICKHASH
	_currentInputHashes += command->GetTickHash();
#endif

#if KEEP_ACTION_HISTORY
	// Poor man's copy
	PunSerializedData blobIn(true);
	NetworkHelper::SerializeAndAppendToBlob(command, blobIn);

	PunSerializedData blobOut(false, blobIn);
	
	_commandsExecuted.push_back(NetworkHelper::DeserializeFromBlob(blobOut));
	_commandsTickExecuted.push_back(Time::Ticks());
#endif

	return true;
}

int32 GameSimulationCore::PlaceBuilding(FPlaceBuilding parameters)
{
	TileArea area = parameters.area;
	Direction faceDirection = static_cast<Direction>(parameters.faceDirection);
	TileArea frontArea = area.GetFrontArea(faceDirection);
	CardEnum cardEnum = static_cast<CardEnum>(parameters.buildingEnum);
	int32 playerId = parameters.playerId;

	bool succeedUsingCard = true;

//#if TRAILER_MODE
//	#define TRAILER_LOG(x) if (playerId == 0) _LOG(PunTrailer, "!!!Place Failed %s %s %s", ToTChar(GetBuildingInfoInt(parameters.buildingEnum).name), ToTChar(parameters.area.ToString()), ToTChar(std::string(x)));
//#else
//	#define TRAILER_LOG(x) 
//#endif

	if (cardEnum != CardEnum::BoarBurrow &&
		parameters.playerId != -1) 
	{
		_LOG(LogNetworkInput, "[%d] PlaceBuilding (pid:%d) %s %s", tempVariable, playerId, *BuildingInfo[parameters.buildingEnum].nameF(), *ToFString(parameters.area.ToString()));
	}

	// Don't allow building without bought card...
	if (!IsReplayPlayer(parameters.playerId) &&
		parameters.useBoughtCard && 
		!_cardSystem[playerId].CanUseBoughtCard(cardEnum))
	{
		//TRAILER_LOG("IsReplayPlayer:" + to_string(IsReplayPlayer(parameters.playerId)) + 
		//			" useBoughtCard:" + to_string(parameters.useBoughtCard) + 
		//			" CanUseBought:" + to_string(_cardSystem[playerId].CanUseBoughtCard(cardEnum)));
		return -1;
	}

	// Delivery Point
	if (parameters.buildingIdToSetDelivery != -1)
	{
		// Guard: SetDeliveryTarget crash
		if (!IsValidBuilding(parameters.buildingIdToSetDelivery)) {
			return -1;
		}
		
		Building& buildingToSetDelivery = buildingChecked(parameters.buildingIdToSetDelivery);

		if (parameters.center == WorldTile2::Invalid) {
			buildingToSetDelivery.TryRemoveDeliveryTarget();
			return -1;
		}
		
		int32 deliveryTargetId = buildingIdAtTile(parameters.center);
		
		if (Building* targetBuilding = buildingPtr(deliveryTargetId))
		{
			// Ensure delivery point is in the same city
			if (targetBuilding->townId() != buildingToSetDelivery.townId()) {
				AddPopup(parameters.playerId, LOCTEXT("SetDeliveryWarning_SameTown", "Cannot set Delivery Target to a building in another Town."));
				return -1;
			}
			
			if (buildingToSetDelivery.deliveryTargetId() != deliveryTargetId) {
				PUN_CHECK(IsStorage(buildingChecked(deliveryTargetId).buildingEnum()));
				buildingToSetDelivery.SetDeliveryTarget(deliveryTargetId);
			}
		}
		return -1;
	}

	//! Reveal Spy Nest
	if (parameters.placementType == PlacementType::RevealSpyNest)
	{
		if (moneyCap32(parameters.playerId) < GetRevealSpyNestPrice())
		{
			AddPopup(parameters.playerId, LOCTEXT("RevealSpyNest_NoMoney", "Not enough money to Reveal Spy Nest."));
			return -1;
		}
		
		ChangeMoney(parameters.playerId, -GetRevealSpyNestPrice());

		TileArea(parameters.center, GetRevealSpyNestRadius()).ExecuteOnAreaSkip4_WorldTile2([&](WorldTile2 tile)
		{
			if (Building* bld = buildingAtTile(tile)) {
				if (bld->isEnum(CardEnum::House)) {
					House& house = bld->subclass<House>();
					if (house.spyPlayerId() != -1) {
						house.RemoveSpyNest();
					}
				}
			}
		});

		return -1;
	}
	

	// Used converter card, decrease the converter card we have
	if (parameters.useWildCard != CardEnum::None)
	{
		if (!_cardSystem[playerId].HasBoughtCard(parameters.useWildCard)) {
			//TRAILER_LOG("!HasBoughtCard");
			return -1;
		}

		int32 converterPrice = cardSystem(playerId).GetCardPrice(cardEnum);
		if (moneyCap32(playerId) < converterPrice) {
			AddPopupToFront(playerId, 
				FText::Format(LOCTEXT("NeedCoinToConvertWildCard", "Need {0}<img id=\"Coin\"/> to convert wild card to this building."), TEXT_NUM(converterPrice)), 
				ExclusiveUIEnum::ConverterCardHand, "PopupCannot");
			//TRAILER_LOG("money(playerId) < converterPrice");
			return -1;
		}

		_cardSystem[playerId].RemoveCardsOld(parameters.useWildCard, 1);
		_cardSystem[playerId].converterCardState = ConverterCardUseState::None;
		ChangeMoney(playerId, -converterPrice);
	}

	// Hand over Foreign building to another player
	int32 foreignBuildingOwner = parameters.playerId;
	if (cardEnum == CardEnum::HumanitarianAidCamp) {
		parameters.playerId = tileOwnerTown(parameters.center);
	}

	/*
	 * Special cases
	 */
	if (IsSpellCard(cardEnum))
	{
		auto spellOnArea = [&](auto spellOnTile) {
			area.ExecuteOnArea_WorldTile2([&](WorldTile2 location) {
				if (WorldAtom2::DistanceLessThan(parameters.center, location, AreaSpellRadius(cardEnum))) {
					spellOnTile(location);
				}
			});
		};

		if (cardEnum == CardEnum::FireStarter)
		{
			spellOnArea([&](WorldTile2 location) {
				int32 buildingId = buildingIdAtTile(location);
				if (buildingId != -1) {
					_buildingSystem->StartFire(buildingId);
				}
			});
		}

		//! Building Spell
		else if (/*cardEnum == CardEnum::Steal ||*/
				cardEnum == CardEnum::Snatch ||
				cardEnum == CardEnum::Kidnap ||
				cardEnum == CardEnum::Terrorism ||
				cardEnum == CardEnum::SharingIsCaring)
		{
			int32 buildingId = buildingIdAtTile(parameters.center);
			if (buildingId != -1 && building(buildingId).isEnum(CardEnum::Townhall)) 
			{
				int32 targetPlayerId = building(buildingId).playerId();
				int32 targetTownId = building(buildingId).townId();

				//if (cardEnum == CardEnum::Steal)
				//{
				//	//if (population(targetPlayerId) < population(playerId)) {
				//	//	AddPopup(playerId, "Cannot steal from a weaker town.");
				//	//	return -1;
				//	//}

				//	// Guard
				//	if (playerOwned(targetPlayerId).HasBuff(CardEnum::TreasuryGuard))
				//	{
				//		AddPopup(targetPlayerId, 
				//			FText::Format(LOCTEXT("FailedStealMoney_TargetPop", "{0} failed to steal money, because of your well-planned Treasury Guard."), playerNameT(playerId))
				//		);
				//		AddPopup(playerId, 
				//			FText::Format(LOCTEXT("FailedStealMoney_SelfPop", "You failed to steal money from {0}, because of Treasury Guard."), playerNameT(targetPlayerId))
				//		);
				//	}
				//	else
				//	{
				//		int32 targetPlayerMoney = moneyCap32(targetPlayerId);
				//		targetPlayerMoney = max(0, targetPlayerMoney); // Ensure no negative steal..
				//		
				//		int32 actualSteal = targetPlayerMoney * 30 / 100;
				//		ChangeMoney(targetPlayerId, -actualSteal);
				//		ChangeMoney(playerId, actualSteal);
				//		AddPopup(targetPlayerId, 
				//			FText::Format(LOCTEXT("XStoleCoinFromYou_Pop", "{0} stole {1}<img id=\"Coin\"/> from you"), playerNameT(playerId), TEXT_NUM(actualSteal))
				//		);
				//		AddPopup(playerId, 
				//			FText::Format(LOCTEXT("YouStoleCoinFromX_Pop", "You stole {0}<img id=\"Coin\"/> from {1}"), TEXT_NUM(actualSteal), townNameT(targetPlayerId))
				//		);

				//		ChangeRelationshipModifier(targetPlayerId, playerId, RelationshipModifierEnum::YouStealFromUs, -actualSteal / GoldToRelationship);
				//	}
				//}
				//else 
				if (cardEnum == CardEnum::Snatch)
				{
					//if (population(targetPlayerId) < population(playerId)) {
					//	AddPopup(playerId, "Cannot steal from a weaker town.");
					//	return -1;
					//}

					// Guard
					if (playerOwned(targetPlayerId).HasBuff(CardEnum::TreasuryGuard))
					{
						AddPopup(targetPlayerId, 
							FText::Format(LOCTEXT("FailedToSnatch_TargetPop", "{0} failed to snatch money, because of your well-planned Treasury Guard."), playerNameT(playerId))
						);
						AddPopup(playerId,
							FText::Format(LOCTEXT("FailedToSnatch_SelfPop", "You failed to snatch money from {0}, because of Treasury Guard."), playerNameT(targetPlayerId))
						);
					}
					else
					{
						int32 targetPlayerMoney = moneyCap32(targetPlayerId);
						targetPlayerMoney = max(0, targetPlayerMoney); // Ensure no negative steal..

						int32 targetStealMoney = GameRand::RandRound(10 * populationTown(targetPlayerId) * GetSpyEffectivenessOnTarget(playerId, targetPlayerId), 100);
						
						int32 actualSteal = min(targetPlayerMoney, targetStealMoney);
						ChangeMoney(targetPlayerId, -actualSteal);
						ChangeMoney(playerId, actualSteal);
						AddPopup(targetPlayerId, 
							FText::Format(LOCTEXT("Snatched_TargetPop", "{0} snatched {1}<img id=\"Coin\"/> from you"), townNameT(playerId), TEXT_NUM(actualSteal))
						);
						AddPopup(playerId, 
							FText::Format(LOCTEXT("Snatched_SelfPop", "You snatched {0}<img id=\"Coin\"/> from {1}"), TEXT_NUM(actualSteal), townNameT(targetPlayerId))
						);

						ChangeRelationshipModifier(targetPlayerId, playerId, RelationshipModifierEnum::YouStealFromUs, -actualSteal / GoldToRelationship);
					}
				}
				else if (cardEnum == CardEnum::Kidnap)
				{
					//int32 kidnapMoney = 5 * population(playerId);
					//if (money(playerId) < kidnapMoney) {
					//	AddPopup(playerId, "Need (5 x Population)<img id=\"Coin\"/> to kidnap (" + to_string(kidnapMoney) + "<img id=\"Coin\"/>).");
					//	return -1;
					//}

					//int32 targetPopulation = population(targetPlayerId);
					//if (targetPopulation < 20) {
					//	AddPopup(playerId, "Target town's population is too low for kidnap.");
					//	return -1;
					//}

					// Guard
					if (playerOwned(targetPlayerId).HasBuff(CardEnum::KidnapGuard))
					{
						AddPopup(targetPlayerId, 
							FText::Format(LOCTEXT("FailedToKidnap_TargetPop", "{0} failed to kidnap your citizens, because of your well-planned Kidnap Guard."), playerNameT(playerId))
						);
						AddPopup(playerId, 
							FText::Format(LOCTEXT("FailedToKidnap_SelfPop", "You failed to kidnap citizens from {0}, because of Kidnap Guard."), playerNameT(targetPlayerId))
						);
					}
					else
					{
						std::vector<int32> humanIds = townManager(targetTownId).humanIds();

						int32 kidnapTarget = ApplySpyEffectiveness(3, playerId, targetPlayerId);

						int32 kidnapCount = std::min(kidnapTarget, static_cast<int>(humanIds.size()));
						
						for (int32 i = 0; i < kidnapCount; i++) {
							UnitStateAI& unitAI = unitSystem().unitStateAI(humanIds[i]);
							unitAI.Die();
						}

						AddPopup(targetPlayerId, 
							 FText::Format(LOCTEXT("Kidnapped_TargetPop", "{0} kidnapped {1} citizens from you"), playerNameT(playerId), TEXT_NUM(kidnapCount))
						);

						AddPopup(playerId, 
							FText::Format(LOCTEXT("Kidnapped_SelfPop", "You kidnapped {0} citizens from {1}."), TEXT_NUM(kidnapCount), playerNameT(targetPlayerId))
						);

						GetTownhallCapital(playerId).AddImmigrants(kidnapCount);

						ChangeRelationshipModifier(targetPlayerId, playerId, RelationshipModifierEnum::YouStealFromUs, -(kidnapCount * 300) / GoldToRelationship);
					}
				}
				else if (cardEnum == CardEnum::Terrorism)
				{
					std::vector<int32> humanIds = townManager(targetTownId).humanIds();

					int32 targetKillCount = ApplySpyEffectiveness(5, playerId, targetPlayerId);
					
					int32 killCount = std::min(targetKillCount, static_cast<int>(humanIds.size()));

					for (int32 i = 0; i < killCount; i++) {
						UnitStateAI& unitAI = unitSystem().unitStateAI(humanIds[i]);
						unitAI.Die();
					}

					int32 terrorismInfluenceLost = ApplySpyEffectiveness(1000, playerId, targetPlayerId);
					
					AddPopup(targetPlayerId, FText::Format(
						LOCTEXT("Terrorism_TargetPop", "TODO: Terrorism act, {0} died... {1} is the culprit.. fear lead to {2} influence lost"), 
						TEXT_NUM(killCount), playerNameT(playerId), TEXT_NUM(terrorismInfluenceLost)
					));
					AddPopup(playerId, FText::Format(
						LOCTEXT("Terrorism_SelfPop", "TODO: Your Terrorist planted bomb at {0}, killing {1} people...{2} lost {3} influence from fear of Terrorism"), 
						townNameT(targetTownId), TEXT_NUM(killCount), playerNameT(targetPlayerId), TEXT_NUM(terrorismInfluenceLost)
					));

					ChangeInfluence(targetPlayerId, -terrorismInfluenceLost);
					
					ChangeRelationshipModifier(targetPlayerId, playerId, RelationshipModifierEnum::YouTerrorizedUs, -(killCount * 300) / GoldToRelationship);
				}
				
				else if (cardEnum == CardEnum::SharingIsCaring)
				{
					if (resourceSystem(targetPlayerId).CanAddResourceGlobal(ResourceEnum::Wheat, 100))
					{
						AddPopup(targetPlayerId, 
							FText::Format(LOCTEXT("SharingCaring_TargetPop", "{0} gave you 100 wheat."), townNameT(playerId))
						);
						AddPopup(playerId, 
							FText::Format(LOCTEXT("SharingCaring_SelfPop", "You gave {0} 100 wheat"), townNameT(targetPlayerId))
						);
						resourceSystem(targetPlayerId).AddResourceGlobal(ResourceEnum::Wheat, 100, *this);

						ChangeRelationshipModifier(targetPlayerId, playerId, RelationshipModifierEnum::YouGaveUsGifts, FoodCost * 100 / GoldToRelationship);
					}
					else {
						AddPopup(playerId, 
							FText::Format(LOCTEXT("SharingCaringFailed_Pop", "Failed to give {0} 100 wheat.<space>Not enough storage space at the target city."), townNameT(targetPlayerId))
						);
						succeedUsingCard = false;
					}
				}
				else
				{
					UE_DEBUG_BREAK();
				}
			}
		}
		else if (IsAnimalCard(cardEnum))
		{
			int32 buildingId = buildingIdAtTile(parameters.center);
			if (buildingId != -1)
			{
				Building& bld = building(buildingId);

				// If center is Zoo..
				if (bld.isEnum(CardEnum::Zoo)) {
					bld.AddSlotCard(CardStatus(cardEnum, 1));
				}
			}
			else
			{
				if (IsWalkable(parameters.center)) {
					AddUnit(GetAnimalUnitEnumFromCardEnum(cardEnum), -1, parameters.center.worldAtom2(), Time::TicksPerYear);
				}
			}
		}
		else if (cardEnum == CardEnum::InstantBuild)
		{
			int32 buildingId = buildingIdAtTile(parameters.center);
			Building& bld = building(buildingId);
			if (buildingId != -1 && !bld.isConstructed()) {
				int32_t cost = bld.buildingInfo().constructionCostAsMoney() * 3;
				ChangeMoney(playerId, -cost);
				bld.FinishConstruction();
			}
		}
		//else if (cardEnum == CardEnum::Kidnap)
		//{
		//	int32 targetPlayerId = -1;
		//	std::vector<WorldRegion2> regions = area.GetOverlapRegions();
		//	//int32 unitsStoleCount = 0;
		//	//for (WorldRegion2 region : regions) {
		//	//	unitSystem().unitSubregionLists().ExecuteRegion(region, [&](int32 unitId) {
		//	//		if (unitsStoleCount < 3 &&
		//	//			unitSystem().unitEnum(unitId) == UnitEnum::Human &&
		//	//			WorldTile2::Distance(unitSystem().atomLocation(unitId).worldTile2(), parameters.center) < AreaSpellRadius(cardEnum)) 
		//	//		{
		//	//			UnitStateAI& unitAI = unitSystem().unitStateAI(unitId);
		//	//			targetPlayerId = unitAI.playerId();
		//	//			unitSystem().unitStateAI(unitId).Die();
		//	//			unitsStoleCount++;
		//	//		}
		//	//	});
		//	//}


		//	
		//	//if (targetPlayerId != -1)
		//	//{
		//	//	// TODO: people/person resolves, resolve with images?
		//	//	AddPopup(targetPlayerId, townName(playerId) + " kidnapped " + to_string(unitsStoleCount) + " people from you");
		//	//	AddEventLog(playerId, "You kidnapped " + to_string(unitsStoleCount) + " people from " + townName(targetPlayerId), false);
		//	//} else {
		//	//	AddEventLog(playerId, "You kidnapped " + to_string(unitsStoleCount) + " people", false);
		//	//}

		//	//// Add stolen units to our town
		//	//townhall(playerId).AddImmigrants(unitsStoleCount);
		//}
		
		//area.ExecuteOnArea_WorldTile2([&](WorldTile2 location) {
		//	if (WorldTile2::Distance(parameters.center, location) <= AreaSpellRadius(buildingEnum)) 
		//	{
		//		int32_t buildingId = GameMap::buildingId(location.x, location.y);
		//		if (buildingId != -1) {
		//			_buildingSystem->StartFire(buildingId);
		//		}
		//	}
		//});

		// Use action card, remove the card
		if (!IsReplayPlayer(parameters.playerId) &&
			parameters.useBoughtCard &&
			succeedUsingCard)
		{
			_cardSystem[playerId].UseBoughtCard(cardEnum, 1);
		}

		/*
		 * Skill
		 */
		if (cardEnum == CardEnum::SpeedBoost)
		{
			int32 buildingId = buildingIdAtTile(parameters.center);
			if (buildingId != -1)
			{
				Building& bld = building(buildingId);
				playerOwned(playerId).UseSkill(buildingId);
				AddEventLog(playerId, 
					FText::Format(LOCTEXT("SpeedBoostApply_Event", "You boosted {0}'s efficiency."), GetBuildingInfo(bld.buildingEnum()).name),
					false
				);
			}
		}
		
		return -1;
	} // End if(IsSpell)

	if (IsBridgeOrTunnel(cardEnum))
	{
		bool canPlace = true;
		area.ExecuteOnArea_WorldTile2([&](WorldTile2 tile) {
			if (cardEnum == CardEnum::Tunnel) { // Tunnel
				if (!IsMountain(tile)) {
					canPlace = false;
				}
			}
			else {
				if (!IsWater(tile)) {
					canPlace = false;
				}
			}
		});

		if (canPlace) 
		{
			// Permanent card, pay its cost
			if (playerId != -1 && IsPermanentBuilding(playerId, cardEnum)) {
				ChangeMoney(playerId, -_cardSystem[playerId].GetCardPrice(cardEnum));
			}
			
			int32 buildingId = _buildingSystem->AddBuilding(parameters);
			return buildingId;
		}

		return -1;
	}


	// Special Case:
	if (cardEnum == CardEnum::Mint &&
		buildingCount(playerId, CardEnum::Mint) >= 8)
	{
		AddPopup(playerId, 
			LOCTEXT("MaxMintReached", "You can only build the maximum of 8 Mints.")
		);
		return -1;
	}
	
	if (cardEnum == CardEnum::Archives &&
		buildingCount(playerId, CardEnum::Archives) >= 8)
	{
		AddPopup(playerId,
			LOCTEXT("MaxArchivesReached", "You can only build the maximum of 8 Archives.")
		);
		return -1;
	}
	

	// Trap
	if (IsRoadOverlapBuilding(cardEnum)) {
		frontArea = TileArea(frontArea.min(), WorldTile2::Zero);
	}

	// Critter building demolition
	area.ExecuteOnArea_WorldTile2([&](WorldTile2 tile) {
		DemolishCritterBuildingsIncludingFronts(tile, playerId);
	});
	frontArea.ExecuteOnArea_WorldTile2([&](WorldTile2 tile) {
		DemolishCritterBuildingsIncludingFronts(tile, playerId);
	});

	// Double check if the building can still be placed
	bool canPlace = true;

	// TODO: have a proper way to specify these buildability from central system?
	if (IsMountainMine(cardEnum)) 
	{
		int32_t mountainCount = 0;
		area.ExecuteOnArea_WorldTile2([&](WorldTile2 tile) {
			if (GameMap::IsInGrid(tile.x, tile.y) && IsTileBuildableForPlayer(tile, playerId))
			{
				int steps = GameMap::GetFacingStep(faceDirection, area, tile);
				if (steps <= 1) { // 0,1
					if (!IsBuildableForPlayer(tile, playerId)) {
						canPlace = false; // Entrance not buildable
					}
				} else { // 2,3,4
					if (IsMountain(tile)) {
						mountainCount++;
					}
				}
			}
		});
		if (mountainCount < 5) {
			canPlace = false; // Not enough mountain tiles
		}
	}
	else if (IsPortBuilding(cardEnum))
	{
		bool setDockInstruct = false;
		std::vector<PlacementGridInfo> grids;

		int32 portPlayerId = tileOwnerPlayer(area.centerTile());
		
		CheckPortArea(area, faceDirection, cardEnum, grids, setDockInstruct, portPlayerId);

		canPlace = true;
		for (PlacementGridInfo& gridInfo : grids) {
			if (gridInfo.gridEnum == PlacementGridEnum::Red) {
				canPlace = false;
				break;
			}
		}

	}
	// Clay pit
	// Irrigation Reservoir
	else if (cardEnum == CardEnum::ClayPit ||
			cardEnum == CardEnum::IrrigationReservoir)
	{
		area.ExecuteOnArea_WorldTile2([&](WorldTile2 tile) {
			if (!(IsBuildable(tile) && _terrainGenerator->riverFraction(tile) > GetRiverFractionPercentThreshold(cardEnum) / 100.0f)) {
				canPlace = false;
			}
		});
	}
	// Farm
	else if (cardEnum == CardEnum::Farm) 
	{
		// Note: startTile was sent through parameters.center
		std::vector<FarmTile> farmTiles = GetFarmTiles(area, parameters.center, parameters.playerId);
		check(farmTiles.size() > 0);

		// Calculate farm center
		parameters.center = WorldTile2::Invalid;
		int32 minDist = 99999;
		
		WorldTile2 targetCenter = area.centerTile();
		for (int32 i = 0; i < farmTiles.size(); i++) {
			int32 dist = WorldTile2::Distance(targetCenter, farmTiles[i].worldTile);
			if (dist < minDist) {
				minDist = dist;
				parameters.center = farmTiles[i].worldTile;
			}
		}
		check(parameters.center.isValid());
		
		if (IsFarmSizeInvalid(farmTiles, area)) {
			canPlace = false;
		}
	}
	// Road Overlap Building
	else if (IsRoadOverlapBuilding(cardEnum)) {
		area.ExecuteOnArea_WorldTile2([&](WorldTile2 tile) {
			if (!IsRoadOverlapBuildable(tile)) {
				canPlace = false;
			}
		});
	}
	// Townhall doesn't need to check for land ownership
	else if (IsTownPlacement(cardEnum))
	{	
		area.ExecuteOnArea_WorldTile2([&](WorldTile2 tile) {
			if (!IsBuildable(tile)) {
				canPlace = false;
			}
		});

		_LOG(PunBuilding, "Try Building Townhall Area pid:%d canPlace:%d", parameters.playerId, canPlace);
	}
	else if (cardEnum == CardEnum::HumanitarianAidCamp)
	{
		area.ExecuteOnArea_WorldTile2([&](WorldTile2 tile)
		{
			if (!IsBuildable(tile)) {
				canPlace = false;
			}
		});
	}
	// FastBuild should be able to place outside player's territory
	else if (cardEnum != CardEnum::BoarBurrow &&
			!IsRegionalBuilding(cardEnum) &&
			SimSettings::IsOn("CheatFastBuild"))
	{
		area.ExecuteOnArea_WorldTile2([&](WorldTile2 tile) {
			if (!IsBuildable(tile)) {
				canPlace = false;
			}
		});

		parameters.playerId = tileOwnerPlayer(area.centerTile());
		parameters.townId = tileOwnerTown(area.centerTile());

		if (parameters.playerId == -1) {
			canPlace = false;
		}
	}
	/*
	 * All other buildings
	 */
	 // Tech Researched... Build Foreign
	else if (IsForeignPlacement(area.centerTile(), playerId))
	{
		auto emptyFunc = [&](WorldTile2 tile, bool isGreen, bool isInsideTargetTown) {};
		
		auto isBuildableFunc = [&](WorldTile2 tile) { return IsBuildable(tile); };
		auto isFrontBuildableFunc = [&](WorldTile2 tile) { return IsFrontBuildable(tile); };
		
		if (!IsForeignBuildable(area, emptyFunc, area.centerTile(), isBuildableFunc)) {
			return -1;
		}
		if (!IsForeignBuildable(area.GetFrontArea(faceDirection), emptyFunc, area.centerTile(), isFrontBuildableFunc)) {
			return -1;
		}

		//! Check QuickBuild cost + Popup
		int32 quickBuildCost = Building::GetQuickBuildBaseCost(cardEnum, GetBuildingInfo(cardEnum).constructionResources, [&](ResourceEnum resourceEnum) { return 0; });
		if (IsAIPlayer(playerId)) { // Free quickBuild for AI
			ChangeMoney(playerId, quickBuildCost);
		}
		
		if (quickBuildCost > moneyCap32(playerId))
		{
			AddPopupToFront(playerId, FText::Format(
				LOCTEXT("ForeignBuilding_NotEnoughMoney", "Not enough money.<space>Requires {0}<img id=\"Coin\"/> to build."),
				TEXT_NUM(quickBuildCost)
			));
			return -1;
		}
		

		// Minor Towns doesn't allow any building except hotel
		WorldTile2 centerTile = area.centerTile();
		int32 tileTownId = tileOwnerTown(centerTile);
		if (IsMinorTown(tileTownId)) {
			if (cardEnum != CardEnum::Hotel)
			{
				AddPopupToFront(playerId, FText::Format(
					LOCTEXT("MinorTown_NoHouseAndJobWarning", "{0} rejected your request to build.<space>We are a mere minor town, and have no use for this type of building. Hotels could be useful though."),
					townNameT(tileTownId)
				));
				return -1;
			}
		}

		// Minor Town/AI Player allow if relationship is good enough..
		auto popupRelationshipWarning = [&]()
		{
			AddPopupToFront(playerId, FText::Format(
				LOCTEXT("AI_WarnNeedRelationshipForForeignBuilding", "{0} rejected your request to build.<space>Our relationship isn't at that level yet..."),
				townNameT(tileTownId)
			));
		};
		
		const int32 relationshipToAllowForeignBuilding = 0;
		if (IsMinorTown(tileTownId))
		{
			if (townManagerBase(tileTownId)->minorTownRelationship().GetTotalRelationship(playerId) < relationshipToAllowForeignBuilding)
			{
				popupRelationshipWarning();
				return -1;
			}
		}
		int32 tilePlayerId = tileOwnerPlayer(centerTile);
		if (IsAIPlayer(tilePlayerId))
		{
			if (aiPlayerSystem(tilePlayerId).relationship().GetTotalRelationship(playerId) < relationshipToAllowForeignBuilding) 
			{
				popupRelationshipWarning();
				return -1;
			}
		}
	}
	else {
		area.ExecuteOnArea_WorldTile2([&](WorldTile2 tile) {
			if (!IsBuildableForPlayer(tile, playerId)) {
				canPlace = false;
			}
		});
	}


	if (playerId == 0 && !canPlace) {
		PUN_LOG(" !!! Failed to place %s %s", *GetBuildingInfoInt(parameters.buildingEnum).nameF(), ToTChar(parameters.area.ToString()));
	}
	

	/*
	 * Front Grid
	 */
	if (HasBuildingFront(cardEnum)) 
	{
		frontArea.ExecuteOnArea_WorldTile2([&](WorldTile2 tile) {
			if (!IsFrontBuildable(tile)) {
				//if (canPlace) {
				//	TRAILER_LOG(" front buildingEnumAtTile" + GetBuildingInfo(buildingEnumAtTile(tile)).name);
				//}
				
				canPlace = false;
			}
		});

		if (IsTownPlacement(cardEnum)) {
			_LOG(PunBuilding, "Try Building Townhall frontArea pid:%d canPlace:%d", parameters.playerId, canPlace);
		}
	}

	if (canPlace)
	{		
		// TileObjs
		_treeSystem->ForceRemoveTileReservationArea(area);
		if (HasBuildingFront(cardEnum)) {
			_treeSystem->ForceRemoveTileReservationArea(frontArea);
		}
		

		// Special case: Colony Prepare
		if (IsTownPlacement(cardEnum)) 
		{
			PUN_LOG("Town Placement %s", ToTChar(parameters.area.ToString()));
			
			if (cardEnum != CardEnum::Townhall)
			{
				std::vector<int32> provinceIds;
				bool isInvalid = parameters.area2.ExecuteOnAreaWithExit_WorldTile2([&](WorldTile2 tile)
				{
					int32 provinceId = GetProvinceIdClean(tile);
					if (provinceId == -1) return true;
					if (provinceOwnerTown_Major(provinceId) != -1) return true;
					CppUtils::TryAdd(provinceIds, provinceId);
					return false;
				});

				// Prevent Townhall TryInstantFinishConstruction() crash
				if (tileOwnerTown(parameters.center) == -1) {
					isInvalid = false;
				}

				if (!isInvalid)
				{
					// Check if we have enough money
					int32 totalProvincePriceMoney = 0;
					for (int32 provinceId : provinceIds) {
						totalProvincePriceMoney += GetProvinceClaimPrice(provinceId, playerId) * GameConstants::ClaimProvinceByMoneyMultiplier;
					}
					if (moneyCap32(playerId) >= totalProvincePriceMoney)
					{
						// Add town
						int32 townId = _resourceSystems.size();
						_resourceSystems.push_back(ResourceSystem(townId, this));
						_statSystem.AddTown(townId);
						_worldTradeSystem.AddTown(townId);
						_buildingSystem->AddMajorTown(townId);
						_townManagers.push_back(make_unique<TownManager>(playerId, townId, this));
						_playerOwnedManagers[playerId].AddTownId(townId);

						// Conquer Land
						for (int32 provinceId : provinceIds) {
							SetProvinceOwnerFull(provinceId, townId);
						}
						ChangeMoney(playerId, -totalProvincePriceMoney);
					}
					else {
						AddPopupToFront(playerId, LOCTEXT("Not enough money to buy province", "Cannot place the Colony. Not enough money to buy the province."));
						return -1;
					}
				}
			}
		}
		

		// Place Building ***
		int32 buildingId = _buildingSystem->AddBuilding(parameters);

		// Invalid building
		if (buildingId == -1) {
			return - 1;
		}

		Building& bld = building(buildingId);

		// Special case: Townhall
		if (IsTownPlacement(cardEnum)) 
		{
			PlaceInitialTownhallHelper(parameters, buildingId);
			_buildingSystem->OnRefreshFloodGrid(bld.gateTile().region());
		}

		// Special case: Foreign Building
		if (IsForeignPlacement(area.centerTile(), playerId))
		{
			bld.SetForeignBuilder(playerId);
			
			// Take the build money
			int32 quickBuildCost = Building::GetQuickBuildBaseCost(cardEnum, GetBuildingInfo(cardEnum).constructionResources, [&](ResourceEnum resourceEnum) { return 0; });
			ChangeMoney(playerId, -quickBuildCost);

			// AI would already approved this... just finish up the building right away
			if (IsMinorTown(tileOwnerTown(area.centerTile())) || 
				IsAIPlayer(tileOwnerPlayer(area.centerTile())))
			{
				bld.ApproveForeignBuilder();
			}
		}
		

		// Use bought card, remove the card
		if (!IsReplayPlayer(parameters.playerId) && 
			parameters.useBoughtCard) 
		{
			// For foreign building, the builder uses the card
			if (cardEnum == CardEnum::HumanitarianAidCamp) {
				_cardSystem[foreignBuildingOwner].UseBoughtCard(cardEnum, 1);
			}
			else {
				_cardSystem[playerId].UseBoughtCard(cardEnum, 1);
			}
		}

		// Permanent card, pay its cost
		if (playerId != -1 && IsPermanentBuilding(playerId, cardEnum)) {
			ChangeMoney(playerId, -_cardSystem[playerId].GetCardPrice(cardEnum));
		}

		// Auto road placement
		if (!IsCritterBuildingEnum(cardEnum))
		{
			auto tryBuildRoad = [&](WorldTile2 tile) {
				if (IsFrontBuildable(tile) && !_overlaySystem.IsRoad(tile)) {
					_buildingSystem->AddTileBuilding(tile, CardEnum::DirtRoad, playerId);
				}
			};

			// Farm doesn't need front TODO: For no front, just add to array
			if (!HasBuildingFront(cardEnum) ||
				cardEnum == CardEnum::Townhall ||
				cardEnum == CardEnum::MinorCity || // Minor City/Port doesn't need road placement
				cardEnum == CardEnum::MinorCityPort ||
				IsRegionalBuilding(cardEnum)) 
			{}
			else {
				// Finish road construction right away for provincial buildings and TrailerMode
				if (cardEnum == CardEnum::Fort ||
					cardEnum == CardEnum::ResourceOutpost ||
					PunSettings::TrailerSession) 
				{
					frontArea.ExecuteOnArea_WorldTile2([&](WorldTile2 tile) {
						if (IsFrontBuildable(tile) && !_overlaySystem.IsRoad(tile)) {
							overlaySystem().AddRoad(tile, true, true);
						}
					});
				}
				else
				{
					// Place Road Construction otherwise
					frontArea.ExecuteOnArea_WorldTile2([&](WorldTile2 tile) {
						tryBuildRoad(tile);
					});
				}

				// Refresh trade network
				_worldTradeSystem.RefreshTradeRoutes();
			}
		}

		// AIPlayer auto upgrade
		if (IsAIPlayer(parameters.playerId))
		{
			int32 upgradeCount = bld.upgrades().size();
			for (int32 i = 0; i < upgradeCount; i++) {
				if (!bld.IsUpgraded(i)) {
					bld.UpgradeInstantly(i);
				}
			}
		}

		return buildingId;
	}
	
	return -1;
}

void GameSimulationCore::PlaceDrag(FPlaceDrag parameters)
{
	_LOG(LogNetworkInput, " PlaceDrag pid:%d type:%d", parameters.playerId, parameters.placementType);
	
	TileArea area = parameters.area;
	auto placementType = static_cast<PlacementType>(parameters.placementType);
	//PUN_LOG("sim PlaceDrag: %d, %d, %d, %d", area.minX, area.minY, area.maxX, area.maxY);

	if (placementType == PlacementType::Gather) {
		int32 markCount = _treeSystem->MarkArea(parameters.playerId, area, false, parameters.harvestResourceEnum);
		playerOwned(parameters.playerId).alreadyDidGatherMark = true;

		QuestUpdateStatus(parameters.playerId, QuestEnum::GatherMarkQuest, markCount);
	}
	else if (placementType == PlacementType::GatherRemove) {
		_treeSystem->MarkArea(parameters.playerId, area, true, parameters.harvestResourceEnum);
	}

	/*
	 * Demolish
	 */
	else if (placementType == PlacementType::Demolish)
	{
		// Sound here since demolish needs to pass confirmation first before playing sound.
		_soundInterface->Spawn2DSound("UI", "PlaceBuilding", parameters.playerId);
		
		PUN_LOG("DragPlacement Demolish!!");
		area.EnforceWorldLimit();
		area.ExecuteOnArea_WorldTile2([&](WorldTile2 tile) 
		{
			// Delete buildings
			int32 buildingId = buildingIdAtTile(tile);
			if (buildingId != -1) 
			{
				Building& bld = building(buildingId);

				// Player's building or ForeignOwner
				if (bld.playerId() == parameters.playerId ||
					bld.foreignBuilder() == parameters.playerId)
				{
					// Don't demolish a burning building..
					if (bld.isOnFire()) {
						// TODO: Just do normal front popup remove
						PopupInfo* popupInfo = PopupToDisplay(parameters.playerId);
						//if (popupInfo == nullptr || PopupToDisplay(parameters.playerId)->body != "Cannot demolish a building on fire.") {
						//	AddPopup(parameters.playerId, "Cannot demolish a building on fire.");
						//}
						return;
					}


					/*
					 *  If there was a card in the building, put it in the hand
					 */
					auto& cardSys = cardSystem(parameters.playerId);
					if (bld.isEnum(CardEnum::Townhall))
					{
						if (!SimSettings::IsOn("CheatFastBuild")) {
							AddPopupToFront(parameters.playerId, LOCTEXT("CannotDemolishTownhall", "Cannot demolish the Townhall."), ExclusiveUIEnum::None, "PopupCannot");
							return;
						}
						
						// Check if we can add townhall's cards to hand
						auto& townManage = townManager(bld.townId());

						bool returnSuccessful = cardSys.ReturnBuildingSlotCardsToHand(townManage.cardsInTownhall());

						if (returnSuccessful) {
							townManage.ClearCardsFromTownhall();
						}
						else {
							AddPopupToFront(parameters.playerId,
								LOCTEXT("CardFullDemolitionFailed", "Card hand is full. Demolition failed."), 
								ExclusiveUIEnum::None, "PopupCannot"
							);
							return;
						}

						//std::vector<CardEnum> addedCards;
						
						//std::vector<CardStatus> slotCards = townManage.cardsInTownhall();
						//for (size_t i = 0; i < slotCards.size(); i++)
						//{
						//	if (cardSys.CanAddCardToBoughtHand(slotCards[i].cardEnum, 1)) {
						//		townManage.RemoveCardFromTownhall(i);
						//		
						//		addedCards.push_back(slotCards[i].cardEnum);
						//		cardSys.AddCardToHand2(slotCards[i].cardEnum);
						//	}
						//	else {
						//		// Remove the added cards from the Hand
						//		for (CardEnum addedCard : addedCards) {
						//			cardSys.RemoveCardsOld(addedCard, 1);
						//		}
						//		
						//		AddPopupToFront(parameters.playerId,
						//			LOCTEXT("CardFullDemolitionFailed", "Card hand is full. Demolition failed."), 
						//			ExclusiveUIEnum::None, "PopupCannot"
						//		);
						//		return;
						//	}
						//}
					}
					else
					{
						bool returnSuccessful = cardSys.ReturnBuildingSlotCardsToHand(bld.slotCards());

						if (returnSuccessful) {
							bld.ResetCardSlots();
						}
						else {
							AddPopupToFront(parameters.playerId,
								LOCTEXT("CardFullDemolitionFailed", "Card hand is full. Demolition failed."),
								ExclusiveUIEnum::None, "PopupCannot"
							);
							return;
						}
						
						//// Return cards to hand
						//std::vector<CardEnum> addedCards;
						//
						//std::vector<CardStatus> slotCards = bld.slotCards();
						//for (CardStatus card : slotCards)
						//{
						//	if (cardSys.CanAddCardToBoughtHand(card.cardEnum, 1)) 
						//	{
						//		addedCards.push_back(card.cardEnum);
						//		cardSys.AddCardToHand2(card.cardEnum);
						//	}
						//	else {
						//		// Remove the added cards from the Hand
						//		for (CardEnum addedCard : addedCards) {
						//			cardSys.RemoveCardsOld(addedCard, 1);
						//		}
						//		
						//		AddPopupToFront(parameters.playerId, 
						//			LOCTEXT("CardFullDemolitionFailed", "Card hand is full. Demolition failed."), 
						//			ExclusiveUIEnum::None, "PopupCannot"
						//		);
						//		return;
						//	}
						//}

						//bld.ResetCardSlots();
					}

					/*
					 * if this isn't a permanent building or Fort/Colony, return the card used to build this
					 */
					// If this building was just built less than 15 sec ago, return the card...
					//if (bld.buildingAge() < Time::TicksPerSecond * 15)
					CardEnum buildingEnum = bld.buildingEnum();
					if (!IsPermanentBuilding(parameters.playerId, buildingEnum) &&
						buildingEnum != CardEnum::Fort && 
						buildingEnum != CardEnum::ResourceOutpost)
					{
						if (cardSys.CanAddCardToBoughtHand(buildingEnum, 1)) {
							cardSys.AddCardToHand2(buildingEnum);
						} else {
							AddPopupToFront(parameters.playerId, 
								LOCTEXT("CardFullDemolitionFailed", "Card hand is full. Demolition failed."), 
								ExclusiveUIEnum::None, "PopupCannot");
							return;
						}
					}

					/*
					 * Spawn resource drops
					 */
					if (!SimSettings::IsOn("DemolishNoDrop") &&
						!PunSettings::TrailerSession)
					{
						std::vector<ResourceHolderInfo>& holderInfos = bld.holderInfos();
						int32 tileCount = 0;

						for (ResourceHolderInfo info : holderInfos) {
							int32 count = bld.tryResourceCount(info.resourceEnum);
							if (count > 0) {
								TileArea bldArea = bld.area();
								WorldTile2 dropTile(bldArea.minX + tileCount % bldArea.sizeX(),
									bldArea.minY + tileCount / bldArea.sizeX());
								resourceSystem(parameters.playerId).SpawnDrop(info.resourceEnum, count, dropTile);
								tileCount++;
							}
						}
					}

					/*
					 * Demolishing farm remove TileObj if farm is constructed
					 */
					if (buildingEnum == CardEnum::Farm && bld.isConstructed()) 
					{
						bld.subclass<Farm>().ClearAllPlants();

						// Change all drops
						auto& resourceSys = resourceSystem(bld.playerId());
						std::vector<DropInfo> drops = resourceSys.GetDropsFromArea_Pickable(bld.area(), true);
						for (DropInfo drop : drops) {
							resourceSys.SetHolderTypeAndTarget(drop.holderInfo, ResourceHolderType::Drop, 0);
						}
					}
					

					/*
					 * Keep storage demolition stat for quest
					 */
					statSystem(bld.townId()).AddStat(AccumulatedStatEnum::StoragesDestroyed);

					// Stop any sound
					soundInterface()->TryStopBuildingWorkSound(bld);
					
					_buildingSystem->RemoveBuilding(buildingId);

					AddDemolishDisplayInfo(bld.centerTile(), { bld.buildingEnum(), bld.area(), Time::Ticks() });
					//_regionToDemolishDisplayInfos[bld.centerTile().regionId()].push_back({ bld.buildingEnum(), bld.area(), Time::Ticks() });
				}
				// Regional buildings (Shrines etc.)
				//  Allow demolition for shrine in player's region.
				else if (IsRegionalBuilding(bld.buildingEnum()) &&
						tileOwnerTown(bld.centerTile()) == parameters.playerId)
				{
					_buildingSystem->RemoveBuilding(buildingId);

					AddDemolishDisplayInfo(bld.centerTile(), { bld.buildingEnum(), bld.area(), Time::Ticks() });
					//_regionToDemolishDisplayInfos[bld.centerTile().regionId()].push_back({ bld.buildingEnum(), bld.area(), Time::Ticks() });
				}
			}

			// Can delete critter building within territory
			int32 tileOwner = tileOwnerPlayer(tile);
			if (tileOwner == parameters.playerId)
			{
				// Critter building demolition
				DemolishCritterBuildingsIncludingFronts(tile, parameters.playerId);
			}

			// Can delete intercity road
			if (tileOwner == parameters.playerId ||
				tileOwner == -1)
			{
				// Delete road (non-construction)
				if (_overlaySystem.IsRoad(tile))
				{
					RoadTile roadTile = _overlaySystem.GetRoad(tile);
					_overlaySystem.RemoveRoad(tile);
					PUN_CHECK(IsFrontBuildable(tile));

					AddDemolishDisplayInfo(tile, { roadTile.isDirt ? CardEnum::DirtRoad : CardEnum::StoneRoad, TileArea(tile, WorldTile2(1, 1)), Time::Ticks() });
				}
			}
		});

		_worldTradeSystem.RefreshTradeRoutes();
	}

	//! Tile placement types
	else
	{
		CardEnum buildingEnum = CardEnum::None;
		function<bool(WorldTile2)> canBuild = [&] (WorldTile2 tile) { return true; };

		switch (placementType)
		{
		case PlacementType::DirtRoad:
		case PlacementType::IntercityRoad: {
			buildingEnum = CardEnum::DirtRoad; 
			canBuild = [&](WorldTile2 tile) { return IsFrontBuildable(tile) && !_overlaySystem.IsRoad(tile); };
			break;
		}
		case PlacementType::StoneRoad: {
			buildingEnum = CardEnum::StoneRoad;
			canBuild = [&](WorldTile2 tile) { return IsFrontBuildable(tile) && !_overlaySystem.IsRoad(tile); };
			break;
		}
		case PlacementType::Fence: {
			buildingEnum = CardEnum::Fence;
			canBuild = [&](WorldTile2 tile) { return IsBuildableForPlayer(tile, parameters.playerId); };
			break;
		}
		default:
			UE_DEBUG_BREAK();
		}


		auto tryBuild = [&](WorldTile2 tile)
		{
			PUN_LOG("Road tryBuild: %s", *tile.To_FString());
			
			DemolishCritterBuildingsIncludingFronts(tile, parameters.playerId);
			if (canBuild(tile)) {
				PUN_LOG("Road AddTileBuilding: %s", *tile.To_FString());
				_buildingSystem->AddTileBuilding(tile, buildingEnum, parameters.playerId);
				//_dropSystem.ForceRemoveDrop(tile);
			}

			// Stone road special case
			// Can override dirt road
			if (buildingEnum == CardEnum::StoneRoad) {
				if (IsFrontBuildable(tile)) {
					RoadTile roadTile = _overlaySystem.GetRoad(tile);
					if (roadTile.isValid() && roadTile.isDirt) {
						// Remove old dirt road and add this new one
						_overlaySystem.RemoveRoad(tile);
						_buildingSystem->AddTileBuilding(tile, buildingEnum, parameters.playerId);
					}
				}
			}
		};

		// For road, use roadPath if possible
		const TArray<int32>& path = parameters.path;
		if (parameters.path.Num() > 0)
		{
			if (placementType == PlacementType::IntercityRoad)
			{
				vector<int32> foreignPlayerIds;
				
				for (int32 i = 0; i < path.Num(); i++) 
				{
					WorldTile2 tile(path[i]);

					DemolishCritterBuildingsIncludingFronts(tile, parameters.playerId);
					
					if (IsFrontBuildable(tile) && !IsRoadTile(tile)) 
					{
						_treeSystem->ForceRemoveTileObj(tile, false);
						overlaySystem().AddRoad(tile, true, true);

						ChangeMoney(parameters.playerId, -IntercityRoadTileCost);

						// For road, also refresh the grass since we want it to be more visible
						SetNeedDisplayUpdate(DisplayClusterEnum::Trees, tile.regionId(), true);

						int32 foreignPlayerId = tileOwnerPlayer(tile);
						if (foreignPlayerId != -1 &&
							foreignPlayerId != parameters.playerId) {
							CppUtils::TryAdd(foreignPlayerIds, foreignPlayerId);
						}
					}
				}

				for (int32 foreignPlayerId : foreignPlayerIds) {
					AddPopup(foreignPlayerId, 
						FText::Format(LOCTEXT("IntercityRoadBuildWarning_Pop", "{0} built an intercity road in your territory."), townNameT(parameters.playerId))
					);
				}
				
				return;
			}

			// Trailer instant road
			if (PunSettings::TrailerMode())
			{
				for (int32 i = 0; i < path.Num(); i++) {
					WorldTile2 tile(path[i]);
					if (IsFrontBuildable(tile) && !_overlaySystem.IsRoad(tile)) {
						TileArea clearArea(tile, 1);
						_treeSystem->ForceRemoveTileObjArea(clearArea); // Remove 3x3
						overlaySystem().AddRoad(tile, true, true);

						// Road construct smoke
						//PUN_LOG("Road smoke %s", ToTChar(tile.ToString()));
						AddDemolishDisplayInfo(tile, { CardEnum::DirtRoad, TileArea(tile, WorldTile2(1, 1)), Time::Ticks() });

						// For road, also refresh the grass since we want it to be more visible
						SetNeedDisplayUpdate(DisplayClusterEnum::Trees, tile.regionId(), true);
					}
				}
				return;
			}

			// Dirt/Stone Road
			for (int32 i = 0; i < path.Num(); i++) {
				WorldTile2 tile(path[i]);
				tryBuild(tile);

				// For road, also refresh the grass since we want it to be more visible
				SetNeedDisplayUpdate(DisplayClusterEnum::Trees, tile.regionId(), true);
			}
			
			return;
		}

		//_treeSystem->ForceRemoveTileObjArea(area);
		area.ExecuteOnArea_WorldTile2([&](WorldTile2 tile) {
			tryBuild(tile);
		});

		TileArea area2 = parameters.area2;
		if (!area2.isInvalid()) {
			//_treeSystem->ForceRemoveTileObjArea(area2);
			area2.ExecuteOnArea_WorldTile2([&](WorldTile2 tile) {
				tryBuild(tile);
			});
		}
	}
}

void GameSimulationCore::JobSlotChange(FJobSlotChange command)
{
	_LOG(LogNetworkInput, " JobSlotChange pid:%d id:%d, %d", command.playerId, command.buildingId, command.allowedOccupants);

	PUN_ENSURE(IsValidBuilding(command.buildingId), return);
	
	building(command.buildingId).ChangeAllowedOccupants(command.allowedOccupants);

	RefreshJobDelayed(command.playerId);
}

void GameSimulationCore::SetAllowResource(FSetAllowResource command)
{
	// Guard against invalid building
	if (!IsValidBuilding(command.buildingId)) {
		return;
	}
	
	Building& bld = building(command.buildingId);
	if (IsHouse(bld.buildingEnum())) {
		int32 townId = bld.townId();
		if (IsValidTown(townId)) {
			townManager(bld.townId()).SetHouseResourceAllow(command.resourceEnum, command.allowed);
		}
	}
	else if (IsStorage(bld.buildingEnum()))
	{
		StorageBase& storage = bld.subclass<StorageBase>();
		if (command.isExpansionCommand) 
		{
			if (command.resourceEnum == ResourceEnum::Food) {
				storage.expandedFood = command.expanded;
			}
			else if (command.resourceEnum == ResourceEnum::Luxury) {
				storage.expandedLuxury = command.expanded;
			}
		}
		else {
			if (storage.isConstructed()) {
				storage.SetHolderTypeAndTarget(command.resourceEnum, command.allowed ? storage.defaultHolderType(command.resourceEnum) : ResourceHolderType::Provider, 0);

				// Refresh storage after setting allowed resource, so that citizens taking the resources to this storage will be reset.
				resourceSystem(command.playerId).ResetHolderReservers(storage.holderInfo(command.resourceEnum));
			}
			else {
				// Not yet constructed, queue the checkState to apply once construction finishes
				if (command.resourceEnum == ResourceEnum::Food)
				{
					for (ResourceEnum resourceEnumLocal : StaticData::FoodEnums) {
						storage.queuedResourceAllowed[static_cast<int>(resourceEnumLocal)] = command.allowed;
					}
				}
				else if (command.resourceEnum == ResourceEnum::Luxury)
				{
					ExecuteOnLuxuryResources([&](ResourceEnum resourceEnumLocal) {
						storage.queuedResourceAllowed[static_cast<int>(resourceEnumLocal)] = command.allowed;
					});
				}
				else if (command.resourceEnum == ResourceEnum::None) {
					for (int32 i = 0; i < ResourceEnumCount; i++) {
						storage.queuedResourceAllowed[i] = command.allowed;
					}
				}
				else
				{
					storage.queuedResourceAllowed[static_cast<int>(command.resourceEnum)] = command.allowed;
				}
			}

			// Market also set target
			if (bld.isEnum(CardEnum::Market) &&
				command.target != -1) 
			{
				bld.subclass<Market>().SetMarketTarget(command.resourceEnum, command.target);
			}
		}
	}
	else {
		UE_DEBUG_BREAK();
	}
}

void GameSimulationCore::SetPriority(FSetPriority command)
{
	_LOG(LogNetworkInput, " SetPriority pid:%d id:%d, %d", command.playerId, command.buildingId, command.priority);

	PUN_ENSURE(IsValidBuilding(command.buildingId), return);
	
	building(command.buildingId).SetBuildingPriority(static_cast<PriorityEnum>(command.priority));

	RefreshJobDelayed(command.playerId);
}

void GameSimulationCore::SetTownPriority(FSetTownPriority command)
{
	_LOG(LogNetworkInput, " SetTownPriority");

	auto& town = townManager(command.townId);
	town.targetLaborerHighPriority = command.laborerPriority;
	town.targetBuilderHighPriority = command.builderPriority;
	town.targetRoadMakerHighPriority = command.roadMakerPriority;
	
	town.targetLaborerCount = command.targetLaborerCount;
	town.targetBuilderCount = command.targetBuilderCount;
	town.targetRoadMakerCount = command.targetRoadMakerCount;

	town.RefreshJobDelayed();
}

void GameSimulationCore::SetGlobalJobPriority(FSetGlobalJobPriority command)
{
	PUN_ENSURE(command.townId != -1, return);
	
	_LOG(LogNetworkInput, "[%d] SetGlobalJobPriority", tempVariable);
	townManager(command.townId).SetGlobalJobPriority(command);
}

void GameSimulationCore::GenericCommand(FGenericCommand command)
{
	_LOG(LogNetworkInput, "[%d] GenericCommand callbackEnum:%d genericCommandType%d var:%d %d %d %d %d", tempVariable, static_cast<int32>(command.callbackEnum), command.genericCommandType,
		command.intVar1, command.intVar2, command.intVar3, command.intVar4, command.intVar5);

	if (command.callbackEnum == CallbackEnum::EstablishTradeRoute)
	{
		worldTradeSystem().TryEstablishTradeRoute(command);
		return;
	}
	
	if (command.callbackEnum == CallbackEnum::CancelTradeRoute)
	{
		worldTradeSystem().TryCancelTradeRoute(command);
		return;
	}

	if (command.callbackEnum == CallbackEnum::ForeignBuildingAllow)
	{
		if (Building* bld = buildingPtr(command.intVar1)) {
			if (bld->foreignBuilder() != -1 && 
				!bld->isForeignBuildingApproved())
			{
				bld->ApproveForeignBuilder();
			}
		}
		return;
	}

	if (command.callbackEnum == CallbackEnum::ForeignBuildingDisallow)
	{
		int32 buildingId = command.intVar1;
		
		if (Building* bld = buildingPtr(buildingId))
		{
			int32 foreignBuilderId = bld->foreignBuilder();
			
			if (foreignBuilderId != -1 &&
				!bld->isForeignBuildingApproved())
			{
				bool returnSuccessful = cardSystem(foreignBuilderId).ReturnBuildingSlotCardsToHand(bld->slotCards());

				if (returnSuccessful) {
					bld->ResetCardSlots();
				}
				else {
					AddPopupToFront(command.playerId,
						LOCTEXT("CardFullDemolitionFailed", "Card hand is full. Demolition failed."),
						ExclusiveUIEnum::None, "PopupCannot"
					);
					return;
				}

				AddDemolishDisplayInfo(bld->centerTile(), { bld->buildingEnum(), bld->area(), Time::Ticks() });
				
				_buildingSystem->RemoveBuilding(buildingId);

				AddPopupToFront(foreignBuilderId, FText::Format(
					LOCTEXT("PlayerForeignBuildingRejected_Pop", "{0} rejected your request to build {1}."),
					townNameT(bld->townId()),
					bld->buildingInfo().name
				));
			}
		}
		
		return;
	}

	// TODO: move this out?
	if (command.callbackEnum != CallbackEnum::None)
	{
		if (command.callbackEnum == CallbackEnum::DeclareFriendship) {
			aiPlayerSystem(command.intVar1).DeclareFriendship(command.playerId);
		}
		else if (command.callbackEnum == CallbackEnum::MarryOut) {
			aiPlayerSystem(command.intVar1).MarryOut(command.playerId);
		}
		else if (command.callbackEnum == CallbackEnum::EditableNumberSetOutputTarget) {
			if (command.intVar1 != -1) {
				townManager(command.townId).SetOutputTarget(static_cast<ResourceEnum>(command.intVar1), command.intVar2);
			}
		}
		else if (command.callbackEnum == CallbackEnum::EditableNumberSetHotelFeePerVisitor) {
			if (IsValidBuilding(command.intVar1)) {
				building(command.intVar1).subclass<Hotel>(CardEnum::Hotel).SetFeePerVisitor(command.intVar2);
			}
		}
		else if (command.callbackEnum == CallbackEnum::QuickBuild) {
			if (command.intVar1 != -1) 
			{
				if (Building* bld = buildingPtr(command.intVar1))
				{
					// Quick Build All for Road
					if (IsRoad(bld->buildingEnum()))
					{
						std::vector<int32> roadIds = buildingIds(bld->townId(), bld->buildingEnum());

						// Sort road Ids by closest
						WorldTile2 origin = bld->centerTile();
						std::sort(roadIds.begin(), roadIds.end(), [&](int32 a, int32 b) {
							int32 distA = WorldTile2::Distance(building(a).centerTile(), origin);
							int32 distB = WorldTile2::Distance(building(b).centerTile(), origin);
							return distA < distB;
						});
						
						int32 quickBuildAllCost = 0;
						int32 roadTilesBuilt = 0;
						for (int32 roadId : roadIds) 
						{
							Building& curBld = building(roadId);
							int32 quickBuildCost = curBld.GetQuickBuildCost();
							if (moneyCap32(command.playerId) >= quickBuildCost)
							{
								WorldTile2 tile = curBld.centerTile();
								_treeSystem->ForceRemoveTileObj(tile, false);
								SetNeedDisplayUpdate(DisplayClusterEnum::Trees, tile.regionId(), true); // For road, also refresh the grass since we want it to be more visible
								
								curBld.FinishConstruction();
								
								ChangeMoney(command.playerId, -quickBuildCost);
								
								roadTilesBuilt++;
								quickBuildAllCost += quickBuildCost;
							}
							else {
								break;
							}
						}

						if (roadTilesBuilt < roadIds.size()) {
							// Not enough money to finish all the road warn about it
							AddPopup(command.playerId,
								FText::Format(LOCTEXT("QuickBuildAllNotEnoughMoney_Pop", "Not enough money to build all Road Construction of this type. Only {0} Road Tiles was built with {1}<img id=\"Coin\"/>."), 
									TEXT_NUM(roadTilesBuilt),
									TEXT_NUM(quickBuildAllCost)
								)
							);
						}
					}
					else
					{
						if (!bld->isConstructed() && moneyCap32(command.playerId) >= bld->GetQuickBuildCost())
						{
							ChangeMoney(command.playerId, -bld->GetQuickBuildCost());
							_buildingSystem->AddQuickBuild(command.intVar1);

							//// Remove the tree at the gate tile to prevent inaccessibility
							//bld->frontArea().ExecuteOnArea_WorldTile2([&](WorldTile2 tile) {
							//	treeSystem().ForceRemoveTileObj(tile.tileId(), false);
							//});

							bld->InstantClearArea();
							bld->FinishConstructionResourceAndWorkerReset();
							bld->SetAreaWalkable();
						}
					}
				}
			}
		}
		else if (command.callbackEnum == CallbackEnum::AddAnimalRanch) {
			if (command.intVar1 != -1)
			{
				if (Building* bld = buildingPtr(command.intVar1))
				{
					if (IsRanch(bld->buildingEnum()))
					{
						Ranch& ranch = bld->subclass<Ranch>();
						int32 animalCost = ranch.animalCost();

						if (ranch.openAnimalSlots() > 0 &&
							animalCost <= moneyCap32(ranch.playerId()))
						{
							UnitEnum animalEnum = ranch.GetAnimalEnum();
							ranch.AddAnimalOccupant(animalEnum, GetUnitInfo(animalEnum).minBreedingAgeTicks);
							ChangeMoney(ranch.playerId(), -animalCost);
						}
					}
				}
			}
		}
		else if (command.callbackEnum == CallbackEnum::BudgetAdjust)
		{
			int32 buildingId = command.intVar1;
			bool isBudgetOrTime = command.intVar2;
			int32 level = command.intVar3;

			if (Building* bld = buildingPtr(buildingId))
			{
				auto adjustBuilding = [&](Building* curBuilding)
				{
					if (isBudgetOrTime) {
						curBuilding->SetBudgetLevel(level);
					} else {
						curBuilding->SetWorkTimeLevel(level);
					}
				};
				
				bool isShiftDown = command.intVar4;
				if (isShiftDown)
				{
					// Adjust all buildings of the same type
					const std::vector<int32>& bldIds = buildingIds(bld->townId(), bld->buildingEnum());
					for (int32 bldId : bldIds) {
						adjustBuilding(buildingPtr(bldId));
					}
				}
				else
				{
					adjustBuilding(bld);
				}
			}
		}
		else if (IsSpyCallback(command.callbackEnum))
		{
			int32 buildingId = command.intVar1;
			if (buildingIsAlive(buildingId) && building(buildingId).isEnum(CardEnum::House)) {
				building<House>(buildingId).ExecuteSpyCommand(command);
			}
		}
		else if (command.callbackEnum == CallbackEnum::CaptureUnit)
		{
			if (unitAlive(UnitFullId(command.intVar1, command.intVar2)))
			{
				auto& unit = unitAI(command.intVar1);

				int32 animalPrice = GetCaptureAnimalPrice(command.playerId, unit.unitEnum(), unit.unitTile());
				
				if (animalPrice <= moneyCap32(command.playerId))
				{
					CardEnum cardEnum = GetAnimalCardEnumFromUnitEnum(unit.unitEnum());

					if (cardSystem(command.playerId).TryAddCardToBoughtHand(cardEnum))
					{
						unit.Die();
						AddPopupToFront(command.playerId,
							FText::Format(
								LOCTEXT("CapturedAnimal_Succeed", "Captured {0}."),
								GetUnitInfo(unit.unitEnum()).name
							)
						);
					}
					else {
						AddPopupToFront(command.playerId,
							LOCTEXT("CapturedAnimal_NoSpace", "Cannot Capture the Unit. Your Card Hand is full."),
							ExclusiveUIEnum::None, "PopupCannot"
						);
					}
				}
				else {
					AddPopupToFront(command.playerId,
						LOCTEXT("CapturedAnimal_NoMoney", "Cannot Capture the Unit. Not enough money."),
						ExclusiveUIEnum::None, "PopupCannot"
					);
				}
			}
		}
		else if (IsAutoTradeCallback(command.callbackEnum))
		{
			if (IsValidTown(command.townId)) {
				townManager(command.townId).ExecuteAutoTradeCommand(command);
			}
		}
		
		return;
	}

	if (command.genericCommandType == FGenericCommand::Type::SendGift)
	{
		int32 giverPlayerId = command.intVar1;
		int32 targetTownId = command.intVar2;
		int32 targetPlayerId = townPlayerId(targetTownId);
		TradeDealStageEnum dealStageEnum = static_cast<TradeDealStageEnum>(command.intVar5);
		
		
		PopupInfo popupInfo(targetPlayerId,
			FText::Format(
				LOCTEXT("TradeDeal_TakerPop", "{0} offered you a deal."), 
				playerNameT(giverPlayerId)
			),
			{
				LOCTEXT("Show the offer", "Show the offer"),
				LOCTEXT("Refuse the offer", "Refuse the offer")
			},
			PopupReceiverEnum::ShowTradeDeal, false, ""
		);
		popupInfo.forcedSkipNetworking = true;
		
		// Convert command to poupInfo
		popupInfo.replyVar1 = command.intVar1;
		popupInfo.replyVar2 = command.intVar2;
		popupInfo.replyVar3 = command.intVar3;
		popupInfo.replyVar4 = command.intVar4;
		popupInfo.replyVar5 = command.intVar5; // deal stage
		
		popupInfo.array1 = command.array1;
		popupInfo.array2 = command.array2;
		popupInfo.array3 = command.array3;
		popupInfo.array4 = command.array4;
		popupInfo.array5 = command.array5;
		popupInfo.array6 = command.array6;
		popupInfo.array7 = command.array7;
		popupInfo.array8 = command.array8;

		//! Gifting/AcceptDeal, just process the deal without asking
		if (dealStageEnum == TradeDealStageEnum::Gifting ||
			dealStageEnum == TradeDealStageEnum::AcceptDeal)
		{
			ProcessTradeDeal(popupInfo);
			return;
		}
		
		//! AI deal handling
		if (IsAIPlayer(targetPlayerId))
		{
			// AI will try to ask for ask much money to make it fair...
			TradeDealSideInfo sourceDealInfo;
			TradeDealSideInfo targetDealInfo; // AI

			UnpackPopupInfoToTradeDealInfo(popupInfo, sourceDealInfo, targetDealInfo);

			int32 aiMoneyRequest = GetTradeDealValue(targetDealInfo) - GetTradeDealValue(sourceDealInfo);

			if (aiMoneyRequest > 0) {
				// Swap deal side.. AI requests more money
				sourceDealInfo.moneyAmount += aiMoneyRequest;
				std::shared_ptr<FGenericCommand> counterOfferCommand = PackTradeDealInfoToCommand(targetDealInfo, sourceDealInfo, TradeDealStageEnum::ExamineCounterOfferDeal);
				GenericCommand(*counterOfferCommand);
			}
			else {
				popupInfo.replyVar5 = static_cast<int32>(TradeDealStageEnum::AcceptDeal);
				ProcessTradeDeal(popupInfo);
			}
			return;
		}

		
		//! Say this is a counter offer
		if (dealStageEnum == TradeDealStageEnum::ExamineCounterOfferDeal)
		{	
			popupInfo.body = FText::Format(
				LOCTEXT("TradeDeal_CounterOfferPop", "{0} gave you a counter offer."),
				playerNameT(giverPlayerId)
			);
		}

		
		AddPopup(popupInfo);

		
		
		//AddPopup(townPlayerId(targetTownId), 
		//	FText::Format(
		//		LOCTEXT("Gifted_GiverPop", "You gifted {0} with {1} {2}."), playerNameT(targetPlayerId), TEXT_NUM(amount), ResourceNameT(resourceEnum)
		//	),

		//);

		
		
		//ResourceEnum resourceEnum = static_cast<ResourceEnum>(command.intVar2);
		//int32 amount = command.intVar3;

		//if (resourceEnum == ResourceEnum::Money)
		//{
		//	if (moneyCap32(giverPlayerId) < amount) {
		//		AddPopupToFront(giverPlayerId, 
		//			LOCTEXT("NotEnoughMoneyToGive", "Not enough <img id=\"Coin\"/> to give out."), 
		//			ExclusiveUIEnum::GiftResourceUI, "PopupCannot");
		//		return;
		//	}

		//	ChangeMoney(giverPlayerId, -amount);
		//	ChangeMoney(targetPlayerId, amount);
		//	AddPopup(giverPlayerId, 
		//		FText::Format(LOCTEXT("GiftedMoney_GiverPop", "You gifted {0} with {1}<img id=\"Coin\"/>."), playerNameT(targetPlayerId), TEXT_NUM(amount))
		//	);

		//	// To Gift Receiver
		//	if (IsAIPlayer(giverPlayerId)) {
		//		AddPopup(targetPlayerId, 
		//			FText::Format(LOCTEXT("GiftedMoneyAI_TargetPop", "{0} gifted you with {1}<img id=\"Coin\"/> for good relationship."), playerNameT(giverPlayerId), TEXT_NUM(amount))
		//		);
		//	}
		//	else {
		//		AddPopup(targetPlayerId, 
		//			FText::Format(LOCTEXT("GiftedMoney_TargetPop", "{0} gifted you with {1}<img id=\"Coin\"/>."), playerNameT(giverPlayerId), TEXT_NUM(amount))
		//		);
		//	}

		//	ChangeRelationshipModifier(targetPlayerId, giverPlayerId, RelationshipModifierEnum::YouGaveUsGifts, amount / GoldToRelationship);
		//}
		//else
		//{
		//	if (resourceCountTown(giverPlayerId, resourceEnum) < amount) {
		//		AddPopupToFront(giverPlayerId, 
		//			LOCTEXT("NotEnoughResourceToGive", "Not enough resource to give out."), 
		//			ExclusiveUIEnum::GiftResourceUI, "PopupCannot");
		//		return;
		//	}

		//	if (!resourceSystem(targetPlayerId).CanAddResourceGlobal(resourceEnum, amount)) {
		//		AddPopup(giverPlayerId, 
		//			LOCTEXT("NotEnoughStorageAtTarget_Pop", "Not enough storage space in target city.")
		//		);
		//		return;
		//	}
		//
		//	resourceSystem(giverPlayerId).RemoveResourceGlobal(resourceEnum, amount);
		//	resourceSystem(targetPlayerId).AddResourceGlobal(resourceEnum, amount, *this);
		//	AddPopup(giverPlayerId, 
		//		FText::Format(LOCTEXT("Gifted_GiverPop", "You gifted {0} with {1} {2}."), playerNameT(targetPlayerId), TEXT_NUM(amount), ResourceNameT(resourceEnum))
		//	);
		//	AddPopup(targetPlayerId, 
		//		FText::Format(LOCTEXT("Gifted_TargetPop", "{0} gifted you with {1} {2}."), playerNameT(giverPlayerId), TEXT_NUM(amount), ResourceNameT(resourceEnum))
		//	);

		//	ChangeRelationshipModifier(targetPlayerId, giverPlayerId, RelationshipModifierEnum::YouGaveUsGifts, (amount * price(resourceEnum)) / GoldToRelationship);
		//}
		
		return;
	}

	if (command.genericCommandType == FGenericCommand::Type::SendImmigrants)
	{	
		int32 startTownId = command.intVar1;
		int32 endTownId = command.intVar2;

		auto& townManage = townManager(startTownId);
		const auto& adultIds = townManage.adultIds();
		const auto& childIds = townManage.childIds();
		
		int32 adultsTargetCount = std::min(command.intVar3, static_cast<int32>(adultIds.size()));
		int32 childrenTargetCount = std::min(command.intVar4, static_cast<int32>(childIds.size()));

		WorldTile2 startTownGate = GetTownhallGate(startTownId);
		WorldTile2 endTownGate = GetTownhallGate(endTownId);

		bool sendByLand = false;

		std::vector<uint32_t> path;
		bool succeed = pathAI()->FindPathRoadOnly(startTownGate.x, startTownGate.y, endTownGate.x, endTownGate.y, path);
		if (succeed) {
			sendByLand = true;
		}


		int32 startPortId = -1;
		int32 endPortId = -1;

		if (!sendByLand)
		{
			FindBestPathWater(startTownId, endTownId, startTownGate, startPortId, endPortId);

			if (startPortId == -1) 
			{
				AddPopupToFront(command.playerId,
					LOCTEXT("SendImmigrants_RequireConnection", "Failed to send immigrants.<space>Require road or port connection between towns.")
				);
				return;
			}
		}

		AddPopupToFront(command.playerId, FText::Format(
			LOCTEXT("SendImmigrants_TargetPop", "Sent immigrants to {2}.<bullet>Adults: {0}</><bullet>Children: {1}</>"),
			TEXT_NUM(adultsTargetCount),
			TEXT_NUM(childrenTargetCount),
			GetTownhall(endTownId).townNameT())
		);

		auto replaceAndSendUnitToNewTown = [&](int32 unitId)
		{
			auto& unit = unitAI(unitId);
			WorldAtom2 atom = unit.unitAtom();
			int32 ageTicks = unit.age();
			unit.Die();

			int32 newUnitId = AddUnit(UnitEnum::Human, endTownId, atom, ageTicks);
			auto& newUnit = unitAI(newUnitId).subclass<HumanStateAI>(UnitEnum::Human);

			if (sendByLand) {
				newUnit.SendToTownLand(startTownId, endTownId);
			} else {
				newUnit.SendToTownWater(startTownId, endTownId, startPortId, endPortId);
			}
		};
		
		for (int32 i = 0; i < adultsTargetCount; i++) {
			replaceAndSendUnitToNewTown(adultIds[i]);
		}
		for (int32 i = 0; i < childrenTargetCount; i++) {
			replaceAndSendUnitToNewTown(childIds[i]);
		}
		
		return;
	}
}

void GameSimulationCore::ChangeName(FChangeName command)
{
	UE_LOG(LogNetworkInput, Log, TEXT(" ChangeName[%d] %s"), command.objectId, *command.name);

	if (isValidBuildingId(command.objectId)) {
		Building& bld = building(command.objectId);
		
		if (bld.isEnum(CardEnum::Townhall)) {
			bld.subclass<TownHall>().SetTownName(command.name);
		}
	}
}

void GameSimulationCore::SendChat(FSendChat command)
{
	_LOG(PunSync, "Sim SendChat[%d] %s", command.playerId,  *command.message);
	_chatSystem.AddMessage(command);
}

void GameSimulationCore::TradeResource(FTradeResource command)
{
	_LOG(LogNetworkInput, " TradeResource");

	PUN_ENSURE(IsValidBuilding(command.objectId), return);
	
	Building& bld = building(command.objectId);
	if (!IsTradingPostLike(bld.buildingEnum()) && 
		!bld.isEnum(CardEnum::Townhall)) {
		return; // Message may be so slow that the building was demolished and replaced...
	}

	if (bld.isEnum(CardEnum::Townhall))
	{
		int32 tradingFeePercent = command.isIntercityTrade ? 0 : bld.baseTradingFeePercent();

		int32 exportMoney100 = 0;
		int32 importMoney100 = 0;
		TradeBuilding::ExecuteTrade(command, tradingFeePercent, bld.centerTile(), this, true, exportMoney100, importMoney100);
	}
	else
	{
		auto tradingPost = static_cast<TradeBuilding*>(&bld);
		if (!tradingPost->CanTrade()) {
			return;
		}
		tradingPost->UsedTrade(command);
	}
}

void GameSimulationCore::SetIntercityTrade(FSetIntercityTrade command)
{
	if (!HasTownhall(command.playerId)) { // For Direct Command on AIs
		return;
	}
	
	//if (command.buildingIdToEstablishTradeRoute != -1) {
	//	if (command.isCancelingTradeRoute) {
	//		//worldTradeSystem().TryCancelTradeRoute(command);
	//	} else {
	//		//worldTradeSystem().TryEstablishTradeRoute(command);
	//	}
	//}
	//else {
	//	worldTradeSystem().SetIntercityTradeOffers(command);
	//}
}

void GameSimulationCore::UpgradeBuilding(FUpgradeBuilding command)
{
	PUN_ENSURE(IsValidBuilding(command.buildingId), return);

	Building* bld = &(building(command.buildingId));

	_LOG(LogNetworkInput, " UpgradeBuilding: pid:%d id:%d bldType:%s upgradeType:%d isShiftDown:%d", command.playerId, command.buildingId, *bld->buildingInfo().nameF(), command.upgradeType, command.isShiftDown);

	// Townhall special case
	if (bld->isEnum(CardEnum::Townhall))
	{
		auto& townhall = building(command.buildingId).subclass<TownHall>();
		if (command.upgradeType == 0) {
			auto unlockSys = unlockSystem(command.playerId);
			if (unlockSys->GetEra() < townhall.townhallLvl + 1) {
				AddPopup(command.playerId, LOCTEXT("TownhallUpgradeRequireEra", "Research your way to the next Era to unlock the Townhall Upgrade."));
			}
			
			if (townhall.townhallLvl < townhall.GetMaxUpgradeLvl() &&
				townhall.HasEnoughUpgradeMoney()) 
			{
				townhall.UpgradeTownhall();
			}
		}
	}
	// Other buildings
	else
	{
		ResourceEnum neededResource = ResourceEnum::None;
		int32 upgradedCount = 0;

		// Guard against bad upgrade
		if (0 <= command.upgradeType && command.upgradeType < bld->upgrades().size())
		{
			// Shift Upgrade
			if (command.isShiftDown)
			{
				int32 upgradedLevelCount = -1;
				
				// Upgrade the clicked building first to ensure it gets upgraded
				if (bld->UpgradeBuilding(command.upgradeType, false, neededResource)) {
					upgradedCount++;

					const BuildingUpgrade& upgrade = bld->GetUpgrade(command.upgradeType);
					if (upgrade.isLevelUpgrade()) {
						upgradedLevelCount = upgrade.upgradeLevel - 1;
					}
				}

				// Try to upgrade as many buildings in the town as possible when using shift
				const std::vector<int32>& bldIds = buildingIds(bld->townId(), bld->buildingEnum());
				for (int32 bldId : bldIds) 
				{
					// Don't upgrade, if this building isn't the same level as the Shift-click origin building
					Building& curBuilding = building(bldId);
					if (!curBuilding.isConstructed()) {
						continue;
					}
					
					const BuildingUpgrade& upgrade = curBuilding.GetUpgrade(command.upgradeType);
					if (upgrade.isEraUpgrade() &&
						upgrade.upgradeLevel != upgradedLevelCount)
					{
						continue;
					}
					
					if (curBuilding.UpgradeBuilding(command.upgradeType, false, neededResource)) {
						upgradedCount++;
					}
				}

				if (upgradedCount > 0)
				{
					BuildingUpgrade upgrade = bld->upgrades()[command.upgradeType];

					TArray<FText> args;
					ADDTEXT_NAMED_(LOCTEXT("ShiftUpgrade_Pop", "Upgraded {UpgradeName} on {UpgradedBuildingCount} {BuildingName}."),
						TEXT("UpgradeName"), upgrade.name,
						TEXT("UpgradedBuildingCount"), TEXT_NUM(upgradedCount),
						TEXT("BuildingName"), bld->buildingInfo().name
					);

					if (neededResource == ResourceEnum::Money) {
						ADDTEXT_NAMED_(LOCTEXT("ShiftUpgradeWarnMoney_Pop", "<space>Note: Not enough Money to upgrade all {BuildingName}."),
							TEXT("BuildingName"), bld->buildingInfo().name
						);
					}
					else if (neededResource != ResourceEnum::None) {
						ADDTEXT_NAMED_(LOCTEXT("ShiftUpgradeWarnResource_Pop", "<space>Note: Not enough {ResourceName} to upgrade all {BuildingName}."),
							TEXT("ResourceName"), ResourceNameT(neededResource),
							TEXT("BuildingName"), bld->buildingInfo().name
						);
					}

					soundInterface()->Spawn2DSound("UI", "UpgradeBuilding", command.playerId, bld->centerTile());

					AddPopup(command.playerId, JOINTEXT(args));
				}
				else {
					// Invalid Shift-Upgrade, call single upgrade to show the Popup why the Upgrade won't go through
					bld->UpgradeBuilding(command.upgradeType, true, neededResource);
				}
			}
			// Single Upgrade
			else
			{
				// Note: If single UpgradeBuilding succeed, we only need sound (no popup)
				bld->UpgradeBuilding(command.upgradeType, true, neededResource);
			}

		}
	}
}

void GameSimulationCore::ChangeWorkMode(FChangeWorkMode command)
{
	PUN_ENSURE(IsValidBuilding(command.buildingId), return);
	
	Building& bld = building(command.buildingId);

	_LOG(LogNetworkInput, " ChangeWorkMode %d %s enumInt:%d", command.buildingId, *(bld.buildingInfo().nameF()), command.enumInt);

	// Special case: Forester
	if (bld.isEnum(CardEnum::Forester) &&
		command.intVar1 != -1)
	{
		if (command.intVar1 == 0) {
			bld.subclass<Forester>().plantingEnum = static_cast<CutTreeEnum>(command.intVar2);
		} else {
			bld.subclass<Forester>().cuttingEnum = static_cast<CutTreeEnum>(command.intVar2);
		}
	}
	else if (bld.isEnum(CardEnum::Farm))
	{
		Farm& farm = bld.subclass<Farm>();

		//PUN_LOG("ChangeWorkMode id:%d, from: %s, to: %s", command.buildingId, 
		//	*FString(PlantInfos[(int)farm->currentPlantEnum].name.c_str()),
		//	*FString(PlantInfos[command.enumInt].name.c_str()));
		PUN_LOG("ChangeWorkMode id:%d, from: %s, to: %s", command.buildingId,
			*TreeInfos[(int)farm.currentPlantEnum].name.ToString(),
			*TreeInfos[command.enumInt].name.ToString());

		farm.currentPlantEnum = static_cast<TileObjEnum>(command.enumInt);
	}
	else if (bld.hasWorkModes()) // BuildingHasDropdown(bld.buildingEnum()))
	{
		PUN_LOG("ChangeBld id:%d, from: %s, to: %s", command.buildingId,
				*bld.workMode().name.ToString(),
				*bld.workModes[command.enumInt].name.ToString());

		PUN_ENSURE(command.enumInt < bld.workModes.size(), return);
		
		bld.ChangeWorkMode(bld.workModes[command.enumInt]);
	}
	else if (bld.isEnum(CardEnum::Townhall))
	{	
		// Is owner ... show normal tax
		if (bld.townId() == command.playerId) 
		{
			townManager(bld.townId()).taxLevel = command.enumInt;
			RecalculateTaxDelayedTown(bld.townId());
			return;
		}

		// Vassal Tax adjustment...
		//// Another player made the change, that player is lord, and this is vassal
		//const std::vector<int32_t>& vassalPlayerIds = playerOwn.vassalPlayerIds();
		//for (int32_t vassalPlayerId : vassalPlayerIds) {
		//	if (bld.playerId() == vassalPlayerId) {
		//		playerOwned(vassalPlayerId).vassalTaxLevel = command.enumInt;
		//		RecalculateTaxDelayed(command.playerId);
		//		return;
		//	}
		//}
	}
	else if (bld.isEnum(CardEnum::TradingCompany))
	{
		// Update trading company values
		auto& tradingCompany = bld.subclass<TradingCompany>();
		tradingCompany.activeResourceEnum = static_cast<ResourceEnum>(command.intVar1);
		tradingCompany.isImport = static_cast<bool>(command.intVar2);
		tradingCompany.targetAmount = std::max(0, command.intVar3);;
		tradingCompany.ResetTradeRetryTick();

		if (tradingCompany.activeResourceEnum != ResourceEnum::None) {
			tradingCompany.needTradingCompanySetup = false;
		}
	}
	else if (bld.isEnum(CardEnum::ShippingDepot))
	{
		// Update trading company values
		auto& shippingDepot = bld.subclass<ShippingDepot>();
		shippingDepot.resourceEnums[command.intVar1] = static_cast<ResourceEnum>(command.intVar2);
	}
	else if (bld.isEnum(CardEnum::IntercityLogisticsHub) ||
			bld.isEnum(CardEnum::IntercityLogisticsPort))
	{
		// Update trading company values
		auto& hub = bld.subclass<IntercityLogisticsHub>();

		// Dropdowns
		if (command.intVar1 < hub.resourceEnums.size()) {
			hub.resourceEnums[command.intVar1] = static_cast<ResourceEnum>(command.intVar2);
		}
		// NumberBoxes
		else if (command.intVar1 < hub.resourceEnums.size() * 2) {
			hub.resourceCounts[command.intVar1 - hub.resourceEnums.size()] = command.intVar2;
		}
		// Target Town
		else {
			hub.SetTargetTownId(command.intVar2);
		}
	}
	else {
		UE_DEBUG_BREAK();
	}
}

void GameSimulationCore::AbandonTown(int32 townId)
{
	if (townId == -1) {
		return;
	}
	if (!GetTownhallPtr(townId)) {
		return;
	}

	
	//vector<int32> provinceIds = GetProvincesPlayer(playerId);
	//const SubregionLists<int32>& buildingList = buildingSystem().buildingSubregionList();

	//// Kill all humans
	//std::vector<int32> townIds = playerOwned(playerId).townIds();
	//for (int32 townId : townIds)
	//{
	//	std::vector<int32> citizenIds = townManager(townId).adultIds();
	//	std::vector<int32> childIds = townManager(townId).childIds();
	//	citizenIds.insert(citizenIds.end(), childIds.begin(), childIds.end());
	//	
	//	for (int32 citizenId : citizenIds) {
	//		unitSystem().unitStateAI(citizenId).Die();
	//	}
	//}

	//// End all Alliance...
	//std::vector<int32> allyPlayerIds = _playerOwnedManagers[playerId].allyPlayerIds();
	//for (int32 allyPlayerId : allyPlayerIds) {
	//	LoseAlly(playerId, allyPlayerId);
	//}
	//PUN_CHECK(_playerOwnedManagers[playerId].allyPlayerIds().size() == 0);

	//// End all Vassalage...
	//std::vector<int32> vassalBuildingIds = _playerOwnedManagers[playerId].vassalBuildingIds();
	//for (int32 vassalBuildingId : vassalBuildingIds) {
	//	//ArmyNode& armyNode = building(vassalBuildingId).subclass<ArmyNodeBuilding>().GetArmyNode();
	//	//armyNode.lordPlayerId = armyNode.originalPlayerId;

	//	_playerOwnedManagers[playerId].LoseVassal(vassalBuildingId);
	//}
	//PUN_CHECK(_playerOwnedManagers[playerId].vassalBuildingIds().size() == 0);

	//// Detach from lord
	//{
	//	int32 lordPlayerId = _playerOwnedManagers[playerId].lordPlayerId();

	//	PUN_DEBUG2("[BEFORE] Detach from lord %d, size:%llu", lordPlayerId, _playerOwnedManagers[lordPlayerId].vassalBuildingIds().size());
	//	_playerOwnedManagers[lordPlayerId].LoseVassal(playerId);
	//	PUN_DEBUG2("[AFTER] Detach from lord %d, size:%llu", lordPlayerId, _playerOwnedManagers[lordPlayerId].vassalBuildingIds().size());
	//}

	//std::vector<WorldRegion2> overlapRegions = _provinceSystem.GetRegionOverlaps(provinceIds);
	//for (WorldRegion2 region : overlapRegions)
	//{
	//	// Destroy all buildings owned by player
	//	buildingList.ExecuteRegion(region, [&](int32 buildingId)
	//	{
	//		Building& bld = building(buildingId);

	//		if (bld.playerId() == playerId) {
	//			soundInterface()->TryStopBuildingWorkSound(bld);
	//			_buildingSystem->RemoveBuilding(buildingId);

	//			AddDemolishDisplayInfo(region.centerTile(), { bld.buildingEnum(), bld.area(), Time::Ticks() });
	//			//_regionToDemolishDisplayInfos[region.regionId()].push_back({ bld.buildingEnum(), bld.area(), Time::Ticks() });
	//		}
	//	});
	//}

	//for (int32 provinceId : provinceIds)
	//{
	//	// Destroy all drops
	//	_dropSystem.ResetProvinceDrop(provinceId);

	//	// Clear Road
	//	_overlaySystem.ClearRoadInProvince(provinceId);

	//	// Remove all gather marks
	//	TileArea area = _provinceSystem.GetProvinceRectArea(provinceId);
	//	_treeSystem->MarkArea(playerId, area, true, ResourceEnum::None);

	//	// Release region ownership
	//	SetProvinceOwner(provinceId, -1);
	//}

	//// Remove Trade Route
	//worldTradeSystem().RemoveAllTradeRoutes(playerId);


	//// Remove Relationship
	//ExecuteOnAI([&](int32 aiPlayerId) {
	//	aiPlayerSystem(aiPlayerId).ClearRelationshipModifiers(playerId);
	//});
	//

	//// Reset all Systems
	//for (int32 townId : townIds) {
	//	_resourceSystems[townId] = ResourceSystem(townId, this);
	//	_townManagers[townId].reset();
	//	_townManagers[townId] = make_unique<TownManager>(playerId, townId, this); // TODO: Proper reset of TownManager
	//}

	//
	//_unlockSystems[playerId] = UnlockSystem(playerId, this);
	//_questSystems[playerId] = QuestSystem(playerId, this);
	//_playerOwnedManagers[playerId] = PlayerOwnedManager(playerId, this);
	//_playerParameters[playerId] = PlayerParameters(playerId, this);
	//_statSystem.ResetTown(playerId);
	//_popupSystems[playerId] = PopupSystem(playerId, this);
	//_cardSystem[playerId] = BuildingCardSystem(playerId, this);

	//_aiPlayerSystem[playerId] = AIPlayerSystem(playerId, this, this);

	//_eventLogSystem.ResetPlayer(playerId);

	//_playerIdToNonRepeatActionToAvailableTick[playerId] = std::vector<int32>(static_cast<int>(NonRepeatActionEnum::Count), 0);

	//AddEventLogToAllExcept(playerId, 
	//	FText::Format(LOCTEXT("XAbandonedTown_AllPop", "{0} abandoned the old town to start a new one."), playerNameT(playerId)),
	//	false
	//);
}

void GameSimulationCore::PopupInstantReply(const PopupInfo& popupInfo, int32 choiceIndex)
{
	int32 playerId = popupInfo.playerId;
	PopupReceiverEnum replyReceiver = popupInfo.replyReceiver;
	
	if (replyReceiver == PopupReceiverEnum::StartGame_Story)
	{
		TArray<FText> args;
		ADDTEXT_LOCTEXT("PointToTutorial", "Tutorials can be opened using the top-left \"?\" button.");
		ADDTEXT__(ToFText(TutorialLinkString(TutorialLinkEnum::TutorialButton)));

		ADDTEXT_INV_("\n");
		ADDTEXT_LOCTEXT("StartGameCamControl", "Camera control:<bullet>W, A, S, D keys to pan</><bullet>Mouse wheel to zoom</><bullet>Q, E keys to rotate</>");
		ADDTEXT__(ToFText(TutorialLinkString(TutorialLinkEnum::CameraControl)));

		AddPopup(playerId, JOINTEXT(args));
	}
	else if (replyReceiver == PopupReceiverEnum::DoneResearchEvent_ShowTree)
	{
		if (choiceIndex == 0) {
			unlockSystem(playerId)->shouldOpenTechUI = true;
		}
	}
	else if (replyReceiver == PopupReceiverEnum::DoneResearchEvent_ShowAllTrees)
	{
		if (choiceIndex == 0) {
			unlockSystem(playerId)->shouldOpenTechUI = true;
		}
		else if (choiceIndex == 1) {
			unlockSystem(playerId)->shouldOpenProsperityUI = true;
		}
	}
	else if (replyReceiver == PopupReceiverEnum::UnlockedHouseTree_ShowProsperityUI)
	{
		if (choiceIndex == 0) {
			unlockSystem(playerId)->shouldOpenProsperityUI = true;
		}
	}
	else if (replyReceiver == PopupReceiverEnum::Approve_AbandonTown1)
	{
		if (choiceIndex == 0) {
			AddPopup(PopupInfo(playerId,
				LOCTEXT("AskAbandonTown_Pop", "Are you sure you want to abandon this settlement? This settlement will be destroyed as a result."),
				{
					LOCTEXT("Yes", "Yes"),
					LOCTEXT("No", "No")
				}, PopupReceiverEnum::Approve_AbandonTown2, true, "PopupBad")
			);
		}
	}

	else if (replyReceiver == PopupReceiverEnum::ShowTradeDeal)
	{
		if (choiceIndex == 0) {
			ProcessTradeDeal(popupInfo);
		}
	}

	//else if (replyReceiver == PopupReceiverEnum::EraPopup_MiddleAge)
	//{
	//	GenerateRareCardSelection(playerId, RareHandEnum::Era2_1_Cards, FText());
	//	GenerateRareCardSelection(playerId, RareHandEnum::Era2_2_Cards, FText());
	//}
	//else if (replyReceiver == PopupReceiverEnum::EraPopup_EnlightenmentAge)
	//{
	//	GenerateRareCardSelection(playerId, RareHandEnum::Era3_1_Cards, FText());
	//	GenerateRareCardSelection(playerId, RareHandEnum::Era3_2_Cards, FText());
	//}
	//else if (replyReceiver == PopupReceiverEnum::EraPopup_IndustrialAge)
	//{
	//	GenerateRareCardSelection(playerId, RareHandEnum::Era4_1_Cards, FText());
	//	GenerateRareCardSelection(playerId, RareHandEnum::Era4_2_Cards, FText());
	//}
}

int32 GameSimulationCore::GetTradeDealValue(const TradeDealSideInfo& tradeDealSideInfo)
{
	int32 totalValue100 = tradeDealSideInfo.moneyAmount * 100;
	
	for (const ResourcePair& resourcePair : tradeDealSideInfo.resourcePairs) {
		totalValue100 += resourcePair.count * price100(resourcePair.resourceEnum);
	}
	for (const CardStatus& cardStatus : tradeDealSideInfo.cardStatuses) {
		totalValue100 += cardStatus.stackSize * GetBuildingInfo(cardStatus.cardEnum).baseCardPrice * 100;
	}
	
	return totalValue100 / 100;
}

void GameSimulationCore::UnpackPopupInfoToTradeDealInfo(const PopupInfo& popupInfo, TradeDealSideInfo& sourceDealInfo, TradeDealSideInfo& targetDealInfo)
{
	sourceDealInfo.playerId = popupInfo.replyVar1;
	targetDealInfo.playerId = townPlayerId(popupInfo.replyVar2);

	sourceDealInfo.moneyAmount = popupInfo.replyVar3;
	targetDealInfo.moneyAmount = popupInfo.replyVar4;

	auto unpackResourcePairs = [&](std::vector<ResourcePair>& resourcePairs, const TArray<int32>& resourceEnums, const TArray<int32>& resourceCounts)
	{
		for (int32 i = 0; i < resourceEnums.Num(); i++) {
			resourcePairs.push_back({ static_cast<ResourceEnum>(resourceEnums[i]), resourceCounts[i] });
		}
	};
	unpackResourcePairs(sourceDealInfo.resourcePairs, popupInfo.array1, popupInfo.array2);
	unpackResourcePairs(targetDealInfo.resourcePairs, popupInfo.array3, popupInfo.array4);


	auto unpackCardStatuses = [&](std::vector<CardStatus>& cardStatuses, const TArray<int32>& cardEnums, const TArray<int32>& cardCounts)
	{
		for (int32 i = 0; i < cardEnums.Num(); i++) {
			cardStatuses.push_back(CardStatus(static_cast<CardEnum>(cardEnums[i]), cardCounts[i]));
		}
	};
	unpackCardStatuses(sourceDealInfo.cardStatuses, popupInfo.array5, popupInfo.array6);
	unpackCardStatuses(targetDealInfo.cardStatuses, popupInfo.array7, popupInfo.array8);
}

std::shared_ptr<FGenericCommand> GameSimulationCore::PackTradeDealInfoToCommand(const TradeDealSideInfo& sourceDealInfo, const TradeDealSideInfo& targetDealInfo, TradeDealStageEnum nextStage)
{
	auto command = make_shared<FGenericCommand>();
	command->genericCommandType = FGenericCommand::Type::SendGift;
	command->intVar1 = sourceDealInfo.playerId;
	command->intVar2 = targetDealInfo.playerId;
	command->intVar3 = sourceDealInfo.moneyAmount;
	command->intVar4 = targetDealInfo.moneyAmount;
	command->intVar5 = static_cast<int32>(nextStage);

	auto fillResourceValues = [&](const TradeDealSideInfo& dealInfo, TArray<int32>& arrayResourceEnum, TArray<int32>& arrayResourceCount)
	{
		for (int32 i = 0; i < dealInfo.resourcePairs.size(); i++)
		{
			arrayResourceEnum.Add(static_cast<int32>(dealInfo.resourcePairs[i].resourceEnum));
			arrayResourceCount.Add(dealInfo.resourcePairs[i].count);
		}
	};
	fillResourceValues(sourceDealInfo, command->array1, command->array2);
	fillResourceValues(targetDealInfo, command->array3, command->array4);


	auto fillCards = [&](const TradeDealSideInfo& dealInfo, TArray<int32>& arrayCardEnum, TArray<int32>& arrayCardCount)
	{
		for (int32 i = 0; i < dealInfo.cardStatuses.size(); i++)
		{
			arrayCardEnum.Add(static_cast<int32>(dealInfo.cardStatuses[i].cardEnum));
			arrayCardCount.Add(dealInfo.cardStatuses[i].stackSize);
		}
	};
	fillCards(sourceDealInfo, command->array5, command->array6);
	fillCards(targetDealInfo, command->array7, command->array8);

	return command;
}

void GameSimulationCore::ProcessTradeDeal(const PopupInfo& popupInfo)
{
	TradeDealStageEnum dealStageEnum = static_cast<TradeDealStageEnum>(popupInfo.replyVar5);

	TradeDealSideInfo sourceDealInfo;
	TradeDealSideInfo targetDealInfo;
	UnpackPopupInfoToTradeDealInfo(popupInfo, sourceDealInfo, targetDealInfo);


	int32 sourcePlayerId = sourceDealInfo.playerId;
	int32 targetPlayerId = targetDealInfo.playerId;
	

	// Open the UI
	if (dealStageEnum == TradeDealStageEnum::ExamineDeal ||
		dealStageEnum == TradeDealStageEnum::ExamineCounterOfferDeal)
	{
		// swap source and target
		_uiInterface->OpenGiftUI(targetPlayerId, sourcePlayerId, dealStageEnum);
		_uiInterface->FillDealInfo(targetDealInfo, sourceDealInfo);
		return;
	}

	check(dealStageEnum == TradeDealStageEnum::Gifting ||
		dealStageEnum == TradeDealStageEnum::AcceptDeal);

	// TODO: Deal containing resource + multiple cities, ask which city it should go to

	// TODO: trade with non-capital

	/*
	 * Check Deal availability
	 */
	 // (just go negative for money)

	 //! Resource Availability
	bool sourceHasEnoughResource = HasEnoughResource(sourcePlayerId, sourceDealInfo.resourcePairs);

	if (!sourceHasEnoughResource ||
		!HasEnoughResource(targetPlayerId, targetDealInfo.resourcePairs))
	{
		if (dealStageEnum == TradeDealStageEnum::Gifting) {
			AddPopupToFront(sourcePlayerId,
				LOCTEXT("Gifting_NotEnoughResource", "Not enough resource to give out."),
				ExclusiveUIEnum::GiftResourceUI, "PopupCannot"
			);
		}
		else
		{
			AddPopupToFront(sourcePlayerId,
				FText::Format(
					LOCTEXT("TradeDeal_NotEnoughResource", "Deal failed.<space>{0} no longer have enough resource."),
					playerNameT(!sourceHasEnoughResource ? sourcePlayerId : targetPlayerId)
				),
				ExclusiveUIEnum::GiftResourceUI, "PopupCannot"
			);
		}
		return;
	}


	//! Card Availability
	auto checkCardAvailability = [&](int32 checkPlayerId, const TradeDealSideInfo& dealSideInfo)
	{
		for (const CardStatus& cardStatus : dealSideInfo.cardStatuses) 
		{
			if (cardSystem(sourcePlayerId).BoughtCardCount(cardStatus.cardEnum) < cardStatus.stackSize)
			{
				if (dealStageEnum == TradeDealStageEnum::Gifting) {
					AddPopupToFront(sourcePlayerId,
						LOCTEXT("Gifting_NotEnoughCards", "Not enough cards to give out."),
						ExclusiveUIEnum::GiftResourceUI, "PopupCannot"
					);
				}
				else {
					auto addPopup = [&](int32 popupPlayerId)
					{
						AddPopupToFront(popupPlayerId,
							FText::Format(
								LOCTEXT("TradeDeal_NotEnoughCards", "Deal failed.<space>{0} no longer have enough cards."),
								playerNameT(checkPlayerId)
							),
							ExclusiveUIEnum::GiftResourceUI, "PopupCannot"
						);
					};
					addPopup(sourcePlayerId);
					addPopup(targetPlayerId);
				}
				return;
			}
		}
	};

	checkCardAvailability(sourcePlayerId, sourceDealInfo);
	checkCardAvailability(targetPlayerId, targetDealInfo);

	/*
	 * Execute the deal
	 */

	 //! Money
	ChangeMoney(sourcePlayerId, -sourceDealInfo.moneyAmount);
	ChangeMoney(sourcePlayerId, targetDealInfo.moneyAmount);
	ChangeMoney(targetPlayerId, -targetDealInfo.moneyAmount);
	ChangeMoney(targetPlayerId, sourceDealInfo.moneyAmount);


	//! Resources
	auto tradeResource = [&](int32 removePlayerId, int32 addPlayerId, const std::vector<ResourcePair>& resourcePairs)
	{
		auto& removeResourceSys = resourceSystem(removePlayerId);
		for (ResourcePair resourcePair : resourcePairs)
		{
			int32 amountToRemove = std::min(abs(resourcePair.count), resourceCountTown(removePlayerId, resourcePair.resourceEnum));
			removeResourceSys.RemoveResourceGlobal(resourcePair.resourceEnum, amountToRemove);
			AddResourceGlobal(addPlayerId, resourcePair.resourceEnum, amountToRemove);
		}
	};

	tradeResource(sourcePlayerId, targetPlayerId, sourceDealInfo.resourcePairs);
	tradeResource(targetPlayerId, sourcePlayerId, targetDealInfo.resourcePairs);


	//! Cards
	auto tradeCards = [&](int32 removePlayerId, int32 addPlayerId, const std::vector<CardStatus>& cardStatuses)
	{
		for (const CardStatus& cardStatus : cardStatuses) {
			cardSystem(removePlayerId).RemoveCards_BoughtHandAndInventory(cardStatus);
			cardSystem(addPlayerId).AddCards_BoughtHandAndInventory(cardStatus);
		}
	};

	tradeCards(sourcePlayerId, targetPlayerId, sourceDealInfo.cardStatuses);
	tradeCards(targetPlayerId, sourcePlayerId, targetDealInfo.cardStatuses);


	//! Add Trade/Gift Popup

	FText sourcePlayerText1;
	FText sourcePlayerText2;

	FText targetPlayerText1;
	FText targetPlayerText2;

	if (dealStageEnum == TradeDealStageEnum::Gifting)
	{
		sourcePlayerText1 = FText::Format(LOCTEXT("Gifted_SourcePop", "You gifted {0} with:<space>"), playerNameT(targetPlayerId));
		sourcePlayerText2 = FText();

		targetPlayerText1 = FText::Format(LOCTEXT("Gifted_TargetPop", "{0} gifted you with:<space>"), playerNameT(sourcePlayerId));
		targetPlayerText2 = FText();
	}
	else
	{
		sourcePlayerText1 = LOCTEXT("TradeDeal_DealSuccessPop1", "Deal successfully completed!<space>You gave:<space>");
		sourcePlayerText2 = LOCTEXT("TradeDeal_DealSuccessPop2", "<space>They gave:<space>");

		targetPlayerText1 = sourcePlayerText1;
		targetPlayerText2 = sourcePlayerText2;
	}

	auto fillListText = [&](TArray<FText>& listText, TradeDealSideInfo dealSideInfo)
	{
		if (dealSideInfo.moneyAmount > 0) {
			listText.Add(FText::Format(INVTEXT("<bullet>{0}<img id=\"Coin\"/></>"), dealSideInfo.moneyAmount));
		}
		for (const ResourcePair& resourcePair : dealSideInfo.resourcePairs) {
			listText.Add(FText::Format(INVTEXT("<bullet>{0} {1}</>"), resourcePair.count, ResourceNameT(resourcePair.resourceEnum)));
		}
		for (const CardStatus& cardStatus : dealSideInfo.cardStatuses) {
			listText.Add(FText::Format(INVTEXT("<bullet>{0} {1} {0}|plural(one=Card,other=Cards)</>"), cardStatus.stackSize, GetBuildingInfo(cardStatus.cardEnum).name));
		}
	};


	TArray<FText> textList;
	textList.Add(sourcePlayerText1);
	fillListText(textList, sourceDealInfo);
	textList.Add(sourcePlayerText2);
	fillListText(textList, targetDealInfo);

	AddPopup(sourcePlayerId, JOINTEXT(textList));


	textList.Empty();
	textList.Add(targetPlayerText1);
	fillListText(textList, targetDealInfo);
	textList.Add(targetPlayerText2);
	fillListText(textList, sourceDealInfo);
	
	AddPopup(targetPlayerId, JOINTEXT(textList));

	
	// Relationship Modifier
	ChangeRelationshipModifier(targetPlayerId, sourcePlayerId,
		dealStageEnum == TradeDealStageEnum::Gifting ? RelationshipModifierEnum::YouGaveUsGifts : RelationshipModifierEnum::GoodTradeDeal,
		(GetTradeDealValue(sourceDealInfo) - GetTradeDealValue(targetDealInfo)) / GoldToRelationship
	);
	ChangeRelationshipModifier(sourcePlayerId, targetPlayerId,
		dealStageEnum == TradeDealStageEnum::Gifting ? RelationshipModifierEnum::YouGaveUsGifts : RelationshipModifierEnum::GoodTradeDeal,
		(GetTradeDealValue(targetDealInfo) - GetTradeDealValue(sourceDealInfo)) / GoldToRelationship
	);
}



void GameSimulationCore::PopupDecision(FPopupDecision command) 
{
	UE_LOG(LogNetworkInput, Log, TEXT(" PopupDecision replyReceiver:%d choice:%d"), command.replyReceiverIndex, command.choiceIndex);
	
	TownHall& town = GetTownhallCapital(command.playerId);

	CloseCurrentPopup(command.playerId);

	PopupReceiverEnum replyReceiver = static_cast<PopupReceiverEnum>(command.replyReceiverIndex);
	
	if (replyReceiver == PopupReceiverEnum::ImmigrationEvent) 
	{
		if (command.choiceIndex == 0) {
			AddPopupToFront(command.playerId, 
				LOCTEXT("LetImmigrantsIn", "You can't help but notice the wide smile that spread across an immigrant child as she enters her promised land."));
			town.AddRequestedImmigrants();
		}
		else if(command.choiceIndex == 1) {
			//if (Time::Years() % 2 == 0) {
				int32 sneakedIns = town.migrationPull() / 2;
				AddPopupToFront(command.playerId, 
					FText::Format(LOCTEXT("ImmigrantsSneakedIn", "{0} immigrants decided to illegally sneaked in anyway..."), TEXT_NUM(sneakedIns)));
				town.AddImmigrants(sneakedIns);
			//} else {
			//	AddPopupToFront(command.playerId, "Their earlier joyful smiles of hope turned into gloom as the immigrants leave the town.");
			//}
		}
		else {
			AddPopupToFront(command.playerId, 
				LOCTEXT("ImmigrantsCannibalized", "News of your horrifying atrocity spreads across the world. You stole 100 gold and gained 100 pork.")
			);
			ChangeMoney(command.playerId, 100);
			resourceSystem(command.playerId).AddResourceGlobal(ResourceEnum::Pork, 100, *this);
		}
	}

	else if (replyReceiver == PopupReceiverEnum::ImmigrationBetweenPlayersEvent)
	{
		if (command.choiceIndex == 0) {
			AddPopupToFront(command.playerId, 
				LOCTEXT("ImmigrantsPromisedLand", "You can't help but notice the wide smile that spread across an immigrant child as she enters her promised land.")
			);
			town.AddRequestedImmigrants();
		}
		else if (command.choiceIndex == 1) {
			AddPopupToFront(command.playerId, 
				LOCTEXT("ImmigrantsGloom", "Their earlier joyful smiles of hope turned into gloom as the immigrants left your town.")
			);
		}
		else {
			AddPopupToFront(command.playerId, 
				LOCTEXT("ImmigrantsStoleGold", "News of your horrifying atrocity spreads across the world. You stole 100 gold.")
			);
			ChangeMoney(command.playerId, 100);
		}
	}
	
	else if (replyReceiver == PopupReceiverEnum::TribalJoinEvent) 
	{
		if (command.choiceIndex == 0) {
			AddPopupToFront(command.playerId, 
				LOCTEXT("TribeConsumed", "Tribe people are now a part of our big family.")
			);
			town.AddRequestedImmigrants();
			GenerateRareCardSelection(command.playerId, RareHandEnum::BuildingSlotCards, LOCTEXT("A gift from the tribe.", "A gift from the tribe."));
		}
		else if (command.choiceIndex == 1) {
			AddPopupToFront(command.playerId, 
				LOCTEXT("TribedForcedOut", "Forced out of their home, the tribe left our land.")
			);
		}
		else {
			AddPopupToFront(command.playerId, 
				LOCTEXT("TribeStoleGold", "News of your horrifying atrocity spreads across the world. You stole 100 gold.")
			);
			ChangeMoney(command.playerId, 100);
		}
	}
	
	else if (replyReceiver == PopupReceiverEnum::CaravanBuyer)
	{
		if (command.choiceIndex == 0) {
			if (command.playerId == gameManagerPlayerId()) {
				uiInterface()->OpenTradeUI(town.buildingId());
			}
		}
	}

	else if (replyReceiver == PopupReceiverEnum::DoneResearchBuyCardEvent)
	{
		if (command.choiceIndex == 0) 
		{
			CardEnum unlockedEnum = static_cast<CardEnum>(command.replyVar1);
			PUN_CHECK(unlockedEnum != CardEnum::None);

			if (CanBuyCard(command.playerId, unlockedEnum))
			{
				auto& cardSys = cardSystem(command.playerId);
				cardSys.AddCardToHand2(unlockedEnum);
				ChangeMoney(command.playerId, -cardSys.GetCardPrice(unlockedEnum));
			}
		}
	}
	else if (replyReceiver == PopupReceiverEnum::ResetBuff)
	{
		if (command.choiceIndex == 0) {
			playerOwned(command.playerId).TryApplyBuff(static_cast<CardEnum>(command.replyVar1));
		}
	}
	else if (replyReceiver == PopupReceiverEnum::Approve_Cannibalism)
	{
		if (command.choiceIndex == 0) {
			auto& cardSys = cardSystem(command.playerId);
			cardSys.AddCardToHand2(CardEnum::Cannibalism);
		}
	}
	else if (replyReceiver == PopupReceiverEnum::Approve_AbandonTown2)
	{
		if (command.choiceIndex == 0) {
			AbandonTown(command.playerId);
		}
	}
	else if (replyReceiver == PopupReceiverEnum::ChooseLocationDone)
	{
		// Initial hand
		//GenerateRareCardSelection(command.playerId, RareHandEnum::InitialCards1, "A starting card.");
		//GenerateRareCardSelection(command.playerId, RareHandEnum::InitialCards2, "Another starting card.");
	}
	else if (replyReceiver == PopupReceiverEnum::AllyRequest)
	{
		if (command.choiceIndex == 0)
		{
			int32 requesterPlayerId = command.replyVar1;

			if (!townManager(requesterPlayerId).IsAlly(command.playerId))
			{
				GainAlly(requesterPlayerId, command.playerId);

				// Tell requester alliance is accepted
				AddPopup(requesterPlayerId,
					FText::Format(LOCTEXT("AllianceAccepted_RequesterPop", "Alliance request accepted by {0}."), playerNameT(command.playerId))
				);
				AddPopup(command.playerId,
					FText::Format(LOCTEXT("AllianceAccepted_SelfPop", "You are now allied with {0}."), playerNameT(requesterPlayerId))
				);
			}
		}
	}
	else if (replyReceiver == PopupReceiverEnum::RaidHandleDecision)
	{
		int32 provinceId = command.replyVar1;
		int32 raiderPlayerId = command.replyVar2;
		int32 defenderPlayerId = command.playerId;
		
		if (command.choiceIndex == 0) {
			CompleteRaid(provinceId, raiderPlayerId, defenderPlayerId);
		}
		else if (command.choiceIndex == 1) {
			_uiInterface->OpenReinforcementUI(provinceId, CallbackEnum::RaidBattle);
		}
	}
	
	else if (replyReceiver == PopupReceiverEnum::StartGame_AskAboutAdvice)
	{
		if (command.choiceIndex == 0) {
			// Need advices, start with food building
			AddQuest(command.playerId, make_shared<FoodBuildingQuest>());
		}
		else  {
			// Doesn't need advices, population quest right away
			auto popQuest = std::make_shared<PopulationQuest>();
			popQuest->populationSizeTier = 1;
			AddQuest(command.playerId, popQuest);
		}
	}
	else if (replyReceiver == PopupReceiverEnum::MaxCardHandQueuePopup)
	{
		if (command.choiceIndex != 0) {
			cardSystem(command.playerId).allowMaxCardHandQueuePopup = false;
		}
	}
	else if (replyReceiver == PopupReceiverEnum::ChooseTownToClaimWith)
	{
		int32 provinceId = command.replyVar1;
		vector<int32> attackerTownIds = GetProvinceAttackerTownIds(command.playerId, provinceId);
		if (command.choiceIndex < attackerTownIds.size()) {
			int32 townId = attackerTownIds[command.choiceIndex];
			bool isFull = command.replyVar2;
			if (isFull) { // Full ProvinceClaim
				SetProvinceOwnerFull(provinceId, townId);
			} else {
				SetProvinceOwner(provinceId, townId);
			}
		}
	}
	else if (replyReceiver == PopupReceiverEnum::EraPopup_MiddleAge)
	{
		GenerateRareCardSelection(command.playerId, RareHandEnum::Era2_1_Cards, FText());
		GenerateRareCardSelection(command.playerId, RareHandEnum::Era2_2_Cards, FText());
	}
	else if (replyReceiver == PopupReceiverEnum::EraPopup_EnlightenmentAge)
	{
		GenerateRareCardSelection(command.playerId, RareHandEnum::Era3_1_Cards, FText());
		GenerateRareCardSelection(command.playerId, RareHandEnum::Era3_2_Cards, FText());
	}
	else if (replyReceiver == PopupReceiverEnum::EraPopup_IndustrialAge)
	{
		GenerateRareCardSelection(command.playerId, RareHandEnum::Era4_1_Cards, FText());
		GenerateRareCardSelection(command.playerId, RareHandEnum::Era4_2_Cards, FText());
	}
	else if (replyReceiver == PopupReceiverEnum::None)
	{

	}
	else {
		UE_DEBUG_BREAK();
	}

}

void GameSimulationCore::CompleteRaid(int32 provinceId, int32 raiderPlayerId, int32 defenderTownId)
{
	int32 raidMoney = GetProvinceRaidMoney100(provinceId) / 100;
	int32 raidInfluence = raidMoney / 2;

	ChangeMoney(raiderPlayerId, raidMoney);
	ChangeInfluence(raiderPlayerId, raidInfluence);

	int32 defenderPlayerId = townPlayerId(defenderTownId);
	
	if (IsMinorTown(defenderTownId)) {
		townManagerBase(defenderTownId)->ChangeMoney(-raidMoney);
	}
	else {
		ChangeMoney(defenderPlayerId, -raidMoney);
		ChangeInfluence(defenderPlayerId, -raidInfluence);
	}

	AddPopup(raiderPlayerId, FText::Format(
		LOCTEXT("RaidComplete_Pop_Attacker", "You raided {0}.<space>You received <img id=\"Coin\"/>{1} and <img id=\"Influence\"/>{2}."),
		townNameT(defenderTownId),
		TEXT_NUM(raidMoney),
		TEXT_NUM(raidInfluence)
	));

	AddPopup(defenderPlayerId, FText::Format(
		LOCTEXT("RaidComplete_Pop_Defender", "{0} raided you.<space>You lost <img id=\"Coin\"/>{1} and <img id=\"Influence\"/>{2}."),
		playerNameT(raiderPlayerId),
		TEXT_NUM(raidMoney),
		TEXT_NUM(raidInfluence)
	));
}

void GameSimulationCore::RerollCards(FRerollCards command)
{
	UE_LOG(LogNetworkInput, Log, TEXT(" RerollCards"));
	
	auto& cardSys = cardSystem(command.playerId);
	
	int32 rerollPrice = cardSys.GetRerollPrice();

	if (rerollPrice == 0) { // Free reroll
		cardSys.RollHand(cardSys.handSize(), true);
	}
	else if (moneyCap32(command.playerId) >= rerollPrice) {
		cardSys.RollHand(cardSys.handSize(), true);
		ChangeMoney(command.playerId , -rerollPrice);
	} else {
		AddPopupToFront(command.playerId, 
			LOCTEXT("NoRerollMoney", "Not enough money for reroll"), 
			ExclusiveUIEnum::CardHand1, "PopupCannot"
		);
	}

	cardSys.SetPendingCommand(false);
}

void GameSimulationCore::SelectRareCard(FSelectRareCard command)
{
	_LOG(LogNetworkInput, " SelectRareCard");
	
	// TODO: rename this to just select card?
	auto& cardSys = cardSystem(command.playerId);

	if (cardSys.CanSelectRareCardPrize(command.cardEnum)) 
	{
		// Town Bonus Cards go straight TownManager
		if (IsPermanentTownBonus(command.cardEnum)) {
			townManager(command.objectId).AddTownBonus(command.cardEnum);
			cardSys.DoneSelectRareHand();
		}
		// Global Bonus Cards go straight to PlayerManager
		else if (IsPermanentGlobalBonus(command.cardEnum)) {
			playerOwned(command.playerId).AddGlobalBonus(command.cardEnum);
			cardSys.DoneSelectRareHand();
		}
		else
		{
			cardSys.AddCardToHand2(command.cardEnum);
			cardSys.DoneSelectRareHand();
		}

		// Special case: unlocked rare card before it is done
		if (command.cardEnum == CardEnum::WheatSeed)
		{
			auto unlockSys = unlockSystem(command.playerId);
			std::shared_ptr<ResearchInfo> tech = unlockSys->GetTechInfo(TechEnum::Wheat);
			tech->state = TechStateEnum::Researched;
			tech->upgradeCount++;
			unlockSys->techsFinished++;
		}
	}
}

void GameSimulationCore::BuyCards(FBuyCard command)
{
	UE_LOG(LogNetworkInput, Log, TEXT(" BuyCards"));
	
	auto& cardSys = cardSystem(command.playerId);
	TArray<int32>& buyIndices = command.cardHandBuyIndices;
	for (int32 i = 0; i < buyIndices.Num(); i++)
	{
		CardEnum buildingEnum = cardSys.GetCardEnumFromHand(buyIndices[i]);

		if (CanBuyCard(command.playerId, buildingEnum)) {
			ChangeMoney(command.playerId, -cardSys.GetCardPrice(buildingEnum)); // Must ChangeMoney before adding cards for cards with lvl...
			cardSys.AddCardToHand2(buildingEnum, true);

			// First time buying card for this quest? Introduce round timer properly
			if (!playerOwned(command.playerId).alreadyBoughtFirstCard &&
				HasQuest(command.playerId, QuestEnum::FoodBuildingQuest))
			{
				AddPopup(command.playerId, 
					LOCTEXT("FirstBuyCard_Pop", "Great job! You have bought your first card.<space>Card selections automatically refresh every round.<space>The round timer is shown next to the card stack.\n(2 rounds per season)")
				);
			}
		}
	}

	cardSys.UseCardHandQueue();
	
	cardSys.SetPendingCommand(false);
}

void GameSimulationCore::SellCards(FSellCards command)
{
	UE_LOG(LogNetworkInput, Log, TEXT(" SellCards"));

	auto& cardSys = cardSystem(command.playerId);

	CardStatus cardStatus = cardSys.GetActualBoughtCardStatus(command.cardStatus);

	if (cardStatus.isValid())
	{
		int32 sellCount = command.isShiftDown ? cardStatus.stackSize : 1;

		int32 soldCount = cardSys.RemoveCardsFromBoughtHand(cardStatus, sellCount);
		if (soldCount != -1)
		{
			ChangeMoney(command.playerId, soldCount * cardSys.GetCardPrice(cardStatus.cardEnum));

			AddEventLog(command.playerId,
				FText::Format(
					LOCTEXT("SoldCard_Event", "Sold {1} {0} {1}|plural(one=Card,other=Cards) for {2}<img id=\"Coin\"/>"),
					GetBuildingInfo(cardStatus.cardEnum).name,
					TEXT_NUM(sellCount),
					TEXT_NUM(soldCount)
				),
				false
			);
		}
	}
}

void GameSimulationCore::UseCard(FUseCard command)
{
	UE_LOG(LogNetworkInput, Log, TEXT(" UseCard"));
	
	auto& cardSys = cardSystem(command.playerId);
	auto& resourceSys = resourceSystem(command.playerId);
	auto& globalResourceSys = globalResourceSystem(command.playerId);

	CardEnum commandCardEnum = command.cardStatus.cardEnum;

	// Archives Store Card
	if (command.callbackEnum == CallbackEnum::ArchivesSlotting)
	{
		int32 buildingId = command.variable1;
		if (isValidBuildingId(buildingId))
		{
			Building& bld = building(buildingId);
			if (bld.isEnum(CardEnum::Archives) && bld.CanAddSlotCard())
			{
				int32 soldCount = cardSys.RemoveCardsFromBoughtHand(command.cardStatus, 1);
				if (soldCount != -1)
				{
					CardStatus cardStatus = command.GetCardStatus_AnimationOnly(_gameManager->GetDisplayWorldTime() * 100.0f);
					bld.AddSlotCard(cardStatus);
				}
			}
		}
		return;
	}

	// Train Unit
	if (command.callbackEnum == CallbackEnum::TrainUnit ||
		command.callbackEnum == CallbackEnum::CancelTrainUnit)
	{
		if (IsValidTown(command.townId)) {
			townManager(command.townId).TrainUnits(command);
		}
		return;
	}

	// Card Inventory Store Card
	if (command.callbackEnum == CallbackEnum::SelectInventorySlotCard)
	{
		cardSys.MoveCardInventoryToHand(command.cardStatus, command.variable1);
		return;
	}
	if (command.callbackEnum == CallbackEnum::CardInventorySlotting)
	{
		cardSys.MoveHandToCardInventory(command.cardStatus, command.variable1);
		return;
	}
	

	// CardRemoval
	if (commandCardEnum == CardEnum::CardRemoval)
	{
		if (cardSys.HasBoughtCard(CardEnum::CardRemoval))
		{
			FText cardName = GetBuildingInfoInt(command.variable1).name;
			AddPopupToFront(command.playerId, 
				FText::Format(LOCTEXT("CardRemovalDone", "Removed {0} Card from your deck.<space>{0} is now only available from Wild Cards."), GetBuildingInfoInt(command.variable1).name));
			
			cardSys.RemoveCardsOld(CardEnum::CardRemoval, 1);
			cardSys.RemoveDrawCards(static_cast<CardEnum>(command.variable1));
		}
		return;
	}

	// Global Slot
	if (IsTownSlotCard(commandCardEnum))
	{
		if (IsValidTown(command.townId)) // (Edge case where Archive UI was hidden)
		{
			auto& townManage = townManager(command.townId);

			if (townManage.TownhallHasCard(commandCardEnum)) {
				AddPopupToFront(command.playerId, FText::Format(
					LOCTEXT("TownhallSlots_NoSameTypeCard", "Townhall cannot take a duplicate Card.<space>{0} is already slotted."),
					GetBuildingInfo(commandCardEnum).name
				));
			}
			else if (townManage.CanAddCardToTownhall()) {
				int32 soldCount = cardSys.RemoveCardsOld(commandCardEnum, 1);
				if (soldCount != -1) {
					townManage.AddCardToTownhall(command.GetCardStatus_AnimationOnly(_gameManager->GetDisplayWorldTime() * 100.0f));
					townManage.RecalculateTaxDelayed();
				}
			}
			else {
				AddPopupToFront(command.playerId, LOCTEXT("TownhallSlotsFull", "Townhall card slots full."));
			}
		}
		return;
	}

	// Building Slot
	if (IsBuildingSlotCard(commandCardEnum))
	{
		// - Guard against isEnum() crash
		if (isValidBuildingId(command.variable1))
		{
			Building& bld = building(command.variable1);
			if (bld.CanAddSlotCard())
			{
				// Don't allow slotting Mint with "SustainabilityBook"
				if (commandCardEnum == CardEnum::SustainabilityBook &&
					bld.isEnum(CardEnum::Mint))
				{
					return;
				}

				int32 soldCount = cardSys.RemoveCardsOld(commandCardEnum, 1);
				if (soldCount != -1) {
					bld.AddSlotCard(command.GetCardStatus_AnimationOnly(_gameManager->GetDisplayWorldTime() * 100.0f));
				}
			}
		}
		
		return;
	}

	// Crate
	if (IsCrateCard(commandCardEnum))
	{
		ResourcePair resourcePair = GetCrateResource(commandCardEnum);

		if (resourceSystem(command.playerId).CanAddResourceGlobal(resourcePair.resourceEnum, resourcePair.count) &&
			cardSys.HasBoughtCard(commandCardEnum))
		{
			resourceSystem(command.playerId).AddResourceGlobal(resourcePair.resourceEnum, resourcePair.count, *this);
			AddPopupToFront(command.playerId, 
				FText::Format(LOCTEXT("GainedXResourceFromCrate", "Gained {0} {1} from crates."), TEXT_NUM(resourcePair.count), GetResourceInfo(resourcePair.resourceEnum).name)
			);
			cardSys.RemoveCardsOld(commandCardEnum, 1);
		}
		else {
			AddPopup(command.playerId, 
				LOCTEXT("NotEnoughStorageSpace", "Not enough storage space."), 
				"PopupCannot"
			);
		}
		return;
	}
	

	/*
	 * Others
	 */
	bool succeedUsingCard = true;
	
	if (commandCardEnum == CardEnum::Treasure) {
		ChangeMoney(command.playerId, 500);
		AddPopupToFront(command.playerId, 
			LOCTEXT("GainedCoinFromTreasure", "Gained 500<img id=\"Coin\"/> from treasure.")
		);
	}
	else if (commandCardEnum == CardEnum::EmergencyRations) {
		resourceSystem(command.playerId).AddResourceGlobal(ResourceEnum::Wheat, 50, *this);
		AddPopupToFront(command.playerId, 
			FText::Format(LOCTEXT("GainedEmergencyRation", "Gained {0} {1} from emergency ration."), TEXT_NUM(50), GetResourceInfo(ResourceEnum::Wheat).name)
		);
	}
	
	else if (commandCardEnum == CardEnum::KidnapGuard ||
		commandCardEnum == CardEnum::TreasuryGuard)
	{
		playerOwned(command.playerId).TryApplyBuff(commandCardEnum);
	}
	
	else if (IsSeedCard(commandCardEnum))
	{
		SeedInfo seedInfo = GetSeedInfo(commandCardEnum);
		FText plantName = GetTileObjInfo(seedInfo.tileObjEnum).name;
		
		// Unlock farm if needed
		if (!unlockSystem(command.playerId)->isUnlocked(CardEnum::Farm)) {
			unlockSystem(command.playerId)->UnlockBuilding(CardEnum::Farm);
			AddPopupToFront(command.playerId, 
				LOCTEXT("Unlocked farm!", "Unlocked farm!")
			);
		}
		
		globalResourceSystem(command.playerId).AddSeed(seedInfo);

		if (IsCommonSeedCard(commandCardEnum)) {
			AddPopupToFront(command.playerId, 
				FText::Format(LOCTEXT("UnlockedCropSwithToGrow", "Unlocked {0}. Switch your farm's workmode to grow {0}."), plantName)
			);
		} else {
			PUN_CHECK(IsSpecialSeedCard(commandCardEnum));
			
			AddPopupToFront(command.playerId, FText::Format(
				LOCTEXT("UnlockedCropRequireSuitableRegion", "Unlocked {0}. {0} requires suitable regions marked on the map to be grown."), 
				plantName
			));
		}
	}

	else if (commandCardEnum == CardEnum::SellFood)
	{
		if (IsValidTown(command.townId))
		{
			auto& townResourceSys = resourceSystem(command.townId);
			
			int32 totalRemoved = 0;
			for (ResourceEnum foodEnum : StaticData::FoodEnums) 
			{
				int32 amountToRemove = townResourceSys.resourceCount(foodEnum) / 2;
				townResourceSys.RemoveResourceGlobal(foodEnum, amountToRemove);
				globalResourceSys.ChangeMoney(amountToRemove * FoodCost);
				totalRemoved += amountToRemove;
			}
			AddPopupToFront(command.playerId,
				FText::Format(LOCTEXT("SoldFoodForCoin", "Sold {0} food for <img id=\"Coin\"/>{1}."), TEXT_NUM(totalRemoved), TEXT_NUM(totalRemoved * FoodCost))
			);
			
		}
	}
	else if (commandCardEnum == CardEnum::BuyWood)
	{
		if (globalResourceSys.moneyCap32() > 0)
		{
			auto& townResourceSys = resourceSystem(command.townId);
			
			int32 cost = GetResourceInfo(ResourceEnum::Wood).basePrice;
			int32 amountToBuy = globalResourceSys.moneyCap32() / 2 / cost;
			amountToBuy = min(amountToBuy, 1000);

			if (townResourceSys.CanAddResourceGlobal(ResourceEnum::Wood, amountToBuy)) {
				townResourceSys.AddResourceGlobal(ResourceEnum::Wood, amountToBuy, *this);
				int32 moneyPaid = amountToBuy * cost;
				globalResourceSys.ChangeMoney(-moneyPaid);

				AddPopupToFront(command.playerId, 
					FText::Format(LOCTEXT("BoughtWoodUseCoin", "Bought {0} wood for <img id=\"Coin\"/>{1}."), TEXT_NUM(amountToBuy), TEXT_NUM(moneyPaid))
				);
			}
			else {
				succeedUsingCard = false;
				AddPopupToFront(command.playerId, 
					LOCTEXT("NotEnoughStorageInCity", "Not enough storage space in our city.")
				);
			}
		}
		else {
			succeedUsingCard = false;
			AddPopupToFront(command.playerId, 
				LOCTEXT("NeedMoreMoneyForBuyWoodCard", "Need more money to use the Buy Wood Card.")
			);
		}
	}
	else if (commandCardEnum == CardEnum::Immigration) 
	{
		if (IsValidTown(command.townId))
		{
			GetTownhall(command.townId).AddImmigrants(5);
			AddPopupToFront(command.playerId,
				LOCTEXT("ImmigrantsJoinedFromAds", "5 immigrants joined after hearing the advertisement.")
			);
		}
	}

	else {
		UE_DEBUG_BREAK();
	}

	if (succeedUsingCard) {
		cardSys.RemoveCardsOld(commandCardEnum, 1);
	}
}

void GameSimulationCore::UnslotCard(FUnslotCard command)
{
	PUN_ENSURE(IsValidBuilding(command.buildingId), return);

	UE_LOG(LogNetworkInput, Log, TEXT(" UnslotCard"));
	
	Building& bld = building(command.buildingId);
	PUN_ENSURE(bld.playerId() == command.playerId, return);

	
	auto& cardSys = cardSystem(command.playerId);

	if (bld.isEnum(CardEnum::Townhall))
	{
		auto& townManage = townManager(bld.townId());
		CardEnum cardEnum = townManage.RemoveCardFromTownhall(command.unslotIndex);
		if (cardEnum != CardEnum::None) {
			cardSys.AddCardToHand2(cardEnum);
			townManage.RecalculateTaxDelayed();
		}
	}
	else
	{
		// Ensure there is enough space to add card
		CardStatus cardStatus = bld.slotCard(command.unslotIndex);
		if (cardStatus.cardEnum != CardEnum::None)
		{
			if (cardSys.CanAddCardToBoughtHand(cardStatus.cardEnum, 1))
			{
				CardEnum cardEnum = bld.RemoveSlotCard(command.unslotIndex);
				if (cardEnum != CardEnum::None) {
					cardSys.AddCardToHand2(cardEnum);
				}
			}
			else {
				AddPopupToFront(command.playerId,
					LOCTEXT("UnslotErrorHandsFull", "Cannot unslot the Card. Your Card Hand is full.")
				);
			}
		}

	}
}

// Any army order, not just attack anymore...
void GameSimulationCore::Attack(FAttack command)
{
	PUN_ENSURE(IsValidBuilding(command.originNodeId), return);
	PUN_ENSURE(IsValidBuilding(command.targetNodeId), return);
	
	_LOG(LogNetworkInput, " Attack");

	//ArmyNode& originNode = GetArmyNode(command.originNodeId);
	//ArmyNode& targetNode = GetArmyNode(command.targetNodeId);

	// Marked targetNode as visited
	//playerOwned(command.playerId).TryAddArmyNodeVisited(targetNode.nodeId);

	//auto makeMarchGroup = [&](std::vector<int32>& armyCountsIn, int32 helpPlayerId) {
	//	return ArmyGroup(command.playerId, command.targetNodeId, armyCountsIn, helpPlayerId);
	//};

	CallbackEnum orderEnum = command.armyOrderEnum;

	//if (orderEnum == CallbackEnum::ArmyRebel)
	//{
	//	// Rebeling: switch ordering player's army to attacking army...
	//	ArmyGroup* rebelGroup1 = originNode.GetRebelGroup(command.playerId);
	//	if (rebelGroup1) {
	//		ArmyGroup rebelGroup = CppUtils::RemoveOneIf(targetNode.rebelGroups, [&](const ArmyGroup& group) { return group.playerId == command.playerId; });
	//		targetNode.attackGroups.push_back(rebelGroup);
	//	}
	//}
	//else if (orderEnum == CallbackEnum::ArmyConquer)
	//{
	//	ArmyGroup* group = originNode.GetArmyGroup(command.playerId);
	//	if (group) {
	//		std::vector<int32> armyCounts = group->RemoveArmyPartial(CppUtils::ArrayToVec(command.armyCounts));
	//		GetArmyNode(command.targetNodeId).MarchStart(makeMarchGroup(armyCounts, -1), originNode);

	//		AddPopup(targetNode.originalPlayerId, 
	//			FText::Format(LOCTEXT("ArmyConquer_TargetPop", "{0} had launched an attack against you. If you lose this battle, you will become {0}'s vassal"), playerNameT(command.playerId))
	//		);
	//	}
	//}
	//else if (orderEnum == CallbackEnum::ArmyRecall)
	//{
	//	ArmyGroup* groupPtr = originNode.GetArmyGroup(command.playerId);
	//	if (groupPtr) {
	//		// existing army in attack/defense group depart to targetNode
	//		std::vector<int32> armyCounts = groupPtr->GetArmyCounts();
	//		groupPtr->ClearArmy();
	//		GetArmyNode(command.targetNodeId).MarchStart(makeMarchGroup(armyCounts, command.playerId), originNode);
	//	}

	//	// if there are arriving armies calculate the tile where it is, and go to targetNode
	//	std::vector<ArmyGroup> playerArrivalGroups = originNode.RemoveArrivalArmyGroups(command.playerId);
	//	for (ArmyGroup& arrGroup : playerArrivalGroups) {
	//		std::vector<int32> armyCounts = arrGroup.GetArmyCounts();
	//		GetArmyNode(command.targetNodeId).MarchStart(makeMarchGroup(armyCounts, command.playerId), originNode, ArmyUtils::GetArmyTile(arrGroup, this));
	//	}
	//}
	//else if (orderEnum == CallbackEnum::ArmyReinforce)
	//{
	//	ArmyGroup* group = originNode.GetArmyGroup(command.playerId);
	//	if (group) 
	//	{
	//		ArmyGroup* targetGroup = targetNode.GetArmyGroup(command.playerId);
	//		if (!targetGroup) {
	//			targetGroup = targetNode.GetArrivingGroup(command.playerId); // Use arriving group as example instead of there is no army at destination
	//		}
	//		
	//		if (targetGroup) {
	//			std::vector<int32> armyCounts = group->RemoveArmyPartial(CppUtils::ArrayToVec(command.armyCounts));

	//			// when reinforcing, we keep the same intention as the army operating in the region (just in case that army is dead, this army can find the correct purpose)
	//			GetArmyNode(command.targetNodeId).MarchStart(makeMarchGroup(armyCounts, targetGroup->helpPlayerId), originNode);
	//		}
	//	}
	//}
	//else if (orderEnum == CallbackEnum::ArmyHelp)
	//{
	//	ArmyGroup* group = originNode.GetArmyGroup(command.playerId);
	//	if (group)
	//	{
	//		std::vector<int32> armyCounts = group->RemoveArmyPartial(CppUtils::ArrayToVec(command.armyCounts));

	//		// when reinforcing, we keep the same intention as the army operating in the region (just in case that army is dead, this army can find the correct purpose)
	//		GetArmyNode(command.targetNodeId).MarchStart(makeMarchGroup(armyCounts, command.helpPlayerId), originNode);
	//	}
	//}
	//else if (orderEnum == CallbackEnum::ArmyLiberate)
	//{
	//	ArmyGroup* group = originNode.GetArmyGroup(command.playerId);
	//	if (group) {
	//		std::vector<int32> armyCounts = group->RemoveArmyPartial(CppUtils::ArrayToVec(command.armyCounts));
	//		ArmyNode& node = GetArmyNode(command.targetNodeId);
	//		node.MarchStart(makeMarchGroup(armyCounts, node.originalPlayerId), originNode);
	//	}
	//}
	//else if (orderEnum == CallbackEnum::ArmyMoveBetweenNode)
	//{
	//	ArmyGroup* group = originNode.GetArmyGroup(command.playerId);
	//	if (group) {
	//		std::vector<int32> armyCounts = group->RemoveArmyPartial(CppUtils::ArrayToVec(command.armyCounts));
	//		GetArmyNode(command.targetNodeId).MarchStart(makeMarchGroup(armyCounts, command.playerId), originNode);
	//	}
	//}
	
	// Ally
	if (orderEnum == CallbackEnum::AllyRequest)
	{
		int32 requesterPlayerId = building(command.originNodeId).playerId();
		int32 targetPlayerId = building(command.targetNodeId).playerId();
		
		PopupInfo info(targetPlayerId, 
			FText::Format(LOCTEXT("AskAlliance_Pop", "{0} is requesting an alliance with you. Will you accept?"), playerNameT(requesterPlayerId)),
			{ LOCTEXT("Yes", "Yes"),
				LOCTEXT("No", "No") },
			PopupReceiverEnum::AllyRequest
		);
		info.replyVar1 = requesterPlayerId;
		AddPopup(info);
	}
	else if(orderEnum == CallbackEnum::AllyBetray)
	{
		int32 betrayalTargetPlayerId = building<TownHall>(command.targetNodeId).playerId();

		if (townManager(command.playerId).IsAlly(betrayalTargetPlayerId)) 
		{
			LoseAlly(command.playerId, betrayalTargetPlayerId);

			AddPopup(betrayalTargetPlayerId, 
				FText::Format(LOCTEXT("EndedAlliance_TargetPop", "{0} ended the alliance with you."), playerNameT(command.playerId))
			);
			AddPopup(command.playerId, 
				FText::Format(LOCTEXT("EndedAlliance_SelfPop", "You ended the alliance with {0}."), playerNameT(betrayalTargetPlayerId))
			);
		}

		// If your army is stationed here, become an attacker...

		// If your army is marked as helping this ally directly in other nodes the state becomes helpPlayerId == -1 instead...
	}
	//else if (orderEnum == CallbackEnum::ArmyRetreat)
	//{
	//	// original owner retreat... 
	//	ArmyGroup* retreatGroup = targetNode.GetArmyGroup(command.playerId);
	//	if (retreatGroup) {
	//		// The army goes into hiding...
	//		std::vector<int32> armyCounts = retreatGroup->GetArmyCounts();
	//		armyCounts[static_cast<int>(ArmyEnum::Tower)] = 0;
	//		targetNode.rebelGroups.push_back(ArmyGroup(command.playerId, command.targetNodeId, armyCounts, -1));

	//		CppUtils::RemoveIf(targetNode.defendGroups, [&](const ArmyGroup& group) { return group.playerId == command.playerId; });
	//		CppUtils::RemoveIf(targetNode.attackGroups, [&](const ArmyGroup& group) { return group.playerId == command.playerId; });
	//	}
	//}
	else {
		UE_DEBUG_BREAK();
	}
}

void GameSimulationCore::ChooseResearch(FChooseResearch command)
{
	UE_LOG(LogNetworkInput, Log, TEXT(" ChooseResearch"));
	
	unlockSystem(command.playerId)->ChooseResearch(command.techEnum);
}

void GameSimulationCore::ClaimLand(FClaimLand command)
{
	UE_LOG(LogNetworkInput, Log, TEXT(" ClaimLand"));
	int32 attackerPlayerId = command.playerId;
	

	// Get Military Units...
	std::vector<CardStatus> militaryCards;
	if (command.cardEnums.Num() > 0)
	{
		cardSystem(command.playerId).pendingMilitarySlotCards.clear();

		for (int32 i = 0; i < command.cardEnums.Num(); i++) {
			CardStatus cardStatus(static_cast<CardEnum>(command.cardEnums[i]), command.cardCount[i]);
			militaryCards.push_back(cardStatus);
			cardSystem(command.playerId).RemoveCardsFromBoughtHand(cardStatus, cardStatus.stackSize);
		}
	}


	auto& globalResourceSys = globalResourceSystem(attackerPlayerId);

	/*
	 * No Battle
	 */
	if (command.claimEnum == CallbackEnum::ClaimLandMoney ||
		command.claimEnum == CallbackEnum::ClaimLandInfluence)
	{
		//int32 attackerTownId = GetProvinceAttackerTownId(attackerPlayerId, provinceId);
		//check(attackerTownId != -1);

		int32 provincePrice = GetProvinceClaimPrice(command.provinceId, attackerPlayerId);
		
		if (command.claimEnum == CallbackEnum::ClaimLandMoney)
		{
			int32 provincePriceMoney = provincePrice * GameConstants::ClaimProvinceByMoneyMultiplier;

			if (globalResourceSys.moneyCap32() >= provincePriceMoney &&
				provinceOwnerTown_Major(command.provinceId) == -1)
			{
				SetProvinceOwner_Popup(command.provinceId, attackerPlayerId, true);
				//SetProvinceOwnerFull(command.provinceId, attackerTownId);
				
				globalResourceSys.ChangeMoney(-provincePriceMoney);
			}
		}
		else if (command.claimEnum == CallbackEnum::ClaimLandInfluence)
		{
			if (globalResourceSys.influence() >= provincePrice &&
				provinceOwnerTown_Major(command.provinceId) == -1)
			{
				SetProvinceOwner_Popup(command.provinceId, attackerPlayerId, true);
				//SetProvinceOwnerFull(command.provinceId, attackerTownId);
				
				globalResourceSys.ChangeInfluence(-provincePrice);
			}
		}
	}
	/*
	 * Battle
	 */
	else if (command.claimEnum == CallbackEnum::StartAttackProvince ||
			command.claimEnum == CallbackEnum::Liberate ||
			command.claimEnum == CallbackEnum::RaidBattle)
	{
		// Attack could be: Conquer Province, Vassalize, or Declare Independence

		int32 provinceTownId = provinceOwnerTown_Major(command.provinceId);
		int32 provincePlayerId = provinceOwnerPlayer(command.provinceId);
		check(provincePlayerId != -1);
		TownManagerBase* provinceTownManager = townManagerBase(provincePlayerId);
		

		ProvinceAttackEnum attackEnum = GetProvinceAttackEnum(command.playerId, command.provinceId, command.claimEnum);

		bool canAttack = true;

		if (attackEnum == ProvinceAttackEnum::Vassalize ||
			attackEnum == ProvinceAttackEnum::VassalCompetition) {
			canAttack = CanVassalizeOtherPlayers(command.playerId);
		}
		if (attackEnum == ProvinceAttackEnum::ConquerProvince) {
			canAttack = CanConquerProvinceOtherPlayers(command.playerId);
		}
		if (attackEnum == ProvinceAttackEnum::ConquerColony) {
			canAttack = CanConquerColony(provinceTownId, command.playerId);
		}

		// If there was no claim yet, start the attack
		if (canAttack &&
			!provinceTownManager->GetDefendingClaimProgress(command.provinceId).isValid())
		{
			
			//int32 conquerPrice = 0;
			//switch (attackEnum)
			//{
			//case ProvinceAttackEnum::ConquerProvince: conquerPrice = GetProvinceAttackStartPrice(provinceId, GetProvinceClaimConnectionEnumPlayer(command.provinceId, command.playerId)); break;
			//case ProvinceAttackEnum::Vassalize: conquerPrice = GetProvinceVassalizeStartPrice(provinceId); break;
			//case ProvinceAttackEnum::DeclareIndependence: conquerPrice = BattleInfluencePrice; break; // Declare Independence has no defense/attack advantage...
			//case ProvinceAttackEnum::ConquerColony: conquerPrice = GetProvinceConquerColonyStartPrice(provinceId); break;
			//default: break;
			//}

			//TODO: special AI
			//if (IsAIPlayer(command.playerId)) {
			//	globalResourceSys.ChangeInfluence(conquerPrice * 2);
			//}
			
			//if (influence(command.playerId) >= conquerPrice) 
			//{
			//globalResourceSys.ChangeInfluence(-conquerPrice);

			attackerPlayerId = (command.claimEnum == CallbackEnum::Liberate) ? provincePlayerId : command.playerId;

			/*
			 * Conquer Part!!!
			 */
			provinceTownManager->StartAttack_Defender(attackerPlayerId, command.provinceId, attackEnum, militaryCards);
			townManagerBase(attackerPlayerId)->StartAttack_Attacker(command.provinceId);

			//! Popups
			if (attackEnum == ProvinceAttackEnum::RaidBattle)
			{
				if (command.provinceId != -1 && provincePlayerId != -1)
				{
					int32 raidMoney100 = GetProvinceRaidMoney100(command.provinceId);
					int32 raidInfluence100 = raidMoney100 / 2;

					AddPopup(provincePlayerId, FText::Format(
							LOCTEXT("RaidHandleDecision_Pop", "{0} is launching a raid against you.<space>You can ignore the raid and stay in a fortified position or fight in an open field (without defensive advantage).<bullet> If you stay fortified, you will lose <img id=\"Coin\"/>{1} and <img id=\"Influence\"/>{2}.</>If you fight, you may lose your army.<bullet></>"),
							playerNameT(attackerPlayerId),
							TEXT_100(raidMoney100),
							TEXT_100(raidInfluence100)
					));
				}
			}
			else if (attackEnum == ProvinceAttackEnum::ConquerProvince) 
			{
				AddPopup(provincePlayerId, 
					FText::Format(
						LOCTEXT("GotAttacked_Pop", "{0} is trying to take over your territory.<space>If you lose the province, all its buildings will be destroyed."), 
						playerNameT(command.playerId)
					)
				);

				AddPopup(command.playerId, 
					FText::Format(LOCTEXT("YouAttack_Pop", "You started attacking {0}."), playerNameT(provincePlayerId))
				);

				if (IsAIPlayer(provincePlayerId)) {
					aiPlayerSystem(provincePlayerId).DeclareWar(command.playerId);
				}
			}
			else if (attackEnum == ProvinceAttackEnum::Vassalize) 
			{
				int32 oldLordPlayerId = provinceTownManager->lordPlayerId();
				if (oldLordPlayerId != -1) { // Already a lord here, fight against old lord
					AddPopup(oldLordPlayerId,
						FText::Format(
							LOCTEXT("VassalGotAttack", "{0} started attacking on your vassal city, {1}, to gain control of the vassal.<space>If you lose this battle, you will lose control of the vassal."),
							playerNameT(command.playerId),
							playerNameT(provincePlayerId)
						)
					);
					
					AddPopup(provincePlayerId, 
						FText::Format(
							LOCTEXT("AttackYourCityReplaceOldLord_Pop", "{0} started attacking on your city, to expel and replace your old lord, {1}."),
							playerNameT(command.playerId),
							playerNameT(oldLordPlayerId)
						)
					);
					
					AddPopup(command.playerId, 
						FText::Format(
							LOCTEXT("YouStartedAttackingForVassal_Pop", "You started attacking {0} to take {1} as your vassal."),
							playerNameT(oldLordPlayerId),
							playerNameT(provincePlayerId)
						)
					);
				}
				else {
					AddPopup(provincePlayerId,
						FText::Format(
							LOCTEXT("XTryToVassalizeYou_Pop", "{0} started attacking to vassalize you. If you lose this battle, you will become {0}'s vassal"),
							playerNameT(command.playerId)
						)
					);
					AddPopup(command.playerId,
						FText::Format(
							LOCTEXT("YouStartVassalize_Pop", "You started attacking to vassalize {0}."),
							playerNameT(provincePlayerId)
						)
					);
				}

				if (IsAIPlayer(provincePlayerId)) {
					aiPlayerSystem(provincePlayerId).DeclareWar(command.playerId);
				}
			}
			else if (attackEnum == ProvinceAttackEnum::ConquerColony)
			{
				// Defender
				AddPopup(provincePlayerId,
					FText::Format(
						LOCTEXT("XTryToConquerYourTown_Pop", "{0} started attacking to take control of {1} from you. If you lose this battle, you will lose control of the town"),
						playerNameT(command.playerId),
						townNameT(provinceTownId)
					)
				);
				
				// Attacker
				AddPopup(command.playerId,
					FText::Format(
						LOCTEXT("YouStartConquerTown_Pop", "You started attacking to gain control of {0} from {1}."),
						townNameT(provinceTownId),
						playerNameT(provincePlayerId)
					)
				);

				if (IsAIPlayer(provincePlayerId)) {
					aiPlayerSystem(provincePlayerId).DeclareWar(command.playerId);
				}
			}
			else if (attackEnum == ProvinceAttackEnum::DeclareIndependence) 
			{
				int32 lordId = lordPlayerId(provincePlayerId);
				
				if (command.claimEnum == CallbackEnum::Liberate)
				{
					AddPopup(lordId,
						FText::Format(
							LOCTEXT("LiberatePlayer_LordPop", "{0} is trying to liberate {1} from you.<space>If you lose this battle, you will lose control of the vassal."),
							playerNameT(command.playerId),
							playerNameT(provincePlayerId)
						)
					);

					AddPopup(command.playerId,
						FText::Format(
							LOCTEXT("LiberatePlayer_SelfPop", "You attempt to liberate {0} from {1}."),
							playerNameT(provincePlayerId),
							playerNameT(lordId)
						)
					);

					AddPopup(provincePlayerId,
						FText::Format(
							LOCTEXT("LiberatePlayer_ProvincePlayerPop", "{0} tries to liberate your city from {1}."),
							playerNameT(command.playerId),
							playerNameT(lordId)
						)
					);
				}
				else
				{
					AddPopup(lordId,
						FText::Format(
							LOCTEXT("XTryToDeclareIndependenceFromYou_Pop", "{0} is trying to declare independence from you.<space>If you lose this battle, you will lose control of the vassal."),
							playerNameT(provincePlayerId)
						)
					);

					AddPopup(provincePlayerId,
						FText::Format(
							LOCTEXT("YouTryToDeclareIndependenceFromX_Pop", "You attempt to declare independence from {0}."),
							playerNameT(lordId)
						)
					);
				}
			}
		}
	}
	
	else if (command.claimEnum == CallbackEnum::ReinforceAttackProvince)
	{
		auto& provinceTownManager = townManager(provinceOwnerTownSafe(command.provinceId));
		ProvinceClaimProgress* claimProgress = provinceTownManager.GetDefendingClaimProgressPtr(command.provinceId);
		
		if (claimProgress) {
			claimProgress->Reinforce(militaryCards, true, command.playerId);
		}
	}
	
	else if (command.claimEnum == CallbackEnum::ReinforceDefendProvince)
	{
		auto& provinceTownManager = townManager(provinceOwnerTownSafe(command.provinceId));
		ProvinceClaimProgress* claimProgress = provinceTownManager.GetDefendingClaimProgressPtr(command.provinceId);

		if (claimProgress) {
			claimProgress->Reinforce(militaryCards, false, command.playerId);
		}
	}
	
	else if (command.claimEnum == CallbackEnum::BattleRetreat)
	{
		auto& provinceTownManager = townManager(provinceOwnerTownSafe(command.provinceId));
		ProvinceClaimProgress* claimProgress = provinceTownManager.GetDefendingClaimProgressPtr(command.provinceId);
		
		if (claimProgress) {
			// Return Military Units
			provinceTownManager.ReturnMilitaryUnitCards(claimProgress->attackerFrontLine, command.playerId, false);
			provinceTownManager.ReturnMilitaryUnitCards(claimProgress->attackerBackLine, command.playerId, false);
			provinceTownManager.ReturnMilitaryUnitCards(claimProgress->defenderFrontLine, command.playerId, false);
			provinceTownManager.ReturnMilitaryUnitCards(claimProgress->defenderBackLine, command.playerId, false);

			claimProgress->Retreat(command.playerId);
		}
	}
	//else if (command.claimEnum == CallbackEnum::DefendProvinceMoney)
	//{
	//	auto& provincePlayerOwner = playerOwned(provinceOwnerPlayer(command.provinceId));

	//	if (provincePlayerOwner.GetDefendingClaimProgress(command.provinceId).isValid() &&
	//		moneyCap32(command.playerId) >= BattleInfluencePrice)
	//	{
	//		ChangeMoney(command.playerId, -BattleInfluencePrice);
	//		provincePlayerOwner.ReinforceDefender(command.provinceId, BattleInfluencePrice);
	//	}
	//}
	
	else {
		UE_DEBUG_BREAK();
	}
}

void GameSimulationCore::SetProvinceOwner(int32 provinceId, int32 townId, bool lightMode)
{
	int32 playerId = townPlayerId(townId);
	
	// When transfering land if it is oversea, warn of 200% influence upkeep
	if (playerId != -1 &&
		GetProvinceClaimConnectionEnumPlayer(provinceId, playerId) == ClaimConnectionEnum::Deepwater)
	{
		AddPopup(playerId,
			LOCTEXT("OverseaPenalty_Pop", "You have claimed a Province oversea.<space>Oversea provinces have upkeep penalty of +200%")
		);
	}

	int32 oldTownId = provinceOwnerTown_Major(provinceId);
	if (oldTownId != -1) {
		townManagerBase(oldTownId)->TryRemoveProvinceClaim(provinceId, lightMode); // TODO: Try moving this below???
	}

	_regionSystem->SetProvinceOwner(provinceId, townId, lightMode);

	if (townId != -1) {
		townManagerBase(townId)->ClaimProvince(provinceId, lightMode);

		if (!lightMode) {
			RefreshTerritoryEdge(townId);
		}
	}
	if (oldTownId != -1) {
		if (!lightMode) {
			RefreshTerritoryEdge(oldTownId);
		}
	}
}

void GameSimulationCore::SetProvinceOwnerFull(int32 provinceId, int32 townId)
{
	PUN_CHECK(_provinceSystem.IsProvinceValid(provinceId));
	PUN_CHECK(townId != -1);

	SetProvinceOwner(provinceId, townId);

	int32 playerId = townManager(townId).playerId();

	// Succeed claiming land
	{
		// Unlock bridge if there is a river or coast
		if (_provinceSystem.provinceRiverTileCount(provinceId) > 0 ||
			_provinceSystem.provinceOceanTileCount(provinceId) > 0)
		{
			if (!unlockSystem(playerId)->isUnlocked(CardEnum::Bridge)) {
				AddPopupToFront(playerId, 
					LOCTEXT("Unlocked bridge!", "Unlocked bridge!"),
					ExclusiveUIEnum::None, "PopupNeutral"
				);
				unlockSystem(playerId)->UnlockBuilding(CardEnum::Bridge);
			}
		}
		
		QuestUpdateStatus(playerId, QuestEnum::ClaimLandQuest);

		// Desert, unlock the desert pilgrim card
		//if (GetBiomeProvince(provinceId) == BiomeEnum::Desert &&
		//	!parameters(playerId)->DesertPilgrimOffered)
		//{
		//	cardSystem(playerId).AddDrawCards(CardEnum::DesertPilgrim, 1);
		//	parameters(playerId)->DesertPilgrimOffered = true;
		//}

		// Taking over proper land grants seeds or unlocks proper mine
		CheckSeedAndMineCard(playerId);
		
		// Claim land hand
		int32 claimCount = GetProvinceCountPlayer(playerId);

		// Remove any existing regional building and give the according bonus...
		if (claimCount > 1)
		{
			ExecuteOnProvinceBuildings(provinceId, [&](Building& bld, const std::vector<WorldRegion2>& regionOverlaps)
			{
				if (bld.isEnum(CardEnum::RegionTribalVillage))
				{
					// Clear Wildmen
					for (WorldRegion2 curRegionOverlap : regionOverlaps) {
						unitSubregionLists().ExecuteRegion(curRegionOverlap, [&](int32_t unitId)
						{
							if (unitEnum(unitId) == UnitEnum::WildMan)
							{
								WorldTile2 curTile = unitAtom(unitId).worldTile2();
								if (GetProvinceIdClean(curTile) == provinceId) {
									unitAI(unitId).Die();
								}
							}
						});
					}

					ImmigrationEvent(townId, 5,
						FText::Format(LOCTEXT("TribalImmigrantAsk_Pop", "{0} wish to join your city."), GenerateTribeName(bld.buildingId())),
						PopupReceiverEnum::TribalJoinEvent
					);
					//townhall(playerId).ImmigrationEvent(5, GenerateTribeName(bld.buildingId()) + " wish to join your city.", PopupReceiverEnum::TribalJoinEvent);
					ClearProvinceBuildings(bld.provinceId());
				}
				else if (bld.isEnum(CardEnum::RegionCrates)) {
					GenerateRareCardSelection(playerId, RareHandEnum::BuildingSlotCards, LOCTEXT("CratesUseCardSelection", "Searching through the crates you found."));
					ClearProvinceBuildings(bld.provinceId());
				}
				else if (bld.isEnum(CardEnum::RegionShrine)) {
					GenerateRareCardSelection(playerId, RareHandEnum::BuildingSlotCards, LOCTEXT("ShrineUseCardSelection", "The Shrine bestows its wisdom upon us."));
					//bld.subclass<RegionShrine>().PlayerTookOver(playerId);
				}
			});
			
			//const std::vector<WorldRegion2>& regionOverlaps = _provinceSystem.GetRegionOverlaps(provinceId);

			//for (WorldRegion2 regionOverlap : regionOverlaps)
			//{
			//	auto& buildingList = buildingSystem().buildingSubregionList();
			//	buildingList.ExecuteRegion(regionOverlap, [&](int32 buildingId)
			//	{
			//		auto bld = building(buildingId);
			//		if (IsRegionalBuilding(bld.buildingEnum()) && 
			//			GetProvinceIdClean(bld.centerTile()) == provinceId)
			//		{
			//			

			//		}
			//	});
			//}
		}

	}
}

void GameSimulationCore::SetProvinceOwner_Popup(int32 provinceId, int32 attackerPlayerId, bool isFull)
{
	vector<int32> attackerTownIds = GetProvinceAttackerTownIds(attackerPlayerId, provinceId);
	
	if (attackerTownIds.size() > 1)
	{	
		vector<FText> attackerTownNames;
		for (int32 attackerTownId : attackerTownIds) {
			attackerTownNames.push_back(townNameT(attackerTownId));
		}

		AddPopup(PopupInfo(attackerPlayerId,
			LOCTEXT("ChooseTownToClaimWith_Popup", "Which town should claim the province?"),
			attackerTownNames,
			PopupReceiverEnum::ChooseTownToClaimWith, true, "",
			provinceId, isFull
		));
	}
	else if (attackerTownIds.size() == 1) {
		if (isFull) {
			SetProvinceOwnerFull(provinceId, attackerTownIds[0]);
		} else {
			SetProvinceOwner(provinceId, attackerTownIds[0]);
		}
	}
}


void GameSimulationCore::ChooseLocation(FChooseLocation command)
{
	_LOG(LogNetworkInput, " ChooseLocation");

	// No duplicate choosing...
	if (playerOwned(command.playerId).hasChosenLocation()) {
		return;
	}
	
	if (command.isChoosingOrReserving)
	{
		// Find the start spot to double check
		TileArea startArea = SimUtils::FindStartSpot(command.provinceId, this);

		if (!startArea.isValid()) {
			return;
		}

		// Ensure province price is less than Initial Money
		int32 provincePrice = GetProvinceClaimPrice(command.provinceId, command.playerId, ClaimConnectionEnum::Flat) * GameConstants::ClaimProvinceByMoneyMultiplier;
		if (provincePrice > GameConstants::InitialMoney) {
			AddPopup(command.playerId, 
				LOCTEXT("NoMoneyToBuyInitialProvince", "Not enough initial money to buy the province."), 
				"PopupCannot"
			);
			return;
		}

		// Shave off storage yard
		startArea.minY += 4;
		WorldTile2 townhallCenter = startArea.centerTile();
		
		PUN_DEBUG(FString("ChooseLocation GameSimulationCore"));
		PUN_LOG("ChooseLocation %d %s", command.playerId, ToTChar(WorldRegion2(command.provinceId).ToString()));
		
		// Clear any regional building
		ClearProvinceBuildings(command.provinceId);

		// Claim Land
		SetProvinceOwnerFull(command.provinceId, command.playerId);
		
		// For Camera Movement
		playerOwned(command.playerId).justChoseLocation = true;


		// Give money/seeds
		globalResourceSystem(command.playerId).SetMoney(GameConstants::InitialMoney - provincePrice);
		globalResourceSystem(command.playerId).SetInfluence(0);

		// EventLog inform all players someone selected a start
		if (!IsAIPlayer(command.playerId))
		{
			ExecuteOnConnectedPlayers([&](int32 playerId) {
				AddEventLog(playerId, 
					FText::Format(LOCTEXT("ChoseStart_Event", "{0} chose a starting location."), playerNameT(command.playerId)),
					false
				);
			});
		}

	}
}

void GameSimulationCore::ChooseInitialResources(FChooseInitialResources command)
{
	if (!cardSystem(command.playerId).HasBoughtCard(CardEnum::Townhall)) {
		cardSystem(command.playerId).AddCardToHand2(CardEnum::Townhall);
		
		playerOwned(command.playerId).initialResources = command;
		globalResourceSystem(command.playerId).ChangeMoney(-(command.totalCost() - FChooseInitialResources::GetDefault().totalCost()));
	}
}

void GameSimulationCore::Cheat(FCheat command)
{
	std::string cheatName = GetCheatName(command.cheatEnum);
	UE_LOG(LogNetworkInput, Log, TEXT(" Cheat %s var1:%d var2:%d str1:%s"), ToTChar(cheatName), command.var1, command.var2, *command.stringVar1);

	auto& cardSys = cardSystem(command.playerId);
	
	switch (command.cheatEnum)
	{
		case CheatEnum::UnlockAll: {
			unlockSystem(command.playerId)->researchEnabled = true;
			unlockSystem(command.playerId)->UnlockAll();
			_popupSystems[command.playerId].ClearPopups();
			cardSys.ClearBoughtCards();
			break;
		}
		case CheatEnum::Money: {
			ChangeMoney(command.playerId, 3000000);
			PunSettings::Set("ForceQuickBuild", 1);
			break;
		}
		case CheatEnum::Influence: ChangeInfluence(command.playerId, 10000); break;
		
		case CheatEnum::FastBuild: SimSettings::Toggle("CheatFastBuild"); break;
		
		case CheatEnum::Cheat:
			SimSettings::Set("CheatFastBuild", 1);
			ChangeMoney(command.playerId, 100000);

			GetTownhallCapital(command.playerId).AddImmigrants(50);

			unlockSystem(command.playerId)->researchEnabled = true;
			unlockSystem(command.playerId)->UnlockAll();
		
			cardSys.ClearBoughtCards();
			cardSys.ClearRareHands();
			_popupSystems[command.playerId].ClearPopups();
		
		//case CheatEnum::Army:
		//{
		//	std::vector<int32> armyNodeIds = GetArmyNodeIds(command.playerId);
		//	for (int32 armyNodeId : armyNodeIds) {
		//		std::vector<int32> armyCounts(ArmyEnumCount, 0);
		//		armyCounts[1] = 5;
		//		armyCounts[2] = 5;
		//		armyCounts[3] = 5;
		//		GetArmyNode(armyNodeId).AddArmyToCapital(command.playerId, armyCounts);
		//	}
		//	break;
		//}
		
		case CheatEnum::Resources: {
			for (ResourceEnum resourceEnum : StaticData::FoodEnums) {
				resourceSystem(command.playerId).AddResourceGlobal(resourceEnum, 50, *this);
			}
			for (ResourceEnum resourceEnum : ConstructionResources) {
				resourceSystem(command.playerId).AddResourceGlobal(resourceEnum, 200, *this);
			}
			ExecuteOnLuxuryResources([&](ResourceEnum resourceEnum) {
				resourceSystem(command.playerId).AddResourceGlobal(resourceEnum, 50, *this);
			});
			//for (ResourceEnum resourceEnum : LuxuryResources) {
			//	resourceSystem(command.playerId).AddResourceGlobal(resourceEnum, 50, *this);
			//}
			for (ResourceEnum resourceEnum : MiscResources) {
				resourceSystem(command.playerId).AddResourceGlobal(resourceEnum, 50, *this);
			}
			break;
		}
		case CheatEnum::ConstructionResources: {
			for (ResourceEnum resourceEnum : ConstructionResources) {
				resourceSystem(command.playerId).AddResourceGlobal(resourceEnum, 1000, *this);
			}
			break;
		}
		case CheatEnum::Undead: SimSettings::Toggle("CheatUndead"); break;
		case CheatEnum::Immigration: GetTownhallCapital(command.playerId).ImmigrationEvent(30); break;

		case CheatEnum::FastTech: SimSettings::Toggle("CheatFastTech"); break;

		case CheatEnum::ForceFunDown:
		{
			auto setFunDown = [&](const std::vector<int32>& humanIds) {
				for (int32 humanId : humanIds) {
					unitAI(humanId).SetFunTicks(FunTicksAt100Percent * 20 / 100);
				}
			};
			setFunDown(townManager(command.playerId).adultIds());
			setFunDown(townManager(command.playerId).childIds());
			break;
		}
		case CheatEnum::ForceFoodDown:
		{
			auto setFoodDown = [&](const std::vector<int32>& humanIds) {
				for (int32 humanId : humanIds) {
					unitAI(humanId).SetFood(unitAI(humanId).foodThreshold_Get2());
				}
			};
			setFoodDown(townManager(command.playerId).adultIds());
			setFoodDown(townManager(command.playerId).childIds());
			break;
		}
		case CheatEnum::ForceSick:
		{
			auto set = [&](const std::vector<int32>& humanIds) {
				for (int32 humanId : humanIds) {
					unitAI(humanId).SetSick(true);
				}
			};
			set(townManager(command.playerId).adultIds());
			set(townManager(command.playerId).childIds());
			break;
		}

		case CheatEnum::Kill: {
			const std::vector<int32>& humanIds = townManager(command.playerId).humanIds();
			for (int32 i = humanIds.size(); i-- > 0;) {
				UnitStateAI& unit = unitAI(humanIds[i]);
				PUN_LOG("Kill %d %d", humanIds[i], unit.id());
				check(humanIds[i] == unit.id());
				unit.Die();
			}
			//townManager(playerId()).ExecuteOnPopulation([&](int32 humanId) {
			//	UnitStateAI& unit = unitAI(humanId);
			//	PUN_LOG("Kill %d %d", humanId, unit.id());
			//	unit.Die();
			//	
			//});
			break;
		}

		case CheatEnum::ClearLand: {
			// Clear trees/deposits from regions owned
			const std::vector<int32>& provinceIds = GetProvincesPlayer(command.playerId);
			for (int32 provinceId : provinceIds) {
				_provinceSystem.ExecuteOnProvinceTiles(provinceId, [&](WorldTile2 tile)
				{
					ResourceTileType type = treeSystem().tileInfo(tile.tileId()).type;
					if (type == ResourceTileType::Bush) {
						treeSystem().UnitHarvestBush(tile);
					}
					else if (type == ResourceTileType::Tree || type == ResourceTileType::Deposit) {
						treeSystem().UnitHarvestTreeOrDeposit(tile);
					}
				});
			}
			break;
		}
		case CheatEnum::Unhappy:
		{
			cardSys.AddCardToHand2(CardEnum::Cannibalism);
			cardSys.AddCardToHand2(CardEnum::Cannibalism);
			break;
		}

		case CheatEnum::AddHuman:
		{
			AddUnit(UnitEnum::Human, 0, WorldTile2(command.var1, command.var2).worldAtom2(), Time::TicksPerYear);
			break;
		}
		case CheatEnum::AddAnimal:
		{
			WorldTile2 tile(command.var1, command.var2);
			if (pathAI()->isWalkable(tile.x, tile.y)) {
				AddUnit(UnitEnum::RedDeer, 0, tile.worldAtom2(), Time::TicksPerYear);
			}
			break;
		}
		case CheatEnum::AddWildMan:
		{
			WorldTile2 tile(command.var1, command.var2);
			if (pathAI()->isWalkable(tile.x, tile.y)) {
				AddUnit(UnitEnum::WildMan, 0, tile.worldAtom2(), Time::TicksPerYear);
			}
			break;
		}
		case CheatEnum::KillUnit:
		{
			int32 humanId = command.var1; 
			if (_unitSystem->IsUnitValid(humanId))
			{
				UnitStateAI& unit = unitAI(humanId);
				PUN_LOG("Kill %d %d", humanId, unit.id());
				check(humanId == unit.id());
				unit.Die();
			}
			break;
		}
		case CheatEnum::SpawnDrop:
		{
			WorldTile2 tile(command.var1, command.var2);
			if (pathAI()->isWalkable(tile.x, tile.y)) 
			{
				// Drops
				std::vector<ResourcePair> drops = GetUnitInfo(UnitEnum::Boar).resourceDrops100;

				// Random 100 to 1, add efficiency
				for (size_t i = 0; i < drops.size(); i++)
				{
					PUN_CHECK(drops[i].count >= 100);
					drops[i].count = GameRand::Rand100RoundTo1(drops[i].count);
					drops[i].count = max(1, drops[i].count);
				}

				for (ResourcePair& drop : drops) {
					resourceSystem(0).SpawnDrop(drop.resourceEnum, drop.count, tile);
				}
			}
			break;
		}
		

		case CheatEnum::AddResource:
		{
			ResourceEnum resourceEnum = static_cast<ResourceEnum>(command.var1);
			int32 amount = command.var2;
			if (amount > 0) {
				resourceSystem(command.playerId).AddResourceGlobal(resourceEnum, amount, *this);
			} else if (amount < 0) {
				int32 amountToRemove = std::min(abs(amount), resourceCountTown(command.playerId, resourceEnum));
				resourceSystem(command.playerId).RemoveResourceGlobal(resourceEnum, amountToRemove);
			}
			break;
		}
		case CheatEnum::AddMoney: {
			ChangeMoney(command.playerId, command.var1);
			break;
		}
		case CheatEnum::AddInfluence: {
			ChangeInfluence(command.playerId, command.var1);
			RecalculateTaxDelayedPlayer(command.playerId);
			break;
		}
		case CheatEnum::AddCard:
		{
			CardEnum cardEnum = static_cast<CardEnum>(command.var1);
			int32 addCount = command.var2;
				
			for (int32 j = 0; j < addCount; j++) {
				cardSys.AddCardToHand2(cardEnum);
			}
			break;
		}
		case CheatEnum::AddImmigrants:
		{
			int32 addCount = command.var1;
			GetTownhallCapital(command.playerId).AddImmigrants(addCount);

			_popupSystems[command.playerId].ClearPopups();
			cardSys.ClearBoughtCards();
			cardSys.ClearRareHands();
			break;
		}

		case CheatEnum::AddAIImmigrants:
		{
			int32 addCount = command.var1;
			ExecuteOnAI([&](int32 playerId) {
				if (HasTownhall(playerId)) {
					GetTownhallCapital(playerId).AddImmigrants(addCount);
				}
			});
				
			break;
		}
		case CheatEnum::AddAIMoney:
		{
			int32 addCount = command.var1;
			ExecuteOnAI([&](int32 playerId) {
				ChangeMoney(playerId, addCount);
			});
			break;
		}
		case CheatEnum::AddAIInfluence:
		{
			int32 addCount = command.var1;
			ExecuteOnAI([&](int32 playerId) {
				ChangeInfluence(playerId, addCount);
			});
			break;
		}

		/*
		 * Test Large City
		 */
		case CheatEnum::TestCity:
		{
			SimSettings::Set("CheatFastBuild", 1);
				
			//SimSettings::Set("CheatHouseLevel", command.var1);
			WorldTile2 curTile = GetTownhallGate(command.playerId);
			if (curTile.isValid())
			{
				int32 addCount = command.var1;
				addCount = std::max(0, std::min(1000, addCount));


				// X ppl per province...
				int32 provinceAddCount = 0;
				while (true)
				{
					int32 lastProvinceAddCount = provinceAddCount;
					
					// Claim one out level at a time
					std::vector<int32> provinceIds = townManager(command.playerId).provincesClaimed();
					for (int32 i = 0; i < provinceIds.size(); i++) {
						std::vector<ProvinceConnection> connections = GetProvinceConnections(provinceIds[i]);
						for (ProvinceConnection connection : connections) {
							if (IsProvinceValid(connection.provinceId) &&
								provinceOwnerTown_Major(connection.provinceId) == -1)
							{
								SetProvinceOwnerFull(connection.provinceId, command.playerId);
								provinceAddCount++;
							}
						}
					}

					if (lastProvinceAddCount == provinceAddCount ||
						provinceAddCount > addCount / 15) 
					{
						break;
					}
				}
				

				auto tryAddRoad = [&](WorldTile2 tile) {
					if (IsFrontBuildable(tile) && !_overlaySystem.IsRoad(tile)) {
						_treeSystem->ForceRemoveTileObj(tile, false);
						overlaySystem().AddRoad(tile, true, true);
					}
				};

				auto placeCluster = [&](CardEnum buildingEnum)
				{
					TileArea startArea(curTile, WorldTile2(14, 14));

					TileArea endArea = SimUtils::SpiralFindAvailableBuildArea(startArea,
						[&](WorldTile2 tile) {
						return IsPlayerBuildable(tile, command.playerId);
					},
						[&](WorldTile2 tile) {
						return WorldTile2::ManDistance(curTile, tile) > 200;
					}
					);

					auto makeBuilding = [&](WorldTile2 shift, CardEnum cardEnum, Direction faceDirection)
					{
						FPlaceBuilding params;
						params.center = endArea.min() + shift;
						params.buildingEnum = static_cast<uint8>(cardEnum);
						params.faceDirection = static_cast<uint8>(faceDirection);
						params.area = BuildingArea(params.center, WorldTile2(6, 6), static_cast<Direction>(params.faceDirection));
						params.playerId = command.playerId;
						PlaceBuilding(params);
					};

					makeBuilding(WorldTile2(3, 3), buildingEnum, Direction::S);
					makeBuilding(WorldTile2(3, 9), buildingEnum, Direction::S);
					makeBuilding(WorldTile2(10, 4), buildingEnum, Direction::N);
					makeBuilding(WorldTile2(10, 10), buildingEnum, Direction::N);

					endArea.ExecuteOnBorder_WorldTile2([&](WorldTile2 tile) {
						tryAddRoad(tile);
					});

					curTile = endArea.min();
				};

				int32 houseClustersNeeded = addCount / (4 * 4) + 1;
				int32 wineryClustersNeeded = addCount / (4 * 6) * 2 / 3;
				int32 warehouseClustersNeeded = houseClustersNeeded / 3;

				int32 maxClusterCount = max(houseClustersNeeded, max(wineryClustersNeeded, warehouseClustersNeeded));
				
				for (int32 i = 0; i < maxClusterCount; i++)
				{
					if (i < houseClustersNeeded) {
						placeCluster(CardEnum::House);
					}
					if (i < wineryClustersNeeded) {
						placeCluster(CardEnum::Winery);
					}
					if (i < warehouseClustersNeeded) {
						placeCluster(CardEnum::Warehouse);
					}
				}

				_popupSystems[command.playerId].ClearPopups();
				cardSys.ClearBoughtCards();
				cardSys.ClearRareHands();

				AddResourceGlobal(command.playerId, ResourceEnum::SteelTools, 2000);
				AddResourceGlobal(command.playerId, ResourceEnum::Coal, 50000);
				AddResourceGlobal(command.playerId, ResourceEnum::Medicine, 10000);
				AddResourceGlobal(command.playerId, ResourceEnum::Grape, 150000);
				
				AddResourceGlobal(command.playerId, ResourceEnum::Furniture, 10000);
				AddResourceGlobal(command.playerId, ResourceEnum::Cannabis, 10000);
				AddResourceGlobal(command.playerId, ResourceEnum::Pottery, 10000);

				AddResourceGlobal(command.playerId, ResourceEnum::Candle, 10000);
				AddResourceGlobal(command.playerId, ResourceEnum::Cloth, 10000);

				AddResourceGlobal(command.playerId, ResourceEnum::LuxuriousClothes, 10000);
				AddResourceGlobal(command.playerId, ResourceEnum::Jewelry, 10000);
				AddResourceGlobal(command.playerId, ResourceEnum::Book, 10000);
			}
				
			break;
		}

		/*
		 * Test City Building Across Network (Desync)
		 */
		case CheatEnum::TestCityNetwork:
		{
			if (command.playerId == gameManagerPlayerId()) 
			{
				if (PunSettings::Get("TestCityNetwork_Stage") > 0) {
					PunSettings::Set("TestCityNetwork_Stage", -1);
				} else {
					PunSettings::Set("TestCityNetwork_Stage", 0);
				}
			}
			break;
		}

		case CheatEnum::DebugUI:
		{	
			PunSettings::Toggle("DebugUI");
			break;
		}

		case CheatEnum::PunTog: 
		case CheatEnum::PunSet:
		case CheatEnum::PunGet:
		{
			if (command.cheatEnum == CheatEnum::PunTog) {
				PunSettings::Toggle(command.stringVar1);
			}
			if (command.cheatEnum == CheatEnum::PunSet) {
				PunSettings::Set(command.stringVar1, command.var1);
			}
				
			auto it = PunSettings::Settings.find(ToStdString(command.stringVar1));
			if (it != PunSettings::Settings.end()) {
				AddPopup(command.playerId, FText::Format(INVTEXT("{0} {1}"), FText::FromString(command.stringVar1), it->second));
			} else {
				AddPopup(command.playerId, FText::Format(INVTEXT("{0} doesn't exists"), FText::FromString(command.stringVar1)));
			}
			break;
		}
		
		case CheatEnum::UnitInfo:
		{
			int32 unitId = command.var1;
			if (unitSystem().IsUnitValid(unitId)) {
				auto& unitAI = unitSystem().unitStateAI(unitId);
				AddPopup(command.playerId, FText::FromString(unitAI.GetUnitDebugInfo()));
				AddPopup(command.playerId, FText::FromString(unitAI.GetUnitActivityHistory()));
			}
			else {
				AddPopup(command.playerId, INVTEXT("Unit doesn't exists"));
			}
			break;
		}
		case CheatEnum::BuildingInfo:
		{
			int32 buildingId = command.var1;
			if (IsValidBuilding(buildingId)) {
				auto& bld = building(buildingId);
				AddPopup(command.playerId, FText::FromString(bld.GetBuildingActionHistory()));
			}
			break;
		}

		case CheatEnum::YearlyTrade:
		{
			{
				AddPopup(PopupInfo(command.playerId,
					LOCTEXT("CaravanArrive_Pop", "A caravan has arrived. They wish to buy any goods you might have."),
					{ LOCTEXT("Trade", "Trade"),
						LOCTEXT("Refuse", "Refuse") },
					PopupReceiverEnum::CaravanBuyer)
				);
			}
			break;
		}


		case CheatEnum::HouseLevel:
		{
			SimSettings::Set("CheatHouseLevel", command.var1);
			FSendChat chat;
			chat.isSystemMessage = true;
			chat.message = "House Lvl " + FString::FromInt(command.var1);
			SendChat(chat);
			break;
		}
		case CheatEnum::HouseLevelKey:
		{
			SimSettings::Toggle("CheatHouseLevelKey");
			break;
		}
		case CheatEnum::FullFarmRoad: {
			PunSettings::SetCheatFullFarmRoad(!PunSettings::CheatFullFarmRoad());
			break;
		}

		case CheatEnum::NoCameraSnap: {
			SimSettings::Toggle("NoCameraSnap");
			break;
		}
		case CheatEnum::GeoresourceAnywhere: {
			SimSettings::Toggle("GeoresourceAnywhere");
			break;
		}
		case CheatEnum::NoFarmSizeCap: {
			SimSettings::Toggle("NoFarmSizeCap");
			break;
		}
		case CheatEnum::MarkedTreesNoDisplay: {
			PunSettings::MarkedTreesNoDisplay = !PunSettings::MarkedTreesNoDisplay;
			break;
		}
		case CheatEnum::WorkAnimate: {
			SimSettings::Toggle("WorkAnimate");
			break;
		}
		case CheatEnum::DemolishNoDrop: {
			SimSettings::Toggle("DemolishNoDrop");
			break;
		}
		case CheatEnum::God: {
			auto setSettings = [&](const FString& name) {
				//SimSettings::Toggle(name);
				SimSettings::Set(name, 1);
			};
				
			PunSettings::SetCheatFullFarmRoad(true);
			setSettings("NoCameraSnap");
			setSettings("GeoresourceAnywhere");
			setSettings("NoFarmSizeCap");
			PunSettings::MarkedTreesNoDisplay = true;
			setSettings("WorkAnimate");
			setSettings("DemolishNoDrop");

			//FCheat cheat;
			//cheat.cheatEnum = static_cast<int32>(CheatEnum::BuyMap);
			//Cheat(cheat);
			//cheat.cheatEnum = static_cast<int32>(CheatEnum::FastBuild);
			//Cheat(cheat);
			//cheat.cheatEnum = static_cast<int32>(CheatEnum::Money);
			//Cheat(cheat);

			break;
		}
		
		case CheatEnum::RemoveAllCards:
		{
			cardSys.ClearBoughtCards();
			break;
		}
		case CheatEnum::BuyMap:
		{
			// Loop through whole map setting
			for (int32 provinceId = 0; provinceId < GameMapConstants::TotalRegions; provinceId++) {
				if (_provinceSystem.IsProvinceValid(provinceId) &&
					_regionSystem->provinceOwner(provinceId) != command.playerId)
				{
					SetProvinceOwner(provinceId, command.playerId);
				}
			}
			break;
		}

		case CheatEnum::ScoreVictory: {
			ExecuteScoreVictory();
			break;
		}

		case CheatEnum::ResourceStatus:
		{
			int32 buildingId = command.var1;
			Building* bld = buildingPtr(buildingId);
			if (bld) {
				int32 resourceInt = command.var2;
				ResourceHolderInfo info = bld->holderInfo(static_cast<ResourceEnum>(resourceInt));
				if (info.isValid()) {
					const ResourceHolder& holder = resourceSystem(command.playerId).holder(info);
					PUN_LOG("ResourceStatus %s current:%d target:%d pop:%d push:%d", *ResourceNameF(info.resourceEnum), holder.current(), holder.target(), holder.reservedPop(), holder.reservedPush());
				}
				else {
					PUN_LOG("ResourceStatus Invalid %s", *ResourceNameF(info.resourceEnum));
				}
			}
			break;
		}
		case CheatEnum::AddAllResources:
		{
			for (int32 i = 0; i < ResourceEnumCount; i++) {
				AddResourceGlobal(command.playerId, static_cast<ResourceEnum>(i), 120);
			}
			break;
		}

		case CheatEnum::GetRoadConstructionCount:
		{
			AddPopup(command.playerId, ToFText("RoadConstructionCount: " + std::to_string(townManager(command.playerId).roadConstructionIds().size())));
			break;
		}
		case CheatEnum::ResetCachedWaypoints:
		{
			for (int32 i = 0; i < BuildingEnumCount; i++) {
				const std::vector<int32>& bldIds = buildingIds(command.playerId, static_cast<CardEnum>(i));
				for (int32 bldId : bldIds) {
					building(bldId).ClearCachedWaypoints();
				}
			}
			break;
		}
		case CheatEnum::GetResourceTypeHolders:
		{
			ResourceEnum resourceEnum = FindResourceEnumByName(ToWString(command.stringVar1));
			if (resourceEnum == ResourceEnum::None) {
				return;
			}
				
			const std::vector<std::vector<ResourceTypeHolders::HolderIdToAmount>>& holders = resourceSystem(command.playerId).GetDebugHolder(resourceEnum).findTypeToAvailableIdToAmount();

			std::stringstream ss;
			for (int32 i = 0; i < holders.size(); i++) 
			{
				ss << ResourceFindTypeName[i] << ":\n";
				
				for (int32 j = 0; j < holders[i].size(); j++) {
					ResourceTypeHolders::HolderIdToAmount holderIdToAmount = holders[i][j];

					const ResourceHolder& holder = resourceSystem(command.playerId).holder(ResourceHolderInfo(resourceEnum, holderIdToAmount.holderId));
					if (holder.objectId != -1) {
						Building& bld = building(holder.objectId);
						ss << " - " << ToStdString(bld.buildingInfo().nameF()) << " " << holderIdToAmount.amount << "\n";
					}
				}
			}
			AddPopup(command.playerId, ToFText(ss.str()));
				
			break;
		}

		case CheatEnum::SaveCameraTransform: {
			_gameManager->ExecuteCheat(CheatEnum::SaveCameraTransform);
			break;
		}
		case CheatEnum::LoadCameraTransform: {
			_gameManager->ExecuteCheat(CheatEnum::LoadCameraTransform);
			break;
		}
		case CheatEnum::TestGetJson: {
			_gameManager->ExecuteCheat(CheatEnum::TestGetJson);
			break;
		}
		case CheatEnum::StepSimulation: {
			_gameManager->ExecuteCheat(CheatEnum::StepSimulation);
			break;
		}
		case CheatEnum::ClearLuxuryTier1: {
			std::vector<int32> houseIds = buildingIds(command.playerId, CardEnum::House);
			for (int32 houseId : houseIds) {
				Building& house = building(houseId);
				int32 beerCount = house.resourceCount(ResourceEnum::Beer);
				house.RemoveResource(ResourceEnum::Beer, beerCount - 1);
			}
			break;
		}
		case CheatEnum::AISpyNest:
		{
			WorldTile2 originPlayerTile(command.var1, command.var2);
			WorldTile2 targetTile(command.var3, command.var4);

			int32 originPlayerId = tileOwnerPlayer(originPlayerTile);
			int32 targetHouseId = buildingIdAtTile(targetTile);
			if (originPlayerId != -1 &&
				IsValidBuilding(targetHouseId) && building(targetHouseId).isEnum(CardEnum::House))
			{
				FGenericCommand spyCommand;
				spyCommand.playerId = originPlayerId;
				spyCommand.callbackEnum = command.var5 == 0 ? CallbackEnum::SpyEstablishNest : CallbackEnum::SpyEnsureAnonymity;
				spyCommand.intVar1 = targetHouseId;
				
				GenericCommand(spyCommand);
			}
			break;
		}
		
		case CheatEnum::TestAITradeDeal:
		{
			auto tradeCommand = make_shared<FGenericCommand>();
			tradeCommand->genericCommandType = FGenericCommand::Type::SendGift;
			tradeCommand->intVar1 = command.var1; // sourceTargetId
			tradeCommand->intVar2 = _gameManager->playerId(); // targetPlayerId
			tradeCommand->intVar3 = 100;
			tradeCommand->intVar4 = 0;

			tradeCommand->intVar5 = static_cast<int32>(TradeDealStageEnum::ExamineDeal);

			tradeCommand->array1.Add(static_cast<int32>(ResourceEnum::Wood)); // Left ResourceEnum
			tradeCommand->array2.Add(100); // Left ResourceCount

			tradeCommand->array5.Add(static_cast<int32>(CardEnum::Townhall)); // Left CardEnum
			tradeCommand->array6.Add(1); // Left CardCount

			_gameManager->SendNetworkCommand(tradeCommand);
			
			break;
		}
		case CheatEnum::TestAIForeignBuild:
		{
			CardEnum cardEnum = FindCardEnumByName(command.stringVar1);
			if (cardEnum == CardEnum::None) {
				return;
			}
				
			auto placeCommand = make_shared<FPlaceBuilding>();
			placeCommand->playerId = command.var1;
			placeCommand->center = WorldTile2(command.var2, command.var3);
			placeCommand->buildingEnum = static_cast<uint8>(cardEnum);
			placeCommand->faceDirection = static_cast<uint8>(Direction::S);
			placeCommand->area = BuildingArea(placeCommand->center, GetBuildingInfo(cardEnum).size, Direction::S);

			check(tileOwnerPlayer(placeCommand->center) != -1);
				
			_gameManager->SendNetworkCommand(placeCommand);
			break;
		}
		
		case CheatEnum::TrailerCityGreen1:
		{
			cardSys.AddCardToHand2(CardEnum::FurnitureWorkshop);
			cardSys.AddCardToHand2(CardEnum::TradingPort); cardSys.AddCardToHand2(CardEnum::TradingPort);
			cardSys.AddCardToHand2(CardEnum::Fisher); cardSys.AddCardToHand2(CardEnum::Fisher);
			cardSys.AddCardToHand2(CardEnum::Windmill);
			cardSys.AddCardToHand2(CardEnum::PaperMaker);
			cardSys.AddCardToHand2(CardEnum::CandleMaker);
			cardSys.AddCardToHand2(CardEnum::Winery);
			cardSys.AddCardToHand2(CardEnum::BeerBrewery);
				
			break;
		}
		case CheatEnum::TrailerCityGreen2:
		{
			cardSys.AddCardToHand2(CardEnum::Tailor);
			cardSys.AddCardToHand2(CardEnum::Beekeeper);
			cardSys.AddCardToHand2(CardEnum::FruitGatherer);
			cardSys.AddCardToHand2(CardEnum::HuntingLodge);
			cardSys.AddCardToHand2(CardEnum::Forester);
			break;
		}
		case CheatEnum::TrailerCityBrown:
		{
			cardSys.AddCardToHand2(CardEnum::Quarry);
			cardSys.AddCardToHand2(CardEnum::CoalMine);
			cardSys.AddCardToHand2(CardEnum::GoldMine);
			cardSys.AddCardToHand2(CardEnum::IronMine);
			cardSys.AddCardToHand2(CardEnum::GemstoneMine);
			cardSys.AddCardToHand2(CardEnum::IronSmelter);
			cardSys.AddCardToHand2(CardEnum::GoldSmelter);
			cardSys.AddCardToHand2(CardEnum::Jeweler);
			break;
		}


		UE_DEBUG_BREAK();
	}
}

std::string GameSimulationCore::unitdebugStr(int id) 
{
	return _unitSystem->debugStr(id); 
}

void GameSimulationCore::unitAddDebugSpeech(int32 id, std::string message)
{
	return _unitSystem->unitStateAI(id).AddDebugSpeech(message);
}

FString GameSimulationCore::GetTileBuildingDescription(WorldTile2 tile)
{
	return "[" + GetBuildingInfo(buildingEnumAtTile(tile)).nameF() + ", " + tile.To_FString();
}

void GameSimulationCore::PlaceInitialTownhallHelper(FPlaceBuilding command, int32 townhallId)
{
	_LOG(PunBuilding, "PlaceInitialTownhallHelper pid:%d canPlace:%d", command.playerId);
	
	auto& playerOwned = _playerOwnedManagers[command.playerId];
	
	CardEnum commandBuildingEnum = static_cast<CardEnum>(command.buildingEnum);
	Building& townhallBld = building(townhallId);
	
	int32 townId = townhallBld.townId();

	/*
	 * Build Townhall
	 */
	{
		FPlaceBuilding params = command;
		WorldTile2 size = GetBuildingInfo(CardEnum::Townhall).size;

		//townhallBld.InstantClearArea();
		//townhallBld.SetAreaWalkable();
		//townhallBld.FinishConstruction();
		townhallBld.TryInstantFinishConstruction();

		// Place road around townhall
		WorldTile2 roadMin(params.area.minX - 1, params.area.minY - 1);
		WorldTile2 roadMax = roadMin + WorldTile2(size.x + 1, size.y + 1);

		std::vector<TileArea> roadAreas;
		roadAreas.push_back(TileArea(roadMin, WorldTile2(size.x + 2, 1)));
		roadAreas.push_back(TileArea(WorldTile2(roadMin.x, roadMax.y), WorldTile2(size.x + 2, 1)));
		roadAreas.push_back(TileArea(WorldTile2(roadMin.x, roadMin.y + 1), WorldTile2(1, size.y)));
		roadAreas.push_back(TileArea(WorldTile2(roadMax.x, roadMin.y + 1), WorldTile2(1, size.y)));

		auto tryAddRoad = [&](WorldTile2 tile) {
			if (IsFrontBuildable(tile) && !_overlaySystem.IsRoad(tile)) {
				PUN_LOG("tryAddRoad %s", ToTChar(tile.ToString()));
				_treeSystem->ForceRemoveTileObj(tile, false);
				overlaySystem().AddRoad(tile, true, true);
			}
		};

		for (size_t i = 0; i < roadAreas.size(); i++) {
			_treeSystem->ForceRemoveTileObjArea(roadAreas[i]);
			roadAreas[i].ExecuteOnArea_WorldTile2([&](WorldTile2 tile) {
				tryAddRoad(tile);
			});
		}

		// Extra colony road
		if (commandBuildingEnum == CardEnum::Colony)
		{
			SimUtils::PlaceColonyAuxRoad(townhallBld.centerTile(), townhallBld.faceDirection(), [&](WorldTile2 tile, Direction faceDirection) {
				tryAddRoad(tile);
			});
		}

		// PlayerOwnedManager
		if (commandBuildingEnum == CardEnum::Townhall) {
			playerOwned.capitalTownhallId = townhallId;
			playerOwned.needChooseLocation = false;
		}
		
		// Townhall
		PUN_LOG("Set TownManager pid:%d", command.playerId);
		townManager(townId).townhallId = townhallId;

		CheckSeedAndMineCard(command.playerId);
		
		//playerOwned.TryAddArmyNodeVisited(townhallId);
	}

	// Build Aux Buildings
	{
		Direction townhallFaceDirection = townhallBld.faceDirection();
		
		FPlaceBuilding params;

		FChooseInitialResources initialResources = playerOwned.initialResources;
		if (IsColonyPlacement(commandBuildingEnum)) {
			initialResources = FChooseInitialResources::GetDefault();
			initialResources.medicineAmount /= 2;
			initialResources.toolsAmount /= 4;
		}

		auto makeBuilding = [&](CardEnum buildingEnum, WorldTile2 buildingSize, WorldTile2 center, Direction initialFaceDirection) -> Building*
		{
			params.center = center;
			params.buildingEnum = static_cast<uint8>(buildingEnum);
			params.faceDirection = static_cast<uint8>(RotateDirection(initialFaceDirection, townhallFaceDirection));
			params.area = BuildingArea(params.center, buildingSize, static_cast<Direction>(params.faceDirection));
			params.playerId = command.playerId;

			PUN_LOG("Place %s area: %d, %d, %d ,%d", *GetBuildingInfo(buildingEnum).nameF(), params.area.minX, params.area.minY, params.area.maxX, params.area.maxY);

			int32 bldId = PlaceBuilding(params);
			if (bldId == -1) {
				return nullptr;
			}

			Building& bld = building(bldId);
			//bld.InstantClearArea();
			//bld.SetAreaWalkable();
			//bld.FinishConstruction();
			bld.TryInstantFinishConstruction();
			
			return &bld;
		};

		if (commandBuildingEnum == CardEnum::Townhall)
		{
			WorldTile2 storageCenter1 = command.center + WorldTile2::RotateTileVector(Storage1ShiftTileVec, townhallFaceDirection);
			WorldTile2 storageCenter2 = storageCenter1 + WorldTile2::RotateTileVector(InitialStorage2Shift, townhallFaceDirection);
			WorldTile2 storageCenter3 = storageCenter1 - WorldTile2::RotateTileVector(InitialStorage2Shift, townhallFaceDirection);
			
			// Storage 1
			{
				Building* storage = makeBuilding(CardEnum::StorageYard, InitialStorageTileSize, storageCenter1, Direction::E);
				PUN_ENSURE(storage, return);
			}

			// Storage 2
			{
				Building* storage = makeBuilding(CardEnum::StorageYard, InitialStorageTileSize, storageCenter2, Direction::E);
				PUN_ENSURE(storage, return);
			}

			// Storage 3 ...
			{
				Building* storage = makeBuilding(CardEnum::StorageYard, InitialStorageTileSize, storageCenter3, Direction::E);
				PUN_ENSURE(storage, return);
			}
		}
		else if (commandBuildingEnum == CardEnum::Colony ||
				commandBuildingEnum == CardEnum::PortColony)
		{
			WorldTile2 buildingCenter1 = townhallBld.centerTile() + WorldTile2::RotateTileVector(PortColony_Storage1ShiftTileVec, townhallFaceDirection);
			WorldTile2 buildingCenter2 = buildingCenter1 + WorldTile2::RotateTileVector(PortColony_InitialStorage2Shift, townhallFaceDirection);

			//Direction auxFaceDirection = RotateDirection(Direction::N, townhallFaceDirection);
			
			// Storage
			Building* storage = makeBuilding(CardEnum::StorageYard, Colony_InitialStorageTileSize, buildingCenter2, Direction::N);
			PUN_ENSURE(storage, return);

			// Intercity LogisticsHub/Port
			Building* bld = nullptr;

			auto& cardSys = cardSystem(command.playerId);
			
			if (commandBuildingEnum == CardEnum::Colony)
			{
				bld = makeBuilding(CardEnum::IntercityLogisticsHub, GetBuildingInfo(CardEnum::IntercityLogisticsHub).size, buildingCenter1, Direction::N);
				PUN_ENSURE(bld, return);

				// Unlock IntercityLogisticsHub
				if (!cardSys.HasCardInAnyPile(CardEnum::IntercityLogisticsHub)) {
					AddPopup(command.playerId, {
						LOCTEXT("IntercityLogisticsHub_Pop", "Unlocked Intercity Logistics Hub.<space>Use Intercity Logistics Hub to transfer goods between your Colony and your main City")
					});
					cardSys.AddDrawCards(CardEnum::IntercityLogisticsHub);
				}
			}
			else {
				WorldTile2 center = buildingCenter1 + WorldTile2::RotateTileVector(PortColony_PortExtraShiftTileVec, townhallFaceDirection);
				bld = makeBuilding(CardEnum::IntercityLogisticsPort, GetBuildingInfo(CardEnum::IntercityLogisticsPort).size, center, Direction::N);
				PUN_ENSURE(bld, return);

				// Unlock IntercityLogisticsHub
				if (!cardSys.HasCardInAnyPile(CardEnum::IntercityLogisticsPort)) {
					AddPopup(command.playerId, {
						LOCTEXT("IntercityLogisticsPort_Pop", "Unlocked Intercity Logistics Port.<space>Use Intercity Logistics Port to transfer goods between your Port Colony and your main City")
					});
					cardSys.AddDrawCards(CardEnum::IntercityLogisticsPort);
				}
			}

			auto& hub = bld->subclass<IntercityLogisticsHub>();
			hub.resourceEnums = {
				ResourceEnum::Food,
				ResourceEnum::Coal,
				ResourceEnum::Medicine,
				ResourceEnum::SteelTools,
			};
			hub.resourceCounts = {
				initialResources.foodAmount,
				initialResources.woodAmount,
				initialResources.medicineAmount,
				initialResources.toolsAmount,
			};
		}

		// Add resources
		if (terrainGenerator().GetBiome(params.center) == BiomeEnum::Jungle) {
			AddResourceGlobal(townId, ResourceEnum::Papaya, initialResources.foodAmount);
		}
		else {
			AddResourceGlobal(townId, ResourceEnum::Orange, initialResources.foodAmount);
		}

		AddResourceGlobal(townId, ResourceEnum::Wood, initialResources.woodAmount);
		AddResourceGlobal(townId, ResourceEnum::Stone,initialResources.stoneAmount);

		AddResourceGlobal(townId, ResourceEnum::Medicine, initialResources.medicineAmount);
		AddResourceGlobal(townId, ResourceEnum::SteelTools, initialResources.toolsAmount);
	}

	/*
	 * Trailer Special Case
	 */
	if (PunSettings::TrailerMode())
	{
		// Buy the large chunk of map
		for (int32 provinceId = 0; provinceId < GameMapConstants::TotalRegions; provinceId++) {
			if (_provinceSystem.IsProvinceValid(provinceId) &&
				_regionSystem->provinceOwner(provinceId) != command.playerId)
			{
				SetProvinceOwner(provinceId, command.playerId, true);
			}
		}

		// Unlimited Money
		ChangeMoney(command.playerId, 50000);

		
		// Build any prebuilt buildings
		std::vector<ReplayPlayer>& replayPlayers = _replaySystem.replayPlayers;
		auto& trailerCommands = replayPlayers[command.playerId].trailerCommands;

		for (shared_ptr<FNetworkCommand>& trailerCommand : trailerCommands)
		{
			if (trailerCommand->commandType() == NetworkCommandEnum::PlaceBuilding)
			{
				auto placeCommand = static_pointer_cast<FPlaceBuilding>(trailerCommand);
				if (placeCommand->isTrailerPreBuilt())
				{
					placeCommand->playerId = command.playerId;
					int32 buildingId = PlaceBuilding(*placeCommand);
					if (buildingId != -1) {
						//building(buildingId).InstantClearArea();
						//building(buildingId).SetAreaWalkable();
						//building(buildingId).FinishConstruction();
						building(buildingId).TryInstantFinishConstruction();
					}
				}
			}
			else if (trailerCommand->commandType() == NetworkCommandEnum::PlaceDrag)
			{
				auto placeCommand = static_pointer_cast<FPlaceDrag>(trailerCommand);
				if (placeCommand->isTrailerPreBuilt()) {
					placeCommand->playerId = command.playerId;
					PlaceDrag(*placeCommand); // PlaceDrag is only for road which is already instant
				}
			}
		}
		
	}
}

void GameSimulationCore::DemolishCritterBuildingsIncludingFronts(WorldTile2 tile, int32 playerId) 
{
	// TODO: Guard GetCritterBuildingsIncludeFronts Crash
	if (!tile.isValid()) {
		return;
	}
	
	std::vector<int32> critterBuildings = GetCritterBuildingsIncludeFronts(tile);
	for (int i = critterBuildings.size(); i-- > 0;) {
		// TODO: later on, mark critter building for demolition, in which it gets attacked until destroyed (dropping items)
		BoarBurrow& burrow = building(critterBuildings[i]).subclass<BoarBurrow>();
		UnitInventory inventory = burrow.inventory;

		_buildingSystem->RemoveBuilding(critterBuildings[i]);

		AddDemolishDisplayInfo(burrow.centerTile(), { CardEnum::BoarBurrow, burrow.area(), Time::Ticks() });
		//_regionToDemolishDisplayInfos[burrow.centerTile().regionId()].push_back({ CardEnum::BoarBurrow, burrow.area(), Time::Ticks() });

		//inventory.Execute([&](ResourcePair& resource) {
		//	if (resource.count > 0) {
		//		resourceSystem(playerId).SpawnDrop(resource.resourceEnum, resource.count, tile);
		//	}
		//});
	}
}

#if CHECK_TICKHASH
void GameSimulationCore::AddTickHash(TickHashes& tickHashes, int32 tickCount, TickHashEnum tickHashEnum, int32 tickHash)
{
	if (PunSettings::IsOn("AlreadyDesynced")) {
		return;
	}
	
	_gameManager->CheckDesync(tickHashes.allTickHashes.Num() == tickHashes.GetTickIndex(tickCount, tickHashEnum), FString("AddTickHash-") + TickHashEnumName[static_cast<int32>(tickHashEnum)]);
	
	//check(tickHashes.allTickHashes.Num() == tickHashes.GetTickIndex(tickCount, tickHashEnum));
	tickHashes.allTickHashes.Add(tickHash);
}
#endif


/*
 * Test for what causes the desync
 */
void GameSimulationCore::TestCityNetworkStage()
{
	if (PunSettings::Get("TestCityNetwork_Stage") == -1) {
		return;
	}

	if (Time::Seconds() % PunSettings::Get("TestCityNetwork_TimeInterval") != 0) {
		return;
	}

	
	const int32 addCount = 500;

	int32 commandPlayerId = gameManagerPlayerId();
	int32 townId = gameManagerPlayerId();
	auto& townManage = townManager(townId);


	// Money
	if (moneyCap32(commandPlayerId) < 1000000)
	{
		auto command = make_shared<FCheat>();
		command->cheatEnum = CheatEnum::Money;
		_gameManager->SendNetworkCommand(command);

		PUN_LOG("TestCityNetworkStage Money");
		return;
	}

	

	// Add enough province
	std::vector<int32> provinceIds = townManage.provincesClaimed();
	if (provinceIds.size() < addCount / 15)
	{
		// Claim one out level at a time
		for (int32 i = 0; i < provinceIds.size(); i++) {
			std::vector<ProvinceConnection> connections = GetProvinceConnections(provinceIds[i]);
			for (ProvinceConnection connection : connections) {
				if (IsProvinceValid(connection.provinceId) &&
					provinceOwnerTown_Major(connection.provinceId) == -1)
				{
					auto command = make_shared<FClaimLand>();
					command->claimEnum = CallbackEnum::ClaimLandMoney;
					command->provinceId = connection.provinceId;
					PUN_CHECK(command->provinceId != -1);
					_gameManager->SendNetworkCommand(command);

					PUN_LOG("TestCityNetworkStage ClaimProvince:%d", connection.provinceId);
					return;
				}
			}
		}
	}

	if (PunSettings::Get("TestCityNetwork_CurTileId") == -1) {
		PunSettings::Set("TestCityNetwork_CurTileId", GetTownhallGateCapital(commandPlayerId).tileId());
	}

	auto placeBuilding = [&](CardEnum buildingEnum)
	{
		WorldTile2 curTile(PunSettings::Get("TestCityNetwork_CurTileId"));

		const WorldTile2 buildingSpacing(3, 3);
		TileArea startArea(curTile, GetBuildingInfo(buildingEnum).size + WorldTile2(2, 2) + buildingSpacing);

		TileArea endArea = SimUtils::SpiralFindAvailableBuildArea(startArea,
			[&](WorldTile2 tile) {
			return IsPlayerBuildable(tile, commandPlayerId);
		},
			[&](WorldTile2 tile) {
			return WorldTile2::ManDistance(curTile, tile) > 200;
		}
		);

		if (!endArea.isValid()) {
			return false;
		}

		PunSettings::Set("TestCityNetwork_CurTileId", curTile.tileId());

		{
			auto command = make_shared<FPlaceBuilding>();
			command->center = endArea.centerTile();
			command->buildingEnum = static_cast<uint8>(buildingEnum);
			command->faceDirection = static_cast<uint8>(Direction::S);
			command->area = BuildingArea(command->center, GetBuildingInfo(buildingEnum).size, static_cast<Direction>(command->faceDirection));
			_gameManager->SendNetworkCommand(command);

			PUN_LOG("TestCityNetworkStage FPlaceBuilding:%s", *GetBuildingInfo(buildingEnum).nameF());
		}

		return true;
	};

	auto quickBuild = [&](CardEnum buildingEnum)
	{
		const std::vector<int32>& bldIds = buildingIds(townId, buildingEnum);
		for (int32 bldId : bldIds)
		{
			if (!building(bldId).isConstructed() &&
				!_buildingSystem->IsQuickBuild(bldId))
			{
				auto command = make_shared<FGenericCommand>();
				command->callbackEnum = CallbackEnum::QuickBuild;
				command->intVar1 = bldId;
				_gameManager->SendNetworkCommand(command);

				PUN_LOG("TestCityNetworkStage QuickBuild:%s id:%d", *GetBuildingInfo(buildingEnum).nameF(), bldId);
			}
		}
	};

	// Build Storages
	{
		int32 numberOfBuildings = 5;
		if (townBuildingFinishedCount(townId, CardEnum::Warehouse) < numberOfBuildings)
		{
			if (buildingCount(townId, CardEnum::Warehouse) < numberOfBuildings) {
				if (placeBuilding(CardEnum::Warehouse)) {
					return;
				}
			}

			quickBuild(CardEnum::Warehouse);
		}
	}
	

	// Build Houses
	int32 numberOfHouses = 10;
	if (townBuildingFinishedCount(townId, CardEnum::House) < numberOfHouses)
	{
		if (buildingCount(townId, CardEnum::House) < numberOfHouses) {
			if (placeBuilding(CardEnum::House)) {
				return;
			}
		}
		
		quickBuild(CardEnum::House);
	}

	// Research
	if (!unlockSystem(commandPlayerId)->IsResearched(TechEnum::IndustrialAge))
	{
		auto command = make_shared<FCheat>();
		command->cheatEnum = CheatEnum::UnlockAll;
		_gameManager->SendNetworkCommand(command);

		PUN_LOG("TestCityNetworkStage UnlockAll");
		return;
	}
	

	std::vector<CardEnum> availableCards = cardSystem(gameManagerPlayerId()).GetAllPiles();
	unordered_set<CardEnum> uniqueAvailableCards(availableCards.begin(), availableCards.end());

	std::vector<CardEnum> buildingsToSpawn = SortedNameBuildingEnum;
	//std::vector<CardEnum> buildingsToSpawn = { CardEnum::ExhibitionHall };
	
	for (CardEnum buildingEnum : buildingsToSpawn)
	{
		if (static_cast<int32>(buildingEnum) >= PunSettings::Get("TestCityNetwork_BuildingEnumToStop")) {
			continue;
		}
		if (static_cast<int32>(buildingEnum) < PunSettings::Get("TestCityNetwork_BuildingEnumToStart")) {
			continue;
		}
		
		if (uniqueAvailableCards.find(buildingEnum) != uniqueAvailableCards.end() &&
			buildingCount(townId, buildingEnum) < 1 && 
			buildingEnum != CardEnum::Bridge &&
			buildingEnum != CardEnum::ClayPit &&
			buildingEnum != CardEnum::IrrigationReservoir &&
			!IsPortBuilding(buildingEnum) &&
			!IsMountainMine(buildingEnum))
		{
			if (placeBuilding(buildingEnum)) {
				return;
			}
		}

		quickBuild(buildingEnum);

		// check resources
		const std::vector<int32>& bldIds = buildingIds(townId, buildingEnum);
		for (int32 bldId : bldIds)
		{
			Building& bld = building(bldId);

			auto fillResource = [&](ResourceEnum resourceEnum) {
				if (resourceEnum != ResourceEnum::None && resourceCountTown(townId, resourceEnum) < bld.inputPerBatch(resourceEnum) * 2)
				{
					auto command = make_shared<FCheat>();
					command->cheatEnum = CheatEnum::AddResource;
					command->var1 = static_cast<int32>(resourceEnum);
					command->var2 = bld.inputPerBatch(resourceEnum) * 2;
					_gameManager->SendNetworkCommand(command);

					PUN_LOG("TestCityNetworkStage AddResource:%s id:%d", *GetBuildingInfo(buildingEnum).nameF(), bldId);
				}
			};

			fillResource(bld.input1());
			fillResource(bld.input2());
		}
	}
}


#undef LOCTEXT_NAMESPACE