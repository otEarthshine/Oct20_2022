// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include <unordered_map>

/**
 * 
 */
class PunSettings
{
public:
	static std::unordered_map<std::string, int32> Settings;

	static int32 Get(const FString& name) {
		return Settings[std::string(TCHAR_TO_UTF8(*name))];
	}

	static void Set(const FString& name, int32 value) {
		Settings[std::string(TCHAR_TO_UTF8(*name))] = value;

		bShouldRefreshMainMenuDisplay = true;
	}

	static void Toggle(const FString& name) {
		Set(name, !Get(name));
	}

	static bool IsOn(std::string settingName) {
		check(Settings.find(settingName) != Settings.end());
		return Settings[settingName] > 0;
	}

	static bool bShouldRefreshMainMenuDisplay;

	/*
	 * Fast Settings
	 */
	static void SetTrailerMode(bool value) { _TrailerMode = value; }
	static bool TrailerMode() { return _TrailerMode; }

	static void SetCheatFullFarmRoad(bool value) { _CheatFullFarmRoad = value; }
	static bool CheatFullFarmRoad() { return _CheatFullFarmRoad; }

	static bool MarkedTreesNoDisplay;
	
	static bool TrailerSession;
	static struct WorldTile2 TrailerTile_Chopper;
	static struct WorldTile2 TrailerTile_Builder;

	static struct WorldAtom2 TrailerAtomStart_Ship;
	static struct WorldAtom2 TrailerAtomTarget_Ship;
	static float TrailerShipStartTime;
	static float TrailerShipTargetTime;
	
private:
	static bool _TrailerMode;
	static bool _CheatFullFarmRoad;
};


class SimSettings
{
public:
	static std::unordered_map<std::string, int32> Settings;

	static int32 Get(const FString& name) {
		return Settings[std::string(TCHAR_TO_UTF8(*name))];
	}

	static void Set(const FString& name, int32 value) {
		Settings[std::string(TCHAR_TO_UTF8(*name))] = value;
	}

	static void Toggle(const FString& name) {
		std::string nameStr(TCHAR_TO_UTF8(*name));
		if (Settings.find(nameStr) == Settings.end()) {
			Settings[nameStr] = 1; // First toggle is on
		}
		Settings[nameStr] = !(Settings[nameStr]);
	}

	static bool IsOn(std::string settingName) {
		if (Settings.find(settingName) == Settings.end()) {
			Settings[settingName] = 0; // Create off if there is no setting...
		}
		return Settings[settingName] > 0;
	}
};