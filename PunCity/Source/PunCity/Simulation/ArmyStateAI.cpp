//// Fill out your copyright notice in the Description page of Project Settings.
//
//
//#include "ArmyStateAI.h"
//#include "Buildings/Townhall.h"
//#include "Buildings/GathererHut.h"
//
//using namespace std;
//
//void ArmyStateAI::CalculateActions()
//{
//	PushLastDebugSpeech();
//
//	AddDebugSpeech("--- CalculateActions (Army)");
//
//	if (TryGarrison()) return;
//
//	Add_Wait(60);
//	_unitState = UnitState::Idle;
//}
//
//bool ArmyStateAI::TryGarrison()
//{
//	//if (_targetBuildingId == -1) {
//	//	AddDebugSpeech("(Failed)TryMoveArmy: no target... target:" + to_string(_targetBuildingId) + ", garrison:" + to_string(_garrisonBuildingId));
//	//	return false;
//	//}
//	//if (_targetBuildingId == _garrisonBuildingId) {
//	//	AddDebugSpeech("(Failed)TryMoveArmy: already garrisoned at target:" + to_string(_targetBuildingId) + ", garrison:" + to_string(_garrisonBuildingId));
//	//	return false;
//	//}
//
//	//_actions.push_back(bind(&ArmyStateAI::GarrisonTarget, this, _targetBuildingId));
//	//_actions.push_back(bind(&UnitStateAI::MoveToForceLongDistance, this, _simulation->gateTile(_targetBuildingId)));
//	//_unitState = UnitState::MoveArmy;
//	//AddDebugSpeech("(Succeed)TryMoveArmy:");
//	return true;
//}
//
//void ArmyStateAI::GarrisonTarget(int32_t garrisonTargetId)
//{
//	Building& building = _simulation->building(garrisonTargetId);
//	bool succeed;
//	if (building.isEnum(BuildingEnum::Townhall) || building.isEnum(BuildingEnum::Barrack)) {
//		//succeed = building.garrisons.AddGarrison(_id);
//	} else {
//		succeed = false;
//		UE_DEBUG_BREAK();
//	}
//
//	if (succeed) {
//		_garrisonBuildingId = garrisonTargetId;
//	}
//
//	// Unit might have died in an assault
//	if (!_unitData->alive(_id)) {
//		return;
//	}
//
//	NextAction("GarrisonTarget");
//	AddDebugSpeech("(Done)GarrisonTarget");
//}
//
//void ArmyStateAI::IssueUnitOrderGarrison(int32_t targetBuildingId) {
//	PUN_LOG("IssueUnitOrderGarrison");
//	if (_garrisonBuildingId != -1) {
//		//_simulation->building(_garrisonBuildingId).garrisons.ReleaseGarrison(_id);
//	}
//	_targetBuildingId = targetBuildingId;
//}
//
//void ArmyStateAI::Die()
//{
//	UnitStateAI::Die();
//
//	if (_garrisonBuildingId != -1) {
//		//_simulation->building(_garrisonBuildingId).garrisons.ReleaseGarrison(_id);
//		_garrisonBuildingId = -1;
//	}
//}