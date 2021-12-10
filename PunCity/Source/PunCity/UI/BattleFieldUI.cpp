// Pun Dumnernchanvanit's


#include "BattleFieldUI.h"

#include "BattleFieldArmyUI.h"
#include "BattleFieldUnitIcon.h"
#include "DamageFloatupUI.h"
#include "PunSpacerElement.h"

#define LOCTEXT_NAMESPACE "BattleFieldUI"

static const std::unordered_map<CardEnum, std::string> AttackSoundNames
{
	{ CardEnum::Archer, "Battle_Attack_Archer" },
	{ CardEnum::Artillery, "Battle_Attack_Artillery" },
	{ CardEnum::Battleship, "Battle_Attack_Battleship" },
	{ CardEnum::Cannon, "Battle_Attack_Cannon" },
	{ CardEnum::Catapult, "Battle_Attack_Catapult" },

	{ CardEnum::Conscript, "Battle_Attack_Conscript" },
	{ CardEnum::Frigate, "Battle_Attack_Frigate" },
	{ CardEnum::Galley, "Battle_Attack_Galley" },
	{ CardEnum::Infantry, "Battle_Attack_Infantry" },
	//{ CardEnum::KnightArab, "Battle_Attack_KnightArab" },

	{ CardEnum::Knight, "Battle_Attack_Knight" },
	{ CardEnum::MachineGun, "Battle_Attack_MachineGun" },
	//{ CardEnum::Conscript, "Battle_Attack_MilitiaArab" },
	{ CardEnum::Militia, "Battle_Attack_Militia" },
	{ CardEnum::Musketeer, "Battle_Attack_Musketeer" },

	//{ CardEnum::Conscript, "Battle_Attack_NationalGuard" },
	{ CardEnum::Swordsman, "Battle_Attack_Swordman" },
	{ CardEnum::Tank, "Battle_Attack_Tank" },
	{ CardEnum::Warrior, "Battle_Attack_Warrior" },
};

std::string GetAttackSoundName(CardEnum cardEnum)
{
	auto it = AttackSoundNames.find(cardEnum);
	if (it != AttackSoundNames.end()) {
		return it->second;
	}
	return AttackSoundNames.find(CardEnum::Archer)->second;
}

static const std::unordered_map<CardEnum, std::string> UnitLossSoundNames
{
	{ CardEnum::Archer, "Battle_Loss_Human" },
	{ CardEnum::Artillery, "Battle_Loss_Metal" },
	{ CardEnum::Battleship, "Battle_Loss_Battleship" },
	{ CardEnum::Cannon, "Battle_Loss_Metal" },
	{ CardEnum::Catapult, "Battle_Loss_Wood" },

	{ CardEnum::Conscript, "Battle_Loss_Human" },
	{ CardEnum::Frigate, "Battle_Loss_Frigate" },
	{ CardEnum::Galley, "Battle_Loss_Galley" },
	{ CardEnum::Infantry, "Battle_Loss_Human" },
	//{ CardEnum::KnightArab, "Battle_Attack_KnightArab" },

	{ CardEnum::Knight, "Battle_Loss_Human" },
	{ CardEnum::MachineGun, "Battle_Loss_Human" },
	//{ CardEnum::Conscript, "Battle_Attack_MilitiaArab" },
	{ CardEnum::Militia, "Battle_Loss_Human" },
	{ CardEnum::Musketeer, "Battle_Loss_Human" },

	//{ CardEnum::Conscript, "Battle_Attack_NationalGuard" },
	{ CardEnum::Swordsman, "Battle_Loss_Human" },
	{ CardEnum::Tank, "Battle_Loss_Metal" },
	{ CardEnum::Warrior, "Battle_Loss_Human" },

	{ CardEnum::Wall, "Battle_Loss_StoneBuilding" },
	{ CardEnum::RaidTreasure, "Battle_Loss_Treasure" },
};


std::string GetUnitLossSoundName(CardEnum cardEnum)
{
	auto it = UnitLossSoundNames.find(cardEnum);
	if (it != UnitLossSoundNames.end()) {
		return it->second;
	}
	return UnitLossSoundNames.find(CardEnum::Archer)->second;
}



void UBattleFieldUI::UpdateBattleFieldUI(int32 provinceIdIn, ProvinceClaimProgress claimProgress, bool showAttacher)
{
	// Beyond a zoom range, use mini UI
	if (dataSource()->ZoomDistanceAbove(WorldZoomTransition_Region4x4ToMap))
	{
		MiniBattleField->SetVisibility(ESlateVisibility::SelfHitTestInvisible);
		FullBattleField->SetVisibility(ESlateVisibility::Collapsed);

		SetChildHUD(MiniBattleField);
		MiniBattleField->UpdateUIBase(provinceIdIn, claimProgress, showAttacher);
		return;
	}
	FullBattleField->SetVisibility(ESlateVisibility::SelfHitTestInvisible);
	MiniBattleField->SetVisibility(ESlateVisibility::Collapsed);

	
	// Initialize
	if (provinceId == -1 || provinceId != provinceIdIn) 
	{
		provinceId = provinceIdIn;
		
		LeftArmyFrontOuterBox->ClearChildren();
		LeftArmyBackOuterBox->ClearChildren();
		RightArmyFrontOuterBox->ClearChildren();
		RightArmyBackOuterBox->ClearChildren();

		float openDuration = BattleOpeningSpine->GetAnimationDuration("Open") - 0.2;
		BattleOpeningSpine->animationDoneSec = openDuration + GetWorld()->GetTimeSeconds();
		BattleOpeningSpine->SetAnimation(0, "Open", true);
		BattleOpeningSpine->SetVisibility(ESlateVisibility::SelfHitTestInvisible);

		simulation().soundInterface()->Spawn3DSound("CitizenAction", "BattleBegin", simulation().GetProvinceCenterTile(provinceId).worldAtom2());
	}

	// Battle Opening Animation
	if (GetWorld()->GetTimeSeconds() > BattleOpeningSpine->animationDoneSec) {
		BattleOpeningSpine->SetVisibility(ESlateVisibility::Collapsed);
	}
	BattleOpeningSpine->PunTick();

	GroundAttacher->SetVisibility(showAttacher ? ESlateVisibility::HitTestInvisible : ESlateVisibility::Collapsed);

	BUTTON_ON_CLICK(LeftReinforceButton, this, &UBattleFieldUI::OnClickReinforceLeft);
	BUTTON_ON_CLICK(RightReinforceButton, this, &UBattleFieldUI::OnClickReinforceRight);

	BUTTON_ON_CLICK(LeftRetreatButton, this, &UBattleFieldUI::OnClickRetreatButton);
	BUTTON_ON_CLICK(RightRetreatButton, this, &UBattleFieldUI::OnClickRetreatButton);

	auto& sim = simulation();

	int32 provincePlayerId = sim.provinceOwnerPlayer(provinceId);
	int32 provinceTownId = sim.provinceOwnerTownSafe(provinceId);

	UpdateUIBase(provinceIdIn, claimProgress, showAttacher);

	// Update Animation Speed if game speed changed
	int32 currentGameSpeed = sim.gameSpeed();
	bool shouldUpdateGameSpeed = (lastGameSpeed != currentGameSpeed);
	lastGameSpeed = currentGameSpeed;


	// UI-Player is Attacker
	if (claimProgress.attackerPlayerId == playerId())
	{
		SetShowReinforceRetreat(true, true, false, false);
	}
	// UI-Player is Defender
	else if (provincePlayerId == playerId())
	{
		SetShowReinforceRetreat(false, false, true, true);
	}
	// UI-Player is Lord Crushing Rebel (Lord Defender)
	else if (sim.townManagerBase(provinceTownId)->lordPlayerId() == playerId()) 
	{
		SetShowReinforceRetreat(false, false, true, true);
	}
	else {
		int32 reinforcingAttacker = claimProgress.IsPlayerReinforcingAttacker(playerId());
		int32 reinforcingDefender = claimProgress.IsPlayerReinforcingDefender(playerId());
		SetShowReinforceRetreat(!reinforcingDefender, reinforcingAttacker,
								!reinforcingAttacker, reinforcingDefender);
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

		for (int32 i = 1; i <= 4; i++) 
		{
			int32 rows = std::max((frontLine.size() + i - 1) / i, (backLine.size() + (5 - i) - 1) / (5 - i));
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
	auto addUnits = [&](std::vector<CardStatus> simUnits, 
						const std::vector<CardStatus>& simUnits_pendingRemoval, 
						UHorizontalBox* armyOuterBox, bool isLeft)
	{
		// Sort units by card enum
		// so they are in same order even with pending removal added
		simUnits.insert(simUnits.end(), simUnits_pendingRemoval.begin(), simUnits_pendingRemoval.end());
		std::sort(simUnits.begin(), simUnits.end(), [&](const CardStatus& a, const CardStatus& b) {
			if (static_cast<int32>(a.cardEnum) == static_cast<int32>(b.cardEnum)) {
				return a.cardStateValue1 > b.cardStateValue1; // compare playerId
			}
			return static_cast<int32>(a.cardEnum) > static_cast<int32>(b.cardEnum);
		});
		
		int32 columnIndex = 0;
		int32 simUnitIndex = 0;
		
		while (simUnitIndex < simUnits.size())
		{
			auto armyColumn = GetBoxChild<UBattleFieldArmyUI>(armyOuterBox, columnIndex, UIEnum::WG_BattlefieldArmyUI, true);
			int32 unitIndex = 0;

			for (int32 i = 0; i < calculatedRowCount; i++)
			{
				const CardStatus& simUnit = simUnits[simUnitIndex];
				int32 unitPlayerId = simUnit.cardStateValue1;
				
				auto unitIcon = GetBoxChild<UBattleFieldUnitIcon>(armyColumn->ArmyBox, unitIndex, UIEnum::WG_BattlefieldUnitIcon, true);
				unitIcon->UnitCountText->SetText(TEXT_NUM(simUnit.stackSize));

				//LogoImage
				//PunUIUtils::SetPlayerLogo(unitIcon->LogoImage, unitIcon->UnitImage, dataSource()->playerInfo(unitPlayerId), assetLoader());
				
				unitIcon->BackgroundImage->GetDynamicMaterial()->SetVectorParameterValue("ColorBackground", dataSource()->playerInfo(unitPlayerId).logoColorBackground);
				unitIcon->UnitImage->PunTick();
				unitIcon->UnitImage->SetRenderScale(FVector2D(isLeft ? 1 : -1, 1));

				FSpineAsset spineAsset = assetLoader()->GetSpine(simUnit.cardEnum);
				unitIcon->UnitImage->Atlas = spineAsset.atlas;
				unitIcon->UnitImage->SkeletonData = spineAsset.skeletonData;

				unitIcon->TickFXSpine();

				MilitaryCardInfo militaryInfo = GetMilitaryInfo(simUnit.cardEnum);

				int32 currentHP = simUnit.cardStateValue2 / 100;
				int32 unitHP = militaryInfo.hp100 / 100;
				if (simUnit.cardEnum == CardEnum::RaidTreasure ||
					simUnit.cardEnum == CardEnum::Wall) 
				{
					unitHP = simUnit.cardStateValue3 / 100;
				}

				FText nameText = GetBuildingInfo(simUnit.cardEnum).name;
				if (simUnit.cardEnum == CardEnum::RaidTreasure) {
					nameText = FText::Format(
						LOCTEXT("Raid_Treasure_Tooltip", "Raid Treasure <img id=\"Coin\"/>{0}"),
						TEXT_NUM(claimProgress.raidMoney)
					);
				}

				if (simUnit.stackSize == 0)
				{
					/*
					 * Pending Death
					 */
					int32 deathTick = simUnit.displayCardStateValue3;
					if (deathTick != -1 &&
						deathTick > unitIcon->lastDeathTick) // lastAttack is still the same as the current one
					{
						unitIcon->lastDeathTick = deathTick;
						unitIcon->UnitImage->SetAnimation(0, "Dead", false);
						unitIcon->UnitImage->SetTimeScale(sim.gameSpeedFloat());

						std::string unitLossName = GetUnitLossSoundName(simUnit.cardEnum);
						sim.soundInterface()->Spawn3DSound("CitizenAction", unitLossName, sim.GetProvinceCenterTile(provinceId).worldAtom2());
					}

					AddToolTip(unitIcon,
						LOCTEXT("DeadUnitIcon_Tip", "Dead Unit")
					);
				}
				else
				{
					int32 armyCurrentHP = currentHP + unitHP * (simUnit.stackSize - 1);
					int32 armyHP = unitHP * simUnit.stackSize;

					AddToolTip(unitIcon, FText::Format(
						LOCTEXT("UnitIcon_Tip", "{0}<space><img id=\"Sword\"/>{1} \n<img id=\"Shield\"/>{2}<space>HP: {3}<space>Army Attack: {4}\nArmy HP: {5}"),
						nameText,
						TEXT_NUM(militaryInfo.attackDisplay()),
						TEXT_NUM(militaryInfo.defenseDisplay()),
						TextNumberColor(
							FText::Format(INVTEXT("{0}/{1}"), TEXT_NUM(currentHP), TEXT_NUM(unitHP)),
							currentHP * 100 / unitHP, 60, 30
						),
						TEXT_NUM(militaryInfo.attackDisplay() * simUnit.stackSize),
						TextNumberColor(
							FText::Format(INVTEXT("{0}/{1}"), TEXT_NUM(armyCurrentHP), TEXT_NUM(armyHP)),
							armyCurrentHP * 100 / max(1, armyHP), 60, 30
						)
					));
					
					/*
					 * Attack
					 */
					int32 lastAttackTick = simUnit.displayCardStateValue3;
					if (lastAttackTick != -1 &&
						Time::Ticks() - lastAttackTick < Time::TicksPerSecond && // if more than 1 sec already passed, don't show the damage
						lastAttackTick > unitIcon->lastAttackTick) // lastAttack is still the same as the current one
					{
						unitIcon->lastAttackTick = lastAttackTick;
						unitIcon->UnitImage->SetAnimation(0, "Attack", false);
						unitIcon->UnitImage->SetTimeScale(sim.gameSpeedFloat());

						sim.soundInterface()->Spawn3DSound("CitizenAction", GetAttackSoundName(simUnit.cardEnum), sim.GetProvinceCenterTile(provinceId).worldAtom2());
					}
				}
				
				/*
				 * Damage Floatup
				 */
				int32 lastDamage = simUnit.displayCardStateValue1;
				int32 lastDamageTick = simUnit.displayCardStateValue2;

				if (lastDamageTick != -1 &&
					lastDamage > 0 &&
					//Time::Ticks() - lastDamageTick > Time::TicksPerSecond * 2 / 3 && // Damage Delay
					Time::Ticks() - lastDamageTick < Time::TicksPerSecond * 5 / 3 && // if more than 1 sec already passed, don't show the damage
					lastDamageTick > unitIcon->lastDamageTick) // lastDamage is still the same as the current one
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

					// Don't hit if stack size is 0 (die instead)
					if (simUnit.stackSize > 0) {
						unitIcon->UnitImage->SetAnimation(0, "Hit", false);
						unitIcon->UnitImage->SetTimeScale(sim.gameSpeedFloat());

						CardEnum attackerEnum = static_cast<CardEnum>(simUnit.displayCardStateValue4);
						FSpineAsset attackerSpineAsset = assetLoader()->GetSpine(attackerEnum);

						SetChildHUD(unitIcon);
						unitIcon->AddFXSpine(attackerSpineAsset.atlas_fx, attackerSpineAsset.skeletonData_fx, sim.gameSpeedFloat());
						//unitIcon->FXImage->Atlas = attackerSpineAsset.atlas_fx;
						//unitIcon->FXImage->SkeletonData = attackerSpineAsset.skeletonData_fx;
						//unitIcon->FXImage->SetAnimation(0, "Attack", false);
						//unitIcon->FXImage->SetTimeScale(sim.gameSpeedFloat());
						//
						//unitIcon->FXCompleteTime = GetWorld()->GetTimeSeconds() + ProvinceClaimProgress::AnimationLengthSecs / sim.gameSpeedFloat();
					}
					
					//PUN_LOG("DamageOverlay: %d", unitUI->DamageOverlay->GetChildrenCount());
				}

				
				if (shouldUpdateGameSpeed)
				{
					unitIcon->UnitImage->SetTimeScale(sim.gameSpeedFloat());
					TArray<UWG_PunSpine*> fxSpines = unitIcon->FXSpines;
					for (int32 j = fxSpines.Num(); j-- > 0;) {
						if (fxSpines[j]->IsVisible()) {
							fxSpines[j]->Spine->SetTimeScale(sim.gameSpeedFloat());
						}
					}
				}
				

				simUnitIndex++;
				if (simUnitIndex >= simUnits.size()) {
					break;
				}
			}

			BoxAfterAdd(armyColumn->ArmyBox, unitIndex);
		}

		BoxAfterAdd(armyOuterBox, columnIndex);

		armyOuterBox->SetVisibility(simUnits.size() > 0 ? ESlateVisibility::SelfHitTestInvisible : ESlateVisibility::Collapsed);
	};

	//! Attacker
	{
		addUnits(claimProgress.attackerBackLine, claimProgress.attackerBackLine_pendingRemoval, 
			LeftArmyBackOuterBox, true
		);
		addUnits(claimProgress.attackerFrontLine, claimProgress.attackerFrontLine_pendingRemoval,
			LeftArmyFrontOuterBox, true
		);
	}

	//! Defender
	{
		addUnits(claimProgress.defenderBackLine, claimProgress.defenderBackLine_pendingRemoval,
			RightArmyBackOuterBox, false
		);
		addUnits(claimProgress.defenderFrontLine, claimProgress.defenderFrontLine_pendingRemoval,
			RightArmyFrontOuterBox, false
		);

		addUnits(claimProgress.defenderWall, claimProgress.defenderWall_pendingRemoval,
			RightArmyWall, false
		);
		addUnits(claimProgress.defenderTreasure, claimProgress.defenderTreasure_pendingRemoval,
			RightArmyTreasure, false
		);
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
		
		
		//// Army Strength
		//int32 attackerArmyStrength = GetArmyStrength(claimProgress.attackerFrontLine) + GetArmyStrength(claimProgress.attackerBackLine);
		//int32 defenderArmyStrength = GetArmyStrength(claimProgress.defenderFrontLine) +  GetArmyStrength(claimProgress.defenderBackLine) + GetArmyStrength(claimProgress.defenderWall);
		//
		//LeftArmyStrength->SetText(TEXT_NUM(attackerArmyStrength));
		//RightArmyStrength->SetText(TEXT_NUM(defenderArmyStrength));
		//
		//bool isUIPlayerAttacker = claimProgress.attackerPlayerId == playerId();

		//// Battle Bar
		//float fraction = static_cast<float>(attackerArmyStrength) / (attackerArmyStrength + defenderArmyStrength);

		//BattleBarImage->GetDynamicMaterial()->SetScalarParameterValue("Fraction", 1.0f - fraction);
		//BattleBarImage->GetDynamicMaterial()->SetScalarParameterValue("IsGreenLeft", isUIPlayerAttacker);
		//BattleBarImage->SetVisibility(ESlateVisibility::SelfHitTestInvisible);
	}
}


#undef LOCTEXT_NAMESPACE