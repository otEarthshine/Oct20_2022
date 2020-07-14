// Fill out your copyright notice in the Description page of Project Settings.


#include "TrapSystem.h"
#include "PunCity/CppUtils.h"
#include "UnitStateAI.h"

void TrapSystem::Tick()
{
	//// Only check trap every half second
	//if (Time::Ticks() % 30 == 0) {
	//	auto& unitSystem = _simulation->unitSystem();
	//	auto& unitSubregionLists = unitSystem.unitSubregionLists();

	//	for (int i = 0; i < _regionToTraps.size(); i++) {
	//		auto& traps = _regionToTraps[i];
	//		WorldRegion2 region(i);
	//		for (int j = 0; j < traps.size(); j++) {
	//			unitSubregionLists.ExecuteRegion_WithTile(region, [&](auto& worldTileAndUnitId) {
	//				if (traps[j].tile == worldTileAndUnitId.tile) {
	//					int32_t unitId = worldTileAndUnitId.value;
	//					UnitStateAI& unitAI = unitSystem.unitStateAI(unitId);

	//					// Animal not under player control gets killed
	//					if (IsAnimal(unitSystem.unitEnum(unitId)) && unitAI.playerId() == -1)
	//					{
	//						PUN_LOG("Trap kill %d", unitId);
	//						unitAI.AttackIncoming(UnitFullId::Invalid(), 100);
	//					}
	//				}
	//			});
	//		}
	//	}

	//}
}

bool TrapSystem::TryAddTrap(WorldTile2 tile)
{
	TrapInfo trap;
	trap.tile = tile;
	return CppUtils::TryAdd(_regionToTraps[tile.regionId()], trap);
}