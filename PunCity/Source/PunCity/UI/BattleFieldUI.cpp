// Pun Dumnernchanvanit's


#include "BattleFieldUI.h"

#include "BattleFieldArmyUI.h"
#include "BattleFieldUnitIcon.h"
#include "DamageFloatupUI.h"
#include "PunSpacerElement.h"

#define LOCTEXT_NAMESPACE "BattleFieldUI"

void UBattleFieldUI::UpdateUI(int32 provinceIdIn, ProvinceClaimProgress claimProgress)
{
	// Initialize
	if (provinceId == -1) {
		LeftArmyFrontOuterBox->ClearChildren();
		LeftArmyBackOuterBox->ClearChildren();
		RightArmyFrontOuterBox->ClearChildren();
		RightArmyBackOuterBox->ClearChildren();
	}
	
	provinceId = provinceIdIn;

	BUTTON_ON_CLICK(LeftReinforceButton, this, &UBattleFieldUI::OnClickReinforceLeft);
	BUTTON_ON_CLICK(RightReinforceButton, this, &UBattleFieldUI::OnClickReinforceRight);

	BUTTON_ON_CLICK(LeftRetreatButton, this, &UBattleFieldUI::OnClickRetreatButton);
	BUTTON_ON_CLICK(RightRetreatButton, this, &UBattleFieldUI::OnClickRetreatButton);

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
	{
		TArray<FText> args;
		if (claimProgress.attackEnum == ProvinceAttackEnum::RaidBattle) {
			args.Add(LOCTEXT("Raid", "Raid"));
		}
		else if (sim.homeProvinceId(provincePlayerId) == provinceId) {
			args.Add(LOCTEXT("Vassalize", "Vassalize"));
		}
		else {
			args.Add(LOCTEXT("Annex Province", "Annex Province"));
		}
		if (claimProgress.isWaitingForBattleFinishCountdown()) {
			args.Add(FText::Format(LOCTEXT(" ends in {0}s", " ends in {0}s"), TEXT_NUM(claimProgress.battleFinishCountdownSecs)));
		}
		SetText(BattleText, TEXT_TAG("<Shadowed>", JOINTEXT(args)));
	}


	// UI-Player is Attacker
	if (claimProgress.attackerPlayerId == playerId())
	{
		//int32 provincePlayerIdTemp = sim.provinceOwnerPlayer(claimProgress.provinceId); //TODO: does it needs claimProgress.provinceId?
		//auto& provincePlayerOwner = sim.playerOwned(provincePlayerIdTemp);

		//ClaimConnectionEnum claimConnectionEnum = sim.GetProvinceClaimConnectionEnumPlayer(provinceId, claimProgress.attackerPlayerId);
		//ProvinceAttackEnum attackEnum = provincePlayerOwner.GetProvinceAttackEnum(provinceId, claimProgress.attackerPlayerId);

		//int32 reinforcePrice = (attackEnum == ProvinceAttackEnum::DeclareIndependence) ? BattleInfluencePrice : sim.GetProvinceAttackReinforcePrice(provinceId, claimConnectionEnum);
		//bool hasEnoughInfluence = sim.influence(playerId()) >= reinforcePrice;


		
		//SetText(LeftReinforceText, LOCTEXT("Reinforce", "Reinforce"));
		//LeftReinforceButton->SetIsEnabled(hasEnoughInfluence);

		SetShowReinforceRetreat(true, true);
	}
	// UI-Player is Defender
	else if (provincePlayerId == playerId())
	{
		//int32 hasEnoughInfluence = sim.influence(playerId()) >= BattleInfluencePrice;
		//SetText(RightReinforceText,
		//	FText::Format(INVTEXT("{0}\n<img id=\"Influence\"/>{1}"), LOCTEXT("Reinforce", "Reinforce"), TextRed(TEXT_NUM(BattleInfluencePrice), !hasEnoughInfluence))
		//);
		//RightReinforceButton->SetIsEnabled(hasEnoughInfluence);


		SetShowReinforceRetreat(true, false);
	}
	else {
		SetShowReinforceRetreat(false, true);
	}

	/*
	 * Military Units
	 */
	//! Calculate Row Count
	auto getMaxRow = [&](const std::vector<CardStatus>& frontLine, const std::vector<CardStatus>& backLine, int32& minRowCount)
	{
		// Try different splits to see which one has minimum rows
		int32 bestSplitIndex = 1;
		minRowCount = INT32_MAX;

		for (int32 i = 1; i <= 4; i++) {
			int32 rows = std::max(frontLine.size() / i, backLine.size() / (5 - i));
			if (rows < minRowCount) {
				minRowCount = rows;
				bestSplitIndex = i;
			}
		}

		return bestSplitIndex;
	};

	int32 attackerMinRowCount;
	getMaxRow(claimProgress.attackerFrontLine, claimProgress.attackerBackLine, attackerMinRowCount);

	int32 defenderMinRowsCount;
	getMaxRow(claimProgress.defenderFrontLine, claimProgress.defenderBackLine, defenderMinRowsCount);

	int32 calculatedRowCount = std::max(1, std::max(attackerMinRowCount, defenderMinRowsCount));

	//! Fill Unit UI
	auto addUnits = [&](std::vector<CardStatus>& simUnits, UHorizontalBox* armyOuterBox)
	{
		int32 columnIndex = 0;
		int32 simUnitIndex = 0;
		
		while (simUnitIndex < simUnits.size())
		{
			auto armyColumn = GetBoxChild<UBattleFieldArmyUI>(armyOuterBox, columnIndex, UIEnum::WG_BattlefieldArmyUI, true);
			int32 unitIndex = 0;

			for (int32 i = 0; i < calculatedRowCount; i++)
			{
				CardStatus& simUnit = simUnits[simUnitIndex];
				int32 unitPlayerId = simUnit.cardStateValue1;
				
				auto unitIcon = GetBoxChild<UBattleFieldUnitIcon>(armyColumn->ArmyBox, unitIndex, UIEnum::WG_BattlefieldUnitIcon, true);
				unitIcon->UnitCountText->SetText(TEXT_NUM(simUnit.stackSize));

				PunUIUtils::SetPlayerLogo(unitIcon->UnitImage->GetDynamicMaterial(), dataSource()->playerInfo(unitPlayerId), assetLoader());

				MilitaryCardInfo militaryInfo = GetMilitaryInfo(simUnit.cardEnum);
				
				AddToolTip(unitIcon, FText::Format(
					LOCTEXT("UnitIcon_Tip", "{0} {1}/{2}"),
					GetBuildingInfo(simUnit.cardEnum).name,
					TEXT_NUM(simUnit.cardStateValue2),
					TEXT_NUM(militaryInfo.hp100)
				));

				/*
				 * Damage Floatup
				 */
				int32 lastDamage = simUnit.displayCardStateValue1;
				int32 lastDamageTick = simUnit.displayCardStateValue2;

				if (lastDamageTick != -1 &&
					Time::Ticks() - lastDamageTick < Time::TicksPerSecond &&
					lastDamageTick > unitIcon->lastDamageTick &&
					lastDamage > 0)
				{
					// Clear any existing damage that expired
					UOverlay* damageOverlay = unitIcon->DamageFloatupOverlay;
					for (int32 j = damageOverlay->GetChildrenCount(); j-- > 0;) {
						auto curFloat = CastChecked<UDamageFloatupUI>(damageOverlay->GetChildAt(j));
						if (UGameplayStatics::GetTimeSeconds(this) - curFloat->startTime > 10.0f) {
							damageOverlay->RemoveChildAt(i);
						}
					}

					
					UDamageFloatupUI* damageFloatup = AddWidget<UDamageFloatupUI>(UIEnum::DamageFloatup);
					damageFloatup->startTime = UGameplayStatics::GetTimeSeconds(this);
					
					damageOverlay->AddChild(damageFloatup);
					damageFloatup->DamageText->SetText(TEXT_NUM(lastDamage));
					unitIcon->lastDamageTick = lastDamageTick;

					auto animation = GetAnimation(damageFloatup, FString("Floatup"));
					damageFloatup->PlayAnimation(animation);

					//PUN_LOG("DamageOverlay: %d", unitUI->DamageOverlay->GetChildrenCount());
				}
				

				simUnitIndex++;
				if (simUnitIndex >= simUnits.size()) {
					break;
				}
			}

			BoxAfterAdd(armyColumn->ArmyBox, unitIndex);
		}

		BoxAfterAdd(armyOuterBox, columnIndex);
	};

	//! Attacker
	{
		addUnits(claimProgress.attackerBackLine, LeftArmyBackOuterBox);
		addUnits(claimProgress.attackerFrontLine, LeftArmyFrontOuterBox);
	}

	//! Defender
	{
		addUnits(claimProgress.defenderBackLine, RightArmyBackOuterBox);
		addUnits(claimProgress.defenderFrontLine, RightArmyFrontOuterBox);
	}

	//! Battle Bar
	{
		LeftDefenseBonus->SetText(TEXT_PERCENT(claimProgress.attackerDefenseBonus));
		RightDefenseBonus->SetText(TEXT_PERCENT(claimProgress.defenderDefenseBonus));
		
		int32 attackerArmyStrength = GetArmyStrength(claimProgress.attackerFrontLine) + GetArmyStrength(claimProgress.attackerBackLine);
		int32 defenderArmyStrength = GetArmyStrength(claimProgress.defenderFrontLine) + GetArmyStrength(claimProgress.defenderBackLine);
		
		LeftArmyStrength->SetText(TEXT_NUM(attackerArmyStrength));
		RightArmyStrength->SetText(TEXT_NUM(defenderArmyStrength));
		
		bool isUIPlayerAttacker = claimProgress.attackerPlayerId == playerId();

		// Battle Bar
		float fraction = static_cast<float>(attackerArmyStrength) / (attackerArmyStrength + defenderArmyStrength);

		BattleBarImage->GetDynamicMaterial()->SetScalarParameterValue("Fraction", 1.0f - fraction);
		BattleBarImage->GetDynamicMaterial()->SetScalarParameterValue("IsGreenLeft", isUIPlayerAttacker);
		BattleBarImage->SetVisibility(ESlateVisibility::SelfHitTestInvisible);
	}
}


#undef LOCTEXT_NAMESPACE