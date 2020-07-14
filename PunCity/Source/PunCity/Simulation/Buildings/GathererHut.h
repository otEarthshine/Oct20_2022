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
			BuildingUpgrade("Delicate gathering", "+20% efficiency.", 80),
			BuildingUpgrade("Fruit bait for hunters", "Hunter in the same region gets +30% efficiency (does not stack).", 80),
		};
	}

	ResourceEnum product() final { return ResourceEnum::Orange; }

	std::vector<BonusPair> GetBonuses() override
	{
		std::vector<BonusPair> bonuses = Building::GetBonuses();

		if (IsUpgraded(0)) {
			bonuses.push_back({ "Delicate gathering upgrade", 20 });
		}
		
		std::vector<Building*> buildings = GetBuildingsInRegion(CardEnum::HuntingLodge);
		for (Building* building : buildings) {
			if (building->IsUpgraded(1)) {
				bonuses.push_back({ "Hunting lodge upgrade", 30 });
				break;
			}
		}
		
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
			BuildingUpgrade("Smoking chamber", "+30% efficiency.", 80),
			BuildingUpgrade("Trap fruit-eating pests.", "Fruit gatherer in the same region gets +30% productivity (does not stack).", 80),
		};
	}

	std::vector<BonusPair> GetBonuses() override {
		std::vector<BonusPair> bonuses = Building::GetBonuses();

		if (IsUpgraded(0)) {
			bonuses.push_back({ "Smoking chamber", 30 });
		}
		
		const std::vector<int32>& buildingIds = _simulation->buildingIds(_playerId, CardEnum::FruitGatherer);
		for (int32 buildingId : buildingIds) {
			Building& building = _simulation->building(buildingId);
			PUN_CHECK(building.isEnum(CardEnum::FruitGatherer));
			if (building.centerTile().region() == centerTile().regionId() &&
				building.IsUpgraded(1))
			{
				bonuses.push_back({ "Gatherer upgrade", 30 });
			}
		}
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
			{"Cut and plant", ResourceEnum::None, ResourceEnum::None, 0},
			{"Prioritize planting", ResourceEnum::None, ResourceEnum::None, 0},
			{"Prioritize cutting", ResourceEnum::None, ResourceEnum::None, 0},
		});
	}
	
	void FinishConstruction() override
	{
		Building::FinishConstruction();

		_upgrades = {
			BuildingUpgrade("Timber management", "+30% efficiency.", 200),
			BuildingUpgrade("Forest Town", "+30% production when you own 3+ foresters.", 300),
		};
	}

	ResourceEnum product() override { return ResourceEnum::Wood; }

	std::vector<BonusPair> GetBonuses() override {
		std::vector<BonusPair> bonuses = Building::GetBonuses();

		if (IsUpgraded(0)) {
			bonuses.push_back({ "Timber management", 30 });
		}
		if (IsUpgraded(1)) {
			if (_simulation->buildingCount(_playerId, CardEnum::Forester) >= 3) {
				bonuses.push_back({ "Forest Town", 30 });
			}
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
			//BuildingUpgrade("Shroomery", "Produce psychedelic shroom (luxury) instead.", 200),
			BuildingUpgrade("Intensive care", "+30% production bonus when worker slots are full", 200),
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

		_upgrades = {
			BuildingUpgrade("Intensive care", "+30% production bonus when worker slots are full", 200),
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

	std::vector<BonusPair> GetBonuses() override {
		std::vector<BonusPair> bonuses = Building::GetBonuses();
		//if (_simulation->townLvl(_playerId) >= 2) {
		//	bonuses.push_back({ "Townhall upgrade", 30 });
		//}
		if (_simulation->IsResearched(_playerId, TechEnum::FarmingBreakthrough)) {
			bonuses.push_back({ "Farm breakthrough upgrade", 20 });
		}
		if (_simulation->IsResearched(_playerId, TechEnum::FarmImprovement)) {
			bonuses.push_back({ "Farm improvement upgrade", 5 });
		}
		if (_simulation->buildingFinishedCount(_playerId, CardEnum::DepartmentOfAgriculture) && 
			_simulation->buildingCount(_playerId, CardEnum::Farm) >= 8)
		{
			bonuses.push_back({ "Department of agriculture", 5 });
		}
		if (_simulation->buildingFinishedCount(_playerId, CardEnum::CensorshipInstitute)) {
			bonuses.push_back({ "Censorship", 7 });
		}

		if (_simulation->IsResearched(_playerId, TechEnum::FarmLastEra)) {
			bonuses.push_back({ "Last era technology", 20 });
		}

		return bonuses;
	}

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
		SerializeVecLoop(Ar, _reservingUnitIdToFarmTileId, [&](std::pair<int32_t, int32_t>& pair) {
			Ar << pair.first;
			Ar << pair.second;
		});
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
			BuildingUpgrade("More Workers", "+2 worker slots", 120),
			BuildingUpgrade("Improved shift", "Mine with full worker slots get 20% production bonus", 500),
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
		if (_simulation->townLvl(_playerId) >= 3) {
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

	int32 oreLeft();

	void OnProduce(int32 productionAmount) override;
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
			BuildingUpgrade("Better process", "Uses 50% less wood to produce paper.", 500),
			BuildingUpgrade("More workers", "+2 worker slots.", 120),
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
			BuildingUpgrade("Teamwork", "Smelter with full worker slots get 50% production bonus", 500),
			BuildingUpgrade("Efficient furnace", "Decrease input by 30%", 1000),
		};
	}

	std::vector<BonusPair> GetBonuses() override {
		std::vector<BonusPair> bonuses = IndustrialBuilding::GetBonuses();
		if (_simulation->buildingCount(_playerId, CardEnum::EnvironmentalistGuild)) {
			bonuses.push_back({ "Environmentalist", -30 });
		}
		if (IsUpgraded(0) && isOccupantFull()) {
			bonuses.push_back({ "Teamwork", 50 });
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
			if (GetBuildingsInRegion(CardEnum::IronSmelter).size() >= 2) {
				bonuses.push_back({ "Iron smelt combo", 30 });
			}
		}

		if (_simulation->TownhallCardCount(_playerId, CardEnum::CoalPipeline) > 0) {
			if (_simulation->resourceCount(_playerId, ResourceEnum::Coal) >= 1000) {
				bonuses.push_back({ "Coal pipeline", 30 });
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

class Mint final : public IndustrialBuilding
{
public:
	void FinishConstruction() final
	{
		IndustrialBuilding::FinishConstruction();

		_upgrades = {
			BuildingUpgrade("Improved Production", "+30% production.", 500),
			BuildingUpgrade("Mint Town", "+30% production when you have 4 or more Mints.", 500),
		};
	}

	std::vector<BonusPair> GetBonuses() override {
		std::vector<BonusPair> bonuses = IndustrialBuilding::GetBonuses();

		if (IsUpgraded(0)) {
			bonuses.push_back({ "Improved Production", 30 });
		}
		if (IsUpgraded(1)) {
			if (_simulation->buildingCount(_playerId, CardEnum::Mint) >= 4) {
				bonuses.push_back({ "Mint Town", 30 });
			}
		}
		return bonuses;
	}
	
	bool NeedWork() final {
		return MathUtils::SumVector(_workReserved) + _workDone100 < workManSecPerBatch100();
	}

	// Same amount of work required to acquire resources
	int32 workManSecPerBatch100() final
	{
		// 500 - 10 * (gold price 25) = 250 profit
		return (500 - batchCost()) * 100 * 100 / buildingInfo().workRevenuePerSec100_perMan; // first 100 for workManSecPerBatch100, second 100 to cancel out WorkRevenuePerManSec100
	}

	int32 productPerBatch() override {
		return inputPerBatch() * _simulation->price100(input1()) * 2 * efficiency() / 100 / 100;
	}
};

class CardMaker final : public IndustrialBuilding
{
public:
	void FinishConstruction() final
	{
		IndustrialBuilding::FinishConstruction();

		_upgrades = {
			BuildingUpgrade("Improved Production", "+30% production.", 500),
		};
	}

	std::vector<BonusPair> GetBonuses() override {
		std::vector<BonusPair> bonuses = IndustrialBuilding::GetBonuses();

		if (IsUpgraded(0)) {
			bonuses.push_back({ "Improved Production", 30 });
		}
		return bonuses;
	}

	bool NeedWork() final {
		return MathUtils::SumVector(_workReserved) + _workDone100 < workManSecPerBatch100();
	}

	// Same amount of work required to acquire resources
	int32 workManSecPerBatch100() final
	{
		// Assume card 800 price here...
		return (800 - batchCost()) * 100 * 100 / buildingInfo().workRevenuePerSec100_perMan; // first 100 for workManSecPerBatch100, second 100 to cancel out WorkRevenuePerManSec100
	}

	int32 baseInputPerBatch() override { return 10; }
};

class ImmigrationOffice final : public IndustrialBuilding
{
public:
	void FinishConstruction() final
	{
		IndustrialBuilding::FinishConstruction();

		_upgrades = {
			BuildingUpgrade("First Impression", "+30% efficiency.", 300),
		};
	}

	std::vector<BonusPair> GetBonuses() override {
		std::vector<BonusPair> bonuses = IndustrialBuilding::GetBonuses();

		if (IsUpgraded(0)) {
			bonuses.push_back({ "First Impression Upgrade", 30 });
		}
		return bonuses;
	}

	bool NeedWork() final {
		return MathUtils::SumVector(_workReserved) + _workDone100 < workManSecPerBatch100();
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
			BuildingUpgrade("Improved Forge", "+30% production.", 1000),
			BuildingUpgrade("Alloy Recipe", "+30% production.", 1200),
			BuildingUpgrade("Blacksmith Guild", "+30% production when you have 4 or more Blacksmith.", 1000),
		};
	}

	std::vector<BonusPair> GetBonuses() override {
		std::vector<BonusPair> bonuses = Building::GetBonuses();
		if (IsUpgraded(0)) {
			bonuses.push_back({ "Improved Forge", 30 });
		}
		if (IsUpgraded(1)) {
			bonuses.push_back({ "Alloy Recipe", 30 });
		}
		if (IsUpgraded(2)) {
			if (_simulation->buildingCount(_playerId, CardEnum::Blacksmith) >= 4) {
				bonuses.push_back({ "Blacksmith Guild", 30 });
			}
		}
		return bonuses;
	}
};
class Herbalist : public IndustrialBuilding
{
public:
};
class MedicineMaker : public IndustrialBuilding
{
public:
	int32 baseInputPerBatch() override {
		return 5;
	}
};

class CharcoalMaker final : public IndustrialBuilding
{
public:
	void FinishConstruction() final
	{
		Building::FinishConstruction();

		_upgrades = {
			BuildingUpgrade("Charcoal Conversion", "Use 30% less wood input.", 50),
			BuildingUpgrade("Improved Production", "+30% production.", 200),
		};
	}

	//void OnUpgradeBuilding(int upgradeIndex) final {
	//	if (upgradeIndex == 0) {
	//		SetJobBuilding(5);
	//	}
	//}

	int32 baseInputPerBatch() override { return IsUpgraded(0) ? 7 : 10; }

	std::vector<BonusPair> GetBonuses() override {
		std::vector<BonusPair> bonuses = Building::GetBonuses();
		if (_simulation->buildingCount(_playerId, CardEnum::EnvironmentalistGuild)) {
			bonuses.push_back({ "Environmentalist", -30 });
		}
		if (IsUpgraded(1)) {
			bonuses.push_back({ "Improved Production", 30 });
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
			BuildingUpgrade("Cocoa Processing", "Consumes 50% less input.", 2000),
			BuildingUpgrade("Improved Production", "+50% production.", 1000),
			BuildingUpgrade("Chocolate Town", "+50% production when you have 4 or more chocolatiers.", 1000),
			BuildingUpgrade("Reduce Upkeep", "Reduce upkeep by 50%", 1000),
		};
	}

	std::vector<BonusPair> GetBonuses() override {
		std::vector<BonusPair> bonuses = IndustrialBuilding::GetBonuses();

		if (IsUpgraded(1)) {
			bonuses.push_back({ "Improved Production", 50 });
		}
		if (IsUpgraded(2)) {
			if (_simulation->buildingCount(_playerId, CardEnum::Chocolatier) >= 4) {
				bonuses.push_back({ "Chocolate Town", 50 });
			}
		}
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
			BuildingUpgrade("Wine appreciation", "+50% production.", 2000),
			BuildingUpgrade("Wine Town", "+50% production when you have 4 or more wineries.", 1000),
		};
	}

	std::vector<BonusPair> GetBonuses() final {
		std::vector<BonusPair> bonuses = IndustrialBuilding::GetBonuses();
		if (_simulation->IsResearched(_playerId, TechEnum::WineryImprovement)) {
			bonuses.push_back({"Winery improvement tech", 30});
		}
		if (IsUpgraded(0)) {
			bonuses.push_back({ "Wine appreciation", 50 });
		}
		if (IsUpgraded(1)) {
			if (_simulation->buildingCount(_playerId, CardEnum::Winery) >= 4) {
				bonuses.push_back({ "Wine Town", 50 });
			}
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
			BuildingUpgrade("Weaving Machine", "+100% production.", 3000),
			BuildingUpgrade("Tailor Town", "+50% production when you have 4 or more tailors.", 1000),
		};
	}

	//ResourceEnum input1() final { return _workMode.input1; }
	//ResourceEnum input2() final { return _workMode.input2; }
	//int32 baseInputPerBatch() final { return _workMode.inputPerBatch; }

	std::vector<BonusPair> GetBonuses() final {
		std::vector<BonusPair> bonuses = IndustrialBuilding::GetBonuses();
		if (IsUpgraded(0)) {
			bonuses.push_back({ "Weaving Machine", 100 });
		}
		if (IsUpgraded(1)) {
			if (_simulation->buildingCount(_playerId, CardEnum::Tailor) >= 4) {
				bonuses.push_back({ "Tailor Town", 50 });
			}
		}
		return bonuses;
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

		AddResourceHolder(ResourceEnum::Wheat, ResourceHolderType::Requester, 20);
		AddResourceHolder(ResourceEnum::Orange, ResourceHolderType::Requester, 20);
		AddResourceHolder(ResourceEnum::Mushroom, ResourceHolderType::Requester, 20);
		AddResourceHolder(ResourceEnum::Beer, ResourceHolderType::Provider, 0);
		
		_upgrades = {
			BuildingUpgrade("Improved Malting", "Consumes 30% less input.", 200),
			BuildingUpgrade("Fast Malting", "+30% production.", 200),
			BuildingUpgrade("Brewery Town", "+30% production when you have 4 or more breweries.", 300),
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

		if (IsUpgraded(1)) {
			bonuses.push_back({ "Fast malting", 30 });
		}
		if (IsUpgraded(2)) {
			if (_simulation->buildingCount(_playerId, CardEnum::BeerBrewery) >= 4) {
				bonuses.push_back({ "Brewery Town", 30 });
			}
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
			BuildingUpgrade("Improved Grinder", "+10% productivity", 500),
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
	static int32 WindmillBaseEfficiency(int32 playerId, WorldTile2 centerTileIn, IGameSimulationCore* simulation) {
		const std::vector<int32>& windmills = simulation->buildingIds(playerId, CardEnum::Windmill);

		// Adjust efficiency by distance linearly
		// efficiency from pairing with other windmill gets multiplied together for the final efficiency
		int32 efficiency = 100;
		int32 radiusTouchAtom = 2 * Radius * CoordinateConstants::AtomsPerTile;
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

	static const int32 Radius = 20;
};
class Bakery final : public IndustrialBuilding
{
public:
	void OnInit() override
	{
		SetupWorkMode({
			{"Coal-fired", ResourceEnum::Flour, ResourceEnum::Coal, 5, ResourceEnum::None},
			{"Wood-fired", ResourceEnum::Flour, ResourceEnum::Wood, 5, ResourceEnum::None, "Wood-fired oven cook food faster locking in more nutrient. +20% productivity"},
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
			BuildingUpgrade("Improved Oven", "+10% productivity", 700),
			BuildingUpgrade("Baker's Guild", "+10% production when you have 4 or more Bakery.", 300),
		};

		ChangeWorkMode(_workMode); // Need this to setup resource target etc.
	}

	std::vector<BonusPair> GetBonuses() final {
		std::vector<BonusPair> bonuses = IndustrialBuilding::GetBonuses();

		if (IsUpgraded(0)) {
			bonuses.push_back({ "Improved Oven", 10 });
		}

		if (IsUpgraded(1)) {
			if (_simulation->buildingCount(_playerId, CardEnum::Bakery) >= 4) {
				bonuses.push_back({ "Baker's Guild", 10 });
			}
		}

		if (workMode().name == "Wood-fired") {
			bonuses.push_back({ "Wood-fired", 20 });
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
			BuildingUpgrade("Rigorous Training", "+50% productivity", 1000),
			BuildingUpgrade("Specialized Tools", "+50% productivity", 1000),
			BuildingUpgrade("Jeweler's Guild", "+30% production when you have 4 or more Jeweler.", 500),
		};
	}

	std::vector<BonusPair> GetBonuses() final
	{
		std::vector<BonusPair> bonuses = IndustrialBuilding::GetBonuses();
		
		if (IsUpgraded(0)) {
			bonuses.push_back({ "Rigorous Training", 50 });
		}
		if (IsUpgraded(1)) {
			bonuses.push_back({ "Specialized Tools", 50 });
		}
		if (IsUpgraded(2)) {
			if (_simulation->buildingCount(_playerId, CardEnum::Jeweler) >= 4) {
				bonuses.push_back({ "Jeweler's Guild", 30 });
			}
		}
		
		return bonuses;
	}
};


class Brickworks final : public IndustrialBuilding
{
public:
	void FinishConstruction() final
	{
		Building::FinishConstruction();

		_upgrades = {
			BuildingUpgrade("Specialized Tools", "+30% productivity", 500),
			BuildingUpgrade("Brickworks Town", "+30% production when you have 4 or more Brickworks.", 500),
		};
	}

	std::vector<BonusPair> GetBonuses() final {
		std::vector<BonusPair> bonuses = IndustrialBuilding::GetBonuses();

		if (IsUpgraded(0)) {
			bonuses.push_back({ "Specialized Tools", 50 });
		}
		if (IsUpgraded(1)) {
			if (_simulation->buildingCount(_playerId, CardEnum::Brickworks) >= 4) {
				bonuses.push_back({ "Brickworks Town", 30 });
			}
		}

		return bonuses;
	}
};

class CandleMaker final : public IndustrialBuilding
{
public:
	void FinishConstruction() final
	{
		Building::FinishConstruction();

		_upgrades = {
			BuildingUpgrade("Specialized Tools", "+30% productivity", 500),
			BuildingUpgrade("Candle Maker's Guild", "+30% production when you have 4 or more Candle Maker.", 500),
		};
	}

	std::vector<BonusPair> GetBonuses() final {
		std::vector<BonusPair> bonuses = IndustrialBuilding::GetBonuses();

		if (IsUpgraded(0)) {
			bonuses.push_back({ "Specialized Tools", 50 });
		}
		if (IsUpgraded(1)) {
			if (_simulation->buildingCount(_playerId, CardEnum::CandleMaker) >= 4) {
				bonuses.push_back({ "Candle Maker's Guild", 30 });
			}
		}

		return bonuses;
	}
};

class CottonMill final : public IndustrialBuilding
{
public:
	void FinishConstruction() final
	{
		Building::FinishConstruction();

		_upgrades = {
			BuildingUpgrade("Advanced Machinery", "+200% productivity", 5000),
			BuildingUpgrade("Cotton Mill Town", "+50% production when you have 4 or more Cotton Mill.", 800),
		};
	}

	std::vector<BonusPair> GetBonuses() final {
		std::vector<BonusPair> bonuses = IndustrialBuilding::GetBonuses();

		if (IsUpgraded(0)) {
			bonuses.push_back({ "Advanced Machinery", 200 });
		}
		if (IsUpgraded(1)) {
			if (_simulation->buildingCount(_playerId, CardEnum::CottonMill) >= 4) {
				bonuses.push_back({ "Cotton Mill Town", 50 });
			}
		}

		return bonuses;
	}
};

class PrintingPress final : public IndustrialBuilding
{
public:
	void FinishConstruction() final
	{
		Building::FinishConstruction();

		_upgrades = {
			BuildingUpgrade("Advanced Machinery", "+300% productivity", 7000),
			BuildingUpgrade("Printing Press Town", "+50% production when you have 4 or more Printing Press.", 800),
		};
	}

	std::vector<BonusPair> GetBonuses() final {
		std::vector<BonusPair> bonuses = IndustrialBuilding::GetBonuses();

		if (IsUpgraded(0)) {
			bonuses.push_back({ "Advanced Machinery", 200 });
		}
		if (IsUpgraded(1)) {
			if (_simulation->buildingCount(_playerId, CardEnum::PrintingPress) >= 4) {
				bonuses.push_back({ "Printing Press Town", 50 });
			}
		}

		return bonuses;
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
			BuildingUpgrade("Improved kiln", "+30% production.", 200),
			BuildingUpgrade("More workers", "+1 workers.", 100),
			BuildingUpgrade("Potter Town", "+30% production when you have 4 or more potters.", 300),
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
		if (IsUpgraded(0)) {
			bonuses.push_back({ "Improved kiln", 30 });
		}
		if (IsUpgraded(2)) {
			if (_simulation->buildingCount(_playerId, CardEnum::Potter) >= 4) {
				bonuses.push_back({ "Potter Town", 30 });
			}
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
			BuildingUpgrade("Furniture Town", "+30% production when you have 4 or more furniture workshops.", 300),
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
		if (IsUpgraded(2)) {
			if (_simulation->buildingCount(_playerId, CardEnum::FurnitureWorkshop) >= 4) {
				bonuses.push_back({ "Furniture Town", 30 });
			}
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
			BuildingUpgrade("Juicier bait", "Juicier bait that attracts more fish. +25% production when fishing.", 350),
			BuildingUpgrade("More Workers", "+1 worker slots", 100),
			//BuildingUpgrade("Whaling", "Catch whale from deep sea instead.\n  Produces whale meat.\n  +2 worker slots.\n  No effect nearby fish population", 120)
		};
		// TODO: urchin harvester (luxury?)
		// There are about 950 species of sea urchins that inhabit a wide range of depth zones in all climates across the world�s oceans. About 18 of them are edible.

		PUN_LOG("FinishContruction Fisher %d", buildingId());

		_simulation->TryAddQuest(_playerId, std::make_shared<CooperativeFishingQuest>());
	}

	void OnDeinit() final {
		ChangeFisherTilesInRadius(-1);
	}

	static const int32 Radius = 8;

	//ResourceEnum product() final {
	//	return IsUpgraded(1) ? ResourceEnum::WhaleMeat : ResourceEnum::Fish;
	//}
	ResourceEnum product() final {
		return ResourceEnum::Fish;
	}

	void OnUpgradeBuilding(int upgradeIndex) final {
		//if (upgradeIndex == 1) {
		//	_maxOccupants = 4;
		//	_allowedOccupants = _maxOccupants;
		//	ChangeFisherTilesInRadius(-1);
		//}
		if (upgradeIndex == 1) {
			SetJobBuilding(3);
		}
	}

	void ChangeFisherTilesInRadius(int32 valueChange);
	static int32 FisherAreaEfficiency(WorldTile2 centerTile, bool alreadyPlaced, WorldTile2 extraFisherTile, IGameSimulationCore* simulation); // TODO: extraFisher tile for showing all fisher's effiency during placement...

	std::vector<BonusPair> GetBonuses() final {
		std::vector<BonusPair> bonuses = Building::GetBonuses();
		int32 cardCount = _simulation->TownhallCardCount(_playerId, CardEnum::CooperativeFishing);
		if (cardCount > 0) {
			bonuses.push_back({ "Cooperative fishing", cardCount * 10 });
		}
		if (IsUpgraded(0)) {
			bonuses.push_back({ "Juicier bait", 25 });
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
	
	void FinishConstruction() final {
		Building::FinishConstruction();

		//// Make the area on which this was built walkable.
		//_area.ExecuteOnArea_WorldTile2([&](WorldTile2 tile) {
		//	_simulation->SetWalkable(_centerTile, true);
		//});
	}

	void OnDeinit() final
	{
		Building::OnDeinit();
		
		_area.ExecuteOnArea_WorldTile2([&](WorldTile2 tile) {
			_simulation->SetWalkable(tile, false);
		});
	}

	void SetAreaWalkable() override
	{
		// Make the area on which this was built walkable.
		_area.ExecuteOnArea_WorldTile2([&](WorldTile2 tile) {
			_simulation->SetWalkable(tile, true);
		});

		// clear the tile on each side of the bridge, in case the tree is blocking
		// TODO: this is not working
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


class Barrack final : public Building
{
public:
	void FinishConstruction() final {
		Building::FinishConstruction();
	}

	const ArmyInfo& armyInfo() { return GetArmyInfo(_armyEnum); }

	void QueueTrainUnit()
	{
		resourceSystem().ChangeMoney(-armyInfo().moneyCost);
		for (ResourcePair pair : armyInfo().resourceCost) {
			resourceSystem().RemoveResourceGlobal(pair.resourceEnum, pair.count);
		}
		
		if (_queueCount < maxQueueSize()) {
			_queueCount++;
		}
		TryStartTraining();
	}

	// Done Training
	void ScheduleTick() override;

	float trainingPercent() {
		if (_trainingStartTick == -1) {
			return 0;
		}
		return (Time::Ticks() - _trainingStartTick) * 100.0f / armyInfo().timeCostTicks();
	}

	static int32 maxQueueSize() { return 10; }
	int32 queueCount() { return _queueCount; }

	/*
	 * Build/Unit cost scaling
	 * - Clubman 1
	 * - Swordman 3
	 * - Archer 2
	 *
	 */

	void SetArmyEnum(ArmyEnum armyEnum) {
		_armyEnum = armyEnum;
	}

	void TryCancelTrainingQueue()
	{
		if (_queueCount > 0) {
			if (_queueCount == 1) {
				CancelCurrentTraining();
			}
			
			_queueCount--;
			AddBackTrainingResource();
		}
	}

	void Serialize(FArchive& Ar) override
	{
		Building::Serialize(Ar);
		Ar << _armyEnum;
		Ar << _queueCount;
		Ar << _trainingStartTick;
	}

private:
	void TryStartTraining()
	{
		if (_queueCount > 0 && _trainingStartTick == -1) {
			_trainingStartTick = Time::Ticks();

			int32 timeCost = armyInfo().timeCostTicks();
			if (SimSettings::IsOn("CheatFastBuild")) {
				timeCost /= 60;
			}
			
			_simulation->ScheduleTickBuilding(buildingId(), _trainingStartTick + timeCost);
		}
	}

	void CancelCurrentTraining()
	{
		if (_trainingStartTick != -1) {
			_trainingStartTick = -1;
			_simulation->RemoveScheduleTickBuilding(buildingId());
		}
	}

	void RemoveTrainingResource()
	{
		resourceSystem().ChangeMoney(-armyInfo().moneyCost);
		for (ResourcePair pair : armyInfo().resourceCost) {
			resourceSystem().RemoveResourceGlobal(pair.resourceEnum, pair.count);
		}
	}
	void AddBackTrainingResource()
	{
		resourceSystem().ChangeMoney(armyInfo().moneyCost);
		for (ResourcePair pair : armyInfo().resourceCost) {
			resourceSystem().AddResourceGlobal(pair.resourceEnum, pair.count, *_simulation);
		}
	}
	
private:
	ArmyEnum _armyEnum = ArmyEnum::Clubman;
	int32 _queueCount = 0;
	int32 _trainingStartTick = -1;
};



//! Other workplace


class LaborerGuild final : public Building
{
public:
};