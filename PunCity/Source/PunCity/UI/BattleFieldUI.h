// Pun Dumnernchanvanit's

#pragma once

#include "BattleFieldMiniUI.h"
#include "PunSpineWidget.h"

#include "BattleFieldUI.generated.h"

/**
 * 
 */
UCLASS()
class PROTOTYPECITY_API UBattleFieldUI : public UBattleFieldMiniUI
{
	GENERATED_BODY()
public:

	UPROPERTY(meta = (BindWidget)) URichTextBlock* LeftAttackBonus;
	UPROPERTY(meta = (BindWidget)) URichTextBlock* LeftDefenseBonus;
	UPROPERTY(meta = (BindWidget)) URichTextBlock* RightAttackBonus;
	UPROPERTY(meta = (BindWidget)) URichTextBlock* RightDefenseBonus;
	
	UPROPERTY(meta = (BindWidget)) UHorizontalBox* LeftArmyBackOuterBox;
	UPROPERTY(meta = (BindWidget)) UHorizontalBox* LeftArmyFrontOuterBox;

	UPROPERTY(meta = (BindWidget)) UHorizontalBox* RightArmyBackOuterBox;
	UPROPERTY(meta = (BindWidget)) UHorizontalBox* RightArmyFrontOuterBox;
	UPROPERTY(meta = (BindWidget)) UHorizontalBox* RightArmyWall;
	UPROPERTY(meta = (BindWidget)) UHorizontalBox* RightArmyTreasure;

	UPROPERTY(meta = (BindWidget)) UButton* LeftReinforceButton;
	UPROPERTY(meta = (BindWidget)) UButton* RightReinforceButton;

	UPROPERTY(meta = (BindWidget)) URichTextBlock* LeftReinforceText;
	UPROPERTY(meta = (BindWidget)) URichTextBlock* RightReinforceText;

	UPROPERTY(meta = (BindWidget)) UButton* LeftRetreatButton;
	UPROPERTY(meta = (BindWidget)) UButton* RightRetreatButton;

	UPROPERTY(meta = (BindWidget)) UOverlay* GroundAttacher;

	UPROPERTY(meta = (BindWidget)) UPunSpineWidget* BattleOpeningSpine;
	
	int32 provinceId = -1;

	void UpdateBattleFieldUI(int32 provinceIdIn, ProvinceClaimProgress claimProgress, bool showAttacher);
	
	virtual bool IsMiniUI() override { return false; }

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

	void SetShowReinforceRetreat(bool showLeftReinforce, bool showLeftRetreat, bool showRightReinforce, bool showRightRetreat)
	{
		LeftReinforceButton->SetVisibility(showLeftReinforce ? ESlateVisibility::Visible : ESlateVisibility::Collapsed);
		LeftRetreatButton->SetVisibility(showLeftRetreat ? ESlateVisibility::Visible : ESlateVisibility::Collapsed);
		RightReinforceButton->SetVisibility(showRightReinforce ? ESlateVisibility::Visible : ESlateVisibility::Collapsed);
		RightRetreatButton->SetVisibility(showRightRetreat ? ESlateVisibility::Visible : ESlateVisibility::Collapsed);
	}

};
