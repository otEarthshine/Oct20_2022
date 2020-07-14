#pragma once

#include <cstdint>

struct LocAndPriority
{
	uint32_t locInt;
	int priority;
};

class PunBinaryHeap
{
public:
	PunBinaryHeap();

	void Clear();

	void Insert(uint32_t locationInt, int priority);

	uint32_t RemoveMin();

	int Count() const { return _heapCurrentSize; }
	bool isEmpty() const { return _heapCurrentSize == 0; }
	
	static int maxSize() { return _maxSize; }
	int heapCurrentSize() { return _heapCurrentSize; }
private:
	int MoveHoleDown(int holeIndex);

private:
	static const int _maxSize = 128 * 256 * 32 * 32;
	LocAndPriority _heap[_maxSize];

	int _heapCurrentSize;

	//void TrySwapWithChild(int index);
	void TrySwapWithParent(int index);

};
