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

	UPROPERTY(meta = (BindWidget)) UImage* ForeignLogo;
	UPROPERTY(meta = (BindWidget)) UHorizontalBox* ForeignAllowBox;
	UPROPERTY(meta = (BindWidget)) UButton* ForeignAllowButton;
	UPROPERTY(meta = (BindWidget)) UButton* ForeignDisallowButton;
	
	UPROPERTY(meta = (BindWidget)) UButton* ArrowUp;
	UPROPERTY(meta = (BindWidget)) UButton* ArrowDown;
	UPROPERTY(meta = (BindWidget)) UHorizontalBox* HumanSlots;

	UPROPERTY(meta = (BindWidget)) UOverlay* HumanSlotCountOverlay;
	UPROPERTY(meta = (BindWidget)) UTextBlock* HumanSlotCount1;
	UPROPERTY(meta = (BindWidget)) UTextBlock* HumanSlotCount2;

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

	UPROPERTY(meta = (BindWidget)) UTextBlock* LargeWhiteText;
	UPROPERTY(meta = (BindWidget)) URichTextBlock* MediumGrayText;

	UPROPERTY(meta = (BindWidget)) USizeBox* SpeedBoostIcon;
	UPROPERTY(meta = (BindWidget)) UButton* TradeButton;
	UPROPERTY(meta = (BindWidget)) UButton* AutoTradeButton;

	UPROPERTY(meta = (BindWidget)) UButton* RevealSpyNestButton;
	UPROPERTY(meta = (BindWidget)) URichTextBlock* RevealSpyNestButtonText;

	UPROPERTY(meta = (BindWidget)) UButton* StatisticsButton;
	UPROPERTY(meta = (BindWidget)) UButton* JobPriorityButton;

	UPROPERTY(meta = (BindWidget)) UPunBoxWidget* PunBox;

public:
	void PunInit(int buildingId, bool isHouse);

	void SetShowHumanSlots(bool isVisible, bool canManipulateOccupants, bool isTileBld = false);
	void SetShowBar(bool showBar, bool showHouseUpgradeBar = false) {
		LEAN_PROFILING_UI(TickWorldSpaceUI_BldJobShowBars);
		
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
	
	void SetConstructionResource(std::vector<int32> constructionResourcesCount, Building& building);

	void SetSpeedBoost(Building& building)
	{
		bool hasSpeedBoost = simulation().playerOwned(playerId()).HasSpeedBoost(building.buildingId());
		SpeedBoostIcon->SetVisibility(hasSpeedBoost ? ESlateVisibility::HitTestInvisible : ESlateVisibility::Collapsed);
	}

	void SetBuildingStatus(Building& building, JobUIState jobUIState)
	{	
		LEAN_PROFILING_UI(TickWorldSpaceUI_BldJobBldStatus);

		if (building.isEnum(CardEnum::StatisticsBureau)) {
			// Note StatisticsButton gets its visibility set to Collapsed when it gets init
			if (building.ownedBy(playerId())) {
				StatisticsButton->SetVisibility(ESlateVisibility::Visible);
			}
			return;
		}
		if (building.isEnum(CardEnum::JobManagementBureau)) {
			// Note JobPriorityButton gets its visibility set to Collapsed when it gets init
			if (building.ownedBy(playerId())) {
				JobPriorityButton->SetVisibility(ESlateVisibility::Visible);
			}
			return;
		}
		if (building.isEnum(CardEnum::TradingCompany)) 
		{
			if (building.ownedBy(playerId()) || SimSettings::IsOn("CheatFastBuild")) {
				AutoTradeButton->SetVisibility(ESlateVisibility::Visible);
			}
			return;
		}
		if (building.isEnum(CardEnum::SpyCenter))
		{
			if (building.ownedBy(playerId()) || SimSettings::IsOn("CheatFastBuild")) {
				RevealSpyNestButton->SetVisibility(ESlateVisibility::Visible);
				RevealSpyNestButtonText->SetText(FText::Format(
					NSLOCTEXT("BuildingJobUI", "Reveal Spy Nest Button", "Reveal Spy Nest <img id=\"Coin\"/>{0}"), 
					TEXT_NUM(simulation().GetRevealSpyNestPrice()))
				);
			}
			return;
		}
		
		if (IsStorage(building.buildingEnum()) ||
			building.isEnum(CardEnum::IntercityLogisticsHub) ||
			building.isEnum(CardEnum::IntercityLogisticsPort) ||
			building.isEnum(CardEnum::FruitGatherer) ||
			building.isEnum(CardEnum::HuntingLodge))
		{
			// Show items above warehouse
			if (jobUIState == JobUIState::Storage &&
					(
					building.isEnum(CardEnum::Warehouse) ||
					building.isEnum(CardEnum::Granary) ||
					building.isEnum(CardEnum::IntercityLogisticsHub) ||
					building.isEnum(CardEnum::IntercityLogisticsPort)
					)
				)
			{
				if (building.isEnum(CardEnum::IntercityLogisticsHub) ||
					building.isEnum(CardEnum::IntercityLogisticsPort))
				{
					bool canManipulateOccupant = playerId() == building.playerId();
					SetShowHumanSlots(true, canManipulateOccupant);
					SetSlots(building.occupantCount(), building.allowedOccupants(), building.maxOccupants(), FLinearColor::White);
				}
				
				PunBox->SetVisibility(ESlateVisibility::SelfHitTestInvisible);
				const std::vector<ResourceHolderInfo>& holderInfos = building.subclass<StorageYard>().holderInfos();

				std::vector<ResourcePair> maxResourcePairs;
				bool hasMore = false;

				for (ResourceHolderInfo holderInfo : holderInfos)
				{
					int32 resourceCount = building.GetResourceCount(holderInfo);
					if (resourceCount > 0)
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
					PunBox->AddIconPair(FText(), pair.resourceEnum, TEXT_NUM(pair.count), false, true);
				}
				if (hasMore) {
					auto textWidget = PunBox->AddText(INVTEXT("..."));
					textWidget->PunText->SetShadowOffset(FVector2D(1, 1));
					textWidget->PunText->SetShadowColorAndOpacity(FLinearColor(0, 0, 0, 0.5));
					CastChecked<UVerticalBoxSlot>(textWidget->Slot)->SetHorizontalAlignment(HAlign_Center);
				}

				PunBox->AfterAdd();
			}
			else {
				PunBox->SetVisibility(ESlateVisibility::Collapsed);
			}
			return;
		}
		
		if (IsTradingPostLike(building.buildingEnum()) ||
			building.isEnum(CardEnum::TradingCompany)) 
		{
			SetTradeProgress(building.subclass<TradeBuilding>(), building.barFraction());
			return;
		}


		// ResourceUI Dirty?
		if (building.isBuildingResourceUIDirty()) 
		{
			building.SetBuildingResourceUIDirty(false);


			if (IsSpecialProducer(building.buildingEnum())) {
				SetSpecialProducerProgress(building.barFraction(), building);
			}
			else {
				SetResourceCompletion(building.inputs(), building.products(), building);
			}
		}
	}

	void SetHoverWarning(Building& building)
	{
		LEAN_PROFILING_UI(TickWorldSpaceUI_BldJobHoverWarning);
		
		// Refresh Hover Warning
		// Check every sec
		if (building.ownedBy(playerId()))
		{
			building.TryRefreshHoverWarning(UGameplayStatics::GetTimeSeconds(this));
			//if (time - building.lastHoverWarningCheckTime >= 1.0f)
			//{
			//	building.lastHoverWarningCheckTime = time;
			//	building.RefreshHoverWarning();
			//}
		}
		else {
			building.hoverWarning = HoverWarning::None;
		}

		if (building.hoverWarning != HoverWarning::None)
		{
			//PUN_LOG("Hover Warning %s warningId:%d", ToTChar(building.buildingInfo().nameStd()), static_cast<int>(building.hoverWarning));

			DepletedText->SetVisibility(ESlateVisibility::SelfHitTestInvisible);
			SetText(DepletedText, GetHoverWarningName(building.hoverWarning));
		}
		else {
			DepletedText->SetVisibility(ESlateVisibility::Collapsed);
		}
	}
	
	
	void SetTradeProgress(TradeBuilding& tradeBuilding, float fraction);

	
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
	void SetResourceCompletion(std::vector<ResourceEnum> inputs, std::vector<ResourceEnum> outputs, Building& building);
	

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

	UFUNCTION() void OnClickAutoTradeButton()
	{
		Building& building = dataSource()->simulation().building(_buildingId);
		PUN_CHECK(building.buildingEnum() == CardEnum::TradingCompany);

		GetPunHUD()->OpenTownAutoTradeUI(building.townId());
	}

	UFUNCTION() void OnClickRevealSpyNestButton() {
		inputSystemInterface()->StartRevealSpyNest();
	}

	UFUNCTION() void OnClickStatisticsButton() {
		GetPunHUD()->OpenStatisticsUI(playerId()); // Open capital statistics UI first
	}
	UFUNCTION() void OnClickJobPriorityButton() {
		GetPunHUD()->OpenJobPriorityUI(playerId()); // Open capital statistics UI first
	}

	UFUNCTION() void OnClickForeignAllowButton() {
		ForeignAllowDisallowBuilding(CallbackEnum::ForeignBuildingAllow);
	}
	UFUNCTION() void OnClickForeignDisallowButton() {
		ForeignAllowDisallowBuilding(CallbackEnum::ForeignBuildingDisallow);
	}
	void ForeignAllowDisallowBuilding(CallbackEnum callbackEnum)
	{
		auto command = std::make_shared<FGenericCommand>();
		command->callbackEnum = callbackEnum;
		command->intVar1 = _buildingId;
		networkInterface()->SendNetworkCommand(command);
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
