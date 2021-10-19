// Pun Dumnernchanvanit's


#include "MainTechTreeUI.h"

void UMainTechTreeUI::SetupTechBoxUIs()
{
	UnlockSystem* unlockSys = simulation().unlockSystem(playerId());
	const std::vector<std::vector<TechEnum>>& columnToTechEnums = unlockSys->columnToTechEnums();

	SetupTechBoxColumn(columnToTechEnums[1], TechList_DarkAge1);
	SetupTechBoxColumn(columnToTechEnums[2], TechList_DarkAge2);

	SetupTechBoxColumn(columnToTechEnums[3], TechList_MiddleAge1);
	SetupTechBoxColumn(columnToTechEnums[4], TechList_MiddleAge2);
	SetupTechBoxColumn(columnToTechEnums[5], TechList_MiddleAge3);

	SetupTechBoxColumn(columnToTechEnums[6], TechList_EnlightenmentAge1);
	SetupTechBoxColumn(columnToTechEnums[7], TechList_EnlightenmentAge2);
	SetupTechBoxColumn(columnToTechEnums[8], TechList_EnlightenmentAge3);
	SetupTechBoxColumn(columnToTechEnums[9], TechList_EnlightenmentAge4);

	SetupTechBoxColumn(columnToTechEnums[10], TechList_IndustrialAge1);
	SetupTechBoxColumn(columnToTechEnums[11], TechList_IndustrialAge2);
	SetupTechBoxColumn(columnToTechEnums[12], TechList_IndustrialAge3);
	SetupTechBoxColumn(columnToTechEnums[13], TechList_IndustrialAge4);

	check(techEnumToTechBox.Num() == unlockSys->GetTechCount());


	isInitialized = true;
}