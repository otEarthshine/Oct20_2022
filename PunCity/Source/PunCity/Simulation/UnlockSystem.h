// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "IGameSimulationCore.h"
#include "PunCity/CppUtils.h"
#include <unordered_map>
#include "PunCity/PunUtils.h"
#include "Buildings/House.h"

#define LOCTEXT_NAMESPACE "UnlockSys"

const std::vector<FText> eraNumberToText =
{
	INVTEXT(""),
	INVTEXT("I"),
	INVTEXT("II"),
	INVTEXT("III"),
	INVTEXT("IV"),
	INVTEXT("V"),
	INVTEXT("VI"),
	INVTEXT("VII"),
	INVTEXT("VIII"),
	INVTEXT("IX"),
	INVTEXT("X"),
};

struct TechBoxLocation
{
	int32 column = -1;
	int32 row = -1;

	TechBoxLocation() {}
	TechBoxLocation(int32 column, int32 row) : column(column), row(row) {}

	bool isValid() { return column != -1; }

	static const int32 MaxRows = 10;
	int32 id() { return column * MaxRows + row; }

	//! Serialize
	void operator>>(FArchive &Ar) {
		Ar << column << row;
	}
};

/*
 * For research with custom name (that isn't building name etc.)
 * - [0] is name
 * - [1] is bonus description
 * Note: Just like Civ 6, all research can only contain one bonus.
 */
static const std::unordered_map<TechEnum, std::vector<FText>> ResearchName_BonusDescription =
{
	{ TechEnum::MiddleAge, {
		LOCTEXT("Middle Age", "Middle Age"),
		LOCTEXT("Middle Age Desc", "Unlocks Townhall Level 2"),
	} },
	{ TechEnum::EnlightenmentAge, {
		LOCTEXT("Enlightenment Age", "Enlightenment Age"),
		LOCTEXT("Enlightenment Age Desc", "Unlocks Townhall Level 3"),
	} },
	{ TechEnum::IndustrialAge, {
		LOCTEXT("Industrial Age", "Industrial Age"),
		LOCTEXT("Industrial Age Desc", "Unlocks Townhall Level 4"),
	} },
	
	{ TechEnum::DeepMining, { LOCTEXT("Deep Mining", "Deep Mining") }},

	{ TechEnum::IronRefining, { LOCTEXT("Ironworks", "Ironworks") }},
	{ TechEnum::GoldRefining, { LOCTEXT("Goldworks", "Goldworks") }},

	{ TechEnum::HerbFarming , {LOCTEXT("Basic Medicine", "Basic Medicine")}},


	{ TechEnum::Logistics1, { LOCTEXT("Logistics I", "Logistics I") }},
	{ TechEnum::Logistics2, { LOCTEXT("Logistics II", "Logistics II") }},
	{ TechEnum::Logistics3, { LOCTEXT("Logistics III", "Logistics III") }},
	{ TechEnum::Logistics4, { LOCTEXT("Logistics IV", "Logistics IV") }},
	{ TechEnum::Logistics5, {
		LOCTEXT("Logistics V", "Logistics V"),
		LOCTEXT("Logistics V Desc", "Allows Citizens to carry +10 more goods."),
	} },

	{TechEnum::JewelryCrafting, { LOCTEXT("Jewelry Crafting", "Jewelry Crafting") }},
	{TechEnum::Baking, { LOCTEXT("Baking", "Baking") }},

	{TechEnum::Fence, { LOCTEXT("Fence", "Fence") }},

	{TechEnum::RerollCardsPlus1, {
		LOCTEXT("Ideation", "Ideation"),
		LOCTEXT("Ideation Desc", "+1 card each reroll.")
	}},

	{TechEnum::FarmImprovement, {
		LOCTEXT("Farm Improvements", "Farm Improvements"),
		LOCTEXT("Farm Improvements Desc", "+5% farm production.")
	}},

	//{TechEnum::FarmImprovement, {
	//	"Centralization",
	//	"Townhall level 2+ gain +20 culture per round."
	//}},

	{TechEnum::BeerBrewery, { LOCTEXT("Beer Brewing", "Beer Brewing") }},
	{TechEnum::Pottery, { LOCTEXT("Pottery", "Pottery") }},

	{TechEnum::FurnitureWorkshop, { LOCTEXT("Wood Industry", "Wood Industry") }},

	{TechEnum::AgriculturalRevolution, {
		LOCTEXT("Agricultural Revolution", "Agricultural Revolution"),
		LOCTEXT("Agricultural Revolution Desc", "Farms get +10% efficiency for each nearby farm. (max at 20%)")
	}},
	{TechEnum::Industrialization, {
		LOCTEXT("Industrialization", "Industrialization"),
		LOCTEXT("Industrialization Desc", "+20% Industrial Output")
	}},

	{TechEnum::IndustrialAdjacency, {
		LOCTEXT("Industrial Adjacency", "Industrial Adjacency"),
		LOCTEXT("Industrial Adjacency Desc", "Industrial Buildings get +5% efficiency for each nearby industry of the same type. (max at 15%)")
	}},
	
	{TechEnum::HouseLvl6Income, {
		LOCTEXT("House Level 6 Income", "House Level 6 Income"),
		LOCTEXT("House Level 6 Income Desc", "House level 6+ gain +10 income.")
	}},
	{TechEnum::TaxAdjustment, {
		LOCTEXT("Tax Adjustment", "Tax Adjustment"),
		LOCTEXT("Tax Adjustment Desc", "Allows tax adjustment from townhall.")
	}},

	{TechEnum::TradingPost, {
		LOCTEXT("Foreign Trade", "Foreign Trade"),
		LOCTEXT("Foreign Trade Desc", "Increases the number of immigrants by 20%.")
	}},
	
	{ TechEnum::TraderDiscount, {
		LOCTEXT("Trader Discount", "Trader Discount"),
		LOCTEXT("Trader Discount Desc", "Trading company adjacent to trading port gain -5% trading fee."),
	}},
	// TODO: BuildingComboEnum?? Encourage ppl to build next building... holes to fill...

	{ TechEnum::Espionage, {
		LOCTEXT("Espionage", "Espionage")
	}},
	{ TechEnum::SpyGuard, {
		LOCTEXT("Spy Guard", "Spy Guard")
	}},

	{ TechEnum::Colony, {
		LOCTEXT("Colonization", "Colonization"),
		//LOCTEXT("Colonization Desc", "Unlock the ability to build new Cities."),
	}},
	{ TechEnum::IntercityLogistics, {
		LOCTEXT("Intercity Logistics", "Intercity Logistics"),
	}},
	
	//{TechEnum::ShrineRot, {
	//	LOCTEXT("Unlock Shrines",
	//}},

	//{TechEnum::CropStudy, {
	//	LOCTEXT("Crop Variety",
	//	LOCTEXT("aaaa",
	//}},

	//{TechEnum::CityToCityTrade, {
	//	LOCTEXT("City-to-city Trade",
	//	LOCTEXT("Allow player to set trade offers for other players to directly trade with.",
	//}},

	{ TechEnum::Plantation, {
		LOCTEXT("Plantation", "Plantation"),
		LOCTEXT("Plantation Desc", "Unlocks farm crops cards: Cannabis, Grape, Cocoa, Cotton, Coffee, Tulip Seeds")
	}},
	
	{ TechEnum::MushroomSubstrateSterilization, {
		LOCTEXT("Mushroom Log Sterilization", "Mushroom Log Sterilization"),
		LOCTEXT("Mushroom Log Sterilization Desc", "Decreases mushroom farm's wood consumption by 50%.")
	}},
	
	{ TechEnum::QuarryImprovement, {
		LOCTEXT("Quarry Improvement", "Quarry Improvement"),
		LOCTEXT("Quarry Improvement Desc", "+30% production to quarries.")
	}},

	{ TechEnum::WineryImprovement, {
		LOCTEXT("Wine Snob", "Wine Snob"),
		LOCTEXT("Wine Snob Desc", "+30% production to winery.")
	}},

	{ TechEnum::BorealLandCost, {
		LOCTEXT("Boreal and Tundra Expedition", "Boreal and Tundra Expedition"),
		LOCTEXT("Boreal and Tundra Expedition Desc", "Claiming Boreal Forest and Tundra Province costs half as usual.")
	} },
	{ TechEnum::DesertTrade, {
		LOCTEXT("Silk Road", "Silk Road"),
		LOCTEXT("Silk Road Desc", "Trading post and company built on desert gets -10% trade fee.")
	} },
	
	{ TechEnum::ShallowWaterEmbark, {
		LOCTEXT("Shallow Water Embark", "Shallow Water Embark"),
		LOCTEXT("Shallow Water Embark Desc", "Allows claiming bordering provinces across a body of water. 100% cost penalty applied to claim/attack through shallow water.")
	}},
	{ TechEnum::DeepWaterEmbark, {
		LOCTEXT("Deepwater Embark", "Deepwater Embark"),
		LOCTEXT("Deepwater Embark Desc", "Allows claiming coastal provinces across the sea. 200% cost penalty applied to claim/attack through deep water.")
	}},

	{ TechEnum::Sawmill, {
		LOCTEXT("Sawmill", "Sawmill"),
		LOCTEXT("Sawmill Desc", "+50% furniture workshop's efficiency.")
	}},
	{ TechEnum::ImprovedWoodCutting, {
		LOCTEXT("Improved woodcutting", "Improved woodcutting"),
		LOCTEXT("Improved woodcutting Desc", "+50% wood harvesting yield."),
	}},
	{ TechEnum::ImprovedWoodCutting2, {
		LOCTEXT("Improved woodcutting Lvl 2", "Improved woodcutting Lvl 2"),
		LOCTEXT("Improved woodcutting Lvl 2 Desc", "+20% wood harvesting yield.")
	}},
	{TechEnum::FarmingBreakthrough, {
		LOCTEXT("Improved farming", "Improved farming"),
		LOCTEXT("Improved farming Desc", "+20% farm production.")
	}},
	{TechEnum::CheapLand, {
		LOCTEXT("Cheap Land", "Cheap Land"),
		LOCTEXT("Cheap Land Desc", "Claiming land costs 30% less.")
	}},
	{TechEnum::CheapReroll, {
		LOCTEXT("Cheap Reroll", "Cheap Reroll"),
		LOCTEXT("Cheap Reroll Desc", "Rerolls cost half as much.")
	}},

	{ TechEnum::QuickBuild, {
		LOCTEXT("Quick Build", "Quick Build"),
		LOCTEXT("Quick Build Desc", "Allows speeding up construction with money.")
	} },

	{ TechEnum::CityManagementI, {
		LOCTEXT("City Management I", "City Management I"),
		LOCTEXT("City Management I Desc", "......."),
	} },
	
	//{TechEnum::ClaimLandByFood, {
	//	"Expeditionary supply",
	//	"Allows claiming land by food",
	//}},


	{TechEnum::FireStarter, {
		LOCTEXT("Offensive Fire", "Offensive Fire")
	}},
	{TechEnum::MoreGoldPerHouse, {
		LOCTEXT("Extra House Income", "Extra House Income"),
		LOCTEXT("Extra House Income Desc", "+<img id=\"Coin\"/>3 house income")
	}},

	{TechEnum::HouseAdjacency, {
		LOCTEXT("House Adjacency", "House Adjacency"),
		LOCTEXT("House Adjacency Desc", "Houses get +<img id=\"Coin\"/>1 for each nearby house. (max at +<img id=\"Coin\"/>3)"),
	}},

	{ TechEnum::BudgetAdjustment, {
		LOCTEXT("BudgetAdjustment", "Budget Adjustment"),
		LOCTEXT("BudgetAdjustment Desc", "Unlocks the ability to adjust Workplace's Budget Level.\n\nIncreasing the Budget Level lead to higher Effectiveness and Job Happiness, but also increases the Building Upkeep."),
	} },
	{ TechEnum::WorkSchedule, {
		LOCTEXT("WorkSchedule", "Work Schedule"),
		LOCTEXT("WorkSchedule Desc", "Unlocks the ability to adjust Workplace's Work Hours Level.\n\nIncreasing the Work Hours Level lead to higher Effectiveness, but lower Job Happiness."),
	} },

	{ TechEnum::ScientificTheories, {
		LOCTEXT("Scientific Theories", "Scientific Theories"),
		LOCTEXT("Scientific Theories Desc", "+20% Science Output from Houses"),
	}},
	{ TechEnum::EconomicTheories, {
		LOCTEXT("Economics Theories", "Economics Theories"),
		LOCTEXT("Economics Theories Desc", "+20% House Income"),
	}},
	{ TechEnum::SocialScience, {
		LOCTEXT("Social Science", "Social Science"),
		LOCTEXT("Social Science Desc", "Allows production of Motivation and Passion Cards at Scholars Office."),
	}},
	
	{ TechEnum::Fertilizers, {
		LOCTEXT("Fertilizers", "Fertilizers"),
		LOCTEXT("Fertilizers Desc", "+20% Farm Productivity"),
	}},
	{ TechEnum::MilitaryLastEra, {
		LOCTEXT("Advanced Military", "Advanced Military"),
		LOCTEXT("Advanced Military Desc", "+100% influence points from Barracks"),
	}},

	/*
	 * Prosperity Tech
	 */
	{ TechEnum::Rationalism, {
		LOCTEXT("Rationalism", "Rationalism"),
		LOCTEXT("Rationalism Desc", "+30% Science Output"),
	}},

	{ TechEnum::InfluencePoints, {
		LOCTEXT("Influence Points", "Influence Points"),
		LOCTEXT("Influence Points Desc", "Unlock Influence Points <img id=\"Influence\"/> used to claim land."),
	}},

	{ TechEnum::Conquer, {
		LOCTEXT("Conquer Province", "Conquer Province"),
		LOCTEXT("Conquer Province Desc", "Unlock Province Conquering"),
	}},

	{ TechEnum::HomeLandDefense, {
		LOCTEXT("Homeland Defense", "Homeland Defense"),
		LOCTEXT("Homeland Defense Desc", "Provinces gain +10% defense bonus for each building on it."),
	}},

	{ TechEnum::Vassalize, {
		LOCTEXT("Vassalize", "Vassalize"),
		LOCTEXT("Vassalize Desc", "Unlock Vassalization which allows you to turn other city into a vassal."),
	}},

	{ TechEnum::IntercityRoad, {
		LOCTEXT("Intercity Connection", "Intercity Connection"),
		LOCTEXT("Intercity Connection Desc", "Allow connecting Cities with Intercity Road to establish trade connections"),
	}},

	{ TechEnum::Combo, {
		LOCTEXT("Building Combo", "Building Combo"),
		LOCTEXT("Building Combo Desc", "Gain Combo level 1,2,3 by constructing 2,4,8 buildings of the same type."),
	} },
};

enum class TechClassEnum
{
	ResearchInfo,
	ResearchNone,
	Building_Research,
	MoreBuilding_Research,
	BonusToggle_Research,
};


class ResearchInfo
{
public:
	virtual ~ResearchInfo() {}

	void Init(int32 eraIn, TechEnum researchEnumIn, std::vector<TechEnum> prerequisites) {
		column = eraIn;
		techEnum = researchEnumIn;
		_prerequisites = prerequisites;
	}

	bool HasCustomName() {
		auto found = ResearchName_BonusDescription.find(techEnum);
		return found != ResearchName_BonusDescription.end();
	}
	bool HasBonus() {
		auto found = ResearchName_BonusDescription.find(techEnum);
		return found != ResearchName_BonusDescription.end() && 
			found->second.size() >= 2;
	}
	
	virtual FText GetName() {
		auto found = ResearchName_BonusDescription.find(techEnum);
		PUN_CHECK(found != ResearchName_BonusDescription.end());
		return found->second[0];
	}

	virtual FText GetBonusDescription() {
		auto found = ResearchName_BonusDescription.find(techEnum);
		PUN_CHECK(found != ResearchName_BonusDescription.end());
		if (found->second.size() >= 2) {
			return found->second[1];
		}
		return FText();
	}

	virtual std::vector<CardEnum> GetUnlockNames() {
		return {};
	}

	std::vector<TechEnum> prerequisites() { return _prerequisites; }

	virtual void OnUnlock(int32 playerId, IGameSimulationCore* simulation)
	{

	}

	// Helpers
	int64 scienceNeeded(int64 techsFinished) {
		/*
		 * After 40 techs
		 * - 1250 cost ... (techsFinished + 10) * (techsFinished + 10) * (techsFinished + 10) / 10 / 10
		 * - 50 * 5 * 5 * 5
		 * - at 125 sci production... it is 2.5 seasons
		 *
		 * Tech scaling affected by:
		 * - population growth
		 * - house upgrades
		 *
		 * - House Lvl 7 sci = 30
		 *
		 * Expected population in each age:
		 * - 
		 * 
		 */
		int64 sciNeeded = (techsFinished + 10LL) * (techsFinished + 10LL) * (techsFinished + 10LL) * (techsFinished + 10LL) / 10LL / 10LL / 10LL  /* Tech factor: */  * 12000LL / 1000LL; // 5400 / 1000;
		return std::min(sciNeeded, 100000LL);
	}

	float researchFraction(int32 researchesFinished, int64 science100XsecPerRound) {
		return science100XsecPerRound / (100.0f * scienceNeeded(researchesFinished) * Time::SecondsPerRound);
	}

	virtual TechClassEnum classEnum() { return TechClassEnum::ResearchInfo; }
	virtual void Serialize(FArchive& Ar, IGameSimulationCore* simulation)
	{
		Ar << column;
		Ar << techEnum;
		Ar << state;

		SerializeVecValue(Ar, _prerequisites);

		//Ar << science100XsecPerRound;

		SerializeVecValue(Ar, _buildingEnums);
		SerializeVecValue(Ar, _permanentBuildingEnums);

		Ar << requiredResourceEnum;
		Ar << requiredResourceCount;
	}
	
public:
	int32 column = 0;
	TechEnum techEnum = TechEnum::None;
	TechStateEnum state = TechStateEnum::Locked;

	std::vector<TechEnum> _prerequisites;

	//int32 science100XsecPerRound = 0;
	
	std::vector<CardEnum> _buildingEnums;
	std::vector<CardEnum> _permanentBuildingEnums;


	ResourceEnum requiredResourceEnum = ResourceEnum::None;
	int32 requiredResourceCount = 0;
};

class ResearchNone final : public ResearchInfo
{
public:
	FText GetName() override {
		return LOCTEXT("Choose Research", "Choose Research");
	}

	FText GetBonusDescription() override {
		return LOCTEXT("ResearchNone_Desc", "Click here to choose a research task");
	}
	
	void OnUnlock(int32 playerId, IGameSimulationCore* simulation) final {
		UE_DEBUG_BREAK();
	}

	TechClassEnum classEnum() override { return TechClassEnum::ResearchNone; }
};

class Building_Research : public ResearchInfo
{
public:
	virtual ~Building_Research(){}

	void InitBuildingResearch(std::vector<CardEnum> buildingEnums, std::vector<CardEnum> permanentBuildingEnums, IGameSimulationCore* simulation, int32_t cardCount = -1);

	FText GetName() override {
		if (HasCustomName()) {
			return ResearchInfo::GetName();
		}
		if (_buildingEnums.size() > 0) {
			return GetBuildingInfo(_buildingEnums[0]).name;
		}
		return GetBuildingInfo(_permanentBuildingEnums[0]).name;
	}

	std::vector<CardEnum> GetUnlockNames() override {
		std::vector<CardEnum> results;
		for (CardEnum buildingEnum : _buildingEnums) {
			results.push_back(buildingEnum);
		}
		for (CardEnum buildingEnum : _permanentBuildingEnums) {
			results.push_back(buildingEnum);
		}
		return results;
	}
	

	void OnUnlock(int32 playerId, IGameSimulationCore* simulation) override;

	TechClassEnum classEnum() override { return TechClassEnum::Building_Research; }
	void Serialize(FArchive& Ar, IGameSimulationCore* simulation) override
	{
		ResearchInfo::Serialize(Ar, simulation);
		Ar << _cardCount;

		if (Ar.IsLoading()) {
			_simulation = simulation;
		}
	}
	
public:
	IGameSimulationCore* _simulation;
	int32 _cardCount = -1;
};

class MoreBuilding_Research final : public Building_Research
{
public:
	virtual ~MoreBuilding_Research() {}

	//std::string Name() override { return ResearchInfo::Name(); }
	//std::string Description() override { return ResearchInfo::Description(); }

	TechClassEnum classEnum() override { return TechClassEnum::MoreBuilding_Research; }
};

class BonusToggle_Research : public ResearchInfo 
{
public:
	virtual ~BonusToggle_Research() {}

	TechClassEnum classEnum() override { return TechClassEnum::BonusToggle_Research; }

	
	void OnUnlock(int32 playerId, IGameSimulationCore* simulation) final;
};

//class 

/**
 * 
 */
class UnlockSystem
{
public:
	/*
	 * Tech UI
	 */
	void AddTech(int32 columnIndex, std::shared_ptr<ResearchInfo> tech) {
		PUN_CHECK(_enumToTech.find(tech->techEnum) == _enumToTech.end());
		if (columnIndex >= _columnToTechs.size()) {
			_columnToTechs.push_back({});
		}

		_columnToTechs[columnIndex].push_back(tech->techEnum);
		_enumToTech[tech->techEnum] = std::static_pointer_cast<ResearchInfo>(tech);
	}

	void AddTech_Building(int32 era, TechEnum researchEnum, std::vector<TechEnum> prerequisites,
		CardEnum buildingEnum, int32 cardCount = 1)
	{
		auto tech = std::make_shared<Building_Research>();
		tech->Init(era, researchEnum, prerequisites);
		tech->InitBuildingResearch({ buildingEnum }, {}, _simulation, cardCount);
		AddTech(era, tech);
	}
	void AddTech_Building(int32 era, TechEnum researchEnum, std::vector<TechEnum> prerequisites,
		std::vector<CardEnum> buildingEnums, int32_t cardCount = 1)
	{
		auto tech = std::make_shared<Building_Research>();
		tech->Init(era, researchEnum, prerequisites);
		tech->InitBuildingResearch(buildingEnums, {}, _simulation, cardCount);
		AddTech(era, tech);
	}
	void AddTech_Building(int32 era, TechEnum researchEnum, std::vector<TechEnum> prerequisites,
		CardEnum buildingEnum, ResourceEnum requiredResourceEnum, int32 requiredResourceCount)
	{
		auto tech = std::make_shared<Building_Research>();
		tech->Init(era, researchEnum, prerequisites);
		tech->InitBuildingResearch({ buildingEnum }, {}, _simulation, 1);
		tech->requiredResourceEnum = requiredResourceEnum;
		tech->requiredResourceCount = requiredResourceCount;
		AddTech(era, tech);
	}

	void AddTech_BuildingPermanent(int32 era, TechEnum researchEnum, std::vector<TechEnum> prerequisites,
		std::vector<CardEnum> buildingEnums)
	{
		auto tech = std::make_shared<Building_Research>();
		tech->Init(era, researchEnum, prerequisites);
		tech->InitBuildingResearch({}, buildingEnums, _simulation);
		auto techCasted = std::static_pointer_cast<ResearchInfo>(tech);
		AddTech(era, tech);
	}

	void AddTech_Bonus(int32 era, TechEnum researchEnum, std::vector<TechEnum> prerequisites)
	{
		auto tech = std::make_shared<BonusToggle_Research>();
		tech->Init(era, researchEnum, prerequisites);
		AddTech(era, tech);
	}

	/*
	 * Prosperity UI
	 */
	void AddProsperityTech(int32 houseLevel, int32 unlockCount, std::shared_ptr<ResearchInfo> tech)
	{	
		PUN_CHECK(_enumToTech.find(tech->techEnum) == _enumToTech.end());
		
		while (houseLevel >= _houseLvlToProsperityTechEnum.size()) {
			_houseLvlToProsperityTechEnum.push_back({});
			_houseLvlToUnlockCount.push_back({});
		}

		int32 cumulativeUnlockCount = unlockCount;
		int32 unlockListSize = _houseLvlToUnlockCount[houseLevel].size();
		if (unlockListSize > 0) {
			cumulativeUnlockCount += _houseLvlToUnlockCount[houseLevel][unlockListSize - 1];
		}
		
		_houseLvlToProsperityTechEnum[houseLevel].push_back(tech->techEnum);
		_houseLvlToUnlockCount[houseLevel].push_back(cumulativeUnlockCount);
		_enumToTech[tech->techEnum] = std::static_pointer_cast<ResearchInfo>(tech);
	}
	
	void AddProsperityTech_Building(int32 era, int32 unlockCount, TechEnum researchEnum, CardEnum buildingEnum, int32 cardCount = 1) {
		AddProsperityTech_BuildingX(era, unlockCount, researchEnum, { buildingEnum }, cardCount);
	}
	void AddProsperityTech_BuildingX(int32 era, int32 unlockCount, TechEnum researchEnum, std::vector<CardEnum> buildingEnums, int32 cardCount = 1) {
		auto tech = std::make_shared<Building_Research>();
		tech->Init(era, researchEnum, {});
		tech->InitBuildingResearch(buildingEnums, {}, _simulation, cardCount);
		AddProsperityTech(era, unlockCount, tech);
	}

	void AddProsperityTech_BuildingPermanent(int32 era, int32 unlockCount, TechEnum researchEnum, std::vector<CardEnum> buildingEnums) {
		auto tech = std::make_shared<Building_Research>();
		tech->Init(era, researchEnum, {});
		tech->InitBuildingResearch({}, buildingEnums, _simulation);
		auto techCasted = std::static_pointer_cast<ResearchInfo>(tech);
		AddProsperityTech(era, unlockCount, tech);
	}

	void AddProsperityTech_Bonus(int32 era, int32 unlockCount, TechEnum researchEnum) {
		auto tech = std::make_shared<BonusToggle_Research>();
		tech->Init(era, researchEnum, {});
		AddProsperityTech(era, unlockCount, tech);
	}

	const std::vector<std::vector<TechEnum>>& houseLvlToProsperityTechEnum() {
		return _houseLvlToProsperityTechEnum;
	}
	const std::vector<std::vector<int32>>& houseLvlToUnlockCounts() { return _houseLvlToUnlockCount; }

	TechEnum GetProsperityTechEnum(int32 houseLvl, int32 localIndex) {
		return _houseLvlToProsperityTechEnum[houseLvl][localIndex];
	}
	
	/*
	 * Constructor
	 */
	UnlockSystem(int32 playerId, IGameSimulationCore* simulation)
	{
		_simulation = simulation;
		_playerId = playerId;

		if (!_simulation->isLoadingFromFile())
		{
			researchEnabled = false;
			techsFinished = 0;

			prosperityEnabled = false;
			
			UnlockBuilding(CardEnum::DirtRoad);
			UnlockBuilding(CardEnum::House);
			UnlockBuilding(CardEnum::StorageYard);
			//UnlockBuilding(BuildingEnum::Fence);
			//UnlockBuilding(BuildingEnum::FenceGate);

			if (PunSettings::TrailerSession) {
				UnlockAll();
			}

			_resourceEnumToProductionCount.resize(ResourceEnumCount, 0);

			/*
			 * Tech UI
			 */
			_columnToTechs.push_back({}); // Era 0 as blank
			_enumToTech[TechEnum::None] = std::static_pointer_cast<ResearchInfo>(std::make_shared<ResearchNone>());


			// TODO: May be separate era into:
			// - 3 major unlocks
			// - 4 bonuses where 2 can be chosen?
			//
			//To change:
			// BorealLandCost
			// RerollCardsPlus1
			// SpyGuard??
			// CheapReroll
			// Sawmill, ImprovedWoodCutting
			// HouseAdjacency
			//
			//Can add bonus
			// Market
			// Steal
			//

			int32 column = -1; // Unused stuff
			//AddTech_Bonus(column, TechEnum::Plantation);
			//AddTech_Bonus(column, TechEnum::DesertTrade);
			//AddTech_Bonus(column, TechEnum::Sawmill);
			//AddTech_Bonus(column, TechEnum::ImprovedWoodCutting2);
			//AddTech_Building(column, TechEnum::BarrackKnight, CardEnum::BarrackSwordman);
			//AddTech_Building(column, TechEnum::IntercityLogistics, { CardEnum::IntercityLogisticsHub, CardEnum::IntercityLogisticsPort });

			/*
			AddTech_Building(column, TechEnum::Espionage, { CardEnum::Steal });
			AddTech_Bonus(column, TechEnum::QuarryImprovement);
			AddTech_Bonus(column, TechEnum::HouseAdjacency);
			AddTech_Bonus(column, TechEnum::ShallowWaterEmbark);
			AddTech_Building(column, TechEnum::SpyGuard, { CardEnum::KidnapGuard, CardEnum::TreasuryGuard });
			AddTech_Bonus(column, TechEnum::TaxAdjustment);
			AddTech_Building(column, TechEnum::Garden, CardEnum::Garden);
			AddTech_Bonus(column, TechEnum::WineryImprovement);
			AddTech_Bonus(column, TechEnum::TraderDiscount);
			*/
			
			
			//
			column = 1;
			AddTech_Building(column, TechEnum::TradingPost, {},
				{ CardEnum::TradingPost, CardEnum::TradingPort }
			);
			AddTech_Building(column, TechEnum::HerbFarming, {},
				{ CardEnum::HerbSeed }
			);
			
			
			//
			column = 2;
			AddTech_Building(column, TechEnum::StoneToolsShop, { TechEnum::TradingPost },
				CardEnum::StoneToolsShop
			);
			AddTech_Building(column, TechEnum::Pottery, { TechEnum::TradingPost },
				{ CardEnum::Potter, CardEnum::ClayPit }
			);
			AddTech_Building(column, TechEnum::FurnitureWorkshop, { TechEnum::TradingPost },
				CardEnum::FurnitureWorkshop
			);
			AddTech_Building(column, TechEnum::BeerBrewery, { TechEnum::TradingPost },
				CardEnum::BeerBrewery
			);

			AddTech_Building(column, TechEnum::RanchSheep, { TechEnum::HerbFarming },
				{ CardEnum::RanchSheep }
			);
			AddTech_Bonus(column, TechEnum::MushroomSubstrateSterilization, { TechEnum::HerbFarming });
			
			//
			column = 3;
			AddTech_Bonus(column, TechEnum::MiddleAge, {});
			AddTech_Building(column, TechEnum::IronRefining, { TechEnum::StoneToolsShop, TechEnum::Pottery },
				{ CardEnum::IronSmelter, CardEnum::BarrackSwordman }
			);
			AddTech_Building(column, TechEnum::BrickMaking, { TechEnum::Pottery },
				{ CardEnum::Brickworks }
			);
			AddTech_Building(column, TechEnum::Library, { TechEnum::FurnitureWorkshop, TechEnum::BeerBrewery },
				CardEnum::Library
			);
			AddTech_Building(column, TechEnum::Logistics1, {},
				CardEnum::HaulingServices
			);
			AddTech_Building(column, TechEnum::AgriculturalRevolution, { TechEnum::BeerBrewery, TechEnum::RanchSheep, TechEnum::MushroomSubstrateSterilization },
				CardEnum::Granary
			);
			AddTech_Building(column, TechEnum::PotatoFarming, { TechEnum::MushroomSubstrateSterilization },
				{ CardEnum::PotatoSeed }
			);
			
			//
			column = 4;
			AddTech_Building(column, TechEnum::Tailor, { TechEnum::IronRefining },
				CardEnum::Tailor
			);
			AddTech_Building(column, TechEnum::Blacksmith, { TechEnum::IronRefining, TechEnum::BrickMaking },
				CardEnum::Blacksmith
			);
			AddTech_Building(column, TechEnum::PaperMaker, { TechEnum::Library },
				CardEnum::PaperMaker
			);
			AddTech_Building(column, TechEnum::Archives, { TechEnum::Library },
				CardEnum::Archives
			);
			AddTech_Building(column, TechEnum::Logistics2, { TechEnum::Logistics1 },
				{ CardEnum::Market }
			);
			AddTech_Building(column, TechEnum::Beekeeper, { TechEnum::AgriculturalRevolution },
				CardEnum::Beekeeper
			);
			AddTech_Building(column, TechEnum::Baking, { TechEnum::AgriculturalRevolution },
				{ CardEnum::Windmill, CardEnum::Bakery }
			);
			AddTech_Building(column, TechEnum::VodkaDistillery, { TechEnum::PotatoFarming },
				{ CardEnum::VodkaDistillery }, ResourceEnum::Potato, 1000
			);
			AddTech_Building(column, TechEnum::BlueberryFarming, { TechEnum::PotatoFarming },
				{ CardEnum::BlueberrySeed }
			);
			
			//
			column = 5;
			AddTech_Bonus(column, TechEnum::QuickBuild, { TechEnum::Blacksmith });
			AddTech_Building(column, TechEnum::Medicine, { TechEnum::PaperMaker },
				CardEnum::MedicineMaker
			);
			AddTech_Building(column, TechEnum::CardMaker, { TechEnum::PaperMaker, TechEnum::Archives },
				CardEnum::CardMaker
			);
			AddTech_BuildingPermanent(column, TechEnum::Logistics3, { TechEnum::Logistics2 },
				{ CardEnum::Warehouse }
			);
			AddTech_Building(column, TechEnum::CandleMaker, { TechEnum::Beekeeper },
				CardEnum::CandleMaker
			);
			AddTech_Building(column, TechEnum::Winery, { TechEnum::Beekeeper, TechEnum::Baking },
				CardEnum::Winery
			);
			AddTech_Building(column, TechEnum::Irrigation, { TechEnum::BlueberryFarming },
				{ CardEnum::IrrigationReservoir }
			);
			
			//
			column = 6;
			AddTech_Bonus(column, TechEnum::EnlightenmentAge, {});
			AddTech_Building(column, TechEnum::Glassworks, { TechEnum::QuickBuild },
				CardEnum::Glassworks
			);
			AddTech_Building(column, TechEnum::CoffeeRoaster, { TechEnum::Medicine },
				{ CardEnum::CoffeeRoaster }
			);
			AddTech_Building(column, TechEnum::School, { TechEnum::Medicine, TechEnum::CardMaker },
				CardEnum::School
			);
			AddTech_Building(column, TechEnum::TradingCompany, { TechEnum::Logistics3 },
				CardEnum::TradingCompany
			);
			AddTech_Building(column, TechEnum::Logistics4, { TechEnum::Logistics3 },
				{ CardEnum::ShippingDepot }
			);
			AddTech_Building(column, TechEnum::PumpkinFarming, { TechEnum::Irrigation },
				{ CardEnum::PumpkinSeed }, ResourceEnum::Blueberries, 1000
			);
			
			
			//
			column = 7;
			AddTech_Bonus(column, TechEnum::WorkSchedule, { TechEnum::Glassworks, TechEnum::CoffeeRoaster });
			AddTech_Bonus(column, TechEnum::BudgetAdjustment, { TechEnum::School, TechEnum::TradingCompany });
			AddTech_Bonus(column, TechEnum::Logistics5, { TechEnum::TradingCompany, TechEnum::Logistics4 });
			AddTech_BuildingPermanent(column, TechEnum::IntercityRoad, { TechEnum::Logistics4 },
				{ CardEnum::IntercityRoad, CardEnum::IntercityBridge }
			);
			AddTech_Building(column, TechEnum::Theatre, { TechEnum::Winery },
				CardEnum::Theatre
			);
			AddTech_Building(column, TechEnum::ShroomFarm, { TechEnum::PumpkinFarming },
				{ CardEnum::ShroomFarm }, ResourceEnum::Mushroom, 3000
			);
			
				
			//
			column = 8;
			AddTech_BuildingPermanent(column, TechEnum::Tunnel, { TechEnum::WorkSchedule },
				{ CardEnum::Tunnel }
			); // TODO: Machinery + Improved Woodcutting
			AddTech_Building(column, TechEnum::Mint, { TechEnum::WorkSchedule, TechEnum::BudgetAdjustment },
				CardEnum::Mint
			);
			AddTech_Building(column, TechEnum::Bank, { TechEnum::BudgetAdjustment },
				{ CardEnum::Bank }
			);
			AddTech_BuildingPermanent(column, TechEnum::Colony, { TechEnum::Logistics5, TechEnum::IntercityRoad },
				{ CardEnum::Colony, CardEnum::PortColony }
			);
			AddTech_Building(column, TechEnum::Chocolatier, { TechEnum::Theatre },
				CardEnum::Chocolatier
			);
			AddTech_Building(column, TechEnum::RanchCow, { TechEnum::ShroomFarm },
				{ CardEnum::RanchCow }
			);

			//
			column = 9;
			AddTech_Bonus(column, TechEnum::IndustrialAge, {});
			AddTech_Building(column, TechEnum::ConcreteFactory, { TechEnum::Tunnel },
				CardEnum::ConcreteFactory
			);
			AddTech_Building(column, TechEnum::Industrialization, { TechEnum::Tunnel },
				CardEnum::CoalPowerPlant
			);
			AddTech_Building(column, TechEnum::JewelryCrafting, { TechEnum::Mint },
				{ CardEnum::GemstoneMine, CardEnum::Jeweler }
			);
			AddTech_Bonus(column, TechEnum::ScientificTheories, { TechEnum::Bank, TechEnum::Colony });
			AddTech_Bonus(column, TechEnum::Fertilizers, { TechEnum::Chocolatier, TechEnum::RanchCow });


			column = 10;
			AddTech_Building(column, TechEnum::Steelworks, { TechEnum::ConcreteFactory, TechEnum::Industrialization },
				CardEnum::Steelworks
			);
			AddTech_Building(column, TechEnum::PaperMill, { TechEnum::Industrialization },
				CardEnum::PaperMill
			);
			AddTech_Building(column, TechEnum::CottonMilling, { TechEnum::Industrialization },
				CardEnum::CottonMill
			);
			AddTech_Building(column, TechEnum::ClockMakers, { TechEnum::JewelryCrafting, TechEnum::ScientificTheories },
				CardEnum::ClockMakers
			);
			AddTech_Bonus(column, TechEnum::SocialScience, { TechEnum::ScientificTheories });
			AddTech_Building(column, TechEnum::MelonFarming, { TechEnum::Fertilizers },
				{ CardEnum::MelonSeed }, ResourceEnum::Pumpkin, 3000
			);
			


			column = 11;
			AddTech_Building(column, TechEnum::OilWell, { TechEnum::Steelworks },
				{ CardEnum::OilWell, CardEnum::OilPowerPlant }
			);
			AddTech_Building(column, TechEnum::GrandMuseum, { TechEnum::Steelworks },
				{ CardEnum::GrandMuseum, CardEnum::ExhibitionHall }
			);
			AddTech_Building(column, TechEnum::Printing, { TechEnum::Steelworks },
				{ CardEnum::PrintingPress }
			);
			AddTech_Bonus(column, TechEnum::EconomicTheories, { TechEnum::ClockMakers, TechEnum::SocialScience });

			townhallUpgradeUnlocked = false;
			unlockedStatisticsBureau = false;
			unlockedEmploymentBureau = false;

			unlockedPriorityStar = false;
			unlockedSetTradeAmount = false;
			unlockedSetDeliveryTarget = false;

			/*
			 * Prosperity UI
			 *  500 pop is 100 houses
			 *  unlockCount is suppose to skewed so that lower level
			 */
			column = 1;
			AddProsperityTech_Bonus(column, 2, TechEnum::InfluencePoints);
			AddProsperityTech_Building(column, 2, TechEnum::BarrackArcher, CardEnum::BarrackArcher);
			AddProsperityTech_Bonus(column, 2, TechEnum::Conquer);
			AddProsperityTech_Bonus(column, 2, TechEnum::Vassalize);
			

			column = 2;
			AddProsperityTech_Bonus(column, 2, TechEnum::Combo);
			AddProsperityTech_Bonus(column, 2, TechEnum::HomeLandDefense);
			AddProsperityTech_Bonus(column, 20, TechEnum::FarmingBreakthrough);
			AddProsperityTech_Bonus(column, 20, TechEnum::FarmImprovement);
			
			
			column = 3;
			AddProsperityTech_BuildingPermanent(column, 10, TechEnum::GardenShrubbery1, { CardEnum::GardenShrubbery1 });
			
			
			column = 4;
			AddProsperityTech_Building(column, 10, TechEnum::InventorsWorkshop, CardEnum::InventorsWorkshop);
			AddProsperityTech_Bonus(column, 14, TechEnum::IndustrialAdjacency);
			AddProsperityTech_Bonus(column, 14, TechEnum::Rationalism);
			
			column = 5;
			AddProsperityTech_BuildingPermanent(column, 4, TechEnum::StoneRoad, { CardEnum::StoneRoad });
			AddProsperityTech_BuildingX(column, 4, TechEnum::GoldRefining, { CardEnum::GoldMine, CardEnum::GoldSmelter });
			
			column = 6;
			AddProsperityTech_Building(column, 4, TechEnum::Fort, CardEnum::Fort);
			AddProsperityTech_Building(column, 4, TechEnum::ResourceOutpost, CardEnum::ResourceOutpost);
			AddProsperityTech_BuildingPermanent(column, 10, TechEnum::FlowerBed, { CardEnum::FlowerBed });
			
			column = 7;

			AddProsperityTech_BuildingPermanent(column, 30, TechEnum::GardenCypress, { CardEnum::GardenCypress });
		}
	}
	//virtual ~UnlockSystem() = default;

	void UpdateProsperityHouseCount();

	void UpdateResourceProductionCount(ResourceEnum resourceEnum, int32 count) {
		_resourceEnumToProductionCount[static_cast<int32>(resourceEnum)] += count;
		needTechDisplayUpdate = true;
	}
	int32 GetResourceProductionCount(ResourceEnum resourceEnum) {
		return _resourceEnumToProductionCount[static_cast<int32>(resourceEnum)];
	}

	// Age Change Tech
	static bool IsAgeChangeTech(TechEnum techEnum) {
		return techEnum == TechEnum::MiddleAge ||
			techEnum == TechEnum::EnlightenmentAge ||
			techEnum == TechEnum::IndustrialAge;
	}
	static int32 GetAgeChangeRequiredTechCount(TechEnum techEnum) {
		if (techEnum == TechEnum::MiddleAge) return 5;
		if (techEnum == TechEnum::EnlightenmentAge) return 14;
		if (techEnum == TechEnum::IndustrialAge) return 12;
		UE_DEBUG_BREAK();
		return 0;
	}
	int32 GetAgeChangeResearchCount(TechEnum techEnum) {
		if (techEnum == TechEnum::MiddleAge) return columnToResearchedCount(1) + columnToResearchedCount(2);
		if (techEnum == TechEnum::EnlightenmentAge) return columnToResearchedCount(3) + columnToResearchedCount(4) + columnToResearchedCount(5);
		if (techEnum == TechEnum::IndustrialAge) return columnToResearchedCount(6) + columnToResearchedCount(7) + columnToResearchedCount(8);
		UE_DEBUG_BREAK();
		return 0;
	}
	static FText GetPreviousAgeText(TechEnum techEnum) {
		if (techEnum == TechEnum::MiddleAge) return LOCTEXT("Dark Age", "Dark Age");
		if (techEnum == TechEnum::EnlightenmentAge) return LOCTEXT("Middle Age", "Middle Age");
		if (techEnum == TechEnum::IndustrialAge) return LOCTEXT("Enlightenment Age", "Enlightenment Age");
		UE_DEBUG_BREAK();
		return FText();
	}
	
	bool IsRequirementMetForTech(TechEnum techEnum)
	{
		if (IsAgeChangeTech(techEnum)) {
			return GetAgeChangeResearchCount(techEnum) >= GetAgeChangeRequiredTechCount(techEnum);
		}
		
		ResourceEnum requiredResourceEnum = _enumToTech[techEnum]->requiredResourceEnum;
		if (requiredResourceEnum != ResourceEnum::None) {
			if (SimSettings::IsOn("CheatFastTech")) {
				return true;
			}
			return GetResourceProductionCount(requiredResourceEnum) >= _enumToTech[techEnum]->requiredResourceCount;
		}
		return true;
	}
	FText GetTechRequirementPopupText(TechEnum techEnum)
	{
		auto tech = GetTechInfo(techEnum);

		if (tech->requiredResourceEnum != ResourceEnum::None) {
			return FText::Format(
				NSLOCTEXT("TechUI", "NeedSatisfyTechPrereq_Pop", "Technology's Prerequisite not met.<space>Satisfy the Prerequisite by producing {0} {1}"),
				TEXT_NUM(tech->requiredResourceCount),
				GetResourceInfo(tech->requiredResourceEnum).name
			);
		}

		if (IsAgeChangeTech(techEnum)) {
			return FText::Format(
				NSLOCTEXT("TechUI", "NeedSatisfyTechPrereqAgeChange_Pop", "Technology's Prerequisite not met.<space>Satisfy the Prerequisite by Researching {0} Technologies from the {1}."),
				TEXT_NUM(GetAgeChangeRequiredTechCount(techEnum)),
				GetPreviousAgeText(techEnum)
			);
		}

		UE_DEBUG_BREAK();
		return FText();
	}
	
	
	void UnlockAll()
	{
		if (!isUnlocked(CardEnum::Farm)) {
			UnlockBuilding(CardEnum::Farm);
		}

		auto unlockTree = [&](std::vector<std::vector<TechEnum>>& treeTechEnums)
		{
			for (auto& techEnums : treeTechEnums) {
				for (TechEnum techEnum : techEnums) {
					auto tech = GetTechInfo(techEnum);
					if (tech->state != TechStateEnum::Researched) {
						tech->state = TechStateEnum::Researched;
						tech->OnUnlock(_playerId, _simulation);
						techsFinished++;
					}
				}
			}
		};

		unlockTree(_columnToTechs);
		unlockTree(_houseLvlToProsperityTechEnum);

		_techQueue.clear();
	}

	//! Building Unlocks
	const std::vector<CardEnum>& unlockedBuildings() const { return _unlockedBuildings; }
	bool isUnlocked(CardEnum cardEnumIn) { 
		for (CardEnum cardEnum : _unlockedBuildings) {
			if (cardEnum == cardEnumIn) {
				return true;
			}
		}
		return false;
	}

	bool IsPermanentBuilding(CardEnum buildingEnum) {
		return CppUtils::Contains(_unlockedBuildings, buildingEnum);
	}

	void UnlockBuilding(CardEnum buildingEnum) { 
		PUN_CHECK(find(_unlockedBuildings.begin(), _unlockedBuildings.end(), buildingEnum) == _unlockedBuildings.end())
		_unlockedBuildings.push_back(buildingEnum); 
	}

	//! Research
	// UI Display
	const std::vector<std::vector<TechEnum>>& columnToTechEnums() { return _columnToTechs; }

	int32 columnToResearchedCount(int32 columnIndex)
	{
		const std::vector<TechEnum>& techEnums = _columnToTechs[columnIndex];
		int32 researchedCount = 0;
		for (TechEnum techEnum : techEnums) {
			if (GetTechInfo(techEnum)->state == TechStateEnum::Researched) {
				researchedCount++;
			}
		}
		return researchedCount;
	}
	
	std::shared_ptr<ResearchInfo> currentResearch()
	{
		if (_techQueue.empty()) {
			return GetTechInfo(TechEnum::None);
		}
		return GetTechInfo(_techQueue.back());
	}
	bool hasTargetResearch() { return _techQueue.size() > 0; }


	const std::vector<TechEnum>& techQueue() { return _techQueue; }

	float researchFraction() { return static_cast<float>(science100()) / science100Needed(); }

	bool IsResearched(TechEnum techEnum) {
		if (_enumToTech.find(techEnum) == _enumToTech.end()) {
			return false;
		}
		return _enumToTech[techEnum]->state == TechStateEnum::Researched;
	}

	bool IsLocked(TechEnum techEnum)
	{
		// Age
		auto techInfo = GetTechInfo(techEnum);
		int32 column = techInfo->column;
		if (column == 3 || column == 4 || column == 5) {
			if (techEnum == TechEnum::MiddleAge) {
				return columnToResearchedCount(2) == 0;
			}
			if (!IsResearched(TechEnum::MiddleAge)) {
				return true;
			}
		}
		if (column == 6 || column == 7 || column == 8) {
			if (techEnum == TechEnum::EnlightenmentAge) {
				return columnToResearchedCount(5) == 0;
			}
			if (!IsResearched(TechEnum::EnlightenmentAge)) {
				return true;
			}
		}
		if (column == 9 || column == 10 || column == 11) {
			if (techEnum == TechEnum::IndustrialAge) {
				return columnToResearchedCount(8) == 0;
			}
			if (!IsResearched(TechEnum::IndustrialAge)) {
				return true;
			}
		}
		
		// Prerequisites
		std::vector<TechEnum> prerequisites = techInfo->prerequisites();
		for (TechEnum prerequisiteEnum : prerequisites) {
			if (!IsResearched(prerequisiteEnum)) {
				return true;
			}
		}

		return false;
	}

	int32 GetTechCount()
	{
		int32 count = 0;
		for (std::vector<TechEnum>& techEnums : _columnToTechs) {
			count += techEnums.size();
		}
		return count;
	}
	

	// UI Input
	void ChooseResearch(TechEnum researchEnum) 
	{
		_techQueue.clear();
		_techQueue.push_back(researchEnum);
	}

	// Simulation
	void Research(int64 science100PerRound, int32 updatesPerSec);

	static void EraUnlockedDescription(TArray<FText>& args, int32 era, bool isTip);

	void OnEraUnlocked(TArray<FText>& args);

	void SetDisplaySciencePoint(TArray<FText>& args, bool hasIcon = true)
	{
		if (hasTargetResearch()) {
			ADDTEXT_(INVTEXT("{0}/{1}"), TEXT_100(science100()), scienceNeeded());
		}
		else {
			ADDTEXT_(INVTEXT("{0}"), TEXT_100(science100()));
		}
		
		if (hasIcon) {
			ADDTEXT_INV_("<img id=\"Science\"/>");
		}
	}
	

	std::shared_ptr<ResearchInfo> GetTechInfo(TechEnum researchEnum) {
		PUN_CHECK(_enumToTech.find(researchEnum) != _enumToTech.end());
		return _enumToTech[researchEnum];
	}

	int32 totalTechs() { return _enumToTech.size() - 1;  } // -1 for TechEnum::None tech
	bool allTechsUnlocked() { return techsFinished == totalTechs(); }

	int32 currentEra()
	{
		int32 currentEra = 1;
		for (int32 i = 1; i < _columnToTechs.size(); i++) {
			if (currentEra == _columnToTechs.size() - 1) {
				return currentEra; // Max era...
			}
			if (techsUnlockedInEra(i) < techsToUnlockedNextEra(i)) {
				return currentEra;
			}
			currentEra++;
		}
		return currentEra;
	}
	int32 techsUnlockedInEra(int32 era)
	{
		int32 techsUnlocked = 0;
		auto& techs = _columnToTechs[era];
		for (auto& techEnum : techs) {
			if (IsResearched(techEnum)) {
				techsUnlocked++;
			}
		}
		return techsUnlocked;
	}

	int32 lastEra() { return _columnToTechs.size() - 1; }

	bool shouldFlashTechToggler()
	{
		if (techsUnlockedInEra(lastEra()) == totalTechsInEra(lastEra())) {
			return false;
		}
		return !hasTargetResearch();
	}
	

	static const int32 percentTechToEraUnlock = 70;// 80;
	int32 techsToUnlockedNextEra(int32 era) {
		return totalTechsInEra(era) * percentTechToEraUnlock / 100;
	}
	int32 totalTechsInEra(int32 era) {
		return _columnToTechs[era].size();
	}

	int64 science100Needed() {
		return scienceNeeded() * 100;
	}
	int64 scienceNeeded() {
		PUN_CHECK(currentResearch()->techEnum != TechEnum::None);
		return currentResearch()->scienceNeeded(techsFinished);
	}

	int64 science100() { return science100XsecPerRound / Time::SecondsPerRound; }
	
	
	/*
	 * Serialize
	 */
	void Serialize(FArchive& Ar)
	{
		needTechDisplayUpdate = true; // After load/save, we need to update the display
		needProsperityDisplayUpdate = true;

		Ar << researchEnabled;
		Ar << techsFinished;
		Ar << science100XsecPerRound;
		Ar << shouldOpenTechUI;

		Ar << prosperityEnabled;
		Ar << shouldOpenProsperityUI;
	

		Ar << townhallUpgradeUnlocked;
		Ar << unlockedStatisticsBureau;
		Ar << unlockedEmploymentBureau;

		Ar << unlockedPriorityStar;
		Ar << unlockedSetTradeAmount;
		Ar << unlockedSetDeliveryTarget;

		// Other states
		
		/*
		 * Private
		 */

		SerializeVecValue(Ar, _unlockedBuildings);

		auto serializeTechPtr = [&](std::shared_ptr<ResearchInfo>& tech) {
			TechClassEnum classEnum;
			if (Ar.IsSaving()) {
				classEnum = tech->classEnum();
			}
			Ar << classEnum;

			if (Ar.IsLoading()) {
				switch (classEnum)
				{
				case TechClassEnum::ResearchInfo: tech = std::make_shared<ResearchInfo>(); break;
				case TechClassEnum::ResearchNone: tech = std::make_shared<ResearchNone>(); break;
				case TechClassEnum::Building_Research: tech = std::make_shared<Building_Research>(); break;
				case TechClassEnum::MoreBuilding_Research: tech = std::make_shared<MoreBuilding_Research>(); break;
				case TechClassEnum::BonusToggle_Research: tech = std::make_shared<BonusToggle_Research>(); break;
				default: UE_DEBUG_BREAK();
				}
			}
			tech->Serialize(Ar, _simulation);
		};

		// Tech
		SerializeVecVecValue(Ar, _columnToTechs);
		//SerializeVecLoop(Ar, _eraToTechs, [&](std::vector<std::shared_ptr<ResearchInfo>>& vecTech) {
		//	SerializeVecLoop(Ar, vecTech, [&](std::shared_ptr<ResearchInfo>& tech) {
		//		serializeTechPtr(tech);
		//	});
		//});

		// TODO: is this a problem??
		SerializeMapLoop(Ar, _enumToTech, [&](std::shared_ptr<ResearchInfo>& tech) {
			serializeTechPtr(tech);
		});

		SerializeVecValue(Ar, _techQueue);

		// Prosperity
		SerializeVecVecValue(Ar, _houseLvlToProsperityTechEnum);
		//SerializeVecLoop(Ar, _houseLvlToProsperityTech, [&](std::vector<std::shared_ptr<ResearchInfo>>& vecTech) {
		//	SerializeVecLoop(Ar, vecTech, [&](std::shared_ptr<ResearchInfo>& tech) {
		//		serializeTechPtr(tech);
		//	});
		//});
		SerializeVecVecValue(Ar, _houseLvlToUnlockCount);

		SerializeVecValue(Ar, _resourceEnumToProductionCount);
		
		//SerializeVecLoop(Ar, _techQueue, [&](std::shared_ptr<ResearchInfo>& tech) {
		//	serializeTechPtr(tech);
		//});

		Ar << didFirstTimeAnimalRavage;
		Ar << didFirstTimeMedicineLowPopup;
		Ar << didFirstTimeToolsLowPopup;
		Ar << didFirstTimeLaborer0;
		Ar << didFirstTimeLowHappiness;
	}

	int32 techsCompleted() { return techsFinished; }
	
public:
	/*
	 * Serialize
	 */
	bool needTechDisplayUpdate = true;
	bool needProsperityDisplayUpdate = true;
	
	int32 techsFinished = -1;
	bool researchEnabled;
	int64 science100XsecPerRound = 0;
	bool shouldOpenTechUI = false;
	
	// prosperity
	bool prosperityEnabled;
	bool shouldOpenProsperityUI = false;

	// Unlock states
	bool townhallUpgradeUnlocked = false;
	bool unlockedStatisticsBureau = false;
	bool unlockedEmploymentBureau = false;

	bool unlockedPriorityStar = false;
	bool unlockedSetTradeAmount = false;
	bool unlockedSetDeliveryTarget = false;

	// Other states

	bool didFirstTimeAnimalRavage = false;
	bool didFirstTimeMedicineLowPopup = false;
	bool didFirstTimeToolsLowPopup = false;
	bool didFirstTimeLaborer0 = false;
	bool didFirstTimeLowHappiness = false;

private:
	IGameSimulationCore* _simulation = nullptr;
	int32 _playerId = -1;

private:
	/*
	 * Serialize
	 */
	std::vector<CardEnum> _unlockedBuildings; // Permanent Building Only

	// Tech
	std::vector<std::vector<TechEnum>> _columnToTechs; // Need shared_ptr because of polymorphism
	std::unordered_map<TechEnum, std::shared_ptr<ResearchInfo>> _enumToTech;
	std::vector<TechEnum> _techQueue;

	// Prosperity
	std::vector<std::vector<TechEnum>> _houseLvlToProsperityTechEnum;
	std::vector<std::vector<int32>> _houseLvlToUnlockCount;

	// Resource Required
	std::vector<int32> _resourceEnumToProductionCount;
};


#undef LOCTEXT_NAMESPACE