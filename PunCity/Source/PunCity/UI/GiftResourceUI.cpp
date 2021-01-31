// Pun Dumnernchanvanit's


#include "GiftResourceUI.h"

#define LOCTEXT_NAMESPACE "GiftResourceUI"

void UGiftResourceUI::OnClickConfirmButton()
{
	if (targetPlayerId != -1)
	{
		FString resourceName = GiftTypeDropdown->GetSelectedOption();
		ResourceEnum resourceEnum;
		int32 amount = GiftTargetAmount->amount;

		if (resourceName == MoneyText.ToString()) {
			resourceEnum = ResourceEnum::Money;
			if (amount > simulation().money(playerId())) {
				simulation().AddPopupToFront(playerId(),
					LOCTEXT("Not enough money to give.", "Not enough money to give."),
					ExclusiveUIEnum::GiftResourceUI, "PopupCannot"
				);
				return;
			}
		}
		else {
			resourceEnum = FindResourceEnumByName(ToWString(resourceName));
			if (amount > simulation().resourceCount(playerId(), resourceEnum)) {
				simulation().AddPopupToFront(playerId(), 
					LOCTEXT("Not enough resource to give.", "Not enough resource to give."),
					ExclusiveUIEnum::GiftResourceUI, "PopupCannot"
				);
				return;
			}
		}

		auto command = make_shared<FGenericCommand>();
		command->genericCommandType = FGenericCommand::Type::SendGift;
		command->intVar1 = static_cast<int>(targetPlayerId);
		command->intVar2 = static_cast<int>(resourceEnum);
		command->intVar3 = static_cast<int>(amount);

		networkInterface()->SendNetworkCommand(command);
	}

	SetVisibility(ESlateVisibility::Collapsed);
}

#undef LOCTEXT_NAMESPACE