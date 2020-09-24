// Pun Dumnernchanvanit's

#pragma once

#include "PunWidget.h"
#include "JobPriorityRow.generated.h"

/**
 * 
 */
UCLASS()
class PROTOTYPECITY_API UJobPriorityRow : public UPunWidget
{
	GENERATED_BODY()
public:
	UPROPERTY(meta = (BindWidget)) UButton* ArrowUpButton;
	UPROPERTY(meta = (BindWidget)) UButton* ArrowDownButton;
	UPROPERTY(meta = (BindWidget)) UTextBlock* JobNameText;
	UPROPERTY(meta = (BindWidget)) UTextBlock* JobCountText;
	UPROPERTY(meta = (BindWidget)) UButton* ArrowFastUpButton;
	UPROPERTY(meta = (BindWidget)) UButton* ArrowFastDownButton;

	CardEnum cardEnum = CardEnum::None;
	int32 index = -1;
	int32 visibleIndex = -1;

	void Init(UPunWidget* parent, CardEnum cardEnumIn)
	{
		cardEnum = cardEnumIn;
		SetText(JobNameText, GetBuildingInfo(cardEnum).name);
		
		BUTTON_ON_CLICK(ArrowUpButton, this, &UJobPriorityRow::OnClickUpButton);
		BUTTON_ON_CLICK(ArrowDownButton, this, &UJobPriorityRow::OnClickDownButton);
		BUTTON_ON_CLICK(ArrowFastUpButton, this, &UJobPriorityRow::OnClickFastUpButton);
		BUTTON_ON_CLICK(ArrowFastDownButton, this, &UJobPriorityRow::OnClickFastDownButton);
		
		_parent = parent;
	}

	UFUNCTION() void  OnClickUpButton() {
		_parent->CallBack2(this, CallbackEnum::SetGlobalJobPriority_Up);
	}
	UFUNCTION() void  OnClickDownButton() {
		_parent->CallBack2(this, CallbackEnum::SetGlobalJobPriority_Down);
	}
	UFUNCTION() void  OnClickFastUpButton() {
		_parent->CallBack2(this, CallbackEnum::SetGlobalJobPriority_FastUp);
	}
	UFUNCTION() void  OnClickFastDownButton() {
		_parent->CallBack2(this, CallbackEnum::SetGlobalJobPriority_FastDown);
	}

private:
	UPROPERTY() UPunWidget* _parent = nullptr;
};
