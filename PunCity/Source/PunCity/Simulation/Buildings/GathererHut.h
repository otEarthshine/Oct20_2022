// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "../Building.h"
//#include "Garrisons.h"
#include "PunCity/NetworkStructs.h"
#include "../QuestSystem.h"
#include "../PunTerrainChanges.h"

/*
 * Gather buildings
 */

class GathererHut final : public Building
{
public:
	void OnInit() override
	{	
		SetupWorkMode({
			WorkMode::Create("Normal", ""),
			WorkMode::Create("Meticulous", "Gathering action takes twice as long, but yield 30% more fruit."),
		});
	}
	
	void FinishConstruction() override
	{
		Building::FinishConstruction();

		AddResourceHolder(ResourceEnum::Orange, ResourceHolderType::Provider, 0);
		AddResourceHolder(ResourceEnum::Papaya, ResourceHolderType::Provider, 0);

		_upgrades = {
			MakeUpgrade("Delicate gathering", "+20% efficiency.", ResourceEnum::SteelTools, 50),
			MakeUpgrade("Pests traps", "+30% productivity if there is an adjacent hunter (does not stack).", ResourceEnum::Wood, 30),
		};
	}

	ResourceEnum product() final { return ResourceEnum::Orange; }

	std::vector<BonusPair> GetBonuses() override
	{
		std::vector<BonusPair> bonuses = Building::GetBonuses();

		if (IsUpgraded(0)) {
			bonuses.push_back({ "Delicate gathering upgrade", 20 });
		}

		if (IsUpgraded(1) && adjacentCount(CardEnum::HuntingLodge) > 0) {
			bonuses.push_back({ "Pests traps", 30 });
		}
		
		//std::vector<Building*> buildings = GetBuildingsInRegion(CardEnum::HuntingLodge);
		//for (Building* building : buildings) {
		//	if (building->IsUpgraded(1)) {
		//		bonuses.push_back({ "Hunting lodge upgrade", 30 });
		//		break;
		//	}
		//}
		
		return bonuses;
	}

	static const int Radius = 24;
};

class HuntingLodge final : public Building
{
public:
	ResourceEnum product() final { return ResourceEnum::Pork; }

	void OnInit() override
	{
		SetupWorkMode({
			WorkMode::Create("Normal", ""),
			WorkMode::Create("Poison Arrows", "Kill animals x4 faster but get -50% drop"),
		});
	}

	void FinishConstruction() final {
		Building::FinishConstruction();

		AddResourceHolder(ResourceEnum::Pork, ResourceHolderType::Provider, 0);

		_upgrades = {
			MakeUpgrade("Smoking chamber", "+30% efficiency.", ResourceEnum::Stone, 50),
			MakeUpgrade("Fruit bait", "+30% efficiency if there is an adjacent Fruit Gatherer (does not stack).",ResourceEnum::Wood, 30),
		};
	}

	std::vector<BonusPair> GetBonuses() override {
		std::vector<BonusPair> bonuses = Building::GetBonuses();

		if (IsUpgraded(0)) {
			bonuses.push_back({ "Smoking chamber", 30 });
		}

		if (IsUpgraded(1) && adjacentCount(CardEnum::FruitGatherer) > 0) {
			bonuses.push_back({ "Fruit bait", 30 });
		}
		
		//const std::vector<int32>& buildingIds = _simulation->buildingIds(_playerId, CardEnum::FruitGatherer);
		//for (int32 buildingId : buildingIds) {
		//	Building& building = _simulation->building(buildingId);
		//	PUN_CHECK(building.isEnum(CardEnum::FruitGatherer));
		//	if (building.centerTile().region() == centerTile().regionId() &&
		//		building.IsUpgraded(1))
		//	{
		//		bonuses.push_back({ "Gatherer upgrade", 30 });
		//	}
		//}
		return bonuses;
	}

	static const int Radius = 32;
};


class Forester final : public Building
{
public:
	void OnInit() override
	{
		SetupWorkMode({
			{"Cut and Plant", ResourceEnum::None, ResourceEnum::None, 0},
			{"Prioritize Planting", ResourceEnum::None, ResourceEnum::None, 0},
			{"Prioritize Cutting", ResourceEnum::None, ResourceEnum::None, 0},
		});
	}
	
	void FinishConstruction() override
	{
		Building::FinishConstruction();

		_upgrades = {
			MakeUpgrade("Timber Management", "+30% efficiency.", ResourceEnum::Stone,50),
			MakeUpgrade("Tree-felling Technique", "+50% efficiency.", ResourceEnum::Stone,80),
			MakeComboUpgrade("Forest Town", ResourceEnum::Wood, 50, 20),
		};
	}

	ResourceEnum product() override { return ResourceEnum::Wood; }

	std::vector<BonusPair> GetBonuses() override {
		std::vector<BonusPair> bonuses = Building::GetBonuses();

		if (IsUpgraded(0)) {
			bonuses.push_back({ "Timber Management", 30 });
		}
		if (IsUpgraded(1)) {
			bonuses.push_back({ "Tree-felling Technique", 50 });
		}

		return bonuses;
	}
	
	static const int32 Radius = 30;
};

/*
 * Production buildings
 */

class MushroomHut final : public Building
{
public:
	void FinishConstruction() final
	{
		Building::FinishConstruction();

		AddResourceHolder(ResourceEnum::Shroom, ResourceHolderType::Provider, 0);
		_upgrades = {
			//MakeUpgrade("Shroomery", "Produce psychedelic shroom (luxury) instead.", 200),
			MakeUpgrade("Intensive care", "+30% production bonus when worker slots are full", ResourceEnum::Stone, 50),
		};
	}

	std::vector<BonusPair> GetBonuses() override {
		std::vector<BonusPair> bonuses = Building::GetBonuses();
		if (IsUpgraded(0) && isOccupantFull()) {
			bonuses.push_back({ "Intensive care", 30 });
		}
		return bonuses;
	}
	

	int32 baseInputPerBatch() final;
};

class Beekeeper final : public Building
{
public:
	void FinishConstruction() final
	{
		Building::FinishConstruction();

		AddResourceHolder(ResourceEnum::Honey, ResourceHolderType::Provider, 0);

		_upgrades = {
			MakeUpgrade("Intensive Care", "+30% production bonus when worker slots are full", ResourceEnum::Brick, 50),
			MakeComboUpgrade("Knowledge Sharing", ResourceEnum::Paper, 70, 50),
		};
	}

	std::vector<BonusPair> GetBonuses() override {
		std::vector<BonusPair> bonuses = Building::GetBonuses();
		if (IsUpgraded(0) && isOccupantFull()) {
			bonuses.push_back({ "Intensive care", 30 });
		}
		return bonuses;
	}

	static int32 BeekeeperBaseEfficiency(int32 playerId, WorldTile2 centerTileIn, IGameSimulationCore* simulation);

	int32 efficiencyBeforeBonus() override {
		return BeekeeperBaseEfficiency(_playerId, centerTile(), _simulation);
	}
	
	static const int Radius = 18;
};

/*
 * Farm
 */

class Farm final : public Building
{
public:
	void OnInit() override;
	
	void FinishConstruction() override;

	ResourceEnum product() override { return GetTileObjInfo(currentPlantEnum).harvestResourceEnum(); }
	
	int32 efficiencyBeforeBonus() override {
		return _fertility;
	}

	bool HasAdjacencyBonus() override {
		return _simulation->IsResearched(_playerId, TechEnum::FarmAdjacency);
	}

	std::vector<BonusPair> GetBonuses() override;

	int32 totalFarmTiles() { return _area.sizeX() * _area.sizeY();  }

	WorldTile2 FindFarmableTile(int32 unitId);

	bool IsStageCompleted();
	int32 MinCropGrowthPercent();
	void ResetStageTo(FarmStage farmStage) {
		_farmStage = farmStage;
		std::fill(_isTileWorked.begin(), _isTileWorked.end(), false);
	}

	void SetAreaWalkable() final {
		_didSetWalkable = true;
	}
	
	void DoFarmWork(int32 unitId, WorldTile2 tile, FarmStage farmStage);

	void ClearAllPlants();

	bool IsStage(FarmStage farmStage) { return _farmStage == farmStage; }
	std::string farmStageName() {
		switch (_farmStage) {
		case FarmStage::Dormant: return "Dormant";
		case FarmStage::Seeding: return "Seeding";
		case FarmStage::Nourishing: return "Nourishing";
		case FarmStage::Harvesting: return "Harvesting";
		}
		UE_DEBUG_BREAK();
		return "";
	}

	bool ShouldAddWorker_ConstructedNonPriority() override {
		return !IsStage(FarmStage::Dormant);
	}

	void Tick1Sec() final
	{
		if (IsStage(FarmStage::Dormant))
		{
			// Farm can return to seeding spring to mid summer (round 1 summer)
			if (Time::IsValidFarmBeginTime()) {
				ResetStageTo(FarmStage::Seeding);
			}
		}
		else
		{
			// Force the farm into dormant stage in winter
			if (Time::IsSnowing())
			{
				ClearAllPlants();

				ResetWorkReservers(); // TODO: still worker crash???

				ResetStageTo(FarmStage::Dormant);
			}
		}
	}

	int32 workedTiles() {
		int32 tilesWorked = 0;
		for (size_t i = _isTileWorked.size(); i-- > 0;) {
			if (_isTileWorked[i]) tilesWorked++;
		}
		return tilesWorked;
	}

	void UnreserveFarmTile(int32 unitId, WorldTile2 tile);
	void ReserveFarmTile(int32 unitId, WorldTile2 tile);

	void Serialize(FArchive& Ar) override {
		Building::Serialize(Ar);
		Ar << currentPlantEnum;
		Ar << _farmStage;
		SerializeVecBool(Ar, _isTileWorked);
		SerializeVecLoop(Ar, _reservingUnitIdToFarmTileId, [&](std::pair<int32, int32>& pair) {
			Ar << pair.first;
			Ar << pair.second;
		});
		Ar << _fertility;
	}

	static int32 GetAverageFertility(TileArea area, IGameSimulationCore* simulation)
	{
		int32 sumFertility = 0;
		area.ExecuteOnArea_WorldTile2([&](WorldTile2 tile) {
			sumFertility += simulation->GetFertilityPercent(tile);
		});
		return sumFertility / area.tileCount();
	}
	int32 fertility() { return _fertility; }

private:
	bool NoFarmerOnTileId(int32 farmTileId);

	int32 farmTileIdFromWorldTile(WorldTile2 tile) {
		WorldTile2 farmTile = tile - _area.min();
		PUN_CHECK2(farmTile.x >= 0 && farmTile.y >= 0, debugStr());
		return farmTile.x + farmTile.y * _area.sizeX();
	}
public:
	TileObjEnum currentPlantEnum = TileObjEnum::None;
	
private:	
	FarmStage _farmStage = FarmStage::Dormant; // seed until done ... then nourish until autumn, or until fruit (for fruit bearers)
	std::vector<bool> _isTileWorked;
	int32 _fertility = 0;
	
	std::vector<std::pair<int32, int32>> _reservingUnitIdToFarmTileId;
};

/*
 * Mine
 */

class Mine : public Building
{
public:
	void OnInit() override
	{
		SetupWorkMode({
			WorkMode::Create("Normal", ""),
			WorkMode::Create("Conserve resource", "-30% productivity.\nDeposit depletes 30% slower for each mined resource unit."),
			WorkMode::Create("Rapid mining", "+30% productivity.\nDeposit depletes 30% faster for each mined resource unit."),
		});
	}
	
	void FinishConstruction() override
	{
		Building::FinishConstruction();

		_upgrades = {
			MakeUpgrade("More Workers", "+2 worker slots", 50),
			MakeUpgrade("Improved shift", "Mine with full worker slots get 20% production bonus", 40),
			MakeProductionUpgrade("Wide Shaft", ResourceEnum::Stone, 100, 50)
		};

	}

	void OnUpgradeBuilding(int upgradeIndex) override {
		if (upgradeIndex == 0) {
			_maxOccupants = 5;
			_allowedOccupants = _maxOccupants;
		}
	}
	
	std::vector<BonusPair> GetBonuses() override {
		std::vector<BonusPair> bonuses = Building::GetBonuses();
		if (_simulation->townLvl(_playerId) >= 5) {
			bonuses.push_back({ "Townhall upgrade", 10 });
		}
		if (IsUpgraded(1) && isOccupantFull()) {
			bonuses.push_back({ "Improved shift", 20 });
		}
		if (_simulation->buildingCount(_playerId, CardEnum::EnvironmentalistGuild)) {
			bonuses.push_back({ "Environmentalist", -30 });
		}

		if (_simulation->TownhallCardCount(_playerId, CardEnum::MiningEquipment) > 0) {
			if (_simulation->buildingCount(_playerId, CardEnum::Blacksmith) >= 1) {
				bonuses.push_back({ "Mining equipment", 30 });
			}
		}

		if (_workMode.name == "Conserve resource") {
			bonuses.push_back({ "Conserve resource", -30 });
		}
		else if (_workMode.name == "Rapid mining") {
			bonuses.push_back({ "Rapid mining", 30 });
		}
		
		return bonuses;
	}

	bool ShouldAddWorker_ConstructedNonPriority() override {
		return oreLeft() > 0;
	}

	void OnProduce(int32 productionAmount) override;

	void RefreshHoverWarning() override
	{
		hoverWarning = (oreLeft() <= 0) ? HoverWarning::Depleted : HoverWarning::None;
	}
};

class Quarry final : public Mine
{
public:
	std::vector<BonusPair> GetBonuses() final {
		std::vector<BonusPair> bonuses = Mine::GetBonuses();
		if (_simulation->IsResearched(_playerId, TechEnum::QuarryImprovement)) {
			bonuses.push_back({"Quarry improvement tech", 30});
		}
		return bonuses;
	}
};

class IronMine final : public Mine
{
public:

};

class CoalMine final : public Mine
{
public:

};

class GoldMine final : public Mine
{
public:
	std::vector<BonusPair> GetBonuses() final {
		std::vector<BonusPair> bonuses = Mine::GetBonuses();
		if (_simulation->TownhallCardCount(_playerId, CardEnum::GoldRush) > 0) {
			bonuses.push_back({ "Gold Rush", 30 });
		}
		return bonuses;
	}
};

class GemstoneMine final : public Mine
{
public:

};

/*
 * Industry
 */

class IndustrialBuilding : public Building
{
public:
	std::vector<BonusPair> GetBonuses() override {
		std::vector<BonusPair> bonuses = Building::GetBonuses();
		if (_simulation->townLvl(_playerId) >= 4) {
			bonuses.push_back({ "Townhall upgrade", 10 });
		}
		return bonuses;
	}
};

class PaperMaker final : public IndustrialBuilding
{
public:
	void FinishConstruction() final
	{
		_upgrades = {
			MakeUpgrade("Better process", "Uses 50% less wood to produce paper.", ResourceEnum::Brick, 50),
			MakeUpgrade("More workers", "+2 worker slots.", 20),
		};

		Building::FinishConstruction();
	}

	void OnUpgradeBuilding(int upgradeIndex) final {
		if (upgradeIndex == 1) {
			_maxOccupants = 5;
			_allowedOccupants = _maxOccupants;
		}
	}

	int32 baseInputPerBatch() final {
		return IsUpgraded(0) ? 5 : 10;
	}
};

class Smelter : public IndustrialBuilding
{
public:
	void FinishConstruction() final
	{
		Building::FinishConstruction();

		_upgrades = {
			MakeUpgrade("Teamwork", "Smelter with full worker slots get 50% production bonus", ResourceEnum::Stone, 100),
			MakeUpgrade("Efficient furnace", "Decrease input by 30%", ResourceEnum::Brick, 100),
			MakeComboUpgrade( buildingInfo().name + " Guild", ResourceEnum::Paper, 70, 30),
		};
	}

	std::vector<BonusPair> GetBonuses() override
	{
		std::vector<BonusPair> bonuses = IndustrialBuilding::GetBonuses();
		if (_simulation->buildingCount(_playerId, CardEnum::EnvironmentalistGuild)) {
			bonuses.push_back({ "Environmentalist", -30 });
		}
		if (IsUpgraded(0) && isOccupantFull()) {
			bonuses.push_back({ "Teamwork", 50 });
		}

		if (_simulation->TownhallCardCount(_playerId, CardEnum::CoalPipeline) > 0) {
			if (_simulation->resourceCount(_playerId, ResourceEnum::Coal) >= 1000) {
				bonuses.push_back({ "Coal pipeline", 30 });
			}
		}
		
		return bonuses;
	}

	int32 baseInputPerBatch() override {
		return Building::baseInputPerBatch() * (IsUpgraded(1) ? 70 : 100) / 100;
	}
};

class IronSmelter : public Smelter
{
public:
	std::vector<BonusPair> GetBonuses() override {
		std::vector<BonusPair> bonuses = Smelter::GetBonuses();

		if (_simulation->TownhallCardCount(_playerId, CardEnum::SmeltCombo) > 0) {
			//if (GetBuildingsInRegion(CardEnum::IronSmelter).size() >= 2) {
			//	bonuses.push_back({ "Iron smelt combo", 30 });
			//}
			if (adjacentCount(CardEnum::IronSmelter) > 0) {
				bonuses.push_back({ "Iron smelter combo", 30 });
			}
		}

		return bonuses;
	}
};
class IronSmelterGiant final : public IronSmelter
{
	
};

class GoldSmelter final : public Smelter
{
public:
};

class ConsumerIndustrialBuilding : public IndustrialBuilding
{
public:
	bool NeedWork() final {
		return MathUtils::SumVector(_workReserved) + _workDone100 < workManSecPerBatch100();
	}

	// Calculations for work/product are done at basePrice.
	// If input price rise, there would be less profit
	int32 workManSecPerBatch100() override
	{
		// Same amount of work required to acquire resources
		return baseProfitValue() * 100 * 100 / buildingInfo().workRevenuePerSec100_perMan; // first 100 for workManSecPerBatch100, second 100 to cancel out WorkRevenuePerManSec100
	}

	// Production always yield the same amount of product (2 * baseinput)
	int32 productPerBatch() override {
		return baseOutputValue() * efficiency() / 100;
	}
	

	int32 baseInputValue() {
		return GetResourceInfo(input1()).basePrice * baseInputPerBatch();
	}
	int32 baseOutputValue() {
		return baseInputValue() * 2;
	}
	int32 baseProfitValue() {
		// Without sustainability card, baseProfitValue == baseInputValue
		// With Sustainability, baseProfitValue would increase, increase the work time...
		return baseOutputValue() - GetResourceInfo(input1()).basePrice * inputPerBatch(); // inputPerBatch default to 10 taking into account sustainability card...
	}
};

class Mint final : public ConsumerIndustrialBuilding
{
public:
	void FinishConstruction() final
	{
		ConsumerIndustrialBuilding::FinishConstruction();

		_upgrades = {
			MakeUpgrade("Improved Production", "+30% production.", ResourceEnum::Brick, 50),
			MakeComboUpgrade("Mint Town", ResourceEnum::Brick, 50, 10),
		};
	}

	std::vector<BonusPair> GetBonuses() override {
		std::vector<BonusPair> bonuses = ConsumerIndustrialBuilding::GetBonuses();

		if (IsUpgraded(0)) {
			bonuses.push_back({ "Improved Production", 30 });
		}

		return bonuses;
	}

};

class InventorsWorkshop final : public ConsumerIndustrialBuilding
{
public:
	void FinishConstruction() final
	{
		ConsumerIndustrialBuilding::FinishConstruction();

		_upgrades = {
			MakeUpgrade("Better tools", "+50% production.", ResourceEnum::SteelTools, 100),
			MakeUpgrade("Component Blueprints", "+50% production.", ResourceEnum::Paper, 100),
			MakeComboUpgrade("Inventor Guild", ResourceEnum::Brick, 50, 25),
		};
	}

	std::vector<BonusPair> GetBonuses() override {
		std::vector<BonusPair> bonuses = IndustrialBuilding::GetBonuses();

		if (IsUpgraded(0)) {
			bonuses.push_back({ "Better tools", 50 });
		}
		if (IsUpgraded(1)) {
			bonuses.push_back({ "Component Blueprints", 50 });
		}

		return bonuses;
	}
};

class Barrack final : public ConsumerIndustrialBuilding
{
public:
	void FinishConstruction() final
	{
		ConsumerIndustrialBuilding::FinishConstruction();

		_upgrades = {
		};
	}

	std::vector<BonusPair> GetBonuses() override {
		std::vector<BonusPair> bonuses = IndustrialBuilding::GetBonuses();

		if (_simulation->IsResearched(_playerId, TechEnum::MilitaryLastEra)) {
			bonuses.push_back({ "Advanced Military", 100 });
		}
		
		return bonuses;
	}
};


class CardMaker final : public ConsumerIndustrialBuilding
{
public:
	void OnInit() override
	{
		SetupWorkMode({
			WorkMode::Create("Productivity Book", "Create Productivity Book Card"),
			WorkMode::Create("Sustainability Book", "Create Sustainability Book Card"),
			WorkMode::Create("Frugality Book", "Create Frugality Book Card"),
			WorkMode::Create("Wild Card", "Create Wild Card"),
			WorkMode::Create("Card Removal Card", "Create Card Removal Card"),
		});
	}
	
	void FinishConstruction() final
	{
		ConsumerIndustrialBuilding::FinishConstruction();

		_upgrades = {
			MakeUpgrade("Improved Production", "+30% production.", ResourceEnum::SteelTools, 50),
		};
	}

	std::vector<BonusPair> GetBonuses() override {
		std::vector<BonusPair> bonuses = ConsumerIndustrialBuilding::GetBonuses();

		if (IsUpgraded(0)) {
			bonuses.push_back({ "Improved Production", 30 });
		}
		return bonuses;
	}

	// Same amount of work required to acquire resources
	int32 workManSecPerBatch100() final
	{
		// Assume card 800 price for SlotCards
		CardEnum cardEnum = GetCardProduced();
		int32 cardPrice = GetBuildingInfo(cardEnum).baseCardPrice;

		if (IsBuildingSlotCard(cardEnum)) {
			cardPrice = 500;
		}
		// ensure Wild Card and Card Removal cards are not negative
		cardPrice = max(cardPrice, 120);
		
		int32 result = (cardPrice - batchCost()) * 100 * 100 / buildingInfo().workRevenuePerSec100_perMan; // first 100 for workManSecPerBatch100, second 100 to cancel out WorkRevenuePerManSec100

		return result * 100 / efficiency();
	}

	CardEnum GetCardProduced();
	
	int32 baseInputPerBatch() override
	{
		return 10;
		//CardEnum cardEnum = GetCardProduced();
		//if (IsBuildingSlotCard(cardEnum)) {
		//	return 10;
		//}
		//return 5;
	}
};

class ImmigrationOffice final : public ConsumerIndustrialBuilding
{
public:
	void FinishConstruction() final
	{
		ConsumerIndustrialBuilding::FinishConstruction();

		_upgrades = {
			MakeUpgrade("First Impression", "+30% efficiency.", ResourceEnum::Stone, 30),
		};
	}

	std::vector<BonusPair> GetBonuses() override {
		std::vector<BonusPair> bonuses = ConsumerIndustrialBuilding::GetBonuses();

		if (IsUpgraded(0)) {
			bonuses.push_back({ "First Impression Upgrade", 30 });
		}
		return bonuses;
	}

	// Same amount of work required to acquire resources
	int32 workManSecPerBatch100() final
	{
		// Assume card 500 price here...
		return (170 - batchCost()) * 100 * 100 / buildingInfo().workRevenuePerSec100_perMan; // first 100 for workManSecPerBatch100, second 100 to cancel out WorkRevenuePerManSec100
	}

	int32 baseInputPerBatch() override { return 0; }
};

class StoneToolShop : public IndustrialBuilding
{
public:
};
class Blacksmith : public IndustrialBuilding
{
public:
	void FinishConstruction() final
	{
		Building::FinishConstruction();

		_upgrades = {
			MakeProductionUpgrade("Improved Forge", ResourceEnum::Brick, 50, 30),
			MakeProductionUpgrade("Alloy Recipe", ResourceEnum::Paper, 50, 30),
			MakeComboUpgrade("Blacksmith Guild", ResourceEnum::Paper, 50, 25),
		};
	}

};
class Herbalist : public IndustrialBuilding
{
public:
};
class MedicineMaker : public IndustrialBuilding
{
public:
	void FinishConstruction() final
	{
		Building::FinishConstruction();

		_upgrades = {
			MakeUpgrade("Catalyst", "+30% production.", 100),
			MakeUpgrade("Improved Extraction", "+50% production.", 150),
			MakeComboUpgrade("Pharmaceutical Guild", ResourceEnum::Paper, 50, 25),
		};
	}
	
	int32 baseInputPerBatch() override {
		return 5;
	}
	
	std::vector<BonusPair> GetBonuses() override {
		std::vector<BonusPair> bonuses = Building::GetBonuses();

		if (IsUpgraded(0)) {
			bonuses.push_back({ "Catalyst", 30 });
		}
		if (IsUpgraded(1)) {
			bonuses.push_back({ "Improved Extraction", 50 });
		}

		return bonuses;
	}
};

class CharcoalMaker final : public IndustrialBuilding
{
public:
	void FinishConstruction() final
	{
		Building::FinishConstruction();

		_upgrades = {
			MakeUpgrade("Charcoal Conversion", "Use 30% less wood input.", 20),
			MakeProductionUpgrade("Improved Production", 50, 50),
			MakeComboUpgrade("Charcoal Burner Guild", ResourceEnum::Wood, 30, 15),
		};
	}

	int32 baseInputPerBatch() override { return IsUpgraded(0) ? 7 : 10; }

	std::vector<BonusPair> GetBonuses() override {
		std::vector<BonusPair> bonuses = Building::GetBonuses();
		if (_simulation->buildingCount(_playerId, CardEnum::EnvironmentalistGuild)) {
			bonuses.push_back({ "Environmentalist", -30 });
		}
		return bonuses;
	}
};

class Chocolatier : public IndustrialBuilding
{
public:
	void FinishConstruction() override
	{
		Building::FinishConstruction();


		_upgrades = {
			MakeUpgrade("Cocoa Processing", "Consumes 50% less input.", ResourceEnum::Iron, 50),
			MakeProductionUpgrade("Improved Production", ResourceEnum::Iron, 50, 50),
			MakeUpgrade("Reduce Upkeep", "Reduce upkeep by 50%", ResourceEnum::Brick, 20),
			MakeComboUpgrade("Chocolate Town", ResourceEnum::Iron, 50, 25),
		};
	}

	std::vector<BonusPair> GetBonuses() override {
		std::vector<BonusPair> bonuses = IndustrialBuilding::GetBonuses();


		return bonuses;
	}

	int32 upkeep() override {
		if (IsUpgraded(3)) {
			return Building::upkeep() / 2;
		}
		return Building::upkeep();
	}

	int32 baseInputPerBatch() override {
		return IsUpgraded(0) ? 5 : 10;
	}
};

class Winery final: public IndustrialBuilding
{
public:
	void FinishConstruction() final
	{
		Building::FinishConstruction();

		_upgrades = {
			MakeProductionUpgrade("Wine appreciation", 70, 50),
			MakeComboUpgrade("Wine Town", ResourceEnum::Brick, 25, 25),
		};
	}

	std::vector<BonusPair> GetBonuses() final {
		std::vector<BonusPair> bonuses = IndustrialBuilding::GetBonuses();
		if (_simulation->IsResearched(_playerId, TechEnum::WineryImprovement)) {
			bonuses.push_back({"Winery Improvement Tech", 30});
		}

		return bonuses;
	}
};

class Tailor final : public IndustrialBuilding
{
public:
	void OnInit() override
	{
		SetupWorkMode({
			{"Leather clothing", ResourceEnum::Leather, ResourceEnum::None, 10},
			{"Wool clothing", ResourceEnum::Wool, ResourceEnum::None, 10},
			{"Warm clothing", ResourceEnum::Leather, ResourceEnum::Wool, 5},
		});
	}
	
	void FinishConstruction() final
	{
		Building::FinishConstruction();

		AddResourceHolder(ResourceEnum::Leather, ResourceHolderType::Requester, 0);
		AddResourceHolder(ResourceEnum::Wool, ResourceHolderType::Requester, 0);
		AddResourceHolder(ResourceEnum::Cloth, ResourceHolderType::Provider, 0);

		ChangeWorkMode(_workMode);

		_upgrades = {
			MakeProductionUpgrade("Weaving Machine", ResourceEnum::Iron, 150, 100),
			MakeComboUpgrade("Tailor Town", ResourceEnum::Iron, 70, 25),
		};
	}

};

class BeerBrewery : public IndustrialBuilding
{
public:
	void OnInit() override
	{
		SetupWorkMode({
			{"Wheat Beer", ResourceEnum::Wheat, ResourceEnum::None, 10},
			{"Orange Cider", ResourceEnum::Orange, ResourceEnum::None, 10},
			{"Mushroom Beer", ResourceEnum::Mushroom, ResourceEnum::None, 10},
		});
	}
	
	void FinishConstruction() override
	{
		Building::FinishConstruction();

		AddResourceHolder(ResourceEnum::Wheat, ResourceHolderType::Requester, 0);
		AddResourceHolder(ResourceEnum::Orange, ResourceHolderType::Requester, 0);
		AddResourceHolder(ResourceEnum::Mushroom, ResourceHolderType::Requester, 0);
		AddResourceHolder(ResourceEnum::Beer, ResourceHolderType::Provider, 0);
		
		_upgrades = {
			MakeUpgrade("Improved Malting", "Consumes 30% less input.", ResourceEnum::Stone, 50),
			MakeProductionUpgrade("Fast Malting", ResourceEnum::Stone, 50, 30),
			MakeComboUpgrade("Brewery Town", ResourceEnum::Stone, 30, 20),
		};

		_simulation->TryAddQuest(_playerId, std::make_shared<BeerQuest>());

		ChangeWorkMode(_workMode); // Need this to setup resource target etc.
	}

	int32 baseInputPerBatch() override {
		return _workMode.inputPerBatch * (IsUpgraded(0) ? 70 : 100) / 100;
	}

	std::vector<BonusPair> GetBonuses() override {
		std::vector<BonusPair> bonuses = IndustrialBuilding::GetBonuses();
		if (_simulation->TownhallCardCount(_playerId, CardEnum::MasterBrewer) > 0) {
			bonuses.push_back({ "Master brewer", 30 });
		}

		return bonuses;
	}

	//ResourceEnum input1() final { return _workMode.input1; }
	//ResourceEnum input2() final { return _workMode.input2; }
};

class BeerBreweryFamous final : public BeerBrewery
{
public:
	std::vector<BonusPair> GetBonuses() final {
		std::vector<BonusPair> bonuses = BeerBrewery::GetBonuses();
		bonuses.push_back({ "Famous Brewery", 20 });
		return bonuses;
	}
};

class Windmill final : public IndustrialBuilding
{
public:
	void FinishConstruction() final
	{
		Building::FinishConstruction();

		_upgrades = {
			MakeUpgrade("Improved Grinder", "+10% productivity", ResourceEnum::Stone, 30),
		};
	}

	std::vector<BonusPair> GetBonuses() final {
		std::vector<BonusPair> bonuses = IndustrialBuilding::GetBonuses();

		if (IsUpgraded(0)) {
			bonuses.push_back({ "Improved Grinder", 10 });
		}
		
		return bonuses;
	}

	// Check other nearby windmill for efficiency
	static int32 WindmillBaseEfficiency(int32 playerId, WorldTile2 centerTileIn, IGameSimulationCore* simulation)
	{
		const std::vector<int32>& windmills = simulation->buildingIds(playerId, CardEnum::Windmill);

		// Adjust efficiency by distance linearly
		// efficiency from pairing with other windmill gets multiplied together for the final efficiency
		int32 efficiency = 100;
		int32 radiusTouchAtom = 2 * Radius * CoordinateConstants::AtomsPerTile; // 2*Radius because that is when two windmill's radii starts to overlap
		for (int32 windmillId : windmills) {
			WorldTile2 centerTile = simulation->building(windmillId).centerTile();
			if (centerTileIn != centerTile) {
				int32 atomDist = WorldAtom2::Distance(centerTileIn.worldAtom2(), centerTile.worldAtom2());
				if (atomDist < radiusTouchAtom) {
					int32 pairEfficiency = atomDist * 100 / radiusTouchAtom;
					efficiency = efficiency * pairEfficiency / 100;
				}
			}
		}
		return efficiency;
	}
	
	int32 efficiencyBeforeBonus() override {
		return WindmillBaseEfficiency(_playerId, centerTile(), _simulation);
	}

	static const int32 Radius = 16;
};
class Bakery final : public IndustrialBuilding
{
public:
	void OnInit() override
	{
		SetupWorkMode({
			{"Coal-fired", ResourceEnum::Flour, ResourceEnum::Coal, 5, ResourceEnum::None},
			{"Wood-fired", ResourceEnum::Flour, ResourceEnum::Wood, 5, ResourceEnum::None, "Wood-fired oven cook food faster locking in more nutrient. +30% productivity"},
		});
	}
	
	void FinishConstruction() final
	{
		Building::FinishConstruction();

		AddResourceHolder(ResourceEnum::Flour, ResourceHolderType::Requester, 0);
		AddResourceHolder(ResourceEnum::Coal, ResourceHolderType::Requester, 0);
		AddResourceHolder(ResourceEnum::Wood, ResourceHolderType::Requester, 0);
		AddResourceHolder(ResourceEnum::Bread, ResourceHolderType::Provider, 0);

		_upgrades = {
			MakeUpgrade("Improved Oven", "+10% productivity", ResourceEnum::Stone, 50),
			MakeComboUpgrade("Baker Guild", ResourceEnum::Paper, 50, 15),
		};

		ChangeWorkMode(_workMode); // Need this to setup resource target etc.
	}

	std::vector<BonusPair> GetBonuses() final {
		std::vector<BonusPair> bonuses = IndustrialBuilding::GetBonuses();

		if (IsUpgraded(0)) {
			bonuses.push_back({ "Improved Oven", 10 });
		}

		if (workMode().name == "Wood-fired") {
			bonuses.push_back({ "Wood-fired", 30 });
		}
		
		return bonuses;
	}

	//int32 baseInputPerBatch() {
	//	return 5;
	//}
};
class Jeweler final : public IndustrialBuilding
{
public:
	void FinishConstruction() final
	{
		Building::FinishConstruction();

		_upgrades = {
			MakeProductionUpgrade("Rigorous Training", ResourceEnum::Brick, 80, 50),
			MakeProductionUpgrade("Specialized Tools", ResourceEnum::SteelTools, 80, 50),
			MakeComboUpgrade("Jeweler's Guild", ResourceEnum::Brick, 50, 20),
		};
	}

};


class Brickworks final : public IndustrialBuilding
{
public:
	void FinishConstruction() final
	{
		Building::FinishConstruction();

		_upgrades = {
			MakeProductionUpgrade("Specialized Tools", ResourceEnum::Stone, 50, 50),
			MakeComboUpgrade("Brickworks Town", ResourceEnum::Brick, 50, 20),
		};
	}

};

class CandleMaker final : public IndustrialBuilding
{
public:
	void FinishConstruction() final
	{
		Building::FinishConstruction();

		_upgrades = {
			MakeProductionUpgrade("Specialized Tools", ResourceEnum::SteelTools, 50, 50),
			MakeComboUpgrade("Candle Maker Guild", ResourceEnum::Brick, 50, 20),
		};
	}

};

class CottonMill final : public IndustrialBuilding
{
public:
	void FinishConstruction() final
	{
		Building::FinishConstruction();

		_upgrades = {
			MakeProductionUpgrade("Advanced Machinery", ResourceEnum::Iron, 250, 200),
			MakeComboUpgrade("Cotton Mill Town", ResourceEnum::Iron, 80, 50),
		};
	}

};

class PrintingPress final : public IndustrialBuilding
{
public:
	void FinishConstruction() final
	{
		Building::FinishConstruction();

		_upgrades = {
			MakeProductionUpgrade("Advanced Machinery", ResourceEnum::Iron, 500, 300),
			MakeComboUpgrade("Printing Press Town", ResourceEnum::Iron, 80, 50),
		};
	}

};



class ClayPit final : public IndustrialBuilding
{
public:
	void OnInit() override {
		TileArea digArea = _area;
		digArea.minX++;
		digArea.minY++;
		digArea.maxX--;
		digArea.maxY--;
		digArea.ExecuteOnArea_WorldTile2([&](WorldTile2 tile) {
			WorldTile2 bldTileBeforeRotation = UndoRotateBuildingTileByDirection(tile - centerTile());
			if (bldTileBeforeRotation != WorldTile2(-1, -1) &&
				bldTileBeforeRotation != WorldTile2(0, -1)) {
				_simulation->terrainChanges().AddHole(tile);
			}
		});
	}
	
	void FinishConstruction() override {
		Building::FinishConstruction();

		_upgrades = {
			BuildingUpgrade("More workers", "+1 worker slots.", 80),
		};
	}

	void OnDeinit() override {
		TileArea digArea = _area;
		digArea.minX++;
		digArea.minY++;
		digArea.maxX--;
		digArea.maxY--;
		digArea.ExecuteOnArea_WorldTile2([&](WorldTile2 tile) {
			WorldTile2 bldTileBeforeRotation = UndoRotateBuildingTileByDirection(tile - centerTile());
			if (bldTileBeforeRotation != WorldTile2(-1, -1) &&
				bldTileBeforeRotation != WorldTile2(0, -1)) {
				_simulation->terrainChanges().RemoveHole(tile);
			}
		});
	}

	void OnUpgradeBuilding(int upgradeIndex) final {
		if (upgradeIndex == 0) {
			SetJobBuilding(3);
		}
	}
};

class Potter final : public IndustrialBuilding
{
public:
	void FinishConstruction() override {
		Building::FinishConstruction();

		_upgrades = {
			MakeProductionUpgrade("Improved kiln", 50, 30),
			BuildingUpgrade("More workers", "+1 workers.", 100),
			MakeComboUpgrade("Potter Town", ResourceEnum::Stone, 50, 20),
		};

		_simulation->TryAddQuest(_playerId, std::make_shared<PotteryQuest>());
	}

	void OnUpgradeBuilding(int upgradeIndex) final {
		if (upgradeIndex == 1) {
			SetJobBuilding(3);
		}
	}

	std::vector<BonusPair> GetBonuses() override {
		std::vector<BonusPair> bonuses = IndustrialBuilding::GetBonuses();
		if (_simulation->TownhallCardCount(_playerId, CardEnum::MasterPotter) > 0) {
			bonuses.push_back({ "Master potter", 20 });
		}

		return bonuses;
	}
};

class FurnitureWorkshop final : public IndustrialBuilding
{
public:
	void FinishConstruction() final
	{
		Building::FinishConstruction();

		_upgrades = {
			BuildingUpgrade("More workers", "+1 worker slots.", 100),
			BuildingUpgrade("Minimalism", "Consumes 30% less input.", 200),
			MakeComboUpgrade("Furniture Town", ResourceEnum::Stone, 20, 20),
		};
	}

	void OnUpgradeBuilding(int upgradeIndex) final {
		if (upgradeIndex == 0) {
			SetJobBuilding(3);
		}
	}

	int32 baseInputPerBatch() override {
		return Building::baseInputPerBatch() * (IsUpgraded(1) ? 70 : 100) / 100;
	}

	std::vector<BonusPair> GetBonuses() final {
		std::vector<BonusPair> bonuses = IndustrialBuilding::GetBonuses();
		if (_simulation->IsResearched(_playerId, TechEnum::Sawmill)) {
			bonuses.push_back({ "Sawmill tech", 50 });
		}
		
		return bonuses;
	}

};

/*
 * Others
 */

class Fisher final : public Building
{
public:
	void OnInit() final {
		ChangeFisherTilesInRadius(1);
	}

	void FinishConstruction() final
	{
		Building::FinishConstruction();

		_resourceDisplayShift = FVector(-10, -10, 0);

		//AddResourceHolder(ResourceEnum::WhaleMeat, ResourceHolderType::Provider, 0);

		_upgrades = {
			BuildingUpgrade("Juicier Bait", "Juicier bait that attracts more fish. +25% productivity.", 300),
			MakeUpgrade("Improved Fishing Tools", "+50% productivity.", ResourceEnum::SteelTools, 140),
			BuildingUpgrade("More Workers", "+1 worker slots", 100),
			//BuildingUpgrade("Whaling", "Catch whale from deep sea instead.\n  Produces whale meat.\n  +2 worker slots.\n  No effect nearby fish population", 120)
		};
		// TODO: urchin harvester (luxury?)
		// There are about 950 species of sea urchins that inhabit a wide range of depth zones in all climates across the world’s oceans. About 18 of them are edible.

		PUN_LOG("FinishContruction Fisher %d", buildingId());

		_simulation->TryAddQuest(_playerId, std::make_shared<CooperativeFishingQuest>());
	}

	void OnDeinit() final {
		ChangeFisherTilesInRadius(-1);
	}

	static const int32 Radius = 12;

	//ResourceEnum product() final {
	//	return IsUpgraded(1) ? ResourceEnum::WhaleMeat : ResourceEnum::Fish;
	//}
	ResourceEnum product() final {
		return ResourceEnum::Fish;
	}

	int32 workManSecPerBatch100() final {
		return Building::workManSecPerBatch100() * 100 / 130; // Fisher work faster
	}

	void OnUpgradeBuilding(int upgradeIndex) final {
		//if (upgradeIndex == 1) {
		//	_maxOccupants = 4;
		//	_allowedOccupants = _maxOccupants;
		//	ChangeFisherTilesInRadius(-1);
		//}
		if (upgradeIndex == 2) {
			SetJobBuilding(3);
		}
	}

	void ChangeFisherTilesInRadius(int32 valueChange);
	static int32 FisherAreaEfficiency(WorldTile2 centerTile, bool alreadyPlaced, WorldTile2 extraFisherTile, IGameSimulationCore* simulation); // TODO: extraFisher tile for showing all fisher's effiency during placement...

	std::vector<BonusPair> GetBonuses() final {
		std::vector<BonusPair> bonuses = Building::GetBonuses();
		int32 cardCount = _simulation->TownhallCardCount(_playerId, CardEnum::CooperativeFishing);
		if (cardCount > 0) {
			bonuses.push_back({ "Cooperative Fishing", cardCount * 10 });
		}
		if (IsUpgraded(0)) {
			bonuses.push_back({ "Juicier Bait", 25 });
		}
		if (IsUpgraded(1)) {
			bonuses.push_back({ "Improved Fishing Tools", 50 });
		}
		return bonuses;
	}
	
	int32 efficiencyBeforeBonus() override {
		return FisherAreaEfficiency(_centerTile, true, WorldTile2::Invalid, _simulation);
	}
};


class LovelyHeartStatue : public Building 
{
public:
	void FinishConstruction() final
	{
		Building::FinishConstruction();
		_simulation->AddTickBuilding(_objectId);

		tickCount = 0;
	}

	void Tick() override;

	int32_t tickCount;
};

class SmallMarket : public Building
{
public:

};


class RoadConstruction final : public Building
{
public:
	void OnInit() final;
	void FinishConstruction() final;

	bool isDirt() { return isEnum(CardEnum::DirtRoad); }
};

class Trap final : public Building
{
public:
	void Tick1Sec() final;
};

class Fence final : public Building
{
public:
	void FinishConstruction() final {
		Building::FinishConstruction();
		_simulation->SetWalkable(_centerTile, false);

		// Need to refresh surrounding tiles (skip self, no need)
		WorldTile2 tile = centerTile();
		_simulation->SetNeedDisplayUpdate(DisplayClusterEnum::Building, WorldTile2(tile.x + 1, tile.y).regionId(), true);
		_simulation->SetNeedDisplayUpdate(DisplayClusterEnum::Building, WorldTile2(tile.x - 1, tile.y).regionId(), true);
		_simulation->SetNeedDisplayUpdate(DisplayClusterEnum::Building, WorldTile2(tile.x, tile.y + 1).regionId(), true);
		_simulation->SetNeedDisplayUpdate(DisplayClusterEnum::Building, WorldTile2(tile.x, tile.y - 1).regionId(), true);
	}
};

class FenceGate final : public Building 
{
public:
	void FinishConstruction() final;
	void OnDeinit() final;
};

class Bridge final : public Building
{
public:
	void OnInit() override {
		FinishConstruction();
	}

	void OnDeinit() final
	{
		Building::OnDeinit();
		
		//PUN_LOG("Bridge OnDeinit");
		
		_area.ExecuteOnArea_WorldTile2([&](WorldTile2 tile) {
			//PUN_LOG("Bridge SetAreaWalkable false: %s", *tile.To_FString());
			_simulation->SetWalkable(tile, false);
		});

		// Remove Marked Road
		_area.ExecuteOnArea_WorldTile2([&](WorldTile2 tile) {
			_simulation->SetRoadPathAI(tile, false);
		});
	}

	void SetAreaWalkable() override
	{
		// Make the area on which this was built walkable.
		_area.ExecuteOnArea_WorldTile2([&](WorldTile2 tile) {
			_simulation->SetWalkable(tile, true);
			//PUN_LOG("Bridge SetAreaWalkable true: %s", *tile.To_FString());
		});

		// clear the tile on each side of the bridge, in case the tree is blocking
		if (_area.sizeX() > 1) {
			_simulation->treeSystem().ForceRemoveTileObj(WorldTile2(_area.minX - 1, _area.minY));
			_simulation->treeSystem().ForceRemoveTileObj(WorldTile2(_area.maxX + 1, _area.minY));
		}
		if (_area.sizeY() > 1) {
			_simulation->treeSystem().ForceRemoveTileObj(WorldTile2(_area.minX, _area.minY - 1));
			_simulation->treeSystem().ForceRemoveTileObj(WorldTile2(_area.minX, _area.maxY + 1));
		}
		
		_simulation->ResetUnitActionsInArea(_area);

		_didSetWalkable = true;

		// Mark the area as road for Trade Route
		_area.ExecuteOnArea_WorldTile2([&](WorldTile2 tile) {
			_simulation->SetRoadPathAI(tile, true);
		});
	}

	int32 maxCardSlots() override { return 0; }
};



//! ConsumerWorkplace

class ConsumerWorkplace : public Building
{
public:
	// Same amount of work required to acquire resources
	int32 workManSecPerBatch100() final
	{
		// 500 - 10 * (gold price 25) = 250 profit
		return (500 - batchCost()) * 100 * 100 / buildingInfo().workRevenuePerSec100_perMan; // first 100 for workManSecPerBatch100, second 100 to cancel out WorkRevenuePerManSec100
	}

	// TODO: remove these...
	virtual void DoConsumerWork(int32_t workManSec100, int32_t resourceConsumed) = 0;

	int32 resourceConsumptionAmount100(int32 workManSec100) { return inputPerBatch() * workManSec100 * 100 / workManSecPerBatch100(); }
};

class Library final : public Building
{
public:
	static const int32_t Radius = 20;
	static const int32_t SciencePerHouse = 2;

	static const int32 MinHouseLvl = 2;

	void FinishConstruction() final {
		Building::FinishConstruction();
		_simulation->RecalculateTaxDelayed(_playerId);
	}
};
class School final : public Building
{
public:
	static const int32_t Radius = 20;
	static const int32_t SciencePerHouse = 3;

	static const int32 MinHouseLvl = 3;

	void FinishConstruction() final {
		Building::FinishConstruction();
		_simulation->RecalculateTaxDelayed(_playerId);
	}
};

class Bank final : public Building
{
public:
	static const int32_t Radius = 20;
	static const int32_t ProfitPerHouse = 10;

	static const int32 MinHouseLvl = 2;
	
	void FinishConstruction() final {
		Building::FinishConstruction();
		roundProfit = 0;
	}

	void CalculateRoundProfit();

	void Serialize(FArchive& Ar) override {
		Building::Serialize(Ar);
		Ar << roundProfit;
	}

	int32 roundProfit;
};

class FunBuilding : public Building
{
public:
	virtual int32 serviceQuality() { return 90; }

	// System:
	//  - 1 Tavern serves ~40 people (10 per season)
	//  - citizens visit starting below 70 fun... so assum ~60
	//  - each need 1 visit per year taking fun from ~60 -> 90
	//  - 20 fun loss per year... so takes 2 years to reach 50 from the initial 90

	void TickRound() override {
		_guestCountLastRound = _guestCountThisRound;
		_guestCountThisRound = 0;
	}

	void UseService() {
		_guestCountThisRound++;
	}

	// Debug:
	int32 guestCountLastRound() { return _guestCountLastRound; }
	int32 guestCountThisRound() { return _guestCountThisRound; }

	void Serialize(FArchive& Ar) override {
		Building::Serialize(Ar);
		Ar << _guestCountLastRound;
		Ar << _guestCountThisRound;
	}

private:
	int32 _guestCountLastRound = 0;
	int32 _guestCountThisRound = 0;
};

class Theatre final : public FunBuilding
{
public:
	static const int32 Radius = 30;

	static const int32 MinHouseLvl = 2;
	
	int32 serviceQuality() override { return 110; }
};


class Tavern final : public FunBuilding
{
public:
	static const int32 Radius = 30;
	int32 serviceQuality() override { return 90; }
};


//class Barracfk final : public Building
//{
//public:
//	void FinishConstruction() final {
//		Building::FinishConstruction();
//	}
//
//	const ArmyInfo& armyInfo() { return GetArmyInfo(_armyEnum); }
//
//	void QueueTrainUnit()
//	{
//		resourceSystem().ChangeMoney(-armyInfo().moneyCost);
//		for (ResourcePair pair : armyInfo().resourceCost) {
//			resourceSystem().RemoveResourceGlobal(pair.resourceEnum, pair.count);
//		}
//		
//		if (_queueCount < maxQueueSize()) {
//			_queueCount++;
//		}
//		TryStartTraining();
//	}
//
//	// Done Training
//	void ScheduleTick() override;
//
//	float trainingPercent() {
//		if (_trainingStartTick == -1) {
//			return 0;
//		}
//		return (Time::Ticks() - _trainingStartTick) * 100.0f / armyInfo().timeCostTicks();
//	}
//
//	static int32 maxQueueSize() { return 10; }
//	int32 queueCount() { return _queueCount; }
//
//	/*
//	 * Build/Unit cost scaling
//	 * - Clubman 1
//	 * - Swordman 3
//	 * - Archer 2
//	 *
//	 */
//
//	void SetArmyEnum(ArmyEnum armyEnum) {
//		_armyEnum = armyEnum;
//	}
//
//	void TryCancelTrainingQueue()
//	{
//		if (_queueCount > 0) {
//			if (_queueCount == 1) {
//				CancelCurrentTraining();
//			}
//			
//			_queueCount--;
//			AddBackTrainingResource();
//		}
//	}
//
//	void Serialize(FArchive& Ar) override
//	{
//		Building::Serialize(Ar);
//		Ar << _armyEnum;
//		Ar << _queueCount;
//		Ar << _trainingStartTick;
//	}
//
//private:
//	void TryStartTraining()
//	{
//		if (_queueCount > 0 && _trainingStartTick == -1) {
//			_trainingStartTick = Time::Ticks();
//
//			int32 timeCost = armyInfo().timeCostTicks();
//			if (SimSettings::IsOn("CheatFastBuild")) {
//				timeCost /= 60;
//			}
//			
//			_simulation->ScheduleTickBuilding(buildingId(), _trainingStartTick + timeCost);
//		}
//	}
//
//	void CancelCurrentTraining()
//	{
//		if (_trainingStartTick != -1) {
//			_trainingStartTick = -1;
//			_simulation->RemoveScheduleTickBuilding(buildingId());
//		}
//	}
//
//	void RemoveTrainingResource()
//	{
//		resourceSystem().ChangeMoney(-armyInfo().moneyCost);
//		for (ResourcePair pair : armyInfo().resourceCost) {
//			resourceSystem().RemoveResourceGlobal(pair.resourceEnum, pair.count);
//		}
//	}
//	void AddBackTrainingResource()
//	{
//		resourceSystem().ChangeMoney(armyInfo().moneyCost);
//		for (ResourcePair pair : armyInfo().resourceCost) {
//			resourceSystem().AddResourceGlobal(pair.resourceEnum, pair.count, *_simulation);
//		}
//	}
//	
//private:
//	ArmyEnum _armyEnum = ArmyEnum::Clubman;
//	int32 _queueCount = 0;
//	int32 _trainingStartTick = -1;
//};

class ProvinceBuilding : public Building
{
public:
	int32 maxCardSlots() override { return 0; }
	
	void OnInit() override
	{
		InstantClearArea();
	}
	
	void Tick1Sec() override {
		TickConstruction(100); // takes 100 secs to finish the constrution
	}
};


class Fort final : public ProvinceBuilding
{
public:
};

class Colony final : public ProvinceBuilding
{
public:
	static int32 GetColonyIncomeValue() { return GetBuildingInfo(CardEnum::Colony).baseCardPrice / 16; } // 16 rounds or 2 year to recoup the 
	static int32 GetColonyUpkeep() { return (GetColonyIncomeValue() / 3) / 10 * 10; }

	static int32 GetColonyResourceIncome(ResourceEnum resourceEnum) {
		return GetColonyIncomeValue() / GetResourceInfo(resourceEnum).basePrice;
	}
	ResourceEnum GetColonyResourceEnum() {
		return _simulation->georesource(_simulation->GetProvinceIdClean(centerTile())).info().resourceEnum;
	}

	void TickRound() override;
};


//! Other workplace


class LaborerGuild final : public Building
{
public:
};

class ShippingDepot final : public Building
{
public:
	void OnInit() override {
		resourceEnums.resize(3, ResourceEnum::None);
	}
	
	void Serialize(FArchive& Ar) override {
		Building::Serialize(Ar);
		SerializeVecValue(Ar, resourceEnums);
	}

	std::vector<ResourceEnum> resourceEnums;
	static const int32 Radius = 12;
};


class IrrigationReservoir final : public Building
{
public:
	void OnInit() override {
		TileArea digArea = _area;
		digArea.minX++;
		digArea.minY++;
		digArea.maxX--;
		digArea.maxY--;
		digArea.ExecuteOnArea_WorldTile2([&](WorldTile2 tile) {
			_simulation->terrainChanges().AddHole(tile);
		});
	}

	void FinishConstruction() override {
		Building::FinishConstruction();

		_upgrades = {
			MakeUpgrade("Wind-powered Pump", "Halve the upkeep if adjacent to Windmill.", 20),
		};
	}

	void OnDeinit() override {
		TileArea digArea = _area;
		digArea.minX++;
		digArea.minY++;
		digArea.maxX--;
		digArea.maxY--;
		digArea.ExecuteOnArea_WorldTile2([&](WorldTile2 tile) {
			_simulation->terrainChanges().RemoveHole(tile);
		});
	}

	int32 upkeep() override {
		int32 baseUpkeep = 80;
		if (IsUpgraded(0)) {
			return baseUpkeep / 2;
		}
		return baseUpkeep;
	}

	static const int32 Radius = 12;
};