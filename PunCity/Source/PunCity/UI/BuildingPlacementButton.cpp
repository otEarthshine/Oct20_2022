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


void UBuildingPlacementButton::PunInit(CardStatus cardStatusIn, int32 cardHandIndexIn, UPunWidget* callbackParent, CallbackEnum callbackEnum, CardHandEnum cardHandEnum)
{
	cardStatus = cardStatusIn;
	cardHandIndex = cardHandIndexIn;

	_callbackParent = callbackParent;
	_callbackEnum = callbackEnum;
	_cardHandEnum = cardHandEnum;

	bool isFullCard = _cardHandEnum == CardHandEnum::DrawHand ||
						_cardHandEnum == CardHandEnum::ConverterHand ||
						_cardHandEnum == CardHandEnum::RareHand;

	CardEnum buildingEnum = cardStatus.cardEnum;

	// Prevent Crash for None
	if (buildingEnum == CardEnum::None) 
	{
		ParentOverlay->SetVisibility(ESlateVisibility::Collapsed);
		CardSlotUnderneath->SetVisibility(ESlateVisibility::HitTestInvisible);
		return;
	}
	ParentOverlay->SetVisibility(ESlateVisibility::Visible);
	CardSlotUnderneath->SetVisibility(ESlateVisibility::Collapsed);

	
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
	args.Add(FText::Format(INVTEXT("{0}{1}</>"), isFullCard ? INVTEXT("<CardNameLarge>") : INVTEXT("<CardName>"), info.name));
	
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
	else if (buildingEnum == CardEnum::Demolish) {
		ADDTEXT_INV_(" <Orange>[X]</>");
	}
	SetText(BuildingNameRichText, args);

	CardGlow->SetVisibility(ESlateVisibility::Hidden);

	if (cardStatus.stackSize > 1) {
		Count->SetText(FText::FromString(FString::FromInt(cardStatus.stackSize)));
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
		//SetStars(0);
		SellButton->SetVisibility(ESlateVisibility::Collapsed);
	}
	else if (isFullCard ||
			cardHandEnum == CardHandEnum::CardInventorySlots
		)
	{
		SellButton->SetVisibility(ESlateVisibility::Collapsed);
	}
	else {
		SellButton->SetVisibility(ESlateVisibility::Visible);
		SellButton->OnClicked.AddUniqueDynamic(this, &UBuildingPlacementButton::OnSellButtonClicked);
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