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


class PunUnrealUtils
{
public:
	// Decal... Note: Only need ShowDecal() and UpdateDecal()
	static UDecalComponent* CreateDecal(USceneComponent* scene, UMaterialInterface* material);

	static UDecalComponent* ShowDecal(WorldAtom2 centerAtom, WorldTile2 size, TArray<UDecalComponent*>& decals, int32& decalCount, 
										USceneComponent* scene, UMaterialInterface* material, IDisplaySystemDataSource* gameInterface, bool useMaterialInstance = false);
	static void UpdateDecals(TArray<UDecalComponent*>& decals, int32& decalCount);

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

};


static void FString_SerializeAndAppendToBlob(FString inStr, TArray<int32>& arr)
{
	arr.Add(inStr.Len());
	for (int32 i = 0; i < inStr.Len(); i++) {
		arr.Add(static_cast<int32>(inStr[i]));
	}
}

static FString FString_DeserializeFromBlob(const TArray<int32>& arr, int32& readIndex)
{
	FString result;
	int32 len = arr[readIndex++];
	for (int32 i = 0; i < len; i++) {
		result.AppendChar(static_cast<TCHAR>(arr[readIndex++]));
	}
	return result;
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

