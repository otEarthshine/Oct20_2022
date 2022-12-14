// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "PunTextWidget.h"
#include "PunScrollBoxWidget.h"
#include "BuildingResourceChain.h"
#include "IconTextPairWidget.h"
#include "PunButton.h"
#include "PunSpacerElement.h"
#include "PunRichTextTwoSided.h"
#include "ChooseResourceElement.h"
#include "PunDropdown.h"
#include "PunEditableNumberBox.h"
#include "ArmyRow.h"
#include "PunGraph.h"
#include "PunTutorialLink.h"
#include "ManageStorageElement.h"


#include "PunBoxWidget.generated.h"

/**
 * Manages the recycling of items so that we can easily do BeforeAdd(), Add... , AfterAdd() for UI manipulation
 */
UCLASS()
class UPunBoxWidget : public UPunWidget
{
	GENERATED_BODY()
public:
	
	void ResetBeforeAdd()
	{
		PunVerticalBox->ClearChildren();
		_elementEnums.clear();
		_elementHashes.clear();
	}

	void AfterAdd()
	{
		// Hide unused elements
		for (size_t i = currentIndex; i < _elementEnums.size(); i++) {
			PunVerticalBox->GetChildAt(i)->SetVisibility(ESlateVisibility::Collapsed);
		}
		currentIndex = 0;

		bElementResetThisRound = false;
	}

	bool DidElementResetThisRound() { return bElementResetThisRound; }

	int32 ChildrenCount() { return currentIndex; }

	template<typename T>
	T* GetChildElement(UIEnum uiEnum, int32 elementHash = 0)
	{
		// If we have children count less than currentIndex, even after AddChild, currentIndex will still be out of bound.
		PUN_CHECK(PunVerticalBox->GetChildrenCount() >= currentIndex);
		
		if (currentIndex < _elementEnums.size()) {
			// Replace
			if (_elementEnums[currentIndex] != uiEnum ||
				_elementHashes[currentIndex] != elementHash) 
			{
				// Since no proper non-editor-only solution, just remove all children up to this element and re-add...
				for (int32 i = PunVerticalBox->GetChildrenCount(); i-- > currentIndex;) {
					DespawnWidget(_elementEnums[i], PunVerticalBox->GetChildAt(i));
					PunVerticalBox->RemoveChildAt(i);
					_elementEnums.erase(_elementEnums.begin() + i);
					_elementHashes.erase(_elementHashes.begin() + i);
				}
				// Re-Add
				//PunVerticalBox->AddChild(AddWidget<T>(uiEnum));
				PunVerticalBox->AddChild(SpawnWidget<T>(uiEnum));
				_elementEnums.push_back(uiEnum);
				_elementHashes.push_back(elementHash);

				bElementResetThisRound = true;
			}
		} else {
			// Add
			//PunVerticalBox->AddChild(AddWidget<T>(uiEnum));
			PunVerticalBox->AddChild(SpawnWidget<T>(uiEnum));
			_elementEnums.push_back(uiEnum);
			_elementHashes.push_back(elementHash);

			bElementResetThisRound = true;
		}
		
		// !!! Error here could be caused by leaving items in PunBoxWidget !!!
		
		UWidget* childBeforeCast = PunVerticalBox->GetChildAt(currentIndex);
		T* child = CastChecked<T>(childBeforeCast);
		child->SetVisibility(ESlateVisibility::Visible);
		currentIndex++;

		// !!! Error here could be caused by leaving items in PunBoxWidget !!!

		PUN_CHECK(PunVerticalBox->GetChildrenCount() >= currentIndex);
		return child;
	}

	// Add Elements

	UPunTextWidget* AddText(std::string string) {
		auto textWidget = GetChildElement<UPunTextWidget>(UIEnum::PunTextWidget);
		textWidget->SetText(string);
		return textWidget;
	}
	UPunTextWidget* AddText(std::wstring string) {
		auto textWidget = GetChildElement<UPunTextWidget>(UIEnum::PunTextWidget);
		textWidget->SetText(string);
		return textWidget;
	}

	UPunTextWidget* AddText(std::stringstream& ss) {
		auto textWidget = AddText(ss.str());
		ss.str(std::string());
		return textWidget;
	}

	UPunRichText* AddRichText(std::stringstream& ss) {
		std::string str = ss.str();
		if (str.empty()) {
			return nullptr;
		}
		auto textWidget = AddRichText(str);
		ss.str(std::string());
		return textWidget;
	}
	
	UPunRichText* AddRichText(std::wstringstream& ss) {
		auto textWidget = AddRichText(ss.str());
		ss.str(std::wstring());
		return textWidget;
	}

	UPunRichText* AddSpecialRichText(FText prefix, TArray<FText>& args) {
		auto textWidget = AddRichText(FText::Format(INVTEXT("{0}{1}</>"), prefix, JOINTEXT(args)));
		args.Empty();
		return textWidget;
	}
	
	UPunRichText* AddRichText(std::string string) {
		auto textWidget = GetChildElement<UPunRichText>(UIEnum::PunRichText, std::hash<std::string>{}(string));
		textWidget->SetText(string);
		return textWidget;
	}
	UPunRichText* AddRichText(std::wstring string) {
		auto textWidget = GetChildElement<UPunRichText>(UIEnum::PunRichText, std::hash<std::wstring>{}(string));
		textWidget->SetText(string);
		return textWidget;
	}

	// FString
	UPunRichText* AddRichTextF(FString string) {
		auto textWidget = GetChildElement<UPunRichText>(UIEnum::PunRichText, GetTypeHash(string));
		textWidget->SetRichText(string);
		return textWidget;
	}
	UPunRichText* AddRichText(FText text) {
		auto textWidget = GetChildElement<UPunRichText>(UIEnum::PunRichText, GetTypeHash(text.ToString()));
		textWidget->PunRichText->SetText(text);
		return textWidget;
	}

	// std::string
	UPunRichTextTwoSided* AddRichText(std::string leftText, std::string rightText, ResourceEnum resourceEnum = ResourceEnum::None, std::string expandedText = "") {
		auto textWidget = GetChildElement<UPunRichTextTwoSided>(UIEnum::PunRichTextTwoSided);
		textWidget->SetText(leftText, rightText, resourceEnum, expandedText);
		return textWidget;
	}
	UPunRichTextTwoSided* AddRichText(std::string leftText, std::stringstream& ss) {
		auto textWidget = AddRichText(leftText, ss.str());
		ss.str(std::string());
		return textWidget;
	}

	// FText
	UPunRichTextTwoSided* AddRichText(FText leftText, FText rightText, ResourceEnum resourceEnum = ResourceEnum::None, FText expandedText = FText()) {
		auto textWidget = GetChildElement<UPunRichTextTwoSided>(UIEnum::PunRichTextTwoSided);
		textWidget->SetText(leftText, rightText, resourceEnum, expandedText);
		return textWidget;
	}
	UPunRichTextTwoSided* AddRichText(FText leftText, TArray<FText>& args) {
		auto textWidget = AddRichText(leftText, FText::Join(FText(), args));
		args.Empty();
		return textWidget;
	}
	UPunRichText* AddRichText(TArray<FText>& args) {
		auto textWidget = AddRichText(FText::Join(FText(), args));
		args.Empty();
		return textWidget;
	}


	

	UPunRichText* AddRichTextCenter(std::string string) {
		return AddRichText(string)->SetJustification(ETextJustify::Type::Center);
	}

	UPunRichText* AddRichTextBullet(std::string string, int32 sizeX) {
		auto textWidget = GetChildElement<UPunRichText>(UIEnum::PunRichTextBullet, std::hash<std::string>{}(string));
		textWidget->SetText(WrapString(string, sizeX));
		//PUN_LOG("AddRichTextBullet PunVerticalBox:%s", *PunVerticalBox->GetDesiredSize().ToString());
		//PUN_LOG("AddRichTextBullet textWidget:%s", *textWidget->GetDesiredSize().ToString());
		//PUN_LOG("AddRichTextBullet PunRichText:%s", *textWidget->PunRichText->GetDesiredSize().ToString());
		CastChecked<UVerticalBoxSlot>(textWidget->Slot)->SetHorizontalAlignment(HAlign_Fill);
		return textWidget;
	}
	
	void AddSpacer(int32 height = 5) {
		auto widget = GetChildElement<UPunSpacerElement>(UIEnum::PunSpacerWidget);
		widget->Spacer->SetSize(FVector2D(0, height));
	}

	void AddTextWithSpacer(std::stringstream& ss, int32_t height = 5) {
		AddText(ss);
		AddSpacer(height);
	}

	void AddLineSpacer(int32 height = 10) {
		auto widget = GetChildElement<UPunSpacerElement>(UIEnum::PunLineSpacerWidget);
		widget->Spacer->SetSize(FVector2D(0, height));
	}
	void AddThinLineSpacer(int32 height = 8) {
		auto widget = GetChildElement<UPunSpacerElement>(UIEnum::PunThinLineSpacerWidget);
		widget->Spacer->SetSize(FVector2D(0, height));
	}

	
	UPunScrollBoxWidget* AddScrollBox() {
		auto widget = GetChildElement<UPunScrollBoxWidget>(UIEnum::PunScrollBoxWidget);
		widget->PunBoxWidget->SetHUD(GetTPunHUD(), UIEnum::PunScrollBoxWidget);
		return widget;
	}

	void AddProductionChain(ResourcePair input1, ResourcePair input2, ResourcePair output, UTexture2D* productTexture = nullptr, std::string productStr = "") {
		auto widget = GetChildElement<UBuildingResourceChain>(UIEnum::BuildingResourceChain);
		widget->SetResource(input1, input2, output, productTexture, productStr);
	}

	UIconTextPairWidget* AddIconPair(FText prefix, ResourceEnum resourceEnum, FText suffix, bool isRed = false, bool hasShadow = false) {
		auto widget = GetChildElement<UIconTextPairWidget>(UIEnum::IconTextPair);
		widget->SetText(prefix, suffix);
		widget->SetImage(resourceEnum, dataSource()->assetLoader(), true);
		if (isRed) {
			widget->SetTextRed();
		}
		if (hasShadow) {
			widget->SetTextShadow();
		}
		return widget;
	}
	UIconTextPairWidget* AddIconPair(FText prefix, ResourceEnum resourceEnum, TArray<FText>& args, bool isRed = false, bool hasShadow = false) {
		auto widget = AddIconPair(prefix, resourceEnum, JOINTEXT(args), isRed, hasShadow);
		args.Empty();
		return widget;
	}

	UIconTextPairWidget* AddIconPair(FText prefix, UTexture2D* texture, FText suffix, bool isRed = false)
	{
		auto widget = GetChildElement<UIconTextPairWidget>(UIEnum::IconTextPair);
		widget->SetText(prefix, suffix);
		widget->SetImage(texture);
		if (isRed) {
			widget->SetTextRed();
		}
		return widget;
	}

	UPunButton* AddButton(FText prefix, UTexture2D* texture, FText suffix, UPunWidget* callbackParent, CallbackEnum callbackEnum,
							bool showEnabled = true, bool showExclamation = false, int32 callbackVar1In = -1, int32 callbackVar2In = -1)
	{
		return AddButtonBase(FText(), prefix, texture, suffix, callbackParent, callbackEnum, showEnabled, showExclamation, callbackVar1In, callbackVar2In);
	}
	UPunButton* AddButton2Lines(FText topString, UPunWidget* callbackParent, CallbackEnum callbackEnum,
		bool showEnabled = true, bool showExclamation = false, int32 callbackVar1In = -1, int32 callbackVar2In = -1)
	{
		auto widget = GetChildElement<UPunButton>(UIEnum::PunButton);
		widget->Set(topString, FText(), nullptr, FText(), callbackParent, callbackEnum, callbackVar1In, callbackVar2In);
		SetButtonEnabled(widget->Button, showEnabled ? ButtonStateEnum::Enabled : ButtonStateEnum::Disabled);
		widget->ExclamationIcon->SetShow(showExclamation);
		return widget;
	}
	UPunButton* AddButtonBase(FText topString, FText prefix, UTexture2D* texture, FText suffix, UPunWidget* callbackParent, CallbackEnum callbackEnum,
							bool showEnabled = true, bool showExclamation = false, int32 callbackVar1In = -1, int32 callbackVar2In = -1)
	{
		auto widget = GetChildElement<UPunButton>(UIEnum::PunButton);
		widget->Set(topString, prefix, texture, suffix, callbackParent, callbackEnum, callbackVar1In, callbackVar2In);
		SetButtonEnabled(widget->Button, showEnabled ? ButtonStateEnum::Enabled : ButtonStateEnum::Disabled);
		widget->ExclamationIcon->SetShow(showExclamation);
		return widget;
	}

	UPunButton* AddButtonRed(FText topString, FText prefix, UTexture2D* texture, FText suffix, UPunWidget* callbackParent, CallbackEnum callbackEnum,
							int32 callbackVar1In = -1, int32 callbackVar2In = -1)
	{
		auto widget = GetChildElement<UPunButton>(UIEnum::PunButton);
		widget->Set(topString, prefix, texture, suffix, callbackParent, callbackEnum, callbackVar1In, callbackVar2In);
		SetButtonEnabled(widget->Button, ButtonStateEnum::RedEnabled);
		widget->ExclamationIcon->SetShow(false);
		return widget;
	}

	UPunDropdown* AddDropdown(int32 objectId, TArray<FText> options, FText selectedOption,
								std::function<void(int32, FString, IGameUIDataSource*, IGameNetworkInterface*, int32)> onSelectOption, int32 dropdownIndex = 0)
	{
		auto widget = GetChildElement<UPunDropdown>(UIEnum::PunDropdown);
		widget->Set(objectId, options, selectedOption, onSelectOption);
		widget->dropdownIndex = dropdownIndex;
		return widget;
	}

	UPunEditableNumberBox* AddEditableNumberBox(UPunWidget* callbackParent, CallbackEnum callbackEnum, int32 objectId, std::string description, int32 amount, 
													std::string checkBoxEnabledDescription = "", bool isChecked = false, ResourceEnum resourceEnum = ResourceEnum::None)
	{
		auto widget = GetChildElement<UPunEditableNumberBox>(UIEnum::PunEditableNumberBox);
		if (widget->justInitialized) {
			widget->amount = amount;
			widget->UpdateText();
		}
		widget->Set(callbackParent, callbackEnum, objectId, description, checkBoxEnabledDescription, isChecked, resourceEnum);
		return widget;
	}

	UChooseResourceElement* AddChooseResourceElement(ResourceEnum resourceEnum, UPunWidget* callbackParent, CallbackEnum callbackEnum)
	{
		auto widget = GetChildElement<UChooseResourceElement>(UIEnum::ChooseResourceElement);
		widget->PunInit(callbackParent, callbackEnum, resourceEnum);
		return widget;
	}
	// Warning!!! AddChooseResourceElement and AddChooseResourceElement2 should not be used in the same box
	UChooseResourceElement* AddChooseResourceElement2(ResourceEnum resourceEnumIn, FText resourceStr, UPunWidget* callbackParent, CallbackEnum callbackEnum = CallbackEnum::None)
	{
		auto widget = GetChildElement<UChooseResourceElement>(UIEnum::ChooseResourceElement);
		widget->PunInit2(resourceEnumIn, resourceStr, callbackParent, callbackEnum);
		return widget;
	}

	UManageStorageElement* AddManageStorageElement(ResourceEnum resourceEnum, FText sectionName, int32 buildingId, ECheckBoxState checkBoxState, bool isSection, bool justOpenedUI, bool showTarget, int32 target = -1)
	{
		auto widget = GetChildElement<UManageStorageElement>(UIEnum::ManageStorageElement);
		widget->PunInit(resourceEnum, sectionName, buildingId, checkBoxState, isSection);

		if (justOpenedUI) {
			widget->TargetAmount_InitOnce(target);
		}
		widget->TargetAmount_Update(showTarget);
		
		return widget;
	}

	//UArmyRow* AddArmyRow(ArmyEnum armyEnum, std::string armyName, std::string armySize)
	//{
	//	auto widget = GetChildElement<UArmyRow>(UIEnum::ArmyRow);
	//	widget->PunInit(armyEnum, armyName, armySize);
	//	return widget;
	//}

	UPunGraph* AddGraph()
	{
		auto widget = GetChildElement<UPunGraph>(UIEnum::PunGraph);
		return widget;
	}

	UPunGraph* AddThinGraph()
	{
		auto widget = GetChildElement<UPunGraph>(UIEnum::PunThinGraph);
		return widget;
	}

	UPunTutorialLink* AddTutorialLink(TutorialLinkEnum linkEnum) {
		auto widget = GetChildElement<UPunTutorialLink>(UIEnum::PunTutorialLink);
		widget->SetLink(linkEnum);
		return widget;
	}

	

	/*
	 * Helper
	 */
	void AddRichTextParsed(std::string body)
	{
		std::string curBody;
		bool isBulletText = false;

		auto addNonEmptyRichText = [&](const std::string& str) {
			if (str.size() > 0) {
				AddRichText(curBody);
			}
		};

		for (int32 i = 0; i < body.size(); i++)
		{
			if (isBulletText &&
				body[i] == '<' &&
				body.substr(i, 3) == "</>")
			{
				AddRichTextBullet(curBody, 430);
				curBody.clear();
				isBulletText = false;
				i += 2;
			}
			else if (body[i] == '<' &&
				body.substr(i, 6) == "<link>")
			{
				addNonEmptyRichText(curBody);
				curBody.clear();
				i += 5;

				int32 linkEnumInt = FCString::Atoi(ToTChar(body.substr(i + 1, 2))); // i + 1 because i+=5 took into account i++
				AddTutorialLink(static_cast<TutorialLinkEnum>(linkEnumInt));
				
				i += 2;// Add increment 2 more indices since we are using 2 chars to determine linkEnum
			}
			
			else if (body[i] == '<' &&
				body.substr(i, 8) == "<bullet>")
			{
				addNonEmptyRichText(curBody);
				curBody.clear();
				isBulletText = true;
				i += 7;
			}
			else if (body[i] == '<' &&
				body.substr(i, 7) == "<space>")
			{
				addNonEmptyRichText(curBody);
				curBody.clear();
				AddSpacer(8);
				i += 6;
			}
			else if (body[i] == '<' &&
				body.substr(i, 6) == "<line>")
			{
				addNonEmptyRichText(curBody);
				curBody.clear();
				AddThinLineSpacer(8);
				i += 5;
			}
			else {
				curBody.push_back(body[i]);
			}
		}
		addNonEmptyRichText(curBody);
	}
	void AddRichTextParsed(std::stringstream& ss) {
		AddRichTextParsed(ss.str());
		ss.str("");
	}

	/*
	 * Tooltip helper
	 */

	static UToolTipWidgetBase* AddToolTip(UWidget* widget, UPunWidget* punWidgetSupport) {
		// Detect if tooltip was already created, if so don't recreate
		if (widget->ToolTipWidget) {
			auto tooltip = Cast<UToolTipWidgetBase>(widget->ToolTipWidget);
			if (tooltip) {
				return tooltip;
			}
		}

		auto tooltip = punWidgetSupport->AddWidget<UToolTipWidgetBase>(UIEnum::ToolTip);
		punWidgetSupport->SetChildHUD(tooltip->TooltipPunBoxWidget);
		widget->SetToolTip(tooltip);
		return tooltip;
	}
	
	static void AddBuildingTooltip(UWidget* widget, CardEnum buildingEnum, UPunWidget* punWidgetSupport, bool isPermanent);

	UPROPERTY(meta = (BindWidget)) UVerticalBox* PunVerticalBox;
	
private:
	int32 currentIndex = 0;
	std::vector<UIEnum> _elementEnums;
	std::vector<int32> _elementHashes;

	bool bElementResetThisRound = false; // The box widget got reset within this round (from last AfterAdd())
};
