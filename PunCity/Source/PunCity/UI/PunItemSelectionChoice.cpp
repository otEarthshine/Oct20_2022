// Pun Dumnernchanvanit's


#include "PunItemSelectionChoice.h"

using namespace std;

void UPunItemSelectionChoice::PunInit()
{
	ItemSelectButton->OnClicked.AddDynamic(this, &UPunItemSelectionChoice::ClickItemSelect);
}

void UPunItemSelectionChoice::ClickItemSelect()
{
	//// TODO: networking..
	//auto command = make_shared<FSelectItem>();
	//command->itemEnum = itemEnum;
	//networkInterface()->SendNetworkCommand(command);

	//simulation().itemSystem(playerId())->waitingCommandArrival = true;
	//
	//parentWidget->SetVisibility(ESlateVisibility::Collapsed);
}