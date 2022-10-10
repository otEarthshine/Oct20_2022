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
	//UPROPERTY(meta = (BindWidget)) URichTextBlock* DescriptionUITitle;
	UPROPERTY(meta = (BindWidget)) UPunBoxWidget* DescriptionPunBox;
	//UPROPERTY(meta = (BindWidget)) UButton* BuildingsStatOpener;

	//UPROPERTY(meta = (BindWidget)) UEditableTextBox* NameEditTextBox;
	//UPROPERTY(meta = (BindWidget)) UButton* NameEditButton;
	//UPROPERTY(meta = (BindWidget)) UTextBlock* NameEditButtonText;

	UPROPERTY(meta = (BindWidget)) UWGT_ButtonCpp* CloseButton;

	UPROPERTY(meta = (BindWidget)) UComboBoxString* ObjectDropDownBox;
	UPROPERTY(meta = (BindWidget)) UPunEditableNumberBox* EditableNumberBox;
	
	UPROPERTY(meta = (BindWidget)) UOverlay* ChooseResourceOverlay;
	UPROPERTY(meta = (BindWidget)) UPunBoxWidget* ChooseResourceBox;
	UPROPERTY(meta = (BindWidget)) UButton* ChooseResourceCloseButton;
	UPROPERTY(meta = (BindWidget)) UEditableTextBox* SearchBox;

	UPROPERTY(meta = (BindWidget)) UOverlay* ManageStorageOverlay;
	UPROPERTY(meta = (BindWidget)) USizeBox* ManageStorageWidthSizeBox;
	UPROPERTY(meta = (BindWidget)) UTextBlock* ManageStorageTitle;
	UPROPERTY(meta = (BindWidget)) UPunBoxWidget* ManageStorageBox;
	UPROPERTY(meta = (BindWidget)) UButton* ManageStorageCloseButton;
	UPROPERTY(meta = (BindWidget)) UButton* AllowAllButton;
	UPROPERTY(meta = (BindWidget)) UButton* DisallowAllButton;
	UPROPERTY(meta = (BindWidget)) UEditableTextBox* ManageStorageSearchBox;

	UPROPERTY(meta = (BindWidget)) USizeBox* DescriptionPunBoxScrollOuter;
	UPROPERTY(meta = (BindWidget)) UPunBoxWidget* DescriptionPunBoxScroll;

	UPROPERTY(meta = (BindWidget)) UVerticalBox* TownBonusBox;
	UPROPERTY(meta = (BindWidget)) UWrapBox* TownBonusSlots;
	UPROPERTY(meta = (BindWidget)) UVerticalBox* GlobalBonusBox;
	UPROPERTY(meta = (BindWidget)) UWrapBox* GlobalBonusSlots;

	//UPROPERTY(meta = (BindWidget)) UOverlay* OldFocusUITitle;

	UPROPERTY(meta = (BindWidget)) USizeBox* NameEditPopup;
	UPROPERTY(meta = (BindWidget)) UEditableTextBox* NameEditTextBox;
	UPROPERTY(meta = (BindWidget)) UButton* NameEditSubmitButton;
	UPROPERTY(meta = (BindWidget)) UButton* NameEditCancelButton;
	UPROPERTY(meta = (BindWidget)) UWGT_ButtonCpp* NameEditCloseButton;

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
		//ToolCheckBox->SetVisibility(ESlateVisibility::Collapsed);
		NameEditPopup->SetVisibility(ESlateVisibility::Collapsed);

		ObjectDropDownBox->SetVisibility(ESlateVisibility::Collapsed);
		EditableNumberBox->SetVisibility(ESlateVisibility::Collapsed);

		ChooseResourceOverlay->SetVisibility(ESlateVisibility::Collapsed);
		ManageStorageOverlay->SetVisibility(ESlateVisibility::Collapsed);

		CardSlots->SetVisibility(ESlateVisibility::Collapsed);
	}

	void OpenNameEditPopup()
	{
		NameEditTextBox->SetText(simulation().townNameT(playerId()));
		NameEditPopup->SetVisibility(ESlateVisibility::Visible);
	}

public:
	DescriptionUIState state;
	UObjectDescriptionUIParent* parent;

private:

	//UFUNCTION() void ToolCheckBoxChanged(bool active);

	UFUNCTION() void ClickedChooseResource() {
		ChooseResourceOverlay->SetVisibility(ESlateVisibility::Collapsed);
	}

	UFUNCTION() void ClickCloseManageStorageOverlay() {
		ManageStorageOverlay->SetVisibility(ESlateVisibility::Collapsed);
	}
	UFUNCTION() void ClickAllowAll() {
		SetAllowResource(true);
	}
	UFUNCTION() void ClickDisallowAll() {
		SetAllowResource(false);
	}
	void SetAllowResource(bool allow) {
		auto command = std::make_shared<FSetAllowResource>();
		command->buildingId = simulation().descriptionUIState().objectId;
		command->resourceEnum = ResourceEnum::None;
		command->allowed = allow;
		networkInterface()->SendNetworkCommand(command);
	}

	UFUNCTION() void OnClickCloseButton() {
		simulation().SetDescriptionUIState(DescriptionUIState());
	}

	//UFUNCTION() void OnClickBuildingsStatOpener() {
	//	//ESlateVisibility visibility = StatisticsPanel->GetVisibility();
	//	//if (visibility == ESlateVisibility::Visible) {
	//	//	StatisticsPanel->SetVisibility(ESlateVisibility::Collapsed);
	//	//}
	//	//else {
	//	//	StatisticsPanel->SetVisibility(ESlateVisibility::Visible);
	//	//	OnBuildingsStatButtonClick();
	//	//}
	//}
	
	
	UFUNCTION() void OnDropDownChanged(FString sItem, ESelectInfo::Type seltype);



	UFUNCTION() void OnClickNameEditSubmitButton()
	{
		auto command = make_shared<FChangeName>();
		command->name = TrimStringF(NameEditTextBox->GetText().ToString(), 30);
		PUN_CHECK(simulation().descriptionUIState().objectType == ObjectTypeEnum::Building);
		command->objectId = simulation().descriptionUIState().objectId;
		networkInterface()->SendNetworkCommand(command);
		
		NameEditPopup->SetVisibility(ESlateVisibility::Collapsed);
	}
	UFUNCTION() void OnClickNameEditCancelButton() {
		NameEditPopup->SetVisibility(ESlateVisibility::Collapsed);
	}
};
