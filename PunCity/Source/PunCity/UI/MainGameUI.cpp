// Fill out your copyright notice in the Description page of Project Settings.

#include "MainGameUI.h"
#include "ToolTipWidgetBase.h"
#include "../Simulation/GameSimulationCore.h"
#include "../Simulation/PolicySystem.h"
#include "../Simulation/UnlockSystem.h"
#include "../Simulation/Resource/ResourceSystem.h"
#include "../Simulation/OverlaySystem.h"
#include "../Simulation/TreeSystem.h"
#include "../Simulation/UnitSystem.h"
#include "../Simulation/PlayerOwnedManager.h"
#include "../Simulation/StatSystem.h"

#include "Materials/MaterialInstanceDynamic.h"

#include <sstream>
#include "PunCity/Simulation/Buildings/GathererHut.h"
#include "Kismet/GameplayStatics.h"
#include "PunRichText.h"
#include "JobPriorityRow.h"
#include "GraphDataSource.h"

#define LOCTEXT_NAMESPACE "MainGameUI"

using namespace std;

static int32 kTickCount = 0; // local tick count. Can't use Time::Tick() since it may be paused

void UMainGameUI::PunInit()
{
	kTickCount = 0;

	if (!ensure(BuildMenuOverlay)) return;
	if (!ensure(BuildMenuTogglerButton)) return;
	if (!ensure(BuildingMenuWrap)) return;

	// Build Menu
	BuildMenuTogglerButton->OnClicked.AddDynamic(this, &UMainGameUI::ToggleBuildingMenu);

	// Gather Menu
	GatherButton->OnClicked.AddDynamic(this, &UMainGameUI::ToggleGatherButton);
	GatherSettingsCloseButton->OnClicked.AddDynamic(this, &UMainGameUI::CloseGatherUI);

	DemolishButton->OnClicked.AddDynamic(this, &UMainGameUI::ToggleDemolishButton);

	//StatsButton->OnClicked.AddDynamic(this, &UMainGameUI::ToggleStatisticsUI);
	//AddToolTip(StatsButton, "Show statistics");
	StatsButtonOverlay->SetVisibility(ESlateVisibility::Collapsed);

	CardRerollButton->OnClicked.AddDynamic(this, &UMainGameUI::ClickRerollButton);
	CardRerollButton1->OnClicked.AddDynamic(this, &UMainGameUI::ClickRerollButton);
	
	ResetBottomMenuDisplay();

	// Card hand 1
	CardHand1Overlay->SetVisibility(ESlateVisibility::Collapsed);

	CardStackButton->IsFocusable = false;
	CardStackButton->OnClicked.AddDynamic(this, &UMainGameUI::ClickCardStackButton);

	CardHand1SubmitButton->OnClicked.AddDynamic(this, &UMainGameUI::ClickCardHand1SubmitButton);
	CardHand1CancelButton->OnClicked.AddDynamic(this, &UMainGameUI::ClickCardHand1CancelButton);
	CardHand1CloseButton->OnClicked.AddDynamic(this, &UMainGameUI::ClickCardHand1CancelButton);

	{
		AddToolTip(CardHand1SubmitButton, 
			LOCTEXT("CardHand1SubmitButton_Tip", "Submit Card Selection to confirm purchase.<space>Submitting without selected Card will pass the current Card Hand for the next Card Hand."
		));
	}

	CardHand2Box->ClearChildren();

	RareCardHandOverlay->SetVisibility(ESlateVisibility::Collapsed);
	RareCardHandSubmitButton->OnClicked.AddDynamic(this, &UMainGameUI::ClickRareCardHandSubmitButton);

	ConverterCardHandOverlay->SetVisibility(ESlateVisibility::Collapsed);
	//ConverterCardHandSubmitButton->OnClicked.AddDynamic(this, &UMainGameUI::ClickConverterCardHandSubmitButton);
	ConverterCardHandCancelButton->OnClicked.AddDynamic(this, &UMainGameUI::ClickConverterCardHandCancelButton);

	ConverterCardHandConfirmUI->ConfirmYesButton->OnClicked.AddDynamic(this, &UMainGameUI::ClickCardRemovalConfirmYesButton);
	ConverterCardHandConfirmUI->ConfirmNoButton->OnClicked.AddDynamic(this, &UMainGameUI::ClickCardRemovalConfirmNoButton);
	ConverterCardHandConfirmUI->SetVisibility(ESlateVisibility::Collapsed);

	ConfirmationOverlay->SetVisibility(ESlateVisibility::Collapsed);
	ConfirmationYesButton->OnClicked.AddDynamic(this, &UMainGameUI::OnClickConfirmationYes);
	ConfirmationNoButton->OnClicked.AddDynamic(this, &UMainGameUI::OnClickConfirmationNo);

	BuildingMenuWrap->ClearChildren();

	//ItemSelectionUI->SetVisibility(ESlateVisibility::Collapsed);
	//GlobalItemsHorizontalBox->ClearChildren();

	ResearchBarUI->OnClicked.AddDynamic(this, &UMainGameUI::ToggleResearchMenu);
	AddToolTip(ResearchBarUI, 
		LOCTEXT("ResearchBarUI_Tip", "Bring up Technology UI.\n<Orange>[T]</>")
	);

	ProsperityBarUI->OnClicked.AddDynamic(this, &UMainGameUI::ToggleProsperityUI);
	AddToolTip(ProsperityBarUI, 
		LOCTEXT("ProsperityBarUI_Tip", "Bring up House Upgrade Unlocks UI.")
	);

	GatherSettingsOverlay->SetVisibility(ESlateVisibility::Collapsed);

	HarvestCheckBox_All->OnCheckStateChanged.AddDynamic(this, &UMainGameUI::OnCheckHarvest_All);
	HarvestCheckBox_Wood->OnCheckStateChanged.AddDynamic(this, &UMainGameUI::OnCheckHarvest_Wood);
	HarvestCheckBox_NonFruitTrees->OnCheckStateChanged.AddDynamic(this, &UMainGameUI::OnCheckHarvest_NonFruitTrees);
	HarvestCheckBox_Stone->OnCheckStateChanged.AddDynamic(this, &UMainGameUI::OnCheckHarvest_Stone);
	RemoveHarvestCheckBox_All->OnCheckStateChanged.AddDynamic(this, &UMainGameUI::OnCheckRemoveHarvest_All);
	RemoveHarvestCheckBox_Wood->OnCheckStateChanged.AddDynamic(this, &UMainGameUI::OnCheckRemoveHarvest_Wood);
	RemoveHarvestCheckBox_Stone->OnCheckStateChanged.AddDynamic(this, &UMainGameUI::OnCheckRemoveHarvest_Stone);

	MidScreenMessage->SetVisibility(ESlateVisibility::Collapsed);

	//WorldMapRegionUI->SetVisibility(ESlateVisibility::Collapsed);

	// Tooltips 
	AddToolTip(BuildMenuTogglerButton, LOCTEXT("BuildMenuTogglerButton_Tip", "Build houses, farms, and infrastructures\n<Orange>[B]</>"));
	AddToolTip(GatherButton, LOCTEXT("GatherButton_Tip", "Gather trees, stone etc\n<Orange>[G]</><space>Activate this button, then Click and Drag to Gather."));
	AddToolTip(DemolishButton, LOCTEXT("DemolishButton_Tip", "Demolish\n<Orange>[X]</>"));

	AddToolTip(CardStackButton, LOCTEXT("CardStackButton_Tip", "Show drawn cards that can be bought.\n<Orange>[C]</>"));
	AddToolTip(RoundCountdownImage, LOCTEXT("RoundCountdownImage_Tip", "Round timer<space>You get a new card hand each round.<space>Each season contains 2 rounds."));

	BUTTON_ON_CLICK(LeftUITownSwapArrowLeftButton, this, &UMainGameUI::OnClickLeftUITownSwapArrowLeftButton);
	BUTTON_ON_CLICK(LeftUITownSwapArrowRightButton, this, &UMainGameUI::OnClickLeftUITownSwapArrowRightButton);
	const FText townSwapHotkeyTipText = LOCTEXT("TownSwapHotkey_Tip", "Press <Orange>[Ctrl-Q]</> and <Orange>[Ctrl-E]</> to switch between towns.");
	AddToolTip(LeftUITownSwapArrowLeftButton, townSwapHotkeyTipText);
	AddToolTip(LeftUITownSwapArrowRightButton, townSwapHotkeyTipText);

	BUTTON_ON_CLICK(TownSwapArrowLeftButton, this, &UMainGameUI::OnClickJobTownSwapArrowLeftButton);
	BUTTON_ON_CLICK(TownSwapArrowRightButton, this, &UMainGameUI::OnClickJobTownSwapArrowRightButton);

	// Set Icon images
	auto assetLoader = dataSource()->assetLoader();
	
	Money->SetImage(assetLoader->CoinIcon);
	Money->SetTextColorCoin();

	Influence->SetImage(assetLoader->InfluenceIcon);
	Influence->SetTextColorCulture();
	
	Science->SetImage(assetLoader->ScienceIcon);
	Science->SetTextColorScience();

	LeaderSkillButton->OnClicked.AddDynamic(this, &UMainGameUI::OnClickLeaderSkillButton);
	LeaderSkillButton->bOverride_Cursor = false;

	/*
	 * Job Priority
	 */
	JobPriorityCloseButton->OnClicked.AddDynamic(this, &UMainGameUI::OnClickJobPriorityCloseButton);
	JobPriorityCloseButton2->OnClicked.AddDynamic(this, &UMainGameUI::OnClickJobPriorityCloseButton);
	JobPriorityOverlay->SetVisibility(ESlateVisibility::Collapsed);
	_laborerPriorityState.lastPriorityInputTime = -999.0f;

	BUTTON_ON_CLICK(LaborerNonPriorityButton, this, &UMainGameUI::OnClickLaborerNonPriorityButton);
	BUTTON_ON_CLICK(LaborerPriorityButton, this, &UMainGameUI::OnClickLaborerPriorityButton);
	BUTTON_ON_CLICK(LaborerArrowUp, this, &UMainGameUI::IncreaseLaborers);
	BUTTON_ON_CLICK(LaborerArrowDown, this, &UMainGameUI::DecreaseLaborers);

	BUTTON_ON_CLICK(BuilderNonPriorityButton, this, &UMainGameUI::OnClickBuilderNonPriorityButton);
	BUTTON_ON_CLICK(BuilderPriorityButton, this, &UMainGameUI::OnClickBuilderPriorityButton);
	BUTTON_ON_CLICK(BuilderArrowUp, this, &UMainGameUI::IncreaseBuilders);
	BUTTON_ON_CLICK(BuilderArrowDown, this, &UMainGameUI::DecreaseBuilders);

	

	

	// ImportantResources
	auto SetupImportantResources = [&](UIconTextPairWidget* iconTextPair)
	{
		SetChildHUD(iconTextPair);
		iconTextPair->GetAnimations();
	};
	//SetupImportantResources(WoodCount);

	// Create Main resource list
	FuelList->ClearChildren();
	MedicineList->ClearChildren();
	ToolsList->ClearChildren();
	
	MainResourceList->ClearChildren();
	MainFoodList->ClearChildren();
	LuxuryTier1List->ClearChildren();
	LuxuryTier2List->ClearChildren();
	LuxuryTier3List->ClearChildren();
	
	for (int i = 0; i < ResourceEnumCount; i++) 
	{
		ResourceEnum resourceEnum = static_cast<ResourceEnum>(i);
		auto iconTextPair = AddWidget<UIconTextPairWidget>(UIEnum::IconTextPair);
		iconTextPair->SetImage(resourceEnum, assetLoader, true);

		if (IsFuelEnum(resourceEnum)) {
			FuelList->AddChild(iconTextPair);
			SetupImportantResources(iconTextPair);
		}
		else if (IsMedicineEnum(resourceEnum)) {
			MedicineList->AddChild(iconTextPair);
			SetupImportantResources(iconTextPair);
		}
		else if (IsToolsEnum(resourceEnum)) {
			ToolsList->AddChild(iconTextPair);
			SetupImportantResources(iconTextPair);
		}
		else if (IsFoodEnum(resourceEnum)) {
			MainFoodList->AddChild(iconTextPair);
		}
		else if (IsLuxuryEnum(resourceEnum, 1)) {
			LuxuryTier1List->AddChild(iconTextPair);
		}
		else if (IsLuxuryEnum(resourceEnum, 2)) {
			LuxuryTier2List->AddChild(iconTextPair);
		}
		else if (IsLuxuryEnum(resourceEnum, 3)) {
			LuxuryTier3List->AddChild(iconTextPair);
		}
		else {
			MainResourceList->AddChild(iconTextPair);
		}
		iconTextPair->SetVisibility(ESlateVisibility::Collapsed);
		iconTextPair->ObjectId = i;
	}

	ExclamationIcon_Build->SetVisibility(ESlateVisibility::Collapsed);
	ExclamationIcon_Gather->SetVisibility(ESlateVisibility::Collapsed);
	ExclamationIcon_CardStack->SetVisibility(ESlateVisibility::Collapsed);
	
	// Animations
	GetAnimations(Animations);
	check(Animations.Num() > 0);
	//PUN_LOG("Animations %d", Animations.Num());
	//for (auto& pair : Animations) {
	//	PUN_LOG("Animation %s", *pair.Key);
	//	PlayAnimation(pair.Value);
	//	break;
	//}

	//

	//TestMainMenuOverlay1->SetVisibility(ESlateVisibility::Collapsed);
	//TestMainMenuOverlay2->SetVisibility(ESlateVisibility::Collapsed);


	DefaultFont = ResearchingText->Font;
}

void UMainGameUI::Tick()
{
#if !UI_MAINGAME
	return;
#endif
	if (!PunSettings::IsOn("UIMainGame")) {
		return;
	}


	// TODO: Debug kPointerOnUI
	if (PunSettings::IsOn("ShowDebugExtra"))
	{
		std::stringstream ss;
		ss << "kPointerOnUI: " << kPointerOnUI << "\n";

		for (int32 i = 0; i < UPunWidget::kPointerOnUINames.Num(); i++) {
			ss << ToStdString(UPunWidget::kPointerOnUINames[i]);
			ss << "\n";
		}
		
		SetText(QuickDebugText, ss.str());
		QuickDebugBox->SetVisibility(ESlateVisibility::Visible);
	}
	else {
		QuickDebugBox->SetVisibility(ESlateVisibility::Collapsed);
	}

	
	
	kTickCount++;

	if (InterfacesInvalid()) return;

	
	GameSimulationCore& sim = dataSource()->simulation();



	//! Set visibility
	bool shouldDisplayMainGameUI = dataSource()->ZoomDistanceBelow(WorldZoomAmountStage3) &&
									sim.HasChosenLocation(playerId()) &&
									!inputSystemInterface()->isSystemMovingCamera();

	if (shouldDisplayMainGameUI && GetVisibility() == ESlateVisibility::Collapsed) {
		SetVisibility(ESlateVisibility::SelfHitTestInvisible);
	}
	else if (!shouldDisplayMainGameUI && GetVisibility() != ESlateVisibility::Collapsed) {
		SetVisibility(ESlateVisibility::Collapsed);
	}


	/*
	 * Initial hide before building a townhall
	 */
	if (sim.HasTownhall(playerId()))
	{
		BuildGatherMenu->SetVisibility(ESlateVisibility::SelfHitTestInvisible);
		CardStackSizeBox->SetVisibility(ESlateVisibility::SelfHitTestInvisible);
		RoundCountdownOverlay->SetVisibility(ESlateVisibility::SelfHitTestInvisible);
		LeaderSkillOverlay->SetVisibility(ESlateVisibility::SelfHitTestInvisible);
		ResourceOverlay->SetVisibility(ESlateVisibility::SelfHitTestInvisible);
	}
	else
	{
		BuildGatherMenu->SetVisibility(ESlateVisibility::Hidden);
		CardStackSizeBox->SetVisibility(ESlateVisibility::Hidden);
		RoundCountdownOverlay->SetVisibility(ESlateVisibility::Hidden);
		LeaderSkillOverlay->SetVisibility(ESlateVisibility::Hidden);
		ResourceOverlay->SetVisibility(ESlateVisibility::Hidden);
	}
	

	////! If clicked something else, remove confirmation UI
	//if (ConfirmationOverlay->GetVisibility() != ESlateVisibility::Collapsed)
	//{
	//	// Started other task, close the Confirmation overlay
	//	int32 numUITasks = GetPunHUD()->NumberOfUITasks();
	//	if (numUITasks > _onConfirmationNumUITask) {
	//		OnClickConfirmationNo();
	//	} else if (numUITasks < _onConfirmationNumUITask) {
	//		// Rarecard UI was closed
	//		_onConfirmationNumUITask = numUITasks;
	//	}
	//}
	
	{
		// Cards
		auto& cardSystem = sim.cardSystem(playerId());
		int32 rollCountdown = Time::SecondsPerRound - Time::Seconds() % Time::SecondsPerRound;
		
		if (_lastRoundCountdown != rollCountdown) {
			_lastRoundCountdown = rollCountdown;
			if (_lastRoundCountdown <= 5) {
				PlayAnimation(Animations["CountdownFlash"]);

				PlayAnimation(Animations["CardHand1Flash"]);

				dataSource()->Spawn2DSound("UI", "RoundCountdown" + to_string(_lastRoundCountdown));
			}

			// At exact 0
			if (Time::Seconds() > 10 && _lastRoundCountdown == Time::SecondsPerRound) {
				PlayAnimation(Animations["CountdownFlash"]);

				dataSource()->Spawn2DSound("UI", "RoundCountdown0");
			}
			
		}

		
		SetText(RoundCountdownText, to_string(rollCountdown) + "s");
		RoundCountdownImage->GetDynamicMaterial()->SetScalarParameterValue("Fraction", static_cast<float>(Time::Seconds() % Time::SecondsPerRound) / Time::SecondsPerRound);

		ResourceSystem& resourceSystem = sim.resourceSystem(playerId());
		GlobalResourceSystem& globalResourceSys = sim.globalResourceSystem(playerId());

		// Refresh card stack
		// refresh the stack if its state changed...
		// Clicking "Submit" closes the UI temporarily. While the command is being verified, we do not update this
		int32 queueCount = cardSystem.cardHandQueueCount();
		
		if (!cardSystem.IsPendingCommand() && 
			(_lastIsCardStackBlank == BoolEnum::NeedUpdate || 
			 _lastIsCardStackBlank != static_cast<BoolEnum>(cardSystem.IsCardStackBlank()) ||
			 _lastQueueCount != queueCount)
		   )
		{
			_lastQueueCount = queueCount;
			
			_lastIsCardStackBlank = static_cast<BoolEnum>(cardSystem.IsCardStackBlank());
			ESlateVisibility cardStackVisible = static_cast<bool>(_lastIsCardStackBlank) ? ESlateVisibility::Collapsed : ESlateVisibility::SelfHitTestInvisible;

			CardStack1->SetVisibility(cardStackVisible);
			CardStack2->SetVisibility(cardStackVisible);
			CardStack3->SetVisibility(cardStackVisible);
			CardStack4->SetVisibility(cardStackVisible);
			CardStack5->SetVisibility(cardStackVisible);
			CardRerollBox1->SetVisibility(static_cast<bool>(_lastIsCardStackBlank) ? ESlateVisibility::Visible : ESlateVisibility::Collapsed);

			TryPlayAnimation("CardStackFlash");
		}

		// Card Hand Queue Count
		FText submitStr = LOCTEXT("Submit", "Submit");
		if (queueCount > 1) {
			SetText(CardHandCount, to_string(queueCount));
			CardHandCount->SetVisibility(ESlateVisibility::SelfHitTestInvisible);
			CardRerollButton->SetVisibility(ESlateVisibility::Collapsed); // Also hide reroll button when there is card queue to prevent confusion

			// "Pass" instead of "Submit" when there is no card selected, and there are 2+ card hand queue
			int32 reservedCount = CppUtils::Sum(_lastHand1ReserveStatus);
			if (reservedCount == 0) {
				submitStr = LOCTEXT("Pass", "Pass");
			}
		}
		else {
			CardHandCount->SetVisibility(ESlateVisibility::Collapsed);
			CardRerollButton->SetVisibility(ESlateVisibility::Visible);
		}
		SetText(CardHand1SubmitButtonText, submitStr);

		// Reroll price
		int32 rerollPrice = cardSystem.GetRerollPrice();
		RerollPrice->SetColorAndOpacity(globalResourceSys.money() >= rerollPrice ? FLinearColor::White : FLinearColor(0.4, 0.4, 0.4));
		if (rerollPrice == 0) {
			RerollPrice->SetText(LOCTEXT("Free", "Free"));
			RerollPrice1->SetText(LOCTEXT("Free", "Free"));
			RerollCoinIcon->SetVisibility(ESlateVisibility::Collapsed);
			RerollCoinIcon1->SetVisibility(ESlateVisibility::Collapsed);
		} else {
			RerollPrice->SetText(TEXT_NUM(rerollPrice));
			RerollPrice1->SetText(TEXT_NUM(rerollPrice));
			RerollCoinIcon->SetVisibility(ESlateVisibility::HitTestInvisible);
			RerollCoinIcon1->SetVisibility(ESlateVisibility::HitTestInvisible);
		}


		// Money
		//bool moneyChanged = false;
		int32 currentMoneyLeft = MoneyLeftAfterTentativeBuy();
		//if (_lastMoney != currentMoneyLeft) {
		//	_lastMoney = currentMoneyLeft;
		//	//moneyChanged = true;
		//}

		if (currentMoneyLeft < 0) {
			// Unreserve if there isn't enough money
			cardSystem.UnreserveIfRanOutOfCash(currentMoneyLeft);
		}

		// Recalculate need resource
		std::vector<bool> hand1NeedMoney;
		{
			std::vector<CardEnum> hand = cardSystem.GetHand();
			std::vector<bool> handReservation = cardSystem.GetHand1ReserveStatus();
			
			for (size_t i = 0; i < hand.size(); i++) {
				bool needResource = currentMoneyLeft < cardSystem.GetCardPrice(hand[i]);
				if (handReservation[i]) {
					needResource = false; // reserved card doesn't need resource 
				}
				hand1NeedMoney.push_back(needResource);
			}
		}

		/*
		 * Refresh hand 1
		 */
		auto refreshHand1 = [&]()
		{
			CardHand1Box->ClearChildren();

			std::vector<CardEnum> hand = cardSystem.GetHand();
			
			for (size_t i = 0; i < hand.size(); i++)
			{
				CardEnum buildingEnum = hand[i];
				auto cardButton = AddCard(CardHandEnum::DrawHand, buildingEnum, CardHand1Box, CallbackEnum::SelectUnboughtCard, i);


				// Price
				cardButton->SetPrice(cardSystem.GetCardPrice(buildingEnum));
			}

			// Highlight the selected card
			// Gray out cards that can't be bought
			TArray<UWidget*> cardButtons = CardHand1Box->GetAllChildren();
			for (int i = 0; i < cardButtons.Num(); i++) {
				auto cardButton = CastChecked<UBuildingPlacementButton>(cardButtons[i]);
				cardButton->SetCardStatus(CardHandEnum::DrawHand, _lastHand1ReserveStatus[i], _lastHand1NeedMoneyStatus[i]);
			}
		};

		//bool handChanged = _lastDisplayHand != cardSystem.GetHand();
		bool questChanged = false;
		if (_lastQuests != sim.questSystem(playerId())->quests()) {
			_lastQuests = sim.questSystem(playerId())->quests();
			questChanged = true;
		}

		if (cardSystem.needHand1Refresh || questChanged)
		{
			//_lastDisplayHand = cardSystem.GetHand();
			_lastHand1ReserveStatus = cardSystem.GetHand1ReserveStatus();
			_lastHand1NeedMoneyStatus = hand1NeedMoney;
			refreshHand1();

			TryPlayAnimation("CardStackFlash");
			cardSystem.needHand1Refresh = false;

			// Show hand
			if (cardSystem.justRerollButtonPressed) {
				cardSystem.justRerollButtonPressed = false;

				networkInterface()->ResetGameUI();
				CardHand1Overlay->SetVisibility(ESlateVisibility::Visible);
				
				//if (GetPunHUD()->IsDoingUITask()) {
				//	_needDrawHandDisplay = true; // Delayed display
				//} else {
				//	CardHand1Overlay->SetVisibility(ESlateVisibility::Visible);
				//}

				// TODO: proper card deal animation+sound
				dataSource()->Spawn2DSound("UI", "CardDeal");

				TryStopAnimation("CardStackFlash");
			}
		}


		
		//else if (_lastHand1ReserveStatus != cardSystem.GetHand1ReserveStatus()) {
		//	_lastHand1ReserveStatus = cardSystem.GetHand1ReserveStatus();
		//	_lastHand1NeedMoneyStatus = hand1NeedMoney;
		//	refreshHand1();
		//}
		// Note that we need to update both _lastHand1ReserveStatus and _lastHand1ReserveStatus to handle the situation where a save was just loaded.
		else if (_lastHand1ReserveStatus != cardSystem.GetHand1ReserveStatus() ||
				_lastHand1NeedMoneyStatus != hand1NeedMoney) 
		{
			_lastHand1ReserveStatus = cardSystem.GetHand1ReserveStatus();
			_lastHand1NeedMoneyStatus = hand1NeedMoney;
			refreshHand1();
		}

		/*
		 * Refresh rare hand
		 */
		auto refreshRareHand = [&]()
		{
			RareCardHandBox->ClearChildren();

			std::vector<CardEnum> rareHand = cardSystem.GetRareHand();

			// For Loaded game, rareHand might be 0
			if (rareHand.size() == 0) {
				return false;
			}
			
			for (size_t i = 0; i < rareHand.size(); i++) {
				CardEnum buildingEnum = rareHand[i];
				AddCard(CardHandEnum::RareHand, buildingEnum, RareCardHandBox, CallbackEnum::SelectUnboughtRareCardPrize, i);
			}

			// Highlight the selected card
			// Gray out cards that can't be bought
			TArray<UWidget*> cardButtons = RareCardHandBox->GetAllChildren();
			for (int i = 0; i < cardButtons.Num(); i++) {
				auto cardButton = CastChecked<UBuildingPlacementButton>(cardButtons[i]);
				cardButton->SetCardStatus(CardHandEnum::RareHand, _lastRareHandReserveStatus[i], false, true);
			}

			// Set RareCardHand message
			SetText(RareCardHandText, cardSystem.rareHandMessage());
			SetText(RareCardHandText2, cardSystem.rareHandMessage2());

			return true;
		};

		// Refresh RareHand if needed
		if (cardSystem.justRerolledRareHand) {
			cardSystem.justRerolledRareHand = false;

			_lastRareHandReserveStatus = cardSystem.GetRareHandReserveStatus();
			bool succeed = refreshRareHand();

			if (succeed) {
				GetPunHUD()->QueueStickyExclusiveUI(ExclusiveUIEnum::RareCardHand);
			}
		}
		else if (_lastRareHandReserveStatus != cardSystem.GetRareHandReserveStatus())
		{
			_lastRareHandReserveStatus = cardSystem.GetRareHandReserveStatus();
			refreshRareHand();
		}
		

		/*
		 * Refresh converter hand
		 *
		 * !!! CONFUSING CARD HANDS
		 * We do not need special variable like _needDrawHandDisplay because we will never need a delayed hand display for this
		 */
		if (cardSystem.converterCardState == ConverterCardUseState::JustUsed)
		{
			FString searchString = ConverterCardHandOverlay->IsVisible() ? ConverterCardHandSearchBox->GetText().ToString() : "";
			
			// Open if not already done so
			if (!ConverterCardHandOverlay->IsVisible() ||
				lastSearchString != searchString)
			{
				lastSearchString = searchString;
				
				ConverterCardHandBox->ClearChildren();
				ConverterCardHandConfirmUI->SetVisibility(ESlateVisibility::Collapsed);
				
				std::vector<CardEnum> availableCards = cardSystem.GetAllPiles();
				unordered_set<CardEnum> uniqueAvailableCards(availableCards.begin(), availableCards.end());

				// Loop through BuildingEnumCount so that cards are arranged by BuildingEnumINt
				CardEnum wildCardEnum = cardSystem.wildCardEnumUsed;

				if (wildCardEnum == CardEnum::CardRemoval)
				{
					std::vector<CardEnum> unremovedCards = cardSystem.GetUnremovedPiles();
					unordered_set<CardEnum> uniqueUnremovedCards(unremovedCards.begin(), unremovedCards.end());
					
					for (size_t i = 0; i < BuildingEnumCount; i++)
					{
						CardEnum buildingEnum = static_cast<CardEnum>(i);

						if (uniqueUnremovedCards.find(buildingEnum) != uniqueUnremovedCards.end())
						{
							// Can only convert to building
							if (IsBuildingCard(buildingEnum)) 
							{
								// Only Cards filtered by Search Box
								if (searchString.IsEmpty() ||
									GetBuildingInfo(buildingEnum).nameF().Find(searchString, ESearchCase::Type::IgnoreCase, ESearchDir::FromStart) != INDEX_NONE)
								{
									auto cardButton = AddCard(CardHandEnum::ConverterHand, buildingEnum, ConverterCardHandBox, CallbackEnum::SelectCardRemoval, i);

									SetText(ConverterCardHandTitle, LOCTEXT("CHOOSE A CARD TO REMOVE", "CHOOSE A CARD TO REMOVE"));
									SetText(cardButton->PriceText, TEXT_NUM(cardSystem.GetCardPrice(buildingEnum)));
									cardButton->PriceTextBox->SetVisibility(ESlateVisibility::HitTestInvisible);
								}
							}
						}
					}
				}
				else
				{
					
					
					for (int32 i = 0; i < SortedNameBuildingEnum.size(); i++)
					{
						CardEnum buildingEnum = SortedNameBuildingEnum[i];
						
						// Show wild card by type
						if (wildCardEnum == CardEnum::WildCardFood && !IsAgricultureBuilding(buildingEnum)) {
							continue;
						}
						if (wildCardEnum == CardEnum::WildCardIndustry && !IsIndustrialBuilding(buildingEnum)) {
							continue;
						}
						if (wildCardEnum == CardEnum::WildCardMine && !IsMountainMine(buildingEnum)) {
							continue;
						}
						if (wildCardEnum == CardEnum::WildCardService && !IsServiceBuilding(buildingEnum)) {
							continue;
						}

						if (uniqueAvailableCards.find(buildingEnum) != uniqueAvailableCards.end())
						{
							// Can only convert to building
							if (IsBuildingCard(buildingEnum)) 
							{
								// Only Cards filtered by Search Box
								if (searchString.IsEmpty() ||
									GetBuildingInfo(buildingEnum).nameF().Find(searchString, ESearchCase::Type::IgnoreCase, ESearchDir::FromStart) != INDEX_NONE)
								{
									auto cardButton = AddCard(CardHandEnum::ConverterHand, buildingEnum, ConverterCardHandBox, CallbackEnum::SelectConverterCard, i);

									SetText(ConverterCardHandTitle, LOCTEXT("ConverterCardHandTitle", "CHOOSE A CARD\npay the price to build"));
									SetText(cardButton->PriceText, TEXT_NUM(cardSystem.GetCardPrice(buildingEnum)));
									cardButton->PriceTextBox->SetVisibility(ESlateVisibility::HitTestInvisible);
								}
							}
						}
					}
				}

				networkInterface()->ResetGameUI();
				ConverterCardHandOverlay->SetVisibility(ESlateVisibility::Visible);
				_lastConverterChosenCard = CardEnum::None; // Start off with nothing chosen

				// TODO: proper card deal animation+sound
				dataSource()->Spawn2DSound("UI", "CardDeal");
			}

			// Highlight the selected card
			TArray<UWidget*> cardButtons = ConverterCardHandBox->GetAllChildren();
			for (int i = 0; i < cardButtons.Num(); i++) {
				auto cardButton = CastChecked<UBuildingPlacementButton>(cardButtons[i]);
				bool shouldHighlight = cardButton->buildingEnum == _lastConverterChosenCard;
				cardButton->SetCardStatus(CardHandEnum::ConverterHand, shouldHighlight, false, false, false);
			}
		}
		else {
			ConverterCardHandSearchBox->SetText(FText());
		}

		/*
		 * Refresh hand 2
		 */
		 // Card being placed, taking into account network delay
		CardEnum cardEnumBeingPlaced = inputSystemInterface()->GetBuildingEnumBeingPlaced();
		
		if (_lastDisplayBought != cardSystem.GetCardsBought() ||
			_lastCardEnumBeingPlaced != cardEnumBeingPlaced ||
			_lastConverterCardState != cardSystem.converterCardState)
		{
			_lastDisplayBought = cardSystem.GetCardsBought();
			_lastCardEnumBeingPlaced = cardEnumBeingPlaced;
			_lastConverterCardState = cardSystem.converterCardState;

			// Remove index being placed or used
			std::vector<BuildingCardStack> actualDisplayBought;
			for (size_t i = 0; i < _lastDisplayBought.size(); i++)
			{
				BuildingCardStack currentStack = _lastDisplayBought[i];
				if (currentStack.buildingEnum == cardEnumBeingPlaced) {
					currentStack.stackSize--;
				}
				else if (_lastConverterCardState != ConverterCardUseState::None && currentStack.buildingEnum == cardSystem.wildCardEnumUsed)
				{
					currentStack.stackSize--;
				}
				
				if (currentStack.stackSize > 0) {
					actualDisplayBought.push_back(currentStack);
				}
			}

			// Bought hand
			CardHand2Box->ClearChildren();
			for (size_t i = 0; i < actualDisplayBought.size(); i++)
			{
				BuildingCardStack stack = actualDisplayBought[i];

				int32_t stackSize = stack.stackSize;// +simulation.buildingIds(playerId(), stack.buildingEnum).size();

				// Show stars??
				auto cardButton = AddCard(CardHandEnum::BoughtHand, stack.buildingEnum, CardHand2Box, CallbackEnum::SelectBoughtCard, i, 0, stackSize, true);
			}

			// Warning to bought cards that need resources
			TArray<UWidget*> cardBoughtButtons = CardHand2Box->GetAllChildren();
			for (int i = 0; i < cardBoughtButtons.Num(); i++) {
				auto cardButton = CastChecked<UBuildingPlacementButton>(cardBoughtButtons[i]);

				if (IsBuildingCard(cardButton->buildingEnum)) 
				{
					bool needResource = false;
					const std::vector<int32> resourceNeeded = GetBuildingInfo(cardButton->buildingEnum).constructionResources;
					
					for (size_t j = 0; j < _countof(ConstructionResources); j++) {
						if (resourceNeeded[j] > resourceSystem.resourceCount(ConstructionResources[j])) {
							needResource = true;
							break;
						}
					}

					cardButton->SetBoughtCardNeedResource(needResource);
				}
			}

		}
	}

	// No Townhall Don't show UI other than Townhall Card
	if (!sim.HasTownhall(playerId())) {
		ResearchBarUI->SetVisibility(ESlateVisibility::Collapsed);
		ProsperityBarUI->SetVisibility(ESlateVisibility::Collapsed);
		return;
	}

	{
		// Events
		auto& eventSystem = sim.eventLogSystem();

		if (eventSystem.needRefreshEventLog[playerId()])
		{
			EventBox->ClearChildren();
			const std::vector<EventLog>& events = eventSystem.events(playerId());

			PUN_LOG("Event log refresh  ------- _playerId:%d", playerId());
			
			for (int32 i = 0; i < events.size(); i++) 
			{
				auto widget = AddWidget<UPunRichText>(UIEnum::PunRichText);

				FText richMessage = TEXT_TAG_IGNORE_OTHER_TAGS("<EventLog>", events[i].messageT());
				if (events[i].isImportant) {
					richMessage = TEXT_TAG_IGNORE_OTHER_TAGS("<EventLogRed>", events[i].messageT());
				}

				
				
				widget->SetText(richMessage);
				EventBox->AddChild(widget);

				PUN_LOG(" --- widget: %s", *events[i].message);
			}
			
			eventSystem.needRefreshEventLog[playerId()] = false;
		}
	}
	

	{
		/*
		 * Exclamation
		 */
		// Gather
		auto questSys = sim.questSystem(playerId());
		PlacementType placementType = inputSystemInterface()->placementState();
		ExclamationIcon_Gather->SetShow(questSys->GetQuest(QuestEnum::GatherMarkQuest) 
											&& placementType != PlacementType::Gather 
											&& placementType != PlacementType::GatherRemove);

		// Card stack
		{
			bool needExclamation = false;
			
			if (CardStack1->GetVisibility() != ESlateVisibility::Collapsed &&
				questSys->GetQuest(QuestEnum::FoodBuildingQuest)) 
			{
				needExclamation = true;
				
				auto cardsBought = sim.cardSystem(playerId()).GetCardsBought();
				for (const auto& cardStack : cardsBought) {
					if (IsAgricultureBuilding(cardStack.buildingEnum)) {
						needExclamation = false; // Food card alreay bought, don't need too buy more...
						break;
					}
				}
				
			}

			ExclamationIcon_CardStack->SetShow(needExclamation);
		}

		{
			bool isBuilding = BuildMenuOverlay->GetVisibility() != ESlateVisibility::Collapsed || 
								placementType == PlacementType::Building;
			
			bool needExclamation = false;
			if (!isBuilding)
			{
				if (questSys->GetQuest(QuestEnum::BuildStorageQuest) ||
					sim.NeedQuestExclamation(playerId(), QuestEnum::BuildHousesQuest)) {
					needExclamation = true;
				}
			}
			ExclamationIcon_Build->SetShow(needExclamation);
		}
	}


	{
		//! Stats:
		UnitSystem& unitSystem = sim.unitSystem();
		StatSystem& statSystem = sim.statSystem();
		GlobalResourceSystem& globalResourceSys = sim.globalResourceSystem(playerId());

		// Top left
		{
			SetText(TimeText, FText::FormatNamed(
				LOCTEXT("InGameTopLeft_SeasonYear", "{EarlyMidLate} {SeasonName}\nYear {Years}"),
				TEXT("EarlyMidLate"), Time::SeasonPrefix(Time::Ticks()),
				TEXT("SeasonName"), Time::SeasonName(Time::Seasons()),
				TEXT("Years"), TEXT_NUM(Time::Years())
			));
		}
		
		{
			FloatDet celsius = sim.Celsius(dataSource()->cameraAtom().worldTile2());
			SetText(TemperatureText, FText::Format(
				INVTEXT("{0}°C ({1}°F)"),
				TEXT_NUM(FDToInt(celsius)),
				TEXT_NUM(FDToInt(CelsiusToFahrenheit(celsius)))
			));

			float fraction = FDToFloat(celsius - Time::MinCelsiusBase()) / FDToFloat(Time::MaxCelsiusBase() - Time::MinCelsiusBase());
			TemperatureImage->GetDynamicMaterial()->SetScalarParameterValue("Fraction", fraction);

			AddToolTip(TemperatureTextBox, FText::Format(
				LOCTEXT("TemperatureTextBox_Tip", "Below {0}°C, citizens will need wood/coal to heat themselves"),
				TEXT_NUM(FDToInt(Time::ColdCelsius()))
			));

			TemperatureTextBox->SetVisibility(ESlateVisibility::Visible);
		}

		stringstream ss;
		//TopLeftText->SetText(FText::FromString(FString(ss.str().c_str())));
		//ss("");

		int32 population = sim.populationTown(currentTownId());
		int32 childPopulation = sim.townManager(currentTownId()).childPopulation();
		int32 adultPopulation = population - childPopulation;

		AdultPopulationText->SetText(FText::FromString(FString::FromInt(adultPopulation)));
		ChildPopulationText->SetText(FText::FromString(FString::FromInt(childPopulation)));
		{
			AddToolTip(PopulationBox, FText::Format(
				LOCTEXT("PopulationBox_Tip", "Population: {0}<bullet>{1} Adults</><bullet>{2} Children</>"),
				TEXT_NUM(population),
				TEXT_NUM(adultPopulation),
				TEXT_NUM(childPopulation)
			));
		}

		if (shouldDisplayMainGameUI)
		{
			int32 housingCapacity = sim.HousingCapacity(currentTownId());
			HousingSpaceText->SetText(FText::FromString(FString::FromInt(population) + FString("/") + FString::FromInt(housingCapacity)));


			std::pair<int32, int32> slotsPair = sim.GetStorageCapacity(currentTownId());
			int32 usedSlots = slotsPair.first;
			int32 totalSlots = slotsPair.second;
			SetText(StorageSpaceText, to_string(usedSlots) + "/" + to_string(totalSlots));

			{
				TArray<FText> args;
				ADDTEXT_(
					LOCTEXT("HousingSpaceBox_Tip1", "Population: {0}\nHousing space: {1}"),
					TEXT_NUM(population),
					TEXT_NUM(housingCapacity)
				);
				ADDTEXT_INV_("<space>");
				for (int32 i = 1; i <= House::GetMaxHouseLvl(); i++) {
					int32 houseLvlCount = sim.GetHouseLvlCount(currentTownId(), i, false);
					if (houseLvlCount > 0) {
						ADDTEXT_(LOCTEXT("HousingSpaceBox_Tip2", "<bullet>House lvl {0}: {1}</>"), TEXT_NUM(i), TEXT_NUM(houseLvlCount));
					}
				}
				
				AddToolTip(HousingSpaceBox, JOINTEXT(args));
			}
			{
				TArray<FText> args;

				AddToolTip(StorageSpaceBox, FText::Format(LOCTEXT("StorageSpaceBox_Tip",
					"Storage space:\n"
					"Used slots: {0}\n"
					"Total slots: {1}"
					),
					TEXT_NUM(usedSlots),
					TEXT_NUM(totalSlots)
				));
			}
			
			//auto& townhall = simulation.townhall(playerId());
			//MigrationText->SetText(ToFText("Migration " + to_string(townhall.migrationPull())));
			
			//std::stringstream migrationTip;
			//migrationTip << "City's attractiveness in the eyes of immigrants.\n";
			//migrationTip << " Total: " << townhall.migrationPull() << "\n";
			//migrationTip << "  population size: " << townhall.migrationPull_populationSize << "\n";

			//if (townhall.migrationPull_freeLivingSpace >= 0) {
			//	migrationTip << "  free space: " << townhall.migrationPull_freeLivingSpace << "\n";
			//} else {
			//	migrationTip << "  homeless: " << townhall.migrationPull_freeLivingSpace << "\n";
			//}
			//
			//migrationTip << "  happiness: " << townhall.migrationPull_happiness << "\n";
			//migrationTip << "  bonuses: " << townhall.migrationPull_bonuses;
			//AddToolTip(MigrationText, migrationTip.str());
		}

		auto& playerOwned = sim.playerOwned(playerId());
		auto& townManager = sim.townManager(currentTownId());
		
		// Happiness (Town)
		{
			int32 overallHappiness = townManager.aveOverallHappiness();
			Happiness->SetImage(assetLoader()->GetHappinessFace(overallHappiness));
			Happiness->SetText(FText(), TEXT_NUM(overallHappiness));
			Happiness->SetTextColor(GetHappinessColor(overallHappiness));
			
			TArray<FText> args;

			ADDTEXT_(LOCTEXT("Happiness_Tip1", "Happiness: {0}%\n"), TEXT_NUM(overallHappiness));
			for (size_t i = 0; i < HappinessEnumCount; i++) {
				int32 aveHappiness = townManager.aveHappinessByType(static_cast<HappinessEnum>(i));
				ADDTEXT_(INVTEXT("  {0} {1}\n"), 
					ColorHappinessText(aveHappiness, FText::Format(INVTEXT("{0}%"), TEXT_NUM(aveHappiness))),
					HappinessEnumName[i]
				);
			}

			AddToolTip(Happiness, JOINTEXT(args));
		}

		// Money (Player)
		{
			Money->SetText(FText(), TEXT_NUM(globalResourceSys.money()));

			int32 totalIncome100 = playerOwned.totalIncome100();
			MoneyChangeText->SetText(TEXT_100SIGNED(totalIncome100));

			TArray<FText> args;
			ADDTEXT_LOCTEXT("Money_TipTitle", "Coins (Money) is used for purchasing buildings and goods. Coins come from tax and trade.\n\n");
			playerOwned.AddTaxIncomeToString(args);

			FText moneyTipText = JOINTEXT(args);
			AddToolTip(Money, moneyTipText);
			AddToolTip(MoneyChangeText, moneyTipText);
		}

		// Influence (Player)
		if (sim.HasChosenLocation(playerId()) &&
			sim.unlockedInfluence(playerId()))
		{
			Influence->SetText("", to_string(globalResourceSys.influence()));
			InfluenceChangeText->SetText(TEXT_100SIGNED(playerOwned.totalInfluenceIncome100()));

			TArray<FText> args;
			ADDTEXT_LOCTEXT("Influence_Tip1", "Influence points used for claiming land and vassalizing other towns.\n\n");
			playerOwned.AddInfluenceIncomeToString(args);

			FText tipText = JOINTEXT(args);
			AddToolTip(Influence, tipText);
			AddToolTip(InfluenceChangeText, tipText);

			Influence->SetVisibility(ESlateVisibility::Visible);
			InfluenceChangeText->SetVisibility(ESlateVisibility::Visible);
		} else {
			Influence->SetVisibility(ESlateVisibility::Collapsed);
			InfluenceChangeText->SetVisibility(ESlateVisibility::Collapsed);
		}

		// Science (Player)
		if (sim.unlockSystem(playerId())->researchEnabled) 
		{
			// Science Text
			TArray<FText> args;
			ADDTEXT_(INVTEXT("+{0}"), TEXT_100(playerOwned.science100PerRound()));
			Science->SetText(FText(), JOINTEXT(args));

			// Science Tip
			args.Empty();
			ADDTEXT_(LOCTEXT("ScienceTip", "Science is used for researching new technology.\n Science per round: {0} <img id=\"Science\"/>\n"), TEXT_100(playerOwned.science100PerRound()));

			for (size_t i = 0; i < ScienceEnumCount; i++)
			{
				int32 science100 = 0;
				const auto& townIds = playerOwned.townIds();
				for (int32 townId : townIds) {
					science100 += sim.townManager(townId).sciences100[i];
				}
				if (science100 != 0) {
					ADDTEXT_(INVTEXT("  {0} {1}\n"), TEXT_100(science100), ScienceEnumName(i));
				}
			}
			
			AddToolTip(Science, args);

			Science->SetVisibility(ESlateVisibility::Visible);
		} else {
			Science->SetVisibility(ESlateVisibility::Collapsed);
		}

		/*
		 * Town Swap Texts
		 */
		{
			//int32 townId = currentTownId();
			//if (townId != -1) {
			//	LeftUITownName->SetText(sim.townNameT(townId));
			//	LeftUITownSwapHorizontalBox->SetVisibility(ESlateVisibility::SelfHitTestInvisible);
			//} else {
			//	LeftUITownSwapHorizontalBox->SetVisibility(ESlateVisibility::Collapsed);
			//}
			PunUIUtils::SetTownSwapText(currentTownId(), &sim, LeftUITownSwapText, LeftUITownSwapHorizontalBox);
			PunUIUtils::SetTownSwapText(jobPriorityTownId, &sim, TownSwapText, TownSwapHorizontalBox);
			//if (jobPriorityTownId != -1) {
			//	int32 townPlayerId = sim.townPlayerId(jobPriorityTownId);
			//	if (sim.GetTownIds(townPlayerId).size() > 1) {
			//		TownSwapText->SetText(sim.townNameT(jobPriorityTownId));
			//		TownSwapHorizontalBox->SetVisibility(ESlateVisibility::SelfHitTestInvisible);
			//	}
			//	else {
			//		TownSwapHorizontalBox->SetVisibility(ESlateVisibility::Collapsed);
			//	}
			//} else {
			//	TownSwapHorizontalBox->SetVisibility(ESlateVisibility::Collapsed);
			//}
		}

		/*
		 * 
		 */
		BUTTON_ON_CLICK(LeftUIPopulationButton, this, &UMainGameUI::OnClickPopulationButton);
		BUTTON_ON_CLICK(LeftUIMoneyButton, this, &UMainGameUI::OnClickMoneyButton);
		//BUTTON_ON_CLICK(LeftUIInfluenceButton, this, &UMainGameUI::OnClickFoodCountButton);
		BUTTON_ON_CLICK(LeftUIScienceButton, this, &UMainGameUI::OnClickScienceButton);

		// Skill
		{
			BldInfo cardInfo = GetBuildingInfo(playerOwned.currentSkill());
			int32 skillMana = GetSkillManaCost(cardInfo.cardEnum);
			int32 maxMana = playerOwned.maxSP();
			
			TArray<FText> args;
			ADDTEXT_(INVTEXT("<Bold>{0}</>\n"), cardInfo.name);
			ADDTEXT_TAGN_("<SPColor>", LOCTEXT("Leader Skill", "Leader Skill"));
			ADDTEXT_(INVTEXT("{0}: <Orange>[V]</>"), LOCTEXT("Hotkey", "Hotkey"));
			ADDTEXT_INV_("<line><space>");
			ADDTEXT_(LOCTEXT("SP cost: {0}\n", "SP cost: {0}\n"), skillMana);
			ADDTEXT__(cardInfo.GetDescription())
			ADDTEXT_INV_("<space>");
			ADDTEXT_(INVTEXT("<SPColor>SP: {0}/{1}</>"), TEXT_NUM(playerOwned.GetSP()), TEXT_NUM(maxMana));
			AddToolTip(LeaderSkillButton, args);
			
			LeaderManaBar->GetDynamicMaterial()->SetScalarParameterValue("Fraction", Clamp01(playerOwned.spFloat() / maxMana));
			SetText(LeaderManaText, "SP " + to_string(playerOwned.GetSP()) + "/" + to_string(maxMana));
			LeaderSkillClock->GetDynamicMaterial()->SetScalarParameterValue("Fraction", Clamp01(playerOwned.spFloat() / skillMana));
		}

		// Animals

		//FString animalsText;
		//if (resourceSystem.pigs > 0) animalsText.Append("Pigs (need ranch): ").AppendInt(resourceSystem.pigs);
		//if (resourceSystem.sheep > 0) animalsText.Append("Sheep (need ranch): ").AppendInt(resourceSystem.sheep);
		//if (resourceSystem.cows > 0) animalsText.Append("Cows (need ranch): ").AppendInt(resourceSystem.cows);
		//if (resourceSystem.pandas > 0) animalsText.Append("Pandas (need ranch): ").AppendInt(resourceSystem.pandas);
		//if (resourceSystem.pigs > 0 || resourceSystem.sheep > 0 || resourceSystem.cows > 0 || resourceSystem.pandas > 0) {
		//	AnimalsNeedingRanch->SetText(FText::FromString(animalsText));
		//	AnimalsNeedingRanch->SetVisibility(ESlateVisibility::SelfHitTestInvisible);
		//} else {
			//AnimalsNeedingRanch->SetVisibility(ESlateVisibility::Collapsed);
		//}

		/*
		 * Resources
		 */

		// This will set IconTextPair to red text when amount is zero
		auto SetResourceIconPair = [&](UIconTextPairWidget* iconTextPair, ResourceEnum resourceEnum)
		{
			int32 resourceCount = sim.resourceCountTown(currentTownId(), resourceEnum);
			iconTextPair->SetFString(FString(), FString::FromInt(resourceCount));
			iconTextPair->SetTextColor(resourceCount == 0 ? FLinearColor::Red : FLinearColor::White);

			if (iconTextPair->HasAnimation()) {
				iconTextPair->PlayAnimationIf("Flash", resourceCount == 0);
			}
			
			iconTextPair->SetImage(resourceEnum, assetLoader(), true);
			iconTextPair->InitBackgroundButton(resourceEnum);
		};

		//SetResourceIconPair(WoodCount, ResourceEnum::Wood);


		// Food
		int32 foodCount = sim.foodCount(currentTownId());
		FoodCountText->SetText(FText::Format(LOCTEXT("Food: {0}", "Food: {0}"), TEXT_NUM(foodCount)));
		FoodCountText->SetColorAndOpacity(foodCount > 0 ? FLinearColor::White : FLinearColor::Red);
		PlayAnimationIf("FoodCountLowFlash", foodCount == 0);

		BUTTON_ON_CLICK(FoodCountButton, this, &UMainGameUI::OnClickFoodCountButton);

		{
			auto& statSys = sim.statSystem(currentTownId());
			
			const std::vector<std::vector<int32>>& productionStats = statSys.GetResourceStat(ResourceSeasonStatEnum::Production);
			const std::vector<std::vector<int32>>& consumptionStats = statSys.GetResourceStat(ResourceSeasonStatEnum::Consumption);

			int32 foodProduction = 0;
			int32 foodConsumption = 0;
			for (int i = 0; i < StaticData::FoodEnumCount; i++) {
				foodProduction += CppUtils::Sum(productionStats[static_cast<int>(StaticData::FoodEnums[i])]);
				foodConsumption += CppUtils::Sum(consumptionStats[static_cast<int>(StaticData::FoodEnums[i])]);
			}

			auto tooltip = AddToolTip(FoodCountText, FText::Format(
				LOCTEXT("FoodCountText_Tip", "Food Count: {0}<space>Food Production (yearly): <FaintGreen>{1}</>\nFood Consumption (yearly): <FaintRed>{2}</>"),
				TEXT_NUM(foodCount),
				TEXT_NUM(foodProduction),
				TEXT_NUM(foodConsumption)
			));

			auto punGraph = tooltip->TooltipPunBoxWidget->AddThinGraph();

			// Food Graph
			bool shouldUpdateFuel = false;
			if (bIsHoveredButGraphNotSetup || 
				tooltip->TooltipPunBoxWidget->DidElementResetThisRound()) 
			{
				bIsHoveredButGraphNotSetup = false;
				shouldUpdateFuel = true;

				punGraph->SetupGraph({
					{ LOCTEXT("Production (season)", "Production (season)").ToString(), PlotStatEnum::FoodProduction, FLinearColor(0.3, 1, 0.3) },
					{ LOCTEXT("Consumption (season)", "Consumption (season)").ToString(), PlotStatEnum::FoodConsumption, FLinearColor(1, 0.3, 0.3) },
				});
			}

			//// Fuel Graph
			//int32 fuelProduction = CppUtils::Sum(productionStats[static_cast<int>(ResourceEnum::Wood)]) + CppUtils::Sum(productionStats[static_cast<int>(ResourceEnum::Coal)]);
			//int32 fuelConsumption = CppUtils::Sum(consumptionStats[static_cast<int>(ResourceEnum::Wood)]) + CppUtils::Sum(consumptionStats[static_cast<int>(ResourceEnum::Coal)]);

			//tip << "<space>";
			//tip << "<space>";
			//tip << "Fuel Production (yearly): <FaintGreen>" << fuelProduction << "</>\n";
			//tip << "Fuel Consumption (yearly): <FaintRed>" << fuelConsumption << "</>\n";
			//tip << "Fuel Count: " << simulation.resourceCount(currentTownId(), ResourceEnum::Wood) + simulation.resourceCount(currentTownId(), ResourceEnum::Coal);
			//
			//tooltip->TooltipPunBoxWidget->AddRichTextParsed(tip);

			//auto punThinGraph = tooltip->TooltipPunBoxWidget->AddThinGraph();

			//if (shouldUpdateFuel)
			//{
			//	punGraph->SetupGraph({
			//		{ FString("Fuel"), PlotStatEnum::Fuel, FLinearColor::Yellow },
			//	});
			//}
		}
		
		// Luxury
		AddToolTip(LuxuryTier1Text, LuxuryResourceTip(1));
		AddToolTip(LuxuryTier2Text, LuxuryResourceTip(2));
		AddToolTip(LuxuryTier3Text, LuxuryResourceTip(3));

		for (int i = 0; i < ResourceEnumCount; i++)
		{
			ResourceEnum resourceEnum = static_cast<ResourceEnum>(i);
			int amount = dataSource()->GetResourceCount(currentTownId(), resourceEnum);

			UVerticalBox* listToAdd = MainResourceList;
			if (IsFuelEnum(resourceEnum)) {
				listToAdd = FuelList;
			}
			else if (IsMedicineEnum(resourceEnum)) {
				listToAdd = MedicineList;
			}
			else if (IsToolsEnum(resourceEnum)) {
				listToAdd = ToolsList;
			}
			else if (IsFoodEnum(resourceEnum)) {
				listToAdd = MainFoodList;
			}
			else if (IsLuxuryEnum(resourceEnum, 1)) {
				listToAdd = LuxuryTier1List;
			}
			else if (IsLuxuryEnum(resourceEnum, 2)) {
				listToAdd = LuxuryTier2List;
			}
			else if (IsLuxuryEnum(resourceEnum, 3)) {
				listToAdd = LuxuryTier3List;
			}

			// Go through the list to change the correct iconTextPair
			for (int j = 0; j < listToAdd->GetChildrenCount(); j++) 
			{
				auto iconTextPair = CastChecked<UIconTextPairWidget>(listToAdd->GetChildAt(j));
				if (iconTextPair->ObjectId == i) 
				{
					// Display normally = display only those above 0
					auto displayNormally = [&]() {
						if (amount > 0) {
							iconTextPair->SetVisibility(ESlateVisibility::SelfHitTestInvisible);
							SetResourceIconPair(iconTextPair, resourceEnum);
						}
						else {
							iconTextPair->SetVisibility(ESlateVisibility::Collapsed);
						}
					};
					
					//if (IsFuelEnum(resourceEnum))
					//{
					//	int32 fuelCount = dataSource()->GetResourceCount(currentTownId(), FuelEnums);
					//	if (fuelCount > 0) {
					//		displayNormally();
					//	}
					//	else {
					//		// Without fuel, we show flashing 0 wood
					//		if (resourceEnum == ResourceEnum::Wood) {
					//			iconTextPair->SetVisibility(ESlateVisibility::SelfHitTestInvisible);
					//			SetResourceIconPair(iconTextPair, resourceEnum);
					//		}
					//		else {
					//			iconTextPair->SetVisibility(ESlateVisibility::Collapsed);
					//		}
					//	}
					//}
					
					if (resourceEnum == ResourceEnum::Wood)
					{
						iconTextPair->SetVisibility(ESlateVisibility::SelfHitTestInvisible);
						SetResourceIconPair(iconTextPair, resourceEnum);
					}
					else if (IsMedicineEnum(resourceEnum)) 
					{
						int32 medicineCount = dataSource()->GetResourceCount(currentTownId(), MedicineEnums);
						if (medicineCount > 0) {
							displayNormally();
						} else {
							// Without medicine, we show flashing medicine
							if (resourceEnum == ResourceEnum::Medicine) {
								iconTextPair->SetVisibility(ESlateVisibility::SelfHitTestInvisible);
								SetResourceIconPair(iconTextPair, resourceEnum);
							} else {
								iconTextPair->SetVisibility(ESlateVisibility::Collapsed);
							}
						}
					}
					else if (IsToolsEnum(resourceEnum)) 
					{
						int32 toolsCount = dataSource()->GetResourceCount(currentTownId(), ToolsEnums);
						if (toolsCount > 0) {
							displayNormally();
						} else {
							// Without tools, we show flashing steel tools
							if (resourceEnum == ResourceEnum::SteelTools) {
								iconTextPair->SetVisibility(ESlateVisibility::SelfHitTestInvisible);
								SetResourceIconPair(iconTextPair, resourceEnum);
							} else {
								iconTextPair->SetVisibility(ESlateVisibility::Collapsed);
							}
						}
					}
					// Special case stone.. Always show
					else if (resourceEnum == ResourceEnum::Stone) 
					{
						iconTextPair->SetVisibility(ESlateVisibility::SelfHitTestInvisible);
						SetResourceIconPair(iconTextPair, resourceEnum);
					}
					else {
						displayNormally();
					}
					break;
				}
			}
		}

		/*
		 * Research
		 */
		UnlockSystem* unlockSys = dataSource()->simulation().unlockSystem(playerId());
		if (unlockSys && 
			unlockSys->researchEnabled)
		{
			std::shared_ptr<ResearchInfo> currentTech = unlockSys->currentResearch();

			ResearchingText->SetText(currentTech->GetName());
			
			TArray<FText> args;
			unlockSys->SetDisplaySciencePoint(args, false);
			SetText(ResearchingAmountText, args);
			
			ResearchBar->SetWidthOverride(unlockSys->hasTargetResearch() ? (unlockSys->researchFraction() * 240) : 0);
			ResearchBarUI->SetVisibility(ESlateVisibility::Visible);
		}
		else {
			ResearchBarUI->SetVisibility(ESlateVisibility::Collapsed);
		}
	}

	UnlockSystem* unlockSystem = sim.unlockSystem(playerId());
	if (unlockSystem) 
	{
		// ResearchBarUI
		// Flash ResearchBarUI if there is nothing being researched
		FLinearColor researchBarDark = FLinearColor(.025f, .025f, .025f, 0.8f);
		FLinearColor researchBarColor = researchBarDark;
		if (unlockSystem->shouldFlashTechToggler()) {
			if (kTickCount % 60 < 30) {
				researchBarColor = FLinearColor(1.0f, 0.33333f, 0.0f, 0.8f);
			}
		}
		
		ResearchBarUI->SetBackgroundColor(researchBarColor);
		//PUN_LOG("ResearchBarUI %f, %f, %f", researchBarColor.R, researchBarColor.G, researchBarColor.B);

		if (unlockSystem->allTechsUnlocked()) {
			ResearchBarUI->SetVisibility(ESlateVisibility::Collapsed);
		}


		// ProsperityBarUI Tick
		if (unlockSystem->prosperityEnabled) 
		{
			ProsperityBarUI->SetBackgroundColor(researchBarDark);
			ProsperityBarUI->SetVisibility(ESlateVisibility::Visible);

			// Find the tech that is closest to done and display that...
			//  Note: This system allow for flexibility in tweaking unlockCount to get the desired techs to come first
			int32 closestTech_HouseNeeded = 99999;
			int32 closestTech_HouseLvl = -1;
			int32 closestTech_CurrentHouseCount = 0;
			
			const std::vector<std::vector<int32>>& houseLvlToUnlockCount = unlockSystem->houseLvlToUnlockCounts();
			for (size_t lvl = 1; lvl < houseLvlToUnlockCount.size(); lvl++) 
			{
				int32 houseLvlCount = sim.GetHouseLvlCount(playerId(), lvl, true);
				for (size_t i = 0; i < houseLvlToUnlockCount[lvl].size(); i++) 
				{
					// skip i == 0 for the tech with zero left?
					if (i == 0 && lvl > 1)
					{
						//int32 houseLvlCountToLeft = simulation.GetHouseLvlCount(playerId(), lvl - 1, true);
						//// If the left tech is unfinished, skip this
						//if (houseLvlCountToLeft < houseLvlToUnlockCount[lvl - 1][0]) {
						//	continue;
						//}
						int32 houseLvlCountToLeft = sim.GetHouseLvlCount(playerId(), lvl - 1, true);
						if (houseLvlCountToLeft == 0) {
							continue;
						}
					}

					// skip showing the part of the column
					//if (simulation.GetHouseLvlCount(playerId(), lvl, true) == 0) {
					//	continue;
					//}
					
					int32 houseNeeded = houseLvlToUnlockCount[lvl][i] - houseLvlCount;
					if (houseNeeded > 0 && 
						houseNeeded < closestTech_HouseNeeded) 
					{
						closestTech_HouseNeeded = houseNeeded;
						closestTech_HouseLvl = lvl;
						closestTech_CurrentHouseCount = houseLvlCount;
					}
				}
			}

			if (closestTech_HouseLvl != -1) 
			{
				int32 totalHouseCount = closestTech_CurrentHouseCount + closestTech_HouseNeeded;

				SetText(ProsperityAmountText, FText::Format(
					LOCTEXT("ProsperityAmountText", "House Lvl {0}: {1}/{2}"),
					TEXT_NUM(closestTech_HouseLvl),
					TEXT_NUM(closestTech_CurrentHouseCount),
					TEXT_NUM(totalHouseCount)
				));
				ProsperityBar->SetWidthOverride(closestTech_CurrentHouseCount * 240.0f / totalHouseCount);
			}
			else {
				ProsperityAmountText->SetVisibility(ESlateVisibility::Collapsed);
			}
		} else {
			ProsperityBarUI->SetVisibility(ESlateVisibility::Collapsed);
		}
	}

	// Midscreen message
	{
		PlacementInfo placementInfo = inputSystemInterface()->PlacementBuildingInfo();

		auto setMidscreenText = [&](FText text) {
			MidScreenMessage->SetVisibility(ESlateVisibility::Visible);
			SetText(MidScreenMessageText, text);
		};

		if (IsRoadPlacement(placementInfo.placementType)) 
		{
			setMidscreenText(LOCTEXT("Shift-click to repeat", "Shift-click to repeat"));
		}
		else {
			MidScreenMessage->SetVisibility(ESlateVisibility::Collapsed);
		}
	}

	/*
	 * Job Priority Tick
	 */
	if (jobPriorityTownId != -1)
	{
		auto& townManager = sim.townManager(jobPriorityTownId);

		// lazy init
		if (JobPriorityRows.Num() == 0)
		{
			const std::vector<CardEnum>& jobPriorityList = townManager.GetJobPriorityList();

			JobPriorityScrollBox->ClearChildren();
			for (int32 i = 0; i < jobPriorityList.size(); i++)
			{
				CardEnum jobEnum = jobPriorityList[i];

				UJobPriorityRow* jobRow = AddWidget<UJobPriorityRow>(UIEnum::JobPriorityRow);
				jobRow->Init(this, jobEnum);
				JobPriorityScrollBox->AddChild(jobRow);
				JobPriorityRows.Add(jobRow);
				jobRow->Rename(*(FString("Job_") + FString::FromInt(static_cast<int>(jobEnum))));
			}
		}

		if (JobPriorityOverlay->GetVisibility() != ESlateVisibility::Collapsed)
		{
			CardEnum firstCardEnum = CardEnum::None;
			CardEnum lastCardEnum = CardEnum::None;
			for (int32 i = 0; i < JobPriorityRows.Num(); i++) {
				CardEnum cardEnum = JobPriorityRows[i]->cardEnum;;
				if (townManager.jobBuildingEnumToIds()[static_cast<int>(cardEnum)].size() > 0) {
					if (firstCardEnum == CardEnum::None) {
						firstCardEnum = cardEnum;
					}
					lastCardEnum = cardEnum;
				}
			}

			int32 visibleIndex = 0;
			for (int32 i = 0; i < JobPriorityRows.Num(); i++)
			{
				auto jobRow = JobPriorityRows[i];
				CardEnum cardEnum = jobRow->cardEnum;

				const std::vector<int32>& buildingIds = townManager.jobBuildingEnumToIds()[static_cast<int>(cardEnum)];
				if (buildingIds.size() == 0) {
					jobRow->SetVisibility(ESlateVisibility::Collapsed);
					continue;
				}

				int32 occupantCount = 0;
				int32 allowedCount = 0;
				for (int32 buildingId : buildingIds)
				{
					auto& building = sim.building(buildingId);
					occupantCount += building.occupantCount();
					allowedCount += building.allowedOccupants();
				}

				//std::stringstream ss;
				//ss << occupantCount << "/" << allowedCount;
				SetText(jobRow->JobCountText,
					FText::Format(INVTEXT("{0}/{1}"), TEXT_NUM(occupantCount), TEXT_NUM(allowedCount))
				);

				jobRow->SetVisibility(ESlateVisibility::Visible);

				bool hideArrowUp = (cardEnum == firstCardEnum);
				jobRow->ArrowUpButton->SetVisibility(hideArrowUp ? ESlateVisibility::Hidden : ESlateVisibility::Visible);
				jobRow->ArrowFastUpButton->SetVisibility(hideArrowUp ? ESlateVisibility::Hidden : ESlateVisibility::Visible);

				bool hideArrowDown = (cardEnum == lastCardEnum);
				jobRow->ArrowDownButton->SetVisibility(hideArrowDown ? ESlateVisibility::Hidden : ESlateVisibility::Visible);
				jobRow->ArrowFastDownButton->SetVisibility(hideArrowDown ? ESlateVisibility::Hidden : ESlateVisibility::Visible);

				jobRow->index = i;
				jobRow->visibleIndex = visibleIndex;
				visibleIndex++;
			}

			_laborerPriorityState.TrySyncToSimulation(&sim, jobPriorityTownId, this);
			RefreshLaborerUI();
		}
	}

	/*
	 * Card animation, the beginning part
	 */
	//DescriptionUIState uiState = simulation.descriptionUIState();
	//
	//for (int32 i = AnimatedCardOverlay->GetChildrenCount(); i-- > 0;)
	//{
	//	// TODO: so much simillar
	//	auto cardButton = CastChecked<UBuildingPlacementButton>(AnimatedCardOverlay->GetChildAt(i));
	//	if (cardButton->animationEnum == CardAnimationEnum::ToGlobalSlot)
	//	{
	//		if (uiState.objectType == ObjectTypeEnum::Building &&
	//			simulation.building(uiState.objectId).isEnum(CardEnum::Townhall))
	//		{
	//			std::vector<CardStatus> cardStatuses = simulation.cardSystem(playerId()).cardsInTownhall();
	//			// Remove if the card arrived at the building already
	//			for (CardStatus& cardStatus : cardStatuses) {
	//				if ((cardStatus.animationStartTime100 / 100.0f) == cardButton->cardAnimationStartTime) {
	//					cardButton->RemoveFromParent();
	//				}
	//			}
	//		}
	//		else {
	//			// No longer displaying townhall ObjectDescriptionUI, close this
	//			cardButton->RemoveFromParent();
	//		}
	//	}
	//	else if (cardButton->animationEnum == CardAnimationEnum::ToBuildingSlot)
	//	{
	//		if (uiState.objectType == ObjectTypeEnum::Building)
	//		{
	//			std::vector<CardStatus> cardStatuses = simulation.building(uiState.objectId).slotCards();
	//			
	//			// Remove if the card arrived at the building already
	//			for (CardStatus& cardStatus : cardStatuses) {
	//				if ((cardStatus.animationStartTime100 / 100.0f) == cardButton->cardAnimationStartTime) {
	//					cardButton->RemoveFromParent();
	//				}
	//			}
	//		}
	//		else {
	//			// No longer displaying building DescriptionUI, close this
	//			cardButton->RemoveFromParent();
	//		}
	//	}
	//}
}

//UBuildingPlacementButton* UMainGameUI::AddAnimationCard(CardEnum buildingEnum)
//{
//	if (InterfacesInvalid()) return nullptr;
//
//	UBuildingPlacementButton* cardButton = AddWidget<UBuildingPlacementButton>(UIEnum::CardMini);
//
//	cardButton->PunInit(buildingEnum, -1, -1, -1, this, CallbackEnum::None);
//	SetChildHUD(cardButton);
//
//	cardButton->SetupMaterials(dataSource()->assetLoader());
//	cardButton->SetCardStatus(false, false, IsRareCard(buildingEnum));
//
//	return cardButton;
//}
UBuildingPlacementButton* UMainGameUI::AddCard(CardHandEnum cardHandEnum, CardEnum buildingEnum, UWrapBox* buttonParent, CallbackEnum callbackEnum, int32 cardHandIndex,
																	int32 buildingLvl, int32 stackSize, bool isMiniature)
{
	if (InterfacesInvalid()) return nullptr;

	UBuildingPlacementButton* cardButton = AddWidget<UBuildingPlacementButton>(isMiniature ? UIEnum::CardMini : UIEnum::BuildingPlacementButton);
	
	buttonParent->AddChild(cardButton);

	cardButton->PunInit(buildingEnum, cardHandIndex, buildingLvl, stackSize, this, callbackEnum);

	SetChildHUD(cardButton);

	cardButton->SetupMaterials(dataSource()->assetLoader());
	cardButton->SetCardStatus(cardHandEnum, false, false, IsRareCard(buildingEnum));

	return cardButton;
}

void UMainGameUI::ResetBottomMenuDisplay()
{
	SetButtonImage(BuildMenuTogglerImage, false);
	SetButtonImage(GatherImage, false);
	SetButtonImage(DemolishImage, false);
	SetButtonImage(StatsImage, false);

	if (shouldCloseGatherSettingsOverlay) {
		GatherSettingsOverlay->SetVisibility(ESlateVisibility::Collapsed);
	}
	shouldCloseGatherSettingsOverlay = true;
	
	BuildMenuOverlay->SetVisibility(ESlateVisibility::Collapsed);
}

void UMainGameUI::ToggleBuildingMenu()
{
	if (InterfacesInvalid()) return;

	bool wasActive = BuildMenuTogglerImage->GetVisibility() != ESlateVisibility::Collapsed;
	ResetBottomMenuDisplay();

	if (!wasActive) {
		// Refresh building buttons
		std::vector<CardEnum> buildingEnums = simulation().unlockSystem(playerId())->unlockedBuildings();

		auto cardSys = simulation().cardSystem(playerId());
		int32 money = simulation().money(playerId());

		auto refreshPermanentCard = [&](UBuildingPlacementButton* cardButton)
		{
			int32 cardPrice = cardSys.GetCardPrice(cardButton->buildingEnum);
			cardButton->SetCardStatus(CardHandEnum::PermanentHand, false, cardPrice > 0 && money < cardPrice);
			cardButton->SetPrice(cardPrice);
		};

		// erase buildingEnums that already have buttons
		for (int i = BuildingMenuWrap->GetChildrenCount(); i --> 0;) 
		{
			auto cardButton = Cast<UBuildingPlacementButton>(BuildingMenuWrap->GetChildAt(i));
			auto found = find(buildingEnums.begin(), buildingEnums.end(), cardButton->buildingEnum);
			if (found != buildingEnums.end()) { // found old card, reusing it...
				buildingEnums.erase(found);

				// Make sure to update old card (for exclamation)
				cardButton->SetCardStatus(CardHandEnum::PermanentHand, false, false);
				refreshPermanentCard(cardButton);
			}
		}

		// Create the rest of the buttons
		for (CardEnum buildingEnum : buildingEnums) {
			auto cardButton = AddCard(CardHandEnum::PermanentHand, buildingEnum, BuildingMenuWrap, CallbackEnum::SelectPermanentCard, -1, -1, -1, true);
			refreshPermanentCard(cardButton);
		}

		// Close other UIs
		networkInterface()->ResetGameUI();

		dataSource()->Spawn2DSound("UI", "UIWindowOpen");
	}
	else {
		dataSource()->Spawn2DSound("UI", "UIWindowClose");
	}

	SetButtonImage(BuildMenuTogglerImage, !wasActive);
	BuildMenuOverlay->SetVisibility(wasActive ? ESlateVisibility::Collapsed : ESlateVisibility::SelfHitTestInvisible);
}

void UMainGameUI::ToggleGatherButton()
{
	if (InterfacesInvalid()) return;

	PUN_LOG("ToggleGatherButton");

	bool wasActive = GatherImage->IsVisible();
	ResetBottomMenuDisplay();

	inputSystemInterface()->CancelPlacement();
	
	if (!wasActive) {
		HarvestCheckBox_All->SetCheckedState(ECheckBoxState::Checked);
		HarvestCheckBox_Wood->SetCheckedState(ECheckBoxState::Unchecked);
		HarvestCheckBox_NonFruitTrees->SetCheckedState(ECheckBoxState::Unchecked);
		HarvestCheckBox_Stone->SetCheckedState(ECheckBoxState::Unchecked);
		RemoveHarvestCheckBox_All->SetCheckedState(ECheckBoxState::Unchecked);
		RemoveHarvestCheckBox_Wood->SetCheckedState(ECheckBoxState::Unchecked);
		RemoveHarvestCheckBox_Stone->SetCheckedState(ECheckBoxState::Unchecked);

		inputSystemInterface()->StartHarvestPlacement(false, ResourceEnum::None);
		GetPunHUD()->CloseDescriptionUI();

		dataSource()->Spawn2DSound("UI", "ButtonClick"); //TODO: need button click start/end
	} else {
		dataSource()->Spawn2DSound("UI", "CancelPlacement");
	}

	SetButtonImage(GatherImage, !wasActive);
	GatherSettingsOverlay->SetVisibility(wasActive ? ESlateVisibility::Collapsed : ESlateVisibility::SelfHitTestInvisible);
}
void UMainGameUI::CloseGatherUI()
{
	ResetBottomMenuDisplay();
	inputSystemInterface()->CancelPlacement();
	dataSource()->Spawn2DSound("UI", "CancelPlacement");
	
	SetButtonImage(GatherImage, false);
	GatherSettingsOverlay->SetVisibility(ESlateVisibility::Collapsed);
}

void UMainGameUI::ToggleDemolishButton()
{
	PUN_LOG("ToggleDemolishButton");
	if (InterfacesInvalid()) return;

	bool wasActive = DemolishImage->IsVisible();
	ResetBottomMenuDisplay();

	inputSystemInterface()->CancelPlacement();
	
	if (!wasActive) {
		inputSystemInterface()->StartDemolish();
		GetPunHUD()->CloseDescriptionUI();

		dataSource()->Spawn2DSound("UI", "ButtonClick"); //TODO: need button click start/end
	} else {
		dataSource()->Spawn2DSound("UI", "CancelPlacement");
	}

	
	SetButtonImage(DemolishImage, !wasActive);
}

void UMainGameUI::ToggleResearchMenu()
{
	UnlockSystem* unlockSystem = dataSource()->simulation().unlockSystem(playerId());
	if (unlockSystem && unlockSystem->researchEnabled) {
		GetPunHUD()->ToggleTechUI();
	}
}
void UMainGameUI::ToggleProsperityUI()
{
	UnlockSystem* unlockSystem = dataSource()->simulation().unlockSystem(playerId());
	if (unlockSystem && unlockSystem->prosperityEnabled) {
		GetPunHUD()->ToggleProsperityUI();
	}
}


void UMainGameUI::ClickRerollButton()
{
	auto& cardSystem = simulation().cardSystem(playerId());
	if (cardSystem.IsPendingCommand()) {
		return;
	}
	
	int32 money = simulation().money(playerId());
	int32 rerollPrice = cardSystem.GetRerollPrice();
	if (money >= rerollPrice || rerollPrice == 0) {
		auto command = make_shared<FRerollCards>();
		networkInterface()->SendNetworkCommand(command);
		cardSystem.SetPendingCommand(true);
	} else {
		simulation().AddPopupToFront(playerId(), 
			LOCTEXT("Not enough money for reroll",  "Not enough money for reroll"),
			ExclusiveUIEnum::CardHand1, "PopupCannot"
		);
	}
	
}

void UMainGameUI::ClickCardStackButton()
{
	auto& cardSystem = simulation().cardSystem(playerId());
	if (cardSystem.IsPendingCommand()) {
		return;
	}

	if (CardHand1Overlay->GetVisibility() == ESlateVisibility::Collapsed) {
		if (!cardSystem.IsCardStackBlank()) {
			networkInterface()->ResetGameUI();
			CardHand1Overlay->SetVisibility(ESlateVisibility::Visible);

			// TODO: proper card deal animation+sound
			dataSource()->Spawn2DSound("UI", "CardDeal");
			
			TryStopAnimation("CardStackFlash");
		}
	} else {
		ClickCardHand1CancelButton();
		//cardSystem.ClearHand1CardReservation();
		//CardHand1Overlay->SetVisibility(ESlateVisibility::Collapsed);

		//// TODO: proper card deal animation+sound
		//dataSource()->Spawn2DSound("UI", "CardDeal");
	}
	
	//CardHand1Overlay->SetVisibility(cardSystem.IsCardStackBlank() ? ESlateVisibility::Collapsed : ESlateVisibility::Visible);
}

void UMainGameUI::ClickCardHand1SubmitButton()
{
	auto& cardSystem = simulation().cardSystem(playerId());
	if (cardSystem.IsPendingCommand()) {
		return;
	}
	
	// Check if not enough money, and put on a warning...
	if (CardHand1SubmitButtonText->GetText().ToString() == FString("Submit")) {
		if (MoneyLeftAfterTentativeBuy() < 0) {
			simulation().AddPopupToFront(playerId(), 
				LOCTEXT("NotEnoughMoneyToBuyCard_Pop", "Not enough money to purchase the card."), 
				ExclusiveUIEnum::CardHand1, "PopupCannot"
			);
			return;
		}
	}

	// TODO:
	//if (!simulation().cardSystem(playerId()).CanAddCardToTopRow(buildingEnum)) {
	//	simulation().AddPopup(playerId(), "Reached hand limit for bought cards.", ExclusiveUIEnum::CardHand1);
	//	return;
	//}

	// Issue command and close the UI
	TArray<UWidget*> cardButtons = CardHand1Box->GetAllChildren();
	auto command = make_shared<FBuyCard>();
	for (int i = 0; i < cardButtons.Num(); i++) {
		auto cardButton = CastChecked<UBuildingPlacementButton>(cardButtons[i]);
		if (_lastHand1ReserveStatus[i]) {
			command->cardHandBuyIndices.Add(cardButton->cardHandIndex);
		}
	}
	
	networkInterface()->SendNetworkCommand(command);
	cardSystem.SetPendingCommand(true);

	CardHand1Overlay->SetVisibility(ESlateVisibility::Collapsed);

	CardStack1->SetVisibility(ESlateVisibility::Collapsed);
	CardStack2->SetVisibility(ESlateVisibility::Collapsed);
	CardStack3->SetVisibility(ESlateVisibility::Collapsed);
	CardStack4->SetVisibility(ESlateVisibility::Collapsed);
	CardStack5->SetVisibility(ESlateVisibility::Collapsed);
	CardRerollBox1->SetVisibility(ESlateVisibility::Visible);

	// TODO: proper card deal animation+sound
	dataSource()->Spawn2DSound("UI", "CardDeal");
}
void UMainGameUI::ClickCardHand1CancelButton()
{
	if (CardHand1Overlay->GetVisibility() != ESlateVisibility::Collapsed)
	{
		auto& cardSystem = simulation().cardSystem(playerId());

		cardSystem.ClearHand1CardReservation();
		CardHand1Overlay->SetVisibility(ESlateVisibility::Collapsed);

		// TODO: proper card deal animation+sound
		dataSource()->Spawn2DSound("UI", "CardDeal");
	}
}

void UMainGameUI::ClickRareCardHandSubmitButton()
{
	// Issue command and close the UI
	TArray<UWidget*> cardButtons = RareCardHandBox->GetAllChildren();
	auto command = make_shared<FSelectRareCard>();
	command->cardEnum = CardEnum::None;
	
	for (int i = 0; i < cardButtons.Num(); i++) {
		auto cardButton = CastChecked<UBuildingPlacementButton>(cardButtons[i]);
		if (_lastRareHandReserveStatus[i]) {
			command->cardEnum = cardButton->buildingEnum;
			break;
		}
	}

	if (command->cardEnum == CardEnum::None) {
		simulation().AddPopupToFront(playerId(), 
			LOCTEXT("ChooseCardBeforeSubmit_Pop", "Please choose a card before submitting."), 
			ExclusiveUIEnum::RareCardHand, "PopupCannot"
		);
		return;
	}
	
	networkInterface()->SendNetworkCommand(command);

	RareCardHandOverlay->SetVisibility(ESlateVisibility::Collapsed);

	dataSource()->Spawn2DSound("UI", "CardDeal");
}


void UMainGameUI::ClickConverterCardHandCancelButton()
{
	if (ConverterCardHandOverlay->GetVisibility() != ESlateVisibility::Collapsed)
	{
		ConverterCardHandOverlay->SetVisibility(ESlateVisibility::Collapsed);
		simulation().cardSystem(playerId()).converterCardState = ConverterCardUseState::None;

		dataSource()->Spawn2DSound("UI", "CardDeal");
	}
}

void UMainGameUI::OnClickLeaderSkillButton()
{
	auto& playerOwned = simulation().playerOwned(playerId());
	CardEnum skillEnum = playerOwned.currentSkill();
	if (playerOwned.GetSP() < GetSkillManaCost(skillEnum)) {
		simulation().AddPopupToFront(playerId(), 
			LOCTEXT("NoSPForSkill_Pop", "Not enough SP to use the leader skill."), 
			ExclusiveUIEnum::None, "PopupCannot"
		);
		return;
	}

	inputSystemInterface()->StartBuildingPlacement(skillEnum, 0, false);
}

void UMainGameUI::RightMouseUp()
{
	if (BuildMenuOverlay->GetVisibility() != ESlateVisibility::Collapsed) {
		dataSource()->Spawn2DSound("UI", "UIWindowClose");
	}

	ResetBottomMenuDisplay();
}

void UMainGameUI::EscDown()
{
	if (BuildMenuOverlay->GetVisibility() != ESlateVisibility::Collapsed) {
		dataSource()->Spawn2DSound("UI", "UIWindowClose");
	}
	ResetBottomMenuDisplay();

	// Esc down will also quit any card menu
	ClickCardHand1CancelButton();
	ClickConverterCardHandCancelButton();
}

void UMainGameUI::CallBack1(UPunWidget* punWidgetCaller, CallbackEnum callbackEnum)
{
	UBuildingPlacementButton* cardButton = CastChecked<UBuildingPlacementButton>(punWidgetCaller);
	int32 cardHandIndex = cardButton->cardHandIndex;
	CardEnum buildingEnum = cardButton->buildingEnum;
	auto& cardSystem = simulation().cardSystem(playerId());

	// Permanent cards
	if (callbackEnum == CallbackEnum::SelectPermanentCard)
	{
		int32 money = simulation().money(playerId());
		int32 cardPrice = cardSystem.GetCardPrice(buildingEnum);
		if (cardPrice > 0 && money < cardPrice) {
			simulation().AddPopupToFront(playerId(), 
				LOCTEXT("NoMoneyToBuyCommonCard_Pop", "Not enough money to buy the common card."), 
				ExclusiveUIEnum::BuildMenu, "PopupCannot"
			);
			return;
		}

		if (buildingEnum == CardEnum::IntercityRoad) {
			inputSystemInterface()->StartRoadPlacement(false, true);
		}
		else if (IsRoad(buildingEnum)) {
			inputSystemInterface()->StartRoadPlacement(buildingEnum == CardEnum::StoneRoad);
			//inputSystemInterface()->StartRoadPlacement(buildingEnum == CardEnum::StoneRoad);
		}
		else if (buildingEnum == CardEnum::Fence) {
			inputSystemInterface()->StartFencePlacement();
		}
		else if (buildingEnum == CardEnum::Bridge) {
			inputSystemInterface()->StartBridgePlacement(false);
			simulation().parameters(playerId())->BridgeNoticed = true;
		}
		else if (buildingEnum == CardEnum::IntercityBridge) {
			inputSystemInterface()->StartBridgePlacement(true);
		}
		else if (buildingEnum == CardEnum::Tunnel) {
			inputSystemInterface()->StartTunnelPlacement();
		}
		else {
			inputSystemInterface()->StartBuildingPlacement(buildingEnum, 0, false);

			// Noticed farm, no longer need exclamation on farm after this...
			if (buildingEnum == CardEnum::Farm) {
				simulation().parameters(playerId())->FarmNoticed = true;
			}
		}

		GetPunHUD()->CloseDescriptionUI();

		BuildMenuOverlay->SetVisibility(ESlateVisibility::Collapsed);
		SetButtonImage(BuildMenuTogglerImage, false);
		return;
	}
	
	if (callbackEnum == CallbackEnum::SelectUnboughtCard)
	{
		if (cardSystem.GetHand1ReserveStatus(cardHandIndex)) {
			// Is alreadyreserved for buying, cancel it
			cardSystem.SetHand1CardReservation(cardHandIndex, false);
		}
		else {
			// Check if there is enough money
			int32 money = MoneyLeftAfterTentativeBuy();
			if (cardHandIndex != -1 && money < cardSystem.GetCardPrice(buildingEnum)) {
				simulation().AddPopupToFront(playerId(), 
					LOCTEXT("NoMoneyToBuyCard_Pop", "Not enough money to purchase the card."), 
					ExclusiveUIEnum::CardHand1, "PopupCannot"
				);
				return;
			}

			// Check if we reached hand limit TODO: delete this
			//std::vector<bool> reserveStatuses = cardSystem.GetHand1ReserveStatus();
			//int alreadyReserved = 0;
			//for (bool reserved : reserveStatuses) {
			//	if (reserved) {
			//		alreadyReserved++;
			//	}
			//}
			
			if (!cardSystem.CanAddCardToBoughtHand(buildingEnum, 1, true)) {
				simulation().AddPopupToFront(playerId(), 
					LOCTEXT("ReachedHandLimit_Pop", "Reached hand limit for bought cards."),
					ExclusiveUIEnum::CardHand1, "PopupCannot"
				);
				return;
			}
			
			cardSystem.SetHand1CardReservation(cardHandIndex, true);
		}
		return;
	}

	if (callbackEnum == CallbackEnum::SelectUnboughtRareCardPrize)
	{
		std::vector<bool> cardsRareHandReserved = cardSystem.GetRareHandReserveStatus();
		if (cardsRareHandReserved[cardHandIndex]) {
			// Is alreadyreserved for buying, cancel it
			cardSystem.SetRareHandCardReservation(cardHandIndex, false);
		}
		else {
			for (size_t i = 0; i < cardsRareHandReserved.size(); i++) {
				if (cardsRareHandReserved[i]) {
					cardSystem.SetRareHandCardReservation(i, false);
				}
			}

			// Check if we reached hand limit
			if (!simulation().cardSystem(playerId()).CanAddCardToBoughtHand(buildingEnum, 1)) 
			{
				simulation().AddPopupToFront(playerId(), 
					LOCTEXT("ReachedHandLimitRare_Pop", "Reached hand limit for bought cards.<space>Please sell or use some cards on your hand, then choose a rare card prize again."),
					ExclusiveUIEnum::RareCardHand, "PopupCannot"
				);
				return;
			}

			cardSystem.SetRareHandCardReservation(cardHandIndex, true);
		}
		return;
	}

	if (callbackEnum == CallbackEnum::SelectConverterCard)
	{
		// Check if there is enough money...
		if (simulation().money(playerId()) < cardSystem.GetCardPrice(buildingEnum) / 2) {
			simulation().AddPopupToFront(playerId(), 
				LOCTEXT("NotEnoughMoneyWildCard_Pop", "Not enough money. Need to pay the building price to use wild card."), 
				ExclusiveUIEnum::ConverterCardHand, "PopupCannot"
			);
			return;
		}

		dataSource()->Spawn2DSound("UI", "CardDeal");
		ConverterCardHandOverlay->SetVisibility(ESlateVisibility::Collapsed);
		cardSystem.converterCardState = ConverterCardUseState::SubmittedUI;
		
		inputSystemInterface()->StartBuildingPlacement(buildingEnum, cardButton->buildingLvl, false, cardSystem.wildCardEnumUsed);

		return;
	}
	if (callbackEnum == CallbackEnum::SelectCardRemoval)
	{
		buildingEnumToRemove = buildingEnum;
		SetText(ConverterCardHandConfirmUI->ConfirmText, FText::Format(
			LOCTEXT("SureRemoveCard", "Are you sure you want to remove {0} Card?"),
			GetBuildingInfo(buildingEnum).name
		));
		ConverterCardHandConfirmUI->SetVisibility(ESlateVisibility::Visible);

		return;
	}

	if (callbackEnum == CallbackEnum::SelectBoughtCard)
	{
		// Non-Building Cards
		if (!IsBuildingCard(buildingEnum))
		{
			PUN_LOG("Not Building Card %s", *GetBuildingInfo(buildingEnum).nameF());
			if (IsAreaSpell(buildingEnum)) {
				inputSystemInterface()->StartBuildingPlacement(buildingEnum, cardButton->buildingLvl, true);
			}

			auto& resourceSys = simulation().resourceSystem(playerId());

			/*
			 * Use Global Slot Card
			 */
			if (IsGlobalSlotCard(buildingEnum)) 
			{
				DescriptionUIState uiState = simulation().descriptionUIState();
				if (uiState.objectType == ObjectTypeEnum::Building)
				{
					Building& bld = simulation().building(uiState.objectId);
					if (bld.isEnum(CardEnum::Townhall))
					{
						if (simulation().townManager(bld.townId()).CanAddCardToTownhall())
						{
							FVector2D initialPosition = GetViewportPosition(cardButton->GetCachedGeometry());

							auto command = make_shared<FUseCard>();
							command->townId = bld.subclass<TownHall>().townId();
							command->cardEnum = buildingEnum;
							command->SetPosition(initialPosition);
							networkInterface()->SendNetworkCommand(command);

							// Card initial animation
							//auto animationCard = AddAnimationCard(buildingEnum);
							//animationCard->StartAnimation(initialPosition, transitionPosition, CardAnimationEnum::ToGlobalSlot, Time::TicksPerSecond / 10);
							//AnimatedCardOverlay->AddChild(animationCard);

							//cardButton->SetVisibility(ESlateVisibility::Hidden);
						}
						else {
							simulation().AddPopupToFront(playerId(),
								LOCTEXT("NotEnoughSlotUnslotFirst_Pop", "Not enough slot. Unslot a card in this building first."),
								ExclusiveUIEnum::None, "PopupCannot"
							);
						}
					}
				}
				else
				{
					simulation().AddPopupToFront(playerId(), 
						LOCTEXT("GlobalCardNeedsTownhall_Pop", "Global-slot card must be slotted to the townhall. Click the townhall to open its panel before slotting the card."),
						ExclusiveUIEnum::None, "PopupCannot"
					);
				}
				return;
			}

			/*
			 * Use Building Slot Card
			 */
			if (IsBuildingSlotCard(buildingEnum)) 
			{
				DescriptionUIState descriptionUIState = simulation().descriptionUIState();
				if (descriptionUIState.objectType == ObjectTypeEnum::Building)
				{
					int32 buildingId = descriptionUIState.objectId;
					Building& building = simulation().building(buildingId);

					if (building.playerId() != playerId()) {
						simulation().AddPopupToFront(playerId(), 
							LOCTEXT("CannotInsertSlotNotBldOwner_Pop", "Cannot insert a building-slot card. You don't own the Building."), 
							ExclusiveUIEnum::None, "PopupCannot"
						);
						return;
					}
					
					if (building.isEnum(CardEnum::Townhall)) {
						simulation().AddPopupToFront(playerId(), 
							LOCTEXT("CannotInsertBldSlotToTownhall_Pop", "You cannot insert a building-slot card into the townhall. Townhall's card slot requires a global-slot card."),
							ExclusiveUIEnum::None, "PopupCannot"
						);
						return;
					}

					// No Card Slot
					if (building.maxCardSlots() == 0) {
						simulation().AddPopupToFront(playerId(), 
							LOCTEXT("BldHasNoSlot_Pop", "This building has no card slot."),
							ExclusiveUIEnum::None, "PopupCannot"
						);
						return;
					}

					// Card Slots Full
					if (building.IsCardSlotsFull()) {
						simulation().AddPopupToFront(playerId(), 
							LOCTEXT("BldSlotAlreadyHasCard_Pop", "This Building's Card Slots are full. Remove a Card first by clicking on a Slotted Sard."),
							ExclusiveUIEnum::None, "PopupCannot"
						);
						return;
					}
					
					
					if (building.CanAddSlotCard()) 
					{
						if (buildingEnum == CardEnum::SustainabilityBook &&
							building.isEnum(CardEnum::Mint))
						{
							simulation().AddPopupToFront(playerId(), 
								LOCTEXT("SustainabilityCardNoMint_Pop", "Sustainability Book does not work on Mint. The usage of Gold Bar is already meticulously conserved."),
								ExclusiveUIEnum::None, "PopupCannot"
							);
							return;
						}
						
						FVector2D initialPosition = GetViewportPosition(cardButton->GetCachedGeometry());
						
						auto command = make_shared<FUseCard>();
						command->cardEnum = buildingEnum;
						command->variable1 = buildingId;
						command->SetPosition(initialPosition);
						networkInterface()->SendNetworkCommand(command);

						// Card initial animation
						//auto animationCard = AddAnimationCard(buildingEnum);
						//animationCard->StartAnimation(initialPosition, transitionPosition, CardAnimationEnum::ToBuildingSlot, Time::TicksPerSecond / 10);
						//AnimatedCardOverlay->AddChild(animationCard);

						//cardButton->SetVisibility(ESlateVisibility::Hidden);
					}
				}
				else {
					simulation().AddPopupToFront(playerId(), 
						LOCTEXT("BldSlotCardNeedBldCardSlot_Pop", "Building-slot card must be inserted into a building with card slots.<space>Click a card-slottable building to open its panel, before slotting the card."), 
						ExclusiveUIEnum::None, "PopupCannot"
					);
				}
				return;
			}

			if (buildingEnum == CardEnum::Treasure ||
				buildingEnum == CardEnum::SellFood ||
				buildingEnum == CardEnum::BuyWood ||
				buildingEnum == CardEnum::Immigration ||
				buildingEnum == CardEnum::EmergencyRations ||
				buildingEnum == CardEnum::KidnapGuard ||
				buildingEnum == CardEnum::TreasuryGuard ||
				IsSeedCard(buildingEnum) ||
				IsCrateCard(buildingEnum))
			{
				if (IsSeedCard(buildingEnum)) 
				{
					// Special Card, ensure there is a valid georesourceEnum
					auto hasGeoresource = [&](GeoresourceEnum georesourceEnum)
					{
						const std::vector<int32>& provinceIds = simulation().GetProvincesPlayer(playerId());
						for (int32 provinceId : provinceIds) {
							if (simulation().georesource(provinceId).georesourceEnum == georesourceEnum) {
								return true;
							}
						}
						return false;
					};

					FText plantName = GetTileObjInfo(GetSeedInfo(buildingEnum).tileObjEnum).name;

					// TODO: clean up
					bool noSuitableArea = false;
					if (buildingEnum == CardEnum::CannabisSeeds && !hasGeoresource(GeoresourceEnum::CannabisFarm)) {
						noSuitableArea = true;
					}
					else if (buildingEnum == CardEnum::GrapeSeeds && !hasGeoresource(GeoresourceEnum::GrapeFarm)) {
						noSuitableArea = true;
					}
					else if (buildingEnum == CardEnum::CocoaSeeds && !hasGeoresource(GeoresourceEnum::CocoaFarm)) {
						noSuitableArea = true;
					}
					else if (buildingEnum == CardEnum::CottonSeeds && !hasGeoresource(GeoresourceEnum::CottonFarm)) {
						noSuitableArea = true;
					}
					else if (buildingEnum == CardEnum::DyeSeeds && !hasGeoresource(GeoresourceEnum::DyeFarm)) {
						noSuitableArea = true;
					}
					if (noSuitableArea) {
						simulation().AddPopupToFront(playerId(), 
							FText::Format(LOCTEXT("NoLandSuitableForCrop_Pop", "None of your land is suitable for growing {0}"), plantName),
							ExclusiveUIEnum::None, "PopupCannot"
						);
						return;
					}
				}

				
				auto command = make_shared<FUseCard>();
				command->cardEnum = buildingEnum;

				auto sendCommand = [&]() {
					networkInterface()->SendNetworkCommand(command);
					cardButton->SetVisibility(ESlateVisibility::Hidden);
				};

				auto sendCommandWithWarning = [&](FText text) {
					networkInterface()->ShowConfirmationUI(text, command);
				};

				
				if (buildingEnum == CardEnum::SellFood) {
					sendCommandWithWarning(LOCTEXT("SellFood_Ask", "Are you sure you want to sell half of city's food?"));
				} 
				else if (buildingEnum == CardEnum::BuyWood) {
					int32 cost = GetResourceInfo(ResourceEnum::Wood).basePrice;
					int32 amountToBuy = simulation().money(playerId()) / 2 / cost;

					if (resourceSys.CanAddResourceGlobal(ResourceEnum::Wood, amountToBuy)) {
						sendCommandWithWarning(LOCTEXT("BuyWood_Ask", "Are you sure you want to spend half of city's money to buy wood?"));
					} else {
						simulation().AddPopupToFront(playerId(), 
							FText::Format(LOCTEXT("BuyWoodNoStorageFit_Pop", "Not enough storage space to fit {0} wood."), TEXT_NUM(amountToBuy)),
							ExclusiveUIEnum::None, "PopupCannot"
						);
					}
				}
				else if (IsCrateCard(buildingEnum))
				{
					ResourcePair resourcePair = GetCrateResource(buildingEnum);
					
					if (resourceSys.CanAddResourceGlobal(resourcePair.resourceEnum, resourcePair.count)) {
						sendCommand();
					} else {
						simulation().AddPopup(playerId(), 
							LOCTEXT("Not enough storage space.", "Not enough storage space."),
							"PopupCannot"
						);
					}
				}
				else {
					sendCommand();
				}
				
				return;
			}

			// Select ConverterCard
			if (IsWildCard(buildingEnum)) {
				// Make sure the hand is not full..
				if (cardButton->cardCount > 1 &&
					!cardSystem.CanAddCardToBoughtHand(buildingEnum, 1))
				{
					simulation().AddPopupToFront(playerId(), 
						LOCTEXT("ReachedHandLimit_Pop", "Reached hand limit for bought cards."), 
						ExclusiveUIEnum::ConverterCardHand, "PopupCannot"
					);
					return;
				}
				
				cardSystem.converterCardState = ConverterCardUseState::JustUsed;
				cardSystem.wildCardEnumUsed = buildingEnum;
				
				// The network command is sent after submitting chosen card
				return;
			}

			//auto warnOfBarnWorkMode = [&](UnitEnum unitEnum)
			//{
			//	const std::vector<int32_t>& barns = simulation().buildingIds(playerId(), CardEnum::RanchBarn);
			//	for (int32_t barnId : barns) {
			//		if (simulation().building(barnId).subclass<RanchBarn>().animalEnum() == unitEnum) {
			//			return;
			//		}
			//	}
			//	simulation().AddPopupToFront(playerId(), "Need barn with work mode set to " + GetUnitInfo(unitEnum).name);
			//};
			
			if (IsAnimalCard(buildingEnum)) 
			{
				//warnOfBarnWorkMode(GetAnimalEnumFromCardEnum(buildingEnum));
				cardButton->SetVisibility(ESlateVisibility::Hidden); // Temporily set so people can't click on this twice

				auto command = make_shared<FUseCard>();
				command->cardEnum = buildingEnum;
				networkInterface()->SendNetworkCommand(command);
				return;
			}
			
			GetPunHUD()->CloseDescriptionUI();
			return;
		}

		//// Special cases
		//if (buildingEnum == BuildingEnum::BeerBreweryFamous) {
		//	if (simulation().buildingCount(playerId(), BuildingEnum::BeerBrewery) < 4) {
		//		simulation().AddPopupToFront(playerId(), "Build 4 beer breweries before placing the famous beer brewery.");
		//		GetPunHUD()->CloseDescriptionUI();
		//		return;
		//	}
		//}
		

		inputSystemInterface()->StartBuildingPlacement(buildingEnum, cardButton->buildingLvl, true);
		
		GetPunHUD()->CloseDescriptionUI();
		return;
	}

	if (callbackEnum == CallbackEnum::SellCard)
	{
		auto command = make_shared<FSellCards>();
		command->buildingEnum = cardButton->buildingEnum;
		command->cardCount = cardButton->cardCount;
		command->isShiftDown = dataSource()->isShiftDown();

		int32 cardPrice = simulation().cardSystem(playerId()).GetCardPrice(command->buildingEnum);

		if (command->isShiftDown)
		{
			networkInterface()->ShowConfirmationUI(
				FText::Format(
					LOCTEXT("SellCardSure_Pop", "Are you sure you want to sell {0} {1} {0}|plural(one=Card,other=Cards) for {2}<img id=\"Coin\"/>?"),
					command->cardCount,
					GetBuildingInfo(command->buildingEnum).name,
					TEXT_NUM(cardPrice * command->cardCount)
				),
				command
			);
		}
		else {
			networkInterface()->ShowConfirmationUI(
				FText::Format(
					LOCTEXT("SellCardSure_Pop", "Are you sure you want to sell {0} for {1}<img id=\"Coin\"/>?"),
					GetBuildingInfo(command->buildingEnum).name,
					TEXT_NUM(cardPrice)
				),
				command
			);
		}
		
		return;
	}
}

void UMainGameUI::CallBack2(UPunWidget* punWidgetCaller, CallbackEnum callbackEnum)
{
	/*
	 * Job Priority Callback
	 */

	UJobPriorityRow* row = CastChecked<UJobPriorityRow>(punWidgetCaller);
	int32 index = row->index;

	// Move the element in JobPriorityRows
	if (callbackEnum == CallbackEnum::SetGlobalJobPriority_Up)
	{
		if (index > 0)
		{
			JobPriorityRows.RemoveAt(index);

			// Find next visible index to replace
			for (int32 insertIndex = index - 1; insertIndex >= 0; insertIndex--) {
				if (JobPriorityRows[insertIndex]->IsVisible() || insertIndex == 0) {
					JobPriorityRows.Insert(row, insertIndex);
					break;
				}
			}
		}

		PUN_CHECK(JobPriorityRows.Num() == DefaultJobPriorityListAllSeason.size());
	}
	else if (callbackEnum == CallbackEnum::SetGlobalJobPriority_Down)
	{
		if (index < JobPriorityRows.Num() - 1)
		{
			// TODO: fix weird job priority row gone bug
			int32 rowCountBeforeRemove = JobPriorityRows.Num();
			
			JobPriorityRows.RemoveAt(index);

			// Find next visible index to replace
			for (int32 insertIndex = index + 1; insertIndex < JobPriorityRows.Num(); insertIndex++) {
				if (JobPriorityRows[insertIndex]->IsVisible() || insertIndex == (JobPriorityRows.Num() - 1)) {
					JobPriorityRows.Insert(row, insertIndex);
					break;
				}
			}

			// Row disappeared, add it at the end...
			if (JobPriorityRows.Num() < rowCountBeforeRemove) {
				JobPriorityRows.Add(row);
			}
		}

		PUN_CHECK(JobPriorityRows.Num() == DefaultJobPriorityListAllSeason.size());
	}
	else if (callbackEnum == CallbackEnum::SetGlobalJobPriority_FastUp)
	{
		JobPriorityRows.RemoveAt(index);
		JobPriorityRows.Insert(row, 0);
	}
	else if (callbackEnum == CallbackEnum::SetGlobalJobPriority_FastDown)
	{
		JobPriorityRows.RemoveAt(index);
		JobPriorityRows.Add(row);
	}

	// Put JobPriorityRows into JobPriorityScrollBox
	// Construct command
	if (jobPriorityTownId != -1)
	{
		auto command = make_shared<FSetGlobalJobPriority>();
		command->townId = jobPriorityTownId;

		JobPriorityScrollBox->ClearChildren();
		for (int32 i = 0; i < JobPriorityRows.Num(); i++) {
			JobPriorityScrollBox->AddChild(JobPriorityRows[i]);
			command->jobPriorityList.Add(static_cast<int32>(JobPriorityRows[i]->cardEnum));
		}

		networkInterface()->SendNetworkCommand(command);
	}
}


int32 UMainGameUI::MoneyLeftAfterTentativeBuy()
{
	int32 money = simulation().money(playerId());
	auto& cardSystem = simulation().cardSystem(playerId());
	std::vector<bool> reserveStatus = cardSystem.GetHand1ReserveStatus();
	
	TArray<UWidget*> cardButtons = CardHand1Box->GetAllChildren();
	int32 loopSize = min(cardButtons.Num(), static_cast<int32>(reserveStatus.size()));
	for (int i = 0; i < loopSize; i++) {
		auto cardButton = CastChecked<UBuildingPlacementButton>(cardButtons[i]);
		if (reserveStatus[i]) {
			money -= cardSystem.GetCardPrice(cardButton->buildingEnum);
		}
	}
	return money;
}


#undef LOCTEXT_NAMESPACE