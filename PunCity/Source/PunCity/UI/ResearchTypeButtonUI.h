// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "../Simulation/UnlockSystem.h"
#include "Blueprint/UserWidget.h"
#include "ResearchTypeButtonUI.generated.h"

class UResearchTypeButtonUIParent
{
public:
	//virtual void UnhighlightAll() = 0;
};

/**
 * DEPRECATED...
 */
UCLASS()
class UResearchTypeButtonUI : public UUserWidget
{
	GENERATED_BODY()
public:
	//UPROPERTY(meta = (BindWidget)) class UTextBlock* ResearchNameText;
	//UPROPERTY(meta = (BindWidget)) class UImage* ResearchNameBackgroundImage;

	//FReply NativeOnMouseButtonDown(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent) override;
	//void SetHighlight(bool active);

	//UResearchTypeButtonUIParent* parentInterface;
	//UnlockSystem* unlockSystem;
	//TechEnum researchEnum = TechEnum::None;
};
