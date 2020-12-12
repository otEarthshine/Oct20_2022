// Pun Dumnernchanvanit's

#pragma once

#include "CoreMinimal.h"
#include "PunCity/DisplaySystem/DisplaySystemComponent.h"
#include "Materials/MaterialInstanceDynamic.h"

#include "RegionDecalDisplayComponent.generated.h"

USTRUCT()
struct FPunDecal
{
	GENERATED_BODY()
	UPROPERTY() class UDecalComponent* decal = nullptr;
	UPROPERTY() class UMaterialInstanceDynamic* material = nullptr;
	UPROPERTY() class UTexture2D* texture = nullptr;

	FPunDecal(class UDecalComponent* decal, class UMaterialInstanceDynamic* material, class UTexture2D* texture)
		: decal(decal), material(material), texture(texture)
	{}
	FPunDecal() {}
};


UCLASS()
class URegionDecalDisplayComponent : public UDisplaySystemComponent
{
	GENERATED_BODY()
public:
	URegionDecalDisplayComponent() { PrimaryComponentTick.bCanEverTick = false; }
	
	int CreateNewDisplay(int objectId) override;
	void OnSpawnDisplay(int regionId, int meshId, WorldAtom2 cameraAtom) override;
	void UpdateDisplay(int regionId, int meshId, WorldAtom2 cameraAtom, bool justSpawned, bool justCreated) override;
	void HideDisplay(int meshId, int32 regionId) override;
	
	void UpdateRoadDisplay(int regionId, int meshId, OverlaySystem& overlaySystem);
	
private:
	UPROPERTY() TArray<FPunDecal> _roadDecals;
	UPROPERTY() TArray<FPunDecal> _overlayDecals;
	//UPROPERTY() TArray<class UDecalComponent*> _wetnessDecals;

	UPROPERTY() TArray<class UDecalComponent*> _oreDecals;

	TileArea _lastDemolishArea;
	//OverlayType _lastOverlayType = OverlayType::None;

	int32 roadTextureDim;
};
