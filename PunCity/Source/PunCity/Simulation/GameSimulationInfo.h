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

// VERSION
// !!! Don't forget SAVE_VERSION !!!
#define MAJOR_VERSION 0
#define MINOR_VERSION 24 // 3 digit

#define VERSION_DAY 31
#define VERSION_MONTH 3
#define VERSION_YEAR 21
#define VERSION_DATE (VERSION_YEAR * 10000) + (VERSION_MONTH * 100) + VERSION_DAY

#define COMBINE_VERSION (MAJOR_VERSION * 1000000000) + (MINOR_VERSION * 1000000) + VERSION_DATE

#define GAME_VERSION COMBINE_VERSION

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

// VERSION
#define MAJOR_SAVE_VERSION 0
#define MINOR_SAVE_VERSION 17 // 3 digit

#define VERSION_SAVE_DAY 15
#define VERSION_SAVE_MONTH 3
#define VERSION_SAVE_YEAR 21
#define VERSION_SAVE_DATE (VERSION_SAVE_YEAR * 10000) + (VERSION_SAVE_MONTH * 100) + VERSION_SAVE_DAY

#define SAVE_VERSION (MAJOR_SAVE_VERSION * 1000000000) + (MINOR_SAVE_VERSION * 1000000) + VERSION_SAVE_DATE

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
#define DEV_BUILD !UE_BUILD_SHIPPING

#if !defined(PUN_CHECK)
	#if UE_BUILD_SHIPPING
		#define PUN_CHECK(x) 
	#elif DEBUG_BUILD
		#define PUN_CHECK(x) if (!(x)) { FDebug::DumpStackTraceToLog(); checkNoEntry(); }
	#else
		#define PUN_CHECK(x) if (!(x)) { FDebug::DumpStackTraceToLog(); UE_LOG(LogTemp, Error, TEXT("PUN_ERROR")); }
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


#define TEXT_PERCENT(number) FText::Join(FText(), FText::AsNumber(number), INVTEXT("%"))
#define TEXT_100(number) FText::Join(FText(), TEXT_INTEGER(number), TEXT_DECIMAL(number))
#define TEXT_100_2(number) FText::Join(FText(), FText::AsNumber(number / 100), INVTEXT("."), FText::AsNumber(abs(static_cast<int>(number) % 100)))
#define TEXT_100SIGNED(number) FText::Join(FText(), (number > 0 ? INVTEXT("+") : INVTEXT("")), TEXT_INTEGER(number), TEXT_DECIMAL(number))
#define TEXT_NUM(number) FText::AsNumber(number)
#define TEXT_NUMSIGNED(number) FText::Join(FText(), (number > 0 ? INVTEXT("+") : INVTEXT("")), FText::AsNumber(number))
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

		return rainStartTick <= Ticks() && Ticks() <= rainStartTick + rainLengthTicks;
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
	Shroom,

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
	SteelBeam,

	Glassware,
	PocketWatch,

	// --- End
	None,

	Money,
	Food,
	Luxury,
	Fuel,
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
static const int32 HumanFoodPerYear = 27; // 45 (nov 19) // 35 // 70
static const int32 FoodCost = 5; // 3 (nov 19)
static const int32 BaseHumanFoodCost100PerYear = 100 * HumanFoodPerYear * FoodCost;

/*
 * HumanFoodCostPerYear affects:
 * - Industrial
 * - Hunting/Gathering through AssumedFoodProductionPerYear
 * Note: Farm is affected by this
 */
static const int32 WoodGatherYield_Base100 = 250;
static const int32 FarmBaseYield100 = 150; // 250 (nov 19)
static const int32 StoneGatherYield_Base = 4;

static const int32 CutTreeTicksBase = Time::TicksPerSecond * 10;
static const int32 HarvestDepositTicksBase = Time::TicksPerSecond * 14;

// How fast people produce value when working compare to value spent on food
// This is high because people don't spend all their time working.
//!!! Change Base Production (Except FarmBaseYield100) !!!!
static const int32 WorkRevenueToCost_Base = 128; // 150 -> 128 (Mar 26)

// TODO: WorkRevenueToCost is not yet affected by resource modifiers...

static const int32 WorkRevenue100PerYear_perMan_Base = BaseHumanFoodCost100PerYear * WorkRevenueToCost_Base / 100;
// This is the same as WorkRevenuePerManSec100_Base
static const int32 WorkRevenue100PerSec_perMan_Base = WorkRevenue100PerYear_perMan_Base / Time::SecondsPerYear;

static const int32 AssumedFoodProduction100PerYear = WorkRevenue100PerYear_perMan_Base / FoodCost; // Food has cost of 3...

// How much a person spend on each type of luxury per year. ... /4 comes from testing... it makes houselvl 2 gives x2 income compare to house lvl 1
static const int32 HumanLuxuryCost100PerYear_ForEachType = BaseHumanFoodCost100PerYear / 8;
static const int32 HumanLuxuryCost100PerRound_ForEachType = HumanLuxuryCost100PerYear_ForEachType / Time::RoundsPerYear;


static const int32 BuildManSecCostFactor100 = 10; // Building work time is x% of the time it takes to acquire the resources

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

	ResourceInfo(ResourceEnum::Paper,		LOCTEXT("Paper", "Paper"),	15, LOCTEXT("Paper Desc", "Used for research and book making")),
	ResourceInfo(ResourceEnum::Clay,		LOCTEXT("Clay", "Clay"),		3, LOCTEXT("Clay Desc", "Fine-grained earth used to make Pottery and Bricks")),
	ResourceInfo(ResourceEnum::Brick,		LOCTEXT("Brick", "Brick"),	12, LOCTEXT("Brick Desc", "Sturdy, versatile construction material")),

	ResourceInfo(ResourceEnum::Coal,		LOCTEXT("Coal", "Coal"),		7, LOCTEXT("Coal Desc", "Fuel used to heat houses or smelt ores. When heating houses, provides x2 heat vs. Wood")),
	ResourceInfo(ResourceEnum::IronOre,		LOCTEXT("Iron Ore", "Iron Ore"),		6, LOCTEXT("Iron Ore Desc", "Valuable ore that can be smelted into Iron Bar")),
	ResourceInfo(ResourceEnum::Iron,		LOCTEXT("Iron Bar", "Iron Bar"),		18, LOCTEXT("Iron Bar Desc", "Sturdy bar of metal used in construction and tool-making.")),
	ResourceInfo(ResourceEnum::Furniture,	LOCTEXT("Furniture", "Furniture"), 10,  LOCTEXT("Furniture Desc", "Luxury tier 1 used for housing upgrade. Make house a home.")),
	ResourceInfo(ResourceEnum::Chocolate,	LOCTEXT("Chocolate", "Chocolate"), 20,   LOCTEXT("Chocolate Desc", "Everyone's favorite confectionary. (Luxury tier 3)")),

	//ResourceInfo(ResourceEnum::StoneTools,		"Stone Tools",		15,  "Lowest-grade tool made by Stone Tool Shop."),
	//ResourceInfo(ResourceEnum::CrudeIronTools,	"Crude Iron Tools",	15,  "Medium-grade tool made by Blacksmith using Iron Ore and Wood."),
	ResourceInfo(ResourceEnum::SteelTools,		LOCTEXT("Steel Tool", "Steel Tool"),			27,  LOCTEXT("Steel Tool Desc", "High-grade tool made by Blacksmith from Iron Bars and Wood")),
	ResourceInfo(ResourceEnum::Herb,				LOCTEXT("Medicinal Herb", "Medicinal Herb"),				6, 	LOCTEXT("Medicinal Herb Desc", "Medicinal plant used to heal sickness")),
	ResourceInfo(ResourceEnum::Medicine,			LOCTEXT("Medicine", "Medicine"),			12,  LOCTEXT("Medicine Desc", "Potent Medicinal Herb extract used to heal sickness")),
	
	
	//ResourceInfo(ResourceEnum::Tools,		"Tools", 25, 100, "Construction material"),
	ResourceInfo(ResourceEnum::Fish,			LOCTEXT("Fish", "Fish"),		FoodCost, LOCTEXT("Fish Desc", "Tasty catch from the sea/river")), // Fish is all year round... shouldn't be very high yield...

	//ResourceInfo(ResourceEnum::WhaleMeat, "WhaleMeat", 7, 100, "Luxury food obtained from Fishing Lodge"),
	ResourceInfo(ResourceEnum::Grape,		LOCTEXT("Grape", "Grape"),			FoodCost, LOCTEXT("Grape Desc", "Juicy, delicious fruit used in Wine-making.")),
	ResourceInfo(ResourceEnum::Wine,		LOCTEXT("Wine", "Wine"),			30, LOCTEXT("Wine Desc", "Luxury tier 2 used for housing upgrade. Alcoholic drink that makes everything tastes better.")),
	ResourceInfo(ResourceEnum::Shroom,		LOCTEXT("Shroom", "Shroom"),	15,		LOCTEXT("Shroom Desc", "Psychedelic mushroom that can bring you on a hallucination trip. (Luxury tier 2)")),

	ResourceInfo(ResourceEnum::Pork,			LOCTEXT("Pork", "Pork"),		FoodCost, LOCTEXT("Pork Desc", "Delicious meat from farmed Pigs")),
	ResourceInfo(ResourceEnum::GameMeat,		LOCTEXT("Game Meat", "Game Meat"), FoodCost,  LOCTEXT("Game Meat Desc", "Delicious meat from wild animals")),
	ResourceInfo(ResourceEnum::Beef,			LOCTEXT("Beef", "Beef"),		FoodCost,  LOCTEXT("Beef Desc", "Delicious meat from ranched Cattle")),
	ResourceInfo(ResourceEnum::Lamb,			LOCTEXT("Lamb", "Lamb"),		FoodCost, LOCTEXT("Lamb Desc", "Delicious meat from ranched Sheep")),
	ResourceInfo(ResourceEnum::Cocoa,		LOCTEXT("Cocoa", "Cocoa"),		7, LOCTEXT("Cocoa Desc", "Raw cocoa used in Chocolate-making")),

	ResourceInfo(ResourceEnum::Wool,			LOCTEXT("Wool", "Wool"),	7, LOCTEXT("Wool Desc", "Fine, soft fiber used to make Clothes")),
	ResourceInfo(ResourceEnum::Leather,		LOCTEXT("Leather", "Leather"), 6, LOCTEXT("Leather Desc", "Animal skin that can be used to make Clothes")),
	ResourceInfo(ResourceEnum::Cloth,		LOCTEXT("Clothes", "Clothes"), 30, LOCTEXT("Clothes Desc", "Luxury tier 2 used for housing upgrade. Provide cover and comfort.")),

	ResourceInfo(ResourceEnum::GoldOre,		LOCTEXT("Gold Ore", "Gold Ore"), 10, LOCTEXT("Gold Ore Desc", "Precious ore that can be smelted into Gold Bar")),
	ResourceInfo(ResourceEnum::GoldBar,		LOCTEXT("Gold Bar", "Gold Bar"), 25, LOCTEXT("Gold Bar Desc", "Precious metal that can be minted into money or crafted into Jewelry")),

	ResourceInfo(ResourceEnum::Beer,			LOCTEXT("Beer", "Beer"), 10, LOCTEXT("Beer Desc", "Luxury tier 1 used for housing upgrade. The cause and solution to all life's problems.")),
	//ResourceInfo(ResourceEnum::Barley,		"Barley", FoodCost, 100, "Edible grain, obtained from farming. Ideal for brewing Beer"),
	//ResourceInfo(ResourceEnum::Oyster,		"Oyster", 7, IndustryTuneFactor + 50, "A delicacy from the Sea"),
	ResourceInfo(ResourceEnum::Cannabis,		LOCTEXT("Cannabis", "Cannabis"), 6, LOCTEXT("Cannabis Desc", "Luxury tier 1 used for housing upgrade.")),
	//ResourceInfo(ResourceEnum::Truffle,		"Truffle", 7, IndustryTuneFactor + 50, "Construction material"),
	//ResourceInfo(ResourceEnum::Coconut,		"Coconut", 7, IndustryTuneFactor + 50, "Hard shell fruit with sweet white meat and delicious juice"),
	ResourceInfo(ResourceEnum::Cabbage,		LOCTEXT("Cabbage", "Cabbage"), FoodCost, LOCTEXT("Cabbage Desc", "Healthy green vegetable.")),

	ResourceInfo(ResourceEnum::Pottery,		LOCTEXT("Pottery", "Pottery"), 8, LOCTEXT("Pottery Desc", "Luxury tier 1 used for housing upgrade. Versatile pieces of earthenware.")),

	ResourceInfo(ResourceEnum::Flour,		LOCTEXT("Wheat Flour", "Wheat Flour"),	9, LOCTEXT("Wheat Flour Desc", "Ingredient used to bake Bread")),
	ResourceInfo(ResourceEnum::Bread,		LOCTEXT("Bread", "Bread"),			FoodCost, LOCTEXT("Bread Desc", "Delicious food baked from Wheat Flour")), // 3 bread from 1 flour
	ResourceInfo(ResourceEnum::Gemstone,	LOCTEXT("Gemstone", "Gemstone"),	20, LOCTEXT("Gemstone Desc", "Precious stone that can be crafted into Jewelry")),
	ResourceInfo(ResourceEnum::Jewelry,		LOCTEXT("Jewelry", "Jewelry"),		70, LOCTEXT("Jewelry Desc", "Luxury tier 3 used for housing upgrade. Expensive adornment of Gold and Gems.")),

	// June 9
	
	ResourceInfo(ResourceEnum::Cotton,				LOCTEXT("Cotton", "Cotton"),				7, LOCTEXT("Cotton Desc", "Raw material used to make Cotton Fabric.")),
	ResourceInfo(ResourceEnum::CottonFabric,		LOCTEXT("Cotton Fabric", "Cotton Fabric"), 23, LOCTEXT("Cotton Fabric Desc", "Fabric used by tailors to make Clothes.")),
	ResourceInfo(ResourceEnum::DyedCottonFabric,	LOCTEXT("Dyed Cotton Fabric", "Dyed Cotton Fabric"), 43, LOCTEXT("Dyed Cotton Fabric Desc", "Fancy fabric used by tailors to make Fashionable Clothes.")),
	ResourceInfo(ResourceEnum::LuxuriousClothes,	LOCTEXT("Fashionable Clothes", "Fashionable Clothes"), 50, LOCTEXT("Fashionable Clothes Desc", "Luxury tier 3 used for housing upgrade.")),
	
	ResourceInfo(ResourceEnum::Honey,		LOCTEXT("Honey", "Honey"), FoodCost, LOCTEXT("Honey Desc", "Delicious, viscous liquid produced by bees.")),
	ResourceInfo(ResourceEnum::Beeswax,		LOCTEXT("Beeswax", "Beeswax"), 7,	LOCTEXT("Beeswax Desc", "Raw material used to make Candles.")),
	ResourceInfo(ResourceEnum::Candle,		LOCTEXT("Candles", "Candles"), 15,	LOCTEXT("Candles Desc", "Luxury tier 2 used for housing upgrade.")),
	
	ResourceInfo(ResourceEnum::Dye,			LOCTEXT("Dye", "Dye"), 7, LOCTEXT("Dye Desc", "Colored substance used for printing or dyeing clothes.")),
	ResourceInfo(ResourceEnum::Book,		LOCTEXT("Book", "Book"), 30, LOCTEXT("Book Desc", "Luxury tier 3 used for housing upgrade.")),

	// Oct 26
	ResourceInfo(ResourceEnum::Coconut,		LOCTEXT("Coconut", "Coconut"),	FoodCost, LOCTEXT("Coconut Desc", "Large delicious fruit with white meat and refreshing juice.")),

	// Dec 17
	ResourceInfo(ResourceEnum::Potato,		LOCTEXT("Potato", "Potato"),	FoodCost, LOCTEXT("Potato Desc", "Common tuber. Can be consumed as Food or brewed into Vodka.")),
	ResourceInfo(ResourceEnum::Blueberries,	LOCTEXT("Blueberries", "Blueberries"),	FoodCost, LOCTEXT("Blueberries Desc", "Blue-skinned fruit with refreshing taste.")),
	ResourceInfo(ResourceEnum::Melon,		LOCTEXT("Melon", "Melon"),			FoodCost + 3, LOCTEXT("Melon Desc", "Sweet and refreshing fruit. +3<img id=\"Coin\"/> each unit when consumed.")),
	ResourceInfo(ResourceEnum::Pumpkin,		LOCTEXT("Pumpkin", "Pumpkin"),		FoodCost, LOCTEXT("Pumpkin Desc", "Fruit with delicate, mildly-flavored flesh.")),
	ResourceInfo(ResourceEnum::RawCoffee,	LOCTEXT("Raw Coffee", "Raw Coffee"),	FoodCost + 1, LOCTEXT("Raw Coffee Desc", "Fruit that can be roasted to make Coffee.")),
	ResourceInfo(ResourceEnum::Tulip,		LOCTEXT("Tulip", "Tulip"),				FoodCost * 2, LOCTEXT("Tulip Desc", "Beautiful decorative flower. (Luxury tier 1)")),

	ResourceInfo(ResourceEnum::Coffee,		LOCTEXT("Coffee", "Coffee"),	17, LOCTEXT("Coffee Desc", "Keeps you awake. (Luxury tier 2)")), // +5<img id=\"Science\"/> each unit when consumed.
	ResourceInfo(ResourceEnum::Vodka,		LOCTEXT("Vodka", "Vodka"),		15, LOCTEXT("Vodka Desc", "Clear alcoholic beverage made from Potato. (Luxury tier 2)")),

	// Apr 9
	ResourceInfo(ResourceEnum::StoneTools,		LOCTEXT("Stone Tools", "Stone Tools"),	12, LOCTEXT("Stone Tools Desc", "Low-grade Tools made by Stone Tool Shop.")),
	ResourceInfo(ResourceEnum::Sand,		LOCTEXT("Sand", "Sand"),	12, LOCTEXT("Sand Desc", "Raw material for producing Glass.")),
	ResourceInfo(ResourceEnum::Oil,			LOCTEXT("Oil", "Oil"),	12, LOCTEXT("Oil Desc", "Fuel used to produce electricity at Oil Power Plant.")),
	
	ResourceInfo(ResourceEnum::Glass,		LOCTEXT("Glass", "Glass"),	12, LOCTEXT("Glass Desc", "Transparent construction material made from Sand")),
	ResourceInfo(ResourceEnum::Concrete,	LOCTEXT("Concrete", "Concrete"),	12, LOCTEXT("Concrete Desc", "Sturdy, versatile construction material")),
	ResourceInfo(ResourceEnum::SteelBeam,	LOCTEXT("Steel Beam", "Steel Beam"),	12, LOCTEXT("Steel Beam Desc", "Sturdy, versatile construction material")),

	ResourceInfo(ResourceEnum::Glassware,	LOCTEXT("Glassware", "Glassware"),	12, LOCTEXT("Glassware Desc", "Beautiful liquid container made from Glass. (Luxury tier 2)")),
	ResourceInfo(ResourceEnum::PocketWatch,		LOCTEXT("Pocket Watch", "Pocket Watch"),	12, LOCTEXT("Pocket Watch Desc", "Elegant timepiece crafted by Clockmakers. (Luxury tier 3)")),
	
};

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
	return ResourceInfos[static_cast<int>(resourceEnum)].name;
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
		resourceNames.push_back(ResourceNameW(static_cast<ResourceEnum>(i)));
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
static const std::vector<ResourceInfo> SortedNameResourceInfo = GetSortedNameResourceEnum();


static const std::vector<ResourceEnum> FoodEnums_NonInput
{
	// Arrange food from high to low grabbing priority
	ResourceEnum::Bread,
	ResourceEnum::Cabbage,
	ResourceEnum::Papaya,
	ResourceEnum::Coconut,
	ResourceEnum::Fish,

	ResourceEnum::Pork,
	ResourceEnum::GameMeat,
	ResourceEnum::Beef,
	ResourceEnum::Lamb,
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
	ResourceEnum::Melon,
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
	//ResourceEnum::StoneTools,
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
	
	ResourceEnum::Wheat,
	ResourceEnum::Milk,
	ResourceEnum::Mushroom,
	ResourceEnum::Grape,
};

static const ResourceEnum ConstructionResources[] = {
	ResourceEnum::Wood,
	ResourceEnum::Stone,
	ResourceEnum::Iron,
};
static const int32 ConstructionResourceCount = _countof(ConstructionResources);

/*
 * Luxury
 */

static const std::vector<std::vector<ResourceEnum>> TierToLuxuryEnums =
{
	{},
	{ResourceEnum::Beer, ResourceEnum::Cannabis, ResourceEnum::Furniture, ResourceEnum::Pottery, ResourceEnum::Tulip },
	{ResourceEnum::Cloth, ResourceEnum::Wine, ResourceEnum::Candle, ResourceEnum::Vodka, ResourceEnum::Shroom, ResourceEnum::Coffee},
	{ResourceEnum::Book, ResourceEnum::LuxuriousClothes, ResourceEnum::Jewelry, ResourceEnum::Chocolate},
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
	if (resourceEnum == ResourceEnum::IronOre ||
		resourceEnum == ResourceEnum::GoldOre ||
		resourceEnum == ResourceEnum::Clay ||
		resourceEnum == ResourceEnum::Cocoa)
	{
		return 50;
	}
	if (resourceEnum == ResourceEnum::Wood ||
		resourceEnum == ResourceEnum::Coal) 
	{
		return 80;
	}
	if (resourceEnum == ResourceEnum::Iron ||
		resourceEnum == ResourceEnum::GoldBar) 
	{
		return 120;
	}
	if (resourceEnum == ResourceEnum::Gemstone) { // Jewelry has small market
		return 30;
	}
	if (resourceEnum == ResourceEnum::Jewelry) { // Jewelry has small market
		return 80;
	}
	if (IsLuxuryEnum(resourceEnum)) {
		return 120;
	}
	
	return 80;
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
	
	StoneToolShop,
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
	TrapSpike,

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
	InventorsWorkshop,
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
	ShroomFarm,
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
	Glassworks,
	ConcreteFactory,
	CoalPowerPlant,
	Steelworks,
	StoneToolsShop,
	OilRig,
	OilPowerPlant,
	PaperMill,
	ClockMakers,

	Cathedral,
	Castle,
	GrandMuseum,
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
	SustainabilityBook,
	FrugalityBook,
	Motivation,
	Passion,
	
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
	DuplicateBuilding,

	Pig,
	Sheep,
	Cow,
	Panda,
	
	FireStarter,
	Steal,
	Snatch,
	SharingIsCaring,
	Kidnap,
	KidnapGuard,
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
	GoldRush,

	AllYouCanEat,
	SlaveLabor,
	Lockdown,
	SocialWelfare,
	
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
	
	None,

	//! For Laborer Priority
	GatherTiles,
	Hauler,
	Constructor,


	//! Special for Callback
	ArchivesSlotting,
};

enum class CardHandEnum
{
	CardSlots,
	PermanentHand,
	DrawHand,
	BoughtHand,
	ConverterHand,
	RareHand,
};

static const std::vector<std::pair<CardEnum, int32>> BuildingEnumToUpkeep =
{
	{ CardEnum::Garden, 5 },
	
	{ CardEnum::Library, 15 },
	{ CardEnum::School, 18 },
	{ CardEnum::Theatre, 50 },
	{ CardEnum::Tavern, 15 },

	{ CardEnum::Granary, 10 },

	{ CardEnum::TradingPost, 20 },
	{ CardEnum::TradingCompany, 10 },
	{ CardEnum::TradingPort, 20 },
	{ CardEnum::CardMaker, 20 },
	{ CardEnum::ImmigrationOffice, 10 },

	{ CardEnum::BarrackClubman, 10 },
	{ CardEnum::BarrackSwordman, 20 },
	{ CardEnum::BarrackArcher, 20 },


	{ CardEnum::AdventurersGuild, 50 },
};

// Building upkeep per round
static int32 GetCardUpkeepBase(CardEnum buildingEnum)
{
	auto found = std::find_if(BuildingEnumToUpkeep.begin(), BuildingEnumToUpkeep.end(), 
								[&](std::pair<CardEnum, int32> pair) { return pair.first == buildingEnum; });
	if (found != BuildingEnumToUpkeep.end()) {
		return found->second;
	}
	return 0;
}

struct BldInfo
{
	CardEnum cardEnum;
	
	// Need std::string since we need to copy when passing into FName (allows using name.c_str() instead of FName in some cases)
	FText name;
	FText namePlural;
	
	WorldTile2 size;
	ResourceEnum input1 = ResourceEnum::None;
	ResourceEnum input2 = ResourceEnum::None;
	ResourceEnum produce = ResourceEnum::None;
	int32 productionBatch = 0;

	int32 workerCount = 0;

	std::vector<int32> constructionResources;
	int32 buildTime_ManSec100 = 0;
	int32 maxBuilderCount = 0;

	int32 workRevenuePerSec100_perMan = 0;
	int32 revenuePerSec100_initialCost_perMan = 0;
	int32 revenuePerSec100_upkeep_perMan = 0;

	FText description;
	FText miniDescription;

	int32 baseCardPrice = 0;
	int32 baseUpkeep = 0;

	std::string nameStd() const { return FTextToStd(name); }
	std::wstring nameW() const { return FTextToW(name); }
	FString nameF() const { return name.ToString(); }
	FText GetName() { return name; }
	FText GetDescription() { return description; }

	FText GetName(int32 count) { return count > 1 ? namePlural : name; }

	const FString* GetDisplayName() { return FTextInspector::GetSourceString(name); }

	bool hasInput1() { return input1 != ResourceEnum::None; }
	bool hasInput2() { return input2 != ResourceEnum::None; }

	int32 constructionCostAsMoney() {
		int32 resourceCost = 0;
		for (int32 i = 0; i < constructionResources.size(); i++) {
			resourceCost += constructionResources[i] * GetResourceInfo(ConstructionResources[i]).basePrice;
		}
		return resourceCost;
	}
	

	BldInfo(CardEnum buildingEnum,
		FText nameIn,
		FText namePluralIn,
		WorldTile2 size,
		ResourceEnum input1,
		ResourceEnum input2,
		ResourceEnum produce,
		int32 productionBatch,
		int32 workerCount,

		std::vector<int32_t> constructionResources,

		FText description,
		FText miniDescriptionIn = FText()
	) :
		cardEnum(buildingEnum),
		//name(name),
		size(size),
		input1(input1),
		input2(input2),
		produce(produce),
		productionBatch(productionBatch),
		workerCount(workerCount),

		constructionResources(constructionResources),

		description(description),
		miniDescription(miniDescriptionIn)
	{
		name = nameIn;
		namePlural = namePluralIn.IsEmpty() ? name : namePluralIn;
		
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

		maxBuilderCount = std::min(maxBuilderCount, 5);

		if (miniDescription.IsEmpty()) {
			miniDescription = description;
		}

		// Farm has large area to clear.
		if (buildingEnum == CardEnum::Farm) {
			maxBuilderCount = 2;
		}
		else if (buildingEnum == CardEnum::Fort ||
				buildingEnum == CardEnum::ResourceOutpost) 
		{
			maxBuilderCount = 0;
		}

		// TEMP... instant building for storage...
		//if (name == "Storage Yard" || name == "Farm") {
		//	maxBuilderCount = 0;
		//}

		// Calculate card price from building cost to give player's idea about how expensive a building is
		int32 resourceCost = constructionCostAsMoney();

		int32 variablePrice = resourceCost / 12;
		variablePrice = (variablePrice / 10) * 10; // round to 10,20,30... etc.
		baseCardPrice = 20 + variablePrice * 2;

		// After 100, card price grows 2 times faster...
		int32 baseCardPrice_LowPart = std::min(baseCardPrice, 100);
		int32 baseCardPrice_HighPart = std::max(baseCardPrice - 100, 0);
		PUN_CHECK(baseCardPrice_LowPart > 0);
		PUN_CHECK(baseCardPrice_HighPart >= 0);
		baseCardPrice = baseCardPrice_LowPart + baseCardPrice_HighPart * 2;
		

		// Special case
		switch (buildingEnum)
		{
#define CASE(cardEnumName, cardPriceIn) case CardEnum::cardEnumName: baseCardPrice = cardPriceIn; break;

			CASE(House, 0);
			CASE(StorageYard, 0);
			CASE(DirtRoad, 0);
			CASE(StoneRoad, 0);
			CASE(Farm, 0);

			CASE(Tunnel, 3000);
			CASE(Bridge, 300);
			CASE(IntercityBridge, 1000);
			
			CASE(HumanitarianAidCamp, 500);
			
			CASE(FlowerBed, 100);
			CASE(GardenShrubbery1, 100);
			CASE(GardenCypress, 200);

			CASE(Fort, 500);
			CASE(ResourceOutpost, 5000);

			CASE(Colony, 15000);
			CASE(PortColony, 20000);
#undef CASE
		default:
			break;
		}


		/*
		 * Base upkeep varies with card price
		 */
		baseUpkeep = baseCardPrice * PercentUpkeepToPrice / 100;

		// Industry special case
		//  lux t1 is cheap to build to encourage people to build them, but harder to maintain
		if (produce != ResourceEnum::None)
		{
			if (IsLuxuryEnum(produce, 1)
				//IsLuxuryEnum(produce, 2) ||
				//IsLuxuryEnum(produce, 3) ||
				//produce == ResourceEnum::Iron ||
				//produce == ResourceEnum::GoldBar ||
				//produce == ResourceEnum::CottonFabric ||
				//cardEnum == CardEnum::Blacksmith ||
				//cardEnum == CardEnum::MedicineMaker ||
				//cardEnum == CardEnum::Brickworks ||
				//cardEnum == CardEnum::Mint ||
				//cardEnum == CardEnum::ClayPit
				)
			{
				baseUpkeep *= 2;
			}
		}
		

		if (workerCount > 0)
		{
			// Calculate WorkRevenue received from construction cost.
			// This is added to the base WorkRevenue
			// Higher cost building will work faster...
			const int32 secsToBreakEven = Time::SecondsPerYear * 3 / 4;

			int32 totalCost = resourceCost + baseCardPrice;
			int32 revenuePerSec100_initialCost = totalCost * 100 / secsToBreakEven; // Revenue increase required to get to breakeven within secsToBreakEven

			// Adjust revenueIncreasePerSec100 by WorkRevenueToCost_Base (), just like WorkRevenuePerSec100_perMan_Base
			//  This take into account time workers spend walking
			revenuePerSec100_initialCost = revenuePerSec100_initialCost * WorkRevenueToCost_Base / 100;

			revenuePerSec100_initialCost_perMan = revenuePerSec100_initialCost / workerCount; // Revenue increase effect should be distributed among workers


			// Adjust revenue by upkeep cost
			int32 upkeepPerSec100 = baseUpkeep * 100 / Time::SecondsPerRound;
			int32 revenuePerSec100_fromUpkeep = upkeepPerSec100 * 5; // upkeep turns x5 into production revenue
			revenuePerSec100_fromUpkeep = revenuePerSec100_fromUpkeep * WorkRevenueToCost_Base / 100;

			revenuePerSec100_upkeep_perMan = revenuePerSec100_fromUpkeep / workerCount;
			
			workRevenuePerSec100_perMan = WorkRevenue100PerSec_perMan_Base
											+ revenuePerSec100_initialCost_perMan
											+ revenuePerSec100_upkeep_perMan;

			//UE_LOG(LogTemp, Error, TEXT("workRevenuePerSec100_perMan %s: %d"), ToTChar(name), workRevenuePerSec100_perMan);
		}
	}

	// Cards ... TODO: may be later make this CardInfo?
		BldInfo(CardEnum buildingEnum,
				FText name, int32
				cardPrice, FText description) :
	
				cardEnum(buildingEnum),
				name(name),
				description(description),
				baseCardPrice(cardPrice)
	{
		if (miniDescription.IsEmpty()) {
			miniDescription = description;
		}
	}

	static const int32 PercentUpkeepToPrice = 10;
};

TileArea BuildingArea(WorldTile2 centerTile, WorldTile2 size, Direction faceDirection);
WorldTile2 GetBuildingCenter(TileArea area, Direction faceDirection);


// Building Cost Calculation



#define LOCTEXT_NAMESPACE "CardInfo"

static const BldInfo BuildingInfo[]
{
	// Note that size is (y, x) since y is horizontal and x is vertical in UE4
	BldInfo(CardEnum::House,		LOCTEXT("House", "House"),					LOCTEXT("House (Plural)", "Houses"),					WorldTile2(6, 6),	ResourceEnum::None, ResourceEnum::None,	ResourceEnum::None,		0, 0,	{20,0,0},	LOCTEXT("House Desc", "Protects people from cold. Extra houses boost population growth.")),
	BldInfo(CardEnum::StoneHouse,	LOCTEXT("Stone House", "Stone House"),		LOCTEXT("Stone House (Plural)", "Stone Houses"),		WorldTile2(4, 4),	ResourceEnum::None, ResourceEnum::None, ResourceEnum::None,		 0, 0,	{0,40,0},	FText()),
	BldInfo(CardEnum::FruitGatherer,LOCTEXT("Fruit Gatherer", "Fruit Gatherer"),	LOCTEXT("Fruit Gatherer (Plural)", "Fruit Gatherers"),		WorldTile2(4, 4),	ResourceEnum::None, ResourceEnum::None, ResourceEnum::None,		0, 2,	{30,0,0},	LOCTEXT("Fruit Gatherer Desc", "Gathers fruit from trees and bushes.")),
	BldInfo(CardEnum::Townhall,		LOCTEXT("Townhall",	"Townhall"),			LOCTEXT("Townhall (Plural)", "Townhalls"),			WorldTile2(12, 12),	ResourceEnum::None, ResourceEnum::None, ResourceEnum::None,		 0, 0,	{0,0,0},	LOCTEXT("Townhall Desc",	"The administrative center of your town.")),
	BldInfo(CardEnum::StorageYard,	LOCTEXT("Storage Yard",	"Storage Yard"),	LOCTEXT("Storage Yard (Plural)", "Storage Yards"),		WorldTile2(2, 2),	ResourceEnum::None, ResourceEnum::None, ResourceEnum::None,		 0, 0,	{10,0,0},	LOCTEXT("Storage Yard Desc",	"Store resources.")),

	BldInfo(CardEnum::GoldMine,		LOCTEXT("Gold Mine", "Gold Mine"),			LOCTEXT("Gold Mine (Plural)", "Gold Mines"),		WorldTile2(5, 5),	ResourceEnum::None, ResourceEnum::None, ResourceEnum::GoldOre,	 10, 3,	{50,50,0},	LOCTEXT("Gold Mine Desc", "Mine Gold Ores from Gold Deposit.")),
	BldInfo(CardEnum::Quarry,		LOCTEXT("Quarry", "Quarry"),				LOCTEXT("Quarry (Plural)", "Quarries"),				WorldTile2(5, 5),	ResourceEnum::None, ResourceEnum::None, ResourceEnum::Stone,	 10, 3,	{110,0,0},	LOCTEXT("Quarry Desc", "Mine Stone from mountain.")),
	BldInfo(CardEnum::IronStatue,	LOCTEXT("Stone Statue",	"Stone Statue"),	LOCTEXT("Stone Statue (Plural)", "Stone Statues"),	WorldTile2(4, 4),	ResourceEnum::None, ResourceEnum::None, ResourceEnum::None,		 0, 0,	{0,110,0},	FText()),
	BldInfo(CardEnum::Bank,			LOCTEXT("Bank",	"Bank"),					LOCTEXT("Bank (Plural)", "Banks"),			WorldTile2(4, 5),	ResourceEnum::None, ResourceEnum::None, ResourceEnum::None,		 0, 0,	{110,110,0},	LOCTEXT("Bank Desc", "+<img id=\"Coin\"/>10 for each surrounding level 2+ houses.")),
	BldInfo(CardEnum::IceAgeSpire,	INVTEXT("Ice Age Spire"),					INVTEXT("Ice Age Spire"),	WorldTile2(8, 8),	ResourceEnum::None, ResourceEnum::None, ResourceEnum::None,		 0, 0,	{0,300,200},	INVTEXT("Spire for worshipping Groth, the god of destruction. Decrease global temperature by -10 C."), INVTEXT("Decrease global temperature by -10 C.")),

	BldInfo(CardEnum::Farm,			LOCTEXT("Farm",	"Farm"),					LOCTEXT("Farm (Plural)",	"Farms"),				WorldTile2(8, 8),	ResourceEnum::None, ResourceEnum::None, ResourceEnum::None,		 10, 1,	{20,0,0},	LOCTEXT("Farm Desc",	"Grow food/raw materials. Harvest during autumn.")),
	BldInfo(CardEnum::MushroomFarm,	LOCTEXT("Mushroom Farm", "Mushroom Farm"), LOCTEXT("Mushroom Farm (Plural)", "Mushroom Farms"),	WorldTile2(8, 8),	ResourceEnum::Wood, ResourceEnum::None,	ResourceEnum::Mushroom,	20, 2,	{70,0,0},	LOCTEXT("Mushroom Farm Desc", "Farm Mushroom using wood.")),
	BldInfo(CardEnum::Fence,			INVTEXT("Fence"),						FText(),		WorldTile2(1, 1),	ResourceEnum::None, ResourceEnum::None, ResourceEnum::None,		 0, 0,	{0,0,0},	INVTEXT("Fence used to keep farm animals in ranches, or wild animals away from farm crops"), INVTEXT("Block units from walking on tile.")),
	BldInfo(CardEnum::FenceGate,		INVTEXT("Fence Gate"),					FText(),		WorldTile2(1, 1),	ResourceEnum::None, ResourceEnum::None, ResourceEnum::None,		 0, 0,	{0,0,0},	INVTEXT("Fence gate blocks animals from entering while letting people through."), INVTEXT("Block animals from walking on tile, while letting citizens through.")),
	BldInfo(CardEnum::Bridge,		LOCTEXT("Bridge", "Bridge"),				LOCTEXT("Bridge (Plural)", "Bridges"),		WorldTile2(1, 1),	ResourceEnum::None, ResourceEnum::None, ResourceEnum::None,		 0, 0,	{0,0,0},	LOCTEXT("Bridge Desc", "Allow citizens to cross over water.")),
	
	BldInfo(CardEnum::Forester,		LOCTEXT("Forester", "Forester"),			LOCTEXT("Forester (Plural)", "Foresters"),	WorldTile2(5, 5),	ResourceEnum::None, ResourceEnum::None, ResourceEnum::None,		 0, 2,	{50,50,0},	LOCTEXT("Forester Desc", "Cut/plants trees within your territory.")),

	BldInfo(CardEnum::CoalMine,		LOCTEXT("Coal Mine", "Coal Mine"),			LOCTEXT("Coal Mine (Plural)", "Coal Mines"),	WorldTile2(5, 5),	ResourceEnum::None, ResourceEnum::None, ResourceEnum::Coal,		10, 3,	{50,30,0},	LOCTEXT("Coal Mine Desc", "Mine Coal from Coal Deposits.")),
	BldInfo(CardEnum::IronMine,		LOCTEXT("Iron Mine", "Iron Mine"),			LOCTEXT("Iron Mine (Plural)", "Iron Mines"),	WorldTile2(5, 5),	ResourceEnum::None, ResourceEnum::None, ResourceEnum::IronOre,	 10, 3,	{50,30,0},	LOCTEXT("Iron Mine Desc", "Mine Iron Ores from Iron Deposits.")),
	BldInfo(CardEnum::SmallMarket,	LOCTEXT("Small Market", "Small Market"),	LOCTEXT("Small Market (Plural)", "Small Markets"),	WorldTile2(5, 8),	ResourceEnum::None, ResourceEnum::None, ResourceEnum::None,		 0, 0,	{80,0,0},	LOCTEXT("Small Market Desc", "Sell goods to people for <img id=\"Coin\"/>")),
	BldInfo(CardEnum::PaperMaker,	LOCTEXT("Paper Maker", "Paper Maker"),		LOCTEXT("Paper Maker (Plural)", "Paper Makers"),	WorldTile2(6, 6),	ResourceEnum::Wood, ResourceEnum::None, ResourceEnum::Paper,		 20, 3,	{80,0,0},	LOCTEXT("Paper Maker Desc", "Produce Paper.")),
	BldInfo(CardEnum::IronSmelter,		LOCTEXT("Iron Smelter", "Iron Smelter"),				LOCTEXT("Iron Smelter (Plural)", "Iron Smelters"),	WorldTile2(5, 6),	ResourceEnum::Coal,ResourceEnum::IronOre,ResourceEnum::Iron,		 10, 5,	{120,120,0},	LOCTEXT("Iron Smelter Desc", "Smelt Iron Ores into Iron Bars.")),


	BldInfo(CardEnum::StoneToolShop,	INVTEXT("Stone Tool Shop"),					INVTEXT("Stone Tool Shop"),	WorldTile2(5, 8),	ResourceEnum::Stone, ResourceEnum::Wood, ResourceEnum::SteelTools,		 10, 2,	{50,20,0},	FText()),
	BldInfo(CardEnum::Blacksmith,		LOCTEXT("Blacksmith", "Blacksmith"),	LOCTEXT("Blacksmith (Plural)", "Blacksmiths"),	WorldTile2(5, 8),	ResourceEnum::Iron, ResourceEnum::Wood, ResourceEnum::SteelTools,		 10, 2,	{50,50,50},	LOCTEXT("Blacksmith Desc", "Forge Tools from Iron Bars and Wood.")),
	BldInfo(CardEnum::Herbalist,		LOCTEXT("Herbalist", "Herbalist"),			LOCTEXT("Herbalist (Plural)", "Herbalists"),			WorldTile2(4, 4),	ResourceEnum::None, ResourceEnum::None, ResourceEnum::None,		 10, 2,	{50,30,0},	FText()),
	BldInfo(CardEnum::MedicineMaker,	LOCTEXT("Medicine Maker", "Medicine Maker"),LOCTEXT("Medicine Maker (Plural)", "Medicine Makers"),		WorldTile2(4, 4),	ResourceEnum::Herb, ResourceEnum::None, ResourceEnum::Medicine,		 10, 2,	{50,50,50},	LOCTEXT("Medicine Maker Desc", "Make Medicine from Medicinal Herb.")),

	
	BldInfo(CardEnum::FurnitureWorkshop,LOCTEXT("Furniture Workshop", "Furniture Workshop"),LOCTEXT("Furniture Workshop (Plural)", "Furniture Workshops"),	WorldTile2(6, 7),	ResourceEnum::Wood, ResourceEnum::None, ResourceEnum::Furniture,	 10, 2,	{50,20,0},	LOCTEXT("Furniture Workshop Desc", "Make Furniture from Wood.")),
	BldInfo(CardEnum::Chocolatier,	LOCTEXT("Chocolatier", "Chocolatier"),					LOCTEXT("Chocolatier (Plural)", "Chocolatiers"),			WorldTile2(6, 8),	ResourceEnum::Cocoa, ResourceEnum::Milk, ResourceEnum::Chocolate,	 10, 5,	{30, 30, 80},	LOCTEXT("Chocolatier Desc", "Make Chocolate from Milk and Cocoa.")),

	// Decorations
	BldInfo(CardEnum::Garden,		LOCTEXT("Garden", "Garden"),			LOCTEXT("Garden (Plural)", "Gardens"),				WorldTile2(4, 4),	ResourceEnum::None, ResourceEnum::None, ResourceEnum::None,		 0, 0,	{30,0,0},	LOCTEXT("Garden Desc", "Increase the surrounding appeal by 5 within 5 tiles radius.")),
	
	BldInfo(CardEnum::BoarBurrow,	LOCTEXT("Boar Burrow", "Boar Burrow"), LOCTEXT("Boar Burrow (Plural)", "Boar Burrows"),	WorldTile2(3, 3),	ResourceEnum::None, ResourceEnum::None, ResourceEnum::None,		 0, 0,	{0,0,0},	LOCTEXT("Boar Burrow Desc", "A cozy burrow occupied by a Boar family.")),

	BldInfo(CardEnum::DirtRoad,		LOCTEXT("Dirt Road", "Dirt Road"),		LOCTEXT("Dirt Road (Plural)", "Dirt Road"),			WorldTile2(1, 1),	ResourceEnum::None, ResourceEnum::None, ResourceEnum::None,		 0, 0,	{0,0,0},	LOCTEXT("Dirt Road Desc", "A crude road.\n +20% movement speed.")),
	BldInfo(CardEnum::StoneRoad,		LOCTEXT("Stone Road", "Stone Road"),	LOCTEXT("Stone Road (Plural)", "Stone Road"),			WorldTile2(1, 1),	ResourceEnum::None, ResourceEnum::None, ResourceEnum::None,		 0, 0,	{0, GameConstants::StoneNeededPerRoadTile,0},	LOCTEXT("Stone Road Desc", "A sturdy road.\n +30% movement speed.")),
	BldInfo(CardEnum::TrapSpike,		INVTEXT("Spike Trap"),					FText(),			WorldTile2(1, 1),	ResourceEnum::None, ResourceEnum::None, ResourceEnum::None,		 0, 0,	{10,0,0},	INVTEXT("Trap that can be used to hurt/disable animals or humans.")),

	BldInfo(CardEnum::Fisher,		LOCTEXT("Fishing Lodge", "Fishing Lodge"), LOCTEXT("Fishing Lodge (Plural)", "Fishing Lodges"),		WorldTile2(6, 4),	ResourceEnum::None, ResourceEnum::None, ResourceEnum::Fish,		 10, 2,	{70,0,0},	LOCTEXT("Fishing Lodge Desc", "Catch Fish from seas, lakes or rivers.")),
	BldInfo(CardEnum::BlossomShrine,	LOCTEXT("Blossom Shrine", "Blossom Shrine"),			LOCTEXT("Blossom Shrine (Plural)", "Blossom Shrines"),		WorldTile2(3, 3),	ResourceEnum::None, ResourceEnum::None, ResourceEnum::None,		 0, 0,	{0,10,0}, FText()),
	BldInfo(CardEnum::Winery,		LOCTEXT("Winery", "Winery"),	LOCTEXT("Winery (Plural)", "Wineries"),		WorldTile2(6, 6),	ResourceEnum::Grape, ResourceEnum::None, ResourceEnum::Wine,	10, 5,	{200,0,50},	LOCTEXT("Winery Desc", "Ferment Grapes into Wine.")),

	BldInfo(CardEnum::Library,		LOCTEXT("Library", "Library"),	LOCTEXT("Library (Plural)", "Libraries"),				WorldTile2(4, 5),	ResourceEnum::None, ResourceEnum::None, ResourceEnum::None,		 0, 0,	{80,20,0},	LOCTEXT("Library Desc", "+100%<img id=\"Science\"/> for surrounding Houses (effect doesn't stack)")),
	BldInfo(CardEnum::School,		LOCTEXT("School", "School"),	LOCTEXT("School (Plural)", "Schools"),		WorldTile2(4, 7),	ResourceEnum::None, ResourceEnum::None, ResourceEnum::None,		 0, 0,	{80,50,10},	LOCTEXT("School Desc", "+120%<img id=\"Science\"/> for surrounding Houses (effect doesn't stack)")),

	BldInfo(CardEnum::Theatre,		LOCTEXT("Theatre", "Theatre"),	LOCTEXT("Theatre (Plural)", "Theatres"),				WorldTile2(7, 6),	ResourceEnum::None, ResourceEnum::None, ResourceEnum::None,		 0, 0,	{80,50,20},	LOCTEXT("Theatre Desc", "Increase visitor's Fun. Base Service quality 70.")),
	BldInfo(CardEnum::Tavern,		LOCTEXT("Tavern", "Tavern"),	LOCTEXT("Tavern (Plural)", "Taverns"),				WorldTile2(5, 5),	ResourceEnum::None, ResourceEnum::None, ResourceEnum::None,		 0, 0,	{50,30,0},	LOCTEXT("Tavern Desc", "Increase visitor's Fun. Base Service quality 50.")),

	BldInfo(CardEnum::Tailor,		LOCTEXT("Tailor", "Tailor"),	LOCTEXT("Tailor (Plural)", "Tailors"),				WorldTile2(5, 6),	ResourceEnum::Leather, ResourceEnum::None, ResourceEnum::Cloth,	 10, 4,	{120, 100, 30},	LOCTEXT("Tailor Desc", "Make Clothes from Leather or Wool.")),

	BldInfo(CardEnum::CharcoalMaker,LOCTEXT("Charcoal Burner", "Charcoal Burner"), LOCTEXT("Charcoal Burner (Plural)", "Charcoal Burners"),		WorldTile2(4, 5),	ResourceEnum::Wood, ResourceEnum::None, ResourceEnum::Coal,		 10, 2,	{40,0,0},		LOCTEXT("Charcoal Burner Desc", "Burn Wood into Coal which provides x2 heat when heating houses.")),
	BldInfo(CardEnum::BeerBrewery,	LOCTEXT("Beer Brewery", "Beer Brewery"),		LOCTEXT("Beer Brewery (Plural)", "Beer Breweries"),			WorldTile2(5, 5),	ResourceEnum::Wheat, ResourceEnum::None, ResourceEnum::Beer,		 10, 2,	{40,30,0},		LOCTEXT("Beer Brewery Desc", "Brew Wheat into Beer.")),
	BldInfo(CardEnum::ClayPit,		LOCTEXT("Claypit", "Claypit"),					LOCTEXT("Claypit (Plural)", "Claypits"),				WorldTile2(6, 6),	ResourceEnum::None, ResourceEnum::None, ResourceEnum::Clay,		 10, 2,	{20,20,0},		LOCTEXT("Claypit Desc", "Produce Clay, used to make Pottery or Brick. Must be built next to a river.")),
	BldInfo(CardEnum::Potter,		LOCTEXT("Potter", "Potter"),					LOCTEXT("Potter (Plural)", "Potters"),				WorldTile2(5, 4),	ResourceEnum::Clay, ResourceEnum::None, ResourceEnum::Pottery,		 10, 2,	{20,40,0},		LOCTEXT("Potter Desc", "Make Pottery from Clay.")),
	BldInfo(CardEnum::HolySlimeRanch,INVTEXT("Holy Slime Ranch"),					FText(),		WorldTile2(10, 10),	ResourceEnum::None, ResourceEnum::None, ResourceEnum::None,		 10, 0,	{30,0,0},		INVTEXT("Raise holy slime. Bonus from nearby slime ranches/pyramid")),

	BldInfo(CardEnum::TradingPost,	LOCTEXT("Trading Post", "Trading Post"),		LOCTEXT("Trading Post (Plural)", "Trading Posts"),			WorldTile2(8, 8),	ResourceEnum::None, ResourceEnum::None, ResourceEnum::None,		 0, 0,	{50, 50, 0},	LOCTEXT("Trading Post Desc", "Trade resources with world market.")),
	BldInfo(CardEnum::TradingCompany,LOCTEXT("Trading Company", "Trading Company"), LOCTEXT("Trading Company (Plural)", "Trading Companies"),		WorldTile2(6, 6),	ResourceEnum::None, ResourceEnum::None, ResourceEnum::None,		 0, 0,	{30, 30, 0},	LOCTEXT("Trading Company Desc", "Automatically trade resources with world market at lower fees.")),
	BldInfo(CardEnum::TradingPort,	LOCTEXT("Trading Port", "Trading Port"),		LOCTEXT("Trading Port (Plural)", "Trading Ports"),			WorldTile2(10, 9),	ResourceEnum::None, ResourceEnum::None, ResourceEnum::None,		 0, 0,	{50, 50, 0},	LOCTEXT("Trading Port Desc", "Trade resources with world market. Must be built on the coast.")),
	BldInfo(CardEnum::CardMaker,		LOCTEXT("Scholars Office", "Scholars Office"), LOCTEXT("Scholars Office (Plural)", "Scholars Offices"),		WorldTile2(5, 5),	ResourceEnum::Paper, ResourceEnum::None, ResourceEnum::None,		 0, 2,	{ 100, 100, 100 },	LOCTEXT("Scholars Office Desc", "Craft a Card from Paper.")),
	BldInfo(CardEnum::ImmigrationOffice,	LOCTEXT("Immigration Office", "Immigration Office"), LOCTEXT("Immigration Office (Plural)", "Immigration Offices"), WorldTile2(5, 6),	ResourceEnum::None, ResourceEnum::None, ResourceEnum::None,		 0, 2,	{50, 0, 0},	LOCTEXT("Immigration Office Desc", "Attract new immigrants.")),

	BldInfo(CardEnum::ThiefGuild,	LOCTEXT("Thief Guild", "Thief Guild"),	LOCTEXT("Thief Guild (Plural)", "Thief Guilds"),			WorldTile2(5, 7),	ResourceEnum::None, ResourceEnum::None, ResourceEnum::None,		 0, 0,	{50,0,0},	FText()),
	BldInfo(CardEnum::SlimePyramid,	LOCTEXT("Pyramid", "Pyramid"),			LOCTEXT("Pyramid (Plural)", "Pyramids"),				WorldTile2(14, 14),	ResourceEnum::None, ResourceEnum::None, ResourceEnum::None,		 0, 0,	{50,0,0},	FText()),

	BldInfo(CardEnum::LovelyHeartStatue, INVTEXT("LovelyHeartStatue"),		FText(), WorldTile2(4, 4),	ResourceEnum::None, ResourceEnum::None, ResourceEnum::None,		 0, 0,	{0,0,0},	INVTEXT("Mysterious giant hearts.")),

	BldInfo(CardEnum::HuntingLodge,	LOCTEXT("Hunting Lodge", "Hunting Lodge"), LOCTEXT("Hunting Lodge (Plural)", "Hunting Lodges"),		WorldTile2(4, 5),	ResourceEnum::None, ResourceEnum::None, ResourceEnum::None,		 0, 2,	{30,0,0},	LOCTEXT("Hunting Lodge Desc", "Hunt wild animals for food.")),
	BldInfo(CardEnum::RanchBarn,	INVTEXT("Ranch Barn"),						FText(),			WorldTile2(4, 4),	ResourceEnum::None, ResourceEnum::None, ResourceEnum::None,		 0, 1,	{50,20,0},	FText()),

	BldInfo(CardEnum::RanchPig,		LOCTEXT("Pig Ranch", "Pig Ranch"),		LOCTEXT("Pig Ranch (Plural)", "Pig Ranches"),			WorldTile2(16, 16),	ResourceEnum::None, ResourceEnum::None, ResourceEnum::None,		 0, 1,	{30,10,0},	LOCTEXT("Pig Ranch Desc", "Rear Pigs for food.")),
	BldInfo(CardEnum::RanchSheep,	LOCTEXT("Sheep Ranch", "Sheep Ranch"),	LOCTEXT("Sheep Ranch (Plural)", "Sheep Ranches"),			WorldTile2(16, 16),	ResourceEnum::None, ResourceEnum::None, ResourceEnum::None,		 0, 1,	{30,20,0},	LOCTEXT("Sheep Ranch Desc", "Rear Sheep for food and Wool.")),
	BldInfo(CardEnum::RanchCow,		LOCTEXT("Cattle Ranch", "Cattle Ranch"), LOCTEXT("Cattle Ranch (Plural)", "Cattle Ranches"),			WorldTile2(16, 16),	ResourceEnum::None, ResourceEnum::None, ResourceEnum::None,	0, 1,	{30,10,10},	LOCTEXT("Cattle Ranch Desc", "Rear Dairy Cows for Milk")),

	
	BldInfo(CardEnum::GoldSmelter,	LOCTEXT("Gold Smelter", "Gold Smelter"), LOCTEXT("Gold Smelter (Plural)", "Gold Smelters"),			WorldTile2(5, 6),	ResourceEnum::Coal,ResourceEnum::GoldOre,ResourceEnum::GoldBar,	 10, 5,	{120,120,0},	LOCTEXT("Gold Smelter Desc", "Smelt Gold Ores into Gold Bars.")),
	BldInfo(CardEnum::Mint,			LOCTEXT("Mint", "Mint"),				LOCTEXT("Mint (Plural)", "Mints"),					WorldTile2(4, 6),	ResourceEnum::GoldBar,ResourceEnum::None,ResourceEnum::None,		 0, 2,	{120,120,0},	LOCTEXT("Mint Desc", "Mint Gold Bars into <img id=\"Coin\"/>.")),

	BldInfo(CardEnum::BarrackClubman,	LOCTEXT("Clubman Barracks", "Clubman Barracks"), LOCTEXT("Clubman Barracks (Plural)", "Clubman Barracks"),	WorldTile2(7, 7),	ResourceEnum::None, ResourceEnum::None, ResourceEnum::None,		0, 2,	{30,0,0},	LOCTEXT("Clubman Barracks Desc", "Train Clubmen.")),
	BldInfo(CardEnum::BarrackSwordman,	LOCTEXT("Knight Barracks", "Knight Barracks"), LOCTEXT("Knight Barracks (Plural)", "Knight Barracks"),	WorldTile2(7, 7),	ResourceEnum::Iron, ResourceEnum::None, ResourceEnum::None,		0, 4,	{80,30,30},	LOCTEXT("Knight Barracks Desc", "Consume Iron to increase <img id=\"Influence\"/>.")),
	BldInfo(CardEnum::BarrackArcher,	LOCTEXT("Archer Barracks", "Archer Barracks"), LOCTEXT("Archer Barracks (Plural)", "Archer Barracks"),	WorldTile2(7, 7),	ResourceEnum::Wood, ResourceEnum::None, ResourceEnum::None,		0, 2,	{50,30,0},	LOCTEXT("Archer Barracks Desc", "Consume Wood to increase <img id=\"Influence\"/>.")),

	BldInfo(CardEnum::ShrineWisdom,	LOCTEXT("Shrine of Wisdom", "Shrine of Wisdom"),	LOCTEXT("Shrine of Wisdom (Plural)", "Shrines of Wisdom"),		WorldTile2(4, 4),	ResourceEnum::None,ResourceEnum::None,ResourceEnum::None,	 0, 0,	{0, 50, 0},	INVTEXT("+1 Wild Card to the deck.")),
	BldInfo(CardEnum::ShrineLove,	LOCTEXT("Shrine of Love", "Shrine of Love"),		LOCTEXT("Shrine of Love (Plural)", "Shrines of Love"),		WorldTile2(4, 4),	ResourceEnum::None,ResourceEnum::None,ResourceEnum::None,		 0, 0,	{0, 80, 10},	INVTEXT("+5<img id=\"Smile\"/> to surrounding houses.")),
	BldInfo(CardEnum::ShrineGreed,	LOCTEXT("Shrine of Greed", "Shrine of Greed"),		LOCTEXT("Shrine of Greed (Plural)", "Shrines of Greed"),		WorldTile2(4, 4),	ResourceEnum::None,ResourceEnum::None,ResourceEnum::None,		 0, 0,	{0, 80, 10},	INVTEXT("+20<img id=\"Coin\"/> per round and -5<img id=\"Smile\"/> to surrounding houses.")),


	BldInfo(CardEnum::HellPortal,	LOCTEXT("Hell Portal", "Hell Portal"),			LOCTEXT("Hell Portal (Plural)", "Hell Portals"),			WorldTile2(6, 6),	ResourceEnum::None, ResourceEnum::None, ResourceEnum::None,		0, 0,	{100,100,50},	INVTEXT("Open a portal to hell to harvest its riches. Gain 200 <img id=\"Coin\"/> each round, but may explode letting demons through."), INVTEXT("Gain 200 <img id=\"Coin\"/> each round, but may explode letting demons through.")),
	BldInfo(CardEnum::LaborerGuild,	LOCTEXT("Laborer's Guild", "Laborer's Guild"),				LOCTEXT("Laborer's Guild (Plural)", "Laborer's Guilds"),	WorldTile2(4, 6),		ResourceEnum::None, ResourceEnum::None, ResourceEnum::None,		0, 0,	{20, 0, 0},	LOCTEXT("Laborer's Guild Desc", "Dispatch laborers to haul resources and cut trees. Laborers from the guild has x2 inventory carry capacity.")),

	BldInfo(CardEnum::HumanitarianAidCamp,	LOCTEXT("Humanitarian Aid Camp", "Humanitarian Aid Camp"),	LOCTEXT("Humanitarian Aid Camp (Plural)", "Humanitarian Aid Camps"),	WorldTile2(4, 4), ResourceEnum::None, ResourceEnum::None, ResourceEnum::None, 0, 0, {0, 0, 0}, LOCTEXT("Humanitarian Aid Camp Desc", "Supply your neighbor with 100 food.")),

	BldInfo(CardEnum::RegionTribalVillage, LOCTEXT("Tribal Village", "Tribal Village"), LOCTEXT("Tribal Village (Plural)", "Tribal Villages"), WorldTile2(9, 9), ResourceEnum::None, ResourceEnum::None, ResourceEnum::None, 0, 0, {0, 0, 0}, LOCTEXT("Tribal Village Desc", "Tribal village that might want to join your city.")),
	BldInfo(CardEnum::RegionShrine, LOCTEXT("Ancient Shrine", "Ancient Shrine"),		LOCTEXT("Ancient Shrine (Plural)", "Ancient Shrines"), WorldTile2(4, 6), ResourceEnum::None, ResourceEnum::None, ResourceEnum::None, 0, 0, {0, 0, 0}, LOCTEXT("Ancient Shrine Desc", "Shrines built by the ancients that grants wisdom to those who seek it.")),
	BldInfo(CardEnum::RegionPort,	LOCTEXT("Port Settlement", "Port Settlement"),		LOCTEXT("Port Settlement (Plural)", "Port Settlements"), WorldTile2(12, 9), ResourceEnum::None, ResourceEnum::None, ResourceEnum::None, 0, 0, {0, 0, 0}, INVTEXT("Port settlement specialized in trading certain resources.")),
	BldInfo(CardEnum::RegionCrates, LOCTEXT("Crates", "Crates"),						LOCTEXT("Crates (Plural)", "Crates"), WorldTile2(4, 6), ResourceEnum::None, ResourceEnum::None, ResourceEnum::None, 0, 0, {0, 0, 0}, LOCTEXT("Crates Desc", "Crates that may contain valuable resources.")),

	// June 1 addition
	BldInfo(CardEnum::Windmill, LOCTEXT("Windmill", "Windmill"),				LOCTEXT("Windmill (Plural)", "Windmills"), WorldTile2(5, 5), ResourceEnum::Wheat, ResourceEnum::None, ResourceEnum::Flour, 10, 2, { 150,0,0 }, LOCTEXT("Windmill Desc", "Grinds Wheat into Wheat Flour. +10% Productivity to surrounding Farms.")),
	BldInfo(CardEnum::Bakery,	LOCTEXT("Bakery", "Bakery"),					LOCTEXT("Bakery (Plural)", "Bakeries"), WorldTile2(5, 5), ResourceEnum::Flour, ResourceEnum::Coal, ResourceEnum::Bread, 30, 2, { 70,30,0 }, LOCTEXT("Bakery Desc", "Bakes Bread with Wheat Flour and heat.")),
	BldInfo(CardEnum::GemstoneMine, LOCTEXT("Gemstone Mine", "Gemstone Mine"),	LOCTEXT("Gemstone Mine (Plural)", "Gemstone Mines"), WorldTile2(5, 5), ResourceEnum::None, ResourceEnum::None, ResourceEnum::Gemstone, 10, 3, { 70,30,0 }, LOCTEXT("Gemstone Mine Desc", "Mine Gemstone from Gemstone Deposits.")),
	BldInfo(CardEnum::Jeweler, LOCTEXT("Jeweler", "Jeweler"),					LOCTEXT("Jeweler (Plural)", "Jewelers"), WorldTile2(4, 7), ResourceEnum::Gemstone, ResourceEnum::GoldBar, ResourceEnum::Jewelry, 10, 3, { 150, 70, 70 }, LOCTEXT("Jeweler Desc", "Craft Gemstone and Gold Bar into Jewelry.")),

	// June 9 addition
	BldInfo(CardEnum::Beekeeper, LOCTEXT("Beekeeper", "Beekeeper"),				LOCTEXT("Beekeeper (Plural)", "Beekeepers"), WorldTile2(6, 9), ResourceEnum::None, ResourceEnum::None, ResourceEnum::Beeswax, 10, 2, { 50,50,0 }, LOCTEXT("Beekeeper Desc", "Produces Beeswax and Honey. Efficiency increases with more surrounding trees.")),
	BldInfo(CardEnum::Brickworks, LOCTEXT("Brickworks", "Brickworks"),			LOCTEXT("Brickworks (Plural)", "Brickworks"), WorldTile2(6, 5), ResourceEnum::Clay, ResourceEnum::Coal, ResourceEnum::Brick, 20, 3, { 20,100,0 }, LOCTEXT("Brickworks Desc", "Produces Brick from Clay and Coal.")),
	BldInfo(CardEnum::CandleMaker, LOCTEXT("Candle Maker", "Candle Maker"),		LOCTEXT("Candle Maker (Plural)", "Candle Makers"), WorldTile2(5, 6), ResourceEnum::Beeswax, ResourceEnum::Cotton, ResourceEnum::Candle, 20, 3, { 150, 100, 0 }, LOCTEXT("Candle Maker Desc", "Make Candles from Beeswax and Cotton wicks.")),
	BldInfo(CardEnum::CottonMill, LOCTEXT("Cotton Mill", "Cotton Mill"),		LOCTEXT("Cotton Mill (Plural)", "Cotton Mills"), WorldTile2(7, 6), ResourceEnum::Cotton, ResourceEnum::None, ResourceEnum::CottonFabric, 10, 5, { 0, 100, 100 }, LOCTEXT("Cotton Mill Desc", "Mass-produce Cotton into Cotton Fabric.")),
	BldInfo(CardEnum::PrintingPress, LOCTEXT("Printing Press", "Printing Press"), LOCTEXT("Printing Press (Plural)", "Printing Presses"), WorldTile2(5, 6), ResourceEnum::Paper, ResourceEnum::Dye, ResourceEnum::Book, 20, 5, { 0, 150, 100 }, LOCTEXT("Printing Press Desc", "Print Books.")),

	// June 25 addition
	BldInfo(CardEnum::Warehouse, LOCTEXT("Warehouse", "Warehouse"),	LOCTEXT("Warehouse (Plural)", "Warehouses"), WorldTile2(6, 4), ResourceEnum::None, ResourceEnum::None, ResourceEnum::None, 0, 0, { 70,70,0 }, LOCTEXT("Warehouse Desc", "Advanced storage with 30 storage slots.")),
	BldInfo(CardEnum::Fort, LOCTEXT("Fort", "Fort"),				LOCTEXT("Fort (Plural)", "Forts"), WorldTile2(9, 9), ResourceEnum::None, ResourceEnum::None, ResourceEnum::None, 0, 0, { 0,0,0 }, LOCTEXT("Fort Desc", "+100% province's defense.")),
	BldInfo(CardEnum::ResourceOutpost, LOCTEXT("Resource Outpost", "Resource Outpost"),	LOCTEXT("Resource Outpost (Plural)", "Resource Outposts"), WorldTile2(10, 10), ResourceEnum::None, ResourceEnum::None, ResourceEnum::None, 0, 0, { 0,0,0 }, LOCTEXT("Resource Outpost Desc", "Extract resource from province.")),
	BldInfo(CardEnum::InventorsWorkshop, LOCTEXT("Inventor's Workshop", "Inventor's Workshop"), LOCTEXT("Inventor's Workshop (Plural)", "Inventor's Workshops"), WorldTile2(6, 6), ResourceEnum::Iron, ResourceEnum::None, ResourceEnum::None, 0, 2, { 50,50,0 }, LOCTEXT("Inventor's Workshop Desc", "Generate Science Points. Use Iron Bars as input.")),
	BldInfo(CardEnum::IntercityRoad, LOCTEXT("Intercity Road", "Intercity Road"),				LOCTEXT("Intercity Road (Plural)", "Intercity Road"), WorldTile2(1, 1), ResourceEnum::None, ResourceEnum::None, ResourceEnum::None, 0, 0, { 0,0,0 }, LOCTEXT("Intercity Road Desc", "Build Road to connect with other Cities. Same as Dirt Road, but buildable outside your territory.")),

	// August 16
	BldInfo(CardEnum::FakeTownhall, INVTEXT("FakeTownhall"),			FText(), WorldTile2(12, 12), ResourceEnum::None, ResourceEnum::None, ResourceEnum::None, 0, 0, { 0,0,0 }, FText()),
	BldInfo(CardEnum::FakeTribalVillage, INVTEXT("FakeTribalVillage"),	FText(), WorldTile2(12, 12), ResourceEnum::None, ResourceEnum::None, ResourceEnum::None, 0, 0, { 0,0,0 }, FText()),
	BldInfo(CardEnum::ChichenItza, LOCTEXT("Chichen Itza", "Chichen Itza"), FText(), WorldTile2(16, 16), ResourceEnum::None, ResourceEnum::None, ResourceEnum::None, 0, 0, { 0,0,0 }, FText()),

	// October 20
	BldInfo(CardEnum::Market,				LOCTEXT("Market", "Market"),							LOCTEXT("Market (Plural)", "Markets"), WorldTile2(6, 12), ResourceEnum::None, ResourceEnum::None, ResourceEnum::None, 0, 3, { 120, 120, 0 }, LOCTEXT("Market Desc", "Supplies anything a household needs within its radius. Workers carry 50 units.")),
	BldInfo(CardEnum::ShippingDepot,		LOCTEXT("Logistics Office", "Logistics Office"),		LOCTEXT("Logistics Office (Plural)", "Logistics Offices"), WorldTile2(4, 4), ResourceEnum::None, ResourceEnum::None, ResourceEnum::None, 0, 1, { 20, 10, 0 }, LOCTEXT("Logistics Office Desc", "Haul specified resources from within the radius to its delivery target in 50-units bulk.")),
	BldInfo(CardEnum::IrrigationReservoir, LOCTEXT("Irrigation Reservoir", "Irrigation Reservoir"), LOCTEXT("Irrigation Reservoir (Plural)", "Irrigation Reservoirs"), WorldTile2(5, 5), ResourceEnum::None, ResourceEnum::None, ResourceEnum::None, 0, 0, { 0, 150, 0 }, LOCTEXT("Irrigation Reservoir Desc", "Raises fertility within its radius to 100%.")),

	// November 18
	BldInfo(CardEnum::Tunnel,		LOCTEXT("Tunnel", "Tunnel"),					LOCTEXT("Tunnel (Plural)", "Tunnels"), WorldTile2(1, 1), ResourceEnum::None, ResourceEnum::None, ResourceEnum::None, 0, 0, { 0,0,0 }, LOCTEXT("Tunnel Desc", "Allow citizens to walk through mountain.")),
	BldInfo(CardEnum::GarmentFactory, LOCTEXT("Garment Factory", "Garment Factory"), LOCTEXT("Garment Factory (Plural)", "Garment Factories"), WorldTile2(7, 6), ResourceEnum::DyedCottonFabric, ResourceEnum::None, ResourceEnum::LuxuriousClothes, 10, 5, { 0, 100, 100 }, LOCTEXT("Garment Factory Desc", "Mass-produce Clothes with Fabrics.")),

	// December 29
	BldInfo(CardEnum::ShroomFarm,		LOCTEXT("Shroom Farm", "Shroom Farm"),			LOCTEXT("Shroom Farm (Plural)", "Shroom Farms"), WorldTile2(8, 8), ResourceEnum::Wood, ResourceEnum::None, ResourceEnum::Shroom, 20, 2, { 70,70,0 }, LOCTEXT("Shroom Farm Desc", "Farm Shroom using wood.")),
	BldInfo(CardEnum::VodkaDistillery, LOCTEXT("Vodka Distillery", "Vodka Distillery"), LOCTEXT("Vodka Distillery (Plural)", "Vodka Distilleries"), WorldTile2(5, 5), ResourceEnum::Potato, ResourceEnum::None, ResourceEnum::Vodka, 10, 2, { 120,120,0 }, LOCTEXT("Vodka Distillery Desc", "Brew Potato into Vodka.")),
	BldInfo(CardEnum::CoffeeRoaster, LOCTEXT("Coffee Roaster", "Coffee Roaster"),		LOCTEXT("Coffee Roaster (Plural)", "Coffee Roasters"), WorldTile2(6, 6), ResourceEnum::RawCoffee, ResourceEnum::None, ResourceEnum::Coffee, 10, 2, { 150, 100, 0 }, LOCTEXT("Coffee Roaster Desc", "Roast Raw Coffee into Coffee.")),

	// February 2
	BldInfo(CardEnum::Colony, LOCTEXT("Colony", "Colony"), LOCTEXT("Colony (Plural)", "Colonies"), WorldTile2(12, 12), ResourceEnum::None, ResourceEnum::None, ResourceEnum::None, 0, 0, { 0,0,0 }, LOCTEXT("Colony Desc", "Build a new city with 10 citizens from your capital.")),
	BldInfo(CardEnum::PortColony, LOCTEXT("Port Colony", "Port Colony"), LOCTEXT("Port Colony (Plural)", "Port Colonies"), WorldTile2(12, 12), ResourceEnum::None, ResourceEnum::None, ResourceEnum::None, 0, 0, { 0,0,0 }, LOCTEXT("Port Colony Desc", "Build a new port city with 10 citizens from your capital.")),
	BldInfo(CardEnum::IntercityLogisticsHub, LOCTEXT("Intercity Logistics Hub", "Intercity Logistics Hub"), LOCTEXT("Intercity Logistics Hub (Plural)", "Intercity Logistics Hubs"), WorldTile2(6, 6), ResourceEnum::None, ResourceEnum::None, ResourceEnum::None, 0, 1, { 50,20,0 }, LOCTEXT("Intercity Logistics Hub Desc", "Bring resources from another city (land).")),
	BldInfo(CardEnum::IntercityLogisticsPort, LOCTEXT("Intercity Logistics Port", "Intercity Logistics Port"), LOCTEXT("Intercity Logistics Port (Plural)", "Intercity Logistics Ports"), WorldTile2(12, 6), ResourceEnum::None, ResourceEnum::None, ResourceEnum::None, 0, 1, { 80,20,0 }, LOCTEXT("Intercity Logistics Port Desc", "Bring resources from another city (water).")),
	BldInfo(CardEnum::IntercityBridge, LOCTEXT("Intercity Bridge", "Intercity Bridge"), LOCTEXT("Intercity Bridge (Plural)", "Intercity Bridges"), WorldTile2(1, 1), ResourceEnum::None, ResourceEnum::None, ResourceEnum::None, 0, 0, { 0,0,0 }, LOCTEXT("Intercity Bridge Desc", "Bridge that can be built outside your territory to connect Cities.")),

	// March 12
	BldInfo(CardEnum::Granary, LOCTEXT("Granary", "Granary"), LOCTEXT("Granary (Plural)", "Granaries"), WorldTile2(6, 6), ResourceEnum::None, ResourceEnum::None, ResourceEnum::None, 0, 0, { 50, 50, 0 }, LOCTEXT("Granary Desc", "Food Storage. +25% Productivity to surrounding Food Producers.")),
	BldInfo(CardEnum::Archives, LOCTEXT("Archives", "Archives"), LOCTEXT("Archives (Plural)", "Archives"), WorldTile2(6, 6), ResourceEnum::None, ResourceEnum::None, ResourceEnum::None, 0, 0, { 50, 50, 0 }, LOCTEXT("Archives Desc", "Store any Cards. Additionally, earn Income equals to 24% of the Card Price per year.")),
	BldInfo(CardEnum::HaulingServices, LOCTEXT("Hauling Services", "Hauling Services"), LOCTEXT("Hauling Services (Plural)", "Hauling Services"), WorldTile2(6, 5), ResourceEnum::None, ResourceEnum::None, ResourceEnum::None, 0, 3, { 50, 0, 0 }, LOCTEXT("Hauling Services Desc", "Workers use carts to haul resources to fill building inputs or clear building outputs.")),

	// Apr 1
	BldInfo(CardEnum::SandMine, LOCTEXT("SandMine", "Sand Mine"), LOCTEXT("Sand Mine (Plural)", "Sand Mines"), WorldTile2(6, 8), ResourceEnum::None, ResourceEnum::None, ResourceEnum::Sand, 10, 0, { 50, 50, 0 }, LOCTEXT("Sand Mine Desc", "Extract Sand from beach or river. Sand can be used to make Glass.")),
	BldInfo(CardEnum::Glassworks, LOCTEXT("Glassworks", "Glassworks"), LOCTEXT("Glassworks (Plural)", "Glassworks"), WorldTile2(8, 8), ResourceEnum::Sand, ResourceEnum::Coal, ResourceEnum::Glass, 10, 0, { 50, 50, 0 }, LOCTEXT("Glassworks Desc", "Produce Glass from Sand and Coal.")),
	BldInfo(CardEnum::ConcreteFactory, LOCTEXT("ConcreteFactory", "Concrete Factory"), LOCTEXT("Concrete Factory (Plural)", "Concrete Factories"), WorldTile2(8, 8), ResourceEnum::Stone, ResourceEnum::Sand, ResourceEnum::Concrete, 10, 0, { 50, 50, 0 }, LOCTEXT("Concrete Factory Desc", "Make Concrete from Stone and Sand.")),
	BldInfo(CardEnum::CoalPowerPlant, LOCTEXT("CoalPowerPlant", "Coal Power Plant"), LOCTEXT("CoalPowerPlant (Plural)", "Coal Power Plants"), WorldTile2(8, 8), ResourceEnum::None, ResourceEnum::None, ResourceEnum::None, 0, 0, { 50, 50, 0 }, LOCTEXT("Coal Power Plants Desc", "Provide nearby Buildings with Electricity from Coal.")),
	BldInfo(CardEnum::Steelworks, LOCTEXT("Steelworks", "Steelworks"), LOCTEXT("Steelworks (Plural)", "Steelworks"), WorldTile2(8, 8), ResourceEnum::Iron, ResourceEnum::Coal, ResourceEnum::SteelBeam, 0, 10, { 50, 50, 0 }, LOCTEXT("Steelworks Desc", "Produce Steel Beam from Iron Bars and Coal.")),
	BldInfo(CardEnum::StoneToolsShop, LOCTEXT("StoneToolsShop", "Stone Tools Shop"), LOCTEXT("StoneToolsShop (Plural)", "Stone Tools Shops"), WorldTile2(8, 8), ResourceEnum::Stone, ResourceEnum::Wood, ResourceEnum::StoneTools, 10, 0, { 50, 50, 0 }, LOCTEXT("Stone Tools Shop Desc", "Make Stone Tools from Stone and Wood.")),
	BldInfo(CardEnum::OilRig, LOCTEXT("Oil Rig", "Oil Rig"), LOCTEXT("Oil Rig (Plural)", "Oil Rigs"), WorldTile2(8, 8), ResourceEnum::None, ResourceEnum::None, ResourceEnum::Oil, 10, 0, { 50, 50, 0 }, LOCTEXT("Oil Rig Desc", "Extract Oil from Oil Well.")),
	BldInfo(CardEnum::OilPowerPlant, LOCTEXT("OilPowerPlant", "Oil Power Plant"), LOCTEXT("Oil Power Plant (Plural)", "Oil Power Plants"), WorldTile2(8, 8), ResourceEnum::None, ResourceEnum::None, ResourceEnum::None, 0, 0, { 50, 50, 0 }, LOCTEXT("Oil Power Plant Desc", "Provide nearby Buildings with Electricity from Oil.")),
	BldInfo(CardEnum::PaperMill, LOCTEXT("PaperMill", "Paper Mill"), LOCTEXT("Paper Mill (Plural)", "Paper Mills"), WorldTile2(8, 8), ResourceEnum::Wood, ResourceEnum::None, ResourceEnum::None, 30, 0, { 50, 50, 0 }, LOCTEXT("Paper Mill Desc", "Mass-produce Paper from Wood.")),
	BldInfo(CardEnum::ClockMakers, LOCTEXT("ClockMakers", "Clock Makers"), LOCTEXT("Clock Makers (Plural)", "Clock Makers"), WorldTile2(8, 8), ResourceEnum::Glass, ResourceEnum::GoldBar, ResourceEnum::PocketWatch, 10, 0, { 50, 50, 0 }, LOCTEXT("Clock Makers Desc", "Craft Pocket Watch from Glass and Gold Bars.")),

	BldInfo(CardEnum::Cathedral, LOCTEXT("Cathedral", "Cathedral"), LOCTEXT("Cathedral (Plural)", "Cathedrals"), WorldTile2(8, 8), ResourceEnum::None, ResourceEnum::None, ResourceEnum::None, 0, 0, { 50, 50, 0 }, LOCTEXT("Cathedral Desc", "Extract Sand from beach or river. Sand can be used to make Glass.")),
	BldInfo(CardEnum::Castle, LOCTEXT("Castle", "Castle"), LOCTEXT("Castle (Plural)", "Castles"), WorldTile2(8, 8), ResourceEnum::None, ResourceEnum::None, ResourceEnum::None, 0, 0, { 50, 50, 0 }, LOCTEXT("Castle Desc", "Extract Sand from beach or river. Sand can be used to make Glass.")),
	BldInfo(CardEnum::GrandMuseum, LOCTEXT("GrandMuseum", "Grand Museum"), LOCTEXT("Grand Museum (Plural)", "Grand Museums"), WorldTile2(8, 8), ResourceEnum::None, ResourceEnum::None, ResourceEnum::None, 0, 0, { 50, 50, 0 }, LOCTEXT("Grand Museum Desc", "Extract Sand from beach or river. Sand can be used to make Glass.")),
	BldInfo(CardEnum::ExhibitionHall, LOCTEXT("ExhibitionHall", "Exhibition Hall"), LOCTEXT("Exhibition Hall (Plural)", "Exhibition Halls"), WorldTile2(8, 8), ResourceEnum::None, ResourceEnum::None, ResourceEnum::None, 0, 0, { 50, 50, 0 }, LOCTEXT("Exhibition Hall Desc", "Extract Sand from beach or river. Sand can be used to make Glass.")),

	
	// Decorations
	BldInfo(CardEnum::FlowerBed,		LOCTEXT("Flower Bed", "Flower Bed"),		LOCTEXT("Flower Bed (Plural)", "Flower Beds"), WorldTile2(1, 1), ResourceEnum::None, ResourceEnum::None, ResourceEnum::None, 0, 0, { 0,0,0 }, LOCTEXT("Flower Bed Desc", "Increase the surrounding appeal by 5 within 5 tiles radius.")),
	BldInfo(CardEnum::GardenShrubbery1, LOCTEXT("Shrubbery", "Shrubbery"),			LOCTEXT("Shrubbery (Plural)", "Shrubberies"), WorldTile2(1, 1), ResourceEnum::None, ResourceEnum::None, ResourceEnum::None, 0, 0, { 0,0,0 }, LOCTEXT("Shrubbery Desc", "Increase the surrounding appeal by 5 within 5 tiles radius.")),
	BldInfo(CardEnum::GardenCypress,	LOCTEXT("Garden Cypress", "Garden Cypress"), LOCTEXT("Garden Cypress (Plural)", "Garden Cypresses"), WorldTile2(1, 1), ResourceEnum::None, ResourceEnum::None, ResourceEnum::None, 0, 0, { 0,0,0 }, LOCTEXT("Garden Cypress Desc", "Increase the surrounding appeal by 8 within 5 tiles radius.")),	
	
	// Rare cards
	//BldInfo("Sales Office",			WorldTile2(4, 5),		ResourceEnum::None, ResourceEnum::None, ResourceEnum::None,		0,	{20, 20, 0},	"Decrease the amount of trade fee by 50%."),
	BldInfo(CardEnum::ArchitectStudio,		LOCTEXT("Architect's Studio", "Architect's Studio"),		LOCTEXT("Architect's Studio (Plural)", "Architect's Studios"), WorldTile2(4, 5),		ResourceEnum::None, ResourceEnum::None, ResourceEnum::None,		0, 0,	{30, 30, 0}, LOCTEXT("Architect's Studio Desc", "+5% housing appeal.")),
	BldInfo(CardEnum::EngineeringOffice,	LOCTEXT("Engineer's Office", "Engineer's Office"),			LOCTEXT("Engineer's Office (Plural)", "Engineer's Offices"), WorldTile2(4, 5),		ResourceEnum::None, ResourceEnum::None, ResourceEnum::None,		0, 0,	{30, 30, 0}, LOCTEXT("Engineer's Office Desc", "+10% industrial production.")),
	BldInfo(CardEnum::DepartmentOfAgriculture, LOCTEXT("Ministry of Agriculture", "Ministry of Agriculture"), FText(), WorldTile2(4, 5),		ResourceEnum::None, ResourceEnum::None, ResourceEnum::None,		0, 0,	{30, 30, 0}, LOCTEXT("Ministry of Agriculture Desc", "+5% Farm production when there are 8 or more Farms.")),
	//BldInfo("Isolationist Publisher", WorldTile2(4, 5),		ResourceEnum::None, ResourceEnum::None, ResourceEnum::None,		0,	{20, 20, 0},	"Double trade fees. -10% food consumption."),

	BldInfo(CardEnum::StockMarket,			LOCTEXT("Stock Market", "Stock Market"),					FText(), WorldTile2(4, 5),		ResourceEnum::None, ResourceEnum::None, ResourceEnum::None,		0, 0,	{30, 30, 0}, LOCTEXT("Stock Market Desc", "Upkeep for Trading Companies reduced to 1<img id=\"Coin\"/>.")),
	BldInfo(CardEnum::CensorshipInstitute, LOCTEXT("Censorship Institute", "Censorship Institute"),		FText(), WorldTile2(4, 5),		ResourceEnum::None, ResourceEnum::None, ResourceEnum::None,		0, 0,	{30, 30, 0}, LOCTEXT("Censorship Institute Desc", "-1 card from each hand roll. +7% Farm production.")),
	BldInfo(CardEnum::EnvironmentalistGuild, LOCTEXT("Environmentalist Guild", "Environmentalist Guild"), FText(), WorldTile2(4, 5),		ResourceEnum::None, ResourceEnum::None, ResourceEnum::None,		0, 0,	{30, 30, 0}, LOCTEXT("Environmentalist Guild Desc", "+15% house appeal. -30% production for mines, smelters, and Charcoal Makers.")),
	BldInfo(CardEnum::IndustrialistsGuild, LOCTEXT("Industrialist Guild", "Industrialist Guild"),		FText(), WorldTile2(4, 5),		ResourceEnum::None, ResourceEnum::None, ResourceEnum::None,		0, 0,	{50, 50, 0}, LOCTEXT("Industrialist Guild Desc", "+20% production to surrounding industries within 10 tiles radius.")),
	BldInfo(CardEnum::Oracle,			LOCTEXT("Oracle", "Oracle"),									FText(), WorldTile2(4, 5),		ResourceEnum::None, ResourceEnum::None, ResourceEnum::None,		0, 0,	{50, 50, 0}, INVTEXT("Gain one additional choice when selecting Rare Cards.")),
	BldInfo(CardEnum::AdventurersGuild, LOCTEXT("Adventurer's Guild", "Adventurer's Guild"),			FText(), WorldTile2(4, 5),		ResourceEnum::None, ResourceEnum::None, ResourceEnum::None,		0, 0,	{80, 50, 0}, INVTEXT("At the end of summer choose a Rare Card.")),
	
	BldInfo(CardEnum::ConsultingFirm, INVTEXT("Consulting Firm"),							FText(), WorldTile2(4, 5), ResourceEnum::None, ResourceEnum::None, ResourceEnum::None, 0, 0, { 30, 0, 0}, INVTEXT("+30% production to surrounding buildings. -30 <img id=\"Coin\"/> per round")),
	BldInfo(CardEnum::ImmigrationPropagandaOffice, INVTEXT("Immigration Propaganda Office"), FText(), WorldTile2(4, 5), ResourceEnum::None, ResourceEnum::None, ResourceEnum::None, 0, 0, {30, 0, 0}, INVTEXT("+30% immigration")),
	BldInfo(CardEnum::MerchantGuild, INVTEXT("Merchant Guild"),								FText(), WorldTile2(4, 5), ResourceEnum::None, ResourceEnum::None, ResourceEnum::None, 0, 0, { 30, 0, 0}, INVTEXT("-5% trade fee.")),
	BldInfo(CardEnum::OreSupplier, INVTEXT("Ore Supplier Guild"),										FText(), WorldTile2(4, 5), ResourceEnum::None, ResourceEnum::None, ResourceEnum::None, 0, 0, { 30, 0, 0}, INVTEXT("Buy selected ore each season with 0% trading fee.")),
	BldInfo(CardEnum::BeerBreweryFamous, LOCTEXT("Famous Beer Brewery", "Famous Beer Brewery"),			LOCTEXT("Famous Beer Brewery (Plural)", "Famous Beer Breweries"), WorldTile2(5, 7),	ResourceEnum::Wheat, ResourceEnum::None, ResourceEnum::Beer,		 10, 8, {150,90,0}, LOCTEXT("Famous Beer Brewery Desc", "Large brewery with improved productivity. Require 4 Beer Breweries to unlock.")),
	BldInfo(CardEnum::IronSmelterGiant, LOCTEXT("Giant Iron Smelter", "Giant Iron Smelter"),			LOCTEXT("Giant Iron Smelter (Plural)", "Giant Iron Smelters"), WorldTile2(7, 8), ResourceEnum::IronOre, ResourceEnum::Coal, ResourceEnum::Iron, 10, 8, { 150,90,0 }, INVTEXT("Large smelter.")),

	
	BldInfo(CardEnum::Cattery,		LOCTEXT("Cattery", "Cattery"),					LOCTEXT("Cattery (Plural)", "Catteries"), WorldTile2(4, 5), ResourceEnum::None, ResourceEnum::None, ResourceEnum::None, 0, 0, { 30, 30, 0 }, INVTEXT("+15% housing appeal to surrounding houses.")),
	BldInfo(CardEnum::InvestmentBank, LOCTEXT("Investment Bank", "Investment Bank"), LOCTEXT("Investment Bank (Plural)", "Investment Banks"), WorldTile2(4, 5), ResourceEnum::None, ResourceEnum::None, ResourceEnum::None, 0, 0, { 0, 70, 20 }, INVTEXT("Gain <img id=\"Coin\"/> equals to +10% of <img id=\"Coin\"/> every round.")),

	// Unique Cards	
	BldInfo(CardEnum::StatisticsBureau, LOCTEXT("Statistics Bureau", "Statistics Bureau"), FText(), WorldTile2(4, 5), ResourceEnum::None, ResourceEnum::None, ResourceEnum::None, 0, 0, { 30, 0, 0 }, LOCTEXT("Statistics Bureau Desc", "Show Town Statistics.")),
	BldInfo(CardEnum::JobManagementBureau, LOCTEXT("Employment Bureau", "Employment Bureau"), FText(), WorldTile2(4, 5), ResourceEnum::None, ResourceEnum::None, ResourceEnum::None, 0, 0, { 30, 0, 0 }, LOCTEXT("Employment Bureau Desc", "Allow managing job priority (global)."))

	
	// Can no longer pickup cards
	//BldInfo("Necromancer tower",	WorldTile2(4, 5),		ResourceEnum::None, ResourceEnum::None, ResourceEnum::None,		0,	{30, 30, 0},	"All citizens become zombie minions. Happiness becomes irrelevant. Immigration ceased."),
	// Imperialist
};


static const BldInfo CardInfos[]
{
	BldInfo(CardEnum::Investment,		LOCTEXT("Investment", "Investment"), 300, LOCTEXT("Investment Desc", "+<img id=\"Coin\"/>30 income")),
	BldInfo(CardEnum::InstantBuild,		LOCTEXT("Instant Build", "Instant Build"), 200, LOCTEXT("Instant Build Desc", "Use on a construction site to pay x3 the resource cost and instantly build it.")),
	BldInfo(CardEnum::ShrineWisdomPiece,	INVTEXT("OLD"), 200, FText()),
	BldInfo(CardEnum::ShrineLovePiece,	INVTEXT("Shrine Piece: Love"), 200, INVTEXT("Collect 3 shrine pieces to build a shrine.")),
	BldInfo(CardEnum::ShrineGreedPiece,	INVTEXT("Shrine Piece: Greed"), 200, INVTEXT("Collect 3 shrine pieces to build a shrine.")),

	/*
	 * Passives
	 */
	BldInfo(CardEnum::BeerTax,			LOCTEXT("Beer Tax", "Beer Tax"), 200, LOCTEXT("Beer Tax Desc", "Houses with Beer get +5<img id=\"Coin\"/>")),
	BldInfo(CardEnum::HomeBrew,			LOCTEXT("Home Brew", "Home Brew"), 200, LOCTEXT("Home Brew Desc", "Houses with Pottery get +4<img id=\"Science\"/>")),

	BldInfo(CardEnum::MasterBrewer,			LOCTEXT("Master Brewer", "Master Brewer"), 200, LOCTEXT("Master Brewer Desc", "Breweries/Distilleries gain +30% efficiency")),
	BldInfo(CardEnum::MasterPotter,			LOCTEXT("Master Potter", "Master Potter"), 200, LOCTEXT("Master Potter Desc", "Potters gain +20% efficiency")),

	BldInfo(CardEnum::CooperativeFishing,	LOCTEXT("Cooperative Fishing", "Cooperative Fishing"), 200, LOCTEXT("Cooperative Fishing Desc","Every 1% Happiness above 60%, gives +1% productivity to Fishing Lodge")),
	BldInfo(CardEnum::CompaniesAct,			LOCTEXT("Companies Act", "Companies Act"), 200, LOCTEXT("Companies Act Desc", "-10% trade fees for Trading Companies.")),

	BldInfo(CardEnum::ProductivityBook,	LOCTEXT("Productivity Book", "Productivity Book"), 100, LOCTEXT("Productivity Book Desc", "+20% productivity")),
	BldInfo(CardEnum::SustainabilityBook,LOCTEXT("Sustainability Book", "Sustainability Book"), 100, LOCTEXT("Sustainability Book Desc","Consume 40% less input")),
	BldInfo(CardEnum::FrugalityBook,		LOCTEXT("Frugality Book", "Frugality Book"), 100, LOCTEXT("Frugality Book Desc",  "Decrease upkeep by 50%.")),
	BldInfo(CardEnum::Motivation,	LOCTEXT("Motivation", "Motivation"), 100, LOCTEXT("Motivation Desc", "Every 1% Happiness above 60%, gives +1% productivity")),
	BldInfo(CardEnum::Passion,		LOCTEXT("Passion", "Passion"), 100, LOCTEXT("Passion Desc", "+20% Job Happiness, +15% Productivity")),

	
	BldInfo(CardEnum::DesertPilgrim,		LOCTEXT("Desert Pilgrim", "Desert Pilgrim"), 200, LOCTEXT("Desert Pilgrim Desc", "Houses built on Desert get +5<img id=\"Coin\"/>.")),

	BldInfo(CardEnum::WheatSeed,			LOCTEXT("Wheat Seeds", "Wheat Seeds"), 300, LOCTEXT("Wheat Seeds Desc", "Unlock Wheat farming. Wheat can be eaten or brewed into Beer.")),
	BldInfo(CardEnum::CabbageSeed,		LOCTEXT("Cabbage Seeds", "Cabbage Seeds"), 350, LOCTEXT("Cabbage Seeds Desc", "Unlock Cabbage farming. Cabbage has high fertility sensitivity.")),
	BldInfo(CardEnum::HerbSeed,			LOCTEXT("Medicinal Herb Seeds", "Medicinal Herb Seeds"), 500, LOCTEXT("Medicinal Herb Seeds Desc", "Unlock Medicinal Herb farming. Medicinal Herb can be used to heal sickness.")),
	BldInfo(CardEnum::PotatoSeed,			LOCTEXT("Potato Seeds", "Potato Seeds"), 300, LOCTEXT("Potato Seeds Desc", "Unlock Potato farming. Potato can be eaten or brewed into Vodka.")),
	BldInfo(CardEnum::BlueberrySeed,		LOCTEXT("Blueberry Seeds", "Blueberry Seeds"), 300, LOCTEXT("Blueberry Seeds Desc", "Unlock Blueberry farming. Blueberries can be eaten.")),
	BldInfo(CardEnum::MelonSeed,			LOCTEXT("Melon Seeds", "Melon Seeds"), 300, LOCTEXT("Melon Seeds Desc", "Unlock Melon farming. Melon can be eaten.")),
	BldInfo(CardEnum::PumpkinSeed,			LOCTEXT("Pumpkin Seeds", "Pumpkin Seeds"), 300, LOCTEXT("Pumpkin Seeds Desc", "Unlock Pumpkin farming. Pumpkin can be eaten.")),
	
	BldInfo(CardEnum::CannabisSeeds,		LOCTEXT("Cannabis Seeds", "Cannabis Seeds"), 0, LOCTEXT("Cannabis Seeds Desc", "Unlock Cannabis farming. Require region suitable for Cannabis.")),
	BldInfo(CardEnum::GrapeSeeds,			LOCTEXT("Grape Seeds", "Grape Seeds"), 0, LOCTEXT("Grape Seeds Desc", "Unlock Grape farming. Requires region suitable for Grape.")),
	BldInfo(CardEnum::CocoaSeeds,			LOCTEXT("Cocoa Seeds", "Cocoa Seeds"), 0, LOCTEXT("Cocoa Seeds Desc", "Unlock Cocoa farming. Requires region suitable for Cocoa.")),
	BldInfo(CardEnum::CottonSeeds,			LOCTEXT("Cotton Seeds", "Cotton Seeds"), 0, LOCTEXT("Cotton Seeds Desc", "Unlock Cotton farming. Requires region suitable for Cotton.")),
	BldInfo(CardEnum::DyeSeeds,				LOCTEXT("Dye Seeds", "Dye Seeds"), 0, LOCTEXT("Dye Seeds Desc", "Unlock Dye farming. Requires region suitable for Dye.")),
	BldInfo(CardEnum::CoffeeSeeds,			LOCTEXT("Coffee Seeds", "Coffee Seeds"), 0, LOCTEXT("Coffee Seeds Desc", "Unlock Coffee farming. Requires region suitable for Coffee.")),
	BldInfo(CardEnum::TulipSeeds,			LOCTEXT("Tulip Seeds", "Tulip Seeds"), 0, LOCTEXT("Tulip Seeds Desc", "Unlock Tulip farming. Requires region suitable for Tulip.")),

	BldInfo(CardEnum::ChimneyRestrictor,	LOCTEXT("Chimney Restrictor", "Chimney Restrictor"), 250, LOCTEXT("Chimney Restrictor Desc", "Wood/Coal gives 15% more heat")),
	BldInfo(CardEnum::SellFood,				LOCTEXT("Sell Food", "Sell Food"), 90, LOCTEXT("Sell Food Desc", "Sell half of city's food for 5<img id=\"Coin\"/> each.")),
	BldInfo(CardEnum::BuyWood,				LOCTEXT("Buy Wood", "Buy Wood"), 50, LOCTEXT("Buy Wood Desc", "Buy Wood with half of your treasury for 6<img id=\"Coin\"/> each. (max: 1000)")),
	BldInfo(CardEnum::ChildMarriage,		LOCTEXT("Child Marriage", "Child Marriage"), 200, LOCTEXT("Child Marriage Desc", "Decrease the minimum age for having children.")),
	BldInfo(CardEnum::ProlongLife,			LOCTEXT("Prolong Life", "Prolong Life"), 200, LOCTEXT("Prolong Life Desc", "People live longer.")),
	BldInfo(CardEnum::BirthControl,			LOCTEXT("Birth Control", "Birth Control"), 200, LOCTEXT("Birth Control Desc", "Prevents childbirth when the housing capacity is full.")),
	BldInfo(CardEnum::CoalTreatment,		LOCTEXT("Coal Treatment", "Coal Treatment"), 250, LOCTEXT("Coal Treatment Desc", "Coal gives 20% more heat")),


	BldInfo(CardEnum::CoalPipeline,			LOCTEXT("Coal Pipeline", "Coal Pipeline"), 150, LOCTEXT("Coal Pipeline Desc", "+50% productivity for smelters if the town has more than 1,000 Coal")),
	BldInfo(CardEnum::MiningEquipment,		LOCTEXT("Mining Equipment", "Mining Equipment"), 150, LOCTEXT("Mining Equipment Desc", "+30% productivity for mines if you have a Blacksmith")),
	BldInfo(CardEnum::Conglomerate,			LOCTEXT("Conglomerate", "Conglomerate"), 150, LOCTEXT("Conglomerate Desc", "Upkeep for Trading Companies are reduced to 1")),
	BldInfo(CardEnum::SmeltCombo,			LOCTEXT("Iron Smelter Combo", "Iron Smelter Combo"), 150, LOCTEXT("Iron Smelter Combo Desc", "+30% productivity to all Iron Smelter with adjacent Iron Smelter")),


	BldInfo(CardEnum::Immigration,			LOCTEXT("Immigrants", "Immigrants"), 500, LOCTEXT("Immigrants Desc", "5 immigrants join upon use.")),
	BldInfo(CardEnum::DuplicateBuilding,	INVTEXT("Duplicate Building"), 200, INVTEXT("Duplicate the chosen building into a card.")),


	BldInfo(CardEnum::Pig,				INVTEXT("Pig"), 100, INVTEXT("Spawn 3 Pigs on a Ranch.")),
	BldInfo(CardEnum::Sheep,			INVTEXT("Sheep"), 200, INVTEXT("Spawn 3 Sheep on a Ranch.")),
	BldInfo(CardEnum::Cow,				INVTEXT("Cow"), 300, INVTEXT("Spawn 3 Cows on a Ranch.")),
	BldInfo(CardEnum::Panda,			INVTEXT("Panda"), 1000, INVTEXT("Spawn 3 Pandas on a Ranch.")),

	BldInfo(CardEnum::FireStarter,		LOCTEXT("Fire Starter", "Fire Starter"), 200,	LOCTEXT("Fire Starter Desc", "Start a fire in an area (3 tiles radius).")),
	BldInfo(CardEnum::Steal,			LOCTEXT("Steal", "Steal"), 200,					LOCTEXT("Steal Desc", "Steal 30% of target player's treasury<img id=\"Coin\"/>. Use on Townhall.")),
	BldInfo(CardEnum::Snatch,			LOCTEXT("Snatch", "Snatch"), 50,				LOCTEXT("Snatch Desc", "Steal <img id=\"Coin\"/> equal to target player's population. Use on Townhall.")),
	BldInfo(CardEnum::SharingIsCaring,	LOCTEXT("Sharing is Caring", "Sharing is Caring"), 120, LOCTEXT("Sharing is Caring Desc", "Give 100 Wheat to the target player. Use on Townhall.")),
	BldInfo(CardEnum::Kidnap,			LOCTEXT("Kidnap", "Kidnap"), 350,				LOCTEXT("Kidnap Desc", "Steal up to 3 citizens from target player. Apply on Townhall.")),
	BldInfo(CardEnum::KidnapGuard,		LOCTEXT("Kidnap Guard", "Kidnap Guard"), 20,	LOCTEXT("Kidnap Guard Desc", "Guard your city against Kidnap for two years. Require <img id=\"Coin\"/>xPopulation to activate.")),
	BldInfo(CardEnum::TreasuryGuard,	LOCTEXT("Treasury Guard", "Treasury Guard"), 20, LOCTEXT("Treasury Guard Desc", "Guard your city against Steal and Snatch for two years. Require <img id=\"Coin\"/>xPopulation to activate.")),

	
	BldInfo(CardEnum::Cannibalism,		LOCTEXT("Cannibalism", "Cannibalism"), 0, LOCTEXT("Cannibalism Desc", "On death, people drop Meat. -50% City Attractiveness Happiness.")),

	BldInfo(CardEnum::WildCard,		LOCTEXT("Wild Card", "Wild Card"), 15, LOCTEXT("Wild Card Desc", "Build an unlocked building of your choice.")),

	// June Additions
	BldInfo(CardEnum::WildCardFood,		LOCTEXT("Agriculture Wild Card", "Agriculture Wild Card"), 10, LOCTEXT("Agriculture Wild Card Desc", "Build an unlocked agriculture building of your choice.")),
	BldInfo(CardEnum::WildCardIndustry,	LOCTEXT("Industry Wild Card", "Industry Wild Card"), 10, LOCTEXT("Industry Wild Card Desc", "Build an unlocked industry of your choice.")),
	BldInfo(CardEnum::WildCardMine,		LOCTEXT("Mine Wild Card", "Mine Wild Card"), 10, LOCTEXT("Mine Wild Card Desc", "Build an unlocked mine of your choice.")),
	BldInfo(CardEnum::WildCardService,		LOCTEXT("Service Wild Card", "Service Wild Card"), 10, LOCTEXT("Service Wild Card Desc", "Build an unlocked service building of your choice.")),
	BldInfo(CardEnum::CardRemoval,			LOCTEXT("Card Removal", "Card Removal"), 30, LOCTEXT("Card Removal Desc", "Remove a card from the draw deck.")),

	// Rare cards
	BldInfo(CardEnum::HappyBreadDay,		LOCTEXT("Happy Bread Day", "Happy Bread Day"), 150, LOCTEXT("Happy Bread Day Desc", "+20% Food Happiness to Citizens, if the Town has more than 1,000 Bread")),
	BldInfo(CardEnum::BlingBling,		LOCTEXT("Bling Bling", "Bling Bling"), 150, LOCTEXT("Bling Bling Desc", "+20% Luxury Happiness from Jewelry")),
	BldInfo(CardEnum::GoldRush,			LOCTEXT("Gold Rush", "Gold Rush"), 150, LOCTEXT("Gold Rush Desc", "+30% productivity from Gold Mine.")),

	BldInfo(CardEnum::AllYouCanEat,		LOCTEXT("All You Can Eat", "All You Can Eat"), 200, LOCTEXT("All You Can Eat Desc", "+30% Food Happiness. +50% food consumption")),
	BldInfo(CardEnum::SlaveLabor,		LOCTEXT("Slave Labor", "Slave Labor"), 200,		LOCTEXT("Slave Labor Desc", "Work Efficiency Penalty from low Happiness will not exceed 30%")),
	BldInfo(CardEnum::Lockdown,			LOCTEXT("Lockdown", "Lockdown"), 200,				LOCTEXT("Lockdown Desc", "Citizens cannot immigrate out of town without permission.")),
	BldInfo(CardEnum::SocialWelfare,		LOCTEXT("Social Welfare", "Social Welfare"), 200, LOCTEXT("Social Welfare Desc", "+20% Job Happiness, -10 gold for each citizen")),

	
	BldInfo(CardEnum::Treasure,			LOCTEXT("Treasure", "Treasure"), 100, LOCTEXT("Treasure Desc", "Instantly gain 500 <img id=\"Coin\"/>")),
	BldInfo(CardEnum::IndustrialRevolution,LOCTEXT("Industrial Revolution", "Industrial Revolution"), 200, LOCTEXT("Industrial Revolution Desc", "-50% cost of industrial building cards")),
	BldInfo(CardEnum::EmergencyRations,	LOCTEXT("Emergency Rations", "Emergency Rations"), 250, LOCTEXT("Emergency Rations Desc", "Instantly gain 50 Wheat.")),

	BldInfo(CardEnum::CrateWood,		LOCTEXT("Wood Crates", "Wood Crates"), 100, LOCTEXT("Wood Crates Desc", "Instantly gain 100 Wood")),
	BldInfo(CardEnum::CrateCoal,		LOCTEXT("Coal Crates", "Coal Crates"), 100, LOCTEXT("Coal Crates Desc", "Instantly gain 150 Coal")),
	BldInfo(CardEnum::CrateIronOre,		LOCTEXT("Iron Ore Crates", "Iron Ore Crates"), 100, LOCTEXT("Iron Ore Crates Desc", "Instantly gain 200 Iron Ore")),
	BldInfo(CardEnum::CratePottery,		LOCTEXT("Pottery Crates", "Pottery Crates"), 100, LOCTEXT("Pottery Crates Desc", "Instantly gain 80 Pottery")),
	BldInfo(CardEnum::CrateJewelry,		LOCTEXT("Jewelry Crates", "Jewelry Crates"), 100, LOCTEXT("Jewelry Crates Desc", "Instantly gain 10 Jewelry")),

	BldInfo(CardEnum::SpeedBoost,		LOCTEXT("Speed Boost", "Speed Boost"), 0, LOCTEXT("Speed Boost Desc", "Boost target building's work speed by +50% for 50s.")),
};

#undef LOCTEXT_NAMESPACE

static const int32 BuildingEnumCount = _countof(BuildingInfo);
static const int32 NonBuildingCardEnumCount = _countof(CardInfos);

static const std::vector<CardEnum> ActionCards
{
	CardEnum::Treasure,
	CardEnum::SellFood,
	CardEnum::BuyWood,
	CardEnum::Immigration,
	CardEnum::EmergencyRations,

	CardEnum::FireStarter,
	CardEnum::Steal,
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

static bool IsGlobalSlotCard(CardEnum cardEnum)
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
	case CardEnum::GoldRush:

		return true;
	default:
		return false;
	}
}

struct CardStatus
{
	// Enum and status used to animate card
	CardEnum cardEnum = CardEnum::None;
	int32 lastPositionX100 = -1;
	int32 lastPositionY100 = -1;
	int32 animationStartTime100 = -1;

	FVector2D lastPosition() const {
		return FVector2D(lastPositionX100 / 100.0f, lastPositionY100 / 100.0f);
	}

	void ResetAnimation() {
		lastPositionX100 = -1;
		lastPositionY100 = -1;
		animationStartTime100 = -1;
	}

	//! Serialize
	FArchive& operator>>(FArchive &Ar)
	{
		if (Ar.IsSaving()) {
			ResetAnimation();
		}
		
		Ar << cardEnum;
		Ar << lastPositionX100;
		Ar << lastPositionY100;
		Ar << animationStartTime100;
		return Ar;
	}
};
inline bool operator==(const CardStatus& lhs, const CardStatus& rhs) {
	return lhs.cardEnum == rhs.cardEnum && lhs.animationStartTime100 == rhs.animationStartTime100;
}

static const std::vector<CardEnum> BuildingSlotCards
{
	CardEnum::ProductivityBook,
	CardEnum::SustainabilityBook,
	CardEnum::FrugalityBook,
	CardEnum::Motivation,
	CardEnum::Passion,
};

static bool IsBuildingSlotCard(CardEnum cardEnum)
{
	for (CardEnum slotCardEnum : BuildingSlotCards) {
		if (slotCardEnum == cardEnum) {
			return true;
		}
	}
	return false;
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

static int32 AreaSpellRadius(CardEnum cardEnum) {
	switch (cardEnum) {
	case CardEnum::FireStarter: return 3;
	case CardEnum::Steal: return 1;
	case CardEnum::Snatch: return 1;
	case CardEnum::Kidnap: return 1;
	case CardEnum::SharingIsCaring: return 1;

	// Skill
	case CardEnum::SpeedBoost: return 1;
		
	case CardEnum::InstantBuild: return 1;
	default: return -1;
	}
}
inline bool IsAreaSpell(CardEnum cardEnum) {
	return AreaSpellRadius(cardEnum) != -1;
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

inline CardEnum FindCardEnumByName(std::wstring nameIn)
{
	ToLowerCase(nameIn);
	
	auto tryAddCard = [&](CardEnum cardEnum)
	{
		std::wstring name = GetBuildingInfo(cardEnum).nameW();
		ToLowerCase(name);

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
		CardEnum cardEnum = FindCardEnumByName(name);
		PUN_CHECK(IsBuildingCard(cardEnum));
		results.push_back(cardEnum);
	}

	return results;
}
static const std::vector<CardEnum> SortedNameBuildingEnum = GetSortedBuildingEnum();



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

static bool IsHeavyIndustryOrMine(CardEnum buildingEnum)
{
	if (IsMountainMine(buildingEnum)) {
		return true;
	}
	switch (buildingEnum) {
	case CardEnum::IronSmelter:
	case CardEnum::GoldSmelter:
	case CardEnum::CharcoalMaker:
	case CardEnum::IronSmelterGiant:
		return true;
	default:
		return false;
	}
}

static std::vector<CardEnum> FoodBuildings // Change to AgricultureBuildings
{
	CardEnum::Farm,
	
	CardEnum::FruitGatherer,
	CardEnum::HuntingLodge,
	CardEnum::Fisher,
	CardEnum::MushroomFarm,
	CardEnum::Windmill,
	CardEnum::Bakery,
	CardEnum::Beekeeper,

	CardEnum::Forester,
	
	CardEnum::RanchBarn,
	CardEnum::RanchPig,
	CardEnum::RanchSheep,
	CardEnum::RanchCow,
};
static bool IsAgricultureBuilding(CardEnum cardEnum) {
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
	switch (buildingEnum) {
	case CardEnum::Tavern:
	case CardEnum::Theatre:
		
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
	CardEnum::Bank,
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
	case CardEnum::InventorsWorkshop:
	//case CardEnum::RegionShrine:
	case CardEnum::ImmigrationOffice:

	case CardEnum::BarrackArcher:
	case CardEnum::BarrackSwordman:
		return true;
	default: return false;
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


static bool IsStackableTileBuilding(CardEnum buildingEnum) {
	switch (buildingEnum) {
	case CardEnum::DirtRoad:
	case CardEnum::StoneRoad:
	case CardEnum::TrapSpike:
		return true;
	default: return false;
	}
}

static bool IsRoadOverlapBuilding(CardEnum buildingEnum)
{
	switch (buildingEnum) {
	case CardEnum::FenceGate:
	case CardEnum::TrapSpike:
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

static const std::vector<CardEnum> RegionalBuildingEnums
{
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
	case CardEnum::TradingPort:
	case CardEnum::IntercityLogisticsPort:
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

	if (IsHeavyIndustryOrMine(cardEnum) ||
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
	ResourcePair resourceNeeded;
	int32 moneyNeeded;

	int32 efficiencyBonus = 0;
	int32 comboEfficiencyBonus = 0;
	int32 workerSlotBonus = 0;

	bool isUpgraded = false;

	BuildingUpgrade() : name(FText()), description(FText()), resourceNeeded(ResourcePair()), moneyNeeded(-1) {}
	
	BuildingUpgrade(FText name, FText description, ResourcePair resourceNeeded, int32 moneyNeeded = 0)
		: name(name), description(description), resourceNeeded(resourceNeeded), moneyNeeded(moneyNeeded) {}

	BuildingUpgrade(FText name, FText description, int32 moneyNeeded)
		: name(name), description(description), resourceNeeded(ResourcePair()), moneyNeeded(moneyNeeded) {}

	BuildingUpgrade(FText name, FText description, ResourceEnum resourceEnum, int32 resourceCount)
		: name(name), description(description), resourceNeeded(ResourcePair(resourceEnum, resourceCount)), moneyNeeded(0) {}

	void operator>>(FArchive& Ar)
	{
		//SerializeStr(Ar, name);
		//SerializeStr(Ar, description);
		Ar << name;
		Ar << description;
		resourceNeeded >> Ar;
		Ar << moneyNeeded;
		
		Ar << efficiencyBonus;
		Ar << comboEfficiencyBonus;
		Ar << workerSlotBonus;

		//Ar << extraWorkerSlots;

		Ar << isUpgraded;
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
	Vassalize,
	DeclareIndependence,

	VassalCompetition,
	ConquerColony,
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
	

	// ... TreeEnumSize ... Don't forget!!!
	GrassGreen, // Currently includes savanna grass

	OreganoBush,
	CommonBush,
	
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

const static int32 TreeEnumSize = 14; // TileObjEnum up to last tree... TODO: change to last tree...
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
static const int32 GatherUnitsPerYear = 5; // 6;  // GatherBaseYield 4 gives 60 fruits per year... So 15 estimated gather per year...
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
	
	TileObjInfo(TileObjEnum::GrassGreen, LOCTEXT("Grass", "Grass"),	ResourceTileType::Bush,	ResourcePair::Invalid(),		defaultGrass100, LOCTEXT("Grass Desc", "Common grass. A nice food-source for grazing animals.")),

	TileObjInfo(TileObjEnum::OreganoBush, LOCTEXT("Common Bush", "Common Bush"),	ResourceTileType::Bush,	ResourcePair::Invalid(), defaultHay100, LOCTEXT("Common Bush Desc", "Fluffy little bush. A nice food-source for grazing animals.")),
	TileObjInfo(TileObjEnum::CommonBush, LOCTEXT("Common Bush", "Common Bush"), ResourceTileType::Bush,	ResourcePair::Invalid(), defaultHay100, LOCTEXT("Common Bush Desc", "Fluffy little  bush. A nice food-source for grazing animals.")),

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
	TileObjInfo(TileObjEnum::Melon,		LOCTEXT("Melon", "Melon"),	ResourceTileType::Bush,	ResourcePair::Invalid(), ResourcePair(ResourceEnum::Melon, FarmBaseYield100), LOCTEXT("Melon Desc", "Sweet and refreshing fruit. +3<img id=\"Coin\"/> each unit when consumed.")),
	TileObjInfo(TileObjEnum::Pumpkin,	LOCTEXT("Pumpkin", "Pumpkin"),	ResourceTileType::Bush,	ResourcePair::Invalid(), ResourcePair(ResourceEnum::Pumpkin, FarmBaseYield100), LOCTEXT("Pumpkin Desc", "Fruit with delicate, mildly-flavored flesh.")),
	TileObjInfo(TileObjEnum::RawCoffee,	LOCTEXT("Raw Coffee", "Raw Coffee"),	ResourceTileType::Bush,	ResourcePair::Invalid(), ResourcePair(ResourceEnum::RawCoffee, FarmBaseYield100), LOCTEXT("Raw Coffee Desc", "Fruit that can be roasted to make Coffee.")),
	TileObjInfo(TileObjEnum::Tulip,		LOCTEXT("Tulip", "Tulip"),	ResourceTileType::Bush,	ResourcePair::Invalid(), ResourcePair(ResourceEnum::Tulip, FarmBaseYield100), LOCTEXT("Tulip Desc", "Beautiful decorative flower. (Luxury tier 1)")),

	
	TileObjInfo(TileObjEnum::Herb,		LOCTEXT("Medicinal Herb", "Medicinal Herb"),		ResourceTileType::Bush,	ResourcePair::Invalid(), ResourcePair(ResourceEnum::Herb, FarmBaseYield100), LOCTEXT("Medicinal Herb Desc", "Herb used to heal sickness or make medicine.")),

	TileObjInfo(TileObjEnum::Stone, LOCTEXT("Stone", "Stone"),	ResourceTileType::Deposit,	ResourcePair::Invalid(),								ResourcePair(ResourceEnum::Stone, 2) /*this is not used?*/, LOCTEXT("Stone Desc", "Easily-access ible stone deposits.")),

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

enum class PlacementType
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

	DeliveryTarget,
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
			!IsDecorativeBuilding(cardEnum);
}

static bool IsStorageTooLarge(TileArea area) {
	return (area.sizeX() / 2 * 2) > 8 || 
		   (area.sizeY() / 2 * 2) > 8; // 4x4 max storage yard
}
static bool IsStorageWidthTooHigh(TileArea area) {
	return (area.sizeX() / 2) > 16 || (area.sizeY() / 2) > 16;
}

static bool IsFarmTooLarge(TileArea area) {
	return area.sizeX() * area.sizeY() > 64;
}
static bool IsFarmTooSmall(TileArea area) {
	return area.sizeX() * area.sizeY() < 16;
}
static bool IsFarmWidthTooHigh(TileArea area) {
	return area.sizeX() > 16 || area.sizeY() > 16;
}
static bool IsFarmSizeInvalid(TileArea area) {
	if (SimSettings::IsOn("NoFarmSizeCap")) {
		return false;
	}
	return IsFarmTooLarge(area) || 
			IsFarmTooSmall(area) || 
			IsFarmWidthTooHigh(area);
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

	BadAppeal,
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
	Luxury,
	TerritoryUpkeep,
	BorderProvinceUpkeep,
	TooMuchInfluencePoints,

	Fort,
	Colony,
	
	Count,
};
static const TArray<FText> InfluenceIncomeEnumName
{
	LOCTEXT("Townhall", "Townhall"),
	LOCTEXT("Population", "Population"),
	LOCTEXT("Luxury Consumption", "Luxury Consumption"),
	LOCTEXT("Territory Upkeep", "Territory Upkeep"),
	LOCTEXT("Flat-Land Border Province Upkeep", "Flat-Land Border Province Upkeep"),
	LOCTEXT("Too Much Stored Influence Points", "Too Much Stored Influence Points"),

	LOCTEXT("Fort", "Fort"),
	LOCTEXT("Colony", "Colony"),
};
static int32 InfluenceIncomeEnumCount = static_cast<int32>(InfluenceIncomeEnum::Count);

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
	//Card_MiddleClass,
	Card_BeerTax,
	Card_DesertPilgrim,

	// HouseIncomeEnumCount!!!

	// Other income
	TownhallIncome, // HouseIncomeEnumCount!!!
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
	TerritoryUpkeep,
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
	//LOCTEXT("Card Middle Class Income", "Card Middle Class Income"),
	LOCTEXT("Card Beer Tax", "Card Beer Tax"),
	LOCTEXT("Card Desert Pilgrim", "Card Desert Pilgrim"),

	LOCTEXT("Townhall Income", "Townhall Income"),
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

static const int32 HouseIncomeEnumCount = static_cast<int32>(IncomeEnum::TownhallIncome);

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

	// ScienceModifiers Below
	Library,
	School,

	Count,
};
#define LOCTEXT_NAMESPACE "ScienceEnumName"
static TArray<FText> ScienceEnumNameList
{
	LOCTEXT("HouseBase", "House Base"),
	LOCTEXT("HouseLuxury", "House Luxury"),
	LOCTEXT("HomeBrew", "Home Brew"),

	LOCTEXT("Knowledgetransfer", "Knowledge transfer"),
	LOCTEXT("ScientificTheories", "Scientific Theories"),

	LOCTEXT("Rationalism", "Rationalism"),

	LOCTEXT("Library", "Library"),
	LOCTEXT("School", "School"),
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
	
	ShallowWaterEmbark,
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

	FarmImprovement,
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
	Glassworks,
	ConcreteFactory,
	Industrialization,
	Steelworks,
	
	StoneToolsShop,
	OilWell,
	OilPowerPlant,
	PaperMill,
	ClockMakers,

	Cathedral,
	Castle,
	GrandMuseum,
	ExhibitionHall,

	SocialScience,

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
	PortColony,
	IntercityLogistics,

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

	FarmingBreakthrough,

	MoreGoldPerHouse,
	
	WineryImprovement,

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

	Rationalism,

	Fort,
	ResourceOutpost,
	InventorsWorkshop,
	IntercityRoad,

	Combo,

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
	ShallowWater,
	Deepwater,
};

void AppendClaimConnectionString(TArray<FText>& args, bool isConquering, ClaimConnectionEnum claimConnectionEnum);

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
	Alpaca,
	Human,
	Boar,

	RedDeer,
	YellowDeer,
	DarkDeer,

	BrownBear,
	BlackBear,
	Panda,

	WildMan,
	Hippo,
	Penguin,

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

static bool IsHumanDisplay(UnitEnum unitEnum)
{
	return unitEnum == UnitEnum::Human ||
		unitEnum == UnitEnum::HorseCaravan ||
		unitEnum == UnitEnum::HorseMarket ||
		unitEnum == UnitEnum::HorseLogistics;
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

	TileObjEnum GetRandomTreeEnum() {
		return trees[GameRand::Rand() % trees.size()];
	}
	TileObjEnum GetRandomPlantEnum() {
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
		{ TileObjEnum::Cactus1  },
		{ TileObjEnum::GrassGreen },
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
static BiomeInfo GetBiomeInfo(BiomeEnum biomeEnum) {
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
static const FloatDet RiverMaxDepth = FlatLandHeight - FD0_XXX(015);// FD0_XXX(110);

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
static const int32 AssumedHuntUnitPerYear = 4; // 6; // at BaseUnitDrop 5... hunters produce 60 per year ... or 12 Hunt Unit...
static const int32 BaseUnitDrop100 = AssumedFoodProduction100PerYear / AssumedHuntUnitPerYear;

static const int32 AnimalFoodPerYear = 300;

static const int32 UsualAnimalAge = 200;
static const int32 AnimalMinBreedingAge = 70;

static const int32 AnimalGestation = 020;

static const UnitInfo UnitInfos[]
{
	// Human last half winter without heat...
	// average winter temperature is -5

	//	adultYears100, maxAgeYears100, minBreedingAgeYears100,
	//	gestationYears100, winterSurvivalLength_Years100, foodPerYear
	UnitInfo(UnitEnum::Alpaca, LOCTEXT("Feral Alpaca", "Feral Alpaca"),	500,	100,		AnimalGestation,	100,	HumanFoodPerYear, {{ResourceEnum::Pork, 2 * BaseUnitDrop100}}),
	UnitInfo(UnitEnum::Human,	LOCTEXT("Human", "Human"),	1000,	100,		025,	020,	HumanFoodPerYear, {{ResourceEnum::GameMeat, 2 * BaseUnitDrop100}}),
	UnitInfo(UnitEnum::Boar,	LOCTEXT("Boar", "Boar"),	UsualAnimalAge,	AnimalMinBreedingAge,		AnimalGestation,	100,	AnimalFoodPerYear, {{ResourceEnum::GameMeat, 2 * BaseUnitDrop100}, {ResourceEnum::Leather, BaseUnitDrop100}}),

	UnitInfo(UnitEnum::RedDeer,	LOCTEXT("Red Deer", "Red Deer"),	UsualAnimalAge,	AnimalMinBreedingAge,		AnimalGestation,	100,	AnimalFoodPerYear, {{ResourceEnum::GameMeat, 2 * BaseUnitDrop100}, {ResourceEnum::Leather, BaseUnitDrop100}}),
	UnitInfo(UnitEnum::YellowDeer, LOCTEXT("Mule Deer", "Mule Deer"),	UsualAnimalAge,	AnimalMinBreedingAge,		AnimalGestation,	100,	AnimalFoodPerYear, {{ResourceEnum::GameMeat, 2 * BaseUnitDrop100}, {ResourceEnum::Leather, BaseUnitDrop100}}),
	UnitInfo(UnitEnum::DarkDeer, LOCTEXT("Sambar Deer", "Sambar Deer"),	UsualAnimalAge,	AnimalMinBreedingAge,		AnimalGestation,	100,	AnimalFoodPerYear, {{ResourceEnum::GameMeat, 2 * BaseUnitDrop100}, {ResourceEnum::Leather, BaseUnitDrop100}}),

	UnitInfo(UnitEnum::BrownBear, LOCTEXT("Brown Bear", "Brown Bear"),	UsualAnimalAge,	AnimalMinBreedingAge,		AnimalGestation,	100,	AnimalFoodPerYear, {{ResourceEnum::GameMeat, 2 * BaseUnitDrop100}, {ResourceEnum::Leather, BaseUnitDrop100}}),
	UnitInfo(UnitEnum::BlackBear, LOCTEXT("Black Bear", "Black Bear"),	UsualAnimalAge,	AnimalMinBreedingAge,		AnimalGestation,	100,	AnimalFoodPerYear, {{ResourceEnum::GameMeat, 2 * BaseUnitDrop100}, {ResourceEnum::Leather, BaseUnitDrop100}}),
	UnitInfo(UnitEnum::Panda, LOCTEXT("Panda", "Panda"),	UsualAnimalAge,	AnimalMinBreedingAge,		AnimalGestation,	100,	AnimalFoodPerYear, {{ResourceEnum::GameMeat, 2 * BaseUnitDrop100}, {ResourceEnum::Leather, BaseUnitDrop100}}),

	UnitInfo(UnitEnum::WildMan, LOCTEXT("WildMan", "Wild Man"),	UsualAnimalAge,	AnimalMinBreedingAge,		AnimalGestation,	100,	AnimalFoodPerYear, {{ResourceEnum::GameMeat, 2 * BaseUnitDrop100}, {ResourceEnum::Leather, BaseUnitDrop100}}),
	UnitInfo(UnitEnum::Hippo, LOCTEXT("Hippo", "Hippo"),	UsualAnimalAge,	AnimalMinBreedingAge,		AnimalGestation,	100,	AnimalFoodPerYear, {{ResourceEnum::GameMeat, 2 * BaseUnitDrop100}, {ResourceEnum::Leather, BaseUnitDrop100}}),
	UnitInfo(UnitEnum::Penguin, LOCTEXT("Penguin", "Penguin"),	UsualAnimalAge,	AnimalMinBreedingAge,		AnimalGestation,	100,	AnimalFoodPerYear, {{ResourceEnum::GameMeat, 2 * BaseUnitDrop100}, {ResourceEnum::Leather, BaseUnitDrop100}}),
	
	
	UnitInfo(UnitEnum::Pig, LOCTEXT("Pig", "Pig"),	UsualAnimalAge,	AnimalMinBreedingAge,		AnimalGestation,	100,	HumanFoodPerYear, {{ResourceEnum::Pork, BaseUnitDrop100 * 240 / 100}}),
	UnitInfo(UnitEnum::Sheep, LOCTEXT("Sheep", "Sheep"),	UsualAnimalAge,	AnimalMinBreedingAge,		AnimalGestation,	100,	HumanFoodPerYear, {{ResourceEnum::Lamb, BaseUnitDrop100 * 120 / 100}, {ResourceEnum::Wool, BaseUnitDrop100 * 120 / 100}}),
	UnitInfo(UnitEnum::Cow, LOCTEXT("Cow", "Cow"),	UsualAnimalAge,	AnimalMinBreedingAge,		AnimalGestation,	100,	HumanFoodPerYear, {{ResourceEnum::Beef, BaseUnitDrop100 * 120 / 100}, {ResourceEnum::Leather,  BaseUnitDrop100 * 120 / 100}}),

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

static const CardEnum AnimalCards[] =
{
	CardEnum::Cow,
	CardEnum::Pig,
	CardEnum::Sheep,
	CardEnum::Panda,
};
static bool IsAnimalCard(CardEnum cardEnumIn)
{
	for (CardEnum cardEnum : AnimalCards) {
		if (cardEnum == cardEnumIn) {
			return true;
		}
	}
	return false;
}
static UnitEnum GetAnimalEnumFromCardEnum(CardEnum buildingEnum)
{
	switch (buildingEnum)
	{
	case CardEnum::Cow: return UnitEnum::Cow;
	case CardEnum::Pig: return UnitEnum::Pig;
	case CardEnum::Sheep: return UnitEnum::Sheep;
	case CardEnum::Panda: return UnitEnum::Panda;
	default:
		UE_DEBUG_BREAK();
	}
	return UnitEnum::Cow;
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
static const std::vector<std::string> UnitAnimationNames =
{
	"None",
	"Wait",
	"Walk",
	"Build",

	"ChopWood",
	"StoneMining",
	"FarmPlanting",

	"Caravan",
	"Ship",
	"Immigration",

	"HorseMarket",
	"HorseLogistics",
	"HaulingCart",

	"Rest",
	"Invisible",
};
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

static const std::string& GetUnitAnimationName(UnitAnimationEnum animationEnum) {
	return UnitAnimationNames[static_cast<int>(animationEnum)];
}
static const int32 UnitAnimationCount = UnitAnimationNames.size();

static bool IsHorseAnimation(UnitAnimationEnum animationEnum)
{
	return animationEnum == UnitAnimationEnum::HorseCaravan ||
		animationEnum == UnitAnimationEnum::HorseMarket ||
		animationEnum == UnitAnimationEnum::HorseLogistics;
}

static float GetUnitAnimationPlayRate(UnitAnimationEnum animationEnum) {
	return UnitAnimationPlayRate[static_cast<int>(animationEnum)];
}



struct UnitDisplayState
{
	UnitEnum unitEnum = UnitEnum::Alpaca;
	UnitAnimationEnum animationEnum = UnitAnimationEnum::None;
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

static const UnitEnum SkelMeshUnits[] = {
	UnitEnum::Human,
	UnitEnum::WildMan,
	UnitEnum::Hippo,
	UnitEnum::Penguin,
};
static bool IsUsingSkeletalMesh(UnitEnum unitEnumIn, UnitAnimationEnum animationEnum) {
	if (animationEnum == UnitAnimationEnum::Ship) {
		return false;
	}
	for (UnitEnum unitEnum : SkelMeshUnits) {
		if (unitEnum == unitEnumIn) {
			return true;
		}
	}
	return false;
}
static bool IsUsingVertexAnimation(UnitEnum unitEnumIn) {
	for (UnitEnum unitEnum : SkelMeshUnits) {
		if (unitEnum == unitEnumIn) {
			return false;
		}
	}
	return true;
}

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

//! Cheat
enum class CheatEnum : int32
{
	UnlockAll,
	Money,
	Influence,
	FastBuild,
	Resources,
	Undead,
	Immigration,

	FastTech,
	ForceFunDown,
	ForceFoodDown,
	ForceSick,
	Kill,
	ClearLand,
	Unhappy,

	AddHuman,
	AddAnimal,
	AddWildMan,
	KillUnit,
	SpawnDrop,

	AddResource,
	AddCard,
	AddMoney,
	AddInfluence,
	AddImmigrants,

	Cheat,
	Army,
	YearlyTrade,

	HouseLevel,
	HouseLevelKey,
	FullFarmRoad,
	
	NoCameraSnap,
	GeoresourceAnywhere,
	NoFarmSizeCap,
	MarkedTreesNoDisplay,
	WorkAnimate,
	DemolishNoDrop,
	God,
	
	RemoveAllCards,
	BuyMap,
	TrailerCityGreen1,
	TrailerCityGreen2,
	TrailerCityBrown,
	
	TrailerPauseForCamera,
	TrailerForceSnowStart,
	TrailerForceSnowStop,
	TrailerIncreaseAllHouseLevel,
	TrailerPlaceSpeed,
	TrailerHouseUpgradeSpeed,
	TrailerForceAutumn,
	TrailerBeatShiftBack,
	TrailerTimePerBeat,
	TrailerRoadPerTick,

	AddAIImmigrants,
	AddAIMoney,
	AddAIInfluence,

	TestCity,
	DebugUI,
	Tog,

	ScoreVictory,
};
static const std::string CheatName[]
{
	"UnlockAll",
	"Money",
	"Influence",
	"FastBuild",
	"Resources",
	"Undead",
	"Immigration",

	"FastTech",
	"ForceFunDown",
	"ForceFoodDown",
	"ForceSick",
	"Kill",
	"ClearLand",
	"Unhappy",

	"AddHuman",
	"AddAnimal",
	"AddWildMan",
	"KillUnit",
	"SpawnDrop",

	"AddResource",
	"AddCard",
	"AddMoney",
	"AddInfluence",
	"AddImmigrants",

	"Cheat",
	"Army",
	"YearlyTrade",

	"HouseLevel",
	"HouseLevelKey",
	"FullFarmRoad",

	"NoCameraSnap",
	"GeoresourceAnywhere",
	"NoFarmSizeCap",
	"MarkedTreesNoDisplay",
	"WorkAnimate",
	"DemolishNoDrop",
	"God",
	
	"RemoveAllCards",
	"BuyMap",
	"TrailerCityGreen1",
	"TrailerCityGreen2",
	"TrailerCityBrown",
	
	"TrailerPauseForCamera",
	"TrailerForceSnowStart",
	"TrailerForceSnowStop",
	"TrailerIncreaseAllHouseLevel",
	"TrailerPlaceSpeed",
	"TrailerHouseUpgradeSpeed",
	"TrailerForceAutumn",
	"TrailerBeatShiftBack",
	"TrailerTimePerBeat",
	"TrailerRoadPerTick",

	"AddAIImmigrants",
	"AddAIMoney",
	"AddAIInfluence",

	"TestCity",
	"DebugUI",
	"Tog",

	"ScoreVictory",
};
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

	Count,
};

enum class DisplayGlobalEnum
{
	Province,
	Territory,

	Count,
};

struct DemolishDisplayInfo
{
	CardEnum buildingEnum = CardEnum::None;
	TileArea area;
	int32 tickDemolished = -1;

	int32 animationTicks() { return Time::Ticks() - tickDemolished; }

	bool isInUse() { return tickDemolished != -1; }
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
	
	Count,
};

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
	Four,
	Three,
	Opposite,
	Adjacent,
};

/*
 * UI
 */

enum class ExclusiveUIEnum : uint8
{
	CardHand1,
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

	SendImmigrantsUI,
	GiftResourceUI,
	ProsperityUI,

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


//!
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
		//SerializeStr(Ar, title);
		
		Ar << body;
		
		SerializeVecLoop(Ar, choices, [&](FText& text) {
			Ar << text;
		});
		
		Ar << replyReceiver;
		SerializeStr(Ar, popupSound);
		
		Ar << replyVar1;
		Ar << replyVar2;
		Ar << replyVar3;

		Ar << forcedNetworking;
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

	PopulationQuestCards1, // PopulationQuest cards drive people to upgrade their houses
	PopulationQuestCards2,
	PopulationQuestCards3,
	PopulationQuestCards4,
	PopulationQuestCards5,
	PopulationQuestCards6,
	PopulationQuestCards7,
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

	int32_t resourceCountAll() {
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

enum class FunServiceEnum : uint8
{
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
};

const std::vector<FunServiceInfo> FunServiceToCardEnum
{
	{FunServiceEnum::Theatre, CardEnum::Theatre, 70 },
	{FunServiceEnum::Tavern, CardEnum::Tavern, 50},
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

#if TRAILER_MODE
const float WorldZoomTransition_Unit = 1200;
#else
const float WorldZoomTransition_Unit = 680; // 680
#endif
const float WorldZoomTransition_UnitSmall = 680;
const float WorldZoomTransition_UnitAnimate = 1500;
const float WorldZoomTransition_HumanNoAnimate = 490;

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
	int32 aiPlayerId = std::max(0, playerId - GameConstants::MaxPlayers);
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
	DefendProvinceInfluence,
	DefendProvinceMoney,
	Liberate,

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

	SelectStartingLocation,
	UpgradeBuilding,
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

	SetGlobalJobPriority_Up,
	SetGlobalJobPriority_Down,
	SetGlobalJobPriority_FastUp,
	SetGlobalJobPriority_FastDown,

	SetMarketTarget,

	TradingPostTrade,

	QuickBuild,
	AddAnimalRanch,

	BudgetAdjust,

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
};

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
	case GameSpeedValue1: return LOCTEXT("half speed", "half speed");
	case GameSpeedValue2: return LOCTEXT("x1 speed", "x1 speed");
	case GameSpeedValue3: return LOCTEXT("x2 speed", "x2 speed");
	case GameSpeedValue4: return LOCTEXT("x5 speed", "x5 speed");
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

static const int32 Income100PerTiles = 1;
static const int32 ClaimToIncomeRatio = 50; // When 2, actual is 4 since upkeep is half the income

struct ProvinceConnection
{
	int32 provinceId;
	TerrainTileType tileType;
	int32 connectedTiles;

	bool isConnectedTileType() const {
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
};

static const std::vector<FText> HoverWarningString = {
	FText(),
	LOCTEXT("Depleted", "Depleted"),
	LOCTEXT("Storage Full", "Storage Full"),
	LOCTEXT("Storage Too Far", "Storage Too Far"),
	LOCTEXT("House Too Far", "House Too Far"),

	LOCTEXT("Not Enough Input", "Not Enough Input"),

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
};
static FText GetHoverWarningString(HoverWarning hoverWarning) { return HoverWarningString[static_cast<int>(hoverWarning)]; }


static const std::vector<FText> HoverWarningDescription = {
	FText(),
	LOCTEXT("Depleted Desc","Ores are Depleted in this Province."),
	LOCTEXT("Storage Full Desc", "All Storage are full. Build more Storage Yard or Warehouse."),
	LOCTEXT("Storage Too Far Desc", "This building is too far from Storage. Build a new Storage nearby to ensure this building runs efficiently."),
	LOCTEXT("House Too Far Desc", "This building is too far from Houses. Build a new House nearby to ensure this building runs efficiently."),

	LOCTEXT("Not Enough Input Desc", "Not enough Input Resource to keep this building running."),

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
};
static FText GetHoverWarningDescription(HoverWarning hoverWarning) { return HoverWarningDescription[static_cast<int>(hoverWarning)]; }

#undef LOCTEXT_NAMESPACE


/*
 * Diplomacy
 */

static const int32 GoldToRelationship = 20;

enum class RelationshipModifierEnum : uint8
{
	YouGaveUsGifts,
	YouAreStrong,
	YouBefriendedUs,
	WeAreFamily,

	AdjacentBordersSparkTensions,
	TownhallProximitySparkTensions,
	YouAreWeak,
	YouStealFromUs,
	YouKidnapFromUs,
	YouAttackedUs,
	WeFearCannibals,
};

FText RelationshipModifierNameInt(int32 index);
int32 RelationshipModifierCount();

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