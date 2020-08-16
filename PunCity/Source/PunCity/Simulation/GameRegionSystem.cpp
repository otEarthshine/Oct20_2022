// Fill out your copyright notice in the Description page of Project Settings.

#include "GameRegionSystem.h"
#include "PunCity/CppUtils.h"
#include "UnitSystem.h"
#include "UnitStateAI.h"
#include "SimUtils.h"

void GameRegionSystem::RemoveBoarBurrow(int32_t regionId, int32_t buildingId)
{
	check(CppUtils::Contains(_boarBurrowsToRegion[regionId], buildingId));
	CppUtils::Remove(_boarBurrowsToRegion[regionId], buildingId);
}


void GameRegionSystem::AddAnimalColony(UnitEnum unitEnum, WorldTile2 center, int32 radius, int32 chancePercentMultiplier)
{
	int32 provinceId = _simulation->GetProvinceIdClean(center);
	if (provinceId == -1) {
		return;
	}

	auto& terrainGen = _simulation->terrainGenerator();

	std::vector<UnitFullId> unitIds;
	int32 random = center.tileId();
	const int32 atomShiftAmount = CoordinateConstants::AtomsPerTile / 3;
	
	SimUtils::PerlinRadius_ExecuteOnArea_WorldTile2(center, radius, _simulation, [&](int32 chancePercent, WorldTile2 tile)
	{
		chancePercent = chancePercent * chancePercentMultiplier / 100;

		int32 random2 = GameRand::Rand(random);
		random = GameRand::Rand(random2);
		if (random % 100 < chancePercent) {
			int32 ageTicks = GameRand::Rand() % GetUnitInfo(unitEnum).maxAgeTicks;

			WorldAtom2 atomShift = WorldAtom2(GameRand::Rand(random) % (atomShiftAmount * 2) - atomShiftAmount,
				GameRand::Rand(random2) % (atomShiftAmount * 2) - atomShiftAmount);
			int32 unitId = _simulation->AddUnit(unitEnum, GameInfo::PlayerIdNone, tile.worldAtom2() + atomShift, ageTicks);
			unitIds.push_back(_simulation->unitSystem().fullId(unitId));

			// Random initial target facing (facing the center should do this)
			WorldAtom2 targetAtom = tile.worldAtom2();
			_simulation->unitSystem().SetTargetLocation(unitId, targetAtom);
		}
	});
	

	//TileArea area(center, radius);

	//area.ExecuteOnArea_WorldTile2([&](WorldTile2 tile)
	//{
	//	if (_simulation->IsBuildable(tile))
	//	{
	//		FloatDet perlinChance = terrainGen.resourcePerlin(tile);
	//		int32 dist = WorldTile2::Distance(tile, center);
	//		FloatDet distChance = FD0_XX(FDMax(100 - dist * 100 / radius, 0));
	//		FloatDet chance = FDMul(perlinChance, distChance);
	//		chance = FDMax(chance, 0);
	//		int32 chancePercent = 100 * chance / FDOne;
	//		
	//		chancePercent = chancePercent * chancePercentMultiplier / 100;

	//		int32 random2 = GameRand::Rand(random);
	//		random = GameRand::Rand(random2);
	//		if (random % 100 < chancePercent) {
	//			int32 ageTicks = GameRand::Rand() % GetUnitInfo(unitEnum).maxAgeTicks;
	//			
	//			WorldAtom2 atomShift = WorldAtom2(GameRand::Rand(random) % (atomShiftAmount * 2) - atomShiftAmount, 
	//												GameRand::Rand(random2) % (atomShiftAmount * 2) - atomShiftAmount);
	//			int32 unitId = _simulation->AddUnit(unitEnum, GameInfo::PlayerIdNone, tile.worldAtom2() + atomShift, ageTicks);
	//			unitIds.push_back(_simulation->unitSystem().fullId(unitId));

	//			// Random initial target facing (facing the center should do this)
	//			WorldAtom2 targetAtom = tile.worldAtom2();
	//			_simulation->unitSystem().SetTargetLocation(unitId, targetAtom);
	//		}
	//	}
	//});

	_animalColonies.push_back({ unitEnum, provinceId, center, radius, unitIds });
}
void GameRegionSystem::RemoveAnimalColony(UnitEnum unitEnum)
{
	for (size_t i = _animalColonies.size(); i-- > 0;) {
		if (_animalColonies[i].unitEnum == unitEnum)
		{
			const std::vector<UnitFullId>& unitIds = _animalColonies[i].units;
			for (UnitFullId unitId : unitIds) {
				if (_simulation->unitAlive(unitId)) {
					_simulation->unitAI(unitId.id).Die();
				}
			}
			_animalColonies.erase(_animalColonies.begin() + i);
			break;
		}
	}
}