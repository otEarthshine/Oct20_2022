// Pun Dumnernchanvanit's


#include "TownManagerBase.h"
#include "BuildingCardSystem.h"
#include "PlayerOwnedManager.h"
#include "Buildings/GathererHut.h"

void ProvinceClaimProgress::Reinforce(std::vector<CardStatus>& militaryCards, bool isReinforcingAttacker, int32 unitPlayerId)
{
	PrepareCardStatusForMilitary(unitPlayerId, militaryCards);

	auto& frontLine = isReinforcingAttacker ? attackerFrontLine : defenderFrontLine;
	auto& backLine = isReinforcingAttacker ? attackerBackLine : defenderBackLine;

	for (const CardStatus& card : militaryCards)
	{
		check(IsMilitaryCardEnum(card.cardEnum));
		if (IsFrontlineCardEnum(card.cardEnum)) {
			AddMilitaryCards(frontLine, card);
		} else {
			AddMilitaryCards(backLine, card);
		}
	}
}

void ProvinceClaimProgress::Retreat_DeleteUnits(int32 unitPlayerId)
{
	auto deleteUnits = [&](std::vector<CardStatus>& line) {
		for (int32 i = line.size(); i-- > 0;) {
			if (line[i].cardStateValue1 == unitPlayerId) {
				line.erase(line.begin() + i);
			}
		}
	};
	deleteUnits(attackerFrontLine);
	deleteUnits(attackerBackLine);
	
	deleteUnits(defenderFrontLine);
	deleteUnits(defenderBackLine);
	deleteUnits(defenderWall);
	deleteUnits(defenderTreasure);
	
}


void ProvinceClaimProgress::Tick(IGameSimulationCore* simulation)
{
	/*
	 * cardStateValue1 = playerId
	 * cardStateValue2 = HP
	 * displayCardStateValue1 = last damage
	 * displayCardStateValue2 = last damage tick
	 */

	auto doDamage = [&](CardStatus& attackerCard, std::vector<CardStatus>& damageTakerLine, int32 attackBonusPercent, int32 defenseBonusPercent)
	{
		int32 randomIndex = GameRand::Rand() % damageTakerLine.size();
		CardStatus& damageTaker = damageTakerLine[randomIndex];

		int32 attack = GetMilitaryInfo(attackerCard.cardEnum).attack100 * attackerCard.stackSize;
		attackerCard.displayCardStateValue3 = Time::Ticks();

		// Damage Bonus
		attack = attack * (100 + attackBonusPercent) / 100;

		MilitaryCardInfo damageTakerInfo = GetMilitaryInfo(damageTaker.cardEnum);

		int32 defense100 = damageTakerInfo.defense100 * (100 + defenseBonusPercent) / 100;

		// Artillery ignores wall defense
		if (IsArtilleryMilitaryCardEnum(attackerCard.cardEnum)) {
			defense100 = MilitaryConstants::BaseDefense100;
		}
		
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

	auto doDamageToArmy = [&](CardStatus& attackerCard, std::vector<std::vector<CardStatus>*> damageTakerLines, int32 attackBonus, int32 defenseBonus)
	{
		for (int32 i = 0; i < damageTakerLines.size(); i++)
		{
			if (damageTakerLines[i]->size() > 0) {
				doDamage(attackerCard, *damageTakerLines[i], attackBonus, defenseBonus);
				break;
			}
		}
	};

	//! Defender Turn
	int32 ticksPerAttackRound = MilitaryConstants::SecondsPerAttack * Time::TicksPerSecond;
	int32 tickSinceAttackRoundStart = Time::Ticks() % ticksPerAttackRound;

	auto tickAttackRound = [&](int32 ticksShift, std::vector<CardStatus>& backLine, std::vector<CardStatus>& frontLine, std::vector<std::vector<CardStatus>*> damageTakerLines, int32 attackBonus, int32 defenseBonus)
	{
		int32 availableAttackTicks = Time::TicksPerSecond * 3;
		int32 ticksBetweenAttacks = std::max(1, availableAttackTicks / std::max(1, static_cast<int32>(backLine.size() + frontLine.size())));
		
		for (int32 i = 0; i < backLine.size(); i++) {
			if (tickSinceAttackRoundStart == ticksShift + i * ticksBetweenAttacks) {
				doDamageToArmy(backLine[i], damageTakerLines, attackBonus, defenseBonus);
			}
		}
		for (int32 i = 0; i < frontLine.size(); i++) {
			if (tickSinceAttackRoundStart == ticksShift + (i + backLine.size()) * ticksBetweenAttacks) {
				doDamageToArmy(frontLine[i], damageTakerLines, attackBonus, defenseBonus);
			}
		}
	};
	

	// First Half
	if (tickSinceAttackRoundStart < ticksPerAttackRound / 2)
	{
		tickAttackRound(0, defenderBackLine, defenderFrontLine, { &attackerFrontLine, &attackerBackLine }, defender_attackBonus, attacker_defenseBonus);
	}
	// Second Half
	else
	{
		tickAttackRound(ticksPerAttackRound / 2, attackerBackLine, attackerFrontLine, { &defenderWall, &defenderFrontLine, &defenderBackLine, &defenderTreasure }, attacker_attackBonus, defender_defenseBonus);
	}
	
	//if (Time::Ticks() % ticksPerAttack == 0)
	//{
	//	doDamageToArmy(defenderBackLine, { &attackerFrontLine, &attackerBackLine }, defender_attackBonus, attacker_defenseBonus);
	//}
	//else if (Time::Ticks() % ticksPerAttack == 1)
	//{
	//	doDamageToArmy(defenderFrontLine, { &attackerFrontLine, &attackerBackLine }, defender_attackBonus, attacker_defenseBonus);
	//}
	////! Attacker Turn
	//else if (Time::Ticks() % ticksPerAttack == 4)
	//{
	//	doDamageToArmy(attackerBackLine, { &defenderWall, &defenderFrontLine, &defenderBackLine, &defenderTreasure }, attacker_attackBonus, defender_defenseBonus);
	//}
	//else if (Time::Ticks() % ticksPerAttack == 5)
	//{
	//	doDamageToArmy(attackerFrontLine, { &defenderWall, &defenderFrontLine, &defenderBackLine, &defenderTreasure }, attacker_attackBonus, defender_defenseBonus);
	//}

	//! Battle Countdown once it is done
	battleFinishCountdownTicks = std::max(0, battleFinishCountdownTicks - 1);
}

/*
 * TownManagerBase
 */

void TownManagerBase::Tick1Sec_TownBase()
{

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

		/*
		 * Tourism
		 */
		if (Time::Seconds() % GameConstants::TourismCheckIntervalSec == 0)
		{
			std::vector<TradeRoutePair> tradeRoutes = _simulation->worldTradeSystem().GetTradeRoutesTo(_townId);

			// Get Hotels on Trade Route
			std::vector<Hotel*> hotels = GetConnectedHotels();

			if (hotels.size() > 0)
			{
				// Sort Hotels by Service Quality
				std::sort(hotels.begin(), hotels.end(), [&](Hotel* a, Hotel* b) {
					return a->serviceQuality() > b->serviceQuality();
				});

				// 1 pop tour 1 time per rounds 
				// TickXSec so:
				// clusterSize = population * GameConstants::TourismCheckIntervalSec / (seconds per round)

				int32 fakePopulation = GetTourismPopulation();
				const int32 clusterSize = GameRand::RandRound(fakePopulation * GameConstants::TourismCheckIntervalSec, Time::SecondsPerRound);
				int32 hotelIndex = 0;

				for (int32 i = 0; i < clusterSize; i++)
				{
					if (hotelIndex >= hotels.size()) {
						break;
					}
					Hotel* hotel = hotels[hotelIndex++];
					hotel->Visit(_townId);

					ChangeWealth_MinorCity(hotel->feePerVisitor());
				}

			}
		}

		
	}


	Tick1SecRelationship();
}


void TownManagerBase::StartAttack_Defender(int32 attackerPlayerId, int32 provinceId, ProvinceAttackEnum provinceAttackEnum, std::vector<CardStatus>& initialMilitaryCards)
{
	ProvinceClaimProgress claimProgress;
	claimProgress.attackEnum = provinceAttackEnum;
	claimProgress.provinceId = provinceId;
	claimProgress.attackerPlayerId = attackerPlayerId;
	claimProgress.defenderTownId = _townId;
	claimProgress.battleFinishCountdownTicks = 0;


	//! Fill Attacker Military Units
	claimProgress.Reinforce(initialMilitaryCards, true, attackerPlayerId);


	//! Fill Defender Military Units
	std::vector<CardStatus> defenderCards;
	auto addWall = [&](int32 hpAmount) {
		CardStatus wall(CardEnum::Wall, 1);
		wall.cardStateValue1 = defenderPlayerId();
		wall.cardStateValue2 = hpAmount;
		wall.cardStateValue3 = hpAmount;
		claimProgress.defenderWall = { wall };
	};

	if (provinceAttackEnum == ProvinceAttackEnum::Raze) {
		defenderCards = { CardStatus(CardEnum::Militia, baseDefenderUnits() * 3 / 2) }; // More resistance if the intent is to raze...
	}
	else if (provinceAttackEnum == ProvinceAttackEnum::RaidBattle) {
		claimProgress.raidMoney = _simulation->GetProvinceRaidMoney100(provinceId) / 100;
		defenderCards = { CardStatus(CardEnum::Militia, 1) };
	}
	else if (provinceAttackEnum == ProvinceAttackEnum::DeclareIndependence) {
		claimProgress.battleFinishCountdownTicks = Time::TicksPerRound;
	}
	else {
		defenderCards = { CardStatus(CardEnum::Militia, baseDefenderUnits()) }; // Town Population help is this is not a raid
	}


	//! Fill Defender Wall
	int32 majorTownId = _simulation->provinceOwnerTown_Major(provinceId);
	if (majorTownId != -1)
	{
		// Vassalize is hard
		if (provinceAttackEnum == ProvinceAttackEnum::Vassalize)
		{
			addWall(Fort::GetFortHP100(IsMinorTown(_townId) ?  1 : _simulation->GetTownLvl(_townId)));
		}
		else {
			const std::vector<int32>& bldIds = _simulation->buildingIds(majorTownId, CardEnum::Fort);
			for (int32 bldId : bldIds) {
				Fort& fort = _simulation->building(bldId).subclass<Fort>();
				if (fort.provinceId() == provinceId) {
					addWall(fort.GetFortHP100());
					break;
				}
			}
		}
	}

	
	//! Reinforce defenders
	claimProgress.Reinforce(defenderCards, false, defenderPlayerId());


	//! Fill Defender Raid Treasure
	auto addRaidTreasure = [&](int32 moneyAmount)
	{
		CardStatus raidTreasure(CardEnum::RaidTreasure, 1);
		raidTreasure.cardStateValue1 = defenderPlayerId();
		raidTreasure.cardStateValue2 = moneyAmount * 100;
		raidTreasure.cardStateValue3 = moneyAmount * 100;
		claimProgress.defenderTreasure = { raidTreasure };
	};

	if (claimProgress.attackEnum == ProvinceAttackEnum::RaidBattle)
	{
		int32 raidMoney = _simulation->GetProvinceRaidMoney100(provinceId) / 100;
		raidMoney = std::max(500, raidMoney);

		addRaidTreasure(raidMoney);
	}
	else if (claimProgress.attackEnum == ProvinceAttackEnum::ConquerProvince)
	{
		int32 claimMoney = 3 * _simulation->GetProvinceClaimPrice(provinceId, attackerPlayerId) / 100;

		addRaidTreasure(claimMoney);
	}

	//! Fill Attacker/Defender Bonus
	auto getBonus = [&](std::vector<BonusPair> bonuses)
	{
		int32 value = 0;
		for (const BonusPair& bonus : bonuses) {
			value += bonus.value;
		}
		return value;
	};

	claimProgress.attacker_attackBonus = getBonus(_simulation->GetAttackBonuses(provinceId, attackerPlayerId));
	claimProgress.attacker_defenseBonus = getBonus(_simulation->GetDefenseBonuses(provinceId, attackerPlayerId));
	claimProgress.defender_attackBonus = getBonus(_simulation->GetAttackBonuses(provinceId, claimProgress.defenderTownId));
	claimProgress.defender_defenseBonus = getBonus(_simulation->GetDefenseBonuses(provinceId, claimProgress.defenderTownId));

	_defendingClaimProgress.push_back(claimProgress);
}

void TownManagerBase::ReturnMilitaryUnitCards(std::vector<CardStatus>& cards, int32 playerIdToReturn, bool forcedAll, bool isRetreating)
{
	check(_simulation->IsValidPlayer(playerIdToReturn));
	for (CardStatus card : cards) 
	{
		if (card.cardEnum == CardEnum::Militia) {
			continue;
		}
		
		if (forcedAll || playerIdToReturn == card.cardStateValue1) 
		{	
			// Retreating death
			if (isRetreating && !IsNavyCardEnum(card.cardEnum)) {
				card.stackSize = GameRand::RandRound(card.stackSize * 2, 3);
			}

			_simulation->cardSystem(card.cardStateValue1).TryAddCards_BoughtHandAndInventory(card);
		}
	}
}

//void TownManagerBase::RefreshMinorCityTargetWealth()
//{
//	const int32 maxTileDistanceToCheck = 300;
//	int32 maxTownRevenue = 0;
//	WorldTile2 thisTownGateTile = _simulation->GetTownhallGate_All(_townId);
//	
//	_simulation->ExecuteOnMajorTowns([&](int32 townId) 
//	{
//		WorldTile2 gateTile = _simulation->GetMajorTownhallGate(townId);
//		if (gateTile.isValid() &&
//			WorldTile2::Distance(gateTile, thisTownGateTile) < maxTileDistanceToCheck)
//		{
//			maxTownRevenue = std::max(maxTownRevenue, _simulation->GetMajorTownTotalRevenue100(townId) / 100);
//		}
//	});
//
//	_minorCityTargetWealth = maxTownRevenue;
//}

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

			addInfluenceReward(GetMinorCityAllyInfluenceReward(playerId));
			
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


const std::vector<Hotel*> TownManagerBase::GetConnectedHotels()
{
	std::vector<TradeRoutePair> tradeRoutes = _simulation->worldTradeSystem().GetTradeRoutesTo(_townId);

	// Get Hotels on Trade Route
	std::vector<Hotel*> hotels;
	for (const TradeRoutePair& route : tradeRoutes) {
		int32 destinationTownId = route.GetCounterpartTownId(_townId);
		const std::vector<int32>& localHotelIds = _simulation->buildingIds(destinationTownId, CardEnum::Hotel);
		for (int32 localHotelId : localHotelIds) {
			Hotel* hotel = static_cast<Hotel*>(_simulation->buildingPtr(localHotelId));
			if (hotel->isAvailable() &&
				hotel->serviceQuality() >= 50)
			{
				hotels.push_back(hotel);
			}
		}
	}

	return hotels;
}