// Pun Dumnernchanvanit's

#pragma once

#include "PunWidget.h"
#include "PunEditableNumberBox.h"
#include "SendImmigrantsUI.generated.h"

/**
 * 
 */
UCLASS()
class PROTOTYPECITY_API USendImmigrantsUI : public UPunWidget
{
	GENERATED_BODY()
public:
	UPROPERTY(meta = (BindWidget)) UComboBoxString* ImmigrantsFromTownDropdown;
	UPROPERTY(meta = (BindWidget)) URichTextBlock* ImmigrantsToTownText;
	UPROPERTY(meta = (BindWidget)) UPunEditableNumberBox* ImmigrantsTargetAmountAdults;
	UPROPERTY(meta = (BindWidget)) UPunEditableNumberBox* ImmigrantsTargetAmountChildren;

	UPROPERTY(meta = (BindWidget)) UButton* ConfirmButton;
	UPROPERTY(meta = (BindWidget)) UButton* CloseButton1;
	UPROPERTY(meta = (BindWidget)) UButton* CloseButton2;

	UPROPERTY(meta = (BindWidget)) URichTextBlock* AdultAvailableText;
	UPROPERTY(meta = (BindWidget)) URichTextBlock* ChildrenAvailableText;

	int32 toTownId = -1;

	void PunInit()
	{
		ImmigrantsFromTownDropdown->OnSelectionChanged.Clear();
		//ImmigrantsFromTownDropdown->OnSelectionChanged.AddDynamic(this, &USendImmigrantsUI::OnDropDownChanged);

		SetChildHUD(ImmigrantsTargetAmountAdults);
		SetChildHUD(ImmigrantsTargetAmountChildren);
		ImmigrantsTargetAmountAdults->minAmount = 0;
		ImmigrantsTargetAmountChildren->minAmount = 0;
		ImmigrantsTargetAmountAdults->incrementMultiplier = 1;
		ImmigrantsTargetAmountChildren->incrementMultiplier = 1;

		BUTTON_ON_CLICK(ConfirmButton, this, &USendImmigrantsUI::OnClickConfirmButton);
		BUTTON_ON_CLICK(CloseButton1, this, &USendImmigrantsUI::OnClickCloseButton);
		BUTTON_ON_CLICK(CloseButton2, this, &USendImmigrantsUI::OnClickCloseButton);

		SetVisibility(ESlateVisibility::Collapsed);
	}

	void OpenUI(int32 townIdIn)
	{
		toTownId = townIdIn;

		SetText(ImmigrantsToTownText, simulation().townNameT(toTownId));

		// Dropdown
		const auto& townIds = simulation().GetTownIds(simulation().townPlayerId(toTownId));
		ImmigrantsFromTownDropdown->ClearOptions();
		for (int32 townIdTemp : townIds) {
			if (townIdTemp != toTownId) {
				ImmigrantsFromTownDropdown->AddOption(simulation().townNameT(townIdTemp).ToString());
			}
		}
		ImmigrantsFromTownDropdown->SetSelectedOption(ImmigrantsFromTownDropdown->GetOptionAtIndex(0));

		ImmigrantsTargetAmountAdults->amount = 0;
		ImmigrantsTargetAmountChildren->amount = 0;
		ImmigrantsTargetAmountAdults->UpdateText();
		ImmigrantsTargetAmountChildren->UpdateText();

		SetVisibility(ESlateVisibility::SelfHitTestInvisible);
	}

	void CloseUI()
	{
		SetVisibility(ESlateVisibility::Collapsed);
	}

	void TickUI()
	{
		if (IsVisible())
		{
			int32 fromTownId = simulation().FindTownIdFromName(playerId(), ImmigrantsFromTownDropdown->GetSelectedOption());
			auto& townManager = simulation().townManager(fromTownId);
			
			AdultAvailableText->SetText(FText::Format(
				INVTEXT("/{0}"), TEXT_NUM(townManager.adultPopulation())
			));
			ChildrenAvailableText->SetText(FText::Format(
				INVTEXT("/{0}"), TEXT_NUM(townManager.childPopulation())
			));

			ImmigrantsTargetAmountAdults->maxAmount = townManager.adultPopulation();
			ImmigrantsTargetAmountChildren->maxAmount = townManager.childPopulation();
		}
	}
	
	//UFUNCTION() void OnDropDownChanged(FString sItem, ESelectInfo::Type seltype)
	//{

	//}

#define LOCTEXT_NAMESPACE "USendImmigrantsUI"
	
	UFUNCTION() void OnClickConfirmButton()
	{
		auto& sim = simulation();

		if (ImmigrantsTargetAmountAdults->amount <= 0 &&
			ImmigrantsTargetAmountChildren->amount <= 0) 
		{
			sim.AddPopupToFront(playerId(), LOCTEXT("USendImmigrantsUI_amount0", "Please set the number of immigrants to send."), ExclusiveUIEnum::SendImmigrantsUI, "PopupCannot");
			return;
		}
		
		if (sim.IsValidTown(toTownId))
		{
			FString fromTownName = ImmigrantsFromTownDropdown->GetSelectedOption();
			int32 fromTownId = simulation().FindTownIdFromName(playerId(), fromTownName);

			auto command = make_shared<FGenericCommand>();
			command->genericCommandType = FGenericCommand::Type::SendImmigrants;
			command->intVar1 = static_cast<int>(fromTownId);
			command->intVar2 = static_cast<int>(toTownId);
			command->intVar3 = static_cast<int>(ImmigrantsTargetAmountAdults->amount);
			command->intVar4 = static_cast<int>(ImmigrantsTargetAmountChildren->amount);

			networkInterface()->SendNetworkCommand(command);
		}

		SetVisibility(ESlateVisibility::Collapsed);
	}
	UFUNCTION() void OnClickCloseButton() {
		SetVisibility(ESlateVisibility::Collapsed);
	}

#undef LOCTEXT_NAMESPACE
};