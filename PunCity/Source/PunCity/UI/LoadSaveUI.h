// Pun Dumnernchanvanit's

#pragma once

#include "PunCity/UI/PunWidget.h"
#include "PunSelectButton.h"
#include "PunCity/GameSaveSystem.h"
#include <iomanip>
#include "Components/BackgroundBlur.h"
#include "ConfirmUI.h"


#include "LoadSaveUI.generated.h"

/**
 * 
 */
UCLASS()
class PROTOTYPECITY_API ULoadSaveUI : public UPunWidget
{
	GENERATED_BODY()
public:
	void PunInit(UPunWidget* parent);

	UPROPERTY(meta = (BindWidget)) UOverlay* LoadSaveOverlay;
	
	UPROPERTY(meta = (BindWidget)) URichTextBlock* LoadGameTitleText;
	UPROPERTY(meta = (BindWidget)) UButton* BackButton;

	UPROPERTY(meta = (BindWidget)) UButton* LoadGameButton;
	UPROPERTY(meta = (BindWidget)) UButton* DeleteGameButton;

	UPROPERTY(meta = (BindWidget)) UVerticalBox* InfoColumn;
	UPROPERTY(meta = (BindWidget)) UVerticalBox* SaveGamesColumn;

	UPROPERTY(meta = (BindWidget)) UEditableTextBox* SelectedSavePlayerNameEditable;
	UPROPERTY(meta = (BindWidget)) UTextBlock* SelectedSavePlayerName;
	UPROPERTY(meta = (BindWidget)) UTextBlock* SelectedSaveDate;
	UPROPERTY(meta = (BindWidget)) UTextBlock* SelectedSaveTime;
	UPROPERTY(meta = (BindWidget)) UTextBlock* SelectedSaveMapSeed;
	UPROPERTY(meta = (BindWidget)) UTextBlock* SelectedSaveGameTime;
	UPROPERTY(meta = (BindWidget)) UTextBlock* SelectedSaveGameYear;
	UPROPERTY(meta = (BindWidget)) UTextBlock* SelectedSavePopulation;

	UPROPERTY(meta = (BindWidget)) UScrollBox* SaveSelectionList;
	UPROPERTY(meta = (BindWidget)) UTextBlock* LoadGameButtonText;

	UPROPERTY(meta = (BindWidget)) UTextBlock* NoSavedGameText;
	UPROPERTY(meta = (BindWidget)) UTextBlock* NoSavedGameSelectedText;

	UPROPERTY(meta = (BindWidget)) UTextBlock* SavingBlurText;

	UPROPERTY(meta = (BindWidget)) UOverlay* ConfirmBlur;
	UPROPERTY(meta = (BindWidget)) UConfirmUI* ConfirmUI;
	
	void OpenSaveUI()
	{
		_isSavingGame = true;
		SetVisibility(ESlateVisibility::Visible);
		LoadSaveOverlay->SetVisibility(ESlateVisibility::Visible);
		SetText(LoadGameTitleText, "SAVE GAME");
		SetText(LoadGameButtonText, "Save Game");
		
		RefreshSaveSelectionList(SaveActiveIndex_Unselected);
		SwapColumn(true);
	}

	void OpenLoadUI()
	{
		_isSavingGame = false;
		SetVisibility(ESlateVisibility::Visible);
		LoadSaveOverlay->SetVisibility(ESlateVisibility::Visible);
		SetText(LoadGameTitleText, gameInstance()->isMultiplayer() ? "LOAD MULTIPLAYER GAME" : "LOAD GAME");
		SetText(LoadGameButtonText, "Load Game");

		RefreshSaveSelectionList(SaveActiveIndex_SelectFirstAvailable);
		SwapColumn(false);
	}

	void Tick()
	{
		if (_delayedActionCountDown != -1) {	
			_delayedActionCountDown--;
			
			_LOG(PunSaveLoad, "_delayedActionCountDown:%d", _delayedActionCountDown);
			
			if (_delayedActionCountDown == 0) {
				_delayedActionCountDown = -1;
				
				if (_isSavingGame) {
					SaveGame();
				} else {
					LoadGame();
				}
			}
		}
	}

	int32 SaveActiveIndex_Unselected = -1;
	int32 SaveActiveIndex_SelectFirstAvailable = -2;
	
	void RefreshSaveSelectionList(int32 activeIndexIn)
	{
		_lastOpened = UGameplayStatics::GetTimeSeconds(this);
		
		SaveSelectionList->ClearChildren();

		saveSystem().RefreshSaveList();
		TArray<GameSaveInfo> saveList = saveSystem().saveList();


		bool isSinglePlayer = gameInstance()->isSinglePlayer;

		// Remove any wrong mode save from the list
		// Add buttons for those with correct mode
		for (int32 i = 0; i < saveList.Num(); i++)
		{
			UPunSelectButton* selectButton = AddWidget<UPunSelectButton>(UIEnum::SaveSelection);
			SaveSelectionList->AddChild(selectButton);
			selectButton->PunInit(this);

			std::stringstream ss;
			ss << ToStdString(saveList[i].name);
			//ss << " " << saveList[i].dateTime.GetMonth() << "/" << saveList[i].dateTime.GetDay();
			//ss << std::setw(2) << std::setfill('0') << ", " << saveList[i].dateTime.GetHour() << ":" << saveList[i].dateTime.GetMinute();
			selectButton->Set(ss.str(), this, CallbackEnum::SelectSaveGame, i);

			// Show the saves in the correct category (single vs. multiplayer)
			bool showSave = (saveList[i].mapSettings.isSinglePlayer == isSinglePlayer) && saveList[i].version == SAVE_VERSION;
			selectButton->SetVisibility(showSave ? ESlateVisibility::Visible : ESlateVisibility::Collapsed);

			if (showSave && activeIndexIn == SaveActiveIndex_SelectFirstAvailable) {
				activeIndexIn = i;
			}
		}

		// Wanted to select the first available, but there is nothing here, set to unselected
		if (activeIndexIn == SaveActiveIndex_SelectFirstAvailable) {
			activeIndex = SaveActiveIndex_Unselected;
		} else {
			activeIndex = activeIndexIn;
		}

		//if (activeIndexIn != SaveActiveIndex_Unselected) {
		//	activeIndex = activeIndexIn < saveList.Num() ? activeIndexIn : SaveActiveIndex_Unselected;
		//} else {
		//	activeIndex = SaveActiveIndex_Unselected;
		//}
		
		// Highlight the active widget
		TArray<UWidget*> children = SaveSelectionList->GetAllChildren();
		PUN_CHECK(activeIndex < children.Num());
		for (int32 i = 0; i < children.Num(); i++) {
			auto punButton = CastChecked<UPunSelectButton>(children[i]);
			punButton->SetHighlight(i == activeIndex);
		}

		ESlateVisibility visibility = _isSavingGame || activeIndex != -1 ? ESlateVisibility::Visible : ESlateVisibility::Collapsed;

		if (_isSavingGame) {
			SelectedSavePlayerNameEditable->SetVisibility(ESlateVisibility::Visible);
			SelectedSavePlayerName->SetVisibility(ESlateVisibility::Collapsed);
		} else {
			SelectedSavePlayerNameEditable->SetVisibility(ESlateVisibility::Collapsed);
			SelectedSavePlayerName->SetVisibility(visibility);
		}
		
		SelectedSaveDate->GetParent()->SetVisibility(visibility);
		SelectedSaveTime->GetParent()->SetVisibility(visibility);
		SelectedSaveMapSeed->GetParent()->SetVisibility(visibility);
		SelectedSaveGameTime->GetParent()->SetVisibility(visibility);
		SelectedSaveGameYear->GetParent()->SetVisibility(visibility);
		SelectedSavePopulation->GetParent()->SetVisibility(visibility);

		GameSaveInfo saveInfo;
		if (_isSavingGame) {
			// Show info for current save
			saveInfo.name = TrimStringF_Dots( simulation().playerNameF(playerId()), 10);
			saveInfo.dateTime = FDateTime::Now();
			saveInfo.gameTicks = Time::Ticks();
			saveInfo.population = simulation().population(playerId());
			saveInfo.mapSettings = dataSource()->GetMapSettings();

			if (activeIndex == SaveActiveIndex_Unselected) {
				SelectedSavePlayerNameEditable->SetText(FText::FromString(saveInfo.DefaultSaveName()));
			}
		}
		else {
			// Show save selection
			if (activeIndex != SaveActiveIndex_Unselected) {
				saveInfo = saveList[activeIndex];
			}
		}

		if (visibility == ESlateVisibility::Visible)
		{
			FDateTime dateTime = saveInfo.dateTime;
			FString date = FString::Printf(TEXT("%04d/%02d/%02d"), dateTime.GetYear(), dateTime.GetMonth(), dateTime.GetDay());
			FString time = FString::Printf(TEXT("%02d:%02d"), dateTime.GetHour(), dateTime.GetMinute());

			int32 gameTicks = saveInfo.gameTicks;
			FString gameSeason = FString::Printf(TEXT("%s %s"), ToTChar(Time::SeasonPrefix(gameTicks)), ToTChar(Time::SeasonName(Time::Seasons(gameTicks))));

			SelectedSavePlayerNameEditable->SetText(FText::FromString(saveInfo.DefaultSaveName()));
			SelectedSavePlayerName->SetText(FText::FromString(saveInfo.name));
			SelectedSaveDate->SetText(FText::FromString(date));
			SelectedSaveTime->SetText(FText::FromString(time));
			SelectedSaveMapSeed->SetText(FText::FromString(saveInfo.mapSettings.mapSeed));
			SelectedSaveGameTime->SetText(FText::FromString(gameSeason));
			SelectedSaveGameYear->SetText(FText::FromString(FString::FromInt(Time::Years(gameTicks))));
			SelectedSavePopulation->SetText(FText::FromString(FString::FromInt(saveInfo.population)));
		}

		DeleteGameButton->SetVisibility(activeIndex != SaveActiveIndex_Unselected ? ESlateVisibility::Visible : ESlateVisibility::Collapsed);

		if (_isSavingGame) {
			bool isSaveButtonEnabled = SelectedSavePlayerNameEditable->Text.ToString().Len() > 0;
			SetButtonEnabled(LoadGameButton, isSaveButtonEnabled ? ButtonStateEnum::Enabled : ButtonStateEnum::Disabled);
		} else {
			SetButtonEnabled(LoadGameButton, activeIndex != SaveActiveIndex_Unselected ? ButtonStateEnum::Enabled : ButtonStateEnum::Disabled);
		}

		NoSavedGameText->SetVisibility(saveList.Num() > 0 ? ESlateVisibility::Collapsed : ESlateVisibility::SelfHitTestInvisible);
		NoSavedGameSelectedText->SetVisibility(_isSavingGame || activeIndex != SaveActiveIndex_Unselected ? ESlateVisibility::Collapsed : ESlateVisibility::SelfHitTestInvisible);
	}

	void CallBack1(UPunWidget* punWidgetCaller, CallbackEnum callbackEnum) override
	{
		if (_isSavingGame) {
			// Saving game, change editable text to the name we are going to override
			int32 saveIndex = punWidgetCaller->callbackVar1;
			const TArray<GameSaveInfo>& saveList = saveSystem().saveList();
			FString saveName = saveList[saveIndex].name;
			
			RefreshSaveSelectionList(punWidgetCaller->callbackVar1); // Highlight the save...
			SelectedSavePlayerNameEditable->SetText(FText::FromString(saveName));
		} else {
			// Loading game, just refresh to new activeIndex
			RefreshSaveSelectionList(punWidgetCaller->callbackVar1);
		}
	}

	bool isSavingGame() { return _isSavingGame; }
	bool isAutosaving() { return _isAutosaving; }

	void SaveGameDelayed(bool isAutoSaving = false)
	{
		_LOG(PunSaveLoad, "SaveConfirm");

		SetVisibility(ESlateVisibility::Visible);
		LoadSaveOverlay->SetVisibility(ESlateVisibility::Collapsed);

		SavingBlurText->SetVisibility(ESlateVisibility::Visible);
		SetText(SavingBlurText, isAutoSaving ? "Autosaving..." : "Saving...");
		
		_callbackParent->CallBack2(this, CallbackEnum::OpenBlur);

		_isSavingGame = true;
		_delayedActionCountDown = 10;
		_isAutosaving = isAutoSaving;
	}
	void LoadGameDelayed()
	{
		_LOG(PunSaveLoad, "Load ServerTravel");
		
		LoadSaveOverlay->SetVisibility(ESlateVisibility::Collapsed);
		_callbackParent->CallBack2(this, CallbackEnum::OpenBlur);

		if (gameInstance()->isMultiplayer()) {
			SavingBlurText->SetVisibility(ESlateVisibility::Collapsed);
		}
		else {
			SavingBlurText->SetVisibility(ESlateVisibility::Visible);
			SetText(SavingBlurText, "Loading...");
		}

		_isSavingGame = false;
		_delayedActionCountDown = 10;
		_isAutosaving = false;
	}

	bool KeyPressed_Escape()
	{
		// Close ExitConfirm to EscMenu
		if (ConfirmUI->GetVisibility() != ESlateVisibility::Collapsed) {
			OnClickCancelDeleteGameButton();
			return true;
		}
		return false;
	}
	
private:
	UFUNCTION() void OnClickBackButton();
	UFUNCTION() void OnClickSaveLoadGameButton();
	
	UFUNCTION() void OnClickDeleteGameButton()
	{
		if (activeIndex != -1)
		{
			ConfirmUI->SetVisibility(ESlateVisibility::Visible);
			ConfirmBlur->SetVisibility(ESlateVisibility::Visible);

			const TArray<GameSaveInfo>& saveList = saveSystem().saveList();
			PUN_CHECK(activeIndex < saveList.Num())
			
			std::stringstream ss;
			ss << "<Subheader>Do you want to delete this saved game?</>\n\n";
			ss << ToStdString(saveList[activeIndex].name);
			SetText(ConfirmUI->ConfirmText, ss);

			Spawn2DSound("UI", "UIWindowOpen");
		}
		else {
			Spawn2DSound("UI", "ButtonClickInvalid");
		}
	}
	UFUNCTION() void OnClickConfirmDeleteGameButton();
	UFUNCTION() void OnClickCancelDeleteGameButton() {
		ConfirmUI->SetVisibility(ESlateVisibility::Collapsed);
		ConfirmBlur->SetVisibility(ESlateVisibility::Collapsed);
		Spawn2DSound("UI", "UIWindowClose");
	}

	UFUNCTION() void OnSaveNameChanged(const FText& text);
	UFUNCTION() void OnSaveNameCommited(const FText& text, ETextCommit::Type CommitMethod);

	void SwapColumn(bool infoFirst)
	{
		// Swap the InfoColumn to be first for save game
		UPanelWidget* columnParent = InfoColumn->GetParent();
		InfoColumn->RemoveFromParent();
		SaveGamesColumn->RemoveFromParent();

		if (infoFirst) {
			columnParent->AddChild(InfoColumn);
			columnParent->AddChild(SaveGamesColumn);
		} else {
			columnParent->AddChild(SaveGamesColumn);
			columnParent->AddChild(InfoColumn);
		}

		SaveSelectionList->InvalidateLayoutAndVolatility();
		SelectedSavePlayerName->InvalidateLayoutAndVolatility();
	}

	void SaveGame()
	{
		SCOPE_TIMER("SaveGame");
		
		FString nameString;

		if (_isAutosaving) 
		{
			// Remove the oldest autosaves if there are 3 or more autosaves
			saveSystem().RefreshSaveList();

			bool isSinglePlayer = gameInstance()->isSinglePlayer;

			TArray<GameSaveInfo> autoSaves;
			const TArray<GameSaveInfo>& saveList = saveSystem().saveList();
			for (int32 i = 0; i < saveList.Num(); i++) {
				if (saveList[i].IsAutosave() && saveList[i].mapSettings.isSinglePlayer == isSinglePlayer) {
					autoSaves.Add(saveList[i]);
				}
			}

			while (autoSaves.Num() >= 3)
			{
				GameSaveInfo oldestAutosave = autoSaves[0];
				for (int32 i = 1; i < autoSaves.Num(); i++) {
					if (autoSaves[i].dateTime < oldestAutosave.dateTime) {
						oldestAutosave = autoSaves[i];
					}
				}
				saveSystem().DeleteSave(oldestAutosave.folderPath);
				autoSaves.Remove(oldestAutosave);

				saveSystem().RefreshSaveList();
				RefreshSaveSelectionList(SaveActiveIndex_SelectFirstAvailable);
			}
			
			GameSaveInfo saveInfo;
			
			// Show info for current save
			saveInfo.name = TrimStringF_Dots(simulation().playerNameF(playerId()), 10);
			saveInfo.dateTime = FDateTime::Now();
			saveInfo.gameTicks = Time::Ticks();
			saveInfo.population = simulation().population(playerId());
			saveInfo.mapSettings = dataSource()->GetMapSettings();
			
			nameString = saveInfo.DefaultSaveName(true);
		}
		else {
			nameString = SelectedSavePlayerNameEditable->Text.ToString();
		}
		
		
		GameSaveInfo saveInfo = saveSystem().SaveDataToFile(nameString);

#if WITH_EDITOR
		if (!saveInfo.IsAutosave())
		{
			// Test that after loading the file up and saving again, the checksum is still the same
			saveSystem().Load(saveInfo.folderPath);
			GameSaveInfo saveInfoCheck = saveSystem().SaveDataToFile(nameString);
			PUN_CHECK(saveInfo.checksum == saveInfoCheck.checksum);
		}
#endif

		SavingBlurText->SetVisibility(ESlateVisibility::Collapsed);
		_callbackParent->CallBack2(this, CallbackEnum::CloseLoadSaveUI);
	}
	void LoadGame();
	//{
	//	SCOPE_TIMER("LoadGame");
	//	
	//	auto gameInstance = Cast<UPunGameInstance>(GetGameInstance());
	//	auto saveInfo = saveSystem().saveList()[activeIndex];
	//	gameInstance->SetSavedGameToLoad(saveSystem().saveList()[activeIndex]);

	//	// Set new mapSettings
	//	gameInstance->SetMapSettings(saveInfo.mapSettings);

	//	// If in-game
	//	// Transition is starting, Disable any ticking so it doesn't tick after gameInstance data was cleared
	//	if (gameInstance->IsInGame(this)) {
	//		networkInterface()->SetTickDisabled(true);
	//	}

	//	// Reset game instance
	//	gameInstance->ResetPlayerCount();
	//	
	//	gameInstance->isSinglePlayer = saveInfo.mapSettings.isSinglePlayer;

	//	// Loading a multiplayer game, create a lobby
	//	if (gameInstance->isMultiplayer()) 
	//	{
	//		GetPunHUD()->OpenPreLobbySettings();
	//		gameInstance->LoadMultiplayerGame();
	//		Spawn2DSound("UI", "UIWindowOpen");
	//	}
	//	else {
	//		// Load the game up right away
	//		GetWorld()->ServerTravel("/Game/Maps/GameMap");
	//	}
	//}

	GameSaveSystem& saveSystem() {
		auto gameInstance = CastChecked<UPunGameInstance>(GetGameInstance());
		return gameInstance->saveSystem();
	}

	float _lastOpened = 0.0f;
	void Spawn2DSound(std::string groupName, std::string soundName) {
		if (UGameplayStatics::GetTimeSeconds(this) - _lastOpened < 0.5f) {
			return;
		}
		if (dataSource()) {
			dataSource()->Spawn2DSound(groupName, soundName);
		}
		else {
			gameInstance()->Spawn2DSound(groupName, soundName);
		}
	}
	
private:
	
	UPROPERTY() UPunWidget* _callbackParent;

	bool _isSavingGame = false;
	int32 activeIndex = -1;

	int32 _delayedActionCountDown = -1;
	bool _isAutosaving = false;
};
