// Fill out your copyright notice in the Description page of Project Settings.

#include "GameSimulationConstants.h"

int GameMapConstants::RegionsPerWorldX = 0;
int GameMapConstants::RegionsPerWorldY = 0;
int GameMapConstants::TotalRegions = 0;
int GameMapConstants::TilesPerWorldX = 0;
int GameMapConstants::TilesPerWorldY = 0;
int GameMapConstants::TilesPerWorld = 0;
int GameMapConstants::Tiles2x2PerWorldX = 0;
int GameMapConstants::Tiles2x2PerWorldY = 0;
int GameMapConstants::Tiles4x4PerWorldX = 0;
int GameMapConstants::Tiles4x4PerWorldY = 0;

float CoordinateConstants::AtomPerDisplayUnit = 10000.0f; // 100,000 atom per tile, 10,000 atoms per UE cm
float CoordinateConstants::DisplayUnitPerRegion = CoordinateConstants::AtomsPerRegion / CoordinateConstants::AtomPerDisplayUnit; //TODO: check?? 0.5?
float CoordinateConstants::DisplayUnitPerTile = CoordinateConstants::AtomsPerTile / CoordinateConstants::AtomPerDisplayUnit;

float CoordinateConstants::AboveFlatDisplayUnitHeight = 350.0f; 
float CoordinateConstants::BelowWaterDisplayUnitHeight = 200.0f;
float CoordinateConstants::RegionCenterDisplayUnits = DisplayUnitPerRegion / 2.0f - DisplayUnitPerTile * 0.5f; // 0.5 is to shift it back to vertCenter... tile != vert

float CoordinateConstants::AtomPerMapDisplayUnit = CoordinateConstants::AtomPerDisplayUnit * MapToWorldMultiple;

int GameMapConstants::RegionsPerWorldX_Inner = 0;
int GameMapConstants::RegionsPerWorldY_Inner = 0;
int32 GameMapConstants::MaxInnerRegionX = 0;
int32 GameMapConstants::MaxInnerRegionY = 0;

int32 GameMapConstants::TundraTemperatureStart10000 = 6000;
int32 GameMapConstants::BorealTemperatureStart10000 = 4000;
int32 GameMapConstants::ForestTemperatureStart10000 = 700;

void GameMapConstants::SetRegionsPerWorld(int regionPerWorldX, int regionPerWorldY)
{
	RegionsPerWorldX = regionPerWorldX;
	RegionsPerWorldY = regionPerWorldY;
	TotalRegions = RegionsPerWorldX * RegionsPerWorldY;

	TilesPerWorldX = RegionsPerWorldX * CoordinateConstants::TilesPerRegion;
	TilesPerWorldY = RegionsPerWorldY * CoordinateConstants::TilesPerRegion;
	TilesPerWorld = TilesPerWorldX * TilesPerWorldY;

	Tiles2x2PerWorldX = TilesPerWorldX / 2;
	Tiles2x2PerWorldY = TilesPerWorldY / 2;
	Tiles4x4PerWorldX = TilesPerWorldX / 4;
	Tiles4x4PerWorldY = TilesPerWorldY / 4;

	RegionsPerWorldX_Inner = RegionsPerWorldX - 2 * RegionsOuterBorderSize;
	RegionsPerWorldY_Inner = RegionsPerWorldY - 2 * RegionsOuterBorderSize;
	MaxInnerRegionX = RegionsPerWorldX - RegionsOuterBorderSize;
	MaxInnerRegionY = RegionsPerWorldY - RegionsOuterBorderSize;
}