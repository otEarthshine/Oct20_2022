#include "PunCity/AssetLoaderComponent.h"
#include "Engine/StaticMesh.h"
#include "StaticMeshResources.h"
#include "Particles/ParticleSystem.h"

#include "Materials/Material.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "Materials/MaterialParameterCollection.h"
#include "Kismet/KismetMaterialLibrary.h"

#include "PunCity/GameRand.h"
#include "RawMesh.h"

#include "PunCity/PunUtils.h"
#include <algorithm>
#include <sstream>
#include "HAL/PlatformFilemanager.h"
#include "CppUtils.h"

using namespace std;

const ModuleTransformGroup ModuleTransformGroup::Empty = ModuleTransformGroup();

// Sets default values for this component's properties
UAssetLoaderComponent::UAssetLoaderComponent()
{
	PrimaryComponentTick.bCanEverTick = false;

	// Note: omit check() since error comes out in the log anyway
	GroundMesh = Load<UStaticMesh>("/Game/Models/Ground/Ground");

	WaterMesh = Load<UStaticMesh>("/Game/Models/Water/Water");

	PlayerFlagMesh = Load<UStaticMesh>("/Game/Models/PlayerTag/PlayerTagMesh");
	PlayerFlagMaterial = Load<UMaterial>("/Game/Models/PlayerTag/PlayerTagMaterial");

	SelectionMesh = Load<UStaticMesh>("/Game/Models/SelectionMesh/SelectionMesh");
	SelectionMaterialGreen = Load<UMaterial>("/Game/Models/SelectionMesh/SelectionMaterialGreen");
	SelectionMaterialYellow = Load<UMaterial>("/Game/Models/SelectionMesh/SelectionMaterialYellow");

	UpgradableMesh = Load<UStaticMesh>("/Game/Models/SelectionMesh/UpgradableMesh");
	AttentionCircle = Load<UStaticMesh>("/Game/Models/SelectionMesh/AttentionCircle");
	AttentionMark = Load<UStaticMesh>("/Game/Models/SelectionMesh/AttentionMark");

	GatherMarkMeshMaterial = Load<UStaticMesh>("/Game/Models/Trees/GatherMark");


	/**
	 * Building
	 */

	WorldMapMesh = Load<UStaticMesh>("/Game/Models/WorldMap/WorldMap");
	WorldMapMeshSubdivide = Load<UStaticMesh>("/Game/Models/WorldMap/WorldMapSubdivide");
	
	WorldMapMeshWater = Load<UStaticMesh>("/Game/Models/WorldMap/WorldMapWater");
	WorldMapMeshWaterOutside = Load<UStaticMesh>("/Game/Models/WorldMap/WorldMapWaterOutside");

	M_HiddenInMainPass = Load<UMaterial>("/Game/Models/WorldMap/M_HiddenInMainPass");
	
	WorldMapCityBlock = Load<UStaticMesh>("/Game/Models/WorldMap/WorldMapCityBlock");
	WorldMapFlag = Load<UStaticMesh>("/Game/Models/WorldMap/WorldMapFlag");
	WorldMapFlagBase = Load<UStaticMesh>("/Game/Models/WorldMap/WorldMapFlagBase");
	WorldMapCameraSpot = Load<UStaticMesh>("/Game/Models/WorldMap/WorldMapCameraSpot");

	WorldMapBlockMaterial = Load<UMaterial>("/Game/Models/WorldMap/WorldMapBlockMaterial");

	WorldMapForestMesh = Load<UStaticMesh>("/Game/Models/WorldMap/WorldMapMeshes/MapForestMesh");


	// !!! There is an issue with LayerBlend node being loaded in constructor..
	// As a workaround, we just loadup single materials that does use LayerBlend nodes, this way UE4 can keep track of references.
	ReferenceTerrainMaterial = Load<UMaterial>("/Game/Models/WorldMap/M_ReferenceTerrain");


	BuildingIconMaterial = Load<UMaterial>("/Game/UI/UIMaterials/M_BuildingIcon");
	BuildingIconGrayMaterial = Load<UMaterial>("/Game/UI/UIMaterials/M_BuildingIconGray");

	CardIconMaterial = Load<UMaterial>("/Game/UI/UIMaterials/M_CardIcon");
	CardIconGrayMaterial = Load<UMaterial>("/Game/UI/UIMaterials/M_CardIconGray");

	CardFront = Load<UTexture2D>("/Game/UI/Images/CardFront");
	CardFrontBevel = Load<UTexture2D>("/Game/UI/Images/CardFrontBevel");
	CardFrontRound = Load<UTexture2D>("/Game/UI/Images/CardFrontRound");

	CardSlot = Load<UTexture2D>("/Game/UI/Images/CardSlot");
	CardSlotBevel = Load<UTexture2D>("/Game/UI/Images/CardSlotBevel");
	CardSlotRound = Load<UTexture2D>("/Game/UI/Images/CardSlotRound");

	CardBack = Load<UTexture2D>("/Game/UI/Images/CardBackSide_FullCard");

	ResourceIconMaterial = Load<UMaterial>("/Game/UI/ResourceIcons/ResourceIconMaterial");
	NoResourceIconMaterial = Load<UMaterial>("/Game/UI/UIMaterials/NoResourceIconMaterial");
	//M_HoverIcon = Load<UMaterial>("/Game/UI/UIMaterials/M_HoverIcon");

	HouseIcon = Load<UTexture2D>("/Game/UI/MiscIcons/HousePNG");
	SmileIcon = Load<UTexture2D>("/Game/UI/MiscIcons/SmilingPNG");
	UnhappyIcon = Load<UTexture2D>("/Game/UI/MiscIcons/UnhappyPNG");
	//CoinIcon = Load<UTexture2D>("/Game/UI/MiscIcons/CoinPNG");
	CoinIcon = Load<UTexture2D>("/Game/UI/MiscIcons/CoinGameReady");
	InfluenceIcon = Load<UTexture2D>("/Game/UI/MiscIcons/InfluencePNG");
	ScienceIcon = Load<UTexture2D>("/Game/UI/MiscIcons/SciencePNG");

	HappinessGreenIcon = Load<UTexture2D>("/Game/UI/MiscIcons/HappinessGreenPNG");
	HappinessYellowIcon = Load<UTexture2D>("/Game/UI/MiscIcons/HappinessYellowPNG");
	HappinessOrangeIcon = Load<UTexture2D>("/Game/UI/MiscIcons/HappinessOrangePNG");
	HappinessRedIcon = Load<UTexture2D>("/Game/UI/MiscIcons/HappinessRedPNG");

	CoinGrayIcon = Load<UTexture2D>("/Game/UI/MiscIcons/CoinGrayPNG");
	ClockIcon = Load<UTexture2D>("/Game/UI/MiscIcons/ClockPNG");
	ClockGrayIcon = Load<UTexture2D>("/Game/UI/MiscIcons/ClockGrayPNG");
	

	ExclamationIcon = Load<UTexture2D>("/Game/UI/Images/ExclamationIconPNG");

	AdultIcon = Load<UTexture2D>("/Game/UI/Images/HumanIcon");
	AdultIconSquare = Load<UTexture2D>("/Game/UI/Images/HumanIconSquare");

	UnhappyHoverIcon = Load<UTexture2D>("/Game/UI/Images/Unhappy");

	BlackIcon = Load<UTexture2D>("/Game/UI/GeneratedIcons/BlackIcon");
	WhiteIcon = Load<UTexture2D>("/Game/UI/GeneratedIcons/WhiteIcon");

	M_GeoresourceIcon = Load<UMaterial>("/Game/UI/GeoresourceIcons/M_GeoresourceIcon");

	//! Hover warning
	M_HoverWarning = Load<UMaterial>("/Game/UI/Images/M_HoverWarning");
	M_HoverWarningHappiness = Load<UMaterial>("/Game/UI/Images/M_HoverWarningHappiness");
	WarningHouse = Load<UTexture2D>("/Game/UI/Images/HouseIcon_diffuse");
	WarningStarving = Load<UTexture2D>("/Game/UI/Images/StarvingIcon");
	WarningSnow = Load<UTexture2D>("/Game/UI/Images/SnowIcon_diffuse");
	WarningHealthcare = Load<UTexture2D>("/Game/UI/Images/Healthcare");
	WarningTools = Load<UTexture2D>("/Game/UI/Images/ToolsWarning");
	WarningUnhappy = Load<UTexture2D>("/Game/UI/Images/Unhappy");
	

	//! Video
	//MediaPlayer = Load<UMediaPlayer>("/Game/UI/Images/Unhappy");

	

	//! GeoresourceIcon
	auto loadCustomGeoIcon = [&](GeoresourceEnum georesourceEnum, std::string path) {
		int32 geoInt = static_cast<int32>(georesourceEnum);
		_geo_Icon.Add(geoInt, Load<UTexture2D>(path.c_str()));
		_geo_IconAlpha.Add(geoInt, Load<UTexture2D>((path + "Alpha").c_str()));
	};
	loadCustomGeoIcon(GeoresourceEnum::GiantMushroom, "/Game/UI/GeneratedIcons/Geo_Mushroom");
	loadCustomGeoIcon(GeoresourceEnum::CannabisFarm, "/Game/UI/GeneratedIcons/Geo_Cannabis");
	loadCustomGeoIcon(GeoresourceEnum::CherryBlossom, "/Game/UI/GeneratedIcons/Geo_Cherry");
	loadCustomGeoIcon(GeoresourceEnum::Ruin, "/Game/UI/GeneratedIcons/Geo_Ruin");

	EmptyMesh = Load<UStaticMesh>("/Game/Models/WorldMap/EmptyMeshTest");

	// Modules:
	_modulesNeedingPaintConstruction.Empty();
	_moduleNames.Empty();
	

	/*
	 * Building Modules
	 */
	_buildingEnumToModuleGroups.SetNum(BuildingEnumCount);
	_buildingEnumToMinEraModel.SetNum(BuildingEnumCount);
	
	LoadBuilding(CardEnum::FruitGatherer, "Fruit_GathererEra", "FruitGatherer", 1);
	LoadBuilding(CardEnum::Bank, "Bank_Era", "Bank", 3);

	LoadBuilding(CardEnum::StoneToolsShop, "StoneToolsShop", "StoneToolShop");
	LoadBuilding(CardEnum::Blacksmith, "Blacksmith_Era", "Blacksmith", 2);
	LoadBuilding(CardEnum::MedicineMaker, "MedicineMaker_Era", "MedicineMaker", 2);

	LoadBuilding(CardEnum::BeerBrewery, "Brewery_Era", "BeerBrewery", 1);
	
	LoadBuilding(CardEnum::Chocolatier, "Chocolatier_Era", "Chocolatier", 3);

	LoadBuilding(CardEnum::MushroomFarm, "Mushroom_FarmEra", "MushroomFarm", 1, 4);

	LoadBuilding(CardEnum::Garden, "Garden_Variation1", "Garden/Variation1");
	//TryLoadBuildingModuleSet("Garden2", "Garden/Variation2");
	//TryLoadBuildingModuleSet("Garden3", "Garden/Variation3");

	LoadBuilding(CardEnum::Fisher, "Fishing_Lodge_Era_", "FishingLodge", 1);

	LoadBuilding(CardEnum::Winery, "Winery_Era_", "Winery", 2);

	LoadBuilding(CardEnum::Library, "Library_Era", "Library", 2);
	LoadBuilding(CardEnum::School, "College_Era", "College", 3);

	LoadBuilding(CardEnum::PaperMaker, "PaperMakerEra", "PaperMaker", 2, 2);
	
	LoadBuilding(CardEnum::Theatre, "Theatre_Era_", "Theatre", 3);
	LoadBuilding(CardEnum::Tavern, "TavernEra", "Tavern", 1);
	LoadBuilding(CardEnum::Tailor, "TailorEra", "Tailor", 2);

	LoadBuilding(CardEnum::ClayPit, "ClayPit_Era", "ClayPit", 1, 1);
	LoadBuilding(CardEnum::Potter, "potterEra", "Potter", 1);

	LoadBuilding(CardEnum::TradingPort, "TradingPort_Era", "TradingPort", 1);
	LoadBuilding(CardEnum::TradingPost, "TradingPost_Era", "TradingPost", 1);
	LoadBuilding(CardEnum::TradingCompany, "TradingCompany_Era", "TradingCompany", 2);

	//LoadBuilding(CardEnum::CardMaker, "Scholars_Office_Era_", "ScholarsOffice", 2);
	LoadBuilding(CardEnum::CardMaker, "ScholarsOffice_Era2", "ScholarsOffice/Era2", false, {}, 2);
	LoadBuilding(CardEnum::CardMaker, "ScholarsOffice_Era3", "ScholarsOffice/Era3", false, {}, 3);
	LoadBuilding(CardEnum::CardMaker, "Scholars_Office_Era_4", "ScholarsOffice/Era4", false, {}, 4);
	
	LoadBuilding(CardEnum::Archives, "Archives_Era", "Archives", 2);

	LoadBuilding(CardEnum::ImmigrationOffice, "Immigration_Office_Era_", "ImmigrationOffice", 1);
	
	LoadBuilding(CardEnum::HuntingLodge, "Hunting_LodgeERA", "HuntingLodge", 1);

	LoadBuilding(CardEnum::RanchPig, "pigranchERA", "Ranch", 1);
	LinkBuildingEras(CardEnum::RanchSheep, "pigranchERA", 1);
	LinkBuildingEras(CardEnum::RanchCow, "pigranchERA", 1);

	LoadBuilding(CardEnum::GoldSmelter, "GoldSmelter_Era", "SmelterGold", 2);
	LoadBuilding(CardEnum::Mint, "Mint_Era", "Mint", 3);
	LoadBuilding(CardEnum::Jeweler, "Jeweler_Era", "Jeweler", 4);
	LoadBuilding(CardEnum::CandleMaker, "CandleMaker_Era", "CandleMaker", 2);

	LoadBuilding(CardEnum::CottonMill, "CottonMill_Era", "TextileMill", 4);
	LoadBuilding(CardEnum::PrintingPress, "PrintingPress_Industrial_PrintingPress_Era", "PrintingPress", 4);

	LoadBuilding(CardEnum::Warehouse, "wherehouse_era", "Warehouse", 2);
	LoadBuilding(CardEnum::ShippingDepot, "LogisticsSenderOfficeERA", "LogisticsSender", 3);
	LoadBuilding(CardEnum::HaulingServices, "HaulingServicecERA", "HaulingServices", 2);

	LoadBuilding(CardEnum::MagicMushroomFarm, "ShroomFarmEra", "MagicMushroomFarm", 3);

	LoadBuilding(CardEnum::VodkaDistillery, "Vodka_DistilleryERA", "VodkaDistillery", 2);

	LoadBuilding(CardEnum::CoffeeRoaster, "CoffeeRoaster_Era", "CoffeeRoaster", 3);

	LoadBuilding(CardEnum::Granary, "GRANARY_ERA", "Granary", 2);

	LoadBuilding(CardEnum::SandMine, "SandMine_Era", "SandMine", 3);
	LoadBuilding(CardEnum::GlassSmelter, "GlassSmelter_Era", "GlassSmelter", 3);
	LoadBuilding(CardEnum::Glassworks, "GlassWorks_Era", "GlassWorks", 3);
	LoadBuilding(CardEnum::ConcreteFactory, "concretefactoryERA", "ConcreteFactory", 4);
	
	LoadBuilding(CardEnum::CoalPowerPlant, "coalpowerplant", "CoalPowerPlant/Era4");
	LoadBuilding(CardEnum::IndustrialIronSmelter, "IndustrialIronSmelterERA", "IndustrialIronSmelter/Era4");
	LoadBuilding(CardEnum::Steelworks, "SteelworksEra4", "Steelworks");
	LoadBuilding(CardEnum::OilRig, "OilWell_Era", "OilWell");
	LoadBuilding(CardEnum::OilPowerPlant, "OilPowerPlantERA", "OilPowerPlant/Era4");
	LoadBuilding(CardEnum::PaperMill, "PaperMill_PaperMill_Era4", "PaperMill");
	LoadBuilding(CardEnum::ClockMakers, "Clock_Maker_Era", "ClockMaker");

	LoadBuilding(CardEnum::Cathedral, "Cathedral_Era2", "Cathedral");
	LoadBuilding(CardEnum::GrandPalace, "Grand_Museum_Era_", "GrandMuseum", 4);
	LoadBuilding(CardEnum::ExhibitionHall, "Crystal_Palace", "CrystalPalace", false, ModuleTransformGroup::CreateAuxSet(
		{}, {}, {},
		{
			{0.4f, 70.0f, FLinearColor(1, 0.651, 0.246), FVector(-40, 0, 20), FVector::OneVector},
			{0.4f, 70.0f, FLinearColor(1, 0.651, 0.246), FVector(70, 0, 20), FVector::OneVector}
		}
	));
	LoadBuilding(CardEnum::Castle, "Castle", "Castle");

	LoadBuilding(CardEnum::StatisticsBureau, "Statistic_Bureau_Era_", "StatisticsBureau", 1);
	LoadBuilding(CardEnum::JobManagementBureau, "Employment_Bureau_Era_", "EmploymentBureau", 1);

	// Test Townhall
	LoadBuilding(CardEnum::Townhall, "Townhall_Era0", "Townhall/Era0", false, ModuleTransformGroup::CreateAuxSet(
		{
			{ParticleEnum::CampFire, TransformFromPositionYawScale(-5.65, -.669, 1, 0, 0.17)}
		},
		{}, {},
		{ {0.12f, 35.0f, FLinearColor(1, 0.527f, 0.076f), FVector(-5.65, -.669, 5), FVector::OneVector} }
	));
	LoadBuilding(CardEnum::Townhall, "Townhall_Era1", "Townhall/Era1", false, ModuleTransformGroup::CreateAuxSet(
		{
			{ParticleEnum::CampFire, TransformFromPositionYawScale(-10.7, 14.6, 1.85, 0, 0.17)}
		},
		{}, {},
		{ {0.12f, 35.0f, FLinearColor(1, 0.527f, 0.076f), FVector(-10.7, 14.6, 8.5), FVector::OneVector} }
	));
	LoadBuilding(CardEnum::Townhall, "Townhall_Era2", "Townhall/Era2", false, ModuleTransformGroup::CreateAuxSet(
		{
			//{ParticleEnum::Smoke, TransformFromPosition(51.6, -24.3, 38.8)},
			{ParticleEnum::CampFire, TransformFromPositionYawScale(-15.38, 6.41, 1.85, 0, 0.17)}
		},
		{}, {},
		{ {0.12f, 35.0f, FLinearColor(1, 0.527f, 0.076f), FVector(-15.38, 6.41, 1.85), FVector::OneVector} }
	));
	LoadBuilding(CardEnum::Townhall, "Townhall_Era3", "Townhall/Era3", false, ModuleTransformGroup::CreateAuxSet(
		{},
		{}, {},
		{ {0.12f, 35.0f, FLinearColor(1, 0.527f, 0.076f), FVector(-2.3, 10.2, 8.5), FVector::OneVector} }
	));
	LoadBuilding(CardEnum::Townhall, "Townhall_Era4", "Townhall/Era4", false, ModuleTransformGroup::CreateAuxSet(
		{},
		{}, {},
		{ {0.12f, 35.0f, FLinearColor(1, 0.527f, 0.076f), FVector(-2.3, 10.2, 8.5), FVector::OneVector} }
	));

	
	LinkBuilding(CardEnum::Colony, "Townhall_Era0");
	LinkBuilding(CardEnum::PortColony, "Townhall_Era0");
	

	LoadBuilding(CardEnum::FurnitureWorkshop, "Furniture_Workshop_Era", "FurnitureWorkshop", 1);


	LoadBuilding(CardEnum::CharcoalMaker, "CharcoalMakerEra1", "CharcoalBurner/Era1", false, ModuleTransformGroup::CreateAuxSet(
		{
			{ParticleEnum::BlackSmoke,  TransformFromPosition(3.6, 7.92, 9.0)}, // 8.2 -> 5.6 ... decrease by 2.6
			{ParticleEnum::TorchFire,  FTransform(FRotator::ZeroRotator, FVector(3.6, 7.92, 11.5), FVector(1, 1, 1))}
		}
	));
	LoadBuilding(CardEnum::CharcoalMaker, "CharcoalMakerEra2", "CharcoalBurner/Era2", false, ModuleTransformGroup::CreateAuxSet(
		{
			{ParticleEnum::BlackSmoke,  TransformFromPosition(3.6, 8.53, 9.0)}, // 8.2 -> 5.6 ... decrease by 2.6
			{ParticleEnum::TorchFire,  FTransform(FRotator::ZeroRotator, FVector(3.6, 8.53, 11.5), FVector(1, 1, 1))}
		}
	));
	LoadBuilding(CardEnum::CharcoalMaker, "CharcoalMakerEra3", "CharcoalBurner/Era3", false, ModuleTransformGroup::CreateAuxSet(
		{
			{ParticleEnum::BlackSmoke,  TransformFromPosition(3.6, 8.3, 20.0)}, // 8.2 -> 5.6 ... decrease by 2.6
			{ParticleEnum::TorchFire,  FTransform(FRotator::ZeroRotator, FVector(3.6, 8.3, 22.6), FVector(1, 1, 1))}
		}
	));
	LoadBuilding(CardEnum::CharcoalMaker, "CharcoalMakerEra4", "CharcoalBurner/Era4", false, ModuleTransformGroup::CreateAuxSet(
		{
			{ParticleEnum::BlackSmoke,  TransformFromPosition(3.3, 8.37, 21.3)}, // 8.2 -> 5.6 ... decrease by 2.6
			{ParticleEnum::TorchFire,  FTransform(FRotator::ZeroRotator, FVector(3.3, 8.37, 23.9), FVector(1, 1, 1))}
		}
	));

	
	LoadBuilding(CardEnum::Brickworks, "BrickworkEra", "Brickworks", 2);
	
	LoadBuilding(CardEnum::Beekeeper, "BeeKeeper_Era", "Beekeeper", 2);
	LoadBuilding(CardEnum::Market, "Market__Era", "Market", 2, 2);

	LoadBuilding(CardEnum::IrrigationReservoir, "IrrigationReservoir", "IrrigationReservoir");
	
	LoadBuilding(CardEnum::Bakery, "Bakery_Era", "Bakery", 2);
	LoadBuilding(CardEnum::IronSmelter, "Iron_Smelter_Era", "IronSmelter", 2, 2);

	
	LoadBuilding(CardEnum::Quarry, "Quarry_Era", "Quarry", 1);

	for (int32 eraInt = 1; eraInt <= 4; eraInt++)
	{
		FString era = FString::FromInt(eraInt);
		LoadBuilding(CardEnum::GoldMine, "Ore_Mine_Era" + era, "OreMine/Era" + era);
		LinkBuilding(CardEnum::IronMine, "Ore_Mine_Era" + era, _lastTempAuxGroup);
		LinkBuilding(CardEnum::GemstoneMine, "Ore_Mine_Era" + era, _lastTempAuxGroup);
		LinkBuilding(CardEnum::CoalMine, "Ore_Mine_Era" + era, _lastTempAuxGroup);

		{
			//PUN_LOG("_recentlyAddedModuleNames %d", _recentlyAddedModuleNames.Num());

			auto manualAddTransforms = [&](CardEnum buildingEnum, FString searchString)
			{
				for (int32 i = _recentlyAddedModuleNames.Num(); i-- > 0;) {
					if (_recentlyAddedModuleNames[i].Contains("_Manual_"))
					{
						if (_recentlyAddedModuleNames[i].Contains(searchString)) {
							_buildingEnumToModuleGroups[static_cast<int>(buildingEnum)][eraInt - 1].transforms.push_back(ModuleTransform(_recentlyAddedModuleNames[i]));
						}
					}
					else if (_recentlyAddedModuleNames[i].Contains("_ManualToggle_"))
					{
						if (_recentlyAddedModuleNames[i].Contains(searchString)) {
							_buildingEnumToModuleGroups[static_cast<int>(buildingEnum)][eraInt - 1].togglableTransforms.push_back(ModuleTransform(_recentlyAddedModuleNames[i]));
						}
					}
				}
			};

			manualAddTransforms(CardEnum::GoldMine, "Gold");
			manualAddTransforms(CardEnum::IronMine, "Iron");
			manualAddTransforms(CardEnum::CoalMine, "Coal");
			manualAddTransforms(CardEnum::GemstoneMine, "Gem");
		}
	}

	//

	//set(CardEnum::CharcoalMaker, {
	//ModuleTransformGroup::CreateSet("CharcoalMaker", {},
	//{
	//	{ParticleEnum::BlackSmoke,  TransformFromPosition(9.8, 5.9, 5.6)},
	//	{ParticleEnum::TorchFire,  FTransform(FRotator::ZeroRotator, FVector(9.11, 5.27, 8.2), FVector(1, 1, 1))},
	//})
	//	});
	
	// -
	//
		//

		//
		//	//
		//

	TryLoadBuildingModuleSet("Quarry", "Quarry");
	LoadAnimModule("QuarrySpecialToggle", "Quarry/QuarrySpecialToggle");
	
	TryLoadBuildingModuleSet("OreMine", "OreMine");
	//LoadAnimModule("OreMineSpecial_Stone", "OreMine/OreMineSpecial_Stone");
	LoadAnimModule("OreMineSpecial_Coal", "OreMine/OreMineSpecial_Coal");
	LoadAnimModule("OreMineSpecial_Iron", "OreMine/OreMineSpecial_Iron");
	LoadAnimModule("OreMineSpecial_Gold", "OreMine/OreMineSpecial_Gold");
	LoadAnimModule("OreMineSpecial_Gemstone", "OreMine/OreMineSpecial_Gemstone");
	
	LoadTogglableModule("OreMineWorkStatic_Stone", "OreMine/StoneSpecial");
	LoadTogglableModule("OreMineWorkStatic_Coal", "OreMine/CoalSpecial");
	LoadTogglableModule("OreMineWorkStatic_Iron", "OreMine/IronOreSpecial");
	LoadTogglableModule("OreMineWorkStatic_Gold", "OreMine/GoldOreSpecial");
	LoadTogglableModule("OreMineWorkStatic_Gemstone", "OreMine/GemstoneSpecial");
	
	
	//TryLoadBuildingModuleSet("Smelter", "Smelter");
	TryLoadBuildingModuleSet("SmelterGold", "SmelterGold");
	TryLoadBuildingModuleSet("SmelterGiant", "SmelterGiant");
	
	//TryLoadBuildingModuleSet("CharcoalMaker", "CharcoalMaker");
	TryLoadBuildingModuleSet("Forester", "Forester");

	
	TryLoadBuildingModuleSet("StorageYard", "StorageYard");

	TryLoadBuildingModuleSet("FlowerBed", "FlowerBed");
	TryLoadBuildingModuleSet("GardenCypress", "GardenCypress");
	TryLoadBuildingModuleSet("GardenShrubbery1", "GardenShrubbery1");

	TryLoadBuildingModuleSet("IntercityLogisticsHub", "IntercityLogisticsHub");
	TryLoadBuildingModuleSet("IntercityLogisticsPort", "IntercityLogisticsPort");
	


	// Animal modules
	LoadBuilding(CardEnum::BoarBurrow, "BoarBurrow", "BoarBurrow");

	// Houses
	// Note: Model count starts with House Lvl 0 (legacy imports, not worth the change)

	// 5 Variations per house lvl
	
	// Lvl 0 (1)
	for (int32 i = 1; i <= 3; i++) {
		FString moduleGroupName = "HouseERA0V" + FString::FromInt(i);
		FString moduleGroupFolderName = "House/Level0/V" + FString::FromInt(i);
		LoadBuilding(CardEnum::House, moduleGroupName, moduleGroupFolderName);
	}
	TryLoadBuildingModuleSet("HouseClayLvl1", "HouseClayLvl1");
	LinkBuilding(CardEnum::House, "HouseClayLvl1");
	LinkBuilding(CardEnum::House, "HouseClayLvl1");

	// Lvl 1 (2)
	for (int32 i = 1; i <= 3; i++) {
		FString moduleGroupName = "House_Lvl1_Var" + FString::FromInt(i);
		FString moduleGroupFolderName = "House/Level1/V" + FString::FromInt(i);
		LoadBuilding(CardEnum::House, moduleGroupName, moduleGroupFolderName);
	}
	LinkBuilding(CardEnum::House, "HouseClayLvl1");
	LinkBuilding(CardEnum::House, "HouseClayLvl1"); // Buffer

	// Lvl 2 (3)
	for (int32 i = 1; i <= 4; i++) {
		FString moduleGroupName = "House_LV2_V" + FString::FromInt(i);
		FString moduleGroupFolderName = "House/Level2/V" + FString::FromInt(i);
		LoadBuilding(CardEnum::House, moduleGroupName, moduleGroupFolderName);
	}
	TryLoadBuildingModuleSet("HouseClayLvl2", "HouseClayLvl2");
	LinkBuilding(CardEnum::House, "HouseClayLvl2");

	// Lvl 3 (4)
	for (int32 i = 1; i <= 4; i++) {
		FString moduleGroupName = "House_LV3_V" + FString::FromInt(i);
		FString moduleGroupFolderName = "House/Level3/V" + FString::FromInt(i);
		LoadBuilding(CardEnum::House, moduleGroupName, moduleGroupFolderName);
	}
	TryLoadBuildingModuleSet("HouseClayLvl3", "HouseClayLvl3");
	LinkBuilding(CardEnum::House, "HouseClayLvl3");

	

	// Lvl 4 (5)
	for (int32 i = 1; i <= 4; i++) {
		FString moduleGroupName = "House_LV5_V" + FString::FromInt(i);
		FString moduleGroupFolderName = "House/Level5/V" + FString::FromInt(i);
		LoadBuilding(CardEnum::House, moduleGroupName, moduleGroupFolderName);
	}
	LinkBuilding(CardEnum::House, "HouseClayLvl1"); // Buffer

	// Lvl 5 (6)
	for (int32 i = 1; i <= 3; i++) {
		FString moduleGroupName = "House_LV6_V" + FString::FromInt(i);
		FString moduleGroupFolderName = "House/Level6/V" + FString::FromInt(i);
		LoadBuilding(CardEnum::House, moduleGroupName, moduleGroupFolderName);
	}
	LinkBuilding(CardEnum::House, "HouseClayLvl1"); // Buffer
	LinkBuilding(CardEnum::House, "HouseClayLvl1"); // Buffer

	// Lvl 6 (7)
	for (int32 i = 1; i <= 2; i++) {
		FString moduleGroupName = "House_LV7_V" + FString::FromInt(i);
		FString moduleGroupFolderName = "House/Level7/V" + FString::FromInt(i);
		LoadBuilding(CardEnum::House, moduleGroupName, moduleGroupFolderName);
	}
	LinkBuilding(CardEnum::House, "HouseClayLvl1"); // Buffer
	LinkBuilding(CardEnum::House, "HouseClayLvl1"); // Buffer
	LinkBuilding(CardEnum::House, "HouseClayLvl1"); // Buffer

		// Lvl 7 (8)
	for (int32 i = 1; i <= 2; i++) {
		FString moduleGroupName = "House_LV8_V" + FString::FromInt(i);
		FString moduleGroupFolderName = "House/Level8/V" + FString::FromInt(i);
		LoadBuilding(CardEnum::House, moduleGroupName, moduleGroupFolderName);
	}
	LinkBuilding(CardEnum::House, "HouseClayLvl1"); // Buffer
	LinkBuilding(CardEnum::House, "HouseClayLvl1"); // Buffer
	LinkBuilding(CardEnum::House, "HouseClayLvl1"); // Buffer
	
	
	//TryLoadBuildingModuleSet("HouseLvl2", "HouseLvl2");
	//TryLoadBuildingModuleSet("HouseLvl2V2", "HouseLvl2");
	//TryLoadBuildingModuleSet("HouseClayLvl2", "HouseClayLvl2");
	
	//TryLoadBuildingModuleSet("HouseLvl3", "HouseLvl3");
	//TryLoadBuildingModuleSet("HouseLvl3V2", "HouseLvl3");
	//TryLoadBuildingModuleSet("HouseClayLvl3", "HouseClayLvl3");

	//!!! June 18
	//TryLoadBuildingModuleSet("HouseLvl4", "HouseLvl4");
	//TryLoadBuildingModuleSet("HouseLvl4V2", "HouseLvl4");
	//
	//TryLoadBuildingModuleSet("HouseLvl5", "HouseLvl5");
	//TryLoadBuildingModuleSet("HouseLvl5V2", "HouseLvl5");
	//
	//TryLoadBuildingModuleSet("HouseLvl6", "HouseLvl6");
	//TryLoadBuildingModuleSet("HouseLvl6V2", "HouseLvl6");
	//
	//TryLoadBuildingModuleSet("HouseLvl7", "HouseLvl7");
	//TryLoadBuildingModuleSet("HouseLvl7V2", "HouseLvl7");
	
	// Construction
	//LoadModule("ConstructionBase", "BuildingModule1/ConstructionBase/ConstructionBase");
	LoadModule("ConstructionBaseHighlight", "BuildingModule1/ConstructionBase/ConstructionBase");
	LoadModule("ConstructionPoles", "BuildingModule1/ConstructionBase/ConstructionPoles");

	LoadModule("ConstructionPolesTopLeft", "BuildingModule1/ConstructionBase/ConstructionPolesTopLeft");
	LoadModule("ConstructionPolesTopRight", "BuildingModule1/ConstructionBase/ConstructionPolesTopRight");
	LoadModule("ConstructionPolesBottomLeft", "BuildingModule1/ConstructionBase/ConstructionPolesBottomLeft");
	LoadModule("ConstructionPolesBottomRight", "BuildingModule1/ConstructionBase/ConstructionPolesBottomRight");
	
	LoadModule("FisherConstructionPoles", "FisherModules/FisherConstructionPoles");
	LoadModule("FisherConstructionPolesWater", "FisherModules/FisherConstructionPolesWater");
	LoadModule("TradingPortConstructionPoles", "TradingPort/TradingPortConstructionPoles");
	LoadModule("TradingPortConstructionPolesWater", "TradingPort/TradingPortConstructionPolesWater");
	
	LoadModule("BuildingBaseBurnt", "BuildingModule1/ConstructionBase/BuildingBaseBurnt");

	// Townhall Modules
	TryLoadBuildingModuleSet("TownhallThatch", "TownhallThatch");
	TryLoadBuildingModuleSet("Townhall0", "TownhallModules");
	TryLoadBuildingModuleSet("Townhall3", "TownhallLvl3");
	TryLoadBuildingModuleSet("Townhall4", "TownhallLvl3");
	LoadModule("TownhallLvl5GardenAndSpire", "TownhallLvl5/TownhallLvl5GardenAndSpire");
	
	//PUN_CHECK(moduleMesh("Townhall3Special2"));


	//
	//TryLoadBuildingModuleSet("PaperMaker", "PaperMaker");

	TryLoadBuildingModuleSet("ConstructionOffice", "ConstructionOffice");
	TryLoadBuildingModuleSet("Ministry", "Ministry");

	TryLoadBuildingModuleSet("BeerBreweryFamous", "BeerBreweryFamous");

	// Ranch Barn Modules
	TryLoadBuildingModuleSet("RanchBarn", "RanchBarnModules/RanchBarn"); // TODO: This won't work...

	// Barrack Modules
	TryLoadBuildingModuleSet("Barrack", "Barrack");

	//TryLoadBuildingModuleSet("Bakery", "Bakery");
	TryLoadBuildingModuleSet("Windmill", "Windmill");

	//TryLoadBuildingModuleSet("Beekeeper", "Beekeeper");
	//TryLoadBuildingModuleSet("Brickworks", "Brickworks");

	TryLoadBuildingModuleSet("Colony", "Colony");
	TryLoadBuildingModuleSet("Outpost", "Outpost");
	TryLoadBuildingModuleSet("InventorsWorkshop", "InventorsWorkshop");


	TryLoadBuildingModuleSet("ChichenItza", "ChichenItza");

	//TryLoadBuildingModuleSet("Market", "Market");
	//TryLoadBuildingModuleSet("IrrigationReservoir", "IrrigationReservoir");
	

	TryLoadBuildingModuleSet("TribalVillage", "RegionTribalVillage");
	TryLoadBuildingModuleSet("AncientShrine", "RegionShrine");
	TryLoadBuildingModuleSet("PortVillage", "RegionPortVillage");
	TryLoadBuildingModuleSet("RegionCratePile", "RegionCratePile");


	// Fence Modules
	LoadModule("FenceFour", "Fence/FenceFour", true);
	LoadModule("FenceThree", "Fence/FenceThree", true);
	LoadModule("FenceAdjacent", "Fence/FenceAdjacent", true);
	LoadModule("FenceOpposite", "Fence/FenceOpposite", true);
	LoadModule("FenceGate", "FenceGate/FenceGate", true);

	// Bridge Modules
	LoadModule("Bridge1", "Bridge/Bridge1", true);
	LoadModule("Bridge1Plane", "Bridge/Bridge1Plane", true);
	LoadModule("Bridge1Plane90", "Bridge/Bridge1Plane90", true);
	LoadModule("Bridge2", "Bridge/Bridge2", true);
	LoadModule("Bridge2Plane", "Bridge/Bridge2Plane", true);
	LoadModule("Bridge2Plane90", "Bridge/Bridge2Plane90", true);
	LoadModule("Bridge4", "Bridge/Bridge4", true);
	LoadModule("Bridge4Plane", "Bridge/Bridge4Plane", true);
	LoadModule("Bridge4Plane90", "Bridge/Bridge4Plane90", true);
	LoadModule("Ramp", "Bridge/Ramp", true);
	LoadModule("RampPlane", "Bridge/RampPlane", true);
	LoadModule("RampPlane90", "Bridge/RampPlane90", true);

	// Tunnel
	LoadModule("Tunnel", "Tunnel/Tunnel");

	// Storage
	LoadModule("StorageTile", "StorageYard/StorageYardTile");
	

	// Trap
	LoadModule("TrapSpike", "TrapSpike/TrapSpike", true);


	// Shrines
	//LoadModule("ShrineRot", "ShrineRot/ShrineRot");
	//LoadModule("ShrineFrost", "ShrineFrost/ShrineFrost");
	LoadModule("HellPortal", "HellPortal/HellPortal");


	// Other Modules
	//LoadModule("WindowFrame", "BuildingModule1/WindowsFrame/WindowFrame");
	//LoadModule("WindowGlass", "BuildingModule1/WindowsFrame/WindowGlass");

	LoadModule("LovelyHeart", "EasterEggs/LovelyHeart");

	//TryLoadBuildingModuleSet("TownhallLvl5", "TownhallLvl5");

	//CheckMeshesAvailable();
	
	
	/*
	 * Building Icons
	 */
	auto addBuildIcon = [&](CardEnum cardEnum, FString iconName, FString iconAlphaName, bool isSpecial)
	{
		if (isSpecial) {
			_buildingsUsingSpecialIcon.Add(cardEnum);
		}
		_buildingIcons.Add(LoadF<UTexture2D>(FString("/Game/UI/GeneratedIcons/") + iconName));
		_buildingIconsAlpha.Add(LoadF<UTexture2D>(FString("/Game/UI/GeneratedIcons/") + iconAlphaName));
	};

	// TODO: Icons for all Cards...
	for (int i = 0; i < BuildingEnumCount; i++) 
	{
		// Special cases
		FString fileName = FString("BuildingIcon");
		FString fileNameAlpha = FString("BuildingIcon");

		CardEnum buildingEnum = static_cast<CardEnum>(i);
		switch(buildingEnum)
		{
#define CASE(enumName)	case CardEnum::##enumName: addBuildIcon(CardEnum::##enumName, FString(TO_STR(enumName##Icon)), FString("SpecialIconAlpha"), true); break;
			CASE(Farm);
			CASE(DirtRoad);
			CASE(StoneRoad);
			CASE(Fence);
			CASE(FenceGate);
			CASE(Bridge);
			CASE(Tunnel);
			CASE(StorageYard);

			case CardEnum::IntercityRoad: addBuildIcon(CardEnum::IntercityRoad, FString(TO_STR(DirtRoadIcon)), FString("SpecialIconAlpha"), true); break;
			case CardEnum::IntercityBridge: addBuildIcon(CardEnum::IntercityBridge, FString(TO_STR(BridgeIcon)), FString("SpecialIconAlpha"), true); break;
#undef CASE
		default:
			addBuildIcon(buildingEnum, FString("BuildingIcon") + FString::FromInt(i), FString("BuildingIconAlpha") + FString::FromInt(i), false);
			break;
		}
	}

	// Resource Icon
	for (int i = 0; i < ResourceEnumCount; i++) 
	{
		_resourceIcons.Add(LoadF<UTexture2D>(FString("/Game/UI/ResourceIcons/ResourceIcon") + FString::FromInt(i)));
		_resourceIconsAlpha.Add(LoadF<UTexture2D>(FString("/Game/UI/ResourceIcons/ResourceIconAlpha") + FString::FromInt(i)));
		_resourceIconMaterials.Add(nullptr);
	}

	// Georesource World Icons
	_georesourceIcons[ResourceEnum::Cannabis] = LoadF<UTexture2D>(FString("/Game/UI/Images/GeoresourceIcons/LogoWorldCannabis"));
	_georesourceIcons[ResourceEnum::Cocoa] = LoadF<UTexture2D>(FString("/Game/UI/Images/GeoresourceIcons/LogoWorldCocoa"));
	_georesourceIcons[ResourceEnum::RawCoffee] = LoadF<UTexture2D>(FString("/Game/UI/Images/GeoresourceIcons/LogoWorldCoffee"));
	
	_georesourceIcons[ResourceEnum::Cotton] = LoadF<UTexture2D>(FString("/Game/UI/Images/GeoresourceIcons/LogoWorldCotton"));
	_georesourceIcons[ResourceEnum::Dye] = LoadF<UTexture2D>(FString("/Game/UI/Images/GeoresourceIcons/LogoWorldDye"));
	_georesourceIcons[ResourceEnum::Grape] = LoadF<UTexture2D>(FString("/Game/UI/Images/GeoresourceIcons/LogoWorldGrape"));
	_georesourceIcons[ResourceEnum::Tulip] = LoadF<UTexture2D>(FString("/Game/UI/Images/GeoresourceIcons/LogoWorldTulip"));

	_georesourceIcons[ResourceEnum::Coal] = LoadF<UTexture2D>(FString("/Game/UI/Images/GeoresourceIcons/LogoWorldCoal"));
	_georesourceIcons[ResourceEnum::Gemstone] = LoadF<UTexture2D>(FString("/Game/UI/Images/GeoresourceIcons/LogoWorldGemstone"));

	// Biome Icons
	_biomeIcons[BiomeEnum::BorealForest] = LoadF<UTexture2D>(FString("/Game/UI/Components/Icons/BiomeBorealForest"));
	_biomeIcons[BiomeEnum::Desert] = LoadF<UTexture2D>(FString("/Game/UI/Components/Icons/BiomeDesert"));
	_biomeIcons[BiomeEnum::Forest] = LoadF<UTexture2D>(FString("/Game/UI/Components/Icons/BiomeForest"));
	_biomeIcons[BiomeEnum::GrassLand] = LoadF<UTexture2D>(FString("/Game/UI/Components/Icons/BiomeGrassland"));
	_biomeIcons[BiomeEnum::Jungle] = LoadF<UTexture2D>(FString("/Game/UI/Components/Icons/BiomeJungle"));
	_biomeIcons[BiomeEnum::Savanna] = LoadF<UTexture2D>(FString("/Game/UI/Components/Icons/BiomeSavana"));
	_biomeIcons[BiomeEnum::Tundra] = LoadF<UTexture2D>(FString("/Game/UI/Components/Icons/BiomeTundra"));
	
	/*
	 * Card Icons
	 */
	int32 addCardIconCount = 0;
	auto addCardIcon = [&](CardEnum cardEnum, FString iconFileName) {
		_cardIcons.Add(static_cast<int32>(cardEnum), LoadF<UTexture2D>(FString("/Game/UI/Images/CardImages_BleGood/") + iconFileName + FString("_1024")));
		addCardIconCount++;
	};
	addCardIcon(CardEnum::None, "CardNone");

	
	addCardIcon(CardEnum::Agriculturalist, "Agriculturalist");
	addCardIcon(CardEnum::AllYouCanEat, "AllYouCanEat");
	addCardIcon(CardEnum::AlcoholAppreciation, "Appreciation");
	addCardIcon(CardEnum::BeerTax, "BeerTax");
	addCardIcon(CardEnum::BirthControl, "BirthControl");
	addCardIcon(CardEnum::BlingBling, "BlingBling");
	addCardIcon(CardEnum::BlueberrySeed, "Blueberry");
	addCardIcon(CardEnum::BookWorm, "BookWorm");
	addCardIcon(CardEnum::BuyWood, "BuyWood");
	addCardIcon(CardEnum::CabbageSeed, "CabbageSeeds");
	addCardIcon(CardEnum::CannabisSeeds, "Cannabis");
	addCardIcon(CardEnum::Cannibalism, "Cannibalism");
	addCardIcon(CardEnum::Capitalism, "Capitalism");
	addCardIcon(CardEnum::ChimneyRestrictor, "ChimneyRestrictor");
	addCardIcon(CardEnum::CoalPipeline, "CoalPipeline");
	addCardIcon(CardEnum::CoalTreatment, "CoalTreatment");
	addCardIcon(CardEnum::CocoaSeeds, "Cocoa");
	addCardIcon(CardEnum::CoffeeSeeds, "Coffee");
	addCardIcon(CardEnum::Communism, "Communism");
	addCardIcon(CardEnum::CooperativeFishing, "CooperativeFishing");
	addCardIcon(CardEnum::CottonSeeds, "Cotton");
	addCardIcon(CardEnum::Craftmanship, "Craftmanship");
	addCardIcon(CardEnum::DesertIndustry, "DesertIndustry");
	addCardIcon(CardEnum::DyeSeeds, "Dye");
	addCardIcon(CardEnum::FarmWaterManagement, "FarmWaterManagement");
	addCardIcon(CardEnum::BorealWinterFishing, "Fish");
	addCardIcon(CardEnum::FreeThoughts, "FreeThoughts");
	addCardIcon(CardEnum::FrugalityBook, "Frugality");
	addCardIcon(CardEnum::DesertGem, "Gem");
	addCardIcon(CardEnum::BorealGoldOil, "GoldOil");
	addCardIcon(CardEnum::GrapeSeeds, "Grape");
	addCardIcon(CardEnum::Geologist, "Geologist");
	addCardIcon(CardEnum::SavannaRanch, "GrasslandHerding");
	addCardIcon(CardEnum::SavannaHunt, "GrasslandHunting");
	addCardIcon(CardEnum::HappyBreadDay, "HappyBreadDay");
	addCardIcon(CardEnum::HomeBrew, "HomeBrew");
	addCardIcon(CardEnum::Immigration, "Immigration");
	addCardIcon(CardEnum::ForestCharcoal, "ImprovedCharcoalMaking");
	addCardIcon(CardEnum::ForestFarm, "ImprovedFarming");
	addCardIcon(CardEnum::Investment, "Investment");
	addCardIcon(CardEnum::JungleGatherer, "JungleGatherer");
	addCardIcon(CardEnum::Kidnap, "Kidnap");
	addCardIcon(CardEnum::Lockdown, "Lockdown");
	addCardIcon(CardEnum::MasterBrewer, "MasterBrewer");
	addCardIcon(CardEnum::MasterPotter, "MasterPotter");
	addCardIcon(CardEnum::Motivation, "Motivation");
	addCardIcon(CardEnum::HerbSeed, "MedicinalHerbFarming");
	addCardIcon(CardEnum::MelonSeed, "Melon");
	addCardIcon(CardEnum::JungleMushroom, "Mushroom");
	addCardIcon(CardEnum::DesertOreTrade, "OreTrade");
	addCardIcon(CardEnum::Passion, "Passion");
	addCardIcon(CardEnum::BorealPineForesting, "PineLumber");
	addCardIcon(CardEnum::PopulationScoreMultiplier, "PopulationScore");
	addCardIcon(CardEnum::PotatoSeed, "Potato");
	addCardIcon(CardEnum::ProductivityBook, "Productivity");
	addCardIcon(CardEnum::Protectionism, "Protectionism");
	addCardIcon(CardEnum::PumpkinSeed, "Pumpkin");
	addCardIcon(CardEnum::Rationalism, "Rationalism");
	addCardIcon(CardEnum::Romanticism, "Romanticism");
	addCardIcon(CardEnum::SellFood, "SellFood");
	addCardIcon(CardEnum::SlaveLabor, "SlaveLabor");
	addCardIcon(CardEnum::SocialWelfare, "SocialWelfare");
	addCardIcon(CardEnum::SustainabilityBook, "Sustainability");
	addCardIcon(CardEnum::TulipSeeds, "Tulip");
	addCardIcon(CardEnum::DesertTradeForALiving, "TradeForALiving");
	addCardIcon(CardEnum::Demolish, "Demolish");
	
	addCardIcon(CardEnum::WildCard, "WildCard");
	addCardIcon(CardEnum::WildCardFood, "WildCard");
	addCardIcon(CardEnum::WildCardIndustry, "WildCard");
	addCardIcon(CardEnum::WildCardMine, "WildCard");
	addCardIcon(CardEnum::WildCardService, "WildCard");

	addCardIcon(CardEnum::WheatSeed, "WheatSeeds");
	addCardIcon(CardEnum::BorealWinterResist, "WinterResistance");
	addCardIcon(CardEnum::WondersScoreMultiplier, "WondersScore");

	addCardIcon(CardEnum::DesertPilgrim, "DesertPilgrim");
	addCardIcon(CardEnum::MiningEquipment, "MiningEquipment");

	addCardIcon(CardEnum::ForestTools, "ForestImprovedToolmaking");
	addCardIcon(CardEnum::ForestBeer, "ForestImprovedBeerBrewing");

	addCardIcon(CardEnum::JungleTree, "JungleForestry");
	addCardIcon(CardEnum::JungleHerbFarm, "JungleHerb");

	addCardIcon(CardEnum::Snatch, "StealCard");
	addCardIcon(CardEnum::Steal, "SnatchCard");

	

	// Add Building Card Icons
	addCardIconCount = 0;
	
	for (CardEnum buildingEnum : SortedNameBuildingEnum) {
		FString name = GetBuildingInfo(buildingEnum).nameF().Replace(TEXT(" "), TEXT(""));
		name = name.Replace(TEXT("'"), TEXT(""));

		FString path = FString("UI/BuildingSnapshots/") + name;

		IPlatformFile& platformFile = FPlatformFileManager::Get().GetPlatformFile();
		if (platformFile.FileExists(*(FPaths::ProjectContentDir() + path + FString(".uasset"))))
		{
			UObject* cardIconTextureObj = StaticLoadObject(UTexture2D::StaticClass(), NULL, *(FString("/Game/") + path));
			UTexture2D* cardIconTexture = Cast<UTexture2D>(cardIconTextureObj);

			if (cardIconTexture) {
				_cardIcons.Add(static_cast<int32>(buildingEnum), cardIconTexture);
				addCardIconCount++;
			}
		}
	}

	check(addCardIconCount == 70);

	//BorealFishing
	//BorealWinterResistant

	// Army Icons
	auto addArmyIcon = [&](std::string iconName) {
		_armyIcons.Add(LoadF<UTexture2D>(FString("/Game/UI/MiscIcons/") + ToFString(iconName)));
	};
	addArmyIcon("TowerPNG");
	addArmyIcon("ClubPNG");
	addArmyIcon("SwordPNG");
	addArmyIcon("BowPNG");
	
	/**
	 * Unit
	 */
	_unitEnumToAsset.resize(UnitEnumCount);
	_unitDisplayEnumToAsset.resize(UnitDisplayEnumCount);
	
	//LoadUnit(UnitEnum::Human, "Human/Man/Man1");
	//LoadUnit(UnitEnum::Human, "Human/Man/Boy1");
	
	LoadUnit(UnitEnum::Alpaca, "Alpaca/Alpaca");
	
	LoadUnit(UnitEnum::Boar, "Boar/Boar_Game");
	LoadUnit(UnitEnum::Boar, "Boar/BoarMini_Game");
	
	LoadUnit(UnitEnum::RedDeer, "Deer/Stag_Game");
	LoadUnit(UnitEnum::YellowDeer, "Deer/StagYellow_Game");
	LoadUnit(UnitEnum::DarkDeer, "Deer/StagDark_Game");

	LoadUnit(UnitEnum::BlackBear, "Bear/BlackBear_Game");
	LoadUnit(UnitEnum::BrownBear, "Bear/BrownBear_Game");
	LoadUnit(UnitEnum::Panda, "Panda/Panda_Game");

	// Proper way to do skel collision
	LoadUnit(UnitEnum::Hippo, "Human/Man/Man1");
	LoadUnit(UnitEnum::Penguin, "Human/Man/Man1");

	LoadUnit(UnitEnum::Pig, "Pig/StorePig");
	LoadUnit(UnitEnum::Sheep, "Sheep/Sheep_Game");
	LoadUnit(UnitEnum::Cow, "Cow/Cow_Game");

	LoadUnit(UnitEnum::Infantry, "Infantry/Infantry");
	LoadUnit(UnitEnum::ProjectileArrow, "Projectiles/Arrow/Arrow");

	LoadUnit(UnitEnum::SmallShip, "Ship/SmallShip");


	/*
	 * Unit Full (SkeletalMesh)
	 */
	
	std::unordered_map<UnitAnimationEnum, std::string> animationFileNames = {
		{ UnitAnimationEnum::Walk, "MOB1_Walk_F_IPC" },
		{ UnitAnimationEnum::Build, "Builder_1" },
		{ UnitAnimationEnum::ChopWood, "Choping_wood" },
		{ UnitAnimationEnum::StoneMining, "Stone_mining" },
		{ UnitAnimationEnum::FarmPlanting, "Rake_cleaning_1" },
		{ UnitAnimationEnum::Wait, "Waiting_2" },
	};

	// Adult Male
	LoadUnitFull(UnitEnum::Human, "Human/CitizenMale/", "CitizenMale", animationFileNames, "Human/CitizenMale/VertexAnim/CitizenMale_VertexAnim3"); // "Human/CitizenMale/CitizenMaleStatic"
	LoadUnitAuxMesh(UnitAnimationEnum::ImmigrationCart, "Human/Cart/Cart");
	LoadUnitAuxMesh(UnitAnimationEnum::HaulingCart, "Human/Cart/FrontCart");

	// Adult Female
	LoadUnitFull(UnitEnum::Human, "Human/CitizenFemale/", "CitizenFemale", animationFileNames, "Human/CitizenFemale/VertexAnim/CitizenFemale_VertexAnim"); // "Human/CitizenFemale/CitizenFemaleStatic"

	// Child Male
	LoadUnitFull(UnitEnum::Human, "Human/CitizenChildMale/", "CitizenChildMale", animationFileNames, "Human/CitizenChildMale/VertexAnim/CitizenChildMale_VertexAnim");

	// Child Female
	LoadUnitFull(UnitEnum::Human, "Human/CitizenChildFemale/", "CitizenChildFemale", animationFileNames, "Human/CitizenChildFemale/VertexAnim/CitizenChildFemale_VertexAnim");


	// Wild Man
	LoadUnitFull(UnitEnum::WildMan, "Human/WildMan/", "WildMan", animationFileNames, "Human/WildMan/WildManStatic");

	// Hippo
	LoadUnitFull(UnitEnum::Hippo, "Hippo/", "HippoSmall", {
		{ UnitAnimationEnum::Walk, "Hippo_Walk"},
		{ UnitAnimationEnum::Wait, "Hippo_LookAround"},
	}, "Horse/HorseSmallStatic");
	
	// Penguin
	LoadUnitFull(UnitEnum::Penguin, "Penguin/", "Penguin", {
		{ UnitAnimationEnum::Wait, "Penguin_Wait"},
	}, "Horse/HorseSmallStatic");

	
	// Horse Caravan
	LoadUnitFull(UnitEnum::HorseCaravan, "Horse/", "HorseSmall", {
		{ UnitAnimationEnum::Walk, "Anim_Horse_Trot_F_IP"},
	}, "Horse/HorseSmallStatic");
	LoadUnitAuxMesh(UnitAnimationEnum::HorseCaravan, "Horse/CaravanWagon_Game");

	// Horse Market
	LoadUnitFull(UnitEnum::HorseMarket, "Horse/", "HorseSmall", {
		{ UnitAnimationEnum::Walk, "Anim_Horse_Trot_F_IP"},
	}, "Horse/HorseSmallStatic");
	LoadUnitAuxMesh(UnitAnimationEnum::HorseMarket, "Horse/MarketWagon");

	// Horse Logistics
	LoadUnitFull(UnitEnum::HorseLogistics, "Horse/", "HorseSmall", {
		{ UnitAnimationEnum::Walk, "Anim_Horse_Trot_F_IP"},
	}, "Horse/HorseSmallStatic");
	LoadUnitAuxMesh(UnitAnimationEnum::HorseLogistics, "Horse/LogisticsWagon");
	
	
	LoadUnitWeapon(UnitAnimationEnum::Build, "Human/CitizenMale/CitizenMaleHammer");
	LoadUnitWeapon(UnitAnimationEnum::ChopWood, "Human/CitizenMale/CitizenMaleAxe");
	LoadUnitWeapon(UnitAnimationEnum::StoneMining, "Human/CitizenMale/CitizenMalePickAxe");
	LoadUnitWeapon(UnitAnimationEnum::FarmPlanting, "Human/CitizenMale/CitizenMaleFarmTool");
	
	/**
	 * Resource
	 */

	//LoadResource(ResourceEnum::Berries, "Berries/ResourceBerries", "Berries/ResourceBerriesMaterial");
	// TODO: Put this whole thing in resourceInfo??? so it is easier to add resource in only single place...
	LoadResource2(ResourceEnum::Orange, "Orange/Orange");
	LoadResource2(ResourceEnum::Papaya, "Papaya/Papaya");
	LoadResource2(ResourceEnum::Coconut, "Coconut/Coconut");
	
	LoadResource2(ResourceEnum::Wheat, "Wheat/Wheat");
	LoadResource2(ResourceEnum::Milk, "Milk/Milk");
	LoadResource2(ResourceEnum::Mushroom, "Mushroom/Mushroom");
	LoadResource2(ResourceEnum::Hay, "Hay/Hay");

	LoadResource2(ResourceEnum::Wood, "Wood/Wood");
	LoadResource2(ResourceEnum::Stone, "Stone/Stone");

	LoadResource2(ResourceEnum::Paper, "Paper/Paper");
	LoadResource2(ResourceEnum::Clay, "Clay/Clay");
	LoadResource2(ResourceEnum::Brick, "Brick/Brick");

	LoadResource2(ResourceEnum::Coal, "Coal/Coal");
	LoadResource2(ResourceEnum::IronOre, "IronOre/IronOre");
	LoadResource2(ResourceEnum::Iron, "Iron/Iron");
	LoadResource2(ResourceEnum::Furniture, "Furniture/Furniture");
	LoadResource2(ResourceEnum::Chocolate, "Chocolate/Chocolate");

	//LoadResource2(ResourceEnum::StoneTools, "StoneTools/StoneTools");
	//LoadResource2(ResourceEnum::CrudeIronTools, "CrudeIronTools/CrudeIronTools");
	LoadResource2(ResourceEnum::SteelTools, "SteelTools/SteelTools");
	LoadResource2(ResourceEnum::Herb, "Herb/Herb");
	LoadResource2(ResourceEnum::Medicine, "Medicine/Medicine");

	//LoadResource(ResourceEnum::Tools, "Tools/ResourceTools");
	LoadResource2(ResourceEnum::Fish, "Fish/Fish");

	//LoadResource(ResourceEnum::WhaleMeat, "WhaleMeat/ResourceWhaleMeat");
	LoadResource2(ResourceEnum::Grape, "Grape/Grape");
	LoadResource2(ResourceEnum::Wine, "Wine/Wine");
	LoadResource2(ResourceEnum::MagicMushroom, "Shroom/Shroom");
	LoadResource2(ResourceEnum::Vodka, "Vodka/Vodka");

	LoadResource2(ResourceEnum::Pork, "Meat/Meat");
	LoadResource2(ResourceEnum::Beef, "Meat/Meat");
	LoadResource2(ResourceEnum::GameMeat, "Meat/Meat");
	LoadResource2(ResourceEnum::Lamb, "Meat/Meat");
	
	LoadResource2(ResourceEnum::Cocoa, "Cocoa/Cocoa");

	LoadResource2(ResourceEnum::Wool, "Wool/Wool");
	LoadResource2(ResourceEnum::Leather, "Leather/AnimalSkin");
	LoadResource2(ResourceEnum::Cloth, "Cloth/Cloth");

	LoadResource2(ResourceEnum::GoldOre, "GoldOre/GoldOre");
	LoadResource2(ResourceEnum::GoldBar, "Gold/Gold");

	LoadResource2(ResourceEnum::Beer, "Beer/Beer");
	LoadResource2(ResourceEnum::Cannabis, "Cannabis/PotWeed");
	LoadResource2(ResourceEnum::Cabbage, "Cabbage/Cabbage");

	LoadResource2(ResourceEnum::Pottery, "Pottery/Pottery");

	LoadResource2(ResourceEnum::Flour, "Flour/Flour");
	LoadResource2(ResourceEnum::Bread, "Bread/BreadBasket");
	LoadResource2(ResourceEnum::Gemstone, "Gemstones/GemstoneBasket");
	LoadResource2(ResourceEnum::Jewelry, "Jewelry/JewelryChest");

	
	LoadResource2(ResourceEnum::Cotton, "Cotton/CottonBasket");
	LoadResource2(ResourceEnum::CottonFabric, "CottonFabric/CottonFabric");
	LoadResource2(ResourceEnum::DyedCottonFabric, "CottonFabric/DyedCottonFabric");
	LoadResource2(ResourceEnum::LuxuriousClothes, "Cloth/LuxuriousCloth");

	LoadResource2(ResourceEnum::Honey, "Honey/Honey");
	LoadResource2(ResourceEnum::Beeswax, "Beeswax/Beeswax");
	LoadResource2(ResourceEnum::Candle, "Candle/Candle");

	LoadResource2(ResourceEnum::Dye, "Dye/Dye");
	LoadResource2(ResourceEnum::Book, "Book/Book");

	// Dec 2020
	LoadResource2(ResourceEnum::Blueberries, "Blueberries/Blueberries");
	LoadResource2(ResourceEnum::Melon, "Melon/Melon");
	LoadResource2(ResourceEnum::Potato, "Potato/Potato");
	LoadResource2(ResourceEnum::Pumpkin, "Pumpkin/Pumpkin");

	LoadResource2(ResourceEnum::RawCoffee, "CoffeeFruit/CoffeeFruit");
	LoadResource2(ResourceEnum::Coffee, "Coffee/Coffee");
	LoadResource2(ResourceEnum::Tulip, "Tulip/Tulip");

	// Apr 9
	LoadResource2(ResourceEnum::StoneTools, "StoneTools/StoneTools");
	LoadResource2(ResourceEnum::Sand, "Sand/Sand");
	LoadResource2(ResourceEnum::Oil, "OilBarrel/Oil");

	LoadResource2(ResourceEnum::Glass, "Glass/Glass");
	LoadResource2(ResourceEnum::Concrete, "Concrete/Concrete");
	LoadResource2(ResourceEnum::SteelBeam, "SteelBeam/SteelBeam");
	LoadResource2(ResourceEnum::Glassware, "Glassware/Glassware");
	LoadResource2(ResourceEnum::PocketWatch, "PocketWatch/Pocketwatch");
	
	//LoadResource2(ResourceEnum::Oyster, "Pottery/Pottery");
	//LoadResource2(ResourceEnum::Truffle, "Pottery/Pottery");
	//LoadResource2(ResourceEnum::Coconut, "Pottery/Pottery");
	//LoadResource2(ResourceEnum::Diamond, "Pottery/Pottery");
	//LoadResource2(ResourceEnum::Jewelry, "Pottery/Pottery");

	/**
	 * Georesources
	 */

	LoadGeoresource(GeoresourceEnum::Hotspring, "Hotspring", "Hotspring", 2);
	//LoadGeoresource(GeoresourceEnum::GiantMushroom, "GiantMushroom", "GiantMushroom", 2);
	//LoadGeoresource(GeoresourceEnum::GiantTree, "GiantTree", "GiantTree", 2);
	LoadGeoresource(GeoresourceEnum::Ruin, "Ruin", "Ruin", 1);

	LoadGeoresource(GeoresourceEnum::IronOre, "IronDeposit", "IronOre", 2);
	LoadGeoresource(GeoresourceEnum::CoalOre, "CoalDeposit", "CoalOre", 2);
	LoadGeoresource(GeoresourceEnum::GoldOre, "GoldDeposit", "GoldOre", 2);
	LoadGeoresource(GeoresourceEnum::Gemstone, "GemstoneDeposit", "Gemstone", 2);

	/**
	 * Trees
	 */
	std::string defaultFruit = "TreeJune9/TreeOrangeFruit";
	LoadTree(TileObjEnum::Orange,	"OrangeTrunk",		"OrangeTree/TreeLeaf",		"TreeJune9/TreeOrangeFruit", "OrangeTree/TreeLeaf_lo", "OrangeTree/TreeLeafShadow", "OrangeStump");
	LoadTree(TileObjEnum::Birch,	"BirchTrunk",		"OrangeTree/TreeLeaf_Round",			"TreeJune9/TreeOrangeFruit", "OrangeTree/TreeLeaf_Round_lo", "OrangeTree/TreeLeafShadow", "BirchStump");

	LoadTree(TileObjEnum::Apple,	"OrangeTrunk",		"TreeJune9/TreeJune9Leaves2",			"TreeJune9/TreeOrangeFruit", "OrangeTree/TreeLeaf_lo", "OrangeTree/TreeLeafShadow", "OrangeStump");
	LoadTree(TileObjEnum::Papaya,	"PapayaTree/PapayaTree",	"PapayaTree/PapayaLeaf",			"PapayaTree/PapayaFruits", "OrangeTree/TreeLeaf_Papaya_lo", "OrangeTree/TreeLeaf_Papaya_lo", "PapayaTree/PapayaStump");
	LoadTree(TileObjEnum::Durian,	"OrangeTrunk",		"JungleTree3/JungleTree3",			"TreeJune9/TreeOrangeFruit", "OrangeTree/TreeLeaf_lo", "OrangeTree/TreeLeafShadow", "OrangeStump");
	LoadTree(TileObjEnum::Pine1,	"Pine1/PineTrunk",	"Pine1/PineLeaf2",					"TreeJune9/TreeOrangeFruit", "Pine1/PineLeaf1_lo", "Pine1/PineLeaf1_lo", "Pine1/PineStump");
	LoadTree(TileObjEnum::Pine2,	"Pine1/PineTrunk",	"Pine1/PineLeaf2",					"TreeJune9/TreeOrangeFruit", "Pine1/PineLeaf2_lo", "Pine1/PineLeaf2_lo", "Pine1/PineStump");
	LoadTree(TileObjEnum::GiantMushroom, "GiantMushroom/GiantMushroom", "GiantMushroom/GiantMushroom", defaultFruit, "BirchSeed", "OrangeTree/TreeLeafShadow", "OrangeStump");

	LoadTree(TileObjEnum::Cherry,	"OrangeTrunk",		"CherryTree/CherryTreeLeaf",			"TreeJune9/TreeOrangeFruit", "OrangeTree/TreeLeaf_lo", "OrangeTree/TreeLeafShadow", "OrangeStump");
	LoadTree(TileObjEnum::Coconut,	"Coconut/Coconut1/CoconutTrunk", "Coconut/Coconut1/CoconutLeaf", "Coconut/Coconut1/CoconutFruits", "Coconut/Coconut1/CoconutLeaf_lo", "Coconut/Coconut1/CoconutLeaf_lo", "Coconut/Coconut1/CoconutStump");
	LoadTree(TileObjEnum::Cyathea,	"Fern/Cyathea/CyatheaTrunk", "Fern/Cyathea/CyatheaLeaf",				defaultFruit, "Fern/Cyathea/CyatheaLeaf_lo", "Fern/Cyathea/CyatheaLeaf_lo", "Coconut/Coconut1/CoconutStump");
	LoadTree(TileObjEnum::ZamiaDrosi, "ZamiaDrosi/ZamiaDrosiTrunk", "ZamiaDrosi/ZamiaDrosiLeaf",			defaultFruit, "ZamiaDrosi/ZamiaDrosiLeaf_lo", "ZamiaDrosi/ZamiaDrosiLeaf_shadow", "ZamiaDrosi/ZamiaDrosiStump");

	LoadTree(TileObjEnum::SavannaTree1, "SavannaTree1/SavannaTree1Trunk", "SavannaTree1/SavannaTree1Leaf", defaultFruit, "OrangeTree/TreeLeaf_lo", "SavannaTree1/SavannaTree1Leaf", "SavannaTree1/SavannaTree1Stump");
	LoadTree(TileObjEnum::Cactus1, "BirchSeed", "Cactus1/Cactus1", defaultFruit, "Cactus1/Cactus1", "Cactus1/Cactus1", "Cactus1/Cactus1");
	

	
	LoadTileObject(TileObjEnum::Stone, { "Trees/StoneOutcrop/StoneOutcropMegascan1", 
										"Trees/StoneOutcrop/StoneOutcropMegascan2",
										"Trees/StoneOutcrop/StoneOutcropMegascan3",
										"Trees/StoneOutcrop/StoneOutcropMegascan4" });

	LoadTileObject(TileObjEnum::CoalMountainOre, { "Trees/MountainOre/CoalOre" });
	LoadTileObject(TileObjEnum::IronMountainOre, { "Trees/MountainOre/IronOre" });
	LoadTileObject(TileObjEnum::GoldMountainOre, { "Trees/MountainOre/GoldOre" });
	LoadTileObject(TileObjEnum::GemstoneOre, { "Trees/MountainOre/GemstoneTileObj" });

	LoadTileObject(TileObjEnum::GrassGreen, { 
		//"Trees/GrassWild/GrassWildFin",
		"Plant/Wheat/PlantGrassGreen",
	});

	/*
	 * Plants
	 */
	LoadTileObject(TileObjEnum::OreganoBush, {
		"Trees/OreganoBush/OreganoBush1",
	});
	LoadTileObject(TileObjEnum::CommonBush, {
		"Trees/CommonBush1/CommonBush1",
	});
	LoadTileObject(TileObjEnum::CommonBush2, {
		"Trees/CommonBush1/CommonBush2",
	});
	
	LoadTileObject(TileObjEnum::Fern, {
		"Trees/Fern/Var1/Var1_LOD3",
		//"Trees/Fern/Var2/Var2_LOD3",
	});
	LoadTileObject(TileObjEnum::SavannaGrass, {
		"Plant/Wheat/PlantGrassGreen",
	});
	LoadTileObject(TileObjEnum::JungleThickLeaf, {
		"Trees/JunglePlant1/Var1/Var1_LOD4",
		//"Trees/JunglePlant1/Var2/Var2_LOD4",
	});

	
	LoadTileObject(TileObjEnum::BlueFlowerBush, {
		"Trees/FlowerWhite/FlowerWhite",
	});
	LoadTileObject(TileObjEnum::WhiteFlowerBush, {
		"Trees/FlowerWhite/FlowerWhite",
	});
	LoadTileObject(TileObjEnum::RedPinkFlowerBush, {
		"Trees/FlowerRedPink/FlowerRedPink",
	});

	//LoadTileObject(TileObjEnum::FieldFlowerPurple, {
	//	"Trees/FieldFlower/FieldFlower",
	//	"Trees/FieldFlower/FieldFlowerTrunk",
	//});
	LoadTileObject(TileObjEnum::FieldFlowerYellow, {
		"Trees/FieldFlower/FieldFlowerYellow",
		"Trees/FieldFlower/FieldFlowerTrunk",
	});
	LoadTileObject(TileObjEnum::FieldFlowerHeart, {
		"Trees/FieldFlower/FieldFlowerHeart",
		"Trees/FieldFlower/FieldFlowerTrunk",
	});
	LoadTileObject(TileObjEnum::FieldFlowerPic, {
		"Trees/FieldFlower/FieldFlowerPic",
		"Trees/FieldFlower/FieldFlowerTrunk",
	});


	LoadTileObject(TileObjEnum::WheatBush, {
		"Plant/Wheat/PlantWheat",
	});
	LoadTileObject(TileObjEnum::BarleyBush, {
		"Plant/Wheat/PlantWheat",
	});
	LoadTileObject(TileObjEnum::Cannabis, {
		"Trees/Cannabis/Cannabis",
	});
	LoadTileObject(TileObjEnum::Grapevines, {
		//"Plant/Grape/PlantGrape",
		"Trees/Grapevines/Grapeframes",
		"Trees/Grapevines/Grapevines",
		"Trees/Grapevines/Grapes",
	});
	LoadTileObject(TileObjEnum::PricklyPearCactus, {
		"Trees/Grapevines/Grapeframes",
	});

	//LoadTileObject(TileObjEnum::PlumpCob, {
	//	"Trees/Grapevines/Grapeframes",
	//});
	//LoadTileObject(TileObjEnum::CreamPod, {
	//	"Trees/Grapevines/Grapeframes",
	//});
	LoadTileObject(TileObjEnum::Cabbage, {
		"Trees/Cabbage/Cabbage",
	});
	LoadTileObject(TileObjEnum::Cocoa, {
		"Trees/Cocoa/CocoaPlant",
	});

	LoadTileObject(TileObjEnum::Cotton, {
		"Plant/Cotton/CottonPlant",
	});
	LoadTileObject(TileObjEnum::Dye, {
		"Plant/Dye/DyePlant",
	});
	
	LoadTileObject(TileObjEnum::Herb, {
		"Trees/HerbPlant/HerbPlant",
	});

	// December 2020 new Crops
	LoadTileObject(TileObjEnum::Potato, {
		"Trees/Potato/PotatoPlant",
	});
	LoadTileObject(TileObjEnum::Pumpkin, {
		"Trees/Pumpkin/PumpkinPlant",
		"Trees/Pumpkin/PumpkinPlantFruit",
	});
	LoadTileObject(TileObjEnum::RawCoffee, {
		"Trees/Coffee/CoffeePlantFruit",
		"Trees/Coffee/CoffeeTrunk",
	});
	LoadTileObject(TileObjEnum::Tulip, {
		"Trees/Tulip/TulipPlant",
	});
	LoadTileObject(TileObjEnum::Blueberry, {
		"Trees/Blueberry/BlueberryPlant",
		"Trees/Blueberry/BlueberryPlantFruit",
	});
	LoadTileObject(TileObjEnum::Melon, {
		"Trees/Melon/MelonPlant",
		"Trees/Melon/MelonPlantFruit",
	});

	/**
	 * Placement
	 */

	PlacementMesh = Load<UStaticMesh>("/Game/Models/Placement/placementMesh");
	PlacementMaterialGreen = Load<UMaterial>("/Game/Models/Placement/placementMaterialGreen");
	PlacementMaterialRed = Load<UMaterial>("/Game/Models/Placement/placementMaterialRed");
	PlacementMaterialGray = Load<UMaterial>("/Game/Models/Placement/placementMaterialGray");
	PlacementArrowMaterialGreen = Load<UMaterial>("/Game/Models/Placement/placementArrowMaterialGreen");
	PlacementArrowMaterialYellow = Load<UMaterial>("/Game/Models/Placement/placementArrowMaterialYellow");
	PlacementArrowMaterialRed = Load<UMaterial>("/Game/Models/Placement/placementArrowMaterialRed");

	DeliveryArrowMesh = Load<UStaticMesh>("/Game/Models/Placement/DeliveryArrow");
	M_DeliveryArrow = Load<UMaterial>("/Game/Models/Placement/M_DeliveryArrow");

	/**
	 * Other Materials
	 */

	TerritoryMaterial = Load<UMaterial>("/Game/Models/Decals/TerritoryMaterial");
	
	M_Territory = Load<UMaterial>("/Game/Models/Decals/M_Territory");
	M_Territory_Top = Load<UMaterial>("/Game/Models/Decals/M_Territory_Top");
	M_Province = Load<UMaterial>("/Game/Models/Decals/M_Province");
	M_Province_Top = Load<UMaterial>("/Game/Models/Decals/M_Province_Top");
	
	RegionBorderMaterial = Load<UMaterial>("/Game/Models/Decals/RegionBorderMaterial");
	OverlayMaterial = Load<UMaterial>("/Game/Models/Decals/OverlayMaterial");
	GridGuideMaterial = Load<UMaterial>("/Game/Models/Decals/GridGuideMaterial");
	M_GridGuideLine = Load<UMaterial>("/Game/Models/Decals/M_GridGuideLine");

	RadiusMesh = Load<UStaticMesh>("/Game/Models/SelectionMesh/RadiusMesh");
	M_Radius = Load<UMaterial>("/Game/Models/SelectionMesh/M_Radius");
	MI_RadiusRed = Load<UMaterialInstance>("/Game/Models/SelectionMesh/MI_RadiusRed");
	
	RadiusMaterial = Load<UMaterial>("/Game/Models/Decals/RadiusMaterial");
	MI_RedRadius = Load<UMaterialInstance>("/Game/Models/Decals/MI_RedRadius");

	M_GoldOreDecal = Load<UMaterial>("/Game/Models/Decals/M_GoldOreDecal");
	M_IronOreDecal = Load<UMaterial>("/Game/Models/Decals/M_IronOreDecal");
	M_CoalOreDecal = Load<UMaterial>("/Game/Models/Decals/M_CoalOreDecal");
	M_GemstoneDecal = Load<UMaterial>("/Game/Models/Decals/M_GemstoneDecal");
	
	FarmDecalMaterial = Load<UMaterial>("/Game/Models/Buildings/Farm/M_Farm");
	ConstructionBaseDecalMaterial = Load<UMaterial>("/Game/Models/Buildings/BuildingModule1/ConstructionBase/M_BuildingBase");
	
	WetnessMaterial = Load<UMaterial>("/Game/Models/Decals/WetnessMaterial");

	M_PlainMaterial = Load<UMaterial>("/Game/Models/Materials/M_PlainMaterial");
	
	M_WoodFrameBurned = Load<UMaterial>("/Game/Models/Buildings/BuildingModule1/WoodFrame/M_WoodFrameBurned");
	
	M_TileHighlightDecal = Load<UMaterial>("/Game/Models/Decals/M_TileHighlightDecal");
	M_TileHighlightForMesh = Load<UMaterial>("/Game/Models/Decals/M_TileHighlightForMesh");
	
	M_ConstructionHighlightDecal = Load<UMaterial>("/Game/Models/Decals/M_ConstructionHighlightDecal");

	M_TerritoryHighlightForMesh = Load<UMaterial>("/Game/Models/Decals/M_TerritoryHighlightForMesh");
	M_TerritoryHighlightForMeshFaded = Load<UMaterial>("/Game/Models/Decals/M_TerritoryHighlightForMeshFaded");
	M_TerritoryMapDecal = Load<UMaterial>("/Game/Models/Decals/M_TerritoryMapDecal");
	
	M_RegionHighlightDecal = Load<UMaterial>("/Game/Models/Decals/M_RegionHighlightDecal");
	M_RegionHighlightDecalFaded = Load<UMaterial>("/Game/Models/Decals/M_RegionHighlightDecalFaded");
	
	HighlightMaterial = Load<UMaterialInstance>("/Game/Models/Decals/M_HighlightDecal");

	RoadMaterial = Load<UMaterial>("/Game/Models/Ground/Road/RoadMaterial");
	TextMaterial = Load<UMaterial>("/Game/Models/Others/TextMaterial");

	collection = Load<UMaterialParameterCollection>("/Game/Models/Ground/GlobalWeather");
	snowParticles = Load<UParticleSystem>("/Game/Models/Weather/Snow/SnowParticles");
	blizzardParticles = Load<UParticleSystem>("/Game/Models/Weather/Snow/BlizzardParticles");
	rainParticles = Load<UParticleSystem>("/Game/Models/Weather/Rain/RainParticles");
	M_RainWetness = Load<UMaterial>("/Game/Models/Weather/Rain/M_RainWetness");

	ParticlesByEnum.Add(Load<UParticleSystem>("/Game/Models/Others/P_PunSteamLit"));
	ParticlesByEnum.Add(Load<UParticleSystem>("/Game/Models/Others/P_PunHeavySteamLit"));
	ParticlesByEnum.Add(Load<UParticleSystem>("/Game/Models/Others/P_PunBlackSmokeLit"));
	ParticlesByEnum.Add(Load<UParticleSystem>("/Game/Models/Others/P_PunHeavyBlackSmokeLit"));
	ParticlesByEnum.Add(Load<UParticleSystem>("/Game/Models/Others/P_PunStoveFire"));
	ParticlesByEnum.Add(Load<UParticleSystem>("/Game/Models/Others/P_PunTorchFire2"));

	ParticlesByEnum.Add(Load<UParticleSystem>("/Game/Models/Others/P_CampFire"));
	ParticlesByEnum.Add(Load<UParticleSystem>("/Game/Models/Others/P_BuildingFire"));
	ParticlesByEnum.Add(Load<UParticleSystem>("/Game/Models/Others/P_PunBuildingFireSmokeLit"));

	ParticlesByEnum.Add(Load<UParticleSystem>("/Game/Models/Others/P_PunDemolishDustLit"));


	CheckMeshesAvailable();

	_LOG(PunInit, "!!! AssetLoader Done Init !!!");
}

void UAssetLoaderComponent::InitNiagara()
{
	NiagaraByEnum.Add(NS_OnDemolish);
	NiagaraByEnum.Add(NS_OnPlacement);
	NiagaraByEnum.Add(NS_OnTownhall);
	NiagaraByEnum.Add(NS_OnUpgrade);
}

UMaterialInstanceDynamic* UAssetLoaderComponent::GetResourceIconMaterial(ResourceEnum resourceEnum)
{
	if (_resourceIconMaterials[static_cast<int>(resourceEnum)] == nullptr)
	{
		UMaterialInstanceDynamic* materialInstance = UMaterialInstanceDynamic::Create(ResourceIconMaterial, this);
		materialInstance->SetTextureParameterValue("ColorTexture", GetResourceIcon(resourceEnum));
		materialInstance->SetTextureParameterValue("DepthTexture", GetResourceIconAlpha(resourceEnum));
		_resourceIconMaterials[static_cast<int>(resourceEnum)] = materialInstance;
	}
	
	return _resourceIconMaterials[static_cast<int>(resourceEnum)];
}

void UAssetLoaderComponent::SetMaterialCollectionParametersScalar(FName ParameterName, float ParameterValue)
{
	if (UWorld* world = GetWorld()) {
		UKismetMaterialLibrary::SetScalarParameterValue(world, collection, ParameterName, ParameterValue);
		//PUN_EDITOR_LOG("SetSnow %f", ParameterValue);
	}
}

void UAssetLoaderComponent::SetMaterialCollectionParametersVector(FName ParameterName, FLinearColor ParameterValue)
{
	if (UWorld* world = GetWorld()) {
		UKismetMaterialLibrary::SetVectorParameterValue(world, collection, ParameterName, ParameterValue);
		//PUN_EDITOR_LOG("SetVectorParameterValue %f, %f", ParameterValue.R, ParameterValue.G);
	}
}

UMaterial* UAssetLoaderComponent::GetTerrainMaterial()
{
	if (!_terrainMaterial) {
		_terrainMaterial = Cast<UMaterial>(StaticLoadObject(UMaterial::StaticClass(), nullptr, *FString("/Game/Models/Terrain/Forest/ForestTerrainMaterial")));
	}
	return _terrainMaterial;
}

/**
 * Buildings
 */

UStaticMesh* UAssetLoaderComponent::moduleMesh(FString moduleName)
{
	if (_moduleNameToMesh.Contains(moduleName)) {
		return _moduleNameToMesh[moduleName];
	}
	
	//PUN_EDITOR_LOG("No Module Mesh For %s",  *moduleName);
	return nullptr;
}

void UAssetLoaderComponent::LoadModule(FString moduleName, FString meshFile, bool paintConstructionVertexColor, bool isTogglable)
{
	const FString buildingPath = "/Game/Models/Buildings/";
	const auto mesh = LoadF<UStaticMesh>(buildingPath + meshFile);
	
	AddBuildingModule(moduleName, mesh, _moduleNames);
	
	PUN_EDITOR_LOG("LoadModule: %s _ %s", *_moduleNames.Last(), *mesh->GetName());

	if (isTogglable) {
		_togglableModuleNames.Add(moduleName);
	}

	// Note: this is costly, but usually only done on low poly meshes such as construction frames
	if (paintConstructionVertexColor) {
		_modulesNeedingPaintConstruction.Add(moduleName);
		//PaintMeshForConstruction(moduleName);
	}
}

void UAssetLoaderComponent::LoadTogglableModule(FString moduleName, FString meshFile)
{
	const FString buildingPath = "/Game/Models/Buildings/";
	const auto mesh = LoadF<UStaticMesh>(buildingPath + meshFile);

	AddBuildingModule(moduleName, mesh, _togglableModuleNames);
}

void UAssetLoaderComponent::LoadAnimModule(FString moduleName, FString meshFile)
{
	const FString buildingPath = "/Game/Models/Buildings/";
	const auto mesh = LoadF<UStaticMesh>(buildingPath + meshFile);

	AddBuildingModule(moduleName, mesh, _animModuleNames);
}

void UAssetLoaderComponent::TryLoadBuildingModuleSet(FString moduleSetName, FString meshSetFolder, bool useOldMethod, CardEnum buildingEnum, int32 era)
{
	const FString buildingPath = "Models/Buildings/";
	IPlatformFile& platformFile = FPlatformFileManager::Get().GetPlatformFile();

	if (useOldMethod)
	{
		TArray<FString> meshTypeNames = {
			"SpecialInstant",
			"Frame",
			"Floor",
			"Chimney",
			"Body",
			"Roof",

			"RoofEdge",
			"WindowFrame",
			"FrameAux",
		};
		meshTypeNames.Add(FString("Special"));
		for (int32 i = 1; i < 30; i++)  {
			meshTypeNames.Add(FString("Special") + FString::FromInt(i));
		}

		for (int i = 0; i < meshTypeNames.Num(); i++)
		{
			FString moduleName = moduleSetName + meshTypeNames[i];
			FString path = buildingPath + meshSetFolder + FString("/") + moduleName;

			//PUN_EDITOR_LOG("Try Adding Module: %s path: %s", *moduleName, *path);

			if (platformFile.FileExists(*(FPaths::ProjectContentDir() + path + FString(".uasset"))))
			{
				const auto mesh = LoadF<UStaticMesh>("/Game/" + path);

				AddBuildingModule(moduleName, mesh, _moduleNames);
				PUN_EDITOR_LOG("TryLoadBuildingModuleSet Old: %s _ %s", *_moduleNames.Last(), *mesh->GetName());

				if (meshTypeNames[i].Equals(FString("Frame"))) {
					_modulesNeedingPaintConstruction.Add(moduleName);
				}
			}
		}

		/*
		 * WorkStatic: Meshes that are shown only when the building is working
		 */
		static const TArray<FString> togglableMeshTypeNames = {
			"WorkStatic", // This needs manual specification in DisplayInfo to input position
			"WindowGlass",
		};

		for (int i = 0; i < togglableMeshTypeNames.Num(); i++) {
			FString moduleName = moduleSetName + togglableMeshTypeNames[i];
			FString path = buildingPath + meshSetFolder + FString("/") + moduleName;

			if (platformFile.FileExists(*(FPaths::ProjectContentDir() + path + FString(".uasset"))))
			{
				const auto mesh = LoadF<UStaticMesh>("/Game/" + path);

				AddBuildingModule(moduleName, mesh, _togglableModuleNames);
			}
		}

		/*
		 * Animated Meshes
		 */
		static const TArray<FString> animMeshTypeNames = {
			"WorkRotation1", // This needs manual specification in DisplayInfo to input position
			"WorkRotation2", // This needs manual specification in DisplayInfo to input position
			"WorkShaderAnimate",
			"WorkShaderOnOff", // If we set scale to turn building on and off (such as beer brewery boiler scaling toggle)
		};


		for (int i = 0; i < animMeshTypeNames.Num(); i++) {
			FString moduleName = moduleSetName + animMeshTypeNames[i];
			FString path = buildingPath + meshSetFolder + FString("/") + moduleName;

			if (platformFile.FileExists(*(FPaths::ProjectContentDir() + path + FString(".uasset"))))
			{
				const auto mesh = LoadF<UStaticMesh>("/Game/" + path);
				
				AddBuildingModule(moduleName, mesh, _animModuleNames);
			}
		}
	}
	else
	{
		TArray<FString> foundFiles;

		FString findDirectory = FPaths::ProjectContentDir() + buildingPath + meshSetFolder + FString("/");
		IFileManager::Get().FindFiles(foundFiles, *findDirectory, TEXT(".uasset"));

		check(foundFiles.Num() > 0);

		_recentlyAddedModuleNames.Empty();

		//// foundFiles are just file names, so we append the folder to it
		int32 bodyMainIndex = 1;
		int32 bodySpecialIndex = 1;
		int32 alwaysOnIndex = 1;

		auto loadMesh = [&](int32 fileIndex)
		{
			FString path = "/Game/" + buildingPath + meshSetFolder + FString("/") + foundFiles[fileIndex];
			return LoadF<UStaticMesh>(path.LeftChop(7)); // Chop out .uasset
		};
		
		for (int32 i = 0; i < foundFiles.Num(); i++) 
		{
			//foundFiles[i] = folderPath + FString("/") + foundFiles[i];
			PUN_EDITOR_LOG("Files:%s", *(foundFiles[i]));

			// Is from this set
			if (moduleSetName == foundFiles[i].Left(moduleSetName.Len()))
			{
				PUN_EDITOR_LOG("- File is from set");

				auto addMesh = [&](FString moduleTypeName, bool isSpecial = false)
				{
					FString moduleName = moduleSetName + moduleTypeName;

					//FString path = "/Game/" + buildingPath + meshSetFolder + FString("/") + foundFiles[i];
					//const auto mesh = LoadF<UStaticMesh>(path.LeftChop(7)); // Chop out .uasset

					const auto mesh = loadMesh(i);
					check(mesh);

					// Use Lightmap Resolution 100 to mark
					if (moduleTypeName != "Frame" &&
						moduleTypeName != "AlwaysOn" &&
						(mesh->LightMapResolution == 100 || mesh->GetMaterial(0)->GetName().Contains(TEXT("Roof"), ESearchCase::Type::IgnoreCase))
						&& bodyMainIndex <= 3) 
					{
						moduleName = moduleSetName + FString("Body") + FString::FromInt(bodyMainIndex);
						bodyMainIndex++;
					}
					else if (isSpecial) {
						bodySpecialIndex++;
					}

					AddBuildingModule(moduleName, mesh, _moduleNames);
					
					PUN_EDITOR_LOG("TryLoadBuildingModuleSet New: %s _ %s", *_moduleNames.Last(), *mesh->GetName());
					
					return moduleName;
				};

				
				if (foundFiles[i].Contains("_AlwaysOn_")) {
					addMesh("AlwaysOn");
					alwaysOnIndex++;
				}
				else if (foundFiles[i].Contains("_Body_") ||
					foundFiles[i].Contains("_Body1_") ||
					foundFiles[i].Contains("_Body2_") ||
					foundFiles[i].Contains("_Body3_") ||
					foundFiles[i].Contains("_Windows_") ||
					foundFiles[i].Contains("_Window_"))
				{
					FString moduleTypeNameIn = FString("Special") + FString::FromInt(bodySpecialIndex);
					addMesh(moduleTypeNameIn, true);
				}
				else if (foundFiles[i].Contains("_Scaffolding_")) {
					FString meshName = addMesh("Frame");

					_modulesNeedingPaintConstruction.Add(meshName);
				}
				else if (foundFiles[i].Contains("_Manual_"))
				{
					const auto mesh = loadMesh(i);
					FString moduleName = moduleSetName + "_Manual_" + foundFiles[i];
					AddBuildingModule(moduleName, mesh, _moduleNames);
				}
				else if (foundFiles[i].Contains("_ManualToggle_"))
				{
					const auto mesh = loadMesh(i);
					FString moduleName = moduleSetName + "_ManualToggle_" + foundFiles[i];
					AddBuildingModule(moduleName, mesh, _togglableModuleNames);
				}
				else if (foundFiles[i].Contains("_WorkStatic"))
				{
					const auto mesh = loadMesh(i);
					FString moduleName = moduleSetName + "WorkStatic";

					AddBuildingModule(moduleName, mesh, _togglableModuleNames);

					_tempAuxGroup.togglableTransforms.push_back(ModuleTransform(moduleName));
				}
				else if (foundFiles[i].Contains("_WorkShaderAnimate"))
				{
					const auto mesh = loadMesh(i);
					FString moduleName = moduleSetName + "WorkShaderAnimate";

					AddBuildingModule(moduleName, mesh, _animModuleNames);

					_tempAuxGroup.animTransforms.push_back(ModuleTransform(moduleName, FTransform::Identity, 0.0f, ModuleTypeEnum::ShaderAnimate));
				}
				else if (foundFiles[i].Contains("_WorkShaderOnOff"))
				{
					const auto mesh = loadMesh(i);
					FString moduleName = moduleSetName + "WorkShaderOnOff";

					AddBuildingModule(moduleName, mesh, _animModuleNames);
					
					_tempAuxGroup.animTransforms.push_back(ModuleTransform(moduleName, FTransform::Identity, 0, ModuleTypeEnum::ShaderOnOff));
				}
				// Work Rotation
				else if (foundFiles[i].Contains("_WorkRotation1"))
				{
					const auto mesh = loadMesh(i);
					FString moduleName = moduleSetName + "WorkRotation1";

					AddBuildingModule(moduleName, mesh, _animModuleNames);

					FTransform transform;
					ModuleTypeEnum moduleTypeEnum = ModuleTypeEnum::RotateRoll;

					if (buildingEnum == CardEnum::Quarry) {
						transform = TransformFromPosition(0, -11.122, 7.325);
						moduleTypeEnum = ModuleTypeEnum::RotateRollQuarry;
					}
					else if (IsMountainMine(buildingEnum)) {
						transform = TransformFromPosition(2.3193, 0, 5.1714);
						moduleTypeEnum = ModuleTypeEnum::RotateRollMine;
					}
					else if (buildingEnum == CardEnum::FurnitureWorkshop) {
						transform = TransformFromPosition(6.506, 0, 13.303);
						moduleTypeEnum = ModuleTypeEnum::RotateRollFurniture;
					}
					else if (buildingEnum == CardEnum::PaperMaker) {
						transform = TransformFromPosition(0, 0, 7.388654);
					}
					else {
						UE_DEBUG_BREAK();
					}
					
					_tempAuxGroup.animTransforms.push_back(ModuleTransform(moduleName, transform, 0.0f, moduleTypeEnum));
				}
				else if (foundFiles[i].Contains("_WorkRotation2"))
				{
					const auto mesh = loadMesh(i);
					FString moduleName = moduleSetName + "WorkRotation2";

					AddBuildingModule(moduleName, mesh, _animModuleNames);

					FTransform transform;
					ModuleTypeEnum moduleTypeEnum = ModuleTypeEnum::RotateRoll;
					
					if (IsMountainMine(buildingEnum) && buildingEnum != CardEnum::Quarry) {
						transform = TransformFromPosition(21.994, 0, 27.776);
						moduleTypeEnum = ModuleTypeEnum::RotateRollMine2;
					}

					_tempAuxGroup.animTransforms.push_back(ModuleTransform(moduleName, transform, 0.0f, moduleTypeEnum));
				}
				else if (foundFiles[i].Contains("_Smoke")) {
					const auto mesh = loadMesh(i);
					DetectParticleSystemPosition(buildingEnum, mesh);
				}
			}
		}

		// Hit here means Body needs to be specified. Body is for showing model from far away.
		//  Set with LightMapResolution == 100
		check(bodyMainIndex > 1 || alwaysOnIndex > 1);
	}


}

/**
 * Units
 */
//FUnitAsset UAssetLoaderComponent::unitAsset(UnitEnum unitEnum, int32 variationIndex)
//{
//	int32 unitEnumInt = static_cast<int>(unitEnum);
//	PUN_ENSURE(0 <= unitEnumInt && unitEnumInt < UnitEnumCount, return FUnitAsset());
//
//	std::vector<FUnitAsset>& assets =  _unitEnumToAsset[unitEnumInt];
//	PUN_ENSURE(0 <= variationIndex && variationIndex < assets.size(), return FUnitAsset());
//	
//	const FUnitAsset& asset = assets[variationIndex];
//	PUN_CHECK(asset.staticMesh);
//	return asset;
//}

const std::string unitsPath = "/Game/Models/Units/";
void UAssetLoaderComponent::LoadUnit(UnitEnum unitEnum, std::string meshFile)
{
	FUnitAsset asset;
	asset.staticMesh = Load<UStaticMesh>((unitsPath + meshFile).c_str());
	
	_unitDisplayEnumToAsset[static_cast<int>(GetUnitDisplayEnum(unitEnum, _unitEnumToAsset[static_cast<int>(unitEnum)].size()))] = asset;
	_unitEnumToAsset[static_cast<int>(unitEnum)].push_back(asset);
}

// SkelMesh will still need 
void UAssetLoaderComponent::LoadUnitFull(UnitEnum unitEnum, std::string folderPath, std::string skelFileName,
	std::unordered_map<UnitAnimationEnum, std::string> animationFileNames, std::string staticFileName)
{
	FUnitAsset asset;
	asset.skeletalMesh = Load<USkeletalMesh>((unitsPath + folderPath + skelFileName).c_str());
	for (const auto it : animationFileNames) {
		asset.animationEnumToSequence.Add(it.first, Load<UAnimSequence>((unitsPath + folderPath + it.second).c_str()));
	}
	if (staticFileName != "") {
		//PUN_EDITOR_LOG("LoadUnitFull staticFileName %s static:%s", ToTChar(skelFileName), ToTChar(staticFileName));
		asset.staticMesh = Load<UStaticMesh>((unitsPath + staticFileName).c_str());
	}

	_unitDisplayEnumToAsset[static_cast<int>(GetUnitDisplayEnum(unitEnum, _unitEnumToAsset[static_cast<int>(unitEnum)].size()))] = asset;
	_unitEnumToAsset[static_cast<int>(unitEnum)].push_back(asset);
}
void UAssetLoaderComponent::LoadUnitWeapon(UnitAnimationEnum unitAnimation, std::string file)
{
	_animationEnumToWeaponMesh[unitAnimation] = Load<UStaticMesh>((unitsPath + file).c_str());
}

void UAssetLoaderComponent::LoadUnitAuxMesh(UnitAnimationEnum unitAnimation, std::string file)
{
	_animationEnumToAuxMesh[unitAnimation] = Load<UStaticMesh>((unitsPath + file).c_str());
}

/**
 * Resources
 */

UStaticMesh* UAssetLoaderComponent::resourceMesh(ResourceEnum resourceEnum)
{
	auto got = _resourceToMesh.find(resourceEnum);
	if (got != _resourceToMesh.end()) {
		return _resourceToMesh[resourceEnum];
	}
	_LOG(PunAsset, "No Resource Mesh For %s", ToTChar(ResourceName(resourceEnum)));
	return nullptr;
}

UStaticMesh* UAssetLoaderComponent::resourceHandMesh(ResourceEnum resourceEnum)
{
	auto got = _resourceToHandMesh.find(resourceEnum);
	if (got != _resourceToHandMesh.end()) {
		return _resourceToHandMesh[resourceEnum];
	}
	_LOG(PunAsset, "No Resource Hand Mesh For %s", ToTChar(ResourceName(resourceEnum)));
	return resourceMesh(resourceEnum);
}

void UAssetLoaderComponent::LoadResource(ResourceEnum resourceEnum, std::string meshFile)
{
	const std::string path = "/Game/Models/Resources/";
	_resourceToMesh[resourceEnum] = Load<UStaticMesh>((path + meshFile).c_str());
}

void UAssetLoaderComponent::LoadResource2(ResourceEnum resourceEnum, std::string meshFilePrefix)
{
	const std::string path = "/Game/Models/Resources/";
	_resourceToMesh[resourceEnum] = Load<UStaticMesh>((path + meshFilePrefix + "_Stack").c_str());
	_resourceToHandMesh[resourceEnum] = Load<UStaticMesh>((path + meshFilePrefix + "_Hand").c_str());
}


FGeoresourceMeshAssets UAssetLoaderComponent::georesourceMesh(GeoresourceEnum georesourceEnum)
{
	auto got = _georesourceToMesh.find(georesourceEnum);
	if (got != _georesourceToMesh.end()) {
		return _georesourceToMesh[georesourceEnum];
	}
	//PUN_EDITOR_LOG("No Georesource Mesh For %d", static_cast<int>(georesourceEnum));
	return FGeoresourceMeshAssets();
}

void UAssetLoaderComponent::LoadGeoresource(GeoresourceEnum georesourceEnum, std::string folder, std::string meshFileNamePrefix, int32 numberOfMeshes)
{
	const std::string path = "/Game/Models/Georesources/";
	FGeoresourceMeshAssets meshAssets;
	for (int32 i = 0; i < numberOfMeshes; i++) {
		meshAssets.miniMeshes.Add(Load<UStaticMesh>((path + folder + "/" + meshFileNamePrefix + "MiniMesh" + to_string(i + 1)).c_str()));

		// Landmark resource also have true mesh...
		if (GetGeoresourceInfo(georesourceEnum).isLandmark()) {
			meshAssets.meshes.Add(Load<UStaticMesh>((path + folder + "/" + meshFileNamePrefix + "Mesh" + to_string(i + 1)).c_str()));
		}
	}
	_georesourceToMesh[georesourceEnum] = meshAssets;
}

void UAssetLoaderComponent::PaintConstructionMesh()
{
	PUN_EDITOR_LOG("Painting this shit 11 %d", _modulesNeedingPaintConstruction.Num());
	for (auto i = 0; i < _modulesNeedingPaintConstruction.Num(); i++) {
		PUN_EDITOR_LOG("----- %d", i);
		PaintMeshForConstruction(_modulesNeedingPaintConstruction[i]);
	}
	PUN_EDITOR_LOG("PAINTING ALL DONE");
}

void UAssetLoaderComponent::SaveSmokeJson()
{
	/*
	 * Save meshName_to_groupIndexToConnectedVertIndices to json (for Smoke Position)
	 */
	FString path = FPaths::ConvertRelativePathToFull(FPaths::ProjectContentDir());

	FString saveFileName = "Paks/SmokePositions.json";


	auto allJsonObject = MakeShared<FJsonObject>();

	for (auto meshIt : meshName_to_groupIndexToConnectedVertIndices)
	{
		auto meshJsonObject = MakeShared<FJsonObject>();

		// vertex positions
		TArray<TSharedPtr<FJsonValue>> vertexPositionsJsonArray;
		TArray<FVector>& vertexPositions = meshName_to_vertexPositions[ToFString(meshIt.first)];
		for (FVector vertexPosition : vertexPositions)
		{
			TSharedPtr<FJsonObject> vertexPosition_jsonObj = MakeShared<FJsonObject>();

			vertexPosition_jsonObj->SetNumberField("X", vertexPosition.X);
			vertexPosition_jsonObj->SetNumberField("Y", vertexPosition.Y);
			vertexPosition_jsonObj->SetNumberField("Z", vertexPosition.Z);

			TSharedPtr<FJsonValue> jsonValue = MakeShared<FJsonValueObject>(vertexPosition_jsonObj);

			vertexPositionsJsonArray.Add(jsonValue);
		}
		meshJsonObject->SetArrayField("vertexPosition", vertexPositionsJsonArray);


		auto groupsJsonObject = MakeShared<FJsonObject>();
		std::unordered_map<int32, std::vector<int32>>& groupIndexToConnectedVertIndices = meshIt.second;
		for (auto vertGroupIt : groupIndexToConnectedVertIndices)
		{
			TArray<TSharedPtr<FJsonValue>> Array;

			std::vector<int32>& connectedVertIndices = vertGroupIt.second;
			for (int32 connectedVertIndex : connectedVertIndices)
			{
				Array.Add(MakeShareable(new FJsonValueNumber(connectedVertIndex)));
			}

			groupsJsonObject->SetArrayField(FString::FromInt(vertGroupIt.first), Array);
		}
		meshJsonObject->SetObjectField("group", groupsJsonObject);


		allJsonObject->SetObjectField(ToFString(meshIt.first), meshJsonObject);
	}


	FString jsonString;
	TSharedRef<TJsonWriter<>> writer = TJsonWriterFactory<>::Create(&jsonString);
	FJsonSerializer::Serialize(allJsonObject, writer);

	FFileHelper::SaveStringToFile(jsonString, *(path + saveFileName));

}

void UAssetLoaderComponent::RemoveVertexColor() 
{
	PUN_EDITOR_LOG("RemoveVertexColor");
	for (auto i = 0; i < _modulesNeedingPaintConstruction.Num(); i++) {
		FString moduleName = _modulesNeedingPaintConstruction[i];
		FStaticMeshVertexBuffers* vertexBuffers = &_moduleNameToMesh[moduleName]->RenderData->LODResources[0].VertexBuffers;
		UStaticMesh* mesh = _moduleNameToMesh[moduleName];

		mesh->RemoveVertexColors();
	}
}

void UAssetLoaderComponent::PrintConstructionMesh()
{
	PUN_EDITOR_LOG("PrintConstructionMesh dfgsdfg %d", _modulesNeedingPaintConstruction.Num());
	for (auto i = 0; i < _modulesNeedingPaintConstruction.Num(); i++) {
		FString moduleName = _modulesNeedingPaintConstruction[i];
		FStaticMeshVertexBuffers& vertexBuffers = _moduleNameToMesh[moduleName]->RenderData->LODResources[0].VertexBuffers;
		UStaticMesh* mesh = _moduleNameToMesh[moduleName];

		int vertexCount = vertexBuffers.PositionVertexBuffer.GetNumVertices();
		PUN_EDITOR_LOG("PrintConstructionMesh %s, vertCount: %d , colorBuffer:%d", *moduleName, vertexCount, &vertexBuffers.ColorVertexBuffer != nullptr);
		if (&vertexBuffers.ColorVertexBuffer) {
			PUN_EDITOR_LOG(" ------------ colorCount:%d", vertexBuffers.ColorVertexBuffer.GetNumVertices());
		}

		for (int j = 0; j < vertexCount; j++) {
			FString colorStr;
			if (&vertexBuffers.ColorVertexBuffer && vertexBuffers.ColorVertexBuffer.GetNumVertices() > 0) {
				colorStr = vertexBuffers.ColorVertexBuffer.VertexColor(j).ToString();
			}

			PUN_EDITOR_LOG("%s ... %s", *vertexBuffers.PositionVertexBuffer.VertexPosition(j).ToCompactString(), *colorStr);
		}

	}
}

void UAssetLoaderComponent::UpdateRHIConstructionMesh()
{
	PUN_EDITOR_LOG("UpdateRHIConstructionMesh aaa %d", _modulesNeedingPaintConstruction.Num());
	//for (int i = 0; i < _modulesNeedingPaintConstruction.Num(); i++) {
	//	FString moduleName = _modulesNeedingPaintConstruction[i];
	//	UStaticMesh* mesh = _moduleNameToMesh[moduleName];

	//	mesh->SetVertexColorData(_colorMap);
	//	mesh->Build(false); // ??? Some weird bullshit with threading.. slower non-silent build can circumvent the issue
	//	mesh->MarkPackageDirty();
	//}
}


static unordered_map<int32, std::vector<int32>> groupIndexToConnectedVertIndices; //Can't do TMap+TArray

static unordered_map<int32, std::vector<int32>> vertIndexToConnectedTrisIndices;

static unordered_map<int32, std::vector<int32>> mergedVertIndexToVertIndices;
static std::vector<int32> vertIndexToMergedVertIndex;

static unordered_map<int32, std::vector<int32>> mergedVertIndexToConnectedTrisIndices;
static unordered_map<int32, std::vector<int32>> groupIndexToConnectedTrisIndices;

static TArray<bool> processedVertices;
static int32 isPrinting = false;

void UAssetLoaderComponent::TraverseTris(uint32 mergedVertIndex, int32 groupIndex_Parent, const TArray<uint32>& indexBuffer, const TArray<FVector>& vertexPositions)
{
	//if (isPrinting) PUN_EDITOR_LOG(" TraverseTris groupIndex:%d groupIndex_Parent:%d %s", groupIndex, groupIndex_Parent, *vertexPositions.VertexPosition(groupIndex).ToCompactString());
	
	// already processed
	if (processedVertices[mergedVertIndex]) {
		return;
	}
	processedVertices[mergedVertIndex] = true;

	const vector<int32>& trisIndices = mergedVertIndexToConnectedTrisIndices[mergedVertIndex];

	for (int32 trisIndex : trisIndices)
	{
		int32 vertIndex = indexBuffer[trisIndex];
		
		CppUtils::TryAdd(groupIndexToConnectedVertIndices[groupIndex_Parent], vertIndex);
		processedVertices[vertIndex] = true;
		
		int32 trisIndex_0 = (trisIndex / 3) * 3;

		//if (isPrinting) PUN_EDITOR_LOG("   -Recurse trisIndex_0:%d", trisIndex_0);

		PUN_CHECK((indexBuffer.Num() / 3) * 3 == indexBuffer.Num());
		PUN_CHECK((trisIndex_0 + 2) < indexBuffer.Num());

		TraverseTris(vertIndexToMergedVertIndex[indexBuffer[trisIndex_0]], groupIndex_Parent, indexBuffer, vertexPositions);
		TraverseTris(vertIndexToMergedVertIndex[indexBuffer[trisIndex_0 + 1]], groupIndex_Parent, indexBuffer, vertexPositions);
		TraverseTris(vertIndexToMergedVertIndex[indexBuffer[trisIndex_0 + 2]], groupIndex_Parent, indexBuffer, vertexPositions);
	}
};

void UAssetLoaderComponent::TraverseTris_July10(uint32 mergedVertIndex, int32 groupIndex_Parent, const TArray<int32>& indexBuffer, const TArray<FVector>& vertexPositions)
{
	//if (isPrinting) PUN_EDITOR_LOG(" TraverseTris groupIndex:%d groupIndex_Parent:%d %s", groupIndex, groupIndex_Parent, *vertexPositions.VertexPosition(groupIndex).ToCompactString());

	// already processed
	if (processedVertices[mergedVertIndex]) {
		return;
	}
	processedVertices[mergedVertIndex] = true;

	const vector<int32>& trisIndices = mergedVertIndexToConnectedTrisIndices[mergedVertIndex];

	for (int32 trisIndex : trisIndices)
	{
		int32 vertIndex = indexBuffer[trisIndex];

		CppUtils::TryAdd(groupIndexToConnectedVertIndices[groupIndex_Parent], vertIndex);
		processedVertices[vertIndex] = true;

		int32 trisIndex_0 = (trisIndex / 3) * 3;

		//if (isPrinting) PUN_EDITOR_LOG("   -Recurse trisIndex_0:%d", trisIndex_0);

		PUN_CHECK((indexBuffer.Num() / 3) * 3 == indexBuffer.Num());
		PUN_CHECK((trisIndex_0 + 2) < indexBuffer.Num());

		TraverseTris_July10(vertIndexToMergedVertIndex[indexBuffer[trisIndex_0]], groupIndex_Parent, indexBuffer, vertexPositions);
		TraverseTris_July10(vertIndexToMergedVertIndex[indexBuffer[trisIndex_0 + 1]], groupIndex_Parent, indexBuffer, vertexPositions);
		TraverseTris_July10(vertIndexToMergedVertIndex[indexBuffer[trisIndex_0 + 2]], groupIndex_Parent, indexBuffer, vertexPositions);
	}
};

void UAssetLoaderComponent::DetectParticleSystemPosition(CardEnum buildingEnum, UStaticMesh* mesh)
{
	TArray<FVector> vertexPositions;

//#if WITH_EDITOR
	// In the editor, we DetectMeshGroups and cache results in meshName_to_groupIndexToConnectedVertIndices
	
	//isPrinting = true;
	//DetectMeshGroups(mesh, vertexPositions);
	isPrinting = false;

	//meshName_to_vertexPositions.Add(mesh->GetName(), vertexPositions);
	//meshName_to_groupIndexToConnectedVertIndices[ToStdString(mesh->GetName())] = groupIndexToConnectedVertIndices;
//#else
	/*
	 * Load groupIndexToConnectedVertIndices
	 *  -  In the actual game, we fill meshName_to_groupIndexToConnectedVertIndices from json file
	 */
	if (meshName_to_groupIndexToConnectedVertIndices.size() == 0)
	{
		FString path = FPaths::ConvertRelativePathToFull(FPaths::ProjectContentDir());
		FString saveFileName = "Paks/SmokePositions.json";
		
		FString jsonString;
		FFileHelper::LoadFileToString(jsonString, *(path + saveFileName));

		TSharedPtr<FJsonObject> all_jsonObj(new FJsonObject());

		TSharedRef<TJsonReader<>> reader = TJsonReaderFactory<>::Create(jsonString);
		FJsonSerializer::Deserialize(reader, all_jsonObj);

		for (auto& all_jsonObj_it : all_jsonObj->Values)
		{
			const FString& meshName = all_jsonObj_it.Key;
			const TSharedPtr<FJsonObject>& mesh_jsonObj = all_jsonObj->GetObjectField(meshName);

			meshName_to_vertexPositions.Add(meshName, {});
			const TArray<TSharedPtr<FJsonValue>>& vertexPositions_jsonArray = mesh_jsonObj->GetArrayField("vertexPosition");
			for (int32 j = 0; j < vertexPositions_jsonArray.Num(); j++)
			{
				TSharedPtr<FJsonObject> vertexPosition_jsonObj = vertexPositions_jsonArray[j]->AsObject();
				meshName_to_vertexPositions[meshName].Add(
					FVector(vertexPosition_jsonObj->GetNumberField("X"),
								vertexPosition_jsonObj->GetNumberField("Y"),
								vertexPosition_jsonObj->GetNumberField("Z"))
				);
			}

			const TSharedPtr<FJsonObject>& groups_jsonObj = mesh_jsonObj->GetObjectField("group");
			for (auto& groups_jsonObj_it : groups_jsonObj->Values)
			{
				std::vector<int32> connectedIndices;
				
				const FString& groupIndex = groups_jsonObj_it.Key;
				
				const TArray<TSharedPtr<FJsonValue>>& vertIndices_jsonObj = groups_jsonObj->GetArrayField(groupIndex);
				for (int32 i = 0; i < vertIndices_jsonObj.Num(); i++) {
					connectedIndices.push_back(FMath::RoundToInt(vertIndices_jsonObj[i]->AsNumber()));
				}
				
				meshName_to_groupIndexToConnectedVertIndices[ToStdString(meshName)][FCString::Atoi(*groupIndex)] = connectedIndices;
			}
		}
	}

	vertexPositions = meshName_to_vertexPositions[mesh->GetName()];
	groupIndexToConnectedVertIndices = meshName_to_groupIndexToConnectedVertIndices[ToStdString(mesh->GetName())];

//#endif

	for (const auto& it : groupIndexToConnectedVertIndices) 
	{
		PUN_EDITOR_LOG(" group:%d", it.first);

		FVector averagePosition = FVector::ZeroVector;
		for (int32 vertIndex : it.second) {
			PUN_EDITOR_LOG("  vertIndex:%d %s", vertIndex, *vertexPositions[vertIndex].ToCompactString());
			averagePosition += vertexPositions[vertIndex];
		}
		averagePosition = averagePosition / it.second.size();

		ParticleEnum particleEnum = ParticleEnum::Smoke;
		const std::vector<CardEnum> blackSmokers {
			CardEnum::Blacksmith,
			CardEnum::CandleMaker,
			CardEnum::ConcreteFactory,
			CardEnum::GoldSmelter,
			CardEnum::Mint,
			CardEnum::GlassSmelter,
			CardEnum::Glassworks,
			CardEnum::CharcoalMaker
		};
		const std::vector<CardEnum> heavySteam{
			CardEnum::CottonMill,
			CardEnum::PaperMill,
			CardEnum::BeerBrewery,
		};
		
		if (it.second.size() == 12) { // BlackSmoke
			particleEnum = ParticleEnum::BlackSmoke;
		}
		else if (it.second.size() == 16) { // TorchFire
			particleEnum = ParticleEnum::TorchFire;
		}
		else if (CppUtils::Contains(blackSmokers, buildingEnum)) {
			particleEnum = ParticleEnum::BlackSmoke;
		}
		else if (IsPollutingHeavyIndustryOrMine(buildingEnum)) {
			particleEnum = ParticleEnum::HeavyBlackSmoke;
		}
		else if (CppUtils::Contains(heavySteam, buildingEnum)) {
			particleEnum = ParticleEnum::HeavySteam;
		}
		else if (it.second.size() == 12) { // Black Smoker
			particleEnum = ParticleEnum::BlackSmoke;
		}

		_tempAuxGroup.particleInfos.push_back({ particleEnum, FTransform(averagePosition) });
	}

}

void UAssetLoaderComponent::DetectMeshGroups(UStaticMesh* mesh, TArray<FVector>& vertexPositions)
{
#if !WITH_EDITOR
	return;
#endif
	_LOG(PunInit, "DetectMeshGroups %s", *mesh->GetName());
	
	//// TODO: use this for Paint Mesh?
	//const uint32 meshLODIndex = 0;
	//if (mesh->IsSourceModelValid(meshLODIndex) &&
	//	mesh->GetSourceModel(meshLODIndex).IsRawMeshEmpty() == false)
	//{
	//	// Extract the raw mesh.
	//	FRawMesh rawMesh;
	//	mesh->GetSourceModel(meshLODIndex).LoadRawMesh(rawMesh);

	//	// Reserve space for the new vertex colors.
	//	if (rawMesh.WedgeColors.Num() == 0 || rawMesh.WedgeColors.Num() != rawMesh.WedgeIndices.Num())
	//	{
	//		rawMesh.WedgeColors.Empty(rawMesh.WedgeIndices.Num());
	//		rawMesh.WedgeColors.AddUninitialized(rawMesh.WedgeIndices.Num());
	//	}

	// vertexPositions = rawMesh.VertexPositions;
	// const TArray<uint32>& wedgeIndices = rawMesh.WedgeIndices;

	if (mesh &&
		mesh->RenderData &&
		mesh->RenderData->LODResources.Num() > 0)
	{
		// Set Vertex Position
		vertexPositions.Empty();

		FStaticMeshLODResources& meshLOD0 = mesh->RenderData->LODResources[0];
		
		const FPositionVertexBuffer& VertexBuffer = meshLOD0.VertexBuffers.PositionVertexBuffer;
		
		const int32 VertexCount = VertexBuffer.GetNumVertices();
		for (int32 Index = 0; Index < VertexCount; Index++) {
			vertexPositions.Add(VertexBuffer.VertexPosition(Index));
		}

		_LOG(PunInit, " -- VertexCount:%d", VertexCount);




		
		FRawStaticIndexBuffer* IndexBuffer = &(meshLOD0.IndexBuffer);
		int32 numIndices = IndexBuffer->GetNumIndices();
		check(numIndices >= 0);


		_LOG(PunInit, " -- numIndices:%d", numIndices);
		
		
		TArray<int32> wedgeIndices;
		
		for (int32 i = 0; i < numIndices; i++) {
			int32 wedgeIndex = meshLOD0.IndexBuffer.GetIndex(i);
			wedgeIndices.Add(wedgeIndex);
		}

#if WITH_EDITOR
		const TArray<int32>& wedgeIndicesTest = meshLOD0.WedgeMap;

		check(wedgeIndices.Num() == wedgeIndicesTest.Num());
#endif

		_LOG(PunInit, " -- wedgeIndices %d", wedgeIndices.Num());
	
		// ---------------

		stringstream ss;

		int32 vertexCount = vertexPositions.Num();
		int32 indexCount = wedgeIndices.Num();

		//PUN_EDITOR_LOG("Start Painting");

		auto vertToString = [&](int32 vertIndex) -> FString {
			return vertexPositions[vertIndex].ToCompactString();
		};

		// Print Vertices
		if (isPrinting) {
			PUN_EDITOR_LOG("Print Vertices:");
			for (int32 i = 0; i < vertexCount; i++) {
				PUN_EDITOR_LOG(" %s", *vertToString(i));
			}
		}


		/*
		 * Merge nearby vertices
		 */
		{
			mergedVertIndexToVertIndices.clear();
			vertIndexToMergedVertIndex.clear();
			vertIndexToMergedVertIndex.resize(vertexCount, 0);


			processedVertices.Empty();
			processedVertices.SetNumZeroed(vertexCount);

			if (isPrinting) { PUN_EDITOR_LOG("Merge nearby vertices"); }

			for (int i = 0; i < vertexCount; i++)
			{
				if (isPrinting) { PUN_EDITOR_LOG(" i = %d", i); }

				if (!processedVertices[i])
				{
					processedVertices[i] = true;
					mergedVertIndexToVertIndices[i].push_back(i);
					vertIndexToMergedVertIndex[i] = i;

					if (isPrinting) { PUN_EDITOR_LOG(" i,i groupIndexToVertIndices_PositionMerge[%d].push_back(%d)", i, i); }

					// Add any vertex with the same position
					// This is because UE4 import sometimes split up the geometry...
					for (int j = i + 1; j < vertexCount; j++) {
						if (!processedVertices[j] &&
							vertexPositions[i].Equals(vertexPositions[j], 0.01f))
						{
							processedVertices[j] = true;
							mergedVertIndexToVertIndices[i].push_back(j);
							vertIndexToMergedVertIndex[j] = i;

							if (isPrinting) { PUN_EDITOR_LOG(" i,j groupIndexToVertIndices_PositionMerge[%d].push_back(%d)", i, j); }
						}
					}
				}
			}


			PUN_EDITOR_LOG("Position Merge Group Count:%d vertCount:%d", mergedVertIndexToVertIndices.size(), vertexCount);

			if (isPrinting)
			{
				PUN_EDITOR_LOG("________");
				PUN_EDITOR_LOG("Print mergedVertIndexToVertIndices size:%d", mergedVertIndexToVertIndices.size());
				for (const auto& it : mergedVertIndexToVertIndices) {
					PUN_EDITOR_LOG(" mergedVertIndex:%d", it.first);
					for (int32 vertIndex : it.second) {
						PUN_EDITOR_LOG("  vertIndex:%d %s", vertIndex, *vertToString(vertIndex));
					}
				}
			}
		}


		/*
		 * Generate vertIndexToConnectedTrisIndices
		 */
		vertIndexToConnectedTrisIndices.clear();

		for (int indexBufferI = 0; indexBufferI < indexCount; indexBufferI++)
		{
			int32 vertIndex = wedgeIndices[indexBufferI];
			vertIndexToConnectedTrisIndices[vertIndex].push_back(indexBufferI);

			if (isPrinting) {
				PUN_EDITOR_LOG("- indexI:%d vertIndex:%d vertIndexToConnectedTrisIndices.size:%d", indexBufferI, vertIndex, vertIndexToConnectedTrisIndices.size());
			}
		}

		if (isPrinting)
		{
			PUN_EDITOR_LOG("________");
			PUN_EDITOR_LOG("Print vertIndexToConnectedTrisIndices size:%d", vertIndexToConnectedTrisIndices.size());
			for (const auto& it : vertIndexToConnectedTrisIndices)
			{
				PUN_EDITOR_LOG(" vertIndex:%d %s", it.first, *vertToString(it.first));
				for (int32 trisIndex : it.second) {
					PUN_CHECK(trisIndex < wedgeIndices.Num());
					int32 connectedVertIndex = wedgeIndices[trisIndex];
					PUN_EDITOR_LOG("  trisIndex:%d vertIndex:%d %s", trisIndex, connectedVertIndex, *vertToString(connectedVertIndex));
				}
			}
		}

		// How much average tris connected to vert?
		{
			int32 total = 0;
			for (const auto& it : vertIndexToConnectedTrisIndices) {
				total += it.second.size();
			}
			PUN_EDITOR_LOG("Average trisIndices per Vert: %f", static_cast<float>(total) / vertIndexToConnectedTrisIndices.size());
		}

		/*
		 * Merge into mergedVertIndexToConnectedTrisIndices
		 *  - Allow for easy traversal to other vertIndex via tris
		 */
		mergedVertIndexToConnectedTrisIndices.clear();

		for (const auto& it : mergedVertIndexToVertIndices)
		{
			for (int32 vertIndex : it.second) {
				const vector<int32>& trisIndices = vertIndexToConnectedTrisIndices[vertIndex];
				vector<int32>& result = mergedVertIndexToConnectedTrisIndices[it.first];
				result.insert(result.end(), trisIndices.begin(), trisIndices.end());
			}
		}

		if (isPrinting)
		{
			PUN_EDITOR_LOG("________");
			PUN_EDITOR_LOG("Print mergedVertIndexToConnectedTrisIndices num:%d", mergedVertIndexToConnectedTrisIndices.size());
			for (const auto& it : mergedVertIndexToConnectedTrisIndices) {
				PUN_EDITOR_LOG(" mergedVertIndex:%d", it.first);
				for (int32 trisIndex : it.second) {
					PUN_CHECK(trisIndex < wedgeIndices.Num());
					int32 vertIndex = wedgeIndices[trisIndex];
					PUN_EDITOR_LOG(" trisIndex:%d vertIndex:%d %s", trisIndex, vertIndex, *vertToString(vertIndex));
				}
			}
		}


		/*
		 * Traverse tris to merge
		 */
		{
			processedVertices.Empty();
			processedVertices.SetNumZeroed(vertexCount);

			groupIndexToConnectedVertIndices.clear();

			if (isPrinting) { PUN_EDITOR_LOG("Traverse Tris"); }

			for (const auto& it : mergedVertIndexToVertIndices) {
				if (isPrinting) { PUN_EDITOR_LOG(" groupIndex:%d", it.first); }

				TraverseTris_July10(it.first, it.first, wedgeIndices, vertexPositions);
			}
		}

		// Check vertexCount to ensure no group has duplicate vertIndex
		{
			int32 vertCount = 0;
			for (const auto& it : groupIndexToConnectedVertIndices) {
				vertCount += it.second.size();
			}
			PUN_CHECK(vertCount <= vertexCount);
		}

		// Print groupIndexToConnectedVertIndices
		if (isPrinting)
		{
			PUN_EDITOR_LOG("________");
			PUN_EDITOR_LOG("Print groupIndexToConnectedVertIndices size:%d", groupIndexToConnectedVertIndices.size());
			for (const auto& it : groupIndexToConnectedVertIndices) {
				PUN_EDITOR_LOG(" groupIndex:%d", it.first);
				const auto& vertIndices = it.second;
				for (int32 vertIndex : vertIndices) {
					PUN_EDITOR_LOG("  vertIndex:%d pos:%s", vertIndex, *vertToString(vertIndex));
				}
			}
		}

		PUN_EDITOR_LOG("Group Count %d", groupIndexToConnectedVertIndices.size());
	}
//#endif
}

void UAssetLoaderComponent::PaintMeshForConstruction(FString moduleName)
{
#if WITH_EDITOR
	PUN_EDITOR_LOG("PaintMeshForConstruction s1ds33s %s", *moduleName);

	isPrinting = false; // (moduleName == "HouseLvl1Frame");

	UStaticMesh* mesh = _moduleNameToMesh[moduleName];

	PUN_EDITOR_LOG("!!! Start Paint !!! UpdateRHIConstructionMesh");

	/*
	 * Paint
	 */
	const uint32 PaintingMeshLODIndex = 0;
	if (mesh->IsSourceModelValid(PaintingMeshLODIndex) &&
		mesh->GetSourceModel(PaintingMeshLODIndex).IsRawMeshEmpty() == false)
	{
		// Extract the raw mesh.
		FRawMesh rawMesh;
		mesh->GetSourceModel(PaintingMeshLODIndex).LoadRawMesh(rawMesh);

		// Reserve space for the new vertex colors.
		if (rawMesh.WedgeColors.Num() == 0 || rawMesh.WedgeColors.Num() != rawMesh.WedgeIndices.Num())
		{
			rawMesh.WedgeColors.Empty(rawMesh.WedgeIndices.Num());
			rawMesh.WedgeColors.AddUninitialized(rawMesh.WedgeIndices.Num());
		}

		// ---------------

		stringstream ss;

		const TArray<FVector>& vertexPositions = rawMesh.VertexPositions;
		const TArray<uint32>& wedgeIndices = rawMesh.WedgeIndices;
		int32 vertexCount = vertexPositions.Num();
		int32 indexCount = wedgeIndices.Num();

		//PUN_EDITOR_LOG("Start Painting");

		auto vertToString = [&](int32 vertIndex) -> FString {
			return vertexPositions[vertIndex].ToCompactString();
		};

		// Print Vertices
		if (isPrinting) {
			PUN_EDITOR_LOG("Print Vertices:");
			for (int32 i = 0; i < vertexCount; i++) {
				PUN_EDITOR_LOG(" %s", *vertToString(i));
			}
		}


		/*
		 * Merge nearby vertices
		 */
		{
			mergedVertIndexToVertIndices.clear();
			vertIndexToMergedVertIndex.clear();
			vertIndexToMergedVertIndex.resize(vertexCount, 0);


			processedVertices.Empty();
			processedVertices.SetNumZeroed(vertexCount);

			if (isPrinting) PUN_EDITOR_LOG("Merge nearby vertices");

			for (int i = 0; i < vertexCount; i++)
			{
				if (isPrinting) PUN_EDITOR_LOG(" i = %d", i);

				if (!processedVertices[i])
				{
					processedVertices[i] = true;
					mergedVertIndexToVertIndices[i].push_back(i);
					vertIndexToMergedVertIndex[i] = i;

					if (isPrinting) PUN_EDITOR_LOG(" i,i groupIndexToVertIndices_PositionMerge[%d].push_back(%d)", i, i);

					// Add any vertex with the same position
					// This is because UE4 import sometimes split up the geometry...
					for (int j = i + 1; j < vertexCount; j++) {
						if (!processedVertices[j] &&
							vertexPositions[i].Equals(vertexPositions[j], 0.01f))
						{
							processedVertices[j] = true;
							mergedVertIndexToVertIndices[i].push_back(j);
							vertIndexToMergedVertIndex[j] = i;

							if (isPrinting) PUN_EDITOR_LOG(" i,j groupIndexToVertIndices_PositionMerge[%d].push_back(%d)", i, j);
						}
					}
				}
			}


			PUN_EDITOR_LOG("Position Merge Group Count:%d vertCount:%d", mergedVertIndexToVertIndices.size(), vertexCount);

			if (isPrinting)
			{
				PUN_EDITOR_LOG("________");
				PUN_EDITOR_LOG("Print mergedVertIndexToVertIndices size:%d", mergedVertIndexToVertIndices.size());
				for (const auto& it : mergedVertIndexToVertIndices) {
					PUN_EDITOR_LOG(" mergedVertIndex:%d", it.first);
					for (int32 vertIndex : it.second) {
						PUN_EDITOR_LOG("  vertIndex:%d %s", vertIndex, *vertToString(vertIndex));
					}
				}
			}
		}


		/*
		 * Generate vertIndexToConnectedTrisIndices
		 */
		vertIndexToConnectedTrisIndices.clear();
		
		for (int indexBufferI = 0; indexBufferI < indexCount; indexBufferI++)
		{
			int32 vertIndex = wedgeIndices[indexBufferI];
			vertIndexToConnectedTrisIndices[vertIndex].push_back(indexBufferI);

			if (isPrinting) PUN_EDITOR_LOG("- indexI:%d vertIndex:%d vertIndexToConnectedTrisIndices.size:%d", indexBufferI, vertIndex, vertIndexToConnectedTrisIndices.size());
		}

		if (isPrinting)
		{
			PUN_EDITOR_LOG("________");
			PUN_EDITOR_LOG("Print vertIndexToConnectedTrisIndices size:%d", vertIndexToConnectedTrisIndices.size());
			for (const auto& it : vertIndexToConnectedTrisIndices)
			{
				PUN_EDITOR_LOG(" vertIndex:%d %s", it.first, *vertToString(it.first));
				for (int32 trisIndex : it.second) {
					PUN_CHECK(trisIndex < wedgeIndices.Num());
					int32 connectedVertIndex = wedgeIndices[trisIndex];
					PUN_EDITOR_LOG("  trisIndex:%d vertIndex:%d %s", trisIndex, connectedVertIndex, *vertToString(connectedVertIndex));
				}
			}
		}

		// How much average tris connected to vert?
		{
			int32 total = 0;
			for (const auto& it : vertIndexToConnectedTrisIndices) {
				total += it.second.size();
			}
			PUN_EDITOR_LOG("Average trisIndices per Vert: %f", static_cast<float>(total) / vertIndexToConnectedTrisIndices.size());
		}

		/*
		 * Merge into mergedVertIndexToConnectedTrisIndices
		 *  - Allow for easy traversal to other vertIndex via tris
		 */
		mergedVertIndexToConnectedTrisIndices.clear();

		for (const auto& it : mergedVertIndexToVertIndices)
		{
			for (int32 vertIndex : it.second) {
				const vector<int32>& trisIndices = vertIndexToConnectedTrisIndices[vertIndex];
				vector<int32>& result = mergedVertIndexToConnectedTrisIndices[it.first];
				result.insert(result.end(), trisIndices.begin(), trisIndices.end());
			}
		}

		if (isPrinting)
		{
			PUN_EDITOR_LOG("________");
			PUN_EDITOR_LOG("Print mergedVertIndexToConnectedTrisIndices num:%d", mergedVertIndexToConnectedTrisIndices.size());
			for (const auto& it : mergedVertIndexToConnectedTrisIndices) {
				PUN_EDITOR_LOG(" mergedVertIndex:%d", it.first);
				for (int32 trisIndex : it.second) {
					PUN_CHECK(trisIndex < wedgeIndices.Num());
					int32 vertIndex = wedgeIndices[trisIndex];
					PUN_EDITOR_LOG(" trisIndex:%d vertIndex:%d %s", trisIndex, vertIndex, *vertToString(vertIndex));
				}
			}
		}


		/*
		 * Traverse tris to merge
		 */
		{
			processedVertices.Empty();
			processedVertices.SetNumZeroed(vertexCount);

			groupIndexToConnectedVertIndices.clear();

			if (isPrinting) PUN_EDITOR_LOG("Traverse Tris");

			for (const auto& it : mergedVertIndexToVertIndices) {
				if (isPrinting) PUN_EDITOR_LOG(" groupIndex:%d", it.first);

				TraverseTris(it.first, it.first, wedgeIndices, vertexPositions);
			}
		}

		// Check vertexCount to ensure no group has duplicate vertIndex
		{
			int32 vertCount = 0;
			for (const auto& it : groupIndexToConnectedVertIndices) {
				vertCount += it.second.size();
			}
			PUN_CHECK(vertCount <= vertexCount);
		}

		// Print groupIndexToConnectedVertIndices
		if (isPrinting)
		{
			PUN_EDITOR_LOG("________");
			PUN_EDITOR_LOG("Print groupIndexToConnectedVertIndices size:%d", groupIndexToConnectedVertIndices.size());
			for (const auto& it : groupIndexToConnectedVertIndices) {
				PUN_EDITOR_LOG(" groupIndex:%d", it.first);
				const auto& vertIndices = it.second;
				for (int32 vertIndex : vertIndices) {
					PUN_EDITOR_LOG("  vertIndex:%d pos:%s", vertIndex, *vertToString(vertIndex));
				}
			}
		}

		PUN_EDITOR_LOG("Group Count %d", groupIndexToConnectedVertIndices.size());



		
		/*
		 * Build meshGroupInfo list sorted by submesh's lowest height
		 */
		struct MeshGroupInfo {
			int32 groupIndex;
			float lowestX;
			float lowestY;
			float lowestZ;
			float highestZ;
		};
		std::vector<MeshGroupInfo> meshGroupInfos;

		ss << "Start Loop groupIndexToConnectedIndices\n";
		for (auto& it : groupIndexToConnectedVertIndices)
		{
			float lowestZ = 999.0f;
			float lowestX = 999.0f;
			float lowestY = 999.0f;
			float highestZ = -999.0f;
			std::vector<int32>& connectedVertIndices = it.second;

			ss << "connectedIndices size:" << to_string(connectedVertIndices.size()) << "\n";

			for (auto& connectedVertIndex : connectedVertIndices) {
				float x = vertexPositions[connectedVertIndex].X;
				if (x < lowestX) {
					lowestX = x;
				}
				float y = vertexPositions[connectedVertIndex].Y;
				if (y < lowestY) {
					lowestY = y;
				}
				float z = vertexPositions[connectedVertIndex].Z;
				if (z < lowestZ) {
					lowestZ = z;
				}
				if (z > highestZ) {
					highestZ = z;
				}
			}
			PUN_CHECK2(lowestZ < 999.0f, ss.str());
			PUN_CHECK2(highestZ > -999.0f, ss.str());
			meshGroupInfos.push_back({ it.first, lowestX, lowestY, lowestZ, highestZ });
		}

		sort(meshGroupInfos.begin(), meshGroupInfos.end(), [&](const MeshGroupInfo& a, const MeshGroupInfo& b) -> bool {
			// Same level Z
			if (FGenericPlatformMath::RoundToInt(a.lowestZ) == FGenericPlatformMath::RoundToInt(b.lowestZ)) {
				return a.lowestX > b.lowestX;
				
				//// compare highestZ
				//if (FGenericPlatformMath::RoundToInt(a.highestZ / 2) == FGenericPlatformMath::RoundToInt(b.highestZ / 2)) {
				//	// Same highestZ, compare X, Y
				//	return a.lowestX < b.lowestX;
				//}
				//return a.highestZ > b.highestZ; // the group with higher highestZ, or higher length is probably vertical, which should come first
			}
			return a.lowestZ < b.lowestZ;
		});

		PUN_EDITOR_LOG("meshGroupInfos %d groupTrisCount:%f indices:%d indices/3:%d verts:%d", meshGroupInfos.size(), static_cast<float>(indexCount / 3) / meshGroupInfos.size(), indexCount, indexCount / 3, vertexCount);


		/*
		 * Make groupIndexToConnectedTrisIndices
		 */
		{
			groupIndexToConnectedTrisIndices.clear();
			for (const auto& it : groupIndexToConnectedVertIndices)
			{
				for (int32 vertIndex : it.second) {
					int32 mergedVertIndex = vertIndexToMergedVertIndex[vertIndex];
					vector<int32>& trisIndices = mergedVertIndexToConnectedTrisIndices[mergedVertIndex];
					vector<int32>& result = groupIndexToConnectedTrisIndices[it.first];
					result.insert(result.end(), trisIndices.begin(), trisIndices.end());
				}
			}

			if (isPrinting)
			{
				PUN_EDITOR_LOG("________");
				PUN_EDITOR_LOG("Print groupIndexToConnectedTrisIndices size:%d", groupIndexToConnectedTrisIndices.size());
				for (const auto& it : groupIndexToConnectedTrisIndices) {
					PUN_EDITOR_LOG(" groupIndex:%d", it.first);
					for (int32 trisIndex : it.second) {
						int32 vertIndex = wedgeIndices[trisIndex];
						PUN_EDITOR_LOG("  trisIndex:%d vertIndex:%d %s", trisIndex, vertIndex, *vertToString(vertIndex));
					}
				}
			}
		}


		/*
		 * Get Longest Edge
		 */
		 // Longer Edge = More time weight
		unordered_map<int32, float> groupIndexToMaxEdgeLength;

		for (const auto& it : groupIndexToConnectedTrisIndices)
		{
			float maxEdgeLength = 0.0f;

			const std::vector<int32>& trisIndices = it.second;

			for (int32 trisIndex : trisIndices)
			{
				int32 index_tris0 = (trisIndex / 3) * 3;
				FVector vert1 = vertexPositions[wedgeIndices[index_tris0]];
				FVector vert2 = vertexPositions[wedgeIndices[index_tris0 + 1]];
				FVector vert3 = vertexPositions[wedgeIndices[index_tris0 + 2]];

				float length1 = FVector::Dist(vert1, vert2);
				float length2 = FVector::Dist(vert2, vert3);
				float length3 = FVector::Dist(vert1, vert3);
				maxEdgeLength = max(maxEdgeLength, max(length1, max(length2, length3)));
			}

			groupIndexToMaxEdgeLength[it.first] = maxEdgeLength;
		}







		/*
		 * Paint
		 */

		float totalEdgeLength = 0;
		for (const auto& it : groupIndexToMaxEdgeLength) {
			PUN_CHECK(it.second > 0);
			totalEdgeLength += it.second;
		}
		
		float currentColorFloat = 0.0f;

		PUN_EDITOR_LOG("meshGroupInfos:%d", meshGroupInfos.size());
		
		for (int i = 0; i < meshGroupInfos.size(); i++) 
		{
			PUN_CHECK(currentColorFloat <= 1.01f);
			
			int32 groupIndex = meshGroupInfos[i].groupIndex;

			if (isPrinting) PUN_EDITOR_LOG("  groupIndex:%d", groupIndex);

			// Color by group
			FColor currentColorVec = FLinearColor(currentColorFloat, currentColorFloat, currentColorFloat, 1.0f).ToFColor(false);
			//FColor currentColorVec(GameRand::DisplayRand(i * 100) % 255, GameRand::DisplayRand(i * 1003) % 255, GameRand::DisplayRand(i * 1030) % 255, 255);
			
			const vector<int32>& trisIndices = groupIndexToConnectedTrisIndices[groupIndex];
			for (int32 trisIndex : trisIndices) {
				rawMesh.WedgeColors[trisIndex] = currentColorVec;

				int32 vertIndex = wedgeIndices[trisIndex];
				if (isPrinting) PUN_EDITOR_LOG("  - tris:%d vertIndex:%d %s", trisIndex, vertIndex, *vertToString(vertIndex));
			}
			
			currentColorFloat += groupIndexToMaxEdgeLength[groupIndex] / totalEdgeLength;
		}

		// Save the new raw mesh.
		mesh->GetSourceModel(PaintingMeshLODIndex).SaveRawMesh(rawMesh);
		
	}

	//FPlatformProcess::Sleep(0.1f);
	
	mesh->Build(true);
	mesh->MarkPackageDirty();


#endif
}

/**
 * Trees
 */

FTileMeshAssets UAssetLoaderComponent::tileMeshAsset(TileObjEnum treeEnum) 
{
	check(_tileMeshes.Contains(static_cast<int32>(treeEnum)));
	return _tileMeshes[static_cast<int32>(treeEnum)];
}

void UAssetLoaderComponent::LoadTree(TileObjEnum treeEnum, std::string trunkMeshFile, std::string leafMeshFile,
										std::string fruitMeshFile, std::string leafLowMeshFile, std::string leafShadowMeshFile, std::string stumpMeshFile)
{
	const std::string path = "/Game/Models/Trees/";
	FTileMeshAssets proto;
	proto.assets.Add(Load<UStaticMesh>((path + trunkMeshFile).c_str()));
	proto.assets.Add(Load<UStaticMesh>((path + leafMeshFile).c_str()));
	proto.assets.Add(Load<UStaticMesh>((path + fruitMeshFile).c_str()));
	proto.assets.Add(Load<UStaticMesh>((path + leafLowMeshFile).c_str()));
	proto.assets.Add(Load<UStaticMesh>((path + leafShadowMeshFile).c_str()));
	proto.assets.Add(Load<UStaticMesh>((path + stumpMeshFile).c_str()));
	
	_tileMeshes.Add(static_cast<int32>(treeEnum), proto);
}

void UAssetLoaderComponent::LoadTileObject(TileObjEnum treeEnum, std::vector<std::string> meshFiles)
{
	const std::string path = "/Game/Models/";
	FTileMeshAssets proto;
	for (int32 i = 0; i < meshFiles.size(); i++) {
		proto.assets.Add(Load<UStaticMesh>((path + meshFiles[i]).c_str()));
	}
	_tileMeshes.Add(static_cast<int32>(treeEnum), proto);
}
