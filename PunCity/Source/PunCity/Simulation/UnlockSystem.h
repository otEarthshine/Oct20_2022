// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "IGameSimulationCore.h"
#include "PunCity/CppUtils.h"
#include <unordered_map>
#include "PunCity/PunUtils.h"
#include "Buildings/House.h"


const std::vector<std::string> eraNumberToText =
{
	"",
	"I",
	"II",
	"III",
	"IV",
	"V",
	"VI",
	"VII",
	"VIII",
	"IX",
	"X",
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
static const std::unordered_map<TechEnum, std::vector<std::string>> ResearchName_BonusDescription =
{
	{TechEnum::DeepMining, { "Deep Mining" }},
	
	{TechEnum::IronRefining, { "Ironwork" }},
	{TechEnum::GoldRefining, { "Goldwork" }},

	{TechEnum::JewelryCrafting, { "Jewelry Crafting" }},
	{TechEnum::Baking, { "Baking" }},
	
	{TechEnum::Fence, { "Fence" }},

	{TechEnum::RerollCardsPlus1, {
		"Ideation",
		"+1 card each reroll."
	}},

	{TechEnum::FarmImprovement, {
		"Farm Improvements",
		"+5% farm production."
	}},

	//{TechEnum::FarmImprovement, {
	//	"Centralization",
	//	"Townhall level 2+ gain +20 culture per round."
	//}},

	{TechEnum::BeerBrewery, { "Beer Brewing" }},
	{TechEnum::Pottery, { "Pottery" }},

	{TechEnum::FurnitureWorkshop, {"Wood Industry"}},
	
	{TechEnum::HouseLvl2Income, {
		"Upgraded House Tax",
		"House level 2+ gain +2 income."
	}},
	{TechEnum::TaxAdjustment, {
		"Tax Adjustment",
		"Allow tax adjustment from townhall."
	}},

	{TechEnum::TradingPost, {
		"Foreign Trade",
		"Increase the number of immigrants by 20%."
	}},
	
	{TechEnum::TraderDiscount, {
		"Trader Discount",
		"If trading company and trading post are on the same region, both gain -5% trading fee.",
	}},
	// TODO: BuildingComboEnum?? Encourage ppl to build next building... holes to fill...

	{TechEnum::Library, {
		"Writing",
		"+10% science."
	}},

	{TechEnum::Espionage, {
		"Espionage",
	}},

	{TechEnum::ShrineRot, {
		"Unlock Shrines.",
	}},

	{TechEnum::CropStudy, {
		"Crop Variety",
		"aaaa",
	}},

	{TechEnum::Plantation, {
		"Plantation",
		"Unlock farm crops cards: Cannabis Seeds, Grape Seeds, Cocoa Seeds"
	}},
	
	{TechEnum::MushroomSubstrateSterilization, {
		"Mushroom Log Sterilization",
		"Decrease mushroom farm's wood consumption by 50%."
	}},
	
	{TechEnum::QuarryImprovement, {
		"Quarry Improvement",
		"+30% production to quarries."
	}},

	{TechEnum::WineryImprovement, {
		"Wine Snob",
		"+30% production to winery."
	}},

	{TechEnum::BorealLandCost, {
		"Boreal and Tundra Expedition",
		"Claiming boreal forest and tundra land cost half the usual gold."
	}},
	{TechEnum::DesertTrade, {
		"Silk Road",
		"Trading post and company built on desert gets -10% trade fee."
	}},
	{TechEnum::ShallowWaterEmbark, {
		"Shallow Water Embark",
		"Allow claiming land an unclaimed land across a body of water."
	}},

	{TechEnum::Sawmill, {
		"Sawmill",
		"+50% furniture workshop's efficiency."
	}},
	{TechEnum::ImprovedWoodCutting, {
		"Improved woodcutting Lvl 1",
		"+20% wood harvesting yield."
	}},
	{TechEnum::ImprovedWoodCutting2, {
		"Improved woodcutting Lvl 2",
		"+20% wood harvesting yield."
	}},
	{TechEnum::FarmingBreakthrough, {
		"Improved farming",
		"+20% farming yield."
	}},
	{TechEnum::CheapLand, {
		"Cheap land",
		"Claiming land cost 30% less."
	}},
	{TechEnum::CheapReroll, {
		"Cheap reroll",
		"Reroll cost half as much."
	}},

	//{TechEnum::ClaimLandByFood, {
	//	"Expeditionary supply",
	//	"Allows claiming land by food",
	//}},


	{TechEnum::FireStarter, {
		"Offensive fire"
	}},
	{TechEnum::MoreGoldPerHouse, {
		"Extra house income",
		"+3 <img id=\"Coin\"/> house income"
	}},

	{TechEnum::FarmAdjacency, {
		"Farm adjacency",
		"Farms get +5% efficiency for each nearby farm. (max at 15%)",
	}},
	{TechEnum::HouseAdjacency, {
		"House adjacency",
		"Houses get +1 gold for each nearby house. (max at +3 gold)",
	}},
	{TechEnum::IndustrialAdjacency, {
		"Industrial adjacency",
		"Industrial buildings get +5% efficiency for each nearby industry of the same type. (max at 15%)",
	}},

	{ TechEnum::ScienceLastEra, {
		"Thinking Machine",
		"+20% science output (Counts toward scientific victory)",
	}},
	{ TechEnum::MoneyLastEra, {
		"Advanced Economics",
		"+20% house income (Counts toward scientific victory)",
	}},
	{ TechEnum::FarmLastEra, {
		"Microbial Mastery",
		"+20% farm output (Counts toward scientific victory)",
	}},
	{ TechEnum::IndustryLastEra, {
		"Mastery of Machinery",
		"+20% industry output (Counts toward scientific victory)",
	}},
	{ TechEnum::MilitaryLastEra, {
		"Advanced Military",
		"+20% attack damage for all military units (Counts toward scientific victory)",
	}},

	/*
	 * Prosperity Tech
	 */
	{ TechEnum::Rationalism, {
		"Rationalism",
		"+20% Science Output",
	}},
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
	
	virtual std::string GetName() {
		auto found = ResearchName_BonusDescription.find(techEnum);
		PUN_CHECK(found != ResearchName_BonusDescription.end());
		return found->second[0];
	}

	virtual std::string GetBonusDescription() {
		auto found = ResearchName_BonusDescription.find(techEnum);
		PUN_CHECK(found != ResearchName_BonusDescription.end());
		if (found->second.size() >= 2) {
			return found->second[1];
		}
		return "";
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
		return (techsFinished + 10) * (techsFinished + 10) * (techsFinished + 10) / 10 / 10  /* Tech factor: */  * 15000 / 1000; // 5400 / 1000;
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
	}
	
public:
	int32 era = 0;
	TechEnum techEnum = TechEnum::None;
	TechStateEnum state = TechStateEnum::Locked;

	//int32 science100XsecPerRound = 0;
	
	std::vector<CardEnum> _buildingEnums;
	std::vector<CardEnum> _permanentBuildingEnums;
};

class ResearchNone final : public ResearchInfo
{
public:
	std::string GetName() override {
		return  "Choose Research";
	}

	std::string GetBonusDescription() override {
		return "Click here to choose a research task";
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

	std::string GetName() override {
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
	int32_t _cardCount = -1;
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
class UnlockSystem //: public IUnlockSystem
{
public:
	/*
	 * Tech UI
	 */
	void AddTech(int32 era, std::shared_ptr<ResearchInfo> tech) {
		PUN_CHECK(_enumToTech.find(tech->techEnum) == _enumToTech.end());
		auto techCasted = std::static_pointer_cast<ResearchInfo>(tech);
		if (era >= _eraToTechs.size()) {
			_eraToTechs.push_back({});
		}
		_eraToTechs[era].push_back(techCasted);
		_enumToTech[tech->techEnum] = techCasted;
	}

	void AddTech_Building(int32 era, TechEnum researchEnum, CardEnum buildingEnum, int32_t cardCount = 1) {
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
		auto techCasted = std::static_pointer_cast<ResearchInfo>(tech);
		while (houseLevel >= _houseLvlToProsperityTech.size()) {
			_houseLvlToProsperityTech.push_back({});
			_houseLvlToUnlockCount.push_back({});
		}

		int32 cumulativeUnlockCount = unlockCount;
		int32 unlockListSize = _houseLvlToUnlockCount[houseLevel].size();
		if (unlockListSize > 0) {
			cumulativeUnlockCount += _houseLvlToUnlockCount[houseLevel][unlockListSize - 1];
		}
		
		_houseLvlToProsperityTech[houseLevel].push_back(techCasted);
		_houseLvlToUnlockCount[houseLevel].push_back(cumulativeUnlockCount);
		_enumToTech[tech->techEnum] = techCasted;
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

	const std::vector<std::vector<std::shared_ptr<ResearchInfo>>>& houseLvlToProsperityTech() { return _houseLvlToProsperityTech; }
	const std::vector<std::vector<int32>>& houseLvlToUnlockCounts() { return _houseLvlToUnlockCount; }

	const std::shared_ptr<ResearchInfo>& GetProsperityTech(int32 houseLvl, int32 localIndex) {
		return _houseLvlToProsperityTech[houseLvl][localIndex];
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
			AddTech_Bonus(era, TechEnum::RerollCardsPlus1);
			AddTech_Bonus(era, TechEnum::CheapReroll);
			AddTech_Bonus(era, TechEnum::MushroomSubstrateSterilization);
			AddTech_Bonus(era, TechEnum::BorealLandCost);
			
			//
			era = 2;
			AddTech_Building(era, TechEnum::HerbFarming, { CardEnum::HerbSeed });
			//AddTech_Building(era, TechEnum::DeepMining, { CardEnum::GemstoneMine });
			AddTech_Building(era, TechEnum::TradingPost, { CardEnum::TradingPost, CardEnum::TradingPort });
			AddTech_Bonus(era, TechEnum::Plantation);
			AddTech_Building(era, TechEnum::IronRefining, { CardEnum::IronMine, CardEnum::IronSmelter });
			AddTech_Building(era, TechEnum::Blacksmith, CardEnum::Blacksmith);
			
			//
			era = 3;
			AddTech_Building(era, TechEnum::TradingCompany, CardEnum::TradingCompany);
			AddTech_Bonus(era, TechEnum::DesertTrade);
			AddTech_Bonus(era, TechEnum::ShallowWaterEmbark);
			AddTech_Bonus(era, TechEnum::Sawmill);
			AddTech_Building(era, TechEnum::Medicine, CardEnum::MedicineMaker);
			AddTech_Building(era, TechEnum::RanchSheep, { CardEnum::RanchSheep });
			
			//
			era = 4;
			//AddTech_Building(era, TechEnum::Forester, CardEnum::Forester);
			AddTech_Building(era, TechEnum::RanchCow, { CardEnum::RanchCow });
			AddTech_Bonus(era, TechEnum::HouseAdjacency);
			AddTech_Bonus(era, TechEnum::QuarryImprovement);
			AddTech_Bonus(era, TechEnum::HouseLvl2Income);
			
			//AddPermanentBuildingResearch(3, TechEnum::Fence, { BuildingEnum::Fence, BuildingEnum::FenceGate });

			
			//
			era = 5;
			AddTech_Bonus(era, TechEnum::ImprovedWoodCutting2);
			AddTech_Bonus(era, TechEnum::MoreGoldPerHouse);
			AddTech_Building(era, TechEnum::Theatre, CardEnum::Theatre);
			
			//
			era = 6;
			AddTech_Bonus(era, TechEnum::TaxAdjustment);
			//AddTech_Building(era, TechEnum::HumanitarianAid, { CardEnum::HumanitarianAidCamp });
			AddTech_Building(era, TechEnum::Garden, CardEnum::Garden);
			AddTech_Building(era, TechEnum::Bank, { CardEnum::Bank });
			AddTech_Bonus(era, TechEnum::TraderDiscount);
			
			//
			era = 7;
			//AddTech_Bonus(6, TechEnum::CropStudy);
			AddTech_Building(era, TechEnum::Espionage, { CardEnum::Steal });
			AddTech_Bonus(era, TechEnum::CheapLand);
			AddTech_Bonus(era, TechEnum::WineryImprovement);
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

			/*
			 * Prosperity UI
			 *  500 pop is 100 houses
			 *  unlockCount is suppose to skewed so that lower level
			 */
			era = 1;
			AddProsperityTech_Building(era, 8, TechEnum::FurnitureWorkshop, CardEnum::FurnitureWorkshop);
			AddProsperityTech_BuildingX(era, 2, TechEnum::Pottery, { CardEnum::Potter, CardEnum::ClayPit });
			AddProsperityTech_Building(era, 2, TechEnum::BeerBrewery, CardEnum::BeerBrewery);
			AddProsperityTech_BuildingPermanent(era, 20, TechEnum::FlowerBed, { CardEnum::FlowerBed });
			

			era = 2;
			AddProsperityTech_Building(era, 4, TechEnum::Library, CardEnum::Library);
			AddProsperityTech_BuildingX(era, 20, TechEnum::Baking, { CardEnum::Windmill, CardEnum::Bakery });
			AddProsperityTech_Bonus(era, 20, TechEnum::FarmAdjacency);
			AddProsperityTech_Bonus(era, 30, TechEnum::FarmingBreakthrough);
			AddProsperityTech_Bonus(era, 30, TechEnum::FarmImprovement);
			
			
			era = 3;
			AddProsperityTech_BuildingX(era, 4, TechEnum::BrickMaking, { CardEnum::Brickworks });

			AddProsperityTech_Building(era, 4, TechEnum::Tailor, CardEnum::Tailor);
			AddProsperityTech_Building(era, 2, TechEnum::Beekeeper, CardEnum::Beekeeper);
			AddProsperityTech_Building(era, 2, TechEnum::CandleMaker, CardEnum::CandleMaker);

			AddProsperityTech_BuildingPermanent(era, 10, TechEnum::GardenShrubbery1, { CardEnum::GardenShrubbery1 });
			
			
			era = 4;
			AddProsperityTech_Building(era, 10, TechEnum::Winery, CardEnum::Winery);
			AddProsperityTech_Building(era, 20, TechEnum::Chocolatier, CardEnum::Chocolatier);
			
			AddProsperityTech_Building(era, 10, TechEnum::School, CardEnum::School);
			AddProsperityTech_Building(era, 10, TechEnum::InventorsWorkshop, CardEnum::InventorsWorkshop);
		
			
			era = 5;
			AddProsperityTech_BuildingX(era, 4, TechEnum::GoldRefining, { CardEnum::GoldMine, CardEnum::GoldSmelter });
			AddProsperityTech_Building(era, 4, TechEnum::Mint, CardEnum::Mint);
			AddProsperityTech_BuildingX(era, 4, TechEnum::JewelryCrafting, { CardEnum::GemstoneMine, CardEnum::Jeweler });
			AddProsperityTech_Building(era, 20, TechEnum::PaperMaker, CardEnum::PaperMaker);
			AddProsperityTech_Building(era, 20, TechEnum::Printing, { CardEnum::PrintingPress });
			
			
			era = 6;
			AddProsperityTech_BuildingPermanent(era, 4, TechEnum::StoneRoad, { CardEnum::StoneRoad });
			AddProsperityTech_Building(era, 4, TechEnum::Fort, CardEnum::Fort);
			AddProsperityTech_BuildingPermanent(era, 4, TechEnum::IntercityRoad, { CardEnum::IntercityRoad });
			AddProsperityTech_Building(era, 20, TechEnum::Colony, CardEnum::Colony);
			AddProsperityTech_Building(era, 20, TechEnum::CardMaker, CardEnum::CardMaker);
			
			
			era = 7;
			AddProsperityTech_Bonus(era, 10, TechEnum::IndustrialAdjacency);
			AddProsperityTech_Bonus(era, 10, TechEnum::Rationalism);
			AddProsperityTech_Building(era, 20, TechEnum::CottonMilling, CardEnum::CottonMill);

			AddProsperityTech_BuildingPermanent(era, 30, TechEnum::GardenCypress, { CardEnum::GardenCypress });
		}
	}
	//virtual ~UnlockSystem() = default;

	void UpdateProsperityHouseCount()
	{
		// Prosperity unlock at 7 houses
		if (!prosperityEnabled &&
			_simulation->buildingFinishedCount(_playerId, CardEnum::House) >= 7)
		{
			prosperityEnabled = true;

			std::stringstream ss;
			ss << "Unlocked: House Upgrade Unlocks Menu.";
			PopupInfo popupInfo(_playerId, ss.str(), { "Show House Upgrade Unlocks", "Close" },
				PopupReceiverEnum::UnlockedHouseTree_ShowProsperityUI);
			popupInfo.warningForExclusiveUI = ExclusiveUIEnum::ProsperityUI;
			popupInfo.forcedSkipNetworking = true;
			_simulation->AddPopupToFront(popupInfo);
		}
		
		if (!prosperityEnabled) {
			return;
		}
		
		for (int32 i = 1; i <= House::GetMaxHouseLvl(); i++)
		{
			int32 houseCount = _simulation->GetHouseLvlCount(_playerId, i, true);
			
			std::vector<std::shared_ptr<ResearchInfo>>& prosperityTechs = _houseLvlToProsperityTech[i];
			for (size_t j = 0; j < prosperityTechs.size(); j++) 
			{
				auto& properityTech = prosperityTechs[j];
				if (properityTech->state != TechStateEnum::Researched)
				{
					if (houseCount >= _houseLvlToUnlockCount[i][j]) 
					{
						// Unlocked
						// Take away the amount of science used 
						properityTech->state = TechStateEnum::Researched;
						properityTech->OnUnlock(_playerId, _simulation);

						techsFinished++;
						needProsperityDisplayUpdate = true;

						//// popup
						//{
						//	std::stringstream ss;
						//	ss << "Reached " << _houseLvlToUnlockCount[i][j] << " house lvl " << i << ".\n";
						//	ss << "Unlocked " << properityTech->GetName() << ".";
						//	PopupInfo popupInfo(_playerId, ss.str());
						//	popupInfo.warningForExclusiveUI = ExclusiveUIEnum::ProsperityUI;
						//	popupInfo.forcedSkipNetworking = true;
						//	_simulation->AddPopupToFront(popupInfo);

						//	_simulation->soundInterface()->Spawn2DSound("UI", "ResearchComplete", _playerId);
						//}
					}
					else {
						break;
					}
				}
			}
		}
	}

	void UnlockAll()
	{
		if (!isUnlocked(CardEnum::Farm)) {
			UnlockBuilding(CardEnum::Farm);
		}
		
		for (auto& techs : _eraToTechs) {
			for (auto& tech : techs) {
				if (tech->state != TechStateEnum::Researched) {
					tech->state = TechStateEnum::Researched;
					tech->OnUnlock(_playerId, _simulation);
					techsFinished++;
				}
			}
		}

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
	const std::vector<std::vector<std::shared_ptr<ResearchInfo>>>& eraToTechs() { return _eraToTechs; }

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
	void Research(int32 science100PerRound, int32 updatesPerSec)
	{
		if (SimSettings::IsOn("CheatFastTech")) {
			science100PerRound += 20000 * 100 * 20;
		}
		
		// Multiple updates per second, so we divide accordingly science100PerRound/updatesPerSec
		science100XsecPerRound += GameRand::RandRound(science100PerRound, updatesPerSec);
		
		if (!hasTargetResearch()) {
			return;
		}
		
		auto tech = currentResearch();

		if (science100() >= science100Needed()) 
		{
			// Take away the amount of science used 
			science100XsecPerRound -= science100Needed() * Time::SecondsPerRound;
			
			int32 lastEra = currentEra();
			
			tech->state = TechStateEnum::Researched;
			_techQueue.pop_back();
			
			tech->OnUnlock(_playerId, _simulation);

			techsFinished++;
			needTechDisplayUpdate = true;

			std::vector<std::string> choices = { "Show tech tree", "Close" };
			PopupReceiverEnum receiver = PopupReceiverEnum::DoneResearchEvent_ShowTree;

			// Era popup
			bool unlockedNewEra = currentEra() > lastEra;
			if (unlockedNewEra)
			{
				std::stringstream ss;

				OnEraUnlocked(ss);
				
				PopupInfo popupInfo(_playerId, ss.str(), choices, receiver, true);
				popupInfo.warningForExclusiveUI = ExclusiveUIEnum::TechUI;
				popupInfo.forcedSkipNetworking = true;
				_simulation->AddPopupToFront(popupInfo);

				_simulation->soundInterface()->Spawn2DSound("UI", "ResearchCompleteNewEra", _playerId);

				_simulation->GenerateRareCardSelection(_playerId, RareHandEnum::BuildingSlotCards, "New era!");
			}
			else
			{
				PopupInfo popupInfo(_playerId, "Research Completed.", choices, receiver, true);
				popupInfo.warningForExclusiveUI = ExclusiveUIEnum::TechUI;
				popupInfo.forcedSkipNetworking = true;
				_simulation->AddPopupToFront(popupInfo);
				
				_simulation->soundInterface()->Spawn2DSound("UI", "ResearchComplete", _playerId);

				/*
				 * Science victory
				 */
				if (currentEra() >= 8) 
				{
					//std::vector<std::shared_ptr<ResearchInfo>> finalTechs = _eraToTechs[8];
					//int32 finalEraTechsDone = 0;
					//for (auto& finalTech : finalTechs) {
					//	if (IsResearched(finalTech->techEnum)) {
					//		finalEraTechsDone++;
					//	}
					//}

					//if (finalEraTechsDone == 3) {
					//	_simulation->AddPopupAll(PopupInfo(_playerId,
					//		_simulation->playerName(_playerId) + " is only 2 technolgies away from the science victory."
					//	), _playerId);
					//	_simulation->AddPopup(_playerId, "You are only 2 technolgies away from the science victory.");
					//}
					//else if (finalEraTechsDone == 5) {
					//	_simulation->ExecuteScienceVictory(_playerId);
					//}
				}
			}

			// Reply to show tech tree if needed..
			//std::string replyReceiver = _simulation->HasTargetResearch(_playerId) ? "" : "DoneResearchEvent_ShowTree";

			//auto researchCompletePopup = [&](std::string body)
			//{
			//	if (_simulation->HasTargetResearch(_playerId)) 
			//	{
			//		// TODO: more fancy research complete!
			//		
			//		PopupInfo popupInfo(_playerId, body, { "Close" }, "", true);
			//		popupInfo.warningForExclusiveUI = ExclusiveUIEnum::TechUI;

			//		_simulation->AddPopupToFront(popupInfo);
			//	}
			//	else {
			//		PopupInfo popupInfo(_playerId, body, { "Show tech tree", "Close" }, "DoneResearchEvent_ShowTree", true);
			//		popupInfo.warningForExclusiveUI = ExclusiveUIEnum::TechUI;

			//		_simulation->AddPopupToFront(popupInfo);
			//	}
			//};

			//researchCompletePopup("Research Completed.");
		}
	}

	void OnEraUnlocked(std::stringstream& ss);

	void SetDisplaySciencePoint(std::stringstream& ss, bool hasIcon = true) {
		ss << std::fixed << std::setprecision(1);
		ss << (science100() / 100.0f) << "";
		if (hasTargetResearch()) {
			ss << "/" << scienceNeeded();
		}
		if (hasIcon) {
			ss << "<img id=\"Science\"/>";
		}
	}
	

	std::shared_ptr<ResearchInfo> GetTechInfo(TechEnum researchEnum) {
		PUN_CHECK(_enumToTech.find(researchEnum) != _enumToTech.end());
		return _enumToTech[researchEnum];
	}

	// Others
	std::vector<TileObjEnum> allFarmPlants()
	{
		return {
			TileObjEnum::WheatBush,
			TileObjEnum::BarleyBush,
			TileObjEnum::Grapevines,
			TileObjEnum::GrassGreen,
			
			//TileObjEnum::PlumpCob,
			//TileObjEnum::CreamPod,
			TileObjEnum::Cabbage,
			TileObjEnum::Cocoa,
			TileObjEnum::Cotton,
			TileObjEnum::Dye,
			
			TileObjEnum::Herb,
			//TileObjEnum::BaconBush,
		};
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
		for (auto& tech : techs) {
			if (IsResearched(tech->techEnum)) {
				techsUnlocked++;
			}
		}
		return techsUnlocked;
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
		SerializeVecLoop(Ar, _eraToTechs, [&](std::vector<std::shared_ptr<ResearchInfo>>& vecTech) {
			SerializeVecLoop(Ar, vecTech, [&](std::shared_ptr<ResearchInfo>& tech) {
				serializeTechPtr(tech);
			});
		});
		
		SerializeMapLoop(Ar, _enumToTech, [&](std::shared_ptr<ResearchInfo>& tech) {
			serializeTechPtr(tech);
		});

		SerializeVecValue(Ar, _techQueue);

		// Prosperity
		SerializeVecLoop(Ar, _houseLvlToProsperityTech, [&](std::vector<std::shared_ptr<ResearchInfo>>& vecTech) {
			SerializeVecLoop(Ar, vecTech, [&](std::shared_ptr<ResearchInfo>& tech) {
				serializeTechPtr(tech);
			});
		});
		SerializeVecVecValue(Ar, _houseLvlToUnlockCount);
		
		//SerializeVecLoop(Ar, _techQueue, [&](std::shared_ptr<ResearchInfo>& tech) {
		//	serializeTechPtr(tech);
		//});
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

	bool townhallUpgradeUnlocked = false;
	bool unlockedStatisticsBureau = false;
	bool unlockedEmploymentBureau = false;

	bool unlockedPriorityStar = false;
	bool unlockedSetTradeAmount = false;

private:
	IGameSimulationCore* _simulation = nullptr;
	int32 _playerId = -1;

private:
	/*
	 * Serialize
	 */
	std::vector<CardEnum> _unlockedBuildings;

	// Tech
	std::vector<std::vector<std::shared_ptr<ResearchInfo>>> _eraToTechs; // Need shared_ptr because of polymorphism
	std::unordered_map<TechEnum, std::shared_ptr<ResearchInfo>> _enumToTech;
	std::vector<TechEnum> _techQueue;

	// Prosperity
	std::vector<std::vector<std::shared_ptr<ResearchInfo>>> _houseLvlToProsperityTech;
	std::vector<std::vector<int32>> _houseLvlToUnlockCount;
};
