// Fill out your copyright notice in the Description page of Project Settings.


#include "TechBoxUI.h"
#include "Materials/MaterialInstanceDynamic.h"

FReply UTechBoxUI::NativeOnMouseButtonDown(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent)
{
	PUN_CHECK(_callbackParent);
	if (InMouseEvent.GetEffectingButton() == EKeys::LeftMouseButton) {
		_callbackParent->CallBack1(this, CallbackEnum::None);
	}
	return FReply::Handled();
}
FReply UTechBoxUI::NativeOnMouseButtonDoubleClick(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent)
{
	if (InMouseEvent.GetEffectingButton() == EKeys::LeftMouseButton) {
		_callbackParent->CallBack1(this, CallbackEnum::None);
	}
	return FReply::Handled();
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

		stringstream ss;
		ss << std::fixed << std::showpoint << std::setprecision(1);
		ss << (researchFraction * 100) << "%";
		SetText(PercentText, ss.str());

		ss.str("");
		int32 science100XsecPerRound_Left = (100.0f * tech->scienceNeeded(unlockSys->techsFinished) * Time::SecondsPerRound) - unlockSys->science100XsecPerRound;
		int32 science100Left = science100XsecPerRound_Left / Time::SecondsPerRound; // Redundant, but just make it easier to read

		int32 science100PerRound = simulation().playerOwned(playerId()).science100PerRound();
		if (science100PerRound > 0) {
			int32 secRequired = (science100Left * Time::SecondsPerRound) / science100PerRound;
			
			int32 minuteRequired = secRequired / Time::SecondsPerMinute;
			if (minuteRequired > 0) {
				ss << minuteRequired << "m ";
			}
			
			int32 remainderSecRequired = secRequired % Time::SecondsPerMinute;
			ss << remainderSecRequired << "s";
			SetText(SecText, ss.str());
		}

		OuterImage->GetDynamicMaterial()->SetScalarParameterValue("ResearchFraction", unlockSys->researchFraction());
	}
	else {
		PercentText->SetVisibility(ESlateVisibility::Collapsed);
		SecText->SetVisibility(ESlateVisibility::Collapsed);

		OuterImage->GetDynamicMaterial()->SetScalarParameterValue("ResearchFraction", 0);
	}

	// Required Resource
	if (tech && tech->requiredResourceEnum != ResourceEnum::None)
	{
		int32 productionCount =  unlockSys->GetResourceProductionCount(tech->requiredResourceEnum);

		if (productionCount < tech->requiredResourceCount)
		{
			std::stringstream ss;
			ss << "Required:\n" << productionCount << "/" << tech->requiredResourceCount << " " << ResourceName(tech->requiredResourceEnum);
			SetText(TechRequirement, ss.str());
		}
		else {
			std::stringstream ss;
			ss << "Completed:\n" << tech->requiredResourceCount << " " << ResourceName(tech->requiredResourceEnum);
			SetText(TechRequirement, ss.str());
		}

		//SetResourceImage(TechRequirementIcon, tech->requiredResourceEnum, assetLoader());
		//TechRequirementIcon->SetVisibility(ESlateVisibility::SelfHitTestInvisible);
		TechRequirementIcon->SetVisibility(ESlateVisibility::Collapsed);
		TechRequirement->SetVisibility(ESlateVisibility::SelfHitTestInvisible);
	}
	else {
		TechRequirement->SetVisibility(ESlateVisibility::Collapsed);
		TechRequirementIcon->SetVisibility(ESlateVisibility::Collapsed);
	}
	
}