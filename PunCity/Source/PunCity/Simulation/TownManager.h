// Pun Dumnernchanvanit's

#pragma once

#include "IGameSimulationCore.h"
#include "PlayerParameters.h"
#include "TreeSystem.h"
#include "PunCity/NetworkStructs.h"
#include "PunCity/DisplaySystem/PunTerrainGenerator.h"
#include <algorithm>
#include "PunCity/CppUtils.h"
#include <numeric>
#include "ProvinceSystem.h"
#include "GameRegionSystem.h"


using namespace std;

static const int32_t TownSizeMinPopulation[]
{
	0,
	25,
	50,
	100,
	150,
	200,
};
static int32 GetTownSizeMinPopulation(int32 tier) { return TownSizeMinPopulation[tier]; }


// Priority list for what job need people living closer
static const std::vector<CardEnum> DefaultJobPriorityListAllSeason
{
	CardEnum::IntercityLogisticsHub,
	
	CardEnum::FruitGatherer,
	CardEnum::Farm,

	CardEnum::MushroomFarm,
	CardEnum::Fisher,
	CardEnum::HuntingLodge,

	CardEnum::RanchCow,
	CardEnum::RanchSheep,
	CardEnum::RanchPig,

	CardEnum::Windmill,
	CardEnum::Bakery,

	CardEnum::StoneToolShop,
	CardEnum::Blacksmith,
	CardEnum::Herbalist,
	CardEnum::MedicineMaker,

	CardEnum::Forester,
	CardEnum::CharcoalMaker,
	CardEnum::BeerBrewery,
	CardEnum::ClayPit,
	CardEnum::Potter,
	CardEnum::Tailor,

	CardEnum::ShroomFarm,
	CardEnum::VodkaDistillery,
	CardEnum::CoffeeRoaster,

	CardEnum::GoldMine,
	CardEnum::Quarry,
	CardEnum::CoalMine,
	CardEnum::IronMine,

	CardEnum::IronSmelter,
	CardEnum::GoldSmelter,
	CardEnum::Mint,
	CardEnum::InventorsWorkshop,

	CardEnum::FurnitureWorkshop,
	CardEnum::Chocolatier,
	CardEnum::Winery,

	CardEnum::GemstoneMine,
	CardEnum::Jeweler,

	CardEnum::Beekeeper,
	CardEnum::Brickworks,
	CardEnum::CandleMaker,
	CardEnum::CottonMill,
	CardEnum::GarmentFactory,
	CardEnum::PrintingPress,

	CardEnum::PaperMaker,

	CardEnum::Market,
	CardEnum::ShippingDepot,

	CardEnum::CardMaker,
	CardEnum::ImmigrationOffice,

	CardEnum::BarrackSwordman,
	CardEnum::BarrackArcher,

	CardEnum::RegionShrine,
};


struct ProvinceClaimProgress
{
	int32 provinceId = -1;
	int32 attackerPlayerId = -1;

	int32 committedInfluencesAttacker = 0;
	int32 committedInfluencesDefender = 0;

	int32 ticksElapsed = 0;

	bool isValid() { return provinceId != -1; }

	void Tick1Sec(IGameSimulationCore* simulation)
	{
		//int32 attackerScaled = committedInfluencesAttacker * simulation->

		if (committedInfluencesAttacker > committedInfluencesDefender) {
			ticksElapsed += Time::TicksPerSecond * committedInfluencesAttacker / (committedInfluencesDefender + BattleInfluencePrice);
		}
		else {
			ticksElapsed -= Time::TicksPerSecond * (committedInfluencesDefender + BattleInfluencePrice) / committedInfluencesAttacker;
		}
	}

	//! Serialize
	FArchive& operator>>(FArchive &Ar) {
		Ar << provinceId;
		Ar << attackerPlayerId;
		Ar << committedInfluencesAttacker;
		Ar << committedInfluencesDefender;
		Ar << ticksElapsed;
		return Ar;
	}
};


/**
 * 
 */
class TownManager
{
public:
	TownManager(int32 playerId, int32 townId, IGameSimulationCore* simulation)
	{
		_playerId = playerId;
		_townId = townId;
		PUN_LOG("Create TownManager pid:%d tid:%d", playerId, townId);
		
		_simulation = simulation;
		taxLevel = 2;

		_aveHappinessModifiers.resize(HappinessModifierName.Num());

		incomes100.resize(IncomeEnumCount);
		influenceIncomes100.resize(InfluenceIncomeEnumCount);
		sciences100.resize(ScienceEnumCount);

		_enumToStatVec.resize(static_cast<int>(PlotStatEnum::Count));

		_nextDiseaseCheckTick = 0;

		_houseResourceAllowed.resize(static_cast<int>(ResourceEnumCount), true);

		_resourceEnumToOutputTarget.resize(static_cast<int>(ResourceEnumCount), -1);
		_resourceEnumToOutputTargetDisplay.resize(static_cast<int>(ResourceEnumCount), -1);

		_jobPriorityList = DefaultJobPriorityListAllSeason;

		_lastResourceCounts.resize(ResourceEnumCount);

		_jobBuildingEnumToIds.resize(BuildingEnumCount);
	}

	/*
	 * Tick
	 */
	void TickRound();

	void Tick1Sec();

	void Tick()
	{
		if (_needTaxRecalculation) {
			_needTaxRecalculation = false;
			RecalculateTax(false);
		}
	}

	/*
	 * Get Variables
	 */
	int32 playerId() { return _playerId; }

	bool hasTownhall() { return townHallId != -1; }

	bool isCapital() { return _townId == _playerId; }
	
	const std::vector<int32>& provincesClaimed() { return _provincesClaimed; }
	
	
	int32 aveFoodHappiness() { return _aveFoodHappiness; }
	int32 aveHeatHappiness() { return _aveHeatHappiness; }
	int32 aveHousingHappiness() { return _aveHousingHappiness; }
	int32 aveFunHappiness() { return _aveFunHappiness; }
	int32 aveNeedHappiness() { return (_aveFoodHappiness + _aveHeatHappiness + _aveHousingHappiness + _aveFunHappiness) / 4; }

	int32 aveHappinessModifier(HappinessModifierEnum modifierEnum) {
		return _aveHappinessModifiers[static_cast<int>(modifierEnum)];
	}
	int32 aveHappinessModifierSum() {
		int32 sum = 0;
		for (size_t i = 0; i < _aveHappinessModifiers.size(); i++) {
			sum += _aveHappinessModifiers[i];
		}
		return sum;
	}

	int32 aveHappiness() { return aveNeedHappiness() + aveHappinessModifierSum(); }


	const std::vector<int32>& adultIds() { return _adultIds; }
	const std::vector<int32>& childIds() { return _childIds; }

	int32 totalRevenue100() {
		int32 revenue100 = 0;
		for (size_t i = 0; i < incomes100.size(); i++) {
			if (incomes100[i] > 0) revenue100 += incomes100[i];
		}
		revenue100 = revenue100 * taxPercent() / 100;
		return revenue100;
	}

	int32 totalRevenue100WithoutHouse() {
		int32 revenue100 = 0;
		for (size_t i = HouseIncomeEnumCount; i < incomes100.size(); i++) {
			if (incomes100[i] > 0) revenue100 += incomes100[i];
		}
		revenue100 = revenue100 * taxPercent() / 100;
		return revenue100;
	}
	int32 totalRevenue100HouseOnly() {
		int32 revenue100 = 0;
		for (size_t i = 0; i < HouseIncomeEnumCount; i++) {
			if (incomes100[i] > 0) revenue100 += incomes100[i];
		}
		revenue100 = revenue100 * taxPercent() / 100;
		return revenue100;
	}

	int32 totalExpense100() {
		int32 expense100 = 0;
		for (size_t i = 0; i < incomes100.size(); i++) {
			if (incomes100[i] < 0) expense100 += std::abs(incomes100[i]);
		}
		return expense100;
	}

	int32 totalIncome100() {
		return totalRevenue100() - totalExpense100();
	}

	void RecalculateTaxDelayed() {
		_needTaxRecalculation = true;
	}
	void RecalculateTax(bool showFloatup);

	void ChangeIncome(int32 changeAmount, bool showFloatup, WorldTile2 floatupTile)
	{
		incomes100[static_cast<int>(IncomeEnum::BuildingUpkeep)] += changeAmount * 100;

		if (showFloatup) {
			_simulation->uiInterface()->ShowFloatupInfo(FloatupInfo(FloatupEnum::GainMoney, Time::Ticks(), floatupTile, TEXT_NUMSIGNED(changeAmount)));
		}
	}

	int32 totalInfluenceIncome100()
	{
		int32 influence100 = 0;
		for (size_t i = 0; i < influenceIncomes100.size(); i++) {
			influence100 += influenceIncomes100[i];
		}
		return influence100;
	}

	/*
	 * Population
	 */
	 //! Population
	int32 population() { return adultPopulation() + childPopulation(); }

	int32 adultPopulation() { return _adultIds.size(); }
	int32 childPopulation() { return _childIds.size(); }

	FText GetTownSizeSuffix();
	FText GetTownSizeName();

	void PlayerAddHuman(int32 objectId);
	void PlayerRemoveHuman(int32 objectId) {
		auto found = std::find(_adultIds.begin(), _adultIds.end(), objectId);
		if (found != _adultIds.end()) {
			_adultIds.erase(found);
		}
		//_humanIds.erase(std::remove(_humanIds.begin(), _humanIds.end(), objectId));

		auto foundChild = std::find(_childIds.begin(), _childIds.end(), objectId);
		if (foundChild != _childIds.end()) {
			_childIds.erase(foundChild);
		}

		_simulation->QuestUpdateStatus(_playerId, QuestEnum::PopulationQuest, 0);

		RefreshJobDelayed();
		RecalculateTaxDelayed();
	}

	//! Houses
	void PlayerAddHouse(int32_t objectId) {
		// PUN_LOG("PlayerAddHouse");
		_houseIds.push_back(objectId);
	}
	void PlayerRemoveHouse(int32_t objectId) {
		auto found = std::find(_houseIds.begin(), _houseIds.end(), objectId);
		check(found != _houseIds.end());
		_houseIds.erase(found);
	}

	void PlayerAddJobBuilding(class Building& building, bool isConstructed);
	void PlayerRemoveJobBuilding(class Building& building, bool isConstructed);

	void PlayerAddBonusBuilding(int32 buildingId) {
		//_bonusBuildingIds.push_back(buildingId);
	}
	void PlayerRemoveBonusBuilding(int32 buildingId) {

	}

	int32 housingCapacity();
	int32 employedCount_WithBuilder() { return _employedCount; }
	int32 laborerCount() { return _adultIds.size() - _employedCount; }
	int32 builderCount() { return _builderCount; }
	int32 roadMakerCount() { return _roadMakerIds.size(); }

	int32 employedCount_WithoutBuilder() { return employedCount_WithBuilder() - builderCount(); }

	// Job changes when: 
	//  - Slots changed: building constructed/demolish/upgrade, human global priority manip, human building priority manip, human building set job manip
	//  - Population changed: Add/Remove Adults
	//  
	// Refresh at:
	//  - building constructed/demolish/upgrade: PlayerAddJobBuilding, PlayerRemoveJobBuilding, SetJobBuilding
	//  - human global priority manip: SetTownPriority
	//  - human building priority manip: SetPriority
	//  - human building set job manip: JobSlotChange
	//
	//  - Add/Remove Adults: PlayerAddHuman, PlayerRemoveHuman
	//  Note: child promotion done at beginning of RefreshJob
	void RefreshJobDelayed() {
		_needRefreshJob = true;
	}
	void RefreshJobs();


	void RefreshHousing();

	void FillHouseSlots_FromWorkplace(std::vector<int32>& tempHumanIds);


	



	

	/*
	 * Job
	 */

	const std::vector<std::vector<int32>>& jobBuildingEnumToIds() const { return _jobBuildingEnumToIds; }
	const std::vector<int32>& constructionIds() const { return _constructionIds; }

	const std::vector<int32>& roadConstructionIds() { return _roadConstructionIds; }

	int32 jobBuildingCount()
	{
		int32 jobBuildingCount = 0;
		for (size_t i = 0; i < _jobBuildingEnumToIds.size(); i++) {
			jobBuildingCount += _jobBuildingEnumToIds[i].size();
		}
		return jobBuildingCount;
	}


	const std::vector<CardEnum>& GetJobPriorityList() {
		return _jobPriorityList;
	}
	const std::vector<CardEnum>& GetLaborerPriorityList() {
		return _laborerPriorityList;
	}

	void SetGlobalJobPriority(FSetGlobalJobPriority command)
	{
		//if (command.jobPriorityList.Num() == 3) {
		//	TArray<int32> laborerPriorityListInt = command.jobPriorityList;
		//
		//	return;
		//}

		_jobPriorityList.clear();
		for (int32 i = 0; i < command.jobPriorityList.Num(); i++) {
			_jobPriorityList.push_back(static_cast<CardEnum>(command.jobPriorityList[i]));
		}

		// TODO: This is to fix the jobPriority disappearing
		if (_jobPriorityList.size() < DefaultJobPriorityListAllSeason.size())
		{
			// Find the missing jobPriority Row and add it to the end
			for (size_t i = 0; i < DefaultJobPriorityListAllSeason.size(); i++) {
				CardEnum cardEnum = DefaultJobPriorityListAllSeason[i];

				auto found = std::find(_jobPriorityList.begin(), _jobPriorityList.end(), cardEnum);
				if (found == _jobPriorityList.end()) {
					_jobPriorityList.push_back(cardEnum);
				}
			}
		}


		RefreshJobDelayed();

#if WITH_EDITOR
		for (int32 i = 0; i < _jobPriorityList.size(); i++) {
			PUN_LOG("_jobPriorityList %s", *(GetBuildingInfo(_jobPriorityList[i]).name.ToString()));
		}
#endif
	}

	void AddDeliverySource(int32 deliverySource) {
		CppUtils::TryAdd(_deliverySources, deliverySource);
	}
	void RemoveDeliverySource(int32 deliverySource) {
		CppUtils::TryRemove(_deliverySources, deliverySource);
	}
	const std::vector<int32>& allDeliverySources() {
		return _deliverySources;
	}

	std::shared_ptr<FSetTownPriority> CreateTownPriorityCommand()
	{
		auto command = std::make_shared<FSetTownPriority>();
		command->laborerPriority = targetLaborerHighPriority;
		command->builderPriority = targetBuilderHighPriority;
		command->roadMakerPriority = targetRoadMakerHighPriority;

		command->targetLaborerCount = targetLaborerCount;
		command->targetBuilderCount = targetBuilderCount;
		command->targetRoadMakerCount = targetRoadMakerCount;
		return command;
	}
	

	/*
	 * Science
	 */
	int32 science100PerRound() {
		return std::accumulate(sciences100.begin(), sciences100.end(), 0);
	}


	/*
	 * Stats
	 */
	const std::vector<int32>& GetPlotStatVec(PlotStatEnum roundStatEnum) const
	{
		return _enumToStatVec[static_cast<int>(roundStatEnum)];
	}

	/*
	 * Tax
	 */
	int32_t taxHappinessModifier()
	{
		switch (taxLevel)
		{
		case 0: return 10;
		case 1: return 5;
		case 2: return 0;
		case 3: return -20;
		case 4: return -40;
		default:
			UE_DEBUG_BREAK();
		}
		return -1;
	}
	int32_t taxPercent() {
		switch (taxLevel)
		{
		case 0: return 50;
		case 1: return 75;
		case 2: return 100;
		case 3: return 125;
		case 4: return 150;
		default:
			UE_DEBUG_BREAK();
		}
		return -1;
	}

	/*
	 * Data
	 */
	void AddDataPoint(PlotStatEnum statEnum, int32 data, int32 ticksPerStatInterval = TicksPerStatInterval)
	{
		std::vector<int32>& statVec = _enumToStatVec[static_cast<int>(statEnum)];
		int32 statTickCount = Time::Ticks() / ticksPerStatInterval;

		//while (statVec.size() < statTickCount) {
		for (int32 i = statVec.size(); i < statTickCount; i++) {
			statVec.push_back(0); // PlayerOwned might not be initialized from the first round...
		}

		statVec.push_back(data);
	};


	/*
	 * Resource Management
	 */
	bool GetHouseResourceAllow(ResourceEnum resourceEnum) {
		return _houseResourceAllowed[static_cast<int>(resourceEnum)];
	}
	void SetHouseResourceAllow(ResourceEnum resourceEnum, bool resourceAllowed);

	int32 GetOutputTarget(ResourceEnum resourceEnum) {
		return _resourceEnumToOutputTarget[static_cast<int>(resourceEnum)];
	}
	void SetOutputTarget(ResourceEnum resourceEnum, int32 amount) {
		_resourceEnumToOutputTarget[static_cast<int>(resourceEnum)] = amount;
	}

	int32 GetOutputTargetDisplay(ResourceEnum resourceEnum) { return _resourceEnumToOutputTargetDisplay[static_cast<int>(resourceEnum)]; }
	void SetOutputTargetDisplay(ResourceEnum resourceEnum, int32 amount) {
		_resourceEnumToOutputTargetDisplay[static_cast<int>(resourceEnum)] = amount;
	}

	/*
	 * Migration
	 */
	void AddMigrationPendingCount(int32 count) {
		_migrationPendingCount += count;
	}

	/*
	 * Claim Province
	 */
	void TryRemoveProvinceClaim(int32 provinceId, bool lightMode)
	{
		CppUtils::TryRemove(_provincesClaimed, provinceId);
		//_claimedProvinceConnected.erase(provinceId);

		if (!lightMode) {
			RecalculateTax(false);
		}
	}

	void ClaimProvince(int32 provinceIdIn, bool lightMode = false)
	{
		//PUN_LOG("ClaimProvince province:%d pid:%d", provinceId, _playerId);

		//WorldTile2 minTile = region.minTile();
		//WorldTile2 maxTile = region.maxTile();
		//if (_regionsClaimed.size() == 0) {
		//	_territoryBoxExtent = TileArea(minTile.x, minTile.y, maxTile.x, maxTile.y);
		//}
		//_territoryBoxExtent = TileArea(std::min(_territoryBoxExtent.minX, minTile.x), std::min(_territoryBoxExtent.minY, minTile.y),
		//								std::max(_territoryBoxExtent.maxX, maxTile.x), std::max(_territoryBoxExtent.maxY, maxTile.y));

		_provincesClaimed.push_back(provinceIdIn);

		//WorldTile2 townGate = _simulation->townhallGateTile(_playerId);
		//WorldTile2 provinceCenter = _simulation->GetProvinceCenterTile(provinceId);
		//WorldTile2 availableProvinceTile = AlgorithmUtils::FindNearbyAvailableTile(provinceCenter, [&](const WorldTile2& tile) {
		//	return _simulation->pathAI(true)->isWalkable(tile.x, tile.y);
		//}, 9);
		//DEBUG_ISCONNECTED_VAR(ClaimProvince);
		//_claimedProvinceConnected.emplace(provinceId, _simulation->IsConnected(townGate, availableProvinceTile, GameConstants::MaxFloodDistance_HumanLogistics, true));

		// Check for number of provinces from townhall
		//  Use Breadth-first search
		auto& regionSys = _simulation->regionSystem();
		
		for (int32 provinceId : _provincesClaimed) {
			regionSys.SetProvinceDistance(provinceId, -1);
		}

		int32 level = 0;
		std::vector<int32> curProvinceIds;
		curProvinceIds.push_back(_provincesClaimed[0]);
		regionSys.SetProvinceDistance(_provincesClaimed[0], level);
		
		while (level < 7) 
		{
			level++;
			
			std::vector<int32> nextProvinceIds;
			for (int32 curProvinceId : curProvinceIds) {
				const std::vector<ProvinceConnection>& connections = _simulation->GetProvinceConnections(curProvinceId);
				for (const ProvinceConnection& connection : connections) {
					if (connection.tileType == TerrainTileType::River ||
						connection.tileType == TerrainTileType::None)
					{
						// If this province is claimed by us
						if (_simulation->provinceOwnerTown(connection.provinceId) == _townId) 
						{
							// Set the provincesFromTownhall if needed
							for (int32 i = 0; i < _provincesClaimed.size(); i++) {
								if (connection.provinceId == _provincesClaimed[i] &&
									regionSys.provinceDistance(connection.provinceId) == -1)
								{
									regionSys.SetProvinceDistance(connection.provinceId, level);
									nextProvinceIds.push_back(connection.provinceId);
								}
							}
						}
					}
				}
			}
			
			curProvinceIds = nextProvinceIds;
		}

		//// DEBUG
		//for (int32 i = 0; i < _provincesClaimed.size(); i++) {
		//	PUN_LOG("Refresh province:%d distance:%d", _provincesClaimed[i], _provincesFromTownhall[i]);
		//}
		
		if (lightMode) {
			return;
		}

		RecalculateTax(false);
	}

	/*
	 * Townhall cards
	 */

	bool CanAddCardToTownhall() {
		return _cardsInTownhall.size() < _simulation->GetTownLvl(_townId);
	}

	void AddCardToTownhall(CardStatus card);

	CardEnum RemoveCardFromTownhall(int32 slotIndex) {
		if (slotIndex < _cardsInTownhall.size()) {
			CardEnum cardEnum = _cardsInTownhall[slotIndex].cardEnum;
			_cardsInTownhall.erase(_cardsInTownhall.begin() + slotIndex);
			return cardEnum;
		}
		return CardEnum::None;
	}

	bool TownhallHasCard(CardEnum cardEnum)
	{
		for (size_t i = 0; i < _cardsInTownhall.size(); i++) {
			if (_cardsInTownhall[i].cardEnum == cardEnum) {
				return true;
			}
		}
		return false;
	}

	std::vector<CardStatus> cardsInTownhall() {
		return _cardsInTownhall;
	}

	int32 maxTownhallCards() {
		return _simulation->GetTownLvl(_townId);
	}

	int32 TownhallCardCount(CardEnum cardEnum)
	{
		int32 count = 0;
		for (size_t i = 0; i < _cardsInTownhall.size(); i++) {
			if (_cardsInTownhall[i].cardEnum == cardEnum) {
				count++;
			}
		}
		return count;
	}

	

	/*
	 * Others
	 */
	TileArea territoryBoxExtent() { return _territoryBoxExtent; }

	int32 GetPlayerLandTileCount(bool includeMountain) {
		int32 tileCount = 0;
		auto& provinceSys = _simulation->provinceSystem();
		for (int32 provinceId : _provincesClaimed) {
			tileCount += provinceSys.provinceFlatTileCount(provinceId);
			if (includeMountain) {
				tileCount += provinceSys.provinceMountainTileCount(provinceId);
			}
		}
		return tileCount;
	}

private:
	/*
	 * Private Functions
	 */
	// Check buildings
	int32 TryFillJobBuildings(const std::vector<int32>& jobBuildingIds, PriorityEnum priority, int& index, bool shouldDoDistanceSort = true, int32 maximumFill = INT32_MAX);

	void CollectRoundIncome();
	void CollectHouseIncome();

	int32 targetNonLaborerCount()
	{
		if (targetLaborerHighPriority) {
			return _adultIds.size() - targetLaborerCount;
		}
		return _adultIds.size();
	}

	/*
	 * Influence Price
	 */
	 // TODO: Other way to prevent strip empire?
	 // TODO: Border Province Count
	//int32 distanceFromCapital_PenaltyPercent(int32 provinceId)
	//{
	//	// Distance from Capital
	//	// Beyond X tiles from capital, we get penalty
	//	WorldTile2 townhallCenter = _simulation->townhallGateTile(_playerId);
	//	WorldTile2 provinceCenter = _simulation->GetProvinceCenterTile(provinceId);

	//	const int32 maxIncreasePercent = 100;
	//	const int32 penaltyStartDistance = 200;
	//	const int32 penaltyLerpDistance = 300;
	//	int32 distanceFromCapital = WorldTile2::Distance(townhallCenter, provinceCenter);
	//	return std::min(std::max(0, distanceFromCapital - penaltyStartDistance) * maxIncreasePercent / penaltyLerpDistance, maxIncreasePercent);
	//}

	//// TODO: Include this in Influence increment
	//int32 areaVsPopulation_PenaltyPercent()
	//{
	//	// Total Influence Area vs Population
	//	// Penalty applies when population is too small for empire.
	//	// 500 pop = 250,000 tiles empire ideal??
	//	const int32 tilesPerPopulation_minPenalty = 500;
	//	const int32 tilesPerPopulation_lerp = 1000;
	//	const int32 maxPenaltyPercent = 100;
	//	int32 tilesPerPopulation = GetPlayerLandTileCount(false) / std::max(1, population());
	//	return std::min(std::max(0, tilesPerPopulation - tilesPerPopulation_minPenalty) * maxPenaltyPercent / tilesPerPopulation_lerp, maxPenaltyPercent);
	//}

public:
	/*
	 * Serialize
	 */
	void Serialize(FArchive& Ar)
	{
		//! Public
		Ar << townHallId;

		SerializeVecValue(Ar, _provincesClaimed);
		//SerializeVecValue(Ar, _provincesFromTownhall);

		SerializeVecValue(Ar, incomes100);
		SerializeVecValue(Ar, sciences100);
		SerializeVecValue(Ar, influenceIncomes100);

		Ar << taxLevel;

		Ar << targetLaborerHighPriority;
		Ar << targetBuilderHighPriority;
		Ar << targetRoadMakerHighPriority;
		Ar << targetLaborerCount;
		Ar << targetBuilderCount;
		Ar << targetRoadMakerCount;

		//! Private
		Ar << _needTaxRecalculation;
		Ar << _needRefreshJob;

		SerializeVecValue(Ar, _lastResourceCounts);

		SerializeVecValue(Ar, _adultIds);
		SerializeVecValue(Ar, _childIds);
		SerializeVecValue(Ar, _houseIds);

		SerializeVecVecValue(Ar, _jobBuildingEnumToIds);
		SerializeVecValue(Ar, _roadConstructionIds);
		SerializeVecValue(Ar, _constructionIds);
		SerializeVecValue(Ar, _bonusBuildingIds);

		SerializeVecValue(Ar, _jobPriorityList);
		SerializeVecValue(Ar, _laborerPriorityList);

		SerializeVecValue(Ar, _deliverySources);

		Ar << _employedCount;
		Ar << _builderCount;
		SerializeVecValue(Ar, _roadMakerIds);

		Ar << _aveFoodHappiness;
		Ar << _aveHeatHappiness;
		Ar << _aveHousingHappiness;
		Ar << _aveFunHappiness;

		SerializeVecValue(Ar, _aveHappinessModifiers);

		//SerializeMapValue(Ar, _claimedProvinceConnected);

		_territoryBoxExtent >> Ar;

		// Stats
		SerializeVecVecValue(Ar, _enumToStatVec);

		Ar << _nextDiseaseCheckTick;

		SerializeVecValue(Ar, _houseResourceAllowed);

		SerializeVecValue(Ar, _resourceEnumToOutputTarget);
		SerializeVecValue(Ar, _resourceEnumToOutputTargetDisplay);

		Ar << _migrationPendingCount;

		SerializeVecObj(Ar, _cardsInTownhall);;
	}

public:
	int32 townHallId = -1;

	std::vector<int32> _provincesClaimed;
	//std::vector<int32> _provincesFromTownhall; // 

	std::vector<int32> incomes100;
	std::vector<int32> sciences100;
	std::vector<int32> influenceIncomes100;

	// Tax Level
	int32 taxLevel = 2;

	// Laborer/Builder adjustments
	// normally Laborer and Builder counts are shown
	// Builder count is from summing up constructors in buildings
	// if star high priority, that means we want to set a target, in that case, fraction target appears along with adjustment arrows
	// if starred laborer, building slots filling will terminate once the targetLaborerCount is hit
	// if starred builder, 
	bool targetLaborerHighPriority = false;
	bool targetBuilderHighPriority = false;
	bool targetRoadMakerHighPriority = false;
	int32 targetLaborerCount = 1;
	int32 targetBuilderCount = 1;
	int32 targetRoadMakerCount = 1;
	
private:
	bool _needTaxRecalculation = false;
	bool _needRefreshJob = false;

	std::vector<int32> _lastResourceCounts; // Use this to check if resource goes from 0 to nonzero.

	std::vector<int32> _childIds;
	std::vector<int32> _adultIds;
	std::vector<int32> _houseIds;

	std::vector<std::vector<int32>> _jobBuildingEnumToIds;
	std::vector<int32> _constructionIds;
	std::vector<int32> _roadConstructionIds;
	std::vector<int32> _bonusBuildingIds;

	std::vector<CardEnum> _jobPriorityList;
	std::vector<CardEnum> _laborerPriorityList;

	std::vector<int32> _deliverySources; // Doesn't count shipping depot

	//

	int32 _employedCount = 0;
	int32 _builderCount = 0;
	std::vector<int32> _roadMakerIds;

	int32 _aveFoodHappiness = 0;
	int32 _aveHeatHappiness = 0;
	int32 _aveHousingHappiness = 0;
	int32 _aveFunHappiness = 0;

	std::vector<int32> _aveHappinessModifiers;

	//std::unordered_map<int32, int32> _claimedProvinceConnected; // Fast check if the province is too far from townhall
	TileArea _territoryBoxExtent;

	// 
	std::vector<std::vector<int32>> _enumToStatVec;

	int32 _nextDiseaseCheckTick;

	std::vector<uint8> _houseResourceAllowed;

	std::vector<int32> _resourceEnumToOutputTarget;
	std::vector<int32> _resourceEnumToOutputTargetDisplay; // Display changes instantly

	int32 _migrationPendingCount = 0;

	std::vector<CardStatus> _cardsInTownhall;

private:
	/*
	 * Non-Serialize
	 */
	IGameSimulationCore* _simulation;
	int32 _playerId;
	int32 _townId;
};
