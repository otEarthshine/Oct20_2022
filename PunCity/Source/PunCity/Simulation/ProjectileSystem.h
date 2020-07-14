// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "IGameSimulationCore.h"
#include "UnitStateAI.h"

struct ProjectileDisplayInfo
{
	WorldAtom2 location;
	WorldAtom2 targetLocation;
	int32_t fraction100000;
	int32_t groundAtomSpeed;
	int32_t totalTicks;
	int32_t groundAtomDistance;
};

// Using UnitStateAI to build projectile:
// - do update without lots of looping
// - less code, no projectileSystem, no extra AssetLoader, no extra displayComponent
class ProjectileArrow : public UnitStateAI
{
public:
	ProjectileDisplayInfo GetProjectileDisplayInfo() const;

	void Launch(UnitFullId ownerIdIn, int32 ownerWorkplaceIdIn, UnitFullId targetIdIn, int32 damage);

	void CalculateActions() final; // Calculate action is when arrow's wait is done and it hits..

	UnitAIClassEnum classEnum() override { return UnitAIClassEnum::ProjectileArrow; }
	void Serialize(FArchive& Ar) override
	{
		UnitStateAI::Serialize(Ar);
		
		_launchAtom >> Ar;
		Ar << _launchTick;
		Ar << _travelDistance;

		ownerId >> Ar;
		targetId >> Ar;

		Ar << ownerWorkplaceId;

		Ar << damage;
	}
	
protected:
	WorldAtom2 _launchAtom;
	int32 _launchTick = -1; //TODO: find out why lastActiveTick didn't work...
	int32 _travelDistance = -1;
public:
	UnitFullId ownerId;
	UnitFullId targetId;

	int32 ownerWorkplaceId;

	int32 damage;
	
	const int32 GroundSpeed = HumanGlobalInfo::MoveAtomsPerTick * 5;
};

//class ProjectileArrow : public ProjectileBase
//{
//public:
//
//	int32_t damage;
//	const int32_t GroundSpeed = HumanGlobalInfo::MoveAtomsPerTick * 2;
//};

/**
 * 
 */
class ProjectileSystem
{
public:






};
