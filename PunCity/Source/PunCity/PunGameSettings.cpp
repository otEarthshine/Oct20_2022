// Fill out your copyright notice in the Description page of Project Settings.

#include "PunCity/PunGameSettings.h"
#include "PunCity/GameConstants.h"
#include "PunCity/Simulation/GameCoordinate.h"

bool PunSettings::bShouldRefreshMainMenuDisplay = true;


bool PunSettings::MarkedTreesNoDisplay = false;
bool PunSettings::TrailerSession = false;
WorldTile2 PunSettings::TrailerTile_Chopper = WorldTile2::Invalid;
WorldTile2 PunSettings::TrailerTile_Builder = WorldTile2::Invalid;

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
	{ "DebugTemp", 0 },
	{ "DebugTemp1", 0 },
	{ "DebugTemp2", 0 },
	{ "DebugTemp3", 0 },
	{ "DebugTemp4", 0 },
	{ "DebugTemp5", 0 },

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

	{ "ForceSnow", 0 },
	{ "ForceSnowPercent", 0 },
	{ "ForceAutumn", 0 },
	{ "TrailerTundraMinSnowPercent", 0 },
	{ "TrailerPlaceSpeed", 100 },
	{ "TrailerHouseUpgradeSpeed", 100 }
	//{ "ForceShowTileObj", 0 },
};


std::unordered_map<std::string, int32> SimSettings::Settings =
{
	{ "CheatFastBuild", 0 },
	{ "CheatUndead", 0 },
	{ "CheatFastTech", 0},

	{ "CheatHouseLevel", 1},
	{ "CheatHouseLevelKey", 0},
};