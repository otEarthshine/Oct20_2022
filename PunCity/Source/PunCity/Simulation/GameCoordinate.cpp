#include "GameCoordinate.h"
#include "GameSimulationConstants.h"

using namespace std;

//! WorldTile2

WorldTile2::WorldTile2(int32_t tileId) 
{
	x = tileId % GameMapConstants::TilesPerWorldX;
	y = tileId / GameMapConstants::TilesPerWorldX;
}

const WorldTile2 WorldTile2::Zero = WorldTile2(0, 0);
const WorldTile2 WorldTile2::Invalid = WorldTile2(-1, -1);

int32_t WorldTile2::tileId() const { return x + y * GameMapConstants::TilesPerWorldX; }

WorldRegion2 WorldTile2::region() const  {
	return WorldRegion2(x / CoordinateConstants::TilesPerRegion, y / CoordinateConstants::TilesPerRegion);
}

int32_t WorldTile2::regionId() const {
	return WorldRegion2(x / CoordinateConstants::TilesPerRegion, y / CoordinateConstants::TilesPerRegion).regionId();
}

LocalTile2 WorldTile2::localTile() const  {
	return LocalTile2(x % CoordinateConstants::TilesPerRegion, y % CoordinateConstants::TilesPerRegion);
}

int16_t WorldTile2::localTileId() const {
	int16_t xx = x % CoordinateConstants::TilesPerRegion;
	int16_t yy = y % CoordinateConstants::TilesPerRegion;
	return xx + yy * CoordinateConstants::TilesPerRegion;
}

LocalTile2 WorldTile2::localTile(WorldRegion2 region) const
{
	return LocalTile2(x - region.x * CoordinateConstants::TilesPerRegion, 
						y - region.y * CoordinateConstants::TilesPerRegion);
}

std::vector<WorldRegion2> WorldTile2::nearby4Regions() const
{
	LocalTile2 local = this->localTile();
	int32_t halfRegionTiles = CoordinateConstants::TilesPerRegion / 2;
	bool usePosX = local.x >= halfRegionTiles;
	bool usePosY = local.y >= halfRegionTiles;

	WorldRegion2 reg = region();
	
	std::vector<WorldRegion2> results;
	results.push_back(reg);
	results.push_back(usePosX ? WorldRegion2(reg.x + 1, reg.y) : WorldRegion2(reg.x - 1, reg.y)); // N S
	results.push_back(usePosY ? WorldRegion2(reg.x, reg.y + 1) : WorldRegion2(reg.x, reg.y - 1)); // E W
	if (usePosX && usePosY) results.push_back(WorldRegion2(reg.x + 1, reg.y + 1));
	if (!usePosX && usePosY) results.push_back(WorldRegion2(reg.x - 1, reg.y + 1));
	if (usePosX && !usePosY) results.push_back(WorldRegion2(reg.x + 1, reg.y - 1));
	if (!usePosX && !usePosY) results.push_back(WorldRegion2(reg.x - 1, reg.y - 1));
	return results;
}

//! LocalTile2

WorldTile2 LocalTile2::worldTile2(WorldRegion2 region) {
	return WorldTile2(region.x * CoordinateConstants::TilesPerRegion + x, region.y * CoordinateConstants::TilesPerRegion + y);
}

//! WorldTile4x4

//! WorldAtom2

WorldTile2 WorldAtom2::worldTile2() const {
	return WorldTile2(x / CoordinateConstants::AtomsPerTile, y / CoordinateConstants::AtomsPerTile);
}

WorldRegion2 WorldAtom2::region() const {
	return WorldRegion2(x / CoordinateConstants::AtomsPerRegion, y / CoordinateConstants::AtomsPerRegion); 
}

WorldAtom2 WorldAtom2::Zero = WorldAtom2(0, 0);
WorldAtom2 WorldAtom2::Invalid = WorldAtom2(-1, -1);

int32_t WorldAtom2::Distance(WorldAtom2 start, WorldAtom2 end)
{
	int64_t diffX = static_cast<int64_t>(end.x - start.x);
	int64_t diffY = static_cast<int64_t>(end.y - start.y);

	int64_t sumSquare = diffX * diffX + diffY * diffY;
	return static_cast<int32_t>(sqrt(static_cast<double>(sumSquare)));
}

bool WorldAtom2::DistanceLessThan(WorldTile2 start, WorldTile2 end, int32_t tileRadius) {
	return Distance(start.worldAtom2(), end.worldAtom2()) < tileRadius * CoordinateConstants::AtomsPerTile;
}

WorldAtom2 WorldAtom2::Lerp(WorldAtom2 start, WorldAtom2 end, int64_t fraction100000)
{
	return WorldAtom2(start.x + static_cast<int64_t>(end.x - start.x) * fraction100000 / 100000,
					start.y + static_cast<int64_t>(end.y - start.y) * fraction100000 / 100000);
}

WorldRegion2::WorldRegion2(int32 regionId)
{
	x = regionId % GameMapConstants::RegionsPerWorldX;
	y = regionId / GameMapConstants::RegionsPerWorldX;
}

int32 WorldRegion2::regionId() const { return x + y * GameMapConstants::RegionsPerWorldX; }

//! TileArea

TileArea::TileArea(WorldTile2 start, int16_t distanceAway)
{
	WorldTile2 min = start - distanceAway;
	WorldTile2 max = start + distanceAway; // max inclusive
	minX = min.x;
	minY = min.y;
	maxX = max.x;
	maxY = max.y;
}

void TileArea::EnforceWorldLimit()
{
	minX = minX >= 0 ? minX : 0;
	minY = minY >= 0 ? minY : 0;
	maxX = maxX < GameMapConstants::TilesPerWorldX ? maxX : GameMapConstants::TilesPerWorldX - 1;
	maxY = maxY < GameMapConstants::TilesPerWorldY ? maxY : GameMapConstants::TilesPerWorldY - 1;
}

TileArea TileArea::Invalid = TileArea(-1, -1, -1, -1);