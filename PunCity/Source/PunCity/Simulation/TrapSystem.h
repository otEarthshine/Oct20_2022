// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "IGameSimulationCore.h"

struct TrapInfo
{
	WorldTile2 tile;

	bool operator==(const TrapInfo& a) const {
		return tile == a.tile;
	}
};

/**
 * !!!Might not be needed???
 */
class TrapSystem
{
public:
	void Init(IGameSimulationCore* simulation) {
		_simulation = simulation;
		_regionToTraps.resize(GameMapConstants::TotalRegions);
	}

	void Tick();

	bool TryAddTrap(WorldTile2 tile);

private:
	IGameSimulationCore* _simulation;
	std::vector<std::vector<TrapInfo>> _regionToTraps;

};
