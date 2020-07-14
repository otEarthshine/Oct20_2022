#pragma once

#include "PunCity/PunSTLContainerOverride.h"
#include <memory>
#include "PunBinaryHeap.h"

#include "CoreMinimal.h"

// Cost without road
const int COST_ONE = 100;
const int COST_SQRT2 = 142;

// Cost with road, approximate at 50% faster speed on road (regardless of type)
const int COST_ROAD = 67;
const int COST_ROAD_SQRT2 = 94;

const int COST_SQRT2_MINUS_2ONE = COST_SQRT2 - 2 * COST_ONE;
const int COST_OBSTACLE = 10000;

struct LeanAStarNode
{
	uint32_t locInt;
	uint32_t cameFromLocInt;
	int32 cost;
};

/**
 * Arrangement of bits in locationUInt32:
 * for 256x128
 * 8 bits regionIdX
 * 12 bits bitsY
 * 5 bits xBits
 */
class PunAStar128x256
{
public:
	// !!!These values only works y-region = 128
	//static const uint32_t bitsY = 0b111111111111;
	//static const uint32_t shiftAcrossBitsY = 12;
	//static const uint32_t shiftToXRegion = 17;

	static const uint32_t bitsY = 0b1111111111111;
	static const uint32_t shiftAcrossBitsY = 13;
	static const uint32_t shiftToXRegion = 18;

	PunAStar128x256();
	~PunAStar128x256();

	bool FindPath(int startX, int startY, int endX, int endY, std::vector<uint32_t>& path, bool isAccurate, bool isRoadable, bool isLongDistance = false);
	bool FindPathRobust(int startX, int startY, int endX, int endY, std::vector<uint32_t>& path);

	void CleanGrid();

	void SetWalkable(int x, int y, bool isWalkable)
	{ 
		uint32_t locInt = GetUIntId(x, y);
		uint32_t xBits = locInt & 0b11111; // 5 bits
		uint32_t xRegionAndY = locInt >> 5;

		if (isWalkable) {
			_isWalkableGrid[xRegionAndY] |= (1 << xBits);
		}
		else {
			_isWalkableGrid[xRegionAndY] &= ~(1 << xBits);
		}
	}

	bool isWalkable(int x, int y) 
	{ 
		uint32_t locInt = GetUIntId(x, y);
		uint32_t xBits = locInt & 0b11111; // 5 bits
		uint32_t xRegionAndY = locInt >> 5;
		//check(xRegionAndY >= 0);
		//check(xRegionAndY < _walkableGridSize);
		return (_isWalkableGrid[xRegionAndY] >> xBits) & 1;
	}

	static uint32_t GetUIntId(int x, int y) 
	{
		uint32_t xBits = x & 0b11111; // 5 bits
		uint32_t xRegion = (x >> 5); // bitsXRegion

		return (xRegion << shiftToXRegion) | (y << 5) | xBits;
	}

	int xDim() const { return _xDim; }
	int yDim() const { return _yDim; }

	//! Road
	static void SetRoad(int x, int y, bool isRoad) { SetTile(x, y, isRoad, _isRoadGrid); }
	static bool isRoad(int x, int y) { return isTile(x, y, _isRoadGrid); }

	static void SetTile(int x, int y, bool isXX, uint32_t* grid)
	{
		uint32_t locInt = GetUIntId(x, y);
		uint32_t xBits = locInt & 0b11111; // 5 bits
		uint32_t xRegionAndY = locInt >> 5;

		if (isXX) {
			grid[xRegionAndY] |= (1 << xBits);
		}
		else {
			grid[xRegionAndY] &= ~(1 << xBits);
		}
	}

	static bool isTile(int x, int y, uint32_t* grid)
	{
		uint32_t locInt = GetUIntId(x, y);
		uint32_t xBits = locInt & 0b11111; // 5 bits
		uint32_t xRegionAndY = locInt >> 5;
		return (grid[xRegionAndY] >> xBits) & 1;
	}

	static bool isTile(uint32_t locInt, uint32_t* grid)
	{
		uint32_t xBits = locInt & 0b11111; // 5 bits
		uint32_t xRegionAndY = locInt >> 5;
		return (grid[xRegionAndY] >> xBits) & 1;
	}

private:
	void GetSuccessors(uint32_t locInt, uint32_t* successors, int& nonDiagArraySize, int& arraySize);
	int GetHeuristic(uint32_t start, uint32_t target) const;
	void ConstructPath(uint32_t start, uint32_t end, std::vector<uint32_t>& path);

	void GetSuccessorsRobust(uint32_t locInt, uint32_t* successors, int& nonDiagArraySize, int& arraySize);

	void PrintUInt32(uint32_t locInt);

	inline int GetHeuristicFast(int startX, int startY, int endX, int endY) const 
	{
		int dx = abs(startX - endX);
		int dz = abs(startY - endY);

		// First take the Manhattan distance, then replaces part of it with diagonal
		return (dx + dz) * COST_ONE + COST_SQRT2_MINUS_2ONE * (dx < dz ? dx : dz);
	}

public:
	int badPathCount;
	int openListEmpty;
	int calculationTooLongCount;
	int iterationCount;
	int pathCount;

	// DEBUG:
	int walkableGridSize() { return _walkableGridSize; }
	bool isInitialized() { return _isInitialized; }

	void Serialize(FArchive &Ar)
	{
		Ar << badPathCount;
		Ar << openListEmpty;
		Ar << calculationTooLongCount;
		Ar << iterationCount;
		Ar << pathCount;

		Ar << _isInitialized;
		//Ar << _nodesToRenewLength;

		Ar.Serialize(_isWalkableGrid, _walkableGridSize * 4);
		Ar.Serialize(_isRoadGrid, _walkableGridSize * 4);
	}
	
private:
	const uint32_t _xDim = 128 * 32;
	const uint32_t _yDim = 256 * 32;
	const uint32_t _xyLength = _xDim * _yDim;
	const uint32_t _walkableGridSize = _xyLength / 32; // Size in 32bit unit
	
	bool _isInitialized = false;

	/**
	 * 32 bits of uint32_t represents 32 width of region in x-direction
	 * Memory arranged going in y direction first to take advantage of cache friendliness 
	 * Memory will then be read in 32x16 tiles chunk (64 bytes cache-line)
	 */
	uint32_t* _isWalkableGrid;

	static uint32_t* _isRoadGrid;
	

	//int _target;
	//int _nodesExpanded;
	//int _nodesVisited;
	

	static PunBinaryHeap _openList;

	static LeanAStarNode* _nodes;

	static uint32_t* _calculatedNodes;
	//uint32_t* _nodesToRenew; // making these array const length doesn't really improve the speed

	static std::vector<uint32_t> _nodesToRenew;
	
	//int _nodesToRenewLength;
};
