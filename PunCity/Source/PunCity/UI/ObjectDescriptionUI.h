// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "PunWidget.h"
#include "PunBoxWidget.h"
#include "PunEditableNumberBox.h"

#include "GraphDataSource.h"
#include "KantanChartLegend.h"

class UObjectDescriptionUIParent
{
public:
	virtual void CloseDescriptionUI() = 0;
};

#include "ObjectDescriptionUI.generated.h"
/**
 * 
 */
UCLASS()
class UObjectDescriptionUI : public UPunWidget
{
	GENERATED_BODY()
public:
	UPROPERTY(meta = (BindWidget)) URichTextBlock* DescriptionUITitle;
	UPROPERTY(meta = (BindWidget)) UPunBoxWidget* DescriptionPunBox;
	UPROPERTY(meta = (BindWidget)) UButton* BuildingsStatOpener;
	UPROPERTY(meta = (BindWidget)) UButton* CloseButton;

	UPROPERTY(meta = (BindWidget)) UCheckBox* ToolCheckBox;
	UPROPERTY(meta = (BindWidget)) UTextBlock* ToolCheckBoxText;

	UPROPERTY(meta = (BindWidget)) UButton* TradeButton;
	UPROPERTY(meta = (BindWidget)) UTextBlock* TradeButtonText;

	UPROPERTY(meta = (BindWidget)) UComboBoxString* ObjectDropDownBox;
	UPROPERTY(meta = (BindWidget)) UPunEditableNumberBox* EditableNumberBox;
	
	UPROPERTY(meta = (BindWidget)) UOverlay* ChooseResourceOverlay;
	UPROPERTY(meta = (BindWidget)) UPunBoxWidget* ChooseResourceBox;
	UPROPERTY(meta = (BindWidget)) UButton* ChooseResourceCloseButton;
	UPROPERTY(meta = (BindWidget)) UEditableTextBox* SearchBox;

	UPROPERTY(meta = (BindWidget)) UOverlay* ManageStorageOverlay;
	UPROPERTY(meta = (BindWidget)) UPunBoxWidget* ManageStorageBox;
	UPROPERTY(meta = (BindWidget)) UButton* ManageStorageCloseButton;


	UPROPERTY(meta = (BindWidget)) UWrapBox* CardSlots;
	std::vector<CardStatus> lastCards;
	int32 lastMaxCards = -1;
	bool lastIsTownhall = false;

	UPROPERTY(meta = (BindWidget)) TArray<UWidget*> EmptyCardSlots;

	void Setup();
	void SetDropDown(int id);
	void SetEditableNumberBox(int32 id, UPunWidget* callbackParent, CallbackEnum callbackEnum);

	void CloseAllSubUIs(bool shouldCloseStatistics)
	{
		ToolCheckBox->SetVisibility(ESlateVisibility::Collapsed);

		ObjectDropDownBox->SetVisibility(ESlateVisibility::Collapsed);
		EditableNumberBox->SetVisibility(ESlateVisibility::Collapsed);
		
		TradeButton->SetVisibility(ESlateVisibility::Collapsed);


		TradeButton->SetBackgroundColor(PunUIGreen2);

		ChooseResourceOverlay->SetVisibility(ESlateVisibility::Collapsed);
		ManageStorageOverlay->SetVisibility(ESlateVisibility::Collapsed);

		CardSlots->SetVisibility(ESlateVisibility::Collapsed);
	}

	DescriptionUIState state;
	UObjectDescriptionUIParent* parent;

private:

	UFUNCTION() void ToolCheckBoxChanged(bool active);

	UFUNCTION() void ClickedTradeButton();
	UFUNCTION() void ClickedChooseResource() {
		ChooseResourceOverlay->SetVisibility(ESlateVisibility::Collapsed);
	}

	UFUNCTION() void ClickCloseManageStorageOverlay() {
		ManageStorageOverlay->SetVisibility(ESlateVisibility::Collapsed);
	}

	UFUNCTION() void OnClickCloseButton() {
		simulation().SetDescriptionUIState(DescriptionUIState());
	}

	UFUNCTION() void OnClickBuildingsStatOpener() {
		//ESlateVisibility visibility = StatisticsPanel->GetVisibility();
		//if (visibility == ESlateVisibility::Visible) {
		//	StatisticsPanel->SetVisibility(ESlateVisibility::Collapsed);
		//}
		//else {
		//	StatisticsPanel->SetVisibility(ESlateVisibility::Visible);
		//	OnBuildingsStatButtonClick();
		//}
	}

	
	UFUNCTION() void OnDropDownChanged(FString sItem, ESelectInfo::Type seltype);

};
