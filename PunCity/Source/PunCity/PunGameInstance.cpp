// Pun Dumnernchanvanit's


#include "PunCity/PunGameInstance.h"
#include "MoviePlayer.h"
#include "PunCity/PunUtils.h"
#include "Blueprint/UserWidget.h"
#include "Engine/LocalPlayer.h"
#include "Engine/Player.h"
#include "PunCity/Sound/SoundSystemComponent.h"
#include "GameDelegates.h"
#include "PunCity/PunPlayerController.h"

const static FName SESSION_NAME = TEXT("Pun Session Game");

void UPunGameInstance::Init()
{
	InitLLM();

	LLM_SCOPE_(EPunSimLLMTag::PUN_GameInstance);
	
	Super::Init();

	FCoreUObjectDelegates::PreLoadMap.AddUObject(this, &UPunGameInstance::BeginLoadingScreen);
	FCoreUObjectDelegates::PostLoadMapWithWorld.AddUObject(this, &UPunGameInstance::EndLoadingScreen);

	InitOnline("Steam");
	InitOnline("NULL");

	_saveSystem = make_unique<GameSaveSystem>();

	GetEngine()->OnNetworkFailure().AddUObject(this, &UPunGameInstance::HandleNetworkFailure);
	FGameDelegates::Get().GetExitCommandDelegate().AddUObject(this, &UPunGameInstance::OnGameExit);

#if !WITH_EDITOR
	GEngine->bEnableOnScreenDebugMessages = false;
#endif
	

//#if WITH_EDITOR
//	GAreScreenMessagesEnabled = false;
//#endif
}

void UPunGameInstance::CreateMainMenuSound(USoundBase* sound)
{
	LLM_SCOPE_(EPunSimLLMTag::PUN_GameInstance);

#if !MAIN_MENU_SOUND
	return;
#endif
	
	PUN_LOG("CreatePersistentSound World %p", this->GetWorld());

	if (!_soundSystem) {
		_soundSystem = NewObject<USoundSystemComponent>();
		_soundSystem->gameInstance = this;
		_soundSystem->bAllowAnyoneToDestroyMe = false;
		RegisterReferencedObject(_soundSystem);
		_soundSystem->InitForMainMenu(this, this);
	}

	// Main Menu Music
	if (!AudioComponent) {
		AudioComponent = UGameplayStatics::CreateSound2D(this, sound, 1, 1, 0, nullptr, true);
		AudioComponent->bAutoDestroy = false;
		AudioComponent->bAllowAnyoneToDestroyMe = false;
		RegisterReferencedObject(AudioComponent);
	}
}

void UPunGameInstance::PunTick()
{
	LLM_SCOPE_(EPunSimLLMTag::PUN_GameInstance);

#if MAIN_MENU_SOUND
	_soundSystem->CheckDespawnPunAudio();
#endif

	sessionTickCount++;
	if (sessionInterface.IsValid() && sessionTickCount % 120 == 0) {
		FOnlineSessionSettings sessionSettings = GetSessionSettings();
		//sessionInterface->UpdateSession(SESSION_NAME, sessionSettings);

		//PUN_DEBUG2("UpdateSession %d", sessionTickCount/120);
	}
}

void UPunGameInstance::BeginLoadingScreen(const FString& InMapName)
{
	LLM_SCOPE_(EPunSimLLMTag::PUN_GameInstance);
	
	PUN_LOG("BeginLoadingScreen");
	if (!IsRunningDedicatedServer())
	{
		FLoadingScreenAttributes LoadingScreenInfo;
		LoadingScreenInfo.bAutoCompleteWhenLoadingCompletes = false;

		if (LoadingScreenClass) {
			LoadingScreenUI = CreateWidget<UUserWidget>(GetWorld(), LoadingScreenClass);
			if (LoadingScreenUI) {
				LoadingScreenUI->AddToViewport();
				LoadingScreenInfo.WidgetLoadingScreen = LoadingScreenUI->TakeWidget();
			}
		} else {
			LoadingScreenInfo.WidgetLoadingScreen = FLoadingScreenAttributes::NewTestLoadingScreenWidget();
		}

		GetMoviePlayer()->SetupLoadingScreen(LoadingScreenInfo);
	}
}

void UPunGameInstance::EndLoadingScreen(UWorld* InLoadedWorld)
{
	PUN_LOG("EndLoadingScreen");
}


void UPunGameInstance::InitOnline(FName subsystemName)
{
	LLM_SCOPE_(EPunSimLLMTag::PUN_GameInstance);
	
	IOnlineSubsystem* onlineSubsystem = IOnlineSubsystem::Get(subsystemName); // IOnlineSubsystem::Get(FName("Steam"));
	if (onlineSubsystem) 
	{
		sessionInterface = onlineSubsystem->GetSessionInterface();
		if (sessionInterface.IsValid()) {
			sessionInterface->OnCreateSessionCompleteDelegates.Clear();
			sessionInterface->OnDestroySessionCompleteDelegates.Clear();
			sessionInterface->OnFindSessionsCompleteDelegates.Clear();
			sessionInterface->OnJoinSessionCompleteDelegates.Clear();
			sessionInterface->OnSessionInviteReceivedDelegates.Clear();
			sessionInterface->OnSessionUserInviteAcceptedDelegates.Clear();
			
			sessionInterface->OnCreateSessionCompleteDelegates.AddUObject(this, &UPunGameInstance::OnCreateSessionComplete);
			sessionInterface->OnDestroySessionCompleteDelegates.AddUObject(this, &UPunGameInstance::OnDestroySessionComplete);
			sessionInterface->OnFindSessionsCompleteDelegates.AddUObject(this, &UPunGameInstance::OnFindSessionsComplete);
			sessionInterface->OnJoinSessionCompleteDelegates.AddUObject(this, &UPunGameInstance::OnJoinSessionComplete);
			sessionInterface->OnSessionInviteReceivedDelegates.AddUObject(this, &UPunGameInstance::OnSessionInviteReceived);
			sessionInterface->OnSessionUserInviteAcceptedDelegates.AddUObject(this, &UPunGameInstance::OnSessionUserInviteAccepted);
			
			//DEFINE_ONLINE_DELEGATE_FOUR_PARAM(OnSessionUserInviteAccepted, const bool /*bWasSuccessful*/, const int32 /*ControllerId*/, TSharedPtr<const FUniqueNetId> /*UserId*/, const FOnlineSessionSearchResult& /*InviteResult*/);

			DestroySessionBeforeCreatingCompleteDelegate.BindLambda([&](FName name, bool success) {
				PUN_DEBUG2("Done DestroySessionBeforeCreatingCompleteDelegate");
				if (success) {
					CreateGame_Phase2();
				} else {
					bIsCreatingSession = false;
				}
			});
			DestroySessionThenDoesNothingCompleteDelegate.BindLambda([&](FName name, bool success) {
				PUN_DEBUG2("Done DestroySessionThenDoesNothingCompleteDelegate");
			});
			
			return;
		}
		else {
			PUN_DEBUG2("sessionInterface invalid");
		}
	}
	else {
		PUN_DEBUG2("Error no subsystem: %p", onlineSubsystem);
	}
}


void UPunGameInstance::FindGame()
{
	LLM_SCOPE_(EPunSimLLMTag::PUN_GameInstance);
	
	if (!IsSessionInterfaceValid()) {
		PUN_DEBUG2("[ERROR]FindGame: Not connected to Steam");
		mainMenuPopup = "Failed to find games. Please connect to Steam.";
		return;
	}
	
	sessionSearch = MakeShareable(new FOnlineSessionSearch());

	bool isLAN = IOnlineSubsystem::Get()->GetSubsystemName() == "NULL";
	//sessionSearch->bIsLanQuery = true; // This as false will find both offline/online sessions.

	sessionSearch->MaxSearchResults = 100;
	sessionSearch->QuerySettings.Set(SEARCH_PRESENCE, true, EOnlineComparisonOp::Equals);

	if (isLAN) {
		sessionInterface->FindSessions(0, sessionSearch.ToSharedRef());
		PUN_DEBUG2("FindSession: LAN");
	}
	else {
		TSharedPtr<const FUniqueNetId> localNetId = IOnlineSubsystem::Get()->GetIdentityInterface()->GetUniquePlayerId(0);
		if (localNetId.IsValid()) {
			sessionInterface->FindSessions(*localNetId, sessionSearch.ToSharedRef());
			PUN_DEBUG2("FindSession: %s , %s", *IOnlineSubsystem::Get()->GetSubsystemName().ToString(), *localNetId->ToDebugString());
		}
		else {
			PUN_DEBUG2("[ERROR]FindSession: localNetId invalid");
		}
	}

	sessionInterfaceUsedToFindGame = sessionInterface;

}

void UPunGameInstance::JoinGame(const FOnlineSessionSearchResult& searchResult)
{
	LLM_SCOPE_(EPunSimLLMTag::PUN_GameInstance);
	
	if (!IsSessionInterfaceValid()) {
		PUN_DEBUG2("[ERROR]JoinGame: Not connected to Steam");
		mainMenuPopup = "Failed to join the game. Please connect to Steam.";
		return;
	}
	
	FNamedOnlineSession* Session = sessionInterface->GetNamedSession(SESSION_NAME);
	if (Session) {
		PUN_DEBUG2("[ERROR]JoinGame: failed, already as session");
		mainMenuPopup = "Join game failed, try again in a moment.";

		sessionInterface->DestroySession(SESSION_NAME, DestroySessionThenDoesNothingCompleteDelegate);
		return;
	}
	
	sessionInterface->JoinSession(0, SESSION_NAME, searchResult);
	isJoiningGame = true;
}

void UPunGameInstance::DestroyGame()
{
	LLM_SCOPE_(EPunSimLLMTag::PUN_GameInstance);
	
	sessionInterface->DestroySession(SESSION_NAME);
}

void UPunGameInstance::UpdateSession()
{
	LLM_SCOPE_(EPunSimLLMTag::PUN_GameInstance);
	
	if (!IsSessionInterfaceValid()) {
		PUN_DEBUG2("[ERROR]UpdateSession: Not connected to Steam");
		return;
	}
	
	PUN_DEBUG2("UpdateSession");
	auto sessionSettings = GetSessionSettings();
	sessionInterface->UpdateSession(SESSION_NAME, sessionSettings);
}

void UPunGameInstance::ServerOnStartedGame()
{
	LLM_SCOPE_(EPunSimLLMTag::PUN_GameInstance);
	
	PUN_DEBUG2("OnStartedGame: UpdateSession");
	auto sessionSettings = GetSessionSettings();
	sessionSettings.bAllowJoinInProgress = false;
	sessionSettings.bUsesPresence = false;
#if !WITH_EDITOR
	sessionInterface->UpdateSession(SESSION_NAME, sessionSettings);
#endif
}

void UPunGameInstance::CreateGame_Phase1()
{
	LLM_SCOPE_(EPunSimLLMTag::PUN_GameInstance);
	
	if (!IsSessionInterfaceValid()) {
		PUN_DEBUG2("[ERROR]Host: Not connected to Steam");
		mainMenuPopup = "Failed to create a game. Please connect to Steam.";
		return;
	}

	// Check for existing session and destroy it
	// After session is destroyed, CreateGame_Phase2() is called
	auto existingSession = sessionInterface->GetNamedSession(SESSION_NAME);
	if (existingSession) {
		sessionInterface->DestroySession(SESSION_NAME, DestroySessionBeforeCreatingCompleteDelegate);
		bIsCreatingSession = true;
		return;
	}
	
	CreateGame_Phase2();
}

void UPunGameInstance::CreateGame_Phase2()
{
	LLM_SCOPE_(EPunSimLLMTag::PUN_GameInstance);

	if (!IsSessionInterfaceValid()) {
		PUN_DEBUG2("[ERROR]CreateMultiplayerGame: Not connected to Steam");
		mainMenuPopup = "Failed to create a game. Please connect to Steam.";
		bIsCreatingSession = false;
		return;
	}

	bool cancelSucceed = sessionInterface->CancelFindSessions();
	PUN_DEBUG2("CreateMultiplayerGame - CancelFindSessions:%d", cancelSucceed);

	FOnlineSessionSettings sessionSettings = GetSessionSettings();
	//PUN_DEBUG2("CreateSession: subsystem:%s", *IOnlineSubsystem::Get()->GetSubsystemName().ToString());

	if (sessionSettings.bIsLANMatch) {
		sessionInterface->CreateSession(0, SESSION_NAME, sessionSettings);

		//// TODO: REMOVE
		//AGameModeBase* gameMode = UGameplayStatics::GetGameMode(GetWorld());
		//bool canTravel = gameMode->CanServerTravel("/Game/Maps/Lobby?listen", false);
		//PUN_LOG("canTravel %d", canTravel);
		
		PUN_DEBUG2("CreateSession: LAN");
		bIsCreatingSession = true;
	}
	else {
		// Note... CreateSession(FUniqueNetId ... is not done from UE4 side... 
		TSharedPtr<const FUniqueNetId> localNetId = IOnlineSubsystem::Get()->GetIdentityInterface()->GetUniquePlayerId(0);
		if (localNetId.IsValid()) {
			sessionInterface->CreateSession(*localNetId, SESSION_NAME, sessionSettings);
			PUN_DEBUG2("CreateSession: %s , %s", *IOnlineSubsystem::Get()->GetSubsystemName().ToString(), *localNetId->ToDebugString());

			DumpSessionSettings(&sessionSettings);
			bIsCreatingSession = true;
		}
		else {
			PUN_DEBUG2("[ERROR]CreateSession: localNetId invalid");
			bIsCreatingSession = false;
		}
	}
}



void UPunGameInstance::OnCreateSessionComplete(FName sessionName, bool success)
{
	LLM_SCOPE_(EPunSimLLMTag::PUN_GameInstance);
	
	if (!success) {
		PUN_DEBUG2("[ERROR]Unable to create session %s", *sessionName.ToString());
		bIsCreatingSession = false;
		return;
	}

	//AGameModeBase* gameMode = UGameplayStatics::GetGameMode(GetWorld());
	//if (!gameMode->CanServerTravel("/Game/Maps/Lobby?listen", false)) {
	//	return;
	//}
	
	GetWorld()->ServerTravel("/Game/Maps/Lobby?listen");
	
	//FString Error;
	//GetEngine()->Browse(*WorldContext, FURL(*FString("/Game/Maps/Lobby?listen")), Error);
	//PUN_LOG("OnCreateSessionComplete %s", *Error);
	
	//UGameplayStatics::OpenLevel(GetWorld(), "Lobby", true, "listen");
}

void UPunGameInstance::OnFindSessionsComplete(bool success)
{
	LLM_SCOPE_(EPunSimLLMTag::PUN_GameInstance);
	
	// The FindSession command was using a different sessionInterface
	if (sessionInterfaceUsedToFindGame != sessionInterface) {
		PUN_DEBUG2("[ERROR]OnFindSessionsComplete: Wrong sessionInterface");
		return;
	}

	if (!success) {
		PUN_DEBUG2("[ERROR]OnFindSessionsComplete: !success");
		return;
	}

	if (!sessionSearch.IsValid()) {
		PUN_DEBUG2("[ERROR]OnFindSessionsComplete: sessionSearch invalid");
		return;
	}
	
	PUN_DEBUG2("Finished finding session %d", sessionSearch->SearchResults.Num());
	for (const FOnlineSessionSearchResult& searchResult : sessionSearch->SearchResults)
	{
		FString value;
		searchResult.Session.SessionSettings.Get(SETTING_MAPNAME, value);

		PUN_DEBUG2(" Found session name: %s", *value);
	}
}

void UPunGameInstance::OnDestroySessionComplete(FName sessionName, bool success)
{
	LLM_SCOPE_(EPunSimLLMTag::PUN_GameInstance);
	
	PUN_DEBUG2("OnDestroySessionComplete");
	LOG_ERROR(LogNetworkInput, "OnDestroySessionComplete");
}

void UPunGameInstance::OnJoinSessionComplete(FName sessionName, EOnJoinSessionCompleteResult::Type resultType)
{
	LLM_SCOPE_(EPunSimLLMTag::PUN_GameInstance);
	
	PUN_DEBUG2("OnJoinSessionComplete");
	
	if (!sessionInterface.IsValid()) {
		PUN_DEBUG2("- [ERROR]SessionInvalid");
		mainMenuPopup = "Failed to join the game. Please ensure your connection to Steam.";
		isJoiningGame = false;
		return;
	}
	
	APlayerController* controller = GetFirstLocalPlayerController();
	FString travelURL;

	if (resultType == EOnJoinSessionCompleteResult::Type::Success)
	{
		if (controller)
		{
			if (sessionInterface->GetResolvedConnectString(SESSION_NAME, travelURL))
			{
				PUN_DEBUG2("- Travel URL:%s", *travelURL);
				isSinglePlayer = false;
				controller->ClientTravel(travelURL, ETravelType::TRAVEL_Absolute);
			}
			else {
				PUN_DEBUG2("- [ERROR]GetResolvedConnectString Failed");
				mainMenuPopup = "Failed to resolve connection";
				isJoiningGame = false;
			}
		}
		else {
			PUN_DEBUG2("- [ERROR]No Controller");
			mainMenuPopup = "Failed to join the game. No FirstPlayerController";
			isJoiningGame = false;
		}
	}
	else if (resultType == EOnJoinSessionCompleteResult::Type::SessionIsFull)
	{
		PUN_DEBUG2("- [ERROR]SessionIsFull");
		mainMenuPopup = "Failed to join the game. The game is full";
		isJoiningGame = false;
	}
	else if (resultType == EOnJoinSessionCompleteResult::Type::SessionDoesNotExist)
	{
		PUN_DEBUG2("- [ERROR]SessionDoesNotExist");
		mainMenuPopup = "Failed to join the game. The game no longer exist";
		isJoiningGame = false;
	}
	/** There was an error getting the session server's address */
	else if (resultType == EOnJoinSessionCompleteResult::Type::CouldNotRetrieveAddress)
	{
		PUN_DEBUG2("- [ERROR]CouldNotRetrieveAddress");
		mainMenuPopup = "Failed to join the game. Could not retrieve the address";
		isJoiningGame = false;
	}
	/** The user attempting to join is already a member of the session */
	else if (resultType == EOnJoinSessionCompleteResult::Type::AlreadyInSession)
	{
		PUN_DEBUG2("- [ERROR]AlreadyInSession");
		mainMenuPopup = "Failed to join the game. Already in the game";
		isJoiningGame = false;
	}
	else if (resultType == EOnJoinSessionCompleteResult::Type::UnknownError)
	{
		PUN_DEBUG2("- [ERROR]UnknownError");
		mainMenuPopup = "Failed to join the game. Unknown error joining";
		isJoiningGame = false;
	}
	
}

void UPunGameInstance::OnSessionInviteReceived(const FUniqueNetId& UserId, const FUniqueNetId& FromId, const FString& AppId, const FOnlineSessionSearchResult& InviteResult)
{
	LLM_SCOPE_(EPunSimLLMTag::PUN_GameInstance);
	
	PUN_DEBUG2("OnSessionInviteReceived From:%s", *FromId.ToDebugString());
	PUN_DEBUG2("SelfId:%s, Session:%s", *(UserId.ToDebugString()), *InviteResult.GetSessionIdStr());


	
}

void UPunGameInstance::OnSessionUserInviteAccepted(bool bWasSuccessful, int32 ControllerId, TSharedPtr<const FUniqueNetId> UserId, const FOnlineSessionSearchResult& InviteResult)
{
	LLM_SCOPE_(EPunSimLLMTag::PUN_GameInstance);
	
	PUN_DEBUG2("OnSessionUserInviteAccepted success:%d", bWasSuccessful);
	PUN_DEBUG2("UserId:%s, Session:%s", *UserId->ToDebugString(), *InviteResult.GetSessionIdStr());
	
	if (bWasSuccessful) {
		if (InviteResult.IsValid()) {
			JoinGame(InviteResult);
		}
		else {
			PUN_DEBUG2("- [ERROR]Invalid InviteSearchResult");
		}
	}
}


void UPunGameInstance::Spawn2DSound(std::string groupName, std::string soundName)
{
	LLM_SCOPE_(EPunSimLLMTag::PUN_GameInstance);

#if MAIN_MENU_SOUND
	_soundSystem->Spawn2DSound(groupName, soundName);
#endif
}

void UPunGameInstance::OnGameExit()
{
	LLM_SCOPE_(EPunSimLLMTag::PUN_GameInstance);
	
	PUN_DEBUG2("OnGameExit");
	EnsureSessionDestroyed();
}

void UPunGameInstance::EnsureSessionDestroyed()
{
	LLM_SCOPE_(EPunSimLLMTag::PUN_GameInstance);
	
	PUN_DEBUG2("EnsureSessionDestroyed");
	
	if (sessionInterface.IsValid())
	{
		auto existingSession = sessionInterface->GetNamedSession(SESSION_NAME);
		if (existingSession) {
			PUN_DEBUG2("EnsureSessionDestroyed: Completed");
			sessionInterface->DestroySession(SESSION_NAME, DestroySessionThenDoesNothingCompleteDelegate);
		}
	}
}

bool UPunGameInstance::IsInGame(const UObject* worldContextObject) {
	auto gameController = Cast<APunPlayerController>(worldContextObject->GetWorld()->GetFirstPlayerController());
	return gameController != nullptr;
}

//
//TSharedPtr<const FUniqueNetId> UPunGameInstance::GetSteamID()
//{
//	IOnlineSubsystem* onlineSubsystem = IOnlineSubsystem::Get(FName("Steam"));
//
//	if (onlineSubsystem) {
//		PUN_DEBUG2("Using Subsystem : %s", *onlineSubsystem->GetSubsystemName().ToString());
//	} else {
//		PUN_DEBUG2("Error no steam: %p", onlineSubsystem);
//		return nullptr;
//	}
//
//	APlayerController* controller = GetFirstLocalPlayerController();
//	int32 controllerId = UGameplayStatics::GetPlayerControllerID(controller);
//	IOnlineIdentityPtr identityInterface = onlineSubsystem->GetIdentityInterface();
//	TSharedPtr<const FUniqueNetId> netId = identityInterface->GetUniquePlayerId(controllerId);
//
//	PUN_DEBUG2("Login status: %s", ToString(identityInterface->GetLoginStatus(*netId)));
//
//	PUN_DEBUG2("GetPlatformUserName: %s", *UKismetSystemLibrary::GetPlatformUserName());
//
//	PUN_DEBUG2("GetPlayerName: %s", *controller->PlayerState->GetPlayerName());
//
//	//IOnlineUserPtr userInterface = onlineSubsystem->GetUserInterface();
//// TODO: IOnlineUserPtr not valid
//
//	// TODO: GetUserAccount etc. never work?
//	//if (identityInterface.IsValid())
//	//{
//
//	//	TArray<TSharedPtr<FUserOnlineAccount>> OutUsers = identityInterface->GetAllUserAccounts();
//
//	//	PUN_DEBUG2("GetAllUserInfo: %d", OutUsers.Num());
//	//	for (int32 i = 0; i < OutUsers.Num(); i++) {
//	//		PUN_DEBUG2("  OutUsers: %d", i);
//	//		PUN_DEBUG2("    Display name: %s", *OutUsers[i]->GetDisplayName());
//	//		PUN_DEBUG2("    Real name: %s", *OutUsers[i]->GetRealName());
//	//	}
//
//	//	TSharedPtr<FUserOnlineAccount> account = identityInterface->GetUserAccount(*netId);
//	//	if (account.IsValid()) {
//	//		PUN_DEBUG2("Display name: %s", *account->GetDisplayName());
//	//		PUN_DEBUG2("Real name: %s", *account->GetRealName());
//	//	}
//	//	else {
//	//		PUN_DEBUG2("FUserOnlineAccount not valid");
//	//	}
//	//}
//	//else {
//	//	PUN_DEBUG2("IOnlineIdentityPtr not valid");
//	//}
//
//	// TODO: explore user attributes
//	//PUN_DEBUG2("Display name: %s", *account->GetUserAttribute());
//
//	return netId;
//}

//void UPunGameInstance::HostOnlineGame()
//{
//	PUN_DEBUG2("HostOnlineGame");
//	
//	ULocalPlayer* const Player = GetFirstGamePlayer();
//	FUniqueNetIdRepl uniqueNetId = Player->GetPreferredUniqueNetId();
//	HostSession(*uniqueNetId, GameSessionName, true, true, 4);
//}

///*
// * Friends
// */
//void UPunGameInstance::FindFriends(TSharedPtr<const FUniqueNetId> UserId)
//{
//	PUN_DEBUG2("FindFriends");
//	
//	IOnlineSubsystem* onlineSubsystem = IOnlineSubsystem::Get(FName("Steam"));
//
//	if (onlineSubsystem) {
//		IOnlineFriendsPtr friendsInterface = onlineSubsystem->GetFriendsInterface();
//
//		APlayerController* controller = GetFirstLocalPlayerController();
//		int32 controllerId = UGameplayStatics::GetPlayerControllerID(controller);
//
//		TArray<TSharedRef<FOnlineFriend>> friendList;
//		friendsInterface->GetFriendsList(controllerId, FString(), friendList);
//
//		PUN_DEBUG2(" Friends:%d", friendList.Num());
//		for (int32 i = 0; i < friendList.Num(); i++) {
//			PUN_DEBUG2("  Friend: %s, real:%s", *friendList[i]->GetDisplayName(), *friendList[i]->GetRealName());
//		}
//	}
//}