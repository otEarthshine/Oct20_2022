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
#include "PunCity/Simulation/Buildings/GathererHut.h"
#include "PunCity/Simulation/GeoresourceSystem.h"

using namespace std;

#define LOCTEXT_NAMESPACE "Building"

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

	_deliveryTargetId = -1;

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
	

	// Special case Bridge,
	// Note: Done for bridge Walkable issue
	if (_buildingEnum == CardEnum::Bridge ||
		_buildingEnum == CardEnum::Tunnel)
	{
		SetAreaWalkable();
	}
	else
	{
		// If land is cleared, just SetWalkable without needing ppl to do this.
		// This allows multiple roads to be queued without failing workplace.didSetWalkable()
		if (_simulation->IsLandCleared_SmallOnly(_playerId, area) &&
			_simulation->IsLandCleared_SmallOnly(_playerId, frontArea()))
		{
			SetAreaWalkable();
		}
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
		for (ResourceEnum resourceEnumLocal : StaticData::FoodEnums) {
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

	TryRemoveDeliveryTarget();
	ClearDeliverySources();

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
	std::vector<ResourceEnum> productEnums = products();

	for (ResourceEnum productEnum : productEnums)
	{
		ResourceHolderInfo info = holderInfo(productEnum);
		PUN_CHECK2(info.isValid(), string("Probably no holder for product?"));
		int resourceCount = resourceSystem().resourceCountWithPush(info); // TODO: shouldn't this be pop?

		if (resourceCount > GameConstants::WorkerEmptyBuildingInventoryAmount) {
			return false;
		}
	}

	// TODO: proper work limit
	return MathUtils::SumVector(_workReserved) + _workDone100 < workManSecPerBatch100();
}

bool Building::NeedConstruct()
{
	PUN_CHECK(!isConstructed())

	// TODO: Possible fix for construction site not finishing
	int32 extraBuildTimeFactor = (buildingAge() > Time::TicksPerYear) ? 2 : 1;
	
	return MathUtils::SumVector(_workReserved) + _workDone100 < buildTime_ManSec100() * extraBuildTimeFactor;
}


bool Building::UpgradeBuilding(int upgradeIndex, bool showDisplay)
{
	PUN_ENSURE(upgradeIndex < _upgrades.size(), return false; );
	
	if (IsUpgraded(upgradeIndex)) {
		if (showDisplay) {
			_simulation->AddPopup(_playerId, LOCTEXT("Already upgraded", "Already upgraded"));
		}
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

			if (showDisplay) {
				_simulation->soundInterface()->Spawn2DSound("UI", "UpgradeBuilding", _playerId, _centerTile);
			}

			OnUpgradeBuildingBase(upgradeIndex);
			return true;
		}

		if (showDisplay) {
			_simulation->AddPopupToFront(_playerId, FText::Format(LOCTEXT("NotEnoughResourceForUpgrade", "Not enough {0} for upgrade."), ResourceNameT(resourceNeeded.resourceEnum)),
											ExclusiveUIEnum::None, "PopupCannot");
		}
		
		return false;
	}
	
	
	// Money Upgrade
	PUN_CHECK(moneyNeeded > 0);
	if (resourceSystem().money() >= moneyNeeded)
	{
		resourceSystem().ChangeMoney(-moneyNeeded);
		_upgrades[upgradeIndex].isUpgraded = true;

		ResetDisplay();

		if (showDisplay) {
			_simulation->soundInterface()->Spawn2DSound("UI", "UpgradeBuilding", _playerId, _centerTile);
		}

		OnUpgradeBuildingBase(upgradeIndex);
		return true;
	}

	if (showDisplay) {
		_simulation->AddPopupToFront(_playerId, LOCTEXT("NoUpgradeMoney", "Not enough money for upgrade."), ExclusiveUIEnum::None, "PopupCannot");
	}
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

				_simulation->uiInterface()->ShowFloatupInfo(FloatupEnum::GainMoney, centerTile(), TEXT_NUMSIGNED(moneyReceived));
				AddProductionStat(moneyReceived);
				AddConsumptionStats();
				
				_simulation->SetNeedDisplayUpdate(DisplayClusterEnum::BuildingAnimation, _centerTile.regionId());
				return;
			}
			if (isEnum(CardEnum::InventorsWorkshop) ||
				isEnum(CardEnum::RegionShrine))
			{
				_workDone100 = 0;
				_filledInputs = false;

				int32 sciReceived = productPerBatch();
				_simulation->unlockSystem(_playerId)->Research(sciReceived * 100 * Time::SecondsPerRound, 1);

				_simulation->uiInterface()->ShowFloatupInfo(FloatupEnum::GainScience, centerTile(), TEXT_NUMSIGNED(sciReceived));
				AddProductionStat(sciReceived);
				AddConsumptionStats();
				
				_simulation->SetNeedDisplayUpdate(DisplayClusterEnum::BuildingAnimation, _centerTile.regionId());
				return;
			}
			if (IsBarrack(buildingEnum()))
			{
				_workDone100 = 0;
				_filledInputs = false;

				int32 influenceReceived = productPerBatch();
				resourceSystem().ChangeInfluence(influenceReceived);

				_simulation->uiInterface()->ShowFloatupInfo(FloatupEnum::GainInfluence, centerTile(), TEXT_NUMSIGNED(influenceReceived));
				AddProductionStat(influenceReceived);
				AddConsumptionStats();
				
				_simulation->SetNeedDisplayUpdate(DisplayClusterEnum::BuildingAnimation, _centerTile.regionId());
				return;
			}
			if (isEnum(CardEnum::CardMaker))
			{
				_workDone100 = 0;
				_filledInputs = false;

				auto& cardSys = _simulation->cardSystem(_playerId);
				cardSys.AddCardToHand2(static_cast<CardMaker*>(this)->GetCardProduced());

				AddConsumptionStats();

				_simulation->SetNeedDisplayUpdate(DisplayClusterEnum::BuildingAnimation, _centerTile.regionId());
				return;
			}
			if (isEnum(CardEnum::ImmigrationOffice))
			{
				_workDone100 = 0;
				_filledInputs = false;

				_simulation->AddImmigrants(_playerId, 1);
				_simulation->AddEventLog(_playerId, 
					LOCTEXT("ImmigrationOfficeBroughtIn_Event", "The immigration office brought in an immigrant."), 
					false
				);

				_simulation->SetNeedDisplayUpdate(DisplayClusterEnum::BuildingAnimation, _centerTile.regionId());
				return;
			}

			_workDone100 = 0;
			_filledInputs = false;

			ResourceEnum productEnum = product();
			ResourceHolderInfo info = holderInfo(productEnum);
			PUN_CHECK2(info.isValid(), _simulation->unitdebugStr(unitId));
			int productionAmount = productPerBatch();

			AddResource(info.resourceEnum, productionAmount);
			AddProductionStat(productionAmount);
			AddConsumptionStats();

			// Special case: Beeswax + Honey
			if (info.resourceEnum == ResourceEnum::Beeswax) 
			{
				AddResource(ResourceEnum::Honey, productionAmount);
				AddProductionStat(productionAmount);

				FloatupInfo floatupInfo(FloatupEnum::GainResource, Time::Ticks(), centerTile(), TEXT_NUMSIGNED(productionAmount), ResourceEnum::Beeswax);
				floatupInfo.resourceEnum2 = ResourceEnum::Honey;
				floatupInfo.text2 = TEXT_NUMSIGNED(productionAmount);

				_simulation->uiInterface()->ShowFloatupInfo(floatupInfo);
			}
			else
			{
				// All other floatups
				
				_simulation->uiInterface()->ShowFloatupInfo(FloatupEnum::GainResource, centerTile(), TEXT_NUMSIGNED(productionAmount), info.resourceEnum);
			}
			

			// Quest and Update Requirements
			if (productEnum == ResourceEnum::Beer) {
				_simulation->QuestUpdateStatus(_playerId, QuestEnum::BeerQuest, productionAmount);
			}
			else if (productEnum == ResourceEnum::Pottery) {
				_simulation->QuestUpdateStatus(_playerId, QuestEnum::PotteryQuest, productionAmount);
			}
			else if (productEnum == ResourceEnum::Fish) {
				_simulation->QuestUpdateStatus(_playerId, QuestEnum::CooperativeFishingQuest, productionAmount);
			}

			_simulation->unlockSystem(_playerId)->UpdateResourceProductionCount(productEnum, productionAmount);


			OnProduce(productionAmount);

			_simulation->SetNeedDisplayUpdate(DisplayClusterEnum::BuildingAnimation, _centerTile.regionId());
		}

		if (isEnum(CardEnum::Farm)) {
			_simulation->SetNeedDisplayUpdate(DisplayClusterEnum::FarmPlant, centerTile().regionId());
		}


		// TODO: check why you can have 168% no smelter... without finishing...
		PUN_CHECK(workPercent() < 101);
		PUN_CHECK(_workDone100 < workManSecPerBatch100());

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
			_simulation->uiInterface()->ShowFloatupInfo(FloatupEnum::BuildingComplete, _centerTile, FText());
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
	if (isEnum(CardEnum::RegionShrine)) {
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

int32 Building::oreLeft()
{
	auto node = _simulation->georesourceSystem().georesourceNode(_simulation->GetProvinceIdClean(centerTile()));
	if (isEnum(CardEnum::Quarry)) {
		return node.stoneAmount;
	}
	return node.depositAmount;
}


void Building::CheckCombo()
{
	if (_playerId == -1) {
		return;
	}
	
	// Don't check permanent buildings
	if (IsStorage(_buildingEnum)) {
		return;
	}
	switch(_buildingEnum)
	{
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
		
	case CardEnum::Market:
	case CardEnum::ShippingDepot:
		return;
	default:
		break;
	}

	// Don't check decorations
	if (IsDecorativeBuilding(_buildingEnum)) {
		return;
	}

	if (!_simulation->IsResearched(_playerId, TechEnum::Combo)) {
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

				_simulation->uiInterface()->ShowFloatupInfo(FloatupEnum::ComboComplete, building.centerTile(), 
					FText::Format(LOCTEXT("ComboLvl_Float", "Combo Lvl {0}"), TEXT_NUM(i))
				);
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
			bonuses.push_back({ LOCTEXT("Engineering office", "Engineering office"), 10 });
		}

		if (_simulation->IsResearched(_playerId, TechEnum::IndustryLastEra)) {
			bonuses.push_back({ LOCTEXT("Last era technology", "Last era technology"), 10 });
		}

		// Industrialist
		{
			const int32 industrialistGuildRadius = 10;
			int32 radiusBonus = GetRadiusBonus(CardEnum::IndustrialistsGuild, industrialistGuildRadius, [&](int32_t bonus, Building& building) {
				return std::max(bonus, 20);
			});
			if (radiusBonus > 0) {
				bonuses.push_back({ LOCTEXT("Industrialist", "Industrialist"), radiusBonus });
			}
		}
	}

	if (slotCardCount(CardEnum::ProductivityBook) > 0) {
		bonuses.push_back({ LOCTEXT("Productivity Book", "Productivity Book"), slotCardCount(CardEnum::ProductivityBook) * 20 });
	}

	// Upgrade bonuses
	for (const BuildingUpgrade& upgrade : _upgrades) 
	{
		if (upgrade.isUpgraded && upgrade.efficiencyBonus > 0) {
			bonuses.push_back({ upgrade.name, upgrade.efficiencyBonus });
		}
		
		// Combo Upgrade bonuses
		if (upgrade.isUpgraded && upgrade.comboEfficiencyBonus > 0) 
		{
			int32 buildingCount = _simulation->buildingCount(_playerId, _buildingEnum);
			int32 comboLevel = 0;
			if (buildingCount >= 8) {
				comboLevel = 3;
			} else if (buildingCount >= 4) {
				comboLevel = 2;
			} else if (buildingCount >= 2) {
				comboLevel = 1;
			}

			int32 finalComboEfficiencyBonus = upgrade.comboEfficiencyBonus * comboLevel;
			bonuses.push_back({ upgrade.name, finalComboEfficiencyBonus });
		}
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


void Building::SetDeliveryTarget(int32 deliveryTargetId)
{
	check(deliveryTargetId != -1);
	
	// Remove old target if needed
	TryRemoveDeliveryTarget();

	_deliveryTargetId = deliveryTargetId;

	auto& resourceSys = resourceSystem();

	Building& deliveryTarget = _simulation->buildingChecked(_deliveryTargetId);
	deliveryTarget._deliverySourceIds.push_back(_objectId);

	if (!isEnum(CardEnum::ShippingDepot)) {
		_simulation->playerOwned(_playerId).AddDeliverySource(_objectId);
	}

	// Ensure all Providers are set back to Manual
	for (const ResourceHolderInfo& info : _holderInfos) {
		if (resourceSys.holder(info).type == ResourceHolderType::Provider) {
			SetHolderTypeAndTarget(info.resourceEnum, ResourceHolderType::Manual, 0);
		}
	}
}
void Building::TryRemoveDeliveryTarget()
{
	if (_deliveryTargetId != -1) 
	{
		auto& resourceSys = resourceSystem();
		
		std::vector<int32>& sourceIds = _simulation->building(_deliveryTargetId)._deliverySourceIds;
		CppUtils::Remove(sourceIds, _objectId);
		_deliveryTargetId = -1;

		if (!isEnum(CardEnum::ShippingDepot)) {
			_simulation->playerOwned(_playerId).RemoveDeliverySource(_objectId);
		}

		// Ensure all Manual are set back to Providers
		for (const ResourceHolderInfo& info : _holderInfos) {
			if (resourceSys.holder(info).type == ResourceHolderType::Manual) {
				SetHolderTypeAndTarget(info.resourceEnum, ResourceHolderType::Provider, 0);
			}
		}
	}
}

/*
 * Bonus
 */
std::vector<BonusPair> Building::GetTradingFeeBonuses()
{
	std::vector<BonusPair> bonuses;

	if (isEnum(CardEnum::TradingCompany)) {
		bonuses.push_back({ LOCTEXT("Trading Company", "Trading Company"), -5 });

		if (_simulation->TownhallCardCount(playerId(), CardEnum::CompaniesAct)) {
			bonuses.push_back({ LOCTEXT("Companies Act", "Companies Act"), -10 });
		}
	}

	if (_simulation->buildingFinishedCount(playerId(), CardEnum::MerchantGuild)) {
		bonuses.push_back({ LOCTEXT("Merchant Guild", "Merchant Guild"), -5 });
	}
	if (IsUpgraded(0)) {
		bonuses.push_back({ LOCTEXT("Fee Discount", "Fee Discount"), -5 });
	}

	if (_simulation->IsResearched(playerId(), TechEnum::TraderDiscount))
	{
		if (isEnum(CardEnum::TradingCompany)) {
			if (adjacentCount(CardEnum::TradingPort) > 0) {
				bonuses.push_back({ LOCTEXT("Trader Discount", "Trader Discount"), -5 });
			}
		}
	}

	if (_simulation->IsResearched(playerId(), TechEnum::DesertTrade) &&
		_simulation->GetBiomeEnum(_centerTile) == BiomeEnum::Desert)
	{
		bonuses.push_back({ LOCTEXT("Silk Road", "Silk Road"), -10 });
	}

	return bonuses;
}

/*
 * Make Upgrades
 */

BuildingUpgrade Building::MakeUpgrade(FText name, FText description, ResourceEnum resourceEnum, int32 percentOfTotalPrice)
{
	int32 totalCost = buildingInfo().constructionCostAsMoney();
	int32 resourceCount = 1 + totalCost * percentOfTotalPrice / 100 / GetResourceInfo(resourceEnum).basePrice;
	return BuildingUpgrade(name, description, resourceEnum, resourceCount);
}
BuildingUpgrade Building::MakeUpgrade(FText name, FText description, int32 percentOfTotalPrice)
{
	int32 totalCost = buildingInfo().constructionCostAsMoney();
	int32 upgradeCost = totalCost * percentOfTotalPrice / 100;
	return BuildingUpgrade(name, description, upgradeCost);
}

BuildingUpgrade Building::MakeProductionUpgrade(FText name, ResourceEnum resourceEnum, int32 percentOfTotalPrice, int32 efficiencyBonus)
{
	const FText bonusText = FText::Format(LOCTEXT("+{0}% productivity", "+{0}% productivity"), TEXT_NUM(efficiencyBonus));
	BuildingUpgrade upgrade = MakeUpgrade(name, bonusText, resourceEnum, percentOfTotalPrice);
	upgrade.efficiencyBonus = efficiencyBonus;
	return upgrade;
}
BuildingUpgrade Building::MakeProductionUpgrade(FText name, int32 percentOfTotalPrice, int32 efficiencyBonus)
{
	const FText bonusText = FText::Format(LOCTEXT("+{0}% productivity", "+{0}% productivity"), TEXT_NUM(efficiencyBonus));
	BuildingUpgrade upgrade = MakeUpgrade(name, bonusText, percentOfTotalPrice);
	upgrade.efficiencyBonus = efficiencyBonus;
	return upgrade;
}

BuildingUpgrade Building::MakeWorkerSlotUpgrade(int32 percentOfTotalPrice, int32 workerSlotBonus)
{
	const FText name = FText::Format(LOCTEXT("+{0} Worker Slots", "+{0} Worker {0}|plural(one=Slot,other=Slots)"), TEXT_NUM(workerSlotBonus));
	BuildingUpgrade upgrade = MakeUpgrade(name, name, percentOfTotalPrice);
	upgrade.workerSlotBonus = workerSlotBonus;
	return upgrade;
}

BuildingUpgrade Building::MakeComboUpgrade(FText name, ResourceEnum resourceEnum, int32 percentOfTotalPrice, int32 comboEfficiencyBonus)
{
	FText buildingNamePluralText;
	
	switch (buildingInfo().cardEnum)
	{
#define CASE(cardEnum, pluralName) case cardEnum: buildingNamePluralText = pluralName; break;
		CASE(CardEnum::Bakery,			LOCTEXT("Bakeries", "Bakeries"));
		CASE(CardEnum::BeerBrewery,		LOCTEXT("Breweries", "Breweries"));
		CASE(CardEnum::Winery,			LOCTEXT("Wineries", "Wineries"));
		CASE(CardEnum::Brickworks,		LOCTEXT("Brickworks", "Brickworks"));
		CASE(CardEnum::PrintingPress,	LOCTEXT("Printing Presses", "Printing Presses"));
		CASE(CardEnum::VodkaDistillery, LOCTEXT("Distilleries", "Distilleries"));
#undef CASE
	default:
		buildingNamePluralText =  FText::Format(INVTEXT("{0}s"), buildingInfo().GetName());
		break;
	}

	FText description = FText::Format(
		LOCTEXT("Combo Upgrade Description",
			"Gain +{0}/{1}/{2}% productivity if this city has 2/4/8 {3}"
		),
		TEXT_NUM(comboEfficiencyBonus),
		TEXT_NUM(comboEfficiencyBonus * 2),
		TEXT_NUM(comboEfficiencyBonus * 3),
		buildingNamePluralText
	);

	BuildingUpgrade upgrade = MakeUpgrade(name, description, resourceEnum, percentOfTotalPrice);
	upgrade.comboEfficiencyBonus = comboEfficiencyBonus;
	return upgrade;
}


#undef LOCTEXT_NAMESPACE 