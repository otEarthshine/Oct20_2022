// Pun Dumnernchanvanit's

#pragma once

#include "CoreMinimal.h"
#include "PunCity/UI/PunBoxWidget.h"
#include "RegionHoverUI.generated.h"

/**
 * 
 */
UCLASS()
class PROTOTYPECITY_API URegionHoverUI : public UPunWidget
{
	GENERATED_BODY()
public:
	// Battle
	UPROPERTY(meta = (BindWidget)) UOverlay* BattleOverlay;
	UPROPERTY(meta = (BindWidget)) URichTextBlock* BattleText;
	
	UPROPERTY(meta = (BindWidget)) UImage* BattleBarImage;

	UPROPERTY(meta = (BindWidget)) UImage* PlayerLogoLeft;
	UPROPERTY(meta = (BindWidget)) UImage* PlayerLogoRight;

	UPROPERTY(meta = (BindWidget)) URichTextBlock* BattleInfluenceLeft;
	UPROPERTY(meta = (BindWidget)) URichTextBlock* BattleInfluenceRight;

	UPROPERTY(meta = (BindWidget)) URichTextBlock* DefenseBonusLeft;
	UPROPERTY(meta = (BindWidget)) URichTextBlock* DefenseBonusRight;

	UPROPERTY(meta = (BindWidget)) UButton* ReinforceLeftButton;
	UPROPERTY(meta = (BindWidget)) UButton* ReinforceRightButton;
	UPROPERTY(meta = (BindWidget)) UButton* ReinforceMoneyRightButton;

	UPROPERTY(meta = (BindWidget)) URichTextBlock* ReinforceLeftButtonText;
	UPROPERTY(meta = (BindWidget)) URichTextBlock* ReinforceRightButtonText;
	UPROPERTY(meta = (BindWidget)) URichTextBlock* ReinforceMoneyRightButtonText;

	// Province Overlay
	UPROPERTY(meta = (BindWidget)) UOverlay* ProvinceOverlay;

	UPROPERTY(meta = (BindWidget)) USizeBox* IconSizeBox;
	UPROPERTY(meta = (BindWidget)) UImage* IconImage;

	UPROPERTY(meta = (BindWidget)) UPunBoxWidget* PunBox;
	
	UPROPERTY(meta = (BindWidget)) UTextBlock* IncomeText;
	UPROPERTY(meta = (BindWidget)) UTextBlock* IncomeCount;
	UPROPERTY(meta = (BindWidget)) UTextBlock* UpkeepText;
	UPROPERTY(meta = (BindWidget)) UTextBlock* UpkeepCount;
	UPROPERTY(meta = (BindWidget)) UTextBlock* BorderUpkeepText;
	UPROPERTY(meta = (BindWidget)) UTextBlock* BorderUpkeepCount;

	int32 provinceId = -1;
	
	void UpdateUI(int32 provinceIdIn)
	{
		provinceId = provinceIdIn;
		
		BUTTON_ON_CLICK(ReinforceLeftButton, this, &URegionHoverUI::OnClickReinforceLeft);
		BUTTON_ON_CLICK(ReinforceRightButton, this, &URegionHoverUI::OnClickReinforceRight);
		BUTTON_ON_CLICK(ReinforceMoneyRightButton, this, &URegionHoverUI::OnClickReinforceMoneyRight);
	}

	void UpdateProvinceOverlayInfo(int32 provinceIdIn)
	{
		auto& sim = simulation();
		int32 provinceOwnerId = sim.provinceOwner(provinceIdIn);
		bool unlockedInfluence = sim.unlockedInfluence(provinceOwnerId);

		BorderUpkeepText->SetVisibility(ESlateVisibility::Hidden);
		BorderUpkeepCount->SetVisibility(ESlateVisibility::Hidden);
		IconSizeBox->SetVisibility(ESlateVisibility::Collapsed);
		
		// Already own this province, Showr real income/upkeep
		if (provinceOwnerId == playerId())
		{
			SetText(IncomeText, "Income:");
			SetText(IncomeCount, std::to_string(sim.GetProvinceIncome100(provinceIdIn) / 100.0f));
			
			if (unlockedInfluence) {
				SetText(UpkeepText, "Upkeep:");
				SetText(UpkeepCount, std::to_string(sim.GetProvinceUpkeep100(provinceIdIn, provinceOwnerId) / 100.0f));

				if (sim.IsBorderProvince(provinceIdIn)) {
					SetText(UpkeepText, "Border Upkeep:");
					SetText(UpkeepCount, std::to_string(5));
						
					BorderUpkeepText->SetVisibility(ESlateVisibility::SelfHitTestInvisible);
					BorderUpkeepCount->SetVisibility(ESlateVisibility::SelfHitTestInvisible);
				}
			}
		}
		else
		{
			SetText(IncomeText, "Income:");
			SetText(IncomeCount, std::to_string(sim.GetProvinceIncome100(provinceIdIn) / 100.0f));

			if (unlockedInfluence) {
				SetText(UpkeepText, "Upkeep:");
				SetText(UpkeepCount, std::to_string(sim.GetProvinceBaseUpkeep100(provinceIdIn) / 100.0f));
			}
		}

		GeoresourceSystem& georesourceSys = sim.georesourceSystem();
		GeoresourceNode node = georesourceSys.georesourceNode(provinceId);
		ProvinceSystem& provinceSys = sim.provinceSystem();
		bool isMountain = provinceSys.provinceMountainTileCount(provinceId) > 0;
		
		if (node.HasResource())
		{
			IconSizeBox->SetVisibility(ESlateVisibility::SelfHitTestInvisible);
			ResourceEnum resourceEnum = node.info().resourceEnum;
			switch (resourceEnum) {
				case ResourceEnum::Iron: resourceEnum = ResourceEnum::IronOre; break;
				case ResourceEnum::GoldBar: resourceEnum = ResourceEnum::GoldOre; break;
				case ResourceEnum::Gemstone: resourceEnum = ResourceEnum::Gemstone; break;
				default: break;
			}
			
			SetResourceImage(IconImage, resourceEnum, assetLoader());

			if (node.depositAmount > 0) {
				PunBox->AddIconPair("", node.info().resourceEnum, to_string(node.depositAmount));
			}
		}
		
		if (isMountain) {
			PunBox->AddIconPair("", ResourceEnum::Stone, to_string(node.stoneAmount));
		}

		PunBox->AfterAdd();
	}

	

	UFUNCTION() void OnClickReinforceLeft()
	{
		auto command = make_shared<FClaimLand>();
		command->claimEnum = CallbackEnum::ReinforceAttackProvince;
		command->provinceId = provinceId;
		networkInterface()->SendNetworkCommand(command);
	}
	UFUNCTION() void OnClickReinforceRight()
	{
		PUN_CHECK(provinceId != -1);

		auto command = make_shared<FClaimLand>();
		command->claimEnum = CallbackEnum::DefendProvinceInfluence;
		command->provinceId = provinceId;
		networkInterface()->SendNetworkCommand(command);
	}
	UFUNCTION() void OnClickReinforceMoneyRight()
	{
		PUN_CHECK(provinceId != -1);

		auto command = make_shared<FClaimLand>();
		command->claimEnum = CallbackEnum::DefendProvinceMoney;
		command->provinceId = provinceId;
		networkInterface()->SendNetworkCommand(command);
	}
	
};
