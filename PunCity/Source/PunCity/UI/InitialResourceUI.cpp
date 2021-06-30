// Pun Dumnernchanvanit's


#include "InitialResourceUI.h"

#define LOCTEXT_NAMESPACE "InitialResourceUI"

bool UInitialResourceUI::CheckEnoughMoneyAndStorage()
{
	int32 valueIncrease = initialResources.totalCost() - FChooseInitialResources::GetDefault().totalCost();
	if (valueIncrease > simulation().moneyCap32(playerId())) {
		// Not enough money... revert the change
		initialResources = lastInitialResources;
		simulation().AddPopupToFront(playerId(),
			LOCTEXT("Not enough money", "Not enough money"),
			ExclusiveUIEnum::InitialResourceUI, "PopupCannot"
		);
		return false;
	}
	auto resourceMap = initialResources.resourceMap();
	if (StorageTilesOccupied(resourceMap) > InitialStorageSpace) {
		// Not enough space
		initialResources = lastInitialResources;
		simulation().AddPopupToFront(playerId(), 
			LOCTEXT("Not enough storage space", "Not enough storage space"),
			ExclusiveUIEnum::InitialResourceUI, "PopupCannot"
		);
		return false;
	}
	
	return true;
}

#undef LOCTEXT_NAMESPACE