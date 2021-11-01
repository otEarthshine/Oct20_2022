#pragma once

#include "PunCity/PunAStar128x256.h"
#include "GameCoordinate.h"
#include "PunCity/GameConstants.h"
#include "Math/Color.h"

#include <string>
#include <cctype>
#include <sstream>
#include <algorithm>
#include "PunCity/PunGameSettings.h"
#include <iomanip>

//#include "PunCity/PunSTLContainerOverride.h"

#define TRAILER_MODE 0

// GAME_VERSION
// !!! Don't forget SAVE_VERSION !!!
#define MAJOR_VERSION 0
#define MINOR_VERSION 56 // 3 digit

#define VERSION_DAY 26
#define VERSION_MONTH 10
#define VERSION_YEAR 21

#define VERSION_DATE (VERSION_YEAR * 10000) + (VERSION_MONTH * 100) + VERSION_DAY
#define COMBINE_VERSION (MAJOR_VERSION * 1000000000) + (MINOR_VERSION * 1000000) + VERSION_DATE

#define GAME_VERSION COMBINE_VERSION

// SAVE_VERSION
#define MAJOR_SAVE_VERSION 0
#define MINOR_SAVE_VERSION 36 // 3 digit

#define VERSION_SAVE_DAY 26
#define VERSION_SAVE_MONTH 10
#define VERSION_SAVE_YEAR 21

#define VERSION_SAVE_DATE (VERSION_SAVE_YEAR * 10000) + (VERSION_SAVE_MONTH * 100) + VERSION_SAVE_DAY
#define SAVE_VERSION (MAJOR_SAVE_VERSION * 1000000000) + (MINOR_SAVE_VERSION * 1000000) + VERSION_SAVE_DATE


static std::string GetGameVersionString()
{
	std::stringstream ss;
	ss << MAJOR_VERSION << "." << MINOR_VERSION << " (" << VERSION_DAY << "/" << VERSION_MONTH << "/" << VERSION_YEAR << ")";
	return ss.str();
}

static FString GetGameVersionString(int32 version, bool includeDate = true)
{
	FString result;
	result += FString::FromInt(version / 1000000000) + ".";
	version = version % 1000000000;
	
	result += FString::FromInt(version / 1000000);
	version = version % 1000000;

	if (includeDate)
	{
		result += " (" + FString::FromInt(version / 10000);
		version = version % 10000;

		result += "/" + FString::FromInt(version / 100);
		version = version % 100;

		result += "/" + FString::FromInt(version) + ")";
	}
	
	return result;
}

//! Utils

#define ToFString(stdString) FString((stdString).c_str())

#define ToFName(stdString) FName(*ToFString(stdString))

#define ToTChar(stdString) UTF8_TO_TCHAR((stdString).c_str())

#define ToStdString(fString) (std::string(TCHAR_TO_UTF8(*(fString))))
#define ToWString(fString) (std::wstring(*(fString)))

#define FTextToStd(fText) (std::string(TCHAR_TO_UTF8(*(fText.ToString()))))
#define FTextToW(fText) (std::wstring(*(fText.ToString())))

#define FToUTF8(fString) (TCHAR_TO_UTF8(*(fString)));

#define ToFText(stdString) (FText::FromString(FString((stdString).c_str())))

#define DEBUG_BUILD !UE_BUILD_SHIPPING // WITH_EDITOR

#define FULL_CHECK 0

#define KEEP_ACTION_HISTORY 0
#define CHECK_TICKHASH 0 // Requires KEEP_ACTION_HISTORY
#define TICK_DEBUG_UI 1

#define USE_LEAN_PROFILING 1
#define USE_RESOURCE_PROFILING (USE_LEAN_PROFILING && 1)
#define USE_PATH_PROFILING (USE_LEAN_PROFILING && 1)
#define USE_UNIT_PROFILING (USE_LEAN_PROFILING && 1)
#define USE_TRYCALC_PROFILING (USE_LEAN_PROFILING && 1)
#define USE_ACTION_PROFILING (USE_LEAN_PROFILING && 1)
#define USE_DISPLAY_PROFILING (USE_LEAN_PROFILING && 1)
#define USE_UI_PROFILING (USE_LEAN_PROFILING && 1)

#if !defined(REMINDER_CHECK)
	#if FULL_CHECK
		#define REMINDER_CHECK(x) if (!(x)) { FDebug::DumpStackTraceToLog(); checkNoEntry(); }
	#else
		#define REMINDER_CHECK(x)
	#endif
#endif

#if !defined(PUN_CHECK)
	#if UE_BUILD_SHIPPING
		#define PUN_CHECK(x) 
	#elif DEBUG_BUILD
		#define PUN_CHECK(x) if (!(x)) { FDebug::DumpStackTraceToLog(); checkNoEntry(); }
	#else
		#define PUN_CHECK(x) //if (!(x)) { FDebug::DumpStackTraceToLog(); UE_LOG(LogTemp, Error, TEXT("PUN_ERROR")); }
	#endif
#endif

#if !defined(PUN_ENSURE)
	#if DEBUG_BUILD
		#define PUN_ENSURE(condition, backupStatement) if (!(condition)) { FDebug::DumpStackTraceToLog(); UE_DEBUG_BREAK(); backupStatement; }
	#else
		#define PUN_ENSURE(condition, backupStatement) if (!(condition)) { /*FDebug::DumpStackTraceToLog(); UE_LOG(LogTemp, Error, TEXT("PUN_ERROR_ENSURE"));*/ backupStatement; }
	#endif
#endif

#define PUN_CHECK_CRIT(x) if (!(x)) { FDebug::DumpStackTraceToLog(); checkNoEntry(); }

#if !defined(PUN_CHECK2)
	#if DEBUG_BUILD
		#define PUN_CHECK2(x, message) if (!(x)) { FDebug::DumpStackTraceToLog(); UE_LOG(LogTemp, Error, TEXT("PUN_ERROR_CHECK2:\n%s"), *FString((message).c_str())); checkNoEntry(); }
	#else
		#define PUN_CHECK2(x, message) if (!(x)) { FDebug::DumpStackTraceToLog(); UE_LOG(LogTemp, Error, TEXT("PUN_ERROR_CHECK2:\n%s"), *FString((message).c_str())); }
	#endif
#endif


#if !defined(PUN_NOENTRY)
	#if DEBUG_BUILD
		#define PUN_NOENTRY() { FDebug::DumpStackTraceToLog(); checkNoEntry(); }
	#else
		#define PUN_NOENTRY() { FDebug::DumpStackTraceToLog(); UE_LOG(LogTemp, Error, TEXT("PUN_ERROR")); checkNoEntry(); }
	#endif
#endif

#if !defined(PUN_NOENTRY_LOG)
	#if DEBUG_BUILD
		#define PUN_NOENTRY_LOG(message) { FDebug::DumpStackTraceToLog(); UE_LOG(LogTemp, Error, TEXT("PUN_ERROR:\n%s"), *FString(message.c_str())); checkNoEntry(); }
	#else
		#define PUN_NOENTRY_LOG(message) { FDebug::DumpStackTraceToLog(); UE_LOG(LogTemp, Error, TEXT("PUN_ERROR:\n%s"), *FString(message.c_str())); checkNoEntry(); }
	#endif
#endif

class PunStringStream
{
public:
	
};

#define FTEXT(textIn) (FText::FromString(FString(textIn)))

#define TEXT_INTEGER(number) FText::AsNumber(static_cast<int>(number) / 100)
// Display decimal .X only if X is not 0
#define TEXT_DECIMAL(number) (((static_cast<int>(number) % 100) / 10) == 0 ? FText() : FText::Format(INVTEXT(".{0}"), FText::AsNumber(abs(static_cast<int>(number) % 100) / 10)))

// For FText::Format instead
#define TEXT_DECIMAL1(number) ((static_cast<int>(number) % 100) / 10) == 0 ? FText() : INVTEXT(".")
#define TEXT_DECIMAL2(number) ((static_cast<int>(number) % 100) / 10) == 0 ? FText() : FText::AsNumber(abs(static_cast<int>(number) % 100) / 10)


#define TEXT_PERCENT(number) FText::Format(INVTEXT("{0}%"), FText::AsNumber(number))
//#define TEXT_100(number) FText::Join(FText(), TEXT_INTEGER(number), TEXT_DECIMAL(number))
#define TEXT_100(number) FText::Format(INVTEXT("{0}{1}{2}"), TEXT_INTEGER(number), TEXT_DECIMAL1(number), TEXT_DECIMAL2(number))
#define TEXT_100_2(number) FText::Join(FText(), FText::AsNumber(number / 100), INVTEXT("."), FText::AsNumber(abs(static_cast<int>(number) % 100)))

//#define TEXT_100SIGNED(number) FText::Join(FText(), (number > 0 ? INVTEXT("+") : INVTEXT("")), TEXT_INTEGER(number), TEXT_DECIMAL(number))
#define TEXT_100SIGNED(number) FText::Format(INVTEXT("{0}{1}{2}{3}"), (number > 0 ? INVTEXT("+") : INVTEXT("")), TEXT_INTEGER(number), TEXT_DECIMAL1(number), TEXT_DECIMAL2(number))


#define TEXT_NUM(number) FText::AsNumber(number)
#define TEXT_NUMSIGNED(number) FText::Format(INVTEXT("{0}{1}"), (number > 0 ? INVTEXT("+") : INVTEXT("")), FText::AsNumber(number))
#define TEXT_NUMINT(number) FText::AsNumber(static_cast<int>(number))


#define TEXT_FLOAT1(number) FText::Join(FText(), FText::AsNumber(static_cast<int>(number)), INVTEXT("."), FText::AsNumber(static_cast<int>(number * 10) % 10)
#define TEXT_FLOAT1_PERCENT(number) FText::Join(FText(), TEXT_FLOAT1(number), INVTEXT("%"))

#define TEXT_TAG(Tag, InText) FText::Format(INVTEXT("{0}{1}</>"), INVTEXT(Tag), InText)
//#define TEXT_JOIN(...) FText::Join(FText(), __VA_ARGS__)

#define ADDTEXT(InArgs, InText, ...) InArgs.Add(FText::Format(InText, __VA_ARGS__));
#define ADDTEXT_(InText, ...) args.Add(FText::Format(InText, __VA_ARGS__));
#define ADDTEXT_NAMED_(InText, ...) args.Add(FText::FormatNamed(InText, __VA_ARGS__));
#define ADDTEXT__(InText) args.Add(InText);
#define CLEARTEXT_() args.Empty();

#define ADDTEXT_JOIN_(...) args.Add(FText::Join(__VA_ARGS__));

#define ADDTEXT_NUM(InArgs, number) InArgs.Add(FText::AsNumber(number));
#define ADDTEXT_100(InArgs, number) InArgs.Add(FText::AsNumber(number / 100)); InArgs.Add(INVTEXT(".")); InArgs.Add(FText::AsNumber((number % 100) / 10));
#define ADDTEXT_PERCENT(InArgs, number) InArgs.Add(FText::AsNumber(number)); InArgs.Add(INVTEXT("%"));

#define ADDTEXT_TAG_(Tag, InText) ADDTEXT_(INVTEXT("{0}{1}</>"), INVTEXT(Tag), InText)
#define ADDTEXT_TAGN_(Tag, InText) ADDTEXT_(INVTEXT("{0}{1}</>"), INVTEXT(Tag), InText)

#define ADDTEXT_NUM_(number) args.Add(FText::AsNumber(number));
#define ADDTEXT_100_(number) args.Add(FText::AsNumber(number / 100)); args.Add(INVTEXT(".")); args.Add(FText::AsNumber((number % 100) / 10));
#define ADDTEXT_INV_(InText) args.Add(INVTEXT(InText));
#define ADDTEXT_LOCTEXT(InKey, InText) args.Add(LOCTEXT(InKey, InText));

#define JOINTEXT(args) FText::Join(FText(), args)


static FText TextTag_IgnoreOtherTags(FText tag, FText baseText)
{
	FString baseString = baseText.ToString();
	for (int32 i = baseString.Len(); i-- > 0;) 
	{
		if (baseString[i] == L'>') {
			if (i + 1 < baseString.Len()) {
				baseString.InsertAt(i + 1, tag.ToString());
			} else {
				baseString.Append(tag.ToString());
			}
		}
		else if (baseString[i] == L'<') {
			if (i < baseString.Len()) {
				baseString.InsertAt(i, FString("</>"));
			} else {
				baseString.Append(FString("</>"));
			}
		}
	}
	return FText::FromString(baseString);
}
#define TEXT_TAG_IGNORE_OTHER_TAGS(Tag, InText) FText::Format(INVTEXT("{0}{1}</>"), INVTEXT(Tag), TextTag_IgnoreOtherTags(INVTEXT(Tag), InText))


// !!!Note: there is a problem with .EqualTo() so this is used for now
static bool TextEquals(const FText& a, const FText& b) {
	FString aStr = a.ToString();
	FString bStr = b.ToString();
	return aStr == bStr;
}

static bool TextArrayEquals(const TArray<FText>& a, const TArray<FText>& b)
{
	if (a.Num() != b.Num()) {
		return false;
	}
	for (int32 i = 0; i < a.Num(); i++) {
		if (!TextEquals(a[i], b[i])) {
			return false;
		}
	}
	return true;
}
static bool TextArrayEquals(const std::vector<FText>& a, const std::vector<FText>& b)
{
	if (a.size() != b.size()) {
		return false;
	}
	for (int32 i = 0; i < a.size(); i++) {
		if (!TextEquals(a[i], b[i])) {
			return false;
		}
	}
	return true;
}
static TArray<FString> TextArrayToStringArray(const TArray<FText>& texts)
{
	TArray<FString> results;
	for (const FText& text : texts) {
		results.Add(text.ToString());
	}
	return results;
}

//#define JOINTEXT(...) FText::Join(FText(), __VA_ARGS__);

static float Clamp01(float value) {
	return std::max(0.0f, std::min(1.0f, value));
}
static float Clamp(int32 value, int32 minValue, int32 maxValue) {
	return std::max(minValue, std::min(maxValue, value));
}

static int32 Clamp0_100(int32 value) {
	return std::max(0, std::min(100, value));
}

static int32 IncrementByPercent100(int32 value, int32 percent) {
	return value * (100 + percent) / 100;
}


static void DecreaseToZero(int32& value, int32 decrement = 1)
{
	value = std::max(value - decrement, 0);
}
static void IncreaseToZero(int32& value, int32 increment = 1)
{
	value = std::min(value + increment, 0);
}

static void ToLowerCase(std::string& str) {
	std::transform(str.begin(), str.end(), str.begin(), [](unsigned char c) { return std::tolower(c); });
}
static void ToLowerCase(std::wstring& str) {
	std::transform(str.begin(), str.end(), str.begin(), [](wchar_t c) { return std::tolower(c); });
}

static std::string ToSignedNumber(int32 value) {
	return (value > 0 ? "+" : "") + std::to_string(value);
}
static std::string ToForcedSignedNumber(int32 value) {
	return (value >= 0 ? "+" : "") + std::to_string(value);
}

static std::string ToSignedNumber(float value, int32 precision = 1) {
	std::stringstream ss;
	ss << std::fixed << std::setprecision(precision);
	ss << (value > 0 ? "+" : "") << value;
	return ss.str();
}

static std::string TextRed(std::string str, bool isRed)
{
	if (isRed) {
		str = "<Red>" + str + "</>";
	}
	return str;
}
static FText TextRed(FText str, bool isRed)
{
	if (isRed) {
		return TEXT_TAG("<Red>", str);
	}
	return str;
}

static FText TextRedOrange(FText text, int32 value, int32 orangeThreshold, int32 redThreshold)
{
	if (value < redThreshold) {
		return TEXT_TAG("<Red>", text);
	}
	if (value < orangeThreshold) {
		return TEXT_TAG("<Orange>", text);
	}
	return text;
}

static FText TextNumberColor(FText text, int32 value, int32 orangeThreshold, int32 redThreshold)
{
	if (value < redThreshold) {
		return TEXT_TAG("<FaintRed12>", text);
	}
	if (value < orangeThreshold) {
		return TEXT_TAG("<FaintOrange12>", text);
	}
	return TEXT_TAG("<FaintGreen12>", text);
}


static bool SearchBoxCompare(const std::string& searchString, const std::string& compare)
{
	for (int32 i = 0; i < searchString.size(); i++) {
		if (searchString[i] != compare[i]) {
			return false;
		}
	}
	return true;
}


/*
 * Open/Close systems
 */
#define MAIN_MENU_DISPLAY 1
#define MAIN_MENU_SOUND 1

#define DISPLAY_BUILDING 1
#define DISPLAY_MINIBUILDING 1
#define DISPLAY_UNIT 1
#define DISPLAY_TILEOBJ 1

#define DISPLAY_TERRAIN 1
#define DISPLAY_WORLDMAP 1
#define DISPLAY_WORLDMAP_BUILDING 1

#define DISPLAY_REGIONDECAL 1
#define DISPLAY_TERRITORY 1
#define DISPLAY_RESOURCE 1

#define AUDIO_ALL 1
#define AUDIO_BIRD 1

#define UI_ALL 1
#define UI_WORLDSPACE 1
#define UI_ESCMENU 1
#define UI_CHAT 1
#define UI_QUEST 1
#define UI_MAINGAME 1

/*
 * Debug Counter
 * Use: "PunTog DebugUI"
 * TIP: Test using AddAIImmigrants 500 (Chat Cheat) and full speed Ctrl-5
 * "PunSet ShowFullDebugLog 0" to ensure less logging
 */

#if DEBUG_BUILD

#define DEBUG_AI_VAR(VarName) \
{\
	static int32 _lastSec_##VarName = 0; \
	static int32 _lastSecCount_##VarName = 0; \
	if (Time::Seconds() > _lastSec_##VarName) { \
		_lastSec_##VarName = Time::Seconds(); \
		Debug_##VarName = _lastSecCount_##VarName; \
		_lastSecCount_##VarName = 0; \
	} \
	_lastSecCount_##VarName++;\
}

#define DECLARE_DEBUG_AI_VAR(VarName) int32 Debug_##VarName = 0;

DECLARE_DEBUG_AI_VAR(ResetCountPerSec);
DECLARE_DEBUG_AI_VAR(FailedToFindPath);
DECLARE_DEBUG_AI_VAR(FailedToFindPathLongDist);

DECLARE_DEBUG_AI_VAR(MoveRandomly);

DECLARE_DEBUG_AI_VAR(TryGatherTreeOnly_Succeed);
DECLARE_DEBUG_AI_VAR(TryGatherNotTreeOnly_Succeed);

DECLARE_DEBUG_AI_VAR(CalculateActions);
DECLARE_DEBUG_AI_VAR(TryCheckBadTile);
DECLARE_DEBUG_AI_VAR(TryStoreInventory);
DECLARE_DEBUG_AI_VAR(TryFindFood);
DECLARE_DEBUG_AI_VAR(TryHeatup);
DECLARE_DEBUG_AI_VAR(TryToolup);
DECLARE_DEBUG_AI_VAR(TryHealup);

DECLARE_DEBUG_AI_VAR(TryFillLuxuries);
DECLARE_DEBUG_AI_VAR(TryFun);
DECLARE_DEBUG_AI_VAR(TryConstructRoad_RoadMaker);
DECLARE_DEBUG_AI_VAR(TryConstruct_Constructor);
DECLARE_DEBUG_AI_VAR(TryGatherFruit);
DECLARE_DEBUG_AI_VAR(TryHunt);
DECLARE_DEBUG_AI_VAR(TryRanch);
DECLARE_DEBUG_AI_VAR(TryFarm);
DECLARE_DEBUG_AI_VAR(TryForesting);

DECLARE_DEBUG_AI_VAR(TryProduce_Special);
DECLARE_DEBUG_AI_VAR(TryProduce_Mine);
DECLARE_DEBUG_AI_VAR(TryProduce_Others);

DECLARE_DEBUG_AI_VAR(EmergencyTree10_TryMoveResourceAny);
DECLARE_DEBUG_AI_VAR(EmergencyTree0_TryMoveResourceAny);
DECLARE_DEBUG_AI_VAR(TryMoveResourcesAny_Drop);
DECLARE_DEBUG_AI_VAR(TryMoveResourcesAny_All10);
DECLARE_DEBUG_AI_VAR(TryMoveResourcesAny_All0);

DECLARE_DEBUG_AI_VAR(TryConstructRoad);
DECLARE_DEBUG_AI_VAR(TryConstructRoad_NotWorkConstructState);
DECLARE_DEBUG_AI_VAR(TryConstructRoad_Queued);
DECLARE_DEBUG_AI_VAR(TryConstructRoad_0Action);
DECLARE_DEBUG_AI_VAR(TryConstructRoad_MaxQueued);

DECLARE_DEBUG_AI_VAR(TryGoNearbyHome);

DECLARE_DEBUG_AI_VAR(WorldSpaceUICreate);
DECLARE_DEBUG_AI_VAR(AddWidget);
DECLARE_DEBUG_AI_VAR(AddToolTip);

// IsConnected
#define DEBUG_ISCONNECTED_VAR(VarName) DEBUG_AI_VAR(IsConnected_##VarName)
#define DECLARE_DEBUG_ISCONNECTED_VAR(VarName) DECLARE_DEBUG_AI_VAR(IsConnected_##VarName)

DECLARE_DEBUG_ISCONNECTED_VAR(ResourceMoveValid);
DECLARE_DEBUG_ISCONNECTED_VAR(IsMoveValid);
DECLARE_DEBUG_ISCONNECTED_VAR(MoveRandomlyPerlin);
DECLARE_DEBUG_ISCONNECTED_VAR(JustBruteIt);
DECLARE_DEBUG_ISCONNECTED_VAR(MoveRandomly);
DECLARE_DEBUG_ISCONNECTED_VAR(TryStockBurrowFood);
DECLARE_DEBUG_ISCONNECTED_VAR(TryGoNearbyHome);
DECLARE_DEBUG_ISCONNECTED_VAR(FindNearestUnreservedFullBush);
DECLARE_DEBUG_ISCONNECTED_VAR(GetProvinceRandomTile);
//DECLARE_DEBUG_ISCONNECTED_VAR(FindMarketResourceHolderInfo);
DECLARE_DEBUG_ISCONNECTED_VAR(RefreshIsBuildingConnected);
DECLARE_DEBUG_ISCONNECTED_VAR(ClaimProvince);
DECLARE_DEBUG_ISCONNECTED_VAR(RefreshHoverWarning);
DECLARE_DEBUG_ISCONNECTED_VAR(adjacentTileNearestTo);
DECLARE_DEBUG_ISCONNECTED_VAR(DropResourceSystem);

DECLARE_DEBUG_ISCONNECTED_VAR(IsConnectedBuildingResource);

#undef DECLARE_DEBUG_AI_VAR


#else

#define DEBUG_AI_VAR(VarName)
#define DEBUG_ISCONNECTED_VAR(VarName)

#endif

/*
 * LLM
 *
 * !!! turn on PUN_LLM_ON to get pun's stuff
 */
#include "HAL/LowLevelMemStats.h"
#include "HAL/LowLevelMemTracker.h"


#define STRINGIFY(x)  #x
#define TO_STR(x)  STRINGIFY(x)



#if UE_BUILD_SHIPPING
	#define LLM_SCOPE_(customTag)

#else
	 // LLM Declare
	#define DECLARE_LLM(TagName) DECLARE_LLM_MEMORY_STAT(TEXT(TO_STR(TagName)), STAT_##TagName, STATGROUP_LLMFULL);

	DECLARE_LLM(PUN_GameManager);
	DECLARE_LLM(PUN_Controller);
	DECLARE_LLM(PUN_GameInstance);
	DECLARE_LLM(PUN_Sound);

	DECLARE_LLM(PUN_DisplayBuilding);
	DECLARE_LLM(PUN_DisplayTileObj);
	DECLARE_LLM(PUN_DisplayUnit);
	DECLARE_LLM(PUN_DisplayTerrain);

	#undef DECLARE_LLM


	// LLM Enum
	enum class EPunSimLLMTag : LLM_TAG_TYPE
	{
		PUN_GameManager = static_cast<int32>(ELLMTag::ProjectTagStart) + 20,
		PUN_Controller,
		PUN_GameInstance,
		PUN_Sound,
		//PunSimulation,

		PUN_DisplayBuilding,
		PUN_DisplayTileObj,
		PUN_DisplayUnit,
		PUN_DisplayTerrain,
	};

	#define LLM_SCOPE_(customTag) LLM_SCOPE(static_cast<ELLMTag>(customTag));

#endif

// LLM Regiser
static void InitLLM()
{
#if UE_BUILD_SHIPPING
	#define REGISTER_LLM(TagName)
#else
	#define REGISTER_LLM(TagName) FLowLevelMemTracker::Get().RegisterProjectTag(static_cast<int32>(EPunSimLLMTag::TagName), TEXT(TO_STR(TagName)), GET_STATFNAME(STAT_##TagName), NAME_None);
#endif
	REGISTER_LLM(PUN_GameManager);
	REGISTER_LLM(PUN_Controller);
	REGISTER_LLM(PUN_GameInstance);
	REGISTER_LLM(PUN_Sound);

	REGISTER_LLM(PUN_DisplayBuilding);
	REGISTER_LLM(PUN_DisplayTileObj);
	REGISTER_LLM(PUN_DisplayUnit);
	REGISTER_LLM(PUN_DisplayTerrain);
	
#undef REGISTER_LLM
}


/**
 * Time
 */

static const int32 GameTicksPerNetworkTicks = 15;

class TimeDisplay
{
public:
	static void SetTicks(int32_t tick) {
		_Ticks = tick;
	}

	static int32_t Ticks() { return _Ticks; }

private:
	static int32_t _Ticks;
};

class Time
{
public:
	// dwarfFortress 40 minutes year, 100 fps -> 40 * 60 / 100 = 24 minutes ??? wrong? 1 hr = 1 year?
	static const int32 TicksPerSecond = 60;
	static const int32 SecondsPerMinute = 60;
	static const int32 MinutesPerSeason = 5;
	static const int32 SeasonsPerYear = 4;

	static const int32 TicksPerMinute = TicksPerSecond * SecondsPerMinute;
	static const int32 TicksPerSeason = TicksPerMinute * MinutesPerSeason;
	static const int32 TicksPerYear = TicksPerSeason * SeasonsPerYear;

	static const int32 MinutesPerYear = MinutesPerSeason * SeasonsPerYear;
	static const int32 SecondsPerYear = SecondsPerMinute * MinutesPerSeason * SeasonsPerYear;

	// Round
	static const int32 SecondsPerRound = 150;
	static const int32 TicksPerRound = TicksPerSecond * SecondsPerRound;
	static const int32 RoundsPerYear = SecondsPerYear / SecondsPerRound;

	static void ResetTicks();
	static void SetTickCount(int32 tickCount);

	static int32 Ticks() { return _Ticks; }
	static int32 Seconds() { return _Seconds; }
	static int32 Minutes() { return _Minutes; }
	static int32 Rounds() { return _Rounds; }
	static int32 Seasons() { return _Seasons; }
	static int32 Years() { return _Years; }

	static int32 Minutes(int32 ticks) { return ticks / TicksPerMinute; }
	static int32 Seasons(int32 ticks) { return ticks / TicksPerSeason; }
	static int32 Years(int32 ticks) { return ticks / TicksPerYear; }

	static bool IsSpringStart() { return Time::Ticks() % Time::TicksPerYear == 0; }
	static bool IsSpringMid() { return Time::Ticks() % Time::TicksPerYear == TicksPerSeason / 2; }
	static bool IsSummerStart() { return Time::Ticks() % Time::TicksPerYear == TicksPerSeason; }
	static bool IsAutumnStart() { return Time::Ticks() % Time::TicksPerYear == TicksPerSeason * 2; }
	static bool IsAutumnMid() { return Time::Ticks() % Time::TicksPerYear == (TicksPerSeason * 2 + TicksPerSeason / 2); }

	// Start farming only early spring to mid summer (round 1 summer)
	static bool IsValidFarmBeginTime() {
		return (Ticks() % TicksPerYear) < TicksPerSeason * 3 / 2;
	}

	static int32 SeasonMod() { return _Seasons % 4; }
	static bool IsSeasonStart() { return Time::Ticks() % Time::TicksPerSeason == 0; }

	static bool IsSpring() { return SeasonMod() == 0; }
	static bool IsSummer() { return SeasonMod() == 1; }
	static bool IsAutumn() { return SeasonMod() == 2; }
	static bool IsWinter() { return SeasonMod() == 3; }

	static float SeasonFraction() {
		return static_cast<float>(Time::Ticks() % Time::TicksPerSeason) / Time::TicksPerSeason;
	}
	static int32 SeasonPercent() {
		return (Ticks() % TicksPerSeason) * 100 / TicksPerSeason;
	}

	//static bool IsAlmostWinter() { return Time::Ticks() % Time::TicksPerYear >= TicksPerSeason * 5 / 2; }

#define LOCTEXT_NAMESPACE "SeasonName"
	static FText SeasonName(int32 season) {
		switch (season % 4) {
		case 0: return LOCTEXT("Spring", "Spring");
		case 1: return LOCTEXT("Summer", "Summer");
		case 2: return LOCTEXT("Autumn", "Autumn");
		case 3: return LOCTEXT("Winter", "Winter");
		}
		return FText();
	}

	static FText SeasonPrefix(int32 ticks)
	{
		int32_t minutesIntoSeason = (ticks / TicksPerMinute) - Seasons(ticks) * MinutesPerSeason;

		auto formatSpace = [&](FText text) {
			return FText::Format(INVTEXT("{0} "), text);
		};
		
		if (minutesIntoSeason >= 4) return formatSpace(LOCTEXT("Late", "Late")); 
		if (minutesIntoSeason >= 2) return formatSpace(LOCTEXT("Mid", "Mid"));
		return formatSpace(LOCTEXT("Early", "Early"));
	}
#undef LOCTEXT_NAMESPACE
	

	static int32 GraphBeginSeason() {
		return (Time::Seasons() + 1) % 4;
	}

	static FloatDet MinCelsiusBase() { return _MinCelsiusBase; }
	static FloatDet MaxCelsiusBase() { return _MaxCelsiusBase; }

	static FloatDet ColdCelsius() { return IntToFD(0); }

	static FloatDet CelsiusHelper(FloatDet minCelsius, FloatDet maxCelsius)
	{
		if (PunSettings::IsOn("ForceSnow")) { 
			return minCelsius;
		}
		
		int32 yearTick = _Ticks % Time::TicksPerYear;

		// NOTE!!!: One of the few places to use double.... Might not be good for determinism
		double normalizedWeather = sin(2.0 * PI * yearTick / Time::TicksPerYear - PI / 4.0) * 0.5 + 0.5;

		//if (Time::Ticks() % 60 == 0) {
		//	UE_LOG(LogTemp, Error, TEXT("normalizedWeather %f, weatherFD:%d, min-maxCelFD:%d, %d"), normalizedWeather, FloatToFD(normalizedWeather), _MaxCelsius - _MinCelsius, FDSafeMul(FloatToFD(normalizedWeather), _MaxCelsius - _MinCelsius));
		//}

		return FDSafeMul(FloatToFD(normalizedWeather), maxCelsius - minCelsius) + minCelsius; // PI/4 shift is to make mid summer hottest and mid winter coldest
	}

	static bool IsSnowing() { return CelsiusHelper(_MinCelsiusBase, _MaxCelsiusBase) < FD0_X(-45); } // Snow when lower than -4.5 C so it doesn't snow in spring

	static bool IsRaining()
	{
		if (PunSettings::IsOn("ForceNoRain")) {
			return false;
		}
		if (PunSettings::IsOn("ToggleRain")) {
			return true;
		}
		if (PunSettings::TrailerMode()) {
			return false;
		}
		
		if (IsWinter() || IsAutumn() || Ticks() < TicksPerMinute) { // Do not start rain until at least 1 min into the game
			return false;
		}

		// Rain around 1/4 of the time in spring/summer
		int32_t rainLengthTicks = TicksPerRound;
		int32_t rainStartRangeTicks = TicksPerSeason * 2 - rainLengthTicks; // rain can starts within 3/4 of spring+summer (start after and it won't be full rain)
		int32_t rainStartTick = GameRand::Rand(Years() + 1000) % rainStartRangeTicks; // rain can start anywhere from 0 - rainStartRangeTicks

		int32 tickWithinYear = Ticks() % TicksPerYear;
		return rainStartTick <= tickWithinYear && tickWithinYear <= rainStartTick + rainLengthTicks;
	}
	static bool IsFogging()
	{
		if (Ticks() < TicksPerMinute) { // Do not start fog until at least 1 min into the game
			return false;
		}

		// Rain around 1/4 of the time in spring/summer
		int32_t lengthTicks = TicksPerRound / 2;
		int32_t startRangeTicks = TicksPerSeason * 2 - lengthTicks; // rain can starts within 3/4 of spring+summer (start after and it won't be full rain)
		int32_t startTick = GameRand::Rand(Years() + 10000) % startRangeTicks; // rain can start anywhere from 0 - rainStartRangeTicks

		return startTick <= Ticks() && Ticks() <= startTick + lengthTicks;
	}

	static float kForcedFallSeason;
	
	static float FallSeason() // 0 before fall and 1 after fall
	{
#if TRAILER_MODE
		kForcedFallSeason = Clamp01(kForcedFallSeason + (PunSettings::IsOn("ForceAutumn") ? 1 : -1) * 1.0f / (PunSettings::Get("ForceAutumnTicks"))); // change in X*2 sec
		return kForcedFallSeason;
#endif
		
		const float fallSeasonFullFraction = 0.3;
		int32_t fallProgressionTicks = Ticks() % TicksPerYear - TicksPerSeason * 2;
		float fallProgression = static_cast<float>(fallProgressionTicks) / TicksPerSeason / fallSeasonFullFraction;
		return Clamp01(fallProgression);
	}
	static float SpringSeason() { // 1 before spring and 1 after fall
		const float seasonFullFraction = 0.5;
		float springProgressionUp = Clamp01(static_cast<float>(Ticks() % TicksPerYear - TicksPerSeason * 7 / 2) / TicksPerSeason / seasonFullFraction); // start mid winter...
		float springProgressionDown = 1.0f - Clamp01(static_cast<float>(Ticks() % TicksPerYear - TicksPerSeason / 2) / TicksPerSeason / seasonFullFraction); // start mid spring...
		return Clamp01(springProgressionUp * springProgressionDown);
	}
	
	static float FloweringIntensity() {
		const float springSeasonFullFraction = 0.3;
		int32_t springProgressionTicks = Ticks() % TicksPerYear;
		float floweringBegin = float(springProgressionTicks) / TicksPerSeason / springSeasonFullFraction;
		float floweringEnd = float(TicksPerSeason - springProgressionTicks) / TicksPerSeason / springSeasonFullFraction;
		float flowering = std::min(floweringBegin, floweringEnd);
		return std::max(0.0f, std::min(1.0f, flowering));
	}

	static void Serialize(FArchive& Ar, TArray<uint8>& data)
	{
		Ar << _Ticks;
		Ar << _Seconds;
		Ar << _Minutes;
		Ar << _Rounds;
		Ar << _Seasons;
		Ar << _Years;
		
		Ar << _MinCelsiusBase << _MaxCelsiusBase;

		//if (Ar.IsLoading()) {
		//	_TicksLoaded = _Ticks;
		//}
	}

private:
	static int32 _Ticks;
	static int32 _Seconds;
	static int32 _Minutes;
	static int32 _Rounds;
	static int32 _Seasons;
	static int32 _Years;

	static FloatDet _MinCelsiusBase, _MaxCelsiusBase;

	//static int32 _TicksLoaded = 0; // TODO: Not used yetc
};

static FloatDet CelsiusToFahrenheit(FloatDet celsius)
{
	return 	celsius * 9 / 5 + 32 * FDOne;
}

/*
 * Time Constants
 */
static const int32 TicksPerStatInterval = Time::TicksPerSecond * 30;


/*
 * Constants
 */
const int32 MaxPacketSize = 1024;

const int32 MaxPacketSize32 = MaxPacketSize / 4;


const int32 ConstructionWaitTicks = Time::TicksPerSecond / 4; //15;
const int32 ConstructTimesPerBatch = 20 * 4; // 20 sec


/*
 * Map
 */

#define LOCTEXT_NAMESPACE "MapDropdowns"

enum class MapSizeEnum : uint8
{
	Small,
	Medium,
	Large,
};
static const TArray<FText> MapSizeNames
{
	LOCTEXT("Small", "Small"),
	LOCTEXT("Medium", "Medium"),
	LOCTEXT("Large", "Large\u200B"),
	//LOCTEXT("Large", "Large"),
};

static const std::vector<WorldRegion2> MapSizes
{
	WorldRegion2(32, 64),
	WorldRegion2(64, 128),
	WorldRegion2(128, 256),
};

static WorldRegion2 GetMapSize(int32 mapSizeEnumInt) {
	return MapSizes[mapSizeEnumInt];
}
static WorldRegion2 GetMapSize(MapSizeEnum mapSizeEnum) {
	return MapSizes[static_cast<int>(mapSizeEnum)];
}

static MapSizeEnum GetMapSizeEnumFromString(const FString& str) {
	for (size_t i = 0; i < MapSizeNames.Num(); i++) {
		if (MapSizeNames[i].ToString() == str) {
			return static_cast<MapSizeEnum>(i);
		}
	}
	UE_DEBUG_BREAK();
	return MapSizeEnum::Medium;
}

template <typename TEnum>
static TEnum GetEnumFromName(const FString& str, const std::vector<FString>& names) {
	for (size_t i = 0; i < names.size(); i++) {
		if (names[i] == str) {
			return static_cast<TEnum>(i);
		}
	}
	UE_DEBUG_BREAK();
	return static_cast<TEnum>(0);
}
template <typename TEnum>
static TEnum GetEnumFromName(const FString& str, const std::vector<FText>& names) {
	for (size_t i = 0; i < names.size(); i++) {
		if (names[i].ToString() == str) {
			return static_cast<TEnum>(i);
		}
	}
	UE_DEBUG_BREAK();
	return static_cast<TEnum>(0);
}

enum class MapSeaLevelEnum : uint8
{
	VeryLow,
	Low,
	Medium,
	High,
	VeryHigh
};

static const std::vector<FText> MapSettingsLevelNames
{
	LOCTEXT("Very Low", "Very Low"),
	LOCTEXT("Low", "Low"),
	LOCTEXT("Medium", "Medium"),
	LOCTEXT("High", "High"),
	LOCTEXT("Very High", "Very High"),
};

enum class MapMoistureEnum : uint8
{
	VeryLow,
	Low,
	Medium,
	High,
	VeryHigh
};
static const std::vector<FText> MapMoistureNames
{
	LOCTEXT("Very Dry", "Very Dry"),
	LOCTEXT("Dry", "Dry"),
	LOCTEXT("Medium", "Medium"),
	LOCTEXT("Wet", "Wet"),
	LOCTEXT("Very Wet", "Very Wet"),
};

enum class MapTemperatureEnum : uint8
{
	VeryLow,
	Low,
	Medium,
	High,
	VeryHigh
};
enum class MapMountainDensityEnum : uint8
{
	VeryLow,
	Low,
	Medium,
	High,
	VeryHigh
};

enum class DifficultyLevel : uint8
{
	Easy,
	Normal,
	Hard,
	Brutal,
	//King,
	Emperor,
	Immortal,
	Deity,
};
static const std::vector<FText> DifficultyLevelNames
{
	LOCTEXT("Easy", "Easy"),
	LOCTEXT("Normal", "Normal"),
	LOCTEXT("Hard", "Hard"),
	LOCTEXT("Brutal", "Brutal"),
	//LOCTEXT("King", "King"),
	LOCTEXT("Emperor", "Emperor"),
	LOCTEXT("Immortal", "Immortal"),
	LOCTEXT("Deity", "Deity"),
};
static const std::vector<int32> DifficultyConsumptionAdjustment
{
	0,
	30,
	70,
	110,
	160,
	230,
	300,
};

#undef LOCTEXT_NAMESPACE

static DifficultyLevel GetDifficultyLevelFromString(const FString& str) {
	for (size_t i = 0; i < DifficultyLevelNames.size(); i++) {
		if (DifficultyLevelNames[i].ToString() == str) {
			return static_cast<DifficultyLevel>(i);
		}
	}
	UE_DEBUG_BREAK();
	return DifficultyLevel::Normal;
}

/**
 * Resources
 */

enum class ResourceEnum : uint8
{
	Wood,
	Stone,

	Orange,
	Papaya,

	//Berries,
	Wheat,
	Milk,
	Mushroom,
	Hay,

	Paper,
	Clay,
	Brick,

	Coal,
	IronOre,
	Iron,
	Furniture,
	Chocolate,

	SteelTools,
	Herb,
	Medicine,

	Fish,

	//WhaleMeat,
	Grape,
	Wine,
	MagicMushroom,

	Pork,
	GameMeat,
	Beef,
	Lamb,
	Cocoa,

	Wool,
	Leather,
	Cloth,
	GoldOre,
	GoldBar,

	Beer,
	//Barley,
	//Oyster,
	Cannabis,
	//Truffle,
	//Coconut,
	Cabbage,

	Pottery,

	Flour,
	Bread,
	Gemstone,
	Jewelry,

	// Added June 9

	Cotton,
	CottonFabric,
	DyedCottonFabric,
	LuxuriousClothes,

	Honey,
	Beeswax,
	Candle,

	Dye,
	Book,

	Coconut,

	// Added Dec 26
	Potato,
	Blueberries,
	Melon,
	Pumpkin,
	RawCoffee,
	Tulip,

	Coffee,
	Vodka,

	// Added Apr 9
	StoneTools,
	Sand,
	Oil,
	
	Glass,
	Concrete,
	Steel,

	Glassware,
	PocketWatch,

	// Added Oct 2
	PitaBread,
	Carpet,
	DateFruit,
	ToiletPaper,

	// --- End
	None,

	Money,
	Food,
	Luxury,
	Fuel,

	Influence,
};

struct ResourceInfo
{
	// Need std::string since we need to copy when passing into FName (allows using name.c_str() instead of FName in some cases)
	ResourceEnum resourceEnum;
	FText name;
	int32 basePrice;
	FText description;

	std::string nameStd() { return FTextToStd(name); }
	FText GetName() const { return name; }
	FText GetDescription() const { return description; }

	ResourceInfo(ResourceEnum resourceEnum, FText name, int32 cost, FText description)
		: resourceEnum(resourceEnum), name(name), basePrice(cost), description(description) {}

	int32 resourceEnumInt() { return static_cast<int32>(resourceEnum); }
	int32 basePrice100() { return basePrice * 100; }
};

//
// BALANCE
// human consume 100 food per year, 100 food is roughly 300 coins, 1 year is 20 mins or 1200 sec..
//
static const int32 HumanFoodPerYear = 24; // 45 (nov 19) // 35 // 70
static const int32 FoodCost = 5; // 3 (nov 19)
static const int32 BaseHumanFoodCost100PerYear = 100 * HumanFoodPerYear * FoodCost;

/*
 * HumanFoodCostPerYear affects:
 * - Industrial
 * - Hunting/Gathering through AssumedFoodProductionPerYear
 * Note: Farm is affected by this
 */
static const int32 WoodGatherYield_Base100 = 250;
static const int32 FarmBaseYield100 = 200; // 250 (nov 19)
static const int32 StoneGatherYield_Base = 4;

static const int32 CutTreeTicksBase = Time::TicksPerSecond * 10;
static const int32 HarvestDepositTicksBase = Time::TicksPerSecond * 14;

// How fast people produce value when working compare to value spent on food
// This is high because people don't spend all their time working.
//!!! Change Base Production (Except FarmBaseYield100) !!!!
static const int32 WorkRevenueToCost_Base = 150; // 150 -> 128 (Mar 26)... -> 150 (May 10)

// TODO: WorkRevenueToCost is not yet affected by resource modifiers...

static const int32 WorkRevenue100PerYear_perMan_Base = BaseHumanFoodCost100PerYear * WorkRevenueToCost_Base / 100;
// This is the same as WorkRevenuePerManSec100_Base
static const int32 WorkRevenue100PerSec_perMan_Base = WorkRevenue100PerYear_perMan_Base / Time::SecondsPerYear;

static const int32 AssumedFoodProduction100PerYear = WorkRevenue100PerYear_perMan_Base / FoodCost;

// How much a person spend on each type of luxury per year. ... /4 comes from testing... it makes houselvl 2 gives x2 income compare to house lvl 1
static const int32 HumanLuxuryCost100PerYear_ForEachType = BaseHumanFoodCost100PerYear / 8;
static const int32 HumanLuxuryCost100PerRound_ForEachType = HumanLuxuryCost100PerYear_ForEachType / Time::RoundsPerYear;


static const int32 BuildManSecCostFactor100 = 10; // Building work time is x% of the time it takes to acquire the resources


// Electricity
// - Electricity half worker usage. Existing worker works 100% faster
// - Electricity consumption 1 kW = 1  person
// - 135 food value per year = 20 coal per year, 10 coal per year ~ 1 Coal per Round
// - 1 kW = 1 coal burn per round

// High supply quantity = elastic demand
static const int32 DefaultYearlySupplyPerPlayer = 10000;
static const int32 MaxGoodsPricePercent = 300; // Max price is when round supply goes to 0
static const int32 MinGoodsPricePercent = 30;

#define LOCTEXT_NAMESPACE "ResourceInfo"

static const ResourceInfo ResourceInfos[]
{
	ResourceInfo(ResourceEnum::Wood,		LOCTEXT("Wood", "Wood"),		6, LOCTEXT("Wood Desc", "Construction material and fuel for house furnaces")),
	ResourceInfo(ResourceEnum::Stone,		LOCTEXT("Stone", "Stone"),	7,  LOCTEXT("Stone Desc", "Construction material mined from Quarry or Stone Outcrop")),
	
	ResourceInfo(ResourceEnum::Orange,		LOCTEXT("Orange", "Orange"),	FoodCost, LOCTEXT("Orange Desc", "Edible, tangy fruit obtained from Forest and Orchards")),
	ResourceInfo(ResourceEnum::Papaya,		LOCTEXT("Papaya", "Papaya"),	FoodCost, LOCTEXT("Papaya Desc", "Sweet, tropical fruit obtained from Jungle and Orchards")),
	
	ResourceInfo(ResourceEnum::Wheat,		LOCTEXT("Wheat", "Wheat"),	FoodCost, LOCTEXT("Wheat Desc", "Edible grain that can be brewed into Beer")),
	ResourceInfo(ResourceEnum::Milk,		LOCTEXT("Milk", "Milk"),		FoodCost, LOCTEXT("Milk Desc", "Yummy white liquid from cows")),
	ResourceInfo(ResourceEnum::Mushroom,	LOCTEXT("Mushroom", "Mushroom"),	FoodCost, LOCTEXT("Mushroom Desc", "Delicious, earthy tasting fungus")),
	ResourceInfo(ResourceEnum::Hay,			LOCTEXT("Hay", "Hay"),		1, LOCTEXT("Hay Desc", "Dried grass that can be used as animal feed")),

	ResourceInfo(ResourceEnum::Paper,		LOCTEXT("Paper", "Paper"),	18, LOCTEXT("Paper Desc", "Used for research and book making")),
	ResourceInfo(ResourceEnum::Clay,		LOCTEXT("Clay", "Clay"),		3, LOCTEXT("Clay Desc", "Fine-grained earth used to make Pottery and Bricks")),
	ResourceInfo(ResourceEnum::Brick,		LOCTEXT("Brick", "Brick"),	12, LOCTEXT("Brick Desc", "Sturdy, versatile construction material")),

	ResourceInfo(ResourceEnum::Coal,		LOCTEXT("Coal", "Coal"),		7, LOCTEXT("Coal Desc", "Fuel used to heat houses or smelt ores. When heating houses, provides x2 heat vs. Wood")),
	ResourceInfo(ResourceEnum::IronOre,		LOCTEXT("Iron Ore", "Iron Ore"),		6, LOCTEXT("Iron Ore Desc", "Valuable ore that can be smelted into Iron Bar")),
	ResourceInfo(ResourceEnum::Iron,		LOCTEXT("Iron Bar", "Iron Bar"),		18, LOCTEXT("Iron Bar Desc", "Sturdy bar of metal used in construction and tool-making.")),
	ResourceInfo(ResourceEnum::Furniture,	LOCTEXT("Furniture", "Furniture"), 15,  LOCTEXT("Furniture Desc", "Luxury tier 1 used for housing upgrade. Make house a home.")),
	ResourceInfo(ResourceEnum::Chocolate,	LOCTEXT("Chocolate", "Chocolate"), 30,   LOCTEXT("Chocolate Desc", "Everyone's favorite confectionary. (Luxury tier 3)")),

	//ResourceInfo(ResourceEnum::CrudeIronTools,	"Crude Iron Tools",	15,  "Medium-grade tool made by Blacksmith using Iron Ore and Wood."),
	ResourceInfo(ResourceEnum::SteelTools,		LOCTEXT("Steel Tools", "Steel Tools"),			27,  LOCTEXT("Steel Tool Desc", "High-grade tool made by Blacksmith from Iron Bars and Wood")),
	ResourceInfo(ResourceEnum::Herb,				LOCTEXT("Medicinal Herb", "Medicinal Herb"),				6, 	LOCTEXT("Medicinal Herb Desc", "Medicinal plant used to heal sickness")),
	ResourceInfo(ResourceEnum::Medicine,			LOCTEXT("Medicine", "Medicine"),			12,  LOCTEXT("Medicine Desc", "Potent Medicinal Herb extract used to heal sickness")),
	
	
	//ResourceInfo(ResourceEnum::Tools,		"Tools", 25, 100, "Construction material"),
	ResourceInfo(ResourceEnum::Fish,			LOCTEXT("Fish", "Fish"),		FoodCost, LOCTEXT("Fish Desc", "Tasty catch from the sea/river")), // Fish is all year round... shouldn't be very high yield...

	//ResourceInfo(ResourceEnum::WhaleMeat, "WhaleMeat", 7, 100, "Luxury food obtained from Fishing Lodge"),
	ResourceInfo(ResourceEnum::Grape,		LOCTEXT("Grape", "Grape"),			FoodCost, LOCTEXT("Grape Desc", "Juicy, delicious fruit used in Wine-making.")),
	ResourceInfo(ResourceEnum::Wine,		LOCTEXT("Wine", "Wine"),			35, LOCTEXT("Wine Desc", "Luxury tier 2 used for housing upgrade. Alcoholic drink that makes everything tastes better.")),
	ResourceInfo(ResourceEnum::MagicMushroom,		LOCTEXT("Magic Mushroom", "Magic Mushroom"),	20,		LOCTEXT("Magic Mushroom Desc", "Psychedelic mushroom that can bring you on a hallucination trip. (Luxury tier 2)")),

	ResourceInfo(ResourceEnum::Pork,			LOCTEXT("Pork", "Pork"),		FoodCost, LOCTEXT("Pork Desc", "Delicious meat from farmed Pigs")),
	ResourceInfo(ResourceEnum::GameMeat,		LOCTEXT("Game Meat", "Game Meat"), FoodCost,  LOCTEXT("Game Meat Desc", "Delicious meat from wild animals")),
	ResourceInfo(ResourceEnum::Beef,			LOCTEXT("Beef", "Beef"),		FoodCost,  LOCTEXT("Beef Desc", "Delicious meat from ranched Cattle")),
	ResourceInfo(ResourceEnum::Lamb,			LOCTEXT("Lamb", "Lamb"),		FoodCost, LOCTEXT("Lamb Desc", "Delicious meat from ranched Sheep")),
	ResourceInfo(ResourceEnum::Cocoa,		LOCTEXT("Cocoa", "Cocoa"),		FoodCost + 4, LOCTEXT("Cocoa Desc", "Raw cocoa used in Chocolate-making")),

	ResourceInfo(ResourceEnum::Wool,			LOCTEXT("Wool", "Wool"),	7, LOCTEXT("Wool Desc", "Fine, soft fiber used to make Clothes")),
	ResourceInfo(ResourceEnum::Leather,		LOCTEXT("Leather", "Leather"), 6, LOCTEXT("Leather Desc", "Animal skin that can be used to make Clothes")),
	ResourceInfo(ResourceEnum::Cloth,		LOCTEXT("Clothes", "Clothes"), 30, LOCTEXT("Clothes Desc", "Luxury tier 2 used for housing upgrade. Provide cover and comfort.")),

	ResourceInfo(ResourceEnum::GoldOre,		LOCTEXT("Gold Ore", "Gold Ore"), 14, LOCTEXT("Gold Ore Desc", "Precious ore that can be smelted into Gold Bar")),
	ResourceInfo(ResourceEnum::GoldBar,		LOCTEXT("Gold Bar", "Gold Bar"), 30, LOCTEXT("Gold Bar Desc", "Precious metal that can be minted into money or crafted into Jewelry")),

	ResourceInfo(ResourceEnum::Beer,			LOCTEXT("Beer", "Beer"), 12, LOCTEXT("Beer Desc", "Luxury tier 1 used for housing upgrade. The cause and solution to all life's problems.")),
	//ResourceInfo(ResourceEnum::Barley,		"Barley", FoodCost, 100, "Edible grain, obtained from farming. Ideal for brewing Beer"),
	//ResourceInfo(ResourceEnum::Oyster,		"Oyster", 7, IndustryTuneFactor + 50, "A delicacy from the Sea"),
	ResourceInfo(ResourceEnum::Cannabis,		LOCTEXT("Cannabis", "Cannabis"), 8, LOCTEXT("Cannabis Desc", "Luxury tier 1 used for housing upgrade.")),
	//ResourceInfo(ResourceEnum::Truffle,		"Truffle", 7, IndustryTuneFactor + 50, "Construction material"),
	//ResourceInfo(ResourceEnum::Coconut,		"Coconut", 7, IndustryTuneFactor + 50, "Hard shell fruit with sweet white meat and delicious juice"),
	ResourceInfo(ResourceEnum::Cabbage,		LOCTEXT("Cabbage", "Cabbage"), FoodCost, LOCTEXT("Cabbage Desc", "Healthy green vegetable.")),

	ResourceInfo(ResourceEnum::Pottery,		LOCTEXT("Pottery", "Pottery"), 12, LOCTEXT("Pottery Desc", "Luxury tier 1 used for housing upgrade. Versatile pieces of earthenware.")),

	ResourceInfo(ResourceEnum::Flour,		LOCTEXT("Wheat Flour", "Wheat Flour"),	9, LOCTEXT("Wheat Flour Desc", "Ingredient used to bake Bread")),
	ResourceInfo(ResourceEnum::Bread,		LOCTEXT("Bread", "Bread"),			FoodCost, LOCTEXT("Bread Desc", "Delicious food baked from Wheat Flour")), // 3 bread from 1 flour
	ResourceInfo(ResourceEnum::Gemstone,	LOCTEXT("Gemstone", "Gemstone"),	20, LOCTEXT("Gemstone Desc", "Precious stone that can be crafted into Jewelry")),
	ResourceInfo(ResourceEnum::Jewelry,		LOCTEXT("Jewelry", "Jewelry"),		70, LOCTEXT("Jewelry Desc", "Luxury tier 3 used for housing upgrade. Expensive adornment of Gold and Gems.")),

	// June 9
	ResourceInfo(ResourceEnum::Cotton,				LOCTEXT("Cotton", "Cotton"),				7, LOCTEXT("Cotton Desc", "Raw material used to make Cotton Fabric.")),
	ResourceInfo(ResourceEnum::CottonFabric,		LOCTEXT("Cotton Fabric", "Cotton Fabric"), 23, LOCTEXT("Cotton Fabric Desc", "Fabric used by tailors to make Clothes.")),
	ResourceInfo(ResourceEnum::DyedCottonFabric,	LOCTEXT("Dyed Cotton Fabric", "Dyed Cotton Fabric"), 43, LOCTEXT("Dyed Cotton Fabric Desc", "Fancy fabric used by tailors to make Fashionable Clothes.")),
	ResourceInfo(ResourceEnum::LuxuriousClothes,	LOCTEXT("Fashionable Clothes", "Fashionable Clothes"), 50, LOCTEXT("Fashionable Clothes Desc", "Luxury tier 3 used for housing upgrade. Requires High Fashion")),
	
	ResourceInfo(ResourceEnum::Honey,		LOCTEXT("Honey", "Honey"), FoodCost, LOCTEXT("Honey Desc", "Delicious, viscous liquid produced by bees.")),
	ResourceInfo(ResourceEnum::Beeswax,		LOCTEXT("Beeswax", "Beeswax"), 10,	LOCTEXT("Beeswax Desc", "Raw material used to make Candles.")),
	ResourceInfo(ResourceEnum::Candle,		LOCTEXT("Candles", "Candles"), 20,	LOCTEXT("Candles Desc", "Luxury tier 2 used for housing upgrade.")),
	
	ResourceInfo(ResourceEnum::Dye,			LOCTEXT("Dye", "Dye"), 9, LOCTEXT("Dye Desc", "Colored substance used for printing or dyeing clothes.")),
	ResourceInfo(ResourceEnum::Book,		LOCTEXT("Book", "Book"), 50, LOCTEXT("Book Desc", "Luxury tier 3 used for housing upgrade.")),

	// Oct 26
	ResourceInfo(ResourceEnum::Coconut,		LOCTEXT("Coconut", "Coconut"),	FoodCost, LOCTEXT("Coconut Desc", "Large delicious fruit with white meat and refreshing juice.")),

	// Dec 17
	ResourceInfo(ResourceEnum::Potato,		LOCTEXT("Potato", "Potato"),	FoodCost, LOCTEXT("Potato Desc", "Common tuber. Can be consumed as Food or brewed into Vodka.")),
	ResourceInfo(ResourceEnum::Blueberries,	LOCTEXT("Blueberries", "Blueberries"),	FoodCost, LOCTEXT("Blueberries Desc", "Blue-skinned fruit with refreshing taste.")),
	ResourceInfo(ResourceEnum::Melon,		LOCTEXT("Melon", "Melon"),			FoodCost + 5, LOCTEXT("Melon Desc", "Sweet and refreshing fruit. +5<img id=\"Coin\"/> each unit when consumed.")),
	ResourceInfo(ResourceEnum::Pumpkin,		LOCTEXT("Pumpkin", "Pumpkin"),		FoodCost, LOCTEXT("Pumpkin Desc", "Fruit with delicate, mildly-flavored flesh.")),
	ResourceInfo(ResourceEnum::RawCoffee,	LOCTEXT("Raw Coffee", "Raw Coffee"),	FoodCost + 4, LOCTEXT("Raw Coffee Desc", "Fruit that can be roasted to make Coffee.")),
	ResourceInfo(ResourceEnum::Tulip,		LOCTEXT("Tulip", "Tulip"),				FoodCost + 3, LOCTEXT("Tulip Desc", "Beautiful decorative flower. (Luxury tier 1)")),

	ResourceInfo(ResourceEnum::Coffee,		LOCTEXT("Coffee", "Coffee"),	25, LOCTEXT("Coffee Desc", "Keeps you awake. (Luxury tier 2)")), // +5<img id=\"Science\"/> each unit when consumed.
	ResourceInfo(ResourceEnum::Vodka,		LOCTEXT("Vodka", "Vodka"),		28, LOCTEXT("Vodka Desc", "Clear alcoholic beverage made from Potato. (Luxury tier 2)")),

	// Apr 9
	ResourceInfo(ResourceEnum::StoneTools,	LOCTEXT("Stone Tools", "Stone Tools"),	10, LOCTEXT("Stone Tools Desc", "Low-grade Tools made by Stone Tool Shop.")),
	ResourceInfo(ResourceEnum::Sand,		LOCTEXT("Sand", "Sand"),	6, LOCTEXT("Sand Desc", "Raw material for producing Glass.")),
	ResourceInfo(ResourceEnum::Oil,			LOCTEXT("Oil", "Oil"),	12, LOCTEXT("Oil Desc", "Fuel used to produce electricity at Oil Power Plant.")),
	
	ResourceInfo(ResourceEnum::Glass,		LOCTEXT("Glass", "Glass"),	30, LOCTEXT("Glass Desc", "Transparent construction material made from Sand")),
	ResourceInfo(ResourceEnum::Concrete,	LOCTEXT("Concrete", "Concrete"),	30, LOCTEXT("Concrete Desc", "Sturdy, versatile construction material")),
	ResourceInfo(ResourceEnum::Steel,	LOCTEXT("Steel", "Steel"), 50, LOCTEXT("Steel Desc", "Sturdy, versatile construction material")),

	ResourceInfo(ResourceEnum::Glassware,	LOCTEXT("Glassware", "Glassware"),	30, LOCTEXT("Glassware Desc", "Beautiful liquid container made from Glass. (Luxury tier 2)")),
	ResourceInfo(ResourceEnum::PocketWatch,		LOCTEXT("Pocket Watch", "Pocket Watch"),	75, LOCTEXT("Pocket Watch Desc", "Elegant timepiece crafted by Clockmakers. (Luxury tier 3)")),

	ResourceInfo(ResourceEnum::PitaBread, LOCTEXT("Pita Bread", "Pita Bread"), FoodCost, LOCTEXT("Pita Bread Desc", "Delicious food baked from Wheat Flour")),
	ResourceInfo(ResourceEnum::Carpet, LOCTEXT("Carpet", "Carpet"), 50, LOCTEXT("Carpet Desc", "Luxury tier 3 used for housing upgrade.")),
	ResourceInfo(ResourceEnum::DateFruit, LOCTEXT("Date Fruit", "Date Fruit"), FoodCost, LOCTEXT("Date Fruit Desc", "Fruit with delicate, mildly-flavored flesh.")),
	ResourceInfo(ResourceEnum::ToiletPaper, LOCTEXT("Toilet Paper", "Toilet Paper"), 38, LOCTEXT("Toilet Paper Desc", "Luxury tier 3 used for housing upgrade.")),
	
};

//!!! Remember that resources other than Food shouldn't cost 5 !!!

static const int ResourceEnumCount = _countof(ResourceInfos);

#undef LOCTEXT_NAMESPACE

static bool IsResourceValid(ResourceEnum resourceEnum)
{
	int32 resourceEnumInt = static_cast<int32>(resourceEnum);;
	return ResourceEnumCount > resourceEnumInt && resourceEnumInt >= 0;
}


inline ResourceInfo GetResourceInfo(ResourceEnum resourceEnum) {
	return ResourceInfos[static_cast<int>(resourceEnum)];
}
inline ResourceInfo GetResourceInfo(int32 resourceEnumInt) {
	return ResourceInfos[resourceEnumInt];
}
inline int32 GetResourceCostWithMoney(ResourceEnum resourceEnum) {
	return resourceEnum == ResourceEnum::Money ? 1 : ResourceInfos[static_cast<int>(resourceEnum)].basePrice;
}

inline ResourceInfo GetResourceInfoSafe(ResourceEnum resourceEnum) {
	if (!IsResourceValid(resourceEnum)) {
		return ResourceInfos[static_cast<int>(ResourceEnum::Wood)];
	}
	return ResourceInfos[static_cast<int>(resourceEnum)];
}

inline std::string ResourceName(ResourceEnum resourceEnum) {
	PUN_CHECK(resourceEnum != ResourceEnum::None);
	return FTextToStd(ResourceInfos[static_cast<int>(resourceEnum)].name);
}
inline std::wstring ResourceNameW(ResourceEnum resourceEnum) {
	PUN_CHECK(resourceEnum != ResourceEnum::None);
	return FTextToW(ResourceInfos[static_cast<int>(resourceEnum)].name);
}
inline FText ResourceNameT(ResourceEnum resourceEnum) {
	PUN_CHECK(resourceEnum != ResourceEnum::None);
	if (resourceEnum == ResourceEnum::Money) {
		return NSLOCTEXT("GameSimulationInfo", "Money", "Money");
	}
	if (resourceEnum == ResourceEnum::Influence) {
		return NSLOCTEXT("GameSimulationInfo", "Influence", "Influence");
	}
	return ResourceInfos[static_cast<int>(resourceEnum)].name;
}

inline FText ResourceNameT_WithSpaceIcons(ResourceEnum resourceEnum) {
	PUN_CHECK(resourceEnum != ResourceEnum::None);
	if (resourceEnum == ResourceEnum::Money) {
		return INVTEXT("<img id=\"Coin\"/>");
	}
	if (resourceEnum == ResourceEnum::Influence) {
		return INVTEXT("<img id=\"Influence\"/>");
	}
	return FText::Format(INVTEXT(" {0}"), ResourceInfos[static_cast<int>(resourceEnum)].name);
}

inline FText ResourceName_WithNone(ResourceEnum resourceEnum) {
	if (resourceEnum == ResourceEnum::None) {
		return NSLOCTEXT("GameSimulationInfo", "None", "None");
	}
	if (resourceEnum == ResourceEnum::Food) {
		return NSLOCTEXT("GameSimulationInfo", "Food", "Food");
	}
	if (resourceEnum == ResourceEnum::Fuel) {
		return NSLOCTEXT("GameSimulationInfo", "Fuel", "Fuel");
	}
	return ResourceInfos[static_cast<int>(resourceEnum)].GetName();
}

inline FString ResourceNameF(ResourceEnum resourceEnum) {
	return ResourceInfos[static_cast<int>(resourceEnum)].name.ToString();
}
inline FString ResourceDisplayNameF(ResourceEnum resourceEnum) {
	return FString("Resource") + FString::FromInt(static_cast<int>(resourceEnum));
}


inline ResourceEnum FindResourceEnumByName(std::wstring name)
{
	for (int32 i = 0; i < ResourceEnumCount; i++) {
		ResourceEnum resourceEnum = static_cast<ResourceEnum>(i);
		if (ResourceNameW(resourceEnum) == name) {
			return resourceEnum;
		}
	}
	if (name == TEXT("Food")) {
		return ResourceEnum::Food;
	}
	if (name == TEXT("Fuel")) {
		return ResourceEnum::Fuel;
	}
	return ResourceEnum::None;
}

// Sorted resource
static std::vector<ResourceInfo> GetSortedNameResourceEnum()
{
	std::vector<std::wstring> resourceNames;
	for (int32 i = 0; i < ResourceEnumCount; i++) {
		ResourceInfo info = GetResourceInfo(static_cast<ResourceEnum>(i));
		std::wstring wString(*(info.name.ToString()));
		resourceNames.push_back(wString);
	}
	std::sort(resourceNames.begin(), resourceNames.end());

	std::vector<ResourceInfo> results;
	for (const std::wstring& name : resourceNames) {
		ResourceEnum resourceEnum = FindResourceEnumByName(name);
		PUN_CHECK(resourceEnum != ResourceEnum::None);
		results.push_back(GetResourceInfo(resourceEnum));
	}
	
	return results;
}
static std::vector<ResourceInfo> SortedNameResourceInfo = GetSortedNameResourceEnum();
static void ResetSortedNameResourceEnum() {
	SortedNameResourceInfo = GetSortedNameResourceEnum();
}


static const std::vector<ResourceEnum> FoodEnums_NonInput
{
	// Arrange food from high to low grabbing priority
	ResourceEnum::Bread,
	ResourceEnum::PitaBread,
	ResourceEnum::Cabbage,
	ResourceEnum::Papaya,
	ResourceEnum::Coconut,
	ResourceEnum::DateFruit,
	ResourceEnum::Fish,

	ResourceEnum::Pork,
	ResourceEnum::GameMeat,
	ResourceEnum::Beef,
	ResourceEnum::Lamb,

	ResourceEnum::Melon,
};

static const std::vector<ResourceEnum> FoodEnums_Input
{
	ResourceEnum::Honey,
	ResourceEnum::Orange,
	ResourceEnum::Milk,
	ResourceEnum::Mushroom,
	ResourceEnum::Wheat,
	ResourceEnum::Grape,

	ResourceEnum::Blueberries,
	ResourceEnum::Pumpkin,
	ResourceEnum::Potato,
};

static std::vector<ResourceEnum> GetFoodEnums()
{
	std::vector<ResourceEnum> result;
	result.insert(result.end(), FoodEnums_NonInput.begin(), FoodEnums_NonInput.end());
	result.insert(result.end(), FoodEnums_Input.begin(), FoodEnums_Input.end());
	return result;
}

static const std::vector<ResourceEnum> FoodEnums = GetFoodEnums();
static int32 FoodEnumCount = FoodEnums.size();

class StaticData
{
public:
	static std::vector<ResourceEnum> FoodEnums;
	static int32 FoodEnumCount;

	static void ResetFoodEnums()
	{
		FoodEnums.clear();
		FoodEnums.insert(FoodEnums.end(), FoodEnums_NonInput.begin(), FoodEnums_NonInput.end());
		FoodEnums.insert(FoodEnums.end(), FoodEnums_Input.begin(), FoodEnums_Input.end());

		FoodEnumCount = FoodEnums.size();
	}
};


static bool IsFoodEnum(ResourceEnum resourceEnum) {
	if (resourceEnum == ResourceEnum::Clay) {
		return false;
	}
	switch (resourceEnum)
	{
		// Food that cost more than FoodCost
	case ResourceEnum::Melon:
		return true;
	default:
		return GetResourceInfo(resourceEnum).basePrice == FoodCost; // !!! No other good's base cost should be the same as food
	}
}

static bool IsAgriculturalGoods(ResourceEnum resourceEnum)
{
	return IsFoodEnum(resourceEnum) || resourceEnum == ResourceEnum::Flour;
}

static bool IsMeatEnum(ResourceEnum resourceEnum) {
	switch(resourceEnum)
	{
	case ResourceEnum::Pork:
	case ResourceEnum::GameMeat:
	case ResourceEnum::Beef:
	case ResourceEnum::Lamb:
		return true;
	default:
		return false;
	}
}

static bool IsOreEnum(ResourceEnum resourceEnum) {
	switch (resourceEnum)
	{
	case ResourceEnum::IronOre:
	case ResourceEnum::GoldOre:
	case ResourceEnum::Gemstone:
	case ResourceEnum::Coal:
		return true;
	default:
		return false;
	}
}
static bool IsMetalEnum(ResourceEnum resourceEnum) {
	switch (resourceEnum)
	{
	case ResourceEnum::Iron:
		return true;
	default:
		return false;
	}
}

static bool IsTradeResource(ResourceEnum resourceEnum) {
	// Can put switch in, In case there is something that should be disabled from trade (unused resource etc.)
	return true;
}


static const std::vector<ResourceEnum> FuelEnums
{
	ResourceEnum::Coal,
	ResourceEnum::Wood,
};
static bool IsFuelEnum(ResourceEnum resourceEnumIn) {
	for (ResourceEnum resourceEnum : FuelEnums) {
		if (resourceEnumIn == resourceEnum) return true;
	}
	return false;
}

static const std::vector<ResourceEnum> MedicineEnums
{
	ResourceEnum::Medicine,
	ResourceEnum::Herb,
};
static bool IsMedicineEnum(ResourceEnum resourceEnumIn) {
	for (ResourceEnum resourceEnum : MedicineEnums) {
		if (resourceEnumIn == resourceEnum) return true;
	}
	return false;
}

static const std::vector<ResourceEnum> ToolsEnums
{
	ResourceEnum::SteelTools,
	//ResourceEnum::CrudeIronTools,
	ResourceEnum::StoneTools,
};
static bool IsToolsEnum(ResourceEnum resourceEnumIn) {
	for (ResourceEnum resourceEnum : ToolsEnums) {
		if (resourceEnumIn == resourceEnum) return true;
	}
	return false;
}

const ResourceEnum HerbivoreFoodEnums[] =
{
	ResourceEnum::Hay,
	ResourceEnum::Orange,
	ResourceEnum::Papaya,
	ResourceEnum::Coconut,
	ResourceEnum::DateFruit,
	
	ResourceEnum::Wheat,
	ResourceEnum::Milk,
	ResourceEnum::Mushroom,
	ResourceEnum::Grape,
};

static const ResourceEnum ConstructionResources[] = {
	ResourceEnum::Wood,
	ResourceEnum::Stone,
	ResourceEnum::Iron,
	ResourceEnum::Brick,
	ResourceEnum::Glass,
	ResourceEnum::Concrete,
	ResourceEnum::Steel,
	
	ResourceEnum::Clay,
};
static const int32 ConstructionResourceCount = _countof(ConstructionResources);


static int32 ConstructionCostAsMoney(std::vector<int32> bldResourceCost) {
	int32 resourceValue = 0;
	for (int32 i = 0; i < bldResourceCost.size(); i++) {
		resourceValue += bldResourceCost[i] * GetResourceInfo(ConstructionResources[i]).basePrice;
	}
	return resourceValue;
}

static bool IsConstructionResourceEnum(ResourceEnum resourceEnumIn)
{
	for (ResourceEnum resourceEnum : ConstructionResources) {
		if (resourceEnum == resourceEnumIn) {
			return true;
		}
	}
	return false;
}

/*
 * Luxury
 */

static const std::vector<std::vector<ResourceEnum>> TierToLuxuryEnums =
{
	{},
	{ResourceEnum::Beer, ResourceEnum::Cannabis, ResourceEnum::Furniture, ResourceEnum::Pottery, ResourceEnum::Tulip },
	{ResourceEnum::Cloth, ResourceEnum::Wine, ResourceEnum::Candle, ResourceEnum::Vodka, ResourceEnum::MagicMushroom, ResourceEnum::Coffee, ResourceEnum::Glassware},
	{ResourceEnum::Book, ResourceEnum::LuxuriousClothes, ResourceEnum::Jewelry, ResourceEnum::Chocolate, ResourceEnum::PocketWatch},
};

static const std::vector<ResourceEnum>& GetLuxuryResourcesByTier(int32 tier) {
	return TierToLuxuryEnums[tier];
}

static std::vector<ResourceEnum> _LuxuryResources;
static const std::vector<ResourceEnum>& GetLuxuryResources() {
	if (_LuxuryResources.size() == 0) {
		for (int32 i = 1; i < TierToLuxuryEnums.size(); i++) {
			_LuxuryResources.insert(_LuxuryResources.end(), TierToLuxuryEnums[i].begin(), TierToLuxuryEnums[i].end());
		}
	}
	return _LuxuryResources;
}

static int32 LuxuryResourceCount() {
	return GetLuxuryResources().size();
}

template <typename Func>
static void ExecuteOnLuxuryResources(Func func) {
	const std::vector<ResourceEnum>& luxuryResources = GetLuxuryResources();
	for (ResourceEnum resourceEnum : luxuryResources) {
		func(resourceEnum);
	}
}

static bool IsLuxuryEnum(ResourceEnum resourceEnum, int32 tier) {
	for (ResourceEnum luxuryEnum : TierToLuxuryEnums[tier]) {
		if (luxuryEnum == resourceEnum) return true;
	}
	return false;
}

static bool IsLuxuryEnum(ResourceEnum resourceEnum) {
	for (int32 i = 1; i < TierToLuxuryEnums.size(); i++) {
		if (IsLuxuryEnum(resourceEnum, i)) return true;
	}
	return false;
}

static FText LuxuryResourceTip(int32 tier)
{
	TArray<FText> args;
	args.Add(NSLOCTEXT("LuxuryTip", "Luxury Tip Usage", "To Upgrade Houses, provide them with Luxury Resources."));
	ADDTEXT_INV_("<space>");
	ADDTEXT_TAG_("<Bold>", FText::Format(NSLOCTEXT("LuxuryTip", "Luxury tier X:", "Luxury tier {0}:"), TEXT_NUM(tier)));
	for (size_t i = 0; i < TierToLuxuryEnums[tier].size(); i++) {
		ADDTEXT_(INVTEXT("\n {0}"), GetResourceInfo(TierToLuxuryEnums[tier][i]).GetName());
	}
	return JOINTEXT(args);
}

static bool IsMarketEnums(ResourceEnum resourceEnum) {
	return IsFoodEnum(resourceEnum) || IsFuelEnum(resourceEnum) || IsMedicineEnum(resourceEnum) || IsLuxuryEnum(resourceEnum) || IsToolsEnum(resourceEnum);
}



// Other than Food, Construction, Luxury
static std::vector<ResourceEnum> MiscResources = {
	ResourceEnum::Wool,
	ResourceEnum::Leather,
	ResourceEnum::IronOre,
	ResourceEnum::GoldOre,
	ResourceEnum::GoldBar,
	ResourceEnum::Gemstone,
	ResourceEnum::Paper,
	ResourceEnum::Hay,
	ResourceEnum::Clay,
};

/*
 * Trade
 */
// How fast price goes back to base price without tampering
static int64 EquilibriumSupplyValue_PerPerson(ResourceEnum resourceEnum)
{
	static const int32 SupplyMultiplier = 30; // Determines how fast price changes
	
	return GetResourceInfo(resourceEnum).basePrice * SupplyMultiplier;
}
static int64 EquilibriumSupplyValue100_PerPerson(ResourceEnum resourceEnum) {
	return EquilibriumSupplyValue_PerPerson(resourceEnum) * 100;
}

#define LOCTEXT_NAMESPACE "IntercityTradeOfferEnumName"

enum class IntercityTradeOfferEnum : uint8
{
	None,
	BuyWhenBelow,
	SellWhenAbove,
};
static TArray<FText> IntercityTradeOfferEnumName
{
	LOCTEXT("None", "None"),
	LOCTEXT("Buy When Below", "Buy When Below"),
	LOCTEXT("Sell When Above", "Sell When Above"),
};

#undef LOCTEXT_NAMESPACE

static FText GetIntercityTradeOfferEnumName(IntercityTradeOfferEnum offerEnum) {
	return IntercityTradeOfferEnumName[static_cast<int>(offerEnum)];
}
static IntercityTradeOfferEnum GetIntercityTradeOfferEnumFromName(FString nameIn)
{
	for (size_t i = 0; i < IntercityTradeOfferEnumName.Num(); i++) {
		if (IntercityTradeOfferEnumName[i].ToString() == nameIn) {
			return static_cast<IntercityTradeOfferEnum>(i);
		}
	}
	checkNoEntry();
	return IntercityTradeOfferEnum::None;
}


struct IntercityTradeOffer
{
	ResourceEnum resourceEnum = ResourceEnum::None;
	IntercityTradeOfferEnum offerEnum = IntercityTradeOfferEnum::None;
	int32 targetInventory = 0;

	//! Serialize
	FArchive& operator>>(FArchive &Ar) {
		Ar << resourceEnum
			<< offerEnum
			<< targetInventory;
		return Ar;
	}
};


/*
 * ResourcePair
 */
struct ResourcePair
{
	ResourceEnum resourceEnum;
	int32 count;
	
	ResourcePair(ResourceEnum resourceEnum, int count) : resourceEnum(resourceEnum), count(count) {}
	ResourcePair() : resourceEnum(ResourceEnum::None), count(0) {}

	ResourcePair& operator+=(const ResourcePair& a) {
		PUN_CHECK(resourceEnum == a.resourceEnum);
		count += a.count;
		return *this;
	}
	ResourcePair& operator-=(const ResourcePair& a) {
		PUN_CHECK(resourceEnum == a.resourceEnum);
		count -= a.count;
		PUN_CHECK(count >= 0);
		return *this;
	}
	bool operator==(const ResourcePair& a) const {
		return resourceEnum == a.resourceEnum && count == a.count;
	}

	bool isEnum(ResourceEnum resourceEnumIn) { return resourceEnum == resourceEnumIn; }

	std::string ToString() {
		return std::to_string(count) + " " + ResourceName(resourceEnum);
	}

	static ResourcePair Invalid() { return { ResourceEnum::None, 0}; }
	bool isValid() { return resourceEnum != ResourceEnum::None; }

	//! Serialize
	FArchive& operator>>(FArchive &Ar) {
		Ar << resourceEnum << count;
		return Ar;
	}

};

static int32 SumResourceCost(const std::vector<ResourcePair>& resourcePairs)
{
	int32 result = 0;
	for (const ResourcePair resourcePair : resourcePairs) {
		if (resourcePair.resourceEnum == ResourceEnum::Food) {
			result += FoodCost * resourcePair.count;
		}
		else {
			result += GetResourceInfo(resourcePair.resourceEnum).basePrice * resourcePair.count;
		}
	}
	return result;
}

static std::vector<int32> GetConstructionResourceListFromResourcePairs(std::vector<ResourcePair> resourcePairs)
{
	std::vector<int32> result(ConstructionResourceCount, 0);
	
	for (int32 i = 0; i < ConstructionResourceCount; i++)  
	{
		for (int32 j = 0; j < resourcePairs.size(); j++) {
			if (ConstructionResources[i] == resourcePairs[j].resourceEnum) {
				result[i] = resourcePairs[j].count;
				break;
			}
		}
	}
	return result;
}

//static std::string MaybeRedText(std::string text, bool isRed)
//{
//	if (isRed) {
//		return "<Red>" + text + "</>";
//	}
//	return text;
//};


enum class ResourceHolderType : uint8
{
	Manual, // Refilled only from explicit command. House for example.
	Requester,
	Storage,
	Provider,
	Drop,
	DropManual,
	Dead,

	Market,
};

static const std::vector<std::string> ResourceHolderTypeName
{
	"Manual", // Refilled only from explicit command. House for example.
	"Requester",
	"Storage",
	"Provider",
	"Drop",
	"DropManual",
	"Dead",

	"Market",
};

static std::string GetResourceHolderTypeName(ResourceHolderType type) {
	return ResourceHolderTypeName[static_cast<int>(type)];
}

enum class ReservationType : uint8
{
	None,
	Push,
	Pop,
	Workplace,
	TreeTile,
	FarmTile,
};

static std::string ReservationTypeName(ReservationType reservationType)
{
	switch (reservationType) {
		case ReservationType::None: return "None";
		case ReservationType::Push: return "Push";
		case ReservationType::Pop: return "Pop";
		case ReservationType::Workplace: return "Workplace";
		case ReservationType::TreeTile: return "TreeTile";
		case ReservationType::FarmTile: return "FarmTile";
	}
	return "";
};

// Light info for querying ResourceHolder
static const int InvalidResourceHolderId = -1;
struct ResourceHolderInfo
{
	ResourceEnum resourceEnum;
	int32 holderId;

	//TODO: once resourceSystem has information on holder's location... this should also hold WorldTile2 tile...

	ResourceHolderInfo() : resourceEnum(ResourceEnum::None), holderId(-1) {}
	ResourceHolderInfo(ResourceEnum resourceEnum, int holderId) : resourceEnum(resourceEnum), holderId(holderId) {}

	int32 resourceEnumInt() { return static_cast<int32_t>(resourceEnum); }

	bool isValid() { 
		return holderId != InvalidResourceHolderId && 
			resourceEnum != ResourceEnum::None && 
			static_cast<int>(resourceEnum) < ResourceEnumCount; 
	}

	bool operator==(const ResourceHolderInfo& info) const {
		return resourceEnum == info.resourceEnum && holderId == info.holderId;
	}
	bool operator!=(const ResourceHolderInfo& info) const {
		return resourceEnum != info.resourceEnum || holderId != info.holderId;
	}

	static ResourceHolderInfo Invalid() { return ResourceHolderInfo(ResourceEnum::None, InvalidResourceHolderId); }

	// Helper
	std::string resourceName() {
		return FTextToStd(ResourceInfos[static_cast<int>(resourceEnum)].name);
	}
	std::string ToString() {
		return "[Holder: " + resourceName() + " id:" + std::to_string(holderId) + "]";
	}

	void operator>>(FArchive &Ar) {
		Ar << resourceEnum;
		Ar << holderId;
	}

	int32 GetSyncHash() const {
		return static_cast<int32>(resourceEnum) + holderId;
	}
};

struct FoundResourceHolderInfo
{
	ResourceHolderInfo info;
	int32 amount; // amount available
	WorldTile2 tile;
	int32 objectId = -1;

	int32 distance = 99999; // convenience variable

	FoundResourceHolderInfo() : info(ResourceHolderInfo::Invalid()), amount(0) {}
	FoundResourceHolderInfo(ResourceHolderInfo info, int32 amount, WorldTile2 tile, int32 objectId = -1, int32 distance = 99999)
		: info(info), amount(amount), tile(tile), objectId(objectId), distance(distance) {}

	bool isValid() { return info.isValid(); }
	ResourceEnum resourceEnum() { return info.resourceEnum; }

	static FoundResourceHolderInfo Invalid() { return FoundResourceHolderInfo(ResourceHolderInfo::Invalid(), -1, WorldTile2::Invalid); }

	std::string resourceName() {
		return FTextToStd(ResourceInfos[static_cast<int>(info.resourceEnum)].name);
	}
	std::string ToString() {
		return "[Holder: " + resourceName() + " id:" + std::to_string(info.holderId) + " amount:" + std::to_string(amount) + " tile:" + tile.ToString() + "]";
	}
	
	void operator>>(FArchive &Ar) {
		info >> Ar;
		Ar << amount;
		tile >> Ar;
		Ar << objectId;
	}
};
struct FoundResourceHolderInfos
{
	std::vector<FoundResourceHolderInfo> foundInfos;

	FoundResourceHolderInfos() {}
	FoundResourceHolderInfos(std::vector<FoundResourceHolderInfo> foundInfos) : foundInfos(foundInfos) {}

	bool hasInfos() {
		return foundInfos.size() > 0;
	}

	int32 amount() const {
		int32 total = 0;
		for (FoundResourceHolderInfo foundInfo : foundInfos) {
			total += foundInfo.amount;
		}
		return total;
	}
	int32 amountWithNoBack() {
		int32_t total = 0;
		for (int i = 1; i < foundInfos.size(); i++) {
			total += foundInfos[i].amount;
		}
		return total;
	}

	int32 amountCapped(int32 targetAmount) const {
		return std::min(targetAmount, amount());
	}

	int32 totalDistance() const {
		int32 total = 0;
		for (FoundResourceHolderInfo foundInfo : foundInfos) {
			total += foundInfo.distance;
		}
		if (total == 0) {
			return INT_MAX;
		}
		return total;
	}

	bool IsBetterThan(const FoundResourceHolderInfos& a, int32 targetAmount) const
	{
		int32 thisAmount = std::min(targetAmount, amount());
		int32 a_amount = std::min(targetAmount, a.amount());
		if (thisAmount > a_amount) {
			return true;
		}
		if (a_amount > thisAmount) {
			return false;
		}

		return totalDistance() < a.totalDistance();
	}
	

	void Trim(int32 targetAmount) 
	{
		PUN_CHECK(amount() >= targetAmount);
		int32 targetTrim = amount() - targetAmount;
		for (int i = foundInfos.size(); i-- > 0;) {
			if (foundInfos[i].amount > targetTrim) {
				foundInfos[i].amount -= targetTrim;
				break;
			}
			else {
				targetTrim -= foundInfos[i].amount;
				foundInfos.erase(foundInfos.begin() + i);
				if (targetTrim == 0) {
					break;
				}
			}
		}

		PUN_CHECK(amount() == targetAmount);
	}

	FoundResourceHolderInfo best() { return foundInfos[0]; }
};


/**
 * Buildings
 */

enum class CardEnum : uint16
{
	House,
	StoneHouse,
	FruitGatherer,
	Townhall,
	StorageYard,

	GoldMine,
	Quarry,
	IronStatue,
	Bank,
	IceAgeSpire,

	Farm,
	MushroomFarm,
	Fence,
	FenceGate,
	Bridge,
	
	Forester,

	CoalMine,
	IronMine,
	SmallMarket,
	PaperMaker,
	IronSmelter,
	
	StoneToolShopOld,
	Blacksmith,
	Herbalist,
	MedicineMaker,

	FurnitureWorkshop,
	Chocolatier,

	// Decorations
	Garden,


	BoarBurrow,

	DirtRoad,
	StoneRoad,
	Demolish,

	Fisher,
	BlossomShrine,
	Winery,
	Library,
	School,
	
	Theatre,
	Tavern,
	Tailor,

	CharcoalMaker,
	BeerBrewery,
	ClayPit,
	Potter,
	HolySlimeRanch,

	TradingPost,
	TradingCompany,
	TradingPort,
	
	CardMaker,
	ImmigrationOffice,
	
	ThiefGuild,
	SlimePyramid,

	LovelyHeartStatue,
	HuntingLodge,
	RanchBarn,
	RanchPig,
	RanchSheep,
	RanchCow,

	GoldSmelter,
	Mint,
	
	BarrackClubman,
	BarrackSwordman,
	BarrackArcher,

	ShrineWisdom,
	ShrineLove,
	ShrineGreed,
	
	HellPortal,

	LaborerGuild,
	HumanitarianAidCamp,

	RegionTribalVillage,
	RegionShrine,
	RegionPort,
	RegionCrates,

	
	// June 1 additions
	Windmill,
	Bakery,
	GemstoneMine,
	Jeweler,

	// June 9 additions
	Beekeeper,
	Brickworks,
	CandleMaker,
	CottonMill,
	PrintingPress,

	// June 25
	Warehouse,
	Fort,
	ResourceOutpost,
	ResearchLab,
	IntercityRoad,

	// August 16
	FakeTownhall,
	FakeTribalVillage,
	ChichenItza,

	// October 20
	Market,
	ShippingDepot,
	IrrigationReservoir,

	// Nov 18
	Tunnel,
	GarmentFactory,

	// Dec 29
	MagicMushroomFarm,
	VodkaDistillery,
	CoffeeRoaster,

	// Feb 2
	Colony,
	PortColony,
	IntercityLogisticsHub,
	IntercityLogisticsPort,
	IntercityBridge,

	// Mar 12
	Granary,
	Archives,
	HaulingServices,

	// Apr 1
	SandMine,
	GlassSmelter,
	Glassworks,
	ConcreteFactory,
	CoalPowerPlant,
	IndustrialIronSmelter,
	Steelworks,
	StoneToolsShop,
	OilRig,
	OilPowerPlant,
	PaperMill,
	ClockMakers,

	Cathedral,
	Castle,
	GrandPalace,
	ExhibitionHall,

	// Decorations
	FlowerBed,
	GardenShrubbery1,
	GardenCypress,

	//! Rare
	//SalesOffice,
	ArchitectStudio,
	EngineeringOffice,
	DepartmentOfAgriculture,
	//IsolationistPublisher,

	StockMarket,
	CensorshipInstitute,
	EnvironmentalistGuild,
	IndustrialistsGuild,
	Oracle,
	AdventurersGuild,

	ConsultingFirm,
	ImmigrationPropagandaOffice,
	MerchantGuild,
	OreSupplier,
	BeerBreweryFamous,
	IronSmelterGiant,

	Cattery,
	InvestmentBank,

	StatisticsBureau,
	JobManagementBureau,

	// Aug 25
	MinorCity,
	MinorCityPort,

	TourismAgency,
	Hotel,
	Zoo,
	Museum,
	Embassy,
	ForeignQuarter,
	ForeignPort,
	SpyCenter,
	PolicyOffice,
	WorldTradeOffice,
	CardCombiner,

	MayanPyramid,
	EgyptianPyramid,
	StoneHenge,
	EasterIsland,
	Oasis,

	IrrigationPump,
	IrrigationDitch,
	CarpetWeaver,
	BathHouse,
	Caravansary,
	PitaBakery,
	GreatMosque,
	
	
	//! Non-Building Cards
	Investment,
	InstantBuild,
	ShrineWisdomPiece,
	ShrineLovePiece,
	ShrineGreedPiece,
	
	BeerTax,
	HomeBrew,
	
	MasterBrewer,
	MasterPotter,
	
	CooperativeFishing,
	CompaniesAct,

	ProductivityBook,
	ProductivityBook2,
	SustainabilityBook,
	SustainabilityBook2,
	FrugalityBook,
	Motivation,
	Motivation2,
	Passion,
	Passion2,
	
	DesertPilgrim,

	WheatSeed,
	CabbageSeed,
	HerbSeed,
	PotatoSeed,
	BlueberrySeed,
	MelonSeed,
	PumpkinSeed,
	
	CannabisSeeds,
	GrapeSeeds,
	CocoaSeeds,
	CottonSeeds,
	DyeSeeds,
	CoffeeSeeds,
	TulipSeeds,

	ChimneyRestrictor,
	SellFood,
	BuyWood,
	ChildMarriage,
	ProlongLife,
	BirthControl,
	CoalTreatment,

	CoalPipeline,
	MiningEquipment,
	Conglomerate,
	SmeltCombo,
	
	Immigration,
	DuplicateBuilding, // ??

	Pig,
	Sheep,
	Cow,

	//! Zoo Animals
	Boar,
	RedDeer,
	YellowDeer,
	DarkDeer,

	BrownBear,
	BlackBear,
	Panda,

	Moose,
	Hippo,
	Penguin,
	Bobcat,

	//! Artifacts
	Codex,
	SacrificialAltar,
	StoneStele,
	BallCourtGoals,
	FeatherCrown,
	
	CanopicJars,
	DepartureScrolls,
	DeathMask,
	SolarBarque,
	GoldCapstone,

	FeastRemains,
	ForeignTrinkets,
	ChalkPlaque,
	OfferingCup,
	OrnateTrinkets,

	TatooingNeedles,
	Petroglyphs,
	StoneFishhooks,
	CoralEyes,
	AncientStaff,
	
	//
	FireStarter,
	Steal,
	Snatch,
	SharingIsCaring,
	Kidnap,
	KidnapGuard,
	Raid,
	Terrorism,
	TreasuryGuard,
	Cannibalism,

	WildCard,

	// June Addition
	WildCardFood,
	WildCardIndustry,
	WildCardMine,
	WildCardService,
	CardRemoval,

	//! Rare
	HappyBreadDay,
	BlingBling,
	FarmWaterManagement,

	AllYouCanEat,
	SlaveLabor,
	Lockdown,
	SocialWelfare,

	Consumerism,
	BookWorm,

	
	Treasure,
	IndustrialRevolution,
	EmergencyRations, // TODO: move this

	//! Crate
	CrateWood,
	CrateCoal,
	CrateIronOre,
	CratePottery,
	CrateJewelry,

	//! Skills
	SpeedBoost,

	//! Permanent Bonuses
	BorealWinterFishing, // Start Section
	BorealWinterResist,
	BorealGoldOil,
	BorealPineForesting,

	DesertGem,
	DesertTradeForALiving,
	DesertOreTrade,
	DesertIndustry, // Industry, but can't farm

	SavannaRanch,
	SavannaHunt,
	SavannaGrasslandHerder,
	SavannaGrasslandRoamer,

	JungleGatherer,
	JungleMushroom,
	JungleHerbFarm,
	JungleTree,

	ForestFarm,
	ForestCharcoal,
	ForestTools,
	ForestBeer, // End Section

	Agriculturalist, // Start Section
	Geologist,
	AlcoholAppreciation,
	Craftmanship,

	Rationalism,
	Romanticism,
	Protectionism,
	FreeThoughts,

	Capitalism,
	Communism,
	WondersScoreMultiplier,
	PopulationScoreMultiplier, // End Section

	/* 
	 * Military
	 */
	Militia,
	Conscript,
	
	Warrior,
	Swordsman,
	Musketeer,
	Infantry,
	
	Knight, // Attack Bonus
	Tank, // Attack Bonus
	
	Archer,
	MachineGun, // Defense Bonus
	
	Catapult,
	Cannon,
	Artillery,
	
	Galley,
	Frigate,
	Battleship,

	
	
	None, // Also Total Count

	//! For Laborer Priority
	GatherTiles,
	Hauler,
	Constructor,

};

enum class CardHandEnum : uint8
{
	CardSlots,
	CardInventorySlots,
	PermanentHand,
	DrawHand,
	BoughtHand,
	ConverterHand,
	RareHand,
	TrainUnits,
	
	TradeDealSelect,
	TradeDealOffer,

	DeployMilitarySlots,
	CardSetSlots,

	None,
};

static const TMap<CardEnum, int32> BuildingEnumToUpkeep =
{
	{ CardEnum::Garden, 5 },
	
	{ CardEnum::Library, 15 },
	{ CardEnum::School, 18 },

	{ CardEnum::Museum, 100 },
	{ CardEnum::Zoo, 100 },
	{ CardEnum::Theatre, 50 },
	{ CardEnum::Tavern, 15 },

	{ CardEnum::Hotel, 100 },

	{ CardEnum::SpyCenter, 300 },

	{ CardEnum::PolicyOffice, 100 },

	{ CardEnum::Granary, 10 },

	{ CardEnum::TradingPost, 20 },
	{ CardEnum::TradingCompany, 10 },
	{ CardEnum::TradingPort, 20 },
	{ CardEnum::CardMaker, 20 },
	{ CardEnum::ImmigrationOffice, 10 },

	{ CardEnum::ImmigrationOffice, 10 },

	{ CardEnum::Fort, 50 },
};

// Building upkeep per round
static int32 GetCardUpkeepBase(CardEnum buildingEnum)
{
	if (BuildingEnumToUpkeep.Contains(buildingEnum)) {
		return BuildingEnumToUpkeep[buildingEnum];
	}
	//auto found = std::find_if(BuildingEnumToUpkeep.begin(), BuildingEnumToUpkeep.end(), 
	//							[&](std::pair<CardEnum, int32> pair) { return pair.first == buildingEnum; });
	//if (found != BuildingEnumToUpkeep.end()) {
	//	return found->second;
	//}
	return 0;
}

/*
 * World Wonders
 */
static const std::vector<CardEnum> WorldWonders
{
	CardEnum::Cathedral,
	CardEnum::Castle,
	CardEnum::GrandPalace,
	CardEnum::ExhibitionHall,

	CardEnum::GreatMosque,
};
static bool IsWorldWonder(CardEnum cardEnumIn) {
	for (CardEnum cardEnum : WorldWonders) {
		if (cardEnum == cardEnumIn)	return true;
	}
	return false;
}

static const std::vector<CardEnum> AutoQuickBuildList
{
	CardEnum::StatisticsBureau,
	CardEnum::JobManagementBureau,
	CardEnum::Fort,
	CardEnum::ResourceOutpost,
};
static bool IsAutoQuickBuild(CardEnum cardEnumIn) {
	for (CardEnum cardEnum : AutoQuickBuildList) {
		if (cardEnum == cardEnumIn)	return true;
	}
	return false;
}


/*
 * No Era Upgrade
 */
static bool HasNoEraUpgrade(CardEnum buildingEnumIn) { // Upgrade with era without pressing upgrade manually
	switch (buildingEnumIn) {
	case CardEnum::PaperMaker:
	case CardEnum::IronSmelter:
		return true;
	default:
		return false;
	}
}

static bool IsEndOfEraBuilding(CardEnum buildingEnumIn) { // End of Era building gets some bonus
	switch (buildingEnumIn) {
	case CardEnum::MedicineMaker:
	case CardEnum::CardMaker:
	case CardEnum::CandleMaker:
	case CardEnum::Winery:

	case CardEnum::Mint:
	case CardEnum::Chocolatier:
		return true;
	default:
		return false;
	}
}


/*
 * Power Plants
 */
struct PowerPlantInfo
{
	CardEnum buildingEnum;
	ResourceEnum resourceEnum;
	int32 baseCapacity;
};
static const std::vector<PowerPlantInfo> PowerPlantInfoList
{
	{ CardEnum::CoalPowerPlant, ResourceEnum::Coal, 50 },
	{ CardEnum::OilPowerPlant, ResourceEnum::Oil, 50 },
};
static bool IsPowerPlant(CardEnum cardEnumIn) {
	for (const PowerPlantInfo& info : PowerPlantInfoList) {
		if (info.buildingEnum == cardEnumIn) return true;
	}
	return false;
}
static const PowerPlantInfo& GetPowerPlantInfo(CardEnum cardEnumIn)
{
	for (int32 i = 0; i < PowerPlantInfoList.size(); i++) {
		if (PowerPlantInfoList[i].buildingEnum == cardEnumIn) {
			return PowerPlantInfoList[i];
		}
	}
	UE_DEBUG_BREAK();
	return PowerPlantInfoList[0];
}


/*
 * Building Cost Calculation
 *
static const ResourceEnum ConstructionResources[] = {
	ResourceEnum::Wood,
	ResourceEnum::Stone,
	ResourceEnum::Iron,
	ResourceEnum::Brick,
	ResourceEnum::Glass,
	ResourceEnum::Concrete,
	ResourceEnum::SteelBeam,
};
Furniture: 50,20
Beer: 40, 30
Potter: 20, 40
 */
struct BldResourceInfo
{
	/*
	 * Ideas to convey
	 *  - workers increase by era 2->4->7->10
	 *  - Large factory benefit from Economy of Scale => more revenue
	 */
	
	CardEnum buildingEnum = CardEnum::None;
	int32 era = 1;
	
	ResourceEnum input1 = ResourceEnum::None;
	ResourceEnum input2 = ResourceEnum::None;
	ResourceEnum produce = ResourceEnum::None;
	
	int32 workerCount = 0;
	std::vector<int32> constructionResources;

	int32 costPerWorker_beforeEraAndUpgrade = 0;
	int32 baseResourceCostMoney = 0;

	int32 baseCardPrice = 0;
	int32 baseUpkeep = 0;
	
	int32 workRevenuePerSec100_perMan_beforeUpgrade = -1;

	int32 workRevenuePerSec100_perMan(int32 upgradeCount) const
	{
		check(workRevenuePerSec100_perMan_beforeUpgrade > 0);
		int32 result = std::max(1, workRevenuePerSec100_perMan_beforeUpgrade);
		for (int32 i = 0; i < upgradeCount; i++) {
			result = result * UpgradeProfitPercentEraMultiplier / 100;
		}
		return result;
	}
	
	int32 ApplyUpgradeAndEraProfitMultipliers(int32 value, int32 minEra, int32 upgradeCount) const
	{
		if (value <= 0) {
			return 0;
		}
		
		for (int32 i = 1; i < minEra; i++) {
			value = value * BaseProfitPercentEraMultiplier / 100;
		}
		for (int32 i = 0; i < upgradeCount; i++) {
			value = value * UpgradeProfitPercentEraMultiplier / 100;
		}
		return value;
	}
	int32 ApplyUpgradeCostMultipliers(int32 value, int32 upgradeCount) const
	{
		check(value > 0);
		for (int32 i = 0; i < upgradeCount; i++) {
			value = value * UpgradeCostPercentEraMultiplier / 100;
		}
		return value;
	}

	static const int32 PercentUpkeepToPrice = 10; // May 31: 5 -> 10

	// TODO:
	static const int32 BaseCostPercentEraMultiplier = 140;
	static const int32 BaseProfitPercentEraMultiplier = 120;

	// For calculating upgrade cost and final base production
	static const int32 UpgradeCostPercentEraMultiplier = 150;
	static const int32 UpgradeProfitPercentEraMultiplier = 115;

	static const int32 FirstIndustryIncentiveMultiplier = 80;

	static const int32 BaseCostPerWorker = 250;

	void CalculateResourceCostValueBeforeDiscount(const std::vector<ResourceEnum>& inputsAndOutput, int32 percentDiff)
	{
		// End of Era building gets some bonus
		int32 costPerWorker = BaseCostPerWorker;

		// Special cases:
		if (IsEndOfEraBuilding(buildingEnum)) {
			costPerWorker = costPerWorker * 130 / 100;
		}
		if (buildingEnum == CardEnum::Tailor) { // Tailor is has high worker count while not being too expensive
			costPerWorker = costPerWorker * 70 / 100;
		}

		// Percent Diff
		costPerWorker = costPerWorker * (100 + percentDiff) / 100;

		// More resource requirement means more value for building
		if (inputsAndOutput.size() == 3) {
			costPerWorker = costPerWorker * 130 / 100;
		}

		// Building without era upgrade gets bonus
		if (HasNoEraUpgrade(buildingEnum)) {
			costPerWorker = costPerWorker * BaseProfitPercentEraMultiplier / 100;
		}

		costPerWorker_beforeEraAndUpgrade = costPerWorker;
	}

	void CalculateConstructionCosts(const std::vector<int32>& resourceRatio)
	{
		int32 costPerWorker = costPerWorker_beforeEraAndUpgrade;

		// Era
		for (int32 i = 1; i < era; i++) {
			costPerWorker = costPerWorker * BaseCostPercentEraMultiplier / 100;
		}

		// Era 1 special discount encouragement
		if (era == 1) {
			costPerWorker = costPerWorker * FirstIndustryIncentiveMultiplier / 100;
		}

		int32 constructionCostMoney = costPerWorker * (workerCount + 1);
		
		int32 totalValueRatio = 0;
		std::vector<int32> valueRatio(resourceRatio.size());
		for (int32 i = 0; i < resourceRatio.size(); i++) {
			valueRatio[i] = resourceRatio[i] * GetResourceInfo(ConstructionResources[i]).basePrice;
			totalValueRatio += valueRatio[i];
		}

		std::vector<int32> resourceCost(resourceRatio.size());
		for (int32 i = 0; i < resourceCost.size(); i++) {
			resourceCost[i] = constructionCostMoney * valueRatio[i] / totalValueRatio / GetResourceInfo(ConstructionResources[i]).basePrice;
			resourceCost[i] = resourceCost[i] / 5 * 5; // Round 5
		}
		constructionResources = resourceCost;
	}

	void CalculateBaseCardPrice()
	{
		baseResourceCostMoney = ConstructionCostAsMoney(constructionResources);

		int32 variablePrice = baseResourceCostMoney / 12;
		variablePrice = (variablePrice / 10) * 10; // round to 10,20,30... etc.
		baseCardPrice = 20 + variablePrice * 2;

		if (buildingEnum == CardEnum::StatisticsBureau ||
			buildingEnum == CardEnum::JobManagementBureau ||
			buildingEnum == CardEnum::House ||
			buildingEnum == CardEnum::Farm ||
			buildingEnum == CardEnum::StorageYard ||
			buildingEnum == CardEnum::StoneRoad)
		{
			baseCardPrice = 0;
		}
	}

	void CalculateBaseUpkeep()
	{
		baseUpkeep = baseCardPrice * PercentUpkeepToPrice / 100;

		// Industry special case
		//  lux t1 is cheap to build to encourage people to build them, but harder to maintain
		if (produce != ResourceEnum::None)
		{
			if (IsLuxuryEnum(produce, 1)) {
				baseUpkeep *= 2;
			}
		}

		baseUpkeep = baseUpkeep / 10 * 10;
		baseUpkeep = std::max(5, baseUpkeep);
	}

	void CalculateWorkRevenue()
	{
		if (workerCount > 0)
		{
			// Revenue calculated from base cost (cost before discount for construction)
			const int32 secsToBreakEven = Time::SecondsPerYear / 2;

			// Era
			int32 costPerWorker_beforeUpgrade = costPerWorker_beforeEraAndUpgrade;
			for (int32 i = 1; i < era; i++) {
				costPerWorker_beforeUpgrade = costPerWorker_beforeUpgrade * BaseProfitPercentEraMultiplier / 100;
			}

			workRevenuePerSec100_perMan_beforeUpgrade = costPerWorker_beforeUpgrade * 100 / secsToBreakEven; // Revenue increase required to get to breakeven within secsToBreakEven
			

			// Adjust revenueIncreasePerSec100 by WorkRevenueToCost_Base (), just like WorkRevenuePerSec100_perMan_Base
			//  This take into account time workers spend walking
			workRevenuePerSec100_perMan_beforeUpgrade = workRevenuePerSec100_perMan_beforeUpgrade * WorkRevenueToCost_Base / 100;

			// Economy of scale, each worker increases revenue by 5%
			workRevenuePerSec100_perMan_beforeUpgrade = workRevenuePerSec100_perMan_beforeUpgrade * (workerCount * 5 + 100) / 100;

			// Special cases
			// Building without era upgrade gets work bonus
			if (HasNoEraUpgrade(buildingEnum)) {
				workRevenuePerSec100_perMan_beforeUpgrade = workRevenuePerSec100_perMan_beforeUpgrade * UpgradeProfitPercentEraMultiplier / 100;
			}

			if (buildingEnum == CardEnum::Tailor) { // Tailor is has high worker count while not being too expensive
				workRevenuePerSec100_perMan_beforeUpgrade = workRevenuePerSec100_perMan_beforeUpgrade * 70 / 100;
			}
		}
	}
};


enum class FactionEnum : uint8
{
	Europe,
	Arab,

	None,
};


struct BldInfo
{
	CardEnum cardEnum;
	
	// Need std::string since we need to copy when passing into FName (allows using name.c_str() instead of FName in some cases)
	FText name;
	FString nameFString;
	FText namePlural;
	FText description;
	
	WorldTile2 size;
	ResourceEnum input1 = ResourceEnum::None;
	ResourceEnum input2 = ResourceEnum::None;
	ResourceEnum produce = ResourceEnum::None;

	int32 workerCount = 0;

	std::vector<int32> constructionResources;
	
	int32 buildTime_ManSec100 = 0;
	int32 maxBuilderCount = 0;

	int32 baseCardPrice = 0;
	int32 baseUpkeep = 0;

	BldResourceInfo resourceInfo;

	std::vector<WorldTile2> sizeByFactions;

	WorldTile2 GetSize(FactionEnum factionEnum) const {
		if (factionEnum != FactionEnum::None && sizeByFactions.size() > 0) {
			return sizeByFactions[static_cast<int>(factionEnum)];
		}
		return size;
	}

	std::string nameStd() const { return FTextToStd(name); }
	std::wstring nameW() const { return FTextToW(name); }
	FString nameF() const { return nameFString; }
	FText GetName() const { return name; }
	FText GetDescription() const { return description; }

	FText GetName(int32 count) { return count > 1 ? namePlural : name; }

	const FString* GetDisplayName() { return FTextInspector::GetSourceString(name); }

	bool hasInput1() { return input1 != ResourceEnum::None; }
	bool hasInput2() { return input2 != ResourceEnum::None; }

	int32 constructionCostAsMoney() const {
		return ConstructionCostAsMoney(constructionResources);
	}

	int32 workRevenuePerSec100_perMan(int32 upgradeCount = 0) const {
		return resourceInfo.workRevenuePerSec100_perMan(upgradeCount);
	}

	int32 minEra() const { return resourceInfo.era; }

	std::vector<int32> GetConstructionResources(FactionEnum factionEnum) const
	{
		if (factionEnum == FactionEnum::Arab) {
			if (cardEnum == CardEnum::House) {
				return GetConstructionResourceListFromResourcePairs({ ResourcePair(ResourceEnum::Clay, 20) });
			}
		}
		return constructionResources;
	}
	

	BldInfo(CardEnum buildingEnum,
		FText nameIn,
		FString nameFStringIn,
		FText namePluralIn,
		FText descriptionIn,

		std::vector<WorldTile2> sizeByFactionsIn,
		BldResourceInfo bldResourceInfoIn
	) :
		BldInfo(buildingEnum, nameIn, nameFStringIn, namePluralIn, descriptionIn, sizeByFactionsIn[0], bldResourceInfoIn)
	{
		sizeByFactions = sizeByFactionsIn;
	}
	

	BldInfo(CardEnum buildingEnum,
		FText nameIn,
		FString nameFStringIn,
		FText namePluralIn,
		FText descriptionIn,

		WorldTile2 sizeIn,
		BldResourceInfo bldResourceInfoIn
	)
	{
		cardEnum = buildingEnum;
		check(bldResourceInfoIn.buildingEnum == buildingEnum);
		
		name = nameIn;
		nameFString = nameFStringIn;
		namePlural = namePluralIn.IsEmpty() ? name : namePluralIn;
		description = descriptionIn;

		size = sizeIn;


		resourceInfo = bldResourceInfoIn;

		input1 = bldResourceInfoIn.input1;
		input2 = bldResourceInfoIn.input2;
		produce = bldResourceInfoIn.produce;
		//baseOutputPerBatch = bldResourceInfoIn.baseOutputPerBatch;

		workerCount = bldResourceInfoIn.workerCount;
		constructionResources = bldResourceInfoIn.constructionResources;

		
		// Ensure complete construction list
		constructionResources.resize(ConstructionResourceCount, 0);
		
		/*
		 * Constraints for buildTime_ManSec100:
		 *  - House should be 6000 stand for 60 sec build time
		 *  - Iron smelter should be 330 sec or 33000
		 *
		 *  Iron smelter takes x10 house's resource...
		 *  This works:
		 *  - 3000 base
		 *  - 3000 house variable time
		 *  - 30000 smelter variable time
		 */
		// buildManSecCost100 = constructionCost * 100 /  CoinsPerManSec100 * buildManSecCostFactor100 ... ManSec100
		buildTime_ManSec100 = 3000; // base
		for (size_t i = 0; i < _countof(ConstructionResources); i++) {
			int32 constructionCost = constructionResources[i] * GetResourceInfo(ResourceEnum::Wood).basePrice; // Just calculate based on wood cost.. or else iron building seems to take too long to build // GetResourceInfo(ConstructionResources[i]).cost;
			int32 resourceCostManSec = constructionCost * 100 / WorkRevenue100PerSec_perMan_Base; // For house this is 384, meaning it takes 384 sec to complete a house...
			buildTime_ManSec100 += resourceCostManSec * BuildManSecCostFactor100;
		}

		// Special case
		switch (buildingEnum)
		{
		// Special case dirt road is built almost instantly
		case CardEnum::DirtRoad: buildTime_ManSec100 = 3; break;
		case CardEnum::StoneRoad: buildTime_ManSec100 = 3; break;
		default:
			break;
		}

		// Note: 40 is hack to make it double previous
		const int32 baseHouseSecCost100 = (40 * GetResourceInfo(ResourceEnum::Wood).basePrice * 100 / WorkRevenue100PerSec_perMan_Base) * BuildManSecCostFactor100; // For house, with 20 resource, we leave 2 people so that they can carry resource all at once
		int32 bldToHouseRatio = buildTime_ManSec100 / baseHouseSecCost100;

		maxBuilderCount = 1 + bldToHouseRatio; // Do this so there isn't too many builders on expensive buildings...

		maxBuilderCount = std::min(maxBuilderCount, 10);


		// Farm has large area to clear.
		if (buildingEnum == CardEnum::Farm) {
			maxBuilderCount = 2;
		}
		else if (IsAutoQuickBuild(buildingEnum)) {
			maxBuilderCount = 0;
		}


		baseCardPrice = bldResourceInfoIn.baseCardPrice;
		baseUpkeep = bldResourceInfoIn.baseUpkeep;

	}

	// Cards ... TODO: may be later make this CardInfo?
	BldInfo(CardEnum buildingEnum,
			FText name, FString nameFString,
			int32 cardPrice, FText description) :

			cardEnum(buildingEnum),
			name(name),
			nameFString(nameFString),
			description(description),
			baseCardPrice(cardPrice)
	{}

};

TileArea BuildingArea(WorldTile2 centerTile, WorldTile2 size, Direction faceDirection);
WorldTile2 GetBuildingCenter(TileArea area, Direction faceDirection);


struct BuildPlacement
{
public:
	WorldTile2 centerTile;
	WorldTile2 size;
	Direction faceDirection;

	bool isValid() const { return centerTile.isValid(); }

	BuildPlacement() : centerTile(WorldTile2::Invalid), size(WorldTile2::Invalid), faceDirection(Direction::S) {}
	
	BuildPlacement(WorldTile2 centerTile, WorldTile2 size, Direction faceDirection)
		: centerTile(centerTile), size(size), faceDirection(faceDirection)
	{}

	TileArea area() {
		return BuildingArea(centerTile, size, faceDirection);
	}

	FArchive& operator>>(FArchive &Ar) {
		centerTile >> Ar;
		size >> Ar;
		Ar << faceDirection;
		return Ar;
	}
};



// percentDiff is for base cost
//
static int32 BldResourceInfoCount = 0;
static BldResourceInfo GetBldResourceInfo(int32 era, std::vector<ResourceEnum> inputsAndOutput, std::vector<int32> resourceRatio, int32 percentDiff = 0, int32 outputBatchProfitPercentMultiplier = 100, int32 workerCountDiff = 0)
{
	BldResourceInfo bldResourceInfo;
	bldResourceInfo.buildingEnum = static_cast<CardEnum>(BldResourceInfoCount);
	BldResourceInfoCount++;
	
	bldResourceInfo.era = era;

	// Determine Worker Count by Era
	int32 workerCount = 2;
	switch (era) {
		case 2: workerCount = 4; break;
		case 3: workerCount = 7; break;
		case 4: workerCount = 10; break;
		default: break;
	}
	workerCount += workerCountDiff;
	if (workerCountDiff == -999) {
		workerCount = 0;
	}
	check(workerCount >= 0);
	
	bldResourceInfo.workerCount = workerCount;

	if (inputsAndOutput.size() == 1) {
		bldResourceInfo.produce = inputsAndOutput[0];
	}
	else if (inputsAndOutput.size() == 2) {
		bldResourceInfo.input1 = inputsAndOutput[0];
		bldResourceInfo.produce = inputsAndOutput[1];
	}
	else if (inputsAndOutput.size() == 3) {
		bldResourceInfo.input1 = inputsAndOutput[0];
		bldResourceInfo.input2 = inputsAndOutput[1];
		bldResourceInfo.produce = inputsAndOutput[2];
	}

	/*
	 * Calculate
	 */
	bldResourceInfo.CalculateResourceCostValueBeforeDiscount(inputsAndOutput, percentDiff);
	bldResourceInfo.CalculateConstructionCosts(resourceRatio);
	
	bldResourceInfo.CalculateBaseCardPrice();
	bldResourceInfo.CalculateBaseUpkeep();
	bldResourceInfo.CalculateWorkRevenue();

	return bldResourceInfo;
}

static BldResourceInfo GetBldResourceInfoMoney(int32 moneyCost, int32 workerCount = 0)
{
	BldResourceInfo result;
	result.buildingEnum = static_cast<CardEnum>(BldResourceInfoCount);
	BldResourceInfoCount++;
	
	result.workerCount = workerCount;
	result.baseCardPrice = moneyCost;
	return result;
}
static BldResourceInfo GetBldResourceInfoManual(std::vector<int32> resourceCost, int32 workerCount = 0, int32 minEra = 1)
{
	BldResourceInfo result;
	result.buildingEnum = static_cast<CardEnum>(BldResourceInfoCount);
	BldResourceInfoCount++;
	
	result.workerCount = workerCount;
	result.constructionResources = resourceCost;
	result.costPerWorker_beforeEraAndUpgrade = ConstructionCostAsMoney(resourceCost) / (1 + workerCount);
	// Note: This costPerWorker_beforeEraAndUpgrade would not work properly beyond Era 1

	result.era = minEra;

	result.CalculateBaseCardPrice();
	result.CalculateBaseUpkeep();
	result.CalculateWorkRevenue();
	
	return result;
}



#define LOCTEXT_NAMESPACE "CardInfo"

#define _LOCTEXT(a, b) LOCTEXT(a, b), FString(b)
#define _INVTEXT(a) INVTEXT(a), FString(a)

static const BldInfo BuildingInfo[]
{
	// Note that size is (y, x) since y is horizontal and x is vertical in UE4
	BldInfo(CardEnum::House, _LOCTEXT("House", "House"), LOCTEXT("House (Plural)", "Houses"), LOCTEXT("House Desc", "Protects people from cold. Extra houses boost population growth."),
		WorldTile2(6, 6), GetBldResourceInfoManual({20})
	),
	BldInfo(CardEnum::StoneHouse,	_LOCTEXT("Stone House", "Stone House"),		LOCTEXT("Stone House (Plural)", "Stone Houses"), FText(),
		WorldTile2(4, 4), GetBldResourceInfoManual({20})
	),
	BldInfo(CardEnum::FruitGatherer, _LOCTEXT("Fruit Gatherer", "Fruit Gatherer"), LOCTEXT("Fruit Gatherer (Plural)", "Fruit Gatherers"), LOCTEXT("Fruit Gatherer Desc", "Gathers fruit from trees and bushes."),
		WorldTile2(4, 4), GetBldResourceInfoManual({30}, 2)
	),
	BldInfo(CardEnum::Townhall,		_LOCTEXT("Townhall",	"Townhall"),	LOCTEXT("Townhall (Plural)", "Townhalls"),	LOCTEXT("Townhall Desc",	"The administrative center of your town."),
		WorldTile2(12, 12),	GetBldResourceInfoMoney(0)
	),
	BldInfo(CardEnum::StorageYard,	_LOCTEXT("Storage Yard",	"Storage Yard"),	LOCTEXT("Storage Yard (Plural)", "Storage Yards"),	LOCTEXT("Storage Yard Desc",	"Store resources."),
		WorldTile2(2, 2),	 GetBldResourceInfoManual({10})
	),

	BldInfo(CardEnum::GoldMine,		_LOCTEXT("Gold Mine", "Gold Mine"),	LOCTEXT("Gold Mine (Plural)", "Gold Mines"),	LOCTEXT("Gold Mine Desc", "Mine Gold Ores from Gold Deposit."),
		WorldTile2(6, 5), GetBldResourceInfo(1, { ResourceEnum::GoldOre }, {1,1}, 100, 0, 2)
	),
	BldInfo(CardEnum::Quarry,		_LOCTEXT("Quarry", "Quarry"),				LOCTEXT("Quarry (Plural)", "Quarries"),	LOCTEXT("Quarry Desc", "Mine Stone from mountain."),
		WorldTile2(5, 6),	GetBldResourceInfo(1, { ResourceEnum::Stone }, {1}, -20, 0, 2)
	),
	BldInfo(CardEnum::IronStatue,	_LOCTEXT("Stone Statue",	"Stone Statue"),	LOCTEXT("Stone Statue (Plural)", "Stone Statues"), FText(),
		WorldTile2(4, 4),	GetBldResourceInfoManual({})
	),
	BldInfo(CardEnum::Bank,			_LOCTEXT("Bank",	"Bank"), LOCTEXT("Bank (Plural)", "Banks"),	LOCTEXT("Bank Desc", "+50%<img id=\"Coin\"/> Income from Luxury for each surrounding level 5+ houses."),
		WorldTile2(4, 5),	GetBldResourceInfo(3, {}, {0, 0, 0, 2, 1}, 0, 100, -999)
	),
	BldInfo(CardEnum::IceAgeSpire,	_INVTEXT("Ice Age Spire"),	INVTEXT("Ice Age Spire"),	INVTEXT("Spire for worshipping Groth, the god of destruction. Decrease global temperature by -10 C."),
		WorldTile2(8, 8),	GetBldResourceInfoManual({})
	),

	BldInfo(CardEnum::Farm,	_LOCTEXT("Farm",	"Farm"), LOCTEXT("Farm (Plural)",	"Farms"), LOCTEXT("Farm Desc",	"Grow food/raw materials. Harvest during autumn."),
		WorldTile2(8, 8),	GetBldResourceInfoManual({20}, 1)
	),
	BldInfo(CardEnum::MushroomFarm,	_LOCTEXT("Mushroom Farm", "Mushroom Farm"), LOCTEXT("Mushroom Farm (Plural)", "Mushroom Farms"),	LOCTEXT("Mushroom Farm Desc", "Farm Mushroom using wood."),
		WorldTile2(8, 8), GetBldResourceInfo(1, {ResourceEnum::Wood, ResourceEnum::Mushroom}, {1}, 0, 100)
	),
	BldInfo(CardEnum::Fence,			_LOCTEXT("Fence", "Fence"),	FText(), INVTEXT("Fence used to keep farm animals in ranches, or wild animals away from farm crops"),
		WorldTile2(1, 1),	GetBldResourceInfoManual({})
	),
	BldInfo(CardEnum::FenceGate, _LOCTEXT("Fence Gate", "Fence Gate"), FText(), INVTEXT("Fence gate blocks animals from entering while letting people through."),
		WorldTile2(1, 1),	GetBldResourceInfoManual({})
	),
	BldInfo(CardEnum::Bridge,		_LOCTEXT("Bridge", "Bridge"),	LOCTEXT("Bridge (Plural)", "Bridges"),	LOCTEXT("Bridge Desc", "Allow citizens to cross over water."),
		WorldTile2(1, 1),	GetBldResourceInfoMoney(300)
	),

	BldInfo(CardEnum::Forester,		_LOCTEXT("Forester", "Forester"),	LOCTEXT("Forester (Plural)", "Foresters"),	LOCTEXT("Forester Desc", "Cut/plants trees within your territory."),
		WorldTile2(5, 5),	GetBldResourceInfoManual({50,50}, 2)
	),

	BldInfo(CardEnum::CoalMine,		_LOCTEXT("Coal Mine", "Coal Mine"),	LOCTEXT("Coal Mine (Plural)", "Coal Mines"), LOCTEXT("Coal Mine Desc", "Mine Coal from Coal Deposits."),
		WorldTile2(6, 5),	GetBldResourceInfo(1, {ResourceEnum::Coal}, {5,3}, 0, 100, 2)
	),
	BldInfo(CardEnum::IronMine,		_LOCTEXT("Iron Mine", "Iron Mine"),	LOCTEXT("Iron Mine (Plural)", "Iron Mines"), LOCTEXT("Iron Mine Desc", "Mine Iron Ores from Iron Deposits."),
		WorldTile2(6, 5), GetBldResourceInfo(1, {ResourceEnum::IronOre}, {5,3}, 0, 100, 2)
	),
	BldInfo(CardEnum::SmallMarket,	_LOCTEXT("Small Market", "Small Market"),	LOCTEXT("Small Market (Plural)", "Small Markets"),	LOCTEXT("Small Market Desc", "Sell goods to people for <img id=\"Coin\"/>"),
		WorldTile2(5, 8), GetBldResourceInfoManual({})
	),
	BldInfo(CardEnum::PaperMaker,	_LOCTEXT("Paper Maker", "Paper Maker"),		LOCTEXT("Paper Maker (Plural)", "Paper Makers"),	LOCTEXT("Paper Maker Desc", "Produce Paper."),
		WorldTile2(8, 6), GetBldResourceInfo(2, {ResourceEnum::Wood, ResourceEnum::Paper}, {3, 0, 1, 1}, 0)
	),
	BldInfo(CardEnum::IronSmelter,	_LOCTEXT("Iron Smelter", "Iron Smelter"), LOCTEXT("Iron Smelter (Plural)", "Iron Smelters"),	LOCTEXT("Iron Smelter Desc", "Smelt Iron Ores into Iron Bars."),
		WorldTile2(6, 7), GetBldResourceInfo(2, {ResourceEnum::IronOre, ResourceEnum::Coal, ResourceEnum::Iron}, {1,5}, 0, 100, 1)
	),


	/**/ BldInfo(CardEnum::StoneToolShopOld, _LOCTEXT("Stone Tool Shop", "Stone Tool Shop"),	LOCTEXT("Stone Tool Shop (Plural)", "Stone Tool Shops"), LOCTEXT("Stone Tool Shop Desc", "Craft Stone Tools from Stone and Wood."),
		WorldTile2(5, 8), GetBldResourceInfo(1, {ResourceEnum::Stone, ResourceEnum::Wood, ResourceEnum::SteelTools}, {2, 1})
	),
	BldInfo(CardEnum::Blacksmith,	_LOCTEXT("Blacksmith", "Blacksmith"),	LOCTEXT("Blacksmith (Plural)", "Blacksmiths"),	LOCTEXT("Blacksmith Desc", "Forge Tools from Iron Bars and Wood."),
		WorldTile2(5, 8), GetBldResourceInfo(2, {ResourceEnum::Iron, ResourceEnum::Wood, ResourceEnum::SteelTools}, {1, 1, 1}, 0, 100, -1)
	),
	BldInfo(CardEnum::Herbalist, _LOCTEXT("Herbalist", "Herbalist"),	LOCTEXT("Herbalist (Plural)", "Herbalists"), FText(),
		WorldTile2(4, 4),	GetBldResourceInfoManual ({})
	),
	BldInfo(CardEnum::MedicineMaker,	_LOCTEXT("Medicine Maker", "Medicine Maker"),LOCTEXT("Medicine Maker (Plural)", "Medicine Makers"),	LOCTEXT("Medicine Maker Desc", "Make Medicine from Medicinal Herb."),
		WorldTile2(4, 4),	GetBldResourceInfo(2, {ResourceEnum::Herb, ResourceEnum::Medicine}, {1, 1, 1, 1}, 0, 100, -1)
	),

	
	BldInfo(CardEnum::FurnitureWorkshop, _LOCTEXT("Furniture Workshop", "Furniture Workshop"), LOCTEXT("Furniture Workshop (Plural)", "Furniture Workshops"), LOCTEXT("Furniture Workshop Desc", "Make Furniture from Wood."),
		{ WorldTile2(6, 7), WorldTile2(6, 6)},	GetBldResourceInfo(1, {ResourceEnum::Wood, ResourceEnum::Furniture}, {2, 1})
	),
	BldInfo(CardEnum::Chocolatier,	_LOCTEXT("Chocolatier", "Chocolatier"),	LOCTEXT("Chocolatier (Plural)", "Chocolatiers"), LOCTEXT("Chocolatier Desc", "Make Chocolate from Milk and Cocoa."),
		WorldTile2(6, 8),	GetBldResourceInfo(3, {ResourceEnum::Cocoa, ResourceEnum::Milk, ResourceEnum::Chocolate}, {0, 1, 1, 1}, 80)
	),

	// Decorations
	BldInfo(CardEnum::Garden,	_LOCTEXT("Garden", "Garden"), LOCTEXT("Garden (Plural)", "Gardens"),	LOCTEXT("Garden Desc", "Increase the surrounding appeal by 20 within 12 tiles radius."),
		WorldTile2(4, 4),	GetBldResourceInfoMoney(500)
	),
	
	BldInfo(CardEnum::BoarBurrow,	_LOCTEXT("Boar Burrow", "Boar Burrow"), LOCTEXT("Boar Burrow (Plural)", "Boar Burrows"),	LOCTEXT("Boar Burrow Desc", "A cozy burrow occupied by a Boar family."),
		WorldTile2(3, 3),	GetBldResourceInfoManual({})
	),

	BldInfo(CardEnum::DirtRoad, _LOCTEXT("Dirt Road", "Dirt Road"),		LOCTEXT("Dirt Road (Plural)", "Dirt Road"), LOCTEXT("Dirt Road Desc", "A crude road.\n +20% movement speed."),
		WorldTile2(1, 1), GetBldResourceInfoMoney(0)
	),
	BldInfo(CardEnum::StoneRoad, _LOCTEXT("Stone Road", "Stone Road"),	LOCTEXT("Stone Road (Plural)", "Stone Road"), LOCTEXT("Stone Road Desc", "A sturdy road.\n +30% movement speed."),
		WorldTile2(1, 1), GetBldResourceInfoManual({ 0, GameConstants::StoneNeededPerRoadTile })
	),
	BldInfo(CardEnum::Demolish, _LOCTEXT("Demolish", "Demolish"),	FText(), INVTEXT("Demolish Buildings."),
		WorldTile2(1, 1), GetBldResourceInfoMoney(0)
	),

	BldInfo(CardEnum::Fisher, _LOCTEXT("Fishing Lodge", "Fishing Lodge"), LOCTEXT("Fishing Lodge (Plural)", "Fishing Lodges"), LOCTEXT("Fishing Lodge Desc", "Catch Fish from seas, lakes or rivers."),
		WorldTile2(6, 4), GetBldResourceInfo(1, { ResourceEnum::Fish }, {1}, -30)
	),
	BldInfo(CardEnum::BlossomShrine, _LOCTEXT("Blossom Shrine", "Blossom Shrine"),	LOCTEXT("Blossom Shrine (Plural)", "Blossom Shrines"), FText(),
		WorldTile2(3, 3), GetBldResourceInfoManual({0, 10})
	),
	BldInfo(CardEnum::Winery, _LOCTEXT("Winery", "Winery"),	LOCTEXT("Winery (Plural)", "Wineries"), LOCTEXT("Winery Desc", "Ferment Grapes into Wine."),
		WorldTile2(6, 9), GetBldResourceInfo(2, { ResourceEnum::Grape, ResourceEnum::Wine }, { 1, 0, 0, 1 }, 50, 100, 1)
	),

	BldInfo(CardEnum::Library, _LOCTEXT("Library", "Library"),	LOCTEXT("Library (Plural)", "Libraries"), LOCTEXT("Library Desc", "+100%<img id=\"Science\"/> for surrounding Houses (effect doesn't stack)"),
		WorldTile2(4, 6), GetBldResourceInfo(2, {}, { 3, 1 }, 0, 100, -999)
	),
	BldInfo(CardEnum::School, _LOCTEXT("School", "School"),	LOCTEXT("School (Plural)", "Schools"), LOCTEXT("School Desc", "+120%<img id=\"Science\"/> for surrounding Houses (effect doesn't stack)"),
		WorldTile2(6, 8), GetBldResourceInfo(3, {}, { 2, 1, 1, 1 }, 50, 100, -999)
	),
	
	BldInfo(CardEnum::Theatre, _LOCTEXT("Theatre", "Theatre"),	LOCTEXT("Theatre (Plural)", "Theatres"), LOCTEXT("Theatre Desc", "Increase visitor's Fun. Base Service quality 70."),
		WorldTile2(10, 8), GetBldResourceInfo(3, {}, { 0, 0, 1, 1, 1 }, 0, 100, -999)
	),
	BldInfo(CardEnum::Tavern, _LOCTEXT("Tavern", "Tavern"),	LOCTEXT("Tavern (Plural)", "Taverns"), LOCTEXT("Tavern Desc", "Increase visitor's Entertainment Happiness <img id=\"Smile\"/>. Base Service quality 50."),
		WorldTile2(6, 8), GetBldResourceInfo(1, {}, { 1, 1 }, 50, 100, -999)
	),

	BldInfo(CardEnum::Tailor, _LOCTEXT("Tailor", "Tailor"),	LOCTEXT("Tailor (Plural)", "Tailors"), LOCTEXT("Tailor Desc", "Make Clothes from Leather or Wool."),
		WorldTile2(8, 5), GetBldResourceInfo(2, { ResourceEnum::Leather, ResourceEnum::Cloth }, { 3, 2, 1 }, 50, 100, 3)
	),

	BldInfo(CardEnum::CharcoalMaker, _LOCTEXT("Charcoal Burner", "Charcoal Burner"), LOCTEXT("Charcoal Burner (Plural)", "Charcoal Burners"), LOCTEXT("Charcoal Burner Desc", "Burn Wood into Coal which provides x2 heat when heating houses."),
		WorldTile2(6, 6), GetBldResourceInfo(1, { ResourceEnum::Wood, ResourceEnum::Coal }, { 1 }, -30)
	),
	BldInfo(CardEnum::BeerBrewery, _LOCTEXT("Beer Brewery", "Beer Brewery"),	LOCTEXT("Beer Brewery (Plural)", "Beer Breweries"), LOCTEXT("Beer Brewery Desc", "Brew Wheat into Beer."),
		WorldTile2(6, 6), GetBldResourceInfo(1, { ResourceEnum::Wheat, ResourceEnum::Beer }, { 1, 1 }, 30)
	),
	BldInfo(CardEnum::ClayPit, _LOCTEXT("Claypit", "Claypit"),		LOCTEXT("Claypit (Plural)", "Claypits"), LOCTEXT("Claypit Desc", "Produce Clay, used to make Pottery or Brick. Must be built next to a river."),
		WorldTile2(6, 6), GetBldResourceInfo(1, { ResourceEnum::Clay }, { 1, 1 }, -30)
	),
	BldInfo(CardEnum::Potter, _LOCTEXT("Potter", "Potter"), LOCTEXT("Potter (Plural)", "Potters"), LOCTEXT("Potter Desc", "Make Pottery from Clay."),
		{ WorldTile2(5, 6), WorldTile2(6, 6) }, GetBldResourceInfo(1, { ResourceEnum::Clay, ResourceEnum::Pottery }, { 1, 2 })
	),
	BldInfo(CardEnum::HolySlimeRanch, _INVTEXT("Holy Slime Ranch"),	FText(), INVTEXT("Raise holy slime. Bonus from nearby slime ranches/pyramid"),
		WorldTile2(10, 10), GetBldResourceInfoManual({})
	),

	BldInfo(CardEnum::TradingPost, _LOCTEXT("Trading Post", "Trading Post"),		LOCTEXT("Trading Post (Plural)", "Trading Posts"), LOCTEXT("Trading Post Desc", "Trade resources with world market."),
		WorldTile2(8, 8), GetBldResourceInfoManual({50, 50})
	),
	BldInfo(CardEnum::TradingCompany, _LOCTEXT("Trading Company", "Trading Company"), LOCTEXT("Trading Company (Plural)", "Trading Companies"), LOCTEXT("Trading Company Desc", "Automatically trade resources with world market with low trade fees."),
		WorldTile2(6, 6), GetBldResourceInfoManual({ 50, 50 }, 0, 2)
	),
	BldInfo(CardEnum::TradingPort, _LOCTEXT("Trading Port", "Trading Port"),		LOCTEXT("Trading Port (Plural)", "Trading Ports"), LOCTEXT("Trading Port Desc", "Trade resources with world market. Must be built on the coast."),
		WorldTile2(10, 9), GetBldResourceInfoManual({ 50, 50 })
	),
	BldInfo(CardEnum::CardMaker, _LOCTEXT("Scholars Office", "Scholars Office"), LOCTEXT("Scholars Office (Plural)", "Scholars Offices"), LOCTEXT("Scholars Office Desc", "Craft a Card from Paper."),
		WorldTile2(6, 6), GetBldResourceInfo(2, { ResourceEnum::Paper, ResourceEnum::None }, { 1, 1, 1, 1 }, 80)
		//Old: WorldTile2(10, 8)
	),
	BldInfo(CardEnum::ImmigrationOffice, _LOCTEXT("Immigration Office", "Immigration Office"), LOCTEXT("Immigration Office (Plural)", "Immigration Offices"), LOCTEXT("Immigration Office Desc", "Attract new immigrants."),
		WorldTile2(6, 6), GetBldResourceInfoManual({50}, 1)
	),

	/**/BldInfo(CardEnum::ThiefGuild, _LOCTEXT("Thief Guild", "Thief Guild"),	LOCTEXT("Thief Guild (Plural)", "Thief Guilds"), FText(),
		WorldTile2(5, 7), GetBldResourceInfoManual({})
	),
	/**/BldInfo(CardEnum::SlimePyramid, _LOCTEXT("Pyramid", "Pyramid"),			LOCTEXT("Pyramid (Plural)", "Pyramids"), FText(),
		WorldTile2(14, 14), GetBldResourceInfoManual({})
	),

	/**/BldInfo(CardEnum::LovelyHeartStatue, _INVTEXT("LovelyHeartStatue"),		FText(), INVTEXT("Mysterious giant hearts."),
		WorldTile2(4, 4), GetBldResourceInfoManual({})
	),

	BldInfo(CardEnum::HuntingLodge, _LOCTEXT("Hunting Lodge", "Hunting Lodge"), LOCTEXT("Hunting Lodge (Plural)", "Hunting Lodges"), LOCTEXT("Hunting Lodge Desc", "Hunt wild animals for food."),
		WorldTile2(4, 6), GetBldResourceInfoManual({30}, 2)
	),
	/**/BldInfo(CardEnum::RanchBarn, _INVTEXT("Ranch Barn"),						FText(), FText(),
		WorldTile2(4, 4), GetBldResourceInfoManual({})
	),

	BldInfo(CardEnum::RanchPig, _LOCTEXT("Pig Ranch", "Pig Ranch"),		LOCTEXT("Pig Ranch (Plural)", "Pig Ranches"), LOCTEXT("Pig Ranch Desc", "Rear Pigs for food."),
		WorldTile2(18, 12), GetBldResourceInfoManual({ 30, 10}, 1)
	),
	BldInfo(CardEnum::RanchSheep, _LOCTEXT("Sheep Ranch", "Sheep Ranch"),	LOCTEXT("Sheep Ranch (Plural)", "Sheep Ranches"), LOCTEXT("Sheep Ranch Desc", "Rear Sheep for food and Wool."),
		WorldTile2(18, 12), GetBldResourceInfoManual({ 40, 40 }, 1)
	),
	BldInfo(CardEnum::RanchCow, _LOCTEXT("Cattle Ranch", "Cattle Ranch"), LOCTEXT("Cattle Ranch (Plural)", "Cattle Ranches"), LOCTEXT("Cattle Ranch Desc", "Rear Dairy Cows for Milk"),
		WorldTile2(18, 12), GetBldResourceInfoManual({ 40, 40, 20 }, 1)
	),

	
	BldInfo(CardEnum::GoldSmelter, _LOCTEXT("Gold Smelter", "Gold Smelter"), LOCTEXT("Gold Smelter (Plural)", "Gold Smelters"), LOCTEXT("Gold Smelter Desc", "Smelt Gold Ores into Gold Bars."),
		WorldTile2(6, 6), GetBldResourceInfo(3, { ResourceEnum::GoldOre, ResourceEnum::Coal, ResourceEnum::GoldBar }, { 0, 2, 1, 2 }, 50)
	),
	BldInfo(CardEnum::Mint, _LOCTEXT("Mint", "Mint"),				LOCTEXT("Mint (Plural)", "Mints"), LOCTEXT("Mint Desc", "Mint Gold Bars into <img id=\"Coin\"/>."),
		WorldTile2(6, 6), GetBldResourceInfo(3, { ResourceEnum::GoldBar, ResourceEnum::None }, { 0, 0, 1, 3 }, 50)
	),

	BldInfo(CardEnum::BarrackClubman, _LOCTEXT("Clubman Barracks", "Clubman Barracks"), LOCTEXT("Clubman Barracks (Plural)", "Clubman Barracks"), LOCTEXT("Clubman Barracks Desc", "Train Clubmen."),
		WorldTile2(7, 7), GetBldResourceInfo(1, {}, {})
	),
	BldInfo(CardEnum::BarrackSwordman, _LOCTEXT("Knight Barracks", "Knight Barracks"), LOCTEXT("Knight Barracks (Plural)", "Knight Barracks"), LOCTEXT("Knight Barracks Desc", "Consume Iron to increase <img id=\"Influence\"/>."),
		WorldTile2(7, 7), GetBldResourceInfo(2, { ResourceEnum::Iron, ResourceEnum::None }, { 1, 1, 1 })
	),
	BldInfo(CardEnum::BarrackArcher, _LOCTEXT("Archer Barracks", "Archer Barracks"), LOCTEXT("Archer Barracks (Plural)", "Archer Barracks"), LOCTEXT("Archer Barracks Desc", "Consume Wood to increase <img id=\"Influence\"/>."),
		WorldTile2(7, 7), GetBldResourceInfo(1, { ResourceEnum::Wood, ResourceEnum::None }, { 2, 1 })
	),

	/**/BldInfo(CardEnum::ShrineWisdom, _LOCTEXT("Shrine of Wisdom", "Shrine of Wisdom"),	LOCTEXT("Shrine of Wisdom (Plural)", "Shrines of Wisdom"), INVTEXT("+1 Wild Card to the deck."),
		WorldTile2(4, 4), GetBldResourceInfoManual({})
	),
	/**/BldInfo(CardEnum::ShrineLove, _LOCTEXT("Shrine of Love", "Shrine of Love"),		LOCTEXT("Shrine of Love (Plural)", "Shrines of Love"), INVTEXT("+5<img id=\"Smile\"/> to surrounding houses."),
		WorldTile2(4, 4), GetBldResourceInfoManual({})
	),
	/**/BldInfo(CardEnum::ShrineGreed, _LOCTEXT("Shrine of Greed", "Shrine of Greed"),		LOCTEXT("Shrine of Greed (Plural)", "Shrines of Greed"), INVTEXT("+20<img id=\"Coin\"/> per round and -5<img id=\"Smile\"/> to surrounding houses."),
		WorldTile2(4, 4), GetBldResourceInfoManual({})
	),


	/**/BldInfo(CardEnum::HellPortal, _LOCTEXT("Hell Portal", "Hell Portal"),			LOCTEXT("Hell Portal (Plural)", "Hell Portals"), INVTEXT("Open a portal to hell to harvest its riches. Gain 200 <img id=\"Coin\"/> each round, but may explode letting demons through."),
		WorldTile2(6, 6), GetBldResourceInfoManual({})
	),
	/**/BldInfo(CardEnum::LaborerGuild, _LOCTEXT("Laborer's Guild", "Laborer's Guild"),	LOCTEXT("Laborer's Guild (Plural)", "Laborer's Guilds"), LOCTEXT("Laborer's Guild Desc", "Dispatch laborers to haul resources and cut trees. Laborers from the guild has x2 inventory carry capacity."),
		WorldTile2(4, 6), GetBldResourceInfoManual({})
	),

	BldInfo(CardEnum::HumanitarianAidCamp, _LOCTEXT("Humanitarian Aid Camp", "Humanitarian Aid Camp"),	LOCTEXT("Humanitarian Aid Camp (Plural)", "Humanitarian Aid Camps"), LOCTEXT("Humanitarian Aid Camp Desc", "Supply your neighbor with 100 food."),
		WorldTile2(4, 4), GetBldResourceInfoMoney(500)
	),

	BldInfo(CardEnum::RegionTribalVillage, _LOCTEXT("Tribal Village", "Tribal Village"), LOCTEXT("Tribal Village (Plural)", "Tribal Villages"), LOCTEXT("Tribal Village Desc", "Tribal village that might want to join your city."),
		WorldTile2(9, 9), GetBldResourceInfoManual({})
	),
	BldInfo(CardEnum::RegionShrine, _LOCTEXT("Ancient Shrine", "Ancient Shrine"),		LOCTEXT("Ancient Shrine (Plural)", "Ancient Shrines"), LOCTEXT("Ancient Shrine Desc", "Shrines built by the ancients that grants wisdom to those who seek it."),
		WorldTile2(4, 6), GetBldResourceInfoManual({})
	),
	BldInfo(CardEnum::RegionPort, _LOCTEXT("Port Settlement", "Port Settlement"),		LOCTEXT("Port Settlement (Plural)", "Port Settlements"), INVTEXT("Port settlement specialized in trading certain resources."),
		WorldTile2(12, 9), GetBldResourceInfoManual({})
	),
	BldInfo(CardEnum::RegionCrates, _LOCTEXT("Crates", "Crates"),						LOCTEXT("Crates (Plural)", "Crates"), LOCTEXT("Crates Desc", "Crates that may contain valuable resources."),
		WorldTile2(4, 6), GetBldResourceInfoManual({})
	),
	

	// June 1 addition
	BldInfo(CardEnum::Windmill, _LOCTEXT("Windmill", "Windmill"),				LOCTEXT("Windmill (Plural)", "Windmills"), LOCTEXT("Windmill Desc", "Grinds Wheat into Wheat Flour. +10% Productivity to surrounding Farms."),
		WorldTile2(5, 5), GetBldResourceInfo(2, { ResourceEnum::Wheat, ResourceEnum::Flour }, { 1 })
	),
	BldInfo(CardEnum::Bakery, _LOCTEXT("Bakery", "Bakery"),					LOCTEXT("Bakery (Plural)", "Bakeries"), LOCTEXT("Bakery Desc", "Bakes Bread with Wheat Flour and heat."),
		WorldTile2(6, 6), GetBldResourceInfo(2, { ResourceEnum::Flour, ResourceEnum::Coal, ResourceEnum::Bread }, { 0, 1, 0, 2 })
	),
	BldInfo(CardEnum::GemstoneMine, _LOCTEXT("Gemstone Mine", "Gemstone Mine"),	LOCTEXT("Gemstone Mine (Plural)", "Gemstone Mines"), LOCTEXT("Gemstone Mine Desc", "Mine Gemstone from Gemstone Deposits."),
		WorldTile2(6, 5), GetBldResourceInfo(1, { ResourceEnum::Gemstone }, { 3,5 }, 2)
	),
	BldInfo(CardEnum::Jeweler, _LOCTEXT("Jeweler", "Jeweler"),					LOCTEXT("Jeweler (Plural)", "Jewelers"), LOCTEXT("Jeweler Desc", "Craft Gemstone and Gold Bar into Jewelry."),
		WorldTile2(5, 5), GetBldResourceInfo(4, { ResourceEnum::Gemstone, ResourceEnum::GoldBar, ResourceEnum::Jewelry }, { 0,1,1,1,1 }, -6)
	),

	// June 9 addition
	BldInfo(CardEnum::Beekeeper, _LOCTEXT("Beekeeper", "Beekeeper"),				LOCTEXT("Beekeeper (Plural)", "Beekeepers"), LOCTEXT("Beekeeper Desc", "Produces Beeswax and Honey. Efficiency increases with more surrounding trees."),
		WorldTile2(6, 8), GetBldResourceInfo(2, { ResourceEnum::Beeswax }, { 1, 1}, -1)
	),
	BldInfo(CardEnum::Brickworks, _LOCTEXT("Brickworks", "Brickworks"),			LOCTEXT("Brickworks (Plural)", "Brickworks"), LOCTEXT("Brickworks Desc", "Produces Brick from Clay and Coal."),
		WorldTile2(8, 6), GetBldResourceInfo(2, { ResourceEnum::Clay, ResourceEnum::Coal, ResourceEnum::Brick }, { 1, 2 }, 0, 100, 3)
	),
	BldInfo(CardEnum::CandleMaker, _LOCTEXT("Candle Maker", "Candle Maker"),		LOCTEXT("Candle Maker (Plural)", "Candle Makers"), LOCTEXT("Candle Maker Desc", "Make Candles from Beeswax."),
		WorldTile2(4, 6), GetBldResourceInfo(2, { ResourceEnum::Beeswax, ResourceEnum::Candle }, { 3, 2 }, 0)
	),
	BldInfo(CardEnum::CottonMill, _LOCTEXT("Cotton Mill", "Cotton Mill"),		LOCTEXT("Cotton Mill (Plural)", "Cotton Mills"), LOCTEXT("Cotton Mill Desc", "Mass-produce Cotton into Cotton Fabric."),
		WorldTile2(10, 16), GetBldResourceInfo(4, { ResourceEnum::Cotton, ResourceEnum::None, ResourceEnum::CottonFabric }, { 1, 0, 0, 5, 1, 0, 2 }, 0)
	),
	BldInfo(CardEnum::PrintingPress, _LOCTEXT("Printing Press", "Printing Press"), LOCTEXT("Printing Press (Plural)", "Printing Presses"), LOCTEXT("Printing Press Desc", "Print Books."),
		WorldTile2(8, 6), GetBldResourceInfo(4, { ResourceEnum::Paper, ResourceEnum::Dye, ResourceEnum::Book }, { 1, 0, 0, 5, 1, 0, 2 }, 0, 100, -3)
	),

	// June 25 addition
	BldInfo(CardEnum::Warehouse, _LOCTEXT("Warehouse", "Warehouse"),	LOCTEXT("Warehouse (Plural)", "Warehouses"), LOCTEXT("Warehouse Desc", "Advanced storage with 30 storage slots."),
		WorldTile2(6, 4), GetBldResourceInfoManual({ 50, 0, 0, 50 })
	),
	BldInfo(CardEnum::Fort, _LOCTEXT("Fort", "Fort"),				LOCTEXT("Fort (Plural)", "Forts"), LOCTEXT("Fort Desc", "Place on choke point for raid protection. +200% defense on protected provinces."),
		WorldTile2(9, 9), GetBldResourceInfoMoney(3000)
	),
	BldInfo(CardEnum::ResourceOutpost, _LOCTEXT("Resource Camp", "Resource Camp"),	LOCTEXT("Resource Camp (Plural)", "Resource Camps"), LOCTEXT("Resource Camp Desc", "Extract resource from province."),
		WorldTile2(10, 10), GetBldResourceInfoMoney(10000)
	),
	BldInfo(CardEnum::ResearchLab, _LOCTEXT("Research Lab", "Research Lab"), LOCTEXT("Research Lab (Plural)", "Research Labs"), LOCTEXT("Research Lab Desc", "Generate Science Points. Uses Paper as input."),
		WorldTile2(6, 6), GetBldResourceInfo(3, { ResourceEnum::Paper, ResourceEnum::None }, {1, 0, 0, 1, 1}, 0, 100, -2)
	),
	BldInfo(CardEnum::IntercityRoad, _LOCTEXT("Intercity Road", "Intercity Road"),	LOCTEXT("Intercity Road (Plural)", "Intercity Road"), LOCTEXT("Intercity Road Desc", "Build Road to connect with other Cities. Same as Dirt Road, but buildable outside your territory."),
		WorldTile2(1, 1), GetBldResourceInfoManual({})
	),

	// August 16
	BldInfo(CardEnum::FakeTownhall, _INVTEXT("FakeTownhall"),	FText(), FText(),
		WorldTile2(12, 12), GetBldResourceInfoManual({})
	),
	BldInfo(CardEnum::FakeTribalVillage, _INVTEXT("FakeTribalVillage"),	FText(), FText(),
		WorldTile2(12, 12), GetBldResourceInfoManual({})
	),
	BldInfo(CardEnum::ChichenItza, _LOCTEXT("Chichen Itza", "Chichen Itza"), FText(), FText(),
		WorldTile2(16, 16), GetBldResourceInfoManual({})
	),

	// October 20
	BldInfo(CardEnum::Market, _LOCTEXT("Market", "Market"),	LOCTEXT("Market (Plural)", "Markets"), LOCTEXT("Market Desc", "Supplies anything a household needs within its radius. Workers carry 50 units."),
		WorldTile2(6, 12), GetBldResourceInfoManual({ 50 }, 3)
	),
	BldInfo(CardEnum::ShippingDepot, _LOCTEXT("Logistics Office", "Logistics Office"),	LOCTEXT("Logistics Office (Plural)", "Logistics Offices"), LOCTEXT("Logistics Office Desc", "Haul specified resources from within the radius to its delivery target in 50-units bulk."),
		WorldTile2(4, 4), GetBldResourceInfoManual({ 30, 10 }, 1)
	),
	BldInfo(CardEnum::IrrigationReservoir, _LOCTEXT("Irrigation Reservoir", "Irrigation Reservoir"), LOCTEXT("Irrigation Reservoir (Plural)", "Irrigation Reservoirs"), LOCTEXT("Irrigation Reservoir Desc", "Raises fertility within its radius to 100%."),
		WorldTile2(5, 5), GetBldResourceInfoManual({ 0, 150 })
	),

	// November 18
	BldInfo(CardEnum::Tunnel, _LOCTEXT("Tunnel", "Tunnel"), LOCTEXT("Tunnel (Plural)", "Tunnels"), LOCTEXT("Tunnel Desc", "Allow citizens to walk through mountain."),
		WorldTile2(1, 1), GetBldResourceInfoMoney(3000)
	),
	BldInfo(CardEnum::GarmentFactory, _LOCTEXT("Garment Factory", "Garment Factory"), LOCTEXT("Garment Factory (Plural)", "Garment Factories"), LOCTEXT("Garment Factory Desc", "Mass-produce Clothes with Fabrics."),
		WorldTile2(7, 6), GetBldResourceInfo(4, { ResourceEnum::DyedCottonFabric, ResourceEnum::LuxuriousClothes }, { 0, 0, 0, 5, 2, 0, 2 }, 0)
	),

	// December 29
	BldInfo(CardEnum::MagicMushroomFarm, _LOCTEXT("Magic Mushroom Farm", "Magic Mushroom Farm"), LOCTEXT("Magic Mushroom Farm (Plural)", "Magic Mushroom Farms"), LOCTEXT("Magic Mushroom Farm Desc", "Farm Magic Mushroom using wood."),
		WorldTile2(8, 8), GetBldResourceInfo(3, { ResourceEnum::Wood, ResourceEnum::MagicMushroom }, { 1, 1, 0, 1, 1 }, 0, 120, -2)
	),
	BldInfo(CardEnum::VodkaDistillery, _LOCTEXT("Vodka Distillery", "Vodka Distillery"), LOCTEXT("Vodka Distillery (Plural)", "Vodka Distilleries"), LOCTEXT("Vodka Distillery Desc", "Brew Potato into Vodka."),
		WorldTile2(6, 6), GetBldResourceInfo(2, { ResourceEnum::Potato, ResourceEnum::Vodka }, { 1, 1, 1 }, 20)
	),
	BldInfo(CardEnum::CoffeeRoaster, _LOCTEXT("Coffee Roaster", "Coffee Roaster"),		LOCTEXT("Coffee Roaster (Plural)", "Coffee Roasters"), LOCTEXT("Coffee Roaster Desc", "Roast Raw Coffee into Coffee."),
		WorldTile2(5, 5), GetBldResourceInfo(3, { ResourceEnum::RawCoffee, ResourceEnum::Coffee }, { 2, 0, 1, 3 }, 20, 100, -2)
	),

	// February 2
	BldInfo(CardEnum::Colony, _LOCTEXT("Colony", "Colony"), LOCTEXT("Colony (Plural)", "Colonies"), LOCTEXT("Colony Desc", "Build a new city with 10 citizens from your capital."),
		WorldTile2(12, 12), GetBldResourceInfoMoney(15000)
	),
	BldInfo(CardEnum::PortColony, _LOCTEXT("Port Colony", "Port Colony"), LOCTEXT("Port Colony (Plural)", "Port Colonies"), LOCTEXT("Port Colony Desc", "Build a new port city with 10 citizens from your capital."),
		WorldTile2(12, 12), GetBldResourceInfoMoney(20000)
	),
	BldInfo(CardEnum::IntercityLogisticsHub, _LOCTEXT("Intercity Logistics Hub", "Intercity Logistics Hub"), LOCTEXT("Intercity Logistics Hub (Plural)", "Intercity Logistics Hubs"), LOCTEXT("Intercity Logistics Hub Desc", "Take(Import) resources from another city (land)."),
		WorldTile2(6, 6), GetBldResourceInfoManual({ 50, 20 }, 1)
	),
	BldInfo(CardEnum::IntercityLogisticsPort, _LOCTEXT("Intercity Logistics Port", "Intercity Logistics Port"), LOCTEXT("Intercity Logistics Port (Plural)", "Intercity Logistics Ports"), LOCTEXT("Intercity Logistics Port Desc", "Take(Import) resources from another city (water)."),
		WorldTile2(12, 6), GetBldResourceInfoManual({ 80, 20 }, 1)
	),
	BldInfo(CardEnum::IntercityBridge, _LOCTEXT("Intercity Bridge", "Intercity Bridge"), LOCTEXT("Intercity Bridge (Plural)", "Intercity Bridges"), LOCTEXT("Intercity Bridge Desc", "Bridge that can be built outside your territory to connect Cities."),
		WorldTile2(1, 1), GetBldResourceInfoMoney(1000)
	),

	// March 12
	BldInfo(CardEnum::Granary, _LOCTEXT("Granary", "Granary"), LOCTEXT("Granary (Plural)", "Granaries"), LOCTEXT("Granary Desc", "Food Storage. +25% Productivity to surrounding Food Producers."),
		WorldTile2(6, 6), GetBldResourceInfoManual({ 50, 50 })
	),
	BldInfo(CardEnum::Archives, _LOCTEXT("Archives", "Archives"), LOCTEXT("Archives (Plural)", "Archives"), LOCTEXT("Archives Desc", "Store any Cards. Additionally, earn Income equals to 12% of the Card Price per year."),
		WorldTile2(6, 6), GetBldResourceInfoManual({ 50, 50 })
	),
	BldInfo(CardEnum::HaulingServices, _LOCTEXT("Hauling Services", "Hauling Services"), LOCTEXT("Hauling Services (Plural)", "Hauling Services"), LOCTEXT("Hauling Services Desc", "Workers use carts to haul resources to fill building inputs or clear building outputs."),
		WorldTile2(6, 5), GetBldResourceInfoManual({ 50 }, 3)
	),

	// Apr 1
	BldInfo(CardEnum::SandMine, _LOCTEXT("SandMine", "Sand Mine"), LOCTEXT("Sand Mine (Plural)", "Sand Mines"), LOCTEXT("Sand Mine Desc", "Extract Sand from beach or river. Sand can be used to make Glass."),
		WorldTile2(10, 6), GetBldResourceInfo(3, { ResourceEnum::Sand }, { 1, 1, 1, 5 }, -30)
	),
	BldInfo(CardEnum::GlassSmelter, _LOCTEXT("GlassSmelter", "Glass Smelter"), LOCTEXT("GlassSmelter (Plural)", "GlassSmelters"), LOCTEXT("GlassSmelter Desc", "Produce Glass from Sand and Coal."),
		WorldTile2(8, 8), GetBldResourceInfo(3, { ResourceEnum::Sand, ResourceEnum::Coal, ResourceEnum::Glass }, { 1, 1, 1, 5 }, 0)
	),
	BldInfo(CardEnum::Glassworks, _LOCTEXT("Glassworks", "Glassworks"), LOCTEXT("Glassworks (Plural)", "Glassworks"), LOCTEXT("Glassworks Desc", "Produce Glassware from Glass and Coal."),
		WorldTile2(8, 8), GetBldResourceInfo(3, { ResourceEnum::Glass, ResourceEnum::Coal, ResourceEnum::Glassware }, { 1, 1, 1, 5 }, 0)
	),
	BldInfo(CardEnum::ConcreteFactory, _LOCTEXT("ConcreteFactory", "Concrete Factory"), LOCTEXT("Concrete Factory (Plural)", "Concrete Factories"), LOCTEXT("Concrete Factory Desc", "Make Concrete from Stone and Sand."),
		WorldTile2(8, 8), GetBldResourceInfo(4, { ResourceEnum::Stone, ResourceEnum::Sand, ResourceEnum::Concrete }, { 0, 0, 1, 5 }, 0, 100, -3)
	),
	BldInfo(CardEnum::CoalPowerPlant, _LOCTEXT("CoalPowerPlant", "Coal Power Plant"), LOCTEXT("CoalPowerPlant (Plural)", "Coal Power Plants"), LOCTEXT("Coal Power Plants Desc", "Provide Electricity converting 1 Coal to 1 kWh of Electricity."),
		WorldTile2(8, 12), GetBldResourceInfo(4, { ResourceEnum::Coal, ResourceEnum::None }, { 0, 0, 0, 5, 0, 5, 3 }, 0, 100, -8)
	),
	BldInfo(CardEnum::IndustrialIronSmelter, _LOCTEXT("Industrial Iron Smelter", "Industrial Iron Smelter"), LOCTEXT("Industrial Iron Smelter (Plural)", "Industrial Iron Smelters"), LOCTEXT("Industrial Iron Smelter Desc", "Produce Iron Bars on the industrial scale."),
		WorldTile2(10, 14), GetBldResourceInfo(4, { ResourceEnum::IronOre, ResourceEnum::Coal, ResourceEnum::Iron }, { 0, 0, 0, 5, 0 }, 20)
	),
	BldInfo(CardEnum::Steelworks, _LOCTEXT("Steelworks", "Steelworks"), LOCTEXT("Steelworks (Plural)", "Steelworks"), LOCTEXT("Steelworks Desc", "Produce Steel from Iron Bars and Coal."),
		WorldTile2(10, 14), GetBldResourceInfo(4, { ResourceEnum::Iron, ResourceEnum::Coal, ResourceEnum::Steel }, { 0, 0, 0, 5, 0, 5 }, 20)
	),
	BldInfo(CardEnum::StoneToolsShop, _LOCTEXT("StoneToolsShop", "Stone Tools Shop"), LOCTEXT("StoneToolsShop (Plural)", "Stone Tools Shops"), LOCTEXT("Stone Tools Shop Desc", "Craft Stone Tools from Stone and Wood."),
		WorldTile2(4, 6), GetBldResourceInfo(1, { ResourceEnum::Stone, ResourceEnum::Wood, ResourceEnum::StoneTools }, { 1, 1 })
	),
	BldInfo(CardEnum::OilRig, _LOCTEXT("Oil Rig", "Oil Rig"), LOCTEXT("Oil Rig (Plural)", "Oil Rigs"), LOCTEXT("Oil Rig Desc", "Extract Oil from Oil Well."),
		WorldTile2(6, 6), GetBldResourceInfo(4, { ResourceEnum::Oil }, { 0, 0, 0, 0, 0, 1, 5 }, 0, 150, -5)
	),
	BldInfo(CardEnum::OilPowerPlant, _LOCTEXT("OilPowerPlant", "Oil Power Plant"), LOCTEXT("Oil Power Plant (Plural)", "Oil Power Plants"), LOCTEXT("Oil Power Plant Desc", "Provide Electricity converting 1 Oil to 2 kWh of Electricity."),
		WorldTile2(8, 12), GetBldResourceInfo(4, { ResourceEnum::Oil, ResourceEnum::None }, { 0, 0, 0, 5, 0, 5, 5 }, 30, 100, -8)
	),
	BldInfo(CardEnum::PaperMill, _LOCTEXT("PaperMill", "Paper Mill"), LOCTEXT("Paper Mill (Plural)", "Paper Mills"), LOCTEXT("Paper Mill Desc", "Mass-produce Paper from Wood."),
		WorldTile2(10, 16), GetBldResourceInfo(4, { ResourceEnum::Wood, ResourceEnum::Paper }, { 0, 0, 0, 5, 0, 0, 5 }, 0, 100, -2)
	),
	BldInfo(CardEnum::ClockMakers, _LOCTEXT("ClockMakers", "Clock Makers"), LOCTEXT("Clock Makers (Plural)", "Clock Makers"), LOCTEXT("Clock Makers Desc", "Craft Pocket Watch from Glass and Gold Bars."),
		WorldTile2(8, 6), GetBldResourceInfo(4, { ResourceEnum::Glass, ResourceEnum::GoldBar, ResourceEnum::PocketWatch }, { 0, 0, 0, 5, 10, 5, 0 }, 0, 100, -3)
	),

	BldInfo(CardEnum::Cathedral, _LOCTEXT("Cathedral", "Cathedral"), LOCTEXT("Cathedral (Plural)", "Cathedrals"), LOCTEXT("Cathedral Desc", "First Cathedral grants X Victory Score. +10% Job Happiness in the city"),
		WorldTile2(21, 13), GetBldResourceInfo(3, {}, { 0, 0, 0, 5, 2 }, 5000, 100, -999)
	),
	BldInfo(CardEnum::Castle, _LOCTEXT("Castle", "Castle"), LOCTEXT("Castle (Plural)", "Castles"), LOCTEXT("Castle Desc", "First Castle grants X Victory Score. +15%<img id=\"Influence\"/> Income from Houses"),
		WorldTile2(16, 16), GetBldResourceInfo(3, {}, { 0, 5, 1, 0, 1 }, 8000, 100, -999)
	),
	BldInfo(CardEnum::GrandPalace, _LOCTEXT("Grand Palace", "Grand Palace"), LOCTEXT("Grand Palace (Plural)", "Grand Palaces"), LOCTEXT("Grand Palace Desc", "First Grand Palace grants X Victory Score. +20% Building Appeal in the city. "),
		WorldTile2(18, 26), GetBldResourceInfo(4, {}, { 0, 0, 0, 2, 1, 5, 1 }, 20000, 100, -999)
	),
	BldInfo(CardEnum::ExhibitionHall, _LOCTEXT("ExhibitionHall", "Exhibition Hall"), LOCTEXT("Exhibition Hall (Plural)", "Exhibition Halls"), LOCTEXT("Exhibition Hall Desc", "First Exhibition Hall grants X Victory Score. +20%<img id=\"Coin\"/> from luxury consumption"),
		WorldTile2(36, 24), GetBldResourceInfo(4, {}, { 0, 0, 0, 0, 2, 1, 5 }, 20000, 100, -999)
	),

	
	// Decorations
	BldInfo(CardEnum::FlowerBed, _LOCTEXT("Flower Bed", "Flower Bed"),		LOCTEXT("Flower Bed (Plural)", "Flower Beds"), LOCTEXT("Flower Bed Desc", "Increase the surrounding appeal by 5 within 5 tiles radius."),
		WorldTile2(1, 1), GetBldResourceInfoMoney(100)
	),
	BldInfo(CardEnum::GardenShrubbery1, _LOCTEXT("Shrubbery", "Shrubbery"),			LOCTEXT("Shrubbery (Plural)", "Shrubberies"), LOCTEXT("Shrubbery Desc", "Increase the surrounding appeal by 5 within 5 tiles radius."),
		WorldTile2(1, 1), GetBldResourceInfoMoney(100)
	),
	BldInfo(CardEnum::GardenCypress, _LOCTEXT("Garden Cypress", "Garden Cypress"), LOCTEXT("Garden Cypress (Plural)", "Garden Cypresses"), LOCTEXT("Garden Cypress Desc", "Increase the surrounding appeal by 8 within 5 tiles radius."),
		WorldTile2(1, 1), GetBldResourceInfoMoney(200)
	),	
	
	// Rare cards
	//BldInfo("Sales Office",			WorldTile2(4, 5),		ResourceEnum::None, ResourceEnum::None, ResourceEnum::None,		0,	{20, 20, 0},	"Decrease the amount of trade fee by 50%."),
	BldInfo(CardEnum::ArchitectStudio, _LOCTEXT("Architect's Studio", "Architect's Studio"),		LOCTEXT("Architect's Studio (Plural)", "Architect's Studios"), LOCTEXT("Architect's Studio Desc", "+5% housing appeal."),
		WorldTile2(4, 5), GetBldResourceInfoManual({0, 0, 0, 0, 0, 30, 30})
	),
	BldInfo(CardEnum::EngineeringOffice, _LOCTEXT("Engineer's Office", "Engineer's Office"),			LOCTEXT("Engineer's Office (Plural)", "Engineer's Offices"), LOCTEXT("Engineer's Office Desc", "+10% industrial production."),
		WorldTile2(4, 5), GetBldResourceInfoManual({ 0, 0, 0, 0, 0, 30, 30 })
	),
	BldInfo(CardEnum::DepartmentOfAgriculture, _LOCTEXT("Ministry of Agriculture", "Ministry of Agriculture"), FText(), LOCTEXT("Ministry of Agriculture Desc", "+5% Farm production when there are 8 or more Farms."),
		WorldTile2(4, 5), GetBldResourceInfoManual({ 0, 0, 0, 0, 0, 30, 30 })
	),
	//BldInfo("Isolationist Publisher", WorldTile2(4, 5),		ResourceEnum::None, ResourceEnum::None, ResourceEnum::None,		0,	{20, 20, 0},	"Double trade fees. -10% food consumption."),

	BldInfo(CardEnum::StockMarket, _LOCTEXT("Stock Market", "Stock Market"),					FText(), LOCTEXT("Stock Market Desc", "Upkeep for Trading Companies reduced to 1<img id=\"Coin\"/>."),
		WorldTile2(4, 5), GetBldResourceInfoManual({ 30, 30 })
	),
	BldInfo(CardEnum::CensorshipInstitute, _LOCTEXT("Censorship Institute", "Censorship Institute"),		FText(), LOCTEXT("Censorship Institute Desc", "-1 card from each hand roll. +7% Farm production."),
		WorldTile2(4, 5), GetBldResourceInfoManual({ 30, 30 })
	),
	BldInfo(CardEnum::EnvironmentalistGuild, _LOCTEXT("Environmentalist Guild", "Environmentalist Guild"), FText(), LOCTEXT("Environmentalist Guild Desc", "+15% house appeal. -30% production for mines, smelters, and Charcoal Makers."),
		WorldTile2(4, 5), GetBldResourceInfoManual({ 30, 30 })
	),
	BldInfo(CardEnum::IndustrialistsGuild, _LOCTEXT("Industrialist Guild", "Industrialist Guild"),		FText(), LOCTEXT("Industrialist Guild Desc", "+20% production to surrounding industries within 10 tiles radius."),
		WorldTile2(4, 5), GetBldResourceInfoManual({ 30, 30 })
	),
	BldInfo(CardEnum::Oracle, _LOCTEXT("Oracle", "Oracle"),									FText(), INVTEXT("Gain one additional choice when selecting Rare Cards."),
		WorldTile2(4, 5), GetBldResourceInfoManual({ 30, 30 })
	),
	BldInfo(CardEnum::AdventurersGuild, _LOCTEXT("Adventurer's Guild", "Adventurer's Guild"),			FText(), INVTEXT("At the end of summer choose a Rare Card."),
		WorldTile2(4, 5), GetBldResourceInfoManual({ 30, 30 })
	),
	
	BldInfo(CardEnum::ConsultingFirm, _INVTEXT("Consulting Firm"),							FText(), INVTEXT("+30% production to surrounding buildings. -30 <img id=\"Coin\"/> per round"),
		WorldTile2(4, 5), GetBldResourceInfoManual({ 30, 30 })
	),
	BldInfo(CardEnum::ImmigrationPropagandaOffice, _INVTEXT("Immigration Propaganda Office"), FText(), INVTEXT("+30% immigration"),
		WorldTile2(4, 5), GetBldResourceInfoManual({ 30, 30 })
	),
	BldInfo(CardEnum::MerchantGuild, _INVTEXT("Merchant Guild"),								FText(), INVTEXT("-5% trade fee."),
		WorldTile2(4, 5), GetBldResourceInfoManual({ 30, 30 })
	),
	BldInfo(CardEnum::OreSupplier, _INVTEXT("Ore Supplier Guild"),										FText(), INVTEXT("Buy selected ore each season with 0% trading fee."),
		WorldTile2(4, 5), GetBldResourceInfoManual({ 30, 30 })
	),
	BldInfo(CardEnum::BeerBreweryFamous, _LOCTEXT("Famous Beer Brewery", "Famous Beer Brewery"),	LOCTEXT("Famous Beer Brewery (Plural)", "Famous Beer Breweries"), LOCTEXT("Famous Beer Brewery Desc", "Large brewery with improved productivity. Require 4 Beer Breweries to unlock."),
		WorldTile2(5, 7), GetBldResourceInfo(1, { ResourceEnum::Wheat, ResourceEnum::Beer }, { 1, 1, 0, 1 })
	),
	BldInfo(CardEnum::IronSmelterGiant, _LOCTEXT("Giant Iron Smelter", "Giant Iron Smelter"),			LOCTEXT("Giant Iron Smelter (Plural)", "Giant Iron Smelters"), INVTEXT("Large smelter."),
		WorldTile2(7, 8), GetBldResourceInfo(2, { ResourceEnum::IronOre, ResourceEnum::Coal, ResourceEnum::Iron }, { 1,1 }, 200)
	),

	
	BldInfo(CardEnum::Cattery, _LOCTEXT("Cattery", "Cattery"),					LOCTEXT("Cattery (Plural)", "Catteries"), INVTEXT("+15% housing appeal to surrounding houses."),
		WorldTile2(4, 5), GetBldResourceInfoManual({ 30, 30 })
	),
	BldInfo(CardEnum::InvestmentBank, _LOCTEXT("Investment Bank", "Investment Bank"), LOCTEXT("Investment Bank (Plural)", "Investment Banks"), INVTEXT("Gain <img id=\"Coin\"/> equals to +10% of <img id=\"Coin\"/> every round."),
		WorldTile2(4, 5), GetBldResourceInfoManual({ 30, 30 })
	),

	// Unique Cards	
	BldInfo(CardEnum::StatisticsBureau, _LOCTEXT("Statistics Bureau", "Statistics Bureau"), FText(), LOCTEXT("Statistics Bureau Desc", "Show Town Statistics."),
		WorldTile2(6, 9), GetBldResourceInfoManual({ 0 })
	),
	BldInfo(CardEnum::JobManagementBureau, _LOCTEXT("Employment Bureau", "Employment Bureau"), FText(), LOCTEXT("Employment Bureau Desc", "Allow managing job priority (global)."),
		WorldTile2(6, 9), GetBldResourceInfoManual({ 0 })
	),


	// Aug 25 additions
	BldInfo(CardEnum::MinorCity, _LOCTEXT("Minor City", "Minor City"), LOCTEXT("Minor City (Plural)", "Minor Cities"), LOCTEXT("Minor City Desc", ""),
		WorldTile2(18, 18), GetBldResourceInfoManual({})
	),
	BldInfo(CardEnum::MinorCityPort, _LOCTEXT("Minor City Port", "Minor City Port"), LOCTEXT("Minor City Port (Plural)", "Minor City Ports"), LOCTEXT("Minor City Port Desc", ""),
		WorldTile2(10, 9), GetBldResourceInfoManual({})
	),
	BldInfo(CardEnum::TourismAgency, _LOCTEXT("Tourism Agency", "Tourism Agency"), LOCTEXT("Tourism Agency (Plural)", "Tourism Agencies"), LOCTEXT("Tourism Agency Desc", ""),
		WorldTile2(6, 6), GetBldResourceInfoManual({})
	),
	BldInfo(CardEnum::Hotel, _LOCTEXT("Hotel", "Hotel"), LOCTEXT("Hotel (Plural)", "Hotels"), LOCTEXT("Hotel Desc", "Allow visitors from other towns to stay (for <img id=\"Coin\"/>)."),
		WorldTile2(8, 12), GetBldResourceInfoManual({})
	),
	BldInfo(CardEnum::Zoo, _LOCTEXT("Zoo", "Zoo"), LOCTEXT("Zoo (Plural)", "Zoos"), LOCTEXT("Zoo Desc", "Slot Animal Cards to get bonuses."),
		WorldTile2(12, 12), GetBldResourceInfoManual({ 0, 0, 0, 100, 100, 100, 100 })
	),
	BldInfo(CardEnum::Museum, _LOCTEXT("Museum", "Museum"), LOCTEXT("Museum (Plural)", "Museums"), LOCTEXT("Museum Desc", "Slot Artifact Cards to get bonuses"),
		WorldTile2(10, 8), GetBldResourceInfoManual({})
	),
	BldInfo(CardEnum::Embassy, _LOCTEXT("Embassy", "Embassy"), LOCTEXT("Embassy (Plural)", "Embassy"), LOCTEXT("Embassy Desc", "+50<img id=\"Influence\"/> income for builder and host player."),
		WorldTile2(6, 6), GetBldResourceInfoManual({ 200 })
	),
	BldInfo(CardEnum::ForeignQuarter, _LOCTEXT("Foreign Quarter", "Foreign Quarter"), LOCTEXT("Foreign Quarter (Plural)", "Foreign Quarters"), LOCTEXT("Foreign Quarter Desc", "+100<img id=\"Influence\"/> income for builder and host player."),
		WorldTile2(12, 18), GetBldResourceInfoManual({ 500 })
	),
	BldInfo(CardEnum::ForeignPort, _LOCTEXT("Foreign Port", "Foreign Port"), LOCTEXT("Foreign Port (Plural)", "Foreign Ports"), LOCTEXT("Foreign Port Desc", "+100<img id=\"Coin\"/> if this town has our Foreign Quarters"),
		WorldTile2(10, 12), GetBldResourceInfoManual({ 300 })
	),
	BldInfo(CardEnum::SpyCenter, _LOCTEXT("Spy Center", "Spy Center"), LOCTEXT("Spy Center (Plural)", "Spy Center"), LOCTEXT("Spy Center Desc", ""),
		WorldTile2(12, 12), GetBldResourceInfoManual({ 0, 0, 0, 100, 100, 100 })
	),
	BldInfo(CardEnum::PolicyOffice, _LOCTEXT("Policy Office", "Policy Office"), LOCTEXT("Policy Office (Plural)", "Policy Office"), LOCTEXT("Policy Office Desc", ""),
		WorldTile2(8, 12), GetBldResourceInfoManual({ 0, 0, 0, 100, 100, 100 })
	),
	BldInfo(CardEnum::WorldTradeOffice, _LOCTEXT("World Trade Office", "World Trade Office"), LOCTEXT("World Trade Office (Plural)", "World Trade Office"), LOCTEXT("World Trade Office Desc", ""),
		WorldTile2(8, 12), GetBldResourceInfoManual({ 0, 0, 0, 100, 100, 100 })
	),
	BldInfo(CardEnum::CardCombiner, _LOCTEXT("Card Combiner", "Card Combiner"), LOCTEXT("Card Combiner (Plural)", "Card Combiner"), LOCTEXT("Card Combiner Desc", ""),
		WorldTile2(8, 12), GetBldResourceInfoManual({ 0, 0, 0, 100, 100, 100 })
	),

	BldInfo(CardEnum::MayanPyramid, _LOCTEXT("Stepped Ruin", "Stepped Ruin"), LOCTEXT("Stepped Ruin (Plural)", "Stepped Ruins"), LOCTEXT("Stepped Ruin Desc", ""),
		WorldTile2(18, 18), GetBldResourceInfoManual({ 100 })
	),
	BldInfo(CardEnum::EgyptianPyramid, _LOCTEXT("Pyramid Ruin", "Pyramid Ruin"), LOCTEXT("Pyramid Ruin (Plural)", "Pyramid Ruins"), LOCTEXT("Pyramid Ruin Desc", ""),
		WorldTile2(18, 18), GetBldResourceInfoManual({ 100 })
	),
	BldInfo(CardEnum::StoneHenge, _LOCTEXT("Stone Circle Ruin", "Stone Circle Ruin"), LOCTEXT("Stone Circle Ruin (Plural)", "Stone Circle Ruins"), LOCTEXT("Stone Circle Ruin Desc", ""),
		WorldTile2(18, 18), GetBldResourceInfoManual({ 100 })
	),
	BldInfo(CardEnum::EasterIsland, _LOCTEXT("Mysterious Statue Ruin", "Mysterious Statues Ruin"), LOCTEXT("Mysterious Statues Ruin (Plural)", "Mysterious Statues Ruins"), LOCTEXT("Mysterious Statue Ruin Desc", ""),
		WorldTile2(18, 18), GetBldResourceInfoManual({ 100 })
	),
	
	BldInfo(CardEnum::Oasis, _LOCTEXT("Oasis", "Oasis"), LOCTEXT("Oasis (Plural)", "Oases"), LOCTEXT("Oasis Desc", ""),
		WorldTile2(18, 18), GetBldResourceInfoManual({})
	),

	BldInfo(CardEnum::IrrigationPump, _LOCTEXT("Irrigation Pump", "Irrigation Pump"), LOCTEXT("Irrigation Pump (Plural)", "Irrigation Pumps"), LOCTEXT("Irrigation Pump Desc", "Pump water to increase soil Fertility."),
		WorldTile2(6, 4), GetBldResourceInfoManual({})
	),
	BldInfo(CardEnum::IrrigationDitch, _LOCTEXT("Irrigation Ditch", "Irrigation Ditch"), LOCTEXT("Irrigation Ditch (Plural)", "Irrigation Ditches"), LOCTEXT("Irrigation Ditch Desc", "Distribute water from Irrigation Pump to increase soil fertility (radius 5)."),
		WorldTile2(1, 1), GetBldResourceInfoManual({})
	),
	BldInfo(CardEnum::CarpetWeaver, _LOCTEXT("Carpet Weaver", "Carpet Weaver"), LOCTEXT("Carpet Weaver (Plural)", "Carpet Weavers"), LOCTEXT("Carpet Weaver Desc", ""),
		WorldTile2(8, 6), GetBldResourceInfo(2, { ResourceEnum::Wool, ResourceEnum::Dye, ResourceEnum::Carpet }, { 0, 0, 0, 5, 10, 5, 0 }, 0, 130)
	),
	BldInfo(CardEnum::BathHouse, _LOCTEXT("Bath House", "Bath House"), LOCTEXT("Bath House (Plural)", "Bath Houses"), LOCTEXT("Bath House Desc", ""),
		WorldTile2(12, 12), GetBldResourceInfoManual({})
	),
	BldInfo(CardEnum::Caravansary, _LOCTEXT("Caravansary", "Caravansary"), LOCTEXT("Caravansary (Plural)", "Caravansaries"), LOCTEXT("Caravansary Desc", "Send Caravan on a Land Trade Route. Give profit to both destinations."),
		WorldTile2(8, 8), GetBldResourceInfoManual({ 100, 50 }, 1)
	),
	BldInfo(CardEnum::PitaBakery, _LOCTEXT("Pita Bakery", "Pita Bakery"), LOCTEXT("Pita Bakery (Plural)", "Pita Bakeries"), LOCTEXT("Pita Bakery Desc", "Bakes Bread with Wheat Flour and heat."),
		WorldTile2(8, 6), GetBldResourceInfo(2, { ResourceEnum::Flour, ResourceEnum::Coal, ResourceEnum::PitaBread }, { 0, 1, 0, 2 })
	),
	BldInfo(CardEnum::GreatMosque, _LOCTEXT("Great Mosque", "Great Mosque"), LOCTEXT("Great Mosque (Plural)", "Great Mosques"), LOCTEXT("Great Mosque Desc", ""),
		WorldTile2(36, 24), GetBldResourceInfo(4, {}, { 0, 0, 0, 0, 2, 1, 5 }, 20000, 100, -999)
	),
	
	
	// Can no longer pickup cards
	//BldInfo("Necromancer tower",	WorldTile2(4, 5),		ResourceEnum::None, ResourceEnum::None, ResourceEnum::None,		0,	{30, 30, 0},	"All citizens become zombie minions. Happiness becomes irrelevant. Immigration ceased."),
	// Imperialist
};


static const BldInfo CardInfos[]
{
	BldInfo(CardEnum::Investment,		_LOCTEXT("Investment", "Investment"), 300, LOCTEXT("Investment Desc", "+1<img id=\"Coin\"/> income for every 20<img id=\"Coin\"/> you own\n<Gray>(max +100)</>")),
	BldInfo(CardEnum::InstantBuild,		_LOCTEXT("Instant Build", "Instant Build"), 200, LOCTEXT("Instant Build Desc", "Use on a construction site to pay x3 the resource cost and instantly build it.")),
	BldInfo(CardEnum::ShrineWisdomPiece,	_INVTEXT("OLD"), 200, FText()),
	BldInfo(CardEnum::ShrineLovePiece,	_INVTEXT("Shrine Piece: Love"), 200, INVTEXT("Collect 3 shrine pieces to build a shrine.")),
	BldInfo(CardEnum::ShrineGreedPiece,	_INVTEXT("Shrine Piece: Greed"), 200, INVTEXT("Collect 3 shrine pieces to build a shrine.")),

	/*
	 * Passives
	 */
	BldInfo(CardEnum::BeerTax,			_LOCTEXT("Beer Tax", "Beer Tax"), 200, LOCTEXT("Beer Tax Desc", "Houses with Beer get +5<img id=\"Coin\"/>")),
	BldInfo(CardEnum::HomeBrew,			_LOCTEXT("Home Brew", "Home Brew"), 200, LOCTEXT("Home Brew Desc", "Houses with Pottery get +4<img id=\"Science\"/>")),

	BldInfo(CardEnum::MasterBrewer,			_LOCTEXT("Master Brewer", "Master Brewer"), 200, LOCTEXT("Master Brewer Desc", "Breweries/Distilleries gain +30% efficiency")),
	BldInfo(CardEnum::MasterPotter,			_LOCTEXT("Master Potter", "Master Potter"), 200, LOCTEXT("Master Potter Desc", "Potters gain +20% efficiency")),

	BldInfo(CardEnum::CooperativeFishing,	_LOCTEXT("Cooperative Fishing", "Cooperative Fishing"), 200, LOCTEXT("Cooperative Fishing Desc","Every 1% Happiness above 60%, gives +1% productivity to Fishing Lodge")),
	BldInfo(CardEnum::CompaniesAct,			_LOCTEXT("Companies Act", "Companies Act"), 200, LOCTEXT("Companies Act Desc", "-10% trade fees for Trading Companies.")),

	BldInfo(CardEnum::ProductivityBook,	_LOCTEXT("Productivity Book", "Productivity Book"), 100, LOCTEXT("Productivity Book Desc", "+20% productivity")),
	BldInfo(CardEnum::ProductivityBook2,_LOCTEXT("Productivity+ Book", "Productivity+ Book"),	100, LOCTEXT("Productivity+ Book Desc", "+30% productivity")),
	BldInfo(CardEnum::SustainabilityBook,_LOCTEXT("Sustainability Book", "Sustainability Book"), 100, LOCTEXT("Sustainability Book Desc","Consume 30% less input")),
	BldInfo(CardEnum::SustainabilityBook2,_LOCTEXT("Sustainability+ Book", "Sustainability+ Book"), 100, LOCTEXT("Sustainability+ Book Desc","Consume 40% less input")),
	BldInfo(CardEnum::FrugalityBook,		_LOCTEXT("Frugality Book", "Frugality Book"), 100, LOCTEXT("Frugality Book Desc",  "Decrease upkeep by 70%.")),
	BldInfo(CardEnum::Motivation,	_LOCTEXT("Motivation", "Motivation"), 100, LOCTEXT("Motivation Desc", "Every 1% Happiness above 60%, gives +1% productivity")),
	BldInfo(CardEnum::Motivation2,	_LOCTEXT("Motivation+", "Motivation+"), 100, LOCTEXT("Motivation+ Desc", "Every 1% Happiness above 75%, gives +2% productivity")),
	BldInfo(CardEnum::Passion,		_LOCTEXT("Passion", "Passion"), 100, LOCTEXT("Passion Desc", "+20% Job Happiness, +15% Productivity")),
	BldInfo(CardEnum::Passion2,		_LOCTEXT("Passion+", "Passion+"), 100, LOCTEXT("Passion+ Desc", "+50% Job Happiness, +30% Productivity")),

	
	BldInfo(CardEnum::DesertPilgrim,		_LOCTEXT("Desert Pilgrim", "Desert Pilgrim"), 200, LOCTEXT("Desert Pilgrim Desc", "Houses built on Desert get +10<img id=\"Coin\"/>.")),

	BldInfo(CardEnum::WheatSeed,			_LOCTEXT("Wheat Seeds", "Wheat Seeds"), 300, LOCTEXT("Wheat Seeds Desc", "Unlock Wheat farming. Wheat can be eaten or brewed into Beer.")),
	BldInfo(CardEnum::CabbageSeed,		_LOCTEXT("Cabbage Seeds", "Cabbage Seeds"), 350, LOCTEXT("Cabbage Seeds Desc", "Unlock Cabbage farming. Cabbage has high fertility sensitivity.")),
	BldInfo(CardEnum::HerbSeed,			_LOCTEXT("Medicinal Herb Seeds", "Medicinal Herb Seeds"), 500, LOCTEXT("Medicinal Herb Seeds Desc", "Unlock Medicinal Herb farming. Medicinal Herb can be used to heal sickness.")),
	BldInfo(CardEnum::PotatoSeed,			_LOCTEXT("Potato Seeds", "Potato Seeds"), 300, LOCTEXT("Potato Seeds Desc", "Unlock Potato farming. Potato can be eaten or brewed into Vodka.")),
	BldInfo(CardEnum::BlueberrySeed,		_LOCTEXT("Blueberry Seeds", "Blueberry Seeds"), 300, LOCTEXT("Blueberry Seeds Desc", "Unlock Blueberry farming. Blueberries can be eaten.")),
	BldInfo(CardEnum::MelonSeed,			_LOCTEXT("Melon Seeds", "Melon Seeds"), 300, LOCTEXT("Melon Seeds Desc", "Unlock Melon farming. Melon can be eaten.")),
	BldInfo(CardEnum::PumpkinSeed,			_LOCTEXT("Pumpkin Seeds", "Pumpkin Seeds"), 300, LOCTEXT("Pumpkin Seeds Desc", "Unlock Pumpkin farming. Pumpkin can be eaten.")),
	
	BldInfo(CardEnum::CannabisSeeds,		_LOCTEXT("Cannabis Seeds", "Cannabis Seeds"), 0, LOCTEXT("Cannabis Seeds Desc", "Unlock Cannabis farming. Require region suitable for Cannabis.")),
	BldInfo(CardEnum::GrapeSeeds,			_LOCTEXT("Grape Seeds", "Grape Seeds"), 0, LOCTEXT("Grape Seeds Desc", "Unlock Grape farming. Requires region suitable for Grape.")),
	BldInfo(CardEnum::CocoaSeeds,			_LOCTEXT("Cocoa Seeds", "Cocoa Seeds"), 0, LOCTEXT("Cocoa Seeds Desc", "Unlock Cocoa farming. Requires region suitable for Cocoa.")),
	BldInfo(CardEnum::CottonSeeds,			_LOCTEXT("Cotton Seeds", "Cotton Seeds"), 0, LOCTEXT("Cotton Seeds Desc", "Unlock Cotton farming. Requires region suitable for Cotton.")),
	BldInfo(CardEnum::DyeSeeds,				_LOCTEXT("Dye Seeds", "Dye Seeds"), 0, LOCTEXT("Dye Seeds Desc", "Unlock Dye farming. Requires region suitable for Dye.")),
	BldInfo(CardEnum::CoffeeSeeds,			_LOCTEXT("Coffee Seeds", "Coffee Seeds"), 0, LOCTEXT("Coffee Seeds Desc", "Unlock Coffee farming. Requires region suitable for Coffee.")),
	BldInfo(CardEnum::TulipSeeds,			_LOCTEXT("Tulip Seeds", "Tulip Seeds"), 0, LOCTEXT("Tulip Seeds Desc", "Unlock Tulip farming. Requires region suitable for Tulip.")),

	BldInfo(CardEnum::ChimneyRestrictor,	_LOCTEXT("Chimney Restrictor", "Chimney Restrictor"), 250, LOCTEXT("Chimney Restrictor Desc", "Wood/Coal gives 15% more heat")),
	BldInfo(CardEnum::SellFood,				_LOCTEXT("Sell Food", "Sell Food"), 90, LOCTEXT("Sell Food Desc", "Sell half of city's food for 5<img id=\"Coin\"/> each.")),
	BldInfo(CardEnum::BuyWood,				_LOCTEXT("Buy Wood", "Buy Wood"), 50, LOCTEXT("Buy Wood Desc", "Buy Wood with half of your treasury for 6<img id=\"Coin\"/> each. (max: 1000)")),
	BldInfo(CardEnum::ChildMarriage,		_LOCTEXT("Child Marriage", "Child Marriage"), 200, LOCTEXT("Child Marriage Desc", "Decrease the minimum age for having children.")),
	BldInfo(CardEnum::ProlongLife,			_LOCTEXT("Prolong Life", "Prolong Life"), 200, LOCTEXT("Prolong Life Desc", "People live longer.")),
	BldInfo(CardEnum::BirthControl,			_LOCTEXT("Birth Control", "Birth Control"), 200, LOCTEXT("Birth Control Desc", "Prevents childbirth when the housing capacity is full.")),
	BldInfo(CardEnum::CoalTreatment,		_LOCTEXT("Coal Treatment", "Coal Treatment"), 250, LOCTEXT("Coal Treatment Desc", "Coal gives 20% more heat")),


	BldInfo(CardEnum::CoalPipeline,			_LOCTEXT("Coal Pipeline", "Coal Pipeline"), 150, LOCTEXT("Coal Pipeline Desc", "+50% productivity for smelters if the town has more than 1,000 Coal")),
	BldInfo(CardEnum::MiningEquipment,		_LOCTEXT("Mining Equipment", "Mining Equipment"), 150, LOCTEXT("Mining Equipment Desc", "+30% productivity for mines if you have a Blacksmith")),
	BldInfo(CardEnum::Conglomerate,			_LOCTEXT("Conglomerate", "Conglomerate"), 150, LOCTEXT("Conglomerate Desc", "Upkeep for Trading Companies are reduced to 1")),
	BldInfo(CardEnum::SmeltCombo,			_LOCTEXT("Iron Smelter Combo", "Iron Smelter Combo"), 150, LOCTEXT("Iron Smelter Combo Desc", "+30% productivity to all Iron Smelter with adjacent Iron Smelter")),
	
	BldInfo(CardEnum::Immigration,			_LOCTEXT("Immigrants", "Immigrants"), 500, LOCTEXT("Immigrants Desc", "5 immigrants join upon use.")),
	BldInfo(CardEnum::DuplicateBuilding,	_INVTEXT("Duplicate Building"), 200, INVTEXT("Duplicate the chosen building into a card.")),


	BldInfo(CardEnum::Pig,				_INVTEXT("Pig"), 100, INVTEXT("Spawn 3 Pigs on a Ranch.")),
	BldInfo(CardEnum::Sheep,			_INVTEXT("Sheep"), 200, INVTEXT("Spawn 3 Sheep on a Ranch.")),
	BldInfo(CardEnum::Cow,				_INVTEXT("Cow"), 300, INVTEXT("Spawn 3 Cows on a Ranch.")),

	// Zoo animals
	BldInfo(CardEnum::Boar,				_LOCTEXT("Boar", "Boar"), 1000, LOCTEXT("Boar Desc", "+5% City Attractiveness when placed in Zoo Collection")),
	BldInfo(CardEnum::RedDeer,			_LOCTEXT("Red Deer", "Red Deer"), 1000, LOCTEXT("Red Deer Desc", "+5% City Attractiveness when placed in Zoo Collection")),
	BldInfo(CardEnum::YellowDeer,		_LOCTEXT("Yellow Deer", "Yellow Deer"), 1000, LOCTEXT("Yellow Deer Desc", "+5% City Attractiveness when placed in Zoo Collection")),
	BldInfo(CardEnum::DarkDeer,			_LOCTEXT("Dark Deer", "Dark Deer"), 1000, LOCTEXT("Dark Deer Desc", "+5% City Attractiveness when placed in Zoo Collection")),

	BldInfo(CardEnum::BrownBear,		_LOCTEXT("Brown Bear", "Brown Bear"), 1000, LOCTEXT("Brown Bear Desc", "+5% City Attractiveness when placed in Zoo Collection")),
	BldInfo(CardEnum::BlackBear,		_LOCTEXT("Black Bear", "Black Bear"), 1000, LOCTEXT("Black Bear Desc", "+5% City Attractiveness when placed in Zoo Collection)")),
	BldInfo(CardEnum::Panda,			_LOCTEXT("Panda", "Panda"), 1000, LOCTEXT("Panda Desc", "+5% City Attractiveness when placed in Zoo Collection")),

	BldInfo(CardEnum::Moose,			_LOCTEXT("Moose", "Moose"), 1000, LOCTEXT("Moose Desc", "+5% City Attractiveness when placed in Zoo Collection")),
	BldInfo(CardEnum::Hippo,			_LOCTEXT("Hippo", "Hippo"), 1000, LOCTEXT("Hippo Desc", "+5% City Attractiveness when placed in Zoo Collection")),
	BldInfo(CardEnum::Penguin,			_LOCTEXT("Penguin", "Penguin"), 1000, LOCTEXT("Penguin Desc", "+5% City Attractiveness when placed in Zoo Collection")),
	BldInfo(CardEnum::Bobcat,			_LOCTEXT("Bobcat", "Bobcat"), 1000, LOCTEXT("Bobcat Desc", "+5% City Attractiveness when placed in Zoo Collection")),

	//! Artifacts
	BldInfo(CardEnum::Codex,			_LOCTEXT("Codex", "Codex"), 1000,						LOCTEXT("Codex Desc","+5% City Attractiveness when placed in Museum Collection")),
	BldInfo(CardEnum::SacrificialAltar,	_LOCTEXT("Sacrificial Altar", "Sacrificial Altar"), 1000, LOCTEXT("Sacrificial Altar Desc","+5% City Attractiveness when placed in Museum Collection")),
	BldInfo(CardEnum::StoneStele,		_LOCTEXT("Stone Stele", "Stone Stele"), 1000,			LOCTEXT("Stone Stele Desc","+5% City Attractiveness when placed in Museum Collection")),
	BldInfo(CardEnum::BallCourtGoals,	_LOCTEXT("Ball Court Goals", "Ball Court Goals"), 1000, LOCTEXT("Ball Court Goals Desc","+5% City Attractiveness when placed in Museum Collection")),
	BldInfo(CardEnum::FeatherCrown,		_LOCTEXT("Feather Crown", "Feather Crown"), 1000,		LOCTEXT("Feather Crown Desc","+5% City Attractiveness when placed in Museum Collection")),

	BldInfo(CardEnum::CanopicJars,		_LOCTEXT("Canopic Jars", "Canopic Jars"), 1000,			LOCTEXT("Canopic Jars Desc","+5% City Attractiveness when placed in Museum Collection")),
	BldInfo(CardEnum::DepartureScrolls,	_LOCTEXT("Departure Scrolls", "Departure Scrolls"), 1000, LOCTEXT("Departure Scrolls Desc","+5% City Attractiveness when placed in Museum Collection")),
	BldInfo(CardEnum::DeathMask,		_LOCTEXT("Death Mask", "Death Mask"), 1000,				LOCTEXT("Death Mask Desc","+5% City Attractiveness when placed in Museum Collection")),
	BldInfo(CardEnum::SolarBarque,		_LOCTEXT("Solar Barque", "Solar Barque"), 1000,			LOCTEXT("Solar Barque Desc","+5% City Attractiveness when placed in Museum Collection")),
	BldInfo(CardEnum::GoldCapstone,		_LOCTEXT("Gold Capstone", "Gold Capstone"), 1000,		LOCTEXT("Gold Capstone Desc","+5% City Attractiveness when placed in Museum Collection")),

	BldInfo(CardEnum::FeastRemains,		_LOCTEXT("Feast Remains", "Feast Remains"), 1000,		LOCTEXT("Feast Remains Desc","+5% City Attractiveness when placed in Museum Collection")),
	BldInfo(CardEnum::ForeignTrinkets,	_LOCTEXT("Foreign Trinkets", "Foreign Trinkets"), 1000, LOCTEXT("Foreign Trinkets Desc","+5% City Attractiveness when placed in Museum Collection")),
	BldInfo(CardEnum::ChalkPlaque,		_LOCTEXT("Chalk Plaque", "Chalk Plaque"), 1000,			LOCTEXT("Chalk Plaque Desc","+5% City Attractiveness when placed in Museum Collection")),
	BldInfo(CardEnum::OfferingCup,		_LOCTEXT("Offering Cup", "Offering Cup"), 1000,		LOCTEXT("Offering Cup Desc","+5% City Attractiveness when placed in Museum Collection")),
	BldInfo(CardEnum::OrnateTrinkets,	_LOCTEXT("Ornate Trinkets", "Ornate Trinkets"), 1000,		LOCTEXT("Ornate Trinkets Desc","+5% City Attractiveness when placed in Museum Collection")),

	BldInfo(CardEnum::TatooingNeedles,	_LOCTEXT("Tatooing Needles", "Tatooing Needles"), 1000, LOCTEXT("Tatooing Needles Desc", "+5% City Attractiveness when placed in Museum Collection")),
	BldInfo(CardEnum::Petroglyphs,		_LOCTEXT("Petroglyphs", "Petroglyphs"), 1000,			LOCTEXT("Petroglyphs Desc", "+5% City Attractiveness when placed in Museum Collection")),
	BldInfo(CardEnum::StoneFishhooks,	_LOCTEXT("Stone Fishhooks", "Stone Fishhooks"), 1000, LOCTEXT("Stone Fishhooks Desc", "+5% City Attractiveness when placed in Museum Collection")),
	BldInfo(CardEnum::CoralEyes,		_LOCTEXT("Coral Eyes", "Coral Eyes"), 1000,			LOCTEXT("Coral Eyes Desc", "+5% City Attractiveness when placed in Museum Collection")),
	BldInfo(CardEnum::AncientStaff,		_LOCTEXT("Ancient Staff", "Ancient Staff"), 1000,		LOCTEXT("Ancient Staff Desc", "+5% City Attractiveness when placed in Museum Collection")),

	
	//
	BldInfo(CardEnum::FireStarter,		_LOCTEXT("Fire Starter", "Fire Starter"), 200,	LOCTEXT("Fire Starter Desc", "Start a fire in an area (3 tiles radius).")),
	BldInfo(CardEnum::Steal,			_LOCTEXT("StealOld", "StealOld"), 200,					LOCTEXT("StealOld Desc", "Steal 30% of target player's treasury<img id=\"Coin\"/>. Use on Townhall. <Gray>(max 10000)</>")),
	BldInfo(CardEnum::Snatch,			_LOCTEXT("Steal", "Steal"), 100,				LOCTEXT("Steal Desc", "Steal <img id=\"Coin\"/> equal to target player's population X 10. Use on Townhall.")),
	BldInfo(CardEnum::SharingIsCaring,	_LOCTEXT("Sharing is Caring", "Sharing is Caring"), 120, LOCTEXT("Sharing is Caring Desc", "Give 100 Wheat to the target player. Use on Townhall.")),
	BldInfo(CardEnum::Kidnap,			_LOCTEXT("Kidnap", "Kidnap"), 350,				LOCTEXT("Kidnap Desc", "Steal up to 3 citizens from target player. Apply on Townhall.")),
	BldInfo(CardEnum::KidnapGuard,		_LOCTEXT("Kidnap Guard", "Kidnap Guard"), 20,	LOCTEXT("Kidnap Guard Desc", "Guard your city against Kidnap for two years. Require <img id=\"Coin\"/>xPopulation to activate.")),
	BldInfo(CardEnum::Raid,				_LOCTEXT("Raid", "Raid"), 350,				LOCTEXT("Raid Desc", "")),
	BldInfo(CardEnum::Terrorism,		_LOCTEXT("Terrorism", "Terrorism"), 350,	LOCTEXT("Terrorism Desc", "Kill 5 citizens at target Town, opponent loses 1000 influence")),
	BldInfo(CardEnum::TreasuryGuard,	_LOCTEXT("Treasury Guard", "Treasury Guard"), 20, LOCTEXT("Treasury Guard Desc", "Guard your city against Steal and Snatch for two years. Require <img id=\"Coin\"/>xPopulation to activate.")),

	
	BldInfo(CardEnum::Cannibalism,		_LOCTEXT("Cannibalism", "Cannibalism"), 0, LOCTEXT("Cannibalism Desc", "On death, people drop Meat. Hunters can hunt Wild Man. -50% City Attractiveness Happiness.")),

	BldInfo(CardEnum::WildCard,		_LOCTEXT("Wild Card", "Wild Card"), 15, LOCTEXT("Wild Card Desc", "Build an unlocked building of your choice.")),

	// June Additions
	BldInfo(CardEnum::WildCardFood,		_LOCTEXT("Agriculture Wild Card", "Agriculture Wild Card"), 10, LOCTEXT("Agriculture Wild Card Desc", "Build an unlocked agriculture building of your choice.")),
	BldInfo(CardEnum::WildCardIndustry,	_LOCTEXT("Industry Wild Card", "Industry Wild Card"), 10, LOCTEXT("Industry Wild Card Desc", "Build an unlocked industry of your choice.")),
	BldInfo(CardEnum::WildCardMine,		_LOCTEXT("Mine Wild Card", "Mine Wild Card"), 10, LOCTEXT("Mine Wild Card Desc", "Build an unlocked mine of your choice.")),
	BldInfo(CardEnum::WildCardService,		_LOCTEXT("Service Wild Card", "Service Wild Card"), 10, LOCTEXT("Service Wild Card Desc", "Build an unlocked service building of your choice.")),
	BldInfo(CardEnum::CardRemoval,			_LOCTEXT("Card Removal", "Card Removal"), 30, LOCTEXT("Card Removal Desc", "Remove a card from the draw deck.")),

	// Rare cards
	BldInfo(CardEnum::HappyBreadDay,		_LOCTEXT("Happy Bread Day", "Happy Bread Day"), 150, LOCTEXT("Happy Bread Day Desc", "+20% Food Happiness to Citizens, if the Town has more than 1,000 Bread")),
	BldInfo(CardEnum::BlingBling,		_LOCTEXT("Bling Bling", "Bling Bling"), 150, LOCTEXT("Bling Bling Desc", "+30% Luxury Happiness in Houses with Jewelry. Houses consume x2 more Jewelry.")),
	BldInfo(CardEnum::FarmWaterManagement,			_LOCTEXT("Farm Water Management", "Farm Water Management"), 150, LOCTEXT("Farm Water Management Desc", "+8% Farm Productivity.")),

	BldInfo(CardEnum::AllYouCanEat,		_LOCTEXT("All You Can Eat", "All You Can Eat"), 200, LOCTEXT("All You Can Eat Desc", "+30% Food Happiness. +50% food consumption")),
	BldInfo(CardEnum::SlaveLabor,		_LOCTEXT("Slave Labor", "Slave Labor"), 200,		LOCTEXT("Slave Labor Desc", "Work Efficiency Penalty from low Happiness will not exceed 30%")),
	BldInfo(CardEnum::Lockdown,			_LOCTEXT("Lockdown", "Lockdown"), 200,				LOCTEXT("Lockdown Desc", "Citizens cannot immigrate out of town without permission.")),
	BldInfo(CardEnum::SocialWelfare,		_LOCTEXT("Social Welfare", "Social Welfare"), 200, LOCTEXT("Social Welfare Desc", "+30% Job Happiness, -5<img id=\"Coin\"/> for each citizen")),

	BldInfo(CardEnum::Consumerism,			_LOCTEXT("Consumerism", "Consumerism"), 150, LOCTEXT("Consumerism Desc", ".....")),
	BldInfo(CardEnum::BookWorm,			_LOCTEXT("Book Worm", "Book Worm"), 150, LOCTEXT("BookWorm Desc", "+50%<img id=\"Science\"/> in Houses with Books. Houses consume x3 more Books.")),

	
	BldInfo(CardEnum::Treasure, _LOCTEXT("Treasure", "Treasure"), 100, LOCTEXT("Treasure Desc", "Instantly gain 500<img id=\"Coin\"/>")),
	BldInfo(CardEnum::IndustrialRevolution, _LOCTEXT("Industrial Revolution", "Industrial Revolution"), 200, LOCTEXT("Industrial Revolution Desc", "-50% cost of industrial building cards")),
	BldInfo(CardEnum::EmergencyRations, _LOCTEXT("Emergency Rations", "Emergency Rations"), 250, LOCTEXT("Emergency Rations Desc", "Instantly gain 50 Wheat.")),

	BldInfo(CardEnum::CrateWood, _LOCTEXT("Wood Crates", "Wood Crates"), 100, LOCTEXT("Wood Crates Desc", "Instantly gain 100 Wood")),
	BldInfo(CardEnum::CrateCoal, _LOCTEXT("Coal Crates", "Coal Crates"), 100, LOCTEXT("Coal Crates Desc", "Instantly gain 150 Coal")),
	BldInfo(CardEnum::CrateIronOre, _LOCTEXT("Iron Ore Crates", "Iron Ore Crates"), 100, LOCTEXT("Iron Ore Crates Desc", "Instantly gain 200 Iron Ore")),
	BldInfo(CardEnum::CratePottery, _LOCTEXT("Pottery Crates", "Pottery Crates"), 100, LOCTEXT("Pottery Crates Desc", "Instantly gain 80 Pottery")),
	BldInfo(CardEnum::CrateJewelry, _LOCTEXT("Jewelry Crates", "Jewelry Crates"), 100, LOCTEXT("Jewelry Crates Desc", "Instantly gain 10 Jewelry")),

	BldInfo(CardEnum::SpeedBoost, _LOCTEXT("Speed Boost", "Speed Boost"), 0, LOCTEXT("Speed Boost Desc", "Boost target building's work speed by +50% for 50s.")),


		BldInfo(CardEnum::BorealWinterFishing, _LOCTEXT("Winter Fishing", "Winter Fishing"), 0, LOCTEXT("Winter Fishing Desc", "+50% Productivity to Fishing Lodges in Boreal Forest or Tundra.")),
		BldInfo(CardEnum::BorealWinterResist, _LOCTEXT("Winter Resistance", "Winter Resistance"), 0, LOCTEXT("Winter Resistance Desc", "Wood/Coal gives 30% more heat.")),
		BldInfo(CardEnum::BorealGoldOil, _LOCTEXT("Gold Rush", "Gold Rush"), 0, LOCTEXT("Gold Rush Desc", "+70% Productivity to Gold Mines and Oil Rigs.")),
		BldInfo(CardEnum::BorealPineForesting, _LOCTEXT("Pine Foresting", "Pine Foresting"), 0, LOCTEXT("Pine Foresting Desc", "+30% Wood yield when cutting Pine Trees.")),

		BldInfo(CardEnum::DesertGem, _LOCTEXT("Desert Gem", "Desert Gem"), 0, LOCTEXT("Desert Gem Desc", "+50% Productivity to Gem and Gold Mines.")),
		BldInfo(CardEnum::DesertTradeForALiving, _LOCTEXT("Trade for a Living", "Trade for a Living"), 0, LOCTEXT("Trade for a Living Desc", "Food/Wood/Coal has 0% Trading Fee.")),
		BldInfo(CardEnum::DesertOreTrade, _LOCTEXT("Ore Trade", "Ore Trade"), 0, LOCTEXT("Ore Trade Desc", "Ores/Coal/Gemstone has 0% Trading Fee.")),
		BldInfo(CardEnum::DesertIndustry, _LOCTEXT("Desert Industry", "Desert Industry"), 0, LOCTEXT("Desert Industry Desc", "+20% Productivity to all Industries. -50% Farm Productivity.")),

		BldInfo(CardEnum::SavannaRanch, _LOCTEXT("Grass Fed", "Grass Fed"), 0, LOCTEXT("Grass Fed Desc", "+35% Ranch Productivity.")),
		BldInfo(CardEnum::SavannaHunt, _LOCTEXT("Grassland Hunting", "Grassland Hunting"), 0, LOCTEXT("Grassland Hunting Desc", "+50% Hunting Lodge Productivity.")),
		BldInfo(CardEnum::SavannaGrasslandHerder, _LOCTEXT("Grassland Herder", "Grassland Herder"), 0, LOCTEXT("Grassland Herder Desc", "+50% Productivity for Ranch.")),
		BldInfo(CardEnum::SavannaGrasslandRoamer, _LOCTEXT("Grassland Roamer", "Grassland Roamer"), 0, LOCTEXT("Grassland Roamer Desc", "+50% <img id=\"Influence\"/> from Houses built on Grassland/Savanna.")),

		BldInfo(CardEnum::JungleGatherer, _LOCTEXT("Jungle Gatherer", "Jungle Gatherer"), 0, LOCTEXT("Jungle Gatherer Desc", "+30% Productivity for Fruit Gatherers in Jungle Biome.")),
		BldInfo(CardEnum::JungleMushroom, _LOCTEXT("Jungle Mushroom", "Jungle Mushroom"), 0, LOCTEXT("Jungle Mushroom Desc", "+30% Productivity for Mushroom Farms in Jungle Biome.")),
		BldInfo(CardEnum::JungleHerbFarm, _LOCTEXT("Jungle Herb", "Jungle Herb"), 0, LOCTEXT("Jungle Herb Desc", "+30% Productivity for Medicinal Herb Farms in Jungle Biome.")),
		BldInfo(CardEnum::JungleTree, _LOCTEXT("Jungle Forestry", "Jungle Forestry"), 0, LOCTEXT("Jungle Forestry Desc", "+20% Wood yield when cutting Jungles Trees.")),

		BldInfo(CardEnum::ForestFarm, _LOCTEXT("Improved Farming", "Improved Farming"), 0, LOCTEXT("Improved Farming Desc", "+10% Productivity of Farms in Forest Biome.")),
		BldInfo(CardEnum::ForestCharcoal, _LOCTEXT("Improved Charcoal Making", "Improved Charcoal Making"), 0, LOCTEXT("Improved Charcoal Making Desc", "+30% Productivity of Charcoal Burner.")),
		BldInfo(CardEnum::ForestTools, _LOCTEXT("Improved Toolmaking", "Improved Toolmaking"), 0, LOCTEXT("Improved Toolmaking Desc", "+20% Productivity of Stone Tools Workshop and Blacksmith.")),
		BldInfo(CardEnum::ForestBeer, _LOCTEXT("Improved Beer Brewing", "Improved Beer Brewing"), 0, LOCTEXT("Improved Beer Brewing Desc", "+30% Productivity for Beer Brewery")),

		BldInfo(CardEnum::Agriculturalist, _LOCTEXT("Agriculturalist", "Agriculturalist"), 0, LOCTEXT("Agriculturalist Desc", "+10% Farm Productivity within Granary's Radius.")),
		BldInfo(CardEnum::Geologist, _LOCTEXT("Geologist", "Geologist"), 0, LOCTEXT("Geologist Desc", "Mines depletes 50% slower.")),
		BldInfo(CardEnum::AlcoholAppreciation, _LOCTEXT("Alcohol Appreciation", "Alcohol Appreciation"), 0, LOCTEXT("Alcohol Appreciation Desc", "+15% Productivity when producing Beer, Vodka, and Wine.")),
		BldInfo(CardEnum::Craftmanship, _LOCTEXT("Craftmanship", "Craftmanship"), 0, LOCTEXT("Craftmanship Desc", "+20% Productivity for Blacksmith and Potter.")),

		BldInfo(CardEnum::Rationalism, _LOCTEXT("Rationalism", "Rationalism"), 0, LOCTEXT("Rationalism Desc", "+1% Science Boost for each 2% Happiness above 80%.")),
		BldInfo(CardEnum::Romanticism, _LOCTEXT("Romanticism", "Romanticism"), 0, LOCTEXT("Romanticism Desc", "+1% Town Attractiveness for each House lv 5+ <Gray>(max 30%)</>.")),
		BldInfo(CardEnum::Protectionism, _LOCTEXT("Protectionism", "Protectionism"), 0, LOCTEXT("Protectionism Desc", "+20% Luxury Resources Productivity. Doubles Luxury Resources Import Fee.")),
		BldInfo(CardEnum::FreeThoughts, _LOCTEXT("Free Thoughts", "Free Thoughts"), 0, LOCTEXT("Free Thoughts Desc", "+0.5% Science Boost for each Trading Post/Port/Company in the World")),

		BldInfo(CardEnum::Capitalism, _LOCTEXT("Capitalism", "Capitalism"), 0, LOCTEXT("Capitalism Desc", "+100%<img id=\"Coin\"/> from Luxury Consumption")),
		BldInfo(CardEnum::Communism, _LOCTEXT("Communism", "Communism"), 0, LOCTEXT("Communism Desc", "+25% Productivity to all Production Buildings. Halve the effectiveness of Motivation/Passion Cards.")),
		BldInfo(CardEnum::WondersScoreMultiplier, _LOCTEXT("Wonders Score", "Wonders Score"), 0, LOCTEXT("Wonders Score Desc", "+100% Scores from Wonders.")),
		BldInfo(CardEnum::PopulationScoreMultiplier, _LOCTEXT("Population Score", "Population Score"), 0, LOCTEXT("Population Score Desc", "+100% Scores from Population and Happiness.")),


		BldInfo(CardEnum::Militia, _LOCTEXT("Militia", "Militia"), 0, LOCTEXT("Militia Desc", "")),
		BldInfo(CardEnum::Conscript, _LOCTEXT("Conscript", "Conscript"), 0, LOCTEXT("Conscript Desc", "")),
	
		BldInfo(CardEnum::Warrior, _LOCTEXT("Warrior", "Warrior"),		0, LOCTEXT("Warrior Desc", "")),
		BldInfo(CardEnum::Swordsman, _LOCTEXT("Swordsman", "Swordsman"), 0, LOCTEXT("Swordsman Desc", "")),
		BldInfo(CardEnum::Musketeer, _LOCTEXT("Musketeer", "Musketeer"), 0, LOCTEXT("Musketeer Desc", "")),
		BldInfo(CardEnum::Infantry, _LOCTEXT("Infantry", "Infantry"), 0, LOCTEXT("Infantry Desc", "")), // (Trench Warfare = Defense Bonus for Infantry)

		BldInfo(CardEnum::Knight, _LOCTEXT("Knight", "Knight"), 0, LOCTEXT("Knight Desc", "")), // Attack Bonus
		BldInfo(CardEnum::Tank, _LOCTEXT("Tank", "Tank"),		0, LOCTEXT("Tank Desc", "")), // Attack Bonus

		BldInfo(CardEnum::Archer, _LOCTEXT("Archer", "Archer"),				0, LOCTEXT("Archer Desc", "")),
		BldInfo(CardEnum::MachineGun, _LOCTEXT("Machine Gun", "Machine Gun"), 0, LOCTEXT("Machine Gun Desc", "")), // Defense Bonus

		BldInfo(CardEnum::Catapult, _LOCTEXT("Catapult", "Catapult"), 0, LOCTEXT("Catapult Desc", "")),
		BldInfo(CardEnum::Cannon, _LOCTEXT("Cannon", "Cannon"),			0, LOCTEXT("Cannon Desc", "")),
		BldInfo(CardEnum::Artillery, _LOCTEXT("Artillery", "Artillery"), 0, LOCTEXT("Artillery Desc", "")), // Defense Bonus

		BldInfo(CardEnum::Galley, _LOCTEXT("Galley", "Galley"),			0, LOCTEXT("Galley Desc", "")),
		BldInfo(CardEnum::Frigate, _LOCTEXT("Frigate", "Frigate"),		0, LOCTEXT("Frigate Desc", "")),
		BldInfo(CardEnum::Battleship, _LOCTEXT("Battleship", "Battleship"), 0, LOCTEXT("Battleship Desc", "")),
};

#undef _LOCTEXT
#undef _INVTEXT

#undef LOCTEXT_NAMESPACE

static const int32 BuildingEnumCount = _countof(BuildingInfo);
static const int32 NonBuildingCardEnumCount = _countof(CardInfos);
static const int32 CardEnumCount_WithNone = static_cast<int32>(CardEnum::None) + 1;

static const std::vector<CardEnum> ActionCards
{
	CardEnum::Treasure,
	CardEnum::SellFood,
	CardEnum::BuyWood,
	CardEnum::Immigration,
	CardEnum::EmergencyRations,

	CardEnum::FireStarter,
	//CardEnum::Steal,
	CardEnum::Snatch,
	CardEnum::SharingIsCaring,
	CardEnum::Kidnap,

	CardEnum::KidnapGuard,
	CardEnum::TreasuryGuard,

	CardEnum::InstantBuild,
	
	CardEnum::WildCard,
	CardEnum::WildCardFood,
	CardEnum::WildCardIndustry,
	CardEnum::WildCardMine,
	CardEnum::WildCardService,
	CardEnum::CardRemoval,

	CardEnum::CrateWood,
	CardEnum::CrateCoal,
	CardEnum::CrateIronOre,
	CardEnum::CratePottery,
	CardEnum::CrateJewelry,
};

// Note: IsActionCard is below

static bool IsTownSlotCard(CardEnum cardEnum)
{
	switch (cardEnum)
	{
	case CardEnum::Investment:

	case CardEnum::BeerTax:
	case CardEnum::HomeBrew:

	case CardEnum::MasterBrewer:
	case CardEnum::MasterPotter:
		
	case CardEnum::CooperativeFishing:
	case CardEnum::DesertPilgrim:

	case CardEnum::ChimneyRestrictor:
	case CardEnum::ChildMarriage:
	case CardEnum::ProlongLife:
	case CardEnum::BirthControl:

	case CardEnum::CoalTreatment:

	case CardEnum::CoalPipeline:
	case CardEnum::MiningEquipment:
	case CardEnum::Conglomerate:
	case CardEnum::SmeltCombo:
		
	case CardEnum::Cannibalism:

	case CardEnum::AllYouCanEat:
	case CardEnum::SlaveLabor:
	case CardEnum::Lockdown:
	case CardEnum::SocialWelfare:
		
	case CardEnum::IndustrialRevolution:

	case CardEnum::CompaniesAct:

	case CardEnum::HappyBreadDay:
	case CardEnum::BlingBling:
	case CardEnum::FarmWaterManagement:

	case CardEnum::BookWorm:

		return true;
	default:
		return false;
	}
}

struct CardStatus
{
	CardEnum cardEnum = CardEnum::None;
	
	//! Status
	int32 cardBirthTicks = -1; // used for tracking down the right card stack (network delay means we cannot use index)
	int32 stackSize = 1;

	int32 cardStateValue1 = 0;
	int32 cardStateValue2 = 0;
	int32 cardStateValue3 = 0;

	//! Animation
	int32 lastPositionX100 = -1;
	int32 lastPositionY100 = -1;
	int32 animationStartTime100 = -1;

	//! Display
	int32 displayCardStateValue1 = -1;
	int32 displayCardStateValue2 = -1;


	FVector2D lastPosition() const {
		return FVector2D(lastPositionX100 / 100.0f, lastPositionY100 / 100.0f);
	}


	CardStatus() {
		
	}
	CardStatus(CardEnum cardEnum, int32 stackSize) :
		cardEnum(cardEnum), stackSize(stackSize)
	{
	}

	void ResetAnimation() {
		lastPositionX100 = -1;
		lastPositionY100 = -1;
		animationStartTime100 = -1;
	}

	static const CardStatus None;

	bool operator==(const CardStatus& a) const {
		return cardEnum == a.cardEnum && 
			stackSize == a.stackSize &&
			cardStateValue1 == a.cardStateValue1 &&
			cardStateValue2 == a.cardStateValue2 &&
			cardStateValue3 == a.cardStateValue3 &&
			animationStartTime100 == a.animationStartTime100;
	}

	bool isValid() {
		return cardEnum != CardEnum::None;
	}
	
	//! Serialize
	FArchive& operator>>(FArchive &Ar);

	void Serialize(class PunSerializedData& blob);
};

static const std::vector<CardEnum> ZooAnimalCards
{
	CardEnum::Boar,
	CardEnum::RedDeer,
	CardEnum::YellowDeer,
	CardEnum::DarkDeer,
	
	CardEnum::BrownBear,
	CardEnum::BlackBear,
	CardEnum::Panda,

	CardEnum::Moose,
	CardEnum::Hippo,
	CardEnum::Penguin,
	CardEnum::Bobcat, 
};

// CardEnum List Helper
static bool IsCardEnumInList(CardEnum cardEnum, const std::vector<CardEnum>& vec)
{
	int32 cardEnumInt = static_cast<int32>(cardEnum);
	return static_cast<int32>(vec[0]) <= cardEnumInt && cardEnumInt <= static_cast<int32>(vec[vec.size() - 1]);
}

static bool IsCardEnumBetween(CardEnum cardEnum, CardEnum cardEnumMin, CardEnum cardEnumMax) {
	int32 cardEnumInt = static_cast<int32>(cardEnum);
	return static_cast<int32>(cardEnumMin) <= cardEnumInt && cardEnumInt <= static_cast<int32>(cardEnumMax);
}


static bool IsZooAnimalCard(CardEnum cardEnum) {
	return IsCardEnumInList(cardEnum, ZooAnimalCards);
}

static bool IsArtifactCard(CardEnum cardEnum) {
	return IsCardEnumBetween(cardEnum, CardEnum::Codex, CardEnum::AncientStaff);
}

static const std::vector<CardEnum> BuildingSlotCards_NoUpgrade
{
	CardEnum::ProductivityBook,
	CardEnum::SustainabilityBook,
	CardEnum::FrugalityBook,
	CardEnum::Motivation,
	CardEnum::Passion,
};

static const std::vector<CardEnum> BuildingSlotCards
{
	CardEnum::ProductivityBook,
	CardEnum::ProductivityBook2,
	CardEnum::SustainabilityBook,
	CardEnum::SustainabilityBook2,
	CardEnum::FrugalityBook,
	CardEnum::Motivation,
	CardEnum::Motivation2,
	CardEnum::Passion,
	CardEnum::Passion2,
};

static bool IsBuildingSlotCard(CardEnum cardEnum)
{
	if (IsZooAnimalCard(cardEnum)) {
		return true;
	}
	if (IsArtifactCard(cardEnum)) {
		return true;
	}
	return IsCardEnumInList(cardEnum, BuildingSlotCards);
}

static const CardEnum RareCards[]
{
	// Building
	//BuildingEnum::HellPortal,
	//CardEnum::AllYouCanEat,
	
	CardEnum::ArchitectStudio,
	CardEnum::EngineeringOffice,
	CardEnum::DepartmentOfAgriculture,
	
	CardEnum::StockMarket,
	CardEnum::CensorshipInstitute,
	CardEnum::EnvironmentalistGuild,
	CardEnum::IndustrialistsGuild,
	CardEnum::Oracle,
	CardEnum::AdventurersGuild,

	//BuildingEnum::ConsultingFirm,
	//BuildingEnum::ImmigrationPropagandaOffice,
	CardEnum::MerchantGuild,
	//CardEnum::OreSupplier,
	CardEnum::BeerBreweryFamous,

	////BuildingEnum::Cattery,
	//BuildingEnum::InvestmentBank,

	//CardEnum::CoalPipeline,
	//CardEnum::MiningEquipment,
	//CardEnum::Conglomerate,
	//CardEnum::SmeltCombo,

	//CardEnum::HomeBrew,
	//CardEnum::BeerTax,

	// Non-Building
	CardEnum::Treasure,
	CardEnum::IndustrialRevolution,
};
static const int32_t RareCardsCount = _countof(RareCards);

static bool IsRareCard(CardEnum cardEnumIn) {
	for (CardEnum cardEnum : RareCards) {
		if (cardEnum == cardEnumIn) {
			return true;
		}
	}
	return false;
}


static bool IsPermanentTownBonus(CardEnum cardEnum)
{
	int32 cardEnumInt = static_cast<int>(cardEnum);
	return static_cast<int>(CardEnum::BorealWinterFishing) <= cardEnumInt && cardEnumInt <= static_cast<int>(CardEnum::ForestBeer);
}

static bool IsPermanentGlobalBonus(CardEnum cardEnum)
{
	int32 cardEnumInt = static_cast<int>(cardEnum);
	return static_cast<int>(CardEnum::Agriculturalist) <= cardEnumInt && cardEnumInt <= static_cast<int>(CardEnum::PopulationScoreMultiplier);
}

static bool IsPermanentBonus(CardEnum cardEnum) {
	return IsPermanentTownBonus(cardEnum) || IsPermanentGlobalBonus(cardEnum);
}


/*
 * 
 */
static const std::vector<CardEnum> WildCards
{
	CardEnum::WildCard,
	CardEnum::WildCardFood,
	CardEnum::WildCardIndustry,
	CardEnum::WildCardMine,
	CardEnum::WildCardService,
	CardEnum::CardRemoval,
};
static bool IsWildCard(CardEnum cardEnumIn)
{
	for (CardEnum cardEnum : WildCards) {
		if (cardEnum == cardEnumIn) return true;
	}
	return false;
}

/*
 * Crate
 */

static const std::vector<CardEnum> CrateCards
{
	CardEnum::CrateWood,
	CardEnum::CrateCoal,
	CardEnum::CrateIronOre,
	CardEnum::CratePottery,
	CardEnum::CrateJewelry,
};
static const std::vector<ResourcePair> CrateResources
{
	{ ResourceEnum::Wood, 100 },
	{ ResourceEnum::Coal, 150 },
	{ ResourceEnum::IronOre, 200 },
	{ ResourceEnum::Pottery, 80 },
	{ ResourceEnum::Jewelry, 10 }, // 50 price
};
static bool IsCrateCard(CardEnum cardEnumIn)
{
	for (CardEnum cardEnum : CrateCards) {
		if (cardEnum == cardEnumIn) return true;
	}
	return false;
}
static ResourcePair GetCrateResource(CardEnum cardEnumIn)
{
	for (size_t i = 0; i < CrateCards.size(); i++) {
		if (CrateCards[i] == cardEnumIn) return CrateResources[i];
	}
	UE_DEBUG_BREAK();
	return ResourcePair::Invalid();
}


inline bool IsBuildingCard(CardEnum buildingEnum) {
	return static_cast<int32>(buildingEnum) < BuildingEnumCount;
}

inline const BldInfo& GetBuildingInfoInt(int32 buildingEnumInt) {
	if (buildingEnumInt >= BuildingEnumCount) {
		return CardInfos[buildingEnumInt - BuildingEnumCount];
	}
	return BuildingInfo[buildingEnumInt];
}
inline const BldInfo& GetBuildingInfo(CardEnum cardEnum) {
	PUN_CHECK(cardEnum != CardEnum::None);
	return GetBuildingInfoInt(static_cast<int32>(cardEnum));
}

inline CardEnum FindCardEnumByName(FString nameIn)
{
	nameIn = nameIn.ToLower();
	
	auto tryAddCard = [&](CardEnum cardEnum)
	{
		FString name = GetBuildingInfo(cardEnum).nameF();
		name = name.ToLower();

		if (name == nameIn) {
			return cardEnum;
		}
		return CardEnum::None;
	};

	for (int32 i = 0; i < BuildingEnumCount; i++) {
		CardEnum cardEnum = tryAddCard(static_cast<CardEnum>(i));
		if (cardEnum != CardEnum::None) {
			return cardEnum;
		}
	}
	for (int32 i = 0; i < NonBuildingCardEnumCount; i++) {
		CardEnum cardEnum = tryAddCard(CardInfos[i].cardEnum);
		if (cardEnum != CardEnum::None) {
			return cardEnum;
		}
	}
	
	return CardEnum::None;
}




static std::vector<CardEnum> GetSortedBuildingEnum()
{
	std::vector<std::wstring> cardNames;
	for (int32 i = 0; i < BuildingEnumCount; i++) {
		FText nameText = GetBuildingInfo(static_cast<CardEnum>(i)).name;
		std::wstring wString(*(nameText.ToString()));
		cardNames.push_back(wString);
	}
	std::sort(cardNames.begin(), cardNames.end());

	std::vector<CardEnum> results;
	for (const std::wstring& name : cardNames) {
		CardEnum cardEnum = FindCardEnumByName(FString(name.c_str()));
		PUN_CHECK(IsBuildingCard(cardEnum));
		results.push_back(cardEnum);
	}

	return results;
}
static std::vector<CardEnum> SortedNameBuildingEnum = GetSortedBuildingEnum();
static void ResetSortedNameBuildingEnum() {
	SortedNameBuildingEnum = GetSortedBuildingEnum();
}


inline bool IsProducer(CardEnum buildingEnum) {
	return BuildingInfo[static_cast<int32>(buildingEnum)].produce != ResourceEnum::None;
}

// Producers without Gatherer/Hunter
static bool IsProducerProcessor(CardEnum buildingEnum) {
	if (!IsProducer(buildingEnum)) {
		return false;
	}
	//switch (buildingEnum) {
	//	case BuildingEnum::BerryGatherer: // Not needed anymore?
	//	case BuildingEnum::HuntingLodge:
	//		return false;
	//	default: return true;
	//}
	return true;
}

static bool IsMountainMine(CardEnum buildingEnum)
{
	switch (buildingEnum) {
	case CardEnum::Quarry:
	case CardEnum::CoalMine:
	case CardEnum::IronMine:
	case CardEnum::GoldMine:
	case CardEnum::GemstoneMine:
		return true;
	default: return false;
	}
}

// For IsIndustry...
static bool IsDirtyProducer(CardEnum buildingEnum)
{
	if (buildingEnum == CardEnum::Mint) {
		return true;
	}
	if (IsProducer(buildingEnum)) {
		// Excludes
		switch (buildingEnum) {
		//case CardEnum::FruitGatherer: // Note: FruitGatherer and HuntingLodge
		//case CardEnum::HuntingLodge:
		case CardEnum::Fisher:
		case CardEnum::MushroomFarm:
		case CardEnum::Windmill:
		case CardEnum::Bakery:
		case CardEnum::PitaBakery:
		case CardEnum::Beekeeper:

			return false;
		default: return true;
		}
	}
	return false;
}

// Used for
// - Appeal
// - WildCard
// - Bonuses
static bool IsIndustrialBuilding(CardEnum buildingEnum)
{
	if (IsMountainMine(buildingEnum)) {
		return false;
	}
	
	return IsDirtyProducer(buildingEnum);
}

static bool IsIndustryOrMine(CardEnum buildingEnum)
{
	if (IsMountainMine(buildingEnum)) {
		return true;
	}
	
	return IsDirtyProducer(buildingEnum);
}

static bool IsPollutingHeavyIndustryOrMine(CardEnum buildingEnum)
{
	if (IsMountainMine(buildingEnum)) {
		return true;
	}
	switch (buildingEnum) {
	case CardEnum::Brickworks:
	case CardEnum::IronSmelter:
	case CardEnum::GoldSmelter:
	case CardEnum::IronSmelterGiant:
	case CardEnum::IndustrialIronSmelter:
	case CardEnum::Steelworks:
		
	case CardEnum::CoalPowerPlant:
	case CardEnum::OilPowerPlant:
		return true;
	default:
		return false;
	}
}

static bool IsBudgetAdjustable(CardEnum buildingEnum) {
	return IsProducer(buildingEnum) || buildingEnum == CardEnum::CardMaker;
}

static std::vector<CardEnum> FoodBuildings // Change to AgricultureBuildings
{
	CardEnum::Farm,
	
	CardEnum::FruitGatherer,
	CardEnum::HuntingLodge,
	CardEnum::Fisher,
	CardEnum::MushroomFarm,
	CardEnum::Bakery,
	CardEnum::PitaBakery,
	CardEnum::Beekeeper,
	
	CardEnum::RanchBarn,
	CardEnum::RanchPig,
	CardEnum::RanchSheep,
	CardEnum::RanchCow,
};
static std::vector<CardEnum> NonFoodAgricultureBuildings // Change to AgricultureBuildings
{
	CardEnum::Windmill,
	CardEnum::Forester,
}; // Gets 


static bool IsFoodBuilding(CardEnum cardEnum) {
	return std::find(FoodBuildings.begin(), FoodBuildings.end(), cardEnum) != FoodBuildings.end();
}

static bool IsAgricultureBuilding(CardEnum cardEnum) {
	if (std::find(NonFoodAgricultureBuildings.begin(), NonFoodAgricultureBuildings.end(), cardEnum) != NonFoodAgricultureBuildings.end()) {
		return true;
	}
	return std::find(FoodBuildings.begin(), FoodBuildings.end(), cardEnum) != FoodBuildings.end();
}

static bool IsRanch(CardEnum cardEnum) {
	switch (cardEnum) {
	case CardEnum::RanchPig:
	case CardEnum::RanchSheep:
	case CardEnum::RanchCow:
		return true;
	default:
		return false;
	}
}

static bool IsServiceBuilding(CardEnum buildingEnum)
{
	switch (buildingEnum)
	{
	case CardEnum::Tavern:
	case CardEnum::Theatre:
	//case CardEnum::Zoo:
	//case CardEnum::Museum:
		
	case CardEnum::Library:
	case CardEnum::School:
	case CardEnum::Bank:
		
	case CardEnum::ImmigrationOffice:
		
	case CardEnum::TradingPort:
	case CardEnum::TradingPost:
	case CardEnum::TradingCompany:

	case CardEnum::Market:
	case CardEnum::ShippingDepot:
	case CardEnum::HaulingServices:

	case CardEnum::IntercityLogisticsHub:
	case CardEnum::IntercityLogisticsPort:
		return true;
	default:
		return false;
	}
}

static const std::vector<CardEnum> ProfitBuildings
{
	//CardEnum::Bank,
	CardEnum::Archives,
};
static bool IsProfitBuilding(CardEnum cardEnumIn)
{
	for (CardEnum cardEnum : ProfitBuildings) {
		if (cardEnum == cardEnumIn)	return true;
	}
	return false;
}

static const std::vector<CardEnum> TradingPostLikeBuildings
{
	CardEnum::TradingPost,
	CardEnum::TradingPort,
};
static bool IsTradingPostLike(CardEnum cardEnumIn)
{
	for (CardEnum cardEnum : TradingPostLikeBuildings) {
		if (cardEnum == cardEnumIn)	return true;
	}
	return false;
}

static bool IsEfficiencyBuilding(CardEnum cardEnum)
{
	return IsProducer(cardEnum) ||
				cardEnum == CardEnum::FruitGatherer ||
				cardEnum == CardEnum::Forester ||
				cardEnum == CardEnum::HuntingLodge ||

				cardEnum == CardEnum::TradingCompany ||
				IsTradingPostLike(cardEnum) ||
				cardEnum == CardEnum::Farm;
}


// Any workplace that consume resources
static bool IsConsumerWorkplace(CardEnum buildingEnum) {
	return BuildingInfo[static_cast<int32_t>(buildingEnum)].input1 != ResourceEnum::None;
}

// Workplace that consume resource minus producer
static bool IsConsumerOnlyWorkplace(CardEnum buildingEnum) {
	return IsConsumerWorkplace(buildingEnum) && !IsProducer(buildingEnum);
}

static bool IsSpecialProducer(CardEnum buildingEnum)
{
	switch (buildingEnum) {
	case CardEnum::Mint:
	case CardEnum::CardMaker:
	case CardEnum::ResearchLab:
	//case CardEnum::RegionShrine:
	case CardEnum::ImmigrationOffice:

		return true;
	default: 
		return false;
	}
}

static bool IsHumanHouse(CardEnum buildingEnum) {
	switch (buildingEnum) {
		case CardEnum::House:
		case CardEnum::StoneHouse:
			return true;
		default: return false;
	}
}

static bool IsHouse(CardEnum buildingEnum) {
	switch (buildingEnum) {
		case CardEnum::House:
		case CardEnum::StoneHouse:
		case CardEnum::BoarBurrow:
			return true;
		default: return false;
	}
}

static bool IsTownPlacement(CardEnum buildingEnum)
{
	switch (buildingEnum) {
	case CardEnum::Townhall:
	case CardEnum::Colony:
	case CardEnum::PortColony:
		return true;
	default: return false;
	}
}
static bool IsColonyPlacement(CardEnum buildingEnum)
{
	switch (buildingEnum) {
	case CardEnum::Colony:
	case CardEnum::PortColony:
		return true;
	default: return false;
	}
}

static bool IsPlacementHidingTree(CardEnum buildingEnum)
{
	return IsColonyPlacement(buildingEnum) ||
		buildingEnum == CardEnum::Farm;
}


static bool IsRoad(CardEnum buildingEnum) {
	switch (buildingEnum) {
		case CardEnum::DirtRoad:
		case CardEnum::StoneRoad:
			return true;
		default: return false;
	}
}

static const std::vector<CardEnum> StorageEnums {
	CardEnum::StorageYard,
	CardEnum::Warehouse,
	CardEnum::Market,
	CardEnum::Granary,
};
static bool IsStorage(CardEnum buildingEnum) {
	for (CardEnum storageEnum : StorageEnums) {
		if (storageEnum == buildingEnum) {
			return true;
		}
	}
	return false;
}


static bool IsAutoEraUpgrade(CardEnum buildingEnumIn) { // Upgrade with era without pressing upgrade manually
	switch(buildingEnumIn) {
	case CardEnum::ImmigrationOffice:
	case CardEnum::ResearchLab:
		
	case CardEnum::TradingCompany:
	case CardEnum::TradingPort:
	case CardEnum::TradingPost:

		return false;
	default: break;
	}
	const BldInfo& info = GetBuildingInfo(buildingEnumIn);
	return info.produce == ResourceEnum::None && info.input1 == ResourceEnum::None;
}


static bool IsStackableTileBuilding(CardEnum buildingEnum) {
	switch (buildingEnum) {
	case CardEnum::DirtRoad:
	case CardEnum::StoneRoad:
	//case CardEnum::TrapSpike:
		return true;
	default: return false;
	}
}

static bool IsRoadOverlapBuilding(CardEnum buildingEnum)
{
	switch (buildingEnum) {
	case CardEnum::FenceGate:
	//case CardEnum::TrapSpike:
		return true;
	default: return false;
	}
}

static bool IsCritterBuildingEnum(CardEnum buildingEnum) {
	switch (buildingEnum) {
		case CardEnum::BoarBurrow:
			return true;
		default: return false;
	}
}

static bool IsBuildingInList_Loop(CardEnum buildingEnum, const std::vector<CardEnum>& list) {
	for (CardEnum regionalBldEnum : list) {
		if (buildingEnum == regionalBldEnum) {
			return true;
		}
	}
	return false;
}

/*
 * Foreign-only Buildings
 */
static const std::vector<CardEnum> ForeignOnlyBuildingEnums
{
	CardEnum::Embassy,
	CardEnum::ForeignQuarter,
	CardEnum::ForeignPort,
};

static bool IsForeignOnlyBuilding(CardEnum buildingEnum) {
	return IsBuildingInList_Loop(buildingEnum, ForeignOnlyBuildingEnums);
}


/*
 * Province Buildings
 */
static const std::vector<CardEnum> RegionalBuildingEnums
{
	CardEnum::MinorCity,
	CardEnum::RegionTribalVillage,
	CardEnum::RegionShrine,
	CardEnum::RegionCrates,

	//CardEnum::RegionPort,
};
static bool IsRegionalBuilding(CardEnum buildingEnum) {
	for (CardEnum regionalBldEnum : RegionalBuildingEnums) {
		if (buildingEnum == regionalBldEnum) {
			return true;
		}
	}
	return false;
}

static bool IsBridgeOrTunnel(CardEnum buildingEnum)
{
	return buildingEnum == CardEnum::Bridge ||
		buildingEnum == CardEnum::IntercityBridge ||
		buildingEnum == CardEnum::Tunnel;
}

static bool IsPortBuilding(CardEnum buildingEnum)
{
	switch (buildingEnum) {
	case CardEnum::Fisher:
	case CardEnum::IrrigationPump:
	case CardEnum::SandMine:
	case CardEnum::PaperMaker:
	case CardEnum::TradingPort:
	case CardEnum::MinorCityPort:
	case CardEnum::IntercityLogisticsPort:
	case CardEnum::ForeignPort:
		return true;
	default: return false;
	}
}
static std::pair<int32, int32> DockPlacementExtraInfo(CardEnum cardEnum)
{
	int32 indexLandEnd = 1;
	int32 minWaterCount = 5;

	if (cardEnum == CardEnum::TradingPort) { // 10 x 8
		indexLandEnd = 3; // Note: this is 4 tiles (0,1,2,3)
		minWaterCount = 25; // 8 * 5 = 40 full??
	}
	if (cardEnum == CardEnum::IntercityLogisticsPort) { // 12 x 6
		indexLandEnd = 5; 
		minWaterCount = 16; 
	}
	if (cardEnum == CardEnum::PaperMaker) { // 12 x 6
		indexLandEnd = 2;
		minWaterCount = 27;
	}
	
	return std::make_pair(indexLandEnd, minWaterCount);
}

/*
 * Decorations
 */
static const int32 BadAppealRadius = 12;
struct AppealInfo
{
	int32 appealRadius = -1;
	int32 appealIncrease = 0;
};

static AppealInfo GetBuildingAppealInfo(CardEnum cardEnum)
{
	switch (cardEnum) {
	case CardEnum::Garden: return { 12, 20 };
	case CardEnum::FlowerBed: return { 5, 5 };
	case CardEnum::GardenShrubbery1: return { 5, 5 };
	case CardEnum::GardenCypress: return { 5, 8 };
	}

	if (IsPollutingHeavyIndustryOrMine(cardEnum) ||
		IsMountainMine(cardEnum))
	{
		return { BadAppealRadius, -20 };
	}
	if (IsIndustrialBuilding(cardEnum)) {
		return { BadAppealRadius, -10 };
	}
	
	return AppealInfo();
}

static bool IsAppealAffectingBuilding(CardEnum cardEnum) {
	return GetBuildingAppealInfo(cardEnum).appealRadius > 0;
}

static bool IsDecorativeBuilding(CardEnum cardEnum) {
	return GetBuildingAppealInfo(cardEnum).appealIncrease > 0;
}

//static bool BuildingHasDropdown(CardEnum cardEnum)
//{
//	return cardEnum == CardEnum::BeerBrewery ||
//		cardEnum == CardEnum::Tailor ||
//
//		cardEnum == CardEnum::Forester ||
//		cardEnum == CardEnum::HuntingLodge ||
//		cardEnum == CardEnum::FruitGatherer ||
//
//		IsRanch(cardEnum) ||
//		IsMountainMine(cardEnum);
//}

/*
 * Skill 
 */
static bool IsCardInList(CardEnum cardEnumIn, const std::vector<CardEnum>& cardEnumList) {
	for (CardEnum cardEnum : cardEnumList) {
		if (cardEnumIn == cardEnum) {
			return true;
		}
	}
	return false;
}

static int32 GetSkillManaCost(CardEnum cardEnum)
{
	switch (cardEnum) {
	case CardEnum::SpeedBoost: return 60;
	default:
		UE_DEBUG_BREAK();
		return 10;
	}
}
static const std::vector<CardEnum> SkillEnums
{
	CardEnum::SpeedBoost,
};
static bool IsSkillEnum(CardEnum cardEnum) {
	return IsCardInList(cardEnum, SkillEnums);
}

static bool CanGetSpeedBoosted(CardEnum buildingEnum, bool isConstructed)
{
	if (!isConstructed) {
		return true;
	}

	if (IsRanch(buildingEnum)) {
		return false;
	}

	if (buildingEnum == CardEnum::FruitGatherer ||
		buildingEnum == CardEnum::HuntingLodge) 
	{
		return true;
	}

	return IsProducer(buildingEnum) || 
		IsSpecialProducer(buildingEnum) ||
		IsTradingPostLike(buildingEnum) ||
		buildingEnum == CardEnum::TradingCompany;
	
	//return buildingEnum == CardEnum::Townhall ||
	//	IsStorage(buildingEnum) ||
	//	buildingEnum == CardEnum::Farm ||
	//	IsDecorativeBuilding(buildingEnum) ||
	//	(IsHouse(buildingEnum) && isConstructed) ||
	//	(IsRanch(buildingEnum) && isConstructed) ||
	//	IsRareCard(buildingEnum);
}


enum class PriorityEnum : uint8
{
	Priority,
	NonPriority,
	Disable,
};

struct BuildingUpgrade
{
	FText name;
	FText description;
	ResourcePair baseUpgradeResourceNeeded;

	int32 efficiencyBonus = 0;
	int32 comboEfficiencyBonus = 0;
	int32 workerSlotBonus = 0;
	int32 houseLvlForBonus = 0; // From this, calculate: MaxPercentBonus, resourceCost

	bool isUpgraded = false;

	/*
	 * Why lock with era (not with townhall)?
	 *  - Weird to have Steelworks buildable, but can't upgrade Mine lvl 2 on remote colony
	 */
	int32 startEra = -1;
	int32 upgradeLevel = -1;
	int32 upgradeLevelResourceScalePercent = 50;
	std::vector<ResourcePair> resourceNeededPerLevel;
	
	bool isEraUpgrade() const { return startEra != -1; }
	bool isLevelUpgrade() const { return upgradeLevel != -1; }

	int32 GetAdvancedMachineryLevel() const { return upgradeLevel - resourceNeededPerLevel.size(); }
	
	
	int32 maxUpgradeLevel(CardEnum cardEnum) const
	{
		if (IsTradingPostLike(cardEnum)) {
			return 3;
		}
		return 100;
		//return 5 - startEra; // Old
	}

	ResourcePair currentUpgradeResourceNeeded(int32 upgraderPlayerId = -1)
	{
		if (isEraUpgrade())
		{
			if (upgradeLevel < resourceNeededPerLevel.size()) {
				return resourceNeededPerLevel[upgradeLevel];
			}
			int32 upgradeResourceCount = resourceNeededPerLevel.back().count;
			for (int32 i = resourceNeededPerLevel.size(); i <= upgradeLevel; i++) {
				upgradeResourceCount = upgradeResourceCount * (100 + upgradeLevelResourceScalePercent) / 100; // !! Only Applies Beyond Max Era
			}
			
			return ResourcePair(resourceNeededPerLevel.back().resourceEnum, upgradeResourceCount);
		}
		if (isLevelUpgrade()) 
		{
			int32 upgradeResourceCount = baseUpgradeResourceNeeded.count;
			for (int32 i = 1; i <= upgradeLevel; i++) {
				upgradeResourceCount = upgradeResourceCount * (100 + upgradeLevelResourceScalePercent) / 100;
			}

			return ResourcePair(baseUpgradeResourceNeeded.resourceEnum, upgradeResourceCount);
		}
		return baseUpgradeResourceNeeded;
	}

	static int32 CalculateMaxHouseLvlBonus(int32 houseLvl) {
		return houseLvl * 10; // at House Lvl 8 -> can go up to 80% from 80 House Lvl Count
	}
	

	BuildingUpgrade() : name(FText()), description(FText()), baseUpgradeResourceNeeded(ResourcePair()) {}
	
	BuildingUpgrade(FText name, FText description, ResourcePair resourceNeeded)
		: name(name), description(description), baseUpgradeResourceNeeded(resourceNeeded) {}

	BuildingUpgrade(FText name, FText description, int32 moneyNeeded)
		: name(name), description(description), baseUpgradeResourceNeeded(ResourcePair(ResourceEnum::Money, moneyNeeded)) {}

	BuildingUpgrade(FText name, FText description, ResourceEnum resourceEnum, int32 resourceCount)
		: name(name), description(description), baseUpgradeResourceNeeded(ResourcePair(resourceEnum, resourceCount)) {}

	void operator>>(FArchive& Ar)
	{
		Ar << name;
		Ar << description;
		baseUpgradeResourceNeeded >> Ar;
		
		Ar << efficiencyBonus;
		Ar << comboEfficiencyBonus;
		Ar << workerSlotBonus;
		Ar << houseLvlForBonus;

		Ar << isUpgraded;

		Ar << startEra;
		Ar << upgradeLevel;
		Ar << upgradeLevelResourceScalePercent;
		SerializeVecObj(Ar, resourceNeededPerLevel);
	}
};

struct BonusPair
{
	FText name;
	int32 value;
};


/*
 * Military
 */
enum class ProvinceAttackEnum : uint8
{
	None,
	
	ConquerProvince,
	RaidBattle,
	Raze,
	Vassalize,
	DeclareIndependence,

	VassalCompetition,
	ConquerColony,

	Reinforce,
};


/*
 * Old Army
 */
enum class ArmyEnum : uint8
{
	Tower,
	Clubman,
	Swordman,
	
	Archer,
	
	Count,
};

static const int32 TowerArmyEnumCount = 1;
static const int32 FrontLineArmyEnumCount = 3;

// Army Balance Calculations
// conquering low lvl city is pointless... only small portion of economy in tax
// Estimates:
//  - With at city size 50 with house lvl 2 -> 200 revenue or 40 vassal tax (20% tax) ... And then 5% fee from trade, but also buff vassal with 3% fee deduction
//  - 5 clubman 2 coin maintenance each round... income of 30 per round ... 240 coins per year
//  - Get money back in 1 year, so each clubman cost around 24... But let's just increase it to 60 ... club cost 20 food, 
// Balance:
//  - Set wall HP
//  - 5 clubman can overpower a wall by 2 times...
//  - Power = attack / attackDelay * def * hp
//  - Clubman has attack:10, attackDelay:200, defense:10, hp:500 = power:250
static const int32 BaseUnitPower = 500;
static const int32 BaseUnitCost = 60;
static const int32 ArmyUpkeepToCostPercent = 3;

static const int32 BaseDamagePerAttack = 3;

// LandClaimValue100 produced per unit, per 0.25s
static const int32 UnitCostForLandClaimCalc = 60;
static const int32 AssumeArmyLandClaimBreakevenRounds = 8;
static const int32 LandClaimValue10000PerRound = 10000 * UnitCostForLandClaimCalc / AssumeArmyLandClaimBreakevenRounds;
static const int32 LandClaimValue10000PerQuarterSec = LandClaimValue10000PerRound / Time::SecondsPerRound / 4; // div 4 for quarter sec

struct ArmyInfo
{
	ArmyEnum armyEnum;
	std::string name;
	
	int32 attack;
	int32 attackDelaySec100;
	int32 defense;
	int32 maxHp = 0;

	int32 moneyCost;
	std::vector<ResourcePair> resourceCost;
	int32 moneyAndResourceCost = 0;

	
	ArmyInfo(ArmyEnum armyEnum, std::string name, int32 attack, int32 attackDelaySec100, int32 defense, int32 powerMultiplier, int32 moneyCost, ResourcePair resourceCostIn)
		: armyEnum(armyEnum),
			name(name),
			attack(attack),
			attackDelaySec100(attackDelaySec100),
			defense(defense),
			moneyCost(moneyCost)
	{
		if (resourceCostIn.resourceEnum != ResourceEnum::None) {
			resourceCost.push_back(resourceCostIn);
		}

		moneyAndResourceCost = moneyCost + SumResourceCost(resourceCost);

		int32 preferredPower = BaseUnitPower * moneyAndResourceCost / BaseUnitCost * powerMultiplier / 100;

		// Power = attack / attackDelay * def * hp
		// hp = Power * attackDelay / attack / def
		if (armyEnum == ArmyEnum::Tower) {
			maxHp = (BaseUnitPower * 5 / 2) * attackDelaySec100 / attack / defense; // Tower has 1/3 power of 5 club or (BaseUnitPower * 5 / 2)
		} else {
			maxHp = preferredPower * attackDelaySec100 / attack / defense;
		}
		// round
		maxHp = (maxHp / 10) * 10;

	}

	int32 upkeep100() const { return 100 * moneyAndResourceCost * ArmyUpkeepToCostPercent / 100; }

	int32 maxWallHP(int32 wallLvl) const {
		int32 wallHpMax = maxHp;
		for (int32 i = 1; i < wallLvl; i++) {
			wallHpMax *= 2;
		}
		return wallHpMax;
	}
	int32 wallLvl(int32 initialHP) const {
		int32 wallHpMax = maxHp;
		for (int32 i = 1; i < 10; i++) {
			if (wallHpMax >= initialHP) {
				return i;
			}
			wallHpMax *= 2;
		}
		return 1;
	}

	int32 attackDelayTicks() const {
		return attackDelaySec100 * Time::TicksPerSecond / 100;
	}

	// 100 cost = 60 sec
	int32 timeCostTicks() const { return moneyAndResourceCost * Time::TicksPerSecond * 2; } //
};

static const std::vector<CardEnum> BarrackEnums
{
	CardEnum::BarrackClubman,
	CardEnum::BarrackSwordman,
	CardEnum::BarrackArcher,
};
static CardEnum GetBarrackEnumInt(int armyEnumInt) { return BarrackEnums[armyEnumInt]; }

static bool IsBarrack(CardEnum cardEnum)
{
	for (CardEnum barrackEnum : BarrackEnums) {
		if (cardEnum == barrackEnum) {
			return true;
		}
	}
	return false;
}

static const std::vector<ArmyInfo> ArmyInfos
{
	ArmyInfo(ArmyEnum::Tower,	"Tower",		10, 190, 20, 100, 0, {}),
	ArmyInfo(ArmyEnum::Clubman,	"Clubman",		10, 180, 10, 100,	10, { ResourceEnum::Food, 20}),
	ArmyInfo(ArmyEnum::Swordman,	"Swordman",	20, 240, 20, 170,10, { ResourceEnum::Iron, 15 }),
	ArmyInfo(ArmyEnum::Archer,		"Archer",	30, 200, 5, 120,	30, { ResourceEnum::Wood, 20 }),
};
static const int32 ArmyEnumCount = ArmyInfos.size();

static bool IsFrontline(int32 armyEnumInt) {
	return armyEnumInt < FrontLineArmyEnumCount;
}

static const ArmyInfo& GetArmyInfo(ArmyEnum armyEnum)
{
	PUN_CHECK(static_cast<int>(armyEnum) < ArmyInfos.size());
	return ArmyInfos[static_cast<int>(armyEnum)];
}
static const ArmyInfo& GetArmyInfoInt(int armyEnumInt)
{
	PUN_CHECK(armyEnumInt < ArmyInfos.size());
	return ArmyInfos[armyEnumInt];
}

// Army distance penalty
static const int32 ArmyPenalty_MinDistance = CoordinateConstants::TilesPerRegion * 8;
static const int32 ArmyPenalty_MaxDistance = CoordinateConstants::TilesPerRegion * 24;
static const int32 ArmyAttackDelayPenaltyPercent_AtMaxDistance = 100;

/***************
 * Resource tiles
 */

enum class ResourceTileType
{
	None,
	Tree,
	Bush,
	Deposit,
	Fish,
};

/**
 * Trees/Bushes
 */

enum class TileObjEnum : uint8
{
	Birch, // Common good looking tree

	// Why fruit trees:
	// -Easy to understand that Gatherer's Hut needs to be placed near dense forest.
	// -Subconcious instincts (hoarding, cozy)
	Orange, // Orange Fruit ... Easy to understand, sounds delicious
	
	Apple, // Red Fruit ... Easy to understand, sounds delicious
	Papaya, // Yellow Fruit ... Easy to understand, sounds delicious
	Durian,
	Pine1, // No fruit, high staying power in cold weather
	Pine2,
	GiantMushroom,

	Cherry,
	Coconut,
	Cyathea,
	ZamiaDrosi,

	Cactus1,
	SavannaTree1,
	
	// Oak,
	// Cherry tree
	// Peach tree

	DesertDatePalm,
	DesertGingerbreadTree,

	// ... TreeEnumSize ... Don't forget!!!
	GrassGreen, // Currently includes savanna grass

	OreganoBush,
	CommonBush,
	DesertBush1,
	DesertBush2,
	
	Fern,
	SavannaGrass, // Currently unused...
	JungleThickLeaf,
	
	BlueFlowerBush,
	WhiteFlowerBush,
	RedPinkFlowerBush,

	CommonBush2, // Replaces FieldFlowerFornow
	//FieldFlowerPurple,
	FieldFlowerYellow,
	FieldFlowerHeart,
	FieldFlowerPic,

	//! Wheat to Stone are farm crops
	WheatBush,
	BarleyBush,
	Grapevines,
	Cannabis,
	
	//PlumpCob,
	//CreamPod,
	Cabbage,

	Cocoa,
	Cotton,
	Dye,

	Potato,
	Blueberry,
	Melon,
	Pumpkin,
	RawCoffee,
	Tulip,
	
	Herb,
	//BaconBush,

	//! Beyond stone are ores (Use stone to mark the end of plant)
	Stone,
	CoalMountainOre,
	IronMountainOre,
	GoldMountainOre,
	GemstoneOre,

	Count,

	//--------- size cutoff, after this we have special TreeEnums
	Fish,

	PricklyPearCactus,

	Avocado, // Green Fruit ... Easy to understand, sounds delicious

	Cactus, // Cactus fruit ... Exotic

	// Cold Biome: Pine, Apple, Orange
	// Hot Biome: Orange, Papaya, Avocado

	None,
};

const static int32 TreeEnumSize = static_cast<int32>(TileObjEnum::GrassGreen); // TileObjEnum up to last tree... TODO: change to last tree...
const static int32 TileObjEnumSize = static_cast<int32>(TileObjEnum::Count);

static bool IsTileObjEnumValid(TileObjEnum tileObjEnum) {
	return static_cast<int32_t>(tileObjEnum) < TileObjEnumSize || tileObjEnum == TileObjEnum::None;
}

static bool IsOutcrop(TileObjEnum tileResourceEnum) {
	switch (tileResourceEnum) {
		case TileObjEnum::Stone:
		return true;
	}
	return false;
}

static bool IsPlantFast(TileObjEnum tileObjEnum) {
	return static_cast<int32>(tileObjEnum) < static_cast<uint8>(TileObjEnum::Stone);
}
static bool IsCrop(TileObjEnum tileObjEnum) {
	return static_cast<uint8>(TileObjEnum::WheatBush) <= static_cast<int32>(tileObjEnum) && static_cast<int32>(tileObjEnum) < static_cast<uint8>(TileObjEnum::Stone);
}

static const int32 WinterTrimGrowthPercent = 20;
static const int32 HardyTreeChance = 100;

struct TileObjInfo
{
	// 6 minutes = 1 season = 20 ticks, 1 year = 80 ticks
	TileObjEnum treeEnum;
	FText name;
	ResourceTileType type;

	int32 deathChancePerCycle; // (1- 1/2000)^(80 * 10 years) = 0.67
	int32 fruitChancePerCycle; // Still need fruit grown tick for trees without fruit. Since that determines seeding

	int32 fruitToEmitSeedChance = 2; // Only 1 / 3 fruits emit seed

	// After max growth, and tree will continue to grow (+10x maxGrowthTick = 0.4 more scale)
	// maxGrowthTick is also the point where tree starts bearing fruit
	int32 maxGrowthTick;

	ResourcePair fruitResourceBase100;
	ResourcePair cutDownResourceBase100;

	FText description;

	std::string nameStr() const { return FTextToStd(name); }

	
	int32 treeEnumInt() { return static_cast<int32>(treeEnum); }
	ResourceEnum harvestResourceEnum() { return cutDownResourceBase100.resourceEnum; }

	TileObjInfo(
		TileObjEnum treeEnumIn, FText nameIn, ResourceTileType typeIn,
		ResourcePair fruitResourceBase100In, ResourcePair cutDownResourceBase100In, FText descriptionIn)
	{
		treeEnum = treeEnumIn;
		name = nameIn;
		type = typeIn;

		// Death Chance
		deathChancePerCycle = 0;
		if (typeIn == ResourceTileType::Bush && !IsCrop(treeEnum)) {
			deathChancePerCycle = Time::TicksPerYear / UpdateChunkCount; // most likely die in 1 season after full age
		}
		if (typeIn == ResourceTileType::Tree) {
			deathChancePerCycle = 10000;
		}
		deathChancePerCycle = std::max(deathChancePerCycle, 1);


		// Fruit Chance
		static const int32 FruitChancePerCycleTree = 50;
		static const int32 FruitChancePerCycleBush = 20;
		
		fruitChancePerCycle = 0;
		if (typeIn == ResourceTileType::Tree) {
			fruitChancePerCycle = FruitChancePerCycleTree;
			if (treeEnum == TileObjEnum::Cactus1 ||
				treeEnum == TileObjEnum::SavannaTree1) 
			{
				fruitChancePerCycle = FruitChancePerCycleTree * HardyTreeChance;
			}
		}
		if (typeIn == ResourceTileType::Bush && !IsCrop(treeEnum)) {
			fruitChancePerCycle = FruitChancePerCycleBush;
		}

		// maxGrowthSeasons100
		int32 maxGrowthSeasons100 = 0;
		if (typeIn == ResourceTileType::Tree) {
			maxGrowthSeasons100 = 500; // 1 year 1 season to grow for trees
		}
		if (typeIn == ResourceTileType::Bush) {
			if (IsCrop(treeEnum)) {
				maxGrowthSeasons100 = 170;
			}
			else {
				maxGrowthSeasons100 = 170;
			}
		}
		
		
		maxGrowthTick = maxGrowthSeasons100 * Time::TicksPerSeason / 100;
		

		fruitResourceBase100 = fruitResourceBase100In;
		cutDownResourceBase100 = cutDownResourceBase100In;

		description = descriptionIn;
	}

public:
	static const int32 UpdateChunkCount = 2048; // means ~34s per cycle?

	static const int32 FellAnimationTicks = 180;
	static const int32 FellRotateTicks = 120;

	static const int32 LifespanMultiplier = 2;

	int32 lifeSpanTicks() const { return maxGrowthTick * LifespanMultiplier; }

	int32 trimmedGrowthTicks() const {
		return (GameRand::Rand() % (maxGrowthTick * 10 / 100)) + (maxGrowthTick * 20 / 100); // randomized 20 - 30% growth, need random or bushes will look uniform out of spring...
	}
	int32 randomGrowthTicks() const {
		return GameRand::Rand() % (maxGrowthTick * 3);
		// randomize up to 3 since LifeSpanMultiplier is 2
		// Note: If we do 1, there are many bush that suddenly dies together
	}

	int32 growthPercent(int32_t age) const {
		return std::min(100, age * 100 / maxGrowthTick);
	}
	int32 bonusYieldPercent(int32_t age) const {
		const int32 bonusYieldFactor = 4;
		return std::max(0, (age * 100 / maxGrowthTick - 100) / bonusYieldFactor);
	}
	int32 lifeSpanPercent(int32_t age) const {
		return std::min(100, age * 100 / lifeSpanTicks());
	}
	int32 tileYieldPercent(int32_t age) const { return growthPercent(age) + bonusYieldPercent(age); }


	// Motivations:
	// - Animals can trim land to nothing...
	// - Animals can make land look like desert

	// Balancing mechanisms:
	// - Human's farm plants are food rich, but doesn't reproduce well ... (decouple human farm food balancing from animal ecosystem)
	// - Animals trim, not kill plants ... (Overpopulation of animals won't kill all plants too quickly)

	//
	// Animal Harvestable ... 50% grown, Animal Harvestable
	// Fully Grown ... 100% grown, human harvestable, full-size
	// Can Die ... 

	// Bush is ready for harvest for animals at 50%
	// Not doing 100% since this will leave a lot of nearly fullgrown larger bushes, with this, it may look weird that animals are not eating large bushes..
	int32 isBushReadyForAnimalHarvest(int32 age) const { return age > (maxGrowthTick * 3 / 4); }
	int32 isSeedEmissionAge(int32 age) const { return age > (maxGrowthTick / 2); }
	//int32_t isFullyGrown(int32_t age) const { return age > maxGrowthTick; }


	
	int32 canDieFromAge(int32_t age) const { return age > lifeSpanTicks(); }

	bool IsPlant() { return type == ResourceTileType::Tree || type == ResourceTileType::Bush; }

	bool IsFruitBearer() { return fruitResourceBase100.isValid(); }

	ResourcePair fruitResource100(int32 age) const {
		int32 yield100 = fruitResourceBase100.count * tileYieldPercent(age);
		int32 amount = yield100 / 100;
		if ((yield100 % 100) > (GameRand::Rand() % 100)) { // use random number to fluctuate yield to get accurate yield...
			amount++;
		}
		PUN_CHECK(amount >= 0);
		amount = std::max(1, amount); // Min 1 resource
		return ResourcePair(fruitResourceBase100.resourceEnum, amount);
	}

	ResourcePair cutDownResource(int32 age, int32 percentLeft, int32 trimEfficiency = 100) const {
		int32 yield100 = (tileYieldPercent(age) - percentLeft) * cutDownResourceBase100.count * trimEfficiency / 100 / 100;
		int32 amount = yield100 / 100;
		if ((yield100 % 100) > (GameRand::Rand() % 100)) { // use random number to fluctuate yield to get accurate yield...
			amount++;
		}
		amount = std::max(1, amount); // Min 1 resource
		return ResourcePair(cutDownResourceBase100.resourceEnum, amount);
	}

	static int32 TicksPerCycle() { return UpdateChunkCount; }

	// Texts
#define LOCTEXT_NAMESPACE "TileObjInfo"
	static FText commonFlowerDesc() { return LOCTEXT("Common Flower Desc", "Common flower. A nice food-source for grazing animals.");}
	static FText wheatGrassDesc() { return LOCTEXT("Wheat Grass Desc", "Grass that can be cultivated for its seed."); }
#undef LOCTEXT_NAMESPACE
};

static const int32 GrassToBushValue = 3;

/*
 *
 * Note: Gather production tied into the HumanFoodCostPerYear through AssumedFoodProduction100PerYear
 */
static const int32 GatherUnitsPerYear = 6; // 6;  // GatherBaseYield 4 gives 60 fruits per year... So 15 estimated gather per year...
static const int32 GatherBaseYield100 = AssumedFoodProduction100PerYear / GatherUnitsPerYear;
static const int32 JungleGatherBaseYield100 = GatherBaseYield100 * 2;

static const int32 HayBaseYield = 50; // 50 is from tuning
static const ResourcePair defaultWood100(ResourceEnum::Wood, WoodGatherYield_Base100);
static const ResourcePair defaultHay100(ResourceEnum::Hay, HayBaseYield);
static const ResourcePair defaultGrass100(ResourceEnum::Hay, HayBaseYield / GrassToBushValue);

#define LOCTEXT_NAMESPACE "TileObjInfo"

static const TileObjInfo TreeInfos[] = {
	//TileObjEnum treeEnumIn, std::string nameIn, ResourceTileType typeIn,
	//int32_t deathChancePerCycleIn, int32_t fruitChancePerCycleIn,  int32_t maxGrowthSeasons100,
	//ResourcePair fruitResourceBaseIn, ResourcePair cutDownResourceBaseIn, std::string descriptionIn)
	TileObjInfo(TileObjEnum::Birch,	LOCTEXT("Birch", "Birch"),	ResourceTileType::Tree,	ResourcePair::Invalid(),						defaultWood100, LOCTEXT("Birch Desc", "Fast-growing hardwood tree.")),
	TileObjInfo(TileObjEnum::Orange,	LOCTEXT("Orange", "Orange"),	ResourceTileType::Tree,	ResourcePair(ResourceEnum::Orange, GatherBaseYield100), defaultWood100, LOCTEXT("Orange Desc", "Orange trees bear delicious fruits during non-winter seasons.")),

	TileObjInfo(TileObjEnum::Apple,	LOCTEXT("Apple", "Apple"),	ResourceTileType::Tree,	ResourcePair(ResourceEnum::Orange, GatherBaseYield100), defaultWood100, LOCTEXT("Apple Desc", "Apple trees bear delicious fruits during non-winter seasons.")),
	TileObjInfo(TileObjEnum::Papaya,	LOCTEXT("Papaya", "Papaya"),	ResourceTileType::Tree,	ResourcePair(ResourceEnum::Papaya, JungleGatherBaseYield100), defaultWood100, LOCTEXT("Papaya Desc", "Papaya trees bear delicious fruits during non-winter seasons.")),
	TileObjInfo(TileObjEnum::Durian,	LOCTEXT("Durian", "Durian"),	ResourceTileType::Tree,	ResourcePair(ResourceEnum::Orange, GatherBaseYield100), defaultWood100, LOCTEXT("Durian Desc", "Durian trees bear delicious fruits during non-winter seasons.")),
	TileObjInfo(TileObjEnum::Pine1,	LOCTEXT("Pine", "Pine"),	ResourceTileType::Tree,	ResourcePair::Invalid(),									defaultWood100, LOCTEXT("Pine Desc", "Fast-growing hardwood tree.")),
	TileObjInfo(TileObjEnum::Pine2,	LOCTEXT("Pine", "Pine"),	ResourceTileType::Tree,	ResourcePair::Invalid(),									defaultWood100, LOCTEXT("Pine Desc", "Fast-growing hardwood tree.")),
	TileObjInfo(TileObjEnum::GiantMushroom,	LOCTEXT("Giant Mushroom", "Giant Mushroom"),	ResourceTileType::Tree,	ResourcePair::Invalid(),					ResourcePair(ResourceEnum::Mushroom, WoodGatherYield_Base100 * 2), LOCTEXT("Giant Mushroom Desc", "A very large mushroom...")),

	TileObjInfo(TileObjEnum::Cherry,	LOCTEXT("Cherry", "Cherry"),	ResourceTileType::Tree,	ResourcePair(ResourceEnum::Orange, GatherBaseYield100), defaultWood100, LOCTEXT("Cherry Desc", "Cherry trees bear delicious fruit during non-winter seasons.")),
	TileObjInfo(TileObjEnum::Coconut,	LOCTEXT("Coconut", "Coconut"),	ResourceTileType::Tree,	ResourcePair(ResourceEnum::Coconut, GatherBaseYield100),								defaultWood100, LOCTEXT("Coconut Desc", "Coconut bears delicious fruit during non-winter seasons.")),
	TileObjInfo(TileObjEnum::Cyathea,	LOCTEXT("Cyathea", "Cyathea"),	ResourceTileType::Tree,	ResourcePair::Invalid(),								defaultWood100, LOCTEXT("Cyathea Desc", "Fern tree.")),
	TileObjInfo(TileObjEnum::ZamiaDrosi,	LOCTEXT("Zamia Drosi", "Zamia Drosi"),	ResourceTileType::Tree,	ResourcePair::Invalid(),						defaultWood100, LOCTEXT("Zamia Drosi Desc", "Giant leaf tropical tree.")),

	TileObjInfo(TileObjEnum::Cactus1,	LOCTEXT("Cactus", "Cactus"),	ResourceTileType::Tree,	ResourcePair::Invalid(),						defaultWood100, LOCTEXT("Cactus Desc", "Desert plant with thick leafless stem covered in sharp spikes. Hurts to touch.")),
	TileObjInfo(TileObjEnum::SavannaTree1,	LOCTEXT("Savanna Acacia", "Savanna Acacia"),	ResourceTileType::Tree,	ResourcePair::Invalid(),						defaultWood100, LOCTEXT("Savanna Acacia Desc", "Myths say acacia trees descended from an ancient tree of life.")),

	TileObjInfo(TileObjEnum::DesertDatePalm,	LOCTEXT("Date Palm", "Date Palm"),	ResourceTileType::Tree,	ResourcePair(ResourceEnum::DateFruit, GatherBaseYield100),						defaultWood100, LOCTEXT("Date Palm Desc", "TODO:TEXT")),
	TileObjInfo(TileObjEnum::DesertGingerbreadTree,	LOCTEXT("Gingerbread Tree", "Gingerbread Tree"),	ResourceTileType::Tree,	ResourcePair::Invalid(),						defaultWood100, LOCTEXT("Gingerbread Tree Desc", "TODO:TEXT")),


	//! Bushes
	TileObjInfo(TileObjEnum::GrassGreen, LOCTEXT("Grass", "Grass"),	ResourceTileType::Bush,	ResourcePair::Invalid(),		defaultGrass100, LOCTEXT("Grass Desc", "Common grass. A nice food-source for grazing animals.")),

	TileObjInfo(TileObjEnum::OreganoBush, LOCTEXT("Common Bush", "Common Bush"),	ResourceTileType::Bush,	ResourcePair::Invalid(), defaultHay100, LOCTEXT("Common Bush Desc", "Fluffy little bush. A nice food-source for grazing animals.")),
	TileObjInfo(TileObjEnum::CommonBush, LOCTEXT("Common Bush", "Common Bush"), ResourceTileType::Bush,	ResourcePair::Invalid(), defaultHay100, LOCTEXT("Common Bush Desc", "Fluffy little  bush. A nice food-source for grazing animals.")),
	TileObjInfo(TileObjEnum::DesertBush1, LOCTEXT("Desert Bush", "Desert Bush"), ResourceTileType::Bush,	ResourcePair::Invalid(), defaultHay100, LOCTEXT("Common Bush Desc", "Fluffy little  bush. A nice food-source for grazing animals.")),
	TileObjInfo(TileObjEnum::DesertBush2, LOCTEXT("Desert Bush", "Desert Bush"), ResourceTileType::Bush,	ResourcePair::Invalid(), defaultHay100, LOCTEXT("Common Bush Desc", "Fluffy little  bush. A nice food-source for grazing animals.")),

	TileObjInfo(TileObjEnum::Fern, LOCTEXT("Fern", "Fern"),				ResourceTileType::Bush,	ResourcePair::Invalid(), defaultHay100,  LOCTEXT("Fern Desc", "Common plant. A nice food-source for grazing animals.")),
	TileObjInfo(TileObjEnum::SavannaGrass, LOCTEXT("Savanna Grass", "Savanna Grass"),	ResourceTileType::Bush,	ResourcePair::Invalid(), defaultGrass100, LOCTEXT("Savanna Grass Desc", "Common plant. A nice food-source for grazing animals.")),
	TileObjInfo(TileObjEnum::JungleThickLeaf, LOCTEXT("Jungle Plant", "Jungle Plant"),	ResourceTileType::Bush,	ResourcePair::Invalid(), defaultHay100, LOCTEXT("Jungle Plant Desc", "Common plant. A nice food-source for grazing animals.")),

	
	TileObjInfo(TileObjEnum::BlueFlowerBush, LOCTEXT("Primula Flower", "Primula Flower"),	ResourceTileType::Bush,	ResourcePair::Invalid(), defaultHay100, TileObjInfo::commonFlowerDesc()),
	TileObjInfo(TileObjEnum::WhiteFlowerBush, LOCTEXT("Daisy Flower", "Daisy Flower"),	ResourceTileType::Bush,	ResourcePair::Invalid(), defaultHay100, TileObjInfo::commonFlowerDesc()),

	TileObjInfo(TileObjEnum::RedPinkFlowerBush, LOCTEXT("Porin Flower", "Porin Flower"),	ResourceTileType::Bush,	ResourcePair::Invalid(), defaultHay100, TileObjInfo::commonFlowerDesc()),

	TileObjInfo(TileObjEnum::CommonBush2, LOCTEXT("Common Bush", "Common Bush"), ResourceTileType::Bush,	ResourcePair::Invalid(), defaultHay100, LOCTEXT("Common Bush Desc", "Fluffy little  bush. A nice food-source for grazing animals.")),
	//TileObjInfo(TileObjEnum::FieldFlowerPurple, "FieldFlowerPurple",	ResourceTileType::Bush,	0,	0,	ResourcePair::Invalid(), defaultHay100, "Common flower. A nice food-source for grazing animals."),
	TileObjInfo(TileObjEnum::FieldFlowerYellow, INVTEXT("FieldFlowerYellow"),	ResourceTileType::Bush,	ResourcePair::Invalid(), defaultHay100, TileObjInfo::commonFlowerDesc()),

	TileObjInfo(TileObjEnum::FieldFlowerHeart, INVTEXT("FieldFlowerHeart"),	ResourceTileType::Bush,	ResourcePair::Invalid(), defaultHay100, TileObjInfo::commonFlowerDesc()),
	TileObjInfo(TileObjEnum::FieldFlowerPic, INVTEXT("FieldFlowerPic"),	ResourceTileType::Bush,	ResourcePair::Invalid(), defaultHay100, TileObjInfo::commonFlowerDesc()),

	// Wheat to Stone are farm crops
	TileObjInfo(TileObjEnum::WheatBush, LOCTEXT("Wheat", "Wheat"),	ResourceTileType::Bush,	ResourcePair::Invalid(), ResourcePair(ResourceEnum::Wheat, FarmBaseYield100), TileObjInfo::wheatGrassDesc()),
	TileObjInfo(TileObjEnum::BarleyBush, LOCTEXT("Barley", "Barley"),	ResourceTileType::Bush,	ResourcePair::Invalid(), ResourcePair(ResourceEnum::Wheat, FarmBaseYield100), TileObjInfo::wheatGrassDesc()),
	TileObjInfo(TileObjEnum::Grapevines, LOCTEXT("Grapevines", "Grapevines"),	ResourceTileType::Bush,	ResourcePair::Invalid(), ResourcePair(ResourceEnum::Grape, FarmBaseYield100), LOCTEXT("Grapevines Desc", "Produces delicious Grape that can be eaten fresh or make expensive wine.")),
	TileObjInfo(TileObjEnum::Cannabis, LOCTEXT("Cannabis", "Cannabis"),	ResourceTileType::Bush,	ResourcePair::Invalid(), ResourcePair(ResourceEnum::Cannabis, FarmBaseYield100), LOCTEXT("Cannabis Desc", "Plant whose parts can be smoked or added to food for recreational purposes.")),

	//TileObjInfo(TileObjEnum::PlumpCob, "Plump cob",	ResourceTileType::Bush,				1,	0,	170,	ResourcePair::Invalid(), ResourcePair(ResourceEnum::Wheat, FarmBaseYield100), "Produces large soft yellow tasty cob. Need 2 years before it is ready for harvest, but has 3x yield."),
	//TileObjInfo(TileObjEnum::CreamPod, "Cream pod",	ResourceTileType::Bush,				1,	0,	170,	ResourcePair::Invalid(), ResourcePair(ResourceEnum::Wheat, FarmBaseYield100), "Produces round pods, which, when cut open, reveals thick sweet cream substance."),
	TileObjInfo(TileObjEnum::Cabbage, LOCTEXT("Cabbage", "Cabbage"),	ResourceTileType::Bush,	ResourcePair::Invalid(), ResourcePair(ResourceEnum::Cabbage, FarmBaseYield100 * 120 / 100), LOCTEXT("Cabbage Desc", "Healthy vegetable great for making salad.")),

	TileObjInfo(TileObjEnum::Cocoa,	LOCTEXT("Cocoa", "Cocoa"),	ResourceTileType::Bush,	ResourcePair::Invalid(), ResourcePair(ResourceEnum::Cocoa, FarmBaseYield100), LOCTEXT("Cocoa Desc", "Cocoa used to make delicious chocolate.")),
	TileObjInfo(TileObjEnum::Cotton,	LOCTEXT("Cotton", "Cotton"),	ResourceTileType::Bush,	ResourcePair::Invalid(), ResourcePair(ResourceEnum::Cotton, FarmBaseYield100), LOCTEXT("Cotton Desc",  "Cotton used to make Cotton Fabric.")),
	TileObjInfo(TileObjEnum::Dye,		LOCTEXT("Dye", "Dye"),	ResourceTileType::Bush,	ResourcePair::Invalid(), ResourcePair(ResourceEnum::Dye, FarmBaseYield100), LOCTEXT("Dye Desc", "Dye used to dye Cotton Fabric or print Book.")),

	// Dec 17
	TileObjInfo(TileObjEnum::Potato,	LOCTEXT("Potato", "Potato"),	ResourceTileType::Bush,	ResourcePair::Invalid(), ResourcePair(ResourceEnum::Potato, FarmBaseYield100), LOCTEXT("Potato Desc", "Common tuber.")),
	TileObjInfo(TileObjEnum::Blueberry,	LOCTEXT("Blueberries", "Blueberries"),	ResourceTileType::Bush,	ResourcePair::Invalid(), ResourcePair(ResourceEnum::Blueberries, FarmBaseYield100), LOCTEXT("Blueberries Desc", "Blue-skinned fruit with refreshing taste.")),
	TileObjInfo(TileObjEnum::Melon,		LOCTEXT("Melon", "Melon"),	ResourceTileType::Bush,	ResourcePair::Invalid(), ResourcePair(ResourceEnum::Melon, FarmBaseYield100), LOCTEXT("Melon Desc", "Sweet and refreshing fruit. +5<img id=\"Coin\"/> each unit when consumed.")),
	TileObjInfo(TileObjEnum::Pumpkin,	LOCTEXT("Pumpkin", "Pumpkin"),	ResourceTileType::Bush,	ResourcePair::Invalid(), ResourcePair(ResourceEnum::Pumpkin, FarmBaseYield100), LOCTEXT("Pumpkin Desc", "Fruit with delicate, mildly-flavored flesh.")),
	TileObjInfo(TileObjEnum::RawCoffee,	LOCTEXT("Raw Coffee", "Raw Coffee"),	ResourceTileType::Bush,	ResourcePair::Invalid(), ResourcePair(ResourceEnum::RawCoffee, FarmBaseYield100), LOCTEXT("Raw Coffee Desc", "Fruit that can be roasted to make Coffee.")),
	TileObjInfo(TileObjEnum::Tulip,		LOCTEXT("Tulip", "Tulip"),	ResourceTileType::Bush,	ResourcePair::Invalid(), ResourcePair(ResourceEnum::Tulip, FarmBaseYield100), LOCTEXT("Tulip Desc", "Beautiful decorative flower. (Luxury tier 1)")),

	
	TileObjInfo(TileObjEnum::Herb,		LOCTEXT("Medicinal Herb", "Medicinal Herb"),		ResourceTileType::Bush,	ResourcePair::Invalid(), ResourcePair(ResourceEnum::Herb, FarmBaseYield100), LOCTEXT("Medicinal Herb Desc", "Herb used to heal sickness or make medicine.")),

	TileObjInfo(TileObjEnum::Stone, LOCTEXT("Stone", "Stone"),	ResourceTileType::Deposit,	ResourcePair::Invalid(),								ResourcePair(ResourceEnum::Stone, 2) /*this is not used?*/, LOCTEXT("Stone Desc", "Easily-accessible stone deposits.")),

	TileObjInfo(TileObjEnum::CoalMountainOre, LOCTEXT("Coal Ore", "Coal Ore"),	ResourceTileType::Deposit,	ResourcePair::Invalid(), ResourcePair(ResourceEnum::Stone, 3), LOCTEXT("CoalOreDesc", "This region contains Coal that can be mined from mountain.")),
	TileObjInfo(TileObjEnum::IronMountainOre, LOCTEXT("Iron Ore", "Iron Ore"),	ResourceTileType::Deposit,	ResourcePair::Invalid(), ResourcePair(ResourceEnum::Stone, 3), LOCTEXT("IronOreDesc", "This region contains Iron Ore that can be mined from mountain.")),
	TileObjInfo(TileObjEnum::GoldMountainOre, LOCTEXT("Gold Ore", "Gold Ore"),	ResourceTileType::Deposit,	ResourcePair::Invalid(), ResourcePair(ResourceEnum::Stone, 3), LOCTEXT("GoldOreDesc", "This region contains Gold Ore that can be mined from mountain.")),
	TileObjInfo(TileObjEnum::GemstoneOre, LOCTEXT("Gemstone", "Gemstone"),	ResourceTileType::Deposit,	ResourcePair::Invalid(), ResourcePair(ResourceEnum::Stone, 3), LOCTEXT("GemstoneDesc", "This region contains Gemstone that can be mined from mountain.")),


	// ----------------------------------------------------------------------------------------------------------------------------------------------------------

	//TileObjInfo(TileObjEnum::PricklyPearCactus, "Prickly Pear Cactus",	ResourceTileType::Bush,ResourceEnum::Orange,			1,	10,	2,	0, "This cactus produces sweet red fruits."),

	//TileObjInfo(TileObjEnum::WheatBush, "Saguaro Cactus",	ResourceTileType::Bush,ResourceEnum::None,			1,	10,	2,	0, "Hardy cactus can be harvest wood."),


	TileObjInfo(TileObjEnum::Count, INVTEXT("-----------"),	ResourceTileType::Bush,	ResourcePair::Invalid(), ResourcePair(ResourceEnum::None, 5), INVTEXT("-----------")),

	TileObjInfo(TileObjEnum::Fish, LOCTEXT("Fish", "Fish"),	ResourceTileType::Fish,	ResourcePair::Invalid(), ResourcePair(ResourceEnum::Fish, 5), LOCTEXT("Fish", "Fish")),

	//TileObjInfo(TileObjEnum::Avocado, "Avocado",	ResourceTileType::Tree,	10000,	80 * 3,	50,	ResourcePair::Invalid(), ResourcePair(ResourceEnum::None, 5), "A tree that bears delicious and healthy fruits"),
};
static const int32 TileObjEnumCount = _countof(TreeInfos);

#undef LOCTEXT_NAMESPACE

static void TileObjInfosIntegrityCheck()
{
	for (int i = 0; i < TileObjEnumSize; i++) {
		PUN_CHECK(TreeInfos[i].treeEnum == static_cast<TileObjEnum>(i));
	}
}

static TileObjInfo GetTileObjInfoInt(int32 tileObjEnumInt) {
	if (tileObjEnumInt == static_cast<int32>(TileObjEnum::None)) {
		const FText noneText = NSLOCTEXT("TileObjInfo", "None", "None");
		static TileObjInfo noneTileInfo(TileObjEnum::None, noneText, ResourceTileType::None, ResourcePair::Invalid(), ResourcePair::Invalid(), noneText);
		return noneTileInfo;
	}
	PUN_CHECK(tileObjEnumInt < TileObjEnumCount);
	return TreeInfos[tileObjEnumInt];
}
static TileObjInfo GetTileObjInfo(TileObjEnum tileObjEnum) {
	return GetTileObjInfoInt(static_cast<int32>(tileObjEnum));
}

static bool IsGrass(TileObjEnum tileObjEnum) {
	return GetTileObjInfo(tileObjEnum).cutDownResourceBase100 == defaultGrass100;
}

static int32_t TileObjEnumFromName(FString name) {
	for (int i = 0; i < TileObjEnumSize; i++) {
		if (name.Equals(TreeInfos[i].name.ToString())) {
			return i;
		}
	}
	PUN_NOENTRY();
	return -1;
}

/**
 * Map
 */

#define LOCTEXT_NAMESPACE "TerrainTileTypeName"

enum class TerrainTileType : uint8
{
	None = 0,
	River,
	Ocean,
	Mountain,
	ImpassableFlat,
};
static const TArray<FText> TerrainTileTypeName
{
	LOCTEXT("None", "None"),
	LOCTEXT("River", "River"),
	LOCTEXT("Ocean", "Ocean"),
	LOCTEXT("Mountain", "Mountain"),
	LOCTEXT("Impassable Terrain", "Impassable Terrain"),
};

#undef LOCTEXT_NAMESPACE

static FText GetTerrainTileTypeName(TerrainTileType type) {
	return TerrainTileTypeName[static_cast<int>(type)];
}

static bool IsWaterTileType(TerrainTileType tileType) {
	return tileType == TerrainTileType::River || tileType == TerrainTileType::Ocean;
}

static bool IsLandPassableTileType(TerrainTileType tileType) {
	return tileType == TerrainTileType::None || tileType == TerrainTileType::River;
}

class GameMap
{
public:
	static void SetRegionsPerWorld(int regionPerWorldX, int regionPerWorldY);

	//static void Serialize(FArchive& Ar, TArray<uint8>& data);

	static bool IsInGrid(int32_t x, int32_t y) {
		return GameMapConstants::TilesPerWorldX > x && x >= 0 && GameMapConstants::TilesPerWorldY > y && y >= 0;
	}
	static bool IsInGrid(WorldTile2 tile) {
		return GameMapConstants::TilesPerWorldX > tile.x && tile.x >= 0 && GameMapConstants::TilesPerWorldY > tile.y && tile.y >= 0;
	}

	//! Helpers
	static int32 GetFacingStep(Direction faceDirection, TileArea area, WorldTile2 tile)
	{
		switch (faceDirection)
		{
		case Direction::N: return area.maxX - tile.x; // Step going -x
		case Direction::S: return tile.x - area.minX; // Step going +x
		case Direction::E: return area.maxY - tile.y; // Step going -y
		case Direction::W: return tile.y - area.minY; // Step going +y
		}
		UE_DEBUG_BREAK();
		return 0;
	}
	static bool IsLastStep(Direction faceDirection, TileArea area, WorldTile2 tile)
	{
		switch (faceDirection)
		{
		case Direction::N: return area.minX == tile.x; // Step going -x
		case Direction::S: return tile.x == area.maxX; // Step going +x
		case Direction::E: return area.minY == tile.y; // Step going -y
		case Direction::W: return tile.y == area.maxY; // Step going +y
		}
		UE_DEBUG_BREAK();
		return false;
	}
};

class GameInfo
{
public:
	static const int32_t PlayerIdNone = -1;
};

class HumanGlobalInfo
{
public:
	static const int64_t MoveAtomsPerTick = CoordinateConstants::AtomsPerTile / Time::TicksPerSecond * 2;// *3;
};

enum class PlacementType : uint8
{
	None,
	Building,
	BuildingDrag,
	StorageDrag,
	
	Gather,
	GatherRemove,
	Demolish,
	DirtRoad,
	StoneRoad,
	IntercityRoad,
	Fence,
	Bridge,
	IntercityBridge,
	Tunnel,
	IrrigationDitch,

	DeliveryTarget,
	RevealSpyNest,
};

static bool IsRoadPlacement(PlacementType placementType) {
	return placementType == PlacementType::DirtRoad ||
		placementType == PlacementType::StoneRoad ||
		placementType == PlacementType::IntercityRoad;
}
static bool IsGatherPlacement(PlacementType placementType) {
	return placementType == PlacementType::Gather ||
		placementType == PlacementType::GatherRemove;
}

static bool IsBridgePlacement(PlacementType placementType)
{
	return placementType == PlacementType::Bridge ||
		placementType == PlacementType::IntercityBridge;
}

static const int32 IntercityRoadTileCost = 20;
static const int32 IrrigationDitchTileCost = 100;

enum class PlacementGridEnum : uint8
{
	Green,
	Red,
	Gray,
	ArrowGreen,
	ArrowYellow,
	ArrowRed,
};

struct PlacementGridInfo
{
	PlacementGridEnum gridEnum;
	WorldTile2 location;
	Direction direction = Direction::S;
};

// Drag can start by leftClick down/up on the same tile. This drag will end with another leftClickDown
// Or leftClick down, mouse move to new tile. This drag ends on mouse release.
enum class DragState
{
	None,
	NeedDragStart,
	LeftMouseDown, // Left mouse is down, but not yet dragging
	Dragging,
};

enum class PlacementInstructionEnum
{
	OutsideTerritory,
	DragGather,
	DragRoad1,
	DragRoad2,
	DragRoadStone,
	DragRoadIntercity,

	DragStorageYard,
	DragFarm,
	
	DragDemolish,

	DeliveryPointInstruction,
	DeliveryPointMustBeStorage,
	ShipperDeliveryPointShouldBeOutOfRadius,

	MountainMine,
	Dock,
	MustBeNearRiver,
	LogisticsOffice,

	Fort,
	Colony,
	ColonyNoGeoresource,
	
	ColonyNeedsEmptyProvinces,
	ColonyNeedsPopulation,
	ColonyNextToIntercityRoad,
	PortColonyNeedsPort,
	ColonyTooFar,
	PortColonyTooFar,
	ColonyClaimCost,
	
	FarmAndRanch,
	FarmNoValidSeedForRegion,
	NeedGeoresource,
	
	TownhallTarget,
	FireOnTownhall,
	ForeignBuilding,

	YourBuildingTarget,
	//SkillOnOwnedBuilding,
	NotThisBuildingTarget,

	Kidnap,

	Generic,

	// Secondary instructions...
	Rotate,
	Shift,

	Count,
};

struct PlacementInstructionInfo
{
	bool shouldShow = false;
	int32 intVar1 = -1;
	int32 intVar2 = -1;
	FText instruction;
};

struct PlacementInfo
{
	PlacementType placementType = PlacementType::None;
	CardEnum buildingEnum = CardEnum::None;
	std::vector<PlacementInstructionInfo> requiredInstructions;
	
	int32 buildingLvl = -1;

	WorldTile2 mouseOnTile = WorldTile2::Invalid;
	FVector placementLocation;

	PlacementInfo() {}

	PlacementInfo(PlacementType placementType, CardEnum buildingEnum, std::vector<PlacementInstructionInfo> requiredInstructions, WorldTile2 mouseOnTile, FVector placementLocation)
		: placementType(placementType), buildingEnum(buildingEnum), requiredInstructions(requiredInstructions),
		mouseOnTile(mouseOnTile), placementLocation(placementLocation) {}
};

static bool HasBuildingFront(CardEnum cardEnum)
{
	PUN_CHECK(IsBuildingCard(cardEnum));
	return cardEnum != CardEnum::Farm &&
			cardEnum != CardEnum::StorageYard &&
			cardEnum != CardEnum::FakeTribalVillage &&
			cardEnum != CardEnum::ChichenItza &&
			
			cardEnum != CardEnum::RegionTribalVillage &&

			!IsDecorativeBuilding(cardEnum);
}

static bool IsStorageTooLarge(TileArea area) {
	return (area.sizeX() / 2 * 2) > 8 || 
		   (area.sizeY() / 2 * 2) > 8; // 4x4 max storage yard
}
static bool IsStorageWidthTooHigh(TileArea area) {
	return (area.sizeX() / 2) > 16 || (area.sizeY() / 2) > 16;
}

enum class OverlayType
{
	None,
	Appeal,
	Fish, // Fish uses TreeSystem not overlay system
	Farm,
	Gatherer,
	Hunter,
	Forester,

	Windmill,
	IrrigationReservoir,
	Market,
	ShippingDepot,

	Beekeeper,

	//ConstructionOffice,
	Industrialist,
	Consulting,
	Human,

	Library,
	School,
	Bank,

	Granary,
	HaulingServices,
	
	Theatre,
	Tavern,
	//Zoo,
	//Museum,

	BadAppeal,

	Raid,
	Fort,
	RevealSpyNest,
};

static bool IsGridOverlay(OverlayType overlayEnum)
{
	switch(overlayEnum)
	{
	case OverlayType::Appeal:
	case OverlayType::Fish:
	case OverlayType::Farm:
	case OverlayType::IrrigationReservoir:
		return true;
	default: return false;
	}
}

static const int32 BattleInfluencePrice = 200;
static const int32 BattleClaimTicks = Time::TicksPerSeason;

#define LOCTEXT_NAMESPACE "InfluenceIncomeEnum"

enum class InfluenceIncomeEnum : uint8
{
	Townhall,
	Population,
	//Luxury,
	//Castle,
	DiplomaticBuildings,
	
	TerritoryUpkeep,
	UnsafeProvinceUpkeep,

	Fort,
	Colony,

	GainFromVassal,
	LoseToLord,
	
	Count,
};
static const TArray<FText> InfluenceIncomeEnumName
{
	LOCTEXT("Townhall", "Townhall"),
	LOCTEXT("Population", "Population"),
	//LOCTEXT("Luxury Consumption", "Luxury Consumption"),
	//LOCTEXT("Castle", "Castle"),
	LOCTEXT("Diplomatic Buildings", "Diplomatic Buildings"),
	
	LOCTEXT("Province Upkeep", "Province Upkeep"),
	LOCTEXT("Unsafe Province Upkeep", "Unsafe Province Upkeep"),

	LOCTEXT("Fort", "Fort"),
	LOCTEXT("Colony", "Colony"),

	LOCTEXT("Gain from Vassal", "Gain from Vassal"),
	LOCTEXT("Lose to Lord", "Lose to Lord"),
};
static int32 InfluenceIncomeEnumCount = static_cast<int32>(InfluenceIncomeEnum::Count);

static const FText& GetInfluenceIncomeName(InfluenceIncomeEnum influenceIncomeEnum) { return InfluenceIncomeEnumName[static_cast<int32>(influenceIncomeEnum)]; }

#undef LOCTEXT_NAMESPACE

enum class IncomeEnum : uint8
{
	// House income
	Base,
	Tech_MoreGoldPerHouse,
	Tech_HouseLvl6Income,
	Appeal,
	Luxury,

	Adjacency,
	Card_BeerTax,
	Card_DesertPilgrim,
	Bank,

	// HouseIncomeEnumCount!!!

	// Other income
	//TownhallIncome, // HouseIncomeEnumCount!!!
	BankProfit,
	ArchivesProfit,
	InvestmentProfit,
	SocialWelfare,

	FromVassalTax,
	ToLordTax,
	Others,
	BuildingUpkeep,
	ArmyUpkeep,
	
	TerritoryRevenue,
	TerritoryUpkeep, // Upkeep goes to influence unless that is 0...
	TradeRoute,
	
	EconomicTheories,

	Count,
};
#define LOCTEXT_NAMESPACE "IncomeEnumName"
static const TArray<FText> IncomeEnumName
{
	LOCTEXT("Base", "Base"),
	LOCTEXT("Tech More Gold per House", "Tech More Gold per House"),
	LOCTEXT("Tech House Lvl 6 Income", "Tech House Lvl 6 Income"),
	LOCTEXT("Appeal", "Appeal"),
	LOCTEXT("Luxury", "Luxury"),

	LOCTEXT("Adjacency", "Adjacency"),
	LOCTEXT("Card Beer Tax", "Card Beer Tax"),
	LOCTEXT("Card Desert Pilgrim", "Card Desert Pilgrim"),
	LOCTEXT("Bank", "Bank"),

	//LOCTEXT("Townhall Income", "Townhall Income"),
	LOCTEXT("Bank Profit", "Bank Profit"),
	LOCTEXT("Archives Income", "Archives Income"),
	LOCTEXT("Investment Profit", "Investment Profit"),
	LOCTEXT("Social Welfare", "Social Welfare"),
	
	LOCTEXT("Tax from Vassal", "Tax from Vassal"),
	LOCTEXT("Tax for Lord", "Tax for Lord"),
	LOCTEXT("Others", "Others"),
	LOCTEXT("Building Upkeep", "Building Upkeep"),
	LOCTEXT("Military Upkeep", "Military Upkeep"),
	
	LOCTEXT("Territory Income", "Territory Income"),
	LOCTEXT("Territory Upkeep", "Territory Upkeep"),
	LOCTEXT("Trade Route", "Trade Route"),
	
	LOCTEXT("Economic Theories", "Economic Theories"),

	LOCTEXT("Count", "Count"),
};
#undef LOCTEXT_NAMESPACE

static const int32 HouseIncomeEnumCount = static_cast<int32>(IncomeEnum::BankProfit);

static int32 IncomeEnumCount = static_cast<int32>(IncomeEnum::Count);


static FText GetIncomeEnumName(int32 incomeEnumInt) {
	return IncomeEnumName[static_cast<int>(incomeEnumInt)];
}


inline bool IsHouseIncomeEnumInt(int32 incomeEnumInt) {
	return incomeEnumInt < HouseIncomeEnumCount;
}

inline bool IsHouseIncomeEnum(IncomeEnum incomeEnum) {
	return static_cast<int32>(incomeEnum) < HouseIncomeEnumCount;
}

enum class ScienceEnum : uint8
{
	Base,
	Luxury,
	HomeBrew,

	KnowledgeTransfer,
	ScientificTheories,

	Rationalism,
	FreeThoughts,
	EuropeBonus,

	// ScienceModifiers Below
	Library,
	School,
	BookWorm,

	Count,
};
#define LOCTEXT_NAMESPACE "ScienceEnumName"
static TArray<FText> ScienceEnumNameList
{
	LOCTEXT("HouseBase", "House Base(Food)"),
	LOCTEXT("HouseLuxury", "House Luxury"),
	LOCTEXT("HomeBrew", "Home Brew"),

	LOCTEXT("Knowledgetransfer", "Knowledge transfer"),
	LOCTEXT("ScientificTheories", "Scientific Theories"),

	LOCTEXT("Rationalism", "Rationalism"),
	LOCTEXT("Free Thoughts", "Free Thoughts"),
	LOCTEXT("Faction Bonus", "Faction Bonus"),

	LOCTEXT("Library", "Library"),
	LOCTEXT("School", "School"),
	LOCTEXT("Book Worm", "Book Worm"),
};
#undef LOCTEXT_NAMESPACE

static const FText& ScienceEnumName(int32 index) {
	return ScienceEnumNameList[index];
}
static int32 ScienceEnumCount = static_cast<int32>(ScienceEnum::Count);

static std::vector<ScienceEnum> HouseScienceEnums
{
	ScienceEnum::Base,
	ScienceEnum::Luxury,
	ScienceEnum::HomeBrew,
};

static std::vector<ScienceEnum> HouseScienceModifierEnums
{
	ScienceEnum::Library,
	ScienceEnum::School,
	ScienceEnum::BookWorm,
};


/**
 * Research
 */

enum class TechEnum : uint8
{
	None,

	DeepMining,
	Ironworks,

	RerollCardsPlus1,

	Fence,
	HouseLvl6Income,
	TaxAdjustment,

	TradingPost, // With immigrationBoost

	FireStarter,

	CityToCityTrade,
	InfluencePoints,
	Conquer,
	Vassalize,
	HomeLandDefense,
	
	/*
	 * Building techs
	 */
	GoldSmelting,
	GoldWorking,
	//TermiteFarm,

	Blacksmith,

	QuarryImprovement,
	CharcoalBurnerImprovement,
	
	StoneRoad,
	Garden,
	BlossomShrine,

	Chocolatier,

	HerbFarming,
	PotatoFarming,
	BlueberryFarming,
	MelonFarming,
	PumpkinFarming,
	
	Plantation,
	
	StoneTools,
	Medicine,
	
	IronStatue,
	Bank,
	TempleGrograth,

	CharcoalMaker,
	BeerBrewery,
	Pottery,
	BrickMaking,

	BorealLandCost,
	DesertTrade,
	
	ChocolateSnob,
	DeepWaterEmbark,

	RanchSheep,
	RanchCow,
	Winery,

	Baking,
	JewelryCrafting,

	Beekeeper,
	CandleMaker,
	CottonMilling,
	Printing,

	CardMaker,
	ImmigrationOffice,

	TrapSpike,
	Tailor,
	ShrineRot,
	CropStudy,
	FurnitureWorkshop,
	PaperMaker,
	Library, // With faster sci
	School,
	
	Theatre,
	
	Forester,

	FarmingTechnologies,
	RanchingTechnologies,
	HeatingTechnologies,
	ForestryTechnologies,
	IndustrialTechnologies,
	TradeRelations,
	HighFashion,
	
	Espionage,
	SpyGuard,

	//
	ShroomFarm,
	VodkaDistillery,
	CoffeeRoaster,

	// Mar 12
	Logistics3,
	AgriculturalRevolution,
	Archives,
	Logistics1,
	QuickBuild,

	// Apr 1
	SandMine,
	//GlassSmelting,
	Glassworks,
	ConcreteFactory,
	Industrialization,
	Electricity,
	
	StoneToolsShop,
	Petroleum,
	OilPowerPlant_Remove,
	PaperMill,
	ClockMakers,

	Cathedral,
	Castle,
	GrandPalace,
	ExhibitionHall,

	GreatMosque,

	SocialScience,

	// CardGiving
	Wheat,
	Cabbage,
	
	Frugality,
	Productivity,
	Sustainability,
	Motivation,
	Passion,

	ChimneyRestrictor,
	HomeBrew,
	BeerTax,

	SmelterCombo,
	MiningEquipment,
	FarmWaterManagement,
	CoalPipeline,
	CoalTreatment,

	Lockdown,
	SlaveLabor,
	DesertPilgrim,
	SocialWelfare,

	Conglomerate,
	TO_CHANGE_WineSnob,
	BlingBling,
	BookWorm,
	BirthControl,
	HappyBreadDay,
	AllYouCanEat,

	DepartmentOfAgriculture,
	EngineeringOffice,
	ArchitectsStudio,

	//Bridge,
	HumanitarianAid,

	TradingCompany,

	BarrackArcher,
	BarrackKnight,

	Irrigation,
	Logistics2,
	Logistics4,

	Machinery,
	Colony,
	//PortColony,

	IndustrialAdjacency,

	/*
	 * Bonuses
	 */

	MushroomSubstrateSterilization,
	Sawmill,
	ImprovedWoodCutting,
	ImprovedWoodCutting2,
	CheapReroll,
	CheapLand,

	CropBreeding,

	MoreGoldPerHouse,

	WinerySnob,

	TraderDiscount,

	//ResearchSpeed,
	//ClaimLandByFood,

	HouseAdjacency,

	Logistics5,
	BudgetAdjustment,
	WorkSchedule,

	CityManagementI,

	ScientificTheories,
	EconomicTheories,

	Fertilizers,
	MilitaryLastEra,

	MiddleAge,
	EnlightenmentAge,
	IndustrialAge,

	/*
	 * Prosperity
	 */
	FlowerBed,
	GardenShrubbery1,
	GardenCypress,

	Fort,
	ResourceOutpost,
	ResearchLab,
	//IntercityRoad,

	Combo,

	/*
	 * August 4
	 */
	CardInventory1,
	CardInventory2,

	//! Military
	Warrior,
	Archer,
	Swordsman,

	MilitaryEngineering1,
	MilitaryEngineering2,

	Knight,
	Tank,

	Conscription,
	
	Musketeer,
	Infantry,
	
	MachineGun,
	Artillery,
	Battleship,
	

	TradeRoute,
	ForeignRelation,
	PolicyMaking,
	ForeignInvestment,
	Tourism,
	Museum,
	Zoo,

	SpyCenter,
	CardCombiner,
	MarketInfluence,

	//! Faction
	SpiceFarming,
	CarpetWeaver,
	CarpetTrade,
	
	Count,
};

// TODO: remove this for bool?
enum class TechStateEnum : uint8
{
	Researched,
	Researching,
	Available,
	Locked, // was using isLocked instead...
};

enum class ClaimConnectionEnum : uint8
{
	None,
	Flat,
	River,
	ShallowWater,
	Deepwater,
};

void AppendClaimConnectionString(TArray<FText>& args, ClaimConnectionEnum claimConnectionEnum);

static bool IsMilitaryTechEnum(TechEnum techEnum)
{
	return static_cast<int>(TechEnum::Warrior) <= static_cast<int>(techEnum) && static_cast<int>(techEnum) <= static_cast<int>(TechEnum::Battleship);
}


/*
 * Quest
 */

enum class QuestEnum : uint8
{
	None,
	//ChooseLocationQuest,
	GatherMarkQuest,
	FoodBuildingQuest,
	ClaimLandQuest,
	
	BuildHousesQuest, // Probably not a good idea, people are already building houses?
	HouseUpgradeQuest,
	
	PopulationQuest,

	BuildStorageQuest,
	SurviveWinterQuest,

	// Optional Quests
	CooperativeFishingQuest,
	BeerQuest,
	PotteryQuest,
	
	TradeQuest,

	//
	TownhallUpgradeQuest,
};

static bool IsImportantQuest(QuestEnum questEnum)
{
	switch (questEnum)
	{
	//case QuestEnum::ChooseLocationQuest:
	case QuestEnum::GatherMarkQuest:
	case QuestEnum::FoodBuildingQuest:
	case QuestEnum::ClaimLandQuest:

	case QuestEnum::BuildHousesQuest:
	case QuestEnum::HouseUpgradeQuest:
	case QuestEnum::PopulationQuest:

	case QuestEnum::SurviveWinterQuest:

	case QuestEnum::TownhallUpgradeQuest:

		return true;
	default:
		return false;
	}
}

enum class PolicyMenuStateEnum
{
	None,
	Spring,
	Autumn,
};

//! Terrains

enum class BiomeEnum
{
	Forest,
	GrassLand,
	Desert,
	Jungle,
	Savanna,
	BorealForest,
	Tundra,
};

static bool IsGrassDominant(BiomeEnum biomeEnum) {
	return biomeEnum == BiomeEnum::Savanna || biomeEnum == BiomeEnum::GrassLand;
}

enum class UnitEnum : uint8
{
	Human,
	WildMan,
	
	Boar,
	RedDeer,
	YellowDeer,
	DarkDeer,

	BrownBear,
	BlackBear,
	Panda,

	Moose,
	Hippo,
	Penguin,
	Bobcat,

	Alpaca,
	Pig,
	Sheep,
	Cow,

	Infantry,
	ProjectileArrow,

	HorseCaravan,
	HorseMarket,
	HorseLogistics,
	SmallShip,
};

static bool IsHumanAnimatedDisplay(UnitEnum unitEnum)
{
	return unitEnum == UnitEnum::Human ||
		unitEnum == UnitEnum::HorseCaravan ||
		unitEnum == UnitEnum::HorseMarket ||
		unitEnum == UnitEnum::HorseLogistics;
}

static int32 GetAnimalBaseCost(UnitEnum unitEnum)
{
	switch (unitEnum) {
	case UnitEnum::Boar:
		return 200;
	case UnitEnum::RedDeer:
	case UnitEnum::YellowDeer:
	case UnitEnum::DarkDeer:
		return 300;
	case UnitEnum::BrownBear:
	case UnitEnum::BlackBear:
	case UnitEnum::Panda:
		return 1000;
	case UnitEnum::Bobcat:
		return 700;
	case UnitEnum::WildMan:
		return 1000;
	default:
		return 200;
	}
}



struct BiomeInfo
{
	FText name;
	FText description;
	std::vector<TileObjEnum> trees;
	std::vector<TileObjEnum> plants;
	std::vector<TileObjEnum> rarePlants;

	std::vector<UnitEnum> animals;
	std::vector<UnitEnum> rareAnimals;
	
	int32 initialBushChance = 10;

	TileObjEnum GetRandomTreeEnum() const {
		return trees[GameRand::Rand() % trees.size()];
	}
	TileObjEnum GetRandomPlantEnum() const {
		return plants[GameRand::Rand() % plants.size()];
	}

	bool HasRarePlant() { return rarePlants.size() > 0; }
	TileObjEnum GetRandomRarePlantEnum() {
		return rarePlants[GameRand::Rand() % rarePlants.size()];
	}

	std::string nameStr() const { return ToStdString(name.ToString()); }
	
	std::string GetDisplayNameWithoutSpace() const {
		std::string result = ToStdString(*FTextInspector::GetSourceString(name));
		result.erase(std::remove(result.begin(), result.end(), ' '), result.end());
		return result;
	}
};

#define LOCTEXT_NAMESPACE "BiomeInfo"

static const BiomeInfo BiomeInfos[]
{
	{ LOCTEXT("Forest", "Forest"),
		LOCTEXT("Forest Desc", "Serene broadleaf forest with moderate temperature. Its friendly conditions make it an ideal starting area for new players."),
		{ TileObjEnum::Orange, TileObjEnum::Birch },
		{TileObjEnum::OreganoBush, TileObjEnum::CommonBush, TileObjEnum::CommonBush2},
		{TileObjEnum::WhiteFlowerBush},
		{ UnitEnum::RedDeer, UnitEnum::Boar },
		{ UnitEnum::BlackBear, UnitEnum::BrownBear }
	},
	{LOCTEXT("Grassland", "Grassland"),
		LOCTEXT("Grassland Desc", "Biome filled with grasses, flowers and herbs. Although erratic precipitation makes the land unsuitable for farming, the dense grass makes this biome ideal for ranching."),
		{ TileObjEnum::Orange, TileObjEnum::Birch },
		{TileObjEnum::GrassGreen},
		{TileObjEnum::WhiteFlowerBush},
		{ UnitEnum::RedDeer, UnitEnum::Boar },
		{},
		5
	},
	{ LOCTEXT("Desert", "Desert"),
		LOCTEXT("Desert Desc", "Dry barren land with just sand and stone, but rich with mineral deposits."),
		{ TileObjEnum::DesertDatePalm,  TileObjEnum::DesertGingerbreadTree  },
		{ TileObjEnum::DesertBush1, TileObjEnum::DesertBush2 },
		{TileObjEnum::WhiteFlowerBush},
		{},
		{}
	},
	
	{ LOCTEXT("Jungle", "Jungle"),
		LOCTEXT("Jungle Desc", "Wet tropical jungle thick with trees and dense underbrush, teeming with life, and infested with disease."),
		{ TileObjEnum::Papaya, TileObjEnum::Cyathea, TileObjEnum::ZamiaDrosi },
		{ TileObjEnum::Fern, TileObjEnum::JungleThickLeaf },
		{TileObjEnum::WhiteFlowerBush},
		{UnitEnum::DarkDeer, UnitEnum::Boar },
		{ UnitEnum::BlackBear},
		5
	},
	{ LOCTEXT("Savanna", "Savanna"),
		LOCTEXT("Savanna Desc", "Tropical grassland scattered with shrubs and isolated trees. The rich ecosystem supports a wide variety of wildlife."),
		{ TileObjEnum::SavannaTree1  },
		{ TileObjEnum::GrassGreen },
		{},
		{UnitEnum::YellowDeer, UnitEnum::Boar },
		{},
		5
	},
	
	{ LOCTEXT("Boreal Forest", "Boreal Forest"),
		LOCTEXT("Boreal Forest Desc", "Coniferous forest with long and punishing winters."),
		{ TileObjEnum::Pine1, TileObjEnum::Pine2 },
		{ TileObjEnum::OreganoBush },
		{ TileObjEnum::WhiteFlowerBush},
		{ UnitEnum::RedDeer },
		{ UnitEnum::BrownBear},
	},
	{ LOCTEXT("Tundra", "Tundra"),
		LOCTEXT("Tundra Desc", "Extremely cold, frozen plain where almost nothing grows."),
		{ TileObjEnum::Pine1, TileObjEnum::Pine2 },
		{ }, // TileObjEnum::OreganoBush
		{TileObjEnum::WhiteFlowerBush},
		{},
		{}
	},
};

#undef LOCTEXT_NAMESPACE

// TODO: add plants
//  plants like cactus that can grow in very low fertility area, but very sparse (limited number per region, limited density)
static const BiomeInfo& GetBiomeInfo(BiomeEnum biomeEnum) {
	return BiomeInfos[static_cast<int>(biomeEnum)];
}

static const int32 MaxFertility = 100;
static const int32 TreeFertility = 45;
static const int32 TreeFertilityAtMaxPlantChance = 58;
static const int32 PlantFertility = 19;

static const int32 HardyTreeFertility = 15;

static const int32 OverlayFertilityMaxRed = TreeFertility - 10;

static const int32 RandomFertilityMinCutoffPercent = 50;

static const FloatDet StonePerlinCutoff = FD0_XX(35);
static const FloatDet TreePerlinMaxCutoff = FD0_XX(30);


static const FloatDet FlatLandHeight = FD0_XXX(125);
static const FloatDet BeachToWaterHeight = FlatLandHeight - FD0_XXX(005);
static const FloatDet MountainHeight = FlatLandHeight + FD0_XXX(020);
static const FloatDet DeepWaterHeight = FlatLandHeight - FD0_XXX(050);
static const FloatDet RiverMaxDepth = FlatLandHeight - FD0_XXX(015); // FD0_XXX(110);

static const float FlatLandHeightFloat = FDToFloat(FlatLandHeight);

// Note: Real world...
// Tundra latitude 70
// Desert latitude 15-35

// Temperature Band
const int32 tundraTemperatureStart100 = 60;
const int32 borealTemperatureStart100 = 40;
const int32 forestTemperatureStart100 = 7;

// Temperate Rainfall Band
const int32 forestRainfallStart100 = 50;
const int32 grasslandRainfallStart100 = 20;

// Equator Rainfall Band
const int32 jungleRainfallStart100 = 30;

// Taiga to Tundra Rainfall Band
const int32 taigaStartRainfall100 = 45;


const int32 tundraTemperatureStart10000 = tundraTemperatureStart100 * 100;
const int32 borealTemperatureStart10000 = borealTemperatureStart100 * 100;
const int32 forestTemperatureStart10000 = forestTemperatureStart100 * 100;


const int32 maxCelsiusDivider = 2;

/*
 * Units
 */

 // Unit's constants
static const int32_t UnitFoodFetchPerYear = 5;

#define LOCTEXT_NAMESPACE "UnitInfoName"

struct UnitInfo
{
	UnitEnum unitEnum;
	// Need std::string since we need to copy when passing into FName (allows using name.c_str() instead of FName in some cases)
	FText name;
	//int32_t adultTicks;
	int32 maxAgeTicks;
	int32 minBreedingAgeTicks;
	int32 gestationTicks;

	int32 maxHeatCelsiusTicks;

	// from foodResourcePerYear
	int32 maxFoodTicks;
	int32 foodPerFetch;
	int32 foodTicksPerResource;

	std::vector<ResourcePair> resourceDrops100;

	std::vector<BiomeEnum> biomes;

	int32 miniAgeTicks() { return maxAgeTicks / 8; }

	std::string nameStr() { return ToStdString(name.ToString()); }

	// 1 season = 3 minutes = 180 secs = 180 turns ... eating could take ~20% of the time ... 20 tiles to get to food = ~45 sec for food...

	// year100 is year * 100
	UnitInfo(
		UnitEnum unitEnum,
		FText name,
		//int32_t adult_Years100,
		int32 maxAge_Years100,
		int32 minBreedingAge_Years100,
		int32 gestation_Years100,
		int32 winterSurvivalLength_Years100,
		int32 foodResourcePerYear,
		std::vector<ResourcePair> resourceDrops100In
	) :
		unitEnum(unitEnum),
		name(name)
	{
		//adultTicks =			Time::TicksPerYear * adult_Years100 / 100;
		maxAgeTicks = Time::TicksPerYear * maxAge_Years100 / 100;
		minBreedingAgeTicks = Time::TicksPerYear * minBreedingAge_Years100 / 100;
		gestationTicks = Time::TicksPerYear * gestation_Years100 / 100;

		// Average winter is -5 C, 
		// ppl get heat damage below 5 C
		// So 10 Celsius damage average throughout winter.. so to survive
		maxHeatCelsiusTicks = winterSurvivalLength_Years100 * 10 * Time::TicksPerYear / 100;

		// Food
		// ppl must eat 5 times a year (slightly less than a season so they can winter starve)... maxFood = 2 * one consumption
		int32 foodTicksPerFetch = Time::TicksPerYear / UnitFoodFetchPerYear;
		
		foodPerFetch = foodResourcePerYear / UnitFoodFetchPerYear * 2; // How much food do human take at once

		maxFoodTicks = foodTicksPerFetch * (unitEnum == UnitEnum::Human ? 3 : 5); // Fragile humans ... Animals has loads of maxFoodTicks
		
		foodTicksPerResource = Time::TicksPerYear / foodResourcePerYear;

		resourceDrops100 = resourceDrops100In;

		// Calculate biomes for animals using BiomeInfo
		for (size_t i = 0; i < _countof(BiomeInfos); i++) {
			for (UnitEnum animalEnum : BiomeInfos[i].animals) {
				if (animalEnum == unitEnum) {
					biomes.push_back(static_cast<BiomeEnum>(i));
				}
			}
		}
	}

	bool IsAgingUnit() { return maxAgeTicks > 0; }
};

/*
 *
 * Note: Hunting production tied into the HumanFoodCostPerYear through AssumedFoodProduction100PerYear
 */
static const int32 AssumedHuntUnitPerYear = 5; // 6; // at BaseUnitDrop 5... hunters produce 60 per year ... or 12 Hunt Unit...
static const int32 BaseUnitDrop100 = AssumedFoodProduction100PerYear / AssumedHuntUnitPerYear;

static const int32 AnimalFoodPerYear = 300;

static const int32 UsualAnimalAge = 200;
static const int32 AnimalMinBreedingAge = 70;

static const int32 AnimalGestation = 020;

static const int32 Ranch_UsualAnimalAge = UsualAnimalAge * 100 / 150;
static const int32 Ranch_AnimalMinBreedingAge = AnimalMinBreedingAge * 100 / 200;
static const int32 Ranch_AnimalGestation = AnimalGestation * 100 / 200;


static const UnitInfo UnitInfos[]
{
	// Human last half winter without heat...
	// average winter temperature is -5

	//	adultYears100, maxAgeYears100, minBreedingAgeYears100,
	//	gestationYears100, winterSurvivalLength_Years100, foodPerYear
	UnitInfo(UnitEnum::Human,	LOCTEXT("Human", "Human"),	1000,	100,		025,	020,	HumanFoodPerYear, {{ResourceEnum::GameMeat, 2 * BaseUnitDrop100}}),
	UnitInfo(UnitEnum::WildMan, LOCTEXT("WildMan", "Wild Man"),	UsualAnimalAge,	AnimalMinBreedingAge,		AnimalGestation,	100,	AnimalFoodPerYear, {{ResourceEnum::GameMeat, 2 * BaseUnitDrop100}, {ResourceEnum::Leather, BaseUnitDrop100}}),

	UnitInfo(UnitEnum::Boar,	LOCTEXT("Boar", "Boar"),	UsualAnimalAge,	AnimalMinBreedingAge,		AnimalGestation,	100,	AnimalFoodPerYear, {{ResourceEnum::GameMeat, 2 * BaseUnitDrop100}, {ResourceEnum::Leather, BaseUnitDrop100}}),
	UnitInfo(UnitEnum::RedDeer,	LOCTEXT("Red Deer", "Red Deer"),	UsualAnimalAge,	AnimalMinBreedingAge,		AnimalGestation,	100,	AnimalFoodPerYear, {{ResourceEnum::GameMeat, 2 * BaseUnitDrop100}, {ResourceEnum::Leather, BaseUnitDrop100}}),
	UnitInfo(UnitEnum::YellowDeer, LOCTEXT("Mule Deer", "Mule Deer"),	UsualAnimalAge,	AnimalMinBreedingAge,		AnimalGestation,	100,	AnimalFoodPerYear, {{ResourceEnum::GameMeat, 2 * BaseUnitDrop100}, {ResourceEnum::Leather, BaseUnitDrop100}}),
	UnitInfo(UnitEnum::DarkDeer, LOCTEXT("Sambar Deer", "Sambar Deer"),	UsualAnimalAge,	AnimalMinBreedingAge,		AnimalGestation,	100,	AnimalFoodPerYear, {{ResourceEnum::GameMeat, 2 * BaseUnitDrop100}, {ResourceEnum::Leather, BaseUnitDrop100}}),

	UnitInfo(UnitEnum::BrownBear, LOCTEXT("Brown Bear", "Brown Bear"),	UsualAnimalAge,	AnimalMinBreedingAge,		AnimalGestation,	100,	AnimalFoodPerYear, {{ResourceEnum::GameMeat, 2 * BaseUnitDrop100}, {ResourceEnum::Leather, BaseUnitDrop100}}),
	UnitInfo(UnitEnum::BlackBear, LOCTEXT("Black Bear", "Black Bear"),	UsualAnimalAge,	AnimalMinBreedingAge,		AnimalGestation,	100,	AnimalFoodPerYear, {{ResourceEnum::GameMeat, 2 * BaseUnitDrop100}, {ResourceEnum::Leather, BaseUnitDrop100}}),
	UnitInfo(UnitEnum::Panda, LOCTEXT("Panda", "Panda"),	UsualAnimalAge,	AnimalMinBreedingAge,		AnimalGestation,	100,	AnimalFoodPerYear, {{ResourceEnum::GameMeat, 2 * BaseUnitDrop100}, {ResourceEnum::Leather, BaseUnitDrop100}}),

	UnitInfo(UnitEnum::Moose, LOCTEXT("Moose", "Moose"),	UsualAnimalAge,	AnimalMinBreedingAge,		AnimalGestation,	100,	AnimalFoodPerYear, {{ResourceEnum::GameMeat, 2 * BaseUnitDrop100}, {ResourceEnum::Leather, BaseUnitDrop100}}),
	UnitInfo(UnitEnum::Hippo, LOCTEXT("Hippo", "Hippo"),	UsualAnimalAge,	AnimalMinBreedingAge,		AnimalGestation,	100,	AnimalFoodPerYear, {{ResourceEnum::GameMeat, 2 * BaseUnitDrop100}, {ResourceEnum::Leather, BaseUnitDrop100}}),
	UnitInfo(UnitEnum::Penguin, LOCTEXT("Penguin", "Penguin"),	UsualAnimalAge,	AnimalMinBreedingAge,		AnimalGestation,	100,	AnimalFoodPerYear, {{ResourceEnum::GameMeat, 2 * BaseUnitDrop100}, {ResourceEnum::Leather, BaseUnitDrop100}}),
	UnitInfo(UnitEnum::Bobcat, LOCTEXT("Bobcat", "Bobcat"),	UsualAnimalAge,	AnimalMinBreedingAge,		AnimalGestation,	100,	AnimalFoodPerYear, {{ResourceEnum::GameMeat, 2 * BaseUnitDrop100}, {ResourceEnum::Leather, BaseUnitDrop100}}),

	UnitInfo(UnitEnum::Alpaca, LOCTEXT("Alpaca", "Alpaca"),	500,	100,		AnimalGestation,	100,	HumanFoodPerYear, {{ResourceEnum::Pork, 2 * BaseUnitDrop100}}),
	UnitInfo(UnitEnum::Pig, LOCTEXT("Pig", "Pig"),	Ranch_UsualAnimalAge,	Ranch_AnimalMinBreedingAge,		Ranch_AnimalGestation,	100,	HumanFoodPerYear, {{ResourceEnum::Pork, BaseUnitDrop100 * 400 / 100}}),
	UnitInfo(UnitEnum::Sheep, LOCTEXT("Sheep", "Sheep"),	Ranch_UsualAnimalAge,	Ranch_AnimalMinBreedingAge,		Ranch_AnimalGestation,	100,	HumanFoodPerYear, {{ResourceEnum::Lamb, BaseUnitDrop100 * 480 / 2 / 100}, {ResourceEnum::Wool, BaseUnitDrop100 * 480 / 2 / 100}}),
	UnitInfo(UnitEnum::Cow, LOCTEXT("Cow", "Cow"),	Ranch_UsualAnimalAge,	Ranch_AnimalMinBreedingAge,		Ranch_AnimalGestation,	100,	HumanFoodPerYear, {{ResourceEnum::Beef, BaseUnitDrop100 * 576 / 2 / 100}, {ResourceEnum::Leather,  BaseUnitDrop100 * 576 / 2 / 100}}),

	UnitInfo(UnitEnum::Infantry, LOCTEXT("Infantry", "Infantry"),	0,	1,		1,	1,	1, {{ResourceEnum::Pork, 15}}),
	UnitInfo(UnitEnum::ProjectileArrow, LOCTEXT("ProjectileArrow", "ProjectileArrow"),	0,	1,		1,	1,	1, {{ResourceEnum::Pork, 15}}),

	UnitInfo(UnitEnum::HorseCaravan, LOCTEXT("Horse", "Horse"),	UsualAnimalAge,	AnimalMinBreedingAge,		AnimalGestation,	100,	AnimalFoodPerYear, {{ResourceEnum::GameMeat, 2 * BaseUnitDrop100}, {ResourceEnum::Leather, BaseUnitDrop100}}),
	UnitInfo(UnitEnum::HorseMarket, LOCTEXT("Horse", "Horse"),	UsualAnimalAge,	AnimalMinBreedingAge,		AnimalGestation,	100,	AnimalFoodPerYear, {{ResourceEnum::GameMeat, 2 * BaseUnitDrop100}, {ResourceEnum::Leather, BaseUnitDrop100}}),
	UnitInfo(UnitEnum::HorseLogistics, LOCTEXT("Horse", "Horse"),	UsualAnimalAge,	AnimalMinBreedingAge,		AnimalGestation,	100,	AnimalFoodPerYear, {{ResourceEnum::GameMeat, 2 * BaseUnitDrop100}, {ResourceEnum::Leather, BaseUnitDrop100}}),

	UnitInfo(UnitEnum::SmallShip, LOCTEXT("SmallShip", "SmallShip"),	0,	1,		1,	1,	1, {{ResourceEnum::Pork, 15}}),
	//UnitInfo("Bear",	050,	500,	200,		010,	050,	5),
};

#undef LOCTEXT_NAMESPACE

static const int32 UnitEnumCount = _countof(UnitInfos);

static UnitInfo GetUnitInfo(UnitEnum unitEnum) {
	return UnitInfos[static_cast<int>(unitEnum)];
}
static UnitInfo GetUnitInfo(int32_t unitEnumInt) {
	return UnitInfos[unitEnumInt];
}


/*
 * Unit Cards
 */

static UnitEnum GetAnimalUnitEnumFromCardEnum(CardEnum cardEnum)
{
	int32 shift = static_cast<int32>(cardEnum) - static_cast<int32>(CardEnum::Boar);
	return static_cast<UnitEnum>(static_cast<int32>(UnitEnum::Boar) + shift);
}
static CardEnum GetAnimalCardEnumFromUnitEnum(UnitEnum unitEnum)
{
	int32 shift = static_cast<int32>(unitEnum) - static_cast<int32>(UnitEnum::Boar);
	return static_cast<CardEnum>(static_cast<int32>(CardEnum::Boar) + shift);
}
static bool IsAnimalCard(CardEnum cardEnum) {
	return IsCardEnumBetween(cardEnum, CardEnum::Boar, CardEnum::Bobcat);
}

/*
 * Military
 */

static const CardEnum MilitaryCardEnumMin = CardEnum::Militia;
static const CardEnum MilitaryCardEnumMax = CardEnum::Battleship;

static bool IsLandMilitaryCardEnum(CardEnum cardEnum) {
	return IsCardEnumBetween(cardEnum, CardEnum::Militia, CardEnum::Artillery);
};

static bool IsConscriptMilitaryCardEnum(CardEnum cardEnum) {
	return IsCardEnumBetween(cardEnum, CardEnum::Militia, CardEnum::Conscript);
};
static bool IsInfantryMilitaryCardEnum(CardEnum cardEnum) {
	return IsCardEnumBetween(cardEnum, CardEnum::Warrior, CardEnum::Infantry);
};
static bool IsCavalryMilitaryCardEnum(CardEnum cardEnum) {
	return IsCardEnumBetween(cardEnum, CardEnum::Knight, CardEnum::Tank);
};
static bool IsRangedMilitaryCardEnum(CardEnum cardEnum) {
	return IsCardEnumBetween(cardEnum, CardEnum::Archer, CardEnum::MachineGun);
};
static bool IsArtilleryMilitaryCardEnum(CardEnum cardEnum) {
	return IsCardEnumBetween(cardEnum, CardEnum::Catapult, CardEnum::Artillery);
};

static bool IsNavyCardEnum(CardEnum cardEnum) {
	return IsCardEnumBetween(cardEnum, CardEnum::Galley, CardEnum::Battleship);
};
static bool IsMilitaryCardEnum(CardEnum cardEnum) {
	return IsCardEnumBetween(cardEnum, MilitaryCardEnumMin, MilitaryCardEnumMax);
};

static bool IsFrontlineCardEnum(CardEnum cardEnum) {
	return IsCardEnumBetween(cardEnum, CardEnum::Militia, CardEnum::Tank);
}



class MilitaryConstants
{
public:
	static const int32 RoundsToWinAtEqualStrength = 4;
	static const int32 SecondsPerAttack = 8;

	static const int32 BaseHP100 = 100000;
	static const int32 BaseAttack100 = BaseHP100 / (Time::SecondsPerRound * RoundsToWinAtEqualStrength / SecondsPerAttack);
	static const int32 BaseDefense100 = 100;
};

struct MilitaryCardInfo
{
	CardEnum cardEnum;
	int32 baseMoneyCost;

	ResourcePair resourceCost;
	
	int32 humanCost;

	int32 upkeep;

	int32 allCostCombined() { return humanCost + resourceCost.count * GetResourceCostWithMoney(resourceCost.resourceEnum); }
	
	int32 hp100;
	int32 defense100; // attack / def = damage
	int32 attack100;

	int32 strength() { return hp100 * defense100 / MilitaryConstants::BaseDefense100 * attack100 / MilitaryConstants::BaseAttack100; }
};

static MilitaryCardInfo CreateMilitaryInfo(CardEnum cardEnum, int32 era)
{
	MilitaryCardInfo result;
	result.cardEnum = cardEnum;

	const int32 MilitaryEraCostMultiplier = 150;

	auto setMilitaryCost = [&](int32 baseCost, ResourceEnum resourceEnum = ResourceEnum::Money)
	{
		result.baseMoneyCost = baseCost;

		int32 moneyCost = baseCost;
		for (int32 i = 1; i < era; i++) {
			moneyCost = moneyCost * MilitaryEraCostMultiplier / 100;
		}
		
		result.resourceCost = ResourcePair(resourceEnum, moneyCost / GetResourceCostWithMoney(resourceEnum));
	};
	

	if (IsConscriptMilitaryCardEnum(cardEnum)) {
		setMilitaryCost(200); // 675 era 4
	}
	else if (IsInfantryMilitaryCardEnum(cardEnum)) {
		setMilitaryCost(1000); // 3375 era 4
	}
	else if (cardEnum == CardEnum::Knight) {
		setMilitaryCost(2000, ResourceEnum::Iron);
	}
	else if (cardEnum == CardEnum::Tank) {
		setMilitaryCost(3000, ResourceEnum::Steel);
	}
	else if (IsRangedMilitaryCardEnum(cardEnum)) {
		setMilitaryCost(1000);
	}
	else if (IsArtilleryMilitaryCardEnum(cardEnum)) {
		setMilitaryCost(1500, cardEnum == CardEnum::Artillery ? ResourceEnum::Steel : ResourceEnum::Money);
	}
	else if (IsNavyCardEnum(cardEnum)) {
		setMilitaryCost(3000, cardEnum == CardEnum::Battleship ? ResourceEnum::Steel : ResourceEnum::Money);
	}
	

	// Human Cost
	result.humanCost = 3;
	if (cardEnum == CardEnum::Conscript) {
		result.humanCost = 10;
	}
	

	// x y z
	result.hp100 = MilitaryConstants::BaseHP100;
	result.attack100 = MilitaryConstants::BaseAttack100;
	result.defense100 = MilitaryConstants::BaseDefense100;

	const int32 MilitaryEraStatMultiplier = 120; // 1.7^(1/3)

	for (int32 i = 1; i < era; i++) {
		result.hp100 = result.hp100 * MilitaryEraStatMultiplier / 100;
		result.attack100 = result.attack100 * MilitaryEraStatMultiplier / 100;
		result.defense100 = result.defense100 * MilitaryEraStatMultiplier / 100;
	}
	

	if (IsConscriptMilitaryCardEnum(cardEnum)) {
		result.attack100 = result.attack100 * 50 / 100;
		result.hp100 = result.hp100 * 150 / 100;
		//result.hp100 = MilitaryConstants::BaseHP100 * (cardEnum == CardEnum::Conscript ? 100 : 50) / 100; // conscript meat shield...
	}
	else if (IsInfantryMilitaryCardEnum(cardEnum)) {
		result.hp100 = result.hp100 * 150 / 100;
	}
	else if (IsCavalryMilitaryCardEnum(cardEnum)) {
		result.attack100 = result.attack100 * 200 / 100; // 1500... high attack pay extra cost since it allows faster war end (225 -> 200)
	}
	else if (IsRangedMilitaryCardEnum(cardEnum)) {
		result.hp100 = result.hp100 * 50 / 100;
		result.attack100 = result.attack100 * 150 / 100;
	}
	else if (IsArtilleryMilitaryCardEnum(cardEnum)) {
		result.hp100 = result.hp100 * 20 / 100;
		result.attack100 = result.attack100 * 220 / 100; // 1500
	}
	else if (IsNavyCardEnum(cardEnum)) {
		result.hp100 = result.hp100 * 150 / 100;
		result.attack100 = result.attack100 * 150 / 100;
	}

	// HP rounding
	result.hp100 = result.hp100 / 10000 * 10000;

	check(result.hp100 > 0);
	check(result.defense100 > 0);
	check(result.attack100 > 0);

	// Upkeep
	int32 roundsToEqualUpfrontCost = 200;
	result.upkeep = result.allCostCombined() / roundsToEqualUpfrontCost + 5;
	
	return result;
}

static const std::vector<MilitaryCardInfo> MilitaryCardInfoBaseList
{
	CreateMilitaryInfo(CardEnum::Militia, 1),
	CreateMilitaryInfo(CardEnum::Conscript, 4),

	CreateMilitaryInfo(CardEnum::Warrior, 1),
	CreateMilitaryInfo(CardEnum::Swordsman, 2),
	CreateMilitaryInfo(CardEnum::Musketeer, 3),
	CreateMilitaryInfo(CardEnum::Infantry, 4),

	CreateMilitaryInfo(CardEnum::Knight, 2),
	CreateMilitaryInfo(CardEnum::Tank, 4),

	CreateMilitaryInfo(CardEnum::Archer, 2),
	CreateMilitaryInfo(CardEnum::MachineGun, 3),

	CreateMilitaryInfo(CardEnum::Catapult, 2),
	CreateMilitaryInfo(CardEnum::Cannon, 3),
	CreateMilitaryInfo(CardEnum::Artillery, 4),

	CreateMilitaryInfo(CardEnum::Galley, 2),
	CreateMilitaryInfo(CardEnum::Frigate, 3),
	CreateMilitaryInfo(CardEnum::Battleship, 4),
};

static MilitaryCardInfo GetMilitaryInfo(CardEnum cardEnum) {
	return MilitaryCardInfoBaseList[static_cast<int>(cardEnum) - static_cast<int>(CardEnum::Militia)];
}

static FText GetMilitaryInfoDescription(CardEnum cardEnum)
{
	MilitaryCardInfo militaryInfo = GetMilitaryInfo(cardEnum);
	return FText::Format(
		NSLOCTEXT("Military", "Military Description", "Cost: {0}{1} \nUpkeep: -{2}<img id=\"Coin\"/><space>HP: {3}\nDefense: {4}\nAttack: {5}"),
		TEXT_NUM(GetMilitaryInfo(cardEnum).resourceCost.count),
		ResourceNameT_WithSpaceIcons(GetMilitaryInfo(cardEnum).resourceCost.resourceEnum),
		TEXT_NUM(GetMilitaryInfo(cardEnum).upkeep),

		TEXT_NUM(militaryInfo.hp100 / 100),
		TEXT_100(militaryInfo.defense100),
		TEXT_100(militaryInfo.attack100)
	);
}

static int32 GetArmyStrength(const std::vector<CardStatus>& cards)
{
	int32 strength = 0;
	for (const CardStatus& card : cards) {
		MilitaryCardInfo militaryInfo = GetMilitaryInfo(card.cardEnum);
		strength += militaryInfo.strength() * std::max(0, card.stackSize - 1);
		strength += militaryInfo.strength() * card.cardStateValue2 / militaryInfo.hp100;
	}
	return strength;
}


/*
 * Spell Cards
 */
static std::vector<CardEnum> SpellCards
{
	CardEnum::FireStarter,
	//CardEnum::Steal,
	CardEnum::Snatch,
	CardEnum::Kidnap,
	CardEnum::Terrorism,
	CardEnum::Raid,
	CardEnum::SharingIsCaring,

	// Skill
	CardEnum::SpeedBoost,

	CardEnum::InstantBuild,
};
static int32 AreaSpellRadius(CardEnum cardEnum) {
	switch (cardEnum) {
	case CardEnum::FireStarter: return 3;
	default: return 1;
	}
}
inline bool IsSpellCard(CardEnum cardEnumIn) {
	for (const CardEnum& cardEnum : SpellCards) {
		if (cardEnum == cardEnumIn) {
			return true;
		}
	}
	if (IsAnimalCard(cardEnumIn)) {
		return true;
	}
	return false;
}


static int32_t UnitEnumIntFromName(FString name) {
	for (int i = 0; i < UnitEnumCount; i++) {
		if (name.Equals(UnitInfos[i].name.ToString())) {
			return i;
		}
	}

	PUN_NOENTRY();
	return -1;
}

static const int32 UnitBaseBirthChance = 1;
static const int32 HumanBaseBirthChance = 3;

// BALANCE
// Heat _Year100Per10Resource... 10 resources will go through 4 winters (but 4 ppl in a house, so just 1 winter, hence 025 or 0.25 year)
// Mar29 ... 3 -> 30ppl -> 50
static const int32 HumanHeatResourcePerYear = 10; // 5 (for most of Feb) // 10 = 025 * 5 / 2

// Assume area is just a box: (ColdCelsius - MinCelsius) * Time::TicksPerSeason
inline int32 MaxWinterCelsiusBelowComfort() { return std::max(0, FDToInt(Time::ColdCelsius() - Time::MinCelsiusBase())); }

// Per Winter same as per year...
// This value assumes winter is always max cold... in reality it is a sin curve... so we approximate the area (celsius tick
inline int32 HeatNeededPerWinter_CelsiusTicks_Estimate() { return MaxWinterCelsiusBelowComfort() * Time::TicksPerSeason / 2; }
inline int32 HeatPerResource_CelsiusTicks() { return HeatNeededPerWinter_CelsiusTicks_Estimate() / HumanHeatResourcePerYear; }

static const int32 FunPercentLossPerYear = 20;
static const int32 FunTicksAt100Percent = Time::TicksPerYear * 100 / FunPercentLossPerYear;

// Tested 40 ppl 400 wood needed...

static const UnitEnum WildAnimalNoHomeEnums[] =
{
	UnitEnum::Alpaca,
	UnitEnum::RedDeer,
	UnitEnum::YellowDeer,
	UnitEnum::DarkDeer,

	UnitEnum::BrownBear,
	UnitEnum::BlackBear,
	UnitEnum::Panda,

	UnitEnum::Hippo,
	UnitEnum::Penguin,
	UnitEnum::WildMan,
};

static const UnitEnum WildAnimalWithHomeEnums[] =
{
	UnitEnum::Boar,
};


static bool IsWildAnimalNoHome(UnitEnum unitEnum) {
	for (UnitEnum animalEnum : WildAnimalNoHomeEnums) {
		if (animalEnum == unitEnum) {
			return true;
		}
	}
	return false;
}

static bool IsWildAnimalWithHome(UnitEnum unitEnum) {
	for (UnitEnum animalEnum : WildAnimalWithHomeEnums) {
		if (animalEnum == unitEnum) {
			return true;
		}
	}
	return false;
}

static const UnitEnum WildAnimalWithColony[] = {
	UnitEnum::WildMan,
	UnitEnum::Hippo,
	UnitEnum::Penguin,
};
static bool IsWildAnimalWithColony(UnitEnum unitEnum) {
	for (UnitEnum animalEnum : WildAnimalWithColony) {
		if (animalEnum == unitEnum) {
			return true;
		}
	}
	return false;
}

static bool IsDomesticatedAnimal(UnitEnum unitEnum)
{
	switch (unitEnum) {
	case UnitEnum::Pig:
	case UnitEnum::Sheep:
	case UnitEnum::Cow:
		return true;
	default:
		return false;
	}
}

static bool IsWildAnimal(UnitEnum unitEnum) {
	return IsWildAnimalWithHome(unitEnum) || IsWildAnimalNoHome(unitEnum);
}

static bool IsAnimalWithHome(UnitEnum unitEnum) {
	return IsWildAnimalWithHome(unitEnum) || IsDomesticatedAnimal(unitEnum);
}

static bool IsAnimal(UnitEnum unitEnum) {
	return IsWildAnimal(unitEnum) || IsDomesticatedAnimal(unitEnum);
}

static bool IsProjectile(UnitEnum unitEnum) {
	switch (unitEnum) {
	case UnitEnum::ProjectileArrow: return true;
	default: return false;
	}
}

// Humans can pass through gates etc.
static bool IsIntelligentUnit(UnitEnum unitEnum) {
	switch (unitEnum) {
	case UnitEnum::Human:
	case UnitEnum::Infantry:
		return true;
	default: return false;
	}
}

/*
 * Zooom Transition
 */

#if TRAILER_MODE
const float WorldZoomTransition_Unit = 1200;
#else
const float WorldZoomTransition_Unit = 680; // 680
#endif
const float WorldZoomTransition_UnitSmall = 680;
const float WorldZoomTransition_UnitAnimate = 1500;
const float WorldZoomTransition_HumanWalkAnimate = 300;
const float WorldZoomTransition_HumanNoAnimate = 490;

UENUM()
enum class UnitAnimationEnum : uint8
{
	None,
	Wait,
	Walk,
	Build,
	
	ChopWood,
	StoneMining,
	FarmPlanting,

	HorseCaravan,
	Ship,
	ImmigrationCart,

	HorseMarket,
	HorseLogistics,
	HaulingCart,

	Rest,
	Invisible,
};

struct UnitAnimationInfo
{
	UnitAnimationEnum animationEnum;
	std::string name;
	float playrate;

	UnitAnimationInfo(UnitAnimationEnum animationEnumIn,
					std::string nameIn,
					float playrateIn)
	{
		animationEnum = animationEnumIn;
		name = nameIn;
		playrate = playrateIn;
	}
};

static const std::vector<UnitAnimationInfo> UnitAnimationInfos
{
	UnitAnimationInfo(UnitAnimationEnum::None, "None", 1.0f),
	UnitAnimationInfo(UnitAnimationEnum::Wait, "Wait", 1.0f),
	UnitAnimationInfo(UnitAnimationEnum::Walk, "Walk", 2.5f),
	UnitAnimationInfo(UnitAnimationEnum::Build, "Build", 1.183),

	UnitAnimationInfo(UnitAnimationEnum::ChopWood, "ChopWood", 2.0f),
	UnitAnimationInfo(UnitAnimationEnum::StoneMining, "StoneMining", 3.0f),
	UnitAnimationInfo(UnitAnimationEnum::FarmPlanting, "FarmPlanting", 1.0f),

	UnitAnimationInfo(UnitAnimationEnum::HorseCaravan, "Caravan", 7.0f),
	UnitAnimationInfo(UnitAnimationEnum::Ship, "Ship", 4.0f),
	UnitAnimationInfo(UnitAnimationEnum::ImmigrationCart, "Immigration", 1.0f),

	UnitAnimationInfo(UnitAnimationEnum::HorseMarket, "HorseMarket", 7.0f),
	UnitAnimationInfo(UnitAnimationEnum::HorseLogistics, "HorseLogistics", 7.0f),
	UnitAnimationInfo(UnitAnimationEnum::HaulingCart, "HaulingCart", 1.0f),

	UnitAnimationInfo(UnitAnimationEnum::Rest, "Rest", 1.0f),
	UnitAnimationInfo(UnitAnimationEnum::Invisible, "Invisible", 1.0f),
};

static const std::string& GetUnitAnimationName(UnitAnimationEnum animationEnum) {
	return UnitAnimationInfos[static_cast<int>(animationEnum)].name;
}
static const int32 UnitAnimationCount = UnitAnimationInfos.size();

static float GetUnitAnimationPlayRate(UnitAnimationEnum animationEnum) {
	return UnitAnimationInfos[static_cast<int>(animationEnum)].playrate;
}

static const std::vector<float> UnitAnimationPlayRate =
{
	1.0f,
	1.0f,
	2.5f, // Walk

//#if TRAILER_MODE
//	1.479f, // Build 1.183 for 0.5s
//	2.0833, // ChopWood 1.6666
//#else
	1.0f, // Build
	2.0f, // ChopWood
//#endif

	3.0f, // StoneMining
	1.0f, // FarmPlanting

	7.0f, // Caravan
	4.0f, // Ship
	1.0f, // Immigration

	7.0f, // HorseMarket
	7.0f, // HorseLogistics
	1.0f, // HaulingCart

	1.0f,
	1.0f, // Invisible
};




static bool IsHorseAnimation(UnitAnimationEnum animationEnum)
{
	return animationEnum == UnitAnimationEnum::HorseCaravan ||
		animationEnum == UnitAnimationEnum::HorseMarket ||
		animationEnum == UnitAnimationEnum::HorseLogistics;
}

static bool IsCitizenWalkAnimation(UnitAnimationEnum animationEnum)
{
	return animationEnum == UnitAnimationEnum::Walk ||
		animationEnum == UnitAnimationEnum::ImmigrationCart || 
		animationEnum == UnitAnimationEnum::HaulingCart;
}

static bool ShouldHumanUseVertexAnimation(UnitAnimationEnum animationEnum, int32 zoomDistance)
{	
	if (animationEnum != UnitAnimationEnum::Ship &&
		!IsHorseAnimation(animationEnum) &&
		animationEnum != UnitAnimationEnum::ImmigrationCart &&
		animationEnum != UnitAnimationEnum::HaulingCart)
	{
		// Beyond some zoomDistance, just use skel mesh regardless of animation type
		if (zoomDistance > WorldZoomTransition_HumanNoAnimate) {
			return true;
		}
		
		if (IsCitizenWalkAnimation(animationEnum)) {
			return true;
		}
	}
	return false;
}


struct UnitDisplayState
{
	UnitEnum unitEnum = UnitEnum::Alpaca;
	UnitAnimationEnum animationEnum = UnitAnimationEnum::Wait;
	int32 variationIndex = -1;

	bool isValid() { return variationIndex != -1; }
};

enum class HumanVariationEnum : uint8
{
	AdultMale,
	AdultFemale,
	ChildMale,
	ChildFemale,
	Caravan,
};

static HumanVariationEnum GetHumanVariationEnum(bool isChild, bool isMale, UnitAnimationEnum animationEnum)
{
	if (animationEnum == UnitAnimationEnum::HorseCaravan) {
		return HumanVariationEnum::Caravan;
	}
	if (isChild) {
		return isMale ? HumanVariationEnum::ChildMale : HumanVariationEnum::ChildFemale;
	}
	return isMale ? HumanVariationEnum::AdultMale : HumanVariationEnum::AdultFemale;
}

//static const UnitEnum SkelMeshUnits[] = {
//	UnitEnum::Human,
//	UnitEnum::WildMan,
//	UnitEnum::Hippo,
//	UnitEnum::Penguin,
//};
//static bool IsUsingSkeletalMesh(UnitEnum unitEnumIn, UnitAnimationEnum animationEnum, int32 zoomDistance) {
//	if (animationEnum == UnitAnimationEnum::Ship) {
//		return false;
//	}
//	
//	if (unitEnumIn == UnitEnum::Human) {
//		if (PunSettings::IsOn("UseFullSkelAnim")) {
//			return true;
//		}
//		// Walk animation should use Vertex Animation
//		if (IsCitizenWalkAnimation(animationEnum)) {
//			return true;
//			//return zoomDistance > WorldZoomTransition_HumanWalkAnimate;
//		}
//		return true;
//	}
//	
//	for (UnitEnum unitEnum : SkelMeshUnits) {
//		if (unitEnum == unitEnumIn) {
//			return true;
//		}
//	}
//	return false;
//}
//static bool IsUsingVertexAnimation(UnitEnum unitEnumIn) {
//	for (UnitEnum unitEnum : SkelMeshUnits) {
//		if (unitEnum == unitEnumIn) {
//			return false;
//		}
//	}
//	return true;
//}

/*
 * Colors
 */

// H .. max 360, S V.. max 1.0f
static FLinearColor MakeColorHSV(float H, float S, float V)
{
	const FLinearColor HSVColor(H, S, V);
	return HSVColor.HSVToLinearRGB();
}

static FLinearColor PlayerColor1(int32 playerId)
{
	static const FLinearColor arr[] = {
		FLinearColor(0.225, 0.00769, 0.972), // 1. Purple
		FLinearColor(0.082688, 0.972, 0.842026), // 2. Cyan
		FLinearColor(0.045003, 0.328125, 0.045003), // 3. Green
		FLinearColor(1, 0, 0), // 4. Red
		FLinearColor(.02, .09, 1), // 5. Blue
		FLinearColor(1, .943, 0), // 6. Yellow
		MakeColorHSV(0, 0.9, 0.7),
	};
	if (playerId >= _countof(arr)) {
		return FLinearColor(0.02, 0.02, 0.02, 1);
	}
	return arr[playerId];
}

static FLinearColor PlayerColor2(int32 playerId)
{
	static const FLinearColor arr[] = {
		FLinearColor(0.972, 0.875, 0), // 1. Yellow
		FLinearColor(0.972, 0.495342, 0.059063), // 2. Light Orange
		FLinearColor(0.225, 0.00769, 0), // 3. Dark Red
		FLinearColor(1, .9, 0), // 4. Yellow
		FLinearColor(.954, 1, 1), // 5. White
		FLinearColor(0, 0.105, 1), // 6. Blue
		MakeColorHSV(90, 0.8, 0.5),
	};
	if (playerId >= _countof(arr)) {
		return FLinearColor(0.02, 0.02, 0.02, 1);
	}
	return arr[playerId];
}

#define CHEAT_ENUM_LIST(entry) \
	entry(UnlockAll) \
	entry(Money) \
	entry(Influence) \
	entry(FastBuild) \
	entry(Resources) \
	entry(ConstructionResources) \
	entry(Undead) \
	entry(Immigration) \
	\
	entry(FastTech) \
	entry(ForceFunDown) \
	entry(ForceFoodDown) \
	entry(ForceSick) \
	entry(Kill) \
	entry(ClearLand) \
	entry(Unhappy) \
	\
	entry(AddHuman) \
	entry(AddAnimal) \
	entry(AddWildMan) \
	entry(KillUnit) \
	entry(SpawnDrop) \
	\
	entry(AddResource) \
	entry(AddCard) \
	entry(AddMoney) \
	entry(AddInfluence) \
	entry(AddImmigrants) \
	\
	entry(Cheat) \
	entry(Army) \
	entry(YearlyTrade) \
	\
	entry(HouseLevel) \
	entry(HouseLevelKey) \
	entry(FullFarmRoad) \
	\
	entry(NoCameraSnap) \
	entry(GeoresourceAnywhere) \
	entry(NoFarmSizeCap) \
	entry(MarkedTreesNoDisplay) \
	entry(WorkAnimate) \
	entry(DemolishNoDrop) \
	entry(God) \
	\
	entry(RemoveAllCards) \
	entry(BuyMap) \
	entry(TrailerCityGreen1) \
	entry(TrailerCityGreen2) \
	entry(TrailerCityBrown) \
	\
	entry(TrailerPauseForCamera) \
	entry(TrailerForceSnowStart) \
	entry(TrailerForceSnowStop) \
	entry(TrailerIncreaseAllHouseLevel) \
	entry(TrailerPlaceSpeed) \
	entry(TrailerHouseUpgradeSpeed) \
	entry(TrailerForceAutumn) \
	entry(TrailerBeatShiftBack) \
	entry(TrailerTimePerBeat) \
	entry(TrailerRoadPerTick) \
	\
	entry(AddAIImmigrants) \
	entry(AddAIMoney) \
	entry(AddAIInfluence) \
	\
	entry(TestCity) \
	entry(TestCityNetwork) \
	entry(DebugUI) \
	entry(PunTog) \
	entry(PunGet) \
	entry(PunSet) \
	entry(UnitInfo) \
	entry(BuildingInfo) \
	\
	entry(ScoreVictory) \
	entry(ResourceStatus) \
	entry(AddAllResources) \
	\
	entry(GetRoadConstructionCount) \
	entry(ResetCachedWaypoints) \
	entry(GetResourceTypeHolders)\
	\
	entry(SaveCameraTransform) \
	entry(LoadCameraTransform) \
	\
	entry(StepSimulation) \
	\
	entry(TestGetJson) \
	\
	entry(ClearLuxuryTier1) \
	\
	entry(AISpyNest) \
	\
	entry(TestAITradeDeal) \
	entry(TestAIForeignBuild)

#define CREATE_ENUM(name) name,
#define CREATE_STRINGS(name) #name,

//! Cheat
enum class CheatEnum : int32
{
	CHEAT_ENUM_LIST(CREATE_ENUM)
};
static const std::string CheatName[]
{
	CHEAT_ENUM_LIST(CREATE_STRINGS)
};

#undef CREATE_ENUM
#undef CREATE_STRINGS


static std::string GetCheatName(CheatEnum cheatEnum) {
	return CheatName[static_cast<int>(cheatEnum)];
}

static CheatEnum GetCheatEnum(const FString& cheatName)
{
	for (int i = 0; i < _countof(CheatName); i++) {
		if (ToFString(CheatName[i]).Equals(cheatName)) {
			return static_cast<CheatEnum>(i);
		}
	}
	UE_DEBUG_BREAK();
	return CheatEnum::UnlockAll;
}

enum class ObjectTypeEnum
{
	Building,
	Unit,
	TileObject,
	Georesource,
	StartingSpot,
	Drop,
	EmptyTile,

	Map,

	None,
	Dead,
};

enum class FarmStage : uint8 {
	Dormant,
	Seeding,
	Nourishing,
	Harvesting,
};


// For communicating to ObjectDescriptionUI
// allows simulation to despawn UI of object that no longer exists
struct DescriptionUIState
{
	ObjectTypeEnum objectType;
	int32 objectId;
	bool shouldCloseStatUI = true;

	int32 openedTick = -1;

	DescriptionUIState() : objectType(ObjectTypeEnum::None), objectId(-1) {}
	DescriptionUIState(ObjectTypeEnum objectType, int32_t objectId) : objectType(objectType), objectId(objectId) {}

	bool isValid() const {
		return objectType != ObjectTypeEnum::None && objectId != -1;
	}

	void TryRemove(ObjectTypeEnum objectTypeIn, int32_t objectIdIn) {
		if (objectType == objectTypeIn && objectId == objectIdIn) {
			objectType = ObjectTypeEnum::None;
			objectId = -1;
		}
	}

	bool operator!=(const DescriptionUIState& a) {
		return objectType != a.objectType || objectId != a.objectId;
	}
};

enum class DisplayClusterEnum
{
	Building,
	Unit,
	Trees,
	Resource,
	FarmPlant,
	Road,
	Overlay,
	TechTree,
	BuildingAnimation,
	Terrain,
	//IrrigationDitch,

	Count,
};

enum class DisplayGlobalEnum
{
	Province,
	Territory,
	MapDefenseNode,

	Count,
};

/*
 * Particles
 */

enum class ParticleEnum
{
	Smoke,
	HeavySteam,
	BlackSmoke,
	HeavyBlackSmoke,
	StoveFire,
	TorchFire,

	CampFire,
	BuildingFire,
	BuildingFireSmoke,

	DemolishDust, // End Particle

	OnDemolish,
	OnPlacement,
	OnTownhall,
	OnUpgrade,

	Count,
};

struct ParticleInfo
{
	ParticleEnum particleEnum = ParticleEnum::Smoke;
	FTransform transform;
};

struct DemolishDisplayInfo
{
	CardEnum buildingEnum = CardEnum::None;
	TileArea area;
	int32 tickDemolished = -1;

	int32 animationTicks() { return Time::Ticks() - tickDemolished; }

	bool isInUse() { return tickDemolished != -1; }
};

struct FireOnceParticleInfo
{
	ParticleEnum particleEnum = ParticleEnum::Smoke;
	TileArea area;
	int32 startTick = -1;
};


// Prevent flood of some actions. For example, event log shouldn't get flooded with "Food reserve low." text.
enum class NonRepeatActionEnum
{
	FoodReserveLowEvent,
	WoodReserveLowEvent,
	StorageFullEvent,
	NeedMoreToolsEvent,
	AnimalsEatingCrop,

	BabyBornBell,

	TreeChopping,
	StonePicking,

	// HomelessLeftEvent, TODO
	
	Count,
};

enum class PlayerCallOnceActionEnum : uint8
{
	ForeignBuiltSuccessfulPopup,
	LowTourismHappinessPopup,
	Count
};
static const int32 CallOnceEnumCount = static_cast<int32>(PlayerCallOnceActionEnum::Count);



// Flood
struct NonWalkableTileAccessInfo
{
	WorldTile2 tile;
	WorldTile2 nearbyTile;
	
	NonWalkableTileAccessInfo() : tile(WorldTile2::Invalid), nearbyTile(WorldTile2::Invalid) {}
	NonWalkableTileAccessInfo(WorldTile2 tile, WorldTile2 nearbyTile) : tile(tile), nearbyTile(nearbyTile) {}

	bool isValid() { return tile != WorldTile2::Invalid; }
};

static const NonWalkableTileAccessInfo NonWalkableTileAccessInfoInvalid(WorldTile2(-1, -1), WorldTile2(-1, -1));


// No init needed.. easy clear...
// Deterministic (no unordered_map), Fast... no unordered_map linked list...
struct PunStates
{
	int32_t& operator[](std::string stateName) {
		for (size_t i = 0; i < stateNames.size(); i++) {
			if (stateName == stateNames[i]) {
				return states[i];
			}
		}
		stateNames.push_back(stateName);
		states.push_back(0);
		return states.back();
	}

	int32_t get(std::string stateName) {
		for (size_t i = 0; i < stateNames.size(); i++) {
			if (stateName == stateNames[i]) {
				return states[i];
			}
		}
		//UE_DEBUG_BREAK();
		return 0;
	}

	void SetZeros() {
		std::fill(states.begin(), states.end(), 0);
	}

	int32 Sum() {
		int32_t sum = 0;
		for (size_t i = 0; i < states.size(); i++) {
			sum += states[i];
		}
		return sum;
	}

private:
	std::vector<std::string> stateNames;
	std::vector<int32_t> states;
};

//!

enum class GridConnectType {
	Opposite,
	Adjacent,
	Four,
	Three,
	End,
};

/*
 * UI
 */

enum class ExclusiveUIEnum : uint8
{
	CardHand1,
	CardInventory,
	RareCardHand,
	ConverterCardHand,

	BuildMenu,
	Placement,
	ConfirmingAction,
	EscMenu,

	Trading,
	IntercityTrading,
	TargetConfirm,

	TechTreeUI,
	QuestUI,
	//ObjectDescription, // shouldn't be one, to easy to misclick on this...

	StatisticsUI,
	PlayerOverviewUI,
	//ArmyMoveUI,

	InitialResourceUI,
	DiplomacyUI,
	TrainUnitsUI,
	TownAutoTradeUI,

	SendImmigrantsUI,
	GiftResourceUI,
	ProsperityUI,

	DeployMilitaryUI,
	CardSetsUI,

	Count,

	None,
};

static const int32 ExclusiveUIEnumCount = static_cast<int32>(ExclusiveUIEnum::Count);


enum class TutorialLinkEnum : uint8
{
	None,
	TutorialButton,
	Overview,
	CameraControl,
	Happiness,
};

static std::string TutorialLinkString(TutorialLinkEnum linkEnum)
{
	std::string str = std::to_string(static_cast<int32>(linkEnum));
	PUN_CHECK(1 <= str.size() && str.size() <= 2);
	if (str.size() == 1) {
		str = "0" + str;
	}
	return "<link>" + str;
}


/*
 * Popup
 */
enum class PopupReceiverEnum : uint8
{
	None,
	ImmigrationEvent,
	TribalJoinEvent,
	ImmigrationBetweenPlayersEvent,
	TradeRequest,
	DoneResearchBuyCardEvent,
	DoneResearchEvent_ShowTree,
	UnlockedHouseTree_ShowProsperityUI,

	//"PopulationQuest",
	//"HouseUpgradeQuest",
	
	Approve_Cannibalism,
	Approve_AbandonTown1,
	Approve_AbandonTown2,
	ChooseLocationDone,
	AllyRequest,
	CaravanBuyer,

	StartGame_Story,
	StartGame_AskAboutAdvice,

	ResetBuff,

	MaxCardHandQueuePopup,

	ChooseTownToClaimWith,

	EraPopup_MiddleAge,
	EraPopup_EnlightenmentAge,
	EraPopup_IndustrialAge,

	DoneResearchEvent_ShowAllTrees,

	ShowTradeDeal,

	RaidHandleDecision,
	RetreatConfirmDecision,
};

struct PopupInfo
{
	int32 playerId;
	FText body;
	
	std::vector<FText> choices;
	
	PopupReceiverEnum replyReceiver = PopupReceiverEnum::None;
	std::string popupSound;
	
	int32 replyVar1;
	int32 replyVar2;
	int32 replyVar3;
	int32 replyVar4;
	int32 replyVar5;
	int32 replyVar6;
	int32 replyVar7;
	int32 replyVar8;

	TArray<int32> array1;
	TArray<int32> array2;
	TArray<int32> array3;
	TArray<int32> array4;
	TArray<int32> array5;
	TArray<int32> array6;
	TArray<int32> array7;
	TArray<int32> array8;

	bool forcedNetworking = false;
	bool forcedSkipNetworking = false;
	
	ExclusiveUIEnum warningForExclusiveUI = ExclusiveUIEnum::None;
	int32 startTick = -1;
	int32 startDisplayTick = -1;

	PopupInfo() : playerId(-1) {}
	PopupInfo(int32 playerId, FText body, std::string popupSound = "") : playerId(playerId), body(body), popupSound(popupSound) {
		startTick = Time::Ticks();
	}

	PopupInfo(int32 playerId, FText body, std::vector<FText> choices, PopupReceiverEnum replyReceiver = PopupReceiverEnum::None,
		bool forcedNetworking = false, std::string popupSound = "", 
		int32 replyVar1 = -1, int32 replyVar2 = -1, int32 replyVar3 = -1
	)
		: playerId(playerId), body(body), choices(choices), replyReceiver(replyReceiver), popupSound(popupSound),
		replyVar1(replyVar1), replyVar2(replyVar2), replyVar3(replyVar3), forcedNetworking(forcedNetworking)
	{
		startTick = Time::Ticks();
	}

	PopupInfo(int32 playerId, TArray<FText> bodyArray, std::vector<FText> choices, PopupReceiverEnum replyReceiver = PopupReceiverEnum::None,
		bool forcedNetworking = false, std::string popupSound = "", int32 replyVar1 = -1)
		: playerId(playerId), body(JOINTEXT(bodyArray)), choices(choices), replyReceiver(replyReceiver), popupSound(popupSound), replyVar1(replyVar1), forcedNetworking(forcedNetworking)
	{
		startTick = Time::Ticks();
	}

	bool needNetworking()
	{
		if (replyReceiver == PopupReceiverEnum::None) {
			return false;
		}
		if (forcedSkipNetworking) {
			return false;
		}
		return choices.size() > 1 || forcedNetworking;
	}

	void operator>>(FArchive& Ar)
	{
		Ar << playerId;
		Ar << body;
		
		SerializeVecLoop(Ar, choices, [&](FText& text) {
			Ar << text;
		});
		
		Ar << replyReceiver;
		SerializeStr(Ar, popupSound);
		
		Ar << replyVar1;
		Ar << replyVar2;
		Ar << replyVar3;
		Ar << replyVar4;
		Ar << replyVar5;
		Ar << replyVar6;
		Ar << replyVar7;
		Ar << replyVar8;

		Ar << array1;
		Ar << array2;
		Ar << array3;
		Ar << array4;
		Ar << array5;
		Ar << array6;
		Ar << array7;
		Ar << array8;

		Ar << forcedNetworking;
		//Ar << forcedSkipNetworking;
		
		Ar << warningForExclusiveUI;
		Ar << startTick;
		Ar << startDisplayTick;
	}

	bool operator==(const PopupInfo& a) const
	{
		return playerId == a.playerId &&
			TextEquals(body, a.body) &&
			TextArrayEquals(choices, a.choices);
	}
};



enum class RareHandEnum : uint8
{
	InitialCards1,
	InitialCards2,
	RareCards,
	BuildingSlotCards,
	CratesCards, // Crates gives resource cards

	TownhallLvl2Cards,
	TownhallLvl3Cards,
	TownhallLvl4Cards,
	TownhallLvl5Cards,

	PopulationQuestCards1, // PopulationQuest cards drive people to upgrade their houses
	PopulationQuestCards2,
	PopulationQuestCards3,
	PopulationQuestCards4,
	PopulationQuestCards5,
	PopulationQuestCards6,
	PopulationQuestCards7,
	//PopulationQuestCards8,

	BorealCards,
	BorealCards2,
	DesertCards,
	DesertCards2,
	SavannaCards,
	SavannaCards2,
	JungleCards,
	JungleCards2,
	ForestCards,
	ForestCards2,

	Era2_1_Cards,
	Era2_2_Cards,
	Era3_1_Cards,
	Era3_2_Cards,
	Era4_1_Cards,
	Era4_2_Cards,

	Invalid,
};


class PermanentBonus
{
public:
	struct RareCardHand
	{
		RareHandEnum rareHandEnum;
		std::vector<CardEnum> cardEnums;
	};

	static const std::vector<RareCardHand> PermanentBonusRareCardHands;

	static std::vector<CardEnum> BonusHandEnumToCardEnums(RareHandEnum rareHandEnumIn)
	{
		for (const RareCardHand& rareHand : PermanentBonusRareCardHands) {
			if (rareHand.rareHandEnum == rareHandEnumIn) {
				return rareHand.cardEnums;
			}
		}
		return {};
	}
	static RareHandEnum CardEnumToBonusHandEnum(CardEnum cardEnumIn)
	{
		for (const RareCardHand& rareHand : PermanentBonusRareCardHands) {
			for (CardEnum cardEnum : rareHand.cardEnums) {
				if (cardEnum == cardEnumIn) {
					return rareHand.rareHandEnum;
				}
			}
		}
		return RareHandEnum::Invalid;
	}
};



static const int32 PopulationQuestTierThreshold[]
{
	0,
	25,
	50,
	100,
	150,
	200,
	300,
	500,
};
static int32 GetPopulationQuestTierThreshold(int32 tier) { return PopulationQuestTierThreshold[tier]; }


enum class FloatupEnum : uint8
{
	None,

	ComboComplete,
	BuildingComplete,
	HouseUpgrade,
	HouseDowngrade,
	TownhallUpgrade,
	
	GainResource,
	GainMoney,
	GainScience,
	GainInfluence,
};
struct FloatupInfo
{
	FloatupEnum floatupEnum = FloatupEnum::None;
	int32_t startTick = -1;
	WorldTile2 tile = WorldTile2::Invalid;

	FText text;
	ResourceEnum resourceEnum = ResourceEnum::None;

	FText text2;
	ResourceEnum resourceEnum2 = ResourceEnum::None;

	FloatupInfo() {}
	FloatupInfo(FloatupEnum floatupEnum, int32_t startTick, WorldTile2 tile, FText text = FText(),
				ResourceEnum resourceEnum = ResourceEnum::None, FText text2 = FText())
			: floatupEnum(floatupEnum), startTick(startTick), tile(tile), text(text), resourceEnum(resourceEnum), text2(text2)
	{}

	void operator>>(FArchive& Ar)
	{
		Ar << floatupEnum;
		Ar << startTick;

		Ar << text;
		Ar << resourceEnum;

		Ar << text2;
		Ar << resourceEnum2;
	}
};

#define LOCTEXT_NAMESPACE "MaleNames"

static const TArray<FText> MaleNames
{
	LOCTEXT("Pun", "Pun"),
	LOCTEXT("Pong", "Pong"),
	LOCTEXT("Pep", "Pep"),

	LOCTEXT("Omega", "Omega"),
	LOCTEXT("Dutelut", "Dutelut"),
	LOCTEXT("Grawen", "Grawen"),

	LOCTEXT("Earn", "Earn"),
	LOCTEXT("Nick", "Nick"),
	LOCTEXT("John", "John"),
	LOCTEXT("Joe", "Joe"),
	LOCTEXT("Jack", "Jack"),
	LOCTEXT("Jeff", "Jeff"),
	LOCTEXT("James", "James"),
	LOCTEXT("Alistair", "Alistair"),
	LOCTEXT("George", "George"),
	LOCTEXT("Sam", "Sam"),
	LOCTEXT("Clement", "Clement"),
	LOCTEXT("Tim", "Tim"),
	LOCTEXT("Edwin", "Edwin"),
	LOCTEXT("Elvis", "Elvis"),
	LOCTEXT("Elvis", "Elvis"),
	LOCTEXT("Stephen", "Stephen"),
	LOCTEXT("Albert", "Albert"),
	LOCTEXT("Kirk", "Kirk"),
	LOCTEXT("Todd", "Todd"),
	LOCTEXT("Howard", "Howard"),
	LOCTEXT("Wilson", "Wilson"),
	LOCTEXT("Geoffrey", "Geoffrey"),
	LOCTEXT("Joseph", "Joseph"),
	LOCTEXT("Abraham", "Abraham"),
	LOCTEXT("Lincoln", "Lincoln"),
	LOCTEXT("Shawn", "Shawn"),
	LOCTEXT("Dave", "Dave"),
	LOCTEXT("Danny", "Danny"),
	LOCTEXT("Diego", "Diego"),
	LOCTEXT("Donald", "Donald"),

	LOCTEXT("Elton", "Elton"),
	LOCTEXT("Chris", "Chris"),
	LOCTEXT("Kevin", "Kevin"),
	LOCTEXT("Martin", "Martin"),
	LOCTEXT("Fluffy", "Fluffy"),
	LOCTEXT("Robert", "Robert"),
	LOCTEXT("Marlon", "Marlon"),
	LOCTEXT("Denzel", "Denzel"),
	LOCTEXT("Arnold", "Arnold"),
	LOCTEXT("Daniel", "Daniel"),
	LOCTEXT("Sidney", "Sidney"),
	LOCTEXT("Clark", "Clark"),
	LOCTEXT("Tom", "Tom"),
	LOCTEXT("Trump", "Trump"),
	LOCTEXT("Greg", "Greg"),
	LOCTEXT("Kanye", "Kanye"),
	LOCTEXT("Leonardo", "Leonardo"),
	LOCTEXT("Spencer", "Spencer"),
	LOCTEXT("Bruce", "Bruce"),
	LOCTEXT("Henry", "Henry"),
	LOCTEXT("Morgan", "Morgan"),
	LOCTEXT("Johnny", "Johnny"),
	LOCTEXT("Charles", "Charles"),
	LOCTEXT("Kirk", "Kirk"),
	LOCTEXT("Will", "Will"),
	LOCTEXT("Harrison", "Harrison"),
	LOCTEXT("Gary", "Gary"),
	LOCTEXT("Gerard", "Gerard"),
	LOCTEXT("Justin", "Justin"),
	LOCTEXT("Dustin", "Dustin"),
	LOCTEXT("Samuel", "Samuel"),
	LOCTEXT("Robin", "Robin"),
	LOCTEXT("Don", "Don"),
	LOCTEXT("Eddie", "Eddie"),
	LOCTEXT("Anthony", "Anthony"),
	LOCTEXT("Lennon", "Lennon"),
	LOCTEXT("Tyler", "Tyler"),
	LOCTEXT("Steven", "Steven"),
	LOCTEXT("Solomon", "Solomon"),
	LOCTEXT("Willie", "Willie"),
	LOCTEXT("Frankie", "Frankie"),
	LOCTEXT("Ron", "Ron"),
	LOCTEXT("Harry", "Harry"),
	LOCTEXT("Bob", "Bob"),
	LOCTEXT("Marvin", "Marvin"),
	LOCTEXT("Richard", "Richard"),
	LOCTEXT("Freddie", "Freddie"),
	LOCTEXT("Marley", "Marley"),
	LOCTEXT("Chuck", "Chuck"),
	LOCTEXT("Bobby", "Bobby"),
	LOCTEXT("Kurt", "Kurt"),

	LOCTEXT("Victor", "Victor"),
	LOCTEXT("Vincent", "Vincent"),
	LOCTEXT("Bjorn", "Bjorn"),
	LOCTEXT("Ragnar", "Ragnar"),

	// Discord
	LOCTEXT("Maxo", "Maxo"),
	LOCTEXT("Noot", "Noot"),
	LOCTEXT("Biffa", "Biffa"),
	LOCTEXT("Nookrium", "Nookrium"),
	LOCTEXT("Dan", "Dan"),
	LOCTEXT("Zakh", "Zakh"),
	LOCTEXT("Escoces", "Escoces"),
	LOCTEXT("Kirill", "Kirill"),
	LOCTEXT("Rick", "Rick"),
	LOCTEXT("Indrik", "Indrik"),
	LOCTEXT("Boreale", "Boreale"),

	LOCTEXT("Lewis", "Lewis"),
	LOCTEXT("Simon", "Simon"),
	LOCTEXT("Sip", "Sip"),
	LOCTEXT("Splat", "Splat"),
	
	LOCTEXT("Jakob", "Jakob"),
	LOCTEXT("Jimba", "Jimba"),
	LOCTEXT("Rufio", "Rufio"),
	LOCTEXT("Tjelve", "Tjelve"),
	LOCTEXT("Kuro", "Kuro"),
	LOCTEXT("Ralinad", "Ralinad"),
	LOCTEXT("Shevy", "Shevy"),
	LOCTEXT("Coryn", "Coryn"),
	
};

#undef LOCTEXT_NAMESPACE


#define LOCTEXT_NAMESPACE "FemaleNames"

static const TArray<FText> FemaleNames
{
	LOCTEXT("Natthacha", "Natthacha"),
	
	LOCTEXT("Catherine", "Catherine"),
	LOCTEXT("Helen", "Helen"),
	LOCTEXT("Jeane", "Jeane"),
	LOCTEXT("Mary", "Mary"),
	LOCTEXT("Jane", "Jane"),
	LOCTEXT("Natasha", "Natasha"),
	LOCTEXT("Lucy", "Lucy"),
	LOCTEXT("Lyn", "Lyn"),
	LOCTEXT("Lidia", "Lidia"),
	LOCTEXT("Erica", "Erica"),
	LOCTEXT("Maile", "Maile"),
	LOCTEXT("Linda", "Linda"),
	LOCTEXT("Cheryl", "Cheryl"),
	LOCTEXT("Sheryl", "Sheryl"),
	LOCTEXT("Chan", "Chan"),
	LOCTEXT("Meghan", "Meghan"),
	LOCTEXT("Ava", "Ava"),
	LOCTEXT("Natalie", "Natalie"),
	LOCTEXT("Molly", "Molly"),

	LOCTEXT("Katherine", "Katherine"),
	LOCTEXT("Meryl", "Meryl"),
	LOCTEXT("Daniela", "Daniela"),
	LOCTEXT("Ingrid", "Ingrid"),
	LOCTEXT("Elizabeth", "Elizabeth"),
	LOCTEXT("Betty", "Betty"),
	LOCTEXT("Katy", "Katy"),
	LOCTEXT("Kylie", "Kylie"),
	LOCTEXT("Kate", "Kate"),
	LOCTEXT("Audrey", "Audrey"),
	LOCTEXT("Viola", "Viola"),
	LOCTEXT("Sophia", "Sophia"),
	LOCTEXT("Vivien", "Vivien"),
	LOCTEXT("Judy", "Judy"),
	LOCTEXT("Cindy", "Cindy"),
	LOCTEXT("Grace", "Grace"),
	LOCTEXT("Angela", "Angela"),
	LOCTEXT("Olivia", "Olivia"),
	LOCTEXT("Julie", "Julie"),
	LOCTEXT("Isabelle", "Isabelle"),
	LOCTEXT("Isabella", "Isabella"),
	LOCTEXT("Diane", "Diane"),
	LOCTEXT("Rita", "Rita"),
	LOCTEXT("Susan", "Susan"),
	LOCTEXT("Angelina", "Angelina"),
	LOCTEXT("Jolie", "Jolie"),
	LOCTEXT("Sandra", "Sandra"),
	LOCTEXT("Emma", "Emma"),
	LOCTEXT("Michelle", "Michelle"),
	LOCTEXT("Faye", "Faye"),
	LOCTEXT("Nicole", "Nicole"),
	LOCTEXT("Karen", "Karen"),
	LOCTEXT("Annie", "Annie"),
	LOCTEXT("Mariah", "Mariah"),
	LOCTEXT("Hermione", "Hermione"),
	LOCTEXT("Patsy", "Patsy"),

	LOCTEXT("Freya", "Freya"),
	LOCTEXT("Vivien", "Vivien"),
	LOCTEXT("Liza", "Liza"),
	
	// Discord
	LOCTEXT("Venti", "Venti"),
	LOCTEXT("Firis", "Firis"),
	LOCTEXT("Iris", "Iris"),
	LOCTEXT("Izavel", "Izavel"),
	LOCTEXT("Illia", "Illia"),
	LOCTEXT("Illya", "Illya"),
};

#undef LOCTEXT_NAMESPACE


// UnitId with aliveAndLifeCount for proper comparison countering reuse 
struct UnitFullId
{
	int32 id;
	int32 birthTicks; // TODO: remove this and use simpler system like birthTicks to reuse Units?

	UnitFullId() : id(-1), birthTicks(0) {}
	UnitFullId(int32 id, uint32 birthTicks) : id(id), birthTicks(birthTicks) {}

	bool isValid() { return id != -1; }

	static UnitFullId Invalid() { return UnitFullId(); }

	void operator>>(FArchive& Ar) {
		Ar << id << birthTicks;
	}

	std::string ToString() {
		return "[fid id:" + std::to_string(id) + " birth:" + std::to_string(birthTicks) + "]";
	}
};

class UnitInventory
{
public:
	void Add(ResourcePair resourcePair) {
		PUN_CHECK(resourcePair.count > 0);
		for (int i = _resourcePairs.size(); i-- > 0;) {
			if (_resourcePairs[i].resourceEnum == resourcePair.resourceEnum) {
				_resourcePairs[i] += resourcePair;
				return;
			}
		}
		_resourcePairs.push_back(resourcePair);
	}
	void Remove(ResourcePair resourcePair) {
		for (int i = _resourcePairs.size(); i-- > 0;) {
			if (_resourcePairs[i].resourceEnum == resourcePair.resourceEnum) {
				_resourcePairs[i] -= resourcePair;
				PUN_CHECK(_resourcePairs[i].count >= 0);

				if (_resourcePairs[i].count == 0) {
					_resourcePairs.erase(_resourcePairs.begin() + i);
				}
				return;
			}
		}
		UE_DEBUG_BREAK();
	}
	bool Has(ResourceEnum resourceEnum) {
		for (int i = _resourcePairs.size(); i-- > 0;) {
			if (_resourcePairs[i].resourceEnum == resourceEnum) {
				return true;
			}
		}
		return false;
	}
	int32_t Amount(ResourceEnum resourceEnum) {
		for (int i = _resourcePairs.size(); i-- > 0;) {
			if (_resourcePairs[i].resourceEnum == resourceEnum) {
				return _resourcePairs[i].count;
			}
		}
		return 0;
	}

	int32 Count() { return _resourcePairs.size(); }

	// Display largest...
	ResourceEnum Display() {
		ResourceEnum display = ResourceEnum::None;
		int largestAmount = 0;
		for (int i = _resourcePairs.size(); i-- > 0;) {
			if (_resourcePairs[i].count > largestAmount) {
				display = _resourcePairs[i].resourceEnum;
				largestAmount = _resourcePairs[i].count;
			}
		}
		return display;
	}

	std::string ToString() const {
		std::stringstream ss;
		ss << "\nInventory:\n";
		for (const ResourcePair& pair : _resourcePairs) {
			ss << " " << ResourceName(pair.resourceEnum) << " " << pair.count << "\n";
		}
		ss << "\n";
		return ss.str();
	}
	FText ToText() const;

	template <typename Func>
	void ForEachResource(Func func) {
		for (ResourcePair& pair : _resourcePairs) {
			func(pair);
		}
	}

	std::vector<ResourcePair>& resourcePairs() { return _resourcePairs; }
	bool hasResource() { return _resourcePairs.size() > 0; }

	int32_t resourceCountAll() const {
		int32_t sum = 0;
		for (int i = 0; i < _resourcePairs.size(); i++) {
			PUN_CHECK(_resourcePairs[i].count > 0);
			sum += _resourcePairs[i].count;
		}
		return sum;
	}

	template<typename Func>
	void Execute(Func func)
	{
		for (size_t i = 0; i < _resourcePairs.size(); i++) {
			PUN_CHECK(_resourcePairs[i].count > 0);
			func(_resourcePairs[i]);
		}
	}

	template<typename Func>
	void Execute_WithBreak(Func func)
	{
		for (size_t i = 0; i < _resourcePairs.size(); i++) {
			PUN_CHECK(_resourcePairs[i].count > 0);
			if (func(_resourcePairs[i])) {
				break;
			}
		}
	}

	void Clear() {
		_resourcePairs.clear();
	}

	bool CheckIngrity() {
#if WITH_EDITOR
		for (size_t i = 0; i < _resourcePairs.size(); i++) {
			if (_resourcePairs[i].count <= 0) {
				return false;
			}
		}
#endif
		return true;
	}

	//! Serialize
	FArchive& operator>>(FArchive &Ar) {
		SerializeVecObj(Ar, _resourcePairs);
		return Ar;
	}

private:
	std::vector<ResourcePair> _resourcePairs;
};

/*
 * Georesource
 */

enum class GeoresourceEnum : uint8
{
	Hotspring,
	GiantMushroom,
	CherryBlossom,
	
	//GiantTree,
	Ruin,

	IronOre,
	CoalOre,
	GoldOre,
	Gemstone,
	Oil,

	CannabisFarm,
	CocoaFarm,
	GrapeFarm,

	CottonFarm,
	DyeFarm,

	CoffeeFarm,
	TulipFarm,
	
	None,

	//
	//Ruin,
	//Glow mushroom
	//Hive
	//Excellent wine area
};

struct GeoresourceInfo
{
	GeoresourceEnum georesourceEnum = GeoresourceEnum::None;
	FText name;

	bool isLandmark() {
		return !isMountainOre() && 
				geoTreeEnum == TileObjEnum::None && 
				geoBushEnum == TileObjEnum::None;
	}
	bool isMountainOre() { return mountainOreEnum != TileObjEnum::None; }

	const FString* GetDisplayName() { return FTextInspector::GetSourceString(name); }
	
	TileObjEnum mountainOreEnum = TileObjEnum::None;
	TileObjEnum geoTreeEnum;
	TileObjEnum geoBushEnum;

	ResourceEnum resourceEnum = ResourceEnum::None;
	
	FText description;
};

#define LOCTEXT_NAMESPACE "GeoresourceInfos"

static const std::vector<GeoresourceInfo> GeoresourceInfos
{
	{ GeoresourceEnum::Hotspring,		LOCTEXT("Hot Spring", "Hot Spring") , TileObjEnum::None,TileObjEnum::None, TileObjEnum::None, ResourceEnum::None, LOCTEXT("Hot Spring Desc", "Grants happiness bonus to any house built in this region.") },
	{ GeoresourceEnum::GiantMushroom,  LOCTEXT("Giant Mushroom", "Giant Mushroom") , TileObjEnum::None,TileObjEnum::GiantMushroom, TileObjEnum::None, ResourceEnum::None, LOCTEXT("Giant Mushroom Desc", "Strange giant mushrooms. Grants science bonus to any house built in this region.") },
	{ GeoresourceEnum::CherryBlossom,  LOCTEXT("Cherry Blossom", "Cherry Blossom") , TileObjEnum::None,TileObjEnum::Cherry, TileObjEnum::None, ResourceEnum::None, LOCTEXT("Cherry Blossom Desc", "Area known for beautiful cherry blossoms. When they bloom, cherry blossoms give citizens bonus happiness.") },

	//{ GeoresourceEnum::GiantTree,  "Giant tree" , TileObjEnum::None,TileObjEnum::None, TileObjEnum::None, "Grants happiness bonus to any house built in this region." },
	{ GeoresourceEnum::Ruin,			LOCTEXT("Ruin", "Ruin"), TileObjEnum::None,TileObjEnum::None,TileObjEnum::None,   ResourceEnum::None, LOCTEXT("Ruin Desc", "Remains of a mysterious ancient civilization.") },

	{ GeoresourceEnum::IronOre,			LOCTEXT("Iron Ore", "Iron Ore"),	TileObjEnum::IronMountainOre,TileObjEnum::None,TileObjEnum::None, ResourceEnum::IronOre, LOCTEXT("Iron Ore Desc", "Build Iron Mine over this Iron Deposit to mine Iron.") },
	{ GeoresourceEnum::CoalOre,			LOCTEXT("Coal Ore", "Coal Ore"),	TileObjEnum::CoalMountainOre,TileObjEnum::None,TileObjEnum::None, ResourceEnum::Coal, LOCTEXT("Coal Ore Desc", "Build Coal Mine over this Coal Deposit to mine Coal.") },
	{ GeoresourceEnum::GoldOre,			LOCTEXT("Gold Ore", "Gold Ore"),	TileObjEnum::GoldMountainOre,TileObjEnum::None,TileObjEnum::None, ResourceEnum::GoldOre, LOCTEXT("Gold Ore Desc", "Build Gold Mine over this Gold Deposit to mine Gold.") },
	{ GeoresourceEnum::Gemstone,		LOCTEXT("Gemstone", "Gemstone"),	TileObjEnum::GemstoneOre,TileObjEnum::None,TileObjEnum::None, ResourceEnum::Gemstone, LOCTEXT("Gemstone Desc", "Build Gemstone Mine over this Gemstone Deposit to mine Gemstone.") },
	{ GeoresourceEnum::Oil,			LOCTEXT("Oil", "Oil"),				TileObjEnum::None, TileObjEnum::None, TileObjEnum::None, ResourceEnum::Oil,		LOCTEXT("Oil Desc", "Build Oil Rig over this Oil Well to extract Oil.") },

	{ GeoresourceEnum::CannabisFarm,	LOCTEXT("Cannabis", "Cannabis"), TileObjEnum::None,TileObjEnum::None, TileObjEnum::Cannabis,  ResourceEnum::Cannabis, LOCTEXT("Cannabis Desc", "Area suitable for growing Cannabis.") },
	{ GeoresourceEnum::CocoaFarm,		LOCTEXT("Cocoa", "Cocoa"), TileObjEnum::None,TileObjEnum::None, TileObjEnum::Cocoa,  ResourceEnum::Cocoa, LOCTEXT("Cocoa Desc", "Area suitable for growing Cocoa.") },
	{ GeoresourceEnum::GrapeFarm,		LOCTEXT("Grape", "Grape"), TileObjEnum::None,TileObjEnum::None, TileObjEnum::Grapevines,  ResourceEnum::Grape, LOCTEXT("Grape Desc", "Area suitable for growing Grape.") },
	{ GeoresourceEnum::CottonFarm,		LOCTEXT("Cotton", "Cotton"), TileObjEnum::None,TileObjEnum::None, TileObjEnum::Cotton,  ResourceEnum::Cotton, LOCTEXT("Cotton Desc", "Area suitable for growing Cotton.") },
	{ GeoresourceEnum::DyeFarm,			LOCTEXT("Dye", "Dye"), TileObjEnum::None,TileObjEnum::None, TileObjEnum::Dye,  ResourceEnum::Dye, LOCTEXT("Dye Desc", "Area suitable for growing Dye.") },

	{ GeoresourceEnum::CoffeeFarm,		LOCTEXT("Coffee", "Coffee"), TileObjEnum::None,TileObjEnum::None, TileObjEnum::RawCoffee,  ResourceEnum::RawCoffee, LOCTEXT("Coffee Desc", "Area suitable for growing Coffee.") },
	{ GeoresourceEnum::TulipFarm,		LOCTEXT("Tulip", "Tulip"), TileObjEnum::None,TileObjEnum::None, TileObjEnum::Tulip,  ResourceEnum::Tulip, LOCTEXT("Tulip Desc", "Area suitable for growing Tulip.") },

	// Diamond, Copper

};

#undef LOCTEXT_NAMESPACE 

static const int32 GeoresourceEnumCount = GeoresourceInfos.size();

static const GeoresourceInfo GeoresourceInfoNone = { GeoresourceEnum::None, FText(), TileObjEnum::None, TileObjEnum::None, TileObjEnum::None, ResourceEnum::None, FText() };

static GeoresourceInfo GetGeoresourceInfo(GeoresourceEnum georesourceEnum) {
	if (georesourceEnum == GeoresourceEnum::None) {
		return GeoresourceInfoNone;
	}
	return GeoresourceInfos[static_cast<int>(georesourceEnum)];
};
static GeoresourceInfo GetGeoresourceInfo(int32 georesourceEnumInt) {
	if (static_cast<GeoresourceEnum>(georesourceEnumInt) == GeoresourceEnum::None) {
		return GeoresourceInfoNone;
	}
	return GeoresourceInfos[georesourceEnumInt];
};

static const std::vector<GeoresourceEnum> FarmGeoresourceEnums
{
	GeoresourceEnum::CannabisFarm,
	GeoresourceEnum::CocoaFarm,
	GeoresourceEnum::GrapeFarm,
	GeoresourceEnum::CottonFarm,
	GeoresourceEnum::DyeFarm,
	
	GeoresourceEnum::CoffeeFarm,
	GeoresourceEnum::TulipFarm,
};
static bool IsFarmGeoresource(GeoresourceEnum georesourceEnumIn)
{
	for (GeoresourceEnum georesourceEnum : FarmGeoresourceEnums) {
		if (georesourceEnumIn == georesourceEnum) {
			return true;
		}
	}
	return false;
}


struct GeoresourceNode
{
	GeoresourceEnum georesourceEnum = GeoresourceEnum::None;
	int32 provinceId = -1;
	WorldTile2 centerTile;
	
	TileArea area;

	// mining
	int32 depositAmount = 0;
	int32 stoneAmount = 0;
	
	bool isUsedUp = false; // such as ruins etc.

	static GeoresourceNode Create(GeoresourceEnum georesourceEnum, int32 provinceId, WorldTile2 centerTile, TileArea georesourceArea = TileArea())
	{
		GeoresourceNode node = { georesourceEnum, provinceId, centerTile, georesourceArea };
		node.stoneAmount = 15000 + GameRand::Rand(provinceId) % 10000;
		return node;
	}
	
	GeoresourceInfo info() const {
		return GetGeoresourceInfo(georesourceEnum);
	}

	bool HasResource() const {
		return georesourceEnum != GeoresourceEnum::None;
	}

	//! Serialize
	FArchive& operator>>(FArchive &Ar) {
		Ar << georesourceEnum;
		Ar << provinceId;
		centerTile >> Ar;
		area >> Ar;

		Ar << depositAmount;
		Ar << stoneAmount;
		
		Ar << isUsedUp;
		return Ar;
	}
};

/*
 * Seeds
 */

struct SeedInfo
{
	CardEnum cardEnum = CardEnum::None;
	TileObjEnum tileObjEnum = TileObjEnum::None;
	GeoresourceEnum georesourceEnum = GeoresourceEnum::None;

	bool isValid() { return cardEnum != CardEnum::None; }
	static SeedInfo Invalid() { return {}; }

	bool operator==(const SeedInfo& a) const {
		return cardEnum == a.cardEnum && tileObjEnum == a.tileObjEnum;
	}
};

static const std::vector<SeedInfo> CommonSeedCards
{
	{ CardEnum::WheatSeed, TileObjEnum::WheatBush },
	{ CardEnum::CabbageSeed, TileObjEnum::Cabbage },
	{ CardEnum::HerbSeed, TileObjEnum::Herb },

	{ CardEnum::PotatoSeed, TileObjEnum::Potato },
	{ CardEnum::BlueberrySeed, TileObjEnum::Blueberry },
	{ CardEnum::PumpkinSeed, TileObjEnum::Pumpkin },
	{ CardEnum::MelonSeed, TileObjEnum::Melon },
};

static const std::vector<SeedInfo> SpecialSeedCards
{
	{ CardEnum::CannabisSeeds, TileObjEnum::Cannabis, GeoresourceEnum::CannabisFarm },
	{ CardEnum::GrapeSeeds, TileObjEnum::Grapevines, GeoresourceEnum::GrapeFarm },
	{ CardEnum::CocoaSeeds, TileObjEnum::Cocoa, GeoresourceEnum::CocoaFarm },
	{ CardEnum::CottonSeeds, TileObjEnum::Cotton, GeoresourceEnum::CottonFarm },
	{ CardEnum::DyeSeeds, TileObjEnum::Dye, GeoresourceEnum::DyeFarm },

	{ CardEnum::CoffeeSeeds, TileObjEnum::RawCoffee, GeoresourceEnum::CoffeeFarm },
	{ CardEnum::TulipSeeds, TileObjEnum::Tulip, GeoresourceEnum::TulipFarm },
};

static bool IsActionCard(CardEnum cardEnumIn)
{
	for (CardEnum cardEnum : ActionCards) {
		if (cardEnumIn == cardEnum) return true;
	}
	for (const SeedInfo& seedInfo : CommonSeedCards) {
		if (cardEnumIn == seedInfo.cardEnum) return true;
	}
	for (const SeedInfo& seedInfo : SpecialSeedCards) {
		if (cardEnumIn == seedInfo.cardEnum) return true;
	}
	return false;
}

static std::vector<SeedInfo> GetSeedInfos()
{
	std::vector<SeedInfo> allSeeds;
	allSeeds.insert(allSeeds.begin(), CommonSeedCards.begin(), CommonSeedCards.end());
	allSeeds.insert(allSeeds.begin(), SpecialSeedCards.begin(), SpecialSeedCards.end());
	return allSeeds;
};

static SeedInfo GetSeedInfo(CardEnum seedCardEnum)
{
	for (auto info : CommonSeedCards) {
		if (info.cardEnum == seedCardEnum) return info;
	}
	for (auto info : SpecialSeedCards) {
		if (info.cardEnum == seedCardEnum) return info;
	}
	return SeedInfo::Invalid();
};

static SeedInfo GetSeedInfo(GeoresourceEnum georesourceEnum)
{
	for (auto info : CommonSeedCards) {
		if (info.georesourceEnum == georesourceEnum) return info;
	}
	for (auto info : SpecialSeedCards) {
		if (info.georesourceEnum == georesourceEnum) return info;
	}
	return SeedInfo::Invalid();
};

static bool IsCommonSeedCard(CardEnum cardEnumIn)
{
	for (auto info : CommonSeedCards) {
		if (info.cardEnum == cardEnumIn) {
			return true;
		}
	}
	return false;
}
static bool IsSpecialSeedCard(CardEnum cardEnumIn)
{
	for (auto info : SpecialSeedCards) {
		if (info.cardEnum == cardEnumIn) {
			return true;
		}
	}
	return false;
}
static bool IsSeedCard(CardEnum cardEnumIn) {
	return IsCommonSeedCard(cardEnumIn) || IsSpecialSeedCard(cardEnumIn);
}

static ResourceEnum GeoresourceEnumToResourceEnum(GeoresourceEnum georesourceEnum)
{
	if (IsFarmGeoresource(georesourceEnum))
	{
		SeedInfo seedInfo = GetSeedInfo(georesourceEnum);
		check(seedInfo.isValid());
		return GetTileObjInfo(seedInfo.tileObjEnum).cutDownResourceBase100.resourceEnum;
	}
	
	switch (georesourceEnum)
	{
	case GeoresourceEnum::IronOre: return ResourceEnum::IronOre;
	case GeoresourceEnum::CoalOre: return ResourceEnum::Coal;
	case GeoresourceEnum::GoldOre: return ResourceEnum::GoldOre;
	case GeoresourceEnum::Gemstone: return ResourceEnum::Gemstone;
	case GeoresourceEnum::Oil: return ResourceEnum::Oil;
		
	default:
		UE_DEBUG_BREAK();
		return ResourceEnum::Wheat;
	}
}


struct FarmTile
{
	int32 farmTileId = -1;
	int32 groupId = -1;
	WorldTile2 worldTile;

	bool isTileWorked = false;
	int32 reservedUnitId = -1;

	bool isValid() { return farmTileId != -1; }

	//! Serialize
	FArchive& operator>>(FArchive &Ar) {
		Ar << farmTileId;
		Ar << groupId;
		worldTile >> Ar;
		Ar << isTileWorked;
		Ar << reservedUnitId;
		return Ar;
	}
};

/*
 * Items
 */

enum class ItemEnum
{
	RubiksCube,
	BottledFairy,
	AncientCoin,
	MasterTraderManual,
	
	HitchhikersGuide,
	LonelyMansJournal,
	RadiatingPoop,
	
	//Necronomicon,
	//Codex,
	//Skull,
	//StrangeDoll,
	////DeadBranch

	None,
};

struct ItemInfo
{
	//ItemEnum itemEnum = ItemEnum::None;
	std::string name;
	std::string description;
};

static const ItemInfo ItemInfos[]
{
	{ "Rubiks Cube", "+10% science." },
	{ "Ancient Coin", "+5% house tax." },
	{ "Business Manual", "Halved trade countdown. Trading post's maintenance is x5 more expensive." },
	
	{ "Hitchhiker's Guide", "-10% science, +10% house tax." },
	{ "Lonely Man's Journal", "Can no longer trade. -10% food consumption." },
	{ "Radiating Poop", "+30 gold per round." },
};

static ItemInfo GetItemInfo(ItemEnum itemEnum) {
	return ItemInfos[static_cast<int>(itemEnum)];
}

static const int32_t ItemCount = _countof(ItemInfos);

#define LOCTEXT_NAMESPACE "TaxOptions"

static const TArray<FText> TaxOptions = {
	LOCTEXT("Very low tax", "Very low tax"),
	LOCTEXT("Low tax", "Low tax"),
	LOCTEXT("Medium tax", "Medium tax"),
	LOCTEXT("High tax", "High tax"),
	LOCTEXT("Very high tax", "Very high tax"),
};
static const TArray<FString> VassalTaxOptions = {
	FString("No vassal tax"), 
	FString("Low vassal tax"),
	FString("Medium vassal tax"), 
	FString("High vassal tax"),
	FString("Very high vassal tax"), 
};

#undef LOCTEXT_NAMESPACE


enum class AutosaveEnum : uint8
{
	Off,
	HalfYear,
	Year,
	TwoYears,
};
#define LOCTEXT_NAMESPACE "AutosaveOptions"
static const std::vector<FText> AutosaveOptions = {
	LOCTEXT("Autosave_Off", "Off"),
	LOCTEXT("Autosave_HalfYear", "Every Half Year"),
	LOCTEXT("Autosave_Yearly", "Yearly"),
	LOCTEXT("Autosave_TwoYears", "Every Two Years"),
};
#undef LOCTEXT_NAMESPACE

/*
 * Stat
 */
enum class SeasonStatEnum : uint8
{
	Birth,
	DeathAge,
	DeathStarve,
	DeathCold,
	DeathKilled,
	DeathSick,

	BushBirth,
	BushDeathAge,
	BushDeathWinter,
	BushDeathGather,
	BushUnitTrimmed,
	BushEmitSeed,
	BushEmitSeedGoodSpot,
	BushEmitSeedSuccess,

	Money,
	Science,
	Influence,
};
static const std::string SeasonStatName[] =
{
	"Birth",
	"DeathAge",
	"DeathStarve",
	"DeathCold",
	"DeathKilled",
	"DeathSick",

	"BushBirth",
	"BushDeathAge",
	"BushDeathWinter",
	"BushDeathGather",
	"BushUnitTrimmed",
	"BushEmitSeed",
	"BushEmitSeedGoodSpot",
	"BushEmitSeedSuccess",

	"Money",
	"Science",
	"Influence",
};
static const int32 SeasonStatEnumCount = _countof(SeasonStatName);

enum class AccumulatedStatEnum
{
	StoragesDestroyed,
};
static const std::string AccumulatedStatName[] =
{
	"StoragesDestroyed",
};
static const int32 AccumulatedStatEnumCount = _countof(AccumulatedStatName);


enum class PlotStatEnum
{
	Population,
	AdultPopulation,
	ChildPopulation,
	
	Income,
	Revenue,
	Expense,

	Science,
	
	Food,
	Fuel,
	Export,
	Import,
	TradeBalance,

	MarketPrice,
	TipMarketPrice,

	Technologies,
	InfluencePoints,

	FoodProduction,
	FoodConsumption,

	Count,
};

/*
 * Happiness
 */
enum class HappinessEnum
{
	Food,
	//Heat,
	//Healthcare,
	Housing,
	Luxury,
	Entertainment,
	Job,
	CityAttractiveness, // New Colonists, Cannibalism, Tax etc.
	Tourism,
};

#define LOCTEXT_NAMESPACE "HappinessName"
static const TArray<FText> HappinessEnumName
{
	LOCTEXT("Food", "Food"),
	//LOCTEXT("Heat", "Heat"),
	//LOCTEXT("Healthcare", "Healthcare"),
	LOCTEXT("Housing", "Housing"),
	LOCTEXT("Luxury", "Luxury"),
	LOCTEXT("Entertainment", "Entertainment"),
	LOCTEXT("Job", "Job"),
	LOCTEXT("City Attractiveness", "City Attractiveness"),
	
	LOCTEXT("Tourism", "Tourism"),
};
static const TArray<FText> HappinessEnumTip
{
	LOCTEXT("FoodHappiness_Tip", "To Increase Food Happiness:<bullet>Have enough food for Citizens</><bullet>Have Food Variety</>"),
	//LOCTEXT("Heat", "Heat"),
	//LOCTEXT("Healthcare", "Healthcare"),
	LOCTEXT("HousingHappiness_Tip", "To Increase Housing Happiness:<bullet>Have enough housing for Citizens</><bullet>Have high Housing Appeal</>"),
	LOCTEXT("LuxuryHappiness_Tip", "To Increase Luxury Happiness, provide your Citizens with Luxury Resources"),
	LOCTEXT("EntertainmentHappiness_Tip", "To Increase Entertain Happiness, provide your Citizens with access to Entertainment Services (Tavern, Theatre etc.)."),
	LOCTEXT("JobHappiness_Tip", "Job Happiness is affected by:<bullet>Job type of your Citizens</><bullet>Building's Budget and Work Hours</>"),
	LOCTEXT("CityAttractivenessHappiness_Tip", "City Attractiveness is affected by Tax and Card Bonuses. City Attractivenss starts off high with hopes and dreams of initial settlers, but decay over time."),
	
	LOCTEXT("TourismHappiness_Tip", "Citizens of our town will gain Tourism Happiness if they can visit Hotels other towns via trade routes."),
};
static FText GetHappinessEnumTip(HappinessEnum happinessEnum) { return HappinessEnumTip[static_cast<int>(happinessEnum)]; }
#undef LOCTEXT_NAMESPACE
static const int32 HappinessEnumCount = HappinessEnumName.Num();

static int32 GetHappinessColorLevel(int32 value)
{
	if (value < 50) return 0; // Red
	if (value < 70) return 1; // Orange
	if (value < 85) return 2; // Yellow
	return 3; // Green
}
static FText ColorHappinessText(int32 value, FText textIn)
{
	int32 level = GetHappinessColorLevel(value);
	switch (level)
	{
	case 0: return TEXT_TAG("<FaintRed12>", textIn);
	case 1: return TEXT_TAG("<FaintOrange12>", textIn);
	case 2: return TEXT_TAG("<FaintYellow12>", textIn);
	case 3: return TEXT_TAG("<FaintGreen12>", textIn);
	default:
		UE_DEBUG_BREAK();
		return textIn;
	}
}
static FLinearColor GetHappinessColor(int32 value)
{
	int32 level = GetHappinessColorLevel(value);
	switch (level)
	{
	case 0: return FLinearColor(1, 0.3, 0.3);
	case 1: return FLinearColor(1, 0.6, 0.3);
	case 2: return FLinearColor(1, 1, 0.3);
	case 3: return FLinearColor(0.3, 1, 0.3);
	default:
		UE_DEBUG_BREAK();
		return FLinearColor(1, 0, 0);
	}
}
static FText GetHappinessFace(int32 value)
{
	int32 level = GetHappinessColorLevel(value);
	switch (level)
	{
	case 0: return INVTEXT("<img id=\"HappinessRed\"/>");
	case 1: return INVTEXT("<img id=\"HappinessOrange\"/>");
	case 2: return INVTEXT("<img id=\"HappinessYellow\"/>");
	case 3: return INVTEXT("<img id=\"HappinessGreen\"/>");
	default:
		UE_DEBUG_BREAK();
		return INVTEXT("<Red>");
	}
}

/*
 * Fun Service
 */
enum class FunServiceEnum : uint8
{
	//Museum,
	//Zoo,
	Theatre,
	Tavern,
	
	Count,
};
static const int32 FunServiceEnumCount = static_cast<int32>(FunServiceEnum::Count);

struct FunServiceInfo
{
	FunServiceEnum funServiceEnum = FunServiceEnum::Theatre;
	CardEnum buildingEnum = CardEnum::None;
	int32 baseServiceQuality = -1;
	int32 funPercentToGetService = -1;
};

const std::vector<FunServiceInfo> FunServiceToCardEnum
{
	//{FunServiceEnum::Museum, CardEnum::Museum, 100, 95 },
	//{FunServiceEnum::Zoo, CardEnum::Zoo, 100, 95 },
	{FunServiceEnum::Theatre, CardEnum::Theatre, 100, 95 },
	{FunServiceEnum::Tavern, CardEnum::Tavern, 75, 70 },
};

static bool IsFunServiceBuilding(CardEnum buildingEnum)
{
	for (const FunServiceInfo& info : FunServiceToCardEnum) {
		if (info.buildingEnum == buildingEnum) {
			return true;
		}
	}
	return false;
}
static const FunServiceInfo& GetFunServiceInfo(CardEnum buildingEnum)
{
	for (const FunServiceInfo& info : FunServiceToCardEnum) {
		if (info.buildingEnum == buildingEnum) {
			return info;
		}
	}
	UE_DEBUG_BREAK();
	return FunServiceToCardEnum[0];
}
static const FunServiceInfo& GetFunServiceInfo(FunServiceEnum funServiceEnum) {
	return FunServiceToCardEnum[static_cast<int32>(funServiceEnum)];
}

static FunServiceEnum BuildingEnumToFunService(CardEnum buildingEnum)
{
	for (const auto& pair : FunServiceToCardEnum) {
		if (pair.buildingEnum == buildingEnum) {
			return pair.funServiceEnum;
		}
	}
	UE_DEBUG_BREAK();
	return FunServiceEnum::Tavern;
}
static int32 BuildingEnumToBaseServiceQuality(CardEnum buildingEnum) // Find a better way to do this???
{
	for (const auto& pair : FunServiceToCardEnum) {
		if (pair.buildingEnum == buildingEnum) {
			return pair.baseServiceQuality;
		}
	}
	UE_DEBUG_BREAK();
	return 0;
}
static CardEnum FunServiceToBuildingEnum(FunServiceEnum funEnum)
{
	for (const auto& pair : FunServiceToCardEnum) {
		if (pair.funServiceEnum == funEnum) {
			return pair.buildingEnum;
		}
	}
	UE_DEBUG_BREAK();
	return CardEnum::Tavern;
}


/*
 * Happiness Modifier
 */

enum class HappinessModifierEnum
{
	Luxury,
	HappyBreadDay,
	BlingBling,
	
	Tax,
	Cannibalism,
	DeathStarve,
	DeathCold,
};

#define LOCTEXT_NAMESPACE "HappinessModifierName"

static const TArray<FText> HappinessModifierName
{
	LOCTEXT("luxury", "luxury"),
	LOCTEXT("happy bread day", "happy bread day"),
	LOCTEXT("bling bling", "bling bling"),
	
	LOCTEXT("tax", "tax"),
	LOCTEXT("cannibalism", "cannibalism"),
	LOCTEXT("starvation death", "starvation death"),
	LOCTEXT("freezing death", "freezing death"),
};

#undef LOCTEXT_NAMESPACE

/*
 * Save
 */
enum class SaveCheckState
{
	None,
	SerializeBeforeSave,
	SerializeBeforeSave_Done,
	MapTransition,
	SerializeAfterSave,
	SerializeAfterSave_Done,
};



/*
 * Camera
 */

const int32 MinZoomSkipSteps = 1;
const int32 NormalZoomSkipSteps = 4;
const int32 MaxZoomSkipSteps = 8;

const int32 MaxZoomStep = 28 * NormalZoomSkipSteps;
const int32 MinZoomStep = 0;

const int32 StepsTakenToDoubleZoomHeight = 4 * NormalZoomSkipSteps;
const float ZoomDoubleFactor = 1.0f / StepsTakenToDoubleZoomHeight; // = 1 / (steps taken to double the zoom height)

const float MinZoomAmount = 100.0f;
const float MinZoomLookAtHeight = MinZoomAmount * 0.3f; // 0.5f


static float GetCameraZoomAmount(int32 cameraZoomStep) {
	return  MinZoomAmount * pow(2.0f, cameraZoomStep * ZoomDoubleFactor);
}
static float GetZoomStepFromAmount(float zoomAmount) {
	// zoomAmount = minZoomAmount * pow(2.0f, _cameraZoomStep * _zoomDoubleFactor)
	// log(zoomAmount / minZoomAmount) = _cameraZoomStep * _zoomDoubleFactor * log(2)
	// _cameraZoomStep = log(zoomAmount / minZoomAmount) / (_zoomDoubleFactor * log(2))
	// Note: _zoomDoubleFactor of 1.0f means 1 step is double the zoom...
	return std::log(zoomAmount / MinZoomAmount) / (ZoomDoubleFactor * std::log(2));
}

// TODO: this should be its own file??

// 475, 565, 672, 800, 951
// 541

const int32 zoomStepBounceLow = 40;
const int32 zoomStepBounceHigh = 60;

// Note: Careful when use ... not 100% accurate
static bool IsZoomBouncing(float smoothZoomAmount) {
	return  GetCameraZoomAmount(zoomStepBounceLow) < smoothZoomAmount && smoothZoomAmount < GetCameraZoomAmount(zoomStepBounceHigh);
}
static bool IsZoomBouncingUp(float smoothZoomAmount, int32 cameraZoomStep) {
	return IsZoomBouncing(smoothZoomAmount) && cameraZoomStep >= zoomStepBounceHigh;
}

const float WorldZoomAmountStage1 = 480;//480;
const float WorldZoomAmountStage2 = 680; // 680
const float WorldZoomAmountStage3 = 810; // 810
const float WorldZoomAmountStage4 = 1200;
const float WorldZoomAmountStage5 = 1800;

const float WorldZoomTransition_WorldSpaceUIShrink = 480;
const float WorldZoomTransition_WorldSpaceUIHide = 680;

const float WorldZoomTransition_RegionToRegion4x4 = GetCameraZoomAmount(zoomStepBounceLow + 1);
const float WorldZoomTransition_RegionToRegion4x4_Mid = GetCameraZoomAmount((zoomStepBounceLow + zoomStepBounceHigh) / 2); // swap Region in at mid height when zoom bouncing to prevent flash

const int32 ZoomStep_Region4x4ToMap = 69; // 2260, When terrain region chunks changed to TerrainMap mesh
const float WorldZoomTransition_Region4x4ToMap = GetCameraZoomAmount(ZoomStep_Region4x4ToMap); 
const float WorldZoomTransition_Region4x4ToMap_SlightBelow = GetCameraZoomAmount(ZoomStep_Region4x4ToMap - 1); // 1900

const float WorldZoomTransition_Buildings = 810;
const float WorldZoomTransition_BuildingsMini = WorldZoomTransition_Region4x4ToMap;

const float WorldToMapZoomAmount = WorldZoomTransition_Region4x4ToMap;

const float WorldZoomTransition_Tree = WorldZoomTransition_Region4x4ToMap;
const float WorldZoomTransition_Bush = GetCameraZoomAmount(zoomStepBounceLow + 1);
const float WorldZoomTransition_TreeHidden = 5000;

const float WorldZoomTransition_GameToMap = GetCameraZoomAmount(zoomStepBounceLow + 1);

const float WorldZoomTransition_PostProcessEnable = 3000;

const float MapIconShrinkZoomAmount = 5382;

//

static int32 GetTileRadius(float zoomDistance)
{
	if (zoomDistance < WorldZoomAmountStage1) {
		return 70;
	}
	if (zoomDistance < WorldZoomAmountStage2) {
		return 105; // 680 / 475 * 70 = 100
	}
	if (zoomDistance < WorldZoomAmountStage3) {
		return 128; // 810 / 475 * 70 = 119
	}
	if (zoomDistance < WorldZoomAmountStage3) {
		return WorldZoomAmountStage3 * 70 / 475; // 810 / 475 * 70 = 119
	}
	if (zoomDistance < WorldZoomAmountStage4) {
		return  WorldZoomAmountStage4 * 70 / 475; 
	}
	if (zoomDistance < WorldZoomAmountStage5) {
		return WorldZoomAmountStage5 * 70 / 475;
	}
	return WorldToMapZoomAmount * 70 / 475;
}

const int32 MinMouseRotateAnglePerHalfScreenMove = 20;
const int32 MaxMouseRotateAnglePerHalfScreenMove = 180;




const float MinVolume = 0.0f;// 0.011f;

const float NetworkInputDelayTime = 3.0f;


static const FString AITownNames[] = 
{
	"Rouen",
	"Hastein",
	"Borghild",
	"Uppsala",
	"Birka",
	"Luna",
	"Havre",
	"Rohal",
	"Stark",
	"Rivendell",
	"Lannister",
	"Baratheon",
	"Gotham",
	"Baltia",
	"Biarmaland",
	"Camelot",
	"Diyu",
	"Elysian",
	"Irkalla",
	"Nibiru",
	"Jotunheim",
	"Talaki",
	"Bujia",
	"Barameria",
	"Karaku",
	"Zakan",
	"Buroc",
	"Kiacook",
	"Bhuket",
	"Katao",
	"Bjorn",
	"Munso",
	"Malaren",
};

static FString GetAITownName(int32 playerId)
{
	int32 count = _countof(AITownNames);
	int32 aiPlayerId = std::max(0, playerId - GameConstants::MaxPlayersPossible);
	FString name = AITownNames[aiPlayerId % count];
	if (aiPlayerId / count > 0) {
		name += FString::FromInt(aiPlayerId / count);
	}
	return name;
}

static const std::vector<FString> NameAdjectives
{
	"Atori",
	"Badae",
	"Blleque",
	"Brague",
	"Brant",
	"Ecanchoun",
	"Ecomaunt",
	"Gaupai",
	"Gloe",
	"Hatie",
	"Liban",
	"Lige",
	"Mulaer",
	"Nord",
	"Ouest",
	"Ouinaer",
	"Sud",
	"Trutae",
	
	
	"Jara",
	"Kuga",
	"Bak",
	"Rata",
	"Ga",
	"Bu",
	"Da",
	"Di",
	"Aki",
};

static const std::vector<FString> NameNoun
{
	"bec",
	"bau",
	"ber",
	"bie",
	"bar",
	"bingue",
	"bitaer",
	"bitte",
	"blikyi",
	"colin",
	"dale",
	"ecalle",
	"eclo",
	"est",
	"estran",
	"etac",
	"etoc",
	"feste",
	"fllo",
	"furolles",
	"gardin",
	"gare",
	"leican",
	"marsouin",
	"londe",
	"mielle",
	"mucre",
	"muque",
	"roque",
	"rogue",
	"rohal",
	"sigle",
	"tierre",
	"thuit",
	"tondre",
	"tot",
	"tun",
	"ullac",
	"vamoque",
	"vrek",
	"vrak",
	"videco",
	"virewite",
	"vatre",
	"torve",

	
	"nu",
	"baya",
	"ri",
	"ku",
	"bana"
	"ron",
	"wa",
	"nus",
	"nu",
	"ki",
	"mun",
};

static FText GenerateTribeName(int32 seed)
{
	int32 firstNameIndex = GameRand::Rand(seed);
	FString firstName = NameAdjectives[firstNameIndex % NameAdjectives.size()];
	int32 lastNameIndex = GameRand::Rand(firstNameIndex);
	FString lastName = NameNoun[lastNameIndex % NameNoun.size()];

	FString finalName;
	if (firstName.Len() + lastName.Len() > 10) {
		TCHAR firstChar = lastName[0];
		lastName[0] = TChar<TCHAR>::ToUpper(firstChar);
		finalName = firstName + " " + lastName;
	}
	else {
		finalName = firstName + lastName;
	}

	return FText::FromString(finalName);
}




template<typename TA, typename TB>
struct PunPair
{
	TA first;
	TB second;

	PunPair(TA first, TB second) : first(first), second(second) {}

	//! Serialize
	FArchive& operator>>(FArchive &Ar) {
		Ar << first << second;
		return Ar;
	}
};


enum class CallbackEnum : uint8
{
	None,
	ClaimLandMoney,
	ClaimLandInfluence,
	//ClaimLandFood,

	StartAttackProvince,
	ReinforceAttackProvince,
	ReinforceDefendProvince,
	DefendProvinceMoney,
	Liberate,
	RaidBattle,
	Raze,

	BattleRetreat,

	ClaimLandArmy,
	CancelClaimLandArmy,

	ClaimLandIndirect,
	BuildOutpost,
	DemolishOutpost,

	ClaimRuin,
	SelectPermanentCard,
	SelectUnboughtCard,
	SelectBoughtCard,
	SelectUnboughtRareCardPrize,
	SelectConverterCard,
	SelectCardRemoval,
	SellCard,
	SelectBuildingSlotCard,
	SelectDeployMilitarySlotCard,
	SelectInventorySlotCard,
	CardInventorySlotting,
	SelectCardSetSlotCard,
	CardSetSlotting,
	ArchivesSlotting,

	SelectStartingLocation,
	UpgradeBuilding,
	CaptureUnit,
	TrainUnit,
	CancelTrainUnit,
	OpenStatistics,

	OpenSetTradeOffers,
	IntercityTrade,

	OpenChooseResource,
	PickChooseResource,
	EditNumberChooseResource,

	OpenManageStorage,

	EditableNumberSetOutputTarget,
	EditableNumberSetHotelFeePerVisitor,

	SetDeliveryTarget,
	RemoveDeliveryTarget,

	CloseGameSettingsUI,
	CloseLoadSaveUI,
	OpenBlur,
	SaveGameOverrideConfirm,
	SelectSaveGame,
	AbandonTown,

	LobbyListElementSelect,

	QuestOpenDescription,

	SelectEmptySlot,
	LobbyChoosePlayerLogo,

	SetGlobalJobPriority_Up,
	SetGlobalJobPriority_Down,
	SetGlobalJobPriority_FastUp,
	SetGlobalJobPriority_FastDown,

	SetMarketTarget,

	TradingPostTrade,

	QuickBuild,
	AddAnimalRanch,

	BudgetAdjust,

	TownNameEdit,
	BuildingSwapArrow,

	// Military
	ArmyConquer,
	ArmyLiberate,
	ArmyHelp,
	ArmyMoveBetweenNode,
	ArmyRecall,
	ArmyReinforce,
	ArmyRebel,
	ArmyRetreat,
	AllyRequest,
	AllyBetray,

	IncrementArmyCount,
	ChooseArmyNode,

	DeclareFriendship,
	MarryOut,
	ProposeAlliance,

	// AutoTrade
	AddAutoTradeResource,
	ShiftAutoTradeRowUp,
	ShiftAutoTradeRowDown,
	ShiftAutoTradeRowFastUp,
	ShiftAutoTradeRowFastDown,
	RemoveAutoTradeRow,
	AutoTradeRowTargetInventoryChanged,
	AutoTradeRowMaxTradeAmountChanged,
	EstablishTradeRoute,
	CancelTradeRoute,

	// Spy
	SpyEstablishNest,
	SpyEnsureAnonymity,

	// TradeDeal
	AddTradeDealResource,
	RemoveTradeDealResource,
	AddTradeDealCard,
	RemoveTradeDealCard,

	UpdateTradeDealMoney,
	UpdateTradeDealResource,

	// ForeignBuilding
	ForeignBuildingAllow,
	ForeignBuildingDisallow,

	// Lobby Player Settings
	ChoosePlayerLogo,
	ChoosePlayerColor1,
	ChoosePlayerColor2,
	ChoosePlayerCharacter,
};

static bool IsAutoTradeCallback(CallbackEnum callbackEnum)
{
	int32 callbackInt = static_cast<int32>(callbackEnum);
	return static_cast<int32>(CallbackEnum::AddAutoTradeResource) <= callbackInt && callbackInt <= static_cast<int32>(CallbackEnum::AutoTradeRowMaxTradeAmountChanged);
}

static bool IsSpyCallback(CallbackEnum callbackEnum)
{
	return callbackEnum == CallbackEnum::SpyEstablishNest ||
		callbackEnum == CallbackEnum::SpyEnsureAnonymity;
}

static const int32 SpyNestBasePrice = 3000;


static const int32 GameSpeedHalf = -12;
static const int32 GameSpeedValue1 = GameSpeedHalf;
static const int32 GameSpeedValue2 = 1;
static const int32 GameSpeedValue3 = 2;
static const int32 GameSpeedValue4 = 5;

#define LOCTEXT_NAMESPACE "GameSpeedName"

static FText GameSpeedName(int32 gameSpeed)
{
	switch(gameSpeed)
	{
	case GameSpeedValue1: return LOCTEXT("x1 speed", "x1 speed");
	case GameSpeedValue2: return LOCTEXT("x2 speed", "x2 speed");
	case GameSpeedValue3: return LOCTEXT("x4 speed", "x4 speed");
	case GameSpeedValue4: return LOCTEXT("x10 speed", "x10 speed");
	}
	return LOCTEXT("paused", "paused");
}

#undef LOCTEXT_NAMESPACE

/*
 * Provinces
 */
const int32 RiverProvinceId = -INT32_MAX;
const int32 OceanProvinceId = -INT32_MAX + 1;
const int32 MountainProvinceId = -INT32_MAX + 2;
const int32 InvalidProvinceId = -INT32_MAX + 3;
const int32 EmptyProvinceId = -INT32_MAX + 4;

static bool IsValidRawProvinceId(int32 provinceId) {
	return provinceId > EmptyProvinceId;
}
static bool IsMountainOrWaterProvinceId(int32 provinceId) {
	return provinceId <= MountainProvinceId;
}
static bool IsWaterProvinceId(int32 provinceId) {
	return provinceId <= OceanProvinceId;
}
static bool IsValidNonEdgeProvinceId(int32 provinceId) {
	return provinceId >= 0; // Edge marked with negative...
}

static bool IsEdgeProvinceId(int32 provinceId) {
	return 0 > provinceId && provinceId > EmptyProvinceId; // Edge marked with negative...
}

static const int32 Income100PerFertilityPercent = 15; // 10 -> Sep: 30 ->
static const int32 ClaimToIncomeRatio = 50; // When 2, actual is 4 since upkeep is half the income...

struct ProvinceConnection
{
	int32 provinceId;
	TerrainTileType tileType;
	int32 connectedTiles;

	bool isConnectedTileType(bool withShallowWater = false) const {
		if (withShallowWater && tileType == TerrainTileType::Ocean) {
			return true;
		}
		return tileType == TerrainTileType::None ||
				tileType == TerrainTileType::River;
	}

	ProvinceConnection() : provinceId(-1), tileType(TerrainTileType::None), connectedTiles(0) {}

	ProvinceConnection(int32 provinceId, TerrainTileType tileType) : provinceId(provinceId), tileType(tileType), connectedTiles(1) {}

	bool operator==(const ProvinceConnection& a) const {
		return a.provinceId == provinceId && a.tileType == tileType;
	}

	//! Serialize
	FArchive& operator>>(FArchive &Ar) {
		Ar << provinceId;
		Ar << tileType;
		return Ar;
	}
};

enum class BoolEnum : uint8
{
	False,
	True,
	NeedUpdate,
};


#define LOCTEXT_NAMESPACE "HoverWarning"

enum class HoverWarning : uint8 {
	None,
	Depleted,
	StorageFull,
	StorageTooFar,
	HouseTooFar,

	NotEnoughInput,
	NotEnoughElectricity,

	Inaccessible,

	NotEnoughMoney,
	AlreadyReachedTarget,
	ResourcesBelowTarget,

	IntercityLogisticsNeedTargetTown,
	IntercityLogisticsNeedTargetResource,

	NeedSetup,
	NeedDeliveryTarget,

	DeliveryTargetTooFar,
	OutputInventoryFull,

	NeedTradeRoute,
	AwaitingApproval,
};

static const std::vector<FText> HoverWarningString = {
	FText(),
	LOCTEXT("Depleted", "Depleted"),
	LOCTEXT("Storage Full", "Storage Full"),
	LOCTEXT("Storage Too Far", "Storage Too Far"),
	LOCTEXT("House Too Far", "House Too Far"),

	LOCTEXT("Not Enough Input", "Not Enough Input"),
	LOCTEXT("Not Enough Electricity", "Not Enough\nElectricity"),

	LOCTEXT("Inaccessible", "Inaccessible"),

	LOCTEXT("Not Enough Money", "Not Enough Money"),
	LOCTEXT("Import Target Reached", "Import Target\nReached"),
	LOCTEXT("Resources Below Export Target", "Resources Below\nExport Target"),

	LOCTEXT("IntercityLogisticsNeedTargetTown", "Need Target\nCity"),
	LOCTEXT("IntercityLogisticsNeedTargetResource", "Need Target\nResource"),

	LOCTEXT("Need Setup", "Need Setup"),
	LOCTEXT("Need Delivery Target", "Need\nDelivery Target"),

	LOCTEXT("Delivery Target Too Far", "Delivery Target\nToo Far"),
	LOCTEXT("Output Inventory Full", "Output Inventory\nFull"),

	LOCTEXT("Need Trade Route", "Need Trade Route"),
	LOCTEXT("Awaiting Approval", "Awaiting\nApproval"),
};
static const std::vector<FText> HoverWarningDescription = {
	FText(),
	LOCTEXT("Depleted Desc","Ores are Depleted in this Province."),
	LOCTEXT("Storage Full Desc", "All Storage are full. Build more Storage Yard or Warehouse."),
	LOCTEXT("Storage Too Far Desc", "This building is too far from Storage. Build a new Storage nearby to ensure this building runs efficiently."),
	LOCTEXT("House Too Far Desc", "This building is too far from Houses. Build a new House nearby to ensure this building runs efficiently."),

	LOCTEXT("Not Enough Input Desc", "Not enough Input Resource to keep this building running."),
	LOCTEXT("Not Enough Electricity Desc", "Not Enough Electricity to make this Building produce at maximum Productivity."),

	LOCTEXT("Inaccessible Desc", "Building's gate tile cannot be reached by Citizens"),

	FText(),
	FText(),
	FText(),

	LOCTEXT("IntercityLogisticsNeedTargetTown Desc", "Set target City where resources will be taken from."),
	LOCTEXT("IntercityLogisticsNeedTargetResource Desc", "Set storage targets of resources to be delivered."),

	LOCTEXT("Need Setup Desc", "Choose Resource Types that will be carried by Logistics Workers."),
	LOCTEXT("Need Delivery Target Desc", "Set the Delivery Target Storage where Logistics Worker will carry resources to."),

	LOCTEXT("Delivery Target Too Far Desc", "Delivery Target is too far (maximum 150 tiles)"), // TODO: For now only Logistics Office
	LOCTEXT("Output Inventory Full Desc", "Output Resource Inventory is full causing the production to pause."),

	LOCTEXT("Need Trade Route Desc", "Need Trade Route"),
	LOCTEXT("Awaiting Approval Desc", "Waiting for another player to approve foreign building."),
};
static FText GetHoverWarningName(HoverWarning hoverWarning) { return HoverWarningString[static_cast<int>(hoverWarning)]; }
static FText GetHoverWarningDescription(HoverWarning hoverWarning) { return HoverWarningDescription[static_cast<int>(hoverWarning)]; }

#undef LOCTEXT_NAMESPACE


/*
 * Diplomacy
 */

static const int32 GoldToRelationship = 20;
static const int32 InfluenceToRelationship = 5;

enum class RelationshipModifierEnum : uint8
{
	YouGaveUsGifts,
	YouAreStrong,
	YouBefriendedUs,
	WeAreFamily,
	GoodTradeDeal,
	DiplomaticBuildings,

	AdjacentBordersSparkTensions,
	TownhallProximitySparkTensions,
	YouAreWeak,
	YouStealFromUs,
	YouKidnapFromUs,
	YouTerrorizedUs,
	YouAttackedUs,
	WeFearCannibals,
};

FText RelationshipModifierNameInt(int32 index); // Names
int32 RelationshipModifierCount();

class RelationshipModifiers
{
public:
	RelationshipModifiers()
	{
		_relationshipModifiers.resize(GameConstants::MaxPlayersAndAI);
		for (size_t i = 0; i < _relationshipModifiers.size(); i++) {
			_relationshipModifiers[i].resize(RelationshipModifierCount());
		}
		_isAlly.SetNum(GameConstants::MaxPlayersAndAI);
	}

	void GetAIRelationshipText(TArray<FText>& args, int32 playerId) const;

	int32 GetTotalRelationship(int32 towardPlayerId) const;

	void SetModifier(int32 askingPlayerId, RelationshipModifierEnum modifier, int32 value) {
		_relationshipModifiers[askingPlayerId][static_cast<int>(modifier)] = value;
	}
	void ChangeModifier(int32 askingPlayerId, RelationshipModifierEnum modifier, int32 value) {
		_relationshipModifiers[askingPlayerId][static_cast<int>(modifier)] += value;
	}
	void DecayModifier(int32 askingPlayerId, RelationshipModifierEnum modifier, int32 changeValue = 1) {
		int32& modifierValue = _relationshipModifiers[askingPlayerId][static_cast<int>(modifier)];
		if (modifierValue > 0) modifierValue = std::max(0, modifierValue - changeValue);
		if (modifierValue < 0) modifierValue = std::min(0, modifierValue + changeValue);
	}

	//int32& GetModifierMutable(int32 askingPlayerId, RelationshipModifierEnum modifier) {
	//	return _relationshipModifiers[askingPlayerId][static_cast<int>(modifier)];
	//}
	int32 GetModifier(int32 askingPlayerId, RelationshipModifierEnum modifier) const {
		return _relationshipModifiers[askingPlayerId][static_cast<int>(modifier)];
	}

	//const std::vector<std::vector<int32>>& relationshipModifiers() { return _relationshipModifiers; }

	

	// TODO: for abandon town
	void ClearRelationshipModifiers(int32 towardPlayerId) {
		std::vector<int32>& modifiers = _relationshipModifiers[towardPlayerId];
		std::fill(modifiers.begin(), modifiers.end(), 0);
	}
	

	//! Friendship
	bool shouldShow_DeclareFriendship(int32 askingPlayerId) {
		if (GetModifier(askingPlayerId, RelationshipModifierEnum::YouAttackedUs) > 0) {
			return false;
		}
		return GetModifier(askingPlayerId, RelationshipModifierEnum::YouBefriendedUs) == 0;
	}
	int32 friendshipPrice() { return 200; }
	void DeclareFriendship(int32 askingPlayerId) {
		SetModifier(askingPlayerId, RelationshipModifierEnum::YouBefriendedUs, friendshipPrice() / GoldToRelationship);
	}
	

	//! Marriage
	bool shouldShow_MarryOut(int32 askingPlayerId) {
		return GetModifier(askingPlayerId, RelationshipModifierEnum::WeAreFamily) == 0;
	}
	int32 marryOutPrice() { return 1000; }
	void MarryOut(int32 askingPlayerId) {
		SetModifier(askingPlayerId, RelationshipModifierEnum::WeAreFamily, marryOutPrice() / GoldToRelationship);
	}

	//! Alliance
	bool isAlly(int32 askingPlayerId) const { return _isAlly[askingPlayerId]; }
	bool CanCreateAlliance(int32 askingPlayerId) {
		return GetTotalRelationship(askingPlayerId) >= AllyRelationshipRequirement(askingPlayerId);
	}
	void SetAlliance(int32 askingPlayerId, bool isAlly) {
		_isAlly[askingPlayerId] = isAlly;
	}

	int32 AllyRelationshipRequirement(int32 askingPlayerId)
	{
		int32 maxRelationship = AllyRelationshipMinimumRequirement;
		for (int32 i = 0; i < _relationshipModifiers.size(); i++) {
			if (i != askingPlayerId) {
				maxRelationship = std::max(maxRelationship, GetTotalRelationship(i) + 30);
			}
		}
		return maxRelationship;
	}

	int32 GetMainAllyId()
	{
		for (int32 i = 0; i < _isAlly.Num(); i++) {
			if (_isAlly[i]) {
				return i;
			}
		}
		return -1;
	}
	

	bool isEnemy(int32 askingPlayerId) { return GetTotalRelationship(askingPlayerId) < 0; }
	

	static const int32 AllyRelationshipMinimumRequirement = 100;

	
	//! Serialize
	friend FArchive& operator<<(FArchive& Ar, RelationshipModifiers& object)
	{
		SerializeVecVecValue(Ar, object._relationshipModifiers);
		Ar << object._isAlly;
		return Ar;
	}


private:
	std::vector<std::vector<int32>> _relationshipModifiers;
	TArray<uint8> _isAlly;
};


enum class TradeDealStageEnum : uint8
{
	None,
	
	Gifting, // Instant
	CreateDeal,
	ExamineDeal,
	PrepareCounterOfferDeal,
	ExamineCounterOfferDeal,
	AcceptDeal, // Instant
};

struct TradeDealSideInfo
{
	int32 playerId = -1;
	int32 moneyAmount = 0;
	std::vector<ResourcePair> resourcePairs;
	std::vector<CardStatus> cardStatuses;
};


enum class GameSaveChunkEnum : uint8
{
	Terrain,
	Trees,
	Flood1,
	//Debug,
	//Debug2,
	Others,

	Count,

	All,
};

/*
 * Buildings
 */
enum class CutTreeEnum : uint8 {
	Any,
	FruitTreeOnly,
	NonFruitTreeOnly,
};

/*
 * Game Constants
 */
static const int32 InitialStorageSize = 4;
static const WorldTile2 InitialStorageTileSize(4, 4);
static const int32 InitialStorageShiftFromTownhall = GetBuildingInfo(CardEnum::Townhall).size.y / 2 + InitialStorageSize / 2;
static const WorldTile2 Storage1ShiftTileVec(0, -InitialStorageShiftFromTownhall);
static const WorldTile2 InitialStorage2Shift(4, 0);

static const int32 Colony_InitialStorageSize = 6;
static const WorldTile2 Colony_InitialStorageTileSize(Colony_InitialStorageSize, Colony_InitialStorageSize);
static const int32 Colony_InitialStorageShiftFromTownhall = GetBuildingInfo(CardEnum::Townhall).size.y / 2 + Colony_InitialStorageSize / 2;
static const WorldTile2 PortColony_Storage1ShiftTileVec(-Colony_InitialStorageShiftFromTownhall, -2);
static const WorldTile2 PortColony_InitialStorage2Shift(0, 6);
static const WorldTile2 PortColony_PortExtraShiftTileVec(-3, 0);

static const int32 ClaypitRiverFractionPercentThreshold = 20;
static const int32 IrrigationReservoirRiverFractionPercentThreshold = 2;
static int32 GetRiverFractionPercentThreshold(CardEnum buildingEnum) {
	return buildingEnum == CardEnum::ClayPit ? ClaypitRiverFractionPercentThreshold : IrrigationReservoirRiverFractionPercentThreshold;
}


static int32 GetMaxAICount(MapSizeEnum mapSizeEnum)
{
	switch (mapSizeEnum) {
	case MapSizeEnum::Large: return GameConstants::MaxAIs;
	case MapSizeEnum::Medium: return 8;
	case MapSizeEnum::Small: return 3;
	default: return 0;
	}
}
static int32 GetDefaultAICount(MapSizeEnum mapSizeEnum)
{
	switch (mapSizeEnum) {
	case MapSizeEnum::Large: return 7;
	case MapSizeEnum::Medium: return 3;
	case MapSizeEnum::Small: return 1;
	default: return 0;
	}
}

static const int32 MinorTownShift = 10000;
static bool IsMinorTown(int32 townId) {
	return townId >= MinorTownShift;
}
static bool IsMajorTown(int32 townId) {
	return townId < MinorTownShift;
}
static bool IsValidMajorTown(int32 townId) {
	return townId != -1 && townId < MinorTownShift;
}
static int32 TownIdToMinorTownId(int32 townId) {
	return townId - MinorTownShift;
}

/*
 * Faction
 */

class FactionInfo
{
public:
	FactionEnum factionEnum = FactionEnum::None;

	FText name;

	FText uniqueBonusDescription;
	
	FactionInfo(FactionEnum factionEnum, FText name, FText uniqueBonusDescription) :
		factionEnum(factionEnum),
		name(name),
		uniqueBonusDescription(uniqueBonusDescription)
	{}

};

#define LOCTEXT_NAMESPACE "FactionInfo"

static const std::vector<FactionInfo> FactionInfos =
{
	FactionInfo(FactionEnum::Europe, LOCTEXT("Europe", "Europe"), LOCTEXT("Europe Ability Description", "+5% research speed")),
	FactionInfo(FactionEnum::Arab, LOCTEXT("Arab", "Arab"), LOCTEXT("Arab Ability Description", "+10% industrial production"))
};

#undef LOCTEXT_NAMESPACE


static int32 FactionEnumCount = FactionInfos.size();

static const FactionInfo& GetFactionInfo(FactionEnum factionEnum) {
	return FactionInfos[static_cast<int32>(factionEnum)];
}
static const FactionInfo& GetFactionInfoInt(int32 factionEnumInt) {
	return FactionInfos[factionEnumInt];
}

static FString WithFactionName(FactionEnum factionEnum, const FString& moduleSetName) {
	return moduleSetName + GetFactionInfo(factionEnum).name.ToString();
}
static FString WithFactionName(const FString& moduleSetName) {
	return moduleSetName + GetFactionInfo(FactionEnum::Europe).name.ToString();
}


// TODO:
enum class AIPersonalityEnum
{
	Warlike,
	Raider,
	Friendly,
	Peaceful,
	DislikeWar,
	Kind,
	Militaristic,
	ZooLover,
	ArtifactLover,
	Financier,
	Expansionist,
	AdmireExpansionist,
	HateStrictWorkCondition,
	LoveStrictWorkCondition,
	Xenophobic,
	Xenophilic,
};

enum class AIArchetypeEnum
{
	Peaceful1,
	Peaceful2,
	//Peaceful3,
	//Peaceful4,
	//Peaceful5,
	//Peaceful6,
	//Peaceful7,
	//Peaceful8,
	//
	//Merchant1,
	//Warlike1,
};

class AIArchetypeInfo
{
public:
	AIArchetypeEnum archetypeEnum;
	std::vector<FText> names;
	
	std::vector<int32> logoIndices;
	FLinearColor logoColor1;
	FLinearColor logoColor2;
	int32 characterIndex;
	int32 factionIndex;

	FactionEnum factionEnum() const { return static_cast<FactionEnum>(factionIndex); }
	
	std::vector<AIPersonalityEnum> personalityEnums;

	const FText& name(int32 seed) const {
		return names[seed % names.size()];
	}
	
	int32 logoIndex(int32 seed) const {
		return logoIndices[seed % logoIndices.size()];
	}

	//AIArchetypeInfo(AIArchetypeEnum archetypeEnum, FText name) :
	//	archetypeEnum(archetypeEnum),
	//	name(name)
	//{}
};


#define LOCTEXT_NAMESPACE "AIPlayerArchetypeInfo"

static const std::vector<AIArchetypeInfo> AIArchetypeInfos =
{
	{ AIArchetypeEnum::Peaceful1,
		{ LOCTEXT("Peaceful1", "Peaceful1") },
		{ 0, 1, 2, 3, 4, 5, 6, 7, 8 },
		FLinearColor::Black,
		FLinearColor::Yellow,
		1,
		0
	},
	{ AIArchetypeEnum::Peaceful2,
		{ LOCTEXT("Peaceful2", "Peaceful2") },
		{ 0, 1, 2, 3, 4, 5, 6, 7, 8 },
		FLinearColor::Black,
		FLinearColor::Yellow,
		1,
		0
	},
};

#undef LOCTEXT_NAMESPACE

static const int32 AIArchetypeCount = AIArchetypeInfos.size();

static const AIArchetypeInfo& GetAIArchetypeInfo(AIArchetypeEnum aiArchetypeEnum)
{
	return AIArchetypeInfos[static_cast<int32>(aiArchetypeEnum)];
}


/*
 * Ancient Wonders
 */

static const std::vector<CardEnum> AncientWonderEnums =
{
	CardEnum::MayanPyramid,
	CardEnum::EgyptianPyramid,
	CardEnum::StoneHenge,
	CardEnum::EasterIsland,
};
static const std::vector<std::vector<BiomeEnum>> AncientWonderToBiomeEnums =
{
	{ BiomeEnum::Jungle, BiomeEnum::Savanna, BiomeEnum::Forest },
	{ BiomeEnum::Desert, BiomeEnum::Savanna, BiomeEnum::Forest  },
	{ BiomeEnum::BorealForest, BiomeEnum::Forest,  BiomeEnum::Tundra },
	{ BiomeEnum::Forest, BiomeEnum::Jungle,  BiomeEnum::Savanna },
};

static bool IsAncientWonderCardEnum(CardEnum cardEnum) {
	return IsCardEnumBetween(cardEnum, CardEnum::MayanPyramid, CardEnum::EasterIsland);
}

/*
 * Artifacts
 */


static const int32 BaseArtifactExcavationCost = 1000;


/*
 * Zoo
 * Museum
 * Card Combiner
 */

enum class CardSetTypeEnum : uint8
{
	Zoo,
	Museum,
	CardCombiner,
};

class CardSetInfo
{
public:
	FText name;
	FText description;
	
	std::vector<CardEnum> cardEnums;

	CardSetInfo(FText name, FText description, std::vector<CardEnum> cardEnums) :
		name(name), description(description), cardEnums(cardEnums)
	{}
};

#define LOCTEXT_NAMESPACE "CardSetInfo"

static const std::vector<CardSetInfo> ZooSetInfos
{
	CardSetInfo(LOCTEXT("Deer Set", "Deer Set"), LOCTEXT("Deer Set Desc", "+50%<img id=\"Coin\"/> from Caravan"), { CardEnum::RedDeer, CardEnum::YellowDeer, CardEnum::DarkDeer }),
	CardSetInfo(LOCTEXT("Boar Set", "Boar Set"), LOCTEXT("Boar Set Desc", "-5% food consumption"), { CardEnum::Boar }),
	CardSetInfo(LOCTEXT("Bear Set", "Bear Set"), LOCTEXT("Bear Set Desc", "+50% City Attractiveness"), { CardEnum::BrownBear, CardEnum::BlackBear, CardEnum::Panda }),
};

static const std::vector<CardSetInfo> MuseumSetInfos
{
	CardSetInfo(LOCTEXT("Prosperity", "Prosperity"),		LOCTEXT("Prosperity Desc", "+50%<img id=\"Coin\"/> from Caravan"), { CardEnum::Codex, CardEnum::SacrificialAltar, CardEnum::StoneStele }),
	CardSetInfo(LOCTEXT("Mummification", "Mummification"), LOCTEXT("Mummification Desc", ""), { CardEnum::CanopicJars, CardEnum::DepartureScrolls, CardEnum::DeathMask }),
	CardSetInfo(LOCTEXT("Pilgrimage", "Pilgrimage"),		LOCTEXT("Pilgrimage Desc", "Hotel gives +50% influence bonus"), { CardEnum::FeastRemains, CardEnum::ForeignTrinkets, CardEnum::ChalkPlaque }),
	CardSetInfo(LOCTEXT("Islander Life", "Islander Life"), LOCTEXT("Islander Life Desc", "+50%<img id=\"Influence\"/> from Minor Cities Bonus"), { CardEnum::TatooingNeedles, CardEnum::Petroglyphs, CardEnum::StoneFishhooks }),

	CardSetInfo(LOCTEXT("A Fulfilled Life", "A Fulfilled Life"), LOCTEXT("A Fulfilled Life Desc", "Every 2% Happiness above 70%,\ngives +1% productivity (town)"), { CardEnum::BallCourtGoals, CardEnum::SolarBarque }),
	CardSetInfo(LOCTEXT("Remembrance", "Remembrance"),		LOCTEXT("Remembrance Desc", "+1% City Attractiveness for each House Lv 8\n(per city, max 100%)"), { CardEnum::OfferingCup, CardEnum::CoralEyes }),

	CardSetInfo(LOCTEXT("Royal Heritage", "Royal Heritage"), LOCTEXT("Royal Heritage Desc", "Every 1% City Attractiveness gives \n+1%<img id=\"Influence\"/> from Minor Cities Bonus"), { CardEnum::FeatherCrown, CardEnum::GoldCapstone, CardEnum::OrnateTrinkets, CardEnum::AncientStaff }),
};

static const std::vector<CardSetInfo> CardCombinerSetInfos
{
	CardSetInfo(LOCTEXT("Productivity+ Book", "Productivity+ Book"), LOCTEXT("", ""), { CardEnum::ProductivityBook, CardEnum::ProductivityBook, CardEnum::ProductivityBook }),
	CardSetInfo(LOCTEXT("Sustainanility+ Book", "Sustainanility+ Book"), LOCTEXT("", ""), { CardEnum::SustainabilityBook, CardEnum::SustainabilityBook, CardEnum::SustainabilityBook }),
	
	CardSetInfo(LOCTEXT("Motivation+ Book", "Motivation+ Book"), LOCTEXT("", ""), { CardEnum::Motivation, CardEnum::Motivation, CardEnum::Motivation, CardEnum::Motivation, CardEnum::Motivation }),
	CardSetInfo(LOCTEXT("Passion+ Book", "Passion+ Book"), LOCTEXT("", ""), { CardEnum::Passion, CardEnum::Passion, CardEnum::Passion, CardEnum::Passion, CardEnum::Passion }),
};

#undef LOCTEXT_NAMESPACE

enum class ZooCardSetEnum
{
	
};


enum class MuseumCardSetEnum
{

};

enum class CombinerCardSetEnum
{

};


/*
 * Light weight alternative to UE4's FArchive FMemoryReader etc.
 */
 // Network Serializer
static void FString_SerializeAndAppendToBlob(FString inStr, TArray<int32>& arr)
{
	arr.Add(inStr.Len());
	for (int32 i = 0; i < inStr.Len(); i++) {
		arr.Add(static_cast<int32>(inStr[i]));
	}
}

static FString FString_DeserializeFromBlob(const TArray<int32>& arr, int32& readIndex)
{
	FString result;
	int32 len = arr[readIndex++];
	for (int32 i = 0; i < len; i++) {
		result.AppendChar(static_cast<TCHAR>(arr[readIndex++]));
	}
	return result;
}

class PunSerializedData : public TArray<int32>
{
public:
	PunSerializedData(bool isSaving) {
		readIndex = isSaving ? -1 : 0;
	}

	PunSerializedData(bool isSaving, const TArray<int32>& blob, int32 index = 0)
	{
		readIndex = isSaving ? -1 : index;
		Append(blob);
	}

	int32 readIndex = -1;

	bool isSaving() { return readIndex == -1; }


	void operator<<(int32& value) {
		if (isSaving()) {
			Add(value);
		}
		else {
			value = (*this)[readIndex++];
		}
	}
	void operator<<(int16& value) {
		if (isSaving()) {
			Add(value);
		}
		else {
			value = (*this)[readIndex++];
		}
	}
	void operator<<(int8& value) {
		if (isSaving()) {
			Add(value);
		}
		else {
			value = (*this)[readIndex++];
		}
	}
	void operator<<(uint16& value) {
		if (isSaving()) {
			Add(value);
		}
		else {
			value = (*this)[readIndex++];
		}
	}
	void operator<<(uint8& value) {
		if (isSaving()) {
			Add(value);
		}
		else {
			value = (*this)[readIndex++];
		}
	}
	void operator<<(bool& value) {
		if (isSaving()) {
			Add(value);
		}
		else {
			value = (*this)[readIndex++];
		}
	}

	// Enum
	template <
		typename EnumType,
		typename = typename TEnableIf<TIsEnumClass<EnumType>::Value>::Type
	>
		void operator<<(EnumType& Value) {
		return (*this) << (__underlying_type(EnumType)&)Value;
	}

	void operator<<(FString& value) {
		if (isSaving()) {
			FString_SerializeAndAppendToBlob(value, *this);
		}
		else {
			value = FString_DeserializeFromBlob(*this, readIndex);
		}
	}

	void operator<<(TArray<int32>& inArray) {
		if (isSaving()) {
			Add(inArray.Num());
			Append(inArray);
		}
		else {
			int32 count = (*this)[readIndex++];
			for (int i = 0; i < count; i++) {
				inArray.Add((*this)[readIndex++]);
			}
		}
	}
	void operator<<(TArray<uint8>& inArray) {
		if (isSaving()) {
			Add(inArray.Num());
			Append(inArray);
		}
		else {
			int32 count = (*this)[readIndex++];
			for (int i = 0; i < count; i++) {
				inArray.Add((*this)[readIndex++]);
			}
		}
	}

	void operator<<(WorldTile2& value) {
		(*this) << value.x;
		(*this) << value.y;
	}
	void operator<<(TileArea& value) {
		(*this) << value.minX;
		(*this) << value.minY;
		(*this) << value.maxX;
		(*this) << value.maxY;
	}
};