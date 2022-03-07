// Pun Dumnernchanvanit's

#pragma once

#include "IGameSimulationCore.h"
#include "ProvinceSystem.h"
#include "Misc/AES.h"
#include "PunCity/AlgorithmUtils.h"
#include "AIPlayerBase.h"


/**
 * 
 */
class SimUtils
{
public:
	static bool CanReserveSpot_Buildable2(WorldRegion2 region, IGameSimulationCore* simulation)
	{
		bool canPlant = true;
		const int32 spotReserveDim = 24;
		const WorldTile2 spotReserveSize(spotReserveDim, spotReserveDim);
		TileArea area(region.centerTile() - spotReserveSize / 2, spotReserveSize);
		area.ExecuteOnArea_WorldTile2([&](WorldTile2 tile) {
			if (!simulation->IsBuildable(tile) &&
				!simulation->IsCritterBuildingIncludeFronts(tile))
			{
				std::vector<int32> frontIds = simulation->frontBuildingIds(tile);
				if (frontIds.size() > 0)
				{
					// If any front isn't regional building, we can't plant
					for (int32 frontId : frontIds) {
						if (!IsRegionalBuilding(simulation->buildingEnum(frontId))) {
							canPlant = false;
							return;
						}
					}
					return;
				}
				
				CardEnum buildingEnum = simulation->buildingEnumAtTile(tile);

				if (!IsRegionalBuilding(buildingEnum)) {
					canPlant = false;
				}
			}
		});

		return canPlant;
	}

	static bool CanReserveSpot_NotTooCloseToAnother(int32 provinceId, IGameSimulationCore* simulation, int spread = 2)
	{
		// Also do not allow reserveSpot
		//if (!region.IsInnerRegion()) {
		//	return false;
		//}
		
		//const int _startingSpotSpread = (spread != -1) ? spread : 2;

		//if (spread == -1) {
		//	spread = _startingSpotSpread;
		//}

		//PUN_LOG("CanReserveSpot_NotTooCloseToAnother spread:%d", spread);
		
		bool cannotPlant = simulation->provinceSystem().ExecuteNearbyProvincesWithExitTrue(provinceId, spread, [&](ProvinceConnection connection)
		{
			return simulation->provinceOwnerTown_Major(connection.provinceId) != -1;
		});

		return !cannotPlant;

		//bool canPlant = true;
		//int minX = std::max(0, region.x - spread);
		//int maxX = std::min(GameMapConstants::RegionsPerWorldX - 1, region.x + spread);
		//int minY = std::max(0, region.y - spread);
		//int maxY = std::min(GameMapConstants::RegionsPerWorldY - 1, region.y + spread);
		//for (int xx = minX; xx <= maxX; xx++) {
		//	for (int yy = minY; yy <= maxY; yy++)
		//	{
		//		int32_t regionId = xx + yy * GameMapConstants::RegionsPerWorldX;
		//		//if (_regionIdToStartingSpots.find(regionId) != _regionIdToStartingSpots.end()) {
		//		//	canPlant = false;
		//		//	break;
		//		//}
		//		if (simulation->regionOwner(regionId) != -1) {
		//			canPlant = false;
		//			break;
		//		}
		//	}
		//}

		//return canPlant;
	}

	static TileArea FindStartSpot(int32 provinceId, IGameSimulationCore* simulation)
	{
		auto& provinceSys = simulation->provinceSystem();
		
		PUN_CHECK(provinceSys.IsProvinceValid(provinceId));

		// Spiral from the middle of province's rectArea
		TileArea provinceRectArea = provinceSys.GetProvinceRectArea(provinceId);
		
		WorldTile2 townhallSize = GetBuildingInfo(CardEnum::Townhall).baseBuildingSize;
		TileArea area = BuildingArea(provinceRectArea.centerTile(), townhallSize, Direction::S);

		// Add Road...
		area.minX = area.minX - 1;
		area.minY = area.minY - 1;
		area.maxX = area.maxX + 1;
		area.maxY = area.maxY + 1;

		// Add Storages
		area.minY = area.minY - 5;
		area.maxY = area.maxY + 5; // Ensure area center is also townhall center

		if (!area.isValid()) {
			return TileArea::Invalid;
		}
		
		return SpiralFindAvailableBuildArea(area, [&](WorldTile2 tile)
		{
			// Out of province, can't build...
			if (abs(provinceSys.GetProvinceIdRaw(tile)) != provinceId) {
				return false;
			}
			
			// Not buildable and not critter building
			if (!simulation->IsBuildable(tile) &&
				!simulation->IsCritterBuildingIncludeFronts(tile))
			{
				std::vector<int32> frontIds = simulation->frontBuildingIds(tile);
				if (frontIds.size() > 0)
				{
					// If any front isn't regional building, we can't plant
					for (int32 frontId : frontIds) {
						if (!IsRegionalBuilding(simulation->buildingEnum(frontId))) {
							return false;
						}
					}
					return true;
				}

				CardEnum buildingEnum = simulation->buildingEnumAtTile(tile);
				return IsRegionalBuilding(buildingEnum);
			}
			return true;
		},
		[&](WorldTile2 tile) {
			return !provinceRectArea.HasTile(tile);
		});
	}

	template <typename Func, typename Func2>
	static TileArea SpiralFindAvailableBuildArea(TileArea startArea, Func isValidTile, Func2 isOutOfBound)
	{
		// Just spiral out trying to find isWalkable tile...
		int32 x = 0;
		int32 y = 0;
		int32 dx = 0;
		int32 dy = -1;

		int32 badTileCount = 0;
		startArea.ExecuteOnArea_WorldTile2([&](WorldTile2 tile) {
			if (!isValidTile(tile)) {
				badTileCount++;
			}
		});


		WorldTile2 minTile = startArea.min();
		WorldTile2 size = startArea.size();
		
		int32 loop;
		for (loop = 20000; loop-- > 0;)
		{
			TileArea area = TileArea(minTile + WorldTile2(x, y), size);

			// If area is out of bound, exit from it
			if (isOutOfBound(area.corner00()) ||
				isOutOfBound(area.corner01()) ||
				isOutOfBound(area.corner10()) ||
				isOutOfBound(area.corner11())) 
			{
				return TileArea::Invalid;
			}

			if (badTileCount == 0) {
				return area;
			}
			check(badTileCount > 0);
		
			

			// Spiral
			if ((x == y) || ((x < 0) && (x == -y)) || ((x > 0) && (x == 1 - y)))
			{
				int32 temp = dx;
				dx = -dy;
				dy = temp;
			}


			// Moving in x direction: remove -x, add +x
			if (dx > 0) {
				int32 xRemove = x;
				int32 xAdd = x + size.x;
				for (int32 y_ = 0; y_ < size.y; y_++) {
					if (!isValidTile(minTile + WorldTile2(xRemove, y_ + y))) {
						badTileCount--;
					}
					if (!isValidTile(minTile + WorldTile2(xAdd, y_ + y))) {
						badTileCount++;
					}
				}
			}
			else if (dx < 0) {
				int32 xRemove = x + size.x - 1;
				int32 xAdd = x - 1;
				for (int32 y_ = 0; y_ < size.y; y_++) {
					if (!isValidTile(minTile + WorldTile2(xRemove, y_ + y))) {
						badTileCount--;
					}
					if (!isValidTile(minTile + WorldTile2(xAdd, y_ + y))) {
						badTileCount++;
					}
				}
			}
			else if (dy > 0) {
				int32 yRemove = y;
				int32 yAdd = y + size.y;
				for (int32 x_ = 0; x_ < size.x; x_++) {
					if (!isValidTile(minTile + WorldTile2(x_ + x, yRemove))) {
						badTileCount--;
					}
					if (!isValidTile(minTile + WorldTile2(x_ + x, yAdd))) {
						badTileCount++;
					}
				}
			}
			else if (dy < 0) {
				int32 yRemove = y + size.y - 1;
				int32 yAdd = y - 1;
				for (int32 x_ = 0; x_ < size.x; x_++) {
					if (!isValidTile(minTile + WorldTile2(x_ + x, yRemove))) {
						badTileCount--;
					}
					if (!isValidTile(minTile + WorldTile2(x_ + x, yAdd))) {
						badTileCount++;
					}
				}
			}
			else {
				UE_DEBUG_BREAK();
			}
			
			

			x += dx;
			y += dy;
			
		}

		// Note: FindStartSpot isn't always valid
		//check(loop > 1);

		return TileArea::Invalid;
	}



	


	template<typename Func>
	static void PerlinRadius_ExecuteOnArea_WorldTile2(WorldTile2 center, int32 radius, IGameSimulationCore* simulation, Func func)
	{
		auto& terrainGen = simulation->terrainGenerator();
		
		TileArea area(center, radius);
		int32 random = center.tileId();
		const int32 atomShiftAmount = CoordinateConstants::AtomsPerTile / 3;
		area.ExecuteOnArea_WorldTile2([&](WorldTile2 tile)
		{
			if (simulation->IsBuildable(tile))
			{
				FloatDet perlinChance = terrainGen.resourcePerlin(tile);
				int32 dist = WorldTile2::Distance(tile, center);
				FloatDet distChance = FD0_XX(FDMax(100 - dist * 100 / radius, 0));
				FloatDet chance = FDMul(perlinChance, distChance);
				chance = FDMax(chance, 0);
				int32 chancePercent = 100 * chance / FDOne;
				
				func(chancePercent, tile);
			}
		});
	}

	/*
	 * Automatic placement
	 */
	template<class T>
	static std::shared_ptr<T> MakeCommand(int32 playerId) {
		auto command = std::make_shared<T>();
		std::static_pointer_cast<FNetworkCommand>(command)->playerId = playerId;
		return command;
	}

	static void PlaceBuildingRow(const std::vector<CardEnum>& buildingEnums, WorldTile2 start, bool isFaceSouth, int32 playerId, std::vector<std::shared_ptr<FNetworkCommand>>& commands, FactionEnum factionEnum)
	{
		int32 currentY = start.y;
		int32 currentX = start.x;

		auto placeRow = [&](CardEnum buildingEnum, int32 sign)
		{
			WorldTile2 size = GetBuildingInfo(buildingEnum).GetSize(factionEnum);

			// Storage yard always 4x4
			if (buildingEnum == CardEnum::StorageYard) {
				size = WorldTile2(4, 4);
			}

			// 1 shift 1 ... 2 shift 1 .. 3 shift 2 ... 4 shift 2 ... 5 shift 3
			int32_t yShift = (size.y + 1) / 2;
			int32_t xShift = (size.x + 1) / 2;
			WorldTile2 centerTile(currentX + sign * xShift, currentY + sign * yShift);
			Direction faceDirection = isFaceSouth ? Direction::S : Direction::N;

			auto command = MakeCommand<FPlaceBuilding>(playerId);
			command->buildingEnum = static_cast<uint8>(buildingEnum);
			command->area = BuildingArea(centerTile, size, faceDirection);
			command->center = centerTile;
			command->faceDirection = uint8(faceDirection);

			commands.push_back(command);

			//PUN_LOG("AI Build %s", *centerTile.To_FString());

			currentY += sign * size.y;
		};

		if (isFaceSouth) {
			for (CardEnum buildingEnum : buildingEnums) {
				placeRow(buildingEnum, 1);
			}
		}
		else {
			for (size_t i = buildingEnums.size(); i-- > 0;) {
				placeRow(buildingEnums[i], -1);
			}
		}
	}

	static void PlaceCityBlock(AICityBlock& block, int32 playerId, std::vector<std::shared_ptr<FNetworkCommand>>& commands, IGameSimulationCore* simulation)
	{
		PUN_LOG("PlaceCityBlock Begin playerId:%d", playerId);
		
		TileArea blockArea = block.area();

		// Build surrounding road...
		{
			TArray<int32> path;
			auto tryAddPath = [&](WorldTile2 tile) {
				if (simulation->buildingEnumAtTile(tile) == CardEnum::None &&
					!simulation->IsRoadTile(tile))
				{
					PUN_LOG("PlaceCityBlock add path %d %d", tile.x, tile.y);
					
					path.Add(tile.tileId());
				}
			};

			blockArea.ExecuteOnBorder_WorldTile2([&](WorldTile2 tile) {
				tryAddPath(tile);
			});

			auto command = MakeCommand<FPlaceDrag>(playerId);
			command->path = path;
			command->placementType = static_cast<int8>(PlacementType::DirtRoad);
			commands.push_back(command);
		}

		// Build buildings
		{
			PlaceBuildingRow(block.topBuildingEnums, blockArea.max(), false, playerId, commands, block.factionEnum);
			PlaceBuildingRow(block.bottomBuildingEnums, blockArea.min(), true, playerId, commands, block.factionEnum);
		}
	}

	// TODO: eventually only use city block??? Easiest way??
	static void PlaceForestBlock(AICityBlock& block, int32 playerId, std::vector<std::shared_ptr<FNetworkCommand>>& commands)
	{
		PUN_CHECK(block.IsValid());

		TileArea blockArea = block.area();
		int32 roadTileX = block.midRoadTileX();

		PUN_LOG("PlaceForestBlock[%d] %s", playerId, *ToFString(blockArea.ToString()));

		// Face up row
		PlaceBuildingRow(block.topBuildingEnums, WorldTile2(roadTileX, blockArea.maxY), false, playerId, commands, block.factionEnum);
		PlaceBuildingRow(block.bottomBuildingEnums, WorldTile2(roadTileX, blockArea.minY), true, playerId, commands, block.factionEnum);
	}

	template <typename Func>
	static void PlaceColonyAuxRoad(WorldTile2 townhallCenter, Direction faceDirection, Func func)
	{
		// Extra road surrounding Logistics Hub and Storage
		TileArea auxArea = BuildingArea(townhallCenter + PortColony_Storage1ShiftTileVec, Colony_InitialStorageTileSize, Direction::N);
		auxArea.maxY += 6;

		TileArea bottomRoad(auxArea.minX - 1, auxArea.minY - 1, auxArea.minX - 1, auxArea.maxY + 1);
		bottomRoad = bottomRoad.RotateArea(townhallCenter, faceDirection);
		bottomRoad.ExecuteOnArea_WorldTile2([&](WorldTile2 tile) {
			func(tile, RotateDirection(Direction::S, faceDirection));
		});
		TileArea leftRoad(auxArea.minX, auxArea.minY - 1, auxArea.maxX, auxArea.minY - 1);
		leftRoad = leftRoad.RotateArea(townhallCenter, faceDirection);
		leftRoad.ExecuteOnArea_WorldTile2([&](WorldTile2 tile) {
			func(tile, RotateDirection(Direction::E, faceDirection));
		});
		TileArea rightRoad(auxArea.minX, auxArea.maxY + 1, auxArea.maxX, auxArea.maxY + 1);
		rightRoad = rightRoad.RotateArea(townhallCenter, faceDirection);
		rightRoad.ExecuteOnArea_WorldTile2([&](WorldTile2 tile) {
			func(tile, RotateDirection(Direction::E, faceDirection));
		});
	}

	

	// For Trailer
	static bool TryPlaceArea(AICityBlock& block, WorldTile2 provinceCenter, int32 playerId, IGameSimulationCore* simulation, int32 maxLookup)
	{
		block.TryPlaceForestBlock(provinceCenter, playerId, simulation, maxLookup);
		if (block.HasArea()) {
			std::vector<std::shared_ptr<FNetworkCommand>> commands;
			PlaceCityBlock(block, playerId, commands, simulation);
			simulation->ExecuteNetworkCommands(commands);
			return true;
		}
		return false;
	}
	
};
