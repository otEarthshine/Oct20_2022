// Pun Dumnernchanvanit's


#include "PlayerListElementUI.h"
#include "PunCity/PunGameMode.h"
#include "PunCity/MainMenuPlayerController.h"

void UPlayerListElementUI::OnClickPlayerKickButton()
{
	auto gameMode = UGameplayStatics::GetGameMode(this);
	if (gameMode)
	{
		auto punGameMode = CastChecked<APunGameMode>(gameMode);
		auto controller = CastChecked<AMainMenuPlayerController>(punGameMode->ServerGetConnectedControllerByName(playerName));
		controller->Client_GotKicked();
	}
}
