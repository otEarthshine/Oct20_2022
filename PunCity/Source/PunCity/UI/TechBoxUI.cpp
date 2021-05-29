// Fill out your copyright notice in the Description page of Project Settings.


#include "TechBoxUI.h"
#include "Materials/MaterialInstanceDynamic.h"

#define LOCTEXT_NAMESPACE "TechBoxUI"

FReply UTechBoxUI::NativeOnMouseButtonDown(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent)
{
	PUN_CHECK(_callbackParent);
	if (InMouseEvent.GetEffectingButton() == EKeys::LeftMouseButton) {
		_callbackParent->CallBack1(this, CallbackEnum::None);
	}
	return FReply::Unhandled();
}
FReply UTechBoxUI::NativeOnMouseButtonDoubleClick(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent)
{
	if (InMouseEvent.GetEffectingButton() == EKeys::LeftMouseButton) {
		_callbackParent->CallBack1(this, CallbackEnum::None);
	}
	return FReply::Unhandled();
}

//void UTechBoxUI::SetHighlight(bool active)
//{
//	OuterImage->SetColorAndOpacity(active ? UMGColor(.8, .7, .2) : UMGColor(.03, .03, .02));
//
//	for (int32 i = 0; i < _lineImages.Num(); i++) {
//		_lineImages[i]->SetColorAndOpacity(active ? FLinearColor(1, 1, 0, 1) : FLinearColor(1, 1, 1, 0.3));
//	}
//}


void UTechBoxUI::SetTechState(TechStateEnum techStateIn, bool isLockedIn, bool isInTechQueue, std::shared_ptr<ResearchInfo> tech)
{
	auto unlockSys = simulation().unlockSystem(playerId());
	isLocked = isLockedIn;
	
	float colorState = 0.0f;
	float showBubbles = 0.0f;
	bool showResearchPercent = false;

	auto setLineColorAndOpacity = [&](FLinearColor color)
	{
		auto setSingleLineColorAndOpacity = [&](UOverlay* lineOverlay) {
			if (lineOverlay)
			{
				for (int32 i = 0; i < lineOverlay->GetChildrenCount(); i++) {
					if (auto image = Cast<UImage>(lineOverlay->GetChildAt(i))) {
						image->SetColorAndOpacity(color);
					}
				}
			}
		};

		setSingleLineColorAndOpacity(lineChild);
		setSingleLineColorAndOpacity(lineChild2);
	};

	if (tech && tech->CannotUpgradeFurther())
	{
		colorState = 0.75f;
		
		setLineColorAndOpacity(FLinearColor(0.28, 0.42, 0.7, 1));
	}
	else if (isLocked)
	{
		colorState = 0.0f;
		
		setLineColorAndOpacity(FLinearColor(0.1, 0.1, 0.1, 1));
	}
	else
	{	
		if (isInTechQueue) {
			colorState = 0.5f;
			showBubbles = 1.0f;

			auto currentTech = unlockSys->currentResearch();
			
			showResearchPercent = (currentTech->techEnum == tech->techEnum); // show research percent on the current tech

			setLineColorAndOpacity(FLinearColor(0.9, 0.9, 0.9, 1));
			//lineColor = FLinearColor(1, 1, 1, 1);
		} else {
			colorState = 0.25f;

			setLineColorAndOpacity(FLinearColor(0.3, 0.3, 0.3, 1));
			//lineColor = FLinearColor(1, 1, 1, 0.3);
		}
	}

	OuterImage->GetDynamicMaterial()->SetScalarParameterValue("ColorState", colorState);
	InnerImage->GetDynamicMaterial()->SetScalarParameterValue("ColorState", colorState);

	OuterImage->GetDynamicMaterial()->SetScalarParameterValue("ShowBubbles", showBubbles);
	InnerImage->GetDynamicMaterial()->SetScalarParameterValue("ShowBubbles", showBubbles);

	FLinearColor techNameColor = colorState < 0.25f ? FLinearColor(0.5, 0.5, 0.5) : FLinearColor::White;
	TechName->SetColorAndOpacity(techNameColor);
	TechUpgradeCount->SetColorAndOpacity(techNameColor);

	if (tech && tech->maxUpgradeCount != -1) {
		TechUpgradeCount->SetText(FText::FromString(FString::FromInt(tech->upgradeCount) + "/" + FString::FromInt(tech->maxUpgradeCount)));
	} else {
		TechUpgradeCount->SetText(FText());
	}
	

	RewardBuildingIcon1->SetOpacity(colorState < 0.25f ? 0.3 : 1);
	RewardBuildingIcon2->SetOpacity(colorState < 0.25f ? 0.3 : 1);
	RewardBuildingIcon3->SetOpacity(colorState < 0.25f ? 0.3 : 1);
	RewardBonusIcon1->SetOpacity(colorState < 0.25f ? 0.3 : 1);
	RewardBonusIcon2->SetOpacity(colorState < 0.25f ? 0.3 : 1);


	if (showResearchPercent && unlockSys->hasTargetResearch())
	{
		PercentText->SetVisibility(ESlateVisibility::HitTestInvisible);
		SecText->SetVisibility(ESlateVisibility::HitTestInvisible);

		float researchFraction = unlockSys->researchFraction();

		researchFraction = min(researchFraction, 1.0f);
		SetText(PercentText, TEXT_FLOAT1_PERCENT(researchFraction * 100)));

		int64 science100XsecPerRound_Left = (100LL * tech->scienceNeeded(unlockSys->techsFinished) * Time::SecondsPerRound) - unlockSys->science100XsecPerRound;
		int64 science100Left = science100XsecPerRound_Left / Time::SecondsPerRound; // Redundant, but just make it easier to read

		int64 science100PerRound = simulation().playerOwned(playerId()).science100PerRound();
		if (science100PerRound > 0) {
			int64 secRequired = (science100Left * Time::SecondsPerRound) / science100PerRound;
			secRequired = std::max(secRequired, 0LL);

			TArray<FText> args;
			
			int64 minuteRequired = secRequired / Time::SecondsPerMinute;
			if (minuteRequired > 0) {
				ADDTEXT_(INVTEXT("{0}m "), TEXT_NUM(minuteRequired));
			}
			
			int64 remainderSecRequired = secRequired % Time::SecondsPerMinute;
			ADDTEXT_(INVTEXT("{0}s"), TEXT_NUM(remainderSecRequired));
			SetText(SecText, args);
		}

		OuterImage->GetDynamicMaterial()->SetScalarParameterValue("ResearchFraction", researchFraction);
	}
	else {
		PercentText->SetVisibility(ESlateVisibility::Collapsed);
		SecText->SetVisibility(ESlateVisibility::Collapsed);

		OuterImage->GetDynamicMaterial()->SetScalarParameterValue("ResearchFraction", 0);
	}

	/*
	 * Set Tech Requirement Text
	 */
	auto setTechRequirementText = [&](int32 currentCount, int32 requiredCount, const FText& requiredName)
	{
		if (currentCount >= requiredCount)
		{
			SetText(TechRequirement,
				LOCTEXT("TechBoxRequirementsCompleted", "Ready for Research!")
			);
		}
		else {
			SetText(TechRequirement, FText::Format(
				LOCTEXT("TechBoxRequires", "Requires:\n{0}/{1} {2}"),
				TEXT_NUM(currentCount),
				TEXT_NUM(requiredCount),
				requiredName
			));
		}
		
		//SetResourceImage(TechRequirementIcon, tech->requiredResourceEnum, assetLoader());
		//TechRequirementIcon->SetVisibility(ESlateVisibility::SelfHitTestInvisible);
		TechRequirementIcon->SetVisibility(ESlateVisibility::Collapsed);
		TechRequirement->SetVisibility(ESlateVisibility::SelfHitTestInvisible);
	};

	TechRequirement->SetVisibility(ESlateVisibility::Collapsed);
	TechRequirementIcon->SetVisibility(ESlateVisibility::Collapsed);
	if (tech)
	{
		// Required Resource
		TechRequirements requirements = tech->techRequirements;
		if (requirements.requiredResourceEnum != ResourceEnum::None &&
			tech->state != TechStateEnum::Researched)
		{
			int32 productionCount = unlockSys->GetResourceProductionCount(requirements.requiredResourceEnum);
			setTechRequirementText(
				productionCount,
				requirements.requiredResourceCount,
				ResourceNameT(requirements.requiredResourceEnum)
			);
		}

		// Required House Lvl
		else if (requirements.requiredHouseLvl != -1 &&
			tech->state != TechStateEnum::Researched)
		{
			int32 houseLvlCount = simulation().GetHouseLvlCount(playerId(), requirements.requiredHouseLvl, true);
			setTechRequirementText(
				houseLvlCount,
				requirements.requiredHouselvlCount,
				FText::Format(LOCTEXT("House Lv X", "House Lv {0}"), TEXT_NUM(requirements.requiredHouseLvl))
			);
		}
	}
}


void UTechBoxUI::UpdateTooltip()
{
	auto unlockSys = simulation().unlockSystem(playerId());
	auto tech = unlockSys->GetTechInfo(techEnum);
	std::vector<CardEnum> unlockCards = tech->GetUnlockNames();

	UPunBoxWidget* tooltipBox = UPunBoxWidget::AddToolTip(this, this)->TooltipPunBoxWidget;
	if (tooltipBox)
	{
		tooltipBox->AfterAdd();

		// Header
		tooltipBox->AddRichText(TEXT_TAG("<TipHeader>", tech->GetName()));
		tooltipBox->AddSpacer();

		// Sci points
		//std::stringstream ss;
		//tooltipBox->AddSpacer();

		const FText costText = LOCTEXT("Cost", "Cost");
		const FText requirementText = LOCTEXT("Requirement", "Requirement");

		TArray<FText> args;
		ADDTEXT_(INVTEXT("{0}: {1}<img id=\"Science\"/>"), costText, TEXT_NUM(tech->scienceNeeded(unlockSys->techsFinished)));
		tooltipBox->AddRichText(args);
		tooltipBox->AddSpacer();
		//tooltipBox->AddLineSpacer(12);

		auto setRequirementTooltipText = [&](const FText& frontText, int32 currentCount, int32 requiredCount, const FText& requiredText)
		{
			if (currentCount < requiredCount) {
				ADDTEXT_(INVTEXT("{0}:\n - {1} {2}/{3} {4}"), requirementText, frontText, TEXT_NUM(currentCount), TEXT_NUM(requiredCount), requiredText);
			}
			else {
				ADDTEXT_(INVTEXT("{0}:\n - {1} {2} {3} ({4})"), requirementText, frontText, TEXT_NUM(requiredCount), requiredText, LOCTEXT("Completed", "Completed"));
			}
			tooltipBox->AddRichText(args);
			tooltipBox->AddSpacer(12);
		};

		// Resource Requirement
		TechRequirements requirements = tech->techRequirements;
		if (requirements.requiredResourceEnum != ResourceEnum::None)
		{
			setRequirementTooltipText(LOCTEXT("Produce", "Produce"), 
				unlockSys->GetResourceProductionCount(requirements.requiredResourceEnum),
				requirements.requiredResourceCount,
				ResourceNameT(requirements.requiredResourceEnum)
			);
		}
		else if (requirements.requiredHouseLvl != -1)
		{
			int32 houseLvlCount = simulation().GetHouseLvlCount(playerId(), requirements.requiredHouseLvl, true);
			
			setRequirementTooltipText(LOCTEXT("Produce", "Produce"),
				houseLvlCount,
				requirements.requiredHouselvlCount,
				FText::Format(LOCTEXT("House Lv X", "House Lv {0}"), requirements.requiredHouseLvl)
			);
		}
		//// Tech Requirement
		//else if (unlockSys->IsAgeChangeTech(tech->techEnum))
		//{
		//	int32 requiredHouseLvl = unlockSys->GetTechRequirement_HouseLvl(tech->techEnum);
		//	int32 requiredHouseLvlCount = unlockSys->GetTechRequirement_HouseLvlCount(tech->techEnum);
		//	int32 houseLvlCount = simulation().GetHouseLvlCount(playerId(), requiredHouseLvl, true);
		//	
		//	setRequirementTooltipText(FText(),
		//		houseLvlCount,
		//		requiredHouseLvlCount,
		//		FText::Format(LOCTEXT("House Lv X", "House Lv {0}"), requiredHouseLvl)
		//	);
		//}


		// Bonus body
		if (tech->HasBonus()) {
			tooltipBox->AddRichText(tech->GetBonusDescription());
		}

		if (tech->HasBonus() && unlockCards.size() > 0) {
			tooltipBox->AddLineSpacer(12);
		}

		// Unlock body
		if (unlockCards.size() > 0)
		{
			ADDTEXT_LOCTEXT("Unlocks:", "Unlocks:");
			for (const CardEnum& cardEnum : unlockCards) {
				if (IsBuildingCard(cardEnum)) {
					ADDTEXT_(LOCTEXT("Unlocked Building Tip", "\n - Building: {0}"), GetBuildingInfo(cardEnum).name);
				}
				else if (IsActionCard(cardEnum)) {
					ADDTEXT_(LOCTEXT("Unlocked Action card Tip", "\n - Action card: {0}"), GetBuildingInfo(cardEnum).name);
				}
				else if (IsTownSlotCard(cardEnum)) {
					ADDTEXT_(LOCTEXT("Unlocked Slot card (Global) Tip", "\n - Slot card (Global): {0}"), GetBuildingInfo(cardEnum).name);
				}
				else {
					ADDTEXT_(LOCTEXT("Unlocked Slot card (Building) Tip", "\n - Slot card (Building): {0}"), GetBuildingInfo(cardEnum).name);
				}
			}
			tooltipBox->AddRichText(args);
		}
	}
}

#undef LOCTEXT_NAMESPACE 