// Pun Dumnernchanvanit's


#include "TechUI.h"
#include "TechEraUI.h"

using namespace std;

void UTechUI::SetupTechBoxUIs()
{
	UnlockSystem* unlockSys = simulation().unlockSystem(playerId());
	const std::vector<std::vector<TechEnum>>& eraToTechEnums = unlockSys->eraToTechEnums();

	TechScrollBox->ClearChildren();

	for (int32 i = 1; i < eraToTechEnums.size(); i++)
	{
		UTechEraUI* techEraUI = AddWidget<UTechEraUI>(UIEnum::TechEraUI);

		{ // EraText
			techEraUI->EraText->SetText(ToFText("Era " + eraNumberToText[i]));

			std::stringstream ss;
			ss << "Era " << eraNumberToText[i];

			if (i < eraToTechEnums.size() - 1)
			{
				ss << "<space>";
				ss << "Unlock " << unlockSys->techsToUnlockedNextEra(i)
					<< " Technologies in Era " << eraNumberToText[i]
					<< " to unlock Era " << eraNumberToText[i + 1] << ".";

				std::stringstream ss2;
				UnlockSystem::EraUnlockedDescription(ss2, i + 1, true);

				if (ss2.str().size() > 0) {
					ss << "<space>";
					ss << "Rewards for Unlocking Era " << eraNumberToText[i + 1] << ":";
					ss << "<space>";
					ss << ss2.str();
				}
			}

			AddToolTip(techEraUI->EraText, ss);
		}
		
		techEraUI->TechList->ClearChildren();
		TechScrollBox->AddChild(techEraUI);
		std::vector<TechEnum> techEnums = eraToTechEnums[i];
		
		for (TechEnum techEnum : techEnums)
		{
			auto tech = unlockSys->GetTechInfo(techEnum);
			
			UTechBoxUI* techBox = AddWidget<UTechBoxUI>(UIEnum::TechBox);
			techEraUI->TechList->AddChild(techBox);
			
			techEnumToTechBox.Add(static_cast<int32>(tech->techEnum), techBox);
			PUN_CHECK(techBox->techEnum == TechEnum::None);

			techBox->Init(this, tech->techEnum);

			techBox->TechName->SetText(ToFText(tech->GetName()));
		}
	}

	isInitialized = true;
}

void UTechUI::TickUI()
{
	const auto& unlockSys = simulation().unlockSystem(playerId());

	if (GetVisibility() == ESlateVisibility::Collapsed) {
		// Open tech UI if there is no more queue...
		if (unlockSys->shouldOpenTechUI) {
			SetShowUI(true);
			unlockSys->shouldOpenTechUI = false;
		}
		return;
	}

	// Try initialize
	if (!isInitialized) {
		if (simulation().isInitialized()) {
			SetupTechBoxUIs();
		} else {
			return;
		}
	}


	int32 currentEra = unlockSys->currentEra();

	{
		std::stringstream ss;
		unlockSys->SetDisplaySciencePoint(ss);
		ss << " Science Points";
		ScienceAmountText->SetText(ToFText(ss.str()));
	}

	// Set next tech sci need
				//ss << techInfo->scienceNeeded(unlockSys->techsFinished) << "<img id=\"Science\"/> Science";
			//tooltipBox->AddRichText(ss);
			//tooltipBox->AddLineSpacer(12);

	// Set Era status...
	for (int32 i = 0; i < TechScrollBox->GetChildrenCount(); i++)
	{
		int32 era = i + 1;
		auto techEraUI = CastChecked<UTechEraUI>(TechScrollBox->GetChildAt(i));
		techEraUI->EraText->SetColorAndOpacity(era <= currentEra ? FLinearColor::White : FLinearColor(.2, .2, .2));

		if (era == currentEra + 1) {
			stringstream ss;
			ss << "Unlock: "
			<< unlockSys->techsUnlockedInEra(currentEra) << "/" << unlockSys->techsToUnlockedNextEra(currentEra)
			<< " Era " << eraNumberToText[i] << " Techs";
			techEraUI->EraUnlockText->SetText(ToFText(ss.str()));
		} else {
			techEraUI->EraUnlockText->SetText(FText());
		}
	}
	

	// Update highlight to highlight the tech queue
	const auto& techQueue = unlockSys->techQueue();
	if (techQueue != _lastTechQueue || unlockSys->needTechDisplayUpdate)
	{
		// First pass: isResearched or isLocked
		for (const auto& pair : techEnumToTechBox) {
			UTechBoxUI* techBox = pair.Value;
			auto tech = unlockSys->GetTechInfo(techBox->techEnum);
			bool isLocked = tech->era > currentEra;
			
			pair.Value->SetTechState(tech->state, isLocked, false, tech);
		}

		// Second pass: active
		for (TechEnum techEnum : techQueue) {
			auto tech = unlockSys->GetTechInfo(techEnum);
			techEnumToTechBox[static_cast<int>(techEnum)]->SetTechState(tech->state, false, true, tech);
		}
		
		_lastTechQueue = techQueue;
		unlockSys->needTechDisplayUpdate = false;
	}

	// Move research fraction...
	if (techQueue.size() > 0) {
		auto tech = unlockSys->GetTechInfo(techQueue.back());
		techEnumToTechBox[static_cast<int>(tech->techEnum)]->SetTechState(tech->state, false, true, tech);
	}


	/*
	 * Update tooltip
	 */
	for (const auto& pair : techEnumToTechBox) {
		pair.Value->UpdateTooltip();
	}
}

void UTechUI::CallBack1(UPunWidget* punWidgetCaller, CallbackEnum callBackEnum)
{
	UTechBoxUI* techBox = CastChecked<UTechBoxUI>(punWidgetCaller);

	// Make sure this tech is not locked or researched
	auto unlockSys = simulation().unlockSystem(playerId());
	if (unlockSys->isTechLocked(techBox->techEnum) ||
		unlockSys->IsResearched(techBox->techEnum)) 
	{
		dataSource()->Spawn2DSound("UI", "UIIncrementalError");
		return;
	}
	

	PUN_LOG("Chose Research: %d", (int)techBox->techEnum);

	auto command = make_shared<FChooseResearch>();
	command->techEnum = techBox->techEnum;
	networkInterface()->SendNetworkCommand(command);

	// Play Sound
	dataSource()->Spawn2DSound("UI", "ResearchInitiated");
}