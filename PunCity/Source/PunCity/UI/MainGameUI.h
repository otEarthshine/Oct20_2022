// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

//#include "ResearchTypeButtonUI.h"
#include "PunWidget.h"
#include "IconTextPairWidget.h"

#include "BuildingPlacementButton.h"
#include "PunItemSelectionChoice.h"
#include <algorithm>
#include "Components/WidgetSwitcher.h"
#include "GameSettingsUI.h"
#include "JobPriorityRow.h"
#include "ConfirmUI.h"
#include "WGT_ButtonCpp.h"

#include "MainGameUI.generated.h"

/**
 * 
 */
UCLASS()
class UMainGameUI : public UPunWidget //, public UResearchTypeButtonUIParent
{
	GENERATED_BODY()
public:
	void PunInit();
	void Tick();
	void RightMouseUp();
	bool EscDown();

	//UBuildingPlacementButton* AddAnimationCard(CardEnum buildingEnum);
	//UBuildingPlacementButton* AddCardOld(CardHandEnum cardHandEnum, CardEnum buildingEnum, UWrapBox* buttonParent, CallbackEnum callbackEnum, int32 cardHandIndex = -1,
	//														int32 buildingLvl = -1, int32 stackSize = -1, bool isMiniature = false){}
	UBuildingPlacementButton* AddCard(CardHandEnum cardHandEnum, CardStatus cardStatus, UWrapBox* buttonParent, CallbackEnum callbackEnum, int32 cardHandIndex = -1, bool isMiniature = false);

	void CallBack1(UPunWidget* punWidgetCaller, CallbackEnum callbackEnum) override;
	void CallBack2(UPunWidget* punWidgetCaller, CallbackEnum callbackEnum) override;


	//void OnCancelPlacement()
	//{
	//	GatherSettingsOverlay->SetVisibility(ESlateVisibility::Collapsed);

	//	SetButtonImage(BuildMenuTogglerImage, false);
	//	SetButtonImage(GatherImage, false);
	//	SetButtonImage(DemolishImage, false);
	//}

	void CheckChildrenPointerOnUI() {
		CheckPointerOnUI(ResourceOverlay);
		CheckPointerOnUI(GatherSettingsOverlay);
	}

	void ResetBottomMenuDisplay();

	void ResetGameUI()
	{
		// Needed for ExclusiveUIEnum
		
		ResetBottomMenuDisplay();

		if (RareCardHandOverlay->GetVisibility() != ESlateVisibility::Collapsed) {
			GetPunHUD()->QueueStickyExclusiveUI(ExclusiveUIEnum::RareCardHand);
		}

		//if (CardHand1Overlay->GetVisibility() != ESlateVisibility::Collapsed) {
		//	dataSource()->Spawn2DSound("UI", "CardDeal");
		//}
		CardHand1Overlay->SetVisibility(ESlateVisibility::Collapsed);
		RareCardHandOverlay->SetVisibility(ESlateVisibility::Collapsed);
		ConverterCardHandOverlay->SetVisibility(ESlateVisibility::Collapsed);

		CardInventorySizeBox->SetVisibility(ESlateVisibility::Collapsed);
		
		BuildMenuOverlay->SetVisibility(ESlateVisibility::Collapsed);
		
		ConfirmationOverlay->SetVisibility(ESlateVisibility::Collapsed);


		ReinforcementOverlay->SetVisibility(ESlateVisibility::Collapsed);
		CardSetsUIOverlay->SetVisibility(ESlateVisibility::Collapsed);

		CloseReinforcementUI();
	}
	
	
	//bool IsChoosingCard() {
	//	return CardHand1Overlay->GetVisibility() == ESlateVisibility::Visible ||
	//		RareCardHandOverlay->GetVisibility() == ESlateVisibility::Visible ||
	//		ConverterCardHandOverlay->GetVisibility() == ESlateVisibility::Visible ||
	//		BuildMenuOverlay->GetVisibility() != ESlateVisibility::Collapsed;
	//}

	bool IsHoveredOnScrollUI() {
		return ConverterCardHandOverlay->IsHovered() ||
				JobPriorityOverlay->IsHovered() ||

			ResourceListScrollOverlay->IsHovered() ||
			ResourceListScrollBox->IsHovered();
	}

	void ShowConfirmationUI(FText confirmationStr, std::shared_ptr<FNetworkCommand> commandIn) {
		confirmationString = confirmationStr;
		onConfirmationYesCommand = commandIn;
		ConfirmationText->SetText(confirmationStr);

		networkInterface()->ResetGameUI();
		ConfirmationOverlay->SetVisibility(ESlateVisibility::Visible);

		//_onConfirmationNumUITask = GetPunHUD()->NumberOfUITasks();
	}

	bool IsShowingConfirmationUI(FText confirmationStrIn) {
		return  ConfirmationOverlay->IsVisible() && TextEquals(confirmationStrIn, confirmationString);
	}

	void ToggleRoad(bool isStoneRoad)
	{
		if (inputSystemInterface()->placementState() == (isStoneRoad ? PlacementType::StoneRoad : PlacementType::DirtRoad)) {
			inputSystemInterface()->CancelPlacement();
		} else {
			inputSystemInterface()->StartRoadPlacement(isStoneRoad);
		}
	}

	void SelectPermanentCard(CardEnum buildingEnum);

	
	/*
	 * Toggle
	 */
	UFUNCTION() void ToggleBuildingMenu();
	UFUNCTION() void ToggleGatherButton();
	UFUNCTION() void CloseGatherUI();
	UFUNCTION() void ToggleDemolishButton();
	UFUNCTION() void ToggleCardInventoryButton();
	
	UFUNCTION() void ToggleResearchMenu();
	UFUNCTION() void ToggleProsperityUI();

	UFUNCTION() void ToggleStatisticsUI()
	{
		bool statOpened = GetPunHUD()->IsStatisticsUIOpened();
		if (statOpened) {
			GetPunHUD()->CloseStatisticsUI();
		} else {
			GetPunHUD()->OpenStatisticsUI(playerId());
		}
		//SetButtonImage(StatsImage, !statOpened); // Reverse, since we just toggled
	}

	UFUNCTION() void ClickCardStackButton();

	UFUNCTION() void OnClickLeaderSkillButton();

	void ToggleJobPriorityUI(int32 townIdIn) {
		JobPriorityOverlay->SetVisibility(JobPriorityOverlay->GetVisibility() == ESlateVisibility::Collapsed ? ESlateVisibility::Visible : ESlateVisibility::Collapsed);
		jobPriorityTownId = townIdIn;
	}


public:
	//UPROPERTY(meta = (BindWidget)) UOverlay* AnimatedCardOverlay;
	
	UPROPERTY(meta = (BindWidget)) UOverlay* BuildMenuOverlay;
	UPROPERTY(meta = (BindWidget)) UOverlay* CardHand1Overlay;
	UPROPERTY(meta = (BindWidget)) UOverlay* RareCardHandOverlay;
	UPROPERTY(meta = (BindWidget)) UOverlay* ConverterCardHandOverlay;
	UPROPERTY(meta = (BindWidget)) UTextBlock* ConverterCardHandTitle;
	UPROPERTY(meta = (BindWidget)) USizeBox* CardInventorySizeBox;

	UPROPERTY(meta = (BindWidget)) UEditableTextBox* ConverterCardHandSearchBox;
	FString lastSearchString;

	UPROPERTY(meta = (BindWidget)) UOverlay* ConfirmationOverlay;


	//UPROPERTY(meta = (BindWidget)) UOverlay* TestMainMenuOverlay1;
	//UPROPERTY(meta = (BindWidget)) UOverlay* TestMainMenuOverlay2;
	
	FSlateFontInfo DefaultFont;

	// For Non-card Hide
	UPROPERTY(meta = (BindWidget)) UHorizontalBox* BuildGatherMenu;
	UPROPERTY(meta = (BindWidget)) USizeBox* CardStackSizeBox;
	UPROPERTY(meta = (BindWidget)) UOverlay* RoundCountdownOverlay;
	UPROPERTY(meta = (BindWidget)) UOverlay* LeaderSkillOverlay;

	
private:
	UPROPERTY(meta = (BindWidget)) UWGT_ButtonCpp* BuildMenuTogglerButton;
	UPROPERTY(meta = (BindWidget)) UImage* BuildMenuTogglerImage;

	UPROPERTY(meta = (BindWidget)) UWrapBox* BuildingMenuWrap;


	UPROPERTY(meta = (BindWidget)) UWGT_ButtonCpp* GatherButton;
	UPROPERTY(meta = (BindWidget)) UImage* GatherImage;
	UPROPERTY(meta = (BindWidget)) UOverlay* GatherSettingsOverlay;
	UPROPERTY(meta = (BindWidget)) UWGT_ButtonCpp* GatherSettingsCloseButton;

	//UPROPERTY(meta = (BindWidget)) UOverlay* StatsButtonOverlay;
	//UPROPERTY(meta = (BindWidget)) UButton* StatsButton;
	//UPROPERTY(meta = (BindWidget)) UImage* StatsImage;

	UPROPERTY(meta = (BindWidget)) UCheckBox* HarvestCheckBox_All;
	UPROPERTY(meta = (BindWidget)) UCheckBox* HarvestCheckBox_Wood;
	UPROPERTY(meta = (BindWidget)) UCheckBox* HarvestCheckBox_NonFruitTrees;
	UPROPERTY(meta = (BindWidget)) UCheckBox* HarvestCheckBox_Stone;
	UPROPERTY(meta = (BindWidget)) UCheckBox* RemoveHarvestCheckBox_All;
	UPROPERTY(meta = (BindWidget)) UCheckBox* RemoveHarvestCheckBox_Wood;
	UPROPERTY(meta = (BindWidget)) UCheckBox* RemoveHarvestCheckBox_Stone;

	bool shouldCloseGatherSettingsOverlay = true;


	// Card Inventory
	UPROPERTY(meta = (BindWidget)) UButton* CardInventoryToggleButton;
	UPROPERTY(meta = (BindWidget)) UImage* CardInventoryLinkImage;
	
	UPROPERTY(meta = (BindWidget)) UWrapBox* CardInventoryUIWrap;
	

	UPROPERTY(meta = (BindWidget)) UHorizontalBox* PopulationBox;
	UPROPERTY(meta = (BindWidget)) UTextBlock* AdultPopulationText;
	UPROPERTY(meta = (BindWidget)) UTextBlock* ChildPopulationText;

	UPROPERTY(meta = (BindWidget)) UHorizontalBox* HousingSpaceBox;
	UPROPERTY(meta = (BindWidget)) UTextBlock* HousingSpaceText;

	UPROPERTY(meta = (BindWidget)) UHorizontalBox* StorageSpaceBox;
	UPROPERTY(meta = (BindWidget)) UTextBlock* StorageSpaceText;
	//UPROPERTY(meta = (BindWidget)) UTextBlock* Food;

	// Town Swap
	UPROPERTY(meta = (BindWidget)) UHorizontalBox* LeftUITownSwapHorizontalBox;
	UPROPERTY(meta = (BindWidget)) UTextBlock* LeftUITownSwapText;
	UPROPERTY(meta = (BindWidget)) UButton* LeftUITownSwapArrowLeftButton;
	UPROPERTY(meta = (BindWidget)) UButton* LeftUITownSwapArrowRightButton;
	

	// Main Info
	UPROPERTY(meta = (BindWidget)) UIconTextPairWidget* Money;
	UPROPERTY(meta = (BindWidget)) UTextBlock* MoneyChangeText;
	UPROPERTY(meta = (BindWidget)) UIconTextPairWidget* Influence;
	UPROPERTY(meta = (BindWidget)) UTextBlock* InfluenceChangeText;
	UPROPERTY(meta = (BindWidget)) UIconTextPairWidget* Science;
	
	UPROPERTY(meta = (BindWidget)) UIconTextPairWidget* Happiness;

	UPROPERTY(meta = (BindWidget)) UButton* LeftUIPopulationButton;
	UPROPERTY(meta = (BindWidget)) UButton* LeftUIMoneyButton;
	//UPROPERTY(meta = (BindWidget)) UButton* LeftUIInfluenceButton;
	UPROPERTY(meta = (BindWidget)) UButton* LeftUIScienceButton;

	//UPROPERTY(meta = (BindWidget)) UTextBlock* AnimalsNeedingRanch;

	//UPROPERTY(meta = (BindWidget)) UIconTextPairWidget* WoodCount;
	UPROPERTY(meta = (BindWidget)) UVerticalBox* FuelList;
	UPROPERTY(meta = (BindWidget)) UVerticalBox* MedicineList;
	UPROPERTY(meta = (BindWidget)) UVerticalBox* ToolsList;
	
	UPROPERTY(meta = (BindWidget)) UVerticalBox* MainResourceList;
	UPROPERTY(meta = (BindWidget)) UVerticalBox* MainFoodList;
	UPROPERTY(meta = (BindWidget)) UVerticalBox* LuxuryTier1List;
	UPROPERTY(meta = (BindWidget)) UVerticalBox* LuxuryTier2List;
	UPROPERTY(meta = (BindWidget)) UVerticalBox* LuxuryTier3List;

	UPROPERTY(meta = (BindWidget)) UButton* FoodCountButton;
	UPROPERTY(meta = (BindWidget)) UTextBlock* FoodCountText;
	UPROPERTY(meta = (BindWidget)) UTextBlock* LuxuryTier1Text;
	UPROPERTY(meta = (BindWidget)) UTextBlock* LuxuryTier2Text;
	UPROPERTY(meta = (BindWidget)) UTextBlock* LuxuryTier3Text;
	
	UPROPERTY(meta = (BindWidget)) UOverlay* ResourceOverlay;

	UPROPERTY(meta = (BindWidget)) UOverlay* ResourceListScrollOverlay;
	UPROPERTY(meta = (BindWidget)) UScrollBox* ResourceListScrollBox;

	//UPROPERTY(meta = (BindWidget)) class UTextBlock* TopLeftText;
	UPROPERTY(meta = (BindWidget)) UTextBlock* TimeText;
	UPROPERTY(meta = (BindWidget)) UTextBlock* YearText;

	UPROPERTY(meta = (BindWidget)) UHorizontalBox* TemperatureTextBox;
	UPROPERTY(meta = (BindWidget)) URichTextBlock* TemperatureText;
	UPROPERTY(meta = (BindWidget)) UImage* TemperatureImage;

	UPROPERTY(meta = (BindWidget)) UOverlay* MidScreenMessage;
	UPROPERTY(meta = (BindWidget)) UTextBlock* MidScreenMessageText;

	UPROPERTY(meta = (BindWidget)) UButton* ResearchBarUI;
	UPROPERTY(meta = (BindWidget)) UTextBlock* ResearchingText;
	UPROPERTY(meta = (BindWidget)) UHorizontalBox* ResearchingLeftoverAmountBox;
	UPROPERTY(meta = (BindWidget)) UTextBlock* ResearchingLeftoverAmountText;
	
	UPROPERTY(meta = (BindWidget)) UTextBlock* ResearchingAmountText;
	UPROPERTY(meta = (BindWidget)) USizeBox* ResearchBarBox;
	UPROPERTY(meta = (BindWidget)) USizeBox* ResearchBar;

	//! Job Priority

	UPROPERTY(meta = (BindWidget)) UOverlay* JobPriorityOverlay;
	UPROPERTY(meta = (BindWidget)) UScrollBox* JobPriorityScrollBox;
	UPROPERTY(meta = (BindWidget)) UButton* JobPriorityCloseButton;
	UPROPERTY(meta = (BindWidget)) UButton* JobPriorityCloseButton2;
	UPROPERTY() TArray<UJobPriorityRow*> JobPriorityRows;
	int32 jobPriorityTownId = -1;

	// Town Swap (Job)
	UPROPERTY(meta = (BindWidget)) UHorizontalBox* TownSwapHorizontalBox;
	UPROPERTY(meta = (BindWidget)) UTextBlock* TownSwapText;
	UPROPERTY(meta = (BindWidget)) UButton* TownSwapArrowLeftButton;
	UPROPERTY(meta = (BindWidget)) UButton* TownSwapArrowRightButton;

	UPROPERTY(meta = (BindWidget)) UTextBlock* EmployedText;

	UPROPERTY(meta = (BindWidget)) UHorizontalBox* LaborerBox;
	UPROPERTY(meta = (BindWidget)) UCheckBox* LaborerManualCheckBox;
	UPROPERTY(meta = (BindWidget)) UTextBlock* Laborer;
	UPROPERTY(meta = (BindWidget)) UTextBlock* LaborerRed;
	UPROPERTY(meta = (BindWidget)) UButton* LaborerArrowUp;
	UPROPERTY(meta = (BindWidget)) UButton* LaborerArrowDown;
	UPROPERTY(meta = (BindWidget)) USizeBox* LaborerArrowOverlay;

	UPROPERTY(meta = (BindWidget)) UHorizontalBox* BuilderBox;
	UPROPERTY(meta = (BindWidget)) UCheckBox* BuilderManualCheckBox;
	UPROPERTY(meta = (BindWidget)) UTextBlock* Builder;
	UPROPERTY(meta = (BindWidget)) UButton* BuilderArrowUp;
	UPROPERTY(meta = (BindWidget)) UButton* BuilderArrowDown;
	UPROPERTY(meta = (BindWidget)) USizeBox* BuilderArrowOverlay;

	FSetTownPriority& townPriorityState() { return _laborerPriorityState.townPriorityState; }
	LaborerPriorityState _laborerPriorityState;

	void SendNetworkPriority()
	{
		if (jobPriorityTownId != -1)
		{
			auto command = std::make_shared<FSetTownPriority>();
			*command = townPriorityState();
			command->townId = jobPriorityTownId;
			networkInterface()->SendNetworkCommand(command);

			_laborerPriorityState.lastPriorityInputTime = UGameplayStatics::GetTimeSeconds(this);
		}
	}

	void RefreshLaborerUI()
	{
		_laborerPriorityState.RefreshUILaborerPriority(
			this,
			&simulation(),
			jobPriorityTownId,

			nullptr,
			EmployedText,

			LaborerBox,
			LaborerManualCheckBox, LaborerArrowOverlay,
			Laborer,
			LaborerRed,

			BuilderBox,
			BuilderManualCheckBox,
			Builder,
			BuilderArrowOverlay
		);
	}

	// Laborer
	UFUNCTION() void OnCheckManualLaborer(bool isChecked) {
		townPriorityState().laborerPriority = isChecked;
		RefreshLaborerUI();
		SendNetworkPriority();
	}
	UFUNCTION() void IncreaseLaborers() {
		townPriorityState().targetLaborerCount++;
		RefreshLaborerUI();
		SendNetworkPriority();

		dataSource()->Spawn2DSound("UI", "UIIncrementalChange");
	}
	UFUNCTION() void DecreaseLaborers() {
		townPriorityState().targetLaborerCount = std::max(0, townPriorityState().targetLaborerCount - 1);
		RefreshLaborerUI();
		SendNetworkPriority();

		dataSource()->Spawn2DSound("UI", "UIIncrementalChange");
	}

	// Builder
	UFUNCTION() void OnCheckManualBuilder(bool isChecked) {
		townPriorityState().builderPriority = isChecked;
		RefreshLaborerUI();
		SendNetworkPriority();
	}
	UFUNCTION() void IncreaseBuilders() {
		townPriorityState().targetBuilderCount++;
		RefreshLaborerUI();
		SendNetworkPriority();

		dataSource()->Spawn2DSound("UI", "UIIncrementalChange");
	}
	UFUNCTION() void DecreaseBuilders() {
		townPriorityState().targetBuilderCount = std::max(0, townPriorityState().targetBuilderCount - 1);
		RefreshLaborerUI();
		SendNetworkPriority();

		dataSource()->Spawn2DSound("UI", "UIIncrementalChange");
	}

	/*
	 * Prosperity
	 */
	
	UPROPERTY(meta = (BindWidget)) UButton* ProsperityBarUI;
	UPROPERTY(meta = (BindWidget)) USizeBox* ProsperityBarBox;
	UPROPERTY(meta = (BindWidget)) USizeBox* ProsperityBar;
	UPROPERTY(meta = (BindWidget)) UTextBlock* ProsperityAmountText;
	UPROPERTY(meta = (BindWidget)) UTextBlock* UpgradeTitleText;


	UPROPERTY(meta = (BindWidget)) UExclamationIcon* ExclamationIcon_Build;
	UPROPERTY(meta = (BindWidget)) UExclamationIcon* ExclamationIcon_Gather;
	UPROPERTY(meta = (BindWidget)) UExclamationIcon* ExclamationIcon_CardStack;

	// Cards
	BoolEnum _lastIsCardStackBlank = BoolEnum::NeedUpdate;
	int32 _lastQueueCount = 0;
	
	//std::vector<BuildingEnum> _lastDisplayHand;
	std::vector<bool> _lastHand1ReserveStatus;
	std::vector<bool> _lastHand1NeedMoneyOrInfluenceStatus;
	
	std::vector<bool> _lastRareHandReserveStatus;
	std::vector<std::shared_ptr<Quest>> _lastQuests;

	CardEnum _lastConverterChosenCard = CardEnum::None;
	
	std::vector<CardStatus> _lastDisplayBought;
	CardEnum _lastCardEnumBeingPlaced = CardEnum::None;
	ConverterCardUseState _lastConverterCardState = ConverterCardUseState::None;
	//int32_t _lastMoney = -1;

	// Card Hand 1
	UPROPERTY(meta = (BindWidget)) UWrapBox* CardHand1Box;
	UPROPERTY(meta = (BindWidget)) UButton* CardHand1SubmitButton;
	UPROPERTY(meta = (BindWidget)) UButton* CardHand1CancelButton;
	UPROPERTY(meta = (BindWidget)) UWGT_ButtonCpp* CardHand1CloseButton;

	UPROPERTY(meta = (BindWidget)) UTextBlock* CardHand1SubmitButtonText;

	//bool _needDrawHandDisplay = false;

	// Rare Cards
	UPROPERTY(meta = (BindWidget)) UWrapBox* RareCardHandBox;
	UPROPERTY(meta = (BindWidget)) UButton* RareCardHandSubmitButton;
	UPROPERTY(meta = (BindWidget)) UTextBlock* RareCardHandText;
	UPROPERTY(meta = (BindWidget)) UTextBlock* RareCardHandText2;
	int32 rareHandObjectId = -1;

	//bool _needRareCardDisplay = false;

	// Converter Hand
	UPROPERTY(meta = (BindWidget)) UWrapBox* ConverterCardHandBox;
	UPROPERTY(meta = (BindWidget)) UButton* ConverterCardHandCancelButton;
	UPROPERTY(meta = (BindWidget)) UWGT_ButtonCpp* ConverterCardHandXCloseButton;
	UPROPERTY(meta = (BindWidget)) UConfirmUI* ConverterCardHandConfirmUI;

	int32 converterHandCategoryState = -1;
	int32 lastConverterHandCategoryState = -1;
	UPROPERTY(meta = (BindWidget)) UImage* ConverterCardHand_AgricultureActiveImage;
	UPROPERTY(meta = (BindWidget)) UImage* ConverterCardHand_IndustryActiveImage;
	UPROPERTY(meta = (BindWidget)) UImage* ConverterCardHand_ServicesActiveImage;
	UPROPERTY(meta = (BindWidget)) UImage* ConverterCardHand_OthersActiveImage;

	UPROPERTY(meta = (BindWidget)) UButton* ConverterCardHand_AgricultureButton;
	UPROPERTY(meta = (BindWidget)) UButton* ConverterCardHand_IndustryButton;
	UPROPERTY(meta = (BindWidget)) UButton* ConverterCardHand_ServicesButton;
	UPROPERTY(meta = (BindWidget)) UButton* ConverterCardHand_OthersButton;
	
	
	
	UPROPERTY(meta = (BindWidget)) UWrapBox* CardHand2Box;

	UPROPERTY(meta = (BindWidget)) UButton* CardStackButton;
	UPROPERTY(meta = (BindWidget)) USizeBox* CardStackBlank;
	UPROPERTY(meta = (BindWidget)) USizeBox* CardStack1;
	UPROPERTY(meta = (BindWidget)) USizeBox* CardStack2;
	UPROPERTY(meta = (BindWidget)) USizeBox* CardStack3;
	UPROPERTY(meta = (BindWidget)) USizeBox* CardStack4;
	UPROPERTY(meta = (BindWidget)) USizeBox* CardStack5;
	UPROPERTY(meta = (BindWidget)) UTextBlock* CardHandCount;
	
	UPROPERTY(meta = (BindWidget)) UButton* CardRerollButton;
	UPROPERTY(meta = (BindWidget)) UTextBlock* RerollPrice;
	UPROPERTY(meta = (BindWidget)) USizeBox* RerollCoinIcon;

	UPROPERTY(meta = (BindWidget)) USizeBox* CardRerollBox1;
	UPROPERTY(meta = (BindWidget)) UButton* CardRerollButton1;
	UPROPERTY(meta = (BindWidget)) UTextBlock* RerollPrice1;
	UPROPERTY(meta = (BindWidget)) USizeBox* RerollCoinIcon1;
	
	UPROPERTY(meta = (BindWidget)) UTextBlock* RoundCountdownText;
	UPROPERTY(meta = (BindWidget)) UImage* RoundCountdownImage;

	int32 _lastRoundCountdown = -1;

	UPROPERTY(meta = (BindWidget)) UWGT_ButtonCpp* LeaderSkillButton;
	UPROPERTY(meta = (BindWidget)) UImage* LeaderSkillClock;
	UPROPERTY(meta = (BindWidget)) UImage* LeaderManaBar;
	//UPROPERTY(meta = (BindWidget)) UTextBlock* LeaderManaText;

	// Confirmation
	UPROPERTY(meta = (BindWidget)) UButton* ConfirmationYesButton;
	UPROPERTY(meta = (BindWidget)) UButton* ConfirmationNoButton;
	UPROPERTY(meta = (BindWidget)) URichTextBlock* ConfirmationText;

	FText confirmationString;
	std::shared_ptr<FNetworkCommand> onConfirmationYesCommand = nullptr;
	//int32 _onConfirmationNumUITask = 0; // Counts the UI Tasks so that Confirmation will still appear for RareCardUI


	
	// Logs
	UPROPERTY(meta = (BindWidget)) UVerticalBox* EventBox;

	//
	UPROPERTY(meta = (BindWidget)) USizeBox* QuickDebugBox;
	UPROPERTY(meta = (BindWidget)) UTextBlock* QuickDebugText;

	UPROPERTY(meta = (BindWidget)) USizeBox* QuickDebugDesyncBox;
	UPROPERTY(meta = (BindWidget)) UTextBlock* QuickDebugDesyncText;


	/*
	 * Reinforcement UI
	 * 	- Deploy Military
	 */
	UPROPERTY(meta = (BindWidget)) UTextBlock* ReinforcementTitleText;
	UPROPERTY(meta = (BindWidget)) UTextBlock* ReinforcementTitleText2;
	UPROPERTY(meta = (BindWidget)) UWrapBox* ReinforcementSlots;

	UPROPERTY(meta = (BindWidget)) UVerticalBox* ReinforcementRemovedOuterBox;
	UPROPERTY(meta = (BindWidget)) UWrapBox* ReinforcementRemovedSlots;

	UPROPERTY(meta = (BindWidget)) UButton* ReinforcementSubmitButton;
	UPROPERTY(meta = (BindWidget)) UButton* ReinforcementCancelButton;

	UPROPERTY(meta = (BindWidget)) UWGT_ButtonCpp* ReinforcementCloseXButton;

	int32 reinforcementProvinceId = -1;
	CallbackEnum reinforcementCallbackEnum = CallbackEnum::None;

public:
	UPROPERTY(meta = (BindWidget)) UOverlay* ReinforcementOverlay;
	
	void OpenReinforcementUI(int32 provinceId, CallbackEnum callbackEnum)
	{
		reinforcementProvinceId = provinceId;
		reinforcementCallbackEnum = callbackEnum;
		
		ReinforcementOverlay->SetVisibility(ESlateVisibility::Visible);

		if (reinforcementCallbackEnum == CallbackEnum::ReinforceAttackProvince ||
			reinforcementCallbackEnum == CallbackEnum::ReinforceDefendProvince)
		{
			ReinforcementTitleText2->SetText(NSLOCTEXT("ReinforceUI", "Reinforce", "Reinforce"));
		}
		else if (reinforcementCallbackEnum == CallbackEnum::StartAttackProvince) {
			ReinforcementTitleText2->SetText(NSLOCTEXT("ReinforceUI", "Vassalize", "Vassalize"));
		}
		else if (reinforcementCallbackEnum == CallbackEnum::DefendProvinceMoney) {
			ReinforcementTitleText2->SetText(NSLOCTEXT("ReinforceUI", "Defend", "Defend"));
		}
		else if (reinforcementCallbackEnum == CallbackEnum::Liberate) {
			ReinforcementTitleText2->SetText(NSLOCTEXT("ReinforceUI", "Liberate", "Liberate"));
		}
		else if (reinforcementCallbackEnum == CallbackEnum::RaidBattle) {
			ReinforcementTitleText2->SetText(NSLOCTEXT("ReinforceUI", "Raid", "Raid"));
		}
		else if (reinforcementCallbackEnum == CallbackEnum::Raze) {
			ReinforcementTitleText2->SetText(NSLOCTEXT("ReinforceUI", "Raze", "Raze"));
		}
		else if (reinforcementCallbackEnum == CallbackEnum::RazeFort) {
			ReinforcementTitleText2->SetText(NSLOCTEXT("ReinforceUI", "Destroy Fort", "Destroy Fort"));
		}
		else {
			UE_DEBUG_BREAK();
		}
		

		ReinforcementRemovedOuterBox->SetVisibility(ESlateVisibility::Collapsed);

		BUTTON_ON_CLICK(ReinforcementSubmitButton, this, &UMainGameUI::OnClickReinforcementSubmitButton);
		BUTTON_ON_CLICK(ReinforcementCancelButton, this, &UMainGameUI::OnClickReinforcementCancelButton);
		BUTTON_ON_CLICK(ReinforcementCloseXButton->CoreButton, this, &UMainGameUI::OnClickReinforcementCancelButton);

		for (int i = ReinforcementSlots->GetChildrenCount(); i-- > 0;) {
			auto cardButton = CastChecked<UBuildingPlacementButton>(ReinforcementSlots->GetChildAt(i));
			SetChildHUD(cardButton);
		}


		TickReinforcementUI();
	}

	void TickReinforcementUI()
	{
		std::vector<CardStatus>& cards = simulation().cardSystem(playerId()).pendingMilitarySlotCards;

		if (ReinforcementOverlay->IsVisible())
		{
			for (int i = ReinforcementSlots->GetChildrenCount(); i-- > 0;)
			{
				auto cardButton = CastChecked<UBuildingPlacementButton>(ReinforcementSlots->GetChildAt(i));

				cardButton->SetVisibility(ESlateVisibility::Visible);

				cardButton->PunInit(i < cards.size() ? cards[i] : CardStatus(), i, this, CallbackEnum::SelectDeployMilitarySlotCard, CardHandEnum::DeployMilitarySlots);
				cardButton->SetCardStatus(CardHandEnum::DeployMilitarySlots, false, false);
				cardButton->RefreshBuildingIcon(assetLoader());
			}
		}
	}

	UFUNCTION() void OnClickReinforcementSubmitButton();
	UFUNCTION() void OnClickReinforcementCancelButton() {
		CloseReinforcementUI();
	}

	void CloseReinforcementUI(bool clearPendingMilitarySlotCards = true)
	{
		ReinforcementOverlay->SetVisibility(ESlateVisibility::Collapsed);
		reinforcementProvinceId = -1;
		reinforcementCallbackEnum = CallbackEnum::None;

		if (clearPendingMilitarySlotCards) {
			simulation().cardSystem(playerId()).pendingMilitarySlotCards.clear();
		}
	}

public:
	/*
	 * Card Sets UI
	 */
	UPROPERTY(meta = (BindWidget)) UOverlay* CardSetsUIOverlay;
	UPROPERTY(meta = (BindWidget)) UTextBlock* CardSetsUITitleText;

	UPROPERTY(meta = (BindWidget)) UButton* CardSetsUICloseButton;
	UPROPERTY(meta = (BindWidget)) UWGT_ButtonCpp* CardSetsUICloseXButton;

	UPROPERTY(meta = (BindWidget)) UVerticalBox* CardSetsUIOuterVerticalBox;

	CardSetTypeEnum cardSetTypeEnum = CardSetTypeEnum::Zoo;

	void OpenCardSetsUI(CardSetTypeEnum cardSetTypeEnumIn)
	{
		cardSetTypeEnum = cardSetTypeEnumIn;

		CardSetsUIOverlay->SetVisibility(ESlateVisibility::Visible);


		// Clear UI
		for (int i = CardSetsUIOuterVerticalBox->GetChildrenCount(); i-- > 0;)
		{
			auto cardSetBoxRow = CastChecked<UHorizontalBox>(CardSetsUIOuterVerticalBox->GetChildAt(i));
			cardSetBoxRow->SetVisibility(ESlateVisibility::Collapsed);
			
			for (int j = cardSetBoxRow->GetChildrenCount(); j-- > 0;)
			{
				auto cardSetBox = Cast<UVerticalBox>(cardSetBoxRow->GetChildAt(j));
				if (cardSetBox)
				{
					cardSetBox->SetVisibility(ESlateVisibility::Collapsed);
					cardSetBoxRow->GetChildAt(j - 1)->SetVisibility(ESlateVisibility::Collapsed);

					auto cardSetsUISlots = CastChecked<UHorizontalBox>(cardSetBox->GetChildAt(2));
					for (int32 k = cardSetsUISlots->GetChildrenCount(); k-- > 0;) {
						auto cardButton = CastChecked<UBuildingPlacementButton>(cardSetsUISlots->GetChildAt(k));
						cardButton->SetVisibility(ESlateVisibility::Collapsed);
						SetChildHUD(cardButton);
					}
				}
			}
		}
		

		BUTTON_ON_CLICK(CardSetsUICloseButton, this, &UMainGameUI::OnClickCardSetsUICloseButton);
		BUTTON_ON_CLICK(CardSetsUICloseXButton->CoreButton, this, &UMainGameUI::OnClickCardSetsUICloseButton);

		TickCardSetsUI();
	}

	void TickCardSetsUI()
	{
		if (!CardSetsUIOverlay->IsVisible()) {
			return;
		}
		
		// Fill Card Sets
		const std::vector<std::vector<CardStatus>>& cardSets = simulation().cardSystem(playerId()).GetCardSets(cardSetTypeEnum);

		int32 cardSetTypeInfoIndex = 0;
		auto fillCardSet = [&](int32 rowIndex, int32 columnIndex, const std::vector<CardSetInfo>& cardSetTypeInfos)
		{
			columnIndex = columnIndex * 2 + 1;
			
			auto cardSetBoxRow = CastChecked<UHorizontalBox>(CardSetsUIOuterVerticalBox->GetChildAt(rowIndex));
			auto cardSetBox = CastChecked<UVerticalBox>(cardSetBoxRow->GetChildAt(columnIndex));

			cardSetBoxRow->SetVisibility(ESlateVisibility::SelfHitTestInvisible);
			cardSetBox->SetVisibility(ESlateVisibility::SelfHitTestInvisible);
			cardSetBoxRow->GetChildAt(columnIndex - 1)->SetVisibility(ESlateVisibility::SelfHitTestInvisible);

			auto cardSetsUITitleText = CastChecked<UTextBlock>(cardSetBox->GetChildAt(0));
			cardSetsUITitleText->SetText(cardSetTypeInfos[cardSetTypeInfoIndex].name);

			auto cardSetsUIDescriptionText = CastChecked<URichTextBlock>(CastChecked<USizeBox>(cardSetBox->GetChildAt(1))->GetChildAt(0));
			cardSetsUIDescriptionText->SetText(cardSetTypeInfos[cardSetTypeInfoIndex].description);

			auto cardSetsUISlots = CastChecked<UHorizontalBox>(cardSetBox->GetChildAt(2));
			for (int32 i = 0; i < cardSetsUISlots->GetChildrenCount(); i++)
			{
				const std::vector<CardStatus>& cardSet = cardSets[cardSetTypeInfoIndex];
				
				auto cardButton = CastChecked<UBuildingPlacementButton>(cardSetsUISlots->GetChildAt(i));
				if (i < cardSet.size()) {
					cardButton->SetVisibility(ESlateVisibility::Visible);
					cardButton->PunInit(cardSet[i], i, this, CallbackEnum::SelectCardSetSlotCard, CardHandEnum::CardSetSlots);
					cardButton->SetCardStatus(CardHandEnum::CardSetSlots, false, false);
					cardButton->RefreshBuildingIcon(assetLoader());
					cardButton->callbackVar1 = static_cast<int32>(cardSetTypeEnum);
				}
				else {
					cardButton->SetVisibility(ESlateVisibility::Collapsed);
				}
			}

			cardSetTypeInfoIndex++;
		};

		if (cardSetTypeEnum == CardSetTypeEnum::Zoo)
		{
			CardSetsUITitleText->SetText(NSLOCTEXT("CardSetsUI", "Zoo Animal Collection", "Zoo Animal Collection"));

			cardSetTypeInfoIndex = 0;
			fillCardSet(0, 0, ZooSetInfos);
			fillCardSet(0, 1, ZooSetInfos);
			fillCardSet(1, 0, ZooSetInfos);
		}
		else if (cardSetTypeEnum == CardSetTypeEnum::Museum)
		{
			CardSetsUITitleText->SetText(NSLOCTEXT("CardSetsUI", "Museum Artifact Collection", "Museum Artifact Collection"));

			fillCardSet(0, 0, MuseumSetInfos);
			fillCardSet(0, 1, MuseumSetInfos);
			fillCardSet(0, 2, MuseumSetInfos);
			
			fillCardSet(1, 0, MuseumSetInfos);
			fillCardSet(1, 1, MuseumSetInfos);
			fillCardSet(1, 2, MuseumSetInfos);
			
			fillCardSet(2, 0, MuseumSetInfos);
		}
		else
		{
			CardSetsUITitleText->SetText(NSLOCTEXT("CardSetsUI", "Card Combiner", "Card Combiner"));

			fillCardSet(0, 0, CardCombinerSetInfos);
			fillCardSet(0, 1, CardCombinerSetInfos);
			fillCardSet(1, 0, CardCombinerSetInfos);
			fillCardSet(2, 0, CardCombinerSetInfos);
		}
	}

	UFUNCTION() void OnClickCardSetsUICloseButton()
	{
		CardSetsUIOverlay->SetVisibility(ESlateVisibility::Collapsed);
		simulation().cardSystem(playerId()).pendingCardSetsSlotCards.clear();
	}
	
	
private:
	// Confirmation
	UFUNCTION() void OnClickConfirmationYes() {
		PUN_CHECK(onConfirmationYesCommand != nullptr);
		networkInterface()->SendNetworkCommand(onConfirmationYesCommand);
		ConfirmationOverlay->SetVisibility(ESlateVisibility::Collapsed);

		confirmationString = FText();
		onConfirmationYesCommand = nullptr;
	}
	UFUNCTION() void OnClickConfirmationNo() {
		ConfirmationOverlay->SetVisibility(ESlateVisibility::Collapsed);

		confirmationString = FText();
		onConfirmationYesCommand = nullptr;
	}

	
	int32 TokensNeededForTentativeBuy(ResourceEnum resourceEnum);
	
	static void SetButtonImage(UImage* buttonImage, bool active, bool hovered = false) {
		buttonImage->SetVisibility(active ? ESlateVisibility::SelfHitTestInvisible : ESlateVisibility::Collapsed);
	}
	
	/*
	 * Cards
	 */
	
	UFUNCTION() void ClickRerollButton();
	UFUNCTION() void ClickCardHand1SubmitButton();
	UFUNCTION() void ClickCardHand1CancelButton();

	UFUNCTION() void ClickRareCardHandSubmitButton();


	// Card Removal
	CardEnum buildingEnumToRemove = CardEnum::None;
	UFUNCTION() void ClickCardRemovalConfirmYesButton()
	{
		if (buildingEnumToRemove == CardEnum::None) {
			ConverterCardHandConfirmUI->SetVisibility(ESlateVisibility::Collapsed);
			return;
		}
		
		dataSource()->Spawn2DSound("UI", "CardDeal");
		ConverterCardHandOverlay->SetVisibility(ESlateVisibility::Collapsed);
		simulation().cardSystem(playerId()).converterCardState = ConverterCardUseState::SubmittedUI;

		converterHandCategoryState = -1;

		auto command = make_shared<FUseCard>();
		command->cardStatus = CardStatus(CardEnum::CardRemoval, 1);
		command->variable1 = static_cast<int32>(buildingEnumToRemove);
		networkInterface()->SendNetworkCommand(command);
	}
	UFUNCTION() void ClickCardRemovalConfirmNoButton() {
		ConverterCardHandConfirmUI->SetVisibility(ESlateVisibility::Collapsed);
	}

	UFUNCTION() void ClickConverterCardHand_AgricultureButton() {
		converterHandCategoryState = (converterHandCategoryState != 0) ?  0 : -1;
	}
	UFUNCTION() void ClickConverterCardHand_IndustryButton() {
		converterHandCategoryState = (converterHandCategoryState != 1) ? 1 : -1;
	}
	UFUNCTION() void ClickConverterCardHand_ServicesButton() {
		converterHandCategoryState = (converterHandCategoryState != 2) ? 2 : -1;
	}
	UFUNCTION() void ClickConverterCardHand_OthersButton() {
		converterHandCategoryState = (converterHandCategoryState != 3) ? 3 : -1;
	}

	
	UFUNCTION() void ClickConverterCardHandCancelButton();


	UFUNCTION() void OnClickJobPriorityCloseButton() {
		JobPriorityOverlay->SetVisibility(ESlateVisibility::Collapsed);
	}


	UFUNCTION() void OnCheckHarvest_All(bool active) {
		OnCheckHarvest(false, ResourceEnum::None);
	}
	UFUNCTION() void OnCheckHarvest_Wood(bool active) {
		OnCheckHarvest(false, ResourceEnum::Wood);
	}
	UFUNCTION() void OnCheckHarvest_NonFruitTrees(bool active) {
		OnCheckHarvest(false, ResourceEnum::Orange);
	}
	UFUNCTION() void OnCheckHarvest_Stone(bool active) {
		OnCheckHarvest(false, ResourceEnum::Stone);
	}
	UFUNCTION() void OnCheckRemoveHarvest_All(bool active) {
		OnCheckHarvest(true, ResourceEnum::None);
	}
	UFUNCTION() void OnCheckRemoveHarvest_Wood(bool active) {
		OnCheckHarvest(true, ResourceEnum::Wood);
	}
	UFUNCTION() void OnCheckRemoveHarvest_Stone(bool active) {
		OnCheckHarvest(true, ResourceEnum::Stone);
	}

	void OnCheckHarvest(bool isRemoving, ResourceEnum resourceEnum) {
		HarvestCheckBox_All->SetIsChecked(!isRemoving && resourceEnum == ResourceEnum::None);
		HarvestCheckBox_Wood->SetIsChecked(!isRemoving && resourceEnum == ResourceEnum::Wood);
		HarvestCheckBox_NonFruitTrees->SetIsChecked(!isRemoving && resourceEnum == ResourceEnum::Orange);
		HarvestCheckBox_Stone->SetIsChecked(!isRemoving && resourceEnum == ResourceEnum::Stone);
		RemoveHarvestCheckBox_All->SetIsChecked(isRemoving && resourceEnum == ResourceEnum::None);
		RemoveHarvestCheckBox_Wood->SetIsChecked(isRemoving && resourceEnum == ResourceEnum::Wood);
		RemoveHarvestCheckBox_Stone->SetIsChecked(isRemoving && resourceEnum == ResourceEnum::Stone);
		
		PUN_LOG("OnCheckHarvest isRemoving:%d, %d", isRemoving, static_cast<int>(resourceEnum));
		shouldCloseGatherSettingsOverlay = false;
		inputSystemInterface()->CancelPlacement();
		inputSystemInterface()->StartHarvestPlacement(isRemoving, resourceEnum);
	}

	UFUNCTION() void OnClickFoodCountButton() {
		if (GetPunHUD()->IsFoodFuelGraphUIOpened()) {
			GetPunHUD()->CloseStatisticsUI();
		} else {
			GetPunHUD()->OpenFoodFuelGraphUI();
		}
	}

	UFUNCTION() void OnClickPopulationButton() { GetPunHUD()->ToggleGraphUI(2); }
	UFUNCTION() void OnClickMoneyButton() { GetPunHUD()->ToggleGraphUI(3); }
	UFUNCTION() void OnClickScienceButton() { GetPunHUD()->ToggleGraphUI(4); }

	UFUNCTION() void OnClickLeftUITownSwapArrowLeftButton() {
		networkInterface()->CameraSwapTown(false);
	}
	UFUNCTION() void OnClickLeftUITownSwapArrowRightButton() {
		networkInterface()->CameraSwapTown(true);
	}

	UFUNCTION() void OnClickJobTownSwapArrowLeftButton() {
		jobPriorityTownId = simulation().GetNextTown(false, jobPriorityTownId, playerId());
	}
	UFUNCTION() void OnClickJobTownSwapArrowRightButton() {
		jobPriorityTownId = simulation().GetNextTown(true, jobPriorityTownId, playerId());
	}
};
