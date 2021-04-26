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

	int32 GetObjectId(int32 meshId, FString protoName, int32 instanceIndex) {
		return _moduleMeshes[meshId]->GetObjectId(protoName, instanceIndex);
	}

	void SetHighlight(UStaticFastInstancedMeshesComp* moduleMesh, CardEnum buildingEnum, int32 customDepthIndex)
	{
		LLM_SCOPE_(EPunSimLLMTag::PUN_DisplayBuilding);
		
		const GameDisplayInfo& displayInfo = gameManager()->displayInfo();
		const ModuleTransformGroup& modulePrototype = displayInfo.GetDisplayModules(buildingEnum, 0);
		std::vector<ModuleTransform> modules = modulePrototype.transforms;
		
		for (int i = 0; i < modules.size(); i++)
		{
			if (modules[i].moduleTypeEnum != ModuleTypeEnum::ConstructionOnly &&
				modules[i].upgradeStates == 0)
			{
				FString setName = modulePrototype.setName;
				if (setName == modules[i].moduleName.Mid(0, setName.Len())) {
					moduleMesh->SetCustomDepth(modules[i].moduleName, customDepthIndex);
				}
			}
		}
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

	// give back connectionType and mesh rotation from default
	std::pair<GridConnectType, int8_t> GetGridConnectType(WorldTile2 tile, bool isGate = false);

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
