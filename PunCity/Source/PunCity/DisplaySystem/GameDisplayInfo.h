// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "PunCity/AssetLoaderComponent.h"
#include "Components/PrimitiveComponent.h"



class GameDisplayInfo
{
public:
	
	void InitBuildingSets(UAssetLoaderComponent* assetLoader)
	{
		BuildingEnumToVariationToModuleTransforms = assetLoader->buildingEnumToVariationToModuleTransforms();

		auto set = [&](CardEnum buildingEnum, TArray<ModuleTransformGroup> moduleGroups) {
			BuildingEnumToVariationToModuleTransforms[static_cast<int>(buildingEnum)] = moduleGroups;
		};
		auto setName = [&](CardEnum buildingEnum, FString name) {
			BuildingEnumToVariationToModuleTransforms[static_cast<int>(buildingEnum)] = { ModuleTransformGroup::CreateSet(name) };
		};


		auto add = [&](CardEnum buildingEnum, TArray<ModuleTransformGroup> moduleGroups) {
			for (int32 i = 0; i < moduleGroups.Num(); i++) {
				BuildingEnumToVariationToModuleTransforms[static_cast<int>(buildingEnum)].Add(moduleGroups[i]);
			}
		};
		add(CardEnum::House, {
			// // House level 1
			// ModuleTransformGroup::CreateSet("House_Lvl1_Var1"),
			// ModuleTransformGroup::CreateSet("House_Lvl1_Var2"),
			// ModuleTransformGroup::CreateSet("House_Lvl1_Var3"),
			// ModuleTransformGroup::CreateSet("House_Lvl1_Var1"),
			// ModuleTransformGroup::CreateSet("HouseClayLvl1",
			//	 {},
			//	 { {ParticleEnum::Smoke, TransformFromPosition(23.8, -5.7, 29.4)} }
			// ),

			//// House level 2
			//ModuleTransformGroup::CreateSet("House_Lvl1_Var1"),
			//ModuleTransformGroup::CreateSet("HouseClayLvl2"),

			//// House level 3
			//ModuleTransformGroup::CreateSet("HouseLvl3",
			//	{
			//	},
			//	{ {ParticleEnum::Smoke, TransformFromPosition(17.5, 13.3, 43.7)} }
			//),
			//ModuleTransformGroup::CreateSet("HouseLvl3V2",
			//	{
			//	},
			//	{ {ParticleEnum::Smoke, TransformFromPosition(16.9, -2.4, 48.4)} }
			//),
			//ModuleTransformGroup::CreateSet("HouseClayLvl3", {},
			//	{ {ParticleEnum::Smoke, TransformFromPosition(24.65, -4.72, 42.16)} }
			//),

			// House level 4
			ModuleTransformGroup::CreateSet("HouseLvl4", {},
				{ {ParticleEnum::Smoke, TransformFromPosition(14.5, 4.7, 41.3)} }
			),
			ModuleTransformGroup::CreateSet("HouseLvl4V2", {},
				{ {ParticleEnum::Smoke, TransformFromPosition(17.1, 21.24, 40.05)} }
			),
			ModuleTransformGroup::CreateSet("HouseLvl4"),
			ModuleTransformGroup::CreateSet("HouseLvl4"),
			ModuleTransformGroup::CreateSet("HouseLvl4"),

			// House level 5
			ModuleTransformGroup::CreateSet("HouseLvl5", {},
				{ {ParticleEnum::Smoke, TransformFromPosition(1.10, 4.99, 42.03)} }
			),
			ModuleTransformGroup::CreateSet("HouseLvl5V2", {},
				{ {ParticleEnum::Smoke, TransformFromPosition(18.04, 15.43, 39.4)} }
			),
			ModuleTransformGroup::CreateSet("HouseLvl5"),
			ModuleTransformGroup::CreateSet("HouseLvl5"),
			ModuleTransformGroup::CreateSet("HouseLvl5"),

			// House level 6
			ModuleTransformGroup::CreateSet("HouseLvl6", {},
				{ {ParticleEnum::Smoke, TransformFromPosition(22.12, -7.4, 50.88)} }
			),
			ModuleTransformGroup::CreateSet("HouseLvl6V2", {},
				{ {ParticleEnum::Smoke, TransformFromPosition(20.94, -7.36, 45.18)} }
			),
			ModuleTransformGroup::CreateSet("HouseLvl6"),
			ModuleTransformGroup::CreateSet("HouseLvl6"),
			ModuleTransformGroup::CreateSet("HouseLvl6"),
			
			// House level 7
			ModuleTransformGroup::CreateSet("HouseLvl7", {},
				{ {ParticleEnum::Smoke, TransformFromPosition(5.95, -15.14, 62.02)} }
			),
			ModuleTransformGroup::CreateSet("HouseLvl7V2", {},
				{ {ParticleEnum::Smoke, TransformFromPosition(5.74, 25.3, 62.0)} }
			),
			ModuleTransformGroup::CreateSet("HouseLvl7"),
			ModuleTransformGroup::CreateSet("HouseLvl7"),
			ModuleTransformGroup::CreateSet("HouseLvl7"),
		});
		

		set(CardEnum::StorageYard, {
			ModuleTransformGroup(),
		});

		//set(CardEnum::Quarry, {
		//	ModuleTransformGroup::CreateSet("Quarry", {}, {
		//			//{ParticleEnum::BlackSmoke, TransformFromPosition(-.27, 11.4, 21.6)},
		//		}, {
		//			//ModuleTransform("OreMineWorkRotation2", TransformFromPosition(4.99, -8.10, 22.899), 0.0f, ModuleTypeEnum::RotateRoll),
		//			ModuleTransform("QuarrySpecialToggle", FTransform::Identity, 0, ModuleTypeEnum::ShaderOnOff),
		//		},
		//		{
		//			ModuleTransform("OreMineWorkStatic_Stone"),
		//		}
		//	),
		//});


		set(CardEnum::Fence, {
			ModuleTransformGroup({ ModuleTransform("FenceFour", FTransform::Identity, 1.0f, ModuleTypeEnum::Frame)}),
			ModuleTransformGroup({ ModuleTransform("FenceThree", FTransform::Identity, 1.0f, ModuleTypeEnum::Frame)}),
			ModuleTransformGroup({ ModuleTransform("FenceOpposite", FTransform::Identity, 1.0f, ModuleTypeEnum::Frame)}),
			ModuleTransformGroup({ ModuleTransform("FenceAdjacent", FTransform::Identity, 1.0f, ModuleTypeEnum::Frame)}),
		});

		set(CardEnum::FenceGate, {
			ModuleTransformGroup({ ModuleTransform("FenceGate", FTransform::Identity, 1.0f, ModuleTypeEnum::Frame)})
		});

		set(CardEnum::Bridge, {
			ModuleTransformGroup({ ModuleTransform("Bridge1", FTransform::Identity, 1.0f, ModuleTypeEnum::Frame)}),
		});

		set(CardEnum::Forester, {
			ModuleTransformGroup::CreateSet("Forester", {}, {{ParticleEnum::Smoke, TransformFromPosition(9.5, 10.2, 29.5)}})
		});

		set(CardEnum::IronSmelter, {
			ModuleTransformGroup::CreateSet("Smelter", {},
				{
					{ParticleEnum::HeavyBlackSmoke, TransformFromPosition(10.7, -11.1, 37.4)},
				}, {},
				{
					ModuleTransform("SmelterWorkStatic"),
				}
			)
		});


		set(CardEnum::DirtRoad, { ModuleTransformGroup({ ModuleTransform("DirtRoad") }) });
		set(CardEnum::StoneRoad, { ModuleTransformGroup({ ModuleTransform("StoneRoad") }) });


		
		setName(CardEnum::BarrackClubman, "Barrack");
		setName(CardEnum::BarrackArcher, "Barrack");
		setName(CardEnum::BarrackSwordman, "Barrack");

		setName(CardEnum::LaborerGuild, "LaborerGuild");

		setName(CardEnum::HumanitarianAidCamp, "StorageYard");

		set(CardEnum::RegionTribalVillage, {
			ModuleTransformGroup::CreateSet("TribalVillage", {},
				{
					{ParticleEnum::CampFire, TransformFromPositionYawScale(-5.4, -0.82, 0.62, 0, 0.17)}
				},
				{}, {},
				{{0.12f, 35.0f, FLinearColor(1, 0.527f, 0.076f), FVector(-5.4, -0.82, 8.5), FVector::OneVector}}
			),
		});

		setName(CardEnum::RegionShrine, "AncientShrine");
		setName(CardEnum::RegionPort, "PortVillage");
		setName(CardEnum::RegionCrates, "RegionCratePile");

		set(CardEnum::Windmill, {
			ModuleTransformGroup::CreateSet("Windmill", {}, {},
			{
				ModuleTransform("WindmillWorkRotation1", TransformFromPosition(0, 0, 60), 0.0f, ModuleTypeEnum::RotateRoll),
			})
		});

		setName(CardEnum::Bakery, "Bakery");


		setName(CardEnum::Fort, "Outpost");
		setName(CardEnum::ResourceOutpost, "Colony");
		setName(CardEnum::ResearchLab, "InventorsWorkshop");

		set(CardEnum::IntercityRoad, {
			ModuleTransformGroup({ ModuleTransform("DirtRoad")})
		});

		setName(CardEnum::FakeTownhall, "Townhall0");

		set(CardEnum::FakeTribalVillage, {
			ModuleTransformGroup::CreateSet("TribalVillage", {},
				{
					{ParticleEnum::CampFire, TransformFromPositionYawScale(-5.4, -0.82, 0.62, 0, 0.17)}
				},
				{}, {},
				{{0.12f, 35.0f, FLinearColor(1, 0.527f, 0.076f), FVector(-5.4, -0.82, 8.5), FVector::OneVector}}
			)
		});

		setName(CardEnum::ChichenItza, "ChichenItza");

		setName(CardEnum::Market, "Market");
		setName(CardEnum::IrrigationReservoir, "IrrigationReservoir");

		set(CardEnum::Tunnel, {
			ModuleTransformGroup({ ModuleTransform("Tunnel", FTransform::Identity, 1.0f, ModuleTypeEnum::Frame)}),
		});


		setName(CardEnum::Colony, "Townhall0");
		setName(CardEnum::PortColony, "Townhall0");

		setName(CardEnum::IntercityLogisticsHub, "IntercityLogisticsHub");
		setName(CardEnum::IntercityLogisticsPort, "IntercityLogisticsPort");

		set(CardEnum::IntercityBridge, {
			ModuleTransformGroup({ ModuleTransform("Bridge1", FTransform::Identity, 1.0f, ModuleTypeEnum::Frame)}),
		});


		setName(CardEnum::FlowerBed, "FlowerBed");
		setName(CardEnum::GardenShrubbery1, "GardenShrubbery1");
		setName(CardEnum::GardenCypress, "GardenCypress");
		


		


		for (int32 i = 0; i < BuildingEnumCount; i++) {
			if (BuildingEnumToVariationToModuleTransforms[i].Num() == 0) {
				CardEnum buildingEnum = static_cast<CardEnum>(i);
				if (buildingEnum != CardEnum::Farm) {
					setName(buildingEnum, "Ministry");
				}
			}
		}
	}

	// Things that gets added when zoomed out
	static bool IsMiniModule(FString submeshName)
	{
		static const TArray<FString> miniModuleList
		{
			"Body",
			"Body1",
			"Body2",
			"Body3",
			"Roof",
			"Frame",
			"WindowFrame",
			"WindowGlass",
			"Special",
			"AlwaysOn",
		};

		for (const FString& suffix : miniModuleList) {
			if (submeshName.Right(suffix.Len()) == suffix) {
				return true;
			}
		}
		return false;
	}

	void LoadBuildingSets(UAssetLoaderComponent* assetLoader)
	{
		InitBuildingSets(assetLoader);

		// For each building
		for (size_t buildingEnumInt = BuildingEnumToVariationToModuleTransforms.Num(); buildingEnumInt-- > 0;) 
		{
			auto& variationToModuleTransforms = BuildingEnumToVariationToModuleTransforms[buildingEnumInt];

			// For each variations specified
			for (auto& moduleTransforms : variationToModuleTransforms) 
			{
				//UE_LOG(LogTemp, Error, TEXT("LoadBuildingSets %d %s %s"), buildingEnumInt, ToTChar(GetBuildingInfoInt(buildingEnumInt).name), ToTChar(moduleTransforms.setName));
				
				if (moduleTransforms.setName != "")
				{
					// Load building set and replace this moduleTransform with building set..
					FString setName = moduleTransforms.setName;

					{
						//std::vector<ModuleTransform>& transforms = moduleTransforms.transforms;
						std::vector<ModuleTransform> transforms;
						std::vector<ModuleTransform> miniTransforms;

						auto addTransform = [&](const ModuleTransform& transform)
						{
							transforms.push_back(transform);
							if (IsMiniModule(transform.moduleName)) {
								miniTransforms.push_back(transform);
							}
						};

						// Such as clay pit's hole...
						//FString subMeshName = setName + FString("SpecialInstant");
						//if (assetLoader->moduleMesh(subMeshName)) addTransform(ModuleTransform(subMeshName, FTransform::Identity, 0.0f));

						FString subMeshName = setName + FString("Frame");
						if (assetLoader->moduleMesh(subMeshName)) addTransform(ModuleTransform(subMeshName, FTransform::Identity, 10.0f, ModuleTypeEnum::Frame));

						//subMeshName = setName + FString("FrameConstructionOnly");
						//if (assetLoader->moduleMesh(subMeshName)) addTransform(ModuleTransform(subMeshName, FTransform::Identity, 10.0f, ModuleTypeEnum::FrameConstructionOnly));

						subMeshName = setName + FString("Chimney");
						if (assetLoader->moduleMesh(subMeshName)) addTransform(ModuleTransform(subMeshName, FTransform::Identity, 1.0f));

						subMeshName = setName + FString("Floor");
						if (assetLoader->moduleMesh(subMeshName)) addTransform(ModuleTransform(subMeshName, FTransform::Identity, 0.5f));

						subMeshName = setName + FString("Body");
						if (assetLoader->moduleMesh(subMeshName)) {
							addTransform(ModuleTransform(subMeshName));
						}
						for (int32 i = 1; i <= 3; i++) {
							subMeshName = setName + FString("Body") + FString::FromInt(i);
							if (assetLoader->moduleMesh(subMeshName)) addTransform(ModuleTransform(subMeshName));
						}

						// Such as clay pit's hole...
						subMeshName = setName + FString("AlwaysOn");
						if (assetLoader->moduleMesh(subMeshName)) {
							// AlwaysOn Goes to the front so it gets displayed right away
							ModuleTransform transform(subMeshName, FTransform::Identity, 0.0f, ModuleTypeEnum::AlwaysOn);
							transforms.insert(transforms.begin(), transform);
							miniTransforms.push_back(transform);
						}

						subMeshName = setName + FString("Roof");
						if (assetLoader->moduleMesh(subMeshName)) {
							addTransform(ModuleTransform(subMeshName));
						}

						subMeshName = setName + FString("RoofEdge");
						if (assetLoader->moduleMesh(subMeshName)) addTransform(ModuleTransform(subMeshName));

						subMeshName = setName + FString("WindowFrame");
						if (assetLoader->moduleMesh(subMeshName)) addTransform(ModuleTransform(subMeshName));

						subMeshName = setName + FString("FrameAux");
						if (assetLoader->moduleMesh(subMeshName)) addTransform(ModuleTransform(subMeshName));

						subMeshName = setName + FString("Special");
						if (assetLoader->moduleMesh(subMeshName)) {
							addTransform(ModuleTransform(subMeshName));
						}

						for (int32 i = 0; i < 30; i++) {
							subMeshName = setName + FString("Special") + FString::FromInt(i);
							if (assetLoader->moduleMesh(subMeshName)) addTransform(ModuleTransform(subMeshName));
						}

						moduleTransforms.transforms.insert(moduleTransforms.transforms.begin(), transforms.begin(), transforms.end());
						moduleTransforms.miniModules.insert(moduleTransforms.miniModules.begin(), miniTransforms.begin(), miniTransforms.end());
						
						moduleTransforms.CalculateConstructionFractions();
					}

					// Togglables
					{
						std::vector<ModuleTransform>& togglableTransforms = moduleTransforms.togglableTransforms;

						FString subMeshName = setName + FString("WorkStatic");
						if (assetLoader->moduleMesh(subMeshName)) togglableTransforms.push_back(ModuleTransform(subMeshName));

						// Window
						subMeshName = setName + FString("WindowGlass");
						if (assetLoader->moduleMesh(subMeshName)) togglableTransforms.push_back(ModuleTransform(subMeshName, FTransform::Identity, 0.0f, ModuleTypeEnum::Window));
					}
				}
				
			}
		}

		
		// Buildings with 0 frame count uses Era 1's scaff
		for (int32 j = 0; j < BuildingEnumCount; j++)
		{
			CardEnum buildingEnum = static_cast<CardEnum>(j);
			if (IsAutoEraUpgrade(buildingEnum)) 
			{
				TArray<ModuleTransformGroup>& moduleGroups = BuildingEnumToVariationToModuleTransforms[static_cast<int>(buildingEnum)];
				for (int32 era = 1; era < moduleGroups.Num(); era++)
				{
					std::vector<ModuleTransform>& curEraModules = moduleGroups[era].transforms;
					bool hasFrame = false;
					for (int32 i = 0; i < curEraModules.size(); i++) {
						if (FStringCompareRight(curEraModules[i].moduleName, FString("Frame"))) {
							hasFrame = true;
							break;
						}
					}
					if (!hasFrame) {
						const std::vector<ModuleTransform>& firstEraModules = moduleGroups[0].transforms;
						for (int32 i = 0; i < firstEraModules.size(); i++) {
							if (FStringCompareRight(firstEraModules[i].moduleName, FString("Frame"))) {
								ModuleTransform moduleTransform = firstEraModules[i];
								moduleTransform.moduleTypeEnum = ModuleTypeEnum::FrameConstructionOnly;
								curEraModules.insert(curEraModules.begin(), moduleTransform);
								
								moduleGroups[era].CalculateConstructionFractions();
								break;
							}
						}
					}
				}
			}
		}
	}

	int32 GetVariationCount(CardEnum buildingEnum) const {
		return BuildingEnumToVariationToModuleTransforms[static_cast<int>(buildingEnum)].Num();
	}
	
	const ModuleTransformGroup& GetDisplayModules(CardEnum buildingEnum, int32 variationIndex) const {
		int buildingEnumInt = static_cast<int>(buildingEnum);
		check(buildingEnumInt < BuildingEnumToVariationToModuleTransforms.Num()); // !!! Hit here? Forgot to put int GameDisplayInfo?
		check(variationIndex != -1);
		int32 variationCount = BuildingEnumToVariationToModuleTransforms[buildingEnumInt].Num();
		if (variationCount == 0) {
			return ModuleTransformGroup::Empty;
		}
		if (variationIndex >= variationCount) {
			variationIndex = variationCount - 1;
		}
		return BuildingEnumToVariationToModuleTransforms[buildingEnumInt][variationIndex];
	}

private:
	TArray<TArray<ModuleTransformGroup>> BuildingEnumToVariationToModuleTransforms;
};

class GameDisplayUtils
{
public:
	
	static FTransform GetDepositTransform(int32_t worldTileId, FVector displayLocation, int32 variationCount, int32& variationIndex) {
		// Random position
		uint32_t rand = GameRand::DisplayRand(worldTileId);
		//displayLocation.X += rand % 3;
		//rand = GameRand::DisplayRand(rand);
		//displayLocation.Y += rand % 3;

		rand = GameRand::DisplayRand(rand);
		float scale = 1.0f + 0.4f * static_cast<float>(rand % 100) / 100.0f;

		rand = GameRand::DisplayRand(rand); // (do this after scale since second rand is less likely to have same value)
		FRotator rotator(0, static_cast<float>(rand % 360), 0);

		// variation
		rand = GameRand::DisplayRand(rand);
		variationIndex = rand % variationCount;

		return FTransform(rotator, displayLocation, FVector(scale, scale, scale));
	}

	static FTransform GetBushTransform(FVector displayLocation, float xRotation, int worldTileId, int32 ageTicks, TileObjInfo info, BiomeEnum biomeEnum, bool isSqrt = true)
	{
		uint32_t rand = GameRand::DisplayRand(worldTileId);
		displayLocation.X += rand % 3;
		rand = GameRand::DisplayRand(rand);
		displayLocation.Y += rand % 3;

		FRotator rotator(xRotation, static_cast<float>(rand % 360), 0);

		float growthFraction = std::min(1.0f, ageTicks / static_cast<float>(info.maxGrowthTick));
		float scale = 0.2f + 0.8f * (isSqrt ? sqrtf(growthFraction) : growthFraction);

		//rand = GameRand::DisplayRand(rand);
		//float scale = 0.9f + 0.6f * (float)(rand % 100) / 100.0f;

		float heightMultiplier = 1.0f;
		if (biomeEnum == BiomeEnum::Savanna) {
			heightMultiplier = 2.0f;
		}

		return FTransform(rotator, displayLocation, FVector(scale, scale, scale * heightMultiplier));
	}

	static FTransform GetTreeTransform(FVector displayLocation, float xRotation, int32_t worldTileId, int32_t ageTicks, TileObjInfo info)
	{
		uint32_t rand = GameRand::DisplayRand(worldTileId);
		displayLocation.X += rand % 3;
		rand = GameRand::DisplayRand(rand);
		displayLocation.Y += rand % 3;

		FRotator rotator(xRotation, (float)(rand % 360), 0);

		float scale = 0.1f
			+ 0.8f * std::min(1.0f, ageTicks / static_cast<float>(info.maxGrowthTick))
			+ 0.3f * std::max(0.0f, std::min(1.0f, (ageTicks - info.maxGrowthTick) / (float)info.maxGrowthTick))
			+ 0.3f * std::max(0.0f, (rand % 100) / 100.0f); // Rand according to tileId
		float scaleZ = scale;

#if WITH_EDITOR
		if (PunSettings::IsOn("MiniTree")) {
			scale *= 0.1f;
		}
#endif

		return FTransform(rotator, displayLocation, FVector(scale, scale, scaleZ));
	}

	static float TileObjHoverMeshHeight(TileObjInfo info, int32_t worldTileId, int32_t ageTicks)
	{
		float selectionHeight = 20;
		if (info.type == ResourceTileType::Tree) 
		{
			FTransform transform = GetTreeTransform(FVector::ZeroVector, 0, worldTileId, ageTicks, info);
			
			if (info.treeEnum == TileObjEnum::Pine1 ||
				info.treeEnum == TileObjEnum::Pine2) {
				selectionHeight = 55 * transform.GetScale3D().Z;
			}
			else {
				selectionHeight = 42 * transform.GetScale3D().Z;
			}
		}
		return selectionHeight;
	}

	static void SetCustomDepth(UPrimitiveComponent* component, int32 customDepthIndex) {
		if (customDepthIndex > 0) {
			component->SetRenderCustomDepth(true);
			component->SetCustomDepthStencilValue(customDepthIndex);
		}
		else {
			component->SetRenderCustomDepth(false);
		}
	}

	struct BridgeModule
	{
		FString moduleName;
		WorldTile2 tile;
		float rotation;
	};
	static std::vector<BridgeModule> GetBridgeModules(TileArea area)
	{
		int32_t areaLength = std::max(area.sizeX(), area.sizeY());
		bool isXDirection = area.sizeX() > area.sizeY();
		float rotation = RotationFromDirection(isXDirection ? Direction::N : Direction::E);

		WorldTile2 start = area.min();
		
		std::vector<BridgeModule> modules;
		if (areaLength >= 3) {
			WorldTile2 end = isXDirection ? WorldTile2(start.x + areaLength - 1, start.y) : WorldTile2(start.x, start.y + areaLength - 1);
			
			// Ramps
			modules.push_back({ FString("Ramp"), start, rotation });
			modules.push_back({ FString("Ramp"), end, rotation + 180 });
			modules.push_back({ isXDirection ? FString("RampPlane") : FString("RampPlane90"), start, rotation });
			modules.push_back({ isXDirection ? FString("RampPlane") : FString("RampPlane90"), end, rotation + 180 });

			GetBridgeModulesHelper(area.min(), isXDirection, 1, areaLength - 2, modules);
		} else {
			GetBridgeModulesHelper(area.min(), isXDirection, 0, areaLength - 1, modules);
		}
		return modules;
	}
	static void GetBridgeModulesHelper(WorldTile2 start, bool isXDirection, int beginIndex, int endIndex, std::vector<BridgeModule>& modules)
	{
		if (beginIndex > endIndex) {
			return;
		}
		
		float rotation = RotationFromDirection(isXDirection ? Direction::N : Direction::E);

		auto getTileAtIndex = [&] (int index) {
			return isXDirection ? WorldTile2(start.x + index, start.y) : WorldTile2(start.x, start.y + index);
		};

		PUN_CHECK(modules.size() < GameMapConstants::TilesPerWorldX);
		
		// Try to fit Bridge 4, 2, 1 in the middle first
		// tiles leftover on the two sides gets recursed...
		if (endIndex - beginIndex + 1 >= 4) 
		{
			// 3,0 -> 0 ...  4,0 -> 0 ... 5,0 -> 1 ... 6,0 -> 1 ... 7,0 -> 2 ... 8,0 -> 2 ...
			// 6,1 -> 2 ... 7,1 -> 2 ... 8,1 -> 3 ...
			int32_t bridgeStartIndex = (endIndex + beginIndex - 3) / 2;
			int32_t bridgeEndIndex = bridgeStartIndex + 3;

			modules.push_back({ FString("Bridge4"), getTileAtIndex(bridgeStartIndex), rotation });
			modules.push_back({ isXDirection ? FString("Bridge4Plane") : FString("Bridge4Plane90"), getTileAtIndex(bridgeStartIndex), rotation });

			GetBridgeModulesHelper(start, isXDirection, beginIndex, bridgeStartIndex - 1, modules);
			GetBridgeModulesHelper(start, isXDirection, bridgeEndIndex + 1, endIndex, modules);
		}
		else if (endIndex - beginIndex + 1 >= 2) 
		{
			int32 bridgeStartIndex = (endIndex + beginIndex - 1) / 2; // 1,0 -> 0 ...  2,0 -> 0 ... 3,0 -> 1 ... 3,2 -> 2
			int32 bridgeEndIndex = bridgeStartIndex + 1;

			modules.push_back({ FString("Bridge2"), getTileAtIndex(bridgeStartIndex), rotation });
			modules.push_back({ isXDirection ? FString("Bridge2Plane") : FString("Bridge2Plane90"), getTileAtIndex(bridgeStartIndex), rotation });

			GetBridgeModulesHelper(start, isXDirection, beginIndex, bridgeStartIndex - 1, modules);
			GetBridgeModulesHelper(start, isXDirection, bridgeEndIndex + 1, endIndex, modules);
		}
		else {
			PUN_CHECK(endIndex == beginIndex);
			
			modules.push_back({ FString("Bridge1"), getTileAtIndex(beginIndex), rotation});
			modules.push_back({ isXDirection ? FString("Bridge1Plane") : FString("Bridge1Plane90"), getTileAtIndex(beginIndex), rotation });
		}
	}

	static void GetConstructionPoles(TileArea area, std::vector<ModuleTransform>& modules)
	{
		WorldTile2 size = area.size();
		
		// Short side (below center)
		// 1,2 = 0
		// 3,4 = 1
		// 5,6 = 2
		FTransform poleTransform(FRotator::ZeroRotator, FVector((size.x - 1) / 2 * 10, (size.y - 1) / 2 * 10, 0));
		modules.insert(modules.begin(), ModuleTransform("ConstructionPolesBottomLeft", poleTransform, 0.0f, ModuleTypeEnum::ConstructionOnly));

		// Long side (above center)
		// 1 = 0
		// 2,3 = 1
		// 4,5 = 2
		poleTransform = FTransform(FRotator::ZeroRotator, FVector((size.x - 1) / 2 * 10, size.y / 2 * 10, 0));
		modules.insert(modules.begin(), ModuleTransform("ConstructionPolesBottomRight", poleTransform, 0.0f, ModuleTypeEnum::ConstructionOnly));

		poleTransform = FTransform(FRotator::ZeroRotator, FVector(size.x * 10, (size.y - 1) / 2 * 10, 0));
		modules.insert(modules.begin(), ModuleTransform("ConstructionPolesTopLeft", poleTransform, 0.0f, ModuleTypeEnum::ConstructionOnly));

		poleTransform = FTransform(FRotator::ZeroRotator, FVector(size.x / 2 * 10, size.y / 2 * 10, 0));
		modules.insert(modules.begin(), ModuleTransform("ConstructionPolesTopRight", poleTransform, 0.0f, ModuleTypeEnum::ConstructionOnly));
	}
	
};

//static int32_t GetVariationCount() {
//
//}