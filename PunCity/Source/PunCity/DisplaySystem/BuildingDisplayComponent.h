#pragma once

#include "DisplaySystemComponent.h"
#include "StaticFastInstancedMesh.h"
#include "StaticParticleSystemsComponent.h"
#include "PunCity/BuildingMeshesComponent.h"
#include "Components/PointLightComponent.h"

#include "BuildingDisplayComponent.generated.h"


UCLASS( ClassGroup=(Custom), meta=(BlueprintSpawnableComponent) )
class UBuildingDisplayComponent : public UDisplaySystemComponent
{
	GENERATED_BODY()
public:
	UBuildingDisplayComponent()
	{
		PrimaryComponentTick.bCanEverTick = false;
	}

	void Init(int size, TScriptInterface<IDisplaySystemDataSource> gameManager, UAssetLoaderComponent* assetLoader, int32 initialPoolSize) override
	{
		UDisplaySystemComponent::Init(size, gameManager, assetLoader, initialPoolSize);

		_demolishParticlePool.SetNum(100);
		_demolishInfos.resize(100);
		for (int32 i = 0; i < _demolishParticlePool.Num(); i++) {
			_demolishParticlePool[i] = UGameplayStatics::SpawnEmitterAtLocation(this, _assetLoader->particleSystem(ParticleEnum::DemolishDust),
																		FVector::ZeroVector, FRotator::ZeroRotator, FVector::OneVector, false);
			_demolishParticlePool[i]->AttachToComponent(_gameManager->componentToAttach(), FAttachmentTransformRules::KeepRelativeTransform);
		}
	}

	void BeforeAdd() override { 
		//PUN_LOG("BeforeAdd");
		//_radiusCount = 0;
		
		//_constructionBaseCount = 0;
		//_farmCount = 0;
		//_pointLightCount = 0;
	}
	void AfterAdd() override;

	int32 GetObjectId(int32 meshId, FName protoName, int32 instanceIndex) {
		return _moduleMeshes[meshId]->GetObjectId(protoName, instanceIndex);
	}

	void SetHighlight(UStaticFastInstancedMeshesComp* moduleMesh, CardEnum buildingEnum, int32 customDepthIndex)
	{
		LLM_SCOPE_(EPunSimLLMTag::PUN_DisplayBuilding);

		auto highlight = [&](FactionEnum factionEnum)
		{
			const GameDisplayInfo& displayInfo = gameManager()->displayInfo();
			const ModuleTransformGroup& modulePrototype = displayInfo.GetDisplayModules(factionEnum, buildingEnum, 0);
			std::vector<ModuleTransform> modules = modulePrototype.transforms;
			
			for (int i = 0; i < modules.size(); i++)
			{
				if (modules[i].moduleTypeEnum != ModuleTypeEnum::ConstructionOnly &&
					modules[i].upgradeStates == 0)
				{
					FString setName = modulePrototype.setName;
					if (setName == modules[i].moduleName.ToString().Mid(0, setName.Len())) {
						moduleMesh->SetCustomDepth(modules[i].moduleName, customDepthIndex);
					}
				}
			}
				
		};
		
		highlight(FactionEnum::Europe);
		highlight(FactionEnum::Arab);
	}

protected:
	int CreateNewDisplay(int objectId) override;
	void OnSpawnDisplay(int objectId, int meshId, WorldAtom2 cameraAtom) override;
	void UpdateDisplay(int objectId, int meshId, WorldAtom2 cameraAtom, bool justSpawned, bool justCreated) override;
	void HideDisplay(int meshId, int32 regionId) override;

private:
	UInstancedStaticMeshComponent* CreateInstancedMesh(FString name, UStaticMesh* mesh, USceneComponent* parent);

	void UpdateDisplayBuilding(int objectId, int meshId, WorldAtom2 cameraAtom);

	// Radius
	void ShowRadius(int radius, WorldAtom2 centerAtom, Building& building);

	//void ShowSmoke(int radius, WorldAtom2 centerAtom);


	std::pair<GridConnectType, int8_t> GetGridConnectType_Fence(WorldTile2 tile, bool isGate)
	{
		return GetGridConnectType(tile, [&](WorldTile2 tileLocal) {
			Building* bld = simulation().buildingAtTile(tileLocal);
			return (bld != nullptr) && (bld->isEnum(CardEnum::Fence) || bld->isEnum(CardEnum::FenceGate));
		}, false, isGate);
	}
	
	// give back connectionType and mesh rotation from default
	template <typename Func>
	std::pair<GridConnectType, int8_t> GetGridConnectType(WorldTile2 tile, Func isConnected, bool hasEnd, bool isGate)
	{
		bool top = false;
		bool bottom = false;
		bool right = false;
		bool left = false;
		int connectCount = 0;

		WorldTile2 topTile = tile + WorldTile2(1, 0);
		WorldTile2 rightTile = tile + WorldTile2(0, 1);
		WorldTile2 leftTile = tile + WorldTile2(0, -1);
		WorldTile2 bottomTile = tile + WorldTile2(-1, 0);

		// Special case: Gate's road
		if (isGate) {
			const auto& overlaySys = simulation().overlaySystem();
			if (overlaySys.IsRoad(topTile) || overlaySys.IsRoad(bottomTile)) {
				return { GridConnectType::Opposite, 1 };
			}
			if (overlaySys.IsRoad(rightTile) || overlaySys.IsRoad(leftTile)) {
				return { GridConnectType::Opposite, 0 };
			}
		}

		// Calc bools
		if (topTile.x < GameMapConstants::TilesPerWorldX) {
			top = isConnected(topTile);
			connectCount += top;
		}
		if (rightTile.y < GameMapConstants::TilesPerWorldY) {
			right = isConnected(rightTile);
			connectCount += right;
		}
		if (leftTile.y >= 0) {
			left = isConnected(leftTile);
			connectCount += left;
		}
		if (bottomTile.y >= 0) {
			bottom = isConnected(bottomTile);
			connectCount += bottom;
		}

		// Each GridConnectType's default... Clockwise, first arm start on +x
		// { GridConnectType::Three, 2 } means it rotates 2 times from default

		if (connectCount == 4) {
			return { GridConnectType::Four, 0 };
		}
		if (connectCount == 3) {
			if (!left) {
				return { GridConnectType::Three, 0 };
			}
			if (!top) {
				return { GridConnectType::Three, 1 };
			}
			if (!right) {
				return { GridConnectType::Three, 2 };
			}
			else {
				check(!bottom);
				return { GridConnectType::Three, 3 };
			}
		}
		if (connectCount == 2) {
			// Opposite
			if (top && bottom) {
				return { GridConnectType::Opposite, 0 };
			}
			if (left && right) {
				return { GridConnectType::Opposite, 1 };
			}
			// Adjacent
			if (top && right) {
				return { GridConnectType::Adjacent, 0 };
			}
			if (right && bottom) {
				return { GridConnectType::Adjacent, 1 };
			}
			if (bottom && left) {
				return { GridConnectType::Adjacent, 2 };
			}
			if (left && top) {
				return { GridConnectType::Adjacent, 3 };
			}
			else {
				UE_DEBUG_BREAK();
			}
		}
		else if (connectCount == 1) 
		{
			if (hasEnd)
			{
				if (top) return { GridConnectType::End, 0 };
				if (right) return { GridConnectType::End, 1 };
				if (bottom) return { GridConnectType::End, 2 };
				if (left) return { GridConnectType::End, 3 };
			}
			else
			{
				if (top || bottom) {
					return { GridConnectType::Opposite, 0 };
				}
				return { GridConnectType::Opposite, 1 };
			}
		}
		else if (connectCount == 0) {
			return { GridConnectType::Opposite, 1 };
		}

		UE_DEBUG_BREAK();
		return pair<GridConnectType, int8_t>();
	}




	

	void UpdateDisplayOverlay(Building& building, OverlayType overlayType);
	void UpdateDisplayLight(Building& building);

private:
	//UPROPERTY() TArray<class UPointLightComponent*> _lights;
	UPROPERTY() TArray<UStaticParticleSystemsComponent*> _particles;
	
	//UPROPERTY() TArray<UParticleSystemComponent*> _oneShotParticles;

	//! Radius
	//UPROPERTY() TArray<class UDecalComponent*> _radiusDecals;
	UPROPERTY() TArray<UStaticMeshComponent*> _radiusMeshes;
	int32 _radiusCount = 0;
	
	UPROPERTY() TArray<class UDecalComponent*> _farmDecals;
	int32 _farmCount = 0;

	UPROPERTY() TArray<class UDecalComponent*> _constructionBaseDecals;
	int32 _constructionBaseCount = 0;

	//! Lights
	UPROPERTY() TArray<UPointLightComponent*> _pointLights;
	int32 _pointLightCount = 0;
	
	WorldAtom2 _cameraAtom;

	//!
	UPROPERTY() TArray<UStaticFastInstancedMeshesComp*> _moduleMeshes;
	UPROPERTY() TArray<UStaticFastInstancedMeshesComp*> _togglableModuleMeshes;

	std::vector<OverlayType> _lastModuleMeshesOverlayType; // So costly SetHighlight doesn't have to refresh every tick


	//!
	UPROPERTY() TArray<UParticleSystemComponent*> _demolishParticlePool;
	std::vector<DemolishDisplayInfo> _demolishInfos;


};
