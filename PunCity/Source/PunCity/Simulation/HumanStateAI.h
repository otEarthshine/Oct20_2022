// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "UnitStateAI.h"
#include "Resource/ResourceSystem.h"


class HumanStateAI : public UnitStateAI
{
public:
	void CalculateActions() final;

	//! Macros
	bool TryMoveResourcesProviderToDropoff(int32 providerBuildingId, int32 dropoffBuildingId, ResourceEnum resourceEnum, int32 amountAtLeast, UnitAnimationEnum animationEnum = UnitAnimationEnum::Walk);
	bool TryMoveResourcesAnyProviderToDropoff(ResourceFindType providerType, FoundResourceHolderInfo dropoffInfo, UnitAnimationEnum animationEnum = UnitAnimationEnum::Walk, int32 maxDistance = -1);

	bool TryMoveResourcesProviderToAnyDropoff(FoundResourceHolderInfo providerInfo, ResourceFindType dropoffType, UnitAnimationEnum animationEnum = UnitAnimationEnum::Walk);
	bool TryMoveResourcesAny(ResourceEnum resourceEnum, ResourceFindType providerType, ResourceFindType dropoffType, int32 amountAtLeast);

	bool TryMoveResourcesToDeliveryTarget(int32 deliverySourceId, ResourceEnum resourceEnum, int32 amountAtLeast);
	bool TryMoveResourcesToDeliveryTargetAll(int32 amountAtLeast);

	bool TryMoveResourceAny_AllTypes(ResourceInfo info, int32 amountAtLeast)
	{
		if (TryMoveResourcesAny(info.resourceEnum, ResourceFindType::AvailableForPickup, ResourceFindType::Requester, amountAtLeast) || justReset()) return true;
		
		if (TryMoveResourcesAny(info.resourceEnum, ResourceFindType::Provider, ResourceFindType::AvailableForDropoff, amountAtLeast) || justReset()) return true;

		// TODO: no need for this???
		//if (TryMoveResourcesAny(info.resourceEnum, ResourceFindType::AvailableForPickup, ResourceFindType::AvailableForDropoff, amountAtLeast) || justReset()) return true;
		
		return false;
	}

	bool TryStoreInventory();

	bool TryClearLand(TileArea area, Building* farmPtr = nullptr);

	// Fill Specific Building. Worker fill workshop raw material
	bool TryFillWorkplace(ResourceEnum resourceEnum);
	bool TryFindFood();
	bool TryHeatup();
	bool TryToolup();
	bool TryHealup();
	bool TryFillLuxuries();

	FoundResourceHolderInfos FindNeedHelper(ResourceEnum resourceEnum, int32 wantAmount, int32 maxDistance);

	// TODO: might be removable..
	//FoundResourceHolderInfos FindMarketResourceHolderInfo(ResourceEnum resourceEnum, int32 wantAmount, bool checkOnlyOneMarket, int32 maxFloodDist);

	bool TryFun();

	bool TryGatherFruit();
	bool TryHunt();
	bool TryRanch();
	
	bool TryFarm();
	bool TryClearFarmDrop(class Farm& farm, int32 minDropCount);

	bool TryBulkHaul_ShippingDepot();
	bool TryBulkHaul_Intercity();
	bool TryBulkHaul_IntercityWater();
	bool TryBulkHaul_Market();
	bool TryHaulingServices();
	bool TryHaulingPowerPlant();
	bool TryDistribute_Market();
	//bool TryConsumerWork();

	bool TryGather(bool treeOnly);

	bool TryForestingCut(bool cutAndPlant);
	bool TryForestingPlant(TileObjEnum lastCutTileObjEnum, NonWalkableTileAccessInfo accessInfo = NonWalkableTileAccessInfo());
	bool TryForestingNourish();
	bool TryForesting();

	bool TryProduce();

	bool TryConstruct() {
		return TryConstructHelper(_workplaceId);
	}
	bool TryConstructRoad();
	bool TryConstructHelper(int32 workplaceId);

	bool TryGoNearWorkplace(int32 distanceThreshold);

	bool TryCheckBadTile_Human();

	//
	
	/*
	 * Actions
	 */
	void Add_AttackOutgoing(UnitFullId defender, int32 damage);	void AttackOutgoing();
	//void Add_SearchAreaForDrop(ResourceEnum resourceEnum);			void SearchAreaForDrop();

	//void Add_DoConsumerWork(int32_t workManSec100);					void DoConsumerWork();
	void Add_DoFarmWork(int32 farmTileId, WorldTile2 farmWorldTile, FarmStage farmStage);		void DoFarmWork();

	void Add_TryForestingPlantAction(TileObjEnum tileObjEnum, NonWalkableTileAccessInfo accessInfo);		void TryForestingPlantAction();


	void ReserveAndAdd_DoFarmWork(const FarmTile& farmTile, FarmStage farmStage, int32 workplaceId = -1);
	

	void ExecuteAction() override
	{
		LEAN_PROFILING_U(ExecuteAction);
		
#define CASE(Action) case ActionEnum::Action: Action(); break;
		switch (_currentAction.actionEnum) {
			// HumanAI
			CASE(AttackOutgoing);
			//CASE(SearchAreaForDrop);

			//CASE(DoConsumerWork);
			CASE(DoFarmWork);

			CASE(TryForestingPlantAction);

		default:
			UnitStateAI::ExecuteAction();
		}
#undef CASE
	}

	/*
	 * States
	 */

	int32 workEfficiency100(bool isMoveSpeedPenalty = false)
	{
		if (_townId == -1) {
			return 100;
		}
		
		int32 foodPenalty = foodWorkPenalty100();
		int32 heatPenalty = heatWorkPenalty100();
		int32 toolPenalty = isMoveSpeedPenalty ? 0 : toolPenalty100();
		int32 sicknessPenalty = sicknessPenalty100();
		int32 happinessPenalty = isMoveSpeedPenalty ? 0 : happinessPenalty100();
		
		int32 result = 100 + std::min(foodPenalty, 
						std::min(heatPenalty, 
						std::min(toolPenalty,
						std::min(sicknessPenalty, happinessPenalty))));

		result += speedBoostEfficiency100();

		PUN_CHECK(result >= 0);
		return result;
	}
	
	int32 foodWorkPenalty100() {
		if (_food < minWarnFood()) {
			int32 result = 50 * (minWarnFood() - _food) / minWarnFood();
			PUN_CHECK(result >= 0);
			return result;
		}
		return 0;
	}
	int32 heatWorkPenalty100() {
		if (_heat < minWarnHeat()) {
			int32 result = 50 * (minWarnHeat() - _heat) / minWarnHeat();
			PUN_CHECK(result >= 0);
			return result;
		}
		return 0;
	}
	
	int32 toolPenalty100() {
		return needTools() ? -75 : 0;
	}
	int32 sicknessPenalty100() {
		if (_simulation->GetResourceCount(_townId, MedicineEnums) > 0) {
			return 0;
		}
		return isSick() ? -50 : 0;
	}

	int32 happinessPenalty100() {
		int32 happ = happinessOverall();
		if (happ < minWarnHappiness()) {
			// at minWarnHappiness, penalty is 0
			// at 0, penalty is -50
			int32 result = -90 * (minWarnHappiness() - happ) / minWarnHappiness();

			if (_simulation->TownhallCardCountTown(_playerId, CardEnum::SlaveLabor)) {
				result = std::max(-30, result);
			}
			
			PUN_CHECK(result <= 0);
			return result;
		}
		return 0;
	}

	int32 speedBoostEfficiency100();

	/*
	 * Happiness
	 */
	// Housing
	int32 housingHappiness();

	int32 GetHappinessByType(HappinessEnum happinessEnum) {
		int32 happinessEnumInt = static_cast<int32>(happinessEnum);
		if (happinessEnumInt < _happiness.size()) {
			return _happiness[happinessEnumInt];
		}
		return 0;
	}

	void SetHappiness(HappinessEnum happinessEnum, int32 value) {
		int32 happinessEnumInt = static_cast<int32>(happinessEnum);
		if (happinessEnumInt < _happiness.size()) {
			_happiness[static_cast<int>(happinessEnum)] = value;
		}
	}

	void UpdateHappiness();

	void OnUnitUpdate() override {
		UpdateHappiness();
	}

	//int32 baseHappiness() {
	//	return (foodHappiness() + heatHappiness() + housingHappiness() + funHappiness()) / 4;
	//}

	// TODO: remove?
	//int32 GetHappinessModifier(HappinessModifierEnum modifierEnum)
	//{
	//	// Guard
	//	if (_townId == -1) {
	//		return 0;
	//	}
	//	
	//	switch (modifierEnum)
	//	{
	//	//case HappinessModifierEnum::Luxury: return luxuryHappinessModifier();
	//	//case HappinessModifierEnum::HappyBreadDay: {
	//	//	if (_simulation->TownhallCardCountTown(_townId, CardEnum::HappyBreadDay) == 0) {
	//	//		return 0;
	//	//	}
	//	//	return _simulation->resourceCountTown(_townId, ResourceEnum::Bread) >= 1000 ? 5 : 0;
	//	//}
	//	//case HappinessModifierEnum::BlingBling: {
	//	//	if (_simulation->TownhallCardCountTown(_townId, CardEnum::BlingBling) == 0) {
	//	//		return 0;
	//	//	}
	//	//	if (_houseId == -1) {
	//	//		return 0;
	//	//	}
	//	//	return _simulation->building(_houseId).tryResourceCount(ResourceEnum::Jewelry) > 0 ? 7 : 0;
	//	//}
	//	//	
	//	//case HappinessModifierEnum::Tax: return _simulation->taxHappinessModifier(_townId);
	//	////case HappinessModifierEnum::Cannibalism: return _simulation->cannibalismHappinessModifier(_playerId);
	//	//case HappinessModifierEnum::DeathStarve: return _simulation->citizenDeathHappinessModifier(_townId, SeasonStatEnum::DeathStarve);
	//	//case HappinessModifierEnum::DeathCold: return _simulation->citizenDeathHappinessModifier(_townId, SeasonStatEnum::DeathCold);
	//	default:
	//		UE_DEBUG_BREAK(); return -1;
	//	}
	//}
	
	//int32 luxuryHappinessModifier();
	
	//int32 modifiersHappiness() {
	//	int32 result = 0;
	//	for (int32 i = 0; i < HappinessModifierName.Num(); i++) {
	//		result += GetHappinessModifier(static_cast<HappinessModifierEnum>(i));
	//	}
	//	return result;
	//}
	
	int32 happinessOverall() override { 
		int32 result = 0;
		for (int32 i = 0; i < HappinessEnumCount; i++) {
			result += GetHappinessByType(static_cast<HappinessEnum>(i));
		}
		return result / HappinessEnumCount;
	}

	static int32 minWarnHappiness() { return 65; }
	static int32 happinessLeaveTownThreshold() { return 50; }
	bool needHappiness() { return happinessOverall() < minWarnHappiness(); }

	static int32 FunTickToPercent(int32 funTicks) {
		return funTicks * 100 / FunTicksAt100Percent;
	}
	int32 funPercent(int32 funServiceEnumInt) {
		return FunTickToPercent(_serviceToFunTicks[funServiceEnumInt]);
	}
	int32 funTicks(int32 funServiceEnumInt) {
		return _serviceToFunTicks[funServiceEnumInt];
	}

	/*
	 * Medicine/Tools
	 */
	bool needHealthcare() {
		if (_simulation->GetResourceCount(_townId, MedicineEnums) > 0) {
			return false;
		}
		return _isSick && _hp100 < 5000;
	}

	int32 nextToolTick() { return _nextToolNeedTick; }
	bool needTools() 	{
		return !isBelowWorkingAge() &&
				Time::Ticks() > _nextToolNeedTick && 
				!resourceSystem().HasAvailableTools();
	}

	/*
	 * Others
	 */
	UnitAIClassEnum classEnum() override { return UnitAIClassEnum::HumanStateAI; }

	bool isBelowWorkingAge() override {
		return age() < _simulation->parameters(_playerId)->BeginWorkingAgeTicks();
	}
	bool isChild() override {
		return age() < PlayerParameters::BaseAdultTicks;
	}
	FText GetTypeName() override;

	void SendToTownLand(int32 lastTownId, int32 newTownId)
	{
		_simulation->ResetUnitActions(_id, 0);
		
		WorldTile2 lastTownGate = _simulation->GetTownhallGate(lastTownId);
		WorldTile2 newTownGate = _simulation->GetTownhallGate(newTownId);

		_townId = newTownId;
		_playerId = _simulation->townPlayerId(newTownId);

		Add_MoveToCaravan(newTownGate, UnitAnimationEnum::ImmigrationCart);
		Add_MoveTo(lastTownGate);
	}


	void SendToTownWater(int32 lastTownId, int32 newTownId, int32 startPortId, int32 endPortId)
	{
		_simulation->ResetUnitActions(_id, 0);

		// Go to old townhall, go to startPort, go to endPort, go to new townhall
		WorldTile2 lastTownGate = _simulation->GetTownhallGate(lastTownId);
		WorldTile2 newTownGate = _simulation->GetTownhallGate(newTownId);

		WorldTile2 startPortGate = _simulation->building(startPortId).gateTile();
		WorldTile2 endPortGate = _simulation->building(endPortId).gateTile();

		_townId = newTownId;
		_playerId = _simulation->townPlayerId(newTownId);
		

		Add_MoveTo(newTownGate, -1, UnitAnimationEnum::ImmigrationCart);
		Add_MoveToward(endPortGate.worldAtom2(), 100000, UnitAnimationEnum::ImmigrationCart); // TODO: Have Forced Move To Later?
		Add_MoveToShip(startPortId, endPortId, UnitAnimationEnum::Ship);

		// People got stuck in the sea, just move them to port to go home...
		if (!IsMoveValid(startPortGate)) {
			_simulation->MoveUnitInstantly(_id, startPortGate.worldAtom2());
		}
		else {
			Add_MoveTo(startPortGate, -1, UnitAnimationEnum::ImmigrationCart);
			Add_MoveTo(lastTownGate);
		}
	}


protected:
	void MoveResourceSequence(std::vector<FoundResourceHolderInfo> providerInfos, std::vector<FoundResourceHolderInfo> dropoffInfos, int32 customFloodDistance = -1, UnitAnimationEnum animationEnum = UnitAnimationEnum::Walk);

	void GatherSequence(NonWalkableTileAccessInfo accessInfo);

private:
	int32 haulCapacity() {
		int32 haulCapacity = 10;
		if (_simulation->IsResearched(_playerId, TechEnum::Logistics5)) {
			haulCapacity *= 2;
		}
		return haulCapacity;
	}
	int32 haulerServicesCapacity() {
		int32 haulCapacity = 50;
		if (_simulation->IsResearched(_playerId, TechEnum::Logistics5)) {
			haulCapacity *= 2;
		}
		return haulCapacity;
	}
	int32 bulkHaulCapacity() {
		int32 haulCapacity = 50;
		if (_simulation->IsResearched(_playerId, TechEnum::Logistics5)) {
			haulCapacity *= 2;
		}
		return haulCapacity;
	}
};
