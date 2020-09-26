// Pun Dumnernchanvanit's


#include "PunBasePlayerController.h"
#include "Kismet/GameplayStatics.h"
#include "PunCity/PunUtils.h"
#include "PunCity/PunGameInstance.h"
#include "PunCity/PunGameMode.h"
#include "PunCity/UI/PunLobbyHUD.h"
#include "PunCity/UI/LobbyUI.h"
#include "UI/PunMainMenuHUD.h"


void APunBasePlayerController::OnLogout()
{
	PUN_DEBUG2("OnLogout %s", *PlayerState->GetPlayerName());
	_LOG(LogNetworkInput, "OnLogout %s", *PlayerState->GetPlayerName());


}

void APunBasePlayerController::SetClientId_Implementation(int32 clientPlayerIdIn) {
	_controllerPlayerId = clientPlayerIdIn;
}

void APunBasePlayerController::SyncPlayersState_ToClient_Implementation(const TArray<FString>& playerNamesF,
																const TArray<bool>& playerReadyStates, 
																const TArray<bool>& playerConnectedStates,
																const TArray<int32>& clientDataReceived,
																int32 hostPlayerIdIn, int32 serverTick)
{
	auto hud = GetHUD();
	
	gameInstance()->DebugPunSync("SyncPlayersState Before: ");

	auto mainMenuHUD = Cast<APunMainMenuHUD>(hud);

	// Sync State if this isn't server. Also don't sync if this is in main menu (leftover queued sync when hitting GoToMainMenu)
	if (!IsServer() && !mainMenuHUD)
	{
		gameInstance()->SetPlayerNamesF(playerNamesF);
		gameInstance()->SetPlayerReadyStates(playerReadyStates);
		gameInstance()->playerConnectedStates = playerConnectedStates;
		gameInstance()->clientPacketsReceived = clientDataReceived;
		gameInstance()->hostPlayerId = hostPlayerIdIn;
		gameInstance()->SetServerTick(serverTick);
	}

	gameInstance()->DebugPunSync("SyncPlayersState After: ");

	auto lobbyHUD = Cast<APunLobbyHUD>(hud);
	if (lobbyHUD) {
		lobbyHUD->lobbyUI()->UpdateLobbyUI();
	}
}

void APunBasePlayerController::TryChangePlayerId_ToServer_Implementation(int32 targetPlayerId)
{
	int32 originalPlayerId = controllerPlayerId();
	bool succeed =  gameInstance()->TryChangePlayerId(originalPlayerId, targetPlayerId);
	
	if (succeed)
	{
		SetControllerPlayerId(targetPlayerId);

		// If the player is host, change host playerId too
		if (originalPlayerId == gameInstance()->hostPlayerId) {
			gameInstance()->hostPlayerId = targetPlayerId;
		}

		auto gameMode = CastChecked<APunGameMode>(UGameplayStatics::GetGameMode(this));
		gameMode->Server_SyncPlayerStateToAllControllers();
	}
}

void APunBasePlayerController::SendSaveInfo_ToClient_Implementation(const TArray<uint8>& saveInfoData)
{
	FMemoryReader LoadArchive(saveInfoData, true);
	LoadArchive.Seek(0);
	LoadArchive.SetIsSaving(false);
	LoadArchive.SetIsLoading(true);

	GameSaveInfo saveInfo;
	saveInfo.Serialize(LoadArchive);

	_LOG(PunNetwork, "Sync - SendSaveInfo_ToClient %s", *saveInfo.name);

	gameInstance()->SetSavedGameToLoad(saveInfo);
	gameInstance()->saveSystem().SetSyncSaveInfo(saveInfo);

	_packetsRequested = 0;
}

void APunBasePlayerController::Tick(float DeltaTime)
{
	_dataSyncTick++;
	if (_dataSyncTick % 5 != 0) {
		return;
	}

	/*
	 * Lobby Load Data Transfer
	 */
	auto& saveSys = gameInstance()->saveSystem();
	
	if (!IsServer() && saveSys.NeedSyncData())
	{
		_dataSendTime += DeltaTime; // Tick by fixed delta time to prevent buffer overflow
		const float fixedDeltaTime = 0.02;
		
		if (_dataSendTime >= fixedDeltaTime &&
			controllerPlayerId() < gameInstance()->clientPacketsReceived.Num() &&
			gameInstance()->clientPacketsReceived[controllerPlayerId()] <= saveSys.totalPackets())
		{
			_dataSendTime -= fixedDeltaTime;
			
			int32 receivedPacketsCount = saveSys.receivedPacketCount();

			//PUN_DEBUG2("Tick %d received:%d", packets.Num(), gameInstance()->clientPacketsReceived[controllerPlayerId()]);

			_LOG(PunNetwork, "Sync - Controller Tick %d/%d", receivedPacketsCount, _packetsRequested);
			
			// Don't overflow the buffer
			const int32 packetsPerRequest = 40;
			const int32 packetCountThreshold = 80;
			if (_packetsRequested - receivedPacketsCount <= packetCountThreshold)
			{
				TArray<int32> packets = gameInstance()->saveSystem().GetSyncPacketIndices(packetsPerRequest);
				_packetsRequested += packets.Num();
				RequestDataChunks_ToServer(receivedPacketsCount, packets);

				//PUN_DEBUG2("Requested %d received:%d", _packetsRequested, receivedPacketsCount);
			}
			else {
				PUN_DEBUG2("Pause request %d received:%d", receivedPacketsCount, receivedPacketsCount);
			}
		}
	}


	// !!! Don't try this ResetPlayerCount() on Tick... It breaks...
	//if (IsServer() && !IsLobbyUIOpened()) {
	//	gameInstance()->ResetPlayerCount();
	//}
}

// TODO: since we can't use unrealiable, Simplify this?
void APunBasePlayerController::RequestDataChunks_ToServer_Implementation(int32 clientPacketsReceived, const TArray<int32>& packetIndices)
{
	//if (ShouldSkipLobbyNetworkCommand()) {
	//	return;
	//}
	
	//PUN_DEBUG2("RequestDataChunks %d, %d", clientPacketsReceived, packetIndices.Num());

	auto gameInst = gameInstance();

	if (controllerPlayerId() >= gameInst->clientPacketsReceived.Num()) {
		return;
	}
	
	gameInst->clientPacketsReceived[controllerPlayerId()] = clientPacketsReceived;
	
	int32 lastPacketIndex = gameInst->saveSystem().totalPackets() - 1;
	const TArray<uint8>& sourceData = gameInst->saveSystem().GetSyncCompressedData();

	_LOG(PunNetwork, "Sync - RequestDataChunks_ToServer last:%d packIndicesCount:%d", lastPacketIndex, packetIndices.Num());
	
	for (int32 packetIndex : packetIndices)
	{
		int32 packetSize = MaxPacketSize;

		if (packetIndex == lastPacketIndex) {
			packetSize = gameInst->saveSystem().lastPacketSize();
		}

		_LOG(PunNetwork, "Sync - RequestDataChunks_ToServer packetSize:%d sourceData:%d", packetSize, sourceData.Num());

		TArray<uint8> dataChunk;
		dataChunk.Reserve(packetSize);
		for (int32 i = 0; i < packetSize; i++) {
			dataChunk.Add(sourceData[packetIndex * MaxPacketSize + i]);
		}

		SendSaveDataChunk_ToClient(packetIndex, dataChunk);
	}

	auto gameMode = CastChecked<APunGameMode>(UGameplayStatics::GetGameMode(this));
	gameMode->Server_SyncPlayerStateToAllControllers();
}
void APunBasePlayerController::SendSaveDataChunk_ToClient_Implementation(int32 packetIndex, const TArray<uint8>& saveDataChunk)
{
	_LOG(PunNetwork, "Sync - SendSaveDataChunk_ToClient packetIndex:%d saveDataChunk:%d", packetIndex, saveDataChunk.Num());
	//if (ShouldSkipLobbyNetworkCommand()) {
	//	return;
	//}
	
	//PUN_DEBUG2("Client Get Data[%d] %d", packetIndex, saveDataChunk.Num());
	gameInstance()->saveSystem().ReceivePacket(packetIndex, saveDataChunk);
}



void APunBasePlayerController::PunSet(const FString& settingName, int32 value)
{
	PUN_LOG("PunSet: %s, %d", *settingName, value);
	PunSettings::Settings[string(TCHAR_TO_UTF8(*settingName))] = value;
}

void APunBasePlayerController::PunTog(const FString& setting)
{
	PUN_LOG("PunToggle: %s", *setting);

	bool value = PunSettings::Get(setting) == 0;
	PunSettings::Set(setting, value);
}