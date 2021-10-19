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
		TownManagerBase* townManagerBase = sim.townManagerBase(targetTownId);
		RelationshipModifiers& relationship = townManagerBase->relationship();
		
		// Name
		SetText(PlayerNameText, FText::Format(
			relationship.isAlly(playerId()) ? LOCTEXT("{0} (Ally)", "{0} (Ally)") : INVTEXT("{0}"),
			sim.townNameT(targetTownId)
		));

		// Relationship
		TArray<FText> args;
		relationship.GetAIRelationshipText(args, playerId());
		SetText(RelationshipText, args);

		// Interactions
		if (relationship.shouldShow_DeclareFriendship(playerId()))
		{
			bool isRed = sim.moneyCap32(playerId()) < relationship.friendshipPrice();
			ADDTEXT_LOCTEXT("Declare Friendship", "Declare Friendship");
			ADDTEXT_(INVTEXT("\n<img id=\"Coin\"/>{0}"), TextRed(TEXT_NUM(relationship.friendshipPrice()), isRed));
			InteractionBox->AddButton2Lines(JOINTEXT(args), this, CallbackEnum::DeclareFriendship, !isRed, false);
		}

		args.Empty();
		if (relationship.shouldShow_MarryOut(playerId()))
		{
			bool isRed = sim.moneyCap32(playerId()) < relationship.marryOutPrice();
			ADDTEXT_LOCTEXT("Marry out", "Marry out daughter or son");
			ADDTEXT_(INVTEXT("\n<img id=\"Coin\"/>{0}"), TextRed(TEXT_NUM(relationship.marryOutPrice()), isRed));
			InteractionBox->AddButton2Lines(JOINTEXT(args), this, CallbackEnum::MarryOut, !isRed, false);
		}

		args.Empty();
		if (!relationship.isAlly(playerId()) &&
			townManagerBase->lordPlayerId() != playerId())
		{
			bool showEnabled = relationship.CanCreateAlliance(playerId());
			ADDTEXT_LOCTEXT("Propose Alliance", "Propose Alliance");
			if (!showEnabled) {
				ADDTEXT_(LOCTEXT("Need {0} Relationship", "\nNeed {0} Relationship"), TEXT_NUM(relationship.AllyRelationshipRequirement(playerId())));
			}
			InteractionBox->AddButton2Lines(JOINTEXT(args), this, CallbackEnum::ProposeAlliance, showEnabled, false);
		}
		
		InteractionBox->AfterAdd();
	}
	
}


#undef LOCTEXT_NAMESPACE