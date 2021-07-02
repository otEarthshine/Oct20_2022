#include "PunCity/PunPlayerController.h"
#include "Kismet/GameplayStatics.h"

#include "PunCity/Simulation/GameSimulationInfo.h"

#include "PunCity/Simulation/GameSimulationCore.h"
#include "PunCity/Simulation/TreeSystem.h"
#include "PunCity/Simulation/UnitSystem.h"

#include "PunCity/PunUtils.h"
#include "PunCity/PunGameInstance.h"
#include "PunCity/PunGameMode.h"

#include "Framework/Application/NavigationConfig.h"

using namespace std;


struct NetworkCommand
{
	int32 simulationTick;
	function<void(APunPlayerController*)> commandFunction;
};

void FInputModeGameAndUI_Pun::ApplyInputMode(FReply& SlateOperations, class UGameViewportClient& GameViewportClient) const
{
	TSharedPtr<SViewport> ViewportWidget = GameViewportClient.GetGameViewportWidget();
	if (ViewportWidget.IsValid())
	{
		const bool bLockMouseToViewport = MouseLockMode == EMouseLockMode::LockAlways
			|| (MouseLockMode == EMouseLockMode::LockInFullscreen && GameViewportClient.IsExclusiveFullscreenViewport());
		SetFocusAndLocking(SlateOperations, WidgetToFocus, bLockMouseToViewport, ViewportWidget.ToSharedRef());

		SlateOperations.ReleaseMouseCapture();

		GameViewportClient.SetMouseLockMode(MouseLockMode);
		GameViewportClient.SetIgnoreInput(false);
		GameViewportClient.SetHideCursorDuringCapture(bHideCursorDuringCapture);

		GameViewportClient.SetCaptureMouseOnClick(isGameOnly ? EMouseCaptureMode::CapturePermanently : EMouseCaptureMode::CaptureDuringMouseDown);
	}
}



/*
 * 
 */

class FOhNavigationConfig : public FNavigationConfig
{
public:
	/** Returns the navigation action corresponding to this key, or Invalid if not found */
	EUINavigationAction GetNavigationActionFromKey(const FKeyEvent& InKeyEvent) const override
	{
		FKey key = InKeyEvent.GetKey();
		
		if (/*InKey == EKeys::Enter || InKey == EKeys::SpaceBar ||*/ key == EKeys::Virtual_Accept)
		{
			// By default, enter, space, and gamepad accept are all counted as accept
			return EUINavigationAction::Accept;
		}
		else if (key == EKeys::Escape || key == EKeys::Virtual_Back)
		{
			// By default, escape and gamepad back count as leaving current scope
			return EUINavigationAction::Back;
		}
		else if (key == EKeys::Semicolon)
		{
			
		}

		return EUINavigationAction::Invalid;
	}
};

//{
//	TSharedPtr<SViewport> ViewportWidget = GameViewportClient.GetGameViewportWidget();
//	if (ViewportWidget.IsValid())
//	{
//		TSharedRef<SViewport> ViewportWidgetRef = ViewportWidget.ToSharedRef();
//		SlateOperations.UseHighPrecisionMouseMovement(ViewportWidgetRef);
//		SlateOperations.SetUserFocus(ViewportWidgetRef);
//		SlateOperations.LockMouseToWidget(ViewportWidgetRef);
//		GameViewportClient.SetMouseLockMode(EMouseLockMode::LockOnCapture);
//		GameViewportClient.SetIgnoreInput(false);
//		GameViewportClient.SetCaptureMouseOnClick(bConsumeCaptureMouseDown ? EMouseCaptureMode::CapturePermanently : EMouseCaptureMode::CapturePermanently_IncludingInitialMouseDown);
//	}
//}

unordered_map<int32, int32> APunPlayerController::kPlayerIdToClogStatus;
unordered_map<int32, std::vector<int32>> APunPlayerController::kPlayerIdToTickHashList;

int32 APunPlayerController::kGameSpeed = 1;
int32 APunPlayerController::kResumeGameSpeed = 1;

std::vector<shared_ptr<FNetworkCommand>> APunPlayerController::kCommandQueue;

std::vector<NetworkTickInfo> APunPlayerController::kNetworkTickInfoCache;
std::unordered_map<int32, int32> APunPlayerController::kPlayerIdToMissingGameTick;


APunPlayerController::APunPlayerController() : APunBasePlayerController()
{
	PrimaryActorTick.bCanEverTick = true;
	bAllowTickBeforeBeginPlay = false;

	gameManager = nullptr;

	// Set player controller's statuses
	bShowMouseCursor = true;
	bEnableClickEvents = true;
	bEnableMouseOverEvents = true;
	FViewTargetTransitionParams params;
	SetViewTarget(this, params);

	if (FSlateApplication::IsInitialized())
	{
		TSharedPtr<FOhNavigationConfig> Navigation = MakeShared<FOhNavigationConfig>();
		Navigation->bKeyNavigation = false;
		Navigation->bTabNavigation = false;
		Navigation->bAnalogNavigation = false;
		FSlateApplication::Get().SetNavigationConfig(Navigation.ToSharedRef());
	}


	_loadStep = LoadStepEnum::NotStarted;

	///*
	// * Prevent spacebar/enter on button
	// */
	//TSharedPtr<FPunInputProcessor> inputPreProcessor = MakeShared<FPunInputProcessor>();
	//FSlateApplication::Get().RegisterInputPreProcessor(inputPreProcessor);
	//
}

void APunPlayerController::OnPostLogin()
{
	LLM_SCOPE_(EPunSimLLMTag::PUN_Controller);
	
	PUN_DEBUG2("OnPostLogin %s", *PlayerState->GetPlayerName());

	int32 controllerId = UGameplayStatics::GetPlayerControllerID(this);

	// Set server up to wait for InitCallbacks from clients 
	if (controllerId == 0)
	{
		kPlayerIdToClogStatus.clear();
		kPlayerIdToTickHashList.clear();
		kCommandQueue.clear();

		PUN_DEBUG(TEXT("Server SetupLocalPrimaryController pid:0"));
		kGameSpeed = 1;
		kResumeGameSpeed = 1;

		_serverLoadStage = ServerLoadStage::WaitingForInitCallbacks;
		return;
	}

	// Send out message to make clients start loading
	InitializeClient_ToClient();

}

void APunPlayerController::BeginPlay()
{
	LLM_SCOPE_(EPunSimLLMTag::PUN_Controller);
	
	Super::BeginPlay();

	_playerControllerTick = 0;
	_hashSendTick = 0;

	_proxyControllerTick = 0;

}

void APunPlayerController::LoadController_Prepare()
{
	LLM_SCOPE_(EPunSimLLMTag::PUN_Controller);
	
	PUN_DEBUG(FString::Printf(TEXT("SetupLocalPrimaryController pid:%d cid:%d"), controllerPlayerId(), UGameplayStatics::GetPlayerControllerID(this)));
	INIT_LOG("SetupLocalPrimaryController pid:%d cid:%d", controllerPlayerId(), UGameplayStatics::GetPlayerControllerID(this));

	// Setup initial loading screen...
	auto punHUD = GetPunHUD();
	if (punHUD) {
		auto escMenuUI = punHUD->escMenuUI();
		if (escMenuUI) {
			escMenuUI->SetLoadingText(NSLOCTEXT("PunPlayerController", "Loading_PrepareWorld", "Preparing the World..."));
		}
	}

	_loadStep = LoadStepEnum::GameManagerStart;
	//_primaryControllerInitStage = 2;
}

void APunPlayerController::LoadController_GameManager()
{
	LLM_SCOPE_(EPunSimLLMTag::PUN_Controller);
	
	auto gameInstance = Cast<UPunGameInstance>(GetGameInstance());
	GameSaveInfo saveInfo = gameInstance->GetSavedGameToLoad();

	if (saveInfo.IsValid()) {
		gameInstance->SetMapSettings(saveInfo.mapSettings);
	}
	
	SetupGameManager(saveInfo.IsValid());
	SetupCameraPawn();

	cameraPawn->SetGameInterfaces(gameManager, this);
	cameraPawn->InitBuildingPlacementSystem(gameManager->assetLoader());

	GetPunHUD()->Setup(this, gameManager->worldWidgetParent);

	gameInstance->ResetServerTick();

	// Load if needed
	if (saveInfo.IsValid()) 
	{
		PUN_LOG("IsLoadingGame");
		_LOG(PunSaveCheck, "IsLoadingGame");

		gameInstance->saveSystem().Load(saveInfo.folderPath);

		gameInstance->SetSavedGameToLoad(GameSaveInfo());

#if CHECK_TICKHASH
		_hashSendTick = gameManager->simulation().GetHashSendTickAfterLoad();
		PUN_LOG("CHECK_TICKHASH Set _hashSendTick:%d", _hashSendTick);
#endif

		//PUN_LOG("UnitSystem LoadController unitCount():%d _unitLeans:%d", simulation().unitSystem().unitCount(), simulation().unitCount() - simulation().unitSystem().deadCount());
	}

	_loadStep = LoadStepEnum::GameManagerInProgress;
	PUN_DEBUG2("LoadStepEnum::GameManagerInProgress");
}

void APunPlayerController::SetupCameraPawn()
{
	LLM_SCOPE_(EPunSimLLMTag::PUN_Controller);
	
	TArray<AActor*> FoundCameraPawn;
	UGameplayStatics::GetAllActorsOfClass(GetWorld(), ACameraPawn::StaticClass(), FoundCameraPawn);
	if (FoundCameraPawn.Num() > 0) {
		cameraPawn = Cast<ACameraPawn>(FoundCameraPawn[0]);
	}
	else {
		cameraPawn = GetWorld()->SpawnActor<ACameraPawn>(ACameraPawn::StaticClass());
	}

	cameraPawn->Controller = this; // Need this for the restart to work
	cameraPawn->PawnClientRestart(); // PawnClientRestart help fix SetActorLocation not working problem

	/** 
	 * Possess Pawn
	 */
	if (GetPawn() && GetPawn() != cameraPawn){
		UnPossess();
	}
	if (cameraPawn->Controller != nullptr) {
		cameraPawn->Controller->UnPossess();
	}
	cameraPawn->PossessedBy(this);

	// update rotation to match possessed pawn's rotation
	SetControlRotation(cameraPawn->GetActorRotation());

	SetPawn(cameraPawn);
	check(GetPawn() != nullptr);

	if (GetPawn() && GetPawn()->PrimaryActorTick.bStartWithTickEnabled) {
		GetPawn()->SetActorTickEnabled(true);
	}

	ClientRestart(GetPawn());

	cameraPawn->SetActorLocationAndRotation(FVector::ZeroVector, FRotator(-60.0f, 0.0f, 0.0f)); // Y rotate 30.0f??
	cameraPawn->camera()->SetRelativeLocation(FVector(-300.0f, 0.0f, 100.0f));
}

void APunPlayerController::SetupGameManager(bool isLoadingFromFile)
{
	LLM_SCOPE_(EPunSimLLMTag::PUN_Controller);
	
	TArray<AActor*> FoundGameManagers;
	UGameplayStatics::GetAllActorsOfClass(GetWorld(), AGameManager::StaticClass(), FoundGameManagers);
	if (FoundGameManagers.Num() > 0) {
		PUN_LOG("Got GameManager %p", FoundGameManagers[0]);
		
		gameManager = Cast<AGameManager>(FoundGameManagers[0]);

		gameManager->assetLoader()->CleanModuleNames();
		gameManager->assetLoader()->CheckMeshesAvailable();
		//PUN_CHECK(gameManager->assetLoader()->moduleNames().Num() == ModuleMeshCount);
		
		gameManager->SetOwner(this);
		gameManager->Init(controllerPlayerId(), isLoadingFromFile, GetPunHUD(), this);

		auto gameInstance = CastChecked<UPunGameInstance>(GetGameInstance());
		gameInstance->SetSaveSystemController(this);


		PunSettings::Set("AlreadyDesynced", 0);
	}
	else {
		PUN_LOG("No GameManager");
	}
}


static std::chrono::time_point<std::chrono::steady_clock> lastFPSCountTime;
static int32 fpsCount = 0;
static int32 lastCountedFPS = 0;
int32 APunPlayerController::GetFPS() {
	return lastCountedFPS;
}


void APunPlayerController::Tick(float DeltaTime)
{
	APunBasePlayerController::Tick(DeltaTime);
	
	LLM_SCOPE_(EPunSimLLMTag::PUN_Controller);

	if (_tickDisabled) {
		return;
	}

	_playerControllerTick++;

#if USE_LEAN_PROFILING
	// FPS counter
	{
		auto timeNow = std::chrono::high_resolution_clock::now();
		long long time_span_nanoseconds = std::chrono::duration_cast<std::chrono::nanoseconds>(timeNow - lastFPSCountTime).count();

		if (time_span_nanoseconds >= 1000 * 1000 * 1000) 
		{
			lastCountedFPS = fpsCount;
			
			lastFPSCountTime = timeNow;
			fpsCount = 0;
		}

		fpsCount++;
	}

	if (LeanProfiler::EnumToElements.size() > 0) 
	{
		int32 endIndex = static_cast<int32>(LeanProfilerEnum::TickStatisticsUI);
		LeanProfiler::FinishTick(0, endIndex);
		if (_playerControllerTick % (PunSettings::Get("LeanProfilingTicksInterval")) == 0) {
			LeanProfiler::FinishInterval(0, endIndex);
		}
	}
#endif
	
	
#if CHECK_TICKHASH
	// Once in a while also send tickHash to client.
	if (GetLocalRole() == ROLE_Authority && _playerControllerTick > 300)
	{
		auto firstController = CastChecked<APunPlayerController>(GetWorld()->GetFirstPlayerController());
		if (firstController->gameManager)
		{
			GameSimulationCore* simulation = firstController->gameManager->simulationPtr();
			if (simulation && simulation->tickHashes().TickCount() - _hashSendTick > 30)
			{
				// For Save Load, get a proper
				TArray<int32> allTickHashes;
				simulation->GetUnsentTickHashes(_hashSendTick, allTickHashes);

				if (allTickHashes.Num() > 0)
				{
					_LOG(PunTickHash, "[%d] SendHash Out Start hashSendTick:%d", playerId(), _hashSendTick);

					// Note: _hashSendTick becomes 0 upon loading, which is fine, since hashes will just override the loaded one...
					check(allTickHashes.Num() <= MaxPacketSize32);
					check(allTickHashes.Num() % TickHashes::TickHashEnumCount() == 0);

					SendHash_ToClient(_hashSendTick, allTickHashes);

					_hashSendTick += allTickHashes.Num() / TickHashes::TickHashEnumCount();

					_LOG(PunTickHash, "[%d] SendHash Out End hashSendTick:%d", playerId(), _hashSendTick);
				}
			}
		}
	}
#endif
	
	//PUN_DEBUG2("Tick controller: playerId:%d _remoteInitialized:%d ticks:%d", controllerPlayerId(), _remoteInitialized, _playerControllerTick); \

	auto gameInstance = CastChecked<UPunGameInstance>(GetGameInstance());
	auto gameModeRaw = UGameplayStatics::GetGameMode(this);
	

	/*
	 * Loading step
	 */
	if (_loadStep == LoadStepEnum::GameManagerStart) {
		LoadController_GameManager();
	}
	else if (_loadStep == LoadStepEnum::GameManagerInProgress) {
		if (gameManager)
		{
			gameManager->TickNetworking();

			if (gameManager->isGameManagerInitialized()) {
				if (IsServer()) {
					_serverLoadStage = ServerLoadStage::WaitingForLoadedCallbacks; // Host done, waiting on others
				}

				_loadStep = LoadStepEnum::Completed;
				PUN_DEBUG2("[InitSync] LoadStepEnum::Completed");

				// Refresh Culture
				_LOG(PunInit, "PunPlayerController Load Completed RefreshCulture");
				gameInstance->RefreshCulture();

				LoadedClientCallback_ToServer();
			}
		}
	}

	int32 targetPlayerCount = 1;
	if (gameInstance->isMultiplayer()) {
		targetPlayerCount = gameInstance->targetPlayerCount;
	}
	
	/*
	 * Server stage management
	 */
	 // The host start only once everyone else started...
	if (_serverLoadStage == ServerLoadStage::WaitingForInitCallbacks)
	{
		PUN_DEBUG2("[InitSync] WaitingForInitCallbacks : pcount:%d stage:%d , %d, %d", targetPlayerCount, GetPlayersBeyondLoadStage(ClientLoadStage::NotStarted),
																										GetPlayersBeyondLoadStage(ClientLoadStage::ClientLoading),
																										GetPlayersBeyondLoadStage(ClientLoadStage::Completed));
		
		if ((targetPlayerCount - 1) == GetPlayersBeyondLoadStage(ClientLoadStage::ClientLoading))
		{
			PUN_DEBUG2("[InitSync] Done WaitingForInitCallbacks");
			
			_serverLoadStage = ServerLoadStage::ServerLoading;
			_clientLoadStage = ClientLoadStage::ClientLoading;

			LoadController_Prepare();
		}

		return;
	}

	if (_serverLoadStage == ServerLoadStage::WaitingForLoadedCallbacks)
	{
		if (targetPlayerCount == GetPlayersBeyondLoadStage(ClientLoadStage::ClientLoading)) //??
		{
			PUN_DEBUG2("[InitSync] Done WaitingForLoadedCallbacks : %d", gameInstance->playerCount());
			_serverLoadStage = ServerLoadStage::Completed;
		}

		return;
	}

	
	if (_loadStep != LoadStepEnum::Completed) {
		return;
	}

	auto punHUD = GetPunHUD();

	// Given that all players are initialized, we start doing TickLocalSimulation_ToClients
	//  To make ticks for all players consistent, we do all server controller's TickLocalSimulation_ToClients together in the mainServerController tick
	if (gameModeRaw &&
		_serverLoadStage == ServerLoadStage::Completed)
	{
		APunGameMode* gameMode = CastChecked<APunGameMode>(gameModeRaw);
		
		/*
		 * Server give out Network Ticks
		 */
		if (gameInstance->IsAllPlayersReady())
		{
			//PUN_DEBUG2("[InitSync] Ticking: ready:%d players:%d", gameInstance->playerReadyCount(), gameInstance->playerCount());
			
			// Add player in the beginning
			if (gameInstance->serverTick() == 0)
			{
				if (punHUD) {
					punHUD->escMenuUI()->SetLoadingText(NSLOCTEXT("PunPlayerController", "Loading_WeaveSpacetimeFabric", "Weaving spacetime fabric..."));
				}
			}

			/*
			 * Send the missing Tick if not yet sent
			 */
			for (auto it : kPlayerIdToMissingGameTick)
			{
				int32 missingTick = it.second;
				if (missingTick != -1)
				{
					_LOG(PunTickHash, "[%d] TickMissingGameTick <p%d> missingTick:%d kNetworkTickInfoCache:%d %d->%d", 
						playerId(), it.first, missingTick, kNetworkTickInfoCache.size(), kNetworkTickInfoCache.front().proxyControllerTick, kNetworkTickInfoCache.back().proxyControllerTick);
					
					// Find and Send the missing tick
					for (size_t i = 0; i < kNetworkTickInfoCache.size(); i++)
					{
						_LOG(PunTickHash, "[%d] TickMissingGameTick <p%d> i:%d tick:%d", playerId(), it.first, i, kNetworkTickInfoCache[i].proxyControllerTick);
						
						auto& curTickInfo = kNetworkTickInfoCache[i];
						if (curTickInfo.proxyControllerTick == missingTick)
						{
							// Serialize NetworkTickInfo 
							TArray<int32> networkTickInfoBlob;
							curTickInfo.SerializeToBlob(networkTickInfoBlob);

							_LOG(PunTickHash, "[%d] TickMissingGameTick <p%d> TickLocalSimulation_ToClients i:%d", playerId(), it.first, i);
							check(networkTickInfoBlob.Num() <= MaxPacketSize32);

							TickLocalSimulation_ToClients(networkTickInfoBlob);
							return;
						}
					}
					checkNoEntry();
				}
			}
			
			/*
			 * No Clog Tick the Sims of all controllers
			 */
			if (!gameInstance->shouldDelayInput() ||
				NoPlayerClogged()) 
			{
				const float serverTickDeltaTime = 1.0f / 60.0f;
				//if (DeltaTime / serverTickDeltaTime >= 2.0f) {
				//	numberOfServerTick = 2;
				//}

				int numberOfServerTick = 1;

				// DeltaTime is too slow... accumulate the leftover time to double tick later
				// This is to keep roughly 1/60 sec pace
				if (PunSettings::IsOn("FixedDeltaTime")) {
					if (DeltaTime > serverTickDeltaTime) {
						float combinedDeltaTime = DeltaTime + _leftOverDeltaTime;
						numberOfServerTick = static_cast<int>(combinedDeltaTime / serverTickDeltaTime);
						_leftOverDeltaTime = fmod(combinedDeltaTime, serverTickDeltaTime);
					}
				}

				for (int i = 0; i < numberOfServerTick; i++)
				{
					if (gameInstance->shouldDelayInput())
					{
						// serverTick runs at 60fps when there is no lag.
						// serverTick == _gameTick
						// networkTicks link between serverTick and gameTick
						// 
						if (gameInstance->serverTick() % GameTicksPerNetworkTicks == 0) // 0, 15, 30 ...
						{
							// Send tickInfo to each client
							//   serverTick isn't  used beyond this point, each controller count its own ticks
							//   beyond this, _proxyControllerTick keep track of ticks

							// Prepare NetworkTickInfo
							//  NetworkTickInfo contains information used to tick simulation including player interactions
							NetworkTickInfo tickInfo;
							tickInfo.gameSpeed = kGameSpeed;

							// Dispatch all commands (Limited to 10)
							int32 numberOfCommandsToSend = min(static_cast<int>(kCommandQueue.size()), 10);
							for (int32 j = 0; j < numberOfCommandsToSend; j++) {
								tickInfo.commands.push_back(kCommandQueue[j]);
								//PUN_DEBUG(FString::Printf(TEXT("Add tickInfo.commands init: %d"), tickInfo.commands.size()));
							}
							vector<shared_ptr<FNetworkCommand>> commands;
							for (int32 j = numberOfCommandsToSend; j < kCommandQueue.size(); j++) {
								commands.push_back(kCommandQueue[j]);
							}
							kCommandQueue = commands;

							// Cache
							if (kNetworkTickInfoCache.size() > 100) {
								kNetworkTickInfoCache.erase(kNetworkTickInfoCache.begin());
							}
							kNetworkTickInfoCache.push_back(tickInfo);

							const TArray<APunBasePlayerController*>& controllers = gameMode->ConnectedControllers();
							for (APunBasePlayerController* controller : controllers) {
								CastChecked<APunPlayerController>(controller)->SendTickToClient(tickInfo);
							}
						}
					}
					else
					{
						// Prepare NetworkTickInfo
						NetworkTickInfo tickInfo;
						tickInfo.gameSpeed = kGameSpeed;
						for (auto& command : kCommandQueue) {
							tickInfo.commands.push_back(command);
							//PUN_DEBUG(FString::Printf(TEXT("Add tickInfo.commands init: %d"), tickInfo.commands.size()));
						}
						kCommandQueue.clear();
						
						// Send each tick without the GameTicksPerNetworkTicks delay
						SendTickToClient(tickInfo);
					}
					
					gameInstance->IncrementServerTick();
				}
			}
			else {
				if (GEngine) {
					GEngine->AddOnScreenDebugMessage(-1, 0.5f, FColor::White, FString::Printf(TEXT("Clogged")));
				}
			}
		}
	}

	// Waiting For Player Load Screen
	// Applies to both server/clients
	if (gameInstance->IsAllPlayersReady()) {
		if (punHUD->escMenuUI()->LoadingScreen->GameStartBlockerBackground1->GetVisibility() != ESlateVisibility::Collapsed) {
			punHUD->escMenuUI()->HideLoadingScreen();
		}
	}
	else {
		FString waitMessage = FString::Printf(TEXT("Waiting For Players: %d / %d"), gameInstance->playerCount() - gameInstance->playerReadyCount(), gameInstance->playerCount());
		PUN_DEBUG2("[InitSync] %s", *waitMessage);
		
		if (punHUD) {
			punHUD->escMenuUI()->LoadingScreen->LoadingText->SetText(FText::FromString(waitMessage));
		}
	}

	if (_shouldGameEnd) {
		APunGameMode* gameMode = CastChecked<APunGameMode>(UGameplayStatics::GetGameMode(this));
		if (_playersReadyForGameEnd >= gameMode->connectedPlayerCount()) {
			GetWorld()->ServerTravel("/Game/Maps/VictoryScreen", TRAVEL_Absolute);
		}
	}

	/*
	 * Clients TickNetworking
	 * GameManager consume gameTickQueue as necessary
	 */
	if (gameManager) 
	{
		// Note: TickNetworking is also where init phases happens
		gameManager->TickNetworking();

		// All Client-side primaryControllers needs to inform server of clogged-status if necessary
		if (gameManager->isGameTickQueueCloggedDirty) {
			SendServerCloggedStatus(controllerPlayerId(), gameManager->isGameTickQueueClogged);
			gameManager->isGameTickQueueCloggedDirty = false;
		}
	}

	/*
	 * Display
	 */
	if (gameManager && gameManager->isGameManagerInitialized()) 
	{
		WorldAtom2 cameraAtom = WorldAtom2::Zero;
		//FVector mapCameraLocation = FVector::ZeroVector;
		//bool isMapMode = false;
		float zoomDistance = 0;
		float smoothZoomDistance = 0;
		bool isPhotoMode = false;
		bool isBounceZooming = false;
		bool isSystemMovingCamera = false;
		
		if (cameraPawn) {
			cameraAtom = cameraPawn->cameraAtom();
			//mapCameraLocation = cameraPawn->GetMapCameraLocation();
			//isMapMode = cameraPawn->isMapMode();
			zoomDistance = cameraPawn->zoomDistance();
			smoothZoomDistance = cameraPawn->smoothZoomDistance();
			
			isPhotoMode = cameraPawn->isPhotoMode;
			isBounceZooming = cameraPawn->isBounceZooming();// TODO: remove??
		}
		
		gameManager->TickDisplay(DeltaTime, cameraAtom, zoomDistance, smoothZoomDistance, cameraPawn->CameraComponent->GetComponentLocation(), isPhotoMode, isBounceZooming);

		// Tick UI
		// (After display to prevent flash)
		if (punHUD) {
			//PUN_CHECK(gameManager->assetLoader()->moduleNames().Num() == ModuleMeshCount);
			punHUD->PunTick(isPhotoMode);
		}

		// Finalize UI Meshes
		gameManager->UIMeshAfterAdd();


		// If gameManager just initialize this player, go to the spawn point
		PlayerOwnedManager& playerOwned = gameManager->simulation().playerOwned(playerId());


		if (cameraPawn && playerOwned.justChoseLocation) {
			WorldAtom2 lookAtAtom = gameManager->simulation().homeAtom(playerId());

			if (!PunSettings::TrailerMode()) {
				cameraPawn->MoveCameraTo(lookAtAtom, 350.0f);
			}
			
			playerOwned.justChoseLocation = false;
		}
	}

	// Trailer Mode has no mouse
	bShowMouseCursor = !PunSettings::TrailerMode();
	

	// Inputs
	if (cameraPawn) {
		MouseInfo mouseInfo;
		mouseInfo.isValid = DeprojectMousePositionToWorld(mouseInfo.mouseLocation, mouseInfo.mouseDirection);

		cameraPawn->TickInputSystem(gameManager, DeltaTime, mouseInfo);
	}

	/*
	 * Settings update
	 */
	if (gameInstance->needSettingsUpdate) 
	{
		if (gameManager && gameManager->isGameManagerInitialized())  {
			gameInstance->needSettingsUpdate = false;

			// TODO: remove this??
		}

		if (cameraPawn) {
			cameraPawn->zoomSpeedFraction = gameInstance->mouseZoomSpeedFraction;
			cameraPawn->mouseRotateSpeedFraction = gameInstance->mouseRotateSpeedFraction;
		}
	}

	/*
	 * SaveCheck
	 */
#if WITH_EDITOR
	SaveCheckTick();
#endif
}

int32 APunPlayerController::GetPlayersBeyondLoadStage(ClientLoadStage stageIn)
{
	LLM_SCOPE_(EPunSimLLMTag::PUN_Controller);
	
	APunGameMode* gameMode = CastChecked<APunGameMode>(UGameplayStatics::GetGameMode(this));
	TArray<APunBasePlayerController*> controllers = gameMode->ConnectedControllers();

	int32 count = 0;
	for (int32 i = 0; i < controllers.Num(); i++) {
		if (controllers[i]) {
			ClientLoadStage stage = CastChecked<APunPlayerController>(controllers[i])->_clientLoadStage;
			if (static_cast<int>(stage) >= static_cast<int>(stageIn)) {
				count++;
			}
		}
	}
	return count;
}

// Called on server-side playerControllers to tick the simulations
void APunPlayerController::SendTickToClient(NetworkTickInfo& tickInfo)
{
	LLM_SCOPE_(EPunSimLLMTag::PUN_Controller);

	//PUN_LOG("SendTickToClient cid:%d _proxyControllerTick %d", controllerPlayerId(), _proxyControllerTick);
	
	tickInfo.proxyControllerTick = _proxyControllerTick++;


	for (int32 j = 0; j < tickInfo.commands.size(); j++) {
		tickInfo.commands[j]->proxyControllerTick = tickInfo.proxyControllerTick;
	}


	// Serialize NetworkTickInfo 
	TArray<int32> networkTickInfoBlob;
	tickInfo.SerializeToBlob(networkTickInfoBlob);

	//PUN_DEBUG(FString::Printf(TEXT("Send Tick:%d"), tickInfo.tickCount));

	if (tickInfo.hasCommand()) {
		PUN_DEBUG(FString::Printf(TEXT("networkTickInfoBlob init: blobSize:%d commands:%d"), networkTickInfoBlob.Num(), tickInfo.commands.size()));
	}
	//PUN_DEBUG(FString::Printf(TEXT("Server Add Tick:%d queue:%d"), tickInfo.tickCount, gameManager->networkTickQueueCount()));

	check(networkTickInfoBlob.Num() <= MaxPacketSize32);

	// TickSimulations
	if (gameInstance()->shouldDelayInput()) {
		TickLocalSimulation_ToClients(networkTickInfoBlob);
	} else {
		TickLocalSimulation_Base(tickInfo);
	}
}

/**
 * IGameNetworkInterface
 */

void APunPlayerController::SendNetworkCommand(std::shared_ptr<FNetworkCommand> networkCommand)
{
	LLM_SCOPE_(EPunSimLLMTag::PUN_Controller);
	
	// Don't send command if this controller is not initialized
	if (gameManager == nullptr) return;

	if (networkCommand->playerId == -1) {
		networkCommand->playerId = controllerPlayerId();
	}

	PunSerializedData blob(true);
	NetworkHelper::SerializeAndAppendToBlob(networkCommand, blob);

	//PUN_DEBUG(FString::Printf(TEXT("serializedCommand init: %d"), blob.Num()));
	check(blob.Num() <= MaxPacketSize32);

	if (gameInstance()->shouldDelayInput()) {
		ServerSendNetworkCommand_ToServer(blob);
	} else {
		ServerSendNetworkCommand_Base(blob);
	}
}

void APunPlayerController::SetGameSpeed(int32 gameSpeed)
{
	kResumeGameSpeed = gameSpeed;
	kGameSpeed = gameSpeed;
}

void APunPlayerController::Pause() {
	kGameSpeed = 0;
	PUN_LOG("Pause %d", kResumeGameSpeed);
}
void APunPlayerController::Resume() {
	kGameSpeed = kResumeGameSpeed;
	PUN_LOG("Resume %d", kResumeGameSpeed);
}

/** 
 * Networking 
 */

void APunPlayerController::InitializeClient_ToClient_Implementation()
{
	LLM_SCOPE_(EPunSimLLMTag::PUN_Controller);
	
	PUN_DEBUG2("[InitSync] InitializeClient pid:%d", controllerPlayerId());

	InitializeClientCallback();

	// Start loading the client
	if (GetLocalRole() == ROLE_AutonomousProxy) {
		LoadController_Prepare();
	}
}

void APunPlayerController::InitializeClientCallback_Implementation()
{
	LLM_SCOPE_(EPunSimLLMTag::PUN_Controller);
	
	PUN_DEBUG2("[InitSync] InitializeClient Callback pid:%d", controllerPlayerId());

	int32 controllerId = UGameplayStatics::GetPlayerControllerID(this);
	if (controllerId != 0) {
		_clientLoadStage = ClientLoadStage::ClientLoading;
	}
}

void APunPlayerController::LoadedClientCallback_ToServer_Implementation()
{
	LLM_SCOPE_(EPunSimLLMTag::PUN_Controller);
	
	_clientLoadStage = ClientLoadStage::Completed;

	gameInstance()->SetPlayerReady(controllerPlayerId(), true);
	
	APunGameMode* gameMode = CastChecked<APunGameMode>(UGameplayStatics::GetGameMode(this));
	gameMode->Server_SyncPlayerStateToAllControllers();

	PUN_DEBUG2("[InitSync] LoadedClient Callback pid:%d allready:%d", controllerPlayerId(), gameInstance()->IsAllPlayersReady());
}






//! Send current networkTick buffer count and inputs to server
void APunPlayerController::SendServerCloggedStatus_Implementation(int32 playerId, bool clogStatus)
{
	//PUN_DEBUG(FString::Printf(TEXT("Imp SendServerTickStatus: pid:%d bufferCount:%d"), playerId, networkTickBufferCount));
	kPlayerIdToClogStatus[playerId] = clogStatus;
}

void APunPlayerController::SendServerMissingTick_Implementation(int32 playerId, int32 missingTick)
{
	PUN_LOG("Receive SendServerMissingTick <p:%d> missingTick:%d", playerId, missingTick);
	
	kPlayerIdToMissingGameTick[playerId] = missingTick;
}

void APunPlayerController::SendHash_ToClient_Implementation(int32 hashSendTick, const TArray<int32>& allTickHashes)
{
	LLM_SCOPE_(EPunSimLLMTag::PUN_Controller);
	
	_LOG(PunTickHash, "SendHash Client receive hashSendTick:%d hash32Size:%d hashTickBlockSize:%d", hashSendTick, allTickHashes.Num(), allTickHashes.Num() / TickHashes::TickHashEnumCount());
	check(allTickHashes.Num() <= MaxPacketSize32);

	gameManager->simulation().AppendAndCompareServerHashes(hashSendTick, allTickHashes);
}





//! Tick Everyone
void APunPlayerController::TickLocalSimulation_Base(const NetworkTickInfo& tickInfo)
{
	LLM_SCOPE_(EPunSimLLMTag::PUN_Controller);

	if (!gameManager) {
		return;
	}

	//PUN_LOG("Tick: controller->TickLocalSimulation_Base() tickCount:%d gameSpeed:%d tickCountSim:%d", tickInfo.tickCount, tickInfo.gameSpeed, tickInfo.tickCountSim);

	// Discard previous tick (might be resent)
	if (tickInfo.proxyControllerTick - _lastNetworkTickCount <= 0) {
		return;
	}

	// Tick was skipped
	if (tickInfo.proxyControllerTick - _lastNetworkTickCount > 1)
	{
		PUN_LOG("Tick was skipped tick:%d last:%d", tickInfo.proxyControllerTick, _lastNetworkTickCount);
		SendServerMissingTick(controllerPlayerId(), _lastNetworkTickCount + 1);
		_blockedNetworkTickInfoList.push_back(tickInfo);
		return;
	}

	_lastNetworkTickCount = tickInfo.proxyControllerTick;
	AddTickInfo(tickInfo);

	if (_blockedNetworkTickInfoList.size() > 0)
	{
		for (int32 i = 0; i < _blockedNetworkTickInfoList.size(); i++) 
		{
			auto& curTickInfo = _blockedNetworkTickInfoList[i];
			if (curTickInfo.proxyControllerTick - _lastNetworkTickCount == 1) {
				_lastNetworkTickCount = curTickInfo.proxyControllerTick;
				AddTickInfo(curTickInfo);
			}
			else if (curTickInfo.proxyControllerTick - _lastNetworkTickCount <= 0) {
				continue;
			}
			else {
				PUN_LOG("Tick was skipped (2) tick:%d last:%d", curTickInfo.proxyControllerTick, _lastNetworkTickCount);
				SendServerMissingTick(controllerPlayerId(), _lastNetworkTickCount + 1);
				_blockedNetworkTickInfoList = std::vector<NetworkTickInfo>(_blockedNetworkTickInfoList.begin() + i, _blockedNetworkTickInfoList.end());
				return;
			}
		}

		// This was blocked and just got unlocked
		SendServerMissingTick(controllerPlayerId(), -1);
	}
}

void APunPlayerController::AddTickInfo(const NetworkTickInfo& tickInfo)
{
	//PUN_DEBUG(FString::Printf(TEXT("Add Tick:%d"), tickInfo.tickCount));

	// Server ticks comes at 5fps while game tick is 60fps
	// server_tick is the server playerController's tick
	// network_tick is a batch of ticks that gets send across the network
	// game_tick is broken down network_ticks used to tick client's sim
	// sim_tick is game_tick with game_speed
	// 
	// X server_tick = 1 network_tick
	// 1 network_tick = X game_ticks
	// Note: game_ticks = sim_ticks * game_speed
	// Add tick-1 info with input commands
	gameManager->AddGameTick(tickInfo);

	if (gameInstance()->shouldDelayInput()) // Multiplayer does multiple ticks at once (GameTicksPerNetworkTicks)
	{
		// Add (GameTicksPerNetworkTicks - 1) more empty ticks
		int tickCount = tickInfo.proxyControllerTick;
		const int32 moreTicksToAdd = GameTicksPerNetworkTicks - 1;

		for (int i = 0; i < moreTicksToAdd; i++) {
			NetworkTickInfo tickInfoNoCommand;
			tickInfoNoCommand.proxyControllerTick = ++tickCount; // since game ticks at 30fps the tickCount sent is serverTick / 2
			//tickInfoNoCommand.playerCount = tickInfo.playerCount;
			tickInfoNoCommand.gameSpeed = tickInfo.gameSpeed;
			gameManager->AddGameTick(tickInfoNoCommand);
		}
	}
	
}

void APunPlayerController::TickLocalSimulation_ToClients_Implementation(const TArray<int32>& networkTickInfoBlob)
{
	NetworkTickInfo tickInfo;
	tickInfo.DeserializeFromBlob(networkTickInfoBlob);

	if (tickInfo.hasCommand()) { // Check if this is comand tick
		PUN_LOG("ClientTickLocalSimulation..TickInfo: size:%d tickCount: %d, commandSize:%d", networkTickInfoBlob.Num(), tickInfo.proxyControllerTick, tickInfo.commands.size());
	}
	
	TickLocalSimulation_Base(tickInfo);
}
bool APunPlayerController::TickLocalSimulation_ToClients_Validate(const TArray<int32>& networkTickInfoBlob) { return true; }


//! Send NetworkCommand to server to be added to a tick
void APunPlayerController::ServerSendNetworkCommand_Base(const TArray<int32>& serializedCommand)
{
	LLM_SCOPE_(EPunSimLLMTag::PUN_Controller);

	PunSerializedData punBlob(false, serializedCommand);
	shared_ptr<FNetworkCommand> command = NetworkHelper::DeserializeFromBlob(punBlob);

	PUN_DEBUG(FString::Printf(TEXT("ServerSendNetworkCommand serializedCommand: type:%d num:%d"), (int)command->commandType(), serializedCommand.Num()));

	kCommandQueue.push_back(command);
}
void APunPlayerController::ServerSendNetworkCommand_ToServer_Implementation(const TArray<int32>& serializedCommand)
{
	ServerSendNetworkCommand_Base(serializedCommand);
}
bool APunPlayerController::ServerSendNetworkCommand_ToServer_Validate(const TArray<int32>& serializedCommand) { return true; }


void APunPlayerController::GamePause_ToServer_Implementation() {
	Pause();
}


void APunPlayerController::CompareUnitHashes_ToClient_Implementation(int32 startIndex, const TArray<int32>& serverHashes)
{
#if CHECK_TICKHASH
	std::vector<int32> localHashes = simulation().unitSystem().GetUnitSyncHashes();
	for (int32 i = 0; i < serverHashes.Num(); i++) {
		if (localHashes[startIndex + i] != serverHashes[i]) {
			simulation().AddPopup(playerId(), FText::Format(
				INVTEXT("Unit {0} Hash Compare Failed localHash:{1} serverHash:{2}"), 
				TEXT_NUM(i), TEXT_NUM(localHashes[startIndex + i]), TEXT_NUM(serverHashes[i])
			));
		}
	}

	simulation().AddPopup(playerId(), FText::Format(INVTEXT("Hash Compare Ended startIndex:{0}"), TEXT_NUM(startIndex)));
#endif
}
void APunPlayerController::CompareUnitHashes()
{
#if CHECK_TICKHASH
	if (APunGameMode* gameMode = Cast<APunGameMode>(UGameplayStatics::GetGameMode(this)))
	{
		const TArray<APunBasePlayerController*>& controllers = gameMode->ConnectedControllers();
		for (APunBasePlayerController* controller : controllers) 
		{
			std::vector<int32> serverHashes = simulation().unitSystem().GetUnitSyncHashes();
			for (int32 i = 0; i < serverHashes.size(); i += MaxPacketSize32)
			{
				TArray<int32> packetHashes;
				for (int32 j = i; j < i + MaxPacketSize32; j++) {
					if (j >= serverHashes.size()) {
						break;
					}
					packetHashes.Add(serverHashes[j]);
				}

				CastChecked<APunPlayerController>(controller)->CompareUnitHashes_ToClient(i, packetHashes);
			}
		}
	}
#endif
}

void APunPlayerController::SendResourceHashes_ToClient_Implementation(int32 startIndex, const TArray<int32>& serverHashes)
{
	PUN_LOG("Receive: SendResourceHashes_ToClient startIndex:%d serverHashes:%d", startIndex, serverHashes.Num());

	tempServerHashes.Append(serverHashes);
}
void APunPlayerController::CompareResourceHashes_ToClient_Implementation()
{
#if CHECK_TICKHASH
	int32 currentIndex = 0;
	int32 curPlayerId = tempServerHashes[currentIndex++];
	simulation().resourceSystem(curPlayerId).FindDesyncInResourceSyncHashes(tempServerHashes, currentIndex);
	tempServerHashes.SetNum(0);

	PUN_LOG("Receive: CompareResourceHashes_ToClient pid:%d", curPlayerId);
	
	simulation().AddPopup(curPlayerId, INVTEXT("Resource Hash Compare Ended"));
#endif
}
void APunPlayerController::CompareResourceHashes()
{
#if CHECK_TICKHASH
	if (APunGameMode* gameMode = Cast<APunGameMode>(UGameplayStatics::GetGameMode(this)))
	{
		const TArray<APunBasePlayerController*>& controllers = gameMode->ConnectedControllers();
		for (APunBasePlayerController* controller : controllers)
		{
			simulation().ExecuteOnPlayersAndAI([&](int32 playerId)
			{
				std::vector<int32> serverHashes = simulation().resourceSystem(playerId).GetResourcesSyncHashes();
				check(serverHashes.size() > 0);
				
				for (int32 i = 0; i < serverHashes.size(); i += MaxPacketSize32)
				{
					TArray<int32> packetHashes;
					for (int32 j = i; j < i + MaxPacketSize32; j++) {
						if (j >= serverHashes.size()) {
							break;
						}
						packetHashes.Add(serverHashes[j]);
					}

					PUN_LOG("Send: SendResourceHashes_ToClient startIndex:%d serverHashes:%d", i, packetHashes.Num());
					CastChecked<APunPlayerController>(controller)->SendResourceHashes_ToClient(i, packetHashes);
				}

				PUN_LOG("Send: CompareResourceHashes_ToClient");
				CastChecked<APunPlayerController>(controller)->CompareResourceHashes_ToClient();
			});
		}
	}
#endif
}

//! Send NetworkCommand to server to be added to a tick
void APunPlayerController::ToServer_SavedGameEndStatus_Implementation(int32 playerId)
{
	_playersReadyForGameEnd++;
}
bool APunPlayerController::ToServer_SavedGameEndStatus_Validate(int32 playerId) { return true; }


/**
 * Console
 */

void APunPlayerController::PlayerId()
{
	PUN_DEBUG(FString::Printf(TEXT("Player ID: %d"), controllerPlayerId()));
}


void APunPlayerController::TileInfo(int32 x, int32 y)
{
	LLM_SCOPE_(EPunSimLLMTag::PUN_Controller);
	
	PUN_LOG("Tile: %d, %d", x, y);
	if (gameManager) {
		int32_t id = WorldTile2(x, y).tileId();
		auto& treeSystem = gameManager->simulation().treeSystem();
		PUN_LOG("TreeEnum: %d", (int)treeSystem.tileObjEnum(id));
		PUN_LOG("TreeAge: %d", treeSystem.tileObjAge(id));
	}
}


void APunPlayerController::TileConnected(int32 startX, int32 startY, int32 endX, int32 endY, int32 maxRegionDist)
{
	WorldTile2 start(startX, startY);
	WorldTile2 end(endX, endY);
	if (!start.isValid() || !end.isValid()) {
		PUN_DEBUG(FString("Tile Invalid"));
	}
	if (gameManager->simulation().IsConnected(start, end, maxRegionDist)) {
		PUN_DEBUG(FString("IsConnected True"));
	} else {
		PUN_DEBUG(FString("IsConnected False"));
	}
}

void APunPlayerController::Cheat(const FString& cheatName)
{
	auto command = make_shared<FCheat>();
	command->cheatEnum = GetCheatEnum(cheatName);
	SendNetworkCommand(command);
}


void APunPlayerController::ShowUnit(const FString& unitName)
{
	int unitId = FCString::Atoi(*unitName);
	auto& unitSystem = gameManager->simulation().unitSystem();
	if (unitId >= 0 && unitId < unitSystem.unitCount()) {
		WorldAtom2 lookAtAtom = unitSystem.atomLocation(unitId);
		cameraPawn->SetCameraAtom(lookAtAtom);
	}
}

void APunPlayerController::AnimalCount()
{
	auto& unitSystem = gameManager->simulation().unitSystem();

#if !WITH_EDITOR
	GEngine->bEnableOnScreenDebugMessages = true;
#endif
	PUN_DEBUG2("Unit Count:%d", unitSystem.unitCount());
#if !WITH_EDITOR
	GEngine->bEnableOnScreenDebugMessages = false;
#endif
}
void APunPlayerController::AddAnimals(int32 unitCount)
{
	auto& unitSystem = gameManager->simulation().unitSystem();
	unitSystem.AddAnimals(unitCount);
}

void APunPlayerController::BushCount()
{
	auto& treeSystem = gameManager->simulation().treeSystem();

	PUN_DEBUG2("Bush Count:%d", treeSystem.bushCount());
}


void APunPlayerController::OpenTradeUI()
{
	GetPunHUD()->OpenTradeUI(0);
}

void APunPlayerController::OpenRareCardUI()
{
	gameManager->simulation().GenerateRareCardSelection(GetPunHUD()->playerId(), RareHandEnum::RareCards, INVTEXT("CMD"));
}

void APunPlayerController::SavePlayerActions(int32 playerId, const FString& fileName)
{
	gameManager->simulation().replaySystem().SavePlayerActions(playerId, fileName);
}

void APunPlayerController::SaveCheck()
{
	_LOG(PunSaveCheck, "Start SaveCheck");
	
	auto gameInstance = CastChecked<UPunGameInstance>(GetGameInstance());
	auto& sim = gameManager->simulation();
	
	GameSaveInfo saveInfo = gameInstance->saveSystem().SaveDataToFile(FString("TestSave"));
	gameInstance->saveCheckInfo = saveInfo;

	// Start save check...
	sim.saveCheckStartTick = Time::Ticks();
	sim.saveCheckState = SaveCheckState::SerializeBeforeSave;
}

void APunPlayerController::SaveCheckTick()
{
	auto gameInstance = Cast<UPunGameInstance>(GetGameInstance());
	if (gameManager == nullptr || !gameManager->HasSimulation()) {
		return;
	}
	
	auto& sim = gameManager->simulation();

	if (sim.saveCheckState == SaveCheckState::SerializeBeforeSave_Done) 
	{
		_LOG(PunSaveCheck, "SerializeBeforeSave_Done");
		
		sim.saveCheckState = SaveCheckState::None;
		gameInstance->saveCheckTransitioning = true;

		PunFileUtils::SaveFile("SaveCheckCRC", [&](FArchive& Ar) {
			SerializeVecVecValue(Ar, sim.saveCheckCrcsToTick);
		});

		gameInstance->SetSavedGameToLoad(gameInstance->saveCheckInfo);
		GetWorld()->ServerTravel("/Game/Maps/Lobby?listen");
	}
	else if (sim.saveCheckState == SaveCheckState::SerializeAfterSave_Done) 
	{
		_LOG(PunSaveCheck, "SerializeAfterSave_Done");
		
		sim.saveCheckState = SaveCheckState::None;

		PUN_CHECK(sim.saveCheckCrcsToTick.size() > 0);

		std::vector<std::vector<int32>> loadedCrcsToTicks;
		PunFileUtils::LoadFile("SaveCheckCRC", [&](FArchive& Ar) {
			SerializeVecVecValue(Ar, loadedCrcsToTicks);
		});

		// Do the actual check...
		_LOG(PunSaveCheck, "SaveCheck!");
		PUN_CHECK(loadedCrcsToTicks.size() == sim.saveCheckCrcsToTick.size());
		for (size_t i = 0; i < loadedCrcsToTicks.size(); i++) {
			for (size_t j = 0; j < sim.saveCheckCrcLabels.size(); j++) {
				std::string label = sim.saveCheckCrcLabels[j];
				_LOG(PunSaveCheck, "Check %s %d == %d ", ToTChar(label), loadedCrcsToTicks[i][j], sim.saveCheckCrcsToTick[i][j]);
				PUN_CHECK2(loadedCrcsToTicks[i][j] == sim.saveCheckCrcsToTick[i][j], "SaveCheck Failed: " + label);
			}
		}
		_LOG(PunSaveCheck, "SaveCheck - Completed!!!");
	}
}