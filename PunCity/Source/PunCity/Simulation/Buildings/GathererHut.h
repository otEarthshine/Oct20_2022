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
const FText MeticulousWorkModeText = NSLOCTEXT("GathererHut", "Meticulous_WorkMode", "Meticulous");
const FText PoisonArrowWorkModeText = NSLOCTEXT("GathererHut", "PoisonArrow_WorkMode", "Poison Arrows");

class GathererHut final : public Building
{
public:
	ResourceEnum product() final { return ResourceEnum::Orange; }
	
	void OnInit() override;
	void FinishConstruction() override;
	std::vector<BonusPair> GetBonuses() override;

	static const int Radius = 24;
};

class HuntingLodge final : public Building
{
public:
	ResourceEnum product() final { return ResourceEnum::Pork; }

	void OnInit() override;
	void FinishConstruction() final;
	std::vector<BonusPair> GetBonuses() override;

	static const int Radius = 32;
};


const FText CutAndPlantText = NSLOCTEXT("GathererHut", "Cut and Plant", "Cut and Plant");
const FText PrioritizePlantText = NSLOCTEXT("GathererHut", "Prioritize Planting", "Prioritize Planting");
const FText PrioritizeCutText = NSLOCTEXT("GathererHut", "Prioritize Cutting", "Prioritize Cutting");

class Forester final : public Building
{
public:
	ResourceEnum product() override { return ResourceEnum::Wood; }
	
	void OnInit() override;
	void FinishConstruction() override;
	std::vector<BonusPair> GetBonuses() override;

	void Serialize(FArchive& Ar) override {
		Building::Serialize(Ar);
		Ar << cuttingEnum;
		Ar << plantingEnum;
	}

	CutTreeEnum cuttingEnum = CutTreeEnum::Any;
	CutTreeEnum plantingEnum = CutTreeEnum::FruitTreeOnly;
	
	static const int32 Radius = 30;
};

/*
 * Production buildings
 */

class MushroomFarm final : public Building
{
public:
	void FinishConstruction() final;
	std::vector<BonusPair> GetBonuses() override;

	int32 baseInputPerBatch() final;
};

class ShroomFarm final : public Building
{
public:
	void FinishConstruction() final;
	std::vector<BonusPair> GetBonuses() override;

	int32 baseInputPerBatch() final {
		return _simulation->unlockSystem(_playerId)->IsResearched(TechEnum::MushroomSubstrateSterilization) ? 4 : 8;
	}
};


class Beekeeper final : public Building
{
public:
	void FinishConstruction() final;
	std::vector<BonusPair> GetBonuses() override;
	
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
	int32 GetStagePercent() {
		return CppUtils::Sum(_isTileWorked) * 100 / _isTileWorked.size();
	}
	
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
	
	FText farmStageName();

	bool ShouldAddWorker_ConstructedNonPriority() override {
		return !IsStage(FarmStage::Dormant);
	}

	void OnTick1Sec() final
	{
		if (IsStage(FarmStage::Dormant))
		{
			// Farm can return to seeding spring to mid summer (round 1 summer)
			if (Time::IsValidFarmBeginTime() &&
				!_simulation->IsOutputTargetReached(_playerId, product())) 
			{
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

				//ResetStageTo(FarmStage::Dormant);
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

	void RefreshFertility() {
		_fertility = GetAverageFertility(area(), _simulation);
	}

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
	void OnInit() override;
	void FinishConstruction() override;
	
	std::vector<BonusPair> GetBonuses() override;

	bool ShouldAddWorker_ConstructedNonPriority() override {
		if (_simulation->IsOutputTargetReached(_playerId, product())) {
			return false;
		}
		return oreLeft() > 0;
	}

	void OnProduce(int32 productionAmount) override;

	bool RefreshHoverWarning() override
	{
		if (Building::RefreshHoverWarning()) {
			return true;
		}
		if (oreLeft() <= 0) {
			hoverWarning = HoverWarning::Depleted;
			return true;
		}
		
		hoverWarning = HoverWarning::None;
		return false;
	}
};

class Quarry final : public Mine
{
public:
	std::vector<BonusPair> GetBonuses() final;
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
	std::vector<BonusPair> GetBonuses() final;
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
	std::vector<BonusPair> GetBonuses() override;
};

class PaperMaker final : public IndustrialBuilding
{
public:
	void FinishConstruction() final;

	int32 baseInputPerBatch() final {
		return IsUpgraded(0) ? 5 : 10;
	}
};

class Smelter : public IndustrialBuilding
{
public:
	void FinishConstruction() final;
	std::vector<BonusPair> GetBonuses() override;

	int32 baseInputPerBatch() override {
		return Building::baseInputPerBatch() * (IsUpgraded(1) ? 70 : 100) / 100;
	}
};

class IronSmelter : public Smelter
{
public:
	std::vector<BonusPair> GetBonuses() override;
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
	void FinishConstruction() final;
	std::vector<BonusPair> GetBonuses() override;

};

class InventorsWorkshop : public ConsumerIndustrialBuilding
{
public:
	void FinishConstruction() final;
	std::vector<BonusPair> GetBonuses() override;
};


class RegionShrine final : public Building
{

};
//class RegionShrine final : public ConsumerIndustrialBuilding
//{
//public:
//	void FinishConstruction() final
//	{
//		ConsumerIndustrialBuilding::FinishConstruction();
//
//		_upgrades = {
//
//		};
//	}
//
//	void PlayerTookOver(int32 playerId)
//	{
//		// Reset old workers
//		if (_playerId != -1) {
//			ResetWorkReservers();
//			_simulation->RemoveJobsFrom(buildingId(), false);
//			_simulation->PlayerRemoveJobBuilding(_playerId, *this, _isConstructed);
//		}
//
//		_playerId = playerId;
//		_simulation->PlayerAddJobBuilding(_playerId, *this, false);
//		SetJobBuilding(3);
//
//		OnInit();
//		InitStatistics();
//	}
//
//	std::vector<BonusPair> GetBonuses() override {
//		std::vector<BonusPair> bonuses = ConsumerIndustrialBuilding::GetBonuses();
//
//		return bonuses;
//	}
//
//	int32 productPerBatch() override {
//		return 5 * efficiency() / 100;
//	}
//
//	// Same amount of work required to acquire resources
//	int32 workManSecPerBatch100() final
//	{
//		const int32 value = 250; // 500 / 2
//		return value * 100 * 100 / WorkRevenue100PerSec_perMan_Base; // first 100 for workManSecPerBatch100, second 100 to cancel out WorkRevenuePerManSec100
//	}
//
//	int32 baseInputPerBatch() override { return 0; }
//};

class Barrack final : public ConsumerIndustrialBuilding
{
public:
	void FinishConstruction() final
	{
		ConsumerIndustrialBuilding::FinishConstruction();

		_upgrades = {
		};
	}

	std::vector<BonusPair> GetBonuses() override;
};


class CardMaker final : public ConsumerIndustrialBuilding
{
public:
	void OnInit() override;
	
	void FinishConstruction() final;
	std::vector<BonusPair> GetBonuses() override;

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
	void FinishConstruction() final;
	std::vector<BonusPair> GetBonuses() override;

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
	void FinishConstruction() final;

};
class Herbalist : public IndustrialBuilding
{
public:
};
class MedicineMaker : public IndustrialBuilding
{
public:
	void FinishConstruction() final;
	std::vector<BonusPair> GetBonuses() override;

	int32 baseInputPerBatch() override {
		return 5;
	}
};

class CharcoalMaker final : public IndustrialBuilding
{
public:
	int32 baseInputPerBatch() override { return IsUpgraded(0) ? 7 : 10; }
	
	void FinishConstruction() final;
	std::vector<BonusPair> GetBonuses() override;
};

class Chocolatier : public IndustrialBuilding
{
public:
	void FinishConstruction() override;

	std::vector<BonusPair> GetBonuses() override {
		std::vector<BonusPair> bonuses = IndustrialBuilding::GetBonuses();


		return bonuses;
	}

	int32 baseUpkeep() override {
		if (IsUpgraded(3)) {
			return Building::baseUpkeep() / 2;
		}
		return Building::baseUpkeep();
	}

	int32 baseInputPerBatch() override {
		return IsUpgraded(0) ? 5 : 10;
	}
};

class Winery final: public IndustrialBuilding
{
public:
	void FinishConstruction() final;
	std::vector<BonusPair> GetBonuses() final;
};

class CoffeeRoaster final : public IndustrialBuilding
{
public:
	void FinishConstruction() final;
};

class Tailor final : public IndustrialBuilding
{
public:
	void OnInit() override;
	void FinishConstruction() final;
};

class BeerBrewery : public IndustrialBuilding
{
public:
	int32 baseInputPerBatch() override {
		return _workMode.inputPerBatch * (IsUpgraded(0) ? 70 : 100) / 100;
	}
	
	void OnInit() override;
	
	void FinishConstruction() override;
	std::vector<BonusPair> GetBonuses() override;
};

class BeerBreweryFamous final : public BeerBrewery
{
public:
	std::vector<BonusPair> GetBonuses() final;
};

class VodkaDistillery : public IndustrialBuilding
{
public:
	int32 baseInputPerBatch() override {
		return Building::baseInputPerBatch() * (IsUpgraded(0) ? 70 : 100) / 100;
	}
	
	void FinishConstruction() override;
	std::vector<BonusPair> GetBonuses() override;
};


class Windmill final : public IndustrialBuilding
{
public:
	void FinishConstruction() final;
	std::vector<BonusPair> GetBonuses() final;

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
	void OnInit() override;
	void FinishConstruction() final;
	std::vector<BonusPair> GetBonuses() final;

	//int32 baseInputPerBatch() {
	//	return 5;
	//}
};
class Jeweler final : public IndustrialBuilding
{
public:
	void FinishConstruction() final;
};


class Brickworks final : public IndustrialBuilding
{
public:
	void FinishConstruction() final;
};

class CandleMaker final : public IndustrialBuilding
{
public:
	void FinishConstruction() final;
};

class CottonMill final : public IndustrialBuilding
{
public:
	void OnInit() override;
	void FinishConstruction() final;
};

// TODO: include this??
class GarmentFactory final : public IndustrialBuilding
{
public:
	void OnInit() override
	{
		//SetupWorkMode({
		//	{"Fashionable Clothes", ResourceEnum::DyedCottonFabric, ResourceEnum::None, 10, ResourceEnum::LuxuriousClothes },
		//	{"Cotton Clothes", ResourceEnum::CottonFabric, ResourceEnum::None, 10},
		//});
	}

	void FinishConstruction() final
	{
		Building::FinishConstruction();

		AddResourceHolder(ResourceEnum::CottonFabric, ResourceHolderType::Requester, 0);
		AddResourceHolder(ResourceEnum::Cloth, ResourceHolderType::Provider, 0);

		AddResourceHolder(ResourceEnum::DyedCottonFabric, ResourceHolderType::Requester, 0);
		AddResourceHolder(ResourceEnum::LuxuriousClothes, ResourceHolderType::Provider, 0);

		ChangeWorkMode(_workMode);

		//_upgrades = {
		//	MakeProductionUpgrade("Advanced Machinery", ResourceEnum::Iron, 500, 200),
		//	MakeComboUpgrade("Garment Factory Town", ResourceEnum::Iron, 80, 50),
		//};
	}

};

class PrintingPress final : public IndustrialBuilding
{
public:
	void FinishConstruction() final;
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
	
	void FinishConstruction() override;

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

};

class Potter final : public IndustrialBuilding
{
public:
	void FinishConstruction() override;
	std::vector<BonusPair> GetBonuses() override;
};

class FurnitureWorkshop final : public IndustrialBuilding
{
public:
	void FinishConstruction() final;
	std::vector<BonusPair> GetBonuses() final;

	int32 baseInputPerBatch() override {
		return Building::baseInputPerBatch() * (IsUpgraded(1) ? 70 : 100) / 100;
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

	void FinishConstruction() final;
	std::vector<BonusPair> GetBonuses() final;

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

	void ChangeFisherTilesInRadius(int32 valueChange);
	static int32 FisherAreaEfficiency(WorldTile2 centerTile, bool alreadyPlaced, WorldTile2 extraFisherTile, IGameSimulationCore* simulation); // TODO: extraFisher tile for showing all fisher's effiency during placement...
	
	
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
	void OnTick1Sec() final;
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

class Bridge : public Building
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

class Tunnel final : public Bridge
{
public:
	void FinishConstruction() override {
		Bridge::FinishConstruction();

		int32 end1 = _simulation->GetProvinceIdClean(_area.min());
		int32 end2 = _simulation->GetProvinceIdClean(_area.max());
		
		_simulation->AddTunnelProvinceConnections(end1, end2);


		// TODO: demolish should remove connection
	}
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
	static const int32 Radius = 20;
	static const int32 ProfitPerHouse = 10;

	static const int32 MinHouseLvl = 2;
	
	void FinishConstruction() final {
		Building::FinishConstruction();
		lastRoundProfit = 0;
	}

	void CalculateRoundProfit();

	void Serialize(FArchive& Ar) override {
		Building::Serialize(Ar);
		Ar << lastRoundProfit;
	}

	int32 lastRoundProfit;
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
	
	void OnTick1Sec() override {
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

	void FinishConstruction() override {
		Building::FinishConstruction();

		_upgrades = {
			MakeWorkerSlotUpgrade(50),
		};
	}

	bool needSetup() {
		for (ResourceEnum resourceEnum : resourceEnums) {
			if (resourceEnum != ResourceEnum::None) {
				return false;
			}
		}
		return true;
	}

	bool RefreshHoverWarning() override
	{
		if (Building::RefreshHoverWarning()) {
			return true;
		}

		if (needSetup()) {
			hoverWarning = HoverWarning::NeedSetup;
			return true;
		}

		if (deliveryTargetId() == -1) {
			hoverWarning = HoverWarning::NeedDeliveryTarget;
			return true;
		}
		
		Building& building = _simulation->buildingChecked(deliveryTargetId());
		if (WorldTile2::Distance(building.centerTile(), centerTile()) > 150) {
			hoverWarning = HoverWarning::DeliveryTargetTooFar;
			return true;
		}
		

		hoverWarning = HoverWarning::None;
		return true;
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

	void FinishConstruction() override;

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

	int32 baseUpkeep() override {
		int32 baseUpkeep = 80;

		if (IsUpgraded(0) && adjacentCount(CardEnum::Windmill) > 0) {
			return baseUpkeep / 2;
		}
		return baseUpkeep;
	}

	static const int32 Radius = 12;
};
