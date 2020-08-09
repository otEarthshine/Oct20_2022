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

		// Map Settings TODO: needed?
		FMapSettings mapSettings = gameManager->GetMapSettings();
		jsonObject->SetStringField("mapSeed", mapSettings.mapSeed);
		jsonObject->SetNumberField("mapSize", mapSettings.mapSizeEnumInt);
		jsonObject->SetNumberField("mapSeaLevel", static_cast<double>(mapSettings.mapSeaLevel));
		jsonObject->SetNumberField("mapMoisture", static_cast<double>(mapSettings.mapMoisture));
		jsonObject->SetNumberField("mapTemperature", static_cast<double>(mapSettings.mapTemperature));
		
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
	void SaveJsonToFile(const TSharedRef<FJsonObject>& jsonObject, FString saveFileName, bool isSaveDir = false)
	{
		FString relativePath = isSaveDir ? FPaths::ProjectSavedDir() : FPaths::ProjectContentDir();
		FString path = FPaths::ConvertRelativePathToFull(relativePath);

		FString jsonString;
		TSharedRef<TJsonWriter<>> writer = TJsonWriterFactory<>::Create(&jsonString);
		FJsonSerializer::Serialize(jsonObject, writer);;

		FFileHelper::SaveStringToFile(jsonString, *(path + saveFileName));
	}
	TSharedPtr<FJsonObject> LoadJsonFromFile(FString saveFileName, bool isSaveDir = false)
	{
		FString relativePath = isSaveDir ? FPaths::ProjectSavedDir() : FPaths::ProjectContentDir();
		FString path = FPaths::ConvertRelativePathToFull(relativePath);
		
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
	// !!! In-Game Only
	UFUNCTION(Exec) void SetShadowDistanceMultiplier(float multiplier) {
		gameManager->ShadowDistanceMultiplier = multiplier;
	}
	UFUNCTION(Exec) void SetMaxRegionCullDistance(float maxRegionCullDistance) {
		gameManager->MaxRegionCullDistance = maxRegionCullDistance;
	}

	UFUNCTION(Exec) void SetLightAngle(float lightAngle) {
		gameManager->directionalLight()->SetActorRotation(FRotator(308, lightAngle, 0));
		PUN_LOG("SetLightAngle %f", lightAngle);
		// Default lightAngle: 47.5
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

	/*
	 * Trailer
	 */
	UFUNCTION(Exec) void TrailerCityReplayUnpause() override {
		gameManager->simulation().replaySystem().TrailerCityReplayUnpause();
	}
	
	UFUNCTION(Exec) void ReplaySave(const FString& replayFileName) {
		gameManager->simulation().replaySystem().SavePlayerActions(0, replayFileName);
	}

	std::vector<TrailerCameraRecord> cameraRecords;

	UFUNCTION(Exec) void TrailerCameraClear() {
		cameraRecords.clear();
	}

	UFUNCTION(Exec) void TrailerCameraStart() {
		cameraPawn->SetCameraSequence(cameraRecords);
	}
	UFUNCTION(Exec) void TrailerCameraStop() {
		cameraPawn->ClearCameraSequence();
	}

	UFUNCTION(Exec) void TrailerCameraPrint() {
		PUN_LOG("TrailerCameraPrint:");
		for (size_t i = 0; i < cameraRecords.size(); i++) {
			PUN_LOG("trailerCamera %s zoom:%f rotator:%s transition:%s t_time:%f", 
						ToTChar(cameraRecords[i].cameraAtom.ToString()), 
						cameraRecords[i].zoomDistance, 
						*cameraRecords[i].rotator.ToCompactString(),
						*cameraRecords[i].transition, cameraRecords[i].transitionTime);
		}
	}
	
	UFUNCTION(Exec) void TrailerCameraSave(const FString& cameraRecordFileName, float transitionTime = 3.0f)
	{
		TrailerCameraRecord record;
		record.cameraAtom = gameManager->cameraAtom();
		record.zoomDistance = gameManager->zoomDistance();
		record.rotator = cameraPawn->GetActorRotation();
		record.transition = "Lerp";
		record.transitionTime = transitionTime;
		cameraRecords.push_back(record);
		
		TArray<TSharedPtr<FJsonValue>> cameraRecordJsons;

		for (size_t i = 0; i < cameraRecords.size(); i++) 
		{
			auto cameraRecordJson = MakeShared<FJsonObject>();
			
			// Camera
			cameraRecordJson->SetNumberField("cameraAtomX", cameraRecords[i].cameraAtom.x);
			cameraRecordJson->SetNumberField("cameraAtomY", cameraRecords[i].cameraAtom.y);
			cameraRecordJson->SetNumberField("zoomDistance", cameraRecords[i].zoomDistance);

			FRotator rotator = cameraRecords[i].rotator;
			cameraRecordJson->SetNumberField("rotatorPitch", rotator.Pitch);
			cameraRecordJson->SetNumberField("rotatorYaw", rotator.Yaw);
			cameraRecordJson->SetNumberField("rotatorRoll", rotator.Roll);

			cameraRecordJson->SetStringField("transition", cameraRecords[i].transition);
			cameraRecordJson->SetNumberField("transitionTime", cameraRecords[i].transitionTime);

			cameraRecordJson->SetNumberField("unpause", cameraRecords[i].isCameraReplayUnpause);

			// TileMoveDistance is useful in determining transitionTime
			float tileMoveDistance = 0;
			if (i > 0) {
				int32 atomDistance = WorldAtom2::Distance(cameraRecords[i].cameraAtom, cameraRecords[i - 1].cameraAtom);
				tileMoveDistance = static_cast<float>(atomDistance) / CoordinateConstants::AtomsPerTile;
			}
			cameraRecordJson->SetNumberField("tileMoveDistance", tileMoveDistance);

			TSharedPtr<FJsonValue> jsonValue = MakeShared<FJsonValueObject>(cameraRecordJson);
			cameraRecordJsons.Add(jsonValue);
		}

		auto jsonObject = MakeShared<FJsonObject>();
		jsonObject->SetArrayField("CameraRecords", cameraRecordJsons);
		
		SaveJsonToFile(jsonObject, "TrailerCamera_" + cameraRecordFileName + ".json", true);
	}

	UFUNCTION(Exec) void TrailerCameraLoad(const FString& cameraRecordFileName)
	{
		cameraRecords.clear();
		
		TSharedPtr<FJsonObject> jsonObject = LoadJsonFromFile("TrailerCamera_" + cameraRecordFileName + ".json", true);

		const TArray<TSharedPtr<FJsonValue>>& cameraRecordsJson = jsonObject->GetArrayField("CameraRecords");
		for (const TSharedPtr<FJsonValue>& cameraRecordValue : cameraRecordsJson)
		{
			TSharedPtr<FJsonObject> cameraRecordJson = cameraRecordValue->AsObject();

			TrailerCameraRecord record;
			record.cameraAtom = WorldAtom2(cameraRecordJson->GetNumberField("cameraAtomX"), 
												cameraRecordJson->GetNumberField("cameraAtomY"));
			record.zoomDistance = cameraRecordJson->GetNumberField("zoomDistance");

			record.rotator = FRotator(cameraRecordJson->GetNumberField("rotatorPitch"),
												cameraRecordJson->GetNumberField("rotatorYaw"),
												cameraRecordJson->GetNumberField("rotatorRoll"));

			record.transition = cameraRecordJson->GetStringField("transition");
			record.transitionTime = cameraRecordJson->GetNumberField("transitionTime");

			record.isCameraReplayUnpause = FGenericPlatformMath::RoundToInt(cameraRecordJson->GetNumberField("unpause"));

			cameraRecords.push_back(record);
		}
	}

	// Helpers
	static void SetAreaField(TSharedRef<FJsonObject>& jsonObj, FString areaName, TileArea& area) {
		jsonObj->SetNumberField(areaName + "_minX", area.minX);
		jsonObj->SetNumberField(areaName + "_minY", area.minY);
		jsonObj->SetNumberField(areaName + "_maxX", area.maxX);
		jsonObj->SetNumberField(areaName + "_maxY", area.maxY);
	}
	static void GetAreaField(TSharedPtr<FJsonObject>& jsonObj, FString areaName, TileArea& area) {
		area.minX = jsonObj->GetNumberField(areaName + "_minX");
		area.minY = jsonObj->GetNumberField(areaName + "_minY");
		area.maxX = jsonObj->GetNumberField(areaName + "_maxX");
		area.maxY = jsonObj->GetNumberField(areaName + "_maxY");
	}
	
	// Trailer Commands Save
	UFUNCTION(Exec) void TrailerCitySave(const FString& trailerCityName)
	{
		auto& sim = gameManager->simulation();
		std::vector<std::shared_ptr<FNetworkCommand>> commands = sim.replaySystem().trailerCommandsSave;
		PUN_CHECK(commands.size() > 1);
		if (commands.size() <= 1) {
			return;
		}

		// Trim off buildings/road that was already demolished.
		for (size_t i = commands.size(); i-- > 0;)
		{
			if (commands[i]->commandType() == NetworkCommandEnum::PlaceBuilding) {
				auto command = std::static_pointer_cast<FPlaceBuildingParameters>(commands[i]);

				// Check by buildingId
				bool isDemolished = command->area.ExecuteOnAreaWithExit_WorldTile2([&](WorldTile2 tile) {
					return static_cast<CardEnum>(command->buildingEnum) != sim.buildingEnumAtTile(tile);
				});
				if (isDemolished) {
					commands.erase(commands.begin() + i);
				}
			}
			//else if (commands[i]->commandType() == NetworkCommandEnum::PlaceGather) {
			//	auto command = std::static_pointer_cast<FPlaceGatherParameters>(commands[i]);

			//	// Check if it is road
			//	if (IsRoadPlacement(static_cast<PlacementType>(command->placementType))) {
			//		bool isDemolished = false;
			//		for (int32 tile : command->path) {
			//			if (!sim.IsRoadTile(tile)) {
			//				isDemolished = true;
			//				break;
			//			}
			//		}
			//		if (isDemolished) {
			//			commands.erase(commands.begin() + i);
			//		}
			//	}
			//}
			// UpgradeBuilding is only for townhall
			else if (commands[i]->commandType() == NetworkCommandEnum::UpgradeBuilding)
			{
				auto command = std::static_pointer_cast<FUpgradeBuilding>(commands[i]);
				if (!sim.building(command->buildingId).isEnum(CardEnum::Townhall)) {
					commands.erase(commands.begin() + i);
				}
			}
			else if (commands[i]->commandType() == NetworkCommandEnum::Cheat)
			{
				auto command = std::static_pointer_cast<FCheat>(commands[i]);
				switch(command->cheatEnum)
				{
				case CheatEnum::AddResource:
				case CheatEnum::TrailerPauseForCamera:
				case CheatEnum::TrailerForceSnowStart:
				case CheatEnum::TrailerForceSnowStop:
				case CheatEnum::TrailerIncreaseAllHouseLevel:
					break;
				default:
					commands.erase(commands.begin() + i);
					break;
				}
			}
		}

		// Merge PlaceDrag for roads
		for (size_t i = commands.size() - 1; i-- > 1;)
		{
			auto& command = commands[i];
			auto& nextCommand = commands[i + 1];

			if (command->commandType() == NetworkCommandEnum::PlaceGather &&
				nextCommand->commandType() == NetworkCommandEnum::PlaceGather)
			{
				auto commandCasted = std::static_pointer_cast<FPlaceGatherParameters>(command);
				auto nextCommandCasted = std::static_pointer_cast<FPlaceGatherParameters>(nextCommand);

				if (commandCasted->placementType == static_cast<int32>(PlacementType::DirtRoad) &&
					nextCommandCasted->placementType == static_cast<int32>(PlacementType::DirtRoad)) 
				{
					commandCasted->path.Append(nextCommandCasted->path);
					commands.erase(commands.begin() + i + 1);
				}
			}
		}


		

		PUN_CHECK(commands[0]->commandType() == NetworkCommandEnum::ChooseLocation);
		

		TArray<TSharedPtr<FJsonValue>> jsons;
		
		for (size_t i = 0; i < commands.size(); i++)
		{
			auto jsonObj = MakeShared<FJsonObject>();

			if (commands[i]->commandType() == NetworkCommandEnum::ChooseLocation)
			{
				auto command = std::static_pointer_cast<FChooseLocation>(commands[i]);
				
				jsonObj->SetStringField("commandType", "ChooseLocation");
				jsonObj->SetNumberField("provinceId", command->provinceId);
				jsonObj->SetNumberField("isChoosingOrReserving", command->isChoosingOrReserving);
			}
			if (commands[i]->commandType() == NetworkCommandEnum::PlaceBuilding)
			{
				auto command = std::static_pointer_cast<FPlaceBuildingParameters>(commands[i]);
				
				jsonObj->SetStringField("commandType", "PlaceBuilding");
				
				jsonObj->SetNumberField("buildingEnum", command->buildingEnum);
				jsonObj->SetStringField("buildingName", ToFString(GetBuildingInfoInt(command->buildingEnum).name));
				
				jsonObj->SetNumberField("buildingLevel", command->buildingLevel);

				SetAreaField(jsonObj, "area", command->area);
				SetAreaField(jsonObj, "area2", command->area2);

				jsonObj->SetNumberField("center_x", command->center.x);
				jsonObj->SetNumberField("center_y", command->center.y);

				jsonObj->SetNumberField("faceDirection", command->faceDirection);
			}
			else if (commands[i]->commandType() == NetworkCommandEnum::PlaceGather)
			{
				auto command = std::static_pointer_cast<FPlaceGatherParameters>(commands[i]);

				jsonObj->SetStringField("commandType", "PlaceDrag");

				SetAreaField(jsonObj, "area", command->area);
				SetAreaField(jsonObj, "area2", command->area2);

				TArray<TSharedPtr<FJsonValue>> pathJsonValues;
				for (int32 j = 0; j < command->path.Num(); j++) {
					pathJsonValues.Add(MakeShared<FJsonValueNumber>(command->path[j]));
				}
				jsonObj->SetArrayField("path", pathJsonValues);
				jsonObj->SetNumberField("pathSize", command->path.Num()); // For calculating transitionTime

				jsonObj->SetNumberField("placementType", command->placementType);
				jsonObj->SetNumberField("isInstantRoad", 0); // use command->harvestResourceEnum for isInstantRoad
			}
			else if (commands[i]->commandType() == NetworkCommandEnum::ClaimLand)
			{
				auto command = std::static_pointer_cast<FClaimLand>(commands[i]);

				jsonObj->SetStringField("commandType", "ClaimLand");

				jsonObj->SetNumberField("provinceId", command->provinceId);
				jsonObj->SetNumberField("claimEnum", static_cast<double>(command->claimEnum));
			}
			else if (commands[i]->commandType() == NetworkCommandEnum::Cheat)
			{
				auto command = std::static_pointer_cast<FCheat>(commands[i]);

				jsonObj->SetStringField("commandType", "Cheat");

				jsonObj->SetNumberField("cheatEnum", static_cast<double>(command->cheatEnum));
				jsonObj->SetStringField("cheatName", ToFString(GetCheatName(command->cheatEnum)));
				jsonObj->SetNumberField("var1", static_cast<double>(command->var1));
				jsonObj->SetNumberField("var2", static_cast<double>(command->var2));
			}
			else if (commands[i]->commandType() == NetworkCommandEnum::UpgradeBuilding)
			{
				auto command = std::static_pointer_cast<FUpgradeBuilding>(commands[i]);

				jsonObj->SetStringField("commandType", "UpgradeBuilding");
				jsonObj->SetNumberField("upgradeLevel", command->upgradeLevel);
				jsonObj->SetNumberField("upgradeType", command->upgradeType);
			}

			TSharedPtr<FJsonValue> jsonValue = MakeShared<FJsonValueObject>(jsonObj);
			jsons.Add(jsonValue);
		}

		auto jsonObject = MakeShared<FJsonObject>();
		jsonObject->SetArrayField("TrailerCommands", jsons);

		SaveJsonToFile(jsonObject, "TrailerCity_" + trailerCityName + ".json", true);

		PUN_LOG("TrailerCitySave %d", commands.size());
		for (size_t i = 0; i < commands.size(); i++) {
			PUN_LOG("  trailer[%d] %s", i, *commands[i]->ToCompactString());
		}
	}
	UFUNCTION(Exec) void TrailerCityPrintSave() {
		auto commands = gameManager->simulation().replaySystem().trailerCommandsSave;
		PUN_LOG("TrailerCityPrintSave %d", commands.size());
		for (size_t i = 0; i < commands.size(); i++) {
			PUN_LOG("  trailer[%d] %s", i, *commands[i]->ToCompactString());
		}
	}

	UFUNCTION(Exec) void TrailerCityClearSave() {
		gameManager->simulation().replaySystem().trailerCommandsSave.clear();
	}


	UFUNCTION(Exec) void TrailerCityLoad(const FString& trailerCityName)
	{
		std::vector<std::shared_ptr<FNetworkCommand>>& commands = gameManager->simulation().replaySystem().trailerCommandsLoad;
		commands.clear();

		TSharedPtr<FJsonObject> jsonObject = LoadJsonFromFile("TrailerCity_" + trailerCityName + ".json", true);

		const TArray<TSharedPtr<FJsonValue>>& commandsJson = jsonObject->GetArrayField("TrailerCommands");
		for (const TSharedPtr<FJsonValue>& commandValue : commandsJson)
		{
			TSharedPtr<FJsonObject> jsonObj = commandValue->AsObject();

			if (jsonObj->GetStringField("commandType") == "ChooseLocation") 
			{
				auto command = std::make_shared<FChooseLocation>();
				command->provinceId = jsonObj->GetNumberField("provinceId");
				command->isChoosingOrReserving = jsonObj->GetNumberField("isChoosingOrReserving");

				commands.push_back(command);
			}
			else if (jsonObj->GetStringField("commandType") == "PlaceBuilding") 
			{
				auto command = std::make_shared<FPlaceBuildingParameters>();

				command->buildingEnum = jsonObj->GetNumberField("buildingEnum");

				command->buildingLevel = jsonObj->GetNumberField("buildingLevel");

				GetAreaField(jsonObj, "area", command->area);
				GetAreaField(jsonObj, "area2", command->area2);

				command->center.x = jsonObj->GetNumberField("center_x");
				command->center.y = jsonObj->GetNumberField("center_y");

				command->faceDirection = jsonObj->GetNumberField("faceDirection");

				commands.push_back(command);
			}
			else if (jsonObj->GetStringField("commandType") == "PlaceDrag") 
			{
				auto command = std::make_shared<FPlaceGatherParameters>();

				GetAreaField(jsonObj, "area", command->area);
				GetAreaField(jsonObj, "area2", command->area2);

				const TArray<TSharedPtr<FJsonValue>>& pathJson = jsonObj->GetArrayField("path");
				for (int32 i = 0; i < pathJson.Num(); i++) {
					command->path.Add(pathJson[i]->AsNumber());
				}

				command->placementType = jsonObj->GetNumberField("placementType");
				command->harvestResourceEnum = static_cast<ResourceEnum>(FGenericPlatformMath::RoundToInt(jsonObj->GetNumberField("isInstantRoad")));

				commands.push_back(command);
			}
			else if (jsonObj->GetStringField("commandType") == "ClaimLand") 
			{
				auto command = std::make_shared<FClaimLand>();

				command->provinceId = jsonObj->GetNumberField("provinceId");
				command->claimEnum = static_cast<CallbackEnum>(FGenericPlatformMath::RoundToInt(jsonObj->GetNumberField("claimEnum")));

				commands.push_back(command);
			}
			else if (jsonObj->GetStringField("commandType") == "Cheat")
			{
				auto command = std::make_shared<FCheat>();

				command->cheatEnum = static_cast<CheatEnum>(FGenericPlatformMath::RoundToInt(jsonObj->GetNumberField("cheatEnum")));

				command->var1 = jsonObj->GetNumberField("var1");
				command->var2 = jsonObj->GetNumberField("var2");

				commands.push_back(command);
			}
			else if (jsonObj->GetStringField("commandType") == "UpgradeBuilding")
			{
				auto command = std::make_shared<FUpgradeBuilding>();
				command->upgradeLevel = jsonObj->GetNumberField("upgradeLevel");
				command->upgradeType = jsonObj->GetNumberField("upgradeType");

				commands.push_back(command);
			}
		}

		PUN_LOG("TrailerCityLoad %d", commands.size());
		for (size_t i = 0; i < commands.size(); i++) {
			PUN_LOG("  trailer[%d] %s", i, *commands[i]->ToCompactString());
		}
	}

	UFUNCTION(Exec) void TrailerCityLoadedToSave()
	{	
		auto& replaySys = gameManager->simulation().replaySystem();

		PUN_LOG("TrailerCityLoadedToSave[Before] Save:%llu Load:%llu", replaySys.trailerCommandsSave.size(), replaySys.trailerCommandsLoad.size());
		replaySys.trailerCommandsSave = replaySys.trailerCommandsLoad;
		PUN_LOG("TrailerCityLoadedToSave[After] Save:%llu Load:%llu", replaySys.trailerCommandsSave.size(), replaySys.trailerCommandsLoad.size());
	}

	UFUNCTION(Exec) void TrailerCityStart(int32 playerId)
	{
		auto& replaySys = gameManager->simulation().replaySystem();
		replaySys.trailerCommandsSave.clear(); // Clear old saves

		replaySys.replayPlayers[playerId].SetTrailerCommands(replaySys.trailerCommandsLoad);

		SimSettings::Set("TrailerMode", 1);
	}
	UFUNCTION(Exec) void TrailerModeStop() {
		SimSettings::Set("TrailerMode", 0);
	}
	UFUNCTION(Exec) void TrailerModeStart() {
		SimSettings::Set("TrailerMode", 1);
	}

	UFUNCTION(Exec) void TrailerCityPause(int32 playerId)
	{
		auto& replaySys = gameManager->simulation().replaySystem();
		replaySys.replayPlayers[playerId].PauseTrailerCommands();
		SimSettings::Set("TrailerMode", 0);
	}
	UFUNCTION(Exec) void TrailerCityUnpause(int32 playerId)
	{
		auto& replaySys = gameManager->simulation().replaySystem();
		replaySys.replayPlayers[playerId].UnpauseTrailerCommands();
		SimSettings::Set("TrailerMode", 1);
	}

	UFUNCTION(Exec) void TrailerStartAll(const FString& trailerName)
	{	
		SetGameSpeed(2);
		TrailerCityLoad(trailerName);
		TrailerCityStart(0);
		
		TrailerCameraLoad(trailerName);
		TrailerCameraStart();
		
		GetPunHUD()->HideMainGameUI(); // TODO: why not working?
	}
	UFUNCTION(Exec) void TrailerStartCityOnly(const FString& trailerName)
	{
		SetGameSpeed(2);
		TrailerCityLoad(trailerName);
		TrailerCityStart(0);
	}

	UFUNCTION(Exec) void AbandonTown(int32 playerId) {
		if (gameManager->simulation().IsPlayerInitialized(playerId)) {
			gameManager->simulation().AbandonTown(playerId);
		}
	}
	

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
