// Pun Dumnernchanvanit's


#include "ProsperityUI.h"


void UProsperityUI::CallBack1(UPunWidget* punWidgetCaller, CallbackEnum callBackEnum)
{
	UTechBoxUI* techBox = CastChecked<UTechBoxUI>(punWidgetCaller);

	// Make sure this tech is not locked or researched
	auto unlockSys = simulation().unlockSystem(playerId());
	if (unlockSys->IsLocked(techBox->techEnum) ||
		unlockSys->IsResearched(techBox->techEnum))
	{
		dataSource()->Spawn2DSound("UI", "UIIncrementalError");
		return;
	}

	// Disallow clicking with requirements not met
	if (!unlockSys->IsRequirementMetForTech(techBox->techEnum))
	{
		simulation().AddPopupToFront(playerId(),
			unlockSys->GetTechRequirementPopupText(techBox->techEnum),
			ExclusiveUIEnum::TechTreeUI, "PopupCannot"
		);
		return;
	}


	PUN_LOG("Chose Research: %d", (int)techBox->techEnum);

	auto command = make_shared<FChooseResearch>();
	command->techEnum = techBox->techEnum;
	networkInterface()->SendNetworkCommand(command);

	// Play Sound
	dataSource()->Spawn2DSound("UI", "ResearchInitiated");
}
