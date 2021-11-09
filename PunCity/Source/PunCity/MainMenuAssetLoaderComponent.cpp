// Pun Dumnernchanvanit's


#include "MainMenuAssetLoaderComponent.h"

// Sets default values for this component's properties
UMainMenuAssetLoaderComponent::UMainMenuAssetLoaderComponent()
{
	PrimaryComponentTick.bCanEverTick = true;

	/*
	 * Add Player Logos
	 */
	int32 playerLogoCount = 97;

	for (int32 i = 0; i < playerLogoCount; i++)
	{
		FString number = FString::FromInt(i + 1);
		if (number.Len() == 1) {
			number = FString("0") + number;
		}
		FString path = FString("UI/PlayerLogos/LogoTeam_") + number;

		IPlatformFile& platformFile = FPlatformFileManager::Get().GetPlatformFile();
		check(platformFile.FileExists(*(FPaths::ProjectContentDir() + path + FString(".uasset"))));

		UObject* playerLogosTextureObj = StaticLoadObject(UTexture2D::StaticClass(), NULL, *(FString("/Game/") + path));
		UTexture2D* playerLogosTexture = CastChecked<UTexture2D>(playerLogosTextureObj);

		PlayerLogos.Add(playerLogosTexture);
	}

	
	/*
	 * Add Player Characters
	 */
	TArray<FString> foundFiles;

	FString findDirectory = FPaths::ProjectContentDir() + FString("/UI/PlayerCharacters/");
	IFileManager::Get().FindFiles(foundFiles, *findDirectory, TEXT(".uasset"));

	check(foundFiles.Num() > 0);
	for (int32 i = 0; i < foundFiles.Num(); i++)
	{
		FString fileName = FPaths::GetBaseFilename(foundFiles[i]);
		
		UObject* playerCharacterTextureObj = StaticLoadObject(UTexture2D::StaticClass(), NULL, *(FString("/Game/UI/PlayerCharacters/") + fileName));
		UTexture2D* playerCharacterTexture = CastChecked<UTexture2D>(playerCharacterTextureObj);
		PlayerCharacters.Add(fileName, playerCharacterTexture);
	}

	

}



