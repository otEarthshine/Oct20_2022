// Pun Dumnernchanvanit's


#include "DiplomacyUI.h"

#define LOCTEXT_NAMESPACE "DiplomacyUI"

void UDiplomacyUI::TickUI()
{
	LEAN_PROFILING_UI(TickDiplomacyUI);

	if (!IsVisible() || targetTownId == -1) {
		return;
	}
	
	auto& sim = simulation();
	

	/*
	 * AI Town
	 */
	if (sim.IsAITown(targetTownId))
	{
		// Name
		SetText(PlayerNameText, sim.townNameT(targetTownId));

		// Relationship
		TownManagerBase* townManagerBase = sim.townManagerBase(targetTownId);
		TArray<FText> args;
		townManagerBase->relationship().GetAIRelationshipText(args, playerId());
		SetText(RelationshipText, args);

		// Interactions
		if (townManagerBase->shouldShow_DeclareFriendship(playerId()))
		{
			bool isRed = sim.moneyCap32(playerId()) < townManagerBase->friendshipPrice();
			ADDTEXT_LOCTEXT("Declare Friendship", "Declare Friendship");
			ADDTEXT_(INVTEXT("\n<img id=\"Coin\"/>{0}"), TextRed(TEXT_NUM(townManagerBase->friendshipPrice()), isRed));
			InteractionBox->AddButton2Lines(JOINTEXT(args), this, CallbackEnum::DeclareFriendship, !isRed, false);
		}

		args.Empty();
		if (townManagerBase->shouldShow_MarryOut(playerId()))
		{
			bool isRed = sim.moneyCap32(playerId()) < townManagerBase->marryOutPrice();
			ADDTEXT_LOCTEXT("Marry out", "Marry out daughter or son");
			ADDTEXT_(INVTEXT("\n<img id=\"Coin\"/>{0}"), TextRed(TEXT_NUM(townManagerBase->marryOutPrice()), isRed));
			InteractionBox->AddButton2Lines(JOINTEXT(args), this, CallbackEnum::MarryOut, !isRed, false);
		}
		
		InteractionBox->AfterAdd();
	}
	
}


#undef LOCTEXT_NAMESPACE