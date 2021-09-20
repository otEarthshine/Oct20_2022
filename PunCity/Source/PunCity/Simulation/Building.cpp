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
#include "UnitStateAI.h"

using namespace std;

#define LOCTEXT_NAMESPACE "Building"

Building::~Building() {}

void Building::Init(IGameSimulationCore& simulation, int32 objectId, int32 townId, uint8_t buildingEnum,
					TileArea area, WorldTile2 centerTile, Direction faceDirection)
{
	_simulation = &simulation;
	_objectId = objectId;

	_townId = townId;
	_playerId = _simulation->townPlayerId(_townId);
	
	_buildingEnum = static_cast<CardEnum>(buildingEnum);
	_centerTile = centerTile;
	_faceDirection = faceDirection;
	_area = area;

	_filledInputs = false;

	_buildingPlacedTick = Time::Ticks();
	_buildingLastWorkedTick = Time::Ticks();
	
	//workplaceNeedInput = false;
	workplaceInputNeeded = ResourceEnum::None;

	//useTools = false;
	lastWorkedOn = -10000;

	_upgrades.clear();

	_budgetLevel = 3;
	_workTimeLevel = 3;

	lastBudgetLevel = 3;
	lastWorkTimeLevel = 3;

	_deliveryTargetId = -1;

	_priority = PriorityEnum::NonPriority;
	_cardSlots.clear();

	_cachedWaterDestinationPortIds.clear();
	_cachedWaterRoutes.clear();

	BldInfo info = BuildingInfo[buildingEnumInt()];
	// Construction
	_maxOccupants = info.maxBuilderCount;
	_allowedOccupants = _maxOccupants;
	_isConstructed = false;
	_didSetWalkable = false;

	SetBuildingResourceUIDirty();
	_justFinishedConstructionJobUIDirty = false;

	OnPreInit_IncludeMinorTown();

	if (_playerId == -1 && 
		!IsBridgeOrTunnel(_buildingEnum))
	{
		return;  // Animal Controlled / Minor Town Control
	}

	//PUN_LOG("Building start: %s", *FString(info.name.c_str()));
		
	simulation.PlayerAddJobBuilding(_townId, *this, false);

	// Mark area for land clearing
	simulation.treeSystem().MarkArea(_townId, _area, false, ResourceEnum::None);

	OnPreInit();
	
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
		bool cheatBuild = SimSettings::IsOn("CheatFastBuild") && (!IsRoad(_buildingEnum) && _buildingEnum != CardEnum::Townhall);

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

	if (isEnum(CardEnum::Townhall)) {
		_simulation->AddFireOnceParticleInfo(ParticleEnum::OnTownhall, _area);
	} else {
		_simulation->AddFireOnceParticleInfo(ParticleEnum::OnPlacement, _area);
	}

	
	// AutoQuickBuild?
	if (IsAutoQuickBuild(_buildingEnum)) {
		InstantClearArea();
		_simulation->AddQuickBuild(_objectId);
	}
	

	// Special case Bridge,
	// Note: Done for bridge Walkable issue
	if (IsBridgeOrTunnel(_buildingEnum))
	{
		SetAreaWalkable();
	}
	else
	{
		// If land is cleared, just SetWalkable without needing ppl to do this.
		// This allows multiple roads to be queued without failing workplace.didSetWalkable()
		if (_simulation->IsLandCleared_SmallOnly(_townId, area) &&
			_simulation->IsLandCleared_SmallOnly(_townId, frontArea()))
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
	_simulation->SetRoadPathAI(gateTile(), true); // Set gate to road for fast pathfinding...

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

	auto& treeSys = _simulation->treeSystem();
	treeSys.ForceRemoveTileObjArea(_area);

	// Remove the tree at the gate tile to prevent inaccessibility
	frontArea().ExecuteOnArea_WorldTile2([&](WorldTile2 tile) {
		treeSys.ForceRemoveTileObj(tile.tileId(), false);
	});
}

void Building::FinishConstruction()
{
	//PUN_LOG("FinishConstruction %d constructed:%d", _objectId, _isConstructed);
	//PUN_CHECK(!_isConstructed);
	
	_maxOccupants = 0;
	_allowedOccupants = 0;
	_workDone100 = 0;
	_isConstructed = true;

	_buildingLastWorkedTick = Time::Ticks();

	_justFinishedConstructionJobUIDirty = true;

	// Do not reset roadMakers since they should construct multiple roads
	if (!IsRoad(_buildingEnum))
	{
		// In the case with other workReservers, reset them
		for (size_t i = 0; i < _workReservers.size(); i++) {
			_simulation->ResetUnitActions(_workReservers[i]);
		}
	}

	// Tribe
	//if (_buildingEnum == CardEnum::RegionTribalVillage)
	//{
	//	int32 provinceId = _simulation->GetProvinceIdClean(centerTile());
	//	if (_simulation->IsProvinceValid(provinceId)) 
	//	{
	//		for (int32 i = 0; i < 5; i++) {
	//			WorldTile2 spawnTile = _simulation->GetProvinceRandomTile(provinceId, gateTile(), 1, false, 10);
	//			if (spawnTile.isValid()) {
	//				int32 ageTicks = GameRand::Rand() % GetUnitInfo(UnitEnum::WildMan).maxAgeTicks;
	//				int32 newBornId = _simulation->AddUnit(UnitEnum::WildMan, GameInfo::PlayerIdNone, spawnTile.worldAtom2(), ageTicks);
	//				_simulation->unitAI(newBornId).SetHouseId(buildingId());
	//				
	//				PUN_LOG("Wildman Born Village id:%d bldId:%d bldTile:%s", newBornId, buildingId(), *centerTile().To_FString());
	//			}
	//		}
	//	}
	//}


	
	if (_townId == -1) {
		return; // Animal Controlled
	}
	/*
	 * City Beyond This Point
	 */

	FinishConstructionResourceAndWorkerReset();

	ResetDisplay();

	/*_simulation->buildingSystem().RefreshIsBuildingConnected()*/
	

	// Auto-add inputs/outputs accordingly
	if (hasInput1()) {
		AddResourceHolder(input1(), ResourceHolderType::Requester, baseInputPerBatch(input1()) * 2);
	}
	if (hasInput2()) {
		AddResourceHolder(input2(), ResourceHolderType::Requester, baseInputPerBatch(input2()) * 2);
	}
	if (product() != ResourceEnum::None) AddResourceHolder(product(), ResourceHolderType::Provider, 0);
	//if (IsProducer(buildingEnum())) AddResourceHolder(ResourceEnum::Tools, ResourceHolderType::Provider, 0);

	/*
	 * Add workers
	 */
	int32 workerCount = GetWorkerCount();
	if (workerCount > 0) {
		AddJobBuilding(workerCount);
	}

	// Quest
	if (_buildingEnum == CardEnum::StorageYard) {
		_simulation->QuestUpdateStatus(_playerId, QuestEnum::BuildStorageQuest);
	}


	// Foreign Builder
	if (foreignBuilder() != -1)
	{
		// Foreign Investment
		if (_simulation->HasForeignBuildingWithUpgrade(foreignBuilder(), _townId, CardEnum::Embassy, 0))
		{
			int32 influenceGained = GetQuickBuildBaseCost(buildingEnum(), GetConstructionResourceCost(), [&](ResourceEnum resourceEnum) { return 0; }) / 2;
			_simulation->ChangeInfluence(foreignBuilder(), influenceGained);
			_simulation->uiInterface()->ShowFloatupInfo(FloatupEnum::GainInfluence, centerTile(), TEXT_NUMSIGNED(influenceGained));
		}

		// Popup Once
		if (_simulation->TryDoCallOnceAction(foreignBuilder(), PlayerCallOnceActionEnum::ForeignBuiltSuccessfulPopup)) {
			_simulation->AddPopupToFront(foreignBuilder(),
				LOCTEXT("ForeignBuiltSuccessful_Popup", "Successfully built on foreign land.<space>Note: You retain the rights to demolish this building.")
			);
		}
	}
}

int32 Building::resourceCount(ResourceEnum resourceEnum) const
{
	ResourceHolderInfo info = holderInfo(resourceEnum);
	PUN_CHECK(info.isValid());
	return resourceSystem().resourceCount(info);
}
int32 Building::resourceCountSafe(ResourceEnum resourceEnum)
{
	ResourceHolderInfo info = holderInfo(resourceEnum);
	PUN_ENSURE(info.isValid(), return 0);
	return resourceSystem().resourceCountSafe(info);
}

int32 Building::resourceCountWithPop(ResourceEnum resourceEnum)
{
	ResourceHolderInfo info = holderInfo(resourceEnum);
	PUN_CHECK(info.isValid());
	return resourceSystem().resourceCountWithPop(info);
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
			if (input1Info.isValid()) SetResourceTarget(input1Info, baseInputPerBatch(input1()) * 2);
			if (input2Info.isValid()) SetResourceTarget(input2Info, baseInputPerBatch(input2()) * 2);
		}
		else {
			// TODO: make this 0??
			if (input1Info.isValid()) SetResourceTarget(input1Info, baseInputPerBatch(input1()) * 2);
			if (input2Info.isValid()) SetResourceTarget(input2Info, baseInputPerBatch(input2()) * 2);
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
	
	if (_townId == -1) {
		OnDeinit();

		_area.ExecuteOnArea_WorldTile2([&](WorldTile2 tile) {
			_simulation->SetWalkable(tile, true);
		});
		_simulation->SetRoadPathAI(gateTile(), false);
		return; // Animal Controlled
	}

	// Despawn all resource holders
	if (_holderInfos.size() > 0)
	{
		ResourceSystem& resourceSys = resourceSystem();
		for (auto& info : _holderInfos) {
			resourceSys.DespawnHolder(info);
		}
		_holderInfos.clear();
	}

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
		_simulation->PlayerRemoveJobBuilding(_townId, *this, _isConstructed);
	}

	// SetWalkable
	_area.ExecuteOnArea_WorldTile2([&](WorldTile2 tile) {
		_simulation->SetWalkable(tile, true);
	});
	_simulation->SetRoadPathAI(gateTile(), false);

	// Smoke
	_simulation->AddFireOnceParticleInfo(IsRoad(buildingEnum()) ? ParticleEnum::OnPlacement : ParticleEnum::OnDemolish, _area);
	

	TryRemoveDeliveryTarget();
	ClearDeliverySources();

	OnDeinit();

	//! Clear Variables so they are not misused ???
	//  Not supposed to be needed since ResetActions should trigger when workplace is Deinit
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

	SetBuildingResourceUIDirty();
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

	SetBuildingResourceUIDirty();
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
	PUN_CHECK(!isConstructed());

	// Go build anyway
	//if (shouldPrioritizeConstruction()) {
	//	return true;
	//}

	// TODO: Possible fix for construction site not finishing
	int32 extraBuildTimeFactor = (buildingAge() > Time::TicksPerYear) ? 2 : 1;
	
	return MathUtils::SumVector(_workReserved) + _workDone100 < buildTime_ManSec100() * extraBuildTimeFactor;
}


bool Building::UpgradeBuilding(int upgradeIndex, bool showPopups, ResourceEnum& needResourceEnumOut)
{
	PUN_ENSURE(upgradeIndex < _upgrades.size(), return false; );

	needResourceEnumOut = ResourceEnum::None;
	
	if (IsUpgraded(upgradeIndex)) {
		if (showPopups) {
			_simulation->AddPopup(_playerId, LOCTEXT("Already upgraded", "Already upgraded"));
		}
		return false;
	}

	// Is Era upgrade valid?
	BuildingUpgrade& upgrade = _upgrades[upgradeIndex];
	auto unlockSys = _simulation->unlockSystem(_playerId);
	if (upgrade.isEraUpgrade() && !IsEraUpgradable()) {
		if (showPopups) {
			if (GetUpgradeEraLevel() <= 4) {
				_simulation->AddPopupToFront(_playerId, FText::Format(LOCTEXT("UpgradeRequiresEra", "Upgrade Failed.<space>Advance to the {0} to unlock this Upgrade."), 
					unlockSys->GetEraText(unlockSys->GetEra() + 1)),
					ExclusiveUIEnum::None, "PopupCannot"
				);
			} else {
				_simulation->AddPopupToFront(_playerId, LOCTEXT("UpgradeRequiresElectricity", "Upgrade Failed.<space>Unlock Electricity Technology to unlock this Upgrade."));
			}
		}
		return false;
	}
	

	ResourcePair resourceNeeded = upgrade.currentUpgradeResourceNeeded();
	
	// Resource Upgrade
	check(resourceNeeded.isValid());

	PUN_CHECK(resourceNeeded.count > 0);

	int32 resourceCount = (resourceNeeded.resourceEnum == ResourceEnum::Money) ? _simulation->moneyCap32(_playerId) : resourceSystem().resourceCount(resourceNeeded.resourceEnum);
	
	if (resourceCount >= resourceNeeded.count)
	{
		// Remove Resource or Money
		if (resourceNeeded.resourceEnum == ResourceEnum::Money) {
			globalResourceSystem().ChangeMoney(-resourceNeeded.count);
		}
		else {
			resourceSystem().RemoveResourceGlobal(resourceNeeded.resourceEnum, resourceNeeded.count);
		}

		if (upgrade.isEraUpgrade()) 
		{
			check(upgrade.upgradeLevel != -1);
			if (upgrade.upgradeLevel < upgrade.maxUpgradeLevel(buildingEnum())) {
				upgrade.upgradeLevel++;
				ResetDisplay();
				
				if (upgrade.upgradeLevel >= upgrade.maxUpgradeLevel(buildingEnum())) {
					upgrade.isUpgraded = true;
				}
			}
		}
		else if (upgrade.isLevelUpgrade())
		{
			check(upgrade.upgradeLevel != -1);

			upgrade.upgradeLevel++;
			ResetDisplay();
		}
		else {
			upgrade.isUpgraded = true;
		}

		ResetDisplay();

		if (showPopups) {
			_simulation->soundInterface()->Spawn2DSound("UI", "UpgradeBuilding", _playerId, _centerTile);
		}

		OnUpgradeBuildingBase(upgradeIndex);
		return true;
	}

	if (showPopups) {
		_simulation->AddPopupToFront(_playerId, FText::Format(
			LOCTEXT("NotEnoughResourceForUpgrade", "Not enough {0} for upgrade."), 
			ResourceNameT(resourceNeeded.resourceEnum)
		), ExclusiveUIEnum::None, "PopupCannot");
	}
	
	needResourceEnumOut = resourceNeeded.resourceEnum;
	return false;
	
	
	//// Money Upgrade
	//PUN_CHECK(moneyNeeded > 0);
	//if (globalResourceSystem().moneyCap32() >= moneyNeeded)
	//{
	//	globalResourceSystem().ChangeMoney(-moneyNeeded);
	//	_upgrades[upgradeIndex].isUpgraded = true;

	//	ResetDisplay();

	//	if (showPopups) {
	//		_simulation->soundInterface()->Spawn2DSound("UI", "UpgradeBuilding", _playerId, _centerTile);
	//	}

	//	OnUpgradeBuildingBase(upgradeIndex);
	//	return true;
	//}

	//if (showPopups) {
	//	_simulation->AddPopupToFront(_playerId, LOCTEXT("NoUpgradeMoney", "Not enough money for upgrade."), ExclusiveUIEnum::None, "PopupCannot");
	//}
	//needResourceEnumOut = ResourceEnum::Money;
	//return false;
}

FText Building::GetUpgradeDisplayDescription(int32 index)
{
	BuildingUpgrade& upgrade = _upgrades[index];
	if (upgrade.isEraUpgrade()) 
	{
		check(upgrade.upgradeLevel != -1);
		int32 nextEra = upgrade.upgradeLevel + buildingInfo().minEra() + 1;
		
		// Special case: Trading Post/Port
		if (IsTradingPostLike(buildingEnum())) {
			return NSLOCTEXT("BuildingUpgrade", "Trader Level Desc", "Decrease Trading Fee by 5% per Upgrade Level");
		}

		if (nextEra > 5)
		{
			return FText::Format(
				NSLOCTEXT("BuildingUpgrade", "Advanced Machinery Level Desc", "Increases Efficiency by {0}% per level"),
				TEXT_NUM(10)
			);
		}
		if (nextEra == 5) 
		{
			int32 electricityPerBatch = ElectricityAmountNeeded();
			return FText::Format(
				NSLOCTEXT("BuildingUpgrade", "Electric Machinery Desc", "Once upgraded, this Building will need Electricity.<space>Workers work 50% faster with Electricity.<space>Consumes {0} kW Electricity."),
				TEXT_NUM(electricityPerBatch)
			);
		}

		return FText::Format(
			NSLOCTEXT("BuildingUpgrade", "Level Desc", "Increases Work Speed by {0}%"),
			TEXT_NUM(BldResourceInfo::UpgradeProfitPercentEraMultiplier - 100)
		);
	}
	return upgrade.description;
}
FText Building::GetUpgradeDisplayName(int32 index)
{
	BuildingUpgrade& upgrade = _upgrades[index];
	if (upgrade.isEraUpgrade()) 
	{
		check(upgrade.upgradeLevel != -1);
		
		int32 currentEraLevel = upgrade.upgradeLevel + buildingInfo().minEra();
		if (upgrade.upgradeLevel == upgrade.maxUpgradeLevel(buildingEnum())) {
			return NSLOCTEXT("BuildingUpgrade", "Level Maxed", " Level Maxed");
		}
		if (currentEraLevel == 4) {
			if (IsTradingPostLike(buildingEnum())) {
				return NSLOCTEXT("BuildingUpgrade", "Level Maxed", " Level Maxed");
			}
			return NSLOCTEXT("BuildingUpgrade", "Electric Machinery", "Electric Machinery");
		}

		// Advanced machinery
		if (currentEraLevel > 4) {
			return FText::Format(NSLOCTEXT("BuildingUpgrade", "Advanced Machinery Lv", "Advanced Machinery Lv {0}"), TEXT_NUM(currentEraLevel - 4));
		}
		return FText::Format(NSLOCTEXT("BuildingUpgrade", "Level", "Level ({0})"), _simulation->unlockSystem(_playerId)->GetEraText(currentEraLevel + 1));
	}
	
	if (upgrade.isLevelUpgrade())
	{
		return FText::Format(NSLOCTEXT("BuildingUpgrade", "LevelUpgradeDisplay", "{0} Lv {1}"), upgrade.name, TEXT_NUM(upgrade.upgradeLevel + 1));
	}
	return upgrade.name;
}

void Building::TestWorkDone()
{
	if (!isConstructed() ||
		!isJobBuilding()) {
		return;
	}
	if (!_filledInputs) {
		return;
	}
	
	if (_workDone100 >= workManSecPerBatch100())
	{
		// Special case: ConsumerWorkplace
		if (isEnum(CardEnum::Mint))
		{
			_workDone100 = 0;
			_filledInputs = false;

			int32 moneyReceived = outputPerBatch();
			globalResourceSystem().ChangeMoney(moneyReceived);
			_simulation->worldTradeSystem().ChangeSupply(_playerId, ResourceEnum::GoldBar, inputPerBatch(input1()));

			_simulation->uiInterface()->ShowFloatupInfo(FloatupEnum::GainMoney, centerTile(), TEXT_NUMSIGNED(moneyReceived));
			AddProductionStat(moneyReceived);
			AddConsumptionStats();

			_simulation->SetNeedDisplayUpdate(DisplayClusterEnum::BuildingAnimation, _centerTile.regionId());
			return;
		}
		if (isEnum(CardEnum::ResearchLab) ||
			isEnum(CardEnum::RegionShrine))
		{
			_workDone100 = 0;
			_filledInputs = false;

			int32 sciReceived = outputPerBatch();
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

			int32 influenceReceived = outputPerBatch();
			globalResourceSystem().ChangeInfluence(influenceReceived);

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

			_simulation->AddImmigrants(_townId, 1);
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
		PUN_CHECK(info.isValid());
		int productionAmount = outputPerBatch();

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
}



void Building::DoWork(int unitId, int workAmount100)
{
	//ACTION_LOG(_objectId, TEXT("DoWork: unitId:%d"), objectId)

	_workDone100 += workAmount100;
	lastWorkedOn = Time::Ticks();

	SetBuildingResourceUIDirty();

	// TODO: On Finish Work
	if (_isConstructed) 
	{
		TestWorkDone();
		
		//if (_workDone100 >= workManSecPerBatch100()) 
		//{
		//	// Special case: ConsumerWorkplace
		//	if (isEnum(CardEnum::Mint)) 
		//	{
		//		_workDone100 = 0;
		//		_filledInputs = false;
		//		
		//		int32 moneyReceived = outputPerBatch();
		//		globalResourceSystem().ChangeMoney(moneyReceived);
		//		_simulation->worldTradeSystem().ChangeSupply(_playerId, ResourceEnum::GoldBar, inputPerBatch(input1()));

		//		_simulation->uiInterface()->ShowFloatupInfo(FloatupEnum::GainMoney, centerTile(), TEXT_NUMSIGNED(moneyReceived));
		//		AddProductionStat(moneyReceived);
		//		AddConsumptionStats();
		//		
		//		_simulation->SetNeedDisplayUpdate(DisplayClusterEnum::BuildingAnimation, _centerTile.regionId());
		//		return;
		//	}
		//	if (isEnum(CardEnum::ResearchLab) ||
		//		isEnum(CardEnum::RegionShrine))
		//	{
		//		_workDone100 = 0;
		//		_filledInputs = false;

		//		int32 sciReceived = outputPerBatch();
		//		_simulation->unlockSystem(_playerId)->Research(sciReceived * 100 * Time::SecondsPerRound, 1);

		//		_simulation->uiInterface()->ShowFloatupInfo(FloatupEnum::GainScience, centerTile(), TEXT_NUMSIGNED(sciReceived));
		//		AddProductionStat(sciReceived);
		//		AddConsumptionStats();
		//		
		//		_simulation->SetNeedDisplayUpdate(DisplayClusterEnum::BuildingAnimation, _centerTile.regionId());
		//		return;
		//	}
		//	if (IsBarrack(buildingEnum()))
		//	{
		//		_workDone100 = 0;
		//		_filledInputs = false;

		//		int32 influenceReceived = outputPerBatch();
		//		globalResourceSystem().ChangeInfluence(influenceReceived);

		//		_simulation->uiInterface()->ShowFloatupInfo(FloatupEnum::GainInfluence, centerTile(), TEXT_NUMSIGNED(influenceReceived));
		//		AddProductionStat(influenceReceived);
		//		AddConsumptionStats();
		//		
		//		_simulation->SetNeedDisplayUpdate(DisplayClusterEnum::BuildingAnimation, _centerTile.regionId());
		//		return;
		//	}
		//	if (isEnum(CardEnum::CardMaker))
		//	{
		//		_workDone100 = 0;
		//		_filledInputs = false;

		//		auto& cardSys = _simulation->cardSystem(_playerId);
		//		cardSys.AddCardToHand2(static_cast<CardMaker*>(this)->GetCardProduced());

		//		AddConsumptionStats();

		//		_simulation->SetNeedDisplayUpdate(DisplayClusterEnum::BuildingAnimation, _centerTile.regionId());
		//		return;
		//	}
		//	if (isEnum(CardEnum::ImmigrationOffice))
		//	{
		//		_workDone100 = 0;
		//		_filledInputs = false;

		//		_simulation->AddImmigrants(_townId, 1);
		//		_simulation->AddEventLog(_playerId, 
		//			LOCTEXT("ImmigrationOfficeBroughtIn_Event", "The immigration office brought in an immigrant."), 
		//			false
		//		);

		//		_simulation->SetNeedDisplayUpdate(DisplayClusterEnum::BuildingAnimation, _centerTile.regionId());
		//		return;
		//	}

		//	_workDone100 = 0;
		//	_filledInputs = false;

		//	ResourceEnum productEnum = product();
		//	ResourceHolderInfo info = holderInfo(productEnum);
		//	PUN_CHECK2(info.isValid(), _simulation->unitdebugStr(unitId));
		//	int productionAmount = outputPerBatch();

		//	AddResource(info.resourceEnum, productionAmount);
		//	AddProductionStat(productionAmount);
		//	AddConsumptionStats();

		//	// Special case: Beeswax + Honey
		//	if (info.resourceEnum == ResourceEnum::Beeswax) 
		//	{
		//		AddResource(ResourceEnum::Honey, productionAmount);
		//		AddProductionStat(productionAmount);

		//		FloatupInfo floatupInfo(FloatupEnum::GainResource, Time::Ticks(), centerTile(), TEXT_NUMSIGNED(productionAmount), ResourceEnum::Beeswax);
		//		floatupInfo.resourceEnum2 = ResourceEnum::Honey;
		//		floatupInfo.text2 = TEXT_NUMSIGNED(productionAmount);

		//		_simulation->uiInterface()->ShowFloatupInfo(floatupInfo);
		//	}
		//	else
		//	{
		//		// All other floatups
		//		
		//		_simulation->uiInterface()->ShowFloatupInfo(FloatupEnum::GainResource, centerTile(), TEXT_NUMSIGNED(productionAmount), info.resourceEnum);
		//	}
		//	

		//	// Quest and Update Requirements
		//	if (productEnum == ResourceEnum::Beer) {
		//		_simulation->QuestUpdateStatus(_playerId, QuestEnum::BeerQuest, productionAmount);
		//	}
		//	else if (productEnum == ResourceEnum::Pottery) {
		//		_simulation->QuestUpdateStatus(_playerId, QuestEnum::PotteryQuest, productionAmount);
		//	}
		//	else if (productEnum == ResourceEnum::Fish) {
		//		_simulation->QuestUpdateStatus(_playerId, QuestEnum::CooperativeFishingQuest, productionAmount);
		//	}

		//	_simulation->unlockSystem(_playerId)->UpdateResourceProductionCount(productEnum, productionAmount);


		//	OnProduce(productionAmount);

		//	_simulation->SetNeedDisplayUpdate(DisplayClusterEnum::BuildingAnimation, _centerTile.regionId());
		//}

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

		_buildingLastWorkedTick = Time::Ticks();
	}
}


void Building::AddProductionStat(ResourcePair resource)
{
	AlgorithmUtils::AddResourcePairToPairs(_seasonalProductionPairs[0], resource);
	//_seasonalProduction[0] += resource.count;

	// Mint case, resource.count is money
	if (isEnum(CardEnum::Mint)) {
		statSystem().AddStat(SeasonStatEnum::Money, resource.count);
		return;
	}
	if (isEnum(CardEnum::ResearchLab)) {
		statSystem().AddStat(SeasonStatEnum::Science, resource.count);
		return;
	}
	if (isEnum(CardEnum::RegionShrine)) {
		statSystem().AddStat(SeasonStatEnum::Science, resource.count);
		return;
	}
	if (IsBarrack(buildingEnum())) 
	{
		statSystem().AddStat(SeasonStatEnum::Influence, resource.count);
		return;
	}

	if (resource.resourceEnum != ResourceEnum::None) {
		statSystem().AddResourceStat(ResourceSeasonStatEnum::Production, resource.resourceEnum, resource.count);
	}
}
void Building::AddConsumption1Stat(ResourcePair resource)
{
	_seasonalConsumption1[0] += resource.count;
	statSystem().AddResourceStat(ResourceSeasonStatEnum::Consumption, resource.resourceEnum, resource.count);
}
void Building::AddConsumption2Stat(ResourcePair resource)
{
	_seasonalConsumption2[0] += resource.count;
	statSystem().AddResourceStat(ResourceSeasonStatEnum::Consumption, resource.resourceEnum, resource.count);
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
	if (_townId == -1) {
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
	case CardEnum::IntercityBridge:
	//case CardEnum::TrapSpike:
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
	
	std::vector<int32> buildingIds = _simulation->buildingIds(_townId, _buildingEnum);
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

		_workMode = workMode;
		setTarget(workMode.input1, baseInputPerBatch(input1()) * 2);
		setTarget(workMode.input2, baseInputPerBatch(input2()) * 2);
		setTarget(workMode.product, 0);
	}
	else {
		_workMode = workMode;
	}
}

std::vector<BonusPair> Building::GetBonuses()
{
	if (_playerId == -1) {
		return {};
	}
	
	std::vector<BonusPair> bonuses;

	// Budget
	int32 budgetBonus = (_budgetLevel - 3) * 20;
	if (budgetBonus != 0) {
		bonuses.push_back({ FText::Format(LOCTEXT("BudgetLvlX", "Budget Lvl {0}"), TEXT_NUM(_budgetLevel)), budgetBonus });
	}

	// WorkTime
	int32 workTimeBonus = (_workTimeLevel - 3) * 20;
	if (workTimeBonus != 0) {
		bonuses.push_back({ FText::Format(LOCTEXT("WorkHourLvlX", "Work Hours Lvl {0}"), TEXT_NUM(_workTimeLevel)), workTimeBonus });
	}

	
	// Industrial
	if (IsIndustrialBuilding(_buildingEnum))
	{
		if (_simulation->townBuildingFinishedCount(_townId, CardEnum::EngineeringOffice)) {
			bonuses.push_back({ LOCTEXT("Engineering office", "Engineering office"), 10 });
		}

		if (_simulation->IsResearched(_playerId, TechEnum::Industrialization)) {
			bonuses.push_back({ LOCTEXT("Industrialization", "Industrialization"), 20 });
		}

		if (_simulation->HasTownBonus(_townId, CardEnum::DesertIndustry)) {
			bonuses.push_back({ LOCTEXT("Desert Industry", "Desert Industry"), 20 });
		}

		if (int32 industrialTechUpgradeCount = _simulation->GetTechnologyUpgradeCount(_playerId, TechEnum::IndustrialTechnologies)) {
			bonuses.push_back({ LOCTEXT("Industrial Technologies", "Industrial Technologies"), 3 * industrialTechUpgradeCount });
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

	// Agriculture
	if (IsAgricultureBuilding(_buildingEnum))
	{
		int32 radiusBonus = GetRadiusBonus(CardEnum::Granary, Windmill::Radius, [&](int32 bonus, Building& building) {
			return max(bonus, 25);
		});
		if (radiusBonus > 0) {
			bonuses.push_back({ LOCTEXT("Near Granary", "Near Granary"), radiusBonus });
		}
	}

	if (slotCardCount(CardEnum::ProductivityBook) > 0) {
		bonuses.push_back({ LOCTEXT("Productivity Book", "Productivity Book"), slotCardCount(CardEnum::ProductivityBook) * 20 });
	}
	if (slotCardCount(CardEnum::Passion) > 0) {
		int32 passionBonus = slotCardCount(CardEnum::Passion) * 15;
		if (_simulation->HasGlobalBonus(_playerId, CardEnum::Communism)) {
			passionBonus /= 2;
		}
		bonuses.push_back({ LOCTEXT("Passion", "Passion"), passionBonus });
	}
	if (slotCardCount(CardEnum::Motivation) > 0) {
		int32 motivationBonus = max(0, (_simulation->GetAverageHappiness(_townId) - 60) * slotCardCount(CardEnum::Motivation));
		if (_simulation->HasGlobalBonus(_playerId, CardEnum::Communism)) {
			motivationBonus /= 2;
		}
		bonuses.push_back({ LOCTEXT("Motivation", "Motivation"), motivationBonus });
	}


	if (_simulation->HasGlobalBonus(_playerId, CardEnum::Protectionism) &&
		IsLuxuryEnum(product())) 
	{
		bonuses.push_back({ LOCTEXT("Protectionism", "Protectionism"), 20 });
	}

	if (_simulation->HasGlobalBonus(_playerId, CardEnum::Communism) && product() != ResourceEnum::None) {
		bonuses.push_back({ LOCTEXT("Communism", "Communism"), 25 });
	}
	

	// Upgrade bonuses
	for (const BuildingUpgrade& upgrade : _upgrades) 
	{
		if (upgrade.isUpgraded && upgrade.efficiencyBonus > 0) {
			bonuses.push_back({ upgrade.name, upgrade.efficiencyBonus });
		}

		// House Lvl Upgrade Bonuses
		if (upgrade.isUpgraded && upgrade.houseLvlForBonus > 0) {
			int32 bonusAmount = _simulation->GetHouseLvlCount(_townId, upgrade.houseLvlForBonus, true);
			bonusAmount = std::min(bonusAmount, BuildingUpgrade::CalculateMaxHouseLvlBonus(upgrade.houseLvlForBonus));
			bonuses.push_back({ upgrade.name, bonusAmount });
		}
		
		// Combo Upgrade bonuses
		if (upgrade.isUpgraded && upgrade.comboEfficiencyBonus > 0) 
		{
			int32 buildingCount = _simulation->buildingCount(_townId, _buildingEnum);
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

		// Advanced Machinery
		if (upgrade.isEraUpgrade() && upgrade.GetAdvancedMachineryLevel() > 0)
		{
			bonuses.push_back({ LOCTEXT("Advanced Machinery", "Advanced Machinery"), upgrade.GetAdvancedMachineryLevel() * 10 });
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


int32 Building::GetAppealPercent()
{
	int32 appeal = _simulation->overlaySystem().GetAppealPercent(_centerTile);
	if (_simulation->townBuildingFinishedCount(_townId, CardEnum::ArchitectStudio)) {
		appeal += 5;
	}
	if (_simulation->townBuildingFinishedCount(_townId, CardEnum::GrandPalace) > 0) {
		appeal += 20;
	}
	if (_simulation->townBuildingFinishedCount(_townId, CardEnum::EnvironmentalistGuild)) {
		appeal += 15;
	}
	return appeal;
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
		townManager().AddDeliverySource(_objectId);
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
			townManager().RemoveDeliverySource(_objectId);
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

	int32 eraLevel = GetUpgradeEraLevel();
	if (eraLevel > 1) {
		bonuses.push_back({ LOCTEXT("Trade Building Upgrade Level", "Upgrade Level"), -5 * (eraLevel - 1) });
	}

	if (isEnum(CardEnum::TradingCompany)) {
		bonuses.push_back({ LOCTEXT("Trading Company", "Trading Company"), -5 });

		if (_simulation->TownhallCardCountTown(_townId, CardEnum::CompaniesAct)) {
			bonuses.push_back({ LOCTEXT("Companies Act", "Companies Act"), -10 });
		}
	}

	if (_simulation->townBuildingFinishedCount(_townId, CardEnum::MerchantGuild)) {
		bonuses.push_back({ LOCTEXT("Merchant Guild", "Merchant Guild"), -5 });
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

	if (int32 tradeRelationsTechUpgradeCount = _simulation->GetTechnologyUpgradeCount(_playerId, TechEnum::TradeRelations)) {
		bonuses.push_back({ LOCTEXT("Trade Relations", "Trade Relations"), -2 * tradeRelationsTechUpgradeCount });
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
	return BuildingUpgrade(name, description, ResourceEnum::Money, upgradeCost);
}

BuildingUpgrade Building::MakeProductionUpgrade(FText name, ResourceEnum resourceEnum, int32 efficiencyBonus)
{
	int32 costToProfitRatio100 = hasInput1() ? 200 : 300;
	int32 percentOfTotalPrice = efficiencyBonus * costToProfitRatio100 / 100;
	
	const FText bonusText = FText::Format(LOCTEXT("+{0}% productivity", "+{0}% productivity"), TEXT_NUM(efficiencyBonus));
	BuildingUpgrade upgrade = MakeUpgrade(name, bonusText, resourceEnum, percentOfTotalPrice);
	upgrade.efficiencyBonus = efficiencyBonus;
	return upgrade;
}
BuildingUpgrade Building::MakeProductionUpgrade_Money(FText name, int32 efficiencyBonus)
{
	int32 percentOfTotalPrice = efficiencyBonus * 300 / 100;
	
	const FText bonusText = FText::Format(LOCTEXT("+{0}% productivity", "+{0}% productivity"), TEXT_NUM(efficiencyBonus));
	BuildingUpgrade upgrade = MakeUpgrade(name, bonusText, percentOfTotalPrice);
	upgrade.efficiencyBonus = efficiencyBonus;
	return upgrade;
}

BuildingUpgrade Building::MakeProductionUpgrade_WithHouseLvl(FText name, ResourceEnum resourceEnum, int32 houseLvl)
{
	check(houseLvl <= House::GetMaxHouseLvl())
	
	int32 maxHouseLvlBonus = BuildingUpgrade::CalculateMaxHouseLvlBonus(houseLvl);
	const FText bonusText = FText::Format(
		LOCTEXT("ProductionUpgradeWithHouseLvl_Description", "+1% Productivity for each\nHouse Lvl {0}+ in this City\n(max {1}%)"), 
		TEXT_NUM(houseLvl),
		TEXT_NUM(maxHouseLvlBonus)
	);
	int32 percentOfTotalPrice = maxHouseLvlBonus;
	
	BuildingUpgrade upgrade = MakeUpgrade(name, bonusText, resourceEnum, percentOfTotalPrice);
	upgrade.houseLvlForBonus = houseLvl;
	return upgrade;
}

BuildingUpgrade Building::MakeWorkerSlotUpgrade(int32 percentOfTotalPrice, int32 workerSlotBonus)
{
	const FText name = FText::Format(LOCTEXT("+{0} Worker Slots", "+{0} Worker {0}|plural(one=Slot,other=Slots)"), TEXT_NUM(workerSlotBonus));
	BuildingUpgrade upgrade = MakeUpgrade(name, FText(), percentOfTotalPrice);
	upgrade.workerSlotBonus = workerSlotBonus;
	return upgrade;
}

BuildingUpgrade Building::MakeEraUpgrade(int32 startEra)
{
	BuildingUpgrade upgrade = BuildingUpgrade(FText(), FText(), ResourceEnum::Money, 0);
	upgrade.startEra = startEra;
	upgrade.upgradeLevel = 0;

	// Fill in alll the Era upgrades including "Upgrade Electric Machinery"
	int32 baseResourceCostMoney = buildingInfo().resourceInfo.baseResourceCostMoney;
	int32 upgradeCount = 0;
	
	for (int32 i = startEra; i <= 4; i++)
	{
		auto getUpgradeResourceEnum = [&]() {
			switch (i) {
			case 1: return ResourceEnum::Brick;
			case 2: return ResourceEnum::Glass;
			case 3: return ResourceEnum::Concrete;
			case 4: default: return ResourceEnum::Steel;
			}
		};
		int32 scaledBaseResourceCostMoney = buildingInfo().resourceInfo.ApplyUpgradeCostMultipliers(baseResourceCostMoney, upgradeCount);
		upgradeCount++;
		
		int32 upgradeResourceCostMoney = scaledBaseResourceCostMoney * (BldResourceInfo::UpgradeCostPercentEraMultiplier - 100) / 100;
		
		upgrade.resourceNeededPerLevel.push_back(ResourcePair(getUpgradeResourceEnum(), upgradeResourceCostMoney / GetResourceInfo(getUpgradeResourceEnum()).basePrice));
	}
	
	return upgrade;
}

BuildingUpgrade Building::MakeLevelUpgrade(FText name, FText description, ResourceEnum resourceEnum, int32 percentOfTotalPrice)
{
	int32 totalCost = buildingInfo().constructionCostAsMoney();
	
	int32 price = (resourceEnum == ResourceEnum::Money) ? 1 : GetResourceInfo(resourceEnum).basePrice;
	int32 resourceCount = 1 + totalCost * percentOfTotalPrice / 100 / price;
	BuildingUpgrade upgrade(name, description, resourceEnum, resourceCount);

	upgrade.upgradeLevel = 0;
	
	return upgrade;
}

BuildingUpgrade Building::MakeComboUpgrade(FText name, ResourceEnum resourceEnum)
{
	int32 comboEfficiencyBonus = 10;
	for (int32 i = 1; i < buildingInfo().minEra(); i++) {
		comboEfficiencyBonus += 5;
	}

	int32 percentOfTotalPrice = comboEfficiencyBonus * 2 * 2 / 3; // May 30: -30% comboupgrade price
	
	FText description = FText::Format(
		LOCTEXT("Combo Upgrade Description", "Gain +{0}/{1}/{2}% productivity if this city has 2/4/8 {3}"),
		TEXT_NUM(comboEfficiencyBonus),
		TEXT_NUM(comboEfficiencyBonus * 2),
		TEXT_NUM(comboEfficiencyBonus * 3),
		buildingInfo().namePlural
	);

	BuildingUpgrade upgrade = MakeUpgrade(name, description, resourceEnum, percentOfTotalPrice);
	upgrade.comboEfficiencyBonus = comboEfficiencyBonus;
	return upgrade;
}

int32 Building::displayVariationIndex()
{
	if (_playerId != -1)
	{
		if (IsAutoEraUpgrade(buildingEnum())) 
		{
			// Auto Era Upgrade should use lowest era while under construction
			if (!isConstructed()) {
				return 0;
			}
			
			// Some building upgrade according to Town Lvl
			int32 variationIndex = (_simulation->GetTownLvl(_townId) - 1) - _simulation->GetMinEraDisplay(_buildingEnum);
			variationIndex = max(variationIndex, 0);
			return variationIndex;
		}
		return GetEraUpgradeCount();
	}
	return 0;
}

#undef LOCTEXT_NAMESPACE 