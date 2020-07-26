#pragma once

#include "UnrealEngine.h"
#include "CameraPawn.h"
#include "GameManager.h"

#include "PunCity/NetworkStructs.h"
#include "PunCity/GameNetworkInterface.h"
#include "PunCity/UI/PunHUD.h"
#include "PunCity/GameSaveSystem.h"
#include "Framework/Application/IInputProcessor.h"

#include <unordered_map>
#include <functional>
#include "PunCity/Simulation/MainMenuDisplaySaveSystem.h"
#include "PunBasePlayerController.h"

struct NetworkCommand;

struct ChangeNameInfo
{
	int32 type;
	int32 objectId;
	FString name;
	ChangeNameInfo(int32 type, int32 objectId, FString name) : type(type), objectId(objectId), name(name) {}
};

#include "GameFramework/PlayerController.h"

struct FInputModeGameAndUI_Pun : public FInputModeDataBase
{
	/** Widget to focus */
	FInputModeGameAndUI_Pun& SetWidgetToFocus(TSharedPtr<SWidget> InWidgetToFocus) { WidgetToFocus = InWidgetToFocus; return *this; }

	/** Sets the mouse locking behavior of the viewport */
	FInputModeGameAndUI_Pun& SetLockMouseToViewportBehavior(EMouseLockMode InMouseLockMode) { MouseLockMode = InMouseLockMode; return *this; }

	/** Whether to hide the cursor during temporary mouse capture caused by a mouse down */
	FInputModeGameAndUI_Pun& SetHideCursorDuringCapture(bool InHideCursorDuringCapture) { bHideCursorDuringCapture = InHideCursorDuringCapture; return *this; }

	FInputModeGameAndUI_Pun()
		: WidgetToFocus()
		, MouseLockMode(EMouseLockMode::DoNotLock)
		, bHideCursorDuringCapture(false)
	{}

	bool isGameOnly = false;

protected:

	TSharedPtr<SWidget> WidgetToFocus;
	EMouseLockMode MouseLockMode;
	bool bHideCursorDuringCapture;

	virtual void ApplyInputMode(FReply& SlateOperations, class UGameViewportClient& GameViewportClient) const override;
};

enum class ServerLoadStage : uint8
{
	NotStarted,
	WaitingForInitCallbacks,
	ServerLoading,
	WaitingForLoadedCallbacks,
	Completed,
};

enum class ClientLoadStage : uint8
{
	NotStarted,
	ClientLoading,
	Completed,
};

enum class LoadStepEnum : uint8
{
	NotStarted,
	GameManagerStart,
	GameManagerInProgress,
	Completed,
};

class FPunInputProcessor : virtual public IInputProcessor
{
public:
	FPunInputProcessor() {}
	virtual ~FPunInputProcessor() {}

	void Tick(const float DeltaTime, FSlateApplication& SlateApp, TSharedRef<ICursor> Cursor) override {}

	bool HandleKeyDownEvent(FSlateApplication& SlateApp, const FKeyEvent& InKeyEvent) override
	{
		//if (InKeyEvent.GetKey() == EKeys::SpaceBar ||
		//	InKeyEvent.GetKey() == EKeys::Enter) 
		//{
		//	return true;
		//}
		return false;
	}
	bool HandleKeyUpEvent(FSlateApplication& SlateApp, const FKeyEvent& InKeyEvent) override
	{
		//if (InKeyEvent.GetKey() == EKeys::SpaceBar ||
		//	InKeyEvent.GetKey() == EKeys::Enter)
		//{
		//	return true;
		//}
		return false;
	}
};


#include "PunPlayerController.generated.h"
/**
 * 
 */
UCLASS()
class APunPlayerController : public APunBasePlayerController,
							public IGameNetworkInterface,
							public IPunPlayerController
{
	GENERATED_BODY()
public:
	APunPlayerController();

public:
	void Tick(float DeltaTime) final;

	//int32 _primaryControllerInitStage = 1;
	void LoadController_Prepare();
	void LoadController_GameManager();

	void BeginPlay() override;
	void OnPostLogin();

private:
	void SetupCameraPawn();
	void SetupGameManager(bool isLoadingFromFile);

	static bool NoPlayerClogged() {
		for (auto it : kPlayerIdToClogStatus) {
			if (it.second) return false;
		}
		return true;
	}
	void SendTickToClient();

	//bool isLocalInitialized() { return gameManager != nullptr; }

public:
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Camera Pawn") ACameraPawn* cameraPawn;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Game Manager") AGameManager* gameManager;

	//! IPunPlayerController
	IGameUIDataSource* dataSource() final {  return Cast<IGameUIDataSource>(gameManager); }
	IGameUIInputSystemInterface* inputSystemInterface() final {  return Cast<IGameUIInputSystemInterface>(cameraPawn); }
	IGameNetworkInterface* networkInterface() final { return Cast<IGameNetworkInterface>(this); }

	/**
	 * IGameNetworkInterface
	 */
public:
	int32 playerId() final { return controllerPlayerId(); }

	const TArray<FString>& playerNamesF() final {
		return gameManager->playerNamesF();
	}

	bool IsPlayerConnected(int32 playerId) final { return gameInstance()->IsPlayerConnected(playerId); }

	//! Send network command to server so it can propagate out with network ticks
	void SendNetworkCommand(std::shared_ptr<FNetworkCommand> networkCommand) final;

	bool IsHost() final { return IsServer(); }

	void SetGameSpeed(int32 gameSpeed) final;
	int32 hostGameSpeed() final { return kGameSpeed; } // Note: gameSpeed() here isn't accurate for client

	void Pause() final;
	void Resume() final;

	void GoToMainMenu() final {
		LLM_SCOPE_(EPunSimLLMTag::PUN_Controller);

		_LOG(PunNetwork, "GoToMainMenu");

		gameInstance()->ResetPlayerCount();
		gameInstance()->PrintPlayers();

		gameInstance()->EnsureSessionDestroyed(true);
		//ClientTravel("/Game/Maps/MainMenu", TRAVEL_Absolute);
	}
	void GoToVictoryScreen() final {
		LLM_SCOPE_(EPunSimLLMTag::PUN_Controller);
		
		// Bring everyone to victory screen
		if (GetLocalRole() == ROLE_Authority) {
			_shouldGameEnd = true;
		}
		ToServer_SavedGameEndStatus(controllerPlayerId());
	}

	bool WorldLocationToScreen(FVector WorldLocation, FVector2D& ScreenLocation) final {
		return ProjectWorldLocationToScreen(WorldLocation, ScreenLocation);
	}
	
	bool ControllerGetMouseHit(FHitResult& hitResult, ECollisionChannel channel) final {
		float mouseX, mouseY;
		GetMousePosition(mouseX, mouseY);
		return GetHitResultAtScreenPosition(FVector2D(mouseX, mouseY), channel, false, hitResult);
	}
	bool ControllerGetHit(FHitResult& hitResult, ECollisionChannel channel, FVector2D screenPosition) final {
		return GetHitResultAtScreenPosition(screenPosition, channel, false, hitResult);
	}

	FVector2D GetMousePositionPun() const final
	{
		FVector2D result;
		bool valid = GetMousePosition(result.X, result.Y);
		if (!valid) {
			return FVector2D(-1.0f, -1.0f);
		}
		return result;
	}
	FVector2D GetViewportSizePun() const final
	{
		int32 resultX, resultY;
		GetViewportSize(resultX, resultY);
		return FVector2D(resultX, resultY);
	}

	WorldAtom2 cameraAtom() final { return cameraPawn->cameraAtom(); }
	void SetCameraAtom(WorldAtom2 lookAtAtom) final {
		cameraPawn->SetCameraAtom(lookAtAtom);
	}

	APunHUD* GetPunHUD() final {
		return Cast<APunHUD>(GetHUD());
	}

	bool isChatFocus = false;
	bool IsChatFocus() final { return isChatFocus; }
	void SetFocusChat() final
	{
#if !UI_ALL
		return;
#endif
		dataSource()->simulation().SetDescriptionUIState(DescriptionUIState());
		
		UChatUI* chatUI = GetPunHUD()->chatUI();
		//FInputModeUIOnly inputModeData; //FInputModeUIOnly
		FInputModeGameAndUI_Pun inputModeData;
		
		inputModeData.SetWidgetToFocus(chatUI->ChatInputBox->TakeWidget());
		inputModeData.SetLockMouseToViewportBehavior(EMouseLockMode::DoNotLock);
		SetInputMode(inputModeData);

		chatUI->OnSetFocusChat();
		isChatFocus = true;
	}
	void SetFocusGame() final
	{
#if !UI_ALL
		return;
#endif
		SetInputMode(FInputModeGameOnly()); // ?? Need this to release Chat Widget Focus

		FInputModeGameAndUI_Pun inputModeData; //FInputModeGameAndUI
		inputModeData.SetWidgetToFocus(nullptr);
		inputModeData.SetLockMouseToViewportBehavior(EMouseLockMode::DoNotLock);
		SetInputMode(inputModeData);

		GetPunHUD()->chatUI()->OnSetFocusGame();
		isChatFocus = false;
	}
	//void SetFocusGameOnly() final
	//{
	//	FInputModeGameAndUI_Pun inputModeData; //FInputModeGameAndUI
	//	inputModeData.SetWidgetToFocus(nullptr);
	//	inputModeData.SetLockMouseToViewportBehavior(EMouseLockMode::DoNotLock);
	//	inputModeData.isGameOnly = true;
	//	SetInputMode(inputModeData);

	//	GetPunHUD()->chatUI()->OnSetFocusGame();
	//	isChatFocus = false;
	//}

	void SetCursor(FName name) final {
		//CurrentMouseCursor = cursorType;
		//if (FSlateApplication::IsInitialized()) {
		//	//FSlateApplication::Get().GetMouse
		//	
		//	FSlateApplication::Get().OnCursorSet();
		//	FSlateApplication::Get().QueryCursor();
		//}

		auto viewport = GetWorld()->GetGameViewport();
		viewport->SetHardwareCursor(EMouseCursor::Type::Default, name, FVector2D(0.5f, 0.5f));
	}

	
	bool IsHoveredOnScrollUI() final {
#if !UI_ALL
		return false;
#endif
		return GetPunHUD()->IsHoveredOnScrollUI();
	}

	void OnCancelPlacement() final {
#if !UI_ALL
		return;
#endif
		
		GetPunHUD()->mainGameUI()->ResetBottomMenuDisplay();

		//
		gameManager->simulation().cardSystem(playerId()).converterCardState = ConverterCardUseState::None;
	}
	//void SetMainGameUIActive(bool active) final {
	//	GetPunHUD()->mainGameUI()->SetVisibility(active ? ESlateVisibility::SelfHitTestInvisible : ESlateVisibility::Collapsed);
	//


	void ShowConfirmationUI(std::string confirmationStr, std::shared_ptr<FNetworkCommand> commandIn) final {
#if UI_ALL
		GetPunHUD()->mainGameUI()->ShowConfirmationUI(confirmationStr, commandIn);
#endif
	}
	bool IsShowingConfirmationUI(std::string confirmationStr) final {
#if !UI_ALL
		return false;
#endif
		return GetPunHUD()->mainGameUI()->IsShowingConfirmationUI(confirmationStr);
	}
	TileArea GetDemolishHighlightArea() final {
		return cameraPawn->buildingPlacementSystem->GetDemolishHighlightArea();
	}

	// Closes only bottom menu display
	// For: clicking objectDescriptionUI
	void ResetBottomMenuDisplay() final {
#if UI_ALL
		GetPunHUD()->mainGameUI()->ResetBottomMenuDisplay();
#endif
	}

	// Closes all
	void ResetGameUI() final {
#if UI_ALL
		GetPunHUD()->ResetGameUI();
		cameraPawn->CancelPlacement();
#endif
	}

	FString playerNameF(int32_t playerId) final {
		auto gameInstance = CastChecked<UPunGameInstance>(GetGameInstance());
		if (gameInstance->playerNamesF().Num() == 0) {
			return "EditorPlayer";
		}
		
		return gameInstance->playerNameF(playerId);
	}
	std::string playerName(int32 playerId) final {
		return ToStdString(playerNameF(playerId));
	}

	int32 serverTick() final { return gameInstance()->serverTick(); }


public:
	/**
	 * Networking
	 */
	UFUNCTION(Reliable, Client) void InitializeClient_ToClient();
	UFUNCTION(Reliable, Server) void InitializeClientCallback();
	UFUNCTION(Reliable, Server) void LoadedClientCallback_ToServer();
	
	UFUNCTION(Reliable, Server) void SendServerCloggedStatus(int32 playerId, bool clogStatus);
	UFUNCTION(Reliable, Client) void SendHash_ToClient(int32 insertIndex, const TArray<int32>& tickToHashes);

	UFUNCTION(Reliable, Client, WithValidation) void TickLocalSimulation_ToClients(const TArray<int32>& networkTickInfoBlob);
	

	//! Send Command to server
	UFUNCTION(Reliable, Server, WithValidation) void ServerSendNetworkCommand(const TArray<int32>& serializedCommand);


	// Victory
	UFUNCTION(Reliable, Server, WithValidation) void ToServer_SavedGameEndStatus(int32 playerId);

	/**
	 * Console
	 */
	UFUNCTION(Exec) void PlayerId();

	UFUNCTION(Exec) void TileInfo(int32 x, int32 y);

	UFUNCTION(Exec) void TileConnected(int32 startX, int32 startY, int32 endX, int32 endY, int32 maxRegionDist);

	UFUNCTION(Exec) void Cheat(const FString& cheatName);

	UFUNCTION(Exec) void ShowUnit(const FString& unitName);
	
	UFUNCTION(Exec) void AnimalCount();
	UFUNCTION(Exec) void AddAnimals(int32 unitCount);
	UFUNCTION(Exec) void BushCount();

	UFUNCTION(Exec) void OpenTradeUI();
	UFUNCTION(Exec) void OpenRareCardUI();

	UFUNCTION(Exec) void SetAIIntercityTrade()
	{
		auto command = make_shared<FSetIntercityTrade>();
		command->playerId = gameManager->simulation().aiStartIndex();
		
		command->resourceEnums.Add(static_cast<uint8>(ResourceEnum::Medicine));
		command->intercityTradeOfferEnum.Add(static_cast<uint8>(IntercityTradeOfferEnum::SellWhenAbove));
		command->targetInventories.Add(0);

		command->resourceEnums.Add(static_cast<uint8>(ResourceEnum::Wood));
		command->intercityTradeOfferEnum.Add(static_cast<uint8>(IntercityTradeOfferEnum::BuyWhenBelow));
		command->targetInventories.Add(500);

		networkInterface()->SendNetworkCommand(command);
	}
	

	
	UFUNCTION(Exec) void SaveMainMenuDisplay()
	{
		MainMenuCameraState cameraState;
		cameraState.cameraAtom = gameManager->cameraAtom();
		cameraState.zoomDistance = gameManager->zoomDistance();
		cameraState.worldTransform = cameraPawn->CameraComponent->GetComponentTransform();
		cameraState.sampleRegionIds = gameManager->sampleRegionIds();
		cameraState.rotator = cameraPawn->GetActorRotation();
		cameraState.mapSizeEnum = gameManager->GetMapSettings().mapSizeEnum();
		
		MainMenuDisplaySaveSystem::SaveOrLoad(gameManager->simulation(), cameraState, true);
	}

	UFUNCTION(Exec) void LoadCameraOnly()
	{
		MainMenuCameraState cameraState;
		MainMenuDisplaySaveSystem::LoadCameraOnly(cameraState);
		cameraPawn->SetCameraAtom(cameraState.cameraAtom);
		cameraPawn->SetZoomAmount(cameraState.zoomDistance);
		cameraPawn->SetActorRotation(cameraState.rotator);
		cameraPawn->CameraComponent->SetComponentToWorld(cameraState.worldTransform);
	}
	
	/*
	 * Main Menu Display Camera
	 * - Normal Save Serialize is troublesome when we change the code.
	 * - JSON save can be used to get the old game state (before code change) and do a normal Save
	 */
	const FString mainMenuDisplayJSON = "Paks/MainMenuDisplayJSON.json";
	UFUNCTION(Exec) void SaveMainMenuDisplayJSON()
	{
		auto jsonObject = MakeShared<FJsonObject>();

		// Map Settings
		FMapSettings mapSettings = gameManager->GetMapSettings();
		jsonObject->SetStringField("mapSeed", mapSettings.mapSeed);
		jsonObject->SetNumberField("mapSize", mapSettings.mapSizeEnumInt);
		
		// Camera
		jsonObject->SetNumberField("cameraAtomX", gameManager->cameraAtom().x);
		jsonObject->SetNumberField("cameraAtomY", gameManager->cameraAtom().y);
		jsonObject->SetNumberField("zoomDistance", gameManager->zoomDistance());

		FTransform cameraTransform = cameraPawn->CameraComponent->GetComponentTransform();;
		jsonObject->SetNumberField("worldTransform_TranslationX", cameraTransform.GetTranslation().X);
		jsonObject->SetNumberField("worldTransform_TranslationY", cameraTransform.GetTranslation().Y);
		jsonObject->SetNumberField("worldTransform_TranslationZ", cameraTransform.GetTranslation().Z);
		jsonObject->SetNumberField("worldTransform_RotationX", cameraTransform.GetRotation().X);
		jsonObject->SetNumberField("worldTransform_RotationY", cameraTransform.GetRotation().Y);
		jsonObject->SetNumberField("worldTransform_RotationZ", cameraTransform.GetRotation().Z);
		jsonObject->SetNumberField("worldTransform_RotationW", cameraTransform.GetRotation().W);

		FRotator rotator = cameraPawn->GetActorRotation();
		jsonObject->SetNumberField("rotatorPitch", rotator.Pitch);
		jsonObject->SetNumberField("rotatorYaw", rotator.Yaw);
		jsonObject->SetNumberField("rotatorRoll", rotator.Roll);

		gameManager->simulation().SaveBuildingsToJSON(jsonObject, gameManager->sampleRegionIds());

		SaveJsonToFile(jsonObject, mainMenuDisplayJSON);
	}

	UFUNCTION(Exec) void LoadMainMenuDisplayJSON()
	{
		TSharedPtr<FJsonObject> jsonObject = LoadJsonFromFile(mainMenuDisplayJSON);

		WorldAtom2 cameraAtom;
		cameraAtom.x = jsonObject->GetIntegerField("cameraAtomX");
		cameraAtom.y = jsonObject->GetIntegerField("cameraAtomY");
		cameraPawn->SetCameraAtom(cameraAtom);
		cameraPawn->SetZoomAmount(jsonObject->GetNumberField("zoomDistance"));

		FVector cameraTranslation;
		cameraTranslation.X = jsonObject->GetNumberField("worldTransform_TranslationX");
		cameraTranslation.Y = jsonObject->GetNumberField("worldTransform_TranslationY");
		cameraTranslation.Z = jsonObject->GetNumberField("worldTransform_TranslationZ");

		FQuat cameraQuat;
		cameraQuat.X = jsonObject->GetNumberField("worldTransform_RotationX");
		cameraQuat.Y = jsonObject->GetNumberField("worldTransform_RotationY");
		cameraQuat.Z = jsonObject->GetNumberField("worldTransform_RotationZ");
		cameraQuat.W = jsonObject->GetNumberField("worldTransform_RotationW");

		FTransform cameraTransform;
		cameraTransform.SetTranslationAndScale3D(cameraTranslation, FVector::OneVector);
		cameraTransform.SetRotation(cameraQuat);

		cameraPawn->CameraComponent->SetComponentToWorld(cameraTransform);

		FRotator rotator;
		rotator.Pitch = jsonObject->GetNumberField("rotatorPitch");
		rotator.Yaw = jsonObject->GetNumberField("rotatorYaw");
		rotator.Roll = jsonObject->GetNumberField("rotatorRoll");
		cameraPawn->SetActorRotation(rotator);

		gameManager->simulation().LoadBuildingsFromJSON(jsonObject.ToSharedRef());
	}

	/*
	 * Save and load JSON
	 */
	void SaveJsonToFile(const TSharedRef<FJsonObject>& jsonObject, FString saveFileName)
	{
		FString path = FPaths::ConvertRelativePathToFull(FPaths::ProjectContentDir());

		FString jsonString;
		TSharedRef<TJsonWriter<>> writer = TJsonWriterFactory<>::Create(&jsonString);
		FJsonSerializer::Serialize(jsonObject, writer);;

		FFileHelper::SaveStringToFile(jsonString, *(path + saveFileName));
	}
	TSharedPtr<FJsonObject> LoadJsonFromFile(FString saveFileName)
	{
		FString path = FPaths::ConvertRelativePathToFull(FPaths::ProjectContentDir());
		
		FString jsonString;
		FFileHelper::LoadFileToString(jsonString, *(path + saveFileName));

		TSharedPtr<FJsonObject> jsonObject(new FJsonObject());

		TSharedRef<TJsonReader<>> reader = TJsonReaderFactory<>::Create(jsonString);
		FJsonSerializer::Deserialize(reader, jsonObject);

		return jsonObject;
	}



	UFUNCTION(Exec) void ToggleTestMainMenu()
	{
#if !UI_ALL
		return;
#endif
		if (GetPunHUD()->mainGameUI()->TestMainMenuOverlay1->GetVisibility() == ESlateVisibility::Collapsed) {
			GetPunHUD()->mainGameUI()->TestMainMenuOverlay1->SetVisibility(ESlateVisibility::Visible);
			GetPunHUD()->mainGameUI()->TestMainMenuOverlay2->SetVisibility(ESlateVisibility::Visible);
		} else {
			GetPunHUD()->mainGameUI()->TestMainMenuOverlay1->SetVisibility(ESlateVisibility::Collapsed);
			GetPunHUD()->mainGameUI()->TestMainMenuOverlay2->SetVisibility(ESlateVisibility::Collapsed);
		}
	}
	
	UFUNCTION(Exec) void SavePlayerActions(int32 playerId, const FString& fileName);

	UFUNCTION(Exec) void ZoomDistance() {
		PUN_LOG("Zoom Distance: %f", gameManager->zoomDistance());
	}

	UFUNCTION(Exec) void PrintResourceSys()
	{
		std::stringstream ss;
		auto& sim = gameManager->simulation();
		sim.resourceSystem(playerId()).resourcedebugStr(ss);
		PUN_LOG("%s", ToTChar(ss.str()));
	}
	UFUNCTION(Exec) void PrintResourceSysFor(const FString& resourceName)
	{
		std::string resourceNameStd = ToStdString(resourceName);
		ResourceEnum resourceEnum = FindResourceEnumByName(resourceNameStd);
		
		std::stringstream ss;
		auto& sim = gameManager->simulation();
		sim.resourceSystem(playerId()).resourcedebugStr(ss, resourceEnum);
		PUN_LOG("%s", ToTChar(ss.str()));
	}

	UFUNCTION(Exec) void PrintAISys(int32 playerId)
	{
		std::stringstream ss;
		gameManager->simulation().aiPlayerSystem(playerId).AIDebugString(ss);
		PUN_LOG("%s", ToTChar(ss.str()));
	}

	UFUNCTION(Exec) void SimInfo() {
		PUN_LOG("Tick:%d", Time::Ticks());
	}

	UFUNCTION(Exec) void PrintLLM() {
		PUN_LLM_ONLY(PunScopeLLM::Print());
	}


	// Photo taking
	UFUNCTION(Exec) void SetShadowDistanceMultiplier(float multiplier) {
		gameManager->ShadowDistanceMultiplier = multiplier;
	}
	UFUNCTION(Exec) void SetMaxRegionCullDistance(float maxRegionCullDistance) {
		gameManager->MaxRegionCullDistance = maxRegionCullDistance;
	}

	//UFUNCTION(Exec) void PackageSound() {
	//	LLM_SCOPE_(EPunSimLLMTag::PUN_Controller);
	//	
	//	gameManager->soundSystem()->PackageSound();
	//}
	//UFUNCTION(Exec) void UsePackageSound() {
	//	LLM_SCOPE_(EPunSimLLMTag::PUN_Controller);
	//	
	//	gameManager->soundSystem()->UsePackageSound();
	//}

	UFUNCTION(Exec) void SaveCheck();
	void SaveCheckTick();


	// Done for map transition so tick doesn't happpen after gameInst's data was cleared
	void SetTickDisabled(bool tickDisabled) final {
		_tickDisabled = tickDisabled;
	}

private:
	template<typename Func>
	void ExecuteAllControllers(Func func) {
		TArray<AActor*> found;
		UGameplayStatics::GetAllActorsOfClass(GetWorld(), APlayerController::StaticClass(), found);

		for (int i = 0; i < found.Num(); i++) {
			func(Cast<APunPlayerController>(found[i]));
		}
	}

	// Server First Controller only!
	int32 GetPlayersBeyondLoadStage(ClientLoadStage stageIn);
	
private:
	static std::unordered_map<int32, int32> kPlayerIdToClogStatus;
	
	static int32 kGameSpeed;
	static int32 kResumeGameSpeed;

	static std::vector<std::shared_ptr<FNetworkCommand>> kCommandQueue;

	int32 _playerControllerTick = 0;

	ServerLoadStage _serverLoadStage = ServerLoadStage::NotStarted;
	ClientLoadStage _clientLoadStage = ClientLoadStage::NotStarted;
	LoadStepEnum _loadStep = LoadStepEnum::NotStarted;

	bool _shouldGameEnd = false;
	int32 _playersReadyForGameEnd = 0;

	//! In the server, each proxy controller keep track of its own tick states
	int32 _proxyControllerTick = 0;

	float _leftOverDeltaTime = 0;

	bool _tickDisabled = false;

private:
	int32 _hashSendTick = 0; // hashes from this tick up until the most recent tick will be sent to server
	static std::unordered_map<int32, std::vector<int32>> kPlayerIdToTickHashList;
};
