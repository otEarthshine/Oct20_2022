// Pun Dumnernchanvanit's


#include "MinorTownWorldUI.h"

#define LOCTEXT_NAMESPACE "MinorTownWorldUI"



void UMinorTownWorldUI::UpdateUIBase(bool isMini)
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


void UMinorTownWorldUI::UpdateMinorTownUI(bool isMini)
{
	UpdateUIBase(isMini);

	auto& sim = simulation();
	TownManagerBase* uiTownManagerBase = sim.townManagerBase(uiTownId);

	// Gift
	GiftButton->SetVisibility(ESlateVisibility::Visible);
	BUTTON_ON_CLICK(GiftButton, this, &UMinorTownWorldUI::OnClickGiftButton);

	// Diplomacy
	DiplomacyButton->SetVisibility(ESlateVisibility::Visible);
	BUTTON_ON_CLICK(DiplomacyButton, this, &UMinorTownWorldUI::OnClickDiplomacyButton);

	/*
	 * AttackButtons
	 */
	AttackButton1->SetVisibility(ESlateVisibility::Collapsed);
	AttackButton2->SetVisibility(ESlateVisibility::Collapsed);
	AttackButton3->SetVisibility(ESlateVisibility::Collapsed);

	// Not already a vassal? Might be able to attack
	if (!sim.townManagerBase(playerId())->IsVassal(townId()))
	{
		// Vassalize
		if (sim.CanVassalizeOtherPlayers(playerId()) &&
			!uiTownManagerBase->GetDefendingClaimProgress(townProvinceId()).isValid())
		{
			//! Vassalize (AttackButton1)
			SetText(AttackButton1RichText, 
				LOCTEXT("VassalizeButtonRichText_Text", "Conquer (Vassalize)")
			);

			BUTTON_ON_CLICK(AttackButton1, this, &UMinorTownWorldUI::OnClickVassalizeButton);
			AttackButton1->SetVisibility(ESlateVisibility::Visible);

			//! Raze
			AttackButton3->SetVisibility(ESlateVisibility::Visible);
			BUTTON_ON_CLICK(AttackButton3, this, &UMinorTownWorldUI::OnClickRazeButton);

			// Can also liberate if there is an existing conquerer
			if (uiTownManagerBase->lordPlayerId() != -1) {
				SetText(AttackButton2RichText, 
					LOCTEXT("LiberationButtonRichText_Text", "Liberation")
				);
				AttackButton2->SetVisibility(ESlateVisibility::Visible);
				BUTTON_ON_CLICK(AttackButton2, this, &UMinorTownWorldUI::OnClickLiberateButton);
			}
		}
	}

}





















#undef LOCTEXT_NAMESPACE