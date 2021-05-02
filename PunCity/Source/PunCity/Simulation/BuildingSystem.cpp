#include "BuildingSystem.h"
#include "Buildings/House.h"
#include "Buildings/GathererHut.h"
#include "Buildings/TownHall.h"
#include "Buildings/StorageYard.h"

#include "Buildings/Statues.h"
#include "Buildings/Temples.h"
#include "PunCity/CppUtils.h"

#include "OverlaySystem.h"

#include "UnrealEngine.h"

using namespace std;

DECLARE_CYCLE_STAT(TEXT("PUN: Bld"), STAT_PunBld, STATGROUP_Game);

void BuildingSystem::Tick()
{
	SCOPE_CYCLE_COUNTER(STAT_PunBld);

	if (Time::Ticks() % Time::TicksPerMinute == 0) {
		for (size_t i = 0; i < _buildings.size(); i++) {
			if (_alive[i]) {
				_buildings[i]->MinuteStatisticsUpdate();
			}
		}
	}

	if (Time::Ticks() % Time::TicksPerSecond == 0) {
		//PUN_LOG("BuildingSystem Tick1Sec %d", _buildings.size());
		for (size_t i = 0; i < _buildings.size(); i++) {
			if (_alive[i]) {
				_buildings[i]->Tick1Sec();
			}
		}
	}

	if (Time::Ticks() % Time::TicksPerRound == 0) {
		for (size_t i = 0; i < _buildings.size(); i++) {
			if (_alive[i]) {
				_buildings[i]->TickRound();
			}
		}
	}

	// TODO: this is not really used??
	for (int i = 0; i < _buildingsToTick.size(); i++) {
		_buildings[_buildingsToTick[i]]->Tick();
	}

	// Schedule
	for (size_t i = _scheduleTicks.size(); i-- > 0;) {
		if (Time::Ticks() >= _scheduleTicks[i]) {
			_buildings[_scheduleBuildingIds[i]]->ScheduleTick();
			
			_scheduleTicks.erase(_scheduleTicks.begin() + i);
			_scheduleBuildingIds.erase(_scheduleBuildingIds.begin() + i);
		}
	}

	for (int i = _buildingsOnFire.size(); i-- > 0;) {
		Building& buildingOnFire = building(_buildingsOnFire[i]);

		// After 2 minute of fire, becomes ruin
		int32_t fireMinutes = (Time::Ticks() - buildingOnFire.fireStartTick()) / Time::TicksPerMinute;
		if (fireMinutes >= 2) {
			buildingOnFire.BecomeBurnedRuin();
			_buildingsOnFire.erase(_buildingsOnFire.begin() + i);
		}
	}

	// Quick build construct
	for (int32 i = _quickBuildList.size(); i-- > 0;)
	{
		Building& bld = building(_quickBuildList[i]);;

		if (bld.isConstructed()) {
			_quickBuildList.erase(_quickBuildList.begin() + i);
		}
		else {
			if (bld.isEnum(CardEnum::StorageYard)) {
				bld.ChangeConstructionPercent(100);
			} else {
				bld.ChangeConstructionPercent(1);
			}

			int32 newPercent = bld.constructionPercent();
			if (newPercent % 3 == 0) {
				_simulation->SetNeedDisplayUpdate(DisplayClusterEnum::Building, bld.centerTile().regionId());
			}
			
			if (newPercent >= 100)
			{
				bld.FinishConstruction();

				// Play sound
				_simulation->soundInterface()->Spawn3DSound("CitizenAction", "ConstructionComplete", bld.centerTile().worldAtom2());
				_simulation->uiInterface()->ShowFloatupInfo(FloatupEnum::BuildingComplete, bld.centerTile(), FText());
			}
		}
	}
}

int BuildingSystem::AddTileBuilding(WorldTile2 tile, CardEnum buildingEnum, int32 playerId)
{
	FPlaceBuilding placeBuildingParams;
	placeBuildingParams.townId = _simulation->tileOwnerTown(tile);
	if (placeBuildingParams.townId == -1) {
		return -1;
	}

	
	placeBuildingParams.buildingEnum = static_cast<uint8>(buildingEnum);
	placeBuildingParams.area = TileArea(tile, WorldTile2(1, 1));
	placeBuildingParams.center = tile;
	placeBuildingParams.faceDirection = uint8(Direction::S);
	placeBuildingParams.playerId = playerId;
	
	return AddBuilding(placeBuildingParams);
}

void BuildingSystem::CreateBuilding(CardEnum buildingEnum, std::unique_ptr<Building>& building)
{
#define CASE_BUILDING(buildingEnumA, classTypeB) case buildingEnumA: building = make_unique<classTypeB>(); break;
#define CASE_BUILDING_PARAM(buildingEnumA, classTypeB, paramApplyFunc) case buildingEnumA: { building = make_unique<classTypeB>(); classTypeB* buildingSub = static_cast<classTypeB*>(building.get()); buildingSub->paramApplyFunc; break; }
	
	switch (buildingEnum)
	{
		CASE_BUILDING(CardEnum::House, House);
		CASE_BUILDING(CardEnum::StoneHouse, House);
		CASE_BUILDING(CardEnum::FruitGatherer, GathererHut);
		CASE_BUILDING(CardEnum::Forester, Forester);
		CASE_BUILDING(CardEnum::Townhall, TownHall);
		CASE_BUILDING(CardEnum::StorageYard, StorageYard);

		CASE_BUILDING(CardEnum::GoldMine, GoldMine);
		CASE_BUILDING(CardEnum::Quarry, Quarry);
		
		CASE_BUILDING(CardEnum::IronStatue, IronStatue);
		CASE_BUILDING(CardEnum::Bank, Bank);
		CASE_BUILDING(CardEnum::IceAgeSpire, TempleGrograth);

		CASE_BUILDING(CardEnum::Farm, Farm);
		CASE_BUILDING(CardEnum::MushroomFarm, MushroomFarm);

		CASE_BUILDING(CardEnum::CoalMine, CoalMine);
		CASE_BUILDING(CardEnum::IronMine, IronMine);
		CASE_BUILDING(CardEnum::IronSmelter, IronSmelter);
		CASE_BUILDING(CardEnum::IronSmelterGiant, IronSmelterGiant);

		CASE_BUILDING(CardEnum::GoldSmelter, GoldSmelter);
		CASE_BUILDING(CardEnum::Mint, Mint);
		CASE_BUILDING(CardEnum::InventorsWorkshop, InventorsWorkshop);
		
		//CASE_BUILDING(CardEnum::Barrack, Barrack);

		//CASE_BUILDING_PARAM(CardEnum::BarrackClubman, Barrack, SetArmyEnum(ArmyEnum::Clubman));
		CASE_BUILDING(CardEnum::BarrackSwordman, Barrack);
		CASE_BUILDING(CardEnum::BarrackArcher, Barrack);

		CASE_BUILDING(CardEnum::SmallMarket, SmallMarket);
		CASE_BUILDING(CardEnum::PaperMaker, PaperMaker);

		CASE_BUILDING(CardEnum::StoneToolShopOld, StoneToolsShop);
		CASE_BUILDING(CardEnum::Blacksmith, Blacksmith);
		CASE_BUILDING(CardEnum::Herbalist, Herbalist);
		CASE_BUILDING(CardEnum::MedicineMaker, MedicineMaker);

		CASE_BUILDING(CardEnum::FurnitureWorkshop, FurnitureWorkshop);
		CASE_BUILDING(CardEnum::Chocolatier, Chocolatier);

		// June 1
		CASE_BUILDING(CardEnum::Windmill, Windmill);
		CASE_BUILDING(CardEnum::Bakery, Bakery);
		CASE_BUILDING(CardEnum::GemstoneMine, GemstoneMine);
		CASE_BUILDING(CardEnum::Jeweler, Jeweler);

		// June 9
		CASE_BUILDING(CardEnum::Beekeeper, Beekeeper);
		CASE_BUILDING(CardEnum::Brickworks, Brickworks);
		CASE_BUILDING(CardEnum::CandleMaker, CandleMaker);
		CASE_BUILDING(CardEnum::CottonMill, CottonMill);
		CASE_BUILDING(CardEnum::PrintingPress, PrintingPress);

		CASE_BUILDING(CardEnum::Warehouse, Warehouse);
		CASE_BUILDING(CardEnum::Fort, Fort);
		CASE_BUILDING(CardEnum::ResourceOutpost, ResourceOutpost);

		// August 16
		CASE_BUILDING(CardEnum::FakeTownhall, Building);
		CASE_BUILDING(CardEnum::FakeTribalVillage, Building);
		CASE_BUILDING(CardEnum::ChichenItza, Building);

		// October 20
		CASE_BUILDING(CardEnum::Market, Market);
		CASE_BUILDING(CardEnum::ShippingDepot, ShippingDepot);
		CASE_BUILDING(CardEnum::IrrigationReservoir, IrrigationReservoir);

		// Nov 18
		CASE_BUILDING(CardEnum::GarmentFactory, GarmentFactory);

		// Dec 29
		CASE_BUILDING(CardEnum::ShroomFarm, ShroomFarm);
		CASE_BUILDING(CardEnum::VodkaDistillery, VodkaDistillery);
		CASE_BUILDING(CardEnum::CoffeeRoaster, CoffeeRoaster);

		// Feb 2
		CASE_BUILDING(CardEnum::Colony, Building);
		CASE_BUILDING(CardEnum::PortColony, Building);
		CASE_BUILDING(CardEnum::IntercityLogisticsHub, IntercityLogisticsHub);
		CASE_BUILDING(CardEnum::IntercityLogisticsPort, IntercityLogisticsPort);
		CASE_BUILDING(CardEnum::IntercityBridge, Bridge);

		// Mar 12
		CASE_BUILDING(CardEnum::Granary, Granary);
		CASE_BUILDING(CardEnum::Archives, Archives);
		CASE_BUILDING(CardEnum::HaulingServices, HaulingServices);

		// Apr 1
		CASE_BUILDING(CardEnum::SandMine, SandMine);
		CASE_BUILDING(CardEnum::GlassSmelter, GlassSmelter);
		CASE_BUILDING(CardEnum::Glassworks, Glasswork);
		CASE_BUILDING(CardEnum::ConcreteFactory, ConcreteFactory);
		CASE_BUILDING(CardEnum::CoalPowerPlant, CoalPowerPlant);
		CASE_BUILDING(CardEnum::Steelworks, Steelworks);
		CASE_BUILDING(CardEnum::IndustrialIronSmelter, IndustrialIronSmelter);
		
		CASE_BUILDING(CardEnum::StoneToolsShop, StoneToolsShop);
		CASE_BUILDING(CardEnum::OilRig, OilRig);
		CASE_BUILDING(CardEnum::OilPowerPlant, OilPowerPlant);
		CASE_BUILDING(CardEnum::PaperMill, PaperMill);
		CASE_BUILDING(CardEnum::ClockMakers, ClockMakers);

		CASE_BUILDING(CardEnum::Cathedral, Cathedral);
		CASE_BUILDING(CardEnum::Castle, Castle);
		CASE_BUILDING(CardEnum::GrandPalace, GrandMuseum);
		CASE_BUILDING(CardEnum::ExhibitionHall, ExhibitionHall);


		// Others
		CASE_BUILDING(CardEnum::BoarBurrow, BoarBurrow);

		case CardEnum::DirtRoad:
		case CardEnum::StoneRoad: {
			building = make_unique<RoadConstruction>();
			break;
		}

		  CASE_BUILDING(CardEnum::Fence, Fence);
		  CASE_BUILDING(CardEnum::FenceGate, FenceGate);
		  CASE_BUILDING(CardEnum::Bridge, Bridge);
		  CASE_BUILDING(CardEnum::Tunnel, Tunnel);

		  CASE_BUILDING(CardEnum::Fisher, Fisher);
		  CASE_BUILDING(CardEnum::Winery, Winery);
		  CASE_BUILDING(CardEnum::BlossomShrine, BlossomShrine);

		  CASE_BUILDING(CardEnum::Library, Library);
		  CASE_BUILDING(CardEnum::School, School);

		  CASE_BUILDING(CardEnum::Theatre, Theatre);
		  CASE_BUILDING(CardEnum::Tavern, Tavern);
		  CASE_BUILDING(CardEnum::Tailor, Tailor);

		  CASE_BUILDING(CardEnum::CharcoalMaker, CharcoalMaker);
		  CASE_BUILDING(CardEnum::LaborerGuild, LaborerGuild);
		  CASE_BUILDING(CardEnum::BeerBrewery, BeerBrewery);
		  CASE_BUILDING(CardEnum::ClayPit, ClayPit);
		  CASE_BUILDING(CardEnum::Potter, Potter);
		  CASE_BUILDING(CardEnum::HolySlimeRanch, Building);

		  CASE_BUILDING(CardEnum::TradingPost, TradingPost);
		  CASE_BUILDING(CardEnum::TradingCompany, TradingCompany);
		  CASE_BUILDING(CardEnum::TradingPort, TradingPort);
		  CASE_BUILDING(CardEnum::CardMaker, CardMaker);
		  CASE_BUILDING(CardEnum::ImmigrationOffice, ImmigrationOffice);
		
		  CASE_BUILDING(CardEnum::ThiefGuild, Building);
		  CASE_BUILDING(CardEnum::SlimePyramid, Building);

		  CASE_BUILDING(CardEnum::LovelyHeartStatue, LovelyHeartStatue);

		  CASE_BUILDING(CardEnum::HuntingLodge, HuntingLodge);
		  CASE_BUILDING(CardEnum::RanchBarn, RanchBarn);
		
		  CASE_BUILDING(CardEnum::RanchPig, Ranch);
		  CASE_BUILDING(CardEnum::RanchSheep, Ranch);
		  CASE_BUILDING(CardEnum::RanchCow, Ranch);

		  CASE_BUILDING(CardEnum::TrapSpike, Trap);

		  CASE_BUILDING(CardEnum::ShrineWisdom, ShrineWisdom);
		  CASE_BUILDING(CardEnum::ShrineLove, Shrine);
		  CASE_BUILDING(CardEnum::ShrineGreed, Shrine);
		  CASE_BUILDING(CardEnum::HellPortal, HellPortal);

		  CASE_BUILDING(CardEnum::ArchitectStudio, Building);
		  CASE_BUILDING(CardEnum::EngineeringOffice, Building);
		  CASE_BUILDING(CardEnum::DepartmentOfAgriculture, Building);

		  CASE_BUILDING(CardEnum::StockMarket, Building);
		  CASE_BUILDING(CardEnum::CensorshipInstitute, Building);
		  CASE_BUILDING(CardEnum::EnvironmentalistGuild, Building);
		  CASE_BUILDING(CardEnum::IndustrialistsGuild, Building);
		  CASE_BUILDING(CardEnum::Oracle, Building);
		  CASE_BUILDING(CardEnum::AdventurersGuild, AdventurersGuild);

		  CASE_BUILDING(CardEnum::HumanitarianAidCamp, HumanitarianAidCamp);

		  CASE_BUILDING(CardEnum::RegionTribalVillage, Building);
		  CASE_BUILDING(CardEnum::RegionShrine, RegionShrine);
		  CASE_BUILDING(CardEnum::RegionPort, Building);
		  CASE_BUILDING(CardEnum::RegionCrates, Building);

		  CASE_BUILDING(CardEnum::ConsultingFirm, Building);
		  CASE_BUILDING(CardEnum::ImmigrationPropagandaOffice, Building);
		  CASE_BUILDING(CardEnum::MerchantGuild, Building);
		  CASE_BUILDING(CardEnum::OreSupplier, OreSupplier);
		  CASE_BUILDING(CardEnum::BeerBreweryFamous, BeerBreweryFamous);

		  CASE_BUILDING(CardEnum::Cattery, Building);
		  CASE_BUILDING(CardEnum::InvestmentBank, Bank);

		  CASE_BUILDING(CardEnum::StatisticsBureau, Building);
		  CASE_BUILDING(CardEnum::JobManagementBureau, Building);

	default:
		building = make_unique<Building>();
		break;
	}
#undef CASE_BUILDING
#undef CASE_BUILDING_PARAM
}

int BuildingSystem::AddBuilding(FPlaceBuilding parameters)
{
	// Special case: Colony turn into townhall
	if (IsTownPlacement(static_cast<CardEnum>(parameters.buildingEnum))) {
		parameters.buildingEnum = static_cast<uint8>(CardEnum::Townhall);
	}
	
	WorldTile2 center = parameters.center;
	CardEnum buildingEnum = static_cast<CardEnum>(parameters.buildingEnum);

	if (buildingEnum == CardEnum::IntercityBridge) {
		parameters.playerId = -1;
	}
	
	int32 townId = -1;
	if (parameters.playerId != -1) {
		townId = _simulation->tileOwnerTown(center);

		if (townId == -1) {
			UE_DEBUG_BREAK();
			return -1;
		}
	}

	//if (parameters.playerId != -1 && buildingEnum != CardEnum::DirtRoad) {
	//	//PUN_LOG("parameters.area after %s", *ToFString(parameters.area.ToString()));
	//	//_simulation->DrawArea(parameters.area, FLinearColor::White, -5);
	//}

	/*
	 * All other buildings
	 */
	_alive.push_back(true);

	_buildingEnum.push_back(parameters.buildingEnum);
	_atomLocation.push_back(center.worldAtom2());

	_buildings.push_back(nullptr);

	CreateBuilding(buildingEnum, _buildings.back());

	// TODO: Building reuse system...
	Building* building = _buildings.back().get();
	int32 buildingId = _buildings.size() - 1;
	TileArea area = parameters.area;

	_townIdPlus1ToEnumToBuildingIds[townId + 1][static_cast<int>(buildingEnum)].push_back(buildingId);
	_isBuildingIdConnected.push_back(-1);
	WorldTile2 assumedGateTile = Building::CalculateGateTile(static_cast<Direction>(parameters.faceDirection), center, GetBuildingInfo(buildingEnum).size);
	RefreshIsBuildingConnected(townId, buildingId, assumedGateTile); // Some building needs IsConnected during Init()

	building->Init(*_simulation, buildingId, townId, parameters.buildingEnum,
							area, center, static_cast<Direction>(parameters.faceDirection));

	if (assumedGateTile != building->gateTile()) {
		RefreshIsBuildingConnected(townId, buildingId, building->gateTile());
	}

	_buildingSubregionList.Add(center, buildingId);

	building->ResetDisplay();

	PlaceBuildingOnMap(buildingId, true);

	// Should CheckAdjacency after placing it on world map
	building->CheckAdjacency();

	return buildingId;
}

void BuildingSystem::PlaceBuildingOnMap(int32 buildingIdIn, bool isBuildingInitialAdd, bool isAdding)
{
	Building& bld = building(buildingIdIn);
	CardEnum buildingEnum = static_cast<CardEnum>(_buildingEnum[buildingIdIn]);
	WorldTile2 center = bld.centerTile();
	TileArea area = bld.area();

	int32 bldId = isAdding ? buildingIdIn : -1;
	
	// Stack Tile Buildings
	if (IsRoad(buildingEnum))
	{
		//// Already a trap here, put this as trap's child
		//// for example: trap built on road construction, flower next to the road, Custom buildings??
		//// Could use this same trick on Unit's tile?? (Double unit one same tile becomes child unit..
		//int32 parentId = buildingIdAtTile(center);
		//if (parentId != -1)
		//{
		//	PUN_CHECK(IsRoadOverlapBuilding(buildingEnumAtTile(center)));

		//	_buildings[parentId]->children.push_back(buildingId);
		//	_buildings[buildingId]->parent = parentId;
		//	return;
		//}

		// Otherwise, we need to set the tile for this
		SetBuildingTile(center, bldId);

		return;
	}
	//if (IsRoadOverlapBuilding(buildingEnum))
	//{
	//	// If already road here, replace road as the parent, and put road as child
	//	int32 existingId = buildingIdAtTile(center);
	//	if (existingId != -1) {
	//		PUN_CHECK(IsRoad(buildingEnumAtTile(center)));

	//		_buildings[buildingId]->children.push_back(existingId);
	//		_buildings[existingId]->parent = buildingId;
	//	}

	//	SetBuildingTile(center, buildingId, buildingEnum);
	//	return;
	//}

	// Fence special case
	if (buildingEnum == CardEnum::Fence)
	{
		SetBuildingTile(center, bldId);
		return;
	}

	// Bridge special case
	if (IsBridgeOrTunnel(buildingEnum))
	{
		area.ExecuteOnArea_WorldTile2([&](WorldTile2 tile) {
			SetBuildingTile(tile, bldId);
		});
		return;
	}

	/*
	 * Typical buildings
	 */

	area.ExecuteOnArea_WorldTile2([&](WorldTile2 tile) {
		SetBuildingTile(tile, bldId);
	});

	// Front Grid
	if (HasBuildingFront(buildingEnum) &&
		buildingEnum != CardEnum::RegionTribalVillage)
	{
		Direction faceDirection = bld.faceDirection();
		TileArea frontArea = area.GetFrontArea(faceDirection);
		frontArea.ExecuteOnArea_WorldTile2([&](WorldTile2 tile) {
			PUN_CHECK(!isBuildingInitialAdd || _simulation->IsFrontBuildable(tile));
			SetFrontTile(tile, faceDirection, bldId);
		});
	}

	//if (parameters.playerId != -1) {
	//	PUN_LOG("AddBuilding: %d, %d", _buildings.size(), parameters.buildingEnum);
	//}

	// Check combo only for newly created building (not loaded one)
	if (isBuildingInitialAdd) {
		bld.CheckCombo();
	}
}

//void BuildingSystem::RemoveBuildingFromBuildingSystem(int32 buildingId)
//{
//	CardEnum buildingEnum = _buildings[buildingId]->buildingEnum();
//	WorldTile2 centerTile = _buildings[buildingId]->centerTile();
//	int32 playerId = _buildings[buildingId]->playerId();
//
//
//	_buildingSubregionList.Remove(centerTile, buildingId);
//	CppUtils::Remove(_playerIdPlus1ToEnumToBuildingIds[playerId + 1][static_cast<int>(buildingEnum)], buildingId);
//	_alive[buildingId] = false;
//}

void BuildingSystem::RemoveBuilding(int buildingId)
{
	//PUN_LOG("RemoveBuilding %d ticks:%d", buildingId, Time::Ticks());
	
	// Prepare
	CardEnum buildingEnum = _buildings[buildingId]->buildingEnum();
	TileArea area = _buildings[buildingId]->area();
	WorldTile2 centerTile = _buildings[buildingId]->centerTile();
	Direction faceDirection = _buildings[buildingId]->faceDirection();
	TileArea frontArea = area.GetFrontArea(faceDirection);
	int32 townId = _buildings[buildingId]->townId();
	bool isConstructed = _buildings[buildingId]->isConstructed();
	

	PlaceBuildingOnMap(buildingId, false, false);

	//RemoveBuildingFromBuildingSystem(buildingId);
	// Remove from system
	_buildingSubregionList.Remove(centerTile, buildingId);
	CppUtils::Remove(_townIdPlus1ToEnumToBuildingIds[townId + 1][static_cast<int>(buildingEnum)], buildingId);
	_alive[buildingId] = false;

	// Reset display/UI
	_simulation->SetNeedDisplayUpdate(DisplayClusterEnum::Building, centerTile.regionId());
	_simulation->SetNeedDisplayUpdate(DisplayClusterEnum::BuildingAnimation, centerTile.regionId());

	_simulation->TryRemoveDescriptionUI(ObjectTypeEnum::Building, buildingId);

	// Remove from scheduleTicks
	for (size_t i = _scheduleBuildingIds.size(); i-- > 0;) {
		if (_scheduleBuildingIds[i] == buildingId) {
			_scheduleBuildingIds.erase(_scheduleBuildingIds.begin() + i);
			_scheduleTicks.erase(_scheduleTicks.begin() + i);
		}
	}

	_isBuildingIdConnected[buildingId] = -1;
	
	_buildings[buildingId]->Deinit();

	// Note: should check after PlaceBuildingOnMap
	_buildings[buildingId]->CheckAdjacency(true, true);

	RemoveQuickBuild(buildingId);
}

int32 BuildingSystem::GetHouseLvlCount(int32 playerId, int32 houseLvl, bool includeHigherLvl) {
	const std::vector<int32>& houseIds = buildingIds(playerId, CardEnum::House);
	int32 count = 0;
	for (int32 houseId : houseIds) {
		House& house = building(houseId).subclass<House>(CardEnum::House);
		if (house.isConstructed()) {
			if (includeHigherLvl) {
				if (house.houseLvl() >= houseLvl) count++;
			} else {
				if (house.houseLvl() == houseLvl) count++;
			}
		}
	}
	return count;
}