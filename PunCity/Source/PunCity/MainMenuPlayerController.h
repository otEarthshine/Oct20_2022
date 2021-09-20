// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "UnrealEngine.h"
#include "GameFramework/PlayerController.h"
#include "CameraPawn.h"
#include "PunCity/DisplaySystem/MainMenuDisplayManager.h"
#include "PunCity/Simulation/MainMenuDisplaySaveSystem.h"
#include "PunBasePlayerController.h"

#include "MainMenuPlayerController.generated.h"

/**
 * 
 */
UCLASS()
class AMainMenuPlayerController : public APunBasePlayerController
{
	GENERATED_BODY()
public:
	AMainMenuPlayerController();
	void BeginPlay() override;

	void Tick(float DeltaTime) override;

	void SetupCameraPawn();
	void SetupDisplayManager();

	// TODO: another camerapawn for this
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Camera Pawn") ACameraPawn* cameraPawn;
	UPROPERTY(EditAnywhere) AMainMenuDisplayManager* mainMenuDisplayManager;

	MainMenuCameraState cameraState;

	int32 initStage;

	bool IsMainMenuController() override { return true; }

	bool IsLobbyUIOpened() override;

public:

	
public:
	bool isStartingGame;

	UFUNCTION(Reliable, Server) void SendReadyStatus_ToServer(bool playerReadyState);
	UFUNCTION(Reliable, Server) void SendPlayerInfo_ToServer(const FPlayerInfo& playerInfo);

	UFUNCTION(Reliable, Client, WithValidation) void SetMapSettings(const TArray<int32>& mapSettingsBlob);

	
	// Send clients the mapSeed, and make client display GameStartBlocker
	UFUNCTION(Reliable, Client) void ServerStartGame(const TArray<int32>& mapSettingsBlob);


	UFUNCTION(Reliable, Server) void SendChat_ToServer(const FString& playerName, const FString& message);
	UFUNCTION(Reliable, Client) void SendChat_ToClient(const FString& playerName, const FString& message);

	
	UFUNCTION(Reliable, Client, WithValidation) void Client_GotKicked();
	


	UFUNCTION(Exec) void SetCameraTranslation(float X, float Y)
	{
		FTransform transform = cameraState.worldTransform;
		transform.SetTranslation(transform.GetTranslation() + FVector(X, Y, 0));
		cameraPawn->CameraComponent->SetComponentToWorld(transform);
	}

	UFUNCTION(Exec) void ToggleMainMenu();
	

	template<typename Func>
	void ExecuteAllMainMenuControllers(Func func) {
		TArray<AActor*> found;
		UGameplayStatics::GetAllActorsOfClass(GetWorld(), APlayerController::StaticClass(), found);

		for (int i = 0; i < found.Num(); i++) {
			AMainMenuPlayerController* mainMenuController = Cast<AMainMenuPlayerController>(found[i]);
			if (mainMenuController) {
				func(mainMenuController);
			}
		}
	}
	
	
private:
	int32 chatIndex = 0;
};
