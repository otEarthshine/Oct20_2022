// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "PunCity/PunUtils.h"

/**
 * 
 */
class UDecalComponent;
class USceneComponent;
class UMaterialInterface;
class IDisplaySystemDataSource;
class IGameUIDataSource;

FORCEINLINE bool IsValidPun(const UObject *Test)
{
	return Test && Test->IsValidLowLevel() && !Test->IsPendingKill();
	//return IsValid(Test) && Test->IsValidLowLevel();
}

class PunUnrealUtils
{
public:
	// Decal... Note: Only need ShowDecal() and UpdateDecal()
	static UDecalComponent* CreateDecal(USceneComponent* scene, UMaterialInterface* material);

	static UDecalComponent* ShowDecal(WorldAtom2 centerAtom, WorldTile2 size, TArray<UDecalComponent*>& decals, int32& decalCount, 
										USceneComponent* scene, UMaterialInterface* material, IDisplaySystemDataSource* gameInterface, bool useMaterialInstance = false, float scaleX = 1.0f);
	static void UpdateDecals(TArray<UDecalComponent*>& decals, int32& decalCount);

	static void UpdateMeshes(TArray<UStaticMeshComponent*>& meshes, int32& meshCount);

	// Light... Note: Only need ShowLight() and UpdateLights()
	static class UPointLightComponent* CreateLight(USceneComponent* scene);
	static UPointLightComponent* ShowLight(FTransform transform, TArray<UPointLightComponent*>& pointLights, int32& pointLightCount, USceneComponent* scene);
	static void UpdateLights(TArray<UPointLightComponent*>& pointLights, int32& pointLightCount);
	
	// Mesh
	static class UStaticMeshComponent* CreateStaticMesh(USceneComponent* scene);
	static class AActor* FindWorldActor(UWorld* world, FName name);
	static TArray<AActor*> FindWorldActors(UWorld* world, FName name);

	static class UTexture2D* CreateTexture2D(int32 dimX, int32 dimY);
	static void SetTextureData(UTexture2D* texture, std::vector<uint32>& heightData);

	static void DestroyTexture2D(UTexture2D* texture) {
		// TODO: need IsValidLowLevel?
		if (IsValidPun(texture)) {
			texture->RemoveFromRoot();
			texture->ConditionalBeginDestroy();
		}
	}

	static void SetActive(USceneComponent* object, bool active) {
		object->SetVisibility(active);
		object->SetActive(active);
	}

	template <class T>
	static T* GetComponent(AActor* actor)
	{
		TArray<T*> components;
		actor->GetComponents<T>(components);
		PUN_CHECK(components.Num() == 1);
		return components[0];
	}

	static uint8 FractionTo255Color(float fraction)
	{
		int32_t color255 = static_cast<int32_t>(255.0f * fraction);
		return static_cast<uint8>(std::min(255, std::max(0, color255)));
	}

	static void ShowComponent(USceneComponent* component, FVector displayLocation, FVector scale) {
		component->SetVisibility(true);
		component->SetWorldLocation(displayLocation);
		component->SetWorldScale3D(scale);
	}

	static FLinearColor GetRandomColor(int32 seed) {
		return FLinearColor(
			(GameRand::DisplayRand(seed) % 255) / 255.0f,
			(GameRand::DisplayRand(seed + 1) % 255) / 255.0f,
			(GameRand::DisplayRand(seed + 2) % 255) / 255.0f
		);
	}
};


// String Utils
static std::wstring StringEnvelopImgTag(std::wstring str, std::wstring envelopTag)
{
	for (size_t i = str.size() - 3; i-- > 0;) {
		if (str.substr(i, 3) == TEXT("<im")) {
			str.insert(i, TEXT("</>"));
		}
		if (str.substr(i, 3) == TEXT("\"/>")) {
			str.insert(i + 3, envelopTag);
		}
	}
	return str;
}

static bool FStringCompareRight(const FString& str, FString rightStr, int32 endChop = 0) {
	return str.LeftChop(endChop).Right(rightStr.Len()) == rightStr;
}



// Serializer
static void SerializeArray(TArray<int32>& blob, TArray<uint8>& inArray) {
	blob.Add(inArray.Num());
	blob.Append(inArray);
}
static void DeserializeArray(const TArray<int32>& blob, int32& index, TArray<uint8>& inArray) {
	int32 count = blob[index++];
	for (int i = 0; i < count; i++) {
		inArray.Add(blob[index++]);
	}
}
static void SerializeArray(TArray<int32>& blob, TArray<int32>& inArray) {
	blob.Add(inArray.Num());
	blob.Append(inArray);
}
static TArray<int32> DeserializeArray(const TArray<int32>& blob, int32& index) {
	TArray<int32> result;
	int32 count = blob[index++];
	for (int i = 0; i < count; i++) {
		result.Add(blob[index++]);
	}
	return result;
}

