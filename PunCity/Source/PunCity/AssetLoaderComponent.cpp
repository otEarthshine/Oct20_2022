#include "PunCity/AssetLoaderComponent.h"
#include "Engine/StaticMesh.h"
#include "StaticMeshResources.h"
#include "Particles/ParticleSystem.h"

#include "Materials/Material.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "Materials/MaterialParameterCollection.h"
#include "Kismet/KismetMaterialLibrary.h"

#include "PunCity/GameRand.h"

#include "PunCity/PunUtils.h"
#include <algorithm>
#include <sstream>
#include "HAL/PlatformFilemanager.h"

using namespace std;

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

	//ProjectileArrow = Load<UStaticMesh>("/Game/Models/Projectiles/Arrow/Arrow");

	/**
	 * Building
	 */

	WorldMapMesh = Load<UStaticMesh>("/Game/Models/WorldMap/WorldMap");
	WorldMapMeshSubdivide = Load<UStaticMesh>("/Game/Models/WorldMap/WorldMapSubdivide");
	
	WorldMapMeshWater = Load<UStaticMesh>("/Game/Models/WorldMap/WorldMapWater");
	WorldMapMeshWaterOutside = Load<UStaticMesh>("/Game/Models/WorldMap/WorldMapWaterOutside");

	//M_WorldMap = Load<UMaterial>("/Game/Models/WorldMap/M_WorldMap");
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
	CoinIcon = Load<UTexture2D>("/Game/UI/MiscIcons/CoinPNG");
	InfluenceIcon = Load<UTexture2D>("/Game/UI/MiscIcons/InfluencePNG");
	ScienceIcon = Load<UTexture2D>("/Game/UI/MiscIcons/SciencePNG");

	ExclamationIcon = Load<UTexture2D>("/Game/UI/Images/ExclamationIconPNG");

	AdultIcon = Load<UTexture2D>("/Game/UI/Images/HumanIcon");
	AdultIconSquare = Load<UTexture2D>("/Game/UI/Images/HumanIconSquare");

	UnhappyHoverIcon = Load<UTexture2D>("/Game/UI/Images/Unhappy");

	BlackIcon = Load<UTexture2D>("/Game/UI/GeneratedIcons/BlackIcon");

	M_GeoresourceIcon = Load<UMaterial>("/Game/UI/GeoresourceIcons/M_GeoresourceIcon");

	//! Hover warning
	M_HoverWarning = Load<UMaterial>("/Game/UI/Images/M_HoverWarning");
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

	LoadModuleWithConstruction("House", "House/House");
	LoadModuleWithConstruction("StoneHouse", "House/StoneHouse");


	/*
	 * Building Modules
	 */
	TryLoadBuildingModuleSet("Gatherer", "GathererHut");

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
	
	
	TryLoadBuildingModuleSet("Smelter", "Smelter");
	TryLoadBuildingModuleSet("SmelterGold", "SmelterGold");
	TryLoadBuildingModuleSet("SmelterGiant", "SmelterGiant");
	
	TryLoadBuildingModuleSet("FurnitureMaker", "FurnitureWorkshop");
	TryLoadBuildingModuleSet("CharcoalMaker", "CharcoalMaker");
	TryLoadBuildingModuleSet("Forester", "Forester");
	
	TryLoadBuildingModuleSet("TradingPost", "TradingPost");
	TryLoadBuildingModuleSet("TradingCompany", "TradingCompany");
	TryLoadBuildingModuleSet("TradingPort", "TradingPort");
	TryLoadBuildingModuleSet("CardMaker", "CardMaker");
	TryLoadBuildingModuleSet("ImmigrationOffice", "ImmigrationOffice");
	
	TryLoadBuildingModuleSet("StoneToolShop", "StoneToolShop");
	TryLoadBuildingModuleSet("Blacksmith", "BlacksmithLocal");
	TryLoadBuildingModuleSet("Herbalist", "Herbalist");
	TryLoadBuildingModuleSet("MedicineMaker", "MedicineMaker");

	TryLoadBuildingModuleSet("Chocolatier", "Chocolatier");

	TryLoadBuildingModuleSet("ShrineBasic", "ShrineFrost");
	TryLoadBuildingModuleSet("ShrineBasic2", "ShrineFrost");

	TryLoadBuildingModuleSet("StorageYard", "StorageYard");
	TryLoadBuildingModuleSet("Garden", "Garden");

	TryLoadBuildingModuleSet("FlowerBed", "FlowerBed");
	TryLoadBuildingModuleSet("GardenCypress", "GardenCypress");
	TryLoadBuildingModuleSet("GardenShrubbery1", "GardenShrubbery1");
	
	//TryLoadBuildingModuleSet("Farm", "Farm");

	TryLoadBuildingModuleSet("Hunter", "HuntingLodge");

	// TODO: get rid of LoadModuleWithConstruction
	//LoadModuleWithConstruction("Townhall", "TownHall/TownHall");
	//LoadModuleWithConstruction("StorageYard", "StorageYard/StorageYard");

	LoadModuleWithConstruction("IronStatue", "IronStatue/IronStatue");
	LoadModuleWithConstruction("Bank", "Bank/Bank");
	LoadModuleWithConstruction("TempleGrograth", "TempleGrograth/TempleGrograth");

	//LoadModuleWithConstruction("Farm", "Farm/Farm");
	LoadModuleWithConstruction("TermiteFarm", "Fence/Fence");

	LoadModuleWithConstruction("SmallMarket", "SmallMarket/SmallMarket");



	//LoadModuleWithConstruction("GoodsTransporter", "GoodsTransporter/GoodsTransporter");
	LoadModuleWithConstruction("BlossomShrine", "BlossomShrine/BlossomShrine");

	LoadModuleWithConstruction("StoneSlimeRanch", "SlimeRanch/SlimeRanch");
	LoadModuleWithConstruction("WaterSlimeRanch", "SlimeRanch/SlimeRanch");
	LoadModuleWithConstruction("LavaSlimeRanch", "SlimeRanch/SlimeRanch");
	LoadModuleWithConstruction("HolySlimeRanch", "SlimeRanch/SlimeRanch");

	LoadModuleWithConstruction("ThiefGuild", "ThiefGuild/ThiefGuild");
	LoadModuleWithConstruction("SlimePyramid", "Pyramid/Pyramid");

	// Animal modules
	LoadModuleWithConstruction("BoarBurrow", "BoarBurrow/BoarBurrow");

	// Houses
	TryLoadBuildingModuleSet("HouseLvl1", "HouseLvl1");
	TryLoadBuildingModuleSet("HouseLvl2", "HouseLvl2");
	TryLoadBuildingModuleSet("HouseLvl3", "HouseLvl3");
	TryLoadBuildingModuleSet("HouseLvl4", "HouseLvl4");
	TryLoadBuildingModuleSet("HouseLvl5", "HouseLvl5");
	TryLoadBuildingModuleSet("HouseLvl6", "HouseLvl6");
	TryLoadBuildingModuleSet("HouseLvl7", "HouseLvl7");
	
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
	TryLoadBuildingModuleSet("Townhall3", "TownhallModules");
	TryLoadBuildingModuleSet("Townhall4", "TownhallLvl2Modules");
	TryLoadBuildingModuleSet("TownhallLvl5", "TownhallLvl5");
	LoadModule("TownhallLvl5GardenAndSpire", "TownhallLvl5/TownhallLvl5GardenAndSpire");
	
	//PUN_CHECK(moduleMesh("Townhall3Special2"));


	// TownhallLvl2 Modules
	LoadModule("Townhall2Floor", "TownhallLvl2Modules/TownhallLvl2Floor");
	LoadModule("Townhall2Chimney", "TownhallLvl2Modules/TownhallLvl2Chimney");

	LoadModule("Townhall2Roof", "TownhallLvl2Modules/TownhallLvl2Roof");
	LoadModule("Townhall2RoofEdge", "TownhallLvl2Modules/TownhallLvl2RoofEdge");

	LoadModule("Townhall2Body", "TownhallLvl2Modules/TownhallLvl2Body");
	LoadModule("Townhall2Frame", "TownhallLvl2Modules/TownhallLvl2Frames");
	LoadModule("Townhall2FrameAux", "TownhallLvl2Modules/TownhallLvl2FramesAux");

	LoadModule("Townhall2WindowsFrame", "TownhallLvl2Modules/TownhallLvl2WindowsFrame");
	LoadModule("Townhall2WindowsGlass", "TownhallLvl2Modules/TownhallLvl2WindowsGlass");

	// Fisher Modules
	TryLoadBuildingModuleSet("Fisher", "FisherModules");
	LoadModule("FisherMarlin", "FisherModules/StoreMarlin", false, true);

	//
	TryLoadBuildingModuleSet("PaperMaker", "PaperMaker");
	
	TryLoadBuildingModuleSet("Winery", "Brewery");
	TryLoadBuildingModuleSet("Library", "Library");
	TryLoadBuildingModuleSet("School", "School");
	
	TryLoadBuildingModuleSet("Theatre", "Theatre");
	TryLoadBuildingModuleSet("Tavern", "Tavern");
	TryLoadBuildingModuleSet("Tailor", "Tailor");
	TryLoadBuildingModuleSet("BeerBrewery", "BeerBrewery");
	TryLoadBuildingModuleSet("MushroomHut", "MushroomHut");

	TryLoadBuildingModuleSet("ClayPit", "ClayPit");
	TryLoadBuildingModuleSet("Potter", "Potter");

	TryLoadBuildingModuleSet("ConstructionOffice", "ConstructionOffice");
	TryLoadBuildingModuleSet("Ministry", "Ministry");

	TryLoadBuildingModuleSet("BeerBreweryFamous", "BeerBreweryFamous");

	TryLoadBuildingModuleSet("Ranch", "Ranch");

	// Ranch Barn Modules
	TryLoadBuildingModuleSet("RanchBarn", "RanchBarnModules/RanchBarn"); // TODO: This won't work...

	// Barrack Modules
	TryLoadBuildingModuleSet("Barrack", "Barrack");
	//LoadModule("BarrackFloor", "Barrack/BarrackFloor");
	//LoadModule("BarrackBody", "Barrack/BarrackBody");
	//LoadModule("BarrackFrames", "Barrack/BarrackFrames", true);
	//LoadModule("BarrackFramesAux", "Barrack/BarrackFramesAux");

	TryLoadBuildingModuleSet("Bakery", "Bakery");
	TryLoadBuildingModuleSet("Windmill", "Windmill");
	TryLoadBuildingModuleSet("Jeweler", "Jeweler");

	TryLoadBuildingModuleSet("Beekeeper", "Beekeeper");
	TryLoadBuildingModuleSet("Brickworks", "Brickworks");
	TryLoadBuildingModuleSet("CandleMaker", "CandleMaker");
	TryLoadBuildingModuleSet("CottonMill", "CottonMill");
	TryLoadBuildingModuleSet("PrintingPress", "PrintingPress");

	TryLoadBuildingModuleSet("Warehouse", "Warehouse");
	TryLoadBuildingModuleSet("Colony", "Colony");
	TryLoadBuildingModuleSet("Outpost", "Outpost");
	

	TryLoadBuildingModuleSet("TribalVillage", "RegionTribalVillage");
	TryLoadBuildingModuleSet("AncientShrine", "RegionShrine");
	TryLoadBuildingModuleSet("PortVillage", "RegionPortVillage");
	TryLoadBuildingModuleSet("RegionCratePile", "RegionCratePile");

	

	// Mint Modules
	LoadModule("MintFloor", "Mint/MintFloor");
	LoadModule("MintBody", "Mint/MintBody");
	LoadModule("MintFrames", "Mint/MintFrames", true);
	LoadModule("MintFramesAux", "Mint/MintFramesAux");

	LoadModule("MintMeltPot", "Mint/MintMeltPot");
	LoadModule("MintMoltenGold", "Mint/MintMoltenGold");
	LoadModule("MintRoof", "Mint/MintRoof");
	LoadModule("MintRoofEdge", "Mint/MintRoofEdge");
	LoadModule("MintStoneBody", "Mint/MintStoneBody");

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

	// Storage
	LoadModule("StorageTile", "StorageYard/StorageYardTile");
	

	// Trap
	LoadModule("TrapSpike", "TrapSpike/TrapSpike", true);


	// Shrines
	//LoadModule("ShrineRot", "ShrineRot/ShrineRot");
	//LoadModule("ShrineFrost", "ShrineFrost/ShrineFrost");
	LoadModule("HellPortal", "HellPortal/HellPortal");


	// Decorative Modules
	LoadModule("DecorativeBarrel", "BuildingModule1/DecorativeBarrel/DecorativeBarrel", false, true);
	LoadModule("DecorativeBasketBox", "BuildingModule1/DecorativeBasket/DecorativeBasketBox");
	LoadModule("DecorativeBasketNormal", "BuildingModule1/DecorativeBasket/DecorativeBasketNormal");
	LoadModule("DecorativeBasketRound", "BuildingModule1/DecorativeBasket/DecorativeBasketRound");
	LoadModule("DecorativeBlueBottle", "BuildingModule1/DecorativeBlueBottle/DecorativeBlueBottle");
	LoadModule("DecorativeBread", "BuildingModule1/DecorativeBread/DecorativeBread");
	LoadModule("DecorativeHam", "BuildingModule1/DecorativeHam/DecorativeHam");
	LoadModule("DecorativeOrange", "BuildingModule1/DecorativeOrange/DecorativeOrange");
	LoadModule("DecorativeSack", "BuildingModule1/DecorativeSack/DecorativeSack");


	// Other Modules
	LoadModule("WoodFrame", "BuildingModule1/WoodFrame/WoodFrame");

	LoadModule("WindowFrame", "BuildingModule1/WindowsFrame/WindowFrame");
	LoadModule("WindowGlass", "BuildingModule1/WindowsFrame/WindowGlass");

	LoadModule("LovelyHeart", "EasterEggs/LovelyHeart");

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
			
		//	case CardEnum::Farm:
		//	addBuildIcon(FString("FarmIcon"), FString("SpecialIconAlpha"));
		//	break;
		//case CardEnum::DirtRoad: 
		//	addBuildIcon(FString("DirtRoadIcon"), FString("SpecialIconAlpha"));
		//	break;
		//case CardEnum::StoneRoad:
		//	addBuildIcon(FString("StoneRoadIcon"), FString("SpecialIconAlpha"));
		//	break;
		//case CardEnum::Fence: 
		//	addBuildIcon(FString("FenceIcon"), FString("SpecialIconAlpha"));
		//	break;
		//case CardEnum::FenceGate: 
		//	addBuildIcon(FString("FenceGateIcon"), FString("SpecialIconAlpha"));
		//	break;
		//case CardEnum::Bridge:
		//	addBuildIcon(FString("BridgeIcon"), FString("SpecialIconAlpha"));
		//	break;
		//case CardEnum::TrapSpike:
		//	addBuildIcon(FString("SpikeTrapIcon"), FString("SpecialIconAlpha"));
		//	break;
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

	/*
	 * Card Icons
	 */
	auto addCardIcon = [&](CardEnum cardEnum, FString iconFileName) {
		_cardIcons.Add(static_cast<int32>(cardEnum), LoadF<UTexture2D>(FString("/Game/UI/Images/CardImages/") + iconFileName));
	};
	addCardIcon(CardEnum::None, "CardNone");
	addCardIcon(CardEnum::ProductivityBook, "CardBook");
	addCardIcon(CardEnum::FrugalityBook, "CardBook");
	addCardIcon(CardEnum::SustainabilityBook, "CardBook");

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

	LoadUnit(UnitEnum::Human, "Human/Man/Man1");
	LoadUnit(UnitEnum::Human, "Human/Man/Boy1");
	
	LoadUnit(UnitEnum::Alpaca, "Alpaca/Alpaca");
	
	LoadUnit(UnitEnum::Boar, "Boar/Boar_Game");
	LoadUnit(UnitEnum::Boar, "Boar/BoarMini_Game");
	
	LoadUnit(UnitEnum::RedDeer, "Deer/Stag_Game");
	LoadUnit(UnitEnum::YellowDeer, "Deer/StagYellow_Game");
	LoadUnit(UnitEnum::DarkDeer, "Deer/StagDark_Game");

	LoadUnit(UnitEnum::BlackBear, "Bear/BlackBear_Game");
	LoadUnit(UnitEnum::BrownBear, "Bear/BrownBear_Game");
	LoadUnit(UnitEnum::Panda, "Panda/Panda_Game");

	LoadUnit(UnitEnum::Pig, "Pig/StorePig");
	LoadUnit(UnitEnum::Sheep, "Sheep/Sheep_Game");
	LoadUnit(UnitEnum::Cow, "Cow/Cow_Game");

	LoadUnit(UnitEnum::Infantry, "Infantry/Infantry");
	LoadUnit(UnitEnum::ProjectileArrow, "Projectiles/Arrow/Arrow");


	LoadUnitSkel(UnitEnum::Human, "");

	/**
	 * Resource
	 */

	//LoadResource(ResourceEnum::Berries, "Berries/ResourceBerries", "Berries/ResourceBerriesMaterial");
	// TODO: Put this whole thing in resourceInfo??? so it is easier to add resource in only single place...
	LoadResource2(ResourceEnum::Orange, "Orange/Orange");
	LoadResource2(ResourceEnum::Papaya, "Papaya/Papaya");
	
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

	LoadResource2(ResourceEnum::StoneTools, "StoneTools/StoneTools");
	LoadResource2(ResourceEnum::CrudeIronTools, "CrudeIronTools/CrudeIronTools");
	LoadResource2(ResourceEnum::SteelTools, "SteelTools/SteelTools");
	LoadResource2(ResourceEnum::Herb, "Herb/Herb");
	LoadResource2(ResourceEnum::Medicine, "Medicine/Medicine");

	//LoadResource(ResourceEnum::Tools, "Tools/ResourceTools");
	LoadResource2(ResourceEnum::Fish, "Fish/Fish");

	//LoadResource(ResourceEnum::WhaleMeat, "WhaleMeat/ResourceWhaleMeat");
	LoadResource2(ResourceEnum::Grape, "Grape/Grape");
	LoadResource2(ResourceEnum::Wine, "Wine/Wine");
	LoadResource(ResourceEnum::Shroom, "BloodMushroom/ResourceBloodMushroom");

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
	LoadResource2(ResourceEnum::LuxuriousClothes, "Cloth/Cloth");

	LoadResource2(ResourceEnum::Honey, "Honey/Honey");
	LoadResource2(ResourceEnum::Beeswax, "Beeswax/Beeswax");
	LoadResource2(ResourceEnum::Candle, "Candle/Candle");

	LoadResource2(ResourceEnum::Dye, "Dye/Dye");
	LoadResource2(ResourceEnum::Book, "Book/Book");


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
	LoadTree(TileObjEnum::Orange,	"OrangeTrunk",		"OrangeTree/TreeLeaf",		"TreeJune9/TreeOrangeFruit", "OrangeTree/TreeLeaf_lo", "OrangeTree/TreeLeafShadow");
	LoadTree(TileObjEnum::Birch,	"BirchTrunk",		"OrangeTree/TreeLeaf_Round",			"TreeJune9/TreeOrangeFruit", "OrangeTree/TreeLeaf_Round_lo", "OrangeTree/TreeLeafShadow");

	LoadTree(TileObjEnum::Apple,	"OrangeTrunk",		"TreeJune9/TreeJune9Leaves2",			"TreeJune9/TreeOrangeFruit", "OrangeTree/TreeLeaf_lo", "OrangeTree/TreeLeafShadow");
	LoadTree(TileObjEnum::Papaya,	"PapayaTree/PapayaTree",	"PapayaTree/PapayaLeaf",			"PapayaTree/PapayaFruits", "OrangeTree/TreeLeaf_Papaya_lo", "OrangeTree/TreeLeaf_Papaya_lo");
	LoadTree(TileObjEnum::Durian,	"OrangeTrunk",		"JungleTree3/JungleTree3",			"TreeJune9/TreeOrangeFruit", "OrangeTree/TreeLeaf_lo", "OrangeTree/TreeLeafShadow");
	LoadTree(TileObjEnum::Pine1,	"Pine1/PineTrunk",	"Pine1/PineLeaf",					"TreeJune9/TreeOrangeFruit", "Pine1/PineLeaf1_lo", "Pine1/PineLeaf1_lo");
	LoadTree(TileObjEnum::Pine2,	"Pine1/PineTrunk",	"Pine1/PineLeaf2",					"TreeJune9/TreeOrangeFruit", "Pine1/PineLeaf2_lo", "Pine1/PineLeaf2_lo");
	LoadTree(TileObjEnum::GiantMushroom, "GiantMushroom/GiantMushroom", "GiantMushroom/GiantMushroom", defaultFruit, "BirchSeed", "OrangeTree/TreeLeafShadow");

	LoadTree(TileObjEnum::Cherry,	"OrangeTrunk",		"CherryTree/CherryTreeLeaf",			"TreeJune9/TreeOrangeFruit", "OrangeTree/TreeLeaf_lo", "OrangeTree/TreeLeafShadow");
	LoadTree(TileObjEnum::Coconut,	"Coconut/Coconut1/CoconutTrunk", "Coconut/Coconut1/CoconutLeaf", "TreeJune9/TreeOrangeFruit", "Coconut/Coconut1/CoconutLeaf_lo", "Coconut/Coconut1/CoconutLeaf_lo");
	LoadTree(TileObjEnum::Cyathea,	"Fern/Cyathea/CyatheaTrunk", "Fern/Cyathea/CyatheaLeaf",				defaultFruit, "Fern/Cyathea/CyatheaLeaf_lo", "Fern/Cyathea/CyatheaLeaf_lo");
	LoadTree(TileObjEnum::ZamiaDrosi, "ZamiaDrosi/ZamiaDrosiTrunk", "ZamiaDrosi/ZamiaDrosiLeaf",			defaultFruit, "ZamiaDrosi/ZamiaDrosiLeaf_lo", "ZamiaDrosi/ZamiaDrosiLeaf_shadow");

	LoadTree(TileObjEnum::SavannaTree1, "SavannaTree1/SavannaTree1Trunk", "SavannaTree1/SavannaTree1Leaf", defaultFruit, "OrangeTree/TreeLeaf_lo", "SavannaTree1/SavannaTree1Leaf");
	LoadTree(TileObjEnum::Cactus1, "BirchSeed", "Cactus1/Cactus1", defaultFruit, "BirchSeed", "Cactus1/Cactus1");
	

	
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
	//LoadTileObject(TileObjEnum::BaconBush, {
	//	"Trees/Grapevines/Grapeframes",
	//});

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
	rainParticles = Load<UParticleSystem>("/Game/Models/Weather/Rain/RainParticles");
	M_RainWetness = Load<UMaterial>("/Game/Models/Weather/Rain/M_RainWetness");

	ParticlesByEnum.Add(Load<UParticleSystem>("/Game/Models/Others/P_PunSteamLit"));
	ParticlesByEnum.Add(Load<UParticleSystem>("/Game/Models/Others/P_PunBlackSmokeLit"));
	ParticlesByEnum.Add(Load<UParticleSystem>("/Game/Models/Others/P_PunStoveFire"));
	ParticlesByEnum.Add(Load<UParticleSystem>("/Game/Models/Others/P_PunTorchFire2"));

	ParticlesByEnum.Add(Load<UParticleSystem>("/Game/Models/Others/P_CampFire"));
	ParticlesByEnum.Add(Load<UParticleSystem>("/Game/Models/Others/P_BuildingFire"));
	ParticlesByEnum.Add(Load<UParticleSystem>("/Game/Models/Others/P_PunBuildingFireSmokeLit"));

	ParticlesByEnum.Add(Load<UParticleSystem>("/Game/Models/Others/P_PunDemolishDustLit"));

	//_terrainMaterial2 = Load<UMaterial>("/Game/Models/Terrain/Forest/ForestTerrainMaterial");

	//PUN_LOG("_moduleNames %d, toMesh:%d", _moduleNames.Num(), _moduleNameToMesh.Num());
	//PUN_CHECK(_moduleNames.Num() == ModuleMeshCount);
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
		//PUN_LOG("SetSnow %f", ParameterValue);
	}
}

void UAssetLoaderComponent::SetMaterialCollectionParametersVector(FName ParameterName, FLinearColor ParameterValue)
{
	if (UWorld* world = GetWorld()) {
		UKismetMaterialLibrary::SetVectorParameterValue(world, collection, ParameterName, ParameterValue);
		//PUN_LOG("SetVectorParameterValue %f, %f", ParameterValue.R, ParameterValue.G);
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
	//PUN_CHECK(moduleName == "Townhall3Special2" || _moduleNames.Num() == 417);
	
	if (_moduleNameToMesh.Contains(moduleName)) {
		return _moduleNameToMesh[moduleName];
	}
	
	//PUN_LOG("No Module Mesh For %s",  *moduleName);
	return nullptr;
}

UStaticMesh* UAssetLoaderComponent::moduleConstructionMesh(FString moduleName)
{
	if (_moduleNameToConstructionMesh.Contains(moduleName)) {
		return _moduleNameToConstructionMesh[moduleName];
	}
	//PUN_LOG("No Module Construction Mesh For %s", *moduleName);
	return nullptr;
}

void UAssetLoaderComponent::LoadModule(FString moduleName, FString meshFile, bool paintConstructionVertexColor, bool isTogglable)
{
	const FString buildingPath = "/Game/Models/Buildings/";
	const auto mesh = LoadF<UStaticMesh>(buildingPath + meshFile);
	_moduleNameToMesh.Add(moduleName, mesh);
	_moduleNames.Add(moduleName);

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
	_moduleNameToMesh.Add(moduleName, mesh);
	_togglableModuleNames.Add(moduleName);
}

void UAssetLoaderComponent::LoadAnimModule(FString moduleName, FString meshFile)
{
	const FString buildingPath = "/Game/Models/Buildings/";
	const auto mesh = LoadF<UStaticMesh>(buildingPath + meshFile);
	_moduleNameToMesh.Add(moduleName, mesh);
	_animModuleNames.Add(moduleName);
}

void UAssetLoaderComponent::TryLoadBuildingModuleSet(FString moduleSetName, FString meshSetFolder)
{
	const FString buildingPath = "Models/Buildings/";

	static const TArray<FString> meshTypeNames = {
		"SpecialInstant",
		"Frame",
		"Floor",
		"Chimney",
		"Body",
		"Roof",

		"RoofEdge",
		"WindowFrame",
		"FrameAux",
		"Special",
		"Special2",
	};

	IPlatformFile& platformFile = FPlatformFileManager::Get().GetPlatformFile();

	for (int i = 0; i < meshTypeNames.Num(); i++) {
		FString moduleName = moduleSetName + meshTypeNames[i];
		FString path = buildingPath + meshSetFolder + FString("/") + moduleName;

		//PUN_LOG("Try Adding Module: %s path: %s", *moduleName, *path);

		if (platformFile.FileExists(*(FPaths::ProjectContentDir() + path + FString(".uasset"))))
		{
			const auto mesh = LoadF<UStaticMesh>("/Game/" + path);
			_moduleNameToMesh.Add(moduleName, mesh);
			_moduleNames.Add(moduleName);

			//PUN_LOG("Adding Module: %s path: %s", *moduleName, *path);

			if (meshTypeNames[i].Equals(FString("Frame"))) {
				_modulesNeedingPaintConstruction.Add(moduleName);
			}
		}
	}

	// Debug Show
	//{
	//	CardEnum cardToPrint = CardEnum::PaperMaker;
	//	FString name = ToFString(GetBuildingInfo(cardToPrint).name);
	//	PUN_LOG("Print _moduleNames");
	//	for (FString moduleName : _moduleNames) {
	//		if (moduleName.Left(name.Len()) == name) {
	//			PUN_LOG(" - %s", *moduleName);
	//		}
	//	}
	//}
	

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
			_moduleNameToMesh.Add(moduleName, mesh);
			_togglableModuleNames.Add(moduleName);
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
			_moduleNameToMesh.Add(moduleName, mesh);
			_animModuleNames.Add(moduleName);
		}
	}
}

void UAssetLoaderComponent::LoadModuleWithConstruction(FString moduleName, FString meshFile)
{
	const FString buildingPath = "/Game/Models/Buildings/";
	_moduleNameToMesh.Add(moduleName, LoadF<UStaticMesh>(buildingPath + meshFile));
	_moduleNameToConstructionMesh.Add(moduleName, LoadF<UStaticMesh>(buildingPath + meshFile + "Construction"));
	_moduleNames.Add(moduleName);
}

/**
 * Units
 */

UStaticMesh * UAssetLoaderComponent::unitMesh(UnitEnum unitEnum, int32 variationIndex)
{
	auto got = _unitToMeshes.find(unitEnum);
	if (got != _unitToMeshes.end()) {
		return _unitToMeshes[unitEnum][variationIndex];
	}
	_LOG(PunAsset, "No Unit Mesh For %d", static_cast<int>(unitEnum));
	return nullptr;
}

void UAssetLoaderComponent::LoadUnit(UnitEnum unitEnum, std::string meshFile)
{
	const std::string path = "/Game/Models/Units/";
	_unitToMeshes[unitEnum].push_back(Load<UStaticMesh>((path + meshFile).c_str()));
}

void UAssetLoaderComponent::LoadUnitSkel(UnitEnum unitEnum, std::string meshFile)
{
	//const std::string path = "/Game/Models/Units/";
	//_unitToMeshes[unitEnum].push_back(Load<UStaticMesh>((path + meshFile).c_str()));

	_unitToSkeletalMesh[unitEnum] = Load<USkeletalMesh>("/Game/AnimalsFullPack/Goat/Meshes/SK_Goat");
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
	//PUN_LOG("No Georesource Mesh For %d", static_cast<int>(georesourceEnum));
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
	PUN_LOG("Painting this shit 11 %d", _modulesNeedingPaintConstruction.Num());
	for (auto i = 0; i < _modulesNeedingPaintConstruction.Num(); i++) {
		PUN_LOG("----- %d", i);
		PaintMeshForConstruction(_modulesNeedingPaintConstruction[i]);
	}
	PUN_LOG("PAINTING ALL DONE");
}

void UAssetLoaderComponent::RemoveVertexColor() 
{
	PUN_LOG("RemoveVertexColor");
	for (auto i = 0; i < _modulesNeedingPaintConstruction.Num(); i++) {
		FString moduleName = _modulesNeedingPaintConstruction[i];
		FStaticMeshVertexBuffers* vertexBuffers = &_moduleNameToMesh[moduleName]->RenderData->LODResources[0].VertexBuffers;
		UStaticMesh* mesh = _moduleNameToMesh[moduleName];

		mesh->RemoveVertexColors();
	}
}

void UAssetLoaderComponent::PrintConstructionMesh()
{
	PUN_LOG("PrintConstructionMesh dfgsdfg %d", _modulesNeedingPaintConstruction.Num());
	for (auto i = 0; i < _modulesNeedingPaintConstruction.Num(); i++) {
		FString moduleName = _modulesNeedingPaintConstruction[i];
		FStaticMeshVertexBuffers& vertexBuffers = _moduleNameToMesh[moduleName]->RenderData->LODResources[0].VertexBuffers;
		UStaticMesh* mesh = _moduleNameToMesh[moduleName];

		int vertexCount = vertexBuffers.PositionVertexBuffer.GetNumVertices();
		PUN_LOG("PrintConstructionMesh %s, vertCount: %d , colorBuffer:%d", *moduleName, vertexCount, &vertexBuffers.ColorVertexBuffer != nullptr);
		if (&vertexBuffers.ColorVertexBuffer) {
			PUN_LOG(" ------------ colorCount:%d", vertexBuffers.ColorVertexBuffer.GetNumVertices());
		}

		for (int j = 0; j < vertexCount; j++) {
			FString colorStr;
			if (&vertexBuffers.ColorVertexBuffer && vertexBuffers.ColorVertexBuffer.GetNumVertices() > 0) {
				colorStr = vertexBuffers.ColorVertexBuffer.VertexColor(j).ToString();
			}

			PUN_LOG("%s ... %s", *vertexBuffers.PositionVertexBuffer.VertexPosition(j).ToCompactString(), *colorStr);
		}

		_colorMap.Empty();
		mesh->GetVertexColorData(_colorMap);
		PUN_LOG("GetVertexColorData %d", _colorMap.Num());
		for (const auto& pair : _colorMap) {
			PUN_LOG("pair %s ... %s", *pair.Key.ToCompactString(), *pair.Value.ToString());
		}
	}
}

void UAssetLoaderComponent::UpdateRHIConstructionMesh()
{
	PUN_LOG("UpdateRHIConstructionMesh aaa %d", _modulesNeedingPaintConstruction.Num());
	for (int i = 0; i < _modulesNeedingPaintConstruction.Num(); i++) {
		FString moduleName = _modulesNeedingPaintConstruction[i];
		UStaticMesh* mesh = _moduleNameToMesh[moduleName];

		mesh->SetVertexColorData(_colorMap);
		mesh->Build(false); // ??? Some weird bullshit with threading.. slower non-silent build can circumvent the issue
		mesh->MarkPackageDirty();
	}
}

void UAssetLoaderComponent::PaintMeshForConstruction(FString moduleName)
{
	PUN_LOG("PaintMeshForConstruction s1ds33s %s", *moduleName);

	// Example! MeshPaintHelpers::ImportVertexColorsToStaticMesh
	// EditorFactories is where mesh import happens

	UStaticMesh* mesh = _moduleNameToMesh[moduleName];
	FStaticMeshVertexBuffers& vertexBuffers = mesh->RenderData->LODResources[0].VertexBuffers;
	FRawStaticIndexBuffer& indexBuffer = mesh->RenderData->LODResources[0].IndexBuffer;
	FPositionVertexBuffer& vertexPositions = vertexBuffers.PositionVertexBuffer;
	FColorVertexBuffer* vertexColorsPtr = &vertexBuffers.ColorVertexBuffer;

	// !! Don't use this, deprecated
	//FStaticMeshSourceModel& srcModel = mesh->SourceModels[0];

	stringstream ss;
	ss << "VertexPos:" << to_string(vertexPositions.GetNumVertices()) << ", VertexColor:" << to_string(vertexColorsPtr->GetNumVertices()) << "\n";
	//PUN_LOG("VertexPos:%d VertexColor:%d", vertexPositions.GetNumVertices(), vertexColorsPtr->GetNumVertices());


	//check(vertexPositions.GetNumVertices() == vertexColors.GetNumVertices());
	//vertexColors.VertexColor(0)
	//vertexPositions.VertexPosition(0);
	//const FVector WorldSpaceVertexLocation = GetActorLocation() + GetTransform().TransformVector(VertexBuffer->VertexPosition(Index));

	//SetVertexColorData

	int32 vertexCount = vertexPositions.GetNumVertices();

	//check(!vertexColorsPtr);
	if (!vertexColorsPtr) {
		PUN_LOG("No vertexColorPtr %s", *moduleName);
		return;
	}

	//mesh->RemoveVertexColors();

	if (vertexColorsPtr->GetNumVertices() == 0) {
		PUN_LOG("0 vertexColorsPtr->GetNumVertices() %d", vertexColorsPtr->GetNumVertices());
		vertexColorsPtr->Init(vertexCount);
	}

	int32 indexCount = indexBuffer.GetNumIndices();
	TArray<bool> paintedVertices;
	//TArray<bool> indicesWithSameGroup;
	TArray<bool> processedVertices;
	paintedVertices.SetNumZeroed(vertexCount);
	processedVertices.SetNumZeroed(vertexCount);

	TArray<int32> indexToProcess;
	unordered_map<int32, std::vector<int32>> groupIndexToConnectedIndices; //Can't do TMap+TArray

	//PUN_LOG("Start Painting");

	for (int i = 0; i < vertexCount; i++)
	{
		//FVector& position = vertexPositions.VertexPosition(i);
		//PUN_LOG("--------- %d Color %s", i, *vertexColorsPtr->VertexColor(i).ToString());
		//PUN_LOG("SetVertexColor %d Color: %s", i, *vertexColorsPtr->VertexColor(i).ToString());

		//FColor& color = vertexColorsPtr->VertexColor(i);
		FColor color(GameRand::DisplayRand(i * 100), GameRand::DisplayRand(i * 1003), GameRand::DisplayRand(i * 1030), 255);

		//PUN_LOG("----- %d Color: %s", i, *vertexColorsPtr->VertexColor(i).ToString());

		ss << "Vertex " << to_string(i) << " Pos:" << TCHAR_TO_UTF8(*vertexPositions.VertexPosition(i).ToCompactString()) << "\n";
		ss << "-------- Color:" << TCHAR_TO_UTF8(*vertexColorsPtr->VertexColor(i).ToString()) << "\n";

		// Not processed yet, meaning previous group finding passes didn't find this vertex
		if (!processedVertices[i])
		{
			// Use index buffer to find all adjacent vertices and place them in indexToProcess
			// Keep repeating for the next index in indexToProcess until none is left...

			indexToProcess.Empty();
			indexToProcess.Add(i);
			processedVertices[i] = true;

			LOOP_CHECK_START();
			while (indexToProcess.Num() > 0)
			{
				LOOP_CHECK_END();
				
				int32 currentIndex = indexToProcess.Pop();
				groupIndexToConnectedIndices[i].push_back(currentIndex);

				//// Set same group to same color
				//FColor& colorToProcess = vertexColorsPtr->VertexColor(currentIndex);
				//colorToProcess = color;

				// Also add any vertex with the same position
				// This is because UE4 import sometimes split up the geometry...
				for (int j = 0; j < vertexCount; j++) {
					if (!processedVertices[j] &&
						vertexPositions.VertexPosition(currentIndex).Equals(vertexPositions.VertexPosition(j), 0.001f))
					{
						indexToProcess.Add(j);
						processedVertices[j] = true;
					}
				}

				// Loop through index buffer to find adjacents
				for (int indexBufferI = 0; indexBufferI < indexCount; indexBufferI++) // replace while loop with for, so no freeze on error
				{
					int32 index = indexBuffer.GetIndex(indexBufferI);

					if (index == currentIndex)
					{
						// Try adding indices in this tris to the indexToProcess
						int32 indexBufferI_tris0 = (indexBufferI / 3) * 3;
						for (int localI = 0; localI < 3; localI++) {
							int32 indexBufferI_tris = indexBufferI_tris0 + localI;
							int32 adjacentIndex = indexBuffer.GetIndex(indexBufferI_tris);
							if (!processedVertices[adjacentIndex]) {
								indexToProcess.Add(adjacentIndex);
								processedVertices[adjacentIndex] = true;
							}
						}
					}
				}

			}
		}
	}

	//PUN_LOG("Group Count %d", groupIndexToConnectedIndices.size());
	ss << "Group Count: " << to_string(groupIndexToConnectedIndices.size()) << "\n";

	// Build meshGroupInfo list sorted by submesh's lowest height
	struct MeshGroupInfo {
		int32 groupIndex;
		float lowestHeight;
	};
	std::vector<MeshGroupInfo> meshGroupInfos;

	ss << "Start Loop groupIndexToConnectedIndices\n";
	for (auto& it : groupIndexToConnectedIndices)
	{
		float lowestHeight = -999.0f;
		std::vector<int32>& connectedIndices = it.second;

		ss << "connectedIndices size:" << to_string(connectedIndices.size()) << "\n";

		for (auto& connectedIndex : connectedIndices) {
			float height = vertexPositions.VertexPosition(connectedIndex).Z;
			if (height > lowestHeight) {
				lowestHeight = height;
			}
		}
		PUN_CHECK2(lowestHeight > -999.0f, ss.str());
		meshGroupInfos.push_back({ it.first, lowestHeight });
	}

	sort(meshGroupInfos.begin(), meshGroupInfos.end(), [&](const MeshGroupInfo& a, const MeshGroupInfo& b) -> bool {
		return a.lowestHeight < b.lowestHeight;
	});

	_colorMap.Empty();

	float colorIncrement = 1.0f / meshGroupInfos.size();
	float currentColor = 0.0f;
	for (int i = 0; i < meshGroupInfos.size(); i++) {
		std::vector<int32> connectedIndices = groupIndexToConnectedIndices[meshGroupInfos[i].groupIndex];
		for (int32 currentIndex : connectedIndices) {
			FColor& colorRef = vertexColorsPtr->VertexColor(currentIndex);
			colorRef = FLinearColor(currentColor, currentColor, currentColor, 1.0f).ToFColor(false);

			FVector vertexPos = vertexPositions.VertexPosition(currentIndex);
			if (!_colorMap.Contains(vertexPos)) {
				_colorMap.Add(vertexPos, FLinearColor(currentColor, currentColor, currentColor, 1.0f).ToFColor(false));
			}
		}
		currentColor += colorIncrement;
	}

	PUN_LOG("SetVertexColorData %d", _colorMap.Num());
	for (const auto& pair : _colorMap) {
		PUN_LOG("pair %s ... %s", *pair.Key.ToCompactString(), *pair.Value.ToString());
	}

	FPlatformProcess::Sleep(0.5f);
	
	PUN_LOG("UpdateRHIConstructionMesh BUJUMbi");

	mesh->SetVertexColorData(_colorMap);
	mesh->Build(true);
	mesh->MarkPackageDirty();


	//AsyncTask(ENamedThreads::GetRenderThread(), [vertexColorsPtr]() {
	//	vertexColorsPtr->UpdateRHI();
	//});



	//FStaticMeshVertexBuffers* vertexBuffersPtr = &mesh->RenderData->LODResources[0].VertexBuffers;
	//ENQUEUE_RENDER_COMMAND(FAssetLoaderPaintConstructionVertexColor)(
	//	[vertexBuffersPtr](FRHICommandListImmediate& RHICmdList)
	//{
	//	vertexBuffersPtr->ColorVertexBuffer.UpdateRHI();
	//});

	//ss << "BeforeRelease... VertexPos:" << to_string(vertexPositions.GetNumVertices()) << ", VertexColor:" << to_string(vertexColorsPtr->GetNumVertices()) << "\n";

	//mesh->ReleaseResources();
	////mesh->ReleaseResourcesFence.Wait();

	//ss << "AfterRelease... VertexPos:" << to_string(vertexPositions.GetNumVertices()) << ", VertexColor:" << to_string(vertexColorsPtr->GetNumVertices()) << "\n";
	//PUN_CHECK2(vertexPositions.GetNumVertices() > 0, ss.str());

	//mesh->InitResources();

	//ENamedThreads::GetRenderThread_Local()

	//AsyncTask(ENamedThreads::GetRenderThread(), [&]()
	//{
	//	mesh->RenderData->LODResources[0].VertexBuffers.ColorVertexBuffer.UpdateRHI();

	//	//mesh->RenderData->LODResources[0].VertexBuffers.PositionVertexBuffer.InitRHI();
	//	//mesh->RenderData->LODResources[0].VertexBuffers.ColorVertexBuffer.ReleaseRHI();
	//	//mesh->RenderData->LODResources[0].VertexBuffers.ColorVertexBuffer.ReleaseResource();
	//	//mesh->RenderData->LODResources[0].VertexBuffers.ColorVertexBuffer.InitResource();
	//	//mesh->RenderData->LODResources[0].VertexBuffers.ColorVertexBuffer.InitRHI();
	//	//mesh->RenderData->LODResources[0].VertexBuffers.ColorVertexBuffer.UpdateRHI();
	//});

	// Get Worldspace position of vertices
	//const FVector WorldSpaceVertexLocation = GetActorLocation() + GetTransform().TransformVector(VertexBuffer->VertexPosition(Index));

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
										std::string fruitMeshFile, std::string leafLowMeshFile, std::string leafShadowMeshFile)
{
	const std::string path = "/Game/Models/Trees/";
	FTileMeshAssets proto;
	proto.assets.Add(Load<UStaticMesh>((path + trunkMeshFile).c_str()));
	proto.assets.Add(Load<UStaticMesh>((path + leafMeshFile).c_str()));
	proto.assets.Add(Load<UStaticMesh>((path + fruitMeshFile).c_str()));
	proto.assets.Add(Load<UStaticMesh>((path + leafLowMeshFile).c_str()));
	proto.assets.Add(Load<UStaticMesh>((path + leafShadowMeshFile).c_str()));
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
