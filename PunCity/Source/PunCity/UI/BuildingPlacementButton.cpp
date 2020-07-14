// Fill out your copyright notice in the Description page of Project Settings.

#include "BuildingPlacementButton.h"
#include "../Simulation/GameSimulationCore.h"

FReply UBuildingPlacementButton::NativeOnMouseButtonDown(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent)
{
	position = GetViewportPosition(InGeometry);
	PUN_LOG("NativeOnMouseButtonDown: GetAbsolutePosition %f,%f", position.X, position.Y);
	
	PUN_CHECK(_callbackParent);
	if (!isAnimating() &&
		InMouseEvent.GetEffectingButton() == EKeys::LeftMouseButton) 
	{
		_callbackParent->CallBack1(this, _callbackEnum);

		dataSource()->Spawn2DSound("UI", "CardClick");
	}
	return FReply::Handled();
}
FReply UBuildingPlacementButton::NativeOnMouseButtonDoubleClick(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent)
{
	return NativeOnMouseButtonDown(InGeometry, InMouseEvent);
}

void UBuildingPlacementButton::OnSellButtonClicked()
{
	PUN_CHECK(_callbackParent);
	_callbackParent->CallBack1(this, CallbackEnum::SellCard);
}