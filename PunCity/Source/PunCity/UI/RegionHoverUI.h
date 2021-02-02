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
	UPROPERTY(meta = (BindWidget)) UHorizontalBox* UpkeepBox;
	UPROPERTY(meta = (BindWidget)) UTextBlock* UpkeepText;
	UPROPERTY(meta = (BindWidget)) UTextBlock* UpkeepCount;
	UPROPERTY(meta = (BindWidget)) UHorizontalBox* BorderUpkeepBox;
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
		provinceId = provinceIdIn;
		
		auto& sim = simulation();
		int32 provincePlayerId = sim.provinceOwnerPlayer(provinceIdIn);
		bool unlockedInfluence = sim.unlockedInfluence(playerId());

		UpkeepText->SetVisibility(ESlateVisibility::Collapsed);
		UpkeepBox->SetVisibility(ESlateVisibility::Collapsed);
		BorderUpkeepText->SetVisibility(ESlateVisibility::Collapsed);
		BorderUpkeepBox->SetVisibility(ESlateVisibility::Collapsed);
		
		IconSizeBox->SetVisibility(ESlateVisibility::Collapsed);

#define LOCTEXT_NAMESPACE "RegionHoverUI"
		const FText incomeText = LOCTEXT("Income:", "Income:");
		const FText upkeepText = LOCTEXT("Upkeep:", "Upkeep:");
		
		// Already own this province, Show real income/upkeep
		if (provincePlayerId == playerId())
		{
			SetText(IncomeText, incomeText);
			SetTextNumber(IncomeCount, sim.GetProvinceIncome100(provinceIdIn) / 100.0f, 1);
			
			if (unlockedInfluence) {
				SetText(UpkeepText, upkeepText);
				SetTextNumber(UpkeepCount, sim.GetProvinceUpkeep100(provinceIdIn, provincePlayerId) / 100.0f, 1);
				UpkeepText->SetVisibility(ESlateVisibility::SelfHitTestInvisible);
				UpkeepBox->SetVisibility(ESlateVisibility::SelfHitTestInvisible);

				if (sim.IsBorderProvince(provinceIdIn)) {
					SetText(BorderUpkeepText, LOCTEXT("Border Upkeep:", "Border Upkeep:"));
					SetTextNumber(BorderUpkeepCount, 5, 1);
						
					BorderUpkeepText->SetVisibility(ESlateVisibility::SelfHitTestInvisible);
					BorderUpkeepBox->SetVisibility(ESlateVisibility::SelfHitTestInvisible);
				}
			}
		}
		else
		{
			SetText(IncomeText, incomeText);
			SetTextNumber(IncomeCount, sim.GetProvinceIncome100(provinceIdIn) / 100.0f, 1);

			if (unlockedInfluence) {
				SetText(UpkeepText, upkeepText);
				SetTextNumber(UpkeepCount, sim.GetProvinceBaseUpkeep100(provinceIdIn) / 100.0f, 1);
				UpkeepText->SetVisibility(ESlateVisibility::SelfHitTestInvisible);
				UpkeepBox->SetVisibility(ESlateVisibility::SelfHitTestInvisible);
			}
		}
#undef LOCTEXT_NAMESPACE
		

		GeoresourceSystem& georesourceSys = sim.georesourceSystem();
		GeoresourceNode node = georesourceSys.georesourceNode(provinceId);
		ProvinceSystem& provinceSys = sim.provinceSystem();
		bool isMountain = provinceSys.provinceMountainTileCount(provinceId) > 0;
		SetChildHUD(PunBox);
		
		if (node.HasResource())
		{
			ResourceEnum resourceEnum = node.info().resourceEnum;

			SetGeoresourceImage(IconImage, resourceEnum, assetLoader(), this);
			
			IconSizeBox->SetVisibility(ESlateVisibility::SelfHitTestInvisible);

			if (node.depositAmount > 0) {
				PunBox->AddIconPair(FText(), node.info().resourceEnum, TEXT_NUM(node.depositAmount));
			}
		}
		
		if (isMountain) {
			PunBox->AddIconPair(FText(), ResourceEnum::Stone, TEXT_NUM(node.stoneAmount));
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
