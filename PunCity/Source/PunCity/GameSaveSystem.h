// Pun Dumnernchanvanit's

#pragma once

#include "CoreMinimal.h"
#include "PunCity/Simulation/GameSimulationCore.h"
#include "PunCity/UI/GameUIDataSource.h"
#include "PunCity/GameNetworkInterface.h"
#include "PunCity/UI/UISystemBase.h"
#include <iomanip>
#include "Serialization/ArchiveSaveCompressedProxy.h"
#include "Serialization/ArchiveLoadCompressedProxy.h"
#include "HAL/FileManager.h"
#include "Misc/FileHelper.h"

//static FBufferArchive GetSaveArchive()
//{
//	FBufferArchive saveArchive;
//	saveArchive.SetIsSaving(true);
//	saveArchive.SetIsLoading(false);
//	return saveArchive;
//}
//static FMemoryReader GetLoadArchive(TArray<uint8>& binary)
//{
//	FMemoryReader loadArchive(binary, true);
//	loadArchive.Seek(0);
//	loadArchive.SetIsSaving(false);
//	loadArchive.SetIsLoading(true);
//	return loadArchive;
//}

struct GameSaveInfo
{
	int32 version = -1;
	
	FString name;
	FDateTime dateTime;
	int32 gameTicks = 0;
	int32 population = 0;
	FMapSettings mapSettings;
	TArray<FString> playerNames;

	FString folderPath;
	int32 checksum = 0;
	
	int32 compressedDataSize = 0;

	bool IsValid() { return name.Len() > 0; }

	FString DefaultSaveName()
	{
		FString date = FString::Printf(TEXT("%02d/%02d"), dateTime.GetMonth(), dateTime.GetDay());
		FString time = FString::Printf(TEXT("%02d:%02d"), dateTime.GetHour(), dateTime.GetMinute());
		
		//std::stringstream ss;
		//ss << ToStdString(name);
		//ss << " " << dateTime.GetMonth() << "/" << dateTime.GetDay();
		//ss << std::setw(2) << std::setfill('0') << ", " << dateTime.GetHour() << ":" << dateTime.GetMinute() << ":" << dateTime.GetSecond();
		return name + " " + date + ", " + time;
	}

	FString GetLastPlayerName(int32 playerId) {
		if (playerId >= playerNames.Num()) {
			return "";
		}
		return playerNames[playerId];
	}

	void Serialize(FArchive& Ar)
	{
		Ar << version;

		// If this is different save version, don't load it
		if (version != SAVE_VERSION) {
			return;
		}

		SerializeTArrayLoop(Ar, playerNames, [&](FString& fstring) {
			Ar << fstring;
		});

		Ar << name;
		Ar << dateTime;
		Ar << gameTicks;
		Ar << population;
		mapSettings.Serialize(Ar);

		Ar << folderPath;
		Ar << checksum;

		Ar << compressedDataSize;
	}
};

/**
 * 
 */
class GameSaveSystem
{
public:
	void Init(IPunPlayerController* controller) {
		_controller = controller;
	}

	const TArray<GameSaveInfo>& saveList() { return _saveList; }


	void RefreshSaveList() {
		_saveList = RefreshSaveListStatic();
	}
	
	static TArray<GameSaveInfo> RefreshSaveListStatic()
	{
		// Obtain only meta data
		TArray<FString> foundDirectories;

		FString saveDirectoryWithWildCard = GetSaveDirectoryPath() + "/*";
		IFileManager::Get().FindFiles(foundDirectories, *saveDirectoryWithWildCard, false, true);

		TArray<GameSaveInfo> saveList;
		for (const FString& directoryName : foundDirectories)
		{
			FString folderPath = GetSaveDirectoryPath() + "/" + directoryName;
			GameSaveInfo saveInfo = LoadMetadata(folderPath);
			saveList.Add(saveInfo);
		}

		return saveList;
	}

	

	bool HasExistingSave(FString saveName)
	{
		FString folderPath = GetSaveDirectoryPath();
		folderPath += FormatStringForPath(saveName);

		return IFileManager::Get().DirectoryExists(*folderPath);
	}

	GameSaveInfo SaveDataToFile(FString saveName, bool isCachingForSync = false);

	
	/*
	 * Delete
	 */
	static void DeleteSave(const FString& folderPath) {
		IFileManager::Get().DeleteDirectory(*folderPath, false, true);
		RefreshSaveListStatic();
	}

	/*
	 * Load
	 */
	static GameSaveInfo LoadMetadata(const FString& folderPath);

	GameSaveInfo LoadDataIntoCache();

	void LoadDataIntoSimulation();
	
	// Note: with metaDataOnly, controller can be nullptr
	GameSaveInfo Load(const FString& folderPath)
	{
		if (!HasSyncData()) {
			_syncSaveInfo = LoadMetadata(folderPath);
			LoadDataIntoCache();
		}

		GameSaveInfo saveInfo = _syncSaveInfo;
		
		LoadDataIntoSimulation();
		ClearSyncData();
		
		return saveInfo;
	}

	
	/*
	 * Sync data to be given to clients
	 */
	bool HasSyncData() { return _syncSaveInfo.IsValid(); }
	
	bool IsSyncDataReady() {
		return _syncSaveInfo.IsValid() && _receivedPacketCount == _receivedData.Num();
	}
	int32 GetSyncDataLoadPercent() {
		return _syncCompressedData.Num() * 100 / _syncSaveInfo.compressedDataSize;
	}
	void ClearSyncData() {
		_lastSyncSaveInfo = _syncSaveInfo;
		_syncSaveInfo = GameSaveInfo();
		_syncCompressedData.Empty();
	}
	
	const GameSaveInfo& GetSyncSaveInfo() { return _syncSaveInfo; }
	const TArray<uint8>& GetSyncCompressedData() { return _syncCompressedData; }

	const GameSaveInfo& GetLastSyncSaveInfo() { return _lastSyncSaveInfo; }

	// Note: Called when loading MP Save (including on server)
	void SetSyncSaveInfo(GameSaveInfo saveInfo) {
		_syncSaveInfo = saveInfo;
		_syncCompressedData.SetNum(_syncSaveInfo.compressedDataSize);
		_receivedData.SetNum((_syncSaveInfo.compressedDataSize - 1) / MaxPacketSize + 1);
		_receivedPacketCount = 0;
		_receivedPacketIterator = 0;
	}
	void ServerPrepareSync() {
		_receivedPacketCount = _receivedData.Num();
	}

	int32 totalPackets() { return _receivedData.Num(); }
	int32 lastPacketSize() {
		int32 remainder = _syncSaveInfo.compressedDataSize % MaxPacketSize;
		return remainder == 0 ? MaxPacketSize : remainder;
	}
	int32 receivedPacketCount() { return _receivedPacketCount; }

	TArray<int32> GetSyncPacketIndices(int32 targetPacketCount)
	{
		TArray<int32> results;
		for (int32 i = 0; i < _receivedData.Num(); i++) 
		{
			int32 packetIndex = (i + _receivedPacketIterator) % _receivedData.Num();
			if (!_receivedData[packetIndex]) {
				results.Add(packetIndex);
				if (results.Num() >= targetPacketCount) {
					//PUN_DEBUG2("GetSync:%d", _receivedPacketIterator);
					_receivedPacketIterator = packetIndex;
					return results;
				}
			}
		}
		return results;
	}
	void ReceivePacket(int32 packetIndex, const TArray<uint8>& data)
	{
		if (!_receivedData[packetIndex]) {
			for (int32 i = 0; i < data.Num(); i++) {
				_syncCompressedData[packetIndex * MaxPacketSize + i] = data[i];
			}
			_receivedData[packetIndex] = true;
			_receivedPacketCount++;
		}
	}
	

private:
	static FString GetSaveDirectoryPath() {
		return FPaths::ConvertRelativePathToFull(FPaths::ProjectSavedDir()) + "/GameSaves/";
	}

	static FString FormatStringForPath(FString& strToFormat) {
		return strToFormat.Replace(TEXT(" "), TEXT(""))
			.Replace(TEXT("/"), TEXT(""))
			.Replace(TEXT(","), TEXT(""))
			.Replace(TEXT(":"), TEXT(""));
	}
	

private:
	IPunPlayerController* _controller = nullptr;

	TArray<GameSaveInfo> _saveList;

	// Data Sync
	GameSaveInfo _syncSaveInfo;
	TArray<uint8> _syncCompressedData;

	GameSaveInfo _lastSyncSaveInfo;

	TArray<bool> _receivedData;
	int32 _receivedPacketCount = 0;
	int32 _receivedPacketIterator = 0;
};
