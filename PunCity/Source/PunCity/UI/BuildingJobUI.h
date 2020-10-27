// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "PunBoxWidget.h"

#include "PunCity/Simulation/GameSimulationCore.h"

#include "Materials/MaterialInstanceDynamic.h"
#include "ResourceCompletionIcon.h"
#include "PunCity/Simulation/Buildings/GathererHut.h"
#include "PunTextWidget.h"
#include "IconTextPairWidget.h"

#include "BuildingJobUI.generated.h"

enum class JobUIState {
	None,
	Job,
	Home,
	Storage,
};

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
	UPROPERTY(meta = (BindWidget)) UImage* ClockPauseImage;
	UPROPERTY(meta = (BindWidget)) UTextBlock* ClockText;
	
	UPROPERTY(meta = (BindWidget)) UHorizontalBox* ResourceCompletionIconBox;
	UPROPERTY(meta = (BindWidget)) UHorizontalBox* OtherIconsBox;

	UPROPERTY(meta = (BindWidget)) UTextBlock* DepletedText;

	UPROPERTY(meta = (BindWidget)) USizeBox* SpeedBoostIcon;
	UPROPERTY(meta = (BindWidget)) UButton* TradeButton;

	UPROPERTY(meta = (BindWidget)) UButton* StatisticsButton;
	UPROPERTY(meta = (BindWidget)) UButton* JobPriorityButton;

	UPROPERTY(meta = (BindWidget)) UPunBoxWidget* PunBox;

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

				completionIcon->SetIsPaused(building.priority() == PriorityEnum::Disable);

				std::stringstream ss;
				ss << "Construction Input<space>";
				ss << ResourceName(resourceEnum) << " " << constructionResourcesCount[i] << "/" << constructionCosts[i];
				ss << "<space>Stored(city): " << simulation().resourceCount(playerId(), resourceEnum);
				
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

	void SetBuildingStatus(Building& building, JobUIState jobUIState)
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
			SetSpecialProducerProgress(building.barFraction(), building);
		}
		else if (building.isEnum(CardEnum::StatisticsBureau)) {
			// Note StatisticsButton gets its visibility set to Collapsed when it gets init
			if (building.ownedBy(playerId())) {
				StatisticsButton->SetVisibility(ESlateVisibility::Visible);
			}
		}
		else if (building.isEnum(CardEnum::JobManagementBureau)) {
			// Note JobPriorityButton gets its visibility set to Collapsed when it gets init
			if (building.ownedBy(playerId())) {
				JobPriorityButton->SetVisibility(ESlateVisibility::Visible);
			}
		}
		else if (IsStorage(building.buildingEnum()) ||
			building.isEnum(CardEnum::FruitGatherer) ||
			building.isEnum(CardEnum::HuntingLodge)) 
		{
			// Show items above warehouse
			if (jobUIState == JobUIState::Storage &&
				building.isEnum(CardEnum::Warehouse)) 
			{
				PunBox->SetVisibility(ESlateVisibility::SelfHitTestInvisible);
				const std::vector<ResourceHolderInfo>& holderInfos = building.subclass<StorageYard>().holderInfos();

				std::vector<ResourcePair> maxResourcePairs;
				bool hasMore = false;

				for (ResourceHolderInfo holderInfo : holderInfos)
				{
					int32 resourceCount = building.GetResourceCount(holderInfo);
					if  (resourceCount > 0)
					{
						bool inserted = false;
						for (size_t i = 0; i < maxResourcePairs.size(); i++) {
							if (resourceCount > maxResourcePairs[i].count) {
								maxResourcePairs.insert(maxResourcePairs.begin() + i, { holderInfo.resourceEnum, resourceCount });
								hasMore = true;
								inserted = true;
								break;
							}
						}
						if (!inserted) {
							maxResourcePairs.push_back({ holderInfo.resourceEnum, resourceCount });
						}
						if (maxResourcePairs.size() > 2) {
							maxResourcePairs.pop_back();
						}
					}
				}

				for (ResourcePair& pair : maxResourcePairs) {
					PunBox->AddIconPair("", pair.resourceEnum, std::to_string(pair.count), false, true);
				}
				if (hasMore) {
					auto textWidget = PunBox->AddText("...");
					textWidget->PunText->SetShadowOffset(FVector2D(1, 1));
					textWidget->PunText->SetShadowColorAndOpacity(FLinearColor(0, 0, 0, 0.5));
					CastChecked<UVerticalBoxSlot>(textWidget->Slot)->SetHorizontalAlignment(HAlign_Center);
				}
				
				PunBox->AfterAdd();
			}
		}
		else {
			//std::vector<float> inputFractions;
			//if (building.hasInput1()) {
			//	inputFractions.push_back(static_cast<float>(building.resourceCount(building.input1())) / building.inputPerBatch());
			//}
			//if (building.hasInput2()) {
			//	inputFractions.push_back(static_cast<float>(building.resourceCount(building.input2())) / building.inputPerBatch());
			//}

			std::vector<ResourceEnum> outputs = { building.product() };
			//std::vector<float> outputFractions = { building.barFraction() };

			// Beeswax special case
			if (outputs[0] == ResourceEnum::Beeswax) {
				outputs.push_back(ResourceEnum::Honey);
				//outputFractions.push_back(building.barFraction());
			}

			// MountainMine DepletedText
			//if (IsMountainMine(building.buildingEnum()) &&
			//	building.subclass<Mine>().oreLeft() <= 0)
			//{
			//	DepletedText->SetVisibility(ESlateVisibility::SelfHitTestInvisible);
			//	SetText(DepletedText, "Depleted");
			//}

			SetResourceCompletion(building.inputs(), outputs, building);
		}

		// Refresh Hover Warning
		// Check every sec
		float time = UGameplayStatics::GetTimeSeconds(this);
		if (building.ownedBy(playerId()))
		{
			if (time - building.lastHoverWarningCheckTime >= 1.0f)
			{
				building.lastHoverWarningCheckTime = time;
				building.RefreshHoverWarning();

				if (building.hoverWarning != HoverWarning::None)
				{
					PUN_LOG("Hover Warning %s warningId:%d", ToTChar(building.buildingInfo().name), static_cast<int>(building.hoverWarning));
					
					DepletedText->SetVisibility(ESlateVisibility::SelfHitTestInvisible);
					SetText(DepletedText, GetHoverWarningString(building.hoverWarning));
				}
				else {
					DepletedText->SetVisibility(ESlateVisibility::Collapsed);
				}
			}
		}
		else
		{
			building.hoverWarning = HoverWarning::None;
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
					ClockText->SetVisibility(ESlateVisibility::Collapsed);
				}
			}
		}

		// Note TradeButton gets its visibility set to Collapsed when it gets init
		TradeButton->SetVisibility(hasTradeButton? ESlateVisibility::Visible : ESlateVisibility::Collapsed);

		ClockBox->SetVisibility(showClock ? ESlateVisibility::HitTestInvisible : ESlateVisibility::Collapsed);
	}
	void SetSpecialProducerProgress(float fraction, Building& building, int32 clockCount = 0)
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

		// Show pause
		bool isPaused = building.priority() == PriorityEnum::Disable;
		ClockPauseImage->SetVisibility(isPaused ? ESlateVisibility::SelfHitTestInvisible : ESlateVisibility::Collapsed);
		ClockImage->GetDynamicMaterial()->SetScalarParameterValue("IsPauseGray", isPaused);

		if (clockCount > 0) {
			ClockText->SetVisibility(ESlateVisibility::SelfHitTestInvisible);
			SetText(ClockText, to_string(clockCount));
		} else {
			ClockText->SetVisibility(ESlateVisibility::Collapsed);
		}

		// Special case: Barracks
		ResourceEnum input = building.input1();

		if (input != ResourceEnum::None)
		{
			SetResourceCompletion({ input }, {}, building);
		}
	}
	void SetResourceCompletion(std::vector<ResourceEnum> inputs, std::vector<ResourceEnum> outputs, Building& building)
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

			std::stringstream ss;
			ss << "Input<space>";
			ss << ResourceName(inputs[i]) << " " << hasCount << "/" << needCount;
			ss << "<space>Stored(city): " << simulation().resourceCount(playerId(), inputs[i]);
			
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

			float outputFraction = building.barFraction();
			material->SetScalarParameterValue("Fraction", outputFraction);
			material->SetScalarParameterValue("IsInput", 0.0f);
			material->SetScalarParameterValue("HasNoResource", 0.0f);

			completionIcon->SetIsPaused(building.priority() == PriorityEnum::Disable);

			//index++;

			std::stringstream ss;
			ss << "Output<space>";
			ss << ResourceName(outputs[i]) << " " << static_cast<int>(outputFraction * 100) << "%";
			
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
