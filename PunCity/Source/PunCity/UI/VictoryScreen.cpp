// Pun Dumnernchanvanit's


#include "VictoryScreen.h"

#define LOCTEXT_NAMESPACE "VictoryScreen"

void UVictoryScreen::Tick()
{
	auto gameInstance = CastChecked<UPunGameInstance>(GetGameInstance());
	FGameEndStatus endStatus = gameInstance->endStatus;
	if (endStatus.playerId == endStatus.victoriousPlayerId)
	{
		if (endStatus.gameEndEnum == GameEndEnum::DominationVictory) {
			SetText(VictoryText, LOCTEXT("Domination Victory", "Domination Victory"));
		}
		else if (endStatus.gameEndEnum == GameEndEnum::EconomicVictory) {
			SetText(VictoryText, LOCTEXT("Economic Victory", "Economic Victory"));
		}
		else if (endStatus.gameEndEnum == GameEndEnum::ScienceVictory) {
			SetText(VictoryText, LOCTEXT("Science Victory", "Science Victory"));
		}
		VictoryBackgroundColor->SetVisibility(ESlateVisibility::Visible);
		DefeatBackgroundColor->SetVisibility(ESlateVisibility::Collapsed);
	}
	else
	{
		VictoryText->SetText(FText::Format(LOCTEXT("XisVictorious",
			"{0} is Victorious"),
			FText::FromString(gameInstance->playerNameF(endStatus.victoriousPlayerId))
		));
		VictoryBackgroundColor->SetVisibility(ESlateVisibility::Collapsed);
		DefeatBackgroundColor->SetVisibility(ESlateVisibility::Visible);
	}
}

#undef LOCTEXT_NAMESPACE