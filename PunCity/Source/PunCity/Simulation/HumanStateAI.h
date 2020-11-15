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
	bool TryMoveResourcesProviderToDropoff(int32 providerBuildingId, int32 dropoffBuildingId, ResourceEnum resourceEnum, int32 amountAtLeast);
	bool TryMoveResourcesAnyProviderToDropoff(ResourceFindType providerType, FoundResourceHolderInfo dropoffInfo, bool prioritizeMarket = false, bool checkMarketAfter = false, int32 maxFloodDist = GameConstants::MaxFloodDistance_Human);
	bool TryMoveResourcesProviderToAnyDropoff(FoundResourceHolderInfo providerInfo, ResourceFindType dropoffType);
	bool TryMoveResourcesAny(ResourceEnum resourceEnum, ResourceFindType providerType, ResourceFindType dropoffType, int32 amountAtLeast);

	bool TryMoveResourcesToDeliveryTarget(int32 deliverySourceId, ResourceEnum resourceEnum, int32 amountAtLeast);
	bool TryMoveResourcesToDeliveryTargetAll(int32 amountAtLeast);

	bool TryMoveResourceAny(ResourceInfo info, int32 amountAtLeast)
	{
		if (TryMoveResourcesAny(info.resourceEnum, ResourceFindType::AvailableForPickup, ResourceFindType::Requester, amountAtLeast) || justReset()) return true;
		
		if (TryMoveResourcesAny(info.resourceEnum, ResourceFindType::Provider, ResourceFindType::AvailableForDropoff, amountAtLeast) || justReset()) return true;
		
		if (TryMoveResourcesAny(info.resourceEnum, ResourceFindType::AvailableForPickup, ResourceFindType::AvailableForDropoff, amountAtLeast) || justReset()) return true;
		
		return false;
	}

	bool TryStoreInventory();

	bool TryClearLand(TileArea area);

	// Fill Specific Building. Worker fill workshop raw material
	bool TryFillWorkplace(ResourceEnum resourceEnum);
	bool TryFindFood();
	bool TryHeatup();
	bool TryToolup();
	bool TryHealup();
	bool TryFillLuxuries();

	FoundResourceHolderInfos FindNeedHelper(ResourceEnum resourceEnum, int32 wantAmount, int32 maxFloodDist);

	FoundResourceHolderInfos FindMarketResourceHolderInfo(ResourceEnum resourceEnum, int32 wantAmount, bool checkOnlyOneMarket, int32 maxFloodDist);

	bool TryFun();

	bool TryGatherFruit();
	bool TryHunt();
	bool TryRanch();
	
	bool TryFarm();
	bool TryClearFarmDrop(class Farm& farm, int32 minDropCount);

	bool TryBulkHaul_ShippingDepot();
	bool TryBulkHaul_Market();
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
	
	/*
	 * Actions
	 */
	void Add_AttackOutgoing(UnitFullId defender, int32 damage);	void AttackOutgoing();
	//void Add_SearchAreaForDrop(ResourceEnum resourceEnum);			void SearchAreaForDrop();

	//void Add_DoConsumerWork(int32_t workManSec100);					void DoConsumerWork();
	void Add_DoFarmWork(WorldTile2 tile, FarmStage farmStage);		void DoFarmWork();

	void Add_TryForestingPlantAction(TileObjEnum tileObjEnum, NonWalkableTileAccessInfo accessInfo);		void TryForestingPlantAction();

	void ExecuteAction() override {
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

	int32 workEfficiency100(bool includeToolPenalty = true)
	{
		if (_playerId == -1) {
			return 100;
		}
		
		int32 foodPenalty = foodWorkPenalty100();
		int32 heatPenalty = heatWorkPenalty100();
		int32 toolPenalty = includeToolPenalty ? toolPenalty100() : 0;
		int32 sicknessPenalty = sicknessPenalty100();
		int32 happinessPenalty = happinessPenalty100();
		
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
		return needTools() ? -50 : 0;
	}
	int32 sicknessPenalty100() {
		if (_simulation->GetResourceCount(_playerId, MedicineEnums) > 0) {
			return 0;
		}
		return isSick() ? -50 : 0;
	}

	int32 happinessPenalty100() {
		int32 happ = happiness();
		if (happ < minWarnHappiness()) {
			int32 result = 90 * (minWarnHappiness() - happ) / minWarnHappiness();
			PUN_CHECK(result >= 0);
			return result;
		}
		return 0;
	}

	int32 speedBoostEfficiency100();

	/*
	 * Happiness
	 */
	// Core: Food, Heat, Housing, Entertainment
	int32 foodHappiness() { return std::max(0, std::min(70, _food * 70 / minWarnFood())); }
	int32 heatHappiness() { return std::min(100, _heat * 100 / minWarnHeat()); }
	int32 housingHappiness();
	int32 funHappiness() {
		return funPercent();
	}

	int32 baseHappiness() {
		return (foodHappiness() + heatHappiness() + housingHappiness() + funHappiness()) / 4;
	}

	int32 GetHappinessModifier(HappinessModifierEnum modifierEnum)
	{
		// Guard
		if (_playerId == -1) {
			return 0;
		}
		
		switch (modifierEnum)
		{
		case HappinessModifierEnum::Luxury: return luxuryHappinessModifier();
		case HappinessModifierEnum::HappyBreadDay: return _simulation->resourceCount(_playerId, ResourceEnum::Bread) >= 1000 ? 5 : 0;
		case HappinessModifierEnum::BlingBling: {
			if (_simulation->TownhallCardCount(_playerId, CardEnum::BlingBling) == 0) {
				return 0;
			}
			if (_houseId == -1) {
				return 0;
			}
			return _simulation->building(_houseId).tryResourceCount(ResourceEnum::Jewelry) > 0 ? 7 : 0;
		}
			
		case HappinessModifierEnum::Tax: return _simulation->taxHappinessModifier(_playerId);
		case HappinessModifierEnum::Cannibalism: return _simulation->cannibalismHappinessModifier(_playerId);
		case HappinessModifierEnum::DeathStarve: return _simulation->citizenDeathHappinessModifier(_playerId, SeasonStatEnum::DeathStarve);
		case HappinessModifierEnum::DeathCold: return _simulation->citizenDeathHappinessModifier(_playerId, SeasonStatEnum::DeathCold);
		default:
			UE_DEBUG_BREAK(); return -1;
		}
	}
	
	int32 luxuryHappinessModifier();
	
	int32 modifiersHappiness() {
		int32 result = 0;
		for (int32 i = 0; i < HappinessModifierEnumCount; i++) {
			result += GetHappinessModifier(static_cast<HappinessModifierEnum>(i));
		}
		return result;
	}
	
	int32 happiness() { 
		return baseHappiness() + modifiersHappiness();
	}

	int32 minWarnHappiness() { return 50; }
	bool needHappiness() { return happiness() < minWarnHappiness(); }

	int32 funPercent() {
		return _funTicks * 100 / FunTicksAt100Percent;
	}
	int32 funTicks() {
		return _funTicks;
	}

	/*
	 * Medicine
	 */
	void GetSick() {
		_isSick = true;
	}
	bool needHealthcare() {
		if (_simulation->GetResourceCount(_playerId, MedicineEnums) > 0) {
			return false;
		}
		return _isSick && _hp100 < 5000;
	}

	int32 nextToolTick() { return _nextToolNeedTick; }
	bool needTools() 	{
		return !isBelowWorkingAge() &&
				Time::Ticks() > _nextToolNeedTick && 
				!_simulation->resourceSystem(_playerId).HasAvailableTools();
	}

	UnitAIClassEnum classEnum() override { return UnitAIClassEnum::HumanStateAI; }

	bool isBelowWorkingAge() override {
		return age() < _simulation->parameters(_playerId)->BeginWorkingAgeTicks();
	}
	bool isChild() override {
		return age() < PlayerParameters::BaseAdultTicks;
	}
	std::string typeName() override {
		if (isMale()) {
			return isBelowWorkingAge() ? "little boy" : "man";
		}
		return isBelowWorkingAge() ? "little girl" : "woman";
	}

protected:
	void MoveResourceSequence(std::vector<FoundResourceHolderInfo> providerInfos, std::vector<FoundResourceHolderInfo> dropoffInfos, int32 customFloodDistance = -1);

	void GatherSequence(NonWalkableTileAccessInfo accessInfo);

};
