// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "UnrealEngine.h"
#include "PunCity/PunGameSettings.h"
#include "PunCity/Simulation/GameSimulationInfo.h"

/*
 * INIT_LOG
 */
DECLARE_LOG_CATEGORY_EXTERN(PunInit, Log, Log);
DECLARE_LOG_CATEGORY_EXTERN(PunSound, Log, Log);
DECLARE_LOG_CATEGORY_EXTERN(PunAsset, Log, Log);
DECLARE_LOG_CATEGORY_EXTERN(PunSaveLoad, Log, Log);
DECLARE_LOG_CATEGORY_EXTERN(PunDisplay, Log, Log);

DECLARE_LOG_CATEGORY_EXTERN(PunTrailer, Log, Log);

DECLARE_LOG_CATEGORY_EXTERN(PunTick, Log, Log);
DECLARE_LOG_CATEGORY_EXTERN(PunPlayerOwned, Log, Log);
DECLARE_LOG_CATEGORY_EXTERN(PunResource, Log, Log);

DECLARE_LOG_CATEGORY_EXTERN(PunTerrain, Log, Log);

DECLARE_LOG_CATEGORY_EXTERN(PunTickHash, Log, Log);
DECLARE_LOG_CATEGORY_EXTERN(PunSync, Log, Log);

DECLARE_LOG_CATEGORY_EXTERN(PunSaveCheck, Log, Log);

DECLARE_LOG_CATEGORY_EXTERN(PunNetwork, Log, Log);

#define INIT_LOG(Format, ...) UE_LOG(PunInit, Log, TEXT(Format), ##__VA_ARGS__);

#define _LOG(LogCategory, Format, ...) UE_LOG(LogCategory, Log, TEXT(Format), ##__VA_ARGS__);
#define LOG_ERROR(LogCategory, Format, ...) UE_LOG(LogCategory, Error, TEXT(Format), ##__VA_ARGS__);

#define DEBUG_UPDATE_BUFFER 0

// EDITOR only
#if !defined(PUN_CHECK_EDITOR)
	#if DEBUG_BUILD
		#define PUN_CHECK_EDITOR(x) if (!(x)) { FDebug::DumpStackTraceToLog(); checkNoEntry(); }
	#else
		#define PUN_CHECK_EDITOR(x) 
	#endif
#endif

#if !defined(PUN_CHECK2_EDITOR)
	#if DEBUG_BUILD
		#define PUN_CHECK2_EDITOR(x, message) if (!(x)) { FDebug::DumpStackTraceToLog(); UE_LOG(LogTemp, Error, TEXT("PUN_ERROR:\n%s"), *FString((message).c_str())); checkNoEntry(); }
	#else
		#define PUN_CHECK2_EDITOR(x, message)
	#endif
#endif

#define LOOP_CHECK_START() int loop = 0;
#define LOOP_CHECK_END() loop++;\
							if (loop > 100000) {\
								FDebug::DumpStackTraceToLog();\
								UE_LOG(LogTemp, Error, TEXT("PUN_ERROR"));\
								checkNoEntry();\
								break;\
							}


/*
 * Utils
 */

#if !defined(PUN_DEBUG_EXPR)
	#if DEBUG_BUILD
		#define PUN_DEBUG_EXPR(x) x;
	#else
		#define PUN_DEBUG_EXPR(x)
	#endif
#endif

#if !defined(PUN_LOG)
	#if DEBUG_BUILD
		#define PUN_LOG(Format, ...) UE_LOG(LogTemp, Error, TEXT(Format), ##__VA_ARGS__);
	#else
		#define PUN_LOG(Format, ...)
	#endif
#endif

#if !defined(PUN_LOG_WARN)
#if DEBUG_BUILD
#define PUN_LOG_WARN(Format, ...) UE_LOG(LogTemp, Warning, TEXT(Format), ##__VA_ARGS__);
#else
#define PUN_LOG_WARN(Format, ...)
#endif
#endif


//#if !defined(PUN_CLOG)
//	#if DEBUG_BUILD
//	#define PUN_CLOG(Format, ...) UE_LOG(LogTemp, Error, TEXT(Format), ##__VA_ARGS__);
//	#else
//	#define PUN_CLOG(Format, ...)
//	#endif
//#endif


// A log is useful for querying in Debug.Print
#if DEBUG_BUILD
#pragma optimize( "", off )
class PunALog
{
public:
	// Allows looking up of TickLogs and displaying a proper one
	struct Pair {
		FString log;
		int32_t tick;
		Pair(FString log) : log(log) {
			tick = TimeDisplay::Ticks();
		}
	};

	static std::unordered_map<std::string, std::unordered_map<int32_t, std::vector<Pair>>> Logs;
	static std::unordered_map<std::string, std::unordered_map<int32_t, std::vector<FString>>> TickLogs;

	__declspec(noinline) static void Print(std::string AKey, int32_t AIndex) {
		std::vector<Pair>& aLogs = Logs[AKey][AIndex];
		PUN_LOG("Print ALog -- AKey:%s AIndex:%d\n", ToTChar(AKey), AIndex);
		int32_t curTick = 0;
		for (int i = 0; i < aLogs.size(); i++) {
			// Just moved tick, print tick related logs
			if (curTick != aLogs[i].tick) {
				curTick = aLogs[i].tick;
				auto& tickLogs = TickLogs[AKey][curTick];
				for (int j = 0; j < tickLogs.size(); j++) {
					PUN_LOG("-- %s", *tickLogs[j]);
				}
			}
			PUN_LOG("%s", *(aLogs[i].log));
		}
	}

	static void Log(std::string AKey, int32_t AIndex, FString log) {
		Logs[AKey][AIndex].push_back(Pair(log));
	}
	static void LogAll(std::string AKey, FString log) {
		TickLogs[AKey][TimeDisplay::Ticks()].push_back(log);
	}
};
#pragma optimize( "", on ) 
#endif

#if !defined(PUN_ALOG)
	#if DEBUG_BUILD
		#define PUN_ALOG(AKey, AIndex, Format, ...) PunALog::Log(AKey, AIndex, FString::Printf(TEXT(Format), ##__VA_ARGS__));
	#else
		#define PUN_ALOG(AKey, AIndex, Format, ...)
	#endif
#endif

#if !defined(PUN_ALOG_ALL)
	#if DEBUG_BUILD
		#define PUN_ALOG_ALL(AKey, Format, ...) PunALog::LogAll(AKey, FString::Printf(TEXT(Format), ##__VA_ARGS__));
	#else
		#define PUN_ALOG_ALL(AKey, Format, ...)
	#endif
#endif

#if !defined(PUN_ACHECK)
#if DEBUG_BUILD
#define PUN_ACHECK(x, AKey, AIndex) \
	if (!(x)) { \
		FDebug::DumpStackTraceToLog(); \
		PunALog::Print(AKey, AIndex); \
		checkNoEntry(); \
	}
#else
#define PUN_ACHECK(x, AKey, AIndex)
#endif
#endif

//

#if !defined(PUN_DEBUG)
	#if DEBUG_BUILD
		#define PUN_DEBUG(x) { if (GEngine) GEngine->AddOnScreenDebugMessage(-1, 10.0f, FColor::White, x); }
	#else
		#define PUN_DEBUG(x)
	#endif
#endif

#define PUN_DEBUG2(Format, ...) { \
		if (GEngine) {\
			GEngine->AddOnScreenDebugMessage(-1, 15.0f, FColor::White, FString::Printf(TEXT(Format), ##__VA_ARGS__)); \
		}\
		UE_LOG(LogTemp, Error, TEXT(Format), ##__VA_ARGS__);\
	}

//#if !defined(PUN_DEBUG2)
//	#if DEBUG_BUILD
//		#define PUN_DEBUG2(Format, ...) { if (GEngine) GEngine->AddOnScreenDebugMessage(-1, 10.0f, FColor::White, FString::Printf(TEXT(Format), ##__VA_ARGS__)); }
//	#else
//		#define PUN_DEBUG2(Format, ...)
//	#endif
//#endif



 static void PUN_DEBUG_FAST(FString string) {
	 if (GEngine) GEngine->AddOnScreenDebugMessage(-1, 0.1f, FColor::White, string);
 }

// // Help check for bad serialization function
//#if !defined(PUN_ARCHIVE_CHECK)
//	#if DEBUG_BUILD
//		#define PUN_ARCHIVE_CHECK(Archive, Array) { \
//			if (Archive.IsSaving()) { \
//				int32 checksum = FCrc::MemCrc32(Array.GetData(), Array.Num()); \
//				Archive << checksum; \
//			}\
//			if (Archive.IsLoading()) { \
//				int32 checksum = FCrc::MemCrc32(Array.GetData(), Array.Num()); \
//				int32 loadedChecksum = 0; \
//				Archive << loadedChecksum; \
//				PUN_CHECK(checksum == loadedChecksum); \
//			} \
//		};
//	#else
//		#define PUN_ARCHIVE_CHECK(Archive, Array)
//	#endif
//#end

