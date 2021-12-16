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
	CashCrop,
};

static const std::vector<std::string> AIRegionProposedPurposeName
{
	"None",
	"Tree",
	"Fertility",
	"Ranch",
	"CashCrop",
};

enum class AIRegionPurposeEnum : uint8
{
	None,
	City,
	FruitGather,
	Forester,
	Farm,
	Ranch,
	CashCrop,
};

static const std::vector<std::string> AIRegionPurposeName
{
	"None",
	"City",
	"FruitGather",
	"Forester",
	"Farm",
	"Ranch",
	"CashCrop",
};

/*
 * Represent a building block that can be placed onto a region..
 */
class AICityBlock
{
public:

	//static bool TryPlaceBlockHelper(WorldTile2 centerTile, int32 aiPlayerId, IGameSimulationCore* simulation);

	static AICityBlock MakeBlock(FactionEnum factionEnum, std::vector<CardEnum> topBuildingEnumsIn, std::vector<CardEnum> bottomBuildingEnumsIn = {})
	{
		AICityBlock block;
		block.topBuildingEnums = topBuildingEnumsIn;
		block.bottomBuildingEnums = bottomBuildingEnumsIn;
		block.factionEnum = factionEnum;
		block.CalculateSize();
		return block;
	}
	
	
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

	void CalculateSize()
	{
		// Calculate the total size required to place this cluster

		auto getSize = [&](CardEnum buildingEnum)
		{
			if (buildingEnum == CardEnum::StorageYard) {
				return WorldTile2(4, 4);
			}
			return GetBuildingInfo(buildingEnum).GetSize(factionEnum);
		};
		
		topTileSizeX = 0;
		int32 topTileSizeY = 0;
		for (CardEnum buildingEnum : topBuildingEnums) {
			WorldTile2 size = getSize(buildingEnum);
			topTileSizeX = std::max(topTileSizeX, static_cast<int32_t>(size.x));
			topTileSizeY += size.y;
		}
		bottomTileSizeX = 0;
		int32 bottomTileSizeY = 0;
		for (CardEnum buildingEnum : bottomBuildingEnums) {
			WorldTile2 size = getSize(buildingEnum);
			bottomTileSizeX = std::max(bottomTileSizeX, static_cast<int32_t>(size.x));
			bottomTileSizeY += size.y;
		}

		int32 tileSizeX = topTileSizeX + bottomTileSizeX;
		int32 tileSizeY = std::max(topTileSizeY, bottomTileSizeY);
		const int32 roadShift = 2;

		_size = WorldTile2(tileSizeX + roadShift, tileSizeY + roadShift);
	}

	// *** Forest block has road in the middle
	bool CanPlaceForestBlock(WorldTile2 minTile, int32 playerId, IGameSimulationCore* simulation)
	{
		// Check block area for invalid tile
		TileArea area(minTile, _size);
		if (!area.isValid()) {
			return false;
		}
		
		bool hasInvalid = area.ExecuteOnAreaWithExit_WorldTile2([&](WorldTile2 curTile) {
			return !simulation->IsBuildableForPlayer(curTile, playerId);
		});

		return !hasInvalid;
	}

	// From some center tile
	// - Look spirally around to find a suitable _minTile
	void TryPlaceForestBlock(WorldTile2 center, int32 playerId, IGameSimulationCore* simulation, int32 maxLookup = 500)
	{
		SetMinTile(
			AlgorithmUtils::FindNearbyAvailableTile(center, [&](const WorldTile2& tile) {
				return CanPlaceForestBlock(tile, playerId, simulation);
			}, maxLookup)
		);
	}
	void TryPlaceCityBlockSpiral(WorldTile2 center, int32 playerId, IGameSimulationCore* simulation, int32 maxLookup = 500)
	{
		SetMinTile(
			AlgorithmUtils::FindNearbyAvailableTile(center, [&](const WorldTile2& tile) {
				return CanPlaceCityBlock(tile, playerId, simulation);
			}, maxLookup)
		);
	}


	// City Block has road around it.
	// *** City block has road around
	bool CanPlaceCityBlock(WorldTile2 minTile, int32 playerId, IGameSimulationCore* simulation)
	{
		// outer rim is checked for FrontBuildable instead
		TileArea area(minTile, _size);
		if (!area.isValid()) {
			return false;
		}

		bool hasInvalid = area.ExecuteOnBorderWithExit_WorldTile2([&](WorldTile2 curTile) {
			if (simulation->IsFrontBuildable(curTile)) {
				//PUN_LOG("CanPlaceCityBlock[%d] IsFrontBuildable %s", playerId, *curTile.To_FString());
				return false;
			}
			//PUN_LOG("CanPlaceCityBlock[%d] Not IsFrontBuildable (Break) %s", playerId, *curTile.To_FString());
			return true;
		});
		
		if (hasInvalid) {
			return false;
		}

		// Trim the area
		area.minX++;
		area.maxX--;
		area.minY++;
		area.maxY--;

		hasInvalid = area.ExecuteOnAreaWithExit_WorldTile2([&](WorldTile2 curTile) {
			return !simulation->IsBuildableForPlayer(curTile, playerId);
		});
		
		return !hasInvalid;
	}
	void TryPlaceCityBlockAroundArea(TileArea area, int32 playerId, IGameSimulationCore* simulation)
	{
		TileArea largerArea(area.min() - _size, area.size() + _size);

		// TODO: Why need this to fix weird block shift down???
		largerArea.minX++;

		WorldTile2 resultTile = WorldTile2::Invalid;
		largerArea.ExecuteOnBorderWithExit_WorldTile2([&](const WorldTile2& tile) {
			if (CanPlaceCityBlock(tile, playerId, simulation)) {
				resultTile = tile;
				return true;
			}
			return false;
		});

		SetMinTile(resultTile);
	}

	

	// TODO: New better way ? ...
	//  Not really?
	void TryFindAreaFast(WorldTile2 startTile, int32 playerId, IGameSimulationCore* simulation, int32 maxLookup = 500)
	{
		WorldTile2 startMinTile = startTile - WorldTile2(-_size.x / 2, -_size.y / 2);
		TileArea startArea(startMinTile, _size);
		if (!startArea.isValid()) {
			return;
		}
		
		WorldTile2 validMinTile = AlgorithmUtils::FindNearbyAvailableArea(startArea, [&](const WorldTile2& tile) {
			return simulation->IsBuildableForPlayer(tile, playerId);
		}, maxLookup);

		
		SetMinTile(validMinTile);
	}

	

	// HasArea if TryFindArea() was successul with valid _minTile
	bool HasArea() {
		return minTile().isValid();
	}

	bool PlacementSucceed() {
		return HasArea();
	}

	bool HasBuildingEnum(CardEnum buildingEnum)
	{
		for (CardEnum topBuildingEnum : topBuildingEnums) {
			if (buildingEnum == topBuildingEnum) {
				return true;
			}
		}
		for (CardEnum bottomBuildingEnum : bottomBuildingEnums) {
			if (buildingEnum == bottomBuildingEnum) {
				return true;
			}
		}
		return false;
	}

	void operator>>(FArchive& Ar)
	{
		SerializeVecValue(Ar, topBuildingEnums);
		SerializeVecValue(Ar, bottomBuildingEnums);
		Ar << factionEnum;

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
	FactionEnum factionEnum = FactionEnum::None;

	int32 topTileSizeX = 0;
	int32 bottomTileSizeX = 0;

private:
	WorldTile2 _minTile = WorldTile2::Invalid;
	WorldTile2 _size = WorldTile2::Invalid;
};