// Pun Dumnernchanvanit's


#include "LoadSaveUI.h"
#include "EscMenuUI.h"
#include "PunCity/PunGameInstance.h"
#include "PunMainMenuHUD.h"

#define LOCTEXT_NAMESPACE "LoadSaveUI"

void ULoadSaveUI::PunInit(UPunWidget* parent)
{
	_callbackParent = parent;
	BackButton->OnClicked.AddDynamic(this, &ULoadSaveUI::OnClickBackButton);
	LoadGameButton->OnClicked.AddDynamic(this, &ULoadSaveUI::OnClickSaveLoadGameButton);
	DeleteGameButton->OnClicked.AddDynamic(this, &ULoadSaveUI::OnClickDeleteGameButton);

	SelectedSavePlayerNameEditable->OnTextChanged.AddDynamic(this, &ULoadSaveUI::OnSaveNameChanged);
	SelectedSavePlayerNameEditable->OnTextCommitted.AddDynamic(this, &ULoadSaveUI::OnSaveNameCommited);

	SaveSelectionList->ClearChildren();

	SavingBlurText->SetVisibility(ESlateVisibility::Collapsed);

	
	ConfirmBlur->SetVisibility(ESlateVisibility::Collapsed);
	ConfirmUI->SetVisibility(ESlateVisibility::Collapsed);
	ConfirmUI->ConfirmYesButton->CoreButton->OnClicked.AddDynamic(this, &ULoadSaveUI::OnClickConfirmDeleteGameButton);
	ConfirmUI->ConfirmNoButton->CoreButton->OnClicked.AddDynamic(this, &ULoadSaveUI::OnClickCancelDeleteGameButton);
}

void ULoadSaveUI::OpenSaveUI()
{
	_isSavingGame = true;
	SetVisibility(ESlateVisibility::Visible);
	LoadSaveOverlay->SetVisibility(ESlateVisibility::Visible);
	SetText(LoadGameTitleText, LOCTEXT("Save Game", "Save Game"));
	SetText(LoadGameButtonText, LOCTEXT("Save Game", "Save Game"));

	RefreshSaveSelectionList(SaveActiveIndex_Unselected);
	SwapColumn(true);
}

void ULoadSaveUI::OpenLoadUI()
{
	_isSavingGame = false;
	SetVisibility(ESlateVisibility::Visible);
	LoadSaveOverlay->SetVisibility(ESlateVisibility::Visible);
	SetText(LoadGameTitleText, gameInstance()->isMultiplayer() ? LOCTEXT("Load Multiplayer Game", "Load Multiplayer Game") : LOCTEXT("Load Game", "Load Game"));
	SetText(LoadGameButtonText, LOCTEXT("Load Game", "Load Game"));

	RefreshSaveSelectionList(SaveActiveIndex_SelectFirstAvailable);
	SwapColumn(false);
}

void ULoadSaveUI::OnClickBackButton()
{
	Spawn2DSound("UI", "UIWindowClose");
	_callbackParent->CallBack2(_callbackParent, CallbackEnum::CloseLoadSaveUI);
}
void ULoadSaveUI::OnClickSaveLoadGameButton()
{
	// Load game:
	//  Server travel
	//  Load information into simulation / Camera position

	if (_isSavingGame) {
		FString nameString = SelectedSavePlayerNameEditable->Text.ToString();
		bool isSaveButtonEnabled = SelectedSavePlayerNameEditable->Text.ToString().Len() > 0;
		if (isSaveButtonEnabled) 
		{
			if (saveSystem().HasExistingSave(SelectedSavePlayerNameEditable->Text.ToString())) {
				_callbackParent->CallBack2(_callbackParent, CallbackEnum::SaveGameOverrideConfirm);
			} else {
				SaveGameDelayed();
			}
		}
	}
	else  {
		if (activeIndex != -1) {
			LoadGameDelayed();
		}
	}

	Spawn2DSound("UI", "ButtonClick");
}

void ULoadSaveUI::OnClickDeleteGameButton()
{
	if (activeIndex != -1)
	{
		ConfirmUI->SetVisibility(ESlateVisibility::Visible);
		ConfirmBlur->SetVisibility(ESlateVisibility::Visible);

		const TArray<GameSaveInfo>& saveList = saveSystem().saveList();
		PUN_CHECK(activeIndex < saveList.Num())

		SetText(ConfirmUI->ConfirmText, FText::Format(
			LOCTEXT("AskDeleteSaveGame", "<Subheader>Do you want to delete this saved game?</>\n\n{0}"),
			FText::FromString(saveList[activeIndex].name)
		));

		Spawn2DSound("UI", "UIWindowOpen");
	}
	else {
		Spawn2DSound("UI", "ButtonClickInvalid");
	}
}

void ULoadSaveUI::OnClickConfirmDeleteGameButton()
{
	if (activeIndex != -1)
	{
		const TArray<GameSaveInfo>& saveList = saveSystem().saveList();
		int32 oldSaveListCount = saveList.Num();
		PUN_CHECK(activeIndex < oldSaveListCount)
		
		saveSystem().DeleteSave(saveList[activeIndex].name);

		RefreshSaveSelectionList(SaveActiveIndex_SelectFirstAvailable);
	}

	Spawn2DSound("UI", "ButtonClick");
	ConfirmUI->SetVisibility(ESlateVisibility::Collapsed);
	ConfirmBlur->SetVisibility(ESlateVisibility::Collapsed);
}

void ULoadSaveUI::OnSaveNameChanged(const FText& text)
{
	// Disable save button if there is no name...
	bool isSaveButtonEnabled = SelectedSavePlayerNameEditable->Text.ToString().Len() > 0;
	LoadGameButton->SetIsEnabled(isSaveButtonEnabled);
	//SetButtonEnabled(LoadGameButton, isSaveButtonEnabled ? ButtonStateEnum::Enabled : ButtonStateEnum::Disabled);
}

void ULoadSaveUI::OnSaveNameCommited(const FText& text, ETextCommit::Type CommitMethod)
{
	// if text is no longer the same as activeIndex, release the highlight
	const TArray<GameSaveInfo>& saveList = saveSystem().saveList();
	FString newName = text.ToString();
	if (activeIndex != -1 && saveList[activeIndex].name != newName) {
		TArray<UWidget*> children = SaveSelectionList->GetAllChildren();
		PUN_CHECK(activeIndex < children.Num());
		CastChecked<UPunSelectButton>(children[activeIndex])->SetHighlight(false);
	}
}


void ULoadSaveUI::LoadGame()
{
	SCOPE_TIMER("LoadGame");

	auto gameInstance = Cast<UPunGameInstance>(GetGameInstance());

	const TArray<GameSaveInfo>& saveList = saveSystem().saveList();
	auto saveInfo = saveList[activeIndex];

	gameInstance->SetSavedGameToLoad(saveList[activeIndex]);

	// Set new mapSettings
	gameInstance->SetMapSettings(saveInfo.mapSettings);

	// If in-game
	// Transition is starting, Disable any ticking so it doesn't tick after gameInstance data was cleared
	if (gameInstance->IsInGame(this)) {
		networkInterface()->SetTickDisabled(true);
	}

	// Reset game instance
	gameInstance->ResetPlayerCount();

	gameInstance->isSinglePlayer = saveInfo.mapSettings.isSinglePlayer;

	// Loading a multiplayer game, create a lobby
	if (gameInstance->isMultiplayer())
	{
		gameInstance->isOpeningLoadMutiplayerPreLobby = true;
		
		//gameInstance->LoadMultiplayerGame();
		Spawn2DSound("UI", "UIWindowOpen");
	}
	else {
		// Load the game up right away
		GetWorld()->ServerTravel("/Game/Maps/GameMap");
	}
}

void ULoadSaveUI::SaveGameDelayed(bool isAutoSaving)
{
	_LOG(PunSaveLoad, "SaveConfirm");

	SetVisibility(ESlateVisibility::Visible);
	LoadSaveOverlay->SetVisibility(ESlateVisibility::Collapsed);

	SavingBlurText->SetVisibility(ESlateVisibility::Visible);
	SetText(SavingBlurText,
		isAutoSaving ? LOCTEXT("Autosaving...", "Autosaving...") : LOCTEXT("Saving...", "Saving...")
	);

	_callbackParent->CallBack2(this, CallbackEnum::OpenBlur);

	_isSavingGame = true;
	_delayedActionCountDown = 10;
	_isAutosaving = isAutoSaving;
}
void ULoadSaveUI::LoadGameDelayed()
{
	_LOG(PunSaveLoad, "Load ServerTravel");

	LoadSaveOverlay->SetVisibility(ESlateVisibility::Collapsed);
	_callbackParent->CallBack2(this, CallbackEnum::OpenBlur);

	if (gameInstance()->isMultiplayer()) {
		SavingBlurText->SetVisibility(ESlateVisibility::Collapsed);
	}
	else {
		SavingBlurText->SetVisibility(ESlateVisibility::Visible);
		SetText(SavingBlurText, LOCTEXT("Loading...", "Loading..."));
	}

	_isSavingGame = false;
	_delayedActionCountDown = 10;
	_isAutosaving = false;
}

#undef LOCTEXT_NAMESPACE