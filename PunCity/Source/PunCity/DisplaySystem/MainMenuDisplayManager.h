// Pun Dumnernchanvanit's

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "TileObjectDisplayComponent.h"
#include "BuildingDisplayComponent.h"
#include "RegionDisplayComponent.h"
#include "ResourceDisplayComponent.h"
#include "RegionDecalDisplayComponent.h"
#include "Engine/PostProcessVolume.h"
#include "MainMenuDisplayManager.generated.h"

UCLASS()
class PROTOTYPECITY_API AMainMenuDisplayManager : public AActor, public IDisplaySystemDataSource
{
	GENERATED_BODY()
public:
	AMainMenuDisplayManager();
	~AMainMenuDisplayManager();

	void InitMainMenuDisplayManager(MapSizeEnum mapSizeEnum);

	void UpdateDisplay(WorldAtom2 cameraAtom, float zoomDistance, std::vector<int32> sampleRegionIds, bool justSpawned, bool justCreated);

	void UpdatePostProcessVolume() {
		SetPostProcessVolume(_postProcessVolume, CastChecked<UPunGameInstance>(GetGameInstance()));
	}

	void SendNetworkCommand(std::shared_ptr<FNetworkCommand> networkCommand) override {}

	UPROPERTY(EditAnywhere, BlueprintReadWrite) USceneComponent* _root;

	UPROPERTY(EditAnywhere) URegionDisplayComponent* _regionDisplaySystem;
	UPROPERTY(EditAnywhere) URegionDecalDisplayComponent* _decalDisplaySystem;
	
	UPROPERTY(EditAnywhere) UBuildingDisplayComponent* _buildingDisplaySystem;
	UPROPERTY(EditAnywhere) UResourceDisplayComponent* _resourceDisplaySystem;
	UPROPERTY(EditAnywhere) UTileObjectDisplayComponent* _tileDisplaySystem;

	UPROPERTY(EditAnywhere) APostProcessVolume* _postProcessVolume;

	UPROPERTY(EditAnywhere) UAssetLoaderComponent* _assetLoader;

	std::unique_ptr<GameSimulationCore> _simulation;

	GameDisplayInfo _displayInfo;

	float _zoomDistance;
	WorldAtom2 _cameraAtom;
	TileArea _sampleArea;

	std::vector<int32> _sampleProvinceIds_Empty;

	bool _isTexturesLoaded = false;

	bool failedToLoadMainMenuJson = false;
	
	/**
	 * IDisplaySystemDataSource
	 */
	int32 playerId() final { return 0; }

	virtual FPlayerInfo playerInfo(int32 playerId) final { return FPlayerInfo(); }
	
	UAssetLoaderComponent* assetLoader() final { return _assetLoader; }

	IGameNetworkInterface* networkInterface() final { return nullptr; }

	GameSimulationCore& simulation() final { return *_simulation; }
	USceneComponent* componentToAttach() final { return _root; }

	OverlayType GetOverlayType() final { return OverlayType::None; }
	virtual bool isHidingTree() override { return false; }
	virtual bool isShowingProvinceOverlay() override { return false; }
	virtual bool isShowingDefenseNodes() override { return false; }

	ULineBatchComponent* lineBatch() final {
		if (UWorld* world = GetWorld()) {
			return world->LineBatcher;
		}
		return nullptr;
	}

	const GameDisplayInfo& displayInfo() final { return _displayInfo; }

	//bool isMapMode() final { return _zoomDistance >= WorldToMapZoomAmount; }
	float zoomDistance() final { return _zoomDistance; }
	float smoothZoomDistance() final { return _zoomDistance; }
	
	bool ZoomDistanceBelow(float threshold) final { return _zoomDistance < threshold; }
	WorldAtom2 cameraAtom() final { return _cameraAtom; }
	TileArea sampleArea() final { return _sampleArea; }

	const std::vector<int32>& sampleProvinceIds() final { return _sampleProvinceIds_Empty; }

	FVector DisplayLocation(WorldAtom2 atom) final { return MapUtil::DisplayLocation(_cameraAtom, atom); }

	float GetTrailerTime() final { return 0.0f; }

	virtual void GetDefenseNodeDisplayInfo(int32 provinceId, float displayScaling, 
		DefenseOverlayEnum& defenseOverlayEnum_Out, DefenseColorEnum& defenseColorEnum,
		FTransform& nodeTransform_Out, TArray<FTransform>& lineTransforms_Out, TArray<DefenseColorEnum>& lineDefenseColorEnums_Out, bool isMap) override {}
	
	int timeSinceTickStart() final {
		return 0;
	}

	float GetTerrainDisplayHeight(WorldTile2 tile) final {
		return _regionDisplaySystem->GetTerrainDisplayHeight(tile);
	}

	bool isPhotoMode() final { return false; }

	bool IsInSampleRange(WorldTile2 tile) override { return true; }
};
