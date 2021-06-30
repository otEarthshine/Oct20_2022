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
	virtual ResourceEnum product() final { return ResourceEnum::Orange; }
	
	virtual void OnInit() override;
	virtual void FinishConstruction() override;
	virtual std::vector<BonusPair> GetBonuses() override;

	static const int Radius = 24;
};

class HuntingLodge final : public Building
{
public:
	virtual ResourceEnum product() final { return ResourceEnum::Pork; }

	virtual void OnInit() override;
	virtual void FinishConstruction() override;
	virtual std::vector<BonusPair> GetBonuses() override;

	static const int Radius = 48;
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

	virtual int32 inputPerBatch(ResourceEnum resourceEnum) final {
		return Building::inputPerBatch(resourceEnum) * (_simulation->IsResearched(_playerId, TechEnum::MushroomSubstrateSterilization) ? 4 : 8) / 10;
	}
};

class ShroomFarm final : public Building
{
public:
	void FinishConstruction() final;
	std::vector<BonusPair> GetBonuses() override;

	virtual int32 inputPerBatch(ResourceEnum resourceEnum) final {
		return Building::inputPerBatch(resourceEnum) * (_simulation->IsResearched(_playerId, TechEnum::MushroomSubstrateSterilization) ? 4 : 8) / 10;
	}
};


class Beekeeper final : public Building
{
public:
	void FinishConstruction() final;
	std::vector<BonusPair> GetBonuses() override;
	
	static int32 BeekeeperBaseEfficiency(int32 townId, WorldTile2 centerTileIn, IGameSimulationCore* simulation);

	int32 efficiencyBeforeBonus() override {
		return BeekeeperBaseEfficiency(_townId, centerTile(), _simulation);
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

	int32 adjacentEfficiency() override {
		return adjacentBonusCount() * 10;
	}
	int32 maxAdjacencyCount() override { return 2; }
	bool HasAdjacencyBonus() override {
		return _simulation->IsResearched(_playerId, TechEnum::AgriculturalRevolution);
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
				!_simulation->IsOutputTargetReached(_townId, product())) 
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
		if (_simulation->IsOutputTargetReached(_townId, product())) {
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

	int32 GetBaseJobHappiness() override {
		return 50;
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
	std::vector<BonusPair> GetBonuses() final;
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
	virtual void FinishConstruction() final;

	virtual int32 inputPerBatch(ResourceEnum resourceEnum) final {
		return Building::inputPerBatch(resourceEnum) * (IsUpgraded_InitialIndex(0) ? 5 : 10) / 10;
	}
};

class Smelter : public IndustrialBuilding
{
public:
	virtual void FinishConstruction() final;
	virtual std::vector<BonusPair> GetBonuses() override;

	virtual int32 inputPerBatch(ResourceEnum resourceEnum) override {
		return Building::inputPerBatch(resourceEnum) * (IsUpgraded_InitialIndex(1) ? 70 : 100) / 100;
	}

	virtual int32 GetBaseJobHappiness() override {
		return 60;
	}

	virtual ResourceEnum mainUpgradeResource() { return ResourceEnum::Brick; }
};

class IronSmelter : public Smelter
{
public:
	virtual std::vector<BonusPair> GetBonuses() override;
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
		return baseProfitValue() * 100 * 100 / workRevenuePerSec100_perMan_(); // first 100 for workManSecPerBatch100, second 100 to cancel out WorkRevenuePerManSec100
	}

	// Production always yield the same amount of product (2 * baseinput)
	int32 outputPerBatch() override {
		return baseOutputValue() * efficiency() / 100;
	}
	

	int32 baseInputValue() {
		return GetResourceInfo(input1()).basePrice * baseInputPerBatch(input1());
	}
	int32 baseOutputValue() {
		return baseInputValue() * 2;
	}
	int32 baseProfitValue() {
		// Without sustainability card, baseProfitValue == baseInputValue
		// With Sustainability, baseProfitValue would increase, increase the work time...
		return baseOutputValue() - GetResourceInfo(input1()).basePrice * inputPerBatch(input1()); // inputPerBatch default to 10 taking into account sustainability card...
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
//	int32 outputPerBatch() override {
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

	}

	std::vector<BonusPair> GetBonuses() override;
};


class CardMaker final : public ConsumerIndustrialBuilding
{
public:
	void OnInit() override;
	
	void FinishConstruction() final;
	std::vector<BonusPair> GetBonuses() override;

	void ResetWorkModes();

	// Same amount of work required to acquire resources
	int32 workManSecPerBatch100() final
	{
		const int32 costFactor = 70; // May 31 adjust from 200
		int32 result = baseBatchCost() * 100 * 100 / workRevenuePerSec100_perMan_() * costFactor / 100; // first 100 for workManSecPerBatch100, second 100 to cancel out WorkRevenuePerManSec100

		result = buildingInfo().resourceInfo.ApplyUpgradeAndEraProfitMultipliers(result, buildingInfo().minEra(), GetEraUpgradeCount());
		
		return result * 100 / efficiency();
	}
	

	CardEnum GetCardProduced();
	
};

class ImmigrationOffice final : public ConsumerIndustrialBuilding
{
public:
	void FinishConstruction() final;
	std::vector<BonusPair> GetBonuses() override;

	// Same amount of work required to acquire resources
	int32 workManSecPerBatch100() final
	{
		// 1 Round per human?
		return 25 * 100 * 100 / workRevenuePerSec100_perMan_(); // first 100 for workManSecPerBatch100, second 100 to cancel out WorkRevenuePerManSec100
	}

	virtual int32 inputPerBatch(ResourceEnum resourceEnum) override { return 0; }
};

class StoneToolsShop : public IndustrialBuilding
{
public:
	std::vector<BonusPair> GetBonuses() override;
};
class Blacksmith : public IndustrialBuilding
{
public:
	void FinishConstruction() final;
	std::vector<BonusPair> GetBonuses() override;

};
class Herbalist : public IndustrialBuilding
{
public:
};
class MedicineMaker : public IndustrialBuilding
{
public:
	virtual void FinishConstruction() final;
	virtual std::vector<BonusPair> GetBonuses() override;

};

class CharcoalBurner final : public IndustrialBuilding
{
public:
	virtual int32 inputPerBatch(ResourceEnum resourceEnum) override { return Building::inputPerBatch(resourceEnum) * (IsUpgraded_InitialIndex(0) ? 7 : 10) / 10; }
	
	virtual void FinishConstruction() final;
	virtual std::vector<BonusPair> GetBonuses() override;

	virtual int32 GetBaseJobHappiness() override { return 60; }
};

class Chocolatier : public IndustrialBuilding
{
public:
	void FinishConstruction() override;

	virtual int32 inputPerBatch(ResourceEnum resourceEnum) override {
		return Building::inputPerBatch(resourceEnum) * (IsUpgraded_InitialIndex(0) ? 50 : 100) / 100;
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

	void ResetWorkModes();
};

class BeerBrewery : public IndustrialBuilding
{
public:
	virtual int32 inputPerBatch(ResourceEnum resourceEnum) override {
		return Building::inputPerBatch(resourceEnum) * (IsUpgraded_InitialIndex(0) ? 70 : 100) / 100;
	}
	
	virtual void OnInit() override;
	
	virtual void FinishConstruction() override;
	virtual std::vector<BonusPair> GetBonuses() override;
};

class BeerBreweryFamous final : public BeerBrewery
{
public:
	std::vector<BonusPair> GetBonuses() final;
};

class VodkaDistillery : public IndustrialBuilding
{
public:
	virtual int32 inputPerBatch(ResourceEnum resourceEnum) override {
		return Building::inputPerBatch(resourceEnum) * (IsUpgraded_InitialIndex(0) ? 70 : 100) / 100;
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
	static int32 WindmillBaseEfficiency(int32 townId, WorldTile2 centerTileIn, IGameSimulationCore* simulation)
	{
		const std::vector<int32>& windmills = simulation->buildingIds(townId, CardEnum::Windmill);

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
		return WindmillBaseEfficiency(_townId, centerTile(), _simulation);
	}

	static const int32 Radius = 24;
};
class Bakery final : public IndustrialBuilding
{
public:
	void OnInit() override;
	void FinishConstruction() final;
	std::vector<BonusPair> GetBonuses() final;

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

	virtual int32 inputPerBatch(ResourceEnum resourceEnum) override {
		return Building::inputPerBatch(resourceEnum) * (IsUpgraded_InitialIndex(1) ? 70 : 100) / 100;
	}
};

class SandMine final : public Building
{
public:
	void FinishConstruction() override;
};

class GlassSmelter final : public Smelter
{
public:

};
class Glassworks final : public Building
{
public:
	void FinishConstruction() override;
};

class ConcreteFactory final : public Building
{
public:
	void FinishConstruction() override;
};

class PowerPlant : public Building
{
public:
	void FinishConstruction() override;

	ResourceEnum fuelEnum() { return GetPowerPlantInfo(_buildingEnum).resourceEnum; }

	void ConsumeFuel1Sec(int32 actualProduction_kW)
	{
		ResourceEnum resourceEnum = fuelEnum();
		if (resourceCount(resourceEnum) <= 0) {
			return;
		}
		
		// 1 kW = 1 coal burn per round
		bool shouldBurnFuel = (GameRand::Rand() % Time::SecondsPerRound) < actualProduction_kW;

		// Half chance for oil
		if (shouldBurnFuel && resourceEnum == ResourceEnum::Oil) {
			shouldBurnFuel = GameRand::Rand() % 2;
		}

		if (shouldBurnFuel) {
			if (resourceCount(resourceEnum) > 0) {
				RemoveResource(resourceEnum, 1);
				AddConsumption1Stat(ResourcePair(resourceEnum, 1));
			} else {
				UE_DEBUG_BREAK();
			}
		}
	}

	int32 ElectricityConsumption()
	{
		if (!isConstructed()) {
			return 0;
		}
		auto& townManage = townManager();
		return ElectricityProductionCapacity() * townManage.electricityConsumption() / std::max(1, townManage.electricityProductionCapacity());
	}

	int32 ElectricityProductionCapacity() {
		if (!isConstructed()) {
			return 0;
		}
		return resourceCount(fuelEnum()) > 0 ? GetPowerPlantInfo(_buildingEnum).baseCapacity : 0;
	}

	bool shouldDisplayParticles() override {
		return isConstructed() && resourceCount(fuelEnum()) > 0;
	}
	
};
class CoalPowerPlant final : public PowerPlant
{
public:


	
};
class OilPowerPlant final : public PowerPlant
{
public:



};

class IndustrialIronSmelter final : public Smelter
{
public:
	virtual ResourceEnum mainUpgradeResource() override { return ResourceEnum::Concrete; }
	
};
class Steelworks final : public Building
{
public:
	void FinishConstruction() override;
};

class OilRig final : public Mine
{
public:
	void FinishConstruction() override;
	std::vector<BonusPair> GetBonuses() final;
};


class PaperMill final : public Building
{
public:
	void FinishConstruction() override;

	virtual int32 inputPerBatch(ResourceEnum resourceEnum) final {
		return Building::inputPerBatch(resourceEnum) * (IsUpgraded_InitialIndex(0) ? 5 : 10) / 10;
	}
};

class ClockMakers final : public Building
{
public:
	void FinishConstruction() override;
};


class WorldWonder : public Building
{
public:
	void FinishConstruction() override {
		Building::FinishConstruction();

		builtNumber = _simulation->playerBuildingFinishedCount(_playerId, _buildingEnum);

		bool allWondersBuilt = true;
		for (CardEnum buildingEnum : WorldWonders) {
			int32 wonderCount = 0;
			_simulation->ExecuteOnPlayersAndAI([&](int32 playerId) {
				wonderCount += _simulation->playerBuildingFinishedCount(playerId, buildingEnum);
			});
			if (wonderCount == 0) {
				allWondersBuilt = false;
				break;
			}
		}
		
		if (allWondersBuilt) {
			_simulation->ExecuteScoreVictory();
		}
	}
	
	int32 WonderScore()
	{
		const BldInfo& bldInfo = buildingInfo();
		int32 moneyValue = bldInfo.baseCardPrice + bldInfo.constructionCostAsMoney();
		int32 minEra = bldInfo.minEra();

		int32 multiplier = 15; // May 30 Adjust
		for (int32 i = minEra; i < 4; i++) {
			multiplier *= 2;
		}
		int32 wonderScore = multiplier * moneyValue / 10000; // 10000 Money = 1 Score

		if (_simulation->HasGlobalBonus(_playerId, CardEnum::WondersScoreMultiplier)) {
			wonderScore *= 2;
		}

		if (builtNumber > 1) {
			wonderScore /= 2;
		}

		return wonderScore;
	}

	void Serialize(FArchive& Ar) override {
		Building::Serialize(Ar);
		Ar << builtNumber;
	}

	int32 builtNumber;
};

class Cathedral final : public WorldWonder
{
public:
};
class Castle final : public WorldWonder
{
public:
};
class GrandMuseum final : public WorldWonder
{
public:
};
class ExhibitionHall final : public WorldWonder
{
public:
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
	//	return IsUpgraded_InitialIndex(1) ? ResourceEnum::WhaleMeat : ResourceEnum::Fish;
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
	void OnDeinit() override;

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
		return (500 - baseBatchCost()) * 100 * 100 / workRevenuePerSec100_perMan_(); // first 100 for workManSecPerBatch100, second 100 to cancel out WorkRevenuePerManSec100
	}

	// TODO: remove these...
	virtual void DoConsumerWork(int32_t workManSec100, int32_t resourceConsumed) = 0;

	int32 resourceConsumptionAmount100(int32 workManSec100) { return inputPerBatch(input1()) * workManSec100 * 100 / workManSecPerBatch100(); }
};

class Library final : public Building
{
public:
	static const int32_t Radius = 30;
	//static const int32_t SciencePerHouse = 2;

	static const int32 MinHouseLvl = 1;

	void FinishConstruction() final {
		Building::FinishConstruction();
		_simulation->RecalculateTaxDelayedTown(_townId);
	}
};
class School final : public Building
{
public:
	static const int32_t Radius = 30;
	//static const int32_t SciencePerHouse = 3;

	static const int32 MinHouseLvl = 1;

	void FinishConstruction() final {
		Building::FinishConstruction();
		_simulation->RecalculateTaxDelayedTown(_townId);
	}
};

class ProfitBuilding : public Building
{
public:
	void FinishConstruction() override {
		Building::FinishConstruction();
		lastRoundProfit = 0;
	}

	virtual IncomeEnum incomeEnum() { return IncomeEnum::BankProfit; }

	virtual void CalculateRoundProfit() {}

	void Serialize(FArchive& Ar) override {
		Building::Serialize(Ar);
		Ar << lastRoundProfit;
	}

	int32 lastRoundProfit;
};

class Bank final : public ProfitBuilding
{
public:
	static const int32 Radius = 30;
	static const int32 ProfitPerHouse = 10;

	static const int32 MinHouseLvl = 5;

	//void CalculateRoundProfit() override;
};

class Archives final : public ProfitBuilding
{
public:
	int32 maxCardSlots() override { return 6; }

	static const int32 CardProfitPercentPerYear = 12; // 12% per year

	IncomeEnum incomeEnum() override { return IncomeEnum::ArchivesProfit; }

	void CalculateRoundProfit() override;
};



//static const std::vector<int32> BaseServiceQuality
//{
//	50,
//	70,
//};

class FunBuilding : public Building
{
public:
	int32 serviceQuality() {
		std::vector<BonusPair> bonuses = GetBonuses();
		int32 quality = ServiceQuality(buildingEnum(), GetAppealPercent());
		for (BonusPair bonus : bonuses) {
			quality += bonus.value;
		}
		return quality;
	}
	static int32 ServiceQuality(CardEnum buildingEnum, int32 appealPercent) {
		return BuildingEnumToBaseServiceQuality(buildingEnum) + std::max(0, appealPercent / 4);
	}

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

	bool shouldDisplayParticles() override { return true; }
};


class Tavern final : public FunBuilding
{
public:
	static const int32 Radius = 30;

	bool shouldDisplayParticles() override { return true; }
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

class ResourceOutpost final : public ProvinceBuilding
{
public:
	static int32 GetColonyIncomeValue() { return GetBuildingInfo(CardEnum::ResourceOutpost).baseCardPrice / 16; } // 16 rounds or 2 year to recoup the 
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

		AddUpgrades({
			MakeWorkerSlotUpgrade(50),
		});
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

class HaulingServices final : public Building
{
public:
	void FinishConstruction() override {
		Building::FinishConstruction();
		AddUpgrades({
			MakeWorkerSlotUpgrade(50, 2),
		});
	}

	static const int32 Radius = 32;
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

		if (IsUpgraded_InitialIndex(0) && adjacentCount(CardEnum::Windmill) > 0) {
			return baseUpkeep / 2;
		}
		return baseUpkeep;
	}

	static const int32 Radius = 12;
};
