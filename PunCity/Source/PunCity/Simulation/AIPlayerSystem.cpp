// Pun Dumnernchanvanit's


#include "AIPlayerSystem.h"

#define LOCTEXT_NAMESPACE "AIPlayerSystem"

void AIPlayerSystem::GetAIRelationshipText(TArray<FText>& args, int32 playerId)
{
	ADDTEXT_(LOCTEXT("Overall: ", "Overall: {0}\n"), FText::AsNumber(GetTotalRelationship(playerId)));

	const std::vector<int32>& modifiers = _relationshipModifiers[playerId];
	for (int32 i = 0; i < modifiers.size(); i++) {
		if (modifiers[i] != 0) {
			ADDTEXT_(INVTEXT("{0} {1}\n"), TEXT_NUMSIGNED(modifiers[i]), RelationshipModifierNameInt(i));
		}
	}
}


#undef LOCTEXT_NAMESPACE