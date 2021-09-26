// Pun Dumnernchanvanit's

#pragma once

#include "PunWidget.h"
#include "MinorTownWorldUI.generated.h"

/**
 * 
 */
UCLASS()
class PROTOTYPECITY_API UMinorTownWorldUI : public UPunWidget
{
	GENERATED_BODY()
public:
	UPROPERTY(meta = (BindWidget)) UButton* GiftButton;
	UPROPERTY(meta = (BindWidget)) UButton* DiplomacyButton;

	UPROPERTY(meta = (BindWidget)) UButton* AttackButton1;
	UPROPERTY(meta = (BindWidget)) URichTextBlock* AttackButton1RichText;
	UPROPERTY(meta = (BindWidget)) UButton* AttackButton2;
	UPROPERTY(meta = (BindWidget)) URichTextBlock* AttackButton2RichText;

	UPROPERTY(meta = (BindWidget)) UTextBlock* CityNameText;

	int32 uiTownId = -1;

	int32 townId() { return uiTownId; }
	int32 townBuildingId() { return simulation().townManagerBase(uiTownId)->townhallId; }
	int32 townPlayerId() { return simulation().townPlayerId(uiTownId); }
	int32 townProvinceId() { return simulation().building(simulation().townManagerBase(uiTownId)->townhallId).provinceId(); }



	void UpdateUIBase(bool isMini);
	
	void UpdateMinorTownUI(bool isMini);


		

	UFUNCTION() void OnClickGiftButton() {
		GetPunHUD()->OpenGiftUI(playerId(), townId(), TradeDealStageEnum::Gifting);
	}
	
	UFUNCTION() void OnClickDiplomacyButton() {
		GetPunHUD()->OpenDiplomacyUI(townPlayerId());
	}

	UFUNCTION() void OnClickVassalizeButton()
	{
		check(playerId() != townPlayerId());
		GetPunHUD()->OpenReinforcementUI(townProvinceId(), CallbackEnum::StartAttackProvince);
	}
	
	UFUNCTION() void OnClickLiberateButton()
	{
		check(playerId() != townPlayerId());
		GetPunHUD()->OpenReinforcementUI(townProvinceId(), CallbackEnum::Liberate);
	}
		
};
