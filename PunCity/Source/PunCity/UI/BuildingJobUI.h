// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "PunWidget.h"

#include "PunCity/Simulation/GameSimulationCore.h"

#include "Materials/MaterialInstanceDynamic.h"
#include "ResourceCompletionIcon.h"
#include "PunCity/Simulation/Buildings/GathererHut.h"
#include "PunTextWidget.h"

#include "BuildingJobUI.generated.h"
/**
 * 
 */
UCLASS()
class UBuildingJobUI : public UPunWidget
{
	GENERATED_BODY()
public:
	UPROPERTY(meta = (BindWidget)) UHorizontalBox* HumanSlotsUI;
	
	UPROPERTY(meta = (BindWidget)) UButton* ArrowUp;
	UPROPERTY(meta = (BindWidget)) UButton* ArrowDown;
	UPROPERTY(meta = (BindWidget)) UHorizontalBox* HumanSlots;

	UPROPERTY(meta = (BindWidget)) UButton* PriorityButton;
	UPROPERTY(meta = (BindWidget)) UButton* NonPriorityButton;
	UPROPERTY(meta = (BindWidget)) UButton* DisabledButton;

	UPROPERTY(meta = (BindWidget)) UImage* ProductionBar;
	UPROPERTY(meta = (BindWidget)) UOverlay* ProductionBarOverlay;

	UPROPERTY(meta = (BindWidget)) UImage* HouseUpgradeCountdownBar;

	UPROPERTY(meta = (BindWidget)) USizeBox* ClockBox;
	UPROPERTY(meta = (BindWidget)) UImage* ClockImage;
	UPROPERTY(meta = (BindWidget)) UTextBlock* ClockText;
	
	UPROPERTY(meta = (BindWidget)) UHorizontalBox* ResourceCompletionIconBox;
	UPROPERTY(meta = (BindWidget)) UHorizontalBox* OtherIconsBox;

	UPROPERTY(meta = (BindWidget)) USizeBox* SpeedBoostIcon;
	UPROPERTY(meta = (BindWidget)) UButton* TradeButton;

	UPROPERTY(meta = (BindWidget)) UButton* StatisticsButton;
	UPROPERTY(meta = (BindWidget)) UButton* JobPriorityButton;

public:
	void PunInit(int buildingId, bool isHouse);

	void SetShowHumanSlots(bool isVisible, bool canManipulateOccupants, bool isTileBld = false);
	void SetShowBar(bool showBar, bool showHouseUpgradeBar = false) {
		ProductionBarOverlay->SetVisibility(showBar ? ESlateVisibility::HitTestInvisible : ESlateVisibility::Collapsed);
		HouseUpgradeCountdownBar->SetVisibility(showHouseUpgradeBar ? ESlateVisibility::HitTestInvisible : ESlateVisibility::Collapsed);
	}
	void SetSlots(int filledSlotCount, int allowedSlotCount, int slotCount, FLinearColor color, bool fromInput = false);

	int buildingId() { return _buildingId; }

	void SetBarFraction(float fraction) {
		ProductionBar->GetDynamicMaterial()->SetScalarParameterValue(FName("Fraction"), fraction);
		ProductionBarOverlay->SetVisibility(ESlateVisibility::SelfHitTestInvisible);
	}

	void SetHouseBarFraction(float fraction) {
		HouseUpgradeCountdownBar->GetDynamicMaterial()->SetScalarParameterValue(FName("Fraction"), fraction);
	}
	
	void SetConstructionResource(std::vector<int32> constructionResourcesCount, Building& building)
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

				std::stringstream ss;
				ss << ResourceName(resourceEnum) << " " << constructionResourcesCount[i] << "/" << constructionCosts[i];
				ss << "<space>";
				ss << "Construction input";
				auto tooltip = AddToolTip(completionIcon->ResourceImage, ss.str());
				if (tooltip) {
					tooltip->TipSizeBox->SetMinDesiredWidth(200);
				}
			}
		}

		BoxAfterAdd(ResourceCompletionIconBox, index);
	}

	void SetSpeedBoost(Building& building)
	{
		bool hasSpeedBoost = simulation().playerOwned(playerId()).HasSpeedBoost(building.buildingId());
		SpeedBoostIcon->SetVisibility(hasSpeedBoost ? ESlateVisibility::HitTestInvisible : ESlateVisibility::Collapsed);
	}

	void SetBuildingStatus(Building& building)
	{
		if (IsTradingPostLike(building.buildingEnum()) ||
			building.isEnum(CardEnum::TradingCompany)) {
			SetTradeProgress(building.subclass<TradeBuilding>(), building.barFraction());
		}
		//else if (IsBarrack(building.buildingEnum())) {
		//	Barrack& barrack = building.subclass<Barrack>();
		//	SetProgress(barrack.trainingPercent() / 100.0f, barrack.queueCount());
		//}
		else if (IsSpecialProducer(building.buildingEnum())) {
			SetProgress(building.barFraction());
		}
		else if (building.isEnum(CardEnum::StatisticsBureau)) {
			// Note StatisticsButton gets its visibility set to Collapsed when it gets init
			StatisticsButton->SetVisibility(ESlateVisibility::Visible);
		}
		else if (building.isEnum(CardEnum::JobManagementBureau)) {
			// Note JobPriorityButton gets its visibility set to Collapsed when it gets init
			JobPriorityButton->SetVisibility(ESlateVisibility::Visible);
		}
		else {
			std::vector<float> inputFractions;
			if (building.hasInput1()) {
				inputFractions.push_back(static_cast<float>(building.resourceCount(building.input1())) / building.inputPerBatch());
			}
			if (building.hasInput2()) {
				inputFractions.push_back(static_cast<float>(building.resourceCount(building.input2())) / building.inputPerBatch());
			}
			SetResourceCompletion(building.inputs(), inputFractions, { building.product() }, { building.barFraction() });
		}
	}
	
	void SetTradeProgress(TradeBuilding& tradeBuilding, float fraction)
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

					std::stringstream ss;
					ss << ResourceName(resourceEnum);
					ss << "<space>";
					ss << "Bought resources\n";
					ss << "arriving in " << tradeBuilding.CountdownSecondsDisplay() << "s";
					AddToolTip(completionIcon->ResourceImage, ss.str());
				}
			}

			// Display clock
			showClock = true;
			ClockImage->GetDynamicMaterial()->SetScalarParameterValue("IsGray", 0.0f);
			ClockImage->GetDynamicMaterial()->SetScalarParameterValue("Fraction", fraction);
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
			SetText(textWidget->PunText, tradeBuilding.buildingInfo().name + "\nStorage Full");
		};

		bool hasTradeButton = false;
		
		if (IsTradingPostLike(tradeBuilding.buildingEnum()))
		{
			if (tradeBuilding.CanTrade()) {
				setWarningIcons(-1);
				hasTradeButton = true;
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
					ClockText->SetVisibility(ESlateVisibility::Collapsed);
				}
			}
		}

		// Note TradeButton gets its visibility set to Collapsed when it gets init
		TradeButton->SetVisibility(hasTradeButton? ESlateVisibility::Visible : ESlateVisibility::Collapsed);

		ClockBox->SetVisibility(showClock ? ESlateVisibility::HitTestInvisible : ESlateVisibility::Collapsed);
	}
	void SetProgress(float fraction, int32 clockCount = 0)
	{
		int32 index = 0;
		BoxAfterAdd(ResourceCompletionIconBox, index);
		
		// Only need progress clock...
		for (int32 i = 0; i < OtherIconsBox->GetChildrenCount(); i++) {
			OtherIconsBox->GetChildAt(i)->SetVisibility(ESlateVisibility::Collapsed);
		}

		ClockImage->GetDynamicMaterial()->SetScalarParameterValue("IsGray", 0.0f);
		ClockImage->GetDynamicMaterial()->SetScalarParameterValue("Fraction", fraction);
		ClockBox->SetVisibility((clockCount > 0 || fraction > 0.01f) ? ESlateVisibility::HitTestInvisible : ESlateVisibility::Collapsed);

		if (clockCount > 0) {
			ClockText->SetVisibility(ESlateVisibility::SelfHitTestInvisible);
			SetText(ClockText, to_string(clockCount));
		} else {
			ClockText->SetVisibility(ESlateVisibility::Collapsed);
		}
	}
	void SetResourceCompletion(std::vector<ResourceEnum> inputs, std::vector<float> inputFractions,
							   std::vector<ResourceEnum> outputs, std::vector<float> outputFractions)
	{
		int32 index = 0;
		for (size_t i = 0; i < inputs.size(); i++)
		{
			auto completionIcon = GetBoxChild<UResourceCompletionIcon>(ResourceCompletionIconBox, index, UIEnum::ResourceCompletionIcon, true);
			UMaterialInstanceDynamic* material = completionIcon->ResourceImage->GetDynamicMaterial();

			material->SetTextureParameterValue("ColorTexture", assetLoader()->GetResourceIcon(inputs[i]));
			material->SetTextureParameterValue("DepthTexture", assetLoader()->GetResourceIconAlpha(inputs[i]));

			material->SetScalarParameterValue("Fraction", inputFractions[i]);
			material->SetScalarParameterValue("IsInput", 1.0f);
			material->SetScalarParameterValue("HasNoResource", inputFractions[i] < 1.0f && simulation().resourceCount(playerId(), inputs[i]) == 0);

			std::stringstream ss;
			ss << ResourceName(inputs[i]);
			ss << "<space>";
			ss << "Input";
			auto tooltip = AddToolTip(completionIcon->ResourceImage, ss.str());
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
			} else {
				material->SetTextureParameterValue("ColorTexture", assetLoader()->GetResourceIcon(outputs[i]));
				material->SetTextureParameterValue("DepthTexture", assetLoader()->GetResourceIconAlpha(outputs[i]));
			}

			material->SetScalarParameterValue("Fraction", outputFractions[i]);
			material->SetScalarParameterValue("IsInput", 0.0f);
			material->SetScalarParameterValue("HasNoResource", 0.0f);

			index++;

			std::stringstream ss;
			ss << ResourceName(outputs[i]);
			ss << "<space>";
			ss << "Output";
			auto tooltip = AddToolTip(completionIcon->ResourceImage, ss.str());
			if (tooltip) {
				tooltip->TipSizeBox->SetMinDesiredWidth(150);
			}
		}

		BoxAfterAdd(ResourceCompletionIconBox, index);
	}
	

	void ClearResourceCompletionBox() {
		for (int32 i = 0; i < ResourceCompletionIconBox->GetChildrenCount(); i++) {
			ResourceCompletionIconBox->GetChildAt(i)->SetVisibility(ESlateVisibility::Collapsed);
		}
	}

private:
	UFUNCTION() void ArrowUpButtonDown() { ArrowButtonDown(true);  }
	UFUNCTION() void ArrowDownButtonDown() { ArrowButtonDown(false); }

	void ArrowButtonDown(bool upOrDown) {
		if (InterfacesInvalid()) return;

		_allowedSlotCount = Clamp(_allowedSlotCount + (upOrDown ? 1 : -1), 0, _slotCount);

		PUN_LOG("ArrowButton %d", _allowedSlotCount);
		auto jobSlotChange = std::make_shared<FJobSlotChange>();
		jobSlotChange->buildingId = _buildingId;
		jobSlotChange->allowedOccupants = _allowedSlotCount;
		networkInterface()->SendNetworkCommand(jobSlotChange);

		SetSlots(std::min(_filledSlotCount, _allowedSlotCount), _allowedSlotCount, _slotCount, _slotColor, true);
		_lastInputTime = UGameplayStatics::GetTimeSeconds(this);

		dataSource()->Spawn2DSound("UI", "UIIncrementalChange");
	}

	UFUNCTION() void PriorityButtonDown() {
		ChangePriorityFromButton(PriorityEnum::Disable);
	}
	UFUNCTION() void NonPriorityButtonDown() {
		ChangePriorityFromButton(PriorityEnum::Priority);
	}
	UFUNCTION() void DisabledButtonDown() {
		ChangePriorityFromButton(PriorityEnum::NonPriority);
	}

	UFUNCTION() void OnClickTradeButton()
	{
		Building& building = dataSource()->simulation().building(_buildingId);
		PUN_CHECK(IsTradingPostLike(building.buildingEnum()));

		if (static_cast<TradingPost*>(&building)->CanTrade()) {
			GetPunHUD()->OpenTradeUI(_buildingId);
		}
	}

	UFUNCTION() void OnClickStatisticsButton() {
		GetPunHUD()->OpenStatisticsUI(playerId());
	}
	UFUNCTION() void OnClickJobPriorityButton() {
		GetPunHUD()->OpenJobPriorityUI();
	}

	void SetPriorityButton(PriorityEnum priority)
	{
		PriorityButton->SetVisibility(priority == PriorityEnum::Priority ? ESlateVisibility::Visible : ESlateVisibility::Collapsed);
		NonPriorityButton->SetVisibility(priority == PriorityEnum::NonPriority ? ESlateVisibility::Visible : ESlateVisibility::Collapsed);
		DisabledButton->SetVisibility(priority == PriorityEnum::Disable ? ESlateVisibility::Visible : ESlateVisibility::Collapsed);
	}
	void ClosePriorityButton()
	{
		PriorityButton->SetVisibility(ESlateVisibility::Collapsed);
		NonPriorityButton->SetVisibility(ESlateVisibility::Collapsed);
		DisabledButton->SetVisibility(ESlateVisibility::Collapsed);
	}

	void ChangePriorityFromButton(PriorityEnum priority)
	{
		auto command = std::make_shared<FSetPriority>();
		command->buildingId = _buildingId;
		command->priority = static_cast<int32>(priority);
		networkInterface()->SendNetworkCommand(command);

		SetPriorityButton(priority);
		_lastPriorityInputTime = UGameplayStatics::GetTimeSeconds(this);
	}
	

	Building& building() { return dataSource()->simulation().building(_buildingId); }

	int _buildingId = -1;

	UPROPERTY() TArray<class UHumanSlotIcon*> _humanIcons;
	//int _filledSlotCount = 0;

	
	float _lastInputTime = -999.0f;
	float _lastPriorityInputTime = -999.0f;
	
	int32 _filledSlotCount = 0;
	int32 _allowedSlotCount = 0;
	int32 _slotCount = 0;
	FLinearColor _slotColor;
};
