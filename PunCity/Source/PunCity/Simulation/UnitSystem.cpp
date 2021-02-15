#include "UnitSystem.h"

#include "HumanStateAI.h"
#include "UnitStateAI.h"
#include "ArmyStateAI.h"
#include "PunCity/MapUtil.h"
#include "IGameSimulationCore.h"
#include "OverlaySystem.h"
#include "UnrealEngine.h"
#include "PunCity/GameRand.h"
#include "PunCity/GameConstants.h"
#include "StatSystem.h"
#include "ProjectileSystem.h"

DECLARE_CYCLE_STAT(TEXT("PUN: Unit"), STAT_PunUnit, STATGROUP_Game);
//DECLARE_CYCLE_STAT(TEXT("PUN: Unit.Outer [0.2]"), STAT_PunUnitOuter, STATGROUP_Game);
DECLARE_CYCLE_STAT(TEXT("PUN: Unit.Moving [1]"), STAT_PunUnitMoving, STATGROUP_Game);

DECLARE_CYCLE_STAT(TEXT("PUN: Unit.NeedTargetAtom [2]"), STAT_PunUnitNeedTargetAtom, STATGROUP_Game);
DECLARE_CYCLE_STAT(TEXT("PUN: Unit.NeedTargetAtom.Start [2]"), STAT_PunUnitNeedTargetAtomStart, STATGROUP_Game);
DECLARE_CYCLE_STAT(TEXT("PUN: Unit.NeedTargetAtom.Road [2]"), STAT_PunUnitNeedTargetAtomRoad, STATGROUP_Game);
DECLARE_CYCLE_STAT(TEXT("PUN: Unit.NeedTargetAtom.SetNextTick [2]"), STAT_PunUnitNeedTargetAtomSetNextTick, STATGROUP_Game);
DECLARE_CYCLE_STAT(TEXT("PUN: Unit.NeedTargetAtom.Force [2]"), STAT_PunUnitNeedTargetAtomForce, STATGROUP_Game);
DECLARE_CYCLE_STAT(TEXT("PUN: Unit.NeedTargetAtom.NeedActionUpdate [2]"), STAT_PunUnitNeedTargetAtomNeedActionUpdate, STATGROUP_Game);

DECLARE_CYCLE_STAT(TEXT("PUN: Unit.SetNextTick [-]"), STAT_PunUnitSetNextTick, STATGROUP_Game);

using namespace std;

void UnitSystem::Init(IGameSimulationCore* simulation)
{
	_simulation = simulation;

	_unitSubregionLists.Init();
	_updateBuffer.Init(9); // 9 shift is 512 ticks

	UnitStateAI::debugFindFullBushSuccessCount = 0;
	UnitStateAI::debugFindFullBushFailCount = 0;

	_unitEnumToUnitCount.resize(UnitEnumCount);
	_animalEnumToInitialCount.resize(UnitEnumCount);

	// Check
	check(static_cast<int32>(UnitState::Count) == UnitStateName.Num());
}

int16 UnitSystem::food(int id)
{
	PUN_CHECK(0 <= id && id < _stateAI.size());
	return _stateAI[id]->food();
}
int UnitSystem::age(int id)
{
	PUN_CHECK(0 <= id && id < _stateAI.size());
	return _stateAI[id]->age();
}

WorldAtom2 UnitSystem::actualAtomLocation(int id) const
{
	// If it is moving, calculate where the unit should be based on start/end and time passed
	if (transformState(id) == TransformState::Moving) {
		const UnitLean& unitLean = _unitLeans[id];
		uint32_t lastActive = unitLean.lastUpdate.nextUpdateTick;

		int32_t travelTicks = (nextActiveTick(id) - lastActive);

		// TODO: Check why there is zero.... Properly fix this...
		if (travelTicks == 0) travelTicks = 1;

		int32_t fraction100000 = static_cast<int64_t>(Time::Ticks() - lastActive) * 100000 / travelTicks;
		return WorldAtom2::Lerp(unitLean.atomLocation, unitLean.targetLocation, fraction100000);
	}
	else {
		return _unitLeans[id].atomLocation;
	}
}

std::string UnitSystem::debugStr(int id)
{
	return unitStateAI(id).debugStr();
}

void UnitSystem::AddAnimals(int animalCount)
{
	for (int i = 0; i < animalCount; i++)
	{
		for (int count = 0; count < 1000; count++) 
		{
			int16 randX = GameRand::Rand() % GameMapConstants::TilesPerWorldX;
			int16 randY = GameRand::Rand() % GameMapConstants::TilesPerWorldY;
			WorldTile2 tile(randX, randY);

			if (_simulation->pathAI()->isWalkable(randX, randY) &&
				_simulation->GetProvinceIdClean(tile) != -1)
			{
				BiomeEnum biomeEnum = _simulation->GetBiomeEnum(tile);
				BiomeInfo biomeInfo = GetBiomeInfo(biomeEnum);

				UnitEnum unitEnum;

#if TRAILER_MODE
				if (biomeEnum == BiomeEnum::Tundra || biomeEnum == BiomeEnum::BorealForest) {
					continue;
				}
#endif
				
				if (biomeEnum == BiomeEnum::Forest && GameRand::Rand() % 500 == 0) { // 1/500 to spawn panda
					unitEnum = UnitEnum::Panda;
				}
				else if (biomeInfo.rareAnimals.size() > 0 && GameRand::Rand() % 100 == 0) // 1/100 rare animal signting
				{
					unitEnum = biomeInfo.rareAnimals[GameRand::Rand() % biomeInfo.rareAnimals.size()];
				}
				else if (biomeInfo.animals.size() > 0)
				{
					unitEnum = biomeInfo.animals[GameRand::Rand() % biomeInfo.animals.size()];
				}
				else
				{
					continue;
				}

				int32 ageTicks = GameRand::Rand() % GetUnitInfo(unitEnum).maxAgeTicks;
				
				AddUnit(unitEnum, GameInfo::PlayerIdNone, tile.worldAtom2(), ageTicks);

				_animalEnumToInitialCount[static_cast<int>(unitEnum)]++;
				
				break;
			}
		}
	}

	//PUN_LOG("InitialUnitCount: %d", _unitLeans.size());
}

int UnitSystem::AddUnit(UnitEnum unitEnum, int32 townId, WorldAtom2 location, int32 ageTicks)
{
	int objectId = -1;

	// Resolve UnitAI
	unique_ptr<UnitStateAI> unitAI;
	switch (unitEnum)
	{
	case UnitEnum::Human: unitAI = make_unique<HumanStateAI>(); break;
	//case UnitEnum::Infantry: unitAI = make_unique<ArmyStateAI>(); break;
	case UnitEnum::ProjectileArrow: unitAI = make_unique<ProjectileArrow>(); break;
	default: unitAI = make_unique<UnitStateAI>();
	}

	if (_deadUnits.empty())
	{
		objectId = _unitLeans.size();

		UnitFullId fullId(objectId, Time::Ticks() - ageTicks);

		UnitLean unitLean;
		unitLean.atomLocation = location;
		unitLean.targetLocation = location;
		unitLean.Init(fullId);
		unitLean.unitEnum = unitEnum;
		_unitLeans.push_back(unitLean);

		//UE_LOG(LogTemp, Error, TEXT("AddUnit1 id:%d tick:%d"), objectId, Time::Ticks());
		SetNextTickState(objectId, TransformState::NeedActionUpdate, UnitUpdateCallerEnum::AddUnit1, GameRand::Rand() % 300 + 1); // Activate units a bit randomly to make CPU more balanced

		//if (unitEnum == UnitEnum::Human) {
		//	_stateAI.push_back(std::make_unique<HumanStateAI>());
		//} 
		//else if (unitEnum == UnitEnum::Infantry) {
		//	_stateAI.push_back(std::make_unique<ArmyStateAI>());
		//}
		//else {
		//	_stateAI.push_back(std::make_unique<UnitStateAI>());
		//}

		_stateAI.push_back(move(unitAI));

		_stateAI.back()->AddUnit(unitEnum, townId, fullId, this, _simulation);

		//if (location.worldTile2().x < 50 && location.worldTile2().y < 50) UE_LOG(LogTemp, Error, TEXT("NewHuman: Count: %d"), unitCount());
	}
	else {
		objectId = _deadUnits.back();
		_deadUnits.pop_back();

		UnitFullId fullId(objectId, Time::Ticks() - ageTicks);

		_unitLeans[objectId].atomLocation = location;
		_unitLeans[objectId].targetLocation = location;
		_unitLeans[objectId].Init(fullId);
		_unitLeans[objectId].unitEnum = unitEnum;


		//if (objectId < 0 || (objectId >= 50000 && objectId < 50010)) {
			//UE_LOG(LogTemp, Error, TEXT("AddUnit2 id:%d tick:%d"), objectId, Time::Ticks());
		//}
		SetNextTickState(objectId, TransformState::NeedActionUpdate, UnitUpdateCallerEnum::AddUnit2);

		//if ((UnitEnum)unitEnum == UnitEnum::Human) {
		//	_stateAI[objectId] = std::make_unique<HumanStateAI>();
		//} 
		//else if (IsProjectile(unitEnum)) {
		//	_stateAI[objectId] = std::make_unique<ProjectileArrow>();
		//}
		//else {
		//	_stateAI[objectId] = std::make_unique<UnitStateAI>();
		//}
		_stateAI[objectId] = move(unitAI);
		_stateAI[objectId]->AddUnit(unitEnum, townId, fullId, this, _simulation);

		//if (location.worldTile2().x < 100 && location.worldTile2().y < 100) UE_LOG(LogTemp, Error, TEXT("ReuseHuman: Count: %d"), unitCount());
	}

	_unitSubregionLists.Add(location.worldTile2(), objectId);

	_unitEnumToUnitCount[static_cast<int>(unitEnum)]++;

	return objectId;
}

void UnitSystem::RemoveUnit(int32 unitId)
{
	//UE_LOG(LogTemp, Error, TEXT("RemoveUnit:%d"), objectId);
	UnitLean& unitLean = _unitLeans[unitId];
	UnitEnum unitEnum = unitLean.unitEnum;

	_updateBuffer.RemoveUpdateInfo(unitId, unitLean.nextUpdate.nextUpdateTick);
	// Don't need to RemoveUpdateInfo for last update...

	unitLean.Deinit();
	_deadUnits.push_back(unitId); // TODO:

	_unitSubregionLists.Remove(unitLean.atomLocation.worldTile2(), unitId);

	_unitEnumToUnitCount[static_cast<int>(unitEnum)]--;
}

void UnitSystem::ResetActions_SystemPart(int id, int32_t waitTicks)
{
	_updateBuffer.RemoveUpdateInfo(id, _unitLeans[id].nextUpdate.nextUpdateTick);

	_stateAI[id]->ResetActions_UnitPart(waitTicks);
}

void UnitSystem::ResetUnitActionsInArea(TileArea area)
{
	// Reset any units within the building area.
	std::vector<WorldRegion2> overlapRegions = area.GetOverlapRegions();

	for (WorldRegion2 region : overlapRegions) {
		_unitSubregionLists.ExecuteRegion(region, [&](int32_t unitId)
		{
			WorldTile2 unitTile = actualAtomLocation(unitId).worldTile2();
			if (area.HasTile(unitTile)) {
				//PUN_LOG("Reset unit actions: %d %s", unitId, *unitTile.To_FString());
				_simulation->ResetUnitActions(unitId);
			}
		});
	}
}

void UnitSystem::UnitCheckIntegrity(bool full)
{
#if TRAILER_MODE
	return;
#endif
	
	// Make sure lastTick is not too faraway for alive units
	for (size_t i = 0; i < _unitLeans.size(); i++) {
		UnitLean& unitLean = _unitLeans[i];
		if (unitLean.alive() && unitLean.lastUpdateTick != -1) {
			int32 ticks = Time::Ticks();
			const UnitStateAI& unitAI = unitStateAI(i); // For debugger...
			
			// TODO: check ppl walk super far
			//PUN_CHECK2(ticks - unitLean.lastUpdateTick < Time::TicksPerMinute, unitAI.debugStr());

			if (full) {
				PUN_CHECK2(unitLean.nextUpdate.nextUpdateTick >= Time::Ticks(), unitAI.debugStr());
			}
		}
	}
}

void UnitSystem::Tick()
{
	SCOPE_CYCLE_COUNTER(STAT_PunUnit);

	PUN_DEBUG_EXPR(_statesCount.SetZeros());

	//UE_LOG(LogTemp, Error, TEXT("Tick %d"), Time::Ticks());
	if (Time::Ticks() > 0) {
		_updateBuffer.AfterProcess();
	}

	//int32 startUnitCount = unitCount();
	

	UnitCheckIntegrity();

	// Ensure no dupe id
	static TSet<int32> usedId;
	usedId.Empty();

	const std::vector<UpdateRingInfo>& updateInfos = _updateBuffer.unitsToUpdate();
	for (size_t j = updateInfos.size(); j-- > 0;)
	{
		const UpdateRingInfo& info = updateInfos[j];
		int32_t id = info.unitId;

		{
			// x5 speed:  1150 count, 0.2 ms
			//SCOPE_CYCLE_COUNTER(STAT_PunUnitOuter);

			// Continue while decreasing round count if this needs more than 1 round
			if (info.nextUpdateTick != Time::Ticks()) {
				check(((info.nextUpdateTick - Time::Ticks()) & _updateBuffer.tickCountMask) == 0);
				_updateBuffer.AddBackUnripeTick(info);
				continue;
			}
		}

		UnitLean& unitLean = _unitLeans[id];

		// TODO: do we really need this??
		if (!unitLean.alive()) {
			continue;
		}

		// Update or ResetActions called this turn, null any further action this turn
		if (unitLean.lastUpdateTick == Time::Ticks()) {
			continue;
		}
		unitLean.lastUpdateTick = Time::Ticks();

		PUN_CHECK(!usedId.Contains(id));
		usedId.Add(id);

		TransformState state = unitLean.nextUpdate.state;

		// If it is in the moving state, try to get new target tile from waypoint
		if (state == TransformState::Moving)
		{
			SCOPE_CYCLE_COUNTER(STAT_PunUnitMoving);

			// Move Unit to new region if needed
			_unitSubregionLists.TryMove(unitLean.atomLocation.worldTile2(), unitLean.targetLocation.worldTile2(), id);

			// Arrived at the old target.
			unitLean.atomLocation = unitLean.targetLocation;

			SetNextTickState(id, TransformState::NeedTargetAtom, UnitUpdateCallerEnum::TransformState_Moving);
			//_stateAI[id]->AddDebugSpeech("TransformState::Moving");

			PUN_DEBUG_EXPR(_statesCount["Moving"]++);
		}
		else if (state == TransformState::NeedTargetAtom)
		{
			//SCOPE_CYCLE_COUNTER(STAT_PunUnitNeedTargetAtom);

			auto& waypoint = unitLean.waypointStack;
			if (!waypoint.empty()) 
			{
				WorldTile2 tile;
				{
					// count:200 time:0.06ms
					//SCOPE_CYCLE_COUNTER(STAT_PunUnitNeedTargetAtomStart);

					tile = waypoint.back();
					unitLean.targetLocation = tile.worldAtom2();
					waypoint.pop_back();
				}

				//check(_targetLocation[i].x >= 0 && _targetLocation[i].y >= 0);
				int32 moveSpeed = HumanGlobalInfo::MoveAtomsPerTick;

				// Humans have to consider road and weight
				if (unitLean.unitEnum == UnitEnum::Human) 
				{
					SCOPE_CYCLE_COUNTER(STAT_PunUnitNeedTargetAtomRoad);

					UnitStateAI& stateAI = unitStateAI(id);
					HumanStateAI& humanAI = stateAI.subclass<HumanStateAI>();

					UnitAnimationEnum animationEnum = stateAI.animationEnum();
					if (animationEnum == UnitAnimationEnum::Caravan) {
						moveSpeed = moveSpeed * 2;
					}
					else if (animationEnum == UnitAnimationEnum::Ship) {
						moveSpeed = moveSpeed * 4;
					}
					else
					{
						RoadTile road = _simulation->overlaySystem().GetRoad(unitLean.targetLocation.worldTile2());
						//bool isRoad = _simulation->IsRoadTile(tile); // Not using this because we need RoadTile

						// 
						// Hauling cases to deal with
						// - notHauling, no road ... 0
						// - hauling, no road ... -33
						// - notHauling, dirt road ... 0
						// - hauling, dirt road ... 0
						// - notHauling, stone road ... 20
						// - hauling, stone road ... 20 (this is actually 80% from the hauling no-road)
						// numbers chosen to make PathAI use less variable
						if (road.isValid() && road.isConstructed) {
							if (road.isDirt) {
								moveSpeed = moveSpeed * 120 / 100;
							}
							else {
								moveSpeed = moveSpeed * 130 / 100;
							}
						}
						
						// TODO: maybe putting this in UnitStateAI might help with less pointer cache miss?
						int32 workEfficiency100 = humanAI.workEfficiency100(false);
						if (workEfficiency100 < 50) {
							workEfficiency100 = 50; // Min movespeed of 50%
						}
						moveSpeed = moveSpeed * workEfficiency100 / 100;
					}
				}
				// Slow down animals
				else {
					moveSpeed = moveSpeed * 2 / 3;
				}

				{
					// count:537 time:0.13ms
					//SCOPE_CYCLE_COUNTER(STAT_PunUnitNeedTargetAtomSetNextTick);
					
					PUN_CHECK(moveSpeed > 0);
					PUN_CHECK(moveSpeed < HumanGlobalInfo::MoveAtomsPerTick * 7);
					// TODO: Hack to prevent crash...
					if (moveSpeed <= 0) {
						moveSpeed = HumanGlobalInfo::MoveAtomsPerTick;
					}

					// Find how long it takes to get to target tile and set the nextActiveFrame to that point
					int32 distance = WorldAtom2::Distance(unitLean.targetLocation, unitLean.atomLocation);
					int32 ticksNeeded = distance / moveSpeed;

					// Make sure that tick is more than 0
					ticksNeeded = ticksNeeded > 0 ? ticksNeeded : 1;

					// Be in the moving state until we arrived at destination.
					SetNextTickState(id, TransformState::Moving, UnitUpdateCallerEnum::TransformState_NeedTargetAtom1, ticksNeeded);
					//_stateAI[id]->AddDebugSpeech("TransformState::NeedTargetAtom1");

#if WITH_EDITOR
					unitLean.ticksNeeded = ticksNeeded;
					unitLean.lastMoveTick = Time::Ticks();
#endif
				}

				//if (unitLean.unitEnum == UnitEnum::Human)

				// If the targetLocation is no longer valid, reset the unit
				// Note: this must be after SetNextTickState..
				if (!unitLean.isForceMove)
				{
					// count:200 time:0.06ms
					//SCOPE_CYCLE_COUNTER(STAT_PunUnitNeedTargetAtomForce);
					
					auto pathAI = _simulation->pathAI();
					if (!pathAI->isWalkable(tile.x, tile.y)) {
						waypoint.clear();
						_simulation->ResetUnitActions(id);
						//PUN_LOG("TargetLocation no longer valid %d", id);
					}
				}
			}
			else {
				// count:85 time:0.02ms
				//SCOPE_CYCLE_COUNTER(STAT_PunUnitNeedTargetAtomNeedActionUpdate);
				
				SetNextTickState(id, TransformState::NeedActionUpdate, UnitUpdateCallerEnum::TransformState_NeedTargetAtom2);
				//_stateAI[id]->AddDebugSpeech("TransformState::NeedTargetAtom2");
			}


			PUN_DEBUG_EXPR(_statesCount["NeedTargetTile"]++);
		}
		else if (state == TransformState::NeedActionUpdate)
		{
			_stateAI[id]->Update();

			PUN_DEBUG_EXPR(_statesCount["NeedActionUpdate"]++);
		}

	}

	//SystemIntegrityCheck();

	//if (startUnitCount != unitCount()) {
	//	PUN_LOG("UnitSystem Tick unitCount():%d _unitLeans:%d", unitCount(), _unitLeans.size());
	//}
}

void UnitSystem::SetNextTickState(int32 id, TransformState state, UnitUpdateCallerEnum caller, int ticksNeeded, bool resetActions)
{
	//SCOPE_CYCLE_COUNTER(STAT_PunUnitSetNextTick);

	UnitLean& unitLean = _unitLeans[id];

	int32 ticks = Time::Ticks();

	// Dealing with ResetActions()
	// 
	// Case 1: A has (nextUpdateTick > tick)
	// B 's loop reset A
	//  During reset, remove A's last nextTickState
	//
	// Case 2: A has (nextUpdateTick == tick, SetNextTickState() called before ResetActions())
	//  A did SetNextTickState() ... putting (nextUpdateTick > tick)
	//  B 's loop reset A
	//  Same ending as case 1
	//
	// Case 3: A has (nextUpdateTick == tick, SetNextTickState() will call after ResetActions())
	//   B 's loop reset A
	//   Must find a way to halt's any further call on A this tick.
	//   Do this through updating UnitLean.lastUpdateTick
	// 
	// Next update is after this tick (which means it won't reset after this tick)
	if (resetActions) {
		if (unitLean.nextUpdate.nextUpdateTick > ticks) {
			_updateBuffer.RemoveUpdateInfo(id, unitLean.nextUpdate.nextUpdateTick);
		} else if (unitLean.nextUpdate.nextUpdateTick == ticks) {

		} else {
			UE_DEBUG_BREAK();
		}
		unitLean.lastUpdateTick = ticks;
	}
	//else if (unitLean.lastUpdate.isValid() && ticks == unitLean.lastUpdate.queuedTick) {
	//	_updateBuffer.RemoveUpdateInfo(id, unitLean.lastUpdate.nextUpdateTick);
	//}

#if WITH_EDITOR
	//_updateBuffer.CheckNoUpdateInfo(id, ticks);
#endif
	PUN_CHECK(ticksNeeded > 0);
	
	int32 nextUpdateTick = ticksNeeded + ticks;
	_updateBuffer.AddUpdateInfo(id, nextUpdateTick, state/*, caller*/);
	
	unitLean.lastLastUpdate = unitLean.lastUpdate.isValid() ? unitLean.lastUpdate : UnitUpdateInfo::InitialState();
	unitLean.lastUpdate = unitLean.nextUpdate.isValid() ? unitLean.nextUpdate : UnitUpdateInfo::InitialState();
	unitLean.nextUpdate = { nextUpdateTick, state, Time::Ticks(), caller };


#if DEBUG_UPDATE_BUFFER
	PUN_CHECK2(nextActiveTick(id) > Time::Ticks() || 
				nextActiveTick(id) < Time::Ticks() + Time::TicksPerSeason, debugStr(id));

	if (_updateBuffer.unitIdToTwoUpdateInfos[id * 2].unitId != -1) {
		if (_updateBuffer.unitIdToTwoUpdateInfos[id * 2].nextUpdateTick == nextActiveTick(id)) {
			std::string message = debugStr(id)
				+ " _last.queuedTick:" + std::to_string(_updateBuffer.lastUpdateInfo(id).queuedTick)
				+ " _last.nextUpdateTick:" + std::to_string(_updateBuffer.lastUpdateInfo(id).nextUpdateTick)
				+ " _last.state:" + TransformStateLabel(_updateBuffer.lastUpdateInfo(id).state)
				//+ " _last.caller:" + _updateBuffer.lastUpdateInfo(id).caller
				+ " ----- _next.queuedTick:" + std::to_string(_updateBuffer.nextUpdateInfo(id).queuedTick)
				+ " _next.nextUpdateTick:" + std::to_string(_updateBuffer.nextUpdateInfo(id).nextUpdateTick)
				+ " _next.state:" + TransformStateLabel(_updateBuffer.nextUpdateInfo(id).state)
				//+ " _next.caller:" + _updateBuffer.nextUpdateInfo(id).caller
			;
			UE_LOG(LogTemp, Error, TEXT("Duplicate Update Ticks: %s"), *ToFString(message))
		}
	}
#endif
}

void UnitSystem::KillHalf()
{
	for (int32_t i = 0; i < _unitLeans.size(); i++) {
		if (GameRand::RandChance(2)) {
			unitStateAI(i).Die();
		}
	}
}

void UnitSystem::Serialize(FArchive& Ar)
{
	_unitSubregionLists.Serialize(Ar);
	SerializeVecObj(Ar, _unitLeans);
	SerializeVecValue(Ar, _deadUnits);
	_updateBuffer.Serialize(Ar);

	int32 animalUnitCount = 0;
	SerializeVecLoop(Ar, _stateAI, [&](std::unique_ptr<UnitStateAI>& unitAI) {
		SerializePtr<std::unique_ptr<UnitStateAI>, UnitAIClassEnum>(Ar, unitAI, [&](UnitAIClassEnum classEnum) {
			switch (classEnum) {
			case UnitAIClassEnum::UnitStateAI: unitAI = std::make_unique<UnitStateAI>(); animalUnitCount++; break;
			case UnitAIClassEnum::HumanStateAI: unitAI = std::make_unique<HumanStateAI>(); break;
			case UnitAIClassEnum::ProjectileArrow: unitAI = std::make_unique<ProjectileArrow>(); break;
			default: UE_DEBUG_BREAK();
			}
			if (Ar.IsLoading()) {
				unitAI->LoadInit(this, _simulation);
			}
		});
	});

	// Check
	for (size_t i = 0; i < _stateAI.size(); i++) {
		int32 id = _stateAI[i]->id();
		PUN_CHECK(id == i);
	}

	int32 deadUnitCount = 0;
	for (const UnitLean& unitLean : _unitLeans) {
		if (!unitLean.alive()) {
			deadUnitCount++;
		}
	}
	SerializeCrc(Ar, _deadUnits);
	PUN_CHECK(_deadUnits.size() == deadUnitCount);
	
	//PUN_LOG("UnitSystem Serialize isSaving:%d dead:%d animals:%d", Ar.IsSaving(), _deadUnits.size(), animalUnitCount);
	//PUN_LOG("UnitSystem Serialize unitCount():%d _unitLeans:%d", unitCount(), _unitLeans.size());

	SerializeVecValue(Ar, _unitEnumToUnitCount);
	SerializeVecValue(Ar, _animalEnumToInitialCount);

	UnitCheckIntegrity(true);
}