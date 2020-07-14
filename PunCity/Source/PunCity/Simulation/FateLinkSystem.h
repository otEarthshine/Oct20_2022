// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "PunCity/PunUtils.h"

/**
 * Spiritual energy, warp
 * 
 * Legend says the side effects of ancient forbidden technology causes random essence warp effect that still happens today.
 * This rift fluctuation causes places far away from each other to be linked, matters randomly transport, and sometimes even portals appear.
 * Most people call this effect "fate link". "rift link?"
 * Cities that become linked can affect each other.
 * 
 */

//Effects:
//- Inquisition
//- Vampires

enum class FateEnum
{
	None,
	//Plague,
	ThiefGuild,
	Rotting,
	Frostbite,
};

struct FateInfo
{
	std::string name;
	std::string descriptionPrefix;
	std::string descriptionSuffix;
};

static const FateInfo FateInfos[] =
{
	{"None", "", ""},
	//{"Plague", "Random death chance, decrease happiness."},
	{"ThiefGuild", "Steal ", "% of current gold per round."},
	{"Rotting", "", "% food rot away per round."},
	{"FrostBite", "Heat decays ","% faster."},
};
static const int32_t FateCount = _countof(FateInfos);

static FateInfo GetFateInfo(FateEnum fateEnum) {
	return FateInfos[static_cast<int>(fateEnum)];
}

struct FateLink
{
	FateEnum fateEnum = FateEnum::None;
	int32_t value = -1;
};

class PlayerFate
{
public:
	PlayerFate() {
		_fateValues.resize(FateCount, 0);
	}

	int32_t fateValue(FateEnum fateEnum) const { return _fateValues[static_cast<int>(fateEnum)];  }
	void ChangeFateValue(FateEnum fateEnum, int32_t value) { _fateValues[static_cast<int>(fateEnum)] += value; }

	std::vector<int8_t> linkedPlayerIds;

private:
	std::vector<int32_t> _fateValues;
};

class FateLinkSystem
{
public:
	void AddPlayer() {
		_playerFate.push_back(PlayerFate());
	}

	void TickRound()
	{
		// Relink fate every round
		for (size_t i = 0; i < _playerFate.size(); i++)
		{
			_playerFate[i].linkedPlayerIds.clear();
			for (size_t j = 0; j < _playerFate.size(); j++) {
				if (i != j) {
					_playerFate[i].linkedPlayerIds.push_back(j);
				}
			}
		}
		needRefreshFateUI = true;
	}

	// Get the cumulative fate effect for this player
	int32_t GetFateEffectFor(int32_t playerId, FateEnum fateEnum)
	{
		PUN_CHECK(playerId != -1);
		int32_t value = 0;
		std::vector<int8_t>& linkedPlayerIds = _playerFate[playerId].linkedPlayerIds;
		for (size_t i = 0; i < linkedPlayerIds.size(); i++) {
			value += _playerFate[linkedPlayerIds[i]].fateValue(fateEnum);
		}
		return value;
	}

	// Change the fate value of this player (this fate affects linked player)
	void ChangeFateValue(int32_t playerId, FateEnum fateEnum, int32_t value) {
		_playerFate[playerId].ChangeFateValue(fateEnum, value);
	}

	const PlayerFate& playerFate(int32_t playerId) { return _playerFate[playerId]; }

	bool needRefreshFateUI = true;
private:

	std::vector<PlayerFate> _playerFate;
};
