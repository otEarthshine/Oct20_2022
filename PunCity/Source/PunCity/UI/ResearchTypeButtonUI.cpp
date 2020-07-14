// Fill out your copyright notice in the Description page of Project Settings.

#include "ResearchTypeButtonUI.h"
#include "Components/Image.h"

//FReply UResearchTypeButtonUI::NativeOnMouseButtonDown(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent)
//{
//	PUN_LOG("Chose Research: %d", static_cast<int>(researchEnum));
//	unlockSystem->ChooseResearch(researchEnum);
//
//	if (InMouseEvent.GetEffectingButton() == EKeys::RightMouseButton) {
//		SetHighlight(true);
//	}
//	return FReply::Handled();
//}
//
//void UResearchTypeButtonUI::SetHighlight(bool active)
//{
//	ResearchNameBackgroundImage->SetColorAndOpacity(active ? FLinearColor(.7, .3, 0) : FLinearColor(.025, .025, .025));
//}