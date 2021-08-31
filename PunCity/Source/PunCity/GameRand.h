#pragma once

#include <stdint.h>

class GameRandObj
{
public:
	GameRandObj() {
		_seed = 1;
		_randState = 1;
	}
	GameRandObj(uint32_t seed) {
		_seed = seed;
		_randState = seed;
	}

	int32_t Rand() {
		_randState ^= _randState << 13;
		_randState ^= _randState >> 17;
		_randState ^= _randState << 5;
		return _randState >> 1; // Shift back one bit to prevent negative
	}

	uint32_t seed() { return _seed; }

	float Rand01() {
		return (Rand() % 10000) / 10000.0f;
	}
	float RandRange(float low, float high) {
		return low + (high - low) * Rand01();
	}

	static GameRandObj SoundRand;

private:
	uint32_t _seed = 1;
	uint32_t _randState = 1;
};

class GameRand
{
public:
	static int32_t Rand();
	static uint32_t URand();
	static int32_t Rand(uint32_t seed);

	static int32_t DisplayRand(uint32_t seed); // Just for clarity

	static void ResetStateToTickCount(uint32_t tickCount);

	static uint32_t RandState();

	static int32_t RandUsageCount() { return randUsageCount; }
	
	//static void SetRandState(uint32_t randStateIn);

	static int32_t Rand100RoundTo1(int32_t value100)
	{
		check(randUsageValid);
		
		int32_t value1 = (value100 / 100);
		if ((value100 % 100) > (Rand() % 100)) {
			value1++;
		}
		return value1;
	}

	static int32_t RandRound(int32_t numerator, int32_t denominator)
	{
		check(randUsageValid);
		
		int32_t value = numerator / denominator;
		int32_t remainder = numerator - value * denominator;
		if (remainder > (Rand() % denominator)) {
			value++;
		}
		return value;
	}

	static int64_t RandRound64(int64_t numerator, int64_t denominator)
	{
		check(randUsageValid);

		int64_t value = numerator / denominator;
		int64_t remainder = numerator - value * denominator;
		if (remainder > (Rand() % denominator)) {
			value++;
		}
		return value;
	}

	static int32_t RandRound(int32_t numerator, int32_t denominator, int32_t seed)
	{
		check(randUsageValid);
		
		int32_t value = numerator / denominator;
		int32_t remainder = numerator - value * denominator;
		if (remainder > (Rand(Rand(seed)) % denominator)) { // Double Rand() to ensure more randomness from seed
			value++;
		}
		return value;
	}

	static bool RandChance(int32_t chance) { return Rand() % chance == 0; }

	
	//! Debug
	
	static void SetRandUsageValid(bool randUsageValidIn);

	
	//! Helpers
	
	static int32_t RandFluctuate(int32_t value, int32_t percent) 	{
		return value * (100 - percent + (Rand() % (percent * 2))) / 100;
	}

private:
	static bool randUsageValid;

	static int32_t randUsageCount;
};

template<typename T>
static int32 FastHashCombine(int32 lhs, T rhs) {
	lhs ^= static_cast<int32>(rhs) + 0x9e3779b9 + (lhs << 6) + (lhs >> 2);
	return lhs;
}

static int32 FastHashCombineMany(std::vector<int32> hashes)
{
	check(hashes.size() >= 2);
	int32 lhs = hashes[0];
	for (int32 i = 1; i < hashes.size(); i++) {
		lhs ^= hashes[i] + 0x9e3779b9 + (lhs << 6) + (lhs >> 2);
	}
	return lhs;
}