#pragma once

#include "UnrealEngine.h"
#include "PunCity/Simulation/IGameSimulationCore.h"

class MapUtil
{
public:
	static WorldTile2 UnpackAStarInt(uint32 locInt);
	static void UnpackAStarPath(std::vector<uint32>& rawWaypoint, std::vector<WorldTile2>& transformWaypoint);

	static WorldTile2 UnpackAStarInt_4x4(uint32 locInt);
	static void UnpackAStarPath_4x4(std::vector<uint32>& rawWaypoint, std::vector<WorldTile2>& transformWaypoint);

	//static WorldTile2 RandomTile(TileArea area);

	static FVector DisplayLocation(WorldAtom2 cameraAtom, WorldAtom2 objectAtom, float height = 0.0f);
	static FVector DisplayLocation_Map(WorldTile2 tile);
	
	static WorldAtom2 AtomLocation(WorldAtom2 cameraAtom, FVector displayLocation);

	static float GlobalDecalZoomFactor(float zoomDistance);

	static FVector GetCamShiftLocation(WorldAtom2 cameraAtom) {
		return FVector(cameraAtom.x, cameraAtom.y, 0.0f) / CoordinateConstants::AtomPerDisplayUnit;
	}

	//
};

static FString PunToString(FVector vec)
{
	return FString::Printf(TEXT("(%3.3f,%3.3f,%3.3f)"), vec.X, vec.Y, vec.Z);
}