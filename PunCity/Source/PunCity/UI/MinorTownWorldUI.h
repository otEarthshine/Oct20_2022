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



	void UpdateUIBase(bool isMini)
	{
		/*
		 * City Name
		 */
		// Make sure town's name is up to date with sim (only for non player's town)
		if (townPlayerId() == playerId()) {
			CityNameText->SetColorAndOpacity(FLinearColor(0.7, 0.85, 1.0)); // Our Capital
		}
		else
		{
			if (simulation().townManagerBase(playerId())->IsVassal(townId())) {
				CityNameText->SetColorAndOpacity(FLinearColor(0.5, 0.6, 0.7)); // Vassal
			}
			else if (simulation().townManagerBase(playerId())->IsAlly(townPlayerId())) {
				CityNameText->SetColorAndOpacity(FLinearColor(.7, 1, .7));
			}
			else {
				CityNameText->SetColorAndOpacity(FLinearColor(1, 1, .7));
			}
		}

			
		// Townhall name
		FText displayedName = CityNameText->GetText();
		FText newDisplayName = simulation().townNameT(townId());

		if (!TextEquals(displayedName, newDisplayName)) {
			CityNameText->SetText(newDisplayName);
		}

	}
	

	void UpdateMinorTownUI()
	{

		// Gift
		GiftButton->SetVisibility(ESlateVisibility::Visible);
		BUTTON_ON_CLICK(GiftButton, this, &UMinorTownWorldUI::OnClickGiftButton);

		// Diplomacy
		DiplomacyButton->SetVisibility(ESlateVisibility::Visible);
		BUTTON_ON_CLICK(DiplomacyButton, this, &UMinorTownWorldUI::OnClickDiplomacyButton);

		
	}


		

	UFUNCTION() void OnClickGiftButton() {
		GetPunHUD()->OpenGiftUI(playerId(), townId(), TradeDealStageEnum::Gifting);
	}
	UFUNCTION() void OnClickDiplomacyButton() {
		GetPunHUD()->OpenDiplomacyUI(townPlayerId());
	}

		
};
