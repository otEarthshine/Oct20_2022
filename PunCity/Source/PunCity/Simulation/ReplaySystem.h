// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "PunCity/NetworkStructs.h"
#include "PunCity/PunUtils.h"


class ReplayPlayer
{
public:
	void Init() {
		currentCommandIndex = 0;
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
	int32_t currentCommandIndex = -1;
};

/**
 * 
 */
class ReplaySystem
{
public:
	void AddPlayer() {
		replayPlayers.push_back(ReplayPlayer());
	}

	void SavePlayerActions(int32_t playerId, FString fileName);

	void LoadPlayerActions(int32_t playerId, FString fileName);

	// 
	void AddNetworkTickInfo(NetworkTickInfo& networkTickInfo) {
		networkTickInfos.push_back(networkTickInfo);
		//PUN_LOG("Add tick %s", *networkTickInfo.ToString());
	}

	void Serialize(FArchive& Ar)
	{
		//TODO: std::vector<ReplayPlayer> replayPlayers;??

		//if (Ar.IsSaving())
		//{
		//	TArray<int32> blob;
		//	int32 vecSize = networkTickInfos.size();
		//	for (size_t i = 0; i < vecSize; i++) {
		//		networkTickInfos[i].SerializeToBlob(blob);
		//	}
		//	Ar << vecSize;
		//	Ar << blob;
		//}
		//else
		//{
		//	TArray<int32> blob;
		//	int32 vecSize;
		//	
		//	Ar << vecSize;
		//	Ar << blob;

		//	int index = 0;
		//	for (size_t i = 0; i < vecSize; i++) {
		//		networkTickInfos.push_back(NetworkTickInfo());
		//		networkTickInfos[i].DeserializeFromBlob(blob, index);
		//	}
		//}
	}

	std::vector<ReplayPlayer> replayPlayers;

	//bool isAIInitialized = false;

private:
	std::vector<NetworkTickInfo> networkTickInfos;
};
