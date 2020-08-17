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
				UE_LOG(LogTimer, Log, TEXT("%d.%d ms, %s"), nanoseconds / 1000000, (nanoseconds / 1000) % 1000, *message);
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
	#define SCOPE_TIMER_FILTER(microSecondsThreshold, Format, ...) ScopeTimer scopeTimer(FString::Printf(TEXT(Format), ##__VA_ARGS__), 1, microSecondsThreshold);
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