// Fill out your copyright notice in the Description page of Project Settings.

#include "BuildingJobUI.h"
#include "HumanSlotIcon.h"
#include "Components/Button.h"
#include "Components/SizeBox.h"
#include "Components/HorizontalBox.h"
#include "PunCity/Simulation/Building.h"
#include <memory>

using namespace std;

#define LOCTEXT_NAMESPACE "BuildingJobUI"

void UBuildingJobUI::PunInit(int buildingId, bool isHouse)
{
	_buildingId = buildingId;

	ArrowUp->OnClicked.Clear();
	ArrowDown->OnClicked.Clear();
	PriorityButton->OnClicked.Clear();
	NonPriorityButton->OnClicked.Clear();
	DisabledButton->OnClicked.Clear();

	ArrowUp->OnClicked.AddDynamic(this, &UBuildingJobUI::ArrowUpButtonDown);
	ArrowDown->OnClicked.AddDynamic(this, &UBuildingJobUI::ArrowDownButtonDown);

	PriorityButton->OnClicked.AddDynamic(this, &UBuildingJobUI::PriorityButtonDown);
	NonPriorityButton->OnClicked.AddDynamic(this, &UBuildingJobUI::NonPriorityButtonDown);
	DisabledButton->OnClicked.AddDynamic(this, &UBuildingJobUI::DisabledButtonDown);

	PriorityButton->SetVisibility(ESlateVisibility::Collapsed);
	NonPriorityButton->SetVisibility(ESlateVisibility::Visible);
	DisabledButton->SetVisibility(ESlateVisibility::Collapsed);

	DepletedText->SetVisibility(ESlateVisibility::Collapsed);

	TradeButton->OnClicked.Clear();
	TradeButton->OnClicked.AddDynamic(this, &UBuildingJobUI::OnClickTradeButton);
	TradeButton->SetVisibility(ESlateVisibility::Collapsed);

	BUTTON_ON_CLICK(StatisticsButton, this, &UBuildingJobUI::OnClickStatisticsButton);
	StatisticsButton->SetVisibility(ESlateVisibility::Collapsed);

	BUTTON_ON_CLICK(JobPriorityButton, this, &UBuildingJobUI::OnClickJobPriorityButton);
	JobPriorityButton->SetVisibility(ESlateVisibility::Collapsed);

	//if (isHouse)
	//{
	//	ArrowUp->SetVisibility(ESlateVisibility::Collapsed);
	//	ArrowDown->SetVisibility(ESlateVisibility::Collapsed);
	//	NonPriorityButton->SetVisibility(ESlateVisibility::Collapsed);
	//}
	SetShowHumanSlots(false, false);
	SetShowBar(false);

	ResourceCompletionIconBox->ClearChildren();
	OtherIconsBox->ClearChildren();

	ClockBox->SetVisibility(ESlateVisibility::Collapsed);

	PunBox->SetVisibility(ESlateVisibility::Collapsed);
	SetChildHUD(PunBox);

	_lastInputTime = -999.0f;
	_lastPriorityInputTime = -999.0f;
}

void UBuildingJobUI::SetSlots(int filledSlotCount, int allowedSlotCount, int slotCount, FLinearColor color, bool fromInput)
{
	// Don't set slots for a while after last user input
	if (!fromInput && UGameplayStatics::GetTimeSeconds(this) < _lastInputTime + NetworkInputDelayTime) {
		return;
	}
	
	_filledSlotCount = filledSlotCount;
	_allowedSlotCount = allowedSlotCount;
	_slotCount = slotCount;
	_slotColor = color;
	
	for (int i = 0; i < slotCount; i++) 
	{
		if (_humanIcons.Num() <= i) {
			auto humanIcon = AddWidget<UHumanSlotIcon>(UIEnum::JobHumanIcon);
			HumanSlots->AddChild(humanIcon);
			_humanIcons.Add(humanIcon);
		}

		_humanIcons[i]->SetVisibility(ESlateVisibility::SelfHitTestInvisible);
		_humanIcons[i]->SlotFiller->SetVisibility(i < filledSlotCount ? ESlateVisibility::SelfHitTestInvisible : ESlateVisibility::Hidden);
		_humanIcons[i]->SlotFillerImage->SetColorAndOpacity(color);
		_humanIcons[i]->SlotShadow->SetVisibility(_humanIcons[i]->SlotFiller->IsVisible() ? ESlateVisibility::SelfHitTestInvisible : ESlateVisibility::Hidden);
		
		_humanIcons[i]->SlotCross->SetVisibility(i < allowedSlotCount ? ESlateVisibility::Hidden : ESlateVisibility::SelfHitTestInvisible);
	}

	// Deactivate unused meshes
	for (int i = slotCount; i < _humanIcons.Num(); i++) {
		_humanIcons[i]->SetVisibility(ESlateVisibility::Collapsed);
	}
}

void UBuildingJobUI::SetConstructionResource(std::vector<int32> constructionResourcesCount, Building& building)
{
	std::vector<int32> constructionCosts = building.GetConstructionResourceCost();

	int index = 0;
	for (size_t i = 0; i < constructionCosts.size(); i++)
	{
		if (constructionCosts[i] > 0)
		{
			auto completionIcon = GetBoxChild<UResourceCompletionIcon>(ResourceCompletionIconBox, index, UIEnum::ResourceCompletionIcon, true);
			UMaterialInstanceDynamic* material = completionIcon->ResourceImage->GetDynamicMaterial();

			ResourceEnum resourceEnum = ConstructionResources[i];
			material->SetTextureParameterValue("ColorTexture", assetLoader()->GetResourceIcon(resourceEnum));
			material->SetTextureParameterValue("DepthTexture", assetLoader()->GetResourceIconAlpha(resourceEnum));

			float fraction = static_cast<float>(constructionResourcesCount[i]) / constructionCosts[i];
			material->SetScalarParameterValue("Fraction", fraction);
			material->SetScalarParameterValue("IsInput", 1.0f);

			material->SetScalarParameterValue("HasNoResource", fraction < 1.0f && simulation().resourceCount(building.playerId(), resourceEnum) == 0);

			completionIcon->SetIsPaused(building.priority() == PriorityEnum::Disable);

			TArray<FText> args;
			ADDTEXT_LOCTEXT("Construction Input", "Construction Input");
			ADDTEXT_INV_("<space>");
			ADDTEXT_(INVTEXT("{0} {1}/{2}"), ResourceNameT(resourceEnum), TEXT_NUM(constructionResourcesCount[i]), constructionCosts[i]);
			ADDTEXT_INV_("<space>");
			ADDTEXT_(LOCTEXT("Stored(city-wide): {0}", "Stored(city-wide): {0}"), TEXT_NUM(simulation().resourceCount(playerId(), resourceEnum)));

			auto tooltip = AddToolTip(completionIcon->ResourceImage, args);
			if (tooltip) {
				tooltip->TipSizeBox->SetMinDesiredWidth(200);
			}
		}
	}

	BoxAfterAdd(ResourceCompletionIconBox, index);
}

void UBuildingJobUI::SetTradeProgress(TradeBuilding& tradeBuilding, float fraction)
{
	int32 index = 0;

	/*
	 * Show completionIcon
	 */
	bool showClock = false;
	if (tradeBuilding.HasPendingTrade())
	{
		std::vector<ResourcePair> tradePairs = tradeBuilding.tradeResourcePairs();
		for (ResourcePair resourcePair : tradePairs)
		{
			if (resourcePair.count > 0)
			{
				auto completionIcon = GetBoxChild<UResourceCompletionIcon>(ResourceCompletionIconBox, index, UIEnum::ResourceCompletionIcon, true);
				UMaterialInstanceDynamic* material = completionIcon->ResourceImage->GetDynamicMaterial();

				ResourceEnum resourceEnum = resourcePair.resourceEnum;
				material->SetTextureParameterValue("ColorTexture", assetLoader()->GetResourceIcon(resourceEnum));
				material->SetTextureParameterValue("DepthTexture", assetLoader()->GetResourceIconAlpha(resourceEnum));

				material->SetScalarParameterValue("Fraction", fraction);
				material->SetScalarParameterValue("IsInput", 0.0f);
				material->SetScalarParameterValue("HasNoResource", 0.0f);

				completionIcon->SetIsPaused(false);

				TArray<FText> args;
				ADDTEXT__(ResourceNameT(resourceEnum));
				ADDTEXT_INV_("<space>");
				ADDTEXT_LOCTEXT("Bought resources\n", "Bought resources\n");
				ADDTEXT_(LOCTEXT("arriving in {0}s", "arriving in {0}s"), TEXT_NUM(tradeBuilding.CountdownSecondsDisplay()));
				AddToolTip(completionIcon->ResourceImage, args);
			}
		}

		// Display clock
		showClock = true;
		ClockImage->GetDynamicMaterial()->SetScalarParameterValue("IsGray", 0.0f);
		ClockImage->GetDynamicMaterial()->SetScalarParameterValue("Fraction", fraction);
		ClockPauseImage->SetVisibility(ESlateVisibility::Collapsed);
		ClockText->SetVisibility(ESlateVisibility::Collapsed);

		//auto completionIcon = GetBoxChild<UResourceCompletionIcon>(ResourceCompletionIconBox, index, UIEnum::ResourceCompletionIcon);
		//UMaterialInstanceDynamic* material = completionIcon->ResourceImage->GetDynamicMaterial();

		//material->SetTextureParameterValue("ColorTexture", assetLoader()->BlackIcon);
		//material->SetTextureParameterValue("DepthTexture", assetLoader()->BlackIcon);

		//material->SetScalarParameterValue("Fraction", fraction);
		//material->SetScalarParameterValue("IsInput", 0.0f);
		//material->SetScalarParameterValue("HasNoResource", 0.0f);
	}

	BoxAfterAdd(ResourceCompletionIconBox, index);

	/*
	 * Ready/NoStorage Icon
	 */
	if (OtherIconsBox->GetChildrenCount() == 0) {
		OtherIconsBox->AddChild(AddWidget<UUserWidget>(UIEnum::BuildingReadyIcon));
		OtherIconsBox->AddChild(AddWidget<UUserWidget>(UIEnum::BuildingNoStorageIcon));
		OtherIconsBox->AddChild(AddWidget<UUserWidget>(UIEnum::BuildingNeedSetupIcon));
	}

	auto setWarningIcons = [&](int32 indexIn) -> UWidget* {
		for (int32 i = 0; i < OtherIconsBox->GetChildrenCount(); i++) {
			OtherIconsBox->GetChildAt(i)->SetVisibility(indexIn == i ? ESlateVisibility::HitTestInvisible : ESlateVisibility::Collapsed);
		}
		return OtherIconsBox->GetChildAt(indexIn);
	};

	auto setWarningTradeBuildingFull = [&]()
	{
		auto textWidget = CastChecked<UPunTextWidget>(setWarningIcons(1));
		SetText(textWidget->PunText, tradeBuilding.buildingInfo().nameStd() + "\nStorage Full");
	};

	bool hasTradeButton = false;

	if (IsTradingPostLike(tradeBuilding.buildingEnum()))
	{
		if (tradeBuilding.CanTrade()) {
			setWarningIcons(-1);
			if (tradeBuilding.playerId() == playerId()) {
				hasTradeButton = true;
			}
		}
		else if (tradeBuilding.IsTradeBuildingFull()) {
			setWarningTradeBuildingFull();
		}
		else {
			setWarningIcons(-1);
		}
	}
	else
	{
		if (tradeBuilding.subclass<TradingCompany>(CardEnum::TradingCompany).needTradingCompanySetup) {
			setWarningIcons(2);
		}
		else if (tradeBuilding.IsTradeBuildingFull()) {
			setWarningTradeBuildingFull();
		}
		else {
			setWarningIcons(-1);

			// Show Gray Clock
			if (!showClock) {
				showClock = true;
				ClockImage->GetDynamicMaterial()->SetScalarParameterValue("IsGray", 1.0f);
				ClockImage->GetDynamicMaterial()->SetScalarParameterValue("Fraction", fraction);
				ClockPauseImage->SetVisibility(ESlateVisibility::Collapsed);
				ClockText->SetVisibility(ESlateVisibility::Collapsed);
			}
		}
	}

	// Note TradeButton gets its visibility set to Collapsed when it gets init
	TradeButton->SetVisibility(hasTradeButton ? ESlateVisibility::Visible : ESlateVisibility::Collapsed);

	ClockBox->SetVisibility(showClock ? ESlateVisibility::HitTestInvisible : ESlateVisibility::Collapsed);
}


void UBuildingJobUI::SetResourceCompletion(std::vector<ResourceEnum> inputs, std::vector<ResourceEnum> outputs, Building& building)
{
	int32 index = 0;
	for (size_t i = 0; i < inputs.size(); i++)
	{
		auto completionIcon = GetBoxChild<UResourceCompletionIcon>(ResourceCompletionIconBox, index, UIEnum::ResourceCompletionIcon, true);
		UMaterialInstanceDynamic* material = completionIcon->ResourceImage->GetDynamicMaterial();

		material->SetTextureParameterValue("ColorTexture", assetLoader()->GetResourceIcon(inputs[i]));
		material->SetTextureParameterValue("DepthTexture", assetLoader()->GetResourceIconAlpha(inputs[i]));

		int32 hasCount = building.resourceCount(building.input1());
		int32 needCount = building.inputPerBatch();

		material->SetScalarParameterValue("Fraction", static_cast<float>(hasCount) / needCount);
		material->SetScalarParameterValue("IsInput", 1.0f);
		material->SetScalarParameterValue("HasNoResource", hasCount < needCount && simulation().resourceCount(playerId(), inputs[i]) == 0);

		completionIcon->SetIsPaused(building.priority() == PriorityEnum::Disable);

		TArray<FText> args;
		ADDTEXT_LOCTEXT("Input", "Input");
		ADDTEXT_INV_("<space>");
		ADDTEXT_(INVTEXT("{0} {1}/{2}"), ResourceNameT(inputs[i]), TEXT_NUM(hasCount), TEXT_NUM(needCount));
		ADDTEXT_INV_("<space>");
		ADDTEXT_(LOCTEXT("Stored(city): {0}", "Stored(city): {0}"), TEXT_NUM(simulation().resourceCount(playerId(), inputs[i])));

		auto tooltip = AddToolTip(completionIcon->ResourceImage, args);
		if (tooltip) {
			tooltip->TipSizeBox->SetMinDesiredWidth(150);
		}
	}

	for (size_t i = 0; i < outputs.size(); i++)
	{
		auto completionIcon = GetBoxChild<UResourceCompletionIcon>(ResourceCompletionIconBox, index, UIEnum::ResourceCompletionIcon, true);
		UMaterialInstanceDynamic* material = completionIcon->ResourceImage->GetDynamicMaterial();

		if (outputs[i] == ResourceEnum::None) {
			// Show a blank completion when there is not output
			material->SetTextureParameterValue("ColorTexture", assetLoader()->BlackIcon);
			material->SetTextureParameterValue("DepthTexture", assetLoader()->BlackIcon);
		}
		else {
			material->SetTextureParameterValue("ColorTexture", assetLoader()->GetResourceIcon(outputs[i]));
			material->SetTextureParameterValue("DepthTexture", assetLoader()->GetResourceIconAlpha(outputs[i]));
		}

		float outputFraction = building.barFraction();
		material->SetScalarParameterValue("Fraction", outputFraction);
		material->SetScalarParameterValue("IsInput", 0.0f);
		material->SetScalarParameterValue("HasNoResource", 0.0f);

		completionIcon->SetIsPaused(building.priority() == PriorityEnum::Disable);

		//index++;

		TArray<FText> args;
		ADDTEXT_LOCTEXT("Output", "Output");
		ADDTEXT_INV_("<space>");
		ADDTEXT_(INVTEXT("{0} {1}"), ResourceNameT(outputs[i]), TEXT_PERCENT(static_cast<int>(outputFraction * 100)));

		auto tooltip = AddToolTip(completionIcon->ResourceImage, args);
		if (tooltip) {
			tooltip->TipSizeBox->SetMinDesiredWidth(150);
		}
	}

	BoxAfterAdd(ResourceCompletionIconBox, index);
}



void UBuildingJobUI::SetShowHumanSlots(bool isVisible, bool canManipulateOccupants, bool isTileBld)
{
	if (isVisible)
	{
		HumanSlotsUI->SetVisibility(ESlateVisibility::Visible);
		
		if (canManipulateOccupants) {
			PriorityEnum priority = building().priority();

			// Show priority button if it is available
			if (simulation().unlockSystem(playerId())->unlockedPriorityStar)
			{
				if (UGameplayStatics::GetTimeSeconds(this) > _lastPriorityInputTime + NetworkInputDelayTime) {
					SetPriorityButton(priority);
				}
			}
			else {
				ClosePriorityButton();
			}

			ArrowUp->SetVisibility(ESlateVisibility::Visible);
			ArrowDown->SetVisibility(ESlateVisibility::Visible);
		}
		else {
			ClosePriorityButton();
			//PriorityButton->SetVisibility(ESlateVisibility::Collapsed);
			//NonPriorityButton->SetVisibility(ESlateVisibility::Collapsed);
			//DisabledButton->SetVisibility(ESlateVisibility::Collapsed);

			ArrowUp->SetVisibility(ESlateVisibility::Collapsed);
			ArrowDown->SetVisibility(ESlateVisibility::Collapsed);
		}
	}
	else
	{
		HumanSlotsUI->SetVisibility(ESlateVisibility::Collapsed);
	}

	
	if (isTileBld) {
		HumanSlotsUI->SetRenderScale(FVector2D(0.7, 0.7));
	} else {
		HumanSlotsUI->SetRenderScale(FVector2D(1, 1));
	}
}


#undef LOCTEXT_NAMESPACE