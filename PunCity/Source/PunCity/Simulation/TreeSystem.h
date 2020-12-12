#pragma once

#include "GameSimulationInfo.h"
#include "IGameSimulationCore.h"

#include "PunCity/PunUtils.h"
#include "PunCity/PunTimer.h"
#include "Perlin.h"

#include "UnitBase.h"

#include <unordered_map>
#include <unordered_set>
#include "PunCity/AlgorithmUtils.h"

struct FallenTreeInfo
{
	TileObjEnum treeEnum;
	int32_t tileObjAgeTick;
	int32_t fellTick;

	//! Serialize
	void operator>>(FArchive &Ar) {
		Ar << treeEnum;
		Ar << tileObjAgeTick;
		Ar << fellTick;
	}

	TileObjInfo tileInfo() { return GetTileObjInfo(treeEnum); }
};

// region-based hash map
// Buckets is vector, not pointer
// No key needed since 
// TODO: use this for looping through trees instead???
// could be faster if we initialize buckets only as needed?
template <typename T>
class SpatialHashMap
{
public:
	struct SpatialHashMapPair
	{
		int32_t tileId;
		T value;

		//! Serialize
		FArchive& operator>>(FArchive &Ar) {
			Ar << tileId;
			value >> Ar;
			return Ar;
		}
	};

	SpatialHashMap() {
		_regionToValues.resize(GameMapConstants::TotalRegions);
	}

	void Add(int32_t tileId, T value) {
		_regionToValues[WorldTile2(tileId).regionId()].push_back({tileId, value});
	}

	T Remove(int32_t tileId) {
		std::vector<SpatialHashMapPair>& values = _regionToValues[WorldTile2(tileId).regionId()];
		for (int i = values.size(); i-- > 0;) {
			if (values[i].tileId == tileId) {
				T result = values[i].value;
				values.erase(values.begin() + i);
				return result;
			}
		}
		PUN_NOENTRY();
		return _invalid;
	}

	T& TryGet(int32_t tileId) {
		std::vector<SpatialHashMapPair>& values = _regionToValues[WorldTile2(tileId).regionId()];
		for (int i = values.size(); i-- > 0;) {
			if (values[i].tileId == tileId) {
				return values[i].value;
			}
		}
		return _invalid;
	}

	bool Contains(int32_t tileId) {
		std::vector<SpatialHashMapPair>& values = _regionToValues[WorldTile2(tileId).regionId()];
		for (int i = values.size(); i-- > 0;) {
			if (values[i].tileId == tileId) {
				return true;
			}
		}
		return false;
	}

	T& operator[] (int32_t tileId){
		std::vector<SpatialHashMapPair>& values = _regionToValues[WorldTile2(tileId).regionId()];
		for (int i = values.size(); i-- > 0;) {
			if (values[i].tileId == tileId) {
				return values[i].value;
			}
		}
		PUN_NOENTRY();
		return _invalid;
	}

	//! Serialize
	FArchive& operator>>(FArchive &Ar) {
		SerializeVecVecObj(Ar, _regionToValues);
		return Ar;
	}

private:
	std::vector<std::vector<SpatialHashMapPair>> _regionToValues;
	T _invalid;
};

/**
 * 
 */
class TreeSystem
{
public:
	void Init(int sizeX, int sizeY, IGameSimulationCore* simulation);
	void Tick();
	void TickTile(int32_t id);

	int treeCount() { return _treeCount; }
	int stoneCount() { return _stoneCount; }
	int bushCount() { return _grassCount; }

	bool IsReserved(int tileId) { return _reservations.Contains(tileId); }
	UnitReservation& Reservation(int tileId) { return _reservations[tileId]; }
	void SetReserved(int tileId, UnitReservation& reservation) { 
		PUN_CHECK2(!_reservations.Contains(tileId), ("ExistingUnit:\n" + _simulation->unitdebugStr(_reservations[tileId].unitId) + "\n\nNewUnit:" + _simulation->unitdebugStr(reservation.unitId)));
		_reservations.Add(tileId, reservation);

		WorldTile2 tile(tileId);
		//GameMap::PathAI->SetUnreservedFruit(tile.x, tile.y, false); // Just in case this is fruit, it won't be available for pathing anymore
	}
	void Unreserve(int tileId, UnitReservation& reservation) {
		UnitReservation tileReservation = _reservations.Remove(tileId);
		PUN_CHECK2(reservation == tileReservation, (tileReservation.ToString() + "," + reservation.ToString() + "," + _simulation->unitdebugStr(reservation.unitId)));
		
		// Just in case this is fruit tile, this opens up fruit for pathfinding again
		if (hasFruit(tileId)) {
			WorldTile2 tile(tileId);
			//GameMap::PathAI->SetUnreservedFruit(tile.x, tile.y, true);
		}
	}

	void PlantTree(int32_t x, int32_t y, TileObjEnum treeEnum, bool initial = false);
	bool HasNearbyShadeObject(int x, int y);
	void PlantTileObj(int32_t id, TileObjEnum tileObjEnum)
	{
		PUN_CHECK(IsTileObjEnumValid(tileObjEnum));
		
		_tileObjAge[id] = 0;
		_treeEnum[id] = tileObjEnum;
		_simulation->SetNeedDisplayUpdate(DisplayClusterEnum::Trees, WorldTile2(id).regionId(), true);
	}
	void PlantBush(int32_t id, TileObjEnum tileObjEnum, bool updateDisplay = true, bool randomizeAge = false);

	void PlantFullTileObj(int32 id, TileObjEnum tileObjEnum)
	{
		PUN_CHECK(IsTileObjEnumValid(tileObjEnum));
		
		_treeEnum[id] = tileObjEnum;
		_tileObjAge[id] = GetTileObjInfo(tileObjEnum).maxGrowthTick;
	}

	ResourcePair HumanHarvest(WorldTile2 tile);
	ResourcePair UnitHarvestTreeOrDeposit(WorldTile2 tile);
	ResourcePair UnitHarvestBush(WorldTile2 tile);
	void UnitNourishBush(WorldTile2 tile);
	void UnitNourishTree(WorldTile2 tile);

	ResourcePair UnitGatherFruit100(WorldTile2 tile);
	ResourcePair AnimalTrimBush(WorldTile2 tile, int32 trimEfficiency);

	// Force = not a reserver, but trying to remove the tile anyway...
	void ForceRemoveTileObj(WorldTile2 tile, bool animate = true);
	void ForceRemoveTileObjArea(TileArea area);

	void ForceRemoveTileReservation(WorldTile2 tile);
	void ForceRemoveTileReservationArea(TileArea area);

	NonWalkableTileAccessInfo FindNearestUnreservedFruitTree(WorldTile2 searchCenter, WorldTile2 unitTile, int32 radius, int32 maxFloodDist, bool canPassGate);

	WorldTile2 FindNearestUnreservedFullBush(WorldTile2 unitTile, const std::vector<WorldRegion2>& regions, int32 maxFloodDist, bool canPassGate);
	WorldTile2 FindNearestUnreservedFullBush(WorldTile2 unitTile, WorldRegion2 originRegion, int32 maxFloodDist, bool canPassGate);

	int32 tileObjAge(int id) { return _tileObjAge[id]; }

	int32 growthPercent(int32 id) {
		return tileInfo(id).growthPercent(tileObjAge(id));
	}
	int32 bonusYieldPercent(int32 id) {
		return tileInfo(id).bonusYieldPercent(tileObjAge(id));
	}
	int32 lifeSpanPercent(int32 id) {
		return tileInfo(id).lifeSpanPercent(tileObjAge(id));
	}
	int32 tileYieldPercent(int32 id) {
		return tileInfo(id).tileYieldPercent(tileObjAge(id));
	}

	int32 isBushReadyForHarvest(int32 id) { return tileInfo(id).isBushReadyForAnimalHarvest(tileObjAge(id)); }

	// Fish
	int32 fish100Count(int32 id, int32 fisherCountChange = 0) {
		if (tileObjEnum(id) == TileObjEnum::Fish) {
			return 100 / (_tileObjAge[id] + 1 + fisherCountChange); // 1 / (fishers + 1)
		}
		return 0;
	}
	void ChangeFisherCount(int32 id, int32 valueChange) {
		if (tileObjEnum(id) == TileObjEnum::Fish) {
			_tileObjAge[id] += valueChange;
		}
	}

	bool treeShade(int32 id) {
		uint32 shift = id & 0b111;
		return (_treeShade[id >> 3] >> shift) & 1;
	}
	TileObjEnum tileObjEnum(int32 id) { return _treeEnum[id]; }

	bool IsEmptyLandTile(int32 id) { return _treeEnum[id] == TileObjEnum::None; }
	bool IsEmptyTile_WaterAsEmpty(int32 id) {
		return _treeEnum[id] == TileObjEnum::None || _treeEnum[id] == TileObjEnum::Fish;
	}

	TileObjInfo tileInfo(int id) { return GetTileObjInfo(_treeEnum[id]); }

	bool CanPlantTreeOrBush(WorldTile2 tile) {
		return GameMap::IsInGrid(tile) && CanPlantTreeOrBushInGrid(tile);
	}
	bool CanPlantTreeOrBushInGrid(WorldTile2 tile);

	bool hasFruit(int id) {
		WorldTile2 tile(id);
		return _regionToReadyFruits[tile.regionId()].Get(tile.localTileId());
	}

	WorldTile2 GetSeedArrivalSpot(int32 id, int32 seedTravelDistance)
	{
		int x = id % _sizeX + GameRand::Rand() % (seedTravelDistance * 2 + 1) - seedTravelDistance;
		int y = id / _sizeX + GameRand::Rand() % (seedTravelDistance * 2 + 1) - seedTravelDistance;
		return WorldTile2(x, y);
	}

	// Debug
	std::string tileObjdebugStr(int32 id)
	{
		auto info = tileInfo(id);
		if (info.treeEnum == TileObjEnum::None) {
			return "NONE";
		}
		
		int32 age = tileObjAge(id);
		return "\n" + info.name + ", age:" + std::to_string(age) 
				+ "\n growth:" + std::to_string(info.growthPercent(age)) 
				+ "\n yield:" + std::to_string(info.tileYieldPercent(age))
				+ "\n cutdown10:" + std::to_string(info.cutDownResource(age, 10).count)
				+ "\n fruit100:" + std::to_string(info.fruitResource100(age).count);
	}

public:
	//! Mark Resources
	bool HasMark(int32 playerId, int32 tileId)
	{
		WorldTile2 tile(tileId);
		if (_simulation->tileOwner(tile) != playerId) {
			return false;
		}
		
		auto& tileIds = _regionToMarkTileIds[tile.regionId()];
		return tileIds.Get(tile.localTileId());
		//return tileIds.find(tileId) != tileIds.end();
	}

	//! This is for human only
	// regionDistance is the farthest distance we will look for gather marks
	NonWalkableTileAccessInfo FindNearestMark(int32 playerId, WorldTile2 originTile, bool treeOnly, int32_t regionDistance = 2)
	{
		WorldRegion2 originRegion = originTile.region();
		int32 nearestDist = GameMapConstants::TilesPerWorldX;
		NonWalkableTileAccessInfo nearestAcessInfo = NonWalkableTileAccessInfoInvalid;

		for (int32 x = originRegion.x - regionDistance; x <= originRegion.x + regionDistance; x++) {
			for (int32 y = originRegion.y - regionDistance; y <= originRegion.y + regionDistance; y++)
			{
				WorldRegion2 region(x, y);

				if (region.IsValid())
				{
					int32 regionId = region.regionId();
					
					{
						_regionToMarkTileIds[regionId].Execute([&](WorldTile2 tile)
						{
							// 
							if (treeOnly && tileInfo(tile.tileId()).type != ResourceTileType::Tree) {
								return;
							}

							if (IsReserved(tile.tileId())) {
								return;
							}

							// Ensure tile is owned by player..
							if (_simulation->tileOwner(tile) != playerId) {
								return;
							}

							// After finding an inaccessible target tile, find an adjacent tile to walk to
							NonWalkableTileAccessInfo accessInfo = _simulation->TryAccessNonWalkableTile(originTile, tile, regionDistance, true);
							if (accessInfo.isValid()) {
								int32 dist = WorldTile2::ManDistance(tile, originTile);
								if (dist < nearestDist) {
									nearestDist = dist;
									nearestAcessInfo = accessInfo;
								}
							}
						});
					}
				}
			}
		}
		return nearestAcessInfo;
	}

	int32 MarkArea(int32 playerId, TileArea area, bool isRemoving, ResourceEnum resourceEnum);


	/*
	 * Foresting
	 */
	NonWalkableTileAccessInfo FindCuttableTree(int32 playerId, WorldTile2 originTile, int32 radius, CutTreeEnum cutTreeEnum)
	{
		TileArea area(originTile, radius);
		area.EnforceWorldLimit();

		// Try finding larger tree first
		NonWalkableTileAccessInfo resultAccessInfo = GetCuttableTreeWithAge(originTile, radius, playerId, 80, cutTreeEnum);

		// If larger trees are gone, cut smaller trees
		if (!resultAccessInfo.isValid()) {
			resultAccessInfo = GetCuttableTreeWithAge(originTile, radius, playerId, 50, cutTreeEnum);
		}

		return resultAccessInfo;
	}
	NonWalkableTileAccessInfo FindNourishableTree(int32 playerId, WorldTile2 originTile, int32 radius, CutTreeEnum cutTreeEnum)
	{
		TileArea area(originTile, radius);
		area.EnforceWorldLimit();

		// Try finding small trees first...
		NonWalkableTileAccessInfo resultAccessInfo = GetNourishableTreeWithAge(originTile, radius, playerId, 50, cutTreeEnum);

		// If there is no more (all nourished, nourish larger trees)
		if (!resultAccessInfo.isValid()) {
			resultAccessInfo = GetNourishableTreeWithAge(originTile, radius, playerId, 80, cutTreeEnum);
		}

		return resultAccessInfo;
	}

	NonWalkableTileAccessInfo FindTreePlantableSpot(int32 playerId, WorldTile2 originTile, int32_t radius)
	{
		const int32 regionDistance = 1;

		NonWalkableTileAccessInfo resultAccessInfo = NonWalkableTileAccessInfoInvalid;
		AlgorithmUtils::FindNearbyAvailableTile(originTile, [&](WorldTile2 tile)
		{
			int32 tileId = tile.tileId();
			if (!IsReserved(tileId) &&
				!treeShade(tileId) &&
				CanPlantTreeOrBushInGrid(tile) &&
				WorldTile2::Distance(originTile, tile) <= radius)
			{
				// Ensure that this tree is accessible
				NonWalkableTileAccessInfo accessInfo = _simulation->TryAccessNonWalkableTile(originTile, tile, regionDistance, true);
				if (accessInfo.isValid()) {
					resultAccessInfo = accessInfo;
					return true;
				}
				
				//if (!treeShade(tileId) &&
				//	CanPlantTreeOrBushInGrid(tile) &&
				//	WorldTile2::Distance(originTile, tile) <= radius) 
				//{
				//	// Ensure that this tree is accessible
				//	NonWalkableTileAccessInfo accessInfo = _simulation->TryAccessNonWalkableTile(originTile, tile, regionDistance, true);
				//	if (accessInfo.isValid()) {
				//		resultAccessInfo = accessInfo;
				//		return true;
				//	}
				//}
			}

			return false;
		}, 5000);
		
		return resultAccessInfo;
	}

	std::unordered_map<int16_t, FallenTreeInfo>& localTileIdToFallenTree(int regionId) { return _regionToLocalTileIdToFallenTreeInfo[regionId]; }

	//std::vector<FloatDet> testPerlin;

	void CheckMemoryValidity();

	// Debug
	bool isOnReadyBushesList(WorldTile2 tile) { return _regionToReadyBushes[tile.regionId()].Get(tile.localTileId()); }
	bool hasReservation(WorldTile2 tile) { return _reservations.Contains(tile.tileId()); }

	//! Helpers
	int32 GetTreeCount(TileArea area) {
		int32 treeCount = 0;
		area.ExecuteOnArea_WorldTile2([&](WorldTile2 tile) {
			if (tileInfo(tile.tileId()).type == ResourceTileType::Tree) {
				treeCount++;
			}
		});
		return treeCount;
	};


	void SerializeBase(FArchive &Ar, TArray<uint8>& data, std::vector<int32>* crcs, std::vector<std::string>* crcLabels)
	{
		Ar << _sizeX;
		Ar << _sizeXY;
		Ar << _treeCount;
		Ar << _stoneCount;
		Ar << _grassCount;
		
		{
			SERIALIZE_TIMER("Tree - _regionTo", data, crcs, crcLabels);
			SerializeVecObj(Ar, _regionToReadyFruits);
			SerializeVecObj(Ar, _regionToReadyBushes);
			SerializeVecMapObj(Ar, _regionToLocalTileIdToFallenTreeInfo);
			SerializeVecValue(Ar, _regionToGrassCount);
		}

		{
			SERIALIZE_TIMER("Tree - _reservations", data, crcs, crcLabels);
			_reservations >> Ar;
		}

		{
			SERIALIZE_TIMER("Tree - _regionTo", data, crcs, crcLabels);
			SerializeVecObj(Ar, _regionToMarkTileIds);
		}
	}

	void Serialize(FArchive &Ar, TArray<uint8>& data, std::vector<int32>* crcs, std::vector<std::string>* crcLabels)
	{
		SerializeBase(Ar, data, crcs, crcLabels);

		{
			SERIALIZE_TIMER("Tree - _treeEnum", data, crcs, crcLabels);
			SerializeVecValue(Ar, _treeEnum);
		}
		{
			SERIALIZE_TIMER("Tree - _tileObjAge", data, crcs, crcLabels);
			//SerializeVecValue(Ar, _tileObjAge);

			std::vector<int32> tileObjAgeCompressed;
			if (Ar.IsSaving()) {
				for (size_t i = 0; i < _treeEnum.size(); i++) {
					if (_treeEnum[i] != TileObjEnum::None) {
						tileObjAgeCompressed.push_back(_tileObjAge[i]);
					}
				}
			}
			SerializeVecValue(Ar, tileObjAgeCompressed);
			if (Ar.IsLoading()) {
				int32 compressIndex = 0;
				for (size_t i = 0; i < _treeEnum.size(); i++) {
					if (_treeEnum[i] != TileObjEnum::None) {
						_tileObjAge[i] = tileObjAgeCompressed[compressIndex++];
					}
				}
				PUN_CHECK(compressIndex == tileObjAgeCompressed.size());
			}
		}
		{
			SERIALIZE_TIMER("Tree - _treeShade", data, crcs, crcLabels);
			SerializeVecValue(Ar, _treeShade);
		}
	}

	void SerializeForMainMenu(FArchive &Ar, const std::vector<int32>& sampleRegionIds, TArray<uint8>& data)
	{
		SerializeBase(Ar, data, nullptr, nullptr);

		_treeEnum.resize(GameMapConstants::TilesPerWorld);
		_tileObjAge.resize(GameMapConstants::TilesPerWorld);
		_treeShade.resize(GameMapConstants::TilesPerWorld);

		_LOG(PunSaveLoad, "SerializeForMainMenu TilesPerWorld:%d _treeEnumSize:%llu sampleRegionIds:%llu", GameMapConstants::TilesPerWorld, _treeEnum.size(), sampleRegionIds.size());
		
		// Only save sample regions
		for (int32 sampleId : sampleRegionIds) {
			WorldRegion2(sampleId).ExecuteOnRegion_WorldTile([&](WorldTile2 tile) {
				int32 tileId = tile.tileId();
				Ar << _treeEnum[tileId];
				Ar << _tileObjAge[tileId];

				// _treeShade
				int32 int32Val = 0;
				if (Ar.IsSaving()) int32Val = treeShade(tileId);
				Ar << int32Val;
				if (Ar.IsLoading()) setTreeShade(tileId, int32Val);
			});
		}
	}
	
private:
	bool IsValidTree_Helper(int32 tileId, CutTreeEnum cutTreeEnum)
	{
		TileObjInfo info = tileInfo(tileId);
		bool isValidTree = info.type == ResourceTileType::Tree;

		if (cutTreeEnum == CutTreeEnum::NonFruitTreeOnly) {
			if (info.IsFruitBearer()) {
				return false;
			}
		}
		else if (cutTreeEnum == CutTreeEnum::FruitTreeOnly) {
			if (!info.IsFruitBearer()) {
				return false;
			}
		}
		
		return isValidTree;
	}
	
	NonWalkableTileAccessInfo GetCuttableTreeWithAge(WorldTile2 originTile, int32 radius, int32 playerId, int32 minGrowthPercent, CutTreeEnum cutTreeEnum = CutTreeEnum::Any)
	{
		const int32 regionDistance = 1;

		NonWalkableTileAccessInfo resultAccessInfo = NonWalkableTileAccessInfoInvalid;
		AlgorithmUtils::FindNearbyAvailableTile(originTile, [&](WorldTile2 tile)
		{
			int32 tileId = tile.tileId();
			if (!IsReserved(tileId))
			{
				if (IsValidTree_Helper(tileId, cutTreeEnum) &&
					growthPercent(tileId) >= minGrowthPercent &&
					WorldTile2::Distance(originTile, tile) <= radius &&
					_simulation->tileOwner(tile) == playerId)
				{
					// Ensure that this tree is accessible
					NonWalkableTileAccessInfo accessInfo = _simulation->TryAccessNonWalkableTile(originTile, tile, regionDistance, true);
					if (accessInfo.isValid()) {
						resultAccessInfo = accessInfo;
						return true;
					}
				}
				//_simulation->DrawLine(tile.worldAtom2(), FVector::ZeroVector, tile.worldAtom2(), FVector(0, 0, 1), FLinearColor(1, 1, 0));
			}
			else {
				//_simulation->DrawLine(tile.worldAtom2(), FVector::ZeroVector, tile.worldAtom2(), FVector(0, 0, 1), FLinearColor(1, 0, 0));
			}
			
			return false;
		}, 5000);

		return resultAccessInfo;
	}
	
	NonWalkableTileAccessInfo GetNourishableTreeWithAge(WorldTile2 originTile, int32 radius, int32 playerId, int32 maxGrowthPercent, CutTreeEnum cutTreeEnum)
	{
		// Note: great thing about this is that the loop may exit early in good cases... is it worth it though??
		const int32 regionDistance = 1;

		NonWalkableTileAccessInfo resultAccessInfo = NonWalkableTileAccessInfoInvalid;
		AlgorithmUtils::FindNearbyAvailableTile(originTile, [&](WorldTile2 tile)
		{
			int32 tileId = tile.tileId();

			if (IsValidTree_Helper(tileId, cutTreeEnum) &&
				growthPercent(tileId) <= maxGrowthPercent &&
				WorldTile2::Distance(originTile, tile) <= radius &&
				_simulation->tileOwner(tile) == playerId)
			{
				// Ensure that this tree is accessible
				NonWalkableTileAccessInfo accessInfo = _simulation->TryAccessNonWalkableTile(originTile, tile, regionDistance, true);
				if (accessInfo.isValid()) {
					resultAccessInfo = accessInfo;
					return true;
				}
			}

			return false;
		}, 5000);

		return resultAccessInfo;
	}
	
	void RemoveMark(int32 tileId) 
	{ 
		_simulation->SetNeedDisplayUpdate(DisplayClusterEnum::Trees, WorldTile2(tileId).regionId(), true);
		
		_regionToMarkTileIds[WorldTile2(tileId).regionId()].Set(WorldTile2(tileId).localTileId(), false);
		//for (int32 i = size - 1; i >= 0; i--) {
		//	auto& regionToMarkedTileIds = _regionToMarkTileIds[i];
		//	//PUN_DEBUG_FAST(FString::Printf(TEXT("RemoveMarks playerId: %d size:%lu size2:%lu"), i, _playerIdToRegionToMarkedTileIds.size(), regionToMarkedTileIds.size()));
		//	_regionToMarkTileIds[WorldTile2(tileId).regionId()].Set(WorldTile2(tileId).localTileId(), false);
		//}
	}

	//void CheckAddPlayer(int32_t playerId) {
	//	if (playerId >= _playerIdToRegionToMarkTileIds.size()) {
	//		_playerIdToRegionToMarkTileIds.resize(playerId + 1);
	//		for (auto& regionToMarkedTileIds : _playerIdToRegionToMarkTileIds) {
	//			regionToMarkedTileIds.resize(GameMapConstants::TotalRegions);
	//			
	//			for (int i = 0; i < GameMapConstants::TotalRegions; i++) {
	//				regionToMarkedTileIds[i].Init(WorldRegion2(i));
	//			}
	//		}
	//	}
	//}

	// Is tree already dead??
	bool IsAliveTree(int32_t tileId) { 
		return tileInfo(tileId).type == ResourceTileType::Tree && _tileObjAge[tileId] >= 0; 
	}

private:

	void setTreeShade(int32 id, bool boolVal) {
		if (0 <= id && id < _sizeXY) {
			uint32 shift = id & 0b111;
			if (boolVal) {
				_treeShade[id >> 3] |= (1 << shift);
			}
			else {
				_treeShade[id >> 3] &= ~(1 << shift);
			}
		}
	}
	void SetSurroundingShade(int32 i)
	{
		setTreeShade(i, true);

		setTreeShade(i + 1, true);
		setTreeShade(i - 1, true);
		setTreeShade(i + _sizeX, true);
		setTreeShade(i - _sizeX, true);

		setTreeShade(i + 1 + _sizeX, true);
		setTreeShade(i - 1 + _sizeX, true);
		setTreeShade(i + 1 - _sizeX, true);
		setTreeShade(i - 1 - _sizeX, true);
	}

	void RemoveFruit(WorldTile2 tile) {
		_regionToReadyFruits[tile.regionId()].Set(tile.localTileId(), false);
		//_regionToFruitLocalTileIds[tile.regionId()].erase(tile.localTileId()); 
	}
	void AddFruit(WorldTile2 tile) {
		_regionToReadyFruits[tile.regionId()].Set(tile.localTileId(), true);
		//_regionToFruitLocalTileIds[tile.regionId()].insert(tile.localTileId()); 
	}

	void PlantInitial();

	void EmitSeed(int32 originId, TileObjInfo info);

	ResourcePair RemoveTileObj(WorldTile2 tile, bool animate = true);

	ResourcePair TrimBush(WorldTile2 tile, int32 trimEfficiency = 100);

private:
	IGameSimulationCore* _simulation = nullptr;

	/*
	 * Serialize
	 */
	int _sizeX = 0;
	int _sizeXY = 0;
	int _treeCount = 0;
	int _stoneCount = 0;
	int _grassCount = 0;
	// 
	std::vector<TileObjEnum> _treeEnum;
	std::vector<int32> _tileObjAge; // Negative age means fell tree. Its age is kept the same (so proper size can be displayed)
	std::vector<uint8> _treeShade;

	std::vector<FastRegionBoolMap> _regionToReadyFruits;
	std::vector<FastRegionBoolMap> _regionToReadyBushes;
	std::vector<std::unordered_map<int16, FallenTreeInfo>> _regionToLocalTileIdToFallenTreeInfo; // the tick when tree fell for animation

	// TODO: use 8x8??
	std::vector<int32> _regionToGrassCount; // 3 grass = 1 bush

	SpatialHashMap<UnitReservation> _reservations;

	//std::vector<std::vector<FastRegionBoolMap>> _playerIdToRegionToMarkTileIds;
	std::vector<FastRegionBoolMap> _regionToMarkTileIds;
	
public:
	//! Debug...

};
