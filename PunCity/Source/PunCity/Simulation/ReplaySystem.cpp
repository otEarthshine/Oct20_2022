// Fill out your copyright notice in the Description page of Project Settings.


#include "ReplaySystem.h"
#include "Misc/Paths.h"
#include <fstream>
#include <iterator>
#include "PunCity/PunUtils.h"
#include "PunCity/PunTimer.h"
#include "Misc/FileHelper.h"

using namespace std;

void ReplaySystem::SavePlayerActions(int32 playerId, FString fileName)
{
	SCOPE_TIMER("SavePlayerActions");
	
	FString path = FPaths::ConvertRelativePathToFull(FPaths::ProjectSavedDir());
	path += fileName;

	// Terrain;
	std::ofstream file;
	file.open(TCHAR_TO_UTF8(*(path)), ios::out | ios::binary);

	std::vector<int32> saveVector;

	for (size_t i = 0; i < networkTickInfos.size(); i++)
	{
		NetworkTickInfo networkTickInfo = networkTickInfos[i];
		std::vector<std::shared_ptr<FNetworkCommand>>& commands = networkTickInfo.commands;

		// Trim any command that isn't from this player
		for (size_t j = commands.size(); j-- > 0;) {
			if (commands[j]->playerId != playerId) {
				commands.erase(commands.begin() + j);
			}
		}

		// Trim any command type that won't need replay
		for (size_t j = commands.size(); j-- > 0;) 
		{
			switch (commands[j]->commandType())
			{
			case NetworkCommandEnum::TradeResource:
			case NetworkCommandEnum::PopupDecision:	
			//case NetworkCommandEnum::SelectRareCard: // Test?

			case NetworkCommandEnum::SetIntercityTrade:
			case NetworkCommandEnum::BuyCard:			
			case NetworkCommandEnum::SellCards:		
			case NetworkCommandEnum::UseCard:		
			case NetworkCommandEnum::UnslotCard:
			case NetworkCommandEnum::ChooseResearch:
				commands.erase(commands.begin() + j);
				break;
			default:
				break;
			}
		}

		// Record the tick with non-empty commands
		if (commands.size() > 0) 
		{
			PunSerializedData blob(true);
			networkTickInfo.SerializeToBlob(blob);

			// For each NetworkTickInfo, record its size, before adding the blob
			saveVector.push_back(blob.Num());
			for (int32 j = 0; j < blob.Num(); j++) {
				saveVector.push_back(blob[j]);
			}

			//PUN_LOG("Saving(size32:%d) %s", blob.Num(), *networkTickInfo.ToString());
		}
	}

	int size = sizeof(saveVector[0]) * saveVector.size();
	file.write(reinterpret_cast<char*>(&saveVector[0]), size);

	file.close();
	file.clear();

	//PUN_LOG("Save Size32: %d", saveVector.size());
	//PUN_LOG("Saved... File Exist: %d, at path: %s", FPaths::FileExists(path), *path);
}

void ReplaySystem::LoadPlayerActions(int32 playerId, FString fileName)
{
	PUN_DEBUG2("LoadPlayerActions: pid:%d %s", playerId, *fileName);
	
	FString path = FPaths::ConvertRelativePathToFull(FPaths::ProjectSavedDir());
	path += fileName;

	if (!FPaths::FileExists(path)) {
		PUN_DEBUG2("No File to load %s", *path);
		return;
	}

	// Terrain:
	ifstream file;
	file.open(TCHAR_TO_UTF8(*(path)), ios::in | ios::binary);

	// Get length of file:
	file.seekg(0, file.end);
	int length = file.tellg();
	file.seekg(0, file.beg);

	check(length % 4 == 0);
	length /= 4; // int32 recorded

	PUN_LOG("File Exist: %d, at path: %s, size32:%d", FPaths::FileExists(path), *path, length);

	std::vector<int32> loadVector(length);

	PUN_LOG("Load Size32: %d", loadVector.size());

	int size = sizeof(loadVector[0]) * loadVector.size();
	file.read(reinterpret_cast<char*>(&loadVector[0]), size);

	file.close();
	file.clear();

	// Convert loadVector into NetworkTickInfo for playerCommands;
	int readIndex = 0;
	PunSerializedData blob(false);

	LOOP_CHECK_START();
	while (readIndex < loadVector.size())
	{
		LOOP_CHECK_END();
		
		int32 tickInfoSize = loadVector[readIndex++];

		blob.Empty();
		blob.Append(&loadVector[readIndex], tickInfoSize);
		readIndex += tickInfoSize;

		NetworkTickInfo tickInfo;
		tickInfo.DeserializeFromBlob(blob);

		PUN_LOG("Loaded %s", *tickInfo.ToString());

		replayPlayers[playerId].playerCommands.push_back(tickInfo);
	}

	replayPlayers[playerId].Init();
	PUN_LOG("Loaded AI player: %d commands:%d", playerId, replayPlayers[playerId].playerCommands.size());
}
