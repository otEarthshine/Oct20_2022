#pragma once

#include "CoreMinimal.h"
#include "PunCity/NetworkStructs.h"

#include "UObject/Interface.h"
#include "GameNetworkInterface.generated.h"

// This class does not need to be modified.
UINTERFACE(MinimalAPI)
class UGameNetworkInterface : public UInterface
{
	GENERATED_BODY()
};

/**
 * 
 */
class IGameNetworkInterface
{
	GENERATED_BODY()

	// Add interface functions to this class. This is the class that will be inherited to implement this interface.
public:
	virtual int32 playerId() = 0;
	virtual const TArray<FString>& playerNamesF() = 0;
	virtual bool IsPlayerConnected(int32 playerId) = 0;

	virtual void SetTickDisabled(bool tickDisabled) = 0;

	virtual void SendNetworkCommand(std::shared_ptr<FNetworkCommand> networkCommand) = 0;

	virtual bool IsHost() = 0;
	virtual void SetGameSpeed(int32 gameSpeed) = 0;
	virtual int32 hostGameSpeed() = 0;
	virtual void Pause() = 0;
	virtual void Resume() = 0;
	virtual void GoToMainMenu() = 0;
	virtual void GoToVictoryScreen() = 0;

	// TODO: shouldn't be here, move if there is better place for this ???
	virtual bool WorldLocationToScreen(FVector WorldLocation, FVector2D& ScreenLocation) = 0;
	virtual bool ControllerGetMouseHit(struct FHitResult& hitResult, ECollisionChannel channel = ECC_WorldStatic) = 0;
	virtual bool ControllerGetHit(FHitResult& hitResult, ECollisionChannel channel, FVector2D screenPosition) = 0;

	virtual FVector2D GetMousePositionPun() const = 0;
	virtual FVector2D GetViewportSizePun() const = 0;
	
	virtual WorldAtom2 cameraAtom() = 0;
	virtual void SetCameraAtom(WorldAtom2 lookAtAtom) = 0;
	
	virtual class APunHUD* GetPunHUD() = 0;

	virtual FString playerNameF(int32 playerId) = 0;
	virtual std::string playerName(int32 playerId) = 0;

	virtual int32 serverTick() = 0;

	virtual bool IsChatFocus() = 0;
	virtual void SetFocusChat() = 0;
	virtual void SetFocusGame() = 0;
	//virtual void SetFocusGameOnly() = 0;

	virtual void SetCursor(FName name) = 0;

	virtual bool IsHoveredOnScrollUI() = 0;

	virtual PlacementType placementType() = 0;
	virtual void OnCancelPlacement() = 0;

	//virtual void SetMainGameUIActive(bool active) = 0;
	virtual void ResetBottomMenuDisplay() = 0;
	virtual void ResetGameUI() = 0;

	virtual void ShowConfirmationUI(std::string confirmationStr, std::shared_ptr<FNetworkCommand> commandIn) = 0;
	virtual bool IsShowingConfirmationUI(std::string confirmationStr) = 0;

	virtual void KeyPressed_H() = 0;
	virtual void KeyPressed_F() = 0;

	virtual TileArea GetDemolishHighlightArea() = 0;

	virtual void TrailerCityReplayUnpause() = 0;
	virtual void TrailerShipStart() = 0;
	virtual void SetLightAngle(float lightAngle) = 0;
	virtual float GetLightAngle() = 0;
	virtual float GetTrailerTime() = 0;

	virtual void ExecuteInitialCloudFade() = 0;

	virtual TArray<class UFireForgetAudioComponent*> GetPunAudios() = 0;
};
