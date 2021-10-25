#pragma once

#include "CoreMinimal.h"
#include "PunCity/Simulation/GameSimulationInfo.h"
#include "PunCity/DisplaySystem/GameDisplayInfo.h"
#include "PunCity/NetworkStructs.h"

#include "UObject/Interface.h"
#include "GameManagerInterface.generated.h"

// This class does not need to be modified.
UINTERFACE(MinimalAPI)
class UGameManagerInterface : public UInterface
{
	GENERATED_BODY()
};

/**
 * Interface that allows InputSystem to call GameManager 
 * Simulation can also GameManager for display?
 */
class IGameManagerInterface
{
	GENERATED_BODY()
public:
	virtual int32 playerId() = 0;
	
	virtual FMapSettings GetMapSettings() = 0;
	virtual bool HasSavedMap(const FMapSettings& mapSettings) = 0;

	virtual int32 playerCount() = 0;
	virtual FString playerNameF(int32 playerId) = 0;

	virtual struct FPlayerInfo playerInfo(int32 playerId) = 0;

	virtual std::vector<int32> allHumanPlayerIds() = 0;
	virtual std::vector<int32> connectedPlayerIds(bool withReplayPlayers = true) = 0;
	virtual std::vector<int32> disconnectedPlayerIds() = 0;

	virtual bool isSinglePlayer() = 0;
	virtual AutosaveEnum autosaveEnum() = 0;

	virtual void SendNetworkCommand(std::shared_ptr<FNetworkCommand> networkCommand) = 0;
	
	//virtual bool IsPlayerBuildable(WorldTile2 tile) const = 0;
	virtual bool IsPlayerRoadBuildable(WorldTile2 tile) const = 0;

	virtual bool IsPlayerTunnelBuildable(WorldTile2 tile) const = 0;

	virtual int32 GetMinEraDisplay(FactionEnum factionEnum, CardEnum buildingEnum) const = 0;

	virtual GeoresourceNode RegionGeoresource(WorldRegion2 region) = 0;

	virtual void ResizeDisplaySystemBuildingIds(int newSize) = 0;

	virtual void SetOverlayType(OverlayType overlayType, OverlaySetterType setterType) = 0;
	virtual void SetOverlayTile(WorldTile2 overlayCenterTile) = 0;
	virtual WorldTile2 GetOverlayTile() = 0;

	virtual void ToggleOverlayHideTree() = 0;
	virtual void ToggleOverlayProvince() = 0;
	virtual void ToggleOverlayDefense() = 0;
	
	virtual void ToggleOverlayFertility() = 0;
	virtual void ToggleOverlayAppeal() = 0;

	virtual class ULineBatchComponent* lineBatch() = 0;

	virtual class GameSimulationCore& simulation() = 0;

	virtual const GameDisplayInfo& displayInfo() = 0;

	virtual void SetCtrl(bool isDown) = 0;
	virtual void SetShift(bool isDown) = 0;

	virtual bool isShiftDown() = 0;
	virtual bool isCtrlDown() = 0;

	virtual void DrawLine(WorldAtom2 atom, FVector startShift, WorldAtom2 endAtom, FVector endShift, FLinearColor Color,
						 float Thickness = 1.0f, float LifeTime = 10000) = 0;

	virtual float GetDisplayWorldTime() = 0;

	virtual TArray<FString> GetReplayFileNames() = 0;

	virtual void ExecuteCheat(CheatEnum cheatEnum) = 0;

	// Audio
	virtual void SpawnResourceDropoffAudio(ResourceEnum resourceEnum, WorldAtom2 worldAtom) = 0;
	virtual void SpawnAnimalSound(UnitEnum unitEnum, bool isAngry, WorldAtom2 worldAtom, bool usePlayProbability = false) = 0;
	virtual void Spawn3DSound(std::string groupName, std::string soundName, WorldAtom2 worldAtom, float height) = 0;
	virtual void Spawn2DSound(std::string groupName, std::string soundName) = 0;

	// Display
	virtual void RefreshHeightForestColorTexture(TileArea area, bool isInstant) = 0;
	virtual void SetRoadWorldTexture(WorldTile2 tile, bool isRoad, bool isDirtRoad) = 0;
	virtual void RefreshHeightForestRoadTexture() = 0;

	virtual void RefreshMapAnnotation() = 0;

	virtual float zoomDistance() = 0;
	virtual bool IsInSampleRange(WorldTile2 tile) = 0;
	virtual const std::vector<int32>& sampleRegionIds() = 0;

	// Debug
#if CHECK_TICKHASH
	virtual void CheckDesync(bool checkSucceed, FString desyncMessage, int32 tick = -1) = 0;
#endif
};
