// Pun Dumnernchanvanit's

#pragma once

#include "PunWidget.h"
#include "BattleFieldUI.generated.h"

/**
 * 
 */
UCLASS()
class PROTOTYPECITY_API UBattleFieldUI : public UPunWidget
{
	GENERATED_BODY()
public:
	UPROPERTY(meta = (BindWidget)) UOverlay* BattleOverlay;

	UPROPERTY(meta = (BindWidget)) URichTextBlock* BattleText;
	
	UPROPERTY(meta = (BindWidget)) UImage* PlayerLogoLeft;
	UPROPERTY(meta = (BindWidget)) UImage* PlayerLogoRight;
	UPROPERTY(meta = (BindWidget)) UImage* BattleBarImage;

	UPROPERTY(meta = (BindWidget)) URichTextBlock* LeftAttackBonus;
	UPROPERTY(meta = (BindWidget)) URichTextBlock* LeftDefenseBonus;
	UPROPERTY(meta = (BindWidget)) URichTextBlock* RightAttackBonus;
	UPROPERTY(meta = (BindWidget)) URichTextBlock* RightDefenseBonus;

	UPROPERTY(meta = (BindWidget)) URichTextBlock* LeftArmyStrength;
	UPROPERTY(meta = (BindWidget)) URichTextBlock* RightArmyStrength;
	
	UPROPERTY(meta = (BindWidget)) UHorizontalBox* LeftArmyBackOuterBox;
	UPROPERTY(meta = (BindWidget)) UHorizontalBox* LeftArmyFrontOuterBox;
	UPROPERTY(meta = (BindWidget)) UHorizontalBox* RightArmyBackOuterBox;
	UPROPERTY(meta = (BindWidget)) UHorizontalBox* RightArmyFrontOuterBox;

	UPROPERTY(meta = (BindWidget)) UButton* LeftReinforceButton;
	UPROPERTY(meta = (BindWidget)) UButton* RightReinforceButton;

	UPROPERTY(meta = (BindWidget)) URichTextBlock* LeftReinforceText;
	UPROPERTY(meta = (BindWidget)) URichTextBlock* RightReinforceText;

	UPROPERTY(meta = (BindWidget)) UButton* LeftRetreatButton;
	UPROPERTY(meta = (BindWidget)) UButton* RightRetreatButton;
	
	int32 provinceId = -1;

	void UpdateUI(int32 provinceIdIn, ProvinceClaimProgress claimProgress);
	


	UFUNCTION() void OnClickReinforceLeft() {
		OnClickReinforce(CallbackEnum::ReinforceAttackProvince);
	}
	UFUNCTION() void OnClickReinforceRight() {
		OnClickReinforce(CallbackEnum::ReinforceDefendProvince);
	}
	UFUNCTION() void OnClickRetreatButton()
	{
		PUN_CHECK(provinceId != -1);

		auto command = make_shared<FClaimLand>();
		command->claimEnum = CallbackEnum::BattleRetreat;
		command->provinceId = provinceId;
		networkInterface()->SendNetworkCommand(command);
	}

	void OnClickReinforce(CallbackEnum callbackEnum)
	{
		PUN_CHECK(provinceId != -1);
		GetPunHUD()->OpenReinforcementUI(provinceId, callbackEnum);
	}

	void SetShowReinforceRetreat(bool isShowing, bool isLeft)
	{
		LeftReinforceButton->SetVisibility(isShowing && isLeft ? ESlateVisibility::Visible : ESlateVisibility::Collapsed);
		LeftRetreatButton->SetVisibility(isShowing && isLeft ? ESlateVisibility::Visible : ESlateVisibility::Collapsed);
		RightReinforceButton->SetVisibility(isShowing && !isLeft ? ESlateVisibility::Visible : ESlateVisibility::Collapsed);
		RightRetreatButton->SetVisibility(isShowing && !isLeft ? ESlateVisibility::Visible : ESlateVisibility::Collapsed);
	}
};
