// Fill out your copyright notice in the Description page of Project Settings.


#include "AssetIconBaker.h"
#include "ImageUtils.h"
#include "PunCity/PunUtils.h"
#include "Logging/MessageLog.h"
#include "Engine/TextureRenderTarget2D.h"
#include "HAL/FileManager.h"
#include "Serialization/BufferArchive.h"
#include "UObject/ConstructorHelpers.h"

// Sets default values
AAssetIconBaker::AAssetIconBaker()
{
	PrimaryActorTick.bCanEverTick = true;
	PrimaryActorTick.bStartWithTickEnabled = true;
	bReplicates = false;

	{
		ConstructorHelpers::FObjectFinder<UCanvasRenderTarget2D> objectFinder(TEXT("/Game/UI/GeneratedIcons/ThumbnailSnapshotTarget"));
		check(objectFinder.Succeeded());
		IconRenderTarget = objectFinder.Object;
	}

	{
		ConstructorHelpers::FObjectFinder<UCanvasRenderTarget2D> objectFinder(TEXT("/Game/UI/GeneratedIcons/ThumbnailSnapshotTargetAlpha"));
		check(objectFinder.Succeeded());
		IconRenderTargetAlpha = objectFinder.Object;
	}
	
	_buildingMeshes = CreateDefaultSubobject<UBuildingMeshesComponent>("BuildingMeshesForIcon");
	_resourceMesh = CreateDefaultSubobject<UStaticMeshComponent>("ResourceMeshForIcon");
}

FString AAssetIconBaker::SaveGameDir(const FString& SaveGameName)
{
	return FPaths::ProjectSavedDir() + TEXT("SaveGames/") + SaveGameName + TEXT(".png");
}

FString AAssetIconBaker::ContentGameDir(const FString& ContentName)
{
	return FPaths::ProjectContentDir() + ContentName;
}

UTexture2D* AAssetIconBaker::ImportSaveThumbnail(UObject* WorldContextObject, const FString& ImportPath)
{
	//Suppress warning messages when we dont have a thumb yet.
	if (FPaths::FileExists(ImportPath)) {
		return FImageUtils::ImportFileAsTexture2D(ImportPath);
	}
	return nullptr;
}

void AAssetIconBaker::ExportSaveThumbnailRT(UObject* WorldContextObject, UTextureRenderTarget2D* TextureRenderTarget, const FString& ExportPath)
{
	FText PathError;
	FPaths::ValidatePath(ExportPath, &PathError);

	if (!TextureRenderTarget)
	{
		PUN_LOG("ExportSaveThumbnailRT_InvalidTextureRenderTarget", "ExportRenderTarget: TextureRenderTarget must be non-null.");
	}
	else if (!TextureRenderTarget->Resource)
	{
		PUN_LOG("ExportSaveThumbnailRT_ReleasedTextureRenderTarget", "ExportRenderTarget: render target has been released.");
	}
	else if (!PathError.IsEmpty())
	{
		PUN_LOG("ExportSaveThumbnailRT_InvalidFilePath", "ExportRenderTarget: Invalid file path provided: '{0}'");
	}
	else if (ExportPath.IsEmpty())
	{
		PUN_LOG("ExportSaveThumbnailRT_InvalidFileName", "ExportRenderTarget: FileName must be non-empty.");
	}
	else
	{
		FArchive* Ar = IFileManager::Get().CreateFileWriter(*ExportPath);

		if (Ar)
		{
			FBufferArchive Buffer;

			bool bSuccess = FImageUtils::ExportRenderTarget2DAsPNG(TextureRenderTarget, Buffer);

			PUN_LOG("Export png %d format: %d", bSuccess, TextureRenderTarget->GetFormat());
			if (bSuccess) {
				Ar->Serialize(const_cast<uint8*>(Buffer.GetData()), Buffer.Num());
			}

			delete Ar;
		}
		else
		{
			PUN_LOG("ExportSaveThumbnailRT_FileWriterFailedToCreate", "ExportRenderTarget: FileWrite failed to create.");
		}
	}
}

void AAssetIconBaker::ExportThumbnailHDR(UObject* WorldContextObject, UTextureRenderTarget2D* TextureRenderTarget, const FString& ExportPath)
{
	FText PathError;
	FPaths::ValidatePath(ExportPath, &PathError);

	if (!TextureRenderTarget) {
		PUN_LOG("ExportSaveThumbnailRT_InvalidTextureRenderTarget", "ExportRenderTarget: TextureRenderTarget must be non-null.");
	}
	else if (!TextureRenderTarget->Resource) {
		PUN_LOG("ExportSaveThumbnailRT_ReleasedTextureRenderTarget", "ExportRenderTarget: render target has been released.");
	}
	else if (!PathError.IsEmpty()) {
		PUN_LOG("ExportSaveThumbnailRT_InvalidFilePath", "ExportRenderTarget: Invalid file path provided: '{0}'");
	}
	else if (ExportPath.IsEmpty()) {
		PUN_LOG("ExportSaveThumbnailRT_InvalidFileName", "ExportRenderTarget: FileName must be non-empty.");
	}
	else
	{
		FArchive* Ar = IFileManager::Get().CreateFileWriter(*ExportPath);

		if (Ar)
		{
			FBufferArchive Buffer;

			bool bSuccess = FImageUtils::ExportRenderTarget2DAsEXR(TextureRenderTarget, Buffer);

			PUN_LOG("Export png %d format: %d", bSuccess, TextureRenderTarget->GetFormat());
			if (bSuccess) {
				Ar->Serialize(const_cast<uint8*>(Buffer.GetData()), Buffer.Num());
			}

			delete Ar;
		}
		else
		{
			PUN_LOG("ExportSaveThumbnailRT_FileWriterFailedToCreate", "ExportRenderTarget: FileWrite failed to create.");
		}
	}
}

void AAssetIconBaker::DeleteSaveThumbnail(UObject* WorldContextObject, const FString& SaveGameName)
{
	FString SaveFile = SaveGameDir(SaveGameName);
	IFileManager::Get().Delete(*SaveFile, true, false, true);
}