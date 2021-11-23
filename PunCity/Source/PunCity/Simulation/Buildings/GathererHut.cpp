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

static const int32 ProductionUpgrade30 = 20;
static const int32 ProductionUpgrade50 = 30;

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
	AddResourceHolder(ResourceEnum::DateFruit, ResourceHolderType::Provider, 0);

	AddUpgrades({
		MakeProductionUpgrade(delicateGatheringText, ResourceEnum::SteelTools, ProductionUpgrade30),
		MakeUpgrade(pestTrapText, LOCTEXT("Pests Traps Desc", "+30% productivity if there is an adjacent hunter (does not stack)."), ResourceEnum::Wood, 30),
	});
}

std::vector<BonusPair> GathererHut::GetBonuses()
{
	std::vector<BonusPair> bonuses = Building::GetBonuses();

	if (IsUpgraded_InitialIndex(1) && adjacentCount(CardEnum::HuntingLodge) > 0) {
		bonuses.push_back({ pestTrapText, 30 });
	}

	if (_simulation->HasTownBonus(_townId, CardEnum::JungleGatherer)) {
		if (centerBiomeEnum() == BiomeEnum::Jungle) {
			bonuses.push_back({ NSLOCTEXT("FruitGatherer", "Jungle Gatherer Bonus", "Jungle Gatherer"), 30 });
		}
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

	AddUpgrades({
		MakeProductionUpgrade(smokingChamberText, ResourceEnum::Stone, ProductionUpgrade30),
		MakeUpgrade(fruitBaitText, LOCTEXT("Fruit Bait Desc", "+30% productivity if there is an adjacent Fruit Gatherer (does not stack)."), ResourceEnum::Wood, 30),
	});
}

std::vector<BonusPair> HuntingLodge::GetBonuses()
{
	std::vector<BonusPair> bonuses = Building::GetBonuses();

	if (IsUpgraded_InitialIndex(1) && adjacentCount(CardEnum::FruitGatherer) > 0) {
		bonuses.push_back({ fruitBaitText, 30 });
	}

	if (_simulation->HasTownBonus(_townId, CardEnum::SavannaHunt)) {
		if (IsGrassDominant(centerBiomeEnum())) {
			bonuses.push_back({ NSLOCTEXT("Ranch", "Savanna Ranch Bonus", "Savanna Ranch"), 50 });
		}
	}

	return bonuses;
}

/*
 * Forester
 */
void Forester::OnInit()
{
	SetupWorkMode({
		{ CutAndPlantText, ResourceEnum::None, ResourceEnum::None },
		{ PrioritizePlantText, ResourceEnum::None, ResourceEnum::None },
		{ PrioritizeCutText, ResourceEnum::None, ResourceEnum::None },
	});
}

void Forester::FinishConstruction()
{
	Building::FinishConstruction();

	AddUpgrades({
		MakeProductionUpgrade(LOCTEXT("Timber Management", "Timber Management"), ResourceEnum::Stone, ProductionUpgrade30),
		MakeProductionUpgrade(LOCTEXT("Tree-cutting Techniques", "Tree-cutting Techniques"), ResourceEnum::Stone, ProductionUpgrade50),
		MakeComboUpgrade(LOCTEXT("Forest Town", "Forest Town"), ResourceEnum::Wood),
	});
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

	AddUpgrades({
		MakeUpgrade(intensiveCareText, intensiveCareDesc, ResourceEnum::Stone, 50),
	});
}

std::vector<BonusPair> MushroomFarm::GetBonuses() {
	std::vector<BonusPair> bonuses = Building::GetBonuses();
	if (IsUpgraded_InitialIndex(0) && isOccupantFull()) {
		bonuses.push_back({ intensiveCareText, 30 });
	}

	if (_simulation->HasTownBonus(_townId, CardEnum::JungleMushroom)) {
		if (centerBiomeEnum() == BiomeEnum::Jungle) {
			bonuses.push_back({ NSLOCTEXT("MushroomFarm", "Jungle Mushroom Bonus", "Jungle Mushroom"), 30 });
		}
	}
	
	return bonuses;
}

/*
 * MagicMushroomFarm
 */
void ShroomFarm::FinishConstruction() {
	Building::FinishConstruction();

	AddUpgrades({
		MakeUpgrade(intensiveCareText, intensiveCareDesc, ResourceEnum::SteelTools, 100),
		MakeProductionUpgrade_WithHouseLvl(LOCTEXT("Substrate Treatment", "Substrate Treatment"), ResourceEnum::SteelTools, 5),
	});
}

std::vector<BonusPair> ShroomFarm::GetBonuses() {
	std::vector<BonusPair> bonuses = Building::GetBonuses();
	if (IsUpgraded_InitialIndex(0) && isOccupantFull()) {
		bonuses.push_back({ intensiveCareText, 30 });
	}

	if (_simulation->HasTownBonus(_townId, CardEnum::JungleMushroom)) {
		if (centerBiomeEnum() == BiomeEnum::Jungle) {
			bonuses.push_back({ NSLOCTEXT("MagicMushroomFarm", "Jungle Mushroom Bonus", "Jungle Mushroom"), 30 });
		}
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

	AddUpgrades({
		MakeUpgrade(intensiveCareText, intensiveCareDesc, ResourceEnum::Brick, 50),
		MakeProductionUpgrade_WithHouseLvl(LOCTEXT("Movable Frame Hive", "Movable Frame Hive"), ResourceEnum::Wood, 4),
		MakeComboUpgrade(LOCTEXT("Knowledge Sharing", "Knowledge Sharing"), ResourceEnum::Paper),
	});
}

std::vector<BonusPair> Beekeeper::GetBonuses()
{
	std::vector<BonusPair> bonuses = Building::GetBonuses();
	if (IsUpgraded_InitialIndex(0) && isOccupantFull()) {
		bonuses.push_back({ intensiveCareText, 30 });
	}
	return bonuses;
}




/*
 * Farm
 */

void Farm::OnPreInit()
{
	/*
	 * Calculate Farm Tiles
	 */
	_farmTiles = _simulation->GetFarmTiles(_area, _startTile, _playerId);
}

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

	// TODO: this prevents Birch showing up as crop
	// ?? This happens when farm was built outside georesource region, and wheat seeds etc. not unlocked?
	if (!globalResourceSystem().HasSeed(currentPlantEnum)) {
		currentPlantEnum = TileObjEnum::WheatBush;
	}
}

void Farm::FinishConstruction()
{
	Building::FinishConstruction();

	std::vector<SeedInfo> seedInfos = GetSeedInfos();
	for (int i = 0; i < seedInfos.size(); i++) {
		AddResourceHolder(GetTileObjInfo(seedInfos[i].tileObjEnum).harvestResourceEnum(), ResourceHolderType::Provider, 0);
	}

	RefreshFertility();
}

int32 Farm::GetFarmerReserverOnTileRow(int32 farmTileId)
{
	// Farmer reservedUnitId is placed at group's firstTile

	for (int32 i = farmTileId; i >= 0; i--)
	{
		if (_farmTiles[i].reservedUnitId != -1) {
			return _farmTiles[i].reservedUnitId;
		}
		
		int32 previousGroupId = -1;
		if (i > 0) {
			previousGroupId = _farmTiles[i - 1].groupId;
		}
		
		if (previousGroupId != _farmTiles[i].groupId) {
			break;
		}
	}

	return -1;
}

std::vector<BonusPair> Farm::GetBonuses()
{
	std::vector<BonusPair> bonuses = Building::GetBonuses();

	if (_simulation->IsResearched(_playerId, TechEnum::CropBreeding)) {
		bonuses.push_back({ LOCTEXT("Farm Breakthrough Upgrade", "Farm Breakthrough Upgrade"), 20 });
	}

	int32 farmingTechUpgradeCount = _simulation->GetTechnologyUpgradeCount(_playerId, TechEnum::FarmingTechnologies);
	if (farmingTechUpgradeCount > 0) {
		bonuses.push_back({ LOCTEXT("Farming Technologies", "Farming Technologies"), farmingTechUpgradeCount * 3 });
	}
	
	if (_simulation->townBuildingFinishedCount(_townId, CardEnum::DepartmentOfAgriculture) &&
		_simulation->buildingCount(_townId, CardEnum::Farm) >= 8)
	{
		bonuses.push_back({ LOCTEXT("Department of Agriculture", "Department of Agriculture"), 5 });
	}
	if (_simulation->townBuildingFinishedCount(_townId, CardEnum::CensorshipInstitute)) {
		bonuses.push_back({ LOCTEXT("Censorship", "Censorship"), 7 });
	}

	if (_simulation->IsResearched(_playerId, TechEnum::Fertilizers)) {
		bonuses.push_back({ LOCTEXT("Fertilizers", "Fertilizers"), 20 });
	}

	if (_simulation->TownhallCardCountTown(_townId, CardEnum::FarmWaterManagement) > 0) {
		bonuses.push_back({ LOCTEXT("Farm Water Management", "Farm Water Management"), 8 });
	}


	if (_simulation->HasTownBonus(_townId, CardEnum::DesertIndustry)) {
		bonuses.push_back({ LOCTEXT("Desert Industry", "Desert Industry"), -50 });
	}
	

	if (_simulation->HasTownBonus(_townId, CardEnum::JungleHerbFarm) && 
		centerBiomeEnum() == BiomeEnum::Jungle &&
		currentPlantEnum == TileObjEnum::Herb) 
	{
		bonuses.push_back({ LOCTEXT("Jungle Herb", "Jungle Herb"), 30 });
	}

	if (_simulation->HasTownBonus(_townId, CardEnum::ForestFarm) &&
		centerBiomeEnum() == BiomeEnum::Forest) {
		bonuses.push_back({ LOCTEXT("Improved Farming", "Improved Farming"), 10 });
	}

	{
		int32 radiusBonus = GetRadiusBonus(CardEnum::Windmill, Windmill::Radius, [&](int32 bonus, Building& building) {
			return max(bonus, 10);
		});
		if (radiusBonus > 0) {
			bonuses.push_back({ LOCTEXT("Near Windmill", "Near Windmill"), radiusBonus });
		}
	}

	if (_simulation->HasGlobalBonus(_playerId, CardEnum::Agriculturalist))
	{
		int32 radiusBonus = GetRadiusBonus(CardEnum::Granary, Windmill::Radius, [&](int32 bonus, Building& building) {
			return max(bonus, 10);
		});
		if (radiusBonus > 0) {
			bonuses.push_back({ LOCTEXT("Agriculturalist", "Agriculturalist"), radiusBonus });
		}
	}

	return bonuses;
}

FarmTile Farm::FindFarmableTile(int32 unitId)
{
	for (int32 i = 0; i < _farmTiles.size(); i++)
	{
		if (!_farmTiles[i].isTileWorked &&
			GetFarmerReserverOnTileRow(i) == -1)
		{
			// Kick any animals out of this tile...
			_simulation->treeSystem().ForceRemoveTileReservation(_farmTiles[i].worldTile);
			return _farmTiles[i];
		}
	}

	return FarmTile();
}

FarmTile Farm::GetNextFarmableTile(int32 farmTileId)
{
	if (farmTileId + 1 < _farmTiles.size() && 
		_farmTiles[farmTileId + 1].groupId == _farmTiles[farmTileId].groupId)
	{
		return _farmTiles[farmTileId + 1];
	}
	
	return FarmTile();
}

bool Farm::IsStageCompleted() {
	// use _isTileWorked to determine this
	for (int i = 0; i < _farmTiles.size(); i++) {
		if (!_farmTiles[i].isTileWorked) return false;
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

	int32_t minGrowth = 200;

	for (const FarmTile& farmTile: _farmTiles) 
	{
		TileObjEnum tileObjEnum = treeSystem.tileObjEnum(farmTile.worldTile.tileId());
		if (tileObjEnum != TileObjEnum::None) 
		{
			int32_t growth = GetTileObjInfo(tileObjEnum).growthPercent(treeSystem.tileObjAge(farmTile.worldTile.tileId()));
			if (growth < minGrowth) {
				minGrowth = growth;
			}
		}
	}

	return minGrowth;
}

void Farm::ReserveFarmTile(int32 unitId, const FarmTile& farmTile) {
	check(_farmTiles[farmTile.farmTileId].reservedUnitId == -1);
	_farmTiles[farmTile.farmTileId].reservedUnitId = unitId;
}
void Farm::UnreserveFarmTile(int32 farmTileId) {
	check(_farmTiles[farmTileId].reservedUnitId != -1);
	_farmTiles[farmTileId].reservedUnitId = -1;
}

void Farm::DoFarmWork(int32_t unitId, int32 farmTileId, FarmStage farmStage)
{
	// TODO: solve this properly ???
	//if (farmStage != _farmStage) {
	//	return;
	//}
	//PUN_CHECK2(farmStage == _farmStage, debugStr());


	auto& treeSystem = _simulation->treeSystem();
	
	WorldTile2 tile = _farmTiles[farmTileId].worldTile;

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
		UE_DEBUG_BREAK();
	}

	check(!_farmTiles[farmTileId].isTileWorked);
	_farmTiles[farmTileId].isTileWorked = true;
}

void Farm::ClearAllPlants()
{
	for (const FarmTile& farmTile : _farmTiles) {
		_simulation->treeSystem().ForceRemoveTileObj(farmTile.worldTile, false);
	}
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


/*
 * CardMaker
 */
static const FText productivityBookText =	LOCTEXT("Productivity Book", "Productivity Book");
static const FText sustainabilityBookText = LOCTEXT("Sustainability Book", "Sustainability Book");
static const FText frugalityBookText =		LOCTEXT("Frugality Book", "Frugality Book");
static const FText motivationBookText =		LOCTEXT("Motivation", "Motivation");
static const FText passionBookText =		LOCTEXT("Passion", "Passion");

static const FText wildCardText =			LOCTEXT("Wild Card", "Wild Card");
static const FText cardRemovalCardText =	LOCTEXT("Card Removal Card", "Card Removal Card");

void CardMaker::OnInit()
{
	ResetWorkModes();
}

void CardMaker::ResetWorkModes()
{
	SetupWorkMode({
		// !!!Note: Don't forget to change hardcoded description when changing params
		WorkMode::Create(wildCardText,				LOCTEXT("Wild Card WorkDesc", "Create Wild Card\n(20 Paper)"), ResourceEnum::Paper, ResourceEnum::None, ResourceEnum::None, 20),
		
		WorkMode::Create(productivityBookText,		LOCTEXT("Productivity Book WorkDesc", "Create Productivity Book Card\n(100 Paper)"), ResourceEnum::Paper, ResourceEnum::None, ResourceEnum::None, 100),
		WorkMode::Create(sustainabilityBookText,	LOCTEXT("Sustainability Book WorkDesc", "Create Sustainability Book Card\n(100 Paper)"), ResourceEnum::Paper, ResourceEnum::None, ResourceEnum::None, 100),
		WorkMode::Create(frugalityBookText,			LOCTEXT("Frugality Book WorkDesc", "Create Frugality Book Card\n(100 Paper)"), ResourceEnum::Paper, ResourceEnum::None, ResourceEnum::None, 100),

		WorkMode::Create(cardRemovalCardText,		LOCTEXT("Card Removal Card WorkDesc", "Create Card Removal Card\n(20 Paper)"), ResourceEnum::Paper, ResourceEnum::None, ResourceEnum::None, 20),
	});

	if (_simulation->IsResearched(_playerId, TechEnum::SocialScience)) {
		AddWorkMode(WorkMode::Create(motivationBookText, LOCTEXT("Motivation WorkDesc", "Create Motivation Card\n(200 Paper)"), ResourceEnum::Paper, ResourceEnum::None, ResourceEnum::None, 200));
		AddWorkMode(WorkMode::Create(passionBookText, LOCTEXT("Passion WorkDesc", "Create Passion Card\n(200 Paper)"), ResourceEnum::Paper, ResourceEnum::None, ResourceEnum::None, 200));
	}
}


void CardMaker::FinishConstruction()
{
	ConsumerIndustrialBuilding::FinishConstruction();

	AddUpgrades({
		MakeProductionUpgrade_WithHouseLvl(LOCTEXT("Improved Production", "Improved Production"), ResourceEnum::SteelTools, 4),
	});
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
	if (name.IdenticalTo(motivationBookText)) {
		return CardEnum::Motivation;
	}
	if (name.IdenticalTo(passionBookText)) {
		return CardEnum::Passion;
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

	AddUpgrades({
		MakeUpgrade(LOCTEXT("Immigrants Dream", "Immigrant's Dream"), LOCTEXT("Immigrants Dream Desc", "+2% Productivity for Every 1% Average Happiness above 75%."), 20),
		MakeProductionUpgrade_Money(LOCTEXT("First Impression", "First Impression"), ProductionUpgrade30),
	});
}

std::vector<BonusPair> ImmigrationOffice::GetBonuses()
{
	std::vector<BonusPair> bonuses = ConsumerIndustrialBuilding::GetBonuses();

	if (IsUpgraded_InitialIndex(0)) {
		bonuses.push_back({LOCTEXT("Immigrants Dream", "Immigrants Dream"), max(0, _simulation->GetAverageHappiness(_townId) - 75) * 2 });
	}

	return bonuses;
}

/*
 * StoneToolsShop
 */
std::vector<BonusPair> StoneToolsShop::GetBonuses()
{
	std::vector<BonusPair> bonuses = IndustrialBuilding::GetBonuses();

	if (_simulation->HasTownBonus(_townId, CardEnum::ForestTools)) {
		bonuses.push_back({ LOCTEXT("Improved Toolmaking (Forest Biome)", "Improved Toolmaking (Forest Biome)"), 20 });
	}
	
	return bonuses;
}

/*
 * Blacksmith
 */
void Blacksmith::FinishConstruction()
{
	IndustrialBuilding::FinishConstruction();

	AddUpgrades({
		MakeProductionUpgrade(LOCTEXT("Improved Forge", "Improved Forge"), ResourceEnum::Brick, ProductionUpgrade30),
		MakeProductionUpgrade_WithHouseLvl(LOCTEXT("Alloy Recipe", "Alloy Recipe"), ResourceEnum::Paper, 4),
		MakeComboUpgrade(LOCTEXT("Blacksmith Guild", "Blacksmith Guild"), ResourceEnum::Paper),
	});
}

std::vector<BonusPair> Blacksmith::GetBonuses()
{
	std::vector<BonusPair> bonuses = IndustrialBuilding::GetBonuses();

	if (_simulation->HasTownBonus(_townId, CardEnum::ForestTools)) {
		bonuses.push_back({ LOCTEXT("Improved Toolmaking (Forest)", "Improved Toolmaking (Forest)"), 20 });
	}

	if (_simulation->HasGlobalBonus(_playerId, CardEnum::Craftmanship)) {
		bonuses.push_back({ LOCTEXT("Craftmanship", "Craftmanship"), 20 });
	}

	return bonuses;
}

/*
 * MedicineMaker
 */
void MedicineMaker::FinishConstruction() {
	Building::FinishConstruction();

	AddResourceHolder(ResourceEnum::Mushroom, ResourceHolderType::Requester, 0);
	AddResourceHolder(ResourceEnum::Wood, ResourceHolderType::Requester, 0);

	SetupWorkMode({
		WorkMode::Create(LOCTEXT("Medicinal Herb Input", "Medicinal Herb Input"),	FText(), ResourceEnum::Herb),
		WorkMode::Create(LOCTEXT("Mushroom Input", "Mushroom Extract"),	FText(), ResourceEnum::Mushroom),
		WorkMode::Create(LOCTEXT("Wood Input", "Bark-based Extract (Wood)"),	FText(), ResourceEnum::Wood),
	});

	AddUpgrades({
		MakeProductionUpgrade_Money(LOCTEXT("Catalyst", "Catalyst"), ProductionUpgrade30),
		MakeProductionUpgrade_WithHouseLvl(LOCTEXT("Improved Extraction", "Improved Extraction"), ResourceEnum::Wood, 4),
		MakeComboUpgrade(LOCTEXT("Pharmaceutical Guild", "Pharmaceutical Guild"), ResourceEnum::Paper),
	});
}

std::vector<BonusPair> MedicineMaker::GetBonuses()
{
	std::vector<BonusPair> bonuses = IndustrialBuilding::GetBonuses();

	return bonuses;
}

/*
 * CharcoalMaker
 */
void CharcoalBurner::FinishConstruction() {
	Building::FinishConstruction();

	AddUpgrades({
		MakeUpgrade(LOCTEXT("Charcoal Conversion", "Charcoal Conversion"), LOCTEXT("Use 30% less wood input.", "Use 30% less wood input."), 20),
		MakeProductionUpgrade_Money(LOCTEXT("Improved Production", "Improved Production"), 50),
		MakeComboUpgrade(LOCTEXT("Charcoal Burner Guild", "Charcoal Burner Guild"), ResourceEnum::Wood),
	});
}

std::vector<BonusPair> CharcoalBurner::GetBonuses()
{
	std::vector<BonusPair> bonuses = IndustrialBuilding::GetBonuses();
	if (_simulation->buildingCount(_townId, CardEnum::EnvironmentalistGuild)) {
		bonuses.push_back({ LOCTEXT("Environmentalist", "Environmentalist"), -30 });
	}
	if (_simulation->IsResearched(_playerId, TechEnum::CharcoalBurnerImprovement)) {
		bonuses.push_back({ LOCTEXT("Charcoal Burner Improvement", "Charcoal Burner Improvement"), 30 });
	}

	if (_simulation->HasTownBonus(_townId, CardEnum::ForestCharcoal)) {
		bonuses.push_back({ LOCTEXT("Improved Charcoal Making (Forest)", "Improved Charcoal Making (Forest)"), 30 });
	}
	
	return bonuses;
}


/*
 * Chocolatier
 */
void Chocolatier::FinishConstruction()
{
	Building::FinishConstruction();

	AddUpgrades({
		MakeUpgrade(LOCTEXT("Cocoa Processing", "Cocoa Processing"), LOCTEXT("Cocoa Processing Desc", "Consumes 50% less input."), ResourceEnum::Iron, 50),
		MakeProductionUpgrade_WithHouseLvl(LOCTEXT("Improved Production", "Improved Production"), ResourceEnum::Iron, 6),
		MakeComboUpgrade(LOCTEXT("Chocolate Town", "Chocolate Town"), ResourceEnum::Iron),
	});
}

std::vector<BonusPair> Chocolatier::GetBonuses()
{
	std::vector<BonusPair> bonuses = IndustrialBuilding::GetBonuses();
	
	if (_simulation->IsResearched(_playerId, TechEnum::ChocolateSnob)) {
		bonuses.push_back({ LOCTEXT("Chocolate Snob Tech", "Chocolate Snob Tech"), 30 });
	}

	return bonuses;
}


/*
 * Winery
 */
void Winery::FinishConstruction() {
	Building::FinishConstruction();

	AddUpgrades({
		MakeProductionUpgrade_Money(LOCTEXT("Wine Appreciation", "Wine Appreciation"), 30),
		MakeProductionUpgrade_WithHouseLvl(LOCTEXT("Wine Recipes", "Wine Recipes"), ResourceEnum::Paper, 5),
		MakeComboUpgrade(LOCTEXT("Wine Town", "Wine Town"), ResourceEnum::Brick),
	});
}

std::vector<BonusPair> Winery::GetBonuses()
{
	std::vector<BonusPair> bonuses = IndustrialBuilding::GetBonuses();
	if (_simulation->IsResearched(_playerId, TechEnum::WinerySnob)) {
		bonuses.push_back({ LOCTEXT("Wine Snob Tech", "Wine Snob Tech"), 30 });
	}

	if (_simulation->HasGlobalBonus(_playerId, CardEnum::AlcoholAppreciation)) {
		bonuses.push_back({ LOCTEXT("Alcohol Appreciation", "Alcohol Appreciation"), 15 });
	}

	return bonuses;
}

/*
 * CoffeeRoaster
 */
void CoffeeRoaster::FinishConstruction() {
	Building::FinishConstruction();

	AddUpgrades({
		MakeProductionUpgrade_Money(LOCTEXT("Coffee Appreciation", "Coffee Appreciation"), 30),
		MakeProductionUpgrade_WithHouseLvl(LOCTEXT("Improved Roasting Stage", "Improved Roasting Stage"), ResourceEnum::Paper, 5),
		MakeComboUpgrade(LOCTEXT("Coffee Town", "Coffee Town"), ResourceEnum::Brick),
	});
}


/*
 * Tailor
 */
void Tailor::OnInit()
{
	ResetWorkModes();
	//SetupWorkMode({
	//	{ LOCTEXT("Leather Clothes", "Leather Clothes"), ResourceEnum::Leather, ResourceEnum::None },
	//	{ LOCTEXT("Wool Clothes", "Wool Clothes"), ResourceEnum::Wool, ResourceEnum::None },

	//	{ LOCTEXT("Cotton Clothes (Cotton)", "Cotton Clothes (Cotton)"), ResourceEnum::Cotton, ResourceEnum::None },
	//	{ LOCTEXT("Cotton Clothes (Cotton Fabric)", "Cotton Clothes (Cotton Fabric)"), ResourceEnum::CottonFabric, ResourceEnum::None },

	//	{ LOCTEXT("Fashionable Clothes (Dyed Fabric)", "Fashionable Clothes (Dyed Fabric)"), ResourceEnum::DyedCottonFabric, ResourceEnum::None, ResourceEnum::LuxuriousClothes },
	//});
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

	AddUpgrades({
		MakeProductionUpgrade(LOCTEXT("Weaving Machine", "Weaving Machine"), ResourceEnum::Iron, 40),
		MakeProductionUpgrade_WithHouseLvl(LOCTEXT("Weaver Recipes", "Weaver Recipes"), ResourceEnum::Paper, 4),
		MakeComboUpgrade(LOCTEXT("Tailor Town", "Tailor Town"), ResourceEnum::Iron),
	});
}

void Tailor::ResetWorkModes()
{
	SetupWorkMode({
		{ LOCTEXT("Leather Clothes", "Leather Clothes"), ResourceEnum::Leather, ResourceEnum::None },
		{ LOCTEXT("Wool Clothes", "Wool Clothes"), ResourceEnum::Wool, ResourceEnum::None },

		{ LOCTEXT("Cotton Clothes (Cotton)", "Cotton Clothes (Cotton)"), ResourceEnum::Cotton, ResourceEnum::None },
		{ LOCTEXT("Cotton Clothes (Cotton Fabric)", "Cotton Clothes (Cotton Fabric)"), ResourceEnum::CottonFabric, ResourceEnum::None },

		//{ LOCTEXT("Fashionable Clothes (Dyed Fabric)", "Fashionable Clothes (Dyed Fabric)"), ResourceEnum::DyedCottonFabric, ResourceEnum::None, ResourceEnum::LuxuriousClothes },
	});

	if (_simulation->IsResearched(_playerId, TechEnum::HighFashion)) {
		AddWorkMode(WorkMode::Create(LOCTEXT("Fashionable Clothes (Dyed Fabric)", "Fashionable Clothes (Dyed Fabric)"), FText(), ResourceEnum::DyedCottonFabric, ResourceEnum::None, ResourceEnum::LuxuriousClothes));
	}
}


/*
 * BeerBrewery
 */
void BeerBrewery::OnInit()
{
	SetupWorkMode({
		{ LOCTEXT("Wheat Beer", "Wheat Beer"), ResourceEnum::Wheat, ResourceEnum::None },
		{ LOCTEXT("Orange Cider", "Orange Cider"), ResourceEnum::Orange, ResourceEnum::None },
		{ LOCTEXT("Mushroom Beer", "Mushroom Beer"), ResourceEnum::Mushroom, ResourceEnum::None },
		{ LOCTEXT("Pumpkin Beer", "Pumpkin Beer"), ResourceEnum::Pumpkin, ResourceEnum::None },
		{ LOCTEXT("Honey Mead", "Honey Mead"), ResourceEnum::Honey, ResourceEnum::None },
	});
}

void BeerBrewery::FinishConstruction() {
	Building::FinishConstruction();

	AddResourceHolder(ResourceEnum::Wheat, ResourceHolderType::Requester, 0);
	AddResourceHolder(ResourceEnum::Orange, ResourceHolderType::Requester, 0);
	AddResourceHolder(ResourceEnum::Mushroom, ResourceHolderType::Requester, 0);
	AddResourceHolder(ResourceEnum::Pumpkin, ResourceHolderType::Requester, 0);
	AddResourceHolder(ResourceEnum::Honey, ResourceHolderType::Requester, 0);
	
	AddResourceHolder(ResourceEnum::Beer, ResourceHolderType::Provider, 0);

	AddUpgrades({
		MakeUpgrade(LOCTEXT("Improved Malting", "Improved Malting"), LOCTEXT("Consumes 30% less input.", "Consumes 30% less input."), ResourceEnum::Stone, 50),
		MakeProductionUpgrade(LOCTEXT("Fast Malting", "Fast Malting"), ResourceEnum::Stone, ProductionUpgrade30),
		MakeComboUpgrade(LOCTEXT("Brewery Town", "Brewery Town"), ResourceEnum::Stone),
	});

	_simulation->TryAddQuest(_playerId, std::make_shared<BeerQuest>());

	ChangeWorkMode(_workMode); // Need this to setup resource target etc.
}

std::vector<BonusPair> BeerBrewery::GetBonuses()
{
	std::vector<BonusPair> bonuses = IndustrialBuilding::GetBonuses();
	if (_simulation->TownhallCardCountTown(_townId, CardEnum::MasterBrewer) > 0) {
		bonuses.push_back({ LOCTEXT("Master brewer", "Master brewer"), 30 });
	}

	if (_simulation->HasTownBonus(_townId, CardEnum::ForestBeer)) {
		bonuses.push_back({ LOCTEXT("Improved Beer-Brewing (Forest)", "Improved Beer-Brewing (Forest)"), 30 });
	}

	if (_simulation->HasGlobalBonus(_playerId, CardEnum::AlcoholAppreciation)) {
		bonuses.push_back({ LOCTEXT("Alcohol Appreciation", "Alcohol Appreciation"), 15 });
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

	AddUpgrades({
		MakeUpgrade(LOCTEXT("Improved Fermentation", "Improved Fermentation"), LOCTEXT("Consumes 30% less input.", "Consumes 30% less input."), ResourceEnum::Stone, 50),
		MakeProductionUpgrade(LOCTEXT("Improved Filtration", "Improved Filtration"), ResourceEnum::Stone, ProductionUpgrade30),
		MakeComboUpgrade(LOCTEXT("Vodka Town", "Vodka Town"), ResourceEnum::Stone),
	});
}

std::vector<BonusPair> VodkaDistillery::GetBonuses() 
{
	std::vector<BonusPair> bonuses = IndustrialBuilding::GetBonuses();
	if (_simulation->TownhallCardCountTown(_townId, CardEnum::MasterBrewer) > 0) {
		bonuses.push_back({ LOCTEXT("Master Brewer", "Master Brewer"), 30 });
	}

	if (_simulation->HasGlobalBonus(_playerId, CardEnum::AlcoholAppreciation)) {
		bonuses.push_back({ LOCTEXT("Alcohol Appreciation", "Alcohol Appreciation"), 15 });
	}

	return bonuses;
}


/*
 * Windmill
 */
void Windmill::FinishConstruction() {
	Building::FinishConstruction();

	AddUpgrades({
		MakeProductionUpgrade(LOCTEXT("Improved Grinder", "Improved Grinder"), ResourceEnum::Stone, ProductionUpgrade30),
	});
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
		{ coalFireText, ResourceEnum::Flour, ResourceEnum::Coal, ResourceEnum::None, FText(), 8, 2},
		{ woodFireText, ResourceEnum::Flour, ResourceEnum::Wood, ResourceEnum::None,
			LOCTEXT("Wood-fired Desc", "Wood-fired oven cooks food faster locking in more nutrients. +30% productivity"),
			8, 2
		},
	});
}

void Bakery::FinishConstruction() {
	Building::FinishConstruction();

	AddResourceHolder(ResourceEnum::Flour, ResourceHolderType::Requester, 0);
	AddResourceHolder(ResourceEnum::Coal, ResourceHolderType::Requester, 0);
	AddResourceHolder(ResourceEnum::Wood, ResourceHolderType::Requester, 0);
	AddResourceHolder(GetBuildingInfo(buildingEnum()).produce, ResourceHolderType::Provider, 0);

	AddUpgrades({
		MakeProductionUpgrade(LOCTEXT("Improved Oven", "Improved Oven"), ResourceEnum::Brick, ProductionUpgrade30),
		MakeProductionUpgrade_WithHouseLvl(LOCTEXT("Baking Recipe", "Baking Recipe"), ResourceEnum::Paper, 5),
		MakeComboUpgrade(LOCTEXT("Baker Guild", "Baker Guild"), ResourceEnum::Paper),
	});

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

	AddUpgrades({
		MakeProductionUpgrade(LOCTEXT("Rigorous Training", "Rigorous Training"), ResourceEnum::Brick, ProductionUpgrade30),
		MakeProductionUpgrade_WithHouseLvl(LOCTEXT("Specialized Tools", "Specialized Tools"), ResourceEnum::SteelTools, 7),
		MakeComboUpgrade(LOCTEXT("Jeweler's Guild", "Jeweler's Guild"), ResourceEnum::Brick),
	});
}

/*
 * Brickworks
 */
void Brickworks::FinishConstruction() {
	Building::FinishConstruction();

	AddUpgrades({
		MakeProductionUpgrade(LOCTEXT("Specialized Tools", "Specialized Tools"), ResourceEnum::Stone, ProductionUpgrade30),
		MakeComboUpgrade(LOCTEXT("Brickworks Town", "Brickworks Town"), ResourceEnum::Brick),
	});
}


/*
 * CarpetWeaver
 */
void CarpetWeaver::FinishConstruction() {
	Building::FinishConstruction();

	AddUpgrades({
		MakeProductionUpgrade(LOCTEXT("Specialized Tools", "Specialized Tools"), ResourceEnum::SteelTools, ProductionUpgrade30),
		MakeProductionUpgrade_WithHouseLvl(LOCTEXT("Craftmanship", "Craftmanship"), ResourceEnum::Money, 4),
		MakeComboUpgrade(LOCTEXT("Carpet Weaver Guild", "Carpet Weaver Guild"), ResourceEnum::Brick),
	});
}

/*
 * CandleMaker
 */
void CandleMaker::FinishConstruction() {
	Building::FinishConstruction();

	AddUpgrades({
		MakeProductionUpgrade(LOCTEXT("Specialized Tools", "Specialized Tools"), ResourceEnum::SteelTools, ProductionUpgrade30),
		MakeComboUpgrade(LOCTEXT("Candle Maker Guild", "Candle Maker Guild"), ResourceEnum::Brick),
	});
}


/*
 * CottonMill
 */
void CottonMill::OnInit()
{
	SetupWorkMode({
		{ LOCTEXT("Cotton Fabric", "Cotton Fabric"), ResourceEnum::Cotton, ResourceEnum::None },
		{ LOCTEXT("Dyed Cotton Fabric", "Dyed Cotton Fabric"), ResourceEnum::Cotton, ResourceEnum::Dye, ResourceEnum::DyedCottonFabric, FText(), 8, 2 },
	});
}

void CottonMill::FinishConstruction() {
	Building::FinishConstruction();

	AddResourceHolder(ResourceEnum::Cotton, ResourceHolderType::Requester, 0);
	AddResourceHolder(ResourceEnum::Dye, ResourceHolderType::Requester, 0);
	AddResourceHolder(ResourceEnum::CottonFabric, ResourceHolderType::Requester, 0);
	AddResourceHolder(ResourceEnum::DyedCottonFabric, ResourceHolderType::Provider, 0);

	AddUpgrades({
		MakeProductionUpgrade_WithHouseLvl(LOCTEXT("Advanced Machinery", "Advanced Machinery"), ResourceEnum::Iron, 7),
		MakeComboUpgrade(LOCTEXT("Cotton Mill Town", "Cotton Mill Town"), ResourceEnum::Iron),
	});
}

/*
 * PrintingPress
 */
void PrintingPress::FinishConstruction() {
	Building::FinishConstruction();

	AddUpgrades({
		MakeProductionUpgrade_WithHouseLvl(LOCTEXT("Advanced Machinery", "Advanced Machinery"), ResourceEnum::Iron, 7),
		MakeComboUpgrade(LOCTEXT("Printing Press Town", "Printing Press Town"), ResourceEnum::Iron),
	});
}

/*
 * ClayPit
 */
void ClayPit::FinishConstruction() {
	Building::FinishConstruction();

	AddUpgrades({
		MakeWorkerSlotUpgrade(50),
	});
}

/*
 * Potter
 */
void Potter::FinishConstruction() {
	Building::FinishConstruction();

	AddUpgrades({
		MakeProductionUpgrade_Money(LOCTEXT("Improved Kiln", "Improved Kiln"), ProductionUpgrade30),
		MakeWorkerSlotUpgrade(30),
		MakeComboUpgrade(LOCTEXT("Potter Town", "Potter Town"), ResourceEnum::Stone),
	});

	_simulation->TryAddQuest(_playerId, std::make_shared<PotteryQuest>());
}

std::vector<BonusPair> Potter::GetBonuses()
{
	std::vector<BonusPair> bonuses = IndustrialBuilding::GetBonuses();
	if (_simulation->TownhallCardCountTown(_townId, CardEnum::MasterPotter) > 0) {
		bonuses.push_back({ LOCTEXT("Master Potter", "Master Potter"), 20 });
	}

	if (_simulation->HasGlobalBonus(_playerId, CardEnum::Craftmanship)) {
		bonuses.push_back({ LOCTEXT("Craftmanship", "Craftmanship"), 20 });
	}

	return bonuses;
}

/*
 * FurnitureWorkshop
 */
void FurnitureWorkshop::FinishConstruction() {
	Building::FinishConstruction();

	AddUpgrades({
		MakeWorkerSlotUpgrade(30),
		MakeUpgrade(LOCTEXT("Minimalism", "Minimalism"), LOCTEXT("Consumes 30% less input.", "Consumes 30% less input."), 30),
		MakeComboUpgrade(LOCTEXT("Furniture Town", "Furniture Town"), ResourceEnum::Stone),
	});
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

	AddUpgrades({
		MakeProductionUpgrade_Money(LOCTEXT("Juicier Bait", "Juicier Bait"), ProductionUpgrade30),
		MakeProductionUpgrade(LOCTEXT("Improved Fishing Tools", "Improved Fishing Tools"), ResourceEnum::SteelTools, ProductionUpgrade50),
		MakeWorkerSlotUpgrade(30),
		//BuildingUpgrade("Whaling", "Catch whale from deep sea instead.\n  Produces whale meat.\n  +2 worker slots.\n  No effect nearby fish population", 120)
	});
	// TODO: urchin harvester (luxury?)
	// There are about 950 species of sea urchins that inhabit a wide range of depth zones in all climates across the world’s oceans. About 18 of them are edible.

	PUN_LOG("FinishContruction Fisher %d", buildingId());

	_simulation->TryAddQuest(_playerId, std::make_shared<CooperativeFishingQuest>());
}

std::vector<BonusPair> Fisher::GetBonuses() 
{
	std::vector<BonusPair> bonuses = Building::GetBonuses();
	if (_simulation->TownhallCardCountTown(_townId, CardEnum::CooperativeFishing)) {
		bonuses.push_back({ LOCTEXT("Cooperative Fishing", "Cooperative Fishing"), max(0, _simulation->GetAverageHappiness(_townId) - 60) });
	}
	if (_simulation->HasTownBonus(_townId, CardEnum::BorealWinterFishing)) 
	{
		BiomeEnum biomeEnum = centerBiomeEnum();
		if (biomeEnum == BiomeEnum::Tundra || biomeEnum == BiomeEnum::BorealForest) {
			bonuses.push_back({ LOCTEXT("Winter Fishing", "Winter Fishing"), 50 });
		}
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

	tileCount = std::max(tileCount, 1); // prevent out of map placement crash
	
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

	AddUpgrades({
		MakeWorkerSlotUpgrade(100, 4),
		MakeUpgrade(LOCTEXT("Improved shift", "Improved shift"), LOCTEXT("Mine with full worker slots get 20% productivity", "Mine with full worker slots get 20% productivity"), 40),
		MakeProductionUpgrade(LOCTEXT("Wide Shaft", "Wide Shaft"), ResourceEnum::Stone, ProductionUpgrade30)
	});

}

std::vector<BonusPair> Mine::GetBonuses()
{
	std::vector<BonusPair> bonuses = Building::GetBonuses();
	if (_simulation->GetTownLvl(_townId) >= 3) {
		bonuses.push_back({ TownhallUpgradeBonusText(3), 10 });
	}
	if (IsUpgraded_InitialIndex(1) && isOccupantFull()) {
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

	{
		int32 sustainabilityCount = slotCardCount(CardEnum::SustainabilityBook);
		for (int32 i = 0; i < sustainabilityCount; i++) {
			depletionMultiplier = depletionMultiplier * 70 / 100;
		}
	}
	{
		int32 sustainabilityCount = slotCardCount(CardEnum::SustainabilityBook2);
		for (int32 i = 0; i < sustainabilityCount; i++) {
			depletionMultiplier = depletionMultiplier * 60 / 100;
		}
	}

	
	if (_simulation->HasGlobalBonus(_playerId, CardEnum::Geologist)) {
		depletionMultiplier = depletionMultiplier * 50 / 100;
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

	if (_simulation->HasTownBonus(_townId, CardEnum::BorealGoldOil)) {
		//BiomeEnum biomeEnum = centerBiomeEnum();
		//if (biomeEnum == BiomeEnum::Tundra || biomeEnum == BiomeEnum::BorealForest) {
			bonuses.push_back({ LOCTEXT("Gold and Oil Bonus", "Gold and Oil"), 70 });
		//}
	}
	
	if (_simulation->HasTownBonus(_townId, CardEnum::DesertGem)) {
		//if (centerBiomeEnum() == BiomeEnum::Desert) {
			bonuses.push_back({ LOCTEXT("Desert Gem Bonus", "Desert Gem"), 50 });
		//}
	}
	
	return bonuses;
}

/*
 * GemstoneMine
 */
std::vector<BonusPair> GemstoneMine::GetBonuses()
{
	std::vector<BonusPair> bonuses = Mine::GetBonuses();
	if (_simulation->HasTownBonus(_townId, CardEnum::DesertGem)) {
		//if (centerBiomeEnum() == BiomeEnum::Desert) {
			bonuses.push_back({ LOCTEXT("Desert Gem Bonus", "Desert Gem"), 50 });
		//}
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
	AddUpgrades({
		MakeUpgrade(LOCTEXT("Better process", "Better process"), LOCTEXT("Uses 50% less wood to produce paper.", "Uses 50% less wood to produce paper."), ResourceEnum::Brick, 50),
		MakeWorkerSlotUpgrade(30, 2),
	});

	Building::FinishConstruction();
}

/*
 * Smelter
 */
const FText teamworkText = LOCTEXT("Teamwork", "Teamwork");

void Smelter::FinishConstruction() {
	Building::FinishConstruction();

	AddUpgrades({
		MakeUpgrade(teamworkText, LOCTEXT("Smelter Teamwork Desc", "Smelter with full worker slots get 50% production bonus"), mainUpgradeResource(), 50),
		MakeUpgrade(LOCTEXT("Efficient Furnace", "Efficient Furnace"), LOCTEXT("Decrease input by 30%", "Decrease input by 30%"), mainUpgradeResource(), 50),
		MakeComboUpgrade(FText::Format(LOCTEXT("UpgradeGuild", "{0} Guild"), buildingInfo().GetName()), mainUpgradeResource()),
	});
}

std::vector<BonusPair> Smelter::GetBonuses()
{
	std::vector<BonusPair> bonuses = IndustrialBuilding::GetBonuses();
	if (_simulation->buildingCount(_townId, CardEnum::EnvironmentalistGuild)) {
		bonuses.push_back({ LOCTEXT("Environmentalist", "Environmentalist"), -30 });
	}
	if (IsUpgraded_InitialIndex(0) && isOccupantFull()) {
		bonuses.push_back({ teamworkText, 50 });
	}

	if (_simulation->TownhallCardCountTown(_townId, CardEnum::CoalPipeline) > 0) {
		if (_simulation->resourceCountTown(_townId, ResourceEnum::Coal) >= 1000) {
			bonuses.push_back({ LOCTEXT("Coal Pipeline", "Coal Pipeline"), 50 });
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

	AddUpgrades({
		MakeProductionUpgrade(LOCTEXT("Improved Production", "Improved Production"), ResourceEnum::Brick, ProductionUpgrade30),
		MakeComboUpgrade(LOCTEXT("Mint Town", "Mint Town"), ResourceEnum::Brick),
	});
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

	AddUpgrades({
		MakeProductionUpgrade(LOCTEXT("Better Tools", "Better Tools"), ResourceEnum::SteelTools, ProductionUpgrade50),
		MakeProductionUpgrade(LOCTEXT("Component Blueprints", "Component Blueprints"), ResourceEnum::Paper, ProductionUpgrade50),
		MakeComboUpgrade(LOCTEXT("Inventor Guild", "Inventor Guild"), ResourceEnum::Brick),
	});
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

	auto pathAI = _simulation->pathAI();

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
	_simulation->RemoveBuilding(_objectId); // Deinit is in here...

	// Leaving behind just plain road...
	overlaySystem().AddRoad(_centerTile, isDirt(), true);
}
void RoadConstruction::OnDeinit()
{
	// Remove road
	if (overlaySystem().IsRoad(_centerTile)) {
		overlaySystem().RemoveRoad(_centerTile);
		//GameMap::RemoveFrontRoadTile(area.min());
		check(_simulation->IsFrontBuildable(_centerTile));
	}
}

void FenceGate::FinishConstruction() 
{
	Building::FinishConstruction();
	//_simulation->SetWalkableNonIntelligent(_centerTile, false);

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

//void Bank::CalculateRoundProfit()
//{
//	lastRoundProfit = GetRadiusBonus(CardEnum::House, Radius, [&](int32 bonus, Building& building) 
//	{
//		House& house = building.subclass<House>();
//
//		// Check how many banks is taking this house...
//		int32 bankCount = GetRadiusBonus(CardEnum::Bank, Radius, [&](int32 bonus1, Building& building1) {
//			return bonus1 + 1;
//		});
//
//		if (bankCount <= 0) {
//			bankCount = 1;
//		}
//		//PUN_CHECK(bankCount >= 1);
//
//		return bonus + (house.houseLvl() >= 5 ? ProfitPerHouse : 0) / bankCount; // Profit shared between banks...
//	});
//}

void Archives::CalculateRoundProfit()
{
	lastRoundProfit = 0;

	for (const CardStatus& cardStatus : _cardSlots) {
		lastRoundProfit += GetBuildingInfo(cardStatus.cardEnum).baseCardPrice;
	}

	lastRoundProfit = lastRoundProfit * CardProfitPercentPerYear / Time::RoundsPerYear / 100;
}

/*
 * 
 */
void SandMine::FinishConstruction() {
	Building::FinishConstruction();

	AddUpgrades({
		MakeProductionUpgrade(LOCTEXT("Improved Tooling", "Improved Tooling"), ResourceEnum::SteelTools, ProductionUpgrade30),
		MakeComboUpgrade(LOCTEXT("Sand Mine Town", "Sand Mine Town"), ResourceEnum::Brick),
	});
}

void Glassworks::FinishConstruction() {
	Building::FinishConstruction();

	AddUpgrades({
		MakeProductionUpgrade(LOCTEXT("Glass Annealer", "Glass Annealer"), ResourceEnum::Brick, ProductionUpgrade30),
		MakeComboUpgrade(LOCTEXT("Glassworks Town", "Glassworks Town"), ResourceEnum::Brick),
	});
}
void ConcreteFactory::FinishConstruction() {
	Building::FinishConstruction();

	AddUpgrades({
		MakeProductionUpgrade(LOCTEXT("High Temperature Kiln", "High Temperature Kiln"), ResourceEnum::Steel, ProductionUpgrade30),
		MakeComboUpgrade(LOCTEXT("Concrete Factory Town", "Concrete Factory Town"), ResourceEnum::Concrete),
	});
}



void Steelworks::FinishConstruction() {
	Building::FinishConstruction();

	AddUpgrades({
		MakeProductionUpgrade_WithHouseLvl(LOCTEXT("Advanced Machinery", "Advanced Machinery"), ResourceEnum::Steel, 8),
		MakeComboUpgrade(LOCTEXT("Steelworks Town", "Steelworks Town"), ResourceEnum::Concrete),
	});
}
void OilRig::FinishConstruction() {
	Building::FinishConstruction();

	AddUpgrades({
		MakeProductionUpgrade_WithHouseLvl(LOCTEXT("Advanced Machinery", "Advanced Machinery"), ResourceEnum::Steel, 8),
		MakeComboUpgrade(LOCTEXT("Oil Rig Town", "Oil Rig Town"), ResourceEnum::Steel),
	});
}

void PaperMill::OnInit()
{
	SetupWorkMode({
		{ LOCTEXT("Printing Paper", "Printing Paper"), ResourceEnum::Wood, ResourceEnum::None, ResourceEnum::Paper },
		{ LOCTEXT("Toilet Paper", "Toilet Paper"), ResourceEnum::Wood, ResourceEnum::None, ResourceEnum::ToiletPaper },
	});
}

void PaperMill::FinishConstruction() {
	Building::FinishConstruction();

	AddResourceHolder(ResourceEnum::Wood, ResourceHolderType::Requester, 0);
	AddResourceHolder(ResourceEnum::Paper, ResourceHolderType::Provider, 0);
	AddResourceHolder(ResourceEnum::ToiletPaper, ResourceHolderType::Provider, 0);

	AddUpgrades({
		MakeUpgrade(LOCTEXT("Drying Cylinder PaperMill", "Drying Cylinder"), LOCTEXT("Uses 50% less wood to produce paper.", "Uses 50% less wood to produce paper."), ResourceEnum::Steel, 50),
		MakeProductionUpgrade_WithHouseLvl(LOCTEXT("Advanced Machinery", "Advanced Machinery"), ResourceEnum::Steel, 8),
		MakeComboUpgrade(LOCTEXT("Paper Mill Town", "Paper Mill Town"), ResourceEnum::Steel),
	});
}



void ClockMakers::FinishConstruction() {
	Building::FinishConstruction();

	AddUpgrades({
		MakeProductionUpgrade_WithHouseLvl(LOCTEXT("Advanced Machinery", "Advanced Machinery"), ResourceEnum::Steel, 8),
		MakeComboUpgrade(LOCTEXT("Clock Makers Town", "Clock Makers Town"), ResourceEnum::Steel),
	});
}



/*
 * Power Plants
 */
void PowerPlant::FinishConstruction() {
	Building::FinishConstruction();

	//AddUpgrades({
	//	// "Regenerative Feed Heating"
	//	MakeWorkerSlotUpgrade(30),
	//});

	AddResourceHolder(fuelEnum(), ResourceHolderType::Requester, 100);
}




/*
 * OilRig
 */
std::vector<BonusPair> OilRig::GetBonuses()
{
	std::vector<BonusPair> bonuses = Mine::GetBonuses();

	if (_simulation->HasTownBonus(_townId, CardEnum::BorealGoldOil)) {
		//BiomeEnum biomeEnum = centerBiomeEnum();
		//if (biomeEnum == BiomeEnum::Tundra || biomeEnum == BiomeEnum::BorealForest) {
			bonuses.push_back({ LOCTEXT("Gold and Oil Bonus", "Gold and Oil"), 70 });
		//}
	}
	
	return bonuses;
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

void Fort::FinishConstruction()
{
	Building::FinishConstruction();

	AddUpgrades({
		MakeLevelUpgrade(
			LOCTEXT("FortUpgrade_WallLevel", "Wall Level"),
			LOCTEXT("FortUpgrade_WallLevel Desc", "Higher level Wall has higher HP."),
			ResourceEnum::Stone, 100, 100, 1000 / GetResourceInfo(ResourceEnum::Stone).basePrice
		)
	});


	_simulation->AddFortToProvince(provinceId(), buildingId());
}

void ResourceOutpost::TickRound()
{
	if (isConstructed())
	{
		ResourceEnum resourceEnum = GetColonyResourceEnum();
		if (IsResourceValid(resourceEnum))
		{
			int32 lordPlayerId = _simulation->townManagerBase(_townId)->lordPlayerId();
			if (lordPlayerId != -1)
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
					// Send resource to the capital
					_simulation->resourceSystem(lordPlayerId).AddResourceGlobal(resourceEnum, resourceCount, *_simulation);
					_simulation->statSystem(lordPlayerId).AddResourceStat(ResourceSeasonStatEnum::Production, resourceEnum, resourceCount);

					_simulation->uiInterface()->ShowFloatupInfo(FloatupEnum::GainResource, centerTile(), TEXT_NUMSIGNED(resourceCount), resourceEnum);
				}
			}
		}
	}
}

/*
 * IrrigationReservoir
 */
void IrrigationReservoir::FinishConstruction() {
	Building::FinishConstruction();

	AddUpgrades({
		MakeUpgrade(LOCTEXT("Wind-powered Pump", "Wind-powered Pump"), LOCTEXT("Wind-powered Pump Desc", "Halve the upkeep if adjacent to Windmill."), 20),
	});

	ExecuteInRadius(CardEnum::Farm, Radius + 20, [&](Building& building) {
		building.subclass<Farm>().RefreshFertility();
	}); // extra 20 just in case it is farm's rim
}

void IrrigationPump::FinishConstruction() {
	Building::FinishConstruction();

	UnlockSystem* unlockSys = _simulation->unlockSystem(_playerId);
	if (!unlockSys->isUnlocked(CardEnum::IrrigationDitch)) {
		_simulation->AddPopupToFront(_playerId,
			LOCTEXT("Unlocked Irrigation Ditch!", "Unlocked Irrigation Ditch!<space>Irrigation Ditch is now available in the Build Menu."),
			ExclusiveUIEnum::None, "PopupNeutral"
		);
		unlockSys->UnlockBuilding(CardEnum::IrrigationDitch);
	}

	_simulation->overlaySystem().AddIrrigationDitch(centerTile() + WorldTile2::RotateTileVector(WorldTile2(-3, 1), _faceDirection));

	// Invisible Irrigation Connection
	_area.ExecuteOnBorder_WorldTile2([&](WorldTile2 tile) {
		_simulation->overlaySystem().AddIrrigationDitch(tile, false);
	});
}

void IrrigationPump::OnDeinit()
{
	_simulation->overlaySystem().RemoveIrrigationDitch(centerTile() + WorldTile2::RotateTileVector(WorldTile2(-3, 1), _faceDirection));
	
	// Invisible Irrigation Connection
	_area.ExecuteOnBorder_WorldTile2([&](WorldTile2 tile) {
		_simulation->overlaySystem().RemoveIrrigationDitch(tile);
	});
}

/*
 * Provincial Building
 */
void MinorCity::FinishConstruction()
{
	Building::FinishConstruction();

	int32 provinceId = _simulation->GetProvinceIdClean(centerTile());
	check(_simulation->IsProvinceValid(provinceId));

	for (int32 i = 0; i < 5; i++) {
		WorldTile2 spawnTile = _simulation->GetProvinceRandomTile(provinceId, gateTile(), 1, false, 10);
		if (spawnTile.isValid()) {
			int32 ageTicks = GameRand::Rand() % GetUnitInfo(UnitEnum::WildMan).maxAgeTicks;
			int32 newBornId = _simulation->AddUnit(UnitEnum::WildMan, GameInfo::PlayerIdNone, spawnTile.worldAtom2(), ageTicks);
			_simulation->unitAI(newBornId).SetHouseId(buildingId());
			AddOccupant(newBornId);
			
			PUN_LOG("Wildman Born Village id:%d bldId:%d bldTile:%s", newBornId, buildingId(), *centerTile().To_FString());
		}
	}

	AddRoadAroundBuilding();
}

void MinorCity::OnDeinit()
{
	for (int32 occupantId : _occupantIds) {
		_simulation->unitAI(occupantId).Die();
	}
	
}

int32 MinorCity::displayVariationIndex()
{
	return std::max(0, _simulation->townManagerBase(_townId)->GetMinorCityLevel() - 1);
}

/*
 * ProvinceRuin
 */
void ProvinceRuin::FinishConstruction()
{
	AddUpgrades({
		MakeLevelUpgrade(
			LOCTEXT("ProvinceRuinUpgrade_Dig", "Artifact Excavation"),
			LOCTEXT("ProvinceRuinUpgrade_Dig Desc", "Excavate the site for Artifacts"),
			ResourceEnum::Money, 0, 70, BaseArtifactExcavationCost // roughly half mil to get to lvl 10...
		),
	});

	Building::FinishConstruction();
}

void ProvinceRuin::OnUpgradeBuildingWithPlayerId(int32 upgradeIndex, int32 upgraderPlayerId)
{
	if (upgradeIndex == 0)
	{
		std::vector<CardEnum> availableCards;

		if (isEnum(CardEnum::MayanPyramid))
		{
			availableCards = {
				CardEnum::DecorativePlate,
				CardEnum::SacrificialAltar,

				CardEnum::BallCourtGoals, //!
				
				CardEnum::StoneStele,

				CardEnum::DecorativePlate,
				CardEnum::SacrificialAltar,
				CardEnum::StoneStele,

				CardEnum::Codex, //!!
			};
		}
		else if (isEnum(CardEnum::EgyptianPyramid))
		{
			availableCards = {
				CardEnum::CanopicJars,
				CardEnum::DepartureScrolls,

				CardEnum::SolarBarque, //!

				CardEnum::DeathMask,

				CardEnum::CanopicJars,
				CardEnum::DepartureScrolls,
				CardEnum::DeathMask,
				
				CardEnum::GoldCapstone, //!!
			};
		}
		else if (isEnum(CardEnum::StoneHenge))
		{
			availableCards = {
				CardEnum::FeastRemains,
				CardEnum::ForeignTrinkets,

				CardEnum::OfferingCup, //!
				
				CardEnum::ChalkPlaque,
				
				CardEnum::FeastRemains,
				CardEnum::ForeignTrinkets,
				CardEnum::ChalkPlaque,

				CardEnum::OrnateTrinkets, //!!
			};
		}
		else if (isEnum(CardEnum::EasterIsland))
		{
			availableCards = {
				CardEnum::TatooingNeedles,
				CardEnum::Petroglyphs,

				CardEnum::CoralEyes, //!
				
				CardEnum::StoneFishhooks,

				CardEnum::TatooingNeedles,
				CardEnum::Petroglyphs,
				CardEnum::StoneFishhooks,

				CardEnum::AncientStaff, //!!
			};
		}

		
		CardEnum artifactEnum = availableCards[GetUpgrade(upgradeIndex).upgradeLevel % availableCards.size()];
		
		_simulation->TryAddCards_BoughtHandAndInventory(upgraderPlayerId, CardStatus(artifactEnum, 1));

		// Warn
		if (_simulation->IsTileOwnedByForeignPlayer(upgraderPlayerId, centerTile()))
		{
			_simulation->AddPopup(_simulation->tileOwnerPlayer(centerTile()), FText::Format(
				LOCTEXT("StealArtifactWarning_Pop", "{0} stole an artifact from your territory ({1} from {2})."),
				_simulation->playerNameT(upgraderPlayerId),
				GetBuildingInfo(artifactEnum).GetName(),
				buildingInfo().GetName()
			));
		}
	}
}

int32 ProvinceRuin::GetDigDistance(int32 upgraderPlayerId) const
{
	// The farther from nearest townhall, the more cost
	int32 distanceToNearestTown = INT32_MAX;

	const std::vector<int32>& townIds = _simulation->GetTownIds(upgraderPlayerId);
	for (int32 townId : townIds) {
		if (_simulation->townManagerBase(townId)->townhallId != -1) {
			int32 distance = WorldTile2::Distance(_simulation->GetMajorTownhallGateFast(townId), centerTile());
			distanceToNearestTown = std::min(distanceToNearestTown, distance);
		}
	}

	return distanceToNearestTown;
}

int32 ProvinceRuin::GetDigDistanceFactor(int32 upgraderPlayerId) const
{

	const int32 distanceToDoublePrice = 300;
	const int32 maxFactor = 500;

	int32 digDistance = GetDigDistance(upgraderPlayerId);
	if (digDistance == INT32_MAX) {
		return maxFactor;
	}
	
	return std::min(maxFactor, 100 + 100 * std::max(0, digDistance - distanceToDoublePrice) / distanceToDoublePrice);
}

/*
 * Zoo
 */


void Zoo::FinishConstruction()
{
	//AddUpgrades({
	//	MakeUpgrade(LOCTEXT("ZooUpgrade_AnimalUniqueness", "Animal Uniqueness"), LOCTEXT("ZooUpgrade_AnimalUniqueness Desc", "Each unique kind of animal increases service quality by additional 10%"), ResourceEnum::Brick, 50),
	//});

	Building::FinishConstruction();
}

std::vector<BonusPair> Zoo::GetBonuses()
{
	std::vector<BonusPair> bonuses = Building::GetBonuses();

	// Animal Uniqueness
	if (IsUpgraded(0)) 
	{
		const std::vector<CardStatus>& cards = slotCards();
		TSet<CardEnum> cardEnums;
		for (const CardStatus& card: cards) {
			if (IsAnimalCard(card.cardEnum)) {
				cardEnums.Add(card.cardEnum);
			}
		}
		
		bonuses.push_back({ GetUpgrade(0).name, 10 * cardEnums.Num() });
	}

	return bonuses;
}

/*
 * Museum
 */
void Museum::FinishConstruction()
{
	AddUpgrades({

	});

	Building::FinishConstruction();
}

/*
 * Diplomatic Building
 */
void DiplomaticBuilding::FinishConstruction()
{
	Building::FinishConstruction();
	_simulation->playerOwned(foreignBuilderId()).AddDiplomaticBuilding(buildingId());
	_simulation->playerOwned(_playerId).AddDiplomaticBuilding(buildingId());
}

void DiplomaticBuilding::OnDeinit() {
	_simulation->playerOwned(foreignBuilderId()).RemoveDiplomaticBuilding(buildingId());
	_simulation->playerOwned(_playerId).RemoveDiplomaticBuilding(buildingId());
}

/*
 * Embassy
 */
void Embassy::FinishConstruction()
{
	/*
		- Upgrade.. Encourage building Foreign Quarter
		- Upgrade: Trade Deal ... 
	 */
	AddUpgrades({
		MakeUpgrade(
			LOCTEXT("EmbassyUpgrade_ForeignInvestment", "Foreign Investment"), 
			LOCTEXT("EmbassyUpgrade_ForeignInvestment Desc", "Each foreign building built gives influence equals to 50% of its cost"), 
			50
		),
		MakeUpgrade(
			LOCTEXT("EmbassyUpgrade_Hotels", "Hotel Chain"),
			LOCTEXT("EmbassyUpgrade_Hotels Desc", "Gain <img id=\"Coin\"/> income equals to 20% of the revenue from Hotels in this town that is built by you."),
			50
		),
		MakeUpgrade(
			LOCTEXT("EmbassyUpgrade_ForeignQuarter", "Foreign Quarter Synergy"),
			LOCTEXT("EmbassyUpgrade_ForeignQuarter Desc", "+30<img id=\"Influence\"/> income if we have Foreign Quarter in this city."),
			50
		),
	});

	DiplomaticBuilding::FinishConstruction();
}

/*
 * Foreign Quarter
 */
void ForeignQuarter::FinishConstruction()
{
	/*
		- Sci.. Hotel give sci etc.
		Upgrade: Pub ... money
		Trading Company, Foreign Manufacturing (Factory), School, Library etc.
	 */
	AddUpgrades({
		MakeLevelUpgrade(
			LOCTEXT("ForeignQuarterUpgrade_MilitaryQuarter", "Military Quarter"),
			LOCTEXT("ForeignQuarterUpgrade_MilitaryQuarter Desc", "Each Upgrade Level Increases <img id=\"Influence\"/> gain by 100"),
			ResourceEnum::Money, 30
		),
		MakeLevelUpgrade(
			LOCTEXT("ForeignQuarterUpgrade_MerchantQuarter", "Merchant Quarter"),
			LOCTEXT("ForeignQuarterUpgrade_MerchantQuarter Desc", "Each Upgrade Level Increases <img id=\"Coin\"/> gain by 100"),
			ResourceEnum::Money, 30
		),
	});

	DiplomaticBuilding::FinishConstruction();

	UnlockSystem* unlockSys = _simulation->unlockSystem(foreignBuilderId());
	if (!unlockSys->unlockState(UnlockStateEnum::ForeignPort)) {
		unlockSys->SetUnlockState(UnlockStateEnum::ForeignPort, true);

		CardEnum unlockBuildingEnum = CardEnum::ForeignPort;
		_simulation->AddDrawCards(foreignBuilderId(), unlockBuildingEnum);

		_simulation->AddPopup(
			PopupInfo(foreignBuilderId(),
				FText::Format(
					LOCTEXT("UnlockedBuilding_Pop", "Unlocked Foreign Port.<space>Foreign Port can increase the effectiveness of Foreign Quarters.<space>Would you like to buy a Foreign Port card for {0}<img id=\"Coin\"/>."),
					TEXT_NUM(_simulation->GetCardPrice(foreignBuilderId(), unlockBuildingEnum))
				),
				{ LOCTEXT("Buy", "Buy"), LOCTEXT("Refuse", "Refuse") },
				PopupReceiverEnum::DoneResearchBuyCardEvent, false, "ResearchComplete", static_cast<int32>(unlockBuildingEnum)
			)
		);
	}
}

/*
 * Spy Center
 */

void SpyCenter::OnInit()
{
	ResetWorkModes();
}

void SpyCenter::FinishConstruction()
{
	AddUpgrades({
		MakeLevelUpgrade(
			LOCTEXT("SpyCenterUpgrade_SpyNestInfluence", "Spy Nest Influence Gain"),
			LOCTEXT("SpyCenterUpgrade_SpyNestInfluence Desc", "TODO: Desc"),
			ResourceEnum::Money, 0, 50, 1000
		),
		MakeLevelUpgrade(
			LOCTEXT("SpyCenterUpgrade_SpyEffectiveness", "Spy Effectiveness"),
			LOCTEXT("SpyCenterUpgrade_SpyEffectiveness Desc", "TODO: Desc"),
			ResourceEnum::Money, 0, 50, 1000
		),
		MakeLevelUpgrade(
			LOCTEXT("SpyCenterUpgrade_CounterIntelligence", "Counter Intelligence"),
			LOCTEXT("SpyCenterUpgrade_CounterIntelligence Desc", "TODO: Espionage on this city is less effective"),
			ResourceEnum::Money, 0, 50, 1000
		),
	});

	_simulation->playerOwned(_playerId).AddUniqueBuildingId(CardEnum::SpyCenter, buildingId());
	
	Building::FinishConstruction();
}

static const FText spyNestSupportText = LOCTEXT("Spy Nest Support", "Spy Nest Support");
static const FText counterintelligenceText = LOCTEXT("Counterintelligence", "Counterintelligence");
static const FText planStealText = LOCTEXT("Plan Steal", "Plan Steal");
static const FText planKidnapText = LOCTEXT("Plan Kidnap", "Plan Kidnap");
static const FText planTerrorismText = LOCTEXT("Plan Terrorism", "Plan Terrorism");

void SpyCenter::ResetWorkModes()
{
	/*
	 * choose workMode: Spy Nest Support/Steal/Kidnap/Terrorism cards every X sec
	 * 
	 */
	SetupWorkMode({
		WorkMode::Create(counterintelligenceText, LOCTEXT("Conserve resource desc", "Increase Counterintelligence effectiveness by 100%")),
		WorkMode::Create(planStealText, LOCTEXT("Plan Steal desc", "")),
		WorkMode::Create(planKidnapText, LOCTEXT("Plan Kidnap desc", "")),
		WorkMode::Create(planTerrorismText, LOCTEXT("Plan Terrorism desc", "")),
	});

	if (_simulation->IsResearched(_playerId, TechEnum::SpyNest)) {
		AddWorkMode(WorkMode::Create(spyNestSupportText, LOCTEXT("Spy Nest Support Desc", "Increase Spy Nest effectiveness by 100%")));
	}
}

int32 SpyCenter::spyNestBonus() {
	int32 bonus = GetUpgrade(0).upgradeLevel * 10;
	if (_workMode.name.IdenticalTo(spyNestSupportText)) {
		bonus += 100;
	}
	
	return bonus;
}

int32 SpyCenter::counterspyEffectiveness() {
	int32 result = 100 + GetUpgrade(2).upgradeLevel * 10;
	if (_workMode.name.IdenticalTo(counterintelligenceText)) {
		result += 100;
	}
	return result;
}

void SpyCenter::OnTick1Sec()
{
	int32 cardCreationMode = -1;
	CardEnum cardEnum = CardEnum::None;
	if (_workMode.name.IdenticalTo(planStealText)) {
		cardCreationMode = 0;
		cardEnum = CardEnum::Steal;
	}
	else if (_workMode.name.IdenticalTo(planKidnapText)) {
		cardCreationMode = 1;
		cardEnum = CardEnum::Kidnap;
	}
	else if (_workMode.name.IdenticalTo(planTerrorismText)) {
		cardCreationMode = 2;
		cardEnum = CardEnum::Terrorism;
	}

	auto resetCardProduction = [&]()
	{
		// Use the 300 upkeep to cheaply produce the target card
		int32 targetCardPrice = GetBuildingInfo(cardEnum).baseCardPrice;
		int32 secsToProduce = targetCardPrice * Time::SecondsPerRound / upkeep();

		secsToProduce /= 4; // Discount

		_secsToCardProduction = secsToProduce;
	};
	
	if (_cardCreationMode != cardCreationMode)
	{
		_cardCreationMode = cardCreationMode;

		if (_cardCreationMode >= 0) {
			resetCardProduction();
		}
		else {
			_secsToCardProduction = -1;
		}
	}
	else if (_cardCreationMode != -1) 
	{
		_secsToCardProduction--;
		if (_secsToCardProduction == 0) 
		{
			_simulation->TryAddCards_BoughtHandAndInventory(_playerId, CardStatus(cardEnum, 1));
			
			resetCardProduction();
		}
	}
	
}

/*
 * Policy Office
 */
void PolicyOffice::FinishConstruction()
{
	AddUpgrades({
		MakeLevelUpgrade(
			LOCTEXT("PolicyOfficeUpgrade_NationalPride", "National Pride"),
			LOCTEXT("PolicyOfficeUpgrade_NationalPride Desc", "+5% City Attractiveness if you have more than X Influence"),
			ResourceEnum::Influence, 0, 50, 1000
		),
		MakeLevelUpgrade(
			LOCTEXT("PolicyOfficeUpgrade_EconomicHegemony", "Economic Hegemony"),
			LOCTEXT("PolicyOfficeUpgrade_EconomicHegemony Desc", "+3% production bonus for the Town"),
			ResourceEnum::Influence, 0, 50, 1000
		),
	});

	_simulation->playerOwned(_playerId).AddUniqueBuildingId(CardEnum::PolicyOffice, buildingId());

	Building::FinishConstruction();
}

/*
 * World Trade Office
 */
void WorldTradeOffice::FinishConstruction()
{
	Building::FinishConstruction();
}
void WorldTradeOffice::OnTick1Sec()
{
	if (Time::Ticks() % (Time::TicksPerSecond * 30) == 0)
	{
		const int32 influenceUsagePerManipulation = 30;

		if (_simulation->influence(_playerId) >= influenceUsagePerManipulation)
		{
			if (resourceEnumToIncreasePrice != ResourceEnum::None) {
				_simulation->worldTradeSystem().ChangePrice100_Percent(resourceEnumToIncreasePrice, 1);
			}
			if (resourceEnumToDecreasePrice != ResourceEnum::None) {
				_simulation->worldTradeSystem().ChangePrice100_Percent(resourceEnumToDecreasePrice, -1);
			}

			_simulation->ChangeInfluence(_playerId, -influenceUsagePerManipulation);
			_simulation->uiInterface()->ShowFloatupInfo(_playerId, FloatupEnum::GainInfluence, centerTile(), TEXT_NUMSIGNED(-influenceUsagePerManipulation));
		}
	}
}

/*
 * Caravansary
 */
std::vector<BonusPair> Caravansary::GetBonuses()
{
	std::vector<BonusPair> bonuses = Building::GetBonuses();

	bonuses.push_back({ LOCTEXT("Trade Route Experience", "Trade Route Experience"), static_cast<int32>(sqrt(_cumulativeRouteMoney)) / 10 });


	return bonuses;
}


#undef LOCTEXT_NAMESPACE 