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
	else if (IsZooAnimalCard(buildingEnum)) {
		ADDTEXT_(INVTEXT("\n<Gray>({0})</>"), LOCTEXT("zoo slot", "zoo slot"));
	}
	else if (IsArtifactCard(buildingEnum)) {
		ADDTEXT_(INVTEXT("\n<Gray>({0})</>"), LOCTEXT("museum slot", "museum slot"));
	}
	else if (IsBuildingSlotCard(buildingEnum)) {
		ADDTEXT_(INVTEXT("\n<Gray>({0})</>"), LOCTEXT("building slot", "building slot"));
	}
	else if (IsMilitaryCardEnum(buildingEnum)) {
		args.Add(GetMilitaryInfoDescription(buildingEnum));
	}

	SetText(DescriptionRichText, args);

	// Card Title
	//args.Add(FText::Format(INVTEXT("{0}{1}</>"), isFullCard ? INVTEXT("<CardNameLarge>") : INVTEXT("<default>"), info.name));
	args.Add(info.name);
	
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
	FTextBlockStyle textBoxStyle = BuildingNameRichText->GetCurrentDefaultTextStyle();
	textBoxStyle.SetFontSize(isFullCard ? 14 : 12);
	BuildingNameRichText->SetDefaultTextStyle(textBoxStyle);
	
	CardGlow->SetVisibility(ESlateVisibility::Hidden);

	if (cardStatus.stackSize > 1) {
		Count->SetText(FText::FromString(FString::FromInt(cardStatus.stackSize)));
		Count->SetVisibility(ESlateVisibility::HitTestInvisible);
	}
	else {
		Count->SetVisibility(ESlateVisibility::Collapsed);
	}

	//! 
	//if (IsUnboughtCard() ||
	//	IsPermanentCard() ||
	//	buildingEnum == CardEnum::Townhall ||
	//	buildingEnum == CardEnum::JobManagementBureau ||
	//	buildingEnum == CardEnum::StatisticsBureau ||
	//	buildingEnum == CardEnum::SpyCenter ||
	//	buildingEnum == CardEnum::PolicyOffice ||
	//	IsSeedCard(buildingEnum))
	//{
	//	//SetStars(0);
	//	SellButton->SetVisibility(ESlateVisibility::Collapsed);
	//}
	//else if (isFullCard ||
	//		cardHandEnum == CardHandEnum::CardInventorySlots ||
	//		cardHandEnum == CardHandEnum::CardSetSlots)
	//{
	//	SellButton->SetVisibility(ESlateVisibility::Collapsed);
	//}
	if (ShouldHideSellButton(buildingEnum)) {
		SellButton->SetVisibility(ESlateVisibility::Collapsed);
	}
	else {
		SellButton->SetVisibility(ESlateVisibility::Visible);
		BUTTON_ON_CLICK(SellButton, this, &UBuildingPlacementButton::OnSellButtonClicked);
	}


	//! 
	if (cardHandEnum == CardHandEnum::CardSetSlots ||
		IsBuildingSlotCard(buildingEnum))
	{
		CardSlotUnderneath->GetDynamicMaterial()->SetTextureParameterValue("CardSlot", assetLoader()->CardSlotBevel);
		CardSlotUnderneath->GetDynamicMaterial()->SetScalarParameterValue("IsBuildingSlotCard", true);
		CardSlotUnderneath->SetVisibility(ESlateVisibility::HitTestInvisible);
	}
	else if (IsTownSlotCard(buildingEnum)) {
		CardSlotUnderneath->GetDynamicMaterial()->SetTextureParameterValue("CardSlot", assetLoader()->CardSlotRound);
		CardSlotUnderneath->GetDynamicMaterial()->SetScalarParameterValue("IsBuildingSlotCard", false);
		CardSlotUnderneath->SetVisibility(ESlateVisibility::HitTestInvisible);
	}
	else {
		CardSlotUnderneath->SetVisibility(ESlateVisibility::Collapsed);
	}

	// Tooltip
	UPunBoxWidget::AddBuildingTooltip(this, buildingEnum, this, IsPermanentCard());

	// Close pricing/combi
	PriceTextBox->SetVisibility(ESlateVisibility::Collapsed);
	HumanPriceTextBox->SetVisibility(ESlateVisibility::Collapsed);

	NeedResourcesText->SetVisibility(ESlateVisibility::Collapsed);

	BuyText->SetVisibility(ESlateVisibility::Collapsed);

	GetAnimations();
	//ExclamationIcon->SetVisibility(ESlateVisibility::Collapsed);

	initTime = 0;
}

void UBuildingPlacementButton::SetCardStatus(CardHandEnum cardHandEnum, bool isReservedForBuying, bool needResource, bool isRareCardHand, bool tryShowBuyText)
{
	CardEnum buildingEnum = cardStatus.cardEnum;

	if (tryShowBuyText) {
		BuyText->SetVisibility(isReservedForBuying && !isRareCardHand ? ESlateVisibility::SelfHitTestInvisible : ESlateVisibility::Collapsed);
	}
	else {
		BuyText->SetVisibility(ESlateVisibility::Collapsed);
	}

	/*
	 * CardSetSlots
	 *
	 * CardBackgroundImage
	 * BuildingNameRichText
	 */
	FTextBlockStyle textBoxStyle = BuildingNameRichText->GetCurrentDefaultTextStyle();
	bool isEmptyCollectionSlot = (cardHandEnum == CardHandEnum::CardSetSlots) && cardStatus.stackSize == 0;
	
	if (isEmptyCollectionSlot)
	{
		CardBackgroundImage->SetVisibility(ESlateVisibility::Hidden);
		textBoxStyle.SetColorAndOpacity(FLinearColor(0.2, 0.2, 0.2));
	}
	else
	{
		CardBackgroundImage->SetVisibility(ESlateVisibility::Visible);
		textBoxStyle.SetColorAndOpacity(FLinearColor(1, 1, 1));

		auto material = CardBackgroundImage->GetDynamicMaterial();
		//material->SetScalarParameterValue("IsRare", isRareCardHand ? 1.0f : 0.0f);
		material->SetScalarParameterValue("Highlight", isReservedForBuying ? 1.0f : 0.0f);

		bool isGlobalSlotCard = IsTownSlotCard(buildingEnum);
		bool isBuildingSlotCard = IsBuildingSlotCard(buildingEnum) ||
									IsZooAnimalCard(buildingEnum) ||
									IsArtifactCard(buildingEnum);

		material->SetScalarParameterValue("IsBuildingCard", IsBuildingCard(buildingEnum) ? 1.0f : 0.0f);
		material->SetScalarParameterValue("IsActionCard", IsActionCard(buildingEnum) ? 1.0f : 0.0f);
		material->SetScalarParameterValue("IsGlobalSlotCard", isGlobalSlotCard ? 1.0f : 0.0f);
		material->SetScalarParameterValue("IsBuildingSlotCard", isBuildingSlotCard ? 1.0f : 0.0f);
		material->SetScalarParameterValue("IsPermanentBonus", IsPermanentBonus(buildingEnum) ? 1.0f : 0.0f);

		//
		if (isGlobalSlotCard) {
			material->SetTextureParameterValue("CardTexture", assetLoader()->CardFrontRound);
		}
		else if (isBuildingSlotCard) {
			material->SetTextureParameterValue("CardTexture", assetLoader()->CardFrontBevel);
		}
		else {
			material->SetTextureParameterValue("CardTexture", assetLoader()->CardFront);
		}
	}

	BuildingNameRichText->SetDefaultTextStyle(textBoxStyle);

	/*
	 * 
	 */
	{
		// Don't show 
		bool showNeedResourceUI = !isReservedForBuying && needResource;

		BuildingIcon->GetDynamicMaterial()->SetScalarParameterValue("IsGray", showNeedResourceUI ? 1.0f : 0.0f);

		BuildingIcon->SetOpacity(isEmptyCollectionSlot ? 0.3 : 1);

		BuildingIcon->SetRenderTranslation(FVector2D(0, cardHandEnum == CardHandEnum::TrainUnits ? 15 : 0));

		
		if (cardHandEnum == CardHandEnum::TrainUnits) {
			NeedResourcesText->SetColorAndOpacity(FLinearColor(.2, 0, 0));
			NeedResourcesText->SetVisibility(simulation().townManager(playerId()).population() <= 0 ? ESlateVisibility::HitTestInvisible : ESlateVisibility::Collapsed);
			NeedResourcesText->SetText(NSLOCTEXT("BuildingPlacementButton", "Need Citizens", "Need\nCitizens"));
		}
		else {
			NeedResourcesText->SetColorAndOpacity(FLinearColor(.2, 0, 0));
			NeedResourcesText->SetVisibility(showNeedResourceUI ? ESlateVisibility::HitTestInvisible : ESlateVisibility::Collapsed);
			NeedResourcesText->SetText(NSLOCTEXT("BuildingPlacementButton", "Need Money", "Need Money"));
		}
	}

	//! Exclamation
	// Notify of the first time seeing permanent card..
	needExclamation = false;
	if (IsPermanentCard())
	{
		if (buildingEnum == CardEnum::Farm && !simulation().parameters(playerId())->FarmNoticed) {
			needExclamation = true;
		}
		else if (buildingEnum == CardEnum::Bridge && !simulation().parameters(playerId())->BridgeNoticed) {
			needExclamation = true;
		}
		else if (buildingEnum == CardEnum::House && simulation().NeedQuestExclamation(playerId(), QuestEnum::BuildHousesQuest)) {
			needExclamation = true;
		}
		else if (buildingEnum == CardEnum::StorageYard && simulation().HasQuest(playerId(), QuestEnum::BuildStorageQuest) &&
			simulation().buildingCount(playerId(), CardEnum::StorageYard) <= 2) {
			needExclamation = true;
		}
	}
	else
	{
		if (buildingEnum == CardEnum::Townhall) {
			needExclamation = true;
		}
		else if (simulation().HasQuest(playerId(), QuestEnum::FoodBuildingQuest) &&
			IsFoodBuilding(buildingEnum))
		{
			needExclamation = true;
		}

		// First Seed Card
		if (cardHandEnum == CardHandEnum::BoughtHand &&
			IsSeedCard(buildingEnum) &&
			!simulation().unlockSystem(playerId())->isUnlocked(CardEnum::Farm))
		{
			needExclamation = true;
		}
	}

	//PlayAnimationIf("Flash", needExclamation);
	//ExclamationIcon->SetShow(needExclamation);

	SellButton->SetVisibility(ShouldHideSellButton(buildingEnum) ? ESlateVisibility::Collapsed : ESlateVisibility::Visible);
}

#undef LOCTEXT_NAMESPACE