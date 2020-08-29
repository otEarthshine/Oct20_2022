// Pun Dumnernchanvanit's

#pragma once

#include "IGameSimulationCore.h"
#include "PlayerOwnedManager.h"
#include "GeoresourceSystem.h"
#include "PunCity/NetworkStructs.h"
#include "Resource/ResourceSystem.h"
#include "PunCity/DisplaySystem/PunTerrainGenerator.h"
#include "TreeSystem.h"
#include "BuildingSystem.h"
#include "Buildings/House.h"
#include "Buildings/TownHall.h"
#include "PunCity/CppUtils.h"
#include "SimUtils.h"

enum class AIRegionProposedPurposeEnum : uint8
{
	None,
	Tree,
	Fertility,
	Ranch,
};

static const std::vector<std::string> AIRegionProposedPurposeName
{
	"None",
	"Tree",
	"Fertility",
	"Ranch",
};

enum class AIRegionPurposeEnum : uint8
{
	None,
	City,
	FruitGather,
	Forester,
	Farm,
	Ranch,
};

static const std::vector<std::string> AIRegionPurposeName
{
	"None",
	"City",
	"FruitGather",
	"Forester",
	"Farm",
	"Ranch",
};

/*
 * Represent a building block that can be placed onto a region..
 */
class AICityBlock
{
public:
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

	void CalculateSize() {
		topTileSizeX = 0;
		int32 topTileSizeY = 0;
		for(CardEnum buildingEnum : topBuildingEnums) {
			WorldTile2 size = GetBuildingInfo(buildingEnum).size;
			topTileSizeX = std::max(topTileSizeX, static_cast<int32_t>(size.x));
			topTileSizeY += size.y;
		}
		bottomTileSizeX = 0;
		int32 bottomTileSizeY = 0;
		for (CardEnum buildingEnum : bottomBuildingEnums) {
			WorldTile2 size = GetBuildingInfo(buildingEnum).size;
			bottomTileSizeX = std::max(bottomTileSizeX, static_cast<int32_t>(size.x));
			bottomTileSizeY += size.y;
		}
		
		int32 tileSizeX = topTileSizeX + bottomTileSizeX;
		int32 tileSizeY = std::max(topTileSizeY, bottomTileSizeY);
		const int32_t roadShift = 2;

		_size = WorldTile2(tileSizeX + roadShift, tileSizeY + roadShift);
	}

	bool CanPlaceForest(WorldTile2 minTile, int32 playerId, IGameSimulationCore* simulation)
	{
		// Check block area for invalid tile
		TileArea area(minTile, _size);
		bool hasInvalid = area.ExecuteOnAreaWithExit_WorldTile2([&](WorldTile2 curTile) {
			if (curTile.isValid() && simulation->IsBuildable(curTile, playerId)) {
				return false;
			}
			return true;
		});

		return !hasInvalid;
	}

	bool CanPlaceCityBlock(WorldTile2 minTile, int32_t playerId, IGameSimulationCore* simulation)
	{
		// outer rim is checked for FrontBuildable instead
		TileArea area(minTile, _size);
		WorldTile2 maxTile = area.max();

		auto isRimBuildable = [&](TileArea rimArea) {
			bool hasInvalid = rimArea.ExecuteOnAreaWithExit_WorldTile2([&](WorldTile2 curTile) {
				return !(curTile.isValid() && simulation->IsFrontBuildable(curTile));
			});
			return !hasInvalid;
		};
		
		if (!isRimBuildable(TileArea(WorldTile2(minTile.x, minTile.y), WorldTile2(_size.x, 1)))) return false;
		if (!isRimBuildable(TileArea(WorldTile2(minTile.x, maxTile.y), WorldTile2(_size.x, 1)))) return false;
		if (!isRimBuildable(TileArea(WorldTile2(minTile.x, minTile.y), WorldTile2(1, _size.y)))) return false;
		if (!isRimBuildable(TileArea(WorldTile2(maxTile.x, minTile.y), WorldTile2(1, _size.y)))) return false;

		// Trim the area
		area.minX++;
		area.maxX--;
		area.minY++;
		area.maxY--;

		bool hasInvalid = area.ExecuteOnAreaWithExit_WorldTile2([&](WorldTile2 curTile) {
			return !(curTile.isValid() && simulation->IsBuildable(curTile, playerId));
		});
		return !hasInvalid;
	}

	void operator>>(FArchive& Ar)
	{
		SerializeVecValue(Ar, topBuildingEnums);
		SerializeVecValue(Ar, bottomBuildingEnums);

		SerializeVecValue(Ar, topBuildingIds);
		SerializeVecValue(Ar, bottomBuildingIds);

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

	std::vector<int32> topBuildingIds;
	std::vector<int32> bottomBuildingIds;

	int32 topTileSizeX = 0;
	int32 bottomTileSizeX = 0;

private:
	WorldTile2 _minTile = WorldTile2::Invalid;
	WorldTile2 _size = WorldTile2::Invalid;
};


class AIRegionStatus
{
public:
	bool useDetermined() {
		return currentPurpose != AIRegionPurposeEnum::None;
	}

	void operator>>(FArchive& Ar)
	{
		Ar << provinceId;
		Ar << proposedPurpose;
		Ar << currentPurpose;

		proposedBlock >> Ar;
		Ar << blockPlaced;

		Ar << updatedThisRound;
	}

public:
	int32 provinceId = -1;
	AIRegionProposedPurposeEnum proposedPurpose = AIRegionProposedPurposeEnum::None;
	AIRegionPurposeEnum currentPurpose = AIRegionPurposeEnum::None;

	AICityBlock proposedBlock;
	bool blockPlaced = false;
	
	bool updatedThisRound = true;
};

/**
 * 
 */
class AIPlayerSystem
{
public:
	AIPlayerSystem(int32 playerId, IGameSimulationCore* simulation, IPlayerSimulationInterface* playerInterface) {
		_playerId = playerId;
		_simulation = simulation;
		_playerInterface = playerInterface;
	}

	void SetActive(bool active) {
		_active = active;
	}
	bool active() { return _active; }

	AIRegionStatus* regionStatus(int32 provinceId) {
		auto it = std::find_if(_regionStatuses.begin(), _regionStatuses.end(), [&](AIRegionStatus& status) {
			return status.provinceId == provinceId;
		});
		if (it == _regionStatuses.end()) {
			return nullptr;
		}
		return &(*it);
	}

	void Tick1Sec()
	{
		if (!_active) {
			return;
		}
		
		auto& playerOwned = _simulation->playerOwned(_playerId);
		auto& resourceSystem = _simulation->resourceSystem(_playerId);
		auto& terrainGenerator = _simulation->terrainGenerator();
		auto& treeSystem = _simulation->treeSystem();
		auto& buildingSystem = _simulation->buildingSystem();
		auto& provinceSys = _simulation->provinceSystem();
		
		const std::vector<int32>& provincesClaimed = playerOwned.provincesClaimed();

		// Sync _regionStatuses
		{
			// Reset to false
			for (AIRegionStatus& status : _regionStatuses) {
				status.updatedThisRound = false;
			}
			// Create status for region we don't have yet
			for (int32 provinceId : provincesClaimed) {
				auto it = std::find_if(_regionStatuses.begin(), _regionStatuses.end(), [&](AIRegionStatus& status) {
					return status.provinceId == provinceId;
				});
				
				if (it == _regionStatuses.end()) 
				{
					AIRegionProposedPurposeEnum purpose = DetermineProvincePurpose(provinceId);
					_regionStatuses.push_back({ provinceId , purpose });
				}
				else {
					it->updatedThisRound = true;
				}
			}
			// Delete the regions we don't use
			for (size_t i = _regionStatuses.size(); i-- > 0;) {
				if (!_regionStatuses[i].updatedThisRound) {
					_regionStatuses.erase(_regionStatuses.begin() + i);
				}
			}
		}

		/*
		 * Choose Location
		 */
		if (!playerOwned.hasChosenLocation())
		{
			for (int count = 10000; count-- > 0;) 
			{
				//int32 regionX = GameRand::Rand() % GameMapConstants::RegionsPerWorldX_Inner + GameMapConstants::MinInnerRegionX;
				//int32 regionY = GameRand::Rand() % GameMapConstants::RegionsPerWorldY_Inner + GameMapConstants::MinInnerRegionY;
				//WorldRegion2 region(regionX, regionY);

				int32 provinceId = GameRand::Rand() % GameMapConstants::TotalRegions;

				if (provinceSys.IsProvinceValid(provinceId))
				{
					//BiomeEnum biomeEnum = terrainGenerator.GetBiome(region);
					AIRegionProposedPurposeEnum purposeEnum = DetermineProvincePurpose(provinceId);

					if (purposeEnum != AIRegionProposedPurposeEnum::None &&
						SimUtils::CanReserveSpot_NotTooCloseToAnother(provinceId, _simulation, 5))
					{
						// Note, FindStartSpot isn't always valid
						TileArea finalArea = SimUtils::FindStartSpot(provinceId, _simulation);

						if (finalArea.isValid())
						{
							auto command = MakeCommand<FChooseLocation>();
							command->provinceId = provinceId;
							command->isChoosingOrReserving = true;
							_playerInterface->ChooseLocation(*command);
							break;
						}
					}
				}
			}
			
			return;
		}

		/*
		 * Build townhall
		 */
		if (!playerOwned.hasTownhall())
		{
			PUN_CHECK(playerOwned.provincesClaimed().size() == 1);
			int32 provinceId = playerOwned.provincesClaimed()[0];
			TileArea provinceRectArea = provinceSys.GetProvinceRectArea(provinceId);

			WorldTile2 townhallSize = GetBuildingInfo(CardEnum::Townhall).size;
			TileArea area = BuildingArea(provinceRectArea.centerTile(), townhallSize, Direction::S);

			// Add Road...
			area.minX = area.minX - 1;
			area.minY = area.minY - 1;
			area.maxX = area.maxX + 1;
			area.maxY = area.maxY + 1;

			// Add Storages
			area.minY = area.minY - 5;

			PUN_CHECK(area.isValid());
			
			TileArea buildableArea = SimUtils::SpiralFindAvailableBuildArea(area, [&](WorldTile2 tile)
			{
				// Out of province, can't build...
				if (provinceSys.GetProvinceIdClean(tile) != provinceId) {
					return false;
				}

				// Not buildable and not critter building
				if (!_simulation->IsBuildable(tile) &&
					!_simulation->IsCritterBuildingIncludeFronts(tile))
				{
					std::vector<int32> frontIds = _simulation->frontBuildingIds(tile);
					if (frontIds.size() > 0)
					{
						// If any front isn't regional building, we can't plant
						for (int32 frontId : frontIds) {
							if (!IsRegionalBuilding(_simulation->buildingEnum(frontId))) {
								return false;
							}
						}
						return true;
					}

					CardEnum buildingEnum = _simulation->buildingEnumAtTile(tile);
					return IsRegionalBuilding(buildingEnum);
				}
				return true;
			},
				[&](WorldTile2 tile) {
				return !provinceRectArea.HasTile(tile);
			});

			if (buildableArea.isValid())
			{
				auto command = MakeCommand<FPlaceBuilding>();
				command->buildingEnum = static_cast<uint8>(CardEnum::Townhall);
				command->playerId = _playerId;
				command->center = buildableArea.centerTile();
				command->faceDirection = static_cast<uint8>(Direction::S);

				BldInfo buildingInfo = GetBuildingInfoInt(command->buildingEnum);
				command->area = BuildingArea(command->center, buildingInfo.size, Direction::S);
				
				_playerInterface->PlaceBuilding(*command);
			}
			
			return;
		}

		/*
		 * Prepare stats
		 */
		TownHall& townhall = _simulation->townhall(_playerId);

		// Make sure townhall is marked as a city block...
		int32 townhallProvinceId = townhall.provinceId();
		for (AIRegionStatus& status : _regionStatuses) 
		{
			if (status.provinceId == townhallProvinceId) {
				status.currentPurpose = AIRegionPurposeEnum::City;
				treeSystem.MarkArea(_playerId, provinceSys.GetProvinceRectArea(status.provinceId), false, ResourceEnum::None);
				break;
			}
		}

		int32 population = playerOwned.population();
		const std::vector<std::vector<int32>>& jobBuildingEnumToIds = playerOwned.jobBuildingEnumToIds();

		int32 jobSlots = 0;
		for (const std::vector<int32>& jobBuildingIds : jobBuildingEnumToIds) {
			for (int32 buildingId : jobBuildingIds) 
			{
				Building& building = buildingSystem.building(buildingId);
				jobSlots += building.occupantCount();
			}
		}


		const std::vector<int32> constructionIds = playerOwned.constructionIds();
		int32 housesUnderConstruction = 0;
		for (int32 constructionId : constructionIds) {
			if (_simulation->buildingEnum(constructionId) == CardEnum::House) {
				housesUnderConstruction++;
			}
		}
		int32 otherBuildingsUnderConstruction = constructionIds.size() - housesUnderConstruction;
		
		bool notTooManyHouseUnderConstruction = housesUnderConstruction <= 1;
		bool notTooManyUnderConstruction = otherBuildingsUnderConstruction <= 1;

		// Need Food 1 month in advance
		bool hasEnoughFood = _simulation->foodCount(_playerId) >= (population * BaseHumanFoodCost100PerYear / 2 / 100);
		bool outOfFood = _simulation->foodCount(_playerId) <= (population * BaseHumanFoodCost100PerYear / 8 / 100);

		// Need heat to survive winter
		int32 heatResources = resourceSystem.resourceCount(ResourceEnum::Coal) + resourceSystem.resourceCount(ResourceEnum::Wood);
		bool hasEnoughHeat = heatResources >= population * HumanHeatResourcePerYear * 2;

		// Place houses
		if (notTooManyHouseUnderConstruction)
		{
			int32 housingCapacity = _simulation->HousingCapacity(_playerId);
			const int32 maxHouseCapacity = 4 * 8;
			
			if (housingCapacity < maxHouseCapacity && population > housingCapacity)
			{
				// Group 4 houses into a block...
				AICityBlock block = AICityBlock();

				block.topBuildingEnums = { CardEnum::House, CardEnum::House };
				block.bottomBuildingEnums = { CardEnum::House, CardEnum::House };
				block.CalculateSize();

				block.SetMinTile(
					AlgorithmUtils::FindNearbyAvailableTile(townhall.centerTile(), [&](const WorldTile2& tile) {
						return block.CanPlaceCityBlock(tile, _playerId, _simulation);
					})
				);

				if (block.minTile() != WorldTile2::Invalid) {
					PlaceCityBlock(block);
				}
			}
		}

		// Put down forest cluster... trying to keep 20% laborer count
		{
			if (population * 8 / 10 > jobSlots)
			{
				AIRegionStatus* bestStatus = nullptr;
				int32 bestScore = -1;
				AICityBlock bestBlock;

				bool alreadyHaveForester = false;
				for (size_t i = 0; i < _regionStatuses.size(); i++) {
					if (_regionStatuses[i].currentPurpose == AIRegionPurposeEnum::Forester) {
						alreadyHaveForester = true;
					}
				}
				
				for (size_t i = 0; i < _regionStatuses.size(); i++) 
				{
					AIRegionStatus& status = _regionStatuses[i];

					// Already placed foresting cluster on this tile..
					if (status.useDetermined()) {
						continue;
					}

					int32 provinceId = status.provinceId;
					WorldTile2 provinceCenter = provinceSys.GetProvinceCenterTile(provinceId);

					// Make sure we can place this block
					AICityBlock block;

					if (status.proposedPurpose == AIRegionProposedPurposeEnum::Fertility) {
						block.topBuildingEnums = { CardEnum::Farm, CardEnum::Farm , CardEnum::StorageYard };
						block.bottomBuildingEnums = { CardEnum::Farm, CardEnum::Farm, CardEnum::StorageYard };
						
						status.currentPurpose = AIRegionPurposeEnum::Farm;
					}
					else if (status.proposedPurpose == AIRegionProposedPurposeEnum::Ranch) {
						block.topBuildingEnums = {};
						block.bottomBuildingEnums = { CardEnum::RanchPig };

						status.currentPurpose = AIRegionPurposeEnum::Ranch;
					}
					else if (status.proposedPurpose == AIRegionProposedPurposeEnum::Tree) {
						if (!alreadyHaveForester) {
							block.topBuildingEnums = { CardEnum::Forester, CardEnum::CharcoalMaker };
							block.bottomBuildingEnums = { CardEnum::HuntingLodge, CardEnum::StorageYard, CardEnum::StorageYard };

							status.currentPurpose = AIRegionPurposeEnum::Forester;
						}
						else {
							block.topBuildingEnums = { CardEnum::FruitGatherer, CardEnum::StorageYard };
							block.bottomBuildingEnums = { CardEnum::HuntingLodge, CardEnum::StorageYard, CardEnum::StorageYard };

							status.currentPurpose = AIRegionPurposeEnum::FruitGather;
						}
					}
					else {
						status.currentPurpose = AIRegionPurposeEnum::City;
					}
					
					block.CalculateSize();

					block.SetMinTile(
						AlgorithmUtils::FindNearbyAvailableTile(provinceCenter, [&](const WorldTile2& tile) {
							return block.CanPlaceForest(tile, _playerId, _simulation);
						}, 500)
					);

					if (!block.minTile().isValid()) {
						continue;
					}

					int32 treeCount = _simulation->GetTreeCount(provinceId);
					
					int32 distance = WorldTile2::ManDistance(provinceCenter, townhall.centerTile());

					int32 score = std::max(0, 1000 + treeCount - distance * 3);

					// too far.. score 0
					if (distance > CoordinateConstants::TilesPerRegion * 2) {
						score = 0;
					}

					if (score > bestScore) {
						bestStatus = &status;
						bestScore = score;
						bestBlock = block;
					}
				}

				if (bestStatus) 
				{
					TileArea bestProvinceRectArea = provinceSys.GetProvinceRectArea(bestStatus->provinceId);
					
					// Remove gather marks, since this will be used for sustainable gathering...
					if (bestStatus->currentPurpose == AIRegionPurposeEnum::City ||
						bestStatus->currentPurpose == AIRegionPurposeEnum::Farm ||
						bestStatus->currentPurpose == AIRegionPurposeEnum::Ranch) 
					{
						treeSystem.MarkArea(_playerId, bestProvinceRectArea, false, ResourceEnum::None);
					}
					else {
						treeSystem.MarkArea(_playerId, bestProvinceRectArea, false, ResourceEnum::Stone);
					}

					bestStatus->proposedBlock = bestBlock;
				}

				
			}

			if (notTooManyUnderConstruction)
			{
				for (size_t i = 0; i < _regionStatuses.size(); i++) 
				{
					if (_regionStatuses[i].currentPurpose != AIRegionPurposeEnum::None &&
						!_regionStatuses[i].blockPlaced) 
					{
						if (_regionStatuses[i].proposedBlock.IsValid()) {
							PlaceForestBlock(_regionStatuses[i].proposedBlock);
						}
						_regionStatuses[i].blockPlaced = true;
						break;
					}
				}
			}
		}

		/*
		 * Accept immigrants
		 */
		PopupInfo* popup = _simulation->PopupToDisplay(_playerId);
		if (popup && popup->replyReceiver == PopupReceiverEnum::ImmigrationEvent) {
			if (hasEnoughFood && hasEnoughHeat && population < 50) {
				townhall.AddRequestedImmigrants();
			}
			_simulation->CloseCurrentPopup(_playerId);
		}

		/*
		 * Find nearby region to claim
		 */
		if (provincesClaimed.size() < 5)
		{
			// Look adjacent to provincesClaimed for something to claim.
			int32 bestClaimProvinceId = -1;
			int32 bestClaimPoints = -99999;
			
			auto checkAdjacent = [&] (ProvinceConnection connection)
			{
				if (_simulation->IsProvinceValid(connection.provinceId) && 
					_simulation->provinceOwner(connection.provinceId) == -1)
				{
					// best place to claim is the one with resource or flat land
					int32 flatLandPoints = provinceSys.provinceFlatTileCount(connection.provinceId) / 2;
					int32 treePoints = _simulation->GetTreeCount(connection.provinceId) * 2;
					//int32 randomPoints = GameRand::Rand() % std::max(treePoints, 1);

					WorldTile2 provinceCenter = provinceSys.GetProvinceCenterTile(connection.provinceId);

					const int32 distanceFactor = 70;
					int32 distance = WorldTile2::ManDistance(provinceCenter, townhall.centerTile());
					int32 distancePoints = distance * (flatLandPoints + treePoints) / CoordinateConstants::TilesPerRegion * distanceFactor / 100;
					//int32_t resourcePoints =

					AIRegionProposedPurposeEnum purpose = DetermineProvincePurpose(connection.provinceId);

					if (purpose != AIRegionProposedPurposeEnum::None)
					{
						int32 points = flatLandPoints + treePoints - distancePoints;

						if (points > bestClaimPoints) {
							bestClaimProvinceId = connection.provinceId;
							bestClaimPoints = points;
						}
					}
				}
			};
			
			for (int32 provinceId : provincesClaimed) {
				provinceSys.ExecuteAdjacentProvinces(provinceId, checkAdjacent);
				
				//checkAdjacent(WorldRegion2(claimedRegion.x + 1, claimedRegion.y));
				//checkAdjacent(WorldRegion2(claimedRegion.x - 1, claimedRegion.y));
				//checkAdjacent(WorldRegion2(claimedRegion.x, claimedRegion.y + 1));
				//checkAdjacent(WorldRegion2(claimedRegion.x, claimedRegion.y - 1));
			}

			if (bestClaimProvinceId != -1)
			{
				int32 regionPrice = playerOwned.GetBaseProvinceClaimPrice(bestClaimProvinceId);
				
				if (resourceSystem.money() >= regionPrice) 
				{
					// AI claim at half price...
					resourceSystem.ChangeMoney(regionPrice / 2);
					
					auto command = MakeCommand<FClaimLand>();
					command->provinceId = bestClaimProvinceId;
					_playerInterface->ClaimLand(*command);
				}
			}
		}
	}

	/*
	 * System
	 */

	void Serialize(FArchive& Ar)
	{
		SerializeVecObj(Ar, _regionStatuses);
		Ar << _active;
	}

	void AIDebugString(std::stringstream& ss) {
		ss << "-- AI Regions " << _simulation->playerName(_playerId) << "\n";
		for (AIRegionStatus& status : _regionStatuses) {
			ss << " Region " << status.provinceId << " purpose:" << AIRegionPurposeName[static_cast<int>(status.currentPurpose)] << " proposed:" << static_cast<int>(status.proposedPurpose);
		}
	}

private:
	template<class T>
	std::shared_ptr<T> MakeCommand() {
		auto command = std::make_shared<T>();
		std::static_pointer_cast<FNetworkCommand>(command)->playerId = _playerId;
		return command;
	}

	void PlaceBuildingRow(const std::vector<CardEnum>& buildingEnums, WorldTile2 start, bool isFaceSouth)
	{
		int32 currentY = start.y;
		int32 currentX = start.x;

		auto placeRow = [&](CardEnum buildingEnum, int32_t sign)
		{
			WorldTile2 size = GetBuildingInfo(buildingEnum).size;

			// Storage yard always 4x4
			if (buildingEnum == CardEnum::StorageYard) {
				size = WorldTile2(4, 4);
			}
			
			// 1 shift 1 ... 2 shift 1 .. 3 shift 2 ... 4 shift 2 ... 5 shift 3
			int32_t yShift = (size.y + 1) / 2;
			int32_t xShift = (size.x + 1) / 2;
			WorldTile2 centerTile(currentX + sign * xShift, currentY + sign * yShift);
			Direction faceDirection = isFaceSouth ? Direction::S : Direction::N;

			auto command = MakeCommand<FPlaceBuilding>();
			command->buildingEnum = static_cast<uint8>(buildingEnum);
			command->area = BuildingArea(centerTile, size, faceDirection);
			command->center = centerTile;
			command->faceDirection = uint8(faceDirection);

			
			_playerInterface->PlaceBuilding(*command);

			//PUN_LOG("AI Build %s", *centerTile.To_FString());

			currentY += sign * size.y;
		};

		if (isFaceSouth) {
			for (CardEnum buildingEnum : buildingEnums) {
				placeRow(buildingEnum, 1);
			}
		} else {
			for (size_t i = buildingEnums.size(); i-- > 0;) {
				placeRow(buildingEnums[i], -1);
			}
		}
	}

	void PlaceCityBlock(AICityBlock& block)
	{
		TileArea blockArea = block.area();

		// Build surrounding road...
		{
			TArray<int32> path;
			for (int32_t x = blockArea.minX; x <= blockArea.maxX; x++) {
				path.Add(WorldTile2(x, blockArea.minY).tileId());
				path.Add(WorldTile2(x, blockArea.maxY).tileId());
			}
			for (int32_t y = blockArea.minY + 1; y <= blockArea.maxY - 1; y++) {
				path.Add(WorldTile2(blockArea.minX, y).tileId());
				path.Add(WorldTile2(blockArea.maxX, y).tileId());
			}

			auto command = MakeCommand<FPlaceDrag>();
			command->path = path;
			command->placementType = static_cast<int8>(PlacementType::DirtRoad);
			_playerInterface->PlaceDrag(*command);
		}

		// Build buildings
		{
			PlaceBuildingRow(block.topBuildingEnums, blockArea.max(), false);
			PlaceBuildingRow(block.bottomBuildingEnums, blockArea.min(), true);
		}
	}

	void PlaceForestBlock(AICityBlock& block)
	{
		PUN_CHECK(block.IsValid());
		
		TileArea blockArea = block.area();
		int32 roadTileX = block.midRoadTileX();

		//PUN_LOG("PlaceForestBlock %s", *ToFString(blockArea.ToString()));

		// Face up row
		PlaceBuildingRow(block.topBuildingEnums, WorldTile2(roadTileX, blockArea.maxY), false);
		PlaceBuildingRow(block.bottomBuildingEnums, WorldTile2(roadTileX, blockArea.minY), true);
	}

	AIRegionProposedPurposeEnum DetermineProvincePurpose(int32 provinceId)
	{
		auto& terrainGenerator = _simulation->terrainGenerator();
		auto& provinceSys = _simulation->provinceSystem();
		
		int32 treeCount = _simulation->GetTreeCount(provinceId);
		int32 fertility = terrainGenerator.GetRegionFertility(provinceSys.GetProvinceCenterTile(provinceId).region()); // Rough fertility calculation
		int32 flatLandCount = provinceSys.provinceFlatTileCount(provinceId);
		
		if (fertility > 75) {
			return AIRegionProposedPurposeEnum::Fertility;
		}
		if (treeCount > CoordinateConstants::TileIdsPerRegion / 4 / 2) {
			return AIRegionProposedPurposeEnum::Tree;
		}
		if (fertility > 60) {
			return AIRegionProposedPurposeEnum::Fertility;
		}
		//if (fertility > 30 && flatLandCount > CoordinateConstants::TileIdsPerRegion * 7/8 ) {
		//	return AIRegionProposedPurposeEnum::Ranch;
		//}
		if (treeCount > CoordinateConstants::TileIdsPerRegion / 4 / 4) {
			return AIRegionProposedPurposeEnum::Tree;
		}
		if (fertility > 50) {
			return AIRegionProposedPurposeEnum::Fertility;
		}

		return AIRegionProposedPurposeEnum::None;
	}

private:
	int32 _playerId;
	IGameSimulationCore* _simulation;
	IPlayerSimulationInterface* _playerInterface;

	/*
	 * Serialize
	 */
	std::vector<AIRegionStatus> _regionStatuses;
	bool _active = false;
};
