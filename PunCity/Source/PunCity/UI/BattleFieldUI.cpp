// Pun Dumnernchanvanit's


#include "BattleFieldUI.h"

#define LOCTEXT_NAMESPACE "BattleFieldUI"

void UBattleFieldUI::UpdateUI(int32 provinceIdIn, ProvinceClaimProgress claimProgress)
{
	provinceId = provinceIdIn;

	BUTTON_ON_CLICK(LeftReinforceButton, this, &UBattleFieldUI::OnClickReinforceLeft);
	BUTTON_ON_CLICK(RightReinforceButton, this, &UBattleFieldUI::OnClickReinforceRight);

	auto& sim = simulation();

	int32 provincePlayerId = sim.provinceOwnerPlayer(provinceId);

	bool isUIPlayerAttacker = claimProgress.attackerPlayerId == playerId();

	
	// Battle Bar
	float fraction = static_cast<float>(claimProgress.ticksElapsed) / BattleClaimTicks;
	BattleBarImage->GetDynamicMaterial()->SetScalarParameterValue("Fraction", 1.0f - fraction);
	BattleBarImage->GetDynamicMaterial()->SetScalarParameterValue("IsGreenLeft", isUIPlayerAttacker);
	BattleBarImage->SetVisibility(ESlateVisibility::SelfHitTestInvisible);


	//! Player Logo
	PunUIUtils::SetPlayerLogo(
		PlayerLogoLeft->GetDynamicMaterial(), 
		dataSource()->playerInfo(claimProgress.attackerPlayerId),
		assetLoader()
	);

	AddToolTip(PlayerLogoLeft, FText::Format(
		LOCTEXT("AttackPlayerLogo_Tip", "Attacker: {0}"),
		sim.playerNameT(claimProgress.attackerPlayerId)
	));

	

	int32 defenderPlayerId = provincePlayerId;
	bool isDeclaringIndependence = (claimProgress.attackerPlayerId == provincePlayerId);
	if (isDeclaringIndependence) {
		defenderPlayerId = sim.playerOwned(provincePlayerId).lordPlayerId(); // Declare Independence
	} // TODO: Declare Independence should init attack from the Lord

	PunUIUtils::SetPlayerLogo(
		PlayerLogoRight->GetDynamicMaterial(),
		dataSource()->playerInfo(defenderPlayerId),
		assetLoader()
	);
	
	AddToolTip(PlayerLogoRight, FText::Format(
		LOCTEXT("DefenderPlayerLogo_Tip", "Defender: {0}"),
		sim.playerNameT(defenderPlayerId)
	));

	

	SetText(LeftDefenseBonus, FText::Format(INVTEXT("<img id=\"Shield\"/><Shadowed>{0}</>"), TEXT_NUM(0)));

	AddToolTip(LeftDefenseBonus,
		LOCTEXT("Attack Bonus: 0%", "Attack Bonus: 0%")
	);

	std::string defenderDefenseBonus = (isDeclaringIndependence ? to_string(0) : to_string(sim.GetProvinceAttackCostPercent(provinceId))) + "%</>";
	SetText(RightDefenseBonus, "<img id=\"Shield\"/><Shadowed>" + defenderDefenseBonus);
	AddToolTip(LeftDefenseBonus, sim.GetProvinceDefenseBonusTip(provinceId));

	// Fight at home province = Vassalize
	if (sim.homeProvinceId(provincePlayerId) == provinceId) {
		SetText(BattleText, TEXT_TAG("<Shadowed>", LOCTEXT("Vassalize", "Vassalize")));
	}
	else {
		SetText(BattleText, TEXT_TAG("<Shadowed>", LOCTEXT("Annex Province", "Annex Province")));
	}


	// UI-Player is Attacker
	if (claimProgress.attackerPlayerId == playerId())
	{
		int32 provincePlayerIdTemp = sim.provinceOwnerPlayer(claimProgress.provinceId); //TODO: does it needs claimProgress.provinceId?
		auto& provincePlayerOwner = sim.playerOwned(provincePlayerIdTemp);

		ClaimConnectionEnum claimConnectionEnum = sim.GetProvinceClaimConnectionEnumPlayer(provinceId, claimProgress.attackerPlayerId);
		ProvinceAttackEnum attackEnum = provincePlayerOwner.GetProvinceAttackEnum(provinceId, claimProgress.attackerPlayerId);

		int32 reinforcePrice = (attackEnum == ProvinceAttackEnum::DeclareIndependence) ? BattleInfluencePrice : sim.GetProvinceAttackReinforcePrice(provinceId, claimConnectionEnum);
		bool hasEnoughInfluence = sim.influence(playerId()) >= reinforcePrice;

		SetText(LeftReinforceText,
			FText::Format(INVTEXT("{0}\n<img id=\"Influence\"/>{1}"), LOCTEXT("Reinforce", "Reinforce"), TextRed(TEXT_NUM(reinforcePrice), !hasEnoughInfluence))
		);
		LeftReinforceButton->SetIsEnabled(hasEnoughInfluence);

		LeftReinforceButton->SetVisibility(ESlateVisibility::Visible);
		RightReinforceButton->SetVisibility(ESlateVisibility::Collapsed);
	}
	// UI-Player is Defender
	else if (provincePlayerId == playerId())
	{
		int32 hasEnoughInfluence = sim.influence(playerId()) >= BattleInfluencePrice;
		SetText(RightReinforceText,
			FText::Format(INVTEXT("{0}\n<img id=\"Influence\"/>{1}"), LOCTEXT("Reinforce", "Reinforce"), TextRed(TEXT_NUM(BattleInfluencePrice), !hasEnoughInfluence))
		);
		RightReinforceButton->SetIsEnabled(hasEnoughInfluence);

		LeftReinforceButton->SetVisibility(ESlateVisibility::Collapsed);
		RightReinforceButton->SetVisibility(ESlateVisibility::Visible);
	}
	else {
		LeftReinforceButton->SetVisibility(ESlateVisibility::Collapsed);
		RightReinforceButton->SetVisibility(ESlateVisibility::Collapsed);
	}
}


#undef LOCTEXT_NAMESPACE