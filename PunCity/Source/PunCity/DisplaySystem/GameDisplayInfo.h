// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "PunCity/AssetLoaderComponent.h"
#include "Components/PrimitiveComponent.h"

// Resolves overlay setting conflict
enum class OverlaySetterType {
	BuildingPlacement,
	ObjectDescriptionUI,
	OverlayToggler,
	Count,
	None,
};

struct ParticleInfo
{
	ParticleEnum particleEnum = ParticleEnum::Smoke;
	FTransform transform;
};

struct PointLightInfo
{
	float intensity = 0.1f;
	float attenuationRadius = 35;
	FLinearColor color = FLinearColor(1, 0.527, 0.076);
	FVector position;
	FVector scale;
};

static FTransform TransformFromPosition(float X, float Y, float Z) {
	return FTransform(FVector(X, Y, Z));
}
static FTransform TransformFromPositionScale(float X, float Y, float Z, float scale) {
	return FTransform(FRotator::ZeroRotator, FVector(X, Y, Z), FVector(scale));
}
static FTransform TransformFromPositionYaw(float X, float Y, float Z, float yaw) {
	return FTransform(FRotator(0, yaw, 0), FVector(X, Y, Z), FVector::OneVector);
}
static FTransform TransformFromPositionYawScale(float X, float Y, float Z, float yaw, float scale) {
	return FTransform(FRotator(0, yaw, 0), FVector(X, Y, Z), FVector(scale));
}
static FTransform TransformFromPositionYawScale(float X, float Y, float Z, float yaw, FVector scale) {
	return FTransform(FRotator(0, yaw, 0), FVector(X, Y, Z), scale);
}


enum class ModuleTypeEnum
{
	Normal,
	ConstructionOnly,
	ConstructionBaseHighlight,
	Frame,
	Window,
	RotateRoll,
	ShaderAnimate,
	ShaderOnOff,
};

struct ModuleTransform
{
	FString moduleName;

	float relativeConstructionTime = 0;
	ModuleTypeEnum moduleTypeEnum = ModuleTypeEnum::Normal;

	// !! Note that cannot do scaling on mesh with constructionMesh... (But then we don't need it?)
	// !! Beware... rotator is Pitch Yaw Roll
	FTransform transform; 

	// TODO: use grouping system?? don't need this?... each ModuleTransform can be assigned group index.
	//std::vector<ModuleTransform> children;

	// Calculated values
	float constructionFractionBegin = 0;
	float constructionFractionEnd = 0;

	// what index this model should turn on
	// 0 means all... -1 means removal if [0] is upgraded ... 1 means add if [0] is upgraded
	// (disadvantage... can't do model upgrade beyond 
	int32 upgradeStates = 0; //TODO: ????

	float moduleConstructionFraction(float constructionFraction) const {
		PUN_CHECK(constructionFractionBegin >= 0.0f);
		return (constructionFraction - constructionFractionBegin) / (constructionFractionEnd - constructionFractionBegin);
	}

	ModuleTransform(FString moduleName,
		FTransform transform = FTransform::Identity, float relativeConstructionTime = 0.0f, ModuleTypeEnum moduleTypeEnum = ModuleTypeEnum::Normal,
		int32 upgradeStates = 0)
		: moduleName(moduleName),
		relativeConstructionTime(relativeConstructionTime),
		moduleTypeEnum(moduleTypeEnum),
		transform(transform),
		upgradeStates(upgradeStates)
	{}
};

struct ModuleTransforms
{
	std::vector<ModuleTransform> transforms; //TODO: change name to modules?
	std::vector<ModuleTransform> miniModules; // For far zoom
	
	std::vector<ParticleInfo> particleInfos;

	std::vector<ModuleTransform> animTransforms;
	std::vector<ModuleTransform> togglableTransforms;

	std::vector<PointLightInfo> lightInfos;
	
	std::string setName; // For building name etc.

	ModuleTransforms(std::vector<ModuleTransform> transforms, 
						std::vector<ParticleInfo> particleInfos = {},
						std::vector<ModuleTransform> animTransforms = {},
						std::vector<ModuleTransform> togglableTransforms = {},
						std::vector<PointLightInfo> lightInfos = {})
		:
		transforms(transforms),
		particleInfos(particleInfos),
		animTransforms(animTransforms),
		togglableTransforms(togglableTransforms),
		lightInfos(lightInfos)
	{
		PUN_CHECK(transforms.size() < 64); // InstanceKey = worldTileId + i * maxWorldTileId ...  2^32 /(2^12*2^13) / 2 = 64 .. more than 128 and it will overflow...
		
		CalculateConstructionFractions();
	}

	static ModuleTransforms CreateSet(std::string setName,
										std::vector<ModuleTransform> transforms = {},
										std::vector<ParticleInfo> particleInfos = {},
										std::vector<ModuleTransform> animTransforms = {},
										std::vector<ModuleTransform> togglableTransforms = {},
										std::vector<PointLightInfo> lightInfos = {}) // Note: animTransforms must be set manually since we need its locations
	{
		ModuleTransforms result(transforms, particleInfos, animTransforms, togglableTransforms, lightInfos);
		result.setName = setName;
		return result;
	}

	void CalculateConstructionFractions()
	{
		//if (setName != "") UE_LOG(LogTemp, Log, TEXT("CalculateConstructionFractions %s"), ToTChar(setName));
		
		float totalRelativeConstructionTime = 0.0f;
		for (ModuleTransform& transform : transforms) {
			totalRelativeConstructionTime += transform.relativeConstructionTime;
		}
		//if (setName != "") UE_LOG(LogTemp, Log, TEXT("  totalRelativeConstructionTime %.2f"), totalRelativeConstructionTime);
		
		float constructionFraction = 0.0f;
		for (size_t i = 0; i < transforms.size(); i++) {
			transforms[i].constructionFractionBegin = constructionFraction;
			constructionFraction += transforms[i].relativeConstructionTime / totalRelativeConstructionTime;
			transforms[i].constructionFractionEnd = constructionFraction;

			//if (transforms[i].moduleTypeEnum == ModuleTypeEnum::Frame) {
			//	if (setName != "") UE_LOG(LogTemp, Log, TEXT("  Frame begin:%.2f end:%.2f"), transforms[i].constructionFractionBegin, transforms[i].constructionFractionEnd);
			//}
			
			PUN_CHECK(constructionFraction <= 1.0f)
		}
	}


	static ModuleTransforms CreateOreMineSet(FString oreMineSpecialName, FString oreMineWorkStaticName)
	{
		return CreateSet("OreMine", {}, {
					{ParticleEnum::BlackSmoke, TransformFromPosition(-.27, 11.4, 21.6)},
				}, {
					ModuleTransform("OreMineWorkRotation2", TransformFromPosition(4.99, -8.10, 22.899), 0.0f, ModuleTypeEnum::RotateRoll),
					ModuleTransform(oreMineSpecialName, FTransform::Identity, 0, ModuleTypeEnum::ShaderOnOff),
				},
				{
					ModuleTransform(oreMineWorkStaticName),
				}
			);
	}
};

// It is roll, pitch, yaw in the editor.. so make it the same..
static FRotator EditorFRotator(float roll, float pitch, float yaw) {
	return FRotator(pitch, yaw, roll);
}

class GameDisplayInfo
{
public:
	void InitBuildingSets()
	{
		BuildingEnumToVariationToModuleTransforms = {
			{
				/*
				 * House by level
				 */
				// House level 1
				ModuleTransforms::CreateSet("HouseLvl1",
					{
						ModuleTransform("DecorativeBasketBox", TransformFromPositionScale(-8.35, 19.9, -0.42, 0.07)),
						ModuleTransform("DecorativeBarrel", TransformFromPosition(-8.53, 5.41, -0.21)),
						ModuleTransform("DecorativeBlueBottle", TransformFromPosition(-9.34, 5.98, 4.7)),
						ModuleTransform("DecorativeOrange", TransformFromPosition(-9.01, 4.63, 4.88)),
						ModuleTransform("DecorativeOrange", TransformFromPosition(-7.92, 5.41, 4.88)),
					},
					{ {ParticleEnum::Smoke, TransformFromPosition(22.7, -0.7, 22.36)} }
				),
				ModuleTransforms::CreateSet("HouseLvl1V2",
					{
						ModuleTransform("DecorativeBasketBox", TransformFromPositionScale(-8.35, 19.9, -0.42, 0.07)),
						ModuleTransform("DecorativeBarrel", TransformFromPosition(-8.53, 5.41, -0.21)),
						ModuleTransform("DecorativeBlueBottle", TransformFromPosition(-9.34, 5.98, 4.7)),
						ModuleTransform("DecorativeOrange", TransformFromPosition(-9.01, 4.63, 4.88)),
						ModuleTransform("DecorativeOrange", TransformFromPosition(-7.92, 5.41, 4.88)),
					},
					{ {ParticleEnum::Smoke, TransformFromPosition(16.7, -12.4, 22.0)} }
				),
				ModuleTransforms::CreateSet("HouseClayLvl1",
					{},
					{ {ParticleEnum::Smoke, TransformFromPosition(23.8, -5.7, 29.4)} }
				),

				// House level 2
				ModuleTransforms::CreateSet("HouseLvl2",
					{
						ModuleTransform("DecorativeBasketBox", TransformFromPositionScale(-8.35, 9.9, -0.42, 0.07)),
						ModuleTransform("DecorativeBarrel", TransformFromPosition(-8.53, 5.41, -0.21)),
						ModuleTransform("DecorativeBlueBottle", TransformFromPosition(-9.34, 5.98, 4.7)),
						ModuleTransform("DecorativeOrange", TransformFromPosition(-9.01, 4.63, 4.88)),
						ModuleTransform("DecorativeOrange", TransformFromPosition(-7.92, 5.41, 4.88)),
					},
					{ {ParticleEnum::Smoke, TransformFromPosition(21.9, 4.23, 31.8)} }
				),
				ModuleTransforms::CreateSet("HouseLvl2V2",
					{
						ModuleTransform("DecorativeBasketBox", TransformFromPositionScale(-8.35, 9.9, -0.42, 0.07)),
						ModuleTransform("DecorativeBarrel", TransformFromPosition(-8.53, 5.41, -0.21)),
						ModuleTransform("DecorativeBlueBottle", TransformFromPosition(-9.34, 5.98, 4.7)),
						ModuleTransform("DecorativeOrange", TransformFromPosition(-9.01, 4.63, 4.88)),
						ModuleTransform("DecorativeOrange", TransformFromPosition(-7.92, 5.41, 4.88)),
					},
					{ {ParticleEnum::Smoke, TransformFromPosition(14.6, -5.7, 34.8)} }
				),
				ModuleTransforms::CreateSet("HouseClayLvl2", {},
					{ {ParticleEnum::Smoke, TransformFromPosition(18.2, 3.3, 44.0)} }
				),

				// House level 3
				ModuleTransforms::CreateSet("HouseLvl3",
					{
						ModuleTransform("DecorativeBasketBox", TransformFromPositionScale(-8.35, 9.9, -0.42, 0.07)),
						ModuleTransform("DecorativeBarrel", TransformFromPosition(-8.53, 5.41, -0.21)),
						ModuleTransform("DecorativeBlueBottle", TransformFromPosition(-9.34, 5.98, 4.7)),
						ModuleTransform("DecorativeOrange", TransformFromPosition(-9.01, 4.63, 4.88)),
						ModuleTransform("DecorativeOrange", TransformFromPosition(-7.92, 5.41, 4.88)),
					},
					{ {ParticleEnum::Smoke, TransformFromPosition(17.5, 13.3, 43.7)} }
				),
				ModuleTransforms::CreateSet("HouseLvl3V2",
					{
						ModuleTransform("DecorativeBasketBox", TransformFromPositionScale(-8.35, 9.9, -0.42, 0.07)),
						ModuleTransform("DecorativeBarrel", TransformFromPosition(-8.53, 5.41, -0.21)),
						ModuleTransform("DecorativeBlueBottle", TransformFromPosition(-9.34, 5.98, 4.7)),
						ModuleTransform("DecorativeOrange", TransformFromPosition(-9.01, 4.63, 4.88)),
						ModuleTransform("DecorativeOrange", TransformFromPosition(-7.92, 5.41, 4.88)),
					},
					{ {ParticleEnum::Smoke, TransformFromPosition(16.9, -2.4, 48.4)} }
				),
				ModuleTransforms::CreateSet("HouseClayLvl3", {},
					{ {ParticleEnum::Smoke, TransformFromPosition(24.65, -4.72, 42.16)} }
				),

				// House level 4
				ModuleTransforms::CreateSet("HouseLvl4", {},
					{ {ParticleEnum::Smoke, TransformFromPosition(14.5, 4.7, 41.3)} }
				),
				ModuleTransforms::CreateSet("HouseLvl4V2", {},
					{ {ParticleEnum::Smoke, TransformFromPosition(17.1, 21.24, 40.05)} }
				),
				ModuleTransforms::CreateSet("HouseLvl4", {},
					{ {ParticleEnum::Smoke, TransformFromPosition(14.5, 4.7, 41.3)} }
				),

				// House level 5
				ModuleTransforms::CreateSet("HouseLvl5", {},
					{ {ParticleEnum::Smoke, TransformFromPosition(1.10, 4.99, 42.03)} }
				),
				ModuleTransforms::CreateSet("HouseLvl5V2", {},
					{ {ParticleEnum::Smoke, TransformFromPosition(18.04, 15.43, 39.4)} }
				),
				ModuleTransforms::CreateSet("HouseLvl5", {},
					{ {ParticleEnum::Smoke, TransformFromPosition(1.10, 4.99, 42.03)} }
				),

				// House level 6
				ModuleTransforms::CreateSet("HouseLvl6", {},
					{ {ParticleEnum::Smoke, TransformFromPosition(22.12, -7.4, 50.88)} }
				),
				ModuleTransforms::CreateSet("HouseLvl6V2", {},
					{ {ParticleEnum::Smoke, TransformFromPosition(20.94, -7.36, 45.18)} }
				),
				ModuleTransforms::CreateSet("HouseLvl6", {},
					{ {ParticleEnum::Smoke, TransformFromPosition(22.12, -7.4, 50.88)} }
				),

				// House level 7
				ModuleTransforms::CreateSet("HouseLvl7", {},
					{ {ParticleEnum::Smoke, TransformFromPosition(5.95, -15.14, 62.02)} }
				),
				ModuleTransforms::CreateSet("HouseLvl7V2", {},
					{ {ParticleEnum::Smoke, TransformFromPosition(5.74, 25.3, 62.0)} }
				),
				ModuleTransforms::CreateSet("HouseLvl7", {},
					{ {ParticleEnum::Smoke, TransformFromPosition(5.95, -15.14, 62.02)} }
				),
			},
			{
				ModuleTransforms({ ModuleTransform("StoneHouse") }),
				ModuleTransforms({ ModuleTransform("StoneHouse2") }),
				ModuleTransforms({ ModuleTransform("StoneHouse3") }),
			},

			{ // Gatherer
				ModuleTransforms::CreateSet("Gatherer")
			},

			{
				// 1
				ModuleTransforms::CreateSet("TownhallThatch",
					{
						ModuleTransform("DecorativeBread", TransformFromPositionScale(-21.95, -12.92, 5.67, 0.6)),
						ModuleTransform("DecorativeBread", TransformFromPositionScale(-19.5, -12.05, 5.66, 0.7)),
						ModuleTransform("DecorativeBlueBottle", TransformFromPosition(-20.59, -18.4, 4.7)),
						ModuleTransform("DecorativeBlueBottle", TransformFromPosition(-23.22, 35.3, 4.7)),
						ModuleTransform("DecorativeBlueBottle", TransformFromPosition(-24.87, 34.25, 4.7)),
						ModuleTransform("DecorativeBlueBottle", TransformFromPosition(9.08, 17.84, 0)),
						ModuleTransform("DecorativeHam", TransformFromPosition(-19.89, -15.81, 5.81)),
						ModuleTransform("DecorativeBasketBox", TransformFromPositionYawScale(-16.5, 27.13, 0, 133, 0.07)),
						ModuleTransform("DecorativeBasketRound", TransformFromPositionScale(7.47, 17.37, 0, 0.08)),
						ModuleTransform("DecorativeSack", TransformFromPositionYaw(-0.4, 1.66, -1.03, 141)),
						ModuleTransform("DecorativeSack", TransformFromPositionYaw(3.09, 3.67, -1.03, 142)),
						ModuleTransform("DecorativeSack", TransformFromPositionYaw(1.09, 2.94, 0.22, 142)),
					},
					{
						{ParticleEnum::Smoke, TransformFromPosition(51.6, -24.3, 38.8)},
						{ParticleEnum::CampFire, TransformFromPositionYawScale(-2.75, 9.9, 1.85, 0, 0.17)}
					},
					{}, {},
					{{0.12f, 35.0f, FLinearColor(1, 0.527f, 0.076f), FVector(-2.3, 10.2, 8.5), FVector::OneVector}}
				),
			// 2
			ModuleTransforms::CreateSet("Townhall0",
				{
					ModuleTransform("DecorativeBread", TransformFromPositionScale(-21.95, -12.92, 5.67, 0.6)),
					ModuleTransform("DecorativeBread", TransformFromPositionScale(-19.5, -12.05, 5.66, 0.7)),
					ModuleTransform("DecorativeBlueBottle", TransformFromPosition(-20.59, -18.4, 4.7)),
					ModuleTransform("DecorativeBlueBottle", TransformFromPosition(-23.22, 35.3, 4.7)),
					ModuleTransform("DecorativeBlueBottle", TransformFromPosition(-24.87, 34.25, 4.7)),
					ModuleTransform("DecorativeBlueBottle", TransformFromPosition(9.08, 17.84, 0)),
					ModuleTransform("DecorativeHam", TransformFromPosition(-19.89, -15.81, 5.81)),
					ModuleTransform("DecorativeBasketBox", TransformFromPositionYawScale(-16.5, 27.13, 0, 133, 0.07)),
					ModuleTransform("DecorativeBasketRound", TransformFromPositionScale(7.47, 17.37, 0, 0.08)),
					ModuleTransform("DecorativeSack", TransformFromPositionYaw(-0.4, 1.66, -1.03, 141)),
					ModuleTransform("DecorativeSack", TransformFromPositionYaw(3.09, 3.67, -1.03, 142)),
					ModuleTransform("DecorativeSack", TransformFromPositionYaw(1.09, 2.94, 0.22, 142)),
				},
				{
					{ParticleEnum::Smoke, TransformFromPosition(51.6, -24.3, 38.8)},
					{ParticleEnum::CampFire, TransformFromPositionYawScale(-2.75, 9.9, 1.85, 0, 0.17)}
				},
				{}, {},
				{{0.12f, 35.0f, FLinearColor(1, 0.527f, 0.076f), FVector(-2.3, 10.2, 8.5), FVector::OneVector}}
			),
			// 3
			ModuleTransforms::CreateSet("Townhall3",
				{
					ModuleTransform("DecorativeBread", TransformFromPositionScale(-21.95, -12.92, 5.67, 0.6)),
					ModuleTransform("DecorativeBread", TransformFromPositionScale(-19.5, -12.05, 5.66, 0.7)),
					ModuleTransform("DecorativeBlueBottle", TransformFromPosition(-20.59, -18.4, 4.7)),
					ModuleTransform("DecorativeBlueBottle", TransformFromPosition(-23.22, 35.3, 4.7)),
					ModuleTransform("DecorativeBlueBottle", TransformFromPosition(-24.87, 34.25, 4.7)),
					ModuleTransform("DecorativeBlueBottle", TransformFromPosition(9.08, 17.84, 0)),
					ModuleTransform("DecorativeHam", TransformFromPosition(-19.89, -15.81, 5.81)),
					ModuleTransform("DecorativeBasketBox", TransformFromPositionYawScale(-16.5, 27.13, 0, 133, 0.07)),
					ModuleTransform("DecorativeBasketRound", TransformFromPositionScale(7.47, 17.37, 0, 0.08)),
					ModuleTransform("DecorativeSack", TransformFromPositionYaw(-0.4, 1.66, -1.03, 141)),
					ModuleTransform("DecorativeSack", TransformFromPositionYaw(3.09, 3.67, -1.03, 142)),
					ModuleTransform("DecorativeSack", TransformFromPositionYaw(1.09, 2.94, 0.22, 142)),
				},
				{
					{ParticleEnum::Smoke, TransformFromPosition(34.99, -25.34, 51.55)},
					{ParticleEnum::CampFire, TransformFromPositionYawScale(-10.88, 8.19, 0.37, 0, 0.17)}
				},
				{}, {},
				{{0.12f, 35.0f, FLinearColor(1, 0.527f, 0.076f), FVector(-10.88, 8.19, 8.5), FVector::OneVector}}
			),
			// 4
			ModuleTransforms::CreateSet("Townhall4",
				{
					ModuleTransform("DecorativeBread", TransformFromPositionScale(-21.95, -12.92, 5.67, 0.6)),
					ModuleTransform("DecorativeBread", TransformFromPositionScale(-19.5, -12.05, 5.66, 0.7)),
					ModuleTransform("DecorativeBlueBottle", TransformFromPosition(-20.59, -18.4, 4.7)),
					ModuleTransform("DecorativeBlueBottle", TransformFromPosition(-23.22, 35.3, 4.7)),
					ModuleTransform("DecorativeBlueBottle", TransformFromPosition(-24.87, 34.25, 4.7)),
					ModuleTransform("DecorativeBlueBottle", TransformFromPosition(9.08, 17.84, 0)),
					ModuleTransform("DecorativeHam", TransformFromPosition(-19.89, -15.81, 5.81)),
					ModuleTransform("DecorativeBasketBox", TransformFromPositionYawScale(-16.5, 27.13, 0, 133, 0.07)),
					ModuleTransform("DecorativeBasketRound", TransformFromPositionScale(7.47, 17.37, 0, 0.08)),
					ModuleTransform("DecorativeSack", TransformFromPositionYaw(-0.4, 1.66, -1.03, 141)),
					ModuleTransform("DecorativeSack", TransformFromPositionYaw(3.09, 3.67, -1.03, 142)),
					ModuleTransform("DecorativeSack", TransformFromPositionYaw(1.09, 2.94, 0.22, 142)),
				},
				{
					{ParticleEnum::Smoke, TransformFromPosition(34.99, -25.34, 51.55)},
					{ParticleEnum::CampFire, TransformFromPositionYawScale(-10.88, 8.19, 0.37, 0, 0.17)}
				},
				{}, {},
				{{0.12f, 35.0f, FLinearColor(1, 0.527f, 0.076f), FVector(-10.88, 8.19, 8.5), FVector::OneVector}}
			),
			// 5
			ModuleTransforms::CreateSet("TownhallLvl5", { ModuleTransform("TownhallLvl5GardenAndSpire", TransformFromPositionYaw(-15, -1.35, -2.364, 0)) }),
		},

		{ // Storage yard
			ModuleTransforms({}),
		},

		{ // Gold Mine
			ModuleTransforms::CreateOreMineSet("OreMineSpecial_Gold", "OreMineWorkStatic_Gold")
		},

		{ // Quarry
			ModuleTransforms::CreateSet("Quarry", {}, {
					//{ParticleEnum::BlackSmoke, TransformFromPosition(-.27, 11.4, 21.6)},
				}, {
					//ModuleTransform("OreMineWorkRotation2", TransformFromPosition(4.99, -8.10, 22.899), 0.0f, ModuleTypeEnum::RotateRoll),
					ModuleTransform("QuarrySpecialToggle", FTransform::Identity, 0, ModuleTypeEnum::ShaderOnOff),
				},
				{
					ModuleTransform("OreMineWorkStatic_Stone"),
				}
			),
		},


		{ ModuleTransforms({ ModuleTransform("IronStatue")}) },

		{ // Bank
			ModuleTransforms::CreateSet("Bank", {}, {})
		},

		{ ModuleTransforms({ ModuleTransform("TempleGrograth")}) },

		{ // Farm
			ModuleTransforms::CreateSet("Farm")
		},


		{ ModuleTransforms({
			ModuleTransforms::CreateSet("MushroomHut", {}, {},
				{
					ModuleTransform("MushroomHutWorkShaderAnimate", FTransform::Identity, 0.0f, ModuleTypeEnum::ShaderAnimate),
				}
			)
		}) },

		{ // Fence 
			ModuleTransforms({ ModuleTransform("FenceFour", FTransform::Identity, 1.0f, ModuleTypeEnum::Frame)}),
			ModuleTransforms({ ModuleTransform("FenceThree", FTransform::Identity, 1.0f, ModuleTypeEnum::Frame)}),
			ModuleTransforms({ ModuleTransform("FenceOpposite", FTransform::Identity, 1.0f, ModuleTypeEnum::Frame)}),
			ModuleTransforms({ ModuleTransform("FenceAdjacent", FTransform::Identity, 1.0f, ModuleTypeEnum::Frame)}),
		},
		{ ModuleTransforms({ ModuleTransform("FenceGate", FTransform::Identity, 1.0f, ModuleTypeEnum::Frame)}) },
		{ // Bridge
			ModuleTransforms({ ModuleTransform("Bridge1", FTransform::Identity, 1.0f, ModuleTypeEnum::Frame)}),
		},

		{ // Forester
			ModuleTransforms::CreateSet("Forester", {}, {{ParticleEnum::Smoke, TransformFromPosition(9.5, 10.2, 29.5)}})
		},

		{ // Coal Mine
			ModuleTransforms::CreateOreMineSet("OreMineSpecial_Coal", "OreMineWorkStatic_Coal")
		},
		{ // Iron Mine
			ModuleTransforms::CreateOreMineSet("OreMineSpecial_Iron", "OreMineWorkStatic_Iron")
		},

		{ ModuleTransforms({ ModuleTransform("SmallMarket")}) },
			
		{ // Paper Maker
			ModuleTransforms::CreateSet("PaperMaker")
		},

		{ // Iron Smelter
			ModuleTransforms::CreateSet("Smelter", {},
				{
					{ParticleEnum::BlackSmoke, TransformFromPosition(10.7, -11.1, 37.4)},
				}, {},
				{
					ModuleTransform("SmelterWorkStatic"),
				}
			)
		},


		{ // Stone tool shop
			ModuleTransforms::CreateSet("StoneToolShop")
		},

		{ // Blacksmith
			ModuleTransforms::CreateSet("Blacksmith")
		},

		{ // Herbalist
			ModuleTransforms::CreateSet("Herbalist")
		},

		{ // MedicineMaker
			ModuleTransforms::CreateSet("MedicineMaker")
		},



		{ // Furniture
			ModuleTransforms::CreateSet("FurnitureMaker", {}, {},
			{
				ModuleTransform("FurnitureMakerWorkRotation1", TransformFromPosition(-10.208, 1.62, 6.62), 0.0f, ModuleTypeEnum::RotateRoll),
				ModuleTransform("FurnitureMakerWorkRotation2", TransformFromPosition(-3.311, -1.472, 10.866), 0.0f, ModuleTypeEnum::RotateRoll),
			})
		},

		{ // Chocolatier
			ModuleTransforms::CreateSet("Chocolatier")
		},

		{ // Garden
			ModuleTransforms::CreateSet("Garden")
		},


		{ ModuleTransforms({ ModuleTransform("BoarBurrow")}) },

		{ ModuleTransforms({ ModuleTransform("DirtRoad")}) },
		{ ModuleTransforms({ ModuleTransform("StoneRoad")}) },
		{ ModuleTransforms({ ModuleTransform("TrapSpike", FTransform::Identity, 10.0f, ModuleTypeEnum::Frame)}) },

		{ // Fisher
			ModuleTransforms::CreateSet("Fisher", {}, {}, {},
				{
					ModuleTransform("FisherMarlin", FTransform(FRotator(0, 90, 90), FVector(2.88, 17.93, 7.51), FVector::OneVector)),

					ModuleTransform("DecorativeBarrel", TransformFromPosition(-23.69, 10.9, -0.21)),
					ModuleTransform("DecorativeBarrel", TransformFromPosition(-23.69, 14.69, -0.21)),
					ModuleTransform("DecorativeBarrel", TransformFromPosition(-7.159, 10.15, -0.21)),
					ModuleTransform("DecorativeBarrel", TransformFromPosition(-2.2, -8.5, -0.21)),
				}
			)
		},
		{ ModuleTransforms({ ModuleTransform("BlossomShrine")}) },

		{ // Winery
			ModuleTransforms::CreateSet("Winery", {}, {{ParticleEnum::Smoke, TransformFromPosition(16.2, -16.6, 21.3)}})
		},

		{ // Library
			ModuleTransforms::CreateSet("Library")
		},
		{ // School
			ModuleTransforms::CreateSet("Library")
		},

		{ // Theatre
			ModuleTransforms::CreateSet("Theatre")
		},
		{ // Tavern
			ModuleTransforms::CreateSet("Tavern", {}, {{ParticleEnum::Smoke, TransformFromPosition(12.2, -10.9, 20.1)}})
		},

		{ // Tailor
			ModuleTransforms::CreateSet("Tailor", {}, {{ParticleEnum::Smoke, TransformFromPosition(9.6, 29.3, 22.5)}})
		},

		{ // Charcoal maker
			ModuleTransforms::CreateSet("CharcoalMaker", {},
			{
				{ParticleEnum::BlackSmoke,  TransformFromPosition(9.8, 5.9, 5.6)},
				{ParticleEnum::TorchFire,  FTransform(FRotator::ZeroRotator, FVector(9.11, 5.27, 8.2), FVector(1, 1, 1))},
			}
			)
		},

		{ // Beer Brewery
			ModuleTransforms::CreateSet("BeerBrewery", {},
			{
				{ParticleEnum::Smoke,  TransformFromPosition(13.37, -16.77, 23.05)},
			}, {
				ModuleTransform("BeerBreweryWorkShaderOnOff", FTransform::Identity, 0, ModuleTypeEnum::ShaderOnOff),
			})
		},

		{ // Clay Pit
			ModuleTransforms::CreateSet("ClayPit")
		},

		{ // Potter
			ModuleTransforms::CreateSet("Potter", {},
			{
				{ParticleEnum::BlackSmoke,  TransformFromPosition(-15.8, -8.77, 7.78)},
			}, {})
		},

		{ ModuleTransforms({ ModuleTransform("HolySlimeRanch")}) },



		{ // Trading Post
			ModuleTransforms::CreateSet("TradingPost")
		},

		{ // Trading Company
			ModuleTransforms::CreateSet("TradingCompany")
		},
		{ // Trading Port
			ModuleTransforms::CreateSet("TradingPort")
		},
		{ // Card Maker
			ModuleTransforms::CreateSet("CardMaker")
		},
		{ // Immigration Office
			ModuleTransforms::CreateSet("ImmigrationOffice")
		},

		{ ModuleTransforms({ ModuleTransform("ThiefGuild")}) },
		{ ModuleTransforms({ ModuleTransform("SlimePyramid")}) },

		{ ModuleTransforms({ ModuleTransform("LovelyHeart")}) },

		{ // Hunter
			ModuleTransforms::CreateSet("Hunter")
		},

		{ // Ranch Barn
			ModuleTransforms::CreateSet("RanchBarn"),
		},

		{ // Ranch Pig
			ModuleTransforms::CreateSet("Ranch"),
		},
		{ // Ranch Sheep
			ModuleTransforms::CreateSet("Ranch"),
		},
		{ // Ranch Cow
			ModuleTransforms::CreateSet("Ranch"),
		},


		{ // Gold Smelter
			ModuleTransforms::CreateSet("SmelterGold", {},
				{
					{ParticleEnum::BlackSmoke, TransformFromPosition(10.6, -11.4, 31.2)},
				}, {},
				{
					ModuleTransform("SmelterGoldWorkStatic"),
				}
			)
		},

		{ ModuleTransforms({
			ModuleTransform("MintFrames", FTransform::Identity, 10.0f, ModuleTypeEnum::Frame),
			ModuleTransform("MintFloor"),

			ModuleTransform("MintMeltPot"),
			ModuleTransform("MintMoltenGold"),
			ModuleTransform("MintBody"),
			ModuleTransform("MintRoof"),
			ModuleTransform("MintRoofEdge"),
			ModuleTransform("MintStoneBody"),
			ModuleTransform("MintFramesAux"),
		}) }, // Mint

		{ // Clubman Barrack
			ModuleTransforms::CreateSet("Barrack"),
		},
		{ // Swordman Barrack
			ModuleTransforms::CreateSet("Barrack"),
		},
		{ // Archer Barrack
			ModuleTransforms::CreateSet("Barrack"),
		},

		{ // Shrine Hope
			ModuleTransforms::CreateSet("ShrineBasic"),
		},
		{ // Shrine Love
			ModuleTransforms::CreateSet("ShrineBasic"),
		},
		{ // Shrine Greed
			ModuleTransforms::CreateSet("ShrineBasic2"),
		},

		//{ ModuleTransforms({ ModuleTransform("ShrineRot")}) },
		//{ ModuleTransforms({ ModuleTransform("ShrineFrost")}) },
		{ ModuleTransforms({ ModuleTransform("HellPortal")}) },

		{ // LaborerGuild
			ModuleTransforms::CreateSet("LaborerGuild", {}, {})
		},

		{ // Humanitarian aid camp
			ModuleTransforms::CreateSet("StorageYard"),
		},

		{ // RegionTribalVillage
			ModuleTransforms::CreateSet("TribalVillage", {},
				{
					{ParticleEnum::CampFire, TransformFromPositionYawScale(-5.4, -0.82, 0.62, 0, 0.17)}
				},
				{}, {},
				{{0.12f, 35.0f, FLinearColor(1, 0.527f, 0.076f), FVector(-5.4, -0.82, 8.5), FVector::OneVector}}
			),
		},
		{ // RegionShrine
			ModuleTransforms::CreateSet("AncientShrine"),
		},
		{ // RegionPort
			ModuleTransforms::CreateSet("PortVillage"),
		},
		{ // RegionCrates
			ModuleTransforms::CreateSet("RegionCratePile"),
		},


		/*
		 * June 1 Additions
		 */
		{ // Windmill
			ModuleTransforms::CreateSet("Windmill", {}, {},
			{
				ModuleTransform("WindmillWorkRotation1", TransformFromPosition(0, 0, 60), 0.0f, ModuleTypeEnum::RotateRoll),
			})
			//ModuleTransforms::CreateSet("Windmill")
		},
		{ // Bakery
			ModuleTransforms::CreateSet("Bakery")
		},
		{ // GemstoneMine
			ModuleTransforms::CreateOreMineSet("OreMineSpecial_Gemstone", "OreMineWorkStatic_Gemstone")
		},
		{ // Jeweler
			ModuleTransforms::CreateSet("Jeweler")
		},

		/*
		 * June 9
		 */
		{ // Beekeeper
			ModuleTransforms::CreateSet("Beekeeper")
		},
		{ // Brickworks
			ModuleTransforms::CreateSet("Brickworks")
		},
		{ // CandleMaker
			ModuleTransforms::CreateSet("CandleMaker")
		},
		{ // CottonMill
			ModuleTransforms::CreateSet("CottonMill")
		},
		{ // PrintingPress
			ModuleTransforms::CreateSet("PrintingPress")
		},

		/*
		 * June 25
		 */
		{ // Warehouse
			ModuleTransforms::CreateSet("Warehouse")
		},
		{ // Fort
			ModuleTransforms::CreateSet("Outpost")
		},
		{ // Resource Outpost
			ModuleTransforms::CreateSet("Colony")
		},
		{ // InventorsWorkshop
			ModuleTransforms::CreateSet("InventorsWorkshop")
		},
			
		// Intercity Road
		{ ModuleTransforms({ ModuleTransform("DirtRoad")}) },


		/*
		 * August 16
		 */
		{ // FakeTownhall
			ModuleTransforms::CreateSet("Townhall0")
		},
		{ // FakeTribalVillage
			ModuleTransforms::CreateSet("TribalVillage", {},
				{
					{ParticleEnum::CampFire, TransformFromPositionYawScale(-5.4, -0.82, 0.62, 0, 0.17)}
				},
				{}, {},
				{{0.12f, 35.0f, FLinearColor(1, 0.527f, 0.076f), FVector(-5.4, -0.82, 8.5), FVector::OneVector}}
			)
		},
		{ // ChichenItza
			ModuleTransforms::CreateSet("ChichenItza")
		},

		/*
		 * October 20
		 */
		{ // Market
			ModuleTransforms::CreateSet("Market", {}, {})
		},
		{ // ShippingDepot
			ModuleTransforms::CreateSet("ShippingDepot", {}, {})
		},
		{ // IrrigationReservoir
			ModuleTransforms::CreateSet("IrrigationReservoir", {}, {})
		},

		/*
		 * Nov 18
		 */
		{ // Tunnel
			ModuleTransforms({ ModuleTransform("Tunnel", FTransform::Identity, 1.0f, ModuleTypeEnum::Frame)}),
		},
		{ // Garment Factory
			ModuleTransforms::CreateSet("CottonMill")
		},

		/*
		 * Dec 29
		 */
		// Shroom Farm
		{ ModuleTransforms({
			ModuleTransforms::CreateSet("ShroomHut", {}, {},
				{
					ModuleTransform("ShroomHutWorkShaderAnimate", FTransform::Identity, 0.0f, ModuleTypeEnum::ShaderAnimate),
				}
			)
		}) },
		{ // Vodka Distillery
			ModuleTransforms::CreateSet("VodkaDistillery", {},
			{
				{ParticleEnum::Smoke,  TransformFromPosition(13.37, -16.77, 23.05)},
			}, {
				ModuleTransform("VodkaDistilleryWorkShaderOnOff", FTransform::Identity, 0, ModuleTypeEnum::ShaderOnOff),
			})
		},
		{ // Coffee Roaster
			ModuleTransforms::CreateSet("CoffeeRoaster", {}, {{ParticleEnum::Smoke, TransformFromPosition(16.2, -16.6, 21.3)}})
		},

		/*
		 * Feb 2
		 */
		{ // Colony
			ModuleTransforms::CreateSet("Townhall0")
		},
		{ // PortColony
			ModuleTransforms::CreateSet("Townhall0")
		},
		{ // Intercity Logistics Hub
			ModuleTransforms::CreateSet("IntercityLogisticsHub", {}, {})
		},
		{ // Intercity Logistics Port
			ModuleTransforms::CreateSet("IntercityLogisticsPort", {}, {})
		},
		{ // Intercity Bridge
			ModuleTransforms({ ModuleTransform("Bridge1", FTransform::Identity, 1.0f, ModuleTypeEnum::Frame)}),
		},

		/*
		 * Mar 12
		 */
		{ // Granary
			ModuleTransforms::CreateSet("Granary_Era4_")
		},
		{ // Archives
			ModuleTransforms::CreateSet("Archives_Era4_")
		},
		{ // Hauling Services
			ModuleTransforms::CreateSet("HaulingServices_Era2_")
		},

		/*
		 * Apr 1
		 */
		{ // SandMine
		ModuleTransforms::CreateSet("Ministry")
		},
		{ // Glasswork
		ModuleTransforms::CreateSet("Ministry")
		},
		{ // CoalPowerPlant
		ModuleTransforms::CreateSet("Ministry")
		},
		{ // Steelworks
		ModuleTransforms::CreateSet("Ministry")
		},
		{ // StoneToolsShop
		ModuleTransforms::CreateSet("Ministry")
		},
		{ // OilWell
		ModuleTransforms::CreateSet("Ministry")
		},
		{ // OilPowerPlant
		ModuleTransforms::CreateSet("Ministry")
		},
		{ // PaperMill
		ModuleTransforms::CreateSet("Ministry")
		},
		{ // ClockMakers
		ModuleTransforms::CreateSet("Ministry")
		},

		{ // Cathedral
		ModuleTransforms::CreateSet("Ministry")
		},
		{ // Castle
		ModuleTransforms::CreateSet("Ministry")
		},
		{ // GrandMuseum
		ModuleTransforms::CreateSet("Ministry")
		},
		{ // ExhibitionHall
		ModuleTransforms::CreateSet("Ministry")
		},

			
		/*
		 * Decorations
		 */
		{ // FlowerBed
			ModuleTransforms::CreateSet("FlowerBed")
		},
		{ // GardenShrubbery1
			ModuleTransforms::CreateSet("GardenShrubbery1")
		},
		{ // GardenCypress
			ModuleTransforms::CreateSet("GardenCypress")
		},


		{ // ArchitectStudio
			ModuleTransforms::CreateSet("Ministry", {}, {})
		},
		{ // EngineeringOffice
			ModuleTransforms::CreateSet("Ministry", {}, {})
		},
		{ // DepartmentOfAgriculture
			ModuleTransforms::CreateSet("Ministry", {}, {})
		},

		{ // AdventurerGuild
			ModuleTransforms::CreateSet("Ministry", {}, {})
		},
		{ // CensorshipInstitute
			ModuleTransforms::CreateSet("Ministry", {}, {})
		},
		{ // EnvironmentalistGuild
			ModuleTransforms::CreateSet("Ministry", {}, {})
		},
		{ // IndustrialistsGuild
			ModuleTransforms::CreateSet("Ministry", {}, {})
		},
		{ // Oracle
			ModuleTransforms::CreateSet("Ministry", {}, {})
		},
		{ // Venture Capital
			ModuleTransforms::CreateSet("Ministry", {}, {})
		},


		{ // Consulting
			ModuleTransforms::CreateSet("Ministry", {}, {})
		},
		{ // ImmigrationPropaganda
			ModuleTransforms::CreateSet("Ministry", {}, {})
		},
		{ // MerchantGuild
			ModuleTransforms::CreateSet("Ministry", {}, {})
		},
		{ // OreSupplier
			ModuleTransforms::CreateSet("Ministry", {}, {})
		},
		{ // BeerBreweryFamous
			ModuleTransforms::CreateSet("BeerBreweryFamous", {},
			{
				{ParticleEnum::Smoke,  TransformFromPosition(19, 0, 23)},
			}, {
				ModuleTransform("BeerBreweryFamousWorkShaderOnOff", FTransform::Identity, 0, ModuleTypeEnum::ShaderOnOff),
			})
		},
		{ // Giant Iron Smelter
			ModuleTransforms::CreateSet("SmelterGiant", {},
				{
					{ParticleEnum::BlackSmoke, TransformFromPositionScale(16.9, -9.96, 59.5, 1.5)},
				}, {},
				{
					ModuleTransform("SmelterGiantWorkStatic"),
				}
			)
		},

		{ // Cattery
			ModuleTransforms::CreateSet("Library", {}, {})
		},
		{ // InvestmentBank
			ModuleTransforms::CreateSet("Ministry", {}, {})
		},

		{ // Statistics Bureau
			ModuleTransforms::CreateSet("Ministry", {}, {})
		},
		{ // JobManagementBureau
			ModuleTransforms::CreateSet("Ministry", {}, {})
		},
			
		};
	}

	static bool IsMiniModule(FString submeshName)
	{
		static const TArray<FString> miniModuleList
		{
			"Body",
			"Roof",
			"Frame",
			"WindowFrame",
			"WindowGlass",
			"Special",
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
		InitBuildingSets();

		// For each building
		for (size_t buildingEnumInt = BuildingEnumToVariationToModuleTransforms.size(); buildingEnumInt-- > 0;) 
		{
			auto& variationToModuleTransforms = BuildingEnumToVariationToModuleTransforms[buildingEnumInt];

			// For each variations specified
			for (auto& moduleTransforms : variationToModuleTransforms) 
			{
				//UE_LOG(LogTemp, Error, TEXT("LoadBuildingSets %d %s %s"), buildingEnumInt, ToTChar(GetBuildingInfoInt(buildingEnumInt).name), ToTChar(moduleTransforms.setName));
				
				if (moduleTransforms.setName != "")
				{
					// Load building set and replace this moduleTransform with building set..
					FString setName = ToFString(moduleTransforms.setName);

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
						FString subMeshName = setName + FString("SpecialInstant");
						if (assetLoader->moduleMesh(subMeshName)) addTransform(ModuleTransform(subMeshName, FTransform::Identity, 0.0f));

						subMeshName = setName + FString("Frame");
						if (assetLoader->moduleMesh(subMeshName)) addTransform(ModuleTransform(subMeshName, FTransform::Identity, 10.0f, ModuleTypeEnum::Frame));

						subMeshName = setName + FString("Chimney");
						if (assetLoader->moduleMesh(subMeshName)) addTransform(ModuleTransform(subMeshName, FTransform::Identity, 1.0f));

						subMeshName = setName + FString("Floor");
						if (assetLoader->moduleMesh(subMeshName)) addTransform(ModuleTransform(subMeshName, FTransform::Identity, 0.5f));

						subMeshName = setName + FString("Body");
						if (assetLoader->moduleMesh(subMeshName)) {
							addTransform(ModuleTransform(subMeshName));
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

							//// Special case where Special is the core structure
							//CardEnum cardEnum = static_cast<CardEnum>(buildingEnumInt);
							//if (cardEnum == CardEnum::RegionCrates ||
							//	cardEnum == CardEnum::RegionShrine) 
							//{
							//	miniTransforms.push_back(ModuleTransform(subMeshName));
							//}
						}

						subMeshName = setName + FString("Special2");
						if (assetLoader->moduleMesh(subMeshName)) addTransform(ModuleTransform(subMeshName));

						subMeshName = setName + FString("Special3");
						if (assetLoader->moduleMesh(subMeshName)) addTransform(ModuleTransform(subMeshName));

						subMeshName = setName + FString("Special4");
						if (assetLoader->moduleMesh(subMeshName)) addTransform(ModuleTransform(subMeshName));

						subMeshName = setName + FString("Special5");
						if (assetLoader->moduleMesh(subMeshName)) addTransform(ModuleTransform(subMeshName));
						

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

	}

	const ModuleTransforms& GetDisplayModules(CardEnum buildingEnum, int32 variationIndex) const {
		int buildingEnumInt = static_cast<int>(buildingEnum);
		PUN_CHECK(buildingEnumInt < BuildingEnumToVariationToModuleTransforms.size()); // !!! Hit here? Forgot to put int GameDisplayInfo?
		PUN_CHECK(variationIndex != -1);
		PUN_CHECK(variationIndex < BuildingEnumToVariationToModuleTransforms[buildingEnumInt].size());
		return BuildingEnumToVariationToModuleTransforms[buildingEnumInt][variationIndex];
	}

private:
	std::vector<std::vector<ModuleTransforms>> BuildingEnumToVariationToModuleTransforms;
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