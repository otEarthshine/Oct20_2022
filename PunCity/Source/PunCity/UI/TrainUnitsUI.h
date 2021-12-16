// Pun Dumnernchanvanit's

#pragma once

#include "BuildingPlacementButton.h"
#include "PunWidget.h"
#include "TrainUnitsUI.generated.h"

/**
 * 
 */
UCLASS()
class PROTOTYPECITY_API UTrainUnitsUI : public UPunWidget
{
	GENERATED_BODY()
public:

	void PunInit()
	{
		BUTTON_ON_CLICK(CloseButton, this, &UTrainUnitsUI::CloseUI);
		BUTTON_ON_CLICK(XCloseButton->CoreButton, this, &UTrainUnitsUI::CloseUI);
		
		auto setChildHUD = [&](UWrapBox* box)
		{
			for (int32 i = 0; i < box->GetChildrenCount(); i++) {
				auto cardButton = CastChecked<UBuildingPlacementButton>(box->GetChildAt(i));
				SetChildHUD(cardButton);
			}
		};

		setChildHUD(FrontlineUnitsBox);
		setChildHUD(BacklineUnitsBox);
		setChildHUD(NavyUnitsBox);

		setChildHUD(ActionCardsBox);

		setChildHUD(TrainingQueueBox);

		SetVisibility(ESlateVisibility::Collapsed);
	}

	// Fill in the cards
	UBuildingPlacementButton* SetupButton(UWrapBox* box, int32 index, CardEnum cardEnum, int32 cardCount = 1, CallbackEnum callbackEnum = CallbackEnum::TrainUnit)
	{
		auto cardButton = CastChecked<UBuildingPlacementButton>(box->GetChildAt(index));
		
		if (callbackEnum == CallbackEnum::TrainUnit && cardEnum == CardEnum::None) {
			cardButton->SetVisibility(ESlateVisibility::Collapsed);
		}
		else {
			cardButton->PunInit(CardStatus(cardEnum, cardCount), 0, this, callbackEnum, CardHandEnum::TrainUnits);
			cardButton->SetCardStatus(CardHandEnum::TrainUnits, false, false);
			cardButton->RefreshBuildingIcon(assetLoader());
			cardButton->SellButton->SetVisibility(ESlateVisibility::Collapsed);

			if (cardEnum == CardEnum::None) {
				cardButton->SetPrice(0, ResourceEnum::Money, 0);
			}
			else if (IsMilitaryCardEnum(cardEnum)) {
				MilitaryCardInfo info = GetMilitaryInfo(cardEnum);
				cardButton->SetPrice(info.resourceCost.count, info.resourceCost.resourceEnum, info.humanCost);
			}
			else {
				cardButton->SetPrice(GetBuildingInfo(cardEnum).baseCardPrice, BuildingCardSystem::GetCardPriceTokenEnum(cardEnum));
			}
			cardButton->SetVisibility(ESlateVisibility::Visible);
		}
		
		return cardButton;
	};

	void OpenUI(int32 townIdIn)
	{
		townId = townIdIn;
		SetVisibility(ESlateVisibility::SelfHitTestInvisible);
	}

	
	UFUNCTION() void CloseUI()
	{
		SetVisibility(ESlateVisibility::Collapsed);
	}

	virtual void CallBack1(UPunWidget* punWidgetCaller, CallbackEnum callbackEnum) override
	{
		auto cardButton = CastChecked<UBuildingPlacementButton>(punWidgetCaller);
		auto command = make_shared<FUseCard>();

		if (callbackEnum == CallbackEnum::TrainUnit ||
			callbackEnum == CallbackEnum::CancelTrainUnit)
		{
			command->townId = townId;
			command->callbackEnum = callbackEnum;
			command->cardStatus = cardButton->cardStatus;

			if (callbackEnum == CallbackEnum::TrainUnit) {
				command->variable1 = dataSource()->isShiftDown() ? 5 : 1;
			} else {
				command->variable1 = dataSource()->isShiftDown() ? cardButton->cardStatus.stackSize : 1;
			}

			TownManager* townManager = simulation().townManagerPtr(townId);

			// Warn if there isn't enough money
			if (callbackEnum == CallbackEnum::TrainUnit)
			{
				ResourcePair resourceCost = townManager->GetTrainUnitCost(command->cardStatus.cardEnum);

				if (PunSettings::IsOn("CheatFastBuild")) {
					resourceCost = ResourcePair(ResourceEnum::Money, 1);
				}
				
				int32 maxPossibleTraining = simulation().resourceCountTownWithMoney(townId, resourceCost.resourceEnum) / resourceCost.count;
				if (maxPossibleTraining <= 0) 
				{
					if (resourceCost.resourceEnum == ResourceEnum::Money) {
						simulation().AddPopupToFront(playerId(),
							NSLOCTEXT("TrainUnits", "NotEnoughMoneyToTrainUnit", "Not enough money to train this unit."),
							ExclusiveUIEnum::TrainUnitsUI, "PopupCannot"
						);
					}
					else {
						simulation().AddPopupToFront(playerId(),
							NSLOCTEXT("TrainUnits", "NotEnoughResourceToTrainUnit", "Not enough resource to train this unit."),
							ExclusiveUIEnum::TrainUnitsUI, "PopupCannot"
						);
					}
					return;
				}
				if (command->variable1 > maxPossibleTraining) {
					command->variable1 = maxPossibleTraining;
				}
			}

			command->variable2 = cardButton->callbackVar1; // hash for canceling the right stack

			townManager->AddPendingTrainCommand(*command);
			networkInterface()->SendNetworkCommand(command);
		}
		
	}

	
	void TickUI()
	{
		if (!IsVisible()) {
			return;
		}
		
		auto& townManager = simulation().townManager(townId);
		const std::vector<CardStatus>& trainingQueue = townManager.trainUnitsQueueDisplay();

		for (int32 i = 0; i < 5; i++) {
			CardStatus cardStatus = (i < trainingQueue.size()) ? trainingQueue[i] : CardStatus::None;

			auto cardButton = SetupButton(TrainingQueueBox, i, cardStatus.cardEnum, cardStatus.stackSize, CallbackEnum::CancelTrainUnit);
			cardButton->callbackVar1 = cardStatus.cardBirthTicks;
		}

		//! Training Clock
		if (trainingQueue.size() > 0) {
			TrainingClock->GetDynamicMaterial()->SetScalarParameterValue(FName("Fraction"), townManager.GetTrainingFraction());
			TrainingClock->SetVisibility(ESlateVisibility::SelfHitTestInvisible);
		} else {
			TrainingClock->SetVisibility(ESlateVisibility::Collapsed);
		}



		/*
		 * Military Training Buttons
		 */
		// TODO: Military resources: Food, Wood, Iron, Steel

		auto& sim = simulation();

		auto isResearched = [&](TechEnum techEnum) { return sim.IsResearched(playerId(), techEnum); };

		CardEnum infantryEnum = CardEnum::Warrior;
		if (isResearched(TechEnum::Infantry)) infantryEnum = CardEnum::Infantry;
		else if (isResearched(TechEnum::Musketeer)) infantryEnum = CardEnum::Musketeer;
		else if (isResearched(TechEnum::Swordsman)) infantryEnum = CardEnum::Swordsman;


		CardEnum cavalryEnum = CardEnum::None;
		if (isResearched(TechEnum::Tank)) cavalryEnum = CardEnum::Tank;
		else if (isResearched(TechEnum::Knight)) cavalryEnum = CardEnum::Knight;


		CardEnum conscriptEnum = isResearched(TechEnum::Infantry) ? CardEnum::Conscript : CardEnum::None;


		CardEnum rangedEnum = CardEnum::None;
		if (isResearched(TechEnum::MachineGun)) rangedEnum = CardEnum::MachineGun;
		else if (isResearched(TechEnum::Archer)) rangedEnum = CardEnum::Archer;


		CardEnum siegeEnum = CardEnum::None;
		if (isResearched(TechEnum::Artillery)) siegeEnum = CardEnum::Artillery;
		else if (isResearched(TechEnum::MilitaryEngineering2)) siegeEnum = CardEnum::Cannon;
		else if (isResearched(TechEnum::MilitaryEngineering1)) siegeEnum = CardEnum::Catapult;


		CardEnum navyEnum = CardEnum::None;
		if (isResearched(TechEnum::Battleship)) navyEnum = CardEnum::Battleship;
		else if (isResearched(TechEnum::MilitaryEngineering2)) navyEnum = CardEnum::Frigate;
		else if (isResearched(TechEnum::MilitaryEngineering1)) navyEnum = CardEnum::Galley;


		SetupButton(FrontlineUnitsBox, 0, infantryEnum);
		SetupButton(FrontlineUnitsBox, 1, cavalryEnum);
		SetupButton(FrontlineUnitsBox, 2, conscriptEnum);

		BacklineBox->SetVisibility(rangedEnum != CardEnum::None || siegeEnum != CardEnum::None ? ESlateVisibility::SelfHitTestInvisible : ESlateVisibility::Collapsed);
		SetupButton(BacklineUnitsBox, 0, rangedEnum);
		SetupButton(BacklineUnitsBox, 1, siegeEnum);

		NavyBox->SetVisibility(navyEnum != CardEnum::None ? ESlateVisibility::SelfHitTestInvisible : ESlateVisibility::Collapsed);
		SetupButton(NavyUnitsBox, 0, navyEnum);

		ActionBox->SetVisibility(sim.cardSystem(playerId()).HasBoughtMilitaryCard() || trainingQueue.size() > 0 ? ESlateVisibility::SelfHitTestInvisible : ESlateVisibility::Collapsed);
		SetupButton(ActionCardsBox, 0, CardEnum::Raid);
	}

	
	UPROPERTY(meta = (BindWidget)) UVerticalBox* BacklineBox;
	UPROPERTY(meta = (BindWidget)) UVerticalBox* NavyBox;
	UPROPERTY(meta = (BindWidget)) UVerticalBox* ActionBox;

	UPROPERTY(meta = (BindWidget)) UWrapBox* FrontlineUnitsBox;
	UPROPERTY(meta = (BindWidget)) UWrapBox* BacklineUnitsBox;
	UPROPERTY(meta = (BindWidget)) UWrapBox* NavyUnitsBox;
	UPROPERTY(meta = (BindWidget)) UWrapBox* ActionCardsBox;
	

	UPROPERTY(meta = (BindWidget)) UVerticalBox* TrainingQueueOuterBox;
	UPROPERTY(meta = (BindWidget)) UWrapBox* TrainingQueueBox;
	UPROPERTY(meta = (BindWidget)) UImage* TrainingClock;

	UPROPERTY(meta = (BindWidget)) UButton* CloseButton;
	UPROPERTY(meta = (BindWidget)) UWGT_ButtonCpp* XCloseButton;

	int32 townId = 0;
};
