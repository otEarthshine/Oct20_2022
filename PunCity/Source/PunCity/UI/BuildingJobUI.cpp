// Fill out your copyright notice in the Description page of Project Settings.

#include "BuildingJobUI.h"
#include "HumanSlotIcon.h"
#include "Components/Button.h"
#include "Components/SizeBox.h"
#include "Components/HorizontalBox.h"
#include "PunCity/Simulation/Building.h"
#include <memory>

using namespace std;

void UBuildingJobUI::PunInit(int buildingId, bool isHouse)
{
	_buildingId = buildingId;

	ArrowUp->OnClicked.Clear();
	ArrowDown->OnClicked.Clear();
	PriorityButton->OnClicked.Clear();
	NonPriorityButton->OnClicked.Clear();
	DisabledButton->OnClicked.Clear();

	ArrowUp->OnClicked.AddDynamic(this, &UBuildingJobUI::ArrowUpButtonDown);
	ArrowDown->OnClicked.AddDynamic(this, &UBuildingJobUI::ArrowDownButtonDown);

	PriorityButton->OnClicked.AddDynamic(this, &UBuildingJobUI::PriorityButtonDown);
	NonPriorityButton->OnClicked.AddDynamic(this, &UBuildingJobUI::NonPriorityButtonDown);
	DisabledButton->OnClicked.AddDynamic(this, &UBuildingJobUI::DisabledButtonDown);

	PriorityButton->SetVisibility(ESlateVisibility::Collapsed);
	NonPriorityButton->SetVisibility(ESlateVisibility::Visible);
	DisabledButton->SetVisibility(ESlateVisibility::Collapsed);

	TradeButton->OnClicked.Clear();
	TradeButton->OnClicked.AddDynamic(this, &UBuildingJobUI::OnClickTradeButton);
	TradeButton->SetVisibility(ESlateVisibility::Collapsed);

	//if (isHouse)
	//{
	//	ArrowUp->SetVisibility(ESlateVisibility::Collapsed);
	//	ArrowDown->SetVisibility(ESlateVisibility::Collapsed);
	//	NonPriorityButton->SetVisibility(ESlateVisibility::Collapsed);
	//}
	SetShowHumanSlots(false, false);
	SetShowBar(false);

	ResourceCompletionIconBox->ClearChildren();
	OtherIconsBox->ClearChildren();

	ClockBox->SetVisibility(ESlateVisibility::Collapsed);

	_lastInputTime = -999.0f;
	_lastPriorityInputTime = -999.0f;
}

void UBuildingJobUI::SetSlots(int filledSlotCount, int allowedSlotCount, int slotCount, FLinearColor color, bool fromInput)
{
	// Don't set slots for a while after last user input
	if (!fromInput && UGameplayStatics::GetTimeSeconds(this) < _lastInputTime + NetworkInputDelayTime) {
		return;
	}
	
	_filledSlotCount = filledSlotCount;
	_allowedSlotCount = allowedSlotCount;
	_slotCount = slotCount;
	_slotColor = color;
	
	for (int i = 0; i < slotCount; i++) 
	{
		if (_humanIcons.Num() <= i) {
			auto humanIcon = AddWidget<UHumanSlotIcon>(UIEnum::JobHumanIcon);
			HumanSlots->AddChild(humanIcon);
			_humanIcons.Add(humanIcon);
		}

		_humanIcons[i]->SetVisibility(ESlateVisibility::SelfHitTestInvisible);
		_humanIcons[i]->SlotFiller->SetVisibility(i < filledSlotCount ? ESlateVisibility::SelfHitTestInvisible : ESlateVisibility::Hidden);
		_humanIcons[i]->SlotFillerImage->SetColorAndOpacity(color);//SetBrushTintColor(FSlateColor(tint));
		_humanIcons[i]->SlotCross->SetVisibility(i < allowedSlotCount ? ESlateVisibility::Hidden : ESlateVisibility::SelfHitTestInvisible);
	}

	// Deactivate unused meshes
	for (int i = slotCount; i < _humanIcons.Num(); i++) {
		_humanIcons[i]->SetVisibility(ESlateVisibility::Collapsed);
	}
}

void UBuildingJobUI::SetShowHumanSlots(bool isVisible, bool canManipulateOccupants, bool isTileBld)
{
	if (isVisible)
	{
		HumanSlotsUI->SetVisibility(ESlateVisibility::Visible);
		
		if (canManipulateOccupants) {
			PriorityEnum priority = building().priority();

			if (UGameplayStatics::GetTimeSeconds(this) > _lastPriorityInputTime + NetworkInputDelayTime) {
				SetPriorityButton(priority);
			}

			ArrowUp->SetVisibility(ESlateVisibility::Visible);
			ArrowDown->SetVisibility(ESlateVisibility::Visible);
		}
		else {
			PriorityButton->SetVisibility(ESlateVisibility::Collapsed);
			NonPriorityButton->SetVisibility(ESlateVisibility::Collapsed);
			DisabledButton->SetVisibility(ESlateVisibility::Collapsed);

			ArrowUp->SetVisibility(ESlateVisibility::Collapsed);
			ArrowDown->SetVisibility(ESlateVisibility::Collapsed);
		}
	}
	else
	{
		HumanSlotsUI->SetVisibility(ESlateVisibility::Collapsed);
	}

	
	if (isTileBld) {
		HumanSlotsUI->SetRenderScale(FVector2D(0.7, 0.7));
	} else {
		HumanSlotsUI->SetRenderScale(FVector2D(1, 1));
	}
}