// Fill out your copyright notice in the Description page of Project Settings.


#include "Garrisons.h"
#include "PunCity/CppUtils.h"
#include "../UnitStateAI.h"

bool Garrisons::AddGarrison(int32_t newArmyId)
{
	UnitStateAI& newArmyAI = _simulation->unitAI(newArmyId);

	// Fight it out...
	if (_garrisonedPlayerId != newArmyAI.playerId() && _garrisons.size() > 0)
	{
		// Eliminate both armies...
		newArmyAI.Die();
		_simulation->unitAI(_garrisons.back()).Die();
		return false;
	}

	_garrisonedPlayerId = newArmyId;
	_garrisons.push_back(newArmyId);
	return true;
}
void Garrisons::ReleaseGarrison(int32_t armyId) {
	CppUtils::Remove(_garrisons, armyId);
}