// Pun Dumnernchanvanit's

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "MainMenuAssetLoaderComponent.generated.h"


UCLASS( ClassGroup=(Custom), meta=(BlueprintSpawnableComponent) )
class PROTOTYPECITY_API UMainMenuAssetLoaderComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UMainMenuAssetLoaderComponent();
	
	UPROPERTY() TArray<UTexture2D*> PlayerLogos;
	
	UPROPERTY() TMap<FString, UTexture2D*> PlayerCharacters;

	UTexture2D* GetFactionImage(const FString& name) {
		if (FactionImages.Contains(name)) {
			return FactionImages[name];
		}
		return nullptr;
	}

	UTexture2D* GetPlayerCharacter(const FString& name) {
		return PlayerCharacters[name];
	}

private:
	UPROPERTY() TMap<FString, UTexture2D*> FactionImages;
	
};
