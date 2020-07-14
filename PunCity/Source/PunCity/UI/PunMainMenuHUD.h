// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/HUD.h"
#include "Blueprint/UserWidget.h"
#include "UObject/ConstructorHelpers.h"
#include "PunHUD.h"

#include "PunMainMenuHUD.generated.h"
/**
 * 
 */
UCLASS()
class APunMainMenuHUD : public APunHUD
{
	GENERATED_BODY()
public:
	APunMainMenuHUD();
	void BeginPlay() override;

	void Tick(float DeltaSeconds) override;

	template<typename T>
	T* LoadF(FString path)
	{
		ConstructorHelpers::FObjectFinder<T> objectFinder(*path);
		check(objectFinder.Succeeded());
		return objectFinder.Object;
	}

	UPROPERTY() TSubclassOf<UUserWidget> LoadingScreenClass;

	class UMainMenuUI* mainMenuUI() { return _mainMenuUI; }
	
private:
	UPROPERTY() USoundBase* _menuSound;
	
	UPROPERTY() TSubclassOf<UUserWidget> _mainMenuClass;
	UPROPERTY() UMainMenuUI* _mainMenuUI;
};
