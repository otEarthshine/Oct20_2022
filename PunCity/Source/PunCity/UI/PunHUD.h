// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "UnrealEngine.h"
#include "GameFramework/HUD.h"
#include "Styling/SlateStyle.h"
#include "Slate/SlateGameResources.h"
#include "UObject/ConstructorHelpers.h"

#include "MainGameUI.h"
#include "PolicyMenu.h"
#include "TradeUIWidgetBase.h"
#include "WorldSpaceUI.h"
#include "ObjectDescriptionUISystem.h"
#include "InitialResourceUI.h"

#include "GameUIInputSystemInterface.h"
#include "../GameNetworkInterface.h"
#include <memory>
#include <string>
#include "WorldTradeUI.h"
#include "PopupUI.h"
#include "QuestUI.h"
#include "EscMenuUI.h"
#include "ChatUI.h"
#include "TechUI.h"
#include "StatisticsUI.h"
#include "ArmyMoveUI.h"
#include "TopLayerGameUI.h"
#include "IntercityTradeUI.h"
#include "TargetConfirmUI.h"
#include "ProsperityUI.h"
#include "GiftResourceUI.h"
#include "DiplomacyUI.h"

#include "PunHUD.generated.h"

USTRUCT()
struct FWidgetPool
{
	GENERATED_BODY();
	UPROPERTY() TArray<UWidget*> widgetArray;
};

/*
 * UI talks to IGameUIDataSource(GameManager) for data.
 * UI issue input system commands like StartBuildingPlacement() to IGameUIInputSystemInterface(CameraPawn).
 * UI could also issue command directly to IGameNetworkInterface(PunPlayerController)
 */
UCLASS()
class APunHUD : public AHUD,
				public IPunHUDInterface,
				public IGameUIInterface
{
	GENERATED_BODY()
public:
	APunHUD();
	~APunHUD(){}

	void PostInitializeComponents() override {
		Super::PostInitializeComponents();
		InitBeforeGameManager();
	}

	void Setup(IPunPlayerController* controller, USceneComponent* worldWidgetParent);
	void PunTick(bool isPhotoMode);

	void SetLoadingText(FString message) final {
#if UI_ALL
		_escMenuUI->SetLoadingText(message);
#endif
	}
	
	/*
	 * Input handling
	 */
	void LeftMouseDown() {
#if UI_ALL
		_descriptionUISystem->LeftMouseDown();
#endif
	}
	void RightMouseUp() {
#if UI_ALL
		_descriptionUISystem->CloseDescriptionUI();
		_mainGameUI->RightMouseUp();
		_escMenuUI->RightMouseUp();
#endif
	}
	virtual void KeyPressed_Escape();


	
	template<typename T>
	T* AddWidgetToHUDCast(UIEnum uiEnum) {
		return CastChecked<T>(AddWidgetToHUD(uiEnum));
	}

	void ToggleMainGameUIActive() {
#if UI_ALL
		_mainGameUI->SetVisibility(_mainGameUI->GetVisibility() == ESlateVisibility::Visible ? ESlateVisibility::Collapsed : ESlateVisibility::Visible);
#endif
	}
	void HideMainGameUI() {
#if UI_ALL
		_mainGameUI->SetVisibility(ESlateVisibility::Collapsed);
#endif
	}

	void ToggleJobUI() {
		//_worldSpaceUI->jobUIActive = !_worldSpaceUI->jobUIActive;
		//_worldSpaceUI->townhallUIActive = !_worldSpaceUI->townhallUIActive;
	}

	UMainGameUI* mainGameUI() { return _mainGameUI; }
	UEscMenuUI* escMenuUI() { return _escMenuUI; }
	UChatUI* chatUI() { return _chatUI; }

	FSlateFontInfo defaultFont() final { return _mainGameUI->DefaultFont; }
	
	bool IsHoveredOnScrollUI() {
#if !UI_ALL
		return false;
#endif
		return _techUI->IsHovered() ||
				_prosperityUI->IsHovered() ||
				_descriptionUISystem->IsHoveredOnScrollUI() ||
				_statisticsUI->IsHovered() ||
				_worldTradeUI->IsHovered() ||

				_giftResourceUI->IsHovered() ||
				_diplomacyUI->IsHovered() ||

				_intercityTradeUI->IsHovered() ||
				_mainGameUI->IsHoveredOnScrollUI() ||
				_chatUI->IsHoveredOnScrollUI() ||
				//_armyMoveUI->IsHovered() ||
				_escMenuUI->IsHovered();
	}
	
	void CloseDescriptionUI() override {
#if UI_ALL
		_descriptionUISystem->CloseDescriptionUI();
#endif
	}


	bool IsStatisticsUIOpened() override {
		return _statisticsUI->GetVisibility() != ESlateVisibility::Collapsed;
	}
	void OpenStatisticsUI(int32 playerId) override {
		ResetGameUI();
		_statisticsUI->OpenStatisticsUI(playerId);
	}
	void OpenJobPriorityUI() override {
		ResetGameUI();
		_mainGameUI->ToggleJobPriorityUI();
	}

	
	bool IsResourcePriceUIOpened(ResourceEnum resourceEnum) override {
		return _statisticsUI->GetVisibility() != ESlateVisibility::Collapsed &&
			_statisticsUI->StatSwitcher->GetActiveWidgetIndex() == 7 &&
			_statisticsUI->simulation().worldTradeSystem().resourceEnumToShowStat == resourceEnum;
	}
	void OpenResourcePriceUI(ResourceEnum resourceEnum) override {
		ResetGameUI();
		_statisticsUI->simulation().worldTradeSystem().resourceEnumToShowStat = resourceEnum;
		_statisticsUI->OpenStatisticsUI(playerId());
		_statisticsUI->SetTabSelection(_statisticsUI->MarketStatButton);
		_statisticsUI->StatSwitcher->SetActiveWidgetIndex(7);
		_statisticsUI->MarketResourceDropdown->SetSelectedOption(ResourceNameF(resourceEnum));
	}

	bool IsFoodFuelGraphUIOpened() override {
		return _statisticsUI->GetVisibility() != ESlateVisibility::Collapsed &&
			_statisticsUI->StatSwitcher->GetActiveWidgetIndex() == 5;
	}
	void OpenFoodFuelGraphUI() override {
		ResetGameUI();
		_statisticsUI->OpenStatisticsUI(playerId());
		_statisticsUI->SetTabSelection(_statisticsUI->FoodFuelStatButton);
		_statisticsUI->StatSwitcher->SetActiveWidgetIndex(5);
	}

	void ToggleGraphUI(int32 widgetIndex) override
	{
		if (_statisticsUI->GetVisibility() != ESlateVisibility::Collapsed &&
			_statisticsUI->StatSwitcher->GetActiveWidgetIndex() == widgetIndex) 
		{
			CloseStatisticsUI();
		} else {
			ResetGameUI();
			_statisticsUI->OpenStatisticsUI(playerId());
			_statisticsUI->SetTabSelection(_statisticsUI->GetButtonFromWidgetIndex(widgetIndex));
			_statisticsUI->StatSwitcher->SetActiveWidgetIndex(widgetIndex);
		}
	}
	
	void CloseStatisticsUI() override {
		_statisticsUI->CloseStatisticsUI();
	}

	void ShowTutorialUI(TutorialLinkEnum linkEnum) override {
		_escMenuUI->ShowTutorialUI(linkEnum);
	}

	// TODO: remove???
	/*
	 * UI tasks
	 *  When doing a UI task:
	 *  - popup should delayed its display until the task is done.
	 *  - Some popup such as ShowTechTree shouldn't show the button.
	 *  -
	 */

	bool IsDoingUITask() override
	{
		return GetCurrentExclusiveUIDisplay() != ExclusiveUIEnum::None;
	}

	// TODO: TEMP
	TArray<UWidget*> GetEmptyCardSlot() override {
		return _descriptionUISystem->GetEmptyCardSlot();
	}

	// Open/close UI
	void OpenTradeUI(int32 objectId) final {
		networkInterface()->ResetGameUI();
		_worldTradeUI->OpenUI(objectId);
	}
	void CloseTradeUI() { _worldTradeUI->CloseUI(); }

	void OpenIntercityTradeUI(int32 objectId) final {
		networkInterface()->ResetGameUI();
		_intercityTradeUI->OpenUI(objectId);
	}
	void CloseIntercityTradeUI() { _intercityTradeUI->CloseUI(); }

	void OpenTargetConfirmUI_IntercityTrade(int32 townhallId, ResourceEnum resourceEnum) final {
		networkInterface()->ResetGameUI();
		_targetConfirmUI->OpenUI(townhallId, resourceEnum);
	}
	void CloseTargetConfirmUI() { _targetConfirmUI->CloseUI(); }

	void OpenGiftUI(int32 targetPlayerId) override {
		_giftResourceUI->OpenUI(targetPlayerId);
	}
	void OpenDiplomacyUI(int32 targetPlayerId) override {
		_diplomacyUI->OpenUI(targetPlayerId);
	}
	
	
	// Tech Tree
	void CloseTechUI() final {
		_techUI->SetVisibility(ESlateVisibility::Collapsed);
	}
	void ToggleTechUI() final {
		_techUI->SetShowUI(_techUI->GetVisibility() == ESlateVisibility::Collapsed);
	}

	// ProsperityTree
	void CloseProsperityUI() final {
		_prosperityUI->SetVisibility(ESlateVisibility::Collapsed);
	}
	void ToggleProsperityUI() final {
		_prosperityUI->SetShowUI(_prosperityUI->GetVisibility() == ESlateVisibility::Collapsed);
	}

	void TogglePlayerDetails() final {
		_questUI->TogglePlayerDetails();
	}
	void CloseQuestUI() {
		_questUI->QuestDescriptionOverlay->SetVisibility(ESlateVisibility::Collapsed);
	}

	void ClosePlayerOverview() {
		_questUI->PlayerDetailsOverlay->SetVisibility(ESlateVisibility::Collapsed);
	}

	void OpenArmyMoveUI(std::shared_ptr<FAttack> armyCommand) final {
		//_armyMoveUI->OpenArmyMoveUI(armyCommand);
	}
	void CloseArmyMoveUI() {
		//_armyMoveUI->CloseArmyMoveUI();
	}

	void ResetGameUI()
	{
		mainGameUI()->ResetGameUI();
		
		CloseTechUI();
		CloseProsperityUI();

		CloseTradeUI();
		CloseIntercityTradeUI();
		CloseTargetConfirmUI();
		
		CloseQuestUI();
		CloseDescriptionUI();
		CloseStatisticsUI();
		ClosePlayerOverview();

		_diplomacyUI->CloseUI();
		_giftResourceUI->CloseUI();

		_escMenuUI->EscMenu->SetVisibility(ESlateVisibility::Collapsed);
		_escMenuUI->TutorialUI->SetVisibility(ESlateVisibility::Collapsed);

		_escMenuUI->CloseOverlayUI();
	}
	
	/*
	 * Determine what exclusive UI to display
	 * .. Difficulty is that RareCardHand can't be closed, only hidden
	 *  - solve by putting RareCardHand behind the display queue when it needs to be shown after another UI close
	 */
	std::vector<ExclusiveUIEnum> _stickyUIQueue;

	//  Note: this is only for non-human sticky UI that needs to be queued for display
	void QueueStickyExclusiveUI(ExclusiveUIEnum exclusiveUIEnum) override
	{
		_stickyUIQueue.push_back(exclusiveUIEnum);
	}

	bool IsShowingExclusiveUI(ExclusiveUIEnum exclusiveUIEnum) override
	{
		switch (exclusiveUIEnum)
		{
		case ExclusiveUIEnum::CardHand1:		return _mainGameUI->CardHand1Overlay->GetVisibility() == ESlateVisibility::Visible;
		case ExclusiveUIEnum::RareCardHand:		return _mainGameUI->RareCardHandOverlay->GetVisibility() == ESlateVisibility::Visible;
		case ExclusiveUIEnum::ConverterCardHand:return _mainGameUI->ConverterCardHandOverlay->GetVisibility() == ESlateVisibility::Visible;
		case ExclusiveUIEnum::BuildMenu:		return _mainGameUI->BuildMenuOverlay->GetVisibility() != ESlateVisibility::Collapsed;
		case ExclusiveUIEnum::Placement:		return inputSystemInterface()->placementState() != PlacementType::None;
		case ExclusiveUIEnum::ConfirmingAction: return _mainGameUI->ConfirmationOverlay->GetVisibility() != ESlateVisibility::Collapsed;
		case ExclusiveUIEnum::EscMenu:			return _escMenuUI->EscMenu->GetVisibility() != ESlateVisibility::Collapsed;

		case ExclusiveUIEnum::TechUI:			return _techUI->GetVisibility() != ESlateVisibility::Collapsed;
		case ExclusiveUIEnum::ProsperityUI:		return _prosperityUI->GetVisibility() != ESlateVisibility::Collapsed;
			
		case ExclusiveUIEnum::Trading:			return _worldTradeUI->GetVisibility() != ESlateVisibility::Collapsed;
		case ExclusiveUIEnum::IntercityTrading:	return _intercityTradeUI->GetVisibility() != ESlateVisibility::Collapsed;
		case ExclusiveUIEnum::TargetConfirm:	return _targetConfirmUI->GetVisibility() != ESlateVisibility::Collapsed;
			
		case ExclusiveUIEnum::QuestUI:			return _questUI->QuestDescriptionOverlay->GetVisibility() != ESlateVisibility::Collapsed;
		case ExclusiveUIEnum::StatisticsUI:		return _statisticsUI->GetVisibility() != ESlateVisibility::Collapsed;
		case ExclusiveUIEnum::PlayerOverviewUI:	return _questUI->PlayerDetailsOverlay->GetVisibility() != ESlateVisibility::Collapsed;
		//case ExclusiveUIEnum::ArmyMoveUI:		return _armyMoveUI->GetVisibility() != ESlateVisibility::Collapsed;

		case ExclusiveUIEnum::InitialResourceUI:return _initialResourceUI->InitialResourceUI->IsVisible();
		case ExclusiveUIEnum::DiplomacyUI:		return _diplomacyUI->IsVisible();
		case ExclusiveUIEnum::GiftResourceUI:	return _giftResourceUI->IsVisible();
		default:
			UE_DEBUG_BREAK();
			return false;
		}
	}

	ExclusiveUIEnum GetCurrentExclusiveUIDisplay() override
	{
		for (size_t i = 0; i < ExclusiveUIEnumCount; i++) {
			if (IsShowingExclusiveUI(static_cast<ExclusiveUIEnum>(i))) {
				return static_cast<ExclusiveUIEnum>(i);
			}
		}
		return ExclusiveUIEnum::None;
	}
	
	void UpdateExclusiveUIDisplay()
	{
		// Don't update exclusiveUI while the system is moving camera
		// This prevent premature sound on initial rare card hand.
		if (inputSystemInterface()->isSystemMovingCamera()) {
			return;
		}
		
		// Sticky UI
		// Rare Card Hand can never go away, only hidden
		ExclusiveUIEnum currentUIEnum = GetCurrentExclusiveUIDisplay();
		if (currentUIEnum == ExclusiveUIEnum::None && _stickyUIQueue.size() > 0)
		{
			ExclusiveUIEnum enumToDisplay = _stickyUIQueue[0];
			_stickyUIQueue.erase(_stickyUIQueue.begin());

			if (enumToDisplay == ExclusiveUIEnum::RareCardHand) {
				dataSource()->Spawn2DSound("UI", "ChooseRareCard");
				_mainGameUI->RareCardHandOverlay->SetVisibility(ESlateVisibility::Visible);
			}
		}
	}

public:
	/*
	 * IGameUIInterface
	 */
	void ShowFloatupInfo(FloatupInfo floatupInfo) override
	{
#if !UI_ALL
		return;
#endif
		PUN_CHECK(floatupInfo.tile.isValid());
		if (dataSource()->IsInSampleRange(floatupInfo.tile)) {
			_worldSpaceUI->AddFloatupInfo(floatupInfo);
		}
	}
	void ShowFloatupInfo(FloatupEnum floatupEnum, WorldTile2 tile, std::string text, 
						ResourceEnum resourceEnum = ResourceEnum::None, std::string text2 = "") override
	{
#if !UI_ALL
		return;
#endif
		PUN_CHECK(tile.isValid());
		if (dataSource()->IsInSampleRange(tile)) {
			FloatupInfo floatupInfo(floatupEnum, Time::Ticks(), tile, text, resourceEnum, text2);
			_worldSpaceUI->AddFloatupInfo(floatupInfo);
		}
	}

	void PunLog(std::string text) {
#if UI_ALL
		_chatUI->PunLog(text);
#endif
	}

	void AutosaveGame() final {
		escMenuUI()->LoadSaveUI->SaveGameDelayed(true);
		PUN_LOG("AutosaveGame %d %d", static_cast<int>(escMenuUI()->GetVisibility()),
										static_cast<int>(escMenuUI()->LoadSaveUI->GetVisibility()));
	}

public:
	/*
	 * IPunHUDInterface
	 */
	UUserWidget* AddWidgetToHUD(UIEnum uiEnum) final {
		return AddWidgetToHUDBase(_uiClasses[static_cast<int>(uiEnum)], uiEnum);
	}

	UUserWidget* AddWidget(UIEnum uiEnum) final
	{
		DEBUG_AI_VAR(AddWidget);
		
		TSubclassOf<UUserWidget> widgetClass = _uiClasses[static_cast<int>(uiEnum)];
		check(widgetClass);
		UUserWidget* widget = CreateWidget<UUserWidget>(GetWorld(), widgetClass);
		check(widget);

		if (UPunWidget* punWidget = Cast<UPunWidget>(widget)) {
			punWidget->SetHUD(this, uiEnum);
		}

		return widget;
	}

	TSubclassOf<UUserWidget> GetPunWidgetClass(UIEnum uiEnum) final {
		return _uiClasses[static_cast<int>(uiEnum)];
	}

	int32 playerId() final { return _controllerInterface->networkInterface()->playerId(); }
	IGameUIDataSource* dataSource() final {  return _controllerInterface ? _controllerInterface->dataSource() : nullptr; }
	IGameUIInputSystemInterface* inputSystemInterface() final {  return _controllerInterface->inputSystemInterface(); }
	IGameNetworkInterface* networkInterface() final { return _controllerInterface->networkInterface(); }

	bool IsInvalid() final {
		return !(_controllerInterface->dataSource() && _controllerInterface->inputSystemInterface() && _controllerInterface->networkInterface());
	}
	bool IsSimulationInvalid() final {
		return IsInvalid() || !dataSource()->HasSimulation();
	}

	UWorld* GetWorldPun() final { return GetWorld(); }

	bool ShouldPauseGameFromUI() override {
		return _escMenuUI->ShouldPauseGameFromUI();
	}

private:
	UUserWidget* AddWidgetToHUDBase(TSubclassOf<UUserWidget> widgetClass, UIEnum uiEnum) {
		check(widgetClass);
		UUserWidget* widget = CreateWidget<UUserWidget>(GetWorld(), widgetClass);
		check(widget);
		widget->AddToViewport();

		if (UPunWidget* punWidget = Cast<UPunWidget>(widget)) {
			punWidget->SetHUD(this, uiEnum);
		}

		return widget;
	}

	void InitBeforeGameManager();

protected:
	void LoadClass(UIEnum uiEnum, std::string path)
	{
		// Note: If crashed here, may be forgot putting in name corresponding to Enum?
		path.insert(0, "/Game/UI/");
		ConstructorHelpers::FClassFinder<UUserWidget> finder(*FString(path.c_str()));
		if (!finder.Succeeded()) {
			if (GEngine) GEngine->AddOnScreenDebugMessage(-1, 3.0f, FColor::White, TEXT("Failed to get class from path: %s"), *FString(path.c_str()));
		}
		_uiClasses[static_cast<int>(uiEnum)] = finder.Class;
	}




protected:
	bool _isInitialized = false;

	bool _isUIHideMode = false;
	std::vector<ESlateVisibility> uiVisibilityBeforeUIHideMode;

	//! PunPlayerController is a reliable interface because it will always be destroyed after the HUD (since it owns the HUD);
	//! Therefore we won't have to worry about stale pointer
	IPunPlayerController* _controllerInterface = nullptr;

	UPROPERTY() TArray<TSubclassOf<UUserWidget>> _uiClasses;

	//! UIs
	UPROPERTY() UMainGameUI* _mainGameUI;

	UPROPERTY() UEscMenuUI* _escMenuUI;
	UPROPERTY() UQuestUI* _questUI;
	UPROPERTY() UPopupUI* _popupUI;
	UPROPERTY() UChatUI* _chatUI;

	UPROPERTY() UTopLayerGameUI* _topLayerGameUI;
	
	UPROPERTY() UPolicyMenu* _policyMenu;

	//UPROPERTY() UTradeUIWidgetBase* _tradeUIWidget;

	UPROPERTY() UWorldTradeUI* _worldTradeUI;
	UPROPERTY() UIntercityTradeUI* _intercityTradeUI;
	UPROPERTY() UTargetConfirmUI* _targetConfirmUI;
	UPROPERTY() UInitialResourceUI* _initialResourceUI;
	UPROPERTY() UDiplomacyUI* _diplomacyUI;
	UPROPERTY() UGiftResourceUI* _giftResourceUI;

	UPROPERTY() TSubclassOf<UUserWidget> _dragCardSlotClass;
	UPROPERTY() TSubclassOf<UUserWidget> _dragCardClass;

	UPROPERTY() UWorldSpaceUI* _worldSpaceUI;

	UPROPERTY() UObjectDescriptionUISystem* _descriptionUISystem;

	UPROPERTY() UStatisticsUI* _statisticsUI;

	UPROPERTY() UTechUI* _techUI;
	UPROPERTY() UProsperityUI* _prosperityUI;

	//UPROPERTY() UArmyMoveUI* _armyMoveUI;

	TSharedPtr<FSlateGameResources> _style;

public:
	/*
	 * Pooling
	 */
	UWidget* SpawnWidget(UIEnum uiEnum) final
	{
		DEBUG_AI_VAR(AddWidget);

		TSubclassOf<UUserWidget> widgetClass = _uiClasses[static_cast<int>(uiEnum)];
		check(widgetClass);


		TArray<UWidget*>& widgetPoolArray = uiEnumToWidgetPool[static_cast<int32>(uiEnum)].widgetArray;

		UWidget* widget = nullptr;
		if (widgetPoolArray.Num() > 0) {
			widget = widgetPoolArray.Pop(false);
		} else {
			widget = CastChecked<UWidget>(CreateWidget<UUserWidget>(GetWorld(), widgetClass));
		}
		
		check(widget);

		if (UPunWidget* punWidget = Cast<UPunWidget>(widget)) {
			punWidget->SetHUD(this, uiEnum);
		}

		return widget;
	}

	void DespawnWidget(UIEnum uiEnum, UWidget* widget) final
	{
		uiEnumToWidgetPool[static_cast<int32>(uiEnum)].widgetArray.Add(widget);
	}

	/*
	 * Pooling GraphDataSource
	 */
	//UGraphDataSource* SpawnGraphDataSource() final
	//{
	//	UGraphDataSource* graphDataSource = nullptr;
	//	if (graphDataSourcePool.Num() > 0) {
	//		graphDataSource = graphDataSourcePool.Pop(false);
	//	} else {
	//		graphDataSource = NewObject<UGraphDataSource>(this);
	//	}
	//	return graphDataSource;
	//}

	//void DespawnGraphDataSourcet(UGraphDataSource* widget) final {
	//	graphDataSourcePool.Add(widget);
	//}

private:
	// Pooling
	UPROPERTY(meta = (BindWidget)) TArray<FWidgetPool> uiEnumToWidgetPool;

	//UPROPERTY(meta = (BindWidget)) TArray<UGraphDataSource*> graphDataSourcePool;
};
