// Pun Dumnernchanvanit's


#include "LoadSaveUI.h"
#include "EscMenuUI.h"
#include "PunCity/PunGameInstance.h"
#include "PunMainMenuHUD.h"

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
		const TArray<GameSaveInfo>& saveList = saveSystem().saveList();
		int32 oldSaveListCount = saveList.Num();
		PUN_CHECK(activeIndex < oldSaveListCount)
		
		saveSystem().DeleteSave(saveList[activeIndex].folderPath);

		RefreshSaveSelectionList(SaveActiveIndex_SelectFirstAvailable);
	}

	Spawn2DSound("UI", "ButtonClick");
}

void ULoadSaveUI::OnSaveNameChanged(const FText& text)
{
	// Disable save button if there is no name...
	bool isSaveButtonEnabled = SelectedSavePlayerNameEditable->Text.ToString().Len() > 0;
	SetButtonEnabled(LoadGameButton, isSaveButtonEnabled ? ButtonStateEnum::Enabled : ButtonStateEnum::Disabled);
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