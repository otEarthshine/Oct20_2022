// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"


// Deterministic float implemented using int to ensure 100% determinism
// This cannot be used for large number!!!
typedef int32_t FloatDet;
const FloatDet FloatDetShifts = 14; // 2096, only 3 bits limit per float??

#define IntToFD(intX) ((intX) << FloatDetShifts)
#define FDToInt(floatDetX) ((floatDetX) >> FloatDetShifts)
#define FDToFloat(floatDetX) (double(floatDetX) / FDOne)
#define FloatToFD(floatX) (FloatDet((floatX) * FDOne))

// 0.65 is FD0_XX(65)
#define FD0_X(x) (((x) << FloatDetShifts) / 10)
#define FD0_XX(x) (((x) << FloatDetShifts) / 100)
#define FD0_XXX(x) (((x) << FloatDetShifts) / 1000)

#define FDMul(floatDetX, floatDetY) ((FloatDet(floatDetX) * (floatDetY)) >> (FloatDetShifts))
#define FDDiv(floatDetX, floatDetY) ((FloatDet(floatDetX) << FloatDetShifts) / (floatDetY)) // This shit can easily overflow , careful..
#define FDHalf(floatDetX) (floatDetX >> 1)
#define FDQuarter(floatDetX) (floatDetX >> 2)
#define FDEight(floatDetX) (floatDetX >> 3)
#define FDSqrt(floatDetX) (FloatToFD(sqrtf(FDToFloat(floatDetX))))

// Note: FDSafeMul(floatDetX, uint16) is also fine
#define FDSafeMul(floatDetX, floatDetY) (FloatDet((int64_t(floatDetX) * (floatDetY)) >> (FloatDetShifts)))

// When calculation length, we don't need to shift since the end point's FloatDetShifts cancels out
#define FDLength(floatDetX, floatDetY) (FloatToFD(sqrt((floatDetX) * (floatDetX) + (floatDetY) * (floatDetY))))

#define FDMin(floatDetX, floatDetY) ((floatDetX) < (floatDetY) ? (floatDetX) : (floatDetY))
#define FDMax(floatDetX, floatDetY) ((floatDetX) > (floatDetY) ? (floatDetX) : (floatDetY))

const FloatDet FDOne = IntToFD(1);
const FloatDet FDHalf = FDOne >> 1;

class GameMapConstants
{
public:
	static int RegionsPerWorldX;
	static int RegionsPerWorldY;
	static int TotalRegions;
	static int TilesPerWorldX;
	static int TilesPerWorldY;
	static int TilesPerWorld;

	static int Tiles4x4PerWorldX;
	static int Tiles4x4PerWorldY;

	// The outer 2 borders are never messed with to make it easier for FindSpot/Flooding algorithm (so they require no out of bound check)
	static int32 RegionsPerWorldX_Inner;
	static int32 RegionsPerWorldY_Inner;
	static const int32 RegionsOuterBorderSize = 2;
	static const int32 MinInnerRegionX = RegionsOuterBorderSize;
	static const int32 MinInnerRegionY = RegionsOuterBorderSize;
	static int32 MaxInnerRegionX; // Region at the max of inner part.. This is so we can loop from RegionsOuterBorderSize to MaxInnerRegionX
	static int32 MaxInnerRegionY;

	static void SetRegionsPerWorld(int regionPerWorldX, int regionPerWorldY);
};

class CoordinateConstants
{
public:
	const static int TilesPerRegion = 32;
	const static int TileIdsPerRegion = TilesPerRegion * TilesPerRegion;

	//! Atom width is the deterministic width that can no longer be splitted
	const static int AtomsPerTile = 100000;
	const static int HalfAtomsPerTile = AtomsPerTile / 2;
	const static int AtomsPerRegion = TilesPerRegion * AtomsPerTile;

	static float AtomPerDisplayUnit; // 100,000 atom per tile, 10,000 atoms per UE cm
	static float DisplayUnitPerRegion;
	static float DisplayUnitPerTile;
	
	static float AboveFlatDisplayUnitHeight;
	static float BelowWaterDisplayUnitHeight;
	static float RegionCenterDisplayUnits; // 0.5 is since we need the exact center

	const static int32 MapOriginShift = -500;
	const static int32 MapToWorldMultiple = 1;

	static float AtomPerMapDisplayUnit; // 100,000 atom per tile, 10,000 atoms per UE cm
};