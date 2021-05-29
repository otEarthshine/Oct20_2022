// Pun Dumnernchanvanit's

#pragma once

#include "PunCity/Simulation/UnlockSystem.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "PunCity/UI/PunBoxWidget.h"
#include "ProsperityBoxUI.generated.h"

/**
 * 
 */
UCLASS()
class PROTOTYPECITY_API UProsperityBoxUI : public UPunWidget
{
	GENERATED_BODY()
public:
	
	TechEnum uiTechEnum = TechEnum::None;
	int32 columnIndex = -1;
	
	int32 localIndex = -1;
	int32 upgradeCount = -1;

	void PunInit(UPunWidget* callbackParent, TechEnum techEnumIn, int32 columnIndexIn, int32 localIndexIn, int32 upgradeCountIn)
	{
		uiTechEnum = techEnumIn;
		columnIndex = columnIndexIn;
		upgradeCount = upgradeCountIn;
		localIndex = localIndexIn;

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
				material->SetTextureParameterValue("ColorTexture", assetLoader()->GetBuildingIcon(CardEnum::House));
				material->SetTextureParameterValue("DepthTexture", assetLoader()->GetBuildingIconAlpha(CardEnum::House));
			}

			// Add Tooltip
			UPunBoxWidget::AddBuildingTooltip(rewardBuildingIcon, buildingEnum, this, isPermanent);
		};

		// Set Building Research Icon
		if (techInfo->_buildingEnums.size() > 0) {
			setBuildingRewardIcon(RewardBuildingIcon1, techInfo->_buildingEnums[0], false);
		}
		else {
			RewardBuildingIcon1->SetVisibility(ESlateVisibility::Collapsed);
		}
		if (techInfo->_buildingEnums.size() > 1) {
			setBuildingRewardIcon(RewardBuildingIcon2, techInfo->_buildingEnums[1], false);
		}
		else {
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

	void UpdateTooltip()
	{
		
	}

	void TickUI()
	{
		auto unlockSys = simulation().unlockSystem(playerId());
		
		// House Count Text
		int32 houseLvlCount = simulation().GetHouseLvlCount(playerId(), columnIndex, true);

		TechEnum techEnum = unlockSys->GetProsperityTechEnum(columnIndex, localIndex);
		auto tech = unlockSys->GetTechInfo(techEnum);

		// Done Tech
		if (tech->state == TechStateEnum::Researched)
		{
			SetText(UpgradeCountText, std::to_string(upgradeCount));
			TechName->SetColorAndOpacity(FLinearColor::White);
			UpgradeCountText->SetColorAndOpacity(FLinearColor::Gray);
			UpgradeCountText->SetVisibility(ESlateVisibility::SelfHitTestInvisible);
			OuterImage->GetDynamicMaterial()->SetScalarParameterValue("ResearchFraction", 1);
			OuterImage->GetDynamicMaterial()->SetScalarParameterValue("IsActive", 0);
			return;
		}
		

		// Helpers
		auto setTechNotReachedYet = [&]() {
			SetText(UpgradeCountText, std::to_string(upgradeCount));
			TechName->SetColorAndOpacity(FLinearColor::Gray);
			UpgradeCountText->SetColorAndOpacity(FLinearColor::Gray);
			UpgradeCountText->SetVisibility(ESlateVisibility::SelfHitTestInvisible);
			OuterImage->GetDynamicMaterial()->SetScalarParameterValue("ResearchFraction", 0);
			OuterImage->GetDynamicMaterial()->SetScalarParameterValue("IsActive", 0);
		};

		auto setTechActive = [&]() {
			std::stringstream ss;
			ss << houseLvlCount << "/" << upgradeCount;
			SetText(UpgradeCountText, ss.str());
			TechName->SetColorAndOpacity(FLinearColor::White);
			UpgradeCountText->SetColorAndOpacity(FLinearColor::White);
			UpgradeCountText->SetVisibility(ESlateVisibility::SelfHitTestInvisible);

			OuterImage->GetDynamicMaterial()->SetScalarParameterValue("ResearchFraction", static_cast<float>(houseLvlCount) / upgradeCount);
			OuterImage->GetDynamicMaterial()->SetScalarParameterValue("IsActive", 1);
		};
		

		// Not the first tech in column
		if (localIndex > 0)
		{
			TechEnum techEnumBelow = unlockSys->GetProsperityTechEnum(columnIndex, localIndex - 1);
			auto techBelow = unlockSys->GetTechInfo(techEnumBelow);

			// Tech below is researched, this one is active
			if (techBelow->state == TechStateEnum::Researched) {
				setTechActive();
				return;
			}

			// Haven't reach this tech yet...
			setTechNotReachedYet();
			return;
		}
		
		// First tech in column
		bool isAdjacentTechDone = true;
		if (columnIndex > 1) {
			TechEnum techEnumToTheLeft = unlockSys->GetProsperityTechEnum(columnIndex - 1, localIndex);
			auto techToTheLeft = unlockSys->GetTechInfo(techEnumToTheLeft);
			isAdjacentTechDone = (techToTheLeft->state == TechStateEnum::Researched);
		}

		if (isAdjacentTechDone || houseLvlCount > 0) {
			setTechActive();
			return;
		}
		
		// Haven't reach this tech yet...
		setTechNotReachedYet();
		
	}
	
public:
	UPROPERTY(meta = (BindWidget)) UImage* InnerImage;
	UPROPERTY(meta = (BindWidget)) UImage* OuterImage;
	
	UPROPERTY(meta = (BindWidget)) UTextBlock* TechName;
	UPROPERTY(meta = (BindWidget)) UTextBlock* UpgradeCountText;

	UPROPERTY(meta = (BindWidget)) UImage* RewardBuildingIcon1;
	UPROPERTY(meta = (BindWidget)) UImage* RewardBuildingIcon2;
	UPROPERTY(meta = (BindWidget)) UImage* RewardBuildingIcon3;
	UPROPERTY(meta = (BindWidget)) UImage* RewardBonusIcon1;
	UPROPERTY(meta = (BindWidget)) UImage* RewardBonusIcon2;
};
