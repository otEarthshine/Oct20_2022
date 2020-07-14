// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "PunCity/PunUtils.h"
#include "Engine/CanvasRenderTarget2D.h"
#include "BuildingMeshesComponent.h"
#include "Kismet/GameplayStatics.h"
#include "GameManager.h"
#include "Components/SceneCaptureComponent2D.h"
#include "Engine/SceneCapture2D.h"

#include "AssetIconBaker.generated.h"

class UTextureRenderTarget2D;

UCLASS()
class AAssetIconBaker : public AActor
{
	GENERATED_BODY()
public:	
	// Sets default values for this actor's properties
	AAssetIconBaker();

	static FString SaveGameDir(const FString& SaveGameName);

	static FString ContentGameDir(const FString& ContentName);

	static UTexture2D* ImportSaveThumbnail(UObject* WorldContextObject, const FString& ImportPath);

	static void ExportSaveThumbnailRT(UObject* WorldContextObject, UTextureRenderTarget2D* TextureRenderTarget, const FString& ExportPath);
	static void ExportThumbnailHDR(UObject* WorldContextObject, UTextureRenderTarget2D* TextureRenderTarget, const FString& ExportPath);

	static void DeleteSaveThumbnail(UObject* WorldContextObject, const FString& SaveGameName);

	void ExportIcon(const FString& iconPath, int32 index)
	{
		FString iconDirectory = ContentGameDir(iconPath + FString::FromInt(index) + FString(".exr"));
		ExportThumbnailHDR(GetWorld(), IconRenderTarget, iconDirectory);

		FString iconDirectoryAlpha = ContentGameDir(iconPath + FString("Alpha") + FString::FromInt(index) + FString(".exr"));
		ExportThumbnailHDR(GetWorld(), IconRenderTargetAlpha, iconDirectoryAlpha);
	}

	void BakeCustomIcon()
	{
		auto skyMesh = PunUnrealUtils::GetComponent<UStaticMeshComponent>(PunUnrealUtils::FindWorldActor(GetWorld(), FName("Sky")));
		skyMesh->SetVisibility(false);
		
		TArray<UActorComponent*> components;
		sceneCapture2D->GetComponents(USceneCaptureComponent2D::StaticClass(), components);
		
		check(components.Num() == 2);
		auto captureComponent = CastChecked<USceneCaptureComponent2D>(components[0]);
		auto captureComponentAlpha = CastChecked<USceneCaptureComponent2D>(components[1]);
		
		captureComponent->CaptureScene();
		captureComponentAlpha->CaptureScene();

		FString iconDirectory = ContentGameDir(FString("PunCity/UI/GeneratedIcons/CustomIcon.exr"));
		ExportThumbnailHDR(GetWorld(), IconRenderTarget, iconDirectory);

		FString iconDirectoryAlpha = ContentGameDir(FString("PunCity/UI/GeneratedIcons/CustomIconAlpha.exr"));
		ExportThumbnailHDR(GetWorld(), IconRenderTargetAlpha, iconDirectoryAlpha);

		skyMesh->SetVisibility(true);
	}

	void BakeIcons(bool bakeBuilding)
	{
		auto skyMesh = PunUnrealUtils::GetComponent<UStaticMeshComponent>(PunUnrealUtils::FindWorldActor(GetWorld(), FName("Sky")));
		skyMesh->SetVisibility(false);
		
		gameManager = FindActor<AGameManager>(GetWorld());
		sceneCapture2D = FindActor<ASceneCapture2D>(GetWorld());
		PUN_LOG("sceneCapture2D %d", sceneCapture2D);

		TArray<UActorComponent*> components;
		sceneCapture2D->GetComponents(USceneCaptureComponent2D::StaticClass(), components);
		check(components.Num() == 2);

		auto captureComponent = CastChecked<USceneCaptureComponent2D>(components[0]);
		auto captureComponentAlpha = CastChecked<USceneCaptureComponent2D>(components[1]);

		UAssetLoaderComponent* assetLoader = gameManager->assetLoader();
		check(assetLoader);
		const GameDisplayInfo& displayInfo = gameManager->displayInfo();

		// Buildings
		if (bakeBuilding)
		{
			const float baseHouseCaptureDistance = 215;

			for (int i = 0; i < BuildingEnumCount; i++)
			{
				CardEnum buildingEnum = static_cast<CardEnum>(i);
				WorldTile2 buildingSize = GetBuildingInfo(buildingEnum).size;
				float usedSize = std::fmax(buildingSize.x, buildingSize.y);
				float captureDistance = baseHouseCaptureDistance * usedSize / 4.0f; // 4.0f is house size

				float heightAdjustment = -20;
				
				// Special adjustment for small buildings
				if (buildingEnum == CardEnum::GardenCypress) {
					captureDistance *= 2.0f;
					heightAdjustment = -15;
				}
				if (buildingEnum == CardEnum::Windmill) {
					captureDistance *= 1.5f;
					heightAdjustment = -40;
				}
				else if (usedSize == 1) {
					captureDistance *= 1.8f;
					heightAdjustment = -5;
				}
				
				const ModuleTransforms& modules = displayInfo.GetDisplayModules(buildingEnum, 0);
				std::vector<ModuleTransform> moduleTransforms = modules.transforms;

				// Always show toggleable transforms
				moduleTransforms.insert(moduleTransforms.end(), modules.togglableTransforms.begin(), modules.togglableTransforms.end());
				moduleTransforms.insert(moduleTransforms.end(), modules.animTransforms.begin(), modules.animTransforms.end());

				// Wind mill show mill
				if (buildingEnum == CardEnum::Windmill) {
					//moduleTransforms.push_back(modules.animTransforms[0]);
					moduleTransforms[moduleTransforms.size() - 1].transform.SetTranslation(FVector(0, 0, 60));
				}

				Direction faceDirection = Direction::S;
				switch(buildingEnum)
				{
				case CardEnum::PrintingPress:
				case CardEnum::CottonMill:
					faceDirection = Direction::W;
					break;
				default:
					break;
				}
				
				
				_buildingMeshes->Show(faceDirection, moduleTransforms, assetLoader, 0);
				_buildingMeshes->SetWorldLocation(captureComponent->GetComponentLocation() + captureComponent->GetForwardVector() * captureDistance + FVector(0, 0, heightAdjustment));

				// TODO: once UE4 fixed alpha exr/png export, use only one texture.
				captureComponent->CaptureScene();
				captureComponentAlpha->CaptureScene();

				ExportIcon(FString("PunCity/UI/GeneratedIcons/BuildingIcon"), i);

				_buildingMeshes->Hide();
			}
		}
		else
		{
			// Resources
			const float resourceCaptureDistance = 45;

			_resourceMesh->SetVisibility(true);
			for (int i = 0; i < ResourceEnumCount; i++) {
				ResourceEnum resourceEnum = static_cast<ResourceEnum>(i);
				_resourceMesh->SetStaticMesh(assetLoader->resourceHandMesh(resourceEnum));
				_resourceMesh->SetWorldLocation(captureComponent->GetComponentLocation() + captureComponent->GetForwardVector() * resourceCaptureDistance);

				captureComponent->CaptureScene();
				captureComponentAlpha->CaptureScene();

				ExportIcon(FString("PunCity/UI/ResourceIcons/ResourceIcon"), i);
			}
			_resourceMesh->SetVisibility(false);
		}

		skyMesh->SetVisibility(true);
	}

	template<typename T>
	static T* FindActor(const UObject* WorldContextObject)
	{
		TArray<AActor*> FoundActors;
		T* actor = nullptr;
		UGameplayStatics::GetAllActorsOfClass(WorldContextObject, T::StaticClass(), FoundActors);
		if (FoundActors.Num() > 0) {
			PUN_LOG("Got Actor %s %p", *FoundActors[0]->GetName(), FoundActors[0]);
			actor = CastChecked<T>(FoundActors[0]);
		} else {
			PUN_LOG("No Actor");
		}
		return actor;
	}

public:
	UPROPERTY(EditAnywhere) bool SnapBuildingIcons;
	UPROPERTY(EditAnywhere) bool SnapResourceIcons;
	UPROPERTY(EditAnywhere) bool SnapCustomIcon;

	UPROPERTY(EditAnywhere) UCanvasRenderTarget2D* IconRenderTarget;
	UPROPERTY(EditAnywhere) UCanvasRenderTarget2D* IconRenderTargetAlpha;

	UPROPERTY(EditAnywhere) UBuildingMeshesComponent* _buildingMeshes;
	UPROPERTY(EditAnywhere) UStaticMeshComponent* _resourceMesh;

	UPROPERTY(EditAnywhere) AGameManager* gameManager;
	UPROPERTY(EditAnywhere) ASceneCapture2D* sceneCapture2D;

public:	
	bool ShouldTickIfViewportsOnly() const override { return true; }
	void Tick(float DeltaSeconds) override {
		//PUN_LOG("Tick AssetIconBaker");
		if (SnapBuildingIcons) {
			BakeIcons(true);
			SnapBuildingIcons = false;
		}
		if (SnapResourceIcons) {
			BakeIcons(false);
			SnapResourceIcons = false;
		}
		if (SnapCustomIcon) {
			BakeCustomIcon();
			SnapCustomIcon = false;
		}
	}
};
