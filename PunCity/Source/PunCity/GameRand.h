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
	//static void SetRandState(uint32_t randStateIn);

	static int32_t Rand100RoundTo1(int32_t value100) {
		int32_t value1 = (value100 / 100);
		if ((value100 % 100) > (Rand() % 100)) {
			value1++;
		}
		return value1;
	}

	static int32_t RandRound(int32_t numerator, int32_t denominator) {
		int32_t value = numerator / denominator;
		int32_t remainder = numerator - value * denominator;
		if (remainder > (Rand() % denominator)) {
			value++;
		}
		return value;
	}

	static int32_t RandRound(int32_t numerator, int32_t denominator, int32_t seed) {
		int32_t value = numerator / denominator;
		int32_t remainder = numerator - value * denominator;
		if (remainder > (Rand(Rand(seed)) % denominator)) { // Double Rand() to ensure more randomness from seed
			value++;
		}
		return value;
	}

	static bool RandChance(int32_t chance) { return Rand() % chance == 0; }

	// Helpers

	static int32_t RandFluctuate(int32_t value, int32_t percent) 	{
		return value * (100 - percent + (Rand() % (percent * 2))) / 100;
	}
};