#include "TreeSystem.h"
#include "PunCity/GameRand.h"
#include "ResourceDropSystem.h"
#include "StatSystem.h"
#include "../DisplaySystem/PunTerrainGenerator.h"
#include "GeoresourceSystem.h"
#include "ProvinceSystem.h"

#include <algorithm>
#include <chrono>

using namespace std;
using namespace std::chrono;

DECLARE_CYCLE_STAT(TEXT("PUN: FindTree"), STAT_PunFindTree, STATGROUP_Game);
DECLARE_CYCLE_STAT(TEXT("PUN: FindFruit"), STAT_PunFindFruit, STATGROUP_Game);
DECLARE_CYCLE_STAT(TEXT("PUN: FindFullBush"), STAT_PunFindFullBush, STATGROUP_Game);
DECLARE_CYCLE_STAT(TEXT("PUN: FindFullBush2"), STAT_PunFindFullBush2, STATGROUP_Game);

DECLARE_CYCLE_STAT(TEXT("PUN: Tree.Tick"), STAT_PunTreeTickTiles, STATGROUP_Game);
DECLARE_CYCLE_STAT(TEXT("PUN: Tree.TickInitial"), STAT_PunTreeTickInitial, STATGROUP_Game);
DECLARE_CYCLE_STAT(TEXT("PUN: Tree.TickAgeDeath"), STAT_PunTreeAgeDeath, STATGROUP_Game);
DECLARE_CYCLE_STAT(TEXT("PUN: Tree.TickFruit"), STAT_PunTreeFruit, STATGROUP_Game);
DECLARE_CYCLE_STAT(TEXT("PUN: Tree.TickFish"), STAT_PunTreeFish, STATGROUP_Game);

DECLARE_CYCLE_STAT(TEXT("PUN: Tree.Fallen"), STAT_PunTreeFallen, STATGROUP_Game);

#pragma runtime_checks( "sc", restore )

void TreeSystem::Init(int sizeX, int sizeY, IGameSimulationCore* simulation)
{
	_simulation = simulation;

	_sizeX = sizeX;
	_sizeXY = sizeX * sizeY;

	_treeEnum.clear();
	_tileObjAge.clear();
	_treeShade.clear();

	_treeEnum.resize(_sizeXY, TileObjEnum::None);
	_tileObjAge.resize(_sizeXY, 0);
	_treeShade.resize(_sizeXY >> 3, 0);

	//testPerlin.resize(_sizeXY);

	_regionToLocalTileIdToFallenTreeInfo.resize(GameMapConstants::TotalRegions);
	_regionToReadyFruits.resize(GameMapConstants::TotalRegions);
	_regionToReadyBushes.resize(GameMapConstants::TotalRegions);
	_regionToLastReadyBushUpdate.resize(GameMapConstants::TotalRegions, 0);
	
	_regionToGrassCount.resize(GameMapConstants::TotalRegions, 0);

	for (int i = 0; i < _regionToReadyBushes.size(); i++) {
		_regionToReadyFruits[i].Init(WorldRegion2(i));
		_regionToReadyBushes[i].Init(WorldRegion2(i));
	}

	_regionToMarkTileIds.resize(GameMapConstants::TotalRegions);
	for (int i = 0; i < GameMapConstants::TotalRegions; i++) {
		_regionToMarkTileIds[i].Init(WorldRegion2(i));

		// TODO: remove _regionToGrassCount and use 5x5 tiles check when planting grass after init... (init is just normal randomize)
		int32 maxBushesForThisRegion = CoordinateConstants::TileIdsPerRegion * GrassToBushValue / 4;
		_regionToGrassCount[i] = maxBushesForThisRegion;
	}

	_treeCount = 0;
	_stoneCount = 0;
	_grassCount = 0;

	if (!_simulation->isLoadingFromFile()) {
		PlantInitial();
	}
}

// Also used for planting stone
void TreeSystem::PlantTree(int32 x, int32 y, TileObjEnum treeEnum, bool initial)
{
	PUN_CHECK(IsTileObjEnumValid(treeEnum));
	
	int32 i = x + y * _sizeX;

	// Shade
	WorldTile2 tile(x, y);
	if (_simulation->GetFertilityPercent(tile) > 25) { //  Desert doesn't need shade (lead to weird green spots on world map)
		SetSurroundingShade(i);
	}

	_tileObjAge[i] = 0;
	_treeEnum[i] = treeEnum;

	if (IsOutcrop(treeEnum)) _stoneCount++;
	else _treeCount++;

	if (initial) {
		_simulation->SetWalkableSkipFlood(tile, false);
	} else {
		_simulation->SetWalkable(tile, false);
		_simulation->SetNeedDisplayUpdate(DisplayClusterEnum::Trees, tile.regionId(), true);
	}
}

bool TreeSystem::HasNearbyShadeObject(int x, int y)
{
	int i = x + y * _sizeX;

	int surround = i + 1;

	// Doesn't need rim check because it was done in outer scope
	auto isShadeObjectTile = [&]() {
		ResourceTileType type = tileInfo(surround).type;
		return type == ResourceTileType::Tree || type == ResourceTileType::Deposit;
	};

	if (isShadeObjectTile()) return true;
	surround = i - 1;
	if (isShadeObjectTile()) return true;
	surround = i + _sizeX;
	if (isShadeObjectTile()) return true;
	surround = i - _sizeX;
	if (isShadeObjectTile()) return true;

	surround = i + 1 + _sizeX;
	if (isShadeObjectTile()) return true;
	surround = i - 1 + _sizeX;
	if (isShadeObjectTile()) return true;
	surround = i + 1 - _sizeX;
	if (isShadeObjectTile()) return true;
	surround = i - 1 - _sizeX;
	if (isShadeObjectTile()) return true;

	return false;
}

void TreeSystem::PlantBush(int32_t id, TileObjEnum tileObjEnum, bool updateDisplay, bool randomizeAge)
{
	PUN_CHECK(IsTileObjEnumValid(tileObjEnum));
	
	_tileObjAge[id] = randomizeAge ? GetTileObjInfo(tileObjEnum).randomGrowthTicks() : 0;

	_treeEnum[id] = tileObjEnum;


	int32 grassCount = IsGrass(tileObjEnum) ? 1 : GrassToBushValue;
	_regionToGrassCount[WorldTile2(id).regionId()] -= grassCount;
	_grassCount += grassCount;
	
	if (updateDisplay) {
		_simulation->SetNeedDisplayUpdate(DisplayClusterEnum::Trees, WorldTile2(id).regionId(), true);
	}
	_simulation->statSystem(-1).AddStat(SeasonStatEnum::BushBirth);
}

void TreeSystem::ForceRemoveTileObj(WorldTile2 tile, bool animate)
{
	ForceRemoveTileReservation(tile);
	RemoveTileObj(tile, animate);
}

void TreeSystem::ForceRemoveTileReservation(WorldTile2 tile)
{
	// Not doing Unreserve since ResetUnitActions() -> PopReservation() -> treeSystem.Unreserve()
	UnitReservation reservation = _reservations.TryGet(tile.tileId());
	if (reservation.isValid()) {
		_simulation->ResetUnitActions(reservation.unitId);
	}
}

void TreeSystem::ForceRemoveTileReservationArea(TileArea area) {
	area.ExecuteOnArea_WorldTile2([&](WorldTile2 tile) {
		ForceRemoveTileReservation(tile);
	});
}

ResourcePair TreeSystem::HumanHarvest(WorldTile2 tile) {
	TileObjInfo info = tileInfo(tile.tileId());

	bool validInfo = info.type == ResourceTileType::Bush ||
						info.type == ResourceTileType::Tree ||
						info.type == ResourceTileType::Deposit;
	PUN_CHECK2(validInfo, tileObjdebugStr(tile.tileId()));
	if (!validInfo) {
		return ResourcePair(ResourceEnum::Wood, 1);
	}
	return RemoveTileObj(tile, true);
}
ResourcePair TreeSystem::UnitHarvestTreeOrDeposit(WorldTile2 tile) {
	ResourceTileType tileType = tileInfo(tile.tileId()).type;
	PUN_CHECK(tileType == ResourceTileType::Tree || tileType == ResourceTileType::Deposit);
	return RemoveTileObj(tile, true);
}
ResourcePair TreeSystem::UnitHarvestBush(WorldTile2 tile) {
	int32 id = tile.tileId();

	// TODO: probably trying to harvest dead bush?? Resolve this properly?
	if (tileInfo(id).type == ResourceTileType::None) {
		return ResourcePair::Invalid();
	}
#if TRAILER_MODE
	if (tileInfo(id).type != ResourceTileType::Bush) {
		return ResourcePair(ResourceEnum::Hay, 1);
	}
#endif
	PUN_CHECK2(tileInfo(id).type == ResourceTileType::Bush, tileObjdebugStr(id));

	_simulation->statSystem(-1).AddStat(SeasonStatEnum::BushDeathGather);
	return RemoveTileObj(tile);
}

void TreeSystem::UnitNourishBush(WorldTile2 tile) {
	int32 id = tile.tileId();

	// TODO: probably trying to nourish dead bush?? Resolved this properly?
	if (tileInfo(id).type == ResourceTileType::None) {
		return;
	}
#if TRAILER_MODE
	if (tileInfo(id).type != ResourceTileType::Bush) {
		return;
	}
#endif
	PUN_CHECK2(tileInfo(id).type == ResourceTileType::Bush, tileObjdebugStr(id));

	// Nourish
	_tileObjAge[id] += Time::TicksPerSeason / 4;

	_simulation->SetNeedDisplayUpdate(DisplayClusterEnum::Trees, tile.regionId(), true);
}
void TreeSystem::UnitNourishTree(WorldTile2 tile) {
	int32 id = tile.tileId();

	if (tileInfo(id).type != ResourceTileType::Tree) {
		return;
	}

	// Nourish
	_tileObjAge[id] += Time::TicksPerSeason;

	_simulation->SetNeedDisplayUpdate(DisplayClusterEnum::Trees, tile.regionId(), true);
}

ResourcePair TreeSystem::RemoveTileObj(WorldTile2 tile, bool animate)
{
	int32 id = tile.tileId();

	auto removeSurroundingShade = [&]()
	{
		// Ensure outer area is valid
		//  Disregard any rim (shouldn't have land anyway)
		TileArea checkAreaOuter(tile, 2);
		if (checkAreaOuter.isValid())
		{
			// Remove surrounding shades
			TileArea checkArea(tile, 1);
			for (int y = checkArea.minY; y <= checkArea.maxY; y++) {
				for (int x = checkArea.minX; x <= checkArea.maxX; x++)
				{
					if (!HasNearbyShadeObject(x, y)) {
						setTreeShade(x + y * _sizeX, false);
					}
				}
			}
		}
	};

	if (IsAliveTree(id))
	{
		// PUN_LOG("Cut Tree: id:%d, (%d,%d) Region: %d", id, tile.x, tile.y, tile.region().regionId())

		if (animate) {
			PUN_CHECK(tile.regionId() >= 0);
			auto& fellTicks = _regionToLocalTileIdToFallenTreeInfo[tile.regionId()];
			FallenTreeInfo fallenInfo;
			fallenInfo.treeEnum = _treeEnum[id];
			fallenInfo.tileObjAgeTick = _tileObjAge[id];
			fallenInfo.fellTick = Time::Ticks();
			PUN_CHECK(static_cast<int>(fallenInfo.treeEnum) < TileObjEnumSize);

			fellTicks[tile.localTileId()] = fallenInfo;

			// Play sound
			_simulation->soundInterface()->Spawn3DSound("CitizenAction", "TreeFalling", tile.worldAtom2());
		}

		RemoveFruit(tile);

		ResourcePair yield = tileInfo(id).cutDownResource(_tileObjAge[id], 0);

		_treeEnum[id] = TileObjEnum::None;
		_tileObjAge[id] = 0;

		RemoveMark(id);

		_simulation->SetWalkable(tile, true);

		// Remove surrounding shades
		removeSurroundingShade();
		TileArea checkArea(tile, 1);
		//checkArea.EnforceWorldLimit();
		//for (int y = checkArea.minY; y <= checkArea.maxY; y++) {
		//	for (int x = checkArea.minX; x <= checkArea.maxX; x++)
		//	{
		//		if (!HasNearbyTree(x, y)) {
		//			setTreeShade(x + y * _sizeX, false);
		//		}
		//	}
		//}

		// Update the HeightForestColor texture
		if (Time::Ticks() > 0) {
			// trees removal not marked animated are Inits
			_simulation->RefreshHeightForestColorTexture(checkArea, false);
		}

		_simulation->SetNeedDisplayUpdate(DisplayClusterEnum::Trees, tile.regionId(), true);

		//UE_LOG(LogTemp, Error, TEXT("RemoveTileObj: %s tick:%d"), *tile.To_FString(), Time::Ticks());

		_treeCount--;
		return yield;
	}

	TileObjInfo tileObjInfo = tileInfo(id);
	ResourceTileType tileType = tileObjInfo.type;
	if (tileType == ResourceTileType::Deposit)
	{
		RemoveMark(id);
		_treeEnum[id] = TileObjEnum::None;

		_simulation->SetWalkable(tile, true);

		removeSurroundingShade();

		_simulation->SetNeedDisplayUpdate(DisplayClusterEnum::Trees, tile.regionId(), true);

		_stoneCount--;
		return ResourcePair(ResourceEnum::Stone, StoneGatherYield_Base);
	}
	
	if (tileType == ResourceTileType::Bush)
	{
		ResourcePair yield = tileObjInfo.cutDownResource(tileObjAge(id), 0);

		TileObjEnum lastEnum = _treeEnum[id];
		
		_treeEnum[id] = TileObjEnum::None;

		_simulation->SetNeedDisplayUpdate(DisplayClusterEnum::Trees, tile.regionId(), true);

		int32 regionId = tile.regionId();
		_regionToReadyBushes[regionId].Set(tile.localTileId(), false);

		int32 grassCount = IsGrass(lastEnum) ? 1 : GrassToBushValue;
		_regionToGrassCount[regionId] += grassCount;
		_grassCount -= grassCount;

		return yield; //TODO: proper growth based return....
	}

	return ResourcePair();
}

ResourcePair TreeSystem::UnitGatherFruit100(WorldTile2 tile)
{
	int id = tile.tileId();
	if (!IsAliveTree(id)) {
		return ResourcePair(); // If the tree is already dead by the time he got here...
	}

	RemoveFruit(tile);

	_simulation->SetNeedDisplayUpdate(DisplayClusterEnum::Trees, tile.regionId(), true);

	return tileInfo(id).fruitResource100(_tileObjAge[id]);
}


ResourcePair TreeSystem::AnimalTrimBush(WorldTile2 tile, int32 trimEfficiency)
{
	// Note: trimEfficiency is the percent of plant resource that is converted to food.
	
	_simulation->statSystem(-1).AddStat(SeasonStatEnum::BushUnitTrimmed);

	int32 id = tile.tileId();
	
	//PUN_CHECK2(tileInfo(id).type == ResourceTileType::Bush, tileObjdebugStr(id));
	const TileObjInfo& info = tileInfo(id);
	if (info.type != ResourceTileType::Bush) {
		//PUN_LOG("Weird: Bad Bush %s", *ToFString(tileObjdebugStr(id)));
		return ResourcePair(ResourceEnum::Hay, 1);
	}
	
	return TrimBush(tile, trimEfficiency);
}

ResourcePair TreeSystem::TrimBush(WorldTile2 tile, int32 trimEfficiency)
{
	int32 id = tile.tileId();
	TileObjInfo tileObjInfo = tileInfo(id);
	check(tileObjInfo.type == ResourceTileType::Bush);

	int32 regionId = tile.regionId();
	_regionToReadyBushes[regionId].Set(tile.localTileId(), false);

	_simulation->SetNeedDisplayUpdate(DisplayClusterEnum::Trees, regionId, true);

	ResourcePair trimYield = tileObjInfo.cutDownResource(tileObjAge(id), 30, trimEfficiency);

	_tileObjAge[id] = tileObjInfo.trimmedGrowthTicks(); // down to less than 10% growth ... A bit randomized so bush size won't look too similar

	return trimYield;
}

void TreeSystem::ForceRemoveTileObjArea(TileArea area)
{
	area.ExecuteOnArea_WorldTile2([&](WorldTile2 tile) {
		int32_t tileId = tile.tileId();

		if (IsAliveTree(tileId)) {
			ForceRemoveTileObj(tile, false);
		}

		ResourceTileType tileType = tileInfo(tileId).type;
		if (tileType == ResourceTileType::Deposit ||
			tileType == ResourceTileType::Bush) {
			ForceRemoveTileObj(tile);
		}
	});
}

NonWalkableTileAccessInfo TreeSystem::FindNearestUnreservedFruitTree(WorldTile2 searchCenter, WorldTile2 unitTile, int32 radius, int32 maxFloodDist, bool canPassGate)
{
	SCOPE_CYCLE_COUNTER(STAT_PunFindFruit);

	WorldRegion2 region = searchCenter.region();

	NonWalkableTileAccessInfo nearestAccessInfo = NonWalkableTileAccessInfoInvalid;
	int nearestDist = radius; // GameMapConstants::TilesPerWorldX;

	for (int xx = region.x - 1; xx <= region.x + 1; xx++) {
		for (int yy = region.y - 1; yy <= region.y + 1; yy++) 
		{
			WorldRegion2 curRegion(xx, yy);
			if (curRegion.IsValid()) 
			{
				_regionToReadyFruits[curRegion.regionId()].Execute([&](WorldTile2 curTile)
				{
					if (_reservations.Contains(curTile.tileId())) {
						return;
					}

					NonWalkableTileAccessInfo accessInfo = _simulation->TryAccessNonWalkableTile(unitTile, curTile, maxFloodDist, canPassGate); // maxFloodDist 2 can be too hard to FindPath...
					if (accessInfo.isValid())
					{
						int dist = abs(curTile.x - unitTile.x) + abs(curTile.y - unitTile.y);
						if (dist < nearestDist) {
							nearestDist = dist;
							nearestAccessInfo = accessInfo;
						}
					}
				});
				
				//auto& fruitLocalTileIds = _regionToFruitLocalTileIds[curRegion.regionId()];

				//// TODO: Potential cause of non-determinism
				//for (int16 localTileId : fruitLocalTileIds) {
				//	WorldTile2 curWorldTile = curRegion.worldTile2(LocalTile2(localTileId));

				//	if (_reservations.Contains(curWorldTile.tileId())) {
				//		continue;
				//	}

				//	NonWalkableTileAccessInfo accessInfo = _simulation->TryAccessNonWalkableTile(tile, curWorldTile, maxFloodDist, canPassGate); // maxFloodDist 2 can be too hard to FindPath...
				//	if (accessInfo.isValid())
				//	{
				//		int dist = abs(curWorldTile.x - tile.x) + abs(curWorldTile.y - tile.y);
				//		if (dist < nearestDist) {
				//			nearestDist = dist;
				//			nearestAccessInfo = accessInfo;
				//		}
				//	}
				//}
			}
		}
	}

	return nearestAccessInfo;
}

WorldTile2 TreeSystem::FindNearestUnreservedFullBush(WorldTile2 unitTile, const std::vector<WorldRegion2>& regions, int32 maxFloodDist, bool canPassGate)
{
	for (WorldRegion2 region : regions) {
		WorldTile2 foundTile = FindNearestUnreservedFullBush(unitTile, region, maxFloodDist, canPassGate);
		if (foundTile.isValid()) {
			return foundTile;
		}
	}
	return WorldTile2::Invalid;
}

WorldTile2 TreeSystem::FindNearestUnreservedFullBush(WorldTile2 unitTile, WorldRegion2 originRegion, int32 maxFloodDist, bool canPassGate)
{
	SCOPE_CYCLE_COUNTER(STAT_PunFindFullBush);

	PUN_CHECK(originRegion.IsValid());
	if (!originRegion.IsValid()) {
		return WorldTile2::Invalid;
	}

	WorldTile2 nearestTile = WorldTile2::Invalid;
	int32 nearestDist = GameMapConstants::TilesPerWorldX;

	_regionToReadyBushes[originRegion.regionId()].Execute([&](WorldTile2 curTile)
	{
		DEBUG_ISCONNECTED_VAR(FindNearestUnreservedFullBush);
		
		if (!_reservations.Contains(curTile.tileId()) &&
			_simulation->IsConnected(unitTile, curTile, maxFloodDist)) //TODO: faster with precomputed floodInfo??
		{
			int dist = WorldTile2::ManDistance(curTile, unitTile);
			if (dist < nearestDist) {
				nearestDist = dist;
				nearestTile = curTile;
			}
		}
	});

	return nearestTile;

	//// Try middle first, if already one just return..
	//findNearest(originRegion);
	//if (nearestTile.isValid()) {
	//	return nearestTile;
	//}


	//for (int xx = originRegion.x - 1; xx <= originRegion.x + 1; xx++) {
	//	for (int yy = originRegion.y - 1; yy <= originRegion.y + 1; yy++)
	//	{
	//		WorldRegion2 curRegion(xx, yy);
	//		if (curRegion == originRegion) {
	//			continue; // Skip mid
	//		}
	//		if (!curRegion.IsValid()) {
	//			continue;
	//		}

	//		findNearest(curRegion);
	//		if (nearestTile.isValid()) {
	//			return nearestTile;
	//		}
	//	}
	//}

	//return nearestTile;
}



const int initialPlantChance = 5;
const int initialStonePlaceChance = 2;

const int initialPlainGrassChance = 2;

//const FloatDet grassFreq = FD0_XX(12);
const FloatDet freq = FD0_XX(8);


void TreeSystem::PlantInitial()
{
	// Run initial simulation to stabilize
	auto t1 = high_resolution_clock::now();

	PunAStar128x256* pathAI = _simulation->pathAI();

	auto& terrainGenerator = _simulation->terrainGenerator();

	// Stones
	for (int i = 0; i < _sizeXY; i++) {
		int x = i % _sizeX;
		int y = i / _sizeX;
		WorldTile2 tile(x, y);

		// Water? Make it fish
		if (_simulation->IsWater(tile))
		{
			_tileObjAge[i] = 0; // Fish use treeAge for number of fishing lodge.
			_treeEnum[i] = TileObjEnum::Fish;
		}
		// Plant Stone randomly
		else if (GameRand::Rand() % initialStonePlaceChance == 0 &&
			terrainGenerator.resourcePerlin(tile) < StonePerlinCutoff &&
			//perlinGenerator.noise01(x * freq, y * freq) < StonePerlinCutoff &&
			pathAI->isWalkable(x, y))
		{
			PlantTree(x, y, TileObjEnum::Stone, true);
			//testPerlin[i] = perlinGenerator.noise01(x * freq, y * freq); // For displaying perlin used
		}
	}
	
	// Trees
	for (int j = 0; j < 2; j++) {
		for (int i = 0; i < _sizeXY; i++)
		{
			int x = i % _sizeX;
			int y = i / _sizeX;
			WorldTile2 tile(x, y);

			BiomeEnum biomeEnum = terrainGenerator.GetBiome(tile);

			if (biomeEnum == BiomeEnum::Tundra) {
				continue;
			}
			
			// Plant randomly
			//if (GameRand::Rand() % initialPlantChance == 0 && !treeShade(i) && GameMap::PathAI->isWalkable(x, y)) {
			int32 fertilityPercent = _simulation->GetFertilityPercent(tile);

			if (biomeEnum == BiomeEnum::Desert ||
				biomeEnum == BiomeEnum::Savanna)
			{
				if (fertilityPercent >= HardyTreeFertility)
				{
					int actualInitialPlantChance = initialPlantChance * HardyTreeChance;
					
					if (GameRand::Rand() % actualInitialPlantChance == 0 &&
						!treeShade(i) &&
						pathAI->isWalkable(x, y))
					{
						//WorldTile2 tile(x, y);
						TileObjEnum treeEnum = GetBiomeInfo(biomeEnum).GetRandomTreeEnum();

						int32 coastal = terrainGenerator.IsOceanCoast(tile);
						if (coastal > 125) {
							treeEnum = TileObjEnum::Coconut;
						}
						
						PlantTree(x, y, treeEnum, true);

						int32 maxGrowth = GetTileObjInfo(treeEnum).maxGrowthTick;
						if (maxGrowth > 0) {
							_tileObjAge[i] = static_cast<int32>(GameRand::Rand() % maxGrowth + maxGrowth);
							check(_tileObjAge[i] > 0);
						}
					}
				}
			}
			else
			{
				if (fertilityPercent > TreeFertility)
				{
					// Plant chance is less likely with less fertility close to TreeFertility
					int32 cappedFertilityPercent = min(fertilityPercent, TreeFertilityAtMaxPlantChance);
					int32 cappedFertilityAboveTree = cappedFertilityPercent - TreeFertility;
					PUN_CHECK(cappedFertilityAboveTree > 0);

					int actualInitialPlantChance = initialPlantChance * (TreeFertilityAtMaxPlantChance - TreeFertility) / cappedFertilityAboveTree;

					if (actualInitialPlantChance > 0 &&
						GameRand::Rand() % actualInitialPlantChance == 0 &&
						!treeShade(i) &&
						terrainGenerator.resourcePerlin(tile) >= TreePerlinMaxCutoff &&
						pathAI->isWalkable(x, y))
					{
						//WorldTile2 tile(x, y);
						BiomeInfo biomeInfo = GetBiomeInfo(biomeEnum);
						TileObjEnum treeEnum = biomeInfo.GetRandomTreeEnum();

						if (biomeEnum != BiomeEnum::BorealForest)
						{
							int32 coastal = terrainGenerator.IsOceanCoast(tile);
							if (coastal > 125) {
								treeEnum = TileObjEnum::Coconut;
							}
						}

						//uint32_t treeEnumInt = GameRand::Rand() % TreeEnumSize; //TileObjEnum

						PlantTree(x, y, treeEnum, true);

						//testPerlin[i] = perlinGenerator.noise01(x * freq, y * freq);

						int32 maxGrowth = GetTileObjInfo(treeEnum).maxGrowthTick;
						if (maxGrowth > 0) {
							// TODO: proper max growth distribution... horizontal line ... then decay drop....
							_tileObjAge[i] = static_cast<int32>(GameRand::Rand() % maxGrowth + maxGrowth);
							check(_tileObjAge[i] > 0);

							// Give 1/8 chance to have fruit to show players building Fruit gatherer right away fruits are available...
							if (tileInfo(i).IsFruitBearer() && GameRand::Rand() % 8 == 0) {
								AddFruit(tile);
							}
						}
					}
				}
			}

		}
	}

	// Replace tree/plant by georesource
	const int32_t maxReplaceRadius = 15;

	auto& provinceSys = _simulation->provinceSystem();
	auto& georesourceSys = _simulation->georesourceSystem();
	
	const vector<int32>& georesourceRegions = georesourceSys.georesourceRegions();
	for (int32 georesourceRegionId : georesourceRegions)
	{
		PUN_CHECK(provinceSys.IsProvinceValid(georesourceRegionId));
		
		GeoresourceInfo geoInfo = georesourceSys.georesourceNode(georesourceRegionId).info();
		
		// Tree
		TileObjEnum treeEnum = geoInfo.geoTreeEnum;
		if (treeEnum != TileObjEnum::None) 
		{
			WorldTile2 center = provinceSys.GetProvinceCenterTile(georesourceRegionId);
			TileArea(center, 15).ExecuteOnArea_WorldTile2([&](WorldTile2 tile)
			{
				int tileId = tile.tileId();
				TileObjEnum tileObjEnum = _treeEnum[tileId];

				if (GetTileObjInfo(tileObjEnum).type == ResourceTileType::Tree) 
				{
					int32 distance = WorldTile2::Distance(tile, center);

					// Distance 15 beyond has 0 chance of spawning...
					// TODO: take into account fertility?
					int32 percentChance = std::max(0, (100 - 100 * distance / maxReplaceRadius));

					if (treeEnum == TileObjEnum::GiantMushroom) {
						percentChance /= 3; // Don't want a lot of mushroom
					}
					
					if (GameRand::Rand() % 100 < percentChance) {
						PUN_CHECK(IsTileObjEnumValid(treeEnum));
						_treeEnum[tileId] = treeEnum;
					}
				}
			});
		}
	}

	// Grass
	for (int i = 0; i < _sizeXY; i++)
	{
		int x = i % _sizeX;
		int y = i / _sizeX;
		WorldTile2 tile(x, y);

		int32 fertility = _simulation->GetFertilityPercent(tile);

		if (tileInfo(i).treeEnum == TileObjEnum::None &&
			pathAI->isWalkable(x, y) &&
			fertility > PlantFertility)
		{
			if (terrainGenerator.resourcePerlin(tile) >= TreePerlinMaxCutoff)
			//if (perlinGenerator.noise01(x * freq, y * freq) > TreePerlinMaxCutoff)
			{
			//	if (GameRand::Rand() % initialPlainGrassChance == 0) {
			//		_treeAge[i] = 0;
			//		
			//		//int32_t plantTypeRand = GameRand::Rand() % 12;
			//		//if (plantTypeRand == 0) {
			//		//	_treeEnum[i] = TileObjEnum::OreganoBush;
			//		//}
			//		//else if (plantTypeRand == 1) {
			//		//	_treeEnum[i] = TileObjEnum::WhiteFlowerBush;
			//		//}
			//		////else if (plantTypeRand == 3) {
			//		////	_treeEnum[i] = TileObjEnum::RedPinkFlowerBush;
			//		////}
			//		//else 
			//		{
			//			_treeEnum[i] = TileObjEnum::GrassGreen;
			//		}

			//		_simulation->SetNeedDisplayUpdate(DisplayClusterEnum::Trees, WorldTile2(i).regionId(), true);
			//	}
			//}
			//else {
				BiomeEnum biomeEnum = terrainGenerator.GetBiome(WorldTile2(x, y));

				if (biomeEnum == BiomeEnum::Tundra) {
					continue;
				}
				
				BiomeInfo biomeInfo = GetBiomeInfo(biomeEnum);

				int32 plantChance = biomeInfo.initialBushChance;

				// The less fertility, the less chance of planting
				const int32 fullPlantBand = 10;
				if (fertility - PlantFertility < fullPlantBand)
				{
					int32 percentToFull = std::min(100, (fertility - PlantFertility) * 100 / fullPlantBand);
					plantChance = plantChance * 100 / percentToFull;
				}
				
				if (IsGrassDominant(biomeEnum)) {
					plantChance /= GrassToBushValue;
				}

				if (GameRand::Rand() % plantChance == 0)
				{

					int32 plantTypeRand = GameRand::Rand() % 100;
					if (plantTypeRand <= 98) {
						TileObjEnum plantEnum = biomeInfo.GetRandomPlantEnum();
						PlantBush(i, plantEnum, false, true);
					}
					// Rare plant
					else if (biomeInfo.HasRarePlant()) {
						TileObjEnum plantEnum = biomeInfo.GetRandomRarePlantEnum();
						PlantBush(i, plantEnum, false, true);
					}
					
					//else
					//{
					//	_treeEnum[i] = TileObjEnum::GrassGreen;
					//}

					_simulation->SetNeedDisplayUpdate(DisplayClusterEnum::Trees, WorldTile2(i).regionId(), true);
				}
			}

		}
	}

	//for (WorldRegion2 georesourceRegion : georesourceRegions)
	//{
	//	GeoresourceInfo geoInfo = georesourceSys.georesourceNode(georesourceRegion.regionId()).info();

	//	// Bush
	//	TileObjEnum bushEnum = geoInfo.geoBushEnum;
	//	if (bushEnum != TileObjEnum::None)
	//	{
	//		WorldTile2 center = georesourceRegion.centerTile();
	//		georesourceRegion.ExecuteOnRegion_Tile([&](int16_t x, int16_t y)
	//		{
	//			WorldTile2 tile(x, y);
	//			int tileId = tile.tileId();
	//			TileObjEnum tileObjEnum = _treeEnum[tileId];
	//			if (GetTileObjInfo(tileObjEnum).type == ResourceTileType::Bush) {
	//				int32_t distance = WorldTile2::Distance(tile, center);

	//				// Distance 15 beyond has 0 chance of spawning...
	//				// TODO: take into account fertility?
	//				int32 percentChance = std::max(0, (100 - 100 * distance / maxReplaceRadius));
	//				percentChance *= 2; // more bush replacement since it is harder too see
	//				
	//				if (GameRand::Rand() % 100 < percentChance) {
	//					_treeEnum[tileId] = bushEnum;
	//				}
	//			}
	//		});
	//	}
	//}

	auto t2 = high_resolution_clock::now();
	auto time_span = duration_cast<milliseconds>(t2 - t1);

	//UE_LOG(LogTemp, Error, TEXT("Run Initial Tree Simulation: %ld ms"), time_span.count());
}

bool TreeSystem::CanPlantTreeOrBushInGrid(WorldTile2 tile)
{
	return _simulation->GetFertilityPercent(tile) >= TreeFertility &&
			_simulation->IsBuildable(tile) &&
			_treeEnum[tile.tileId()] == TileObjEnum::None && // Plant's tile is buildable... so must make sure there is no plant here
			_simulation->dropSystem().GetDrops(tile).size() == 0;  // Avoid drops
}

void TreeSystem::EmitSeed(int32 originId, TileObjInfo info)
{
	check(info.type == ResourceTileType::Bush || info.type == ResourceTileType::Tree);

	if (info.type == ResourceTileType::Bush) {
		_simulation->statSystem(-1).AddStat(SeasonStatEnum::BushEmitSeed);
	}

	WorldTile2 tile = GetSeedArrivalSpot(originId, info.type == ResourceTileType::Tree ? 10 : 5); // Tree seed needs to go farther since it grows very slowly.. plant can just spread..

	auto& terrainGenerator = _simulation->terrainGenerator();
	
	if (CanPlantTreeOrBush(tile))
		//GameMap::IsInGrid(tile) && 
		//_simulation->GetFertilityPercent(tile) >= TreeFertility &&
		//_simulation->IsBuildable(tile) &&
		//_treeEnum[tile.tileId()] == TileObjEnum::None && // Plant's tile is buildable... so must make sure there is no plant here
		//_simulation->dropSystem().GetDrops(tile).size() == 0
		//) // Avoid drops
	{
		if (info.type == ResourceTileType::Tree) 
		{
			if (!treeShade(tile.tileId()) &&
				terrainGenerator.resourcePerlin(tile) < FD0_XX(59))
			{
				// Check to ensure there is no nearby building
				bool hasNoNearbyBuilding = true;
				for (int32 y = -1; y <= 1; y++) {
					for (int32 x = -1; x <= 1; x++) {
						WorldTile2 curTile(tile.x + x, tile.y + y);
						if (curTile.isValid() && _simulation->HasBuilding(curTile.tileId())) {
							hasNoNearbyBuilding = false;
							break;
						}
					}
				}

				if (hasNoNearbyBuilding) {
					PlantTree(tile.x, tile.y, info.treeEnum);
				}
			}
		}
		else {
			_simulation->statSystem(-1).AddStat(SeasonStatEnum::BushEmitSeedGoodSpot);

			if (_regionToGrassCount[tile.regionId()] > 0) {
				PlantBush(tile.tileId(), info.treeEnum);
				_simulation->statSystem(-1).AddStat(SeasonStatEnum::BushEmitSeedSuccess);
			}
		}
	}
}

void TreeSystem::TickTile(int32_t id)
{
	TileObjEnum treeEnum;
	{
		//SCOPE_CYCLE_COUNTER(STAT_PunTreeTickInitial);
		//PUN_CHECK(0 <= id && id < _sizeXY);

		treeEnum = _treeEnum[id];
	}
	//check(TreeEnumSize > (int)tileObjEnum && (int)tileObjEnum >= 0);

	if (IsPlantFast(treeEnum))
	{
		TileObjInfo info = GetTileObjInfo(treeEnum);
		
		WorldTile2 tile(id);
		uint32_t rand = GameRand::Rand();

		{
			//SCOPE_CYCLE_COUNTER(STAT_PunTreeAgeDeath); // Note: This screws up Tree.Tick Count

			// Tree Death
			if (info.canDieFromAge(_tileObjAge[id]) && 
				rand % info.deathChancePerCycle == 0 &&
				_simulation->IsBuildable(tile)) // Plants owned by human won't die
				// TODO: why the last IsTileType None... which is now ... GameMap::IsBuildable(tile)
			{
				ForceRemoveTileObj(tile);

				if (info.type == ResourceTileType::Bush) {
					_simulation->statSystem(-1).AddStat(SeasonStatEnum::BushDeathAge);
				}
				return;
			}
		}

		{
			//SCOPE_CYCLE_COUNTER(STAT_PunTreeFruit); // Note: This screws up Tree.Tick Count

			// No Fruit in winter
			if (Time::IsSnowing()) {
				RemoveFruit(tile); // TODO: don't need to call this multiple times????
				_simulation->SetNeedDisplayUpdate(DisplayClusterEnum::Trees, tile.regionId(), true);

				// Note: Bush used to be trimmed in winter, but that is just a waste of CPU time
				// Nothing grows during winter
			}
			// Fruit and seed
			// fruitChancePerCycle as a chance... If we use timed ripe, all trees ended up riping at the same time...
			// fruitChancePerCycle was computed first since it is cheaper than hasFruit() and there is only 1/50 chance to ripe
			else
			{
				// Grow during non-winter time...
				_tileObjAge[id] += TileObjInfo::TicksPerCycle();
				int32 age = _tileObjAge[id];

				// Ready bushes for animals to eat
				if (info.type == ResourceTileType::Bush)
				{
					if (info.isBushReadyForAnimalHarvest(age))
					{
						auto& readyBushes = _regionToReadyBushes[tile.regionId()];
						int32 localTileId = tile.localTileId();
						if (!readyBushes.Get(localTileId)) {
							readyBushes.Set(localTileId, true);
							_regionToLastReadyBushUpdate[tile.regionId()] = Time::Ticks();
						}
					}

					// Even in plants without fruit, fruitChance is used to emit seed...
					if (info.fruitChancePerCycle > 0 &&
						(rand % info.fruitChancePerCycle == 0))
					{
						if (info.isSeedEmissionAge(age)) 
						{
							EmitSeed(id, info); 

							// Bush doesn't bear fruit
						}
					}
				}
				else
				{
					// Even in trees without fruit, fruitChance is used to emit seed...
					// fruitChancePerCycle of 0 means it doesn't emit seeds (like farmed plants)
					if (info.fruitChancePerCycle > 0 &&
						(rand % info.fruitChancePerCycle == 0))
					{
						if (info.isSeedEmissionAge(age))
						{
							if (rand % info.fruitToEmitSeedChance == 0) {
								EmitSeed(id, info);
							}

							// Fruit bearer
							if (info.IsFruitBearer() && !hasFruit(id)) {
								AddFruit(tile);
								_simulation->SetNeedDisplayUpdate(DisplayClusterEnum::Trees, tile.regionId(), true);
							}
						}
					}
				}

			}
		}
	}

}

void TreeSystem::Tick()
{
	LEAN_PROFILING_R(TreeSysTick);
	//CheckMemoryValidity();

	// Only tick on specific sector.
	int tilesPerSplit = _sizeXY / TileObjInfo::UpdateChunkCount;
	int splitIndex = Time::Ticks() % TileObjInfo::TicksPerCycle();

	int start = splitIndex * tilesPerSplit;
	int end = start + tilesPerSplit;
	end = end <= _sizeXY - 1 ? end : _sizeXY - 1; // A bit slower but, end < _sizeXY makes confusing bugs

	check(end <= _sizeXY - 1)
	{
		SCOPE_CYCLE_COUNTER(STAT_PunTreeTickTiles);
		for (int i = start; i <= end; i++) {
			TickTile(i);
		}
	}

	// Fallen tree, remove it if it has aged long enough (120 ticks)
	// Separate into TileObjInfo::UpdateChunkCount
	{
		SCOPE_CYCLE_COUNTER(STAT_PunTreeFallen);

		check(_regionToLocalTileIdToFallenTreeInfo.size() % TileObjInfo::UpdateChunkCount == 0);
		const int fallenTreeUpdateSplits = 16;
		int regionsPerSplit = _regionToLocalTileIdToFallenTreeInfo.size() / fallenTreeUpdateSplits;
		splitIndex = 0;

		int startRegion = splitIndex * regionsPerSplit;
		int endRegion = startRegion + regionsPerSplit;

		for (int i = startRegion; i < endRegion; i++) {
			auto& localTileIdToFellTicks = _regionToLocalTileIdToFallenTreeInfo[i];

			for (auto it = localTileIdToFellTicks.begin(); it != localTileIdToFellTicks.end();) {
				if (Time::Ticks() - it->second.fellTick > TileObjInfo::FellAnimationTicks) {
					//UE_LOG(LogTemp, Error, TEXT("erase localTileIdToFellTicks: %d, %d"), it->first, it->second);
					it = localTileIdToFellTicks.erase(it);
				}
				else {
					it++;
				}
			}
		}
	}
}

int32 TreeSystem::MarkArea(int32 playerId, TileArea area, bool isRemoving, ResourceEnum resourceEnum)
{
	//PUN_DEBUG(FString::Printf(TEXT("MarkArea playerId: %d size:%lu"), playerId, _playerIdToRegionToMarkedTileIds.size()));
	area.EnforceWorldLimit();

	int32 markCount = 0;
	
	area.ExecuteOnArea_WorldTile2([&](WorldTile2 tile)
	{
		int32 tileId = tile.tileId();
		TileObjInfo info = tileInfo(tileId);
		bool isValidTree = info.type == ResourceTileType::Tree && info.growthPercent(_tileObjAge[tileId]) > 50; // Cut tree only after 50%
		bool isValidDeposit = info.type == ResourceTileType::Deposit;

		bool isValid = false;
		if (resourceEnum == ResourceEnum::Wood) {
			isValid = isValidTree;
		}
		else if (resourceEnum == ResourceEnum::Orange) {
			PUN_CHECK(!isRemoving);
			isValid = isValidTree && (info.treeEnum != TileObjEnum::Orange && info.treeEnum != TileObjEnum::Papaya && info.treeEnum != TileObjEnum::Coconut);
		}
		else if (resourceEnum == ResourceEnum::Stone) {
			isValid = isValidDeposit;
		}
		else if (resourceEnum == ResourceEnum::None) {
			isValid = isValidTree || isValidDeposit;
		}

		// Ensure regionOwner is playerId
		if (isValid && _simulation->tileOwnerPlayer(tile) == playerId)
		{
			if (isRemoving) {
				// If there was reservation, reset those reservers
				if (_reservations.Contains(tileId)) {
					_simulation->ResetUnitActions(_reservations[tileId].unitId);
				}
				PUN_CHECK(!_reservations.Contains(tileId));
				
				//if (!_reservations.Contains(tileId)) {
					RemoveMark(tileId);
				//}
			}
			else {
				_regionToMarkTileIds[tile.regionId()].Set(tile.localTileId(), true);
				markCount++;
			}

			_simulation->SetNeedDisplayUpdate(DisplayClusterEnum::Trees, tile.regionId(), true);
		}
	});

	return markCount;
}

void TreeSystem::CheckMemoryValidity() {
	for (int i = 0; i < _sizeXY; i++) {
		if (!IsTileObjEnumValid(_treeEnum[i])) {
			UE_LOG(LogTemp, Error, TEXT("TreeEnumInvalid i:%d, enum:%d"), i, (int)_treeEnum[i]);
			UE_DEBUG_BREAK();
		}
	}
}