#pragma once

#include "GameSimulationInfo.h"
#include "PunCity/PunContainers.h"
#include <sstream>

enum class TransformState : uint8
{
	Stand = 0,
	Moving = 1,
	NeedTargetAtom = 2,

	NeedActionUpdate = 3,

	// Con't be more than 2 bits...
};

std::string TransformStateLabel(TransformState state);

enum class UnitUpdateCallerEnum : uint8
{
	None,
	FirstEverUpdate,
	ResetActions,
	MoveTo_Done,
	MoveToForceLongDistance_Done,
	MoveToRobust_Done,
	MoveTowards_Done,

	AddUnit1,
	AddUnit2,

	TransformState_Moving,
	TransformState_NeedTargetAtom1,
	TransformState_NeedTargetAtom2,

	TryForestingPlantAction_Done,
	AttackOutgoing_Done,
	DoFarmWork,

	LaunchArrow,
	EmptyActions,

	DropoffFoodAnimal,
	PickupFoodAnimal,
	Wait,
	GatherFruit,
	TrimFullBush,
	HarvestTileObj,
	PlantTree,
	NourishTree,
	Eat_Done,
	Eat_SomeoneElseTookFood,
	Heat,
	Heal,
	Tool,
	HaveFun,

	MoveRandomlyPerlin_Failed,
	MoveInRange,
	PickupResource,
	DropoffResource,
	DropInventoryAction,
	StoreGatheredAtWorkplace,

	Produce_Done,
	Construct_Done,
	FillInputs_Done,
};

class IUnitDataSource
{
public:
	virtual const WorldAtom2& atomLocation(int id) const = 0;
	virtual const WorldAtom2& targetLocation(int id) const = 0;
	//virtual uint8_t& transformState(int id) = 0;
	//virtual uint32_t& nextActiveTick(int id) = 0;

	virtual const UnitEnum& unitEnum(int id) const = 0;
	virtual TransformState transformState(int id) const = 0;

	virtual bool aliveUnsafe(int32 id) const = 0;
	virtual bool alive(UnitFullId fullId) const = 0;

	virtual const struct UnitLean& unitLean(int32 id) const = 0;

	virtual void SetForceMove(int32 id, bool isForceMove) = 0;

	virtual UnitFullId fullId(int32 id) = 0;

	virtual std::vector<WorldTile2>& waypoint(int id) = 0;
	virtual void TrimWaypoint(int storageIdToTrim, int32 unitId) = 0;

	virtual void SetNextTickState(int id, TransformState state, UnitUpdateCallerEnum caller, int ticks = 1, bool resetActions = false) = 0;
	virtual int nextActiveTick(int id) const = 0;

	virtual void SetTargetLocation(int id, WorldAtom2& targetLocation) = 0;
	virtual void SetAtomLocation(int id, WorldAtom2& atomLocation) = 0;

	virtual WorldAtom2 actualAtomLocation(int id) const = 0;

	virtual void CheckIntegrity(int id) = 0;

	virtual SubregionLists<int32>& unitSubregionLists() = 0;
};

struct UnitReservation
{
public:
	int32_t unitId = -1;
	ReservationType reservationType = ReservationType::None;
	ResourceHolderInfo reserveHolder = ResourceHolderInfo::Invalid();
	int32_t reserveTileId = -1;
	int32_t reserveWorkplaceId = -1;
	int32_t amount = -1;

	//! Serialize
	FArchive& operator>>(FArchive &Ar) {
		Ar << unitId;
		Ar << reservationType;
		reserveHolder >> Ar;
		Ar << reserveTileId;
		Ar << reserveWorkplaceId;
		Ar << amount;
		return Ar;
	}

public:
	bool isValid() { return unitId != -1; }

	std::string ToString() {
		std::stringstream ss;
		ss << "[" << ReservationTypeName(reservationType) << ", ";
		switch (reservationType) {
			case ReservationType::Pop:
			case ReservationType::Push: {
				ss << reserveHolder.ToString() << ", amount:" << amount;
				break;
			}
			case ReservationType::Workplace: {
				ss << "workplaceId:" << reserveWorkplaceId;
				break;
			}
			case ReservationType::TreeTile: {
				ss << WorldTile2(reserveTileId).ToString();
				break;
			}
		}
		ss << "]";
		return ss.str();
	}

	bool operator== (const UnitReservation& a) {
		return unitId == a.unitId && reservationType == a.reservationType && reserveHolder == a.reserveHolder &&
				reserveTileId == a.reserveTileId && reserveWorkplaceId == a.reserveWorkplaceId && amount == a.amount;
	}
};


// Pack the most often used vars together for less cache miss...
struct UnitUpdateInfo
{
	int32 nextUpdateTick = -1; // pack state into this??
	TransformState state;

	//! Debug
	int32 queuedTick = -1;
	UnitUpdateCallerEnum caller = UnitUpdateCallerEnum::None;

	bool isValid() { return nextUpdateTick != -1; }

	static UnitUpdateInfo InitialState() {
		return { Time::Ticks(), TransformState::NeedActionUpdate, Time::Ticks(), UnitUpdateCallerEnum::FirstEverUpdate };
	}

	//! Serialize
	FArchive& operator>>(FArchive &Ar) {
		Ar << nextUpdateTick;
		Ar << state;
		Ar << queuedTick;
		Ar << caller;
		return Ar;
	}
};

struct UnitLean
{
	UnitFullId unitId() { return _unitId; }
	
	WorldAtom2 atomLocation;
	WorldAtom2 targetLocation;
	std::vector<WorldTile2> waypointStack;
	UnitEnum unitEnum;
	bool isForceMove = false;

#if WITH_EDITOR
	int32 lastMoveTick = -1;
	int32 ticksNeeded = 0;
#endif

	UnitUpdateInfo nextUpdate;
	UnitUpdateInfo lastUpdate;
	UnitUpdateInfo lastLastUpdate;

	int32 lastUpdateTick = -1; // Used to prevent unit getting update called two times on the same tick

	// TODO: Unit fullId problem???, getting UnitLean with just id instead of fullId might get a different unit...
	
	bool alive() const { return _alive; }
	int32 birthTicks() const { return _unitId.birthTicks; }

	void Init(UnitFullId unitIdIn)
	{
		_unitId = unitIdIn;
		_alive = true;
		lastUpdateTick = Time::Ticks();
	}
	void Deinit() {
		PUN_CHECK(_alive);
		_alive = false;
	}

	//! Serialize
	FArchive& operator>>(FArchive &Ar)
	{
		atomLocation >> Ar;
		targetLocation >> Ar;
		SerializeVecObj(Ar, waypointStack);
		Ar << unitEnum;
		Ar << isForceMove;

#if WITH_EDITOR
		Ar << lastMoveTick;
		Ar << ticksNeeded;
#endif

		nextUpdate >> Ar;
		lastUpdate >> Ar;
		lastLastUpdate >> Ar;

		Ar << lastUpdateTick; // Used to prevent unit getting update called two times on the same tick

		_unitId >> Ar;
		Ar << _alive;
		return Ar;
	}
	
private:
	UnitFullId _unitId;
	bool _alive = false;
};