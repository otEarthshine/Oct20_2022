// Pun Dumnernchanvanit's

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/HUD.h"
#include "Blueprint/UserWidget.h"
#include "PunLoadingScreenHUD.generated.h"

/**
 * 
 */
UCLASS()
class PROTOTYPECITY_API APunLoadingScreenHUD : public AHUD
{
	GENERATED_BODY()
public:
	APunLoadingScreenHUD();
	void BeginPlay() override;

private:
	UPROPERTY() TSubclassOf<UUserWidget> _uiClass;
	UPROPERTY() class UMainMenuUI* _loadingScreenUI;
};
