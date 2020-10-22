// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "UObject/Interface.h"
#include "../Simulation/GameSimulationCore.h"
#include "GameDisplayInfo.h"
#include "Engine/PostProcessVolume.h"
#include "PunCity/PunGameInstance.h"

#include "DisplaySystemDataSource.generated.h"

// This class does not need to be modified.
UINTERFACE(MinimalAPI)
class UDisplaySystemDataSource : public UInterface
{
	GENERATED_BODY()
};

/**
 * 
 */
class IDisplaySystemDataSource
{
	GENERATED_BODY()
public:
	virtual int32 playerId() = 0;
	
	virtual class UAssetLoaderComponent* assetLoader() = 0;

	virtual class IGameNetworkInterface* networkInterface() = 0;
	
	virtual GameSimulationCore& simulation() = 0;
	virtual USceneComponent* componentToAttach() = 0;

	virtual OverlayType GetOverlayType() = 0;
	virtual bool isHidingTree() = 0;
	virtual bool isShowingProvinceOverlay() = 0;
	
	virtual class ULineBatchComponent* lineBatch() = 0;

	virtual const GameDisplayInfo& displayInfo() = 0;

	//virtual bool isMapMode() = 0;
	virtual float zoomDistance() = 0;
	virtual float smoothZoomDistance() = 0;
	
	virtual bool ZoomDistanceBelow(float threshold) = 0;
	
	virtual WorldAtom2 cameraAtom() = 0;
	virtual TileArea sampleArea() = 0;

	virtual const std::vector<int32>& sampleProvinceIds() = 0;

	virtual FVector DisplayLocation(WorldAtom2 atom) = 0;
	//virtual FVector DisplayLocationMapMode(WorldAtom2 atom) = 0;

	virtual int timeSinceTickStart() = 0;

	virtual float GetTerrainDisplayHeight(WorldTile2 tile) = 0;

	virtual bool isPhotoMode() = 0;

	virtual bool IsInSampleRange(WorldTile2 tile) = 0;

	virtual float GetTrailerTime() = 0;

	static void SetPostProcessVolume(APostProcessVolume* postProcessVolume, UPunGameInstance* gameInstance)
	{
		postProcessVolume->Settings.bOverride_AutoExposureMinBrightness = true;
		postProcessVolume->Settings.bOverride_AutoExposureMaxBrightness = true;
		postProcessVolume->Settings.AutoExposureMinBrightness = 0.45;
		postProcessVolume->Settings.AutoExposureMaxBrightness = 0.45;

		postProcessVolume->Settings.bOverride_MotionBlurAmount = true;
		postProcessVolume->Settings.MotionBlurAmount = 0;
		
		postProcessVolume->Settings.AmbientOcclusionIntensity = 0.75f;
		postProcessVolume->Settings.AmbientOcclusionRadius = 20.0f;
		postProcessVolume->Settings.AmbientOcclusionQuality = 0;
		postProcessVolume->Settings.AmbientOcclusionMipBlend = 1;
		
		postProcessVolume->Settings.LPVIntensity = 0;
		postProcessVolume->Settings.ScreenSpaceReflectionQuality = 0;

		postProcessVolume->Settings.bOverride_ScreenPercentage = true;
		postProcessVolume->Settings.ScreenPercentage = gameInstance->GetDisplayResolutionQuality();
		
		//PUN_LOG("SetPostProcessVolume ScreenPercentage %f %f", postProcessVolume->Settings.ScreenPercentage, gameInstance->resolutionQuality());
	}
};

