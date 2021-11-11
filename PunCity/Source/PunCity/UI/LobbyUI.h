// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "LobbySettingsUI.h"
#include "PunButtonImage.h"
#include "W_PlayerCharacterInfo.h"

#include "LobbyUI.generated.h"

/**
 * 
 */
UCLASS()
class ULobbyUI : public UPunWidget
{
	GENERATED_BODY()
public:
	void Init(UMainMenuAssetLoaderComponent* maimMenuAssetLoaderIn);
	void Tick();

	UPROPERTY(meta = (BindWidget)) UOverlay* GameStartBlocker;

	//UPROPERTY(meta = (BindWidget)) UEditableTextBox* NameInputBox;
	//UPROPERTY(meta = (BindWidget)) UScrollBox* PlayersInfoBox;

	//UPROPERTY(meta = (BindWidget)) UEditableTextBox* InitialAnimalsInputBox;

	/*
	 * Lobby UI
	 */

	UPROPERTY(meta = (BindWidget)) URichTextBlock* MultiplayerLobbyTitle;

	UPROPERTY(meta = (BindWidget)) UOverlay* PlayerListOverlay;
	UPROPERTY(meta = (BindWidget)) UOverlay* ChatOverlay;

	UPROPERTY(meta = (BindWidget)) UPlayerListElementUI* SinglePlayerPortraitUI;

	UPROPERTY(meta = (BindWidget)) UButton* LobbyBackButton;
	UPROPERTY(meta = (BindWidget)) UWrapBox* LobbyPlayerListBox;
	UPROPERTY(meta = (BindWidget)) UButton* LobbyStartGameButton;
	UPROPERTY(meta = (BindWidget)) UTextBlock* StartGameButtonText;

	UPROPERTY(meta = (BindWidget)) UScrollBox* LobbyChatScrollBox;
	UPROPERTY(meta = (BindWidget)) URichTextBlock* LobbyChatContentRichText;
	UPROPERTY(meta = (BindWidget)) UEditableTextBox* LobbyChatInputBox;
	
	UPROPERTY(meta = (BindWidget)) ULobbySettingsUI* LobbySettingsUI;

	FMapSettings& serverMapSettings() { return LobbySettingsUI->serverMapSettings; }

	class AMainMenuPlayerController* GetFirstController() { return LobbySettingsUI->GetFirstController(); }
	


	UPROPERTY(meta = (BindWidget)) USizeBox* LobbyReadyBox;
	UPROPERTY(meta = (BindWidget)) UButton* LobbyReadyButton;
	UPROPERTY(meta = (BindWidget)) UImage* LobbyReadyFill;

	UPROPERTY(meta = (BindWidget)) UTextBlock* LastPlayerColumnText;

	// Generate World
	UPROPERTY(meta = (BindWidget)) UOverlay* GenerateWorldOverlay;
	UPROPERTY(meta = (BindWidget)) UImage* GenerateWorldBar;
	UPROPERTY(meta = (BindWidget)) UTextBlock* WorldReadyText;
	UPROPERTY(meta = (BindWidget)) UButton* GenerateWorldButton;
	
	TFuture<uint8> _worldInitCompletedFuture;

	bool _isGeneratingTerrain = false;
	bool _isMapReady = false;
	
	bool _isWaitingToGoBackToMainMenu = false;
	bool _isCancelingWorldGeneration = false;

	UPROPERTY(meta = (BindWidget)) UOverlay* CancelWorldGenerationOverlay;
	UPROPERTY(meta = (BindWidget)) UTextBlock* CancelWorldGenerationText;

	// Popup
	UPROPERTY(meta = (BindWidget)) UOverlay* LobbyPopupOverlay;
	UPROPERTY(meta = (BindWidget)) UTextBlock* LobbyPopupText;
	UPROPERTY(meta = (BindWidget)) UButton* LobbyPopupCloseButton;

public:
	// Choose Logo
	UPROPERTY(meta = (BindWidget)) UOverlay* LobbyChooseLogoOverlay;
	UPROPERTY(meta = (BindWidget)) UButton* LobbyChooseLogoDoneButton;

	UPROPERTY(meta = (BindWidget)) UW_PlayerCharacterInfo* PlayerCharacterInfoUI;
	//UPROPERTY(meta = (BindWidget)) UImage* LogoPreviewImage;
	//UPROPERTY(meta = (BindWidget)) UImage* CharacterPreviewImage;
	//UPROPERTY(meta = (BindWidget)) UTextBlock* PlayerSettingsPreviewFactionName;
	//UPROPERTY(meta = (BindWidget)) UTextBlock* PlayerSettingsPreviewPlayerName;
	
	UPROPERTY(meta = (BindWidget)) UButton* ChooseFactionSectionButton;
	UPROPERTY(meta = (BindWidget)) UButton* ChooseIconSectionButton;
	UPROPERTY(meta = (BindWidget)) UButton* ChooseIconColorSectionButton;
	UPROPERTY(meta = (BindWidget)) UButton* ChooseCharacterSectionButton;

	UPROPERTY(meta = (BindWidget)) UWrapBox* ChooseFactionWrapBox;
	UPROPERTY(meta = (BindWidget)) UButton* ChooseFactionButton1;
	UPROPERTY(meta = (BindWidget)) UButton* ChooseFactionButton2;
	
	UPROPERTY(meta = (BindWidget)) UWrapBox* ChooseIconWrapBox;
	
	UPROPERTY(meta = (BindWidget)) UVerticalBox* ChooseIconColorOuterVerticalBox;
	UPROPERTY(meta = (BindWidget)) UWrapBox* ChooseIconColorWrapBox1;
	UPROPERTY(meta = (BindWidget)) UWrapBox* ChooseIconColorWrapBox2; // TODO: Remove?
	
	UPROPERTY(meta = (BindWidget)) UWrapBox* ChooseCharacterWrapBox;

	FPlayerInfo previewPlayerInfo;

	void SetPlayerSettingsTab(int32 index)
	{
		ChooseFactionWrapBox->SetVisibility(index == 0 ? ESlateVisibility::SelfHitTestInvisible : ESlateVisibility::Collapsed);
		ChooseIconWrapBox->SetVisibility(index == 1 ? ESlateVisibility::SelfHitTestInvisible : ESlateVisibility::Collapsed);
		ChooseIconColorOuterVerticalBox->SetVisibility(index == 2 ? ESlateVisibility::SelfHitTestInvisible : ESlateVisibility::Collapsed);
		ChooseCharacterWrapBox->SetVisibility(index == 3 ? ESlateVisibility::SelfHitTestInvisible : ESlateVisibility::Collapsed);
	}
	
	UFUNCTION() void OnClickChooseFactionSectionButton() {
		SetPlayerSettingsTab(0);
	}
	UFUNCTION() void OnClickChooseIconSectionButton() {
		SetPlayerSettingsTab(1);
	}
	UFUNCTION() void OnClickChooseIconColorSectionButton() {
		SetPlayerSettingsTab(2);
	}
	UFUNCTION() void OnClickChooseCharacterSectionButton() {
		SetPlayerSettingsTab(3);
	}

	UFUNCTION() void OnClickChooseFaction0() {
		previewPlayerInfo.factionIndex = 0;
		UpdatePreviewPlayerInfoDisplay();
	}
	UFUNCTION() void OnClickChooseFaction1() {
		previewPlayerInfo.factionIndex = 1;
		UpdatePreviewPlayerInfoDisplay();
	}

	void UpdatePreviewPlayerInfoDisplay();

	UFUNCTION() void OnClickChoosePlayerLogoCloseButton();

	
	void SavePlayerInfo();
	void LoadPlayerInfo();

public:

	void AddPopup(FText message) {
		LobbyPopupOverlay->SetVisibility(ESlateVisibility::Visible);
		SetText(LobbyPopupText, message);
	}

	//bool isSinglePlayer() { return serverMapSettings.isSinglePlayer; }

	void UpdateLobbyUI();

	virtual void CallBack1(UPunWidget* punWidgetCaller, CallbackEnum callbackEnum) override;
	
public:
	UFUNCTION() void OnChatInputBoxTextCommitted(const FText& text, ETextCommit::Type CommitMethod);


	UFUNCTION() void OnClickReadyButton();

	UFUNCTION() void OnClickPopupCloseButton() {
		gameInstance()->Spawn2DSound("UI", "UIWindowClose");
		LobbyPopupOverlay->SetVisibility(ESlateVisibility::Collapsed);
	}


	UFUNCTION() void OnClickGenerateWorldButton() {
		if (_isGeneratingTerrain) {
			return;
		}
		FMapSettings mapSettings = gameInstance()->GetMapSettings();
		//std::string mapSeed = mapSettings.mapSeedStd();
		WorldRegion2 regionPerWorld = GetMapSize(mapSettings.mapSizeEnum());
		GameMapConstants::SetRegionsPerWorld(regionPerWorld.x, regionPerWorld.y);

		PUN_DEBUG2("GenerateWorld %d, (%d %d)", mapSettings.mapSizeEnumInt, GameMapConstants::TilesPerWorldX, GameMapConstants::TilesPerWorldY);
		
		_terrainGenerator = make_unique<PunTerrainGenerator>();
		_terrainGenerator->Init(nullptr, GameMapConstants::TilesPerWorldX, GameMapConstants::TilesPerWorldY, 0, mapSettings.mapSizeEnum(), nullptr);
		
		_isGeneratingTerrain = true;
		
		PunTerrainGenerator* terrainGenerator = _terrainGenerator.get();
		_worldInitCompletedFuture = Async(EAsyncExecution::Thread, [mapSettings, terrainGenerator]() {
			terrainGenerator->CalculateWorldTerrain(mapSettings);
			return static_cast<uint8>(false);
		});

		GenerateWorldBar->GetDynamicMaterial()->SetScalarParameterValue("Fraction", 0.05f);
		gameInstance()->Spawn2DSound("UI", "ButtonClick");
	}

	void Unready();
	void CheckMapReady();

	std::unique_ptr<PunTerrainGenerator> _terrainGenerator;
	
private:
	//UFUNCTION() void InputBoxChange_InitialAnimals(const FText& text);

	UFUNCTION() void ReturnToMainMenu();
	UFUNCTION() void OnClickLobbyStartGameButton();

	
	void UpdatePlayerPortraitUI(UPlayerListElementUI* element, int32 playerId, const GameSaveInfo& saveInfo);

	void LobbyStartGame_Multiplayer();

	//void RefreshAICountDropdown()
	//{
	//	int32 selectedIndex = LobbyAICountDropdown->GetSelectedIndex();
	//	if (selectedIndex == -1) {
	//		selectedIndex = LobbyAICountDropdown->GetOptionCount() - 1;
	//	}
	//	
	//	LobbyAICountDropdown->ClearOptions();

	//	int maxAICount = GameConstants::MaxAIs;
	//	switch(serverMapSettings.mapSizeEnum()) {
	//		case MapSizeEnum::Medium: maxAICount = 8; break;
	//		case MapSizeEnum::Small: maxAICount = 3; break;
	//	}
	//	
	//	for (int32 i = 0; i <= maxAICount; i++) {
	//		LobbyAICountDropdown->AddOption(FString::FromInt(i));
	//	}

	//	if (selectedIndex > maxAICount) {
	//		selectedIndex = maxAICount-1;
	//	}
	//	LobbyAICountDropdown->SetSelectedIndex(selectedIndex);
	//}

	// TODO: get rid of ULobbyPlayerInfoUI
	//TSubclassOf<class UUserWidget> _playerInfoUIClass;
	//TArray<class ULobbyPlayerInfoUI*> _playerInfos;

	//UPROPERTY() TArray<UPlayerListElementUI*> _playerListElements;


	UPROPERTY() UMainMenuAssetLoaderComponent* _mainMenuAssetLoader;

private:
	FMapSettings clientLastMapSettings;
	bool clientReadyState = false;

	bool _loadedPlayerInfo = false;

	float lastSendMapSettingsTime = 0.0f;

	bool isCountingDown = false;
	float countdownTime = 0.0f;
};
