#include "PunCity/Simulation/GameSimulationInfo.h"
#include <algorithm>
//#include "PunCity/PunUtils.h"

int32 TimeDisplay::_Ticks = 0;

float Time::kForcedFallSeason = 0.0f;

TileArea BuildingArea(WorldTile2 centerTile, WorldTile2 size, Direction faceDirection)
{
	check(size.isValid());

	// If rotated 90 or 270, size must be reversed
	if (faceDirection == Direction::W || faceDirection == Direction::E) {
		size = WorldTile2(size.y, size.x);
	}

	int16 xShiftDown = 0;
	int16 yShiftDown = 0;

	if (faceDirection == Direction::E) {
		yShiftDown = 1;
	}
	else if (faceDirection == Direction::N) {
		xShiftDown = 1;
		yShiftDown = 1;
	}
	else if (faceDirection == Direction::W) {
		xShiftDown = 1;
	}

	int16 minX = centerTile.x - (size.x - 1 + xShiftDown) / 2;
	int16 maxX = centerTile.x + (size.x - xShiftDown) / 2;
	int16 minY = centerTile.y - (size.y - 1 + yShiftDown) / 2;
	int16 maxY = centerTile.y + (size.y - yShiftDown) / 2;

	return TileArea(minX, minY, maxX, maxY);
}
WorldTile2 GetBuildingCenter(TileArea area, Direction faceDirection)
{
	//if (faceDirection == Direction::S) {
	//	
	//}
	return WorldTile2::Invalid;
}


/**
 * Map
 */

void GameMap::SetRegionsPerWorld(int regionPerWorldX, int regionPerWorldY)
{
	// TODO: get rid of GameMap
	GameMapConstants::SetRegionsPerWorld(regionPerWorldX, regionPerWorldY);
}

int32 Time::_Ticks = 0;

int32 Time::_Seconds = 0;
int32 Time::_Minutes = 0;
int32 Time::_Rounds = 0;
int32 Time::_Seasons = 0;
int32 Time::_Years = 0;

FloatDet Time::_MinCelsiusBase = 0;
FloatDet Time::_MaxCelsiusBase = 0;


void Time::ResetTicks()
{
	SetTickCount(0);

	_MinCelsiusBase = IntToFD(-10);
	_MaxCelsiusBase = IntToFD(28);
}

void Time::SetTickCount(int32 tickCount)
{
	_Ticks = tickCount;
	_Seconds = _Ticks / 60;
	_Minutes = _Seconds / 60;
	_Rounds = _Seconds / 150;
	_Seasons = _Minutes / MinutesPerSeason;
	_Years = _Seasons / 4;

}
