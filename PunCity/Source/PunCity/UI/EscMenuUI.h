// Pun Dumnernchanvanit's

#pragma once

#include "PunCity/UI/PunWidget.h"
#include "GameSettingsUI.h"

#include "LoadSaveUI.h"
#include "LoadingScreenUI.h"
#include "TutorialUI.h"
#include "ShaderPipelineCache.h"

#include "EscMenuUI.generated.h"

/**
 * 
 */
UCLASS()
class PROTOTYPECITY_API UEscMenuUI : public UPunWidget
{
	GENERATED_BODY()
public:
	void InitBeforeGameManager();
	void PunInit();

	UPROPERTY(meta = (BindWidget)) USpacer* EscMenuSpacer;
	UPROPERTY(meta = (BindWidget)) UButton* EscMenuToggler;
	UPROPERTY(meta = (BindWidget)) UOverlay* EscMenu;
	UPROPERTY(meta = (BindWidget)) UButton* EscMenuResumeButton;
	UPROPERTY(meta = (BindWidget)) UButton* EscMenuSettingsButton;

	UPROPERTY(meta = (BindWidget)) UOverlay*EscMenuRestartGameButtonOverlay;
	UPROPERTY(meta = (BindWidget)) UButton* EscMenuRestartGameButton;
	
	UPROPERTY(meta = (BindWidget)) UButton* EscMenuExitToMainMenuButton;
	UPROPERTY(meta = (BindWidget)) UButton* EscMenuQuitButton;

	UPROPERTY(meta = (BindWidget)) UHorizontalBox* TopLeftBoxWithSpacer;

	UPROPERTY(meta = (BindWidget)) UOverlay* GameSpeedOverlay;
	UPROPERTY(meta = (BindWidget)) UImage* PauseIcon;
	UPROPERTY(meta = (BindWidget)) UButton* PauseButton;
	UPROPERTY(meta = (BindWidget)) UImage* PlayIcon1;
	UPROPERTY(meta = (BindWidget)) UImage* PlayIcon2;
	UPROPERTY(meta = (BindWidget)) UImage* PlayIcon3;
	UPROPERTY(meta = (BindWidget)) UImage* PlayIcon4;
	UPROPERTY(meta = (BindWidget)) UButton* PlayButton1;
	UPROPERTY(meta = (BindWidget)) UButton* PlayButton2;
	UPROPERTY(meta = (BindWidget)) UButton* PlayButton3;
	UPROPERTY(meta = (BindWidget)) UButton* PlayButton4;

	UPROPERTY(meta = (BindWidget)) UButton* OverlayToggler;
	UPROPERTY(meta = (BindWidget)) UOverlay* OverlaySettingsOverlay;
	UPROPERTY(meta = (BindWidget)) class UCheckBox* OverlayCheckBox_None;
	UPROPERTY(meta = (BindWidget)) UCheckBox* OverlayCheckBox_Appeal;
	UPROPERTY(meta = (BindWidget)) UCheckBox* OverlayCheckBox_Fertility;
	UPROPERTY(meta = (BindWidget)) UCheckBox* OverlayCheckBox_Animals;
	UPROPERTY(meta = (BindWidget)) UCheckBox* MapCheckBox_HideTrees;
	UPROPERTY(meta = (BindWidget)) UCheckBox* MapCheckBox_ProvinceOverlay;

	UPROPERTY(meta = (BindWidget)) UHorizontalBox* OverlayOuterBox_None;
	UPROPERTY(meta = (BindWidget)) UHorizontalBox* OverlayOuterBox_Appeal;
	UPROPERTY(meta = (BindWidget)) UHorizontalBox* OverlayOuterBox_Fertility;
	UPROPERTY(meta = (BindWidget)) UHorizontalBox* OverlayOuterBox_Animals;
	
	UPROPERTY(meta = (BindWidget)) UGameSettingsUI* GameSettingsUI;

	UPROPERTY(meta = (BindWidget)) UOverlay* BackBlur;
	UPROPERTY(meta = (BindWidget)) UOverlay* ConfirmBlur;
	
	UPROPERTY(meta = (BindWidget)) UOverlay* ConfirmUI;
	UPROPERTY(meta = (BindWidget)) URichTextBlock* ConfirmText;
	UPROPERTY(meta = (BindWidget)) UButton* ConfirmYesButton;
	UPROPERTY(meta = (BindWidget)) UButton* ConfirmNoButton;

	UPROPERTY(meta = (BindWidget)) ULoadingScreenUI* LoadingScreen;

	//! Victory
	UPROPERTY(meta = (BindWidget)) UOverlay* VictoryPopup;
	UPROPERTY(meta = (BindWidget)) URichTextBlock* VictoryPopupText;
	UPROPERTY(meta = (BindWidget)) UButton* VictoryScoreScreenButton;
	UPROPERTY(meta = (BindWidget)) UButton* VictoryReturnToGame;
	bool alreadyShownVictory = false;

	//! Load/Save
	UPROPERTY(meta = (BindWidget)) UButton* EscMenuSaveButton;
	UPROPERTY(meta = (BindWidget)) UButton* EscMenuLoadButton;
	UPROPERTY(meta = (BindWidget)) UOverlay* EscMenuLoadButtonOverlay;
	UPROPERTY(meta = (BindWidget)) ULoadSaveUI* LoadSaveUI;

	//! Tutorials
	UPROPERTY(meta = (BindWidget)) UButton* TutorialToggler;
	UPROPERTY(meta = (BindWidget)) UTutorialUI* TutorialUI;
	float lastTutorialAttentionOpen = 0.0f;

	UPROPERTY(meta = (BindWidget)) UImage* TutorialAttentionFlash;
	UPROPERTY(meta = (BindWidget)) UOverlay* ScoreScreenDelayOverlay;

	void ShowTutorialUI(TutorialLinkEnum linkEnum)
	{
		if (linkEnum == TutorialLinkEnum::TutorialButton) {
			TutorialAttentionFlash->SetVisibility(ESlateVisibility::HitTestInvisible);
			TryPlayAnimation("TutorialAttention");
			lastTutorialAttentionOpen = UGameplayStatics::GetTimeSeconds(this);
		} else {
			TutorialAttentionFlash->SetVisibility(ESlateVisibility::Collapsed);
			TryStopAnimation("TutorialAttention");
			
			networkInterface()->ResetGameUI();
			TutorialUI->SetVisibility(ESlateVisibility::Visible);
			TutorialUI->ShowTutorialUI(linkEnum);
		}
		
		dataSource()->Spawn2DSound("UI", "UIWindowOpen");
	}


	/*
	 * 
	 */

	void Tick()
	{
#if !UI_ESCMENU
		return;
#endif
		if (!PunSettings::IsOn("UIEscMenu")) {
			return;
		}
		
		// move the EscMenuToggler to correct position
		EscMenuSpacer->SetVisibility(simulation().playerOwned(playerId()).hasChosenLocation() ? ESlateVisibility::HitTestInvisible : ESlateVisibility::Collapsed);

		//SetLoadingText(ToFString(simulation().loadingText()));

		LoadSaveUI->Tick();

		
		// Multiplayer game don't show LoadButton
		EscMenuLoadButtonOverlay->SetVisibility(gameInstance()->isMultiplayer() ? ESlateVisibility::Collapsed : ESlateVisibility::Visible);

		// Only display "Restart Game" for single player
		EscMenuRestartGameButtonOverlay->SetVisibility(gameInstance()->isMultiplayer() ? ESlateVisibility::Collapsed : ESlateVisibility::Visible);

		if (ShouldPauseGameFromUI() && networkInterface()->hostGameSpeed() != 0) // hostGameSpeed() must be checked all the time since players should be able too unpause.
		{
			_isPausingFromEscMenu = true;
			networkInterface()->Pause();
		}
		else if (!ShouldPauseGameFromUI() && _isPausingFromEscMenu) {
			_isPausingFromEscMenu = false;
			networkInterface()->Resume();
		}

		
		
		if (UGameplayStatics::GetTimeSeconds(this) >  lastShaderCachePrint + 1.0f &&
			FShaderPipelineCache::NumPrecompilesRemaining() > 0) 
		{
			lastShaderCachePrint = UGameplayStatics::GetTimeSeconds(this);
			PUN_DEBUG2("ShaderCache: active:%d remaining:%d", FShaderPipelineCache::NumPrecompilesActive(), FShaderPipelineCache::NumPrecompilesRemaining());
		}

		// Update the gameSpeedBar
		//  Host use hostGameSpeed() while clients use simulation.gameSpeed()
		int32 gameSpeedToDisplay = networkInterface()->IsHost() ? networkInterface()->hostGameSpeed() : simulation().gameSpeed();

		// Special case: This is needed because we are using hostGameSpeed() which does not reflect chosenTownhall state
		//   Pause while players are still choosing location.
		if (!simulation().AllPlayerHasTownhallAfterInitialTicks() ||
			ShouldPauseGameFromUI()) 
		{
			gameSpeedToDisplay = 0;
		}

		FLinearColor activeColor(1, 1, 1);
		FLinearColor inactiveColor(.1, .1, .1);
		FLinearColor hoverColor(.38, .38, 0.2);
		FLinearColor hoverOffColor(.15, .15, 0.1);

		FLinearColor pauseColor = inactiveColor;
		FLinearColor play1Color = inactiveColor;
		FLinearColor play2Color = inactiveColor;
		FLinearColor play3Color = inactiveColor;
		FLinearColor play4Color = inactiveColor;

		// Hovered on Play
		if (hoveredGameSpeed != -1)
		{
			auto setPlayButtonColor = [&](int32 buttonSpeed)
			{
				if (buttonSpeed > gameSpeedToDisplay) {
					return buttonSpeed <= hoveredGameSpeed ? hoverColor : inactiveColor;
				}
				else { // buttonSpeed <= gameSpeed
					return buttonSpeed > hoveredGameSpeed ? hoverOffColor : activeColor;
				}
			};

			play1Color = setPlayButtonColor(GameSpeedValue1);
			play2Color = setPlayButtonColor(GameSpeedValue2);
			play3Color = setPlayButtonColor(GameSpeedValue3);
			play4Color = setPlayButtonColor(GameSpeedValue4);
			
		}
		else
		{
			switch (gameSpeedToDisplay)
			{
			case 0: pauseColor = activeColor;
				break;
			case GameSpeedValue4: play4Color = activeColor;
			case GameSpeedValue3: play3Color = activeColor;
			case GameSpeedValue2: play2Color = activeColor;
			case GameSpeedValue1: play1Color = activeColor;
			}
		}

		PauseIcon->SetColorAndOpacity(pauseColor);
		PlayIcon1->SetColorAndOpacity(play1Color);
		PlayIcon2->SetColorAndOpacity(play2Color);
		PlayIcon3->SetColorAndOpacity(play3Color);
		PlayIcon4->SetColorAndOpacity(play4Color);


#define LOCTEXT_NAMESPACE "EscMenuUI"
		FText PlayButtonTipText = LOCTEXT("PlayButton_Tip", "Change to {0}<space>(current: {1})<space>Hotkey: <Orange>[{2}]</>");
		AddToolTip(PauseButton, gameSpeedToDisplay == 0 ? LOCTEXT("Resume game", "Resume game") : LOCTEXT("Pause game desc", "Pause game<space>Hotkey: <Orange>[spacebar]</>"));
		AddToolTip(PlayButton1, FText::Format(PlayButtonTipText, GameSpeedName(GameSpeedValue1), GameSpeedName(gameSpeedToDisplay), TEXT_NUM(1)));
		AddToolTip(PlayButton2, FText::Format(PlayButtonTipText, GameSpeedName(GameSpeedValue2), GameSpeedName(gameSpeedToDisplay), TEXT_NUM(2)));
		AddToolTip(PlayButton3, FText::Format(PlayButtonTipText, GameSpeedName(GameSpeedValue3), GameSpeedName(gameSpeedToDisplay), TEXT_NUM(3)));
		AddToolTip(PlayButton4, FText::Format(PlayButtonTipText, GameSpeedName(GameSpeedValue4), GameSpeedName(gameSpeedToDisplay), TEXT_NUM(4)));

#undef LOCTEXT_NAMESPACE

		// Close Flash animation
		if (UGameplayStatics::GetTimeSeconds(this) - lastTutorialAttentionOpen > 9.9f &&
			TutorialAttentionFlash->GetVisibility() != ESlateVisibility::Collapsed)
		{
			TutorialAttentionFlash->SetVisibility(ESlateVisibility::Collapsed);
			TryStopAnimation("TutorialAttention");
		}


		/*
		 * Player victorious
		 */
		if (InterfacesInvalid()) {
			return;
		}
		
		if (!alreadyShownVictory &&
			simulation().endStatus().gameEndEnum != GameEndEnum::None)
		{
			auto gameInstance = CastChecked<UPunGameInstance>(GetGameInstance());

			FGameEndStatus endStatus = simulation().endStatus();
			endStatus.playerId = playerId();
			gameInstance->SetGameEndStatus(endStatus);

			VictoryPopup->SetVisibility(ESlateVisibility::Visible);
			
			alreadyShownVictory = true;
		}


		//
		if (overlayTypeToChangeTo != OverlayType::None) {
			dataSource()->SetOverlayType(overlayTypeToChangeTo, OverlaySetterType::OverlayToggler);
			overlayTypeToChangeTo = OverlayType::None;
		}

		TickLoadingScreen();
	}

	void TickLoadingScreen()
	{
		/*
		 * Loading screen
		 */
		if (LoadingScreen->loadingScreenHideCountdown >= 0.0f)
		{
			LoadingScreen->loadingScreenHideCountdown -= UGameplayStatics::GetWorldDeltaSeconds(this);
			LoadingScreen->loadingScreenHideCountdown = fmax(LoadingScreen->loadingScreenHideCountdown, 0.0f);

			if (LoadingScreen->loadingScreenHideCountdown == 0.0f) {
				LoadingScreen->SetVisibility(ESlateVisibility::Collapsed);
			}
			else
			{
				LoadingScreen->GameStartBlockerBackground2->GetDynamicMaterial()->SetScalarParameterValue("InputTime", 3.0f - LoadingScreen->loadingScreenHideCountdown);
			}
		}
	}

	float lastShaderCachePrint = 0.0f;

	// This is before simulation exist
	void SetLoadingText(FText message) {
#if !UI_ALL
		return;
#endif
		LoadingScreen->LoadingTextBox->SetVisibility(ESlateVisibility::HitTestInvisible);
		LoadingScreen->Logo->SetVisibility(ESlateVisibility::HitTestInvisible);
		
		LoadingScreen->LoadingText->SetText(message);
	}
	void HideLoadingScreen() {
#if !UI_ALL
		return;
#endif
		if (LoadingScreen->GameStartBlockerBackground1->GetVisibility() != ESlateVisibility::Collapsed) {
			StartLoadingScreenFade();
		}
	}
	void StartLoadingScreenFade()
	{
		LoadingScreen->loadingScreenHideCountdown = 3.0f;
		LoadingScreen->SetVisibility(ESlateVisibility::HitTestInvisible);
		LoadingScreen->GameStartBlockerBackground1->SetVisibility(ESlateVisibility::Collapsed);
		LoadingScreen->GameStartBlockerBackground2->SetVisibility(ESlateVisibility::HitTestInvisible);

		LoadingScreen->LoadingText->SetVisibility(ESlateVisibility::Collapsed);
		LoadingScreen->LoadingTextBox->SetVisibility(ESlateVisibility::Collapsed);
		LoadingScreen->Logo->SetVisibility(ESlateVisibility::Collapsed);
	}
	

	void KeyPressed_Escape()
	{
		// Close Delete Save Confirmation
		if (LoadSaveUI->KeyPressed_Escape())
		{
			
		}
		// Close ExitConfirm to EscMenu
		else if (ConfirmUI->GetVisibility() != ESlateVisibility::Collapsed) {
			ConfirmUI->SetVisibility(ESlateVisibility::Collapsed);
			ConfirmBlur->SetVisibility(ESlateVisibility::Collapsed);
			BackBlur->SetVisibility(ESlateVisibility::Collapsed);

			dataSource()->Spawn2DSound("UI", "UIWindowClose");
		}
		// Close Settings out to EscMenu
		else if (GameSettingsUI->GetVisibility() != ESlateVisibility::Collapsed) {
			GameSettingsUI->SetVisibility(ESlateVisibility::Collapsed);
			
			EscMenu->SetVisibility(ESlateVisibility::Visible);

			dataSource()->Spawn2DSound("UI", "UIWindowClose");
		}
		// Close Save out to EscMenu
		else if (LoadSaveUI->GetVisibility() != ESlateVisibility::Collapsed) {
			LoadSaveUI->SetVisibility(ESlateVisibility::Collapsed);
			EscMenu->SetVisibility(ESlateVisibility::Visible);
			BackBlur->SetVisibility(ESlateVisibility::Collapsed);

			dataSource()->Spawn2DSound("UI", "UIWindowClose");
		}
		// Close Esc Menu
		else if (EscMenu->GetVisibility() != ESlateVisibility::Collapsed) {
			EscMenu->SetVisibility(ESlateVisibility::Collapsed);

			dataSource()->Spawn2DSound("UI", "UIWindowClose");
		}
		// Nothing up, bring up EscMenu
		else {
			networkInterface()->ResetGameUI();
			EscMenu->SetVisibility(ESlateVisibility::Visible);
			dataSource()->Spawn2DSound("UI", "UIWindowOpen");
		}

		CloseOverlayUI();
	}

	void RightMouseUp() {
		CloseOverlayUI();
	}

	void CloseOverlayUI() {
		OverlaySettingsOverlay->SetVisibility(ESlateVisibility::Collapsed);
		//dataSource()->SetOverlayType(OverlayType::None, OverlaySetterType::OverlayToggler);
	}


	// Pause game when in the esc menu or tutorial
	bool ShouldPauseGameFromUI() {
		return gameInstance()->isSinglePlayer && 
				(EscMenu->IsVisible() || TutorialUI->IsVisible() || GameSettingsUI->IsVisible());
	}
	bool _isPausingFromEscMenu = false;

private:
	UFUNCTION() void ToggleEscMenu() {
		ToggleUI(EscMenu);
	}
	UFUNCTION() void OnClickEscMenuResumeButton();
	UFUNCTION() void OnClickEscMenuSettingsButton();
	UFUNCTION() void OnClickEscMenuRestartGameButton();
	UFUNCTION() void OnClickEscMenuExitToMainMenuButton();
	UFUNCTION() void OnClickEscMenuQuitButton();

	UFUNCTION() void ToggleTutorial() {
		ToggleUI(TutorialUI);

		TutorialAttentionFlash->SetVisibility(ESlateVisibility::Collapsed);
		TryStopAnimation("TutorialAttention");
	}

	UFUNCTION() void OnClickConfirmYesButton();
	UFUNCTION() void OnClickConfirmNoButton();

	UFUNCTION() void OnClickEscMenuSaveButton();
	UFUNCTION() void OnClickEscMenuLoadButton();

	/*
	 * Hover play
	 */
	int32 hoveredGameSpeed = -1;
	UFUNCTION() void OnHoverEnterPlayButton1() { SetHoveredGameSpeed(GameSpeedValue1); }
	UFUNCTION() void OnHoverEnterPlayButton2() { SetHoveredGameSpeed(GameSpeedValue2); }
	UFUNCTION() void OnHoverEnterPlayButton3() { SetHoveredGameSpeed(GameSpeedValue3); }
	UFUNCTION() void OnHoverEnterPlayButton4() { SetHoveredGameSpeed(GameSpeedValue4); }
	
	UFUNCTION() void OnHoverExitPlayButton() {
		if (networkInterface()->IsHost()) {
			PUN_LOG("OnHoverExitPlayButton");
			hoveredGameSpeed = -1;
		}
	}
	void SetHoveredGameSpeed(int32 hoveredGameSpeedIn) {
		if (networkInterface()->IsHost()) {
			PUN_LOG("OnHoverEnterPlayButton");
			hoveredGameSpeed = hoveredGameSpeedIn;
		}
	}



	/*
	 * Click Play
	 */
	UFUNCTION() void OnClickPlayButton1() { 
		OnClickPlayButton(GameSpeedValue1);
	}
	UFUNCTION() void OnClickPlayButton2() { 
		OnClickPlayButton(GameSpeedValue2);
	}
	UFUNCTION() void OnClickPlayButton3() { 
		OnClickPlayButton(GameSpeedValue3);
	}
	UFUNCTION() void OnClickPlayButton4() { 
		OnClickPlayButton(GameSpeedValue4);
	}

	void OnClickPlayButton(int32 newGameSpeed)
	{
		if (!networkInterface()->IsHost()) {
			return;
		}
		// Do not allow speed adjustment before start loc is chosen..
		if (!simulation().AllPlayerHasTownhallAfterInitialTicks() ||
			ShouldPauseGameFromUI()) 
		{
			return;
		}
		networkInterface()->SetGameSpeed(newGameSpeed);
	}

	UFUNCTION() void OnClickPauseButton()
	{
		if (!networkInterface()->IsHost()) {
			return;
		}
		// Do not allow speed adjustment before start loc is chosen..
		if (!simulation().AllPlayerHasTownhallAfterInitialTicks() ||
			ShouldPauseGameFromUI()) 
		{
			return;
		}
		
		if (networkInterface()->hostGameSpeed() == 0) {
			PUN_LOG("Click Pause: Resume");
			networkInterface()->Resume();
		}
		else {
			PUN_LOG("Click Pause: Pause");
			networkInterface()->Pause();
		}
	}

	/*
	 * Overlay
	 */
	UFUNCTION() void OnCheckOverlay_None(bool active) {
		dataSource()->SetOverlayType(OverlayType::None, OverlaySetterType::OverlayToggler);
		overlayTypeToChangeTo = OverlayType::None;

		SetOverlayCheckBox(OverlayCheckBox_None);
	}

	// Set OverlayCheckBoxes so there is only one active.
	void SetOverlayCheckBox(UCheckBox* activeCheckBox);
	
	UFUNCTION() void OnClickOverlayToggler() {
		if (OverlaySettingsOverlay->GetVisibility() != ESlateVisibility::Collapsed) {
			CloseOverlayUI();
		} else {
			networkInterface()->ResetGameUI();
			OverlaySettingsOverlay->SetVisibility(ESlateVisibility::SelfHitTestInvisible);

			// Ensure checkboxes are up to date
			MapCheckBox_HideTrees->SetIsChecked(dataSource()->isHidingTree());
			MapCheckBox_ProvinceOverlay->SetIsChecked(dataSource()->isShowingProvinceOverlay());
			
			//dataSource()->SetOverlayType(OverlayType::Farm, OverlaySetterType::OverlayToggler);
			//SetOverlayCheckBox(OverlayCheckBox_Fertility);
		}
	}
	UFUNCTION() void OnCheckOverlay_Fertility(bool active) {
		dataSource()->SetOverlayType(OverlayType::None, OverlaySetterType::OverlayToggler);
		overlayTypeToChangeTo = OverlayType::Farm;

		SetOverlayCheckBox(OverlayCheckBox_Fertility);
	}
	UFUNCTION() void OnCheckOverlay_Appeal(bool active) {
		dataSource()->SetOverlayType(OverlayType::None, OverlaySetterType::OverlayToggler);
		overlayTypeToChangeTo = OverlayType::Appeal;

		SetOverlayCheckBox(OverlayCheckBox_Appeal);
	}
	UFUNCTION() void OnCheckOverlay_Animals(bool active) {
		dataSource()->SetOverlayType(OverlayType::None, OverlaySetterType::OverlayToggler);
		overlayTypeToChangeTo = OverlayType::Hunter;

		SetOverlayCheckBox(OverlayCheckBox_Animals);
	}

	OverlayType overlayTypeToChangeTo = OverlayType::None;

	UFUNCTION() void OnCheckMap_HideTrees(bool active) {
		dataSource()->SetOverlayHideTree(active);
	}

	UFUNCTION() void OnCheckMap_ProvinceOverlay(bool active) {
		dataSource()->SetOverlayProvince(active);
	}

	

	UFUNCTION() void OnClickVictoryPopupScoreScreen() {
		ScoreScreenDelayOverlay->SetVisibility(ESlateVisibility::Visible);
		networkInterface()->GoToVictoryScreen();
	}
	UFUNCTION() void OnClickVictoryPopupReturnToGame() {
		VictoryPopup->SetVisibility(ESlateVisibility::Collapsed);
	}

private:
	void CallBack2(UPunWidget* punWidgetCaller, CallbackEnum callbackEnum) override;

	void ToggleUI(UWidget* widget)
	{
		if (widget->GetVisibility() != ESlateVisibility::Collapsed) {
			widget->SetVisibility(ESlateVisibility::Collapsed);
			dataSource()->Spawn2DSound("UI", "UIWindowClose");
		}
		else {
			networkInterface()->ResetGameUI();
			widget->SetVisibility(ESlateVisibility::Visible);
			dataSource()->Spawn2DSound("UI", "UIWindowOpen");
		}
	}

	
private:
	enum ConfirmEnum {
		RestartGame,
		ExitToMainMenu,
		ExitGame,
		OverrideSave,
	} _confirmEnum;

	bool wasGamePausedFromUI = false;
};
