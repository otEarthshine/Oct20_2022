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

	UPROPERTY(meta = (BindWidget)) URichTextBlock* LeftDefenseBonus;
	UPROPERTY(meta = (BindWidget)) URichTextBlock* RightDefenseBonus;

	UPROPERTY(meta = (BindWidget)) URichTextBlock* LeftArmyStrength;
	UPROPERTY(meta = (BindWidget)) URichTextBlock* RightArmyStrength;
	
	UPROPERTY(meta = (BindWidget)) UVerticalBox* LeftArmyVerticalBox;
	UPROPERTY(meta = (BindWidget)) UVerticalBox* RightArmyVerticalBox;

	UPROPERTY(meta = (BindWidget)) UButton* LeftReinforceButton;
	UPROPERTY(meta = (BindWidget)) UButton* RightReinforceButton;

	UPROPERTY(meta = (BindWidget)) URichTextBlock* LeftReinforceText;
	UPROPERTY(meta = (BindWidget)) URichTextBlock* RightReinforceText;

	int32 provinceId = -1;

	void UpdateUI(int32 provinceIdIn, ProvinceClaimProgress claimProgress);
	


	UFUNCTION() void OnClickReinforceLeft() {
		OnClickReinforce(CallbackEnum::ReinforceAttackProvince);
	}
	UFUNCTION() void OnClickReinforceRight() {
		OnClickReinforce(CallbackEnum::DefendProvinceInfluence);
	}

	void OnClickReinforce(CallbackEnum callbackEnum)
	{
		PUN_CHECK(provinceId != -1);

		auto command = make_shared<FClaimLand>();
		command->claimEnum = callbackEnum;
		command->provinceId = provinceId;
		networkInterface()->SendNetworkCommand(command);
	}
	
};
