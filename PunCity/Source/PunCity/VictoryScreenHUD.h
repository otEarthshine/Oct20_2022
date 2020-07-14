// Pun Dumnernchanvanit's

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/HUD.h"
#include "Blueprint/UserWidget.h"
#include "PunCity/UI/VictoryScreen.h"
#include "VictoryScreenHUD.generated.h"

/**
 * 
 */
UCLASS()
class PROTOTYPECITY_API AVictoryScreenHUD : public AHUD
{
	GENERATED_BODY()
public:
	AVictoryScreenHUD();
	void BeginPlay() override;

	//void PostInitializeComponents() override;

	void Tick(float DeltaTime) override;

private:
	TSubclassOf<UUserWidget> _victoryScreenClass;

	UPROPERTY() UVictoryScreen* _victoryScreen;
};
