// Fill out your copyright notice in the Description page of Project Settings.


#include "ProjectileSystem.h"
#include "UnitSystem.h"

using namespace std;

ProjectileDisplayInfo ProjectileArrow::GetProjectileDisplayInfo() const
{
	if (!_simulation->unitAlive(targetId)) {
		return { WorldAtom2::Invalid, WorldAtom2::Invalid, -1, -1, -1, -1};
	}
	UnitSystem& unitSystem = _simulation->unitSystem();

	// launch/arrival ticks is just lastUpdate and nextUpdate of this object
	int32_t launchTick = _launchTick;// unitSystem.lastActiveTick(_id);
	int32_t arrivalTick = unitSystem.nextActiveTick(_id);
	int32_t fraction100000 = static_cast<int64_t>(Time::Ticks() - launchTick) * 100000 / (arrivalTick - launchTick);
	WorldAtom2 targetLocation = unitSystem.actualAtomLocation(targetId.id);
	WorldAtom2 currentLocation = WorldAtom2::Lerp(_launchAtom, targetLocation, fraction100000);

	//PUN_LOG("GetProjectileDisplayInfo target %s , current:%s", *targetLocation.worldTile2().To_FString(), *currentLocation.worldTile2().To_FString());
	//PUN_LOG("GetProjectileDisplayInfo %f", fraction100000 / 100000.0f);
	return { currentLocation, targetLocation, fraction100000, GroundSpeed, (arrivalTick - launchTick),  _travelDistance };
}

void ProjectileArrow::Launch(UnitFullId ownerIdIn, int32 ownerWorkplaceIdIn, UnitFullId targetIdIn, int32 damageIn) {
	ownerId = ownerIdIn;
	targetId = targetIdIn;
	damage = damageIn;
	ownerWorkplaceId = ownerWorkplaceIdIn;

	PUN_CHECK2(_simulation->unitAlive(targetIdIn), debugStr());

	_launchAtom = _unitData->atomLocation(_id);
	_launchTick = Time::Ticks();

	_travelDistance = WorldAtom2::Distance(_unitData->actualAtomLocation(targetId.id), _launchAtom);
	int32_t ticksNeeded = _travelDistance / GroundSpeed;

	// Make sure that tick is more than 0
	ticksNeeded = ticksNeeded > 0 ? ticksNeeded : 1;

	// Wait for arrow to hit and trigger CalculateActions
	Add_Wait(ticksNeeded);

	AddDebugSpeech("(Success)Launching: " + to_string(ticksNeeded));
	NextAction(ticksNeeded, UnitUpdateCallerEnum::LaunchArrow);
}


void ProjectileArrow::CalculateActions()
{
	//PUN_LOG("Arrow CalculateActions");
	if (_simulation->unitAlive(targetId)) {
		_simulation->unitAI(targetId.id).AttackIncoming(ownerId, ownerWorkplaceId, damage, -1, targetId);
	}

	Die();
}