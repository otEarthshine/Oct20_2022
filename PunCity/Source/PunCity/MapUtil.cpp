#include "PunCity/MapUtil.h"
#include "PunCity/Simulation/GameSimulationInfo.h"
#include "PunCity/GameRand.h"
#include "UnrealEngine.h"

using namespace std;

WorldTile2 MapUtil::UnpackAStarInt(uint32 locInt) 
{
	uint32 xRegion = locInt >> PunAStar128x256::shiftToXRegion;
	uint32 y = (locInt >> 5) & PunAStar128x256::bitsY;
	uint32 xBits = locInt & 0b11111; // 5 bits

	uint32 x = (xRegion << 5) | xBits;

	return WorldTile2(x, y);
}

void MapUtil::UnpackAStarPath(std::vector<uint32>& rawWaypoint, std::vector<WorldTile2>& transformWaypoint)
{
	transformWaypoint.resize(rawWaypoint.size());
	const int waypointSize = transformWaypoint.size();
	for (int i = 0; i < waypointSize; i++) {
		transformWaypoint[i] = MapUtil::UnpackAStarInt(rawWaypoint[i]);
	}
}


WorldTile2 MapUtil::UnpackAStarInt_4x4(uint32 locInt)
{
	uint32 xRegion = locInt >> PunAStar128x256::shiftToXRegion_4x4;
	uint32 y = (locInt >> 5) & PunAStar128x256::bitsY_4x4;
	uint32 xBits = locInt & 0b11111; // 5 bits

	uint32 x = (xRegion << 5) | xBits;

	return WorldTile2(x * 4, y * 4);
}

void MapUtil::UnpackAStarPath_4x4(std::vector<uint32>& rawWaypoint, std::vector<WorldTile2>& transformWaypoint)
{
	transformWaypoint.resize(rawWaypoint.size());
	const int waypointSize = transformWaypoint.size();
	for (int i = 0; i < waypointSize; i++) {
		transformWaypoint[i] = MapUtil::UnpackAStarInt_4x4(rawWaypoint[i]);
	}
}

//WorldTile2 MapUtil::RandomTile(TileArea area)
//{
//	area.EnforceWorldLimit();
//	int minX = area.minX;
//	int minY = area.minY;
//	int maxX = area.maxX;
//	int maxY = area.maxY;
//
//	int16_t randX = minX + GameRand::Rand() % (maxX - minX + 1);
//	int16_t randY = minY + GameRand::Rand() % (maxY - minY + 1);
//
//	check(GameMap::PathAI != nullptr);
//	auto& pathAI = *GameMap::PathAI;
//
//	for (int16_t x = randX; x <= maxX; x++) {
//		for (int16_t y = randY; y <= maxY; y++)
//		{
//			check(x >= 0 && y >= 0 && x < GameMapConstants::TilesPerWorldX && y < GameMapConstants::TilesPerWorldY);
//			if (pathAI.isWalkable(x, y)) {
//				return WorldTile2(x, y);
//			}
//		}
//	}
//
//	return WorldTile2::Invalid;
//}

FVector MapUtil::DisplayLocation(WorldAtom2 cameraAtom, WorldAtom2 objectAtom, float height)
{
	WorldAtom2 displayAtomLocation = objectAtom - cameraAtom;
	return FVector(displayAtomLocation.x / CoordinateConstants::AtomPerDisplayUnit, 
					displayAtomLocation.y / CoordinateConstants::AtomPerDisplayUnit, height);
}

//FVector MapUtil::DisplayLocationMapMode(FVector mapCameraLocation, WorldAtom2 objectAtom)
//{
//	FVector locationWithinMap = FVector(objectAtom.x, objectAtom.y, 0) / CoordinateConstants::AtomPerMapDisplayUnit;
//	return locationWithinMap - mapCameraLocation;
//}


WorldAtom2 MapUtil::AtomLocation(WorldAtom2 cameraAtom, FVector displayLocation)
{
	WorldAtom2 displayAtomLocation(displayLocation.X * CoordinateConstants::AtomPerDisplayUnit, 
									displayLocation.Y * CoordinateConstants::AtomPerDisplayUnit);
	return displayAtomLocation + cameraAtom;
}

float MapUtil::GlobalDecalZoomFactor(float zoomDistance)
{
	float zoomFactor = max(1.0f, zoomDistance / 700.0f);
	return zoomFactor;
}
