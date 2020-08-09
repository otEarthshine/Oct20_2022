// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "PunCity/NetworkStructs.h"
#include "IGameSimulationCore.h"

class ReplayPlayer
{
public:
	void Init() {
		currentCommandIndex = 0;
	}

	bool isInitialize() {
		return currentCommandIndex != -1;
	}

	// For recorded play, check HasRecordedPlayerAction to see if this tick has action to replay
	// If there is, GetRecordedPlayerActionThenIncrement and replay the actions
	bool HasRecordedPlayerAction(int32_t tickCountSim) const {
		if (currentCommandIndex >= playerCommands.size()) {
			return false;
		}
		const NetworkTickInfo& tickInfo = playerCommands[currentCommandIndex];
		return tickInfo.tickCountSim <= tickCountSim; // tickCountSim may not be exact same on both playthrough, depending on gameSpeed.
	}
	NetworkTickInfo GetRecordedPlayerActionThenIncrement(int32_t tickCountSim) {
		PUN_CHECK(playerCommands[currentCommandIndex].tickCountSim <= tickCountSim);
		return playerCommands[currentCommandIndex++];
	}

	std::vector<NetworkTickInfo> playerCommands;
	int32 currentCommandIndex = -1;

	/*
	 * Trailer
	 */
	void SetTrailerCommands(std::vector<std::shared_ptr<FNetworkCommand>>& trailerCommandsIn)
	{
		PUN_LOG("Trailer Start %d", trailerCommandsIn.size());
		currentCommandIndex = 0; // for isInitialize()
		trailerCommands = trailerCommandsIn;
		nextTrailerCommandTick = Time::Ticks() + Time::TicksPerSecond;
	}
	void PauseTrailerCommands() {
		pausedNextTrailerCommandTick = nextTrailerCommandTick;
		nextTrailerCommandTick = -1;
	}
	void UnpauseTrailerCommands() {
		nextTrailerCommandTick = pausedNextTrailerCommandTick;
	}

	bool IsTrailerReplay() {
		return nextTrailerCommandTick != -1;
	}
	
	std::vector<std::shared_ptr<FNetworkCommand>> trailerCommands;
	int32 nextTrailerCommandTick = -1;
	int32 pausedNextTrailerCommandTick = -1;

	bool isCameraTrailerReplayPaused = false;
};

/**
 * 
 */
class ReplaySystem
{
public:
	void Init(IGameSimulationCore* simulation) {
		_simulation = simulation;
	}
	
	void AddPlayer() {
		replayPlayers.push_back(ReplayPlayer());
	}

	void SavePlayerActions(int32 playerId, FString fileName);

	void LoadPlayerActions(int32 playerId, FString fileName);

	/*
	 * AddCommands
	 */
	void AddTrailerCommands(std::vector<std::shared_ptr<FNetworkCommand>> commands)
	{
		// Placement gets recorded for trailer
		for (size_t j = 0; j < commands.size(); j++)
		{
			if (commands[j]->playerId == 0)
			{
				switch (commands[j]->commandType())
				{
				case NetworkCommandEnum::ChooseLocation:
				case NetworkCommandEnum::PlaceBuilding:
				case NetworkCommandEnum::PlaceGather:
				case NetworkCommandEnum::ClaimLand:
				case NetworkCommandEnum::Cheat:
				case NetworkCommandEnum::UpgradeBuilding:
					trailerCommandsSave.push_back(commands[j]);
					PUN_LOG("Add trailerCommands[Save] %d %s", trailerCommandsSave.size(), ToTChar(GetNetworkCommandName(commands[j]->commandType())));
					break;
				default:
					break;
				}
			}
		}
	}

	// Add Network Ticks as the game is played
	void AddNetworkTickInfo(NetworkTickInfo& networkTickInfo, std::vector<std::shared_ptr<FNetworkCommand>> commands)
	{
		// Preprocess for commands that requires buildingId (convert to buildingTileId)
		// BuildingId won't be valid in the next game, but buildingTileId will
		for (size_t j = commands.size(); j-- > 0;)
		{
			switch (commands[j]->commandType())
			{
			case NetworkCommandEnum::JobSlotChange: // Correct
			case NetworkCommandEnum::SetAllowResource: // Correct
			case NetworkCommandEnum::SetPriority: // Correct
			case NetworkCommandEnum::ChangeWorkMode: { // Correct
				auto command = std::static_pointer_cast<FBuildingCommand>(commands[j]);
				command->buildingTileId = _simulation->buildingCenter(command->buildingId).tileId();
				command->buildingEnum = _simulation->buildingEnum(command->buildingId);
				command->buildingId = -1;
				break;
			}
			default:
				break;
			}
		}
		
		networkTickInfos.push_back(networkTickInfo);
		//PUN_LOG("Add tick %s", *networkTickInfo.ToString());
	}

	void Serialize(FArchive& Ar) {}

	std::vector<ReplayPlayer> replayPlayers; // 

	//bool isAIInitialized = false;


	
	std::vector<std::shared_ptr<FNetworkCommand>> trailerCommandsSave;
	std::vector<std::shared_ptr<FNetworkCommand>> trailerCommandsLoad;

	void TrailerCityReplayUnpause() {
		for (ReplayPlayer& replayPlayer : replayPlayers) {
			replayPlayer.isCameraTrailerReplayPaused = false;
		}
	}
	
private:
	IGameSimulationCore* _simulation = nullptr;
	
	std::vector<NetworkTickInfo> networkTickInfos;
};
