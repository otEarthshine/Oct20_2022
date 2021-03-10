// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "DisplaySystemComponent.h"
#include "StaticFastInstancedMeshesComp.h"
#include "Components/SkeletalMeshComponent.h"


DECLARE_CYCLE_STAT(TEXT("PUN: [Display] Unit.Transform"), STAT_PunDisplayUnitTransform, STATGROUP_Game);
DECLARE_CYCLE_STAT(TEXT("PUN: [Display] Unit.Skel"), STAT_PunDisplayUnitSkel, STATGROUP_Game);
DECLARE_CYCLE_STAT(TEXT("PUN: [Display] Unit.Skel1"), STAT_PunDisplayUnitSkel1, STATGROUP_Game);
DECLARE_CYCLE_STAT(TEXT("PUN: [Display] Unit.Skel2"), STAT_PunDisplayUnitSkel2, STATGROUP_Game);
DECLARE_CYCLE_STAT(TEXT("PUN: [Display] Unit.Projectile"), STAT_PunDisplayUnitProjectile, STATGROUP_Game);
DECLARE_CYCLE_STAT(TEXT("PUN: [Display] Unit.AddInst"), STAT_PunDisplayUnitAddInst, STATGROUP_Game);
DECLARE_CYCLE_STAT(TEXT("PUN: [Display] Unit.Resource"), STAT_PunDisplayUnitResource, STATGROUP_Game);

struct FRotationInfo
{
	float rotationFloat = 0;
	float rotationSpeed = 0;
};

struct UnitSkelMeshState
{
	static const int32 animationChangeDelayTicks = 12;
	
	UnitAnimationEnum animationEnum = UnitAnimationEnum::None;
	int32 animationChangeDelayCountDown = animationChangeDelayTicks;
	float animationPlayRate = 10.0f;
	int32 customDepth = 0;
};


#include "UnitDisplayComponent.generated.h"

UCLASS( ClassGroup=(Custom), meta=(BlueprintSpawnableComponent) )
class UUnitDisplayComponent : public UDisplaySystemComponent
{
	GENERATED_BODY()
public:
	UUnitDisplayComponent()
	{
		LLM_SCOPE_(EPunSimLLMTag::PUN_DisplayUnit);
		
		_unitMeshes = CreateDefaultSubobject<UStaticFastInstancedMeshesComp>("_unitMeshes");
		_resourceMeshes = CreateDefaultSubobject<UStaticFastInstancedMeshesComp>("_resourceMeshes");
		_auxMeshes = CreateDefaultSubobject<UStaticFastInstancedMeshesComp>("_auxMeshes");
		_animatingModuleMeshes = CreateDefaultSubobject<UStaticFastInstancedMeshesComp>("_animatingModuleMeshes");

		//_testAnimatedMesh = CreateDefaultSubobject<USkeletalMeshComponent>("_testAnimatedMesh");
	}
	
	void Init(int size, TScriptInterface<IDisplaySystemDataSource> gameManager, UAssetLoaderComponent* assetLoader, int32 initialPoolSize) override
	{
		LLM_SCOPE_(EPunSimLLMTag::PUN_DisplayUnit);
		
		UDisplaySystemComponent::Init(size, gameManager, assetLoader, initialPoolSize);

		//_unitMeshes->Init("Unit", this, 100, "Unit", 0, true);
		_unitMeshes->Init("Unit", this, 100, "", 0, true);
		_auxMeshes->Init("UnitAux", this, 20, "", 0, true);
		for (int i = 0; i < UnitEnumCount; i++) 
		{
			UnitEnum unitEnum = static_cast<UnitEnum>(i);
			
			int32 variationCount = assetLoader->unitMeshCount(unitEnum);
			for (int32 j = 0; j < variationCount; j++) {
				FUnitAsset unitAsset = assetLoader->unitAsset(unitEnum, j);
				_unitMeshes->AddProtoMesh(GetMeshName(unitEnum, j), unitAsset.staticMesh, nullptr);
				if (unitAsset.auxMesh) {
					_auxMeshes->AddProtoMesh(GetMeshName(unitEnum, j), unitAsset.auxMesh);
				}
			}
		}

		_resourceMeshes->Init("UnitResource", this, 100, "", 0, true);
		for (int i = 0; i < ResourceEnumCount; i++) {
			ResourceEnum resourceEnum = static_cast<ResourceEnum>(i);
			_resourceMeshes->AddProtoMesh(ResourceDisplayNameF(resourceEnum), assetLoader->resourceHandMesh(resourceEnum));
		}

		_animatingModuleMeshes->Init("BuildingAnim", this, 20, "", 0, true);
		TArray<FString>& animModuleNames = _assetLoader->animModuleNames();
		for (int i = 0; i < animModuleNames.Num(); i++) {
			UStaticMesh* protoMesh = _assetLoader->moduleMesh(animModuleNames[i]);
			if (protoMesh) {
				_animatingModuleMeshes->AddProtoMesh(animModuleNames[i], protoMesh);
			}
		}

		//_testAnimatedMesh->SetSkeletalMesh(assetLoader->unitSkelMesh(UnitEnum::Human));
	}

	UnitDisplayState GetUnitTransformAndVariation(UnitStateAI& unitAI, FTransform& transform);

	void BeforeAdd() override {
		LLM_SCOPE_(EPunSimLLMTag::PUN_DisplayUnit);

		// TODO: find a way to swap TMap
		//std::swap(_lastTransforms, _thisTransform);
		_lastTransforms = _thisTransform;
		_thisTransform.Empty();
	}
	void AfterAdd() override
	{
		LLM_SCOPE_(EPunSimLLMTag::PUN_DisplayUnit);
		
		_unitMeshes->AfterAdd();
		_resourceMeshes->AfterAdd();
		_auxMeshes->AfterAdd();
		_animatingModuleMeshes->AfterAdd();

		// Clean up last rotator transforms that are out of sight
		auto lastRotatorTransformsCopy = _lastWorkRotatorRotation;
		for (auto it : lastRotatorTransformsCopy) {
			if (simulation().buildingIsAlive(it.Key) &&
				 _gameManager->IsInSampleRange(simulation().buildingCenter(it.Key)))
			{}
			else {
				_lastWorkRotatorRotation.Remove(it.Key);
			}
		}

		SkelMeshAfterAdd();

#if TRAILER_MODE
		/*
		 * Ship
		 */
		static float lastLerpFraction = 0.0f; // No turning back for ship
		if (PunSettings::TrailerAtomTarget_Ship != WorldAtom2::Invalid)
		{
			if (!_smallShip) {
				_smallShip = NewObject<UStaticMeshComponent>(this);
				_smallShip->AttachToComponent(this, FAttachmentTransformRules::KeepRelativeTransform);
				_smallShip->RegisterComponent();
				_smallShip->SetReceivesDecals(false);
				_smallShip->SetStaticMesh(_assetLoader->unitMesh(UnitEnum::SmallShip));
				lastLerpFraction = 0.0f;
			}
			_smallShip->SetVisibility(true);

			// TODO: Calc TrailerAtom_Ship from start/target
			float lerpFraction = gameManager()->networkInterface()->GetCameraSystemMoveLerpFraction();
			lerpFraction = std::max(lastLerpFraction, lerpFraction); // No turning back for ship
			//float lerpFraction = (_gameManager->GetTrailerTime() - PunSettings::TrailerShipStartTime) / (PunSettings::TrailerShipTargetTime - PunSettings::TrailerShipStartTime);
			lerpFraction = Clamp01(lerpFraction);
			lastLerpFraction = lerpFraction;

			WorldAtom2 lerped = WorldAtom2::Lerp(PunSettings::TrailerAtomStart_Ship, PunSettings::TrailerAtomTarget_Ship, static_cast<int64>(lerpFraction * 100000));

			FVector displayLocation = MapUtil::DisplayLocation(_gameManager->cameraAtom(), lerped);

			//PUN_LOG("Ship time:%f lerp:%f %s %s", _gameManager->GetTrailerTime(), lerpFraction, *lerped.worldTile2().To_FString(), *displayLocation.ToCompactString());
			
			_smallShip->SetWorldLocation(displayLocation + FVector(0, 0, -15));
			_smallShip->SetWorldRotation(FRotator(0, -180, 0));
		}
		//else {
		//	if (_smallShip) {
		//		_smallShip->SetVisibility(false);
		//	}
		//}
#endif
	}

	// Hide displays when sampleIds.size() becomes zero (Switch to world map)
	void Display(std::vector<int>& sampleIds) override
	{
		LLM_SCOPE_(EPunSimLLMTag::PUN_DisplayUnit);
		
		bool shouldDisplay = sampleIds.size() != 0;
		_unitMeshes->SetActive(shouldDisplay);
		_resourceMeshes->SetActive(shouldDisplay);
		_auxMeshes->SetActive(shouldDisplay);
		_animatingModuleMeshes->SetActive(shouldDisplay);
		UDisplaySystemComponent::Display(sampleIds);
	}

	void SetHighlight(UnitEnum unitEnum, int32 customDepthIndex)
	{
		LLM_SCOPE_(EPunSimLLMTag::PUN_DisplayUnit);
		
		int32 variationCount = _assetLoader->unitMeshCount(unitEnum);
		for (int32 i = 0; i < variationCount; i++) {
			_unitMeshes->SetCustomDepth(GetMeshName(unitEnum, i), customDepthIndex);
		}
	}

	/*
	 * Debug
	 */
	void RefreshSkelMeshes()
	{
		// Despawn all unused meshes
		for (auto unitSkelMesh : _unitSkelMeshes) {
			unitSkelMesh->SetVisibility(false);
		}
		_lastUnitKeyToSkelMeshIndex.Empty();
		_unitKeyToSkelMeshIndex.Empty();
	}

protected:
	void UpdateDisplay(int regionId, int meshId, WorldAtom2 cameraAtom, bool justSpawned, bool justCreated) override;

private:
	static FString GetMeshName(UnitEnum unitEnum, int32 variationIndex)
	{
		return "Unit" + FString::FromInt(static_cast<int>(unitEnum)) + "_" + FString::FromInt(variationIndex);
	}


private:
	/*
	 * Skel Mesh
	 */

	void AddSkelMesh(UnitStateAI& unit, FTransform& transform) {
		_currentDisplayState = GetUnitTransformAndVariation(unit, transform);

		AddSkelMesh(unit.id(), _currentDisplayState.unitEnum, _currentDisplayState.animationEnum, unit.isChild(), transform, _currentDisplayState.variationIndex, unit.birthTicks());
	}
	
	void AddSkelMesh(int32 unitId, UnitEnum unitEnum, UnitAnimationEnum animationEnum, bool isChild, FTransform& transform, int32 variationIndex, int32 birthTicks)
	{
		SCOPE_CYCLE_COUNTER(STAT_PunDisplayUnitSkel);

		// Key is needed just in case unitEnum changed
		int32 oneK = 1000;
		int64 unitKey = static_cast<int64>(unitId) + static_cast<int64>(unitEnum) * (oneK * oneK) + variationIndex * (oneK * oneK * oneK) + static_cast<int64>(birthTicks) * (oneK * oneK * oneK * oneK); // 1m Ticks are roughly
		
		int32 index = -1;
		// Use the existing unit already displayed
		if (_lastUnitKeyToSkelMeshIndex.Contains(unitKey)) {
			index = _lastUnitKeyToSkelMeshIndex[unitKey];
			_lastUnitKeyToSkelMeshIndex.Remove(unitKey);
		}
		else
		{
			auto setupSkelMesh = [&](USkeletalMeshComponent* skelMesh)
			{
				skelMesh->SetVisibility(true);
				skelMesh->SetCollisionEnabled(PunSettings::Get("DebugTemp2") ? ECollisionEnabled::NoCollision : ECollisionEnabled::QueryAndPhysics);
				skelMesh->bSkipBoundsUpdateWhenInterpolating = PunSettings::Get("DebugTemp3");
				skelMesh->bComponentUseFixedSkelBounds = PunSettings::Get("DebugTemp4");
			};
			
			// Try to spawn new unit from despawn pool
			for (int32 i = 0; i < _unitSkelMeshes.Num(); i++) {
				if (!_unitSkelMeshes[i]->IsVisible()) {
					index = i;
					
					//_unitSkelMeshes[index]->SetVisibility(true);
					setupSkelMesh(_unitSkelMeshes[index]);

					_unitWeaponMeshes[index]->SetVisibility(false);
					_unitSkelState[index] = UnitSkelMeshState();
					break;
				}
			}
			// Spawn brand new unit
			if (index == -1) {
				index = _unitSkelMeshes.Num();
				auto skelMesh = NewObject<USkeletalMeshComponent>(this);
				skelMesh->AttachToComponent(this, FAttachmentTransformRules::KeepRelativeTransform);
				skelMesh->RegisterComponent();
				setupSkelMesh(skelMesh);
				skelMesh->SetReceivesDecals(false);
				_unitSkelMeshes.Add(skelMesh);

				FUnitAsset asset = _assetLoader->unitAsset(unitEnum, variationIndex);
				_unitSkelMeshes[index]->SetSkeletalMesh(asset.skeletalMesh);

				FAttachmentTransformRules attachmentRules(EAttachmentRule::SnapToTarget, false);
				auto weaponMesh = NewObject<UStaticMeshComponent>(this);
				weaponMesh->AttachToComponent(skelMesh, attachmentRules, TEXT("WeaponSocket"));
				weaponMesh->RegisterComponent();
				weaponMesh->SetReceivesDecals(false);
				weaponMesh->SetVisibility(false);
				weaponMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
				_unitWeaponMeshes.Add(weaponMesh);

				_unitSkelState.push_back(UnitSkelMeshState());
			}

			FUnitAsset asset = _assetLoader->unitAsset(unitEnum, variationIndex);
			_unitSkelMeshes[index]->SetSkeletalMesh(asset.skeletalMesh, false);
			
		}
		_unitKeyToSkelMeshIndex.Add(unitKey, index);

		// Setup the mesh
		USkeletalMeshComponent* skelMesh = _unitSkelMeshes[index];
		skelMesh->SetWorldTransform(transform);

		//
		float playRate = 0.7f; // animal
		if (IsHumanDisplay(unitEnum))
		{
			playRate = GetUnitAnimationPlayRate(animationEnum) * simulation().gameSpeedFloat();
			if (isChild) {
				playRate /= 0.8f;
			}

			if (unitEnum == UnitEnum::Human) {
				int32 workEfficiency100 = simulation().unitAI(unitId).subclass<HumanStateAI>().workEfficiency100(true);
				playRate = playRate * workEfficiency100 / 100.0f;
			}
		}
		
		
		if (_unitSkelState[index].animationEnum != animationEnum ||
			fabs(_unitSkelState[index].animationPlayRate - playRate) > 0.01f)
		{
			SCOPE_CYCLE_COUNTER(STAT_PunDisplayUnitSkel1);
			
			FUnitAsset skelAsset = _assetLoader->unitAsset(unitEnum, variationIndex);

			if (animationEnum == UnitAnimationEnum::Invisible) {
				skelMesh->SetVisibility(false);
			}
			else if (skelAsset.animationEnumToSequence.Contains(animationEnum))
			{
				skelMesh->PlayAnimation(skelAsset.animationEnumToSequence[animationEnum], true);
				skelMesh->SetPlayRate(playRate);

				// DEBUG TEST
				//skelMesh->EnableExternalTickRateControl(true);
				//skelMesh->SetExternalTickRate(PunSettings::Get("DebugTemp"));
				skelMesh->bEnableUpdateRateOptimizations = PunSettings::Get("URO");
				skelMesh->bDisplayDebugUpdateRateOptimizations = PunSettings::Get("URODebug");
				//skelMesh->AnimUpdateRateParams->UpdateRate = debugTemp; Not working???

				skelMesh->SetVisibility(true);
			}
			else {
				UE_DEBUG_BREAK();
			}

			_unitSkelState[index].animationEnum = animationEnum;
			_unitSkelState[index].animationPlayRate = playRate;

			UStaticMesh* weaponMesh = _assetLoader->unitWeaponMesh(animationEnum);
			if (weaponMesh) {
				_unitWeaponMeshes[index]->SetStaticMesh(weaponMesh);
				_unitWeaponMeshes[index]->SetVisibility(true);
			} else {
				_unitWeaponMeshes[index]->SetVisibility(false);
			}
		}

		DescriptionUIState uiState = simulation().descriptionUIState();
		int32 targetCustomDepth = (uiState.objectType == ObjectTypeEnum::Unit && uiState.objectId == unitId) ? 2 : 0;
		if (_unitSkelState[index].customDepth != targetCustomDepth) 
		{
			SCOPE_CYCLE_COUNTER(STAT_PunDisplayUnitSkel2);
			
			_unitSkelState[index].customDepth = targetCustomDepth;
			GameDisplayUtils::SetCustomDepth(skelMesh, targetCustomDepth);
		}

		_thisTransform.Add(unitId, transform);
	}

	void SkelMeshAfterAdd()
	{
		// Despawn all unused meshes
		for (auto it : _lastUnitKeyToSkelMeshIndex) {
			_unitSkelMeshes[it.Value]->SetVisibility(false);
		}
		_lastUnitKeyToSkelMeshIndex = _unitKeyToSkelMeshIndex;
		_unitKeyToSkelMeshIndex.Empty(); // _unitIdToSkelMeshIndex.Num());
	}

	void UpdateResourceDisplay(int32 unitId, UnitStateAI& unit, FTransform& transform);
	
private:
	UPROPERTY() UStaticFastInstancedMeshesComp* _unitMeshes;
	UPROPERTY() UStaticFastInstancedMeshesComp* _resourceMeshes;
	UPROPERTY() UStaticFastInstancedMeshesComp* _auxMeshes;

	TMap<int32, FTransform> _lastTransforms;
	TMap<int32, FTransform> _thisTransform;

	UPROPERTY() UStaticFastInstancedMeshesComp* _animatingModuleMeshes;

	TMap<int32, FRotationInfo> _lastWorkRotatorRotation;

	//UPROPERTY() USkeletalMeshComponent* _testAnimatedMesh;

	// Skel
	UPROPERTY() TArray<USkeletalMeshComponent*> _unitSkelMeshes;
	UPROPERTY() TArray<UStaticMeshComponent*> _unitWeaponMeshes;
	std::vector<UnitSkelMeshState> _unitSkelState;
	TMap<int64, int32> _unitKeyToSkelMeshIndex;
	TMap<int64, int32> _lastUnitKeyToSkelMeshIndex;

	UnitDisplayState _currentDisplayState;

	// Trailer special
	UPROPERTY() UStaticMeshComponent* _smallShip = nullptr;
};
