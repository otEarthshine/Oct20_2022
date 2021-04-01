// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "PunCity/UI/PunWidget.h"
#include "PunCity/Simulation/UnlockSystem.h"

#include "TechBoxUI.h"

#include "TechTreeUI.generated.h"

/**
 * 
 */
UCLASS()
class UTechTreeUI : public UPunWidget
{
	GENERATED_BODY()
public:
	UPROPERTY(meta = (BindWidget)) UScrollBox* TechScrollBox;

	UPROPERTY(meta = (BindWidget)) UTextBlock* Title_DarkAge;
	UPROPERTY(meta = (BindWidget)) UVerticalBox* TechList_DarkAge1;
	UPROPERTY(meta = (BindWidget)) UVerticalBox* TechList_DarkAge2;

	UPROPERTY(meta = (BindWidget)) UTextBlock* Title_MiddleAge;
	UPROPERTY(meta = (BindWidget)) UVerticalBox* TechList_MiddleAge1;
	UPROPERTY(meta = (BindWidget)) UVerticalBox* TechList_MiddleAge2;
	UPROPERTY(meta = (BindWidget)) UVerticalBox* TechList_MiddleAge3;

	UPROPERTY(meta = (BindWidget)) UTextBlock* Title_EnlightenmentAge;
	UPROPERTY(meta = (BindWidget)) UVerticalBox* TechList_EnlightenmentAge1;
	UPROPERTY(meta = (BindWidget)) UVerticalBox* TechList_EnlightenmentAge2;
	UPROPERTY(meta = (BindWidget)) UVerticalBox* TechList_EnlightenmentAge3;

	UPROPERTY(meta = (BindWidget)) UTextBlock* Title_IndustrialAge;
	UPROPERTY(meta = (BindWidget)) UVerticalBox* TechList_IndustrialAge1;
	UPROPERTY(meta = (BindWidget)) UVerticalBox* TechList_IndustrialAge2;
	UPROPERTY(meta = (BindWidget)) UVerticalBox* TechList_IndustrialAge3;

	UPROPERTY(meta = (BindWidget)) UButton* CloseButton;
	UPROPERTY(meta = (BindWidget)) UButton* CloseButton2;

	UPROPERTY(meta = (BindWidget)) URichTextBlock* ScienceAmountText;

	UPROPERTY() TMap<int32, UTechBoxUI*> techEnumToTechBox;

	bool isInitialized = false;

	void LeftMouseDown()
	{

	}
	void LeftMouseUp() {
		_isMouseDownScrolling = false;
	}
	void RightMouseDown() {

	}
	void RightMouseUp() {
		_isMouseDownScrolling = false;
	}

	FReply NativeOnMouseButtonDown(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent) override
	{
		//PUN_LOG("- NativeOnMouseButtonDown");
		GetWorld()->GetGameViewport()->GetMousePosition(_initialMousePosition);
		_initialScrollOffset = TechScrollBox->GetScrollOffset();
		_isMouseDownScrolling = true;
		return FReply::Handled();
	}

	FReply NativeOnMouseButtonUp(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent) override
	{
		//PUN_LOG("- NativeOnMouseButtonUp");
		_isMouseDownScrolling = false;
		return FReply::Handled();
	}

public:
	void PunInit()
	{
		isInitialized = false;
		SetVisibility(ESlateVisibility::Collapsed);

		CloseButton->OnClicked.AddDynamic(this, &UTechTreeUI::CloseUI);
		CloseButton2->OnClicked.AddDynamic(this, &UTechTreeUI::CloseUI);

		TechScrollBox->SetScrollbarThickness(FVector2D(10, 10));
	}

	void SetShowUI(bool show) {
		if (show) {
			networkInterface()->ResetGameUI();

			simulation().TryRemovePopups(playerId(), PopupReceiverEnum::DoneResearchEvent_ShowTree);

			dataSource()->Spawn2DSound("UI", "UIWindowOpen");
		}
		else {
			dataSource()->Spawn2DSound("UI", "UIWindowClose");
		}

		int32 era = dataSource()->simulation().unlockSystem(playerId())->currentEra();
		TechScrollBox->SetScrollOffset(std::max(0, (era - 1) * 294 - 30));

		SetVisibility(show ? ESlateVisibility::SelfHitTestInvisible : ESlateVisibility::Collapsed);
		TickUI();
	}

	void TickUI();

private:
	// Use ResearchInfo's TechBoxLocation to link the techEnum to proper TechBoxUI
	void SetupTechBoxUIs();

	void CallBack1(UPunWidget* punWidgetCaller, CallbackEnum callBackEnum) final;

	UFUNCTION() void CloseUI() {
		SetShowUI(false);
	}

private:
	std::vector<TechEnum> _lastTechQueue;

	bool _isMouseDownScrolling = false;

	FVector2D _initialMousePosition;
	float _initialScrollOffset;
};
