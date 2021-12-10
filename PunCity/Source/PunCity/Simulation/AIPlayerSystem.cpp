// Pun Dumnernchanvanit's


#include "AIPlayerSystem.h"

#include "BuildingCardSystem.h"

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
		TryChooseLocation();
		return;
	}

	/*
	 * Choose initial resources (Initialize)
	 */
	if (!playerOwned.hasChosenInitialResources()) {
		FChooseInitialResources command = FChooseInitialResources::GetDefault(factionEnum());
		command.playerId = _aiPlayerId;
		_playerInterface->ChooseInitialResources(command);
		return;
	}

	/*
	 * Build townhall (Initialize)
	 */
	if (!playerOwned.hasCapitalTownhall())
	{
		TryBuildTownhall();
		return;
	}

	/*
	 * Do good/bad act once every X year
	 */
	Tick1Sec_DoAction();




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
	 * Upgrade Era
	 */
	int32 era = _simulation->GetEra(_aiPlayerId);

	const std::vector<int32> popToIncreaseEra {
		0,
		50,
		100,
		200,
	};

	auto unlockTech = [&](TechEnum techEnum)
	{
		auto unlockSys = _simulation->unlockSystem(_aiPlayerId);
		std::shared_ptr<ResearchInfo> tech = unlockSys->GetTechInfo(techEnum);
		tech->state = TechStateEnum::Researched;
		tech->upgradeCount++;
		unlockSys->techsFinished++;
	};

	if (era == 1)
	{
		if (population >= popToIncreaseEra[era]) {
			unlockTech(TechEnum::MiddleAge);
		}
	}
	else if (era == 2)
	{
		if (population >= popToIncreaseEra[era]) {
			unlockTech(TechEnum::EnlightenmentAge);
			unlockTech(TechEnum::Logistics5);
		}
	}
	else if (era == 3)
	{
		if (population >= popToIncreaseEra[era]) {
			unlockTech(TechEnum::IndustrialAge);
		}
	}

	CardEnum storageEnum = (era == 3 || population > 70) ? CardEnum::Warehouse : CardEnum::StorageYard;
	

	/*
	 * Place City Blocks
	 */
	
	auto hasNoRecentUnderConstruction = [&](CardEnum cardEnum, int32 gapTicks = Time::TicksPerSeason)
	{
		const std::vector<int32>& bldIds = _simulation->buildingIds(_aiPlayerId, cardEnum);
		for (int32 bldId : bldIds) 
		{
			Building& bld = _simulation->building(bldId);
			if (!bld.isConstructed())
			{
				if (bld.buildingAge() > Time::TicksPerYear &&
					!_simulation->buildingSystem().IsQuickBuild(bldId))
				{
					// Quick Build
					_simulation->buildingSystem().AddQuickBuild(bldId);
					bld.InstantClearArea();
					bld.FinishConstructionResourceAndWorkerReset();
					bld.SetAreaWalkable();
				}

				return false;
			}
			// too recent, return false
			if (bld.buildingAge() <= gapTicks) {
				return false;
			}
		}
		
		return true;
	};
	
	/*
	 * Blocks that are placed next to some other block (city center)
	 */
	if (notTooManyHouseUnderConstruction)
	{
		SCOPE_CYCLE_COUNTER(STAT_PunAIPlaceCityBlock);

		int32 housingCapacity = _simulation->HousingCapacity(_aiPlayerId);
		const int32 maxHouseCapacity = 99999;// 4 * 50; // 120 max pop

		// TODO: add last building centerTile which can be used as a new spiral center

		AICityBlock block;
		
		// First Trading Post
		if (Time::Ticks() > Time::TicksPerYear &&
			_simulation->buildingCount(_aiPlayerId, CardEnum::TradingPost) == 0)
		{
			block = AICityBlock::MakeBlock(factionEnum(),
				{ CardEnum::TradingPost }
			);
		}
		// House
		else if (housingCapacity < maxHouseCapacity && population > housingCapacity - 5)
		{
			// Group 4 houses into a block...
			block = AICityBlock::MakeBlock(factionEnum(),
				{ CardEnum::House, CardEnum::House },
				{ CardEnum::House, CardEnum::House }
			);
		}
		// Tavern (One for every 50 citizens)
		else if (population / 50 + 1 > _simulation->buildingCount(_aiPlayerId, CardEnum::Tavern))
		{
			block = AICityBlock::MakeBlock(factionEnum(),
				{ CardEnum::Tavern },
				{ }
			);
		}
		else
		{
			// Industry
			if (Time::Ticks() > Time::TicksPerSeason * 3 / 2) // build only after food producers
			{
				/*
				 * Old Way
				 */
				// Furniture Workshop
				if (_simulation->resourceCountTown(_aiPlayerId, ResourceEnum::Wood) > 200 &&
					_simulation->buildingCount(_aiPlayerId, CardEnum::FurnitureWorkshop) == 0)
				{
					block = AICityBlock::MakeBlock(factionEnum(),
						{ CardEnum::FurnitureWorkshop },
						{ }
					);
				}
				// Beer Brewery
				else if (_simulation->resourceCountTown(_aiPlayerId, ResourceEnum::Wheat) > 100 &&
					_simulation->buildingCount(_aiPlayerId, CardEnum::BeerBrewery) == 0)
				{
					block = AICityBlock::MakeBlock(factionEnum(),
						{ CardEnum::BeerBrewery, },
						{ }
					);
				}
				//// Tailor
				//else if (_simulation->buildingCount(_aiPlayerId, CardEnum::FurnitureWorkshop) > 0 &&
				//	_simulation->buildingCount(_aiPlayerId, CardEnum::BeerBrewery) > 0 &&
				//	_simulation->buildingCount(_aiPlayerId, CardEnum::Tailor) == 0)
				//{
				//	block = AICityBlock::MakeBlock(factionEnum(),
				//		{ CardEnum::Tailor, },
				//		{ }
				//	);
				//}

				//! Build buildings that should scale with population
				TryPlaceNormalBuildings_ScaleToPopCount(block, era, population);

				//! build single building of each type
				TryPlaceNormalBuildings_Single(block, era);
			}

			// Storage
			if (!block.IsValid())
			{
				std::pair<int32, int32> slotsPair = _simulation->GetStorageCapacity(_aiPlayerId, true);
				int32 usedSlots = slotsPair.first;
				int32 totalSlots = slotsPair.second;

				if (usedSlots >= totalSlots * 9 / 10) // 90% full
				{
					block = AICityBlock::MakeBlock(factionEnum(),
						{ storageEnum, storageEnum, },
						{ storageEnum,  storageEnum }
					);
				}
			}
		}


		if (block.IsValid())
		{
			// Try house, then townhall, then storage
			TryPlaceCityBlock({ CardEnum::House, CardEnum::Townhall, storageEnum }, block);
			//TryPlaceCityBlock(CardEnum::House, block);

			//if (!block.PlacementSucceed()) {
			//	TryPlaceCityBlock(CardEnum::Townhall, block);
			//}

			//if (!block.HasArea()) {
			//	TryPlaceCityBlock(storageEnum, block);
			//}
		}
	}


	/*
	 * Coastal/Mountain/River buildings
	 */
	if (_needMoreTradeBuilding &&
		(hasNoRecentUnderConstruction(CardEnum::TradingPost) && hasNoRecentUnderConstruction(CardEnum::TradingPort)))
	{
		bool isPlaced = TryPlaceSingleCoastalBuilding(CardEnum::TradingPort);
		if (!isPlaced) {
			auto block = AICityBlock::MakeBlock(factionEnum(),
				{ CardEnum::TradingPost }
			);
			TryPlaceCityBlock({ storageEnum, CardEnum::House }, block);
		}
	}

	/*
	 * Mines
	 */
	TryPlaceMines();
	

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
								BuildPlacement placement(coastalTile, GetBuildingInfo(CardEnum::Fisher).GetSize(factionEnum()), faceDirection);

								std::vector<PlacementGridInfo> grids;
								bool setDockInstruction;
								_simulation->CheckPortArea(placement, CardEnum::Fisher, grids, setDockInstruction, _aiPlayerId);

								if (AreGridsBuildable(grids))
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
						PlaceBuilding(CardEnum::Fisher, bestCenterTile, bestFaceDirection);
					}

					return false;
				};

				bool placedFisher = tryPlaceFisher();

				if (!placedFisher)
				{
					AICityBlock block = AICityBlock::MakeBlock({}, { CardEnum::RanchPig });
					TryPlaceCityBlock(CardEnum::RanchPig, block);
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
						AICityBlock block = AICityBlock::MakeBlock(factionEnum(),
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
		tryUpgrade(60, 2);
		tryUpgrade(110, 3);
		tryUpgrade(210, 4);
	}

	/*
	 * Cheat Money
	 */
	if (Time::Seconds() % 5 == 0 &&
		_simulation->moneyCap32(_aiPlayerId) < 700)
	{
		// Cheat if no money
		int32 cheatAmount = std::max(333, population * 10);
		_simulation->ChangeMoney(_aiPlayerId, cheatAmount);
		_totalCheatedMoney += cheatAmount;
	}

	/*
	 * Upgrade Buildings
	 */
	TryUpgradeBuildings();

	/*
	 * Trade
	 */
	TryTrade();

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
	std::vector<std::vector<CardStatus>> eraToUnitCountPer100pop
	{
		{},
		{ // Era 1
			CardStatus(CardEnum::Warrior, 1)
		},
		{ // Era 2
			CardStatus(CardEnum::Swordsman, 1),
			CardStatus(CardEnum::Archer, 1)
		},
		{ // Era 3
			CardStatus(CardEnum::Musketeer, 1),
			CardStatus(CardEnum::Cannon, 1)
		},
		{ // Era 4
			CardStatus(CardEnum::Infantry, 1),
			CardStatus(CardEnum::MachineGun, 1)
		},
	};


	// Always Remove any slot cards
	// - These slot cards might be unwanted cards from demolition (AI slot cards cheat added)
	auto& cardSys = _simulation->cardSystem(_aiPlayerId);
	std::vector<CardStatus> cards = cardSys.GetCardsBoughtAndInventory();
	for (const CardStatus& card : cards) {
		if (IsBuildingSlotCard(card.cardEnum)) {
			cardSys.RemoveCards_BoughtHandAndInventory(CardStatus(card.cardEnum, 1));
		}
	}
	

	/*
	 * Military
	 */
	if (Time::Seconds() % 250 == 0 &&
		Time::Ticks() > Time::TicksPerYear)
	{
		const std::vector<CardStatus>& targetCards = eraToUnitCountPer100pop[era];

		// Remove a military card if it isn't wanted for this era
		for (const CardStatus& card : cards) 
		{
			if (IsMilitaryCardEnum(card.cardEnum))
			{
				bool isTargetCard = false;
				for (const CardStatus& targetCard : targetCards) {
					if (card.cardEnum == targetCard.cardEnum) {
						isTargetCard = true;
					}
				}

				if (!isTargetCard) {
					cardSys.RemoveCards_BoughtHandAndInventory(CardStatus(card.cardEnum, 1));
				}
			}
		}

		// Add card that needs to increase
		for (const CardStatus& targetCard : targetCards)
		{
			bool needCardAdding = true;
			for (const CardStatus& card : cards) {
				if (card.cardEnum == targetCard.cardEnum) {
					if (card.stackSize >= targetCard.stackSize) {
						needCardAdding = false;
					}
					break;
				}
			}
			if (needCardAdding) {
				cardSys.TryAddCards_BoughtHandAndInventory(targetCard.cardEnum);
			}
		}
	}
	
	 //_LOG(PunAI, "%s CheckDefendLand(outer): sec:%d money:%d", AIPrintPrefix(), Time::Seconds(), resourceSystem.money());
	if (Time::Seconds() % 10 == 0)
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
	TryProvinceClaim();
}


void AIPlayerSystem::TryChooseLocation()
{
	auto& provinceSys = _simulation->provinceSystem();
	
	for (int count = 10000; count-- > 0;)
	{
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
	
}

void AIPlayerSystem::TryBuildTownhall()
{
	_LOG(PunAI, "%s Try Build Townhall", AIPrintPrefix());

	auto& provinceSys = _simulation->provinceSystem();
	const std::vector<int32>& provincesClaimed = _simulation->GetProvincesPlayer(_aiPlayerId);

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
}

void AIPlayerSystem::Tick1Sec_DoAction()
{
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
			int32 relationship = _simulation->townManagerBase(_aiPlayerId)->relationship().GetTotalRelationship(playerId);
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
			command->intVar1 = _aiPlayerId;
			command->intVar2 = maxRelationshipPlayerId;
			command->intVar3 = std::min(100, _simulation->moneyCap32(_aiPlayerId) - 500);

			command->intVar5 = static_cast<int32>(TradeDealStageEnum::Gifting);


			check(_simulation->IsValidPlayer(maxRelationshipPlayerId));

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
}

void AIPlayerSystem::TryUpgradeBuildings()
{
	if (Time::Seconds() % 20 == 0)
	{
		std::vector<CardEnum> buildingEnums = GetSortedBuildingEnum();
		for (CardEnum buildingEnum : buildingEnums)
		{
			const std::vector<int32>& bldIds = _simulation->buildingIds(_aiPlayerId, buildingEnum);
			for (int32 bldId : bldIds)
			{
				Building& bld = _simulation->building(bldId);

				//! Upgrade
				int32 upgradeCount = bld.upgrades().size();
				for (int32 i = 0; i < upgradeCount; i++)
				{
					const BuildingUpgrade& upgrade = bld.GetUpgrade(i);
					if (upgrade.isEraUpgrade())
					{
						int32 era = _simulation->GetEra(_aiPlayerId);
						
						// Upgrade according to 
						if (upgrade.upgradeLevel < era - upgrade.startEra) 
						{
							bld.UpgradeLevelInstantly(i);
							return; // Only one era upgrade per try
						}
					}
					else {
						// Upgrade instantly if this isn't era upgrade
						if (!bld.IsUpgraded(i)) {
							bld.UpgradeInstantly(i);
						}
					}
				}


				//! Add Slot cards to buildings without one that needs one
				if (GetBuildingInfo(buildingEnum).hasInput1() &&
					bld.slotCardCount(CardEnum::SustainabilityBook) == 0)
				{
					bld.AddSlotCard(CardStatus(CardEnum::SustainabilityBook, 1));
				}
			}
		}
	}
}

void AIPlayerSystem::TryTrade()
{
	if (Time::Ticks() % Time::TicksPerRound == 0)
	{
		SCOPE_CYCLE_COUNTER(STAT_PunAITrade);

		TownManager& townManage = _simulation->townManager(_aiPlayerId);
		int32 population = townManage.population();

		std::vector<int32> tradeBldIds = _simulation->buildingIds(_aiPlayerId, CardEnum::TradingPost);
		const std::vector<int32>& tradingPortIds = _simulation->buildingIds(_aiPlayerId, CardEnum::TradingPort);
		tradeBldIds.insert(tradeBldIds.end(), tradingPortIds.begin(), tradingPortIds.end());
		
		if (tradeBldIds.size() > 0)
		{
			std::vector<int32> sellTargetAmounts(ResourceEnumCount, 0);
			std::vector<int32> buyTargetAmounts(ResourceEnumCount, 0);

			// Sell extra luxury
			auto sellExcess = [&](ResourceEnum resourceEnum, int32 supplyPerPop)
			{
				sellTargetAmounts[static_cast<int>(resourceEnum)] = population * supplyPerPop;
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
				buyTargetAmounts[static_cast<int>(resourceEnum)] = std::max(buyTargetAmounts[static_cast<int>(resourceEnum)], target);
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


			//! Buy Raw materials
			std::vector<CardEnum> buildingEnums = GetSortedBuildingEnum();
			for (CardEnum buildingEnum : buildingEnums)
			{
				//! Buy Construction Materials
				const std::vector<int32>& bldIds = _simulation->buildingIds(_aiPlayerId, buildingEnum);
				for (int32 buildingId : bldIds) 
				{
					Building& bld = _simulation->building(buildingId);
					if (!bld.isConstructed()) 
					{
						std::vector<int32> constructionCosts = bld.GetConstructionResourceCost();
						for (size_t i = 0; i < _countof(ConstructionResources); i++) {
							if (constructionCosts[i] > 0) {
								int32 neededResourceCount = constructionCosts[i] - bld.GetResourceCount(ConstructionResources[i]);
								if (neededResourceCount > 0) {
									buyUntil(ConstructionResources[i], neededResourceCount);
								}
							}
						}
					}
				}


				//! Buy Inputs
				if (bldIds.size() > 0)
				{
					const BldInfo& info = GetBuildingInfo(buildingEnum);
					if (info.hasInput1()) {
						buyUntil(info.input1, 50 + (50 * bldIds.size()));
					}
					if (info.hasInput2()) {
						buyUntil(info.input2, 50 + (50 * bldIds.size()));
					}
				}
			}

			//! Sell Excess
			for (int32 i = 0; i < ResourceEnumCount; i++)
			{
				// Sell Excess of food at higher supply per pop
				ResourceEnum resourceEnum = static_cast<ResourceEnum>(i);
				if (IsFoodEnum(resourceEnum)) {
					sellExcess(resourceEnum, 50);
				}

				// Sell Excess of everything else
				if (sellTargetAmounts[i] == 0) {
					sellExcess(static_cast<ResourceEnum>(i), 5);
				}
			}


			/*
			 * Distribute to different trading posts
			 */
			// If sell target is below buy target, move it up to buy target
			for (int32 i = 0; i < sellTargetAmounts.size(); i++) {
				if (sellTargetAmounts[i] < buyTargetAmounts[i]) {
					sellTargetAmounts[i] = buyTargetAmounts[i];
				}
			}
			
			std::vector<int32> buyAmountPreferred(ResourceEnumCount, 0);
			for (int32 i = 0; i < buyAmountPreferred.size(); i++) {
				ResourceEnum resourceEnum = static_cast<ResourceEnum>(i);
				int32 resourceCount = _simulation->resourceCountTown(_aiPlayerId, resourceEnum);
				if (resourceCount < buyTargetAmounts[i]) {
					buyAmountPreferred[i] = buyTargetAmounts[i] - resourceCount;
				}
				else if (resourceCount > sellTargetAmounts[i]) {
					buyAmountPreferred[i] = sellTargetAmounts[i] - resourceCount;
				}
			}
			
			auto tradeCommand = make_shared<FTradeResource>();
			tradeCommand->playerId = _aiPlayerId;

			for (int32 tradeBldId : tradeBldIds)
			{
				TradeBuilding& tradeBld = _simulation->building<TradeBuilding>(tradeBldId);
				int32 amountLeft = tradeBld.maxTradeQuatity();

				tradeCommand->buyEnums.Empty();
				tradeCommand->buyAmounts.Empty();

				// Prioritize Buy
				for (int32 i = 0; i < buyAmountPreferred.size(); i++) 
				{
					if (amountLeft <= 0) {
						break;
					}
					
					if (buyAmountPreferred[i] > 0) {
						int32 amountToBuy = std::min(buyAmountPreferred[i], amountLeft);
						buyAmountPreferred[i] -= amountToBuy;
						amountLeft -= amountToBuy;
						tradeCommand->buyEnums.Add(i);
						tradeCommand->buyAmounts.Add(amountToBuy);
					}
				}
				// Then sell
				for (int32 i = 0; i < buyAmountPreferred.size(); i++)
				{
					if (amountLeft <= 0) {
						break;
					}

					if (buyAmountPreferred[i] < 0) {
						int32 amountToSell = std::min(-buyAmountPreferred[i], amountLeft);
						buyAmountPreferred[i] += amountToSell;
						amountLeft -= amountToSell;
						tradeCommand->buyEnums.Add(i);
						tradeCommand->buyAmounts.Add(amountToSell);
					}
				}


				if (tradeCommand->buyEnums.Num() > 0)
				{
					if (_simulation->moneyCap32(_aiPlayerId) < 300)
					{
						// Sell only
						for (int32 i = tradeCommand->buyAmounts.Num(); i-- > 0;) {
							if (tradeCommand->buyAmounts[i] > 0) {
								tradeCommand->buyEnums.RemoveAt(i);
								tradeCommand->buyAmounts.RemoveAt(i);
							}
						}
					}

					tradeCommand->objectId = tradeBldId;


					_LOG(PunAI, "%s Trade:", AIPrintPrefix());
					for (int32 i = 0; i < tradeCommand->buyEnums.Num(); i++) {
						_LOG(PunAI, "%s : %s %d", AIPrintPrefix(), ToTChar(ResourceName(static_cast<ResourceEnum>(tradeCommand->buyEnums[i]))), tradeCommand->buyAmounts[i]);
					}

					_simulation->ExecuteNetworkCommand(tradeCommand);
				}
			}

			_needMoreTradeBuilding = false;
			for (int32 i = 0; i < buyAmountPreferred.size(); i++) {
				if (buyAmountPreferred[i] > 0) {
					_needMoreTradeBuilding = true;
				}
			}
			
		}
	}
}

void AIPlayerSystem::TryProvinceClaim()
{
	auto& provinceSys = _simulation->provinceSystem();
	const std::vector<int32>& provincesClaimed = _simulation->GetProvincesPlayer(_aiPlayerId);
	auto& globalResourceSys = _simulation->globalResourceSystem(_aiPlayerId);

	TownHall& townhall = _simulation->GetTownhallCapital(_aiPlayerId);
	int32 population = _simulation->townManager(_aiPlayerId).population();
	
	
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


void AIPlayerSystem::TryPlaceCityBlock(CardEnum buildingEnumToBeNear, AICityBlock& block)
{
	const std::vector<int32>& buildingIds = _simulation->buildingIds(_aiPlayerId, buildingEnumToBeNear);
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

void AIPlayerSystem::PlaceBuilding(CardEnum cardEnum, WorldTile2 centerTile, Direction faceDirection)
{
	auto command = MakeCommand<FPlaceBuilding>();
	command->playerId = _aiPlayerId;
	command->buildingEnum = static_cast<uint8>(cardEnum);
	command->area = BuildingArea(centerTile, GetBuildingInfo(cardEnum).GetSize(factionEnum()), faceDirection);
	command->center = centerTile;
	command->faceDirection = static_cast<uint8>(faceDirection);

	_simulation->ExecuteNetworkCommand(command);
}

bool AIPlayerSystem::TryPlaceSingleCoastalBuilding(CardEnum cardEnum)
{
	const std::vector<int32>& provincesClaimed = _simulation->GetProvincesPlayer(_aiPlayerId);
	
	for (int32 provinceClaimed : provincesClaimed)
	{
		const std::vector<WorldTile2>& coastalTiles = _simulation->provinceInfoSystem().provinceBuildingSlot(provinceClaimed).coastalTiles;

		for (const WorldTile2& coastalTile : coastalTiles)
		{
			for (int32 i = 0; i < DirectionCount; i++)
			{
				Direction faceDirection = static_cast<Direction>(i);
				BuildPlacement placement(coastalTile, GetBuildingInfo(cardEnum).GetSize(factionEnum()), faceDirection);

				std::vector<PlacementGridInfo> grids;
				bool setDockInstruction;
				_simulation->CheckPortArea(placement, cardEnum, grids, setDockInstruction, _aiPlayerId);

				if (AreGridsBuildable(grids))
				{
					PlaceBuilding(cardEnum, placement.centerTile, placement.faceDirection);
					return true;
				}
			}
		}
	}
	
	return false;
}

void AIPlayerSystem::TryPlaceMines()
{
	if (Time::Ticks() % Time::TicksPerRound != 0) {
		return; 
	}
	
	const std::vector<int32>& provincesClaimed = _simulation->GetProvincesPlayer(_aiPlayerId);

	for (int32 provinceClaimed : provincesClaimed)
	{
		CardEnum cardEnum = CardEnum::None;

		auto tryBuildOne = [&](CardEnum cardEnumIn)
		{
			if (_simulation->buildingCount(_aiPlayerId, cardEnumIn) == 0) {
				cardEnum = cardEnumIn;
			}
		};

		GeoresourceNode node = _simulation->georesourceSystem().georesourceNode(provinceClaimed);
		if (node.info().isMountainOre()) {
			if (node.georesourceEnum == GeoresourceEnum::CoalOre) {
				tryBuildOne(CardEnum::CoalMine);
			}
			else if (node.georesourceEnum == GeoresourceEnum::IronOre) {
				tryBuildOne(CardEnum::IronMine);
			}
			else if (node.georesourceEnum == GeoresourceEnum::GoldOre) {
				tryBuildOne(CardEnum::GoldMine);
			}
			else if (node.georesourceEnum == GeoresourceEnum::Gemstone) {
				tryBuildOne(CardEnum::GemstoneMine);
			}
		}

		if (_simulation->buildingCount(_aiPlayerId, CardEnum::Quarry) == 0) {
			tryBuildOne(CardEnum::Quarry);
		}

		if (cardEnum != CardEnum::None)
		{
			const std::vector<WorldTile2>& mountainTiles = _simulation->provinceInfoSystem().provinceBuildingSlot(provinceClaimed).mountainTiles;

			for (const WorldTile2& mountainTile : mountainTiles)
			{
				for (int32 i = 0; i < DirectionCount; i++)
				{
					Direction faceDirection = static_cast<Direction>(i);
					BuildPlacement placement(mountainTile, GetBuildingInfo(cardEnum).GetSize(factionEnum()), faceDirection);

					if (_simulation->CanBuildMountainMineArea(placement, cardEnum, _aiPlayerId)) {
						PlaceBuilding(cardEnum, placement.centerTile, placement.faceDirection);
						return; // Build only one per check
					}
				}
			}
		}
	}
}

void AIPlayerSystem::TryPlaceNormalBuildings_ScaleToPopCount(AICityBlock& block, int32 era, int32 population)
{
	int32 basePopCountPerBuilding = 40;
	int32 baseBuildingPrice = GetBuildingInfo(CardEnum::FurnitureWorkshop).constructionCostAsMoney();

	auto luxBuilding = [&](CardEnum buildingEnum)
	{
		int32 buildingPrice = GetBuildingInfo(buildingEnum).constructionCostAsMoney();

		return std::pair<CardEnum, int32>(buildingEnum, basePopCountPerBuilding * buildingPrice / baseBuildingPrice);
	};

	const std::vector<std::vector<std::pair<CardEnum, int32>>> eraToBuildingEnumToPopCountPerTarget
	{
		{},
		// Era 1
		{
			luxBuilding(CardEnum::FurnitureWorkshop),
			luxBuilding(CardEnum::BeerBrewery),
			//luxBuilding(CardEnum::Potter),

			{ CardEnum::Tavern, 60 },
		},
		// Era 2
		{
			{ CardEnum::Bakery, 40 },
			luxBuilding(CardEnum::Tailor),
			luxBuilding(CardEnum::Winery),
		},
		// Era 3
		{
			luxBuilding(CardEnum::CoffeeRoaster),
			luxBuilding(CardEnum::Chocolatier),
		},
		// Era 4
		{
			luxBuilding(CardEnum::Jeweler),
			luxBuilding(CardEnum::ClockMakers),
		},
	};


	if (!block.IsValid())
	{
		if (_simulation->buildingCount(_aiPlayerId, CardEnum::TradingPort) ||
			_simulation->buildingCount(_aiPlayerId, CardEnum::TradingPost))
		{
			for (int32 tier = 1; tier <= era; tier++)
			{
				const std::vector<std::pair<CardEnum, int32>>& buildingEnumToPopCountPerTarget = eraToBuildingEnumToPopCountPerTarget[tier];
				for (const std::pair<CardEnum, int32>& pair : buildingEnumToPopCountPerTarget) 
				{
					int32 targetCount = 1 + population / pair.second;
					if (_simulation->buildingCount(_aiPlayerId, pair.first) < targetCount)
					{
						block = AICityBlock::MakeBlock(factionEnum(),
							{ pair.first, },
							{ }
						);
						return;
					}
				}
			}
		}
	}
}

void AIPlayerSystem::TryPlaceNormalBuildings_Single(AICityBlock& block, int32 era)
{
	const std::vector<std::vector<CardEnum>> eraToSingleBuildingEnum
	{
		{},
		// Era 1
		{},
		// Era 2
		{
			CardEnum::Library,
			CardEnum::SpyCenter,
			CardEnum::Brickworks,
			CardEnum::Market,
		},
		// Era 3
		{
			CardEnum::School,
			CardEnum::PolicyOffice,
			CardEnum::Glassworks,
			CardEnum::Theatre,
			CardEnum::Bank,
			CardEnum::Museum,
			CardEnum::Zoo,
		},
		// Era 4
		{
			CardEnum::ConcreteFactory,
			CardEnum::CardCombiner,
		},

	};
	
	if (!block.IsValid())
	{
		for (int32 tier = 1; tier <= era; tier++)
		{
			const std::vector<CardEnum>& buildingEnums = eraToSingleBuildingEnum[tier];
			for (CardEnum buildingEnum : buildingEnums)
			{
				if (_simulation->buildingCount(_aiPlayerId, buildingEnum) == 0)
				{
					block = AICityBlock::MakeBlock(factionEnum(),
						{ buildingEnum, },
						{ }
					);
					return;
				}
			}
		}
	}
}