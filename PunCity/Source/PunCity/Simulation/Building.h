#pragma once

#include "GameSimulationInfo.h"
#include "IGameSimulationCore.h"
#include "PlayerParameters.h"
#include "PunCity/PunUtils.h"
//#include "Buildings/Garrisons.h"

#include <algorithm>
#include <numeric>
#include <unordered_set>
#include "PunCity/AlgorithmUtils.h"
#include "Resource/ResourceSystem.h"
#include "GlobalResourceSystem.h"


enum class BuildingActionEnum : uint8
{
	DropoffResource,
	PickupResource,
	ConsumeLuxury,
	FillInput,
};
static const std::vector<std::string> BuildingActionEnums
{
	"DropoffResource",
	"PickupResource",
	"ConsumeLuxury",
	"FillInput",
};
struct BuildingDebugAction
{
	BuildingActionEnum actionEnum = BuildingActionEnum::DropoffResource;
	int32 var1 = -1;
	int32 var2 = -1;
	int32 var3 = -1;
};


class Building
{
public:
	virtual ~Building();

	void Init(IGameSimulationCore& simulation, int32 objectId, int32 townId, uint8_t buildingEnum, 
						TileArea area, WorldTile2 centerTile, Direction faceDirection);
	
	void LoadInit(IGameSimulationCore* simulation) {
		_simulation = simulation;
	}

	virtual void OnPreInit_IncludeMinorTown() {}
	virtual void OnPreInit() {}
	virtual void OnInit() {}
	virtual void OnDeinit() {}

	// Use workDone for construction
	virtual void FinishConstruction();
	
	virtual void SetResourceActive(bool workActive);
	void SetHolderTypeAndTarget(ResourceEnum resourceEnum, ResourceHolderType type, int32 target);

	void Deinit();

	virtual void OnProduce(int32 productionAmount) {}

	virtual void Tick() {}
	virtual void ScheduleTick() {}

	// Building space is tiles from centerTile
	WorldTile2 RotateBuildingTileByDirection(WorldTile2 bldTile) const {
		switch(_faceDirection) {
			case Direction::S: return bldTile;
			case Direction::E: {
				return WorldTile2(bldTile.y, -bldTile.x);
			}
			case Direction::N: {
				return WorldTile2(-bldTile.x, -bldTile.y);
			}
			case Direction::W: {
				return WorldTile2(-bldTile.y, bldTile.x);
			}
			default:
				UE_DEBUG_BREAK();
				return WorldTile2::Invalid;
		}
	}
	WorldTile2 UndoRotateBuildingTileByDirection(WorldTile2 bldTile) const {
		switch (_faceDirection) {
		case Direction::S: return bldTile;
		case Direction::E: {
			return WorldTile2(-bldTile.y, bldTile.x);
		}
		case Direction::N: {
			return WorldTile2(-bldTile.x, -bldTile.y);
		}
		case Direction::W: {
			return WorldTile2(bldTile.y, -bldTile.x);
		}
		default:
			UE_DEBUG_BREAK();
			return WorldTile2::Invalid;
		}
	}

	int32 playerId() const { return _playerId; }
	int32 townId() const { return _townId; }

	virtual void ChangeTownOwningPlayer(int32 playerId) {
		_playerId = playerId;
	}
	
	int32 buildingId() const { return _objectId; }
	uint8 buildingEnumInt() const { return static_cast<uint8_t>(_buildingEnum); }
	CardEnum buildingEnum() const { return _buildingEnum; }
	CardEnum classEnum() const { return _buildingEnum; }
	WorldTile2 centerTile() const { return _centerTile; }
	Direction faceDirection() const { return _faceDirection; }
	TileArea area() const { return _area; }
	TileArea frontArea() const { return _area.GetFrontArea(_faceDirection); }

	virtual int32 tileCount() { return _area.tileCount(); }

	static WorldTile2 GetPortTile(WorldTile2 gateTile, Direction faceDirection, CardEnum buildingEnum) {
		return gateTile + WorldTile2::RotateTileVector(WorldTile2(GetBuildingInfo(buildingEnum).size.x - 1, 0), faceDirection);
	}
	WorldTile2 GetPortTile() {
		return GetPortTile(gateTile(), _faceDirection, _buildingEnum);
	}
	

	bool ownedBy(int32 playerId) const { return _playerId == playerId; }

	int32 provinceId() const { return _simulation->GetProvinceIdClean(_centerTile); }

	int32 DistanceTo(WorldTile2 tile) {
		return WorldTile2::Distance(_centerTile, tile);
	}
	

	BldInfo buildingInfo() { return BuildingInfo[buildingEnumInt()]; }
	bool isEnum(CardEnum buildingEnum) const { return _buildingEnum == buildingEnum; }

	bool isConstructed(CardEnum buildingEnum) {
		return isEnum(buildingEnum) && isConstructed();
	}

	template<class T> 
	T& subclass(CardEnum buildingEnumIn) {
		check(buildingEnum() == buildingEnumIn);
		return *static_cast<T*>(this); 
	}

	template<class T>
	T& subclass() {
		return *static_cast<T*>(this);
	}

	bool isConstructed() { return _isConstructed; }
	bool didSetWalkable() { return _didSetWalkable; }
	bool noWorker() { return !IsHouse(_buildingEnum) && _maxOccupants > 0 && occupantCount() == 0; }

	bool shouldPrioritizeConstruction() {
		if (isConstructed()) {
			return false;
		}
		return (Time::Ticks() - _buildingLastWorkedTick > Time::TicksPerRound) || priority() == PriorityEnum::Priority;
	}
	

	// Not constructed and Not QuickBuild
	bool shouldDisplayConstructionUI() {
		return !isConstructed() && !_simulation->IsQuickBuild(buildingId());
	}

	int32 GetQuickBuildCost()
	{
		if (isConstructed()) {
			return 0;
		}

		return GetQuickBuildBaseCost(_buildingEnum, GetConstructionResourceCost(), [&](ResourceEnum resourceEnum) { return GetResourceCount(resourceEnum); });

		//int32 quickBuildCost_Resource = 0;
		//int32 quickBuildCost_Work = 0;
		//std::vector<int32> constructionCosts = GetConstructionResourceCost();
		//for (size_t i = 0; i < ConstructionResourceCount; i++) 
		//{
		//	int32 basePrice = GetResourceInfo(ConstructionResources[i]).basePrice;
		//	
		//	if (constructionCosts[i] > 0) {
		//		int32 resourceCount = GetResourceCount(ConstructionResources[i]);
		//		quickBuildCost_Resource += std::max(0, (constructionCosts[i] - resourceCount) * basePrice);
		//	}
		//	quickBuildCost_Work += constructionCosts[i] * basePrice;
		//}

		//// Road extra work equals to 1 wood
		//if (IsRoad(buildingEnum())) {
		//	quickBuildCost_Work += GetResourceInfo(ResourceEnum::Wood).basePrice;
		//}

		//// base 
		//quickBuildCost_Work = quickBuildCost_Work * (110 - constructionPercent()) / 100;

		//return (quickBuildCost_Resource * GameConstants::QuickBuildMultiplier_Resource) + (quickBuildCost_Work * GameConstants::QuickBuildMultiplier_Work);
	}

	template<typename Func>
	static int32 GetQuickBuildBaseCost(CardEnum buildingEnum, std::vector<int32> constructionCosts, Func getResourceCount)
	{
		int32 quickBuildCost_Resource = 0;
		int32 quickBuildCost_Work = 0;
		
		for (size_t i = 0; i < ConstructionResourceCount; i++)
		{
			int32 basePrice = GetResourceInfo(ConstructionResources[i]).basePrice;

			if (constructionCosts[i] > 0) {
				int32 resourceCount = getResourceCount(ConstructionResources[i]);
				quickBuildCost_Resource += std::max(0, (constructionCosts[i] - resourceCount) * basePrice);
			}
			quickBuildCost_Work += constructionCosts[i] * basePrice;
		}

		// Road extra work equals to 1 wood
		if (IsRoad(buildingEnum)) {
			quickBuildCost_Work += GetResourceInfo(ResourceEnum::Wood).basePrice;
		}

		// base 
		quickBuildCost_Work = quickBuildCost_Work * 110 / 100;

		return (quickBuildCost_Resource * GameConstants::QuickBuildMultiplier_Resource) + (quickBuildCost_Work * GameConstants::QuickBuildMultiplier_Work);
	}
	

	PriorityEnum priority() { return _priority; }

	std::vector<int32> GetConstructionResourceCost()
	{
		if (_buildingEnum == CardEnum::Farm) {
			std::vector<int32> constructionResources = { _simulation->GetFarmCost(tileCount()) };
			constructionResources.resize(ConstructionResourceCount, 0);
			return constructionResources;
		}
		if (_buildingEnum == CardEnum::StorageYard) {
			std::vector<int32> constructionResources = { _area.tileCount() / 4 * _simulation->StorageCostPerTile() };
			constructionResources.resize(ConstructionResourceCount, 0);
			return constructionResources;
		}
		return GetBuildingInfo(_buildingEnum).constructionResources;
		//return _simulation->GetConstructionResourceCost(_buildingEnum, _area);
	}

	bool hasNeededConstructionResource() {
		std::vector<int32> constructionCosts = GetConstructionResourceCost();
		for (size_t i = 0; i < _countof(ConstructionResources); i++) {
			if (constructionCosts[i] > 0 && GetResourceCount(ConstructionResources[i]) < constructionCosts[i]) return false;
		}
		return true;
	}
	// Debug
	bool IsValidConstructionResourceHolders() {
		std::vector<int32> constructionCosts = GetConstructionResourceCost();
		for (size_t i = 0; i < _countof(ConstructionResources); i++) {
			if (!(constructionCosts[i] == 0 || holderInfo(ConstructionResources[i]).isValid())) {
				return false;
			}
		}
		return true;
	}

	//WorldTile2 buildingSize() { return BuildingInfo[buildingEnumInt()].size; }
	WorldTile2 buildingSize() const {
		if (isEnum(CardEnum::Farm)) {
			return _area.size();
		}
		// Storage Yard??
		return GetBuildingInfoInt(buildingEnumInt()).size;
	}

	virtual int32 storageSlotCount() {
		return 4;
	}
	virtual int32 stackPerSide() {
		return 2;
	}
	

	WorldTile2 gateTileFromDirection(Direction faceDirection) const {
		return CalculateGateTile(faceDirection, _centerTile, buildingSize());
	}
	static WorldTile2 CalculateGateTile(Direction faceDirection, WorldTile2 centerTile, WorldTile2 size)
	{
		int32 centerToGate = (size.x - 1) / 2;
		return centerTile + WorldTile2::DirectionTile(faceDirection) * centerToGate;
	}
	virtual WorldTile2 gateTile() const { return gateTileFromDirection(_faceDirection); }


	BiomeEnum centerBiomeEnum() {
		return _simulation->GetBiomeEnum(centerTile());
	}
	

	// Go nearby the building (like to construct building)
	// TODO: need testing???
	WorldTile2 adjacentTileNearestTo(WorldTile2 start, int32 maxRegionDistance)
	{
		int32_t nearestDist = INT32_MAX;
		WorldTile2 nearestTile = WorldTile2::Invalid;

		auto checkNearest = [&](int16_t x, int16_t y) {
			WorldTile2 curTile(x, y);
			int32_t dist = WorldTile2::ManDistance(curTile, start);

			DEBUG_ISCONNECTED_VAR(adjacentTileNearestTo);
			
			if (dist < nearestDist && _simulation->IsConnected(curTile, start, maxRegionDistance)) {
				nearestDist = dist;
				nearestTile = curTile;
			}
		};

		// Top
		if (_area.maxX + 1 < GameMapConstants::TilesPerWorldX) {
			for (int16_t y = _area.minY; y <= _area.maxY; y++) {
				checkNearest(_area.maxX + 1, y);
			}
		}
		// Bottom
		if (_area.minX - 1 >= 0) {
			for (int16_t y = _area.minY; y <= _area.maxY; y++) {
				checkNearest(_area.minX - 1, y);
			}
		}
		// Right
		if (_area.maxY + 1 < GameMapConstants::TilesPerWorldY) {
			for (int16_t x = _area.minX; x <= _area.maxX; x++) {
				checkNearest(x, _area.maxY + 1);
			}
		}
		// Left
		if (_area.minY - 1 >= 0) {
			for (int16_t x = _area.minX; x <= _area.maxX; x++) {
				checkNearest(x, _area.minY - 1);
			}
		}

		return nearestTile;
	}

	FoundResourceHolderInfo GetHolderInfoFull(ResourceEnum resourceEnum, int32 amount) {
		return FoundResourceHolderInfo(holderInfo(resourceEnum), amount, gateTile(), _objectId);
	}
	FoundResourceHolderInfo GetHolderInfoFull(ResourceEnum resourceEnum, int32 amount, WorldTile2 originTile) {
		return FoundResourceHolderInfo(holderInfo(resourceEnum), amount, gateTile(), _objectId, DistanceTo(originTile));
	}

	void Tick1Sec()
	{
		OnTick1Sec();

		TestWorkDone();
	}
	virtual void OnTick1Sec() {
		//if (_playerId != -1) {
		//	PUN_LOG("Building Tick1Sec %d", buildingId());
		//}
		// ResetActions if the construction is taking too long
	}
	virtual void TickRound() {}

	void ResetDisplay() {
		_simulation->SetNeedDisplayUpdate(DisplayClusterEnum::Building, _centerTile.regionId());
		_simulation->SetNeedDisplayUpdate(DisplayClusterEnum::BuildingAnimation, _centerTile.regionId());
	}

	//! Upgrades
	const std::vector<BuildingUpgrade>& upgrades() { return _upgrades; }
	const BuildingUpgrade& GetUpgrade(int32 upgradeIndex) const { return _upgrades[upgradeIndex]; }
	
	bool UpgradeBuilding(int upgradeIndex, bool showPopups, ResourceEnum& needResourceEnumOut);
	
	virtual void OnUpgradeBuilding(int upgradeIndex) {}
	void OnUpgradeBuildingBase(int upgradeIndex) {
		OnUpgradeBuilding(upgradeIndex);
		if (upgradeIndex < _upgrades.size() && _upgrades[upgradeIndex].workerSlotBonus > 0) {
			SetJobBuilding(maxOccupants() + _upgrades[upgradeIndex].workerSlotBonus);
		}

		// Upgrade might change inputCount
		auto setTarget = [&](ResourceEnum resourceEnumIn, int32 targetIn) {
			if (resourceEnumIn != ResourceEnum::None) {
				resourceSystem().SetResourceTarget(holderInfo(resourceEnumIn), targetIn);
			}
		};
		if (input1() != ResourceEnum::None) setTarget(input1(), baseInputPerBatch(input1()) * 2);
		if (input2() != ResourceEnum::None) setTarget(input2(), baseInputPerBatch(input2()) * 2);
		if (product() != ResourceEnum::None) setTarget(product(), 0);

		// Display
		_simulation->AddFireOnceParticleInfo(ParticleEnum::OnUpgrade, _area);
	}

	BuildingUpgrade MakeUpgrade(FText name, FText description, ResourceEnum resourceEnum, int32 percentOfTotalPrice);
	BuildingUpgrade MakeUpgrade(FText name, FText description, int32 percentOfTotalPrice);

	BuildingUpgrade MakeProductionUpgrade(FText name, ResourceEnum resourceEnum, int32 efficiencyBonus);
	BuildingUpgrade MakeProductionUpgrade_Money(FText name, int32 efficiencyBonus);

	BuildingUpgrade MakeProductionUpgrade_WithHouseLvl(FText name, ResourceEnum resourceEnum, int32 houseLvl);

	BuildingUpgrade MakeWorkerSlotUpgrade(int32 percentOfTotalPrice, int32 workerSlotBonus = 1);
	BuildingUpgrade MakeEraUpgrade(int32 startEra);

	BuildingUpgrade MakeLevelUpgrade(FText name, FText description, ResourceEnum resourceEnum, int32 percentOfTotalPrice);

	BuildingUpgrade MakeComboUpgrade(FText name, ResourceEnum resourceEnum);


	//! Budget
	int32 lastBudgetLevel;
	int32 lastWorkTimeLevel;
	int32 budgetLevel() { return _budgetLevel; }
	int32 workTimeLevel() { return _workTimeLevel; }

	void SetBudgetLevel(int32 budgetLevel) {
		_budgetLevel = budgetLevel;
	}
	void SetWorkTimeLevel(int32 workTimeLevel) {
		_workTimeLevel = workTimeLevel;
	}

	//! level from 0
	int32 level() { return _level; }

	//! Occupancy
	int occupantCount() const { return _occupantIds.size(); }
	int allowedOccupants() { 
		//UE_LOG(LogTemp, Error, TEXT("allowedOccupants %d"), _allowedOccupants);
		return _allowedOccupants; 
	}
	int maxOccupants() { return _maxOccupants; }
	bool isOccupantFull() { return occupantCount() >= maxOccupants();  }

	bool isJobBuilding() {
		return !IsHouse(_buildingEnum) && maxOccupants() > 0;
	}

	virtual int32 GetWorkerCount() {
		return buildingInfo().workerCount;
	}

	void AddJobBuilding(int maxOccupant) {
		_maxOccupants = maxOccupant;
		_allowedOccupants = _maxOccupants;
		_simulation->PlayerAddJobBuilding(_townId, *this, true);
	}
	void SetJobBuilding(int maxOccupant) {
		_maxOccupants = maxOccupant;
		_allowedOccupants = _maxOccupants;
		_simulation->RefreshJobDelayed(_townId);
	}

	void ChangeAllowedOccupants(int32 allowedOccupants);
	bool CanAddOccupant() {
		return _occupantIds.size() < _allowedOccupants && !isDisabled();
	}

	bool CanAddInput()
	{
		// hasInput1 -> hasInput2
		if (hasInput1()) {
			if (!hasInput1Available()) {
				return false;
			}
		}
		if (hasInput2()) {
			if (!hasInput2Available()) {
				return false;
			}
		}
		return true;
	}

	virtual bool ShouldAddWorker_ConstructedNonPriority()
	{
		if (_simulation->IsOutputTargetReached(_townId, product())) {
			return false;
		}
		
		if (!_filledInputs) {
			// TODO: replace with CanAddInput()
			if (hasInput1() && hasInput2()) {
				if (!hasInput1Available()) {
					return false;
				}
				if (!hasInput2Available()) {
					return false;
				}
			}
			else if (hasInput1()) {
				if (!hasInput1Available()) {
					return false;
				}
			}
			else if (hasInput2()) {
				if (!hasInput2Available()) {
					return false;
				}
			}
		}
		return true;
	}

	// For playerOwned only, uses targetOccupants
	bool ShouldAddWorker(PriorityEnum priority)
	{
		//if (priority == PriorityEnum::NonPriority) 
		{
			if (isConstructed())
			{
				if (!ShouldAddWorker_ConstructedNonPriority()) {
					return false;
				}
			}
			else
			{
				std::vector<int32> constructionCosts = GetConstructionResourceCost();

				for (int i = constructionCosts.size(); i-- > 0;) {
					if (constructionCosts[i] > 0) {
						ResourceEnum resourceEnum = ConstructionResources[i];
						int32 neededResource = constructionCosts[i] - GetResourceCountWithPush(resourceEnum);
						if (neededResource > _simulation->resourceCountTown(_townId, resourceEnum)) {
							return false;
						}
					}
				}
			}
		}
		return targetOccupants < _allowedOccupants && !isDisabled();
	}
	

	void AddOccupant(int id) {
		_occupantIds.push_back(id);
		_simulation->SetNeedDisplayUpdate(DisplayClusterEnum::BuildingAnimation, _centerTile.regionId());

		if (isEnum(CardEnum::House)) {
			_simulation->RecalculateTaxDelayedTown(_townId);
		}
	}
	void RemoveOccupant(int id) {
		_occupantIds.erase(std::remove(_occupantIds.begin(), _occupantIds.end(), id), _occupantIds.end());
		_simulation->SetNeedDisplayUpdate(DisplayClusterEnum::BuildingAnimation, _centerTile.regionId());

		if (isEnum(CardEnum::House)) {
			_simulation->RecalculateTaxDelayedTown(_townId);
		}
	}
	std::vector<int>& occupants() { return _occupantIds; }
	void ClearOccupant() {
		_occupantIds.clear();
		_simulation->SetNeedDisplayUpdate(DisplayClusterEnum::BuildingAnimation, _centerTile.regionId());

		if (isEnum(CardEnum::House)) {
			_simulation->RecalculateTaxDelayedTown(_townId);
		}
	}

	virtual void SetBuildingPriority(PriorityEnum priority) {
		if (isFireDisabled()) {
			_priority = PriorityEnum::Disable;
			SetResourceActive(false);
			return;
		}
		
		_priority = priority;
		SetResourceActive(_priority != PriorityEnum::Disable);
	}
	bool isDisabled() { return priority() == PriorityEnum::Disable; }

	virtual void SetAreaWalkable();

	// SetWalkable using local tile from _centerTile
	void SetLocalWalkable(WorldTile2 localTile, bool isWalkable) {
		_simulation->SetWalkable(localTile + _centerTile, isWalkable);
	}
	void SetLocalWalkable_WithDirection(WorldTile2 localTile, bool isWalkable) {
		WorldTile2 rotatedTile = WorldTile2::RotateTileVector(localTile, _faceDirection);
		SetLocalWalkable(rotatedTile, isWalkable);
	}

	void TryInstantFinishConstruction()
	{
		InstantClearArea();
		SetAreaWalkable();
		if (!_isConstructed) {
			FinishConstruction();
		}
	}
	
	virtual void InstantClearArea();

	//! Helper
	class OverlaySystem& overlaySystem() { return _simulation->overlaySystem(); }
	ResourceSystem& resourceSystem() const {
		ResourceSystem& resourceSys = _simulation->resourceSystem(townId());
		resourceSys.CheckIntegrity_ResourceSys();
		return resourceSys;
	}
	GlobalResourceSystem& globalResourceSystem() {
		GlobalResourceSystem& globalResourceSys = _simulation->globalResourceSystem(_playerId);
		//globalResourceSy.CheckIntegrity_ResourceSys();
		return globalResourceSys;
	}

	TownManager& townManager() {
		return _simulation->townManager(_townId);
	}
	SubStatSystem& statSystem() {
		return _simulation->statSystem(_townId);
	}

	//! Resources

	std::vector<ResourceHolderInfo>& holderInfos() { return _holderInfos; }
	ResourceHolderInfo holderInfo(ResourceEnum resourceEnum) const {
		for (const auto& info : _holderInfos) {
			if (info.resourceEnum == resourceEnum) return info;
		}
		return ResourceHolderInfo(resourceEnum, InvalidResourceHolderId);
	}
	
	int32 resourceCount(ResourceEnum resourceEnum) const;
	int32 resourceCountSafe(ResourceEnum resourceEnum);
	
	int32 resourceCountWithPop(ResourceEnum resourceEnum);
	int32 tryResourceCount(ResourceEnum resourceEnum);

	const ResourceHolder& holder(ResourceEnum resourceEnum) {
		return resourceSystem().holder(holderInfo(resourceEnum));
	}

	FVector resourceDisplayShift() { return _resourceDisplayShift; }

	void AddResourceHolder(ResourceEnum resourceEnum, ResourceHolderType holderType, int target);
	void RemoveResourceHolder(ResourceEnum resourceEnum);

	void AddResource(ResourceEnum resourceEnum, int amount);
	void RemoveResource(ResourceEnum resourceEnum, int amount);

	virtual void OnPickupResource(int32 objectId) {
		SetBuildingResourceUIDirty();
	}
	virtual void OnDropoffResource(int32 objectId, ResourceHolderInfo holderInfo, int32 amount) {
		SetBuildingResourceUIDirty();
	}
	
	void FillInputs() {
		_filledInputs = true;
		_workDone100 = 1;
		_simulation->SetNeedDisplayUpdate(DisplayClusterEnum::BuildingAnimation, _centerTile.regionId());

		SetBuildingResourceUIDirty();
	}

	int32 GetResourceCount(ResourceHolderInfo holderInfo);
	int32 GetResourceCount(ResourceEnum resourceEnum) {
		ResourceHolderInfo info = holderInfo(resourceEnum);
		return info.isValid() ? GetResourceCount(info) : 0;
	}
	int32 GetResourceCountWithPush(ResourceHolderInfo holderInfo);
	int32 GetResourceCountWithPush(ResourceEnum resourceEnum) {
		return GetResourceCountWithPush(holderInfo(resourceEnum));
	}
	int32 GetResourceCountWithPop(ResourceHolderInfo holderInfo);
	int32 GetResourceCountWithPop(ResourceEnum resourceEnum) {
		return GetResourceCountWithPop(holderInfo(resourceEnum));
	}
	int32 GetResourceTarget(ResourceHolderInfo holderInfo);

	void SetResourceTarget(ResourceHolderInfo holderInfo, int32 target);

	//! Work
	virtual bool NeedWork();
	bool NeedConstruct();

	//void SatisfyInput1
	void ReserveWork(int objectId, int amount) { 
		//ACTION_LOG(_objectId, TEXT("ReserveWork: unitId:%d amount:%d"), objectId, amount);

		// There could be issues with leftover _workReservers.
		// TryConstruct will force someone to work regardless
		//for (int i = _workReservers.size(); i-- > 0;) {
		//	if (_workReservers[i] == objectId) {
		//		_workReservers.erase(_workReservers.begin() + i);
		//		_workReserved.erase(_workReserved.begin() + i);
		//	}
		//}
		PUN_CHECK(std::find(_workReservers.begin(), _workReservers.end(), objectId) == _workReservers.end());
		//bool reservedBefore = std::find(_workReservers.begin(), _workReservers.end(), objectId) == _workReservers.end();
		//if (reservedBefore) 	{
		//	CppUtils::TryRemove(_workReservers, objectId);
		//}
		
		_workReservers.push_back(objectId);
		_workReserved.push_back(amount);
	}

	void TestWorkDone();
	void DoWork(int unitId, int amount);
	
	int UnreserveWork(int objectId) {
		//ACTION_LOG(_objectId, TEXT("UnreserveWork: unitId:%d"), objectId);

		for (int i = _workReservers.size(); i-- > 0;) {
			if (_workReservers[i] == objectId) {
				int amount = _workReserved[i];
				_workReservers.erase(_workReservers.begin() + i);
				_workReserved.erase(_workReserved.begin() + i);
				return amount;
			}
		}
		PUN_NOENTRY_LOG(_simulation->unitdebugStr(objectId));
		return 0;
	}

	int32 workDone100() { return _workDone100; }
	const std::vector<int>& workReservers() { return _workReservers; }

	bool filledInputs() { return _filledInputs; }

	virtual bool shouldDisplayParticles() {
		if (SimSettings::IsOn("WorkAnimate")) {
			return true;
		}
		if (!isConstructed()) {
			return false;
		}
		if (hasInput1() || hasInput2()) {
			if (!filledInputs()) {
				return false;
			}
		}
		//if (IsProducer(_buildingEnum) && _workDone100 == 0) {
		//	return false;
		//}
		return occupantCount() > 0;
	}

	int32 buildTime_ManSec100()
	{
		BldInfo info = GetBuildingInfo(buildingEnum());
		int32 baseBuildTime = info.buildTime_ManSec100;
		if (isEnum(CardEnum::Farm) ||
			isEnum(CardEnum::StorageYard)) 
		{
			// Scale build time the same ratio as the construction resources
			return baseBuildTime * GetConstructionResourceCost()[0] / info.constructionResources[0];
		}
		if (isEnum(CardEnum::Fort) ||
			isEnum(CardEnum::ResourceOutpost))
		{
			return 10000;
		}
		if (isEnum(CardEnum::StatisticsBureau) ||
			isEnum(CardEnum::JobManagementBureau))
		{
			return 1000;
		}
		return baseBuildTime;
	}

	float constructionFraction()
	{
		float workNeeded100 = std::fmax(0.001f, buildTime_ManSec100());
		return _workDone100 / workNeeded100;
	}
	int32 constructionPercent() {
		int64 workNeeded100 = std::max(1, buildTime_ManSec100());
		return static_cast<int64>(_workDone100) * 100LL / workNeeded100;
	}

	void SetConstructionPercent(int32 percent) { // Note: Be careful of round off issues
		_workDone100 = buildTime_ManSec100() * percent / 100;
	}
	void ChangeConstructionPercent(int32 percent) {
		_workDone100 += buildTime_ManSec100() * percent / 100;
	}

	virtual float barFraction() { return workFraction(); }

	float workFraction() { return _workDone100 / static_cast<float>(workManSecPerBatch100()); }
	int32 workPercent() { return static_cast<int32_t>(workFraction() * 100); }

	// Work Batch Calculations
	int32 baseBatchCost() {
		BldInfo info = buildingInfo();
		int32 batchCost = 0;
		if (hasInput1()) {
			batchCost += baseInputCost(input1());
		}
		if (hasInput2()) {
			batchCost += baseInputCost(input2());
		}
		return batchCost;
	}
	int32 batchProfit() {
		ResourceEnum productEnum = product();
		if (productEnum == ResourceEnum::None) {
			return 0;
		}
		
		int32 baseProfitValue = 50; // Beer Brewery as base: 10 Beer(10) from 10 Wheat(5)
		
		baseProfitValue = buildingInfo().resourceInfo.ApplyUpgradeAndEraProfitMultipliers(baseProfitValue, buildingInfo().minEra(), GetEraUpgradeCount());

		// Adjust to worker count
		baseProfitValue = baseProfitValue * buildingInfo().workerCount / 2;


		// Special Case:
		if (productEnum == ResourceEnum::Gemstone ||
			productEnum == ResourceEnum::GoldOre)
		{
			baseProfitValue = baseProfitValue * 130 / 100;
		}
		if (productEnum == ResourceEnum::Oil) {
			baseProfitValue = baseProfitValue * 200 / 100;
		}
		
		
		check(baseProfitValue > 0);
		return baseProfitValue;
	}

	
	int32 workRevenuePerSec100_perMan_() {
		int32 result = buildingInfo().workRevenuePerSec100_perMan(GetEraUpgradeCount());
		if (IsElectricityUpgraded()) {
			result += result * ElectricityAmountUsage() / std::max(1, ElectricityAmountNeeded()) / 2; // having full (usage == needed) means we get 50% increase in productivity
		}
		return result;
	}

	// Work time required per batch...
	// Calculated based on inputPerBatch() and info.baseOutputPerBatch
	virtual int32 workManSecPerBatch100() {
		// Calculate workPerBatch...
		// workManSecPerBatch = batchProfit / WorkRevenuePerManSec
		// This is since we have to work for profit, so time needed to work depends on "profit amount" and "work rate"
		return batchProfit() * 100 * 100 / workRevenuePerSec100_perMan_();
	}

	virtual int32 baseUpkeep() {
		int32 baseUpkeep = BaseUpkeep(_buildingEnum);
		
		const BldInfo& info = GetBuildingInfo(_buildingEnum);
		baseUpkeep = info.resourceInfo.ApplyUpgradeAndEraProfitMultipliers(baseUpkeep, info.minEra(), GetEraUpgradeCount());
		
		return baseUpkeep;
	}

	static int32 BaseUpkeep(CardEnum buildingEnum)
	{
		const BldInfo& info = GetBuildingInfo(buildingEnum);
		
		int32 baseUpkeep;
		if (info.produce != ResourceEnum::None || IsSpecialProducer(buildingEnum)) {
			baseUpkeep = info.baseUpkeep;
		} else {
			baseUpkeep = GetCardUpkeepBase(buildingEnum);
		}
		
		return baseUpkeep;
	}

	int32 upkeep()
	{
		int32 upkeep = baseUpkeep();

		// Budget Upkeep
		switch (_budgetLevel)
		{
		case 5: upkeep = upkeep * 220 / 100; break; // More budget increase needed to get better efficiency
		case 4: upkeep = upkeep * 150 / 100; break;
		case 2: upkeep = upkeep * 50 / 100; break;
		case 1: upkeep = upkeep * 25 / 100; break; // The more we are trying to cut, the less we can cut to trade for less Effectiveness
		default:
			break;
		}

		// Frugality
		int32 frugalityCount = slotCardCount(CardEnum::FrugalityBook);
		for (int32 i = 0; i < frugalityCount; i++) {
			upkeep = upkeep * 30 / 100; // 30% maintenance (-70%)
		}
		
		// No worker, half upkeep
		if (maxOccupants() > 0 && occupantCount() == 0) {
			return upkeep / 2;
		}
		return upkeep;
	}
	

	// Check combo
	void CheckCombo();

	/*
	 * Adjacency bonuses
	 */
	void CheckAdjacency(bool shouldCheckNearbyToo = true, bool checkNearbyOnly = false)
	{
		if (_playerId == -1) return;

		if (!checkNearbyOnly)
		{
			int32 lastAdjacentEfficiencyCount = adjacentBonusCount();

			TileArea checkArea = _area;
			checkArea.minX -= 2; checkArea.maxX += 2;
			checkArea.minY -= 2; checkArea.maxY += 2;
			checkArea.EnforceWorldLimit();

			//if (_playerId != -1 && _buildingEnum != CardEnum::DirtRoad) {
				//PUN_LOG("checkArea %s", *ToFString(checkArea.ToString()));
				//_simulation->DrawArea(checkArea, FLinearColor::Red, -10);
			//}

			_adjacentIds.clear();
			checkArea.ExecuteOnArea_WorldTile2([&](WorldTile2 tile)
			{
				int32 buildingId = _simulation->buildingIdAtTile(tile);

				if (buildingId == -1 || buildingId == _objectId) {
					//_simulation->DrawLine(tile.worldAtom2(), FVector::ZeroVector, tile.worldAtom2(), FVector(0, 0, 20), FLinearColor::Black, 0.5f);
					return;
				}
				//_simulation->DrawLine(tile.worldAtom2(), FVector::ZeroVector, tile.worldAtom2(), FVector(0, 0, 20), FLinearColor::Green, 0.5f);

				CppUtils::TryAdd(_adjacentIds, buildingId);
			});

			// Combo buildings
			//  Override HasAdjacencyBonus to trigger this
			int32 newAdjacencyEffCount = adjacentBonusCount();
			if (newAdjacencyEffCount > lastAdjacentEfficiencyCount) {
				// TODO: may be back after combo is gone??
				//_simulation->uiInterface()->ShowFloatupInfo(FloatupEnum::ComboComplete, centerTile(), std::to_string(newAdjacencyEffCount) + " Adjacents");
			}
		}
		

		if (shouldCheckNearbyToo) {
			for (auto& adjacentId : _adjacentIds) {
				_simulation->building(adjacentId).CheckAdjacency(false);
			}
		}
	}
	int32 adjacentCount(CardEnum buildingEnum) {
		int count = 0;
		for (int32 adjacentId : _adjacentIds) {
			if (_simulation->building(adjacentId).isEnum(buildingEnum)) count++;
		}
		return count;
	}
	int32 adjacentCount() { return _adjacentIds.size(); }

	/*
	 * Radius
	 */
	template<typename BonusFunc>
	int32 GetRadiusBonus(CardEnum buildingEnum, int32 radius, BonusFunc getBonus) const
	{
		const std::vector<int32>& buildingIds = _simulation->buildingIds(_townId, buildingEnum);
		int32 bonus = 0;
		for (int32 buildingId : buildingIds) {
			Building& building = _simulation->building(buildingId);
			PUN_CHECK(building.isEnum(buildingEnum));
			
			if (building.isConstructed() && building.DistanceTo(_centerTile) <= radius) {
				bonus = getBonus(bonus, building);
			}
		}
		return bonus;
	}

	template<typename Func>
	void ExecuteInRadius(CardEnum buildingEnum, int32 radius, Func func)
	{
		const std::vector<int32>& buildingIds = _simulation->buildingIds(_townId, buildingEnum);
		for (int32 buildingId : buildingIds) {
			Building& building = _simulation->building(buildingId);
			PUN_CHECK(building.isEnum(buildingEnum));

			if (building.isConstructed() && building.DistanceTo(_centerTile) <= radius) {
				func(building);
			}
		}
	}
	

	/*
	 * Statistics
	 */
	int32 seasonalProduction() // sum up all types ... for resources with only one output
	{
		int32 resourceCount = 0;
		for (const std::vector<ResourcePair>& pairs : _seasonalProductionPairs) {
			for (const ResourcePair& pair : pairs) {
				resourceCount += pair.count;
			}
		}
		return resourceCount;
		//return std::accumulate(_seasonalProduction.begin(), _seasonalProduction.end(), 0);
	}
	std::vector<ResourcePair> seasonalProductionPairs()
	{
		std::vector<ResourcePair> results;
		for (const std::vector<ResourcePair>& pairs : _seasonalProductionPairs) {
			for (const ResourcePair& pair : pairs) {
				AlgorithmUtils::AddResourcePairToPairs(results, pair);
			}
		}
		return results;
	}

	int32 seasonalConsumption1() { return std::accumulate(_seasonalConsumption1.begin(), _seasonalConsumption1.end(), 0); }
	int32 seasonalConsumption2() { return std::accumulate(_seasonalConsumption2.begin(), _seasonalConsumption2.end(), 0); }

	void InitStatistics()
	{ 
		if (_buildingEnum == CardEnum::Farm) {
			_seasonalProductionPairs.resize(Time::MinutesPerYear);
		} else {
			_seasonalProductionPairs.resize(Time::MinutesPerSeason);
		}
		
		_seasonalConsumption1.resize(Time::MinutesPerSeason);
		_seasonalConsumption2.resize(Time::MinutesPerSeason);
	}

	void AddProductionStat(int32 amount) {
		AddProductionStat(ResourcePair(product(), amount));
	}
	void AddProductionStat(ResourcePair resource);
	void AddConsumption1Stat(ResourcePair resource);
	void AddConsumption2Stat(ResourcePair resource);
	void AddDepletionStat(ResourcePair resource);

	void AddConsumptionStats() {
		if (hasInput1()) AddConsumption1Stat(ResourcePair(input1(), inputPerBatch(input1())));
		if (hasInput2()) AddConsumption2Stat(ResourcePair(input2(), inputPerBatch(input2())));
	}

	void MinuteStatisticsUpdate()
	{
		_seasonalProductionPairs.insert(_seasonalProductionPairs.begin(), std::vector<ResourcePair>());
		_seasonalProductionPairs.pop_back();
		
		_seasonalConsumption1.insert(_seasonalConsumption1.begin(), 0);
		_seasonalConsumption1.pop_back();
		_seasonalConsumption2.insert(_seasonalConsumption2.begin(), 0);
		_seasonalConsumption2.pop_back();
	}

	// Debug
	std::string debugStr() { return buildingInfo().nameStd() + "[" + std::to_string(_objectId) + "]"; }


	virtual ResourceEnum product()
	{
		if (_workMode.product != ResourceEnum::None) {
			return _workMode.product;
		}
		return buildingInfo().produce;
	}
	virtual std::vector<ResourceEnum> products() {
		if (product() == ResourceEnum::None) {
			return {};
		}
		std::vector<ResourceEnum> outputs{ product() };
		if (isEnum(CardEnum::Beekeeper)) {
			outputs.push_back(ResourceEnum::Honey);
		}
		return outputs;
	}
	
	virtual ResourceEnum input1()
	{
		if (_workMode.input1 != ResourceEnum::None) {
			return _workMode.input1;
		}
		return buildingInfo().input1;
	}
	virtual ResourceEnum input2()
	{
		if (_workMode.input2 != ResourceEnum::None) {
			return _workMode.input2;
		}
		return buildingInfo().input2;
	}

	std::vector<ResourceEnum> inputs()
	{
		std::vector<ResourceEnum> inputs;
		if (hasInput1()) inputs.push_back(input1());
		if (hasInput2()) inputs.push_back(input2());
		return inputs;
	}

	bool hasInput1() { return input1() != ResourceEnum::None; }
	bool hasInput2() { return input2() != ResourceEnum::None; }

	bool hasInput1Available() {
		return _simulation->resourceCountTown(_townId, input1()) > 0 || resourceCount(input1()) >= inputPerBatch(input1());
	}
	bool hasInput2Available() {
		return _simulation->resourceCountTown(_townId, input2()) > 0 || resourceCount(input2()) >= inputPerBatch(input2());
	}

	bool needInput1()
	{
		if (!hasInput1()) {
			return false;
		}
		int32 resourceCountWithPop1 = resourceSystem().resourceCountWithPop(holderInfo(input1()));
		return resourceCountWithPop1 < inputPerBatch(input1());
	}
	bool needInput2()
	{
		if (!hasInput2()) {
			return false;
		}
		int32 resourceCountWithPop2 = resourceSystem().resourceCountWithPop(holderInfo(input2()));
		return resourceCountWithPop2 < inputPerBatch(input2());
	}
	

	virtual int32 baseOutputPerBatch()
	{
		//// Output Batch can be calculated from profit
		//int32 baseProfitValue = 30; // Beer Brewery as base: 10 Beer(10) from 10 Wheat(5)
		//baseProfitValue = buildingInfo().resourceInfo.ApplyUpgradeAndEraProfitMultipliers(baseProfitValue, buildingInfo().minEra(), GetEraUpgradeCount());

		int32 baseProfitValue = batchProfit();
		
		int32 outputValue = baseProfitValue;
		if (input1() != ResourceEnum::None) {
			outputValue += baseInputCost(input1());
		}
		if (input2() != ResourceEnum::None) {
			outputValue += baseInputCost(input2());
		}

		return outputValue / GetResourceInfo(product()).basePrice;
	}

	virtual int32 efficiencyBeforeBonus() { return 100; }
	int32 efficiency() {
		auto bonuses = GetBonuses();
		int32 total = efficiencyBeforeBonus();
		for (BonusPair bonus : bonuses) {
			total += bonus.value;
		}
		return total + adjacentEfficiency() + levelEfficiency();
	}

	// TODO: use outputPerBatch instead
	virtual int32 outputPerBatch() { return baseOutputPerBatch() * efficiency() / 100; }


	int32 oreLeft();

	int32 ElectricityAmountUsage() {
		return _electricityReceived;
	}
	int32 ElectricityAmountNeeded() {
		return occupantCount();
	}
	bool IsElectricityUpgraded() {
		// Last era is electricity
		return GetUpgradeEraLevel() >= 5;
	}
	void SetElectricityAmountUsage(int32 electricityReceivedIn) {
		_electricityReceived = electricityReceivedIn;
	}

	bool NotEnoughElectricity() {
		return IsElectricityUpgraded() && ElectricityAmountUsage() < ElectricityAmountNeeded() * 3 / 4;
	}

	

	
	/*
	 * Input
	 */
	int32 baseInputPerBatch(ResourceEnum resourceEnum)
	{
		int32 result = 10;
		if (input2() != ResourceEnum::None) { // Anything with 2 inputs, gets split equally
			result = 5;
		}
		
		if (resourceEnum == input1() && _workMode.customInputPerBatch1 != -1) {
			result = _workMode.customInputPerBatch1;
		}
		if (resourceEnum == input2() && _workMode.customInputPerBatch2 != -1) {
			result = _workMode.customInputPerBatch2;
		}

		// Special case:
		if (isEnum(CardEnum::PrintingPress)) {
			if (resourceEnum == ResourceEnum::Paper) result = 8;
			if (resourceEnum == ResourceEnum::Dye) result = 2;
		}
		else if (isEnum(CardEnum::Brickworks)) {
			if (resourceEnum == ResourceEnum::Clay) result = 7;
			if (resourceEnum == ResourceEnum::Coal) result = 3;
		}
		else if (isEnum(CardEnum::Chocolatier)) {
			if (resourceEnum == ResourceEnum::Cocoa) result = 8;
			if (resourceEnum == ResourceEnum::Milk) result = 2;
		}
		

		// Scholar Office doesn't scale input batch
		if (_buildingEnum == CardEnum::CardMaker) {
			return result;
		}

		// Adjust to worker count
		result = result * buildingInfo().workerCount / 2;

		return buildingInfo().resourceInfo.ApplyUpgradeAndEraProfitMultipliers(result, buildingInfo().minEra(), GetEraUpgradeCount());
	}

	int32 baseInputCost(ResourceEnum resourceEnum) {
		return GetResourceInfo(resourceEnum).basePrice * baseInputPerBatch(resourceEnum);
	}

	virtual int32 inputPerBatch(ResourceEnum resourceEnum)
	{
		if (resourceEnum == ResourceEnum::None) {
			return 0;
		}
		
		int32 result = baseInputPerBatch(resourceEnum);

		int32 sustainabilityCount = slotCardCount(CardEnum::SustainabilityBook);
		for (int32 i = 0; i < sustainabilityCount; i++) {
			result = result * 60 / 100; // -40%
		}
		
		result = std::max(0, result);
		return result;
	}

	virtual int32 displayVariationIndex();

	//bool NeedRoadConnection();

	bool IsUpgraded(int index) const {
		return _upgrades.size() > index ? _upgrades[index].isUpgraded : false;
	}

	// UpgradeIndex gets changed by Era Upgrade addition. This fixes it.
	bool IsUpgraded_InitialIndex(int index) {
		if (_upgrades.size() > 0 && _upgrades[0].isEraUpgrade()) {
			return IsUpgraded(index + 1);
		}
		return IsUpgraded(index);
	}

	void UpgradeInstantly(int32 index)
	{
		_upgrades[index].isUpgraded = true;

		ResetDisplay();

		OnUpgradeBuildingBase(index);
	}

	void AddUpgrades(std::vector<BuildingUpgrade> upgrades)
	{
		_upgrades = upgrades;
		if (!IsAutoEraUpgrade(_buildingEnum) && 
			!HasNoEraUpgrade(_buildingEnum) &&
			!IsPowerPlant(_buildingEnum)) 
		{
			_upgrades.insert(_upgrades.begin(), MakeEraUpgrade(buildingInfo().resourceInfo.era));
		}
	}

	int32 GetEraUpgradeCount() {
		if (_upgrades.size() > 0 && _upgrades[0].isEraUpgrade()) {
			check(_upgrades[0].upgradeLevel != -1);
			return _upgrades[0].upgradeLevel;
		}
		return 0;
	}

	int32 GetUpgradeEraLevel() {
		if (_upgrades.size() > 0 && _upgrades[0].isEraUpgrade()) {
			check(_upgrades[0].upgradeLevel != -1);
			return _upgrades[0].upgradeLevel + buildingInfo().minEra();
		}
		return -1;
	}
	bool IsEraUpgradable() {
		if (GetUpgradeEraLevel() > 4) {
			return true;
		}
		if (GetUpgradeEraLevel() == 4) {
			return _simulation->IsResearched(playerId(), TechEnum::Electricity);
		}
		return GetUpgradeEraLevel() < _simulation->GetEra(_playerId);
	}
	
	FText GetUpgradeDisplayDescription(int32 index);
	FText GetUpgradeDisplayName(int32 index);

	// Delivery
	int32 deliveryTargetIdAfterConstruction() {  // deliveryTargetId returns -1 if the building under construction
		if (isConstructed()) {
			return _deliveryTargetId;
		}
		return -1;
	}
	
	int32 deliveryTargetId() { return _deliveryTargetId; }
	
	const std::vector<int32>& deliverySourceIds() { return _deliverySourceIds; }
	
	void SetDeliveryTarget(int32 deliveryTargetId);
	void TryRemoveDeliveryTarget();
	void ClearDeliverySources()
	{
		std::vector<int32> deliverySourceIds = _deliverySourceIds;
		for (int32 sourceId : deliverySourceIds) {
			_simulation->buildingChecked(sourceId).TryRemoveDeliveryTarget();
		}
		check(_deliverySourceIds.size() == 0);
	}

	int32 foreignBuilder() const { return _foreignBuilder; }
	void SetForeignBuilder(int32 foreignBuilder) {
		_foreignBuilder = foreignBuilder;
		_isForeignBuildingApproved = false;

		// Foreign buildling should have no constructor
		FinishConstructionResourceAndWorkerReset();
		_maxOccupants = 0;
		_allowedOccupants = 0;
	}
	void ApproveForeignBuilder()
	{
		_isForeignBuildingApproved = true;
		InstantClearArea();
		_simulation->AddQuickBuild(_objectId);
	}
	bool isForeignBuildingApproved() { return _isForeignBuildingApproved; }
	

	// Cached Path
	const std::vector<std::vector<WorldTile2>>& cachedWaypoints() { return _cachedWaypoints; }
	void AddCachedWaypoints(const std::vector<WorldTile2>& waypointsIn)
	{
		check(waypointsIn.back() == gateTile());
		for (int32 i = 0; i < _cachedWaypoints.size(); i++) {
			check(_cachedWaypoints[i].front() != waypointsIn.front());
		}

		_cachedWaypoints.insert(_cachedWaypoints.begin(), waypointsIn);

		if (_cachedWaypoints.size() > PunSettings::Get("CachedWaypointsThreshold")) {
			_cachedWaypoints.pop_back();
		}
	}
	void UseCachedWaypoints(WorldTile2 end)
	{
		// Move the waypoints used to the top (so it doesn't get deleted)
		for (int32 i = 0; i < _cachedWaypoints.size(); i++) {
			if (_cachedWaypoints[i].front() == end) {
				std::vector<WorldTile2> cachedWaypoint = _cachedWaypoints[i];
				_cachedWaypoints.erase(_cachedWaypoints.begin() + i);
				_cachedWaypoints.insert(_cachedWaypoints.begin(), cachedWaypoint);
				return;
			}
		}
		UE_DEBUG_BREAK();
	}
	void ClearCachedWaypoints() {
		_cachedWaypoints.clear();
	}

	
	// Water Path
	const std::vector<int32>& cachedWaterDestinationPortIds() { return _cachedWaterDestinationPortIds; }
	const std::vector<WorldTile2>& cachedWaterRoute(int32 index) { return _cachedWaterRoutes[index]; }
	void AddWaterRoute(int32 portId, const std::vector<WorldTile2>& waterRoute) {
		_cachedWaterDestinationPortIds.push_back(portId);
		_cachedWaterRoutes.push_back(waterRoute);
	}

	//bool HasCombo() {
	//	return _simulation->buildingIds(_playerId, _buildingEnum).size() >= 3;
	//}
	virtual int32 adjacentEfficiency() {
		return adjacentBonusCount() * 5;
	}
	virtual int32 maxAdjacencyCount() { return 3; }
	int32 adjacentBonusCount() {
		// Adjacent Bonus Count max at 3
		return std::min(maxAdjacencyCount(), HasAdjacencyBonus() ? adjacentCount(_buildingEnum) : 0);
	}
	virtual bool HasAdjacencyBonus() {
		return IsProducerProcessor(_buildingEnum) && _simulation->IsResearched(_playerId, TechEnum::IndustrialAdjacency);
	}


	virtual int32_t levelEfficiency() {
		int32_t efficiency = 0;
		if (level() == 1) efficiency += 5;
		if (level() == 2) efficiency += 10;
		if (level() == 3) efficiency += 15;
		return efficiency;
	}

	virtual std::vector<BonusPair> GetBonuses();
	int32 GetTotalBonus()
	{
		int32 total = 0;
		std::vector<BonusPair> bonuses = GetBonuses();
		for (BonusPair bonus : bonuses) {
			total += bonus.value;
		}
		return total;
	}
	

	int32 GetAppealPercent();


	virtual int32 GetBaseJobHappiness() {
		return 70;
	}

	virtual int32 GetJobHappiness()
	{
		int32 jobHappiness = GetBaseJobHappiness();

		// Budget
		jobHappiness += (_budgetLevel - 3) * 10;
		
		// Work Time Level
		switch (_workTimeLevel)
		{
		case 5: jobHappiness -= 60; break; // More happiness decrease needed to get better efficiency
		case 4: jobHappiness -= 20; break;
		case 2: jobHappiness += 20; break;
		case 1: jobHappiness += 30; break; // The more we are trying to increase happiness, the less we can to trade for less Effectiveness
		default:
			break;
		}

		if (_simulation->TownhallCardCountTown(_townId, CardEnum::SocialWelfare)) {
			jobHappiness += 20;
		}

		if (_simulation->townBuildingFinishedCount(_townId, CardEnum::Cathedral) > 0) {
			jobHappiness += 10;
		}
		
		jobHappiness += 20 * slotCardCount(CardEnum::Passion);
		

		jobHappiness = std::max(0, jobHappiness);
		
		return jobHappiness;
	}

	/*
	 * Work Mode
	 */

	int32 workModeIntFromString(FString workModeName) {
		for (size_t i = 0; i < workModes.size(); i++) {
			if (workModes[i].name.ToString().Equals(workModeName)) {
				return i;
			}
		}
		UE_DEBUG_BREAK();
		return -1;
	}

	int32 buildingAge() { return Time::Ticks() - _buildingPlacedTick; }

	bool buildingTooLongToConstruct() { return buildingAge() > Time::TicksPerSeason * 3 / 2; }

	bool isUsable() { return isConstructed() && !isFireDisabled(); }

	int32 buildingPlacedTick() { return _buildingPlacedTick; }
	
	/*
	 * Fire
	 */
	bool isOnFire() { return _fireStartTick != -1; }
	bool isBurnedRuin() { return _isBurnedRuin; }
	bool isFireDisabled() { return isOnFire() || isBurnedRuin(); }
	
	int32_t fireStartTick() { return _fireStartTick; }
	void StartFire() { // Note: Don't call directly. Call the buildingSystem one
		if (!IsCritterBuildingEnum(_buildingEnum)) {
			_fireStartTick = Time::Ticks();
			SetBuildingPriority(PriorityEnum::Disable); // Disable building so no one will enter it.
			_simulation->SetNeedDisplayUpdate(DisplayClusterEnum::BuildingAnimation, _centerTile.regionId());

			if (IsHouse(buildingEnum()) && isConstructed())
			{
				ResetOccupants();
				_simulation->RemoveTenantFrom(buildingId());
			}
			else if (_maxOccupants > 0)  // skip constructed house
			{
				ResetWorkReservers();
				_simulation->RemoveJobsFrom(buildingId(), false);
			}
		}
	}
	void StopFire() {
		_fireStartTick = -1;
		SetBuildingPriority(PriorityEnum::NonPriority);
		_simulation->SetNeedDisplayUpdate(DisplayClusterEnum::BuildingAnimation, _centerTile.regionId());
	}
	void BecomeBurnedRuin() {
		Deinit(); // Deinit so that the stat is no longer counted...
		_isBurnedRuin = true;
		_fireStartTick = -1;
		_simulation->SetNeedDisplayUpdate(DisplayClusterEnum::Building, _centerTile.regionId());
		_simulation->SetNeedDisplayUpdate(DisplayClusterEnum::BuildingAnimation, _centerTile.regionId());
	}

	/*
	 * Trading
	 */
	static int32 baseFixedTradingFeePercent() { return 35; }
	int32 baseTradingFeePercent()
	{
		int32 tradeFeePercent = baseFixedTradingFeePercent();

		std::vector<BonusPair> bonuses = GetTradingFeeBonuses();
		for (const BonusPair& bonus : bonuses) {
			tradeFeePercent += bonus.value;
		}

		tradeFeePercent = std::max(0, tradeFeePercent);

		return tradeFeePercent;
	}
	virtual int32 maxTradeQuatity() { return 0; }
	
	std::vector<BonusPair> GetTradingFeeBonuses();

	int32 tradingFeePercent(int32 baseTradingFeePercent, ResourceEnum resourceEnum)
	{
		int32 tradeFeePercent = baseTradingFeePercent;
		if (_simulation->HasTownBonus(townId(), CardEnum::DesertTradeForALiving))
		{
			if (IsFoodEnum(resourceEnum) || resourceEnum == ResourceEnum::Wood || resourceEnum == ResourceEnum::Coal) {
				tradeFeePercent = 0;
			}
		}
		if (_simulation->HasTownBonus(townId(), CardEnum::DesertOreTrade) &&
			IsOreEnum(resourceEnum))
		{
			tradeFeePercent = 0;
		}

		
		return std::max(0, tradeFeePercent);
	}

	// 
	void TickConstruction(int32 ticksToFinish)
	{
		
		if (!isConstructed())
		{
			if (_workDone100 >= buildTime_ManSec100())
			{
				FinishConstruction();
				_simulation->soundInterface()->Spawn3DSound("CitizenAction", "ConstructionComplete", centerTile().worldAtom2());

				if (!PunSettings::TrailerMode()) {
					_simulation->uiInterface()->ShowFloatupInfo(FloatupEnum::BuildingComplete, _centerTile, FText());
				}
			}
			else
			{
				_workDone100 += buildTime_ManSec100() / ticksToFinish; // takes tickCount secs to finish the constrution

				//PUN_LOG("TickConstruction[%d] %s percent:%d", buildingId(), ToTChar(buildingInfo().name), constructionPercent());

				ResetDisplay(); // Slow??
				
				if (PunSettings::TrailerMode()) {
				} else {
					_simulation->soundInterface()->Spawn3DSound("CitizenAction", "WoodConstruction", centerTile().worldAtom2());
				}
			}
		}
	}

	/*
	 * Card slots
	 */
	bool IsCardSlotsFull() {
		return _cardSlots.size() >= maxCardSlots();
	}
	int32 slotCardCount(CardEnum cardEnum) {
		int32 count = 0;
		for (const CardStatus& cardStatus : _cardSlots) {
			if (cardStatus.cardEnum == cardEnum) {
				count++;
			}
		}
		return count;
	}
	const std::vector<CardStatus>& slotCards() {
		return _cardSlots;
	}
	CardStatus slotCard(int32 index) {
		if (0 <= index && index < _cardSlots.size()) {
			return _cardSlots[index];
		}
		return CardStatus();
	}
	virtual int32 maxCardSlots() {
		if (IsDecorativeBuilding(_buildingEnum) ||
			IsServiceBuilding(_buildingEnum) ||
			IsWorldWonder(_buildingEnum) ||
			IsPowerPlant(_buildingEnum) ||
			IsRoad(_buildingEnum) ||
			_buildingEnum == CardEnum::Farm ||
			_buildingEnum == CardEnum::StatisticsBureau ||
			_buildingEnum == CardEnum::JobManagementBureau ||
			_buildingEnum == CardEnum::TourismAgency ||
			_buildingEnum == CardEnum::SpyCenter ||
			_buildingEnum == CardEnum::ArchitectStudio ||
			_buildingEnum == CardEnum::DepartmentOfAgriculture ||
			_buildingEnum == CardEnum::EngineeringOffice) 
		{
			return 0;
		}
		return 2;
	}
	bool CanAddSlotCard()
	{
		if (isEnum(CardEnum::Townhall)) {
			return false;
		}
		return _cardSlots.size() < maxCardSlots();
	}
	void AddSlotCard(CardStatus card)
	{
		PUN_CHECK(!isEnum(CardEnum::Townhall));
		PUN_CHECK(IsBuildingSlotCard(card.cardEnum) || isEnum(CardEnum::Archives));

		_cardSlots.push_back(card);
	}
	CardEnum RemoveSlotCard(int32 unslotIndex)
	{
		PUN_CHECK(!isEnum(CardEnum::Townhall));
		if (unslotIndex >= _cardSlots.size()) {
			return CardEnum::None; // This could happen when clicking fast
		}

		CardEnum result = _cardSlots[unslotIndex].cardEnum;
		_cardSlots.erase(_cardSlots.begin() + unslotIndex);
		return result;
	}
	void ResetCardSlots()
	{
		PUN_CHECK(!isEnum(CardEnum::Townhall));
		_cardSlots.clear();
	}

	virtual int32 GetBuildingSelectorHeight() {
		return 30;
	}
	

	virtual void Serialize(FArchive& Ar)
	{
		SerializeVecObj(Ar, workModes);
		_workMode >> Ar;

		Ar << lastWorkedOn;

		Ar << workplaceInputNeeded;

		Ar << parent;
		SerializeVecValue(Ar, children);

		Ar << _objectId;

		SerializeVecValue(Ar, _occupantIds);
		Ar << _allowedOccupants;
		Ar << _maxOccupants;

		Ar << _townId;
		Ar << _playerId;
		Ar << _buildingEnum;
		Ar << _level;

		_centerTile >> Ar;
		Ar << _faceDirection;
		_area >> Ar;

		Ar << _fireStartTick;
		Ar << _isBurnedRuin;

		Ar << _buildingPlacedTick;
		Ar << _buildingLastWorkedTick;

		// Resources
		SerializeVecObj(Ar, _holderInfos);
		Ar << _resourceDisplayShift;

		Ar << _workDone100;
		SerializeVecValue(Ar, _workReservers);
		SerializeVecValue(Ar, _workReserved);
		
		Ar << _filledInputs;

		Ar << _electricityReceived;

		// Construction
		Ar << _didSetWalkable;
		Ar << _isConstructed;

		Ar << _priority;
		SerializeVecObj(Ar, _cardSlots);
		//_cardSlot1 >> Ar;

		// Statistics
		SerializeVecVecObj(Ar, _seasonalProductionPairs);

		SerializeVecValue(Ar, _seasonalConsumption1);
		SerializeVecValue(Ar, _seasonalConsumption2);

		// Adjacent
		SerializeVecValue(Ar, _adjacentIds);

		// Upgrade
		SerializeVecObj(Ar, _upgrades);

		Ar << _budgetLevel;
		Ar << _workTimeLevel;

		// Delivery
		Ar << _deliveryTargetId;
		SerializeVecValue(Ar, _deliverySourceIds);

		// Foreign
		Ar << _foreignBuilder;
		Ar << _isForeignBuildingApproved;

		// Cached Path
		SerializeVecVecObj(Ar, _cachedWaypoints);

		// Water Path
		SerializeVecValue(Ar, _cachedWaterDestinationPortIds);
		SerializeVecVecObj(Ar, _cachedWaterRoutes);

		Ar << hoverWarning;
	}

public:
	struct WorkMode
	{
		FText name;
		ResourceEnum input1 = ResourceEnum::None;
		ResourceEnum input2 = ResourceEnum::None;
		ResourceEnum product = ResourceEnum::None;
		FText description;

		int32 customInputPerBatch1 = -1;
		int32 customInputPerBatch2 = -1;

		bool isValid() { return !name.IsEmpty(); }

		static WorkMode Create(FText name, FText description, 
			ResourceEnum input1 = ResourceEnum::None, 
			ResourceEnum input2 = ResourceEnum::None,
			ResourceEnum product = ResourceEnum::None,
			int32 customInputPerBatch1 = -1, 
			int32 customInputPerBatch2 = -1)
		{
			WorkMode workMode;
			workMode.name = name;
			workMode.description = description;

			workMode.input1 = input1;
			workMode.input2 = input2;
			workMode.product = product;

			workMode.customInputPerBatch1 = customInputPerBatch1;
			workMode.customInputPerBatch2 = customInputPerBatch2;
			
			return workMode;
		}

		void operator>>(FArchive& Ar) {
			//SerializeStr(Ar, name);
			Ar << name;
			Ar << input1;
			Ar << input2;
			Ar << product;
			//SerializeStr(Ar, description);
			Ar << description;

			Ar << customInputPerBatch1;
			Ar << customInputPerBatch2;
		}

		bool operator==(const WorkMode& a) {
			return name.EqualTo(a.name);
		}
	};
	
	std::vector<WorkMode> workModes;
	const WorkMode& workMode() { return _workMode; }
	bool hasWorkModes() { return workModes.size() > 0; }

	void SetupWorkMode(std::vector<WorkMode> workModesIn) {
		workModes.clear();
		workModes = workModesIn;
		if (!_workMode.isValid()) {
			_workMode = workModes[0];
		}
	}
	void AddWorkMode(WorkMode workModesIn) {
		workModes.push_back(workModesIn);
	}
	
	
	void ChangeWorkMode(const WorkMode& workMode);
	virtual void OnChangeWorkMode(WorkMode newWorkMode) {}
	TArray<FText> workModeNames() {
		TArray<FText> result;
		for (const WorkMode& mode : workModes) {
			result.Add(mode.name);
		}
		return result;
	}
	

	int32 lastWorkedOn;

	//bool workplaceNeedInput;

	// TODO: might not be needed anymore???
	ResourceEnum workplaceInputNeeded;

	int32_t parent = -1;
	std::vector<int32_t> children;

	// Debug...
	PUN_DEBUG_EXPR(BldInfo buildingInfo_ = BldInfo(CardEnum::None, INVTEXT("none"), FString("none"), -1, INVTEXT("none")));


	// Public Non-serialized (Mostly UI)
	HoverWarning hoverWarning;

	int32 targetOccupants = 0; // Temporary variable used when Refreshing Jobs
	

	float lastHoverWarningCheckTime = 0.0f;

	void TryRefreshHoverWarning(float time) {
		if (time - lastHoverWarningCheckTime >= 1.0f) {
			lastHoverWarningCheckTime = time;
			RefreshHoverWarning();
		}
	}

	virtual bool RefreshHoverWarning()
	{
		if (_playerId == -1) {
			hoverWarning = HoverWarning::None;
			return true;
		}
		
		// Check if inaccessible
		// TODO: possible with second townhall and limit to how far you can conquer from townhall
		//if (_simulation->HasTownhall(_playerId)) {
		//	if (_simulation->IsConnected(gateTile(), houseGate, 2, true));
		//	hoverWarning = HoverWarning::Inaccessible;
		//}

		// Inaccessible Warning
		if (_simulation->HasTownhall(_playerId) &&
			maxOccupants() > 0 &&
			!_simulation->IsConnectedBuilding(buildingId()))
		{
			if (isConstructed()) {
				hoverWarning = HoverWarning::Inaccessible;
				return true;
			}

			// Construction site check all tiles
			bool isAccessible = area().ExecuteOnAreaWithExit_WorldTile2([&](WorldTile2 tile) {
				DEBUG_ISCONNECTED_VAR(RefreshHoverWarning);
				return _simulation->IsConnected(_simulation->GetTownhallGateFast(_townId), tile, GameConstants::MaxFloodDistance_HumanLogistics);
			});

			if (!isAccessible) {
				hoverWarning = HoverWarning::Inaccessible;
				return true;
			}
		}

		// HouseTooFar Warning
		if (_allowedOccupants > 0)
		{
			if (_simulation->buildingIds(_townId, CardEnum::House).size() > 0 &&
				!_simulation->HasBuildingWithinRadius(_centerTile, 55, _townId, CardEnum::House))
			{
				hoverWarning = HoverWarning::HouseTooFar;
				return true;
			}
		}

		
		// Construction will only get above checks
		if (!isConstructed())
		{
			hoverWarning = HoverWarning::None;
			return true;
		}
		// -----

		if (isEnum(CardEnum::TradingCompany) ||
			isEnum(CardEnum::ShippingDepot)) { // Trading Company handle its hoverWarning
			return false;
		}

		
		// Not enough input Warning
		if (hasInput1() && // hasInput1 -> hasInput2
			!_filledInputs &&
			!CanAddInput())
		{
			hoverWarning = HoverWarning::NotEnoughInput;
			return true;
		}
		
		// StorageTooFar Warning
		if (IsAgricultureBuilding(_buildingEnum))
		{
			if (!_simulation->HasBuildingWithinRadius(_centerTile, 40, _townId, CardEnum::StorageYard) &&
				!_simulation->HasBuildingWithinRadius(_centerTile, 40, _townId, CardEnum::Warehouse) &&
				!_simulation->HasBuildingWithinRadius(_centerTile, 40, _townId, CardEnum::Granary) &&
				!_simulation->HasBuildingWithinRadius(_centerTile, 40, _townId, CardEnum::Market))
			{
				hoverWarning = HoverWarning::StorageTooFar;
				return true;
			}
		}
		else
		{
			if (hasInput1() || hasInput2() || product() != ResourceEnum::None)
			{
				if (!_simulation->HasBuildingWithinRadius(_centerTile, 40, _townId, CardEnum::StorageYard) &&
					!_simulation->HasBuildingWithinRadius(_centerTile, 40, _townId, CardEnum::Warehouse) &&
					!_simulation->HasBuildingWithinRadius(_centerTile, 40, _townId, CardEnum::Market))
				{
					hoverWarning = HoverWarning::StorageTooFar;
					return true;
				}
			}
		}

		// Storage Full (Warn on production building)
		if (products().size() > 0 || isEnum(CardEnum::Farm))
		{
			if (_simulation->isStorageAllFull(_townId)) {
				hoverWarning = HoverWarning::StorageFull;
				return true;
			}
		}
		

		// OutputInventoryFull
		{
			std::vector<ResourceEnum> productEnums = products();
			for (ResourceEnum productEnum : productEnums)
			{
				int32 outputInventory = resourceSystem().resourceCountWithPop(holderInfo(productEnum));
				if (outputInventory >= GameConstants::WorkerEmptyBuildingInventoryAmount) 
				{
					hoverWarning = HoverWarning::OutputInventoryFull;
					return true;
				}
			}
		}

		if (NotEnoughElectricity())
		{
			hoverWarning = HoverWarning::NotEnoughElectricity;
			return true;
		}
		
		hoverWarning = HoverWarning::None;
		return false;
	}

	void FinishConstructionResourceAndWorkerReset()
	{
		// Remove Construction Resource Holders
		std::vector<int32> constructionCosts = GetConstructionResourceCost();
		for (int i = constructionCosts.size(); i-- > 0;) {
			RemoveResourceHolder(ConstructionResources[i]);
		}

		// Kick out all the constructors
		ResetWorkReservers();
		_simulation->RemoveJobsFrom(buildingId(), false);
		_simulation->PlayerRemoveJobBuilding(_townId, *this, false);
	}


	virtual bool isBuildingResourceUIDirty() {
		return _isBuildingResourceUIDirty;
	}
	void SetBuildingResourceUIDirty(bool dirty = true) {
		_isBuildingResourceUIDirty = dirty;
	}

	bool justFinishedConstructionJobUIDirty() { return _justFinishedConstructionJobUIDirty; }
	void SetJustFinishedConstructionJobUIDirty(bool value) {
		_justFinishedConstructionJobUIDirty = value;
	}

public:
	// Debug
#if DEBUG_BUILD
	std::vector<BuildingDebugAction> buildingActionHistory;
#endif

	FString GetBuildingActionHistory() const
	{
		std::stringstream ss;
		ss << "buildingActionHistory:";
#if DEBUG_BUILD
		int32 count = 0;
		for (int32 i = buildingActionHistory.size() - 1; i-- > 0;) {
			ss << "\n  " << BuildingActionEnums[static_cast<int32>(buildingActionHistory[i].actionEnum)]
				<< " " << buildingActionHistory[i].var1
				<< " " << buildingActionHistory[i].var2
				<< " " << buildingActionHistory[i].var3;
			if (count++ > 30) {
				break;
			}
		}
#endif
		return ToFString(ss.str());
	}

protected:
	void ResetWorkReservers();
	void ResetOccupants();

	void TrailerAddResource();

protected:
	IGameSimulationCore* _simulation = nullptr;
	
	int32 _objectId = -1;
	
	std::vector<int32> _occupantIds;
	int32 _allowedOccupants = 0;
	int32 _maxOccupants = 0;

	int32 _townId = -1;
	int32 _playerId = -1;
	
	CardEnum _buildingEnum = CardEnum::None;
	int32 _level = 0;

	WorldTile2 _centerTile;
	Direction _faceDirection = Direction::N;
	TileArea _area;

	int32 _fireStartTick = -1;
	bool _isBurnedRuin = false;
	
	int32 _buildingPlacedTick = -1;
	int32 _buildingLastWorkedTick = -1;

	// Resources
	std::vector<ResourceHolderInfo> _holderInfos;
	FVector _resourceDisplayShift = FVector(0, 0, 0);

	// TODO: Move out of building?
	int32 _workDone100 = 0;
	std::vector<int32> _workReservers;
	std::vector<int32> _workReserved;
	bool _filledInputs = false; // TODO: Probably should change filledInput name to _readyForWork ... or get rid of it.. set workDone100 to 1 instead...

	int32 _electricityReceived = 0;

	// Construction
	bool _didSetWalkable = false;
	bool _isConstructed = false;

	PriorityEnum _priority = PriorityEnum::NonPriority;
	std::vector<CardStatus> _cardSlots;

	// Statistics
	std::vector<std::vector<ResourcePair>> _seasonalProductionPairs;
	std::vector<int32> _seasonalConsumption1;
	std::vector<int32> _seasonalConsumption2;

	// Adjacent
	std::vector<int32> _adjacentIds;

	// Upgrade
	std::vector<BuildingUpgrade> _upgrades;

	int32 _budgetLevel = -1; // Increase Effectiveness, Increase Job Happiness, Decrease Money
	int32 _workTimeLevel = -1; // Increase Effectiveness, Decrease Job Happiness

	WorkMode _workMode;

	int32 _deliveryTargetId = -1;
	std::vector<int32> _deliverySourceIds;

	int32 _foreignBuilder = -1;
	bool _isForeignBuildingApproved = false;

	// Waypoint Cache
	std::vector<std::vector<WorldTile2>> _cachedWaypoints;
	

	// Water Path Cache
	// TODO: Cached PortId should be BuildingFullId!!! Need BuildingFullId???
	std::vector<int32> _cachedWaterDestinationPortIds;
	std::vector<std::vector<WorldTile2>> _cachedWaterRoutes;

	/*
	 * No Serialize
	 */
	int32 lastWorkSound = 0;

	bool _isBuildingResourceUIDirty = true;
	bool _justFinishedConstructionJobUIDirty = false;
};