// Pun Dumnernchanvanit's

#pragma once

#include "Engine/GameInstance.h"
#include "Components/AudioComponent.h"
#include "PunCity/GameSaveSystem.h"

#include "Engine.h"
#include "Net/UnrealNetwork.h"
#include "Online.h"
#include "OnlineSessionSettings.h"
#include "Sound/SoundAssets.h"

#include "PunGameInstance.generated.h"

#define SETTING_TEST FName(TEXT("SETTING_TEST"))

#define GAME_SETTINGS_VERSION 11111

/**
 * 
 */
UCLASS()
class PROTOTYPECITY_API UPunGameInstance : public UGameInstance, public IPlayerSoundSettings
{
	GENERATED_BODY()
public:
	void Init() override;

	void PunTick();

	UFUNCTION() virtual void BeginLoadingScreen(const FString& MapName);
	UFUNCTION() virtual void EndLoadingScreen(UWorld* InLoadedWorld);

	UPROPERTY() UAudioComponent* AudioComponent = nullptr;
	UPROPERTY() TSubclassOf<UUserWidget> LoadingScreenClass = nullptr;
	UPROPERTY() UUserWidget* LoadingScreenUI = nullptr;

	void CreateMainMenuSound(USoundBase* sound);
	
	void PlayMenuSound() {
		LLM_SCOPE_(EPunSimLLMTag::PUN_GameInstance);
		
		if (AudioComponent) {
			AudioComponent->Play();
			RefreshSoundSettings();
		}
	}
	void StopMenuSound() {
		LLM_SCOPE_(EPunSimLLMTag::PUN_GameInstance);
		
		if (AudioComponent) {
			AudioComponent->FadeOut(2.0f, 0.0f);
		}
	}

	GameSaveInfo GetSavedGameToLoad() { return _savedGameToLoad; }

	bool IsLoadingSavedGame() { return _savedGameToLoad.IsValid(); }
	
	void SetSavedGameToLoad(GameSaveInfo savedGame)
	{
		_savedGameToLoad = savedGame;
	}

	/*
	 * Map Settings
	 */
	void SetMapSetttings(const TArray<int32>& mapSettingsBlob) {
		LLM_SCOPE_(EPunSimLLMTag::PUN_GameInstance);
		
		int32 index = 0;
		_mapSettings.DeserializeFromBlob(mapSettingsBlob, index);
	}
	void SetMapSettings(FMapSettings mapSettingsIn) {
		_mapSettings = mapSettingsIn;
	}
	
	FMapSettings _mapSettings;

	FMapSettings GetMapSettings() {
		return _mapSettings;
		//std::string mapSeed = ToStdString(mapSettings.mapSeed);
		//return (mapSeed == "seed") ? ("Love Oh") : mapSeed;
	}

	/*
	 * Game End
	 */
	FGameEndStatus endStatus;
	void SetGameEndStatus(FGameEndStatus endStatusIn) {
		endStatus = endStatusIn;
	}

	// Sound
	UPROPERTY() TArray<USoundAssets*> SoundAssetsList; // Need a TArray to prevent GC (TMap doesn't work with GC)
	UPROPERTY() TMap<FString, USoundAssets*> SoundNameToAssets;
	
	UPROPERTY(EditAnywhere) USoundSystemComponent* _soundSystem;

	void TryAddSound(FString soundName, USoundAssets* soundAssets)
	{
		LLM_SCOPE_(EPunSimLLMTag::PUN_GameInstance);
		
		// Add to SoundAssetsList
		if (soundAssets != nullptr) {
			SoundAssetsList.Add(soundAssets);
		}

		// Add to SoundNameToAssets
		if (SoundNameToAssets.Contains(soundName)) {
			SoundNameToAssets[soundName] = soundAssets;
		} else {
			SoundNameToAssets.Add(soundName, soundAssets);
		}
	}
	
	void Spawn2DSound(std::string groupName, std::string soundName);

public:
	// SaveCheck
	GameSaveInfo saveCheckInfo;
	bool saveCheckTransitioning = false;

public:
	// Network Error Check
	void HandleNetworkFailure(UWorld * World, UNetDriver * NetDriver, ENetworkFailure::Type FailureType, const FString & ErrorString) {
		PUN_DEBUG2("NetworkFailure %s, %s", ENetworkFailure::ToString(FailureType), *ErrorString);
		LOG_ERROR(LogNetworkInput, "NetworkFailure %s, %s", ToString(FailureType), *ErrorString);

		mainMenuPopup = ErrorString + " (" + FString(ENetworkFailure::ToString(FailureType)) + ")";
	}
	void OnGameExit();
	void EnsureSessionDestroyed();

	FString mainMenuPopup;

	
	bool isJoiningGame = false;

	// Single player?
	bool isSinglePlayer = false;
	bool isMultiplayer() { return !isSinglePlayer; }
	
public:
	void InitOnline(FName subsystemName);
	
	IOnlineSessionPtr sessionInterface;
	TSharedPtr<class FOnlineSessionSearch> sessionSearch;

	IOnlineSessionPtr sessionInterfaceUsedToFindGame = nullptr;
	FName subsystemNameInUse = "NULL";

	void RefreshSessionInterface(FName subsystemName)
	{
		subsystemNameInUse = subsystemName;
		sessionInterface = nullptr;

		IOnlineSubsystem* onlineSubsystem = IOnlineSubsystem::Get(subsystemName);
		if (onlineSubsystem) {
			sessionInterface = onlineSubsystem->GetSessionInterface();

			// Try refreshing subsystem again if it was invalid
			if (sessionInterface == nullptr || !sessionInterface.IsValid()) {
				InitOnline(subsystemName);
			}
		}
	}
	bool IsSessionInterfaceValid() {
		// Most likely mean this is not connected to Steam...
		return sessionInterface != nullptr && sessionInterface.IsValid();
	}

	void OnCreateSessionComplete(FName sessionName, bool success);
	void OnFindSessionsComplete(bool success);
	void OnDestroySessionComplete(FName sessionName, bool success);
	void OnJoinSessionComplete(FName sessionName, EOnJoinSessionCompleteResult::Type resultType);

	void OnSessionInviteReceived(const FUniqueNetId& UserId, const FUniqueNetId& FromId, const FString& AppId, const FOnlineSessionSearchResult& InviteResult);
	void OnSessionUserInviteAccepted(bool bWasSuccessful, int32 ControllerId, TSharedPtr<const FUniqueNetId> UserId, const FOnlineSessionSearchResult& InviteResult);

	FOnDestroySessionCompleteDelegate DestroySessionBeforeCreatingCompleteDelegate;
	FOnDestroySessionCompleteDelegate DestroySessionThenDoesNothingCompleteDelegate;


	void FindGame();
	void JoinGame(const FOnlineSessionSearchResult& searchResult);
	void DestroyGame();

	void UpdateSession();
	void ServerOnStartedGame();


	/*
	 * Create Game
	 */
	bool bIsCreatingSession = false;
	bool bIsLoadingMultiplayerGame = false;
	
	void CreateSinglePlayerGame() {
		isSinglePlayer = true;
		bIsLoadingMultiplayerGame = false;
		CreateGame_Phase1();
	}
	void CreateMultiplayerGame() {
		isSinglePlayer = false;
		bIsLoadingMultiplayerGame = false;
		CreateGame_Phase1();
	}
	void LoadMultiplayerGame() {
		isSinglePlayer = false;
		bIsLoadingMultiplayerGame = true;
		CreateGame_Phase1();
	}

private:
	void CreateGame_Phase1();
	void CreateGame_Phase2();

public:
	int32 sessionTickCount = 0;
	FOnlineSessionSettings GetSessionSettings()
	{
		FOnlineSessionSettings sessionSettings;

		sessionSettings.bIsLANMatch = (IOnlineSubsystem::Get()->GetSubsystemName() == "NULL");

		const int32 MaxPlayer = 6;
		sessionSettings.NumPublicConnections = MaxPlayer;
		sessionSettings.NumPrivateConnections = 0;
		sessionSettings.bAllowInvites = true;
		sessionSettings.bAllowJoinInProgress = true;
		sessionSettings.bShouldAdvertise = true;
		sessionSettings.bAllowJoinViaPresence = true;
		sessionSettings.bAllowJoinViaPresenceFriendsOnly = false;

		sessionSettings.bUsesPresence = true; // bUsesPresence = true, allows non-public ip server to be seen (like in wireless netwoork)
		sessionSettings.BuildUniqueId = GetBuildUniqueId();

		//FName(TEXT("MAPNAME"))
		sessionSettings.Set(SETTING_MAPNAME, FString("Game Name"), EOnlineDataAdvertisementType::ViaOnlineService);
		sessionSettings.Set(SETTING_TEST, sessionTickCount/120, EOnlineDataAdvertisementType::ViaOnlineService);
		return sessionSettings;
	}

	/*
	 * Player Name
	 */
	// This is needed so that client can show all the players correctly without needing gameMode

	static bool IsServer(const UObject* worldContextObject) {
		return UGameplayStatics::GetGameMode(worldContextObject) != nullptr;
	}

	//static bool
	static bool IsInGame(const UObject* worldContextObject);

private:
	TArray<FString> _playerNames;
	TArray<bool> _playerReadyStates; // ReadyStates in Game Map is used to determined if that player is loaded

	// Cache playerNames for map transition
	TArray<FString> _playerNamesCache;

	//! This is only used by server's main controller...
	//! Count the ticks getting sent out 
	int32 _serverTick = 0;
	
public:
	TArray<bool> playerConnectedStates;
	int32 hostPlayerId = -1;
	
	TArray<int32> clientPacketsReceived;


	// When switching map (Lobby to Game), keep the targetPlayerCount
	// (so we can ensure everyone is connected to the new map before proceeding)
	int32 targetPlayerCount = 0;


	void DebugPunSync(std::string message)
	{
#if DEBUG_BUILD
		std::stringstream ss;
		
		ss << message << ": ";
		for (int32 i = 0; i < _playerNames.Num(); i++) {
			ss << "[" << ToStdString(_playerNames[i]);
			ss << "," << playerConnectedStates[i] << "," << _playerReadyStates[i] << "] ";
		}
		_LOG(PunSync, " %s", ToTChar(ss.str()));
#endif
	}

	
	void SetPlayerCount(int32 count)
	{
		_playerNames.SetNum(count);
		_playerReadyStates.SetNum(count);
		playerConnectedStates.SetNum(count);

		clientPacketsReceived.SetNum(count);
	}

	void ResetPlayerCount()
	{
		_playerNames.SetNum(0);
		_playerReadyStates.SetNum(0);
		playerConnectedStates.SetNum(0);

		clientPacketsReceived.SetNum(0);
	}

	// Player Name
	void SetPlayerNamesF(TArray<FString> playerNamesF) {
		_playerNames = playerNamesF;
	}
	const TArray<FString>& playerNamesF() {
		return _playerNames;
	}
	FString playerNameF(int32 playerId) {
		if (playerId >= GameConstants::MaxPlayers) {
			PUN_CHECK(playerId < GameConstants::MaxPlayersAndAI);
			return GetAITownName(playerId);
		}
		PUN_CHECK(playerId < _playerNames.Num());
		return _playerNames[playerId];
	}
	void SetPlayerNameF(int32 playerId, FString playerNameF) {
		_LOG(PunSync, "SetPlayerNameF %d, %s", playerId, *playerNameF);
		_playerNames[playerId] = playerNameF;
	}

	void CachePlayerNames() {
		_playerNamesCache = _playerNames;
	}
	void UseCachePlayerNames() {
		_playerNames = _playerNamesCache;
	}


	// Player Connected
	bool IsPlayerConnected(int32 playerId) {
		return playerConnectedStates[playerId];
	}

	// Player Ready
	void SetPlayerReadyStates(TArray<bool> playerReadyStatesIn) {
		_playerReadyStates = playerReadyStatesIn;
	}
	const TArray<bool>& playerReadyStates() {
		return _playerReadyStates;
	}
	bool IsPlayerReady(int32 playerId) {
		return _playerReadyStates[playerId];
	}
	void SetPlayerReady(int32 playerId, bool isReady) 	{
		_playerReadyStates[playerId] = isReady;
	}
	void UnreadyAll() {
		for (int32 i = 0; i < _playerReadyStates.Num(); i++) {
			_playerReadyStates[i] = false;
		}
	}
	int32 playerReadyCount() {
		int32 readyCount = 0;
		for (int32 i = 0; i < _playerReadyStates.Num(); i++) {
			if (_playerReadyStates[i]) {
				readyCount++;
			}
		}
		return readyCount;
	}
	

	bool IsAllPlayersReady() {
		//PUN_DEBUG2("IsAllPlayersReady size:%d", playerReadyStates.Num());
		for (int32 i = 0; i < _playerReadyStates.Num(); i++) {
			if (IsPlayerConnected(i) && !_playerReadyStates[i])
			{
				//PUN_DEBUG2("Player not ready:%d", playerReadyStates[i]);
				return false;
			}
		}
		return true;
	}


	/*
	 * Connect/Disconnect
	 */
	 // Returns playerId
	int32 ConnectPlayer(FString playerNameF)
	{
		auto setToExistingSlot = [&](int32 playerId)
		{
			_playerNames[playerId] = playerNameF;
			_playerReadyStates[playerId] = false;
			playerConnectedStates[playerId] = true;
			clientPacketsReceived[playerId] = 0;
		};
		
		// If we are loading a save, try to put the player in the old spot
		//GameSaveInfo saveInfo = GetSavedGameToLoad();
		if (saveSystem().HasSyncData())
		{
			GameSaveInfo saveInfo = saveSystem().GetSyncSaveInfo();
			SetPlayerCount(saveInfo.playerNames.Num());
			
			for (int32 i = 0; i < saveInfo.playerNames.Num(); i++) {
				if (!IsPlayerConnected(i) && playerNameF == saveInfo.playerNames[i]) 
				{
					// If this player was host, change hostPlayerId
					if (playerNameF == _playerNames[hostPlayerId]) {
						hostPlayerId = i;
					}

					setToExistingSlot(i);

					_LOG(PunSync, "ConnectPlayer Old Spot: %s size:%d", *playerNameF, playerNamesF().Num());
					return i;
				}
			}
		}

		// Try Add to the slot with same name first
		for (int32 i = 0; i < _playerNames.Num(); i++) {
			if (!IsPlayerConnected(i) && 
				playerNameF == _playerNames[i])
			{
				setToExistingSlot(i);

				_LOG(PunSync, "ConnectPlayer (Empty): %s size:%d", *playerNameF, playerNamesF().Num());
				return i;
			}
		}
		
		// Add to empty slot
		for (int32 i = 0; i < _playerNames.Num(); i++) {
			if (!IsPlayerConnected(i))
			{
				setToExistingSlot(i);

				_LOG(PunSync, "ConnectPlayer (Empty): %s size:%d", *playerNameF, playerNamesF().Num());
				return i;
			}
		}

		// Make new slot
		_playerNames.Add(playerNameF);
		_playerReadyStates.Add(false);
		playerConnectedStates.Add(true);
		clientPacketsReceived.Add(0);

		_LOG(PunSync, "ConnectPlayer (New slot): %s size:%d", *playerNameF, playerNamesF().Num());

		return _playerNames.Num() - 1;
	}

	bool TryChangePlayerId(int32 originalId, int32 newId)
	{
		// Don't take the slot if someone already took it
		if (playerConnectedStates[newId]) {
			return false;
		}

		_LOG(PunSync, "TryChangePlayerId %d, %d", originalId, newId);
		
		_playerNames[newId] = _playerNames[originalId];
		_playerReadyStates[newId] = _playerReadyStates[originalId];
		playerConnectedStates[newId] = playerConnectedStates[originalId];

		clientPacketsReceived[newId] = clientPacketsReceived[originalId];
		
		
		_playerNames[originalId] = "";
		_playerReadyStates[originalId] = false;
		playerConnectedStates[originalId] = false;

		clientPacketsReceived[originalId] = 0;
		
		return true;
	}


	void DisconnectPlayer(FString playerNameF)
	{
		for (int32 i = 0; i < _playerNames.Num(); i++) {
			if (_playerNames[i] == playerNameF) 
			{
				_playerNames[i] = FString();
				_playerReadyStates[i] = false;
				playerConnectedStates[i] = false;

				clientPacketsReceived[i] = 0;

				_LOG(PunSync, "DisconnectPlayer %d, %s", i, *playerNameF);
				break;
			}
		}
	}

	/*
	 * Server Tick
	 */
	void ResetServerTick() {
		_serverTick = 0 ;
	}
	void IncrementServerTick() {
		_serverTick++;
	}
	int32 serverTick() { return _serverTick; }
	void SetServerTick(int32 serverTick) {
		_serverTick = serverTick;
	}

	/*
	 * Easy to use functions
	 */

	int32 playerCount()
	{
		if (isSinglePlayer) {
			return 1;
		}
		
		// Count playerNames skipping empty slot
		int32 count = 0;
		for (int32 i = 0; i < _playerNames.Num(); i++) {
			if (playerConnectedStates[i]) {
				count++;
			}
		}
		return count;
	}

	// Both connected and disconnected players
	std::vector<int32> allHumanPlayerIds()
	{
		if (isSinglePlayer) {
			return { 0 };
		}

		// Add players skipping any empty slot (that is empty from the beginning)
		std::vector<int32> results;
		for (int32 i = 0; i < _playerNames.Num(); i++) {
			if (!_playerNames[i].IsEmpty()) {
				results.push_back(i);
			}
		}
		return results;
	}

	std::vector<int32> connectedPlayerIds()
	{
		if (isSinglePlayer) {
			return { 0 };
		}

		// Add players skipping disconnected
		std::vector<int32> results;
		for (int32 i = 0; i < _playerNames.Num(); i++) {
			if (playerConnectedStates[i]) {
				results.push_back(i);
			}
		}
		return results;
	}

	std::vector<int32> disconnectedPlayerIds()
	{
		if (isSinglePlayer) {
			return {};
		}

		// Add players with name but no connection
		std::vector<int32> results;
		for (int32 i = 0; i < _playerNames.Num(); i++) {
			if (!_playerNames[i].IsEmpty() && !playerConnectedStates[i]) {
				results.push_back(i);
			}
		}
		return results;
	}
	
	/*
	 * Chat
	 */
	bool lobbyChatDirty = true;
	TArray<FString> lobbyChatPlayerNames;
	TArray<FString> lobbyChatMessages;

	/*
	 * Others
	 */
	bool isReturningToLobbyList = false;

	GameSaveSystem& saveSystem() { return *_saveSystem; }
	void SetSaveSystemController(IPunPlayerController* controller) {
		_saveSystem->Init(controller);
	}

private:
	GameSaveInfo _savedGameToLoad;
	
	std::unique_ptr<GameSaveSystem> _saveSystem;

	
	
public:
	/*
	 * IPlayerSoundSettings
	 */
	float masterVolume() override { return _masterVolume;  }
	float musicVolume() override { return _musicVolume; }
	float ambientVolume() override { return _ambientVolume; }
	float soundEffectsVolume() override { return _soundEffectsVolume; }

	void SetMasterVolume(float masterVolume) { _masterVolume = masterVolume; }
	void SetMusicVolume(float musicVolume) { _musicVolume = musicVolume; }
	void SetAmbientVolume(float ambientVolume) { _ambientVolume = ambientVolume; }
	void SetSoundEffectsVolume(float soundEffectsVolume) { _soundEffectsVolume = soundEffectsVolume; }

public:
	/*
	 * Settings
	 */
	bool needSettingsUpdate = false;

	void RestoreDefaults()
	{
		LLM_SCOPE_(EPunSimLLMTag::PUN_GameInstance);
		
		_masterVolume = 1.0f;
		_musicVolume = 1.0f;
		_soundEffectsVolume = 1.0f;
		_ambientVolume = 1.0f;

		mouseZoomSpeedFraction = 0.5f;
		_resolutionQuality = 70.0f;
		
		RefreshSoundSettings();
	}

	void SerializeSoundAndOtherSettings(FArchive& Ar)
	{
		LLM_SCOPE_(EPunSimLLMTag::PUN_GameInstance);

		if (Ar.IsSaving()) {
			loadedVersion = GAME_SETTINGS_VERSION;
		}
		Ar << loadedVersion;

		if (Ar.IsLoading() &&
			loadedVersion != GAME_SETTINGS_VERSION) {
			return;
		}
		
		Ar << _masterVolume;
		Ar << _musicVolume;
		Ar << _soundEffectsVolume;
		Ar << _ambientVolume;

		Ar << mouseZoomSpeedFraction;

		Ar << _resolutionQuality;
	}

	void RefreshSoundSettings()
	{
		LLM_SCOPE_(EPunSimLLMTag::PUN_GameInstance);
		
		// TODO: remove this???
		needSettingsUpdate = true;
		if (AudioComponent) {
			AudioComponent->SetVolumeMultiplier(_masterVolume * _musicVolume);
		}
	}

	float resolutionQuality() { return _resolutionQuality; }
	void SetResolutionQuality(float value) {
		_resolutionQuality = value;
		PUN_LOG("_resolutionQuality %f %f", _resolutionQuality, value);
	}

	int32 loadedVersion = -1;
	float mouseZoomSpeedFraction;

private:
	float _resolutionQuality;
	
	float _masterVolume;
	float _musicVolume;
	float _soundEffectsVolume;
	float _ambientVolume;


public:
	/*
	 * Save to file
	 */
	// Right now used for non-game settings
	void SaveSettingsToFile()
	{
		LLM_SCOPE_(EPunSimLLMTag::PUN_GameInstance);
		
		FBufferArchive SaveArchive;
		SaveArchive.SetIsSaving(true);
		SaveArchive.SetIsLoading(false);

		SerializeSoundAndOtherSettings(SaveArchive);

		FString path = GetSettingsSavePath();
		bool succeed = FFileHelper::SaveArrayToFile(SaveArchive, *path);
		SaveArchive.FlushCache();
		SaveArchive.Empty();

		if (!succeed) {
			PUN_LOG("SaveSettingsToFile path:%s", *path);
		}
	}

	bool LoadSoundAndOtherSettingsFromFile()
	{
		LLM_SCOPE_(EPunSimLLMTag::PUN_GameInstance);
		
		FString path = GetSettingsSavePath();

		TArray<uint8> binary;
		if (!FFileHelper::LoadFileToArray(binary, *path)) {
			PUN_LOG("Invalid MetaData file %s", *path);
			return false;
		}
		PUN_CHECK(binary.Num() > 0);

		// TODO: may be only one LoadArchive???
		FMemoryReader LoadArchive(binary, true);
		LoadArchive.Seek(0);
		LoadArchive.SetIsSaving(false);
		LoadArchive.SetIsLoading(true);

		SerializeSoundAndOtherSettings(LoadArchive);

		LoadArchive.FlushCache();
		binary.Empty();
		LoadArchive.Close();

		return true;
	}

	FString GetSettingsSavePath() {
		return FPaths::ConvertRelativePathToFull(FPaths::ProjectSavedDir()) + "/SettingsSaves/Settings.dat";
	}
};
