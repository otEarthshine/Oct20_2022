// Pun Dumnernchanvanit's

#pragma once

#include "CoreMinimal.h"
#include "PunCity/DisplaySystem/DisplaySystemComponent.h"
#include "DebugDisplayComponent.generated.h"

// TODO: move these to DisplayInfo??
static FLinearColor GetFloodColor(int floodId)
{
	switch (floodId)
	{
	case -1: return FLinearColor::Red;
	case 0: return FLinearColor::White;
	case 1: return FLinearColor::Green;
	case 2: return FLinearColor::Yellow;
	case 3: return FLinearColor::Blue;
	case 4: return FLinearColor(0, 0, .2);
	case 5: return FLinearColor(0, .2, 0);
	case 6: return FLinearColor(.2, 0, 0);
	case 7: return FLinearColor(.05, .05, .05);
	case 8: return FLinearColor(.05, .05, 0);
	case 9: return FLinearColor(0, .05, .05);
	case 10: return FLinearColor(.05, 0, .05);
	case 11: return FLinearColor(.05, 0, 0);
	case 12: return FLinearColor(0, .05, 0);
	case 13: return FLinearColor(0, 0, .05);
	case 14: return FLinearColor(.2, .05, .05);
	}
	return FLinearColor::Black;
}

static FLinearColor GetRandomColor(int32 colorId)
{
	int32 color1 = GameRand::DisplayRand(colorId);
	int32 color2 = GameRand::DisplayRand(color1);
	int32 color3 = GameRand::DisplayRand(color2);
	return FLinearColor((color1 % 255) / 255.0f, (color2 % 255) / 255.0f, (color3 % 255) / 255.0f);
}

/**
 * 
 */
UCLASS()
class PROTOTYPECITY_API UDebugDisplayComponent : public UDisplaySystemComponent
{
	GENERATED_BODY()
public:

protected:
	//int CreateNewDisplay(int objectId) override {} 
	//void OnSpawnDisplay(int objectId, int meshId, WorldAtom2 cameraAtom) override {}	
	//void HideDisplay(int meshId) override;
	
	void UpdateDisplay(int regionId, int meshId, WorldAtom2 cameraAtom, bool justSpawned, bool justCreated) override
	{
		LLM_SCOPE_(EPunSimLLMTag::PUN_DisplayTerrain);

		WorldRegion2 region(regionId);

		if (isMainMenuDisplay) {
			return;
		}

		auto& sim = simulation();
		TreeSystem& treeSystem = sim.treeSystem();
		auto& terrainGen = sim.terrainGenerator();

		/*
		 * Debug
		 */
		// "TreeGrid" in TileObjDisplay
		if (PunSettings::Settings["WalkableGrid"] ||
			PunSettings::Settings["ShippingGrid"] ||
			PunSettings::Settings["BuildingGrid"] ||
			PunSettings::Settings["BuildingId"] ||
			PunSettings::Settings["FloodId"] ||
			PunSettings::Settings["FloodIdHuman"] ||
			PunSettings::Settings["Province"])
		{
			auto pathAI = sim.pathAI();
			ULineBatchComponent* line = lineBatch();
			auto& buildingSys = sim.buildingSystem();

			int startX = region.minXTile();
			int startY = region.minYTile();

			auto colorFromTileType = [&](TerrainTileType tileType) -> FLinearColor
			{
				switch (tileType)
				{
				case TerrainTileType::Ocean: return FLinearColor::Blue;
				case TerrainTileType::River: return FLinearColor(0.2, 0.2, 1);
				case TerrainTileType::Mountain: return  FLinearColor(0.4f, 0.3f, 0.3f);
				default:
					return FLinearColor(1, 0, 1);
				}
			};

			auto drawLine = [&](FVector start, FVector end, FLinearColor color) {
				line->DrawLine(start, end, color, 100.0f, 1.0f, 10000);
			};


			for (int y = startY; y < startY + CoordinateConstants::TilesPerRegion; y++) {
				for (int x = startX; x < startX + CoordinateConstants::TilesPerRegion; x++)
				{
					WorldTile2 curTile(x, y);
					FVector start = MapUtil::DisplayLocation(cameraAtom, curTile.worldAtom2());

					if (PunSettings::Settings["WalkableGrid"])
					{
						FLinearColor color = FLinearColor::White;

						// AStar Debug
						if (!pathAI->isWalkable(x, y)) {
							color = FLinearColor::Red;
						}
						else if (pathAI->isRoad(x, y)) {
							color = FLinearColor::Green;
						}

						line->DrawLine(start, start + FVector(0, 5, 10), color, 100.0f, 1.0f, 10000);
					}

					if (PunSettings::Settings["ShippingGrid"])
					{
						if (x % 4 == 0 && y % 4 == 0) 
						{
							// AStar Debug
							if (pathAI->isWater(x / 4, y / 4)) {
								line->DrawLine(start, start + FVector(0, 5, 10), FLinearColor::Blue, 100.0f, 1.0f, 10000);
							}
						}
					}

					if (PunSettings::Settings["BuildingGrid"])
					{
						FLinearColor color = FLinearColor::Green;
						TerrainTileType tileType = terrainGen.terrainTileType(curTile.tileId());

						CardEnum buildingEnumAtTile = sim.buildingEnumAtTile(curTile);
						if (IsBridgeOrTunnel(buildingEnumAtTile)) {
							color = FLinearColor(0.5f, .0f, .0f);
						}
						else if (tileType != TerrainTileType::None) {
							color = colorFromTileType(tileType);
						}
						else if (sim.buildingEnumAtTile(curTile) == CardEnum::DirtRoad) {
							color = FLinearColor(1.0f, 1.0f, 0.0f);
						}
						else if (sim.buildingEnumAtTile(curTile) == CardEnum::TrapSpike) {
							color = FLinearColor(0.2f, 0, 0);
						}
						else if (sim.buildingEnumAtTile(curTile) != CardEnum::None) {
							color = FLinearColor::Red;
						}
						else if (buildingSys.frontBuildingIds(curTile).size() > 0) {
							color = FLinearColor(1.0f, 0.5f, 0.0f);
						}
						else if (treeSystem.tileObjEnum(curTile.tileId()) != TileObjEnum::None) {
							color = FLinearColor(0.0f, 0.1f, 0.0f);
						}
						line->DrawLine(start, start + FVector(5, 0, 10), color, 100.0f, 1.0f, 10000);
					}

					if (PunSettings::Settings["BuildingId"])
					{
						int32 buildingId = simulation().buildingIdAtTile(curTile);
						if (buildingId != -1) {
							FLinearColor color((GameRand::DisplayRand(buildingId) % 255) / 255.0f,
								(GameRand::DisplayRand(buildingId + 1) % 255) / 255.0f,
								(GameRand::DisplayRand(buildingId + 2) % 255) / 255.0f);
							line->DrawLine(start, start + FVector(-5, -5, 10), color, 100.0f, 1.0f, 10000);
						}
					}

					if (PunSettings::Settings["FloodId"])
					{
						int16 floodId = sim.floodSystem().GetFloodId(curTile);
						line->DrawLine(start, start + FVector(2, 0, 2), GetFloodColor(floodId), 100.0f, 1.0f, 10000);
					}
					//if (PunSettings::Settings["FloodIdHuman"])
					//{
					//	int16 floodId = sim.floodSystemHuman().GetFloodId(curTile);
					//	line->DrawLine(start, start + FVector(2, 0, 2), GetFloodColor(floodId), 100.0f, 1.0f, 10000);
					//}
					if (PunSettings::Settings["Province"])
					{
						if (curTile.x % 2 == 0 && curTile.y % 2 == 0)
						{
							int32 provinceId = sim.GetProvinceIdRaw(curTile);
							FLinearColor color = GetRandomColor(abs(provinceId));
							if (provinceId == EmptyProvinceId) {
								color = FLinearColor::White;
							}
							else if (provinceId == MountainProvinceId) {
								color = FLinearColor::Black;
							}
							else if (provinceId == OceanProvinceId) {
								color = FLinearColor::Blue;
							}
							else if (provinceId == RiverProvinceId) {
								color = FLinearColor(0.2, 0.2, 1);
							}
							line->DrawLine(start, start + FVector(2, 0, provinceId > 0 ? 2 : 20), color, 100.0f, 1.0f, 10000);

							// Show the edge
							auto& provinceSys = sim.provinceSystem();
							if (IsValidRawProvinceId(provinceId))
							{
								provinceId = abs(provinceId);

								// For displaying only once element in the province
								if (!provinceSys.displayedProvinceThisTick[provinceId]) 
								{
									provinceSys.displayedProvinceThisTick[provinceId] = true;

									int32 townId = sim.provinceOwnerTown(provinceId);

									int32 clusterId = -1;
									if (townId != -1) {
										clusterId = provinceSys.GetTerritoryClusterId(townId, provinceId);
									}

									const std::vector<WorldTile2x2>& edges1 = clusterId != -1 ? provinceSys.GetTerritoryEdges1(townId, clusterId) : provinceSys.GetProvinceEdges1(provinceId);
									const std::vector<WorldTile2x2>& edges2 = clusterId != -1 ? provinceSys.GetTerritoryEdges2(townId, clusterId) : provinceSys.GetProvinceEdges2(provinceId);
									
									/*
									 * Edges
									 */
									for (int32 i = 1; i < edges1.size(); i++)
									{
										FVector to2x2Center(5, 5, 0);

										FVector tileVec1_1 = MapUtil::DisplayLocation(cameraAtom, edges1[i - 1].worldTile2().worldAtom2()) + to2x2Center;
										FVector tileVec1_2 = MapUtil::DisplayLocation(cameraAtom, edges2[i - 1].worldTile2().worldAtom2()) + to2x2Center;
										FVector tileVec1 = (tileVec1_1 + tileVec1_2) / 2;
										tileVec1 += FVector(0, 0, 2);

										FVector tileVec2_1 = MapUtil::DisplayLocation(cameraAtom, edges1[i].worldTile2().worldAtom2()) + to2x2Center;
										FVector tileVec2_2 = MapUtil::DisplayLocation(cameraAtom, edges2[i].worldTile2().worldAtom2()) + to2x2Center;
										FVector tileVec2 = (tileVec2_1 + tileVec2_2) / 2;
										tileVec2 += FVector(0, 0, 7);

										// First edge especially thick
										int32 thickness = (i == 1) ? 3.0f : 1.0f;

										line->DrawLine(tileVec1, tileVec2, color, 100.0f, thickness, 10000);
									}

									// Show the province information
									{
										WorldTile2 tile = provinceSys.GetProvinceCenterTile(provinceId);
										FVector tileVec = MapUtil::DisplayLocation(cameraAtom, tile.worldAtom2()) + FVector(0, 0, 2);


										// Tile count bar
										drawLine(tileVec, tileVec + FVector(0, 0, provinceSys.provinceMountainTileCount(provinceId)), FLinearColor::Gray);
										drawLine(tileVec + FVector(10, 10, 0), tileVec + FVector(10, 10, provinceSys.provinceOceanTileCount(provinceId)), FLinearColor::Blue);
										drawLine(tileVec + FVector(15, 15, 0), tileVec + FVector(15, 15, provinceSys.provinceRiverTileCount(provinceId)), FLinearColor(0.2, 0.2, 1));
										drawLine(tileVec + FVector(20, 20, 0), tileVec + FVector(20, 20, provinceSys.provinceFlatTileCount(provinceId)), FLinearColor::Yellow);

										// Animal count bar
										int32 animalCount = simulation().provinceAnimals(provinceId).size();
										for (int32 i = 0; i < animalCount; i++) {
											FVector animalBarStart = tileVec + FVector(40, 40, i * 10);
											drawLine(animalBarStart, animalBarStart + FVector(3, 0, 10), FLinearColor::Green);
										}
										
										// Connections
										const std::vector<ProvinceConnection>& connections = provinceSys.GetProvinceConnections(provinceId);
										for (int32 c = 0; c < connections.size(); c++)
										{
											// Only flat land and river which can be crossed
											if (PunSettings::IsOn("ProvinceFlatConnectionOnly") &&
												!IsLandPassableTileType(connections[c].tileType)) {
												continue;
											}

											WorldTile2 neighborCenter = provinceSys.GetProvinceCenterTile(connections[c].provinceId);
											FVector neighborVec = MapUtil::DisplayLocation(cameraAtom, neighborCenter.worldAtom2()) + FVector(0, 0, 2);

											drawLine(tileVec + FVector(0, 0, 10 * c), neighborVec + FVector(0, 0, 10 * c), colorFromTileType(connections[c].tileType));
										}
									}
								}
							}
						}
					}
				}
			}

			FVector pos00 = MapUtil::DisplayLocation(cameraAtom, WorldTile2(startX, startY).worldAtom2());
			FVector pos01 = MapUtil::DisplayLocation(cameraAtom, WorldTile2(startX, region.maxYTile()).worldAtom2());
			FVector pos10 = MapUtil::DisplayLocation(cameraAtom, WorldTile2(region.maxXTile(), startY).worldAtom2());
			FVector pos11 = MapUtil::DisplayLocation(cameraAtom, WorldTile2(region.maxXTile(), region.maxYTile()).worldAtom2());
			line->DrawLine(pos00, pos01, FLinearColor(0.1, 0, 0.1), 100.0f, 1.0f, 10000);
			line->DrawLine(pos01, pos11, FLinearColor(0.1, 0, 0.1), 100.0f, 1.0f, 10000);
			line->DrawLine(pos11, pos10, FLinearColor(0.1, 0, 0.1), 100.0f, 1.0f, 10000);
			line->DrawLine(pos10, pos00, FLinearColor(0.1, 0, 0.1), 100.0f, 1.0f, 10000);
		}

		// Flood Connections
		if (PunSettings::Settings["FloodId"] ||
			PunSettings::Settings["FloodIdHuman"])
		{
			int16_t region64Id = TileToRegion64Id(region.centerTile());

			//GameMapFlood* floodSystem = PunSettings::Settings["FloodId"] ? &(simulation().floodSystem()) : &(simulation().floodSystemHuman());
			GameMapFlood* floodSystem = &(simulation().floodSystem());
			//GameMapFlood* floodSystem = &(simulation()->floodSystem());

			RegionFloodConnections& region64Connections = floodSystem->region64ToConnections()[region64Id];
			std::array<std::vector<FloodConnection>, 4>& directionToConnections = region64Connections.directionToConnections();

			for (int directionInt = 0; directionInt < DirectionCount; directionInt++)
			{
				std::vector<FloodConnection>& connectionsInDir = directionToConnections[directionInt];
				Direction direction = static_cast<Direction>(directionInt);
				int16_t neighborRegion64Id = Region64Neighbor(region64Id, direction);

				if (neighborRegion64Id != -1)
				{
					WorldTile2 centerTile(Region64X(region64Id) * Region64Size + Region64Size / 2,
						Region64Y(region64Id) * Region64Size + Region64Size / 2);
					WorldTile2 neighborCenterTile(Region64X(neighborRegion64Id) * Region64Size + Region64Size / 2,
						Region64Y(neighborRegion64Id) * Region64Size + Region64Size / 2);

					FVector shiftCenter = MapUtil::DisplayLocation(cameraAtom, (centerTile + WorldTile2::DirectionTile(direction) * 10).worldAtom2());
					FVector neighboShiftCenter = MapUtil::DisplayLocation(cameraAtom, (neighborCenterTile - WorldTile2::DirectionTile(direction) * 10).worldAtom2());

					for (int i = 0; i < connectionsInDir.size(); i++)
					{
						int16_t originId = connectionsInDir[i].originId;
						int16_t neighborId = connectionsInDir[i].neighborId;

						float localShiftX = GameRand::DisplayRand(originId * 1000 + region64Id) % 50 - 25;
						float localShiftY = GameRand::DisplayRand(originId * 100 + neighborId * 100000 + neighborRegion64Id) % 50 - 25;
						FVector zLift = direction != Direction::E ? FVector(0, 0, 20) : FVector(5, 5, 40);
						FVector start = shiftCenter + FVector(localShiftX, localShiftY, 0);
						FVector end = neighboShiftCenter + FVector(localShiftX, localShiftY, 0);

						lineBatch()->DrawLine(shiftCenter, start, GetFloodColor(originId), 100.0f, 1.0f, 10000);
						lineBatch()->DrawLine(start, start + zLift, GetFloodColor(originId), 100.0f, 1.0f, 10000);
						lineBatch()->DrawLine(start + zLift, end + zLift, GetFloodColor(originId), 100.0f, 1.0f, 10000);
						lineBatch()->DrawLine(end + zLift, end, GetFloodColor(neighborId), 100.0f, 1.0f, 10000);
						lineBatch()->DrawLine(end, neighboShiftCenter, GetFloodColor(neighborId), 100.0f, 1.0f, 10000);
					}
				}
			}
		}

		//if (PunSettings::Settings[""])
		{
			auto& debugLineSystem = simulation().debugLineSystem();
			std::vector<PunDebugLine>& lines = debugLineSystem.GetLines(regionId);
			for (int i = 0; i < lines.size(); i++) {
				FVector start = MapUtil::DisplayLocation(cameraAtom, lines[i].startAtom) + lines[i].startShift;
				FVector end = MapUtil::DisplayLocation(cameraAtom, lines[i].endAtom) + lines[i].endShift;
				lineBatch()->DrawLine(start, end, lines[i].Color, 100.0f, lines[i].thickness, 10000);
			}
		}
	}


};
