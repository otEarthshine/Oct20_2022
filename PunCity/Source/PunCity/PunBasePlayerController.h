// Pun Dumnernchanvanit's

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/PlayerController.h"
#include "PunCity/PunGameInstance.h"
#include "PunBasePlayerController.generated.h"


/**
 * 
 */
UCLASS()
class PROTOTYPECITY_API APunBasePlayerController : public APlayerController
{
	GENERATED_BODY()
public:
	
	void Tick(float DeltaTime) override;

	virtual void OnLogout();

	// Controller PlayerId
	// - Useful in determining what playerId the message is from
	int32 controllerPlayerId() { return _controllerPlayerId; }
	
	void SetControllerPlayerId(int32 playerId) {
		_controllerPlayerId = playerId;
		SetClientId(playerId);
	}


	UFUNCTION(Exec) void PrintSessionInfo() {
		gameInstance()->PrintSessionInfo();
	}
	UFUNCTION(Exec) void PrintPlayers() {
		GEngine->bEnableOnScreenDebugMessages = true;
		gameInstance()->PrintPlayers();
#if !WITH_EDITOR
		GEngine->bEnableOnScreenDebugMessages = false;
#endif
	}

	/*
	 * Setup Client
	 */
	UFUNCTION(Reliable, Client) void SetClientId(int32 clientPlayerIdIn);

	/*
	 * Sync player state
	 */
	// Call Server_SyncPlayerStateToAllControllers to sync to all controllers
	UFUNCTION(Reliable, Client) void SyncPlayersState_ToClient(const TArray<FString>& playerNamesF,
															const TArray<bool>& playerReadyStates,
															const TArray<bool>& playerConnectedStates,
															const TArray<int32>& clientDataReceived,
															int32 hostPlayerIdIn, int32 serverTick);


	UFUNCTION(Reliable, Server) void TryChangePlayerId_ToServer(int32 targetPlayerId);
	


	bool IsServer() { return UGameplayStatics::GetGameMode(this) != nullptr; }

	virtual bool IsMainMenuController() { return false; }

	virtual bool IsLobbyUIOpened() { return false; }

	bool ShouldSkipLobbyNetworkCommand() {
		_LOG(PunNetwork, "Sync - ShouldSkipLobbyNetworkCommand uiOpened:%d", IsLobbyUIOpened());
		return !IsLobbyUIOpened();
	}
	
	/*
	 * Loading
	 */
	UFUNCTION(Reliable, Client) void SendSaveInfo_ToClient(const TArray<uint8>& saveInfoData);

	UFUNCTION(Reliable, Server) void RequestDataChunks_ToServer(int32 clientPacketsReceived, const TArray<int32>& packetIndices);
	UFUNCTION(Reliable, Client) void SendSaveDataChunk_ToClient(int32 packetIndex, const TArray<uint8>& saveDataChunk);

	/*
	 * Commands
	 */
	UFUNCTION(Exec) void PunSet(const FString& settingName, int32 value);
	UFUNCTION(Exec) void PunTog(const FString& settingName);



	// Replays
	UFUNCTION(Exec) void ReplayLoad(const FString& replayFileName) {
		gameInstance()->replayFilesToLoad.Add(replayFileName);
	}
	UFUNCTION(Exec) void ReplayPrint() {
		TArray<FString> files = gameInstance()->replayFilesToLoad;
		PUN_DEBUG2("Replays[%d]:", files.Num());
		for (FString file : files) {
			PUN_DEBUG2(" %s", *file);
		}
	}
	UFUNCTION(Exec) void ReplayClear() {
		gameInstance()->replayFilesToLoad.Empty();
	}


	
public:
	UPunGameInstance* gameInstance() { return Cast<UPunGameInstance>(GetGameInstance()); }

	//bool isExitingToMainMenu = false;

private:
	int32 _controllerPlayerId = -1;


	int32 _dataSyncTick = 0;
	int32 _packetsRequested = 0;

	float _dataSendTime = 0;
};
