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
	GatherBerry,
	GatherBush,
	
	WorkConsume,
	FillInput,
	WorkProduce,
	WorkConstruct,
	Hunt,
	
	FarmSeeding,
	FarmNourishing,
	FarmHarvesting,
	FarmClearDrops,

	MoveArmy,

	GoToWorkplace,

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
	LOCTEXT("Gather Berry", "Gather Berry"),
	LOCTEXT("Gather Bush", "Gather Bush"),
	
	LOCTEXT("Work (Consume)", "Work (Consume)"),
	LOCTEXT("Fill Input", "Fill Input"),
	LOCTEXT("Work (Produce)", "Work (Produce)"),
	LOCTEXT("Work (Construct)", "Work (Construct)"),
	LOCTEXT("Hunt", "Hunt"),
	
	LOCTEXT("Seeding (Farm)", "Seeding (Farm)"), // FarmSeeding
	LOCTEXT("Nourishing (Farm)", "Nourishing (Farm)"), // FarmNourishing
	LOCTEXT("Harvesting (Farm)", "Harvesting (Farm)"), // FarmHarvesting
	LOCTEXT("Clear Drops (Farm)", "Clear Drops (Farm)"), // FarmClearDrops

	LOCTEXT("Move Army", "Move Army"),

	LOCTEXT("Go to Workplace", "Go to Workplace"),
};

#undef LOCTEXT_NAMESPACE


enum class UnitAIClassEnum : uint8
{
	UnitStateAI,
	HumanStateAI,
	ProjectileArrow,
};

class UnitStateAI
{
public:
	UnitStateAI() = default;
	virtual ~UnitStateAI() = default;

	virtual void AddUnit(UnitEnum unitEnum, int32 playerId, UnitFullId fullId, IUnitDataSource* unitData, IGameSimulationCore* simulation);

	void Update();
	virtual void CalculateActions();

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
		MoveToForceLongDistance,
		MoveToRobust,
		MoveToward,

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

	//const std::vector<std::string> UnitActionEnumString = {
	//	"Wait",
	//	"MoveRandomly",
	//	"GatherFruit",
	//	"TrimFullBush",
	//};

	Action& action() { return _currentAction; }
	virtual void ExecuteAction()
	{
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
			CASE(MoveToForceLongDistance);
			CASE(MoveToRobust);
			CASE(MoveToward);

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
	
	void Add_MoveRandomly(TileArea area = TileArea::Invalid);	void MoveRandomly();
	void Add_MoveRandomlyAnimal();	void MoveRandomlyAnimal();
	void Add_MoveRandomlyPerlin(TileArea area);	void MoveRandomlyPerlin();
	
	void Add_GatherFruit(WorldTile2 targetTile);	void GatherFruit();
	void Add_TrimFullBush(WorldTile2 targetTile);	void TrimFullBush();

	void Add_HarvestTileObj(WorldTile2 targetTile);						void HarvestTileObj();
	void Add_PlantTree(WorldTile2 targetTile, TileObjEnum tileObjEnum);	void PlantTree();
	void Add_NourishTree(WorldTile2 targetTile);						void NourishTree();

	void Add_MoveTo(WorldTile2 end, int32 customFloodDistance = -1);	void MoveTo();  bool MoveTo(WorldTile2 end, int32 customFloodDistance = -1);
	void Add_MoveToResource(ResourceHolderInfo holderInfo, int32 customFloodDistance = -1);			void MoveToResource(); bool MoveToResource(ResourceHolderInfo holderInfo, int32 customFloodDistance);
	void Add_MoveInRange(WorldTile2 end, int32_t range);			void MoveInRange(); // TODO: REmove??
	void Add_MoveToForceLongDistance(WorldTile2 end);				void MoveToForceLongDistance();
	void Add_MoveToRobust(WorldTile2 end);							void MoveToRobust();	void MoveToRobust(WorldTile2 end);
	void Add_MoveToward(WorldAtom2 end, int32 fraction100000);	void MoveToward();

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
	void Add_FillInputs(int32 workplaceId);												void FillInputs();

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

	int32 heat() { return _heat; }
	int32 maxHeat() { return unitInfo().maxHeatCelsiusTicks; }
	int32 minWarnHeat() { return maxHeat() / 2; }

	int32 ticksSinceLastUpdate() { return Time::Ticks() - _lastUpdateTick; }
	int32 foodActual() {
		return std::max(0, _food - ticksSinceLastUpdate());
	}
	int32 heatActual() {
		int32 currentHeat = _heat + ticksSinceLastUpdate() * _lastTickCelsiusRate;
		if (currentHeat > maxHeat()) {
			return maxHeat();
		}
		return std::max(0, currentHeat);
	}
	int32 funTicksActual() {
		return std::max(0, _funTicks - ticksSinceLastUpdate());
	}

	void SetFunTicks(int32 funTicks) {
		_funTicks = funTicks;
	}

	int32 hp() { return _hp100 / 100; }
	int32 maxHP100() const { return 10000; }
	bool isSick() { return _isSick; }

	int32 age() { return Time::Ticks() - birthTicks(); }
	int32 maxAge()
	{
		if (unitEnum() == UnitEnum::Human) {
			return _simulation->parameters(_playerId)->DeathAgeTicks();
		}
		return unitInfo().maxAgeTicks;
	}

	int32 playerId() { return _playerId; }
	UnitState unitState() { return _unitState; }

	UnitInventory& inventory() { return _inventory; }

	int nextActiveTick() const { return _unitData->nextActiveTick(_id); }

	int32 id() const { return _id; }
	UnitFullId fullId() const { return _unitData->fullId(_id); }

	int32 birthTicks() { return _fullId.birthTicks; }

	int32 birthChance();

	void SetPlayerId(int32_t playerId) { _playerId = playerId; }

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
		bool isMoveValid = _simulation->IsConnected(unitTile(), tile, customFloodDistance, isIntelligent);;

		// For human, test the second time going to townhall first then going to target.
		//  This allow for a lot longer travel check range.
		//  (Otherwise, IsMoveValid fail could be because IsConnectedBuilding is only rough estimate from townhall)
		if (!isMoveValid && isIntelligent && _playerId != -1) {
			WorldTile2 gateTile = _simulation->townhallGateTile(_playerId);
			isMoveValid = _simulation->IsConnected(gateTile, tile, customFloodDistance, isIntelligent) &&
							_simulation->IsConnected(gateTile, unitTile(), customFloodDistance, isIntelligent);
		}
		
		return isMoveValid;
	}
	bool IsResourceMoveValid(ResourceHolderInfo info) {
		if (!info.isValid()) {
			return false;
		}
		DEBUG_ISCONNECTED_VAR(ResourceMoveValid);
		WorldTile2 tile = dropoffBuilding(info).gateTile();
		return _simulation->IsConnected(unitTile(), tile, unitMaxFloodDistance(), IsIntelligentUnit(unitEnum()));
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
		ss << "[>>>>>>>>" << unitInfo().name << "<<<<<<<<] unitId:" << _id << ", transformState:" << TransformStateLabel(_unitData->transformState(_id))
			<< ", workplace:" << workplaceId()
			<< ", nextActiveTick:" << nextActiveTick() << ", Time:" << Time::Ticks()
			<< _inventory.ToString()
			<< "\n>>>>> debugSpeech:\n" << debugSpeech(true) << "\n-----";
		return ss.str();
	}

	std::string compactStr()
	{
		std::stringstream ss;
		ss << "[Animal fid:" << fullId().ToString() << " pid:" << _playerId << " birth:" << birthTicks() << " house:" << _houseId << " name: "
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
	bool needHouse() { return _houseId == -1 && _simulation->population(_playerId) > _simulation->HousingCapacity(_playerId); }
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

	
public:
	//! Serialize
	virtual UnitAIClassEnum classEnum() { return UnitAIClassEnum::UnitStateAI; }
	virtual void Serialize(FArchive& Ar)
	{
		Ar << _id;
		_fullId >> Ar;
		
		Ar << _playerId;
		Ar << _unitEnum;
		Ar << _unitState;

		SerializeVecObj(Ar, _actions);
		_currentAction >> Ar;

		Ar << _food;
		Ar << _health;

		Ar << _heat;
		Ar << _lastTickCelsiusRate;

		Ar << _funTicks;

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
	void ReserveFarmTile(int32_t tileId);

	void CancelReservations() {
		for (int i = reservations.size(); i-- > 0;) {
			PopReservation(i);
		}
		reservations.clear();
	}

	void ResetAnimation() {
		_animationEnum = UnitAnimationEnum::Wait;
	}

	//! Helpers
	TreeSystem& treeSystem() { return _simulation->treeSystem(); }
	class ResourceSystem& resourceSystem() {
		return _simulation->resourceSystem(_playerId);
	}
	PlayerParameters& playerParameters() {
		PlayerParameters* playerParams = _simulation->parameters(_playerId);
		PUN_CHECK(playerParams);
		return *playerParams;
	}
	class SubStatSystem& statSystem() {
		return _simulation->statSystem(_playerId);
	}

	void PushLastDebugSpeech() {
		_lastDebugSpeeches.push_back(_debugSpeech.str());
		if (_lastDebugSpeeches.size() > 5) {
			_lastDebugSpeeches.erase(_lastDebugSpeeches.begin());
		}
		_debugSpeech.str("");
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

protected:
	IUnitDataSource* _unitData;
	IGameSimulationCore* _simulation;

	std::stringstream _debugSpeech;
	std::vector<std::string> _lastDebugSpeeches;

	std::vector<Action> _actionHistory;
protected:
	/*
	 * Serialize Variables
	 */
	int32 _id;
	UnitFullId _fullId;
	
	int32 _playerId;
	UnitEnum _unitEnum;
	UnitState _unitState;
	
	std::vector<Action> _actions;
	Action _currentAction;

	int32 _food;
	int32 _health;
	
	int32 _heat;
	int32 _lastTickCelsiusRate;

	int32 _funTicks;
	
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

	UnitAnimationEnum _animationEnum;
};