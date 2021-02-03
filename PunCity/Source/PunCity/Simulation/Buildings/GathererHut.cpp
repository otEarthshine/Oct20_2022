// Fill out your copyright notice in the Description page of Project Settings.

#include "GathererHut.h"
#include "TownHall.h"
#include "../OverlaySystem.h"
#include "../Resource/ResourceSystem.h"
#include "../TreeSystem.h"
#include "../UnlockSystem.h"
#include "../UnitSystem.h"
#include "../UnitStateAI.h"
#include "../ArmyStateAI.h"
#include "../PlayerOwnedManager.h"
#include "../GeoresourceSystem.h"
#include "../BuildingCardSystem.h"
#include "PunCity/CppUtils.h"
#include "House.h"

using namespace std;

#define LOCTEXT_NAMESPACE "GathererHut"

static const FText normalWorkModeText = LOCTEXT("Normal_WorkMode", "Normal");
static const FText normalWorkModeDesc = LOCTEXT("Normal_WorkModeDesc", "Work mode without any bonuses.");

//static FText ProductivityDescText(int32 productivity) {
//	return FText::Format(LOCTEXT("+{0}% productivity.", "+{0}% productivity."), TEXT_NUM(productivity));
//}

static FText TownhallUpgradeBonusText(int32 level) {
	return FText::Format(LOCTEXT("Townhall Lvl {0} Upgrade", "Townhall Lvl {0} Upgrade"), TEXT_NUM(level));
}

/*
 * GathererHut
 */
static const FText delicateGatheringText = LOCTEXT("Delicate Gathering", "Delicate Gathering");
static const FText pestTrapText = LOCTEXT("Pests Traps", "Pests Traps");

void GathererHut::OnInit()
{
	SetupWorkMode({
		WorkMode::Create(normalWorkModeText, normalWorkModeDesc),
		WorkMode::Create(MeticulousWorkModeText, LOCTEXT("Meticulous_WorkModeDesc","Gathering action takes twice as long, but yield 30% more fruit.")),
	});
}

void GathererHut::FinishConstruction()
{
	Building::FinishConstruction();

	AddResourceHolder(ResourceEnum::Orange, ResourceHolderType::Provider, 0);
	AddResourceHolder(ResourceEnum::Papaya, ResourceHolderType::Provider, 0);
	AddResourceHolder(ResourceEnum::Coconut, ResourceHolderType::Provider, 0);

	_upgrades = {
		MakeProductionUpgrade(delicateGatheringText, ResourceEnum::SteelTools, 50, 20),
		MakeUpgrade(pestTrapText, LOCTEXT("Pests Traps Desc", "+30% productivity if there is an adjacent hunter (does not stack)."), ResourceEnum::Wood, 30),
	};
}

std::vector<BonusPair> GathererHut::GetBonuses()
{
	std::vector<BonusPair> bonuses = Building::GetBonuses();

	if (IsUpgraded(1) && adjacentCount(CardEnum::HuntingLodge) > 0) {
		bonuses.push_back({ pestTrapText, 30 });
	}

	return bonuses;
}


/*
 * HuntingLodge
 */
static const FText smokingChamberText = LOCTEXT("Smoking Chamber", "Smoking Chamber");
static const FText fruitBaitText = LOCTEXT("Fruit Bait", "Fruit Bait");

void HuntingLodge::OnInit()
{
	SetupWorkMode({
		WorkMode::Create(normalWorkModeText, normalWorkModeDesc),
		WorkMode::Create(PoisonArrowWorkModeText, LOCTEXT("PoisonArrow Desc", "Kill animals x4 faster but get -50% drop")),
	});
}

void HuntingLodge::FinishConstruction()
{
	Building::FinishConstruction();

	AddResourceHolder(ResourceEnum::Pork, ResourceHolderType::Provider, 0);

	_upgrades = {
		MakeProductionUpgrade(smokingChamberText, ResourceEnum::Stone, 50, 30),
		MakeUpgrade(fruitBaitText, LOCTEXT("Pests Traps Desc", "+30% productivity if there is an adjacent Fruit Gatherer (does not stack)."), ResourceEnum::Wood, 30),
	};
}

std::vector<BonusPair> HuntingLodge::GetBonuses()
{
	std::vector<BonusPair> bonuses = Building::GetBonuses();

	if (IsUpgraded(1) && adjacentCount(CardEnum::FruitGatherer) > 0) {
		bonuses.push_back({ fruitBaitText, 30 });
	}

	return bonuses;
}

/*
 * Forester
 */
void Forester::OnInit()
{
	SetupWorkMode({
		{ CutAndPlantText, ResourceEnum::None, ResourceEnum::None, 0},
		{ PrioritizePlantText, ResourceEnum::None, ResourceEnum::None, 0},
		{ PrioritizeCutText, ResourceEnum::None, ResourceEnum::None, 0},
	});
}

void Forester::FinishConstruction()
{
	Building::FinishConstruction();

	_upgrades = {
		MakeProductionUpgrade(LOCTEXT("Timber Management", "Timber Management"), ResourceEnum::Stone, 50, 30),
		MakeProductionUpgrade(LOCTEXT("Tree-felling Technique", "Tree-felling Technique"), ResourceEnum::Stone, 80, 50),
		MakeComboUpgrade(LOCTEXT("Forest Town", "Forest Town"), ResourceEnum::Wood, 50, 20),
	};
}

std::vector<BonusPair> Forester::GetBonuses()
{
	std::vector<BonusPair> bonuses = Building::GetBonuses();

	return bonuses;
}

/*
 * MushroomFarm
 */
const FText intensiveCareText = LOCTEXT("Intensive Care", "Intensive Care");
const FText intensiveCareDesc = LOCTEXT("Intensive Care Desc", "+30% production bonus when worker slots are full");

void MushroomFarm::FinishConstruction() {
	Building::FinishConstruction();

	_upgrades = {
		MakeUpgrade(intensiveCareText, intensiveCareDesc, ResourceEnum::Stone, 50),
	};
}

std::vector<BonusPair> MushroomFarm::GetBonuses() {
	std::vector<BonusPair> bonuses = Building::GetBonuses();
	if (IsUpgraded(0) && isOccupantFull()) {
		bonuses.push_back({ intensiveCareText, 30 });
	}
	return bonuses;
}

/*
 * ShroomFarm
 */
void ShroomFarm::FinishConstruction() {
	Building::FinishConstruction();

	_upgrades = {
		MakeUpgrade(intensiveCareText, intensiveCareDesc, ResourceEnum::SteelTools, 100),
		MakeProductionUpgrade(LOCTEXT("Substrate Treatment", "Substrate Treatment"), ResourceEnum::SteelTools, 200, 50),
	};
}

std::vector<BonusPair> ShroomFarm::GetBonuses() {
	std::vector<BonusPair> bonuses = Building::GetBonuses();
	if (IsUpgraded(0) && isOccupantFull()) {
		bonuses.push_back({ intensiveCareText, 30 });
	}
	return bonuses;
}

/*
 * Beekeeper
 */
void Beekeeper::FinishConstruction()
{
	Building::FinishConstruction();

	AddResourceHolder(ResourceEnum::Honey, ResourceHolderType::Provider, 0);

	_upgrades = {
		MakeUpgrade(intensiveCareText, intensiveCareDesc, ResourceEnum::Brick, 50),
		MakeComboUpgrade(LOCTEXT("Knowledge Sharing", "Knowledge Sharing"), ResourceEnum::Paper, 70, 50),
	};
}

std::vector<BonusPair> Beekeeper::GetBonuses()
{
	std::vector<BonusPair> bonuses = Building::GetBonuses();
	if (IsUpgraded(0) && isOccupantFull()) {
		bonuses.push_back({ intensiveCareText, 30 });
	}
	return bonuses;
}




/*
 * Farm
 */

void Farm::OnInit()
{
	// Set initial seed
	std::vector<SeedInfo> seeds = globalResourceSystem().seedsPlantOwned();

	if (seeds.size() > 0)
	{
		GeoresourceEnum georesourceEnum = _simulation->georesource(provinceId()).georesourceEnum;
		TileObjEnum targetPlantEnum = seeds[0].tileObjEnum;
		for (SeedInfo seedInfo : seeds) {
			if (seedInfo.georesourceEnum == GeoresourceEnum::None ||
				seedInfo.georesourceEnum == georesourceEnum)
			{
				targetPlantEnum = seedInfo.tileObjEnum;
				break;
			}
		}

		currentPlantEnum = targetPlantEnum;
	}
	// AI case, they don't buy seeds
	else
	{
		currentPlantEnum = GameRand::Rand() % 2 == 0 ? TileObjEnum::WheatBush : TileObjEnum::Cabbage;
		
		// Medicine farm count
		// 1 medicine farm per 50 ppl (2 herbs per year)
		const int32 citizensPerMedicineFarm = 50;
		const std::vector<int32>& farmIds = _simulation->buildingIds(_townId, CardEnum::Farm);
		if (farmIds.size() >= 4) // 5th farm is the medicine
		{
			int32 medicineFarmCount = 0;
			for (int32 farmId : farmIds) {
				if (_simulation->building<Farm>(farmId).currentPlantEnum == TileObjEnum::Herb) {
					medicineFarmCount++;
				}
			}
			if (citizensPerMedicineFarm * medicineFarmCount < _simulation->populationTown(_townId)) {
				currentPlantEnum = TileObjEnum::Herb;
			}
		}
	}
}

void Farm::FinishConstruction()
{
	Building::FinishConstruction();

	std::vector<SeedInfo> seedInfos = GetSeedInfos();
	for (int i = 0; i < seedInfos.size(); i++) {
		AddResourceHolder(GetTileObjInfo(seedInfos[i].tileObjEnum).harvestResourceEnum(), ResourceHolderType::Provider, 0);
	}

	_isTileWorked.resize(totalFarmTiles(), false);

	RefreshFertility();
	//_fertility = GetAverageFertility(area(), _simulation);
}

bool Farm::NoFarmerOnTileId(int32_t farmTileId) {
	return !CppUtils::Contains(_reservingUnitIdToFarmTileId, [&](const pair<int32, int32>& pair) { return pair.second == farmTileId; });
}

std::vector<BonusPair> Farm::GetBonuses()
{
	std::vector<BonusPair> bonuses = Building::GetBonuses();

	if (_simulation->IsResearched(_playerId, TechEnum::FarmingBreakthrough)) {
		bonuses.push_back({ LOCTEXT("Farm Breakthrough Upgrade", "Farm Breakthrough Upgrade"), 20 });
	}
	if (_simulation->IsResearched(_playerId, TechEnum::FarmImprovement)) {
		bonuses.push_back({ LOCTEXT("Farm Improvement Upgrade", "Farm Improvement Upgrade"), 5 });
	}
	if (_simulation->buildingFinishedCount(_townId, CardEnum::DepartmentOfAgriculture) &&
		_simulation->buildingCount(_townId, CardEnum::Farm) >= 8)
	{
		bonuses.push_back({ LOCTEXT("Department of Agriculture", "Department of Agriculture"), 5 });
	}
	if (_simulation->buildingFinishedCount(_townId, CardEnum::CensorshipInstitute)) {
		bonuses.push_back({ LOCTEXT("Censorship", "Censorship"), 7 });
	}

	if (_simulation->IsResearched(_playerId, TechEnum::FarmLastEra)) {
		bonuses.push_back({ LOCTEXT("Last Era Technology", "Last Era Technology"), 20 });
	}

	int32 radiusBonus = GetRadiusBonus(CardEnum::Windmill, Windmill::Radius, [&](int32 bonus, Building& building) {
		return max(bonus, 10);
	});
	if (radiusBonus > 0) {
		bonuses.push_back({ LOCTEXT("Near Windmill", "Near Windmill"), radiusBonus });
	}

	return bonuses;
}

WorldTile2 Farm::FindFarmableTile(int32 unitId) {
	WorldTile2 resultTile = WorldTile2::Invalid;

	_area.ExecuteOnAreaWithExit_Zero([&](int16 x, int16 y) 
	{
		int32 farmTileId = x + y * _area.sizeX();
		if (!_isTileWorked[farmTileId] && NoFarmerOnTileId(farmTileId))
		{
			_isTileWorked[farmTileId] = true;
			resultTile = _area.min() + WorldTile2(x, y);

			// Kick any animals out of this tile...
			_simulation->treeSystem().ForceRemoveTileReservation(resultTile);
			return true;
		}
		return false;
	});

	return resultTile;
}

bool Farm::IsStageCompleted() {
	// use _isTileWorked to determine this
	for (int i = 0; i < _isTileWorked.size(); i++) {
		if (!_isTileWorked[i]) return false;
	}
	return true;
}


int32 Farm::MinCropGrowthPercent()
{
	// Ready for harvest when 8/10 of plants are either full-grown, or was eaten.
	// Animals trim plant at 50% into 10%. Therefore, trimmed plant will have -40% slower growth.
	// With this, we ignore plant that has 60% or less when we already have 100% grown plant.
	// Also want at least 3/10 full grown tile before harvest..
	auto& treeSystem = _simulation->treeSystem();
	//int32_t readyTiles = 0;
	//int32_t trimmedTiles = 0;
	int32_t minGrowth = 200;
	_area.ExecuteOnArea_WorldTile2([&](WorldTile2 tile) {
		TileObjEnum tileObjEnum = treeSystem.tileObjEnum(tile.tileId());
		if (tileObjEnum == TileObjEnum::None) {
			return;
		}
		int32_t growth = GetTileObjInfo(tileObjEnum).growthPercent(treeSystem.tileObjAge(tile.tileId()));
		//if (growth == 100) readyTiles++;
		//if (growth <= 60) trimmedTiles++;
		if (growth < minGrowth) {
			minGrowth = growth;
		}
	});

	return minGrowth;

	//if (readyTiles < totalFarmTiles() * 3 / 10) {
	//	return false;
	//}

	//return (readyTiles + trimmedTiles) >= (totalFarmTiles() * 8 / 10);
}

void Farm::ReserveFarmTile(int32_t unitId, WorldTile2 tile) {
	_reservingUnitIdToFarmTileId.push_back(pair<int32_t, int32_t>(unitId, farmTileIdFromWorldTile(tile)));
}
void Farm::UnreserveFarmTile(int32_t unitId, WorldTile2 tile) {
	CppUtils::Remove(_reservingUnitIdToFarmTileId, pair<int32_t, int32_t>(unitId, farmTileIdFromWorldTile(tile)));
}

void Farm::DoFarmWork(int32_t unitId, WorldTile2 tile, FarmStage farmStage)
{
	// TODO: solve this properly ???
	//if (farmStage != _farmStage) {
	//	return;
	//}
	//PUN_CHECK2(farmStage == _farmStage, debugStr());


	auto& treeSystem = _simulation->treeSystem();

	switch (_farmStage)
	{
	case FarmStage::Seeding: {
		treeSystem.PlantBush(tile.tileId(), currentPlantEnum);

		// Play sound
		_simulation->soundInterface()->Spawn3DSound("CitizenAction", "CropPlanting", _simulation->unitAtom(unitId));
			
		break;
	}
	case FarmStage::Nourishing: {
		treeSystem.UnitNourishBush(tile);
		break;
	}
	case FarmStage::Harvesting: {
		ResourcePair resource = treeSystem.UnitHarvestBush(tile);

		if (resource.isValid())
		{
			int32 dropCount = GameRand::Rand100RoundTo1(resource.count * efficiency());

			// Farm multiplier for controlling farm production
			dropCount = GameRand::Rand100RoundTo1(dropCount * FarmFoodMultiplier100);

			dropCount = max(1, dropCount);

			resourceSystem().SpawnDrop(resource.resourceEnum, dropCount, tile, ResourceHolderType::DropManual);

			AddProductionStat(ResourcePair(resource.resourceEnum, dropCount));

			_simulation->unlockSystem(_playerId)->UpdateResourceProductionCount(resource.resourceEnum, dropCount);
//			// Quests
//#define UPDATE_QUEST(enumName) else if (product() == ResourceEnum::##enumName) { \
//				_simulation->QuestUpdateStatus(_playerId, QuestEnum::##enumName##Quest, dropCount); \
//			}
//			
//			if (product() == ResourceEnum::Cabbage) {
//				_simulation->QuestUpdateStatus(_playerId, QuestEnum::CabbageQuest, dropCount);
//			}
//			UPDATE_QUEST(Blueberries)
//			UPDATE_QUEST(Wheat)
//			UPDATE_QUEST(Pumpkin)
//			UPDATE_QUEST(Potato)
//			
//#undef UPDATE_QUEST
		}

		// Play sound
		_simulation->soundInterface()->Spawn3DSound("CitizenAction", "CropHarvesting", _simulation->unitAtom(unitId));
			
		break;
	}
	default:
		break;
		// TODO: figure out why it can reach this point...
		//UE_DEBUG_BREAK();
	}
}

void Farm::ClearAllPlants() {
	_simulation->treeSystem().ForceRemoveTileObjArea(_area);
	_simulation->SetNeedDisplayUpdate(DisplayClusterEnum::Trees, _area, true);
}

FText Farm::farmStageName()
{
	switch (_farmStage) {
	case FarmStage::Dormant: return LOCTEXT("Dormant", "Dormant");
	case FarmStage::Seeding: return LOCTEXT("Seeding", "Seeding");
	case FarmStage::Nourishing: return LOCTEXT("Nourishing", "Nourishing");
	case FarmStage::Harvesting: return LOCTEXT("Harvesting", "Harvesting");
	}
	UE_DEBUG_BREAK();
	return FText();
}

//! Mushroom

int32 MushroomFarm::baseInputPerBatch() {
	return _simulation->IsResearched(_playerId, TechEnum::MushroomSubstrateSterilization) ? 4 : 8;
}


/*
 * CardMaker
 */
static const FText productivityBookText =	LOCTEXT("Productivity Book", "Productivity Book");
static const FText sustainabilityBookText = LOCTEXT("Sustainability Book", "Sustainability Book");
static const FText frugalityBookText =		LOCTEXT("Frugality Book", "Frugality Book");
static const FText wildCardText =			LOCTEXT("Wild Card", "Wild Card");
static const FText cardRemovalCardText =	LOCTEXT("Card Removal Card", "Card Removal Card");

void CardMaker::OnInit()
{
	SetupWorkMode({
		WorkMode::Create(productivityBookText,		LOCTEXT("Productivity Book WorkDesc", "Create Productivity Book Card")),
		WorkMode::Create(sustainabilityBookText,	LOCTEXT("Sustainability Book WorkDesc", "Create Sustainability Book Card")),
		WorkMode::Create(frugalityBookText,			LOCTEXT("Frugality Book WorkDesc", "Create Frugality Book Card")),
		WorkMode::Create(wildCardText,				LOCTEXT("Wild Card WorkDesc", "Create Wild Card")),
		WorkMode::Create(cardRemovalCardText,		LOCTEXT("Card Removal Card WorkDesc", "Create Card Removal Card")),
	});
}

void CardMaker::FinishConstruction()
{
	ConsumerIndustrialBuilding::FinishConstruction();

	_upgrades = {
		MakeProductionUpgrade(LOCTEXT("Improved Production", "Improved Production"), ResourceEnum::SteelTools, 50, 30),
	};
}

std::vector<BonusPair> CardMaker::GetBonuses()
{
	std::vector<BonusPair> bonuses = ConsumerIndustrialBuilding::GetBonuses();

	return bonuses;
}

CardEnum CardMaker::GetCardProduced()
{
	const FText& name = workMode().name;

	if (name.IdenticalTo(productivityBookText)) {
		return CardEnum::ProductivityBook;
	}
	if (name.IdenticalTo(sustainabilityBookText)) {
		return CardEnum::SustainabilityBook;
	}
	if (name.IdenticalTo(frugalityBookText)) {
		return CardEnum::FrugalityBook;
	}
	if (name.IdenticalTo(wildCardText)) {
		return CardEnum::WildCard;
	}
	if (name.IdenticalTo(cardRemovalCardText)) {
		return CardEnum::CardRemoval;
	}

	checkNoEntry();
	return CardEnum::None;
}

/*
 * ImmigrationOffice
 */
void ImmigrationOffice::FinishConstruction() {
	ConsumerIndustrialBuilding::FinishConstruction();

	_upgrades = {
		MakeProductionUpgrade(LOCTEXT("First Impression", "First Impression"), ResourceEnum::Stone, 30, 30),
	};
}

std::vector<BonusPair> ImmigrationOffice::GetBonuses()
{
	std::vector<BonusPair> bonuses = ConsumerIndustrialBuilding::GetBonuses();

	return bonuses;
}

/*
 * Blacksmith
 */
void Blacksmith::FinishConstruction()
{
	IndustrialBuilding::FinishConstruction();

	_upgrades = {
		MakeProductionUpgrade(LOCTEXT("Improved Forge", "Improved Forge"), ResourceEnum::Brick, 50, 30),
		MakeProductionUpgrade(LOCTEXT("Alloy Recipe", "Alloy Recipe"), ResourceEnum::Paper, 50, 30),
		MakeComboUpgrade(LOCTEXT("Blacksmith Guild", "Blacksmith Guild"), ResourceEnum::Paper, 50, 25),
	};
}

/*
 * MedicineMaker
 */
void MedicineMaker::FinishConstruction() {
	Building::FinishConstruction();

	_upgrades = {
		MakeProductionUpgrade(LOCTEXT("Catalyst", "Catalyst"), 100, 30),
		MakeProductionUpgrade(LOCTEXT("Improved Extraction", "Improved Extraction"), 150, 50),
		MakeComboUpgrade(LOCTEXT("Pharmaceutical Guild", "Pharmaceutical Guild"), ResourceEnum::Paper, 50, 25),
	};
}

std::vector<BonusPair> MedicineMaker::GetBonuses()
{
	std::vector<BonusPair> bonuses = IndustrialBuilding::GetBonuses();

	return bonuses;
}

/*
 * CharcoalMaker
 */
void CharcoalMaker::FinishConstruction() {
	Building::FinishConstruction();

	_upgrades = {
		MakeUpgrade(LOCTEXT("Charcoal Conversion", "Charcoal Conversion"), LOCTEXT("Use 30% less wood input.", "Use 30% less wood input."), 20),
		MakeProductionUpgrade(LOCTEXT("Improved Production", "Improved Production"), 50, 50),
		MakeComboUpgrade(LOCTEXT("Charcoal Burner Guild", "Charcoal Burner Guild"), ResourceEnum::Wood, 30, 15),
	};
}

std::vector<BonusPair> CharcoalMaker::GetBonuses()
{
	std::vector<BonusPair> bonuses = IndustrialBuilding::GetBonuses();
	if (_simulation->buildingCount(_townId, CardEnum::EnvironmentalistGuild)) {
		bonuses.push_back({ LOCTEXT("Environmentalist", "Environmentalist"), -30 });
	}
	return bonuses;
}


/*
 * Chocolatier
 */
void Chocolatier::FinishConstruction()
{
	Building::FinishConstruction();

	_upgrades = {
		MakeUpgrade(LOCTEXT("Cocoa Processing", "Cocoa Processing"), LOCTEXT("Cocoa Processing Desc", "Consumes 50% less input."), ResourceEnum::Iron, 50),
		MakeProductionUpgrade(LOCTEXT("Improved Production", "Improved Production"), ResourceEnum::Iron, 50, 50),
		MakeUpgrade(LOCTEXT("Reduce Upkeep", "Reduce Upkeep"), LOCTEXT("Reduce Upkeep Desc", "Reduce upkeep by 50%"), ResourceEnum::Brick, 20),
		MakeComboUpgrade(LOCTEXT("Chocolate Town", "Chocolate Town"), ResourceEnum::Iron, 50, 25),
	};
}


/*
 * Winery
 */
void Winery::FinishConstruction() {
	Building::FinishConstruction();

	_upgrades = {
		MakeProductionUpgrade(LOCTEXT("Wine Appreciation", "Wine Appreciation"), 70, 50),
		MakeComboUpgrade(LOCTEXT("Wine Town", "Wine Town"), ResourceEnum::Brick, 50, 50),
	};
}

std::vector<BonusPair> Winery::GetBonuses()
{
	std::vector<BonusPair> bonuses = IndustrialBuilding::GetBonuses();
	if (_simulation->IsResearched(_playerId, TechEnum::WineryImprovement)) {
		bonuses.push_back({ LOCTEXT("Winery Improvement Tech", "Winery Improvement Tech"), 30 });
	}

	return bonuses;
}

/*
 * CoffeeRoaster
 */
void CoffeeRoaster::FinishConstruction() {
	Building::FinishConstruction();

	_upgrades = {
		MakeProductionUpgrade(LOCTEXT("Coffee Appreciation", "Coffee Appreciation"), 70, 50),
		MakeProductionUpgrade(LOCTEXT("Improved Roasting Stage", "Improved Roasting Stage"), 70, 50),
		MakeComboUpgrade(LOCTEXT("Coffee Town", "Coffee Town"), ResourceEnum::Brick, 50, 50),
	};
}


/*
 * Tailor
 */
void Tailor::OnInit()
{
	SetupWorkMode({
		{ LOCTEXT("Leather Clothes", "Leather Clothes"), ResourceEnum::Leather, ResourceEnum::None, 10},
		{ LOCTEXT("Wool Clothes", "Wool Clothes"), ResourceEnum::Wool, ResourceEnum::None, 10},

		{ LOCTEXT("Cotton Clothes (Cotton)", "Cotton Clothes (Cotton)"), ResourceEnum::Cotton, ResourceEnum::None, 10},
		{ LOCTEXT("Cotton Clothes (Cotton Fabric)", "Cotton Clothes (Cotton Fabric)"), ResourceEnum::CottonFabric, ResourceEnum::None, 10},

		{ LOCTEXT("Fashionable Clothes (Cotton & Dye)", "Fashionable Clothes (Cotton & Dye)"), ResourceEnum::Cotton, ResourceEnum::Dye, 10, ResourceEnum::LuxuriousClothes },
		{ LOCTEXT("Fashionable Clothes (Dyed Fabric)", "Fashionable Clothes (Dyed Fabric)"), ResourceEnum::DyedCottonFabric, ResourceEnum::None, 10, ResourceEnum::LuxuriousClothes },
		});
}

void Tailor::FinishConstruction() {
	Building::FinishConstruction();

	AddResourceHolder(ResourceEnum::Leather, ResourceHolderType::Requester, 0);
	AddResourceHolder(ResourceEnum::Wool, ResourceHolderType::Requester, 0);
	AddResourceHolder(ResourceEnum::CottonFabric, ResourceHolderType::Requester, 0);
	AddResourceHolder(ResourceEnum::Cloth, ResourceHolderType::Provider, 0);

	AddResourceHolder(ResourceEnum::DyedCottonFabric, ResourceHolderType::Requester, 0);
	AddResourceHolder(ResourceEnum::LuxuriousClothes, ResourceHolderType::Provider, 0);

	AddResourceHolder(ResourceEnum::Cotton, ResourceHolderType::Requester, 0);
	AddResourceHolder(ResourceEnum::Dye, ResourceHolderType::Requester, 0);

	ChangeWorkMode(_workMode);

	_upgrades = {
		MakeProductionUpgrade(LOCTEXT("Weaving Machine", "Weaving Machine"), ResourceEnum::Iron, 70, 55),
		MakeComboUpgrade(LOCTEXT("Tailor Town", "Tailor Town"), ResourceEnum::Iron, 70, 25),
	};
}


/*
 * BeerBrewery
 */
void BeerBrewery::OnInit()
{
	SetupWorkMode({
		{ LOCTEXT("Wheat Beer", "Wheat Beer"), ResourceEnum::Wheat, ResourceEnum::None, 10 },
		{ LOCTEXT("Orange Cider", "Orange Cider"), ResourceEnum::Orange, ResourceEnum::None, 10 },
		{ LOCTEXT("Mushroom Beer", "Mushroom Beer"), ResourceEnum::Mushroom, ResourceEnum::None, 10 },
	});
}

void BeerBrewery::FinishConstruction() {
	Building::FinishConstruction();

	AddResourceHolder(ResourceEnum::Wheat, ResourceHolderType::Requester, 0);
	AddResourceHolder(ResourceEnum::Orange, ResourceHolderType::Requester, 0);
	AddResourceHolder(ResourceEnum::Mushroom, ResourceHolderType::Requester, 0);
	AddResourceHolder(ResourceEnum::Beer, ResourceHolderType::Provider, 0);

	_upgrades = {
		MakeUpgrade(LOCTEXT("Improved Malting", "Improved Malting"), LOCTEXT("Consumes 30% less input.", "Consumes 30% less input."), ResourceEnum::Stone, 50),
		MakeProductionUpgrade(LOCTEXT("Fast Malting", "Fast Malting"), ResourceEnum::Stone, 50, 30),
		MakeComboUpgrade(LOCTEXT("Brewery Town", "Brewery Town"), ResourceEnum::Stone, 30, 20),
	};

	_simulation->TryAddQuest(_playerId, std::make_shared<BeerQuest>());

	ChangeWorkMode(_workMode); // Need this to setup resource target etc.
}

std::vector<BonusPair> BeerBrewery::GetBonuses()
{
	std::vector<BonusPair> bonuses = IndustrialBuilding::GetBonuses();
	if (_simulation->TownhallCardCountTown(_townId, CardEnum::MasterBrewer) > 0) {
		bonuses.push_back({ LOCTEXT("Master brewer", "Master brewer"), 30 });
	}

	return bonuses;
}

/*
 * BeerBreweryFamous
 */
std::vector<BonusPair> BeerBreweryFamous::GetBonuses()
{
	std::vector<BonusPair> bonuses = BeerBrewery::GetBonuses();
	bonuses.push_back({ LOCTEXT("Famous Brewery", "Famous Brewery"), 20 });
	return bonuses;
}

/*
 * VodkaDistillery
 */
void VodkaDistillery::FinishConstruction() {
	Building::FinishConstruction();

	_upgrades = {
		MakeUpgrade(LOCTEXT("Improved Fermentation", "Improved Fermentation"), LOCTEXT("Consumes 30% less input.", "Consumes 30% less input."), ResourceEnum::Stone, 50),
		MakeProductionUpgrade(LOCTEXT("Improved Filtration", "Improved Filtration"), ResourceEnum::Stone, 50, 30),
		MakeComboUpgrade(LOCTEXT("Vodka Town", "Vodka Town"), ResourceEnum::Stone, 30, 50),
	};
}

std::vector<BonusPair> VodkaDistillery::GetBonuses() 
{
	std::vector<BonusPair> bonuses = IndustrialBuilding::GetBonuses();
	if (_simulation->TownhallCardCountTown(_townId, CardEnum::MasterBrewer) > 0) {
		bonuses.push_back({ LOCTEXT("Master Brewer", "Master Brewer"), 30 });
	}

	return bonuses;
}


/*
 * Windmill
 */
void Windmill::FinishConstruction() {
	Building::FinishConstruction();

	_upgrades = {
		MakeProductionUpgrade(LOCTEXT("Improved Grinder", "Improved Grinder"), ResourceEnum::Stone, 30, 10),
	};
}

std::vector<BonusPair> Windmill::GetBonuses()
{
	std::vector<BonusPair> bonuses = IndustrialBuilding::GetBonuses();

	return bonuses;
}


/*
 * Bakery
 */
static const FText coalFireText = LOCTEXT("Coal-fired", "Coal-fired");
static const FText woodFireText = LOCTEXT("Wood-fired", "Wood-fired");

void Bakery::OnInit()
{
	SetupWorkMode({
		{ coalFireText, ResourceEnum::Flour, ResourceEnum::Coal, 5, ResourceEnum::None},
		{ woodFireText, ResourceEnum::Flour, ResourceEnum::Wood, 5, ResourceEnum::None,
				LOCTEXT("Wood-fired Desc", "Wood-fired oven cooks food faster locking in more nutrients. +30% productivity")},
	});
}

void Bakery::FinishConstruction() {
	Building::FinishConstruction();

	AddResourceHolder(ResourceEnum::Flour, ResourceHolderType::Requester, 0);
	AddResourceHolder(ResourceEnum::Coal, ResourceHolderType::Requester, 0);
	AddResourceHolder(ResourceEnum::Wood, ResourceHolderType::Requester, 0);
	AddResourceHolder(ResourceEnum::Bread, ResourceHolderType::Provider, 0);

	_upgrades = {
		MakeProductionUpgrade(LOCTEXT("Improved Oven", "Improved Oven"), ResourceEnum::Stone, 50, 10),
		MakeComboUpgrade(LOCTEXT("Baker Guild", "Baker Guild"), ResourceEnum::Paper, 50, 15),
	};

	ChangeWorkMode(_workMode); // Need this to setup resource target etc.
}

std::vector<BonusPair> Bakery::GetBonuses() {
	std::vector<BonusPair> bonuses = IndustrialBuilding::GetBonuses();

	if (workMode().name.IdenticalTo(woodFireText)) {
		bonuses.push_back({ woodFireText, 30 });
	}

	return bonuses;
}

/*
 * Jeweler
 */
void Jeweler::FinishConstruction() {
	Building::FinishConstruction();

	_upgrades = {
		MakeProductionUpgrade(LOCTEXT("Rigorous Training", "Rigorous Training"), ResourceEnum::Brick, 80, 50),
		MakeProductionUpgrade(LOCTEXT("Specialized Tools", "Specialized Tools"), ResourceEnum::SteelTools, 80, 50),
		MakeComboUpgrade(LOCTEXT("Jeweler's Guild", "Jeweler's Guild"), ResourceEnum::Brick, 50, 20),
	};
}

/*
 * Brickworks
 */
void Brickworks::FinishConstruction() {
	Building::FinishConstruction();

	_upgrades = {
		MakeProductionUpgrade(LOCTEXT("Specialized Tools", "Specialized Tools"), ResourceEnum::Stone, 50, 50),
		MakeComboUpgrade(LOCTEXT("Brickworks Town", "Brickworks Town"), ResourceEnum::Brick, 50, 20),
	};
}

/*
 * CandleMaker
 */
void CandleMaker::FinishConstruction() {
	Building::FinishConstruction();

	_upgrades = {
		MakeProductionUpgrade(LOCTEXT("Specialized Tools", "Specialized Tools"), ResourceEnum::SteelTools, 50, 50),
		MakeComboUpgrade(LOCTEXT("Candle Maker Guild", "Candle Maker Guild"), ResourceEnum::Brick, 50, 20),
	};
}


/*
 * CottonMill
 */
void CottonMill::OnInit()
{
	SetupWorkMode({
		{ LOCTEXT("Cotton Fabric", "Cotton Fabric"), ResourceEnum::Cotton, ResourceEnum::None, 10},
		{ LOCTEXT("Dyed Cotton Fabric", "Dyed Cotton Fabric"), ResourceEnum::Cotton, ResourceEnum::Dye, 10, ResourceEnum::DyedCottonFabric },
	});
}

void CottonMill::FinishConstruction() {
	Building::FinishConstruction();

	AddResourceHolder(ResourceEnum::Cotton, ResourceHolderType::Requester, 0);
	AddResourceHolder(ResourceEnum::Dye, ResourceHolderType::Requester, 0);
	AddResourceHolder(ResourceEnum::CottonFabric, ResourceHolderType::Requester, 0);
	AddResourceHolder(ResourceEnum::DyedCottonFabric, ResourceHolderType::Provider, 0);

	_upgrades = {
		MakeProductionUpgrade(LOCTEXT("Advanced Machinery", "Advanced Machinery"), ResourceEnum::Iron, 500, 300),
		MakeComboUpgrade(LOCTEXT("Cotton Mill Town", "Cotton Mill Town"), ResourceEnum::Iron, 80, 50),
	};
}

/*
 * PrintingPress
 */
void PrintingPress::FinishConstruction() {
	Building::FinishConstruction();

	_upgrades = {
		MakeProductionUpgrade(LOCTEXT("Advanced Machinery", "Advanced Machinery"), ResourceEnum::Iron, 500, 300),
		MakeComboUpgrade(LOCTEXT("Printing Press Town", "Printing Press Town"), ResourceEnum::Iron, 80, 50),
	};
}

/*
 * ClayPit
 */
void ClayPit::FinishConstruction() {
	Building::FinishConstruction();

	_upgrades = {
		MakeWorkerSlotUpgrade(50),
	};
}

/*
 * Potter
 */
void Potter::FinishConstruction() {
	Building::FinishConstruction();

	_upgrades = {
		MakeProductionUpgrade(LOCTEXT("Improved Kiln", "Improved Kiln"), 50, 30),
		MakeWorkerSlotUpgrade(30),
		MakeComboUpgrade(LOCTEXT("Potter Town", "Potter Town"), ResourceEnum::Stone, 50, 20),
	};

	_simulation->TryAddQuest(_playerId, std::make_shared<PotteryQuest>());
}

std::vector<BonusPair> Potter::GetBonuses()
{
	std::vector<BonusPair> bonuses = IndustrialBuilding::GetBonuses();
	if (_simulation->TownhallCardCountTown(_townId, CardEnum::MasterPotter) > 0) {
		bonuses.push_back({ LOCTEXT("Master Potter", "Master Potter"), 20 });
	}

	return bonuses;
}

/*
 * FurnitureWorkshop
 */
void FurnitureWorkshop::FinishConstruction() {
	Building::FinishConstruction();

	_upgrades = {
		MakeWorkerSlotUpgrade(30),
		MakeUpgrade(LOCTEXT("Minimalism", "Minimalism"), LOCTEXT("Consumes 30% less input.", "Consumes 30% less input."), 30),
		MakeComboUpgrade(LOCTEXT("Furniture Town", "Furniture Town"), ResourceEnum::Stone, 20, 20),
	};
}

std::vector<BonusPair> FurnitureWorkshop::GetBonuses() 
{
	std::vector<BonusPair> bonuses = IndustrialBuilding::GetBonuses();
	if (_simulation->IsResearched(_playerId, TechEnum::Sawmill)) {
		bonuses.push_back({ LOCTEXT("Sawmill Tech", "Sawmill Tech"), 50 });
	}

	return bonuses;
}


/*
 * Fisher
 */
void Fisher::FinishConstruction() {
	Building::FinishConstruction();

	_resourceDisplayShift = FVector(-10, -10, 0);

	//AddResourceHolder(ResourceEnum::WhaleMeat, ResourceHolderType::Provider, 0);

	_upgrades = {
		MakeProductionUpgrade(LOCTEXT("Juicier Bait", "Juicier Bait"), 50, 25),
		MakeProductionUpgrade(LOCTEXT("Improved Fishing Tools", "Improved Fishing Tools"), ResourceEnum::SteelTools, 140, 50),
		MakeWorkerSlotUpgrade(30),
		//BuildingUpgrade("Whaling", "Catch whale from deep sea instead.\n  Produces whale meat.\n  +2 worker slots.\n  No effect nearby fish population", 120)
	};
	// TODO: urchin harvester (luxury?)
	// There are about 950 species of sea urchins that inhabit a wide range of depth zones in all climates across the world’s oceans. About 18 of them are edible.

	PUN_LOG("FinishContruction Fisher %d", buildingId());

	_simulation->TryAddQuest(_playerId, std::make_shared<CooperativeFishingQuest>());
}

std::vector<BonusPair> Fisher::GetBonuses() 
{
	std::vector<BonusPair> bonuses = Building::GetBonuses();
	int32 cardCount = _simulation->TownhallCardCountTown(_townId, CardEnum::CooperativeFishing);
	if (cardCount > 0) {
		bonuses.push_back({ LOCTEXT("Cooperative Fishing", "Cooperative Fishing"), cardCount * 10 });
	}
	return bonuses;
}

void Fisher::ChangeFisherTilesInRadius(int32_t valueChange)
{
	auto& treeSystem = _simulation->treeSystem();
	TileArea area(_centerTile, Radius);
	area.EnforceWorldLimit();
	WorldAtom2 centerAtom = _centerTile.worldAtom2();

	area.ExecuteOnArea_WorldTile2([&] (WorldTile2 tile) {
		int32_t atomDist = WorldAtom2::Distance(tile.worldAtom2(), centerAtom);
		if (atomDist / CoordinateConstants::AtomsPerTile < Radius) {
			treeSystem.ChangeFisherCount(tile.tileId(), valueChange);
			_simulation->SetNeedDisplayUpdate(DisplayClusterEnum::Overlay, tile.regionId(), true);
		}
	});
}

// Extra fisher tile is when we are placing a fisher... extraFisherTile is for the fisher not yet placed but should affect the result...
// - Building placement.. alreadyPlaced false, extraFisherTile is none..
// - Placed Building with overlapping new placement.. alreadyPlaced = true, extraFisherTile is not none...
int32_t Fisher::FisherAreaEfficiency(WorldTile2 centerTile, bool alreadyPlaced, WorldTile2 extraFisherTile, IGameSimulationCore* simulation)
{
	auto& treeSystem = simulation->treeSystem();
	TileArea area(centerTile, Radius);
	area.EnforceWorldLimit();
	WorldAtom2 centerAtom = centerTile.worldAtom2();

	bool hasExtra = extraFisherTile.isValid();
	WorldAtom2 extraCenterAtom = extraFisherTile.worldAtom2();

	int32_t currentFisherAdd = alreadyPlaced ? -1 : 0; // Fisher already placed needs to remove itself before calculating the efficiency

	// 50% fish water is 100% efficiency
	// one fish is 100 
	int32_t fish100Count = 0;
	int32_t tileCount = 0;
	area.ExecuteOnArea_WorldTile2([&](WorldTile2 tile) {
		WorldAtom2 atom = tile.worldAtom2();
		int32_t atomDist = WorldAtom2::Distance(atom, centerAtom);

		if (atomDist / CoordinateConstants::AtomsPerTile < Radius) {
			bool shouldAddExtra = false;
			if (hasExtra) {
				int32_t extraAtomDist = WorldAtom2::Distance(atom, extraCenterAtom);
				shouldAddExtra = (extraAtomDist / CoordinateConstants::AtomsPerTile < Radius);
			}
			fish100Count += treeSystem.fish100Count(tile.tileId(), currentFisherAdd + shouldAddExtra);
			tileCount++;
		}
	});
	return fish100Count / tileCount * 2; // 2 (half water)
}


int32 Beekeeper::BeekeeperBaseEfficiency(int32 townId, WorldTile2 centerTileIn, IGameSimulationCore* simulation)
{
	const std::vector<int32>& buildings = simulation->buildingIds(townId, CardEnum::Beekeeper);

	// Adjust efficiency by distance linearly
	// efficiency from pairing with other windmill gets multiplied together for the final efficiency
	int32 efficiency = 100;
	int32 radiusTouchAtom = 2 * Radius * CoordinateConstants::AtomsPerTile;
	for (int32 buildingId : buildings) {
		WorldTile2 centerTile = simulation->building(buildingId).centerTile();
		if (centerTileIn != centerTile) {
			int32 atomDist = WorldAtom2::Distance(centerTileIn.worldAtom2(), centerTile.worldAtom2());
			if (atomDist < radiusTouchAtom) {
				int32 pairEfficiency = atomDist * 100 / radiusTouchAtom;
				efficiency = efficiency * pairEfficiency / 100;
			}
		}
	}

	TreeSystem& treeSys = simulation->treeSystem();
	TileArea area(centerTileIn, Radius);

	int32 treeCount = 0; // Full 250 trees
	const int32 treeCountAtFullEfficiency = 120;
	
	// After fullEff.. 1 tree gives half efficiency
	area.ExecuteOnArea_WorldTile2([&](WorldTile2 tile)
	{
		if (treeSys.tileInfo(tile.tileId()).type == ResourceTileType::Tree) {
			treeCount++;
		}
	});

	int32 treeCountForBase = std::min(treeCountAtFullEfficiency, treeCount);
	int32 treeCountForBonus = treeCount > treeCountAtFullEfficiency ? (treeCount - treeCountAtFullEfficiency) : 0;
	int32 treeEfficiency = 100 * treeCountForBase / treeCountAtFullEfficiency + (treeCountForBonus / 2);

	efficiency = treeEfficiency * efficiency / 100;
	
	return efficiency;
}

//! Mine

static const FText conserveResourceText = LOCTEXT("Conserve Resource", "Conserve Resource");
static const FText rapidMiningText = LOCTEXT("Rapid Mining", "Rapid Mining");

void Mine::OnInit()
{
	SetupWorkMode({
		WorkMode::Create(normalWorkModeText, normalWorkModeDesc),
		WorkMode::Create(conserveResourceText, LOCTEXT("Conserve resource desc", "-30% productivity.\nDeposit depletes 30% slower for each mined resource unit.")),
		WorkMode::Create(rapidMiningText, LOCTEXT("Rapid mining desc", "+30% productivity.\nDeposit depletes 30% faster for each mined resource unit.")),
	});
}

void Mine::FinishConstruction() {
	Building::FinishConstruction();

	_upgrades = {
		MakeWorkerSlotUpgrade(50, 2),
		MakeUpgrade(LOCTEXT("Improved shift", "Improved shift"), LOCTEXT("Mine with full worker slots get 20% productivity", "Mine with full worker slots get 20% productivity"), 40),
		MakeProductionUpgrade(LOCTEXT("Wide Shaft", "Wide Shaft"), ResourceEnum::Stone, 100, 50)
	};

}

std::vector<BonusPair> Mine::GetBonuses()
{
	std::vector<BonusPair> bonuses = Building::GetBonuses();
	if (_simulation->GetTownLvl(_townId) >= 3) {
		bonuses.push_back({ TownhallUpgradeBonusText(3), 10 });
	}
	if (IsUpgraded(1) && isOccupantFull()) {
		bonuses.push_back({ LOCTEXT("Improved Shift", "Improved Shift"), 20 });
	}
	if (_simulation->buildingCount(_townId, CardEnum::EnvironmentalistGuild)) {
		bonuses.push_back({ LOCTEXT("Environmentalist", "Environmentalist"), -30 });
	}

	if (_simulation->TownhallCardCountTown(_townId, CardEnum::MiningEquipment) > 0) {
		if (_simulation->buildingCount(_townId, CardEnum::Blacksmith) >= 1) {
			bonuses.push_back({ LOCTEXT("Mining Equipment", "Mining Equipment"), 30 });
		}
	}

	if (_workMode.name.IdenticalTo(conserveResourceText)) {
		bonuses.push_back({ conserveResourceText, -30 });
	}
	else if (_workMode.name.IdenticalTo(rapidMiningText)) {
		bonuses.push_back({ rapidMiningText, 30 });
	}

	return bonuses;
}

void Mine::OnProduce(int32 productionAmount)
{
	int32 depletionMultiplier = 100;
	if (_workMode.name.IdenticalTo(conserveResourceText)) {
		depletionMultiplier -= 30;
	} else if (_workMode.name.IdenticalTo(rapidMiningText)) {
		depletionMultiplier += 30;
	}
	if (slotCardCount(CardEnum::SustainabilityBook) > 0) {
		depletionMultiplier -= 50;
	}

	int32 depletedAmount = productionAmount * depletionMultiplier / 100;

	AddDepletionStat({ product(), depletedAmount });

	int32 provinceIdLocal = provinceId();
	PUN_ENSURE(provinceIdLocal != -1, return);

	if (isEnum(CardEnum::Quarry)) {
		_simulation->georesourceSystem().MineStone(provinceIdLocal, depletedAmount);
	} else {
		_simulation->georesourceSystem().MineOre(provinceIdLocal, depletedAmount);
	}
}

/*
 * Quarry
 */
std::vector<BonusPair> Quarry::GetBonuses()
{
	std::vector<BonusPair> bonuses = Mine::GetBonuses();
	if (_simulation->IsResearched(_playerId, TechEnum::QuarryImprovement)) {
		bonuses.push_back({ LOCTEXT("Quarry Improvement Tech", "Quarry Improvement Tech"), 30 });
	}
	return bonuses;
}

/*
 * GoldMine
 */
std::vector<BonusPair> GoldMine::GetBonuses()
{
	std::vector<BonusPair> bonuses = Mine::GetBonuses();
	if (_simulation->TownhallCardCountTown(_townId, CardEnum::GoldRush) > 0) {
		bonuses.push_back({ LOCTEXT("Gold Rush", "Gold Rush"), 30 });
	}
	return bonuses;
}

/*
 * Industrial Building
 */
std::vector<BonusPair> IndustrialBuilding::GetBonuses()
{
	std::vector<BonusPair> bonuses = Building::GetBonuses();
	if (_simulation->GetTownLvl(_townId) >= 5) {
		bonuses.push_back({ TownhallUpgradeBonusText(5), 10 });
	}
	return bonuses;
}

/*
 * PaperMaker
 */
void PaperMaker::FinishConstruction()
{
	_upgrades = {
		MakeUpgrade(LOCTEXT("Better process", "Better process"), LOCTEXT("Uses 50% less wood to produce paper.", "Uses 50% less wood to produce paper."), ResourceEnum::Brick, 50),
		MakeWorkerSlotUpgrade(30, 2),
	};

	Building::FinishConstruction();
}

/*
 * Smelter
 */
const FText teamworkText = LOCTEXT("Teamwork", "Teamwork");

void Smelter::FinishConstruction() {
	Building::FinishConstruction();

	_upgrades = {
		MakeUpgrade(teamworkText, LOCTEXT("Smelter Teamwork Desc", "Smelter with full worker slots get 50% production bonus"), ResourceEnum::Stone, 100),
		MakeUpgrade(LOCTEXT("Efficient Furnace", "Efficient Furnace"), LOCTEXT("Decrease input by 30%", "Decrease input by 30%"), ResourceEnum::Brick, 100),
		MakeComboUpgrade(
			FText::Format(LOCTEXT("UpgradeGuild", "{0} Guild"), buildingInfo().GetName()),
			ResourceEnum::Paper, 70, 30),
	};
}

std::vector<BonusPair> Smelter::GetBonuses()
{
	std::vector<BonusPair> bonuses = IndustrialBuilding::GetBonuses();
	if (_simulation->buildingCount(_townId, CardEnum::EnvironmentalistGuild)) {
		bonuses.push_back({ LOCTEXT("Environmentalist", "Environmentalist"), -30 });
	}
	if (IsUpgraded(0) && isOccupantFull()) {
		bonuses.push_back({ teamworkText, 50 });
	}

	if (_simulation->TownhallCardCountTown(_townId, CardEnum::CoalPipeline) > 0) {
		if (_simulation->resourceCountTown(_townId, ResourceEnum::Coal) >= 1000) {
			bonuses.push_back({ LOCTEXT("Coal Pipeline", "Coal Pipeline"), 30 });
		}
	}

	return bonuses;
}

/*
 * Iron Smelter
 */
std::vector<BonusPair> IronSmelter::GetBonuses()
{
	std::vector<BonusPair> bonuses = Smelter::GetBonuses();

	if (_simulation->TownhallCardCountTown(_townId, CardEnum::SmeltCombo) > 0) {
		if (adjacentCount(CardEnum::IronSmelter) > 0) {
			bonuses.push_back({ LOCTEXT("Iron Smelter Combo", "Iron Smelter Combo"), 30 });
		}
	}

	return bonuses;
}

/*
 * Mint
 */
void Mint::FinishConstruction() {
	ConsumerIndustrialBuilding::FinishConstruction();

	_upgrades = {
		MakeProductionUpgrade(LOCTEXT("Improved Production", "Improved Production"), ResourceEnum::Brick, 50, 30),
		MakeComboUpgrade(LOCTEXT("Mint Town", "Mint Town"), ResourceEnum::Brick, 50, 10),
	};
}

std::vector<BonusPair> Mint::GetBonuses()
{
	std::vector<BonusPair> bonuses = ConsumerIndustrialBuilding::GetBonuses();

	return bonuses;
}

/*
 * InventorsWorkshop
 */

void InventorsWorkshop::FinishConstruction() {
	ConsumerIndustrialBuilding::FinishConstruction();

	_upgrades = {
		MakeProductionUpgrade(LOCTEXT("Better Tools", "Better Tools"), ResourceEnum::SteelTools, 100, 50),
		MakeProductionUpgrade(LOCTEXT("Component Blueprints", "Component Blueprints"), ResourceEnum::Paper, 100, 50),
		MakeComboUpgrade(LOCTEXT("Inventor Guild", "Inventor Guild"), ResourceEnum::Brick, 50, 25),
	};
}

std::vector<BonusPair> InventorsWorkshop::GetBonuses()
{
	std::vector<BonusPair> bonuses = ConsumerIndustrialBuilding::GetBonuses();
	
	return bonuses;
}

/*
 * Barrack
 */
std::vector<BonusPair> Barrack::GetBonuses()
{
	std::vector<BonusPair> bonuses = ConsumerIndustrialBuilding::GetBonuses();

	if (_simulation->IsResearched(_playerId, TechEnum::MilitaryLastEra)) {
		bonuses.push_back({ LOCTEXT("Advanced Military", "Advanced Military"), 100 });
	}

	return bonuses;
}



//! Lovely Heart

void LovelyHeartStatue::Tick() {

	tickCount++;

	int32_t firstWaveSpeed = CoordinateConstants::AtomsPerTile / 10;
	int32_t secondWaveSpeed = CoordinateConstants::AtomsPerTile / 10;
	int32_t defaultSpeed = CoordinateConstants::AtomsPerTile / 10;

	int32_t shockwaveFinalRadius = CoordinateConstants::AtomsPerTile * 30;

	WorldAtom2 centerAtom = _centerTile.worldAtom2();
	TileArea area(_centerTile, 50);
	area.EnforceWorldLimit();
	auto& treeSystem = _simulation->treeSystem();

	int32_t firstRoundEnd = shockwaveFinalRadius / firstWaveSpeed;
	int32_t secondRoundBegin = firstRoundEnd + 180;
	int32_t secondRoundEnd = secondRoundBegin + shockwaveFinalRadius / secondWaveSpeed;
	int32_t thirdRoundBegin = secondRoundEnd + 180;
	int32_t thirdRoundEnd = thirdRoundBegin + shockwaveFinalRadius / defaultSpeed * 3 / 4; // /2 so we only need yellow in the middle

	int32_t round4Begin = thirdRoundEnd + 180;
	int32_t round4End = round4Begin + shockwaveFinalRadius / defaultSpeed * 7 / 8;
	int32_t round5Begin = round4End + 180;
	int32_t round5End = round5Begin + shockwaveFinalRadius / defaultSpeed * 7 / 8;

	auto pathAI = _simulation->pathAI(false);

	if (tickCount < firstRoundEnd) {
		int32_t destroyAtomRadius = tickCount * firstWaveSpeed;

		area.ExecuteOnArea_WorldTile2([&](WorldTile2 tile) {
			int32_t atomDist = WorldAtom2::Distance(tile.worldAtom2(), centerAtom);
			if (atomDist < destroyAtomRadius) {
				if (treeSystem.tileObjEnum(tile.tileId()) != TileObjEnum::None) {
					treeSystem.ForceRemoveTileObj(tile);
				}
			}
		});
	} 
	else if (secondRoundBegin <= tickCount && tickCount < secondRoundEnd) {
		int32_t plantingRadius = (tickCount - secondRoundBegin) * secondWaveSpeed;

		area.ExecuteOnArea_WorldTile2([&](WorldTile2 tile) {
			int32_t atomDist = WorldAtom2::Distance(tile.worldAtom2(), centerAtom);
			if (atomDist < plantingRadius && pathAI->isWalkable(tile.x, tile.y))
			{
				treeSystem.PlantTileObj(tile.tileId(), TileObjEnum::FieldFlowerYellow); // FieldFlowerPurple
			}
		});
	}
	else if (thirdRoundBegin <= tickCount && tickCount < thirdRoundEnd) {
		int32_t plantingRadius = (tickCount - thirdRoundBegin) * defaultSpeed;
		
		area.ExecuteOnArea_WorldTile2([&](WorldTile2 tile) {
			int32_t atomDist = WorldAtom2::Distance(tile.worldAtom2(), centerAtom);
			if (atomDist < plantingRadius && pathAI->isWalkable(tile.x, tile.y))
			{
				treeSystem.PlantTileObj(tile.tileId(), TileObjEnum::FieldFlowerYellow);
			}
		});
	}
	else if (round4Begin <= tickCount && tickCount < round4End) {
		int32_t plantingRadius = (tickCount - round4Begin) * defaultSpeed;

		area.ExecuteOnArea_WorldTile2([&](WorldTile2 tile) {
			int32_t atomDist = WorldAtom2::Distance(tile.worldAtom2(), centerAtom);
			if (atomDist < plantingRadius && pathAI->isWalkable(tile.x, tile.y))
			{
				treeSystem.PlantTileObj(tile.tileId(), TileObjEnum::FieldFlowerHeart);
			}
		});
	}
	else if (round5Begin <= tickCount && tickCount < round5End) {
		int32_t plantingRadius = (tickCount - round5Begin) * defaultSpeed;

		area.ExecuteOnArea_WorldTile2([&](WorldTile2 tile) {
			int32_t atomDist = WorldAtom2::Distance(tile.worldAtom2(), centerAtom);
			if (atomDist < plantingRadius && pathAI->isWalkable(tile.x, tile.y))
			{
				treeSystem.PlantTileObj(tile.tileId(), TileObjEnum::FieldFlowerPic);
			}
		});
	}

}


//! DragConstruction
void RoadConstruction::OnInit() 
{
	//PUN_LOG("Road OnInit %d constructed:%d", _objectId, _isConstructed);
	
	_simulation->overlaySystem().AddRoad(_centerTile, isEnum(CardEnum::DirtRoad), false, buildingId());
	
	//PUN_LOG("Road Init");
}
void RoadConstruction::FinishConstruction()
{
	//PUN_LOG("Road FinishConstruction %d constructed:%d", _objectId, _isConstructed);
	
	Building::FinishConstruction();

	// Remove the construction.
	_simulation->RemoveBuilding(_objectId);

	// Remove road
	if (overlaySystem().IsRoad(_centerTile)) {
		overlaySystem().RemoveRoad(_centerTile);
		//GameMap::RemoveFrontRoadTile(area.min());
		check(_simulation->IsFrontBuildable(_centerTile));
	}

	// Leaving behind just plain road...
	overlaySystem().AddRoad(_centerTile, isDirt(), true);
}


void FenceGate::FinishConstruction() 
{
	Building::FinishConstruction();
	_simulation->SetWalkableNonIntelligent(_centerTile, false);

	// Add road if there isn't one..
	if (!overlaySystem().IsRoad(_centerTile)) {
		_simulation->overlaySystem().AddRoad(_centerTile, true, true);
	}

	// Need to refresh surrounding tiles (skip self, no need)
	WorldTile2 tile = centerTile();
	_simulation->SetNeedDisplayUpdate(DisplayClusterEnum::Building, WorldTile2(tile.x + 1, tile.y).regionId(), true);
	_simulation->SetNeedDisplayUpdate(DisplayClusterEnum::Building, WorldTile2(tile.x - 1, tile.y).regionId(), true);
	_simulation->SetNeedDisplayUpdate(DisplayClusterEnum::Building, WorldTile2(tile.x, tile.y + 1).regionId(), true);
	_simulation->SetNeedDisplayUpdate(DisplayClusterEnum::Building, WorldTile2(tile.x, tile.y - 1).regionId(), true);
}
void FenceGate::OnDeinit() {
	_simulation->overlaySystem().RemoveRoad(_centerTile);
}

void Trap::OnTick1Sec()
{
	//PUN_LOG("Trap Tick1Sec %d", buildingId());

	if (!isConstructed()) {
		return;
	}

	auto& unitSystem = _simulation->unitSystem();
	auto& unitSubregionLists = unitSystem.unitSubregionLists();

	unitSubregionLists.ExecuteRegion(_centerTile.region(), [&](int32_t unitId)
	{
		WorldTile2 unitTile =  unitSystem.actualAtomLocation(unitId).worldTile2();
		//PUN_LOG("Trap try kill id: %d %s %s", unitId, *_centerTile.To_FString(), *unitTile.To_FString());

		if (_centerTile == unitTile) {
			//PUN_LOG("Trap same tile");

			UnitStateAI& unitAI = unitSystem.unitStateAI(unitId);

			// Unit not under player control gets killed
			if (unitAI.playerId() != _playerId)
			{
				PUN_LOG("Trap kill id:%d", unitId);
				unitAI.AttackIncoming(UnitFullId::Invalid(), -1, 100, _playerId);
			}
		}
	});
}

void Bank::CalculateRoundProfit()
{
	lastRoundProfit = GetRadiusBonus(CardEnum::House, Radius, [&](int32 bonus, Building& building) 
	{
		House& house = building.subclass<House>();

		// Check how many banks is taking this house...
		int32 bankCount = GetRadiusBonus(CardEnum::Bank, Radius, [&](int32 bonus1, Building& building1) {
			return bonus1 + 1;
		});

		if (bankCount <= 0) {
			bankCount = 1;
		}
		//PUN_CHECK(bankCount >= 1);

		return bonus + (house.houseLvl() >= 2 ? ProfitPerHouse : 0) / bankCount; // Profit shared between banks...
	});
}


//void Barrack::ScheduleTick()
//{
//	_queueCount--;
//	_trainingStartTick = -1;
//
//	std::vector<int32> armyCounts(ArmyEnumCount, 0);
//	armyCounts[static_cast<int>(_armyEnum)] = 1;
//	_simulation->townhall(_playerId).armyNode.AddArmyToCapital(_playerId, armyCounts);
//	
//	TryStartTraining(); // Try to start next training
//}

void ResourceOutpost::TickRound()
{
	if (isConstructed())
	{
		ResourceEnum resourceEnum = GetColonyResourceEnum();
		if (IsResourceValid(resourceEnum))
		{
			int32 resourceCount = GetColonyResourceIncome(resourceEnum);

			// Deplete province resource
			if (IsOreEnum(resourceEnum)) {
				resourceCount = min(oreLeft(), resourceCount);
				if (resourceCount > 0) {
					_simulation->georesourceSystem().MineOre(_simulation->GetProvinceIdRaw(centerTile()), resourceCount);
				}
			}

			if (resourceCount > 0) {
				resourceSystem().AddResourceGlobal(resourceEnum, resourceCount, *_simulation);
				AddProductionStat(ResourcePair(resourceEnum, resourceCount));
				
				_simulation->uiInterface()->ShowFloatupInfo(FloatupEnum::GainResource, centerTile(), TEXT_NUMSIGNED(resourceCount), resourceEnum);
			}
		}
	}
}

/*
 * IrrigationReservoir
 */
void IrrigationReservoir::FinishConstruction() {
	Building::FinishConstruction();

	_upgrades = {
		MakeUpgrade(LOCTEXT("Wind-powered Pump", "Wind-powered Pump"), LOCTEXT("Wind-powered Pump Desc", "Halve the upkeep if adjacent to Windmill."), 20),
	};

	ExecuteInRadius(CardEnum::Farm, Radius + 20, [&](Building& building) {
		building.subclass<Farm>().RefreshFertility();
	}); // extra 20 just in case it is farm's rim
}


#undef LOCTEXT_NAMESPACE 