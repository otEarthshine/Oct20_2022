// Fill out your copyright notice in the Description page of Project Settings.

#include "HumanStateAI.h"
#include "Building.h"
#include "Buildings/GathererHut.h"
#include "PlayerParameters.h"
#include "../DisplaySystem/PunTerrainGenerator.h"
#include "StatSystem.h"
#include "UnlockSystem.h"
#include "PunCity/PunContainers.h"
#include "ProjectileSystem.h"
#include "Buildings/House.h"
#include "OverlaySystem.h"
#include "PlayerOwnedManager.h"

using namespace std;

DECLARE_CYCLE_STAT(TEXT("PUN: Unit.CalcHuman [4.2]"), STAT_PunUnit_CalcHuman, STATGROUP_Game);
DECLARE_CYCLE_STAT(TEXT("PUN: Unit.CalcHuman.PreWorkplc [4.2.1]"), STAT_PunUnit_CalcHuman_PreWorkPlc, STATGROUP_Game);
DECLARE_CYCLE_STAT(TEXT("PUN: Unit.CalcHuman.Workplc [4.2.2]"), STAT_PunUnit_CalcHuman_WorkPlc, STATGROUP_Game);

DECLARE_CYCLE_STAT(TEXT("PUN: Unit.CalcHuman.GatherNDrop [4.2.3]"), STAT_PunUnit_CalcHuman_GatherNDrop, STATGROUP_Game);
DECLARE_CYCLE_STAT(TEXT("PUN: Unit.CalcHuman.GatherNDrop.Emergency [4.2.3.1]"), STAT_PunUnit_CalcHuman_GatherNDrop_Emergency, STATGROUP_Game);
DECLARE_CYCLE_STAT(TEXT("PUN: Unit.CalcHuman.GatherNDrop.DropPickup [4.2.3.1]"), STAT_PunUnit_CalcHuman_GatherNDrop_DropPickup, STATGROUP_Game);

DECLARE_CYCLE_STAT(TEXT("PUN: Unit.CalcHuman.TryGather"), STAT_PunUnit_CalcHuman_TryGather, STATGROUP_Game);
DECLARE_CYCLE_STAT(TEXT("PUN: Unit.CalcHuman.TryGather.FindMark"), STAT_PunUnit_CalcHuman_TryGather_FindMark, STATGROUP_Game);
DECLARE_CYCLE_STAT(TEXT("PUN: Unit.CalcHuman.TryGather.GatherSequence"), STAT_PunUnit_CalcHuman_TryGather_GatherSequence, STATGROUP_Game);

DECLARE_CYCLE_STAT(TEXT("PUN: Unit.CalcHuman.MoveResource [4.2.4]"), STAT_PunUnit_CalcHuman_MoveResource, STATGROUP_Game);
DECLARE_CYCLE_STAT(TEXT("PUN: Unit.CalcHuman.PostWorkplc [4.2.5]"), STAT_PunUnit_CalcHuman_PostWorkPlc, STATGROUP_Game);

void HumanStateAI::CalculateActions()
{
	SCOPE_CYCLE_COUNTER(STAT_PunUnit_CalcHuman);

	DEBUG_AI_VAR(CalculateActions);

	PushLastDebugSpeech();

	AddDebugSpeech("[CALCULATE] (Human) workplace:" + to_string(workplaceId()));

	_justDidResetActions = false;

	// Something went wrong... Actions didn't use the reservation...
	// TODO: Turn this on later?
	//PUN_CHECK2(reservations.empty(), debugStr());
	if (!reservations.empty()) {
		_simulation->ResetUnitActions(_id);
		return;
	}
	

	{
		SCOPE_CYCLE_COUNTER(STAT_PunUnit_CalcHuman_PreWorkPlc);
		
		if (TryCheckBadTile() || justReset()) {
			DEBUG_AI_VAR(TryCheckBadTile);
			return;
		}

		if (TryStoreInventory() || justReset()) {
			DEBUG_AI_VAR(TryStoreInventory);
			return;
		}

		if (TryFindFood() || justReset()) {
			DEBUG_AI_VAR(TryFindFood);
			return;
		}
		if (TryHeatup() || justReset()) {
			DEBUG_AI_VAR(TryHeatup);
			return;
		}

		if (TryToolup() || justReset()) {
			DEBUG_AI_VAR(TryToolup);
			return;
		}
		if (TryHealup() || justReset()) {
			DEBUG_AI_VAR(TryHealup);
			return;
		}

		if (TryFillLuxuries() || justReset()) {
			DEBUG_AI_VAR(TryFillLuxuries);
			return;
		}

		if (TryFun() || justReset()) {
			DEBUG_AI_VAR(TryFun);
			return;
		}
	}

	if (Building* workplc = workplace())
	{
		SCOPE_CYCLE_COUNTER(STAT_PunUnit_CalcHuman_WorkPlc);

		if (workplc->isEnum(CardEnum::Townhall)) { // RoadMaker
			if (TryConstructRoad() || justReset()) {
				DEBUG_AI_VAR(TryConstructRoad_RoadMaker);
				return;
			}
		}
		else if (!workplc->isConstructed()) { // Constructor
			if (TryConstruct() || justReset()) {
				DEBUG_AI_VAR(TryConstruct_Constructor);
				return;
			}
		}
		else if (workplc->isEnum(CardEnum::FruitGatherer)) { // Berries
			if (TryGatherFruit() || justReset()) {
				DEBUG_AI_VAR(TryGatherFruit);
				return;
			}
		}
		else if (workplc->isEnum(CardEnum::HuntingLodge)) {
			if (TryHunt() || justReset()) {
				DEBUG_AI_VAR(TryHunt);
				return;
			}
		}
		else if (
			workplc->isEnum(CardEnum::RanchPig) ||
			workplc->isEnum(CardEnum::RanchSheep) ||
			workplc->isEnum(CardEnum::RanchCow)) 
		{
			if (TryRanch() || justReset()) {
				DEBUG_AI_VAR(TryRanch);
				return;
			}
		}
		else if (workplc->isEnum(CardEnum::Farm)) {
			if (TryFarm() || justReset()) {
				DEBUG_AI_VAR(TryFarm);
				return;
			}
		}
		else if (workplc->isEnum(CardEnum::Forester)) {
			if (TryForesting() || justReset()) {
				DEBUG_AI_VAR(TryForesting);
				return;
			}
		}
		else if (IsSpecialProducer(workplc->buildingEnum())) {
			if (TryProduce() || justReset()) {
				DEBUG_AI_VAR(TryProduce_Special);
				return;
			}
		}
		//// TODO: remove
		//else if (IsConsumerOnlyWorkplace(workplc->buildingEnum())) {
		//	if (TryConsumerWork() || justReset()) return;
		//}
		else if (IsMountainMine(workplc->buildingEnum())) {
			if (workplc->subclass<Mine>().oreLeft() <= 0) {
				AddDebugSpeech("(Failed)TryProduce: No ore left");
				return;
			}
			if (TryProduce() || justReset()) {
				DEBUG_AI_VAR(TryProduce_Mine);
				return;
			}
		}
		else if (IsProducer(workplc->buildingEnum())) { // All other producers
			if (TryProduce() || justReset()) {
				DEBUG_AI_VAR(TryProduce_Others);
				return;
			}
		}
	}

	{
		SCOPE_CYCLE_COUNTER(STAT_PunUnit_CalcHuman_GatherNDrop);

		// Emergency gather trees
		//  No heat (but have food)
		if (!_simulation->resourceSystem(_playerId).HasAvailableHeat() &&
			_simulation->resourceSystem(_playerId).HasAvailableFood())
		{
			SCOPE_CYCLE_COUNTER(STAT_PunUnit_CalcHuman_GatherNDrop_Emergency);
			
			AddDebugSpeech(">>>Emergency gather trees");
			if (TryMoveResourceAny(GetResourceInfo(ResourceEnum::Wood), 10)) {
				DEBUG_AI_VAR(EmergencyTree10_TryMoveResourceAny);
				return;
			}
			
			if (TryGather(true) || justReset()) {
				DEBUG_AI_VAR(TryGatherTreeOnly_Succeed);
				return;
			}

			if (TryMoveResourceAny(GetResourceInfo(ResourceEnum::Wood), 0)) {
				DEBUG_AI_VAR(EmergencyTree0_TryMoveResourceAny);
				return;
			}
		}

		// Drop pickup
		if (GameRand::RandChance(3)) 
		{
			SCOPE_CYCLE_COUNTER(STAT_PunUnit_CalcHuman_GatherNDrop_DropPickup);
			
			for (ResourceInfo info : ResourceInfos) {
				int32 amountAtLeast = 10; // Need at least 
				if (TryMoveResourcesAny(info.resourceEnum, ResourceFindType::Drop, ResourceFindType::AvailableForDropoff, amountAtLeast) || justReset()) {
					DEBUG_AI_VAR(TryMoveResourcesAny_Drop);
					return;
				}
			}
		}

		if (TryGather(false) || justReset()) {
			DEBUG_AI_VAR(TryGatherNotTreeOnly_Succeed);
			return;
		}
	}

	{
		SCOPE_CYCLE_COUNTER(STAT_PunUnit_CalcHuman_MoveResource);

		// Proposal...
		// First pass to move larger amount...
		// Second pass just move any little amount...
		// But then this could lead to some important building with just 1 more not getting filled?
		// But may be that is fine? since in that case, the building's workers who is blocked will do the last filling...

		// First pass to move larger amount...
		for (ResourceInfo info : ResourceInfos) {
			if (TryMoveResourceAny(info, 10) || justReset()) {
				DEBUG_AI_VAR(TryMoveResourcesAny_All10);
				return;
			}
		}

		// Second pass just move any little amount... only check once in a while...
		if (GameRand::RandChance(3)) {
			for (ResourceInfo info : ResourceInfos) {
				if (TryMoveResourceAny(info, 0) || justReset()) {
					DEBUG_AI_VAR(TryMoveResourcesAny_All0);
					return;
				}
			}
		}

		AddDebugSpeech("(Failed)TryMoveResourcesAny...");

	}

	{
		SCOPE_CYCLE_COUNTER(STAT_PunUnit_CalcHuman_PostWorkPlc);
		
		// Road
		if (TryConstructRoad() || justReset()) {
			DEBUG_AI_VAR(TryConstructRoad);
			return;
		}
		

		if (TryGoNearbyHome() || justReset()) {
			DEBUG_AI_VAR(TryGoNearbyHome);
			return;
		}

		DEBUG_AI_VAR(MoveRandomly);
		Add_MoveRandomly();
	}

	_unitState = UnitState::Idle;
}

void HumanStateAI::MoveResourceSequence(std::vector<FoundResourceHolderInfo> providerInfos, std::vector<FoundResourceHolderInfo> dropoffInfos)
{
	PUN_UNIT_CHECK(providerInfos.size() > 0);
	PUN_UNIT_CHECK(dropoffInfos.size() > 0);

	// Reverse the order since _actions is a stack
	for (int i = dropoffInfos.size(); i-- > 0;) {
		PUN_UNIT_CHECK(dropoffInfos[i].isValid());

		// if this is a building... turn off needInput icon
		// NeedInput is switched on by building's worker.. and switched off by workers and haulers (code below)
		int32 dropoffBuildingId = resourceSystem().holder(dropoffInfos[i].info).objectId;
		if (dropoffBuildingId != -1) {
			_simulation->building(dropoffBuildingId).workplaceInputNeeded = ResourceEnum::None;
		}

		ReserveResource(ReservationType::Push, dropoffInfos[i].info, dropoffInfos[i].amount);

		Add_DropoffResource(dropoffInfos[i].info, dropoffInfos[i].amount);
		Add_MoveToResource(dropoffInfos[i].info);
		//Add_MoveTo(resourceSystem().GetTile(dropoffInfos[i].info));
	}

	for (int i = providerInfos.size(); i-- > 0;) {
		PUN_UNIT_CHECK(providerInfos[i].isValid());

		// Reserve
		ReserveResource(ReservationType::Pop, providerInfos[i].info, providerInfos[i].amount);

		// Set Actions
		Add_PickupResource(providerInfos[i].info, providerInfos[i].amount);
		Add_MoveToResource(providerInfos[i].info);
		//Add_MoveTo(resourceSystem().GetTile(providerInfos[i].info));
	}

	_unitState = UnitState::MoveResource;
}

void HumanStateAI::GatherSequence(NonWalkableTileAccessInfo accessInfo)
{
	// Reservation must be done here so that FindNearestMark will not return the same tile...
	int32 tileId = accessInfo.tile.tileId();
	ReserveTreeTile(tileId);

	int32 cutTicks = 1;

	UnitAnimationEnum animationEnum = UnitAnimationEnum::ChopWood;

	TileObjInfo info = treeSystem().tileInfo(tileId);
	ResourceTileType tileType = info.type;
	if (tileType == ResourceTileType::Tree) {
		cutTicks = playerParameters().CutTreeTicks();
	}
	else if (tileType == ResourceTileType::Bush) {
		cutTicks = playerParameters().CutTreeTicks() / 12; // base 600, make it 50

		if (info.treeEnum == TileObjEnum::GrassGreen) {
			cutTicks /= 2; // base 600, make it 50
		}
	}
	else if (tileType == ResourceTileType::Deposit) {
		cutTicks = playerParameters().HarvestDepositTicks();
		animationEnum = UnitAnimationEnum::StoneMining;
	}

	cutTicks = cutTicks * 100 / workEfficiency100();

	// Set Actions
	Add_HarvestTileObj(accessInfo.tile);
	Add_Wait(cutTicks, animationEnum);
	Add_MoveToward(accessInfo.tile.worldAtom2(), 20000);
	Add_MoveTo(accessInfo.nearbyTile);
}


bool HumanStateAI::TryMoveResourcesAnyProviderToDropoff(ResourceFindType providerType, FoundResourceHolderInfo dropoffInfo)
{
	PUN_CHECK2(dropoffInfo.isValid(), debugStr());
	if (!IsMoveValid(dropoffInfo.tile)) {
		AddDebugSpeech("(Failed)TryMoveResourcesAnyProviderToDropoff: " + dropoffInfo.ToString());
		return false;
	}

	FoundResourceHolderInfos foundProviders = resourceSystem().FindHolder(providerType, dropoffInfo.info.resourceEnum, dropoffInfo.amount, unitTile());
	if (!foundProviders.hasInfos()) {
		AddDebugSpeech("(Failed)TryMoveResourcesAnyProviderToDropoff: " + dropoffInfo.ToString() + " providerInfo invalid: " + ResourceFindTypeName[(int)providerType]);
		return false;
	}

	for (FoundResourceHolderInfo foundInfo : foundProviders.foundInfos) {
		PUN_CHECK2(resourceSystem().resourceCountWithPop(foundInfo.info) >= foundInfo.amount, (resourceSystem().debugStr(foundInfo.info) + "\n" + debugStr()));
	}

	// TODO: this making vector is ugly and FoundResourceHolderInfo should be renamed..
	// readjust amount to fit foundProviders
	dropoffInfo.amount = foundProviders.amount();
	MoveResourceSequence(foundProviders.foundInfos, { dropoffInfo }); // Can't drop off more than what foundProviders can provide
	AddDebugSpeech("(Succeed)TryMoveResourcesAnyProviderToDropoff:");
	return true;
}

bool HumanStateAI::TryMoveResourcesProviderToAnyDropoff(FoundResourceHolderInfo providerInfo, ResourceFindType dropoffType)
{
	if (!IsResourceMoveValid(providerInfo.info)) {
		//AddDebugSpeech("(Failed)TryMoveResourcesProviderToAnyDropoff: " + providerInfo.resourceName() + " providerInfo not move valid");
		return false;
	}

	if (resourceSystem().resourceCountWithPop(providerInfo.info) < providerInfo.amount) {
		//AddDebugSpeech("(Failed)TryMoveResourcesProviderToAnyDropoff: " + providerInfo.resourceName() + " providerInfo has too little resource");
		return false;
	}

	// BIBU resourceSystem().GetTile(providerInfo)
	FoundResourceHolderInfos foundDropoffs = resourceSystem().FindHolder(dropoffType, providerInfo.info.resourceEnum, providerInfo.amount, providerInfo.tile); // starTile is provider's tile
	if (!foundDropoffs.hasInfos()) {
		//AddDebugSpeech("(Failed)TryMoveResourcesProviderToAnyDropoff: " + providerInfo.resourceName() + " dropoffInfo invalid: " + ResourceFindTypeName[(int)dropoffType]);
		return false;
	}

	MoveResourceSequence({ providerInfo }, foundDropoffs.foundInfos); // can't provide more than what foundDropoff can take
	AddDebugSpeech("(Succeed)TryMoveResourcesProviderToAnyDropoff:");
	return true;
}

bool HumanStateAI::TryMoveResourcesAny(ResourceEnum resourceEnum, ResourceFindType providerType, ResourceFindType dropoffType, int32_t amountAtLeast)
{
	int wantAmount = 10;
	FoundResourceHolderInfos foundProviders = resourceSystem().FindHolder(providerType, resourceEnum, wantAmount, unitTile());
	if (!foundProviders.hasInfos()) {
		//if (resourceEnum == ResourceEnum::Wood) {
		//	AddDebugSpeech("TryMoveAnyAny(Failed): " + ResourceName(resourceEnum) + " no provider: " + ResourceFindTypeName[(int)providerType]);
		//}
		return false;
	}
	if (foundProviders.amount() < amountAtLeast) {
		//if (resourceEnum == ResourceEnum::Wood) {
		//	AddDebugSpeech("TryMoveAnyAny(Failed): " + ResourceName(resourceEnum) + " providers giving too little. dropType:" + ResourceFindTypeName[(int)providerType] + 
		//					" providerAmount:" + to_string(foundProviders.amount()) + " amountAtLeast:" + to_string(amountAtLeast) + " providerSize:" + to_string(foundProviders.foundInfos.size()));
		//}
		return false;
	}

	int32 storageCount = 0;
	for (FoundResourceHolderInfo foundInfo : foundProviders.foundInfos) {
		PUN_CHECK2(resourceSystem().resourceCountWithPop(foundInfo.info) >= foundInfo.amount, debugStr());

		if (resourceSystem().holder(foundInfo.info).type == ResourceHolderType::Storage) {
			storageCount++;
		}
	}

	// If providers are storages, make sure we are dropping off at requester (no storage to storage loop)
	if (storageCount == foundProviders.foundInfos.size()) {
		dropoffType = ResourceFindType::Requester;
	}

	// If provider is a storage, make sure we are dropping off at requester (no storage to storage loop)
	//if (resourceSystem().holder(foundProvider.info).type == ResourceHolderType::Storage) {
	//	dropoffType = ResourceFindType::Requester;
	//}

	// objectIds to avoid list
	std::vector<int32_t> objectIds;
	for (FoundResourceHolderInfo foundInfo : foundProviders.foundInfos) {
		objectIds.push_back(resourceSystem().objectId(foundInfo.info));
	}

	// FindHolder using just first element in foundProviders
	FoundResourceHolderInfos foundDropoffs = resourceSystem().FindHolder(dropoffType, resourceEnum, foundProviders.amount(),
																 resourceSystem().GetTile(foundProviders.foundInfos[0].info), objectIds);
	if (!foundDropoffs.hasInfos() || foundDropoffs.amount() < amountAtLeast) {
		//AddDebugSpeech("TryMoveAnyAny(Failed): " + ResourceName(resourceEnum) + " dropoff invalid: " + ResourceFindTypeName[(int)dropoffType]);
		return false;
	}

	// foundProviders.foundInfos's amount must be trimmed to be equal to dropoff's amount
	foundProviders.Trim(foundDropoffs.amount());
	
	MoveResourceSequence(foundProviders.foundInfos, foundDropoffs.foundInfos);
	AddDebugSpeech("TryMoveAnyAny(Succeed): " + ResourceName(resourceEnum) + " " + ResourceFindTypeName[(int)providerType] + " " + ResourceFindTypeName[(int)dropoffType] + " amount:" + to_string(amountAtLeast));
	return true;
}


bool HumanStateAI::TryStoreInventory()
{
	auto& resourceSystem = _simulation->resourceSystem(_playerId);

	// Free inventory
	std::vector<ResourcePair>& inventory = _inventory.resourcePairs();

	// Put inventory in storage if there are more than 10
	for (int i = 0; i < inventory.size(); i++) {
		if (inventory[i].count < 10) {
			// less than 10 inventroy count, skip putting this in storage (will just drop)
			continue;
		}
		int32_t amount = min(inventory[i].count, 10);
		FoundResourceHolderInfos foundDropoffs = resourceSystem.FindHolder(ResourceFindType::AvailableForDropoff,
																	inventory[i].resourceEnum, amount, unitTile());
		if (foundDropoffs.hasInfos()) {

			for (FoundResourceHolderInfo foundInfo : foundDropoffs.foundInfos) {
				// Reserve
				ReserveResource(ReservationType::Push, foundInfo.info, foundInfo.amount);

				// Set Actions
				Add_DropoffResource(foundInfo.info, foundInfo.amount);
				Add_MoveToResource(foundInfo.info);
				//Add_MoveTo(foundInfo.tile);

				_unitState = UnitState::StoreInventory;
			}

			AddDebugSpeech("(Succeed)TryStoreInventory");
			return true;
		}
	}

	// When TryStoreInventory failed even when there is inventory, just drop the inventory
	// execution also reach here if inventory is inventory is less than 10
	if (!inventory.empty()) {
		for (int i = 0; i < inventory.size(); i++) {
			WorldTile2 tile = _simulation->FindNearbyDroppableTile(unitTile(), TileArea(0,0,0,0));

			if (inventory[i].count > 0) {
				resourceSystem.SpawnDrop(inventory[i].resourceEnum, inventory[i].count, tile);
			}
		}
		inventory.clear();
		AddDebugSpeech("(Failed)TryStoreInventory: drop inventory");
		return false;
	}

	AddDebugSpeech("(Failed)TryStoreInventory: inventory empty");
	return false;
}

bool HumanStateAI::TryClearLand(TileArea area)
{
	// Clear tileObjs
	auto& treeSys = treeSystem();
	NonWalkableTileAccessInfo cutTileAccessInfo = NonWalkableTileAccessInfoInvalid;
	area.ExecuteOnAreaWithExit_WorldTile2([&](WorldTile2 tile) {
		int32_t tileId = tile.tileId();
		if (!treeSys.IsEmptyTile_WaterAsEmpty(tileId)) {
			if (!treeSys.IsReserved(tileId)) {
				NonWalkableTileAccessInfo accessInfo = _simulation->TryAccessNonWalkableTile(unitTile(), tile, GameConstants::MaxFloodDistance_Human, true);
				if (accessInfo.isValid()) {
					cutTileAccessInfo = accessInfo;
				}
				return true;
			}
		}
		return false;
	});
	
	if (cutTileAccessInfo.isValid())
	{
		GatherSequence(cutTileAccessInfo);

		ResourceTileType tileType = treeSystem().tileInfo(cutTileAccessInfo.tile.tileId()).type;
		if (tileType == ResourceTileType::Tree) {
			_unitState = UnitState::ClearLandCutTree;
		} else if (tileType == ResourceTileType::Deposit) {
			_unitState = UnitState::ClearLandCutStone;
		} else {
			_unitState = UnitState::ClearLandCutBush;
		}
		
		AddDebugSpeech("(Succeed)TryClearLand: CutTree");
		return true;
	}
	
	// Clear drops
	WorldTile2 tile = WorldTile2::Invalid;
	DropInfo drop = resourceSystem().GetDropFromArea_Pickable(area);
	if (drop.isValid())
	{
		ResourceHolderInfo holderInfo = drop.holderInfo;
		int32 amount = resourceSystem().resourceCountWithPop(holderInfo);

		// Try dropping the resource off in proper store
		FoundResourceHolderInfos foundDropoffs = resourceSystem().FindHolder(ResourceFindType::AvailableForDropoff, holderInfo.resourceEnum, amount, unitTile());
		if (foundDropoffs.hasInfos()) 
		{
			// Reserve
			ReserveResource(ReservationType::Pop, holderInfo, amount);
			
			for (FoundResourceHolderInfo foundInfo : foundDropoffs.foundInfos) {
				// Reserve
				ReserveResource(ReservationType::Push, foundInfo.info, foundInfo.amount);

				// Set Actions
				Add_DropoffResource(foundInfo.info, foundInfo.amount);
				Add_MoveToResource(foundInfo.info);
				//Add_MoveTo(foundInfo.tile);
			}

			Add_PickupResource(holderInfo, amount);
			Add_MoveToResource(holderInfo);
			_unitState = UnitState::ClearLandRemoveDrop;

			AddDebugSpeech("(Succeed)TryClearLand: RemoveDrop proper dropoff");
			return true;
		}

		// Try dropping in nearby tile
		WorldTile2 newDropTile = _simulation->FindNearbyDroppableTile(drop.tile, area);
		if (newDropTile.isValid())
		{
			// Reserve
			ReserveResource(ReservationType::Pop, holderInfo, amount);

			// Set Actions
			Add_DropInventoryAction();
			Add_MoveTo(newDropTile);
			Add_PickupResource(holderInfo, amount);
			Add_MoveToResource(holderInfo);
			//Add_MoveTo(resourceSystem().GetTile(holderInfo));

			_unitState = UnitState::ClearLandRemoveDrop;
			AddDebugSpeech("(Succeed)TryClearLand: RemoveDrop place nearby");
			return true;
		}
	}

	AddDebugSpeech("(Failed)TryClearLand:");
	return false;
}

// Find food in civilization, if there is really none, get it the wild way
bool HumanStateAI::TryFindFood()
{
	// Go get food only if needed
	int32 maxFood =  unitInfo().maxFoodTicks;
	if (_food < maxFood * 3 / 4) 
	{
		int32 wantAmount = unitInfo().foodPerFetch;
		auto& resourceSystem = _simulation->resourceSystem(_playerId);
		FoundResourceHolderInfos foundProviders = resourceSystem.FindFoodHolder(ResourceFindType::AvailableForPickup, wantAmount, unitTile());

		if (foundProviders.hasInfos()) {
			// Just go to the best provider (the first one)
			FoundResourceHolderInfo bestProvider = foundProviders.best();
			
			// Reserve
			ReserveResource(ReservationType::Pop, bestProvider.info, bestProvider.amount);

			// Set Actions
			Add_Eat(bestProvider.info.resourceEnum);
			Add_Wait(60);
			Add_PickupResource(bestProvider.info, bestProvider.amount);
			Add_MoveToResource(bestProvider.info);
			//Add_MoveTo(bestProvider.tile);
			
			_unitState = UnitState::GetFood;

			AddDebugSpeech("(Success)TryFindFood: pushed actions");
			return true;
		}

		// Event Log
		int32 foodNeededPerHalfMinute = _simulation->population(_playerId) * HumanFoodPerYear / Time::MinutesPerYear / 2;
		if (_simulation->foodCount(_playerId) < foodNeededPerHalfMinute) {
			if (_simulation->TryDoNonRepeatAction(_playerId, NonRepeatActionEnum::FoodReserveLowEvent, Time::TicksPerSecond * 30)) {
				_simulation->AddEventLog(_playerId, "Food reserve is low.", true);
				_simulation->soundInterface()->Spawn2DSound("UI", "FoodLowBell", _playerId);
			}
			AddDebugSpeech("(Try)TryFindFood: providerInfo invalid  (Food reserve is low)");
		} else {
			AddDebugSpeech("(Try)TryFindFood: providerInfo invalid (Food reserve not low)");
		}

		AddDebugSpeech("(Failed)TryFindFood: Cannot find food");
		return false;
	}

	//if (_food < maxFood / 4) {
	//	AddDebugSpeech("(Failed)TryFindFood: TryFindWildFood");
	//	return TryFindWildFood(true);
	//}
	
	AddDebugSpeech("(Failed)TryFindFood: Has food");
	return false;
}

bool HumanStateAI::TryHeatup()
{
	if (_houseId == -1) {
		AddDebugSpeech("(Failed)TryHeatup no house");
		return false;
	}
	Building& house = _simulation->building(_houseId);

	// Can heat with either coal or wood
	int32 fuelCount = house.GetResourceCountWithPush(ResourceEnum::Wood) + house.GetResourceCountWithPush(ResourceEnum::Coal);
	if (fuelCount < 5) 
	{
		auto& playerOwned = _simulation->playerOwned(_playerId);
		
		// Try to fill with coal first since it is cheaper
		if (playerOwned.GetHouseResourceAllow(ResourceEnum::Coal) &&
			TryMoveResourcesAnyProviderToDropoff(ResourceFindType::AvailableForPickup, house.GetFoundHolderInfo(ResourceEnum::Coal, 10))) 
		{
			AddDebugSpeech("(Succeed)TryHeatup move coal");
			return true;
		}

		// Otherwise fill with wood
		if (playerOwned.GetHouseResourceAllow(ResourceEnum::Wood) &&
			TryMoveResourcesAnyProviderToDropoff(ResourceFindType::AvailableForPickup, house.GetFoundHolderInfo(ResourceEnum::Wood, 10)))
		{
			AddDebugSpeech("(Succeed)TryHeatup move firewood");
			return true;
		}
	}

	auto coalHolderInfo = house.holderInfo(ResourceEnum::Coal);
	auto woodHolderInfo = house.holderInfo(ResourceEnum::Wood);

	// Note: need this long version because we need to take into account the pop
	int32 coalCount = resourceSystem().resourceCountWithPop(coalHolderInfo);
	int32 woodCount = resourceSystem().resourceCountWithPop(woodHolderInfo);

	if (_heat < maxHeat() * 3 / 4 && 
		(coalCount > 0 || woodCount > 0) &&
		IsMoveValid(house.gateTile()))
	{
		int32 amount = 1;
		auto holderInfo = coalCount > 0 ? coalHolderInfo : woodHolderInfo;

		ReserveResource(ReservationType::Pop, holderInfo, amount);

		Add_Heat();
		Add_Wait(Time::TicksPerSecond * 2);
		Add_PickupResource(holderInfo, amount);
		Add_MoveToResource(holderInfo);
		//Add_MoveTo(house.gateTile());
		
		_unitState = UnitState::GetHeat;

		AddDebugSpeech("(Succeed)TryHeatup use firewood");
		return true;
	}

	// Event Log
	if (_simulation->resourceSystem(_playerId).resourceCount(ResourceEnum::Wood) == 0 &&
		_simulation->resourceSystem(_playerId).resourceCount(ResourceEnum::Coal) == 0)
	{
		if (_simulation->TryDoNonRepeatAction(_playerId, NonRepeatActionEnum::WoodReserveLowEvent, Time::TicksPerSecond * 30)) {
			_simulation->AddEventLog(_playerId, "Fuel reserve is low.", true);
		}

		AddDebugSpeech("(Try)TryFindFood: providerInfo invalid  (no stored food)");
	}

	AddDebugSpeech("(Failed)TryHeatup");
	return false;
}

bool HumanStateAI::TryToolup()
{
	if (isBelowWorkingAge()) {
		AddDebugSpeech("(Failed)TryToolup: Is child");
		return false;
	}
	
	// Tool up if the tool will only last 1 more round...
	if (Time::Ticks() > _nextToolNeedTick - Time::TicksPerRound)
	{
		int32 wantAmount = 1;
		auto& resourceSystem = _simulation->resourceSystem(_playerId);
		bool hasToolsInStorage = false;

		for (ResourceEnum resourceEnum : ToolResources)
		{
			FoundResourceHolderInfos foundProviders = resourceSystem.FindHolder(ResourceFindType::AvailableForPickup, resourceEnum, wantAmount, unitTile());

			if (resourceSystem.resourceCount(resourceEnum) > 0) {
				hasToolsInStorage = true;
			}
			
			if (foundProviders.hasInfos()) {
				// Just go to the best provider (the first one)
				FoundResourceHolderInfo bestProvider = foundProviders.best();

				// Reserve
				ReserveResource(ReservationType::Pop, bestProvider.info, bestProvider.amount);

				// Set Actions
				Add_UseTools(bestProvider.info.resourceEnum);
				Add_Wait(60);
				Add_PickupResource(bestProvider.info, bestProvider.amount);
				Add_MoveToResource(bestProvider.info);
				//Add_MoveTo(bestProvider.tile);

				_unitState = UnitState::GetTools;

				AddDebugSpeech("(Success)TryToolup: pushed actions");
				return true;
			}
		}

		if (!hasToolsInStorage) {
			if (_simulation->TryDoNonRepeatAction(_playerId, NonRepeatActionEnum::NeedMoreToolsEvent, Time::TicksPerSecond * 30)) {
				_simulation->AddEventLog(_playerId, "Need more tools.", true);
				_simulation->soundInterface()->Spawn2DSound("UI", "NoToolsBell", _playerId);
			}
		}
		AddDebugSpeech("(Failed)TryToolup: Cannot find tool");
		return false;
	}

	AddDebugSpeech("(Failed)TryToolup: Has tool");
	return false;
}
bool HumanStateAI::TryHealup()
{
	if (_isSick)
	{
		int32 wantAmount = 1;
		auto& resourceSystem = _simulation->resourceSystem(_playerId);

		for (ResourceEnum resourceEnum : MedicineResources)
		{
			wantAmount = resourceEnum == ResourceEnum::Herb ? 2 : 1;
			FoundResourceHolderInfos foundProviders = resourceSystem.FindHolder(ResourceFindType::AvailableForPickup, resourceEnum, wantAmount, unitTile());

			if (foundProviders.hasInfos()) {
				// Just go to the best provider (the first one)
				FoundResourceHolderInfo bestProvider = foundProviders.best();

				// Reserve
				ReserveResource(ReservationType::Pop, bestProvider.info, bestProvider.amount);

				// Set Actions
				Add_UseMedicine(bestProvider.info.resourceEnum);
				Add_Wait(60);
				Add_PickupResource(bestProvider.info, bestProvider.amount);
				Add_MoveToResource(bestProvider.info);
				//Add_MoveTo(bestProvider.tile);

				_unitState = UnitState::GetMedicine;

				AddDebugSpeech("(Success)TryHealup: pushed actions");
				return true;
			}
		}

		AddDebugSpeech("(Failed)TryHealup: Cannot find medicine");
		return false;
	}

	AddDebugSpeech("(Failed)TryHealup: Has medicine");
	return false;
}

bool HumanStateAI::TryFillLuxuries()
{
	if (_houseId == -1) {
		AddDebugSpeech("(Failed)TryFillLuxuries: No House");
		return false;
	}
	House& house = _simulation->building(_houseId).subclass<House>(CardEnum::House);

	const int32_t maxPickupAmount = 10;

	// Get resource up to the current level
	std::vector<ResourceEnum> resourceEnums;
	switch(house.houseLvl())
	{
	case 7:
	case 6:
	case 5:
		resourceEnums.insert(resourceEnums.end(), TierToLuxuryEnums[3].begin(), TierToLuxuryEnums[3].end());
	case 4:
	case 3:
		resourceEnums.insert(resourceEnums.end(), TierToLuxuryEnums[2].begin(), TierToLuxuryEnums[2].end());
	case 2:
	case 1:
		resourceEnums.insert(resourceEnums.end(), TierToLuxuryEnums[1].begin(), TierToLuxuryEnums[1].end());
	}

	auto& playerOwned = _simulation->playerOwned(_playerId);

	for (ResourceEnum resourceEnum : resourceEnums)
	{
		if (playerOwned.GetHouseResourceAllow(resourceEnum) &&
			house.GetResourceCountWithPush(resourceEnum) < 5 &&
			TryMoveResourcesAnyProviderToDropoff(ResourceFindType::AvailableForPickup, house.GetFoundHolderInfo(resourceEnum, maxPickupAmount)))
		{
			AddDebugSpeech("(Succeed)TryFillLuxuries: " + house.GetFoundHolderInfo(resourceEnum, maxPickupAmount).ToString());
			return true;
		}
	}

	
	//for (int32 i = 1; i <= house.houseLvl(); i++)
	//{
	//	const std::vector<ResourceEnum>& resourceEnums = HouseLvlToUpgradeNeeds[i];
	//	for (ResourceEnum resourceEnum : resourceEnums) 
	//	{
	//		if (house.GetResourceCountWithPush(resourceEnum) < 5 &&
	//			TryMoveResourcesAnyProviderToDropoff(ResourceFindType::AvailableForPickup, house.GetFoundHolderInfo(resourceEnum, maxPickupAmount)))
	//		{
	//			AddDebugSpeech("(Succeed)TryFillLuxuries: " + house.GetFoundHolderInfo(resourceEnum, maxPickupAmount).ToString());
	//			return true;
	//		}
	//	}
	//}

	AddDebugSpeech("(Failed)TryFillLuxuries");
	return false;
}

bool HumanStateAI::TryFun()
{
	if (_houseId == -1) {
		AddDebugSpeech("(Failed)TryFun no house");
		return false;
	}

	const int32 funPercentToGoToTavern = 70;

	if (_funTicks < FunTicksAt100Percent * funPercentToGoToTavern / 100)
	{
		House& house = _simulation->building<House>(_houseId, CardEnum::House);
		WorldTile2 houseCenter = house.centerTile();

		auto findAvailableFunBuilding = [&](CardEnum funBuildingEnum, int32& nearestFunBuildingId)
		{
			std::vector<int32> funBuildingIds = _simulation->buildingIds(_playerId, funBuildingEnum);
			int32 nearestDist = numeric_limits<int32>::max();
			for (int32 funBuildingId : funBuildingIds) {
				FunBuilding& funBuilding = _simulation->building(funBuildingId).subclass<FunBuilding>(funBuildingEnum);
				if (funBuilding.isUsable()) {
					// Get service nearest to house
					int32 dist = WorldTile2::Distance(houseCenter, funBuilding.centerTile());
					if (dist < nearestDist) {
						nearestDist = dist;
						nearestFunBuildingId = funBuildingId;
					}
				}
			}
		};

		// Find available fun building in range of home
		int32 nearestFunBuildingId = -1;
		
		// if House is lvl 3+, try to find theatre first;
		if (house.houseLvl() >= 3) {
			findAvailableFunBuilding(CardEnum::Theatre, nearestFunBuildingId);
		}

		// if theatre isn't available, try tavern...
		if (nearestFunBuildingId == -1) {
			findAvailableFunBuilding(CardEnum::Tavern, nearestFunBuildingId);
		}

		if (nearestFunBuildingId != -1) 
		{
			Building& funBuilding = _simulation->building(nearestFunBuildingId);
			
			Add_HaveFun(nearestFunBuildingId);
			Add_Wait(Time::TicksPerSecond * 8);
			Add_MoveTo(funBuilding.gateTile());
			
			_unitState = UnitState::GetFun;

			AddDebugSpeech("(Succeed)TryFun theatre");
			return true;
		}
	}

	AddDebugSpeech("(Failed)TryFun ... Fun not low " + to_string(funPercent()));
	return false;
}

bool HumanStateAI::TryGatherFruit()
{
	PUN_UNIT_CHECK(workplace()->isEnum(CardEnum::FruitGatherer));
	ResourceHolderInfo infoOrange = workplace()->holderInfo(ResourceEnum::Orange);
	PUN_UNIT_CHECK(infoOrange.isValid());

	ResourceHolderInfo infoPapaya = workplace()->holderInfo(ResourceEnum::Papaya);
	PUN_UNIT_CHECK(infoPapaya.isValid());
	
	int resourceCount = resourceSystem().resourceCountWithPush(infoOrange) + resourceSystem().resourceCountWithPush(infoPapaya);
	if (resourceCount > GameConstants::WorkerEmptyBuildingInventoryAmount) {
		AddDebugSpeech("(Failed)TryGatherBerry: resourceCount > " + to_string(GameConstants::WorkerEmptyBuildingInventoryAmount));
		return false;
	}

	NonWalkableTileAccessInfo treeAccessInfo = treeSystem().FindNearestUnreservedFruitTree(workplace()->centerTile(), unitTile(), GathererHut::Radius, GameConstants::MaxFloodDistance_Human,
																						IsIntelligentUnit(unitEnum()));
	if (!treeAccessInfo.isValid()) {
		AddDebugSpeech("(Failed)TryGatherBerry: treeAccessInfo invalid");
		return false;
	}

	{
		// reserve 0 work amount, just to make sure _workReservers list has the person. This is useful when going through the list to ResetActions()
		ReserveWork(0);
		ReserveTreeTile(treeAccessInfo.tile.tileId());
	}

	TileObjEnum tileObjEnum = treeSystem().tileObjEnum(treeAccessInfo.tile.tileId());

	int32 gatherTicks = Time::TicksPerSecond * 7;
	if (tileObjEnum == TileObjEnum::Papaya) { // Jungle x2 to get papaya
		gatherTicks *= 2;
	}
	
	gatherTicks = gatherTicks * 100 / workEfficiency100();

	if (workplace()->workMode().name == "Meticulous") {
		gatherTicks *= 2;
	}

	// Doesn't have to check workplace accessibility, if StoreGatheredAtWorkplace failed, ResetActions() will find storage anyway
	Add_StoreGatheredAtWorkplace();
	Add_GatherFruit(treeAccessInfo.tile);
	Add_Wait(gatherTicks);
	Add_MoveToward(treeAccessInfo.tile.worldAtom2(), 5000);
	Add_MoveTo(treeAccessInfo.nearbyTile);

	_unitState = UnitState::GatherBerry;
	AddDebugSpeech("(Succeed)TryGatherBerry: pushed actions");
	return true;
}

bool HumanStateAI::TryHunt()
{
	HuntingLodge& huntingLodge = workplace()->subclass<HuntingLodge>(CardEnum::HuntingLodge);
	
	ResourceHolderInfo info = huntingLodge.holderInfo(ResourceEnum::Pork);
	PUN_CHECK2(info.isValid(), debugStr());
		
	int resourceCount = resourceSystem().resourceCountWithPush(info);
	if (resourceCount > GameConstants::WorkerEmptyBuildingInventoryAmount) {
		AddDebugSpeech("(Failed)TryHunt: resourceCount > " + to_string(GameConstants::WorkerEmptyBuildingInventoryAmount));
		// TODO:, this worker should actually be the one taking inventory to storage...
		return false;
	}

	auto& unitList = _unitData->unitSubregionLists();

	// Get nearby regions
	WorldTile2 huntingLodgeTile = huntingLodge.centerTile();
	WorldRegion2 middleRegion = huntingLodgeTile.region();
	int32 nearestUnit = -1;
	int32 nearestDist = INT32_MAX;
	WorldTile2 nearestTile = WorldTile2::Invalid;

	auto isHuntableAnimal = [&](int32 unitId) {
		UnitEnum unitEnum = _unitData->unitEnum(unitId);
		return IsWildAnimal(unitEnum) || (IsDomesticatedAnimal(unitEnum) && _simulation->unitAI(unitId).houseId() == -1); // Wild animals or domesticated animals without home
	};

	//PUN_LOG("ExecuteRegionStart ticks:%d regionId:%d", TimeDisplay::Ticks(), regionId)
	for (int y = middleRegion.y - 1; y <= middleRegion.y + 1; y++) {
		for (int x = middleRegion.x - 1; x <= middleRegion.x + 1; x++) {
			WorldRegion2 curRegion(x, y);
			if (curRegion.IsValid()) {
				unitList.ExecuteRegion(curRegion, [&](int32 unitId)
				{
					
					if (_unitData->aliveUnsafe(unitId) &&
						isHuntableAnimal(unitId) &&
						_simulation->unitAI(unitId).playerId() != _playerId)
					{
						WorldTile2 curTile = _unitData->atomLocation(unitId).worldTile2();
						int32_t dist = WorldTile2::ManDistance(huntingLodgeTile, curTile);
						if (dist < HuntingLodge::Radius && dist < nearestDist) {
							nearestDist = dist;
							nearestUnit = unitId;
							nearestTile = curTile;
						}
					}
				});
			}
		}
	}

	if (nearestUnit == -1) {
		AddDebugSpeech("(Failed)TryHunt: no valid animal nearby");
		return false;
	}

	// reserve 0 work amount, just to make sure _workReservers list has the person. This is useful when going through the list to ResetActions()
	ReserveWork(0);

	// Doesn't have to check workplace accessibility, if StoreGatheredAtWorkplace failed, ResetActions() will find storage anyway

	UnitFullId targetFullId = _unitData->fullId(nearestUnit);

	int32 damage = 15;
	damage = damage * workEfficiency100() / 100;

	if (workplace()->workMode().name == "Poison Arrows") {
		damage *= 4;
	}
	
	Add_AttackOutgoing(targetFullId, damage);
	Add_MoveInRange(nearestTile, damage);

	_unitState = UnitState::Hunt;
	AddDebugSpeech("(Succeed)TryHunt: pushed actions");
	return true;
}

bool HumanStateAI::TryRanch()
{
	Ranch& ranch = workplace()->subclass<Ranch>();

	if (ranch.animalOccupants().size() == 0) {
		AddDebugSpeech("(Failed)TryRanch: no animal");
		return false;
	}

	// TODO: don't do work when resource is full, but then also
	

	// Check if ranch is almost full, if so, start slaugthering animals...
	if (ranch.workMode().name == "Kill when above full capacity")
	{
		if (ranch.openAnimalSlots() > 1) { // Note > 1 (more balance)
			AddDebugSpeech("(Failed)TryRanch: still growing animals");
			return false;
		}
	}
	else if (ranch.workMode().name == "Kill when above half capacity")
	{
		if (ranch.openAnimalSlots() > ranch.maxAnimals / 2) {
			AddDebugSpeech("(Failed)TryRanch: still growing animals");
			return false;
		}
	}

	// Kill oldest animal...
	UnitFullId targetFullId;
	int32 maxAge = 0;
	const auto& animalOccupants = ranch.animalOccupants();
	for (size_t i = 0; i < animalOccupants.size(); i++) {
		int32 animalId = animalOccupants[i];
		int32 age = _simulation->unitAI(animalId).age();
		if (age >= maxAge) {
			targetFullId = _unitData->fullId(animalId);
			maxAge = age;
		}
	}

	PUN_CHECK2(targetFullId.isValid(), debugStr());

	ReserveWork(0);

	Add_AttackOutgoing(targetFullId, 100);
	Add_MoveTo(_unitData->atomLocation(targetFullId.id).worldTile2());


	auto& unitAI = _simulation->unitAI(targetFullId.id);;
	PUN_LOG("Try Ranch on unit id:%d pid:%d houseId:%d unit:%s", targetFullId.id, unitAI.playerId(), unitAI.houseId(), ToTChar(unitAI.GetUnitName()));
	
	return false;
}

bool HumanStateAI::TryFarm()
{
	Building& workplc = *workplace();
	PUN_CHECK2(workplc.isEnum(CardEnum::Farm), debugStr());
	Farm& farm = workplc.subclass<Farm>();

	//workplc.workplaceNeedInput = false;

	if (Time::IsSnowing()) {
		AddDebugSpeech("(Failed)TryFarm: Snowing already");
		return false;
	}

	// seed until done ... then nourish until autumn, or until fruit (for fruit bearers)
	if (farm.IsStage(FarmStage::Dormant))
	{
		AddDebugSpeech("(Failed)TryFarm: Dormant");
		return false;
	}
	//// Force dormancy for winter if not yet dormant
	//else if (Time::IsWinter()) {
	//	farm.ClearAllPlants();
	//	farm.ResetStageTo(FarmStage::Dormant);
	//	AddDebugSpeech("(Failed)TryFarm: Set to Dormant");
	//	return false;
	//}

	// TODO: also have clearing stage..

	if (farm.IsStage(FarmStage::Seeding))
	{
		WorldTile2 seedTile = farm.FindFarmableTile(_id);

		if (seedTile.isValid()) {

			//if ((farm.currentPlantEnum == TileObjEnum::WheatBush && resourceSystem().wheatSeeds <= 0) ||
			//	(farm.currentPlantEnum == TileObjEnum::GrassGreen && resourceSystem().grassSeeds <= 0) || 
			//	(farm.currentPlantEnum == TileObjEnum::Grapevines && resourceSystem().grapeSeeds <= 0))
			//{
			//	workplc.workplaceNeedInput = true;
			//	AddDebugSpeech("(Failed)TryFarm: No seed");
			//	return false;
			//}

			ReserveWork(0);
			ReserveTreeTile(seedTile.tileId());
			ReserveFarmTile(seedTile.tileId());

			int32_t waitTicks = 60;
			Add_DoFarmWork(seedTile, FarmStage::Seeding);
			Add_Wait(waitTicks, UnitAnimationEnum::FarmPlanting);
			Add_MoveTo(seedTile);

			_unitState = UnitState::Farm;
			AddDebugSpeech("(Succeed)TryFarm: Seeding");
			return true;
		}
		else {
			// Check if we can move on to next stage
			if (farm.IsStageCompleted()) {
				farm.ResetStageTo(FarmStage::Nourishing);
			}
			else {
				AddDebugSpeech("(Failed)TryFarm: Waiting for another person to seed");
				return false;
			}
		}
	}
	
	if (farm.IsStage(FarmStage::Nourishing))
	{
		if (Time::IsAutumn() || farm.MinCropGrowthPercent() >= 100)
		{
			farm.ResetStageTo(FarmStage::Harvesting);
			AddDebugSpeech("(_)TryFarm: Switch to harvesting");
		}
		else
		{
			WorldTile2 nourishTile = farm.FindFarmableTile(_id);

			if (nourishTile.isValid()) {
				ReserveWork(0);
				ReserveTreeTile(nourishTile.tileId());
				ReserveFarmTile(nourishTile.tileId());

				int32_t waitTicks = 60;
				Add_DoFarmWork(nourishTile, FarmStage::Nourishing);
				Add_Wait(waitTicks);
				Add_MoveTo(nourishTile);

				_unitState = UnitState::Farm;
				AddDebugSpeech("(Succeed)TryFarm: Nourishing");
				return true;
			}
			else {
				// Check if we can move on to next stage
				if (farm.IsStageCompleted()) {
					farm.ResetStageTo(FarmStage::Nourishing); // Reset back to nourishing... harvest happens in autumn
					AddDebugSpeech("(Failed)TryFarm: Reset to more nourishing...");
					return false;
				}
				AddDebugSpeech("(Failed)TryFarm: Waiting for another person to nourish");
				return false;
			}
		}
	}

	if (farm.IsStage(FarmStage::Harvesting))
	{
		auto& resourceSys = resourceSystem();
		std::vector<DropInfo> drops = resourceSys.GetDropsFromArea_Pickable(farm.area(), true);
		
		if (drops.size() > 3)
		{
			int32 targetAmount = 10;
			int32 amount = 0;
			ResourceEnum targetResourceEnum = ResourceEnum::None;
			std::vector<DropInfo> dropsToMove;
			for (DropInfo drop : drops) 
			{
				if (targetResourceEnum == ResourceEnum::None) {
					targetResourceEnum = drop.holderInfo.resourceEnum;
				}

				int32 dropResourceCount = resourceSys.resourceCountWithPop(drop.holderInfo);
				if (drop.holderInfo.resourceEnum == targetResourceEnum &&
					amount + dropResourceCount <= targetAmount) 
				{
					amount += dropResourceCount;
					dropsToMove.push_back(drop);
				} else {
					break;
				}
			}
			
			// Try dropping the resource off in proper store
			FoundResourceHolderInfos foundDropoffs = resourceSystem().FindHolder(ResourceFindType::AvailableForDropoff, targetResourceEnum, amount, unitTile());
			if (foundDropoffs.hasInfos())
			{
				for (FoundResourceHolderInfo foundInfo : foundDropoffs.foundInfos) {
					// Reserve
					ReserveResource(ReservationType::Push, foundInfo.info, foundInfo.amount);

					// Set Actions
					Add_DropoffResource(foundInfo.info, foundInfo.amount);
					Add_MoveToResource(foundInfo.info);
					//Add_MoveTo(foundInfo.tile);
				}

				for (DropInfo drop : dropsToMove) {
					int32 dropResourceCount = resourceSys.resourceCountWithPop(drop.holderInfo);
					
					// Reserve
					ReserveResource(ReservationType::Pop, drop.holderInfo, dropResourceCount);

					// Set Actions
					Add_PickupResource(drop.holderInfo, dropResourceCount);
					Add_MoveToResource(drop.holderInfo);
				}

				_unitState = UnitState::Farm;

				AddDebugSpeech("(Succeed)TryFarm: RemoveDrop from farm");
				return true;
			}
		}
		
		WorldTile2 harvestTile = farm.FindFarmableTile(_id);

		if (harvestTile.isValid()) {
			ReserveWork(0);
			ReserveTreeTile(harvestTile.tileId());
			ReserveFarmTile(harvestTile.tileId());

			int32_t waitTicks = 60;
			Add_DoFarmWork(harvestTile, FarmStage::Harvesting);
			Add_Wait(waitTicks);
			Add_MoveTo(harvestTile);

			_unitState = UnitState::Farm;
			AddDebugSpeech("(Succeed)TryFarm: Nourishing");
			return true;
		}
		else {
			// Check if we can move on to next stage
			if (farm.IsStageCompleted()) {
				farm.ResetStageTo(FarmStage::Dormant);
			}
			else {
				AddDebugSpeech("(Failed)TryFarm: Waiting for another person to nourish");
				return false;
			}
		}
		return true;
	}

	UE_DEBUG_BREAK();
	return false;
}

//bool HumanStateAI::TryConsumerWork()
//{
//	Building& workplc = *workplace();
//	//BldInfo info = workplc.buildingInfo();
//
//	if (!workplc.NeedWork()) {
//		AddDebugSpeech("(Failed)TryConsumerWork: !NeedWork " + workplc.buildingInfo().name);
//		return false;
//	}
//
//	workplc.workplaceInputNeeded = ResourceEnum::None;
//
//	// TODO: 2 inputs too??
//	PUN_UNIT_CHECK(workplc.hasInput1());
//	ResourceHolderInfo holderInfo = workplc.holderInfo(workplc.input1());
//	PUN_UNIT_CHECK(resourceSystem().resourceCountWithPop(holderInfo) >= 0);
//	bool needInput1 = resourceSystem().resourceCountWithPop(holderInfo) == 0;
//	if (needInput1) {
//		if (TryFillWorkplace(workplc.input1())) {
//			AddDebugSpeech("(Success)TryConsumerWork: filling inputs");
//			return true;
//		}
//		workplc.workplaceInputNeeded = workplc.input1();
//		AddDebugSpeech("(Failed)TryConsumerWork: no inputs, can't find inputs");
//		return false;
//	}
//	PUN_UNIT_CHECK(resourceSystem().resourceCountWithPop(holderInfo) > 0);
//
//	int32 workManSec100 = 100;
//	int32 waitTicks = 60;
//	int32 resourceAmount = (workplc.subclass<ConsumerWorkplace>().resourceConsumptionAmount100(workManSec100) + 99) / 100; // round up resourceConsumptionAmount100, reserve more than needed...
//	ReserveWork(workManSec100);
//	ReserveResource(ReservationType::Pop, holderInfo, resourceAmount);
//
//
//	PUN_LOG("TryConsumerWork reserveWork: %d withPush:%d , withPop:%d", workplc.resourceCount(workplc.input1()),
//		workplc.GetResourceCountWithPush(workplc.input1()), workplc.GetResourceCountWithPop(workplc.input1()));
//
//	Add_DoConsumerWork(workManSec100);
//	Add_Wait(waitTicks);
//	Add_MoveTo(workplc.gateTile());
//
//	_unitState = UnitState::WorkConsume;
//	AddDebugSpeech("(Success)TryConsumerWork: researching");
//	return true;
//}

bool HumanStateAI::TryGather(bool treeOnly)
{
	SCOPE_CYCLE_COUNTER(STAT_PunUnit_CalcHuman_TryGather);
	
	if (isBelowWorkingAge()) {
		return false;
	}
	
	// Filter for trees to cut
	NonWalkableTileAccessInfo tileAccessInfo;
	{
		SCOPE_CYCLE_COUNTER(STAT_PunUnit_CalcHuman_TryGather_FindMark);
		tileAccessInfo = treeSystem().FindNearestMark(playerId(), unitTile(), treeOnly);
	}

	if (!tileAccessInfo.isValid()) {
		AddDebugSpeech("(Failed)TryGather: no tree nearby");
		return false;
	}

	SCOPE_CYCLE_COUNTER(STAT_PunUnit_CalcHuman_TryGather_GatherSequence);
	
	GatherSequence(tileAccessInfo);

	bool isTreeTile = treeSystem().tileInfo(tileAccessInfo.tile.tileId()).type == ResourceTileType::Tree;
	_unitState = isTreeTile ? UnitState::GatherTree : UnitState::GatherStone;
	
	AddDebugSpeech("(Succeed)TryGather: pushed actions");
	return true;
}

bool HumanStateAI::TryForestingCut(bool cutAndPlant)
{
	Building* workPlc = workplace();

	NonWalkableTileAccessInfo tileAccessInfo = treeSystem().FindCuttableTree(_playerId, workPlc->gateTile(), Forester::Radius);

	if (!tileAccessInfo.isValid()) {
		AddDebugSpeech("(Failed)TryForestingCut: no valid tree");
		return false;
	}

	// Set Actions (Harvest/replant)
	WorldTile2 tile = tileAccessInfo.tile;
	if (cutAndPlant) {
		Add_TryForestingPlantAction(treeSystem().tileObjEnum(tile.tileId()), tileAccessInfo);
		Add_DropInventoryAction();
	}
	GatherSequence(tileAccessInfo);

	_unitState = UnitState::ForestingCut;
	AddDebugSpeech("(Success)TryForestingCut:");
	return true;
}
bool HumanStateAI::TryForestingNourish()
{
	Building* workPlc = workplace();
	
	NonWalkableTileAccessInfo tileAccessInfo = treeSystem().FindNourishableTree(_playerId, workPlc->gateTile(), Forester::Radius);

	if (!tileAccessInfo.isValid()) {
		AddDebugSpeech("(Failed)TryForestingNourish: no valid tree");
		return false;
	}
	int32 nourishTicks = Time::TicksPerSecond * 5;
	nourishTicks  = nourishTicks * 100 / workEfficiency100();
	
	Add_NourishTree(tileAccessInfo.tile);
	Add_Wait(nourishTicks);
	Add_MoveToward(tileAccessInfo.tile.worldAtom2(), 5000);
	Add_MoveTo(tileAccessInfo.nearbyTile);

	_unitState = UnitState::ForestingCut;
	AddDebugSpeech("(Success)TryForestingNourish:");
	return true;
}

void HumanStateAI::Add_TryForestingPlantAction(TileObjEnum tileObjEnum, NonWalkableTileAccessInfo accessInfo) {
	AddAction(ActionEnum::TryForestingPlantAction, static_cast<int32>(tileObjEnum), accessInfo.tile.tileId(), accessInfo.nearbyTile.tileId());
}
void HumanStateAI::TryForestingPlantAction()
{
	TileObjEnum tileObjEnum = static_cast<TileObjEnum>(action().int32val1);
	NonWalkableTileAccessInfo accessInfo(WorldTile2(action().int32val2), WorldTile2(action().int32val3));
	
	NextAction(UnitUpdateCallerEnum::TryForestingPlantAction_Done);
	AddDebugSpeech("TryForestingPlantAction_Done");

	TryForestingPlant(tileObjEnum, accessInfo);
}
bool HumanStateAI::TryForestingPlant(TileObjEnum tileObjEnum, NonWalkableTileAccessInfo accessInfo)
{	
	if (!accessInfo.isValid()) {
		// Find a valid tile to plant tree...
		Building* forester = workplace();
		PUN_UNIT_CHECK(forester);
		
		accessInfo = treeSystem().FindTreePlantableSpot(_playerId, forester->gateTile(), Forester::Radius);
		if (!accessInfo.isValid()) {
			AddDebugSpeech("(Failed)TryForestingPlant: No TreePlantableSpot.");
			return false;
		}
	}
	
	int32 plantTicks = playerParameters().CutTreeTicks();
	plantTicks = plantTicks * 100 / workEfficiency100();

	WorldTile2 tile = accessInfo.tile;

	if (tileObjEnum == TileObjEnum::None) {
		BiomeInfo biomeInfo = GetBiomeInfo(_simulation->terrainGenerator().GetBiome(tile));
		tileObjEnum = biomeInfo.GetRandomTreeEnum();
	}

	// TODO: this is weird, eventually should find the real reason behind this...
	if (treeSystem().IsReserved(tile.tileId())) {
		PUN_LOG("Weird: tree already reserved");
		return false;
	}
	
	ReserveTreeTile(tile.tileId());
	
	// Reserve around so that people won't plant too close to each other.
	auto tryReserveTreeTile = [&](WorldTile2 treeTile) {
		if (!treeSystem().IsReserved(treeTile.tileId())) {
			ReserveTreeTile(treeTile.tileId());
		}
	};
	
	tryReserveTreeTile(WorldTile2(tile.x + 1, tile.y + 1));
	tryReserveTreeTile(WorldTile2(tile.x + 1, tile.y));
	tryReserveTreeTile(WorldTile2(tile.x + 1, tile.y - 1));

	tryReserveTreeTile(WorldTile2(tile.x, tile.y + 1));
	tryReserveTreeTile(WorldTile2(tile.x, tile.y - 1));
	
	tryReserveTreeTile(WorldTile2(tile.x - 1, tile.y + 1));
	tryReserveTreeTile(WorldTile2(tile.x - 1, tile.y));
	tryReserveTreeTile(WorldTile2(tile.x - 1, tile.y - 1));
	

	Add_PlantTree(tile, tileObjEnum);
	Add_Wait(plantTicks, UnitAnimationEnum::FarmPlanting);
	Add_MoveToward(tile.worldAtom2(), 5000);
	Add_MoveTo(accessInfo.nearbyTile);

	_unitState = UnitState::ForestingPlant;
	return true;
}

bool HumanStateAI::TryForesting()
{
	Forester& forester = workplace()->subclass<Forester>(CardEnum::Forester);
	const std::string& workModeName = forester.workMode().name;
	
	if (workModeName == "Cut and Plant") {
		if (TryForestingCut(true)) {
			return true;
		}
		if (TryForestingPlant(TileObjEnum::None)) {
			return true;
		}
		return TryForestingNourish();
	}
	
	if (workModeName == "Prioritize Planting")
	{
		if (TryForestingPlant(TileObjEnum::None)) {
			return true;
		}
		if (TryForestingCut(true)) {
			return true;
		}
		return TryForestingNourish();
	}

	if (workModeName == "Prioritize Cutting")
	{
		if (TryForestingCut(false)) {
			return true;
		}
		if (TryForestingPlant(TileObjEnum::None)) {
			return true;
		}
		return TryForestingNourish();
	}
	
	UE_DEBUG_BREAK();
	return false;
}

bool HumanStateAI::TryFillWorkplace(ResourceEnum resourceEnum)
{
	Building* workPlc = workplace();
	const int32 maxAmount = 10;

	check(IsProducer(workplace()->buildingEnum()) || IsConsumerOnlyWorkplace(workplace()->buildingEnum()));
	FoundResourceHolderInfo dropoffInfo = workPlc->GetFoundHolderInfo(resourceEnum, maxAmount);
	check(dropoffInfo.isValid());
	//UE_LOG(LogTemp, Error, TEXT("!TryFillWorkplace: %s, %s"), *FString(workplace.buildingInfo().name.c_str()), *FString(ResourceName(resourceEnum).c_str()));

	if (!resourceSystem().needResource(dropoffInfo.info)) {
		AddDebugSpeech("TryFillWorkplace(Done): " + workPlc->debugStr() +" no longer need resource");
		return false;
	}

	if (TryMoveResourcesAnyProviderToDropoff(ResourceFindType::AvailableForPickup, dropoffInfo)) {
		AddDebugSpeech("TryFillWorkplace(Done): move resource");
		return true;
	}

	return false;
}

bool HumanStateAI::TryProduce()
{
	Building& workplace = _simulation->building(_workplaceId);
	PUN_UNIT_CHECK(IsProducer(workplace.buildingEnum()) ||
					IsSpecialProducer(workplace.buildingEnum()));

	workplace.workplaceInputNeeded = ResourceEnum::None;

	// Shield against Non connected workplace
	// TODO: just kick the person out??
	if (!IsMoveValid(workplace.gateTile())) {
		AddDebugSpeech("(Failed)TryProduce: Workplace gate-move not valid");
		return false;
	}

	//ResourceHolderInfo info = workplace.holderInfo(workplace.product());
	//PUN_CHECK2(info.isValid(), debugStr());

	if (!workplace.NeedWork()) {
		if (IsSpecialProducer(workplace.buildingEnum())) {
			AddDebugSpeech("(Failed)TryProduce: !workplace.NeedWork, SpecialProducer");
			return true;
		}
		
		auto holderInfo = workplace.holderInfo(workplace.product());
		int32 amount = min(10, resourceSystem().resourceCountWithPop(holderInfo)); // Don't move more than 10 resources at a time
		if (amount == 0) {
			AddDebugSpeech("(Failed)TryProduce: !workplace.NeedWork, but already enough Pop");
			return false;
		}

		if (TryMoveResourcesProviderToAnyDropoff(FoundResourceHolderInfo(holderInfo, amount, workplace.gateTile()), ResourceFindType::AvailableForDropoff)) {
			AddDebugSpeech("(Succeed)TryProduce: !workplace.NeedWork, move instead");
			return true;
		}
		AddDebugSpeech("(Failed)TryProduce: !workplace.NeedWork, Failed to find storage");
		return false;
	}

	if (!workplace.filledInputs())
	{
		// If there is nothing at the workplace, we need to go out to grab the resource to fill it
		ResourceSystem& resourceSystem = _simulation->resourceSystem(_playerId);

		int32 inputPerBatch = workplace.inputPerBatch();

		//bool needInput1 = false;
		//bool needInput2 = false;

		//if (workplace.hasInput1()) {
		//	int32 resourceCountWithPop1 = resourceSystem.resourceCountWithPop(workplace.holderInfo(workplace.input1()));
		//	needInput1 = resourceCountWithPop1 < inputPerBatch;
		//}
		//if (workplace.hasInput2()) {
		//	int32 resourceCountWithPop2 = resourceSystem.resourceCountWithPop(workplace.holderInfo(workplace.input2()));
		//	needInput2 = resourceCountWithPop2 < inputPerBatch;
		//}
		
		bool needInput1 = workplace.needInput1();
		bool needInput2 = workplace.needInput2();
		
		if (needInput1 &&
			TryFillWorkplace(workplace.input1()))
		{
			AddDebugSpeech("(Succeed)TryProduce: TryFillWorkplace input1");
			return true;
		}
		if (needInput2 && 
			TryFillWorkplace(workplace.input2()))
		{
			AddDebugSpeech("(Succeed)TryProduce: TryFillWorkplace input2");
			return true;
		}
		// Need input, but TryFillWorkplace failed
		if (needInput1 || needInput2) {
			
			// Check resourceCountWithPush, we would display needInput icon in the case where we need input and no one is trying to deliver them
			bool needInput1_WithPush = workplace.hasInput1() && resourceSystem.resourceCountWithPush(workplace.holderInfo(workplace.input1())) < inputPerBatch;
			bool needInput2_WithPush = workplace.hasInput2() && resourceSystem.resourceCountWithPush(workplace.holderInfo(workplace.input2())) < inputPerBatch;
			if (needInput1_WithPush) {
				workplace.workplaceInputNeeded = workplace.input1();
			}
			else if (needInput2_WithPush) {
				workplace.workplaceInputNeeded = workplace.input2();
			}
			
			AddDebugSpeech("(Failed)TryProduce: NeedInput but can't get");
			return false;
		}

		// Reserve fill the input
		{
			if (workplace.hasInput1()) {
				ReserveResource(ReservationType::Pop, workplace.holderInfo(workplace.input1()), inputPerBatch);
			}
			if (workplace.hasInput2()) {
				ReserveResource(ReservationType::Pop, workplace.holderInfo(workplace.input2()), inputPerBatch);
			}
			ReserveWork(100); // Reserve work 100 to disallow others from using this building
		}

		// Go fill the input
		Add_FillInputs(workplace.buildingId());
		Add_MoveTo(workplace.gateTile());

		_unitState = UnitState::FillInput;
		AddDebugSpeech("TryProduce: Succeed -- fillInputs actions");
		return true; // No action available to satisfy inputs
	}

	int32 workManSec100 = 100;

	// work efficiency
	workManSec100 = workManSec100 * workEfficiency100() / 100;

	int waitTicks = 60;
	ReserveWork(workManSec100);

	Add_Produce(workManSec100, waitTicks, 5, workplace.buildingId());
	Add_MoveTo(workplace.gateTile());

	_unitState = UnitState::WorkProduce;
	AddDebugSpeech("TryProduce(Done): -- work actions");
	return true;
}

bool HumanStateAI::TryConstructRoad()
{
	const std::vector<int32>& roadConstructionIds = _simulation->playerOwned(_playerId).roadConstructionIds();

	const int32 maxLoop = 1000;
	int32 nearestRoadId = -1;
	int32 nearestRoadDistance = INT32_MAX;

	WorldTile2 tile = unitTile();

	/*
	 * Get the nearest road construction
	 */
	for (size_t i = 0; i < roadConstructionIds.size(); i++)
	{	
		Building& road = _simulation->building(roadConstructionIds[i]);
		if (road.workReservers().size() == 0)
		{
			int32 distance = WorldTile2::ManDistance(tile, road.centerTile());
			if (distance < nearestRoadDistance) {
				nearestRoadId = roadConstructionIds[i];
				nearestRoadDistance = distance;
			}
		}
		// TODO: this looping could be slow???
	}

	if (nearestRoadId != -1) 
	{
		// Continuously find adjacent road tiles to create a chain
		const std::vector<WorldTile2> shifts
		{
			WorldTile2(0, 1),
			WorldTile2(0, -1),
			WorldTile2(1, 0),
			WorldTile2(-1, 0),
		};

		auto& overlaySys = _simulation->overlaySystem();

		auto getRoadId = [&](WorldTile2 newTile) {
			if (!newTile.isValid()) {
				return -1;
			}

			RoadTile roadTile = overlaySys.GetRoad(newTile);
			if (roadTile.isValid() && !roadTile.isConstructed) {
				PUN_UNIT_CHECK(roadTile.buildingId != -1);
				return roadTile.buildingId;
			}

			return -1;
		};

		// Since we are pushing actions, the first tile will get processed last
		// This needs rearrange
		int32 workConstructQueued = 0;
		auto rearrangeActions = [&]()
		{
			std::vector<Action> actions = _actions;
			_actions.clear();
			size_t index = 0;
			for (int32 i = 0; i < workConstructQueued; i++) {
				_actions.insert(_actions.begin(), actions[index + 1]);
				_actions.insert(_actions.begin(), actions[index]);
				index += 2;
			}
			// The rest of the actions goes to the front
			_actions.insert(_actions.begin(), _actions.begin() + index, _actions.end());
		};

		int32 roadQueued = 0;
		WorldTile2 curTile = _simulation->building(nearestRoadId).centerTile();

		// Insert the initial tiles
		{
			int32 roadId = getRoadId(curTile);
			if (roadId != -1) {
				// Try to construct this road...
				if (TryConstructHelper(roadId)) {
					if (_unitState == UnitState::WorkConstruct) {
						//_simulation->DrawLine(curTile.worldAtom2(), FVector::ZeroVector, curTile.worldAtom2(), FVector::ZeroVector, FLinearColor::White);
						workConstructQueued++;
					} else {
						//_simulation->DrawLine(curTile.worldAtom2(), FVector::ZeroVector, curTile.worldAtom2(), FVector::ZeroVector, FLinearColor::Red);
						DEBUG_AI_VAR(TryConstructRoad_NotWorkConstructState);
						return true; // Only not WorkConstruct, end Try and just do this one task...
					}
				}
			}
		}

		LOOP_CHECK_START();
		while (roadQueued < 5)
		{
			LOOP_CHECK_END();
			
			bool gotChainTile = false;
			for (WorldTile2 shift : shifts) 
			{
				WorldTile2 newTile = curTile + shift;
				int32 roadId = getRoadId(newTile);
				if (roadId != -1)
				{
					// Try to construct this road...
					if (TryConstructHelper(roadId)) {
						if (_unitState == UnitState::WorkConstruct) {
							//_simulation->DrawLine(newTile.worldAtom2(), FVector::ZeroVector, newTile.worldAtom2(), FVector::ZeroVector, FLinearColor::Black);

							// Succeed in getting a chain tile.. break and find the next queue
							gotChainTile = true;
							curTile = newTile;
							roadQueued++;
							workConstructQueued++;
							break;
						}
						else {
							//_simulation->DrawLine(newTile.worldAtom2(), FVector::ZeroVector, newTile.worldAtom2(), FVector::ZeroVector, FLinearColor::Yellow);
							rearrangeActions();
							AddDebugSpeech("(Success)TryConstructRoad: Queued:" + to_string(roadQueued + 1));
							DEBUG_AI_VAR(TryConstructRoad_Queued);
							return true; // Only not WorkConstruct, end Try and just do this one task...
						}
					} else {
						//_simulation->DrawLine(newTile.worldAtom2(), FVector::ZeroVector, newTile.worldAtom2(), FVector::ZeroVector, FLinearColor(1, 0, 1));
					}
					
					// Failed to construct, continue trying...
				}
			}

			// No tile to chain with, break oout of this loop
			if (!gotChainTile) {
				break;
			}
		}

		// TODO: Sometimes we get no actions here, somehow?
		if (_actions.size() == 0) {
			AddDebugSpeech("(Failed)TryConstructRoad: 0 Action:" + to_string(roadQueued));
			DEBUG_AI_VAR(TryConstructRoad_0Action);
			return false;
		}

		AddDebugSpeech("(Success)TryConstructRoad: MaxQueued:" + to_string(roadQueued));
		rearrangeActions();

		DEBUG_AI_VAR(TryConstructRoad_MaxQueued);
		return true;
	}
	
	return false;
}

bool HumanStateAI::TryConstructHelper(int32 workplaceId)
{
	Building& workplace = _simulation->building(workplaceId);
	PUN_UNIT_CHECK(!workplace.isConstructed());

	workplace.workplaceInputNeeded = ResourceEnum::None;

	// Clear area
	if (!_simulation->IsLandCleared_SmallOnly(_playerId, workplace.area())) {
		if (TryClearLand(workplace.area())) {
			AddDebugSpeech("(Success)TryConstruct: Clearing land.");
			return true;
		}
		AddDebugSpeech("(Failed)TryConstruct: Failed to clear land.");
		return false;
	}
	if (!_simulation->IsLandCleared_SmallOnly(_playerId, workplace.frontArea())) {
		if (TryClearLand(workplace.frontArea())) {
			AddDebugSpeech("(Success)TryConstruct: Clearing land front.");
			return true;
		}
		AddDebugSpeech("(Failed)TryConstruct: Failed to clear land front.");
		return false;
	}

	// Land cleared and didn't SetWalkable yet.
	// Note: this must return false so that the Walkable can be reset, and we can calculate proper path.
	if (!workplace.didSetWalkable()) {
		workplace.SetAreaWalkable();
		AddDebugSpeech("(Success)TryConstruct: SetAreaWalkable");
		return false;
	}

	//// For farm/storage yard, just need to clear land
	//if (workplace.isEnum(CardEnum::StorageYard) || workplace.isEnum(CardEnum::Farm)) {
	//	workplace.FinishConstruction();
	//	AddDebugSpeech("(Success)TryConstruct: StorageYard/Farm");
	//	return false;
	//}

	WorldTile2 adjacentTile = workplace.adjacentTileNearestTo(unitTile());

	if (!adjacentTile.isValid()) {
		AddDebugSpeech("(Failed)TryConstruct: adjacentTile invalid");
		return false;
	}

	// Need Resource
	PUN_CHECK2(workplace.IsValidConstructionResourceHolders(), debugStr());
	if (!workplace.hasNeededConstructionResource())
	{
		// TODO: bug return..
		if (!workplace.IsValidConstructionResourceHolders()) {
			return false;
		}

		auto& resourceSystem = _simulation->resourceSystem(_playerId);
		std::vector<int32> constructionCosts = workplace.GetConstructionResourceCost();
		bool hasNeededResourceWithPush = true;

		for (int i = constructionCosts.size(); i-- > 0;)
		{
			if (constructionCosts[i] > 0)
			{
				ResourceEnum resourceEnum = ConstructionResources[i];
				int32 neededResource = constructionCosts[i] - workplace.GetResourceCountWithPush(resourceEnum);
				if (neededResource > 0) {
					int32 amount = min(neededResource, 10);
					FoundResourceHolderInfos foundProviders = resourceSystem.FindHolder(ResourceFindType::AvailableForPickup, resourceEnum, amount, unitTile());
					hasNeededResourceWithPush = false;

					if (foundProviders.hasInfos()) {
						MoveResourceSequence(foundProviders.foundInfos, { FoundResourceHolderInfo(workplace.holderInfo(resourceEnum), foundProviders.amount(), adjacentTile) });
						_unitState = UnitState::MoveResourceConstruct;
						AddDebugSpeech("(Succeed)TryConstruct: Move needed resource");
						return true;
					}
					else {
						AddDebugSpeech("(Try)TryConstruct: didn't find resource provider... " + ResourceName(resourceEnum));
						workplace.workplaceInputNeeded = resourceEnum;
					}
				}
				else {
					AddDebugSpeech("(Try)TryConstruct: don't need" + ResourceName(ConstructionResources[i]) + ", need" + to_string(neededResource));
				}
			}
		}

		if (hasNeededResourceWithPush) {
			AddDebugSpeech("(Failed)TryConstruct: already have resource with push. waiting for last pushes");
			return false;
		}

		//workplace.workplaceInputNeeded = true;
		AddDebugSpeech("(Failed)TryConstruct: need resource but can't get");
		return false;
	}

	// Need Construct
	if (!workplace.NeedConstruct()) {
		AddDebugSpeech("(Failed)TryConstruct: !workplace.NeedConstruct");
		return false;
	}

	int32 waitTicks = Time::TicksPerSecond / 4; //15
	int32 workManSec100 = 100;
	workManSec100 = workManSec100 * workEfficiency100() / 100;

	ReserveWork(workManSec100, workplaceId);
	

	_unitState = UnitState::WorkConstruct;
	Add_Construct(workManSec100, waitTicks, ConstructTimesPerBatch, workplaceId);
	Add_MoveToward(workplace.centerTile().worldAtom2(), 12000);
	Add_MoveTo(workplace.gateTile());

	AddDebugSpeech("(Succeed)TryConstruct: Succeed -- work actions");
	return true;
}

//void HumanStateAI::Add_DoConsumerWork(int32_t workManSec100) {
//	AddAction(ActionEnum::DoConsumerWork, workManSec100);
//}
//void HumanStateAI::DoConsumerWork()
//{
//	int32_t workManSec100 = action().int32val1;
//	
//	UnitReservation workplaceReservation = PopReservation(ReservationType::Workplace);
//	PUN_CHECK2(workplaceReservation.amount == workManSec100, debugStr());
//
//	Building& building = _simulation->building(workplaceReservation.reserveWorkplaceId);
//
//	// Remove resource
//	UnitReservation resourceReservation = PopReservation(ReservationType::Pop);
//	PUN_CHECK2(reservations.empty(), debugStr());
//
//	// 
//	int32 resourceConsumptionAmount100 = building.subclass<ConsumerWorkplace>().resourceConsumptionAmount100(workManSec100);
//	int32 resourceConsumptionAmount = GameRand::Rand100RoundTo1(resourceConsumptionAmount100);
//
//	PUN_CHECK2(resourceReservation.amount >= resourceConsumptionAmount, debugStr());
//
//	// May or may not remove resource (in case the unit is less than 1)
//	if (resourceConsumptionAmount > 0) {
//		ResourceEnum input1 = building.input1();
//
//		building.RemoveResource(input1, resourceConsumptionAmount);
//		PUN_LOG("DoConsumerWork RemoveResource: %d withPush:%d , withPop:%d", building.resourceCount(input1), building.GetResourceCountWithPush(input1), building.GetResourceCountWithPop(input1));
//
//		building.AddConsumption1Stat(ResourcePair(input1, resourceConsumptionAmount));
//	}
//
//	// Do research
//	PUN_CHECK2(IsConsumerOnlyWorkplace(building.buildingEnum()), debugStr());
//	ConsumerWorkplace* consumerWorkplace = static_cast<ConsumerWorkplace*> (&building);
//	consumerWorkplace->DoConsumerWork(workManSec100, resourceConsumptionAmount);
//
//	AddDebugSpeech("(Done)DoConsumerWork: bld:" + building.buildingInfo().name +" reservation:" + ReservationsToString());
//	NextAction("(Done)DoConsumerWork");
//}

void HumanStateAI::Add_AttackOutgoing(UnitFullId defender, int32 damage) {
	AddAction(ActionEnum::AttackOutgoing, damage, 0, 0, 0, defender);
}
void HumanStateAI::AttackOutgoing()
{
	UnitFullId defender = action().fullId1;
	int32 damage = action().int32val1;
	
	// TODO: move this to SearchAreaForDrop
	UnitReservation workplaceReservation = PopReservation(ReservationType::Workplace);

	if (!_simulation->unitAlive(defender)) {
		_simulation->ResetUnitActions(_id, 60);
		AddDebugSpeech("(Bad)AttackOutgoing: Unit already dead");
		return;
	}

	int32 arrowId = _simulation->AddUnit(UnitEnum::ProjectileArrow, _playerId, _unitData->atomLocation(_id), 0);
	ProjectileArrow* arrow = static_cast<ProjectileArrow*>(&_simulation->unitAI(arrowId));
	arrow->Launch(_fullId, workplaceReservation.reserveWorkplaceId, defender, damage);

	_simulation->soundInterface()->Spawn3DSound("CitizenAction", "BowShoot", unitAtom());

	int32 waitTicks = arrow->nextActiveTick() - Time::Ticks() + 90; // extra wait time

	//_actions.push_back(bind(&HumanStateAI::SearchAreaForDrop, this, ResourceEnum::Pork));
	Add_Wait(waitTicks);

	NextAction(UnitUpdateCallerEnum::AttackOutgoing_Done);
}

void HumanStateAI::Add_DoFarmWork(WorldTile2 tile, FarmStage farmStage) {
	AddAction(ActionEnum::DoFarmWork, tile.tileId(), static_cast<int32>(farmStage));
}
void HumanStateAI::DoFarmWork()
{
	WorldTile2 tile(action().int32val1);
	FarmStage farmStage = static_cast<FarmStage>(action().int32val2);
	
	UnitReservation reservation = PopReservation(ReservationType::TreeTile);
	PUN_CHECK2(reservation.reserveTileId == tile.tileId(), debugStr());

	UnitReservation workplaceReservation = PopReservation(ReservationType::Workplace);
	Building& building = _simulation->building(workplaceReservation.reserveWorkplaceId);
	PUN_CHECK2(building.isEnum(CardEnum::Farm), debugStr());
	Farm& farm = building.subclass<Farm>();

	UnitReservation farmTileReservation = PopReservation(ReservationType::FarmTile);
	PUN_CHECK2(farmTileReservation.reserveTileId == tile.tileId(), debugStr());

	farm.DoFarmWork(_id, tile, farmStage);

	AddDebugSpeech("(Done)DoFarmWork: bld:" + building.buildingInfo().name + " reservation:" + ReservationsToString());
	NextAction(UnitUpdateCallerEnum::DoFarmWork);
}


int32 HumanStateAI::housingHappiness()
{
	if (_houseId != -1) {
		return _simulation->building(_houseId).subclass<House>(CardEnum::House).housingHappiness();
	}
	return 0;
}
int32 HumanStateAI::luxuryHappinessModifier()
{
	if (_houseId != -1) {
		return _simulation->building(_houseId).subclass<House>(CardEnum::House).luxuryHappiness();
	}
	return 0;
}

int32 HumanStateAI::speedBoostEfficiency100() {
	auto& playerOwned = _simulation->playerOwned(_playerId);
	if (_workplaceId != -1 && playerOwned.HasSpeedBoost(_workplaceId)) {
		return playerOwned.IsInDarkAge() ? 100 : 50; // 50% speed boost... 100% if dark age
	}
	return 0;
}