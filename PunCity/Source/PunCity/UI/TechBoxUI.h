// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "PunCity/Simulation/UnlockSystem.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "PunCity/UI/PunWidget.h"
#include "TechLineUI.h"
#include "PunBoxWidget.h"
#include "TechBoxUI.generated.h"

/**
 * 
 */
UCLASS()
class UTechBoxUI : public UPunWidget
{
	GENERATED_BODY()
public:
	void Init(UPunWidget* callbackParent, TechEnum techEnumIn)
	{
		techEnum = techEnumIn;
		_callbackParent = callbackParent;
		//FindLineImages();
		SetTechState(TechStateEnum::Available, true, false);

		auto unlockSys = simulation().unlockSystem(playerId());
		auto techInfo = unlockSys->GetTechInfo(techEnumIn);

		auto setBuildingRewardIcon = [&](UImage* rewardBuildingIcon, CardEnum buildingEnum, bool isPermanent)
		{
			rewardBuildingIcon->SetVisibility(ESlateVisibility::Visible);
			auto material = rewardBuildingIcon->GetDynamicMaterial();

			if (IsBuildingCard(buildingEnum)) {
				material->SetTextureParameterValue("ColorTexture", assetLoader()->GetBuildingIcon(buildingEnum));
				material->SetTextureParameterValue("DepthTexture", assetLoader()->GetBuildingIconAlpha(buildingEnum));
			}
			else {
				// TODO: GetBuildingIcon used for all cards..
				material->SetTextureParameterValue("ColorTexture", nullptr);
				material->SetTextureParameterValue("DepthTexture", nullptr);
			}

			// Add Tooltip
			UPunBoxWidget::AddBuildingTooltip(rewardBuildingIcon, buildingEnum, this, isPermanent);
		};

		// Set Building Research Icon
		if (techInfo->_buildingEnums.size() > 0) {
			setBuildingRewardIcon(RewardBuildingIcon1, techInfo->_buildingEnums[0], false);
		} else {
			RewardBuildingIcon1->SetVisibility(ESlateVisibility::Collapsed);
		}
		if (techInfo->_buildingEnums.size() > 1) {
			setBuildingRewardIcon(RewardBuildingIcon2, techInfo->_buildingEnums[1], false);
		} else {
			RewardBuildingIcon2->SetVisibility(ESlateVisibility::Collapsed);
		}
		if (techInfo->_buildingEnums.size() > 2) {
			setBuildingRewardIcon(RewardBuildingIcon3, techInfo->_buildingEnums[2], false);
		}
		else {
			RewardBuildingIcon3->SetVisibility(ESlateVisibility::Collapsed);
		}

		// TODO: clean...
		if (techInfo->_buildingEnums.size() == 0)
		{
			if (techInfo->_permanentBuildingEnums.size() > 0) {
				setBuildingRewardIcon(RewardBuildingIcon1, techInfo->_permanentBuildingEnums[0], true);
			}
			else {
				RewardBuildingIcon1->SetVisibility(ESlateVisibility::Collapsed);
			}
			if (techInfo->_permanentBuildingEnums.size() > 1) {
				setBuildingRewardIcon(RewardBuildingIcon2, techInfo->_permanentBuildingEnums[1], true);
			}
			else {
				RewardBuildingIcon2->SetVisibility(ESlateVisibility::Collapsed);
			}
		}

		// Set Bonus Icon
		if (techInfo->HasBonus()) 
		{
			RewardBonusIcon1->SetVisibility(ESlateVisibility::Visible);

			// Add Tooltip for Bonus Icon
			UPunBoxWidget* tooltipBox = UPunBoxWidget::AddToolTip(RewardBonusIcon1, this)->TooltipPunBoxWidget;
			if (tooltipBox) {
				tooltipBox->AfterAdd();
				tooltipBox->AddRichText(techInfo->GetBonusDescription());
			}
			
		}
		else {
			RewardBonusIcon1->SetVisibility(ESlateVisibility::Collapsed);
		}

		
		RewardBonusIcon2->SetVisibility(ESlateVisibility::Collapsed);

		UpdateTooltip();
	}

	FReply NativeOnMouseButtonDown(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent) override;
	FReply NativeOnMouseButtonDoubleClick(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent) override;
	//void SetHighlight(TechStateEnum techState, bool active);
	//FReply NativeOnHover
	void NativeOnMouseEnter(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent) override {
		OuterImage->GetDynamicMaterial()->SetScalarParameterValue("IsHovered", !isLocked);
	}
	void NativeOnMouseLeave(const FPointerEvent& InMouseEvent) override {
		OuterImage->GetDynamicMaterial()->SetScalarParameterValue("IsHovered", 0.0f);
	}

	void SetTechState(TechStateEnum techStateIn, bool isLockedIn, bool isInTechQueue, std::shared_ptr<ResearchInfo> tech = nullptr);

	
	void UpdateTooltip()
	{
		auto unlockSys = simulation().unlockSystem(playerId());
		auto tech = unlockSys->GetTechInfo(techEnum);
		std::vector<CardEnum> unlockCards = tech->GetUnlockNames();

		UPunBoxWidget* tooltipBox = UPunBoxWidget::AddToolTip(this, this)->TooltipPunBoxWidget;
		if (tooltipBox)
		{
			tooltipBox->AfterAdd();

			// Header
			tooltipBox->AddRichText("<TipHeader>" + tech->GetName() + "</>");
			tooltipBox->AddSpacer();

			// Sci points
			//std::stringstream ss;
			//tooltipBox->AddSpacer();

			std::stringstream ss;
			ss << "Cost: " << tech->scienceNeeded(unlockSys->techsFinished) << "<img id=\"Science\"/>";
			tooltipBox->AddRichText(ss);
			tooltipBox->AddSpacer();
			//tooltipBox->AddLineSpacer(12);

			// Requirement
			if (tech->requiredResourceEnum != ResourceEnum::None)
			{
				int32 productionCount = unlockSys->GetResourceProductionCount(tech->requiredResourceEnum);
				if (productionCount < tech->requiredResourceCount) {
					ss << "Requirement:\n - Produce " << productionCount << "/" << tech->requiredResourceCount << " " << ResourceName(tech->requiredResourceEnum);
				} else {
					ss << "Requirement:\n - Produce " << tech->requiredResourceCount << " " << ResourceName(tech->requiredResourceEnum) << " (Completed)";
				}
				tooltipBox->AddRichText(ss);
				tooltipBox->AddSpacer(12);
			}


			// Bonus body
			if (tech->HasBonus()) {
				tooltipBox->AddRichText(tech->GetBonusDescription());
			}

			if (tech->HasBonus() && unlockCards.size() > 0) {
				tooltipBox->AddLineSpacer(12);
			}

			// Unlock body
			if (unlockCards.size() > 0) {
				ss << "Unlocks:";
				for (const CardEnum& cardEnum : unlockCards) {
					if (IsBuildingCard(cardEnum)) {
						ss << "\n - Building: " << GetBuildingInfo(cardEnum).name;
					}
					else if (IsActionCard(cardEnum)) {
						ss << "\n - Action card: " << GetBuildingInfo(cardEnum).name;
					}
					else if (IsGlobalSlotCard(cardEnum)) {
						ss << "\n - Slot card (Global): " << GetBuildingInfo(cardEnum).name;
					}
					else {
						ss << "\n - Slot card (Building): " << GetBuildingInfo(cardEnum).name;
					}
				}
				tooltipBox->AddRichText(ss);
			}
		}
	}
	
public:
	TechEnum techEnum = TechEnum::None;

	UPROPERTY(meta = (BindWidget)) UImage* InnerImage;
	UPROPERTY(meta = (BindWidget)) UImage* OuterImage;
	UPROPERTY(meta = (BindWidget)) UTextBlock* TechName;

	UPROPERTY(meta = (BindWidget)) UTextBlock* PercentText;
	UPROPERTY(meta = (BindWidget)) UTextBlock* SecText;

	UPROPERTY(meta = (BindWidget)) UImage* RewardBuildingIcon1;
	UPROPERTY(meta = (BindWidget)) UImage* RewardBuildingIcon2;
	UPROPERTY(meta = (BindWidget)) UImage* RewardBuildingIcon3;
	UPROPERTY(meta = (BindWidget)) UImage* RewardBonusIcon1;
	UPROPERTY(meta = (BindWidget)) UImage* RewardBonusIcon2;

	UPROPERTY(meta = (BindWidget)) UTextBlock* TechRequirement;
	UPROPERTY(meta = (BindWidget)) UImage* TechRequirementIcon;
	
private:
	void FindLineImages() {
		UOverlay* parent = CastChecked<UOverlay>(GetParent());
		TArray<UWidget*> peers = parent->GetAllChildren();
		for (int32 i = 0; i < peers.Num(); i++) {
			UTechLineUI* peer = Cast<UTechLineUI>(peers[i]);
			if (peer) {
				if (peer->Image1->GetVisibility() != ESlateVisibility::Collapsed) _lineImages.Add(peer->Image1);
				if (peer->Image2->GetVisibility() != ESlateVisibility::Collapsed) _lineImages.Add(peer->Image2);
				if (peer->Image3->GetVisibility() != ESlateVisibility::Collapsed) _lineImages.Add(peer->Image3);
				if (peer->Image4->GetVisibility() != ESlateVisibility::Collapsed) _lineImages.Add(peer->Image4);
				if (peer->Image5->GetVisibility() != ESlateVisibility::Collapsed) _lineImages.Add(peer->Image5);
			}
		}
	}

private:
	UPROPERTY() UPunWidget* _callbackParent;

	UPROPERTY() TArray<UImage*> _lineImages;

	bool isLocked = false;
};
