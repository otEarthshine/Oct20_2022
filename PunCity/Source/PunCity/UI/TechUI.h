// Pun Dumnernchanvanit's

#pragma once

#include "CoreMinimal.h"
#include "PunCity/UI/PunWidget.h"
#include "TechBoxUI.h"
#include "TechUI.generated.h"

/**
 * 
 */
UCLASS()
class PROTOTYPECITY_API UTechUI : public UPunWidget
{
	GENERATED_BODY()
public:
	UPROPERTY(meta = (BindWidget)) UScrollBox* TechScrollBox;

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

		CloseButton->OnClicked.AddDynamic(this, &UTechUI::CloseUI);
		CloseButton2->OnClicked.AddDynamic(this, &UTechUI::CloseUI);

		TechScrollBox->SetScrollbarThickness(FVector2D(10, 10));
	}

	void SetShowUI(bool show) {
		if (show) {
			networkInterface()->ResetGameUI();

			simulation().TryRemovePopups(playerId(), PopupReceiverEnum::DoneResearchEvent_ShowTree);

			dataSource()->Spawn2DSound("UI", "UIWindowOpen");
		} else {
			dataSource()->Spawn2DSound("UI", "UIWindowClose");
		}

		int32 era = dataSource()->simulation().unlockSystem(playerId())->currentTechColumn();
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
