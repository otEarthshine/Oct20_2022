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

	void GoToMainMenu() final
	{
		LLM_SCOPE_(EPunSimLLMTag::PUN_Controller);

		_LOG(PunNetwork, "GoToMainMenu");

		//gameInstance()->ResetPlayerCount(); // Do this every tick when in main menu
		//gameInstance()->PrintPlayers();

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

	WorldAtom2 GetMouseGroundAtom() const final
	{
		FVector2D mousePosition = GetMousePositionPun();
		if (mousePosition.Equals(FVector2D(-1, -1), 0.01)) {
			return WorldAtom2::Invalid;
		}
		
		FVector origin;
		FVector direction;
		UGameplayStatics::DeprojectScreenToWorld(this, mousePosition, origin, direction);

		float traceDistanceToGround = fabs(origin.Z / direction.Z);
		FVector groundPoint = direction * traceDistanceToGround + origin;

		return MapUtil::AtomLocation(cameraAtom(), groundPoint);
	}

	void SetMouseLocationPun(FVector2D mousePositionIn) final {
		SetMouseLocation(FMath::RoundToInt(mousePositionIn.X), FMath::RoundToInt(mousePositionIn.Y));
	}

	WorldAtom2 cameraAtom() const final { return cameraPawn->cameraAtom(); }
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

	PlacementType placementType() final { return cameraPawn->buildingPlacementSystem->placementState(); }
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

	/*
	 * Keys
	 */
	void KeyPressed_H() final
	{
		if (GetPunHUD()->mainGameUI()->BuildMenuOverlay->GetVisibility() != ESlateVisibility::Collapsed)
		{
			inputSystemInterface()->StartBuildingPlacement(CardEnum::House, 0, false);
		}
		else
		{
			WorldAtom2 lookAtAtom = simulation().homeAtom(playerId());
			if (lookAtAtom == WorldAtom2::Zero) {
				return;
			}

			SetCameraAtom(lookAtAtom);

			FRotator rotation = cameraPawn->GetActorRotation();
			rotation.Yaw = 0;
			cameraPawn->SetActorRotation(rotation);
		}
	}
	void KeyPressed_F() final
	{
		if (GetPunHUD()->mainGameUI()->BuildMenuOverlay->GetVisibility() != ESlateVisibility::Collapsed &&
			simulation().IsBuildingUnlocked(playerId(), CardEnum::Farm))
		{
			inputSystemInterface()->StartBuildingPlacement(CardEnum::Farm, 0, false);

			// Noticed farm, no longer need exclamation on farm after this...
			simulation().parameters(playerId())->FarmNoticed = true;
		}
	}
	void KeyPressed_Y() final
	{
		if (GetPunHUD()->mainGameUI()->BuildMenuOverlay->GetVisibility() != ESlateVisibility::Collapsed)
		{
			inputSystemInterface()->StartBuildingPlacement(CardEnum::StorageYard, 0, false);
		}
	}


	/*
	 * Helpers
	 */
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

	UFUNCTION(Exec) void ExecuteInitialCloudFade() final {
		GetPunHUD()->escMenuUI()->StartLoadingScreenFade();
	}
	TArray<class UFireForgetAudioComponent*> GetPunAudios() final {
		return gameManager->soundSystem()->punAudios;
	}
	
	UFUNCTION(Exec) void TestCrash() {
		checkNoEntry();
	}
	UFUNCTION(Exec) void TestAutosave() {
		GetPunHUD()->AutosaveGame();
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
		jsonObject->SetNumberField("mapMountainDensity", static_cast<double>(mapSettings.mapMountainDensity));
		
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



//	UFUNCTION(Exec) void ToggleTestMainMenu()
//	{
//#if !UI_ALL
//		return;
//#endif
//		if (GetPunHUD()->mainGameUI()->TestMainMenuOverlay1->GetVisibility() == ESlateVisibility::Collapsed) {
//			GetPunHUD()->mainGameUI()->TestMainMenuOverlay1->SetVisibility(ESlateVisibility::Visible);
//			GetPunHUD()->mainGameUI()->TestMainMenuOverlay2->SetVisibility(ESlateVisibility::Visible);
//		} else {
//			GetPunHUD()->mainGameUI()->TestMainMenuOverlay1->SetVisibility(ESlateVisibility::Collapsed);
//			GetPunHUD()->mainGameUI()->TestMainMenuOverlay2->SetVisibility(ESlateVisibility::Collapsed);
//		}
//	}
	
	UFUNCTION(Exec) void SavePlayerActions(int32 playerId, const FString& fileName);

	UFUNCTION(Exec) void ZoomDistance() {
		PUN_LOG("Zoom Distance: %f", gameManager->zoomDistance());
	}

	/*
	 * Test Commands
	 */
	// Single Player Only!!!
	//UFUNCTION(Exec) void AddAllResources() {
	//	for (ResourceInfo info : ResourceInfos) {
	//		gameManager->simulation().resourceSystem(playerId()).AddResourceGlobal(info.resourceEnum, 10, gameManager->simulation());
	//	}
	//}

	UFUNCTION(Exec) void MineDeplete(int32 tileX, int32 tileY)
	{
		auto& sim = gameManager->simulation();
		WorldTile2 tile(tileX, tileY);
		if (tile.isValid()) {
			int32 provinceId = sim.GetProvinceIdClean(tile);
			sim.georesourceSystem().MineStone(provinceId, sim.georesource(provinceId).stoneAmount);
			sim.georesourceSystem().MineOre(provinceId, sim.georesource(provinceId).depositAmount);
		}
	}

	UFUNCTION(Exec) void AddKidnapGuardBuff(int32 playerId) {
		gameManager->simulation().playerOwned(playerId).AddBuff(CardEnum::KidnapGuard);
	}
	UFUNCTION(Exec) void AddTreasuryGuardBuff(int32 playerId) {
		gameManager->simulation().playerOwned(playerId).AddBuff(CardEnum::TreasuryGuard);
	}

	
	UFUNCTION(Exec) void OrderAI(int32 claimEnum, int32 aiTileX, int32 aiTileY, int32 tileX, int32 tileY)
	{
		// claimEnum:
		// CallbackEnum::StartAttackProvince = 4
		// CallbackEnum::ReinforceAttackProvince = 5
		// CallbackEnum::Liberate = 8
		auto& sim = gameManager->simulation();
		WorldTile2 tile(tileX, tileY);
		WorldTile2 aiTile(aiTileX, aiTileY);
		if (tile.isValid() && aiTile.isValid() )
		{
			int32 aiPlayerId = sim.tileOwner(aiTile);
			if (sim.IsAIPlayer(aiPlayerId))
			{
				int32 provinceId = sim.GetProvinceIdClean(tile);
				auto command = make_shared<FClaimLand>();
				command->playerId = aiPlayerId;
				command->claimEnum = static_cast<CallbackEnum>(claimEnum);
				command->provinceId = provinceId;
				PUN_CHECK(command->provinceId != -1);

				gameManager->simulation().ClaimLand(*command);
			}
		}
	}

	UFUNCTION(Exec) void PlaceBuilding(const FString& buildingName, int32 tileX, int32 tileY)
	{
		std::string name = ToStdString(buildingName);
		CardEnum buildingEnum = FindCardEnumByName(name);

		int32 tileOwner = simulation().tileOwner(WorldTile2(tileX, tileY));
		if (tileOwner == -1) {
			PUN_LOG("Invalid tileOwner");
			return;
		}

		auto command = make_shared<FPlaceBuilding>();
		command->playerId = tileOwner;
		command->buildingEnum = static_cast<uint8>(buildingEnum);
		command->buildingLevel = 1;
		command->center = WorldTile2(tileX, tileY);
		command->area = BuildingArea(command->center, GetBuildingInfo(buildingEnum).size, Direction::S);
		command->faceDirection = static_cast<uint8_t>(Direction::S);

		SendNetworkCommand(command);
	}

	UFUNCTION(Exec) void SpawnDrop(const FString& resourceName, int32 count, int32 tileX, int32 tileY)
	{
		std::string resourceNameStd = ToStdString(resourceName);
		simulation().resourceSystem(playerId()).SpawnDrop(FindResourceEnumByName(resourceNameStd), count, WorldTile2(tileX, tileY));
	}

	UFUNCTION(Exec) void GoToMaxAnimal()
	{
		int32 provinceId = simulation().regionSystem().debugMaxAnimalCountProvinceId;
		if (provinceId == -1) {
			return;
		}

		SetCameraAtom(simulation().GetProvinceCenterTile(provinceId).worldAtom2());

		FRotator rotation = cameraPawn->GetActorRotation();
		rotation.Yaw = 0;
		cameraPawn->SetActorRotation(rotation);
	}

	/*
	 * Test Print Debug
	 */
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

	UFUNCTION(Exec) void PrintMeshPoolCount() {
		gameManager->PrintMeshPoolCount();
	}

	// Photo taking
	// !!! In-Game Only
	UFUNCTION(Exec) void SetShadowDistanceMultiplier(float multiplier) {
		gameManager->ShadowDistanceMultiplier = multiplier;
	}
	UFUNCTION(Exec) void SetMaxRegionCullDistance(float maxRegionCullDistance) {
		gameManager->MaxRegionCullDistance = maxRegionCullDistance;
	}

	UFUNCTION(Exec) float GetLightAngle() override {
		return gameManager->directionalLight()->GetActorRotation().Yaw;
	}
	UFUNCTION(Exec) void SetLightAngle(float lightAngle) override {
		gameManager->directionalLight()->SetActorRotation(FRotator(308, lightAngle, 0));
		//PUN_LOG("SetLightAngle %f", lightAngle);
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
	 * Auto Build
	 */
	UFUNCTION(Exec) void AutoBuildHouse(int32 count)
	{
		if (simulation().HasTownhall(playerId())) 
		{
			for (int32 i = 0; i < count; i++)
			{
				AICityBlock block;

				block.topBuildingEnums = { CardEnum::House, CardEnum::House };
				block.bottomBuildingEnums = { CardEnum::House, CardEnum::House };
				block.CalculateSize();
				SimUtils::TryPlaceArea(block, simulation().townhallGateTile(playerId()), playerId(), &(simulation()), 20000);
			}
		}
	}

	UFUNCTION(Exec) void AutoBuildFarm(int32 count)
	{
		if (simulation().HasTownhall(playerId()))
		{
			for (int32 i = 0; i < count; i++)
			{
				AICityBlock block;
				block.topBuildingEnums = { CardEnum::Farm, CardEnum::Farm , CardEnum::StorageYard };
				block.bottomBuildingEnums = { CardEnum::Farm, CardEnum::Farm, CardEnum::StorageYard };
				block.CalculateSize();
				SimUtils::TryPlaceArea(block, simulation().townhallGateTile(playerId()), playerId(), &(simulation()), 20000);
			}
		}
	}

	UFUNCTION(Exec) void AutoBuild()
	{
		auto& sim = simulation();
		auto& provinceSys = sim.provinceSystem();
		sim.ChangeMoney(playerId(), 50000);

		// Claim X pieces of land
		int32 landClaimTarget = 15;
		int32 landClaimed = 0;
		while (landClaimed < landClaimTarget)
		{
			int32 claimedThisRound = 0;
			std::vector<int32> provincesClaimed = sim.playerOwned(playerId()).provincesClaimed();
			for (int32 provinceId : provincesClaimed) {
				provinceSys.ExecuteAdjacentProvinces(provinceId, [&](ProvinceConnection connection) 
				{
					if (sim.IsProvinceValid(connection.provinceId) &&
						sim.provinceOwner(connection.provinceId) == -1)
					{
						auto command = SimUtils::MakeCommand<FClaimLand>(playerId());
						command->provinceId = connection.provinceId;
						sim.ExecuteNetworkCommand(command);
						
						landClaimed++;
						claimedThisRound++;
					}
				});
			}

			if (claimedThisRound == 0) {
				break;
			}
		}
		
		SimSettings::Set("CheatFastBuild", 1);
		for (int32 i = 0; i < 4; i++) {
			AutoBuildHouse(3);
			AutoBuildFarm(3);
		}
		auto command = make_shared<FCheat>();
		command->cheatEnum = GetCheatEnum("AddImmigrants");
		command->var1 = 150;
		networkInterface()->SendNetworkCommand(command);
	}

	/*
	 * Trailer
	 */
	UFUNCTION(Exec) void TrailerSession()
	{
		auto& sim = gameManager->simulation();
		
		PunSettings::TrailerSession = true;
		SimSettings::Set("CheatFastBuild", 1);
		SetLightAngle(225);
		sim.unlockSystem(playerId())->UnlockAll();
		sim.ClearPopups(playerId());

		AddPenguinColony(396, 2448, 10, 200);

		// Wildman
		{
			WorldTile2 center(1013, 2601); // WorldTile2(988, 2537);
			
			FPlaceBuilding placeCommand;
			placeCommand.buildingEnum = static_cast<uint8>(CardEnum::FakeTribalVillage);
			placeCommand.buildingLevel = 0;
			placeCommand.center = center;
			placeCommand.faceDirection = static_cast<uint8>(Direction::S);
			placeCommand.area = BuildingArea(placeCommand.center, GetBuildingInfoInt(placeCommand.buildingEnum).size, static_cast<Direction>(placeCommand.faceDirection));
			int32 buildingId = sim.PlaceBuilding(placeCommand);
			
			sim.building(buildingId).InstantClearArea();
			sim.building(buildingId).FinishConstruction();

			AddWildManColony(center.x, center.y, 12, 33);

			SimUtils::PerlinRadius_ExecuteOnArea_WorldTile2(center, 17, &sim, [&](int32 chancePercent, WorldTile2 tile) {
				if (chancePercent > 25) {
					sim.treeSystem().ForceRemoveTileObj(tile, false);
				}
			});
		}

		// Hippo
		{
			WorldTile2 center(1014, 2690);
			int32 radius = 23;

			SimUtils::PerlinRadius_ExecuteOnArea_WorldTile2(center, radius, &sim, [&](int32 chancePercent, WorldTile2 tile)
			{
				if (chancePercent > 20) {
					sim.treeSystem().ForceRemoveTileObj(tile, false);
				}
			});

			AddHippoColony(center.x, center.y, radius, 9);
		}

		// Chichen
		{
			FPlaceBuilding placeCommand;
			placeCommand.buildingEnum = static_cast<uint8>(CardEnum::ChichenItza);
			placeCommand.buildingLevel = 0;
			placeCommand.center = WorldTile2(1057, 2754);
			placeCommand.faceDirection = static_cast<uint8>(Direction::S);
			placeCommand.area = BuildingArea(placeCommand.center, GetBuildingInfoInt(placeCommand.buildingEnum).size, static_cast<Direction>(placeCommand.faceDirection));
			int32 buildingId = sim.PlaceBuilding(placeCommand);
			sim.building(buildingId).InstantClearArea();
			sim.building(buildingId).FinishConstruction();
		}


		// Chop/Build area clear
		{
			sim.treeSystem().ForceRemoveTileObjArea(TileArea(701, 2614, 705, 2619)); // Chop
			sim.treeSystem().ForceRemoveTileObjArea(TileArea(708, 2627, 712, 2631)); // Build
		}

		PunSettings::Set("ForceAutumn", 0);

		PunSettings::TrailerAtomStart_Ship = WorldAtom2::Invalid;
		PunSettings::TrailerAtomTarget_Ship = WorldAtom2::Invalid;
	}

	UFUNCTION(Exec) void RestartGame()
	{
		gameInstance()->UnreadyAll(); // Unready all, so readyStates can be used to determine if the player is loaded
		gameInstance()->DebugPunSync("Traveling to GameMap");
		gameInstance()->CachePlayerNames();

		TrailerModeStop();
		
		GetWorld()->ServerTravel("/Game/Maps/GameMap");
	}
	
	UFUNCTION(Exec) void TrailerCityReplayUnpause() override {
		gameManager->simulation().replaySystem().TrailerCityReplayUnpause();
		SetGameSpeed(2);
	}
	UFUNCTION(Exec) void TrailerShipStart() override {
		PunSettings::TrailerAtomStart_Ship = WorldTile2(351, 2432).worldAtom2();
		PunSettings::TrailerAtomTarget_Ship = WorldTile2(391, 2432).worldAtom2();
		PunSettings::TrailerShipStartTime = cameraPawn->GetTrailerTime();
		PunSettings::TrailerShipTargetTime = cameraPawn->GetTrailerTime() + PunSettings::Get("TrailerShipTime") / 100.0f;

		PUN_LOG("TrailerShipStart");
	}

	float GetCameraSystemMoveLerpFraction() override {
		return cameraPawn->GetCameraSystemMoveLerpFraction();
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
		_LOG(PunTrailer, "TrailerCameraPrint:");
		for (size_t i = 0; i < cameraRecords.size(); i++) {
			_LOG(PunTrailer, "trailerCamera %s zoom:%f rotator:%s transition:%s t_time:%f",
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

		auto& sim = gameManager->simulation();
		
		TArray<TSharedPtr<FJsonValue>> cameraRecordJsons;

		float startTime = -2.0f; // 2 sec delay...
		
		for (size_t i = 0; i < cameraRecords.size(); i++) 
		{
			auto cameraRecordJson = MakeShared<FJsonObject>();
			
			// Camera
			cameraRecordJson->SetNumberField("TimeStart", startTime);
			startTime += cameraRecords[i].transitionTime;
			cameraRecordJson->SetNumberField("transitionTime", cameraRecords[i].transitionTime);
			cameraRecordJson->SetStringField("transition", cameraRecords[i].transition);
			
			cameraRecordJson->SetNumberField("cameraAtomX", cameraRecords[i].cameraAtom.x);
			cameraRecordJson->SetNumberField("cameraAtomY", cameraRecords[i].cameraAtom.y);
			cameraRecordJson->SetNumberField("zoomDistance", cameraRecords[i].zoomDistance);

			FRotator rotator = cameraRecords[i].rotator;
			cameraRecordJson->SetNumberField("rotatorPitch", rotator.Pitch);
			cameraRecordJson->SetNumberField("rotatorYaw", rotator.Yaw);
			cameraRecordJson->SetNumberField("rotatorRoll", rotator.Roll);

			cameraRecordJson->SetStringField("Biome", ToFString(GetBiomeInfo(sim.GetBiomeEnum(cameraRecords[i].cameraAtom.worldTile2())).name));

			cameraRecordJson->SetNumberField("unpause", static_cast<int>(cameraRecords[i].isCameraReplayUnpause));

			cameraRecordJson->SetNumberField("lightAngle", static_cast<int>(cameraRecords[i].lightAngle));

			// TileMoveDistance is useful in determining transitionTime
			float tileMoveDistance = 0;
			float zoomMoveDistance = 0;
			if (i > 0) {
				int32 atomDistance = WorldAtom2::Distance(cameraRecords[i].cameraAtom, cameraRecords[i - 1].cameraAtom);
				tileMoveDistance = static_cast<float>(atomDistance) / CoordinateConstants::AtomsPerTile;
				zoomMoveDistance = cameraRecords[i].zoomDistance - cameraRecords[i - 1].zoomDistance;
			}
			cameraRecordJson->SetNumberField("tileMoveDistance", tileMoveDistance);
			cameraRecordJson->SetNumberField("zoomMoveDistance", zoomMoveDistance);
			cameraRecordJson->SetStringField("transitionType:", (tileMoveDistance > 100.0f) ? "farTransition" : "");

			TSharedPtr<FJsonValue> jsonValue = MakeShared<FJsonValueObject>(cameraRecordJson);
			cameraRecordJsons.Add(jsonValue);
		}

		auto jsonObject = MakeShared<FJsonObject>();
		jsonObject->SetArrayField("CameraRecords", cameraRecordJsons);
		
		SaveJsonToFile(jsonObject, "Camera_" + cameraRecordFileName + ".json", true);
	}

	UFUNCTION(Exec) void TrailerCameraLoad(const FString& cameraRecordFileName)
	{
		cameraRecords.clear();
		
		TSharedPtr<FJsonObject> jsonObject = LoadJsonFromFile("Camera_" + cameraRecordFileName + ".json", true);

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
			//_LOG(PunTrailer, "TrailerCameraLoad isCameraReplayUnpause %d", record.isCameraReplayUnpause);
			
			record.lightAngle = cameraRecordJson->GetNumberField("lightAngle");
			if (record.lightAngle == 0.0f) {
				record.lightAngle = 225.0f;
			}

			cameraRecords.push_back(record);
		}

		for (int32 i = 0; i < cameraRecords.size(); i++) {
			_LOG(PunTrailer,"TrailerCameraLoad[%d] isCameraReplayUnpause:%d", i, cameraRecords[i].isCameraReplayUnpause);
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
		std::vector<std::shared_ptr<FNetworkCommand>>& commands = sim.replaySystem().trailerCommandsSave;
		PUN_CHECK(commands.size() > 1);
		if (commands.size() <= 1) {
			return;
		}

		// Trim off buildings/road that was already demolished.
		for (size_t i = commands.size(); i-- > 0;)
		{
			if (commands[i]->commandType() == NetworkCommandEnum::PlaceBuilding) {
				auto command = std::static_pointer_cast<FPlaceBuilding>(commands[i]);

				// Check by buildingId
				bool isDemolished = command->area.ExecuteOnAreaWithExit_WorldTile2([&](WorldTile2 tile) {
					if (static_cast<CardEnum>(command->buildingEnum) != sim.buildingEnumAtTile(tile)) {
						return true;
					}
					Building* building = sim.buildingAtTile(tile);
					return command->center != building->centerTile();
				});
				if (isDemolished) {
					commands.erase(commands.begin() + i);
				}
			}
			// UpgradeBuilding is only for townhall
			else if (commands[i]->commandType() == NetworkCommandEnum::UpgradeBuilding)
			{
				auto command = std::static_pointer_cast<FUpgradeBuilding>(commands[i]);

#if TRAILER_MODE
				if (command->buildingId == -1) {
					command->buildingId = sim.buildingIdAtTile(WorldTile2(command->tileId));
				}
#endif
				
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
				case CheatEnum::TrailerPlaceSpeed:
				case CheatEnum::TrailerHouseUpgradeSpeed:
				case CheatEnum::TrailerRoadPerTick:
				case CheatEnum::TrailerForceAutumn:
				case CheatEnum::TrailerBeatShiftBack:
				case CheatEnum::TrailerTimePerBeat:
					break;
				default:
					commands.erase(commands.begin() + i);
					break;
				}
			}
		}


		// Print Road
		_LOG(PunTrailer, "Print Road Backward")
		for (size_t i = commands.size(); i-- > 1;)
		{
			auto& command = commands[i];
			if (command->commandType() == NetworkCommandEnum::PlaceDrag)
			{
				auto commandCasted = std::static_pointer_cast<FPlaceDrag>(command);
				for (int32 j = 0; j < commandCasted->path.Num(); j++) {
					_LOG(PunTrailer, " - command:%d %s", i, ToTChar(WorldTile2(commandCasted->path[j]).ToString()));
				}
			}
		}
		

		// Merge PlaceDrag for roads
		for (size_t i = commands.size(); i-- > 1;)
		{
			auto& command = commands[i];
			auto& prevCommand = commands[i - 1];

			if (command->commandType() == NetworkCommandEnum::PlaceDrag &&
				prevCommand->commandType() == NetworkCommandEnum::PlaceDrag)
			{
				auto commandCasted = std::static_pointer_cast<FPlaceDrag>(command);
				auto prevCommandCasted = std::static_pointer_cast<FPlaceDrag>(prevCommand);

				if (commandCasted->placementType == static_cast<int32>(PlacementType::DirtRoad) &&
					prevCommandCasted->placementType == static_cast<int32>(PlacementType::DirtRoad))
				{
					// Possibility of duplication since we can build road on road tile
					TArray<int32> newPath;
					for (int32 j = 0; j < commandCasted->path.Num(); j++) {
						if (!prevCommandCasted->path.Contains(commandCasted->path[j])) {
							newPath.Add(commandCasted->path[j]);
						}
					}

					// Merge the path to previous path only if it is connected
					if (newPath.Num() > 0) {
						if (WorldTile2(newPath.Last()).Is8AdjacentTo(prevCommandCasted->path[0]))
						{
							newPath.Append(prevCommandCasted->path);
							prevCommandCasted->path = newPath;
							commands.erase(commands.begin() + i);
						}
					}
					
					
				}
			}
		}


		

		PUN_CHECK(commands[0]->commandType() == NetworkCommandEnum::ChooseLocation);

		std::vector<float> commandIssueTimeVec = sim.replaySystem().trailerCommandsSaveIssueTime;
		

		TArray<TSharedPtr<FJsonValue>> jsons;

		WorldTile2 lastBuildTile = WorldTile2::Invalid;
		
		for (size_t i = 0; i < commands.size(); i++)
		{
			auto jsonObj = MakeShared<FJsonObject>();

			auto tryAddTime = [&]() {
				if (i < commandIssueTimeVec.size()) {
					jsonObj->SetNumberField("time", commandIssueTimeVec[i]);
				}
			};

			if (commands[i]->commandType() == NetworkCommandEnum::ChooseLocation)
			{
				auto command = std::static_pointer_cast<FChooseLocation>(commands[i]);
				tryAddTime();
				
				jsonObj->SetStringField("commandType", "ChooseLocation");

				// Save province center instead for Editor-Game compatibility
				WorldTile2 tile = sim.GetProvinceCenterTile(command->provinceId);
				jsonObj->SetNumberField("provinceCenterX", tile.x);
				jsonObj->SetNumberField("provinceCenterY", tile.y);
				
				jsonObj->SetNumberField("isChoosingOrReserving", command->isChoosingOrReserving);
			}
			else if (commands[i]->commandType() == NetworkCommandEnum::PlaceBuilding)
			{
				auto command = std::static_pointer_cast<FPlaceBuilding>(commands[i]);
				tryAddTime();

				if (lastBuildTile.isValid() && WorldTile2::Distance(lastBuildTile, command->center) > 100) {
					jsonObj->SetStringField("scene transition", "--------------------------");
				}
				lastBuildTile = command->center;
				
				
				jsonObj->SetStringField("commandType", "PlaceBuilding");
				
				jsonObj->SetNumberField("buildingEnum", command->buildingEnum);
				jsonObj->SetStringField("buildingName", ToFString(GetBuildingInfoInt(command->buildingEnum).name));
				
				jsonObj->SetNumberField("buildingLevel", command->buildingLevel);

				SetAreaField(jsonObj, "area", command->area);
				//SetAreaField(jsonObj, "area2", command->area2);

				jsonObj->SetNumberField("center_x", command->center.x);
				jsonObj->SetNumberField("center_y", command->center.y);

				jsonObj->SetNumberField("faceDirection", command->faceDirection);
				jsonObj->SetNumberField("prebuilt", command->area2.minX); // Area2.minX to keep prebuilt
				
				jsonObj->SetStringField("biome", ToFString(GetBiomeInfo(sim.GetBiomeEnum(command->center)).name));
			}
			else if (commands[i]->commandType() == NetworkCommandEnum::PlaceDrag)
			{
				auto command = std::static_pointer_cast<FPlaceDrag>(commands[i]);

				// Remove demolished path
				if (command->placementType == static_cast<int32>(PlacementType::DirtRoad)) {
					for (int32 j = command->path.Num(); j-- > 0;) {
						WorldTile2 pathTile(command->path[j]);
						if (!sim.overlaySystem().IsRoad(pathTile)) {
							command->path.RemoveAt(j);
						}
					}
				}
				if (command->path.Num() == 0) {
					continue;
				}
				
				tryAddTime();

				jsonObj->SetStringField("commandType", "PlaceDrag");

				SetAreaField(jsonObj, "area", command->area);
				//SetAreaField(jsonObj, "area2", command->area2);

				TArray<TSharedPtr<FJsonValue>> pathJsonValues;
				for (int32 j = 0; j < command->path.Num(); j++) {
					pathJsonValues.Add(MakeShared<FJsonValueNumber>(command->path[j]));
				}
				jsonObj->SetArrayField("path", pathJsonValues);

				
				TArray<TSharedPtr<FJsonValue>> pathJsonValuesX;
				for (int32 j = 0; j < command->path.Num(); j++) {
					pathJsonValuesX.Add(MakeShared<FJsonValueNumber>(WorldTile2(command->path[j]).x));
				}
				jsonObj->SetArrayField("pathX", pathJsonValuesX);
				TArray<TSharedPtr<FJsonValue>> pathJsonValuesY;
				for (int32 j = 0; j < command->path.Num(); j++) {
					pathJsonValuesY.Add(MakeShared<FJsonValueNumber>(WorldTile2(command->path[j]).y));
				}
				jsonObj->SetArrayField("pathY", pathJsonValuesY);

				
				jsonObj->SetNumberField("pathSize", command->path.Num()); // For calculating transitionTime

				jsonObj->SetNumberField("placementType", command->placementType);
				jsonObj->SetNumberField("isInstantRoad", 0); // use command->harvestResourceEnum for isInstantRoad

				
				jsonObj->SetNumberField("prebuilt", (command->area2.minX != -1) ? command->area2.minX : 0); // Area2.minX to keep prebuilt
			}
			else if (commands[i]->commandType() == NetworkCommandEnum::Cheat)
			{
				auto command = std::static_pointer_cast<FCheat>(commands[i]);
				tryAddTime();

				jsonObj->SetStringField("commandType", "Cheat");

				jsonObj->SetNumberField("cheatEnum", static_cast<double>(command->cheatEnum));
				jsonObj->SetStringField("cheatName", ToFString(GetCheatName(command->cheatEnum)));
				jsonObj->SetNumberField("var1", static_cast<double>(command->var1));
				jsonObj->SetNumberField("var2", static_cast<double>(command->var2));
			}
			else if (commands[i]->commandType() == NetworkCommandEnum::UpgradeBuilding)
			{
				auto command = std::static_pointer_cast<FUpgradeBuilding>(commands[i]);
				tryAddTime();

				jsonObj->SetStringField("commandType", "UpgradeBuilding");
				WorldTile2 center = sim.building(command->buildingId).centerTile();
				jsonObj->SetNumberField("centerX", center.x);
				jsonObj->SetNumberField("centerY", center.y);
				jsonObj->SetNumberField("upgradeLevel", command->upgradeLevel);
				jsonObj->SetNumberField("upgradeType", command->upgradeType);
			}

			if (jsonObj->Values.Num() > 0) {
				TSharedPtr<FJsonValue> jsonValue = MakeShared<FJsonValueObject>(jsonObj);
				jsons.Add(jsonValue);
			}
		}

		auto jsonObject = MakeShared<FJsonObject>();
		jsonObject->SetArrayField("TrailerCommands", jsons);

		SaveJsonToFile(jsonObject, "City_" + trailerCityName + ".json", true);

		_LOG(PunTrailer, "TrailerCitySave %d", commands.size());
		for (size_t i = 0; i < commands.size(); i++) {
			_LOG(PunTrailer, "  trailer[%d] %s", i, *commands[i]->ToCompactString());
		}
	}
	UFUNCTION(Exec) void TrailerCityPrintSave() {
		auto commands = gameManager->simulation().replaySystem().trailerCommandsSave;
		_LOG(PunTrailer, "TrailerCityPrintSave %d", commands.size());
		for (size_t i = 0; i < commands.size(); i++) {
			_LOG(PunTrailer, "  trailer[%d] %s", i, *commands[i]->ToCompactString());
		}
	}

	UFUNCTION(Exec) void TrailerCityClearSave() {
		gameManager->simulation().replaySystem().trailerCommandsSave.clear();
	}


	UFUNCTION(Exec) void TrailerCityLoad(const FString& trailerCityName)
	{
		std::vector<std::shared_ptr<FNetworkCommand>>& commands = gameManager->simulation().replaySystem().trailerCommandsSave;
		commands.clear();

		TSharedPtr<FJsonObject> jsonObject = LoadJsonFromFile("City_" + trailerCityName + ".json", true);

		const TArray<TSharedPtr<FJsonValue>>& commandsJson = jsonObject->GetArrayField("TrailerCommands");
		for (const TSharedPtr<FJsonValue>& commandValue : commandsJson)
		{
			TSharedPtr<FJsonObject> jsonObj = commandValue->AsObject();

			auto& sim = gameManager->simulation();

			if (jsonObj->GetStringField("commandType") == "ChooseLocation") 
			{
				auto command = std::make_shared<FChooseLocation>();
				WorldTile2 provinceCenter(FGenericPlatformMath::RoundToInt(jsonObj->GetNumberField("provinceCenterX")),
											FGenericPlatformMath::RoundToInt(jsonObj->GetNumberField("provinceCenterY")));
				command->provinceId = sim.GetProvinceIdClean(provinceCenter);
				
				command->isChoosingOrReserving = jsonObj->GetNumberField("isChoosingOrReserving");

				commands.push_back(command);
			}
			else if (jsonObj->GetStringField("commandType") == "PlaceBuilding") 
			{
				auto command = std::make_shared<FPlaceBuilding>();

				command->buildingEnum = jsonObj->GetNumberField("buildingEnum");

				command->buildingLevel = jsonObj->GetNumberField("buildingLevel");

				GetAreaField(jsonObj, "area", command->area);
				//GetAreaField(jsonObj, "area2", command->area2);

				command->center.x = jsonObj->GetNumberField("center_x");
				command->center.y = jsonObj->GetNumberField("center_y");

				command->faceDirection = jsonObj->GetNumberField("faceDirection");

				command->area2.minX = jsonObj->GetNumberField("prebuilt");

				commands.push_back(command);
			}
			else if (jsonObj->GetStringField("commandType") == "PlaceDrag") 
			{
				auto command = std::make_shared<FPlaceDrag>();

				GetAreaField(jsonObj, "area", command->area);
				//GetAreaField(jsonObj, "area2", command->area2);

				const TArray<TSharedPtr<FJsonValue>>& pathJson = jsonObj->GetArrayField("path");
				for (int32 i = 0; i < pathJson.Num(); i++) {
					command->path.Add(pathJson[i]->AsNumber());
				}

				command->placementType = jsonObj->GetNumberField("placementType");

				// Demolish, don't do it!!!
				if (command->placementType == static_cast<int32>(PlacementType::Demolish)) {
					continue;
				}
				
				command->harvestResourceEnum = static_cast<ResourceEnum>(FGenericPlatformMath::RoundToInt(jsonObj->GetNumberField("isInstantRoad")));

				command->area2.minX = jsonObj->GetNumberField("prebuilt");

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

#if TRAILER_MODE
				WorldTile2 center(jsonObj->GetNumberField("centerX"),
									jsonObj->GetNumberField("centerY"));
				command->tileId = center.tileId();
#endif
				
				command->buildingId = -1;
				command->upgradeLevel = jsonObj->GetNumberField("upgradeLevel");
				command->upgradeType = jsonObj->GetNumberField("upgradeType");

				commands.push_back(command);
			}
		}

		_LOG(PunTrailer, "TrailerCityLoad %d", commands.size());
		for (size_t i = 0; i < commands.size(); i++) {
			_LOG(PunTrailer, "  trailer[%d] %s", i, *commands[i]->ToCompactString());
		}
	}

	UFUNCTION(Exec) void TrailerCityStart(const FString& trailerCityName)
	{
		TrailerCityLoad(trailerCityName);
		
		auto& replaySys = gameManager->simulation().replaySystem();
		
		replaySys.replayPlayers[0].SetTrailerCommands(replaySys.trailerCommandsSave);

		replaySys.trailerCommandsSave.clear(); // Clear old saves since new commands will be written to this
		replaySys.lastTrailerStartTime = UGameplayStatics::GetTimeSeconds(this);

		PunSettings::SetTrailerMode(1);
	}
	UFUNCTION(Exec) void TrailerModeStop() {
		PunSettings::SetTrailerMode(0);
	}
	UFUNCTION(Exec) void TrailerModeStart() {
		PunSettings::SetTrailerMode(1);
	}

	UFUNCTION(Exec) void TrailerCityPause(int32 playerId)
	{
		auto& replaySys = gameManager->simulation().replaySystem();
		replaySys.replayPlayers[playerId].PauseTrailerCommands();
		PunSettings::SetTrailerMode(0);
	}
	UFUNCTION(Exec) void TrailerCityUnpause(int32 playerId)
	{
		auto& replaySys = gameManager->simulation().replaySystem();
		replaySys.replayPlayers[playerId].UnpauseTrailerCommands();
		PunSettings::SetTrailerMode(1);
	}

	UFUNCTION(Exec) void TrailerStartAll(const FString& trailerName)
	{
		if (!PunSettings::TrailerSession) {
			TrailerSession();
		}
		
		SetGameSpeed(GameSpeedHalf);
		TrailerCityStart(trailerName);
		
		TrailerCameraLoad(trailerName);
		TrailerCameraStart();
		
		GetPunHUD()->HideMainGameUI(); // TODO: why not working?
	}
	UFUNCTION(Exec) void TrailerStartCityOnly(const FString& trailerName)
	{
		SetGameSpeed(GameSpeedHalf);
		TrailerCityStart(trailerName);
	}

	UFUNCTION(Exec) void AbandonTown(int32 playerId) {
		if (gameManager->simulation().HasTownhall(playerId)) {
			gameManager->simulation().AbandonTown(playerId);
		}
	}

	UFUNCTION(Exec) void PrintHouseUpgrade()
	{
		auto& bldIds = gameManager->simulation().buildingIds(playerId(), CardEnum::House);
		for (int32 bldId : bldIds) {
			House& house = gameManager->simulation().building<House>(bldId);
			_LOG(PunTrailer, "House trailerLvl:%d houseLvl:%d", house.trailerTargetHouseLvl, house.houseLvl());
		}
	}


	UFUNCTION(Exec) void AddHippoColony(int32 centerX, int32 centerY, int32 radius, int32 chancePercentMultiplier) {
		gameManager->simulation().regionSystem().AddAnimalColony(UnitEnum::Hippo, WorldTile2(centerX, centerY), radius, chancePercentMultiplier);
	}
	UFUNCTION(Exec) void AddPenguinColony(int32 centerX, int32 centerY, int32 radius, int32 chancePercentMultiplier) {
		gameManager->simulation().regionSystem().AddAnimalColony(UnitEnum::Penguin, WorldTile2(centerX, centerY), radius, chancePercentMultiplier);
	}
	UFUNCTION(Exec) void AddWildManColony(int32 centerX, int32 centerY, int32 radius, int32 chancePercentMultiplier) {
		gameManager->simulation().regionSystem().AddAnimalColony(UnitEnum::WildMan, WorldTile2(centerX, centerY), radius, chancePercentMultiplier);
	}
	UFUNCTION(Exec) void ClearHippoColony() {
		gameManager->simulation().regionSystem().RemoveAnimalColony(UnitEnum::Hippo);
	}
	UFUNCTION(Exec) void ClearPenguinColony() {
		gameManager->simulation().regionSystem().RemoveAnimalColony(UnitEnum::Penguin);
	}
	UFUNCTION(Exec) void ClearWildManColony() {
		gameManager->simulation().regionSystem().RemoveAnimalColony(UnitEnum::WildMan);
	}

	UFUNCTION(Exec) void CheckWindmill()
	{
		std::vector<int32> buildingIds = simulation().buildingIds(0, CardEnum::Windmill);
		_LOG(PunTrailer, "Check Windmill %d", buildingIds.size());
		if (buildingIds.size() > 0) {
			Building& building = simulation().building(buildingIds[0]);
			_LOG(PunTrailer, "Windmill %s", ToTChar(building.centerTile().ToString()));
			simulation().buildingSubregionList().ExecuteRegion(building.centerTile().region(), [&](int32 buildingIdLocal) {
				if (simulation().building(buildingIdLocal).buildingEnum() == CardEnum::Windmill) {
					_LOG(PunTrailer, "buildingSubregionList Windmill %s", ToTChar(simulation().building(buildingIdLocal).centerTile().ToString())); //
				}
			});
		}
	}

	UFUNCTION(Exec) void SetApplicationScale(float NewValue) {
		FSlateApplication::Get().SetApplicationScale(NewValue);
	}


	float GetTrailerTime() override {
		return cameraPawn->GetTrailerTime();
	}

	
	
	// Done for map transition so tick doesn't happpen after gameInst's data was cleared
	void SetTickDisabled(bool tickDisabled) final {
		_tickDisabled = tickDisabled;
	}

	IGameSimulationCore& simulation() {
		return gameManager->simulation();
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
