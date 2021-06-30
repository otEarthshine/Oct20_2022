// Pun Dumnernchanvanit's


#include "DiplomacyUI.h"

#define LOCTEXT_NAMESPACE "DiplomacyUI"

void UDiplomacyUI::TickUI()
{
	LEAN_PROFILING_UI(TickDiplomacyUI);
	
	auto& sim = simulation();

	if (IsVisible() &&
		aiPlayerId != -1 &&
		sim.IsAIPlayer(aiPlayerId))
	{
		// Name
		SetText(PlayerNameText, sim.playerName(aiPlayerId));

		// Relationship
		auto& aiPlayerSys = sim.aiPlayerSystem(aiPlayerId);
		TArray<FText> args;
		aiPlayerSys.GetAIRelationshipText(args, playerId());
		SetText(RelationshipText, args);

		// Interactions
		if (aiPlayerSys.shouldShow_DeclareFriendship(playerId()))
		{
			bool isRed = sim.moneyCap32(playerId()) < aiPlayerSys.friendshipPrice();
			ADDTEXT_LOCTEXT("Declare Friendship", "Declare Friendship");
			ADDTEXT_(INVTEXT("\n<img id=\"Coin\"/>{0}"), TextRed(TEXT_NUM(aiPlayerSys.friendshipPrice()), isRed));
			InteractionBox->AddButton2Lines(JOINTEXT(args), this, CallbackEnum::DeclareFriendship, !isRed, false);
		}

		args.Empty();
		if (aiPlayerSys.shouldShow_MarryOut(playerId()))
		{
			bool isRed = sim.moneyCap32(playerId()) < aiPlayerSys.marryOutPrice();
			ADDTEXT_LOCTEXT("Marry out", "Marry out daughter or son");
			ADDTEXT_(INVTEXT("\n<img id=\"Coin\"/>{0}"), TextRed(TEXT_NUM(aiPlayerSys.marryOutPrice()), isRed));
			InteractionBox->AddButton2Lines(JOINTEXT(args), this, CallbackEnum::MarryOut, !isRed, false);
		}
		InteractionBox->AfterAdd();
	}
}


#undef LOCTEXT_NAMESPACE