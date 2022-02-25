// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include <chrono>
#include <unordered_map>
#include <string>
#include "PunCity/PunUtils.h"

DECLARE_LOG_CATEGORY_EXTERN(LogTimer, Log, All);

static std::chrono::time_point<std::chrono::steady_clock> TimerStart() {
	return std::chrono::high_resolution_clock::now();
}

static void TimerEnd(std::chrono::time_point<std::chrono::steady_clock> time1, FString message, int multiplier = 1)
{
	auto time2 = std::chrono::high_resolution_clock::now();
	auto time_span = std::chrono::duration_cast<std::chrono::nanoseconds>(time2 - time1);
	UE_LOG(LogTimer, Log, TEXT("%d ms, %s"), time_span.count() * multiplier / 1000000, *message);
}

// Scope timer
struct ScopeTimer
{
	std::chrono::time_point<std::chrono::steady_clock> time1;
	FString message;
	int multiplier;
	long nanoSecondsThreshold;

	TArray<uint8>* data = nullptr;
	int32 initialSize = 0;
	bool isSerializeTimer = false;
	std::vector<int32>* crcs = nullptr; // SaveCheck

	ScopeTimer(FString message, int multiplier = 1, long microSecondsThreshold = 0) :
			message(message), multiplier(multiplier)
	{
		nanoSecondsThreshold = microSecondsThreshold * 1000;
		initialSize = 0;
		time1 = std::chrono::high_resolution_clock::now();
	}

	static ScopeTimer CreateSerializeTimer(FString message, TArray<uint8>* data, std::vector<int32>* crcs)
	{
		ScopeTimer scopeTimer(message);
		scopeTimer.initialSize = data->Num();
		scopeTimer.data = data;
		scopeTimer.isSerializeTimer = true;
		scopeTimer.crcs = crcs;
		return scopeTimer;
	}
	
	~ScopeTimer()
	{
		// This is so that "ScopeTimer scopeTimer = X" doesn't spawn spew log two times...
		if (isSerializeTimer && data->Num() == initialSize) {
			return;
		}
		
		auto time2 = std::chrono::high_resolution_clock::now();
		auto time_span = std::chrono::duration_cast<std::chrono::nanoseconds>(time2 - time1);
		auto nanoseconds = time_span.count();
		
		// Serialize Timer
		if (data) {
			int32 sizeDiff = data->Num() - initialSize;
			int32 crc32 = FCrc::MemCrc32(data->GetData(), data->Num());
			if (crcs) {
				crcs->push_back(crc32);
				_LOG(PunSaveCheck, "%d.%dms  %umb  crc:%u  %s", nanoseconds / 1000000, (nanoseconds / 1000) % 1000, sizeDiff / 1000000, crc32, *message);
			}
			
			UE_LOG(LogTimer, Log, TEXT("%d.%dms  %umb  crc:%u  %s"), nanoseconds / 1000000, (nanoseconds / 1000) % 1000, sizeDiff / 1000000, crc32, *message);
			return;
		}

		// Normal
		if (multiplier == 1) {
			if (nanoseconds > nanoSecondsThreshold) {
				if (nanoseconds > 1000) {
					UE_LOG(LogTimer, Log, TEXT("%d.%d ms, %s"), nanoseconds / 1000000, (nanoseconds / 1000) % 1000, *message);
				} else {
					UE_LOG(LogTimer, Log, TEXT("%dns, %s"), nanoseconds % 1000, *message);
				}
			}
		} 
		else {
			UE_LOG(LogTimer, Log, TEXT("%d ms, mul:%d, %s"), nanoseconds / 1000000, nanoseconds * multiplier / 1000000, *message);
		}
	}
};

#define SCOPE_TIMER(x) ScopeTimer scopeTimer(x);
#define SCOPE_TIMER_(Format, ...) ScopeTimer scopeTimer(FString::Printf(TEXT(Format), ##__VA_ARGS__));

#define SERIALIZE_TIMER(message, data, crcs, crcLabels) ScopeTimer scopeTimer = ScopeTimer::CreateSerializeTimer(message, &data, crcs);\
														if (crcLabels) crcLabels->push_back(message);

// Editor only, since this is every tick
#if WITH_EDITOR || TRAILER_MODE
	#define SCOPE_TIMER_FILTER(microSecondsThreshold, Format, ...) ScopeTimer scopeTimer(FString("[TIME_FILTER]") + FString::Printf(TEXT(Format), ##__VA_ARGS__), 1, microSecondsThreshold);
#else
	#define SCOPE_TIMER_FILTER(microSecondsThreshold, Format, ...) 
#endif

#define SCOPE_TIMER_MUL(x, y) ScopeTimer scopeTimer(x, y);

// For timing things in a loop
struct ScopeTimerLoop
{
	std::string message;
	std::chrono::time_point<std::chrono::steady_clock> time1;

	static std::unordered_map<std::string, std::chrono::nanoseconds> time_spans;

	ScopeTimerLoop(std::string message) : message(message) {
		time1 = std::chrono::high_resolution_clock::now();
	}
	~ScopeTimerLoop() {
		auto time2 = std::chrono::high_resolution_clock::now();
		time_spans[message] += std::chrono::duration_cast<std::chrono::nanoseconds>(time2 - time1);
	}
};

#define SCOPE_TIMER_LOOP(message) { ScopeTimerLoop scopeTimerLoop(message); }
#define SCOPE_TIMER_LOOP_PRINT(message) UE_LOG(LogTimer, Log, TEXT("%d ms, %s"), ScopeTimerLoop::time_spans[message].count() / 1000000, *FString(message));




//#define PUN_CRC_LOG(name, Ar, data) { \
//			if (Ar.IsSaving()) { \
//				UE_LOG(LogTimer, Log, TEXT("%s %umb crc: %u"), *FString(name), data.Num()/1000000, FCrc::MemCrc32(data.GetData(), data.Num())); \
//			} \
//		};

#if USE_LEAN_PROFILING


#define OUTER_PROFILING_LIST(entry) \
	entry(TickSim) \
	entry(TickUI) \
	entry(TickDisplay)

#define DISPLAY_PROFILING_LIST(entry) \
	entry(TickUnitDisplay) \
	entry(TickUnitDisplay_Resource) \
	entry(TickUnitDisplay_Unit) \
	entry(TickUnitDisplay_Skel) \
	entry(TickUnitDisplay_Skel2) \
	entry(TickTerrainRegion) \
	entry(TickTerrainRegion4x4) \
	entry(TickRegionDecal) \
	entry(TickTileObjDisplay) \
	entry(TickResourceDisplay) \
	entry(TickDisplayMisc) \
	entry(TickMiniBuildingDisplay) \
	entry(TickBuildingDisplay) \
	entry(TickBuildingLOD2Display)

#define UI_PROFILING_LIST(entry) \
	entry(TickMainGameUI) \
	entry(TickMainGameUI_Cards) \
	entry(TickMainGameUI_Events) \
	entry(TickMainGameUI_Exclamation) \
	entry(TickMainGameUI_LeftUI) \
	entry(TickMainGameUI_TopLeft) \
	entry(TickMainGameUI_TopLeftTip) \
	entry(TickMainGameUI_Happiness) \
	entry(TickMainGameUI_Money) \
	entry(TickMainGameUI_MoneyTip) \
	entry(TickMainGameUI_Influence) \
	entry(TickMainGameUI_Science) \
	entry(TickMainGameUI_TownSwap) \
	entry(TickMainGameUI_Skill) \
	entry(TickMainGameUI_LeftFood) \
	entry(TickMainGameUI_LeftLuxTip) \
	entry(TickMainGameUI_LeftResources) \
	entry(TickMainGameUI_TechBar) \
	entry(TickMainGameUI_JobPrior) \
	entry(TickPopupUI) \
	entry(TickQuestUI) \
	entry(TickPlayerDetails) \
	entry(TickPlayerDetails_Graph) \
	entry(TickPlayerCompare) \
	entry(TickEscMenuUI) \
	entry(TickChatUI) \
	entry(TickDebugUI) \
	entry(TickTopLayerGameUI) \
	entry(TickWorldSpaceUI) \
	entry(TickWorldSpaceUI_Building) \
	entry(TickWorldSpaceUI_Townhall) \
	entry(TickWorldSpaceUI_BldJob) \
	entry(TickWorldSpaceUI_BldJobTile) \
	entry(TickWorldSpaceUI_BldJobUC) \
	entry(TickWorldSpaceUI_BldJobHouse) \
	entry(TickWorldSpaceUI_BldJobWork) \
	entry(TickWorldSpaceUI_BldJobHumanSlots) \
	entry(TickWorldSpaceUI_BldJobShowBars) \
	entry(TickWorldSpaceUI_BldJobSetSlots) \
	entry(TickWorldSpaceUI_BldJobBldStatus) \
	entry(TickWorldSpaceUI_BldJobResourceComplete) \
	entry(TickWorldSpaceUI_BldJobHoverWarning) \
	entry(TickWorldSpaceUI_BldOverlay) \
	entry(TickWorldSpaceUI_Province) \
	entry(TickWorldSpaceUI_BldFloatup) \
	entry(TickWorldSpaceUI_Unit) \
	entry(TickWorldSpaceUI_Unit2) \
	entry(TickWorldSpaceUI_GetHoverUI) \
	entry(TickWorldSpaceUI_Map) \
	entry(TickObjDescriptionUI) \
	entry(TickWorldTradeUI) \
	entry(TickIntercityTradeUI) \
	entry(TickTargetConfirmUI) \
	entry(TickInitialResourceUI) \
	entry(TickSendImmigrantsUI) \
	entry(TickDiplomacyUI) \
	entry(TickTechTreeUI) \
	entry(TickStatisticsUI)


#define RESOURCE_PROFILING_LIST(entry) \
	entry(R_resourceCount) \
	entry(R_resourceCountWithPop) \
	entry(R_resourceCountWithDrops) \
	entry(R_resourceCountDropOnly) \
	\
	entry(R_HasAvailableFood) \
	entry(R_HasAvailableHeat) \
	entry(R_HasAvailableMedicine) \
	entry(R_HasAvailableTools) \
	\
	entry(R_CanAddResourceGlobal) \
	\
	entry(R_CanReceiveAmount) \
	entry(R_CanReceiveAmountAfterReset) \
	entry(R_FindHolder) \
	entry(R_FindFoodHolder) \
	entry(R_CanAddReservation) \
	\
	entry(R_GetDropFromSmallArea_Any) \
	entry(R_GetDropFromArea_Pickable) \
	entry(R_GetDropsFromArea_Pickable) \
	\
	entry(R_TreeSysTick)







//
#define CREATE_ENUM(name) name,
#define CREATE_STRINGS(name) #name,

	enum class LeanProfilerEnum
	{
		OUTER_PROFILING_LIST(CREATE_ENUM)
		
#if USE_DISPLAY_PROFILING
		DISPLAY_PROFILING_LIST(CREATE_ENUM)
#endif

#if USE_UI_PROFILING
		UI_PROFILING_LIST(CREATE_ENUM)
#endif
		
#if USE_RESOURCE_PROFILING
		RESOURCE_PROFILING_LIST(CREATE_ENUM)
#endif

#if USE_PATH_PROFILING
		P_FindPath, // Also ResourceProfiling Count;
		P_FindPathAnimal,
		P_FindPathRoadOnly,
		P_FindPathRobust,
		
		IsConnected,
		IsConnectedBuilding,
		FindNearestBuildingId,
#endif

#if USE_UNIT_PROFILING
		// Update group
		U_Update_FoodAge, // Also PathProfilingCount;
		U_CalcAnimal,
		U_CalcHuman,
		U_ExecuteAction,

		// Try Group
		TryCheckBadTile_Human,
		TryStoreInventory,
		TryConstructHelper, // Includes TryConstruct
		TryFindFood,
		TryHeatup,
		TryToolup,
		TryHealup,
		TryFillLuxuries,
		TryFun,
		TryGatherFruit,
		TryHunt,
		TryRanch,
		TryFarm,
		TryBulkHaul_ShippingDepot,
		TryBulkHaul_Intercity,
		TryBulkHaul_Market,
		TryHaulingServices,
		TryHaulingPowerPlant,
		TryGather,
		TryForesting,
		TryFillWorkplace,
		TryProduce,
		TryConstructRoad,
		TryGoNearWorkplace,
#endif

#if USE_ACTION_PROFILING
		// Action Group
		AttackOutgoing,
		DoFarmWork,
		TryForestingPlantAction,
		Wait,
		MoveRandomly,
		MoveRandomlyAnimal,
		MoveRandomlyPerlin,
		GatherFruit,
		TrimFullBush,
		HarvestTileObj,
		PlantTree,
		NourishTree,
		MoveTo_UseCache,
		MoveTo_Robust,
		MoveTo_Human,
		MoveTo_Animal,
		MoveTo_Ending,
		MoveToResource,
		MoveToResource_Drop,
		MoveToResource_custom,
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
		PickupFoodAnimal,
		DropoffFoodAnimal,
		
#endif
		
		Count,
	};
	static const std::vector<std::string> LeanScopeTimerChar
	{
		OUTER_PROFILING_LIST(CREATE_STRINGS)
		
#if USE_DISPLAY_PROFILING
		DISPLAY_PROFILING_LIST(CREATE_STRINGS)
#endif

#if USE_UI_PROFILING
		UI_PROFILING_LIST(CREATE_STRINGS)
#endif
		
#if USE_RESOURCE_PROFILING
		RESOURCE_PROFILING_LIST(CREATE_STRINGS)
#endif

#if USE_PATH_PROFILING
		"P_FindPath",
		"P_FindPathAnimal",
		"P_FindPathRoadOnly",
		"P_FindPathRobust",

		"IsConnected",
		"IsConnectedBuilding",
		"FindNearestBuildingId",
#endif

#if USE_UNIT_PROFILING

		// Update group
		"U_Update_FoodAge", // Also PathProfilingCount;
		"U_CalcAnimal",
		"U_CalcHuman",
		"U_ExecuteAction",

		// Try Group
		"TryCheckBadTile_Human",
		"TryStoreInventory",
		"TryConstructHelper", // Includes TryConstruct
		"TryFindFood",
		"TryHeatup",
		"TryToolup",
		"TryHealup",
		"TryFillLuxuries",
		"TryFun",
		"TryGatherFruit",
		"TryHunt",
		"TryRanch",
		"TryFarm",
		"TryBulkHaul_ShippingDepot",
		"TryBulkHaul_Intercity",
		"TryBulkHaul_Market",
		"TryHaulingServices",
		"TryHaulingPowerPlant",
		"TryGather",
		"TryForesting",
		"TryFillWorkplace",
		"TryProduce",
		"TryConstructRoad",
		"TryGoNearWorkplace",
#endif

#if USE_ACTION_PROFILING
		// Action Group
		"AttackOutgoing",
		"DoFarmWork",
		"TryForestingPlantAction",
		"Wait",
		"MoveRandomly",
		"MoveRandomlyAnimal",
		"MoveRandomlyPerlin",
		"GatherFruit",
		"TrimFullBush",
		"HarvestTileObj",
		"PlantTree",
		"NourishTree",
		"MoveTo_UseCache",
		"MoveTo_Robust",
		"MoveTo_Human",
		"MoveTo_Animal",
		"MoveTo_Ending",
		"MoveToResource",
		"MoveToResource_Drop",
		"MoveToResource_custom",
		"MoveInRange",
		//"MoveToForceLongDistance",
		"MoveToRobust",
		"MoveToward",
		"MoveToCaravan",
		"MoveToShip",
		"Produce",
		"Construct",
		"Eat",
		"Heat",
		"UseMedicine",
		"UseTools",
		"HaveFun",
		"PickupResource",
		"DropoffResource",
		"DropInventoryAction",
		"StoreGatheredAtWorkplace",
		"FillInputs",
		"IntercityHaulPickup",
		"IntercityHaulDropoff",
		"PickupFoodAnimal",
		"DropoffFoodAnimal",
#endif

	};

#undef CREATE_ENUM
#undef CREATE_STRINGS

	struct LeanProfilerElement
	{
		LeanProfilerEnum profilerEnum;
		int32 count;
		long long nanosecondsSum;

		long long tickNanosecondsSum;
		long long maxTickNanosecondsSum;
	};

	struct LeanProfiler
	{
		std::chrono::time_point<std::chrono::steady_clock> time1;
		LeanProfilerEnum timerEnum = LeanProfilerEnum::R_resourceCount;
		
		static std::vector<LeanProfilerElement> EnumToElements;
		static std::vector<LeanProfilerElement> LastEnumToElements;

		LeanProfiler() {}
		
		LeanProfiler(LeanProfilerEnum timerEnumIn) {
			timerEnum = timerEnumIn;
			time1 = std::chrono::high_resolution_clock::now();
			
			if (EnumToElements.size() < LeanScopeTimerChar.size()) {
				for (int32 i = 0; i < LeanScopeTimerChar.size(); i++) {
					EnumToElements.push_back({ static_cast<LeanProfilerEnum>(i), 0, 0});
					LastEnumToElements.push_back({ static_cast<LeanProfilerEnum>(i), 0, 0 });
				}
			}
		}

		~LeanProfiler()
		{
			auto time2 = std::chrono::high_resolution_clock::now();
			auto time_span = std::chrono::duration_cast<std::chrono::nanoseconds>(time2 - time1);
			
			auto nanoseconds = time_span.count();
			EnumToElements[static_cast<long long>(timerEnum)].count++;
			EnumToElements[static_cast<long long>(timerEnum)].nanosecondsSum += nanoseconds;
			EnumToElements[static_cast<long long>(timerEnum)].tickNanosecondsSum += nanoseconds;
		}

		static void FinishInterval(int32 startIndex, int32 endIndex)
		{
			check(endIndex < EnumToElements.size());
			
			for (int32 i = startIndex; i <= endIndex; i++)
			{
				LastEnumToElements[i] = EnumToElements[i];

				EnumToElements[i].count = 0;
				EnumToElements[i].nanosecondsSum = 0;
				
				EnumToElements[i].tickNanosecondsSum = 0;
				EnumToElements[i].maxTickNanosecondsSum = 0;
			}
		}

		static void FinishTick(int32 startIndex, int32 endIndex)
		{
			check(endIndex < EnumToElements.size());
			
			for (int32 i = startIndex; i <= endIndex; i++)
			{
				EnumToElements[i].maxTickNanosecondsSum = std::max(EnumToElements[i].maxTickNanosecondsSum, EnumToElements[i].tickNanosecondsSum);
				EnumToElements[i].tickNanosecondsSum = 0;
			}
		}
	};

#define LEAN_PROFILING(leanProfilerEnumName) LeanProfiler leanProfiler(LeanProfilerEnum::##leanProfilerEnumName);

#if USE_RESOURCE_PROFILING
	#define LEAN_PROFILING_R(leanProfilerEnumName) LeanProfiler leanProfiler(LeanProfilerEnum::R_##leanProfilerEnumName);
#else
	#define LEAN_PROFILING_R(leanProfilerEnumName)
#endif

#if USE_PATH_PROFILING
	#define LEAN_PROFILING_P(leanProfilerEnumName) LeanProfiler leanProfiler(LeanProfilerEnum::P_##leanProfilerEnumName);
#else
	#define LEAN_PROFILING_P(leanProfilerEnumName)
#endif

#if USE_UNIT_PROFILING
	#define LEAN_PROFILING_U(leanProfilerEnumName) LeanProfiler leanProfiler(LeanProfilerEnum::U_##leanProfilerEnumName);
#else
	#define LEAN_PROFILING_U(leanProfilerEnumName)
#endif

#if USE_TRYCALC_PROFILING
	#define LEAN_PROFILING_T(leanProfilerEnumName) LeanProfiler leanProfiler(LeanProfilerEnum::##leanProfilerEnumName);
#else
	#define LEAN_PROFILING_T(leanProfilerEnumName)
#endif

#if USE_ACTION_PROFILING
	#define LEAN_PROFILING_A(leanProfilerEnumName) LeanProfiler leanProfiler(LeanProfilerEnum::##leanProfilerEnumName);
#else
	#define LEAN_PROFILING_A(leanProfilerEnumName)
#endif

#if USE_DISPLAY_PROFILING
	#define LEAN_PROFILING_D(leanProfilerEnumName) LeanProfiler leanProfiler(LeanProfilerEnum::##leanProfilerEnumName);
#else
	#define LEAN_PROFILING_D(leanProfilerEnumName)
#endif

#if USE_UI_PROFILING
	#define LEAN_PROFILING_UI(leanProfilerEnumName) LeanProfiler leanProfiler(LeanProfilerEnum::##leanProfilerEnumName);
#else
	#define LEAN_PROFILING_UI(leanProfilerEnumName)
#endif

#else

	#define LEAN_PROFILING(leanProfilerEnum)

	#define LEAN_PROFILING_R(leanProfilerEnumName)
	#define LEAN_PROFILING_P(leanProfilerEnumName)
	#define LEAN_PROFILING_U(leanProfilerEnumName)
	#define LEAN_PROFILING_T(leanProfilerEnumName)
	#define LEAN_PROFILING_A(leanProfilerEnumName)
	#define LEAN_PROFILING_D(leanProfilerEnumName)

#endif