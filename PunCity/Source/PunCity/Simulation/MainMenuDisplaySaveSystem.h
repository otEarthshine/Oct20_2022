// Pun Dumnernchanvanit's

#pragma once

#include "CoreMinimal.h"
#include "PunCity/Simulation/GameSimulationCore.h"
#include "PunCity/UI/GameUIDataSource.h"
#include "PunCity/GameNetworkInterface.h"
#include "PunCity/UI/UISystemBase.h"
#include <iomanip>
#include "PunCity/PunFileUtils.h"


struct MainMenuCameraState
{
	WorldAtom2 cameraAtom;
	float zoomDistance = -1.0f;
	FTransform worldTransform;
	FRotator rotator;

	MapSizeEnum mapSizeEnum;

	std::vector<int32> sampleRegionIds;
};

/**
 * 
 */
class MainMenuDisplaySaveSystem
{
public:
	static bool SaveOrLoad(GameSimulationCore& simulation, MainMenuCameraState& cameraState, bool isSaving)
	{
#if !MAIN_MENU_DISPLAY
		return false;
#endif
		
		_LOG(PunSaveLoad, "MainMenuDisplaySaveSystem: SaveOrLoad %d", isSaving);
		
		FString path = FPaths::ConvertRelativePathToFull(FPaths::ProjectContentDir());
		FString saveFileName = "Paks/MainMenuDisplayCache.dat";

		auto serialize = [&](FArchive& Ar)
		{
			cameraState.cameraAtom >> Ar;
			Ar << cameraState.zoomDistance;
			Ar << cameraState.worldTransform;
			SerializeVecValue(Ar, cameraState.sampleRegionIds);
			Ar << cameraState.rotator;

			simulation.SerializeForMainMenu(Ar, cameraState.sampleRegionIds);
		};

		if (isSaving) {
			return PunFileUtils::SaveFile(path + saveFileName, serialize);
		}
		return PunFileUtils::LoadFile(path + saveFileName, serialize);
	}

	static bool LoadCameraOnly(MainMenuCameraState& cameraState)
	{
		_LOG(PunSaveLoad, "LoadCameraOnly");
		
		FString path = FPaths::ConvertRelativePathToFull(FPaths::ProjectContentDir());
		FString saveFileName = "Paks/MainMenuDisplayCache.dat";

		auto serialize = [&](FArchive& Ar) {
			cameraState.cameraAtom >> Ar;
			Ar << cameraState.zoomDistance;
			Ar << cameraState.worldTransform;
			SerializeVecValue(Ar, cameraState.sampleRegionIds);
			Ar << cameraState.rotator;
		};

		return PunFileUtils::LoadFile(path + saveFileName, serialize);
	}
};
