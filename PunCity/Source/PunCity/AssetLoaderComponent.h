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

#include "AssetLoaderComponent.generated.h"

enum class TileSubmeshEnum
{
	Trunk,
	Leaf,
	Fruit,
	LeafLowPoly,
	LeafShadow,
};

const std::string TileSubmeshName[]
{
	"Trunk",
	"Leaf",
	"Fruit",
	"LeafLowPoly",
	"LeafShadow",
};
static const int32 TileSubmeshCount = _countof(TileSubmeshName);


enum class ParticleEnum
{
	Smoke,
	BlackSmoke,
	StoveFire,
	TorchFire,
	
	CampFire,
	BuildingFire,
	BuildingFireSmoke,

	DemolishDust,

	Count,
};

USTRUCT()
struct FSkeletonAsset
{
	GENERATED_BODY();
	
	UPROPERTY() USkeletalMesh* skeletalMesh = nullptr;
	UPROPERTY() TMap<UnitAnimationEnum, UAnimSequence*> animationEnumToSequence;
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

	void PaintConstructionMesh();
	void UpdateRHIConstructionMesh();
	void PrintConstructionMesh();
	void RemoveVertexColor();

private:
	void TraverseTris(uint32 groupIndex_PositionMerge, int32 groupIndex_Parent, const TArray<uint32>& indexBuffer, const TArray<FVector>& vertexPositions);

public:
	UStaticMesh* moduleMesh(FString moduleName);

	bool HasConstructionMesh(FString moduleName) { return _moduleNameToConstructionMesh.Contains(moduleName); }
	UStaticMesh* moduleConstructionMesh(FString moduleName);

	//! Module types are used to create separate mesh sets
	const TArray<FString>& moduleNames()
	{
		//PUN_CHECK(_moduleNames.Num() == ModuleMeshCount);
		return _moduleNames;
	}
	TArray<FString>& animModuleNames() { return _animModuleNames; }
	TArray<FString>& togglableModuleNames() { return _togglableModuleNames; }

	UStaticMesh* unitMesh(UnitEnum unitEnum, int32 variationIndex = 0);
	int32 unitMeshCount(UnitEnum unitEnum) { return _unitToMeshes[unitEnum].size(); }

	FSkeletonAsset unitSkelAsset(UnitEnum unitEnum, int32 variationIndex = 0);
	
	UStaticMesh* unitWeaponMesh(UnitAnimationEnum animationEnum) {
		auto found = _animationEnumToWeaponMesh.find(animationEnum);
		if (found != _animationEnumToWeaponMesh.end()) {
			return found->second;
		}
		return nullptr;
	}

	UStaticMesh* resourceMesh(ResourceEnum resourceEnum);
	UStaticMesh* resourceHandMesh(ResourceEnum resourceEnum);

	FGeoresourceMeshAssets georesourceMesh(GeoresourceEnum georesourceEnum);

	UParticleSystem* particleSystem(ParticleEnum particleEnum) { return ParticlesByEnum[static_cast<int>(particleEnum)]; }

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
	//UPROPERTY(EditAnywhere, Category = "Material Import") UMaterial* M_WorldMap;
	
	UPROPERTY(EditAnywhere, Category = "Mesh Import") UStaticMesh* WorldMapCityBlock;
	UPROPERTY(EditAnywhere, Category = "Mesh Import") UStaticMesh* WorldMapFlag;
	UPROPERTY(EditAnywhere, Category = "Mesh Import") UStaticMesh* WorldMapFlagBase;
	UPROPERTY(EditAnywhere, Category = "Mesh Import") UStaticMesh* WorldMapCameraSpot;
	UPROPERTY(EditAnywhere, Category = "Material Import") UMaterial* WorldMapBlockMaterial;

	UPROPERTY(EditAnywhere) UStaticMesh* WorldMapForestMesh;

	UPROPERTY(EditAnywhere, Category = "Material Import") UMaterial* ReferenceTerrainMaterial;

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

	UPROPERTY(EditAnywhere) UTexture2D* AdultIcon;
	UPROPERTY(EditAnywhere) UTexture2D* AdultIconSquare;
	UPROPERTY(EditAnywhere) UTexture2D* ChildIcon;
	UPROPERTY(EditAnywhere) UTexture2D* ExclamationIcon;

	//! Hover warning
	UPROPERTY(EditAnywhere) UMaterial* M_HoverWarning;
	UPROPERTY(EditAnywhere) UTexture2D* WarningHouse;
	UPROPERTY(EditAnywhere) UTexture2D* WarningStarving;
	UPROPERTY(EditAnywhere) UTexture2D* WarningSnow;
	UPROPERTY(EditAnywhere) UTexture2D* WarningHealthcare;
	UPROPERTY(EditAnywhere) UTexture2D* WarningTools;
	UPROPERTY(EditAnywhere) UTexture2D* WarningUnhappy;
	

	//! HoverIcon
	UPROPERTY(EditAnywhere) UTexture2D* UnhappyHoverIcon;

	UPROPERTY(EditAnywhere) UTexture2D* BlackIcon;

	//! Videos
	//UPROPERTY(EditAnywhere) UMediaPlayer* MediaPlayer;


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

	UPROPERTY(EditAnywhere) UMaterial* M_GeoresourceIcon;
	UPROPERTY(EditAnywhere) TMap<int32, UTexture*> _geo_Icon;
	UPROPERTY(EditAnywhere) TMap<int32, UTexture*> _geo_IconAlpha;


	

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
	void LoadUnitSkel(UnitEnum unitEnum, std::string folderPath, std::string skelFileName, std::unordered_map<UnitAnimationEnum, std::string> animationFileNames);
	//void LoadUnitAnimation(UnitEnum unitEnum, int32 variationIndex, UnitAnimationEnum unitAnimation, std::string file);
	void LoadUnitWeapon(UnitAnimationEnum unitAnimation, std::string file);
	
	void LoadResource(ResourceEnum resourceEnum, std::string meshFile);
	void LoadResource2(ResourceEnum resourceEnum, std::string meshFilePrefix);

	void LoadModule(FString moduleName , FString meshFile, bool paintConstructionVertexColor = false, bool isTogglable = false);
	void LoadTogglableModule(FString moduleName, FString meshFile);
	void LoadAnimModule(FString moduleName, FString meshFile);
	void TryLoadBuildingModuleSet(FString moduleSetName, FString meshSetFolder);

	void LoadModuleWithConstruction(FString moduleName, FString meshFile);

	void LoadGeoresource(GeoresourceEnum georesourceEnum, std::string folder, std::string meshFileNamePrefix, int32 numberOfMeshes);
	void LoadTree(TileObjEnum treeEnum, std::string trunkMeshFile, std::string leafMeshFile,
									std::string fruitMeshFile, std::string leafLowMeshFile, std::string leafShadowMeshFile);
	void LoadTileObject(TileObjEnum treeEnum, std::vector<std::string> meshFiles);

	void PaintMeshForConstruction(FString moduleName);

private:
	UPROPERTY() TMap<FString, UStaticMesh*> _moduleNameToMesh;
	UPROPERTY() TMap<FString, UStaticMesh*> _moduleNameToConstructionMesh;
	
	UPROPERTY() TArray<FString> _moduleNames;
	UPROPERTY() TArray<FString> _animModuleNames;
	UPROPERTY() TArray<FString> _togglableModuleNames;
	
	std::unordered_map<UnitEnum, std::vector<UStaticMesh*>> _unitToMeshes;

	std::vector<std::vector<FSkeletonAsset>> _unitEnumToSkelAsset;
	//std::unordered_map<UnitEnum, std::vector<USkeletalMesh*>> _unitToSkeletalMesh;
	//std::unordered_map<UnitEnum, std::vector<std::unordered_map<UnitAnimationEnum, UAnimSequence*>>> _animationEnumToSequence;
	
	std::unordered_map<UnitAnimationEnum, UStaticMesh*> _animationEnumToWeaponMesh;
	
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
