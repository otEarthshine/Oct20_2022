// Fill out your copyright notice in the Description page of Project Settings.


#include "PunUnrealUtils.h"
#include "Components/DecalComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Kismet/GameplayStatics.h"
#include "PunCity/PunUtils.h"
#include "PunCity/PunTimer.h"
#include "PunCity/DisplaySystem/DisplaySystemDataSource.h"
#include "Components/PointLightComponent.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "PunCity/UI/GameUIDataSource.h"

using namespace std;

UDecalComponent* PunUnrealUtils::CreateDecal(USceneComponent* scene, UMaterialInterface* material)
{
	auto decal = NewObject<UDecalComponent>(scene);
	decal->AttachToComponent(scene, FAttachmentTransformRules::KeepRelativeTransform, NAME_None);
	decal->SetRelativeRotation(FVector(0, 0, -1).Rotation()); // Rotate to project the decal down
	decal->SetMaterial(0, material);
	decal->DecalSize = FVector(100, 160 / 32, 160 / 32); // 160 is the region size... div by 32 to get a decal with 1x1 tile in size
	decal->SetWorldScale3D(FVector::ZeroVector); // TODO: Hack around weird decals on startup...
	decal->RegisterComponent();
	return decal;
}

UDecalComponent* PunUnrealUtils::ShowDecal(WorldAtom2 centerAtom, WorldTile2 size, TArray<UDecalComponent*>& decals, int32& decalCount, 
								USceneComponent* scene, UMaterialInterface* material, IDisplaySystemDataSource* gameInterface, bool useMaterialInstance, float scaleX)
{
	int currentIndex = decalCount;
	decalCount++;

	for (int32 i = decals.Num(); i < decalCount; i++) 
	{
		UMaterialInterface* materialUsed = material;
		if (useMaterialInstance) {
			materialUsed = UMaterialInstanceDynamic::Create(material, scene);
		}
		
		decals.Add(PunUnrealUtils::CreateDecal(scene, materialUsed));
	}

	auto decal = decals[currentIndex];
	if (!useMaterialInstance) {
		decal->SetDecalMaterial(material);
	}
	
	FVector displayLocation = gameInterface->DisplayLocation(centerAtom);

	// TODO: why this hack +.+
	displayLocation += FVector(2.5, 2.5, 0);

	PunUnrealUtils::SetActive(decals[currentIndex], true);
	decal->SetWorldLocation(displayLocation);
	decal->SetWorldScale3D(FVector(scaleX, size.y, size.x));

	decalCount++;

	return decal;
}

void PunUnrealUtils::UpdateDecals(TArray<UDecalComponent*>& decals, int32& decalCount)
{
	for (int i = decalCount; i < decals.Num(); i++) {
		SetActive(decals[i], false);
	}

	decalCount = 0;
}

UPointLightComponent* PunUnrealUtils::CreateLight(class USceneComponent* scene)
{
	auto pointLight = NewObject<UPointLightComponent>(scene);
	pointLight->AttachToComponent(scene, FAttachmentTransformRules::KeepRelativeTransform, NAME_None);
	pointLight->RegisterComponent();

	pointLight->SetAffectTranslucentLighting(false);
	pointLight->SetCastShadows(false);
	pointLight->SetCastRaytracedShadow(false);
	pointLight->SetAffectReflection(false);
	pointLight->SetAffectGlobalIllumination(false);
	pointLight->SetIntensity(0); // TODO: Hack around weird decals on startup...

	pointLight->MaxDrawDistance = 580;
	pointLight->MaxDistanceFadeRange = 150;
	
	return pointLight;
}

UPointLightComponent* PunUnrealUtils::ShowLight(FTransform transform, TArray<UPointLightComponent*>& pointLights, int32& pointLightCount, USceneComponent* scene)
{
	int currentIndex = pointLightCount;
	pointLightCount++;

	for (int32 i = pointLights.Num(); i < pointLightCount; i++) {
		pointLights.Add(PunUnrealUtils::CreateLight(scene));
	}
	//while (pointLightCount > pointLights.Num()) {
	//	pointLights.Add(PunUnrealUtils::CreateLight(scene));
	//}

	PunUnrealUtils::SetActive(pointLights[currentIndex], true);
	pointLights[currentIndex]->SetWorldTransform(transform);
	//pointLights[currentIndex]->SetWorldLocation(displayLocation);
	//pointLights[currentIndex]->SetWorldScale3D(scale);

	pointLightCount++;

	return pointLights[currentIndex];
}

void PunUnrealUtils::UpdateLights(TArray<UPointLightComponent*>& pointLights, int32& pointLightCount)
{
	for (int i = pointLightCount; i < pointLights.Num(); i++) {
		SetActive(pointLights[i], false);
	}
	pointLightCount = 0;
}


UStaticMeshComponent* PunUnrealUtils::CreateStaticMesh(class USceneComponent* scene)
{
	UStaticMeshComponent* mesh = NewObject<UStaticMeshComponent>(scene);
	mesh->AttachToComponent(scene, FAttachmentTransformRules::KeepRelativeTransform, NAME_None);
	mesh->SetGenerateOverlapEvents(false);
	mesh->RegisterComponent();
	return mesh;
}

AActor* PunUnrealUtils::FindWorldActor(UWorld* world, FName name)
{
	TArray<AActor*> found;
	UGameplayStatics::GetAllActorsWithTag(world, name, found);
	PUN_CHECK(found.Num() == 1);

	return found[0];
}
TArray<AActor*> PunUnrealUtils::FindWorldActors(UWorld* world, FName name)
{
	TArray<AActor*> found;
	UGameplayStatics::GetAllActorsWithTag(world, name, found);
	return found;
}

UTexture2D* PunUnrealUtils::CreateTexture2D(int32 dimX, int32 dimY)
{
	UTexture2D* texture = UTexture2D::CreateTransient(dimX, dimY, PF_B8G8R8A8);
	texture->Filter = TF_Bilinear;
	texture->SRGB = false;
	texture->AddToRoot();
	texture->UpdateResource();
	return texture;
}

void PunUnrealUtils::SetTextureData(UTexture2D* texture, std::vector<uint32>& heightData)
{
	SCOPE_TIMER("SetTextureData");
	uint32* textureData = reinterpret_cast<uint32*>(texture->PlatformData->Mips[0].BulkData.Lock(LOCK_READ_WRITE));
	const size_t dataSize = texture->GetSizeX() * texture->GetSizeY() * sizeof(uint32);
	FMemory::Memcpy(textureData, heightData.data(), dataSize);
	texture->PlatformData->Mips[0].BulkData.Unlock();
	texture->UpdateResource();
}