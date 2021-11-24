// Pun Dumnernchanvanit's


#include "BattleFieldMiniUI.h"


#define LOCTEXT_NAMESPACE "BattleFieldUI"

void UBattleFieldMiniUI::UpdateUIBase(int32 provinceId, const ProvinceClaimProgress& claimProgress)
{
	auto& sim = simulation();
	int32 provincePlayerId = sim.provinceOwnerPlayer(provinceId);
	int32 provinceTownId = sim.provinceOwnerTownSafe(provinceId);

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
		defenderPlayerId = sim.townManagerBase(provinceTownId)->lordPlayerId(); // Declare Independence
	} // TODO: Declare Independence should init attack from the Lord

	if (defenderPlayerId != -1)
	{
		PunUIUtils::SetPlayerLogo(
			PlayerLogoRight->GetDynamicMaterial(),
			dataSource()->playerInfo(defenderPlayerId),
			assetLoader()
		);

		AddToolTip(PlayerLogoRight, FText::Format(
			LOCTEXT("DefenderPlayerLogo_Tip", "Defender: {0}"),
			sim.playerNameT(defenderPlayerId)
		));

		PlayerLogoRight->SetVisibility(ESlateVisibility::Visible);
	}
	else {
		PlayerLogoRight->SetVisibility(ESlateVisibility::Hidden);
	}


	/*
	 * Text
	 */
	{
		TArray<FText> args;
		if (claimProgress.attackEnum == ProvinceAttackEnum::RaidBattle) {
			args.Add(LOCTEXT("Raid", "Raid"));
		}
		else if (claimProgress.attackEnum == ProvinceAttackEnum::Raze) {
			args.Add(LOCTEXT("Raze", "Raze"));
		}
		else if (claimProgress.attackEnum == ProvinceAttackEnum::RazeFort) {
			args.Add(LOCTEXT("Destroy Fort", "Destroy Fort"));
		}
		// Major Town Vassalize
		else if (provincePlayerId != -1 && sim.homeProvinceId(provincePlayerId) == provinceId)
		{
			if (claimProgress.attackEnum == ProvinceAttackEnum::DeclareIndependence) {
				args.Add(LOCTEXT("Declare Independence", "Declare Independence"));
			}
			else {
				args.Add(LOCTEXT("Vassalize", "Vassalize"));
			}
		}
		// Minor Town Vassalize
		else if (IsMinorTown(provinceTownId))
		{
			args.Add(LOCTEXT("Vassalize", "Vassalize"));
		}
		else {
			args.Add(LOCTEXT("Annex Province", "Annex Province"));
		}

		if (!IsMiniUI() &&
			!claimProgress.defenderStillAlive() &&
			claimProgress.isWaitingForBattleFinishCountdown())
		{
			args.Add(FText::Format(LOCTEXT(" ends in {0}s", " ends in {0}s"), TEXT_NUM(claimProgress.battleFinishCountdownTicks / Time::TicksPerSecond)));
		}

		
		SetText(BattleText, TEXT_TAG("<Shadowed>", JOINTEXT(args)));
	}
}


#undef LOCTEXT_NAMESPACE