// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"

#include "PunCity/PunUtils.h"
#include "PunWidget.h"
#include "PlayerListElementUI.h"

#include "LobbyUI.generated.h"

/**
 * 
 */
UCLASS()
class ULobbyUI : public UPunWidget
{
	GENERATED_BODY()
public:
	void Init();
	void Tick();

	UPROPERTY(meta = (BindWidget)) UOverlay* GameStartBlocker;

	UPROPERTY(meta = (BindWidget)) UEditableTextBox* NameInputBox;
	UPROPERTY(meta = (BindWidget)) UScrollBox* PlayersInfoBox;

	UPROPERTY(meta = (BindWidget)) UEditableTextBox* InitialAnimalsInputBox;

	class AMainMenuPlayerController* GetFirstController();

	void SendMapSettings();

	/*
	 * Lobby UI
	 */

	UPROPERTY(meta = (BindWidget)) URichTextBlock* MultiplayerLobbyTitle;

	UPROPERTY(meta = (BindWidget)) UOverlay* PlayerListOverlay;
	UPROPERTY(meta = (BindWidget)) UOverlay* ChatOverlay;

	UPROPERTY(meta = (BindWidget)) UButton* LobbyBackButton;
	UPROPERTY(meta = (BindWidget)) UVerticalBox* LobbyPlayerListBox;
	UPROPERTY(meta = (BindWidget)) UButton* LobbyStartGameButton;
	UPROPERTY(meta = (BindWidget)) UTextBlock* StartGameButtonText;

	UPROPERTY(meta = (BindWidget)) UScrollBox* LobbyChatScrollBox;
	UPROPERTY(meta = (BindWidget)) URichTextBlock* LobbyChatContentRichText;
	UPROPERTY(meta = (BindWidget)) UEditableTextBox* LobbyChatInputBox;

	UPROPERTY(meta = (BindWidget)) UEditableTextBox* LobbyMapSeedInputBox;
	UPROPERTY(meta = (BindWidget)) UComboBoxString* LobbyMapSizeDropdown;
	UPROPERTY(meta = (BindWidget)) UComboBoxString* LobbyAICountDropdown;
	UPROPERTY(meta = (BindWidget)) UComboBoxString* LobbyDifficultyDropdown;

	UPROPERTY(meta = (BindWidget)) UImage* SettingsBackgroundImage;
	UPROPERTY(meta = (BindWidget)) UTextBlock* LobbyMapSeedText;
	UPROPERTY(meta = (BindWidget)) UTextBlock* LobbyMapSizeText;
	UPROPERTY(meta = (BindWidget)) UTextBlock* LobbyAICountText;
	UPROPERTY(meta = (BindWidget)) UTextBlock* LobbyDifficultyText;

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

	void AddPopup(std::string message) {
		LobbyPopupOverlay->SetVisibility(ESlateVisibility::Visible);
		SetText(LobbyPopupText, message);
	}

	//bool isSinglePlayer() { return serverMapSettings.isSinglePlayer; }

public:
	void UpdateLobbyUI();

	void CallBack1(UPunWidget* punWidgetCaller, CallbackEnum callbackEnum) override;
	
public:
	UFUNCTION() void OnChatInputBoxTextCommitted(const FText& text, ETextCommit::Type CommitMethod);

	UFUNCTION() void OnLobbyMapSeedInputBoxTextCommitted(const FText& text, ETextCommit::Type CommitMethod);
	UFUNCTION() void OnLobbyMapSizeDropdownChanged(FString sItem, ESelectInfo::Type seltype);
	UFUNCTION() void OnLobbyAICountDropdownChanged(FString sItem, ESelectInfo::Type seltype);
	UFUNCTION() void OnLobbyDifficultyDropdownChanged(FString sItem, ESelectInfo::Type seltype);

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
		std::string mapSeed = mapSettings.mapSeedStd();
		WorldRegion2 regionPerWorld = GetMapSize(mapSettings.mapSizeEnum());
		GameMapConstants::SetRegionsPerWorld(regionPerWorld.x, regionPerWorld.y);

		PUN_DEBUG2("GenerateWorld %d, (%d %d)", mapSettings.mapSizeEnumInt, GameMapConstants::TilesPerWorldX, GameMapConstants::TilesPerWorldY);
		
		_terrainGenerator = make_unique<PunTerrainGenerator>();
		_terrainGenerator->Init(nullptr, GameMapConstants::TilesPerWorldX, GameMapConstants::TilesPerWorldY, 0, mapSettings.mapSizeEnum(), nullptr);
		
		_isGeneratingTerrain = true;
		
		PunTerrainGenerator* terrainGenerator = _terrainGenerator.get();
		_worldInitCompletedFuture = Async(EAsyncExecution::Thread, [mapSeed, terrainGenerator]() {
			terrainGenerator->CalculateWorldTerrain(mapSeed);
			return static_cast<uint8>(false);
		});

		GenerateWorldBar->GetDynamicMaterial()->SetScalarParameterValue("Fraction", 0.05f);
		gameInstance()->Spawn2DSound("UI", "ButtonClick");
	}

	void Unready();
	void CheckMapReady();

	std::unique_ptr<PunTerrainGenerator> _terrainGenerator;
	
private:
	UFUNCTION() void InputBoxChange_InitialAnimals(const FText& text);

	UFUNCTION() void ReturnToMainMenu();
	UFUNCTION() void OnClickLobbyStartGameButton();

	void LobbyStartGame();

	void RefreshAICountDropdown()
	{
		int32 selectedIndex = LobbyAICountDropdown->GetSelectedIndex();
		if (selectedIndex == -1) {
			selectedIndex = LobbyAICountDropdown->GetOptionCount() - 1;
		}
		
		LobbyAICountDropdown->ClearOptions();

		int maxAICount = GameConstants::MaxAIs;
		switch(serverMapSettings.mapSizeEnum()) {
			case MapSizeEnum::Medium: maxAICount = 8; break;
			case MapSizeEnum::Small: maxAICount = 3; break;
		}
		
		for (int32 i = 0; i <= maxAICount; i++) {
			LobbyAICountDropdown->AddOption(FString::FromInt(i));
		}

		if (selectedIndex > maxAICount) {
			selectedIndex = maxAICount-1;
		}
		LobbyAICountDropdown->SetSelectedIndex(selectedIndex);
	}

	// TODO: get rid of ULobbyPlayerInfoUI
	//TSubclassOf<class UUserWidget> _playerInfoUIClass;
	//TArray<class ULobbyPlayerInfoUI*> _playerInfos;

	UPROPERTY() TArray<UPlayerListElementUI*> _playerListElements;

private:
	FMapSettings serverMapSettings; // This is only used for server
	FMapSettings clientLastMapSettings;
	bool clientReadyState = false;

	float lastSendMapSettingsTime = 0.0f;

	bool isCountingDown = false;
	float countdownTime = 0.0f;
};
