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
		bool isMainTree = techInfo->isMainTree;

		auto setCardIcon = [&](UMaterialInstanceDynamic* material, CardEnum buildingEnum)
		{
			if (UTexture2D* cardIcon = assetLoader()->GetCardIconNullable(playerFactionEnum(), buildingEnum)) {
				material->SetTextureParameterValue("ColorTexture", cardIcon);
			}
			else {
				material->SetTextureParameterValue("ColorTexture", assetLoader()->BlackIcon);
			}
		};

		auto setBuildingRewardIcon = [&](UImage* rewardBuildingIcon, CardEnum buildingEnum, bool isPermanent)
		{
			rewardBuildingIcon->SetVisibility(ESlateVisibility::Visible);
			auto material = rewardBuildingIcon->GetDynamicMaterial();
			
			// Is Card-Giving Tech
			if (!isMainTree && techInfo->_buildingEnums.size() > 0 && techInfo->maxUpgradeCount != -1)
			{
				material->SetScalarParameterValue("ShowCard", 1);

				setCardIcon(material, buildingEnum);
			}
			else if (IsBuildingCard(buildingEnum))
			{
				material->SetScalarParameterValue("ShowCard", 0);

				setCardIcon(material, buildingEnum);
			}
			else {
				if (isMainTree) {
					material->SetScalarParameterValue("ShowCard", 1);

					setCardIcon(material, buildingEnum);
				}
				else {
					material->SetScalarParameterValue("ShowCard", 0);
					material->SetTextureParameterValue("ColorTexture", assetLoader()->BlackIcon);
				}
			}
				
			// Add Tooltip
			UPunBoxWidget::AddBuildingTooltip(rewardBuildingIcon, buildingEnum, this, isPermanent);
		};

		
		RewardBuildingIcon1->SetVisibility(ESlateVisibility::Collapsed);
		RewardBuildingIcon2->SetVisibility(ESlateVisibility::Collapsed);
		RewardBuildingIcon3->SetVisibility(ESlateVisibility::Collapsed);

		
		//! Set Military Research Icon
		auto setMillitaryRewardIcon = [&](UImage* rewardBuildingIcon, CardEnum buildingEnum)
		{
			rewardBuildingIcon->SetVisibility(ESlateVisibility::Visible);
			auto material = rewardBuildingIcon->GetDynamicMaterial();

			material->SetScalarParameterValue("ShowCard", 1);

			setCardIcon(material, buildingEnum);

			// Add Tooltip
			UPunBoxWidget::AddBuildingTooltip(rewardBuildingIcon, buildingEnum, this, false);
		};
		
		if (techEnum == TechEnum::Infantry) {
			setMillitaryRewardIcon(RewardBuildingIcon1, CardEnum::Infantry);
			setMillitaryRewardIcon(RewardBuildingIcon2, CardEnum::Conscript);
		}
		else if (techEnum == TechEnum::Musketeer) setMillitaryRewardIcon(RewardBuildingIcon1, CardEnum::Musketeer);
		else if (techEnum == TechEnum::Swordsman) setMillitaryRewardIcon(RewardBuildingIcon1, CardEnum::Swordsman);

		else if (techEnum == TechEnum::Tank) setMillitaryRewardIcon(RewardBuildingIcon1, CardEnum::Tank);
		else if (techEnum == TechEnum::Knight) setMillitaryRewardIcon(RewardBuildingIcon1, CardEnum::Knight);
		
		else if (techEnum == TechEnum::MachineGun) setMillitaryRewardIcon(RewardBuildingIcon1, CardEnum::MachineGun);
		else if (techEnum == TechEnum::Archer) setMillitaryRewardIcon(RewardBuildingIcon1, CardEnum::Archer);

		else if (techEnum == TechEnum::Artillery) setMillitaryRewardIcon(RewardBuildingIcon1, CardEnum::Artillery);
		else if (techEnum == TechEnum::MilitaryEngineering2) {
			setMillitaryRewardIcon(RewardBuildingIcon1, CardEnum::Cannon);
			setMillitaryRewardIcon(RewardBuildingIcon2, CardEnum::Frigate);
		}
		else if (techEnum == TechEnum::MilitaryEngineering1) {
			setMillitaryRewardIcon(RewardBuildingIcon1, CardEnum::Catapult);
			setMillitaryRewardIcon(RewardBuildingIcon2, CardEnum::Galley);
		}

		else if (techEnum == TechEnum::Battleship) setMillitaryRewardIcon(RewardBuildingIcon1, CardEnum::Battleship);

		



		//! Set Building Research Icon
		if (techInfo->_buildingEnums.size() > 0) {
			setBuildingRewardIcon(RewardBuildingIcon1, techInfo->_buildingEnums[0], false);
		}
		//else {
		//	RewardBuildingIcon1->SetVisibility(ESlateVisibility::Collapsed);
		//}

		if (techInfo->_buildingEnums.size() > 1) {
			setBuildingRewardIcon(RewardBuildingIcon2, techInfo->_buildingEnums[1], false);
		}
		//else {
		//	RewardBuildingIcon2->SetVisibility(ESlateVisibility::Collapsed);
		//}

		if (techInfo->_buildingEnums.size() > 2) {
			setBuildingRewardIcon(RewardBuildingIcon3, techInfo->_buildingEnums[2], false);
		}
		//else {
		//	RewardBuildingIcon3->SetVisibility(ESlateVisibility::Collapsed);
		//}

		// TODO: clean...
		if (techInfo->_buildingEnums.size() == 0)
		{
			if (techInfo->_permanentBuildingEnums.size() > 0) {
				setBuildingRewardIcon(RewardBuildingIcon1, techInfo->_permanentBuildingEnums[0], true);
			}
			//else {
			//	RewardBuildingIcon1->SetVisibility(ESlateVisibility::Collapsed);
			//}
			if (techInfo->_permanentBuildingEnums.size() > 1) {
				setBuildingRewardIcon(RewardBuildingIcon2, techInfo->_permanentBuildingEnums[1], true);
			}
			//else {
			//	RewardBuildingIcon2->SetVisibility(ESlateVisibility::Collapsed);
			//}
		}
		

		// Set Bonus Icon
		if (techInfo->HasBonus()) 
		{
			RewardBonusIcon1->SetVisibility(ESlateVisibility::Visible);

			// Add Tooltip for Bonus Icon
			UPunBoxWidget* tooltipBox = UPunBoxWidget::AddToolTip(RewardBonusIcon1, this)->TooltipPunBoxWidget;
			if (tooltipBox) {
				tooltipBox->AfterAdd();
				tooltipBox->AddRichTextWrap(techInfo->GetBonusDescription());
			}
			
		}
		else {
			RewardBonusIcon1->SetVisibility(ESlateVisibility::Collapsed);
		}

		
		RewardBonusIcon2->SetVisibility(ESlateVisibility::Collapsed);

		UpdateTooltip();
	}

	virtual FReply NativeOnMouseButtonDown(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent) override;
	virtual FReply NativeOnMouseButtonDoubleClick(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent) override;
	virtual void NativeOnMouseEnter(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent) override {
		OuterImage->GetDynamicMaterial()->SetScalarParameterValue("IsHovered", !isLocked);
	}
	virtual void NativeOnMouseLeave(const FPointerEvent& InMouseEvent) override {
		OuterImage->GetDynamicMaterial()->SetScalarParameterValue("IsHovered", 0.0f);
	}

	void SetTechState(TechStateEnum techStateIn, bool isLockedIn, bool isInTechQueue, std::shared_ptr<ResearchInfo> tech = nullptr);

	
	void UpdateTooltip();
	
public:
	TechEnum techEnum = TechEnum::None;

	UPROPERTY(meta = (BindWidget)) UImage* InnerImage;
	UPROPERTY(meta = (BindWidget)) UImage* OuterImage;
	UPROPERTY(meta = (BindWidget)) UTextBlock* TechName;
	UPROPERTY(meta = (BindWidget)) UTextBlock* TechUpgradeCount;

	UPROPERTY() UOverlay* lineChild;
	UPROPERTY() UOverlay* lineChild2 = nullptr;

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
