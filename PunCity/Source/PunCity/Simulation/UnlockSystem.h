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
	{TechEnum::DeepMining, { LOCTEXT("Deep Mining", "Deep Mining") }},

	{TechEnum::IronRefining, { LOCTEXT("Ironworks", "Ironworks") }},
	{TechEnum::GoldRefining, { LOCTEXT("Goldworks", "Goldworks") }},

	{TechEnum::ImprovedLogistics, { LOCTEXT("Improved Logistics", "Improved Logistics") }},

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
	
	{TechEnum::TraderDiscount, {
		LOCTEXT("Trader Discount", "Trader Discount"),
		LOCTEXT("Trader Discount Desc", "Trading company adjacent to trading port gain -5% trading fee."),
	}},
	// TODO: BuildingComboEnum?? Encourage ppl to build next building... holes to fill...

	{TechEnum::Espionage, {
		LOCTEXT("Espionage", "Espionage")
	}},
	{TechEnum::SpyGuard, {
		LOCTEXT("Spy Guard", "Spy Guard")
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

	{TechEnum::Plantation, {
		LOCTEXT("Plantation", "Plantation"),
		LOCTEXT("Plantation Desc", "Unlocks farm crops cards: Cannabis, Grape, Cocoa, Cotton, Coffee, Tulip Seeds")
	}},
	
	{TechEnum::MushroomSubstrateSterilization, {
		LOCTEXT("Mushroom Log Sterilization", "Mushroom Log Sterilization"),
		LOCTEXT("Mushroom Log Sterilization Desc", "Decreases mushroom farm's wood consumption by 50%.")
	}},
	
	{TechEnum::QuarryImprovement, {
		LOCTEXT("Quarry Improvement", "Quarry Improvement"),
		LOCTEXT("Quarry Improvement Desc", "+30% production to quarries.")
	}},

	{TechEnum::WineryImprovement, {
		LOCTEXT("Wine Snob", "Wine Snob"),
		LOCTEXT("Wine Snob Desc", "+30% production to winery.")
	}},

	{TechEnum::BorealLandCost, {
		LOCTEXT("Boreal and Tundra Expedition", "Boreal and Tundra Expedition"),
		LOCTEXT("Boreal and Tundra Expedition Desc", "Claiming Boreal Forest and Tundra Province costs half as usual.")
	}},
	{TechEnum::DesertTrade, {
		LOCTEXT("Silk Road", "Silk Road"),
		LOCTEXT("Silk Road Desc", "Trading post and company built on desert gets -10% trade fee.")
	}},
	
	{ TechEnum::ShallowWaterEmbark, {
		LOCTEXT("Shallow Water Embark", "Shallow Water Embark"),
		LOCTEXT("Shallow Water Embark Desc", "Allows claiming bordering provinces across a body of water. 100% cost penalty applied to claim/attack through shallow water.")
	}},
	{ TechEnum::DeepWaterEmbark, {
		LOCTEXT("Deepwater Embark", "Deepwater Embark"),
		LOCTEXT("Deepwater Embark Desc", "Allows claiming coastal provinces across the sea. 200% cost penalty applied to claim/attack through deep water.")
	}},

	{TechEnum::Sawmill, {
		LOCTEXT("Sawmill", "Sawmill"),
		LOCTEXT("Sawmill", "+50% furniture workshop's efficiency.")
	}},
	{TechEnum::ImprovedWoodCutting, {
		LOCTEXT("Improved woodcutting Lvl 1", "Improved woodcutting Lvl 1"),
		LOCTEXT("Improved woodcutting Lvl 1 Desc", "+20% wood harvesting yield."),
	}},
	{TechEnum::ImprovedWoodCutting2, {
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

	{TechEnum::FarmAdjacency, {
		LOCTEXT("Farm Adjacency", "Farm Adjacency"),
		LOCTEXT("Farm Adjacency Desc", "Farms get +5% efficiency for each nearby farm. (max at 15%)"),
	}},
	{TechEnum::HouseAdjacency, {
		LOCTEXT("House Adjacency", "House Adjacency"),
		LOCTEXT("House Adjacency Desc", "Houses get +<img id=\"Coin\"/>1 for each nearby house. (max at +<img id=\"Coin\"/>3)"),
	}},
	{TechEnum::IndustrialAdjacency, {
		LOCTEXT("Industrial Adjacency", "Industrial Adjacency"),
		LOCTEXT("Industrial Adjacency Desc", "Industrial buildings get +5% efficiency for each nearby industry of the same type. (max at 15%)"),
	}},

	{ TechEnum::ScienceLastEra, {
		LOCTEXT("Scientific Theories", "Scientific Theories"),
		LOCTEXT("Scientific Theories Desc", "+20% science output"),
	}},
	{ TechEnum::MoneyLastEra, {
		LOCTEXT("Economics Theories", "Economics Theories"),
		LOCTEXT("Economics Theories", "+20% house income"),
	}},
	{ TechEnum::FarmLastEra, {
		LOCTEXT("Pesticides", "Pesticides"),
		LOCTEXT("Pesticides Desc", "+20% farm output"),
	}},
	{ TechEnum::IndustryLastEra, {
		LOCTEXT("Machinery Mastery", "Machinery Mastery"),
		LOCTEXT("Machinery Mastery Desc", "+20% industry output"),
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
		LOCTEXT("Vassalize", "Unlock Vassalization which allows you to turn other city into a vassal."),
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

	void Init(int32 eraIn, TechEnum researchEnumIn) {
		era = eraIn;
		techEnum = researchEnumIn;
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
	

	virtual void OnUnlock(int32 playerId, IGameSimulationCore* simulation)
	{

	}

	// Helpers
	int32 scienceNeeded(int32 techsFinished) {
		/*
		 * After 40 techs
		 * - 1250 cost ... (techsFinished + 10) * (techsFinished + 10) * (techsFinished + 10) / 10 / 10
		 * - at 125 sci production... it is 2.5 seasons
		 */
		int32 sciNeeded = (techsFinished + 10) * (techsFinished + 10) * (techsFinished + 10) * (techsFinished + 10) / 10 / 10 / 10  /* Tech factor: */  * 12000 / 1000; // 5400 / 1000;
		return std::min(sciNeeded, 100000);
	}

	float researchFraction(int32 researchesFinished, int32 science100XsecPerRound) {
		return science100XsecPerRound / (100.0f * scienceNeeded(researchesFinished) * Time::SecondsPerRound);
	}

	virtual TechClassEnum classEnum() { return TechClassEnum::ResearchInfo; }
	virtual void Serialize(FArchive& Ar, IGameSimulationCore* simulation)
	{
		Ar << era;
		Ar << techEnum;
		Ar << state;

		//Ar << science100XsecPerRound;

		SerializeVecValue(Ar, _buildingEnums);
		SerializeVecValue(Ar, _permanentBuildingEnums);

		Ar << requiredResourceEnum;
		Ar << requiredResourceCount;
	}
	
public:
	int32 era = 0;
	TechEnum techEnum = TechEnum::None;
	TechStateEnum state = TechStateEnum::Locked;

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
	void AddTech(int32 era, std::shared_ptr<ResearchInfo> tech) {
		PUN_CHECK(_enumToTech.find(tech->techEnum) == _enumToTech.end());
		if (era >= _eraToTechs.size()) {
			_eraToTechs.push_back({});
		}

		_eraToTechs[era].push_back(tech->techEnum);
		_enumToTech[tech->techEnum] = std::static_pointer_cast<ResearchInfo>(tech);
	}

	void AddTech_Building(int32 era, TechEnum researchEnum, CardEnum buildingEnum, int32 cardCount = 1) {
		auto tech = std::make_shared<Building_Research>();
		tech->Init(era, researchEnum);
		tech->InitBuildingResearch({ buildingEnum }, {}, _simulation, cardCount);
		AddTech(era, tech);
	}
	void AddTech_Building(int32 era, TechEnum researchEnum, std::vector<CardEnum> buildingEnums, int32_t cardCount = 1) {
		auto tech = std::make_shared<Building_Research>();
		tech->Init(era, researchEnum);
		tech->InitBuildingResearch(buildingEnums, {}, _simulation, cardCount);
		AddTech(era, tech);
	}
	void AddTech_Building(int32 era, TechEnum researchEnum, CardEnum buildingEnum, ResourceEnum requiredResourceEnum, int32 requiredResourceCount) {
		auto tech = std::make_shared<Building_Research>();
		tech->Init(era, researchEnum);
		tech->InitBuildingResearch({ buildingEnum }, {}, _simulation, 1);
		tech->requiredResourceEnum = requiredResourceEnum;
		tech->requiredResourceCount = requiredResourceCount;
		AddTech(era, tech);
	}

	void AddTech_BuildingPermanent(int32 era, TechEnum researchEnum, std::vector<CardEnum> buildingEnums) {
		auto tech = std::make_shared<Building_Research>();
		tech->Init(era, researchEnum);
		tech->InitBuildingResearch({}, buildingEnums, _simulation);
		auto techCasted = std::static_pointer_cast<ResearchInfo>(tech);
		AddTech(era, tech);
	}

	void AddTech_Bonus(int32 era, TechEnum researchEnum) {
		auto tech = std::make_shared<BonusToggle_Research>();
		tech->Init(era, researchEnum);
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
		tech->Init(era, researchEnum);
		tech->InitBuildingResearch(buildingEnums, {}, _simulation, cardCount);
		AddProsperityTech(era, unlockCount, tech);
	}

	void AddProsperityTech_BuildingPermanent(int32 era, int32 unlockCount, TechEnum researchEnum, std::vector<CardEnum> buildingEnums) {
		auto tech = std::make_shared<Building_Research>();
		tech->Init(era, researchEnum);
		tech->InitBuildingResearch({}, buildingEnums, _simulation);
		auto techCasted = std::static_pointer_cast<ResearchInfo>(tech);
		AddProsperityTech(era, unlockCount, tech);
	}

	void AddProsperityTech_Bonus(int32 era, int32 unlockCount, TechEnum researchEnum) {
		auto tech = std::make_shared<BonusToggle_Research>();
		tech->Init(era, researchEnum);
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
			_eraToTechs.push_back({}); // Era 0 as blank
			_enumToTech[TechEnum::None] = std::static_pointer_cast<ResearchInfo>(std::make_shared<ResearchNone>());


			// TODO: May be separate era into:
			// - 3 major unlocks
			// - 4 bonuses where 2 can be chosen?
			
			 //
			int32 era = 1;
			AddTech_Bonus(era, TechEnum::ImprovedWoodCutting);
			AddTech_Building(era, TechEnum::TradingPost, { CardEnum::TradingPost, CardEnum::TradingPort });
			AddTech_Bonus(era, TechEnum::MushroomSubstrateSterilization);
			AddTech_Bonus(era, TechEnum::BorealLandCost);
			AddTech_Building(era, TechEnum::SpyGuard, { CardEnum::KidnapGuard, CardEnum::TreasuryGuard });

			AddTech_Building(era, TechEnum::HerbFarming, { CardEnum::HerbSeed });
			AddTech_Building(era, TechEnum::PotatoFarming, { CardEnum::PotatoSeed });
			
			//
			era = 2;
			//AddTech_Building(era, TechEnum::DeepMining, { CardEnum::GemstoneMine });
			AddTech_Bonus(era, TechEnum::Plantation);
			AddTech_Bonus(era, TechEnum::RerollCardsPlus1);
			AddTech_Bonus(era, TechEnum::CheapReroll);
			AddTech_Building(era, TechEnum::IronRefining, { CardEnum::IronMine, CardEnum::IronSmelter });
			AddTech_Building(era, TechEnum::Blacksmith, CardEnum::Blacksmith);
			AddTech_Bonus(era, TechEnum::Sawmill);
			
			AddTech_Building(era, TechEnum::VodkaDistillery, { CardEnum::VodkaDistillery }, ResourceEnum::Potato, 1000);
			
			//
			era = 3;
			AddTech_Building(era, TechEnum::TradingCompany, CardEnum::TradingCompany);
			AddTech_Building(era, TechEnum::Market, { CardEnum::Market });
			AddTech_Building(era, TechEnum::Espionage, { CardEnum::Steal });
			
			AddTech_Building(era, TechEnum::Medicine, CardEnum::MedicineMaker);
			AddTech_Building(era, TechEnum::RanchSheep, { CardEnum::RanchSheep });

			AddTech_Building(era, TechEnum::BlueberryFarming, { CardEnum::BlueberrySeed });
			AddTech_Bonus(era, TechEnum::ShallowWaterEmbark);
			
			//
			era = 4;
			AddTech_Bonus(era, TechEnum::DesertTrade);
			AddTech_Bonus(era, TechEnum::HouseAdjacency);
			AddTech_Bonus(era, TechEnum::QuarryImprovement);
			AddTech_Bonus(era, TechEnum::ImprovedWoodCutting2);
			AddTech_BuildingPermanent(era, TechEnum::Tunnel, {CardEnum::Tunnel});

			AddTech_Building(era, TechEnum::PumpkinFarming, { CardEnum::PumpkinSeed }, ResourceEnum::Blueberries, 1000);
			
			//AddPermanentBuildingResearch(3, TechEnum::Fence, { BuildingEnum::Fence, BuildingEnum::FenceGate });

			
			//
			era = 5;
			AddTech_Bonus(era, TechEnum::MoreGoldPerHouse);
			AddTech_Building(era, TechEnum::ImprovedLogistics, { CardEnum::ShippingDepot });
			AddTech_Building(era, TechEnum::Irrigation, { CardEnum::IrrigationReservoir });

			AddTech_Building(era, TechEnum::ShroomFarm, { CardEnum::ShroomFarm }, ResourceEnum::Mushroom, 3000);
			AddTech_Building(era, TechEnum::CoffeeRoaster, { CardEnum::CoffeeRoaster });
			
			//
			era = 6;
			AddTech_Building(era, TechEnum::BarrackKnight, CardEnum::BarrackSwordman);
			AddTech_Building(era, TechEnum::Bank, { CardEnum::Bank });
			AddTech_Building(era, TechEnum::Theatre, CardEnum::Theatre);
			AddTech_Building(era, TechEnum::RanchCow, { CardEnum::RanchCow });

			AddTech_Building(era, TechEnum::MelonFarming, { CardEnum::MelonSeed }, ResourceEnum::Pumpkin, 3000);
			AddTech_Bonus(era, TechEnum::DeepWaterEmbark);
			
			//
			era = 7;
			AddTech_Bonus(era, TechEnum::HouseLvl6Income);
			AddTech_Bonus(era, TechEnum::TaxAdjustment);
			AddTech_Building(era, TechEnum::Garden, CardEnum::Garden);
			AddTech_Bonus(era, TechEnum::CheapLand);
			AddTech_Bonus(era, TechEnum::WineryImprovement);
			AddTech_Bonus(era, TechEnum::TraderDiscount);
			//AddTech_Building(era, TechEnum::ShrineRot, { CardEnum::ShrineGreedPiece });
			
			//
			era = 8;
			AddTech_Bonus(era, TechEnum::ScienceLastEra);
			AddTech_Bonus(era, TechEnum::MoneyLastEra);
			AddTech_Bonus(era, TechEnum::FarmLastEra);
			AddTech_Bonus(era, TechEnum::IndustryLastEra);
			AddTech_Bonus(era, TechEnum::MilitaryLastEra);

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
			era = 1;
			AddProsperityTech_Building(era, 8, TechEnum::FurnitureWorkshop, CardEnum::FurnitureWorkshop);
			AddProsperityTech_BuildingX(era, 1, TechEnum::Pottery, { CardEnum::Potter, CardEnum::ClayPit });
			AddProsperityTech_Building(era, 1, TechEnum::BeerBrewery, CardEnum::BeerBrewery);
			AddProsperityTech_Bonus(era, 2, TechEnum::InfluencePoints);
			AddProsperityTech_Building(era, 2, TechEnum::BarrackArcher, CardEnum::BarrackArcher);
			AddProsperityTech_Bonus(era, 2, TechEnum::Conquer);
			AddProsperityTech_Bonus(era, 2, TechEnum::Vassalize);
			

			era = 2;
			AddProsperityTech_Bonus(era, 2, TechEnum::Combo);
			AddProsperityTech_Bonus(era, 2, TechEnum::HomeLandDefense);
			AddProsperityTech_Building(era, 4, TechEnum::Library, CardEnum::Library);
			AddProsperityTech_BuildingX(era, 4, TechEnum::Baking, { CardEnum::Windmill, CardEnum::Bakery });
			AddProsperityTech_Bonus(era, 20, TechEnum::FarmAdjacency);
			AddProsperityTech_Bonus(era, 20, TechEnum::FarmingBreakthrough);
			AddProsperityTech_Bonus(era, 20, TechEnum::FarmImprovement);
			
			
			era = 3;
			AddProsperityTech_BuildingX(era, 4, TechEnum::BrickMaking, { CardEnum::Brickworks });

			AddProsperityTech_Building(era, 4, TechEnum::Tailor, CardEnum::Tailor);
			AddProsperityTech_Building(era, 2, TechEnum::Beekeeper, CardEnum::Beekeeper);
			AddProsperityTech_Building(era, 2, TechEnum::CandleMaker, CardEnum::CandleMaker);

			AddProsperityTech_BuildingPermanent(era, 10, TechEnum::GardenShrubbery1, { CardEnum::GardenShrubbery1 });
			
			
			era = 4;
			AddProsperityTech_Building(era, 10, TechEnum::Winery, CardEnum::Winery);
			AddProsperityTech_BuildingPermanent(era, 4, TechEnum::IntercityRoad, { CardEnum::IntercityRoad });
			
			AddProsperityTech_Building(era, 10, TechEnum::School, CardEnum::School);
			AddProsperityTech_Building(era, 10, TechEnum::PaperMaker, CardEnum::PaperMaker);
			AddProsperityTech_Building(era, 10, TechEnum::InventorsWorkshop, CardEnum::InventorsWorkshop);

			AddProsperityTech_Building(era, 10, TechEnum::Chocolatier, CardEnum::Chocolatier);
		
			
			era = 5;
			AddProsperityTech_BuildingPermanent(era, 4, TechEnum::StoneRoad, { CardEnum::StoneRoad });
			AddProsperityTech_BuildingX(era, 4, TechEnum::GoldRefining, { CardEnum::GoldMine, CardEnum::GoldSmelter });
			AddProsperityTech_Building(era, 4, TechEnum::Mint, CardEnum::Mint);
			AddProsperityTech_BuildingX(era, 4, TechEnum::JewelryCrafting, { CardEnum::GemstoneMine, CardEnum::Jeweler });
			AddProsperityTech_Building(era, 20, TechEnum::Printing, { CardEnum::PrintingPress });
			
			era = 6;
			AddProsperityTech_Building(era, 4, TechEnum::Fort, CardEnum::Fort);
			AddProsperityTech_Building(era, 4, TechEnum::Colony, CardEnum::Colony);
			AddProsperityTech_Building(era, 4, TechEnum::CardMaker, CardEnum::CardMaker);
			AddProsperityTech_Building(era, 10, TechEnum::CottonMilling, CardEnum::CottonMill);
			AddProsperityTech_BuildingPermanent(era, 10, TechEnum::FlowerBed, { CardEnum::FlowerBed });
			
			era = 7;
			AddProsperityTech_Bonus(era, 4, TechEnum::IndustrialAdjacency);
			AddProsperityTech_Bonus(era, 14, TechEnum::Rationalism);

			AddProsperityTech_BuildingPermanent(era, 30, TechEnum::GardenCypress, { CardEnum::GardenCypress });
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
	bool IsRequirementMetForTech(TechEnum techEnum)
	{
		ResourceEnum requiredResourceEnum = _enumToTech[techEnum]->requiredResourceEnum;
		if (requiredResourceEnum == ResourceEnum::None) {
			return true;
		}
		return GetResourceProductionCount(requiredResourceEnum) >= _enumToTech[techEnum]->requiredResourceCount;
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

		unlockTree(_eraToTechs);
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
	const std::vector<std::vector<TechEnum>>& eraToTechEnums() { return _eraToTechs; }

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
		PUN_CHECK(_enumToTech.find(techEnum) != _enumToTech.end());
		return _enumToTech[techEnum]->state == TechStateEnum::Researched;
	}


	// UI Input
	void ChooseResearch(TechEnum researchEnum) 
	{
		_techQueue.clear();
		_techQueue.push_back(researchEnum);
	}

	// Simulation
	void Research(int32 science100PerRound, int32 updatesPerSec);

	static void EraUnlockedDescription(TArray<FText>& args, int32 era, bool isTip);

	void OnEraUnlocked(TArray<FText>& args);

	void SetDisplaySciencePoint(TArray<FText>& args, bool hasIcon = true)
	{
		if (hasTargetResearch()) {
			ADDTEXT_(INVTEXT("{0}/{1}"), TEXT_100(science100()), scienceNeeded());
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
		for (int32 i = 1; i < _eraToTechs.size(); i++) {
			if (currentEra == _eraToTechs.size() - 1) {
				return currentEra; // Max era...
			}
			if (techsUnlockedInEra(i) < techsToUnlockedNextEra(i)) {
				return currentEra;
			}
			currentEra++;
		}
		return currentEra;
	}
	int32 isTechLocked(TechEnum techEnum) {
		return _enumToTech[techEnum]->era > currentEra();
	}
	int32 techsUnlockedInEra(int32 era)
	{
		int32 techsUnlocked = 0;
		auto& techs = _eraToTechs[era];
		for (auto& techEnum : techs) {
			if (IsResearched(techEnum)) {
				techsUnlocked++;
			}
		}
		return techsUnlocked;
	}

	int32 lastEra() { return _eraToTechs.size() - 1; }

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
		return _eraToTechs[era].size();
	}

	int32 science100Needed() {
		return scienceNeeded() * 100;
	}
	int32 scienceNeeded() {
		PUN_CHECK(currentResearch()->techEnum != TechEnum::None);
		return currentResearch()->scienceNeeded(techsFinished);
	}

	int32 science100() { return science100XsecPerRound / Time::SecondsPerRound; }
	
	
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
		SerializeVecVecValue(Ar, _eraToTechs);
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
	int32 science100XsecPerRound = 0;
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

private:
	IGameSimulationCore* _simulation = nullptr;
	int32 _playerId = -1;

private:
	/*
	 * Serialize
	 */
	std::vector<CardEnum> _unlockedBuildings; // Permanent Building Only

	// Tech
	std::vector<std::vector<TechEnum>> _eraToTechs; // Need shared_ptr because of polymorphism
	std::unordered_map<TechEnum, std::shared_ptr<ResearchInfo>> _enumToTech;
	std::vector<TechEnum> _techQueue;

	// Prosperity
	std::vector<std::vector<TechEnum>> _houseLvlToProsperityTechEnum;
	std::vector<std::vector<int32>> _houseLvlToUnlockCount;

	// Resource Required
	std::vector<int32> _resourceEnumToProductionCount;
};


#undef LOCTEXT_NAMESPACE