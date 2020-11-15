// Pun Dumnernchanvanit's

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameModeBase.h"
#include "PunBasePlayerController.h"
#include "PunGameMode.generated.h"

/**
 * 
 */
UCLASS()
class PROTOTYPECITY_API APunGameMode : public AGameModeBase
{
	GENERATED_BODY()
public:
	/*
	 * Overrides
	 */
	void PreLogin(const FString& Options, const FString& Address, const FUniqueNetIdRepl& UniqueId, FString& ErrorMessage) override;
	
	APlayerController* Login(UPlayer* NewPlayer, ENetRole InRemoteRole, const FString& Portal, const FString& Options, const FUniqueNetIdRepl& UniqueId, FString& ErrorMessage) override;

	/** Called after a successful login.  This is the first place it is safe to call replicated functions on the PlayerController. */
	void PostLogin(APlayerController* NewPlayer) override;

	/** Called when a Controller with a PlayerState leaves the game or is destroyed */
	void Logout(AController* Exiting) override;

public:
	/*
	 * 
	 */
	void Server_SyncPlayerStateToAllControllers();

	
public:
	/*
	 * Get
	 */
	int32 connectedPlayerCount() {
		return _ConnectedControllers.Num();
	}

	APunBasePlayerController* ServerGetConnectedControllerByName(const FString& name)
	{
		for (APunBasePlayerController* controller: _ConnectedControllers) {
			if (controller->PlayerState->GetPlayerName() == name) {
				return controller;
			}
		}
		return nullptr;
	}


	const TArray<APunBasePlayerController*>& ConnectedControllers() { return _ConnectedControllers; }
	
	/*
	 * Set
	 */

	
private:
	// Only on server's main controller
	// Note this is not in playerId's order, use ServerGetConnectedControllerByName() to get a specific controller
	UPROPERTY() TArray<APunBasePlayerController*> _ConnectedControllers;
	
	//UPROPERTY() TArray<FString> _playerNames; // Empty player name means the slot is empty
	//UPROPERTY() TArray<bool> _playerConnectedStates;
};
