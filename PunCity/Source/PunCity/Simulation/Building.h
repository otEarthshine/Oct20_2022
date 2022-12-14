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

class Building
{
public:
	virtual ~Building();

	void Init(IGameSimulationCore& simulation, int objectId, int32 playerId, uint8_t buildingEnum, 
						TileArea area, WorldTile2 centerTile, Direction faceDirection);
	void LoadInit(IGameSimulationCore* simulation) {
		_simulation = simulation;
	}

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
	int32 buildingId() const { return _objectId; }
	uint8 buildingEnumInt() const { return static_cast<uint8_t>(_buildingEnum); }
	CardEnum buildingEnum() const { return _buildingEnum; }
	CardEnum classEnum() const { return _buildingEnum; }
	WorldTile2 centerTile() const { return _centerTile; }
	Direction faceDirection() const { return _faceDirection; }
	TileArea area() const { return _area; }
	TileArea frontArea() const { return _area.GetFrontArea(_faceDirection);; }

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

	PriorityEnum priority() { return _priority; }

	std::vector<int32> GetConstructionResourceCost() {
		return _simulation->GetConstructionResourceCost(_buildingEnum, _area);
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
	WorldTile2 buildingSize() {
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
	

	WorldTile2 gateTileFromDirection(Direction faceDirection)
	{
		int32 centerToGate = (buildingSize().x - 1) / 2;
		return _centerTile + WorldTile2::DirectionTile(faceDirection) * centerToGate;
	}
	virtual WorldTile2 gateTile() { return gateTileFromDirection(_faceDirection); }

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
			
			if (dist < nearestDist && _simulation->IsConnected(curTile, start, maxRegionDistance, true)) {
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
	bool UpgradeBuilding(int upgradeIndex, bool showDisplay = true);
	
	virtual void OnUpgradeBuilding(int upgradeIndex) {}
	void OnUpgradeBuildingBase(int upgradeIndex) {
		OnUpgradeBuilding(upgradeIndex);
		if (upgradeIndex < _upgrades.size() && _upgrades[upgradeIndex].workerSlotBonus > 0) {
			SetJobBuilding(maxOccupants() + _upgrades[upgradeIndex].workerSlotBonus);
		}
	}

	BuildingUpgrade MakeUpgrade(FText name, FText description, ResourceEnum resourceEnum, int32 percentOfTotalPrice);
	BuildingUpgrade MakeUpgrade(FText name, FText description, int32 percentOfTotalPrice);

	BuildingUpgrade MakeProductionUpgrade(FText name, ResourceEnum resourceEnum, int32 percentOfTotalPrice, int32 efficiencyBonus);
	BuildingUpgrade MakeProductionUpgrade(FText name, int32 percentOfTotalPrice, int32 efficiencyBonus);

	BuildingUpgrade MakeWorkerSlotUpgrade(int32 percentOfTotalPrice, int32 workerSlotBonus = 1);

	BuildingUpgrade MakeComboUpgrade(FText name, ResourceEnum resourceEnum, int32 percentOfTotalPrice, int32 comboEfficiencyBonus);
	

	//! level from 0
	int32 level() { return _level; }

	//! Occupancy
	int occupantCount() { return _occupantIds.size(); }
	int allowedOccupants() { 
		//UE_LOG(LogTemp, Error, TEXT("allowedOccupants %d"), _allowedOccupants);
		return _allowedOccupants; 
	}
	int maxOccupants() { return _maxOccupants; }
	bool isOccupantFull() { return occupantCount() >= maxOccupants();  }

	void AddJobBuilding(int maxOccupant) {
		_maxOccupants = maxOccupant;
		_allowedOccupants = _maxOccupants;
		_simulation->PlayerAddJobBuilding(_playerId, *this, true);
	}
	void SetJobBuilding(int maxOccupant) {
		_maxOccupants = maxOccupant;
		_allowedOccupants = _maxOccupants;
		_simulation->RefreshJobDelayed(_playerId);
	}

	void ChangeAllowedOccupants(int32 allowedOccupants);
	bool CanAddOccupant() {
		return _occupantIds.size() < _allowedOccupants && !isDisabled();
	}

	bool CanAddInput()
	{
		if (hasInput1() && hasInput2()) {
			if (!hasInput1Available() && !hasInput2Available()) {
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
		return true;
	}

	virtual bool ShouldAddWorker_ConstructedNonPriority()
	{
		if (_simulation->IsOutputTargetReached(_playerId, product())) {
			return false;
		}
		
		if (!_filledInputs) {
			// TODO: replace with CanAddInput()
			if (hasInput1() && hasInput2()) {
				if (!hasInput1Available() && !hasInput2Available()) {
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
						if (neededResource > _simulation->resourceCount(_playerId, resourceEnum)) {
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
			_simulation->RecalculateTaxDelayed(_playerId);
		}
	}
	void RemoveOccupant(int id) {
		_occupantIds.erase(std::remove(_occupantIds.begin(), _occupantIds.end(), id), _occupantIds.end());
		_simulation->SetNeedDisplayUpdate(DisplayClusterEnum::BuildingAnimation, _centerTile.regionId());

		if (isEnum(CardEnum::House)) {
			_simulation->RecalculateTaxDelayed(_playerId);
		}
	}
	std::vector<int>& occupants() { return _occupantIds; }
	void ClearOccupant() {
		_occupantIds.clear();
		_simulation->SetNeedDisplayUpdate(DisplayClusterEnum::BuildingAnimation, _centerTile.regionId());

		if (isEnum(CardEnum::House)) {
			_simulation->RecalculateTaxDelayed(_playerId);
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
	
	virtual void InstantClearArea();

	//! Helper
	class OverlaySystem& overlaySystem() { return _simulation->overlaySystem(); }
	ResourceSystem& resourceSystem() {
		ResourceSystem& resourceSys = _simulation->resourceSystem(_playerId);
		resourceSys.CheckIntegrity_ResourceSys();
		return resourceSys;
	}

	//! Resources

	std::vector<ResourceHolderInfo>& holderInfos() { return _holderInfos; }
	ResourceHolderInfo holderInfo(ResourceEnum resourceEnum) {
		for (auto& info : _holderInfos) {
			if (info.resourceEnum == resourceEnum) return info;
		}
		return ResourceHolderInfo(resourceEnum, InvalidResourceHolderId);
	}
	
	int32 resourceCount(ResourceEnum resourceEnum);
	int32 tryResourceCount(ResourceEnum resourceEnum);

	const ResourceHolder& holder(ResourceEnum resourceEnum) {
		return resourceSystem().holder(holderInfo(resourceEnum));
	}

	FVector resourceDisplayShift() { return _resourceDisplayShift; }

	void AddResourceHolder(ResourceEnum resourceEnum, ResourceHolderType holderType, int target);
	void RemoveResourceHolder(ResourceEnum resourceEnum);

	void AddResource(ResourceEnum resourceEnum, int amount);
	void RemoveResource(ResourceEnum resourceEnum, int amount);

	virtual void OnPickupResource(int32 objectId) {}
	virtual void OnDropoffResource(int32 objectId, ResourceHolderInfo holderInfo, int32 amount) {}
	
	void FillInputs() {
		_filledInputs = true;
		_workDone100 = 1;
		_simulation->SetNeedDisplayUpdate(DisplayClusterEnum::BuildingAnimation, _centerTile.regionId());
	}

	int32 GetResourceCount(ResourceHolderInfo holderInfo);
	int32 GetResourceCount(ResourceEnum resourceEnum) {
		return GetResourceCount(holderInfo(resourceEnum));
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

		bool reservedBefore = std::find(_workReservers.begin(), _workReservers.end(), objectId) == _workReservers.end();
		PUN_CHECK2(reservedBefore, _simulation->unitdebugStr(objectId));
		_workReservers.push_back(objectId);
		_workReserved.push_back(amount);
	}
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

	bool shouldDisplayParticles() {
		if (SimSettings::IsOn("WorkAnimate")) {
			return true;
		}
		if (IsProducer(_buildingEnum) && _workDone100 == 0) {
			return false;
		}
		return occupantCount() > 0 || isEnum(CardEnum::Townhall);
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
			isEnum(CardEnum::Colony))
		{
			return 10000;
		}
		return baseBuildTime;
	}

	float constructionFraction() { return _workDone100 / static_cast<float>(buildTime_ManSec100()); }
	int32 constructionPercent() { return _workDone100 * 100 / buildTime_ManSec100(); }

	void SetConstructionPercent(int32 percent) {
		_workDone100 = buildTime_ManSec100() * percent / 100;
	}

	virtual float barFraction() { return workFraction(); }

	float workFraction() { return _workDone100 / static_cast<float>(workManSecPerBatch100()); }
	int32 workPercent() { return static_cast<int32_t>(workFraction() * 100); }

	// Work Batch Calculations
	int32 batchCost() {
		BldInfo info = buildingInfo();
		int32 batchCost = 0;
		if (hasInput1()) {
			batchCost += inputPerBatch() * GetResourceInfo(input1()).basePrice;
		}
		if (hasInput2()) {
			batchCost += inputPerBatch() * GetResourceInfo(input2()).basePrice;
		}
		return batchCost;
	}
	int32 batchProfit() {
		ResourceEnum productEnum = product();
		if (productEnum == ResourceEnum::None) {
			return 0;
		}
		int32 batchRevenue = buildingInfo().productionBatch * GetResourceInfo(productEnum).basePrice;
		int32 cost = batchCost();
		int32 batchProfit = batchRevenue - cost;
		check(batchProfit > 0);
		return batchProfit;
	}

	// Work time required per batch...
	// Calculated based on inputPerBatch() and info.productionBatch
	virtual int32 workManSecPerBatch100() {
		// Calculate workPerBatch...
		// workManSecPerBatch = batchProfit / WorkRevenuePerManSec
		// This is since we have to work for profit, so time needed to work depends on "profit amount" and "work rate"
		return batchProfit() * 100 * 100 / buildingInfo().workRevenuePerSec100_perMan; 
	}

	virtual int32 baseUpkeep()
	{
		int32 baseUpkeep;
		if (product() != ResourceEnum::None || IsSpecialProducer(_buildingEnum)) {
			baseUpkeep = buildingInfo().baseUpkeep;
		}
		else {
			baseUpkeep = GetCardUpkeepBase(_buildingEnum);
		}
		return baseUpkeep;
	}

	int32 upkeep()
	{
		int32 upkeep = baseUpkeep();

		// Frugality
		upkeep = std::max(0, upkeep - upkeep * slotCardCount(CardEnum::FrugalityBook) * 50 / 100);
		
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
	int32 GetRadiusBonus(CardEnum buildingEnum, int32 radius, BonusFunc getBonus)
	{
		const std::vector<int32>& buildingIds = _simulation->buildingIds(_playerId, buildingEnum);
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
		const std::vector<int32>& buildingIds = _simulation->buildingIds(_playerId, buildingEnum);
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
		int32 inputCount = inputPerBatch();
		if (hasInput1()) AddConsumption1Stat(ResourcePair(input1(), inputCount));
		if (hasInput2()) AddConsumption2Stat(ResourcePair(input2(), inputCount));
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
	std::string debugStr() { return buildingInfo().name + "[" + std::to_string(_objectId) + "]"; }


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
		return _simulation->resourceCount(_playerId, input1()) > 0 || resourceCount(input1()) >= inputPerBatch();
	}
	bool hasInput2Available() {
		return _simulation->resourceCount(_playerId, input2()) > 0 || resourceCount(input2()) >= inputPerBatch();
	}

	bool needInput1()
	{
		if (!hasInput1()) {
			return false;
		}
		int32 resourceCountWithPop1 = resourceSystem().resourceCountWithPop(holderInfo(input1()));
		return resourceCountWithPop1 < inputPerBatch();
	}
	bool needInput2()
	{
		if (!hasInput2()) {
			return false;
		}
		int32 resourceCountWithPop2 = resourceSystem().resourceCountWithPop(holderInfo(input2()));
		return resourceCountWithPop2 < inputPerBatch();
	}
	

	virtual int32 baseProductPerBatch() {  return buildingInfo().productionBatch; }

	virtual int32 efficiencyBeforeBonus() { return 100; }
	int32 efficiency() {
		auto bonuses = GetBonuses();
		int32 total = efficiencyBeforeBonus();
		for (BonusPair bonus : bonuses) {
			total += bonus.value;
		}
		return total + adjacentEfficiency() + levelEfficiency();
	}

	// TODO: use productPerBatch instead
	virtual int32 productPerBatch() { return baseProductPerBatch() * efficiency() / 100; }


	int32 oreLeft();

	/*
	 * Input
	 */
	virtual int32 baseInputPerBatch()
	{
		if (_workMode.inputPerBatch > 0) {
			return _workMode.inputPerBatch;
		}
		return 10;
	}

	int32 inputPerBatch()
	{
		int32 result = baseInputPerBatch();
		result = std::max(0, result - result * slotCardCount(CardEnum::SustainabilityBook) * 40 / 100);
		return result;
	}

	virtual int32 displayVariationIndex() {
		return 0;
	}

	//bool NeedRoadConnection();

	bool IsUpgraded(int index) {
		return _upgrades.size() > index ? _upgrades[index].isUpgraded : false;
	}

	void UpgradeInstantly(int32 index)
	{
		_upgrades[index].isUpgraded = true;

		ResetDisplay();

		OnUpgradeBuildingBase(index);
	}


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
	

	//bool HasCombo() {
	//	return _simulation->buildingIds(_playerId, _buildingEnum).size() >= 3;
	//}
	int32 adjacentEfficiency() {
		return adjacentBonusCount() * 5;
	}
	int32 adjacentBonusCount() {
		// Adjacent Bonus Count max at 3
		return std::min(3, HasAdjacencyBonus() ? adjacentCount(_buildingEnum) : 0);
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

	bool isUsable() { return isConstructed() && !isFireDisabled(); }


	
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
	static int32 baseTradingFeePercent() { return 25; }
	int32 tradingFeePercent()
	{
		int32 tradeFeePercent = baseTradingFeePercent();

		std::vector<BonusPair> bonuses = GetTradingFeeBonuses();
		for (const BonusPair& bonus : bonuses) {
			tradeFeePercent += bonus.value;
		}

		tradeFeePercent = std::max(0, tradeFeePercent);

		return tradeFeePercent;
	}
	virtual int32 maxTradeQuatity() { return 0; }
	
	std::vector<BonusPair> GetTradingFeeBonuses();

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
					_simulation->uiInterface()->ShowFloatupInfo(FloatupEnum::BuildingComplete, _centerTile, "");
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
	CardEnum slotCard() {
		return _cardSlot1.cardEnum;
	}
	int32 slotCardCount(CardEnum cardEnum) {
		return _cardSlot1.cardEnum == cardEnum ? 1 : 0;
	}
	std::vector<CardStatus> slotCards() {
		if (_cardSlot1.cardEnum != CardEnum::None) {
			return { _cardSlot1 };
		}
		return {};
	}
	virtual int32 maxCardSlots() {
		if (IsDecorativeBuilding(_buildingEnum)) {
			return 0;
		}
		return 1;
	}
	bool CanAddSlotCard()
	{
		if (isEnum(CardEnum::Townhall)) {
			return false;
		}
		if (maxCardSlots() == 0) {
			return false;
		}
		return _cardSlot1.cardEnum == CardEnum::None;
	}
	void AddSlotCard(CardStatus card)
	{
		PUN_CHECK(!isEnum(CardEnum::Townhall));
		PUN_CHECK(IsBuildingSlotCard(card.cardEnum));
		PUN_CHECK(_cardSlot1.cardEnum == CardEnum::None);
		_cardSlot1 = card;
	}
	CardEnum RemoveSlotCard()
	{
		PUN_CHECK(!isEnum(CardEnum::Townhall));
		PUN_CHECK(_cardSlot1.cardEnum != CardEnum::None);
		CardEnum result = _cardSlot1.cardEnum;
		_cardSlot1 = CardStatus();
		return result;
	}
	void ClearSlotCard()
	{
		PUN_CHECK(!isEnum(CardEnum::Townhall));
		_cardSlot1 = CardStatus();
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

		Ar << _playerId;
		Ar << _buildingEnum;
		Ar << _level;

		_centerTile >> Ar;
		Ar << _faceDirection;
		_area >> Ar;

		Ar << _fireStartTick;
		Ar << _isBurnedRuin;

		Ar << _buildingPlacedTick;

		// Resources
		SerializeVecObj(Ar, _holderInfos);
		Ar << _resourceDisplayShift;

		Ar << _workDone100;
		SerializeVecValue(Ar, _workReservers);
		SerializeVecValue(Ar, _workReserved);
		
		Ar << _filledInputs;

		// Construction
		Ar << _didSetWalkable;
		Ar << _isConstructed;

		Ar << _priority;
		_cardSlot1 >> Ar;

		// Statistics
		SerializeVecVecObj(Ar, _seasonalProductionPairs);

		SerializeVecValue(Ar, _seasonalConsumption1);
		SerializeVecValue(Ar, _seasonalConsumption2);

		// Adjacent
		SerializeVecValue(Ar, _adjacentIds);

		// Upgrade
		SerializeVecObj(Ar, _upgrades);

		// Delivery
		Ar << _deliveryTargetId;
		SerializeVecValue(Ar, _deliverySourceIds);

		Ar << hoverWarning;
	}

public:
	struct WorkMode
	{
		FText name;
		ResourceEnum input1 = ResourceEnum::None;
		ResourceEnum input2 = ResourceEnum::None;
		int32 inputPerBatch = 0;
		ResourceEnum product = ResourceEnum::None;
		FText description;

		static WorkMode Create(FText name, FText description) {
			WorkMode workMode;
			workMode.name = name;
			workMode.description = description;
			return workMode;
		}

		void operator>>(FArchive& Ar) {
			//SerializeStr(Ar, name);
			Ar << name;
			Ar << input1;
			Ar << input2;
			Ar << inputPerBatch;
			Ar << product;
			//SerializeStr(Ar, description);
			Ar << description;
		}
	};
	
	std::vector<WorkMode> workModes;
	const WorkMode& workMode() { return _workMode; }
	bool hasWorkModes() { return workModes.size() > 0; }

	void SetupWorkMode(std::vector<WorkMode> workModesIn) {
		workModes = workModesIn;
		_workMode = workModes[0];
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
	PUN_DEBUG_EXPR(BldInfo buildingInfo_ = BldInfo(CardEnum::None, "none", -1, "none"));


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
			!_simulation->IsConnectedBuilding(buildingId(), _playerId) &&
			maxOccupants() > 0)
		{
			if (isConstructed()) {
				hoverWarning = HoverWarning::Inaccessible;
				return true;
			}
			
			// Construction site check all tiles
			bool isAccessible = area().ExecuteOnAreaWithExit_WorldTile2([&](WorldTile2 tile) {
				DEBUG_ISCONNECTED_VAR(RefreshHoverWarning);
				return _simulation->IsConnected(_simulation->townhallGateTile(_playerId), tile, 7, true);
			});
			//	if (_simulation->IsConnected(_simulation->townhallGateTile(_playerId), gateTile(), 7, true)) {
			//		isAccessible = true;
			//		break;
			//	}
			//}

			if (!isAccessible) {
				hoverWarning = HoverWarning::Inaccessible;
				return true;
			}
		}

		// HouseTooFar Warning
		if (_allowedOccupants > 0)
		{
			if (!_simulation->HasBuildingWithinRadius(_centerTile, 55, _playerId, CardEnum::House))
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
		if (hasInput1() &&
			!_filledInputs &&
			!CanAddInput())
		{
			hoverWarning = HoverWarning::NotEnoughInput;
			return true;
		}
		
		// StorageTooFar Warning
		if (hasInput1() || hasInput2() || product() != ResourceEnum::None ||
			IsAgricultureBuilding(_buildingEnum))
		{
			if (!_simulation->HasBuildingWithinRadius(_centerTile, 40, _playerId, CardEnum::StorageYard) &&
				!_simulation->HasBuildingWithinRadius(_centerTile, 40, _playerId, CardEnum::Warehouse)) 
			{
				hoverWarning = HoverWarning::StorageTooFar;
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
		
		hoverWarning = HoverWarning::None;
		return false;
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

	int32 _playerId = -1;
	CardEnum _buildingEnum = CardEnum::None;
	int32 _level = 0;

	WorldTile2 _centerTile;
	Direction _faceDirection = Direction::N;
	TileArea _area;

	int32 _fireStartTick = -1;
	bool _isBurnedRuin = false;
	
	int32 _buildingPlacedTick = -1;

	// Resources
	std::vector<ResourceHolderInfo> _holderInfos;
	FVector _resourceDisplayShift = FVector(0, 0, 0);

	// TODO: Move out of building?
	int32 _workDone100 = 0;
	std::vector<int32> _workReservers;
	std::vector<int32> _workReserved;
	bool _filledInputs = false; // TODO: Probably should change filledInput name to _readyForWork ... or get rid of it.. set workDone100 to 1 instead...

	// Construction
	bool _didSetWalkable = false;
	bool _isConstructed = false;

	PriorityEnum _priority = PriorityEnum::NonPriority;
	CardStatus _cardSlot1;

	// Statistics
	std::vector<std::vector<ResourcePair>> _seasonalProductionPairs;
	std::vector<int32> _seasonalConsumption1;
	std::vector<int32> _seasonalConsumption2;

	// Adjacent
	std::vector<int32> _adjacentIds;

	// Upgrade
	std::vector<BuildingUpgrade> _upgrades;

	WorkMode _workMode;

	int32 _deliveryTargetId = -1;
	std::vector<int32> _deliverySourceIds;

	/*
	 * No Serialize
	 */
	int32 lastWorkSound = 0;
};