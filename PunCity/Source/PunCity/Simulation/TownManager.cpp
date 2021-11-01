// Pun Dumnernchanvanit's


#include "TownManager.h"
#include "HumanStateAI.h"
#include "Buildings/House.h"
#include "Buildings/TownHall.h"
#include "Buildings/GathererHut.h"
#include "WorldTradeSystem.h"
#include "BuildingCardSystem.h"


#define LOCTEXT_NAMESPACE "PlayerOwnedManager"

/*
 * Size Names
 */

static const TArray<FText> TownSizeNames
{
	LOCTEXT("Village", "Village"),
	LOCTEXT("Small Town", "Small Town"),
	LOCTEXT("Large Town", "Large Town"),
	LOCTEXT("Small City", "Small City"),
	LOCTEXT("Large City", "Large City"),
	LOCTEXT("Metropolis", "Metropolis"),
};
static int32 TownSizeCount() { return TownSizeNames.Num(); }

static const TArray<FText> TownSizeSuffix
{
	LOCTEXT("village", "village"),
	LOCTEXT("town", "town"),
	LOCTEXT("town", "town"),
	LOCTEXT("city", "city"),
	LOCTEXT("city", "city"),
	LOCTEXT("metropolis", "metropolis"),
};

static int32 GetTownSizeTier(int32 population)
{
	for (int32 i = 1; i < TownSizeCount(); i++) {
		if (population < GetTownSizeMinPopulation(i)) {
			return i - 1;
		}
	}
	return TownSizeCount() - 1;
}

FText TownManager::GetTownSizeSuffix()
{
	for (int32 i = 1; i < TownSizeCount(); i++) {
		if (population() < GetTownSizeMinPopulation(i)) {
			return TownSizeSuffix[i - 1];
		}
	}
	return TownSizeSuffix[TownSizeCount() - 1];
}

FText TownManager::GetTownSizeName() {
	for (int32_t i = TownSizeCount(); i-- > 0;) {
		if (population() >= TownSizeMinPopulation[i]) {
			return TownSizeNames[i];
		}
	}
	UE_DEBUG_BREAK();
	return FText();
}


/*
 * Job
 */

void TownManager::PlayerAddHuman(int32 objectId)
{
	if (_simulation->unitAI(objectId).subclass<HumanStateAI>().isBelowWorkingAge()) {
		_childIds.push_back(objectId);
	}
	else {
		_adultIds.push_back(objectId);
	}

	_simulation->QuestUpdateStatus(_playerId, QuestEnum::PopulationQuest, 0);

	RefreshJobDelayed();
	RecalculateTaxDelayed();
}

void TownManager::PlayerAddJobBuilding(Building& building, bool isConstructed)
{
	//PUN_LOG("PlayerAddJobBuilding %d constructed:%d ticks:%d", building.buildingId(), isConstructed, Time::Ticks());

	CardEnum buildingEnum = building.buildingEnum();
	int32 buildingId = building.buildingId();

	//_LOG(PunPlayerOwned, "PlayerAddJobBuilding: id:%d player:%d ,%llu %llu %llu", buildingId, _playerId, _roadConstructionIds.size(), _constructionIds.size(), _jobBuildingEnumToIds.size());

	if (IsBridgeOrTunnel(buildingEnum)) {
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
	if (!unlockSys->unlockedPriorityStar &&
		jobBuildingCount() >= 2)
	{
		unlockSys->unlockedPriorityStar = true;
		_simulation->AddPopup(_playerId, {
			NSLOCTEXT("TownHall", "UnlockedPriorityButton1_Pop", "Unlocked Priority Button <img id=\"NonPriorityStar\"/>!"),
			NSLOCTEXT("TownHall", "UnlockedPriorityButton2_Pop", "<space>Click it to switch building's priority between 3 states:\n  <img id=\"NonPriorityStar\"/> Default worker allocation\n  <img id=\"PriorityStar\"/> Prioritize working here\n  <img id=\"PriorityStop\"/> Don't allow working here"),
		});
	}
	if (!unlockSys->unlockedStatisticsBureau &&
		jobBuildingCount() >= 5)
	{
		if (_simulation->TryAddCardToBoughtHand(_playerId, CardEnum::StatisticsBureau)) 
		{
			unlockSys->unlockedStatisticsBureau = true;
			_simulation->AddPopup(_playerId,
				LOCTEXT("UnlockStatsBureau_Pop", "Unlocked Statistics Bureau. Once built, allow you to view Town Statistics.")
			);
		}
	}
	if (!unlockSys->unlockedEmploymentBureau &&
		jobBuildingCount() >= 7)
	{
		if (_simulation->TryAddCardToBoughtHand(_playerId, CardEnum::JobManagementBureau)) 
		{
			unlockSys->unlockedEmploymentBureau = true;
			_simulation->AddPopup(_playerId,
				LOCTEXT("UnlockEmploymentBureau_Pop", "Unlocked Employment Bureau. Once built, you gain the ability to manage the job priority (global).")
			);
		}
	}
}
void TownManager::PlayerRemoveJobBuilding(Building& building, bool isConstructed)
{
	//PUN_LOG("PlayerRemoveJobBuilding %d constructed:%d ticks:%d", building.buildingId(), isConstructed, Time::Ticks());

	CardEnum buildingEnum = building.buildingEnum();
	int32 buildingId = building.buildingId();

	//PUN_LOG("PlayerRemoveJobBuilding: %d", buildingId);

	if (IsBridgeOrTunnel(buildingEnum)) {
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

int32 TownManager::housingCapacity()
{
	int32 result = 0;
	for (int32 houseId : _houseIds) {
		result += _simulation->building(houseId).allowedOccupants();
	}
	return result;
}

int32 TownManager::TryFillJobBuildings(const std::vector<int32>& jobBuildingIds, PriorityEnum priority, int& index, bool shouldDoDistanceSort, int32 maximumFill)
{
	// Sort job buildings by distance from townhall
	WorldTile2 gateTile = _simulation->GetMajorTownhallGateFast(_townId);
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

			//int32 humanId = _adultIds[index];

			//building.AddOccupant(humanId);
			//_simulation->unitAI(humanId).SetWorkplaceId(jobBuildingId);

			building.targetOccupants++;
			PUN_CHECK(building.targetOccupants <= building.maxOccupants());

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

void TownManager::RefreshJobs()
{
	_LOG(PunRefreshJob, "RefreshJobs %d %dsec", _adultIds.size(), Time::Seconds());

	// Promote child if possible
	for (size_t i = _childIds.size(); i-- > 0;)
	{
		if (!_simulation->unitAI(_childIds[i]).subclass<HumanStateAI>().isBelowWorkingAge()) {
			_adultIds.push_back(_childIds[i]);
			_childIds.erase(_childIds.begin() + i);
		}
	}



	// Reset targetOccupants
	int32 targetRoadMakerLocalFunc = 0;

	auto resetTargetOccupants = [&](const std::vector<int32>& buildingIds) {
		for (int32 buildingId : buildingIds) {
			_simulation->building(buildingId).targetOccupants = 0;
		}
	};

	for (const std::vector<int32>& buildingIds : _jobBuildingEnumToIds) {
		resetTargetOccupants(buildingIds);
	}
	resetTargetOccupants(_constructionIds);






	// TODO: old
	//// Remove Building Job Occupants
	//for (const std::vector<int32>& buildingIds : _jobBuildingEnumToIds) {
	//	RefreshJobsFromBuildings(buildingIds);
	//}
	//RefreshJobsFromBuildings(_constructionIds);

	//
	//// Remove roadBuilders
	//for (size_t i = _roadMakerIds.size(); i-- > 0;) {
	//	_simulation->unitAI(_roadMakerIds[i]).SetWorkplaceId(-1);
	//}
	//_roadMakerIds.clear();


	/*
	 * Stage 1: Set targetOccupants
	 */

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
		while (index < targetNonLaborerCount() && targetRoadMakerLocalFunc < targetRoadMakerCount)
		{
			LOOP_CHECK_END();
			index++;
			targetRoadMakerLocalFunc++;

			//int32 humanId = _adultIds[index];
			//_roadMakerIds.push_back(humanId);
			//_simulation->unitAI(humanId).SetWorkplaceId(townHallId); // roadBuilder just use townHall as workplace for easy query...
			//index++;
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

	_employedCount = index; // Employed are non-laborers


	// Debug
	PUN_CHECK(_jobBuildingEnumToIds[static_cast<int>(CardEnum::Townhall)].size() == 0);


	/*
	 * Stage 2:
	 *  - Determine people that can skip job refresh
	 *  - Remove any unneeded occupants
	 */

	TSet<int32> adultIdsToSkipJobRefresh;

#if WITH_EDITOR
	_LOG(PunRefreshJob, "Start RefreshJob");
	for (int32 adultId : _adultIds) {
		_LOG(PunRefreshJob, " id:%d", adultId);;
	}
#endif

	// Check for adultIds to skip
	auto checkForRefreshSkip = [&](const std::vector<int32>& buildingIds)
	{
		for (int32 buildingId : buildingIds)
		{
			Building& building = _simulation->building(buildingId);
			const std::vector<int32>& occupants = building.occupants();

			_LOG(PunRefreshJob, "Start RemoveOccupants buildingId:%d name:%s", building.buildingId(), *(building.buildingInfo().nameF()));

			// Leave some workers alone
			int32 stableOccupantCount = std::min(static_cast<int32>(occupants.size()), building.targetOccupants);
			for (int32 i = 0; i < stableOccupantCount; i++) {
				adultIdsToSkipJobRefresh.Add(occupants[i]);
				_LOG(PunRefreshJob, " adultIdsToSkipJobRefresh Add %d", occupants[i]);
			}

			// Kick people beyond targetOccupants out of the old job
			//  occupants 4, target 2 .. should kick 2
			//  loop1: 3 < 2
			//  loop2: 2 < 2
			for (int32 i = occupants.size(); i-- > 0;) {
				if (i < building.targetOccupants) {
					_LOG(PunRefreshJob, " -- break i:%d id:%d", i, occupants[i]);
					break;
				}

				_LOG(PunRefreshJob, " RemoveOccupants i:%d id:%d", i, occupants[i]);

				building.RemoveOccupant(occupants[i]);
				_simulation->unitAI(occupants[i]).SetWorkplaceId(-1);
			}

			//// TEST: Refresh all
			//for (int32 i = occupants.size(); i-- > 0;) {
			//	building.RemoveOccupant(occupants[i]);
			//	_simulation->unitAI(occupants[i]).SetWorkplaceId(-1);
			//}
		}
	};
	for (const std::vector<int32>& buildingIds : _jobBuildingEnumToIds) {
		checkForRefreshSkip(buildingIds);
	}
	checkForRefreshSkip(_constructionIds);


	// Remove roadBuilders
	for (size_t i = _roadMakerIds.size(); i-- > 0;) {
		_LOG(PunRefreshJob, "Remove RoadBuilders i:%d id:%d", i, _roadMakerIds[i]);
		_simulation->unitAI(_roadMakerIds[i]).SetWorkplaceId(-1);
	}
	_roadMakerIds.clear();


	// TODO: temporary check that all occupants are removed properly
	//for (int32 adultId : _adultIds) {
	//	PUN_CHECK(_simulation->unitAI(adultId).workplaceId() == -1);
	//}

	_LOG(PunRefreshJob, " --- Done Remove: adultIdsToSkipJobRefresh:%d", adultIdsToSkipJobRefresh.Num());


	/*
	 * Stage 3: Fill up to targetOccupants
	 */
	int32 adultIndex = 0;
	int32 debugTotalJobCount = 0;

	auto getNextHumanId = [&]() {
		int32 humanId = -1;

		LOOP_CHECK_START();
		while (adultIndex < _adultIds.size()) {
			LOOP_CHECK_END();

			int32 localHumanId = _adultIds[adultIndex];
			if (!adultIdsToSkipJobRefresh.Contains(localHumanId)) {
				_LOG(PunRefreshJob, "getNextHumanId[succeed] adultIndex:%d localHumanId:%d", adultIndex, localHumanId);

				humanId = localHumanId;
				adultIndex++;
				break;
			}
			_LOG(PunRefreshJob, "getNextHumanId[failed] adultIndex:%d localHumanId:%d", adultIndex, localHumanId);

			adultIndex++;
		}

		return humanId;
	};

	auto fillToTargetOccupants = [&](const std::vector<int32>& buildingIds)
	{
		for (int32 buildingId : buildingIds)
		{
			Building& building = _simulation->building(buildingId);

			debugTotalJobCount += building.targetOccupants;

			_LOG(PunRefreshJob, "Start AddOccupants buildingId:%d name:%s", buildingId, *(building.buildingInfo().nameF()));

			for (int32 i = building.occupantCount(); i < building.targetOccupants; i++)
			{
				int32 humanId = getNextHumanId();
				PUN_CHECK(humanId != -1); // building.targetOccupants should already be calculated to ensure we do not exceed max adultIds
				_LOG(PunRefreshJob, " - AddOccupants i:%d adultIndex:%d id:%d", i, adultIndex, humanId);

				building.AddOccupant(humanId);
				_simulation->unitAI(humanId).SetWorkplaceId(buildingId);
			}
		}
	};

	for (const std::vector<int32>& buildingIds : _jobBuildingEnumToIds) {
		fillToTargetOccupants(buildingIds);
	}
	fillToTargetOccupants(_constructionIds);

	PUN_CHECK(debugTotalJobCount + targetRoadMakerLocalFunc == index);


	// Add road builders
	_LOG(PunRefreshJob, "Start _roadMakerIds:%d", _roadMakerIds.size());
	LOOP_CHECK_START();
	while (adultIndex < targetNonLaborerCount() && _roadMakerIds.size() < targetRoadMakerCount)
	{
		LOOP_CHECK_END();
		int32 humanId = getNextHumanId();

		if (humanId != -1) {
			_LOG(PunRefreshJob, " - Add _roadMakerIds adultIndex:%d id:%d", adultIndex, humanId);
			PUN_CHECK(_simulation->unitAI(humanId).workplaceId() == -1);

			_roadMakerIds.push_back(humanId);
			_simulation->unitAI(humanId).SetWorkplaceId(townhallId); // roadBuilder just use townHall as workplace for easy query...	
		}
	}

	//
	//// Remove roadBuilders
	//for (size_t i = _roadMakerIds.size(); i-- > 0;) {
	//	_simulation->unitAI(_roadMakerIds[i]).SetWorkplaceId(-1);
	//}
	//_roadMakerIds.clear();

}

void TownManager::FillHouseSlots_FromWorkplace(std::vector<int32>& tempHumanIds)
{
	_LOG(PunRefreshJob, "FillHouseSlots_FromWorkplace tempHumanIds:%d", tempHumanIds.size());
	for (int32 tempHumanId : tempHumanIds) {
		_LOG(PunRefreshJob, " - id:%d", tempHumanId);
	}

#if WITH_EDITOR
	// Debug look for any duplicates
	TSet<int32> debugHumanIds;
#endif

	auto fillHouseFromJobBuildings = [&](const std::vector<int32>& jobBuildingIds)
	{
		for (int32 jobBuildingId : jobBuildingIds) {
			Building& jobBuilding = _simulation->building(jobBuildingId);
			const std::vector<int>& occupants = jobBuilding.occupants();

			// fill each occupant near their job,
			for (int32 occupantId : occupants)
			{
#if WITH_EDITOR
				PUN_CHECK(!debugHumanIds.Contains(occupantId));
				debugHumanIds.Add(occupantId);
#endif

				// Find available house near this building
				int32 bestHouseId = -1;
				int32 bestDistance = numeric_limits<int32_t>::max();
				for (int32 houseId : _houseIds) {
					Building& house = _simulation->building(houseId);
					if (house.CanAddOccupant())
					{
						int32 distance = WorldTile2::ManDistance(jobBuilding.gateTile(), house.gateTile());
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
						_LOG(PunRefreshJob, "- tempHumanIds:%d occupantId:%d bldName:%s", tempHumanIds.size(), occupantId, *(jobBuilding.buildingInfo().nameF()));
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

		_LOG(PunRefreshJob, "-- fillHouseFromJobBuildings jobBuildingIds");
		bool shouldExit = fillHouseFromJobBuildings(jobBuildingIds);

		if (shouldExit) {
			return;
		}
	}

	_LOG(PunRefreshJob, "-- fillHouseFromJobBuildings _constructionIds");
	fillHouseFromJobBuildings(_constructionIds);
}

void TownManager::RefreshHousing()
{
	if (!_simulation->HasChosenLocation(_playerId)) return;

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

void TownManager::CollectHouseIncome()
{
	// Collect every 3 sec
	if (Time::Ticks() % (Time::TicksPerSecond * 3) == 0)
	{
		int32 revenue100Round = totalRevenue100HouseOnly();

		_simulation->ChangeMoney100(_playerId, revenue100Round / (Time::SecondsPerRound / 3));
	}
}

void TownManager::SetHouseResourceAllow(ResourceEnum resourceEnum, bool resourceAllowed)
{
	int32 resourceEnumInt = static_cast<int>(resourceEnum);
	PUN_ENSURE(resourceEnumInt >= 0, return);
	PUN_ENSURE(resourceEnumInt < _houseResourceAllowed.size(), return);
	_houseResourceAllowed[resourceEnumInt] = resourceAllowed;

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

void TownManager::CollectRoundIncome()
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
	GlobalResourceSystem& globalResourceSys = _simulation->globalResourceSystem(_playerId);

	// Change Income
	int32 totalIncome100WithoutHouse = totalRevenue100WithoutHouse() - totalExpense100();
	globalResourceSys.ChangeMoney100(totalIncome100WithoutHouse);

	// Change Influence
	int32 totalInfluenceIncome100WithoutDiplomaticBuilding = totalInfluenceIncome100() - influenceIncomes100[static_cast<int>(InfluenceIncomeEnum::DiplomaticBuildings)];
	globalResourceSys.ChangeInfluence100(totalInfluenceIncome100WithoutDiplomaticBuilding);
	if (globalResourceSys.influence100() < 0)
	{
		FText influenceStr = INVTEXT("<img id=\"Influence\"/>");
		FText coinStr = INVTEXT("<img id=\"Coin\"/>");

		_simulation->AddEventLog(_playerId,
			FText::Format(LOCTEXT("NegativeInfluence_Event",
				"You have {0}0. -{1}{2} penalty is applied from negative {0} ({1}2 per {0}1)."),
				influenceStr,
				coinStr,
				TEXT_NUM(abs(min(-1, globalResourceSys.influence() * 2)))
			),
			true
		);
		globalResourceSys.ChangeMoney100(globalResourceSys.influence100() * 3);
		globalResourceSys.SetInfluence(0);
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


/*
 * Tick
 */

void TownManager::Tick()
{
	if (_needTaxRecalculation) {
		_needTaxRecalculation = false;
		RecalculateTax(false);
	}

	// Unit Training
	if (_trainUnitsQueue.size() > 0)
	{
		_trainUnitsTicks100 += 1 * 100;

		if (_trainUnitsTicks100 >= GetTrainingLengthTicks(_trainUnitsQueue[0].cardEnum))
		{
			if (_simulation->TryAddCardToBoughtHand(_playerId, _trainUnitsQueue[0].cardEnum))
			{
				_trainUnitsTicks100 = 0;
				_trainUnitsQueue[0].stackSize--;

				// Take away 3 population
				std::vector<int32> humanIds = adultIds();
				std::vector<int32> childIdsTemp = childIds();
				humanIds.insert(humanIds.end(), childIdsTemp.begin(), childIdsTemp.end());

				int32 killCount = std::min(GetMilitaryInfo(_trainUnitsQueue[0].cardEnum).humanCost, static_cast<int>(humanIds.size()));
				for (int32 i = 0; i < killCount; i++) {
					_simulation->unitAI(humanIds[i]).Die();
				}

				if (_trainUnitsQueue[0].stackSize <= 0) {
					_trainUnitsQueue.erase(_trainUnitsQueue.begin());
				}
			}
		}
	}
}

void TownManager::Tick1Sec()
{
	if (!_simulation->HasChosenLocation(_playerId)) return;
	
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


	//! Collect house income
	CollectHouseIncome();

	//! Collect Diplomatic Building Influence Income
	if (Time::Ticks() % GameConstants::BaseFloatupIntervalSec == 0 && isCapital())
	{
		const std::vector<int32>& diplomaticBuildings = _simulation->GetDiplomaticBuildings(_playerId);
		for (int32 diplomaticBuildingId : diplomaticBuildings) {
			DiplomaticBuilding& bld = _simulation->building<DiplomaticBuilding>(diplomaticBuildingId);
			int32 influenceIncome100 = bld.influenceIncome100(_playerId);

			int32 influenceIncome_Sec = GameRand::RandRound(influenceIncome100 * GameConstants::BaseFloatupIntervalSec, Time::SecondsPerRound * 100);
			if (influenceIncome_Sec > 0) {
				_simulation->ChangeInfluence(_playerId, influenceIncome_Sec);
				
				_simulation->uiInterface()->ShowFloatupInfo(_playerId, FloatupEnum::GainInfluence, bld.centerTile(), TEXT_NUMSIGNED(influenceIncome_Sec));
				
				if (bld.foreignBuilder() != _playerId) {
					relationship().ChangeModifier(bld.foreignBuilder(), RelationshipModifierEnum::DiplomaticBuildings, GameRand::RandRound(influenceIncome_Sec, InfluenceToRelationship));
				}
			}
		}
	}

	/*
	 * Happiness update...
	 */
	//std::fill(_aveHappinessModifiers.begin(), _aveHappinessModifiers.end(), 0);

	// Update Food Variety
	_townFoodVariety = 0;
	for (ResourceEnum foodEnum : FoodEnums) {
		if (_simulation->resourceCountTown(_townId, foodEnum) > 0) {
			_townFoodVariety++;
		}
	}
	
	std::fill(_aveHappiness.begin(), _aveHappiness.end(), 0);
	
	int32 adultPopulation = _adultIds.size();
	if (adultPopulation > 0) 
	{
		// Sum up
		for (auto humanId : _adultIds)
		{
			HumanStateAI& human = _simulation->unitAI(humanId).subclass<HumanStateAI>(UnitEnum::Human);

			//for (size_t i = 0; i < _aveHappinessModifiers.size(); i++) {
			//	_aveHappinessModifiers[i] += human.GetHappinessModifier(static_cast<HappinessModifierEnum>(i));
			//}

			for (size_t i = 0; i < _aveHappiness.size(); i++) {
				int32 happinessByType = human.GetHappinessByType(static_cast<HappinessEnum>(i));
				_aveHappiness[i] += happinessByType;
				PUN_CHECK(happinessByType < 1000 && happinessByType > -1000);
			}
		}

		// Take average
		for (size_t i = 0; i < _aveHappiness.size(); i++) {
			int32 sumHappiness = _aveHappiness[i];
			_aveHappiness[i] = sumHappiness / adultPopulation;
			PUN_CHECK(_aveHappiness[i] < 1000 && _aveHappiness[i] > -1000);
		}

		//for (size_t i = 0; i < _aveHappinessModifiers.size(); i++) {
		//	int32 sumHappiness = _aveHappinessModifiers[i];
		//	_aveHappinessModifiers[i] = sumHappiness / adultPopulation;
		//	PUN_CHECK(_aveHappinessModifiers[i] < 1000 && _aveHappinessModifiers[i] > -1000);
		//}
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
			const int32 sickPercent100PerYear = 10000; // 
			const int32 sickPercent100PerCheck_base = sickPercent100PerYear / 20;  // 20 checks per year (check every minute)

			int32 sickPercent100PerCheck = sickPercent100PerCheck_base * (100 + _simulation->difficultyConsumptionAdjustment(_playerId)) / 100;

			if (Time::IsWinter()) {
				sickPercent100PerCheck *= 3;
			}
			if (_simulation->GetBiomeEnum(_simulation->unitAtom(humanId).worldTile2()) == BiomeEnum::Jungle) {
				sickPercent100PerCheck *= 3;
			}

			if (GameRand::Rand() % 10000 < sickPercent100PerCheck) {
				_simulation->unitAI(humanId).subclass<HumanStateAI>(UnitEnum::Human).SetSick(true);
			}
		}
	}

	/*
	 * Electricity
	 * - Electricity half worker usage. Existing worker works 100% faster
	 * - Electricity consumption 1 kW = 1  person
	 * - 135 food value per year = 20 coal per year, 10 coal per year ~ 1kWh per Round
	 * - 1 kW = 1 coal burn per round
	 */
	int32 totalElectricityNeeded = 0;
	for (std::vector<int32>& buildingIds : _jobBuildingEnumToIds) {
		for (int32 buildingId : buildingIds) {
			Building& building = _simulation->building(buildingId);
			if (building.IsElectricityUpgraded()) {
				totalElectricityNeeded += building.ElectricityAmountNeeded();
			}
		}
	}
	int32 totalElectricityProductionCapacity = 0;
	for (const auto& powerPlantInfo : PowerPlantInfoList) {
		const std::vector<int32>& buildingIds = _simulation->buildingIds(_townId, powerPlantInfo.buildingEnum);
		for (int32 buildingId : buildingIds) {
			totalElectricityProductionCapacity += _simulation->building(buildingId).subclass<PowerPlant>().ElectricityProductionCapacity();
		}
	}

	int32 totalElectricityUsage = 0;
	if (totalElectricityProductionCapacity >= totalElectricityNeeded) {
		totalElectricityUsage = totalElectricityNeeded;
	}

	// Provide Electricity
	for (std::vector<int32>& buildingIds : _jobBuildingEnumToIds) {
		for (int32 buildingId : buildingIds) {
			Building& building = _simulation->building(buildingId);
			if (building.IsElectricityUpgraded()) {
				building.SetElectricityAmountUsage(building.ElectricityAmountNeeded() * totalElectricityUsage / std::max(1, totalElectricityNeeded));
			}
		}
	}

	// Consume Fuel
	for (const auto& powerPlantInfo : PowerPlantInfoList) {
		const std::vector<int32>& buildingIds = _simulation->buildingIds(_townId, powerPlantInfo.buildingEnum);
		for (int32 buildingId : buildingIds) 
		{
			PowerPlant& building = _simulation->building(buildingId).subclass<PowerPlant>();

			if (building.isConstructed())
			{
				int32 actualProduction_kW = GameRand::RandRound(
					building.ElectricityProductionCapacity() * totalElectricityUsage,
					std::max(1, totalElectricityProductionCapacity)
				);
				building.ConsumeFuel1Sec(actualProduction_kW);
			}
		}
	}

	_electricityConsumption = totalElectricityUsage;
	_electricityProductionCapacity = totalElectricityProductionCapacity;

	/*
	 * Tourism
	 */
	{
		std::vector<TradeRoutePair> tradeRoutes = _simulation->worldTradeSystem().GetTradeRoutesTo(_townId);

		// Get Hotels on Trade Route
		std::vector<Hotel*> hotels;
		for (const TradeRoutePair& route : tradeRoutes) {
			int32 destinationTownId = route.GetCounterpartTownId(_townId);
			const std::vector<int32>& localHotelIds = _simulation->buildingIds(destinationTownId, CardEnum::Hotel);
			for (int32 localHotelId : localHotelIds) {
				Hotel* hotel = static_cast<Hotel*>(_simulation->buildingPtr(localHotelId));
				if (hotel->isAvailable()) {
					hotels.push_back(hotel);
				}
			}
		}

		if (hotels.size() > 0)
		{
			// Sort Hotels by Service Quality
			std::sort(hotels.begin(), hotels.end(), [&](Hotel* a, Hotel* b) {
				return a->serviceQuality() > b->serviceQuality();
			});

			const int32 clusterSize = GameRand::RandRound(population(), Time::SecondsPerRound);

			int32 hotelIndex = 0;

			LoopPopulation(clusterSize, _tourismIncreaseIndex, [&](int32 unitId)
			{
				HumanStateAI& humanAI = _simulation->unitAI(unitId).subclass<HumanStateAI>(UnitEnum::Human);

				hotelIndex = (hotelIndex + 1) % hotels.size();

				Hotel* hotel = hotels[hotelIndex];
				hotel->Visit(_townId);

				_simulation->ChangeMoney(hotel->playerId(), hotel->feePerVisitor());
				_simulation->uiInterface()->ShowFloatupInfo(hotel->playerId(), FloatupEnum::GainMoney, hotel->centerTile(), TEXT_NUMSIGNED(hotel->feePerVisitor()));

				// Embassy Hotel Chain
				if (_simulation->HasForeignBuildingWithUpgrade(hotel->foreignBuilder(), _townId, CardEnum::Embassy, 1))
				{
					int32 moneyGain100 = hotel->feePerVisitor() * 100 * 20 / 100;
					_simulation->ChangeMoney(hotel->foreignBuilder(), moneyGain100);
					_simulation->uiInterface()->ShowFloatupInfo(hotel->foreignBuilder(), FloatupEnum::GainMoney, hotel->centerTile(), TEXT_100SIGNED(moneyGain100));
				}

				humanAI.SetHappiness(HappinessEnum::Tourism, hotel->serviceQuality());
			});

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
		AddDataPoint(PlotStatEnum::Food, _simulation->foodCount(_townId));
		AddDataPoint(PlotStatEnum::Fuel, _simulation->resourceCountTown(_townId, ResourceEnum::Wood) +
												_simulation->resourceCountTown(_townId, ResourceEnum::Coal));

		AddDataPoint(PlotStatEnum::Technologies, _simulation->sciTechsCompleted(_playerId));
		AddDataPoint(PlotStatEnum::InfluencePoints, _simulation->influence(_playerId));
	}

}

void TownManager::TickRound()
{
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
	 * Auto Trade
	 */
	{
		//! Calculate AutoTrade
		int32 exportMoneyTotal = 0;
		int32 importMoneyTotal = 0;
		int32 feeTotal = 0;
		int32 feeDiscountTotal = 0;

		TArray<FText> exportTooltipText;
		TArray<FText> importTooltipText;
		TArray<FText> feeTooltipText;
		TArray<FText> feeDiscountTooltipText;

		CalculateAutoTradeProfit(
			exportMoneyTotal,
			importMoneyTotal,
			feeTotal,
			feeDiscountTotal,

			exportTooltipText,
			importTooltipText,
			feeTooltipText,
			feeDiscountTooltipText,
			false
		);

		int32 netTotal100 = exportMoneyTotal - importMoneyTotal - feeTotal;

		
		//! AutoTrade Exchange goods
		auto& resourceSys = _simulation->resourceSystem(_townId);
		
		for (const AutoTradeElement& element : _autoExportElements) {
			resourceSys.RemoveResourceGlobal(element.resourceEnum, element.calculatedTradeAmountNextRound);
		}

		int32 totalRefundAmount100 = 0;
		
		for (const AutoTradeElement& element : _autoImportElements) {
			// For resources that failed to be added, remove the price from the netTotal100
			int32 leftOverAmount = resourceSys.AddResourceGlobal_ReturnLeftover(element.resourceEnum, element.calculatedTradeAmountNextRound);

			if (leftOverAmount > 0) {
				int32 refundAmount100 = element.GetImportLeftoverRefund100(leftOverAmount);
				totalRefundAmount100 += refundAmount100;
				netTotal100 -= refundAmount100;
			}
		}

		
		//! AutoTrade Receive net
		_simulation->ChangeMoney100(_playerId, netTotal100);

		SetLastRoundAutoTradeProfit(netTotal100);
		
		//! UI Display
		WorldTile2 floatupCenter = _simulation->building(townhallId).centerTile();
		_simulation->uiInterface()->ShowFloatupInfo(FloatupEnum::GainMoney, floatupCenter, TEXT_100SIGNED(netTotal100));

		if (totalRefundAmount100 > 0)
		{
			_simulation->AddPopup(_playerId, FText::Format(
				LOCTEXT("AutoTrade_NotEnoughStorage_Pop", "Auto-trade import did not complete fully because storages are full.<space>You were refunded {0}<img id=\"Coin\"/> for the incomplete import."),
				TEXT_100(totalRefundAmount100)
			));
		}
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
			if (_simulation->HasChosenLocation(playerId) &&
				_simulation->GetAverageHappiness(playerId) > sourceHappiness)
			{
				playerIdsAvailable.push_back(playerId);
			}
		});

		if (playerIdsAvailable.size() > 0)
		{
			int32 playerIdToGoTo = playerIdsAvailable[GameRand::Rand() % playerIdsAvailable.size()];
			_simulation->ImmigrationEvent(playerIdToGoTo, _migrationPendingCount,
				FText::Format(LOCTEXT("ImmigrantsHappyAsk",
					"{0} immigrants from {1} want to join you. They think your city is a lot better place to make a living."),
					TEXT_NUM(_migrationPendingCount),
					_simulation->playerNameT(_playerId)
				),
				PopupReceiverEnum::ImmigrationBetweenPlayersEvent
			);
		}

		_migrationPendingCount = 0;
	}
}

void TownManager::RecalculateTax(bool showFloatup)
{
	std::fill(incomes100.begin(), incomes100.end(), 0);
	std::fill(influenceIncomes100.begin(), influenceIncomes100.end(), 0);

	// Crash Guard
	if (!_simulation->HasTownhall(_playerId)) {
		return;
	}
	if (!_simulation->isValidBuildingId(townhallId)) {
		_LOG(PunBuilding, "RecalculateTax BuildingId Invalid pid:%d townhallId:%d", _playerId, townhallId);
		return;
	}
	if (_simulation->buildingEnum(townhallId) != CardEnum::Townhall) {
		_LOG(PunBuilding, "RecalculateTax Building-Not-Townhall pid:%d townhallId:%d", _playerId, townhallId);
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
		Building& houseBld = _simulation->building(houseId);
		House& house = houseBld.subclass<House>(CardEnum::House);

		if (house.isDisabled()) {
			continue;
		}

		house.CalculateConsumptions(false);

		// Coins
		for (size_t i = 0; i < HouseIncomeEnumCount; i++) {
			incomes100[i] += house.GetIncome100Int(i);
		}

		//// influence from Luxury resource consumption
		//influenceIncomes100[static_cast<int>(InfluenceIncomeEnum::Luxury)] += house.GetInfluenceIncome100();

		//if (_simulation->HasTownBonus(_townId, CardEnum::SavannaGrasslandRoamer)) {
		//	if (IsGrassDominant(house.centerBiomeEnum())) {
		//		influenceIncomes100[static_cast<int>(InfluenceIncomeEnum::GrasslandRoamer)] += house.GetInfluenceIncome100() * 50 / 100;
		//	}
		//}
	}

	{
		int32 sumFromHouses = CppUtils::Sum(incomes100);
		if (_simulation->IsResearched(_playerId, TechEnum::EconomicTheories)) {
			incomes100[static_cast<int>(IncomeEnum::EconomicTheories)] += sumFromHouses * 20 / 100;
		}
	}

	ResourceSystem& resourceSys = _simulation->resourceSystem(_townId);
	GlobalResourceSystem& globalResourceSys = _simulation->globalResourceSystem(_townId);

	/*
	 * Townhall
	 */
	//if (townHallId != -1) {
	//	auto& townhall = _simulation->building(townHallId).subclass<TownHall>(CardEnum::Townhall);
	//	incomes100[static_cast<int>(IncomeEnum::TownhallIncome)] += townhall.townhallIncome() * 100;

	//	if (showFloatup) {
	//		_simulation->uiInterface()->ShowFloatupInfo(FloatupEnum::GainMoney, townhall.centerTile(), TEXT_NUMSIGNED(townhall.townhallIncome()));
	//	}
	//}

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

			//if (building.isConstructed()) // TODO: might not need this anymore?
			//{
			//	// TODO: InvestmentBank not yet used... Make sure it works as a percentage bank...
			//	if (building.isEnum(CardEnum::InvestmentBank)) {
			//		Bank* bank = static_cast<Bank*>(&building);
			//		bank->lastRoundProfit = globalResourceSys.money() / 10;
			//		PUN_LOG("InvestmentBankGain: %d", globalResourceSys.money());

			//		incomes100[static_cast<int>(IncomeEnum::BankProfit)] += bank->lastRoundProfit * 100;

			//		if (showFloatup) {
			//			_simulation->uiInterface()->ShowFloatupInfo(FloatupInfo(FloatupEnum::GainMoney, Time::Ticks(), building.centerTile(), TEXT_NUMSIGNED(bank->lastRoundProfit)));
			//		}
			//	}

			//}
		}
	}

	/*
	 * ProfitBuilding
	 *  Bank, Archives
	 */

	for (CardEnum buildingEnum : ProfitBuildings)
	{
		const std::vector<int32>& buildingIds = _simulation->buildingIds(_townId, buildingEnum);
		for (int32 buildingId : buildingIds) {
			ProfitBuilding& bld = _simulation->building<ProfitBuilding>(buildingId);

			bld.CalculateRoundProfit();
			incomes100[static_cast<int>(bld.incomeEnum())] += bld.lastRoundProfit * 100;

			if (showFloatup) {
				_simulation->uiInterface()->ShowFloatupInfo(FloatupEnum::GainMoney, bld.centerTile(), TEXT_NUMSIGNED(bld.lastRoundProfit));
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

	for (int32 i = 0; i < BuildingEnumCount; i++)
	{
		const std::vector<int32>& buildingIds = _simulation->buildingIds(_townId, static_cast<CardEnum>(i));
		for (int32 buildingId : buildingIds)
		{
			Building& building = _simulation->building(buildingId);

			if (building.isConstructed())
			{
				int32 upkeep = building.upkeep();
				if (upkeep > 0) {
					ChangeIncome(-upkeep, showFloatup, building.centerTile());
				}
			}
		}
	}

	if (isCapital()) {
		BuildingCardSystem& cardSystem = _simulation->cardSystem(_playerId);
		std::vector<CardStatus> cards = cardSystem.GetCardsBoughtAndInventory();
		for (const CardStatus& card : cards) {
			if (IsMilitaryCardEnum(card.cardEnum)) {
				incomes100[static_cast<int>(IncomeEnum::ArmyUpkeep)] -= card.stackSize * GetMilitaryInfo(card.cardEnum).upkeep * 100;
			}
		}
	}

	//// Stock market
	//auto& stockMarketIds = _simulation->buildingIds(_townId, CardEnum::StockMarket);;
	//if (stockMarketIds.size() > 0)
	//{
	//	int32 tradingCompanyCount = _simulation->buildingCount(_townId, CardEnum::TradingCompany);
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
	incomes100[static_cast<int>(IncomeEnum::InvestmentProfit)] = 100 * min(100, _simulation->moneyCap32(_playerId) / 20) * _simulation->TownhallCardCountTown(_townId, CardEnum::Investment);


	if ( _simulation->TownhallCardCountTown(_townId, CardEnum::SocialWelfare) > 0) {
		incomes100[static_cast<int>(IncomeEnum::SocialWelfare)] = -100 * 5 * population();
	}

	/*
	 * Science
	 */
	std::fill(sciences100.begin(), sciences100.end(), 0LL);
	for (auto houseId : _houseIds) 
	{
		House& house = _simulation->building(houseId).subclass<House>(CardEnum::House);

		int32 cumulative100 = 0;
		for (size_t i = 0; i < HouseScienceEnums.size(); i++) 
		{
			int32 houseScience100 = house.GetScience100(HouseScienceEnums[i], 0);
			sciences100[static_cast<int>(HouseScienceEnums[i])] += static_cast<int64>(houseScience100);
			cumulative100 += houseScience100;
		}

		for (size_t i = 0; i < HouseScienceModifierEnums.size(); i++) 
		{
			int32 houseScience100 = house.GetScience100(HouseScienceModifierEnums[i], cumulative100);
			sciences100[static_cast<int>(HouseScienceModifierEnums[i])] += static_cast<int64>(houseScience100);
		}
	}

	int64 sumFromHouses = CppUtils::Sum(sciences100);

	// Less tech than lord, get +20% sci
	if (lordPlayerId() != -1 &&
		_simulation->sciTechsCompleted(_playerId) < _simulation->sciTechsCompleted(lordPlayerId()))
	{
		sciences100[static_cast<int>(ScienceEnum::KnowledgeTransfer)] += sumFromHouses * 20LL / 100LL;
	}


	if (_simulation->IsResearched(_playerId, TechEnum::ScientificTheories)) {
		sciences100[static_cast<int>(ScienceEnum::ScientificTheories)] += sumFromHouses * 20LL / 100LL;
	}


	if (_simulation->HasGlobalBonus(_playerId, CardEnum::Rationalism)) {
		sciences100[static_cast<int>(ScienceEnum::Rationalism)] += std::max(0LL, sumFromHouses * ((_simulation->GetAverageHappiness(_townId) - 80) / 2LL) / 100LL);
	}

	// +0.5% Science Boost for each Trading Post/Port/Company in the World
	if (_simulation->HasGlobalBonus(_playerId, CardEnum::FreeThoughts)) {
		int32 totalTradeBuildings = 0;
		std::vector<int32> playerIds = _simulation->GetAllPlayersAndAI();
		for (int32 playerId : playerIds) {
			totalTradeBuildings += _simulation->playerBuildingFinishedCount(playerId, CardEnum::TradingPost);
			totalTradeBuildings += _simulation->playerBuildingFinishedCount(playerId, CardEnum::TradingPort);
			totalTradeBuildings += _simulation->playerBuildingFinishedCount(playerId, CardEnum::TradingCompany);
		}
		sciences100[static_cast<int>(ScienceEnum::FreeThoughts)] += totalTradeBuildings * sumFromHouses * 5 / 1000;
	}

	if (factionEnum() == FactionEnum::Europe) {
		sciences100[static_cast<int>(ScienceEnum::EuropeBonus)] += sumFromHouses * 5LL / 100LL;
	}

	/*
	 * Vassal
	 */
	if (isCapital())
	{
		for (int32 vassalTownId : _vassalTownIds)
		{
			TownManagerBase* vassalTownManager = _simulation->townManagerBase(vassalTownId);

			// Tax
 			vassalTownManager->RecalculateTax(false);
			incomes100[static_cast<int>(IncomeEnum::FromVassalTax)] += vassalTownManager->totalRevenue100() * vassalTownManager->vassalTaxPercent() / 100;
			influenceIncomes100[static_cast<int>(InfluenceIncomeEnum::GainFromVassal)] += vassalTownManager->totalInfluenceIncome100() * vassalTownManager->vassalInfluencePercent() / 100;
		}
	}

	/*
	 * Pay lord the tax...
	 */
	if (lordPlayerId() != -1) {
		incomes100[static_cast<int>(IncomeEnum::ToLordTax)] -= totalIncome100() * vassalTaxPercent() / 100;
		influenceIncomes100[static_cast<int>(InfluenceIncomeEnum::LoseToLord)] += totalInfluenceIncome100() * vassalInfluencePercent() / 100;
	}

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

	std::vector<TradeRoutePair> tradeRoutesTo = _simulation->worldTradeSystem().GetTradeRoutesTo(_townId);
	int32 tradeClusterTotalPopulation = 0;
	for (const TradeRoutePair& route : tradeRoutesTo) 
	{
		auto addPopulation = [&](int32 townIdLocal)
		{
			if (IsMajorTown(townIdLocal)) {
				tradeClusterTotalPopulation += _simulation->populationTown(townIdLocal);
			}
			else if (IsMinorTown(townIdLocal)) {
				tradeClusterTotalPopulation += _simulation->townManagerBase(townIdLocal)->GetMinorCityLevel();
			}
		};
		
		addPopulation(route.townId1);
		addPopulation(route.townId2);
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

		// Population: Influence gain equals to population / 2
		int32 incomeFromPopulation = _simulation->populationTown(_townId) * 100 / 2;
		influenceIncomes100[static_cast<int>(InfluenceIncomeEnum::Population)] += incomeFromPopulation;

		//! Castle
		//if (_simulation->townBuildingFinishedCount(_townId, CardEnum::Castle) > 0) {
		//	influenceIncomes100[static_cast<int>(InfluenceIncomeEnum::Castle)] += incomeFromPopulation * 15 / 100;
		//}

		// Capital Benefits from embassy
		if (isCapital())
		{
			const std::vector<int32>& diplomaticBuildings = _simulation->GetDiplomaticBuildings(_playerId);
			for (int32 diplomaticBuildingId : diplomaticBuildings) {
				influenceIncomes100[static_cast<int>(InfluenceIncomeEnum::DiplomaticBuildings)] += _simulation->building<DiplomaticBuilding>(diplomaticBuildingId).influenceIncome100(_playerId);
			}
		}
		
		//if (_simulation->townBuildingFinishedCount(_townId, CardEnum::Embassy) > 0) {
		//	influenceIncomes100[static_cast<int>(InfluenceIncomeEnum::Castle)] += incomeFromPopulation * 15 / 100;
		//}
		

		influenceIncomes100[static_cast<int>(InfluenceIncomeEnum::TerritoryUpkeep)] -= territoryUpkeep100;


		// Unprotected Province Upkeep
		auto& provinceInfoSys = _simulation->provinceInfoSystem();
		int32 numberOfUnprotectedProvinces = 0;
		for (int32 provinceId : _provincesClaimed) {
			if (!provinceInfoSys.provinceOwnerInfo(provinceId).isSafe) {
				numberOfUnprotectedProvinces++;
			}
		}
		influenceIncomes100[static_cast<int>(InfluenceIncomeEnum::UnsafeProvinceUpkeep)] -= territoryUpkeep100 * numberOfUnprotectedProvinces / _provincesClaimed.size(); // double upkeep per unprotected province

		// Fort/Colony
		//influenceIncomes100[static_cast<int>(InfluenceIncomeEnum::Fort)] -= _simulation->buildingCount(_townId, CardEnum::Fort) * 10 * 100;
		influenceIncomes100[static_cast<int>(InfluenceIncomeEnum::Colony)] -= _simulation->buildingCount(_townId, CardEnum::ResourceOutpost) * ResourceOutpost::GetColonyUpkeep() * 100;
	}
	else
	{
		// Without influence unlocked,
		// Territory Upkeep become to money penalty
		incomes100[static_cast<int>(IncomeEnum::TerritoryUpkeep)] -= territoryUpkeep100;
	}

	// Influence beyond 1 year accumulation gets damped
	// At 2 years worth accumulation. The influence income becomes 0;
	//int64 influenceIncomeBeforeCapDamp100 = totalInfluenceIncome100();
	//int64 maxInfluence100 = _simulation->playerOwned(_playerId).maxStoredInfluence100();
	//if (influence100 > maxInfluence100 && influenceIncomeBeforeCapDamp100 > 0) {
	//	// Fully damp to 0 influence income at x2 maxStoredInfluence
	//	int64 influenceIncomeDamp100 = influenceIncomeBeforeCapDamp100 * (influence100 - maxInfluence100) / max(static_cast<int64>(1), maxInfluence100); // More damp as influence stored is closer to fullYearInfluenceIncome100
	//	influenceIncomeDamp100 = min(influenceIncomeDamp100, influenceIncomeBeforeCapDamp100); // Can't damp more than existing influence income
	//	PUN_CHECK(influenceIncomeDamp100 >= 0);
	//	influenceIncomes100[static_cast<int>(InfluenceIncomeEnum::TooMuchInfluencePoints)] = -influenceIncomeDamp100;
	//}
}


void TownManager::AddCardToTownhall(CardStatus card)
{
	PUN_CHECK(!IsActionCard(card.cardEnum) && !IsBuildingCard(card.cardEnum));
	_cardsInTownhall.push_back(card);

	// Some cards may trigger refresh etc...
	if (card.cardEnum == CardEnum::IndustrialRevolution) {
		_simulation->cardSystem(_playerId).needHand1Refresh = true;
	}
}

void TownManager::ChangeTownOwningPlayer(int32 newPlayerId)
{
	_playerId = newPlayerId;

	for (int32 childId : _childIds) {
		_simulation->unitAI(childId).ChangeTownOwningPlayer(newPlayerId);
	}

	for (int32 adultId : _adultIds) {
		_simulation->unitAI(adultId).ChangeTownOwningPlayer(newPlayerId);
	}
}

int32 TownManager::GetMaxAutoTradeAmount()
{
	int32 maxAutoTradeAmount = 0;
	const std::vector<int32>& tradingCompanyIds = _simulation->buildingIds(_townId, CardEnum::TradingCompany);
	for (int32 tradingCompanyId : tradingCompanyIds)
	{
		TradingCompany& tradingCompany = _simulation->building<TradingCompany>(tradingCompanyId);
		maxAutoTradeAmount += tradingCompany.tradeMaximumPerRound();
	}
	return maxAutoTradeAmount;
}

void TownManager::DecreaseTourismHappinessByAction(int32 actionCost)
{
	check(actionCost >= 0);
	const int32 actionCostPerHappiness = 50;

	int32 happinessDeduction = GameRand::RandRound(actionCost, actionCostPerHappiness);

	const int32 deductionClusterSize = 20;

	LoopPopulation(deductionClusterSize, _tourismDecreaseIndex, [&](int32 unitId)
	{
		int32 deductionValue = GameRand::RandRound(happinessDeduction, deductionClusterSize);
		HumanStateAI& humanAI = _simulation->unitAI(unitId).subclass<HumanStateAI>(UnitEnum::Human);
		humanAI.ChangeHappiness(HappinessEnum::Tourism, -deductionValue);

		int32 tourismHappiness = humanAI.GetHappinessByType(HappinessEnum::Tourism);
		PUN_CHECK(tourismHappiness < 1000 && tourismHappiness > -1000);

		// Popups
		if (_simulation->GetAverageHappinessByType(_townId, HappinessEnum::Tourism) < 50 &&
			_simulation->TryDoCallOnceAction(humanAI.playerId(), PlayerCallOnceActionEnum::LowTourismHappinessPopup))
		{
			_simulation->AddPopupToFront(humanAI.playerId(),
				LOCTEXT("LowTourismHappinessPopup_Popup", "Tourism Happiness is low.<space>Tourism Happiness decreases with more import/export.<space>The more exchange of goods, the more your citizens want to see the outside world.<space>To increase your citizen's Tourism Happiness:<bullet>Build Tourism Agency</><bullet>Encourage neighboring towns to build Hotels</><bullet>Help your poor neighbors build Hotels yourself.</>")
			);
		}
	});

}

/*
 * Training
 */
void TownManager::TrainUnits_Helper(const FUseCard& command, std::vector<CardStatus>& trainUnitsQueue, bool isRealAction)
{
	//check(_pendingTrainCommands.size() > 0);
	//check(_pendingTrainCommands[0] == command);
	if (isRealAction) {
		// Remove the command if possible
		CppUtils::TryRemove(_pendingTrainCommands, command);
	}

	CardEnum cardEnum = command.cardStatus.cardEnum;

	/*
	 * Cancel Train
	 */
	if (command.callbackEnum == CallbackEnum::CancelTrainUnit)
	{
		for (int32 i = trainUnitsQueue.size(); i-- > 0;)
		{
			if (trainUnitsQueue[i].cardBirthTicks == command.variable2)
			{
				trainUnitsQueue[i].stackSize -= command.variable1;
				trainUnitsQueue[i].stackSize = std::max(0, trainUnitsQueue[i].stackSize);

				// Real Action: return the money
				if (isRealAction) {
					ResourcePair resourceCost = GetMilitaryInfo(trainUnitsQueue[i].cardEnum).resourceCost;
					_simulation->AddResourceGlobal_WithMoney(_townId, resourceCost.resourceEnum, resourceCost.count);
				}

				// Remove stack if the size is 0
				if (trainUnitsQueue[i].stackSize == 0) {
					trainUnitsQueue.erase(trainUnitsQueue.begin() + i);

					// Real Action: If this is the first stack, reset the timer
					if (isRealAction && i == 0) {
						_trainUnitsTicks100 = 0;
					}
				}
				return;
			}
		}
		return;
	}

	/*
	 * Train
	 */
	 // Real Action: pay money
	int32 trainingCount = command.variable1;
	if (isRealAction)
	{
		ResourcePair trainingCost = GetTrainUnitCost(cardEnum);

		// Must be able to train at least one unit
		int32 maxPossibleTraining = _simulation->resourceCountTownWithMoney(_townId, trainingCost.resourceEnum) / trainingCost.count;
		if (maxPossibleTraining > 0)
		{
			// Can't train more than money allows
			trainingCount = std::min(trainingCount, maxPossibleTraining);
			trainingCount = std::max(1, trainingCount);

			int32 resourceToSpend = trainingCost.count * trainingCount;

			_simulation->RemoveResourceGlobal_WithMoney(_townId, trainingCost.resourceEnum, resourceToSpend);
		}
		else {
			if (trainingCost.resourceEnum == ResourceEnum::Money) {
				_simulation->AddPopupToFront(_playerId,
					NSLOCTEXT("TrainUnits", "NotEnoughMoneyToTrainUnit", "Not enough money to train this unit."),
					ExclusiveUIEnum::TrainUnitsUI, "PopupCannot"
				);
			}
			else
			{
				_simulation->AddPopupToFront(_playerId,
					NSLOCTEXT("TrainUnits", "NotEnoughResourceToTrainUnit", "Not enough resource to train this unit."),
					ExclusiveUIEnum::TrainUnitsUI, "PopupCannot"
				);
			}
			return;
		}
	}

	// Fill the old stack
	if (trainUnitsQueue.size() > 0 &&
		trainUnitsQueue.back().cardEnum == cardEnum)
	{
		trainUnitsQueue.back().stackSize += trainingCount;
		return;
	}

	// Make a new stack
	if (trainUnitsQueue.size() < 5)
	{
		CardStatus cardStatus;
		cardStatus.cardEnum = cardEnum;
		cardStatus.stackSize = trainingCount;
		cardStatus.cardBirthTicks = Time::Ticks();

		trainUnitsQueue.push_back(cardStatus);
	}
}

#undef LOCTEXT_NAMESPACE