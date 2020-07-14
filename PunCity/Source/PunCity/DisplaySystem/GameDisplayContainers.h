// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameDisplayInfo.h"

// -Fixed-sized buckets (half-ass between vector/hashmap)
// -No linked list... faster
// -vector bucket allows faster iteration through it
// -consumes a bit more memory, depending on initial allocation

// Motivation:
// Need something like vector, but without limitation on index
// Solution: mod index to go into buckets
// But then looping means looping through empty containers?
// .. So just put all items in vector too, and have FixedMapPair point to that vector.. more cost on add/ more space from FixedMapPair... but a lot faster...
// .. Despawning will only disable items in loop-vector without changing index..

template<typename T>
class CycleMap
{
public:
	struct Pair
	{
		int32_t key;
		T value;
		Pair(int32_t key, T value) : key(key), value(value) {}
	};

	struct Bucket
	{
		std::vector<Pair> pairs;

		// Want to automatically despawn things that weren't added this frame..
		std::vector<int32_t> ticks;
	};

	void Init(int32_t size) {
		_bucketCount = size;
		_count = 0;
		_lastCount = 0;

		_buckets.resize(_bucketCount);

		// Optimize bucket loop lookup by giving bucket a tag if its size is 0
		_bucketHasItems.resize(_bucketCount);
	}

	void Add(int32_t key, T value)
	{
		auto& bucket = _buckets[key % _bucketCount];
		bucket.pairs.push_back(Pair(key, value));
		bucket.ticks.push_back(TimeDisplay::Ticks());

		_bucketHasItems[key % _bucketCount] = true;

		_count++;
	}

	T Get(int32_t key) {
		auto& bucket = _buckets[key % _bucketCount];
		for (size_t i = bucket.pairs.size(); i-- > 0;) {
			if (bucket.pairs[i].key == key) {
				return bucket.pairs[i].value;
			}
		}
		UE_DEBUG_BREAK();
		return bucket.pairs[0].value;
	}

	bool TryGet(int32_t key, T& value)
	{
		auto& bucket = _buckets[key % _bucketCount];

		check(_count == 0 ? bucket.pairs.size() == 0 : true);

		for (size_t i = bucket.pairs.size(); i-- > 0;) {
			if (bucket.pairs[i].key == key) {
				value = bucket.pairs[i].value;
				return true;
			}
		}
		return false;
	}

	void SetInUse(int32_t key, const T& value)
	{
		auto& bucket = _buckets[key % _bucketCount];
		for (size_t i = bucket.pairs.size(); i-- > 0;) {
			if (bucket.pairs[i].key == key) {
				bucket.pairs[i].value = value;
				bucket.ticks[i] = TimeDisplay::Ticks();
				return;
			}
		}
		UE_DEBUG_BREAK();
	}

	void RemoveUnused(std::vector<T>& unused)
	{
		if (_count == 0 && _lastCount == 0) {
			return;
		}
		_lastCount = _count;

		for (size_t i = _bucketHasItems.size(); i-- > 0;) {
			if (_bucketHasItems[i])
			{
				auto& bucket = _buckets[i];
				for (size_t j = bucket.ticks.size(); j-- > 0;) {
					//PUN_ALOG("FastMesh", bucket.pairs[j].key, ".FastMesh..RemoveUnused.Loop ticks:%d id:%d", TimeDisplay::Ticks(), bucket.pairs[j].key);

					if (bucket.ticks[j] != TimeDisplay::Ticks()) 
					{
						//PUN_ALOG("FastMesh", bucket.pairs[j].key, ".FastMesh..RemoveUnused.Remove ticks:%d id:%d", TimeDisplay::Ticks(), bucket.pairs[j].key);

						unused.push_back(bucket.pairs[j].value);
						bucket.pairs.erase(bucket.pairs.begin() + j);
						bucket.ticks.erase(bucket.ticks.begin() + j);
						_count--;
					}
				}

				if (bucket.pairs.empty()) {
					_bucketHasItems[i] = false;
				}
			}
		}
	}

	void Clear()
	{
		if (_count == 0 && _lastCount == 0) {
			return;
		}

		for (size_t i = _bucketHasItems.size(); i-- > 0;) {
			if (_bucketHasItems[i]) {
				auto& bucket = _buckets[i];
				bucket.pairs.clear();
				bucket.ticks.clear();
			}
			_bucketHasItems[i] = false;

			check(_buckets[i].pairs.empty());
			check(_buckets[i].ticks.empty());
		}

		_count = 0;
		_lastCount = 0;
	}

	//! Debug
	int32_t InUseTick(int32_t key) {
		auto& bucket = _buckets[key % _bucketCount];
		for (size_t i = bucket.pairs.size(); i-- > 0;) {
			if (bucket.pairs[i].key == key) {
				return bucket.ticks[i];
			}
		}
		UE_DEBUG_BREAK();
		return -1;
	}
	std::vector<int32_t> GetAll() {
		std::vector<int32_t> results;
		for (size_t i = _bucketHasItems.size(); i-- > 0;) {
			if (_bucketHasItems[i]) {
				auto& bucket = _buckets[i];
				for (size_t j = bucket.ticks.size(); j-- > 0;) {
					results.push_back(bucket.pairs[j].key);
				}
			}
		}
		return results;
	}

	int32_t count() { return _count; }

private:
	int32_t _bucketCount;
	int32_t _count;
	int32_t _lastCount;

	std::vector<Bucket> _buckets;
	std::vector<bool> _bucketHasItems;
};