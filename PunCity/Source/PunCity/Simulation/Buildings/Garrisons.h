// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "../IGameSimulationCore.h"

class Garrisons
{
public:
	void Init(IGameSimulationCore* simulation, int32_t playerId) {
		_simulation = simulation;
		_garrisonedPlayerId = playerId;
	}

	bool AddGarrison(int32_t newArmyId);
	void ReleaseGarrison(int32_t armyId);

	size_t Count() { return _garrisons.size(); }
	int32_t Get(int32_t index) { return _garrisons[index]; }

	int32_t playerId() { return _garrisonedPlayerId; }

private:
	IGameSimulationCore* _simulation = nullptr;

	int32_t _garrisonedPlayerId = -1;
	std::vector<int32_t> _garrisons;
};

//TODO: garrison system??
// armyIdToGarrisonsId
// buildingIdToGarrisonId