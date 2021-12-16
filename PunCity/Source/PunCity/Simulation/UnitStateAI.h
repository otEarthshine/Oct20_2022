#pragma once

#include "UnitBase.h"
#include "IGameSimulationCore.h"
#include "TreeSystem.h"
#include "Resource/ResourceSystem.h"
#include "Building.h"
#include <functional>
#include <sstream>

#define PUN_UNIT_CHECK(x) PUN_CHECK2(x, debugStr());

enum class UnitState : uint8
{
	GetFood,
	GetHeat,
	GetFun,
	GetTools,
	GetMedicine,
	GetLuxury,
	
	GetWildFood,
	Idle,
	AvoidOthers,
	GoNearbyHome,

	Job,
	GatherTree,
	GatherStone,
	
	ClearLandCutBush,
	ClearLandCutTree,
	ClearLandCutStone,
	
	ClearLandRemoveDrop,
	ForestingCut,
	ForestingPlant,
	ForestingNourish,
	MoveResource,
	
	MoveResourceConstruct,
	StoreInventory,
	GatherFruit,
	TrimBush,
	
	WorkConsume,
	FillInput,
	WorkProduce,
	WorkConstruct,
	Hunt,
	Ranch,
	
	FarmSeeding,
	FarmNourishing,
	FarmHarvesting,
	FarmClearDrops,

	IntercityDelivery,
	ImmigrateToNewCity,

	MoveArmy,

	GoToWorkplace,

	Other,

	Count,
};
#define LOCTEXT_NAMESPACE "UnitStateName"
const TArray<FText> UnitStateName
{
	LOCTEXT("Get Food", "Get Food"),
	LOCTEXT("Get Heat", "Get Heat"),
	LOCTEXT("Get Fun", "Get Fun"),
	LOCTEXT("Get Tools", "Get Tools"),
	LOCTEXT("Get Medicine", "Get Medicine"),
	LOCTEXT("Get Luxury", "Get Luxury"),

	LOCTEXT("Get Wild Food", "Get Wild Food"),
	LOCTEXT("Idle", "Idle"),
	LOCTEXT("Avoid Others", "Avoid Others"),
	LOCTEXT("Go Nearby Home", "Go Nearby Home"),

	LOCTEXT("Job", "Job"),
	LOCTEXT("Gather Tree", "Gather Tree"),
	LOCTEXT("Gather Stone", "Gather Stone"),

	LOCTEXT("Clear Land (Cut Bush)", "Clear Land (Cut Bush)"),
	LOCTEXT("Clear Land (Cut Tree)", "Clear Land (Cut Tree)"),
	LOCTEXT("Clear Land (Cut Stone)", "Clear Land (Cut Stone)"),

	LOCTEXT("Clear Land (Remove Drop)", "Clear Land (Remove Drop)"),
	LOCTEXT("Foresting Cut", "Foresting Cut"),
	LOCTEXT("Foresting Plant", "Foresting Plant"),
	LOCTEXT("Foresting Nourish", "Foresting Nourish"),
	LOCTEXT("Move Resource", "Move Resource"),

	LOCTEXT("Move Resource (Construct)", "Move Resource (Construct)"),
	LOCTEXT("Store Inventory", "Store Inventory"),
	LOCTEXT("Gather Fruit", "Gather Fruit"),
	LOCTEXT("Remove Bush", "Remove Bush"),

	LOCTEXT("Work (Consume)", "Work (Consume)"),
	LOCTEXT("Fill Input", "Fill Input"),
	LOCTEXT("Work (Produce)", "Work (Produce)"),
	LOCTEXT("Work (Construct)", "Work (Construct)"),
	LOCTEXT("Hunt", "Hunt"),
	LOCTEXT("Ranch", "Ranch"),

	LOCTEXT("Seeding (Farm)", "Seeding (Farm)"), // FarmSeeding
	LOCTEXT("Nourishing (Farm)", "Nourishing (Farm)"), // FarmNourishing
	LOCTEXT("Harvesting (Farm)", "Harvesting (Farm)"), // FarmHarvesting
	LOCTEXT("Clear Drops (Farm)", "Clear Drops (Farm)"), // FarmClearDrops

	LOCTEXT("Intercity Delivery", "Intercity Delivery"),
	LOCTEXT("Immigrate", "Immigrate"),

	LOCTEXT("Move Army", "Move Army"),

	LOCTEXT("Go to Workplace", "Go to Workplace"),

	LOCTEXT("Other", "Other"),
};
#undef LOCTEXT_NAMESPACE


enum class UnitAIClassEnum : uint8
{
	UnitStateAI,
	HumanStateAI,
	ProjectileArrow,
};


enum class TryWorkFailEnum : uint8
{
	None,

	// Shared
	WorkplaceInventoryFull,
	RequireWorkplaceSetup,
	WorkplaceInaccessible,
	WaitingForAnotherWorkerToClearWorkplaceOutputInventory,
	WorkplaceOutputTargetReached,
	CannotAcquireWorkplaceInput,

	// TryConstruct
	WaitingForConstructionAreaToBeCleared,
	TargetConstructionInaccessible,
	CannotAccessResourcesInTargetConstruction,
	WaitingForAnotherCitizenToFulfillConstructionResources,
	CannotAcquireNeededConstructionResources,
	ConstructionSiteNoLongerNeedHelp,

	// TryGatherFruit
	TargetTreeInaccessible,

	// TryHunt
	NoWildAnimalsNearby,

	// TryRanch
	NeedAnimalInRanch,
	WaitingForAnimalsToGrow,

	// TryFarm
	FarmIsOutOfSeason,
	WaitingForAnotherFarmerToFinishSeeding,
	WaitingForAnotherFarmerToNourish,
	WaitingForAnotherFarmerToHarvest,

	// TryBulkHaul_Intercity
	NeedRoadConnectionBetweenIntercityLogisticsHubAndTargetTownhall,
	NeedRoadConnectionBetweenCaravansaryAndTargetTownhall,

	// TryBulkHaul_IntercityWater
	CannotFindShippingRouteToTargetTown,

	Count,
};
#define LOCTEXT_NAMESPACE "TryWorkFailEnum"
const TArray<FText> TryWorkFailEnumName
{
	LOCTEXT("None", "None"),

	// Shared
	LOCTEXT("WorkplaceInventoryFull", "<Red>Workplace inventory full</>\n<Red>Build Hauling Services to help haul</>"),
	LOCTEXT("RequireWorkplaceSetup", "<Red>Require workplace setup</>"),
	LOCTEXT("WorkplaceInaccessible", "<Red>Workplace inaccessible</>"),
	LOCTEXT("WaitingForAnotherWorkerToClearWorkplaceOutputInventory", "Waiting for another worker to clear workplace output inventory"),
	LOCTEXT("WorkplaceOutputTargetReached", "Workplace output target reached"),
	LOCTEXT("CannotAcquireWorkplaceInput", "<Red>Cannot acquire workplace input</>"),

	// TryConstruct
	LOCTEXT("WaitingForConstructionAreaToBeCleared", "Waiting for construction area to be cleared"),
	LOCTEXT("TargetConstructionInaccessible", "<Red>Target construction inaccessible</>"),
	LOCTEXT("CannotAccessResourcesInTargetConstruction", "<Red>Cannot access resources in target construction</>"),
	LOCTEXT("WaitingForAnotherCitizenToFulfillConstructionResources", "Waiting for another citizen to fulfill construction resources"),
	LOCTEXT("CannotAcquireNeededConstructionResources", "<Red>Cannot acquire needed construction resources</>"),
	LOCTEXT("ConstructionSiteNoLongerNeedHelp", "Construction site no longer need help"),

	// TryGatherFruit
	LOCTEXT("TargetTreeInaccessible", "Target tree inaccessible"),

	// TryHunt
	LOCTEXT("NoWildAnimalsNearby", "<Red>No wild animals nearby</>"),

	// TryRanch
	LOCTEXT("NeedAnimalInRanch", "<Red>Need animal in Ranch</>"),
	LOCTEXT("WaitingForAnimalsToGrow", "Waiting for animals to grow"),

	// TryFarm
	LOCTEXT("FarmIsOutOfSeason", "Farm is out of season"),
	LOCTEXT("WaitingForAnotherFarmerToFinishSeeding", "Waiting for another farmer to finish seeding"),
	LOCTEXT("WaitingForAnotherFarmerToNourish", "Waiting for another farmer to nourish"),
	LOCTEXT("WaitingForAnotherFarmerToHarvest", "Waiting for another farmer to harvest"),

	// TryBulkHaul_Intercity
	LOCTEXT("NeedRoadConnectionBetweenIntercityLogisticsHubAndTargetTownhall", "Need Road Connection between Intercity Logistics Hub and target Townhall"),
	LOCTEXT("NeedRoadConnectionBetweenCaravansaryAndTargetTownhall", "Need Road Connection between Caravansary and target Townhall"),
	
	// TryBulkHaul_IntercityWater
	LOCTEXT("CannotFindShippingRouteToTargetTown", "Cannot find shipping route to target town"),
};
static const FText& GetTryWorkFailEnumName(TryWorkFailEnum tryWorkFailEnum) {
	return TryWorkFailEnumName[static_cast<int>(tryWorkFailEnum)];
}
#undef LOCTEXT_NAMESPACE


class UnitStateAI
{
public:
	UnitStateAI() = default;
	virtual ~UnitStateAI() = default;

	virtual void AddUnit(UnitEnum unitEnum, int32 townId, UnitFullId fullId, IUnitDataSource* unitData, IGameSimulationCore* simulation);

	void Update();
	virtual void CalculateActions();

	virtual void OnUnitUpdate() {}

	// Don't call this directly... Call sim's reset actions
	void ResetActions_UnitPart(int32 waitTicks);

	void NextAction(UnitUpdateCallerEnum caller) {
		_unitData->SetNextTickState(_id, TransformState::NeedActionUpdate, caller);
	}
	void NextAction(int tickCount, UnitUpdateCallerEnum caller) {
		_unitData->SetNextTickState(_id, TransformState::NeedActionUpdate, caller, tickCount);
	}

public:
	/*
	 * Actions
	 */
	enum class ActionEnum : uint8
	{
		None,
		
		Wait,
		//PlayAnimation,
		//StopAnimation,
		
		MoveRandomly,
		MoveRandomlyAnimal,
		MoveRandomlyPerlin,
		
		GatherFruit,
		TrimFullBush,

		HarvestTileObj,
		PlantTree,
		NourishTree,

		MoveTo,
		MoveToResource,
		MoveInRange,
		//MoveToForceLongDistance,
		MoveToRobust,
		MoveToward,
		MoveToCaravan,
		MoveToShip,

		Produce,
		Construct,

		Eat,
		Heat,
		UseMedicine,
		UseTools,

		HaveFun,

		PickupResource,
		DropoffResource,
		DropInventoryAction,
		StoreGatheredAtWorkplace,
		FillInputs,

		IntercityHaulPickup,
		IntercityHaulDropoff,

		CaravanGiveMoney,

		PickupFoodAnimal,
		DropoffFoodAnimal,

		// HumanAI
		AttackOutgoing,
		SearchAreaForDrop,

		DoConsumerWork,
		DoFarmWork,

		TryForestingPlantAction,
	};

	struct Action
	{
		ActionEnum actionEnum;

		int32 int32val1;
		int32 int32val2;
		int32 int32val3;
		int32 int32val4;

		UnitFullId fullId1;

		
		Action(ActionEnum actionEnum = ActionEnum::None, int32 int32val1 = -1, int32 int32val2 = -1, int32 int32val3 = -1, int32 int32val4 = -1, UnitFullId fullId1 = UnitFullId()) :
			actionEnum(actionEnum),
			int32val1(int32val1),
			int32val2(int32val2),
			int32val3(int32val3),
			int32val4(int32val4),
			fullId1(fullId1)
		{}

		void operator>>(FArchive& Ar) {
			Ar << actionEnum;
			Ar << int32val1;
			Ar << int32val2;
			Ar << int32val3;
			Ar << int32val4;

			fullId1 >> Ar;
		}
	};

	void AddAction(ActionEnum actionEnum, int32 int32val1 = -1, int32 int32val2 = -1, int32 int32val3 = -1, int32 int32val4 = -1, UnitFullId fullId1 = UnitFullId()) {
		_actions.push_back(Action(actionEnum, int32val1, int32val2, int32val3, int32val4, fullId1));
	}

	// Note: Front action is the last one to execute
	void AddActionFront(ActionEnum actionEnum, int32 int32val1 = -1, int32 int32val2 = -1, int32 int32val3 = -1, int32 int32val4 = -1, UnitFullId fullId1 = UnitFullId()) {
		_actions.insert(_actions.begin(), Action(actionEnum, int32val1, int32val2, int32val3, int32val4, fullId1));
	}

	int32 GetSyncHash_Actions() const
	{
		int32 hash = 0;
		hash += _id;
		hash += _fullId.birthTicks;
		hash += _townId;
		hash += _playerId;
		hash += static_cast<int32>(_unitEnum);
		hash += static_cast<int32>(_unitState);
		hash += _food;
		hash += _health;
		hash += _heat;
		hash += _lastTickCelsiusRate;
		hash += _lastUpdateTick;
		hash += _lastPregnant;
		hash += _nextPregnantTick;

		hash += _inventory.resourceCountAll();

		hash += _houseId;
		hash += _workplaceId;
		hash += _lastworkplaceId;

		hash += _homeProvinceId;
		hash += _lastFindWildFoodTick;

		for (int32 i = 0; i < reservations.size(); i++)
		{
			hash += reservations[i].unitId;
			hash += static_cast<int32>(reservations[i].reservationType);
			hash += static_cast<int32>(reservations[i].reserveHolder.resourceEnum);
			hash += reservations[i].reserveHolder.holderId;
			hash += reservations[i].reserveTileId;
			hash += reservations[i].reserveWorkplaceId;
			hash += reservations[i].amount;
		}

		hash += _hp100;
		hash += _isSick;
		hash += _nextToolNeedTick;

		hash += _justDidResetActions;

		hash += static_cast<int32>(_animationEnum);

		for (int32 i = 0; i < _actions.size(); i++) {
			hash += static_cast<int32>(_actions[i].actionEnum);
			hash += _actions[i].int32val1;
			hash += _actions[i].int32val2;
		}
		return hash;
	}


	Action& action() { return _currentAction; }
	virtual void ExecuteAction()
	{
		LEAN_PROFILING_U(ExecuteAction);
		
#define CASE(Action) case ActionEnum::Action: Action(); break;
		switch (_currentAction.actionEnum)
		{
			CASE(Wait);
			//CASE(PlayAnimation);
			//CASE(StopAnimation);
			
			CASE(MoveRandomly);
			CASE(MoveRandomlyAnimal);
			CASE(MoveRandomlyPerlin);
			
			CASE(GatherFruit);
			CASE(TrimFullBush);

			CASE(HarvestTileObj);
			CASE(PlantTree);
			CASE(NourishTree);

			CASE(MoveTo);
			CASE(MoveToResource);
			CASE(MoveInRange);
			//CASE(MoveToForceLongDistance);
			CASE(MoveToRobust);
			CASE(MoveToward);
			CASE(MoveToCaravan);
			CASE(MoveToShip);

			CASE(Produce);
			CASE(Construct);

			CASE(Eat);
			CASE(Heat);
			CASE(UseTools);
			CASE(UseMedicine);

			CASE(HaveFun);

			CASE(PickupResource);
			CASE(DropoffResource);
			CASE(DropInventoryAction);
			CASE(StoreGatheredAtWorkplace);
			CASE(FillInputs);

			CASE(IntercityHaulPickup);
			CASE(IntercityHaulDropoff);
			CASE(CaravanGiveMoney);
			
			CASE(PickupFoodAnimal);
			CASE(DropoffFoodAnimal);

		default:
			UE_DEBUG_BREAK();
		}
#undef CASE
	}

	void Add_Wait(int32 tickCount, UnitAnimationEnum animationEnum = UnitAnimationEnum::Wait);		void Wait();

	//void Add_PlayAnimation(UnitAnimationEnum animationEnum);	void PlayAnimation();
	//void Add_StopAnimation();									void StopAnimation();
	
	void Add_MoveRandomly(TileArea area = TileArea::Invalid);	void MoveRandomly(); // Use MoveTo()
	void Add_MoveRandomlyAnimal();	void MoveRandomlyAnimal();
	void Add_MoveRandomlyPerlin(TileArea area);	void MoveRandomlyPerlin();
	
	void Add_GatherFruit(WorldTile2 targetTile);	void GatherFruit();
	void Add_TrimFullBush(WorldTile2 targetTile);	void TrimFullBush();

	void Add_HarvestTileObj(WorldTile2 targetTile);						void HarvestTileObj();
	void Add_PlantTree(WorldTile2 targetTile, TileObjEnum tileObjEnum);	void PlantTree();
	void Add_NourishTree(WorldTile2 targetTile);						void NourishTree();

	void Add_MoveTo(WorldTile2 end, int32 customFloodDistance = -1, UnitAnimationEnum animationEnum = UnitAnimationEnum::Walk);
	void MoveTo();
	bool MoveTo(WorldTile2 end, int32 customFloodDistance = -1, UnitAnimationEnum animationEnum = UnitAnimationEnum::Walk);

	void Add_MoveToResource(ResourceHolderInfo holderInfo, int32 customFloodDistance = -1, UnitAnimationEnum animationEnum = UnitAnimationEnum::Walk); // Use MoveTo()
	void MoveToResource();
	bool MoveToResource(ResourceHolderInfo holderInfo, int32 customFloodDistance, UnitAnimationEnum animationEnum);

	void Add_MoveInRange(WorldTile2 end, int32_t range);			void MoveInRange(); // TODO: REmove??
	//void Add_MoveToForceLongDistance(WorldTile2 end);				void MoveToForceLongDistance();
	void Add_MoveToRobust(WorldTile2 end);							void MoveToRobust();	void MoveToRobust(WorldTile2 end);
	void Add_MoveToward(WorldAtom2 end, int32 fraction100000, UnitAnimationEnum animationEnum = UnitAnimationEnum::Walk);	void MoveToward();
	//void Add_MoveToStraight(WorldAtom2 end);	void MoveToStraight(); // TODO: need save break
	void Add_MoveToCaravan(WorldTile2 end, UnitAnimationEnum animationEnum); bool MoveToCaravan();
	void Add_MoveToShip(int32 startPortId, int32 endPortId, UnitAnimationEnum animationEnum); bool MoveToShip();

	void MoveTo_NoFail(WorldTile2 end, int32 customFloodDistance); // Try MoveTo, if failed, do MoveToRobust

	void Add_Produce(int32 workManSec100, int32 waitTicks, int32 timesLeft, int32 workplaceId);		void Produce();
	void Add_Construct(int32 workManSec100, int32 waitTicks, int32 timesLeft, int32 workplaceId);	void Construct();

	void Add_Eat(ResourceEnum resourceEnum);	virtual void Eat();
	void Add_Heat();							void Heat();
	void Add_UseMedicine(ResourceEnum resourceEnum);void UseMedicine();
	void Add_UseTools(ResourceEnum resourceEnum);void UseTools();

	void Add_HaveFun(int32_t funBuildingId);	void HaveFun();

	//! Resources Actions
	void Add_PickupResource(ResourceHolderInfo info, int amount);		void PickupResource();
	void Add_DropoffResource(ResourceHolderInfo info, int amount);		void DropoffResource();
	void Add_DropInventoryAction();										void DropInventoryAction();
	void Add_StoreGatheredAtWorkplace();								void StoreGatheredAtWorkplace();
	void Add_FillInputs(int32 workplaceId);								void FillInputs();

	void Add_IntercityHaulPickup(int32 workplaceId, int32 townId);		void IntercityHaulPickup();
	void Add_IntercityHaulDropoff(int32 workplaceId);					void IntercityHaulDropoff();

	void Add_CaravanGiveMoney(int32 workplaceId, int32 targetBuildingId, bool isGivingTarget);		void CaravanGiveMoney();

	void Add_PickupFoodAnimal();		void PickupFoodAnimal();
	void Add_DropoffFoodAnimal();		void DropoffFoodAnimal();

	virtual void Die();

	//void DropItemsOnDeath();

	/*
	 * States
	 */
	// Previous: 240s or ~1.5 round of food... 0 food instant death ... warn 0.75 round before.. 0.
	// Now: 1 round of food ... after 0 food, 0.5 round ave before dead (0 - 1 round randomized) .. warn 0.75 round before ave death
	int32 food() { return _food; }
	int32 maxFood() {
		return unitInfo().maxFoodTicks;
	}
	int32 minWarnFood() {
		return maxFood() / 4; //  warn 0.75 round before ave death... 0.25 + 0.5 ave death time
	}
	int32 foodThreshold_Get2() {
		return maxFood() / 2;
	}
	int32 foodThreshold_Get() {
		return maxFood() * 3 / 4;
	}

	static int32 minWarnFoodPercent() { return 25; }
	static int32 foodThreshold_Get2Percent() { return 50; }
	
	
	void SetFood(int32 food) {
		_food = food;
	}
	

	int32 heat() { return _heat; }
	int32 maxHeat() { return unitInfo().maxHeatCelsiusTicks; }
	int32 heatGetThreshold() { return maxHeat() * 7 / 10; } // maxHeat() * 3 / 4;
	int32 minWarnHeat() { return maxHeat() / 2; }

	static int32 heatGetThresholdPercent() { return 75; }
	static int32 minWarnHeatPercent() { return 50; }

	int32 ticksSinceLastUpdate() { return Time::Ticks() - _lastUpdateTick; }
	int32 foodActual()
	{
		// Help construction issue
		if (Building* workplc = workplace()) {
			if (!workplc->isConstructed() && workplc->buildingTooLongToConstruct()) {
				return std::max(foodThreshold_Get() + 1, _food - ticksSinceLastUpdate());
			}
		}
		return std::max(0, _food - ticksSinceLastUpdate());
	}
	int32 heatActual()
	{
		int32 currentHeat = _heat + ticksSinceLastUpdate() * _lastTickCelsiusRate;
		if (currentHeat > maxHeat()) {
			return maxHeat();
		}
		
		// Help construction issue
		if (Building* workplc = workplace()) {
			if (!workplc->isConstructed() && workplc->buildingTooLongToConstruct()) {
				return std::max(heatGetThreshold() + 1, currentHeat);
			}
		}
		return std::max(0, currentHeat);
	}

	void SetFunTicks(int32 funTicks) {
		for (int32 i = 0; i < _serviceToFunTicks.size(); i++) {
			_serviceToFunTicks[i] = funTicks;
		}
	}

	virtual int32 happinessOverall() { return 100; }
	

	int32 hp() { return _hp100 / 100; }
	int32 maxHP100() const { return 10000; }
	
	bool isSick() { return _isSick; }
	void SetSick(bool isSick) { _isSick = isSick; }
	
	int32 age() { return Time::Ticks() - birthTicks(); }
	int32 maxAge()
	{
		if (unitEnum() == UnitEnum::Human) {
			int32 averageDeathAgeTicks = PlayerParameters::DeathAgeTicks();

			// maxAge randomized depending the birthTick
			int32 deathAgeVariation = averageDeathAgeTicks / 5;

			return averageDeathAgeTicks + GameRand::Rand(birthTicks()) % deathAgeVariation;
		}
		return unitInfo().maxAgeTicks;
	}

	int32 townId() { return _townId; }
	int32 playerId() { return _playerId; }
	
	UnitState unitState() { return _unitState; }
	void SetActivity(UnitState unitState) {
		_unitState = unitState;

#if DEBUG_BUILD
		_activityHistory.push_back(_unitState);
#endif
	}

	UnitInventory& inventory() { return _inventory; }

	int nextActiveTick() const { return _unitData->nextActiveTick(_id); }

	int32 id() const { return _id; }
	UnitFullId fullId() const { return _unitData->fullId(_id); }

	int32 birthTicks() const { return _fullId.birthTicks; }

	int32 birthChance();

	//void SetPlayerId(int32_t playerId) { _playerId = playerId; }

	//! Helpers
	UnitEnum unitEnum() { return _unitEnum; }
	int32 unitEnumInt() const { return static_cast<int32_t>(_unitEnum); }
	bool isEnum(UnitEnum inUnitEnum) { return unitEnum() == inUnitEnum; }
	UnitInfo unitInfo() const { return UnitInfos[unitEnumInt()]; }

	WorldTile2 unitTile() { return _unitData->atomLocation(_id).worldTile2(); }
	WorldAtom2 unitAtom() { return _unitData->atomLocation(_id); }
	

	template<class T>
	T& subclass(UnitEnum unitEnumIn) { 
		check(unitEnumIn == unitEnum());
		return *static_cast<T*>(this); 
	}

	template<class T>
	T& subclass() { return *static_cast<T*>(this); }

	bool IsMoveValid(WorldTile2 tile, int32 customFloodDistance = -1)
	{
		if (!tile.isValid()) {
			return false;
		}
		if (customFloodDistance == -1) {
			customFloodDistance = unitMaxFloodDistance();
		}
		DEBUG_ISCONNECTED_VAR(IsMoveValid);

		bool isIntelligent = IsIntelligentUnit(unitEnum());
		bool isMoveValid = _simulation->IsConnected(unitTile(), tile, customFloodDistance);

		// For human, test the second time going to townhall first then going to target.
		//  This allow for a lot longer travel check range.
		//  (Otherwise, IsMoveValid fail could be because IsConnectedBuilding is only rough estimate from townhall)
		if (!isMoveValid && isIntelligent && _townId != -1) {
			WorldTile2 gateTile = _simulation->GetMajorTownhallGateFast(_townId);
			isMoveValid = _simulation->IsConnected(gateTile, tile, customFloodDistance) &&
							_simulation->IsConnected(gateTile, unitTile(), customFloodDistance);
		}
		
		return isMoveValid;
	}
	bool IsResourceMoveValid(ResourceHolderInfo info) {
		if (!info.isValid()) {
			return false;
		}
		DEBUG_ISCONNECTED_VAR(ResourceMoveValid);
		WorldTile2 tile = dropoffBuilding(info).gateTile();
		return _simulation->IsConnected(unitTile(), tile, unitMaxFloodDistance());
	}

	class Building& dropoffBuilding(ResourceHolderInfo dropoffInfo) {
		int dropoffObjectId = resourceSystem().holder(dropoffInfo).objectId;
		return _simulation->building(dropoffObjectId);
	}

	// Used for final check before moving (not in Try())
	int32 unitMaxFloodDistance() {
		if (isEnum(UnitEnum::Human)) {
			return GameConstants::MaxFloodDistance_HumanLogistics;
		}
		return GameConstants::MaxFloodDistance_AnimalFar;
	}

	//! Combat
	void AttackIncoming(UnitFullId attacker, int32 ownerWorkplaceId, int32_t damage, int32_t attackPlayerId = -1, UnitFullId defenderId = UnitFullId::Invalid());

	//! Debug
	std::string debugSpeech(bool includeLast = true) const { 
		if (includeLast) {
			std::stringstream ss;
			for (size_t i = _lastDebugSpeeches.size(); i-- > 0;) {
				ss << "\n>>>>> lastDebugSpeech[" << i << "]:" << _lastDebugSpeeches[i];
			}
			return _debugSpeech.str() + "\n>>>>> lastDebugSpeech:" + ss.str();
		}
		return _debugSpeech.str(); 
	}
	std::string debugStr() const {
		std::stringstream ss;
		ss << "\n\n\n\n\n";
		ss << "[>>>>>>>>" << unitInfo().nameStr() << "<<<<<<<<] unitId:" << _id << ", transformState:" << TransformStateLabel(_unitData->transformState(_id))
			<< ", workplace:" << workplaceId()
			<< ", nextActiveTick:" << nextActiveTick() << ", Time:" << Time::Ticks()
			<< _inventory.ToString()
			<< "\n>>>>> debugSpeech:\n" << debugSpeech(true) << "\n-----";
		return ss.str();
	}

	std::string compactStr()
	{
		std::stringstream ss;
		ss << "[Animal fid:" << fullId().ToString() << " tid:" << _townId << " birth:" << birthTicks() << " house:" << _houseId << " name: "
			<< GetUnitName() << " alive:" << _simulation->unitAlive(fullId()) << " ticks:" << Time::Ticks() << "]";
		return ss.str();
	}

	void AddDebugSpeech(std::string message);
	

	int32 lastActionTick() { return _lastUpdateTick; }

	//! Jobs	
	int32 workplaceId() const { return _workplaceId; }
	void SetWorkplaceId(int32_t workplaceId) {
		AddDebugSpeech("SetWorkplaceId: last:" + std::to_string(_lastworkplaceId) + ", cur:" + std::to_string(workplaceId));
		_lastworkplaceId = _workplaceId;
		_workplaceId = workplaceId;

		_tryWorkFailEnum = TryWorkFailEnum::None;
	}

	class Building* workplace() {
		return _workplaceId != -1 ? &_simulation->building(_workplaceId) : nullptr;
	}

	//! House
	int32 houseId() { return _houseId; }
	void SetHouseId(int32 houseId) {
		//PUN_LOG("SetHouseId %s", ToTChar(compactStr()));
		_houseId = houseId; 
	}

	int32 homeProvinceId() { return _homeProvinceId; }

	//! Needs
	bool needHouse() { return _houseId == -1 && _simulation->populationTown(_townId) > _simulation->HousingCapacity(_townId); }
	bool showNeedFood() {
		return _food < minWarnFood();
	}
	bool showNeedHeat() {
		return _heat < minWarnHeat();
	}
	
	int32 lastPregnantTick() { return _lastPregnant; }

	bool isMale() {
		return GameRand::Rand(_id + birthTicks()) % 2;
	}

	virtual bool isBelowWorkingAge() {
		return age() < unitInfo().miniAgeTicks();
	}
	virtual bool isChild() {
		return age() < unitInfo().miniAgeTicks();
	}
	virtual FText GetTypeName();

	std::string GetUnitName() {
		return ToStdString(GetUnitNameT().ToString());
	}
	FText GetUnitNameT() {
		int32 rand = GameRand::Rand(_id + birthTicks());
		return isMale() ? MaleNames[rand % MaleNames.Num()] : FemaleNames[rand % FemaleNames.Num()];
	}

	UnitAnimationEnum animationEnum() { return _animationEnum; }

	UnitAnimationEnum GetDisplayAnimationEnum(bool useFlashPrevention)
	{
		// This prevents flashes that happens with animation reset
		if (useFlashPrevention)
		{
			if (_lastAnimationEnum != UnitAnimationEnum::Wait &&
				Time::Ticks() - _lastAnimationChangeTick < 10)
			{
				return _lastAnimationEnum;
			}
		}
		return _animationEnum;
	}
	

	void ChangeTownOwningPlayer(int32 playerId) {
		_playerId = playerId;
		_simulation->ResetUnitActions(_id);

		// If outside territory, warp them back
		if (_simulation->tileOwnerTown(unitTile()) != _townId) {
			WorldTile2 townGate = _simulation->GetMajorTownhallGate(_townId);
			if (townGate.isValid()) {
				_unitData->MoveUnitInstantly(_id, townGate.worldAtom2());
			}
		}
	}

	TryWorkFailEnum tryWorkFailEnum() { return _tryWorkFailEnum; }
	
public:
	//! Serialize
	virtual UnitAIClassEnum classEnum() { return UnitAIClassEnum::UnitStateAI; }
	virtual void Serialize(FArchive& Ar)
	{
		Ar << _id;
		_fullId >> Ar;
		
		Ar << _townId;
		Ar << _playerId;
		Ar << _unitEnum;
		Ar << _unitState;

		SerializeVecObj(Ar, _actions);
		_currentAction >> Ar;

		Ar << _food;
		Ar << _health;

		Ar << _heat;
		Ar << _lastTickCelsiusRate;

		//Ar << _funTicks;
		SerializeVecValue(Ar, _serviceToFunTicks);

		SerializeVecValue(Ar, _happiness);
		

		Ar << _lastUpdateTick;
		Ar << _lastPregnant;
		Ar << _nextPregnantTick;

		_inventory >> Ar;

		Ar << _houseId;
		Ar << _workplaceId;
		Ar << _lastworkplaceId;

		Ar << _homeProvinceId;
		Ar << _lastFindWildFoodTick;

		SerializeVecObj(Ar, reservations);

		Ar << _hp100;
		Ar << _isSick;
		Ar << _nextToolNeedTick;

		Ar << _justDidResetActions;

		Ar << _animationEnum;
	}
	void LoadInit(IUnitDataSource* unitData, IGameSimulationCore* simulation) {
		_unitData = unitData;
		_simulation = simulation;
	}
	
protected:

	UnitReservation PopReservation(ReservationType reservationType); // Pop any reservation of type...
	UnitReservation PopReservationWorkplace(int32 workplaceId);
	
	UnitReservation PopReservation(int index);
	
	UnitReservation GetReservation(ReservationType reservationType) {
		for (int i = reservations.size(); i-- > 0;) {
			if (reservations[i].reservationType == reservationType) {
				return reservations[i];
			}
		}
		PUN_NOENTRY();
		return UnitReservation();
	}
	UnitReservation GetReservation(int32 workplaceId) {
		for (int i = reservations.size(); i-- > 0;) {
			if (reservations[i].reserveWorkplaceId == workplaceId) {
				return reservations[i];
			}
		}
		PUN_NOENTRY();
		return UnitReservation();
	}
	

	std::string ReservationsToString() {
		std::stringstream ss;
		ss << "{";
		for (int i = reservations.size(); i-- > 0;) {
			ss << reservations[i].ToString() << ", ";
		}
		ss << "}";
		return ss.str();
	}

	// For State AI, always call Unit's reservation function (not building or resource system)
	void ReserveResource(ReservationType type, ResourceHolderInfo info, int32_t amount);

	void ReserveWork(int32 amount, int32 workplaceId = -1);
	void DoWork(int32 workAmount, int32 workplaceId = -1);

	void ReserveTreeTile(int32_t tileId);
	void ReserveFarmTile(const struct FarmTile& farmTile, int32 workplaceId);

	void CancelReservations() {
		for (int i = reservations.size(); i-- > 0;) {
			PopReservation(i);
		}
		reservations.clear();
	}


	void SetAnimation(UnitAnimationEnum animationEnum) {
		_lastAnimationEnum = _animationEnum;
		_animationEnum = animationEnum;
		_lastAnimationChangeTick = Time::Ticks();
	}
	void ResetAnimation() {
		SetAnimation(UnitAnimationEnum::Wait);
	}

	

	//! Helpers
	TreeSystem& treeSystem() { return _simulation->treeSystem(); }
	class ResourceSystem& resourceSystem() {
		return _simulation->resourceSystem(_townId);
	}
	class TownManager& townManager() {
		return _simulation->townManager(_townId);
	}
	
	PlayerParameters& playerParameters() {
		PlayerParameters* playerParams = _simulation->parameters(_playerId);
		PUN_CHECK(playerParams);
		return *playerParams;
	}
	class SubStatSystem& statSystem() {
		return _simulation->statSystem(_townId);
	}

	void PushLastDebugSpeech() {
		_lastDebugSpeeches.push_back(_debugSpeech.str());
		if (_lastDebugSpeeches.size() > 5) {
			_lastDebugSpeeches.erase(_lastDebugSpeeches.begin());
		}
		_debugSpeech.str("");
	}

	
	void WorkFailed(TryWorkFailEnum workFailEnum) {
		_tryWorkFailEnum = workFailEnum;
	}

protected:
	// Try
	bool TryAvoidOthers(); // TODO: replaced by territorial instinct
	bool TryFindWildFood();

	bool TryGetBurrowFood();
	bool TryStockBurrowFood();
	bool TryStoreAnimalInventory();
	bool TryGoHomeProvince();

	bool TryCheckBadTile();
	bool TryGoNearbyHome();

	bool TryChangeProvince_NoAction();
	bool IsBurrowBuildable(TileArea area, Direction faceDirection);

#if DEBUG_BUILD
	void CheckIntegrity()
	{
		_unitData->CheckIntegrity(_id);
		PUN_CHECK2(_unitData->unitLean(_id).alive(), debugStr());
	}
#endif

	bool justReset() { return _justDidResetActions; }

public:
	static int32 debugFindFullBushSuccessCount;
	static int32 debugFindFullBushFailCount;

	FString GetUnitDebugInfo() const
	{
		std::stringstream ss;
		ss << "_id: " << _id;
		ss << "\n birthTicks: " << birthTicks();
		ss << "\n _townId: " << _townId;
		ss << "\n _playerId: " << _playerId;

		ss << "\n _unitEnum: " << GetUnitInfo(_unitEnum).nameStr();
		ss << "\n _unitState: " << FTextToStd(UnitStateName[static_cast<int32>(_unitState)]);

		ss << "\n _actions: ";
		for (int32 i = 0; i < _actions.size(); i++) {
			ss << " " << static_cast<int32>(_actions[i].actionEnum);
		}

		ss << "\n _currentAction: " << static_cast<int32>(_currentAction.actionEnum);
		
		ss << "\n _food: " << _food;
		ss << "\n _health: " << _health;
		ss << "\n _heat: " << _heat;
		ss << "\n _lastTickCelsiusRate: " << _lastTickCelsiusRate;

		ss << "\n _happiness: ";
		for (int32 i = 0; i < _happiness.size(); i++) {
			ss << " " << _happiness[i];
		}

		ss << "\n _lastUpdateTick: " << _lastUpdateTick;
		ss << "\n _lastPregnant: " << _lastPregnant;
		ss << "\n _nextPregnantTick: " << _nextPregnantTick;

		ss << "\n _inventory: " << _inventory.ToString();

		ss << "\n _houseId: " << _houseId;
		ss << "\n _workplaceId: " << _workplaceId;
		ss << "\n _lastworkplaceId: " << _lastworkplaceId;

		ss << "\n _homeProvinceId: " << _homeProvinceId;
		ss << "\n _lastFindWildFoodTick: " << _lastFindWildFoodTick;

		ss << "\n reservations:";
		for (int32 i = 0; i < reservations.size(); i++) {
			ss << " " << ReservationTypeName(reservations[i].reservationType);
		}

		ss << "\n _hp100: " << _hp100;
		ss << "\n _isSick: " << _isSick;
		ss << "\n _nextToolNeedTick: " << _nextToolNeedTick;

		ss << "\n _justDidResetActions: " << _justDidResetActions;
		ss << "\n _animationEnum: " << GetUnitAnimationName(_animationEnum);

		return ToFString(ss.str());
	}

	FString GetUnitActivityHistory() const
	{
		std::stringstream ss;
		ss << "_activityHistory:";
#if DEBUG_BUILD
		int32 count = 0;
		for (int32 i = _activityHistory.size() - 1; i-- > 0;) {
			ss << "\n  " << FTextToStd(UnitStateName[static_cast<int32>(_activityHistory[i])]);
			if (count++ > 30) {
				break;
			}
		}
#endif
		return ToFString(ss.str());
	}

protected:
	IUnitDataSource* _unitData;
	IGameSimulationCore* _simulation;

	std::stringstream _debugSpeech;
	std::vector<std::string> _lastDebugSpeeches;

#if DEBUG_BUILD
	std::vector<UnitState> _activityHistory;
	std::vector<Action> _actionHistory;
#endif

	TryWorkFailEnum _tryWorkFailEnum; // TODO: Serialize

	int32 _lastAnimationChangeTick = 0;
	UnitAnimationEnum _lastAnimationEnum = UnitAnimationEnum::Wait;

protected:
	/*
	 * Serialize Variables
	 */
	int32 _id;
	UnitFullId _fullId;
	
	int32 _townId;
	int32 _playerId;
	
	UnitEnum _unitEnum;
	UnitState _unitState;
	
	std::vector<Action> _actions;
	Action _currentAction;

	int32 _food;
	int32 _health;
	
	int32 _heat;
	int32 _lastTickCelsiusRate;

	//int32 _funTicks;
	std::vector<int32> _serviceToFunTicks;

	std::vector<int32> _happiness;
	
	
	int32 _lastUpdateTick; // This is for UnitAI's Update. There is another one for UnitLean
	int32 _lastPregnant;
	int32 _nextPregnantTick;

	UnitInventory _inventory;

	int32 _houseId;
	int32 _workplaceId;
	int32 _lastworkplaceId;

	int32 _homeProvinceId = -1;
	int32 _lastFindWildFoodTick = 0;

	std::vector<UnitReservation> reservations;

	int32 _hp100;
	bool _isSick;
	int32 _nextToolNeedTick;

	bool _justDidResetActions;

	UnitAnimationEnum _animationEnum = UnitAnimationEnum::Wait;
};