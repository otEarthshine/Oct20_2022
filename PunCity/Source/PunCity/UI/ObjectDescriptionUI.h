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

	UPROPERTY(meta = (BindWidget)) UEditableTextBox* NameEditTextBox;
	UPROPERTY(meta = (BindWidget)) UButton* NameEditButton;
	UPROPERTY(meta = (BindWidget)) UTextBlock* NameEditButtonText;

	UPROPERTY(meta = (BindWidget)) UButton* CloseButton;

	UPROPERTY(meta = (BindWidget)) USizeBox* BuildingSwapArrows;
	UPROPERTY(meta = (BindWidget)) UButton* BuildingSwapArrowLeftButton;
	UPROPERTY(meta = (BindWidget)) UButton* BuildingSwapArrowRightButton;

	UPROPERTY(meta = (BindWidget)) UCheckBox* ToolCheckBox;
	UPROPERTY(meta = (BindWidget)) UTextBlock* ToolCheckBoxText;

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

	UPROPERTY(meta = (BindWidget)) USizeBox* DescriptionPunBoxScrollOuter;
	UPROPERTY(meta = (BindWidget)) UPunBoxWidget* DescriptionPunBoxScroll;

	UPROPERTY(meta = (BindWidget)) UVerticalBox* TownBonusBox;
	UPROPERTY(meta = (BindWidget)) UWrapBox* TownBonusSlots;
	UPROPERTY(meta = (BindWidget)) UVerticalBox* GlobalBonusBox;
	UPROPERTY(meta = (BindWidget)) UWrapBox* GlobalBonusSlots;
	

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

		ChooseResourceOverlay->SetVisibility(ESlateVisibility::Collapsed);
		ManageStorageOverlay->SetVisibility(ESlateVisibility::Collapsed);

		CardSlots->SetVisibility(ESlateVisibility::Collapsed);

		NameEditTextBox->SetVisibility(ESlateVisibility::Collapsed);
		NameEditButtonText->SetText(NSLOCTEXT("ObjDescUI", "Edit", "Edit"));

		BuildingSwapArrows->SetVisibility(ESlateVisibility::Collapsed);
	}

	void SwitchToNextBuilding(bool isLeft)
	{
		if (state.objectType == ObjectTypeEnum::Building)
		{
			CardEnum buildingEnum = simulation().buildingEnum(state.objectId);
			const std::vector<int32>& buildingIds = simulation().buildingIds(playerId(), buildingEnum);
			if (buildingIds.size() > 1) {
				// Find the index of the current building in the array and increment it by 1
				int32 index = -1;
				for (size_t i = 0; i < buildingIds.size(); i++) {
					if (buildingIds[i] == state.objectId) {
						index = i;
						break;
					}
				}
				if (index != -1) {
					int32 nextIndex = (index + 1) % buildingIds.size();
					int32 buildingId = buildingIds[nextIndex];
					simulation().SetDescriptionUIState({ ObjectTypeEnum::Building, buildingId });
					networkInterface()->SetCameraAtom(simulation().building(buildingId).centerTile().worldAtom2());
				}
			}
		}
	}

public:
	DescriptionUIState state;
	UObjectDescriptionUIParent* parent;

private:

	UFUNCTION() void ToolCheckBoxChanged(bool active);

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

	UFUNCTION() void OnClickNameEditButton()
	{
		if (NameEditTextBox->GetVisibility() == ESlateVisibility::Collapsed) {
			NameEditTextBox->SetText(simulation().townNameT(playerId()));
			NameEditTextBox->SetVisibility(ESlateVisibility::Visible);
			DescriptionUITitle->SetVisibility(ESlateVisibility::Collapsed);
			NameEditButtonText->SetText(NSLOCTEXT("ObjectDescriptionUI", "Done", "Done"));
		}
		else {
			EditName(NameEditTextBox->GetText());
		}
	}
	UFUNCTION() void NameEditCommitted(const FText& Text, ETextCommit::Type CommitMethod)
	{
		if (CommitMethod == ETextCommit::Type::OnEnter)  {
			EditName(Text);
		}
	}
	void EditName(const FText& Text) {
		auto command = make_shared<FChangeName>();
		command->name = TrimStringF(Text.ToString(), 30);
		PUN_CHECK(simulation().descriptionUIState().objectType == ObjectTypeEnum::Building);
		command->objectId = simulation().descriptionUIState().objectId;
		networkInterface()->SendNetworkCommand(command);

		NameEditTextBox->SetVisibility(ESlateVisibility::Collapsed);
		DescriptionUITitle->SetVisibility(ESlateVisibility::Visible);
		NameEditButtonText->SetText(NSLOCTEXT("ObjDescUI", "Edit", "Edit"));
	}

	UFUNCTION() void OnClickBuildingSwapArrowLeftButton() {
		SwitchToNextBuilding(true);
	}
	UFUNCTION() void OnClickBuildingSwapArrowRightButton() {
		SwitchToNextBuilding(false);
	}
	
	
	UFUNCTION() void OnDropDownChanged(FString sItem, ESelectInfo::Type seltype);

};
