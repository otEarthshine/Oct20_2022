// Pun Dumnernchanvanit's

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/HUD.h"
#include "Blueprint/UserWidget.h"
#include "PunTestHUD.generated.h"

/**
 * 
 */
UCLASS()
class PROTOTYPECITY_API APunTestHUD : public AHUD
{
	GENERATED_BODY()
public:
	APunTestHUD();
	void BeginPlay() override;

private:
	UPROPERTY() TSubclassOf<UUserWidget> _mainMenuClass;
	UPROPERTY() class UMainMenuUI* _mainMenuUI;
};
