// Fill out your copyright notice in the Description page of Project Settings.

#include "ResourceSystem.h"
#include "../Building.h"

using namespace std;

int32 ResourceTypeHolders::SpawnHolder(ResourceEnum resourceEnum, ResourceHolderType type, int objectId, WorldTile2 tile, int target, ResourceSystem& resourceSys)
{
	//_LOG(PunResource, "SpawnHolder: player:%d enum:%d type:%d size:%llu sim:%p", _playerId, static_cast<int>(resourceEnum), type, _holders.size(), _simulation);
	PUN_CHECK(_resourceEnum == resourceEnum);

	int holderId;
	if (_disabledHolderIds.size() > 0) {
		// Get old holderId
		holderId = _disabledHolderIds.back();
		_disabledHolderIds.pop_back();
		_holders[holderId] = ResourceHolder(ResourceHolderInfo(resourceEnum, holderId), type, objectId, tile, target);
	}
	else {
		// New holderId
		holderId = _holders.size();
		_holders.push_back(ResourceHolder(ResourceHolderInfo(resourceEnum, holderId), type, objectId, tile, target));
	}

	//_LOG(PunResource, " - : holderId:%d size:%llu", holderId, _holders.size());

	RefreshHolder(holderId, resourceSys);
	return holderId;
}

void ResourceTypeHolders::RefreshHolder(int32 holderId, ResourceSystem& resourceSys)
{
	// Refresh Holder to the correct type on _findTypeToAvailableIdToAmount
	
	ResourceHolder& holder = _holders[holderId];

	RemoveFromAvailableStacks(holderId);

	//_LOG(PunResource, " - : RefreshHolder: player:%d holderId:%d sim:%p", _playerId, holderId, _simulation);

	int32 amount = 0;
	
	auto pushFindType = [&](ResourceFindType findType) {
		_findTypeToAvailableIdToAmount[static_cast<int>(findType)].push_back({ holderId, amount });
	};

	switch (_holders[holderId].type)
	{
	case ResourceHolderType::Provider:
	{
		amount = holder.pickupBalance() - holder.target();
		if (amount > 0) {
			pushFindType(ResourceFindType::Provider);
			pushFindType(ResourceFindType::AvailableForPickup);
		}
		return;
	}
	case ResourceHolderType::Storage:
	{
		// Storage can available for both pickup/dropoff at the same time...
		amount = holder.pickupBalance();
		if (amount > 0) {
			pushFindType(ResourceFindType::AvailableForPickup);
			pushFindType(ResourceFindType::MarketPickup);
		}
		// Storage doesn't go to AvailableForDropoff but instead to StorageDropoff where it needs to be filtered for full storage
		amount = 0;
		pushFindType(ResourceFindType::StorageDropoff);

		// For storage, we must have it update the tilesOccupied
		_simulation->RefreshStorageStatus(holder.objectId);
			
		return;
	}
	case ResourceHolderType::Market:
	{
		// Storage can available for both pickup/dropoff at the same time...
		amount = holder.pickupBalance();
		if (amount > 0) {
			pushFindType(ResourceFindType::AvailableForPickup);
		}
		// Storage doesn't go to AvailableForDropoff but instead to StorageDropoff where it needs to be filtered for full storage
		amount = 0;
		pushFindType(ResourceFindType::StorageDropoff);

		// For storage, we must have it update the tilesOccupied
		_simulation->RefreshStorageStatus(holder.objectId);

		return;
	}
	case ResourceHolderType::Requester:
	{
		amount = holder.target() - holder.dropoffBalance();
		if (amount > 0) {
			pushFindType(ResourceFindType::AvailableForDropoff);
			pushFindType(ResourceFindType::Requester);
			return;
		}

		amount = holder.pickupBalance() - holder.target();
		if (amount > 0) {
			pushFindType(ResourceFindType::AvailableForPickup);
		}
		return;
	}
	case ResourceHolderType::Drop:
	{
		amount = holder.pickupBalance() - holder.target();
		if (amount > 0) {
			pushFindType(ResourceFindType::Drop);
			pushFindType(ResourceFindType::Provider);
			pushFindType(ResourceFindType::AvailableForPickup);
		}
		return;
	}
	default:
		return;
	}
}

FoundResourceHolderInfos ResourceTypeHolders::FindHolder(ResourceFindType type, int32 amount, WorldTile2 origin, std::vector<int32> avoidIds, int32 maxDistance, WorldTile2 maxDistanceCenter) const
{
	//! Case for having Requester in FindType::AvailableForPickup too when it is more than target...
	// There is also a case where Requester's target is set to 0 and we distribute the items a way....
	// Therefore, available to pickup should include requesters like that as wel...

	// target - current - push >= amount ... if there is pop, it won't be taken into account until done
	// current - target - pop

	// case:
	// Building is disabled
	// Requester has more than its target... (from 20 -> 0)
	// current - target - reservedPop >= amount ... use the same formula as Provider... 
	// 20 - 0 - 0 >= 10
	// 2 reservedPop of value 20 are now issued
	// Now, will it creates conflict with ResourceFindType::Requester when reservedPop is in progress?
	// target - current - push >= amount
	// 0 - 20 - 0 >= 10
	// No ... ResourceFindType::Requester not satisfied 

	// case:
	// People kept turning on/off buildings 
	// building off ... 2 reservedPop
	// turned on again ... 
	// Requester: target - current - reservedPush >= amount
	// 20 - 20 - 0 >= 10
	// FindType::Requester won't return... no reservedPush made until Pop arrives...
	// ... But what about work FillInput reservation?
	// resourceCountWithPop says FillInput is not satisfied and will trigger TryMove
	// if TryMove was trigger, but reservePop was canceled... the building will be overloaded..
	//  but that is fine since FindType::AvailableForPickup will take excess resource away...

	return FindHolderHelper(type, amount, origin, avoidIds, maxDistance, maxDistanceCenter);
}

int32 ResourceTypeHolders::CanAddResourceGlobal(int32 amount, ResourceSystem& resourceSys) const
{
	PUN_LOG("CanAddResourceGlobal: %d", amount);
	
	for (int i = 0; i < _holders.size(); i++)
	{
		// add to storage only
		if (_holders[i].type == ResourceHolderType::Storage)
		{
			int canReceiveAmount = resourceSys.CanReceiveAmountAfterReset(_holders[i]);
			amount -= canReceiveAmount;

			if (amount <= 0) {
				return true;
			}
		}
	}

	return false;
}

int32 ResourceTypeHolders::AddResourceGlobal(int32 amount, ResourceSystem& resourceSys)
{
	PUN_LOG("AddResource: %d", amount);

	for (int i = 0; i < _holders.size(); i++) 
	{
		// add to storage only
		if (_holders[i].type == ResourceHolderType::Storage)
		{
			ResetHolderReservers(i, resourceSys);

			resourceSys.UpdateResourceDisplay(_holders[i]);

			int canReceiveAmount = resourceSys.CanReceiveAmount(_holders[i]);
			if (canReceiveAmount >= amount) {
				AddResource(i, amount, resourceSys);
				return 0;
			}
			
			AddResource(i, canReceiveAmount, resourceSys);
			
			amount -= canReceiveAmount;
		}
	}
	
	return amount;
}
void ResourceTypeHolders::RemoveResourceGlobal(int32 amount, ResourceSystem& resourceSys)
{
	PUN_LOG("RemoveResource: %d", amount);

	for (int i = 0; i < _holders.size(); i++)
	{
		// Remove from storage/provider only
		if (_holders[i].type == ResourceHolderType::Storage ||
			_holders[i].type == ResourceHolderType::Provider)
		{
			//_holders[i].ResetPopReservers(*resourceSystem.simulation());
			ResetHolderReservers(i, resourceSys);

			resourceSys.UpdateResourceDisplay(_holders[i]);

			if (_holders[i].current() >= amount) {
				//_holders[i].RemoveResource(amount);
				RemoveResource(i, amount, resourceSys);
				return;
			}
			amount -= _holders[i].current();
			//_holders[i].RemoveResource(_holders[i].current());
			RemoveResource(i, _holders[i].current(), resourceSys);
		}
	}
	UE_DEBUG_BREAK();
}

void ResourceTypeHolders::RemoveResourceGlobal_Unreserved(int32 amount, ResourceSystem& resourceSys)
{
	PUN_LOG("RemoveResourceGlobal_Unreserved: %d", amount);

	for (int i = 0; i < _holders.size(); i++)
	{
		// Remove from storage/provider only
		ResourceHolder& holder = _holders[i];
		if (holder.type == ResourceHolderType::Storage ||
			holder.type == ResourceHolderType::Provider)
		{
			resourceSys.UpdateResourceDisplay(_holders[i]);
			
			int32 removableCount = holder.current() - holder.reservedPop();
			if (removableCount >= amount) {
				RemoveResource(i, amount, resourceSys);
				return;
			}
			amount -= removableCount;
			RemoveResource(i, removableCount, resourceSys);
		}
	}
	UE_DEBUG_BREAK();
}

void ResourceTypeHolders::CheckIntegrity_ResourceTypeHolder() {
	PUN_CHECK(_simulation);
}

int32 ResourceSystem::CanReceiveAmount(const ResourceHolder& holder) const
{
	if (holder.type != ResourceHolderType::Storage) {
		return 0;
	}

	if (holder.objectId == -1) {
		return 0;
	}

	Building& building = _simulation->building(holder.objectId);
	
	const std::vector<ResourceHolderInfo>& holderInfos = building.holderInfos();
	if (holderInfos.size() == 0) {
		return 0;
	}

	// Accumulate Filled tiles that isn't by this holder.
	const int32 countPerTile = GameConstants::StorageCountPerTile;
	int32 tilesLeft = building.storageSlotCount();
	int32 addToTilesCount = 0; // amount that can be added to an existing tiles
	
	for (ResourceHolderInfo holderInfo : holderInfos) 
	{
		int32 tilesOccupied = (countPerTile - 1 + resourceCountWithPush(holderInfo)) / countPerTile;;
		if (tilesOccupied > 0 && holder.info == holderInfo) {
			// Same kind of resource 
			addToTilesCount += tilesOccupied * countPerTile - resourceCountWithPush(holderInfo);
		}
		tilesLeft -= tilesOccupied;
	}

	PUN_CHECK(tilesLeft >= 0);
	if (tilesLeft < 0) {
		return 0;
	}

	return tilesLeft * countPerTile + addToTilesCount;
}

// Modified CanReceiveAmount
//  resourceCountWithPush -> resourceCount
int32 ResourceSystem::CanReceiveAmountAfterReset(const ResourceHolder& holder) const
{
	check(holder.type == ResourceHolderType::Storage);

	Building& building = _simulation->building(holder.objectId);

	const std::vector<ResourceHolderInfo>& holderInfos = building.holderInfos();
	if (holderInfos.size() == 0) {
		return 0;
	}

	// Accumulate Filled tiles that isn't by this holder.
	const int32 countPerTile = GameConstants::StorageCountPerTile;
	int32 tilesLeft = building.storageSlotCount();
	int32 addToTilesCount = 0; // amount that can be added to an existing tiles

	for (ResourceHolderInfo holderInfo : holderInfos)
	{
		int32 tilesOccupied = (countPerTile - 1 + resourceCount(holderInfo)) / countPerTile;;
		if (tilesOccupied > 0 && holder.info == holderInfo) {
			// Same kind of resource 
			addToTilesCount += tilesOccupied * countPerTile - resourceCount(holderInfo);
		}
		tilesLeft -= tilesOccupied;
	}

	PUN_CHECK(tilesLeft >= 0);
	if (tilesLeft < 0) {
		return 0;
	}

	return tilesLeft * countPerTile + addToTilesCount;
}

WorldTile2 ResourceSystem::GetTile(ResourceHolderInfo info) {
	return holder(info).tile;
}

std::string ResourceSystem::debugStr(ResourceHolderInfo info)  const
{
	ResourceHolder resourceHolder = holder(info);
	if (resourceHolder.type == ResourceHolderType::Drop) {
		return "[Drop: " + info.resourceName() + "," + to_string(resourceHolder.current()) + "]";
	}
	return _simulation->building(objectId(info)).debugStr();
}

void ResourceSystem::UpdateResourceDisplay(const ResourceHolder& holder) const
{
	if (_simulation->IsValidBuilding(holder.objectId)) {
		int32 regionId = _simulation->building(holder.objectId).centerTile().regionId();
		_simulation->SetNeedDisplayUpdate(DisplayClusterEnum::Resource, regionId, true);
	}
	// Not a building resource.. it must be dropped resource
	else {
		check(holder.tile.isValid());
		_simulation->SetNeedDisplayUpdate(DisplayClusterEnum::Resource, holder.tile.regionId(), true);
	}
}



std::vector<int32> ResourceSystem::GetResourcesSyncHashes() {
	std::vector<int32> hashes;
	hashes.push_back(_townId);
	for (int32 i = 0; i < _enumToHolders.size(); i++) {
		const std::vector<ResourceHolder>& holders = _enumToHolders[i].holders();
		hashes.push_back(holders.size());
		for (int32 j = 0; j < holders.size(); j++) {
			hashes.push_back(holders[j].GetSyncHash());
		}
	}
	return hashes;
}
void ResourceSystem::FindDesyncInResourceSyncHashes(const TArray<int32>& serverHashes, int32& currentIndex)
{
	for (int32 i = 0; i < _enumToHolders.size(); i++) {
		const std::vector<ResourceHolder>& holders = _enumToHolders[i].holders();

		int32 serverHash1 = serverHashes[currentIndex++];
		if (holders.size() != serverHash1) {
			_simulation->AddPopup(_simulation->gameManagerPlayerId(), FText::Format(
				INVTEXT("Resource {0} Hash Compare Failed holders.size:{1} serverHash:{2}"),
				ResourceNameT(static_cast<ResourceEnum>(i)), TEXT_NUM(holders.size()), TEXT_NUM(serverHash1)
			));
			return;
		}


		for (int32 j = 0; j < holders.size(); j++) {
			int32 serverHash2 = serverHashes[currentIndex++];
			if (holders[j].GetSyncHash() != serverHash2)
			{
				const ResourceHolder& holder = holders[j];

				std::stringstream ss;
				ss << "Resource Hash Compare Failed holderHash:" << holders[j].GetSyncHash() << " serverHash:" << serverHash2;
				ss << "[" << GetResourceInfoSafe(holder.info.resourceEnum).nameStd() << " " << GetResourceHolderTypeName(holder.type)
					<< " - cur:" + std::to_string(holder.current())
					<< " push:" + std::to_string(holder.reservedPush())
					<< " pop:" + std::to_string(holder.reservedPop())
					<< " target:" + std::to_string(holder.target())
					<< " tile:" << holder.tile.x << "," << holder.tile.y
					<< " ownerId:" << holder.objectId << "|";

				if (simulation()->IsValidBuilding(holder.objectId))
				{
					Building& bld = simulation()->building(holder.objectId);
					ss << " ownerPid:" << bld.playerId();
					ss << " ownerName:" << bld.buildingInfo().nameStd();
					ss << " ownerTile:" << bld.centerTile().ToString();
				}

				ss << "]\n";

				_simulation->AddPopup(_simulation->gameManagerPlayerId(), ToFText(ss.str()));
				return;
			}
		}
	}
}