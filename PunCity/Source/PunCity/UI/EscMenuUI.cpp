// Pun Dumnernchanvanit's


#include "EscMenuUI.h"

#define LOCTEXT_NAMESPACE "EscMenuUI"

void UEscMenuUI::InitBeforeGameManager()
{
	BackBlur->SetVisibility(ESlateVisibility::Collapsed);
	ConfirmBlur->SetVisibility(ESlateVisibility::Collapsed);
	ConfirmUI->SetVisibility(ESlateVisibility::Collapsed);

	GameSettingsUI->SetVisibility(ESlateVisibility::Collapsed);
	LoadSaveUI->SetVisibility(ESlateVisibility::Collapsed);

	FString mapName = GetWorld()->GetMapName();
	if (mapName.Right(8) == FString("MainMenu")) {
		LoadingScreen->SetVisibility(ESlateVisibility::Collapsed);

		GameSpeedOverlay->SetVisibility(ESlateVisibility::Collapsed);
		
		EscMenuToggler->SetVisibility(ESlateVisibility::Collapsed);
		
		TutorialToggler->SetVisibility(ESlateVisibility::Collapsed);
		//TutorialToggler->SetVisibility(ESlateVisibility::Visible);

		OverlayToggler->SetVisibility(ESlateVisibility::Collapsed);
		OverlaySettingsOverlay->SetVisibility(ESlateVisibility::Collapsed);
	}
}

void UEscMenuUI::PunInit()
{
	SetChildHUD(GameSettingsUI);
	GameSettingsUI->PunInit(this);
	
	SetChildHUD(TutorialUI);
	TutorialUI->PunInit();

	SetChildHUD(LoadSaveUI);
	LoadSaveUI->PunInit(this);
	
	EscMenuToggler->CoreButton->OnClicked.AddDynamic(this, &UEscMenuUI::ToggleEscMenu);
	EscMenuResumeButton->CoreButton->OnClicked.AddDynamic(this, &UEscMenuUI::OnClickEscMenuResumeButton);
	EscMenuSettingsButton->CoreButton->OnClicked.AddDynamic(this, &UEscMenuUI::OnClickEscMenuSettingsButton);
	EscMenuRestartGameButton->CoreButton->OnClicked.AddDynamic(this, &UEscMenuUI::OnClickEscMenuRestartGameButton);
	EscMenuExitToMainMenuButton->CoreButton->OnClicked.AddDynamic(this, &UEscMenuUI::OnClickEscMenuExitToMainMenuButton);
	EscMenuQuitButton->CoreButton->OnClicked.AddDynamic(this, &UEscMenuUI::OnClickEscMenuQuitButton);

	TutorialToggler->CoreButton->OnClicked.AddDynamic(this, &UEscMenuUI::ToggleTutorial);

	ConfirmYesButton->CoreButton->OnClicked.AddDynamic(this, &UEscMenuUI::OnClickConfirmYesButton);
	ConfirmNoButton->CoreButton->OnClicked.AddDynamic(this, &UEscMenuUI::OnClickConfirmNoButton);

	EscMenuSaveButton->CoreButton->OnClicked.AddDynamic(this, &UEscMenuUI::OnClickEscMenuSaveButton);
	EscMenuLoadButton->CoreButton->OnClicked.AddDynamic(this, &UEscMenuUI::OnClickEscMenuLoadButton);

	PauseButton->OnClicked.AddDynamic(this, &UEscMenuUI::OnClickPauseButton);

	// Play Button
	PlayButton1->OnClicked.AddDynamic(this, &UEscMenuUI::OnClickPlayButton1);
	PlayButton2->OnClicked.AddDynamic(this, &UEscMenuUI::OnClickPlayButton2);
	PlayButton3->OnClicked.AddDynamic(this, &UEscMenuUI::OnClickPlayButton3);
	PlayButton4->OnClicked.AddDynamic(this, &UEscMenuUI::OnClickPlayButton4);
	
	PlayButton1->OnHovered.AddDynamic(this, &UEscMenuUI::OnHoverEnterPlayButton1);
	PlayButton2->OnHovered.AddDynamic(this, &UEscMenuUI::OnHoverEnterPlayButton2);
	PlayButton3->OnHovered.AddDynamic(this, &UEscMenuUI::OnHoverEnterPlayButton3);
	PlayButton4->OnHovered.AddDynamic(this, &UEscMenuUI::OnHoverEnterPlayButton4);

	PlayButton1->OnUnhovered.AddDynamic(this, &UEscMenuUI::OnHoverExitPlayButton);
	PlayButton2->OnUnhovered.AddDynamic(this, &UEscMenuUI::OnHoverExitPlayButton);
	PlayButton3->OnUnhovered.AddDynamic(this, &UEscMenuUI::OnHoverExitPlayButton);
	PlayButton4->OnUnhovered.AddDynamic(this, &UEscMenuUI::OnHoverExitPlayButton);


	// Overlay toggler
	OverlaySettingsOverlay->SetVisibility(ESlateVisibility::Collapsed);
	OverlayToggler->CoreButton->OnClicked.AddDynamic(this, &UEscMenuUI::OnClickOverlayToggler);
	OverlayCheckBox_None->OnCheckStateChanged.AddDynamic(this, &UEscMenuUI::OnCheckOverlay_None);
	OverlayCheckBox_Appeal->OnCheckStateChanged.AddDynamic(this, &UEscMenuUI::OnCheckOverlay_Appeal);
	OverlayCheckBox_Fertility->OnCheckStateChanged.AddDynamic(this, &UEscMenuUI::OnCheckOverlay_Fertility);
	OverlayCheckBox_Animals->OnCheckStateChanged.AddDynamic(this, &UEscMenuUI::OnCheckOverlay_Animals);
	MapCheckBox_HideTrees->OnCheckStateChanged.AddDynamic(this, &UEscMenuUI::OnCheckMap_HideTrees);
	MapCheckBox_ProvinceOverlay->OnCheckStateChanged.AddDynamic(this, &UEscMenuUI::OnCheckMap_ProvinceOverlay);

	SetOverlayCheckBox(OverlayCheckBox_None);

	// Overlay Tooltip
	AddToolTip(OverlayOuterBox_None, LOCTEXT("OverlayCheckBox_None_Tip", "Check to turn off Overlay."));
	AddToolTip(OverlayOuterBox_Appeal, LOCTEXT("OverlayCheckBox_Appeal_Tip", "Houses built on high Appeal area give occupants more Happiness."));
	AddToolTip(OverlayOuterBox_Fertility, LOCTEXT("OverlayCheckBox_Fertility_Tip", "Farms built on high fertility area have higher productivity."));
	AddToolTip(OverlayOuterBox_Animals, LOCTEXT("OverlayCheckBox_Animals_Tip", "Highlight animals."));


	// others

	VictoryScoreScreenButton->OnClicked.AddDynamic(this, &UEscMenuUI::OnClickVictoryPopupScoreScreen);
	VictoryReturnToGame->CoreButton->OnClicked.AddDynamic(this, &UEscMenuUI::OnClickVictoryPopupReturnToGame);

	SetChildHUD(ScoreBreakdown);
	SetChildHUD(OtherPlayerScores);

	AddToolTip(OverlayToggler, LOCTEXT("OverlayToggler_Tip", "Open the Overlay Menu."));
	AddToolTip(EscMenuToggler, LOCTEXT("EscMenuToggler_Tip", "Open the Game Menu."));
	AddToolTip(TutorialToggler, LOCTEXT("TutorialToggler_Tip", "Open the tutorial."));

	GetAnimations();
}

void UEscMenuUI::OnClickEscMenuResumeButton()
{
	EscMenu->SetVisibility(ESlateVisibility::Collapsed);
	
	dataSource()->Spawn2DSound("UI", "UIWindowClose");
}
void UEscMenuUI::OnClickEscMenuSettingsButton()
{
	EscMenu->SetVisibility(ESlateVisibility::Collapsed);
	GameSettingsUI->SetVisibility(ESlateVisibility::Visible);
	GameSettingsUI->RefreshUI(true, true);

	dataSource()->Spawn2DSound("UI", "UIWindowOpen");
}
void UEscMenuUI::OnClickEscMenuRestartGameButton()
{
	ConfirmBlur->SetVisibility(ESlateVisibility::Visible);
	ConfirmUI->SetVisibility(ESlateVisibility::Visible);
	ConfirmText->SetText(LOCTEXT("AskRestartGame", "Are you sure you want to restart the game?"));
	_confirmEnum = RestartGame;

	dataSource()->Spawn2DSound("UI", "UIWindowOpen");
}
void UEscMenuUI::OnClickEscMenuExitToMainMenuButton()
{
	ConfirmBlur->SetVisibility(ESlateVisibility::Visible);
	ConfirmUI->SetVisibility(ESlateVisibility::Visible);
	ConfirmText->SetText(LOCTEXT("AskExitMainMenu", "Are you sure you want to exit to main menu?"));
	_confirmEnum = ExitToMainMenu;
	
	dataSource()->Spawn2DSound("UI", "UIWindowOpen");
}
void UEscMenuUI::OnClickEscMenuQuitButton()
{
	ConfirmBlur->SetVisibility(ESlateVisibility::Visible);
	ConfirmUI->SetVisibility(ESlateVisibility::Visible);
	ConfirmText->SetText(LOCTEXT("AskExitDesktop", "Are you sure you want to exit to desktop?"));
	_confirmEnum = ExitGame;

	dataSource()->Spawn2DSound("UI", "UIWindowOpen");
}

void UEscMenuUI::OnClickConfirmYesButton()
{
	dataSource()->Spawn2DSound("UI", "ButtonClick");

	if (_confirmEnum == RestartGame) {
		networkInterface()->GoToSinglePlayerLobby();
	}
	else if (_confirmEnum == ExitToMainMenu) {
		networkInterface()->GoToMainMenu();
	}
	else if (_confirmEnum == ExitGame) {
		UKismetSystemLibrary::QuitGame(GetWorld(), nullptr, EQuitPreference::Quit, false);
	}
	else if (_confirmEnum == OverrideSave) {
		ConfirmBlur->SetVisibility(ESlateVisibility::Collapsed);
		ConfirmUI->SetVisibility(ESlateVisibility::Collapsed);
		LoadSaveUI->SaveGameDelayed();
	}
}
void UEscMenuUI::OnClickConfirmNoButton()
{
	if (_confirmEnum == ExitGame||
		_confirmEnum == ExitToMainMenu ||
		_confirmEnum == RestartGame)
	{
		ConfirmBlur->SetVisibility(ESlateVisibility::Collapsed);
		ConfirmUI->SetVisibility(ESlateVisibility::Collapsed);
	}
	if (_confirmEnum == OverrideSave) {
		ConfirmBlur->SetVisibility(ESlateVisibility::Collapsed);
		ConfirmUI->SetVisibility(ESlateVisibility::Collapsed);
	}

	dataSource()->Spawn2DSound("UI", "ButtonClick");
}

void UEscMenuUI::CallBack2(UPunWidget* punWidgetCaller, CallbackEnum callbackEnum)
{
	PUN_LOG("CallBack2 CloseGameSettingsUI");
	if (callbackEnum == CallbackEnum::CloseGameSettingsUI) {
		GameSettingsUI->SetVisibility(ESlateVisibility::Collapsed);
		EscMenu->SetVisibility(ESlateVisibility::Visible);
	}
	if (callbackEnum == CallbackEnum::OpenBlur) {
		BackBlur->SetVisibility(ESlateVisibility::Visible);
	}
	if (callbackEnum == CallbackEnum::CloseLoadSaveUI) {
		LoadSaveUI->SetVisibility(ESlateVisibility::Collapsed);
		EscMenu->SetVisibility(LoadSaveUI->isAutosaving() ? ESlateVisibility::Collapsed : ESlateVisibility::Visible);
		BackBlur->SetVisibility(ESlateVisibility::Collapsed);
	}
	if (callbackEnum == CallbackEnum::SaveGameOverrideConfirm) {
		ConfirmBlur->SetVisibility(ESlateVisibility::Visible);
		ConfirmUI->SetVisibility(ESlateVisibility::Visible);
		ConfirmText->SetText(LOCTEXT("SaveGameOverrideConfirm", "Are you sure you want to override this saved game?"));
		_confirmEnum = OverrideSave;
	}
}

void UEscMenuUI::OnClickEscMenuSaveButton()
{
	BackBlur->SetVisibility(ESlateVisibility::Visible);
	LoadSaveUI->OpenSaveUI();

	dataSource()->Spawn2DSound("UI", "UIWindowOpen");
}
void UEscMenuUI::OnClickEscMenuLoadButton()
{
	BackBlur->SetVisibility(ESlateVisibility::Visible);
	LoadSaveUI->OpenLoadUI();

	dataSource()->Spawn2DSound("UI", "UIWindowOpen");
}

void UEscMenuUI::SetOverlayCheckBox(UCheckBox* activeCheckBox)
{
	TArray<UCheckBox*> checkBoxes{
		OverlayCheckBox_None,
		OverlayCheckBox_Fertility,
		OverlayCheckBox_Appeal,
		OverlayCheckBox_Animals,
	};
	for (auto checkBox : checkBoxes) {
		checkBox->SetCheckedState(checkBox == activeCheckBox ? ECheckBoxState::Checked : ECheckBoxState::Unchecked);
	}
}


#undef LOCTEXT_NAMESPACE