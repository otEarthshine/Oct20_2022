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

	// Settings Background
	UPROPERTY(meta = (BindWidget)) UImage* SettingsBackgroundImage;
	
	FMapSettings serverMapSettings; // This is only used for server

	void SetPreLobby(bool isPreLobby)
	{
		_isPreLobby = isPreLobby;
		LobbyPasswordRowBox->SetVisibility(_isPreLobby ? ESlateVisibility::Visible : ESlateVisibility::Collapsed);
	}
	
	bool isServer();

	class AMainMenuPlayerController* GetFirstController();

	void SendMapSettings();

	void Init(FMapSettings mapSettingsIn)
	{
		serverMapSettings = mapSettingsIn;
		
		// Only server can change settings
		if (isServer())
		{
			LobbyPasswordInputBox->OnTextCommitted.AddDynamic(this, &ULobbySettingsUI::OnLobbyPasswordInputBoxTextCommitted);

			LobbyMapSeedInputBox->OnTextCommitted.AddDynamic(this, &ULobbySettingsUI::OnLobbyMapSeedInputBoxTextCommitted);

			LobbyMapSizeDropdown->OnSelectionChanged.AddDynamic(this, &ULobbySettingsUI::OnLobbyMapSizeDropdownChanged);
			// "Small" If adding another map size, also change the options on UI editor
			// Note: Using UI Editor for this to prevent ::Direct selection issue

			LobbySeaLevelDropdown->OnSelectionChanged.AddDynamic(this, &ULobbySettingsUI::OnLobbySeaLevelDropdownChanged);
			LobbyMoistureDropdown->OnSelectionChanged.AddDynamic(this, &ULobbySettingsUI::OnLobbyMoistureDropdownChanged);
			LobbyTemperatureDropdown->OnSelectionChanged.AddDynamic(this, &ULobbySettingsUI::OnLobbyTemperatureDropdownChanged);
			LobbyMountainDensityDropdown->OnSelectionChanged.AddDynamic(this, &ULobbySettingsUI::OnLobbyMountainDensityDropdownChanged);

			LobbyAICountDropdown->OnSelectionChanged.AddDynamic(this, &ULobbySettingsUI::OnLobbyAICountDropdownChanged);
			RefreshAICountDropdown();
			LobbyAICountDropdown->SetSelectedIndex(LobbyAICountDropdown->GetOptionCount() - 1);

			LobbyDifficultyDropdown->OnSelectionChanged.AddDynamic(this, &ULobbySettingsUI::OnLobbyDifficultyDropdownChanged);
		}


		
		// Small map for editor play for speed
		//serverMapSettings.mapSizeEnumInt = static_cast<int32>(MapSizeEnum::Medium);
		LobbyMapSizeDropdown->ClearOptions();
		for (FString name : MapSizeNames) {
			LobbyMapSizeDropdown->AddOption(name);
		}
		LobbyMapSizeDropdown->SetSelectedIndex(serverMapSettings.mapSizeEnumInt);
		RefreshAICountDropdown();

		

		{
			auto setupDropdown = [&](UComboBoxString* LobbyDropdown, const std::vector<FString>& enumNames)
			{
				LobbyDropdown->ClearOptions();
				for (FString name : enumNames) {
					LobbyDropdown->AddOption(name);
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
		setServerVsClientUI(LobbyMapSizeDropdown, LobbyMapSizeText, MapSizeNames[mapSettings.mapSizeEnumInt]);
		setServerVsClientUI(LobbySeaLevelDropdown, LobbySeaLevelText, MapSettingsLevelNames[static_cast<int>(mapSettings.mapSeaLevel)]);
		setServerVsClientUI(LobbyMoistureDropdown, LobbyMoistureText, MapMoistureNames[static_cast<int>(mapSettings.mapMoisture)]);
		setServerVsClientUI(LobbyTemperatureDropdown, LobbyTemperatureText, MapSettingsLevelNames[static_cast<int>(mapSettings.mapTemperature)]);
		setServerVsClientUI(LobbyMountainDensityDropdown, LobbyMountainDensityText, MapSettingsLevelNames[static_cast<int>(mapSettings.mapMountainDensity)]);


		setServerVsClientUI(LobbyAICountDropdown, LobbyAICountText, FString::FromInt(mapSettings.aiCount));
		setServerVsClientUI(LobbyDifficultyDropdown, LobbyDifficultyText, DifficultyLevelNames[static_cast<int>(mapSettings.difficultyLevel)]);
	}

	
	void RefreshAICountDropdown()
	{
		int32 selectedIndex = LobbyAICountDropdown->GetSelectedIndex();
		if (selectedIndex == -1) {
			selectedIndex = LobbyAICountDropdown->GetOptionCount() - 1;
		}

		LobbyAICountDropdown->ClearOptions();

		int maxAICount = GameConstants::MaxAIs;
		switch (serverMapSettings.mapSizeEnum()) {
		case MapSizeEnum::Medium: maxAICount = 8; break;
		case MapSizeEnum::Small: maxAICount = 3; break;
		}

		for (int32 i = 0; i <= maxAICount; i++) {
			LobbyAICountDropdown->AddOption(FString::FromInt(i));
		}

		if (selectedIndex > maxAICount) {
			selectedIndex = maxAICount - 1;
		}
		LobbyAICountDropdown->SetSelectedIndex(selectedIndex);
	}



public:
	UFUNCTION() void OnLobbyPasswordInputBoxTextCommitted(const FText& text, ETextCommit::Type CommitMethod)
	{

	}

	UFUNCTION() void OnLobbyMapSeedInputBoxTextCommitted(const FText& text, ETextCommit::Type CommitMethod);
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
