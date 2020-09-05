
#include "DisplaySystemComponent.h"
#include "Components/StaticMeshComponent.h"

void UDisplaySystemComponent::Display(std::vector<int>& sampleIds)
{
	BeforeAdd();

	WorldAtom2 cameraAtom = gameManager()->cameraAtom();

	bool cameraMovedLargeDistance = WorldTile2::Distance(_lastCameraTile, cameraAtom.worldTile2());
	_lastCameraTile = cameraAtom.worldTile2();
	
	// When display just switched from off to on (because of zoom), we should display everything to prevent display hole flashing on the screen.
	bool displayJustSwitchedOn = _lastSampleSize == 0 && sampleIds.size() > 0;
	_lastSampleSize = sampleIds.size();

	_spawnedThisTick = 0;
	
	/**
	 * Display Regions
	 */
	for (int i = 0; i < _isMeshesInUse.size(); i++) {
		_isMeshesInUse[i] = false;
	}

	const size_t sampleIdsSize = sampleIds.size();
	for (size_t i = 0; i < sampleIdsSize; i++)
	{
		int objectId = sampleIds[i]; // objectId is mostly regionId

		// Resize _meshIdByObjectId if necessary

		PUN_CHECK(objectId >= 0);
		for (int32 j = _meshIdByObjectId.size(); j <= objectId; j++) {
			_meshIdByObjectId.push_back(-1);
		}
		
		//while (objectId >= _meshIdByObjectId.size()) {
		//	_meshIdByObjectId.push_back(-1);
		//}

		int meshId = _meshIdByObjectId[objectId];

		if (meshId == -1)
		{
			// Find disabled mesh and use it if there is one
			for (size_t j = 0; j < _objectIdByMeshId.size(); j++) {
				if (_objectIdByMeshId[j] == -1) {
					meshId = j;
					break;
				}
			}

			// Display Skip for smoothness...
			if (_spawnedThisTick >= _maxSpawnPerTick) { // && !displayJustSwitchedOn && !cameraMovedLargeDistance) {
				continue;
			}
			_spawnedThisTick++;

			// Make a new mesh if there no disabled mesh
			if (meshId == -1)
			{
				meshId = CreateNewDisplay(objectId);

				_objectIdByMeshId.push_back(objectId);
				_isMeshesInUse.push_back(true);
			}

			_objectIdByMeshId[meshId] = objectId;
			_meshIdByObjectId[objectId] = meshId;

			OnSpawnDisplay(objectId, meshId, cameraAtom);
		}

		_isMeshesInUse[meshId] = true;

		UpdateDisplay(objectId, meshId, cameraAtom);
	}

	// Take unused mesh and disable them if necessary
	for (int i = 0; i < _isMeshesInUse.size(); i++)
	{
		if (!_isMeshesInUse[i])
		{
			// Not in use anymore, disable it
			int regionId = _objectIdByMeshId[i];
			if (regionId != -1)
			{
				HideDisplay(i, regionId);

				_meshIdByObjectId[regionId] = -1;
				_objectIdByMeshId[i] = -1;
				_isMeshesInUse[i] = false;
			}
		}
	}

	AfterAdd();
}

void UDisplaySystemComponent::Init(int size, TScriptInterface<IDisplaySystemDataSource> gameManager, UAssetLoaderComponent * assetLoader)
{
	_gameManager = gameManager;
	_assetLoader = assetLoader;

	// This vector map from regionId to regionMeshId
	_meshIdByObjectId.resize(size);
	for (int i = 0; i < size; i++) {
		_meshIdByObjectId[i] = -1;
	}

	AttachToComponent(gameManager->componentToAttach(), FAttachmentTransformRules::KeepWorldTransform);
	//PUN_LOG("Init attaching to %s", *(GetAttachParent()->GetName()));

	_meshId = 0;
}

UStaticMeshComponent* UDisplaySystemComponent::CreateMeshComponent(USceneComponent* parent, std::string name)
{
	UStaticMeshComponent* meshComponent = NewObject<UStaticMeshComponent>(parent, UStaticMeshComponent::StaticClass());
	meshComponent->Rename(*FString(name.c_str()));
	meshComponent->SetGenerateOverlapEvents(false);
	meshComponent->AttachToComponent(parent, FAttachmentTransformRules::KeepRelativeTransform, NAME_None);
	meshComponent->SetMobility(EComponentMobility::Movable);
	meshComponent->RegisterComponent();
	return meshComponent;
}
