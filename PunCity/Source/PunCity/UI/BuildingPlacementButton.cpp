// Fill out your copyright notice in the Description page of Project Settings.

#include "BuildingPlacementButton.h"
#include "../Simulation/GameSimulationCore.h"

#define LOCTEXT_NAMESPACE "BuildingPlacementButton"

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


void UBuildingPlacementButton::PunInit(CardEnum buildingEnumIn, int32 cardHandIndexIn, int32 buildingLvlIn, int32 stackSize, UPunWidget* callbackParent, CallbackEnum callbackEnum, bool isMiniature)
{
	buildingEnum = buildingEnumIn;
	cardHandIndex = cardHandIndexIn;
	buildingLvl = buildingLvlIn;
	_callbackParent = callbackParent;
	_callbackEnum = callbackEnum;
	_isMiniature = isMiniature;

	BldInfo info = GetBuildingInfo(buildingEnum);

	TArray<FText> args;
	ADDTEXT__(info.GetDescription());
	if (IsTownSlotCard(buildingEnum)) {
		ADDTEXT_(INVTEXT("\n<Gray>({0})</>"), LOCTEXT("town slot", "town slot"));
	}
	if (IsBuildingSlotCard(buildingEnum)) {
		ADDTEXT_(INVTEXT("\n<Gray>({0})</>"), LOCTEXT("building slot", "building slot"));
	}

	SetText(DescriptionRichText, args);

	// Card Title
	args.Add(FText::Format(INVTEXT("{0}{1}</>"), _isMiniature ? INVTEXT("<CardName>") : INVTEXT("<CardNameLarge>"), info.name));
	
	if (buildingEnum == CardEnum::House) {
		ADDTEXT_INV_(" <Orange>[B-H]</>");
	}
	else if (buildingEnum == CardEnum::Farm) {
		ADDTEXT_INV_(" <Orange>[B-F]</>");
	}
	else if (buildingEnum == CardEnum::StorageYard) {
		ADDTEXT_INV_(" <Orange>[B-Y]</>");
	}
	else if (buildingEnum == CardEnum::DirtRoad) {
		ADDTEXT_INV_(" <Orange>[Z]</>");
	}
	else if (buildingEnum == CardEnum::StoneRoad) {
		ADDTEXT_INV_(" <Orange>[Shift-Z]</>");
	}
	SetText(BuildingNameRichText, args);

	CardGlow->SetVisibility(ESlateVisibility::Hidden);

	if (stackSize > 0) {
		cardCount = stackSize;

		Count->SetText(FText::FromString(FString::FromInt(stackSize)));

		//// 1/2, 2/4, 3/4, 4/8
		//if (stackSize >= CardCountForLvl[3]) {
		//	Count->SetText(FText::FromString(FString::FromInt(stackSize)));
		//}
		//else if (stackSize >= CardCountForLvl[2]) {
		//	Count->SetText(FText::FromString(FString::FromInt(stackSize) + "/8"));
		//}
		//else if (stackSize >= CardCountForLvl[1]) {
		//	Count->SetText(FText::FromString(FString::FromInt(stackSize) + "/4"));
		//}
		//else if (stackSize >= CardCountForLvl[0]) {
		//	Count->SetText(FText::FromString(FString::FromInt(stackSize) + "/2"));
		//}

		Count->SetVisibility(ESlateVisibility::HitTestInvisible);
	}
	else {
		Count->SetVisibility(ESlateVisibility::Collapsed);
	}

	// 
	if (IsUnboughtCard() ||
		IsPermanentCard() ||
		buildingEnum == CardEnum::Townhall ||
		buildingEnum == CardEnum::JobManagementBureau ||
		buildingEnum == CardEnum::StatisticsBureau ||
		IsSeedCard(buildingEnum))
	{
		SetStars(0);
		SellButton->SetVisibility(ESlateVisibility::Collapsed);
	}
	else {
		SetStars(buildingLvl);
		SellButton->SetVisibility(ESlateVisibility::Visible);
		SellButton->OnClicked.AddDynamic(this, &UBuildingPlacementButton::OnSellButtonClicked);
	}

	if (IsTownSlotCard(buildingEnum)) {
		CardSlotUnderneath->GetDynamicMaterial()->SetTextureParameterValue("CardSlot", assetLoader()->CardSlotRound);
		CardSlotUnderneath->GetDynamicMaterial()->SetScalarParameterValue("IsBuildingSlotCard", false);
		CardSlotUnderneath->SetVisibility(ESlateVisibility::HitTestInvisible);
	}
	else if (IsBuildingSlotCard(buildingEnum)) {
		CardSlotUnderneath->GetDynamicMaterial()->SetTextureParameterValue("CardSlot", assetLoader()->CardSlotBevel);
		CardSlotUnderneath->GetDynamicMaterial()->SetScalarParameterValue("IsBuildingSlotCard", true);
		CardSlotUnderneath->SetVisibility(ESlateVisibility::HitTestInvisible);
	}
	else {
		CardSlotUnderneath->SetVisibility(ESlateVisibility::Collapsed);
	}

	// Tooltip
	UPunBoxWidget::AddBuildingTooltip(this, buildingEnum, this, IsPermanentCard());

	// Close pricing/combi
	PriceTextBox->SetVisibility(ESlateVisibility::Collapsed);

	NeedResourcesText->SetVisibility(ESlateVisibility::Collapsed);

	BuyText->SetVisibility(ESlateVisibility::Collapsed);

	GetAnimations();
	//ExclamationIcon->SetVisibility(ESlateVisibility::Collapsed);

	initTime = 0;
}


#undef LOCTEXT_NAMESPACE