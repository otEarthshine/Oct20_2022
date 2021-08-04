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
	UBuildingPlacementButton* AddCard(CardHandEnum cardHandEnum, CardEnum buildingEnum, UWrapBox* buttonParent, CallbackEnum callbackEnum, int32 cardHandIndex = -1,
															int32 buildingLvl = -1, int32 stackSize = -1, bool isMiniature = false);

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
		BuildMenuOverlay->SetVisibility(ESlateVisibility::Collapsed);
		ConfirmationOverlay->SetVisibility(ESlateVisibility::Collapsed);
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
		return TextEquals(confirmationStrIn, confirmationString);
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
	UPROPERTY(meta = (BindWidget)) UOverlay* CardInventoryOverlay;
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
	std::vector<bool> _lastHand1NeedMoneyStatus;
	
	std::vector<bool> _lastRareHandReserveStatus;
	std::vector<std::shared_ptr<Quest>> _lastQuests;

	CardEnum _lastConverterChosenCard = CardEnum::None;
	
	std::vector<BuildingCardStack> _lastDisplayBought;
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

	
	int32 MoneyNeededForTentativeBuy();
	
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
		command->cardEnum = CardEnum::CardRemoval;
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
