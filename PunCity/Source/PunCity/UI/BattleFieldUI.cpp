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
	

	// Fight at home province = Vassalize
	{
		TArray<FText> args;
		if (claimProgress.attackEnum == ProvinceAttackEnum::RaidBattle) {
			args.Add(LOCTEXT("Raid", "Raid"));
		}
		else if (claimProgress.attackEnum == ProvinceAttackEnum::Raze) {
			args.Add(LOCTEXT("Raze", "Raze"));
		}
		// Major Town Vassalize
		else if (provincePlayerId != -1 && sim.homeProvinceId(provincePlayerId) == provinceId) 
		{
			args.Add(LOCTEXT("Vassalize", "Vassalize"));
		}
		// Minor Town Vassalize
		else if (IsMinorTown(provinceTownId))
		{
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

				//LogoImage
				PunUIUtils::SetPlayerLogo(unitIcon->LogoImage, unitIcon->UnitImage, dataSource()->playerInfo(unitPlayerId), assetLoader());

				MilitaryCardInfo militaryInfo = GetMilitaryInfo(simUnit.cardEnum);

				int32 currentHP = simUnit.cardStateValue2 / 100;
				int32 unitHP = militaryInfo.hp100 / 100;
				
				AddToolTip(unitIcon, FText::Format(
					LOCTEXT("UnitIcon_Tip", "{0}<space>Unit HP: {1}/{2}\nUnit Count: {3}<space>Army HP: {4}/{5}"),
					GetBuildingInfo(simUnit.cardEnum).name,
					TEXT_NUM(currentHP),
					TEXT_NUM(unitHP),
					TEXT_NUM(simUnit.stackSize),
					TEXT_NUM(currentHP + unitHP * (simUnit.stackSize - 1)),
					TEXT_NUM(unitHP * simUnit.stackSize)
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
					damageFloatup->DamageText->SetText(TEXT_NUM(lastDamage / 100));
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
		// Bonuses
		auto fillBonusText = [&](URichTextBlock* bonusText, int32 bonusPercent, bool isAttackBonus, int32 bonusTownId)
		{
			// Text
			FText iconText;
			FText tagText;
			if (bonusPercent >= 0) {
				iconText = isAttackBonus ? INVTEXT("<img id=\"Sword\"/>") : INVTEXT("<img id=\"Shield\"/>");
				tagText = INVTEXT("<Shadowed>");
			} else {
				iconText = isAttackBonus ? INVTEXT("<img id=\"SwordRed\"/>") : INVTEXT("<img id=\"ShieldRed\"/>");
				tagText = INVTEXT("<Red>");
			}
			
			SetText(bonusText, FText::Format(INVTEXT("{0}{1}{2}</>"), iconText, tagText, TEXT_PERCENT(bonusPercent)));

			
			// Tip
			std::vector<BonusPair> bonuses = isAttackBonus ? sim.GetAttackBonuses(provinceId, bonusTownId) : sim.GetDefenseBonuses(provinceId, bonusTownId);

			TArray<FText> args;
			if (isAttackBonus) {
				args.Add(FText::Format(LOCTEXT("Player Attack Bonus", "{0}'s Attack Bonuses:"), sim.townOrPlayerNameT(bonusTownId)));
			} else {
				args.Add(FText::Format(LOCTEXT("Player Defense Bonus", "{0}'s Defense Bonuses:"), sim.townOrPlayerNameT(bonusTownId)));
			}
			args.Add(INVTEXT(":\n"));
			for (const BonusPair& bonus : bonuses) {
				args.Add(FText::Format(INVTEXT("  {0} {1}"), TEXT_PERCENT(bonus.value), bonus.name));
			}
			if (bonuses.size() == 0) {
				args.Add(LOCTEXT("No Bonus", "No Bonus"));
			}
			
			AddToolTip(bonusText, args);
		};

		fillBonusText(LeftAttackBonus, claimProgress.attacker_attackBonus, true, claimProgress.attackerPlayerId);
		fillBonusText(LeftDefenseBonus, claimProgress.attacker_defenseBonus, false, claimProgress.attackerPlayerId);
		fillBonusText(RightAttackBonus, claimProgress.defender_attackBonus, true, claimProgress.defenderTownId);
		fillBonusText(RightDefenseBonus, claimProgress.defender_defenseBonus, false, claimProgress.defenderTownId);
		
		
		// Army Strength
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