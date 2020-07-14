//a// Fill out your copyright notice in the Description page of Project Settings.
//
//#pragma once
//
//#include "UnitStateAI.h"
//
///**
// * 
// */
//class ArmyStateAI : public UnitStateAI
//{
//public:
//	void CalculateActions() final;
//
//	bool TryGarrison();
//
//	void GarrisonTarget(int32_t garrisonTargetId);
//
//	void Die() final;
//
//	// Army order
//	void IssueUnitOrderGarrison(int32_t targetBuildingId);
//
//	int32_t targetBuildingId() { return _targetBuildingId; }
//	int32_t garrisonBuildingId() { return _garrisonBuildingId; }
//private:
//	int32_t _garrisonBuildingId = -1;
//	int32_t _targetBuildingId = -1;
//};
