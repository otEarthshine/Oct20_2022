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

#include <sstream>
#include "Kismet/KismetInternationalizationLibrary.h"
#include "PunFileUtils.h"

#include "PunGameInstance.generated.h"

//#define SESSION_HOSTNAME FName(TEXT("HOST_NAME"))
#define SESSION_TICK FName(TEXT("SETTING_TEST"))
#define SESSION_GAME_VERSION FName(TEXT("GAME_VERSION"))
#define SESSION_NUM_PLAYERS FName(TEXT("NUM_PLAYERS"))
#define SESSION_PASSWORD FName(TEXT("PASSWORD"))

static const FName PUN_SESSION_NAME = TEXT("Pun Session Game");

static int32 GetSessionValue(FName key, const FOnlineSessionSettings& sessionSettings) {
	int32 value = 0;
	sessionSettings.Get(key, value);
	return value;
}

static FString GetSessionValueString(FName key, const FOnlineSessionSettings& sessionSettings) {
	FString value;
	sessionSettings.Get(key, value);
	return value;
}

static void PrintSessionSettings(TArray<FText>& args, const FOnlineSessionSettings& sessionSettings)
{
	ADDTEXT_(INVTEXT("Should Advertise: {0}\n"), TEXT_NUM(sessionSettings.bShouldAdvertise));
	ADDTEXT_(INVTEXT("Is LAN Match: {0}\n"), TEXT_NUM(sessionSettings.bIsLANMatch));
	ADDTEXT_(INVTEXT("Allow Join: {0}\n"), TEXT_NUM(sessionSettings.bAllowJoinInProgress));
	//ss << "IsDedicated:" << sessionSettings.bIsDedicated << "\n";
	//ss << "UsesStats:" << sessionSettings.bUsesStats << "\n";
	ADDTEXT_(INVTEXT("Allow Invites: {0}\n"), TEXT_NUM(sessionSettings.bAllowInvites));
	ADDTEXT_(INVTEXT("Uses Presence: {0}\n"), TEXT_NUM(sessionSettings.bUsesPresence));
	ADDTEXT_(INVTEXT("Allow Join Via Presence: {0}\n"), TEXT_NUM(sessionSettings.bAllowJoinViaPresence));
	ADDTEXT_(INVTEXT("Allow Join Via Presence Friends-Only: {0}\n"), TEXT_NUM(sessionSettings.bAllowJoinViaPresenceFriendsOnly));
	//ss << "AntiCheatProtected:" << sessionSettings.bAntiCheatProtected << "\n";
	ADDTEXT_(INVTEXT("Build Unique Id: {0}\n"), TEXT_NUM(sessionSettings.BuildUniqueId));
	ADDTEXT_INV_("\n");
	
	ADDTEXT_(INVTEXT("Session Tick: {0}\n"), TEXT_NUM(GetSessionValue(SESSION_TICK, sessionSettings)));
	ADDTEXT_(INVTEXT("Player Count: {0}\n"), TEXT_NUM(GetSessionValue(SESSION_NUM_PLAYERS, sessionSettings)));
	FString gameVersion = GetGameVersionString(GetSessionValue(SESSION_GAME_VERSION, sessionSettings));
	ADDTEXT_(INVTEXT("Game Version: {0}\n"), FText::FromString(gameVersion));

	//FString value;
	//sessionSettings.Get(SETTING_MAPNAME, value);
	//ss << "SETTING_MAPNAME: " << ToStdString(value);
	//ss << "\n";
}
static void PrintURL(std::stringstream& ss, FURL url)
{
	ss << "[URL:";
	ss << " protocol:" << ToStdString(url.Protocol);
	ss << " host:" << ToStdString(url.Host);
	ss << " port:" << url.Port;
	ss << " valid:" << url.Valid;
	ss << " valid:" << url.Valid;
	ss << " map:" << ToStdString(url.Map);

	for (FString option: url.Op) {
		ss << " option:" << ToStdString(option);
	}

	ss << " portal:" << ToStdString(url.Portal);
	ss << "]";
}

static void PrintSessionState(std::stringstream& ss, IOnlineSessionPtr& sessionPtr)
{
	ss << "Session Info [Debug]:\n";
	ss << "GetNumSessions: " << sessionPtr->GetNumSessions() << "\n";

	FOnlineSessionSettings* sessionSettings = sessionPtr->GetSessionSettings(PUN_SESSION_NAME);
	if (sessionSettings) {
		TArray<FText> args;
		PrintSessionSettings(args, *sessionSettings);
		ss << FTextToStd(JOINTEXT(args));
	} else {
		ss << "No sessionSettings\n";
	}

	FString connectInfo;
	sessionPtr->GetResolvedConnectString(PUN_SESSION_NAME, connectInfo);
	ss << "GetResolvedConnectString: " << ToStdString(connectInfo);
	ss << "\n\n";

}

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

	void PrintSessionInfo()
	{
		PUN_DEBUG2("Print Session Info:");
		PUN_DEBUG2(" playerCount:%d", playerCount());
		PUN_DEBUG2(" sessionValid:%d", IsSessionInterfaceValid());
		if (IsSessionInterfaceValid()) {
			sessionInterface->DumpSessionState();
			std::stringstream ss;
			PrintSessionState(ss, sessionInterface);
			PUN_DEBUG2("%s", ToTChar(ss.str()));
		}
	}
	void PrintPlayers();


	/*
	 * Save Game
	 */
	
	GameSaveInfo GetSavedGameToLoad() { return _savedGameToLoad; }

	bool IsLoadingSavedGame()
	{
		//PUN_DEBUG2("IsLoadingSavedGame %d", _savedGameToLoad.IsValid());
		return _savedGameToLoad.IsValid();
	}
	
	void SetSavedGameToLoad(GameSaveInfo savedGame)
	{
		PUN_DEBUG2("SetSavedGameToLoad %s %s", *savedGame.name, *savedGame.mapSettings.mapSeed);
		_savedGameToLoad = savedGame;
	}

	/*
	 * Map Settings
	 */
	void SetMapSettings(const TArray<int32>& mapSettingsBlob) {
		LLM_SCOPE_(EPunSimLLMTag::PUN_GameInstance);
		
		PunSerializedData blob(false);
		blob.Append(mapSettingsBlob);
		_mapSettings.Serialize(blob);
	}
	void SetMapSettings(FMapSettings mapSettingsIn) {
		_mapSettings = mapSettingsIn;
	}
	
	FMapSettings _mapSettings;

	FMapSettings GetMapSettings() {
		return _mapSettings;
	}

	static bool HasSavedMapWithSettings(const FMapSettings& mapSettings) {
		FString path = FPaths::ConvertRelativePathToFull(FPaths::ProjectSavedDir()) + "terrainMetaData.dat";

		if (!FPaths::FileExists(path)) {
			return false;
		}

		FMapSettings tempMapSettings;
		PunFileUtils::LoadFile(path, [&](FArchive& Ar) {
			tempMapSettings.Serialize(Ar);
		});

		return tempMapSettings.MapEquals(mapSettings);
	}
	static FMapSettings GetSavedMap(bool isSinglePlayerIn) {
		FString path = FPaths::ConvertRelativePathToFull(FPaths::ProjectSavedDir()) + "terrainMetaData.dat";

		if (!FPaths::FileExists(path)) {
			return FMapSettings::GetDefault(isSinglePlayerIn);
		}

		FMapSettings mapSettings;
		PunFileUtils::LoadFile(path, [&](FArchive& Ar) {
			mapSettings.Serialize(Ar);
		});
		mapSettings.isSinglePlayer = isSinglePlayerIn;
		return mapSettings;
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
	void HandleNetworkFailure(UWorld * World, UNetDriver * NetDriver, ENetworkFailure::Type FailureType, const FString & ErrorString);
	//{
	//	PUN_DEBUG2("!!! NetworkFailure %s, %s", ENetworkFailure::ToString(FailureType), *ErrorString);
	//	LOG_ERROR(LogNetworkInput, "!!! NetworkFailure %s, %s", ToString(FailureType), *ErrorString);

	//	//mainMenuPopup = FText::FormatNamed(ErrorString + " (" + FString(ENetworkFailure::ToString(FailureType)) + ")");
	//	mainMenuPopup = FText::Format(LOCTEXT("NetworkFailure", "{ErrorString} ({FailureType})"), 
	//		FText::FromString(ErrorString), 
	//		FText(ENetworkFailure::ToString(FailureType)));
	//}
	void HandleTravelFailure(UWorld* World, ETravelFailure::Type FailureType, const FString& ErrorString);
	//{
	//	PUN_DEBUG2("!!! TravelFailure %s, %s", ETravelFailure::ToString(FailureType), *ErrorString);
	//	LOG_ERROR(LogNetworkInput, "!!! TravelFailure %s, %s", ToString(FailureType), *ErrorString);

	//	//mainMenuPopup = FText(ErrorString + " (" + FString(ETravelFailure::ToString(FailureType)) + ")");
	//	mainMenuPopup = FText::FormatNamed(FText("{ErrorString} ({FailureType})"),
	//		"ErrorString", FText::FromString(ErrorString),
	//		"FailureType", FText(ETravelFailure::ToString(FailureType)));
	//}
	void HandlePreClientTravel(const FString& PendingURL, ETravelType TravelType, bool bIsSeamlessTravel)
	{
		PUN_DEBUG2("!!! PreClientTravel %s type:%d seamless:%d", *PendingURL, TravelType, bIsSeamlessTravel);
	}
	
	void OnGameExit();
	void EnsureSessionDestroyed(bool gotoMainMenu, bool gotoSinglePlayerLobby = false);

	FText mainMenuPopup;

	
	bool isJoiningGame = false;

	bool isOpeningLoadMutiplayerPreLobby = false;

	FString lobbyPassword;
	

	// Single player?
	bool isSinglePlayer = false;
	bool isMultiplayer() { return !isSinglePlayer; }

	bool shouldDelayInput() {
		if (PunSettings::IsOn("ForceDelayInput")) {
			return true;
		}
		return !isSinglePlayer;
	}
	
public:
	void InitOnline(FName subsystemName);
	
	IOnlineSessionPtr sessionInterface;
	TSharedPtr<class FOnlineSessionSearch> sessionSearch;

	IOnlineSessionPtr sessionInterfaceUsedToFindGame = nullptr;
	FName subsystemNameInUse = "NULL";

	bool IsLANMatch() { return subsystemNameInUse == "NULL"; }

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
	FOnDestroySessionCompleteDelegate DestroySessionThenGoToMainMenuCompleteDelegate;
	FOnDestroySessionCompleteDelegate DestroySessionThenDoesNothingCompleteDelegate;


	void FindGame();
	void JoinGame(const FOnlineSessionSearchResult& searchResult);
	void DestroyGame();

	void UpdateSession();
	void ServerOnStartedGame();

	FOnlineSessionSettings* GetSessionSettings() {
		return sessionInterface->GetSessionSettings(PUN_SESSION_NAME);
	}

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
		SetCreateMultiplayerGameState();
		CreateGame_Phase1();
	}
	void LoadMultiplayerGame() {
		isSinglePlayer = false;
		bIsLoadingMultiplayerGame = true;
		CreateGame_Phase1();
	}


	// Set state to prepare for creating MP game
	void SetCreateMultiplayerGameState() {
		isSinglePlayer = false;
		bIsLoadingMultiplayerGame = false;
	}

private:
	void CreateGame_Phase1();
	void CreateGame_Phase2();

public:
	const int32 MaxPlayer = 12;

	int32 sessionTickCount = 0;

	
	FOnlineSessionSettings GetPreferredSessionSettings()
	{
		FOnlineSessionSettings sessionSettings;

		sessionSettings.bIsLANMatch = (IOnlineSubsystem::Get()->GetSubsystemName() == "NULL") || isSinglePlayer;

		sessionSettings.NumPublicConnections = MaxPlayer;
		sessionSettings.NumPrivateConnections = 0;
		sessionSettings.bAllowInvites = true;
		sessionSettings.bAllowJoinInProgress = true;
		sessionSettings.bShouldAdvertise = true;
		sessionSettings.bAllowJoinViaPresence = true;
		sessionSettings.bAllowJoinViaPresenceFriendsOnly = false;

		sessionSettings.bUsesPresence = true; // bUsesPresence = true, allows non-public ip server to be seen (like in wireless netwoork)
		sessionSettings.BuildUniqueId = GetBuildUniqueId(); // is for engine..

		// In GameMap, don't allow joining
		if (IsInGame()) {
			sessionSettings.bAllowInvites = false;
			sessionSettings.bShouldAdvertise = false;
			sessionSettings.bAllowJoinInProgress = false;
			sessionSettings.bAllowJoinViaPresence = false;
			sessionSettings.bUsesPresence = false;
		}


		FString nickName = IOnlineSubsystem::Get()->GetIdentityInterface()->GetPlayerNickname(0);

		//sessionSettings.Set(SESSION_HOSTNAME, nickName, EOnlineDataAdvertisementType::ViaOnlineService);
		sessionSettings.Set(SESSION_TICK, sessionTickCount/120, EOnlineDataAdvertisementType::ViaOnlineService);
		sessionSettings.Set(SESSION_NUM_PLAYERS, playerCount(), EOnlineDataAdvertisementType::ViaOnlineService);
		sessionSettings.Set(SESSION_GAME_VERSION, GAME_VERSION, EOnlineDataAdvertisementType::ViaOnlineService);

		sessionSettings.Set(SESSION_PASSWORD, lobbyPassword, EOnlineDataAdvertisementType::ViaOnlineService);
		
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
	bool IsInGame();
	static bool IsInGame(const UObject* worldContextObject);


	TArray<FString> replayFilesToLoad;
	bool ReplayCount() { return replayFilesToLoad.Num(); }

private:
	TArray<FPlayerInfo> _playerInfos;
	TArray<bool> _playerReadyStates; // ReadyStates in Game Map is used to determined if that player is loaded

	// Cache playerInfos for map transition
	TArray<FPlayerInfo> _playerInfosCache;

	//! This is only used by server's main controller...
	//! Count the ticks getting sent out 
	int32 _serverTick = 0;
	
public:
	TArray<bool> playerConnectedStates;
	int32 hostPlayerId = -1;
	
	TArray<int32> clientPacketsReceived; // For client to track packets received


	// When switching map (Lobby to Game), keep the targetPlayerCount
	// (so we can ensure everyone is connected to the new map before proceeding)
	int32 targetPlayerCount = 0;


	void DebugPunSync(std::string message)
	{
#if DEBUG_BUILD
		std::stringstream ss;
		
		ss << message << ": ";
		for (int32 i = 0; i < _playerInfos.Num(); i++) {
			ss << "[" << ToStdString(_playerInfos[i].name.ToString());
			ss << "," << _playerInfos[i].steamId64;
			ss << "," << playerConnectedStates[i] << "," << _playerReadyStates[i] << "] ";
		}
		_LOG(PunSync, " %s", ToTChar(ss.str()));
#endif
	}

	
	void SetPlayerCount(int32 count)
	{
		_LOG(PunSync, "SetPlayerCount");
		PrintPlayers();
		
		_playerInfos.SetNum(count);
		_playerReadyStates.SetNum(count);
		playerConnectedStates.SetNum(count);

		clientPacketsReceived.SetNum(count);

		PrintPlayers();
	}

	// This is done upon:
	// - Returning to main menu
	// - Disconnected from host
	// - Go to singleplayer mode
	void ResetPlayerCount()
	{
		_LOG(PunSync, "ResetPlayerCount");
		PrintPlayers();
		
		_playerInfos.SetNum(0);
		_playerReadyStates.SetNum(0);
		playerConnectedStates.SetNum(0);

		clientPacketsReceived.SetNum(0);

		PrintPlayers();
	}
	void ResetPlayers()
	{
		_LOG(PunSync, "ResetPlayers");
		PrintPlayers();
		
		for (int32 i = 0; i < _playerInfos.Num(); i++) {
			_playerInfos[i] = FPlayerInfo();
			_playerReadyStates[i] = false;
			playerConnectedStates[i] = false;
			clientPacketsReceived[i] = 0;
		}

		PrintPlayers();
	}

	// Player Name
	const TArray<FPlayerInfo>& playerInfoList() {
		return _playerInfos;
	}

	FPlayerInfo GetPlayerInfoBySteamId(uint64 steamId64);
	
	
	FString playerNameF(int32 playerId) {
		if (playerId >= GameConstants::MaxPlayersPossible) {
			PUN_CHECK(playerId < GameConstants::MaxPlayersAndAI);
			return GetAITownName(playerId);
		}
		//PUN_ENSURE(playerId < _playerNames.Num(), return FString("None"));
		
		return playerId < _playerInfos.Num() ? _playerInfos[playerId].name.ToString() : FString("None"); // Empty _playerNames may happen on Load
	}
	//void SetPlayerNameF(int32 playerId, FString playerNameF) {
	//	_LOG(PunSync, "SetPlayerNameF %d, %s", playerId, *playerNameF);
	//	_playerInfos[playerId].name = FText::FromString(playerNameF);
	//}

	void CachePlayerInfos() {
		_LOG(PunSync, "CachePlayerInfos");
		PrintPlayers();
		
		_playerInfosCache = _playerInfos;
	}
	FPlayerInfo GetCachedPlayerInfo(uint64 steamId64);


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
		return playerId < _playerReadyStates.Num() ? _playerReadyStates[playerId] : false;
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

	void SetPlayerInfos(TArray<FPlayerInfo> playerInfosIn);
	void SetPlayerInfo(const FPlayerInfo& playerInfo); // Use steamId to match the correct player
	

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
	int32 ConnectPlayer(FString playerNameF, uint64 steamId64);

	bool TryChangePlayerId(int32 originalId, int32 newId);


	void DisconnectPlayer(FString playerNameF, uint64 steamId64);

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
		for (int32 i = 0; i < _playerInfos.Num(); i++) {
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
		for (int32 i = 0; i < _playerInfos.Num(); i++) {
			if (!_playerInfos[i].IsEmpty()) {
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
		for (int32 i = 0; i < _playerInfos.Num(); i++) {
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
		for (int32 i = 0; i < _playerInfos.Num(); i++) {
			if (!_playerInfos[i].IsEmpty() && !playerConnectedStates[i]) {
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
	 * Steam Achievements
	 */
	void QueryAchievements();

	void OnQueryAchievementsComplete(const FUniqueNetId& PlayerId, const bool bWasSuccessful);

	void UpdateAchievementProgress(const FString& Id, float Percent);

	FOnlineAchievementsWritePtr AchievementsWriteObjectPtr;

	/*
	 * Steam Stats
	 */
	bool UseSteamStatsInterface(IOnlineStatsPtr& SteamStatsPtr, TSharedPtr<const FUniqueNetId>& UserId)
	{
		IOnlineSubsystem* OnlineSub = IOnlineSubsystem::Get();
		if (OnlineSub)
		{
			IOnlineIdentityPtr Identity = OnlineSub->GetIdentityInterface();
			if (Identity.IsValid())
			{
				UserId = Identity->GetUniquePlayerId(0);
				SteamStatsPtr = OnlineSub->GetStatsInterface();
				if (SteamStatsPtr.IsValid()) 
				{
					return true;
				}
			}
		}
		return false;
	}

	void UpdateSteamStats(const FString& statName, int32 statValue);
	void QuerySteamStats();
	void GetSteamStats();
	void ResetSteamStats();

	FOnlineStatsUpdateStatsComplete UpdateStatsCompleteDelegate;
	FOnlineStatsQueryUserStatsComplete QueryStatsCompleteDelgate;

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

	AutosaveEnum autosaveEnum = AutosaveEnum::Year;
	FString preferredCultureTag;
	bool useMultithreadedMeshGeneration = true;
	bool forceClickthrough = false;
	
	bool homelessWarningSound = true;
	bool roundCountdownSound = true;
	
public:
	/*
	 * Settings
	 */
	bool needSettingsUpdate = false;

	void RestoreDefaultsGraphics() {
		_resolutionQuality = 70.0f;
	}
	void RestoreDefaultsSound() {
		LLM_SCOPE_(EPunSimLLMTag::PUN_GameInstance);
		
		_masterVolume = 0.8f;
		_musicVolume = 0.8f;
		_soundEffectsVolume = 0.8f;
		_ambientVolume = 0.8f;
		
		RefreshSoundSettings();
	}
	void RestoreDefaultsInputs() {
		mouseZoomSpeedFraction = 0.5f;
		mouseRotateSpeedFraction = 0.5f;
	}

	void RestoreDefaultsOthers() {
		autosaveEnum = AutosaveEnum::Year;
		preferredCultureTag = FString("en");
		useMultithreadedMeshGeneration = true;
		forceClickthrough = false;
		
		homelessWarningSound = true;
		roundCountdownSound = true;
	}

	void RestoreDefaultsAll()
	{
		RestoreDefaultsGraphics();
		RestoreDefaultsSound();
		RestoreDefaultsInputs();
		RestoreDefaultsOthers();
	}

	void SerializeOtherSettings(FArchive& Ar)
	{
		LLM_SCOPE_(EPunSimLLMTag::PUN_GameInstance);

		if (Ar.IsSaving()) {
			loadedVersion = GAME_VERSION;
		}
		Ar << loadedVersion;

		if (Ar.IsLoading() &&
			loadedVersion != GAME_VERSION) {
			return;
		}
		
		Ar << _masterVolume;
		Ar << _musicVolume;
		Ar << _soundEffectsVolume;
		Ar << _ambientVolume;

		Ar << mouseZoomSpeedFraction;
		Ar << mouseRotateSpeedFraction;

		Ar << _resolutionQuality;


		Ar << autosaveEnum;
		Ar << preferredCultureTag;
		Ar << useMultithreadedMeshGeneration;
		Ar << forceClickthrough;

		Ar << homelessWarningSound;
		Ar << roundCountdownSound;

		// Ensure uncorrupted preferredCultureTag
		TArray<FString> languageTags = UKismetInternationalizationLibrary::GetLocalizedCultures(ELocalizationLoadFlags::Game);
		if (!languageTags.Contains(preferredCultureTag)) { 
			preferredCultureTag = "en"; // Invalid culture tag
		}
	}

	void RefreshSoundSettings()
	{
		LLM_SCOPE_(EPunSimLLMTag::PUN_GameInstance);
		needSettingsUpdate = true;

		// This is for Main Menu Music
		if (AudioComponent) {
			AudioComponent->SetVolumeMultiplier(_masterVolume * _musicVolume);
		}
	}

	void RefreshOtherSettings()
	{
		PunSettings::Set("MultithreadedMeshGeneration", useMultithreadedMeshGeneration);
		PunSettings::Set("ForceClickthrough", forceClickthrough);
		PunSettings::Set("ShowDebugExtra", forceClickthrough);

		PunSettings::Set("HomelessWarningSound", homelessWarningSound);
		PunSettings::Set("RoundCountdownSound", roundCountdownSound);
	}

	void RefreshCulture()
	{
		FString currentCulture = UKismetInternationalizationLibrary::GetCurrentCulture();
	
		bool succeed = UKismetInternationalizationLibrary::SetCurrentCulture(preferredCultureTag, true);

		PUN_DEBUG2("RefreshCulture preferred:%s current:%s succeed:%d", *preferredCultureTag, *currentCulture, succeed);
	}
	

	float resolutionQuality() { return _resolutionQuality; }
	void SetResolutionQuality(float value) {
		_resolutionQuality = value;
		PUN_LOG("_resolutionQuality %f %f", _resolutionQuality, value);
	}
	static const int32 MinResolutionQuality = 40;
	int32 GetDisplayResolutionQuality() {
		return FMath::RoundToInt(resolutionQuality() * (100.0f - MinResolutionQuality) / 100.0f) + MinResolutionQuality;
	}

	int32 loadedVersion = -1;
	float mouseZoomSpeedFraction;
	float mouseRotateSpeedFraction;

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

		SerializeOtherSettings(SaveArchive);

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

		SerializeOtherSettings(LoadArchive);

		LoadArchive.FlushCache();
		binary.Empty();
		LoadArchive.Close();

		return true;
	}

	FString GetSettingsSavePath() {
		return FPaths::ConvertRelativePathToFull(FPaths::ProjectSavedDir()) + "/SettingsSaves/Settings.dat";
	}
};
