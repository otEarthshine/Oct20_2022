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

	UPROPERTY(meta = (BindWidget)) UButton* CloseButton;
	UPROPERTY(meta = (BindWidget)) UButton* CloseButton2;

	UPROPERTY(meta = (BindWidget)) URichTextBlock* ScienceAmountText;

	UPROPERTY() TMap<int32, UTechBoxUI*> techEnumToTechBox;

	bool isInitialized = false;

	void LeftMouseDown() {}
	void LeftMouseUp() {
		_isMouseDownScrolling = false;
	}
	void RightMouseDown() {}
	void RightMouseUp() {
		_isMouseDownScrolling = false;
	}

	virtual FReply NativeOnMouseButtonDown(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent) override
	{
		//PUN_LOG("- NativeOnMouseButtonDown");
		GetWorld()->GetGameViewport()->GetMousePosition(_initialMousePosition);
		_initialScrollOffset = TechScrollBox->GetScrollOffset();
		_isMouseDownScrolling = true;
		return FReply::Handled();
	}
	
	virtual FReply NativeOnMouseButtonUp(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent) override
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

		int32 era = dataSource()->simulation().unlockSystem(playerId())->currentTechColumn();
		TechScrollBox->SetScrollOffset(std::max(0, (era - 1) * 294 - 30));

		SetVisibility(show ? ESlateVisibility::SelfHitTestInvisible : ESlateVisibility::Collapsed);
		TickUI();
	}

	virtual void TickUI();

protected: 
	virtual UOverlay* GetTechSpecialLine(TechEnum techEnum) {
		return nullptr;
	}
	virtual void SetupTechBoxUIs() {}

	// Use ResearchInfo's TechBoxLocation to link the techEnum to proper TechBoxUI
	virtual void SetupTechBoxColumn(const std::vector<TechEnum>& techEnums, UVerticalBox* columnBox);

	virtual ExclusiveUIEnum GetExclusiveUIEnum() { return ExclusiveUIEnum::TechTreeUI; }
	
	virtual bool GetShouldOpenUI() { return unlockSys()->shouldOpenTechUI; }
	virtual void SetShouldOpenUI(bool value) { unlockSys()->shouldOpenTechUI = value; }

	virtual bool GetNeedDisplayUpdate() { return unlockSys()->needTechDisplayUpdate; }
	virtual void SetNeedDisplayUpdate(bool value) { unlockSys()->needTechDisplayUpdate = value; }

	UnlockSystem* unlockSys() { return simulation().unlockSystem(playerId()); }

private:

	virtual void CallBack1(UPunWidget* punWidgetCaller, CallbackEnum callBackEnum) override;

	UFUNCTION() void CloseUI() {
		SetShowUI(false);
	}

private:
	std::vector<TechEnum> _lastTechQueue;

	bool _isMouseDownScrolling = false;

	FVector2D _initialMousePosition;
	float _initialScrollOffset;
};
