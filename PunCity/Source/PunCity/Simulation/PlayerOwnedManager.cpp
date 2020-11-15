// Fill out your copyright notice in the Description page of Project Settings.

#include "PlayerOwnedManager.h"
#include "UnitStateAI.h"
#include "Building.h"
#include "Buildings/House.h"
#include "Buildings/Temples.h"
#include "Buildings/TownHall.h"
#include "Resource/ResourceSystem.h"
#include "HumanStateAI.h"
#include "StatSystem.h"
#include "PunCity/CppUtils.h"
#include "UnlockSystem.h"
#include "Buildings/GathererHut.h"
#include "Buildings/TradeBuilding.h"
#include "WorldTradeSystem.h"

using namespace std;



void PlayerOwnedManager::PlayerAddHuman(int32 objectId)
{
	if (_simulation->unitAI(objectId).subclass<HumanStateAI>().isBelowWorkingAge()) {
		_childIds.push_back(objectId);
	} else {
		_adultIds.push_back(objectId);
	}

	_simulation->QuestUpdateStatus(_playerId, QuestEnum::PopulationQuest, 0);

	RefreshJobDelayed();
	RecalculateTaxDelayed();
}

void PlayerOwnedManager::PlayerAddJobBuilding(Building& building, bool isConstructed)
{
	//PUN_LOG("PlayerAddJobBuilding %d constructed:%d ticks:%d", building.buildingId(), isConstructed, Time::Ticks());
	
	CardEnum buildingEnum = building.buildingEnum();
	int32 buildingId = building.buildingId();

	//_LOG(PunPlayerOwned, "PlayerAddJobBuilding: id:%d player:%d ,%llu %llu %llu", buildingId, _playerId, _roadConstructionIds.size(), _constructionIds.size(), _jobBuildingEnumToIds.size());

	if (buildingEnum == CardEnum::Bridge) {
		return;
	}
	
	if (IsRoad(buildingEnum)) {
		_roadConstructionIds.push_back(buildingId);
	}
	else if (!isConstructed) {
		_constructionIds.push_back(buildingId);
	}
	else {
		_jobBuildingEnumToIds[static_cast<int>(buildingEnum)].push_back(buildingId);
	}

	//_LOG(PunPlayerOwned, "- : %llu %llu %llu", _roadConstructionIds.size(), _constructionIds.size(), _jobBuildingEnumToIds.size());

	RefreshJobDelayed();

	// Job Building Unlocks
	auto unlockSys = _simulation->unlockSystem(_playerId);
	if (!unlockSys->unlockedStatisticsBureau &&
		jobBuildingCount() >= 3)
	{
		unlockSys->unlockedStatisticsBureau = true;
		
		if (_simulation->TryAddCardToBoughtHand(_playerId, CardEnum::StatisticsBureau)) {
			_simulation->AddPopup(_playerId, "Unlocked Statistics Bureau. Once built, allow you to view Town Statistics.");
		}
	}
	if (!unlockSys->unlockedEmploymentBureau &&
		jobBuildingCount() >= 7)
	{
		if (_simulation->TryAddCardToBoughtHand(_playerId, CardEnum::JobManagementBureau)) {
			unlockSys->unlockedEmploymentBureau = true;
			_simulation->AddPopup(_playerId, "Unlocked Employment Bureau. Once built, you gain the ability to manage the job priority (global).");
		}
	}
}
void PlayerOwnedManager::PlayerRemoveJobBuilding(Building& building, bool isConstructed)
{
	//PUN_LOG("PlayerRemoveJobBuilding %d constructed:%d ticks:%d", building.buildingId(), isConstructed, Time::Ticks());
	
	CardEnum buildingEnum = building.buildingEnum();
	int32 buildingId = building.buildingId();

	//PUN_LOG("PlayerRemoveJobBuilding: %d", buildingId);

	if (buildingEnum == CardEnum::Bridge) {
		return;
	}
	
	if (IsRoad(buildingEnum)) {
		CppUtils::TryRemove(_roadConstructionIds, buildingId);
	}
	else if (!isConstructed) {
		CppUtils::TryRemove(_constructionIds, buildingId);
	}
	else {
		int32 buildingEnumInt = static_cast<int>(buildingEnum);
		PUN_CHECK(buildingEnumInt < _jobBuildingEnumToIds.size());
		CppUtils::Remove(_jobBuildingEnumToIds[buildingEnumInt], buildingId);
	}

	//PUN_LOG(" - : %d %d %d", _roadConstructionIds.size(), _constructionIds.size(), _jobBuildingEnumToIds.size());

	RefreshJobDelayed();
}

int32 PlayerOwnedManager::housingCapacity()
{
	int32 result = 0;
	for (int32 houseId : _houseIds) {
		result += _simulation->building(houseId).allowedOccupants();
	}
	return result;
}

int32 PlayerOwnedManager::TryFillJobBuildings(const std::vector<int32>& jobBuildingIds, PriorityEnum priority, int& index, bool shouldDoDistanceSort, int32 maximumFill)
{
	// Sort job buildings by distance from townhall
	WorldTile2 gateTile = _simulation->townhall(_playerId).gateTile();
	std::vector<int32> sortedJobBuildingIds;

	if (shouldDoDistanceSort)
	{
		std::vector<int32> distances;

		for (int32 jobBuildingId : jobBuildingIds)
		{
			Building& building = _simulation->building(jobBuildingId);

			if (building.priority() != priority) {
				continue;
			}

			int32 distance = WorldTile2::ManDistance(building.gateTile(), gateTile);
			bool inserted = false;
			for (size_t i = 0; i < sortedJobBuildingIds.size(); i++) {
				if (distance < distances[i]) {
					distances.insert(distances.begin() + i, distance);
					sortedJobBuildingIds.insert(sortedJobBuildingIds.begin() + i, jobBuildingId);
					inserted = true;
					break;
				}
			}

			// distance is more than last index, add to the back
			if (!inserted) {
				distances.push_back(distance);
				sortedJobBuildingIds.push_back(jobBuildingId);
			}

		}
	}
	else
	{
		// No Sort
		for (int32 jobBuildingId : jobBuildingIds)
		{
			Building& building = _simulation->building(jobBuildingId);
			if (building.priority() != priority) {
				continue;
			}
			
			sortedJobBuildingIds.push_back(jobBuildingId);
		}
	}

	// Fill jobs from least distance to most distance.
	int32 localIndex = 0;
	for (int32 jobBuildingId : sortedJobBuildingIds)
	{
		Building& building = _simulation->building(jobBuildingId);

		LOOP_CHECK_START();
		while (index < targetNonLaborerCount() && building.ShouldAddWorker(priority) && maximumFill > localIndex)
		{
			LOOP_CHECK_END();
			
			int32 humanId = _adultIds[index];

			building.AddOccupant(humanId);
			_simulation->unitAI(humanId).SetWorkplaceId(jobBuildingId);
			index++;
			localIndex++;
		}
		if (index >= targetNonLaborerCount()) {
			break;
		}
		// TODO: (maximumFill <= localIndex) break
	}

	return localIndex;
}

void PlayerOwnedManager::RefreshJobs()
{
	PUN_LOG("RefreshJobs %d %dsec", _adultIds.size(), Time::Seconds());

	// Promote child if possible
	for (size_t i = _childIds.size(); i-- > 0;)
	{
		if (!_simulation->unitAI(_childIds[i]).subclass<HumanStateAI>().isBelowWorkingAge()) {
			_adultIds.push_back(_childIds[i]);
			_childIds.erase(_childIds.begin() + i);
		}
	}


	// Remove Building Job Occupants
	for (const std::vector<int32_t>& buildingIds : _jobBuildingEnumToIds) {
		RemoveJobsFromBuildings(buildingIds);
	}
	RemoveJobsFromBuildings(_constructionIds);

	// Remove roadBuilders
	for (size_t i = _roadMakerIds.size(); i-- > 0;) {
		_simulation->unitAI(_roadMakerIds[i]).SetWorkplaceId(-1);
	}
	_roadMakerIds.clear();
	//RemoveJobsFromBuildings(_roadConstructBuildingIds);

	// loop through give job to humans
	int index = 0; // human index

	/*
	 * Priority buildings
	 */
	const std::vector<CardEnum>& buildingEnumPriorityList = GetJobPriorityList();

	// Special case: Farm high priority fill during harvest season
	if (Time::FallSeason()) {
		TryFillJobBuildings(_jobBuildingEnumToIds[static_cast<int>(CardEnum::Farm)], PriorityEnum::Priority, index);
	}

	auto shouldSkipBuilding = [&](CardEnum buildingEnum)
	{
		// Special case Winter
		if (Time::IsWinter()) {
			if (buildingEnum == CardEnum::FruitGatherer) {
				return true;
			}
		}

		// Fall season: Farm has earlier higher priority
		if (buildingEnum == CardEnum::Farm && Time::FallSeason()) {
			return true;
		}
		
		return false;
	};

	// Try fill priority buildings
	for (CardEnum buildingEnum : buildingEnumPriorityList) 
	{
		int buildingEnumInt = static_cast<int>(buildingEnum);
		if (buildingEnumInt >= _jobBuildingEnumToIds.size()) {
			continue;
		}

		if (shouldSkipBuilding(buildingEnum)) {
			continue;
		}
		
		TryFillJobBuildings(_jobBuildingEnumToIds[buildingEnumInt], PriorityEnum::Priority, index);
	}

	// Constructions that are starred individually
	_builderCount = TryFillJobBuildings(_constructionIds, PriorityEnum::Priority, index);
	//_roadBuilderCount = 0;

	// Builder high priority, fill to get to targetBuilderCount
	if (targetBuilderHighPriority) 
	{
		if (_builderCount < targetBuilderCount) {
			int32 maxFill = targetBuilderCount - _builderCount;
			_builderCount += TryFillJobBuildings(_constructionIds, PriorityEnum::NonPriority, index, false, maxFill);
		}
		//if (_builderCount < targetBuilderCount) {
		//	int32 maxFill = targetBuilderCount - _builderCount;
		//	_builderCount += TryFillJobBuildings(_roadConstructBuildingIds, PriorityEnum::NonPriority, index, maxFill);
		//}
	}

	if (targetRoadMakerHighPriority)
	{
		// Add road builders
		LOOP_CHECK_START();
		while (index < targetNonLaborerCount() && _roadMakerIds.size() < targetRoadMakerCount)
		{
			LOOP_CHECK_END();
			
			int32 humanId = _adultIds[index];
			_roadMakerIds.push_back(humanId);
			_simulation->unitAI(humanId).SetWorkplaceId(townHallId); // roadBuilder just use townHall as workplace for easy query...
			index++;
		}
	}
	
	
	/*
	 * Non priority buildings
	 */
	
	 // Special case: Farm fill first during harvest season to ensure the harvest is done.
	if (Time::FallSeason()) {
		TryFillJobBuildings(_jobBuildingEnumToIds[static_cast<int>(CardEnum::Farm)], PriorityEnum::NonPriority, index);
	}

	// Try Fill Non-priority buildings
	for (CardEnum buildingEnum : buildingEnumPriorityList) {
		int buildingEnumInt = static_cast<int>(buildingEnum);
		if (buildingEnumInt >= _jobBuildingEnumToIds.size()) {
			continue;
		}

		if (shouldSkipBuilding(buildingEnum)) {
			continue;
		}
		
		TryFillJobBuildings(_jobBuildingEnumToIds[buildingEnumInt], PriorityEnum::NonPriority, index);
	}

	// If the builders are not manual adjusted, fill them here
	if (!targetBuilderHighPriority)
	{
		_builderCount += TryFillJobBuildings(_constructionIds, PriorityEnum::NonPriority, index);
	}

	//_builderCount += TryFillJobBuildings(_roadConstructBuildingIds, PriorityEnum::NonPriority, index);
	////_roadBuilderCount += _builderCount;
	
	_employedCount = index;
}

void PlayerOwnedManager::FillHouseSlots_FromWorkplace(std::vector<int32>& tempHumanIds)
{
	auto fillHouseFromJobBuildings = [&](const std::vector<int32_t>& jobBuildingIds)
	{
		for (int32_t jobBuildingId : jobBuildingIds) {
			Building& jobBuilding = _simulation->building(jobBuildingId);
			const std::vector<int>& occupants = jobBuilding.occupants();

			// fill each occupant near their job,
			for (int32_t occupantId : occupants)
			{
				// Find available house near this building
				int32_t bestHouseId = -1;
				int32_t bestDistance = numeric_limits<int32_t>::max();
				for (int32_t houseId : _houseIds) {
					Building& house = _simulation->building(houseId);
					if (house.CanAddOccupant())
					{
						int32_t distance = WorldTile2::ManDistance(jobBuilding.gateTile(), house.gateTile());
						if (distance < bestDistance) {
							bestHouseId = houseId;
							bestDistance = distance;
						}
					}
				}

				// No more available house, return
				if (bestHouseId == -1) {
					return true; // True for exit
				}

				House& house = _simulation->building(bestHouseId).subclass<House>(CardEnum::House);
				house.AddOccupant(occupantId);
				
				_simulation->unitAI(occupantId).SetHouseId(bestHouseId);

				// Remove occupant from tempHumanIds
				bool removedFromList = false;
				for (size_t i = tempHumanIds.size(); i-- > 0;) {
					if (occupantId == tempHumanIds[i]) {
						tempHumanIds.erase(tempHumanIds.begin() + i);
						removedFromList = true;
						break;
					}
				}

				PUN_CHECK(removedFromList);

				// Filled everyone, return
				if (tempHumanIds.size() == 0) {
					return true;  // True for exit
				}
			}
		}

		return false;
	};
	

	// Go through the building list and put people near their job
	const std::vector<CardEnum>& buildingEnumPriorityList = GetJobPriorityList();
	for (CardEnum buildingEnum : buildingEnumPriorityList)
	{
		int buildingEnumInt = static_cast<int>(buildingEnum);
		if (buildingEnumInt >= _jobBuildingEnumToIds.size()) {
			continue;
		}

		// Special case Winter
		if (Time::IsWinter()) {
			if (buildingEnum == CardEnum::FruitGatherer ||
				buildingEnum == CardEnum::Farm) {
				continue;
			}
		}
		
		const std::vector<int32>& jobBuildingIds = _jobBuildingEnumToIds[buildingEnumInt];// _simulation->buildingIds(_playerId, buildingEnum);
		
		
		bool shouldExit = fillHouseFromJobBuildings(jobBuildingIds);

		if (shouldExit) {
			return;
		}
	}

	fillHouseFromJobBuildings(_constructionIds);
}

void PlayerOwnedManager::RefreshHousing()
{
	if (!hasChosenLocation()) return;

	// Clear tenants
	for (auto houseId : _houseIds) {
		// Remove Building Occupants
		_simulation->RemoveTenantFrom(houseId);
	}

	std::vector<int32> tempHumanIds = _adultIds;

	// Move people close to their jobs
	// Going from the critical jobs (food etc.) to less critical jobs
	FillHouseSlots_FromWorkplace(tempHumanIds);
	

	// Rest of people without workplace goes to any house..
	int index = 0;
	for (auto houseId : _houseIds) {
		Building& house = _simulation->building(houseId);

		LOOP_CHECK_START();
		while (index < tempHumanIds.size() && house.CanAddOccupant()) 
		{
			LOOP_CHECK_END();
			
			int32 humanId = tempHumanIds[index];
			house.AddOccupant(humanId);
			_simulation->unitAI(humanId).SetHouseId(houseId);
			index++;
		}
		if (index >= tempHumanIds.size()) break;
	}

	int childIndex = 0;
	for (auto houseId : _houseIds) {
		Building& house = _simulation->building(houseId);

		LOOP_CHECK_START();
		while (childIndex < _childIds.size() && house.CanAddOccupant()) 
		{
			LOOP_CHECK_END();
			
			int32 humanId = _childIds[childIndex];
			house.AddOccupant(humanId);
			_simulation->unitAI(humanId).SetHouseId(houseId);
			childIndex++;
		}
		if (childIndex >= _childIds.size()) break;
	}
}

void PlayerOwnedManager::TickRound()
{
	auto& resourceSys = _simulation->resourceSystem(_playerId);

	
	CollectRoundIncome();

	/*
	 * Per season stat...
	 */
	if (Time::Ticks() % Time::TicksPerSeason == 0)
	{
		int32 exportMoney100 = 0;
		int32 importMoney100 = 0;

		auto accumulateData = [&](CardEnum buildingEnum)
		{
			std::vector<int32> buildingIds = _simulation->buildingIds(_playerId, buildingEnum);
			for (int32 buildingId : buildingIds) {
				auto& tradingCompany = _simulation->building(buildingId).subclass<TradeBuilding>(buildingEnum);
				exportMoney100 += tradingCompany.exportMoney100();
				importMoney100 += std::abs(tradingCompany.importMoney100());
			}
		};
		accumulateData(CardEnum::TradingCompany);
		accumulateData(CardEnum::TradingPort);
		accumulateData(CardEnum::TradingPost);
		
		AddDataPoint(PlotStatEnum::Export, exportMoney100 / 100, Time::TicksPerSeason);
		AddDataPoint(PlotStatEnum::Import, importMoney100 / 100, Time::TicksPerSeason);
		//AddDataPoint(PlotStatEnum::TradeBalance, (exportMoney100 - importMoney100) / 100, Time::TicksPerSeason);
	}

	/*
	 * Mid autumn buyer arrival
	 */
	PUN_LOG("MidAutumn TickRound: %d, %d, %d, %d", Time::Ticks(), (Time::Ticks() % Time::TicksPerSeason), (Time::Ticks() % Time::TicksPerSeason != 0), Time::IsAutumn());
	if (Time::IsAutumn() && Time::Ticks() % Time::TicksPerSeason != 0)
	{
		_simulation->AddPopup(PopupInfo(_playerId, "A caravan has arrived. They wish to buy any goods you might have.", { "Trade", "Refuse" }, PopupReceiverEnum::CaravanBuyer));
	}

	
	/*
	 * Migration accumulate and migrate out
	 */
	if (Time::Ticks() % Time::TicksPerSeason != 0 && // Only calculate mid season...
		_migrationPendingCount >= 3)
	{
		// Find a suitable player to accept these people, if there is none, just kill them...
		//  Randomize between suitable player who gets the migrant
		std::vector<int32> playerIdsAvailable;
		
		int32 sourceHappiness = _simulation->GetAverageHappiness(_playerId);
		_simulation->ExecuteOnPlayersAndAI([&](int32 playerId) {
			// Go to a happier place only
			if (_simulation->playerOwned(playerId).hasChosenLocation() &&
				_simulation->GetAverageHappiness(playerId) > sourceHappiness) 
			{
				playerIdsAvailable.push_back(playerId);
			}
		});

		if (playerIdsAvailable.size() > 0) 
		{
			int32 playerIdToGoTo = playerIdsAvailable[GameRand::Rand() % playerIdsAvailable.size()];
			_simulation->ImmigrationEvent(playerIdToGoTo, _migrationPendingCount,
				to_string(_migrationPendingCount) + " immigrants from " + _simulation->playerName(_playerId) + " want to join you. They think your city is a lot better place to make a living.",
				PopupReceiverEnum::ImmigrationBetweenPlayersEvent);
		}

		_migrationPendingCount = 0;
	}
}

void PlayerOwnedManager::RecalculateTax(bool showFloatup)
{
	std::fill(incomes100.begin(), incomes100.end(), 0);
	std::fill(influenceIncomes100.begin(), influenceIncomes100.end(), 0);

	// Crash Guard
	if (!hasTownhall()) {
		return;
	}
	if (!_simulation->isValidBuildingId(townHallId)) {
		_LOG(PunBuilding, "RecalculateTax BuildingId Invalid pid:%d townhallId:%d", _playerId, townHallId);
		return;
	}
	if (_simulation->buildingEnum(townHallId) != CardEnum::Townhall) {
		_LOG(PunBuilding, "RecalculateTax Building-Not-Townhall pid:%d townhallId:%d", _playerId, townHallId);
		return;
	}

	if (PunSettings::TrailerMode()) {
		return;
	}

	/*
	 * Houses
	 */
	for (auto houseId : _houseIds)
	{
		House& house = _simulation->building(houseId).subclass<House>(CardEnum::House);

		if (house.isDisabled()) {
			continue;
		}

		house.CalculateConsumptions(false);
	
		// Coins
		for (size_t i = 0; i < HouseIncomeEnumCount; i++) {
			incomes100[i] += house.GetIncome100Int(i);
		}

		// influence from Luxury resource consumption
		influenceIncomes100[static_cast<int>(InfluenceIncomeEnum::Luxury)] += house.GetInfluenceIncome100();

		//if (showFloatup) {
		//	_simulation->uiInterface()->ShowFloatupInfo(FloatupEnum::GainMoney, house.centerTile(), "+" + to_string(house.totalHouseIncome()));
		//}
	}

	{
		int32 sumFromHouses = CppUtils::Sum(incomes100);
		if (_simulation->IsResearched(_playerId, TechEnum::MoneyLastEra)) {
			incomes100[static_cast<int>(IncomeEnum::MoneyLastEra)] += sumFromHouses * 20 / 100;
		}
	}

	ResourceSystem& resourceSys = _simulation->resourceSystem(_playerId);

	/*
	 * Townhall
	 */
	if (townHallId != -1) {
		auto& townhall = _simulation->building(townHallId).subclass<TownHall>(CardEnum::Townhall);
		incomes100[static_cast<int>(IncomeEnum::TownhallIncome)] += townhall.townhallIncome() * 100;
		
		if (showFloatup) {
			_simulation->uiInterface()->ShowFloatupInfo(FloatupEnum::GainMoney, townhall.centerTile(), "+" + to_string(townhall.townhallIncome()));
		}
	}

	/*
	 * Workplaces
	 */
	for (const std::vector<int32>& jobBuildingIds : _jobBuildingEnumToIds) {
		for (int32 jobBuildingId : jobBuildingIds)
		{
			Building& building = _simulation->building(jobBuildingId);

			if (building.isDisabled()) {
				continue;
			}

			if (building.isConstructed()) // TODO: might not need this anymore?
			{
				// TODO: InvestmentBank not yet used... Make sure it works as a percentage bank...
				if (building.isEnum(CardEnum::InvestmentBank)) {
					Bank* bank = static_cast<Bank*>(&building);
					bank->lastRoundProfit = resourceSys.money() / 10;
					PUN_LOG("InvestmentBankGain: %d", resourceSys.money());

					incomes100[static_cast<int>(IncomeEnum::BankProfit)] += bank->lastRoundProfit * 100;

					if (showFloatup) {
						_simulation->uiInterface()->ShowFloatupInfo(FloatupInfo(FloatupEnum::GainMoney, Time::Ticks(), building.centerTile(), "+" + to_string(bank->lastRoundProfit)));
					}
				}
				//// Bank
				//else if (building.isEnum(CardEnum::Bank)) 
				//{
				//	Bank* bank = static_cast<Bank*>(&building);
				//	
				//	bank->CalculateRoundProfit();
				//	incomes100[static_cast<int>(IncomeEnum::BankProfit)] += bank->lastRoundProfit * 100;

				//	if (showFloatup) {
				//		_simulation->uiInterface()->ShowFloatupInfo(FloatupEnum::GainMoney, bank->centerTile(), "+" + to_string(bank->lastRoundProfit));
				//	}
				//}
				
			}
		}
	}

	/*
	 * Banks
	 */
	{
		const std::vector<int32>& buildingIds = _simulation->buildingIds(_playerId, CardEnum::Bank);
		for (int32 buildingId : buildingIds) {
			Bank& bank = _simulation->building<Bank>(buildingId);

			bank.CalculateRoundProfit();
			incomes100[static_cast<int>(IncomeEnum::BankProfit)] += bank.lastRoundProfit * 100;

			if (showFloatup) {
				_simulation->uiInterface()->ShowFloatupInfo(FloatupEnum::GainMoney, bank.centerTile(), "+" + to_string(bank.lastRoundProfit));
			}
		}
	}

	/*
	 * Bonus Buildings
	 */
	for (int32 bonusBuildingId : _bonusBuildingIds)
	{
		Building& building = _simulation->building(bonusBuildingId);

		if (building.isDisabled()) {
			continue;
		}

		
	}

	for (const std::pair<CardEnum, int32>& pair : BuildingEnumToUpkeep)
	{
		const std::vector<int32>& buildingIds = _simulation->buildingIds(_playerId, pair.first);
		for (int32 buildingId : buildingIds) {
			Building& building = _simulation->building(buildingId);

			if (building.isConstructed())
			{
				int32 upkeep = building.upkeep();
				if (upkeep > 0)
				{
					ChangeIncome(-upkeep, showFloatup, building.centerTile());
					
					//incomes[static_cast<int>(IncomeEnum::BuildingUpkeep)] -= upkeep;

					//if (showFloatup) {
					//	_simulation->uiInterface()->ShowFloatupInfo(FloatupInfo(FloatupEnum::GainMoney, Time::Ticks(), building.centerTile(), "-" + to_string(upkeep)));
					//}
				}
			}
		}
	}

	//// Stock market
	//auto& stockMarketIds = _simulation->buildingIds(_playerId, CardEnum::StockMarket);;
	//if (stockMarketIds.size() > 0)
	//{
	//	int32 tradingCompanyCount = _simulation->buildingCount(_playerId, CardEnum::TradingCompany);
	//	ChangeIncome(tradingCompanyCount * 5, showFloatup, _simulation->building(stockMarketIds[0]).centerTile());
	//}
	

	/*
	 * Region bonus
	 */
	//for (WorldRegion2 region : _regionsClaimed)
	//{
	//	auto& terrainGenerator = _simulation->terrainGenerator();
	//	if (terrainGenerator.GetBiome(region.centerTile()) == BiomeEnum::Desert) 
	//	{
	//		int32_t desertCardCount = _simulation->BoughtCardCount(_playerId, CardEnum::DesertPilgrim);
	//		if (desertCardCount > 0) {
	//			desertPilgrimIncome += desertCardCount * 10;
	//		}
	//	}
	//}

	/*
	 * Card Bonus
	 */
	incomes100[static_cast<int>(IncomeEnum::InvestmentProfit)] = 100 * 30 * _simulation->TownhallCardCount(_playerId, CardEnum::Investment);


	bool conglomerateComplete = _simulation->TownhallCardCount(_playerId, CardEnum::Conglomerate) > 0 &&
								_simulation->buildingCount(_playerId, CardEnum::TradingCompany) >= 2;
	incomes100[static_cast<int>(IncomeEnum::ConglomerateIncome)] = 100 * (conglomerateComplete ? 50 : 0);

	/*
	 * Science
	 */
	std::fill(sciences100.begin(), sciences100.end(), 0);
	for (auto houseId : _houseIds) {
		House& house = _simulation->building(houseId).subclass<House>(CardEnum::House);

		for (size_t i = 0; i < HouseScienceEnums.size(); i++) {
			sciences100[static_cast<int>(HouseScienceEnums[i])] += house.GetScience100(HouseScienceEnums[i]);
		}
	}

	int32 sumFromHouses = CppUtils::Sum(sciences100);

	// Less tech than lord, get +20% sci
	if (_lordPlayerId != -1 && 
		_simulation->sciTechsCompleted(_playerId) < _simulation->sciTechsCompleted(_lordPlayerId))
	{
		sciences100[static_cast<int>(ScienceEnum::KnowledgeTransfer)] += sumFromHouses * 20 / 100;
	}

	
	if (_simulation->IsResearched(_playerId, TechEnum::ScienceLastEra)) {
		sciences100[static_cast<int>(ScienceEnum::ScienceLastEra)] += sumFromHouses * 20 / 100;
	}

	if (_simulation->IsResearched(_playerId, TechEnum::Rationalism)) {
		sciences100[static_cast<int>(ScienceEnum::Rationalism)] += sumFromHouses * 20 / 100;
	}

	/*
	 * Vassal
	 */
	for (int32 vassalBuildingId : _vassalBuildingIds) 
	{
		int32 vassalPlayerId = _simulation->building<TownHall>(vassalBuildingId, CardEnum::Townhall).playerId();
		auto& vassalPlayerOwned = _simulation->playerOwned(vassalPlayerId);

		// Tax
		vassalPlayerOwned.RecalculateTax(false);
		incomes100[static_cast<int>(IncomeEnum::FromVassalTax)] += vassalPlayerOwned.totalRevenue100() * vassalPlayerOwned.vassalTaxPercent() / 100;

	}

	/*
	 * Pay lord the tax...
	 */
	if (_lordPlayerId != -1) {
		incomes100[static_cast<int>(IncomeEnum::ToLordTax)] -= totalIncome100() * vassalTaxPercent() / 100;
	}

	/*
	 * Army maintenance tax...
	 */
	
	int32 totalArmyUpkeep = 0;
	std::vector<int32> armyCounts = _simulation->GetTotalArmyCounts(_playerId);
	for (int32 i = 0; i < ArmyEnumCount; i++) {
		totalArmyUpkeep += armyCounts[i] * GetArmyInfoInt(i).upkeep100();
	}
	incomes100[static_cast<int>(IncomeEnum::ArmyUpkeep)] -= totalArmyUpkeep; // 3% per round or 24% per year... clubman 60..pay 2

	/*
	 * Territory
	 */
	{
		// Territory Revenue
		int32 territoryRevenue100 = 0;
		for (int32 provinceId : _provincesClaimed) {
			territoryRevenue100 += _simulation->GetProvinceIncome100(provinceId);
		}
		incomes100[static_cast<int>(IncomeEnum::TerritoryRevenue)] += territoryRevenue100;
	}

	std::vector<int32> tradePartners = _simulation->worldTradeSystem().GetTradePartners(_playerId);
	int32 tradeClusterTotalPopulation = 0;
	if (tradePartners.size() > 0) { // 1 trade parter means self only, no trade route income
		for (int32 playerId : tradePartners) {
			tradeClusterTotalPopulation += _simulation->population(playerId);
		}
	}
	incomes100[static_cast<int>(IncomeEnum::TradeRoute)] += 100 * tradeClusterTotalPopulation / 2;

	/*
	 * Influence
	 */

	int32 influence100 = _simulation->influence100(_playerId);

	// Upkeep goes to influence unless it is 0
	int32 territoryUpkeep100 = 0;
	for (int32 provinceId : _provincesClaimed) {
		territoryUpkeep100 += _simulation->GetProvinceUpkeep100(provinceId, _playerId);
	}
	
	if (_simulation->unlockedInfluence(_playerId)) 
	{
		influenceIncomes100[static_cast<int>(InfluenceIncomeEnum::Townhall)] += 20 * 100;
		
		// Population: Influence gain equals to population
		influenceIncomes100[static_cast<int>(InfluenceIncomeEnum::Population)] += _simulation->population(_playerId) * 100;
		
		influenceIncomes100[static_cast<int>(InfluenceIncomeEnum::TerritoryUpkeep)] -= territoryUpkeep100;


		// Border Province Upkeep
		int32 numberOfBorderProvinces = 0;
		for (int32 provinceId : _provincesClaimed) {
			if (_simulation->IsBorderProvince(provinceId)) {
				numberOfBorderProvinces++;
			}
		}
		influenceIncomes100[static_cast<int>(InfluenceIncomeEnum::BorderProvinceUpkeep)] -= numberOfBorderProvinces * 500; // 5 upkeep per border province
		
		// Fort/Colony
		influenceIncomes100[static_cast<int>(InfluenceIncomeEnum::Fort)] -= _simulation->buildingCount(_playerId, CardEnum::Fort) * 10 * 100;
		influenceIncomes100[static_cast<int>(InfluenceIncomeEnum::Colony)] -= _simulation->buildingCount(_playerId, CardEnum::Colony) * Colony::GetColonyUpkeep() * 100;
	}
	else
	{
		// Without influence unlocked,
		// Territory Upkeep become to money penalty
		incomes100[static_cast<int>(IncomeEnum::TerritoryUpkeep)] -= territoryUpkeep100;
	}

	// Influence beyond 1 year accumulation gets damped
	// At 2 years worth accumulation. The influence income becomes 0;
	int32 influenceIncomeBeforeCapDamp100 = totalInfluenceIncome100();
	int32 maxInfluence100 = maxStoredInfluence100();
	if (influence100 > maxInfluence100 && influenceIncomeBeforeCapDamp100 > 0) {
		// Fully damp to 0 influence income at x2 maxStoredInfluence
		int32 influenceIncomeDamp100 = influenceIncomeBeforeCapDamp100 * (influence100 - maxInfluence100) / max(1, maxInfluence100); // More damp as influence stored is closer to fullYearInfluenceIncome100
		influenceIncomeDamp100 = min(influenceIncomeDamp100, influenceIncomeBeforeCapDamp100); // Can't damp more than existing influence income
		PUN_CHECK(influenceIncomeDamp100 >= 0);
		influenceIncomes100[static_cast<int>(InfluenceIncomeEnum::TooMuchInfluencePoints)] = -influenceIncomeDamp100;
	}
}


void PlayerOwnedManager::CollectRoundIncome()
{
	RecalculateTax(true);

	// Consume luxury
	for (auto houseId : _houseIds) {
		House& house = _simulation->building(houseId).subclass<House>(CardEnum::House);

		if (house.isDisabled()) {
			continue;
		}

		house.CalculateConsumptions(true);
	}

	ResourceSystem& resourceSys = _simulation->resourceSystem(_playerId);

	// Change Income
	int32 totalIncome100WithoutHouse = totalRevenue100WithoutHouse() - totalExpense100();
	resourceSys.ChangeMoney100(totalIncome100WithoutHouse);

	// Change Influence
	resourceSys.ChangeInfluence100(totalInfluenceIncome100());
	if (resourceSys.influence100() < 0) 
	{
		std::string influenceStr = "</><img id=\"Influence\"/><EventLogRed>";
		std::string coinStr = "</><img id=\"Coin\"/><EventLogRed>";

		std::stringstream ss;
		ss << "You have " << influenceStr << "0. ";
		ss << "-" << coinStr << abs(min(-1, resourceSys.influence() * 2));
		ss << " penalty is applied from negative " << influenceStr << " (" << coinStr << "2 per " << influenceStr << "1).";
		
		_simulation->AddEventLog(_playerId, ss.str(), true);
		resourceSys.ChangeMoney100(resourceSys.influence100() * 3);
		resourceSys.SetInfluence(0);
	}

	//// Disband army if out of cash
	//const int32 maxDisbandPerRound = 3;
	//for (int32 i = 0; i < maxDisbandPerRound; i++) {
	//	if (CheckDisbandArmy()) {
	//		if (i == 0) {
	//			_simulation->AddPopup(_playerId, "Parts of your army have been disbanded for money.", "PopupBad");
	//		}
	//	}
	//}
}
void PlayerOwnedManager::CollectHouseIncome()
{
	// Collect every 3 sec
	if (Time::Ticks() % (Time::TicksPerSecond * 3) == 0)
	{
		int32 revenue100Round = totalRevenue100HouseOnly();

		ResourceSystem& resourceSys = _simulation->resourceSystem(_playerId);
		resourceSys.ChangeMoney100(revenue100Round / (Time::SecondsPerRound / 3));
	}
}

bool PlayerOwnedManager::CheckDisbandArmy()
{
	if (_simulation->money(_playerId) < -300) 
	{
		std::vector<int32> armyNodeIds = _simulation->GetArmyNodeIds(_playerId);
		for (int32 nodeId : armyNodeIds) {
			ArmyGroup* group = _simulation->GetArmyNode(nodeId).GetArmyGroup(_playerId);
			if (group) {
				std::vector<int32> armyCounts = group->GetArmyCounts();
				for (size_t i = TowerArmyEnumCount; i < armyCounts.size(); i++) {
					if (armyCounts[i] > 0) {
						group->RemoveArmyPartial(i, 1);
						_simulation->ChangeMoney(_playerId, GetArmyInfoInt(i).moneyAndResourceCost / 2);
						return true;
					}
				}
			}
		}
	}
	return false;
}

void PlayerOwnedManager::Tick1Sec()
{
	if (!hasChosenLocation()) return;
	//PUN_CHECK(_playerId < _simulation->playerCount() || (_simulation->aiStartIndex() <= _playerId && _playerId <= _simulation->aiEndIndex()));
	PUN_CHECK(_simulation->IsValidPlayer(_playerId));


	// Check resourceCount to detect change betwen 0 and non-zero, and refresh accordingly
	auto& resourceSys = _simulation->resourceSystem(_playerId);
	for (int32 i = 0; i < ResourceEnumCount; i++) {
		int32 newResourceCount = resourceSys.resourceCount(static_cast<ResourceEnum>(i));
		bool newIsZero = (newResourceCount == 0);
		bool lastIsZero = (_lastResourceCounts[i] == 0);
		if (newIsZero != lastIsZero) {
			RefreshJobDelayed();
		}
		
		_lastResourceCounts[i] = newResourceCount;
	}
	

	// Refresh Job if needed
	if (_needRefreshJob) {
		_needRefreshJob = false;
		RefreshJobs();
		RefreshHousing(); // Refresh housing must be done after RefreshJobs() to prevent discrepancy
	}

	
	// Collect house income
	CollectHouseIncome();


	/*
	 * Dark age
	 */
	if (!_isInDarkAge && _simulation->population(_playerId) <= 15)
	{
		std::stringstream ss;
		ss << "Dark Age begins.<line><space>";
		ss << "You must survive this slump and pull your settlement back on track.<space>";
		ss << "Crisis, such as this, tests your skill as a leader.<space>";
		ss << "During Dark Age, Leader Skills are x2 more effective and SP recovers x2 faster.";
		_simulation->AddPopup(PopupInfo(_playerId, ss.str(), { "We must survive!" }, PopupReceiverEnum::None, false, "PopupBad"));

		_isInDarkAge = true;
	}
	if (_isInDarkAge && _simulation->population(_playerId) >= 20)
	{
		std::stringstream ss;
		ss << "Congratulation, you have survived the Dark Age.<line>";
		ss << "What doesn't kill you makes you stronger.\n";
		ss << "Our people are now crisis hardened, ready to march forward through any future obstacles.";
		
		_simulation->AddPopup(PopupInfo(_playerId, ss.str()));

		// TODO: Earn card?? may be x2 leader skill?

		_isInDarkAge = false;
	}

	/*
	 * Buffs
	 */
	for (size_t i = 0; i < _buffTicksLeft.size(); i++) 
	{
		if (_buffTicksLeft[i] > 0) {
			_buffTicksLeft[i] = max(0, _buffTicksLeft[i] - Time::TicksPerSecond);
			if (_buffTicksLeft[i] < Time::TicksPerSecond * 30) 
			{
				int32 cardEnumInt = i + BuildingEnumCount;
				std::stringstream ss;
				ss << "Your " << GetBuildingInfoInt(cardEnumInt).name << " Buff is running out.";
				ss << "<space>";
				ss << "Would you like to renew it with <img id=\"Coin\"/>xPopulation?";
				_simulation->AddPopupNonDuplicate(PopupInfo(_playerId, ss.str(), { "Renew", "Close" }, PopupReceiverEnum::ResetBuff, false, "", cardEnumInt));
			}
		}
	}
	

	/*
	 * Happiness update...
	 */
	_aveFoodHappiness = 0;
	_aveHeatHappiness = 0;
	_aveHousingHappiness = 0;
	_aveFunHappiness = 0;

	std::fill(_aveHappinessModifiers.begin(), _aveHappinessModifiers.end(), 0);

	int32 adultPopulation = _adultIds.size();
	if (adultPopulation > 0) {
		for (auto humanId : _adultIds)
		{
			HumanStateAI& human =_simulation->unitAI(humanId).subclass<HumanStateAI>(UnitEnum::Human);

			_aveFoodHappiness += human.foodHappiness();
			_aveHeatHappiness += human.heatHappiness();
			_aveHousingHappiness += human.housingHappiness();
			_aveFunHappiness += human.funHappiness();
			
			for (size_t i = 0; i < _aveHappinessModifiers.size(); i++) {
				_aveHappinessModifiers[i] += human.GetHappinessModifier(static_cast<HappinessModifierEnum>(i));
			}
		}
		_aveFoodHappiness /= adultPopulation;
		_aveHeatHappiness /= adultPopulation;
		_aveHousingHappiness /= adultPopulation;
		_aveFunHappiness /= adultPopulation;
		
		for (size_t i = 0; i < _aveHappinessModifiers.size(); i++) {
			int32 sumHappiness = _aveHappinessModifiers[i];
			_aveHappinessModifiers[i] = sumHappiness / adultPopulation;
			PUN_CHECK(_aveHappinessModifiers[i] < 1000 && _aveHappinessModifiers[i] > -1000);
		}
	}

	//if (Time::IsSummer() || Time::IsAutumn())
	//{
	//	PUN_LOG("SUMMER %d sum:%d aut:%d", Time::Ticks(), Time::IsSummerStart(), Time::IsAutumnStart());
	//}

	// Immigration...
	if (Time::IsSummerStart() || Time::IsAutumnStart()) {
		//_simulation->townhall(_playerId).ImmigrationEvent();
	}

	// TODO: make this more randomized by using Tick1Sec instead and randomizing check interval... doing that will also allow infecting other ppl in random interval
	/*
	 * Sickness
	 *  on average, a person get sick once every year equating to 1 medicine and 10 coins
	 *  in winter sickness chance x3 .. overall lead to 15 coins usage per year
	 *
	 *  jungle x3 sickness
	 */
	if (Time::Ticks() >= _nextDiseaseCheckTick)
	{
		// 20 mins per year, check around 20 times a year
		int32 ticksInterval = Time::TicksPerMinute * (75 + GameRand::Rand() % 50) / 100;
		_nextDiseaseCheckTick = Time::Ticks() + ticksInterval;

		for (int32 humanId : _adultIds)
		{
			int32 sickPercent100PerYear = 10000; // 
			int32 sickPercent100PerCheck = sickPercent100PerYear / 20;  // 20 checks per year

			if (Time::IsWinter()) {
				sickPercent100PerCheck *= 3;
			}
			if (_simulation->GetBiomeEnum(_simulation->unitAtom(humanId).worldTile2()) == BiomeEnum::Jungle) {
				sickPercent100PerCheck *= 2;
			}

			if (GameRand::Rand() % 10000 < sickPercent100PerCheck) {
				_simulation->unitAI(humanId).subclass<HumanStateAI>(UnitEnum::Human).GetSick();
			}
		}
	}


	/*
	 * Stat: Add Data
	 */
	if (Time::Ticks() % TicksPerStatInterval == 0)
	{
		AddDataPoint(PlotStatEnum::Population, adultPopulation + childPopulation());
		//AddDataPoint(PlotStatEnum::AdultPopulation, adultPopulation());
		AddDataPoint(PlotStatEnum::ChildPopulation, childPopulation());

		AddDataPoint(PlotStatEnum::Income, totalIncome100() / 100);
		AddDataPoint(PlotStatEnum::Revenue, totalRevenue100() / 100);
		AddDataPoint(PlotStatEnum::Expense, totalExpense100() / 100);

		AddDataPoint(PlotStatEnum::Science, science100PerRound() / 100);
		AddDataPoint(PlotStatEnum::Food, _simulation->foodCount(_playerId));
		AddDataPoint(PlotStatEnum::Fuel, _simulation->resourceCount(_playerId, ResourceEnum::Wood) +
												_simulation->resourceCount(_playerId, ResourceEnum::Coal));

		AddDataPoint(PlotStatEnum::Technologies, _simulation->sciTechsCompleted(_playerId));
		AddDataPoint(PlotStatEnum::InfluencePoints, _simulation->influence(_playerId));
	}


	/*
	 * ProvinceClaimProgress
	 */
	for (ProvinceClaimProgress& claimProgress : _defendingClaimProgress) {
		claimProgress.Tick1Sec(_simulation);
	}
}


void PlayerOwnedManager::SetHouseResourceAllow(ResourceEnum resourceEnum, bool resourceAllowed)
{
	_houseResourceAllowed[static_cast<int>(resourceEnum)] = resourceAllowed;

	ResourceHolderType type = resourceAllowed ? ResourceHolderType::Manual : ResourceHolderType::Provider;
	int32 target = resourceAllowed ? 10 : 0;
	
	for (int32 houseId : _houseIds)
	{
		Building& building = _simulation->building(houseId);
		if (building.isEnum(CardEnum::House)) {
			building.SetHolderTypeAndTarget(resourceEnum, type, target);
		}
	}
}