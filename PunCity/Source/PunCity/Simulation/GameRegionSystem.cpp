// Fill out your copyright notice in the Description page of Project Settings.

#include "GameRegionSystem.h"
#include "PunCity/CppUtils.h"

void GameRegionSystem::RemoveBoarBurrow(int32_t regionId, int32_t buildingId)
{
	check(CppUtils::Contains(_boarBurrowsToRegion[regionId], buildingId));
	CppUtils::Remove(_boarBurrowsToRegion[regionId], buildingId);
}