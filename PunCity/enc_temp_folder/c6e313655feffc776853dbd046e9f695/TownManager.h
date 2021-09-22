// Pun Dumnernchanvanit's

#pragma once

#include "IGameSimulationCore.h"
#include "PlayerParameters.h"
#include "TreeSystem.h"
#include "PunCity/NetworkStructs.h"
#include "PunCity/DisplaySystem/PunTerrainGenerator.h"
#include <algorithm>
#include "PunCity/CppUtils.h"
#include <numeric>

#include "Building.h"
#include "ProvinceSystem.h"
#include "ProvinceInfoSystem.h"
#include "WorldTradeSystem.h"


using namespace std;

static const int32 TownSizeMinPopulation[]
{
	0,
	25,
	50,
	100,
	150,
	200,
};
static int32 GetTownSizeMinPopulation(int32 tier) { return TownSizeMinPopulation[tier]; }


// Priority list for what job need people living closer
static const std::vector<CardEnum> DefaultJobPriorityListAllSeason
{
	CardEnum::IntercityLogisticsHub,
	CardEnum::IntercityLogisticsPort,
	
	CardEnum::FruitGatherer,
	CardEnum::Farm,

	CardEnum::MushroomFarm,
	CardEnum::Fisher,
	CardEnum::HuntingLodge,

	CardEnum::RanchCow,
	CardEnum::RanchSheep,
	CardEnum::RanchPig,

	CardEnum::HaulingServices,

	CardEnum::Windmill,
	CardEnum::Bakery,

	CardEnum::StoneToolShopOld,
	CardEnum::Blacksmith,
	CardEnum::Herbalist,
	CardEnum::MedicineMaker,

	CardEnum::Forester,
	CardEnum::CharcoalMaker,
	CardEnum::BeerBrewery,
	CardEnum::ClayPit,
	CardEnum::Potter,
	CardEnum::Tailor,

	CardEnum::MagicMushroomFarm,
	CardEnum::VodkaDistillery,
	CardEnum::CoffeeRoaster,

	CardEnum::GoldMine,
	CardEnum::Quarry,
	CardEnum::CoalMine,
	CardEnum::IronMine,

	CardEnum::IndustrialIronSmelter,
	CardEnum::IronSmelter,
	CardEnum::GoldSmelter,
	CardEnum::Mint,
	CardEnum::ResearchLab,

	CardEnum::FurnitureWorkshop,
	CardEnum::Chocolatier,
	CardEnum::Winery,

	CardEnum::GemstoneMine,
	CardEnum::Jeweler,

	CardEnum::Beekeeper,
	CardEnum::Brickworks,
	CardEnum::CandleMaker,
	CardEnum::CottonMill,
	CardEnum::GarmentFactory,
	CardEnum::PrintingPress,

	CardEnum::PaperMaker,


	CardEnum::SandMine,
	CardEnum::GlassSmelter,
	CardEnum::Glassworks,
	CardEnum::ConcreteFactory,
	CardEnum::CoalPowerPlant,
	CardEnum::Steelworks,
	CardEnum::StoneToolsShop,
	CardEnum::OilRig,
	CardEnum::OilPowerPlant,
	CardEnum::PaperMill,
	CardEnum::ClockMakers,


	CardEnum::Market,
	CardEnum::ShippingDepot,

	CardEnum::CardMaker,
	CardEnum::ImmigrationOffice,

	CardEnum::BarrackSwordman,
	CardEnum::BarrackArcher,

	CardEnum::RegionShrine,
};


struct ProvinceClaimProgress
{
	int32 provinceId = -1;
	int32 attackerPlayerId = -1;

	int32 ticksElapsed = 0; // ticks... The longer the seige, the more strength for defender...

	std::vector<CardStatus> attackerFrontLine; 
	std::vector<CardStatus> attackerBackLine;
	
	std::vector<CardStatus> defenderFrontLine;
	std::vector<CardStatus> defenderBackLine;

	bool isCrossingRiver = false;

	int32 attackerDefenseBonus = 0;
	int32 defenderDefenseBonus = 0;

	bool attackerWon() const {
		return defenderFrontLine.size() == 0 && defenderBackLine.size() == 0;
	}
	bool attackerLost() const {
		return attackerFrontLine.size() == 0 && attackerBackLine.size() == 0;
	}

	bool isValid() { return provinceId != -1; }


	static void PrepareCardStatusForMilitary(int32 playerId, std::vector<CardStatus>& militaryCards)
	{
		// cardStateValue1 = playerId, cardStateValue2 = HP
		for (CardStatus& cardStatus : militaryCards) {
			cardStatus.cardStateValue1 = playerId;
			cardStatus.cardStateValue2 = GetMilitaryInfo(cardStatus.cardEnum).hp100;
		}
	}

	void Reinforce(std::vector<CardStatus>& militaryCards, bool isReinforcingAttacker, int32 unitPlayerId)
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
	

	void Tick1Sec(IGameSimulationCore* simulation)
	{
		/*
		 * cardStateValue1 = playerId
		 * cardStateValue2 = HP
		 * displayCardStateValue1 = last damage
		 * displayCardStateValue2 = last damage tick
		 */

		// TODO: Fight
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
	}


	//! Serialize
	FArchive& operator>>(FArchive &Ar)
	{
		Ar << provinceId;
		Ar << attackerPlayerId;
		Ar << ticksElapsed;

		SerializeVecObj(Ar, attackerFrontLine);
		SerializeVecObj(Ar, attackerBackLine);
		SerializeVecObj(Ar, defenderFrontLine);
		SerializeVecObj(Ar, defenderBackLine);

		Ar << isCrossingRiver;
		Ar << attackerDefenseBonus;
		Ar << defenderDefenseBonus;
		
		return Ar;
	}
};

struct AutoTradeElement
{
	bool isImport = true;
	ResourceEnum resourceEnum = ResourceEnum::None;
	int32 targetInventory = 0;
	int32 maxTradeAmount = 0;
	
	int32 calculatedTradeAmountNextRound = 0;
	int32 calculatedFulfilledTradeAmountNextRound = 0;

	int32 price100 = 0;
	int32 directTradeAmount = 0;
	int32 feePercent = 0;

	int32 calculatedMoney = 0;
	int32 calculatedFeeDiscount = 0;
	int32 calculatedFee = 0;

	int32 calculatedFulfillmentLeft() const {
		return calculatedTradeAmountNextRound - calculatedFulfilledTradeAmountNextRound;
	}

	// Import may not complete successfully due to not having enough storage space
	int32 GetImportLeftoverRefund100(int32 leftoverAmount) const
	{
		check(leftoverAmount >= 0);
		int32 previousNetMoney = calculatedMoney + calculatedFee;

		int32 newTradeAmount = calculatedTradeAmountNextRound - leftoverAmount;
		int32 newImportMoney = newTradeAmount * price100;
		int32 newFeeDiscount = std::min(directTradeAmount, newTradeAmount) * price100 * feePercent / 100;
		int32 newFee = (newImportMoney * feePercent / 100) - newFeeDiscount;
		
		int32 newNetMoney = newImportMoney * newFee;
		
		return previousNetMoney - newNetMoney;
	}

	void CalculateMoney(int32 price100In, int32 directTradeAmountIn, int32 feePercentIn)
	{
		price100 = price100In;
		directTradeAmount = directTradeAmountIn;
		feePercent = feePercentIn;
		
		calculatedMoney = calculatedTradeAmountNextRound * price100;

		calculatedFeeDiscount = directTradeAmount * price100 * feePercent / 100;
		calculatedFee = (calculatedMoney * feePercent / 100) - calculatedFeeDiscount;
	}
	

	//! Serialize
	FArchive& operator>>(FArchive &Ar) {
		Ar << isImport;
		Ar << resourceEnum;
		Ar << targetInventory;
		Ar << maxTradeAmount;
		
		Ar << calculatedTradeAmountNextRound;
		Ar << calculatedFulfilledTradeAmountNextRound;

		Ar << price100;
		Ar << directTradeAmount;
		Ar << feePercent;

		Ar << calculatedMoney;
		Ar << calculatedFeeDiscount;
		Ar << calculatedFee;
		return Ar;
	}

	int32 GetHash() const
	{
		return FastHashCombineMany({
			isImport,
			static_cast<int32>(resourceEnum),
			targetInventory,
			maxTradeAmount,
			calculatedTradeAmountNextRound,
			calculatedFulfilledTradeAmountNextRound,
		});
	}
};

/*
 * 
 */
class TownManagerBase
{
public:
	int32 _playerId = -1;
	int32 _townId = -1;
	int32 townHallId = -1;

	static const int32 BaseAutoTradeFeePercent = 40;

	bool isValid() { return _townId != -1; }

	TownManagerBase(int32 playerId, int32 townId, IGameSimulationCore* simulation) :
		_playerId(playerId),
		_townId(townId),
		_simulation(simulation)
	{

	}

	virtual ~TownManagerBase() {}

	RelationshipModifiers& minorTownRelationship() { return _minorTownRelationshipModifiers; }

	/*
	 * Auto Trade
	 */

	std::vector<AutoTradeElement>& autoExportElements() { return _autoExportElements; }
	std::vector<AutoTradeElement>& autoImportElements() { return _autoImportElements; }

	const std::vector<AutoTradeElement>& autoExportElementsConst() { return _autoExportElements; }
	const std::vector<AutoTradeElement>& autoImportElementsConst() { return _autoImportElements; }

	void InitAutoTrade(int32 provinceId)
	{
		GeoresourceEnum georesourceEnum = _simulation->georesource(provinceId).georesourceEnum;

		AutoTradeElement element;
		int32 tradeAmount = 100;
		element.targetInventory = tradeAmount;
		element.maxTradeAmount = tradeAmount;
		element.calculatedTradeAmountNextRound = tradeAmount;

		// Usually export food, fuel, raw material
		{
			ResourceEnum chosenResourceEnum;
			
			if (georesourceEnum != GeoresourceEnum::None) {
				chosenResourceEnum = GeoresourceEnumToResourceEnum(georesourceEnum);
			}
			else {
				std::vector<ResourceEnum> exportEnums = FoodEnums_Input;
				exportEnums.push_back(ResourceEnum::Coal);
				exportEnums.push_back(ResourceEnum::Wood);
				exportEnums.push_back(ResourceEnum::Stone);
				exportEnums.push_back(ResourceEnum::Clay);
				chosenResourceEnum = exportEnums[GameRand::Rand() % exportEnums.size()];
			}

			element.isImport = false;
			element.resourceEnum = chosenResourceEnum;

			_autoExportElements.push_back(element);
		}

		

		// Usually import processed luxury
		{
			std::vector<ResourceEnum> importEnums = {
				ResourceEnum::Beer,
				ResourceEnum::Furniture,
				ResourceEnum::Pottery,
			};

			element.isImport = true;
			element.resourceEnum = importEnums[GameRand::Rand() % importEnums.size()];
			
			_autoImportElements.push_back(element);
		}
	}

	int32 lastRoundAutoTradeProfit() { return _lastRoundAutoTradeProfit; }
	void SetLastRoundAutoTradeProfit(int32 lastRoundAutoTradeProfit) {
		_lastRoundAutoTradeProfit = lastRoundAutoTradeProfit;
	}

	void CalculateAutoTradeProfit(
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
	
	
	/*
	 * Claim Province
	 */
	void TryRemoveProvinceClaim(int32 provinceId, bool lightMode)
	{
		CppUtils::TryRemove(_provincesClaimed, provinceId);
		//_claimedProvinceConnected.erase(provinceId);

		if (!lightMode) {
			RecalculateTax(false);
		}
	}

	void ClaimProvince(int32 provinceIdIn, bool lightMode = false)
	{
		//PUN_LOG("ClaimProvince province:%d pid:%d", provinceId, _playerId);

		_provincesClaimed.push_back(provinceIdIn);

		RefreshProvinceInfo();


		if (lightMode) {
			return;
		}

		RecalculateTax(false);
	}

	/*
	 * Helpers
	 */
	virtual void RefreshProvinceInfo() {}

	virtual void RecalculateTax(bool showFloatup) {}

	/*
	 * Serialize
	 */
	void Serialize(FArchive& Ar)
	{
		//! Public
		Ar << _playerId;
		Ar << _townId;
		Ar << townHallId;

		SerializeVecValue(Ar, _provincesClaimed);

		SerializeVecValue(Ar, _autoExportElements);
		SerializeVecValue(Ar, _autoImportElements);

		Ar << _lastRoundAutoTradeProfit;

		Ar << _minorTownRelationshipModifiers;
	}

protected:
	std::vector<int32> _provincesClaimed;

	// Auto Trade
	std::vector<AutoTradeElement> _autoExportElements;
	std::vector<AutoTradeElement> _autoImportElements;

	int32 _lastRoundAutoTradeProfit = 0;

	// Relationship
	RelationshipModifiers _minorTownRelationshipModifiers;

	/*
	 * Non-Serialize
	 */
	IGameSimulationCore* _simulation = nullptr;
};


/**
 * 
 */
class TownManager : public TownManagerBase
{
public:
	TownManager(int32 playerId, int32 townId, IGameSimulationCore* simulation) :
		TownManagerBase(playerId, townId, simulation)
	{
		//_playerId = playerId;
		//_townId = townId;
		PUN_LOG("Create TownManager pid:%d tid:%d", playerId, townId);
		
		//_simulation = simulation;
		
		taxLevel = 2;

		_aveHappiness.resize(HappinessEnumName.Num());
		//_aveHappinessModifiers.resize(HappinessModifierName.Num());
		_townFoodVariety = 1;

		incomes100.resize(IncomeEnumCount);
		influenceIncomes100.resize(InfluenceIncomeEnumCount);
		sciences100.resize(ScienceEnumCount);

		_enumToStatVec.resize(static_cast<int>(PlotStatEnum::Count));

		_nextDiseaseCheckTick = 0;

		_houseResourceAllowed.resize(static_cast<int>(ResourceEnumCount), true);

		_resourceEnumToOutputTarget.resize(static_cast<int>(ResourceEnumCount), -1);
		_resourceEnumToOutputTargetDisplay.resize(static_cast<int>(ResourceEnumCount), -1);

		_jobPriorityList = DefaultJobPriorityListAllSeason;

		_lastResourceCounts.resize(ResourceEnumCount);

		_jobBuildingEnumToIds.resize(BuildingEnumCount);

		_tourismIncreaseIndex = 0;
		_tourismDecreaseIndex = 0;
	}

	
	void ChangeTownOwningPlayer(int32 newPlayerId);

	/*
	 * Tick
	 */
	void TickRound();

	void Tick1Sec();

	void Tick();

	/*
	 * Get Variables
	 */
	int32 playerId() { return _playerId; }

	bool hasTownhall() { return townHallId != -1; }

	bool isCapital() { return _townId == _playerId; }
	
	const std::vector<int32>& provincesClaimed() { return _provincesClaimed; }
	

	int32 aveHappinessByType(HappinessEnum happinessEnum) {
		return _aveHappiness[static_cast<int>(happinessEnum)];
	}
	int32 aveOverallHappiness()
	{
		int32 sum = 0;
		for (size_t i = 0; i < _aveHappiness.size(); i++) {
			sum += _aveHappiness[i];
		}
		return sum / _aveHappiness.size();
	}

	int32 townFoodVariety() {
		return _townFoodVariety;
	}

	
	//int32 aveHappinessModifier(HappinessModifierEnum modifierEnum) {
	//	return _aveHappinessModifiers[static_cast<int>(modifierEnum)];
	//}
	//int32 aveHappinessModifierSum() {
	//	int32 sum = 0;
	//	for (size_t i = 0; i < _aveHappinessModifiers.size(); i++) {
	//		sum += _aveHappinessModifiers[i];
	//	}
	//	return sum;
	//}


	const std::vector<int32>& adultIds() { return _adultIds; }
	const std::vector<int32>& childIds() { return _childIds; }

	std::vector<int32> humanIds()
	{
		std::vector<int32> humanIds = _adultIds;
		humanIds.insert(humanIds.end(), _childIds.begin(), _childIds.end());
		return humanIds;
	}
	

	int32 totalRevenue100() {
		int32 revenue100 = 0;
		for (size_t i = 0; i < incomes100.size(); i++) {
			if (incomes100[i] > 0) revenue100 += incomes100[i];
		}
		revenue100 = revenue100 * taxPercent() / 100;
		return revenue100;
	}

	int32 totalRevenue100WithoutHouse() {
		int32 revenue100 = 0;
		for (size_t i = HouseIncomeEnumCount; i < incomes100.size(); i++) {
			if (incomes100[i] > 0) revenue100 += incomes100[i];
		}
		revenue100 = revenue100 * taxPercent() / 100;
		return revenue100;
	}
	int32 totalRevenue100HouseOnly() {
		int32 revenue100 = 0;
		for (size_t i = 0; i < HouseIncomeEnumCount; i++) {
			if (incomes100[i] > 0) revenue100 += incomes100[i];
		}
		revenue100 = revenue100 * taxPercent() / 100;
		return revenue100;
	}

	int32 totalExpense100() {
		int32 expense100 = 0;
		for (size_t i = 0; i < incomes100.size(); i++) {
			if (incomes100[i] < 0) expense100 += std::abs(incomes100[i]);
		}
		return expense100;
	}

	int32 totalIncome100() {
		return totalRevenue100() - totalExpense100();
	}

	void RecalculateTaxDelayed() {
		//PUN_LOG("RecalculateTaxDelayed");
		_needTaxRecalculation = true;
	}
	virtual void RecalculateTax(bool showFloatup) override;

	void ChangeIncome(int32 changeAmount, bool showFloatup, WorldTile2 floatupTile)
	{
		incomes100[static_cast<int>(IncomeEnum::BuildingUpkeep)] += changeAmount * 100;

		if (showFloatup) {
			_simulation->uiInterface()->ShowFloatupInfo(FloatupInfo(FloatupEnum::GainMoney, Time::Ticks(), floatupTile, TEXT_NUMSIGNED(changeAmount)));
		}
	}

	int32 totalInfluenceIncome100()
	{
		int32 influence100 = 0;
		for (size_t i = 0; i < influenceIncomes100.size(); i++) {
			influence100 += influenceIncomes100[i];
		}
		return influence100;
	}

	/*
	 * Population
	 */
	//! Population
	int32 population() { return adultPopulation() + childPopulation(); }

	int32 adultPopulation() { return _adultIds.size(); }
	int32 childPopulation() { return _childIds.size(); }

	FText GetTownSizeSuffix();
	FText GetTownSizeName();

	void PlayerAddHuman(int32 objectId);
	void PlayerRemoveHuman(int32 objectId) {
		auto found = std::find(_adultIds.begin(), _adultIds.end(), objectId);
		if (found != _adultIds.end()) {
			_adultIds.erase(found);
		}
		//_humanIds.erase(std::remove(_humanIds.begin(), _humanIds.end(), objectId));

		auto foundChild = std::find(_childIds.begin(), _childIds.end(), objectId);
		if (foundChild != _childIds.end()) {
			_childIds.erase(foundChild);
		}

		_simulation->QuestUpdateStatus(_playerId, QuestEnum::PopulationQuest, 0);

		RefreshJobDelayed();
		RecalculateTaxDelayed();
	}

	//! Houses
	void PlayerAddHouse(int32_t objectId) {
		// PUN_LOG("PlayerAddHouse");
		check(_simulation->building(objectId).isEnum(CardEnum::House));
		_houseIds.push_back(objectId);
	}
	void PlayerRemoveHouse(int32_t objectId) {
		auto found = std::find(_houseIds.begin(), _houseIds.end(), objectId);
		check(found != _houseIds.end());
		_houseIds.erase(found);
	}

	void PlayerAddJobBuilding(class Building& building, bool isConstructed);
	void PlayerRemoveJobBuilding(class Building& building, bool isConstructed);

	void PlayerAddBonusBuilding(int32 buildingId) {
		//_bonusBuildingIds.push_back(buildingId);
	}
	void PlayerRemoveBonusBuilding(int32 buildingId) {

	}

	int32 housingCapacity();
	int32 employedCount_WithBuilder() { return _employedCount; }
	int32 laborerCount() { return _adultIds.size() - _employedCount; }
	int32 builderCount() { return _builderCount; }
	int32 roadMakerCount() { return _roadMakerIds.size(); }

	int32 employedCount_WithoutBuilder() { return employedCount_WithBuilder() - builderCount(); }

	// Job changes when: 
	//  - Slots changed: building constructed/demolish/upgrade, human global priority manip, human building priority manip, human building set job manip
	//  - Population changed: Add/Remove Adults
	//  
	// Refresh at:
	//  - building constructed/demolish/upgrade: PlayerAddJobBuilding, PlayerRemoveJobBuilding, SetJobBuilding
	//  - human global priority manip: SetTownPriority
	//  - human building priority manip: SetPriority
	//  - human building set job manip: JobSlotChange
	//
	//  - Add/Remove Adults: PlayerAddHuman, PlayerRemoveHuman
	//  Note: child promotion done at beginning of RefreshJob
	void RefreshJobDelayed() {
		_needRefreshJob = true;
	}
	void RefreshJobs();


	void RefreshHousing();

	void FillHouseSlots_FromWorkplace(std::vector<int32>& tempHumanIds);


	
	/*
	 * Electricity
	 */
	int32 electricityConsumption() { return _electricityConsumption; }
	int32 electricityProductionCapacity() { return _electricityProductionCapacity; }

	

	/*
	 * Job
	 */

	const std::vector<std::vector<int32>>& jobBuildingEnumToIds() const { return _jobBuildingEnumToIds; }
	const std::vector<int32>& constructionIds() const { return _constructionIds; }

	const std::vector<int32>& roadConstructionIds() { return _roadConstructionIds; }

	int32 jobBuildingCount()
	{
		int32 jobBuildingCount = 0;
		for (size_t i = 0; i < _jobBuildingEnumToIds.size(); i++) {
			jobBuildingCount += _jobBuildingEnumToIds[i].size();
		}
		return jobBuildingCount;
	}


	const std::vector<CardEnum>& GetJobPriorityList() {
		return _jobPriorityList;
	}
	const std::vector<CardEnum>& GetLaborerPriorityList() {
		return _laborerPriorityList;
	}

	void SetGlobalJobPriority(FSetGlobalJobPriority command)
	{
		//if (command.jobPriorityList.Num() == 3) {
		//	TArray<int32> laborerPriorityListInt = command.jobPriorityList;
		//
		//	return;
		//}

		_jobPriorityList.clear();
		for (int32 i = 0; i < command.jobPriorityList.Num(); i++) {
			_jobPriorityList.push_back(static_cast<CardEnum>(command.jobPriorityList[i]));
		}

		// TODO: This is to fix the jobPriority disappearing
		if (_jobPriorityList.size() < DefaultJobPriorityListAllSeason.size())
		{
			// Find the missing jobPriority Row and add it to the end
			for (size_t i = 0; i < DefaultJobPriorityListAllSeason.size(); i++) {
				CardEnum cardEnum = DefaultJobPriorityListAllSeason[i];

				auto found = std::find(_jobPriorityList.begin(), _jobPriorityList.end(), cardEnum);
				if (found == _jobPriorityList.end()) {
					_jobPriorityList.push_back(cardEnum);
				}
			}
		}


		RefreshJobDelayed();

#if WITH_EDITOR
		for (int32 i = 0; i < _jobPriorityList.size(); i++) {
			PUN_LOG("_jobPriorityList %s", *(GetBuildingInfo(_jobPriorityList[i]).name.ToString()));
		}
#endif
	}

	void AddDeliverySource(int32 deliverySource) {
		CppUtils::TryAdd(_deliverySources, deliverySource);
	}
	void RemoveDeliverySource(int32 deliverySource) {
		CppUtils::TryRemove(_deliverySources, deliverySource);
	}
	const std::vector<int32>& allDeliverySources() {
		return _deliverySources;
	}

	std::shared_ptr<FSetTownPriority> CreateTownPriorityCommand()
	{
		auto command = std::make_shared<FSetTownPriority>();
		command->laborerPriority = targetLaborerHighPriority;
		command->builderPriority = targetBuilderHighPriority;
		command->roadMakerPriority = targetRoadMakerHighPriority;

		command->targetLaborerCount = targetLaborerCount;
		command->targetBuilderCount = targetBuilderCount;
		command->targetRoadMakerCount = targetRoadMakerCount;
		return command;
	}
	

	/*
	 * Science
	 */
	int64 science100PerRound() {
		return std::accumulate(sciences100.begin(), sciences100.end(), 0LL);
	}


	/*
	 * Stats
	 */
	const std::vector<int32>& GetPlotStatVec(PlotStatEnum roundStatEnum) const
	{
		return _enumToStatVec[static_cast<int>(roundStatEnum)];
	}

	/*
	 * Tax
	 */
	int32_t taxHappinessModifier()
	{
		switch (taxLevel)
		{
		case 0: return 10;
		case 1: return 5;
		case 2: return 0;
		case 3: return -20;
		case 4: return -40;
		default:
			UE_DEBUG_BREAK();
		}
		return -1;
	}
	int32_t taxPercent() {
		switch (taxLevel)
		{
		case 0: return 50;
		case 1: return 75;
		case 2: return 100;
		case 3: return 125;
		case 4: return 150;
		default:
			UE_DEBUG_BREAK();
		}
		return -1;
	}

	/*
	 * Data
	 */
	void AddDataPoint(PlotStatEnum statEnum, int32 data, int32 ticksPerStatInterval = TicksPerStatInterval)
	{
		std::vector<int32>& statVec = _enumToStatVec[static_cast<int>(statEnum)];
		int32 statTickCount = Time::Ticks() / ticksPerStatInterval;

		//while (statVec.size() < statTickCount) {
		for (int32 i = statVec.size(); i < statTickCount; i++) {
			statVec.push_back(0); // PlayerOwned might not be initialized from the first round...
		}

		statVec.push_back(data);
	};


	/*
	 * Resource Management
	 */
	bool GetHouseResourceAllow(ResourceEnum resourceEnum) {
		return _houseResourceAllowed[static_cast<int>(resourceEnum)];
	}
	void SetHouseResourceAllow(ResourceEnum resourceEnum, bool resourceAllowed);

	int32 GetOutputTarget(ResourceEnum resourceEnum) {
		return _resourceEnumToOutputTarget[static_cast<int>(resourceEnum)];
	}
	void SetOutputTarget(ResourceEnum resourceEnum, int32 amount) {
		_resourceEnumToOutputTarget[static_cast<int>(resourceEnum)] = amount;
	}

	int32 GetOutputTargetDisplay(ResourceEnum resourceEnum) { return _resourceEnumToOutputTargetDisplay[static_cast<int>(resourceEnum)]; }
	void SetOutputTargetDisplay(ResourceEnum resourceEnum, int32 amount) {
		_resourceEnumToOutputTargetDisplay[static_cast<int>(resourceEnum)] = amount;
	}

	/*
	 * Migration
	 */
	void AddMigrationPendingCount(int32 count) {
		_migrationPendingCount += count;
	}


	/*
	 * Townhall cards
	 */

	bool CanAddCardToTownhall() {
		return _cardsInTownhall.size() < _simulation->GetTownLvl(_townId);
	}

	void AddCardToTownhall(CardStatus card);

	CardEnum RemoveCardFromTownhall(int32 slotIndex) {
		if (slotIndex < _cardsInTownhall.size()) {
			CardEnum cardEnum = _cardsInTownhall[slotIndex].cardEnum;
			_cardsInTownhall.erase(_cardsInTownhall.begin() + slotIndex);
			return cardEnum;
		}
		return CardEnum::None;
	}
	void ClearCardsFromTownhall() {
		_cardsInTownhall.clear();
	}
	

	bool TownhallHasCard(CardEnum cardEnum)
	{
		for (size_t i = 0; i < _cardsInTownhall.size(); i++) {
			if (_cardsInTownhall[i].cardEnum == cardEnum) {
				return true;
			}
		}
		return false;
	}

	std::vector<CardStatus> cardsInTownhall() {
		return _cardsInTownhall;
	}

	int32 maxTownhallCards() {
		return _simulation->GetTownLvl(_townId);
	}

	int32 TownhallCardCount(CardEnum cardEnum)
	{
		int32 count = 0;
		for (size_t i = 0; i < _cardsInTownhall.size(); i++) {
			if (_cardsInTownhall[i].cardEnum == cardEnum) {
				count++;
			}
		}
		return count;
	}

	/*
	 * Permanent Town Bonuses
	 */
	void AddTownBonus(CardEnum cardEnum) {
		_townBonuses.push_back(cardEnum);
	}
	const std::vector<CardEnum>& townBonuses() {
		return _townBonuses;
	}
	bool HasTownBonus(CardEnum cardEnumIn) {
		for (CardEnum cardEnum: _townBonuses) {
			if (cardEnum == cardEnumIn) {
				return true;
			}
		}
		return false;
	}

	/*
	 * Training Queue
	 */
	std::vector<CardStatus> trainUnitsQueueDisplay()
	{
		std::vector<CardStatus> result = _trainUnitsQueue;
		for (const FUseCard& command : _pendingTrainCommands) {
			TrainUnits_Helper(command, result, false);
		}
		return result;
	}

	const std::vector<FUseCard>& pendingTrainCommands() {
		return _pendingTrainCommands;
	}
	void AddPendingTrainCommand(const FUseCard& command) {
		_pendingTrainCommands.push_back(command);
	}

	int32 GetTrainUnitCost(CardEnum cardEnum) {
		return GetBuildingInfo(cardEnum).baseCardPrice;
	}
	static int32 GetTrainingLengthTicks(CardEnum cardEnum)
	{
		int32 price = GetBuildingInfo(cardEnum).baseCardPrice;
		const int32 moneyCostPerSec = 20;
		return Time::TicksPerSecond * price / moneyCostPerSec;
	}

	float GetTrainingFraction()
	{
		if (_trainUnitsQueue.size() > 0) {
			return FMath::Clamp(static_cast<float>(_trainUnitsTicks100) / GetTrainingLengthTicks(_trainUnitsQueue[0].cardEnum), 0.0f, 1.0f);
		}
		return 0.0f;
	}
	
	void TrainUnits(const FUseCard& command) {
		TrainUnits_Helper(command, _trainUnitsQueue, true);
	}
	
	void TrainUnits_Helper(const FUseCard& command, std::vector<CardStatus>& trainUnitsQueue, bool isRealAction)
	{
		check(_pendingTrainCommands.size() > 0);
		check(_pendingTrainCommands[0] == command);
		if (isRealAction) {
			_pendingTrainCommands.erase(_pendingTrainCommands.begin());
		}

		CardEnum cardEnum = command.cardStatus.cardEnum;

		/*
		 * Cancel Train
		 */
		if (command.callbackEnum == CallbackEnum::CancelTrainUnit)
		{
			for (int32 i = trainUnitsQueue.size(); i-- > 0;)
			{
				if (trainUnitsQueue[i].cardBirthTicks == command.variable2) 
				{
					trainUnitsQueue[i].stackSize -= command.variable1;
					trainUnitsQueue[i].stackSize = std::max(0, trainUnitsQueue[i].stackSize);

					// Real Action: return the money
					if (isRealAction) {
						_simulation->ChangeMoney(_playerId, GetBuildingInfo(trainUnitsQueue[i].cardEnum).baseCardPrice);
					}

					// Remove stack if the size is 0
					if (trainUnitsQueue[i].stackSize == 0) {
						trainUnitsQueue.erase(trainUnitsQueue.begin() + i);

						// Real Action: If this is the first stack, reset the timer
						if (isRealAction && i == 0) {
							_trainUnitsTicks100 = 0;
						}
					}
					return;
				}
			}
			return;
		}

		/*
		 * Train
		 */
		// Real Action: pay money
		int32 trainingCount = command.variable1;
		if (isRealAction) 
		{
			int32 cardPrice = GetBuildingInfo(cardEnum).baseCardPrice;

			// Must be able to train at least one unit
			if (_simulation->moneyCap32(_playerId) >= cardPrice) 
			{
				// Can't train more than money allows
				int32 maxPossibleTraining = _simulation->moneyCap32(playerId()) / GetTrainUnitCost(command.cardStatus.cardEnum);
				trainingCount = std::min(trainingCount, maxPossibleTraining);
				trainingCount = std::max(1, trainingCount);

				int32 moneyToSpend = cardPrice * trainingCount;
				
				_simulation->ChangeMoney(_playerId, -moneyToSpend);
			}
			else {
				_simulation->AddPopupToFront(_playerId, 
					NSLOCTEXT("TrainUnits", "NotEnoughMoneyToTrainUnit", "Not enough money to train this unit."), 
					ExclusiveUIEnum::TrainUnitsUI, "PopupCannot"
				);
				return;
			}
		}

		// Fill the old stack
		if (trainUnitsQueue.size() > 0 &&
			trainUnitsQueue.back().cardEnum == cardEnum)
		{
			trainUnitsQueue.back().stackSize += trainingCount;
			return;
		}

		// Make a new stack
		if (trainUnitsQueue.size() < 5)
		{
			CardStatus cardStatus;
			cardStatus.cardEnum = cardEnum;
			cardStatus.stackSize = trainingCount;
			cardStatus.cardBirthTicks = Time::Ticks();

			trainUnitsQueue.push_back(cardStatus);
		}
	}

	/*
	 * Auto Trade
	 */
	void GetAutoTradeElementsDisplay(std::vector<AutoTradeElement>& autoExportElements, std::vector<AutoTradeElement>& autoImportElements)
	{
		autoExportElements = _autoExportElements;
		autoImportElements = _autoImportElements;

		for (int32 i = 0; i < _pendingAutoTradeCommands.size(); i++) {
			ExecuteAutoTradeCommand_Helper(_pendingAutoTradeCommands[i], autoExportElements, autoImportElements);
		}
	}

	int32 GetMaxAutoTradeAmount();

	void CalculateAutoTradeAmountNextRound() {
		CalculateAutoTradeAmountNextRound_Helper(_autoExportElements, _autoImportElements);
	}
	
	void CalculateAutoTradeAmountNextRound_Helper(std::vector<AutoTradeElement>& autoExportElements, std::vector<AutoTradeElement>& autoImportElements)
	{
		int32 tradeAmountLeftOver = GetMaxAutoTradeAmount();
		
		// Calculate the Trade Amount Next Round
		for (AutoTradeElement& autoTradeElement : autoExportElements) {
			int32 resourceCountTown = _simulation->resourceCountTown(_townId, autoTradeElement.resourceEnum);
			int32 tradeAmount = resourceCountTown - autoTradeElement.targetInventory;
			tradeAmount = Clamp(tradeAmount, 0, std::min(autoTradeElement.maxTradeAmount, tradeAmountLeftOver));
			autoTradeElement.calculatedTradeAmountNextRound = tradeAmount;
			
			tradeAmountLeftOver -= tradeAmount;
			check(tradeAmountLeftOver >= 0);
		}
		for (AutoTradeElement& autoTradeElement : autoImportElements) {
			int32 resourceCountTown = _simulation->resourceCountTown(_townId, autoTradeElement.resourceEnum);
			int32 tradeAmount = autoTradeElement.targetInventory - resourceCountTown;
			tradeAmount = Clamp(tradeAmount, 0, std::min(autoTradeElement.maxTradeAmount, tradeAmountLeftOver));
			autoTradeElement.calculatedTradeAmountNextRound = tradeAmount;

			tradeAmountLeftOver -= tradeAmount;
			check(tradeAmountLeftOver >= 0);
		}
	}

	bool HasAutoTradeResource(ResourceEnum resourceEnum)
	{
		std::vector<AutoTradeElement> autoExportElements;
		std::vector<AutoTradeElement> autoImportElements;
		GetAutoTradeElementsDisplay(autoExportElements, autoImportElements);
		
		for (int32 i = autoExportElements.size(); i-- > 0;) {
			if (autoExportElements[i].resourceEnum == resourceEnum) {
				return true;
			}
		}
		for (int32 i = autoImportElements.size(); i-- > 0;) {
			if (autoImportElements[i].resourceEnum == resourceEnum) {
				return true;
			}
		}
		return false;
	}

	void AddPendingAutoTradeCommand(const FGenericCommand& command) {
		_pendingAutoTradeCommands.push_back(command);
	}

	void ExecuteAutoTradeCommand(const FGenericCommand& command)
	{
		check(_pendingAutoTradeCommands.size() > 0);
		check(_pendingAutoTradeCommands[0] == command);
		_pendingAutoTradeCommands.erase(_pendingAutoTradeCommands.begin());
		
		ExecuteAutoTradeCommand_Helper(command, _autoExportElements, _autoImportElements);
	}

	void ExecuteAutoTradeCommand_Helper(const FGenericCommand& command, std::vector<AutoTradeElement>& autoExportElements, std::vector<AutoTradeElement>& autoImportElements)
	{
		CallbackEnum callbackEnum = command.callbackEnum;
		
		bool isExport = command.intVar1;
		ResourceEnum resourceEnum = static_cast<ResourceEnum>(command.intVar2);
		int32 targetInventory = command.intVar3;
		int32 maxTradeAmount = command.intVar4;

		std::vector<AutoTradeElement>& autoTradeElements = isExport ? autoExportElements : autoImportElements;
		
		if (callbackEnum == CallbackEnum::AddAutoTradeResource)
		{
			AutoTradeElement element;
			element.resourceEnum = resourceEnum;
			element.maxTradeAmount = 100;

			autoTradeElements.push_back(element);
		}
		else if (callbackEnum == CallbackEnum::ShiftAutoTradeRowUp ||
				callbackEnum == CallbackEnum::ShiftAutoTradeRowDown ||
				callbackEnum == CallbackEnum::ShiftAutoTradeRowFastUp ||
				callbackEnum == CallbackEnum::ShiftAutoTradeRowFastDown ||
				callbackEnum == CallbackEnum::RemoveAutoTradeRow)
		{
			int32 oldIndex = -1;
			AutoTradeElement oldAutoTradeElement;
			int32 initialElementsSize = autoTradeElements.size();

			for (int32 i = autoTradeElements.size(); i-- > 0;) 
			{
				if (autoTradeElements[i].resourceEnum == resourceEnum)
				{
					oldAutoTradeElement = autoTradeElements[i];
					oldIndex = i;
					autoTradeElements.erase(autoTradeElements.begin() + i);
					break;
				}
			}
			check(oldIndex != -1);

			if (callbackEnum == CallbackEnum::ShiftAutoTradeRowUp) {
				int32 index = std::max(0, oldIndex - 1);
				autoTradeElements.insert(autoTradeElements.begin() + index, oldAutoTradeElement);
			}
			else if (callbackEnum == CallbackEnum::ShiftAutoTradeRowDown) {
				int32 index = std::min(initialElementsSize - 1, oldIndex + 1);
				autoTradeElements.insert(autoTradeElements.begin() + index, oldAutoTradeElement);
			}
			else if (callbackEnum == CallbackEnum::ShiftAutoTradeRowFastUp) {
				autoTradeElements.insert(autoTradeElements.begin(), oldAutoTradeElement);
			}
			else if (callbackEnum == CallbackEnum::ShiftAutoTradeRowFastDown) {
				autoTradeElements.push_back(oldAutoTradeElement);
			}
		}
		else if (callbackEnum == CallbackEnum::AutoTradeRowTargetInventoryChanged ||
				callbackEnum == CallbackEnum::AutoTradeRowMaxTradeAmountChanged)
		{
			for (int32 i = autoTradeElements.size(); i-- > 0;)
			{
				if (autoTradeElements[i].resourceEnum == resourceEnum) {
					autoTradeElements[i].targetInventory = targetInventory;
					autoTradeElements[i].maxTradeAmount = maxTradeAmount;
					break;
				}
			}
		}

	}


	/*
	 * Tourism
	 */

	void DecreaseTourismHappinessByAction(int32 actionCost);

	template <typename Func>
	void LoopPopulation(int32 loopCount, int32& index, Func func)
	{
		for (int32 i = 0; i < loopCount; i++)
		{
			int32 unitId;
			if (index < _adultIds.size()) {
				unitId = _adultIds[index];
			}
			else {
				unitId = _childIds[index - _adultIds.size()];
			}

			func(unitId);

			index++;
			if (index >= _childIds.size() + _adultIds.size()) {
				index = 0;
			}
		}
	}
	
	

	/*
	 * Others
	 */
	TileArea territoryBoxExtent() { return _territoryBoxExtent; }

	int32 GetPlayerLandTileCount(bool includeMountain) {
		int32 tileCount = 0;
		auto& provinceSys = _simulation->provinceSystem();
		for (int32 provinceId : _provincesClaimed) {
			tileCount += provinceSys.provinceFlatTileCount(provinceId);
			if (includeMountain) {
				tileCount += provinceSys.provinceMountainTileCount(provinceId);
			}
		}
		return tileCount;
	}

private:
	/*
	 * Private Functions
	 */
	// Check buildings
	int32 TryFillJobBuildings(const std::vector<int32>& jobBuildingIds, PriorityEnum priority, int& index, bool shouldDoDistanceSort = true, int32 maximumFill = INT32_MAX);

	void CollectRoundIncome();
	void CollectHouseIncome();

	int32 targetNonLaborerCount()
	{
		if (targetLaborerHighPriority) {
			return _adultIds.size() - targetLaborerCount;
		}
		return _adultIds.size();
	}


	virtual void RefreshProvinceInfo() override
	{
		// Check for number of provinces from townhall
		//  Use Breadth-first search
		auto& regionSys = _simulation->provinceInfoSystem();

		// Clear claimed provinces (reset below)
		for (int32 provinceId : _provincesClaimed) {
			regionSys.SetProvinceDistanceMap(provinceId, MAX_int32);
		}

		int32 level = 0;
		std::vector<int32> curProvinceIds;
		curProvinceIds.push_back(_provincesClaimed[0]);
		regionSys.SetProvinceDistanceMap(_provincesClaimed[0], level);

		while (level < 7)
		{
			level++;

			std::vector<int32> nextProvinceIds;
			for (int32 curProvinceId : curProvinceIds)
			{
				const std::vector<ProvinceConnection>& connections = _simulation->GetProvinceConnections(curProvinceId);
				for (const ProvinceConnection& connection : connections) {
					if (connection.tileType == TerrainTileType::River ||
						connection.tileType == TerrainTileType::None)
					{
						// If this province is claimed by us
						if (_simulation->provinceOwnerTown_Major(connection.provinceId) == _townId)
						{
							// Set the provincesFromTownhall if needed
							for (int32 i = 0; i < _provincesClaimed.size(); i++) {
								if (connection.provinceId == _provincesClaimed[i] &&
									regionSys.provinceDistanceMap(connection.provinceId) == MAX_int32)
								{
									regionSys.SetProvinceDistanceMap(connection.provinceId, level);
									nextProvinceIds.push_back(connection.provinceId);
								}
							}
						}
					}
				}
			}

			curProvinceIds = nextProvinceIds;
		}

		// DEBUG
		for (int32 i = 0; i < _provincesClaimed.size(); i++) {
			PUN_LOG("Refresh province:%d distance:%d", _provincesClaimed[i], regionSys.provinceDistanceMap(_provincesClaimed[i]));
		}
	}

	/*
	 * Influence Price
	 */
	 // TODO: Other way to prevent strip empire?
	 // TODO: Border Province Count
	//int32 distanceFromCapital_PenaltyPercent(int32 provinceId)
	//{
	//	// Distance from Capital
	//	// Beyond X tiles from capital, we get penalty
	//	WorldTile2 townhallCenter = _simulation->townhallGateTile(_playerId);
	//	WorldTile2 provinceCenter = _simulation->GetProvinceCenterTile(provinceId);

	//	const int32 maxIncreasePercent = 100;
	//	const int32 penaltyStartDistance = 200;
	//	const int32 penaltyLerpDistance = 300;
	//	int32 distanceFromCapital = WorldTile2::Distance(townhallCenter, provinceCenter);
	//	return std::min(std::max(0, distanceFromCapital - penaltyStartDistance) * maxIncreasePercent / penaltyLerpDistance, maxIncreasePercent);
	//}

	//// TODO: Include this in Influence increment
	//int32 areaVsPopulation_PenaltyPercent()
	//{
	//	// Total Influence Area vs Population
	//	// Penalty applies when population is too small for empire.
	//	// 500 pop = 250,000 tiles empire ideal??
	//	const int32 tilesPerPopulation_minPenalty = 500;
	//	const int32 tilesPerPopulation_lerp = 1000;
	//	const int32 maxPenaltyPercent = 100;
	//	int32 tilesPerPopulation = GetPlayerLandTileCount(false) / std::max(1, population());
	//	return std::min(std::max(0, tilesPerPopulation - tilesPerPopulation_minPenalty) * maxPenaltyPercent / tilesPerPopulation_lerp, maxPenaltyPercent);
	//}

public:
	/*
	 * Serialize
	 */
	void Serialize(FArchive& Ar)
	{
		TownManagerBase::Serialize(Ar);
		
		//! Public
		//Ar << _playerId;
		//Ar << _townId;
		//Ar << townHallId;

		//SerializeVecValue(Ar, _provincesClaimed);

		SerializeVecValue(Ar, incomes100);
		SerializeVecValue(Ar, sciences100);
		SerializeVecValue(Ar, influenceIncomes100);

		Ar << taxLevel;

		Ar << targetLaborerHighPriority;
		Ar << targetBuilderHighPriority;
		Ar << targetRoadMakerHighPriority;
		Ar << targetLaborerCount;
		Ar << targetBuilderCount;
		Ar << targetRoadMakerCount;

		//! Private
		Ar << _needTaxRecalculation;
		Ar << _needRefreshJob;

		SerializeVecValue(Ar, _lastResourceCounts);

		SerializeVecValue(Ar, _adultIds);
		SerializeVecValue(Ar, _childIds);
		SerializeVecValue(Ar, _houseIds);

		SerializeVecVecValue(Ar, _jobBuildingEnumToIds);
		SerializeVecValue(Ar, _roadConstructionIds);
		SerializeVecValue(Ar, _constructionIds);
		SerializeVecValue(Ar, _bonusBuildingIds);

		SerializeVecValue(Ar, _jobPriorityList);
		SerializeVecValue(Ar, _laborerPriorityList);

		SerializeVecValue(Ar, _deliverySources);

		Ar << _employedCount;
		Ar << _builderCount;
		SerializeVecValue(Ar, _roadMakerIds);


		SerializeVecValue(Ar, _aveHappiness);
		Ar << _townFoodVariety;
		//SerializeVecValue(Ar, _aveHappinessModifiers);

		_territoryBoxExtent >> Ar;

		// Stats
		SerializeVecVecValue(Ar, _enumToStatVec);

		Ar << _nextDiseaseCheckTick;

		SerializeVecValue(Ar, _houseResourceAllowed);

		SerializeVecValue(Ar, _resourceEnumToOutputTarget);
		SerializeVecValue(Ar, _resourceEnumToOutputTargetDisplay);

		Ar << _migrationPendingCount;

		SerializeVecObj(Ar, _cardsInTownhall);

		SerializeVecValue(Ar, _townBonuses);
		
		SerializeVecValue(Ar, _trainUnitsQueue);
		Ar << _trainUnitsTicks100;
		
		Ar << _electricityProductionCapacity;
		Ar << _electricityConsumption;

		Ar << _tourismIncreaseIndex;
		Ar << _tourismDecreaseIndex;
	}

public:


	std::vector<int32> incomes100;
	std::vector<int64> sciences100;
	std::vector<int32> influenceIncomes100;

	// Tax Level
	int32 taxLevel = 2;

	// Laborer/Builder adjustments
	// normally Laborer and Builder counts are shown
	// Builder count is from summing up constructors in buildings
	// if star high priority, that means we want to set a target, in that case, fraction target appears along with adjustment arrows
	// if starred laborer, building slots filling will terminate once the targetLaborerCount is hit
	// if starred builder, 
	bool targetLaborerHighPriority = false;
	bool targetBuilderHighPriority = false;
	bool targetRoadMakerHighPriority = false;
	int32 targetLaborerCount = 1;
	int32 targetBuilderCount = 1;
	int32 targetRoadMakerCount = 1;
	
private:
	bool _needTaxRecalculation = false;
	bool _needRefreshJob = false;

	std::vector<int32> _lastResourceCounts; // Use this to check if resource goes from 0 to nonzero.

	std::vector<int32> _childIds;
	std::vector<int32> _adultIds;
	std::vector<int32> _houseIds;

	std::vector<std::vector<int32>> _jobBuildingEnumToIds;
	std::vector<int32> _constructionIds;
	std::vector<int32> _roadConstructionIds;
	std::vector<int32> _bonusBuildingIds;

	std::vector<CardEnum> _jobPriorityList;
	std::vector<CardEnum> _laborerPriorityList;

	std::vector<int32> _deliverySources; // Doesn't count shipping depot

	//

	int32 _employedCount = 0;
	int32 _builderCount = 0;
	std::vector<int32> _roadMakerIds;

	//int32 _aveFoodHappiness = 0;
	//int32 _aveHeatHappiness = 0;
	//int32 _aveHousingHappiness = 0;
	//int32 _aveFunHappiness = 0;
	//std::vector<int32> _aveHappinessModifiers;

	std::vector<int32> _aveHappiness;

	int32 _townFoodVariety = 1;

	//std::unordered_map<int32, int32> _claimedProvinceConnected; // Fast check if the province is too far from townhall
	TileArea _territoryBoxExtent;

	// 
	std::vector<std::vector<int32>> _enumToStatVec;

	int32 _nextDiseaseCheckTick;

	std::vector<uint8> _houseResourceAllowed;

	std::vector<int32> _resourceEnumToOutputTarget;
	std::vector<int32> _resourceEnumToOutputTargetDisplay; // Display changes instantly

	int32 _migrationPendingCount = 0;

	std::vector<CardStatus> _cardsInTownhall;

	std::vector<CardEnum> _townBonuses;

	// Train Units
	std::vector<CardStatus> _trainUnitsQueue;
	int32 _trainUnitsTicks100 = 0;



	int32 _electricityProductionCapacity = 0;
	int32 _electricityConsumption = 0;


	int32 _tourismIncreaseIndex = 0;
	int32 _tourismDecreaseIndex = 0;
	
private:
	/*
	 * Non-Serialize
	 */

	std::vector<FUseCard> _pendingTrainCommands;
	std::vector<FGenericCommand> _pendingAutoTradeCommands;
};
