#pragma once

#include "PunCity/PunAStar128x256.h"
#include "GameCoordinate.h"
#include "Math/Color.h"

#include <string>
#include <cctype>
#include <sstream>
#include <algorithm>
#include "PunCity/PunGameSettings.h"

//#include "PunCity/PunSTLContainerOverride.h"

#define TRAILER_MODE 0

#define SAVE_VERSION 12100147 // Day/Month/Time
#define GAME_VERSION 12100147

//! Utils

#define ToFString(stdString) FString((stdString).c_str())

#define ToFName(stdString) FName(*ToFString(stdString))

#define ToTChar(stdString) UTF8_TO_TCHAR((stdString).c_str())

#define ToStdString(fString) (std::string(TCHAR_TO_UTF8(*(fString))));

#define FToUTF8(fString) (TCHAR_TO_UTF8(*(fString)));

#define DEBUG_BUILD WITH_EDITOR //!UE_BUILD_SHIPPING
#define DEV_BUILD !UE_BUILD_SHIPPING

#if !defined(PUN_CHECK)
	#if DEBUG_BUILD
		#define PUN_CHECK(x) if (!(x)) { FDebug::DumpStackTraceToLog(); checkNoEntry(); }
	#else
		#define PUN_CHECK(x) if (!(x)) { FDebug::DumpStackTraceToLog(); UE_LOG(LogTemp, Error, TEXT("PUN_ERROR")); }
	#endif
#endif

#if !defined(PUN_ENSURE)
	#if DEBUG_BUILD
		#define PUN_ENSURE(condition, backupStatement) if (!(condition)) { FDebug::DumpStackTraceToLog(); UE_DEBUG_BREAK(); backupStatement; }
	#else
		#define PUN_ENSURE(condition, backupStatement) if (!(condition)) { FDebug::DumpStackTraceToLog(); UE_LOG(LogTemp, Error, TEXT("PUN_ERROR_ENSURE")); backupStatement; }
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

static float Clamp01(float value) {
	return std::max(0.0f, std::min(1.0f, value));
}
static float Clamp(int32 value, int32 minValue, int32 maxValue) {
	return std::max(minValue, std::min(maxValue, value));
}

static void ToLowerCase(std::string& str) {
	std::transform(str.begin(), str.end(), str.begin(), [](unsigned char c) { return std::tolower(c); });
}

static std::string ToSignedNumber(int32 value) {
	return (value > 0 ? "+" : "") + std::to_string(value);
}
static std::string ToForcedSignedNumber(int32 value) {
	return (value >= 0 ? "+" : "") + std::to_string(value);
}

static std::string TextRed(std::string str, bool isRed)
{
	if (isRed) {
		str = "<Red>" + str + "</>";
	}
	return str;
}
static std::string TextRedOrange(std::string str, int32 value, int32 orangeThreshold, int32 redThreshold)
{
	if (value < redThreshold) {
		str = "<Red>" + str + "</>";
	}
	else if (value < orangeThreshold) {
		str = "<Orange>" + str + "</>";
	}
	return str;
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
 * 
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


#else

#define DEBUG_AI_VAR(VarName)

#endif

/*
 * LLM
 */
#include "HAL/LowLevelMemStats.h"
#include "HAL/LowLevelMemTracker.h"


#define STRINGIFY(x)  #x
#define TO_STR(x)  STRINGIFY(x)

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

// LLM Regiser
static void InitLLM()
{
#define REGISTER_LLM(TagName) FLowLevelMemTracker::Get().RegisterProjectTag(static_cast<int32>(EPunSimLLMTag::TagName), TEXT(TO_STR(TagName)), GET_STATFNAME(STAT_##TagName), NAME_None);
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

	//static bool IsAlmostWinter() { return Time::Ticks() % Time::TicksPerYear >= TicksPerSeason * 5 / 2; }

	static std::string SeasonName(int32 season) {
		switch (season % 4) {
		case 0: return "Spring";
		case 1: return "Summer";
		case 2: return "Autumn";
		case 3: return "Winter";
		}
		return "";
	}

	static std::string SeasonPrefix(int32 ticks)
	{
		int32_t minutesIntoSeason = (ticks / TicksPerMinute) - Seasons(ticks) * MinutesPerSeason;
		if (minutesIntoSeason >= 4) return "Late";
		if (minutesIntoSeason >= 2) return "Mid ";
		return "Early ";
	}

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

enum class MapSizeEnum : uint8
{
	Small,
	Medium,
	Large,
};
static const std::vector<FString> MapSizeNames
{
	"Small",
	"Medium",
	"Large",	
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
	for (size_t i = 0; i < MapSizeNames.size(); i++) {
		if (MapSizeNames[i] == str) {
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
enum class MapSeaLevelEnum : uint8
{
	VeryLow,
	Low,
	Medium,
	High,
	VeryHigh
};

static const std::vector<FString> MapSettingsLevelNames
{
	"Very Low",
	"Low",
	"Medium",
	"High",
	"Very High",
};

enum class MapMoistureEnum : uint8
{
	VeryLow,
	Low,
	Medium,
	High,
	VeryHigh
};
static const std::vector<FString> MapMoistureNames
{
	"Very Dry",
	"Dry",
	"Medium",
	"Wet",
	"Very Wet",
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
	Normal,
	Hard,
	Brutal,
};
static const std::vector<FString> DifficultyLevelNames
{
	"Normal",
	"Hard",
	"Brutal",
};
static const std::vector<int32> DifficultyConsumptionAdjustment
{
	0,
	25,
	50,
};

static DifficultyLevel GetDifficultyLevelFromString(const FString& str) {
	for (size_t i = 0; i < DifficultyLevelNames.size(); i++) {
		if (DifficultyLevelNames[i] == str) {
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

	//StoneTools,
	//CrudeIronTools,
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
	

	// --- End
	None,

	Money,
	Food,
	Luxury,
};

struct ResourceInfo
{
	// Need std::string since we need to copy when passing into FName (allows using name.c_str() instead of FName in some cases)
	ResourceEnum resourceEnum;
	std::string name;
	int32 basePrice;
	std::string description;

	ResourceInfo(ResourceEnum resourceEnum, std::string name, int32 cost, std::string description)
		: resourceEnum(resourceEnum), name(name), basePrice(cost), description(description) {}

	int32 resourceEnumInt() { return static_cast<int32>(resourceEnum); }
	int32 basePrice100() { return basePrice * 100; }
};

//
// BALANCE
// human consume 100 food per year, 100 food is roughly 300 coins, 1 year is 20 mins or 1200 sec..
//
static const int32 HumanFoodPerYear = 45; // 35 // 70
static const int32 FoodCost = 3;
static const int32 BaseHumanFoodCost100PerYear = 100 * HumanFoodPerYear * FoodCost;

/*
 * HumanFoodCostPerYear affects:
 * - Industrial
 * - Hunting/Gathering through AssumedFoodProductionPerYear
 * Note: Farm is affected by this
 */
static const int32 FarmFoodMultiplier100 = 100;

static const int32 WoodGatherYield_Base100 = 250;
static const int32 FarmBaseYield100 = 250;
static const int32 StoneGatherYield_Base = 4;

static const int32 CutTreeTicksBase = Time::TicksPerSecond * 10;
static const int32 HarvestDepositTicksBase = Time::TicksPerSecond * 14;

// How fast people produce money when working compare to money spent on food
// This is high because people don't spend all their time working.
static const int32 WorkRevenueToCost_Base = 150;

// TODO: WorkRevenueToCost is not yet affected by resource modifiers...

static const int32 WorkRevenue100PerYear_perMan_Base = BaseHumanFoodCost100PerYear * WorkRevenueToCost_Base / 100;
// This is the same as WorkRevenuePerManSec100_Base
static const int32 WorkRevenue100PerSec_perMan_Base = WorkRevenue100PerYear_perMan_Base / Time::SecondsPerYear;

static const int32 AssumedFoodProduction100PerYear = WorkRevenue100PerYear_perMan_Base / 3; // Food has cost of 3...

// How much a person spend on each type of luxury per year. ... /4 comes from testing... it makes houselvl 2 gives x2 income compare to house lvl 1
static const int32 HumanLuxuryCost100PerYear_ForEachType = BaseHumanFoodCost100PerYear / 8;
static const int32 HumanLuxuryCost100PerRound_ForEachType = HumanLuxuryCost100PerYear_ForEachType / Time::RoundsPerYear;

//static const int32 MineTuneFactor = 200;
//static const int32 IndustryTuneFactor = 300;
//static const int32 IndustryFactorTier1 = 350;
//static const int32 IndustryFactorTier1_5 = 400;
//static const int32 IndustryFactorTier2 = 450;

static const int32 BuildManSecCostFactor100 = 10; // Building work time is x% of the time it takes to acquire the resources

// High supply quantity = elastic demand
static const int32 DefaultYearlySupplyPerPlayer = 10000;
static const int32 MaxGoodsPricePercent = 300; // Max price is when round supply goes to 0
static const int32 MinGoodsPricePercent = 30;

static const ResourceInfo ResourceInfos[]
{
	ResourceInfo(ResourceEnum::Wood,		"Wood",		5, "Construction material and fuel for house furnaces"),
	ResourceInfo(ResourceEnum::Stone,		"Stone",	7,  "Construction material mined from Quarry or Stone Outcrop"),
	
	ResourceInfo(ResourceEnum::Orange,		"Orange",	FoodCost, "Edible, tangy fruit obtained from Forest and Orchards"),
	ResourceInfo(ResourceEnum::Papaya,		"Papaya",	FoodCost, "Sweet, tropical fruit obtained from Jungle and Orchards"),
	
	ResourceInfo(ResourceEnum::Wheat,		"Wheat",	FoodCost, "Edible grain that can be brewed into Beer"),
	ResourceInfo(ResourceEnum::Milk,		"Milk",		FoodCost, "Yummy white liquid from cows, sheep, or llamas"),
	ResourceInfo(ResourceEnum::Mushroom,	"Mushroom",	FoodCost, "Delicious, earthy tasting fungus"),
	ResourceInfo(ResourceEnum::Hay,			"Hay",		1, "Dried grass that can be used as animal feed"),

	ResourceInfo(ResourceEnum::Paper,		"Paper",	8, "Used for research and book making"),
	ResourceInfo(ResourceEnum::Clay,		"Clay",		3, "Fine-grained earth used to make Pottery and Bricks"),
	ResourceInfo(ResourceEnum::Brick,		"Brick",	10, "Sturdy, versatile construction material"),

	ResourceInfo(ResourceEnum::Coal,		"Coal",		6, "Fuel used to heat houses or smelt ores. When heating houses, provides x2 heat vs. Wood"),
	ResourceInfo(ResourceEnum::IronOre,		"Iron Ore",		5, "Valuable ore that can be smelted into Iron Bar"),
	ResourceInfo(ResourceEnum::Iron,		"Iron Bar",		18, "Sturdy bar of metal used in construction and tool-making."),
	ResourceInfo(ResourceEnum::Furniture,	"Furniture", 8,  "Luxury tier 1 used for housing upgrade. Make house a home."),
	ResourceInfo(ResourceEnum::Chocolate,	"Chocolate", 20,  "Luxury tier 2 used for housing upgrade. Everyone's favorite confectionary."),

	//ResourceInfo(ResourceEnum::StoneTools,		"Stone Tools",		15,  "Lowest-grade tool made by Stone Tool Shop."),
	//ResourceInfo(ResourceEnum::CrudeIronTools,	"Crude Iron Tools",	15,  "Medium-grade tool made by Blacksmith using Iron Ore and Wood."),
	ResourceInfo(ResourceEnum::SteelTools,		"Steel Tool",			27,  "High-grade tool made by Blacksmith from Iron Bars and Wood"),
	ResourceInfo(ResourceEnum::Herb,				"Medicinal Herb",				5, 		"Medicinal plant used to heal sickness"),
	ResourceInfo(ResourceEnum::Medicine,			"Medicine",			10,  "Potent Medicinal Herb extract used to cure sickness"),
	
	
	//ResourceInfo(ResourceEnum::Tools,		"Tools", 25, 100, "Construction material"),
	ResourceInfo(ResourceEnum::Fish,			"Fish", FoodCost, "Tasty catch from the sea/river"), // Fish is all year round... shouldn't be very high yield...

	//ResourceInfo(ResourceEnum::WhaleMeat, "WhaleMeat", 7, 100, "Luxury food obtained from Fishing Lodge"),
	ResourceInfo(ResourceEnum::Grape,		"Grape", FoodCost, "Juicy, delicious fruit used in Wine-making."),
	ResourceInfo(ResourceEnum::Wine,		"Wine", 30, "Luxury tier 2 used for housing upgrade. Alcoholic drink that makes everything tastes better."),
	ResourceInfo(ResourceEnum::Shroom,		"Shroom", 15, "Psychedelic mushroom that can bring you on a hallucination trip. Luxury used in upgrading houses"),

	ResourceInfo(ResourceEnum::Pork,			"Pork", FoodCost, "Delicious meat from farmed Pigs"),
	ResourceInfo(ResourceEnum::GameMeat,		"Game Meat", FoodCost, "Delicious meat from wild animals"),
	ResourceInfo(ResourceEnum::Beef,			"Beef", FoodCost, "Delicious meat from ranched Cattle"),
	ResourceInfo(ResourceEnum::Lamb,			"Lamb", FoodCost, "Delicious meat from ranched Sheep"),
	ResourceInfo(ResourceEnum::Cocoa,		"Cocoa", 7, "Raw cocoa used in Chocolate-making"),

	ResourceInfo(ResourceEnum::Wool,			"Wool", 7, "Fine, soft fiber used to make Clothes"),
	ResourceInfo(ResourceEnum::Leather,		"Leather", 5, "Animal skin that can be used to make Clothes"),
	ResourceInfo(ResourceEnum::Cloth,		"Clothes", 15, "Luxury tier 2 used for housing upgrade. Provide cover and comfort."),

	ResourceInfo(ResourceEnum::GoldOre,		"Gold Ore", 10, "Precious ore that can be smelted into Gold Bar"),
	ResourceInfo(ResourceEnum::GoldBar,		"Gold Bar", 25, "Precious metal that can be minted into money or crafted into Jewelry"),

	ResourceInfo(ResourceEnum::Beer,			"Beer", 8, "Luxury tier 1 used for housing upgrade. The cause and solution to all life's problems."),
	//ResourceInfo(ResourceEnum::Barley,		"Barley", FoodCost, 100, "Edible grain, obtained from farming. Ideal for brewing Beer"),
	//ResourceInfo(ResourceEnum::Oyster,		"Oyster", 7, IndustryTuneFactor + 50, "A delicacy from the Sea"),
	ResourceInfo(ResourceEnum::Cannabis,		"Cannabis", 5, "Luxury tier 1 used for housing upgrade."),
	//ResourceInfo(ResourceEnum::Truffle,		"Truffle", 7, IndustryTuneFactor + 50, "Construction material"),
	//ResourceInfo(ResourceEnum::Coconut,		"Coconut", 7, IndustryTuneFactor + 50, "Hard shell fruit with sweet white meat and delicious juice"),
	ResourceInfo(ResourceEnum::Cabbage,		"Cabbage", FoodCost, "Healthy green vegetable."),

	ResourceInfo(ResourceEnum::Pottery,		"Pottery", 8, "Luxury tier 1 used for housing upgrade. Versatile pieces of earthenware."),

	ResourceInfo(ResourceEnum::Flour,		"Wheat Flour", 5, "Ingredient used to bake Bread"),
	ResourceInfo(ResourceEnum::Bread,		"Bread", 3, "Delicious food baked from Wheat Flour"), // 3 bread from 1 flour
	ResourceInfo(ResourceEnum::Gemstone,	"Gemstone", 20, "Precious stone that can be crafted into Jewelry"),
	ResourceInfo(ResourceEnum::Jewelry,		"Jewelry", 70, "Luxury tier 3 used for housing upgrade. Expensive adornment of Gold and Gems."),

	// June 9
	
	ResourceInfo(ResourceEnum::Cotton,				"Cotton", 7, "Raw material used to make Cotton Fabric."),
	ResourceInfo(ResourceEnum::CottonFabric,		"Cotton Fabric", 7, "Fabric used by tailors to make Clothes."),
	ResourceInfo(ResourceEnum::DyedCottonFabric,	"Dyed Cotton Fabric", 17, "Fancy fabric used by tailors to make Fashionable Clothes."),
	ResourceInfo(ResourceEnum::LuxuriousClothes,	"Fashionable Clothes", 30, "Luxury tier 3 used for housing upgrade."),
	
	ResourceInfo(ResourceEnum::Honey,		"Honey", FoodCost, "Delicious, viscous liquid produced by bees."),
	ResourceInfo(ResourceEnum::Beeswax,		"Beeswax", 7, "Raw material used to make Candles."),
	ResourceInfo(ResourceEnum::Candle,		"Candles", 15, "Luxury tier 2 used for housing upgrade."),
	
	ResourceInfo(ResourceEnum::Dye,		"Dye", 7, "Colored substance used for printing or dyeing clothes."),
	ResourceInfo(ResourceEnum::Book,		"Book", 30, "Luxury tier 3 used for housing upgrade."),

};

static const int ResourceEnumCount = _countof(ResourceInfos);

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
	return ResourceInfos[static_cast<int>(resourceEnum)].name;
}

inline FString ResourceNameF(ResourceEnum resourceEnum) {
	return FString(ResourceInfos[static_cast<int>(resourceEnum)].name.c_str());
}


inline ResourceEnum FindResourceEnumByName(std::string name)
{
	for (int32 i = 0; i < ResourceEnumCount; i++) {
		ResourceEnum resourceEnum = static_cast<ResourceEnum>(i);
		if (ResourceName(resourceEnum) == name) {
			return resourceEnum;
		}
	}
	return ResourceEnum::None;
}

// Sorted resource
static std::vector<ResourceInfo> GetSortedNameResourceEnum()
{
	std::vector<std::string> resourceNames;
	for (int32 i = 0; i < ResourceEnumCount; i++) {
		resourceNames.push_back(ResourceName(static_cast<ResourceEnum>(i)));
	}
	std::sort(resourceNames.begin(), resourceNames.end());

	std::vector<ResourceInfo> results;
	for (const std::string& name : resourceNames) {
		ResourceEnum resourceEnum = FindResourceEnumByName(name);
		PUN_CHECK(resourceEnum != ResourceEnum::None);
		results.push_back(GetResourceInfo(resourceEnum));
	}
	
	return results;
}
static const std::vector<ResourceInfo> SortedNameResourceEnum = GetSortedNameResourceEnum();


static const ResourceEnum FoodEnums[] =
{
	// Arrange food from high to low grabbing priority
	ResourceEnum::Bread,
	ResourceEnum::Cabbage,
	ResourceEnum::Papaya,
	ResourceEnum::Fish,

	ResourceEnum::Pork,
	ResourceEnum::GameMeat,
	ResourceEnum::Beef,
	ResourceEnum::Lamb,
	
	ResourceEnum::Honey,
	ResourceEnum::Orange,
	ResourceEnum::Milk,
	ResourceEnum::Mushroom,
	ResourceEnum::Wheat,
	
	ResourceEnum::Grape,
};

static bool IsFoodEnum(ResourceEnum resourceEnum) {
	return GetResourceInfo(resourceEnum).basePrice == FoodCost; // !!! No other good's base cost should be the same as food
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
	switch (resourceEnum)
	{
	// Disable
	case ResourceEnum::Shroom:
		return false;
	default:
		return true;
	}
};


static const std::vector<ResourceEnum> FuelEnums
{
	ResourceEnum::Wood,
	ResourceEnum::Coal,
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
	{ResourceEnum::Beer, ResourceEnum::Cannabis, ResourceEnum::Furniture, ResourceEnum::Pottery},
	{ResourceEnum::Cloth, ResourceEnum::Wine, ResourceEnum::Chocolate, ResourceEnum::Candle},
	{ResourceEnum::Book, ResourceEnum::LuxuriousClothes, ResourceEnum::Jewelry},
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

static std::string LuxuryResourceTip(int32 tier)
{
	std::stringstream luxuryTip;
	luxuryTip << "<Bold>Luxury tier " << tier << ":</>";
	for (size_t i = 0; i < TierToLuxuryEnums[tier].size(); i++) {
		luxuryTip << "\n " << GetResourceInfo(TierToLuxuryEnums[tier][i]).name;
	}
	return luxuryTip.str();
}


static const std::vector<ResourceEnum> ToolResources = {
	ResourceEnum::SteelTools,
	//ResourceEnum::CrudeIronTools,
	//ResourceEnum::StoneTools,
};
static const std::vector<ResourceEnum> MedicineResources = {
	ResourceEnum::Medicine,
	ResourceEnum::Herb,
};


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

enum class IntercityTradeOfferEnum : uint8
{
	None,
	BuyWhenBelow,
	SellWhenAbove,
};
static std::vector<std::string> IntercityTradeOfferEnumName
{
	"None",
	"Buy When Below",
	"Sell When Above",
};
static std::string GetIntercityTradeOfferEnumName(IntercityTradeOfferEnum offerEnum) {
	return IntercityTradeOfferEnumName[static_cast<int>(offerEnum)];
}
static IntercityTradeOfferEnum GetIntercityTradeOfferEnumFromName(std::string nameIn)
{
	for (size_t i = 0; i < IntercityTradeOfferEnumName.size(); i++) {
		if (IntercityTradeOfferEnumName[i] == nameIn) {
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

static std::string MaybeRedText(std::string text, bool isRed)
{
	if (isRed) {
		return "<Red>" + text + "</>";
	}
	return text;
};


enum class ResourceHolderType : uint8
{
	Manual, // Refilled only from explicit command. House for example.
	Requester,
	Storage,
	Provider,
	Drop,
	Dead,

	
};

static const std::vector<std::string> ResourceHolderTypeName
{
	"Manual", // Refilled only from explicit command. House for example.
	"Requester",
	"Storage",
	"Provider",
	"Drop",
	"Dead",

	
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
		return ResourceInfos[static_cast<int>(resourceEnum)].name;
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
	int32 amount;
	WorldTile2 tile;

	FoundResourceHolderInfo() : info(ResourceHolderInfo::Invalid()), amount(0) {}
	FoundResourceHolderInfo(ResourceHolderInfo info, int32_t amount, WorldTile2 tile, int32_t distance = 0) : info(info), amount(amount), tile(tile) {}

	bool isValid() {
		return info.isValid();
	}

	static const FoundResourceHolderInfo Invalid() { return FoundResourceHolderInfo(ResourceHolderInfo::Invalid(), -1, WorldTile2::Invalid, -1); }

	std::string resourceName() {
		return ResourceInfos[static_cast<int>(info.resourceEnum)].name;
	}
	std::string ToString() {
		return "[Holder: " + resourceName() + " id:" + std::to_string(info.holderId) + " amount:" + std::to_string(amount) + " tile:" + tile.ToString() + "]";
	}
};
struct FoundResourceHolderInfos
{
	std::vector<FoundResourceHolderInfo> foundInfos;

	FoundResourceHolderInfos(std::vector<FoundResourceHolderInfo> foundInfos) : foundInfos(foundInfos) {}

	bool hasInfos() {
		return foundInfos.size() > 0;
	}

	int32 amount() {
		int32_t total = 0;
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
	Colony,
	InventorsWorkshop,
	IntercityRoad,

	// August 16
	FakeTownhall,
	FakeTribalVillage,
	ChichenItza,
	

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
	
	DesertPilgrim,

	WheatSeed,
	CabbageSeed,
	HerbSeed,
	
	CannabisSeeds,
	GrapeSeeds,
	CocoaSeeds,
	CottonSeeds,
	DyeSeeds,

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
	
	HappyBreadDay,
	BlingBling,
	GoldRush,

	//! Rare
	MiddleClassTax,
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
	{ CardEnum::FruitGatherer, 4 },
	{ CardEnum::MushroomFarm, 5 },
	{ CardEnum::HuntingLodge, 3 },
	{ CardEnum::Fisher, 5 },
	{ CardEnum::Farm, 3 },
	
	{ CardEnum::RanchBarn, 8 },
	{ CardEnum::RanchPig, 8 },
	{ CardEnum::RanchSheep, 12 },
	{ CardEnum::RanchCow, 15 },
	
	{ CardEnum::GoldMine, 20 },
	{ CardEnum::Quarry, 10 },
	{ CardEnum::CoalMine, 10 },
	{ CardEnum::IronMine, 10 },
	{ CardEnum::Forester, 20 },
	
	{ CardEnum::Bank, 20 },
	{ CardEnum::IronSmelter, 50 },
	{ CardEnum::GoldSmelter, 50 },
	{ CardEnum::Mint, 20 },
	{ CardEnum::InventorsWorkshop, 10 },

	{ CardEnum::Blacksmith, 10 },
	{ CardEnum::MedicineMaker, 10 },
	
	{ CardEnum::Garden, 5 },

	{ CardEnum::IronSmelterGiant, 100},

	{ CardEnum::BeerBrewery, 12 },
	{ CardEnum::BeerBreweryFamous, 35 },
	
	{ CardEnum::ClayPit, 12 },
	{ CardEnum::Potter, 12 },
	{ CardEnum::FurnitureWorkshop, 12 },
	{ CardEnum::Tailor, 20 },
	{ CardEnum::Chocolatier, 150 },
	{ CardEnum::Winery, 100 },

	{ CardEnum::Windmill, 20 },
	{ CardEnum::Bakery, 20 },
	{ CardEnum::GemstoneMine, 30 },
	{ CardEnum::Jeweler, 30 },

	{ CardEnum::Beekeeper, 10 },
	{ CardEnum::Brickworks, 20 },
	{ CardEnum::CandleMaker, 20 },
	{ CardEnum::CottonMill, 30 },
	{ CardEnum::PrintingPress, 30 },
	
	{ CardEnum::Library, 15 },
	{ CardEnum::School, 18 },
	{ CardEnum::Theatre, 25 },
	{ CardEnum::Tavern, 15 },

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
	std::string name;
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

	std::string description;
	std::string miniDescription;

	int32 baseCardPrice = 0;

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
		std::string name,
		WorldTile2 size,
		ResourceEnum input1,
		ResourceEnum input2,
		ResourceEnum produce,
		int32 productionBatch,
		int32 workerCount,

		std::vector<int32_t> constructionResources,

		std::string description,
		std::string miniDescriptionIn = ""
	) :
		cardEnum(buildingEnum),
		name(name),
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

		if (miniDescription == "") {
			miniDescription = description;
		}

		// Farm has large area to clear.
		if (buildingEnum == CardEnum::Farm) {
			maxBuilderCount = 2;
		}
		else if (buildingEnum == CardEnum::Fort ||
				buildingEnum == CardEnum::Colony) 
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
		//case CardEnum::Tailor:
		//	baseCardPrice *= 2;
		//	break;

			CASE(House, 0);
			CASE(StorageYard, 0);
			CASE(DirtRoad, 0);
			CASE(StoneRoad, 0);
			CASE(Farm, 0);
			
			CASE(HumanitarianAidCamp, 500);
			
			CASE(FlowerBed, 100);
			CASE(GardenShrubbery1, 100);
			CASE(GardenCypress, 200);

			CASE(Fort, 500);
			CASE(Colony, 2000);
#undef CASE
		default:
			break;
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
			int32 upkeepPerSec100 = GetCardUpkeepBase(buildingEnum) * 100 / Time::SecondsPerRound;
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
				std::string name, int32
				cardPrice, std::string
				description) :
	
				cardEnum(buildingEnum),
				name(name),
				description(description),
				baseCardPrice(cardPrice)
	{
		if (miniDescription == "") {
			miniDescription = description;
		}
	}
};

TileArea BuildingArea(WorldTile2 centerTile, WorldTile2 size, Direction faceDirection);
WorldTile2 GetBuildingCenter(TileArea area, Direction faceDirection);

static const BldInfo BuildingInfo[]
{
	// Note that size is (y, x) since y is horizontal and x is vertical in UE4
	BldInfo(CardEnum::House,			"House",				WorldTile2(6, 6),	ResourceEnum::None, ResourceEnum::None,	ResourceEnum::None,		0, 0,	{20,0,0},	"Protect people from cold. Extra houses boost population growth."),
	BldInfo(CardEnum::StoneHouse,	"Stone House",			WorldTile2(4, 4),	ResourceEnum::None, ResourceEnum::None, ResourceEnum::None,		 0, 0,	{0,40,0},	""),
	BldInfo(CardEnum::FruitGatherer,	"Fruit Gatherer",		WorldTile2(4, 4),	ResourceEnum::None, ResourceEnum::None, ResourceEnum::None,		0, 2,	{30,0,0},	"Gather fruits from trees and bushes."),
	BldInfo(CardEnum::Townhall,		"Townhall",				WorldTile2(12, 12),	ResourceEnum::None, ResourceEnum::None, ResourceEnum::None,		 0, 0,	{0,0,0},	"The administrative center of your town."),
	BldInfo(CardEnum::StorageYard,	"Storage Yard",			WorldTile2(2, 2),	ResourceEnum::None, ResourceEnum::None, ResourceEnum::None,		 0, 0,	{10,0,0},	"Store resources."),

	BldInfo(CardEnum::GoldMine,		"Gold Mine",			WorldTile2(5, 5),	ResourceEnum::None, ResourceEnum::None, ResourceEnum::GoldOre,	 10, 3,	{50,50,0},	"Mine Gold Ores from Gold Deposit."),
	BldInfo(CardEnum::Quarry,		"Quarry",				WorldTile2(5, 5),	ResourceEnum::None, ResourceEnum::None, ResourceEnum::Stone,	 10, 3,	{110,0,0},	"Mine Stone from mountain."),
	BldInfo(CardEnum::IronStatue,	"Stone Statue",			WorldTile2(4, 4),	ResourceEnum::None, ResourceEnum::None, ResourceEnum::None,		 0, 0,	{0,110,0},	"...Show off"),
	BldInfo(CardEnum::Bank,			"Bank",					WorldTile2(4, 5),	ResourceEnum::None, ResourceEnum::None, ResourceEnum::None,		 0, 0,	{110,110,0},	"+10 <img id=\"Coin\"/> for surrounding level 2+ houses."),
	BldInfo(CardEnum::IceAgeSpire,	"Ice Age Spire",		WorldTile2(8, 8),	ResourceEnum::None, ResourceEnum::None, ResourceEnum::None,		 0, 0,	{0,300,200},	"Spire for worshipping Groth, the god of destruction. Decrease global temperature by -10 C.", "Decrease global temperature by -10 C."),

	BldInfo(CardEnum::Farm,			"Farm",					WorldTile2(8, 8),	ResourceEnum::None, ResourceEnum::None, ResourceEnum::None,		 10, 1,	{20,0,0},	"Grow food/raw materials. Harvest during autumn."),
	BldInfo(CardEnum::MushroomFarm,	"Mushroom Farm",		WorldTile2(8, 8),	ResourceEnum::Wood, ResourceEnum::None,	ResourceEnum::Mushroom,	20, 2,	{70,0,0},	"Farm Mushroom using wood."),
	BldInfo(CardEnum::Fence,			"Fence",				WorldTile2(1, 1),	ResourceEnum::None, ResourceEnum::None, ResourceEnum::None,		 0, 0,	{0,0,0},	"Fence used to keep farm animals in ranches, or wild animals away from farm crops", "Block unit from walking on tile."),
	BldInfo(CardEnum::FenceGate,		"Fence Gate",			WorldTile2(1, 1),	ResourceEnum::None, ResourceEnum::None, ResourceEnum::None,		 0, 0,	{0,0,0},	"Fence gate blocks animals from entering while letting people through.", "Block animals from walking on tile, while letting citizens through."),
	BldInfo(CardEnum::Bridge,		"Bridge",				WorldTile2(1, 1),	ResourceEnum::None, ResourceEnum::None, ResourceEnum::None,		 0, 0,	{0,0,0},	"Allow citizens to cross over water."),
	
	BldInfo(CardEnum::Forester,		"Forester",				WorldTile2(5, 5),	ResourceEnum::None, ResourceEnum::None, ResourceEnum::None,		 0, 2,	{50,50,0},	"Chop/plant trees within your territory."),

	BldInfo(CardEnum::CoalMine,		"Coal Mine",			WorldTile2(5, 5),	ResourceEnum::None, ResourceEnum::None, ResourceEnum::Coal,		10, 3,	{50,30,0},	"Mine Coal from Coal Deposits."),
	BldInfo(CardEnum::IronMine,		"Iron Mine",			WorldTile2(5, 5),	ResourceEnum::None, ResourceEnum::None, ResourceEnum::IronOre,	 10, 3,	{50,30,0},	"Mine Iron Ores from Iron Deposits."),
	BldInfo(CardEnum::SmallMarket,	"Small Market",			WorldTile2(5, 8),	ResourceEnum::None, ResourceEnum::None, ResourceEnum::None,		 0, 0,	{80,0,0},	"Sell goods to people for <img id=\"Coin\"/>"),
	BldInfo(CardEnum::PaperMaker,	"Paper Maker",			WorldTile2(6, 6),	ResourceEnum::Wood, ResourceEnum::None, ResourceEnum::Paper,		 20, 3,	{80,0,0},	"Produce Paper."),
	BldInfo(CardEnum::IronSmelter,	"Iron Smelter",			WorldTile2(5, 6),	ResourceEnum::Coal,ResourceEnum::IronOre,ResourceEnum::Iron,		 10, 5,	{80,80,0},	"Smelt Iron Ores into Iron Bars."),


	BldInfo(CardEnum::StoneToolShop,	"Stone Tool Shop",		WorldTile2(5, 8),	ResourceEnum::Stone, ResourceEnum::Wood, ResourceEnum::SteelTools,		 10, 2,	{50,20,0},	"."),
	BldInfo(CardEnum::Blacksmith,	"Blacksmith",			WorldTile2(5, 8),	ResourceEnum::Iron, ResourceEnum::Wood, ResourceEnum::SteelTools,		 10, 2,	{50,50,20},	"Forge Tools from Iron Bars and Wood."),
	BldInfo(CardEnum::Herbalist,		"Herbalist",			WorldTile2(4, 4),	ResourceEnum::None, ResourceEnum::None, ResourceEnum::None,		 10, 2,	{50,30,0},	"."),
	BldInfo(CardEnum::MedicineMaker,	"Medicine Maker",		WorldTile2(4, 4),	ResourceEnum::Herb, ResourceEnum::None, ResourceEnum::Medicine,		 10, 2,	{50,50,20},	"Make Medicine from Medicinal Herb."),

	
	BldInfo(CardEnum::FurnitureWorkshop,"Furniture Workshop",	WorldTile2(6, 7),	ResourceEnum::Wood, ResourceEnum::None, ResourceEnum::Furniture,	 10, 2,	{50,20,0},	"Make Furniture from Wood."),
	BldInfo(CardEnum::Chocolatier,	"Chocolatier",			WorldTile2(6, 8),	ResourceEnum::Cocoa, ResourceEnum::Milk, ResourceEnum::Chocolate,	 10, 7,	{110, 110, 110},	"Make Chocolate from Milk and Cocoa."),

	// Decorations
	BldInfo(CardEnum::Garden,		"Garden",				WorldTile2(4, 4),	ResourceEnum::None, ResourceEnum::None, ResourceEnum::None,		 0, 0,	{30,0,0},	"Increase the surrounding appeal by 5 within 5 tiles radius."),
	
	BldInfo(CardEnum::BoarBurrow,	"Boar Burrow",			WorldTile2(3, 3),	ResourceEnum::None, ResourceEnum::None, ResourceEnum::None,		 0, 0,	{0,0,0},	"A cozy burrow occupied by a Boar family."),

	BldInfo(CardEnum::DirtRoad,		"Dirt Road",			WorldTile2(1, 1),	ResourceEnum::None, ResourceEnum::None, ResourceEnum::None,		 0, 0,	{0,0,0},	"A crude road.\n +20% movement speed."),
	BldInfo(CardEnum::StoneRoad,		"Stone Road",			WorldTile2(1, 1),	ResourceEnum::None, ResourceEnum::None, ResourceEnum::None,		 0, 0,	{0,2,0},	"A sturdy road.\n +30% movement speed."),
	BldInfo(CardEnum::TrapSpike,		"Spike Trap",			WorldTile2(1, 1),	ResourceEnum::None, ResourceEnum::None, ResourceEnum::None,		 0, 0,	{10,0,0},	"Trap that can be used to hurt/disable animals or humans."),

	BldInfo(CardEnum::Fisher,		"Fishing Lodge",		WorldTile2(6, 4),	ResourceEnum::None, ResourceEnum::None, ResourceEnum::Fish,		 10, 2,	{70,0,0},	"Catch Fish from seas, lakes or rivers."),
	BldInfo(CardEnum::BlossomShrine,	"Blossom Shrine",		WorldTile2(3, 3),	ResourceEnum::None, ResourceEnum::None, ResourceEnum::None,		 0, 0,	{0,10,0},	"Increases nearby tree growth/fruit by 100%. Note that this will not help land without trees. While increasing growth helps trees emit more seed, no seed will spawn without any existing tree.", "Increases nearby tree growth / fruit by 100 % ."),
	BldInfo(CardEnum::Winery,		"Winery",				WorldTile2(6, 6),	ResourceEnum::Grape, ResourceEnum::None, ResourceEnum::Wine,	10, 5,	{200,0,80},	"Ferment Grapes into Wine."),

	BldInfo(CardEnum::Library,		"Library",				WorldTile2(4, 5),	ResourceEnum::None, ResourceEnum::None, ResourceEnum::None,		 0, 0,	{80,20,0},	"+2 <img id=\"Science\"/> for surrounding level 2+ houses (effect doesn't stack)."),
	BldInfo(CardEnum::School,		"School",				WorldTile2(4, 7),	ResourceEnum::None, ResourceEnum::None, ResourceEnum::None,		 0, 0,	{80,50,10},	"+3 <img id=\"Science\"/> for surrounding level 3+ houses (effect doesn't stack)."),

	BldInfo(CardEnum::Theatre,		"Theatre",				WorldTile2(7, 6),	ResourceEnum::None, ResourceEnum::None, ResourceEnum::None,		 0, 0,	{80,30,0},	"Increase visitor's Fun. Visitors must live in a level 2+ house. Service quality 120."),
	BldInfo(CardEnum::Tavern,		"Tavern",				WorldTile2(5, 5),	ResourceEnum::None, ResourceEnum::None, ResourceEnum::None,		 0, 0,	{50,30,0},	"Increase visitor's Fun. Service quality 90."),

	BldInfo(CardEnum::Tailor,		"Tailor",				WorldTile2(5, 6),	ResourceEnum::Leather, ResourceEnum::None, ResourceEnum::Cloth,	 10, 5,	{80, 80, 30},	"Make Clothes from Leather or Wool."),

	BldInfo(CardEnum::CharcoalMaker,"Charcoal Burner",		WorldTile2(4, 5),	ResourceEnum::Wood, ResourceEnum::None, ResourceEnum::Coal,		 10, 2,	{40,0,0},		"Burn Wood into Coal which provides x2 heat when heating houses."),
	BldInfo(CardEnum::BeerBrewery,	"Beer Brewery",			WorldTile2(5, 5),	ResourceEnum::Wheat, ResourceEnum::None, ResourceEnum::Beer,		 10, 2,	{40,30,0},		"Brew Wheat into Beer."),
	BldInfo(CardEnum::ClayPit,		"Claypit",				WorldTile2(6, 6),	ResourceEnum::None, ResourceEnum::None, ResourceEnum::Clay,		 10, 2,	{20,20,0},		"Produce Clay, used to make Pottery or Brick. Must be built next to a river."),
	BldInfo(CardEnum::Potter,		"Potter",				WorldTile2(5, 4),	ResourceEnum::Clay, ResourceEnum::None, ResourceEnum::Pottery,		 10, 2,	{20,40,0},		"Make Pottery from Clay."),
	BldInfo(CardEnum::HolySlimeRanch,"Holy Slime Ranch",		WorldTile2(10, 10),	ResourceEnum::None, ResourceEnum::None, ResourceEnum::None,		 10, 0,	{30,0,0},		"Raise holy slime. Bonus from nearby slime ranches/pyramid"),

	BldInfo(CardEnum::TradingPost,	"Trading Post",			WorldTile2(8, 8),	ResourceEnum::None, ResourceEnum::None, ResourceEnum::None,		 0, 0,	{50, 50, 0},	"Trade resources with world market."),
	BldInfo(CardEnum::TradingCompany,"Trading Company",		WorldTile2(6, 6),	ResourceEnum::None, ResourceEnum::None, ResourceEnum::None,		 0, 0,	{30, 30, 0},	"Automatically trade resources with world market at lower fees."),
	BldInfo(CardEnum::TradingPort,	"Trading Port",			WorldTile2(10, 9),	ResourceEnum::None, ResourceEnum::None, ResourceEnum::None,		 0, 0,	{50, 50, 0},	"Trade resources with world market. Must be built on the coast."),
	BldInfo(CardEnum::CardMaker,		"Scholars Office",		WorldTile2(5, 5),	ResourceEnum::Paper, ResourceEnum::None, ResourceEnum::None,		 0, 2,	{50, 50, 50},	"Craft a Card from Paper."),
	BldInfo(CardEnum::ImmigrationOffice,	"Immigration Office",WorldTile2(5, 6),	ResourceEnum::None, ResourceEnum::None, ResourceEnum::None,		 0, 2,	{50, 0, 0},	"Attract new immigrants."),

	BldInfo(CardEnum::ThiefGuild,	"Thief Guild",			WorldTile2(5, 7),	ResourceEnum::None, ResourceEnum::None, ResourceEnum::None,		 0, 0,	{50,0,0},	"Steal resources from another player, or counter steal."),
	BldInfo(CardEnum::SlimePyramid,	"Pyramid",				WorldTile2(14, 14),	ResourceEnum::None, ResourceEnum::None, ResourceEnum::None,		 0, 0,	{50,0,0},	"Bonus for each holy slime huts nearby"),

	BldInfo(CardEnum::LovelyHeartStatue,"LovelyHeartStatue",	WorldTile2(4, 4),	ResourceEnum::None, ResourceEnum::None, ResourceEnum::None,		 0, 0,	{0,0,0},	"Mysterious giant hearts."),

	BldInfo(CardEnum::HuntingLodge,	"Hunting Lodge",		WorldTile2(4, 5),	ResourceEnum::None, ResourceEnum::None, ResourceEnum::None,		 0, 2,	{30,0,0},	"Hunt wild animals for food."),
	BldInfo(CardEnum::RanchBarn,		"Ranch Barn",			WorldTile2(4, 4),	ResourceEnum::None, ResourceEnum::None, ResourceEnum::None,		 0, 1,	{50,20,0},	"Rear animals for food."),

	BldInfo(CardEnum::RanchPig,		"Pig Ranch",			WorldTile2(16, 16),	ResourceEnum::None, ResourceEnum::None, ResourceEnum::None,		 0, 1,	{30,10,0},	"Rear Pigs for food."),
	BldInfo(CardEnum::RanchSheep,	"Sheep Ranch",			WorldTile2(16, 16),	ResourceEnum::None, ResourceEnum::None, ResourceEnum::None,		 0, 1,	{30,20,0},	"Rear Sheep for food and Wool."),
	BldInfo(CardEnum::RanchCow,		"Cattle Ranch",			WorldTile2(16, 16),	ResourceEnum::None, ResourceEnum::None, ResourceEnum::None,	0, 1,	{30,10,10},	"Rear Cows for Milk"),

	
	BldInfo(CardEnum::GoldSmelter,	"Gold Smelter",			WorldTile2(5, 6),	ResourceEnum::Coal,ResourceEnum::GoldOre,ResourceEnum::GoldBar,	 10, 5,	{80,80,0},	"Smelt Gold Ores into Gold Bars."),
	BldInfo(CardEnum::Mint,			"Mint",					WorldTile2(4, 6),	ResourceEnum::GoldBar,ResourceEnum::None,ResourceEnum::None,		 0, 2,	{80,80,0},	"Mint Gold Bars into <img id=\"Coin\"/>."),

	BldInfo(CardEnum::BarrackClubman,	"Clubman Barrack",	WorldTile2(7, 7),	ResourceEnum::None, ResourceEnum::None, ResourceEnum::None,		0, 2,	{30,0,0},	"Train Clubmen."),
	BldInfo(CardEnum::BarrackSwordman,	"Knight Barrack",	WorldTile2(7, 7),	ResourceEnum::Iron, ResourceEnum::None, ResourceEnum::None,		0, 4,	{80,30,30},	"Consume Iron to increase <img id=\"Influence\"/>."),
	BldInfo(CardEnum::BarrackArcher,		"Archer Barrack",	WorldTile2(7, 7),	ResourceEnum::Wood, ResourceEnum::None, ResourceEnum::None,		0, 2,	{50,30,0},	"Consume Wood to increase <img id=\"Influence\"/>."),

	BldInfo(CardEnum::ShrineWisdom,	"Shrine of Wisdom",		WorldTile2(4, 4),	ResourceEnum::None,ResourceEnum::None,ResourceEnum::None,	 0, 0,	{0, 50, 0},	"+1 Wild Card to the deck."),
	BldInfo(CardEnum::ShrineLove,	"Shrine of Love",		WorldTile2(4, 4),	ResourceEnum::None,ResourceEnum::None,ResourceEnum::None,		 0, 0,	{0, 80, 10},	"+5<img id=\"Smile\"/> to surrounding houses."),
	BldInfo(CardEnum::ShrineGreed,	"Shrine of Greed",		WorldTile2(4, 4),	ResourceEnum::None,ResourceEnum::None,ResourceEnum::None,		 0, 0,	{0, 80, 10},	"+20<img id=\"Coin\"/> per round and -5<img id=\"Smile\"/> to surrounding houses."),


	BldInfo(CardEnum::HellPortal,	"Hell Portal",			WorldTile2(6, 6),	ResourceEnum::None, ResourceEnum::None, ResourceEnum::None,		0, 0,	{100,100,50},	"Open a portal to hell to harvest its riches. Gain 200 <img id=\"Coin\"/> each round, but may explode letting demons through.", "Gain 200 <img id=\"Coin\"/> each round, but may explode letting demons through."),
	BldInfo(CardEnum::LaborerGuild,	"Laborer's Guild",	WorldTile2(4, 6),		ResourceEnum::None, ResourceEnum::None, ResourceEnum::None,		0, 0,	{20, 0, 0},	"Dispatch laborers to haul resources and cut trees. Laborers from the guild has x2 inventory carry capacity."),

	BldInfo(CardEnum::HumanitarianAidCamp,	"Humanitarian Aid Camp",	WorldTile2(4, 4), ResourceEnum::None, ResourceEnum::None, ResourceEnum::None, 0, 0, {0, 0, 0}, "Supply your neighbor with 100 food."),

	BldInfo(CardEnum::RegionTribalVillage,	"Tribal Village",	WorldTile2(9, 9), ResourceEnum::None, ResourceEnum::None, ResourceEnum::None, 0, 0, {0, 0, 0}, "Tribal village that might want to join your city."),
	BldInfo(CardEnum::RegionShrine,	"Ancient Shrine",	WorldTile2(4, 6), ResourceEnum::None, ResourceEnum::None, ResourceEnum::None, 0, 0, {0, 0, 0}, "Shrines built by the ancients that grants wisdom to those who seek it."),
	BldInfo(CardEnum::RegionPort,	"Port Settlement",	WorldTile2(12, 9), ResourceEnum::None, ResourceEnum::None, ResourceEnum::None, 0, 0, {0, 0, 0}, "Port settlement specialized in trading certain resources."),
	BldInfo(CardEnum::RegionCrates,	"Crates",			WorldTile2(4, 6), ResourceEnum::None, ResourceEnum::None, ResourceEnum::None, 0, 0, {0, 0, 0}, "Crates that may contain valuable resources."),

	// June 1 addition
	BldInfo(CardEnum::Windmill, "Windmill", WorldTile2(5, 5), ResourceEnum::Wheat, ResourceEnum::None, ResourceEnum::Flour, 10, 2, { 150,0,0 }, "Grind Wheat into Wheat Flour. +10% Productivity to surrounding Farms."),
	BldInfo(CardEnum::Bakery, "Bakery", WorldTile2(5, 5), ResourceEnum::Flour, ResourceEnum::Coal, ResourceEnum::Bread, 30, 2, { 70,30,0 }, "Bake Bread with Wheat Flour and heat."),
	BldInfo(CardEnum::GemstoneMine, "Gemstone Mine", WorldTile2(5, 5), ResourceEnum::None, ResourceEnum::None, ResourceEnum::Gemstone, 10, 3, { 70,30,0 }, "Mine Gemstone from Gemstone Deposits."),
	BldInfo(CardEnum::Jeweler, "Jeweler", WorldTile2(4, 7), ResourceEnum::Gemstone, ResourceEnum::GoldBar, ResourceEnum::Jewelry, 10, 3, { 150,0,50 }, "Craft Gemstone and Gold Bar into Jewelry."),

	// June 9 addition
	BldInfo(CardEnum::Beekeeper, "Beekeeper", WorldTile2(6, 9), ResourceEnum::None, ResourceEnum::None, ResourceEnum::Beeswax, 10, 2, { 50,30,0 }, "Produce Beeswax and Honey. Efficiency increases with more surrounding trees."),
	BldInfo(CardEnum::Brickworks, "Brickworks", WorldTile2(6, 5), ResourceEnum::Clay, ResourceEnum::Coal, ResourceEnum::Brick, 20, 3, { 20,100,0 }, "Produce Brick from Clay and Coal."),
	BldInfo(CardEnum::CandleMaker, "Candle Maker", WorldTile2(5, 6), ResourceEnum::Beeswax, ResourceEnum::Cotton, ResourceEnum::Candle, 20, 3, { 150,50,0 }, "Make Candles from Beeswax and Cotton wicks."),
	BldInfo(CardEnum::CottonMill, "Cotton Mill", WorldTile2(7, 6), ResourceEnum::Cotton, ResourceEnum::None, ResourceEnum::CottonFabric, 20, 5, { 0, 100, 100 }, "Mass-produce Cotton into Cotton Fabric."),
	BldInfo(CardEnum::PrintingPress, "Printing Press", WorldTile2(5, 6), ResourceEnum::Paper, ResourceEnum::Dye, ResourceEnum::Book, 20, 5, { 0, 150, 100 }, "Print Books."),

	// June 25 addition
	BldInfo(CardEnum::Warehouse, "Warehouse", WorldTile2(6, 4), ResourceEnum::None, ResourceEnum::None, ResourceEnum::None, 0, 0, { 70,70,0 }, "Advanced storage with 30 storage slots."),
	BldInfo(CardEnum::Fort, "Fort", WorldTile2(9, 9), ResourceEnum::None, ResourceEnum::None, ResourceEnum::None, 0, 0, { 0,0,0 }, "+100% province's defense."),
	BldInfo(CardEnum::Colony, "Colony", WorldTile2(10, 10), ResourceEnum::None, ResourceEnum::None, ResourceEnum::None, 0, 0, { 0,0,0 }, "Extract resource from province."),
	BldInfo(CardEnum::InventorsWorkshop, "Inventor's Workshop", WorldTile2(6, 6), ResourceEnum::Wood, ResourceEnum::None, ResourceEnum::None, 0, 2, { 50,50,0 }, "Generate science points. Use wood as input."),
	BldInfo(CardEnum::IntercityRoad, "Intercity Road", WorldTile2(1, 1), ResourceEnum::None, ResourceEnum::None, ResourceEnum::None, 0, 0, { 0,0,0 }, "Build Road to connect with other Cities, Fort, and Colonies."),

	BldInfo(CardEnum::FakeTownhall, "FakeTownhall", WorldTile2(12, 12), ResourceEnum::None, ResourceEnum::None, ResourceEnum::None, 0, 0, { 0,0,0 }, "..."),
	BldInfo(CardEnum::FakeTribalVillage, "FakeTribalVillage", WorldTile2(12, 12), ResourceEnum::None, ResourceEnum::None, ResourceEnum::None, 0, 0, { 0,0,0 }, "..."),
	BldInfo(CardEnum::ChichenItza, "ChichenItza", WorldTile2(16, 16), ResourceEnum::None, ResourceEnum::None, ResourceEnum::None, 0, 0, { 0,0,0 }, "..."),

	
	// Decorations
	BldInfo(CardEnum::FlowerBed, "Flower Bed", WorldTile2(1, 1), ResourceEnum::None, ResourceEnum::None, ResourceEnum::None, 0, 0, { 0,0,0 }, "Increase the surrounding appeal by 5 within 5 tiles radius."),
	BldInfo(CardEnum::GardenShrubbery1, "Shrubbery", WorldTile2(1, 1), ResourceEnum::None, ResourceEnum::None, ResourceEnum::None, 0, 0, { 0,0,0 }, "Increase the surrounding appeal by 5 within 5 tiles radius."),
	BldInfo(CardEnum::GardenCypress, "Garden Cypress", WorldTile2(1, 1), ResourceEnum::None, ResourceEnum::None, ResourceEnum::None, 0, 0, { 0,0,0 }, "Increase the surrounding appeal by 8 within 5 tiles radius."),

	
	
	// Rare cards
	//BldInfo("Sales Office",			WorldTile2(4, 5),		ResourceEnum::None, ResourceEnum::None, ResourceEnum::None,		0,	{20, 20, 0},	"Decrease the amount of trade fee by 50%."),
	BldInfo(CardEnum::ArchitectStudio,		"Architect's Studio",	WorldTile2(4, 5),		ResourceEnum::None, ResourceEnum::None, ResourceEnum::None,		0, 0,	{30, 30, 0},	"+5% housing appeal."),
	BldInfo(CardEnum::EngineeringOffice,		"Engineer's Office",	WorldTile2(4, 5),		ResourceEnum::None, ResourceEnum::None, ResourceEnum::None,		0, 0,	{30, 30, 0},	"+10% industrial production."),
	BldInfo(CardEnum::DepartmentOfAgriculture,"Ministry of Agriculture", WorldTile2(4, 5),		ResourceEnum::None, ResourceEnum::None, ResourceEnum::None,		0, 0,	{30, 30, 0},	"+5% Farm production when there are 8 or more Farms."),
	//BldInfo("Isolationist Publisher", WorldTile2(4, 5),		ResourceEnum::None, ResourceEnum::None, ResourceEnum::None,		0,	{20, 20, 0},	"Double trade fees. -10% food consumption."),

	BldInfo(CardEnum::StockMarket,			"Stock Market",			WorldTile2(4, 5),		ResourceEnum::None, ResourceEnum::None, ResourceEnum::None,		0, 0,	{30, 30, 0},	"Upkeep for Trading Companies reduced to 1<img id=\"Coin\"/>."),
	BldInfo(CardEnum::CensorshipInstitute,	"Censorship Institute",		WorldTile2(4, 5),		ResourceEnum::None, ResourceEnum::None, ResourceEnum::None,		0, 0,	{30, 30, 0},	"-1 card from each hand roll. +7% Farm production."),
	BldInfo(CardEnum::EnvironmentalistGuild,	"Environmentalist Guild", WorldTile2(4, 5),		ResourceEnum::None, ResourceEnum::None, ResourceEnum::None,		0, 0,	{30, 30, 0},	"+15% house appeal. -30% production for mines, smelters, and Charcoal Makers."),
	BldInfo(CardEnum::IndustrialistsGuild,	"Industrialist Guild",		WorldTile2(4, 5),		ResourceEnum::None, ResourceEnum::None, ResourceEnum::None,		0, 0,	{50, 50, 0},	"+20% production to surrounding industries within 10 tiles radius."),
	BldInfo(CardEnum::Oracle,				"Oracle",					WorldTile2(4, 5),		ResourceEnum::None, ResourceEnum::None, ResourceEnum::None,		0, 0,	{50, 50, 0},	"Gain one additional choice when selecting Rare Cards."),
	BldInfo(CardEnum::AdventurersGuild,		"Adventurer's Guild",			WorldTile2(4, 5),		ResourceEnum::None, ResourceEnum::None, ResourceEnum::None,		0, 0,	{80, 50, 0},	"At the end of summer choose a Rare Card."),
	
	BldInfo(CardEnum::ConsultingFirm,		"Consulting Firm",			WorldTile2(4, 5), ResourceEnum::None, ResourceEnum::None, ResourceEnum::None, 0, 0, { 30, 0, 0}, "+30% production to surrounding buildings. -30 <img id=\"Coin\"/> per round"),
	BldInfo(CardEnum::ImmigrationPropagandaOffice,"Immigration Propaganda Office", WorldTile2(4, 5), ResourceEnum::None, ResourceEnum::None, ResourceEnum::None, 0, 0, {30, 0, 0}, "+30% immigration"),
	BldInfo(CardEnum::MerchantGuild,			"Merchant Guild",			WorldTile2(4, 5), ResourceEnum::None, ResourceEnum::None, ResourceEnum::None, 0, 0, { 30, 0, 0}, "-5% trade fee."),
	BldInfo(CardEnum::OreSupplier,			"Ore Supplier Guild",		WorldTile2(4, 5), ResourceEnum::None, ResourceEnum::None, ResourceEnum::None, 0, 0, { 30, 0, 0}, "Buy selected ore each season with 0% trading fee."),
	BldInfo(CardEnum::BeerBreweryFamous,		"Famous Beer Brewery",		WorldTile2(5, 7),	ResourceEnum::Wheat, ResourceEnum::None, ResourceEnum::Beer,		 10, 8, {150,90,0}, "Large brewery with improved productivity. Require 4 Beer Breweries to unlock."),
	BldInfo(CardEnum::IronSmelterGiant, "Giant Iron Smelter", WorldTile2(7, 8), ResourceEnum::IronOre, ResourceEnum::Coal, ResourceEnum::Iron, 10, 8, { 150,90,0 }, "Large smelter."),

	
	BldInfo(CardEnum::Cattery,				"Cattery",					WorldTile2(4, 5), ResourceEnum::None, ResourceEnum::None, ResourceEnum::None, 0, 0, { 30, 30, 0 }, "+15% housing appeal to surrounding houses."),
	BldInfo(CardEnum::InvestmentBank,		"Investment Bank",			WorldTile2(4, 5), ResourceEnum::None, ResourceEnum::None, ResourceEnum::None, 0, 0, { 0, 70, 20 }, "Gain <img id=\"Coin\"/> equals to +10% of <img id=\"Coin\"/> every round."),

	// Unique Cards
	BldInfo(CardEnum::StatisticsBureau, "Statistics Bureau", WorldTile2(4, 5), ResourceEnum::None, ResourceEnum::None, ResourceEnum::None, 0, 0, { 30, 0, 0 }, "Show Town Statistics."),
	BldInfo(CardEnum::JobManagementBureau, "Employment Bureau", WorldTile2(4, 5), ResourceEnum::None, ResourceEnum::None, ResourceEnum::None, 0, 0, { 30, 0, 0 }, "Allow managing job priority (global)."),

	
	// Can no longer pickup cards
	//BldInfo("Necromancer tower",	WorldTile2(4, 5),		ResourceEnum::None, ResourceEnum::None, ResourceEnum::None,		0,	{30, 30, 0},	"All citizens become zombie minions. Happiness becomes irrelevant. Immigration ceased."),
	// Imperialist
};


static const BldInfo CardInfos[]
{
	BldInfo(CardEnum::Investment,		"Investment", 300, "+<img id=\"Coin\"/>20 income"),
	BldInfo(CardEnum::InstantBuild,		"Instant Build", 200, "Use on a construction site to pay x3 the resource cost and instantly build it."),
	BldInfo(CardEnum::ShrineWisdomPiece,	"OLD", 200, "OLD"),
	BldInfo(CardEnum::ShrineLovePiece,	"Shrine Piece: Love", 200, "Collect 3 shrine pieces to build a shrine."),
	BldInfo(CardEnum::ShrineGreedPiece,	"Shrine Piece: Greed", 200, "Collect 3 shrine pieces to build a shrine."),

	/*
	 * Passives
	 */
	BldInfo(CardEnum::BeerTax,			"Beer Tax", 200, "Houses with Beer get +5<img id=\"Coin\"/>"),
	BldInfo(CardEnum::HomeBrew,			"Home Brew", 200, "Houses with Pottery get +4<img id=\"Science\"/>"),

	BldInfo(CardEnum::MasterBrewer,			"Master Brewer", 200, "Beer Breweries gain +30% efficiency"),
	BldInfo(CardEnum::MasterPotter,			"Master Potter", 200, "Potters gain +20% efficiency"),

	BldInfo(CardEnum::CooperativeFishing,"Cooperative Fishing", 200, "+10% Fish production when there are 2 or more Fishing Lodges"),
	BldInfo(CardEnum::CompaniesAct,		"Companies Act", 200, "-10% trade fees for Trading Companies."),

	BldInfo(CardEnum::ProductivityBook,	"Productivity Book", 100, "+20% productivity"),
	BldInfo(CardEnum::SustainabilityBook,"Sustainability Book", 100, "Consume 40% less input"),
	BldInfo(CardEnum::FrugalityBook,		"Frugality Book", 100, "Decrease upkeep by 50%."),

	BldInfo(CardEnum::DesertPilgrim,		"Desert Pilgrim", 200, "Houses built on Desert get +5<img id=\"Coin\"/>."),

	BldInfo(CardEnum::WheatSeed,			"Wheat Seeds", 300, "Unlock Wheat farming. Wheat can be eaten or brewed into Beer."),
	BldInfo(CardEnum::CabbageSeed,			"Cabbage Seeds", 350, "Unlock Cabbage farming. Cabbage has high fertility sensitivity."),
	BldInfo(CardEnum::HerbSeed,			"Medicinal Herb Seeds", 500, "Unlock Medicinal Herb farming. Medicinal Herb can be used to heal sickness."),

	BldInfo(CardEnum::CannabisSeeds,			"Cannabis Seeds", 0, "Unlock Cannabis farming. Require region suitable for Cannabis."),
	BldInfo(CardEnum::GrapeSeeds,			"Grape Seeds", 0, "Unlock Grape farming. Requires region suitable for Grape."),
	BldInfo(CardEnum::CocoaSeeds,			"Cocoa Seeds", 0, "Unlock Cocoa farming. Requires region suitable for Cocoa."),
	BldInfo(CardEnum::CottonSeeds,			"Cotton Seeds", 0, "Unlock Cotton farming. Requires region suitable for Cotton."),
	BldInfo(CardEnum::DyeSeeds,				"Dye Seeds", 0, "Unlock Dye farming. Requires region suitable for Dye."),

	BldInfo(CardEnum::ChimneyRestrictor,	"Chimney Restrictor", 250, "Wood/Coal gives 15% more heat"),
	BldInfo(CardEnum::SellFood,			"Sell Food", 90, "Sell half of city's food for 3<img id=\"Coin\"/> each."),
	BldInfo(CardEnum::BuyWood,			"Buy Wood", 50, "Buy Wood with half of your treasury (5<img id=\"Coin\"/> each)."),
	BldInfo(CardEnum::ChildMarriage,			"Child Marriage", 200, "Decrease the minimum age for having children."),
	BldInfo(CardEnum::ProlongLife,			"Prolong Life", 200, "People live longer."),
	BldInfo(CardEnum::BirthControl,			"Birth Control", 200, "Decrease birth rate by 50%."),
	BldInfo(CardEnum::CoalTreatment,			"Coal Treatment", 250, "Coal gives 20% more heat"),


	BldInfo(CardEnum::CoalPipeline,			"Coal Pipeline", 150, "+30% productivity for smelters if the town has more than 1,000 Coal"),
	BldInfo(CardEnum::MiningEquipment,		"Mining Equipment", 150, "+30% productivity for mines if you have a Blacksmith"),
	BldInfo(CardEnum::Conglomerate,			"Conglomerate", 150, "+50<img id=\"Coin\"/> income if there are 2+ Trading Companies"),
	BldInfo(CardEnum::SmeltCombo,			"Iron Smelter Combo", 150, "+30% productivity to all Iron Smelter with adjacent Iron Smelter"),


	BldInfo(CardEnum::Immigration,			"Immigration Advertisement", 500, "5 immigrants join upon use."),
	BldInfo(CardEnum::DuplicateBuilding,			"Duplicate Building", 200, "Duplicate the chosen building into a card."),


	BldInfo(CardEnum::Pig,				"Pig", 100, "Spawn 3 Pigs on a Ranch."),
	BldInfo(CardEnum::Sheep,				"Sheep", 200, "Spawn 3 Sheep on a Ranch."),
	BldInfo(CardEnum::Cow,				"Cow", 300, "Spawn 3 Cows on a Ranch."),
	BldInfo(CardEnum::Panda,				"Panda", 1000, "Spawn 3 Pandas on a Ranch."),

	BldInfo(CardEnum::FireStarter,		"Fire Starter", 200, "Start a fire in an area (3 tiles radius)."),
	BldInfo(CardEnum::Steal,				"Steal", 200, "Steal 30% of target player's treasury<img id=\"Coin\"/>. Use on Townhall."),
	BldInfo(CardEnum::Snatch,			"Snatch", 50, "Steal <img id=\"Coin\"/> equal to target player's population. Use on Townhall."),
	BldInfo(CardEnum::SharingIsCaring,	"Sharing is Caring", 120, "Give 50 Wheat to the target player. Use on Townhall."),
	BldInfo(CardEnum::Kidnap,			"Kidnap", 350, "Steal up to 3 citizens from target player. Apply on Townhall."),
	BldInfo(CardEnum::KidnapGuard,		"Kidnap Guard", 30, "Guard your city against Kidnap for a year. Require <img id=\"Coin\"/>xPopulation to activate."),
	BldInfo(CardEnum::TreasuryGuard,		"Treasury Guard", 30, "Guard your city against Steal and Snatch for a year. Require <img id=\"Coin\"/>xPopulation to activate."),

	
	BldInfo(CardEnum::Cannibalism,		"Cannibalism", 0, "On death, people drop Meat. -10 <img id=\"Smile\"/> to all citizens."),

	BldInfo(CardEnum::WildCard,		"Wild Card", 15, "Build an unlocked building of your choice."),

	// June Additions
	BldInfo(CardEnum::WildCardFood,		"Agriculture Wild Card", 10, "Build an unlocked agriculture building of your choice."),
	BldInfo(CardEnum::WildCardIndustry,		"Industry Wild Card", 10, "Build an unlocked industry of your choice."),
	BldInfo(CardEnum::WildCardMine,		"Mine Wild Card", 10, "Build an unlocked mine of your choice."),
	BldInfo(CardEnum::WildCardService,		"Service Wild Card", 10, "Build an unlocked service building of your choice."),
	BldInfo(CardEnum::CardRemoval,			"Card Removal", 30, "Remove a card from the draw deck."),
	
	BldInfo(CardEnum::HappyBreadDay,		"Happy Bread Day", 150, "+5<img id=\"Smile\"/> to everyone if the town has more than 1,000 Bread"),
	BldInfo(CardEnum::BlingBling,		"Bling Bling", 150, "Houses with Jewelry get +7<img id=\"Smile\"/>"),
	BldInfo(CardEnum::GoldRush,		"Gold Rush", 150, "+30% productivity from Gold Mine."),

	// Rare cards
	BldInfo(CardEnum::MiddleClassTax,	"Middle Class Tax", 200, "+2 <img id=\"Coin\"/> income from each level 2+ house"),
	BldInfo(CardEnum::Treasure,			"Treasure", 100, "Instantly gain 500 <img id=\"Coin\"/>"),
	BldInfo(CardEnum::IndustrialRevolution,"Industrial Revolution", 200, "-50% cost of industrial building cards"),
	BldInfo(CardEnum::EmergencyRations,	"Emergency Rations", 250, "Instantly gain 50 Wheat."),

	BldInfo(CardEnum::CrateWood,			"Wood Crates", 100, "Instantly gain 100 Wood"),
	BldInfo(CardEnum::CrateCoal,			"Coal Crates", 100, "Instantly gain 150 Coal"),
	BldInfo(CardEnum::CrateIronOre,		"Iron Ore Crates", 100, "Instantly gain 200 Iron Ore"),
	BldInfo(CardEnum::CratePottery,		"Pottery Crates", 100, "Instantly gain 80 Pottery"),
	BldInfo(CardEnum::CrateJewelry,		"Jewelry Crates", 100, "Instantly gain 10 Jewelry"),

	BldInfo(CardEnum::SpeedBoost, "Speed Boost", 0, "Boost target building's work speed by +50% for 50s.")
};

static const int32 BuildingEnumCount = _countof(BuildingInfo);
static const int32 NonBuildingCardEnumCount = _countof(CardInfos);

static const std::vector<CardEnum> ActionCards
{
	CardEnum::Treasure,
	CardEnum::SellFood,
	CardEnum::BuyWood,
	CardEnum::Immigration,
	CardEnum::EmergencyRations,

	CardEnum::WheatSeed,
	CardEnum::CabbageSeed,
	CardEnum::HerbSeed,

	CardEnum::CannabisSeeds,
	CardEnum::GrapeSeeds,
	CardEnum::CocoaSeeds,
	CardEnum::CottonSeeds,
	CardEnum::DyeSeeds,

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
static bool IsActionCard(CardEnum cardEnumIn)
{
	for (CardEnum cardEnum : ActionCards) {
		if (cardEnumIn == cardEnum) return true;
	}
	return false;
}

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

	case CardEnum::MiddleClassTax:
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
	CardEnum::MiddleClassTax,
	
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

	CardEnum::CoalPipeline,
	CardEnum::MiningEquipment,
	CardEnum::Conglomerate,
	CardEnum::SmeltCombo,

	CardEnum::HomeBrew,
	CardEnum::BeerTax,

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

inline CardEnum FindCardEnumByName(std::string nameIn)
{
	ToLowerCase(nameIn);
	
	auto tryAddCard = [&](CardEnum cardEnum)
	{
		std::string name = GetBuildingInfo(cardEnum).name;
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
		
	case CardEnum::ImmigrationOffice:
		
	case CardEnum::TradingPort:
	case CardEnum::TradingPost:
	case CardEnum::TradingCompany:
		return true;
	default:
		return false;
	}
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

static bool IsRoad(CardEnum buildingEnum) {
	switch (buildingEnum) {
		case CardEnum::DirtRoad:
		case CardEnum::StoneRoad:
			return true;
		default: return false;
	}
}

static bool IsStorage(CardEnum buildingEnum) {
	switch (buildingEnum) {
	case CardEnum::StorageYard:
	case CardEnum::Warehouse:
		return true;
	default: return false;
	}
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


static std::pair<int32, int32> DockPlacementExtraInfo(CardEnum cardEnum)
{
	int32 indexLandEnd = 1;
	int32 minWaterCount = 5;

	if (cardEnum == CardEnum::TradingPort) {
		indexLandEnd = 3;
		minWaterCount = 25; // 8 * 5 = 40 full??
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
	std::string name;
	std::string description;
	ResourcePair resourceNeeded;
	int32 moneyNeeded;

	bool isUpgraded;

	BuildingUpgrade() : name(""), description(""), resourceNeeded(ResourcePair()), moneyNeeded(-1), isUpgraded(false) {}
	
	BuildingUpgrade(std::string name, std::string description, ResourcePair resourceNeeded, int32 moneyNeeded = 0)
		: name(name), description(description), resourceNeeded(resourceNeeded), moneyNeeded(moneyNeeded), isUpgraded(false) {}

	BuildingUpgrade(std::string name, std::string description, int32 moneyNeeded)
		: name(name), description(description), resourceNeeded(ResourcePair()), moneyNeeded(moneyNeeded), isUpgraded(false) {}

	BuildingUpgrade(std::string name, std::string description, ResourceEnum resourceEnum, int32 resourceCount)
		: name(name), description(description), resourceNeeded(ResourcePair(resourceEnum, resourceCount)), moneyNeeded(0), isUpgraded(false) {}

	void operator>>(FArchive& Ar) {
		Ar << isUpgraded;
		SerializeStr(Ar, name);
		SerializeStr(Ar, description);
		resourceNeeded >> Ar;
		Ar << moneyNeeded;
	}
};

struct BonusPair
{
	std::string name;
	int32 value;
};


/*
 * Military
 */
enum class ProvinceAttackEnum : uint8
{
	ConquerProvince,
	Vassalize,
	DeclareIndependence,

	VassalCompetition,
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
	
	Herb,
	//BaconBush,

	//! Beyond stone are ores
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

const static int32 TreeEnumSize = 14; // TileObjEnum up to last tree...
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
	std::string name;
	ResourceTileType type;

	int32 deathChancePerCycle; // (1- 1/2000)^(80 * 10 years) = 0.67
	int32 fruitChancePerCycle; // Still need fruit grown tick for trees without fruit. Since that determines seeding

	int32 fruitToEmitSeedChance = 2; // Only 1 / 3 fruits emit seed

	// After max growth, and tree will continue to grow (+10x maxGrowthTick = 0.4 more scale)
	// maxGrowthTick is also the point where tree starts bearing fruit
	int32 maxGrowthTick;

	ResourcePair fruitResourceBase100;
	ResourcePair cutDownResourceBase100;

	std::string description;

	int32 treeEnumInt() { return static_cast<int32>(treeEnum); }
	ResourceEnum harvestResourceEnum() { return cutDownResourceBase100.resourceEnum; }

	TileObjInfo(
		TileObjEnum treeEnumIn, std::string nameIn, ResourceTileType typeIn,
		ResourcePair fruitResourceBase100In, ResourcePair cutDownResourceBase100In, std::string descriptionIn)
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
			maxGrowthSeasons100 = 600; // 1.5 years to grow for trees
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

};

static const int32 GrassToBushValue = 3;

/*
 *
 * Note: Gather production tied into the HumanFoodCostPerYear through AssumedFoodProductionPerYear
 */
static const int32 GatherUnitsPerYear = 4; // 6;  // GatherBaseYield 4 gives 60 fruits per year... So 15 estimated gather per year...
static const int32 GatherBaseYield100 = AssumedFoodProduction100PerYear / GatherUnitsPerYear;
static const int32 JungleGatherBaseYield100 = GatherBaseYield100 * 2;

static const ResourcePair defaultWood100(ResourceEnum::Wood, WoodGatherYield_Base100);
static const ResourcePair defaultHay100(ResourceEnum::Hay, FarmBaseYield100 / 5); //5 is from tuning
static const ResourcePair defaultGrass100(ResourceEnum::Hay, FarmBaseYield100 / 5 / GrassToBushValue);


static const TileObjInfo TreeInfos[] = {
	//TileObjEnum treeEnumIn, std::string nameIn, ResourceTileType typeIn,
	//int32_t deathChancePerCycleIn, int32_t fruitChancePerCycleIn,  int32_t maxGrowthSeasons100,
	//ResourcePair fruitResourceBaseIn, ResourcePair cutDownResourceBaseIn, std::string descriptionIn)
	TileObjInfo(TileObjEnum::Birch,	"Birch",	ResourceTileType::Tree,	ResourcePair::Invalid(),									defaultWood100, "Fast-growing hardwood tree."),
	TileObjInfo(TileObjEnum::Orange,	"Orange",	ResourceTileType::Tree,	ResourcePair(ResourceEnum::Orange, GatherBaseYield100), defaultWood100, "Orange trees bear delicious fruits during non-winter seasons."),

	TileObjInfo(TileObjEnum::Apple,	"Apple",	ResourceTileType::Tree,	ResourcePair(ResourceEnum::Orange, GatherBaseYield100), defaultWood100, "Apple trees bear delicious fruits during non-winter seasons."),
	TileObjInfo(TileObjEnum::Papaya,	"Papaya",	ResourceTileType::Tree,	ResourcePair(ResourceEnum::Papaya, JungleGatherBaseYield100), defaultWood100, "Papaya trees bear delicious fruits during non-winter seasons."),
	TileObjInfo(TileObjEnum::Durian,	"Durian",	ResourceTileType::Tree,	ResourcePair(ResourceEnum::Orange, GatherBaseYield100), defaultWood100, "Durian trees bear delicious fruits during non-winter seasons."),
	TileObjInfo(TileObjEnum::Pine1,	"Pine",	ResourceTileType::Tree,	ResourcePair::Invalid(),									defaultWood100, "Fast-growing hardwood tree."),
	TileObjInfo(TileObjEnum::Pine2,	"Pine",	ResourceTileType::Tree,	ResourcePair::Invalid(),									defaultWood100, "Fast-growing hardwood tree."),
	TileObjInfo(TileObjEnum::GiantMushroom,	"Giant Mushroom",	ResourceTileType::Tree,	ResourcePair::Invalid(),					ResourcePair(ResourceEnum::Mushroom, WoodGatherYield_Base100 * 2), "A very large mushroom..."),

	TileObjInfo(TileObjEnum::Cherry,	"Cherry",	ResourceTileType::Tree,	ResourcePair(ResourceEnum::Orange, GatherBaseYield100), defaultWood100, "Cherry trees bear delicious fruit during non-winter seasons."),
	TileObjInfo(TileObjEnum::Coconut,	"Coconut",	ResourceTileType::Tree,	ResourcePair::Invalid(),								defaultWood100, "Coconut bears delicious fruit during non-winter seasons."),
	TileObjInfo(TileObjEnum::Cyathea,	"Cyathea",	ResourceTileType::Tree,	ResourcePair::Invalid(),								defaultWood100, "Fern tree."),
	TileObjInfo(TileObjEnum::ZamiaDrosi,	"Zamia Drosi",	ResourceTileType::Tree,	ResourcePair::Invalid(),						defaultWood100, "Giant leaf tropical tree."),

	TileObjInfo(TileObjEnum::Cactus1,	"Cactus",	ResourceTileType::Tree,	ResourcePair::Invalid(),						defaultWood100, "Desert plant with thick leafless stem covered in sharp spikes. Hurts to touch."),
	TileObjInfo(TileObjEnum::SavannaTree1,	"Savanna Acacia",	ResourceTileType::Tree,	ResourcePair::Invalid(),						defaultWood100, "Myths say acacia trees descended from an ancient tree of life."),
	
	TileObjInfo(TileObjEnum::GrassGreen, "Grass",	ResourceTileType::Bush,	ResourcePair::Invalid(),		defaultGrass100, "Common grass. A nice food-source for grazing animals."),

	TileObjInfo(TileObjEnum::OreganoBush, "Common Bush",	ResourceTileType::Bush,	ResourcePair::Invalid(), defaultHay100, "Fluffy little bush. A nice food-source for grazing animals."),
	TileObjInfo(TileObjEnum::CommonBush, "Common Bush", ResourceTileType::Bush,	ResourcePair::Invalid(), defaultHay100, "Fluffy little  bush. A nice food-source for grazing animals."),

	TileObjInfo(TileObjEnum::Fern, "Fern",					ResourceTileType::Bush,	ResourcePair::Invalid(), defaultHay100, "Common plant. A nice food-source for grazing animals."),
	TileObjInfo(TileObjEnum::SavannaGrass, "Savanna Grass",	ResourceTileType::Bush,	ResourcePair::Invalid(), defaultGrass100, "Common plant. A nice food-source for grazing animals."),
	TileObjInfo(TileObjEnum::JungleThickLeaf, "Jungle plant",	ResourceTileType::Bush,	ResourcePair::Invalid(), defaultHay100, "Common plant. A nice food-source for grazing animals."),

	
	TileObjInfo(TileObjEnum::BlueFlowerBush, "Primula Flower",	ResourceTileType::Bush,	ResourcePair::Invalid(), defaultHay100, "Common flower. A nice food-source for grazing animals."),
	TileObjInfo(TileObjEnum::WhiteFlowerBush, "Daisy Flower",	ResourceTileType::Bush,	ResourcePair::Invalid(), defaultHay100, "Common flower. A nice food-source for grazing animals."),

	TileObjInfo(TileObjEnum::RedPinkFlowerBush, "Porin Flower",	ResourceTileType::Bush,	ResourcePair::Invalid(), defaultHay100, "Common flower. A nice food-source for grazing animals."),

	TileObjInfo(TileObjEnum::CommonBush2, "Common Bush", ResourceTileType::Bush,	ResourcePair::Invalid(), defaultHay100, "Fluffy little  bush. A nice food-source for grazing animals."),
	//TileObjInfo(TileObjEnum::FieldFlowerPurple, "FieldFlowerPurple",	ResourceTileType::Bush,	0,	0,	ResourcePair::Invalid(), defaultHay100, "Common flower. A nice food-source for grazing animals."),
	TileObjInfo(TileObjEnum::FieldFlowerYellow, "FieldFlowerYellow",	ResourceTileType::Bush,	ResourcePair::Invalid(), defaultHay100, "Common flower. A nice food-source for grazing animals."),

	TileObjInfo(TileObjEnum::FieldFlowerHeart, "FieldFlowerHeart",	ResourceTileType::Bush,	ResourcePair::Invalid(), defaultHay100, "Common flower. A nice food-source for grazing animals."),
	TileObjInfo(TileObjEnum::FieldFlowerPic, "FieldFlowerPic",	ResourceTileType::Bush,	ResourcePair::Invalid(), defaultHay100, "Common flower. A nice food-source for grazing animals."),

	// Wheat to Stone are farm crops
	TileObjInfo(TileObjEnum::WheatBush, "Wheat",	ResourceTileType::Bush,	ResourcePair::Invalid(), ResourcePair(ResourceEnum::Wheat, FarmBaseYield100), "Grass that can be cultivated for its seed."),
	TileObjInfo(TileObjEnum::BarleyBush, "Barley",	ResourceTileType::Bush,	ResourcePair::Invalid(), ResourcePair(ResourceEnum::Wheat, FarmBaseYield100), "Grass that can be cultivated for its seed."),
	TileObjInfo(TileObjEnum::Grapevines, "Grapevines",	ResourceTileType::Bush,	ResourcePair::Invalid(), ResourcePair(ResourceEnum::Grape, FarmBaseYield100), "Produces delicious Grape that can be eaten fresh or make expensive wine."),
	TileObjInfo(TileObjEnum::Cannabis, "Cannabis",	ResourceTileType::Bush,	ResourcePair::Invalid(), ResourcePair(ResourceEnum::Cannabis, FarmBaseYield100 / 2), "Plant whose parts can be smoked or added to food for recreational purposes."),

	//TileObjInfo(TileObjEnum::PlumpCob, "Plump cob",	ResourceTileType::Bush,				1,	0,	170,	ResourcePair::Invalid(), ResourcePair(ResourceEnum::Wheat, FarmBaseYield100), "Produces large soft yellow tasty cob. Need 2 years before it is ready for harvest, but has 3x yield."),
	//TileObjInfo(TileObjEnum::CreamPod, "Cream pod",	ResourceTileType::Bush,				1,	0,	170,	ResourcePair::Invalid(), ResourcePair(ResourceEnum::Wheat, FarmBaseYield100), "Produces round pods, which, when cut open, reveals thick sweet cream substance."),
	TileObjInfo(TileObjEnum::Cabbage, "Cabbage",	ResourceTileType::Bush,	ResourcePair::Invalid(), ResourcePair(ResourceEnum::Cabbage, FarmBaseYield100 * 120 / 100), "Healthy vegetable great for making salad."),

	TileObjInfo(TileObjEnum::Cocoa,	"Cocoa",	ResourceTileType::Bush,	ResourcePair::Invalid(), ResourcePair(ResourceEnum::Cocoa, FarmBaseYield100), "Cocoa used to make delicious chocolate."),
	TileObjInfo(TileObjEnum::Cotton,	"Cotton",	ResourceTileType::Bush,	ResourcePair::Invalid(), ResourcePair(ResourceEnum::Cotton, FarmBaseYield100), "Cotton used to make Cotton Fabric."),
	TileObjInfo(TileObjEnum::Dye,		"Dye",	ResourceTileType::Bush,	ResourcePair::Invalid(), ResourcePair(ResourceEnum::Dye, FarmBaseYield100), "Dye used to dye Cotton Fabric or print Book."),

	
	TileObjInfo(TileObjEnum::Herb,		"Medicinal Herb",		ResourceTileType::Bush,	ResourcePair::Invalid(), ResourcePair(ResourceEnum::Herb, FarmBaseYield100 / 2), "Herb used to heal sickness or make medicine."),
	//TileObjInfo(TileObjEnum::BaconBush, "Bacon bush",	ResourceTileType::Bush,				1,	0,	170,	ResourcePair::Invalid(), ResourcePair(ResourceEnum::Wheat, FarmBaseYield100), "Plant with delicious leaves that tastes like bacon when grilled. Legend says, this plant was created by an ancient advanced civilization of giants."),

	TileObjInfo(TileObjEnum::Stone, "Stone",	ResourceTileType::Deposit,	ResourcePair::Invalid(),								ResourcePair(ResourceEnum::Stone, 2) /*this is not used?*/, "Easily-accessible stone deposits."),

	TileObjInfo(TileObjEnum::CoalMountainOre, "Coal Ore",	ResourceTileType::Deposit,	ResourcePair::Invalid(), ResourcePair(ResourceEnum::Stone, 3), "This region contains Coal Ore that can be mined from mountain."),
	TileObjInfo(TileObjEnum::IronMountainOre, "Iron Ore",	ResourceTileType::Deposit,	ResourcePair::Invalid(), ResourcePair(ResourceEnum::Stone, 3), "This region contains Iron Ore that can be mined from mountain."),
	TileObjInfo(TileObjEnum::GoldMountainOre, "Gold Ore",	ResourceTileType::Deposit,	ResourcePair::Invalid(), ResourcePair(ResourceEnum::Stone, 3), "This region contains Gold Ore that can be mined from mountain."),
	TileObjInfo(TileObjEnum::GemstoneOre, "Gemstone",	ResourceTileType::Deposit,	ResourcePair::Invalid(), ResourcePair(ResourceEnum::Stone, 3), "This region contains Gemstone that can be mined from mountain."),


	// ----------------------------------------------------------------------------------------------------------------------------------------------------------

	//TileObjInfo(TileObjEnum::PricklyPearCactus, "Prickly Pear Cactus",	ResourceTileType::Bush,ResourceEnum::Orange,			1,	10,	2,	0, "This cactus produces sweet red fruits."),

	//TileObjInfo(TileObjEnum::WheatBush, "Saguaro Cactus",	ResourceTileType::Bush,ResourceEnum::None,			1,	10,	2,	0, "Hardy cactus can be harvest wood."),


	TileObjInfo(TileObjEnum::Count, "-----------",	ResourceTileType::Bush,	ResourcePair::Invalid(), ResourcePair(ResourceEnum::None, 5), "-----------"),

	TileObjInfo(TileObjEnum::Fish, "Fish",	ResourceTileType::Fish,	ResourcePair::Invalid(), ResourcePair(ResourceEnum::Fish, 5), "Fish..."),

	//TileObjInfo(TileObjEnum::Avocado, "Avocado",	ResourceTileType::Tree,	10000,	80 * 3,	50,	ResourcePair::Invalid(), ResourcePair(ResourceEnum::None, 5), "A tree that bears delicious and healthy fruits"),
};
static const int32_t TileObjEnumCount = _countof(TreeInfos);


static void TileObjInfosIntegrityCheck()
{
	for (int i = 0; i < TileObjEnumSize; i++) {
		PUN_CHECK(TreeInfos[i].treeEnum == static_cast<TileObjEnum>(i));
	}
}

static TileObjInfo GetTileObjInfoInt(int32 tileObjEnumInt) {
	if (tileObjEnumInt == static_cast<int32>(TileObjEnum::None)) {
		static TileObjInfo noneTileInfo(TileObjEnum::None, "None", ResourceTileType::None, ResourcePair::Invalid(), ResourcePair::Invalid(), "None");
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
		if (name.Equals(ToFString(TreeInfos[i].name))) {
			return i;
		}
	}
	PUN_NOENTRY();
	return -1;
}

/**
 * Map
 */

enum class TerrainTileType : uint8
{
	None = 0,
	River,
	Ocean,
	Mountain,
	ImpassableFlat,
};
static const std::vector<std::string> TerrainTileTypeName
{
	"None",
	"River",
	"Ocean",
	"Mountain",
	"Impassable Terrain",
};
static std::string GetTerrainTileTypeName(TerrainTileType type) {
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

	/** 
	 * Building Map
	 *
	 * >= 0 means BuildingTileType
	 * -1,-2,-3,-4 means that it is a reserved front tile
	 * Other building can't build on reserved tile, but reserved tile can overlap
	 *
	 */

	//static std::vector<TerrainTileType> kTerrainMap;
	//static std::vector<int32> kBuildingIdMap;
	//static std::vector<int32> kBuildingFrontIdMap; // x4 size of total tiles... 4 represents each facing directions of buildings using this tile..
	//static std::vector<CardEnum> kBuildingEnumMap;
	
	//static int32 buildingId(int32_t x, int32_t y) { return kBuildingIdMap[WorldTile2(x, y).tileId()];  }
	//static int32 buildingId(WorldTile2 tile) { return kBuildingIdMap[tile.tileId()]; }

	//static bool hasBuilding(WorldTile2 tile) { return kBuildingIdMap[tile.tileId()] >= 0; }

	//static TerrainTileType terrainTileType(int32 x, int32 y) { return kTerrainMap[WorldTile2(x, y).tileId()]; }
	//static CardEnum buildingEnum(WorldTile2 tile) { return kBuildingEnumMap[tile.tileId()]; }


	//static std::vector<int32> frontBuildingIds(WorldTile2 tile) {
	//	int32 tileId = tile.tileId();
	//	std::vector<int32> buildingIds;
	//	if (kBuildingFrontIdMap[tileId * 4] != -1) buildingIds.push_back(kBuildingFrontIdMap[tileId * 4]);
	//	if (kBuildingFrontIdMap[tileId * 4 + 1] != -1) buildingIds.push_back(kBuildingFrontIdMap[tileId * 4 + 1]);
	//	if (kBuildingFrontIdMap[tileId * 4 + 2] != -1) buildingIds.push_back(kBuildingFrontIdMap[tileId * 4 + 2]);
	//	if (kBuildingFrontIdMap[tileId * 4 + 3] != -1) buildingIds.push_back(kBuildingFrontIdMap[tileId * 4 + 3]);
	//	return buildingIds;
	//}

	// This function doens't take road into account
	//static bool IsFrontBuildable(WorldTile2 tile)
	//{
	//	int32 tileId = tile.tileId();
	//	if (kTerrainMap[tileId] != TerrainTileType::None) {
	//		return false;
	//	}

	//	CardEnum buildingEnum = kBuildingEnumMap[tileId];
	//	return buildingEnum == CardEnum::None ||
	//			IsRoad(buildingEnum) ||
	//			IsRoadOverlapBuilding(buildingEnum);
	//}
	//static bool IsRoadOverlapBuildable(WorldTile2 tile) {
	//	return IsFrontBuildable(tile) && !IsRoadOverlapBuilding(kBuildingEnumMap[tile.tileId()]);
	//}
	
	//static void SetBuildingTile(WorldTile2 tile, int32 buildingId, CardEnum buildingEnum)
	//{
	//	//if (Time::Ticks() > 0) UE_LOG(LogTemp, Error, TEXT("SetBuildingTile %d %d"), x, y);

	//	int32 tileId = tile.tileId();
	//	//PUN_CHECK(kTerrainMap[tileId] >= 0) // Building on front tile is not usual

	//	PUN_CHECK(static_cast<int>(buildingEnum) < 1000);

	//	kBuildingIdMap[tileId] = buildingId;
	//	kBuildingEnumMap[tileId] = buildingEnum;
	//}

	//static void AddFrontTile(WorldTile2 tile, Direction faceDirection, int32_t buildingId) { 
	//	//PUN_CHECK(IsFrontBuildable(tile)); TODO: bring back
	//	//if (Time::Ticks() > 0) UE_LOG(LogTemp, Error, TEXT("AddFrontTile %s"), *tile.To_FString());

	//	int32 tileId = tile.tileId();
	//	kBuildingFrontIdMap[tileId * 4 + static_cast<int32_t>(faceDirection)] = buildingId;
	//}

	//static void RemoveFrontTile(WorldTile2 tile, Direction faceDirection) { 
	//	//PUN_CHECK(IsFrontBuildable(tile)); TODO: bring back
	//	//if (Time::Ticks() > 0) UE_LOG(LogTemp, Error, TEXT("RemoveFrontTile %s"), *tile.To_FString());

	//	int32 tileId = tile.tileId();
	//	kBuildingFrontIdMap[tileId * 4 + static_cast<int32_t>(faceDirection)] = -1;
	//}

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
	static const int64_t MoveAtomsPerTick = CoordinateConstants::AtomsPerTile / Time::TicksPerSecond * 3;
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

static const int32 IntercityRoadTileCost = 20;

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
	

	MountainMine,
	Dock,
	ClayPit,

	Fort,
	Colony,
	ColonyNoGeoresource,
	
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
	std::string instruction;
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
	Beekeeper,
	
	//ConstructionOffice,
	Industrialist,
	Consulting,
	Human,

	Library,
	School,
	Bank,
	
	Theatre,
	Tavern,

	BadAppeal,
};

static const std::string OverlayTypeName[] = {
	"None",
	"Appeal",
	"Fish",
	"Farm",
	"Gatherer",
	"Hunter",
	"Forester",
	//"ConstructionOffice",
	"Industrialist",
	"Consulting",
	"Human",

	"Library",
	"School",
	"Bank",
	
	"Theatre",
	"Tavern",

	"BadAppeal",
};

static bool IsGridOverlay(OverlayType overlayEnum)
{
	switch(overlayEnum)
	{
	case OverlayType::Appeal:
	case OverlayType::Fish:
	case OverlayType::Farm:
		return true;
	default: return false;
	}
}

static const int32 BattleInfluencePrice = 200;
static const int32 BattleClaimTicks = Time::TicksPerSeason;

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
static std::vector<std::string> InfluenceIncomeEnumName
{
	"Townhall",
	"Population",
	"Luxury Consumption",
	"Territory Upkeep",
	"Flat-Land Border Province Upkeep",
	"Too Much Stored Influence Points",

	"Fort",
	"Colony",
};
static int32 InfluenceIncomeEnumCount = static_cast<int32>(InfluenceIncomeEnum::Count);

enum class IncomeEnum : uint8
{
	// House income
	Base,
	Tech_MoreGoldPerHouse,
	Tech_HouseLvl2Income,
	Appeal,
	Luxury,

	Adjacency,
	Card_MiddleClass,
	Card_BeerTax,
	Card_DesertPilgrim,

	// HouseIncomeEnumCount!!!

	// Other income

	TownhallIncome,
	BankProfit,
	InvestmentProfit,
	ConglomerateIncome,

	FromVassalTax,
	ToLordTax,
	Others,
	BuildingUpkeep,
	ArmyUpkeep,
	
	TerritoryRevenue,
	TerritoryUpkeep,
	TradeRoute,
	
	MoneyLastEra,

	Count,
};

static const int32 HouseIncomeEnumCount = 9;

static std::vector<std::string> IncomeEnumName
{
	"Base",
	"Tech More Gold per House",
	"Tech House Lvl 2 Income",
	"Appeal",
	"Luxury",

	"Adjacency",
	"Card Middle Class Income",
	"Card Beer Tax",
	"Card Desert Pilgrim",

	"Townhall Income",
	"Bank Profit",
	"Investment Profit",
	"Conglomerate",
	
	"Tax from Vassal",
	"Tax for Lord",
	"Others",
	"Building Upkeep",
	"Military Upkeep",
	
	"Territory Income",
	"Territory Upkeep",
	"Trade Route",
	
	"Last Era Tech",

	"Count",
};

static int32 IncomeEnumCount = static_cast<int32>(IncomeEnum::Count);


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
	Library,
	School,
	HomeBrew,

	KnowledgeTransfer,
	ScienceLastEra,

	Rationalism,

	Count,
};
static int32 ScienceEnumCount = static_cast<int32>(ScienceEnum::Count);

static std::vector<std::string> ScienceEnumName
{
	"House Base",
	"House Luxury",
	"Library",
	"School",
	"Home Brew",

	"Knowledge transfer",
	"Last Era Technology",

	"Rationalism",
};

static std::vector<ScienceEnum> HouseScienceEnums
{
	ScienceEnum::Base,
	ScienceEnum::Luxury,
	ScienceEnum::Library,
	ScienceEnum::School,
	ScienceEnum::HomeBrew,
};


/**
 * Research
 */

enum class TechEnum : uint8
{
	None,

	DeepMining,
	IronRefining,
	GoldRefining,

	RerollCardsPlus1,

	Fence,
	HouseLvl2Income,
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
	Mint,
	//TermiteFarm,

	Blacksmith,

	QuarryImprovement,
	StoneRoad,
	Garden,
	BlossomShrine,

	Chocolatier,

	HerbFarming,
	Plantation,
	
	StoneTools,
	BlackSmith,
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

	//Bridge,
	HumanitarianAid,

	TradingCompany,

	BarrackArcher,
	BarrackKnight,
	


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
	FarmAdjacency,
	IndustrialAdjacency,

	ScienceLastEra,
	MoneyLastEra,
	FarmLastEra,
	IndustryLastEra,
	MilitaryLastEra,

	/*
	 * Prosperity
	 */
	FlowerBed,
	GardenShrubbery1,
	GardenCypress,

	Rationalism,

	Fort,
	Colony,
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

static void AppendClaimConnectionString(std::stringstream& ss, ClaimConnectionEnum claimConnectionEnum)
{
	if (claimConnectionEnum == ClaimConnectionEnum::ShallowWater) {
		ss << " (shallow water)";
	}
	else if (claimConnectionEnum == ClaimConnectionEnum::Deepwater) {
		ss << " (oversea)";
	}
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

	SmallShip,
};

struct BiomeInfo
{
	std::string name;
	std::string description;
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

	std::string GetNameWithoutSpace() const {
		std::string result = name;
		result.erase(std::remove(result.begin(), result.end(), ' '), result.end());
		return result;
	}
};

static const BiomeInfo BiomeInfos[]
{
	{ "Forest",
		"Serene broadleaf forest with moderate temperature. "
					"Its friendly conditions make it an ideal starting area for new players.",
		{ TileObjEnum::Orange, TileObjEnum::Birch },
		{TileObjEnum::OreganoBush, TileObjEnum::CommonBush, TileObjEnum::CommonBush2},
		{TileObjEnum::WhiteFlowerBush},
		{ UnitEnum::RedDeer, UnitEnum::Boar },
		{ UnitEnum::BlackBear, UnitEnum::BrownBear }
	},
	{"Grassland",
		"Biome filled with grasses, flowers and herbs. "
					"Although erratic precipitation makes the land unsuitable for farming, the dense grass makes this biome ideal for ranching.",
		{ TileObjEnum::Orange, TileObjEnum::Birch },
		{TileObjEnum::GrassGreen},
		{TileObjEnum::WhiteFlowerBush},
		{ UnitEnum::RedDeer, UnitEnum::Boar },
		{},
		5
	},
	{ "Desert",
		"Dry barren land with just sand and stone, but rich with mineral deposits",
		{ TileObjEnum::Cactus1  },
		{ TileObjEnum::GrassGreen },
		{TileObjEnum::WhiteFlowerBush},
		{},
		{}
	},
	
	{"Jungle",
		"Wet tropical jungle thick with trees and dense underbrush, teeming with life, and infested with disease. ",
		{ TileObjEnum::Papaya, TileObjEnum::Cyathea, TileObjEnum::ZamiaDrosi },
		{ TileObjEnum::Fern, TileObjEnum::JungleThickLeaf },
		{TileObjEnum::WhiteFlowerBush},
		{UnitEnum::DarkDeer, UnitEnum::Boar},
		{ UnitEnum::BlackBear},
		5
	},
	{ "Savanna",
		"Tropical grassland scattered with shrubs and isolated trees. "
					"The rich ecosystem supports a wide variety of wildlife.",
		{ TileObjEnum::SavannaTree1  },
		{ TileObjEnum::GrassGreen },
		{},
		{UnitEnum::YellowDeer, UnitEnum::Boar },
		{},
		5
	},
	
	{"Boreal Forest",
		"Coniferous forest with long and punishing winters.",
		{ TileObjEnum::Pine1, TileObjEnum::Pine2 },
		{ TileObjEnum::OreganoBush },
		{ TileObjEnum::WhiteFlowerBush},
		{ UnitEnum::RedDeer },
		{ UnitEnum::BrownBear},
	},
	{ "Tundra",
		"Extremely cold, frozen plain where almost nothing grows.",
		{ TileObjEnum::Pine1, TileObjEnum::Pine2 },
		{ }, // TileObjEnum::OreganoBush
		{TileObjEnum::WhiteFlowerBush},
		{},
		{}
	},
};

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


const int32 maxCelsiusDivider = 3;

/*
 * Units
 */

 // Unit's constants
static const int32_t UnitFoodFetchPerYear = 5;

struct UnitInfo
{
	UnitEnum unitEnum;
	// Need std::string since we need to copy when passing into FName (allows using name.c_str() instead of FName in some cases)
	std::string name;
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

	// 1 season = 3 minutes = 180 secs = 180 turns ... eating could take ~20% of the time ... 20 tiles to get to food = ~45 sec for food...

	// year100 is year * 100
	UnitInfo(
		UnitEnum unitEnum,
		std::string name,
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
		foodPerFetch = foodResourcePerYear / UnitFoodFetchPerYear;

		maxFoodTicks = foodTicksPerFetch * (name == "Human" ? 1 : 2); // Fragile humans
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
 * Note: Hunting production tied into the HumanFoodCostPerYear through AssumedFoodProductionPerYear
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
	UnitInfo(UnitEnum::Alpaca, "Feral Alpaca",	500,	100,		AnimalGestation,	100,	HumanFoodPerYear, {{ResourceEnum::Pork, 2 * BaseUnitDrop100}}),
	UnitInfo(UnitEnum::Human, "Human",	1000,	100,		025,	020,	HumanFoodPerYear, {{ResourceEnum::GameMeat, 2 * BaseUnitDrop100}}),
	UnitInfo(UnitEnum::Boar,"Boar",	UsualAnimalAge,	AnimalMinBreedingAge,		AnimalGestation,	100,	AnimalFoodPerYear, {{ResourceEnum::GameMeat, 3 * BaseUnitDrop100}}),

	UnitInfo(UnitEnum::RedDeer,"Red Deer",	UsualAnimalAge,	AnimalMinBreedingAge,		AnimalGestation,	100,	AnimalFoodPerYear, {{ResourceEnum::GameMeat, 2 * BaseUnitDrop100}, {ResourceEnum::Leather, BaseUnitDrop100}}),
	UnitInfo(UnitEnum::YellowDeer,"Mule Deer",	UsualAnimalAge,	AnimalMinBreedingAge,		AnimalGestation,	100,	AnimalFoodPerYear, {{ResourceEnum::GameMeat, 2 * BaseUnitDrop100}, {ResourceEnum::Leather, BaseUnitDrop100}}),
	UnitInfo(UnitEnum::DarkDeer,"Sambar Deer",	UsualAnimalAge,	AnimalMinBreedingAge,		AnimalGestation,	100,	AnimalFoodPerYear, {{ResourceEnum::GameMeat, 2 * BaseUnitDrop100}, {ResourceEnum::Leather, BaseUnitDrop100}}),

	UnitInfo(UnitEnum::BrownBear,"Brown Bear",	UsualAnimalAge,	AnimalMinBreedingAge,		AnimalGestation,	100,	AnimalFoodPerYear, {{ResourceEnum::GameMeat, 2 * BaseUnitDrop100}, {ResourceEnum::Leather, BaseUnitDrop100}}),
	UnitInfo(UnitEnum::BlackBear,"Black Bear",	UsualAnimalAge,	AnimalMinBreedingAge,		AnimalGestation,	100,	AnimalFoodPerYear, {{ResourceEnum::GameMeat, 2 * BaseUnitDrop100}, {ResourceEnum::Leather, BaseUnitDrop100}}),
	UnitInfo(UnitEnum::Panda,"Panda",	UsualAnimalAge,	AnimalMinBreedingAge,		AnimalGestation,	100,	AnimalFoodPerYear, {{ResourceEnum::GameMeat, 2 * BaseUnitDrop100}, {ResourceEnum::Leather, BaseUnitDrop100}}),

	UnitInfo(UnitEnum::WildMan, "WildMan",	UsualAnimalAge,	AnimalMinBreedingAge,		AnimalGestation,	100,	AnimalFoodPerYear, {{ResourceEnum::GameMeat, 2 * BaseUnitDrop100}, {ResourceEnum::Leather, BaseUnitDrop100}}),
	UnitInfo(UnitEnum::Hippo, "Hippo",	UsualAnimalAge,	AnimalMinBreedingAge,		AnimalGestation,	100,	AnimalFoodPerYear, {{ResourceEnum::GameMeat, 2 * BaseUnitDrop100}, {ResourceEnum::Leather, BaseUnitDrop100}}),
	UnitInfo(UnitEnum::Penguin, "Penguin",	UsualAnimalAge,	AnimalMinBreedingAge,		AnimalGestation,	100,	AnimalFoodPerYear, {{ResourceEnum::GameMeat, 2 * BaseUnitDrop100}, {ResourceEnum::Leather, BaseUnitDrop100}}),
	
	
	UnitInfo(UnitEnum::Pig,"Pig",	UsualAnimalAge,	AnimalMinBreedingAge,		AnimalGestation,	100,	HumanFoodPerYear, {{ResourceEnum::Pork, BaseUnitDrop100 * 240 / 100}}),
	UnitInfo(UnitEnum::Sheep,"Sheep",	UsualAnimalAge,	AnimalMinBreedingAge,		AnimalGestation,	100,	HumanFoodPerYear, {{ResourceEnum::Lamb, BaseUnitDrop100 * 120 / 100}, {ResourceEnum::Wool, BaseUnitDrop100 * 120 / 100}}),
	UnitInfo(UnitEnum::Cow,"Cow",	UsualAnimalAge,	AnimalMinBreedingAge,		AnimalGestation,	100,	HumanFoodPerYear, {{ResourceEnum::Milk, BaseUnitDrop100 * 240 / 100}, {ResourceEnum::Leather,  BaseUnitDrop100 * 120 / 100}}),

	UnitInfo(UnitEnum::Infantry,"Infantry",	0,	1,		1,	1,	1, {{ResourceEnum::Pork, 15}}),
	UnitInfo(UnitEnum::ProjectileArrow,"ProjectileArrow",	0,	1,		1,	1,	1, {{ResourceEnum::Pork, 15}}),

	UnitInfo(UnitEnum::SmallShip, "SmallShip",	0,	1,		1,	1,	1, {{ResourceEnum::Pork, 15}}),
	//UnitInfo("Bear",	050,	500,	200,		010,	050,	5),
};

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
		if (name.Equals(ToFString(UnitInfos[i].name))) {
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

static const UnitEnum SkelMeshUnits[] = {
	UnitEnum::Human,
	UnitEnum::WildMan,
	UnitEnum::Hippo,
	UnitEnum::Penguin,
};
static bool IsUsingSkeletalMesh(UnitEnum unitEnumIn) {
	for (UnitEnum unitEnum : SkelMeshUnits) {
		if (unitEnum == unitEnumIn) {
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

	Rest,
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

	"Rest",
};
static const std::string& GetUnitAnimationName(UnitAnimationEnum animationEnum) {
	return UnitAnimationNames[static_cast<int>(animationEnum)];
}

enum class HumanVariationEnum : uint8
{
	AdultMale,
	AdultFemale,
	ChildMale,
	ChildFemale,
};

static HumanVariationEnum GetHumanVariationEnum(bool isChild, bool isMale)
{
	if (isChild) {
		return isMale ? HumanVariationEnum::ChildMale : HumanVariationEnum::ChildFemale;
	}
	return isMale ? HumanVariationEnum::AdultMale : HumanVariationEnum::AdultFemale;
}

static const std::vector<float> UnitAnimationPlayRate =
{
	1.0f,
	1.0f,
	2.5f, // Walk

#if TRAILER_MODE
	1.479f, // Build 1.183 for 0.5s
	2.0833, // ChopWood 1.6666
#else
	1.0f, // Build
	2.0f, // ChopWood
#endif
	
	3.0f, // StoneMining
	1.0f,
};
static float GetUnitAnimationPlayRate(UnitAnimationEnum animationEnum) {
	return UnitAnimationPlayRate[static_cast<int>(animationEnum)];
}

//! Colors

static FLinearColor PlayerColor(int32_t playerId)
{
	switch (playerId)
	{
	case -1: return FLinearColor::Yellow;
	case 0: return FLinearColor::Red;
	case 1: return FLinearColor::Blue;
	case 2: return FLinearColor::Green;
	default: return FLinearColor::Black;
	}
}

static FLinearColor PlayerColor1(int32_t playerId)
{
	static const FLinearColor arr[] = {
		FLinearColor(0.225, 0.00769, 0.972, 1),
		FLinearColor(0, 0.00769, 0.972, 1),
		FLinearColor(0, 0.00769, 0, 1),
		FLinearColor(1, 0, 0, 1),
		FLinearColor(.02, .09, 1, 1),
		FLinearColor(1, .943, 0, 1),
	};
	if (playerId >= _countof(arr)) {
		return FLinearColor(0.1, 0.1, 0.1, 1);
	}
	return arr[playerId];
}

static FLinearColor PlayerColor2(int32_t playerId)
{
	static const FLinearColor arr[] = {
		FLinearColor(0.972, 0.875, 0, 1),
		FLinearColor(0.225, 0.00769, 0.972, 1),
		FLinearColor(0.225, 0.00769, 0, 1),
		FLinearColor(1, .9, 0, 1),
		FLinearColor(.954, 1, 1, 1),
		FLinearColor(0, 0.105, 1, 1),
	};
	if (playerId >= _countof(arr)) {
		return FLinearColor(1, 1, 1, 1);
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
	Kill,
	ClearLand,
	Unhappy,

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
	"Kill",
	"ClearLand",
	"Unhappy",

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
	
	TechUI,
	QuestUI,
	//ObjectDescription, // shouldn't be one, to easy to misclick on this...

	StatisticsUI,
	PlayerOverviewUI,
	ArmyMoveUI,

	InitialResourceUI,
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
};

struct PopupInfo
{
	int32 playerId;
	std::string body;
	
	std::vector<std::string> choices;
	
	PopupReceiverEnum replyReceiver = PopupReceiverEnum::None;
	std::string popupSound;
	int32 replyVar1;

	bool forcedNetworking = false;
	bool forcedSkipNetworking = false;
	
	ExclusiveUIEnum warningForExclusiveUI = ExclusiveUIEnum::None;
	int32 startTick = -1;
	int32 startDisplayTick = -1;

	PopupInfo() : playerId(-1) {}
	PopupInfo(int32 playerId, std::string body, std::string popupSound = "") : playerId(playerId), body(body), popupSound(popupSound) {
		startTick = Time::Ticks();
	}

	PopupInfo(int32 playerId, std::string body, std::vector<std::string> choices, PopupReceiverEnum replyReceiver = PopupReceiverEnum::None, 
				bool forcedNetworking = false, std::string popupSound = "", int32 replyVar1 = -1)
			: playerId(playerId), body(body), choices(choices), replyReceiver(replyReceiver), popupSound(popupSound), replyVar1(replyVar1), forcedNetworking(forcedNetworking)
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
		
		SerializeStr(Ar, body);
		
		SerializeVecLoop(Ar, choices, [&](std::string& str) {
			SerializeStr(Ar, str);
		});
		
		Ar << replyReceiver;
		SerializeStr(Ar, popupSound);
		Ar << replyVar1;

		Ar << forcedNetworking;
		Ar << warningForExclusiveUI;
		
		Ar << startTick;
		Ar << startDisplayTick;
	}

	bool operator==(const PopupInfo& a) const
	{
		return playerId == a.playerId &&
			body == a.body &&
			choices == a.choices;
	}
};

enum class RareHandEnum : uint8
{
	InitialCards1,
	InitialCards2,
	RareCards,
	BuildingSlotCards,
	PopulationQuestCards, // PopulationQuest cards drive people to upgrade their houses
	CratesCards, // Crates gives resource cards
};

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

	std::string text;
	ResourceEnum resourceEnum = ResourceEnum::None;

	std::string text2;
	ResourceEnum resourceEnum2 = ResourceEnum::None;

	FloatupInfo() {}
	FloatupInfo(FloatupEnum floatupEnum, int32_t startTick, WorldTile2 tile, std::string text = "",
				ResourceEnum resourceEnum = ResourceEnum::None, std::string text2 = "")
			: floatupEnum(floatupEnum), startTick(startTick), tile(tile), text(text), resourceEnum(resourceEnum), text2(text2)
	{}

	void operator>>(FArchive& Ar)
	{
		Ar << floatupEnum;
		Ar << startTick;

		SerializeStr(Ar, text);
		Ar << resourceEnum;

		SerializeStr(Ar, text2);
		Ar << resourceEnum2;
	}
};

static const std::vector<std::string> MaleNames
{
	"Pun",
	"Pong",
	"Pep",
	
	"Omega",
	"Dutelut",
	"Grawen,"
	
	"Earn",
	"Nick",
	"John",
	"Joe",
	"Jack",
	"Jeff",
	"James",
	"Alistair",
	"George",
	"Sam",
	"Clement",
	"Tim",
	"Edwin",
	"Elvis",
	"Elvis",
	"Stephen",
	"Albert",
	"Kirk",
	"Todd",
	"Howard",
	"Wilson",
	"Geoffrey",
	"Joseph",
	"Abraham",
	"Lincoln",
	"Shawn",
	"Dave",
	"Danny",
	"Diego",
	"Donald",

	"Elton",
	"Chris",
	"Kevin",
	"Martin",
	"Fluffy",
	"Robert",
	"Marlon",
	"Denzel",
	"Arnold",
	"Daniel",
	"Sidney",
	"Clark",
	"Tom",
	"Trump",
	"Greg",
	"Kanye",
	"Leonardo",
	"Spencer",
	"Bruce",
	"Henry",
	"Morgan",
	"Johnny",
	"Charles",
	"Kirk",
	"Will",
	"Harrison",
	"Gary",
	"Gerard",
	"Justin",
	"Dustin",
	"Samuel",
	"Robin",
	"Don",
	"Eddie",
	"Anthony",
	"Lennon",
	"Tyler",
	"Steven",
	"Solomon",
	"Willie",
	"Frankie",
	"Ron",
	"Harry",
	"Bob",
	"Marvin",
	"Richard",
	"Freddie",
	"Marley",
	"Chuck",
	"Bobby",
	"Kurt",

	"Victor",
	"Vincent",
	"Bjorn",
	"Ragnar",

	// Discord
	"Maxo",
	"Noot",
	"Biffa",
	"Nookrium",
	"Dan",
	"Zakh",
	"Escoces",
	"Kirill",
	"Rick",
	"Indrik",
	"Boreale",
	
	
	"Jakob",
	"Jimba",
	"Rufio",
	"Tjelve",
	"Kuro",
	"Ralinad",
	"Shevy",
	"Coryn",
	
};

static const std::vector<std::string> FemaleNames
{
	"Catherine",
	"Helen",
	"Jeane",
	"Mary",
	"Jane",
	"Natasha",
	"Lucy",
	"Lyn",
	"Lidia",
	"Erica",
	"Maile",
	"Linda",
	"Cheryl",
	"Sheryl",
	"Chan",
	"Meghan",
	"Ava",
	"Natalie",
	"Molly",

	"Katherine",
	"Meryl",
	"Daniela",
	"Ingrid",
	"Elizabeth",
	"Betty",
	"Katy",
	"Kylie",
	"Kate",
	"Audrey",
	"Viola",
	"Sophia",
	"Vivien",
	"Judy",
	"Cindy",
	"Grace",
	"Angela",
	"Olivia",
	"Julie",
	"Isabelle",
	"Isabella",
	"Diane",
	"Rita",
	"Susan",
	"Angelina",
	"Jolie",
	"Sandra",
	"Emma",
	"Michelle",
	"Faye",
	"Nicole",
	"Karen",
	"Annie",
	"Mariah",
	"Hermione",
	"Patsy",

	"Freya",
	"Vivien",
	"Liza",
	
	// Discord
	"Venti",
	"Firis",
	"Illia",
	"Illya",
};

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
	std::string name;

	bool isLandmark() {
		return !isMountainOre() && 
				geoTreeEnum == TileObjEnum::None && 
				geoBushEnum == TileObjEnum::None;
	}
	bool isMountainOre() { return mountainOreEnum != TileObjEnum::None; }
	
	TileObjEnum mountainOreEnum = TileObjEnum::None;
	TileObjEnum geoTreeEnum;
	TileObjEnum geoBushEnum;

	ResourceEnum resourceEnum = ResourceEnum::None;
	
	std::string description;
};

static const std::vector<GeoresourceInfo> GeoresourceInfos
{
	{ GeoresourceEnum::Hotspring,			"Hot Spring" , TileObjEnum::None,TileObjEnum::None, TileObjEnum::None, ResourceEnum::None, "Grants happiness bonus to any house built in this region." },
	{ GeoresourceEnum::GiantMushroom,  "Giant Mushroom" , TileObjEnum::None,TileObjEnum::GiantMushroom, TileObjEnum::None, ResourceEnum::None, "Strange giant mushrooms. Grants science bonus to any house built in this region." },
	{ GeoresourceEnum::CherryBlossom,  "Cherry Blossom" , TileObjEnum::None,TileObjEnum::Cherry, TileObjEnum::None, ResourceEnum::None, "Area known for beautiful cherry blossoms. When they bloom, cherry blossoms give citizens bonus happiness." },

	//{ GeoresourceEnum::GiantTree,  "Giant tree" , TileObjEnum::None,TileObjEnum::None, TileObjEnum::None, "Grants happiness bonus to any house built in this region." },
	{ GeoresourceEnum::Ruin,				"Ruin" , TileObjEnum::None,TileObjEnum::None,TileObjEnum::None,   ResourceEnum::None,"Remains of a mysterious ancient civilization." },

	{ GeoresourceEnum::IronOre,			"Iron ore",	TileObjEnum::IronMountainOre,TileObjEnum::None,TileObjEnum::None, ResourceEnum::IronOre, "Build Iron Mine over this Iron Deposit to mine Iron." },
	{ GeoresourceEnum::CoalOre,			"Coal ore",	TileObjEnum::CoalMountainOre,TileObjEnum::None,TileObjEnum::None, ResourceEnum::Coal, "Build Coal Mine over this Coal Deposit to mine Coal." },
	{ GeoresourceEnum::GoldOre,			"Gold ore",	TileObjEnum::GoldMountainOre,TileObjEnum::None,TileObjEnum::None, ResourceEnum::GoldOre, "Build Gold Mine over this Gold Deposit to mine Gold." },
	{ GeoresourceEnum::Gemstone,			"Gemstone",	TileObjEnum::GemstoneOre,TileObjEnum::None,TileObjEnum::None, ResourceEnum::Gemstone, "Build Gemstone Mine over this Gemstone Deposit to mine Gemstone." },

	
	{ GeoresourceEnum::CannabisFarm,		"Cannabis" , TileObjEnum::None,TileObjEnum::None, TileObjEnum::Cannabis,  ResourceEnum::Cannabis,"Area suitable for growing Cannibis." },
	{ GeoresourceEnum::CocoaFarm,			"Cocoa" , TileObjEnum::None,TileObjEnum::None, TileObjEnum::Cocoa,  ResourceEnum::Cocoa,"Area suitable for growing Cocoa." },
	{ GeoresourceEnum::GrapeFarm,			"Grape" , TileObjEnum::None,TileObjEnum::None, TileObjEnum::Grapevines,  ResourceEnum::Grape,"Area suitable for growing Grape." },

	{ GeoresourceEnum::CottonFarm,		"Cotton" , TileObjEnum::None,TileObjEnum::None, TileObjEnum::Cotton,  ResourceEnum::Cotton,"Area suitable for growing Cotton." },
	{ GeoresourceEnum::DyeFarm,			"Dye" , TileObjEnum::None,TileObjEnum::None, TileObjEnum::Dye,  ResourceEnum::Dye,"Area suitable for growing Dye." },


	// Diamond, Copper

};

static const int32 GeoresourceEnumCount = GeoresourceInfos.size();

static const GeoresourceInfo GeoresourceInfoNone = { GeoresourceEnum::None, "", TileObjEnum::None, TileObjEnum::None, TileObjEnum::None, ResourceEnum::None, "" };

static GeoresourceInfo GetGeoresourceInfo(GeoresourceEnum georesourceEnum) {
	if (georesourceEnum == GeoresourceEnum::None) {
		return GeoresourceInfoNone;
	}
	return GeoresourceInfos[static_cast<int>(georesourceEnum)];
};
static GeoresourceInfo GetGeoresourceInfo(int32_t georesourceEnumInt) {
	if (static_cast<GeoresourceEnum>(georesourceEnumInt) == GeoresourceEnum::None) {
		return GeoresourceInfoNone;
	}
	return GeoresourceInfos[georesourceEnumInt];
};

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
		node.stoneAmount = 7500 + GameRand::Rand(provinceId) % 5000;
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
};

static const std::vector<SeedInfo> SpecialSeedCards
{
	{ CardEnum::CannabisSeeds, TileObjEnum::Cannabis, GeoresourceEnum::CannabisFarm },
	{ CardEnum::GrapeSeeds, TileObjEnum::Grapevines, GeoresourceEnum::GrapeFarm },
	{ CardEnum::CocoaSeeds, TileObjEnum::Cocoa, GeoresourceEnum::CocoaFarm },
	{ CardEnum::CottonSeeds, TileObjEnum::Cotton, GeoresourceEnum::CottonFarm },
	{ CardEnum::DyeSeeds, TileObjEnum::Dye, GeoresourceEnum::DyeFarm },
};

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

static const TArray<FString> TaxOptions = {
	FString("Very low tax"), 
	FString("Low tax"),
	FString("Medium tax"), 
	FString("High tax"),
	FString("Very high tax"), 
};
static const TArray<FString> VassalTaxOptions = {
	FString("No vassal tax"), 
	FString("Low vassal tax"),
	FString("Medium vassal tax"), 
	FString("High vassal tax"),
	FString("Very high vassal tax"), 
};


enum class AutosaveEnum : uint8
{
	Off,
	HalfYear,
	Year,
	TwoYears,
};
static const std::vector<FString> AutosaveOptions = {
	"Off",
	"Every Half Year",
	"Yearly",
	"Every Two Years",
};

/*
 * Stat
 */
enum class SeasonStatEnum
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

	Count,
};


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

static const std::string HappinessModifierName[] = 
{
	"luxury",
	"happy bread day",
	"bling bling",
	
	"tax",
	"cannibalism",
	"starvation death",
	"freezing death",
};

static const int32 HappinessModifierEnumCount = _countof(HappinessModifierName);

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


static const FString AITownNames[] = {
	"Stark",
	"Lannister",
	"Baratheon",
	"Survivor",
	"Zerg",
	"Rivendell",
	"Gotham",
	"Baltia",
	"Biarmaland",
	"Camelot",
	"Diyu",
	"Elysian",
	"Irkalla",
	"Nibiru",
	"Jotunheim",
	"Khmer",
	"Bangkok",
	"Talaki",
	"Bujia",
	"Barameria",
	"London",
	"Karaku",
	"Zakan",
	"Buroc",
	"Kiacook",
	"Bhuket",
	"Katao",
	"Bjorn",
	"Munso",
	"Malaren",
	"Rouen",
	"Hastein",
	"Borghild",
	"Uppsala",
	"Luna",
	"Havre",
	"Rohal",
	
};

static FString GetAITownName(int32_t playerId)
{
	int count = _countof(AITownNames);
	FString name = AITownNames[playerId % count];
	if (playerId / count > 0) {
		name += FString::FromInt(playerId / count);
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

static std::string GenerateTribeName(int32 seed)
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

	return ToStdString(finalName);
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
	ClaimLandFood,

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
};

static const int32 GameSpeedHalf = -12;
static const int32 GameSpeedValue1 = GameSpeedHalf;
static const int32 GameSpeedValue2 = 1;
static const int32 GameSpeedValue3 = 2;
static const int32 GameSpeedValue4 = 5;

static std::string GameSpeedName(int32 gameSpeed)
{
	switch(gameSpeed)
	{
	case GameSpeedValue1: return "half speed";
	case GameSpeedValue2: return "x1 speed";
	case GameSpeedValue3: return "x2 speed";
	case GameSpeedValue4: return "x5 speed";
	}
	return "";
}

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
static const int32 ClaimToIncomeRatio = 40; // When 2, actual is 4 since upkeep is half the income

struct ProvinceConnection
{
	int32 provinceId;
	TerrainTileType tileType;
	int32 connectedTiles;

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

enum class HoverWarning : uint8 {
	None,
	Depleted,
	StoragesFull,
	StorageTooFar,
	HouseTooFar,

	NotEnoughMoney,
	AlreadyReachedTarget,
	ResourcesBelowTarget,
};

static const std::vector<std::string> HoverWarningString = {
	"",
	"Depleted",
	"Storages Full",
	"Storage Too Far",
	"House Too Far",

	"Not Enough Money",
	"Target Reached",
	"Resources Below\nStorage Target",
};
static std::string GetHoverWarningString(HoverWarning hoverWarning) { return HoverWarningString[static_cast<int>(hoverWarning)]; }

/*
 * Game Constants
 */
static const int32 InitialStorageSize = 4;
static const WorldTile2 InitialStorageTileSize(4, 4);
static const int32 InitialStorageShiftFromTownhall = GetBuildingInfo(CardEnum::Townhall).size.y / 2 + InitialStorageSize / 2;
static const WorldTile2 Storage1ShiftTileVec(0, -InitialStorageShiftFromTownhall);
static const WorldTile2 InitialStorage2Shift(4, 0);