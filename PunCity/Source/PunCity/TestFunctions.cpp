// Fill out your copyright notice in the Description page of Project Settings.

#include "TestFunctions.h"

static void TestAStar()
{
	//GameMap::SetRegionsPerWorld(regionSizeX, regionSizeY);

	//int sizeX = GameMap::TilesPerWorldX;
	//int sizeY = GameMap::TilesPerWorldY;

	////! AStar Test
	//PunAStar128x256 punAStar(sizeX, sizeY, sizeX * sizeY);
	//for (int i = 0; i < 40; i++) {
	//	punAStar.SetWalkable(i, 33, false);
	//}

	//std::vector<uint32_t> waypoint;
	//int testInt = 0;
	//punAStar.FindPath(0, 0, 25, 8, waypoint);
	//for (int i = 0; i < waypoint.size(); i++) {
	//	WorldTile2 tile = MapUtil::UnpackAStarInt(waypoint[i]);
	//	UE_LOG(LogTemp, Error, TEXT("waypoint: %s"), *FString(tile.ToString().c_str()));
	//}

	//auto t_1 = high_resolution_clock::now();
	//for (int y = 0; y < 50; y++)
	//{
	//	for (int x = 0; x < 100; x++)
	//	{
	//		punAStar.FindPath(x * 80, y * 80, x * 80 + 25, y * 80 + 8, waypoint);
	//		testInt += waypoint.size();
	//	}
	//}
	//UE_LOG(LogTemp, Error, TEXT("PathTime: %d"), duration_cast<microseconds>(high_resolution_clock::now() - t_1).count());
}