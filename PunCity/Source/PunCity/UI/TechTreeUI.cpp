// Fill out your copyright notice in the Description page of Project Settings.


#include "TechTreeUI.h"

using namespace std;

void UTechTreeUI::SetupTechBoxUIs()
{
	//UnlockSystem* unlockSys = simulation().unlockSystem(playerId());
	//const std::vector<shared_ptr<ResearchInfo>>& techs = unlockSys->techs();

	//for (const auto& tech : techs) {
	//	UTechBoxUI* techBoxUI = techBoxLocationToTechBox[tech->techBoxLocation.id()];
	//	PUN_CHECK(techBoxUI->techEnum == TechEnum::None);

	//	SetChildHUD(techBoxUI);
	//	techBoxUI->Init(this, tech->researchEnum);

	//	std::vector<string> nameAndDescriptions = tech->NameAndDescriptions();
	//	
	//	techBoxUI->TechName->SetText(ToFText(nameAndDescriptions[0]));

	//	// Add Tooltip
	//	stringstream ssTip;
	//	ssTip << nameAndDescriptions[0];
	//	ssTip << "\n\n";
	//	for (size_t i = 1; i < nameAndDescriptions.size(); i++) {
	//		ssTip << nameAndDescriptions[i] << "\n";
	//	}

	//	if (tech->_buildingEnums.size() > 0) {
	//		ssTip << "Unlock cards:\n";
	//	}
	//	for (BuildingEnum buildingEnum : tech->_buildingEnums) {
	//		ssTip << "- " << GetBuildingInfo(buildingEnum).name << "\n";
	//	}

	//	if (tech->_permanentBuildingEnums.size() > 0) {
	//		ssTip << "Unlock permanent cards:\n";
	//	}
	//	for (BuildingEnum buildingEnum : tech->_permanentBuildingEnums) {
	//		ssTip << "- " << GetBuildingInfo(buildingEnum).name << "\n";
	//	}
	//	
	//	AddToolTip(techBoxUI, ssTip.str());
	//}
	
	isInitialized = true;
}

void UTechTreeUI::TickUI()
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

	//// Update highlight to highlight the tech queue
	//const auto& techQueue = unlockSys->techQueue();
	//if (techQueue != _lastTechQueue || unlockSys->needDisplayUpdate)
	//{
	//	// Unhighlight everything
	//	for (const auto& pair : techBoxLocationToTechBox) {
	//		UTechBoxUI* techBox = pair.Value;
	//		auto tech = unlockSys->GetTechInfo(techBox->techEnum);
	//		pair.Value->SetTechState(tech->state, false, tech->researchFraction(unlockSys->techsFinished));
	//	}

	//	for (auto tech : techQueue) {
	//		PUN_CHECK(tech->techBoxLocation.isValid());
	//		techBoxLocationToTechBox[tech->techBoxLocation.id()]->SetTechState(tech->state, true, tech->researchFraction(unlockSys->techsFinished));
	//	}
	//	_lastTechQueue = techQueue;
	//	unlockSys->needDisplayUpdate = false;
	//}

	//// Move research fraction...
	//if (techQueue.size() > 0) {
	//	auto tech = techQueue.back();
	//	PUN_CHECK(tech->techBoxLocation.isValid());
	//	techBoxLocationToTechBox[tech->techBoxLocation.id()]->SetTechState(tech->state, true, tech->researchFraction(unlockSys->techsFinished));
	//}
	
}

void UTechTreeUI::CallBack1(UPunWidget* punWidgetCaller, CallbackEnum callBackEnum)
{
	UTechBoxUI* techBox = CastChecked<UTechBoxUI>(punWidgetCaller);

	PUN_LOG("Chose Research: %d", (int)techBox->techEnum);

	auto command = make_shared<FChooseResearch>();
	command->techEnum = techBox->techEnum;
	networkInterface()->SendNetworkCommand(command);

	// Play Sound
	dataSource()->Spawn2DSound("UI", "ResearchInitiated");
}

void UTechTreeUI::CloseUI()
{
	SetShowUI(false);
}