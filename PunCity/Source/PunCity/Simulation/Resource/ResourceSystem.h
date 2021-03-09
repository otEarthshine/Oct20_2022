// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "../GameSimulationInfo.h"
#include "../IGameSimulationCore.h"
#include "../GameEventSystem.h"
#include "../ResourceDropSystem.h"

#include <algorithm>
#include <iterator>
#include "PunCity/CppUtils.h"
#include "PunCity/GameConstants.h"

DECLARE_CYCLE_STAT(TEXT("PUN: Unit.CalcHuman.MoveResource.FindHolder [4.2.4.1]"), STAT_PunUnit_CalcHuman_MoveResource_FindHolder, STATGROUP_Game);

DECLARE_CYCLE_STAT(TEXT("PUN: Unit.CalcHuman.MoveResource.FindHolder.Filter [4.2.4.1.1]"), STAT_PunUnit_CalcHuman_MoveResource_FindHolder_Filter, STATGROUP_Game);
DECLARE_CYCLE_STAT(TEXT("PUN: Unit.CalcHuman.MoveResource.FindHolder.Rest [4.2.4.1.2]"), STAT_PunUnit_CalcHuman_MoveResource_FindHolder_Rest, STATGROUP_Game);

DECLARE_CYCLE_STAT(TEXT("PUN: ResourceCountTotal"), STAT_PunResourceCountTotal, STATGROUP_Game);

// For single resource type
struct ResourceHolder
{
	ResourceHolderInfo info;
	ResourceHolderType type;
	int32 objectId = -1; // objectId is building's. for drop, this is -1
	WorldTile2 tile;

	std::vector<int32> reservePushUnitIds;
	std::vector<int32> reservePushs;
	std::vector<int32> reservePopUnitIds;
	std::vector<int32> reservePops;

	bool isDrop() const { return objectId == -1; }

	int32 current() const { return _current; }
	int32 reservedPush() const { return _reservedPush; }
	int32 reservedPop() const { return _reservedPop; }
	int32 target() const { return _target; }

	void resourcedebugStr(std::stringstream& ss, WorldTile2 townCenterTile, IGameSimulationCore* sim) const {
		ss << "[" << GetResourceInfoSafe(info.resourceEnum).nameStd() << " " << GetResourceHolderTypeName(type)
			<< " - " + std::to_string(_current)
			<< " push:" + std::to_string(_reservedPush)
			<< " pop:" + std::to_string(_reservedPop)
			<< " target:" + std::to_string(_target)
			<< " connected:" + std::to_string(sim->IsConnected(townCenterTile, tile, 3, true))
			<< " owner:" + GetBuildingInfo(sim->buildingEnum(objectId)).nameStd()
			<< "]\n";
	}

	void SetTarget(int32 newTarget) {
		_target = newTarget;
	}
	void SetType(ResourceHolderType typeIn) {
		type = typeIn;
	}

	int balance() { return _current - _reservedPop + _reservedPush; }
	int pickupBalance() { return _current - _reservedPop; } // Total that can be picked up without going negative
	int dropoffBalance() { return _current + _reservedPush; } // TODO: Total that can be dropped off without exceeding capacity

	ResourceHolder()
		: info(ResourceHolderInfo::Invalid()), type(ResourceHolderType::Dead), objectId(-1), tile(WorldTile2::Invalid), _target(-1) {}

	ResourceHolder(ResourceHolderInfo info, ResourceHolderType type, int objectId, WorldTile2 tile, int target)
		: info(info), type(type), objectId(objectId), tile(tile), _target(target) {}

	void ResetPopReservers(IGameSimulationCore& simulation) {
		// ResetUnitActions should call remove reservations to remove elements from these vectors
		for (int j = reservePopUnitIds.size(); j-- > 0;) {
			simulation.ResetUnitActions(reservePopUnitIds[j]);
		}

		PUN_CHECK(_reservedPop == 0 && reservePopUnitIds.empty());
	}

	void ResetPushReservers(IGameSimulationCore& simulation) {
		// ResetUnitActions should call remove reservations to remove elements from these vectors
		for (int j = reservePushUnitIds.size(); j-- > 0;) {
			simulation.ResetUnitActions(reservePushUnitIds[j]);
		}

		PUN_CHECK(_reservedPush == 0 && reservePushUnitIds.empty());
	}
	 
	//! Add Reservation
	void AddPushReservation(int32 amount, int32 unitId) {
		_reservedPush += amount;
		reservePushUnitIds.push_back(unitId);
		reservePushs.push_back(amount);
	}

	void AddPopReservation(int32 amount, int32 unitId)
	{
		_reservedPop += amount;
		reservePopUnitIds.push_back(unitId);
		reservePops.push_back(amount);

		PUN_ENSURE(_current >= _reservedPop, _current = _reservedPop;);
		// Check that when reservePops.empty(), _reservedPop == 0
	}

	//! Remove Reservation
	int32 RemovePushReservation(int32 unitId) 
	{
		for (size_t i = reservePushUnitIds.size(); i-- > 0;) {
			if (reservePushUnitIds[i] == unitId) {
				int32_t amount = reservePushs[i];
				_reservedPush -= amount;

				reservePushUnitIds.erase(reservePushUnitIds.begin() + i);
				reservePushs.erase(reservePushs.begin() + i);
				
				PUN_ENSURE(_reservedPush >= 0, _reservedPush = 0);
				return amount;
			}
		}
		return -1;
	}

	int32 RemovePopReservation(int32 unitId)
	{
		for (size_t i = reservePopUnitIds.size(); i-- > 0;) {
			if (reservePopUnitIds[i] == unitId) {
				int amount = reservePops[i];
				_reservedPop -= amount;

				reservePopUnitIds.erase(reservePopUnitIds.begin() + i);
				reservePops.erase(reservePops.begin() + i);
				
				PUN_ENSURE(_reservedPop >= 0, _reservedPop = 0);
				return amount;
			}
		}
		return -1;
	}

	// Add Resource
	void AddResource(int32_t amount) {
		_current += amount;
	}
	void RemoveResource(int32_t amount) {
		_current -= amount;
		
		PUN_ENSURE(_current >= _reservedPop, _current = _reservedPop);
	}

	void operator>>(FArchive& Ar)
	{
		info >> Ar;
		Ar << type;
		Ar << objectId; // objectId is building's. for drop, this is -1
		tile >> Ar;

		SerializeVecValue(Ar, reservePushUnitIds);
		SerializeVecValue(Ar, reservePushs);
		SerializeVecValue(Ar, reservePopUnitIds);
		SerializeVecValue(Ar, reservePops);

		Ar << _current;
		Ar << _target;
		Ar << _reservedPush;
		Ar << _reservedPop;

		// Load Saved Fix:
		_current = std::max(_current, 0);
	}

private:
	int32 _current = 0;
	int32 _target = 0;
	int32 _reservedPush = 0;
	int32 _reservedPop = 0;
};

// Scenarios:
// - Find requester/provider to fulfill
// - Find availability for pickup/dropoff
enum class ResourceFindType : uint8
{
	Requester, // Check requesters for one with unfulfilled target
	Provider, // Check providers for one with unfulfilled target
	AvailableForPickup, // Check providers for (current - target - reservedPop > amount) + storages for (current - reservedPop > amount)
	AvailableForDropoff, // Check requester for (target - current - reservedPush) + storages (ask resourceSystem->CanTakeAmount() > amount)
	Drop,
	StorageDropoff,

	MarketPickup, // Market Pickup avoid markets (storage only)

	Count,
};

static const std::string ResourceFindTypeName[] =
{
	"Requester",
	"Provider",
	"AvailableForPickup",
	"AvailableForDropoff",
	"Drop",
	"StorageDropoff",

	"MarketPickup",
};

// Note: 
// We loop through all holders together, then get the nearest holder
// Human could take items from a provider to nearest storage, then from storage to a requester. This is not preferred, we want provider->requester
// To solve this issue, just make sure to check the requester first.  

// Architecture:
// - Separating out holders by type allow quick lookup
// - Shouldn't separate ResourceHolderType into groups, since ResourceHolderType tag may change during its life time... (disable building)
// - TODO: may be separate more by region too??

class ResourceSystem;

// All Resource Holders for a resource type
class ResourceTypeHolders
{
	struct HolderIdToAmount
	{
		int32 holderId;
		int32 amount;

		void operator>>(FArchive &Ar) {
			Ar << holderId;
			Ar << amount;
		}
	};
	
public:
	void Init(ResourceEnum resourceEnum, IGameSimulationCore* simulation, int32 townId)
	{
		//PUN_LOG("ResourceTypeHolders Init: %d, sim:%p, sys%p", static_cast<int>(resourceEnum), simulation, resourceSystem);
		
		_resourceEnum = resourceEnum;
		_simulation = simulation;
		_townId = townId;
		
		_findTypeToAvailableIdToAmount.resize(static_cast<int>(ResourceFindType::Count));
	}
	
	/*
	 * Const Get
	 */
	
	int32 resourceCount() const {
		int count = 0;
		for (int i = 0; i < _holders.size(); i++) {
			ResourceHolderType type = _holders[i].type;
			if (type == ResourceHolderType::Storage ||
				type == ResourceHolderType::Provider ||
				type == ResourceHolderType::Market) {
				count += _holders[i].current();
			}
		}
		return count;
	}
	int32 resourceCountWithPop() const {
		int count = 0;
		for (int i = 0; i < _holders.size(); i++) {
			ResourceHolderType type = _holders[i].type;
			if (type == ResourceHolderType::Storage ||
				type == ResourceHolderType::Provider ||
				type == ResourceHolderType::Market) {
				count += _holders[i].current();
				count -= _holders[i].reservedPop();
			}
		}
		return count;
	}
	int32 resourceCountWithDrops() const {
		int count = 0;
		for (int i = 0; i < _holders.size(); i++) {
			ResourceHolderType type = _holders[i].type;
			if (type == ResourceHolderType::Storage ||
				type == ResourceHolderType::Provider ||
				type == ResourceHolderType::Market ||
				type == ResourceHolderType::Drop ||
				type == ResourceHolderType::DropManual)
			{
				count += _holders[i].current();
			}
		}
		return count;
	}

	int32 resourceCountDropOnly() const {
		int count = 0;
		for (int i = 0; i < _holders.size(); i++) {
			if (_holders[i].type == ResourceHolderType::Drop)
			{
				count += _holders[i].current();
			}
		}
		return count;
	}

	const ResourceHolder& holderConst(int holderId) const
	{
		PUN_CHECK(holderId < _holders.size());
		return _holders[holderId];
	}

	int32 holderCount() const { return _holders.size(); }

	int32 resourceCountWithPush(int holderId) const {
		return _holders[holderId].current() + _holders[holderId].reservedPush();
	}
	int32 resourceCountWithPop(int holderId) const {
		return _holders[holderId].current() - _holders[holderId].reservedPop();
	}

	bool needResource(int holderId) const {
		check(_holders[holderId].type != ResourceHolderType::Provider);
		return resourceCountWithPush(holderId) < _holders[holderId].target();
	}

	FoundResourceHolderInfos FindHolder(ResourceFindType type, int32 amount, WorldTile2 origin, std::vector<int32> avoidIds) const;

	/*
	 * Change
	 */

	//ResourceHolder& holderMutable(int holderId) { return _holders[holderId]; }
	
	int32 SpawnHolder(ResourceEnum resourceEnum, ResourceHolderType type, int objectId, WorldTile2 tile, int target, ResourceSystem& resourceSys);
	void DespawnHolder(int holderId) { 
		_disabledHolderIds.push_back(holderId); 
		_holders[holderId].type = ResourceHolderType::Dead;

		RemoveFromAvailableStacks(holderId);
	}

	int32 CanAddResourceGlobal(int32 amount, ResourceSystem& resourceSys) const;
	int32 AddResourceGlobal(int32 amount, ResourceSystem& resourceSys);
	void RemoveResourceGlobal(int32 amount, ResourceSystem& resourceSys);

	void RemoveResourceGlobal_Unreserved(int32 amount, ResourceSystem& resourceSys);

	void ResetHolderReservers(int32 holderId, ResourceSystem& resourceSys) {
		ResourceHolder& holder = _holders[holderId]; // _enumToHolders[(int)info.resourceEnum].holderMutable(info.holderId);
		holder.ResetPopReservers(*_simulation);
		holder.ResetPushReservers(*_simulation);

		RefreshHolder(holderId, resourceSys);
	}

	void AddResource(int32 holderId, int amount, ResourceSystem& resourceSys) {
		_holders[holderId].AddResource(amount);
		RefreshHolder(holderId, resourceSys);
	}
	void RemoveResource(int32 holderId, int amount, ResourceSystem& resourceSys) {
		_holders[holderId].RemoveResource(amount);
		RefreshHolder(holderId, resourceSys);
	}

	void SetResourceTarget(int32 holderId, int32 target, ResourceSystem& resourceSys) {
		_holders[holderId].SetTarget(target);
		RefreshHolder(holderId, resourceSys);
	}
	void SetHolderTypeAndTarget(int32 holderId, ResourceHolderType type, int32 target, ResourceSystem& resourceSys) {
		_holders[holderId].SetTarget(target);
		_holders[holderId].SetType(type);
		RefreshHolder(holderId, resourceSys);
	}

	//! Reserve resource, positive = reserve for add, negative = reserve for remove
	void AddReservation(ReservationType type, int32 holderId, int unitId, int amount, ResourceSystem& resourceSys)
	{
		if (type == ReservationType::Push) {
			_holders[holderId].AddPushReservation(amount, unitId);
		}
		else if (type == ReservationType::Pop) {
			_holders[holderId].AddPopReservation(amount, unitId);
		}
		else {
			checkNoEntry();
		}
		RefreshHolder(holderId, resourceSys);
	}

	int32 RemoveReservation(ReservationType type, int32 holderId, int unitId, ResourceSystem& resourceSys)
	{
		if (type == ReservationType::Push)
		{
			int32_t amount = _holders[holderId].RemovePushReservation(unitId);
			check(amount != -1);
			RefreshHolder(holderId, resourceSys);
			return amount;
		}
		if (type == ReservationType::Pop)
		{
			int32 amount = _holders[holderId].RemovePopReservation(unitId);
			check(amount != -1);
			RefreshHolder(holderId, resourceSys);
			return amount;
		}

		checkNoEntry();
		return 0;
	}

	/*
	 * System
	 */
	
	void operator>>(FArchive& Ar)
	{
		Ar << _resourceEnum;
		SerializeVecValue(Ar, _disabledHolderIds);
		SerializeVecObj(Ar, _holders);
		
		SerializeVecVecObj(Ar, _findTypeToAvailableIdToAmount);
	}

	void resourcedebugStr(std::stringstream& ss, WorldTile2 townCenterTile, IGameSimulationCore* sim) const
	{
		if (_holders.size() > 0)
		{
			std::string resourceName = GetResourceInfo(_resourceEnum).nameStd();
			ss <<  resourceName << " Holders\n";
			for (const ResourceHolder& holder : _holders) {
				if (holder.type != ResourceHolderType::Dead) {
					holder.resourcedebugStr(ss, townCenterTile, sim);
				}
			}
			for (size_t i = 0; i < _findTypeToAvailableIdToAmount.size(); i++) {
				const std::vector<HolderIdToAmount>& availableIdToAmount = _findTypeToAvailableIdToAmount[i];
				for (size_t j = 0; j < availableIdToAmount.size(); j++) {
					ss << "[" << resourceName << " " << ResourceFindTypeName[i] << " holderId:" << availableIdToAmount[j].holderId << "]\n";
				}
			}
			ss << " - \n";
		}
	}

	void CheckIntegrity_ResourceTypeHolder();

	int32 townId() const { return _townId; }

#if WITH_EDITOR
	//static int32;
#endif

private:
	void RefreshHolder(int32 holderId, ResourceSystem& resourceSys);
	
	void RemoveFromAvailableStacks(int32 holderId) {
		for (int32 i = 0; i < static_cast<int32>(ResourceFindType::Count); i++) 
		{
			std::vector<HolderIdToAmount>& holderIdToAmounts = _findTypeToAvailableIdToAmount[i];
			for (int j = holderIdToAmounts.size(); j-- > 0;) {
				if (holderIdToAmounts[j].holderId == holderId) {
					holderIdToAmounts.erase(holderIdToAmounts.begin() + j);
					break;
				}
			}
		}
	}

	FoundResourceHolderInfos FindHolderHelper(ResourceFindType type, int32 wantAmount, WorldTile2 origin, std::vector<int32> avoidIds) const
	{
		SCOPE_CYCLE_COUNTER(STAT_PunUnit_CalcHuman_MoveResource_FindHolder);
		
		// Find by amount to get pickup candidates
		FoundResourceHolderInfos filteredInfosWrap = FilterHolders(type, origin, wantAmount);
		std::vector<FoundResourceHolderInfo>& filteredInfos = filteredInfosWrap.foundInfos;

		//if (type == ResourceFindType::AvailableForPickup && _resourceEnum == ResourceEnum::Orange) {
		//	PrintFoundResourceHolderInfos(filteredInfos, "TryFindFood", _simulation);
		//}
		//if (type == ResourceFindType::AvailableForPickup && _resourceEnum == ResourceEnum::Medicine) {
		//	PrintFoundResourceHolderInfos(filteredInfos, "TryHealup", _simulation);
		//}

		/*
		 * remove any avoidIds
		 */
		// Make sure this is not an id to avoid
		for (size_t i = filteredInfos.size(); i-- > 0;)
		{
			FoundResourceHolderInfo& info = filteredInfos[i];
			if (info.objectId != -1) {
				for (int32 avoidId : avoidIds) {
					if (avoidId == info.objectId) {
						filteredInfos.erase(filteredInfos.begin() + i);
						break;
					}
				}
			}
		}

		// No info, just return blank
		if (!filteredInfosWrap.hasInfos()) {
			return filteredInfosWrap;
		}


		// Trim amount...
		for (FoundResourceHolderInfo& filteredInfo : filteredInfos) {
			//filteredInfo.amount = std::min(filteredInfo.amount, wantAmount);
			PUN_CHECK(filteredInfo.amount <= wantAmount);
		}

		SCOPE_CYCLE_COUNTER(STAT_PunUnit_CalcHuman_MoveResource_FindHolder_Rest);

		/*
		 * One pickup satisfied all, just return it.
		 */
		FoundResourceHolderInfo foundFullInfo = FoundResourceHolderInfo::Invalid();
		int32 foundFullInfoDist = INT32_MAX;

		if (type == ResourceFindType::AvailableForDropoff)
		{
			// Also try to fill one storage first if possible
			for (int i = 0; i < filteredInfos.size(); i++)
			{
				if (filteredInfos[i].amount >= wantAmount) {
					// filteredInfos[i].distance is decreased if the amount is more than 0
					int32 dist = filteredInfos[i].distance + WorldTile2::ManDistance(origin, filteredInfos[i].tile);

					// Check if closer that last full candidate...
					if (dist < foundFullInfoDist) {
						foundFullInfo = filteredInfos[i];
						foundFullInfo.distance = dist;

						foundFullInfoDist = dist;
					}
				}
			}
		}
		else
		{
			//if (type == ResourceFindType::AvailableForPickup && _resourceEnum == ResourceEnum::Orange) {
			//	PUN_LOG("TryFindFood FindFull filteredInfos:%d", filteredInfos.size());
			//}
			
			for (int i = 0; i < filteredInfos.size(); i++)
			{
				if (filteredInfos[i].amount >= wantAmount) {
					int32 dist = WorldTile2::ManDistance(origin, filteredInfos[i].tile);

					//if (type == ResourceFindType::AvailableForPickup && _resourceEnum == ResourceEnum::Orange) {
					//	auto info = filteredInfos[i];
					//	PUN_LOG(" - TryFindFood info:%s %s amount:%d dist:%d", *GetBuildingInfo(_simulation->buildingEnumAtTile(info.tile)).nameF(), *info.tile.To_FString(), info.amount, dist);
					//}

					// Check if closer that last full candidate...
					if (dist < foundFullInfoDist) {
						foundFullInfo = filteredInfos[i];
						foundFullInfo.distance = dist;

						foundFullInfoDist = dist;
					}
				}
			}
		}
		if (foundFullInfo.isValid()) {
			// Trim and return
			foundFullInfo.amount = wantAmount;
			return FoundResourceHolderInfos({ foundFullInfo });
		}


		// Get to closest candidate in the list and loop through the list to find another pile nearest to that one (with distance cutoff)... 
		// If we can accumulate a chain of resources pickup without any traversal having too much distance, return the chain...
		// When going through, mark candidate processed if it is part of the a chain.. And when a chain fail, try the next chain skipping over processed piles
		std::vector<bool> processedPiles(filteredInfos.size(), false);
		// there is need to begin a chain with a processedPiles since that will just lead similar, shorter, lesser chain

		const int32 maxChainLinkDist = 12;
		FoundResourceHolderInfos bestChain;

		int32 loopCount;
		for (loopCount = 1000000; loopCount-- > 0;)
		{
			//
			FoundResourceHolderInfo foundClosestInfo = FoundResourceHolderInfo::Invalid();
			int32 foundClosestInfoDist = INT32_MAX;
			int foundClosestIndex = -1;
			for (int i = 0; i < filteredInfos.size(); i++) {
				if (!processedPiles[i]) {
					int32_t dist = WorldTile2::ManDistance(origin, filteredInfos[i].tile);
					// Check if closer that the cached closest candidate...
					if (dist < foundClosestInfoDist) {
						foundClosestInfo = filteredInfos[i];
						foundClosestInfo.distance = dist;
						
						foundClosestInfoDist = dist;
						foundClosestIndex = i;
					}
				}
			}
			// the whole list was processed... exit the loop
			if (!foundClosestInfo.isValid()) {
				break;
			}

			// Start the chain with closest pile
			FoundResourceHolderInfos newChain({ foundClosestInfo });
			processedPiles[foundClosestIndex] = true;

			// Link more piles into the chain until we achieve the target amount
			LOOP_CHECK_START();
			while (newChain.amount() < wantAmount)
			{
				LOOP_CHECK_END();
				
				FoundResourceHolderInfo closestNextChainNodeInfo = FoundResourceHolderInfo::Invalid();
				int32 closestNextChainNodeDist = INT32_MAX;
				int closestNextChainNodeIndex = -1;
				
				for (int i = 0; i < filteredInfos.size(); i++) {
					if (!processedPiles[i]) {
						int32 chainBackToCurrentDist = WorldTile2::ManDistance(newChain.foundInfos.back().tile, filteredInfos[i].tile);

						// Check if closer that the cached closest candidate...
						if (chainBackToCurrentDist < maxChainLinkDist && chainBackToCurrentDist < closestNextChainNodeDist) {
							closestNextChainNodeInfo = filteredInfos[i];
							closestNextChainNodeInfo.distance = chainBackToCurrentDist;
							
							closestNextChainNodeDist = chainBackToCurrentDist;
							closestNextChainNodeIndex = i;
						}
					}
				}
				// break if no valid next node
				if (!closestNextChainNodeInfo.isValid()) {
					break;
				}

				newChain.foundInfos.push_back(closestNextChainNodeInfo);
				processedPiles[closestNextChainNodeIndex] = true;
			}

			if (bestChain.hasInfos()) 
			{
				// Compete by amount first
				int32 newChainAmount = std::min(newChain.amount(), wantAmount);
				int32 bestChainAmount = std::min(bestChain.amount(), wantAmount);
				if (newChainAmount > bestChainAmount) {
					bestChain = newChain;
				}
				// Compete by the closest pile (first pile) distance to origin
				else if (newChainAmount == bestChainAmount) {
					int32 bestChainDist = WorldTile2::ManDistance(origin, bestChain.foundInfos.front().tile);
					int32 newChainDist = WorldTile2::ManDistance(origin, newChain.foundInfos.front().tile);
					if (newChainDist < bestChainDist) {
						bestChain = newChain;
					}
				}
			}
			else {
				bestChain = newChain;
			}
		}

		PUN_CHECK(loopCount > 1);
		
		PUN_CHECK(bestChain.hasInfos()); // this means filteredInfos has no infos, should've checked and return earlier
		if (!bestChain.hasInfos()) {
			return bestChain;
		}
		

		// Trim excess bestChain's foundInfos
		std::vector<FoundResourceHolderInfo>& bestChainInfos = bestChain.foundInfos;

		// Trim excess amount before returning.
		int32 amountSatisfied = 0;
		int32 newSize = 0;
		for (size_t i = 0; i < bestChainInfos.size(); i++) {
			if (amountSatisfied >= wantAmount) {
				break;
			}
			amountSatisfied += bestChainInfos[i].amount;
			newSize++;
			if (amountSatisfied > wantAmount) { // just got satisfied, make sure the whole bestChainInfos sum to amountSatisfied
				bestChainInfos[i].amount -= amountSatisfied - wantAmount;
				check(bestChainInfos[i].amount > 0);
			}
		}
		bestChainInfos.resize(newSize);


		return bestChain;
	}

	FoundResourceHolderInfos FilterHolders(ResourceFindType type, WorldTile2 origin, int32 wantAmount) const
	{
		SCOPE_CYCLE_COUNTER(STAT_PunUnit_CalcHuman_MoveResource_FindHolder_Filter);

		// foundInfos are arrange from more amount to less, then from less dist to more dist
		FoundResourceHolderInfos result;
		std::vector<FoundResourceHolderInfo>& foundInfos = result.foundInfos;

		auto tryAddToFoundInfos = [&](int32 availableAmount, const ResourceHolder& holder)
		{
			int32 amountAtLeast;
			if (type == ResourceFindType::AvailableForDropoff) {
				amountAtLeast = 1;
			} else {
				{
					//// TODO: doing an amount check like this causes issues???
					//// TODO: Figure this out?? amountAtLeast is foundInfos.back().amount because we shouldn't try to get
					//amountAtLeast = foundInfos.size() > 0 ? foundInfos.back().amount : 1; // amountAt least is 1 in the beginning... (no point to add holder with 0 amount)

					//// amountAtLeast can still be 0 if foundInfos.back().amount is 0
					//amountAtLeast = std::max(1, amountAtLeast);
				}

				amountAtLeast = 1;
			}

			// DEBUG
			//if (type == ResourceFindType::AvailableForPickup && _resourceEnum == ResourceEnum::Medicine) {
			//	PUN_LOG(" > TryHealup tryAddToFoundInfos availableAmount:%d amountAtLeast:%d  tile:%s", availableAmount, amountAtLeast, *_simulation->GetTileBuildingDescription(holder.tile));
			//}

			if (availableAmount >= amountAtLeast) // Don't get resource from node with less amount, unless necessary
			{
				//PUN_LOG("Holder:%d isDrop:%d isAvoidId:%d info:%s isConnected:%d amountAtLeast:%d", holderId, _holders[holderId].isDrop(), isAvoidId, 
				//				*ToFString(_holders[holderId].info.ToString()), sim->IsConnected(origin, holder.tile, maxFloodDist), amountAtLeast);

				bool isConnected;
				if (holder.objectId != -1) {
					DEBUG_ISCONNECTED_VAR(IsConnectedBuildingResource);
					isConnected = _simulation->IsConnectedBuilding(holder.objectId);
				}
				else {
					DEBUG_ISCONNECTED_VAR(DropResourceSystem);
					isConnected = _simulation->IsConnected(origin, holder.tile, GameConstants::MaxFloodDistance_HumanDropFetch, true); // Drop case
				}

				//PUN_LOG(" > TryHealup tryAddToFoundInfos isConnected:%d", isConnected);

				if (isConnected) 
				{
					check(availableAmount > 0);

					// Don't take more than wantAmount
					int32 amountToTake = std::min(availableAmount, wantAmount);
					
					if (type == ResourceFindType::AvailableForDropoff) {
						int32 distanceShift = (holder.current() + holder.reservedPush()) > 0 ? -20 : 0; // Try store dropoff in the storage that already has an item
						foundInfos.push_back(FoundResourceHolderInfo(holder.info, amountToTake, holder.tile, -1, distanceShift));
					} else {
						foundInfos.push_back(FoundResourceHolderInfo(holder.info, amountToTake, holder.tile));

						//PUN_LOG(" > TryHealup tryAddToFoundInfos ADDED");
					}
				}
			}
		};

		const std::vector<HolderIdToAmount>& availableIds = _findTypeToAvailableIdToAmount[static_cast<int>(type)];
		
		for (int32 i = 0; i < availableIds.size(); i++)
		{
			const ResourceHolder& holder = _holders[availableIds[i].holderId];
			
			int32 availableAmount = availableIds[i].amount;

			tryAddToFoundInfos(availableAmount, holder);
		}

		// Special case for AvailableForDropoff which will also go through StorageDropoff if there is no foundInfos at this point
		if (type == ResourceFindType::AvailableForDropoff && 
			foundInfos.size() == 0)
		{
			const std::vector<HolderIdToAmount>& availableStorageIds = _findTypeToAvailableIdToAmount[static_cast<int>(ResourceFindType::StorageDropoff)];

			for (int32 i = 0; i < availableStorageIds.size(); i++)
			{
				int32 holderId = availableStorageIds[i].holderId;
				const ResourceHolder& holder = _holders[holderId];

				// Available amount is what the storage can take...
				int32 availableAmount = _simulation->SpaceLeftFor(_resourceEnum, holder.objectId);

				tryAddToFoundInfos(availableAmount, holder);
			}
		}

		return result;
	}

private:
	IGameSimulationCore* _simulation = nullptr;
	int32 _townId = -1;
	
	ResourceEnum _resourceEnum = ResourceEnum::None;

	// For holderId reuse
	std::vector<int> _disabledHolderIds;

	// Resource Holders
	std::vector<ResourceHolder> _holders;

	std::vector<std::vector<HolderIdToAmount>> _findTypeToAvailableIdToAmount; // availableIds for each holder Type
};

// TODO: revamp resource system to use gateTile based system?? makes more sense than objId??
class ResourceSystem
{
public:
	ResourceSystem(int32 townId, IGameSimulationCore* simulation)
	{
		//PUN_LOG("ResourceSystem Construct: player:%d, sim:%p", playerId, simulation);
		
		PUN_CHECK(simulation);
		_townId = townId;
		_simulation = simulation;
		_enumToHolders.resize(ResourceEnumCount);

		for (size_t i = 0; i < _enumToHolders.size(); i++) {
			_enumToHolders[i].Init(static_cast<ResourceEnum>(i), _simulation, _townId);
		}

		for (size_t i = 0; i < _enumToHolders.size(); i++) {
			_enumToHolders[i].CheckIntegrity_ResourceTypeHolder();
		}
	}

	void CheckIntegrity_ResourceSys() const {
		PUN_CHECK(_townId >= 0);
		PUN_CHECK(_townId < 1000);
		PUN_CHECK(_simulation);

		PUN_CHECK(_enumToHolders.size() > 0);
	}

	/*
	 * Get
	 */

	int32 foodCount() {
		int count = 0;
		for (ResourceEnum foodEnum : StaticData::FoodEnums) {
			count += resourceCount(foodEnum);
		}
		return count;
	}

	int32 resourceCount(ResourceEnum resourceEnum) const
	{
		// At x5, 700 count, 0.35ms
		//SCOPE_CYCLE_COUNTER(STAT_PunResourceCountTotal);
		
		int resourceEnumInt = static_cast<int>(resourceEnum);
		bool isValid = 0 <= resourceEnumInt && resourceEnumInt < _enumToHolders.size();
		PUN_CHECK(isValid);
		if (!isValid) {
			return 0;
		}
		return _enumToHolders[static_cast<int>(resourceEnum)].resourceCount();
	}
	
	int32 resourceCountWithPop(ResourceEnum resourceEnum) const {
		return _enumToHolders[static_cast<int>(resourceEnum)].resourceCountWithPop();
	}

	int32 resourceCountWithDrops(ResourceEnum resourceEnum) {
		return _enumToHolders[static_cast<int>(resourceEnum)].resourceCountWithDrops();
	}

	int32 resourceCountDropOnly(ResourceEnum resourceEnum) {
		return _enumToHolders[static_cast<int>(resourceEnum)].resourceCountDropOnly();
	}
	
	int32 resourceCount(ResourceHolderInfo info) const {
		check(info.isValid());
		return holder(info).current(); // _enumToHolders[(int)info.resourceEnum].holderConst(info.holderId)
	}
	int32 resourceCountWithPush(ResourceHolderInfo info) const {
		check(info.isValid());
		return _enumToHolders[(int)info.resourceEnum].resourceCountWithPush(info.holderId);
	}
	int32 resourceCountWithPop(ResourceHolderInfo info) const {
		check(info.isValid());
		return _enumToHolders[(int)info.resourceEnum].resourceCountWithPop(info.holderId);
	}
	int32 resourceTarget(ResourceHolderInfo info) const {
		check(info.isValid());
		return holder(info).target(); // _enumToHolders[(int)info.resourceEnum].holderConst(info.holderId)
	}
	bool needResource(ResourceHolderInfo info) const {
		check(info.isValid());
		return _enumToHolders[(int)info.resourceEnum].needResource(info.holderId);
	}

	int32 resourceCountSafe(ResourceHolderInfo info) const {
		if (!info.isValid()) {
			return 0;
		}

		if (info.holderId < 0 && 
			info.holderId >= _enumToHolders[static_cast<int>(info.resourceEnum)].holderCount())
		{
			return 0;
		}
		return holder(info).current(); // _enumToHolders[(int)info.resourceEnum].holderConst(info.holderId)
	}
	

	bool hasResource(ResourcePair resource) const {
		return resourceCount(resource.resourceEnum) >= resource.count;
	}

	const ResourceHolder& holder(ResourceHolderInfo info) const {
		PUN_CHECK(info.isValid());

		const ResourceTypeHolders& holderGroup = _enumToHolders[(int)info.resourceEnum];

		PUN_CHECK2(info.holderId < holderGroup.holderCount(), resourcedebugStr());
		
		return holderGroup.holderConst(info.holderId);
	}
	int32 objectId(ResourceHolderInfo info) const {
		return holder(info).objectId;
	}

	bool HasAvailableFood() const {
		for (ResourceEnum resourceEnum : StaticData::FoodEnums) {
			if (resourceCountWithPop(resourceEnum) > 0) {
				return true;
			}
		}
		return false;
	}
	bool HasAvailableHeat() const {
		// TODO: check AvailableToPickup list instead
		return resourceCountWithPop(ResourceEnum::Coal) + resourceCountWithPop(ResourceEnum::Wood) > 0;
	}
	bool HasAvailableMedicine() const {
		return resourceCountWithPop(ResourceEnum::Medicine) + resourceCountWithPop(ResourceEnum::Herb) > 0;
	}
	bool HasAvailableTools() const {
		for (ResourceEnum resourceEnum : ToolsEnums) {
			if (resourceCountWithPop(resourceEnum) > 0) {
				return true;
			}
		}
		return false;
	}

	int32 townId() const { return _townId; }
	IGameSimulationCore* simulation() const { return _simulation; }

	/*
	 * Change
	 */

	void ResetHolderReservers(ResourceHolderInfo info) {
		if (info.isValid()) {
			holderGroup(info.resourceEnum).ResetHolderReservers(info.holderId, *this);
		}
	}

	//! Add/Remove resource global
	bool CanAddResourceGlobal(ResourceEnum resourceEnum, int32 amount) {
		return holderGroup(resourceEnum).CanAddResourceGlobal(amount, *this);
	}
	void AddResourceGlobal(ResourceEnum resourceEnum, int32 amount, IGameSimulationCore& simulation) {
		int amountLeft = holderGroup(resourceEnum).AddResourceGlobal(amount, *this);

		if (amountLeft > 0) {
			simulation.AddPopup(_simulation->townPlayerId(_townId),
				FText::Format(NSLOCTEXT("ResourceSys", "StorageFullLost_Pop", "Storages full... Lost {0} {1}"), TEXT_NUM(amountLeft), ResourceNameT(resourceEnum))
			);
		}
	}
	void RemoveResourceGlobal(ResourceEnum resourceEnum, int32 amount) {
		if (resourceEnum == ResourceEnum::Food) {
			int32 amountLeftToRemove = amount;
			for (ResourceEnum foodEnum : StaticData::FoodEnums) {
				int32 amountToRemove = std::min(amountLeftToRemove, resourceCount(foodEnum));
				RemoveResourceGlobal(foodEnum, amountToRemove);
				amountLeftToRemove -= amountToRemove;
			}
			PUN_CHECK(amountLeftToRemove == 0);
			return;
		}
		
		holderGroup(resourceEnum).RemoveResourceGlobal(amount, *this);
	}
	void RemoveResourceGlobal_Unreserved(ResourceEnum resourceEnum, int32 amount)
	{
		//if (resourceEnum == ResourceEnum::Food) {
		//	int32 amountLeftToRemove = amount;
		//	for (ResourceEnum foodEnum : StaticData::FoodEnums) {
		//		int32 amountToRemove = std::min(amountLeftToRemove, resourceCountWithPop(foodEnum));
		//		RemoveResourceGlobal_Unreserved(foodEnum, amountToRemove);
		//		amountLeftToRemove -= amountToRemove;
		//	}
		//	PUN_CHECK(amountLeftToRemove == 0);
		//	return;
		//}

		holderGroup(resourceEnum).RemoveResourceGlobal_Unreserved(amount, *this);
	}

	void SetResourceTarget(ResourceHolderInfo info, int32 target) {
		check(info.isValid())
		//holderMutable(info).SetTarget(target);
		holderGroup(info.resourceEnum).SetResourceTarget(info.holderId, target, *this);
		//_enumToHolders[(int)info.resourceEnum].holder(info.holderId).SetTarget(target);
	}
	void SetHolderTypeAndTarget(ResourceHolderInfo info, ResourceHolderType type, int32 target) {
		PUN_CHECK(info.isValid());
		holderGroup(info.resourceEnum).SetHolderTypeAndTarget(info.holderId, type, target, *this);
	}

	int32 CanReceiveAmount(const ResourceHolder& holder) const;

	int32 CanReceiveAmountAfterReset(const ResourceHolder& holder) const;

	//! Spawn/Despawn Holder buildings
	int32 SpawnHolder(ResourceEnum resourceEnum, ResourceHolderType holderType, int32 objectId, WorldTile2 tile, int32 target) {
		//PUN_LOG("ResourceSys SpawnHolder: player:%d enum:%d obj:%d sim:%p", _playerId, static_cast<int>(resourceEnum), objectId, _simulation);
		
		CheckIntegrity_ResourceSys();
		return holderGroup(resourceEnum).SpawnHolder(resourceEnum, holderType, objectId, tile, target, *this);
	}
	// TODO: change holder type
	void DespawnHolder(ResourceHolderInfo info) 
	{
		//PUN_LOG("ResourceSys DespawnHolder: player:%d enum:%d holderId:%d sim:%p", _playerId, static_cast<int>(info.resourceEnum), info.holderId, _simulation);
		
		check(info.isValid());
		UpdateResourceDisplay(holder(info));

		ResetHolderReservers(info);
		holderGroup(info.resourceEnum).DespawnHolder(info.holderId);
	}

	// AddDrops
	int SpawnDrop(ResourceEnum resourceEnum, int32 amount,  WorldTile2 tile, ResourceHolderType type = ResourceHolderType::Drop)
	{
		if (amount <= 0) {
			return -1;
		}

		// Don't spawn drop if not in player's territory
		int32 provinceId = _simulation->GetProvinceIdClean(tile);
		PUN_CHECK(provinceId != -1);
		if (_simulation->provinceOwnerTown(provinceId) != _townId) {
			return -1;
		}
		
		int32 holderId = holderGroup(resourceEnum).SpawnHolder(resourceEnum, type, -1, tile, 0, *this);
		//ResourceHolder& holder = holders(resourceEnum).holderMutable(holderId);
		holderGroup(resourceEnum).AddResource(holderId, amount, *this);

		ResourceHolderInfo info(resourceEnum, holderId);
		UpdateResourceDisplay(holder(info));

		_simulation->dropSystem().AddDrop(_townId, info, tile);
		return holderId;
	}


	//! Find holder
	FoundResourceHolderInfos FindHolder(ResourceFindType findType, ResourceEnum resourceEnum, int32 amount, WorldTile2 origin, std::vector<int> avoidIds = {}) const
	{
		// Counted in STAT_PunUnit_CalcHuman_MoveResource_FindHolder
		return holderGroupConst(resourceEnum).FindHolder(findType, amount, origin, avoidIds);
	}

	FoundResourceHolderInfos FindFoodHolder(ResourceFindType findType, int32 amount, WorldTile2 origin)
	{
		// Eat less expensive food first...
		auto getBestChain = [&](const std::vector<ResourceEnum>& foodEnumsCategory)
		{
			FoundResourceHolderInfos bestFoundInfos;
			for (ResourceEnum resourceEnum : foodEnumsCategory) {
				FoundResourceHolderInfos foundInfos = _enumToHolders[static_cast<int>(resourceEnum)].FindHolder(findType, amount, origin, {});

				if (!bestFoundInfos.hasInfos()) {
					bestFoundInfos = foundInfos;
				}
				else if (foundInfos.hasInfos() && 
						 foundInfos.IsBetterThan(bestFoundInfos, amount))
				{
					bestFoundInfos = foundInfos;
				}
			}
			
			return bestFoundInfos;
		};

		FoundResourceHolderInfos bestChain_HighPriority = getBestChain(FoodEnums_NonInput);
		
		FoundResourceHolderInfos bestChain_LowPriority = getBestChain(FoodEnums_Input);

		if (!bestChain_HighPriority.hasInfos()) {
			return bestChain_LowPriority;
		}
		if (!bestChain_LowPriority.hasInfos()) {
			return bestChain_HighPriority;
		}

		// Factors: distance, amount
		int32 adjustedDistance_highPriority = bestChain_HighPriority.totalDistance() * amount / std::max(1, bestChain_HighPriority.amountCapped(amount));
		int32 adjustedDistance_lowPriority = bestChain_LowPriority.totalDistance() * amount / std::max(1, bestChain_LowPriority.amountCapped(amount));

		// high priority distance count as half
		if (adjustedDistance_highPriority / 2 < adjustedDistance_lowPriority) {
			return bestChain_HighPriority;
		}
		return bestChain_LowPriority;
		
		//for (ResourceEnum resourceEnum : FoodEnums_NonInput) {
		//	FoundResourceHolderInfos foundInfos = _enumToHolders[static_cast<int>(resourceEnum)].FindHolder(findType, amount, origin, {}, maxFloodDist);

		//	if (foundInfos.hasInfos()) {
		//		return foundInfos;
		//	}
		//}
		//return FoundResourceHolderInfos();
	}

	//! Reserve resource, positive = reserve for add, negative = reserve for remove
	void AddReservation(ReservationType type, ResourceHolderInfo info, int unitId, int amount)
	{
		holderGroup(info.resourceEnum).AddReservation(type, info.holderId, unitId, amount, *this);
		
		//ResourceHolder& holder = holderMutable(info); // _enumToHolders[(int)info.resourceEnum].holder(info.holderId);
		//if (type == ReservationType::Push) {
		//	holder.AddPushReservation(amount, unitId);
		//}
		//else if (type == ReservationType::Pop) {
		//	holder.AddPopReservation(amount, unitId);
		//} else {
		//	checkNoEntry();
		//}
	}

	int32 RemoveReservation(ReservationType type, ResourceHolderInfo info, int unitId)
	{
		return holderGroup(info.resourceEnum).RemoveReservation(type, info.holderId, unitId, *this);
		
		//ResourceHolder& holder = holderMutable(info);// _enumToHolders[(int)info.resourceEnum].holder(info.holderId);
		//if (type == ReservationType::Push) 
		//{
		//	int32_t amount = holder.RemovePushReservation(unitId);
		//	check(amount != -1);
		//	return amount;
		//} 
		//if (type == ReservationType::Pop) 
		//{
		//	int32 amount = holder.RemovePopReservation(unitId);
		//	check(amount != -1);
		//	return amount;
		//}

		//checkNoEntry();
		//return 0;
	}

	// Used in IsLandCleared
	DropInfo GetDropFromSmallArea_Any(TileArea area)
	{
		//std::vector<int32> provinceIds = _simulation->GetProvinceIdsFromArea(area, true);
		std::vector<WorldRegion2> regions = area.GetOverlapRegionsSmallArea();

		for (WorldRegion2 region : regions)
		{
			const std::vector<DropInfo>& drops = _simulation->dropSystem().DropsInRegion(region);
			for (const DropInfo& drop : drops) {
				if (area.HasTile(drop.tile) && resourceCountSafe(drop.holderInfo) > 0) {
					return drop;
				}
			}
		}
		return DropInfo::Invalid();
	}
	// Used in TryClearLand
	DropInfo GetDropFromArea_Pickable(TileArea area, bool isSmallArea = false)
	{
		//std::vector<int32> provinceIds = _simulation->GetProvinceIdsFromArea(area, isSmallArea);
		std::vector<WorldRegion2> regions = area.GetOverlapRegions(isSmallArea);
		
		for (WorldRegion2 region : regions)
		{
			const std::vector<DropInfo>& drops = _simulation->dropSystem().DropsInRegion(region);
			for (const DropInfo& drop : drops) {
				if (area.HasTile(drop.tile) && resourceCountWithPop(drop.holderInfo) > 0) {
					return drop;
				}
			}
		}
		return DropInfo::Invalid();
	}
	// Used in Farm pickup (small area less than region's width/height)
	std::vector<DropInfo> GetDropsFromArea_Pickable(TileArea area, bool isSmallArea = false)
	{
		std::vector<DropInfo> results;

		//std::vector<int32> provinceIds = _simulation->GetProvinceIdsFromArea(area, isSmallArea);
		std::vector<WorldRegion2> regions = area.GetOverlapRegions(isSmallArea);
		
		for (WorldRegion2 region : regions)
		{
			const std::vector<DropInfo>& drops = _simulation->dropSystem().DropsInRegion(region);
			for (const DropInfo& drop : drops) {
				if (area.HasTile(drop.tile) && resourceCountWithPop(drop.holderInfo) > 0) {
					results.push_back(drop);
				}
			}
		}
		return results;
	}

	//! Add/Remove Resources
	void AddResource(ResourceHolderInfo info, int amount) {
		check(info.isValid());
		//holder.current += amount;
		//holder.AddResource(amount);
		holderGroup(info.resourceEnum).AddResource(info.holderId, amount, *this);
		
		UpdateResourceDisplay(holder(info));
	}
	void RemoveResource(ResourceHolderInfo info, int amount) {
		check(info.isValid());
		//holder.current -= amount;
		//holder.RemoveResource(amount);
		holderGroup(info.resourceEnum).RemoveResource(info.holderId, amount, *this);

		const ResourceHolder& holderLocal = holder(info);

		// Remove the drop when it was completely picked up
		if (holderLocal.type == ResourceHolderType::Drop ||
			holderLocal.type == ResourceHolderType::DropManual) 
		{
			//PUN_LOG("RemoveResource..Drop player:%d, %s, id:%d", _playerId, *ToFString(info.resourceName()), info.holderId);
			if (holderLocal.current() == 0) {
				_simulation->dropSystem().RemoveDrop(_townId, info, holderLocal.tile);
				DespawnHolder(info);
			}
		}

		UpdateResourceDisplay(holderLocal);
	}


	// TODO: Eventually get rid of GetTile... as we move towards using FoundResourceHolderInfo ???
	WorldTile2 GetTile(ResourceHolderInfo info);
	
	
	std::string debugStr(ResourceHolderInfo info) const;



	void UpdateResourceDisplay(const ResourceHolder& holder) const;

	/*
	 * System
	 */

	void Serialize(FArchive& Ar)
	{
		SerializeVecObj(Ar, _enumToHolders);
	}

	std::string resourcedebugStr() const {
		std::stringstream ss;
		resourcedebugStr(ss);
		return ss.str();
	}
	void resourcedebugStr(std::stringstream& ss) const
	{
		WorldTile2 townCenterTile = _simulation->GetTownhallGateFast(_townId);
		
		ss << "\n--- RESOURCE SYS --- \n";
		for (const ResourceTypeHolders& holders : _enumToHolders) {
			holders.resourcedebugStr(ss, townCenterTile, _simulation);
		}
		ss << "\n--- END RESOURCE SYS--- ";
	}
	void resourcedebugStr(std::stringstream& ss, ResourceEnum resourceEnum) const
	{
		WorldTile2 townCenterTile = _simulation->GetTownhallGateFast(_townId);

		ss << "\n--- RESOURCE SYS --- \n";
		holderGroupConst(resourceEnum).resourcedebugStr(ss, townCenterTile, _simulation);
		ss << "\n--- END RESOURCE SYS--- ";
	}

private:
	ResourceTypeHolders& holderGroup(ResourceEnum resourceEnum) { return _enumToHolders[static_cast<int>(resourceEnum)]; }
	const ResourceTypeHolders& holderGroupConst(ResourceEnum resourceEnum) const { return _enumToHolders[static_cast<int>(resourceEnum)]; }
	
private:
	int32 _townId = -1;
	IGameSimulationCore* _simulation = nullptr;

	/*
	 * Serialize
	 */
	std::vector<ResourceTypeHolders> _enumToHolders;
};

static int32 StorageTilesOccupied(std::vector<ResourceHolderInfo>& holdInfos, ResourceSystem& resourceSys)
{
	int32 occupied = 0;
	int32 countPerTile = GameConstants::StorageCountPerTile;

	for (ResourceHolderInfo holderInfo : holdInfos) {
		occupied += (countPerTile - 1 + resourceSys.resourceCountWithPush(holderInfo)) / countPerTile;
	}
	
	return occupied;
}

static int32 StorageTilesOccupied(std::unordered_map<ResourceEnum, int32>& resources)
{
	int32 occupied = 0;
	int32 countPerTile = GameConstants::StorageCountPerTile;

	for (auto it : resources) {
		occupied += (countPerTile - 1 + it.second) / countPerTile;
	}
	return occupied;
}

static int32 StorageLeftForResource(std::unordered_map<ResourceEnum, int32>& resources, ResourceEnum resourceEnum, int32 totalSpace)
{
	int32 countPerTile = GameConstants::StorageCountPerTile;
	
	int32 leftOverPartialSpace = (countPerTile - resources[resourceEnum]) % countPerTile;
	int32 leftOverFullSpace = (totalSpace - StorageTilesOccupied(resources)) * countPerTile;

	return leftOverPartialSpace + leftOverFullSpace;
}