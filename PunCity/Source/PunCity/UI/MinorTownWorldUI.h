// Pun Dumnernchanvanit's

#pragma once

#include "BattleFieldUI.h"
#include "PunWidget.h"
#include "MinorTownWorldUI.generated.h"

/**
 * Base class.. (But also used for MinorTown, too lazy to separate)
 */
UCLASS()
class PROTOTYPECITY_API UMinorTownWorldUI : public UPunWidget
{
	GENERATED_BODY()
public:
	UPROPERTY(meta = (BindWidget)) UBattleFieldUI* BattlefieldUI;
	
	UPROPERTY(meta = (BindWidget)) UButton* GiftButton;
	UPROPERTY(meta = (BindWidget)) UButton* DiplomacyButton;
	UPROPERTY(meta = (BindWidget)) UButton* ConnectTradeRouteButton;

	UPROPERTY(meta = (BindWidget)) UButton* AttackButton1;
	UPROPERTY(meta = (BindWidget)) URichTextBlock* AttackButton1RichText;
	UPROPERTY(meta = (BindWidget)) UButton* AttackButton2;
	UPROPERTY(meta = (BindWidget)) URichTextBlock* AttackButton2RichText;
	UPROPERTY(meta = (BindWidget)) UButton* AttackButton3;
	UPROPERTY(meta = (BindWidget)) URichTextBlock* AttackButton3RichText;

	UPROPERTY(meta = (BindWidget)) UHorizontalBox* ArmyRow;

	UPROPERTY(meta = (BindWidget)) UTextBlock* TownHoverPopulationText;

	UPROPERTY(meta = (BindWidget)) UTextBlock* CityNameText;
	UPROPERTY(meta = (BindWidget)) UImage* PlayerColorCircle;

	UPROPERTY(meta = (BindWidget)) URichTextBlock* BottomCaptionText;

	int32 uiTownId = -1;

	int32 townId() { return uiTownId; }
	int32 townBuildingId() { return simulation().townManagerBase(uiTownId)->townhallId; }
	int32 townPlayerId() { return simulation().townPlayerId(uiTownId); }
	int32 townProvinceId() { return simulation().building(simulation().townManagerBase(uiTownId)->townhallId).provinceId(); }



	void UpdateUIBase(bool isMini);
	
	void UpdateMinorTownUI(bool isMini);

	virtual void OnDespawnWidget() override {
		//BattlefieldUI->provinceId = -1;
	}
		

	UFUNCTION() void OnClickGiftButton() {
		GetPunHUD()->OpenGiftUI(playerId(), townId(), TradeDealStageEnum::Gifting);
	}
	
	UFUNCTION() void OnClickDiplomacyButton() {
		GetPunHUD()->OpenDiplomacyUI(townId());
	}

	UFUNCTION() void OnClickConnectTradeButton() {
		auto command = make_shared<FGenericCommand>();
		command->callbackEnum = CallbackEnum::EstablishTradeRoute;
		command->townId = uiTownId;
		command->intVar1 = uiTownId;
		command->intVar2 = playerId(); // Capital Town Id

		networkInterface()->SendNetworkCommand(command);
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

	UFUNCTION() void OnClickRazeButton()
	{
		check(playerId() != townPlayerId());
		GetPunHUD()->OpenReinforcementUI(townProvinceId(), CallbackEnum::Raze);
	}
};
