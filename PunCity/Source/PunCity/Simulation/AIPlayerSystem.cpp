// Pun Dumnernchanvanit's


#include "AIPlayerSystem.h"

void AIPlayerSystem::Tick1Sec()
{
	SCOPE_CYCLE_COUNTER(STAT_PunAITick1Sec);

	if (!_active) {
		return;
	}

	auto& playerOwned = _simulation->playerOwned(_aiPlayerId);
	auto& resourceSystem = _simulation->resourceSystem(_aiPlayerId);

	auto& globalResourceSys = _simulation->globalResourceSystem(_aiPlayerId);
	auto& terrainGenerator = _simulation->terrainGenerator();
	auto& treeSystem = _simulation->treeSystem();
	auto& buildingSystem = _simulation->buildingSystem();
	auto& provinceSys = _simulation->provinceSystem();

	const std::vector<int32>& provincesClaimed = _simulation->GetProvincesPlayer(_aiPlayerId);

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
	 * Choose Location (Initialize)
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
					SimUtils::CanReserveSpot_NotTooCloseToAnother(provinceId, _simulation, 4))
				{
					BiomeEnum biomeEnum = _simulation->GetBiomeProvince(provinceId);
					if (biomeEnum == BiomeEnum::Desert ||
						biomeEnum == BiomeEnum::Tundra) {
						continue;
					}

					// Check if we can expand at least 5 provinces around (3 layers)
					std::vector<int32> targetProvinces = { provinceId };
					for (int32 i = 0; i < 3; i++)
					{
						for (int32 j = 0; j < targetProvinces.size(); j++) {
							const auto& connections = provinceSys.GetProvinceConnections(targetProvinces[j]);
							for (const ProvinceConnection& connection : connections) {
								if (connection.tileType == TerrainTileType::None) {
									CppUtils::TryAdd(targetProvinces, connection.provinceId);
								}
							}
						}
					}

					if (targetProvinces.size() < 5) {
						continue;
					}

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
	 * Choose initial resources (Initialize)
	 */
	if (!playerOwned.hasChosenInitialResources()) {
		FChooseInitialResources command = FChooseInitialResources::GetDefault();
		command.playerId = _aiPlayerId;
		_playerInterface->ChooseInitialResources(command);
		return;
	}

	/*
	 * Build townhall (Initialize)
	 */
	if (!playerOwned.hasCapitalTownhall())
	{
		_LOG(PunAI, "%s Try Build Townhall", AIPrintPrefix());

		PUN_CHECK(provincesClaimed.size() == 1);
		int32 provinceId = provincesClaimed[0];
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
		area.maxY = area.maxY + 5; // Ensure area center is also townhall center

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
			_LOG(PunAI, "%s Build Townhall: buildableArea isValid", AIPrintPrefix());

			auto command = MakeCommand<FPlaceBuilding>();
			command->buildingEnum = static_cast<uint8>(CardEnum::Townhall);
			command->playerId = _aiPlayerId;
			command->center = buildableArea.centerTile(); // area center is also townhall center
			command->faceDirection = static_cast<uint8>(Direction::S);

			BldInfo buildingInfo = GetBuildingInfoInt(command->buildingEnum);
			command->area = BuildingArea(command->center, buildingInfo.size, Direction::S);

			_playerInterface->PlaceBuilding(*command);
		}

		return;
	}


	/*
	 * Update Relationship
	 */
	{
		SCOPE_CYCLE_COUNTER(STAT_PunAIUpdateRelationship);

		std::vector<int32> allPlayersAndAI = _simulation->GetAllPlayersAndAI();
		for (int32 playerId = 0; playerId < allPlayersAndAI.size(); playerId++)
		{
			if (_simulation->HasTownhall(playerId))
			{
#define MODIFIER(EnumName) _relationshipModifiers.GetModifierMutable(playerId, RelationshipModifierEnum::EnumName)

				// Decaying modifiers (decay every season)
				if (Time::Ticks() % Time::TicksPerSeason == 0)
				{
					DecreaseToZero(MODIFIER(YouGaveUsGifts));
					IncreaseToZero(MODIFIER(YouStealFromUs));
					IncreaseToZero(MODIFIER(YouKidnapFromUs));
				}

				//if (Time::Ticks() % Time::TicksPerMinute == 0)
				{
					// Calculated modifiers
					auto calculateStrength = [&](int32 playerIdScope) {
						return _simulation->influence(playerIdScope) + _simulation->playerOwned(playerIdScope).totalInfluenceIncome100() * Time::RoundsPerYear;
					};
					int32 aiStrength = calculateStrength(_aiPlayerId);
					int32 counterPartyStrength = calculateStrength(playerId);

					MODIFIER(YouAreStrong) = Clamp((counterPartyStrength - aiStrength) / 50, 0, 20);
					MODIFIER(YouAreWeak) = Clamp((aiStrength - counterPartyStrength) / 50, 0, 20);

					const std::vector<int32>& provinceIds = _simulation->GetProvincesPlayer(_aiPlayerId);
					int32 borderCount = 0;
					for (int32 provinceId : provinceIds) {
						const std::vector<ProvinceConnection>& connections = _simulation->GetProvinceConnections(provinceId);
						for (const ProvinceConnection& connection : connections) {
							if (_simulation->provinceOwnerPlayer(connection.provinceId) == playerId) {
								borderCount++;
							}
						}
					}
					MODIFIER(AdjacentBordersSparkTensions) = std::max(-borderCount * 5, -20);

					// townhall nearer 500 tiles will cause tensions
					int32 townhallDistance = WorldTile2::Distance(_simulation->GetTownhallGateCapital(_aiPlayerId), _simulation->GetTownhallGateCapital(playerId));
					if (townhallDistance <= 500) {
						MODIFIER(TownhallProximitySparkTensions) = -20 * (500 - townhallDistance) / 500;
					}
					else {
						MODIFIER(TownhallProximitySparkTensions) = 0;
					}
				}
#undef MODIFIER
			}
		}
	}

	/*
	 * Do good/bad act once every year
	 */
	const int32 yearsPerAction = 5;

	int32 secondToAct = (GameRand::Rand(Time::Years()) + GameRand::Rand(_aiPlayerId)) % Time::SecondsPerYear;
	if (Time::Seconds() % Time::SecondsPerYear == secondToAct &&
		(GameRand::Rand(Time::Seconds()) % yearsPerAction) == 0)
	{
		PUN_LOG("[AIPlayer] Act pid:%d second:%d", _aiPlayerId, secondToAct);

		int32 maxRelationshipPlayerId = -1;
		int32 maxRelationship = 0;

		int32 minRelationshipPlayerId = -1;
		int32 minRelationship = 0;
		_simulation->ExecuteOnPlayersAndAI([&](int32 playerId)
		{
			int32 relationship = _relationshipModifiers.GetTotalRelationship(playerId);
			if (relationship > maxRelationship) {
				maxRelationship = relationship;
				maxRelationshipPlayerId = playerId;
			}
			if (relationship <= minRelationship)
			{
				minRelationship = relationship;
				minRelationshipPlayerId = playerId;
			}
		});

		// Gifting
		if (maxRelationshipPlayerId != -1 &&
			_simulation->HasTownhall(maxRelationshipPlayerId) &&
			_simulation->moneyCap32(_aiPlayerId) > 500)
		{
			PUN_LOG("[AIPlayer] gift pid:%d target:%d second:%d", _aiPlayerId, maxRelationshipPlayerId, secondToAct);

			auto command = make_shared<FGenericCommand>();
			command->genericCommandType = FGenericCommand::Type::SendGift;
			command->playerId = _aiPlayerId;
			command->intVar1 = maxRelationshipPlayerId;
			command->intVar2 = static_cast<int>(ResourceEnum::Money);
			command->intVar3 = std::min(100, _simulation->moneyCap32(_aiPlayerId) - 500);

			_playerInterface->GenericCommand(*command);
		}

		// TODO: ...
		//// Steal/Kidnap
		//if (minRelationshipPlayerId != -1 &&
		//	_simulation->HasTownhall(minRelationshipPlayerId))
		//{
		//	// Ideally 20 money per pop
		//	//  Target also need to have more than 1k
		//	if (_simulation->money(minRelationshipPlayerId) > 1000 &&
		//		_simulation->money(_aiPlayerId) < _simulation->population(_aiPlayerId) * 20)
		//	{
		//		PUN_LOG("[AIPlayer] Steal pid:%d target:%d second:%d", _aiPlayerId, minRelationshipPlayerId, secondToAct);

		//		auto command = make_shared<FPlaceBuilding>();
		//		command->playerId = _aiPlayerId;
		//		command->center = _simulation->townhall(minRelationshipPlayerId).centerTile();
		//		command->buildingEnum = static_cast<uint8>(CardEnum::Steal);

		//		_playerInterface->PlaceBuilding(*command);
		//	}
		//	else
		//	{
		//		PUN_LOG("AI Kidnap pid:%d second:%d", _aiPlayerId, secondToAct);
		//		
		//		auto command = make_shared<FPlaceBuilding>();
		//		command->playerId = _aiPlayerId;
		//		command->center = _simulation->townhall(minRelationshipPlayerId).centerTile();
		//		command->buildingEnum = static_cast<uint8>(CardEnum::Kidnap);

		//		_playerInterface->PlaceBuilding(*command);
		//	}
		//}
	}




	/*
	 * Prepare stats
	 */
	TownHall& townhall = _simulation->GetTownhallCapital(_aiPlayerId);
	TownManager& townManage = _simulation->townManager(_aiPlayerId);

	// Make sure townhall is marked as a city block...
	int32 townhallProvinceId = townhall.provinceId();
	for (AIRegionStatus& status : _regionStatuses)
	{
		if (status.provinceId == townhallProvinceId) {
			status.currentPurpose = AIRegionPurposeEnum::City;
			treeSystem.MarkArea(_aiPlayerId, provinceSys.GetProvinceRectArea(status.provinceId), false, ResourceEnum::None);
			break;
		}
	}

	int32 population = townManage.population();
	const std::vector<std::vector<int32>>& jobBuildingEnumToIds = townManage.jobBuildingEnumToIds();

	int32 jobSlots = 0;
	for (const std::vector<int32>& jobBuildingIds : jobBuildingEnumToIds) {
		for (int32 buildingId : jobBuildingIds)
		{
			Building& building = buildingSystem.building(buildingId);
			jobSlots += building.occupantCount();
		}
	}


	const std::vector<int32> constructionIds = townManage.constructionIds();
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
	bool hasEnoughFood = _simulation->foodCount(_aiPlayerId) >= (population * BaseHumanFoodCost100PerYear / 2 / 100);
	bool outOfFood = _simulation->foodCount(_aiPlayerId) <= (population * BaseHumanFoodCost100PerYear / 8 / 100);

	// Need heat to survive winter
	int32 heatResources = resourceSystem.resourceCount(ResourceEnum::Coal) + resourceSystem.resourceCount(ResourceEnum::Wood);
	bool hasEnoughHeat = heatResources >= population * HumanHeatResourcePerYear * 2;

	/*
	 * Place City Blocks
	 */
	auto tryPlaceCityBlock = [&](CardEnum cardEnum, AICityBlock& block)
	{
		const std::vector<int32>& buildingIds = _simulation->buildingIds(_aiPlayerId, cardEnum);
		if (buildingIds.size() == 0) {
			return;
		}

		int32 buildingId = buildingIds[GameRand::Rand() % buildingIds.size()];

		TileArea buildingArea = _simulation->building(buildingId).area();

		// include road
		buildingArea.minX--;
		buildingArea.minY--;
		buildingArea.maxX++;
		buildingArea.maxY++;

		block.TryPlaceCityBlockAroundArea(buildingArea, _aiPlayerId, _simulation);

		if (block.HasArea()) {
			std::vector<std::shared_ptr<FNetworkCommand>> commands;
			SimUtils::PlaceCityBlock(block, _aiPlayerId, commands, _simulation);
			_simulation->ExecuteNetworkCommands(commands);
		}
	};

	if (notTooManyHouseUnderConstruction)
	{
		SCOPE_CYCLE_COUNTER(STAT_PunAIPlaceCityBlock);

		int32 housingCapacity = _simulation->HousingCapacity(_aiPlayerId);
		const int32 maxHouseCapacity = 99999;// 4 * 50; // 120 max pop

		// TODO: add last building centerTile which can be used as a new spiral center

		AICityBlock block;

		// Trading Post
		if (Time::Ticks() > Time::TicksPerYear &&
			_simulation->buildingCount(_aiPlayerId, CardEnum::TradingPost) == 0)
		{
			block = AICityBlock::MakeBlock(
				{ CardEnum::TradingPost }
			);
		}
		// House
		else if (housingCapacity < maxHouseCapacity && population > housingCapacity - 5)
		{
			// Group 4 houses into a block...
			block = AICityBlock::MakeBlock(
				{ CardEnum::House, CardEnum::House },
				{ CardEnum::House, CardEnum::House }
			);
		}
		// Tavern (One for every 50 citizens)
		else if (population / 50 + 1 > _simulation->buildingCount(_aiPlayerId, CardEnum::Tavern))
		{
			block = AICityBlock::MakeBlock(
				{ CardEnum::Tavern },
				{ }
			);
		}
		else
		{
			// Industry
			if (Time::Ticks() > Time::TicksPerSeason * 3 / 2)
			{
				// Furniture Workshop
				if (_simulation->resourceCountTown(_aiPlayerId, ResourceEnum::Wood) > 200 &&
					_simulation->buildingCount(_aiPlayerId, CardEnum::FurnitureWorkshop) == 0)
				{
					block = AICityBlock::MakeBlock(
						{ CardEnum::FurnitureWorkshop },
						{ }
					);
				}
				// Beer Brewery
				else if (_simulation->resourceCountTown(_aiPlayerId, ResourceEnum::Wheat) > 100 &&
					_simulation->buildingCount(_aiPlayerId, CardEnum::BeerBrewery) == 0)
				{
					block = AICityBlock::MakeBlock(
						{ CardEnum::BeerBrewery, },
						{ }
					);
				}
				// Tailor
				else if (_simulation->buildingCount(_aiPlayerId, CardEnum::FurnitureWorkshop) > 0 &&
					_simulation->buildingCount(_aiPlayerId, CardEnum::BeerBrewery) > 0 &&
					_simulation->buildingCount(_aiPlayerId, CardEnum::Tailor) == 0)
				{
					block = AICityBlock::MakeBlock(
						{ CardEnum::Tailor, },
						{ }
					);
				}
			}

			// Storage
			if (!block.IsValid())
			{
				std::pair<int32, int32> slotsPair = _simulation->GetStorageCapacity(_aiPlayerId, true);
				int32 usedSlots = slotsPair.first;
				int32 totalSlots = slotsPair.second;

				if (usedSlots >= totalSlots * 9 / 10) // 90% full
				{
					block = AICityBlock::MakeBlock(
						{ CardEnum::StorageYard, CardEnum::StorageYard, },
						{ CardEnum::StorageYard,  CardEnum::StorageYard }
					);
				}
			}
		}


		if (block.IsValid())
		{
			// Try house, then townhall, then storage
			tryPlaceCityBlock(CardEnum::House, block);

			if (!block.HasArea()) {
				tryPlaceCityBlock(CardEnum::Townhall, block);
			}

			if (!block.HasArea()) {
				tryPlaceCityBlock(CardEnum::StorageYard, block);
			}
		}
	}

	/*
	 * Farm or Ranch placement helpers
	 */
	const std::vector<int32>& ranchIds = _simulation->buildingIds(_aiPlayerId, CardEnum::RanchPig);
	const std::vector<int32>& fisherIds = _simulation->buildingIds(_aiPlayerId, CardEnum::Fisher);
	const std::vector<int32>& farmIds = _simulation->buildingIds(_aiPlayerId, CardEnum::Farm);

	auto needFarmRanch = [&]() {
		int32 farmRanchCount = ranchIds.size() + fisherIds.size() + farmIds.size();

		const int32 farmPerPopulation = 5;
		const int32 populationFedByFruit = 10;
		int32 preferredFarmRanchCount = std::max(0, (population - populationFedByFruit) / farmPerPopulation);

		return farmRanchCount < preferredFarmRanchCount;
	};

	auto averageFertility = [&](TileArea curArea)
	{
		int32 fertility = _simulation->GetFertilityPercent(curArea.corner00()) +
			_simulation->GetFertilityPercent(curArea.corner01()) +
			_simulation->GetFertilityPercent(curArea.corner10()) +
			_simulation->GetFertilityPercent(curArea.corner11());
		return fertility / 4;
	};

	auto placeFarm = [&](TileArea area)
	{
		auto command = MakeCommand<FPlaceBuilding>();
		command->playerId = _aiPlayerId;
		command->buildingEnum = static_cast<uint8>(CardEnum::Farm);
		command->area = area;
		command->center = area.centerTile();
		command->faceDirection = uint8(Direction::S);

		_simulation->ExecuteNetworkCommand(command);
	};

	/*
	 * Place Farm
	 */
	if (notTooManyHouseUnderConstruction && needFarmRanch())
	{
		SCOPE_CYCLE_COUNTER(STAT_PunAIPlaceFarm);

		// Addon farms
		bool placed = false;

		if (farmIds.size() > 0)
		{
			int32 farmId = farmIds[GameRand::Rand() % farmIds.size()];
			Building& farm = _simulation->building(farmId);

			WorldTile2 size = GetBuildingInfo(CardEnum::Farm).size;
			TileArea largerArea(farm.area().min() - size, size + size + WorldTile2(1, 1));

			TileArea maxFertilityArea = TileArea::Invalid;
			int32 maxFertility = 0;

			largerArea.ExecuteOnBorder_WorldTile2([&](const WorldTile2& tile) {
				TileArea curArea(tile, size);
				bool hasInvalidTile = curArea.ExecuteOnAreaWithExit_WorldTile2([&](const WorldTile2& curTile) {
					return !_simulation->IsBuildableForPlayer(curTile, _aiPlayerId);
				});
				if (!hasInvalidTile) {
					int32 fertility = averageFertility(curArea);
					if (fertility > maxFertility) {
						maxFertilityArea = curArea;
						maxFertility = fertility;
					}
				}
			});

			if (maxFertilityArea.isValid()) {
				placeFarm(maxFertilityArea);
				placed = true;
			}
		}

		// initial farms (unable to do addon)
		bool isLastProvinceToFarmCheck = false;
		if (!placed)
		{
			int32 regionIndexToExplore = Time::Seconds() % _regionStatuses.size();
			AIRegionStatus& status = _regionStatuses[regionIndexToExplore];

			isLastProvinceToFarmCheck = (regionIndexToExplore == _regionStatuses.size() - 1);

			TileArea provinceArea = _simulation->GetProvinceRectArea(status.provinceId);

			// Use skip 4x4 to find max fertility tile
			WorldTile2 maxFertilityTile = WorldTile2::Invalid;
			int32 maxFertility = 0;
			provinceArea.ExecuteOnAreaSkip4_WorldTile2([&](WorldTile2 tile)
			{
				int32 fertility = terrainGenerator.GetRainfall100(tile);
				if (fertility > maxFertility) {
					maxFertilityTile = tile;
					maxFertility = fertility;
				}
			});

			// Spiral out to find buildable spot
			if (maxFertilityTile.isValid() && maxFertility > 70)
			{
				WorldTile2 size = GetBuildingInfo(CardEnum::Farm).size;

				WorldTile2 availableMinTile = AlgorithmUtils::FindNearbyAvailableTile(maxFertilityTile - size / 2, [&](const WorldTile2& tile)
				{
					TileArea curArea(tile, size);
					bool hasInvalidTile = curArea.ExecuteOnAreaWithExit_WorldTile2([&](const WorldTile2& curTile) {
						return !_simulation->IsFarmBuildable(curTile, _aiPlayerId);
					});

					return !hasInvalidTile;
				}, 500);

				if (availableMinTile.isValid()) {
					placeFarm(TileArea(availableMinTile, size));
					placed = true;
				}
			}
		}

		if (!placed)
		{
			// Try placing ranch/fisher when farm cannot be placed
			if (isLastProvinceToFarmCheck)
			{
				// Try to place fish first..
				auto tryPlaceFisher = [&]()
				{
					int32 bestEfficiency = 0;
					WorldTile2 bestCenterTile = WorldTile2::Invalid;
					Direction bestFaceDirection = Direction::S;

					for (int32 provinceClaimed : provincesClaimed)
					{
						const std::vector<WorldTile2>& coastalTiles = _simulation->provinceInfoSystem().provinceBuildingSlot(provinceClaimed).coastalTiles;

						for (const WorldTile2& coastalTile : coastalTiles)
						{
							for (int32 i = 0; i < DirectionCount; i++)
							{
								Direction faceDirection = static_cast<Direction>(i);
								TileArea area = BuildingArea(coastalTile, GetBuildingInfo(CardEnum::Fisher).size, faceDirection);

								std::vector<PlacementGridInfo> grids;
								bool setDockInstruction;
								_simulation->CheckPortArea(area, faceDirection, CardEnum::Fisher, grids, setDockInstruction, _aiPlayerId);

								bool isBuildable = true;
								for (const PlacementGridInfo& grid : grids) {
									if (grid.gridEnum == PlacementGridEnum::Red) {
										isBuildable = false;
										break;
									}
								}

								if (isBuildable)
								{
									int32 efficiency = Fisher::FisherAreaEfficiency(coastalTile, false, WorldTile2::Invalid, _simulation);
									if (efficiency > bestEfficiency) {
										bestEfficiency = efficiency;
										bestCenterTile = coastalTile;
										bestFaceDirection = faceDirection;
									}
								}
							}
						}
					}

					if (bestCenterTile.isValid())
					{
						auto command = MakeCommand<FPlaceBuilding>();
						command->playerId = _aiPlayerId;
						command->buildingEnum = static_cast<uint8>(CardEnum::Fisher);
						command->area = BuildingArea(bestCenterTile, GetBuildingInfo(CardEnum::Fisher).size, bestFaceDirection);
						command->center = bestCenterTile;
						command->faceDirection = uint8(bestFaceDirection);

						_simulation->ExecuteNetworkCommand(command);
					}

					return false;
				};

				bool placedFisher = tryPlaceFisher();

				if (!placedFisher)
				{
					AICityBlock block = AICityBlock::MakeBlock({}, { CardEnum::RanchPig });
					tryPlaceCityBlock(CardEnum::RanchPig, block);
				}
			}
		}
	}



	/*
	 * Place Province Blocks
	 */
	 // Put down forest cluster... trying to keep 20% laborer count
	{
		SCOPE_CYCLE_COUNTER(STAT_PunAIPlaceForestBlock);

		if (population * 8 / 10 > jobSlots)
		{
			// Ensure there is only one forester/fruit cluster
			bool alreadyHaveForester = false;
			bool alreadyHaveFruitCluser = false;
			for (size_t i = 0; i < _regionStatuses.size(); i++) {
				if (_regionStatuses[i].currentPurpose == AIRegionPurposeEnum::Forester) {
					alreadyHaveForester = true;
				}
				if (_regionStatuses[i].currentPurpose == AIRegionPurposeEnum::FruitGather) {
					alreadyHaveFruitCluser = true;
				}
			}


			for (size_t i = 0; i < _regionStatuses.size(); i++)
			{
				AIRegionStatus& status = _regionStatuses[i];

				// Already placed foresting cluster on this tile..
				if (status.currentPurposeDetermined()) {
					continue;
				}

				int32 provinceId = status.provinceId;
				WorldTile2 provinceCenter = provinceSys.GetProvinceCenterTile(provinceId);



				/*
				 * Set currentPurpose
				 */
				auto trySetPurpose = [&](AIRegionProposedPurposeEnum proposedPurposeEnum, AIRegionPurposeEnum purposeEnum,
					std::vector<CardEnum> topBuildingEnumsIn, std::vector<CardEnum> bottomBuildingEnumsIn = {})
				{
					if (status.currentPurpose == AIRegionPurposeEnum::None &&
						status.proposedPurpose == proposedPurposeEnum)
					{
						AICityBlock block = AICityBlock::MakeBlock(
							topBuildingEnumsIn,
							bottomBuildingEnumsIn
						);

						block.TryPlaceCityBlockSpiral(provinceCenter, _aiPlayerId, _simulation);

						if (block.HasArea()) {
							status.currentPurpose = purposeEnum;
							status.proposedBlock = block;

							TileArea provinceRectArea = provinceSys.GetProvinceRectArea(provinceId);

							// Remove gather marks, since this will be used for sustainable gathering...
							if (status.currentPurpose == AIRegionPurposeEnum::City ||
								status.currentPurpose == AIRegionPurposeEnum::Farm ||
								status.currentPurpose == AIRegionPurposeEnum::Ranch)
							{
								treeSystem.MarkArea(_aiPlayerId, provinceRectArea, false, ResourceEnum::None);
							}
							else {
								treeSystem.MarkArea(_aiPlayerId, provinceRectArea, false, ResourceEnum::Stone);
							}
						}
					}
				};

				//// Farm
				//trySetPurpose(AIRegionProposedPurposeEnum::Fertility, 
				//	AIRegionPurposeEnum::Farm,
				//	{ CardEnum::Farm, CardEnum::Farm , CardEnum::StorageYard },
				//	{ CardEnum::Farm, CardEnum::Farm, CardEnum::StorageYard });

				// Ranch
				trySetPurpose(AIRegionProposedPurposeEnum::Ranch,
					AIRegionPurposeEnum::Ranch,
					{ CardEnum::StorageYard, CardEnum::StorageYard, CardEnum::StorageYard, CardEnum::StorageYard },
					{ CardEnum::RanchPig });


				// Forester
				if (!alreadyHaveForester)
				{
					trySetPurpose(AIRegionProposedPurposeEnum::Tree,
						AIRegionPurposeEnum::Forester,
						{ CardEnum::Forester, CardEnum::CharcoalMaker, CardEnum::StorageYard },
						{ CardEnum::HuntingLodge, CardEnum::StorageYard, CardEnum::StorageYard });
				}

				// Fruit
				if (!alreadyHaveFruitCluser)
				{
					trySetPurpose(AIRegionProposedPurposeEnum::Tree,
						AIRegionPurposeEnum::FruitGather,
						{ CardEnum::FruitGatherer, CardEnum::StorageYard, CardEnum::House },
						{ CardEnum::HuntingLodge, CardEnum::StorageYard, CardEnum::StorageYard });
				}

				//// Farm
				//trySetPurpose(AIRegionProposedPurposeEnum::Tree,
				//	AIRegionPurposeEnum::Farm,
				//	{ CardEnum::Farm, CardEnum::Farm , CardEnum::StorageYard },
				//	{ CardEnum::Farm, CardEnum::Farm, CardEnum::StorageYard });


				if (status.currentPurpose == AIRegionPurposeEnum::None) {
					status.currentPurpose = AIRegionPurposeEnum::City;
				}


			}

		}


		if (notTooManyUnderConstruction)
		{
			// Place any block that hasn't been placed
			for (size_t i = 0; i < _regionStatuses.size(); i++)
			{
				if (_regionStatuses[i].currentPurpose != AIRegionPurposeEnum::None &&
					!_regionStatuses[i].blockPlaced)
				{
					if (_regionStatuses[i].proposedBlock.IsValid()) {
						std::vector<std::shared_ptr<FNetworkCommand>> commands;
						SimUtils::PlaceCityBlock(_regionStatuses[i].proposedBlock, _aiPlayerId, commands, _simulation);
						_simulation->ExecuteNetworkCommands(commands);
					}
					_regionStatuses[i].blockPlaced = true;
					break;
				}
			}
		}
	}

	/*
	 * Upgrade Townhall
	 */
	if (_simulation->HasTownhall(_aiPlayerId))
	{
		SCOPE_CYCLE_COUNTER(STAT_PunAIUpgradeTownhall);

		int32 townLvl = _simulation->GetTownLvlMax(_aiPlayerId);

		auto tryUpgrade = [&](int32 targetPopulation, int32 townhallLvl) {
			if (townLvl == townhallLvl &&
				population > targetPopulation)
			{
				TownHall& townhal = _simulation->GetTownhallCapital(_aiPlayerId);
				int32 upgradeMoney = townhal.GetUpgradeMoney(townhallLvl + 1);
				_simulation->ChangeMoney(_aiPlayerId, upgradeMoney);

				townhal.UpgradeTownhall();
			}
		};

		tryUpgrade(30, 1);
		tryUpgrade(58, 2);
		tryUpgrade(88, 3);
	}

	/*
	 * Trade
	 */
	if (Time::Ticks() % Time::TicksPerRound == 0)
	{
		SCOPE_CYCLE_COUNTER(STAT_PunAITrade);

		const std::vector<int32>& tradingPostIds = _simulation->buildingIds(_aiPlayerId, CardEnum::TradingPost);
		if (tradingPostIds.size() > 0)
		{
			TradingPost& tradingPost = _simulation->building<TradingPost>(tradingPostIds[0]);

			auto tradeCommand = make_shared<FTradeResource>();
			tradeCommand->playerId = _aiPlayerId;

			int32 currentAmount = 0;
			const int32 maxAmount = tradingPost.maxTradeQuatity();

			// Sell extra luxury
			auto sellExcess = [&](ResourceEnum resourceEnum, int32 supplyPerPop)
			{
				if (currentAmount < maxAmount)
				{
					int32 resourceCount = _simulation->resourceCountTown(_aiPlayerId, resourceEnum);
					const int32 amountPreferred = population * supplyPerPop;
					int32 resourceWanted = std::max(0, resourceCount - amountPreferred);
					if (resourceWanted > 0) {
						tradeCommand->buyEnums.Add(static_cast<uint8>(resourceEnum));
						tradeCommand->buyAmounts.Add(-resourceWanted);
						currentAmount += resourceWanted;
					}
				}
			};

			sellExcess(ResourceEnum::Beer, 5);
			sellExcess(ResourceEnum::Furniture, 5);
			sellExcess(ResourceEnum::Cloth, 5);

			sellExcess(ResourceEnum::Coal, 50);
			sellExcess(ResourceEnum::IronOre, 10);
			sellExcess(ResourceEnum::GoldOre, 10);
			sellExcess(ResourceEnum::Gemstone, 10);


			// Buy to maintain some level of goods
			auto buyUntil = [&](ResourceEnum resourceEnum, int32 target)
			{
				if (currentAmount < maxAmount)
				{
					int32 amountWanted = std::max(0, target - _simulation->resourceCountTown(_aiPlayerId, resourceEnum));
					if (amountWanted > 0) {
						tradeCommand->buyEnums.Add(static_cast<uint8>(resourceEnum));
						tradeCommand->buyAmounts.Add(amountWanted);
						currentAmount += amountWanted;
					}
				}
			};


			// Emergency buy medicine
			int32 medicineCount = _simulation->resourceCountTown(_aiPlayerId, ResourceEnum::Medicine) +
				_simulation->resourceCountTown(_aiPlayerId, ResourceEnum::Herb) / 2;
			if (medicineCount < population / 4) {
				buyUntil(ResourceEnum::Medicine, population);
			}

			// Emergency buy tools
			buyUntil(ResourceEnum::SteelTools, population / 4);

			// Buy Cheapest Food
			{
				ResourceEnum cheapestFoodEnum = ResourceEnum::Wheat;
				int32 cheapestPrice100 = _simulation->price100(cheapestFoodEnum);

				for (ResourceEnum foodEnum : FoodEnums) {
					int32 price100 = _simulation->price100(foodEnum);
					if (price100 < cheapestPrice100) {
						cheapestFoodEnum = foodEnum;
						cheapestPrice100 = price100;
					}
				}

				buyUntil(cheapestFoodEnum, 200);
			}

			// Maintain 2 years tools/medicine supply
			buyUntil(ResourceEnum::SteelTools, population * 2 / 3); // 3 years = 1 tools

			if (medicineCount < population * 2) { // Take into account Medicinal Herb
				buyUntil(ResourceEnum::Medicine, population * 2);
			}

			buyUntil(ResourceEnum::Wood, 100);
			buyUntil(ResourceEnum::Stone, 100);

			// Buy Raw materials
			if (_simulation->buildingCount(_aiPlayerId, CardEnum::BeerBrewery) > 0) {
				buyUntil(ResourceEnum::Wheat, 100);
			}
			if (_simulation->buildingCount(_aiPlayerId, CardEnum::Tailor) > 0) {
				buyUntil(ResourceEnum::Iron, 30); // Construction
				buyUntil(ResourceEnum::Leather, 100);
			}


			if (tradeCommand->buyEnums.Num() > 0)
			{
				if (_simulation->moneyCap32(_aiPlayerId) < 300)
				{
					// Cheat if no money
					_simulation->ChangeMoney(_aiPlayerId, population * 10);

					// Sell only
					for (int32 i = tradeCommand->buyAmounts.Num(); i-- > 0;) {
						if (tradeCommand->buyAmounts[i] > 0) {
							tradeCommand->buyEnums.RemoveAt(i);
							tradeCommand->buyAmounts.RemoveAt(i);
						}
					}
				}


				tradeCommand->objectId = tradingPostIds[0];


				_LOG(PunAI, "%s Trade:", AIPrintPrefix());
				for (int32 i = 0; i < tradeCommand->buyEnums.Num(); i++) {
					_LOG(PunAI, "%s : %s %d", AIPrintPrefix(), ToTChar(ResourceName(static_cast<ResourceEnum>(tradeCommand->buyEnums[i]))), tradeCommand->buyAmounts[i]);
				}

				_simulation->ExecuteNetworkCommand(tradeCommand);
			}
		}
	}

	/*
	 * Accept immigrants
	 */
	PopupInfo* popup = _simulation->PopupToDisplay(_aiPlayerId);
	if (popup && popup->replyReceiver == PopupReceiverEnum::ImmigrationEvent) {
		if (hasEnoughFood && hasEnoughHeat && population < 50) {
			townhall.AddRequestedImmigrants();
		}
		_simulation->CloseCurrentPopup(_aiPlayerId);
	}

	/*
	 * Defend land
	 */
	 //_LOG(PunAI, "%s CheckDefendLand(outer): sec:%d money:%d", AIPrintPrefix(), Time::Seconds(), resourceSystem.money());
	if (Time::Seconds() % 10 == 0 && // Check every 10 sec
		globalResourceSys.moneyCap32() > BattleInfluencePrice)
	{
		//_LOG(PunAI, "%s CheckDefendLand", AIPrintPrefix());

		const std::vector<int32>& provinceIds = _simulation->GetProvincesPlayer(_aiPlayerId);
		for (int32 provinceId : provinceIds)
		{
			//_LOG(PunAI, "%s CheckDefendLand provinceId:%d", AIPrintPrefix(), provinceId);

			ProvinceClaimProgress claimProgress = townManage.GetDefendingClaimProgress(provinceId);
			if (claimProgress.isValid())
			{
				// TODO: AI Defense
				//_LOG(PunAI, "%s CheckDefendLand Send Command provinceId:%d", AIPrintPrefix(), provinceId);

				//auto command = make_shared<FClaimLand>();
				//command->playerId = _aiPlayerId;
				//command->claimEnum = CallbackEnum::DefendProvinceMoney;
				//command->provinceId = provinceId;
				//PUN_CHECK(command->provinceId != -1);

				//_simulation->ExecuteNetworkCommand(command);
			}
		}
	}


	/*
	 * Find nearby province to claim
	 */
	int32 preferredProvinceCount = std::max(5, population / 5);
	int32 shouldProvinceClaim = (provincesClaimed.size() < 5) || Time::Ticks() % Time::TicksPerRound;

	if (shouldProvinceClaim &&
		provincesClaimed.size() < preferredProvinceCount)
	{
		SCOPE_CYCLE_COUNTER(STAT_PunAIClaimProvince);

		// Look adjacent to provincesClaimed for something to claim.
		int32 bestClaimProvinceId = -1;
		int32 bestClaimPoints = -99999;

		auto checkAdjacent = [&](ProvinceConnection connection)
		{
			if (connection.tileType == TerrainTileType::None &&
				_simulation->IsProvinceValid(connection.provinceId) &&
				_simulation->provinceOwnerTown_Major(connection.provinceId) == -1 &&
				_simulation->provinceInfoSystem().provinceDistanceToPlayer(connection.provinceId, _aiPlayerId) < 7
				)
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
		}

		if (bestClaimProvinceId != -1)
		{
			int32 regionPrice = _simulation->GetProvinceClaimPrice(bestClaimProvinceId, _aiPlayerId);
			if (globalResourceSys.moneyCap32() >= regionPrice)
			{
				// AI claim at half price...
				globalResourceSys.ChangeMoney(regionPrice / 2);

				auto command = MakeCommand<FClaimLand>();
				command->provinceId = bestClaimProvinceId;
				_playerInterface->ClaimLand(*command);
			}
		}
	}
}