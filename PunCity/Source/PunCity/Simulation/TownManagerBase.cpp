// Pun Dumnernchanvanit's


#include "TownManagerBase.h"
#include "BuildingCardSystem.h"

void ProvinceClaimProgress::Reinforce(std::vector<CardStatus>& militaryCards, bool isReinforcingAttacker, int32 unitPlayerId)
{
	PrepareCardStatusForMilitary(unitPlayerId, militaryCards);

	auto& frontLine = isReinforcingAttacker ? attackerFrontLine : defenderFrontLine;
	auto& backLine = isReinforcingAttacker ? attackerBackLine : defenderBackLine;

	for (const CardStatus& card : militaryCards)
	{
		check(IsLandMilitaryCardEnum(card.cardEnum));
		if (IsFrontlineCardEnum(card.cardEnum)) {
			frontLine.push_back(card);
		}
		else {
			backLine.push_back(card);
		}
	}
}

void ProvinceClaimProgress::Retreat(int32 unitPlayerId)
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

	auto doDamage = [&](std::vector<CardStatus>& lineCards, std::vector<CardStatus>& damageTakerLine)
	{
		int32 randomIndex = GameRand::Rand() % damageTakerLine.size();
		CardStatus& damageTaker = damageTakerLine[randomIndex];

		int32 attack = 0;
		for (CardStatus& card : lineCards) {
			attack += GetMilitaryInfo(card.cardEnum).attack100 * card.stackSize;
		}

		MilitaryCardInfo damageTakerInfo = GetMilitaryInfo(damageTaker.cardEnum);
		int32 damage = attack * 100 / (100 + damageTakerInfo.defense100);
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

	auto doDamageToArmy = [&](std::vector<CardStatus>& attackerLine, std::vector<CardStatus>& damageTakerFrontLine, std::vector<CardStatus>& damageTakerBackLine)
	{
		if (damageTakerFrontLine.size() > 0) {
			doDamage(attackerLine, damageTakerFrontLine);
		}
		else if (damageTakerBackLine.size() > 0) {
			doDamage(attackerLine, damageTakerBackLine);
		}
	};

	//! Defender Turn
	if (Time::Seconds() % 8 == 0)
	{
		doDamageToArmy(defenderBackLine, attackerFrontLine, attackerBackLine);
	}
	else if (Time::Seconds() % 8 == 1)
	{
		doDamageToArmy(defenderFrontLine, attackerFrontLine, attackerBackLine);
	}
	//! Attacker Turn
	else if (Time::Seconds() % 8 == 4)
	{
		doDamageToArmy(attackerBackLine, defenderFrontLine, defenderBackLine);
	}
	else if (Time::Seconds() % 8 == 5)
	{
		doDamageToArmy(attackerFrontLine, defenderFrontLine, defenderBackLine);
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
		//! Income every 2 secs
		if (Time::Seconds() % 2 == 0)
		{
			int32 incomePerRound = GetMinorCityMoneyIncome();
			int32 incomePer2Sec = GameRand::RandRound(incomePerRound * 2, Time::TicksPerSecond);
			
			_minorCityWealth += incomePer2Sec;
		}

		//! Refresh Target Wealth every 30 secs
		if (Time::Seconds() % 30 == 0)
		{
			RefreshMinorCityTargetWealth();
		}
	}
}

void TownManagerBase::ReturnMilitaryUnitCards(std::vector<CardStatus>& cards, int32 playerId, bool forcedAll)
{
	for (CardStatus& card : cards) {
		if (forcedAll || playerId == card.cardStateValue1) {
			// TODO:
			//_simulation->cardSystem(playerId).AddCards_BoughtHandAndInventory(card);
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
			// TODO: ARAB FIX
			//maxTownRevenue = std::max(maxTownRevenue, _simulation->townManager(townId).totalRevenue100() / 100);
		}
	});

	_minorCityTargetWealth = maxTownRevenue;
}

void TownManagerBase::AddChildBuilding(MinorCityChild& child)
{
	// TODO: ARAB FIX
	//_childBuildingIds.push_back(child.buildingId());
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

	const int32 feePercent = 40; // TODO: allow people to manipulate this by building more Trading Company etc.


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