// Fill out your copyright notice in the Description page of Project Settings.

#include "PunCity/MainMenuPlayerController.h"
#include "UnrealEngine.h"
#include "Engine/World.h"
#include "Components/StaticMeshComponent.h"
#include "Kismet/GameplayStatics.h"
#include "Engine/StaticMeshActor.h"
#include "UObject/ConstructorHelpers.h"
#include "Net/UnrealNetwork.h"

#include "PunCity/UI/LobbyUI.h"
#include "PunCity/UI/MainMenuUI.h"
#include "PunCity/PunUtils.h"
#include <locale>
#include "PunCity/PunPlayerController.h"
#include "PunCity/UI/PunMainMenuHUD.h"
#include "PunCity/UI/PunLobbyHUD.h"
#include "PunCity/PunGameMode.h"

#define LOCTEXT_NAMESPACE "MainMenuPlayerController"

AMainMenuPlayerController::AMainMenuPlayerController() : APunBasePlayerController()
{
	PrimaryActorTick.bCanEverTick = true;
	bAllowTickBeforeBeginPlay = false;

	// Set player controller's statuses
	bShowMouseCursor = true;
	bEnableClickEvents = true;
	bEnableMouseOverEvents = true;
	FViewTargetTransitionParams params;
	SetViewTarget(this, params);

	isStartingGame = false;
	initStage = 1;
	
	//CameraComponent = CreateDefaultSubobject<UCameraComponent>(TEXT("GameCamera"));
	//CameraComponent->SetupAttachment(RootComponent);
}

void AMainMenuPlayerController::BeginPlay()
{
	Super::BeginPlay();
	isStartingGame = false;

	SetupCameraPawn();

	initStage = 2;

	_LOG(PunInit, "MainMenuPlayerController Init RefreshCulture");
	gameInstance()->RefreshCulture();
}


void AMainMenuPlayerController::Tick(float DeltaTime)
{
	APunBasePlayerController::Tick(DeltaTime);
	
	// TODO: Delaying SetupDisplayManager may prevent startup crash??
	if (initStage == 2) 
	{
		SetupDisplayManager();


		auto hud = GetHUD();
		if (hud) {
			auto mainMenuHUD = Cast<APunMainMenuHUD>(hud);
			if (mainMenuHUD) {
				FInputModeGameAndUI_Pun inputModeData;
				inputModeData.SetWidgetToFocus(mainMenuHUD->mainMenuUI()->TakeWidget());
				inputModeData.SetLockMouseToViewportBehavior(EMouseLockMode::DoNotLock);
				SetInputMode(inputModeData);
			}

			auto lobbyHUD = Cast<APunLobbyHUD>(hud);
			if (lobbyHUD) {
				FInputModeGameAndUI_Pun inputModeData;
				inputModeData.SetWidgetToFocus(lobbyHUD->lobbyUI()->TakeWidget());
				inputModeData.SetLockMouseToViewportBehavior(EMouseLockMode::DoNotLock);
				SetInputMode(inputModeData);
			}
		}
		
		initStage = 3;
	}

	
	if (PunSettings::bShouldRefreshMainMenuDisplay) {
		PunSettings::bShouldRefreshMainMenuDisplay = false;
		
		mainMenuDisplayManager->UpdateDisplay(cameraState.cameraAtom, cameraState.zoomDistance, cameraState.sampleRegionIds, true, false);
	}

	if (mainMenuDisplayManager) {
		mainMenuDisplayManager->UpdatePostProcessVolume();
	}
}

bool AMainMenuPlayerController::IsLobbyUIOpened()
{
	auto lobbyHUD = Cast<APunLobbyHUD>(GetWorld()->GetFirstPlayerController()->GetHUD());
	return lobbyHUD != nullptr;
}

void AMainMenuPlayerController::ToggleMainMenu()
{
	auto hud = GetHUD();
	if (hud) {
		auto mainMenuHUD = Cast<APunMainMenuHUD>(hud);
		if (mainMenuHUD) {
			auto mainMenu = mainMenuHUD->mainMenuUI();
			mainMenu->SetVisibility(mainMenu->GetVisibility() == ESlateVisibility::Collapsed ? ESlateVisibility::Visible : ESlateVisibility::Collapsed);
		}
	}
}

void AMainMenuPlayerController::SetupCameraPawn()
{
	TArray<AActor*> FoundCameraPawn;
	UGameplayStatics::GetAllActorsOfClass(GetWorld(), ACameraPawn::StaticClass(), FoundCameraPawn);
	if (FoundCameraPawn.Num() > 0) {
		cameraPawn = Cast<ACameraPawn>(FoundCameraPawn[0]);
	}
	else {
		//UE_DEBUG_BREAK();
		cameraPawn = GetWorld()->SpawnActor<ACameraPawn>(ACameraPawn::StaticClass());
	}
	cameraPawn->Controller = this; // Need this for the restart to work
	cameraPawn->isMainMenuDisplay = true;
	cameraPawn->PawnClientRestart(); // PawnClientRestart help fix SetActorLocation not working problem

	/**
	 * Possess Pawn
	 */
	if (GetPawn() && GetPawn() != cameraPawn) {
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
	 //cameraPawn->SetActorLocationAndRotation(FVector::ZeroVector, FRotator(0.0f, 0.0f, 0.0f));
	cameraPawn->camera()->SetRelativeLocation(FVector(-300.0f, 0.0f, 100.0f));
	//cameraPawn->camera()->SetRelativeLocation(FVector::ZeroVector);
}

void AMainMenuPlayerController::SetupDisplayManager()
{
	TArray<AActor*> FoundGameManagers;
	UGameplayStatics::GetAllActorsOfClass(GetWorld(), AMainMenuDisplayManager::StaticClass(), FoundGameManagers);
	if (FoundGameManagers.Num() > 0) {
		PUN_LOG("Got GameManager %p", FoundGameManagers[0]);

		mainMenuDisplayManager = Cast<AMainMenuDisplayManager>(FoundGameManagers[0]);

		// Display the main menu scene
		if (mainMenuDisplayManager)
		{
			_LOG(PunSaveLoad, "AMainMenuPlayerController::SetupDisplayManager");
			
			MainMenuCameraState cameraStateLocal;
			MainMenuDisplaySaveSystem::LoadCameraOnly(cameraStateLocal);
			
			mainMenuDisplayManager->SetOwner(this);
			mainMenuDisplayManager->InitMainMenuDisplayManager(cameraStateLocal.mapSizeEnum);

			bool succeed = MainMenuDisplaySaveSystem::SaveOrLoad(mainMenuDisplayManager->simulation(), cameraState, false);

			if (succeed) 
			{
				_LOG(PunSaveLoad, "AMainMenuPlayerController Load MainMenuDisplayCache Success");
				
				FTransform transform = cameraState.worldTransform;
				transform.SetTranslation(transform.GetTranslation() + FVector(60, -20, 0));
				cameraPawn->CameraComponent->SetComponentToWorld(transform);

				// No tundra??

				// Make all farms fully grown...
				auto& simulation = mainMenuDisplayManager->simulation();
				auto& treeSystem = simulation.treeSystem();
				const auto& buildingSubregionList = simulation.buildingSubregionList();
				for (int32 sampleId : cameraState.sampleRegionIds) 
				{
					buildingSubregionList.ExecuteRegion(WorldRegion2(sampleId), [&](int32 buildingId) {
						Building& building = simulation.building(buildingId);
						if (building.isEnum(CardEnum::Farm)) {
							building.area().ExecuteOnArea_WorldTile2([&](WorldTile2 tile)
							{
								treeSystem.PlantFullTileObj(tile.tileId(), TileObjEnum::WheatBush);
							});
						}
					});
				}

				// Hardcode to small...
				WorldRegion2 mapSize = GetMapSize(MapSizeEnum::Medium);
				mainMenuDisplayManager->assetLoader()->SetMaterialCollectionParametersScalar("TotalRegionX", mapSize.x);
				mainMenuDisplayManager->assetLoader()->SetMaterialCollectionParametersScalar("TotalRegionY", mapSize.y);

				mainMenuDisplayManager->UpdateDisplay(cameraState.cameraAtom, cameraState.zoomDistance, cameraState.sampleRegionIds, true, true);
			}
			else {
				mainMenuDisplayManager->failedToLoadMainMenuJson = true;
				_LOG(PunSaveLoad, "AMainMenuPlayerController Load MainMenuDisplayCache Failed");
			}
		}
	}
	else {
		PUN_LOG("No GameManager");
	}
}


void AMainMenuPlayerController::SendReadyStatus_ToServer_Implementation(bool playerReadyState)
{
	if (ShouldSkipLobbyNetworkCommand()) {
		return;
	}
	
	gameInstance()->SetPlayerReady(controllerPlayerId(), playerReadyState);

	auto gameMode = CastChecked<APunGameMode>(UGameplayStatics::GetGameMode(this));
	gameMode->Server_SyncPlayerStateToAllControllers();
}



void AMainMenuPlayerController::SetMapSettings_Implementation(const TArray<int32>& mapSettingsBlob)
{
	if (ShouldSkipLobbyNetworkCommand()) {
		return;
	}
	
	gameInstance()->SetMapSettings(mapSettingsBlob);

	//PUN_DEBUG2("SetMapSettings_Impl %s", ToTChar(gameInstance()->GetMapSettings().ToString()));
}
bool AMainMenuPlayerController::SetMapSettings_Validate(const TArray<int32>& mapSettingsBlob) { return true; }

void AMainMenuPlayerController::ServerStartGame_Implementation(const TArray<int32>& mapSettingsBlob)
{
	if (ShouldSkipLobbyNetworkCommand()) {
		return;
	}
	
	isStartingGame = true;
	
	gameInstance()->SetMapSettings(mapSettingsBlob);

	PUN_DEBUG2("ServerStartGame_Impl %s", ToTChar(gameInstance()->GetMapSettings().ToString()));
}


void AMainMenuPlayerController::SendChat_ToServer_Implementation(const FString& playerName, const FString& message)
{
	PUN_DEBUG2("SendChat_ToServer_Impl %s, %s", *playerName, *message);
	if (ShouldSkipLobbyNetworkCommand()) {
		return;
	}

	ExecuteAllMainMenuControllers([&](AMainMenuPlayerController* controller) {
		controller->SendChat_ToClient(playerName, message);
	});
}

void AMainMenuPlayerController::SendChat_ToClient_Implementation(const FString& playerName, const FString& message)
{
	PUN_DEBUG2("SendChat_ToClient_Impl %s, %s", *playerName, *message);
	if (ShouldSkipLobbyNetworkCommand()) {
		return;
	}
	
	gameInstance()->lobbyChatDirty = true;
	gameInstance()->lobbyChatPlayerNames.Add(playerName);
	gameInstance()->lobbyChatMessages.Add(message);
}


void AMainMenuPlayerController::Client_GotKicked_Implementation()
{
	if (ShouldSkipLobbyNetworkCommand()) {
		return;
	}
	
	UPunGameInstance* gameInst = Cast<UPunGameInstance>(GetWorld()->GetGameInstance());
	if (gameInst)
	{
		// Show message
		gameInst->mainMenuPopup = LOCTEXT("KickedFromGame", "You were kicked from the game.");
		
		gameInstance()->isReturningToLobbyList = true;
		gameInstance()->EnsureSessionDestroyed(false);
		GetWorld()->GetFirstPlayerController()->ClientTravel("/Game/Maps/MainMenu", TRAVEL_Absolute);
	}
}
bool AMainMenuPlayerController::Client_GotKicked_Validate() { return true; }


#undef LOCTEXT_NAMESPACE