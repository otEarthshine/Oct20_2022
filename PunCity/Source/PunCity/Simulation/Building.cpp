#include "Building.h"
#include "Resource/ResourceSystem.h"
#include "StatSystem.h"
#include "PunCity/GameConstants.h"
#include "OverlaySystem.h"
#include "TreeSystem.h"
#include "BuildingCardSystem.h"
#include "PlayerOwnedManager.h"
#include "WorldTradeSystem.h"
#include "UnlockSystem.h"


using namespace std;

Building::~Building() {}

void Building::Init(IGameSimulationCore& simulation, int objectId, int32_t playerId, uint8_t buildingEnum,
					TileArea area, WorldTile2 centerTile, Direction faceDirection)
{
	_objectId = objectId;
	_playerId = playerId;
	_buildingEnum = static_cast<CardEnum>(buildingEnum);
	_centerTile = centerTile;
	_faceDirection = faceDirection;
	_area = area;

	_simulation = &simulation;

	_filledInputs = false;

	_buildingPlacedTick = Time::Ticks();
	
	//workplaceNeedInput = false;
	workplaceInputNeeded = ResourceEnum::None;

	//useTools = false;
	lastWorkedOn = -10000;

	_upgrades.clear();

	_priority = PriorityEnum::NonPriority;
	_cardSlot1 = CardStatus();

	BldInfo info = BuildingInfo[buildingEnumInt()];
	// Construction
	_maxOccupants = info.maxBuilderCount;
	_allowedOccupants = _maxOccupants;
	_isConstructed = false;
	_didSetWalkable = false;

	if (_playerId == -1) {
		return;  // Animal Controlled
	}

	//PUN_LOG("Building start: %s", *FString(info.name.c_str()));
		
	simulation.PlayerAddJobBuilding(_playerId, *this, false);

	// Mark area for land clearing
	simulation.treeSystem().MarkArea(_playerId, _area, false, ResourceEnum::None);
	
	// Set initial construction resource
	vector<int32> constructionCosts = GetConstructionResourceCost();
	for (size_t i = 0; i < _countof(ConstructionResources); i++) {
		if (constructionCosts[i] > 0) AddResourceHolder(ConstructionResources[i], ResourceHolderType::Requester, constructionCosts[i]);
	}

	OnInit();

	// Clear area for TrailerMode
	if (PunSettings::TrailerMode()) {
		InstantClearArea();

		// Insta build for storage yard to accommodate AddResource
		if (IsStorage(_buildingEnum)) {
			FinishConstruction();
		}
	}
	else
	{
		// Instant build HumanitarianAidCamp
		bool cheatBuild = SimSettings::IsOn("CheatFastBuild") && !IsRoad(_buildingEnum);

		if (cheatBuild ||
			_buildingEnum == CardEnum::HumanitarianAidCamp ||
			IsDecorativeBuilding(_buildingEnum))
		{
			InstantClearArea();
			FinishConstruction();
		}
	}

	InitStatistics();

	_simulation->SetNeedDisplayUpdate(DisplayClusterEnum::Overlay, _centerTile.regionId(), true);
	

	// If land is cleared, just SetWalkable without needing ppl to do this.
	// This allows multiple roads to be queued without failing workplace.didSetWalkable()
	if (_simulation->IsLandCleared_SmallOnly(_playerId, area) &&
		_simulation->IsLandCleared_SmallOnly(_playerId, frontArea())) 
	{
		SetAreaWalkable();
	}

	/*
	 * Quest update
	 */
	if (IsAgricultureBuilding(_buildingEnum)) {
		_simulation->QuestUpdateStatus(_playerId, QuestEnum::FoodBuildingQuest);
	}
	

	PUN_DEBUG_EXPR(buildingInfo_ = info);
}

void Building::SetAreaWalkable() {
	// Body
	_area.ExecuteOnArea_WorldTile2([&](WorldTile2 tile) {
		_simulation->SetWalkable(tile, false);
	});

	// Gate
	_simulation->SetWalkable(gateTile(), true);

	_simulation->ResetUnitActionsInArea(_area);

	// Fence gate's center should be walkable
	if (isEnum(CardEnum::FenceGate)) {
		_simulation->SetWalkable(_centerTile, true);
	}

	_didSetWalkable = true;
}

void Building::InstantClearArea()
{
	//auto& treeSystem = _simulation->treeSystem();
	_area.ExecuteOnArea_WorldTile2([&](WorldTile2 tile) {
		//ResourceTileType type = treeSystem.tileInfo(tile.tileId()).type;
		//if (type == ResourceTileType::Bush) {
		//	treeSystem.UnitHarvestBush(tile);
		//}
		//else if (type == ResourceTileType::Tree || type == ResourceTileType::Deposit) {
		//	treeSystem.UnitHarvestTreeOrDeposit(tile);
		//}
		_simulation->dropSystem().ForceRemoveDrop(tile);
	});
	
	_simulation->treeSystem().ForceRemoveTileObjArea(_area);
}

void Building::FinishConstruction()
{
	//PUN_LOG("FinishConstruction %d constructed:%d", _objectId, _isConstructed);
	
	_maxOccupants = 0;
	_allowedOccupants = 0;
	_workDone100 = 0;
	_isConstructed = true;
	

	// Do not reset roadMakers since they should construct multiple roads
	if (!IsRoad(_buildingEnum))
	{
		// In the case with other workReservers, reset them
		for (size_t i = 0; i < _workReservers.size(); i++) {
			_simulation->ResetUnitActions(_workReservers[i]);
		}
	}


	if (_playerId == -1) {
		return; // Animal Controlled
	}

	// Remove Construction Resource Holders
	vector<int32> constructionCosts = GetConstructionResourceCost();
	for (int i = constructionCosts.size(); i-- > 0;) {
		RemoveResourceHolder(ConstructionResources[i]);
	}

	ResetDisplay();

	// Kick out all the constructors
	ResetWorkReservers();
	_simulation->RemoveJobsFrom(buildingId(), false);
	_simulation->PlayerRemoveJobBuilding(_playerId, *this, false);

	//// if this building is not an instant build, claim the area
	//if (buildingInfo().buildManSecCost100 > 0) {
	//	_simulation->SetRegionOwner(_centerTile.region().regionId(), _playerId);
	//}

	// Auto-add inputs/outputs accordingly
	if (hasInput1()) AddResourceHolder(input1(), ResourceHolderType::Requester, baseInputPerBatch() + 10);
	if (hasInput2()) AddResourceHolder(input2(), ResourceHolderType::Requester, baseInputPerBatch() + 10);
	if (product() != ResourceEnum::None) AddResourceHolder(product(), ResourceHolderType::Provider, 0);
	//if (IsProducer(buildingEnum())) AddResourceHolder(ResourceEnum::Tools, ResourceHolderType::Provider, 0);

	// Add workers
	int32 workerCount = buildingInfo().workerCount;
	if (workerCount > 0) {
		AddJobBuilding(workerCount);
	}

	// Quest
	if (_buildingEnum == CardEnum::StorageYard) {
		_simulation->QuestUpdateStatus(_playerId, QuestEnum::BuildStorageQuest);
	}
}

int32 Building::resourceCount(ResourceEnum resourceEnum) 
{
	ResourceHolderInfo info = holderInfo(resourceEnum);
	PUN_CHECK(info.isValid());
	return resourceSystem().resourceCount(info);
}
int32 Building::tryResourceCount(ResourceEnum resourceEnum)
{
	ResourceHolderInfo info = holderInfo(resourceEnum);
	if (!info.isValid()) {
		return 0;
	}
	return resourceSystem().resourceCount(info);
}

void Building::SetResourceActive(bool workActive)
{
	//UE_LOG(LogTemp, Error, TEXT("OnSetWorkActive %d"), workActive);
	
	// Disabled Building shouldn't take resource
	if (!isConstructed()) 
	{
		vector<int32> constructionCosts = GetConstructionResourceCost();
		for (size_t i = 0; i < _countof(ConstructionResources); i++) {
			ResourceHolderInfo holderInfoTemp = holderInfo(ConstructionResources[i]);
			if (holderInfoTemp.isValid()) {
				SetResourceTarget(holderInfoTemp, workActive ? constructionCosts[i] : 0);
			}
		}
	}
	else
	{
		ResourceHolderInfo input1Info = holderInfo(input1());
		ResourceHolderInfo input2Info = holderInfo(input2());

		if (workActive) {
			if (input1Info.isValid()) SetResourceTarget(input1Info, 20);
			if (input2Info.isValid()) SetResourceTarget(input2Info, 20);
		}
		else {
			if (input1Info.isValid()) SetResourceTarget(input1Info, 0);
			if (input2Info.isValid()) SetResourceTarget(input2Info, 0);
		}
	}
}
void Building::SetHolderTypeAndTarget(ResourceEnum resourceEnum, ResourceHolderType type, int32 target)
{
	if (resourceEnum == ResourceEnum::Food)
	{
		for (ResourceEnum resourceEnumLocal : FoodEnums) {
			resourceSystem().SetHolderTypeAndTarget(holderInfo(resourceEnumLocal), type, target);
		}
		return;
	}
	
	if (resourceEnum == ResourceEnum::Luxury)
	{
		ExecuteOnLuxuryResources([&](ResourceEnum resourceEnumLocal) {
			resourceSystem().SetHolderTypeAndTarget(holderInfo(resourceEnumLocal), type, target);
		});
		return;
	}
	
	// ResourceEnum::None means All (Allow/Disallow All)
	if (resourceEnum == ResourceEnum::None) {
		for (int32 i = 0; i < ResourceEnumCount; i++) {
			ResourceHolderInfo info = holderInfo(static_cast<ResourceEnum>(i));
			resourceSystem().SetHolderTypeAndTarget(info, type, target);
		}
		return;
	}
	
	ResourceHolderInfo info = holderInfo(resourceEnum);
	resourceSystem().SetHolderTypeAndTarget(info, type, target);
}

void Building::Deinit()
{
	//PUN_LOG("Deinit %d constructed:%d ticks:%d", _objectId, _isConstructed, Time::Ticks());
	
	if (_playerId == -1) {
		OnDeinit();
		return; // Animal Controlled
	}

	// Despawn all resource holders
	ResourceSystem& resourceSys = resourceSystem();
	for (auto& info : _holderInfos) {
		resourceSys.DespawnHolder(info);
	}
	_holderInfos.clear();

	// Jobs
	// This is shared for all buildings for simplicity...
	// Note: House occupancy clearing is done in inherited function.
	if (_maxOccupants > 0 && !(IsHouse(buildingEnum()) && isConstructed()))  // skip constructed house
	{
		// _workReservers are people that may just changed job from this building but is still using it for whatever reason
		// Future: they could be the user of this building's service
		//for (int i = _workReservers.size(); i-- > 0;) {
		//	_simulation->ResetUnitActions(_workReservers[i]); // ResetActions will remove the workReserver through Unreserve
		//}
		//ResetWorkReservers();
		//for (int i = _occupantIds.size(); i-- > 0;) {
		//	_simulation->ResetUnitActions(_occupantIds[i]);
		//}

		ResetWorkReservers();
		_simulation->RemoveJobsFrom(buildingId(), false);
		_simulation->PlayerRemoveJobBuilding(_playerId, *this, _isConstructed);
	}

	_area.ExecuteOnArea_WorldTile2([&](WorldTile2 tile) {
		_simulation->SetWalkable(tile, true);
	});

	OnDeinit();
}

void Building::ResetWorkReservers()
{
	for (int i = _workReservers.size(); i-- > 0;) {
		_simulation->ResetUnitActions(_workReservers[i]); // ResetActions will remove the workReserver through Unreserve
	}
}
void Building::ResetOccupants() // For houses etc.
{
	for (int i = _occupantIds.size(); i-- > 0;) {
		_simulation->ResetUnitActions(_occupantIds[i]); // ResetActions will remove the workReserver through Unreserve
	}
}

void Building::TrailerAddResource()
{
	/*
 * Special case: Trailer AddResources from input/output of nearby buildings
 */
	if (PunSettings::TrailerSession)
	{
		bool alreadyHasResource = false;
		for (ResourceInfo info : ResourceInfos) {
			if (resourceCount(info.resourceEnum) > 0) {
				alreadyHasResource = true;
				break;
			}
		}

		if (!alreadyHasResource)
		{
			auto& buildingList = _simulation->buildingSubregionList();

			std::vector<ResourceEnum> resourceEnums;

			centerTile().region().ExecuteOnNearbyRegions([&](WorldRegion2 region)
			{
				buildingList.ExecuteRegion(region, [&](int32 buildingId)
				{
					if (resourceEnums.size() >= 2) {
						return;
					}

					Building& building = _simulation->building(buildingId);
					if (building.product() != ResourceEnum::None) {
						resourceEnums.push_back(building.product());
					}
					if (building.input1() != ResourceEnum::None) {
						resourceEnums.push_back(building.input1());
					}
					if (building.input2() != ResourceEnum::None) {
						resourceEnums.push_back(building.input2());
					}
				});
			});

			// Trailer AddResource
			size_t enumCount = std::min(static_cast<size_t>(std::min(storageSlotCount() / 2, 2)), resourceEnums.size());
			for (size_t i = 0; i < enumCount; i++) {
				AddResource(resourceEnums[i], GameConstants::StorageCountPerTile + GameRand::Rand(centerTile().tileId()) % GameConstants::StorageCountPerTile);
			}
		}
	}
}

//bool Building::NeedRoadConnection() {
//	CardEnum bldEnum = buildingEnum();
//	if (IsCritterBuildingEnum(bldEnum) ||
//		bldEnum == CardEnum::Fence ||
//		IsRoad(bldEnum) ||
//		IsRoadOverlapBuilding(bldEnum) ||
//		bldEnum == CardEnum::Farm ||
//		bldEnum == CardEnum::Bridge) {
//		return false;
//	}
//	return !area().GetFrontArea(_faceDirection).ExecuteOnAreaWithExit_WorldTile2([&](WorldTile2 tile) { return _simulation->overlaySystem().IsRoad(tile); });
//}

void Building::ChangeAllowedOccupants(int32 allowedOccupants)
{
	_allowedOccupants = Clamp(allowedOccupants, 0, _maxOccupants);
	
	// When allowedOccupants is 0, we pause the building
	SetResourceActive(_maxOccupants == 0 || _allowedOccupants > 0);
	PUN_LOG("ChangeAllowedOccupants %d/%d", _allowedOccupants, _maxOccupants);
}

void Building::AddResourceHolder(ResourceEnum resourceEnum, ResourceHolderType holderType, int target)
{
	//_LOG(PunResource, "AddResourceHolder: enum:%d type:%d target:%d", static_cast<int>(resourceEnum), static_cast<int>(holderType), target);
	
	auto foundOldHolder = find_if(_holderInfos.begin(), _holderInfos.end(), [&](ResourceHolderInfo& info) { return info.resourceEnum == resourceEnum; });
	if (foundOldHolder != _holderInfos.end()) {
		return;
	}

	//UE_LOG(LogTemp, Error, TEXT("AddResourceHolder %d, %d, %d"), (int)resourceEnum, (int)holderType, target);
	ResourceSystem& resourceSys = resourceSystem();
	resourceSys.CheckIntegrity_ResourceSys();

	
	int holderId = resourceSys.SpawnHolder(resourceEnum, holderType, _objectId, gateTile(), target);
	_holderInfos.push_back(ResourceHolderInfo(resourceEnum, holderId));
}

void Building::RemoveResourceHolder(ResourceEnum resourceEnum)
{
	//_LOG(PunResource, "RemoveResourceHolder: enum:%d", static_cast<int>(resourceEnum));
	
	for (size_t i = 0; i < _holderInfos.size(); i++) {
		if (_holderInfos[i].resourceEnum == resourceEnum) {
			resourceSystem().DespawnHolder(_holderInfos[i]);
			//PUN_LOG("RemoveResourceHolder1:%s _holderInfos:%d", *FString(BuildingInfo[buildingEnumInt()].name.c_str()), _holderInfos.size());
			_holderInfos.erase(_holderInfos.begin() + i);
			//PUN_LOG("RemoveResourceHolder2: _holderInfos:%d", _holderInfos.size());
			break;
		}
	}
}

void Building::AddResource(ResourceEnum resourceEnum, int amount){
	resourceSystem().AddResource(holderInfo(resourceEnum), amount);
	check(resourceSystem().resourceCount(resourceEnum) >= 0);
}
void Building::RemoveResource(ResourceEnum resourceEnum, int amount){
	resourceSystem().RemoveResource(holderInfo(resourceEnum), amount);
	check(resourceSystem().resourceCount(resourceEnum) >= 0);
}

int32 Building::GetResourceCount(ResourceHolderInfo holderInfo) {
	PUN_CHECK2(holderInfo.isValid(), debugStr());
	// TODO: temp...
	if (!holderInfo.isValid()) {
		return 0;
	}
	return resourceSystem().resourceCount(holderInfo);
}
int32 Building::GetResourceCountWithPush(ResourceHolderInfo holderInfo){
	return resourceSystem().resourceCountWithPush(holderInfo);
}
int32 Building::GetResourceTarget(ResourceHolderInfo holderInfo) {
	return resourceSystem().resourceTarget(holderInfo);
}
int32 Building::GetResourceCountWithPop(ResourceHolderInfo holderInfo) {
	return resourceSystem().resourceCountWithPop(holderInfo);
}

void Building::SetResourceTarget(ResourceHolderInfo holderInfo, int32 target) {
	PUN_CHECK(_simulation);
	resourceSystem().SetResourceTarget(holderInfo, target);
}

bool Building::NeedWork()
{
	ResourceHolderInfo info = holderInfo(product());
	PUN_CHECK2(info.isValid(), string("Probably no holder for product?"));
	int resourceCount = resourceSystem().resourceCountWithPush(info);

	// TODO: proper work limit
	return resourceCount <= GameConstants::WorkerEmptyBuildingInventoryAmount && MathUtils::SumVector(_workReserved) + _workDone100 < workManSecPerBatch100();
}

bool Building::NeedConstruct()
{
	PUN_CHECK(!isConstructed())
	return MathUtils::SumVector(_workReserved) + _workDone100 < buildTime_ManSec100();
}


bool Building::UpgradeBuilding(int upgradeIndex)
{
	PUN_ENSURE(upgradeIndex < _upgrades.size(), return false; );
	
	if (IsUpgraded(upgradeIndex)) {
		_simulation->AddPopup(_playerId, "Already upgraded");
		return false;
	}

	ResourcePair resourceNeeded = _upgrades[upgradeIndex].resourceNeeded;
	int32 moneyNeeded = _upgrades[upgradeIndex].moneyNeeded;
	
	// Resource Upgrade
	if (resourceNeeded.isValid())
	{
		PUN_CHECK(resourceNeeded.count > 0);
		if (resourceSystem().resourceCount(resourceNeeded.resourceEnum) >= resourceNeeded.count)
		{
			PUN_CHECK(moneyNeeded == 0);
			resourceSystem().RemoveResourceGlobal(resourceNeeded.resourceEnum, resourceNeeded.count);
			_upgrades[upgradeIndex].isUpgraded = true;

			ResetDisplay();

			//_simulation->AddPopup(_playerId, "Upgraded " + _upgrades[upgradeIndex].name);

			_simulation->soundInterface()->Spawn2DSound("UI", "UpgradeBuilding", _playerId, _centerTile);

			OnUpgradeBuilding(upgradeIndex);
			return true;
		}

		_simulation->AddPopupToFront(_playerId, { "Not enough " + ResourceName(resourceNeeded.resourceEnum) + " for upgrade." }, 
						ExclusiveUIEnum::None, "PopupCannot");
		return false;
	}
	
	
	// Money Upgrade
	PUN_CHECK(moneyNeeded > 0);
	if (resourceSystem().money() >= moneyNeeded)
	{
		resourceSystem().ChangeMoney(-moneyNeeded);
		_upgrades[upgradeIndex].isUpgraded = true;

		ResetDisplay();

		//_simulation->AddPopup(_playerId, "Upgraded " + _upgrades[upgradeIndex].name);

		_simulation->soundInterface()->Spawn2DSound("UI", "UpgradeBuilding", _playerId, _centerTile);

		OnUpgradeBuilding(upgradeIndex);
		return true;
	}

	_simulation->AddPopupToFront(_playerId, { "Not enough money for upgrade." }, ExclusiveUIEnum::None, "PopupCannot");
	return false;
}



void Building::DoWork(int unitId, int workAmount100)
{
	//ACTION_LOG(_objectId, TEXT("DoWork: unitId:%d"), objectId)

	_workDone100 += workAmount100;
	lastWorkedOn = Time::Ticks();

	// TODO: On Finish Work
	if (_isConstructed) 
	{
		if (_workDone100 >= workManSecPerBatch100()) 
		{
			// Special case: ConsumerWorkplace
			if (isEnum(CardEnum::Mint)) 
			{
				_workDone100 = 0;
				_filledInputs = false;
				
				int32 moneyReceived = productPerBatch();
				resourceSystem().ChangeMoney(moneyReceived);
				_simulation->worldTradeSystem().ChangeSupply(_playerId, ResourceEnum::GoldBar, inputPerBatch());

				_simulation->uiInterface()->ShowFloatupInfo(FloatupEnum::GainMoney, centerTile(), "+" + to_string(moneyReceived));
				AddProductionStat(moneyReceived);
				_simulation->SetNeedDisplayUpdate(DisplayClusterEnum::BuildingAnimation, _centerTile.regionId());
				return;
			}
			if (isEnum(CardEnum::InventorsWorkshop))
			{
				_workDone100 = 0;
				_filledInputs = false;

				int32 sciReceived = productPerBatch();
				_simulation->unlockSystem(_playerId)->Research(sciReceived * 100 * Time::SecondsPerRound, 1);

				_simulation->uiInterface()->ShowFloatupInfo(FloatupEnum::GainScience, centerTile(), "+" + to_string(sciReceived));
				AddProductionStat(sciReceived);
				_simulation->SetNeedDisplayUpdate(DisplayClusterEnum::BuildingAnimation, _centerTile.regionId());
				return;
			}
			if (IsBarrack(buildingEnum()))
			{
				_workDone100 = 0;
				_filledInputs = false;

				int32 influenceReceived = productPerBatch();
				resourceSystem().ChangeInfluence(influenceReceived);

				_simulation->uiInterface()->ShowFloatupInfo(FloatupEnum::GainInfluence, centerTile(), "+" + to_string(influenceReceived));
				AddProductionStat(influenceReceived);
				_simulation->SetNeedDisplayUpdate(DisplayClusterEnum::BuildingAnimation, _centerTile.regionId());
				return;
			}
			if (isEnum(CardEnum::CardMaker))
			{
				_workDone100 = 0;
				_filledInputs = false;

				auto& cardSys = _simulation->cardSystem(_playerId);

				if (workMode().name == "Productivity Book") {
					cardSys.AddCardToHand2(CardEnum::ProductivityBook);
				}
				else if (workMode().name == "Sustainability Book") {
					cardSys.AddCardToHand2(CardEnum::SustainabilityBook);
				}
				else if (workMode().name == "Frugality Book") {
					cardSys.AddCardToHand2(CardEnum::FrugalityBook);
				}
				else {
					PUN_NOENTRY();
				}

				_simulation->SetNeedDisplayUpdate(DisplayClusterEnum::BuildingAnimation, _centerTile.regionId());
				return;
			}
			if (isEnum(CardEnum::ImmigrationOffice))
			{
				_workDone100 = 0;
				_filledInputs = false;

				_simulation->AddImmigrants(_playerId, 1);
				_simulation->AddEventLog(_playerId, "The immigration office brought in an immigrant.", false);

				_simulation->SetNeedDisplayUpdate(DisplayClusterEnum::BuildingAnimation, _centerTile.regionId());
				return;
			}

			_workDone100 = 0;
			_filledInputs = false;
			
			ResourceHolderInfo info = holderInfo(product());
			PUN_CHECK2(info.isValid(), _simulation->unitdebugStr(unitId));
			int productionAmount = productPerBatch();

			AddResource(info.resourceEnum, productionAmount);
			AddProductionStat(productionAmount);

			if (product() == ResourceEnum::Beer) {
				_simulation->QuestUpdateStatus(_playerId, QuestEnum::BeerQuest, productionAmount);
			}
			else if (product() == ResourceEnum::Pottery)
			{
				_simulation->QuestUpdateStatus(_playerId, QuestEnum::PotteryQuest, productionAmount);
			}
			else if (product() == ResourceEnum::Fish)
			{
				_simulation->QuestUpdateStatus(_playerId, QuestEnum::CooperativeFishingQuest, productionAmount);
			}

			_simulation->uiInterface()->ShowFloatupInfo(FloatupEnum::GainResource, centerTile(), "+" + to_string(productionAmount), info.resourceEnum);

			OnProduce(productionAmount);

			_simulation->SetNeedDisplayUpdate(DisplayClusterEnum::BuildingAnimation, _centerTile.regionId());
		}

		if (isEnum(CardEnum::Farm)) {
			_simulation->SetNeedDisplayUpdate(DisplayClusterEnum::FarmPlant, centerTile().regionId());
		}

		return;
	}
	
	// if wasn't constructed
	_simulation->SetNeedDisplayUpdate(DisplayClusterEnum::Building, centerTile().regionId());

	PUN_DEBUG_EXPR(_simulation->unitAddDebugSpeech(unitId, " DoWork: Not constructed"));

	WorldAtom2 unitAtom = _simulation->unitAtom(unitId);

	if (!_isConstructed && _workDone100 >= buildTime_ManSec100())
	{
		PUN_DEBUG_EXPR(_simulation->unitAddDebugSpeech(unitId, " DoWork: FinishConstruction"));
		FinishConstruction();

		// Play sound
		if (IsRoad(buildingEnum())) {
			_simulation->soundInterface()->Spawn3DSound("CitizenAction", "ConstructionCompleteRoad", unitAtom);
		} else {
			_simulation->soundInterface()->Spawn3DSound("CitizenAction", "ConstructionComplete", unitAtom);
		}

		if (!IsRoad(_buildingEnum)) {
			_simulation->uiInterface()->ShowFloatupInfo(FloatupEnum::BuildingComplete, _centerTile, "");
		}
	}
	else 
	{
		// Play sound
		if (IsRoad(buildingEnum())) {
			_simulation->soundInterface()->Spawn3DSound("CitizenAction", "RoadConstruction", unitAtom);
		} else {
			if (Time::Ticks() > lastWorkSound + Time::TicksPerSecond) {
				lastWorkSound = Time::Ticks();

				_simulation->soundInterface()->Spawn3DSound("CitizenAction", "WoodConstruction", unitAtom);
			}
		}
	}
}


void Building::AddProductionStat(ResourcePair resource)
{
	AlgorithmUtils::AddResourcePairToPairs(_seasonalProductionPairs[0], resource);
	//_seasonalProduction[0] += resource.count;

	// Mint case, resource.count is money
	if (isEnum(CardEnum::Mint)) {
		_simulation->statSystem(_playerId).AddStat(SeasonStatEnum::Money, resource.count);
		return;
	}
	if (isEnum(CardEnum::InventorsWorkshop)) {
		_simulation->statSystem(_playerId).AddStat(SeasonStatEnum::Science, resource.count);
		return;
	}
	if (IsBarrack(buildingEnum())) 
	{
		_simulation->statSystem(_playerId).AddStat(SeasonStatEnum::Influence, resource.count);
		return;
	}

	if (resource.resourceEnum != ResourceEnum::None) {
		_simulation->statSystem(_playerId).AddResourceStat(ResourceSeasonStatEnum::Production, resource.resourceEnum, resource.count);
	}
}
void Building::AddConsumption1Stat(ResourcePair resource)
{
	_seasonalConsumption1[0] += resource.count;
	_simulation->statSystem(_playerId).AddResourceStat(ResourceSeasonStatEnum::Consumption, resource.resourceEnum, resource.count);
}
void Building::AddConsumption2Stat(ResourcePair resource)
{
	_seasonalConsumption2[0] += resource.count;
	_simulation->statSystem(_playerId).AddResourceStat(ResourceSeasonStatEnum::Consumption, resource.resourceEnum, resource.count);
}
void Building::AddDepletionStat(ResourcePair resource)
{
	_seasonalConsumption1[0] += resource.count;
}


void Building::CheckCombo()
{
	// Don't check permanent buildings
	switch(_buildingEnum)
	{
	case CardEnum::StorageYard:
	case CardEnum::House:
	case CardEnum::DirtRoad:
	case CardEnum::StoneRoad:
	case CardEnum::IntercityRoad:
	case CardEnum::Fence:
	case CardEnum::FenceGate:
	case CardEnum::Bridge:
	case CardEnum::TrapSpike:
	case CardEnum::None:
	case CardEnum::BoarBurrow:
		return;
	default:
		break;
	}

	// Don't check decorations
	if (IsDecorativeBuilding(_buildingEnum)) {
		return;
	}
	
	std::vector<int32> buildingIds = _simulation->buildingIds(_playerId, _buildingEnum);
	int32 numberOfSameType = buildingIds.size();

	for (int32 i = CardLvlCount; i-- > 1;) {
		if (numberOfSameType >= CardCountForLvl[i]) {
			// Set building 
			for (int32 buildingId : buildingIds) {
				Building& building = _simulation->building(buildingId);
				building._level = i;

				_simulation->uiInterface()->ShowFloatupInfo(FloatupEnum::ComboComplete, building.centerTile(), "Combo Lvl " + std::to_string(i));
			}
			_simulation->soundInterface()->Spawn2DSound("UI", "Combo", _playerId, centerTile());
			break;
		}
	}
}

void Building::ChangeWorkMode(const WorkMode& workMode)
{
	OnChangeWorkMode(workMode);

	// Don't resource change while under construction
	if (isConstructed()) 
	{
		auto setTarget = [&](ResourceEnum resourceEnumIn, int32 targetIn) {
			if (resourceEnumIn != ResourceEnum::None) {
				resourceSystem().SetResourceTarget(holderInfo(resourceEnumIn), targetIn);
				//resourceSystem().holder(holderInfo(resourceEnumIn)).SetTarget(targetIn);
			}
		};

		// Switch resource target
		setTarget(_workMode.input1, 0);
		setTarget(_workMode.input2, 0);
		setTarget(_workMode.product, 0);

		setTarget(workMode.input1, workMode.inputPerBatch * 2);
		setTarget(workMode.input2, workMode.inputPerBatch * 2);
		setTarget(workMode.product, workMode.inputPerBatch * 2);
	}

	_workMode = workMode;
}

std::vector<BonusPair> Building::GetBonuses()
{
	std::vector<BonusPair> bonuses;
	if (IsIndustrialBuilding(_buildingEnum))
	{
		if (_simulation->buildingFinishedCount(_playerId, CardEnum::EngineeringOffice)) {
			bonuses.push_back({ "Engineering office", 10 });
		}

		if (_simulation->IsResearched(_playerId, TechEnum::IndustryLastEra)) {
			bonuses.push_back({ "Last era technology", 10 });
		}

		// Industrialist
		{
			const int32 industrialistGuildRadius = 10;
			int32 radiusBonus = GetRadiusBonus(CardEnum::IndustrialistsGuild, industrialistGuildRadius, [&](int32_t bonus, Building& building) {
				return std::max(bonus, 20);
			});
			if (radiusBonus > 0) {
				bonuses.push_back({ "Industrialist", radiusBonus });
			}
		}
	}

	if (slotCardCount(CardEnum::ProductivityBook) > 0) {
		bonuses.push_back({ "Productivity Book", slotCardCount(CardEnum::ProductivityBook) * 20 });
	}

	//if (_simulation->playerOwned(_playerId).HasSpeedBoost(buildingId())) {
	//	bonuses.push_back({ "Leader Efficiency Boost", 50 });
	//}

	//{
	//	const int32_t consultingRadius = 10;
	//	int32_t radiusBonus = GetRadiusBonus(BuildingEnum::ConsultingFirm, consultingRadius, [&](int32_t bonus, Building& building) {
	//		return std::max(bonus, 30);
	//	});
	//	if (radiusBonus > 0) {
	//		bonuses.push_back({ "Consulting", radiusBonus });
	//	}
	//}

	return bonuses;
}