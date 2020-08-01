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

DECLARE_CYCLE_STAT(TEXT("PUN: GameTree"), STAT_PunGameTreeTick, STATGROUP_Game);
DECLARE_CYCLE_STAT(TEXT("PUN: [Sim]Total"), STAT_PunSimTotal, STATGROUP_Game);

void GameSimulationCore::Init(IGameManagerInterface* gameManager, IGameSoundInterface* soundInterface, IGameUIInterface* uiInterface, bool isLoadingFromFile)
{
	PUN_LLM_ONLY(PunScopeLLM::InitPunLLM());

	PUN_LLM(PunSimLLMTag::Simulation);
	
	_CrtSetDbgFlag(_CRTDBG_ALLOC_MEM_DF | _CRTDBG_LEAK_CHECK_DF);
	
	_gameManager = gameManager;
	_soundInterface = soundInterface;
	_uiInterface = uiInterface;
	
	_tickCount = 0;
	//_playerCount = 0;
	
	GameRand::ResetStateToTickCount(_tickCount);
	Time::ResetTicks();

	_isLoadingFromFile = isLoadingFromFile;

	{
		PUN_LLM(PunSimLLMTag::PathAI);
		_pathAIHuman = make_unique<PunAStar128x256>();
		_pathAI = make_unique<PunAStar128x256>();
	}

	_mapSettings = gameManager->GetMapSettings();
	std::string mapSeed = ToStdString(_mapSettings.mapSeed);

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
			_terrainGenerator->CalculateWorldTerrain(mapSeed);
			_terrainGenerator->SetGameMap();
#else
			if (!_terrainGenerator->HasSavedMap(ToFString(mapSeed), _mapSettings.mapSizeEnum())) {
				INIT_LOG("CalculateWorldTerrain %s", *ToFString(mapSeed));
				_terrainGenerator->CalculateWorldTerrain(mapSeed);
				
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
	}

	_uiInterface->SetLoadingText("Planting Trees...");

	_gameEventSystem.Init();
	_overlaySystem.Init(this); // Needs to come first for road.

	_provinceSystem.InitProvince(this);
	//_terrainGenerator->SetProvinceMap(_provinceSystem.GetProvinceId2x2Vec());

	_unitSystem = make_unique<UnitSystem>();
	_unitSystem->Init(this);

	_buildingSystem = make_unique<BuildingSystem>();
	_buildingSystem->Init(this);

	_regionSystem = make_unique<GameRegionSystem>();
	_regionSystem->Init(this);

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
		_floodSystem.Init(_pathAI.get(), _terrainGenerator.get(), isLoadingFromFile);
		_floodSystemHuman.Init(_pathAIHuman.get(), _terrainGenerator.get(), isLoadingFromFile);
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
		_resourceSystems.push_back(ResourceSystem(playerId, this));
		_unlockSystems.push_back(UnlockSystem(playerId, this));
		_questSystems.push_back(QuestSystem(playerId, this));
		_playerOwnedManagers.push_back(PlayerOwnedManager(playerId, this));
		_playerParameters.push_back(PlayerParameters(playerId, this));
		_statSystem.AddPlayer(playerId);
		_popupSystems.push_back(PopupSystem(playerId, this));
		_cardSystem.push_back(BuildingCardSystem(playerId, this));

		_aiPlayerSystem.push_back(AIPlayerSystem(playerId, this, this));
		
		_eventLogSystem.AddPlayer();
		_replaySystem.AddPlayer();

		_playerIdToNonRepeatActionToAvailableTick.push_back(std::vector<int32>(static_cast<int>(NonRepeatActionEnum::Count), 0));
	}

	// Only set the "InitialAIs" active
	PUN_CHECK(_mapSettings.aiCount <= GameConstants::MaxAIs);
	for (int i = 0; i < _mapSettings.aiCount; i++) {
		_aiPlayerSystem[GameConstants::MaxPlayers + i].SetActive(true);
	}

	// Replay is fixed for 1 player for now
	int32 firstReplayPlayer = 1;
	TArray<FString> replayNames = gameManager->GetReplayFileNames();
	for (int32 i = 0; i < replayNames.Num(); i++) {
		_replaySystem.LoadPlayerActions(firstReplayPlayer, replayNames[i]);
	}


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

	if (!isLoadingFromFile) {
		InitRegionalBuildings();
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
}

void GameSimulationCore::InitRegionalBuildings()
{
	std::vector<CardEnum> regionalBuildings(GameMapConstants::TotalRegions, CardEnum::None);

	auto& provinceSys = provinceSystem();
	
	for (int y = 0; y < GameMapConstants::RegionsPerWorldY; y++) {
		for (int x = 0; x < GameMapConstants::RegionsPerWorldX; x++)
		{
			if (GameRand::Rand() % 3 != 0) {
				continue;
			}
			
			WorldRegion2 region(x, y);
			int32 provinceId = region.regionId();
			//WorldTile2 regionCenter = region.centerTile();

			WorldTile2 provinceCenter = provinceSys.GetProvinceCenterTile(provinceId);
			if (!provinceCenter.isValid()) {
				continue;
			}

			BiomeEnum biomeEnum = terrainGenerator().GetBiome(provinceCenter);

			auto hasNearbyBuilding = [&](CardEnum cardEnum) {
				bool hasNearby = false;
				// TODO: faster if check only 4 regions before loop reach this
				region.ExecuteOnNearbyRegions([&](WorldRegion2 curRegion) {
					if (curRegion.IsValid() && regionalBuildings[curRegion.regionId()] == cardEnum) {
						hasNearby = true;
					}
				});
				return hasNearby;
			};

			CardEnum buildingEnum = CardEnum::None;

			if (GameRand::Rand() % 10 == 0 && !hasNearbyBuilding(CardEnum::RegionShrine)) {
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
					FPlaceBuildingParameters parameters;
					parameters.buildingEnum = static_cast<uint8>(buildingEnum);
					parameters.faceDirection = static_cast<uint8>(faceDirection);
					parameters.area = area;
					parameters.center = centerTile;
					parameters.playerId = -1;

					int32 buildingId = PlaceBuilding(parameters);
					Building& bld = building(buildingId);
					bld.InstantClearArea();
					bld.FinishConstruction();

					// Clear the trees
					const int32 radiusToClearTrees = 7;
					TileArea(centerTile, radiusToClearTrees).ExecuteOnArea_WorldTile2([&](WorldTile2 tile)
					{
						if (WorldTile2::Distance(centerTile, tile) <= radiusToClearTrees) {
							_treeSystem->ForceRemoveTileObj(tile, false);
						}
					});

					regionalBuildings[region.regionId()] = buildingEnum;

					break;
				}
			}

		}
	}
}

/*
 * Tick
 */
void GameSimulationCore::Tick(int bufferCount, NetworkTickInfo& tickInfo)
{
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

	CheckIntegrity();

	// Issue Commands
	std::vector<shared_ptr<FNetworkCommand>> commands = tickInfo.commands;

	{
		// Replay PlayerAction on unused player...
		std::vector<ReplayPlayer>& replayPlayers = _replaySystem.replayPlayers;
		for (size_t i = 0; i < replayPlayers.size(); i++)
		{
			// Replay Actions
			if (replayPlayers[i].HasRecordedPlayerAction(_tickCount))
			{
				NetworkTickInfo replayTickInfo = replayPlayers[i].GetRecordedPlayerActionThenIncrement(_tickCount);
				auto& replayCommands = replayTickInfo.commands;
				PUN_CHECK(replayCommands.size() > 0);

				// Preprocess for commands that requires buildingId
				// BuildingId won't be valid in the next game, but buildingTileId will
				for (size_t j = replayCommands.size(); j-- > 0;)
				{
					switch (replayCommands[j]->commandType())
					{
					case NetworkCommandEnum::JobSlotChange:
					case NetworkCommandEnum::SetAllowResource:
					case NetworkCommandEnum::SetPriority:
					case NetworkCommandEnum::ChangeWorkMode: { 
						auto command = std::static_pointer_cast<FBuildingCommand>(replayCommands[j]);
						Building* bld = buildingAtTile(WorldTile2(command->buildingTileId));
						PUN_CHECK(command->buildingEnum == bld->buildingEnum());
						command->buildingId = bld->buildingId();
						break;
					}
					default:
						break;
					}
				}

				// Add Replay commands to other commands
				for (auto& replayCommand : replayCommands) {
					replayCommand->playerId = i;
					commands.push_back(replayCommand);
				}
			}


			// Trailer
			if (replayPlayers[i].isInitialize() && 
				replayPlayers[i].trailerCommands.size() > 0)
			{
				if (replayPlayers[i].nextTrailerCommandTick != -1 &&
					Time::Ticks() > replayPlayers[i].nextTrailerCommandTick)
				{
					replayPlayers[i].nextTrailerCommandTick = Time::Ticks() + Time::TicksPerSecond;

					auto& trailerCommands = replayPlayers[i].trailerCommands;
					std::shared_ptr<FNetworkCommand> command = trailerCommands.front();
					trailerCommands.erase(trailerCommands.begin());

					// Special case: Road
					// If this is road, build one at a time
					if (command->commandType() == NetworkCommandEnum::PlaceGather)
					{
						auto dragCommand = static_pointer_cast<FPlaceGatherParameters>(command);
						if (dragCommand->path.Num() > 0)
						{
							TArray<int32> newPath = dragCommand->path;
							int32 firstTileId = newPath[0];
							newPath.RemoveAt(0);
							 
							// Put the command back in front with trimmed
							if (newPath.Num() > 0) {
								auto commandToFrontPush = make_shared<FPlaceGatherParameters>(*dragCommand);
								commandToFrontPush->path = newPath;
								trailerCommands.insert(trailerCommands.begin(), commandToFrontPush);
							}

							dragCommand->path = { firstTileId }; // build 1 tile at a time

							replayPlayers[i].nextTrailerCommandTick = Time::Ticks() + Time::TicksPerSecond / 10; // Build 10 tiles a sec
						}
					}

					command->playerId = i;
					commands.push_back(command);

					PUN_LOG("Trailer Command Issued num:%llu pid:%d type:%d tick:%d", trailerCommands.size(), command->playerId, command->commandType(), Time::Ticks());
				}
			}
		}
		
	}


	for (size_t i = 0; i < commands.size(); i++) 
	{
		PUN_CHECK(IsValidPlayer(commands[i]->playerId));

		// Player not initialized, only allow choosing location
		// This check prevent the case where commands arrived after abandoning town (such as commands from leftover popups etc.)
		if (!playerChoseLocation(commands[i]->playerId))
		{
			if (commands[i]->commandType() != NetworkCommandEnum::ChooseLocation &&
				commands[i]->commandType() != NetworkCommandEnum::AddPlayer)
			{
				continue;
			}
		}
		
		switch (commands[i]->commandType())
		{
			//case NetworkCommandEnum::AddPlayer:			AddPlayer(*static_pointer_cast<FAddPlayer>(commands[i])); break;

			case NetworkCommandEnum::PlaceBuilding:		PlaceBuilding(*static_pointer_cast<FPlaceBuildingParameters>(commands[i])); break;
			case NetworkCommandEnum::PlaceGather:		PlaceDrag(*static_pointer_cast<FPlaceGatherParameters>(commands[i])); break;
			case NetworkCommandEnum::JobSlotChange:		JobSlotChange(*static_pointer_cast<FJobSlotChange>(commands[i])); break;
			case NetworkCommandEnum::SetAllowResource:	SetAllowResource(*static_pointer_cast<FSetAllowResource>(commands[i])); break;
			case NetworkCommandEnum::SetPriority:		SetPriority(*static_pointer_cast<FSetPriority>(commands[i])); break;
			case NetworkCommandEnum::SetTownPriority:	SetTownPriority(*static_pointer_cast<FSetTownPriority>(commands[i])); break;

			case NetworkCommandEnum::TradeResource:		TradeResource(*static_pointer_cast<FTradeResource>(commands[i])); break;
			case NetworkCommandEnum::SetIntercityTrade:	SetIntercityTrade(*static_pointer_cast<FSetIntercityTrade>(commands[i])); break;
			case NetworkCommandEnum::UpgradeBuilding:	UpgradeBuilding(*static_pointer_cast<FUpgradeBuilding>(commands[i])); break;
			case NetworkCommandEnum::ChangeWorkMode:	ChangeWorkMode(*static_pointer_cast<FChangeWorkMode>(commands[i])); break;
			case NetworkCommandEnum::ChooseLocation:	ChooseLocation(*static_pointer_cast<FChooseLocation>(commands[i])); break;
			case NetworkCommandEnum::Cheat:				Cheat(*static_pointer_cast<FCheat>(commands[i])); break;
			case NetworkCommandEnum::PopupDecision:		PopupDecision(*static_pointer_cast<FPopupDecision>(commands[i])); break;

			case NetworkCommandEnum::RerollCards:		RerollCards(*static_pointer_cast<FRerollCards>(commands[i])); break;

			case NetworkCommandEnum::SelectRareCard:	SelectRareCard(*static_pointer_cast<FSelectRareCard>(commands[i])); break;
			case NetworkCommandEnum::BuyCard:			BuyCards(*static_pointer_cast<FBuyCard>(commands[i])); break;
			case NetworkCommandEnum::SellCards:			SellCards(*static_pointer_cast<FSellCards>(commands[i])); break;
			case NetworkCommandEnum::UseCard:			UseCard(*static_pointer_cast<FUseCard>(commands[i])); break;
			case NetworkCommandEnum::UnslotCard:		UnslotCard(*static_pointer_cast<FUnslotCard>(commands[i])); break;

			case NetworkCommandEnum::Attack:			Attack(*static_pointer_cast<FAttack>(commands[i])); break;
			//case NetworkCommandEnum::TrainUnit:			TrainUnit(*static_pointer_cast<FTrainUnit>(commands[i])); break;

			case NetworkCommandEnum::ClaimLand:			ClaimLand(*static_pointer_cast<FClaimLand>(commands[i])); break;
			case NetworkCommandEnum::ChooseResearch:	ChooseResearch(*static_pointer_cast<FChooseResearch>(commands[i])); break;

			case NetworkCommandEnum::ChangeName:		ChangeName(*static_pointer_cast<FChangeName>(commands[i])); break;
			case NetworkCommandEnum::SendChat:			SendChat(*static_pointer_cast<FSendChat>(commands[i])); break;
			default: UE_DEBUG_BREAK();
		}
	}

	// Server tick is not the same as SimulationTick
	//   Therefore we need to set SimulationTick before saving it into replay.
	tickInfo.tickCountSim = _tickCount;


	/*
	 * Replays
	 */
	// Record tickInfo
	_replaySystem.AddNetworkTickInfo(tickInfo);

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
	if (!AllPlayerChoseLocationAfterInitialTicks()) {
		gameSpeed = 0; // Pause while players are still choosing location.
	}
	_lastGameSpeed = gameSpeed;

	// Special -12 game speed which means 1/2 discard a tick
	int32 simTicksForThisControllerTick = gameSpeed;
	if (gameSpeed == -12) {
		simTicksForThisControllerTick = tickInfo.tickCount % 2;
	}

	for (size_t i = 0; i < simTicksForThisControllerTick; i++)
	{
		//_LOG(PunTick, "TickCore %d", _tickCount);

		GameRand::ResetStateToTickCount(_tickCount);
		Time::SetTickCount(_tickCount);

		_floodSystem.Tick();
		_floodSystemHuman.Tick();

		//_floodSystem2.Tick();
		//_floodSystem2Human.Tick();

		if (PunSettings::IsOn("TickStats")) {
			_statSystem.Tick();
		}

		if (PunSettings::IsOn("TickPlayerOwned"))
		{
			// Per round
			if (Time::Ticks() % Time::TicksPerRound == 0) 
			{
				ExecuteOnPlayersAndAI([&] (int32_t playerId) {
					if (_playerOwnedManagers[playerId].isInitialized()) {
						_cardSystem[playerId].TickRound();
						_playerOwnedManagers[playerId].TickRound();

						_popupSystems[playerId].TickRound();
					}
				});
			}

			// Event ticks
			if (Time::IsAutumnStart() && Time::Years() > 0) 
			{
				// Randomize an event depending on the status of the town...

			}

			// Check for events every 20 sec
			if (_tickCount % (Time::TicksPerSecond * 20) == 0)
			{
				// Event log check
				ExecuteOnConnectedPlayers([&](int32 playerId)
				{
					if (_playerOwnedManagers[playerId].isInitialized())
					{
						if (isStorageAllFull(playerId)) {
							_eventLogSystem.AddEventLog(playerId, FString("Need more storage space."), true);

							_soundInterface->Spawn2DSound("UI", "NeedStorageBell", playerId);

							// Trigger storage quest if not yet done...
							TryAddQuest(playerId, std::make_shared<BuildStorageQuest>());
						}
					}
				});
				//for (size_t j = 0; j < _playerCount; j++) {
				//	if (_playerOwnedManagers[j].isInitialized())
				//	{
				//		if (isStorageAllFull(j)) {
				//			_eventLogSystem.AddEventLog(j, FString("Need more storage space."), true);

				//			_soundInterface->Spawn2DSound("UI", "NeedStorageBell", j);

				//			// Trigger storage quest if not yet done...
				//			TryAddQuest(j, std::make_shared<BuildStorageQuest>());
				//		}

				//		
				//	}
				//}
			}

			// Tick 1 sec
			if (_tickCount % Time::TicksPerSecond == 0) 
			{
				ExecuteOnPlayersAndAI([&](int32 playerId)
				{
					if (_playerOwnedManagers[playerId].isInitialized()) 
					{
						//_playerOwnedManagers[playerId].RefreshHousing();
						_playerOwnedManagers[playerId].Tick1Sec();

						// Trailer auto-build
						if (SimSettings::IsOn("TrailerMode")) {
							for (int32 enumInt = 0; enumInt < BuildingEnumCount; enumInt++) {
								std::vector<int32> bldIds =  buildingIds(playerId, static_cast<CardEnum>(enumInt));
								for (int32 bldId : bldIds) {
									building(bldId).TickConstruction(10);
								}
							}
						}

						/*
						 * ProvinceClaimProgress
						 */
						// Battle Resolve
						std::vector<ProvinceClaimProgress> claimProgresses = _playerOwnedManagers[playerId].defendingClaimProgress();
						for (const ProvinceClaimProgress& claimProgress : claimProgresses) {
							// One season to conquer in normal case
							if (claimProgress.ticksElapsed > BattleClaimTicks) {
								_playerOwnedManagers[playerId].EndConquer(claimProgress.provinceId);
								_playerOwnedManagers[claimProgress.attackerPlayerId].EndConquer_Attacker(claimProgress.provinceId);

								// Attacker now owns the province
								// Destroy any leftover building owned by player
								ClearProvinceBuildings(claimProgress.provinceId);
								SetProvinceOwner(claimProgress.provinceId, claimProgress.attackerPlayerId);

								
							}
							// Failed to conquer
							else if (claimProgress.ticksElapsed <= 0) {
								_playerOwnedManagers[playerId].EndConquer(claimProgress.provinceId);
								_playerOwnedManagers[claimProgress.attackerPlayerId].EndConquer_Attacker(claimProgress.provinceId);
							}
						}

						_cardSystem[playerId].TryRefreshRareHand();


						// Spawn rare hand after 1 sec... Delay so that the ChooseRareCard sound will appear in sync when menu after systemMoveCamera
						auto& townhal = townhall(playerId);
						if (!townhal.alreadyGotInitialCard && townhal.townAgeTicks() >= Time::TicksPerSecond) 
						{
							GenerateRareCardSelection(playerId, RareHandEnum::InitialCards1, "A starting card.");
							GenerateRareCardSelection(playerId, RareHandEnum::InitialCards2, "Another starting card.");
							
							townhal.alreadyGotInitialCard = true;
						}
						

						// Check for owner change, and update vassals accordingly...
						ArmyNode& armyNode = townhall(playerId).armyNode;
						int32 lastLordPlayerId = armyNode.lastLordPlayerId;
						int32 lordPlayerId = armyNode.lordPlayerId;
						if (lastLordPlayerId != lordPlayerId)
						{
							// If the last lord wasn't the original owner, that player lose vassalage
							if (lastLordPlayerId != armyNode.originalPlayerId) {
								_playerOwnedManagers[lastLordPlayerId].LoseVassal(armyNode.nodeId);
							}

							// Attacker made node owner's a vassal
							if (lordPlayerId != armyNode.originalPlayerId) {
								_playerOwnedManagers[lordPlayerId].GainVassal(armyNode.nodeId);
								AddPopupAll(PopupInfo(lordPlayerId, playerName(lordPlayerId) + " has conquered " + armyNodeName(armyNode.nodeId) + "."), -1);
								AddPopup(armyNode.originalPlayerId, 
									"<Bold>You became " + playerName(lordPlayerId) + "'s vassal.</>"
											 "<space>"
											 "<bullet>As a vassal, you pay your lord 5% <img id=\"Coin\"/> revenue as a tribute each round.</>"
											 "<bullet>If your lord is ahead of you in science, you gain +20% <img id=\"Science\"/> from knowledge transfer.</>");
								_playerOwnedManagers[lordPlayerId].RecalculateTaxDelayed();
								_playerOwnedManagers[armyNode.originalPlayerId].RecalculateTaxDelayed();

								CheckDominationVictory(lordPlayerId);
							}
							// lord title returns to its original owner, liberated!
							else
							{
								AddPopupAll(PopupInfo(lordPlayerId, playerName(armyNode.originalPlayerId) + " has declared independence from " + playerName(lastLordPlayerId) + "."), -1);
							}
							
							armyNode.lastLordPlayerId = lordPlayerId;
						}

						// Economic Victory
						const int32 thousandsToWarn1 = 60;
						const int32 thousandsToWarn2 = 70;
						const int32 thousandsToWin = 80;
						
						if (_playerOwnedManagers[playerId].economicVictoryPhase == 0 && money(playerId) > (thousandsToWarn1 * 1000)) {
							_playerOwnedManagers[playerId].economicVictoryPhase = 1;
							AddPopup(playerId, "You accumulated " + to_string(thousandsToWarn1) 
								+ ",000<img id=\"Coin\"/>!<space>You will achieve economic victory once you accumulate " + to_string(thousandsToWin)
								+ ",000<img id=\"Coin\"/>.");
							AddPopupAll(PopupInfo(-1, playerName(playerId) + " accumulated " + to_string(thousandsToWarn1)
								+ ",000<img id=\"Coin\"/>.<space>At " + to_string(thousandsToWin)
								+ ",000<img id=\"Coin\"/> "
								+ playerName(playerId) +" will achieve the economic victory."), playerId);
						}
						else if (_playerOwnedManagers[playerId].economicVictoryPhase == 1 && money(playerId) > (thousandsToWarn2 * 1000)) {
							_playerOwnedManagers[playerId].economicVictoryPhase = 2;
							AddPopup(playerId, "You accumulated " + to_string(thousandsToWarn2) 
								+ ",000<img id=\"Coin\"/>!<space>You will achieve economic victory once you accumulate " + to_string(thousandsToWin)
								+ ",000<img id=\"Coin\"/>.");
							AddPopupAll(PopupInfo(-1, playerName(playerId) + " accumulated " + to_string(thousandsToWarn2)
								+ ",000<img id=\"Coin\"/>.<space>At " + to_string(thousandsToWin)
								+ ",000<img id=\"Coin\"/> "
								+ playerName(playerId) + " will achieve the economic victory."), playerId);
						}
						else if (_playerOwnedManagers[playerId].economicVictoryPhase == 2 && money(playerId) > (thousandsToWin * 1000)) 	{
							_endStatus.victoriousPlayerId = playerId;
							_endStatus.gameEndEnum = GameEndEnum::EconomicVictory;
						}
					}
				});
				
				for (int j = GameConstants::MaxPlayers; j < GameConstants::MaxPlayersAndAI; j++) {
					_aiPlayerSystem[j].Tick1Sec();
				}

				_eventLogSystem.Tick1Sec();
				_worldTradeSystem.Tick1Sec();
			}

			// Tick quarter sec
			if (_tickCount % (Time::TicksPerSecond / 4) == 0)
			{
				/*
				 * Science
				 */
				ExecuteOnPlayersAndAI([&](int32 playerId) {
					if (_playerOwnedManagers[playerId].isInitialized()) {
						unlockSystem(playerId)->Research(_playerOwnedManagers[playerId].science100PerRound(), 4);
					}
				});
			}

			ExecuteOnPlayersAndAI([&](int32 playerId) {
				if (_playerOwnedManagers[playerId].isInitialized()) {
					_playerOwnedManagers[playerId].Tick();

					questSystem(playerId)->Tick();

					townhall(playerId).armyNode.Tick();
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
			SCOPE_CYCLE_COUNTER(STAT_PunGameTreeTick);
			_treeSystem->Tick();
		}

		if (PunSettings::IsOn("TickUnlocks"))
		{

		}

		// Hashes
		{
			int32 unitHash = _unitSystem->GetDebugHash();
			int32 buildingHash = _buildingSystem->GetDebugHash();
			_tickHashes.push_back(unitHash + buildingHash);
		}

		// Snow
		{
			auto snowAccumulate = [&](FloatDet& lastSnow, FloatDet& snow, int32 temperatureFraction100)
			{
				lastSnow = snow;

				FloatDet minCelsius = ModifyCelsiusByBiome(temperatureFraction100, Time::MinCelsiusBase());
				FloatDet maxCelsius = ModifyCelsiusByBiome(temperatureFraction100, Time::MaxCelsiusBase(), maxCelsiusDivider);

				FloatDet currentCelsius = Time::CelsiusHelper(minCelsius, maxCelsius);

				const FloatDet snowAccumulationStarts = -FD0_X(21);// FD0_X(59); // Starts at 5.9 C
				snow += (-currentCelsius + snowAccumulationStarts) / Time::TicksPerSeason;
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
				Serialize(SaveArchive, SaveArchive, crcs, crcLabels);
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

int GameSimulationCore::AddUnit(UnitEnum unitEnum, int32_t playerId, WorldAtom2 location, int32_t ageTicks)
{
	int objectId = _unitSystem->AddUnit(unitEnum, playerId, location, ageTicks);

	//PlayerOwned
	if (unitEnum == UnitEnum::Human && playerId != GameInfo::PlayerIdNone) {
		_playerOwnedManagers[playerId].PlayerAddHuman(objectId);
	}
	return objectId;
}
void GameSimulationCore::RemoveUnit(int id)
{
	// PlayerOwned
	int32 playerId = _unitSystem->unitStateAI(id).playerId();
	if (_unitSystem->unitEnum(id) == UnitEnum::Human && playerId != GameInfo::PlayerIdNone) {
		PUN_DEBUG_EXPR(unitAI(id).AddDebugSpeech("RemoveUnit Player"));
		
		unitAI(id).SetWorkplaceId(-1);
		unitAI(id).SetHouseId(-1);
		_playerOwnedManagers[playerId].PlayerRemoveHuman(id);
	}

	_unitSystem->RemoveUnit(id);

	TryRemoveDescriptionUI(ObjectTypeEnum::Unit, id);
}

/**
 * Player interactions
 */

int32 GameSimulationCore::PlaceBuilding(FPlaceBuildingParameters parameters)
{	
	TileArea area = parameters.area;
	Direction faceDirection = static_cast<Direction>(parameters.faceDirection);
	TileArea frontArea = area.GetFrontArea(faceDirection);
	CardEnum cardEnum = static_cast<CardEnum>(parameters.buildingEnum);
	int32 playerId = parameters.playerId;

	// TODO: use this for Ranch
	// Storage yard uses front with only 2 tiles
	//if (cardEnum == CardEnum::StorageYard) {
	//	frontArea = parameters.area2.GetFrontArea(faceDirection);
	//}
	

	if (cardEnum != CardEnum::BoarBurrow &&
		parameters.playerId != -1) 
	{
		_LOG(LogNetworkInput, "[pid:%d] PlaceBuilding %s %s", playerId, *ToFString(BuildingInfo[parameters.buildingEnum].name), *ToFString(parameters.area.ToString()));
	}

	// Don't allow building without bought card...
	if (!IsReplayPlayer(parameters.playerId) &&
		parameters.useBoughtCard && 
		!_cardSystem[playerId].CanUseBoughtCard(cardEnum))
	{
		return -1;
	}

	// Used converter card, decrease the converter card we have
	if (parameters.useWildCard != CardEnum::None)
	{
		if (!_cardSystem[playerId].HasBoughtCard(parameters.useWildCard)) {
			return -1;
		}

		int32 converterPrice = cardSystem(playerId).GetCardPrice(cardEnum);
		if (money(playerId) < converterPrice) {
			AddPopupToFront(playerId, "Need " + to_string(converterPrice) + "<img id=\"Coin\"/> to convert wild card to this building.", ExclusiveUIEnum::ConverterCardHand, "PopupCannot");
			return -1;
		}

		_cardSystem[playerId].RemoveCards(parameters.useWildCard, 1);
		_cardSystem[playerId].converterCardState = ConverterCardUseState::None;
		ChangeMoney(playerId, -converterPrice);
	}

	// Hand over Foreign building to another player
	int32 foreignBuildingOwner = parameters.playerId;
	if (cardEnum == CardEnum::HumanitarianAidCamp) {
		parameters.playerId = tileOwner(parameters.center);
	}

	/*
	 * Special cases
	 */
	if (IsAreaSpell(cardEnum))
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
		else if (cardEnum == CardEnum::Steal ||
				cardEnum == CardEnum::Snatch ||
				cardEnum == CardEnum::Kidnap ||
				cardEnum == CardEnum::SharingIsCaring)
		{
			int32 buildingId = buildingIdAtTile(parameters.center);
			if (buildingId != -1 && building(buildingId).isEnum(CardEnum::Townhall)) {
				int32 targetPlayerId = building(buildingId).playerId();

				if (cardEnum == CardEnum::Steal)
				{
					if (population(targetPlayerId) < population(playerId)) {
						AddPopup(playerId, "Cannot steal from a weaker town.");
						return -1;
					}
					int32 targetPlayerMoney = resourceSystem(targetPlayerId).money();
					int32 actualSteal = targetPlayerMoney * 20 / 100;
					resourceSystem(targetPlayerId).ChangeMoney(-actualSteal);
					resourceSystem(playerId).ChangeMoney(actualSteal);
					AddPopup(targetPlayerId, playerName(playerId) + " stole " + to_string(actualSteal) + "<img id=\"Coin\"/> from you");
					AddEventLog(playerId, "You stole " + to_string(actualSteal) + "</><img id=\"Coin\"/><Chat> from " + townName(targetPlayerId), false);
				}
				else if (cardEnum == CardEnum::Snatch)
				{
					if (population(targetPlayerId) < population(playerId)) {
						AddPopup(playerId, "Cannot steal from a weaker town.");
						return -1;
					}
					int32 targetPlayerMoney = resourceSystem(targetPlayerId).money();
					int32 actualSteal = min(targetPlayerMoney, 30);
					resourceSystem(targetPlayerId).ChangeMoney(-actualSteal);
					resourceSystem(playerId).ChangeMoney(actualSteal);
					AddPopup(targetPlayerId, townName(playerId) + " snatched " + to_string(actualSteal) + "<img id=\"Coin\"/> from you");
					AddEventLog(playerId, "You snatched " + to_string(actualSteal) + "</><img id=\"Coin\"/><Chat> from " + townName(targetPlayerId), false);
				}
				else if (cardEnum == CardEnum::Kidnap)
				{
					int32 kidnapMoney = 5 * population(playerId);
					if (money(playerId) < kidnapMoney) {
						AddPopup(playerId, "Need (5 x Population)<img id=\"Coin\"/> to kidnap (" + to_string(kidnapMoney) + "<img id=\"Coin\"/>).");
						return -1;
					}

					int32 targetPopulation = population(targetPlayerId);
					if (targetPopulation < 20) {
						AddPopup(playerId, "Target town's population is too low for kidnap.");
						return -1;
					}
					
					int32 unitsStoleCount = 0;
					std::vector<int32> humanIds = playerOwned(targetPlayerId).adultIds();
					std::vector<int32> childIds = playerOwned(targetPlayerId).childIds();
					humanIds.insert(humanIds.end(), childIds.begin(), childIds.end());
					for (int32 i = 0; i < 3; i++) {
						UnitStateAI& unitAI = unitSystem().unitStateAI(humanIds[i]);
						unitAI.Die();
						unitsStoleCount++;
					}
					ChangeMoney(playerId, -kidnapMoney);

					AddPopup(targetPlayerId, townName(playerId) + " kidnapped " + to_string(unitsStoleCount) + " people from you");

					stringstream ss;
					ss << "You kidnapped " << unitsStoleCount << " people from " << townName(targetPlayerId);
					ss << " using " << kidnapMoney << "</><img id=\"Coin\"/><Chat>.";
					AddEventLog(playerId, ss.str(), false);
					
					townhall(playerId).AddImmigrants(unitsStoleCount);
				}
				else if (cardEnum == CardEnum::SharingIsCaring)
				{
					AddPopup(targetPlayerId, townName(playerId) + " gave you 50 wheat.");
					AddEventLog(playerId, "You gave " + townName(targetPlayerId) + " 50 wheat", false);
					resourceSystem(targetPlayerId).AddResourceGlobal(ResourceEnum::Wheat, 50, *this);
				}
				else
				{
					UE_DEBUG_BREAK();
				}
			}
		}
		else if (cardEnum == CardEnum::InstantBuild)
		{
			int32 buildingId = buildingIdAtTile(parameters.center);
			Building& bld = building(buildingId);
			if (buildingId != -1 && !bld.isConstructed()) {
				int32_t cost = bld.buildingInfo().constructionCostAsMoney() * 3;
				resourceSystem(playerId).ChangeMoney(-cost);
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
			parameters.useBoughtCard) 
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
				AddEventLog(playerId, "You boosted " + GetBuildingInfo(bld.buildingEnum()).name + "'s efficiency.", false);
			}
		}
		
		return -1;
	}

	if (cardEnum == CardEnum::Bridge)
	{
		bool canPlace = true;
		area.ExecuteOnArea_WorldTile2([&](WorldTile2 tile) {
			if (!IsWater(tile)) {
				canPlace = false;
			}
		});

		if (canPlace) {
			int32 buildingId = _buildingSystem->AddBuilding(parameters);
			return buildingId;
		}

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
					if (!IsBuildable(tile, playerId)) {
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
	else if (cardEnum == CardEnum::Fisher ||
			cardEnum == CardEnum::TradingPort)
	{
		auto extraInfoPair = DockPlacementExtraInfo(cardEnum);
		int32 indexLandEnd = extraInfoPair.first;
		int32 minWaterCount = extraInfoPair.second;
		
		int32 waterCount = 0;
		
		area.ExecuteOnArea_WorldTile2([&](WorldTile2 tile) {
			if (GameMap::IsInGrid(tile.x, tile.y) && IsTileBuildableForPlayer(tile, playerId))
			{
				int steps = GameMap::GetFacingStep(faceDirection, area, tile);
				if (steps <= indexLandEnd) { // 0,1
					if (!IsBuildable(tile, playerId)) {
						canPlace = false; // Entrance not buildable
					}
				}
				else {
					if (IsWater(tile)) {
						waterCount++;
					}
				}
			}
		});
		if (waterCount < minWaterCount) {
			canPlace = false; // Not enough mountain tiles
		}
		
	}
	// Clay pit
	else if (cardEnum == CardEnum::ClayPit)
	{
		area.ExecuteOnArea_WorldTile2([&](WorldTile2 tile) {
			if (!(IsBuildable(tile) && _terrainGenerator->riverFraction(tile) > 0.5f)) {
				canPlace = false;
			}
		});
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
	else if (cardEnum == CardEnum::Townhall) {
		area.ExecuteOnArea_WorldTile2([&](WorldTile2 tile) {
			if (!IsBuildable(tile)) {
				canPlace = false;
			}
		});
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
	// All other buildings
	else {
		area.ExecuteOnArea_WorldTile2([&](WorldTile2 tile) {
			if (!IsBuildable(tile, playerId)) {
				canPlace = false;
			}
		});
	}

	/*
	 * Front Grid
	 */
	if (HasBuildingFront(cardEnum)) 
	{
		frontArea.ExecuteOnArea_WorldTile2([&](WorldTile2 tile) {
			if (!IsFrontBuildable(tile)) {
				canPlace = false;
			}
		});
	}

	if (canPlace)
	{
		// TileObjs
		_treeSystem->ForceRemoveTileReservationArea(area);
		if (HasBuildingFront(cardEnum)) {
			_treeSystem->ForceRemoveTileReservationArea(frontArea);
		}
		
		//// Delete all drops
		//area.ExecuteOnArea_WorldTile2([&](WorldTile2 tile) {
		//	_dropSystem.ForceRemoveDrop(tile);
		//});


		if (playerId != -1) {
			//DrawArea(parameters.area, FLinearColor::Blue, 10);
			//PUN_LOG("parameters.area before %s", *ToFString(parameters.area.ToString()));
		}

		// Place Building ***
		int32 buildingId = _buildingSystem->AddBuilding(parameters);


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
			resourceSystem(playerId).ChangeMoney(-_cardSystem[playerId].GetCardPrice(cardEnum));
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
				IsRegionalBuilding(cardEnum)) 
			{}
			else {
				// Finish road construction right away for provincial buildings
				if (cardEnum == CardEnum::Fort ||
					cardEnum == CardEnum::Colony) 
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
				_worldTradeSystem.RefreshTradeClusters();
			}
		}

		return buildingId;
	}
	
	return -1;
}

void GameSimulationCore::PlaceDrag(FPlaceGatherParameters parameters)
{
	_LOG(LogNetworkInput, " PlaceDrag pid:%d type:%d", parameters.playerId, parameters.placementType);
	
	TileArea area = parameters.area;
	auto placementType = static_cast<PlacementType>(parameters.placementType);
	//PUN_LOG("sim PlaceDrag: %d, %d, %d, %d", area.minX, area.minY, area.maxX, area.maxY);

	if (placementType == PlacementType::Gather) {
		int32 markCount = _treeSystem->MarkArea(parameters.playerId, area, false, parameters.harvestResourceEnum);

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
		_soundInterface->Spawn2DSound("UI", "PlaceBuilding", playerId());
		
		PUN_LOG("DragPlacement Demolish!!");
		area.EnforceWorldLimit();
		area.ExecuteOnArea_WorldTile2([&](WorldTile2 tile) {
			// Delete buildings
			int32 buildingId = buildingIdAtTile(tile);
			if (buildingId != -1) 
			{
				Building& bld = building(buildingId);

				// Player's building
				if (bld.playerId() == parameters.playerId)
				{
					// Don't demolish a burning building..
					if (bld.isOnFire()) {
						// TODO: Just do normal front popup remove
						PopupInfo* popupInfo = PopupToDisplay(parameters.playerId);
						if (popupInfo == nullptr || PopupToDisplay(parameters.playerId)->body != "Cannot demolish a building on fire.") {
							AddPopup(parameters.playerId, "Cannot demolish a building on fire.");
						}
						return;
					}


					/*
					 *  If there was a card in the building, put it in the hand
					 */
					auto& cardSys = cardSystem(parameters.playerId);
					if (bld.isEnum(CardEnum::Townhall))
					{
						if (!SimSettings::IsOn("CheatFastBuild")) {
							AddPopupToFront(parameters.playerId, "Cannot demolish the Townhall.");
							return;
						}
						
						// Try adding all townhall's card to hand
						std::vector<CardStatus> slotCards = cardSys.cardsInTownhall();
						for (size_t i = 0; i < slotCards.size(); i++)
						{
							if (cardSys.CanAddCardToBoughtHand(slotCards[i].cardEnum, 1)) {
								cardSys.RemoveCardFromTownhall(i);
								cardSys.AddCardToHand2(slotCards[i].cardEnum);
							}
							else {
								AddPopupToFront(parameters.playerId, "Card hand is full. Demolition failed.", ExclusiveUIEnum::None, "PopupCannot");
								return;
							}
						}
					}
					else
					{
						std::vector<CardStatus> slotCards = bld.slotCards();
						for (CardStatus card : slotCards)
						{
							if (cardSys.CanAddCardToBoughtHand(card.cardEnum, 1)) {
								bld.RemoveSlotCard();
								cardSys.AddCardToHand2(card.cardEnum);
							}
							else {
								AddPopupToFront(parameters.playerId, "Card hand is full. Demolition failed.", ExclusiveUIEnum::None, "PopupCannot");
								return;
							}
						}

						bld.ClearSlotCard();
					}

					/*
					 * if this isn't a permanent building or Fort/Colony, return the card
					 */
					// If this building was just built less than 15 sec ago, return the card...
					//if (bld.buildingAge() < Time::TicksPerSecond * 15)
					CardEnum buildingEnum = bld.buildingEnum();
					if (!IsPermanentBuilding(parameters.playerId, buildingEnum) &&
						buildingEnum != CardEnum::Fort && 
						buildingEnum != CardEnum::Colony)
					{
						if (cardSys.CanAddCardToBoughtHand(buildingEnum, 1)) {
							cardSys.AddCardToHand2(buildingEnum);
						} else {
							AddPopupToFront(parameters.playerId, "Card hand is full. Demolition failed.", ExclusiveUIEnum::None, "PopupCannot");
							return;
						}
					}

					/*
					 * Spawn resource drops
					 */
					if (!SimSettings::IsOn("DemolishNoDrop"))
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
					 * Keep storage demolition stat for quest
					 */
					playerStatSystem(bld.playerId()).AddStat(AccumulatedStatEnum::StoragesDestroyed);

					// Stop any sound
					soundInterface()->TryStopBuildingWorkSound(bld);
					
					_buildingSystem->RemoveBuilding(buildingId);

					_regionToDemolishDisplayInfos[bld.centerTile().regionId()].push_back({ bld.buildingEnum(), bld.area(), Time::Ticks() });
				}
				// Regional buildings (Shrines etc.)
				//  Allow demolition for shrine in player's region.
				else if (IsRegionalBuilding(bld.buildingEnum()) &&
						tileOwner(bld.centerTile()) == parameters.playerId)
				{
					_buildingSystem->RemoveBuilding(buildingId);

					_regionToDemolishDisplayInfos[bld.centerTile().regionId()].push_back({ bld.buildingEnum(), bld.area(), Time::Ticks() });
				}
			}

			// Delete road (non-construction)
			if (_overlaySystem.IsRoad(tile)) {
				_overlaySystem.RemoveRoad(tile);
				//GameMap::RemoveFrontRoadTile(area.min());
				PUN_CHECK(IsFrontBuildable(tile));
				
				_regionToDemolishDisplayInfos[tile.regionId()].push_back({CardEnum::DirtRoad, TileArea(tile, WorldTile2(1, 1)), Time::Ticks() });
				_regionToDemolishDisplayInfos[tile.regionId()].push_back({ CardEnum::StoneRoad, TileArea(tile, WorldTile2(1, 1)), Time::Ticks() });
			}

			// Critter building demolition
			DemolishCritterBuildingsIncludingFronts(tile, parameters.playerId);
		});

		_worldTradeSystem.RefreshTradeClusters();
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
			canBuild = [&](WorldTile2 tile) { return IsBuildable(tile, parameters.playerId); };
			break;
		}
		default:
			UE_DEBUG_BREAK();
		}


		auto tryBuild = [&](WorldTile2 tile) {
			DemolishCritterBuildingsIncludingFronts(tile, parameters.playerId);
			if (canBuild(tile)) {
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
				for (int32 i = 0; i < path.Num(); i++) {
					WorldTile2 tile(path[i]);
					if (IsFrontBuildable(tile) && !_overlaySystem.IsRoad(tile)) {
						_treeSystem->ForceRemoveTileObj(tile, false);
						overlaySystem().AddRoad(tile, true, true);

						resourceSystem(parameters.playerId).ChangeMoney(-IntercityRoadTileCost);

						// For road, also refresh the grass since we want it to be more visible
						SetNeedDisplayUpdate(DisplayClusterEnum::Trees, tile.regionId(), true);
					}
				}
				return;
			}

			// Trailer instant road
			if (SimSettings::IsOn("TrailerMode"))
			{
				for (int32 i = 0; i < path.Num(); i++) {
					WorldTile2 tile(path[i]);
					if (IsFrontBuildable(tile) && !_overlaySystem.IsRoad(tile)) {
						TileArea clearArea(tile, 1);
						_treeSystem->ForceRemoveTileObjArea(clearArea); // Remove 3x3
						overlaySystem().AddRoad(tile, true, true);

						// For road, also refresh the grass since we want it to be more visible
						SetNeedDisplayUpdate(DisplayClusterEnum::Trees, tile.regionId(), true);
					}
				}
				return;
			}

			// Dirt/Stone Road
			for (int32 i = 0; i < path.Num(); i++) {
				WorldTile2 tile(path[i]);
				//_treeSystem->ForceRemoveTileObj(tile, false);
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
	Building& bld = building(command.buildingId);
	if (IsHouse(bld.buildingEnum())) {
		playerOwned(command.playerId).SetHouseResourceAllow(command.resourceEnum, command.allowed);
	}
	else if (bld.isEnum(CardEnum::StorageYard) ||
			bld.isEnum(CardEnum::Warehouse)) 
	{
		bld.SetHolderTypeAndTarget(command.resourceEnum, command.allowed ? ResourceHolderType::Storage : ResourceHolderType::Provider, 0);
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

	auto& playerOwn = playerOwned(command.playerId);
	playerOwn.targetLaborerHighPriority = command.laborerPriority;
	playerOwn.targetBuilderHighPriority = command.builderPriority;
	playerOwn.targetRoadMakerHighPriority = command.roadMakerPriority;
	
	playerOwn.targetLaborerCount = command.targetLaborerCount;
	playerOwn.targetBuilderCount = command.targetBuilderCount;
	playerOwn.targetRoadMakerCount = command.targetRoadMakerCount;

	playerOwn.RefreshJobDelayed();
}

void GameSimulationCore::ChangeName(FChangeName command)
{
	UE_LOG(LogNetworkInput, Log, TEXT(" ChangeName[%d] %s"), command.objectId, *command.name);

	TownHall& townhall = building(command.objectId).subclass<TownHall>(CardEnum::Townhall);
	townhall.SetTownName(command.name);
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
		int32 tradingFeePercent = command.isIntercityTrade ? 0 : bld.tradingFeePercent();
		TradeBuilding::ExecuteTrade(command, tradingFeePercent, bld.centerTile(), this);
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
	if (!IsPlayerInitialized(command.playerId)) { // For Direct Command on AIs
		return;
	}
	
	if (command.buildingIdToEstablishTradeRoute != -1) {
		if (command.isCancelingTradeRoute) {
			worldTradeSystem().TryCancelTradeRoute(command);
		} else {
			worldTradeSystem().TryEstablishTradeRoute(command);
		}
	}
	else {
		worldTradeSystem().SetIntercityTradeOffers(command);
	}
}

void GameSimulationCore::UpgradeBuilding(FUpgradeBuilding command)
{
	// Special case: Replay
	if (IsReplayPlayer(command.playerId)) 
	{
		// Check all the buildings and upgrade as necessary
		for (int32 i = 0; i < BuildingEnumCount; i++) {
			const auto& bldIds = buildingIds(command.playerId, static_cast<CardEnum>(i));
			for (int32 bldId : bldIds) {
				Building& bld = building(bldId);
				std::vector<BuildingUpgrade> upgrades = bld.upgrades();
				for (size_t j = 0; j < upgrades.size(); j++) {
					if (!upgrades[j].isUpgraded) {
						bld.UpgradeBuilding(j);
					}
				}
			}
		}
		return;
	}
	
	PUN_ENSURE(IsValidBuilding(command.buildingId), return);
	
	Building& bld = building(command.buildingId);

	_LOG(LogNetworkInput, " UpgradeBuilding: pid:%d id:%d bldType:%s upgradeType:%d", command.playerId, command.buildingId, ToTChar(bld.buildingInfo().name), command.upgradeType);

	// Townhall special case
	if (bld.isEnum(CardEnum::Townhall))
	{
		auto& townhall = building(command.buildingId).subclass<TownHall>();
		if (command.upgradeType == 0) {
			if (townhall.townhallLvl < townhall.GetMaxUpgradeLvl() &&
				townhall.HasEnoughUpgradeMoney()) 
			{
				townhall.UpgradeTownhall();
			}
		} else {
			if (townhall.wallLvl < townhall.townhallLvl &&
				townhall.HasEnoughStoneToUpgradeWall()) 
			{
				townhall.UpgradeWall();
				if (townhall.armyNode.defendGroups.size() > 0) 	{
					townhall.armyNode.defendGroups[0].UpgradeWallLvl(townhall.wallLvl);
				}
			}
		}
	}
	// Other buildings
	else
	{
		bld.UpgradeBuilding(command.upgradeType);
	}
}

void GameSimulationCore::ChangeWorkMode(FChangeWorkMode command)
{
	PUN_ENSURE(IsValidBuilding(command.buildingId), return);
	
	Building& bld = building(command.buildingId);

	_LOG(LogNetworkInput, " ChangeWorkMode %d %s enumInt:%d", command.buildingId, ToTChar(bld.buildingInfo().name), command.enumInt);

	if (bld.isEnum(CardEnum::Farm))
	{
		Farm& farm = bld.subclass<Farm>();

		//PUN_LOG("ChangeWorkMode id:%d, from: %s, to: %s", command.buildingId, 
		//	*FString(PlantInfos[(int)farm->currentPlantEnum].name.c_str()),
		//	*FString(PlantInfos[command.enumInt].name.c_str()));
		PUN_LOG("ChangeWorkMode id:%d, from: %s, to: %s", command.buildingId,
			*FString(TreeInfos[(int)farm.currentPlantEnum].name.c_str()),
			*FString(TreeInfos[command.enumInt].name.c_str()));

		farm.currentPlantEnum = static_cast<TileObjEnum>(command.enumInt);
	}
	//else if (bld.isEnum(CardEnum::RanchBarn))
	//{
	//	RanchBarn& barn = bld.subclass<RanchBarn>();
	//	PUN_LOG("ChangeBarn id:%d, from: %s, to: %s", command.buildingId,
	//		*ToFString(GetUnitInfo(barn.animalEnum()).name),
	//		*ToFString(GetUnitInfo(command.enumInt).name));

	//	barn.SetAnimalEnum(static_cast<UnitEnum>(command.enumInt));
	//}
	else if (bld.hasWorkModes()) // BuildingHasDropdown(bld.buildingEnum()))
	{
		PUN_LOG("ChangeBld id:%d, from: %s, to: %s", command.buildingId,
			*ToFString(bld.workMode().name),
			*ToFString(bld.workModes[command.enumInt].name));

		bld.ChangeWorkMode(bld.workModes[command.enumInt]);
	}
	else if (bld.isEnum(CardEnum::Townhall))
	{
		auto& playerOwn = playerOwned(bld.playerId());
		
		// Is owner ... show normal tax
		if (bld.playerId() == command.playerId) {
			playerOwn.taxLevel = command.enumInt;
			RecalculateTaxDelayed(command.playerId);
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
	else {
		UE_DEBUG_BREAK();
	}
}

void GameSimulationCore::PopupDecision(FPopupDecision command) 
{
	UE_LOG(LogNetworkInput, Log, TEXT(" PopupDecision replyReceiver:%d choice:%d"), command.replyReceiverIndex, command.choiceIndex);
	
	TownHall& town = townhall(command.playerId);

	CloseCurrentPopup(command.playerId);

	PopupReceiverEnum replyReceiver = static_cast<PopupReceiverEnum>(command.replyReceiverIndex);
	
	if (replyReceiver == PopupReceiverEnum::ImmigrationEvent) 
	{
		if (command.choiceIndex == 0) {
			AddPopupToFront(command.playerId, "You can't help but notice the wide smile that spread across an immigrant child as she enters her promised land.");
			town.AddRequestedImmigrants();
		} else if(command.choiceIndex == 1) {
			//if (Time::Years() % 2 == 0) {
				int32 sneakedIns = town.migrationPull() / 2;
				AddPopupToFront(command.playerId, to_string(sneakedIns) + " immigrants decided to illegally sneaked in anyway...");
				town.AddImmigrants(sneakedIns);
			//} else {
			//	AddPopupToFront(command.playerId, "Their earlier joyful smiles of hope turned into gloom as the immigrants leave the town.");
			//}
		} else {
			AddPopupToFront(command.playerId, "News of your horrifying atrocity spreads across the world. You stole 100 gold.");
			resourceSystem(command.playerId).ChangeMoney(100);
		}
	}

	else if (replyReceiver == PopupReceiverEnum::ImmigrationBetweenPlayersEvent)
	{
		if (command.choiceIndex == 0) {
			AddPopupToFront(command.playerId, "You can't help but notice the wide smile that spread across an immigrant child as she enters her promised land.");
			town.AddRequestedImmigrants();
		}
		else if (command.choiceIndex == 1) {
			AddPopupToFront(command.playerId, "Their earlier joyful smiles of hope turned into gloom as the immigrants left your town.");
		}
		else {
			AddPopupToFront(command.playerId, "News of your horrifying atrocity spreads across the world. You stole 100 gold.");
			resourceSystem(command.playerId).ChangeMoney(100);
		}
	}
	
	else if (replyReceiver == PopupReceiverEnum::TribalJoinEvent) 
	{
		if (command.choiceIndex == 0) {
			AddPopupToFront(command.playerId, "Tribe people are now a part of our big family.");
			town.AddRequestedImmigrants();
			GenerateRareCardSelection(command.playerId, RareHandEnum::BuildingSlotCards, "A gift from the tribe.");
		}
		else if (command.choiceIndex == 1) {
			AddPopupToFront(command.playerId, "Forced out of their home, the tribe left our land.");
		}
		else {
			AddPopupToFront(command.playerId, "News of your horrifying atrocity spreads across the world. You stole 100 gold.");
			resourceSystem(command.playerId).ChangeMoney(100);
		}
	}
	
	else if (replyReceiver == PopupReceiverEnum::CaravanBuyer)
	{
		if (command.choiceIndex == 0) {
			if (command.playerId == playerId()) {
				uiInterface()->OpenTradeUI(town.buildingId());
			}
		}
	}

	else if (replyReceiver == PopupReceiverEnum::DoneResearchBuyCardEvent)
	{
		std::vector<CardEnum>& lastUnlockedBuildings = unlockSystem(command.playerId)->lastUnlockedBuildings;
		
		if (command.choiceIndex == 0) {
			CardEnum lastUnlockedEnum = lastUnlockedBuildings.front();
			
			PUN_CHECK(lastUnlockedEnum != CardEnum::None);

			if (CanBuyCard(command.playerId, lastUnlockedEnum))
			{
				auto& cardSys = cardSystem(command.playerId);
				cardSys.AddCardToHand2(lastUnlockedEnum);
				resourceSystem(command.playerId).ChangeMoney(-cardSys.GetCardPrice(lastUnlockedEnum));
			}
		}

		// Remove the unlockedBuilding regardless or choiceIndex
		lastUnlockedBuildings.erase(lastUnlockedBuildings.begin());
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
		if (command.choiceIndex == 0)
		{
			int32 playerId = command.playerId;
			vector<int32> provinceIds = playerOwned(playerId).provincesClaimed();
			const SubregionLists<int32>& buildingList = buildingSystem().buildingSubregionList();

			// Kill all humans
			std::vector<int32> citizenIds = playerOwned(playerId).adultIds();
			std::vector<int32> childIds = playerOwned(playerId).childIds();
			citizenIds.insert(citizenIds.end(), childIds.begin(), childIds.end());
			for (int32 citizenId : citizenIds) {
				unitSystem().unitStateAI(citizenId).Die();
			}

			// End all Alliance...
			std::vector<int32> allyPlayerIds = _playerOwnedManagers[playerId].allyPlayerIds();
			for (int32 allyPlayerId : allyPlayerIds) {
				LoseAlly(playerId, allyPlayerId);
			}
			PUN_CHECK(_playerOwnedManagers[playerId].allyPlayerIds().size() == 0);

			// End all Vassalage...
			std::vector<int32> vassalNodeIds = _playerOwnedManagers[playerId].vassalNodeIds();
			for (int32 vassalNodeId : vassalNodeIds) {
				ArmyNode& armyNode = building(vassalNodeId).subclass<ArmyNodeBuilding>().GetArmyNode();
				armyNode.lordPlayerId = armyNode.originalPlayerId;
				
				_playerOwnedManagers[playerId].LoseVassal(vassalNodeId);
			}
			PUN_CHECK(_playerOwnedManagers[playerId].vassalNodeIds().size() == 0);

			// Detach from lord
			{
				int32 lordPlayerId = townhall(playerId).armyNode.lordPlayerId;
				
				PUN_DEBUG2("[BEFORE] Detach from lord %d, size:%llu", lordPlayerId, _playerOwnedManagers[lordPlayerId].vassalNodeIds().size());
				_playerOwnedManagers[lordPlayerId].LoseVassal(playerId);
				PUN_DEBUG2("[AFTER] Detach from lord %d, size:%llu", lordPlayerId, _playerOwnedManagers[lordPlayerId].vassalNodeIds().size());
			}

			std::vector<WorldRegion2> overlapRegions = _provinceSystem.GetRegionOverlaps(provinceIds);
			for (WorldRegion2 region : overlapRegions)
			{
				// Destroy all buildings owned by player
				buildingList.ExecuteRegion(region, [&](int32 buildingId)
				{
					Building& bld = building(buildingId);

					if (bld.playerId() == command.playerId) {
						soundInterface()->TryStopBuildingWorkSound(bld);
						_buildingSystem->RemoveBuilding(buildingId);
						_regionToDemolishDisplayInfos[region.regionId()].push_back({ bld.buildingEnum(), bld.area(), Time::Ticks() });
					}
				});
			}

			for (int32 provinceId : provinceIds)
			{
				// Destroy all drops
				_dropSystem.ResetProvinceDrop(provinceId);

				// Clear Road
				_overlaySystem.ClearRoadInProvince(provinceId);

				// Remove all gather marks
				TileArea area = _provinceSystem.GetProvinceRectArea(provinceId);
				_treeSystem->MarkArea(playerId, area, true, ResourceEnum::None);

				// Release region ownership
				SetProvinceOwner(provinceId, -1);
			}

			// Remove Trade Route
			worldTradeSystem().RemoveAllTradeRoutes(playerId);

			// Reset all Systems
			_resourceSystems[playerId] = ResourceSystem(playerId, this);
			_unlockSystems[playerId] = UnlockSystem(playerId, this);
			_questSystems[playerId] = QuestSystem(playerId, this);
			_playerOwnedManagers[playerId] = PlayerOwnedManager(playerId, this);
			_playerParameters[playerId] = PlayerParameters(playerId, this);
			_statSystem.ResetPlayer(playerId);
			_popupSystems[playerId] = PopupSystem(playerId, this);
			_cardSystem[playerId] = BuildingCardSystem(playerId, this);

			_aiPlayerSystem.push_back(AIPlayerSystem(playerId, this, this));

			_eventLogSystem.ResetPlayer(playerId);

			_playerIdToNonRepeatActionToAvailableTick[playerId] = std::vector<int32>(static_cast<int>(NonRepeatActionEnum::Count), 0);

			AddEventLogToAllExcept(playerId, playerName(playerId) + " abandoned the old town to start a new one.", false);
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

			if (!playerOwned(requesterPlayerId).IsAlly(command.playerId))
			{
				GainAlly(requesterPlayerId, command.playerId);

				// Tell requester alliance is accepted
				if (requesterPlayerId == playerId()) {
					AddPopup(requesterPlayerId, "Alliance request accepted by " + playerName(command.playerId) + ".");
					AddPopup(command.playerId, "You are now allied with " + playerName(requesterPlayerId) + ".");
				}
			}
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
			popQuest->townSizeTier = 1;
			AddQuest(command.playerId, popQuest);
		}
	}
	else if (replyReceiver == PopupReceiverEnum::None)
	{

	}
	else {
		UE_DEBUG_BREAK();
	}

}

void GameSimulationCore::RerollCards(FRerollCards command)
{
	UE_LOG(LogNetworkInput, Log, TEXT(" RerollCards"));
	
	auto& resourceSys = resourceSystem(command.playerId);
	auto& cardSys = cardSystem(command.playerId);
	
	int32 rerollPrice = cardSys.GetRerollPrice();

	if (rerollPrice == 0) { // Free reroll
		cardSys.RollHand(cardSys.handSize(), true);
	}
	else if (resourceSys.money() >= rerollPrice) {
		cardSys.RollHand(cardSys.handSize(), true);
		resourceSys.ChangeMoney(-rerollPrice);
	} else {
		AddPopupToFront(command.playerId, "Not enough money for reroll", ExclusiveUIEnum::CardHand1, "PopupCannot");
	}

	cardSys.SetPendingCommand(false);
}

void GameSimulationCore::SelectRareCard(FSelectRareCard command)
{
	_LOG(LogNetworkInput, " SelectRareCard");
	
	// TODO: rename this to just select card?
	auto& cardSys = cardSystem(command.playerId);

	if (cardSys.CanSelectRareCardPrize(command.cardEnum)) {
		cardSys.AddCardToHand2(command.cardEnum);
		cardSys.DoneSelectRareHand();
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
			resourceSystem(command.playerId).ChangeMoney(-cardSys.GetCardPrice(buildingEnum)); // Must ChangeMoney before adding cards for cards with lvl...
			cardSys.AddCardToHand2(buildingEnum, true);
		}
	}
	
	cardSys.SetCardStackBlank(true);
	cardSys.SetPendingCommand(false);
}

void GameSimulationCore::SellCards(FSellCards command)
{
	UE_LOG(LogNetworkInput, Log, TEXT(" SellCards"));
	
	int32 sellTotal = cardSystem(command.playerId).RemoveCards(command.buildingEnum, 1);
	if (sellTotal != -1) {
		resourceSystem(command.playerId).ChangeMoney(sellTotal);

		AddEventLogF(command.playerId, FString::Printf(TEXT("Sold %s card for %d gold"), ToTChar(GetBuildingInfo(command.buildingEnum).name), sellTotal), true);
	}
}

void GameSimulationCore::UseCard(FUseCard command)
{
	UE_LOG(LogNetworkInput, Log, TEXT(" UseCard"));
	
	auto& cardSys = cardSystem(command.playerId);
	auto& resourceSys = resourceSystem(command.playerId);

	// Special case: CardRemoval
	if (command.cardEnum == CardEnum::CardRemoval)
	{
		if (cardSys.HasBoughtCard(CardEnum::CardRemoval))
		{
			std::string cardName = GetBuildingInfoInt(command.variable1).name;
			AddPopupToFront(command.playerId, "Removed " + cardName + " Card from your deck. " + cardName + " is now only available from Wild Cards.");
			
			cardSys.RemoveCards(CardEnum::CardRemoval, 1);
			cardSys.RemoveDrawCards(static_cast<CardEnum>(command.variable1));
		}
		return;
	}

	if (IsGlobalSlotCard(command.cardEnum))
	{
		if (cardSys.CanAddCardToTownhall()) {
			int32 soldPrice = cardSys.RemoveCards(command.cardEnum, 1);
			if (soldPrice != -1) {
				cardSys.AddCardToTownhall(command.GetCardStatus(_gameManager->GetDisplayWorldTime() * 100.0f));
			}
		} else {
			AddPopupToFront(command.playerId, "Townhall card slots full.");
		}
		return;
	}

	if (IsBuildingSlotCard(command.cardEnum))
	{
		Building& bld = building(command.variable1);
		if (bld.CanAddSlotCard()) {
			int32 soldPrice = cardSys.RemoveCards(command.cardEnum, 1);
			if (soldPrice != -1) {
				bld.AddSlotCard(command.GetCardStatus(_gameManager->GetDisplayWorldTime() * 100.0f));
			}
		}
		return;
	}
	
	cardSys.RemoveCards(command.cardEnum, 1);

	if (command.cardEnum == CardEnum::Treasure) {
		resourceSystem(command.playerId).ChangeMoney(500);
		AddPopupToFront(command.playerId, "Gained 500<img id=\"Coin\"/> from treasure.");
	}
	else if (command.cardEnum == CardEnum::EmergencyRations) {
		resourceSystem(command.playerId).AddResourceGlobal(ResourceEnum::Wheat, 50, *this);
		AddPopupToFront(command.playerId, "Gained " + to_string(50) + " " + GetResourceInfo(ResourceEnum::Wheat).name + " from emergency ration.");
	}
	else if (IsSeedCard(command.cardEnum)) 
	{
		SeedInfo seedInfo = GetSeedInfo(command.cardEnum);
		std::string plantName = GetTileObjInfo(seedInfo.tileObjEnum).name;
		
		// Didn't have any seed before, unlock farm
		if (resourceSystem(command.playerId).seedsPlantOwned().size() == 0) {
			unlockSystem(command.playerId)->UnlockBuilding(CardEnum::Farm);
			AddPopupToFront(command.playerId, "Unlocked farm!");
		}
		
		resourceSystem(command.playerId).AddSeed(seedInfo);

		if (IsCommonSeedCard(command.cardEnum)) {
			AddPopupToFront(command.playerId, "Unlocked " + plantName +". Switch your farm's workmode to grow " + plantName + ".");
		} else {
			PUN_CHECK(IsSpecialSeedCard(command.cardEnum));
			
			AddPopupToFront(command.playerId, "Unlocked " + plantName + ". " + plantName + " requires suitable regions marked on the map to be grown.");
		}
	}
	else if (IsCrateCard(command.cardEnum)) {
		ResourcePair resourcePair = GetCrateResource(command.cardEnum);
		resourceSystem(command.playerId).AddResourceGlobal(resourcePair.resourceEnum, resourcePair.count, *this);
		AddPopupToFront(command.playerId, "Gained " + to_string(resourcePair.count) + " " + GetResourceInfo(resourcePair.resourceEnum).name + " from crates.");
	}
	else if (command.cardEnum == CardEnum::Pig) {
		//resourceSys.pigs += 3;
	}
	else if (command.cardEnum == CardEnum::Sheep) {
		//resourceSys.sheep += 3;
	}
	else if (command.cardEnum == CardEnum::Cow) {
		//resourceSys.cows += 3;
	}
	else if (command.cardEnum == CardEnum::Panda) {
		//resourceSys.pandas += 3;
	}

	else if (command.cardEnum == CardEnum::SellFood) {
		int32 totalRemoved = 0;
		for (ResourceEnum foodEnum : FoodEnums) {
			int32 amountToRemove = resourceSys.resourceCount(foodEnum) / 2;
			resourceSys.RemoveResourceGlobal(foodEnum, amountToRemove);
			resourceSys.ChangeMoney(amountToRemove * FoodCost);
			totalRemoved += amountToRemove;
		}
		AddPopupToFront(command.playerId, "Sold " + to_string(totalRemoved) + " food for " + to_string(totalRemoved * FoodCost) + "<img id=\"Coin\"/>.");
	}
	else if (command.cardEnum == CardEnum::BuyWood) {
		int32 cost = GetResourceInfo(ResourceEnum::Wood).basePrice;
		int32 amountToBuy = resourceSys.money() / 2 / cost;
		resourceSys.AddResourceGlobal(ResourceEnum::Wood, amountToBuy, *this);
		int32 moneyPaid = amountToBuy * cost;
		resourceSys.ChangeMoney(-moneyPaid);

		AddPopupToFront(command.playerId, "Bought " + to_string(amountToBuy) + " wood for " + to_string(moneyPaid)  + "<img id=\"Coin\"/>.");
	}
	else if (command.cardEnum == CardEnum::Immigration) {
		townhall(command.playerId).AddImmigrants(5);
		AddPopupToFront(command.playerId, "5 immigrants joined after hearing the advertisement.");
	}

	else {
		UE_DEBUG_BREAK();
	}
}

void GameSimulationCore::UnslotCard(FUnslotCard command)
{
	PUN_ENSURE(IsValidBuilding(command.buildingId), return);

	UE_LOG(LogNetworkInput, Log, TEXT(" UnslotCard"));
	
	Building& bld = building(command.buildingId);
	PUN_CHECK(bld.playerId() == command.playerId);
	auto& cardSys = cardSystem(command.playerId);

	if (bld.isEnum(CardEnum::Townhall))
	{
		CardEnum cardEnum = cardSys.RemoveCardFromTownhall(command.unslotIndex);
		if (cardEnum != CardEnum::None) {
			cardSys.AddCardToHand2(cardEnum);
		}
	}
	else
	{
		CardEnum cardEnum = bld.RemoveSlotCard();
		if (cardEnum != CardEnum::None) {
			cardSys.AddCardToHand2(cardEnum);
		}
	}
}

// TODO: move this into barrack
//bool GameSimulationCore::CanTrainUnit(int32 buildingId)
//{
//	Barrack& barrack = building(buildingId).subclass<Barrack>();
//	int32 playerId = barrack.playerId();
//	const ArmyInfo& info = barrack.armyInfo();
//	
//	if (!HasEnoughResource(playerId, info.resourceCost)) {
//		AddPopupToFront(playerId, { "Not enough resource for training." }, ExclusiveUIEnum::None, "PopupCannot");
//		return false;
//	}
//	if (money(playerId) < info.moneyCost) {
//		AddPopupToFront(playerId, { "Not enough money for training." }, ExclusiveUIEnum::None, "PopupCannot");
//		return false;
//	}
//	return true;
//}
//void GameSimulationCore::TrainUnit(FTrainUnit command)
//{
//	//PUN_ENSURE(IsValidBuilding(command.buildingId), return);
//	//
//	//_LOG(LogNetworkInput, " TrainUnit: cancel?%d", command.isCancel);
//
//	//if (command.isCancel)
//	//{
//	//	building(command.buildingId).subclass<Barrack>().TryCancelTrainingQueue();
//	//}
//	//else
//	//{
//	//	if (CanTrainUnit(command.buildingId)) {
//	//		building(command.buildingId).subclass<Barrack>().QueueTrainUnit();
//	//	}
//	//}
//}

// Any army order, not just attack anymore...
void GameSimulationCore::Attack(FAttack command)
{
	PUN_ENSURE(IsValidBuilding(command.originNodeId), return);
	PUN_ENSURE(IsValidBuilding(command.targetNodeId), return);
	
	_LOG(LogNetworkInput, " Attack");

	ArmyNode& originNode = GetArmyNode(command.originNodeId);
	ArmyNode& targetNode = GetArmyNode(command.targetNodeId);

	// Marked targetNode as visited
	playerOwned(command.playerId).TryAddArmyNodeVisited(targetNode.nodeId);

	auto makeMarchGroup = [&](std::vector<int32>& armyCountsIn, int32 helpPlayerId) {
		return ArmyGroup(command.playerId, command.targetNodeId, armyCountsIn, helpPlayerId);
	};

	CallbackEnum orderEnum = command.armyOrderEnum;

	if (orderEnum == CallbackEnum::ArmyRebel)
	{
		// Rebeling: switch ordering player's army to attacking army...
		ArmyGroup* rebelGroup1 = originNode.GetRebelGroup(command.playerId);
		if (rebelGroup1) {
			ArmyGroup rebelGroup = CppUtils::RemoveOneIf(targetNode.rebelGroups, [&](const ArmyGroup& group) { return group.playerId == command.playerId; });
			targetNode.attackGroups.push_back(rebelGroup);
		}
	}
	else if (orderEnum == CallbackEnum::ArmyConquer)
	{
		ArmyGroup* group = originNode.GetArmyGroup(command.playerId);
		if (group) {
			std::vector<int32> armyCounts = group->RemoveArmyPartial(CppUtils::ArrayToVec(command.armyCounts));
			GetArmyNode(command.targetNodeId).MarchStart(makeMarchGroup(armyCounts, -1), originNode);

			AddPopup(targetNode.originalPlayerId, playerName(command.playerId) + " had launched an attack against you. If you lose this battle, you will become " + playerName(command.playerId) + "'s vassal");
		}
	}
	else if (orderEnum == CallbackEnum::ArmyRecall)
	{
		ArmyGroup* groupPtr = originNode.GetArmyGroup(command.playerId);
		if (groupPtr) {
			// existing army in attack/defense group depart to targetNode
			std::vector<int32> armyCounts = groupPtr->GetArmyCounts();
			groupPtr->ClearArmy();
			GetArmyNode(command.targetNodeId).MarchStart(makeMarchGroup(armyCounts, command.playerId), originNode);
		}

		// if there are arriving armies calculate the tile where it is, and go to targetNode
		std::vector<ArmyGroup> playerArrivalGroups = originNode.RemoveArrivalArmyGroups(command.playerId);
		for (ArmyGroup& arrGroup : playerArrivalGroups) {
			std::vector<int32> armyCounts = arrGroup.GetArmyCounts();
			GetArmyNode(command.targetNodeId).MarchStart(makeMarchGroup(armyCounts, command.playerId), originNode, ArmyUtils::GetArmyTile(arrGroup, this));
		}
	}
	else if (orderEnum == CallbackEnum::ArmyReinforce)
	{
		ArmyGroup* group = originNode.GetArmyGroup(command.playerId);
		if (group) 
		{
			ArmyGroup* targetGroup = targetNode.GetArmyGroup(command.playerId);
			if (!targetGroup) {
				targetGroup = targetNode.GetArrivingGroup(command.playerId); // Use arriving group as example instead of there is no army at destination
			}
			
			if (targetGroup) {
				std::vector<int32> armyCounts = group->RemoveArmyPartial(CppUtils::ArrayToVec(command.armyCounts));

				// when reinforcing, we keep the same intention as the army operating in the region (just in case that army is dead, this army can find the correct purpose)
				GetArmyNode(command.targetNodeId).MarchStart(makeMarchGroup(armyCounts, targetGroup->helpPlayerId), originNode);
			}
		}
	}
	else if (orderEnum == CallbackEnum::ArmyHelp)
	{
		ArmyGroup* group = originNode.GetArmyGroup(command.playerId);
		if (group)
		{
			std::vector<int32> armyCounts = group->RemoveArmyPartial(CppUtils::ArrayToVec(command.armyCounts));

			// when reinforcing, we keep the same intention as the army operating in the region (just in case that army is dead, this army can find the correct purpose)
			GetArmyNode(command.targetNodeId).MarchStart(makeMarchGroup(armyCounts, command.helpPlayerId), originNode);
		}
	}
	else if (orderEnum == CallbackEnum::ArmyLiberate)
	{
		ArmyGroup* group = originNode.GetArmyGroup(command.playerId);
		if (group) {
			std::vector<int32> armyCounts = group->RemoveArmyPartial(CppUtils::ArrayToVec(command.armyCounts));
			ArmyNode& node = GetArmyNode(command.targetNodeId);
			node.MarchStart(makeMarchGroup(armyCounts, node.originalPlayerId), originNode);
		}
	}
	else if (orderEnum == CallbackEnum::ArmyMoveBetweenNode)
	{
		ArmyGroup* group = originNode.GetArmyGroup(command.playerId);
		if (group) {
			std::vector<int32> armyCounts = group->RemoveArmyPartial(CppUtils::ArrayToVec(command.armyCounts));
			GetArmyNode(command.targetNodeId).MarchStart(makeMarchGroup(armyCounts, command.playerId), originNode);
		}
	}
	// Ally
	else if (orderEnum == CallbackEnum::AllyRequest)
	{
		int32 requesterPlayerId = building(command.originNodeId).playerId();
		int32 targetPlayerId = building(command.targetNodeId).playerId();
		
		PopupInfo info(targetPlayerId, playerName(requesterPlayerId) + " is requesting an alliance with you. Will you accept?", { "Yes", "No" }, PopupReceiverEnum::AllyRequest);
		info.replyVar1 = requesterPlayerId;
		AddPopup(info);
	}
	else if(orderEnum == CallbackEnum::AllyBetray)
	{
		int32 betrayalTargetPlayerId = building<TownHall>(command.targetNodeId).playerId();

		if (playerOwned(command.playerId).IsAlly(betrayalTargetPlayerId)) 
		{
			LoseAlly(command.playerId, betrayalTargetPlayerId);

			AddPopup(betrayalTargetPlayerId, playerName(command.playerId) + " ended the alliance with you.");
			AddPopup(command.playerId, "You ended the alliance with " + playerName(betrayalTargetPlayerId) + ".");
		}

		// If your army is stationed here, become an attacker...

		// If your army is marked as helping this ally directly in other nodes the state becomes helpPlayerId == -1 instead...
	}
	else if (orderEnum == CallbackEnum::ArmyRetreat)
	{
		// original owner retreat... 
		ArmyGroup* retreatGroup = targetNode.GetArmyGroup(command.playerId);
		if (retreatGroup) {
			// The army goes into hiding...
			std::vector<int32> armyCounts = retreatGroup->GetArmyCounts();
			armyCounts[static_cast<int>(ArmyEnum::Tower)] = 0;
			targetNode.rebelGroups.push_back(ArmyGroup(command.playerId, command.targetNodeId, armyCounts, -1));

			CppUtils::RemoveIf(targetNode.defendGroups, [&](const ArmyGroup& group) { return group.playerId == command.playerId; });
			CppUtils::RemoveIf(targetNode.attackGroups, [&](const ArmyGroup& group) { return group.playerId == command.playerId; });
		}
	}
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
	
	int32 playerId = command.playerId;
	PlayerOwnedManager& playerOwn = playerOwned(playerId);
	int32 provinceId = command.provinceId;

	// Special case for claim ruin
	if (command.claimEnum == CallbackEnum::ClaimRuin)
	{
		PUN_LOG("Investigate Ruin");
		//if (georesourceSystem().CanClaimRuin(playerId, command.regionId)) {
		//	georesourceSystem().ClaimRuin(playerId, command.regionId);
		//}
		return;
	}

	auto& resourceSys = resourceSystem(playerId);

	if (command.claimEnum == CallbackEnum::ClaimLandFood)
	{
		int32 baseRegionPrice = playerOwn.GetBaseProvinceClaimPrice(command.provinceId);
		int32 neededFood = baseRegionPrice / FoodCost;
		
		if (foodCount(playerId) >= neededFood &&
			provinceOwner(command.provinceId) == -1)
		{
			SetProvinceOwnerFull(command.provinceId, playerId);

			// TODO: change below to RemoveResourceGlobal(ResourceEnum::Food, ..)
			int32 amountLeftToRemove = neededFood;
			for (ResourceEnum foodEnum : FoodEnums) {
				int32 amountToRemove = std::min(amountLeftToRemove, resourceSys.resourceCount(foodEnum));
				resourceSys.RemoveResourceGlobal(foodEnum, amountToRemove);
				amountLeftToRemove -= amountToRemove;
			}
			PUN_CHECK(amountLeftToRemove == 0);
		}
	}
	else if (command.claimEnum == CallbackEnum::ClaimLandMoney)
	{
		int32 regionPriceMoney = GetProvinceClaimPrice(command.provinceId);
		
		if (resourceSys.money() >= regionPriceMoney &&
			provinceOwner(command.provinceId) == -1)
		{
			SetProvinceOwnerFull(command.provinceId, playerId);
			resourceSys.ChangeMoney(-regionPriceMoney);
		}
	}
	else if (command.claimEnum == CallbackEnum::ClaimLandInfluence)
	{
		int32 regionPriceMoney = GetProvinceClaimPrice(command.provinceId);

		if (resourceSys.influence() >= regionPriceMoney &&
			provinceOwner(command.provinceId) == -1)
		{
			SetProvinceOwnerFull(command.provinceId, playerId);
			resourceSys.ChangeInfluence(-regionPriceMoney);
		}
	}

	/*
	 * Battle
	 */
	else if (command.claimEnum == CallbackEnum::StartAttackProvince)
	{
		int32 provincePlayerId = provinceOwner(command.provinceId);
		auto& provincePlayerOwner = playerOwned(provincePlayerId);

		// If there was no claim yet, start the conquer
		if (!provincePlayerOwner.GetDefendingClaimProgress(command.provinceId).isValid()) 
		{
			int32 conquerPrice = GetProvinceClaimPrice(provinceId) * 2 + BattleInfluencePrice;
			
			if (influence(command.playerId) >= conquerPrice)
			{
				resourceSystem(command.playerId).ChangeInfluence(-conquerPrice);
				
				provincePlayerOwner.StartConquerProvince(command.playerId, command.provinceId);
				playerOwned(command.playerId).StartConquerProvince_Attacker(command.provinceId);

				AddPopup(provincePlayerId, playerName(command.playerId) + " is trying to take over your territory.\nIf you lose the province, all its buildings will be destroyed.");
			}
		}
	}
	else if (command.claimEnum == CallbackEnum::ReinforceAttackProvince)
	{
		auto& provincePlayerOwner = playerOwned(provinceOwner(command.provinceId));
		
		if (provincePlayerOwner.GetDefendingClaimProgress(command.provinceId).isValid()) 
		{
			resourceSystem(command.playerId).ChangeInfluence(-BattleInfluencePrice);
			provincePlayerOwner.ReinforceAttacker(command.provinceId, BattleInfluencePrice);
		}
	}
	else if (command.claimEnum == CallbackEnum::DefendProvinceInfluence)
	{
		auto& provincePlayerOwner = playerOwned(provinceOwner(command.provinceId));

		if (provincePlayerOwner.GetDefendingClaimProgress(command.provinceId).isValid() &&
			influence(command.playerId) >= BattleInfluencePrice)
		{
			resourceSystem(command.playerId).ChangeInfluence(-BattleInfluencePrice);
			provincePlayerOwner.ReinforceDefender(command.provinceId, BattleInfluencePrice);
		}
	}
	else if (command.claimEnum == CallbackEnum::DefendProvinceMoney)
	{
		auto& provincePlayerOwner = playerOwned(provinceOwner(command.provinceId));

		if (provincePlayerOwner.GetDefendingClaimProgress(command.provinceId).isValid() &&
			money(command.playerId) >= BattleInfluencePrice)
		{
			ChangeMoney(command.playerId, -BattleInfluencePrice);
			provincePlayerOwner.ReinforceDefender(command.provinceId, BattleInfluencePrice);
		}
	}
	
	else if (command.claimEnum == CallbackEnum::ClaimLandArmy)
	{
		playerOwn.QueueArmyProvinceClaim(command.provinceId);
	}
	else if (command.claimEnum == CallbackEnum::CancelClaimLandArmy)
	{
		playerOwn.CancelArmyProvinceClaim(command.provinceId);
	}
	
	//else if (command.claimEnum == CallbackEnum::ClaimLandIndirect)
	//{
	//	int32 price = playerOwn.GetInfluenceClaimPrice(command.provinceId);

	//	if (resourceSys.money() >= price &&
	//		provinceOwner(command.provinceId) == -1)
	//	{
	//		SetProvinceOwner(command.provinceId, playerId);

	//		resourceSys.ChangeMoney(-price);
	//	}
	//}
	else if (command.claimEnum == CallbackEnum::BuildOutpost)
	{
		int32 price = playerOwn.GetOutpostClaimPrice(command.provinceId);

		if (resourceSys.money() >= price &&
			provinceOwner(command.provinceId) == -1)
		{
			playerOwn.MarkAsOutpost(provinceId);

			resourceSys.ChangeMoney(-price);
		}
	}
	else if (command.claimEnum == CallbackEnum::DemolishOutpost)
	{
		if (playerOwn.TryRemoveOutpost(provinceId))
		{
			int32 price = playerOwn.GetOutpostClaimPrice(command.provinceId) / 2;
			resourceSys.ChangeMoney(-price);
		}
	}
	
	else {
		UE_DEBUG_BREAK();
	}
}

void GameSimulationCore::SetProvinceOwnerFull(int32 provinceId, int32 playerId)
{
	PUN_CHECK(_provinceSystem.IsProvinceValid(provinceId));
	PUN_CHECK(playerId != -1);

	PlayerOwnedManager& playerOwn = playerOwned(playerId);

	int32 previousOwnerId = provinceOwner(provinceId);
	
	SetProvinceOwner(provinceId, playerId);

	// Succeed claiming land
	{
		// Unlock bridge if there is a river or coast
		if (_provinceSystem.provinceRiverTileCount(provinceId) > 0 ||
			_provinceSystem.provinceOceanTileCount(provinceId) > 0)
		{
			if (!unlockSystem(playerId)->isUnlocked(CardEnum::Bridge)) {
				AddPopupToFront(playerId, "Unlocked bridge!");
				unlockSystem(playerId)->UnlockBuilding(CardEnum::Bridge);
			}
		}
		
		QuestUpdateStatus(playerId, QuestEnum::ClaimLandQuest);

		// Desert, unlock the desert pilgrim card
		if (GetBiomeProvince(provinceId) == BiomeEnum::Desert &&
			!parameters(playerId)->DesertPilgrimOffered)
		{
			cardSystem(playerId).AddDrawCards(CardEnum::DesertPilgrim, 1);
			parameters(playerId)->DesertPilgrimOffered = true;
		}

		// Taking over proper land with plantation research grants seeds
		CheckGetSeedCard(playerId);
		
		// Claim land hand
		int32 regionsClaimed = playerOwn.provincesClaimed().size();

		// Remove any existing regional building and give the according bonus...
		if (regionsClaimed > 1)
		{
			const std::vector<WorldRegion2>& regionOverlaps = _provinceSystem.GetRegionOverlaps(provinceId);

			for (WorldRegion2 regionOverlap : regionOverlaps)
			{
				auto& buildingList = buildingSystem().buildingSubregionList();
				buildingList.ExecuteRegion(regionOverlap, [&](int32 buildingId)
				{
					auto bld = building(buildingId);
					if (IsRegionalBuilding(bld.buildingEnum()) && 
						GetProvinceIdClean(bld.centerTile()) == provinceId)
					{
						if (bld.isEnum(CardEnum::RegionTribalVillage)) {
							ImmigrationEvent(playerId, 5, GenerateTribeName(bld.buildingId()) + " wish to join your city.", PopupReceiverEnum::TribalJoinEvent);
							//townhall(playerId).ImmigrationEvent(5, GenerateTribeName(bld.buildingId()) + " wish to join your city.", PopupReceiverEnum::TribalJoinEvent);
							ClearProvinceBuildings(bld.provinceId());
						}
						else if (bld.isEnum(CardEnum::RegionCrates)) {
							GenerateRareCardSelection(playerId, RareHandEnum::CratesCards, "Searching through the crates you found.");
							ClearProvinceBuildings(bld.provinceId());
						}
						else if (bld.isEnum(CardEnum::RegionShrine)) {
							GenerateRareCardSelection(playerId, RareHandEnum::BuildingSlotCards, "The shrine bestows its wisdom upon us.");
						}

					}
				});
			}
		}

		/*
		 * Outpost
		 * TODO: remove this??
		 */
		//// You own an outpost here dismantle it with money back.
		//if (playerOwn.TryRemoveOutpost(provinceId))
		//{
		//	ChangeMoney(playerId, playerOwn.GetOutpostClaimPrice(provinceId));
		//	AddPopupToFront(playerId, "You expanded your city into a province with an outpost. The outpost was dismantled with its build cost returned to you.");
		//}
		//// Other ppl own an outpost here dismantle it.
		//if (previousOwnerId != -1) {
		//	playerOwned(previousOwnerId).TryRemoveOutpost(provinceId);
		//}
	}
}

void GameSimulationCore::ChooseLocation(FChooseLocation command)
{
	_LOG(LogNetworkInput, " ChooseLocation");

	// No duplicate choosing...
	if (playerOwned(command.playerId).isInitialized()) {
		return;
	}
	
	if (command.isChoosingOrReserving)
	{
		// Find the start spot to double check
		TileArea startArea = SimUtils::FindStartSpot(command.provinceId, this);

		if (!startArea.isValid()) {
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
		
		// Build Townhall
		{
			FPlaceBuildingParameters params;
			params.buildingEnum = static_cast<uint8>(CardEnum::Townhall);
			params.faceDirection = static_cast<uint8>(Direction::S);
			params.center = townhallCenter;

			WorldTile2 size = GetBuildingInfo(CardEnum::Townhall).size;
			params.area = BuildingArea(params.center, size, static_cast<Direction>(params.faceDirection));
			params.playerId = command.playerId;
			int32 townhallId = PlaceBuilding(params);
			PUN_CHECK2(townhallId != -1, ("provinceId:" + to_string(command.provinceId)));

			if (townhallId == -1) {
				return;
			}

			building(townhallId).InstantClearArea();
			building(townhallId).SetAreaWalkable();
			building(townhallId).FinishConstruction();

			// Place road around townhall
			WorldTile2 roadMin(params.area.minX - 1, params.area.minY - 1);
			WorldTile2 roadMax = roadMin + WorldTile2(size.x + 1, size.y + 1);

			std::vector<TileArea> roadAreas;
			roadAreas.push_back(TileArea(roadMin, WorldTile2(size.x + 2, 1))); 
			roadAreas.push_back(TileArea(WorldTile2(roadMin.x, roadMax.y), WorldTile2(size.x + 2, 1))); 
			roadAreas.push_back(TileArea(WorldTile2(roadMin.x, roadMin.y + 1), WorldTile2(1, size.y))); 
			roadAreas.push_back(TileArea(WorldTile2(roadMax.x, roadMin.y + 1), WorldTile2(1, size.y))); 

			for (size_t i = 0; i < roadAreas.size(); i++) {
				_treeSystem->ForceRemoveTileObjArea(roadAreas[i]);
				roadAreas[i].ExecuteOnArea_WorldTile2([&](WorldTile2 tile) {
					if (IsFrontBuildable(tile) && !_overlaySystem.IsRoad(tile)) {
						overlaySystem().AddRoad(tile, true, true);
					}
				});
			}

			// PlayerOwnedManager
			auto& playerOwned = _playerOwnedManagers[command.playerId];
			playerOwned.townHallId = townhallId;
			playerOwned.justChoseLocation = true;
			playerOwned.needChooseLocation = false;
			playerOwned.TryAddArmyNodeVisited(townhallId);
		}

		// Build storage yard
		{
			FPlaceBuildingParameters params;
			params.buildingEnum = static_cast<uint8>(CardEnum::StorageYard);
			params.faceDirection = static_cast<uint8>(Direction::E);


			auto makeStorage = [&]() -> StorageYard*
			{
				params.area = BuildingArea(params.center, WorldTile2(4, 4), static_cast<Direction>(params.faceDirection));
				//params.area2 = params.area; // Storage's special case for Non-Human placement
				params.playerId = command.playerId;

				PUN_LOG("Place storage area: %d, %d, %d ,%d", params.area.minX, params.area.minY, params.area.maxX, params.area.maxY);

				int32 storageId = PlaceBuilding(params);
				if (storageId == -1) {
					return nullptr;
				}

				StorageYard& storage = building(storageId).subclass<StorageYard>(CardEnum::StorageYard);
				storage.InstantClearArea();
				storage.SetAreaWalkable();
				storage.FinishConstruction();

				return &storage;
			};
			
			{
				WorldTile2 storageSize = WorldTile2(4, 4);
				int32 shiftFromTownhall = GetBuildingInfo(CardEnum::Townhall).size.y / 2 + storageSize.y / 2;
				params.center = townhallCenter + WorldTile2(0, -shiftFromTownhall); // Old 8x8 WorldTile2(-2, -shiftFromTownhall);
				
				StorageYard* storagePtr = makeStorage();
				PUN_ENSURE(storagePtr, return);
				StorageYard& storage = *storagePtr;

				// Initial Resources
				if (terrainGenerator().GetBiome(params.center) == BiomeEnum::Jungle) {
					storage.AddResource(ResourceEnum::Papaya, 240);
				} else {
					storage.AddResource(ResourceEnum::Orange, 240);
				}
				
				storage.AddResource(ResourceEnum::Wood, 120);
				storage.AddResource(ResourceEnum::Stone, 120);

				//_LOG(PunResource, "(3)%s", ToTChar(resourceSystem(storage.playerId()).resourcedebugStr()));
			}

			// Extra storage
			{
				params.center = params.center + WorldTile2(4, 0);
				//params.area = BuildingArea(params.center, GetBuildingInfo(CardEnum::StorageYard).size, static_cast<Direction>(params.faceDirection));
				//params.playerId = command.playerId;
				//int32 storageId = PlaceBuilding(params);
				//PUN_ENSURE(storageId != -1, return;);

				//StorageYard& storage = building(storageId).subclass<StorageYard>(CardEnum::StorageYard);
				//storage.InstantClearArea();
				//storage.SetAreaWalkable();
				//storage.FinishConstruction();

				StorageYard* storagePtr = makeStorage();
				PUN_ENSURE(storagePtr, return);
				StorageYard& storage = *storagePtr;

				/*
				 * Must have tools to last at least 3 years. avg pop 30 -> 30  tools?? .... 120 tools
				 * Must have medicine to last as least 6 years. avg pop 30 -> 30*10*8 = 2400... 120 medicine...
				 */
				storage.AddResource(ResourceEnum::SteelTools, 180);
				storage.AddResource(ResourceEnum::Medicine, 240);
			}
		}

		// Ensure Map was refreshed (In case AI built city at tick=0)
		RefreshHeightForestColorTexture(startArea);

		// Give money/seeds
		resourceSystem(command.playerId).SetMoney(1000);
		resourceSystem(command.playerId).SetInfluence(0);

		// EventLog inform all players someone selected a start
		if (!IsAI(command.playerId))
		{
			ExecuteOnConnectedPlayers([&](int32 playerId) {
				AddEventLog(playerId, playerName(command.playerId) + " chose a starting location.", false);
			});
		}

	}
}

void GameSimulationCore::Cheat(FCheat command)
{
	UE_LOG(LogNetworkInput, Log, TEXT(" Cheat"));

	auto& cardSys = cardSystem(command.playerId);
	
	CheatEnum cheatEnum = static_cast<CheatEnum>(command.cheatEnum);
	switch (cheatEnum)
	{
		case CheatEnum::UnlockAll: {
			unlockSystem(command.playerId)->researchEnabled = true;
			unlockSystem(command.playerId)->UnlockAll();
			_popupSystems[command.playerId].ClearPopups();
			break;
		}
		case CheatEnum::Money: resourceSystem(command.playerId).ChangeMoney(30000); break;
		case CheatEnum::Influence: resourceSystem(command.playerId).ChangeInfluence(10000); break;
		
		case CheatEnum::FastBuild: SimSettings::Toggle("CheatFastBuild"); break;
		
		case CheatEnum::Cheat:
			SimSettings::Set("CheatFastBuild", 1);
			resourceSystem(command.playerId).ChangeMoney(100000);
		case CheatEnum::Army:
		{
			std::vector<int32> armyNodeIds = GetArmyNodeIds(command.playerId);
			for (int32 armyNodeId : armyNodeIds) {
				std::vector<int32> armyCounts(ArmyEnumCount, 0);
				armyCounts[1] = 5;
				armyCounts[2] = 5;
				armyCounts[3] = 5;
				GetArmyNode(armyNodeId).AddArmyToCapital(command.playerId, armyCounts);
			}
			break;
		}
		
		case CheatEnum::Resources: {
			for (ResourceEnum resourceEnum : FoodEnums) {
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
		case CheatEnum::Undead: SimSettings::Toggle("CheatUndead"); break;
		case CheatEnum::Immigration: townhall(command.playerId).ImmigrationEvent(30); break;

		case CheatEnum::FastTech: SimSettings::Toggle("CheatFastTech"); break;

		case CheatEnum::ForceFunDown:
		{
			auto setFunDown = [&](const std::vector<int32>& humanIds) {
				for (int32 humanId : humanIds) {
					unitAI(humanId).SetFunTicks(FunTicksAt100Percent * 50 / 100);
				}
			};
			setFunDown(playerOwned(command.playerId).adultIds());
			setFunDown(playerOwned(command.playerId).childIds());
			break;
		}

		case CheatEnum::Kill: _unitSystem->KillHalf(); break;

		case CheatEnum::ClearLand: {
			// Clear trees/deposits from regions owned
			const std::vector<int32>& provinceIds = playerOwned(command.playerId).provincesClaimed();
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

		case CheatEnum::AddResource:
		{
			ResourceEnum resourceEnum = static_cast<ResourceEnum>(command.var1);
			int32 amount = command.var2;
			if (amount > 0) {
				resourceSystem(command.playerId).AddResourceGlobal(resourceEnum, amount, *this);
			} else if (amount < 0) {
				amount = std::min(amount, resourceCount(command.playerId, resourceEnum));
				resourceSystem(command.playerId).RemoveResourceGlobal(resourceEnum, amount);
			}
			break;
		}
		case CheatEnum::AddMoney: {
			resourceSystem(command.playerId).ChangeMoney(command.var1);
			break;
		}
		case CheatEnum::AddInfluence: {
			resourceSystem(command.playerId).ChangeInfluence(command.var1);
			playerOwned(command.playerId).RecalculateTaxDelayed();
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
			townhall(command.playerId).AddImmigrants(addCount);
			break;
		}

		case CheatEnum::YearlyTrade:
		{
			AddPopup(PopupInfo(command.playerId, "A caravan has arrived. They wish to buy any goods you might have.", { "Trade", "Refuse" }, PopupReceiverEnum::CaravanBuyer));
			break;
		}


		case CheatEnum::HouseLevel:
		{
			SimSettings::Set("CheatHouseLevel", command.var1);
			break;
		}
		case CheatEnum::FullFarmRoad: {
			SimSettings::Toggle("CheatFullFarmRoad");
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
			SimSettings::Toggle("MarkedTreesNoDisplay");
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
				SimSettings::Toggle(name);
				SimSettings::Set(name, 1);
			};
				
			setSettings("CheatFullFarmRoad");
			setSettings("NoCameraSnap");
			setSettings("GeoresourceAnywhere");
			setSettings("NoFarmSizeCap");
			setSettings("MarkedTreesNoDisplay");
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

void GameSimulationCore::DemolishCritterBuildingsIncludingFronts(WorldTile2 tile, int32 playerId) 
{
	std::vector<int32> critterBuildings = GetCritterBuildingsIncludeFronts(tile);
	for (int i = critterBuildings.size(); i-- > 0;) {
		// TODO: later on, mark critter building for demolition, in which it gets attacked until destroyed (dropping items)
		BoarBurrow& burrow = building(critterBuildings[i]).subclass<BoarBurrow>();
		UnitInventory inventory = burrow.inventory;

		_buildingSystem->RemoveBuilding(critterBuildings[i]);

		_regionToDemolishDisplayInfos[burrow.centerTile().regionId()].push_back({ CardEnum::BoarBurrow, burrow.area(), Time::Ticks() });

		//inventory.Execute([&](ResourcePair& resource) {
		//	if (resource.count > 0) {
		//		resourceSystem(playerId).SpawnDrop(resource.resourceEnum, resource.count, tile);
		//	}
		//});
	}
}