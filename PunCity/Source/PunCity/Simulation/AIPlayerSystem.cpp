// Pun Dumnernchanvanit's


#include "AIPlayerSystem.h"

#include "BuildingCardSystem.h"
#include "UnlockSystem.h"

static const int32 MaxFoodPerPopulation = 10;
static const int32 MaxNonFoodPerPopulation = 5;

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

			_LOG(PunAI, "Townhall MarkArea _aiPlayerId:%d area:%s removing(None)", _aiPlayerId, *provinceSys.GetProvinceRectArea(status.provinceId).ToFstring());
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

	int32 aiTier = 0;
	for (int32 i = PopToIncreaseEra.size(); i-- > 0;) {
		if (population >= PopToIncreaseEra[i]) {
			aiTier = i;
			break;
		}
	}
	int32 targetEra = aiTier / 2 + 1;
	

	auto unlockTech = [&](TechEnum techEnum)
	{
		auto unlockSys = _simulation->unlockSystem(_aiPlayerId);
		std::shared_ptr<ResearchInfo> tech = unlockSys->GetTechInfo(techEnum);
		tech->state = TechStateEnum::Researched;
		tech->upgradeCount++;
		unlockSys->techsFinished++;

		_LOG(PunAI, "unlockTech _aiPlayerId:%d techEnum:%d", _aiPlayerId, techEnum);
	};

	if (era < targetEra) 
	{
		if (era == 1) {
			unlockTech(TechEnum::MiddleAge);
		}
		else if (era == 2) {
			unlockTech(TechEnum::EnlightenmentAge);
			unlockTech(TechEnum::Logistics5);
		}
		else if (era == 3) {
			unlockTech(TechEnum::IndustrialAge);
		}
	}

	CardEnum storageEnum = (era == 2) ? CardEnum::Warehouse : CardEnum::StorageYard;


	/*
	 * 
	 */
	int32 targetLaborerFromPop = population / 5;
	if (std::abs(townManage.targetLaborerCount - targetLaborerFromPop) > 5) // around 1/5 ppl as laborer
	{
		std::shared_ptr<FSetTownPriority> command = townManage.CreateTownPriorityCommand();
		command->playerId = _aiPlayerId;
		command->townId = _aiPlayerId;
		command->targetLaborerCount = targetLaborerFromPop;
		_simulation->ExecuteNetworkCommand(command);
	}
	

	/*
	 * Place City Blocks
	 */
	
	auto hasNoRecentUnderConstruction = [&](CardEnum cardEnum, int32 gapTicks = Time::TicksPerSeason)
	{
		const std::vector<int32>& bldIds = _simulation->buildingIds(_aiPlayerId, cardEnum);
		for (int32 bldId : bldIds) 
		{
			Building& bld = _simulation->building(bldId);
			if (!bld.isConstructed()) {
				return false;
			}
			// too recent, return false
			if (bld.buildingAge() <= gapTicks) {
				return false;
			}
		}
		
		return true;
	};

	// Should QuickBuild
	auto tryQuickBuild = [&](CardEnum cardEnum)
	{
		const std::vector<int32>& bldIds = _simulation->buildingIds(_aiPlayerId, cardEnum);
		for (int32 bldId : bldIds)
		{
			Building& bld = _simulation->building(bldId);
			if (!bld.isConstructed())
			{
				bool shouldQuickBuild = (bld.buildingAge() > Time::TicksPerYear);
				if (IsStorage(cardEnum) ||
					IsTradingPostLike(cardEnum))
				{
					if (bld.buildingAge() > Time::TicksPerRound) {
						shouldQuickBuild = true;
					}
				}

				if (shouldQuickBuild &&
					!_simulation->buildingSystem().IsQuickBuild(bldId))
				{
					// Quick Build
					_simulation->buildingSystem().AddQuickBuild(bldId);
					bld.InstantClearArea();
					bld.FinishConstructionResourceAndWorkerReset();
					bld.SetAreaWalkable();

					_LOG(PunAI, "QuickBuild _aiPlayerId:%d bldId:%d", _aiPlayerId, bldId);
				}
			}
		}
	};

	std::vector<CardEnum> buildingEnums = GetBuildingEnums();
	tryQuickBuild(buildingEnums[Time::Seconds() % buildingEnums.size()]);


	bool shouldPlaceJobBuilding = (population * 8 / 10 > jobSlots);
	
	/*
	 * Blocks that are placed next to some other block (city center)
	 */
	if (notTooManyHouseUnderConstruction)
	{
		SCOPE_CYCLE_COUNTER(STAT_PunAIPlaceCityBlock);

		int32 housingCapacity = _simulation->HousingCapacity(_aiPlayerId);
		const int32 maxHouseCapacity = 99999;// 4 * 50; // 120 max pop

		std::pair<int32, int32> slotsPair = _simulation->GetStorageCapacity(_aiPlayerId, true);
		int32 usedSlots = slotsPair.first;
		int32 totalSlots = slotsPair.second;
		bool needMoreStorage = (usedSlots >= totalSlots * 8 / 10); // 80% full

		// TODO: add last building centerTile which can be used as a new spiral center

		AICityBlock block;
		
		// First Trading Post
		if (Time::Ticks() > Time::TicksPerYear / 2 &&
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
		else
		{
			// Try Build Storage
			if (!block.IsValid())
			{
				if (needMoreStorage)
				{
					block = AICityBlock::MakeBlock(factionEnum(),
						{ storageEnum, storageEnum, },
						{ storageEnum,  storageEnum }
					);
				}
			}

			
			/*
			 * Industries
			 */
			if (Time::Ticks() > Time::TicksPerSeason * 3 / 2) // build only after food producers
			{
				//! Build buildings that should scale with population
				TryPlaceNormalBuildings_ScaleToPopCount(block, aiTier, population, shouldPlaceJobBuilding);

				//! build single building of each type
				TryPlaceNormalBuildings_Single(block, aiTier, shouldPlaceJobBuilding);
			}
		}


		if (block.IsValid())
		{
			// Try house, then townhall, then storage
			std::vector<CardEnum> buildingEnumToBeNear = { CardEnum::House, CardEnum::Townhall, CardEnum::StorageYard, CardEnum::Warehouse };

			// whether to put near house depends on if it is a bad appeal building
			if (block.HasLowAppealBuilding()) {
				buildingEnumToBeNear = { CardEnum::StorageYard, CardEnum::Warehouse, CardEnum::Townhall, CardEnum::House };
			}
			

			if (block.HasBuildingEnum(CardEnum::HaulingServices)) {
				buildingEnumToBeNear = { CardEnum::TradingPort, CardEnum::TradingPost, CardEnum::House, CardEnum::Townhall, storageEnum };
			}
			
			TryPlaceCityBlock(buildingEnumToBeNear, block);

			// Placement failed, try to put a new storage cluster in empty province
			if (!block.PlacementSucceed())
			{
				for (const AIRegionStatus& regionStatus : _regionStatuses)
				{
					if (regionStatus.currentPurpose == AIRegionPurposeEnum::None ||
						regionStatus.currentPurpose == AIRegionPurposeEnum::City)
					{
						if (needMoreStorage)
						{
							block = AICityBlock::MakeBlock(factionEnum(),
								{ CardEnum::StorageYard, CardEnum::StorageYard },
								{ CardEnum::StorageYard, CardEnum::StorageYard }
							);
						}
						else
						{
							block = AICityBlock::MakeBlock(factionEnum(),
								{ CardEnum::House, CardEnum::House },
								{ CardEnum::House, CardEnum::House }
							);
						}
						
						WorldTile2 center = provinceSys.GetProvinceCenterTile(regionStatus.provinceId);
						block.TryPlaceCityBlockSpiral(center, _aiPlayerId, _simulation);

						if (block.HasArea()) {
							std::vector<std::shared_ptr<FNetworkCommand>> commands;
							SimUtils::PlaceCityBlock(block, _aiPlayerId, commands, _simulation);
							_simulation->ExecuteNetworkCommands(commands);
							break;
						}
					}
				}
			}
		}
	}


	/*
	 * Granary
	 */
	if (era >= 2 &&
		Time::Seconds() % (Time::SecondsPerRound * 2) == 0) // Quickbuild should be done before we check this next time
	{
		const std::vector<int32>& farmIds = _simulation->buildingIds(_aiPlayerId, CardEnum::Farm);
		for (int32 farmId : farmIds)
		{
			Building& farm = _simulation->building(farmId);

			int32 radiusBonus = farm.GetRadiusBonus(CardEnum::Granary, Windmill::Radius, [&](int32 bonus, Building& building) {
				return max(bonus, 10);
			}, true);

			if (radiusBonus == 0)
			{
				AICityBlock block = AICityBlock::MakeBlock(factionEnum(),
					{ CardEnum::Granary },
					{}
				);
				block.TryPlaceCityBlockSpiral(farm.centerTile(), _aiPlayerId, _simulation);

				if (block.HasArea()) {
					std::vector<std::shared_ptr<FNetworkCommand>> commands;
					SimUtils::PlaceCityBlock(block, _aiPlayerId, commands, _simulation);
					_simulation->ExecuteNetworkCommands(commands);
					break;
				}
			}
		}
	}


	/*
	 * Coastal/Mountain/River buildings
	 */
	if (_needMoreTradeBuilding &&
		(hasNoRecentUnderConstruction(CardEnum::TradingPost) && hasNoRecentUnderConstruction(CardEnum::TradingPort)))
	{
		bool isPlaced = TryPlaceSingleCoastalBuilding(CardEnum::TradingPort);
		if (isPlaced)
		{
			_needMoreTradeBuilding = false;
		}
		else {
			auto block = AICityBlock::MakeBlock(factionEnum(),
				{ CardEnum::TradingPost }
			);
			TryPlaceCityBlock({ storageEnum, CardEnum::House }, block);

			if (block.PlacementSucceed()) {
				_needMoreTradeBuilding = false;
			}
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

	int32 foodFarmWorkers = 0;
	int32 foodFarmCount = 0;
	for (int32 farmId : farmIds) {
		Farm& farm = _simulation->building<Farm>(farmId);
		if (IsFoodEnum(GetTileObjInfo(farm.currentPlantEnum).harvestResourceEnum())) {
			foodFarmCount++;
			foodFarmWorkers += farm.GetWorkerCount();
		}
	}
	int32 foodWorkerCount = foodFarmWorkers;
	foodWorkerCount += ranchIds.size() * GetBuildingInfo(CardEnum::RanchPig).workerCount;
	foodWorkerCount += fisherIds.size() * (1 + GetBuildingInfo(CardEnum::Fisher).workerCount);
	

	auto needFarmRanch = [&]()
	{
		const int32 populationFedByFruit = 10;
		int32 populationToFeed = std::max(0, population - populationFedByFruit);

		const int32 targetFoodWorkerCount = populationToFeed / 4; // food worker 1/4 of population
		
		//int32 preferredFarmRanchCount = std::max(0, (population - populationFedByFruit) / farmPerPopulation);

		return foodWorkerCount < targetFoodWorkerCount;
	};

	/*
	 * Place Farm
	 */
	if (notTooManyHouseUnderConstruction && needFarmRanch())
	{
		SCOPE_CYCLE_COUNTER(STAT_PunAIPlaceFarm);

		// Addon farms
		bool placed = false;


		/*
		 * Farms
		 */
		if (TryPlaceRiverFarm())
		{
			placed = true;
		}
		
		// initial farms (unable to do addon)
		if (!placed)
		{
			int32 regionIndexToExplore = Time::Seconds() % _regionStatuses.size();
			AIRegionStatus& status = _regionStatuses[regionIndexToExplore];

			int32 maxFertility = 0;
			WorldTile2 maxFertilityTile = GetMaxFertilityTile(status.provinceId, maxFertility);


			if (maxFertilityTile.isValid() && maxFertility > 95)
			{
				TryPlaceFarm(maxFertilityTile, 7);
			}
		}


		/*
		 * Ranch/Fisher
		 */
		if (!placed)
		{
			// Try placing ranch/fisher when farm cannot be placed
			//if (isLastProvinceToFarmCheck)
			{
				// Try to place fish first..
				auto tryPlaceFisher = [&]()
				{
					int32 bestEfficiency = 0;
					WorldTile2 bestCenterTile = WorldTile2::Invalid;
					Direction bestFaceDirection = Direction::S;

					for (int32 provinceClaimed : provincesClaimed)
					{
						const ProvinceBuildingSlot& provinceBuildingSlot = _simulation->provinceInfoSystem().provinceBuildingSlot(provinceClaimed);

						auto tryPlaceFisherOnTile = [&](const WorldTile2& tileIn)
						{
							for (int32 i = 0; i < DirectionCount; i++)
							{
								Direction faceDirection = static_cast<Direction>(i);
								BuildPlacement placement(tileIn, GetBuildingInfo(CardEnum::Fisher).GetSize(factionEnum()), faceDirection);

								std::vector<PlacementGridInfo> grids;
								bool setDockInstruction;
								_simulation->CheckPortArea(placement, CardEnum::Fisher, grids, setDockInstruction, _aiPlayerId);

								if (AreGridsBuildable(grids))
								{
									int32 efficiency = Fisher::FisherAreaEfficiency(tileIn, false, WorldTile2::Invalid, _simulation);
									if (efficiency > bestEfficiency) {
										bestEfficiency = efficiency;
										bestCenterTile = tileIn;
										bestFaceDirection = faceDirection;
									}
								}
							}
						};
						
						const std::vector<WorldTile2>& coastalTiles = provinceBuildingSlot.coastalTiles;
						for (const WorldTile2& coastalTile : coastalTiles) {
							tryPlaceFisherOnTile(coastalTile);
						}

						const std::vector<WorldTile2>& lakeTiles = provinceBuildingSlot.lakeTiles;
						for (const WorldTile2& lakeTile : lakeTiles) {
							tryPlaceFisherOnTile(lakeTile);
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

		if (shouldPlaceJobBuilding)
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

						if (block.HasArea()) 
						{
							PUN_LOG("trySetPurpose provinceId:%d currentPurpose:%d", status.provinceId, purposeEnum);
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

				// CashCrop Farm
				if (status.proposedPurpose == AIRegionProposedPurposeEnum::CashCrop)
				{
					if (status.currentPurpose == AIRegionPurposeEnum::None || 
						status.currentPurpose == AIRegionPurposeEnum::City)
					{
						if (TryPlaceFarm(provinceCenter, 7)) {
							status.currentPurpose = AIRegionPurposeEnum::CashCrop;
						}
					}
				}

				// Ranch
				trySetPurpose(AIRegionProposedPurposeEnum::Ranch,
					AIRegionPurposeEnum::Ranch,
					{ CardEnum::StorageYard, CardEnum::StorageYard, CardEnum::StorageYard, CardEnum::StorageYard },
					{ CardEnum::RanchPig }
				);


				// Forester
				if (!alreadyHaveForester)
				{
					trySetPurpose(AIRegionProposedPurposeEnum::Tree,
						AIRegionPurposeEnum::Forester,
						{ CardEnum::Forester, CardEnum::CharcoalMaker, CardEnum::StorageYard },
						{ CardEnum::HuntingLodge, CardEnum::StorageYard, CardEnum::StorageYard }
					);
				}

				// Fruit
				if (!alreadyHaveFruitCluser)
				{
					trySetPurpose(AIRegionProposedPurposeEnum::Tree,
						AIRegionPurposeEnum::FruitGather,
						{ CardEnum::FruitGatherer, CardEnum::StorageYard, CardEnum::House },
						{ CardEnum::HuntingLodge, CardEnum::StorageYard, CardEnum::StorageYard }
					);
				}

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
					if (_regionStatuses[i].proposedBlock.IsValid()) 
					{
						PUN_LOG("PlaceCityBlock_ExecuteNetworkCommands provinceId:%d currentPurpose:%d", _regionStatuses[i].provinceId, _regionStatuses[i].currentPurpose);
						
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

		auto tryUpgrade = [&](int32 targetAITier, int32 townhallLvl) {
			if (townLvl == townhallLvl &&
				aiTier >= targetAITier)
			{
				TownHall& townhal = _simulation->GetTownhallCapital(_aiPlayerId);
				int32 upgradeMoney = townhal.GetUpgradeMoney(townhallLvl + 1);
				_simulation->ChangeMoney(_aiPlayerId, upgradeMoney);

				townhal.UpgradeTownhall();
			}
		};

		tryUpgrade(1, 1);
		tryUpgrade(3, 2);
		tryUpgrade(5, 3);
		tryUpgrade(7, 4);
	}

	/*
	 * Cheat Money
	 */
	if (Time::Seconds() % 5 == 0 &&
		Time::Ticks() > _lastStolenMoneyTick + Time::TicksPerRound &&
		_simulation->moneyCap32(_aiPlayerId) < std::max(700, population * 20))
	{
		// Cheat if no money
		int32 cheatAmount = std::max(333, population * 10);
		_simulation->ChangeMoney(_aiPlayerId, cheatAmount);
		_totalCheatedMoney += cheatAmount;

		_LOG(PunAI, "CheatMoney _aiPlayerId:%d cheatAmount:%d", _aiPlayerId, cheatAmount);
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
	std::vector<std::vector<CardStatus>> eraToCitizensPerUnitCount
	{
		{},
		{ // Era 1
			CardStatus(CardEnum::Warrior, 90)
		},
		{ // Era 2
			CardStatus(CardEnum::Swordsman, 110),
			CardStatus(CardEnum::Archer, 170)
		},
		{ // Era 3
			CardStatus(CardEnum::Musketeer, 120),
			CardStatus(CardEnum::Cannon, 180)
		},
		{ // Era 4
			CardStatus(CardEnum::Infantry, 130),
			CardStatus(CardEnum::MachineGun, 190)
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
		Time::Ticks() > Time::TicksPerYear &&
		townManage.defendingClaimProgress().size() == 0)
	{
		std::vector<CardStatus> citizensPerUnitCount = eraToCitizensPerUnitCount[era];
		std::vector<CardStatus> targetCards;
		for (const CardStatus& citizenPerUnitCount : citizensPerUnitCount) {
			targetCards.push_back(CardStatus(citizenPerUnitCount.cardEnum, 1 + (population - 1) / citizenPerUnitCount.stackSize));
		}

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
	if (Time::Seconds() % 15 == 0)
	{
		//_LOG(PunAI, "%s CheckDefendLand", AIPrintPrefix());

		const std::vector<int32>& provinceIds = _simulation->GetProvincesPlayer(_aiPlayerId);
		for (int32 provinceId : provinceIds)
		{
			//_LOG(PunAI, "%s CheckDefendLand provinceId:%d", AIPrintPrefix(), provinceId);

			ProvinceClaimProgress* claimProgress = townManage.GetDefendingClaimProgressPtr(provinceId);
			if (claimProgress && claimProgress->isValid())
			{
				std::vector<CardStatus> cardsBoughtAndInventory = cardSys.GetCardsBoughtAndInventory();
				
				std::vector<CardStatus> militaryCards;
				for (const CardStatus& card : cardsBoughtAndInventory) {
					if (IsLandMilitaryCardEnum(card.cardEnum)) {
						militaryCards.push_back(card);
						cardSys.RemoveCardsFromBoughtHand(card, card.stackSize);
					}
				}
				//cardSystem(command.playerId).RemoveCardsFromBoughtHand(cardStatus, cardStatus.stackSize);

				if (militaryCards.size() > 0)
				{
					// Reinforcement Notification
					_simulation->AddPopup(claimProgress->attackerPlayerId,
						NSLOCTEXT("AIPlayerSystem", "ReinforcementDefend_NotificationToAttack", "Your enemy have reinforced their defense.")
					);

					claimProgress->Reinforce(militaryCards, false, _aiPlayerId);
				}
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
		command->area = BuildingArea(command->center, buildingInfo.baseBuildingSize, Direction::S);

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
		std::vector<CardEnum> buildingEnums = GetBuildingEnums();
		for (CardEnum buildingEnum : buildingEnums)
		{
			// Buildings to skip
			if (buildingEnum == CardEnum::HaulingServices) {
				continue;
			}
			
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
				sellTargetAmounts[static_cast<int>(resourceEnum)] = std::max(population, 20) * supplyPerPop;
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
			buyUntil(ResourceEnum::IronTools, population / 4);

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
			buyUntil(ResourceEnum::IronTools, population * 2 / 3); // 3 years = 1 tools

			int32 targetMedicine = population * 2;
			if (_simulation->GetBiomeEnum(_simulation->GetTownhallGateCapital(_aiPlayerId)) == BiomeEnum::Jungle) {
				targetMedicine *= 3;
			}
			
			if (medicineCount < targetMedicine) { // Take into account Medicinal Herb
				buyUntil(ResourceEnum::Medicine, targetMedicine);
			}

			buyUntil(ResourceEnum::Wood, 100);
			buyUntil(ResourceEnum::Stone, 100);


			//! Buy Raw materials
			std::vector<CardEnum> buildingEnums = GetBuildingEnums();
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
						buyUntil(info.input1, 80 + (80 * bldIds.size()));
					}
					if (info.hasInput2()) {
						buyUntil(info.input2, 80 + (80 * bldIds.size()));
					}
				}
			}

			//! Sell Excess
			for (int32 i = 0; i < ResourceEnumCount; i++)
			{
				// Sell Excess of food at higher supply per pop
				ResourceEnum resourceEnum = static_cast<ResourceEnum>(i);
				if (IsFoodEnum(resourceEnum)) {
					sellExcess(resourceEnum, MaxFoodPerPopulation);
					_simulation->townManager(_aiPlayerId).SetOutputTarget(resourceEnum, MaxFoodPerPopulation * 2);
				}

				// Sell Excess of everything else
				if (sellTargetAmounts[i] == 0) {
					sellExcess(resourceEnum, MaxNonFoodPerPopulation);
					_simulation->townManager(_aiPlayerId).SetOutputTarget(resourceEnum, MaxNonFoodPerPopulation * 2);
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

			for (int32 j = 0; j < tradeBldIds.size(); j++)
			{
				TradeBuilding& tradeBld = _simulation->building<TradeBuilding>(tradeBldIds[j]);
				int32 amountLeft = tradeBld.maxTradeQuatity();

				tradeCommand->buyEnums.Empty();
				tradeCommand->buyAmounts.Empty();


				auto buyAdd = [&]()
				{
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
				};
				auto sellAdd = [&]()
				{
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
							tradeCommand->buyAmounts.Add(-amountToSell);
						}
					}
				};

				// Prioritize buy mostly, except second post
				if (j == 1) {
					sellAdd();
				}
				else {
					buyAdd();
					sellAdd();
				}
				

				if (tradeCommand->buyEnums.Num() > 0)
				{
					//! Cheat add money
					int32 moneyNeeded100 = 0;
					WorldTradeSystem& worldTradeSys = _simulation->worldTradeSystem();
					for (int32 i = tradeCommand->buyAmounts.Num(); i-- > 0;) {
						if (tradeCommand->buyAmounts[i] > 0) {
							moneyNeeded100 += tradeCommand->buyAmounts[i] * worldTradeSys.price100(static_cast<ResourceEnum>(tradeCommand->buyEnums[i]));
						}
					}
					
					if (Time::Ticks() > _lastStolenMoneyTick + Time::TicksPerRound &&
						_simulation->moneyCap32(_aiPlayerId) < moneyNeeded100 / 100 + 10)
					{
						// Cheat if no money
						int32 cheatAmount = moneyNeeded100 / 100 + 10;
						_simulation->ChangeMoney(_aiPlayerId, cheatAmount);
						_totalCheatedMoney += cheatAmount;
						
						//// Sell only
						//for (int32 i = tradeCommand->buyAmounts.Num(); i-- > 0;) {
						//	if (tradeCommand->buyAmounts[i] > 0) {
						//		tradeCommand->buyEnums.RemoveAt(i);
						//		tradeCommand->buyAmounts.RemoveAt(i);
						//	}
						//}
					}

					tradeCommand->objectId = tradeBldIds[j];


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
	
	if (provincesClaimed.size() < 3 ||
		Time::Seconds() % Time::SecondsPerRound == 0)
	{	
		auto& globalResourceSys = _simulation->globalResourceSystem(_aiPlayerId);

		TownHall& townhall = _simulation->GetTownhallCapital(_aiPlayerId);
		int32 population = _simulation->townManager(_aiPlayerId).population();


		int32 preferredProvinceCount = std::max(5, population / 8);

		if (provincesClaimed.size() < preferredProvinceCount)
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

					//AIRegionProposedPurposeEnum purpose = DetermineProvincePurpose(connection.provinceId);

					//if (purpose != AIRegionProposedPurposeEnum::None)
					//{
					int32 points = flatLandPoints + treePoints - distancePoints;

					if (points > bestClaimPoints) {
						bestClaimProvinceId = connection.provinceId;
						bestClaimPoints = points;
					}
					//}
				}
			};

			for (int32 provinceId : provincesClaimed) {
				provinceSys.ExecuteAdjacentProvinces(provinceId, checkAdjacent);
			}

			if (bestClaimProvinceId != -1)
			{
				int32 regionPrice = _simulation->GetProvinceClaimPrice(bestClaimProvinceId, _aiPlayerId);

				if (globalResourceSys.moneyCap32() < 0) {
					globalResourceSys.SetMoney(0);
				}
				globalResourceSys.ChangeMoney(regionPrice);
				
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

	// Ensure center is not within province reserved for foresting province
	WorldTile2 centerTile = buildingArea.centerTile();
	int32 provinceId = _simulation->GetProvinceIdClean(centerTile);
	for (const AIRegionStatus& regionStatus : _regionStatuses) {
		if (regionStatus.provinceId == provinceId) {
			if (regionStatus.currentPurpose == AIRegionPurposeEnum::Forester ||
				regionStatus.currentPurpose == AIRegionPurposeEnum::FruitGather) 
			{
				return;
			}
		}
	}

	// Radius buildings shouldn't too close to each other
	auto isTooClose = [&](CardEnum buildingEnum, int32 radius)
	{	
		if (block.HasBuildingEnum(buildingEnum)) 
		{
			int32 checkRadius = radius * 2 / 3;
			
			const std::vector<int32>& curBuildingIds = _simulation->buildingIds(_aiPlayerId, buildingEnum);
			for (int32 curBuildingId : curBuildingIds) {
				if (WorldTile2::Distance(centerTile, _simulation->building(curBuildingId).centerTile()) < checkRadius) {
					return true;
				}
			}
		}
		return false;
	};
	
	if (isTooClose(CardEnum::Tavern, Tavern::Radius)) return;
	if (isTooClose(CardEnum::Library, Library::Radius)) return;
	if (isTooClose(CardEnum::School, School::Radius)) return;
	if (isTooClose(CardEnum::Theatre, Theatre::Radius)) return;
	if (isTooClose(CardEnum::Bank, Bank::Radius)) return;
	if (isTooClose(CardEnum::HaulingServices, HaulingServices::Radius)) return;
	

	// include road
	buildingArea.minX--;
	buildingArea.minY--;
	buildingArea.maxX++;
	buildingArea.maxY++;

	block.TryPlaceCityBlockAroundArea(buildingArea, _aiPlayerId, _simulation);

	if (block.HasArea()) 
	{
		std::vector<std::shared_ptr<FNetworkCommand>> commands;
		SimUtils::PlaceCityBlock(block, _aiPlayerId, commands, _simulation);
		_simulation->ExecuteNetworkCommands(commands);

		_LOG(PunAI, "TryPlaceCityBlock _aiPlayerId:%d block:%s", _aiPlayerId, *block.ToFstring());
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

	_LOG(PunAI, "PlaceBuilding() cardEnum:%d centerTile:%s faceDirection:%d", cardEnum, *centerTile.To_FString(), faceDirection);

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

bool AIPlayerSystem::TryPlaceRiverFarm()
{
	const std::vector<int32>& provincesClaimed = _simulation->GetProvincesPlayer(_aiPlayerId);

	// Get available tiles nearby riverTiles

	for (int32 provinceClaimed : provincesClaimed)
	{
		const std::vector<WorldTile2>& riverTiles = _simulation->provinceInfoSystem().provinceBuildingSlot(provinceClaimed).riverTiles;

		TSet<int32> farmCenterTileIds;
		for (const WorldTile2& riverTile : riverTiles) {
			auto tryAdd = [&](WorldTile2 tile) {
				if (_simulation->IsFarmBuildable(tile, _aiPlayerId)) {
					farmCenterTileIds.Add(tile.tileId());
				}
			};
			tryAdd(riverTile);
			tryAdd(riverTile + WorldTile2(1, 0));
			tryAdd(riverTile + WorldTile2(-1, 0));
			tryAdd(riverTile + WorldTile2(0, 1));
			tryAdd(riverTile + WorldTile2(0, -1));
		}

		for (int32 farmCenterTile : farmCenterTileIds) {
			if (TryPlaceFarm(WorldTile2(farmCenterTile))) {
				return true;
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

		//if (_simulation->buildingCount(_aiPlayerId, CardEnum::Quarry) == 0) {
		//	tryBuildOne(CardEnum::Quarry);
		//}

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

void AIPlayerSystem::TryPlaceNormalBuildings_ScaleToPopCount(AICityBlock& block, int32 aiTier, int32 population, bool shouldBuildJob)
{
	int32 basePopCountPerBuilding = 40;
	int32 baseBuildingPrice = GetBuildingInfo(CardEnum::FurnitureWorkshop).constructionCostAsMoney();

	auto luxBuilding = [&](CardEnum buildingEnum)
	{
		int32 buildingPrice = GetBuildingInfo(buildingEnum).constructionCostAsMoney();

		return std::pair<CardEnum, int32>(buildingEnum, basePopCountPerBuilding * buildingPrice / baseBuildingPrice);
	};

	const std::vector<std::vector<std::pair<CardEnum, int32>>> aiTierToBuildingEnumToPopCountPerTarget
	{
		// Era 1
		{},
		{
			luxBuilding(CardEnum::FurnitureWorkshop),
			luxBuilding(CardEnum::BeerBrewery),

			{ CardEnum::Tavern, 60 },
		},
		// Era 2
		{
			{ CardEnum::HaulingServices, 80 },
			{ CardEnum::Bakery, 70 },
			luxBuilding(CardEnum::Potter),
		},
		{
			luxBuilding(CardEnum::Tailor),
		},
		// Era 3
		{
			luxBuilding(CardEnum::Winery),
			luxBuilding(CardEnum::CoffeeRoaster),
		},
		{
			luxBuilding(CardEnum::Chocolatier),
		},
		// Era 4
		{
			luxBuilding(CardEnum::Jeweler),
		},
		{
			luxBuilding(CardEnum::ClockMakers),
		},
	};


	if (!block.IsValid())
	{
		if (_simulation->buildingCount(_aiPlayerId, CardEnum::TradingPort) ||
			_simulation->buildingCount(_aiPlayerId, CardEnum::TradingPost))
		{
			for (int32 tier = 1; tier <= aiTier; tier++)
			{
				const std::vector<std::pair<CardEnum, int32>>& buildingEnumToPopCountPerTarget = aiTierToBuildingEnumToPopCountPerTarget[tier];
				for (const std::pair<CardEnum, int32>& pair : buildingEnumToPopCountPerTarget) 
				{
					if (GetBuildingInfo(pair.first).workerCount > 0 &&
						!shouldBuildJob) 
					{
						continue;
					}
					
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

void AIPlayerSystem::TryPlaceNormalBuildings_Single(AICityBlock& block, int32 aiTier, bool shouldBuildJob)
{
	const std::vector<std::vector<CardEnum>> eraToSingleBuildingEnum
	{
		// Era 1
		{},
		{},
		// Era 2
		{
			CardEnum::Library,
			CardEnum::Brickworks,
		},
		{
			CardEnum::SpyCenter,
			CardEnum::Market,
		},
		// Era 3
		{
			CardEnum::School,
			CardEnum::PolicyOffice,
			CardEnum::Glassworks,
		},
		{
			CardEnum::Theatre,
			CardEnum::Bank,
			CardEnum::Museum,
			CardEnum::Zoo,
		},
		// Era 4
		{
			CardEnum::ConcreteFactory,
		},
		{
			CardEnum::CardCombiner,
		},

	};
	
	if (!block.IsValid())
	{
		for (int32 tier = 1; tier <= aiTier; tier++)
		{
			const std::vector<CardEnum>& buildingEnums = eraToSingleBuildingEnum[tier];
			for (CardEnum buildingEnum : buildingEnums)
			{
				if (GetBuildingInfo(buildingEnum).workerCount > 0 &&
					!shouldBuildJob)
				{
					continue;
				}
				
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