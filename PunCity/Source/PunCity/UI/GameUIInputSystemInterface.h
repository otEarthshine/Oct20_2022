// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "UObject/Interface.h"
#include "../Simulation/GameSimulationInfo.h"

#include "GameUIInputSystemInterface.generated.h"

// This class does not need to be modified.
UINTERFACE(MinimalAPI)
class UGameUIInputSystemInterface : public UInterface
{
	GENERATED_BODY()
};

/**
 * 
 */
class IGameUIInputSystemInterface
{
	GENERATED_BODY()
public:
	virtual void StartBuildingPlacement(CardEnum buildingEnum, int32 buildingLvl, bool useBoughtCard, CardEnum useWildCard = CardEnum::None) = 0;
	virtual void StartHarvestPlacement(bool isRemoving, ResourceEnum resourceEnum) = 0;
	virtual void StartDemolish() = 0;
	virtual void StartRoadPlacement(bool isStoneRoad) = 0;
	virtual void StartFencePlacement() = 0;
	virtual void StartBridgePlacement() = 0;

	//virtual int32_t GetCardHandIndexBeingPlaced() = 0;
	virtual CardEnum GetBuildingEnumBeingPlaced() = 0;
	
	virtual PlacementInfo PlacementBuildingInfo() = 0;
	virtual PlacementType placementState() = 0;
	virtual void CancelPlacement() = 0;

	virtual WorldAtom2 cameraAtom() = 0;

	virtual bool isSystemMovingCamera() = 0;

	virtual void Serialize(FArchive& archive) = 0;

	virtual float GetMouseZoomSpeedFraction() = 0;
	virtual void SetMouseZoomSpeedFraction(float fraction) = 0;
};
