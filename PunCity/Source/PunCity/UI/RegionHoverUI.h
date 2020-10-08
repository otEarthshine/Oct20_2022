// Pun Dumnernchanvanit's

#pragma once

#include "CoreMinimal.h"
#include "PunCity/UI/PunWidget.h"
#include "RegionHoverUI.generated.h"

/**
 * 
 */
UCLASS()
class PROTOTYPECITY_API URegionHoverUI : public UPunWidget
{
	GENERATED_BODY()
public:
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

	UPROPERTY(meta = (BindWidget)) USizeBox* IconSizeBox;
	UPROPERTY(meta = (BindWidget)) UImage* IconImage;

	int32 provinceId = -1;
	
	void UpdateUI(int32 provinceIdIn)
	{
		provinceId = provinceIdIn;
		
		BUTTON_ON_CLICK(ReinforceLeftButton, this, &URegionHoverUI::OnClickReinforceLeft);
		BUTTON_ON_CLICK(ReinforceRightButton, this, &URegionHoverUI::OnClickReinforceRight);
		BUTTON_ON_CLICK(ReinforceMoneyRightButton, this, &URegionHoverUI::OnClickReinforceMoneyRight);
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
