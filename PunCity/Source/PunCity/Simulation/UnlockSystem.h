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
		LOCTEXT("Middle Age Desc", "Unlocks Townhall Level 3"),
	} },
	{ TechEnum::EnlightenmentAge, {
		LOCTEXT("Enlightenment Age", "Enlightenment Age"),
		LOCTEXT("Enlightenment Age Desc", "Unlocks Townhall Level 4"),
	} },
	{ TechEnum::IndustrialAge, {
		LOCTEXT("Industrial Age", "Industrial Age"),
		LOCTEXT("Industrial Age Desc", "Unlocks Townhall Level 5"),
	} },
	
	{ TechEnum::DeepMining, { LOCTEXT("Deep Mining", "Deep Mining") }},

	{ TechEnum::Ironworks, {
		LOCTEXT("Iron Working", "Iron Working"),
		LOCTEXT("Iron Working Desc", "+30% wood harvesting yield."),
	}},
	{ TechEnum::GoldSmelting, {
		LOCTEXT("Gold Smelting", "Gold Smelting")
	}},
	{ TechEnum::GoldWorking, {
		LOCTEXT("Gold Smithing", "Gold Smithing")
	}},
	{ TechEnum::Machinery, {
		LOCTEXT("Machinery", "Machinery"),
		LOCTEXT("Machinery Desc", "+30% wood harvesting yield."),
	}},

	{ TechEnum::StoneToolsShop, {
		LOCTEXT("Basic Tools", "Basic Tools")
	}},

	{ TechEnum::HerbFarming , {LOCTEXT("Basic Medicine", "Basic Medicine")}},

	{ TechEnum::Logistics1, { LOCTEXT("Logistics I", "Logistics I") }},
	{ TechEnum::Logistics2, { LOCTEXT("Logistics II", "Logistics II") }},
	{ TechEnum::Logistics3, { LOCTEXT("Logistics III", "Logistics III") }},
	{ TechEnum::Logistics4, { LOCTEXT("Logistics IV", "Logistics IV") }},
	{ TechEnum::Logistics5, {
		LOCTEXT("Logistics V", "Logistics V"),
		LOCTEXT("Logistics V Desc", "Doubles hauling capacity of all citizens."),
	} },

	{TechEnum::JewelryCrafting, { LOCTEXT("Jewelry Crafting", "Jewelry Crafting") }},
	{TechEnum::Baking, { LOCTEXT("Baking", "Baking") }},

	{TechEnum::Fence, { LOCTEXT("Fence", "Fence") }},

	{TechEnum::RerollCardsPlus1, {
		LOCTEXT("Ideation", "Ideation"),
		LOCTEXT("Ideation Desc", "+1 card each reroll.")
	}},

	{TechEnum::FarmingTechnologies, {
		LOCTEXT("Farming Technologies", "Farming Technologies"),
		LOCTEXT("Farming Technologies Desc", "+3% Farm Productivity per Level.")
	}},
	{TechEnum::RanchingTechnologies, {
		LOCTEXT("Ranching Technologies", "Ranching Technologies"),
		LOCTEXT("Ranching Technologies Desc", "+3% Ranch Productivity per Level.")
	}},
	{TechEnum::HeatingTechnologies, {
		LOCTEXT("Heating Technologies", "Heating Technologies"),
		LOCTEXT("Heating Technologies Desc", "Wood/Coal gives 5% more Heat per Level.")
	}},
	{TechEnum::ForestryTechnologies, {
		LOCTEXT("Forestry Technologies", "Forestry Technologies"),
		LOCTEXT("Forestry Technologies Desc", "+5% Wood Cutting Yield per Level.")
	}},
	{TechEnum::IndustrialTechnologies, {
		LOCTEXT("Industrial Technologies", "Industrial Technologies"),
		LOCTEXT("Industrial Technologies Desc", "+3% Industrial Productivity per Level.")
	}},
	{TechEnum::TradeRelations, {
		LOCTEXT("Trade Relations", "Trade Relations"),
		LOCTEXT("Trade Relations Desc", "-2% Trading Fee per Level.")
	}},
	{TechEnum::HighFashion, {
		LOCTEXT("High Fashion", "High Fashion"),
		LOCTEXT("High Fashion Desc", "Unlocks Tailor's new Work Mode: Fashionable Cloth")
	}},

	//{TechEnum::FarmImprovement, {
	//	"Centralization",
	//	"Townhall level 2+ gain +20 culture per round."
	//}},

	{TechEnum::BeerBrewery, { LOCTEXT("Beer Brewing", "Beer Brewing") }},
	{TechEnum::Pottery, { LOCTEXT("Pottery", "Pottery") }},

	{TechEnum::FurnitureWorkshop, { LOCTEXT("Furniture", "Furniture") }},

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
	{ TechEnum::SpyCenter, {
		LOCTEXT("Espionage", "Espionage")
	} },
	{ TechEnum::SpyNest, {
		LOCTEXT("Spy Nest", "Spy Nest"),
		LOCTEXT("Spy Nest Desc", "Convert enemy's Houses into Spy Nests to steal influence from the target player.")
	} },
	

	{ TechEnum::Colony, {
		LOCTEXT("Colonization", "Colonization"),
		//LOCTEXT("Colonization Desc", "Unlock the ability to build new Cities."),
	}},

	{ TechEnum::Electricity, {
		LOCTEXT("Electricity", "Electricity"),
	} },
	
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
	{ TechEnum::CharcoalBurnerImprovement, {
		LOCTEXT("Charcoal Burner Improvement", "Charcoal Burner Improvement"),
		LOCTEXT("Charcoal Burner Improvement Desc", "+30% productivity to Charcoal Burner.")
	} },

	{ TechEnum::WinerySnob, {
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
	
	{ TechEnum::ChocolateSnob, {
		LOCTEXT("Chocolate Snob", "Chocolate Snob"),
		LOCTEXT("Chocolate Snob Desc", "+30% production to Chocolatier.")
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
	{TechEnum::CropBreeding, {
		LOCTEXT("Crop Breeding", "Crop Breeding"),
		LOCTEXT("Crop Breeding Desc", "+20% Farm Production.")
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

	{ TechEnum::Petroleum, {
		LOCTEXT("Petroleum", "Petroleum"),
		LOCTEXT("Petroleum Desc", "Reveals Oil Resource on the Map."),
	} },

	/*
	 * Prosperity Tech
	 */
	//{ TechEnum::InfluencePoints, {
	//	LOCTEXT("Influence Points", "Influence Points"),
	//	LOCTEXT("Influence Points Desc", "Unlock Influence Points <img id=\"Influence\"/> used to claim land."),
	//}},

	//{ TechEnum::Conquer, {
	//	LOCTEXT("Conquer Province", "Conquer Province"),
	//	LOCTEXT("Conquer Province Desc", "Unlock Province Conquering"),
	//}},

	{ TechEnum::HomeLandDefense, {
		LOCTEXT("Homeland Defense", "Homeland Defense"),
		LOCTEXT("Homeland Defense Desc", "Provinces gain +10% defense bonus for each building on it."),
	}},

	//{ TechEnum::Vassalize, {
	//	LOCTEXT("Vassalize", "Vassalize"),
	//	LOCTEXT("Vassalize Desc", "Unlock Vassalization which allows you to turn other city into a vassal."),
	//}},

	//{ TechEnum::IntercityRoad, {
	//	LOCTEXT("Intercity Connection", "Intercity Connection"),
	//	LOCTEXT("Intercity Connection Desc", "Allow connecting Cities with Intercity Road to establish trade connections"),
	//}},

	{ TechEnum::Combo, {
		LOCTEXT("Building Combo", "Building Combo"),
		LOCTEXT("Building Combo Desc", "Gain Combo level 1,2,3 by constructing 2,4,8 buildings of the same type."),
	} },

	/*
	 * August 4
	 */
	{ TechEnum::CardInventory1, {
		LOCTEXT("Card Inventory I", "Card Inventory I"),
		LOCTEXT("Card Inventory I Desc", "Unlocks Card Inventory that can be used to stored unused cards (7 slots)."),
	}},
	{ TechEnum::CardInventory2, {
		LOCTEXT("Card Inventory II", "Card Inventory II"),
		LOCTEXT("Card Inventory II Desc", "Expand the number of slots in the Card Inventory (14 slots)."),
	}},

	/*
	 * October 7
	 */
	{ TechEnum::Fort, {
		LOCTEXT("Border Protection", "Border Protection"),
		LOCTEXT("Border Protection Desc", "Protected Provinces Gives x3 more <img id=\"Coin\"/> income."),
	} },
	{ TechEnum::TradeRoute, {
		LOCTEXT("Trade Route", "Trade Route"),
		LOCTEXT("Trade Route Desc", "Allows Establishing Trade Route to other city"),
	} },
	{ TechEnum::ForeignRelation, {
		LOCTEXT("Foreign Relation", "Foreign Relation"),
		LOCTEXT("Foreign Relation Desc", "Unlocks Diplomacy, Trade Deal, and Minor City Interactions"),
	} },
	{ TechEnum::PolicyMaking, {
		LOCTEXT("Policy Making", "Policy Making"),
		LOCTEXT("Policy Making Desc", "TODO: TEXT"),
	} },
	{ TechEnum::ForeignInvestment, {
		LOCTEXT("Foreign Investment", "Foreign Investment"),
		LOCTEXT("Foreign Investment Desc", "Allows building in foreign land to help allies."),
	} },

	{ TechEnum::Museum, {
		LOCTEXT("Museum", "Museum"),
		LOCTEXT("Museum Desc", "Allows Artifact Excavation from Ruins."),
	} },

	{ TechEnum::MarketInfluence, {
		LOCTEXT("Market Influence", "Market Influence"),
		LOCTEXT("Market Influence Desc", ""),
	} },

	{ TechEnum::Archer, {
		LOCTEXT("Archer", "Archer"),
		LOCTEXT("Archer Desc", ""),
	} },
	{ TechEnum::Warrior, {
		LOCTEXT("Warrior", "Warrior"),
		LOCTEXT("Warrior Desc", ""),
	} },
	{ TechEnum::Swordsman, {
		LOCTEXT("Swordsman", "Swordsman"),
		LOCTEXT("Swordsman Desc", ""),
	} },
	{ TechEnum::Knight, {
		LOCTEXT("Knight", "Knight"),
		LOCTEXT("Knight Desc", ""),
	} },
	{ TechEnum::MilitaryEngineering1, {
		LOCTEXT("Military Engineering I", "Military Engineering I"),
		LOCTEXT("Military Engineering I Desc", ""),
	} },
	{ TechEnum::MilitaryEngineering2, {
		LOCTEXT("Military Engineering II", "Military Engineering II"),
		LOCTEXT("Military Engineering II Desc", ""),
	} },

	{ TechEnum::Musketeer, {
		LOCTEXT("Musketeer", "Musketeer"),
		LOCTEXT("Musketeer Desc", ""),
	} },
	{ TechEnum::Infantry, {
		LOCTEXT("Infantry", "Infantry"),
		LOCTEXT("Infantry Desc", ""),
	} },
	{ TechEnum::MachineGun, {
		LOCTEXT("Machine Gun", "Machine Gun"),
		LOCTEXT("Machine Gun Desc", ""),
	} },
	{ TechEnum::Artillery, {
		LOCTEXT("Artillery", "Artillery"),
		LOCTEXT("Artillery Desc", ""),
	} },
	{ TechEnum::Tank, {
		LOCTEXT("Tank", "Tank"),
		LOCTEXT("Tank Desc", ""),
	} },	
	{ TechEnum::Battleship, {
		LOCTEXT("Battleship", "Battleship"),
		LOCTEXT("Battleship Desc", ""),
	} },

	/*
	 * Faction Specific
	 */

	{ TechEnum::SpiceFarming , {LOCTEXT("Spice Farming", "Spice Farming")} },
	{ TechEnum::CarpetTrade , {
		LOCTEXT("Carpet Trade", "Carpet Trade"),
		LOCTEXT("Carpet Desc", "Carpet has 0% Trading Fee."),
	} },
};

enum class TechClassEnum
{
	ResearchInfo,
	ResearchNone,
	Building_Research,
	CardGiving_Research,
	MoreBuilding_Research,
	BonusToggle_Research,
};

struct TechRequirements
{
public:
	ResourceEnum requiredResourceEnum = ResourceEnum::None;
	int32 requiredResourceCount = 0;

	int32 requiredHouseLvl = -1;
	int32 requiredHouselvlCount = -1;

	static TechRequirements ResourceProduced(ResourceEnum requiredResourceEnum, int32 requiredResourceCount) {
		TechRequirements result;
		result.requiredResourceEnum = requiredResourceEnum;
		result.requiredResourceCount = requiredResourceCount;
		return result;
	}

	static TechRequirements HouseLvlCount(int32 requiredHouseLvlIn, int32 requiredHouselvlCountIn) {
		TechRequirements result;
		result.requiredHouseLvl = requiredHouseLvlIn;
		result.requiredHouselvlCount = requiredHouselvlCountIn;
		return result;
	}

	void Serialize(FArchive& Ar) {
		Ar << requiredResourceEnum;
		Ar << requiredResourceCount;

		Ar << requiredHouseLvl;
		Ar << requiredHouselvlCount;
	}
};


class ResearchInfo
{
public:
	virtual ~ResearchInfo() {}

	void Init(int32 eraIn, TechEnum researchEnumIn, std::vector<TechEnum> prerequisites, IGameSimulationCore* simulation) {
		column = eraIn;
		techEnum = researchEnumIn;
		_prerequisites = prerequisites;

		_simulation = simulation;
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
		/*
		 * First Wonder Popup
		 */
		const std::vector<TechEnum> wonderTechs{
			TechEnum::Cathedral,
			TechEnum::Castle,
			TechEnum::GrandPalace,
			TechEnum::ExhibitionHall,

			TechEnum::GreatMosque,
		};
		for (TechEnum wonderTechEnum : wonderTechs) {
			if (techEnum == wonderTechEnum)
			{
				int32 wonderTechesResearched = 0;
				for (TechEnum curWonderTechEnum : wonderTechs) {
					if (simulation->IsResearched(playerId, curWonderTechEnum)) {
						wonderTechesResearched++;
					}
				}

				if (wonderTechesResearched == 1)
				{
					simulation->AddPopup(playerId, {
						LOCTEXT("UnlockedFirstWonder_Pop1", "You have unlocked your first World Wonder!"),
						LOCTEXT("UnlockedFirstWonder_Pop2", "<space>World Wonders grant Victory Score. First World Wonder of its kind that gets built grants its owner its full score, while each subsequent wonders grants half as much score."),
						LOCTEXT("UnlockedFirstWonder_Pop3", "<space>The game ends once all types of World Wonder gets built."),
					});
				}
			}
		}

		/*
		 * Georesource Check
		 */
		if (techEnum == TechEnum::Petroleum)
		{
			simulation->AddPopup(playerId, {
				LOCTEXT("UnlockedOilResource", "Oil Resource on the Map is revealed.")
			});
			simulation->CheckSeedAndMineCard(playerId);
		}

		if (techEnum == TechEnum::PolicyMaking)
		{
			simulation->TryAddCards_BoughtHandAndInventory(playerId, CardStatus(CardEnum::PolicyOffice, 1));
		}
	}

	// Helpers
	int64 scienceNeeded(int64 techsFinished)
	{
		const int64 scalingPercent = 230;
		//const int64 scalingPercentChangeThreshold = 7
		//const int64 scalingPercent2 = ... 100 * 2.4^7 * 2.0^3 = 366917
		
		if (!isMainTree)
		{
			int64 sciNeeded = 100;
			for (int32 i = 1; i < column; i ++) {
				sciNeeded = sciNeeded * scalingPercent / 100;
			}
			
			if (_buildingEnums.size() > 0)
			{
				switch (_buildingEnums[0]) {
				case CardEnum::ProductivityBook:
				case CardEnum::SustainabilityBook:
				case CardEnum::FrugalityBook:
					sciNeeded = 300;
					break;
				case CardEnum::Motivation:
				case CardEnum::Passion:
					sciNeeded = 800;
					break;
				default:
					break;
				}
			}
			

			// upgrade count
			for (int32 i = 1; i < upgradeCount; i++) {
				const int64 upgradeCountMultiplier = 150LL;
				sciNeeded = sciNeeded * upgradeCountMultiplier / 100LL;
			}

			return std::min(sciNeeded, 1000000LL);
		}
		
		/*
		 * After 40 techs
		 * - 1250 cost ... (techsFinished + 10) * (techsFinished + 10) * (techsFinished + 10) / 10 / 10
		 * - 50 * 5 * 5 * 5
		 * - at 125 sci production... it is 2.5 seasons
		 *
		 * Tech scaling affected by:
		 * - population growth
		 * - house upgrades
		 * Power of 4?
		 *
		 * - House Lvl 7 sci = 30
		 *
		 * Expected population in each age:
		 * - 
		 * 
		 */
		// exponential
		// - Last column tech is 143360 cost
		// - Full House Sci = 30 (7 ppl), 100 with library + school
		// - Full lvl 7 house 1400 pop: 1400 / 7  = 200 houses = 20000 sci per round
		int64 sciNeeded = 70; // June 16: 50->70
		for (int32 i = 1; i < column; i++) {
			// May 30: scale up more
			//  100 * 2^10 = 102,400
			//  100 * 2.3^10 = 414,265
			sciNeeded = sciNeeded * scalingPercent / 100;
		}
		
		return std::min(sciNeeded, 1000000LL);
	}

	float researchFraction(int32 researchesFinished, int64 science100XsecPerRound) {
		return science100XsecPerRound / (100.0f * scienceNeeded(researchesFinished) * Time::SecondsPerRound);
	}

	bool CannotUpgradeFurther() {
		if (maxUpgradeCount != -1) {
			return upgradeCount >= maxUpgradeCount;
		}
		return state == TechStateEnum::Researched;
	}
	

	virtual TechClassEnum classEnum() { return TechClassEnum::ResearchInfo; }
	virtual void Serialize(FArchive& Ar, IGameSimulationCore* simulation)
	{
		Ar << column;
		Ar << techEnum;
		Ar << state;

		Ar << isMainTree;

		SerializeVecValue(Ar, _prerequisites);

		//Ar << science100XsecPerRound;

		SerializeVecValue(Ar, _buildingEnums);
		SerializeVecValue(Ar, _permanentBuildingEnums);

		techRequirements.Serialize(Ar);

		Ar << upgradeCount;
		Ar << maxUpgradeCount;

		if (Ar.IsLoading()) {
			_simulation = simulation;
		}
	}
	
public:
	int32 column = 0;
	TechEnum techEnum = TechEnum::None;
	TechStateEnum state = TechStateEnum::Locked;

	//int32 science100XsecPerRound = 0;

	bool isMainTree = false;
	
	std::vector<CardEnum> _buildingEnums;
	std::vector<CardEnum> _permanentBuildingEnums;

	std::vector<TechEnum> _prerequisites;

	TechRequirements techRequirements;

	int32 upgradeCount = 0;
	int32 maxUpgradeCount = -1;

	// Non-Serialized
	IGameSimulationCore* _simulation = nullptr;
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

	void InitBuildingResearch(std::vector<CardEnum> buildingEnums, std::vector<CardEnum> permanentBuildingEnums, int32_t cardCount = -1);

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
	}
	
public:
	int32 _cardCount = -1;
};

class CardGiving_Research : public ResearchInfo
{
public:
	virtual ~CardGiving_Research() {}

	void InitCardGivingResearch(std::vector<CardEnum> buildingEnums, int32 maxUpgradeCountIn) {
		_buildingEnums = buildingEnums;
		maxUpgradeCount = maxUpgradeCountIn;
	}

	virtual FText GetName() override {
		if (HasCustomName()) {
			return ResearchInfo::GetName();
		}
		return GetBuildingInfo(_buildingEnums[0]).name;
	}

	virtual std::vector<CardEnum> GetUnlockNames() override {
		std::vector<CardEnum> results;
		for (CardEnum buildingEnum : _buildingEnums) {
			results.push_back(buildingEnum);
		}
		return results;
	}

	virtual void OnUnlock(int32 playerId, IGameSimulationCore* simulation) override;

	virtual TechClassEnum classEnum() override { return TechClassEnum::CardGiving_Research; }
};

class MoreBuilding_Research final : public Building_Research
{
public:
	virtual ~MoreBuilding_Research() {}

	//std::string Name() override { return ResearchInfo::Name(); }
	//std::string Description() override { return ResearchInfo::Description(); }

	virtual TechClassEnum classEnum() override { return TechClassEnum::MoreBuilding_Research; }
};

class BonusToggle_Research : public ResearchInfo 
{
public:
	virtual ~BonusToggle_Research() {}

	virtual TechClassEnum classEnum() override { return TechClassEnum::BonusToggle_Research; }

	
	virtual void OnUnlock(int32 playerId, IGameSimulationCore* simulation) final;
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
	void AddTech(std::shared_ptr<ResearchInfo> tech)
	{
		PUN_CHECK(_enumToTech.find(tech->techEnum) == _enumToTech.end());

		if (_isMainTree) {
			if (_columnIndex >= _columnToTechs.size()) {
				_columnToTechs.push_back({});
			}
			_columnToTechs[_columnIndex].push_back(tech->techEnum);
		}
		else {
			if (_columnIndex >= _columnToUpgradeTechEnum.size()) {
				_columnToUpgradeTechEnum.push_back({});
			}
			_columnToUpgradeTechEnum[_columnIndex].push_back(tech->techEnum);
		}

		tech->isMainTree = _isMainTree;
		
		_enumToTech[tech->techEnum] = std::static_pointer_cast<ResearchInfo>(tech);
	}

	void AddTech_Building(TechEnum researchEnum, std::vector<TechEnum> prerequisites,
		CardEnum buildingEnum)
	{
		auto tech = std::make_shared<Building_Research>();
		tech->Init(_columnIndex, researchEnum, prerequisites, _simulation);
		tech->InitBuildingResearch({ buildingEnum }, {}, 1);
		AddTech(tech);
	}
	void AddTech_Building(TechEnum researchEnum, std::vector<TechEnum> prerequisites,
		std::vector<CardEnum> buildingEnums)
	{
		auto tech = std::make_shared<Building_Research>();
		tech->Init(_columnIndex, researchEnum, prerequisites, _simulation);
		tech->InitBuildingResearch(buildingEnums, {}, 1);
		AddTech(tech);
	}
	void AddTech_Building(TechEnum researchEnum, std::vector<TechEnum> prerequisites,
		CardEnum buildingEnum, TechRequirements techRequirements)
	{
		auto tech = std::make_shared<Building_Research>();
		tech->Init(_columnIndex, researchEnum, prerequisites, _simulation);
		tech->InitBuildingResearch({ buildingEnum }, {}, 1);
		tech->techRequirements = techRequirements;
		AddTech(tech);
	}


	void AddTech_CardGiving(TechEnum researchEnum, std::vector<TechEnum> prerequisites,
		CardEnum buildingEnum, TechRequirements techRequirements = TechRequirements())
	{
		int32 maxCards = 3;
		if (IsSeedCard(buildingEnum)) {
			maxCards = 1;
		}
		if (IsBuildingSlotCard(buildingEnum)) {
			maxCards = 10;
		}
		if (IsBuildingCard(buildingEnum)) {
			maxCards = 1;
		}
		
		auto tech = std::make_shared<CardGiving_Research>();
		tech->Init(_columnIndex, researchEnum, prerequisites, _simulation);
		tech->InitCardGivingResearch({ buildingEnum }, maxCards);
		tech->techRequirements = techRequirements;
		AddTech(tech);
	}

	

	void AddTech_BuildingPermanent(TechEnum researchEnum, std::vector<TechEnum> prerequisites,
		std::vector<CardEnum> buildingEnums)
	{
		auto tech = std::make_shared<Building_Research>();
		tech->Init(_columnIndex, researchEnum, prerequisites, _simulation);
		tech->InitBuildingResearch({}, buildingEnums);
		auto techCasted = std::static_pointer_cast<ResearchInfo>(tech);
		AddTech(tech);
	}

	void AddTech_Bonus(TechEnum researchEnum, std::vector<TechEnum> prerequisites, TechRequirements techRequirements = TechRequirements())
	{
		auto tech = std::make_shared<BonusToggle_Research>();
		tech->Init(_columnIndex, researchEnum, prerequisites, _simulation);
		tech->techRequirements = techRequirements;

		if (researchEnum == TechEnum::ForestryTechnologies ||
			researchEnum == TechEnum::FarmingTechnologies ||
			researchEnum == TechEnum::RanchingTechnologies ||
			researchEnum == TechEnum::HeatingTechnologies ||
			researchEnum == TechEnum::IndustrialTechnologies ||
			researchEnum == TechEnum::TradeRelations) {
			tech->maxUpgradeCount = 10;
		}
		
		AddTech(tech);
	}

	/*
	 * Prosperity UI
	 */

	const std::vector<std::vector<TechEnum>>& columnToUpgradeTechEnums() {
		return _columnToUpgradeTechEnum;
	}
	//const std::vector<std::vector<int32>>& houseLvlToUnlockCounts() { return _houseLvlToUnlockCount; }

	TechEnum GetProsperityTechEnum(int32 houseLvl, int32 localIndex) {
		return _columnToUpgradeTechEnum[houseLvl][localIndex];
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
			UnlockBuilding(CardEnum::Demolish);
			
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
			_columnToUpgradeTechEnum.push_back({}); // Era 0 as blank
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

			//AddTech_Bonus(column, TechEnum::DesertTrade);
			//AddTech_Bonus(column, TechEnum::Sawmill);
			//AddTech_Bonus(column, TechEnum::ImprovedWoodCutting2);
			
			/*
			*/
			FactionEnum factionEnum = _simulation->playerFactionEnum(_playerId);

			_isMainTree = true;
			
			//
			_columnIndex = 1;
			AddTech_Building(TechEnum::TradingPost, {},
				{ CardEnum::TradingPost, CardEnum::TradingPort }
			);
			AddTech_Building(TechEnum::HerbFarming, {},
				{ CardEnum::HerbSeed }
			);
			
			
			//
			_columnIndex = 2;
			AddTech_Building(TechEnum::StoneToolsShop, { TechEnum::TradingPost },
				CardEnum::StoneToolsShop
			);
			
			if (factionEnum == FactionEnum::Arab) {
				AddTech_Building(TechEnum::Pottery, { TechEnum::TradingPost },
					{ CardEnum::Potter }
				);
				AddTech_Building(TechEnum::FurnitureWorkshop, { TechEnum::TradingPost },
					{ CardEnum::FurnitureWorkshop }
				);
			}
			else {
				AddTech_Building(TechEnum::Pottery, { TechEnum::TradingPost },
					{ CardEnum::Potter, CardEnum::ClayPit }
				);
				AddTech_Building(TechEnum::FurnitureWorkshop, { TechEnum::TradingPost },
					CardEnum::FurnitureWorkshop
				);
			}

			
			AddTech_Building(TechEnum::BeerBrewery, { TechEnum::TradingPost },
				CardEnum::BeerBrewery
			);


			/// - Arab
			TechEnum techLinkEnum_RanchSheep = TechEnum::RanchSheep;
			TechEnum techLinkEnum_MushroomSubstrateSterilization = TechEnum::MushroomSubstrateSterilization;
			if (factionEnum == FactionEnum::Arab) {
				techLinkEnum_RanchSheep = TechEnum::Forester;
				AddTech_Building(TechEnum::Forester, { TechEnum::HerbFarming },
					{ CardEnum::Forester }
				);
				
				techLinkEnum_MushroomSubstrateSterilization = TechEnum::Irrigation;
				AddTech_Building(techLinkEnum_MushroomSubstrateSterilization, { TechEnum::HerbFarming },
					{ CardEnum::IrrigationPump }
				);
			}
			else {
				AddTech_Building(TechEnum::RanchSheep, { TechEnum::HerbFarming },
					{ CardEnum::RanchSheep }
				);
				AddTech_Bonus(techLinkEnum_MushroomSubstrateSterilization, { TechEnum::HerbFarming });
			}
			
			// Middle Age
			_columnIndex = 3;
			AddTech_Bonus(TechEnum::MiddleAge, {},
				TechRequirements::HouseLvlCount(2, 10)
			);
			AddTech_Building(TechEnum::Ironworks, { TechEnum::StoneToolsShop, TechEnum::Pottery },
				{ CardEnum::IronSmelter }
			);
			AddTech_Building(TechEnum::BrickMaking, { TechEnum::Pottery },
				{ CardEnum::Brickworks }
			);
			AddTech_Building(TechEnum::Library, { TechEnum::FurnitureWorkshop, TechEnum::BeerBrewery },
				CardEnum::Library
			);
			AddTech_Building(TechEnum::Logistics1, {},
				CardEnum::HaulingServices
			);
			AddTech_Building(TechEnum::AgriculturalRevolution, { TechEnum::BeerBrewery, techLinkEnum_RanchSheep, techLinkEnum_MushroomSubstrateSterilization },
				CardEnum::Granary
			);
			//AddTech_Building(TechEnum::PotatoFarming, { techLinkEnum_MushroomSubstrateSterilization },
			//	{ CardEnum::PotatoSeed }
			//);

			TechEnum techLinkEnum_Potato = TechEnum::PotatoFarming;
			if (factionEnum == FactionEnum::Arab) {
				techLinkEnum_Potato = TechEnum::AgaveFarming;
				AddTech_Building(techLinkEnum_Potato, { techLinkEnum_MushroomSubstrateSterilization },
					{ CardEnum::AgaveSeeds, CardEnum::CactusFruitSeeds }
				);
			}
			else
			{
				AddTech_Building(techLinkEnum_Potato, { techLinkEnum_MushroomSubstrateSterilization },
					{ CardEnum::PotatoSeed }
				);
			}

			
			//
			_columnIndex = 4;
			AddTech_Building(TechEnum::Tailor, { TechEnum::Ironworks },
				CardEnum::Tailor,
				TechRequirements::HouseLvlCount(3, 5)
			);
			AddTech_Building(TechEnum::Blacksmith, { TechEnum::Ironworks, TechEnum::BrickMaking },
				CardEnum::Blacksmith
			);
			AddTech_Building(TechEnum::Fort, { TechEnum::BrickMaking },
				CardEnum::Fort
			);
			AddTech_Building(TechEnum::TradingCompany, { TechEnum::Library },
				CardEnum::TradingCompany
			);
			AddTech_Building(TechEnum::Logistics2, { TechEnum::AgriculturalRevolution, TechEnum::Logistics1 },
				{ CardEnum::Market }
			);

			if (factionEnum == FactionEnum::Arab) {
				AddTech_Building(TechEnum::Baking, { TechEnum::AgriculturalRevolution, techLinkEnum_Potato },
					{ CardEnum::Windmill, CardEnum::PitaBakery }
				);
			} else {
				AddTech_Building(TechEnum::Baking, { TechEnum::AgriculturalRevolution, techLinkEnum_Potato },
					{ CardEnum::Windmill, CardEnum::Bakery }
				);
			}

			

			/// - Arab
			TechEnum techLinkEnum_VodkaDistillery = TechEnum::VodkaDistillery;
			TechEnum techLinkEnum_Beekeeper = TechEnum::Beekeeper;
			if (factionEnum == FactionEnum::Arab) 
			{
				techLinkEnum_VodkaDistillery = TechEnum::TequilaDistillery;
				AddTech_Building(techLinkEnum_VodkaDistillery, { techLinkEnum_Potato },
					{ CardEnum::TequilaDistillery }
				);
				
				techLinkEnum_Beekeeper = TechEnum::CarpetWeaver;
				AddTech_Building(techLinkEnum_Beekeeper, { techLinkEnum_Potato },
					CardEnum::CarpetWeaver,
					TechRequirements::HouseLvlCount(2, 30)
				);
			}
			else {
				AddTech_Building(techLinkEnum_VodkaDistillery, { techLinkEnum_Potato },
					{ CardEnum::VodkaDistillery }
				);
				
				AddTech_Building(techLinkEnum_Beekeeper, { techLinkEnum_Potato },
					CardEnum::Beekeeper
				);
			}
			
			
			//
			_columnIndex = 5;
			AddTech_Building(TechEnum::GoldSmelting, { TechEnum::Blacksmith },
				{ CardEnum::GoldSmelter }
			);
			AddTech_Bonus(TechEnum::QuickBuild, { TechEnum::Blacksmith });
			AddTech_BuildingPermanent(TechEnum::TradeRoute, { TechEnum::Fort, TechEnum::TradingCompany },
				{ CardEnum::IntercityRoad, CardEnum::IntercityBridge }
			);
			AddTech_Building(TechEnum::ForeignRelation, { TechEnum::TradingCompany, TechEnum::Logistics2 },
				CardEnum::Embassy
			);
			AddTech_BuildingPermanent(TechEnum::Logistics3, { TechEnum::Logistics2 },
				{ CardEnum::Warehouse }
			);
			AddTech_Bonus(TechEnum::SpyNest, { TechEnum::Logistics2 });

			AddTech_Building(TechEnum::Winery, { TechEnum::Baking, techLinkEnum_VodkaDistillery },
				CardEnum::Winery,
				TechRequirements::HouseLvlCount(4, 15)
			);
			AddTech_Building(TechEnum::Medicine, { techLinkEnum_VodkaDistillery, techLinkEnum_Beekeeper },
				CardEnum::MedicineMaker
			);

			/// - Arab
			TechEnum techLinkEnum_CandleMaker = TechEnum::CandleMaker;
			if (factionEnum == FactionEnum::Arab) {
				techLinkEnum_CandleMaker = TechEnum::CarpetTrade;
				AddTech_Bonus(techLinkEnum_CandleMaker, { techLinkEnum_Beekeeper });
			}
			else {
				AddTech_Building(techLinkEnum_CandleMaker, { techLinkEnum_Beekeeper },
					CardEnum::CandleMaker,
					TechRequirements::HouseLvlCount(2, 30)
				);
			}
		
			
			// Enlightenment
			_columnIndex = 6;
			AddTech_Bonus(TechEnum::EnlightenmentAge, {},
				TechRequirements::HouseLvlCount(4, 30)
			);
			AddTech_Building(TechEnum::SandMine, { TechEnum::QuickBuild, TechEnum::GoldSmelting },
				{ CardEnum::SandMine, CardEnum::GlassSmelter }
			);
			//AddTech_Building(TechEnum::CardMaker, { TechEnum::TradeRoute },
			//	{ CardEnum::CardMaker, CardEnum::PaperMaker }
			//);
			AddTech_Bonus(TechEnum::PolicyMaking, { TechEnum::TradeRoute });
			AddTech_Building(TechEnum::School, { TechEnum::TradeRoute, TechEnum::ForeignRelation },
				CardEnum::School
			);
			AddTech_Building(TechEnum::Logistics4, { TechEnum::Logistics3 },
				{ CardEnum::ShippingDepot }
			);
			AddTech_BuildingPermanent(TechEnum::StoneRoad, { TechEnum::Logistics3, TechEnum::SpyNest },
				{ CardEnum::StoneRoad }
			);
			
			AddTech_Building(TechEnum::CoffeeRoaster, { TechEnum::Medicine, techLinkEnum_CandleMaker },
				CardEnum::CoffeeRoaster,
				TechRequirements::HouseLvlCount(4, 40)
			);
			
			
			//
			_columnIndex = 7;
			AddTech_Building(TechEnum::Glassworks, { TechEnum::SandMine },
				CardEnum::Glassworks,
				TechRequirements::HouseLvlCount(5, 30)
			);
			AddTech_Bonus(TechEnum::BudgetAdjustment, { TechEnum::SandMine, TechEnum::CardMaker });
			//AddTech_Bonus(TechEnum::PolicyMaking, { TechEnum::CardMaker, TechEnum::School, TechEnum::Logistics4 });
			AddTech_Building(TechEnum::CardMaker, { TechEnum::PolicyMaking, TechEnum::School, TechEnum::Logistics4 },
				{ CardEnum::CardMaker, CardEnum::PaperMaker }
			);
			
			AddTech_Bonus(TechEnum::Logistics5, { TechEnum::Logistics4, TechEnum::StoneRoad });

			AddTech_Building(TechEnum::Theatre, { TechEnum::Winery },
				CardEnum::Theatre
			);

			/// - Arab
			TechEnum techLinkEnum_ShroomFarm = TechEnum::ShroomFarm;
			if (factionEnum == FactionEnum::Arab) {
				techLinkEnum_ShroomFarm = TechEnum::SpiceFarming;
				AddTech_CardGiving(techLinkEnum_ShroomFarm, { TechEnum::CoffeeRoaster },
					CardEnum::SpicesSeeds
				);
			}
			else {
				AddTech_Building(techLinkEnum_ShroomFarm, { TechEnum::CoffeeRoaster },
					CardEnum::MagicMushroomFarm,
					TechRequirements::HouseLvlCount(4, 80)
				);
			}
			
				
			//
			_columnIndex = 8;
			AddTech_Bonus(TechEnum::WorkSchedule, { TechEnum::BudgetAdjustment });
			AddTech_Building(TechEnum::ForeignInvestment, { TechEnum::BudgetAdjustment, TechEnum::PolicyMaking },
				{ CardEnum::ForeignQuarter }
			);
			
			AddTech_BuildingPermanent(TechEnum::Colony, { TechEnum::Logistics5 },
				{ CardEnum::Colony, CardEnum::PortColony }
			);
			AddTech_Building(TechEnum::Tourism, { TechEnum::Theatre },
				{ CardEnum::Hotel }
			);
			
			AddTech_Building(TechEnum::Chocolatier, { TechEnum::Theatre },
				CardEnum::Chocolatier,
				TechRequirements::HouseLvlCount(6, 50)
			);
			
			AddTech_Building(TechEnum::RanchCow, { techLinkEnum_ShroomFarm },
				{ CardEnum::RanchCow }
			);

			//
			_columnIndex = 9;
			AddTech_BuildingPermanent(TechEnum::Machinery, { TechEnum::WorkSchedule, TechEnum::ForeignInvestment },
				{ CardEnum::Tunnel }
			); // TODO: Machinery + Improved Woodcutting
			AddTech_Building(TechEnum::GoldWorking, { TechEnum::ForeignInvestment },
				{ CardEnum::Mint }
			);
			AddTech_Building(TechEnum::Bank, { TechEnum::ForeignInvestment, TechEnum::Colony },
				{ CardEnum::Bank }
			);

			AddTech_Building(TechEnum::Museum, { TechEnum::Colony, TechEnum::Tourism, TechEnum::Chocolatier },
				{ CardEnum::Museum }
			);
			AddTech_Building(TechEnum::Zoo, { TechEnum::Chocolatier, TechEnum::RanchCow },
				{ CardEnum::Zoo }
			);
			
			
			// Industrial
			_columnIndex = 10;
			AddTech_Bonus(TechEnum::IndustrialAge, {},
				TechRequirements::HouseLvlCount(6, 80)
			);
			AddTech_Building(TechEnum::ConcreteFactory, { TechEnum::Machinery },
				CardEnum::ConcreteFactory
			);
			AddTech_Building(TechEnum::Industrialization, { TechEnum::Machinery, TechEnum::GoldWorking },
				{ CardEnum::IndustrialIronSmelter, CardEnum::Steelworks }
			);
			AddTech_Building(TechEnum::JewelryCrafting, { TechEnum::GoldWorking, TechEnum::Bank },
				{ CardEnum::Jeweler }
			);
			AddTech_Bonus(TechEnum::SpyCenter, { TechEnum::Bank, TechEnum::Museum });
			AddTech_Bonus(TechEnum::CropBreeding, { TechEnum::Zoo });


			_columnIndex = 11;
			AddTech_Building(TechEnum::Electricity, { TechEnum::ConcreteFactory, TechEnum::Industrialization },
				CardEnum::CoalPowerPlant
			);
			AddTech_Building(TechEnum::PaperMill, { TechEnum::Industrialization },
				CardEnum::PaperMill
			);
			AddTech_Building(TechEnum::CottonMilling, { TechEnum::Industrialization },
				CardEnum::CottonMill
			);
			AddTech_Building(TechEnum::ClockMakers, { TechEnum::JewelryCrafting, TechEnum::SpyCenter },
				CardEnum::ClockMakers
			);
			AddTech_Building(TechEnum::CardCombiner, { TechEnum::SpyCenter },
				{ CardEnum::CardCombiner }
			);

			AddTech_Bonus(TechEnum::Fertilizers, { TechEnum::CropBreeding });
			// TODO: ... Melon Gone
			


			_columnIndex = 12;
			AddTech_Building(TechEnum::Petroleum, { TechEnum::Electricity },
				{ CardEnum::OilRig, CardEnum::OilPowerPlant }
			);
			AddTech_Building(TechEnum::Printing, { TechEnum::Electricity, TechEnum::PaperMill },
				{ CardEnum::PrintingPress }
			);
			AddTech_Building(TechEnum::MarketInfluence, { TechEnum::CottonMilling, TechEnum::ClockMakers, TechEnum::CardCombiner },
				{ CardEnum::WorldTradeOffice }
			);

			
			_columnIndex = 13;
			AddTech_Bonus(TechEnum::ScientificTheories, { TechEnum::Petroleum, TechEnum::Printing });
			
			AddTech_Bonus(TechEnum::EconomicTheories, { TechEnum::Printing, TechEnum::MarketInfluence });
			AddTech_Bonus(TechEnum::SocialScience, { TechEnum::MarketInfluence });

			AddTech_Building(TechEnum::ExhibitionHall, { TechEnum::MarketInfluence },
				{ CardEnum::ExhibitionHall },
				TechRequirements::HouseLvlCount(8, 100)
			);

			townhallUpgradeUnlocked = false;
			unlockedStatisticsBureau = false;
			unlockedEmploymentBureau = false;

			unlockedPriorityStar = false;
			unlockedSetTradeAmount = false;
			unlockedSetDeliveryTarget = false;

			_unlockStates.resize(static_cast<int>(UnlockStateEnum::Count), false);

			/*
			 * Prosperity Tech UI
			 *  500 pop is 100 houses
			 *  unlockCount is suppose to skewed so that lower level
			 */


			/*
			AddProsperityTech_Bonus(column, TechEnum::TraderDiscount);
			 */

			//AddProsperityTech_BuildingX(column, 2, TechEnum::SpyGuard, { CardEnum::KidnapGuard, CardEnum::TreasuryGuard }); //TODO: properly bring it back

			_isMainTree = false;

			//
			_columnIndex = 1;
			AddTech_Bonus(TechEnum::CharcoalBurnerImprovement, {});
		
			AddTech_CardGiving(TechEnum::Wheat, {},
				CardEnum::WheatSeed
			);
			AddTech_CardGiving(TechEnum::Cabbage, {},
				CardEnum::CabbageSeed
			);

			AddTech_CardGiving(TechEnum::Frugality, {},
				CardEnum::FrugalityBook
			);
			
			//AddTech_Bonus(TechEnum::InfluencePoints, {});
			

			//
			_columnIndex = 2;
			AddTech_Bonus(TechEnum::ForestryTechnologies, { TechEnum::CharcoalBurnerImprovement });
			AddTech_CardGiving(TechEnum::ChimneyRestrictor, { TechEnum::CharcoalBurnerImprovement },
				CardEnum::ChimneyRestrictor
			);
			AddTech_Bonus(TechEnum::QuarryImprovement, { TechEnum::CharcoalBurnerImprovement });

			AddTech_CardGiving(TechEnum::HomeBrew, {}, 
				CardEnum::HomeBrew
			);
			AddTech_CardGiving(TechEnum::BeerTax, {},
				CardEnum::BeerTax
			);
			AddTech_CardGiving(TechEnum::Productivity, { TechEnum::Frugality },
				CardEnum::ProductivityBook
			);

			//AddTech_Bonus(TechEnum::HomeLandDefense, { TechEnum::InfluencePoints });

			
			//
			_columnIndex = 3;
			AddTech_Bonus(TechEnum::CardInventory1, {});
			AddTech_CardGiving(TechEnum::SmelterCombo, { TechEnum::QuarryImprovement },
				CardEnum::SmeltCombo
			);
			AddTech_CardGiving(TechEnum::MiningEquipment, { TechEnum::QuarryImprovement },
				CardEnum::MiningEquipment
			);
			AddTech_Bonus(TechEnum::FarmingTechnologies, {});

			AddTech_CardGiving(TechEnum::Sustainability, { TechEnum::Productivity },
				CardEnum::SustainabilityBook
			);

			AddTech_Bonus(TechEnum::Archer, {});

			
			//
			_columnIndex = 4;
			AddTech_Bonus(TechEnum::TradeRelations, { TechEnum::CardInventory1 });
			AddTech_CardGiving(TechEnum::CoalPipeline, { TechEnum::SmelterCombo },
				CardEnum::CoalPipeline
			);
			AddTech_Bonus(TechEnum::HeatingTechnologies, {});

			AddTech_CardGiving(TechEnum::FarmWaterManagement, { TechEnum::FarmingTechnologies },
				CardEnum::FarmWaterManagement
			);
			AddTech_Bonus(TechEnum::Combo, {});
			AddTech_BuildingPermanent(TechEnum::GardenShrubbery1, {},
				{ CardEnum::GardenShrubbery1 }
			);

			AddTech_Bonus(TechEnum::Swordsman, { TechEnum::Archer });
			//AddTech_Bonus(TechEnum::Conquer, { TechEnum::Warrior, TechEnum::Archer });
			
			
			//
			_columnIndex = 5;
			AddTech_Bonus(TechEnum::HouseAdjacency, { TechEnum::TradeRelations });
			AddTech_CardGiving(TechEnum::CoalTreatment, { TechEnum::CoalPipeline },
				{ CardEnum::CoalTreatment }
			);

			AddTech_Bonus(TechEnum::RanchingTechnologies, {});
			AddTech_Building(TechEnum::Cathedral, { TechEnum::Combo },
				CardEnum::Cathedral
			);


			AddTech_Bonus(TechEnum::Knight, { TechEnum::Swordsman });
			AddTech_Bonus(TechEnum::MilitaryEngineering1, { TechEnum::Swordsman });
			//AddTech_Bonus(TechEnum::Vassalize, { TechEnum::Conquer });
		

			
			//
			_columnIndex = 6;
			AddTech_Bonus(TechEnum::TaxAdjustment, { TechEnum::HouseAdjacency });
			AddTech_Bonus(TechEnum::CardInventory2, {});

			if (factionEnum == FactionEnum::Arab) {
				AddTech_CardGiving(TechEnum::DesertPilgrim, {},
					{ CardEnum::DesertPilgrim }
				);
			}
			else {
				AddTech_Building(TechEnum::Irrigation, {},
					{ CardEnum::IrrigationPump }
				);
			}
				
			AddTech_CardGiving(TechEnum::BlueberryFarming, {},
				{ CardEnum::BlueberrySeed }
			);
			
			AddTech_CardGiving(TechEnum::Motivation, {},
				{ CardEnum::Motivation }
			);

			AddTech_Bonus(TechEnum::Musketeer, {});

			
			//
			_columnIndex = 7;
			AddTech_Bonus(TechEnum::IndustrialAdjacency, { TechEnum::TaxAdjustment });
			AddTech_Bonus(TechEnum::ChocolateSnob, {});


			AddTech_CardGiving(TechEnum::PumpkinFarming, { TechEnum::BlueberryFarming },
				{ CardEnum::PumpkinSeed },
				TechRequirements::ResourceProduced(ResourceEnum::Blueberries, 1000)
			);


			AddTech_CardGiving(TechEnum::Passion, { TechEnum::Motivation },
				{ CardEnum::Passion }
			);
			AddTech_CardGiving(TechEnum::SlaveLabor, {},
				CardEnum::SlaveLabor
			);
			AddTech_Bonus(TechEnum::MilitaryEngineering2, { TechEnum::Musketeer });


			//
			_columnIndex = 8;

			AddTech_Building(TechEnum::Castle, {},
				CardEnum::Castle
			);
			AddTech_CardGiving(TechEnum::HappyBreadDay, { TechEnum::PumpkinFarming },
				{ CardEnum::HappyBreadDay }
			);
			AddTech_CardGiving(TechEnum::AllYouCanEat, { TechEnum::PumpkinFarming },
				{ CardEnum::AllYouCanEat }
			);

			AddTech_Building(TechEnum::ResourceOutpost, {},
				CardEnum::ResourceOutpost
			);

			AddTech_CardGiving(TechEnum::Lockdown, { TechEnum::SlaveLabor },
				{ CardEnum::Lockdown }
			);
			AddTech_Bonus(TechEnum::MachineGun, { TechEnum::MilitaryEngineering2 });
			
			//
			_columnIndex = 9;
			AddTech_CardGiving(TechEnum::Conglomerate, {},
				{ CardEnum::Conglomerate }
			);

			AddTech_BuildingPermanent(TechEnum::GardenCypress, {},
				{ CardEnum::GardenCypress }
			);
			AddTech_BuildingPermanent(TechEnum::FlowerBed, {},
				{ CardEnum::FlowerBed }
			);

			AddTech_CardGiving(TechEnum::BirthControl, {},
				{ CardEnum::BirthControl }
			);
			
			AddTech_Bonus(TechEnum::Artillery, {});
			AddTech_Bonus(TechEnum::Infantry, {});


			
			// Aux Industrial
			_columnIndex = 10;
			AddTech_Bonus(TechEnum::WinerySnob, { TechEnum::Conglomerate });
			AddTech_Building(TechEnum::ResearchLab, { TechEnum::Conglomerate },
				CardEnum::ResearchLab
			);
			AddTech_Bonus(TechEnum::IndustrialTechnologies, {});
			
			AddTech_BuildingPermanent(TechEnum::Garden, { TechEnum::GardenCypress },
				{ CardEnum::Garden }
			);

			AddTech_CardGiving(TechEnum::MelonFarming, {},
				{ CardEnum::MelonSeed },
				TechRequirements::ResourceProduced(ResourceEnum::Pumpkin, 3000)
			);

			
			//
			_columnIndex = 11;
			AddTech_CardGiving(TechEnum::BlingBling, { TechEnum::WinerySnob },
				{ CardEnum::BlingBling }
			);
			AddTech_CardGiving(TechEnum::BookWorm, { TechEnum ::ResearchLab },
				{ CardEnum::BookWorm }
			);

			AddTech_Bonus(TechEnum::HighFashion, {});
			AddTech_CardGiving(TechEnum::DepartmentOfAgriculture, {},
				{ CardEnum::DepartmentOfAgriculture }
			);
			AddTech_CardGiving(TechEnum::EngineeringOffice, {},
				{ CardEnum::EngineeringOffice }
			);
			AddTech_CardGiving(TechEnum::ArchitectsStudio, {},
				{ CardEnum::ArchitectStudio }
			);

			
			//
			_columnIndex = 12;
			AddTech_Building(TechEnum::GrandPalace, {},
				CardEnum::GrandPalace,
				TechRequirements::HouseLvlCount(8, 100)
			);
			AddTech_Building(TechEnum::GreatMosque, {},
				{ CardEnum::GreatMosque },
				TechRequirements::HouseLvlCount(8, 100)
			);
			
			AddTech_CardGiving(TechEnum::SocialWelfare, {},
				{ CardEnum::SocialWelfare }
			);

			AddTech_Bonus(TechEnum::Tank, {});
			AddTech_Bonus(TechEnum::Battleship, {});
		}
	}
	//virtual ~UnlockSystem() = default;

	void UpdateProsperityHouseCount();

	void UpdateResourceProductionCount(ResourceEnum resourceEnum, int32 count) {
		_resourceEnumToProductionCount[static_cast<int32>(resourceEnum)] += count;
		needTechDisplayUpdate = true;
		needProsperityDisplayUpdate = true;
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
	
	FText GetEraText(int32 era = -1) {
		if (era == -1) {
			era = GetEra();
		}
		if (era == 1) return LOCTEXT("Dark Age", "Dark Age");
		if (era == 2) return LOCTEXT("Middle Age", "Middle Age");
		if (era == 3) return LOCTEXT("Enlightenment Age", "Enlightenment Age");
		if (era == 4) return LOCTEXT("Industrial Age", "Industrial Age");
		if (era == 5) return LOCTEXT("Electricity", "Electricity");
		UE_DEBUG_BREAK();
		return FText();
	}
	
	bool IsRequirementMetForTech(TechEnum techEnum)
	{
		if (SimSettings::IsOn("CheatFastTech")) {
			return true;
		}

		// Tech Requirements
		const TechRequirements& techRequirements = _enumToTech[techEnum]->techRequirements;

		if (techRequirements.requiredResourceEnum != ResourceEnum::None) {
			return GetResourceProductionCount(techRequirements.requiredResourceEnum) >= techRequirements.requiredResourceCount;
		}
		if (techRequirements.requiredHouseLvl != -1) {
			return _simulation->GetHouseLvlCount_Player(_playerId, techRequirements.requiredHouseLvl, true) >= techRequirements.requiredHouselvlCount;
		}
		return true;
	}
	FText GetTechRequirementPopupText(TechEnum techEnum)
	{
		auto tech = GetTechInfo(techEnum);
		const TechRequirements& requirements = tech->techRequirements;
		
		if (requirements.requiredResourceEnum != ResourceEnum::None) {
			return FText::Format(
				NSLOCTEXT("TechUI", "NeedSatisfyTechPrereq_Pop", "Technology's Prerequisite not met.<space>Satisfy the Prerequisite by producing {0} {1}"),
				TEXT_NUM(requirements.requiredResourceCount),
				GetResourceInfo(requirements.requiredResourceEnum).name
			);
		}

		if (requirements.requiredHouseLvl != -1) {
			return FText::Format(
				NSLOCTEXT("TechUI", "NeedSatisfyHouseLvlPrereq_Pop", "Technology's Prerequisite not met.<space>Satisfy the Prerequisite by upgrading {0} Houses to Level {1}"),
				TEXT_NUM(requirements.requiredHouselvlCount),
				TEXT_NUM(requirements.requiredHouseLvl)
			);
		}

		//if (IsAgeChangeTech(techEnum)) {
		//	int32 houseLvl = GetTechRequirement_HouseLvl(techEnum);
		//	int32 houseLvlCount = GetTechRequirement_HouseLvlCount(techEnum);
		//	return FText::Format(
		//		NSLOCTEXT("TechUI", "NeedSatisfyTechPrereqAgeChange_Pop", "Technology's Prerequisite not met.<space>Satisfy the Prerequisite by upgrading {0} Houses to Lv {1}."),
		//		TEXT_NUM(houseLvlCount),
		//		TEXT_NUM(houseLvl)
		//	);
		//}

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
					tech->upgradeCount++;
				}
			}
		};

		unlockTree(_columnToTechs);
		unlockTree(_columnToUpgradeTechEnum);

		for (int32 i = 0; i < static_cast<int32>(UnlockStateEnum::Count); i++) {
			SetUnlockState(static_cast<UnlockStateEnum>(i), true);
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
	bool IsResearchable(TechEnum techEnum) {
		if (_enumToTech.find(techEnum) == _enumToTech.end()) {
			return false;
		}
		if (_enumToTech[techEnum]->maxUpgradeCount != -1) {
			return _enumToTech[techEnum]->upgradeCount < _enumToTech[techEnum]->maxUpgradeCount;
		}
		return _enumToTech[techEnum]->state != TechStateEnum::Researched;
	}

	int32 GetTechnologyUpgradeCount(TechEnum techEnum) {
		if (_enumToTech.find(techEnum) == _enumToTech.end()) {
			return false;
		}
		return _enumToTech[techEnum]->upgradeCount;
	}
	
	bool IsLocked(TechEnum techEnum, bool isMainTechTree)
	{
		// Age
		auto techInfo = GetTechInfo(techEnum);
		int32 column = techInfo->column;

		if (isMainTechTree)
		{
			if (column == 3 || column == 4 || column == 5) {
				if (techEnum == TechEnum::MiddleAge) {
					return columnToResearchedCount(2) == 0;
				}
				if (!IsResearched(TechEnum::MiddleAge)) {
					return true;
				}
			}
			if (column == 6 || column == 7 || column == 8 || column == 9) {
				if (techEnum == TechEnum::EnlightenmentAge) {
					return columnToResearchedCount(5) == 0;
				}
				if (!IsResearched(TechEnum::EnlightenmentAge)) {
					return true;
				}
			}
			if (column == 10 || column == 11 || column == 12 || column == 13) {
				if (techEnum == TechEnum::IndustrialAge) {
					return columnToResearchedCount(8) == 0;
				}
				if (!IsResearched(TechEnum::IndustrialAge)) {
					return true;
				}
			}
		}
		else
		{
			// Upgrades Tree
			if (column == 3 || column == 4 || column == 5) {
				if (!IsResearched(TechEnum::MiddleAge)) {
					return true;
				}
			}
			if (column == 6 || column == 7 || column == 8) {
				if (!IsResearched(TechEnum::EnlightenmentAge)) {
					return true;
				}
			}
			if (column == 9 || column == 10 || column == 11 || column == 12) {
				if (!IsResearched(TechEnum::IndustrialAge)) {
					return true;
				}
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

	int32 GetEra()
	{
		if (IsResearched(TechEnum::IndustrialAge)) return 4;
		if (IsResearched(TechEnum::EnlightenmentAge)) return 3;
		if (IsResearched(TechEnum::MiddleAge)) return 2;
		return 1;
	}
	

	// UI Input
	void ChooseResearch(TechEnum researchEnum) 
	{
		if (_techQueue.size() > 0)
		{
			TechEnum lastTechEnum = _techQueue.back();
			if (IsOnMainTechTree(researchEnum) != IsOnMainTechTree(lastTechEnum))
			{
				_simulation->AddPopupToFront(_playerId,
					FText::Format(
						LOCTEXT("Switched Research", "Switched from researching {0} to {1}"),
						GetTechInfo(lastTechEnum)->GetName(),
						GetTechInfo(researchEnum)->GetName()
					),
					IsOnMainTechTree(researchEnum) ? ExclusiveUIEnum::TechTreeUI : ExclusiveUIEnum::ProsperityUI, ""
				);
			}
		}
		
		_techQueue.clear();
		_techQueue.push_back(researchEnum);
	}

	// Simulation
	void Research(int64 science100PerRound, int32 updatesPerSec);


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
	//bool allTechsUnlocked() { return techsFinished == totalTechs(); } // TODO: fix this... should only compare Tech Tree's tech

	int32 currentTechColumn()
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

	bool IsOnMainTechTree(TechEnum techEnum) {
		for (int32 i = 0; i < _columnToTechs.size(); i++) {
			const auto& techs = _columnToTechs[i];
			for (int32 j = 0; j < techs.size(); j++) {
				if (techs[j] == techEnum) {
					return true;
				}
			}
		}
		return false;
	}

	

	int64 science100Needed() {
		return scienceNeeded() * 100;
	}
	int64 scienceNeeded() {
		PUN_CHECK(currentResearch()->techEnum != TechEnum::None);
		return currentResearch()->scienceNeeded(techsFinished);
	}

	int64 science100() { return science100XsecPerRound / Time::SecondsPerRound; }


	bool unlockState(UnlockStateEnum unlockStateEnum) { return _unlockStates[static_cast<int>(unlockStateEnum)]; }
	void SetUnlockState(UnlockStateEnum unlockStateEnum, bool value) {
		_unlockStates[static_cast<int>(unlockStateEnum)] = value;

		if (unlockStateEnum == UnlockStateEnum::ConquerProvince) {
			_simulation->AddPopup(_playerId,
				LOCTEXT("UnlockedProvinceConqueringPop", "Unlocked Province Conquering<space>You can now conquer opponent's provinces with your army. The province to conquer must be next to your territory.")
			);
		}
		if (unlockStateEnum == UnlockStateEnum::Vassalize) {
			_simulation->AddPopup(_playerId,
				LOCTEXT("UnlockedVassalizationPop", "Unlocked Vassalization<space>You can now vassalize another city with your army.\nTo vassalize another city, click vassalize on the target townhall.<space>Your vassal city keep their control, but must pay vassal tax to you.")
			);
		}
		if (unlockStateEnum == UnlockStateEnum::Raze) {
			_simulation->AddPopup(_playerId,
				LOCTEXT("UnlockedRazePop", "Unlocked Raze<space>You can now raze Minor City or Fort to remove them from the map.")
			);
		}
	}

	
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
				case TechClassEnum::CardGiving_Research: tech = std::make_shared<CardGiving_Research>(); break;
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
		SerializeVecVecValue(Ar, _columnToUpgradeTechEnum);
		//SerializeVecVecValue(Ar, _houseLvlToUnlockCount);

		SerializeVecValue(Ar, _resourceEnumToProductionCount);

		SerializeVecValue(Ar, _unlockStates);
		
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

	// Temp variable
	int32 _columnIndex = 0;
	bool _isMainTree = true;

private:
	/*
	 * Serialize
	 */
	std::vector<CardEnum> _unlockedBuildings; // Permanent Building Only

	// Tech
	std::vector<std::vector<TechEnum>> _columnToTechs; // Need shared_ptr because of polymorphism
	std::unordered_map<TechEnum, std::shared_ptr<ResearchInfo>> _enumToTech;
	std::vector<TechEnum> _techQueue;

	// Bonus Tech
	std::vector<std::vector<TechEnum>> _columnToUpgradeTechEnum;
	//std::vector<std::vector<int32>> _houseLvlToUnlockCount;

	// Resource Required
	std::vector<int32> _resourceEnumToProductionCount;

	std::vector<uint8> _unlockStates;
};


#undef LOCTEXT_NAMESPACE