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

/*
 * Farm
 */

void Farm::OnInit()
{
	// Set initial seed
	std::vector<SeedInfo> seeds = resourceSystem().seedsPlantOwned();

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
		currentPlantEnum = GameRand::Rand() % 2 == 0 ?  TileObjEnum::WheatBush : TileObjEnum::Cabbage;
	}
}

void Farm::FinishConstruction()
{
	Building::FinishConstruction();

	std::vector<TileObjEnum> allFarmPlants = _simulation->unlockSystem(_playerId)->allFarmPlants();
	for (int i = 0; i < allFarmPlants.size(); i++) {
		AddResourceHolder(GetTileObjInfo(allFarmPlants[i]).harvestResourceEnum(), ResourceHolderType::Provider, 0);
	}

	_isTileWorked.resize(totalFarmTiles(), false);

	_fertility = GetAverageFertility(area(), _simulation);
}

bool Farm::NoFarmerOnTileId(int32_t farmTileId) {
	return !CppUtils::Contains(_reservingUnitIdToFarmTileId, [&](const pair<int32, int32>& pair) { return pair.second == farmTileId; });
}

std::vector<BonusPair> Farm::GetBonuses()
{
	std::vector<BonusPair> bonuses = Building::GetBonuses();

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

	int32 radiusBonus = GetRadiusBonus(CardEnum::Windmill, Windmill::Radius, [&](int32 bonus, Building& building) {
		return max(bonus, 10);
	});
	if (radiusBonus > 0) {
		bonuses.push_back({ "Near Windmill", radiusBonus });
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
	// TODO: solve this properly...
	if (farmStage != _farmStage) {
		return;
	}
	PUN_CHECK2(farmStage == _farmStage, debugStr());


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

//! Mushroom

int32 MushroomHut::baseInputPerBatch() {
	return _simulation->unlockSystem(_playerId)->IsResearched(TechEnum::MushroomSubstrateSterilization) ? 4 : 8;
}

//! CardMaker

CardEnum CardMaker::GetCardProduced()
{
	const std::string& name = workMode().name;

	if (name == "Productivity Book") {
		return CardEnum::ProductivityBook;
	}
	if (name == "Sustainability Book") {
		return CardEnum::SustainabilityBook;
	}
	if (name == "Frugality Book") {
		return CardEnum::FrugalityBook;
	}
	if (name == "Wild Card") {
		return CardEnum::WildCard;
	}
	if (name == "Card Removal Card") {
		return CardEnum::CardRemoval;
	}

	checkNoEntry();
	return CardEnum::None;
}


//! Fisher

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


int32 Beekeeper::BeekeeperBaseEfficiency(int32 playerId, WorldTile2 centerTileIn, IGameSimulationCore* simulation)
{
	const std::vector<int32>& buildings = simulation->buildingIds(playerId, CardEnum::Beekeeper);

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

void Mine::OnProduce(int32 productionAmount)
{
	int32 depletionMultiplier = 100;
	if (_workMode.name == "Conserve resource") {
		depletionMultiplier -= 30;
	} else if (_workMode.name == "Rapid mining") {
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

void Trap::Tick1Sec()
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

void Colony::TickRound()
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
				_simulation->uiInterface()->ShowFloatupInfo(FloatupEnum::GainResource, centerTile(), "+" + to_string(resourceCount), resourceEnum);
			}
		}
	}
}