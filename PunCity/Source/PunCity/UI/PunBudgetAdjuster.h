// Pun Dumnernchanvanit's

#pragma once

#include "PunWidget.h"
#include "PunBudgetAdjuster.generated.h"

/**
 * 
 */
UCLASS()
class PROTOTYPECITY_API UPunBudgetAdjuster : public UPunWidget
{
	GENERATED_BODY()
public:
	UPROPERTY(meta = (BindWidget)) UImage* Icon1;
	UPROPERTY(meta = (BindWidget)) UImage* Icon2;
	UPROPERTY(meta = (BindWidget)) UImage* Icon3;
	UPROPERTY(meta = (BindWidget)) UImage* Icon4;
	UPROPERTY(meta = (BindWidget)) UImage* Icon5;

	UPROPERTY(meta = (BindWidget)) UButton* BackButton1;
	UPROPERTY(meta = (BindWidget)) UButton* BackButton2;
	UPROPERTY(meta = (BindWidget)) UButton* BackButton3;
	UPROPERTY(meta = (BindWidget)) UButton* BackButton4;
	UPROPERTY(meta = (BindWidget)) UButton* BackButton5;

	int32 buildingId = -1;
	bool isBudgetOrTime = true;
	int32 level = -1;

	UPROPERTY() UPunWidget* callbackParent;

	void OnInit() override {
		BUTTON_ON_CLICK(BackButton1, this, &UPunBudgetAdjuster::OnClickBackButton1);
		BUTTON_ON_CLICK(BackButton2, this, &UPunBudgetAdjuster::OnClickBackButton2);
		BUTTON_ON_CLICK(BackButton3, this, &UPunBudgetAdjuster::OnClickBackButton3);
		BUTTON_ON_CLICK(BackButton4, this, &UPunBudgetAdjuster::OnClickBackButton4);
		BUTTON_ON_CLICK(BackButton5, this, &UPunBudgetAdjuster::OnClickBackButton5);
	}

	void Set(UPunWidget* callbackParentIn, int32 buildingIdIn, bool isBudgetOrTimeIn, int32 levelIn)
	{
		callbackParent = callbackParentIn;
		
		buildingId = buildingIdIn;
		isBudgetOrTime = isBudgetOrTimeIn;
		level = levelIn;

		UTexture2D* iconTexture = isBudgetOrTime ? assetLoader()->CoinIcon : assetLoader()->ClockIcon;
		UTexture2D* grayIconTexture = isBudgetOrTime ? assetLoader()->CoinGrayIcon : assetLoader()->ClockGrayIcon;
		
		FButtonStyle style = BackButton1->WidgetStyle;
		style.Normal.SetResourceObject(grayIconTexture);
		style.Hovered.SetResourceObject(iconTexture);
		style.Pressed.SetResourceObject(iconTexture);
		BackButton1->SetStyle(style);
		BackButton2->SetStyle(style);
		BackButton3->SetStyle(style);
		BackButton4->SetStyle(style);
		BackButton5->SetStyle(style);

		Icon1->SetBrushFromTexture(iconTexture);
		Icon2->SetBrushFromTexture(iconTexture);
		Icon3->SetBrushFromTexture(iconTexture);
		Icon4->SetBrushFromTexture(iconTexture);
		Icon5->SetBrushFromTexture(iconTexture);

		// Refresh Level
		Icon1->SetVisibility(1 <= level ? ESlateVisibility::SelfHitTestInvisible : ESlateVisibility::Collapsed);
		Icon2->SetVisibility(2 <= level ? ESlateVisibility::SelfHitTestInvisible : ESlateVisibility::Collapsed);
		Icon3->SetVisibility(3 <= level ? ESlateVisibility::SelfHitTestInvisible : ESlateVisibility::Collapsed);
		Icon4->SetVisibility(4 <= level ? ESlateVisibility::SelfHitTestInvisible : ESlateVisibility::Collapsed);
		Icon5->SetVisibility(5 <= level ? ESlateVisibility::SelfHitTestInvisible : ESlateVisibility::Collapsed);
	}
	
	UFUNCTION() void OnClickBackButton1() { OnChangeLevel(1); }
	UFUNCTION() void OnClickBackButton2() { OnChangeLevel(2); }
	UFUNCTION() void OnClickBackButton3() { OnChangeLevel(3); }
	UFUNCTION() void OnClickBackButton4() { OnChangeLevel(4); }
	UFUNCTION() void OnClickBackButton5() { OnChangeLevel(5); }

	void OnChangeLevel(int32 levelIn)
	{
		level = levelIn;

		callbackParent->CallBack1(this, CallbackEnum::BudgetAdjust);
	}
	
};
