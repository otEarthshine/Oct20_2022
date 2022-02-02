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
#include "MainMenuAssetLoaderComponent.h"
#include "SpineAtlasAsset.h"
#include "SpineSkeletonDataAsset.h"

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


enum class ModuleTypeEnum : uint8
{
	Normal,
	ConstructionOnly,
	ConstructionBaseHighlight,
	Frame,
	FrameConstructionOnly,
	Window,
	ShaderAnimate,
	ShaderOnOff,
	AlwaysOn,

	RotateRoll,
	RotateRollMine,
	RotateRollMine2,
	RotateRollQuarry,
	RotateRollFurniture,
	RotateZAxis,
	RotateNewX,
	RotateNewY,
	RotateNewZ,
};

static bool IsModuleTypeRotate(ModuleTypeEnum moduleTypeEnum)
{
	int32 moduleTypeInt = static_cast<int32>(moduleTypeEnum);
	return static_cast<int32>(ModuleTypeEnum::RotateRoll) <= moduleTypeInt && moduleTypeInt <= static_cast<int32>(ModuleTypeEnum::RotateNewZ);
}
static bool IsModuleTypeRotateNew(ModuleTypeEnum moduleTypeEnum)
{
	int32 moduleTypeInt = static_cast<int32>(moduleTypeEnum);
	return static_cast<int32>(ModuleTypeEnum::RotateNewX) <= moduleTypeInt && moduleTypeInt <= static_cast<int32>(ModuleTypeEnum::RotateNewZ);
}

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
	FName moduleName;

	float relativeConstructionTime = 0;
	ModuleTypeEnum moduleTypeEnum = ModuleTypeEnum::Normal;
	FVector moduleTypeSpecialState = FVector::ZeroVector;

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

	ModuleTransform(FName moduleName,
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

const TArray<FString> TileSubmeshNames
{
	"Trunk",
	"Leaf",
	"Fruit",
	"LeafLowPoly",
	"LeafShadow",
	"Stump",
};
static const int32 TileSubmeshCount = TileSubmeshNames.Num();


enum class DefenseOverlayEnum : uint8
{
	Node,
	CityNode,
	FortNode,
	Line,

	Count,
};

const int32 DefenseOverlayEnumSize = static_cast<int32>(DefenseOverlayEnum::Count);

enum class DefenseColorEnum
{
	Empty,
	EnemyProtected,
	EnemyUnprotected,
	Protected,
	Unprotected,

	River,

	Count,
};

const int32 DefenseColorEnumSize = static_cast<int32>(DefenseColorEnum::Count);

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
struct FSpineAsset
{
	GENERATED_BODY();

	UPROPERTY() USpineAtlasAsset* atlas;
	UPROPERTY() USpineSkeletonDataAsset* skeletonData;

	UPROPERTY() USpineAtlasAsset* atlas_fx;
	UPROPERTY() USpineSkeletonDataAsset* skeletonData_fx;
};

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

	void SaveSmokeJson();

private:
	void TraverseTris(uint32 groupIndex_PositionMerge, int32 groupIndex_Parent, const TArray<uint32>& indexBuffer, const TArray<FVector>& vertexPositions);

	void TraverseTris_July10(uint32 groupIndex_PositionMerge, int32 groupIndex_Parent, const TArray<int32>& indexBuffer, const TArray<FVector>& vertexPositions);

public:
	UStaticMesh* moduleMesh(FName moduleName);

	//FBuildingAsset buildingAsset(FString buildingAssetName) {
	//	if (_buildingNameToAsset.Contains(buildingAssetName)) {
	//		return _buildingNameToAsset[buildingAssetName];
	//	}
	//	return FBuildingAsset();
	//}

	//bool HasConstructionMesh(FString moduleName) { return false; } // _moduleNameToConstructionMesh.Contains(moduleName);

	//! Module types are used to create separate mesh sets
	const TArray<FName>& moduleNames()
	{
		//PUN_CHECK(_moduleNames.Num() == ModuleMeshCount);
		return _moduleNames;
	}
	TArray<FName>& animModuleNames() { return _animModuleNames; }
	TArray<FName>& togglableModuleNames() { return _togglableModuleNames; }

	TArray<TArray<ModuleTransformGroup>> buildingEnumToVariationToModuleTransforms(FactionEnum factionEnum) { return _factionEnumToBuildingEnumToModuleGroups[static_cast<int32>(factionEnum)]; }
	int32 GetMinEraDisplay(FactionEnum factionEnum, CardEnum buildingEnum) { return _factionEnumToBuildingEnumToMinEraModel[static_cast<int32>(factionEnum)][static_cast<int>(buildingEnum)]; }
	

	int32 unitMeshCount(UnitEnum unitEnum) {
		return _unitEnumToAsset[static_cast<int>(unitEnum)].size();
	}

	FUnitAsset unitAsset(UnitEnum unitEnum, int32 variationIndex = 0) {
		return _unitEnumToAsset[static_cast<int>(unitEnum)][variationIndex];
	}

	int32 maxUnitMeshVariationCount() {
		return _maxUnitVariationCount;
	}
	
	
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

	//UTexture2D* GetBuildingIcon(CardEnum buildingEnum) { return _buildingIcons[static_cast<int>(buildingEnum)]; }
	//UTexture2D* GetBuildingIconAlpha(CardEnum buildingEnum) { return _buildingIconsAlpha[static_cast<int>(buildingEnum)]; }
	//bool IsBuildingUsingSpecialIcon(CardEnum buildingEnum) {
	//	return _buildingsUsingSpecialIcon.Contains(buildingEnum);
	//}

	//UTexture2D* GetBuildingIconNullable(CardEnum buildingEnum) {
	//	if (static_cast<int32>(buildingEnum) >= _buildingIcons.Num()) {
	//		return nullptr;
	//	}
	//	return _buildingIcons[static_cast<int>(buildingEnum)];
	//}
	//
	
	UTexture2D* GetResourceIcon(ResourceEnum resourceEnum) { return _resourceIcons[static_cast<int>(resourceEnum)]; }
	UTexture2D* GetResourceIconAlpha(ResourceEnum resourceEnum) { return _resourceIconsAlpha[static_cast<int>(resourceEnum)]; }
	UMaterialInstanceDynamic* GetResourceIconMaterial(ResourceEnum resourceEnum);

	UMaterialInstanceDynamic* GetResourceIconMaterial_WithMoneyOrInfluence(ResourceEnum resourceEnum)
	{
		if (_coinIconMaterial == nullptr) {
			UMaterialInstanceDynamic* materialInstance = UMaterialInstanceDynamic::Create(M_Icon, this);
			materialInstance->SetTextureParameterValue("ColorTexture", CoinIcon);
			_coinIconMaterial = materialInstance;
		}
		if (_influenceIconMaterial == nullptr) {
			UMaterialInstanceDynamic* materialInstance = UMaterialInstanceDynamic::Create(M_Icon, this);
			materialInstance->SetTextureParameterValue("ColorTexture", InfluenceIcon);
			_influenceIconMaterial = materialInstance;
		}
		if (resourceEnum == ResourceEnum::Money) {
			return _coinIconMaterial;
		}
		if (resourceEnum == ResourceEnum::Influence) {
			return _influenceIconMaterial;
		}
		return GetResourceIconMaterial(resourceEnum);
	}

	UTexture2D* GetGeoresourceIcon(ResourceEnum resourceEnum) {
		if (_georesourceIcons.find(resourceEnum) == _georesourceIcons.end()) {
			return nullptr;
		}
		return _georesourceIcons[resourceEnum];
	}

	UTexture2D* GetBiomeIcon(BiomeEnum biomeEnum) {
		if (_biomeIcons.find(biomeEnum) == _biomeIcons.end()) {
			return nullptr;
		}
		return _biomeIcons[biomeEnum];
	}

	UTexture2D* GetCardIcon(FactionEnum factionEnum, CardEnum cardEnum) {
		UTexture2D* cardIcon = GetCardIconNullable(factionEnum, cardEnum);
		return cardIcon ? cardIcon : _cardIcons[static_cast<int32>(CardEnum::None)];
	}
	UTexture2D* GetCardIconNullable(FactionEnum factionEnum, CardEnum cardEnum) {
		return _cardIcons[static_cast<int32>(factionEnum) * CardEnumCount_WithNone + static_cast<int32>(cardEnum)];
	}

	bool HasFadeBorder(FactionEnum factionEnum, CardEnum cardEnum) {
		return _cardIconHasFadeBorder[static_cast<int32>(factionEnum) * CardEnumCount_WithNone + static_cast<int32>(cardEnum)];
	}
	

	void SetCardIcon(int32 factionEnumInt, CardEnum cardEnum, UTexture2D* texture, bool hasFadeBorder) {
		int32 index = factionEnumInt * CardEnumCount_WithNone + static_cast<int32>(cardEnum);
		_cardIcons[index] = texture;
		_cardIconHasFadeBorder[index] = hasFadeBorder;
	}


	FSpineAsset GetSpine(CardEnum cardEnum);

	UTexture2D* GetFactionImage(const FString& name) const {
		return _mainMenuAssetLoader->GetFactionImage(name);
	}
	const TArray<UTexture2D*>& GetPlayerLogos() const {
		return _mainMenuAssetLoader->PlayerLogos;
	}
	UTexture2D* GetPlayerLogo(int32 logoIndex) const {
		return _mainMenuAssetLoader->PlayerLogos[logoIndex];
	}
	const TMap<FString, UTexture2D*>& GetPlayerCharacters() const {
		return _mainMenuAssetLoader->PlayerCharacters;
	}
	

	//UTexture2D* GetArmyIcon(ArmyEnum armyEnum) { return _armyIcons[static_cast<int>(armyEnum)]; }

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
	void CheckMeshesAvailable()
	{
		// Building Mesh Check
		// !!! If check broke here, there might be duplicate load???
		
		//check(_moduleNames.Num() + _togglableModuleNames.Num() + _animModuleNames.Num() == _moduleNameToMesh.Num()); // Doesn't work with old methods
		//for (int32 i = 0; i < _moduleNames.Num(); i++) {
		//	check(_moduleNameToMesh.Contains(_moduleNames[i]))
		//}
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
					if (_moduleNames[i] == FName("ClayPit_Era1Special4")) {
						FName newName = FName("ClayPit_Era1Special1");
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



	std::unordered_map<std::string, std::unordered_map<int32, std::vector<int32>>> meshName_to_groupIndexToConnectedVertIndices;
	TMap<FString, TArray<FVector>> meshName_to_vertexPositions;

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

	UPROPERTY(EditAnywhere) UStaticMesh* DefenseOverlay_Line;
	UPROPERTY(EditAnywhere) UStaticMesh* DefenseOverlay_Node;
	UPROPERTY(EditAnywhere) UStaticMesh* DefenseOverlay_CityNode;
	UPROPERTY(EditAnywhere) UStaticMesh* DefenseOverlay_FortNode;

	UPROPERTY(EditAnywhere) UStaticMesh* DefenseOverlay_Line_Map;
	UPROPERTY(EditAnywhere) UStaticMesh* DefenseOverlay_Node_Map;
	UPROPERTY(EditAnywhere) UStaticMesh* DefenseOverlay_CityNode_Map;
	UPROPERTY(EditAnywhere) UStaticMesh* DefenseOverlay_FortNode_Map;

	UPROPERTY(EditAnywhere) TArray<UMaterialInstance*> DefenseMaterialInstances;

	UStaticMesh* GetDefenseOverlayMesh(DefenseOverlayEnum defenseOverlayEnum, bool isMap)
	{
		switch (defenseOverlayEnum) {
		case DefenseOverlayEnum::Node: return isMap ? DefenseOverlay_Node_Map : DefenseOverlay_Node;
		case DefenseOverlayEnum::CityNode: return isMap ? DefenseOverlay_CityNode_Map : DefenseOverlay_CityNode;
		case DefenseOverlayEnum::FortNode: return isMap ? DefenseOverlay_FortNode_Map : DefenseOverlay_FortNode;
		case DefenseOverlayEnum::Line: return isMap ? DefenseOverlay_Line_Map : DefenseOverlay_Line;
		default:
			return nullptr;
		}
	}

	UMaterialInstance* GetDefenseOverlayMaterial(DefenseColorEnum defenseColorEnum) {
		return DefenseMaterialInstances[static_cast<int32>(defenseColorEnum)];
	}


	

	/*
	 * UI
	 */
	
	//UPROPERTY(EditAnywhere) UMaterial* BuildingIconMaterial;

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
	UPROPERTY(EditAnywhere) UMaterial* M_Icon;

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

	UPROPERTY(EditAnywhere) UMaterial* M_LogoHover;

	enum class HoverWarningEnum : uint8 {
		Housing,
		Starving,
		Freezing,
		Sick,
		Tools,
		UnhappyOrange,
		UnhappyRed,

		Logo,

		Count,
	};
	UPROPERTY(EditAnywhere) TArray<UMaterialInstanceDynamic*> MI_HoverWarnings;
	UMaterialInstanceDynamic* GetHoverWarningMaterial(HoverWarningEnum warningEnum)
	{
		if (MI_HoverWarnings.Num() == 0) 
		{
			MI_HoverWarnings.SetNum(static_cast<int>(HoverWarningEnum::Count));
			
			auto addHoverWarningIconInst = [&](HoverWarningEnum warningEnum, UMaterial* material, UTexture2D* image) {
				MI_HoverWarnings[static_cast<int>(warningEnum)] = UMaterialInstanceDynamic::Create(material, this);
				if (image) {
					MI_HoverWarnings[static_cast<int>(warningEnum)]->SetTextureParameterValue("IconImage", image);;
				}
			};
			addHoverWarningIconInst(HoverWarningEnum::Housing, M_HoverWarning, WarningHouse);
			addHoverWarningIconInst(HoverWarningEnum::Starving, M_HoverWarning, WarningStarving);
			addHoverWarningIconInst(HoverWarningEnum::Freezing, M_HoverWarning, WarningSnow);
			addHoverWarningIconInst(HoverWarningEnum::Sick, M_HoverWarning, WarningHealthcare);
			addHoverWarningIconInst(HoverWarningEnum::Tools, M_HoverWarning, WarningTools);
			addHoverWarningIconInst(HoverWarningEnum::UnhappyOrange, M_HoverWarningHappiness, HappinessOrangeIcon);
			addHoverWarningIconInst(HoverWarningEnum::UnhappyRed, M_HoverWarningHappiness, HappinessRedIcon);
		}
		return MI_HoverWarnings[static_cast<int32>(warningEnum)];
	}
	

	//! HoverIcon
	UPROPERTY(EditAnywhere) UTexture2D* UnhappyHoverIcon;

	UPROPERTY(EditAnywhere) UTexture2D* BlackIcon;
	UPROPERTY(EditAnywhere) UTexture2D* WhiteIcon;

	UPROPERTY(EditAnywhere) UMaterial* M_GeoresourceIcon;
	UPROPERTY(EditAnywhere) TMap<int32, UTexture*> _geo_Icon;
	UPROPERTY(EditAnywhere) TMap<int32, UTexture*> _geo_IconAlpha;

	//!
	UPROPERTY(EditAnywhere) UNiagaraSystem* NS_OnDemolish;
	UPROPERTY(EditAnywhere) UNiagaraSystem* NS_OnPlacement;
	UPROPERTY(EditAnywhere) UNiagaraSystem* NS_OnTownhall;
	UPROPERTY(EditAnywhere) UNiagaraSystem* NS_OnUpgrade;
	UPROPERTY(EditAnywhere) UNiagaraSystem* NS_OnReveal;
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
	UPROPERTY(EditAnywhere, Category = "Material Import") UMaterial* M_GridGuideLine;
	
	UPROPERTY(EditAnywhere, Category = "Material Import") UMaterial* RadiusMaterial;
	
	UPROPERTY(EditAnywhere) UStaticMesh* RadiusMesh;
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
	UPROPERTY(EditAnywhere, Category = "Material Import") UMaterial* M_TerritoryBattleHighlight;

	
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
	 * Spine
	 */
	UPROPERTY(EditAnywhere) USpineAtlasAsset* Archer_SpineAtlas;
	UPROPERTY(EditAnywhere) USpineSkeletonDataAsset* Archer_SpineData;
	UPROPERTY(EditAnywhere) USpineAtlasAsset* Artillery_SpineAtlas;
	UPROPERTY(EditAnywhere) USpineSkeletonDataAsset* Artillery_SpineData;
	UPROPERTY(EditAnywhere) USpineAtlasAsset* Battleship_SpineAtlas;
	UPROPERTY(EditAnywhere) USpineSkeletonDataAsset* Battleship_SpineData;
	UPROPERTY(EditAnywhere) USpineAtlasAsset* Cannon_SpineAtlas;
	UPROPERTY(EditAnywhere) USpineSkeletonDataAsset* Cannon_SpineData;
	UPROPERTY(EditAnywhere) USpineAtlasAsset* Catapult_SpineAtlas;
	UPROPERTY(EditAnywhere) USpineSkeletonDataAsset* Catapult_SpineData;
	UPROPERTY(EditAnywhere) USpineAtlasAsset* Conscript_SpineAtlas;
	UPROPERTY(EditAnywhere) USpineSkeletonDataAsset* Conscript_SpineData;
	UPROPERTY(EditAnywhere) USpineAtlasAsset* Frigate_SpineAtlas;
	UPROPERTY(EditAnywhere) USpineSkeletonDataAsset* Frigate_SpineData;
	UPROPERTY(EditAnywhere) USpineAtlasAsset* Galley_SpineAtlas;
	UPROPERTY(EditAnywhere) USpineSkeletonDataAsset* Galley_SpineData;
	UPROPERTY(EditAnywhere) USpineAtlasAsset* Infantry_SpineAtlas;
	UPROPERTY(EditAnywhere) USpineSkeletonDataAsset* Infantry_SpineData;
	UPROPERTY(EditAnywhere) USpineAtlasAsset* Knight_SpineAtlas;
	UPROPERTY(EditAnywhere) USpineSkeletonDataAsset* Knight_SpineData;
	UPROPERTY(EditAnywhere) USpineAtlasAsset* MachineGun_SpineAtlas;
	UPROPERTY(EditAnywhere) USpineSkeletonDataAsset* MachineGun_SpineData;
	UPROPERTY(EditAnywhere) USpineAtlasAsset* Militia_SpineAtlas;
	UPROPERTY(EditAnywhere) USpineSkeletonDataAsset* Militia_SpineData;
	UPROPERTY(EditAnywhere) USpineAtlasAsset* Musketeer_SpineAtlas;
	UPROPERTY(EditAnywhere) USpineSkeletonDataAsset* Musketeer_SpineData;
	UPROPERTY(EditAnywhere) USpineAtlasAsset* NationalGuard_SpineAtlas;
	UPROPERTY(EditAnywhere) USpineSkeletonDataAsset* NationalGuard_SpineData;
	UPROPERTY(EditAnywhere) USpineAtlasAsset* Swordsman_SpineAtlas;
	UPROPERTY(EditAnywhere) USpineSkeletonDataAsset* Swordsman_SpineData;
	UPROPERTY(EditAnywhere) USpineAtlasAsset* Tank_SpineAtlas;
	UPROPERTY(EditAnywhere) USpineSkeletonDataAsset* Tank_SpineData;
	UPROPERTY(EditAnywhere) USpineAtlasAsset* Warrior_SpineAtlas;
	UPROPERTY(EditAnywhere) USpineSkeletonDataAsset* Warrior_SpineData;

	UPROPERTY(EditAnywhere) USpineAtlasAsset* WoodWall_SpineAtlas;
	UPROPERTY(EditAnywhere) USpineSkeletonDataAsset* WoodWall_SpineData;
	UPROPERTY(EditAnywhere) USpineAtlasAsset* StoneWall_SpineAtlas;
	UPROPERTY(EditAnywhere) USpineSkeletonDataAsset* StoneWall_SpineData;
	UPROPERTY(EditAnywhere) USpineAtlasAsset* RaidTreasure_SpineAtlas;
	UPROPERTY(EditAnywhere) USpineSkeletonDataAsset* RaidTreasure_SpineData;
	UPROPERTY(EditAnywhere) USpineAtlasAsset* ProvinceOwnershipFlag_SpineAtlas;
	UPROPERTY(EditAnywhere) USpineSkeletonDataAsset* ProvinceOwnershipFlag_SpineData;

	UPROPERTY(EditAnywhere) USpineAtlasAsset* BattleOpening_SpineAtlas;
	UPROPERTY(EditAnywhere) USpineSkeletonDataAsset* BattleOpening_SpineData;

	UPROPERTY(EditAnywhere) USpineAtlasAsset* Arrow_SpineAtlasFX;
	UPROPERTY(EditAnywhere) USpineSkeletonDataAsset* Arrow_SpineDataFX;
	UPROPERTY(EditAnywhere) USpineAtlasAsset* Cannon_SpineAtlasFX;
	UPROPERTY(EditAnywhere) USpineSkeletonDataAsset* Cannon_SpineDataFX;
	UPROPERTY(EditAnywhere) USpineAtlasAsset* Cannonlv2_SpineAtlasFX;
	UPROPERTY(EditAnywhere) USpineSkeletonDataAsset* Cannonlv2_SpineDataFX;
	UPROPERTY(EditAnywhere) USpineAtlasAsset* CurveSlash_SpineAtlasFX;
	UPROPERTY(EditAnywhere) USpineSkeletonDataAsset* CurveSlash_SpineDataFX;
	UPROPERTY(EditAnywhere) USpineAtlasAsset* DoubleHeavySlash_SpineAtlasFX;
	UPROPERTY(EditAnywhere) USpineSkeletonDataAsset* DoubleHeavySlash_SpineDataFX;

	UPROPERTY(EditAnywhere) USpineAtlasAsset* FireArrow_SpineAtlasFX;
	UPROPERTY(EditAnywhere) USpineSkeletonDataAsset* FireArrow_SpineDataFX;
	UPROPERTY(EditAnywhere) USpineAtlasAsset* HeavyCurveSlash_SpineAtlasFX;
	UPROPERTY(EditAnywhere) USpineSkeletonDataAsset* HeavyCurveSlash_SpineDataFX;
	UPROPERTY(EditAnywhere) USpineAtlasAsset* LongGun_SpineAtlasFX;
	UPROPERTY(EditAnywhere) USpineSkeletonDataAsset* LongGun_SpineDataFX;
	UPROPERTY(EditAnywhere) USpineAtlasAsset* Rock_SpineAtlasFX;
	UPROPERTY(EditAnywhere) USpineSkeletonDataAsset* Rock_SpineDataFX;
	UPROPERTY(EditAnywhere) USpineAtlasAsset* Shoot_SpineAtlasFX;
	UPROPERTY(EditAnywhere) USpineSkeletonDataAsset* Shoot_SpineDataFX;
	UPROPERTY(EditAnywhere) USpineAtlasAsset* Slash_SpineAtlasFX;
	UPROPERTY(EditAnywhere) USpineSkeletonDataAsset* Slash_SpineDataFX;
	UPROPERTY(EditAnywhere) USpineAtlasAsset* Smash_SpineAtlasFX;
	UPROPERTY(EditAnywhere) USpineSkeletonDataAsset* Smash_SpineDataFX;
	UPROPERTY(EditAnywhere) USpineAtlasAsset* Stab_SpineAtlasFX;
	UPROPERTY(EditAnywhere) USpineSkeletonDataAsset* Stab_SpineDataFX;

	
	
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
		PUN_CHECK_DEBUG(objectFinder.Succeeded());
		return objectFinder.Object;
	}

	template<typename T>
	T* LoadF(FString path)
	{
		ConstructorHelpers::FObjectFinder<T> objectFinder(*path);
		PUN_CHECK_DEBUG(objectFinder.Succeeded());
		return objectFinder.Object;
	}

	void LoadUnit(UnitEnum unitEnum, std::string meshFile, int32 variationIndex = 0);
	void LoadUnitFull(UnitEnum unitEnum, std::string folderPath, std::string skelFileName, std::unordered_map<UnitAnimationEnum, std::string> animationFileNames, std::string staticFileName, int32 variationIndex = 0);
	
	void LoadUnitWeapon(UnitAnimationEnum unitAnimation, std::string file);
	void LoadUnitAuxMesh(UnitAnimationEnum unitAnimation, std::string file);
	
	void LoadResource(ResourceEnum resourceEnum, std::string meshFile);
	void LoadResource2(ResourceEnum resourceEnum, std::string meshFilePrefix);

	void LoadModule(FName moduleName , FString meshFile, bool paintConstructionVertexColor = false, bool isTogglable = false);
	void LoadTogglableModule(FName moduleName, FString meshFile);
	void LoadAnimModule(FName moduleName, FString meshFile);

	//! Auxiliary requiring full folder name (doesn't automatically add /Era)
	void LoadBuilding(FactionEnum factionEnum, CardEnum buildingEnum, FString moduleGroupName, FString moduleGroupFolderName, ModuleTransformGroup auxGroup = ModuleTransformGroup(), int32 minEra = 1, int32 era = 1)
	{
		TryLoadBuildingModuleSet(factionEnum, moduleGroupName, moduleGroupFolderName, buildingEnum, era);
		LinkBuilding(factionEnum, buildingEnum, moduleGroupName, auxGroup, minEra);
	}
	
	void LinkBuilding(FactionEnum factionEnum, CardEnum buildingEnum, FString moduleGroupName, ModuleTransformGroup auxGroup = ModuleTransformGroup(), int32 minEra = 1) {
		LinkBuilding(factionEnum, factionEnum, buildingEnum, moduleGroupName, auxGroup, minEra);
	}

	// TODO: Link by just using _factionEnumToBuildingEnumToModuleGroups instead
	void LinkBuilding(FactionEnum factionEnum, FactionEnum modelFactionEnum, CardEnum buildingEnum, FString moduleGroupName, ModuleTransformGroup auxGroup = ModuleTransformGroup(), int32 minEra = 1)
	{
		CppUtils::AppendVec(auxGroup.transforms, _tempAuxGroup.transforms);
		CppUtils::AppendVec(auxGroup.miniModules, _tempAuxGroup.miniModules);
		CppUtils::AppendVec(auxGroup.particleInfos, _tempAuxGroup.particleInfos);
		CppUtils::AppendVec(auxGroup.animTransforms, _tempAuxGroup.animTransforms);
		CppUtils::AppendVec(auxGroup.togglableTransforms, _tempAuxGroup.togglableTransforms);
		_tempAuxGroup = ModuleTransformGroup();
		_lastTempAuxGroup = auxGroup;

		_factionEnumToBuildingEnumToModuleGroups[static_cast<int32>(factionEnum)][static_cast<int>(buildingEnum)].Add(
			ModuleTransformGroup::CreateSet(WithFactionNameInternal(modelFactionEnum, moduleGroupName), auxGroup)
		);
		_factionEnumToBuildingEnumToMinEraModel[static_cast<int32>(factionEnum)][static_cast<int>(buildingEnum)] = minEra;
	}

	/*
	 * Main LoadBuilding
	 * - All Factions, All Eras
	 */
	// TODO: moduleGroupFolderPrefix should be usable as moduleGroupPrefix
	void LoadBuilding(CardEnum buildingEnum, 
		FString moduleGroupPrefix_europe, FString moduleGroupPrefix_arab, FString moduleGroupFolderPrefix,
		int32 minEra, int32 maxEra = 4, ModuleTransformGroup auxGroup = ModuleTransformGroup())
	{
		LoadBuildingEras(FactionEnum::Europe, buildingEnum, moduleGroupPrefix_europe, moduleGroupFolderPrefix, minEra, maxEra, auxGroup);
		LoadBuildingEras(FactionEnum::Arab, buildingEnum, moduleGroupPrefix_arab, moduleGroupFolderPrefix, minEra, maxEra, auxGroup);
	}
	void LoadBuilding(CardEnum buildingEnum, FString moduleGroupFolderPrefix,
					int32 minEra, int32 maxEra = 4, ModuleTransformGroup auxGroup = ModuleTransformGroup())
	{
		LoadBuildingEras(FactionEnum::Europe, buildingEnum, moduleGroupFolderPrefix, moduleGroupFolderPrefix, minEra, maxEra, auxGroup);
		LoadBuildingEras(FactionEnum::Arab, buildingEnum, moduleGroupFolderPrefix, moduleGroupFolderPrefix, minEra, maxEra, auxGroup);
	}

	void LoadBuildingEras(FactionEnum factionEnum, CardEnum buildingEnum, FString moduleGroupPrefix, FString moduleGroupFolderPrefix, 
							int32 minEra, int32 maxEra = 4, ModuleTransformGroup auxGroup = ModuleTransformGroup())
	{
		if (moduleGroupPrefix == "") {
			return;
		}
		for (int32 i = minEra; i <= maxEra; i++) {
			FString moduleGroupName = moduleGroupPrefix + FString::FromInt(i);
			FString moduleGroupFolderName = moduleGroupFolderPrefix + FString("/Era") + FString::FromInt(i);
			LoadBuilding(factionEnum, buildingEnum, moduleGroupName, moduleGroupFolderName,
				auxGroup, minEra, i
			);
		}
		//check(_factionEnumToBuildingEnumToModuleGroups[static_cast<int32>(factionEnum)][static_cast<int>(buildingEnum)].Num() == (maxEra - minEra + 1));
	};
	
	void LinkBuildingEras(FactionEnum factionEnum, CardEnum buildingEnum, FString moduleGroupPrefix, 
							int32 minEra, int32 maxEra = 4, ModuleTransformGroup auxGroup = ModuleTransformGroup())
	{
		for (int32 i = minEra; i <= maxEra; i++) {
			FString moduleGroupName = moduleGroupPrefix + FString::FromInt(i);
			LinkBuilding(factionEnum, buildingEnum, moduleGroupName, auxGroup, minEra);
		}
		check(_factionEnumToBuildingEnumToModuleGroups[static_cast<int32>(factionEnum)][static_cast<int>(buildingEnum)].Num() == (maxEra - minEra + 1));
	}
	void LinkBuildingEras(FactionEnum factionEnum, FactionEnum modelFactionEnum, CardEnum buildingEnum, FString moduleGroupPrefix,
		int32 minEra, int32 maxEra = 4, ModuleTransformGroup auxGroup = ModuleTransformGroup())
	{
		for (int32 i = minEra; i <= maxEra; i++) {
			FString moduleGroupName = moduleGroupPrefix + FString::FromInt(i);
			LinkBuilding(factionEnum, modelFactionEnum, buildingEnum, moduleGroupName, auxGroup, minEra);
		}
		check(_factionEnumToBuildingEnumToModuleGroups[static_cast<int32>(factionEnum)][static_cast<int>(buildingEnum)].Num() == (maxEra - minEra + 1));
	}



	
	void TryLoadBuildingModuleSet_Old(FactionEnum factionEnum, FString moduleSetName, FString meshSetFolder);

	void TryLoadBuildingModuleSet(FactionEnum factionEnum, FString moduleSetName, FString meshSetFolder, CardEnum buildingEnum = CardEnum::None, int32 era = 1);


	static FString GetBuildingPath(FactionEnum factionEnum)
	{
		FString buildingPath;
		if (factionEnum == FactionEnum::Europe) {
			buildingPath = "Models/Buildings/";
		}
		else {
			buildingPath = "Models/Desertfaction/Models/";
		}
		return buildingPath;
	}
	

	void LoadGeoresource(GeoresourceEnum georesourceEnum, std::string folder, std::string meshFileNamePrefix, int32 numberOfMeshes);
	void LoadTree(TileObjEnum treeEnum, std::string trunkMeshFile, std::string leafMeshFile,
									std::string fruitMeshFile, std::string leafLowMeshFile, std::string leafShadowMeshFile, std::string stumpMeshFile);
	void LoadTileObject(TileObjEnum treeEnum, std::vector<std::string> meshFiles);


	void AddBuildingModule(FName moduleName, UStaticMesh* mesh, TArray<FName>& nameListToAdd)
	{
		check(!_moduleNameToMesh.Contains(moduleName));
		_moduleNameToMesh.Add(moduleName, mesh);
		nameListToAdd.Add(moduleName);
		_recentlyAddedModuleNames.Add(moduleName);

		CheckMeshesAvailable();
	}
	

	/*
	 * Mesh Processing
	 */
	void DetectMeshGroups(UStaticMesh* mesh, TArray<FVector>& vertexPositions);

	void DetectOrLoadMeshVertexInfo(FString meshName, UStaticMesh* mesh);

	FVector DetectRotatorPosition(FString meshName, UStaticMesh* mesh, bool extendsNegY = false);
	
	void DetectParticleSystemPosition(CardEnum buildingEnum, FactionEnum factionEnum, UStaticMesh* mesh);
	
	void PaintMeshForConstruction(FName moduleName);


private:
	UPROPERTY() TMap<FName, UStaticMesh*> _moduleNameToMesh;

	UPROPERTY() TArray<FName> _recentlyAddedModuleNames;

	TArray<TArray<TArray<ModuleTransformGroup>>> _factionEnumToBuildingEnumToModuleGroups; // _buildingEnumToModuleGroups
	TArray<TArray<int32>> _factionEnumToBuildingEnumToMinEraModel;
	ModuleTransformGroup _tempAuxGroup; // Temp variable for particleSystem detection
	ModuleTransformGroup _lastTempAuxGroup; // Temp variable
	
	UPROPERTY() TArray<FName> _moduleNames;
	UPROPERTY() TArray<FName> _animModuleNames;
	UPROPERTY() TArray<FName> _togglableModuleNames;
	
	//std::unordered_map<UnitEnum, std::vector<UStaticMesh*>> _unitToMeshes;

	std::vector<std::vector<FUnitAsset>> _unitEnumToAsset;
	int32 _maxUnitVariationCount = 0;
	//std::vector<FUnitAsset> _unitDisplayEnumToAsset;
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

	//UPROPERTY() TArray<UTexture2D*> _buildingIcons;
	//UPROPERTY() TArray<UTexture2D*> _buildingIconsAlpha;
	UPROPERTY() TArray<UTexture2D*> _resourceIcons;
	UPROPERTY() TArray<UTexture2D*> _resourceIconsAlpha;
	
	UPROPERTY() TArray<UMaterialInstanceDynamic*> _resourceIconMaterials;
	UPROPERTY() UMaterialInstanceDynamic* _coinIconMaterial;
	UPROPERTY() UMaterialInstanceDynamic* _influenceIconMaterial;

	std::unordered_map<ResourceEnum, UTexture2D*> _georesourceIcons;
	std::unordered_map<BiomeEnum, UTexture2D*> _biomeIcons;

	//TSet<CardEnum> _buildingsUsingSpecialIcon;

	UPROPERTY() TArray<UTexture2D*> _cardIcons;
	UPROPERTY() TArray<uint8> _cardIconHasFadeBorder;
	

	UPROPERTY() UMainMenuAssetLoaderComponent* _mainMenuAssetLoader;

	//! ArmyIcon
	//UPROPERTY() TArray<UTexture2D*> _armyIcons;

	//! Military
	//UPROPERTY() TMap<int32, FSpineAsset> _spineAssets;

private:
	UPROPERTY() TArray<FName> _modulesNeedingPaintConstruction;
};
