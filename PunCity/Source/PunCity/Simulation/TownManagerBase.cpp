// Pun Dumnernchanvanit's


#include "TownManagerBase.h"
#include "BuildingCardSystem.h"
#include "PlayerOwnedManager.h"

void ProvinceClaimProgress::Reinforce(std::vector<CardStatus>& militaryCards, bool isReinforcingAttacker, int32 unitPlayerId)
{
	PrepareCardStatusForMilitary(unitPlayerId, militaryCards);

	auto& frontLine = isReinforcingAttacker ? attackerFrontLine : defenderFrontLine;
	auto& backLine = isReinforcingAttacker ? attackerBackLine : defenderBackLine;

	for (const CardStatus& card : militaryCards)
	{
		check(IsLandMilitaryCardEnum(card.cardEnum));
		if (IsFrontlineCardEnum(card.cardEnum)) {
			AddMilitaryCards(frontLine, card);
		} else {
			AddMilitaryCards(backLine, card);
		}
	}
}

void ProvinceClaimProgress::Retreat_DeleteUnits(int32 unitPlayerId)
{
	auto retreat = [&](std::vector<CardStatus>& line) {
		for (int32 i = line.size(); i-- > 0;) {
			if (line[i].cardStateValue1 == unitPlayerId) {
				line.erase(line.begin() + i);
			}
		}
	};
	retreat(attackerFrontLine);
	retreat(attackerBackLine);
	retreat(defenderFrontLine);
	retreat(defenderBackLine);
}


void ProvinceClaimProgress::Tick1Sec(IGameSimulationCore* simulation)
{
	/*
	 * cardStateValue1 = playerId
	 * cardStateValue2 = HP
	 * displayCardStateValue1 = last damage
	 * displayCardStateValue2 = last damage tick
	 */

	auto doDamage = [&](std::vector<CardStatus>& lineCards, std::vector<CardStatus>& damageTakerLine, int32 attackBonusPercent, int32 defenseBonusPercent)
	{
		int32 randomIndex = GameRand::Rand() % damageTakerLine.size();
		CardStatus& damageTaker = damageTakerLine[randomIndex];

		int32 attack = 0;
		for (CardStatus& card : lineCards) {
			attack += GetMilitaryInfo(card.cardEnum).attack100 * card.stackSize;
		}

		// Damage Bonus
		attack = attack * (100 + attackBonusPercent) / 100;

		MilitaryCardInfo damageTakerInfo = GetMilitaryInfo(damageTaker.cardEnum);

		int32 defense100 = damageTakerInfo.defense100 * (100 + defenseBonusPercent) / 100;
		
		int32 damage = attack * 100 / std::max(1, defense100);
		damageTaker.displayCardStateValue1 = damage;
		damageTaker.displayCardStateValue2 = Time::Ticks();

		while (damage > 0 && damageTaker.stackSize > 0)
		{
			int32 unitDamage = std::min(damage, damageTaker.cardStateValue2);
			damageTaker.cardStateValue2 -= unitDamage;
			damage -= unitDamage;

			if (damageTaker.cardStateValue2 == 0) {
				damageTaker.cardStateValue2 = damageTakerInfo.hp100;
				damageTaker.stackSize--;
			}
		}

		if (damageTaker.stackSize <= 0) {
			damageTakerLine.erase(damageTakerLine.begin() + randomIndex);
		}
	};

	auto doDamageToArmy = [&](std::vector<CardStatus>& attackerLine, std::vector<CardStatus>& damageTakerFrontLine, std::vector<CardStatus>& damageTakerBackLine, int32 attackBonus, int32 defenseBonus)
	{
		if (damageTakerFrontLine.size() > 0) {
			doDamage(attackerLine, damageTakerFrontLine, attackBonus, defenseBonus);
		}
		else if (damageTakerBackLine.size() > 0) {
			doDamage(attackerLine, damageTakerBackLine, attackBonus, defenseBonus);
		}
	};

	//! Defender Turn
	if (Time::Seconds() % 8 == 0)
	{
		doDamageToArmy(defenderBackLine, attackerFrontLine, attackerBackLine, defender_attackBonus, attacker_defenseBonus);
	}
	else if (Time::Seconds() % 8 == 1)
	{
		doDamageToArmy(defenderFrontLine, attackerFrontLine, attackerBackLine, defender_attackBonus, attacker_defenseBonus);
	}
	//! Attacker Turn
	else if (Time::Seconds() % 8 == 4)
	{
		doDamageToArmy(attackerBackLine, defenderFrontLine, defenderBackLine, attacker_attackBonus, defender_defenseBonus);
	}
	else if (Time::Seconds() % 8 == 5)
	{
		doDamageToArmy(attackerFrontLine, defenderFrontLine, defenderBackLine, attacker_attackBonus, defender_defenseBonus);
	}

	//! Battle Countdown once it is done
	battleFinishCountdownSecs = std::max(0, battleFinishCountdownSecs - 1);
}

/*
 * TownManagerBase
 */

void TownManagerBase::Tick1Sec_TownBase()
{
	// Defense
	for (ProvinceClaimProgress& claimProgress : _defendingClaimProgress) {
		claimProgress.Tick1Sec(_simulation);
	}


	/*
	 * Minor Town
	 */
	if (IsMinorTown(_townId))
	{
		//! Income every X secs
		if (Time::Seconds() % GameConstants::BaseFloatupIntervalSec == 0)
		{
			int32 incomePerRound = GetMinorCityMoneyIncome();
			int32 incomePer2Sec = GameRand::RandRound(incomePerRound * GameConstants::BaseFloatupIntervalSec, Time::SecondsPerRound);

			if (incomePer2Sec > 0) {
				_simulation->uiInterface()->ShowFloatupInfo(FloatupEnum::GainMoney, _simulation->building(townhallId).centerTile(), TEXT_NUMSIGNED(incomePer2Sec));
				_minorCityWealth += incomePer2Sec;
			}
		}

		//! Refresh Target Wealth every 30 secs
		if (Time::Seconds() % 30 == 0)
		{
			RefreshMinorCityTargetWealth();
		}
	}


	Tick1SecRelationship();
}

void TownManagerBase::ReturnMilitaryUnitCards(std::vector<CardStatus>& cards, int32 playerIdToReturn, bool forcedAll, bool isRetreating)
{
	check(_simulation->IsValidPlayer(playerIdToReturn));
	for (CardStatus card : cards) {
		if (forcedAll || playerIdToReturn == card.cardStateValue1) 
		{
			// Retreating death
			if (isRetreating) {
				card.stackSize = (card.stackSize + 1) * 2 / 3;
			}
			
			_simulation->cardSystem(playerIdToReturn).TryAddCards_BoughtHandAndInventory(card);
		}
	}
}

void TownManagerBase::RefreshMinorCityTargetWealth()
{
	const int32 maxTileDistanceToCheck = 300;
	int32 maxTownRevenue = 0;
	WorldTile2 thisTownGateTile = _simulation->GetTownhallGate_All(_townId);
	
	_simulation->ExecuteOnMajorTowns([&](int32 townId) 
	{
		WorldTile2 gateTile = _simulation->GetMajorTownhallGate(townId);
		if (gateTile.isValid() &&
			WorldTile2::Distance(gateTile, thisTownGateTile) < maxTileDistanceToCheck)
		{
			maxTownRevenue = std::max(maxTownRevenue, _simulation->GetMajorTownTotalRevenue100(townId) / 100);
		}
	});

	_minorCityTargetWealth = maxTownRevenue;
}

void TownManagerBase::AddChildBuilding(Building& child)
{
	_childBuildingIds.push_back(child.buildingId());
}

void TownManagerBase::CalculateAutoTradeProfit(
	int32& exportMoneyTotal,
	int32& importMoneyTotal,
	int32& feeTotal,
	int32& feeDiscountTotal,
	TArray<FText>& exportTooltipText,
	TArray<FText>& importTooltipText,
	TArray<FText>& feeTooltipText,
	TArray<FText>& feeDiscountTooltipText,
	bool shouldFillTipText)
{
	auto& worldTradeSys = _simulation->worldTradeSystem();

	// Check all Trade Route to find any fee discount
	// - Do this separately with TradeRoutePair so we can show which TradeRoute is giving the discount

	auto addResource = [&](std::vector<ResourcePair>& vec, ResourcePair pair)
	{
		for (int32 i = 0; i < vec.size(); i++) {
			if (vec[i].resourceEnum == pair.resourceEnum) {
				vec[i].count += pair.count;
				return;
			}
		}
		vec.push_back(pair);
	};

	auto findDirectResource = [&](std::vector<ResourcePair>& vec, ResourceEnum resourceEnum) {
		for (int32 i = 0; i < vec.size(); i++) {
			if (vec[i].resourceEnum == resourceEnum) {
				return vec[i].count;
			}
		}
		return 0;
	};

	//const int32 feePercent = 40; // TODO: allow people to manipulate this by building more Trading Company etc.
	int32 feePercent = Building::baseTradingFeePercent(_townId, _simulation, true);
	

	std::vector<ResourcePair> directExportResources;
	std::vector<ResourcePair> directImportResources;

	//! fill directExport/ImportResources
	//! fee tooltip text
	const std::vector<TradeRoutePair>& tradeRoutePairs = worldTradeSys.tradeRoutePairs();
	for (const TradeRoutePair& tradeRoutePair : tradeRoutePairs)
	{
		if (tradeRoutePair.HasTownId(_townId))
		{
			for (const TradeRouteResourcePair& tradeResourcePair : tradeRoutePair.tradeResources)
			{
				bool isExport = (tradeRoutePair.townId1 == _townId && tradeResourcePair.isTown1ToTown2) ||
					(tradeRoutePair.townId2 == _townId && !tradeResourcePair.isTown1ToTown2);

				ResourcePair pair = tradeResourcePair.resourcePair;

				if (isExport) {
					addResource(directExportResources, pair);
				}
				else {
					addResource(directImportResources, pair);
				}

				// feeDiscountTooltipText
				if (shouldFillTipText)
				{
					int32 feeDiscount = pair.count * worldTradeSys.price100(pair.resourceEnum) * feePercent / 100;
					if (feeDiscount > 0) {
						FText mainText = isExport ?
							NSLOCTEXT("AutoTradeUI", "feeDiscountTooltipText_Export", "\n  {0}<img id=\"Coin\"/> export {1} {2} to {3}") :
							NSLOCTEXT("AutoTradeUI", "feeDiscountTooltipText_Import", "\n  {0}<img id=\"Coin\"/> import {1} {2} from {3}");

						feeDiscountTooltipText.Add(FText::Format(
							mainText,
							TEXT_100(feeDiscount),
							TEXT_NUM(pair.count),
							GetResourceInfo(pair.resourceEnum).name,
							_simulation->townNameT(_townId)
						));
					}
				}
			}
		}
	}


	for (AutoTradeElement& exportElement : _autoExportElements)
	{
		exportElement.CalculateMoney(
			worldTradeSys.price100(exportElement.resourceEnum),
			findDirectResource(directExportResources, exportElement.resourceEnum),
			feePercent
		);

		exportMoneyTotal += exportElement.calculatedMoney;
		feeDiscountTotal += exportElement.calculatedFeeDiscount;
		feeTotal += exportElement.calculatedFee;

		if (shouldFillTipText)
		{
			FText resourceName = GetResourceInfo(exportElement.resourceEnum).GetName();

			exportTooltipText.Add(FText::Format(
				NSLOCTEXT("AutoTradeUI", "exportTooltipText_Elem", "  {0}<img id=\"Coin\"/> {1}\n"), TEXT_100(exportElement.calculatedMoney), resourceName
			));

			if (exportElement.calculatedFee > 0) {
				feeTooltipText.Add(FText::Format(
					NSLOCTEXT("AutoTradeUI", "exportFeeTooltipText_Elem", "  {0}<img id=\"Coin\"/> {1} export\n"), TEXT_100(exportElement.calculatedFee), resourceName
				));
			}
		}
	}

	for (AutoTradeElement& importElement : _autoImportElements)
	{
		importElement.CalculateMoney(
			worldTradeSys.price100(importElement.resourceEnum),
			findDirectResource(directImportResources, importElement.resourceEnum),
			feePercent
		);

		importMoneyTotal += importElement.calculatedMoney;
		feeDiscountTotal += importElement.calculatedFeeDiscount;
		feeTotal += importElement.calculatedFee;


		if (shouldFillTipText)
		{
			FText resourceName = GetResourceInfo(importElement.resourceEnum).GetName();

			importTooltipText.Add(FText::Format(
				NSLOCTEXT("AutoTradeUI", "importTooltipText_Elem", "  {0}<img id=\"Coin\"/> {1}\n"), TEXT_100(importElement.calculatedMoney), resourceName
			));

			if (importElement.calculatedFee > 0) {
				feeTooltipText.Add(FText::Format(
					NSLOCTEXT("AutoTradeUI", "importFeeTooltipText_Elem", "  {0}<img id=\"Coin\"/> {1} import\n"), TEXT_100(importElement.calculatedFee), resourceName
				));
			}
		}
	}

}

void TownManagerBase::Tick1SecRelationship()
{
	SCOPE_CYCLE_COUNTER(STAT_PunAIUpdateRelationship);

	if (!_simulation->IsAITown(_townId)) {
		return;
	}

	std::vector<int32> allPlayersAndAI = _simulation->GetAllPlayersAndAI();
	for (int32 playerId = 0; playerId < allPlayersAndAI.size(); playerId++)
	{
		if (_simulation->HasTownhall(playerId))
		{
#define MODIFIER(EnumName) _relationships.GetModifierMutable(playerId, RelationshipModifierEnum::EnumName)

			// Decaying modifiers (decay every season)
			if (Time::Ticks() % Time::TicksPerSeason == 0)
			{
				_relationships.DecayModifier(playerId, RelationshipModifierEnum::YouGaveUsGifts);
				_relationships.DecayModifier(playerId, RelationshipModifierEnum::YouStealFromUs);
				_relationships.DecayModifier(playerId, RelationshipModifierEnum::YouKidnapFromUs);
				_relationships.DecayModifier(playerId, RelationshipModifierEnum::GoodTradeDeal);
				_relationships.DecayModifier(playerId, RelationshipModifierEnum::DiplomaticBuildings);
			}

			//if (Time::Ticks() % Time::TicksPerMinute == 0)
			{
				// Calculated modifiers
				auto calculateStrength = [&](int32 townIdScope) {
					if (IsMajorTown(townIdScope)) {
						int32 playerIdScope = _simulation->townPlayerId(townIdScope);
						return _simulation->influence(playerIdScope) + _simulation->playerOwned(playerIdScope).totalInfluenceIncome100() * Time::RoundsPerYear;
					}
					return 1000;
				};
				int32 aiStrength = calculateStrength(_townId);
				int32 counterPartyStrength = calculateStrength(playerId);

				_relationships.SetModifier(playerId, RelationshipModifierEnum::YouAreStrong, Clamp((counterPartyStrength - aiStrength) / 50, 0, 20));
				_relationships.SetModifier(playerId, RelationshipModifierEnum::YouAreWeak, Clamp((aiStrength - counterPartyStrength) / 50, 0, 20));
				//MODIFIER(YouAreStrong) = Clamp((counterPartyStrength - aiStrength) / 50, 0, 20);
				//MODIFIER(YouAreWeak) = Clamp((aiStrength - counterPartyStrength) / 50, 0, 20);

				if (IsMajorTown(_townId))
				{
					int32 aiPlayerId = _simulation->townPlayerId(_townId);
					const std::vector<int32>& provinceIds = _simulation->GetProvincesPlayer(aiPlayerId);
					int32 borderCount = 0;
					for (int32 provinceId : provinceIds) {
						const std::vector<ProvinceConnection>& connections = _simulation->GetProvinceConnections(provinceId);
						for (const ProvinceConnection& connection : connections) {
							if (_simulation->provinceOwnerPlayer(connection.provinceId) == playerId) {
								borderCount++;
							}
						}
					}
					_relationships.SetModifier(playerId, RelationshipModifierEnum::AdjacentBordersSparkTensions, std::max(-borderCount * 5, -20));
					//MODIFIER(AdjacentBordersSparkTensions) = std::max(-borderCount * 5, -20);

					// townhall nearer 500 tiles will cause tensions
					int32 townhallDistance = WorldTile2::Distance(_simulation->GetTownhallGateCapital(aiPlayerId), _simulation->GetTownhallGateCapital(playerId));
					if (townhallDistance <= 500) {
						_relationships.SetModifier(playerId, RelationshipModifierEnum::TownhallProximitySparkTensions, -20 * (500 - townhallDistance) / 500);
						//MODIFIER(TownhallProximitySparkTensions) = -20 * (500 - townhallDistance) / 500;
					}
					else {
						_relationships.SetModifier(playerId, RelationshipModifierEnum::TownhallProximitySparkTensions, 0);
						//MODIFIER(TownhallProximitySparkTensions) = 0;
					}
				}
			}


			/*
			 * Check alliance state
			 */
			if (_relationships.isAlly(playerId)) {
				if (!_relationships.CanCreateAlliance(playerId)) {
					_relationships.SetAlliance(playerId, false);
				}
			}


			/*
			 * Relationship reward
			 */
			auto addInfluenceReward = [&](int32 influencePerRound)
			{
				//! Income every X secs
				if (Time::Seconds() % GameConstants::BaseFloatupIntervalSec == 0)
				{
					int32 incomePerXSec = GameRand::RandRound(influencePerRound * GameConstants::BaseFloatupIntervalSec, Time::SecondsPerRound);
					if (incomePerXSec > 0) {
						_simulation->uiInterface()->ShowFloatupInfo(playerId, FloatupEnum::GainInfluence, _simulation->building(townhallId).centerTile(), TEXT_NUMSIGNED(incomePerXSec));
						_simulation->ChangeInfluence(playerId, incomePerXSec);
					}
				}
			};
			
			int32 totalRelationship = _relationships.GetTotalRelationship(playerId);
			if (_relationships.isAlly(playerId)) {
				addInfluenceReward(150);
			}
			else if (totalRelationship >= 60) {
				addInfluenceReward(60);
			}
			else if (totalRelationship >= 30) {
				addInfluenceReward(30);
			}
			
#undef MODIFIER
		}
	}
}

void TownManagerBase::ProposeAlliance(int32 askingPlayerId)
{
	if (_relationships.CanCreateAlliance(askingPlayerId))
	{
		// Cancel old alliances if MinorTown
		if (IsMinorTown(_townId))
		{
			for (int32 i = 0; i < GameConstants::MaxPlayersAndAI; i++) {
				if (_relationships.isAlly(i)) {
					_relationships.SetAlliance(i, false);

					_simulation->AddPopup(i, FText::Format(
						NSLOCTEXT("TownManagerBase", "MinorTownChangeAlly_Popup", "{0} now considers {1} their main ally instead of you.<space>This is due to their improved relationship with {1}."),
						_simulation->townNameT(_townId),
						_simulation->playerNameT(askingPlayerId)
					));
				}
			}
		}

		_relationships.SetAlliance(askingPlayerId, true);
	}
}