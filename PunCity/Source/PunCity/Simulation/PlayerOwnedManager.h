// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "IGameSimulationCore.h"
#include "PlayerParameters.h"
#include "FateLinkSystem.h"
#include "TreeSystem.h"
#include "PunCity/NetworkStructs.h"
#include "PunCity/DisplaySystem/PunTerrainGenerator.h"
#include <algorithm>
#include "PunCity/CppUtils.h"
#include <numeric>
#include "ProvinceSystem.h"
#include "GameRegionSystem.h"
#include "ArmyNode.h"

using namespace std;

static const int32_t TownSizeMinPopulation[]
{
	0,
	25,
	50,
	100,
	150,
	200,
};
static int32 GetTownSizeMinPopulation(int32 tier) { return TownSizeMinPopulation[tier]; }

static const std::string TownSizeNames[]
{
	"Village",
	"Small Town",
	"Large Town",
	"Small City",
	"Large City",
	"Metropolis",
};
static int32_t TownSizeCount() { return _countof(TownSizeNames); }

static const std::string TownSizeSuffix[]
{
	"village",
	"town",
	"town",
	"city",
	"city",
	"metropolis",
};

static int32 GetTownSizeTier(int32 population)
{
	for (int32 i = 1; i < TownSizeCount(); i++) {
		if (population < GetTownSizeMinPopulation(i)) {
			return i - 1;
		}
	}
	return TownSizeCount() - 1;
}

static std::string GetTownSizeSuffix(int32 population)
{
	for (int32 i = 1; i < TownSizeCount(); i++) {
		if (population < GetTownSizeMinPopulation(i)) {
			return TownSizeSuffix[i - 1];
		}
	}
	return TownSizeSuffix[TownSizeCount() - 1];
}

// Priority list for what job need people living closer
static const std::vector<CardEnum> DefaultJobPriorityListAllSeason
{
	CardEnum::FruitGatherer,
	CardEnum::Farm,

	CardEnum::MushroomFarm,
	CardEnum::Fisher,
	CardEnum::HuntingLodge,

	CardEnum::RanchCow,
	CardEnum::RanchSheep,
	CardEnum::RanchPig,

	CardEnum::Windmill,
	CardEnum::Bakery,

	CardEnum::StoneToolShop,
	CardEnum::Blacksmith,
	CardEnum::Herbalist,
	CardEnum::MedicineMaker,

	CardEnum::Forester,
	CardEnum::CharcoalMaker,
	CardEnum::BeerBrewery,
	CardEnum::ClayPit,
	CardEnum::Potter,
	CardEnum::Tailor,

	CardEnum::GoldMine,
	CardEnum::Quarry,
	CardEnum::CoalMine,
	CardEnum::IronMine,

	CardEnum::IronSmelter,
	CardEnum::GoldSmelter,
	CardEnum::Mint,
	CardEnum::InventorsWorkshop,

	CardEnum::FurnitureWorkshop,
	CardEnum::Chocolatier,
	CardEnum::Winery,

	CardEnum::GemstoneMine,
	CardEnum::Jeweler,

	CardEnum::Beekeeper,
	CardEnum::Brickworks,
	CardEnum::CandleMaker,
	CardEnum::CottonMill,
	CardEnum::PrintingPress,

	CardEnum::PaperMaker,
	CardEnum::CardMaker,
	CardEnum::ImmigrationOffice,

	CardEnum::BarrackSwordman,
	CardEnum::BarrackArcher,
};

//static const std::vector<CardEnum> DefaultLaborerPriorityList
//{
//	CardEnum::Constructor,
//	CardEnum::GatherTiles,
//	CardEnum::Hauler,
//};


struct RegionClaimProgress
{
	int32 provinceId = -1;
	int32 claimValue100Inputted = 0;

	//! Serialize
	FArchive& operator>>(FArchive &Ar) {
		Ar << provinceId;
		Ar << claimValue100Inputted;
		return Ar;
	}
};

struct ProvinceClaimProgress
{
	int32 provinceId = -1;
	int32 attackerPlayerId = -1;

	int32 committedInfluencesAttacker = 0;
	int32 committedInfluencesDefender = 0;
	
	int32 ticksElapsed = 0;

	bool isValid() { return provinceId != -1; }

	void Tick1Sec(IGameSimulationCore* simulation)
	{
		//int32 attackerScaled = committedInfluencesAttacker * simulation->
		
		if (committedInfluencesAttacker > committedInfluencesDefender) {
			ticksElapsed += Time::TicksPerSecond * committedInfluencesAttacker / (committedInfluencesDefender + BattleInfluencePrice);
		} else {
			ticksElapsed -= Time::TicksPerSecond * (committedInfluencesDefender + BattleInfluencePrice) / committedInfluencesAttacker;
		}
	}

	//! Serialize
	FArchive& operator>>(FArchive &Ar) {
		Ar << provinceId;
		Ar << attackerPlayerId;
		Ar << committedInfluencesAttacker;
		Ar << committedInfluencesDefender;
		Ar << ticksElapsed;
		return Ar;
	}
};

/**
 * 
 */
class PlayerOwnedManager
{
public:
	PlayerOwnedManager(int32 playerId, IGameSimulationCore* simulation)
	{
		_playerId = playerId;
		townHallId = -1;
		_simulation = simulation;
		taxLevel = 2;

		_aveHappinessModifiers.resize(HappinessModifierEnumCount);

		incomes100.resize(IncomeEnumCount);
		influenceIncomes100.resize(InfluenceIncomeEnumCount);
		sciences100.resize(ScienceEnumCount);

		_enumToStatVec.resize(static_cast<int>(PlotStatEnum::Count));

		_buffTicksLeft.resize(NonBuildingCardEnumCount, -1);

		_spTicks = 0;
		_maxSPTicks = MaxSP * ticksPerSP();
		_currentSkill = CardEnum::SpeedBoost;

		_isInDarkAge = false;
		_nextDiseaseCheckTick = 0;

		_houseResourceAllowed.resize(static_cast<int>(ResourceEnumCount), true);

		_jobPriorityList = DefaultJobPriorityListAllSeason;

		_lastResourceCounts.resize(ResourceEnumCount);

		_jobBuildingEnumToIds.resize(BuildingEnumCount);
	}

	//! Population
	int32 population() { return adultPopulation() + childPopulation(); }

	int32 adultPopulation() { return _adultIds.size(); }
	int32 childPopulation() { return _childIds.size(); }
	
	std::string GetTownSizeName() {
		for (int32_t i = TownSizeCount(); i-- > 0;) {
			if (population() >= TownSizeMinPopulation[i]) {
				return TownSizeNames[i];
			}
		}
		UE_DEBUG_BREAK();
		return "";
	}

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

	void TickRound();
	void Tick1Sec();
	void Tick()
	{
		if (_needTaxRecalculation) {
			_needTaxRecalculation = false;
			RecalculateTax(false);
		}

		// Tick Mana
		int32 spIncrement = _isInDarkAge ? 2 : 1;
		_spTicks = min(_spTicks + spIncrement, _maxSPTicks);

		// Remove skill buff once timer ran out
		for (size_t i = _usedSkillTicks.size(); i-- > 0;) {
			if (Time::Ticks() - _usedSkillTicks[i] > Time::TicksPerSecond * 50) {
				_usedSkillTicks.erase(_usedSkillTicks.begin() + i);
				_usedSkillBuildingIds.erase(_usedSkillBuildingIds.begin() + i);
				_usedSkillEnums.erase(_usedSkillEnums.begin() + i);
			}
		}

		TickArmyRegionClaim();
	}
	
	int32 aveFoodHappiness() { return _aveFoodHappiness; }
	int32 aveHeatHappiness() { return _aveHeatHappiness; }
	int32 aveHousingHappiness() { return _aveHousingHappiness; }
	int32 aveFunHappiness() { return _aveFunHappiness; }
	int32 aveNeedHappiness() { return (_aveFoodHappiness + _aveHeatHappiness + _aveHousingHappiness + _aveFunHappiness) / 4; }

	
	int32 aveHappinessModifier(HappinessModifierEnum modifierEnum) {
		return _aveHappinessModifiers[static_cast<int>(modifierEnum)];
	}
	int32 aveHappinessModifierSum() {
		int32 sum = 0;
		for (size_t i = 0; i < _aveHappinessModifiers.size(); i++) {
			sum += _aveHappinessModifiers[i];
		}
		return sum;
	}

	int32 aveHappiness() { return aveNeedHappiness() + aveHappinessModifierSum(); }


	const std::vector<int32>& adultIds() { return _adultIds; }
	const std::vector<int32>& childIds() { return _childIds; }

	bool hasChosenLocation() { return _provincesClaimed.size() > 0; }
	bool hasChosenInitialResources() { return initialResources.isValid(); }
	bool hasTownhall() { return townHallId != -1; }
	
public:
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
		_needTaxRecalculation = true;
	}
	void RecalculateTax(bool showFloatup);

	void AddTaxIncomeToString(std::stringstream& ss) {
		ss << fixed << setprecision(1);
		ss << "Income: " << totalIncome100() / 100.0f << "<img id=\"Coin\"/>\n";

		for (int32 i = 0; i < IncomeEnumCount; i++)
		{
			IncomeEnum incomeEnum = static_cast<IncomeEnum>(i);

			if (incomes100[i] != 0)
			{
				ss << " " << (incomes100[i] > 0 ? "+" : "") << (incomes100[i] / 100.0f);
				
				if (IsHouseIncomeEnum(incomeEnum)) {
					ss << " House";
				}

				ss << " " << IncomeEnumName[i] <<"\n";
			}
		}


		if (hasChosenLocation()) {
			ss << ToStdString(TaxOptions[taxLevel]);
			ss << " (" << std::to_string(taxPercent()) << "%)";
		}
	}
	void AddInfluenceIncomeToString(std::stringstream& ss) {
		ss << fixed << setprecision(1);
		ss << "Influence Per Round: " << totalInfluenceIncome100() / 100.0f << "<img id=\"Influence\"/>\n";

		for (int32 i = 0; i < InfluenceIncomeEnumCount; i++)
		{
			if (influenceIncomes100[i] != 0)
			{
				ss << " " << (influenceIncomes100[i] > 0 ? "+" : "") << (influenceIncomes100[i] / 100.0f);
				ss << " " << InfluenceIncomeEnumName[i] << "\n";
			}
		}

		auto addStoredInfluenceRow = [&](InfluenceIncomeEnum influenceEnum) {
			ss << " " << (influenceIncomes100[static_cast<int>(influenceEnum)] * storedToInfluenceRevenue)
			   << " " << InfluenceIncomeEnumName[static_cast<int>(influenceEnum)] <<"\n";
		};
		
		ss << "<space>";
		ss << "Max Stored Influence: " << maxStoredInfluence100() << "<img id=\"Influence\"/>\n";
		addStoredInfluenceRow(InfluenceIncomeEnum::Townhall);
		addStoredInfluenceRow(InfluenceIncomeEnum::Population);
		addStoredInfluenceRow(InfluenceIncomeEnum::Luxury);
	}

	void ChangeIncome(int32 changeAmount, bool showFloatup, WorldTile2 floatupTile)
	{
		incomes100[static_cast<int>(IncomeEnum::BuildingUpkeep)] += changeAmount * 100;

		if (showFloatup) {
			_simulation->uiInterface()->ShowFloatupInfo(FloatupInfo(FloatupEnum::GainMoney, Time::Ticks(), floatupTile, ToForcedSignedNumber(changeAmount)));
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

	static const int32 storedToInfluenceRevenue = 20;
	int32 maxStoredInfluence100()
	{
		return (influenceIncomes100[static_cast<int>(InfluenceIncomeEnum::Townhall)] +
			influenceIncomes100[static_cast<int>(InfluenceIncomeEnum::Population)] +
			influenceIncomes100[static_cast<int>(InfluenceIncomeEnum::Luxury)]) * storedToInfluenceRevenue;
	}

	/*
	 * Claim Region
	 */

	// TODO: Old
	int32 GetBaseProvinceClaimPrice(int32 provinceId)
	{
		int32 x = (_provincesClaimed.size() + 5);
		int32 baseClaimPrice = x * x * 2; // 25*4 = 200, 36*4 = 288, 49*4 = 196 .... 8*(5^2 + 6^2 + 7^2) = 880
		baseClaimPrice *= _simulation->IsResearched(_playerId, TechEnum::CheapLand) ? 7 : 10;
		baseClaimPrice /= 15;

		// TODO: check number of adjacent provinces
		//if (_simulation->regionOwner(region.north().regionId()) != _playerId) {
		//	baseClaimPrice += baseClaimPrice / 5;
		//}
		//if (_simulation->regionOwner(region.south().regionId()) != _playerId) {
		//	baseClaimPrice += baseClaimPrice / 5;
		//}
		//if (_simulation->regionOwner(region.east().regionId()) != _playerId) {
		//	baseClaimPrice += baseClaimPrice / 5;
		//}
		//if (_simulation->regionOwner(region.west().regionId()) != _playerId) {
		//	baseClaimPrice += baseClaimPrice / 5;
		//}

		// Count tree price as 1.5
		const int32 singleTreePrice100 = 150;


		// Trees
		auto& provinceSys = _simulation->provinceSystem();
		int32 treePrice = _simulation->GetTreeCount(provinceId) * singleTreePrice100 / 100;

		// Flat land
		int32 flatLandPrice = provinceSys.provinceFlatTileCount(provinceId) / 3;

		BiomeEnum biome = _simulation->GetBiomeProvince(provinceId);
		if (biome == BiomeEnum::GrassLand || biome == BiomeEnum::Savanna) {
			flatLandPrice /= 2;
		}
		else if (biome == BiomeEnum::Tundra || biome == BiomeEnum::Desert) {
			flatLandPrice /= 4;
		}
		else if (biome == BiomeEnum::Jungle) {
			flatLandPrice = flatLandPrice * 3 / 2;
		}

		flatLandPrice = flatLandPrice * x / 20; // at regionsClaimed 0 ... around 500 * 5 /20 = 125

		// Georesource
		int32 georesourcePrice = _simulation->georesource(provinceId).HasResource() ? 200 : 0;

		int32 finalPriceBeforeMod = baseClaimPrice + treePrice + flatLandPrice + georesourcePrice;

		// Game balance price adjustment
		finalPriceBeforeMod /= 2;

		/*
		 * Price modifiers
		 */

		
		return finalPriceBeforeMod;
	}

	//int32 GetProvinceClaimPriceGold(int32 provinceId)
	//{
	//	int32 price = GetInfluenceClaimPrice(provinceId);
	//	
	//	BiomeEnum biomeEnum = _simulation->GetBiomeProvince(provinceId);
	//	if (_simulation->IsResearched(_playerId, TechEnum::BorealLandCost) &&
	//		(biomeEnum == BiomeEnum::BorealForest || biomeEnum == BiomeEnum::Tundra))
	//	{
	//		price /= 2;
	//	}

	//	return price;
	//}

	void TryRemoveProvinceClaim(int32 provinceId, bool lightMode)
	{
		CppUtils::TryRemove(_provincesClaimed, provinceId);

		if (!lightMode) {
			RecalculateTax(false);
		}
	}
	
	void ClaimProvince(int32 provinceId, bool lightMode = false)
	{
		//PUN_LOG("ClaimProvince province:%d pid:%d", provinceId, _playerId);
		
		//WorldTile2 minTile = region.minTile();
		//WorldTile2 maxTile = region.maxTile();
		//if (_regionsClaimed.size() == 0) {
		//	_territoryBoxExtent = TileArea(minTile.x, minTile.y, maxTile.x, maxTile.y);
		//}
		//_territoryBoxExtent = TileArea(std::min(_territoryBoxExtent.minX, minTile.x), std::min(_territoryBoxExtent.minY, minTile.y),
		//								std::max(_territoryBoxExtent.maxX, maxTile.x), std::max(_territoryBoxExtent.maxY, maxTile.y));
		
		_provincesClaimed.push_back(provinceId);
		//_provincesInfluenced.push_back(provinceId);

		// If this is autoClaiming region, remove it from autoclaim
		if (_armyAutoClaimProgress.provinceId == provinceId) {
			_armyAutoClaimProgress = RegionClaimProgress();
		}

		if (lightMode) {
			return;
		}
		
		RecalculateTax(false);
	}

	/*
	 * Claim Province Attack
	 */
	void StartConquerProvince(int32 attackerPlayerId, int32 provinceId)
	{
		ProvinceClaimProgress claimProgress;
		claimProgress.provinceId = provinceId;
		claimProgress.attackerPlayerId = attackerPlayerId;
		claimProgress.committedInfluencesAttacker = BattleInfluencePrice;
		claimProgress.committedInfluencesDefender = 0;
		claimProgress.ticksElapsed = 0;
		
		_defendingClaimProgress.push_back(claimProgress);
	}
	void StartConquerProvince_Attacker(int32 provinceId) {
		_attackingProvinceIds.push_back(provinceId);
	}
	void ReinforceAttacker(int32 provinceId, int32 influenceAmount)
	{
		for (auto& claimProgress : _defendingClaimProgress) {
			if (claimProgress.provinceId == provinceId) {
				claimProgress.committedInfluencesAttacker += influenceAmount;
				break;
			}
		}
	}
	void ReinforceDefender(int32 provinceId, int32 influenceAmount)
	{
		for (auto& claimProgress : _defendingClaimProgress) {
			if (claimProgress.provinceId == provinceId) {
				claimProgress.committedInfluencesDefender += influenceAmount;
				break;
			}
		}
	}
	const std::vector<ProvinceClaimProgress>& defendingClaimProgress() { return _defendingClaimProgress; }

	ProvinceClaimProgress GetDefendingClaimProgress(int32 provinceId) const {
		for (const ProvinceClaimProgress& claimProgress : _defendingClaimProgress) {
			if (claimProgress.provinceId == provinceId) {
				return claimProgress;
			}
		}
		return ProvinceClaimProgress();
	}
	
	
	void EndConquer(int32 provinceId) {
		CppUtils::RemoveOneIf(_defendingClaimProgress, [&](ProvinceClaimProgress& claimProgress) { return claimProgress.provinceId == provinceId; });
	}
	void EndConquer_Attacker(int32 provinceId) {
		CppUtils::Remove(_attackingProvinceIds, provinceId);
	}

	ProvinceAttackEnum GetProvinceAttackEnum(int32 provinceId)
	{
		int32 provincePlayerId = _simulation->provinceOwner(provinceId);
		ProvinceClaimProgress claimProgress = GetDefendingClaimProgress(provinceId);

		// TODO: Add VassalCompetition
		if (lordPlayerId() != -1) 
		{
			if (claimProgress.attackerPlayerId == _playerId) {
				return ProvinceAttackEnum::DeclareIndependence;
			}
			return ProvinceAttackEnum::VassalCompetition;
		}
		if (_simulation->homeProvinceId(provincePlayerId) == provinceId) {
			return ProvinceAttackEnum::Vassalize;
		}
		return ProvinceAttackEnum::ConquerProvince;
	}
	
	
	
	//void MarkAsOutpost(int32 provinceId) {
	//	_provincesOutpost.push_back(provinceId);
	//}
	//bool TryRemoveOutpost(int32 provinceId) {
	//	return CppUtils::TryRemove(_provincesOutpost, provinceId);
	//}

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

	//bool HasOutpostAt(int32 provinceId) {
	//	return CppUtils::Contains(_provincesOutpost, provinceId);
	//}

	const std::vector<int32>& provincesClaimed() {
		return _provincesClaimed;
	}
	//const std::vector<int32>& provincesInfluenced() {
	//	return _provincesInfluenced;
	//}
	//const std::vector<int32>& provincesOutpost() {
	//	return _provincesOutpost;
	//}

	TileArea territoryBoxExtent() { return _territoryBoxExtent; }

	/*
	 * Influence Price
	 */
private:
	// TODO: Other way to prevent strip empire?
	// TODO: Border Province Count
	int32 distanceFromCapital_PenaltyPercent(int32 provinceId)
	{
		// Distance from Capital
		// Beyond X tiles from capital, we get penalty
		WorldTile2 townhallCenter = _simulation->townhallGateTile(_playerId);
		WorldTile2 provinceCenter = _simulation->GetProvinceCenterTile(provinceId);

		const int32 maxIncreasePercent = 100;
		const int32 penaltyStartDistance = 200;
		const int32 penaltyLerpDistance = 300;
		int32 distanceFromCapital = WorldTile2::Distance(townhallCenter, provinceCenter);
		return min(max(0, distanceFromCapital - penaltyStartDistance) * maxIncreasePercent / penaltyLerpDistance, maxIncreasePercent);
	}

	// TODO: Include this in Influence increment
	int32 areaVsPopulation_PenaltyPercent()
	{
		// Total Influence Area vs Population
		// Penalty applies when population is too small for empire.
		// 500 pop = 250,000 tiles empire ideal??
		const int32 tilesPerPopulation_minPenalty = 500;
		const int32 tilesPerPopulation_lerp = 1000;
		const int32 maxPenaltyPercent = 100;
		int32 tilesPerPopulation = GetPlayerLandTileCount(false) / std::max(1, _simulation->population(_playerId));
		return min(max(0, tilesPerPopulation - tilesPerPopulation_minPenalty) * maxPenaltyPercent / tilesPerPopulation_lerp, maxPenaltyPercent);
	}
public:
	int32 GetProvinceUpkeep100(int32 provinceId)
	{
		// Upkeep is half of the income in ideal
		// Upkeep suffers from the same penalty as claim price
		int32 baseUpkeep100 = _simulation->GetProvinceIncome100(provinceId) / 2;
		return baseUpkeep100;
	}

	//int32 GetInfluenceClaimPriceRefund(int32 provinceId) {
	//	return _simulation->GetProvinceIncome100(provinceId) * ClaimToIncomeRatio; // Don't refund penalty cost
	//}
	

	/*
	 * Claim queue
	 */
	RegionClaimProgress armyAutoClaimProgress() {
		return _armyAutoClaimProgress;
	}
	
	const std::vector<RegionClaimProgress>& armyRegionsClaimQueue() {
		return _armyRegionClaimQueue;
	}

	//bool IsProvinceClaimQueuable(int32 provinceId)
	//{
	//	if (_simulation->IsProvinceNextToPlayer(provinceId, _playerId)) {
	//		return true;
	//	}
	//	
	//	// Could be queued next to the previous one...
	//	for (const RegionClaimProgress& claim : _armyRegionClaimQueue) {
	//		if (_simulation->AreAdjacentProvinces(claim.provinceId, provinceId)) {
	//			return true;
	//		}
	//	}
	//	return false;
	//}
	
	//void QueueArmyProvinceClaim(int32 provinceId)
	//{
	//	if (!IsProvinceClaimQueuable(provinceId)) {
	//		return;
	//	}
	//	
	//	// if it has existing progress, bring it back
	//	auto it = std::find_if(_armyRegionClaimCanceled.begin(), _armyRegionClaimCanceled.end(), [&](RegionClaimProgress& claimProgress) {
	//		return claimProgress.provinceId == provinceId;
	//	});
	//	if (it != _armyRegionClaimCanceled.end()) {
	//		_armyRegionClaimQueue.push_back(*it);
	//		_armyRegionClaimCanceled.erase(it);
	//	} else {

	//		// If this is the same as _autoClaimProgress
	//		if (_armyAutoClaimProgress.provinceId == provinceId) {
	//			_armyRegionClaimQueue.push_back(_armyAutoClaimProgress);
	//			_armyAutoClaimProgress = RegionClaimProgress();
	//		} else {
	//			_armyRegionClaimQueue.push_back({ provinceId, 0 });
	//		}
	//	}
	//}
	//void CancelArmyProvinceClaim(int32 provinceId) {
	//	CancelSingleArmyProvinceClaim_Helper(provinceId);

	//	// Canceling one in a queue could cause others in the queue to become invalid
	//	// Solve this by Canceling everything, and readding them making sure the queue is valid
	//	std::vector<RegionClaimProgress> claimQueue = _armyRegionClaimQueue;
	//	ClearArmyProvinceClaim();

	//	for (RegionClaimProgress claim : claimQueue) {
	//		QueueArmyProvinceClaim(claim.provinceId);
	//	}
	//	
	//	for (size_t i = 0; i < _armyRegionClaimQueue.size();) {
	//		if (!IsProvinceClaimQueuable(_armyRegionClaimQueue[i].provinceId)) {
	//			CancelSingleArmyProvinceClaim_Helper(_armyRegionClaimQueue[i].provinceId);
	//		} else {
	//			i++;
	//		}
	//	}
	//}
	void CancelSingleArmyProvinceClaim_Helper(int32 provinceId) {
		// Cancel a single region without considering how it may affect others in queue
		auto it = std::find_if(_armyRegionClaimQueue.begin(), _armyRegionClaimQueue.end(), [&](RegionClaimProgress& claimProgress) {
			return claimProgress.provinceId == provinceId;
		});
		if (it != _armyRegionClaimQueue.end()) {
			if (it->claimValue100Inputted > 0) {
				_armyRegionClaimCanceled.push_back(*it);
			}
			_armyRegionClaimQueue.erase(it);
		}
	}
	
	void ClearArmyProvinceClaim() {
		for (const RegionClaimProgress& claimProgress : _armyRegionClaimQueue) {
			if (claimProgress.claimValue100Inputted > 0) {
				_armyRegionClaimCanceled.push_back(claimProgress);
			}
		}
		_armyRegionClaimQueue.clear();
	}

	void TickArmyRegionClaim()
	{	
		// Tick region claim every 0.25 sec
		if (Time::Ticks() % 15 == 0)
		{
			auto& provinceSys = _simulation->provinceSystem();
			
			if (_armyRegionClaimQueue.size() > 0)
			{
				int32 provinceId = _armyRegionClaimQueue[0].provinceId;

				// land was taken by others, cancel..
				if (_simulation->provinceOwner(provinceId) != -1) {
					_armyRegionClaimQueue.erase(_armyRegionClaimQueue.begin());
					return;
				}
				
				int32 totalArmyCount = CppUtils::Sum(_simulation->GetCapitalArmyCounts(_playerId, true));
				int32 claimValue100Increment = GameRand::Rand100RoundTo1(totalArmyCount * LandClaimValue10000PerQuarterSec); // 10,000 to 100
				if (SimSettings::IsOn("CheatFastBuild")) {
					claimValue100Increment *= 10;
				}

				_armyRegionClaimQueue[0].claimValue100Inputted += claimValue100Increment;

				if ((_armyRegionClaimQueue[0].claimValue100Inputted / 100) >= GetBaseProvinceClaimPrice(provinceId)) {
					_armyRegionClaimQueue.erase(_armyRegionClaimQueue.begin());
					_simulation->SetProvinceOwnerFull(provinceId, _playerId);
					return;
				}
			}
			else
			{
				// Auto claim
				int32 totalArmyCount = CppUtils::Sum(_simulation->GetCapitalArmyCounts(_playerId, true));

				// If there is no army, don't auto claim
				if (totalArmyCount == 0) {
					return;
				}

				// Set autoclaim
				if (!_simulation->IsProvinceValid(_armyAutoClaimProgress.provinceId))
				{
					// Find a closeby region to claim
					int32 maxScore = 0;
					int32 maxScoreProvinceId = -1;
					for (int32 provinceId : _provincesClaimed)
					{
						provinceSys.ExecuteAdjacentProvinces(provinceId, [&](ProvinceConnection connection)
						{
							if (_simulation->provinceOwner(connection.provinceId) != -1) {
								return;
							}
							
							int32 score = provinceSys.provinceFlatTileCount(connection.provinceId);
							BiomeEnum biomeEnum = _simulation->GetBiomeProvince(connection.provinceId);
							if (biomeEnum == BiomeEnum::GrassLand ||
								biomeEnum == BiomeEnum::Savanna)
							{
								score /= 2;
							}
							else if (biomeEnum == BiomeEnum::Desert ||
								biomeEnum == BiomeEnum::Tundra)
							{
								score /= 4;
							}

							// Score affected by distance
							WorldTile2 provinceCenter = provinceSys.GetProvinceCenterTile(connection.provinceId);
							int32 distance = WorldTile2::Distance(provinceCenter, _simulation->townhallGateTile(_playerId));
							score = score * CoordinateConstants::TilesPerRegion / distance;

							if (score > maxScore) {
								maxScore = score;
								maxScoreProvinceId = connection.provinceId;
							}
						});
					}

					if (maxScoreProvinceId != -1) {
						auto found = std::find_if(_armyRegionClaimCanceled.begin(), _armyRegionClaimCanceled.end(), [&](RegionClaimProgress& claim) { return claim.provinceId == maxScoreProvinceId; });
						if (found != _armyRegionClaimCanceled.end()) {
							_armyAutoClaimProgress = *found;
							_armyRegionClaimCanceled.erase(found);
						} else {
							_armyAutoClaimProgress = { maxScoreProvinceId, 0 };
						}
					}
				}

				// Progress autoclaim
				// TODO: else if for this and above?
				if (_simulation->IsProvinceValid(_armyAutoClaimProgress.provinceId))
				{
					int32 claimValue100Increment = GameRand::Rand100RoundTo1(totalArmyCount * LandClaimValue10000PerQuarterSec);

					_armyAutoClaimProgress.claimValue100Inputted += claimValue100Increment;

					if ((_armyAutoClaimProgress.claimValue100Inputted / 100) >= GetBaseProvinceClaimPrice(_armyAutoClaimProgress.provinceId)) {
						_simulation->SetProvinceOwnerFull(_armyAutoClaimProgress.provinceId, _playerId);
						_armyAutoClaimProgress = RegionClaimProgress();
						return;
					}
				}
			}
		}
	}

	/*
	 * Building Ids
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
		
		PUN_CHECK(command.jobPriorityList.Num() == DefaultJobPriorityListAllSeason.size());

		_jobPriorityList.clear();
		for (int32 i = 0; i < command.jobPriorityList.Num(); i++) {
			_jobPriorityList.push_back(static_cast<CardEnum>(command.jobPriorityList[i]));
		}

		RefreshJobDelayed();

#if WITH_EDITOR
		for (int32 i = 0; i < _jobPriorityList.size(); i++) {
			PUN_LOG("_jobPriorityList %s", ToTChar(GetBuildingInfo(_jobPriorityList[i]).name));
		}
#endif
	}
	

	/*
	 * Science
	 */
	int32 science100PerRound() {
		return std::accumulate(sciences100.begin(), sciences100.end(), 0);
	}


	/*
	 * Stats
	 */
	const std::vector<int32>& GetStatVec(PlotStatEnum roundStatEnum) const
	{
		return _enumToStatVec[static_cast<int>(roundStatEnum)];
	}


	/*
	 * Vassal
	 */
	int32 lordPlayerId() { return _lordPlayerId; }
	void SetLordPlayerId(int32 lordPlayerId) {
		_lordPlayerId = lordPlayerId;
	}

	int32 playerIdForColor() {
		if (_lordPlayerId != -1) {
			return _lordPlayerId;
		}
		return _playerId;
	}
	
	const std::vector<int32>& vassalBuildingIds() const { return _vassalBuildingIds; }
	void GainVassal(int32 vassalBuildingId) {
		_vassalBuildingIds.push_back(vassalBuildingId);
	}
	void LoseVassal(int32 vassalBuildingId) {
		CppUtils::TryRemove(_vassalBuildingIds, vassalBuildingId);
	}
	bool IsVassal(int32 vassalBuildingId) {
		return CppUtils::Contains(_vassalBuildingIds, vassalBuildingId);
	}

	/*
	 * Ally
	 */
	const std::vector<int32>& allyPlayerIds() { return _allyPlayerIds; }
	void GainAlly(int32 allyPlayerId) {
		_allyPlayerIds.push_back(allyPlayerId);
	}
	void LoseAlly(int32 allyPlayerId) {
		CppUtils::TryRemove(_allyPlayerIds, allyPlayerId);
	}
	bool IsAlly(int32 playerId) {
		return CppUtils::Contains(_allyPlayerIds, playerId);
	}

	/*
	 * Visited Node
	 */
	const std::vector<int32>& nodeIdsVisited() { return _armyNodesVisited; }
	void TryAddArmyNodeVisited(int32 nodeId) {
		CppUtils::TryAdd(_armyNodesVisited, nodeId);
	}
	


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
	int32 vassalTaxPercent() {
		return 5;
		//switch (taxLevel)
		//{
		//case 0: return 0;
		//case 1: return 10;
		//case 2: return 20;
		//case 3: return 30;
		//case 4: return 40;
		//default:
		//	UE_DEBUG_BREAK();
		//}
		//return -1;
	}

	/*
	 * Buffs
	 */
	void AddBuff(CardEnum cardEnum) {
		_buffTicksLeft[static_cast<int>(cardEnum) - BuildingEnumCount] = Time::TicksPerYear;
	}
	bool HasBuff(CardEnum cardEnum) {
		return _buffTicksLeft[static_cast<int>(cardEnum) - BuildingEnumCount] > 0;
	}
	std::vector<CardEnum> GetBuffs() {
		std::vector<CardEnum> buffEnums;
		for (size_t i = 0; i < _buffTicksLeft.size(); i++) {
			if (_buffTicksLeft[i] > 0) {
				buffEnums.push_back(static_cast<CardEnum>(i + BuildingEnumCount));
			}
		}
		return buffEnums;
	}
	int32 GetBuffTicksLeft(CardEnum cardEnum) {
		return _buffTicksLeft[static_cast<int>(cardEnum) - BuildingEnumCount];
	}

	void TryApplyBuff(CardEnum cardEnum)
	{
		if (HasBuff(cardEnum) && GetBuffTicksLeft(cardEnum) > Time::TicksPerMinute * 5) {
			_simulation->AddPopupToFront(_playerId, GetBuildingInfo(cardEnum).name + " has already been applied.");
			return;
		}
		
		int32 cost = _simulation->population(_playerId);
		if (_simulation->money(_playerId) < cost) {
			_simulation->AddPopupToFront(_playerId, "Require <img id=\"Coin\"/>xPopulation to activate the protection.");
		}
		else {
			_simulation->ChangeMoney(_playerId, -cost);
			AddBuff(cardEnum);

			if (cardEnum == CardEnum::KidnapGuard) {
				_simulation->AddPopupToFront(_playerId, "Applied Kidnap Guard. Protect your town against Kidnap for 1 year.");
			} else {
				_simulation->AddPopupToFront(_playerId, "Applied Treasury Guard. Protect your town against Snatch and Steal for 1 year.");
			}
		}
	}

	/*
	 * SP Skill
	 */
	float spFloat() { return static_cast<float>(_spTicks) / ticksPerSP(); }
	int32 GetSP() { return _spTicks / ticksPerSP(); }
	int32 maxSP() { return _maxSPTicks/ ticksPerSP(); }
	CardEnum currentSkill() { return _currentSkill; }

	void UseSkill(int32 buildingId) {
		_spTicks = max(0, _spTicks - GetSkillManaCost(_currentSkill) * ticksPerSP());

		_usedSkillEnums.push_back(_currentSkill);
		_usedSkillBuildingIds.push_back(buildingId);
		_usedSkillTicks.push_back(Time::Ticks());
	}
	bool HasSpeedBoost(int32 buildingId) {
		for (size_t i = 0; i < _usedSkillEnums.size(); i++) {
			if (_usedSkillEnums[i] == CardEnum::SpeedBoost && _usedSkillBuildingIds[i] == buildingId)  {
				return true;
			}
		}
		return false;
	}

	bool IsInDarkAge() { return _isInDarkAge; }

	bool GetHouseResourceAllow(ResourceEnum resourceEnum) {
		return _houseResourceAllowed[static_cast<int>(resourceEnum)];
	}
	void SetHouseResourceAllow(ResourceEnum resourceEnum, bool resourceAllowed);

	/*
	 * Migration
	 */
	void AddMigrationPendingCount(int32 count) {
		_migrationPendingCount += count;
	}

	/*
	 * Serialize
	 */
	void Serialize(FArchive& Ar)
	{
		//_LOG(PunPlayerOwned, "Serialize[%d] Before isSaving:%d, %d %d %d", _playerId, Ar.IsSaving(), _roadConstructionIds.size(), _constructionIds.size(), _jobBuildingEnumToIds.size());
		
		/*
		 * Public
		 */
		Ar << needChooseLocation;
		Ar << justChoseLocation;
		Ar << townHallId;
		initialResources.Serialize(Ar);

		SerializeVecValue(Ar, incomes100);
		SerializeVecValue(Ar, sciences100);
		SerializeVecValue(Ar, influenceIncomes100);

		Ar << economicVictoryPhase;
		
		// Influence
		//Ar << influencePerRound;

		// Tax Level
		Ar << taxLevel;
		Ar << vassalTaxLevel;

		Ar << targetLaborerHighPriority;
		Ar << targetBuilderHighPriority;
		Ar << targetRoadMakerHighPriority;
		Ar << targetLaborerCount;
		Ar << targetBuilderCount;
		Ar << targetRoadMakerCount;

		/*
		 * Private
		 */
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
		

		Ar << _employedCount;
		Ar << _builderCount;
		SerializeVecValue(Ar, _roadMakerIds);

		Ar << _aveFoodHappiness;
		Ar << _aveHeatHappiness;
		Ar << _aveHousingHappiness;
		Ar << _aveFunHappiness;

		SerializeVecValue(Ar, _aveHappinessModifiers);

		SerializeVecValue(Ar, _provincesClaimed);
		_territoryBoxExtent >> Ar;
		SerializeVecObj(Ar, _armyRegionClaimQueue);
		SerializeVecObj(Ar, _armyRegionClaimCanceled);
		_armyAutoClaimProgress >> Ar;

		SerializeVecValue(Ar, _attackingProvinceIds);
		SerializeVecObj(Ar, _defendingClaimProgress);
		

		// Vassal/Annex
		Ar << _lordPlayerId;
		SerializeVecValue(Ar, _vassalBuildingIds);
		SerializeVecValue(Ar, _allyPlayerIds);
		SerializeVecValue(Ar, _armyNodesVisited);
		//_battle.Serialize(Ar);

		// Stats
		SerializeVecVecValue(Ar, _enumToStatVec);


		SerializeVecValue(Ar, _buffTicksLeft);

		Ar << _spTicks;
		Ar << _maxSPTicks;
		Ar << _currentSkill;

		Ar << _isInDarkAge;
		Ar << _nextDiseaseCheckTick;

		SerializeVecValue(Ar, _usedSkillEnums);
		SerializeVecValue(Ar, _usedSkillBuildingIds);
		SerializeVecValue(Ar, _usedSkillTicks);

		SerializeVecValue(Ar, _houseResourceAllowed);

		Ar << _migrationPendingCount;

		//_LOG(PunPlayerOwned, "Serialize[%d] After isSaving:%d, %d %d %d", _playerId, Ar.IsSaving(), _roadConstructionIds.size(), _constructionIds.size(), _jobBuildingEnumToIds.size());
	}

	// Public NonSerial
	bool alreadyDidGatherMark = false;
	
private:

	void RemoveJobsFromBuildings(const std::vector<int32_t>& buildingIds) {
		for (int32_t buildingId : buildingIds) {
			_simulation->RemoveJobsFrom(buildingId, true);
		}
	}

	int32 TryFillJobBuildings(const std::vector<int32>& jobBuildingIds, PriorityEnum priority, int& index, bool shouldDoDistanceSort = true, int32 maximumFill = INT32_MAX);

	void CollectRoundIncome();
	void CollectHouseIncome();

	bool CheckDisbandArmy();

	int32 targetNonLaborerCount()
	{
		if (targetLaborerHighPriority) {
			return _adultIds.size() - targetLaborerCount;
		}
		return _adultIds.size();
	}

	void AddDataPoint(PlotStatEnum statEnum, int32 data)
	{
		std::vector<int32>& statVec = _enumToStatVec[static_cast<int>(statEnum)];
		int32 statTickCount = Time::Ticks() / TicksPerStatInterval;

		//while (statVec.size() < statTickCount) {
		for (int32 i = statVec.size(); i < statTickCount; i++) {
			statVec.push_back(0); // PlayerOwned might not be initialized from the first round...
		}
		
		statVec.push_back(data);
	};
	
public:
	bool needChooseLocation = true;
	bool justChoseLocation = false;
	int32 townHallId = -1;
	FChooseInitialResources initialResources;

	std::vector<int32> incomes100;
	std::vector<int32> sciences100;
	std::vector<int32> influenceIncomes100;

	int32 economicVictoryPhase = 0;

	// Tax Level
	int32 taxLevel = 2;
	int32 vassalTaxLevel = 2;

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

private:
	/*
	 * Serialize
	 */
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

	//

	int32 _employedCount = 0;
	int32 _builderCount = 0;
	std::vector<int32> _roadMakerIds;
	
	int32 _aveFoodHappiness = 0;
	int32 _aveHeatHappiness = 0;
	int32 _aveHousingHappiness = 0;
	int32 _aveFunHappiness = 0;
	
	//int32_t _aveLuxuryHappinessModifier = 0;
	std::vector<int32> _aveHappinessModifiers;
	
	std::vector<int32> _provincesClaimed;
	//std::vector<int32> _provincesInfluenced; // TODO: Serialize
	//std::vector<int32> _provincesOutpost; // TODO: Serialize
	TileArea _territoryBoxExtent;
	
	std::vector<RegionClaimProgress> _armyRegionClaimQueue;
	std::vector<RegionClaimProgress> _armyRegionClaimCanceled; // Canceled regions's progress are still kept to be continued
	RegionClaimProgress _armyAutoClaimProgress;

	std::vector<int32> _attackingProvinceIds;
	std::vector<ProvinceClaimProgress> _defendingClaimProgress;

	// Vassal/Annex
	int32 _lordPlayerId = -1;
	std::vector<int32> _vassalBuildingIds;
	std::vector<int32> _allyPlayerIds;
	std::vector<int32> _armyNodesVisited; // Used to loop to find total army...

	// 
	std::vector<std::vector<int32>> _enumToStatVec;

	// Buffs
	std::vector<int32> _buffTicksLeft;

	// SP
	static int32 ticksPerSP() { return Time::TicksPerSecond * 2; }
	int32 _spTicks;
	int32 _maxSPTicks;
	CardEnum _currentSkill;

	static const int32 MaxSP = 240;
	

	std::vector<CardEnum> _usedSkillEnums;
	std::vector<int32> _usedSkillBuildingIds;
	std::vector<int32> _usedSkillTicks;

	bool _isInDarkAge;
	int32 _nextDiseaseCheckTick;

	std::vector<uint8> _houseResourceAllowed;

	int32 _migrationPendingCount = 0;

private:
	IGameSimulationCore* _simulation;
	int32 _playerId;
};
