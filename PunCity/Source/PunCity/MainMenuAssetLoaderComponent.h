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
	UPROPERTY() TArray<UTexture2D*> PlayerCharacters;
};
