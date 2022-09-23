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
	LEAN_PROFILING_WORLD_UI(TickWorldSpaceUI_BldJob_PunInit);
	
	_buildingId = buildingId;

	//ArrowUp->OnClicked.Clear();
	//ArrowDown->OnClicked.Clear();
	//PriorityButton->OnClicked.Clear();
	//NonPriorityButton->OnClicked.Clear();
	//DisabledButton->OnClicked.Clear();

	BUTTON_ON_CLICK(ArrowUp, this, &UBuildingJobUI::ArrowUpButtonDown);
	BUTTON_ON_CLICK(ArrowDown, this, &UBuildingJobUI::ArrowDownButtonDown);

	BUTTON_ON_CLICK(PriorityButton, this, &UBuildingJobUI::PriorityButtonDown);
	BUTTON_ON_CLICK(NonPriorityButton, this, &UBuildingJobUI::NonPriorityButtonDown);
	BUTTON_ON_CLICK(DisabledButton, this, &UBuildingJobUI::DisabledButtonDown);

	PriorityButton->SetVisibility(ESlateVisibility::Collapsed);
	NonPriorityButton->SetVisibility(ESlateVisibility::Visible);
	DisabledButton->SetVisibility(ESlateVisibility::Collapsed);

	//DepletedText->SetVisibility(ESlateVisibility::Collapsed);
	SetVisibility_DepletedText(false);

	//LargeWhiteText->SetVisibility(ESlateVisibility::Collapsed);
	//MediumGrayText->SetVisibility(ESlateVisibility::Collapsed);
	SetVisibility_LargeWhiteText(false);
	SetVisibility_MediumGrayText(false);

	TradeButton->OnClicked.Clear();
	TradeButton->OnClicked.AddDynamic(this, &UBuildingJobUI::OnClickTradeButton);
	TradeButton->SetVisibility(ESlateVisibility::Collapsed);

	BUTTON_ON_CLICK(StatisticsButton, this, &UBuildingJobUI::OnClickStatisticsButton);
	StatisticsButton->SetVisibility(ESlateVisibility::Collapsed);

	BUTTON_ON_CLICK(JobPriorityButton, this, &UBuildingJobUI::OnClickJobPriorityButton);
	JobPriorityButton->SetVisibility(ESlateVisibility::Collapsed);

	BUTTON_ON_CLICK(AutoTradeButton, this, &UBuildingJobUI::OnClickAutoTradeButton);
	AutoTradeButton->SetVisibility(ESlateVisibility::Collapsed);

	BUTTON_ON_CLICK(RevealSpyNestButton, this, &UBuildingJobUI::OnClickGenericButton);
	RevealSpyNestButton->SetVisibility(ESlateVisibility::Collapsed);

	BUTTON_ON_CLICK(ForeignAllowButton, this, &UBuildingJobUI::OnClickForeignAllowButton);
	BUTTON_ON_CLICK(ForeignDisallowButton, this, &UBuildingJobUI::OnClickForeignDisallowButton);

	AddToolTip(HumanSlotCount1, LOCTEXT("HumanSlotCount1 Tip", "Current Workers / Allowed Workers Slots"));
	AddToolTip(HumanSlotCount2, LOCTEXT("HumanSlotCount2 Tip", "(Max possible Worker Slots)"));

	//if (isHouse)
	//{
	//	ArrowUp->SetVisibility(ESlateVisibility::Collapsed);
	//	ArrowDown->SetVisibility(ESlateVisibility::Collapsed);
	//	NonPriorityButton->SetVisibility(ESlateVisibility::Collapsed);
	//}
	SetShowHumanSlots(false, false);
	SetShowBar(false);

	ResourceCompletionIconBox->ClearChildren();
	FarmIconBox->SetVisibility(ESlateVisibility::Collapsed);
	OtherIconsBox->ClearChildren();

	ClockBox->SetVisibility(ESlateVisibility::Collapsed);

	PunBox->SetVisibility(ESlateVisibility::Collapsed);
	SetChildHUD(PunBox);

	_lastInputTime = -999.0f;
	_lastPriorityInputTime = -999.0f;

	resourceIconsStateCache.resize(10);
}

void UBuildingJobUI::SetSlots(int filledSlotCount, int allowedSlotCount, int slotCount, FLinearColor color, bool fromInput)
{
	LEAN_PROFILING_WORLD_UI(TickWorldSpaceUI_BldJobSetSlots);
	
	// Don't set slots for a while after last user input
	if (!fromInput && UGameplayStatics::GetTimeSeconds(this) < _lastInputTime + NetworkInputDelayTime) {
		return;
	}
	
	_filledSlotCount = filledSlotCount;
	_allowedSlotCount = allowedSlotCount;
	_slotCount = slotCount;
	_slotColor = color;

	// Display people icons if there less than X slots
	if (slotCount <= 6)
	{
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

		// Hide Texts
		HumanSlotCountOverlay->SetVisibility(ESlateVisibility::Collapsed);
	}
	else
	{
		// Show
		HumanSlotCountOverlay->SetVisibility(ESlateVisibility::SelfHitTestInvisible);
		
		HumanSlotCount2->SetVisibility(allowedSlotCount != slotCount ? ESlateVisibility::SelfHitTestInvisible : ESlateVisibility::Collapsed);
		HumanSlotCount1->SetText(FText::Format(
			INVTEXT("{0}/{1}"),
			TEXT_NUM(filledSlotCount),
			TEXT_NUM(allowedSlotCount)
		));
		HumanSlotCount2->SetText(FText::Format(
			INVTEXT("({0})"),
			TEXT_NUM(slotCount)
		));

		// Deactivate all slots
		for (int i = 0; i < _humanIcons.Num(); i++) {
			_humanIcons[i]->SetVisibility(ESlateVisibility::Collapsed);
		}
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

			material->SetScalarParameterValue("HasNoResource", fraction < 1.0f && simulation().resourceCountTown(building.townId(), resourceEnum) == 0);

			completionIcon->SetIsPaused(building.priority() == PriorityEnum::Disable);

			if (completionIcon->ResourceImage->IsHovered())
			{
				TArray<FText> args;
				ADDTEXT_LOCTEXT("Construction Input", "Construction Input");
				ADDTEXT_INV_("<space>");

				ADDTEXT_(INVTEXT("{0} {1}/{2}"), ResourceNameT(resourceEnum), TEXT_NUM(constructionResourcesCount[i]), TEXT_NUM(constructionCosts[i]));
				ADDTEXT_INV_("<space>");
				ADDTEXT_(LOCTEXT("Stored(city): {0}", "Stored(city): {0}"), TEXT_NUM(simulation().resourceCountTownSafe(building.townId(), resourceEnum)));

				auto tooltip = AddToolTip(completionIcon->ResourceImage, args);
				if (tooltip) {
					tooltip->TipSizeBox->SetMinDesiredWidth(200);
				}
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

				if (completionIcon->ResourceImage->IsHovered())
				{
					TArray<FText> args;
					ADDTEXT_(
						LOCTEXT("Bought resources", "Bought {0} {1}<space>"), TEXT_NUM(resourcePair.count), ResourceNameT(resourceEnum)
					);
					ADDTEXT_(LOCTEXT("arriving in {0}s", "arriving in {0}s"), TEXT_NUM(tradeBuilding.CountdownSecondsDisplay()));
					AddToolTip(completionIcon->ResourceImage, args);
				}
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
		SetText(textWidget->PunText, FText::Format(LOCTEXT("TradeBuildingHoverWarning", "{0}\nStorage Full"), tradeBuilding.buildingInfo().name));
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
	LEAN_PROFILING_WORLD_UI(TickWorldSpaceUI_BldJobResourceComplete);

	if (building.isEnum(CardEnum::Farm) && outputs.size() > 0)
	{
		UMaterialInstanceDynamic* material = FarmIconBox->GetDynamicMaterial();
		material->SetTextureParameterValue("ColorTexture", assetLoader()->GetResourceIcon(outputs[0]));
		material->SetTextureParameterValue("DepthTexture", assetLoader()->GetResourceIconAlpha(outputs[0]));
		FarmIconBox->SetVisibility(ESlateVisibility::Visible);

		if (FarmIconBox->IsHovered())
		{
			TArray<FText> args;
			ADDTEXT_(LOCTEXT("Farm Hover Icon Tip", "{0} Farm"), ResourceNameT(outputs[0]));

			auto tooltip = AddToolTip(FarmIconBox, args);
			if (tooltip) {
				tooltip->TipSizeBox->SetMinDesiredWidth(150);
			}
		}
		return;
	}
	
	
	int32 index = 0;

	// Show input if it is not filled yet
	if (!building.filledInputs())
	{
		for (size_t i = 0; i < inputs.size(); i++)
		{
			LEAN_PROFILING_WORLD_UI(TickWorldSpaceUI_BldJobResourceComplete_In)
			
			int32 hasCount;
			int32 needCount;
			{
				LEAN_PROFILING_WORLD_UI(TickWorldSpaceUI_TEST3);

				hasCount = building.resourceCount(inputs[i]);
				needCount = building.inputPerBatch(inputs[i]);
			}
			
			UResourceCompletionIcon* completionIcon;
			{
				LEAN_PROFILING_WORLD_UI(TickWorldSpaceUI_TEST1);
				completionIcon = GetBoxChild<UResourceCompletionIcon>(ResourceCompletionIconBox, index, UIEnum::ResourceCompletionIcon, true);
			}

			PriorityEnum priority = building.priority();
			
			if (IsStateChanged(1 + 2 * static_cast<uint32>(priority) + 6 * static_cast<uint32>(inputs[i]) + 1800 * hasCount + (1800 * 300) * needCount, resourceIconsStateCache[index]))
			{
				LEAN_PROFILING_WORLD_UI(TickWorldSpaceUI_TEST2);
				
				UMaterialInstanceDynamic* material = completionIcon->ResourceImage->GetDynamicMaterial();
				material->SetScalarParameterValue("Fraction", static_cast<float>(hasCount) / needCount);
				material->SetScalarParameterValue("HasNoResource", hasCount < needCount && simulation().resourceCountTown(building.townId(), inputs[i]) == 0);

				material->SetTextureParameterValue("ColorTexture", assetLoader()->GetResourceIcon(inputs[i]));
				material->SetTextureParameterValue("DepthTexture", assetLoader()->GetResourceIconAlpha(inputs[i]));
				material->SetScalarParameterValue("IsInput", 1.0f);

				completionIcon->SetIsPaused(priority == PriorityEnum::Disable);
			}

			if (completionIcon->ResourceImage->IsHovered())
			{
				LEAN_PROFILING_WORLD_UI(TickWorldSpaceUI_TEST6);
				
				TArray<FText> args;
				ADDTEXT_LOCTEXT("Input", "Input");
				ADDTEXT_INV_("<space>");
				ADDTEXT_(INVTEXT("{0} {1}/{2}"), ResourceNameT(inputs[i]), TEXT_NUM(hasCount), TEXT_NUM(needCount));
				ADDTEXT_INV_("<space>");
				ADDTEXT_(LOCTEXT("Stored(city): {0}", "Stored(city): {0}"), TEXT_NUM(simulation().resourceCountTown(building.townId(), inputs[i])));

				auto tooltip = AddToolTip(completionIcon->ResourceImage, args);
				if (tooltip) {
					tooltip->TipSizeBox->SetMinDesiredWidth(150);
				}
			}
		}
	}

	for (size_t i = 0; i < outputs.size(); i++)
	{
		LEAN_PROFILING_WORLD_UI(TickWorldSpaceUI_BldJobResourceComplete_Out)
		
		auto completionIcon = GetBoxChild<UResourceCompletionIcon>(ResourceCompletionIconBox, index, UIEnum::ResourceCompletionIcon, true);

		ResourceEnum outputEnum = outputs[i];
		float outputFraction = building.barFraction();
		PriorityEnum priority = building.priority();

		if (IsStateChanged(0 + 2 * static_cast<uint32>(priority) + 6 * static_cast<uint32>(outputEnum) + 1800 * static_cast<uint32>(outputFraction * 100), resourceIconsStateCache[index]))
		{
			UMaterialInstanceDynamic* material = completionIcon->ResourceImage->GetDynamicMaterial();

			if (outputEnum == ResourceEnum::None) {
				// Show a blank completion when there is not output
				material->SetTextureParameterValue("ColorTexture", assetLoader()->BlackIcon);
				material->SetTextureParameterValue("DepthTexture", assetLoader()->BlackIcon);
			}
			else {
				material->SetTextureParameterValue("ColorTexture", assetLoader()->GetResourceIcon(outputEnum));
				material->SetTextureParameterValue("DepthTexture", assetLoader()->GetResourceIconAlpha(outputEnum));
			}

			material->SetScalarParameterValue("Fraction", outputFraction);
			material->SetScalarParameterValue("IsInput", 0.0f);
			material->SetScalarParameterValue("HasNoResource", 0.0f);

			completionIcon->SetIsPaused(priority == PriorityEnum::Disable);
		}

		//index++;

		if (completionIcon->ResourceImage->IsHovered())
		{
			TArray<FText> args;
			ADDTEXT_LOCTEXT("Output", "Output");
			ADDTEXT_INV_("<space>");
			ADDTEXT_(INVTEXT("{0} {1}"), ResourceNameT(outputs[i]), TEXT_PERCENT(static_cast<int>(outputFraction * 100)));

			auto tooltip = AddToolTip(completionIcon->ResourceImage, args);
			if (tooltip) {
				tooltip->TipSizeBox->SetMinDesiredWidth(150);
			}
		}
	}

	BoxAfterAdd(ResourceCompletionIconBox, index);
}



void UBuildingJobUI::SetShowHumanSlots(bool isVisible, bool canManipulateOccupants, bool isTileBld)
{
	LEAN_PROFILING_WORLD_UI(TickWorldSpaceUI_BldJobHumanSlots);
	
	if (isVisible)
	{
		//HumanSlotsUI->SetVisibility(ESlateVisibility::Visible);
		SetVisibility_HumanSlotsUI(true);
		
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
		//HumanSlotsUI->SetVisibility(ESlateVisibility::Collapsed);
		SetVisibility_HumanSlotsUI(false);
	}

	
	if (isTileBld) {
		HumanSlotsUI->SetRenderScale(FVector2D(0.7, 0.7));
	} else {
		HumanSlotsUI->SetRenderScale(FVector2D(1, 1));
	}
}


#undef LOCTEXT_NAMESPACE