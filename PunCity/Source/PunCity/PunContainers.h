// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "PunCity/Simulation/GameSimulationInfo.h"
#include "PunCity/PunUtils.h"
#include "Serialization/BufferArchive.h"


// Keep things in 16x16 buckets
// -Less space
// -Execute in region
// -Can do find
// -Fast Add / Ok Remove
//
// Compare to 32x32 ... this has...
// -Faster remove when there are lots of items
// -More space
// -Less items to go through on Find
//
// .. !!!raw value only, no obj..

struct WorldSubregion
{
	int32 x;
	int32 y;

	WorldSubregion(int32 x, int32 y) : x(x), y(y) {}
};

template<typename T>
class SubregionLists
{
public:
	struct SubregionListPair
	{
		WorldTile2 tile;
		T value;

		SubregionListPair() : tile(WorldTile2::Invalid), value(0) {}
		SubregionListPair(WorldTile2 tile, T value) : tile(tile), value(value) {}

		//! Serialize
		FArchive& operator>>(FArchive &Ar) {
			tile >> Ar;
			Ar << value;
			return Ar;
		}
	};

	//template<typename Func>
	//T FindNearest(WorldTile2 tile, int32_t radius, Func condition) {

	//}

	void Init() {
		_subregionToValues.resize(GameMapConstants::TotalRegions * 4);
		_subregionsPerWorldX = GameMapConstants::RegionsPerWorldX * 2;
	}

	void Add(WorldTile2 tile, T value) {
		_subregionToValues[(tile.x / 16) + (tile.y / 16) * _subregionsPerWorldX].push_back(SubregionListPair(tile, value));

		//UE_LOG(LogTemp, Error, TEXT("Subregion Add %s"), *tile.To_FString());
	}

	void Remove(WorldTile2 tile, T value) {
		auto& pairs = _subregionToValues[(tile.x / 16) + (tile.y / 16) * _subregionsPerWorldX];
		for (int i = pairs.size(); i-- > 0;) {
			if (pairs[i].value == value) {
				pairs.erase(pairs.begin() + i);

				return;
			}
		}
		UE_DEBUG_BREAK();
	}

	void TryMove(WorldTile2 lastTile, WorldTile2 tile, T value) {
		if (!IsSameSubregion(lastTile, tile)) {
			Remove(lastTile, value);
			Add(tile, value);
		}
	}

	int32 subregionsPerWorldX() const { return _subregionsPerWorldX; }
	int32 SubregionCount(int32 subregionX, int32 subregionY) const {
		return _subregionToValues[subregionX + subregionY * _subregionsPerWorldX].size();
	}

	int32 SubregionCount(WorldSubregion subregion) const {
		return _subregionToValues[subregion.x + subregion.y * _subregionsPerWorldX].size();
	}
	int32 SubregionCount(WorldTile2 tile) const {
		WorldSubregion subregion = GetSubregion(tile);
		return _subregionToValues[subregion.x + subregion.y * _subregionsPerWorldX].size();
	}
	static WorldSubregion GetSubregion(WorldTile2 tile) {
		return WorldSubregion(tile.x / 16, tile.y / 16);
	}
	
	template<typename Func>
	void ExecuteSubregion(int32 subregionX, int32 subregionY, Func func) const
	{
		int32 subregionId = subregionX + subregionY * _subregionsPerWorldX;
		auto& pairs00 = _subregionToValues[subregionId];
		for (int i = pairs00.size(); i-- > 0;) {
			func(pairs00[i].value);
		}
	}

	template<typename Func>
	void ExecuteRegion(WorldRegion2 region, Func func) const
	{
		int32 subregionId = (region.x * 2) + (region.y * 2) * _subregionsPerWorldX;
		auto& pairs00 = _subregionToValues[subregionId];
		for (int i = pairs00.size(); i-- > 0;) {
			func(pairs00[i].value);
		}

		subregionId++;
		auto& pairs10 = _subregionToValues[subregionId];
		for (int i = pairs10.size(); i-- > 0;) {
			func(pairs10[i].value);
		}

		subregionId += _subregionsPerWorldX;
		auto& pairs11 = _subregionToValues[subregionId];
		for (int i = pairs11.size(); i-- > 0;) {
			func(pairs11[i].value);
		}

		subregionId--;
		auto& pairs01 = _subregionToValues[subregionId];
		for (int i = pairs01.size(); i-- > 0;) {
			func(pairs01[i].value);
		}
	}

	//template<typename Func>
	//void ExecuteRegion_WithTile(WorldRegion2 region, Func func)
	//{
	//	int32_t subregionId = (region.x * 2) + (region.y * 2) * _subregionsPerWorldX;
	//	auto& pairs00 = _subregionToValues[subregionId];
	//	for (int i = pairs00.size(); i-- > 0;) {
	//		func(pairs00[i]);
	//	}

	//	subregionId++;
	//	auto& pairs10 = _subregionToValues[subregionId];
	//	for (int i = pairs10.size(); i-- > 0;) {
	//		func(pairs10[i]);
	//	}

	//	subregionId += _subregionsPerWorldX;
	//	auto& pairs11 = _subregionToValues[subregionId];
	//	for (int i = pairs11.size(); i-- > 0;) {
	//		func(pairs11[i]);
	//	}

	//	subregionId--;
	//	auto& pairs01 = _subregionToValues[subregionId];
	//	for (int i = pairs01.size(); i-- > 0;) {
	//		func(pairs01[i]);
	//	}
	//}
	//

	void Serialize(FArchive& Ar)
	{
		Ar << _subregionsPerWorldX;
		SerializeVecVecObj(Ar, _subregionToValues);
	}

private:
	bool IsSameSubregion(WorldTile2 tile1, WorldTile2 tile2) {
		return ((tile1.x / 16) == (tile2.x / 16)) && ((tile1.y / 16) == (tile2.y / 16));
	}

private:
	int32_t _subregionsPerWorldX = -1;
	std::vector<std::vector<SubregionListPair>> _subregionToValues;
};

// Fast: Add, Get, Remove, Loop
// std's maps/set's hash implementation can differ leading to non-deterministic results...
// Expected usage in trees with just bool store...
// Just 32-bit uint as a row bucket. Very fast lookup by checking if uint is 0
class FastRegionBoolMap
{
public:
	void Init(WorldRegion2 region) {
		_region = region;
		_minXTile = _region.minXTile();
		_minYTile = _region.minYTile();
		ResetToFalse();
	}

	void ResetToFalse() {
		for (int i = 0; i < 32; i++) {
			_packedBools[i] = 0;
		}
	}

	bool Get(int32 localId)
	{
		check(localId >= 0 && localId < 1024);

		uint32 x = localId % 32;
		uint32 y = localId / 32;
		return (_packedBools[y] >> x) & 1;
	}

	void Set(int32 localId, bool value)
	{
		check(localId >= 0 && localId < 1024);

		uint32 x = localId % 32;
		uint32 y = localId / 32;
		uint32 mask = 1 << x;
		if (value) {
			_packedBools[y] |= mask;
		} else {
			_packedBools[y] &= ~mask;
		}
	}

	template <typename Func>
	void Execute(Func func) {
		for (int y = 0; y < 32; y++) {
			if (_packedBools[y] > 0)
			{
				uint32_t packedBool = _packedBools[y];
				for (int x = 0; x < 32; x++) { // Probably gets unrolled into 32 functions by compiler??
					if (packedBool & 1) {
						func(WorldTile2(x + _minXTile, y + _minYTile));
					}
					packedBool >>= 1;
				}
			}
		}
	}

	//! Serialize
	FArchive& operator>>(FArchive &Ar) {
		_region >> Ar;
		Ar << _minXTile;
		Ar << _minYTile;

		Ar.Serialize(_packedBools, sizeof(_packedBools));
		return Ar;
	}

private:
	WorldRegion2 _region;
	int16 _minXTile = 0;
	int16 _minYTile = 0;

	uint32 _packedBools[32];
};



// Motivation:
// - Fast: Add, Get, Remove, loop ... Without storing too much data..
// - std's maps/set's hash implementation can differ leading to non-deterministic results...

//template<typename T>
//class CycleMap
//{
//public:
//	struct Pair
//	{
//		int32_t key;
//		T value;
//		//int32_t loopIndex;
//
//		Pair(int32_t key, T value) : key(key), value(value) {}
//	};
//
//	struct Bucket
//	{
//		std::vector<Pair> pairs;
//
//		// Want to automatically despawn things that weren't added the frame before...
//		// Avoid having to set whole vector to false BeforeAdd ... odd update... use 1 as signal that the component was updated .. even update... use 0
//		std::vector<bool> ticks;
//	};
//
//	void Init(int32_t size) {
//		_bucketCount = size;
//		_count = 0;
//		_buckets.resize(_bucketCount);
//
//		// Optimize bucket loop lookup by giving bucket a tag if its size is 0
//		_bucketHasItems.resize(_bucketCount);
//
//		//_loopValues.clear();
//
//
//	}
//
//	int32_t count() { return _count; }
//
//	void Add(int32_t key, T value) {
//		//// Add to loopValues
//		//int32_t loopIndex;
//		//if (_disabledIndices.empty()) {
//		//	loopIndex = _loopValues.size();
//		//	_loopValues.push_back(value);
//		//	_isAlive.push_back(true);
//		//}
//		//else {
//		//	loopIndex = _disabledIndices.back();
//		//	_disabledIndices.pop_back();
//		//	_loopValues[loopIndex] = value;
//		//	_isAlive[loopIndex] = true;
//		//}
//
//		auto& bucket = _buckets[key % _bucketCount];
//		bucket.pairs.push_back(Pair(key, value));
//		bucket.ticks.push_back(Time::Ticks() & 1);
//
//		_bucketHasItems[key % _bucketCount] = true;
//
//		_count++;
//	}
//
//	T Get(int32_t key) {
//		auto& bucket = _buckets[key % _bucketCount];
//		for (size_t i = bucket.pairs.size(); i-- > 0;) {
//			if (bucket.pairs[i].key == key) {
//				return bucket.pairs[i].value;
//			}
//		}
//		UE_DEBUG_BREAK();
//		return bucket.pairs[0].value;
//	}
//
//	bool TryGet(int32_t key, T& value) {
//		auto& bucket = _buckets[key % _bucketCount];
//		for (size_t i = bucket.pairs.size(); i-- > 0;) {
//			if (bucket.pairs[i].key == key) {
//				value = bucket.pairs[i].value;
//				return true;
//			}
//		}
//		return false;
//	}
//
//	void SetInUse(int32_t key, const T& value) {
//
//		auto& bucket = _buckets[key % _bucketCount];
//		for (size_t i = bucket.pairs.size(); i-- > 0;) {
//			if (bucket.pairs[i].key == key) {
//				bucket.pairs[i].value = value;
//				bucket.ticks[i] = Time::Ticks() & 1;
//				return;
//			}
//		}
//		UE_DEBUG_BREAK();
//	}
//
//	template<typename Func>
//	void Loop(Func func) {
//		//	for (size_t i = _loopValues.size(); i-- > 0;) {
//		//		if (_isAlive[i]) {
//		//			func(_loopValues[i]);
//		//		}
//		//	}
//	}
//
//	//template<typename Func>
//	//void LoopRemove(Func shouldRemoveFunc) {
//	//	for (size_t i = _bucketHasItems.size(); i-- > 0;) {
//	//		if (_bucketHasItems[i]) 
//	//		{
//	//			auto& bucket = _buckets[i];
//	//			for (size_t j = bucket.size(); j-- > 0;) {
//	//				if (shouldRemoveFunc(bucket[j])) {
//	//					//FixedMapPair& mapPair = bucket[j];
//	//					//_disabledIndices.push_back(mapPair.loopIndex);
//	//					//_isAlive[mapPair.loopIndex] = false;
//
//	//					bucket.erase(bucket.begin() + j);
//	//				}
//	//			}
//
//	//			if (bucket.empty()) {
//	//				_bucketHasItems[i] = false;
//	//			}
//	//		}
//	//	}
//	//}
//
//	void RemoveUnused(std::vector<T>& unused)
//	{
//		for (size_t i = _bucketHasItems.size(); i-- > 0;) {
//			if (_bucketHasItems[i])
//			{
//				auto& bucket = _buckets[i];
//				for (size_t j = bucket.ticks.size(); j-- > 0;) {
//					if (bucket.ticks[j] != (Time::Ticks() & 1)) {
//						unused.push_back(bucket.pairs[j].value);
//						bucket.pairs.erase(bucket.pairs.begin() + j);
//						bucket.ticks.erase(bucket.ticks.begin() + j);
//					}
//				}
//
//				if (bucket.pairs.empty()) {
//					_bucketHasItems[i] = false;
//				}
//			}
//		}
//	}
//
//	void Clear() {
//		for (size_t i = _bucketHasItems.size(); i-- > 0;) {
//			if (_bucketHasItems[i]) {
//				auto& bucket = _buckets[i];
//				bucket.pairs.clear();
//				bucket.ticks.clear();
//			}
//		}
//		_bucketHasItems.clear();
//
//		//_loopValues.clear();
//		//_isAlive.clear();
//		//_disabledIndices.clear();
//
//		_count = 0;
//	}
//
//private:
//	int32_t _bucketCount;
//	int32_t _count;
//	std::vector<Bucket> _buckets;
//
//	std::vector<bool> _bucketHasItems;
//
//	//// Looping through all buckets including those not used is too expensive...
//	//std::vector<T> _loopValues;
//	//std::vector<bool> _isAlive;
//	//std::vector<int32_t> _disabledIndices;
//};