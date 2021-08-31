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
		CloseButton->OnClicked.AddUniqueDynamic(this, &UTrainUnitsUI::CloseUI);
		XCloseButton->CoreButton->OnClicked.AddUniqueDynamic(this, &UTrainUnitsUI::CloseUI);
		
		auto setChildHUD = [&](UWrapBox* box)
		{
			for (int32 i = 0; i < box->GetChildrenCount(); i++) {
				auto cardButton = CastChecked<UBuildingPlacementButton>(box->GetChildAt(i));
				SetChildHUD(cardButton);
			}
		};

		setChildHUD(MeleeUnitsBox);
		setChildHUD(CavalryUnitsBox);
		setChildHUD(RangedUnitsBox);
		setChildHUD(SiegeUnitsBox);
		setChildHUD(NavalUnitsBox);

		setChildHUD(TrainingQueueBox);

		SetVisibility(ESlateVisibility::Collapsed);
	}

	// Fill in the cards
	UBuildingPlacementButton* SetupButton(UWrapBox* box, int32 index, CardEnum cardEnum, int32 cardCount = 1, CallbackEnum callbackEnum = CallbackEnum::TrainUnit)
	{
		auto cardButton = CastChecked<UBuildingPlacementButton>(box->GetChildAt(index));
		cardButton->PunInit(CardStatus(cardEnum, cardCount), 0, this, callbackEnum, CardHandEnum::TrainUnits);
		cardButton->SetCardStatus(CardHandEnum::TrainUnits, false, false);
		cardButton->RefreshBuildingIcon(assetLoader());
		cardButton->SellButton->SetVisibility(ESlateVisibility::Collapsed);
		cardButton->SetPrice(cardEnum != CardEnum::None ? GetBuildingInfo(cardEnum).baseCardPrice : 0);
		
		return cardButton;
	};

	void OpenUI(int32 townIdIn)
	{
		townId = townIdIn;
		SetVisibility(ESlateVisibility::SelfHitTestInvisible);

		SetupButton(MeleeUnitsBox, 0, CardEnum::Warrior);
		SetupButton(MeleeUnitsBox, 1, CardEnum::Swordman);
		SetupButton(MeleeUnitsBox, 2, CardEnum::Musketeer);
		SetupButton(MeleeUnitsBox, 3, CardEnum::Infantry);

		SetupButton(CavalryUnitsBox, 0, CardEnum::Knight);
		SetupButton(CavalryUnitsBox, 1, CardEnum::Tank);

		SetupButton(RangedUnitsBox, 0, CardEnum::Archer);
		SetupButton(RangedUnitsBox, 1, CardEnum::MachineGun);

		SetupButton(SiegeUnitsBox, 0, CardEnum::Artillery);
		SetupButton(SiegeUnitsBox, 1, CardEnum::Catapult);
		SetupButton(SiegeUnitsBox, 2, CardEnum::Cannon);

		SetupButton(NavalUnitsBox, 0, CardEnum::Galley);
		SetupButton(NavalUnitsBox, 1, CardEnum::Frigate);
		SetupButton(NavalUnitsBox, 2, CardEnum::Battleship);
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
				int32 maxPossibleTraining = simulation().moneyCap32(playerId()) / townManager->GetTrainUnitCost(command->cardStatus.cardEnum);
				if (maxPossibleTraining <= 0) {
					simulation().AddPopupToFront(playerId(),
						NSLOCTEXT("TrainUnits", "NotEnoughMoneyToTrainUnit", "Not enough money to train this unit."),
						ExclusiveUIEnum::TrainUnitsUI, "PopupCannot"
					);
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
		auto& townManager = simulation().townManager(townId);
		const std::vector<CardStatus>& trainingQueue = townManager.trainUnitsQueueDisplay();

		for (int32 i = 0; i < 5; i++) {
			CardStatus cardStatus = (i < trainingQueue.size()) ? trainingQueue[i] : CardStatus::None;

			auto cardButton = SetupButton(TrainingQueueBox, i, cardStatus.cardEnum, cardStatus.stackSize, CallbackEnum::CancelTrainUnit);
			cardButton->callbackVar1 = cardStatus.cardBirthTicks;
		}

		// clock
		if (trainingQueue.size() > 0) {
			TrainingClock->GetDynamicMaterial()->SetScalarParameterValue(FName("Fraction"), townManager.GetTrainingFraction());
			TrainingClock->SetVisibility(ESlateVisibility::Visible);
		} else {
			TrainingClock->SetVisibility(ESlateVisibility::Collapsed);
		}
	}

	
	UPROPERTY(meta = (BindWidget)) UVerticalBox* BackLineBox;

	UPROPERTY(meta = (BindWidget)) UTextBlock* FrontLineHeader;
	UPROPERTY(meta = (BindWidget)) UTextBlock* BackLineHeader;
	UPROPERTY(meta = (BindWidget)) UTextBlock* NavalHeader;

	UPROPERTY(meta = (BindWidget)) UTextBlock* MeleeSubheader;
	UPROPERTY(meta = (BindWidget)) UTextBlock* CavalrySubheader;
	UPROPERTY(meta = (BindWidget)) UTextBlock* RangedSubheader;
	UPROPERTY(meta = (BindWidget)) UTextBlock* SiegeSubheader;

	UPROPERTY(meta = (BindWidget)) UWrapBox* MeleeUnitsBox;
	UPROPERTY(meta = (BindWidget)) UWrapBox* CavalryUnitsBox;
	UPROPERTY(meta = (BindWidget)) UWrapBox* RangedUnitsBox;
	UPROPERTY(meta = (BindWidget)) UWrapBox* SiegeUnitsBox;
	UPROPERTY(meta = (BindWidget)) UWrapBox* NavalUnitsBox;

	UPROPERTY(meta = (BindWidget)) UVerticalBox* TrainingQueueOuterBox;
	UPROPERTY(meta = (BindWidget)) UWrapBox* TrainingQueueBox;
	UPROPERTY(meta = (BindWidget)) UImage* TrainingClock;

	UPROPERTY(meta = (BindWidget)) UButton* CloseButton;
	UPROPERTY(meta = (BindWidget)) UWGT_ButtonCpp* XCloseButton;

	int32 townId = 0;
};
