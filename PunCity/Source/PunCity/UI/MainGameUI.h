// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "ResearchTypeButtonUI.h"
#include "PunWidget.h"
#include "IconTextPairWidget.h"

#include "BuildingPlacementButton.h"
#include "PunItemSelectionChoice.h"
#include <algorithm>
#include "Components/WidgetSwitcher.h"
#include "GameSettingsUI.h"
#include "JobPriorityRow.h"

#include "MainGameUI.generated.h"

/**
 * 
 */
UCLASS()
class UMainGameUI : public UPunWidget, public UResearchTypeButtonUIParent
{
	GENERATED_BODY()
public:
	void PunInit();
	void Tick();
	void RightMouseDown();
	void EscDown();

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

	void ResetGameUI() {
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
				JobPriorityOverlay->IsHovered();
	}

	void ShowConfirmationUI(std::string confirmationStr, std::shared_ptr<FNetworkCommand> commandIn) {
		confirmationString = confirmationStr;
		onConfirmationYesCommand = commandIn;
		ConfirmationText->SetText(ToFText(confirmationStr));

		networkInterface()->ResetGameUI();
		ConfirmationOverlay->SetVisibility(ESlateVisibility::Visible);

		//_onConfirmationNumUITask = GetPunHUD()->NumberOfUITasks();
	}

	bool IsShowingConfirmationUI(std::string confirmationStrIn) {
		return confirmationStrIn == confirmationString;
	}

	void ToggleDirtRoad()
	{
		if (inputSystemInterface()->placementState() == PlacementType::DirtRoad) {
			inputSystemInterface()->CancelPlacement();
		} else {
			inputSystemInterface()->StartRoadPlacement(false);
		}
	}


	
	/*
	 * Toggle
	 */
	UFUNCTION() void ToggleBuildingMenu();
	UFUNCTION() void ToggleGatherButton();
	UFUNCTION() void ToggleDemolishButton();
	
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
		SetButtonImage(StatsImage, !statOpened); // Reverse, since we just toggled
	}

	UFUNCTION() void ClickCardStackButton();

	UFUNCTION() void OnClickLeaderSkillButton();

	void ToggleJobPriorityUI() {
		JobPriorityOverlay->SetVisibility(JobPriorityOverlay->GetVisibility() == ESlateVisibility::Collapsed ? ESlateVisibility::Visible : ESlateVisibility::Collapsed);
	}

public:
	//UPROPERTY(meta = (BindWidget)) UOverlay* AnimatedCardOverlay;
	
	UPROPERTY(meta = (BindWidget)) UOverlay* BuildMenuOverlay;
	UPROPERTY(meta = (BindWidget)) UOverlay* CardHand1Overlay;
	UPROPERTY(meta = (BindWidget)) UOverlay* RareCardHandOverlay;
	UPROPERTY(meta = (BindWidget)) UOverlay* ConverterCardHandOverlay;
	UPROPERTY(meta = (BindWidget)) UTextBlock* ConverterCardHandTitle;

	UPROPERTY(meta = (BindWidget)) UOverlay* ConfirmationOverlay;


	UPROPERTY(meta = (BindWidget)) UOverlay* TestMainMenuOverlay1;
	UPROPERTY(meta = (BindWidget)) UOverlay* TestMainMenuOverlay2;
	
	FSlateFontInfo DefaultFont;

	// For Non-card Hide
	UPROPERTY(meta = (BindWidget)) UHorizontalBox* BuildGatherMenu;
	UPROPERTY(meta = (BindWidget)) USizeBox* CardStackSizeBox;
	UPROPERTY(meta = (BindWidget)) UOverlay* RoundCountdownOverlay;
	UPROPERTY(meta = (BindWidget)) UOverlay* LeaderSkillOverlay;

	
private:
	UPROPERTY(meta = (BindWidget)) UButton* BuildMenuTogglerButton;
	UPROPERTY(meta = (BindWidget)) UImage* BuildMenuTogglerImage;

	UPROPERTY(meta = (BindWidget)) UWrapBox* BuildingMenuWrap;


	UPROPERTY(meta = (BindWidget)) UButton* GatherButton;
	UPROPERTY(meta = (BindWidget)) UImage* GatherImage;
	UPROPERTY(meta = (BindWidget)) UOverlay* GatherSettingsOverlay;

	UPROPERTY(meta = (BindWidget)) UOverlay* StatsButtonOverlay;
	UPROPERTY(meta = (BindWidget)) UButton* StatsButton;
	UPROPERTY(meta = (BindWidget)) UImage* StatsImage;

	UPROPERTY(meta = (BindWidget)) UCheckBox* HarvestCheckBox_All;
	UPROPERTY(meta = (BindWidget)) UCheckBox* HarvestCheckBox_Wood;
	UPROPERTY(meta = (BindWidget)) UCheckBox* HarvestCheckBox_NonFruitTrees;
	UPROPERTY(meta = (BindWidget)) UCheckBox* HarvestCheckBox_Stone;
	UPROPERTY(meta = (BindWidget)) UCheckBox* RemoveHarvestCheckBox_All;
	UPROPERTY(meta = (BindWidget)) UCheckBox* RemoveHarvestCheckBox_Wood;
	UPROPERTY(meta = (BindWidget)) UCheckBox* RemoveHarvestCheckBox_Stone;

	bool shouldCloseGatherSettingsOverlay = true;
	

	UPROPERTY(meta = (BindWidget)) UButton* DemolishButton;
	UPROPERTY(meta = (BindWidget)) UImage* DemolishImage;

	UPROPERTY(meta = (BindWidget)) UHorizontalBox* PopulationBox;
	UPROPERTY(meta = (BindWidget)) UTextBlock* AdultPopulationText;
	UPROPERTY(meta = (BindWidget)) UTextBlock* ChildPopulationText;

	UPROPERTY(meta = (BindWidget)) UHorizontalBox* HousingSpaceBox;
	UPROPERTY(meta = (BindWidget)) UTextBlock* HousingSpaceText;

	UPROPERTY(meta = (BindWidget)) UHorizontalBox* StorageSpaceBox;
	UPROPERTY(meta = (BindWidget)) UTextBlock* StorageSpaceText;
	//UPROPERTY(meta = (BindWidget)) UTextBlock* Food;

	// Main Info
	UPROPERTY(meta = (BindWidget)) UIconTextPairWidget* Happiness;
	UPROPERTY(meta = (BindWidget)) UIconTextPairWidget* Money;
	UPROPERTY(meta = (BindWidget)) UTextBlock* MoneyChangeText;
	UPROPERTY(meta = (BindWidget)) UIconTextPairWidget* Influence;
	UPROPERTY(meta = (BindWidget)) UTextBlock* InfluenceChangeText;
	UPROPERTY(meta = (BindWidget)) UIconTextPairWidget* Science;


	UPROPERTY(meta = (BindWidget)) UTextBlock* AnimalsNeedingRanch;

	//UPROPERTY(meta = (BindWidget)) UIconTextPairWidget* WoodCount;
	UPROPERTY(meta = (BindWidget)) UVerticalBox* FuelList;
	UPROPERTY(meta = (BindWidget)) UVerticalBox* MedicineList;
	UPROPERTY(meta = (BindWidget)) UVerticalBox* ToolsList;
	
	UPROPERTY(meta = (BindWidget)) UVerticalBox* MainResourceList;
	UPROPERTY(meta = (BindWidget)) UVerticalBox* MainFoodList;
	UPROPERTY(meta = (BindWidget)) UVerticalBox* LuxuryTier1List;
	UPROPERTY(meta = (BindWidget)) UVerticalBox* LuxuryTier2List;
	UPROPERTY(meta = (BindWidget)) UVerticalBox* LuxuryTier3List;

	UPROPERTY(meta = (BindWidget)) UTextBlock* FoodCountText;
	UPROPERTY(meta = (BindWidget)) UTextBlock* LuxuryTier1Text;
	UPROPERTY(meta = (BindWidget)) UTextBlock* LuxuryTier2Text;
	UPROPERTY(meta = (BindWidget)) UTextBlock* LuxuryTier3Text;
	
	UPROPERTY(meta = (BindWidget)) UOverlay* ResourceOverlay;

	//UPROPERTY(meta = (BindWidget)) class UTextBlock* TopLeftText;
	UPROPERTY(meta = (BindWidget)) UTextBlock* TimeText;

	UPROPERTY(meta = (BindWidget)) UHorizontalBox* TemperatureTextBox;
	UPROPERTY(meta = (BindWidget)) URichTextBlock* TemperatureText;
	UPROPERTY(meta = (BindWidget)) UImage* TemperatureImage;

	UPROPERTY(meta = (BindWidget)) UOverlay* MidScreenMessage;
	UPROPERTY(meta = (BindWidget)) UTextBlock* MidScreenMessageText;

	UPROPERTY(meta = (BindWidget)) UButton* ResearchBarUI;
	UPROPERTY(meta = (BindWidget)) UTextBlock* ResearchingText;
	UPROPERTY(meta = (BindWidget)) UTextBlock* ResearchingAmountText;
	UPROPERTY(meta = (BindWidget)) USizeBox* ResearchBar;

	// Job Priority

	UPROPERTY(meta = (BindWidget)) UOverlay* JobPriorityOverlay;
	UPROPERTY(meta = (BindWidget)) UScrollBox* JobPriorityScrollBox;
	UPROPERTY(meta = (BindWidget)) UButton* JobPriorityCloseButton;
	UPROPERTY(meta = (BindWidget)) UButton* JobPriorityCloseButton2;
	UPROPERTY() TArray<UJobPriorityRow*> JobPriorityRows;

	UPROPERTY(meta = (BindWidget)) UTextBlock* EmployedText;

	UPROPERTY(meta = (BindWidget)) UHorizontalBox* LaborerBox;
	UPROPERTY(meta = (BindWidget)) UButton* LaborerNonPriorityButton;
	UPROPERTY(meta = (BindWidget)) UButton* LaborerPriorityButton;
	UPROPERTY(meta = (BindWidget)) UTextBlock* Laborer;
	UPROPERTY(meta = (BindWidget)) UTextBlock* LaborerRed;
	UPROPERTY(meta = (BindWidget)) UButton* LaborerArrowUp;
	UPROPERTY(meta = (BindWidget)) UButton* LaborerArrowDown;
	UPROPERTY(meta = (BindWidget)) USizeBox* LaborerArrowOverlay;

	UPROPERTY(meta = (BindWidget)) UHorizontalBox* BuilderBox;
	UPROPERTY(meta = (BindWidget)) UButton* BuilderNonPriorityButton;
	UPROPERTY(meta = (BindWidget)) UButton* BuilderPriorityButton;
	UPROPERTY(meta = (BindWidget)) UTextBlock* Builder;
	UPROPERTY(meta = (BindWidget)) UButton* BuilderArrowUp;
	UPROPERTY(meta = (BindWidget)) UButton* BuilderArrowDown;
	UPROPERTY(meta = (BindWidget)) USizeBox* BuilderArrowOverlay;

	FSetTownPriority& townPriorityState() { return _laborerPriorityState.townPriorityState; }
	LaborerPriorityState _laborerPriorityState;

	void SendNetworkPriority()
	{
		auto command = std::make_shared<FSetTownPriority>();
		*command = townPriorityState();
		networkInterface()->SendNetworkCommand(command);

		_laborerPriorityState.lastPriorityInputTime = UGameplayStatics::GetTimeSeconds(this);
	}

	void RefreshLaborerUI()
	{
		_laborerPriorityState.RefreshUI(
			this,
			&simulation(),
			playerId(),

			nullptr,
			EmployedText,

			LaborerBox,
			LaborerPriorityButton, LaborerNonPriorityButton, LaborerArrowOverlay,
			Laborer,
			LaborerRed,

			BuilderBox,
			BuilderNonPriorityButton,
			BuilderPriorityButton,
			Builder,
			BuilderArrowOverlay
		);
	}

	// Laborer
	UFUNCTION() void OnClickLaborerNonPriorityButton() {
		townPriorityState().laborerPriority = true;
		RefreshLaborerUI();
		SendNetworkPriority();
	}
	UFUNCTION() void OnClickLaborerPriorityButton() {
		townPriorityState().laborerPriority = false;
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
	UFUNCTION() void OnClickBuilderNonPriorityButton() {
		townPriorityState().builderPriority = true;
		RefreshLaborerUI();
		SendNetworkPriority();
	}
	UFUNCTION() void OnClickBuilderPriorityButton() {
		townPriorityState().builderPriority = false;
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
	UPROPERTY(meta = (BindWidget)) USizeBox* ProsperityBar;
	UPROPERTY(meta = (BindWidget)) UTextBlock* ProsperityAmountText;

	//UPROPERTY(meta = (BindWidget)) UButton* ChooseLocationButton;
	//UPROPERTY(meta = (BindWidget)) UOverlay* WorldMapRegionUI;
	//UPROPERTY(meta = (BindWidget)) UTextBlock* WorldMapRegionText;

	UPROPERTY(meta = (BindWidget)) UExclamationIcon* ExclamationIcon_Build;
	UPROPERTY(meta = (BindWidget)) UExclamationIcon* ExclamationIcon_Gather;
	UPROPERTY(meta = (BindWidget)) UExclamationIcon* ExclamationIcon_CardStack;

	// Cards
	bool _lastIsCardStackBlank = true;
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
	UPROPERTY(meta = (BindWidget)) UButton* CardHand1CloseButton;

	//bool _needDrawHandDisplay = false;

	// Rare Cards
	UPROPERTY(meta = (BindWidget)) UWrapBox* RareCardHandBox;
	UPROPERTY(meta = (BindWidget)) UButton* RareCardHandSubmitButton;
	UPROPERTY(meta = (BindWidget)) UTextBlock* RareCardHandText;
	UPROPERTY(meta = (BindWidget)) UTextBlock* RareCardHandText2;

	//bool _needRareCardDisplay = false;

	// Converter Hand
	UPROPERTY(meta = (BindWidget)) UWrapBox* ConverterCardHandBox;
	//UPROPERTY(meta = (BindWidget)) UButton* ConverterCardHandSubmitButton;
	UPROPERTY(meta = (BindWidget)) UButton* ConverterCardHandCancelButton;
	
	UPROPERTY(meta = (BindWidget)) UWrapBox* CardHand2Box;

	UPROPERTY(meta = (BindWidget)) UButton* CardStackButton;
	UPROPERTY(meta = (BindWidget)) USizeBox* CardStackBlank;
	UPROPERTY(meta = (BindWidget)) USizeBox* CardStack1;
	UPROPERTY(meta = (BindWidget)) USizeBox* CardStack2;
	UPROPERTY(meta = (BindWidget)) USizeBox* CardStack3;
	UPROPERTY(meta = (BindWidget)) USizeBox* CardStack4;
	UPROPERTY(meta = (BindWidget)) USizeBox* CardStack5;
	
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

	UPROPERTY(meta = (BindWidget)) UButton* LeaderSkillButton;
	UPROPERTY(meta = (BindWidget)) UImage* LeaderSkillClock;
	UPROPERTY(meta = (BindWidget)) UImage* LeaderManaBar;
	UPROPERTY(meta = (BindWidget)) UTextBlock* LeaderManaText;

	// Confirmation
	UPROPERTY(meta = (BindWidget)) UButton* ConfirmationYesButton;
	UPROPERTY(meta = (BindWidget)) UButton* ConfirmationNoButton;
	UPROPERTY(meta = (BindWidget)) URichTextBlock* ConfirmationText;

	std::string confirmationString;
	std::shared_ptr<FNetworkCommand> onConfirmationYesCommand = nullptr;
	//int32 _onConfirmationNumUITask = 0; // Counts the UI Tasks so that Confirmation will still appear for RareCardUI


	
	// Logs
	UPROPERTY(meta = (BindWidget)) UVerticalBox* EventBox;

	
private:
	// Confirmation
	UFUNCTION() void OnClickConfirmationYes() {
		PUN_CHECK(onConfirmationYesCommand != nullptr);
		networkInterface()->SendNetworkCommand(onConfirmationYesCommand);
		ConfirmationOverlay->SetVisibility(ESlateVisibility::Collapsed);

		confirmationString = "";
		onConfirmationYesCommand = nullptr;
	}
	UFUNCTION() void OnClickConfirmationNo() {
		ConfirmationOverlay->SetVisibility(ESlateVisibility::Collapsed);

		confirmationString = "";
		onConfirmationYesCommand = nullptr;
	}

	
	int32 MoneyLeftAfterTentativeBuy();
	
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
	//UFUNCTION() void ClickConverterCardHandSubmitButton();
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
};
