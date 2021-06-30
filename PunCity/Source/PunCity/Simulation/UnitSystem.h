#pragma once

#include "IGameSimulationCore.h"
#include "UnitBase.h"

#include <array>

#include "UnitStateAI.h"
#include "PunCity/PunUtils.h"
#include "PunCity/PunContainers.h"
#include "UpdateRingBuffer.h"

class UnitStateAI;

class UnitSystem : public IUnitDataSource
{
public:
	virtual ~UnitSystem() {}

	void Init(IGameSimulationCore* simulation);

	void Tick();

public:
	//! Unit Data Getters
	const WorldAtom2& atomLocation(int id) const final { return _unitLeans[id].atomLocation; }
	const WorldAtom2& targetLocation(int id) const final { return _unitLeans[id].targetLocation; }
	const UnitEnum& unitEnum(int id) const final { return _unitLeans[id].unitEnum; }
	
	std::vector<WorldTile2>& waypoint(int id) final { return _unitLeans[id].waypointStack; }

	void SetWaypoint(int32 id, const std::vector<WorldTile2>& waypoint) final {
		_unitLeans[id].waypointStack = waypoint;
	}
	void SetWaypointCacheBuildingId(int32 id, int32 waypointCacheBuildingId) final {
		_unitLeans[id].waypointCacheBuildingId = waypointCacheBuildingId;
	}

	// Trim waypoint for placing resources on storage's nearest point
	void TrimWaypoint(int storageIdToTrim, int32 unitId) final
	{
		std::vector<WorldTile2>& waypoint = _unitLeans[unitId].waypointStack;
		int32 storageTileCount = 0;
		for (size_t i = waypoint.size(); i-- > 0;) {
			if (_simulation->buildingIdAtTile(waypoint[i]) == storageIdToTrim) {
				storageTileCount++;

				// Already arrived at storage, trim away the rest of the tiles
				if (storageTileCount > 1) 	{
					waypoint.erase(waypoint.begin() + i);
				}
			}
		}
	}

	int nextActiveTick(int id) const override { return _unitLeans[id].nextUpdate.nextUpdateTick; }
	int lastActiveTick(int id) const { return _unitLeans[id].lastUpdate.nextUpdateTick; }

	const UnitLean& unitLean(int32 id) const final { return _unitLeans[id]; }

	void SetForceMove(int32 id, bool isForceMove) final { _unitLeans[id].isForceMove = isForceMove; }

	UnitStateAI& unitStateAI(int id)
	{
		PUN_CHECK(0 <= id && id < _stateAI.size());
		return *_stateAI[id];
	}

	bool IsUnitValid(int32 unitId) {
		return 0 <= unitId && unitId < _stateAI.size();
	}
	

	int16 food(int id);
	int age(int id);

	// Alive that doesn't check for unit that turned alive again from reuse
	bool aliveUnsafe(int32 id) const final
	{
		PUN_CHECK(0 <= id && id < _stateAI.size());
		return _unitLeans[id].alive();
	}
	
	UnitFullId fullId(int32 id) final { return UnitFullId(id, _unitLeans[id].birthTicks()); }
	
	bool alive(UnitFullId fullId) const final {
		PUN_CHECK(0 <= fullId.id && fullId.id < _stateAI.size());
		// alive if fullId is the same as the current one and the unitLean is alive
		if (_unitLeans[fullId.id].birthTicks() != fullId.birthTicks) {
			return false;
		}
		return _unitLeans[fullId.id].alive();
	}

	WorldAtom2 actualAtomLocation(int id) const final;

	bool isMoving(int32 id)
	{
		WorldAtom2 atom = atomLocation(id);
		return atom != targetLocation(id);
	}

	std::string debugStr(int id);

public:
	//! Unit Data Setters

	void SetNextTickState(int32 id, TransformState state, UnitUpdateCallerEnum caller, int ticksNeeded = 1, bool resetActions = false) final;
	TransformState transformState(int id) const final { 
		return _unitLeans[id].nextUpdate.state;
	}

	void SetTargetLocation(int id, WorldAtom2& targetLocation) override { _unitLeans[id].targetLocation = targetLocation; }
	void SetAtomLocation(int id, WorldAtom2& atomLocation) override { _unitLeans[id].atomLocation = atomLocation; }

	void MoveUnitInstantly(int32 id, WorldAtom2 targetAtom) override {
		auto& unitLean = _unitLeans[id];
		WorldAtom2 previousAtom = unitLean.atomLocation;
		unitLean.targetLocation = targetAtom;
		unitLean.atomLocation = targetAtom;
		_unitSubregionLists.TryMove(previousAtom.worldTile2(), targetAtom.worldTile2(), id);
	}

	void UnitCheckIntegrity(bool full = false);
	
	void CheckIntegrity(int id) final
	{
		PUN_CHECK2(nextActiveTick(id) >= Time::Ticks(), debugStr(id));
		PUN_CHECK2(nextActiveTick(id) < Time::Ticks() + Time::TicksPerSeason, debugStr(id));
	}

public:

	int unitCount() const { return _unitLeans.size() - _deadUnits.size(); }
	int deadCount() const { return _deadUnits.size(); }

	int32 unitCount(UnitEnum unitEnum) const { return _unitEnumToUnitCount[static_cast<int>(unitEnum)]; }
	int32 animalInitialCount(UnitEnum unitEnum) const { return _animalEnumToInitialCount[static_cast<int>(unitEnum)]; }

	void AddAnimals(int animalCount);
	int AddUnit(UnitEnum unitEnum, int32 townId, WorldAtom2 location, int32 ageTicks);
	void RemoveUnit(int32 objectId);

	void ResetUnitActionsInArea(TileArea area);

	// !!! Don't call this directly, call from sim
	void ResetActions_SystemPart(int id, int32_t waitTicks);

	PUN_DEBUG_EXPR(
		int stateCount(std::string stateName) { return _statesCount.get(stateName); }
		int stateWaitCount() {
			return unitCount() - _statesCount.Sum();
		}
	);

	// Note that SubregionLists's tile is not unitTile...
	SubregionLists<int32>& unitSubregionLists() final { return _unitSubregionLists; }

#if CHECK_TICKHASH
	int32 GetSyncHash() {
		int32 hash = 0;
		for (size_t i = _unitLeans.size(); i-- > 0;) {
			const UnitLean& unitLean = _unitLeans[i];
			hash += unitLean.atomLocation.x;
			hash += unitLean.atomLocation.y;
			hash += unitLean.targetLocation.x;
			hash += unitLean.targetLocation.y;
			hash += unitLean.lastUpdate.nextUpdateTick;
			hash += unitLean.birthTicks();
			hash += unitLean.alive();
			hash += unitLean.waypointStack.size();
			hash += unitLean.isForceMove;
		}
		return hash;
	}

	int32 GetSyncHash_Actions() {
		int32 hash = 0;
		for (size_t i = _stateAI.size(); i-- > 0;) {
			const UnitStateAI* unit = _stateAI[i].get();
			hash += unit->GetSyncHash_Actions();
		}
		return hash;
	}

	std::vector<int32> GetUnitSyncHashes() {
		std::vector<int32> hashes;
		for (size_t i = _stateAI.size(); i-- > 0;) {
			const UnitStateAI* unit = _stateAI[i].get();
			hashes.push_back(unit->GetSyncHash_Actions());
		}
		return hashes;
	}
#endif
	

	void Serialize(FArchive& Ar);

private:
	IGameSimulationCore* _simulation = nullptr;
	
private:
	/*
	 * Serialize
	 */
	SubregionLists<int32> _unitSubregionLists;
	std::vector<UnitLean> _unitLeans;
	std::vector<int32> _deadUnits;
	UpdateRingBuffer _updateBuffer;
	
	std::vector<std::unique_ptr<UnitStateAI>> _stateAI;

	std::vector<int32> _unitEnumToUnitCount;
	std::vector<int32> _animalEnumToInitialCount; // This is useful for controlling animal population.

private:
	PUN_DEBUG_EXPR(PunStates _statesCount);
};