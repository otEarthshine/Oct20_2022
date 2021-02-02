// Pun Dumnernchanvanit's


#include "PunCity/GameSaveSystem.h"
#include "PunCity/PunGameInstance.h"

static const bool compressData = true;


GameSaveInfo GameSaveSystem::SaveDataToFile(FString saveName, bool isCachingForSync)
{
	auto dataSource = _controller->dataSource();
	auto& simulation = dataSource->simulation();
	int32 playerId = dataSource->playerId();

	GameSaveInfo saveInfo;
	saveInfo.version = SAVE_VERSION;
	saveInfo.name = saveName;
	saveInfo.dateTime = FDateTime::Now();
	saveInfo.gameTicks = Time::Ticks();
	saveInfo.population = simulation.populationTown(playerId);
	saveInfo.mapSettings = dataSource->GetMapSettings();

	saveInfo.playerNames = _controller->networkInterface()->playerNamesF();

	// Remove any connected player with Old in front of the name (meaning it is marked old by game)
	for (int32 i = 0; i < saveInfo.playerNames.Num(); i++) {
		if (!_controller->networkInterface()->IsPlayerConnected(i) &&
			saveInfo.playerNames[i].Left(4) == "Old ")
		{
			saveInfo.playerNames[i] = saveInfo.playerNames[i].RightChop(4);
		}
	}

	FString folderPath = GetSaveDirectoryPath();
	folderPath += FormatStringForPath(saveName);

	saveInfo.folderPath = folderPath;

	/*
	 * Multithreaded Save
	 */
	{
		SCOPE_TIMER("Multithreaded Save Serialize");
		
		IPunPlayerController* controller = _controller;

		int32 chunkCount = static_cast<int>(GameSaveChunkEnum::Count);
		_saveCompleteFutures.SetNum(chunkCount);
		for (int32 i = 0; i < chunkCount; i++) {
			_saveCompleteFutures[i].Reset();
			_saveCompleteFutures[i] = Async(EAsyncExecution::Thread, [isCachingForSync, folderPath, i, controller]() {
				return SaveDataToFile_ThreadHelper(isCachingForSync, folderPath, static_cast<GameSaveChunkEnum>(i), controller);
			});
		}

		// Pause while all the thread works
		while (true) {
			bool completed = true;
			for (int32 i = 0; i < chunkCount; i++) {
				if (!_saveCompleteFutures[i].IsReady()) {
					completed = false;
					break;
				}
			}
			if (completed) {
				break;
			}
		}

		// Record success
		saveInfo.checksum.SetNum(chunkCount);
		saveInfo.compressedDataSize.SetNum(chunkCount);
		saveInfo.success.SetNum(chunkCount);

		for (int32 i = 0; i < chunkCount; i++) {
			saveInfo.checksum[i] = _saveCompleteFutures[i].Get().checksum;
			saveInfo.compressedDataSize[i] = _saveCompleteFutures[i].Get().compressedDataSize;
			saveInfo.success[i] = _saveCompleteFutures[i].Get().succeed;
		}
	}

	/*
	 * Save MetaData
	 */
	bool succeed;
	{
		FBufferArchive SaveArchive;
		SaveArchive.SetIsSaving(true);
		SaveArchive.SetIsLoading(false);

		//! Meta data
		saveInfo.Serialize(SaveArchive);

		FString metaDataPath = folderPath + "/MetaData.dat";
		succeed = FFileHelper::SaveArrayToFile(SaveArchive, *metaDataPath);
		SaveArchive.FlushCache();
		SaveArchive.Empty();

		if (!succeed) {
			_LOG(PunSaveLoad, "Save Metadata failed metaDataPath:%s", *metaDataPath);
			return GameSaveInfo();
		}
	}

	RefreshSaveList();

	_LOG(PunSaveLoad, "Save success:%d folderPath:%s", succeed, *folderPath);
	return saveInfo;
}

FSaveThreadResults GameSaveSystem::SaveDataToFile_ThreadHelper(bool isCachingForSync, FString folderPath, GameSaveChunkEnum saveChunkEnum, IPunPlayerController* controller)
{
	SCOPE_TIMER_("!!! Save Thread %d", static_cast<int>(saveChunkEnum));
	
	auto dataSource = controller->dataSource();
	auto inputSystem = controller->inputSystemInterface();
	auto& simulation = dataSource->simulation();

	FSaveThreadResults results;

	/*
	 * Save GameData
	 */
	{
		FBufferArchive SaveArchive;
		SaveArchive.SetIsSaving(true);
		SaveArchive.SetIsLoading(false);

		{
			SCOPE_TIMER_("Save Serialize %d", static_cast<int>(saveChunkEnum));
			if (static_cast<int>(saveChunkEnum) == 0) {
				inputSystem->Serialize(SaveArchive);
			}

			simulation.Serialize(SaveArchive, SaveArchive, saveChunkEnum);
			PUN_CHECK(SaveArchive.Num() > 0);
		}

		{
			SCOPE_TIMER_("Save Checksum %d", static_cast<int>(saveChunkEnum));
			results.checksum = FCrc::MemCrc32(SaveArchive.GetData(), SaveArchive.Num());
		}
		
		_LOG(PunSaveLoad, "Save GameData Uncompressed size:%d checksum:%d", SaveArchive.Num(), results.checksum);

		FString gameDataPath = folderPath + "/GameData" + FString::FromInt(static_cast<int>(saveChunkEnum)) + ".dat";
		if (compressData)
		{
			TArray<uint8> CompressedData;
			FArchiveSaveCompressedProxy Compressor = FArchiveSaveCompressedProxy(CompressedData, NAME_Zlib, ECompressionFlags::COMPRESS_BiasSpeed);
			{
				SCOPE_TIMER_("Save Compress %d", static_cast<int>(saveChunkEnum));

				Compressor << SaveArchive;
				Compressor.Flush();
				_LOG(PunSaveLoad, "Save GameData Compressed size:%d", CompressedData.Num());
			}
			{
				SCOPE_TIMER_("Save to file %d", static_cast<int>(saveChunkEnum));
				results.succeed = FFileHelper::SaveArrayToFile(CompressedData, *gameDataPath);
				results.compressedDataSize = CompressedData.Num();
			}

			Compressor.FlushCache();
			CompressedData.Empty();
		}
		else
		{
			results.succeed = FFileHelper::SaveArrayToFile(SaveArchive, *gameDataPath);
		}

		SaveArchive.FlushCache();
		SaveArchive.Empty();
		SaveArchive.Close();


		if (!results.succeed) {
			_LOG(PunSaveLoad, "Save Gamedata failed gameDataPath:%s", *gameDataPath);
			return results;
		}
	}

	return results;
}

GameSaveInfo GameSaveSystem::LoadMetadata(const FString& folderPath)
{
	GameSaveInfo saveInfo;

	FString metaDataPath = folderPath + "/MetaData.dat";

	TArray<uint8> metaDataBinary;
	{
		SCOPE_TIMER("Load from file");
		if (!FFileHelper::LoadFileToArray(metaDataBinary, *metaDataPath)) {
			LOG_ERROR(PunSaveLoad, "Invalid MetaData file %s", *metaDataPath);
			return GameSaveInfo();
		}
	}
	PUN_CHECK(metaDataBinary.Num() > 0);

	// TODO: may be only one LoadArchive???
	FMemoryReader LoadArchive(metaDataBinary, true);
	LoadArchive.Seek(0);
	LoadArchive.SetIsSaving(false);
	LoadArchive.SetIsLoading(true);

	saveInfo.folderPath = folderPath;
	saveInfo.Serialize(LoadArchive);

	LoadArchive.FlushCache();
	metaDataBinary.Empty();
	LoadArchive.Close();

	return saveInfo;
}

GameSaveInfo GameSaveSystem::LoadDataIntoCache()
{
	_syncCompressedData.Empty();
	
	TArray<uint8> compressedDataChunk;

	for (int32 i = 0; i < static_cast<int>(GameSaveChunkEnum::Count); i++)
	{
		FString gameDataPath = _syncSaveInfo.folderPath + "/GameData" + FString::FromInt(i) + ".dat";

		compressedDataChunk.Empty();
		if (!FFileHelper::LoadFileToArray(compressedDataChunk, *gameDataPath)) {
			_LOG(PunSaveLoad, "Invalid GameData file %s", *gameDataPath);
			return GameSaveInfo();
		}
		_syncCompressedData.Append(compressedDataChunk);
	}
	
	_LOG(PunSaveLoad, "Loaded GameData Compressed %d", _syncCompressedData.Num());
	PUN_CHECK(_syncCompressedData.Num() > 0);
	PUN_CHECK(_syncCompressedData.Num() == _syncSaveInfo.totalCompressedDataSize());
	
	return _syncSaveInfo;
}

void GameSaveSystem::LoadDataIntoSimulation()
{
	TArray<uint8> compressedDataChunk;

	int32 sizeSoFar = 0;
	
	for (int32 i = 0; i < static_cast<int>(GameSaveChunkEnum::Count); i++)
	{
		compressedDataChunk.Empty();

		int32 chunkDataSize = _syncSaveInfo.compressedDataSize[i];
		int32 end = sizeSoFar + chunkDataSize;
		compressedDataChunk.Reserve(chunkDataSize);
		for (int32 j = sizeSoFar; j < end; j++) {
			compressedDataChunk.Add(_syncCompressedData[j]);
		}
		sizeSoFar += chunkDataSize;
		
		FBufferArchive gameDataBinary;

		// Decompress File 
		FArchiveLoadCompressedProxy Decompressor = FArchiveLoadCompressedProxy(compressedDataChunk, NAME_Zlib, ECompressionFlags::COMPRESS_BiasSpeed);
		if (Decompressor.GetError()) {
			LOG_ERROR(PunSaveLoad, "File Was Not Compressed ");
			return;
		}

		//Decompress
		Decompressor << gameDataBinary;
		Decompressor.FlushCache();


		PUN_CHECK(gameDataBinary.Num() > 0);

		/*
		 *
		 */
		FMemoryReader LoadArchive(gameDataBinary, true);
		LoadArchive.Seek(0);
		LoadArchive.SetIsSaving(false);
		LoadArchive.SetIsLoading(true);

		auto dataSource = _controller->dataSource();
		auto inputSystem = _controller->inputSystemInterface();
		auto& simulation = dataSource->simulation();

		if (i == 0) {
			inputSystem->Serialize(LoadArchive);
		}
		simulation.Serialize(LoadArchive, gameDataBinary, static_cast<GameSaveChunkEnum>(i));

		int32 checksum = FCrc::MemCrc32(gameDataBinary.GetData(), gameDataBinary.Num());
		PUN_CHECK(checksum == _syncSaveInfo.checksum[i]);
		LOG_ERROR(PunSaveLoad, "Loaded GameData Uncompressed size:%d checksum:%d", gameDataBinary.Num(), checksum);

		LoadArchive.FlushCache();
		LoadArchive.Close();
		gameDataBinary.Empty();
		gameDataBinary.Close();

	}

	PUN_CHECK(sizeSoFar == _syncSaveInfo.totalCompressedDataSize());
}