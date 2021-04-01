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
	// TODO: get rid of line color??
	//FLinearColor lineColor;

	auto unlockSys = simulation().unlockSystem(playerId());
	isLocked = isLockedIn;
	
	float colorState = 0.0f;
	float showBubbles = 0.0f;
	bool showResearchPercent = false;

	

	if (techStateIn == TechStateEnum::Researched)
	{
		colorState = 0.75f;
		//lineColor = FLinearColor(1, 1, 1, 1);
	}
	else if (isLocked)
	{
		colorState = 0.0f;
	}
	else
	{
		if (isInTechQueue) {
			colorState = 0.5f;
			showBubbles = 1.0f;

			auto currentTech = unlockSys->currentResearch();
			
			showResearchPercent = (currentTech->techEnum == tech->techEnum); // show research percent on the current tech
			
			//lineColor = FLinearColor(1, 1, 1, 1);
		} else {
			colorState = 0.25f;
			
			//lineColor = FLinearColor(1, 1, 1, 0.3);
		}
	}

	OuterImage->GetDynamicMaterial()->SetScalarParameterValue("ColorState", colorState);
	InnerImage->GetDynamicMaterial()->SetScalarParameterValue("ColorState", colorState);

	OuterImage->GetDynamicMaterial()->SetScalarParameterValue("ShowBubbles", showBubbles);
	InnerImage->GetDynamicMaterial()->SetScalarParameterValue("ShowBubbles", showBubbles);

	TechName->SetColorAndOpacity(colorState < 0.25f ? FLinearColor(0.5, 0.5, 0.5) : FLinearColor::White);

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

		int32 science100XsecPerRound_Left = (100.0f * tech->scienceNeeded(unlockSys->techsFinished) * Time::SecondsPerRound) - unlockSys->science100XsecPerRound;
		int32 science100Left = science100XsecPerRound_Left / Time::SecondsPerRound; // Redundant, but just make it easier to read

		int32 science100PerRound = simulation().playerOwned(playerId()).science100PerRound();
		if (science100PerRound > 0) {
			int32 secRequired = (science100Left * Time::SecondsPerRound) / science100PerRound;
			secRequired = std::max(secRequired, 0);

			TArray<FText> args;
			
			int32 minuteRequired = secRequired / Time::SecondsPerMinute;
			if (minuteRequired > 0) {
				ADDTEXT_(INVTEXT("{0}m "), TEXT_NUM(minuteRequired));
			}
			
			int32 remainderSecRequired = secRequired % Time::SecondsPerMinute;
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
			SetText(TechRequirement, FText::Format(
				LOCTEXT("TechBoxRequirementsCompleted", "Completed:\n{0} {1}"),
				TEXT_NUM(requiredCount),
				requiredName
			));
		}
		else {
			SetText(TechRequirement, FText::Format(
				LOCTEXT("TechBoxRequired", "Required:\n{0}/{1} {2}"),
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
		if (tech->requiredResourceEnum != ResourceEnum::None)
		{
			int32 productionCount = unlockSys->GetResourceProductionCount(tech->requiredResourceEnum);
			setTechRequirementText(
				productionCount,
				tech->requiredResourceCount,
				ResourceNameT(tech->requiredResourceEnum)
			);
		}
		// Required Techs
		else if (UnlockSystem::IsAgeChangeTech(tech->techEnum))
		{
			setTechRequirementText(
				unlockSys->GetAgeChangeResearchCount(tech->techEnum),
				UnlockSystem::GetAgeChangeRequiredTechCount(tech->techEnum),
				FText::Format(LOCTEXT("X-Age Techs", "{0} Techs"), UnlockSystem::GetPreviousAgeText(tech->techEnum))
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
		if (tech->requiredResourceEnum != ResourceEnum::None)
		{
			setRequirementTooltipText(LOCTEXT("Produce", "Produce"), 
				unlockSys->GetResourceProductionCount(tech->requiredResourceEnum),
				tech->requiredResourceCount, 
				ResourceNameT(tech->requiredResourceEnum)
			);
		}
		// Tech Requirement
		else if (unlockSys->IsAgeChangeTech(tech->techEnum))
		{
			setRequirementTooltipText(LOCTEXT("Research", "Research"),
				unlockSys->GetAgeChangeResearchCount(tech->techEnum),
				UnlockSystem::GetAgeChangeRequiredTechCount(tech->techEnum),
				FText::Format(LOCTEXT("X-Age Technologies", "{0} Technologies"), UnlockSystem::GetPreviousAgeText(tech->techEnum))
			);
		}


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
				else if (IsGlobalSlotCard(cardEnum)) {
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