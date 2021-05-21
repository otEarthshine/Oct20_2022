#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "UnrealEngine.h"
#include "UObject/ConstructorHelpers.h"

#include "PunCity/Simulation/GameSimulationInfo.h"
#include <unordered_map>
#include <string>
#include "Materials/MaterialInstance.h"
#include "Animation/AnimSequence.h"

#include "Niagara/Public/NiagaraComponent.h"

#include "PunUnrealUtils.h"
#include "CppUtils.h"

#include "AssetLoaderComponent.generated.h"


/*
 * GameDisplayInfo
 */

 // Resolves overlay setting conflict
enum class OverlaySetterType {
	BuildingPlacement,
	ObjectDescriptionUI,
	OverlayToggler,
	Count,
	None,
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
	FrameConstructionOnly,
	Window,
	RotateRoll,
	ShaderAnimate,
	ShaderOnOff,
	AlwaysOn,
};

static bool IsModuleTypeFrame(ModuleTypeEnum moduleTypeEnum)
{
	return moduleTypeEnum == ModuleTypeEnum::Frame || 
		moduleTypeEnum == ModuleTypeEnum::FrameConstructionOnly;
}
static bool IsModuleTypeConstructionOnly(ModuleTypeEnum moduleTypeEnum)
{
	return moduleTypeEnum == ModuleTypeEnum::ConstructionOnly ||
		moduleTypeEnum == ModuleTypeEnum::FrameConstructionOnly;
}

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


struct ModuleTransformGroup
{
	std::vector<ModuleTransform> transforms; //TODO: change name to modules?
	std::vector<ModuleTransform> miniModules; // For far zoom

	std::vector<ParticleInfo> particleInfos;

	std::vector<ModuleTransform> animTransforms;
	std::vector<ModuleTransform> togglableTransforms;

	std::vector<PointLightInfo> lightInfos;

	FString setName; // For building name etc.

	static const ModuleTransformGroup Empty;

	ModuleTransformGroup() {}
	
	ModuleTransformGroup(std::vector<ModuleTransform> transforms,
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

	static ModuleTransformGroup CreateSet(FString setName,
		std::vector<ModuleTransform> transforms = {},
		std::vector<ParticleInfo> particleInfos = {},
		std::vector<ModuleTransform> animTransforms = {},
		std::vector<ModuleTransform> togglableTransforms = {},
		std::vector<PointLightInfo> lightInfos = {}) // Note: animTransforms must be set manually since we need its locations
	{
		ModuleTransformGroup result(transforms, particleInfos, animTransforms, togglableTransforms, lightInfos);
		result.setName = setName;
		return result;
	}

	static ModuleTransformGroup CreateAuxSet(
		std::vector<ParticleInfo> particleInfos = {},
		std::vector<ModuleTransform> animTransforms = {},
		std::vector<ModuleTransform> togglableTransforms = {},
		std::vector<PointLightInfo> lightInfos = {}) // Note: animTransforms must be set manually since we need its locations
	{
		return ModuleTransformGroup({}, particleInfos, animTransforms, togglableTransforms, lightInfos);
	}

	static ModuleTransformGroup CreateSet(FString setName, ModuleTransformGroup auxSet) // Note: animTransforms must be set manually since we need its locations
	{
		ModuleTransformGroup result(auxSet.transforms, auxSet.particleInfos, auxSet.animTransforms, auxSet.togglableTransforms, auxSet.lightInfos);
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


	static ModuleTransformGroup CreateOreMineSet(FString oreMineSpecialName, FString oreMineWorkStaticName)
	{
		return CreateSet("OreMine", {}, 
			{
				{ParticleEnum::BlackSmoke, TransformFromPosition(-.27, 11.4, 21.6)},
			}, 
			{
				ModuleTransform("OreMineWorkRotation2", TransformFromPosition(4.99, -8.10, 22.899), 0.0f, ModuleTypeEnum::RotateRoll),
				ModuleTransform(oreMineSpecialName, FTransform::Identity, 0, ModuleTypeEnum::ShaderOnOff),
			},
			{
				ModuleTransform(oreMineWorkStaticName),
			}
		);
	}
};

/*
 *
 *
 * 
 */

enum class TileSubmeshEnum
{
	Trunk,
	Leaf,
	Fruit,
	LeafLowPoly,
	LeafShadow,
	Stump,
};

const std::string TileSubmeshName[]
{
	"Trunk",
	"Leaf",
	"Fruit",
	"LeafLowPoly",
	"LeafShadow",
	"Stump",
};
static const int32 TileSubmeshCount = _countof(TileSubmeshName);



USTRUCT()
struct FUnitAsset
{
	GENERATED_BODY();
	
	UPROPERTY() USkeletalMesh* skeletalMesh = nullptr;
	UPROPERTY() TMap<UnitAnimationEnum, UAnimSequence*> animationEnumToSequence;

	// Static part is used when the meshes should be faraway (or if there is only staticMesh for this unit like ship)
	//  Also for collider??
	UPROPERTY() UStaticMesh* staticMesh = nullptr; // default mesh (except for skeletalMesh usage when zoomed in)

	bool isValid() { return staticMesh != nullptr; }
};

//USTRUCT()
//struct FBuildingAsset
//{
//	GENERATED_BODY();
//
//	UPROPERTY() TMap<FString, UStaticMesh*> bodyMeshes;
//	UPROPERTY() TMap<FString, UStaticMesh*> scaffoldingMeshes;
//	UPROPERTY() TMap<FString, UStaticMesh*> windowsMeshes;
//
//	bool isValid() { return bodyMeshes.Num() > 0 || scaffoldingMeshes.Num() > 0 || windowsMeshes.Num() > 0; }
//};

USTRUCT()
struct FTileMeshAssets
{
	GENERATED_BODY();
	UPROPERTY() TArray<UStaticMesh*> assets;

	UPROPERTY() TArray<UMaterialInstanceDynamic*> materials;
};

USTRUCT()
struct FGeoresourceMeshAssets
{
	GENERATED_BODY();
	UPROPERTY() TArray<UStaticMesh*> miniMeshes;
	UPROPERTY() TArray<UStaticMesh*> meshes;

	bool isValid() { return miniMeshes.Num() > 0; }
};

UCLASS( ClassGroup=(Custom), meta=(BlueprintSpawnableComponent) )
class UAssetLoaderComponent : public UActorComponent
{
	GENERATED_BODY()
public:	
	UAssetLoaderComponent();
	void InitNiagara();

	void PaintConstructionMesh();
	void UpdateRHIConstructionMesh();
	void PrintConstructionMesh();
	void RemoveVertexColor();

private:
	void TraverseTris(uint32 groupIndex_PositionMerge, int32 groupIndex_Parent, const TArray<uint32>& indexBuffer, const TArray<FVector>& vertexPositions);

public:
	UStaticMesh* moduleMesh(FString moduleName);

	//FBuildingAsset buildingAsset(FString buildingAssetName) {
	//	if (_buildingNameToAsset.Contains(buildingAssetName)) {
	//		return _buildingNameToAsset[buildingAssetName];
	//	}
	//	return FBuildingAsset();
	//}

	//bool HasConstructionMesh(FString moduleName) { return false; } // _moduleNameToConstructionMesh.Contains(moduleName);

	//! Module types are used to create separate mesh sets
	const TArray<FString>& moduleNames()
	{
		//PUN_CHECK(_moduleNames.Num() == ModuleMeshCount);
		return _moduleNames;
	}
	TArray<FString>& animModuleNames() { return _animModuleNames; }
	TArray<FString>& togglableModuleNames() { return _togglableModuleNames; }


	TArray<TArray<ModuleTransformGroup>> buildingEnumToVariationToModuleTransforms() { return _buildingEnumToModuleGroups; }
	int32 GetMinEraDisplay(CardEnum buildingEnum) { return _buildingEnumToMinEraModel[static_cast<int>(buildingEnum)]; }
	

	//UStaticMesh* unitMesh(UnitEnum unitEnum, int32 variationIndex = 0);
	int32 unitMeshCount(UnitEnum unitEnum) {
		return _unitEnumToAsset[static_cast<int>(unitEnum)].size();
		
		//if (unitEnum == UnitEnum::Human) {
		//	return _unitEnumToAsset[static_cast<int>(unitEnum)].size();
		//}
		//return _unitToMeshes[unitEnum].size();
	}

	FUnitAsset unitAsset(UnitEnum unitEnum, int32 variationIndex = 0);
	
	UStaticMesh* unitWeaponMesh(UnitAnimationEnum animationEnum) {
		auto found = _animationEnumToWeaponMesh.find(animationEnum);
		if (found != _animationEnumToWeaponMesh.end()) {
			return found->second;
		}
		return nullptr;
	}
	UStaticMesh* unitAuxMesh(UnitAnimationEnum animationEnum) {
		auto found = _animationEnumToAuxMesh.find(animationEnum);
		if (found != _animationEnumToAuxMesh.end()) {
			return found->second;
		}
		return nullptr;
	}

	UStaticMesh* resourceMesh(ResourceEnum resourceEnum);
	UStaticMesh* resourceHandMesh(ResourceEnum resourceEnum);

	FGeoresourceMeshAssets georesourceMesh(GeoresourceEnum georesourceEnum);

	UParticleSystem* particleSystem(ParticleEnum particleEnum) { return ParticlesByEnum[static_cast<int>(particleEnum)]; }
	UNiagaraSystem* niagaraSystem(ParticleEnum particleEnum)
	{
		if (NiagaraByEnum.Num() == 0) {
			InitNiagara();
		}
		return NiagaraByEnum[static_cast<int>(particleEnum) - static_cast<int>(ParticleEnum::DemolishDust) - 1];
	}

	UTexture2D* GetBuildingIcon(CardEnum buildingEnum) { return _buildingIcons[static_cast<int>(buildingEnum)]; }
	UTexture2D* GetBuildingIconAlpha(CardEnum buildingEnum) { return _buildingIconsAlpha[static_cast<int>(buildingEnum)]; }
	bool IsBuildingUsingSpecialIcon(CardEnum buildingEnum) {
		return _buildingsUsingSpecialIcon.Contains(buildingEnum);
	}
	
	
	UTexture2D* GetResourceIcon(ResourceEnum resourceEnum) { return _resourceIcons[static_cast<int>(resourceEnum)]; }
	UTexture2D* GetResourceIconAlpha(ResourceEnum resourceEnum) { return _resourceIconsAlpha[static_cast<int>(resourceEnum)]; }
	UMaterialInstanceDynamic* GetResourceIconMaterial(ResourceEnum resourceEnum);
	

	UTexture2D* GetCardIcon(CardEnum cardEnum) {
		if (_cardIcons.Contains(static_cast<int32>(cardEnum))) {
			return _cardIcons[static_cast<int32>(cardEnum)];
		}
		return _cardIcons[static_cast<int32>(CardEnum::None)];
	}

	UTexture2D* GetArmyIcon(ArmyEnum armyEnum) { return _armyIcons[static_cast<int>(armyEnum)]; }

	bool HasGeoIcon(GeoresourceEnum geoEnum) { return _geo_Icon.Contains(static_cast<int32>(geoEnum)); }
	UTexture* GetGeoIcon(GeoresourceEnum geoEnum) { return _geo_Icon[static_cast<int32>(geoEnum)]; }
	UTexture* GetGeoIconAlpha(GeoresourceEnum geoEnum) { return _geo_IconAlpha[static_cast<int32>(geoEnum)]; }
	
	//! Trees
	FTileMeshAssets tileMeshAsset(TileObjEnum treeEnum);

	void SetMaterialCollectionParametersScalar(FName ParameterName, float ParameterValue);
	void SetMaterialCollectionParametersVector(FName ParameterName, FLinearColor ParameterValue);

	//!!! Constructor loading of material with landscapeLayerBlend crashes...
	UMaterial* GetTerrainMaterial();

	/*
	 * Debug
	 */
	void CheckMeshesAvailable() {
		// Building Mesh Check
		check(_moduleNames.Num() + _togglableModuleNames.Num() + _animModuleNames.Num() == _moduleNameToMesh.Num()); // Doesn't work with old methods
		for (int32 i = 0; i < _moduleNames.Num(); i++) {
			check(_moduleNameToMesh.Contains(_moduleNames[i]))
		}
	}
	void CleanModuleNames() {
		_moduleNames.Empty();
		for (auto& it : _moduleNameToMesh) {
			if (!_togglableModuleNames.Contains(it.Key) &&
				!_animModuleNames.Contains(it.Key)) 
			{
				_moduleNames.Add(it.Key);
			}
		}
		for (int32 i = _moduleNames.Num(); i-- > 0;) {
			if (_moduleNameToMesh.Contains(_moduleNames[i])) {
				if (_moduleNameToMesh[_moduleNames[i]] == nullptr) {
					_moduleNameToMesh.Remove(_moduleNames[i]);
					_moduleNames.RemoveAt(i);
				}
				else {
					if (_moduleNames[i] == FString("ClayPit_Era1Special4")) {
						FString newName = FString("ClayPit_Era1Special1");
						_moduleNameToMesh.Add(newName, _moduleNameToMesh[_moduleNames[i]]);
						_moduleNameToMesh.Remove(_moduleNames[i]);
						_moduleNames[i] = newName;
					}
				}
			} else {
				_moduleNames.RemoveAt(i);
			}
		}
		
		check(_moduleNames.Num() + _togglableModuleNames.Num() + _animModuleNames.Num() == _moduleNameToMesh.Num());
	}

public:
	UPROPERTY(EditAnywhere, Category = "Mesh Import") UStaticMesh* GroundMesh;
	UPROPERTY(EditAnywhere, Category = "Material Import") UMaterial* TerrainMaterial;

	UPROPERTY(EditAnywhere, Category = "Mesh Import") UStaticMesh* WaterMesh;

	UPROPERTY(EditAnywhere, Category = "Mesh Import") UStaticMesh* PlayerFlagMesh;
	UPROPERTY(EditAnywhere, Category = "Material Import") UMaterial* PlayerFlagMaterial;

	UPROPERTY(EditAnywhere, Category = "Mesh Import") UStaticMesh* SelectionMesh;
	UPROPERTY(EditAnywhere, Category = "Material Import") UMaterial* SelectionMaterialGreen;
	UPROPERTY(EditAnywhere, Category = "Material Import") UMaterial* SelectionMaterialYellow;
	UPROPERTY(EditAnywhere) UStaticMesh* UpgradableMesh;
	UPROPERTY(EditAnywhere) UStaticMesh* AttentionCircle;
	UPROPERTY(EditAnywhere) UStaticMesh* AttentionMark;

	UPROPERTY(EditAnywhere, Category = "Mesh Import") UStaticMesh* WorldMapMesh;
	UPROPERTY(EditAnywhere, Category = "Mesh Import") UStaticMesh* WorldMapMeshSubdivide; // REMOVE?
	UPROPERTY(EditAnywhere, Category = "Mesh Import") UStaticMesh* WorldMapMeshWater;
	UPROPERTY(EditAnywhere, Category = "Mesh Import") UStaticMesh* WorldMapMeshWaterOutside;
	UPROPERTY(EditAnywhere, Category = "Material Import") UMaterial* M_HiddenInMainPass;
	
	UPROPERTY(EditAnywhere, Category = "Mesh Import") UStaticMesh* WorldMapCityBlock;
	UPROPERTY(EditAnywhere, Category = "Mesh Import") UStaticMesh* WorldMapFlag;
	UPROPERTY(EditAnywhere, Category = "Mesh Import") UStaticMesh* WorldMapFlagBase;
	UPROPERTY(EditAnywhere, Category = "Mesh Import") UStaticMesh* WorldMapCameraSpot;
	UPROPERTY(EditAnywhere, Category = "Material Import") UMaterial* WorldMapBlockMaterial;

	UPROPERTY(EditAnywhere) UStaticMesh* WorldMapForestMesh;

	UPROPERTY(EditAnywhere, Category = "Material Import") UMaterial* ReferenceTerrainMaterial;

	/*
	 * UI
	 */
	
	UPROPERTY(EditAnywhere) UMaterial* BuildingIconMaterial;
	UPROPERTY(EditAnywhere) UMaterial* BuildingIconGrayMaterial;

	UPROPERTY(EditAnywhere) UMaterial* CardIconMaterial;
	UPROPERTY(EditAnywhere) UMaterial* CardIconGrayMaterial;

	UPROPERTY(EditAnywhere) UTexture2D* CardFront;
	UPROPERTY(EditAnywhere) UTexture2D* CardFrontBevel;
	UPROPERTY(EditAnywhere) UTexture2D* CardFrontRound;
	UPROPERTY(EditAnywhere) UTexture2D* CardSlot;
	UPROPERTY(EditAnywhere) UTexture2D* CardSlotBevel;
	UPROPERTY(EditAnywhere) UTexture2D* CardSlotRound;

	UPROPERTY(EditAnywhere) UTexture2D* CardBack;

	UPROPERTY(EditAnywhere) UMaterial* ResourceIconMaterial;
	UPROPERTY(EditAnywhere) UMaterial* NoResourceIconMaterial;
	//UPROPERTY(EditAnywhere) UMaterial* M_HoverIcon;

	UPROPERTY(EditAnywhere) UTexture2D* HouseIcon;
	UPROPERTY(EditAnywhere) UTexture2D* SmileIcon;
	UPROPERTY(EditAnywhere) UTexture2D* UnhappyIcon;
	UPROPERTY(EditAnywhere) UTexture2D* CoinIcon;
	UPROPERTY(EditAnywhere) UTexture2D* InfluenceIcon;
	UPROPERTY(EditAnywhere) UTexture2D* ScienceIcon;

	UPROPERTY(EditAnywhere) UTexture2D* HappinessGreenIcon;
	UPROPERTY(EditAnywhere) UTexture2D* HappinessYellowIcon;
	UPROPERTY(EditAnywhere) UTexture2D* HappinessOrangeIcon;
	UPROPERTY(EditAnywhere) UTexture2D* HappinessRedIcon;
	UTexture2D* GetHappinessFace(int32 value)
	{
		int32 level = GetHappinessColorLevel(value);
		switch (level)
		{
		case 0: return HappinessRedIcon;
		case 1: return HappinessOrangeIcon;
		case 2: return HappinessYellowIcon;
		case 3: return HappinessGreenIcon;
		default:
			UE_DEBUG_BREAK();
			return HappinessRedIcon;
		}
	}

	UPROPERTY(EditAnywhere) UTexture2D* CoinGrayIcon;
	UPROPERTY(EditAnywhere) UTexture2D* ClockIcon;
	UPROPERTY(EditAnywhere) UTexture2D* ClockGrayIcon;

	UPROPERTY(EditAnywhere) UTexture2D* AdultIcon;
	UPROPERTY(EditAnywhere) UTexture2D* AdultIconSquare;
	UPROPERTY(EditAnywhere) UTexture2D* ChildIcon;
	UPROPERTY(EditAnywhere) UTexture2D* ExclamationIcon;

	//! Hover warning
	UPROPERTY(EditAnywhere) UMaterial* M_HoverWarning;
	UPROPERTY(EditAnywhere) UMaterial* M_HoverWarningHappiness;
	UPROPERTY(EditAnywhere) UTexture2D* WarningHouse;
	UPROPERTY(EditAnywhere) UTexture2D* WarningStarving;
	UPROPERTY(EditAnywhere) UTexture2D* WarningSnow;
	UPROPERTY(EditAnywhere) UTexture2D* WarningHealthcare;
	UPROPERTY(EditAnywhere) UTexture2D* WarningTools;
	UPROPERTY(EditAnywhere) UTexture2D* WarningUnhappy;
	

	//! HoverIcon
	UPROPERTY(EditAnywhere) UTexture2D* UnhappyHoverIcon;

	UPROPERTY(EditAnywhere) UTexture2D* BlackIcon;
	UPROPERTY(EditAnywhere) UTexture2D* WhiteIcon;

	UPROPERTY(EditAnywhere) UMaterial* M_GeoresourceIcon;
	UPROPERTY(EditAnywhere) TMap<int32, UTexture*> _geo_Icon;
	UPROPERTY(EditAnywhere) TMap<int32, UTexture*> _geo_IconAlpha;

	//! 
	UPROPERTY(EditAnywhere) UNiagaraSystem* NS_OnPlacement;
	UPROPERTY(EditAnywhere) UNiagaraSystem* NS_OnTownhall;
	UPROPERTY(EditAnywhere) UNiagaraSystem* NS_OnUpgrade;
	//!

	UPROPERTY(EditAnywhere, Category = "Mesh Import") UStaticMesh* EmptyMesh;

	UPROPERTY(EditAnywhere, Category = "Mesh Import") UStaticMesh* GatherMarkMeshMaterial;

	/**
	 * Placement
	 */

	UPROPERTY(EditAnywhere, Category = "Mesh Import") UStaticMesh* PlacementMesh;
	UPROPERTY(EditAnywhere, Category = "Material Import") UMaterial* PlacementMaterialGreen;
	UPROPERTY(EditAnywhere, Category = "Material Import") UMaterial* PlacementMaterialRed;
	UPROPERTY(EditAnywhere, Category = "Material Import") UMaterial* PlacementMaterialGray;
	UPROPERTY(EditAnywhere, Category = "Material Import") UMaterial* PlacementArrowMaterialGreen;
	UPROPERTY(EditAnywhere, Category = "Material Import") UMaterial* PlacementArrowMaterialYellow;
	UPROPERTY(EditAnywhere, Category = "Material Import") UMaterial* PlacementArrowMaterialRed;

	UPROPERTY(EditAnywhere, Category = "Mesh Import") UStaticMesh* DeliveryArrowMesh;
	UPROPERTY(EditAnywhere, Category = "Material Import") UMaterial* M_DeliveryArrow;
	
	/**
	 * Other Materials
	 */

	UPROPERTY(EditAnywhere, Category = "Material Import") UMaterial* TerritoryMaterial; // OLD
	UPROPERTY(EditAnywhere, Category = "Material Import") UMaterial* M_Territory;
	UPROPERTY(EditAnywhere, Category = "Material Import") UMaterial* M_Territory_Top;
	UPROPERTY(EditAnywhere, Category = "Material Import") UMaterial* M_Province;
	UPROPERTY(EditAnywhere, Category = "Material Import") UMaterial* M_Province_Top;

	
	UPROPERTY(EditAnywhere, Category = "Material Import") UMaterial* RegionBorderMaterial;
	UPROPERTY(EditAnywhere, Category = "Material Import") UMaterial* OverlayMaterial;
	UPROPERTY(EditAnywhere, Category = "Material Import") UMaterial* GridGuideMaterial;
	
	UPROPERTY(EditAnywhere, Category = "Material Import") UMaterial* RadiusMaterial;
	
	UPROPERTY(EditAnywhere, Category = "Material Import") UStaticMesh* RadiusMesh;
	UPROPERTY(EditAnywhere, Category = "Material Import") UMaterial* M_Radius;
	UPROPERTY(EditAnywhere, Category = "Material Import") UMaterialInstance* MI_RadiusRed;
	
	//UPROPERTY(EditAnywhere, Category = "Material Import") UMaterialInstanceDynamic* RadiusMaterialInstance;
	UPROPERTY(EditAnywhere, Category = "Material Import") UMaterialInstance* MI_RedRadius;

	UPROPERTY(EditAnywhere, Category = "Material Import") UMaterial* M_GoldOreDecal;
	UPROPERTY(EditAnywhere, Category = "Material Import") UMaterial* M_IronOreDecal;
	UPROPERTY(EditAnywhere, Category = "Material Import") UMaterial* M_CoalOreDecal;
	UPROPERTY(EditAnywhere, Category = "Material Import") UMaterial* M_GemstoneDecal;
	
	UPROPERTY(EditAnywhere, Category = "Material Import") UMaterial* FarmDecalMaterial;
	UPROPERTY(EditAnywhere, Category = "Material Import") UMaterial* ConstructionBaseDecalMaterial;

	UPROPERTY(EditAnywhere, Category = "Material Import") UMaterial* RoadMaterial;
	UPROPERTY(EditAnywhere, Category = "Material Import") UMaterial* WetnessMaterial;

	UPROPERTY(EditAnywhere, Category = "Material Import") UMaterial* M_PlainMaterial;

	UPROPERTY(EditAnywhere, Category = "Material Import") UMaterial* M_WoodFrameBurned;

	UPROPERTY(EditAnywhere, Category = "Material Import") UMaterial* M_TileHighlightDecal;
	UPROPERTY(EditAnywhere, Category = "Material Import") UMaterial* M_TileHighlightForMesh;
	UPROPERTY(EditAnywhere, Category = "Material Import") UMaterial* M_ConstructionHighlightDecal;
	
	UPROPERTY(EditAnywhere, Category = "Material Import") UMaterial* M_RegionHighlightDecal;
	UPROPERTY(EditAnywhere, Category = "Material Import") UMaterial* M_TerritoryHighlightForMesh;
	UPROPERTY(EditAnywhere, Category = "Material Import") UMaterial* M_RegionHighlightDecalFaded;
	UPROPERTY(EditAnywhere, Category = "Material Import") UMaterial* M_TerritoryHighlightForMeshFaded;

	
	UPROPERTY(EditAnywhere, Category = "Material Import") UMaterialInstance* HighlightMaterial;
	UPROPERTY(EditAnywhere, Category = "Material Import") UMaterial* M_TerritoryMapDecal;

	UPROPERTY(EditAnywhere, Category = "Material Import") UMaterial* TextMaterial;

	UPROPERTY(EditAnywhere, Category = "Material Import") UMaterialParameterCollection* collection;
	UPROPERTY(EditAnywhere, Category = "Weather") UParticleSystem* snowParticles;
	UPROPERTY(EditAnywhere, Category = "Weather") UParticleSystem* blizzardParticles;
	UPROPERTY(EditAnywhere, Category = "Weather") UParticleSystem* rainParticles;
	UPROPERTY(EditAnywhere, Category = "Weather") UMaterial* M_RainWetness;

	UPROPERTY(EditAnywhere) TArray<UParticleSystem*> ParticlesByEnum;
	UPROPERTY(EditAnywhere) TArray<UNiagaraSystem*> NiagaraByEnum;

	

	/*
	 * Sounds
	 */

	//UPROPERTY(EditAnywhere) class USoundCue* Cue_BuildingFocusMechanical;
	//UPROPERTY(EditAnywhere) class USoundCue* Cue_BuildingFocusNonMechanical;


	/*
	 * Temporary
	 */
	//UPROPERTY(EditAnywhere) UTexture2D* RiverFraction; // 4x4 map
	//UPROPERTY(EditAnywhere) UTexture2D* RiverFraction;

	UPROPERTY() UTexture2D* heightTexture;
	UPROPERTY() UTexture2D* biomeTexture;
	//UPROPERTY() UTexture2D* riverTexture;
	UPROPERTY() UTexture2D* provinceTexture;

	UPROPERTY() UMaterialInstanceDynamic* M_WorldMap;
	
private:
	template<typename T>
	T* Load(const char* path)
	{
		ConstructorHelpers::FObjectFinder<T> objectFinder(*FString(path));
		check(objectFinder.Succeeded());
		return objectFinder.Object;
	}

	template<typename T>
	T* LoadF(FString path)
	{
		ConstructorHelpers::FObjectFinder<T> objectFinder(*path);
		check(objectFinder.Succeeded());
		return objectFinder.Object;
	}

	void LoadUnit(UnitEnum unitEnum, std::string meshFile);
	void LoadUnitFull(UnitEnum unitEnum, std::string folderPath, std::string skelFileName, std::unordered_map<UnitAnimationEnum, std::string> animationFileNames, std::string staticFileName);
	//void LoadUnitAnimation(UnitEnum unitEnum, int32 variationIndex, UnitAnimationEnum unitAnimation, std::string file);
	void LoadUnitWeapon(UnitAnimationEnum unitAnimation, std::string file);
	void LoadUnitAuxMesh(UnitAnimationEnum unitAnimation, std::string file);
	
	void LoadResource(ResourceEnum resourceEnum, std::string meshFile);
	void LoadResource2(ResourceEnum resourceEnum, std::string meshFilePrefix);

	void LoadModule(FString moduleName , FString meshFile, bool paintConstructionVertexColor = false, bool isTogglable = false);
	void LoadTogglableModule(FString moduleName, FString meshFile);
	void LoadAnimModule(FString moduleName, FString meshFile);


	void LoadBuilding(CardEnum buildingEnum, FString moduleGroupName, FString moduleGroupFolderName, bool useOldMethod = false, ModuleTransformGroup auxGroup = ModuleTransformGroup(), int32 minEra = 1) {
		TryLoadBuildingModuleSet(moduleGroupName, moduleGroupFolderName, useOldMethod, buildingEnum);

		LinkBuilding(buildingEnum, moduleGroupName, auxGroup, minEra);
	}
	void LinkBuilding(CardEnum buildingEnum, FString moduleGroupName, ModuleTransformGroup auxGroup = ModuleTransformGroup(), int32 minEra = 1) {
		CppUtils::AppendVec(auxGroup.particleInfos, _tempAuxGroup.particleInfos);
		CppUtils::AppendVec(auxGroup.animTransforms, _tempAuxGroup.animTransforms);
		CppUtils::AppendVec(auxGroup.togglableTransforms, _tempAuxGroup.togglableTransforms);
		_tempAuxGroup = ModuleTransformGroup();

		_buildingEnumToModuleGroups[static_cast<int>(buildingEnum)].Add(
			ModuleTransformGroup::CreateSet(moduleGroupName, auxGroup)
		);
		_buildingEnumToMinEraModel[static_cast<int>(buildingEnum)] = minEra;
	}
	void LoadBuilding(CardEnum buildingEnum, FString moduleGroupPrefix, FString moduleGroupFolderPrefix, int32 minEra, int32 maxEra = 4, ModuleTransformGroup auxGroup = ModuleTransformGroup())
	{
		for (int32 i = minEra; i <= maxEra; i++) {
			FString moduleGroupName = moduleGroupPrefix + FString::FromInt(i);
			FString moduleGroupFolderName = moduleGroupFolderPrefix + FString::FromInt(i);
			LoadBuilding(buildingEnum, moduleGroupPrefix + FString::FromInt(i), moduleGroupFolderPrefix + FString("/Era") + FString::FromInt(i), 
				false, auxGroup, minEra);
		}
		check(_buildingEnumToModuleGroups[static_cast<int>(buildingEnum)].Num() == (maxEra - minEra + 1));
	}
	void LinkBuildingEras(CardEnum buildingEnum, FString moduleGroupPrefix, int32 minEra, int32 maxEra = 4, ModuleTransformGroup auxGroup = ModuleTransformGroup())
	{
		for (int32 i = minEra; i <= maxEra; i++) {
			FString moduleGroupName = moduleGroupPrefix + FString::FromInt(i);
			LinkBuilding(buildingEnum, moduleGroupPrefix + FString::FromInt(i), auxGroup, minEra);
		}
		check(_buildingEnumToModuleGroups[static_cast<int>(buildingEnum)].Num() == (maxEra - minEra + 1));
	}
	
	void TryLoadBuildingModuleSet(FString moduleSetName, FString meshSetFolder, bool useOldMethod = true, CardEnum buildingEnum = CardEnum::None);

	

	void LoadGeoresource(GeoresourceEnum georesourceEnum, std::string folder, std::string meshFileNamePrefix, int32 numberOfMeshes);
	void LoadTree(TileObjEnum treeEnum, std::string trunkMeshFile, std::string leafMeshFile,
									std::string fruitMeshFile, std::string leafLowMeshFile, std::string leafShadowMeshFile, std::string stumpMeshFile);
	void LoadTileObject(TileObjEnum treeEnum, std::vector<std::string> meshFiles);


	void AddBuildingModule(FString moduleName, UStaticMesh* mesh, TArray<FString>& nameListToAdd)
	{
		check(!_moduleNameToMesh.Contains(moduleName));
		_moduleNameToMesh.Add(moduleName, mesh);
		nameListToAdd.Add(moduleName);

		CheckMeshesAvailable();
	}
	

	/*
	 * Mesh Processing
	 */
	void DetectMeshGroups(UStaticMesh* mesh, TArray<FVector>& vertexPositions);
	
	void DetectParticleSystemPosition(CardEnum buildingEnum, UStaticMesh* mesh);
	
	void PaintMeshForConstruction(FString moduleName);

private:
	UPROPERTY() TMap<FString, UStaticMesh*> _moduleNameToMesh;
	//UPROPERTY() TMap<FString, UStaticMesh*> _moduleNameToConstructionMesh;

	TArray<TArray<ModuleTransformGroup>> _buildingEnumToModuleGroups;
	TArray<int32> _buildingEnumToMinEraModel;
	ModuleTransformGroup _tempAuxGroup; // Temp variable for particleSystem detection
	
	UPROPERTY() TArray<FString> _moduleNames;
	UPROPERTY() TArray<FString> _animModuleNames;
	UPROPERTY() TArray<FString> _togglableModuleNames;
	
	//std::unordered_map<UnitEnum, std::vector<UStaticMesh*>> _unitToMeshes;

	std::vector<std::vector<FUnitAsset>> _unitEnumToAsset;
	//std::unordered_map<UnitEnum, std::vector<USkeletalMesh*>> _unitToSkeletalMesh;
	//std::unordered_map<UnitEnum, std::vector<std::unordered_map<UnitAnimationEnum, UAnimSequence*>>> _animationEnumToSequence;
	
	std::unordered_map<UnitAnimationEnum, UStaticMesh*> _animationEnumToWeaponMesh;

	std::unordered_map<UnitAnimationEnum, UStaticMesh*> _animationEnumToAuxMesh;

	
	std::unordered_map<ResourceEnum, UStaticMesh*> _resourceToMesh;
	std::unordered_map<ResourceEnum, UStaticMesh*> _resourceToHandMesh;

	std::unordered_map<GeoresourceEnum, FGeoresourceMeshAssets> _georesourceToMesh;

	UPROPERTY() UMaterial* _terrainMaterial = nullptr;
	UPROPERTY() UMaterial* _terrainMaterial2 = nullptr;

	TMap<int32, FTileMeshAssets> _tileMeshes;

	std::vector<UStaticMesh*> _plantToMesh;

	UPROPERTY() TArray<UTexture2D*> _buildingIcons;
	UPROPERTY() TArray<UTexture2D*> _buildingIconsAlpha;
	UPROPERTY() TArray<UTexture2D*> _resourceIcons;
	UPROPERTY() TArray<UTexture2D*> _resourceIconsAlpha;
	UPROPERTY() TArray<UMaterialInstanceDynamic*> _resourceIconMaterials;

	TSet<CardEnum> _buildingsUsingSpecialIcon;

	UPROPERTY() TMap<int32, UTexture2D*> _cardIcons;

	//! ArmyIcon
	UPROPERTY() TArray<UTexture2D*> _armyIcons;

private:
	UPROPERTY() TArray<FString> _modulesNeedingPaintConstruction;
};
