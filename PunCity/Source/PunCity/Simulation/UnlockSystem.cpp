// Fill out your copyright notice in the Description page of Project Settings.

#include "UnlockSystem.h"
#include "BuildingCardSystem.h"

using namespace std;

#define LOCTEXT_NAMESPACE "UnlockSystem"

void Building_Research::InitBuildingResearch(std::vector<CardEnum> buildingEnums, std::vector<CardEnum> permanentBuildingEnums, IGameSimulationCore* simulation, int32_t cardCount)
{
	_buildingEnums = buildingEnums;
	_permanentBuildingEnums = permanentBuildingEnums;
	_simulation = simulation;
	_cardCount = cardCount;

	PUN_CHECK(_simulation != nullptr);
}

void Building_Research::OnUnlock(int32 playerId, IGameSimulationCore* simulation)
{
	BuildingCardSystem& cardSystem = _simulation->cardSystem(playerId);
	auto unlockSys = _simulation->unlockSystem(playerId);
	
	for (CardEnum buildingEnum : _buildingEnums)
	{
		int cardCount = _cardCount;

		// Single cards
		if (IsSeedCard(buildingEnum)) {
			cardCount = 1;
		}

		// Special case: Common Seeds (Herbs, Potato etc.)
		if (IsCommonSeedCard(buildingEnum)) {
			cardSystem.AddCardToHand2(buildingEnum);

			_simulation->AddPopup(playerId, 
				FText::Format(LOCTEXT("UnlockedSeed_Pop", "You unlocked and received {0} Card."), GetBuildingInfo(buildingEnum).name)
			);
			continue;
		}
		
		cardSystem.AddDrawCards(buildingEnum, cardCount);

		// TODO: Forced Popup to force people to choose whether to buy or refuse... (For all  Event popups? But then there is UnlockAll... Trim popup by choosing "refuse" in that case)
		if (IsBuildingCard(buildingEnum))
		{
			int32 buildingEnumInt = static_cast<int32>(buildingEnum);
			
			_simulation->AddPopup(
				PopupInfo(playerId, 
					FText::Format(
						LOCTEXT("UnlockedBuilding_Pop", "Unlocked {0}\nWould you like to buy a {0} card for {1}<img id=\"Coin\"/>."),
						GetBuildingInfo(buildingEnum).name,
						TEXT_NUM(cardSystem.GetCardPrice(buildingEnum))
					),
					{ LOCTEXT("Buy", "Buy"), LOCTEXT("Refuse", "Refuse") },
					PopupReceiverEnum::DoneResearchBuyCardEvent, false, "ResearchComplete", buildingEnumInt
				)
			);
		}
	}

	for (CardEnum buildingEnum : _permanentBuildingEnums) {
		unlockSys->UnlockBuilding(buildingEnum);
	}

	ResearchInfo::OnUnlock(playerId, simulation);
}

void UnlockSystem::Research(int32 science100PerRound, int32 updatesPerSec)
{
	if (SimSettings::IsOn("CheatFastTech")) {
		science100PerRound += 20000 * 100 * 20;
	}

	// Multiple updates per second, so we divide accordingly science100PerRound/updatesPerSec
	science100XsecPerRound += GameRand::RandRound(science100PerRound, updatesPerSec);
	science100XsecPerRound = std::min(science100XsecPerRound, 1800000000); // 600m = 40,000 -> 1800m = 120,000

	if (!hasTargetResearch()) {
		return;
	}

	auto tech = currentResearch();

	if (science100() >= science100Needed())
	{
		// Take away the amount of science used 
		science100XsecPerRound -= science100Needed() * Time::SecondsPerRound;

		int32 lastEra = currentEra();

		tech->state = TechStateEnum::Researched;
		_techQueue.pop_back();

		tech->OnUnlock(_playerId, _simulation);

		techsFinished++;
		needTechDisplayUpdate = true;

		std::vector<FText> choices = {
			LOCTEXT("Show tech tree", "Show tech tree"),
			LOCTEXT("Close", "Close")
		};
		PopupReceiverEnum receiver = PopupReceiverEnum::DoneResearchEvent_ShowTree;

		// Era popup
		bool unlockedNewEra = currentEra() > lastEra;
		if (unlockedNewEra)
		{
			TArray<FText> args;

			OnEraUnlocked(args);

			PopupInfo popupInfo(_playerId, JOINTEXT(args), choices, receiver, true);
			popupInfo.warningForExclusiveUI = ExclusiveUIEnum::TechUI;
			popupInfo.forcedSkipNetworking = true;
			_simulation->AddPopupToFront(popupInfo);

			_simulation->soundInterface()->Spawn2DSound("UI", "ResearchCompleteNewEra", _playerId);

			_simulation->GenerateRareCardSelection(_playerId, RareHandEnum::BuildingSlotCards, LOCTEXT("New era!", "New era!"));
		}
		else
		{
			PopupInfo popupInfo(_playerId, 
				LOCTEXT("Research Completed.", "Research Completed."), 
				choices, receiver, true
			);
			popupInfo.warningForExclusiveUI = ExclusiveUIEnum::TechUI;
			popupInfo.forcedSkipNetworking = true;
			_simulation->AddPopupToFront(popupInfo);

			_simulation->soundInterface()->Spawn2DSound("UI", "ResearchComplete", _playerId);

			/*
			 * Science victory
			 */
			if (currentEra() >= 8)
			{
				//std::vector<std::shared_ptr<ResearchInfo>> finalTechs = _eraToTechs[8];
				//int32 finalEraTechsDone = 0;
				//for (auto& finalTech : finalTechs) {
				//	if (IsResearched(finalTech->techEnum)) {
				//		finalEraTechsDone++;
				//	}
				//}

				//if (finalEraTechsDone == 3) {
				//	_simulation->AddPopupAll(PopupInfo(_playerId,
				//		_simulation->playerName(_playerId) + " is only 2 technolgies away from the science victory."
				//	), _playerId);
				//	_simulation->AddPopup(_playerId, "You are only 2 technolgies away from the science victory.");
				//}
				//else if (finalEraTechsDone == 5) {
				//	_simulation->ExecuteScienceVictory(_playerId);
				//}
			}
		}

		// Reply to show tech tree if needed..
		//std::string replyReceiver = _simulation->HasTargetResearch(_playerId) ? "" : "DoneResearchEvent_ShowTree";

		//auto researchCompletePopup = [&](std::string body)
		//{
		//	if (_simulation->HasTargetResearch(_playerId)) 
		//	{
		//		// TODO: more fancy research complete!
		//		
		//		PopupInfo popupInfo(_playerId, body, { "Close" }, "", true);
		//		popupInfo.warningForExclusiveUI = ExclusiveUIEnum::TechUI;

		//		_simulation->AddPopupToFront(popupInfo);
		//	}
		//	else {
		//		PopupInfo popupInfo(_playerId, body, { "Show tech tree", "Close" }, "DoneResearchEvent_ShowTree", true);
		//		popupInfo.warningForExclusiveUI = ExclusiveUIEnum::TechUI;

		//		_simulation->AddPopupToFront(popupInfo);
		//	}
		//};

		//researchCompletePopup("Research Completed.");
	}
}

void UnlockSystem::EraUnlockedDescription(TArray<FText>& args, int32 era, bool isTip)
{
	if (era == 2) {
		if (isTip) {
			ADDTEXT_(INVTEXT(" {0}:"), LOCTEXT("Global Slot Cards", "Global Slot Cards"));
		} else {
			ADDTEXT_(INVTEXT(" {0}:"), LOCTEXT("Unlocked Global Slot Cards", "Unlocked Global Slot Cards"));
		}
		ADDTEXT_TAG_("<bullet>", LOCTEXT("Chimney Restrictor", "Chimney Restrictor"));
		//ss << "<bullet>Child Marriage</>";
		//ss << "<bullet>Prolong Life</>";
		ADDTEXT_TAG_("<bullet>", LOCTEXT("Birth Control", "Birth Control"));
		ADDTEXT_TAG_("<bullet>", LOCTEXT("Coal Treatment", "Coal Treatment"));
	}
	else if (era == 3) {
		if (isTip) {
			ADDTEXT_(INVTEXT(" {0}:"), LOCTEXT("Cards", "Cards"));
		} else {
			ADDTEXT_(INVTEXT(" {0}:"), LOCTEXT("Unlocked Cards", "Unlocked Cards"));
		}
		ADDTEXT_TAG_("<bullet>", LOCTEXT("Wild Card", "Wild Card"));
		ADDTEXT_TAG_("<bullet>", LOCTEXT("Agriculture Wild Card", "Agriculture Wild Card"));
		ADDTEXT_TAG_("<bullet>", LOCTEXT("Industry Wild Card", "Industry Wild Card"));
		ADDTEXT_TAG_("<bullet>", LOCTEXT("Mine Wild Card", "Mine Wild Card"));
		ADDTEXT_TAG_("<bullet>", LOCTEXT("Service Wild Card", "Service Wild Card"));
		ADDTEXT_TAG_("<bullet>", LOCTEXT("Card Removal Card", "Card Removal Card"));
	}
}

void UnlockSystem::OnEraUnlocked(TArray<FText>& args)
{
	auto& cardSys = _simulation->cardSystem(_playerId);
	
	ADDTEXT_(
		LOCTEXT("EraUnlockCongratsPop", "Congratulation!<space> Your town has advanced to Era {0}.<space>"),
		eraNumberToText[currentEra()]
	);

	EraUnlockedDescription(args, currentEra(), false);
	
	if (currentEra() == 2) {
		cardSys.AddDrawCards(CardEnum::ChimneyRestrictor, 1);
		//cardSys.AddDrawCards(CardEnum::ChildMarriage, 1);
		//cardSys.AddDrawCards(CardEnum::ProlongLife, 1);
		cardSys.AddDrawCards(CardEnum::BirthControl, 1);
		cardSys.AddDrawCards(CardEnum::CoalTreatment, 1); // Encourage coal usage...
	}
	else if (currentEra() == 3) {
		cardSys.AddDrawCards(CardEnum::WildCard, 1);
		cardSys.AddDrawCards(CardEnum::WildCardFood, 2);
		cardSys.AddDrawCards(CardEnum::WildCardIndustry, 2);
		cardSys.AddDrawCards(CardEnum::WildCardMine, 1);
		cardSys.AddDrawCards(CardEnum::WildCardService, 1);
		cardSys.AddDrawCards(CardEnum::CardRemoval, 1);
	}
	else if (currentEra() == 8) {
		//ss << " You have reached the final era.";
		//ss << "<space>";
		//ss << " Once you researched all technologies in this era, you win the game.";


		// Warn other players
		FText text = FText::Format(LOCTEXT("FinalEraPopAll", "{0} has reached the final era.<space>"), _simulation->playerNameT(_playerId));
		//warnSS << "Once all final era technologies are researched, " << _simulation->playerName(_playerId) + " will be victorious.";
		_simulation->AddPopupAll(PopupInfo(_playerId, text), _playerId);
	}
}


void BonusToggle_Research::OnUnlock(int32 playerId, IGameSimulationCore* simulation)
{
	if (techEnum == TechEnum::Plantation)
	{
		simulation->CheckGetSeedCard(playerId);
	}

	if (techEnum == TechEnum::ShallowWaterEmbark)
	{
		if (!simulation->IsBuildingUnlocked(playerId, CardEnum::Bridge)) {
			simulation->AddPopupToFront(playerId, 
				LOCTEXT("Unlocked bridge!", "Unlocked bridge!"),
				ExclusiveUIEnum::None, "PopupNeutral"
			);
			simulation->unlockSystem(playerId)->UnlockBuilding(CardEnum::Bridge);
		}
	}

	if (techEnum == TechEnum::InfluencePoints) {
		simulation->AddPopup(playerId, 
			LOCTEXT("UnlockedInfluencePop", "Unlocked Influence Points <img id=\"Influence\"/> used to claim land.")
		);
	}
	if (techEnum == TechEnum::Conquer) {
		simulation->AddPopup(playerId, 
			LOCTEXT("UnlockedProvinceConqueringPop", "Unlocked Province Conquering<space>You can now conquer opponent's provinces with Influence Points <img id=\"Influence\"/>.")
		);
	}
	if (techEnum == TechEnum::Vassalize) 
	{
		simulation->AddPopup(playerId, 
			LOCTEXT("UnlockedVassalizationPop", "Unlocked Vassalization<space>You can now vassalize another city with Influence Points <img id=\"Influence\"/>.\nTo vassalize another city, click vassalize on the target townhall.<space>Your vassal city keep their control, but must pay vassal tax to you.")
		);
	}

	if (techEnum == TechEnum::Combo)
	{
		for (int32 i = 0; i < BuildingEnumCount; i++) {
			const std::vector<int32>& bldIds = simulation->buildingIds(playerId, static_cast<CardEnum>(i));
			for (int32 bldId : bldIds) {
				simulation->building(bldId).CheckCombo();
			}
		}
		
		simulation->AddPopup(playerId, {
			LOCTEXT("UnlockedBuildingCombo_Pop", "Unlocked Building Combo<space>You can gain Building Combo by constructing multiple Buildings of the same type."),
			LOCTEXT("UnlockedBuildingComboBullet1_Pop", "<bullet>Combo Level 1: 2 same-type buildings (+5% productivity)</>"),
			LOCTEXT("UnlockedBuildingComboBullet2_Pop", "<bullet>Combo Level 2: 4 same-type buildings (+10% productivity)</>"),
			LOCTEXT("UnlockedBuildingComboBullet3_Pop", "<bullet>Combo Level 3: 8 same-type buildings (+15% productivity)</>")
		});
	}
}

void UnlockSystem::UpdateProsperityHouseCount()
{
	// Prosperity unlock at 7 houses
	if (!prosperityEnabled &&
		_simulation->buildingFinishedCount(_playerId, CardEnum::House) >= 7)
	{
		prosperityEnabled = true;

		TArray<FText> args;
		ADDTEXT_LOCTEXT("UnlockedProsperityUIPop", "Unlocked: House Upgrade Unlocks Menu.<space>Houses can be upgraded by supplying them with Luxury Resources.<space>Achieving certain house level count will unlock new technologies.");

		PopupInfo popupInfo(_playerId, JOINTEXT(args), {
			LOCTEXT("Show House Upgrade Unlocks", "Show House Upgrade Unlocks"),
			LOCTEXT("Close", "Close") },
			PopupReceiverEnum::UnlockedHouseTree_ShowProsperityUI);
		popupInfo.warningForExclusiveUI = ExclusiveUIEnum::ProsperityUI;
		popupInfo.forcedSkipNetworking = true;
		_simulation->AddPopupToFront(popupInfo);
	}

	if (!prosperityEnabled) {
		return;
	}

	for (int32 i = 1; i <= House::GetMaxHouseLvl(); i++)
	{
		int32 houseCount = _simulation->GetHouseLvlCount(_playerId, i, true);

		std::vector<TechEnum>& prosperityTechEnums = _houseLvlToProsperityTechEnum[i];
		for (size_t j = 0; j < prosperityTechEnums.size(); j++)
		{
			TechEnum properityTechEnum = prosperityTechEnums[j];
			auto properityTech = GetTechInfo(properityTechEnum);

			if (properityTech->state != TechStateEnum::Researched)
			{
				if (houseCount >= _houseLvlToUnlockCount[i][j])
				{
					PUN_LOG("properityTech Researched %s", *(properityTech->GetName().ToString()));

					// Unlocked
					// Take away the amount of science used 
					properityTech->state = TechStateEnum::Researched;
					properityTech->OnUnlock(_playerId, _simulation);

					needProsperityDisplayUpdate = true;

					//// popup
					//{
					//	std::stringstream ss;
					//	ss << "Reached " << _houseLvlToUnlockCount[i][j] << " house lvl " << i << ".\n";
					//	ss << "Unlocked " << properityTech->GetName() << ".";
					//	PopupInfo popupInfo(_playerId, ss.str());
					//	popupInfo.warningForExclusiveUI = ExclusiveUIEnum::ProsperityUI;
					//	popupInfo.forcedSkipNetworking = true;
					//	_simulation->AddPopupToFront(popupInfo);

					//	_simulation->soundInterface()->Spawn2DSound("UI", "ResearchComplete", _playerId);
					//}
				}
				else {
					break;
				}
			}
		}
	}
}


#undef LOCTEXT_NAMESPACE