#include "PunCity/GameRand.h"

static uint32_t randState = 1;

bool GameRand::randUsageValid = false;

int32_t GameRand::Rand()
{
	check(randUsageValid);
	
	randState ^= randState << 13;
	randState ^= randState >> 17;
	randState ^= randState << 5;
	return randState >> 1; // Shift back one bit to prevent negative
}

uint32_t GameRand::URand()
{
	check(randUsageValid);
	
	randState ^= randState << 13;
	randState ^= randState >> 17;
	randState ^= randState << 5;
	return randState;
}

int32_t GameRand::Rand(uint32_t seed)
{
	seed ^= seed << 13;
	seed ^= seed >> 17;
	seed ^= seed << 5;
	return seed >> 1;
}

int32_t GameRand::DisplayRand(uint32_t seed)
{
	seed ^= seed << 13;
	seed ^= seed >> 17;
	seed ^= seed << 5;
	return seed >> 1;
}

void GameRand::ResetStateToTickCount(uint32_t tickCount)
{
	randState = tickCount + 17877; // Add number since randState close to 0 is bad
}

uint32_t GameRand::RandState() {
	return randState;
}


void GameRand::SetRandUsageValid(bool randUsageValidIn)
{
	randUsageValid = randUsageValidIn;
}

//void GameRand::SetRandState(uint32_t randStateIn) {
//	randState = randStateIn;
//}