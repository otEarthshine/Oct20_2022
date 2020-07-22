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
	float researchFraction = unlockSys->researchFraction();
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
			showResearchPercent = (techStateIn == TechStateEnum::Researching);
			
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

	OuterImage->GetDynamicMaterial()->SetScalarParameterValue("ResearchFraction", researchFraction);

	if (showResearchPercent && researchFraction > 0.001f) 
	{
		PercentText->SetVisibility(ESlateVisibility::HitTestInvisible);
		SecText->SetVisibility(ESlateVisibility::HitTestInvisible);

		stringstream ss;
		ss << std::fixed << std::showpoint << std::setprecision(1);
		ss << (researchFraction * 100) << "%";
		SetText(PercentText, ss.str());

		ss.str("");
		int32 science100XsecPerRound_Left = (100.0f * tech->scienceNeeded(unlockSys->techsFinished) * Time::SecondsPerRound) - unlockSys->science100XsecPerRound;
		int32 science100Left = science100XsecPerRound_Left / Time::SecondsPerRound; // Redundant, but just make it easier to read

		int32 science100PerRound = simulation().playerOwned(playerId()).science100PerRound();
		int32 secRequired = (science100Left * Time::SecondsPerRound) / science100PerRound;
		ss << secRequired << "s";
		SetText(SecText, ss.str());
	}
	else {
		PercentText->SetVisibility(ESlateVisibility::Collapsed);
		SecText->SetVisibility(ESlateVisibility::Collapsed);
	}

	//OuterImage->SetColorAndOpacity(outerImageColor);
	//InnerImage->SetColorAndOpacity(outerImageColor);

	//for (int32 i = 0; i < _lineImages.Num(); i++) {
	//	_lineImages[i]->SetColorAndOpacity(lineColor);
	//}
}