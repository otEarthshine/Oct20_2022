// Pun Dumnernchanvanit's


#include "PunCity/GameSaveSystem.h"
#include "PunCity/PunGameInstance.h"

static const bool compressData = true;

GameSaveInfo GameSaveSystem::SaveDataToFile(FString saveName, bool isCachingForSync)
{
	auto dataSource = _controller->dataSource();
	auto inputSystem = _controller->inputSystemInterface();
	auto& simulation = dataSource->simulation();
	int32 playerId = dataSource->playerId();

	GameSaveInfo saveInfo;
	saveInfo.version = SAVE_VERSION;
	saveInfo.name = saveName;
	saveInfo.dateTime = FDateTime::Now();
	saveInfo.gameTicks = Time::Ticks();
	saveInfo.population = simulation.population(playerId);
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
	 * Save GameData
	 */
	bool succeed;
	{
		FBufferArchive SaveArchive;
		SaveArchive.SetIsSaving(true);
		SaveArchive.SetIsLoading(false);

		inputSystem->Serialize(SaveArchive);
		simulation.Serialize(SaveArchive, SaveArchive);
		PUN_CHECK(SaveArchive.Num() > 0);

		saveInfo.checksum = FCrc::MemCrc32(SaveArchive.GetData(), SaveArchive.Num());
		_LOG(PunSaveLoad, "Save GameData Uncompressed size:%d checksum:%d", SaveArchive.Num(), saveInfo.checksum);

		FString gameDataPath = folderPath + "/GameData.dat";
		if (compressData)
		{
			TArray<uint8> CompressedData;
			FArchiveSaveCompressedProxy Compressor = FArchiveSaveCompressedProxy(CompressedData, NAME_Zlib, ECompressionFlags::COMPRESS_BiasSpeed);
			{
				SCOPE_TIMER("Save Compress");

				Compressor << SaveArchive;
				Compressor.Flush();
				_LOG(PunSaveLoad, "Save GameData Compressed size:%d", CompressedData.Num());
			}
			{
				SCOPE_TIMER("Save to file");
				succeed = FFileHelper::SaveArrayToFile(CompressedData, *gameDataPath);
				saveInfo.compressedDataSize = CompressedData.Num();
			}

			Compressor.FlushCache();
			CompressedData.Empty();
		}
		else
		{
			succeed = FFileHelper::SaveArrayToFile(SaveArchive, *gameDataPath);
		}

		SaveArchive.FlushCache();
		SaveArchive.Empty();
		SaveArchive.Close();


		if (!succeed) {
			_LOG(PunSaveLoad, "Save Gamedata failed gameDataPath:%s", *gameDataPath);
			return GameSaveInfo();
		}
	}

	/*
	 * Save MetaData
	 */
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
	FString gameDataPath = _syncSaveInfo.folderPath + "/GameData.dat";
	
	_syncCompressedData.Empty();

	if (!FFileHelper::LoadFileToArray(_syncCompressedData, *gameDataPath)) {
		_LOG(PunSaveLoad, "Invalid GameData file %s", *gameDataPath);
		return GameSaveInfo();
	}
	_LOG(PunSaveLoad, "Loaded GameData Compressed %d", _syncCompressedData.Num());
	PUN_CHECK(_syncCompressedData.Num() > 0);
	
	return _syncSaveInfo;
}

void GameSaveSystem::LoadDataIntoSimulation()
{
	FBufferArchive gameDataBinary;
	
	// Decompress File 
	FArchiveLoadCompressedProxy Decompressor = FArchiveLoadCompressedProxy(_syncCompressedData, NAME_Zlib, ECompressionFlags::COMPRESS_BiasSpeed);
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

	inputSystem->Serialize(LoadArchive);
	simulation.Serialize(LoadArchive, gameDataBinary);

	int32 checksum = FCrc::MemCrc32(gameDataBinary.GetData(), gameDataBinary.Num());
	PUN_CHECK(checksum == _syncSaveInfo.checksum);
	LOG_ERROR(PunSaveLoad, "Loaded GameData Uncompressed size:%d checksum:%d", gameDataBinary.Num(), checksum);

	LoadArchive.FlushCache();
	LoadArchive.Close();
	gameDataBinary.Empty();
	gameDataBinary.Close();
}