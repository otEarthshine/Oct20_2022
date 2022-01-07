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

	_mainMenuAssetLoader = CreateDefaultSubobject<UMainMenuAssetLoaderComponent>("MainMenuAssetLoaderComponent");

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

	DefenseOverlay_Line = Load<UStaticMesh>("/Game/Models/Placement/DefenseOverlay_Line");
	DefenseOverlay_Node = Load<UStaticMesh>("/Game/Models/Placement/DefenseOverlay_Node");
	DefenseOverlay_CityNode = Load<UStaticMesh>("/Game/Models/Placement/DefenseOverlay_CityNode");
	DefenseOverlay_FortNode = Load<UStaticMesh>("/Game/Models/Placement/DefenseOverlay_FortNode");

	DefenseOverlay_Line_Map = Load<UStaticMesh>("/Game/Models/Placement/DefenseOverlay_Line_Map");
	DefenseOverlay_Node_Map = Load<UStaticMesh>("/Game/Models/Placement/DefenseOverlay_Node_Map");
	DefenseOverlay_CityNode_Map = Load<UStaticMesh>("/Game/Models/Placement/DefenseOverlay_CityNode_Map");
	DefenseOverlay_FortNode_Map = Load<UStaticMesh>("/Game/Models/Placement/DefenseOverlay_FortNode_Map");

	DefenseMaterialInstances.SetNum(DefenseColorEnumSize);
	auto loadDefenseMaterial = [&](DefenseColorEnum colorEnum, const char* path) {
		DefenseMaterialInstances[static_cast<int32>(colorEnum)] = Load<UMaterialInstance>(path);
	};
	loadDefenseMaterial(DefenseColorEnum::Empty,"/Game/Models/Placement/Materials/MI_DefenseOverlay_Line_Empty");
	loadDefenseMaterial(DefenseColorEnum::EnemyProtected, "/Game/Models/Placement/Materials/MI_DefenseOverlay_Line_EnemyProtected");
	loadDefenseMaterial(DefenseColorEnum::EnemyUnprotected, "/Game/Models/Placement/Materials/MI_DefenseOverlay_Line_EnemyUnprotected");
	loadDefenseMaterial(DefenseColorEnum::Protected, "/Game/Models/Placement/Materials/MI_DefenseOverlay_Line_Protected");
	loadDefenseMaterial(DefenseColorEnum::Unprotected, "/Game/Models/Placement/Materials/MI_DefenseOverlay_Line_Unprotected");
	loadDefenseMaterial(DefenseColorEnum::River, "/Game/Models/Placement/Materials/MI_DefenseOverlay_Line_River");

	//

	////UMaterialInstanceDynamic *material = UMaterialInstanceDynamic::Create(NormalBlendMaterial, this);
	//
	//FLinearColor color;
	//MI_DefenseOverlay_Line_Empty->GetVectorParameterValue(TEXT("Color"), color);

	
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


	//BuildingIconMaterial = Load<UMaterial>("/Game/UI/UIMaterials/M_BuildingIcon");

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
	M_Icon = Load<UMaterial>("/Game/UI/ResourceIcons/M_Icon");

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

	M_LogoHover = Load<UMaterial>("/Game/UI/PlayerLogos/M_LogoHover");
	

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
	_factionEnumToBuildingEnumToModuleGroups.SetNum(FactionEnumCount);
	_factionEnumToBuildingEnumToMinEraModel.SetNum(FactionEnumCount);
	for (int32 i = 0; i < FactionEnumCount; i++) {
		_factionEnumToBuildingEnumToModuleGroups[i].SetNum(BuildingEnumCount);
		_factionEnumToBuildingEnumToMinEraModel[i].SetNum(BuildingEnumCount);
	}
	
	LoadBuilding(CardEnum::FruitGatherer, "Fruit_GathererEra", "Fruit_GathererEra", "FruitGatherer", 1);
	LoadBuilding(CardEnum::Bank, "Bank_Era", "Bank_Era", "Bank", 3);

	LoadBuilding(CardEnum::StoneToolsShop, "StoneToolsShop", "StoneToolsShop", "StoneToolShop", 1, 1);
	LoadBuilding(CardEnum::Blacksmith, "Blacksmith_Era", "Blacksmith_Era", "Blacksmith", 2);
	LoadBuilding(CardEnum::MedicineMaker, "MedicineMaker_Era", "MedicineMaker_Era", "MedicineMaker", 2);

	LoadBuilding(CardEnum::BeerBrewery, "Brewery_Era", "BeerBrewery", "BeerBrewery", 1);
	
	LoadBuilding(CardEnum::Chocolatier, "Chocolatier_Era", "Chocolatier", "Chocolatier", 3);

	LoadBuilding(CardEnum::MushroomFarm, "Mushroom_FarmEra", "", "MushroomFarm", 1, 4);

	// TODO: Garden
	LoadBuilding(FactionEnum::Europe, CardEnum::Garden, "Garden_Variation1", "Garden/Variation1");
	LoadBuilding(FactionEnum::Arab, CardEnum::Garden, "Garden_V1", "Garden/V1");
	//TryLoadBuildingModuleSet("Garden2", "Garden/Variation2");
	//TryLoadBuildingModuleSet("Garden3", "Garden/Variation3");

	LoadBuilding(CardEnum::Fisher, "Fishing_Lodge_Era_", "FishingLodge_Era", "FishingLodge", 1);

	LoadBuilding(CardEnum::Winery, "Winery", 2);

	LoadBuilding(CardEnum::Library, "Library_Era", "Library_Era", "Library", 2);
	LoadBuilding(CardEnum::School, "College_Era", "College_Era", "College", 3);

	LoadBuilding(CardEnum::PaperMaker, "PaperMakerEra", "PaperMaker_Era", "PaperMaker", 2, 2);
	
	LoadBuilding(CardEnum::Theatre, "Theatre_Era_", "Theatre_Era_", "Theatre", 3);
	LoadBuilding(CardEnum::Tavern, "TavernEra", "TavernEra", "Tavern", 1);
	LoadBuilding(CardEnum::Tailor, "TailorEra", "Tailor_Era", "Tailor", 2);

	//LoadBuilding(CardEnum::ClayPit, "ClayPit_Era", "", "ClayPit");
	LoadBuildingEras(FactionEnum::Europe, CardEnum::ClayPit, "ClayPit", "ClayPit", 1, 1);
	LinkBuildingEras(FactionEnum::Arab, FactionEnum::Europe, CardEnum::ClayPit, "ClayPit", 1, 1);
	
	LoadBuilding(CardEnum::Potter, "potterEra", "DesertPotte", "Potter", 1);

	LoadBuilding(CardEnum::TradingPort, "TradingPort_Era", "TradingPort_Era", "TradingPort", 1);
	
	LoadBuilding(CardEnum::TradingPost, "TradingPost_Era", "TradingPost_Era", "TradingPost", 1);
	LoadBuilding(CardEnum::TradingCompany, "TradingCompany_Era", "TradingCompany_Era", "TradingCompany", 2);

	//LoadBuilding(CardEnum::CardMaker, "ScholarsOffice_Era", "ScholarsOffice_Era", "ScholarsOffice", 2);
	//LoadBuilding(CardEnum::Archives, "Archives_Era", "Archives_Era", "Archives", 2);
	LoadBuilding(CardEnum::CardMaker, "Archives", 2);

	LoadBuilding(CardEnum::ImmigrationOffice, "Immigration_Office_Era_", "ImmigrationOffice_Era", "ImmigrationOffice", 1);
	
	LoadBuilding(CardEnum::HuntingLodge, "HuntingLodge", 1);

	LoadBuilding(CardEnum::RanchPig, "pigranchERA", "pigranchERA", "Ranch", 1);
	LinkBuildingEras(FactionEnum::Europe, CardEnum::RanchSheep, "pigranchERA", 1);
	LinkBuildingEras(FactionEnum::Europe, CardEnum::RanchCow, "pigranchERA", 1);
	LinkBuildingEras(FactionEnum::Arab, CardEnum::RanchSheep, "pigranchERA", 1);
	LinkBuildingEras(FactionEnum::Arab, CardEnum::RanchCow, "pigranchERA", 1);

	LoadBuilding(CardEnum::GoldSmelter, "GoldSmelter_Era", "Gold_Smelter_Era", "SmelterGold", 2);
	
	LoadBuilding(CardEnum::Mint, "Mint_Era", "Mint_Era", "Mint", 3);
	LoadBuilding(CardEnum::Jeweler, "Jeweler_Era", "Jeweler_Era", "Jeweler", 4);
	LoadBuilding(CardEnum::CandleMaker, "CandleMaker_Era", "", "CandleMaker", 2);

	LoadBuilding(CardEnum::CottonMill, "CottonMill_Era", "CottonMill_Era", "TextileMill", 4);
	LoadBuilding(CardEnum::PrintingPress, "PrintingPress_Era", "PrintingPress_Era", "PrintingPress", 4);

	LoadBuilding(CardEnum::Warehouse, "wherehouse_era", "Warehouse_Era", "Warehouse", 2);
	LoadBuilding(CardEnum::ShippingDepot, "LogisticsSenderOfficeERA", "LogisticsSenderOffice_Era", "LogisticsSender", 3);
	LoadBuilding(CardEnum::HaulingServices, "HaulingServicecERA", "HaulingService_Era", "HaulingServices", 2);

	LoadBuilding(CardEnum::MagicMushroomFarm, "ShroomFarmEra", "", "MagicMushroomFarm", 3);

	LoadBuilding(CardEnum::VodkaDistillery, "Vodka", "", "VodkaDistillery", 2);
	LoadBuilding(CardEnum::TequilaDistillery, "", "Vodka", "VodkaDistillery", 2);

	LoadBuilding(CardEnum::CoffeeRoaster, "CoffeeRoaster_Era", "CoffeeRoaster", "CoffeeRoaster", 3);

	LoadBuilding(CardEnum::Granary, "GRANARY_ERA", "Granary", "Granary", 2);

	LoadBuilding(CardEnum::SandMine, "SandMine_Era", "SandMine_Era", "SandMine", 3);
	LoadBuilding(CardEnum::GlassSmelter, "GlassSmelter_Era", "GlassSmelter_Era", "GlassSmelter", 3);
	LoadBuilding(CardEnum::Glassworks, "GlassWorks_Era", "GlassWorks_Era", "GlassWorks", 3);
	LoadBuilding(CardEnum::ConcreteFactory, "concretefactoryERA", "Concretefactory_Era", "ConcreteFactory", 4);
	
	LoadBuilding(CardEnum::CoalPowerPlant, "coalpowerplant", "CoalPowerPlant_Era", "CoalPowerPlant", 4);
	LoadBuilding(CardEnum::IndustrialIronSmelter, "IndustrialIronSmelterERA", "IndustrialIronSmelterERA", "IndustrialIronSmelter", 4);
	LoadBuilding(CardEnum::Steelworks, "SteelworksEra", "SteelWork_Era", "Steelworks", 4);
	LoadBuilding(CardEnum::OilRig, "OilWell_Era", "OilWell_Era", "OilWell", 4);
	LoadBuilding(CardEnum::OilPowerPlant, "OilPowerPlantERA", "OilPowerPlant_Era", "OilPowerPlant", 4);
	LoadBuilding(CardEnum::PaperMill, "PaperMill_PaperMill_Era", "PaperMill", "PaperMill", 4);
	LoadBuilding(CardEnum::ClockMakers, "Clock_Maker_Era", "ClockMaker_ClockMakerEra", "ClockMaker", 4);

	LoadBuilding(FactionEnum::Europe, CardEnum::Cathedral, "Cathedral_Era2", "Cathedral");
	
	LoadBuilding(FactionEnum::Europe, CardEnum::GrandPalace, "Grand_Museum_Era_", "GrandMuseum/Era4");

	
	LoadBuilding(FactionEnum::Europe, CardEnum::ExhibitionHall, "Crystal_Palace", "CrystalPalace", ModuleTransformGroup::CreateAuxSet(
		{}, {}, {},
		{
			{0.4f, 70.0f, FLinearColor(1, 0.651, 0.246), FVector(-40, 0, 20), FVector::OneVector},
			{0.4f, 70.0f, FLinearColor(1, 0.651, 0.246), FVector(70, 0, 20), FVector::OneVector}
		}
	));
	
	LoadBuilding(FactionEnum::Europe, CardEnum::Castle, "Castle", "Castle");


	LoadBuilding(CardEnum::StatisticsBureau, "Statistic_Bureau_Era_", "StatisticBureau_Era", "StatisticsBureau", 1);
	LoadBuilding(CardEnum::JobManagementBureau, "Employment_Bureau_Era_", "EmploymentBureau_Era", "EmploymentBureau", 1);


	LoadBuilding(CardEnum::IntercityLogisticsHub, "IntercityLogisticsHub", 0);
	LoadBuilding(CardEnum::IntercityLogisticsPort, "IntercityLogisticsPort", 0);


	LoadBuildingEras(FactionEnum::Europe, CardEnum::Forester, "Forester", "Foresterv2",  1);
	LoadBuildingEras(FactionEnum::Arab, CardEnum::Forester, "Forester", "Forester", 1);
	//LoadBuildingEras(FactionEnum::Arab, CardEnum::Windmill, "Windmill", "Windmill", 1);

	LoadBuilding(CardEnum::Fort, "Fortress", 1, 3);

	LoadBuilding(CardEnum::WorldTradeOffice, "WorldTradeOffice", 4);
	
	LoadBuilding(CardEnum::ForeignQuarter, "ForeignQuarter", 3);
	LoadBuilding(CardEnum::ForeignPort, "ForeignPort", 3);
	LoadBuilding(CardEnum::SpyCenter, "SpyCenter", 2);

	LoadBuilding(CardEnum::Zoo, "Zoo", 3);


	

	// TODO: EU era 2,3,4
	LoadBuilding(CardEnum::Market, "Market", 2, 2);

	LoadBuilding(CardEnum::Caravansary, "Caravansary", 2);
	LoadBuildingEras(FactionEnum::Arab, CardEnum::Hotel, "Hotel", "Hotel", 3);


	LoadBuilding(FactionEnum::Arab, CardEnum::Mosque, "Mosque", "Mosque/Era2");
	LoadBuilding(FactionEnum::Arab, CardEnum::SultansCastle, "SultansCastle", "Castle/Era3");
	LoadBuilding(FactionEnum::Arab, CardEnum::SultansPalace, "SultansPalace", "TajMahal/Era4");
	LoadBuildingEras(FactionEnum::Arab, CardEnum::GreatMosque, "GreatMosque", "GreatMosque", 4);

	

	LoadBuilding(CardEnum::Embassy, "Embassy", 2);

	LoadBuilding(CardEnum::Museum, "Museum", 3);

	LoadBuilding(CardEnum::PolicyOffice, "PolicyOffice", 3);
	
	LoadBuilding(CardEnum::CardCombiner, "ScholarsOffice", 3);

	LoadBuilding(FactionEnum::Europe, CardEnum::MinorCity, "MinorCity", "MinorCity/Era0", ModuleTransformGroup::CreateAuxSet(
		{
			{ParticleEnum::CampFire, TransformFromPositionYawScale(-5.4, -0.82, 0.62, 0, 0.17)}
		},
		{}, {},
		{{0.12f, 35.0f, FLinearColor(1, 0.527f, 0.076f), FVector(-5.4, -0.82, 8.5), FVector::OneVector}}
	));
	LoadBuildingEras(FactionEnum::Europe, CardEnum::MinorCity, "MinorCity", "MinorCity", 1, 4);

	LoadBuilding(FactionEnum::Arab, CardEnum::MinorCity, "MinorCity", "MinorCity/Era0", ModuleTransformGroup::CreateAuxSet(
		{
			{ParticleEnum::CampFire, TransformFromPositionYawScale(-5.4, -0.82, 0.62, 0, 0.17)}
		},
		{}, {},
		{ {0.12f, 35.0f, FLinearColor(1, 0.527f, 0.076f), FVector(-5.4, -0.82, 8.5), FVector::OneVector} }
	));
	LoadBuildingEras(FactionEnum::Arab, CardEnum::MinorCity, "MinorCity", "MinorCity", 1, 4);

	
	//LinkBuilding(FactionEnum::Europe, CardEnum::,)
	//CardEnum::ResourceOutpost
	LoadBuilding(CardEnum::ResourceOutpost, "ResourceOutpost", 3, 4);
	

	LoadBuilding(CardEnum::MinorCityPort, "MinorCityPort", 0,4);
	
	/*
	 * Townhall
	 */
	//! Europe
	LoadBuilding(FactionEnum::Europe, CardEnum::Townhall, "Townhall_Era0", "Townhall/Era0", ModuleTransformGroup::CreateAuxSet(
		{
			{ParticleEnum::CampFire, TransformFromPositionYawScale(-5.65, -.669, 1, 0, 0.17)}
		},
		{}, {},
		{ {0.12f, 35.0f, FLinearColor(1, 0.527f, 0.076f), FVector(-5.65, -.669, 5), FVector::OneVector} }
	));
	LoadBuilding(FactionEnum::Europe, CardEnum::Townhall, "Townhall_Era1", "Townhall/Era1", ModuleTransformGroup::CreateAuxSet(
		{
			{ParticleEnum::CampFire, TransformFromPositionYawScale(-10.7, 14.6, 1.85, 0, 0.17)}
		},
		{}, {},
		{ {0.12f, 35.0f, FLinearColor(1, 0.527f, 0.076f), FVector(-10.7, 14.6, 8.5), FVector::OneVector} }
	));
	LoadBuilding(FactionEnum::Europe, CardEnum::Townhall, "Townhall_Era2", "Townhall/Era2", ModuleTransformGroup::CreateAuxSet(
		{
			//{ParticleEnum::Smoke, TransformFromPosition(51.6, -24.3, 38.8)},
			{ParticleEnum::CampFire, TransformFromPositionYawScale(-15.38, 6.41, 1.85, 0, 0.17)}
		},
		{}, {},
		{ {0.12f, 35.0f, FLinearColor(1, 0.527f, 0.076f), FVector(-15.38, 6.41, 1.85), FVector::OneVector} }
	));
	LoadBuilding(FactionEnum::Europe, CardEnum::Townhall, "Townhall_Era3", "Townhall/Era3", ModuleTransformGroup::CreateAuxSet(
		{},
		{}, {},
		{ {0.12f, 35.0f, FLinearColor(1, 0.527f, 0.076f), FVector(-2.3, 10.2, 8.5), FVector::OneVector} }
	));
	LoadBuilding(FactionEnum::Europe, CardEnum::Townhall, "Townhall_Era4", "Townhall/Era4", ModuleTransformGroup::CreateAuxSet(
		{},
		{}, {},
		{ {0.12f, 35.0f, FLinearColor(1, 0.527f, 0.076f), FVector(-2.3, 10.2, 8.5), FVector::OneVector} }
	));

	//! Arab
	LoadBuilding(FactionEnum::Arab, CardEnum::Townhall, "Townhall_Era0", "Townhall/Era0", ModuleTransformGroup::CreateAuxSet(
		{
			{ParticleEnum::CampFire, TransformFromPositionYawScale(-0.62, -10, 2.6, 0, 0.17)}
		},
		{}, {},
		{ {0.12f, 35.0f, FLinearColor(1, 0.527f, 0.076f), FVector(-0.62, -10, 6.6), FVector::OneVector} }
	));
	LoadBuilding(FactionEnum::Arab, CardEnum::Townhall, "Townhall_Era1", "Townhall/Era1", ModuleTransformGroup::CreateAuxSet(
		{
			{ParticleEnum::CampFire, TransformFromPositionYawScale(1, 1, 4, 0, 0.17)}
		},
		{}, {},
		{ {0.12f, 35.0f, FLinearColor(1, 0.527f, 0.076f), FVector(1, 1, 10.5), FVector::OneVector} }
	));
	LoadBuilding(FactionEnum::Arab, CardEnum::Townhall, "Townhall_Era2", "Townhall/Era2", ModuleTransformGroup::CreateAuxSet(
		{
			//{ParticleEnum::Smoke, TransformFromPosition(51.6, -24.3, 38.8)},
			//{ParticleEnum::CampFire, TransformFromPositionYawScale(-15.38, 6.41, 1.85, 0, 0.17)}
		},
		{}, {},
		{ /*{0.12f, 35.0f, FLinearColor(1, 0.527f, 0.076f), FVector(-15.38, 6.41, 1.85), FVector::OneVector}*/ }
	));
	LoadBuilding(FactionEnum::Arab, CardEnum::Townhall, "Townhall_Era3", "Townhall/Era3", ModuleTransformGroup::CreateAuxSet(
		{},
		{}, {},
		{/* {0.12f, 35.0f, FLinearColor(1, 0.527f, 0.076f), FVector(-2.3, 10.2, 8.5), FVector::OneVector}*/ }
	));
	LoadBuilding(FactionEnum::Arab, CardEnum::Townhall, "Townhall_Era4", "Townhall/Era4", ModuleTransformGroup::CreateAuxSet(
		{},
		{}, {},
		{ /*{0.12f, 35.0f, FLinearColor(1, 0.527f, 0.076f), FVector(-2.3, 10.2, 8.5), FVector::OneVector}*/ }
	));



	//! Colonies
	LinkBuilding(FactionEnum::Europe, CardEnum::Colony, "Townhall_Era0");
	LinkBuilding(FactionEnum::Europe, CardEnum::PortColony, "Townhall_Era0");
	LinkBuilding(FactionEnum::Arab, CardEnum::Colony, "Townhall_Era0");
	LinkBuilding(FactionEnum::Arab, CardEnum::PortColony, "Townhall_Era0");

	LoadBuilding(CardEnum::FurnitureWorkshop, "Furniture_Workshop_Era", "FurnitureWorkshop_Era", "FurnitureWorkshop", 1);

	/*
	 * Charcoal Burner
	 */
	//! Europe
	LoadBuilding(FactionEnum::Europe, CardEnum::CharcoalMaker, "CharcoalMakerEra1", "CharcoalBurner/Era1", ModuleTransformGroup::CreateAuxSet(
		{
			{ParticleEnum::BlackSmoke,  TransformFromPosition(3.6, 7.92, 9.0)}, // 8.2 -> 5.6 ... decrease by 2.6
			{ParticleEnum::TorchFire,  FTransform(FRotator::ZeroRotator, FVector(3.6, 7.92, 11.5), FVector(1, 1, 1))}
		}
	));
	LoadBuilding(FactionEnum::Europe, CardEnum::CharcoalMaker, "CharcoalMakerEra2", "CharcoalBurner/Era2", ModuleTransformGroup::CreateAuxSet(
		{
			{ParticleEnum::BlackSmoke,  TransformFromPosition(3.6, 8.53, 9.0)}, // 8.2 -> 5.6 ... decrease by 2.6
			{ParticleEnum::TorchFire,  FTransform(FRotator::ZeroRotator, FVector(3.6, 8.53, 11.5), FVector(1, 1, 1))}
		}
	));
	LoadBuilding(FactionEnum::Europe, CardEnum::CharcoalMaker, "CharcoalMakerEra3", "CharcoalBurner/Era3", ModuleTransformGroup::CreateAuxSet(
		{
			{ParticleEnum::BlackSmoke,  TransformFromPosition(3.6, 8.3, 20.0)}, // 8.2 -> 5.6 ... decrease by 2.6
			{ParticleEnum::TorchFire,  FTransform(FRotator::ZeroRotator, FVector(3.6, 8.3, 22.6), FVector(1, 1, 1))}
		}
	));
	LoadBuilding(FactionEnum::Europe, CardEnum::CharcoalMaker, "CharcoalMakerEra4", "CharcoalBurner/Era4", ModuleTransformGroup::CreateAuxSet(
		{
			{ParticleEnum::BlackSmoke,  TransformFromPosition(3.3, 8.37, 21.3)}, // 8.2 -> 5.6 ... decrease by 2.6
			{ParticleEnum::TorchFire,  FTransform(FRotator::ZeroRotator, FVector(3.3, 8.37, 23.9), FVector(1, 1, 1))}
		}
	));

	//! Arab
	LoadBuilding(FactionEnum::Arab, CardEnum::CharcoalMaker, "CharcoalMakerEra1", "CharcoalMaker/Era1", ModuleTransformGroup::CreateAuxSet(
		{
			{ParticleEnum::BlackSmoke,  TransformFromPosition(3.6, 7.92, 9.0)}, // 8.2 -> 5.6 ... decrease by 2.6
			{ParticleEnum::TorchFire,  FTransform(FRotator::ZeroRotator, FVector(3.6, 7.92, 11.5), FVector(1, 1, 1))}
		}
	));
	LoadBuilding(FactionEnum::Arab, CardEnum::CharcoalMaker, "CharcoalMakerEra2", "CharcoalMaker/Era2", ModuleTransformGroup::CreateAuxSet(
		{
			{ParticleEnum::BlackSmoke,  TransformFromPosition(3.6, 8.53, 9.0)}, // 8.2 -> 5.6 ... decrease by 2.6
			{ParticleEnum::TorchFire,  FTransform(FRotator::ZeroRotator, FVector(3.6, 8.53, 11.5), FVector(1, 1, 1))}
		}
	));
	LoadBuilding(FactionEnum::Arab, CardEnum::CharcoalMaker, "CharcoalMakerEra3", "CharcoalMaker/Era3", ModuleTransformGroup::CreateAuxSet(
		{
			{ParticleEnum::BlackSmoke,  TransformFromPosition(3.6, 8.3, 20.0)}, // 8.2 -> 5.6 ... decrease by 2.6
			{ParticleEnum::TorchFire,  FTransform(FRotator::ZeroRotator, FVector(3.6, 8.3, 22.6), FVector(1, 1, 1))}
		}
	));
	LoadBuilding(FactionEnum::Arab, CardEnum::CharcoalMaker, "CharcoalMakerEra4", "CharcoalMaker/Era4", ModuleTransformGroup::CreateAuxSet(
		{
			{ParticleEnum::BlackSmoke,  TransformFromPosition(3.3, 8.37, 21.3)}, // 8.2 -> 5.6 ... decrease by 2.6
			{ParticleEnum::TorchFire,  FTransform(FRotator::ZeroRotator, FVector(3.3, 8.37, 23.9), FVector(1, 1, 1))}
		}
	));

	
	LoadBuilding(CardEnum::Brickworks, "BrickworkEra", "brickwork_Era", "Brickworks", 2);
	
	LoadBuilding(CardEnum::Beekeeper, "BeeKeeper_Era", "", "Beekeeper", 2);

	LoadBuilding(FactionEnum::Europe, CardEnum::IrrigationReservoir, "IrrigationReservoir", "IrrigationReservoir");

	//--
	
	LoadBuilding(CardEnum::Bakery, "Bakery_Era", "", "Bakery", 2);
	LoadBuilding(CardEnum::PitaBakery, "", "PitaBakery_Era", "Bakery", 2);
	LoadBuilding(CardEnum::IronSmelter, "Iron_Smelter_Era", "Iron_Smelter_Era", "IronSmelter", 2, 2);

	LoadBuildingEras(FactionEnum::Arab, CardEnum::IrrigationPump, "IrrigationPump", "IrrigationPump", 2);
	LoadBuildingEras(FactionEnum::Europe, CardEnum::IrrigationPump, "IrrigationPump", "IrrigationPump", 3);

	LoadBuilding(CardEnum::Windmill, "Windmill", 2, 2);

	/*
	 * Irrigation
	 */
	for (int32 era = 2; era <= 2; era++) {
		for (int32 i = 1; i <= 6; i++) {
			FString moduleGroupName = "IrrigationDitch_Era" + FString::FromInt(era) + "_0" + FString::FromInt(i);
			FString moduleGroupFolderName = "IrrigationDitch/Era" + FString::FromInt(era) + "/V" + FString::FromInt(i);
			LoadBuilding(FactionEnum::Arab, CardEnum::IrrigationDitch, moduleGroupName, moduleGroupFolderName);
			LinkBuilding(FactionEnum::Europe, CardEnum::IrrigationDitch, moduleGroupName, _lastTempAuxGroup);
		}
	}
	
	

	
	LoadBuilding(CardEnum::Quarry, "Quarry_Era", "Quarry_Era", "Quarry", 1);

	/*
	 * Ore Mine
	 */
	{
		for (int32 factionInt = 0; factionInt < FactionEnumCount; factionInt++)
		{
			FactionEnum factionEnum = static_cast<FactionEnum>(factionInt);
			
			for (int32 eraInt = 1; eraInt <= 4; eraInt++)
			{
				FString era = FString::FromInt(eraInt);
				LoadBuilding(factionEnum, CardEnum::GoldMine, "Ore_Mine_Era" + era, "OreMine/Era" + era);
				LinkBuilding(factionEnum, CardEnum::IronMine, "Ore_Mine_Era" + era, _lastTempAuxGroup);
				LinkBuilding(factionEnum, CardEnum::GemstoneMine, "Ore_Mine_Era" + era, _lastTempAuxGroup);
				LinkBuilding(factionEnum, CardEnum::CoalMine, "Ore_Mine_Era" + era, _lastTempAuxGroup);

				// For each Mine type... Add the according mesh group
				// (Manually compose transforms/togglableTransforms)
				{
					//PUN_LOG("_recentlyAddedModuleNames %d", _recentlyAddedModuleNames.Num());

					auto manualAddTransforms = [&](CardEnum buildingEnum, FString searchString)
					{
						for (int32 i = _recentlyAddedModuleNames.Num(); i-- > 0;) {
							FString recentModuleName = _recentlyAddedModuleNames[i].ToString();
							if (recentModuleName.Contains("_Manual_"))
							{
								if (recentModuleName.Contains(searchString)) {
									_factionEnumToBuildingEnumToModuleGroups[static_cast<int>(factionEnum)][static_cast<int>(buildingEnum)][eraInt - 1].transforms.push_back(ModuleTransform(_recentlyAddedModuleNames[i]));
								}
							}
							else if (recentModuleName.Contains("_ManualToggle_"))
							{
								if (recentModuleName.Contains(searchString)) {
									_factionEnumToBuildingEnumToModuleGroups[static_cast<int>(factionEnum)][static_cast<int>(buildingEnum)][eraInt - 1].togglableTransforms.push_back(ModuleTransform(_recentlyAddedModuleNames[i]));
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
		}
	}

	/*
	 * Load Unique Buildings
	 */
	
	LoadBuildingEras(FactionEnum::Arab, CardEnum::CarpetWeaver, "CarpetWeaver", "CarpetWeaver", 3);
	


	// ---
	
	TryLoadBuildingModuleSet_Old(FactionEnum::Europe, "Forester", "Forester");

	
	TryLoadBuildingModuleSet_Old(FactionEnum::Europe, "StorageYard", "StorageYard");

	TryLoadBuildingModuleSet_Old(FactionEnum::Europe, "FlowerBed", "FlowerBed");
	TryLoadBuildingModuleSet_Old(FactionEnum::Europe, "GardenCypress", "GardenCypress");
	TryLoadBuildingModuleSet_Old(FactionEnum::Europe, "GardenShrubbery1", "GardenShrubbery1");

	//TryLoadBuildingModuleSet_Old(FactionEnum::Europe, "IntercityLogisticsHub", "IntercityLogisticsHub");
	//TryLoadBuildingModuleSet_Old(FactionEnum::Europe, "IntercityLogisticsPort", "IntercityLogisticsPort");
	


	// Animal modules
	LoadBuilding(FactionEnum::Europe, CardEnum::BoarBurrow, "BoarBurrow", "BoarBurrow");

	/*
	 * Houses (European)
	 */
	// Note: Model count starts with House Lvl 0 (legacy imports, not worth the change)

	// 5 Variations per house lvl
	
	// Lvl 0 (1)
	for (int32 i = 1; i <= 3; i++) {
		FString moduleGroupName = "HouseERA0V" + FString::FromInt(i);
		FString moduleGroupFolderName = "House/Level0/V" + FString::FromInt(i);
		LoadBuilding(FactionEnum::Europe, CardEnum::House, moduleGroupName, moduleGroupFolderName);
	}
	TryLoadBuildingModuleSet_Old(FactionEnum::Europe, "HouseClayLvl1", "HouseClayLvl1");
	LinkBuilding(FactionEnum::Europe, CardEnum::House, "HouseClayLvl1");
	LinkBuilding(FactionEnum::Europe, CardEnum::House, "HouseClayLvl1");

	// Lvl 1 (2)
	for (int32 i = 1; i <= 3; i++) {
		FString moduleGroupName = "House_Lvl1_Var" + FString::FromInt(i);
		FString moduleGroupFolderName = "House/Level1/V" + FString::FromInt(i);
		LoadBuilding(FactionEnum::Europe, CardEnum::House, moduleGroupName, moduleGroupFolderName);
	}
	LinkBuilding(FactionEnum::Europe, CardEnum::House, "HouseClayLvl1");
	LinkBuilding(FactionEnum::Europe, CardEnum::House, "HouseClayLvl1"); // Buffer

	// Lvl 2 (3)
	for (int32 i = 1; i <= 4; i++) {
		FString moduleGroupName = "House_LV2_V" + FString::FromInt(i);
		FString moduleGroupFolderName = "House/Level2/V" + FString::FromInt(i);
		LoadBuilding(FactionEnum::Europe, CardEnum::House, moduleGroupName, moduleGroupFolderName);
	}
	TryLoadBuildingModuleSet_Old(FactionEnum::Europe, "HouseClayLvl2", "HouseClayLvl2");
	LinkBuilding(FactionEnum::Europe, CardEnum::House, "HouseClayLvl2");

	// Lvl 3 (4)
	for (int32 i = 1; i <= 4; i++) {
		FString moduleGroupName = "House_LV3_V" + FString::FromInt(i);
		FString moduleGroupFolderName = "House/Level3/V" + FString::FromInt(i);
		LoadBuilding(FactionEnum::Europe, CardEnum::House, moduleGroupName, moduleGroupFolderName);
	}
	TryLoadBuildingModuleSet_Old(FactionEnum::Europe, "HouseClayLvl3", "HouseClayLvl3");
	LinkBuilding(FactionEnum::Europe, CardEnum::House, "HouseClayLvl3");

	

	// Lvl 4 (5)
	for (int32 i = 1; i <= 4; i++) {
		FString moduleGroupName = "House_LV5_V" + FString::FromInt(i);
		FString moduleGroupFolderName = "House/Level5/V" + FString::FromInt(i);
		LoadBuilding(FactionEnum::Europe, CardEnum::House, moduleGroupName, moduleGroupFolderName);
	}
	LinkBuilding(FactionEnum::Europe, CardEnum::House, "HouseClayLvl1"); // Buffer

	// Lvl 5 (6)
	for (int32 i = 1; i <= 3; i++) {
		FString moduleGroupName = "House_LV6_V" + FString::FromInt(i);
		FString moduleGroupFolderName = "House/Level6/V" + FString::FromInt(i);
		LoadBuilding(FactionEnum::Europe, CardEnum::House, moduleGroupName, moduleGroupFolderName);
	}
	LinkBuilding(FactionEnum::Europe, CardEnum::House, "HouseClayLvl1"); // Buffer
	LinkBuilding(FactionEnum::Europe, CardEnum::House, "HouseClayLvl1"); // Buffer

	// Lvl 6 (7)
	for (int32 i = 1; i <= 2; i++) {
		FString moduleGroupName = "House_LV7_V" + FString::FromInt(i);
		FString moduleGroupFolderName = "House/Level7/V" + FString::FromInt(i);
		LoadBuilding(FactionEnum::Europe, CardEnum::House, moduleGroupName, moduleGroupFolderName);
	}
	LinkBuilding(FactionEnum::Europe, CardEnum::House, "HouseClayLvl1"); // Buffer
	LinkBuilding(FactionEnum::Europe, CardEnum::House, "HouseClayLvl1"); // Buffer
	LinkBuilding(FactionEnum::Europe, CardEnum::House, "HouseClayLvl1"); // Buffer

		// Lvl 7 (8)
	for (int32 i = 1; i <= 2; i++) {
		FString moduleGroupName = "House_LV8_V" + FString::FromInt(i);
		FString moduleGroupFolderName = "House/Level8/V" + FString::FromInt(i);
		LoadBuilding(FactionEnum::Europe, CardEnum::House, moduleGroupName, moduleGroupFolderName);
	}
	LinkBuilding(FactionEnum::Europe, CardEnum::House, "HouseClayLvl1"); // Buffer
	LinkBuilding(FactionEnum::Europe, CardEnum::House, "HouseClayLvl1"); // Buffer
	LinkBuilding(FactionEnum::Europe, CardEnum::House, "HouseClayLvl1"); // Buffer
	

	/*
	 * Houses (Arab)
	 */
	const int32 maxHouseVariation = 5;

	auto loadHouseSet = [&](FactionEnum factionEnum, int32 lvlCount, int32 variationCount)
	{
		for (int32 i = 1; i <= variationCount; i++) {
			FString moduleGroupName = "HouseLv" + FString::FromInt(lvlCount) + "_0" + FString::FromInt(i);
			FString moduleGroupFolderName = "House/Lv" + FString::FromInt(lvlCount) + "/V" + FString::FromInt(i);
			LoadBuilding(factionEnum, CardEnum::House, moduleGroupName, moduleGroupFolderName);
		}
		for (int32 i = variationCount + 1; i <= maxHouseVariation; i++) {
			LinkBuilding(factionEnum, CardEnum::House, "HouseClayLvl1");
		}
	};

	loadHouseSet(FactionEnum::Arab, 1, 4);
	loadHouseSet(FactionEnum::Arab, 2, 3);
	loadHouseSet(FactionEnum::Arab, 3, 3);
	loadHouseSet(FactionEnum::Arab, 4, 3);
	loadHouseSet(FactionEnum::Arab, 5, 3);
	loadHouseSet(FactionEnum::Arab, 6, 3);
	loadHouseSet(FactionEnum::Arab, 7, 5);
	loadHouseSet(FactionEnum::Arab, 8, 4);


	/*
	 * Arab Special
	 */
	
	LoadBuilding(FactionEnum::Arab, CardEnum::Oasis, "Oasis", "Oasis");

	
	LoadBuilding(FactionEnum::Arab, CardEnum::EasterIsland, "EasterIsland", "EasterIsland");
	LoadBuilding(FactionEnum::Arab, CardEnum::EgyptianPyramid, "EgyptianPyramid", "EgyptianPyramid");
	LoadBuilding(FactionEnum::Arab, CardEnum::MayanPyramid, "MayanPyramid", "MayanPyramid");
	LoadBuilding(FactionEnum::Arab, CardEnum::StoneHenge, "StoneHenge", "StoneHenge");
	
	
	/*
	 * Construction
	 */
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
	TryLoadBuildingModuleSet_Old(FactionEnum::Europe, "TownhallThatch", "TownhallThatch");
	TryLoadBuildingModuleSet_Old(FactionEnum::Europe, "Townhall0", "TownhallModules");
	TryLoadBuildingModuleSet_Old(FactionEnum::Europe, "Townhall3", "TownhallLvl3");
	TryLoadBuildingModuleSet_Old(FactionEnum::Europe, "Townhall4", "TownhallLvl3");
	LoadModule("TownhallLvl5GardenAndSpire", "TownhallLvl5/TownhallLvl5GardenAndSpire");
	
	//PUN_CHECK(moduleMesh("Townhall3Special2"));


	//
	//TryLoadBuildingModuleSet("PaperMaker", "PaperMaker");

	TryLoadBuildingModuleSet_Old(FactionEnum::Europe, "Ministry", "Ministry");

	// Barrack Modules
	//TryLoadBuildingModuleSet("Barrack", "Barrack");

	//TryLoadBuildingModuleSet("Bakery", "Bakery");
	//TryLoadBuildingModuleSet_Old(FactionEnum::Europe, "Windmill", "Windmill");

	//TryLoadBuildingModuleSet("Beekeeper", "Beekeeper");
	//TryLoadBuildingModuleSet("Brickworks", "Brickworks");

	TryLoadBuildingModuleSet_Old(FactionEnum::Europe, "Colony", "Colony");
	TryLoadBuildingModuleSet_Old(FactionEnum::Europe, "Outpost", "Outpost");
	TryLoadBuildingModuleSet_Old(FactionEnum::Europe, "InventorsWorkshop", "InventorsWorkshop");


	TryLoadBuildingModuleSet_Old(FactionEnum::Europe, "ChichenItza", "ChichenItza");

	//TryLoadBuildingModuleSet("Market", "Market");
	//TryLoadBuildingModuleSet("IrrigationReservoir", "IrrigationReservoir");
	

	//TryLoadBuildingModuleSet_Old(FactionEnum::Europe, "TribalVillage", "RegionTribalVillage");
	TryLoadBuildingModuleSet_Old(FactionEnum::Europe, "AncientShrine", "RegionShrine");
	TryLoadBuildingModuleSet_Old(FactionEnum::Europe, "PortVillage", "RegionPortVillage");
	TryLoadBuildingModuleSet_Old(FactionEnum::Europe, "RegionCratePile", "RegionCratePile");


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

	// Farm
	LoadModule("FarmTile", "Farm/FarmTile");
	LoadModule("FarmTile_UC", "Farm/FarmTile_UC");

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
//	auto addBuildIcon = [&](CardEnum cardEnum, FString iconName, FString iconAlphaName, bool isSpecial)
//	{
//		if (isSpecial) {
//			_buildingsUsingSpecialIcon.Add(cardEnum);
//		}
//		_buildingIcons.Add(LoadF<UTexture2D>(FString("/Game/UI/GeneratedIcons/") + iconName));
//		_buildingIconsAlpha.Add(LoadF<UTexture2D>(FString("/Game/UI/GeneratedIcons/") + iconAlphaName));
//	};
//
//	// TODO: Icons for all Cards...
//	for (int i = 0; i < BuildingEnumCount; i++) 
//	{
//		// Special cases
//		FString fileName = FString("BuildingIcon");
//		FString fileNameAlpha = FString("BuildingIcon");
//
//		CardEnum buildingEnum = static_cast<CardEnum>(i);
//		switch(buildingEnum)
//		{
//#define CASE(enumName)	case CardEnum::##enumName: addBuildIcon(CardEnum::##enumName, FString(TO_STR(enumName##Icon)), FString("SpecialIconAlpha"), true); break;
//			CASE(Farm);
//			CASE(DirtRoad);
//			CASE(StoneRoad);
//			CASE(Fence);
//			CASE(FenceGate);
//			CASE(Bridge);
//			CASE(Tunnel);
//			CASE(StorageYard);
//
//			case CardEnum::IntercityRoad: addBuildIcon(CardEnum::IntercityRoad, FString(TO_STR(DirtRoadIcon)), FString("SpecialIconAlpha"), true); break;
//			case CardEnum::IntercityBridge: addBuildIcon(CardEnum::IntercityBridge, FString(TO_STR(BridgeIcon)), FString("SpecialIconAlpha"), true); break;
//
//#undef CASE
//		default:
//			int32 buildingEnumInt = static_cast<int32>(buildingEnum);
//			if (static_cast<int32>(CardEnum::PitaBakery) >= buildingEnumInt && buildingEnumInt >= static_cast<int32>(CardEnum::MinorCity)) {
//				// Temp
//				addBuildIcon(buildingEnum, FString(TO_STR(DirtRoadIcon)), FString("SpecialIconAlpha"), true);
//			} else {
//				addBuildIcon(buildingEnum, FString("BuildingIcon") + FString::FromInt(i), FString("BuildingIconAlpha") + FString::FromInt(i), false);
//			}
//			break;
//		}
//	}

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
	_cardIcons.SetNum(FactionEnumCount * CardEnumCount_WithNone);
	_cardIconHasFadeBorder.SetNum(FactionEnumCount * CardEnumCount_WithNone);
	
	int32 addCardIconCount = 0;
	FString folderName = "CardImages_BleGood/";
	FString suffix = "_1024";
	bool hasFadeBorder = false;
	auto addCardIcon = [&](CardEnum cardEnum, FString iconFileName)
	{
		for (int32 i = 0; i < FactionEnumCount; i++) {
			SetCardIcon(i, cardEnum, LoadF<UTexture2D>(FString("/Game/UI/Images/") + folderName + iconFileName + suffix), hasFadeBorder);
		}
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
	addCardIcon(CardEnum::SpicesSeeds, "SpicesSeeds");
	addCardIcon(CardEnum::AgaveSeeds, "AgaveSeeds");
	addCardIcon(CardEnum::CactusFruitSeeds, "CactusFruitSeeds");
	
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

	addCardIcon(CardEnum::Steal, "StealCard");
	addCardIcon(CardEnum::StealOld, "SnatchCard");

	addCardIcon(CardEnum::MinersFortune, "MinersFortune");


	// Artifacts
	folderName = "CardImages_Artifacts/";
	suffix = "_1024";
	hasFadeBorder = false;
	addCardIcon(CardEnum::Codex, "Codex");
	addCardIcon(CardEnum::SacrificialAltar, "SacrificialAltar");
	addCardIcon(CardEnum::StoneStele, "StoneStele");
	addCardIcon(CardEnum::BallCourtGoals, "BallCourtGoals");
	addCardIcon(CardEnum::DecorativePlate, "DecorativePlate");

	addCardIcon(CardEnum::CanopicJars, "CanopicJars");
	addCardIcon(CardEnum::DepartureScrolls, "DepartureScrolls");
	addCardIcon(CardEnum::DeathMask, "DeathMask");
	addCardIcon(CardEnum::SolarBarque, "SolarBarque");
	addCardIcon(CardEnum::GoldCapstone, "GoldCapstone");

	addCardIcon(CardEnum::FeastRemains, "FeastRemains");
	addCardIcon(CardEnum::ForeignTrinkets, "ForeignTrinkets");
	addCardIcon(CardEnum::ChalkPlaque, "ChalkPlaque");
	addCardIcon(CardEnum::OfferingCup, "OfferingCup");
	addCardIcon(CardEnum::OrnateTrinkets, "OrnateTrinkets");

	addCardIcon(CardEnum::TatooingNeedles, "TatooingNeedles");
	addCardIcon(CardEnum::Petroglyphs, "Petroglyphs");
	addCardIcon(CardEnum::StoneFishhooks, "StoneFishhooks");
	addCardIcon(CardEnum::CoralEyes, "CoralEyes");
	addCardIcon(CardEnum::AncientStaff, "AncientStaff");
	
	// Animals
	folderName = "CardImages_Animals/";
	suffix = "_1024";
	hasFadeBorder = false;
	addCardIcon(CardEnum::BlackBear, "BlackBear");
	addCardIcon(CardEnum::Boar, "Boar");
	addCardIcon(CardEnum::BrownBear, "BrownBear");
	addCardIcon(CardEnum::YellowDeer, "MuleDeer");
	
	addCardIcon(CardEnum::Panda, "Panda");
	addCardIcon(CardEnum::RedDeer, "RedDeer");
	//addCardIcon(CardEnum::Reindeer, "Reindeer");
	addCardIcon(CardEnum::DarkDeer, "SambarDeer");


	// Military
	folderName = "CardImages_Military/";
	suffix = "_Card";
	hasFadeBorder = false;
	addCardIcon(CardEnum::Archer, "Archer");
	addCardIcon(CardEnum::Artillery, "Artillery");
	addCardIcon(CardEnum::Battleship, "Battleship");
	addCardIcon(CardEnum::Cannon, "Cannon");
	addCardIcon(CardEnum::Catapult, "Catapult");
	addCardIcon(CardEnum::Conscript, "Conscript");
	addCardIcon(CardEnum::Frigate, "Frigate");
	addCardIcon(CardEnum::Galley, "Galley");
	addCardIcon(CardEnum::Infantry, "Infantry");
	addCardIcon(CardEnum::Knight, "Knight");
	addCardIcon(CardEnum::MachineGun, "MachineGun");
	addCardIcon(CardEnum::Militia, "Militia");
	addCardIcon(CardEnum::Musketeer, "Musketeer");
	addCardIcon(CardEnum::Swordsman, "Swordsman");
	addCardIcon(CardEnum::Tank, "Tank");
	addCardIcon(CardEnum::Warrior, "Warrior");

	addCardIcon(CardEnum::Raid, "Raid");
	

	// Screenshot images
	folderName = "CardImages_Screenshot/";
	suffix = "";
	hasFadeBorder = true;
	addCardIcon(CardEnum::Farm, "FarmIcon");
	addCardIcon(CardEnum::DirtRoad, "DirtRoadIcon");
	addCardIcon(CardEnum::StoneRoad, "StoneRoadIcon");
	
	addCardIcon(CardEnum::Bridge, "BridgeIcon");
	addCardIcon(CardEnum::Tunnel, "TunnelIcon");
	addCardIcon(CardEnum::StorageYard, "StorageYardIcon");

	addCardIcon(CardEnum::IntercityRoad, "DirtRoadIcon");
	addCardIcon(CardEnum::IntercityBridge, "BridgeIcon");

	

	/*
	 * Add Building Card Icons
	 */
	addCardIconCount = 0;

	auto addBuildingCardIcons = [&](FactionEnum factionEnum, FString folderPath)
	{
		for (CardEnum buildingEnum : SortedNameBuildingEnum)
		{
			FString name = GetBuildingInfo(buildingEnum).nameF().Replace(TEXT(" "), TEXT(""));
			name = name.Replace(TEXT("_"), TEXT(""));
			name = name.Replace(TEXT("'"), TEXT(""));

			if (buildingEnum == CardEnum::CardMaker) {
				name = FString("CardMaker");
			}
			else if (buildingEnum == CardEnum::CardCombiner) {
				name = FString("CardCombiner");
			}
			else if (buildingEnum == CardEnum::Garden) {
				name = FString("Garden1");
			}
			else if (buildingEnum == CardEnum::Colony ||
					buildingEnum == CardEnum::PortColony) 
			{
				name = FString("TownHall");
			}

			FString path = folderPath + name;

			IPlatformFile& platformFile = FPlatformFileManager::Get().GetPlatformFile();
			if (platformFile.FileExists(*(FPaths::ProjectContentDir() + path + FString(".uasset"))))
			{
				UObject* cardIconTextureObj = StaticLoadObject(UTexture2D::StaticClass(), NULL, *(FString("/Game/") + path));
				UTexture2D* cardIconTexture = Cast<UTexture2D>(cardIconTextureObj);

				if (cardIconTexture) {
					SetCardIcon(static_cast<int32>(factionEnum), buildingEnum, cardIconTexture, false);
					addCardIconCount++;
				}
			}
		}
	};

	addBuildingCardIcons(FactionEnum::Europe, "UI/BuildingSnapshots/");
	addBuildingCardIcons(FactionEnum::Arab, "UI/BuildingSnapshotsArab/");
	
	//check(addCardIconCount == 126);

	////! Military Unit
	//auto addSpine = [&](CardEnum cardEnum, FString assetName)
	//{
	//	TArray<FString> foundFiles;

	//	FString findDirectory = FPaths::ProjectContentDir() + FString("UI/Spine/") + assetName + FString("/");
	//	IFileManager::Get().FindFiles(foundFiles, *findDirectory, TEXT(".uasset"));

	//	if (foundFiles.Num() > 0)
	//	{
	//		FString fileName = FPaths::MakeValidFileName(foundFiles[0]);
	//		
	//		//UObject* cardIconTextureObj = StaticLoadObject(UTexture2D::StaticClass(), NULL, *(FString("/Game/UI/Spine/") + path));

	//		//auto getPath = [&](FString spineSuffix) {
	//		//	return FString("/Game/UI/Spine/") + assetName + FString("/KR_Military_") + assetName + spineSuffix;
	//		//};
	//		FSpineAsset asset;
	//		UObject* obj = StaticLoadObject(UObject::StaticClass(), NULL, *(FString("/Game/UI/Spine/") + assetName + FString("/") + fileName));
	//		asset.atlas = Cast<USpineAtlasAsset>(obj);
	//		//asset.atlas = LoadF<USpineAtlasAsset>(getPath(""));
	//		//asset.skeletonData = LoadF<USpineSkeletonDataAsset>(getPath(""));
	//		_spineAssets.Add(static_cast<int32>(cardEnum), asset);
	//	}
	//};

	//addSpine(CardEnum::Archer, "Archer");
	//addSpine(CardEnum::Artillery, "Artillery");
	//addSpine(CardEnum::Battleship, "Battleship");
	//addSpine(CardEnum::Cannon, "Cannon");
	//addSpine(CardEnum::Catapult, "Catapult");
	//addSpine(CardEnum::Conscript, "Conscript");
	//addSpine(CardEnum::Frigate, "Frigate");
	//addSpine(CardEnum::Galley, "Galley");
	//addSpine(CardEnum::Infantry, "Infantry");
	//addSpine(CardEnum::Knight, "Knight");
	//addSpine(CardEnum::MachineGun, "MachineGun");
	//addSpine(CardEnum::Militia, "Militia");
	//addSpine(CardEnum::Musketeer, "Musketeer");
	//addSpine(CardEnum::Swordsman, "Swordsman");
	//addSpine(CardEnum::Tank, "Tank");
	//addSpine(CardEnum::Warrior, "Warrior");

	
	
	//BorealFishing
	//BorealWinterResistant

	// Army Icons
	//auto addArmyIcon = [&](std::string iconName) {
	//	_armyIcons.Add(LoadF<UTexture2D>(FString("/Game/UI/MiscIcons/") + ToFString(iconName)));
	//};
	//addArmyIcon("TowerPNG");
	//addArmyIcon("ClubPNG");
	//addArmyIcon("SwordPNG");
	//addArmyIcon("BowPNG");
	
	/**
	 * Unit
	 */
	_unitEnumToAsset.resize(UnitEnumCount);
	//_unitDisplayEnumToAsset.resize(UnitDisplayEnumCount);
	
	//LoadUnit(UnitEnum::Human, "Human/Man/Man1");
	//LoadUnit(UnitEnum::Human, "Human/Man/Boy1");
	
	LoadUnit(UnitEnum::Alpaca, "Alpaca/Alpaca");
	
	LoadUnit(UnitEnum::Boar, "Boar/Boar_Game", 0);
	LoadUnit(UnitEnum::Boar, "Boar/BoarMini_Game", 1);
	
	LoadUnit(UnitEnum::RedDeer, "Deer/Stag_Game");
	LoadUnit(UnitEnum::YellowDeer, "Deer/StagYellow_Game");
	LoadUnit(UnitEnum::DarkDeer, "Deer/StagDark_Game");

	LoadUnit(UnitEnum::BlackBear, "Bear/BlackBear_Game");
	LoadUnit(UnitEnum::BrownBear, "Bear/BrownBear_Game");
	LoadUnit(UnitEnum::Panda, "Panda/Panda_Game");

	// Proper way to do skel collision
	//LoadUnit(UnitEnum::Hippo, "Human/Man/Man1");
	//LoadUnit(UnitEnum::Penguin, "Human/Man/Man1");

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
	LoadUnitFull(UnitEnum::Human, "Human/CitizenMale/", "CitizenMale", animationFileNames, "Human/CitizenMale/VertexAnim/CitizenMale_VertexAnim3", 0); // "Human/CitizenMale/CitizenMaleStatic"
	LoadUnitAuxMesh(UnitAnimationEnum::ImmigrationCart, "Human/Cart/Cart");
	LoadUnitAuxMesh(UnitAnimationEnum::HaulingCart, "Human/Cart/FrontCart");

	// Adult Female
	LoadUnitFull(UnitEnum::Human, "Human/CitizenFemale/", "CitizenFemale", animationFileNames, "Human/CitizenFemale/VertexAnim/CitizenFemale_VertexAnim", 1); // "Human/CitizenFemale/CitizenFemaleStatic"

	// Child Male
	LoadUnitFull(UnitEnum::Human, "Human/CitizenChildMale/", "CitizenChildMale", animationFileNames, "Human/CitizenChildMale/VertexAnim/CitizenChildMale_VertexAnim", 2);

	// Child Female
	LoadUnitFull(UnitEnum::Human, "Human/CitizenChildFemale/", "CitizenChildFemale", animationFileNames, "Human/CitizenChildFemale/VertexAnim/CitizenChildFemale_VertexAnim", 3);


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

	_maxUnitVariationCount = 0;
	for (int32 i = 0; i < _unitEnumToAsset.size(); i++) {
		_maxUnitVariationCount = std::max(_maxUnitVariationCount, static_cast<int32>(_unitEnumToAsset[i].size()));
	}
	
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
	LoadResource2(ResourceEnum::Steel, "SteelBeam/SteelBeam");
	LoadResource2(ResourceEnum::Glassware, "Glassware/Glassware");
	LoadResource2(ResourceEnum::PocketWatch, "PocketWatch/Pocketwatch");

	LoadResource2(ResourceEnum::PitaBread, "PitaBread/PitaBread");
	LoadResource2(ResourceEnum::Carpet, "PocketWatch/Pocketwatch");
	LoadResource2(ResourceEnum::DateFruit, "Dates/Dates");
	LoadResource2(ResourceEnum::ToiletPaper, "ToiletPaper/ToiletPaper");

	LoadResource2(ResourceEnum::Spices, "Spices/Spices");
	LoadResource2(ResourceEnum::Agave, "Agave/Agave");
	LoadResource2(ResourceEnum::CactusFruit, "CactusFruit/CactusFruit");
	LoadResource2(ResourceEnum::Tequila, "Tequila/Tequila");
	
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
	
	LoadTree(TileObjEnum::DesertDatePalm, "OrangeTrunk", "DesertPlants/DesertDatePalm", "Coconut/Coconut1/CoconutFruits", "DesertPlants/DesertDatePalm", "DesertPlants/DesertDatePalm", "OrangeStump");
	LoadTree(TileObjEnum::DesertGingerbreadTree, "OrangeTrunk", "DesertPlants/DesertGingerbreadTree", defaultFruit, "DesertPlants/DesertGingerbreadTree", "DesertPlants/DesertGingerbreadTree", "OrangeStump");

	
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
	LoadTileObject(TileObjEnum::DesertBush1, {
		"Trees/DesertPlants/DesertBush1",
	});
	LoadTileObject(TileObjEnum::DesertBush2, {
		"Trees/DesertPlants/DesertBush2",
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

	// November 2021
	LoadTileObject(TileObjEnum::Spices, {
		"Trees/Spices/SpicesV1",
		"Trees/Spices/SpicesV2",
	});
	LoadTileObject(TileObjEnum::Agave, {
		"Trees/Agave/SM_Agave_Crop",
	});
	LoadTileObject(TileObjEnum::CactusFruit, {
		"Trees/CactusFruit/SM_Cactus_Crop_01",
		"Trees/CactusFruit/SM_Cactus_Crop_02",
		"Trees/CactusFruit/SM_Cactus_Crop_03",
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
	M_TerritoryBattleHighlight = Load<UMaterial>("/Game/Models/Decals/M_TerritoryBattleHighlight");
	
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
	NiagaraByEnum.Add(NS_OnReveal);
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
		//_LOG(PunLogAsset, "SetSnow %f", ParameterValue);
	}
}

void UAssetLoaderComponent::SetMaterialCollectionParametersVector(FName ParameterName, FLinearColor ParameterValue)
{
	if (UWorld* world = GetWorld()) {
		UKismetMaterialLibrary::SetVectorParameterValue(world, collection, ParameterName, ParameterValue);
		//_LOG(PunLogAsset, "SetVectorParameterValue %f, %f", ParameterValue.R, ParameterValue.G);
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

UStaticMesh* UAssetLoaderComponent::moduleMesh(FName moduleName)
{
	if (_moduleNameToMesh.Contains(moduleName)) {
		return _moduleNameToMesh[moduleName];
	}
	
	//_LOG(PunLogAsset, "No Module Mesh For %s",  *moduleName);
	return nullptr;
}

void UAssetLoaderComponent::LoadModule(FName moduleName, FString meshFile, bool paintConstructionVertexColor, bool isTogglable)
{
	const FString buildingPath = "/Game/Models/Buildings/";
	const auto mesh = LoadF<UStaticMesh>(buildingPath + meshFile);
	
	AddBuildingModule(moduleName, mesh, _moduleNames);
	
	_LOG(PunLogAsset, "LoadModule: %s _ %s", *_moduleNames.Last().ToString(), *mesh->GetName());

	if (isTogglable) {
		_togglableModuleNames.Add(moduleName);
	}

	// Note: this is costly, but usually only done on low poly meshes such as construction frames
	if (paintConstructionVertexColor) {
		_modulesNeedingPaintConstruction.Add(moduleName);
		//PaintMeshForConstruction(moduleName);
	}
}

void UAssetLoaderComponent::LoadTogglableModule(FName moduleName, FString meshFile)
{
	const FString buildingPath = "/Game/Models/Buildings/";
	const auto mesh = LoadF<UStaticMesh>(buildingPath + meshFile);

	AddBuildingModule(moduleName, mesh, _togglableModuleNames);
}

void UAssetLoaderComponent::LoadAnimModule(FName moduleName, FString meshFile)
{
	const FString buildingPath = "/Game/Models/Buildings/";
	const auto mesh = LoadF<UStaticMesh>(buildingPath + meshFile);

	AddBuildingModule(moduleName, mesh, _animModuleNames);
}

void UAssetLoaderComponent::TryLoadBuildingModuleSet_Old(FactionEnum factionEnum, FString moduleSetNameIn, FString meshSetFolder)
{
	FString buildingPath = GetBuildingPath(factionEnum);
	IPlatformFile& platformFile = FPlatformFileManager::Get().GetPlatformFile();

	FString moduleSetNameWithFaction = WithFactionNameInternal(moduleSetNameIn);

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
		FString moduleFileName = moduleSetNameIn + meshTypeNames[i];
		FString path = buildingPath + meshSetFolder + FString("/") + moduleFileName;

		//_LOG(PunLogAsset, "Try Adding Module: %s path: %s", *moduleName, *path);

		if (platformFile.FileExists(*(FPaths::ProjectContentDir() + path + FString(".uasset"))))
		{
			const auto mesh = LoadF<UStaticMesh>("/Game/" + path);

			FName wholeName = *(moduleSetNameWithFaction + meshTypeNames[i]);
			
			AddBuildingModule(wholeName, mesh, _moduleNames);
			_LOG(PunLogAsset, "TryLoadBuildingModuleSet Old: %s _ %s", *_moduleNames.Last().ToString(), *mesh->GetName());

			if (meshTypeNames[i].Equals(FString("Frame"))) {
				_modulesNeedingPaintConstruction.Add(wholeName);
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
		FString moduleName = moduleSetNameIn + togglableMeshTypeNames[i];
		FString path = buildingPath + meshSetFolder + FString("/") + moduleName;

		if (platformFile.FileExists(*(FPaths::ProjectContentDir() + path + FString(".uasset"))))
		{
			const auto mesh = LoadF<UStaticMesh>("/Game/" + path);

			//AddBuildingModule(moduleSetNameWithFaction + togglableMeshTypeNames[i], mesh, _togglableModuleNames);
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


	for (int i = 0; i < animMeshTypeNames.Num(); i++) 
	{
		FString moduleName = moduleSetNameIn + animMeshTypeNames[i];
		FString path = buildingPath + meshSetFolder + FString("/") + moduleName;

		if (platformFile.FileExists(*(FPaths::ProjectContentDir() + path + FString(".uasset"))))
		{
			const auto mesh = LoadF<UStaticMesh>("/Game/" + path);
			
			AddBuildingModule(FName(*(moduleSetNameWithFaction + animMeshTypeNames[i])), mesh, _animModuleNames);
		}
	}
	
}

void UAssetLoaderComponent::TryLoadBuildingModuleSet(FactionEnum factionEnum, FString moduleSetName, FString meshSetFolder, CardEnum buildingEnum, int32 era)
{
	FString buildingPath = GetBuildingPath(factionEnum);
	moduleSetName = WithFactionNameInternal(factionEnum, moduleSetName);
	
	TArray<FString> foundFiles;

	FString findDirectory = FPaths::ProjectContentDir() + buildingPath + meshSetFolder + FString("/");
	IFileManager::Get().FindFiles(foundFiles, *findDirectory, TEXT(".uasset"));

#if WITH_EDITOR
	check(foundFiles.Num() > 0);
#endif

	_recentlyAddedModuleNames.Empty();

	//// foundFiles are just file names, so we append the folder to it
	int32 bodyMainIndex = 1;
	int32 bodySpecialIndex = 1;
	int32 alwaysOnIndex = 1;
	int32 frameIndex = 1;
	int32 workStaticIndex = 1;
	int32 lod01_index = 1;

	auto loadMesh = [&](int32 fileIndex)
	{
		FString path = "/Game/" + buildingPath + meshSetFolder + FString("/") + foundFiles[fileIndex];
		return LoadF<UStaticMesh>(path.LeftChop(7)); // Chop out .uasset
	};

	for (int32 i = 0; i < foundFiles.Num(); i++)
	{
		//foundFiles[i] = folderPath + FString("/") + foundFiles[i];
		_LOG(PunLogAsset, "Files:%s", *(foundFiles[i]));

		// Is from this set
		//if (moduleSetNameIn == foundFiles[i].Left(moduleSetNameIn.Len()))
		{
			_LOG(PunLogAsset, "- File is from set");

			auto addMesh = [&](FString moduleTypeName, bool isSpecial = false)
			{
				FString moduleNameStr = moduleSetName + moduleTypeName;

				const auto mesh = loadMesh(i);
				check(mesh);

				// Use Lightmap Resolution 100 to mark things to display when zoomed out
				// - "Body" will display when zoomed out...
				// - see IsMiniModule() for all tags that will displayed in outer map
				if (moduleTypeName == "Frame")
				{
					moduleNameStr = moduleSetName + moduleTypeName + FString::FromInt(frameIndex);
					frameIndex++;
				}
				else if (moduleTypeName == "AlwaysOn")
				{
					
				}
				else if ((mesh->LightMapResolution == 100 || 
					(mesh->GetMaterial(0) && mesh->GetMaterial(0)->GetName().Contains(TEXT("Roof"), ESearchCase::Type::IgnoreCase))) && bodyMainIndex <= 3)
				{
					moduleNameStr = moduleSetName + FString("Body") + FString::FromInt(bodyMainIndex);
					bodyMainIndex++;
				}
				else if (isSpecial) {
					bodySpecialIndex++;
				}

				FName moduleName = *moduleNameStr;
				AddBuildingModule(moduleName, mesh, _moduleNames);

				_LOG(PunLogAsset, "TryLoadBuildingModuleSet New: %s _ %s", *_moduleNames.Last().ToString(), *mesh->GetName());

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
				foundFiles[i].Contains("_Body4_") ||
				foundFiles[i].Contains("_Body5_") ||
				foundFiles[i].Contains("_Body6_") ||
				foundFiles[i].Contains("_Single") ||
				foundFiles[i].Contains("_Nature_") ||
				foundFiles[i].Contains("_Windows_") ||
				foundFiles[i].Contains("_Window_"))
			{
				FString moduleTypeNameIn = FString("Special") + FString::FromInt(bodySpecialIndex);
				addMesh(moduleTypeNameIn, true);
			}
			else if (foundFiles[i].Contains("_LOD01_"))
			{
				const auto mesh = loadMesh(i);

				FName moduleName = *(moduleSetName + "_LOD01_" + FString::FromInt(lod01_index++));
				AddBuildingModule(moduleName, mesh, _moduleNames);
				_tempAuxGroup.miniModules.push_back(ModuleTransform(moduleName));
			}
			// Work Rotation
			else if (foundFiles[i].Contains("_WorkRotation1"))
			{
				const auto mesh = loadMesh(i);
				FName moduleName = *(moduleSetName + "WorkRotation1");

				AddBuildingModule(moduleName, mesh, _animModuleNames);

				FTransform transform;
				ModuleTypeEnum moduleTypeEnum = ModuleTypeEnum::RotateRoll;
				bool extendsNegY = false;

				if (buildingEnum == CardEnum::Quarry) {
					transform = TransformFromPosition(0, -11.122, 7.325);
					moduleTypeEnum = ModuleTypeEnum::RotateRollQuarry;
				}
				else if (IsMountainMine(buildingEnum)) {
					transform = TransformFromPosition(2.3193, 0, 5.1714);
					moduleTypeEnum = ModuleTypeEnum::RotateRollMine;
				}
				else if (buildingEnum == CardEnum::FurnitureWorkshop) {
					if (factionEnum == FactionEnum::Arab) {
						moduleTypeEnum = ModuleTypeEnum::RotateNewY;
					}
					else {
						transform = TransformFromPosition(6.506, 0, 13.303);
						moduleTypeEnum = ModuleTypeEnum::RotateRollFurniture;
					}
				}
				else if (buildingEnum == CardEnum::PaperMaker) {
					transform = TransformFromPosition(0, 0, 7.388654);
				}
				else if (buildingEnum == CardEnum::IrrigationPump) {
					moduleTypeEnum = ModuleTypeEnum::RotateNewX;
				}
				else if (buildingEnum == CardEnum::CoffeeRoaster) {
					if (factionEnum == FactionEnum::Arab) {
						moduleTypeEnum = ModuleTypeEnum::RotateNewZ;
						extendsNegY = true;
					}
					else {
						transform = era == 3 ? TransformFromPosition(-6.8, 9, 0) : TransformFromPosition(-7.2, 9, 0);
						moduleTypeEnum = ModuleTypeEnum::RotateZAxis;
					}
				}
				else if (buildingEnum == CardEnum::Windmill) {
					if (factionEnum == FactionEnum::Arab) {
						moduleTypeEnum = ModuleTypeEnum::RotateNewZ;
					}
					else {
						transform = TransformFromPosition(0, 0, 60);
						moduleTypeEnum = ModuleTypeEnum::RotateRoll;
					}
				}
				else {
					UE_DEBUG_BREAK();
				}

				FVector rotatorPosition = DetectRotatorPosition(moduleName.ToString(), mesh, extendsNegY);
				ModuleTransform moduleTransform(moduleName, transform, 0.0f, moduleTypeEnum);
				moduleTransform.moduleTypeSpecialState = rotatorPosition;

				_tempAuxGroup.animTransforms.push_back(moduleTransform);
			}
			else if (foundFiles[i].Contains("_WorkRotation2"))
			{
				const auto mesh = loadMesh(i);
				FName moduleName = *(moduleSetName + "WorkRotation2");

				AddBuildingModule(moduleName, mesh, _animModuleNames);

				FTransform transform;
				ModuleTypeEnum moduleTypeEnum = ModuleTypeEnum::RotateRoll;

				if (IsMountainMine(buildingEnum) && buildingEnum != CardEnum::Quarry) {
					transform = TransformFromPosition(21.994, 0, 27.776);
					moduleTypeEnum = ModuleTypeEnum::RotateRollMine2;
				}

				_tempAuxGroup.animTransforms.push_back(ModuleTransform(moduleName, transform, 0.0f, moduleTypeEnum));
			}
			else if (foundFiles[i].Contains("_Scaffolding_")) 
			{
				FName meshName = addMesh("Frame");

				_modulesNeedingPaintConstruction.Add(meshName);
			}
			// These "_Manual_" and "_ManualToggle_" can be extracted in code to follow using _recentlyAddedModuleNames
			else if (foundFiles[i].Contains("_Manual_"))
			{
				const auto mesh = loadMesh(i);
				FName moduleName = *(moduleSetName + "_Manual_" + foundFiles[i]);
				AddBuildingModule(moduleName, mesh, _moduleNames);
			}
			else if (foundFiles[i].Contains("_ManualToggle_"))
			{
				const auto mesh = loadMesh(i);
				FName moduleName = *(moduleSetName + "_ManualToggle_" + foundFiles[i]);
				AddBuildingModule(moduleName, mesh, _togglableModuleNames);
			}
			// ---
			else if (foundFiles[i].Contains("_WorkStatic"))
			{
				const auto mesh = loadMesh(i);
				FName moduleName = *(moduleSetName + "WorkStatic" + FString::FromInt(workStaticIndex));
				workStaticIndex++;

				AddBuildingModule(moduleName, mesh, _togglableModuleNames);

				_tempAuxGroup.togglableTransforms.push_back(ModuleTransform(moduleName));
			}
			else if (foundFiles[i].Contains("_WorkShaderAnimate"))
			{
				const auto mesh = loadMesh(i);
				FName moduleName = *(moduleSetName + "WorkShaderAnimate");

				AddBuildingModule(moduleName, mesh, _animModuleNames);

				_tempAuxGroup.animTransforms.push_back(ModuleTransform(moduleName, FTransform::Identity, 0.0f, ModuleTypeEnum::ShaderAnimate));
			}
			else if (foundFiles[i].Contains("_WorkShaderOnOff"))
			{
				const auto mesh = loadMesh(i);
				FName moduleName = *(moduleSetName + "WorkShaderOnOff");

				AddBuildingModule(moduleName, mesh, _animModuleNames);

				_tempAuxGroup.animTransforms.push_back(ModuleTransform(moduleName, FTransform::Identity, 0, ModuleTypeEnum::ShaderOnOff));
			}
			else if (foundFiles[i].Contains("_Smoke")) {
				const auto mesh = loadMesh(i);
				DetectParticleSystemPosition(buildingEnum, factionEnum, mesh);
			}
		}
	}

#if WITH_EDITOR
	// Hit here means Body needs to be specified. Body is for showing model from far away.
	//  Set with LightMapResolution == 100
	check(bodyMainIndex >= 1 || alwaysOnIndex > 1);
#endif
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
void UAssetLoaderComponent::LoadUnit(UnitEnum unitEnum, std::string meshFile, int32 variationIndex)
{
	FUnitAsset asset;
	asset.staticMesh = Load<UStaticMesh>((unitsPath + meshFile).c_str());

	check(_unitEnumToAsset[static_cast<int>(unitEnum)].size() == variationIndex);
	//_unitDisplayEnumToAsset[static_cast<int>(GetUnitDisplayEnum(unitEnum, _unitEnumToAsset[static_cast<int>(unitEnum)].size()))] = asset;
	_unitEnumToAsset[static_cast<int>(unitEnum)].push_back(asset);
}

// SkelMesh will still need 
void UAssetLoaderComponent::LoadUnitFull(UnitEnum unitEnum, std::string folderPath, std::string skelFileName,
	std::unordered_map<UnitAnimationEnum, std::string> animationFileNames, std::string staticFileName, int32 variationIndex)
{
	FUnitAsset asset;
	asset.skeletalMesh = Load<USkeletalMesh>((unitsPath + folderPath + skelFileName).c_str());
	for (const auto it : animationFileNames) {
		asset.animationEnumToSequence.Add(it.first, Load<UAnimSequence>((unitsPath + folderPath + it.second).c_str()));
	}
	if (staticFileName != "") {
		//_LOG(PunLogAsset, "LoadUnitFull staticFileName %s static:%s", ToTChar(skelFileName), ToTChar(staticFileName));
		asset.staticMesh = Load<UStaticMesh>((unitsPath + staticFileName).c_str());
	}

	check(_unitEnumToAsset[static_cast<int>(unitEnum)].size() == variationIndex);
	//_unitDisplayEnumToAsset[static_cast<int>(GetUnitDisplayEnum(unitEnum, _unitEnumToAsset[static_cast<int>(unitEnum)].size()))] = asset;
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
	_LOG(PunLogAsset, "No Resource Mesh For %s", ToTChar(ResourceName(resourceEnum)));
	return nullptr;
}

UStaticMesh* UAssetLoaderComponent::resourceHandMesh(ResourceEnum resourceEnum)
{
	auto got = _resourceToHandMesh.find(resourceEnum);
	if (got != _resourceToHandMesh.end()) {
		return _resourceToHandMesh[resourceEnum];
	}
	_LOG(PunLogAsset, "No Resource Hand Mesh For %s", ToTChar(ResourceName(resourceEnum)));
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
	//_LOG(PunLogAsset, "No Georesource Mesh For %d", static_cast<int>(georesourceEnum));
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
	_LOG(PunLogAsset, "Painting this shit 11 %d", _modulesNeedingPaintConstruction.Num());
	for (auto i = 0; i < _modulesNeedingPaintConstruction.Num(); i++) {
		_LOG(PunLogAsset, "----- %d", i);
		PaintMeshForConstruction(_modulesNeedingPaintConstruction[i]);
	}
	_LOG(PunLogAsset, "PAINTING ALL DONE");
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
	_LOG(PunLogAsset, "RemoveVertexColor");
	for (auto i = 0; i < _modulesNeedingPaintConstruction.Num(); i++) {
		FName moduleName = _modulesNeedingPaintConstruction[i];
		FStaticMeshVertexBuffers* vertexBuffers = &_moduleNameToMesh[moduleName]->RenderData->LODResources[0].VertexBuffers;
		UStaticMesh* mesh = _moduleNameToMesh[moduleName];

		mesh->RemoveVertexColors();
	}
}

void UAssetLoaderComponent::PrintConstructionMesh()
{
	_LOG(PunLogAsset, "PrintConstructionMesh dfgsdfg %d", _modulesNeedingPaintConstruction.Num());
	for (auto i = 0; i < _modulesNeedingPaintConstruction.Num(); i++) {
		FName moduleName = _modulesNeedingPaintConstruction[i];
		FStaticMeshVertexBuffers& vertexBuffers = _moduleNameToMesh[moduleName]->RenderData->LODResources[0].VertexBuffers;
		UStaticMesh* mesh = _moduleNameToMesh[moduleName];

		int vertexCount = vertexBuffers.PositionVertexBuffer.GetNumVertices();
		_LOG(PunLogAsset, "PrintConstructionMesh %s, vertCount: %d , colorBuffer:%d", *moduleName.ToString(), vertexCount, &vertexBuffers.ColorVertexBuffer != nullptr);
		if (&vertexBuffers.ColorVertexBuffer) {
			_LOG(PunLogAsset, " ------------ colorCount:%d", vertexBuffers.ColorVertexBuffer.GetNumVertices());
		}

		for (int j = 0; j < vertexCount; j++) {
			FString colorStr;
			if (&vertexBuffers.ColorVertexBuffer && vertexBuffers.ColorVertexBuffer.GetNumVertices() > 0) {
				colorStr = vertexBuffers.ColorVertexBuffer.VertexColor(j).ToString();
			}

			_LOG(PunLogAsset, "%s ... %s", *vertexBuffers.PositionVertexBuffer.VertexPosition(j).ToCompactString(), *colorStr);
		}

	}
}

void UAssetLoaderComponent::UpdateRHIConstructionMesh()
{
	_LOG(PunLogAsset, "UpdateRHIConstructionMesh aaa %d", _modulesNeedingPaintConstruction.Num());
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
	//if (isPrinting) _LOG(PunLogAsset, " TraverseTris groupIndex:%d groupIndex_Parent:%d %s", groupIndex, groupIndex_Parent, *vertexPositions.VertexPosition(groupIndex).ToCompactString());
	
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

		//if (isPrinting) _LOG(PunLogAsset, "   -Recurse trisIndex_0:%d", trisIndex_0);

		PUN_CHECK((indexBuffer.Num() / 3) * 3 == indexBuffer.Num());
		PUN_CHECK((trisIndex_0 + 2) < indexBuffer.Num());

		TraverseTris(vertIndexToMergedVertIndex[indexBuffer[trisIndex_0]], groupIndex_Parent, indexBuffer, vertexPositions);
		TraverseTris(vertIndexToMergedVertIndex[indexBuffer[trisIndex_0 + 1]], groupIndex_Parent, indexBuffer, vertexPositions);
		TraverseTris(vertIndexToMergedVertIndex[indexBuffer[trisIndex_0 + 2]], groupIndex_Parent, indexBuffer, vertexPositions);
	}
};

void UAssetLoaderComponent::TraverseTris_July10(uint32 mergedVertIndex, int32 groupIndex_Parent, const TArray<int32>& indexBuffer, const TArray<FVector>& vertexPositions)
{
	//if (isPrinting) _LOG(PunLogAsset, " TraverseTris groupIndex:%d groupIndex_Parent:%d %s", groupIndex, groupIndex_Parent, *vertexPositions.VertexPosition(groupIndex).ToCompactString());

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

		//if (isPrinting) _LOG(PunLogAsset, "   -Recurse trisIndex_0:%d", trisIndex_0);

		PUN_CHECK((indexBuffer.Num() / 3) * 3 == indexBuffer.Num());
		PUN_CHECK((trisIndex_0 + 2) < indexBuffer.Num());

		TraverseTris_July10(vertIndexToMergedVertIndex[indexBuffer[trisIndex_0]], groupIndex_Parent, indexBuffer, vertexPositions);
		TraverseTris_July10(vertIndexToMergedVertIndex[indexBuffer[trisIndex_0 + 1]], groupIndex_Parent, indexBuffer, vertexPositions);
		TraverseTris_July10(vertIndexToMergedVertIndex[indexBuffer[trisIndex_0 + 2]], groupIndex_Parent, indexBuffer, vertexPositions);
	}
};

void UAssetLoaderComponent::DetectOrLoadMeshVertexInfo(FString meshName, UStaticMesh* mesh)
{
	//! Turn this on to cache the result
#if (WITH_EDITOR && 1)
	// In the editor, we DetectMeshGroups and cache results in meshName_to_groupIndexToConnectedVertIndices
	TArray<FVector> vertexPositions;
	
	isPrinting = true;
	DetectMeshGroups(mesh, vertexPositions);
	isPrinting = false;

	meshName_to_vertexPositions.Add(meshName, vertexPositions);
	meshName_to_groupIndexToConnectedVertIndices[ToStdString(meshName)] = groupIndexToConnectedVertIndices;
#else
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
			const FString& meshNameTemp = all_jsonObj_it.Key;
			const TSharedPtr<FJsonObject>& mesh_jsonObj = all_jsonObj->GetObjectField(meshNameTemp);

			meshName_to_vertexPositions.Add(meshNameTemp, {});
			const TArray<TSharedPtr<FJsonValue>>& vertexPositions_jsonArray = mesh_jsonObj->GetArrayField("vertexPosition");
			for (int32 j = 0; j < vertexPositions_jsonArray.Num(); j++)
			{
				TSharedPtr<FJsonObject> vertexPosition_jsonObj = vertexPositions_jsonArray[j]->AsObject();
				meshName_to_vertexPositions[meshNameTemp].Add(
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

				meshName_to_groupIndexToConnectedVertIndices[ToStdString(meshNameTemp)][FCString::Atoi(*groupIndex)] = connectedIndices;
			}
		}
	}

	if (!meshName_to_vertexPositions.Contains(meshName)) {
		return;
	}

	groupIndexToConnectedVertIndices = meshName_to_groupIndexToConnectedVertIndices[ToStdString(meshName)];

#endif
}

FVector UAssetLoaderComponent::DetectRotatorPosition(FString meshName, UStaticMesh* mesh, bool extendsNegY)
{
	DetectOrLoadMeshVertexInfo(meshName, mesh);

	const TArray<FVector>& vertexPositions = meshName_to_vertexPositions[meshName];

	FVector minBound(10000.0f, 10000.0f, 10000.0f);
	FVector maxBound = -minBound;
	for (FVector pos : vertexPositions) {
		if (pos.X < minBound.X) minBound.X = pos.X;
		if (pos.Y < minBound.Y) minBound.Y = pos.Y;
		if (pos.Z < minBound.Z) minBound.Z = pos.Z;

		if (pos.X > maxBound.X) maxBound.X = pos.X;
		if (pos.Y > maxBound.Y) maxBound.Y = pos.Y;
		if (pos.Z > maxBound.Z) maxBound.Z = pos.Z;
	}

	if (extendsNegY) {
		minBound.Y -= (maxBound.Y - minBound.Y) / 3;
	}

	FVector center = (minBound + maxBound) / 2;

	return center;
}

void UAssetLoaderComponent::DetectParticleSystemPosition(CardEnum buildingEnum, FactionEnum factionEnum, UStaticMesh* mesh)
{
	FString meshName = WithFactionNameInternal(factionEnum, mesh->GetName());

	DetectOrLoadMeshVertexInfo(meshName, mesh);

	const TArray<FVector>& vertexPositions = meshName_to_vertexPositions[meshName];
	groupIndexToConnectedVertIndices = meshName_to_groupIndexToConnectedVertIndices[ToStdString(meshName)];

	for (const auto& it : groupIndexToConnectedVertIndices) 
	{
		_LOG(PunLogAsset, " group:%d", it.first);

		FVector averagePosition = FVector::ZeroVector;
		for (int32 vertIndex : it.second) {
			_LOG(PunLogAsset, "  vertIndex:%d %s", vertIndex, *vertexPositions[vertIndex].ToCompactString());
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

		//_LOG(PunLogAsset, "Start Painting");

		auto vertToString = [&](int32 vertIndex) -> FString {
			return vertexPositions[vertIndex].ToCompactString();
		};

		// Print Vertices
		if (isPrinting) {
			_LOG(PunLogAsset, "Print Vertices:");
			for (int32 i = 0; i < vertexCount; i++) {
				_LOG(PunLogAsset, " %s", *vertToString(i));
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

			if (isPrinting) { _LOG(PunLogAsset, "Merge nearby vertices"); }

			for (int i = 0; i < vertexCount; i++)
			{
				if (isPrinting) { _LOG(PunLogAsset, " i = %d", i); }

				if (!processedVertices[i])
				{
					processedVertices[i] = true;
					mergedVertIndexToVertIndices[i].push_back(i);
					vertIndexToMergedVertIndex[i] = i;

					if (isPrinting) { _LOG(PunLogAsset, " i,i groupIndexToVertIndices_PositionMerge[%d].push_back(%d)", i, i); }

					// Add any vertex with the same position
					// This is because UE4 import sometimes split up the geometry...
					for (int j = i + 1; j < vertexCount; j++) {
						if (!processedVertices[j] &&
							vertexPositions[i].Equals(vertexPositions[j], 0.01f))
						{
							processedVertices[j] = true;
							mergedVertIndexToVertIndices[i].push_back(j);
							vertIndexToMergedVertIndex[j] = i;

							if (isPrinting) { _LOG(PunLogAsset, " i,j groupIndexToVertIndices_PositionMerge[%d].push_back(%d)", i, j); }
						}
					}
				}
			}


			_LOG(PunLogAsset, "Position Merge Group Count:%d vertCount:%d", mergedVertIndexToVertIndices.size(), vertexCount);

			if (isPrinting)
			{
				_LOG(PunLogAsset, "________");
				_LOG(PunLogAsset, "Print mergedVertIndexToVertIndices size:%d", mergedVertIndexToVertIndices.size());
				for (const auto& it : mergedVertIndexToVertIndices) {
					_LOG(PunLogAsset, " mergedVertIndex:%d", it.first);
					for (int32 vertIndex : it.second) {
						_LOG(PunLogAsset, "  vertIndex:%d %s", vertIndex, *vertToString(vertIndex));
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
				_LOG(PunLogAsset, "- indexI:%d vertIndex:%d vertIndexToConnectedTrisIndices.size:%d", indexBufferI, vertIndex, vertIndexToConnectedTrisIndices.size());
			}
		}

		if (isPrinting)
		{
			_LOG(PunLogAsset, "________");
			_LOG(PunLogAsset, "Print vertIndexToConnectedTrisIndices size:%d", vertIndexToConnectedTrisIndices.size());
			for (const auto& it : vertIndexToConnectedTrisIndices)
			{
				_LOG(PunLogAsset, " vertIndex:%d %s", it.first, *vertToString(it.first));
				for (int32 trisIndex : it.second) {
					PUN_CHECK(trisIndex < wedgeIndices.Num());
					int32 connectedVertIndex = wedgeIndices[trisIndex];
					_LOG(PunLogAsset, "  trisIndex:%d vertIndex:%d %s", trisIndex, connectedVertIndex, *vertToString(connectedVertIndex));
				}
			}
		}

		// How much average tris connected to vert?
		{
			int32 total = 0;
			for (const auto& it : vertIndexToConnectedTrisIndices) {
				total += it.second.size();
			}
			_LOG(PunLogAsset, "Average trisIndices per Vert: %f", static_cast<float>(total) / vertIndexToConnectedTrisIndices.size());
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
			_LOG(PunLogAsset, "________");
			_LOG(PunLogAsset, "Print mergedVertIndexToConnectedTrisIndices num:%d", mergedVertIndexToConnectedTrisIndices.size());
			for (const auto& it : mergedVertIndexToConnectedTrisIndices) {
				_LOG(PunLogAsset, " mergedVertIndex:%d", it.first);
				for (int32 trisIndex : it.second) {
					PUN_CHECK(trisIndex < wedgeIndices.Num());
					int32 vertIndex = wedgeIndices[trisIndex];
					_LOG(PunLogAsset, " trisIndex:%d vertIndex:%d %s", trisIndex, vertIndex, *vertToString(vertIndex));
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

			if (isPrinting) { _LOG(PunLogAsset, "Traverse Tris"); }

			for (const auto& it : mergedVertIndexToVertIndices) {
				if (isPrinting) { _LOG(PunLogAsset, " groupIndex:%d", it.first); }

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
			_LOG(PunLogAsset, "________");
			_LOG(PunLogAsset, "Print groupIndexToConnectedVertIndices size:%d", groupIndexToConnectedVertIndices.size());
			for (const auto& it : groupIndexToConnectedVertIndices) {
				_LOG(PunLogAsset, " groupIndex:%d", it.first);
				const auto& vertIndices = it.second;
				for (int32 vertIndex : vertIndices) {
					_LOG(PunLogAsset, "  vertIndex:%d pos:%s", vertIndex, *vertToString(vertIndex));
				}
			}
		}

		_LOG(PunLogAsset, "Group Count %d", groupIndexToConnectedVertIndices.size());
	}
//#endif
}

void UAssetLoaderComponent::PaintMeshForConstruction(FName moduleName)
{
#if WITH_EDITOR
	_LOG(PunLogAsset, "PaintMeshForConstruction s1ds33s %s", *moduleName.ToString());

	isPrinting = false; // (moduleName == "HouseLvl1Frame");

	UStaticMesh* mesh = _moduleNameToMesh[moduleName];

	_LOG(PunLogAsset, "!!! Start Paint !!! UpdateRHIConstructionMesh");

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

		//_LOG(PunLogAsset, "Start Painting");

		auto vertToString = [&](int32 vertIndex) -> FString {
			return vertexPositions[vertIndex].ToCompactString();
		};

		// Print Vertices
		if (isPrinting) {
			_LOG(PunLogAsset, "Print Vertices:");
			for (int32 i = 0; i < vertexCount; i++) {
				_LOG(PunLogAsset, " %s", *vertToString(i));
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

			if (isPrinting) _LOG(PunLogAsset, "Merge nearby vertices");

			for (int i = 0; i < vertexCount; i++)
			{
				if (isPrinting) _LOG(PunLogAsset, " i = %d", i);

				if (!processedVertices[i])
				{
					processedVertices[i] = true;
					mergedVertIndexToVertIndices[i].push_back(i);
					vertIndexToMergedVertIndex[i] = i;

					if (isPrinting) _LOG(PunLogAsset, " i,i groupIndexToVertIndices_PositionMerge[%d].push_back(%d)", i, i);

					// Add any vertex with the same position
					// This is because UE4 import sometimes split up the geometry...
					for (int j = i + 1; j < vertexCount; j++) {
						if (!processedVertices[j] &&
							vertexPositions[i].Equals(vertexPositions[j], 0.01f))
						{
							processedVertices[j] = true;
							mergedVertIndexToVertIndices[i].push_back(j);
							vertIndexToMergedVertIndex[j] = i;

							if (isPrinting) _LOG(PunLogAsset, " i,j groupIndexToVertIndices_PositionMerge[%d].push_back(%d)", i, j);
						}
					}
				}
			}


			_LOG(PunLogAsset, "Position Merge Group Count:%d vertCount:%d", mergedVertIndexToVertIndices.size(), vertexCount);

			if (isPrinting)
			{
				_LOG(PunLogAsset, "________");
				_LOG(PunLogAsset, "Print mergedVertIndexToVertIndices size:%d", mergedVertIndexToVertIndices.size());
				for (const auto& it : mergedVertIndexToVertIndices) {
					_LOG(PunLogAsset, " mergedVertIndex:%d", it.first);
					for (int32 vertIndex : it.second) {
						_LOG(PunLogAsset, "  vertIndex:%d %s", vertIndex, *vertToString(vertIndex));
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

			if (isPrinting) _LOG(PunLogAsset, "- indexI:%d vertIndex:%d vertIndexToConnectedTrisIndices.size:%d", indexBufferI, vertIndex, vertIndexToConnectedTrisIndices.size());
		}

		if (isPrinting)
		{
			_LOG(PunLogAsset, "________");
			_LOG(PunLogAsset, "Print vertIndexToConnectedTrisIndices size:%d", vertIndexToConnectedTrisIndices.size());
			for (const auto& it : vertIndexToConnectedTrisIndices)
			{
				_LOG(PunLogAsset, " vertIndex:%d %s", it.first, *vertToString(it.first));
				for (int32 trisIndex : it.second) {
					PUN_CHECK(trisIndex < wedgeIndices.Num());
					int32 connectedVertIndex = wedgeIndices[trisIndex];
					_LOG(PunLogAsset, "  trisIndex:%d vertIndex:%d %s", trisIndex, connectedVertIndex, *vertToString(connectedVertIndex));
				}
			}
		}

		// How much average tris connected to vert?
		{
			int32 total = 0;
			for (const auto& it : vertIndexToConnectedTrisIndices) {
				total += it.second.size();
			}
			_LOG(PunLogAsset, "Average trisIndices per Vert: %f", static_cast<float>(total) / vertIndexToConnectedTrisIndices.size());
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
			_LOG(PunLogAsset, "________");
			_LOG(PunLogAsset, "Print mergedVertIndexToConnectedTrisIndices num:%d", mergedVertIndexToConnectedTrisIndices.size());
			for (const auto& it : mergedVertIndexToConnectedTrisIndices) {
				_LOG(PunLogAsset, " mergedVertIndex:%d", it.first);
				for (int32 trisIndex : it.second) {
					PUN_CHECK(trisIndex < wedgeIndices.Num());
					int32 vertIndex = wedgeIndices[trisIndex];
					_LOG(PunLogAsset, " trisIndex:%d vertIndex:%d %s", trisIndex, vertIndex, *vertToString(vertIndex));
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

			if (isPrinting) _LOG(PunLogAsset, "Traverse Tris");

			for (const auto& it : mergedVertIndexToVertIndices) {
				if (isPrinting) _LOG(PunLogAsset, " groupIndex:%d", it.first);

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
			_LOG(PunLogAsset, "________");
			_LOG(PunLogAsset, "Print groupIndexToConnectedVertIndices size:%d", groupIndexToConnectedVertIndices.size());
			for (const auto& it : groupIndexToConnectedVertIndices) {
				_LOG(PunLogAsset, " groupIndex:%d", it.first);
				const auto& vertIndices = it.second;
				for (int32 vertIndex : vertIndices) {
					_LOG(PunLogAsset, "  vertIndex:%d pos:%s", vertIndex, *vertToString(vertIndex));
				}
			}
		}

		_LOG(PunLogAsset, "Group Count %d", groupIndexToConnectedVertIndices.size());



		
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

		_LOG(PunLogAsset, "meshGroupInfos %d groupTrisCount:%f indices:%d indices/3:%d verts:%d", meshGroupInfos.size(), static_cast<float>(indexCount / 3) / meshGroupInfos.size(), indexCount, indexCount / 3, vertexCount);


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
				_LOG(PunLogAsset, "________");
				_LOG(PunLogAsset, "Print groupIndexToConnectedTrisIndices size:%d", groupIndexToConnectedTrisIndices.size());
				for (const auto& it : groupIndexToConnectedTrisIndices) {
					_LOG(PunLogAsset, " groupIndex:%d", it.first);
					for (int32 trisIndex : it.second) {
						int32 vertIndex = wedgeIndices[trisIndex];
						_LOG(PunLogAsset, "  trisIndex:%d vertIndex:%d %s", trisIndex, vertIndex, *vertToString(vertIndex));
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

		_LOG(PunLogAsset, "meshGroupInfos:%d", meshGroupInfos.size());
		
		for (int i = 0; i < meshGroupInfos.size(); i++) 
		{
			PUN_CHECK(currentColorFloat <= 1.01f);
			
			int32 groupIndex = meshGroupInfos[i].groupIndex;

			if (isPrinting) _LOG(PunLogAsset, "  groupIndex:%d", groupIndex);

			// Color by group
			FColor currentColorVec = FLinearColor(currentColorFloat, currentColorFloat, currentColorFloat, 1.0f).ToFColor(false);
			//FColor currentColorVec(GameRand::DisplayRand(i * 100) % 255, GameRand::DisplayRand(i * 1003) % 255, GameRand::DisplayRand(i * 1030) % 255, 255);
			
			const vector<int32>& trisIndices = groupIndexToConnectedTrisIndices[groupIndex];
			for (int32 trisIndex : trisIndices) {
				rawMesh.WedgeColors[trisIndex] = currentColorVec;

				int32 vertIndex = wedgeIndices[trisIndex];
				if (isPrinting) _LOG(PunLogAsset, "  - tris:%d vertIndex:%d %s", trisIndex, vertIndex, *vertToString(vertIndex));
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

/*
 * Spine
 */

FSpineAsset UAssetLoaderComponent::GetSpine(CardEnum cardEnum)
{
	auto makeAsset = [&](USpineAtlasAsset* atlas, USpineSkeletonDataAsset* data, USpineAtlasAsset* atlas_fx, USpineSkeletonDataAsset* data_fx)
	{
		FSpineAsset asset;
		asset.atlas = atlas;
		asset.skeletonData = data;
		asset.atlas_fx = atlas_fx;
		asset.skeletonData_fx = data_fx;
		return asset;
	};

	switch(cardEnum)
	{
	case CardEnum::Archer: return makeAsset(Archer_SpineAtlas, Archer_SpineData, Arrow_SpineAtlasFX, Arrow_SpineDataFX);
	case CardEnum::Artillery: return makeAsset(Artillery_SpineAtlas, Artillery_SpineData, Cannonlv2_SpineAtlasFX, Cannonlv2_SpineDataFX);
	case CardEnum::Battleship: return makeAsset(Battleship_SpineAtlas, Battleship_SpineData, Cannonlv2_SpineAtlasFX, Cannonlv2_SpineDataFX);

	case CardEnum::Cannon: return makeAsset(Cannon_SpineAtlas, Cannon_SpineData, Cannon_SpineAtlasFX, Cannon_SpineDataFX);
	case CardEnum::Catapult: return makeAsset(Catapult_SpineAtlas, Catapult_SpineData, Rock_SpineAtlasFX, Rock_SpineDataFX);
	case CardEnum::Conscript: return makeAsset(Conscript_SpineAtlas, Conscript_SpineData, Shoot_SpineAtlasFX, Shoot_SpineDataFX);

	case CardEnum::Frigate: return makeAsset(Frigate_SpineAtlas, Frigate_SpineData, Cannon_SpineAtlasFX, Cannon_SpineDataFX);
	case CardEnum::Galley: return makeAsset(Galley_SpineAtlas, Galley_SpineData, FireArrow_SpineAtlasFX, FireArrow_SpineDataFX);
	case CardEnum::Infantry: return makeAsset(Infantry_SpineAtlas, Infantry_SpineData, Shoot_SpineAtlasFX, Shoot_SpineDataFX);

	case CardEnum::Knight: return makeAsset(Knight_SpineAtlas, Knight_SpineData, HeavyCurveSlash_SpineAtlasFX, HeavyCurveSlash_SpineDataFX);
	case CardEnum::MachineGun: return makeAsset(MachineGun_SpineAtlas, MachineGun_SpineData, Shoot_SpineAtlasFX, Shoot_SpineDataFX);
	case CardEnum::Militia: return makeAsset(Militia_SpineAtlas, Militia_SpineData, Stab_SpineAtlasFX, Stab_SpineDataFX);
	case CardEnum::Musketeer: return makeAsset(Musketeer_SpineAtlas, Musketeer_SpineData, LongGun_SpineAtlasFX, LongGun_SpineDataFX);
	case CardEnum::Swordsman: return makeAsset(Swordsman_SpineAtlas, Swordsman_SpineData, Slash_SpineAtlasFX, Slash_SpineDataFX);
	case CardEnum::Tank: return makeAsset(Tank_SpineAtlas, Tank_SpineData, Cannonlv2_SpineAtlasFX, Cannonlv2_SpineDataFX);
	case CardEnum::Warrior: return makeAsset(Warrior_SpineAtlas, Warrior_SpineData, Smash_SpineAtlasFX, Smash_SpineDataFX);

	case CardEnum::Wall: return makeAsset(StoneWall_SpineAtlas, StoneWall_SpineData, nullptr, nullptr);
	case CardEnum::RaidTreasure: return makeAsset(RaidTreasure_SpineAtlas, RaidTreasure_SpineData, nullptr, nullptr);

	default:
		UE_DEBUG_BREAK();
		return makeAsset(Archer_SpineAtlas, Archer_SpineData, Arrow_SpineAtlasFX, Arrow_SpineDataFX);
	}
}