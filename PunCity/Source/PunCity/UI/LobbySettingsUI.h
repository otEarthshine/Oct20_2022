// Pun Dumnernchanvanit's

#pragma once

#include "PunCity/PunUtils.h"
#include "PunWidget.h"
#include "PlayerListElementUI.h"

#include "LobbySettingsUI.generated.h"

/**
 * 
 */
UCLASS()
class PROTOTYPECITY_API ULobbySettingsUI : public UPunWidget
{
	GENERATED_BODY()
public:

	// Map Dropdown
	UPROPERTY(meta = (BindWidget)) UHorizontalBox* LobbyPasswordRowBox;
	UPROPERTY(meta = (BindWidget)) UEditableTextBox* LobbyPasswordInputBox;
	UPROPERTY(meta = (BindWidget)) UTextBlock* LobbyPasswordText;

	UPROPERTY(meta = (BindWidget)) UEditableTextBox* LobbyMapSeedInputBox;
	UPROPERTY(meta = (BindWidget)) UComboBoxString* LobbyMapSizeDropdown;
	UPROPERTY(meta = (BindWidget)) UComboBoxString* LobbySeaLevelDropdown;
	UPROPERTY(meta = (BindWidget)) UComboBoxString* LobbyMoistureDropdown;
	UPROPERTY(meta = (BindWidget)) UComboBoxString* LobbyTemperatureDropdown;
	UPROPERTY(meta = (BindWidget)) UComboBoxString* LobbyMountainDensityDropdown;

	UPROPERTY(meta = (BindWidget)) UComboBoxString* LobbyAICountDropdown;
	UPROPERTY(meta = (BindWidget)) UComboBoxString* LobbyDifficultyDropdown;

	// Map Text
	UPROPERTY(meta = (BindWidget)) UTextBlock* LobbyMapSeedText;
	UPROPERTY(meta = (BindWidget)) UTextBlock* LobbyMapSizeText;
	UPROPERTY(meta = (BindWidget)) UTextBlock* LobbySeaLevelText;
	UPROPERTY(meta = (BindWidget)) UTextBlock* LobbyMoistureText;
	UPROPERTY(meta = (BindWidget)) UTextBlock* LobbyTemperatureText;
	UPROPERTY(meta = (BindWidget)) UTextBlock* LobbyMountainDensityText;

	UPROPERTY(meta = (BindWidget)) UTextBlock* LobbyAICountText;
	UPROPERTY(meta = (BindWidget)) UTextBlock* LobbyDifficultyText;

	// Others
	UPROPERTY(meta = (BindWidget)) UButton* LobbyMapSeedRandomizeButton;

	// Settings Background
	UPROPERTY(meta = (BindWidget)) UImage* SettingsBackgroundImage;
	
	FMapSettings serverMapSettings; // This is only used for server

	void SetPreLobby(bool isPreLobby)
	{
		_isPreLobby = isPreLobby;

		if (_isPreLobby) {
			gameInstance()->lobbyPassword = "";
		}
	}
	
	bool isServer();

	class AMainMenuPlayerController* GetFirstController();

	void SendMapSettings();

	void InitLobbySettings(FMapSettings mapSettingsIn)
	{
		serverMapSettings = mapSettingsIn;
		
		// Only server can change settings
		if (isServer())
		{
			LobbyPasswordInputBox->OnTextCommitted.Clear();
			LobbyPasswordInputBox->OnTextCommitted.AddDynamic(this, &ULobbySettingsUI::OnLobbyPasswordInputBoxTextCommitted);

			LobbyMapSeedInputBox->OnTextCommitted.Clear();
			LobbyMapSeedInputBox->OnTextCommitted.AddDynamic(this, &ULobbySettingsUI::OnLobbyMapSeedInputBoxTextCommitted);

			LobbyMapSeedRandomizeButton->OnClicked.Clear();
			LobbyMapSeedRandomizeButton->OnClicked.AddDynamic(this, &ULobbySettingsUI::OnClickLobbyMapSeedRandomizeButton);

			LobbyMapSizeDropdown->OnSelectionChanged.Clear();
			LobbyMapSizeDropdown->OnSelectionChanged.AddDynamic(this, &ULobbySettingsUI::OnLobbyMapSizeDropdownChanged);
			// "Small" If adding another map size, also change the options on UI editor
			// Note: Using UI Editor for this to prevent ::Direct selection issue

			LobbySeaLevelDropdown->OnSelectionChanged.Clear();
			LobbySeaLevelDropdown->OnSelectionChanged.AddDynamic(this, &ULobbySettingsUI::OnLobbySeaLevelDropdownChanged);

			LobbyMoistureDropdown->OnSelectionChanged.Clear();
			LobbyMoistureDropdown->OnSelectionChanged.AddDynamic(this, &ULobbySettingsUI::OnLobbyMoistureDropdownChanged);

			LobbyTemperatureDropdown->OnSelectionChanged.Clear();
			LobbyTemperatureDropdown->OnSelectionChanged.AddDynamic(this, &ULobbySettingsUI::OnLobbyTemperatureDropdownChanged);

			LobbyMountainDensityDropdown->OnSelectionChanged.Clear();
			LobbyMountainDensityDropdown->OnSelectionChanged.AddDynamic(this, &ULobbySettingsUI::OnLobbyMountainDensityDropdownChanged);

			LobbyAICountDropdown->OnSelectionChanged.Clear();
			LobbyAICountDropdown->OnSelectionChanged.AddDynamic(this, &ULobbySettingsUI::OnLobbyAICountDropdownChanged);

			LobbyDifficultyDropdown->OnSelectionChanged.Clear();
			LobbyDifficultyDropdown->OnSelectionChanged.AddDynamic(this, &ULobbySettingsUI::OnLobbyDifficultyDropdownChanged);
		}

		
		// Set Dropdowns
		{
			LobbyMapSeedInputBox->SetText(FText::FromString(serverMapSettings.mapSeed));


			// Small map for editor play for speed
			//serverMapSettings.mapSizeEnumInt = static_cast<int32>(MapSizeEnum::Medium);
			LobbyMapSizeDropdown->ClearOptions();
			for (FText name : MapSizeNames) {
				LobbyMapSizeDropdown->AddOption(name.ToString());
			}
			LobbyMapSizeDropdown->SetSelectedIndex(serverMapSettings.mapSizeEnumInt);

			
			auto setupDropdown = [&](UComboBoxString* LobbyDropdown, const std::vector<FText>& enumNames)
			{
				LobbyDropdown->ClearOptions();
				for (FText name : enumNames) {
					LobbyDropdown->AddOption(name.ToString());
				}
			};

			// Sea level
			//serverMapSettings.mapSeaLevel = MapSeaLevelEnum::Medium;
			setupDropdown(LobbySeaLevelDropdown, MapSettingsLevelNames);
			LobbySeaLevelDropdown->SetSelectedIndex(static_cast<int>(serverMapSettings.mapSeaLevel));

			// Moisture
			//serverMapSettings.mapMoisture = MapMoistureEnum::Medium;
			setupDropdown(LobbyMoistureDropdown, MapMoistureNames);
			LobbyMoistureDropdown->SetSelectedIndex(static_cast<int>(serverMapSettings.mapMoisture));

			// Temperature
			//serverMapSettings.mapTemperature = MapTemperatureEnum::Medium;
			setupDropdown(LobbyTemperatureDropdown, MapSettingsLevelNames);
			LobbyTemperatureDropdown->SetSelectedIndex(static_cast<int>(serverMapSettings.mapTemperature));

			// Mountain Density
			//serverMapSettings.mapMountainDensity = MapMountainDensityEnum::Medium;
			setupDropdown(LobbyMountainDensityDropdown, MapSettingsLevelNames);
			LobbyMountainDensityDropdown->SetSelectedIndex(static_cast<int>(serverMapSettings.mapMountainDensity));

			// AI Count
			RefreshAICountDropdown(serverMapSettings.aiCount);

			// Difficulty Level
			setupDropdown(LobbyDifficultyDropdown, DifficultyLevelNames);
			LobbyDifficultyDropdown->SetSelectedIndex(static_cast<int>(serverMapSettings.difficultyLevel));
		}


		if (isServer()) {
			PUN_DEBUG2("LobbyUI Open: SetPlayerCount 6");

			// Set player count settings
			serverMapSettings.playerCount = 6;
			gameInstance()->SetPlayerCount(6);
		}
	}


	void Tick(bool isLoading)
	{
		// Password Editable only in Settings before Lobby
		if (gameInstance()->isMultiplayer()) {
			LobbyPasswordRowBox->SetVisibility(ESlateVisibility::Visible);
			if (_isPreLobby) {
				LobbyPasswordInputBox->SetVisibility(ESlateVisibility::Visible);
				LobbyPasswordText->SetVisibility(ESlateVisibility::Collapsed);
			} else {
				LobbyPasswordInputBox->SetVisibility(ESlateVisibility::Collapsed);
				LobbyPasswordText->SetVisibility(ESlateVisibility::Visible);
				
				if (gameInstance()->lobbyPassword == "") {
					LobbyPasswordRowBox->SetVisibility(ESlateVisibility::Collapsed);
				} else {
					LobbyPasswordText->SetText(FText::FromString(gameInstance()->lobbyPassword));
				}
			}
		} else {
			LobbyPasswordRowBox->SetVisibility(ESlateVisibility::Collapsed);
		}

		/*
		 * Determine if display should be Dropdown or text
		 */
		auto setServerVsClientUI = [&](UWidget* serverWidget, UTextBlock* clientWidget, FString clientString)
		{
			// Not loading and is server, show serverWidget to allow settings change
			if (!isLoading && isServer())
			{
				serverWidget->SetVisibility(ESlateVisibility::Visible);
				clientWidget->SetVisibility(ESlateVisibility::Collapsed);
			}
			else
			{
				serverWidget->SetVisibility(ESlateVisibility::Collapsed);
				clientWidget->SetVisibility(ESlateVisibility::Visible);

				if (clientString != clientWidget->GetText().ToString()) {
					PUN_DEBUG2("LobbyClientText SetText %s", *clientString);
					clientWidget->SetText(FText::FromString(clientString));
				}
			}
		};

		FMapSettings mapSettings = gameInstance()->GetMapSettings();
		setServerVsClientUI(LobbyMapSeedInputBox, LobbyMapSeedText, mapSettings.mapSeed);
		setServerVsClientUI(LobbyMapSizeDropdown, LobbyMapSizeText, MapSizeNames[mapSettings.mapSizeEnumInt].ToString());
		setServerVsClientUI(LobbySeaLevelDropdown, LobbySeaLevelText, MapSettingsLevelNames[static_cast<int>(mapSettings.mapSeaLevel)].ToString());
		setServerVsClientUI(LobbyMoistureDropdown, LobbyMoistureText, MapMoistureNames[static_cast<int>(mapSettings.mapMoisture)].ToString());
		setServerVsClientUI(LobbyTemperatureDropdown, LobbyTemperatureText, MapSettingsLevelNames[static_cast<int>(mapSettings.mapTemperature)].ToString());
		setServerVsClientUI(LobbyMountainDensityDropdown, LobbyMountainDensityText, MapSettingsLevelNames[static_cast<int>(mapSettings.mapMountainDensity)].ToString());


		setServerVsClientUI(LobbyAICountDropdown, LobbyAICountText, FString::FromInt(mapSettings.aiCount));
		setServerVsClientUI(LobbyDifficultyDropdown, LobbyDifficultyText, DifficultyLevelNames[static_cast<int>(mapSettings.difficultyLevel)].ToString());

#define LOCTEXT_NAMESPACE "LobbySettingsUI"
		TArray<FText> args;
		ADDTEXT_LOCTEXT("DifficultyLevel_Tip", "Difficulty Level:\n");
		for (int32 i = 0; i < DifficultyLevelNames.size(); i++) {
			ADDTEXT_(INVTEXT(" - {0} (+{1}% {2})\n"), DifficultyLevelNames[i], TEXT_NUM(DifficultyConsumptionAdjustment[i]), LOCTEXT("consumption", "consumption"));
		}
		FText tipText = JOINTEXT(args);
		AddToolTip(LobbyDifficultyDropdown, tipText);
		AddToolTip(LobbyDifficultyText, tipText);
#undef LOCTEXT_NAMESPACE
	}

	
	void RefreshAICountDropdown(int32 preferredSelectedIndex)
	{
		LobbyAICountDropdown->ClearOptions();

		int maxAICount = GetMaxAICount(serverMapSettings.mapSizeEnum());

		for (int32 i = 0; i <= maxAICount; i++) {
			LobbyAICountDropdown->AddOption(FString::FromInt(i));
		}

		if (preferredSelectedIndex > maxAICount) {
			preferredSelectedIndex = maxAICount;
		}
		LobbyAICountDropdown->SetSelectedIndex(preferredSelectedIndex);
	}



public:
	UFUNCTION() void OnLobbyPasswordInputBoxTextCommitted(const FText& text, ETextCommit::Type CommitMethod) {
		gameInstance()->lobbyPassword = text.ToString();
		PUN_LOG("WTFFF %s", *gameInstance()->lobbyPassword);
	}

	UFUNCTION() void OnLobbyMapSeedInputBoxTextCommitted(const FText& text, ETextCommit::Type CommitMethod);
	UFUNCTION() void OnClickLobbyMapSeedRandomizeButton();
	
	UFUNCTION() void OnLobbyMapSizeDropdownChanged(FString sItem, ESelectInfo::Type seltype);
	UFUNCTION() void OnLobbySeaLevelDropdownChanged(FString sItem, ESelectInfo::Type seltype);
	UFUNCTION() void OnLobbyMoistureDropdownChanged(FString sItem, ESelectInfo::Type seltype);
	UFUNCTION() void OnLobbyTemperatureDropdownChanged(FString sItem, ESelectInfo::Type seltype);
	UFUNCTION() void OnLobbyMountainDensityDropdownChanged(FString sItem, ESelectInfo::Type seltype);
	UFUNCTION() void OnLobbyAICountDropdownChanged(FString sItem, ESelectInfo::Type seltype);
	UFUNCTION() void OnLobbyDifficultyDropdownChanged(FString sItem, ESelectInfo::Type seltype);

private:
	bool _isPreLobby = false;
};
