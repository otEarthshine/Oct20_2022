#include "PunBinaryHeap.h"
#include <iostream>

#include "PunCity/PunSTLContainerOverride.h"

using namespace std;

PunBinaryHeap::PunBinaryHeap()
{
	_heapCurrentSize = 0;

	for (int i = 0; i < _maxSize; i++)
	{
		LocAndPriority locAndPriority = { 0 , -1 };
		_heap[i] = locAndPriority;
	}
}

void PunBinaryHeap::Clear()
{
	_heapCurrentSize = 0;
}

void PunBinaryHeap::TrySwapWithParent(int index)
{
	if (index == 1) {
		return;
	}

	int parentIndex = index / 2;

	//if index cost is lower and should swap with parent, do the swap
	//then try again to see if this node can go further up the tree
	if (_heap[parentIndex].priority > _heap[index].priority)
	{
		LocAndPriority temp = _heap[parentIndex];
		_heap[parentIndex] = _heap[index];
		_heap[index] = temp;

		TrySwapWithParent(parentIndex);
	}
}

void PunBinaryHeap::Insert(uint32_t locationInt, int priority)
{
	_heapCurrentSize++;
	_heap[_heapCurrentSize].locInt = locationInt;
	_heap[_heapCurrentSize].priority = priority;

	// TODO: Would checking length be too much burden?

	TrySwapWithParent(_heapCurrentSize);
}

uint32_t PunBinaryHeap::RemoveMin()
{
	if (_heapCurrentSize == 0) {
		return -1;
	}

	// Get root value as return value
	uint32_t returnLoc = _heap[1].locInt;

	// Take off and cache the last node
	LocAndPriority lastLeafNode = _heap[_heapCurrentSize];
	_heapCurrentSize--;

	// Move hole which is at root down
	int holeLeafIndex = MoveHoleDown(1);

	// Swap the cached node to the hole
	_heap[holeLeafIndex] = lastLeafNode;

	// Now swap with parent until it goes up to proper place
	TrySwapWithParent(holeLeafIndex);

	return returnLoc;
}

// Return hole index where it end up at leaf
int PunBinaryHeap::MoveHoleDown(int holeIndex)
{
	int leftChildIndex = 2 * holeIndex;
	int rightChildIndex = 2 * holeIndex + 1;

	if (leftChildIndex > _heapCurrentSize) {
		return holeIndex; // Index is a leaf
	}

	// Find the index of minimum value out of parent/2 children
	int minIndex = leftChildIndex;

	if (rightChildIndex <= _heapCurrentSize
		&& _heap[minIndex].priority > _heap[rightChildIndex].priority)
	{
		minIndex = rightChildIndex;
	}

	// Need to swap
	LocAndPriority temp = _heap[holeIndex];
	_heap[holeIndex] = _heap[minIndex];
	_heap[minIndex] = temp;

	// Recursive swap
	return MoveHoleDown(minIndex);
}
