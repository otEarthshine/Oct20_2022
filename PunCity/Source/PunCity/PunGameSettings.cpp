// Fill out your copyright notice in the Description page of Project Settings.

#include "PunCity/PunGameSettings.h"
#include "PunCity/GameConstants.h"
#include "PunCity/Simulation/GameCoordinate.h"

bool PunSettings::bShouldRefreshMainMenuDisplay = true;


bool PunSettings::MarkedTreesNoDisplay = false;
bool PunSettings::TrailerSession = false;
WorldTile2 PunSettings::TrailerTile_Chopper = WorldTile2::Invalid;
WorldTile2 PunSettings::TrailerTile_Builder = WorldTile2::Invalid;

WorldAtom2 PunSettings::TrailerAtomStart_Ship = WorldAtom2::Invalid;
WorldAtom2 PunSettings::TrailerAtomTarget_Ship = WorldAtom2::Invalid;
float PunSettings::TrailerShipStartTime = -1;
float PunSettings::TrailerShipTargetTime = -1;

bool PunSettings::_TrailerMode = false;
bool PunSettings::_CheatFullFarmRoad = false;

std::unordered_map<std::string, int32> PunSettings::Settings = 
{
	{ "WalkableGrid", 0 },
	{ "TreeGrid", 0 },
	{ "ShowPerlinLines", 0 },

	{ "PathLines", 0 },
	{ "UnitLines", 0 },
	{ "BuildingLines", 0},
	{ "DropLines", 0},

	{ "BuildingGrid", 0 },
	{ "ShippingGrid", 0 },
	{ "BuildingId", 0 },
	{ "FloodId", 0},
	{ "FloodIdHuman", 0},
	{ "Province", 0 },
	{ "ProvinceFlatConnectionOnly", 0},

	{ "PlayerCount", 1 },

	{ "ActionLog", -1 },
	{ "ActionLogAll", 0 },

	//! UI
	{ "UIActions", 0 },
	{ "MapIcon", 1 },

	//! Display
	{ "DisplayBuildings", 1 },
	{ "DisplayMiniBuildings", 1 },
	
	{ "DisplayUnits", 1 },
	{ "DisplayRegions", 1 },
	{ "DisplayRegions4x4", 1 },
	{ "DisplayDecals", 1 },
	{ "DisplayTiles", 1 },
	{ "DisplayResources", 1 },
	{ "DisplayTerritory", 1 },
	{ "DisplayTerrainMap", 1 },
	{ "DisplayMapUI", 1 },
	{ "MiniTree", 0},

	{ "MapTree", 1 },
	{ "SingleSample", 0},

	{ "TileObjFull", 1}, // Turn this off to turn off grass, Must refresh manually if in game
	{ "PlainTerrain", 0 },
	{ "HideWater", 0 },

	//! Ticks
	{ "TickBuildings", 1 },
	{ "TickUnits", 1 },
	{ "TickPlayerOwned", 1 },
	{ "TickTiles", 1 },
	{ "TickUnlocks", 1 },
	{ "TickStats", 1 },

	//! Debug
	{ "DebugUI", 0 },
	{ "SoundDebugUI", 0 },
	{ "DebugTemp", 0 },
	{ "DebugTemp1", 0 },
	{ "DebugTemp2", 0 },
	{ "DebugTemp3", 0 },
	{ "DebugTemp4", 0 },
	{ "DebugTemp5", 0 },
	{ "ShowFullDebugLog", 1 },

	//! TickUI
	{ "UIWorldSpace", 1 },
	{ "UIEscMenu", 1 },
	{ "UIChat", 1 },
	{ "UIQuest", 1 },
	{ "UIMainGame", 1 },


#if WITH_EDITOR
	{ "SimLine", 1 },
	{ "AutoLocation", 0 },
	{ "SinglePlayerChat", 1 },
#else
	{ "SimLine", 0 },
	{ "AutoLocation", 0 },
	{ "SinglePlayerChat", 0 },
#endif

	//! Initial settings
	{ "InitialAnimals", GameConstants::InitialWorldPop },
	//{ "InitialAnimals", 1 },

	{ "AllowCameraPitch", 0 },

	{ "ForceSnow", 0 },
	{ "ForceSnowDelay", 5 },
	{ "ForceSnowPercent", 0 },
	{ "ForceSnowSpeed", 100 },
	{ "ForceAutumn", 0 },
	{ "ForceAutumnTicks", 120 * 2 },

	{ "ForceNoFog", 0 },
	{ "ForceNoRain", 0 },
	{ "ForceNoSnow", 0 },
	{ "ToggleRain", 0 },

	// Debug Toggle
	{ "MultithreadedMeshGeneration", 1 },
	{ "ForceClickthrough", 0 },
	{ "ShowDebugExtra", 0 },

	{ "CacheWaterRoutes", 1 },

	{ "SuppressHoverIcon", 0 },
	{ "URO", 0 },
	{ "URODebug", 0 },

	// Debug Network
	{ "ForceDelayInput", 0 },
	{ "FixedDeltaTime", 1 },

	// Trailer
	
	{ "TrailerTundraMinSnowPercent", 0 },
	{ "TrailerPlaceSpeed", 100 },
	{ "TrailerHouseUpgradeSpeed", 100 },
	{ "TrailerBeatShiftBack", 0 },
	{ "TrailerTimePerBeat", 40 },
	{ "TrailerBeatOn", 1 },
	
	{ "TrailerShipTime", 300 },
	{ "TrailerNoTreeRefresh", 0 },
	{ "TrailerRoadPerTick", 3 },
};


std::unordered_map<std::string, int32> SimSettings::Settings =
{
	{ "CheatFastBuild", 0 },
	{ "CheatUndead", 0 },
	{ "CheatFastTech", 0},

	{ "CheatHouseLevel", 1},
	{ "CheatHouseLevelKey", 0},
};