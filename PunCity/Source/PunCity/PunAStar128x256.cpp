#include "PunAStar128x256.h"

#include "UnrealEngine.h"
#include "PunCity/GameRand.h"
#include "PunCity/PunSTLContainerOverride.h"

DECLARE_CYCLE_STAT(TEXT("PUN: PathAI"), STAT_PunPathAI, STATGROUP_Game);

using namespace std;

static const uint32_t bits5 = 0b11111;


uint32_t* PunAStar128x256::_isRoadGrid = nullptr;


PunBinaryHeap PunAStar128x256::_openList;
LeanAStarNode* PunAStar128x256::_nodes = nullptr;
uint32_t* PunAStar128x256::_calculatedNodes = nullptr;

vector<uint32_t> PunAStar128x256::_nodesToRenew;

PunAStar128x256::PunAStar128x256()
{
	PUN_LLM(PunSimLLMTag::PathAI);
	
	
	//_nodesToRenewLength = 0;

	check(_xDim == 128 * 32);
	check(_yDim == 256 * 32);
	check(_xyLength > 0);
	
	_isWalkableGrid = new uint32_t[_walkableGridSize];

#if PUN_LLM_ON
	PunScopeLLM::PunTrackAlloc(_isWalkableGrid, _walkableGridSize * sizeof(uint32_t));
#endif

	// Some data are static, and shared between multiple instances of PunAStar128x256
	if (_isRoadGrid == nullptr) 
	{
		_isRoadGrid = new uint32_t[_walkableGridSize];
		_nodes = new LeanAStarNode[_xyLength];
		_calculatedNodes = new uint32_t[_walkableGridSize];

		_nodesToRenew.reserve(10000);

		for (size_t i = 0; i < _walkableGridSize; i++) {
			_isRoadGrid[i] = 0;
			_calculatedNodes[i] = 0;
		}

		for (uint32_t i = 0; i < _xyLength; i++) {
			_nodes[i] = { i,0,0 };
		}

#if PUN_LLM_ON
		PunScopeLLM::PunTrackAlloc(_isRoadGrid, _walkableGridSize * sizeof(uint32_t));
		PunScopeLLM::PunTrackAlloc(_nodes, _xyLength * sizeof(LeanAStarNode));
		PunScopeLLM::PunTrackAlloc(_calculatedNodes, _walkableGridSize * sizeof(uint32_t));

		// Binary heap size
		PunScopeLLM::PunTrackAlloc(&_openList, _openList.maxSize() * sizeof(LocAndPriority));
#endif
	}

	//_nodes = new LeanAStarNode[_xyLength];
	//_calculatedNodes = new uint32_t[_walkableGridSize];
	//_nodesToRenew = new uint32_t[_xyLength];

	// LLM
	//PunScopeLLM::PunTrackAlloc(_nodesToRenew, _xyLength * sizeof(uint32_t));
	

	//for (size_t i = 0; i < _xyLength; i++)
	//{
	//	LeanAStarNode node = { i,0,0 };
	//	_nodes[i] = node;
	//	//_nodesToRenew[i] = 0;
	//}

	for (size_t i = 0; i < _walkableGridSize; i++) {
		_isWalkableGrid[i] = ~0;
		//_isRoadGrid[i] = 0;
		//_calculatedNodes[i] = 0;
	}

	badPathCount = 0;
	openListEmpty = 0;
	calculationTooLongCount = 0;
	iterationCount = 0;
	pathCount = 0;

	_isInitialized = true;
}

PunAStar128x256::~PunAStar128x256()
{
	delete[] _isWalkableGrid;

	// Some data are static. So we try to delete them here (when game ends)
	if (_isRoadGrid) {
		delete[] _isRoadGrid;
		delete[] _nodes;
		delete[] _calculatedNodes;

		_isRoadGrid = nullptr;
	}

	//delete[] _nodesToRenew;
}

void PunAStar128x256::CleanGrid()
{
	// Mark close all the calculated nodes
	//for (int i = 0; i < _nodesToRenewLength; i++)
	const size_t nodeCount = _nodesToRenew.size();
	for (int i = 0; i < nodeCount; i++)
	{
		uint32_t yWithXRegion = _nodesToRenew[i] >> 5;
		_calculatedNodes[yWithXRegion] = 0; // Just clear the whole chunk 8 bit chunk to 0
	}
	//_nodesToRenewLength = 0;

	_nodesToRenew.clear();
}


void PunAStar128x256::GetSuccessors(uint32_t locInt, uint32_t* successors, int& nonDiagArraySize, int& arraySize)
{
	/**
	 * Location Integer:
	 *	8 bits to represent regionIdX
	 *	y bits
	 *  5 bits to represent regionPosX(uint32_t)
	 */

	uint32_t xRegion = locInt >> shiftToXRegion; // 8 bits
	uint32_t y = (locInt >> 5) & bitsY; // y bits
	uint32_t xBits = locInt & bits5; // 5 bits

	uint32_t x = (xRegion << 5) | xBits; // 13 bits

	nonDiagArraySize = 0;

	uint32_t xRegionAndY; // 20 bits
	uint32_t newXBits;
	uint32_t xp, yp;

	yp = y + 1;
	if (yp < _yDim)
	{
		xRegionAndY = (xRegion << shiftAcrossBitsY) | yp;

		if ((_isWalkableGrid[xRegionAndY] >> xBits) & 1) {
			successors[nonDiagArraySize] = (xRegionAndY << 5) | xBits;
			++nonDiagArraySize;
		}
	}

	yp = y - 1;
	if (y > 0)
	{
		xRegionAndY = (xRegion << shiftAcrossBitsY) | yp;

		if ((_isWalkableGrid[xRegionAndY] >> xBits) & 1) {
			successors[nonDiagArraySize] = (xRegionAndY << 5) | xBits;
			++nonDiagArraySize;
		}
	}

	xp = x + 1;
	if (xp < _xDim)
	{
		xRegionAndY = ((xp >> 5) << shiftAcrossBitsY) | y;
		newXBits = xp & bits5;

		if ((_isWalkableGrid[xRegionAndY] >> newXBits) & 1) {
			successors[nonDiagArraySize] = (xRegionAndY << 5) | newXBits;
			++nonDiagArraySize;
		}
	}

	xp = x - 1;
	if (x > 0)
	{
		xRegionAndY = ((xp >> 5) << shiftAcrossBitsY) | y;
		newXBits = xp & bits5;

		if ((_isWalkableGrid[xRegionAndY] >> newXBits) & 1) {
			successors[nonDiagArraySize] = (xRegionAndY << 5) | newXBits;
			++nonDiagArraySize;
		}
	}

	arraySize = nonDiagArraySize;

	xp = x + 1;
	yp = y + 1;
	if (xp < _xDim && yp < _yDim)
	{
		xRegionAndY = ((xp >> 5) << shiftAcrossBitsY) | yp;
		newXBits = xp & bits5;

		if ((_isWalkableGrid[xRegionAndY] >> newXBits) & 1) {
			successors[arraySize] = (xRegionAndY << 5) | newXBits;
			++arraySize;
		}
	}

	xp = x - 1;
	yp = y + 1;
	if (x > 0 && yp < _yDim)
	{
		xRegionAndY = ((xp >> 5) << shiftAcrossBitsY) | yp;
		newXBits = xp & bits5;

		if ((_isWalkableGrid[xRegionAndY] >> newXBits) & 1) {
			successors[arraySize] = (xRegionAndY << 5) | newXBits;
			++arraySize;
		}
	}

	xp = x + 1;
	yp = y - 1;
	if (xp < _xDim && y > 0)
	{
		xRegionAndY = ((xp >> 5) << shiftAcrossBitsY) | yp;
		newXBits = xp & bits5;

		if ((_isWalkableGrid[xRegionAndY] >> newXBits) & 1) {
			successors[arraySize] = (xRegionAndY << 5) | newXBits;
			++arraySize;
		}
	}

	xp = x - 1;
	yp = y - 1;
	if (x > 0 && y > 0)
	{
		xRegionAndY = ((xp >> 5) << shiftAcrossBitsY) | yp;
		newXBits = xp & bits5;

		if ((_isWalkableGrid[xRegionAndY] >> newXBits) & 1) {
			successors[arraySize] = (xRegionAndY << 5) | newXBits;
			++arraySize;
		}
	}
}

void PunAStar128x256::PrintUInt32(uint32_t locInt)
{
	uint32_t xRegion = locInt >> shiftToXRegion; // 8 bits
	uint32_t y = (locInt >> 5) & bitsY; // y bits
	uint32_t xBits = locInt & bits5; // 5 bits
	uint32_t x = (xRegion << 5) | xBits; // 13 bits
	UE_LOG(LogTemp, Error, TEXT("PrintUInt32: %d, %d"), x, y);
}

bool PunAStar128x256::FindPath(int startX, int startY, int endX, int endY, std::vector<uint32_t>& path, bool isAccurate, bool isRoadable, int32 customCalculationCount)
{
	SCOPE_CYCLE_COUNTER(STAT_PunPathAI);

	if (0 > startX || startX >= (int)_xDim || 
		0 > startY || startY >= (int)_yDim ||
		0 > endX || endX >= (int)_xDim ||
		0 > endY || endY >= (int)_yDim) 
	{
		UE_LOG(LogTemp, Error, TEXT("BadFindPath: start: %d, %d"), startX, startY);
		UE_LOG(LogTemp, Error, TEXT("		      end: %d, %d"), endX, endY);
		UE_DEBUG_BREAK();
		badPathCount++;
		path.clear();
		return false;
	}

	// TODO: remove... If start or end is not walkable, use robust algorithm instead
	if (!(isWalkable(startX, startY) && isWalkable(endX, endY))) {
		return FindPathRobust(startX, startY, endX, endY, path);
	}

	uint32_t start = GetUIntId(startX, startY);
	uint32_t end = GetUIntId(endX, endY);

	_openList.Clear();
	check(_openList.heapCurrentSize() == 0)
	check(_openList.maxSize() > 0)
	_openList.Insert(start, 0);

	uint32_t successors[8];

	uint16_t calculationCount = 0;
	//uint16_t maxCalculationCount = isLongDistance ? 200000 : 30000;

	uint16_t maxCalculationCount = (customCalculationCount == -1) ? 30000 : customCalculationCount;

	while (!_openList.isEmpty() && calculationCount < maxCalculationCount)
	{
		uint32_t nodeLocInt = _openList.RemoveMin();
		check(nodeLocInt >= 0); 

		LeanAStarNode node = _nodes[nodeLocInt];

		if (nodeLocInt == end)
		{
			pathCount++;
			iterationCount += calculationCount;

			// Successfully found path
			ConstructPath(start, end, path);
			CleanGrid();
			return true;
		}

		int nonDiagArraySize, arraySize;
		GetSuccessors(nodeLocInt, successors, nonDiagArraySize, arraySize);

		for (int i = 0; i < arraySize; i++)
		{
			LeanAStarNode& loopNode = _nodes[successors[i]];

			uint32_t locInt = loopNode.locInt;
			uint32_t yWithXRegion = locInt >> 5; // 20 bits
			uint32_t xBits = locInt & bits5; // 5 bits


			// People's speed isn't effected by road if they are not carrying anything. This is done to cap ppl's speed and allow +50% road effectiveness
			int costUpToLoopNode;
			if (isRoadable) {
				bool isRoad = (_isRoadGrid[yWithXRegion] >> xBits) & 1;
				costUpToLoopNode = node.cost + (i < nonDiagArraySize ? (isRoad ? COST_ROAD : COST_ONE) : (isRoad ? COST_ROAD_SQRT2 : COST_SQRT2));
			}
			else {
				// Not Hauling.. Speed is the same in or out of road...
				costUpToLoopNode = node.cost + (i < nonDiagArraySize ? COST_ONE : COST_SQRT2);
				//bool isRoad = (_isRoadGrid[yWithXRegion] >> xBits) & 1;
				//costUpToLoopNode = node.cost + (i < nonDiagArraySize ? (isRoad ? COST_ROAD : COST_ONE) : (isRoad ? COST_ROAD_SQRT2 : COST_SQRT2));
			}


			bool calculated = (_calculatedNodes[yWithXRegion] >> xBits) & 1;

			if (!calculated || costUpToLoopNode < loopNode.cost)
			{
				uint32_t curY = yWithXRegion & bitsY; // y bits
				uint32_t curX = ((locInt >> shiftToXRegion) << 5) | xBits; // 13 bits

				// For road, GetHeuristic should have less cost, since there might be more road going towards destination.
				// This helps with making people go along road kink
				int heuristic = GetHeuristicFast(curX, curY, endX, endY);
				int priorityCost = costUpToLoopNode + (isAccurate ? (heuristic >> 2) : heuristic); // isAccurate means using less heuristics


				// Put this in the node grid
				loopNode.cameFromLocInt = nodeLocInt;
				loopNode.cost = costUpToLoopNode;

				_calculatedNodes[yWithXRegion] |= (1 << xBits);

				// Cache for clearing later
				//_nodesToRenew[_nodesToRenewLength] = locInt;
				//_nodesToRenewLength++;
				_nodesToRenew.push_back(locInt);
				
				_openList.Insert(locInt, priorityCost);
			}
		}

		calculationCount++;
	}

	pathCount++;
	iterationCount += calculationCount;

	// Cannot find path, either we are out of tile, or it took too long (exceed maxCalculationCount)
	int len = abs(startX - endX) + abs(startY - endY);
	int aveIter = iterationCount / pathCount;
	if (_openList.isEmpty()) {
		openListEmpty++;
		UE_LOG(LogTemp, Error, TEXT("openListEmpty(count:%d): from:%d,%d to:%d,%d len:%d isAccurate:%d aveIter:%d"), openListEmpty, startX, startY, endX, endY, len, isAccurate, aveIter);
	}
	else {
		calculationTooLongCount++;
		UE_LOG(LogTemp, Error, TEXT("calcTooLong(count:%d): from:%d,%d to:%d,%d len:%d isAccurate:%d  aveIter:%d"), calculationTooLongCount, startX, startY, endX, endY, len, isAccurate, aveIter);
	}

	CleanGrid();
	path.clear();

	// If isAccurate was previously turned on, turn it off and try again...
	if (isAccurate) {
		return FindPath(startX, startY, endX, endY, path, false, isRoadable);
	}

	return false;
}

void PunAStar128x256::ConstructPath(uint32_t start, uint32_t end, std::vector<uint32_t>& path)
{
	path.clear();

	uint32_t nodeId = end;

	int32 loop = 0;
	while (true)
	{
		path.push_back(nodeId);

		if (nodeId == start) {
			break;
		}

		nodeId = _nodes[nodeId].cameFromLocInt;

		loop++;
		if (loop > 100000) {
			UE_DEBUG_BREAK();
			break;
		}
	}
}

int PunAStar128x256::GetHeuristic(uint32_t start, uint32_t end) const
{
	uint32_t startY = (start >> 5) & bitsY; // y bits
	uint32_t startX = ((start >> shiftToXRegion) << 5) | (start & bits5); // 13 bits

	uint32_t endY = (end >> 5) & bitsY; // y bits
	uint32_t endX = ((end >> shiftToXRegion) << 5) | (end & bits5); // 13 bits

	int dx = abs((int)startX - (int)endX);
	int dz = abs((int)startY - (int)endY);

	// First take the Manhattan distance, then replaces part of it with diagonal
	return (dx + dz) * COST_ONE + COST_SQRT2_MINUS_2ONE * (dx < dz ? dx : dz);
}

/**
 * More expensive AStar that count obstacle as high cost
 */

void PunAStar128x256::GetSuccessorsRobust(uint32_t locInt, uint32_t* successors, int& nonDiagArraySize, int& arraySize)
{
	uint32_t xRegion = locInt >> shiftToXRegion; // 8 bits
	uint32_t y = (locInt >> 5) & bitsY; // y bits
	uint32_t xBits = locInt & bits5; // 5 bits

	uint32_t x = (xRegion << 5) | xBits; // 13 bits

	nonDiagArraySize = 0;

	uint32_t xRegionAndY; // 20 bits
	uint32_t newXBits;
	uint32_t xp, yp;

	yp = y + 1;
	if (yp < _yDim)
	{
		xRegionAndY = (xRegion << shiftAcrossBitsY) | yp;
		successors[nonDiagArraySize] = (xRegionAndY << 5) | xBits;
		++nonDiagArraySize;
	}

	yp = y - 1;
	if (y > 0)
	{
		xRegionAndY = (xRegion << shiftAcrossBitsY) | yp;
		successors[nonDiagArraySize] = (xRegionAndY << 5) | xBits;
		++nonDiagArraySize;
	}

	xp = x + 1;
	if (xp < _xDim)
	{
		xRegionAndY = ((xp >> 5) << shiftAcrossBitsY) | y;
		newXBits = xp & bits5;
		successors[nonDiagArraySize] = (xRegionAndY << 5) | newXBits;
		++nonDiagArraySize;
	}

	xp = x - 1;
	if (x > 0)
	{
		xRegionAndY = ((xp >> 5) << shiftAcrossBitsY) | y;
		newXBits = xp & bits5;
		successors[nonDiagArraySize] = (xRegionAndY << 5) | newXBits;
		++nonDiagArraySize;
	}

	arraySize = nonDiagArraySize;

	xp = x + 1;
	yp = y + 1;
	if (xp < _xDim && yp < _yDim)
	{
		xRegionAndY = ((xp >> 5) << shiftAcrossBitsY) | yp;
		newXBits = xp & bits5;
		successors[arraySize] = (xRegionAndY << 5) | newXBits;
		++arraySize;
	}

	xp = x - 1;
	yp = y + 1;
	if (x > 0 && yp < _yDim)
	{
		xRegionAndY = ((xp >> 5) << shiftAcrossBitsY) | yp;
		newXBits = xp & bits5;
		successors[arraySize] = (xRegionAndY << 5) | newXBits;
		++arraySize;
	}

	xp = x + 1;
	yp = y - 1;
	if (xp < _xDim && y > 0)
	{
		xRegionAndY = ((xp >> 5) << shiftAcrossBitsY) | yp;
		newXBits = xp & bits5;
		successors[arraySize] = (xRegionAndY << 5) | newXBits;
		++arraySize;
	}

	xp = x - 1;
	yp = y - 1;
	if (x > 0 && y > 0)
	{
		xRegionAndY = ((xp >> 5) << shiftAcrossBitsY) | yp;
		newXBits = xp & bits5;
		successors[arraySize] = (xRegionAndY << 5) | newXBits;
		++arraySize;
	}
}

// Robust algorithm can ignores terrain
bool PunAStar128x256::FindPathRobust(int startX, int startY, int endX, int endY, std::vector<uint32_t>& path)
{
	uint32_t start = GetUIntId(startX, startY);
	uint32_t end = GetUIntId(endX, endY);

	_openList.Clear();
	_openList.Insert(start, 0);

	uint32_t successors[8];

	int32 calculationCount = 0;
	int32 maxCalculationCount = 200000;
	
	while (!_openList.isEmpty())
	{
		uint32_t nodeLocInt = _openList.RemoveMin();
		check(nodeLocInt >= 0);

		LeanAStarNode node = _nodes[nodeLocInt];

		if (nodeLocInt == end)
		{
			ConstructPath(start, end, path);
			CleanGrid();

			return true;
		}

		int nonDiagArraySize, arraySize;
		GetSuccessorsRobust(nodeLocInt, successors, nonDiagArraySize, arraySize);

		for (int i = 0; i < arraySize; i++)
		{
			uint32_t successor = successors[i];
			LeanAStarNode& loopNode = _nodes[successors[i]];

			uint32_t locInt = loopNode.locInt;
			uint32_t yWithXRegion = locInt >> 5; // 20 bits
			uint32_t xBits = locInt & bits5; // 5 bits

			//! THIS IS FindPathRobust!!!
			bool isRoad = (_isRoadGrid[yWithXRegion] >> xBits) & 1;
			int costUpToLoopNode = node.cost + (i < nonDiagArraySize ? (isRoad ? COST_ROAD : COST_ONE) : (isRoad ? COST_ROAD_SQRT2 : COST_SQRT2));

			
			//if (!((_isWalkableGrid[yWithXRegion] >> xBits) & 1)) {
			//	costUpToLoopNode += COST_OBSTACLE;
			//}

			bool calculated = (_calculatedNodes[yWithXRegion] >> xBits) & 1;

			if (!calculated || costUpToLoopNode < loopNode.cost)
			{
				// For road, GetHeuristic should have less cost, since there might be more road going towards destination.
				int priorityCost = costUpToLoopNode + isRoad ? (GetHeuristic(locInt, end) >> 2) : (GetHeuristic(locInt, end) >> 1); // >> 1 for half heuristics

				// Put this in the node grid
				loopNode.cameFromLocInt = nodeLocInt;
				loopNode.cost = costUpToLoopNode;

				_calculatedNodes[yWithXRegion] |= (1 << xBits);

				// Cache for clearing later
				//_nodesToRenew[_nodesToRenewLength] = locInt;
				//_nodesToRenewLength++;
				_nodesToRenew.push_back(locInt);
				
				_openList.Insert(locInt, priorityCost);
			}
		}

		calculationCount++;
		if (calculationCount > maxCalculationCount) {
			break;
		}
	}

	CleanGrid();
	path.clear();
	return false;
}