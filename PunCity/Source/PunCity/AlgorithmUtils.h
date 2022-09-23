// Pun Dumnernchanvanit's

#pragma once

#include "PunCity/Simulation/GameSimulationInfo.h"
#include "PunCity/PunUtils.h"

/**
 * 
 */
class AlgorithmUtils
{
public:
	template<typename Func>
	static WorldTile2 FindNearbyAvailableTile(WorldTile2 start, Func isAvailable, int32 maxLookup = 2000)
	{
		// Spiral
		int x = 0;
		int y = 0;
		int dx = 0;
		int dy = -1;
		for (int i = 0; i < maxLookup; i++) {
			WorldTile2 tile = start + WorldTile2(x, y);
			if (tile.isValid()) {
				if (isAvailable(tile)) {
					return tile;
				}
			}
			if ((x == y) || ((x < 0) && (x == -y)) || ((x > 0) && (x == 1 - y))) {
				int temp = dx;
				dx = -dy;
				dy = temp;
			}
			x += dx;
			y += dy;
		}
		//UE_DEBUG_BREAK();
		return WorldTile2::Invalid;
	}

	// Note: isTileAvailable must include tile.isValid()
	template<typename Func>
	static WorldTile2 FindNearbyAvailableArea(TileArea startArea, Func isTileAvailable, int32 maxLookup = 2000)
	{
		// Spiral
		int x = 0;
		int y = 0;
		int dx = 0;
		int dy = -1;

		int32 invalidTileCount = 0;
		startArea.ExecuteOnArea_WorldTile2([&](WorldTile2 tile) {
			if (!isTileAvailable(tile)) {
				invalidTileCount++;
			}
		});

		PUN_LOG("FindNearbyAvailableArea -- startArea:%s", ToTChar(startArea.ToString()));
		
		for (int i = 0; i < maxLookup; i++) 
		{
			PUN_LOG("-- %d i:%d - x:%d y:%d dx:%d dy:%d", invalidTileCount, i, x, y, dx, dy);
			
			if (invalidTileCount == 0) {
				return WorldTile2(x, y);
			}
			if ((x == y) || ((x < 0) && (x == -y)) || ((x > 0) && (x == 1 - y))) {
				int temp = dx;
				dx = -dy;
				dy = temp;
			}
			x += dx;
			y += dy;

			// after the shift, add new invalid tiles, and remove the old ones
			TileArea area = startArea.Move(x, y);
			if (dx > 0) {
				for (int32 y_ = area.minY; y_ <= area.maxY; y_++) {
					if (!isTileAvailable(WorldTile2(area.minX - 1, y_))) {
						invalidTileCount--;
					}
					if (!isTileAvailable(WorldTile2(area.maxX, y_))) {
						invalidTileCount++;
					}
				}
			}
			else if (dx < 0) {
				for (int32 y_ = area.minY; y_ <= area.maxY; y_++) {
					if (!isTileAvailable(WorldTile2(area.minX, y_))) {
						invalidTileCount++;
					}
					if (!isTileAvailable(WorldTile2(area.maxX + 1, y_))) {
						invalidTileCount--;
					}
				}
			}
			else if (dy > 0) {
				for (int32 x_ = area.minX; x_ <= area.maxX; x_++) {
					if (!isTileAvailable(WorldTile2(x_, area.minY - 1))) {
						invalidTileCount--;
					}
					if (!isTileAvailable(WorldTile2(x_, area.maxY))) {
						invalidTileCount++;
					}
				}
			}
			else if (dy < 0) {
				for (int32 x_ = area.minX; x_ <= area.maxX; x_++) {
					if (!isTileAvailable(WorldTile2(x_, area.minY))) {
						invalidTileCount++;
					}
					if (!isTileAvailable(WorldTile2(x_, area.maxY + 1))) {
						invalidTileCount--;
					}
				}
			}

			
		}
		//UE_DEBUG_BREAK();
		return WorldTile2::Invalid;
	}


	template<typename Func>
	static WorldRegion2 LoopSpiralOutward_Regions(WorldRegion2 originRegion, int32 regionDistance, Func shouldExitFunc)
	{
		int32 maxLookup = (1 + regionDistance * 2);
		maxLookup = maxLookup * maxLookup;
		
		// Spiral
		int x = 0;
		int y = 0;
		int dx = 0;
		int dy = -1;
		for (int i = 0; i < maxLookup; i++) 
		{
			WorldRegion2 region = originRegion;
			region.x += x;
			region.y += y;
			
			if (region.IsValid()) {
				if (shouldExitFunc(region)) {
					return region;
				}
			}
			if ((x == y) || ((x < 0) && (x == -y)) || ((x > 0) && (x == 1 - y))) {
				int temp = dx;
				dx = -dy;
				dy = temp;
			}
			x += dx;
			y += dy;
		}

		return WorldRegion2();	
	}

	
	// Shift display location so it is right a the center... useful for decal placement
	static void ShiftDisplayLocationToTrueCenter(FVector& displayLocation, TileArea area, Direction faceDirection)
	{
		FVector shift = FVector(0, 0, 0);
		if (area.sizeX() % 2 == 0) {
			shift += FVector(5, 0, 0);
		}
		if (area.sizeY() % 2 == 0) {
			shift += FVector(0, 5, 0);
		}
		FRotator rotator(0, RotationFromDirection(faceDirection), 0);

		displayLocation += rotator.RotateVector(shift);
	}

	static std::vector<WorldRegion2> Get4NearbyRegions(WorldTile2 tile)
	{
		WorldRegion2 overlapRegion = tile.region();
		std::vector<WorldRegion2> regions;
		regions.push_back(overlapRegion);
		LocalTile2 localTile = tile.localTile();

		int32_t shiftX = localTile.x > CoordinateConstants::TilesPerRegion / 2 ? 1 : -1;
		int32_t shiftY = localTile.y > CoordinateConstants::TilesPerRegion / 2 ? 1 : -1;\
		
		WorldRegion2 region = WorldRegion2(overlapRegion.x + shiftX, overlapRegion.y);
		if (region.IsValid()) {
			regions.push_back(region);
		}
		region = WorldRegion2(overlapRegion.x + shiftX, overlapRegion.y + shiftY);
		if (region.IsValid()) {
			regions.push_back(region);
		}
		region = WorldRegion2(overlapRegion.x, overlapRegion.y + shiftY);
		if (region.IsValid()) {
			regions.push_back(region);
		}
		
		return regions;
	}

	static void AddResourcePairToPairs(std::vector<ResourcePair>& pairVec, ResourcePair pair)
	{
		auto found = std::find_if(pairVec.begin(), pairVec.end(), [&](ResourcePair pairLocal) {
			return pairLocal.resourceEnum == pair.resourceEnum;
		});
		if (found != pairVec.end()) {
			found->count += pair.count;
		}
		else {
			pairVec.push_back(pair);
		}
	}
};
