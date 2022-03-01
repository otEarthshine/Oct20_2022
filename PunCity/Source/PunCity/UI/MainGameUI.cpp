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

	//// Build Menu
	BuildMenuTogglerButton->CoreButton->OnClicked.AddDynamic(this, &UMainGameUI::ToggleBuildingMenu);

	// Gather Menu
	GatherButton->CoreButton->OnClicked.AddDynamic(this, &UMainGameUI::ToggleGatherButton);
	GatherSettingsCloseButton->CoreButton->OnClicked.AddDynamic(this, &UMainGameUI::CloseGatherUI);

	// Card Inventory UI
	CardInventoryToggleButton->OnClicked.AddDynamic(this, &UMainGameUI::ToggleCardInventoryButton);
	CardInventoryToggleButton_Close->OnClicked.AddDynamic(this, &UMainGameUI::ToggleCardInventoryButton);

	CardRerollButton->OnClicked.AddDynamic(this, &UMainGameUI::ClickRerollButton);
	CardRerollButton1->OnClicked.AddDynamic(this, &UMainGameUI::ClickRerollButton);
	
	ResetBottomMenuDisplay();

	// Card hand 1
	CardHand1Overlay->SetVisibility(ESlateVisibility::Collapsed);

	CardStackButton->IsFocusable = false;
	CardStackButton->OnClicked.AddDynamic(this, &UMainGameUI::ClickCardStackButton);

	CardHand1SubmitButton->OnClicked.AddDynamic(this, &UMainGameUI::ClickCardHand1SubmitButton);
	CardHand1CancelButton->OnClicked.AddDynamic(this, &UMainGameUI::ClickCardHand1CancelButton);
	CardHand1CloseButton->CoreButton->OnClicked.AddDynamic(this, &UMainGameUI::ClickCardHand1CancelButton);

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
	ConverterCardHandXCloseButton->CoreButton->OnClicked.AddDynamic(this, &UMainGameUI::ClickConverterCardHandCancelButton);

	converterHandCategoryState = -1;
	lastConverterHandCategoryState = -1;
	
	ConverterCardHand_AgricultureButton->OnClicked.AddDynamic(this, &UMainGameUI::ClickConverterCardHand_AgricultureButton);
	ConverterCardHand_IndustryButton->OnClicked.AddDynamic(this, &UMainGameUI::ClickConverterCardHand_IndustryButton);
	ConverterCardHand_ServicesButton->OnClicked.AddDynamic(this, &UMainGameUI::ClickConverterCardHand_ServicesButton);
	ConverterCardHand_OthersButton->OnClicked.AddDynamic(this, &UMainGameUI::ClickConverterCardHand_OthersButton);

	ConverterCardHandConfirmUI->ConfirmYesButton->CoreButton->OnClicked.AddDynamic(this, &UMainGameUI::ClickCardRemovalConfirmYesButton);
	ConverterCardHandConfirmUI->ConfirmNoButton->CoreButton->OnClicked.AddDynamic(this, &UMainGameUI::ClickCardRemovalConfirmNoButton);
	ConverterCardHandConfirmUI->SetVisibility(ESlateVisibility::Collapsed);
	

	ConfirmationOverlay->SetVisibility(ESlateVisibility::Collapsed);
	ConfirmationYesButton->OnClicked.AddDynamic(this, &UMainGameUI::OnClickConfirmationYes);
	ConfirmationNoButton->OnClicked.AddDynamic(this, &UMainGameUI::OnClickConfirmationNo);

	BuildingMenuWrap->ClearChildren();

	for (int i = CardInventoryUIWrap->GetChildrenCount(); i-- > 0;) {
		auto cardButton = Cast<UBuildingPlacementButton>(CardInventoryUIWrap->GetChildAt(i));
		SetChildHUD(cardButton);
	}

	//ItemSelectionUI->SetVisibility(ESlateVisibility::Collapsed);
	//GlobalItemsHorizontalBox->ClearChildren();

	ResearchBarUI->OnClicked.AddDynamic(this, &UMainGameUI::ToggleResearchMenu);
	AddToolTip(ResearchBarUI, 
		LOCTEXT("ResearchBarUI_Tip", "Bring up the Tech Tree.\n<Orange>[T]</>")
	);

	ProsperityBarUI->OnClicked.AddDynamic(this, &UMainGameUI::ToggleProsperityUI);
	AddToolTip(ProsperityBarUI, 
		LOCTEXT("ProsperityBarUI_Tip", "Bring up the Upgrades Tree.\n<Orange>[U]</>")
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
	
	AddToolTip(CardInventoryToggleButton, LOCTEXT("CardInventoryToggleButton_Tip", "Show the <Bold>Card Inventory</> where you can keep cards that are rarely used."));
	AddToolTip(CardInventoryToggleButton_Close, LOCTEXT("CardInventoryToggleButton_Close_Tip", "Close the Card Inventory."));
	
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

	LeaderSkillButton->CoreButton->OnClicked.AddDynamic(this, &UMainGameUI::OnClickLeaderSkillButton);
	LeaderSkillButton->bOverride_Cursor = false;

	/*
	 * Reinforcement
	 */
	ReinforcementOverlay->SetVisibility(ESlateVisibility::Collapsed);
	for (int32 i = 0; i < ReinforcementSlots->GetChildrenCount(); i++) {
		SetChildHUD(CastChecked<UPunWidget>(ReinforcementSlots->GetChildAt(i)));
	}
	for (int32 i = 0; i < ReinforcementRemovedSlots->GetChildrenCount(); i++) {
		SetChildHUD(CastChecked<UPunWidget>(ReinforcementRemovedSlots->GetChildAt(i)));
	}

	/*
	 * Card Sets
	 */
	CardSetsUIOverlay->SetVisibility(ESlateVisibility::Collapsed);


	
	/*
	 * Job Priority
	 */
	BUTTON_ON_CLICK(JobPriorityCloseButton, this, &UMainGameUI::OnClickJobPriorityCloseButton);
	BUTTON_ON_CLICK(JobPriorityCloseButton2, this, &UMainGameUI::OnClickJobPriorityCloseButton);
	JobPriorityOverlay->SetVisibility(ESlateVisibility::Collapsed);
	_laborerPriorityState.lastPriorityInputTime = -999.0f;

	LaborerManualCheckBox->OnCheckStateChanged.Clear();
	LaborerManualCheckBox->OnCheckStateChanged.AddUniqueDynamic(this, &UMainGameUI::OnCheckManualLaborer);
	BUTTON_ON_CLICK(LaborerArrowUp, this, &UMainGameUI::IncreaseLaborers);
	BUTTON_ON_CLICK(LaborerArrowDown, this, &UMainGameUI::DecreaseLaborers);

	BuilderManualCheckBox->OnCheckStateChanged.Clear();
	BuilderManualCheckBox->OnCheckStateChanged.AddUniqueDynamic(this, &UMainGameUI::OnCheckManualBuilder);
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

	LeanProfiler leanProfilerOuter(LeanProfilerEnum::TickMainGameUI);


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

	int32 modTickCountShift = 0;
	auto runEachXTicks = [&](int32 xTick) {
		return (kTickCount % (xTick + modTickCountShift++)) == 0;
	};
	

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
		LEAN_PROFILING_UI(TickMainGameUI_Cards);
		
		// Cards
		auto& cardSystem = sim.cardSystem(playerId());
		int32 rollCountdown = Time::SecondsPerRound - Time::Seconds() % Time::SecondsPerRound;
		
		if (_lastRoundCountdown != rollCountdown) {
			_lastRoundCountdown = rollCountdown;

			if (PunSettings::IsOn("RoundCountdownSound")) 
			{
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
		RerollPrice->SetColorAndOpacity(globalResourceSys.moneyCap32() >= rerollPrice ? FLinearColor::White : FLinearColor(0.4, 0.4, 0.4));
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
		int32 currentMoneyLeft = sim.moneyCap32(playerId()) - TokensNeededForTentativeBuy(ResourceEnum::Money);
		if (currentMoneyLeft < 0) {
			// Unreserve if there isn't enough money
			cardSystem.UnreserveIfRanOutOfCash(currentMoneyLeft, ResourceEnum::Money);
		}

		int32 currentInfluenceLeft = sim.influence(playerId()) - TokensNeededForTentativeBuy(ResourceEnum::Influence);
		if (currentInfluenceLeft < 0) {
			cardSystem.UnreserveIfRanOutOfCash(currentMoneyLeft, ResourceEnum::Money);
		}

		// Recalculate need resource
		std::vector<bool> hand1NeedMoneyOrInfluence;
		{
			std::vector<CardEnum> hand = cardSystem.GetHand();
			std::vector<bool> handReservation = cardSystem.GetHand1ReserveStatus();
			
			for (size_t i = 0; i < hand.size(); i++)
			{
				bool needResource;
				if (cardSystem.GetCardPriceTokenEnum(hand[i]) == ResourceEnum::Money) {
					needResource = currentMoneyLeft < cardSystem.GetCardPrice(hand[i], ResourceEnum::Money);
				} else {
					needResource = currentInfluenceLeft < cardSystem.GetCardPrice(hand[i], ResourceEnum::Influence);
				}
				if (handReservation[i]) {
					needResource = false; // reserved card doesn't need resource 
				}
				hand1NeedMoneyOrInfluence.push_back(needResource);
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
				auto cardButton = AddCard(CardHandEnum::DrawHand, CardStatus(buildingEnum, 1), CardHand1Box, CallbackEnum::SelectUnboughtCard, i);


				// Price
				cardButton->SetPrice(cardSystem.GetCardPrice(buildingEnum), cardSystem.GetCardPriceTokenEnum(buildingEnum));
			}

			// Highlight the selected card
			// Gray out cards that can't be bought
			TArray<UWidget*> cardButtons = CardHand1Box->GetAllChildren();
			for (int i = 0; i < cardButtons.Num(); i++) {
				auto cardButton = CastChecked<UBuildingPlacementButton>(cardButtons[i]);
				cardButton->SetCardStatus(CardHandEnum::DrawHand, _lastHand1ReserveStatus[i], _lastHand1NeedMoneyOrInfluenceStatus[i]);
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
			_lastHand1NeedMoneyOrInfluenceStatus = hand1NeedMoneyOrInfluence;
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
				_lastHand1NeedMoneyOrInfluenceStatus != hand1NeedMoneyOrInfluence) 
		{
			_lastHand1ReserveStatus = cardSystem.GetHand1ReserveStatus();
			_lastHand1NeedMoneyOrInfluenceStatus = hand1NeedMoneyOrInfluence;
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
				AddCard(CardHandEnum::RareHand, CardStatus(buildingEnum, 1), RareCardHandBox, CallbackEnum::SelectUnboughtRareCardPrize, i);
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
			RareCardHandText->SetVisibility(cardSystem.rareHandMessage().IsEmpty() ? ESlateVisibility::Collapsed : ESlateVisibility::HitTestInvisible);
			
			SetText(RareCardHandText2, cardSystem.rareHandMessage2());

			rareHandObjectId = cardSystem.GetRareHandObjectId();

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
			if (!ConverterCardHandOverlay->IsVisible()) {
				converterHandCategoryState = -1;
			}
			
			// Open if not already done so
			if (!ConverterCardHandOverlay->IsVisible() ||
				lastSearchString != searchString ||
				lastConverterHandCategoryState != converterHandCategoryState)
			{
				lastSearchString = searchString;
				lastConverterHandCategoryState = converterHandCategoryState;
				
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
									GetBuildingInfo(buildingEnum).name.ToString().Find(searchString, ESearchCase::Type::IgnoreCase, ESearchDir::FromStart) != INDEX_NONE)
								{
									auto cardButton = AddCard(CardHandEnum::ConverterHand, CardStatus(buildingEnum, 1), ConverterCardHandBox, CallbackEnum::SelectCardRemoval, i);

									SetText(ConverterCardHandTitle, LOCTEXT("CHOOSE A CARD TO REMOVE", "CHOOSE A CARD TO REMOVE"));

									cardButton->SetPrice(cardSystem.GetCardPrice(buildingEnum), cardSystem.GetCardPriceTokenEnum(buildingEnum));
									//SetText(cardButton->PriceText, TEXT_NUM(cardSystem.GetCardPrice(buildingEnum)));
									//cardButton->PriceTextBox->SetVisibility(ESlateVisibility::HitTestInvisible);
								}
							}
						}
					}
				}
				else
				{
					// Show all buildings
					for (int32 i = 0; i < SortedNameBuildingEnum.size(); i++)
					{
						CardEnum buildingEnum = SortedNameBuildingEnum[i];

						// Show wild card by type
						if (converterHandCategoryState == 0 && !IsAgricultureBuilding(buildingEnum)) {
							continue;
						}
						if (converterHandCategoryState == 1 && !(IsIndustrialBuilding(buildingEnum) || IsMountainMine(buildingEnum))) {
							continue;
						}
						if (converterHandCategoryState == 2 && !IsServiceBuilding(buildingEnum)) {
							continue;
						}
						if (converterHandCategoryState == 3 && 
							(IsAgricultureBuilding(buildingEnum) || IsIndustrialBuilding(buildingEnum) || IsMountainMine(buildingEnum) || IsServiceBuilding(buildingEnum))) {
							continue;
						}

						if (uniqueAvailableCards.find(buildingEnum) != uniqueAvailableCards.end())
						{
							// Can only convert to building
							if (IsBuildingCard(buildingEnum))
							{
								// Only Cards filtered by Search Box
								if (searchString.IsEmpty() ||
									GetBuildingInfo(buildingEnum).name.ToString().Find(searchString, ESearchCase::Type::IgnoreCase, ESearchDir::FromStart) != INDEX_NONE)
								{
									auto cardButton = AddCard(CardHandEnum::ConverterHand, CardStatus(buildingEnum, 1), ConverterCardHandBox, CallbackEnum::SelectConverterCard, i);

									SetText(ConverterCardHandTitle, LOCTEXT("ConverterCardHandTitle", "CHOOSE A CARD\npay the price to build"));

									cardButton->SetPrice(cardSystem.GetCardPrice(buildingEnum), cardSystem.GetCardPriceTokenEnum(buildingEnum));
									//SetText(cardButton->PriceText, TEXT_NUM(cardSystem.GetCardPrice(buildingEnum)));
									//cardButton->PriceTextBox->SetVisibility(ESlateVisibility::HitTestInvisible);
								}
							}
						}
					}

					// When cheat is on, show all cards
					if (PunSettings::IsOn("SeeAllCards"))
					{
						int32 i = 0;
						for (const BldInfo& cardInfo : CardInfos)
						{
							CardEnum buildingEnum = cardInfo.cardEnum;

							// Only Cards filtered by Search Box
							if (searchString.IsEmpty() ||
								GetBuildingInfo(buildingEnum).name.ToString().Find(searchString, ESearchCase::Type::IgnoreCase, ESearchDir::FromStart) != INDEX_NONE)
							{
								auto cardButton = AddCard(CardHandEnum::ConverterHand, CardStatus(buildingEnum, 1), ConverterCardHandBox, CallbackEnum::None, i);

								SetText(ConverterCardHandTitle, LOCTEXT("ConverterCardHandTitle", "CHOOSE A CARD\npay the price to build"));
								cardButton->SetPrice(cardSystem.GetCardPrice(buildingEnum), cardSystem.GetCardPriceTokenEnum(buildingEnum));
								//SetText(cardButton->PriceText, TEXT_NUM(cardSystem.GetCardPrice(buildingEnum)));
								//cardButton->PriceTextBox->SetVisibility(ESlateVisibility::HitTestInvisible);
							}
							i++;
						}
					}
				}

				networkInterface()->ResetGameUI();

				ConverterCardHand_AgricultureActiveImage->SetVisibility(converterHandCategoryState == 0 ? ESlateVisibility::HitTestInvisible : ESlateVisibility::Hidden);
				ConverterCardHand_IndustryActiveImage->SetVisibility(converterHandCategoryState == 1 ? ESlateVisibility::HitTestInvisible : ESlateVisibility::Hidden);
				ConverterCardHand_ServicesActiveImage->SetVisibility(converterHandCategoryState == 2 ? ESlateVisibility::HitTestInvisible : ESlateVisibility::Hidden);
				ConverterCardHand_OthersActiveImage->SetVisibility(converterHandCategoryState == 3 ? ESlateVisibility::HitTestInvisible : ESlateVisibility::Hidden);
				
				ConverterCardHandOverlay->SetVisibility(ESlateVisibility::Visible);

				_lastConverterChosenCard = CardEnum::None; // Start off with nothing chosen

				// TODO: proper card deal animation+sound
				dataSource()->Spawn2DSound("UI", "CardDeal");
			}

			// Highlight the selected card
			TArray<UWidget*> cardButtons = ConverterCardHandBox->GetAllChildren();
			for (int i = 0; i < cardButtons.Num(); i++) {
				auto cardButton = CastChecked<UBuildingPlacementButton>(cardButtons[i]);
				bool shouldHighlight = cardButton->cardStatus.cardEnum == _lastConverterChosenCard;
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

		if (cardEnumBeingPlaced == CardEnum::None &&
			ReinforcementOverlay->IsVisible() && 
			reinforcementCallbackEnum == CallbackEnum::RaidBattle) 
		{
			cardEnumBeingPlaced = CardEnum::Raid;
		}
		
		if (_lastDisplayBought != cardSystem.GetCardsBought_Display() ||
			_lastCardEnumBeingPlaced != cardEnumBeingPlaced ||
			_lastConverterCardState != cardSystem.converterCardState)
		{
			_lastDisplayBought = cardSystem.GetCardsBought_Display();
			_lastCardEnumBeingPlaced = cardEnumBeingPlaced;
			_lastConverterCardState = cardSystem.converterCardState;

			// Remove index being placed or used
			std::vector<CardStatus> actualDisplayBought;
			for (size_t i = 0; i < _lastDisplayBought.size(); i++)
			{
				CardStatus currentStack = _lastDisplayBought[i];
				if (currentStack.cardEnum == cardEnumBeingPlaced) {
					currentStack.stackSize--;
				}
				else if (_lastConverterCardState != ConverterCardUseState::None && currentStack.cardEnum == cardSystem.wildCardEnumUsed)
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
				AddCard(CardHandEnum::BoughtHand, actualDisplayBought[i], CardHand2Box, CallbackEnum::SelectBoughtCard, i, true);
			}

			// Warning to bought cards that need resources
			TArray<UWidget*> cardBoughtButtons = CardHand2Box->GetAllChildren();
			for (int i = 0; i < cardBoughtButtons.Num(); i++) 
			{
				auto cardButton = CastChecked<UBuildingPlacementButton>(cardBoughtButtons[i]);

				if (IsBuildingCard(cardButton->cardStatus.cardEnum))
				{
					bool needResource = false;
					const std::vector<int32> resourceNeeded = GetBuildingInfo(cardButton->cardStatus.cardEnum).GetConstructionResources(sim.playerOwned(playerId()).factionEnum());
					
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
		LEAN_PROFILING_UI(TickMainGameUI_Events);
		
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
		LEAN_PROFILING_UI(TickMainGameUI_Exclamation);
		
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
				
				const std::vector<CardStatus>& cardsBought = sim.cardSystem(playerId()).GetCardsBought();
				for (const auto& cardStack : cardsBought) {
					if (IsAgricultureBuilding(cardStack.cardEnum)) {
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
		LeanProfiler leanProfilerOuter2(LeanProfilerEnum::TickMainGameUI_LeftUI);
		
		//! Stats:
		UnitSystem& unitSys = sim.unitSystem();
		StatSystem& statSys = sim.statSystem();
		GlobalResourceSystem& globalResourceSys = sim.globalResourceSystem(playerId());

		int32 population = sim.populationTown(currentTownId());

		

		// Top left
		{
			LEAN_PROFILING_UI(TickMainGameUI_TopLeft);

			SetText(TimeText, FText::FormatNamed(
				LOCTEXT("InGameTopLeft_SeasonYear", "{EarlyMidLate} {SeasonName}"),
				TEXT("EarlyMidLate"), Time::SeasonPrefix(Time::Ticks()),
				TEXT("SeasonName"), Time::SeasonName(Time::Seasons())
			));

			SetText(YearText, FText::FormatNamed(
				LOCTEXT("InGameTopLeft_SeasonYear", "Year {Years}"),
				TEXT("Years"), TEXT_NUM(Time::Years())
			));


			FloatDet celsius = sim.Celsius(dataSource()->cameraAtom().worldTile2());
			SetText(TemperatureText, FText::Format(
				INVTEXT("{0}°C ({1}°F)"),
				TEXT_NUM(FDToInt(celsius)),
				TEXT_NUM(FDToInt(CelsiusToFahrenheit(celsius)))
			));

			float fraction = FDToFloat(celsius - Time::MinCelsiusBase()) / FDToFloat(Time::MaxCelsiusBase() - Time::MinCelsiusBase());
			TemperatureImage->GetDynamicMaterial()->SetScalarParameterValue("Fraction", fraction);

			TemperatureTextBox->SetVisibility(ESlateVisibility::Visible);

			int32 childPopulation = sim.townManager(currentTownId()).childPopulation();
			int32 adultPopulation = population - childPopulation;
			AdultPopulationText->SetText(FText::FromString(FString::FromInt(adultPopulation)));
			ChildPopulationText->SetText(FText::FromString(FString::FromInt(childPopulation)));

			if (runEachXTicks(60))
			{
				LeanProfiler leanProfilerInner(LeanProfilerEnum::TickMainGameUI_TopLeftTip);

				AddToolTip(TemperatureTextBox, FText::Format(
					LOCTEXT("TemperatureTextBox_Tip", "Below {0}°C, citizens will need wood/coal to heat themselves"),
					TEXT_NUM(FDToInt(Time::ColdCelsius()))
				));
				
				auto tooltip = AddToolTip(PopulationBox, FText::Format(
					LOCTEXT("PopulationBox_Tip", "Population: {0}<bullet>{1} Adults</><bullet>{2} Children</>"),
					TEXT_NUM(population),
					TEXT_NUM(adultPopulation),
					TEXT_NUM(childPopulation)
				));


				auto punGraph = tooltip->TooltipPunBoxWidget->AddThinGraph();

				// Food Graph
				if (bIsHoveredButGraphNotSetup ||
					tooltip->TooltipPunBoxWidget->DidElementResetThisRound())
				{
					bIsHoveredButGraphNotSetup = false;

					punGraph->SetupGraph({
						{ LOCTEXT("Population", "Population").ToString(), PlotStatEnum::Population, FLinearColor(0.3, 1, 0.3), playerId(), currentTownId() },
						{ LOCTEXT("Children", "Children").ToString(), PlotStatEnum::ChildPopulation, FLinearColor(0.3, 0.3, 1), playerId(), currentTownId() },
					});
				}
			}
		}

		if (shouldDisplayMainGameUI)
		{
			LEAN_PROFILING_UI(TickMainGameUI_TopLeft);
			
			int32 housingCapacity = sim.HousingCapacity(currentTownId());
			HousingSpaceText->SetText(FText::FromString(FString::FromInt(population) + FString("/") + FString::FromInt(housingCapacity)));


			std::pair<int32, int32> slotsPair = sim.GetStorageCapacity(currentTownId());
			int32 usedSlots = slotsPair.first;
			int32 totalSlots = slotsPair.second;
			SetText(StorageSpaceText, to_string(usedSlots) + "/" + to_string(totalSlots));

			if (runEachXTicks(60))
			{
				bool isStorageAllFull = sim.isStorageAllFull(currentTownId());
				StorageSpaceText->SetColorAndOpacity(
					isStorageAllFull ? FLinearColor(1, 0, 0, 0.7) : FLinearColor(1, 1, 1, 0.7)
				);
				
				{
					LeanProfiler leanProfilerInner(LeanProfilerEnum::TickMainGameUI_TopLeftTip);

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
					LeanProfiler leanProfilerInner(LeanProfilerEnum::TickMainGameUI_TopLeftTip);

					TArray<FText> args;

					AddToolTip(StorageSpaceBox, FText::Format(LOCTEXT("StorageSpaceBox_Tip", "Storage space:\nUsed slots: {0}\nTotal slots: {1}"),
						TEXT_NUM(usedSlots),
						TEXT_NUM(totalSlots)
					));
				}
			}
		}

		auto& playerOwned = sim.playerOwned(playerId());
		auto& townManager = sim.townManager(currentTownId());
		
		// Happiness (Town)
		if (runEachXTicks(60))
		{
			LEAN_PROFILING_UI(TickMainGameUI_Happiness);
			
			int32 overallHappiness = townManager.aveOverallHappiness();
			Happiness->SetImage(assetLoader()->GetHappinessFace(overallHappiness));
			Happiness->SetText(FText(), TEXT_NUM(overallHappiness));
			Happiness->SetTextColor(GetHappinessColor(overallHappiness));

			TArray<FText> args;
			ADDTEXT_(LOCTEXT("Happiness_Tip1", "Happiness: {0}\n"), ColorHappinessText(overallHappiness, TEXT_PERCENT(overallHappiness)));
			for (size_t i = 0; i < HappinessEnumCount; i++) 
			{
				if (static_cast<HappinessEnum>(i) == HappinessEnum::Tourism &&
					!sim.IsResearched(playerId(), TechEnum::Tourism))
				{
					continue;
				}
				
				int32 aveHappiness = townManager.aveHappinessByType(static_cast<HappinessEnum>(i));
				ADDTEXT_(INVTEXT("  {0} {1}\n"), 
					ColorHappinessText(aveHappiness, TEXT_PERCENT(aveHappiness)),
					HappinessEnumName[i]
				);
			}
			ADDTEXT_INV_("<space>");
			ADDTEXT_LOCTEXT("Happiness_Tip2", "Low Happiness can lower citizens's Work Speed. If Happiness is very low, citizens may leave your city.");

			AddToolTip(Happiness, JOINTEXT(args));
		}

		// Money (Player)
		{
			LEAN_PROFILING_UI(TickMainGameUI_Money);

			Money->SetText(FText(), TEXT_NUM(globalResourceSys.money()));

			int32 totalIncome100 = playerOwned.totalIncome100();
			MoneyChangeText->SetText(TEXT_100SIGNED(totalIncome100));
		}

		if (runEachXTicks(60))
		{
			LEAN_PROFILING_UI(TickMainGameUI_MoneyTip);

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
			LEAN_PROFILING_UI(TickMainGameUI_Influence);
			
			Influence->SetText("", to_string(globalResourceSys.influence()));
			InfluenceChangeText->SetText(TEXT_100SIGNED(playerOwned.totalInfluenceIncome100()));

			if (runEachXTicks(60))
			{
				TArray<FText> args;
				ADDTEXT_LOCTEXT("Influence_Tip1", "Influence points used for claiming land and vassalizing other towns.\n\n");
				playerOwned.AddInfluenceIncomeToString(args);

				FText tipText = JOINTEXT(args);
				AddToolTip(Influence, tipText);
				AddToolTip(InfluenceChangeText, tipText);
			}

			Influence->SetVisibility(ESlateVisibility::Visible);
			InfluenceChangeText->SetVisibility(ESlateVisibility::Visible);
		} else {
			Influence->SetVisibility(ESlateVisibility::Collapsed);
			InfluenceChangeText->SetVisibility(ESlateVisibility::Collapsed);
		}

		// Science (Player)
		if (sim.unlockSystem(playerId())->researchEnabled) 
		{
			LEAN_PROFILING_UI(TickMainGameUI_Science);
			
			// Science Text
			Science->SetText(FText(), FText::Format(INVTEXT("+{0}"), TEXT_100(playerOwned.science100PerRound())));

			// Science Tip
			if (runEachXTicks(60))
			{
				TArray<FText> args;
				ADDTEXT_(LOCTEXT("ScienceTip", "Science is used for researching new technology.\n Science per round: {0} <img id=\"Science\"/>\n"), TEXT_100(playerOwned.science100PerRound()));

				for (size_t i = 0; i < ScienceEnumCount; i++)
				{
					int64 science100 = 0;
					const auto& townIds = playerOwned.townIds();
					for (int32 townId : townIds) {
						science100 += sim.townManager(townId).sciences100[i];
					}
					if (science100 != 0) {
						ADDTEXT_(INVTEXT("  {0} {1}\n"), TEXT_100(science100), ScienceEnumName(i));
					}
				}

				AddToolTip(Science, args);
			}

			Science->SetVisibility(ESlateVisibility::Visible);
		} else {
			Science->SetVisibility(ESlateVisibility::Collapsed);
		}

		/*
		 * Town Swap Texts
		 */
		{
			LEAN_PROFILING_UI(TickMainGameUI_TownSwap);

			PunUIUtils::SetTownSwapText(currentTownId(), &sim, LeftUITownSwapText, LeftUITownSwapHorizontalBox);
			PunUIUtils::SetTownSwapText(jobPriorityTownId, &sim, TownSwapText, TownSwapHorizontalBox);
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
			LEAN_PROFILING_UI(TickMainGameUI_Skill);
			
			BldInfo cardInfo = GetBuildingInfo(playerOwned.currentSkill());
			int32 skillMana = GetSkillManaCost(cardInfo.cardEnum);
			int32 maxMana = playerOwned.maxSP();

			if (runEachXTicks(60))
			{
				TArray<FText> args;
				ADDTEXT_(INVTEXT("<Bold>{0}</>\n"), cardInfo.name);
				ADDTEXT_TAGN_("<SPColor>", LOCTEXT("Leader Skill", "Leader Skill"));
				ADDTEXT_INV_("<space>");
				ADDTEXT_(INVTEXT("{0}: <Orange>[V]</>\n"), LOCTEXT("Hotkey", "Hotkey"));
				ADDTEXT_TAGN_("<Gray>", LOCTEXT("(Click to use)", "(Click to use)"));
				ADDTEXT_INV_("<line><space>");
				ADDTEXT_(LOCTEXT("SP cost: {0}\n", "SP cost: {0}"), skillMana);
					ADDTEXT_INV_("<space>");
				ADDTEXT__(cardInfo.GetDescription())
					ADDTEXT_INV_("<space>");
				ADDTEXT_(INVTEXT("<SPColor>SP: {0}/{1}</>"), TEXT_NUM(playerOwned.GetSP()), TEXT_NUM(maxMana));
				AddToolTip(LeaderSkillButton, args);
			}
			
			LeaderManaBar->GetDynamicMaterial()->SetScalarParameterValue("Fraction", Clamp01(playerOwned.spFloat() / maxMana));
			//SetText(LeaderManaText, "SP " + to_string(playerOwned.GetSP()) + "/" + to_string(maxMana));
			LeaderSkillClock->GetDynamicMaterial()->SetScalarParameterValue("Fraction", Clamp01(playerOwned.spFloat() / skillMana));
		}


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

			iconTextPair->SetImage(resourceEnum, assetLoader(), true); // Note.. AddResourceTooltip already checked for IsHovered
			iconTextPair->InitBackgroundButton(resourceEnum);
		};

		//SetResourceIconPair(WoodCount, ResourceEnum::Wood);



		{
			LEAN_PROFILING_UI(TickMainGameUI_LeftFood);
			
			// Food
			int32 foodCount = sim.foodCount(currentTownId());
			FoodCountText->SetText(FText::Format(LOCTEXT("Food: {0}", "Food: {0}"), TEXT_NUM(foodCount)));
			FoodCountText->SetColorAndOpacity(foodCount > 0 ? FLinearColor::White : FLinearColor::Red);
			PlayAnimationIf("FoodCountLowFlash", foodCount == 0);

			BUTTON_ON_CLICK(FoodCountButton, this, &UMainGameUI::OnClickFoodCountButton);

			if (runEachXTicks(60))
			{
				// Food Stats Tip
				auto& subStatSys = sim.statSystem(currentTownId());

				const std::vector<std::vector<int32>>& productionStats = subStatSys.GetResourceStat(ResourceSeasonStatEnum::Production);
				const std::vector<std::vector<int32>>& consumptionStats = subStatSys.GetResourceStat(ResourceSeasonStatEnum::Consumption);

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
				if (bIsHoveredButGraphNotSetup ||
					tooltip->TooltipPunBoxWidget->DidElementResetThisRound())
				{
					bIsHoveredButGraphNotSetup = false;

					punGraph->SetupGraph({
						{ LOCTEXT("Production (season)", "Production (season)").ToString(), PlotStatEnum::FoodProduction, FLinearColor(0.3, 1, 0.3) },
						{ LOCTEXT("Consumption (season)", "Consumption (season)").ToString(), PlotStatEnum::FoodConsumption, FLinearColor(1, 0.3, 0.3) },
						});
				}

			}
		}

		if (runEachXTicks(60))
		{
			LEAN_PROFILING_UI(TickMainGameUI_LeftLuxTip);
			
			// Luxury
			AddToolTip(LuxuryTier1Text, LuxuryResourceTip(1));
			AddToolTip(LuxuryTier2Text, LuxuryResourceTip(2));
			AddToolTip(LuxuryTier3Text, LuxuryResourceTip(3));
		}

		for (int i = 0; i < ResourceEnumCount; i++)
		{
			LEAN_PROFILING_UI(TickMainGameUI_LeftResources);
			
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
							// Without tools, we show flashing Iron tools
							if (resourceEnum == ResourceEnum::IronTools) {
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
	}

	/*
	 * ResearchBarUI
	 */
	UnlockSystem* unlockSys = sim.unlockSystem(playerId());
	if (unlockSys) 
	{
		LEAN_PROFILING_UI(TickMainGameUI_TechBar);
		
		// Flash ResearchBarUI if there is nothing being researched
		//FLinearColor researchBarDark = FLinearColor(.025f, .025f, .025f, 0.8f);
		//FLinearColor researchBarColor = researchBarDark;
		if (unlockSys->shouldFlashTechToggler()) {
			if (kTickCount % 60 < 30) {
				//researchBarColor = FLinearColor(1.0f, 0.33333f, 0.0f, 0.8f);
			}
		}
		
		std::shared_ptr<ResearchInfo> currentTech = unlockSys->currentResearch();
		bool isOnMainTree = unlockSys->IsOnMainTechTree(currentTech->techEnum);

		/*
		 * Technologies
		 */
		if (unlockSys->researchEnabled)
			//&&!unlockSys->allTechsUnlocked())
		{
			if (currentTech->techEnum != TechEnum::None && isOnMainTree)
			{
				ResearchingText->SetText(currentTech->GetName());
				
				TArray<FText> args;
				unlockSys->SetDisplaySciencePoint(args, false);
				SetText(ResearchingAmountText, args);
				
				ResearchBar->SetWidthOverride(unlockSys->hasTargetResearch() ? (unlockSys->researchFraction() * 240) : 0);
				ResearchBarBox->SetVisibility(ESlateVisibility::Visible);
				ResearchingLeftoverAmountBox->SetVisibility(ESlateVisibility::Collapsed);
			}
			else {
				ResearchingText->SetText(LOCTEXT("Tech Tree", "Tech Tree"));
				ResearchBarBox->SetVisibility(ESlateVisibility::Collapsed);
				ResearchingLeftoverAmountText->SetText(TEXT_100(unlockSys->science100()));
				ResearchingLeftoverAmountBox->SetVisibility(ESlateVisibility::Visible);
			}

			//ResearchBarUI->SetBackgroundColor(researchBarColor);
			ResearchBarUI->SetVisibility(ESlateVisibility::Visible);
		}
		else {
			ResearchBarUI->SetVisibility(ESlateVisibility::Collapsed);
		}

		/*
		 * Upgrades
		 */
		// ProsperityBarUI Tick
		if (unlockSys->prosperityEnabled) 
		{
			if (currentTech->techEnum != TechEnum::None && !isOnMainTree)
			{
				UpgradeTitleText->SetText(currentTech->GetName());

				TArray<FText> args;
				unlockSys->SetDisplaySciencePoint(args, false);
				SetText(ProsperityAmountText, args);

				ProsperityBar->SetWidthOverride(unlockSys->hasTargetResearch() ? (unlockSys->researchFraction() * 240) : 0);
				ProsperityBarBox->SetVisibility(ESlateVisibility::Visible);
			}
			else {
				UpgradeTitleText->SetText(LOCTEXT("Upgrades Tree", "Upgrades Tree"));
				ProsperityBarBox->SetVisibility(ESlateVisibility::Collapsed);
			}
			
			//ProsperityBarUI->SetBackgroundColor(researchBarColor);
			ProsperityBarUI->SetVisibility(ESlateVisibility::Visible);
		}
		else {
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
		LEAN_PROFILING_UI(TickMainGameUI_JobPrior);
		
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
				//jobRow->Rename(*(FString("Job_") + FString::FromInt(static_cast<int>(jobEnum)))); // Causes Crash??
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
	 * CardInventory
	 */
	{
		auto& cardSys = sim.cardSystem(playerId());

		int32 maxCardInventorySlots = cardSys.maxCardInventorySlots();

		CardInventoryToggleButton->SetVisibility(maxCardInventorySlots > 0 && !CardInventorySizeBox->IsVisible() ? ESlateVisibility::Visible : ESlateVisibility::Collapsed);
		CardInventoryToggleButton_Close->SetVisibility(maxCardInventorySlots > 0 && CardInventorySizeBox->IsVisible() ? ESlateVisibility::Visible : ESlateVisibility::Collapsed);
		
		CardInventoryLinkImage->SetVisibility(maxCardInventorySlots > 0 ? ESlateVisibility::Visible : ESlateVisibility::Collapsed);

		const std::vector<CardStatus>& cardInventory = cardSys.cardInventory();

		for (int i = CardInventoryUIWrap->GetChildrenCount(); i-- > 0;)
		{
			CardStatus cardStatus;
			if (i < cardInventory.size()) {
				cardStatus = cardInventory[i];
			}

			auto cardButton = Cast<UBuildingPlacementButton>(CardInventoryUIWrap->GetChildAt(i));

			if (i >= maxCardInventorySlots) {
				cardButton->SetVisibility(ESlateVisibility::Collapsed);
				continue;
			}
			cardButton->SetVisibility(ESlateVisibility::Visible);
			
			cardButton->PunInit(cardStatus, i, this, CallbackEnum::SelectInventorySlotCard, CardHandEnum::CardInventorySlots);
			cardButton->SetCardStatus(CardHandEnum::CardInventorySlots, false, false);
			cardButton->RefreshBuildingIcon(assetLoader());
			
			//cardButton->ParentOverlay->SetVisibility(cardEnum == CardEnum::None ? ESlateVisibility::Collapsed : ESlateVisibility::Visible);
			//cardButton->CardSlotUnderneath->SetVisibility(ESlateVisibility::HitTestInvisible);
		}
	}


	TickReinforcementUI();
	TickCardSetsUI();
	
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
UBuildingPlacementButton* UMainGameUI::AddCard(CardHandEnum cardHandEnum, CardStatus cardStatus, UWrapBox* buttonParent, CallbackEnum callbackEnum, int32 cardHandIndex, bool isMiniature)
{
	if (InterfacesInvalid()) {
		return nullptr;
	}
	
	UBuildingPlacementButton* cardButton = AddWidget<UBuildingPlacementButton>(isMiniature ? UIEnum::CardMini : UIEnum::BuildingPlacementButton);
	
	buttonParent->AddChild(cardButton);

	cardButton->PunInit(cardStatus, cardHandIndex, this, callbackEnum, cardHandEnum);

	//SetChildHUD(cardButton);

	cardButton->RefreshBuildingIcon(dataSource()->assetLoader());
	cardButton->SetCardStatus(cardHandEnum, false, false, IsRareCard(cardStatus.cardEnum));

	return cardButton;
}

void UMainGameUI::ResetBottomMenuDisplay()
{
	SetButtonImage(BuildMenuTogglerImage, false);
	SetButtonImage(GatherImage, false);
	//SetButtonImage(DemolishImage, false);
	//SetButtonImage(StatsImage, false);

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

	if (!wasActive) 
	{
		// Refresh building buttons
		std::vector<CardEnum> buildingEnums = simulation().unlockSystem(playerId())->unlockedBuildings();

		auto cardSys = simulation().cardSystem(playerId());
		int32 moneyCap32 = simulation().moneyCap32(playerId());
		int32 influence = simulation().influence(playerId());

		auto refreshPermanentCard = [&](UBuildingPlacementButton* cardButton)
		{
			int32 cardPrice = cardSys.GetCardPrice(cardButton->cardStatus.cardEnum);
			bool hasEnoughResource;
			if (cardSys.GetCardPriceTokenEnum(cardButton->cardStatus.cardEnum) == ResourceEnum::Money) {
				hasEnoughResource = moneyCap32 < cardPrice;
			} else {
				hasEnoughResource = influence < cardPrice;
			}
			cardButton->SetCardStatus(CardHandEnum::PermanentHand, false, cardPrice > 0 && hasEnoughResource);
			cardButton->SetPrice(cardPrice, cardSys.GetCardPriceTokenEnum(cardButton->cardStatus.cardEnum));
		};

		// erase buildingEnums that already have buttons
		for (int i = BuildingMenuWrap->GetChildrenCount(); i --> 0;) 
		{
			auto cardButton = Cast<UBuildingPlacementButton>(BuildingMenuWrap->GetChildAt(i));
			auto found = find(buildingEnums.begin(), buildingEnums.end(), cardButton->cardEnum());
			if (found != buildingEnums.end()) { // found old card, reusing it...
				buildingEnums.erase(found);

				// Make sure to update old card (for exclamation)
				cardButton->SetCardStatus(CardHandEnum::PermanentHand, false, false);
				refreshPermanentCard(cardButton);
			}
		}

		
		// Create the rest of the buttons
		for (CardEnum buildingEnum : buildingEnums) {
			auto cardButton = AddCard(CardHandEnum::PermanentHand, CardStatus(buildingEnum, 1), BuildingMenuWrap, CallbackEnum::SelectPermanentCard, -1, true);
			refreshPermanentCard(cardButton);
		}

		// Always swap Demolish to the back
		UBuildingPlacementButton* demolishButton = nullptr;
		for (int i = BuildingMenuWrap->GetChildrenCount(); i-- > 0;) {
			auto cardButton = Cast<UBuildingPlacementButton>(BuildingMenuWrap->GetChildAt(i));
			if (cardButton->cardEnum() == CardEnum::Demolish) {
				demolishButton = cardButton;
				BuildingMenuWrap->RemoveChildAt(i);
				break;
			}
		}
		check(demolishButton);
		BuildingMenuWrap->AddChild(demolishButton);
		

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

	bool wasDemolishing = (inputSystemInterface()->PlacementBuildingInfo().placementType == PlacementType::Demolish);
	//ResetBottomMenuDisplay();

	inputSystemInterface()->CancelPlacement();
	if (!wasDemolishing) {
		inputSystemInterface()->StartDemolish();
	}
}

void UMainGameUI::ToggleCardInventoryButton()
{
	if (InterfacesInvalid()) return;

	CardInventorySizeBox->SetVisibility(CardInventorySizeBox->IsVisible() ? ESlateVisibility::Collapsed : ESlateVisibility::SelfHitTestInvisible);
}

void UMainGameUI::SelectPermanentCard(CardEnum buildingEnum)
{
	BuildingCardSystem& cardSys = simulation().cardSystem(playerId());
	
	ResourceEnum tokenEnum = cardSys.GetCardPriceTokenEnum(buildingEnum);
	int32 tokenCount = simulation().GetTokens(playerId(), tokenEnum);
	int32 cardPrice = simulation().cardSystem(playerId()).GetCardPrice(buildingEnum, tokenEnum);
	
	if (cardPrice > 0 && tokenCount < cardPrice) {
		simulation().AddPopupToFront(playerId(),
			FText::Format(
				LOCTEXT("NoMoneyToBuildCommonCard_Pop", "Not enough {0} to place {1}."),
				simulation().GetTokenIconRichText(tokenEnum),
				GetBuildingInfo(buildingEnum).name
			),
			ExclusiveUIEnum::BuildMenu, "PopupCannot"
		);
		return;
	}

	if (buildingEnum == CardEnum::IntercityRoad) {
		inputSystemInterface()->StartRoadPlacement(false, true);
	}
	else if (buildingEnum == CardEnum::IrrigationDitch) {
		inputSystemInterface()->StartIrrigationDitchPlacement();
	}
	else if (buildingEnum == CardEnum::Demolish) {
		inputSystemInterface()->StartDemolish();
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
		inputSystemInterface()->StartBuildingPlacement(CardStatus(buildingEnum, 1), false);

		// Noticed farm, no longer need exclamation on farm after this...
		if (buildingEnum == CardEnum::Farm) {
			simulation().parameters(playerId())->FarmNoticed = true;
		}
	}

	GetPunHUD()->CloseDescriptionUI();

	BuildMenuOverlay->SetVisibility(ESlateVisibility::Collapsed);
	SetButtonImage(BuildMenuTogglerImage, false);
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
	
	int32 moneyCap32 = simulation().moneyCap32(playerId());
	int32 rerollPrice = cardSystem.GetRerollPrice();
	if (moneyCap32 >= rerollPrice || rerollPrice == 0) {
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
	if (CardHand1SubmitButtonText->GetText().ToString() == FString("Submit"))
	{
		{
			int32 moneyNeeded = TokensNeededForTentativeBuy(ResourceEnum::Money);
			int32 currentMoney = simulation().GetTokens(playerId(), ResourceEnum::Money); // simulation().moneyCap32(playerId());
			if (moneyNeeded > 0 && moneyNeeded > currentMoney)
			{
				simulation().AddPopupToFront(playerId(),
					LOCTEXT("NotEnoughMoneyToBuyCard_Pop", "Not enough money to purchase the cards."),
					ExclusiveUIEnum::CardHand1, "PopupCannot"
				);
				return;
			}
		}

		{
			int32 influenceNeeded = TokensNeededForTentativeBuy(ResourceEnum::Influence);
			int32 currentInfluence = simulation().GetTokens(playerId(), ResourceEnum::Influence);
			if (influenceNeeded > 0 && influenceNeeded > currentInfluence)
			{
				simulation().AddPopupToFront(playerId(),
					LOCTEXT("NotEnoughInfluenceToBuyCard_Pop", "Not enough influence to purchase the cards."),
					ExclusiveUIEnum::CardHand1, "PopupCannot"
				);
				return;
			}
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
			command->cardEnum = cardButton->cardEnum();
			command->objectId = rareHandObjectId;
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

	inputSystemInterface()->StartBuildingPlacement(CardStatus(skillEnum, 1), false);
}

void UMainGameUI::RightMouseUp()
{
	if (BuildMenuOverlay->GetVisibility() != ESlateVisibility::Collapsed) {
		dataSource()->Spawn2DSound("UI", "UIWindowClose");
	}

	ResetBottomMenuDisplay();
}

bool UMainGameUI::EscDown()
{
	bool isClosingUI = false;
	if (BuildMenuOverlay->IsVisible()) {
		dataSource()->Spawn2DSound("UI", "UIWindowClose");
		isClosingUI = true;
	}

	// Bottom Menu
	if (GatherSettingsOverlay->IsVisible() ||
		BuildMenuOverlay->IsVisible()) {
		isClosingUI = true;
	}
	ResetBottomMenuDisplay();

	// Esc down will also quit any card menu
	if (CardHand1Overlay->IsVisible() ||
		ConverterCardHandOverlay->IsVisible()) {
		isClosingUI = true;
	}
	ClickCardHand1CancelButton();
	ClickConverterCardHandCancelButton();

	return isClosingUI;
}

void UMainGameUI::CallBack1(UPunWidget* punWidgetCaller, CallbackEnum callbackEnum)
{
	UBuildingPlacementButton* cardButton = CastChecked<UBuildingPlacementButton>(punWidgetCaller);
	int32 cardHandIndex = cardButton->cardHandIndex;

	CardStatus cardStatus = cardButton->cardStatus;
	CardEnum buildingEnum = cardButton->cardEnum();
	
	auto& cardSystem = simulation().cardSystem(playerId());

	// Permanent cards
	if (callbackEnum == CallbackEnum::SelectPermanentCard)
	{
		SelectPermanentCard(buildingEnum);
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
			ResourceEnum cardPriceTokenEnum = cardSystem.GetCardPriceTokenEnum(buildingEnum);
			int32 resourceAvailable = simulation().GetTokens(playerId(), cardPriceTokenEnum) - TokensNeededForTentativeBuy(cardPriceTokenEnum);
			if (cardHandIndex != -1 && resourceAvailable < cardSystem.GetCardPrice(buildingEnum, cardPriceTokenEnum)) 
			{
				if (cardPriceTokenEnum == ResourceEnum::Money) {
					simulation().AddPopupToFront(playerId(),
						LOCTEXT("NotEnoughMoneyToSelectCard_Pop", "Not enough money to purchase the card."),
						ExclusiveUIEnum::CardHand1, "PopupCannot"
					);
				}
				else {
					simulation().AddPopupToFront(playerId(),
						LOCTEXT("NotEnoughInfluenceToSelectCard_Pop", "Not enough influence to purchase the card."),
						ExclusiveUIEnum::CardHand1, "PopupCannot"
					);
				}
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
			
			if (!cardSystem.CanAddCardsToBoughtHandOrInventory_CheckHand1Reserve(buildingEnum)) {
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
			// Deselect old reservation
			for (size_t i = 0; i < cardsRareHandReserved.size(); i++) {
				if (cardsRareHandReserved[i]) {
					cardSystem.SetRareHandCardReservation(i, false);
				}
			}

			// Check if we reached hand limit
			if (!IsPermanentTownBonus(buildingEnum) &&
				!IsPermanentGlobalBonus(buildingEnum))
			{
				if (!simulation().cardSystem(playerId()).CanAddCardsToBoughtHandOrInventory(buildingEnum))
				{
					simulation().AddPopupToFront(playerId(),
						LOCTEXT("ReachedHandLimitRare_Pop", "Reached hand limit for bought cards.<space>Please sell or use some cards on your hand, then choose a rare card prize again."),
						ExclusiveUIEnum::RareCardHand, "PopupCannot"
					);
					return;
				}
			}

			cardSystem.SetRareHandCardReservation(cardHandIndex, true);
		}
		return;
	}

	if (callbackEnum == CallbackEnum::SelectConverterCard)
	{
		// Check if there is enough tokens...
		ResourceEnum tokenEnum = cardSystem.GetCardPriceTokenEnum(buildingEnum);
		int32 tokenCount = simulation().GetTokens(playerId(), tokenEnum);
		
		if (tokenCount < cardSystem.GetCardPrice(buildingEnum, tokenEnum)) 
		{
			simulation().AddPopupToFront(playerId(), 
				FText::Format(
					LOCTEXT("NotEnoughMoneyWildCard_Pop", "Not enough {0}. Need to pay the building price to use wild card."),
					simulation().GetTokenIconRichText(tokenEnum)
				), 
				ExclusiveUIEnum::ConverterCardHand, "PopupCannot"
			);
			return;
		}

		dataSource()->Spawn2DSound("UI", "CardDeal");
		ConverterCardHandOverlay->SetVisibility(ESlateVisibility::Collapsed);
		cardSystem.converterCardState = ConverterCardUseState::SubmittedUI;

		inputSystemInterface()->StartBuildingPlacement(cardButton->cardStatus, false, cardSystem.wildCardEnumUsed);

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

	/*
	 * SelectBoughtCard
	 */
	if (callbackEnum == CallbackEnum::SelectBoughtCard)
	{
		DescriptionUIState descriptionUIState = simulation().descriptionUIState();

		//! Move To Card Inventory
		if (CardInventorySizeBox->IsVisible())
		{
			if (cardSystem.CanAddCardsToCardInventory(buildingEnum)) 
			{
				FVector2D initialPosition = GetViewportPosition(cardButton->GetCachedGeometry());

				auto command = make_shared<FUseCard>();
				command->callbackEnum = CallbackEnum::CardInventorySlotting;
				command->cardStatus = cardButton->cardStatus;
				command->variable1 = dataSource()->isShiftDown() ? command->cardStatus.stackSize : 1;

				command->SetPosition(initialPosition);
				networkInterface()->SendNetworkCommand(command);
			}
			return;
		}

		//! Collection Cards
		auto trySlotCard = [&](CallbackEnum callbackEnumIn, CardSetTypeEnum variable2)
		{
			FVector2D initialPosition = GetViewportPosition(cardButton->GetCachedGeometry());

			auto command = make_shared<FUseCard>();
			command->callbackEnum = callbackEnumIn;
			command->cardStatus = cardButton->cardStatus;
			command->variable1 = dataSource()->isShiftDown() ? command->cardStatus.stackSize : 1;
			command->variable2 = static_cast<int32>(variable2);

			command->SetPosition(initialPosition);
			networkInterface()->SendNetworkCommand(command);
		};
		
		if (IsZooAnimalCard(buildingEnum))
		{
			if (CardSetsUIOverlay->IsVisible() && cardSetTypeEnum == CardSetTypeEnum::Zoo) {
				trySlotCard(CallbackEnum::CardSetSlotting, CardSetTypeEnum::Zoo);
				return;
			}
			//else {
			//	simulation().AddPopupToFront(playerId(),
			//		LOCTEXT("ZooCardNeedZoo_Pop", "Animal Cards should be added to Zoo Slots."),
			//		ExclusiveUIEnum::None, "PopupCannot"
			//	);
			//}
			//return;
		}
		if (IsArtifactCard(buildingEnum))
		{
			if (CardSetsUIOverlay->IsVisible() && cardSetTypeEnum == CardSetTypeEnum::Museum) {
				trySlotCard(CallbackEnum::CardSetSlotting, CardSetTypeEnum::Museum);
			}
			else {
				simulation().AddPopupToFront(playerId(),
					LOCTEXT("ArtifactNeedMuseum_Pop", "Artifact Cards should be added to Museum Slots."),
					ExclusiveUIEnum::CardSetsUI, "PopupCannot"
				);
			}
			return;
		}
		if (CardSetsUIOverlay->IsVisible() && cardSetTypeEnum == CardSetTypeEnum::CardCombiner)
		{
			if (buildingEnum == CardEnum::ProductivityBook ||
				buildingEnum == CardEnum::SustainabilityBook ||
				buildingEnum == CardEnum::Motivation ||
				buildingEnum == CardEnum::Passion)
			{
				trySlotCard(CallbackEnum::CardSetSlotting, CardSetTypeEnum::CardCombiner);
				return;
			}
		}
		

		// Archive take any card
		if (descriptionUIState.objectType == ObjectTypeEnum::Building)
		{
			int32 buildingId = descriptionUIState.objectId;
			Building& building = simulation().building(buildingId);

			if (building.isEnum(CardEnum::Archives))
			{
				// Card Slots Full
				if (building.IsCardSlotsFull()) {
					simulation().AddPopupToFront(playerId(),
						LOCTEXT("BldSlotAlreadyHasCard_Pop", "This Building's Card Slots are full. Remove a Card first by clicking on a Slotted Sard."),
						ExclusiveUIEnum::None, "PopupCannot"
					);
					return;
				}
				
				FVector2D initialPosition = GetViewportPosition(cardButton->GetCachedGeometry());

				auto command = make_shared<FUseCard>();
				command->callbackEnum = CallbackEnum::ArchivesSlotting;
				command->cardStatus = cardStatus;
				command->variable1 = buildingId; // Archives buildingId
				command->SetPosition(initialPosition);
				networkInterface()->SendNetworkCommand(command);

				return;
			}
		}

		/*
		 * Military Card
		 */
		if (ReinforcementOverlay->IsVisible() && !IsMilitaryCardEnum(buildingEnum))
		{
			simulation().AddPopupToFront(playerId(),
				LOCTEXT("MilitaryCardsDeploy_Pop", "Click on Military Cards to deploy them for battle.<space>This Card cannot be deployed."),
				ExclusiveUIEnum::DeployMilitaryUI, "PopupCannot"
			);
			return;
		}
		
		/*
		 * Non-Building Cards
		 */
		if (!IsBuildingCard(buildingEnum))
		{
			//! Military
			if (IsMilitaryCardEnum(buildingEnum))
			{
				PUN_LOG("Not Building Card %s", *GetBuildingInfo(buildingEnum).nameF());
				
				if (ReinforcementOverlay->IsVisible() && reinforcementProvinceId != -1)
				{
					auto addMililtaryCards = [&]()
					{
						std::vector<CardStatus>& cards = cardSystem.pendingMilitarySlotCards;
						int32 boughtCardCount = cardSystem.DisplayedBoughtCardCount(buildingEnum, false);

						if (boughtCardCount > 0)
						{
							int32 cardCount = 1;
							if (dataSource()->isCtrlDown()) {
								cardCount = boughtCardCount;
							}
							else if (dataSource()->isShiftDown()) {
								cardCount = std::min(10, boughtCardCount);
							}

							auto tryAddPendingMilitarySlotCards = [&]()
							{
								for (int32 i = 0; i < cards.size(); i++) {
									if (cards[i].cardEnum == buildingEnum) {
										cards[i].stackSize += cardCount;
										return;
									}
								}

								if (cards.size() < ReinforcementSlots->GetChildrenCount()) {
									cards.push_back(CardStatus(buildingEnum, cardCount));
								}
							};

							tryAddPendingMilitarySlotCards();
						}
					};
					
					if (simulation().provinceSystem().provinceIsCoastal(reinforcementProvinceId))
					{
						if (IsMilitaryCardEnum(buildingEnum))
						{
							addMililtaryCards();
						}
						else {
							simulation().AddPopupToFront(playerId(),
								LOCTEXT("NeedLandMilitaryCard_Pop", "Requires Military Cards to be deployed."),
								ExclusiveUIEnum::None, "PopupCannot"
							);
						}
					}
					else
					{
						if (IsLandMilitaryCardEnum(buildingEnum))
						{
							addMililtaryCards();
							/*std::vector<CardStatus>& cards = cardSystem.pendingMilitarySlotCards;
							int32 boughtCardCount = cardSystem.DisplayedBoughtCardCount(buildingEnum, false);

							if (boughtCardCount > 0)
							{
								int32 cardCount = 1;
								if (dataSource()->isCtrlDown()) {
									cardCount = boughtCardCount;
								}
								else if (dataSource()->isShiftDown()) {
									cardCount = std::min(10, boughtCardCount);
								}

								auto tryAddPendingMilitarySlotCards = [&]()
								{
									for (int32 i = 0; i < cards.size(); i++) {
										if (cards[i].cardEnum == buildingEnum) {
											cards[i].stackSize += cardCount;
											return;
										}
									}

									if (cards.size() < ReinforcementSlots->GetChildrenCount()) {
										cards.push_back(CardStatus(buildingEnum, cardCount));
									}
								};

								tryAddPendingMilitarySlotCards();
							}*/
						}
						else {
							simulation().AddPopupToFront(playerId(),
								LOCTEXT("NeedLandMilitaryCard_Pop", "This battle requires Land Military Cards to be deployed."),
								ExclusiveUIEnum::DeployMilitaryUI, "PopupCannot"
							);
						}
					}
					
					return;
				}

				simulation().AddPopupToFront(playerId(),
					LOCTEXT("MilitaryCardsAreUsedInBattle_Pop", "Military Unit Cards are used in battles.<space>Deploy them when you defend or attack your enemy."),
					ExclusiveUIEnum::DeployMilitaryUI, "PopupCannot"
				);
				return;
			}

			/*
			 * Spell
			 */
			if (IsSpellCard(buildingEnum)) 
			{
				// Zoo Slot
				if (IsAnimalCard(buildingEnum)) {
					if (descriptionUIState.objectType == ObjectTypeEnum::Building)
					{
						Building& building = simulation().building(descriptionUIState.objectId);

						if (building.playerId() == playerId() &&
							building.CanAddSlotCard())
						{
							FVector2D initialPosition = GetViewportPosition(cardButton->GetCachedGeometry());

							auto command = make_shared<FUseCard>();
							command->cardStatus = cardStatus;
							command->variable1 = descriptionUIState.objectId;
							command->SetPosition(initialPosition);
							networkInterface()->SendNetworkCommand(command);
							return;
						}
					}
				}
				
				inputSystemInterface()->StartBuildingPlacement(cardButton->cardStatus, true);
				return;
			}


			/*
			 * Use Town Slot Card
			 */
			if (IsTownSlotCard(buildingEnum)) 
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
							command->cardStatus = cardStatus;
							command->SetPosition(initialPosition);
							networkInterface()->SendNetworkCommand(command);

							// Card initial animation
							//auto animationCard = AddAnimationCard(buildingEnum);
							//animationCard->StartAnimation(initialPosition, transitionPosition, CardAnimationEnum::ToGlobalSlot, Time::TicksPerSecond / 10);
							//AnimatedCardOverlay->AddChild(animationCard);

							//cardButton->SetVisibility(ESlateVisibility::Hidden);
							return;
						}
						else {
							simulation().AddPopupToFront(playerId(),
								LOCTEXT("NotEnoughSlotUnslotFirst_Pop", "Not enough slot. Unslot a card in this building first."),
								ExclusiveUIEnum::None, "PopupCannot"
							);
						}
					}
				}

				simulation().AddPopupToFront(playerId(), 
					LOCTEXT("TownhallCardNeedsTownhall_Pop", "Town-slot card must be slotted to the townhall. Click the townhall to open its panel before slotting the card."),
					ExclusiveUIEnum::None, "PopupCannot"
				);
				
				return;
			}

			/*
			 * Use Building Slot Card
			 */
			if (IsBuildingSlotCard(buildingEnum)) 
			{
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
							LOCTEXT("CannotInsertBldSlotToTownhall_Pop", "You cannot insert a building-slot card into the townhall. Townhall's card slot requires a town-slot card."),
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
						if ((buildingEnum == CardEnum::SustainabilityBook || buildingEnum == CardEnum::SustainabilityBook2) &&
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
						command->cardStatus = cardStatus;
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
				command->townId = currentTownId();
				command->cardStatus = cardStatus;

				auto& townResourceSys = simulation().resourceSystem(command->townId);

				auto sendCommand = [&]() {
					networkInterface()->SendNetworkCommand(command);
					cardButton->SetVisibility(ESlateVisibility::Hidden);
				};

				auto sendCommandWithWarning = [&](FText text) {
					networkInterface()->ShowConfirmationUI(text, command);
				};

				
				if (buildingEnum == CardEnum::SellFood) {
					sendCommandWithWarning(LOCTEXT("SellFood_Ask", "Are you sure you want to sell half of this city's food?"));
				} 
				else if (buildingEnum == CardEnum::BuyWood) 
				{
					int32 cost = GetResourceInfo(ResourceEnum::Wood).basePrice;
					int32 money = simulation().moneyCap32(playerId());

					if (money >= cost)
					{
						int32 amountToBuy = money / 2 / cost;
						amountToBuy = min(amountToBuy, 1000);

						if (townResourceSys.CanAddResourceGlobal(ResourceEnum::Wood, amountToBuy))
						{
							sendCommandWithWarning(
								FText::Format(
									LOCTEXT("BuyWood_Ask", "Are you sure you want to spend {0}<img id=\"Coin\"/> to buy {1} wood?"),
									TEXT_NUM(amountToBuy * cost),
									TEXT_NUM(amountToBuy)
								)
							);
						}
						else {
							simulation().AddPopupToFront(playerId(),
								FText::Format(LOCTEXT("BuyWoodNoStorageFit_Pop", "Not enough storage space to fit {0} wood in this city."), TEXT_NUM(amountToBuy)),
								ExclusiveUIEnum::None, "PopupCannot"
							);
						}
					}
					else {
						simulation().AddPopupToFront(playerId(),
							LOCTEXT("Not Enough Money", "Not Enough Money"),
							ExclusiveUIEnum::None, "PopupCannot"
						);
					}
				}
				else if (IsCrateCard(buildingEnum))
				{
					ResourcePair resourcePair = GetCrateResource(buildingEnum);
					
					if (townResourceSys.CanAddResourceGlobal(resourcePair.resourceEnum, resourcePair.count)) {
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
				if (cardButton->cardStatus.stackSize > 1 &&
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

			
			GetPunHUD()->CloseDescriptionUI();
			return;
		} // End: Non-Building Cards

		//// Special cases
		//if (buildingEnum == BuildingEnum::BeerBreweryFamous) {
		//	if (simulation().buildingCount(playerId(), BuildingEnum::BeerBrewery) < 4) {
		//		simulation().AddPopupToFront(playerId(), "Build 4 beer breweries before placing the famous beer brewery.");
		//		GetPunHUD()->CloseDescriptionUI();
		//		return;
		//	}
		//}

		inputSystemInterface()->StartBuildingPlacement(cardButton->cardStatus, true);
		
		GetPunHUD()->CloseDescriptionUI();
		return;
	}
	
	/*
	 * SelectInventorySlotCard
	 */
	if (callbackEnum == CallbackEnum::SelectInventorySlotCard)
	{
		if (cardButton->cardStatus.cardEnum != CardEnum::None)
		{
			auto command = make_shared<FUseCard>();
			command->callbackEnum = CallbackEnum::SelectInventorySlotCard;
			command->cardStatus = cardButton->cardStatus;
			command->variable1 = dataSource()->isShiftDown() ? simulation().cardSystem(playerId()).cardInventory().size() : 1;

			networkInterface()->SendNetworkCommand(command);
		}
		
		return;
	}

	/*
	 * SelectCardSetSlotCard
	 */
	if (callbackEnum == CallbackEnum::SelectCardSetSlotCard)
	{
		auto command = make_shared<FUseCard>();
		command->callbackEnum = CallbackEnum::SelectCardSetSlotCard;
		command->cardStatus = cardButton->cardStatus;
		command->variable2 = cardButton->callbackVar1; // CardSetTypeEnum

		networkInterface()->SendNetworkCommand(command);

		return;
	}

	/*
	 * SelectDeployMilitarySlotCard,
	 */
	if (callbackEnum == CallbackEnum::SelectDeployMilitarySlotCard)
	{
		std::vector<CardStatus>& cards = cardSystem.pendingMilitarySlotCards;
		CppUtils::RemoveIf(cards, [&](CardStatus& cardIn) { return cardIn.cardEnum == cardButton->cardStatus.cardEnum; });

		return;
	}

	/*
	 * Sell Card
	 */
	if (callbackEnum == CallbackEnum::SellCard)
	{
		auto command = make_shared<FSellCards>();
		command->cardStatus = cardButton->cardStatus;
		command->isShiftDown = dataSource()->isShiftDown();

		auto& cardSys = simulation().cardSystem(playerId());
		ResourceEnum tokenEnum = cardSys.GetCardPriceTokenEnum(command->cardStatus.cardEnum);
		int32 cardPrice = cardSys.GetCardPrice(command->cardStatus.cardEnum, tokenEnum);

		if (command->isShiftDown)
		{
			int32 sellCount = command->cardStatus.stackSize;
			networkInterface()->ShowConfirmationUI(
				FText::Format(
					LOCTEXT("SellCardSure_Pop", "Are you sure you want to sell {0} {1} {0}|plural(one=Card,other=Cards) for {2}{3}?"),
					sellCount,
					GetBuildingInfo(command->cardStatus.cardEnum).name,
					TEXT_NUM(cardPrice * sellCount),
					simulation().GetTokenIconRichText(tokenEnum)
				),
				command
			);
		}
		else {
			networkInterface()->ShowConfirmationUI(
				FText::Format(
					LOCTEXT("SellCardSure_Pop", "Are you sure you want to sell {0} for {1}{2}?"),
					GetBuildingInfo(command->cardStatus.cardEnum).name,
					TEXT_NUM(cardPrice),
					simulation().GetTokenIconRichText(tokenEnum)
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


int32 UMainGameUI::TokensNeededForTentativeBuy(ResourceEnum resourceEnum)
{
	int32 tokensNeeded = 0;
	auto& cardSystem = simulation().cardSystem(playerId());
	std::vector<bool> reserveStatus = cardSystem.GetHand1ReserveStatus();
	
	//TArray<UWidget*> cardButtons = CardHand1Box->GetAllChildren();

	const std::vector<CardEnum>& cardHand = cardSystem.GetHand();
	
	int32 loopSize = min(cardHand.size(), reserveStatus.size()); // TODO: not needed?
	for (int i = 0; i < loopSize; i++) {
		if (reserveStatus[i]) {
			tokensNeeded += cardSystem.GetCardPrice(cardHand[i], resourceEnum);
		}
	}
	return tokensNeeded;
}


void UMainGameUI::OnClickReinforcementSubmitButton()
{
	const std::vector<CardStatus>& cards = simulation().cardSystem(playerId()).pendingMilitarySlotCards;

	if (cards.size() == 0) {
		simulation().AddPopupToFront(playerId(),
			LOCTEXT("MilitaryCardsDeploy_NeedUnits_Pop", "No Military Cards were selected for deployment."),
			ExclusiveUIEnum::DeployMilitaryUI, "PopupCannot"
		);
		return;
	}

	auto command = make_shared<FClaimLand>();
	command->claimEnum = reinforcementCallbackEnum;
	command->provinceId = reinforcementProvinceId;
	PUN_CHECK(command->provinceId != -1);

	for (int32 i = 0; i < cards.size(); i++) {
		command->cardEnums.Add(static_cast<int32>(cards[i].cardEnum));
		command->cardCount.Add(cards[i].stackSize);
	}

	networkInterface()->SendNetworkCommand(command);

	if (reinforcementCallbackEnum == CallbackEnum::RaidBattle) {
		simulation().cardSystem(playerId()).pendingHiddenBoughtHandCards.push_back(CardStatus(CardEnum::Raid, 1));
	}

	CloseReinforcementUI(false);
}


#undef LOCTEXT_NAMESPACE