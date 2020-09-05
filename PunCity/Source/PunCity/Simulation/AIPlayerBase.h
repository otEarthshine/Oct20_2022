// Pun Dumnernchanvanit's

#pragma once

#include "IGameSimulationCore.h"
#include "PunCity/AlgorithmUtils.h"

enum class AIRegionProposedPurposeEnum : uint8
{
	None,
	Tree,
	Fertility,
	Ranch,
};

static const std::vector<std::string> AIRegionProposedPurposeName
{
	"None",
	"Tree",
	"Fertility",
	"Ranch",
};

enum class AIRegionPurposeEnum : uint8
{
	None,
	City,
	FruitGather,
	Forester,
	Farm,
	Ranch,
};

static const std::vector<std::string> AIRegionPurposeName
{
	"None",
	"City",
	"FruitGather",
	"Forester",
	"Farm",
	"Ranch",
};

/*
 * Represent a building block that can be placed onto a region..
 */
class AICityBlock
{
public:
	WorldTile2 minTile() { return _minTile; }
	WorldTile2 size() { return _size; }

	bool IsValid() {
		return topTileSizeX > 0 || bottomTileSizeX > 0;
	}

	void SetMinTile(WorldTile2 minTile) {
		_minTile = minTile;
	}

	TileArea area() { return TileArea(_minTile, _size); }

	int32 midRoadTileX() { return _minTile.x + bottomTileSizeX; }

	void CalculateSize() {
		topTileSizeX = 0;
		int32 topTileSizeY = 0;
		for (CardEnum buildingEnum : topBuildingEnums) {
			WorldTile2 size = GetBuildingInfo(buildingEnum).size;
			topTileSizeX = std::max(topTileSizeX, static_cast<int32_t>(size.x));
			topTileSizeY += size.y;
		}
		bottomTileSizeX = 0;
		int32 bottomTileSizeY = 0;
		for (CardEnum buildingEnum : bottomBuildingEnums) {
			WorldTile2 size = GetBuildingInfo(buildingEnum).size;
			bottomTileSizeX = std::max(bottomTileSizeX, static_cast<int32_t>(size.x));
			bottomTileSizeY += size.y;
		}

		int32 tileSizeX = topTileSizeX + bottomTileSizeX;
		int32 tileSizeY = std::max(topTileSizeY, bottomTileSizeY);
		const int32_t roadShift = 2;

		_size = WorldTile2(tileSizeX + roadShift, tileSizeY + roadShift);
	}

	// Forest block has road in the middle
	bool CanPlaceForestBlock(WorldTile2 minTile, int32 playerId, IGameSimulationCore* simulation)
	{
		// Check block area for invalid tile
		TileArea area(minTile, _size);
		bool hasInvalid = area.ExecuteOnAreaWithExit_WorldTile2([&](WorldTile2 curTile) {
			if (curTile.isValid() && simulation->IsBuildable(curTile, playerId)) {
				return false;
			}
			return true;
		});

		return !hasInvalid;
	}

	void TryFindArea(WorldTile2 provinceCenter, int32 playerId, IGameSimulationCore* simulation, int32 maxLookup = 500)
	{
		SetMinTile(
			AlgorithmUtils::FindNearbyAvailableTile(provinceCenter, [&](const WorldTile2& tile) {
				return CanPlaceForestBlock(tile, playerId, simulation); // TODO: CanPlaceCityBlock should be used when needed to solve x2 road shit
			}, maxLookup)
		);
	}
	bool HasArea() {
		return minTile().isValid();
	}


	// City block has road around
	bool CanPlaceCityBlock(WorldTile2 minTile, int32_t playerId, IGameSimulationCore* simulation)
	{
		// outer rim is checked for FrontBuildable instead
		TileArea area(minTile, _size);
		WorldTile2 maxTile = area.max();

		auto isRimBuildable = [&](TileArea rimArea) {
			bool hasInvalid = rimArea.ExecuteOnAreaWithExit_WorldTile2([&](WorldTile2 curTile) {
				return !(curTile.isValid() && simulation->IsFrontBuildable(curTile));
			});
			return !hasInvalid;
		};

		if (!isRimBuildable(TileArea(WorldTile2(minTile.x, minTile.y), WorldTile2(_size.x, 1)))) return false;
		if (!isRimBuildable(TileArea(WorldTile2(minTile.x, maxTile.y), WorldTile2(_size.x, 1)))) return false;
		if (!isRimBuildable(TileArea(WorldTile2(minTile.x, minTile.y), WorldTile2(1, _size.y)))) return false;
		if (!isRimBuildable(TileArea(WorldTile2(maxTile.x, minTile.y), WorldTile2(1, _size.y)))) return false;

		// Trim the area
		area.minX++;
		area.maxX--;
		area.minY++;
		area.maxY--;

		bool hasInvalid = area.ExecuteOnAreaWithExit_WorldTile2([&](WorldTile2 curTile) {
			return !(curTile.isValid() && simulation->IsBuildable(curTile, playerId));
		});
		return !hasInvalid;
	}

	void operator>>(FArchive& Ar)
	{
		SerializeVecValue(Ar, topBuildingEnums);
		SerializeVecValue(Ar, bottomBuildingEnums);

		SerializeVecValue(Ar, topBuildingIds);
		SerializeVecValue(Ar, bottomBuildingIds);

		Ar << topTileSizeX;
		Ar << bottomTileSizeX;

		_minTile >> Ar;
		_size >> Ar;
	}

public:
	// City block is composed of two opposite facing sides
	// arranged from left to right
	std::vector<CardEnum> topBuildingEnums;
	std::vector<CardEnum> bottomBuildingEnums;

	std::vector<int32> topBuildingIds;
	std::vector<int32> bottomBuildingIds;

	int32 topTileSizeX = 0;
	int32 bottomTileSizeX = 0;

private:
	WorldTile2 _minTile = WorldTile2::Invalid;
	WorldTile2 _size = WorldTile2::Invalid;
};