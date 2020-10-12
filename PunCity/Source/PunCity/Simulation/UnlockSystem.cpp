// Fill out your copyright notice in the Description page of Project Settings.

#include "UnlockSystem.h"
#include "BuildingCardSystem.h"

using namespace std;

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
		//switch (buildingEnum)
		//{
		////case CardEnum::Pig:
		////case CardEnum::Sheep:
		////case CardEnum::Cow:
		////case CardEnum::Panda:
		//	cardCount = 1;
		//	break;
		//default:
		//	break;
		//}
		
		cardSystem.AddDrawCards(buildingEnum, cardCount);

		// TODO: Forced Popup to force people to choose whether to buy or refuse... (For all  Event popups? But then there is UnlockAll... Trim popup by choosing "refuse" in that case)
		if (IsBuildingCard(buildingEnum))
		{
			int32 buildingEnumInt = static_cast<int32>(buildingEnum);
			
			std::stringstream ss;
			ss << "Unlocked " << GetBuildingInfo(buildingEnum).name;
			ss << "\nWould you like to buy a " << GetBuildingInfo(buildingEnum).name << " card for " << cardSystem.GetCardPrice(buildingEnum) << " <img id=\"Coin\"/>.";
			_simulation->AddPopup(
				PopupInfo(playerId, ss.str(), { "buy", "refuse" }, PopupReceiverEnum::DoneResearchBuyCardEvent, false, "ResearchComplete", buildingEnumInt)
			);
		}
	}

	for (CardEnum buildingEnum : _permanentBuildingEnums) {
		unlockSys->UnlockBuilding(buildingEnum);
	}

	ResearchInfo::OnUnlock(playerId, simulation);
}

void UnlockSystem::OnEraUnlocked(std::stringstream& ss)
{
	ss << "Congratulation!<space>";
	ss << " Your town has advanced to Era " + eraNumberToText[currentEra()] + ".";

	if (currentEra() == 2) {
		ss << "<space>";
		ss << " Unlocked Global Slot Cards:";
		ss << "<bullet>Chimney Restrictor</>";
		//ss << "<bullet>Child Marriage</>";
		ss << "<bullet>Prolong Life</>";
		ss << "<bullet>Birth Control</>";
		ss << "<bullet>Coal Treatment</>";
		
		auto& cardSys = _simulation->cardSystem(_playerId);
		cardSys.AddDrawCards(CardEnum::ChimneyRestrictor, 1);
		//cardSys.AddDrawCards(CardEnum::ChildMarriage, 1);
		cardSys.AddDrawCards(CardEnum::ProlongLife, 1);
		cardSys.AddDrawCards(CardEnum::BirthControl, 1);
		cardSys.AddDrawCards(CardEnum::CoalTreatment, 1); // Encourage coal usage...
	}
	else if (currentEra() == 3) {
		ss << "<space>";
		ss << " Unlocked Cards:";
		ss << "<bullet>Wild Card</>";
		ss << "<bullet>Agriculture Wild Card</>";
		ss << "<bullet>Industry Wild Card</>";
		ss << "<bullet>Mine Wild Card</>";
		ss << "<bullet>Service Wild Card</>";
		ss << "<bullet>Card Removal Card</>";

		auto& cardSys = _simulation->cardSystem(_playerId);
		cardSys.AddDrawCards(CardEnum::WildCard, 1);
		cardSys.AddDrawCards(CardEnum::WildCardFood, 2);
		cardSys.AddDrawCards(CardEnum::WildCardIndustry, 2);
		cardSys.AddDrawCards(CardEnum::WildCardMine, 1);
		cardSys.AddDrawCards(CardEnum::WildCardService, 1);
		cardSys.AddDrawCards(CardEnum::CardRemoval, 1);
	}
	else if (currentEra() == 8) {
		ss << "<space>";
		ss << " You have reached the final era.";
		ss << "<space>";
		//ss << " Once you researched all technologies in this era, you win the game.";


		// Warn other players
		std::stringstream warnSS;
		warnSS << _simulation->playerName(_playerId) << " has reached the final era.<space>";
		//warnSS << "Once all final era technologies are researched, " << _simulation->playerName(_playerId) + " will be victorious.";
		_simulation->AddPopupAll(PopupInfo(_playerId, warnSS.str()), _playerId);
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
			simulation->AddPopupToFront(playerId, "Unlocked bridge!", ExclusiveUIEnum::None, "PopupNeutral");
			simulation->unlockSystem(playerId)->UnlockBuilding(CardEnum::Bridge);
		}
	}

	if (techEnum == TechEnum::InfluencePoints) {
		simulation->AddPopup(playerId, "Unlocked Influence Points <img id=\"Influence\"/> used to claim land.");
	}
	if (techEnum == TechEnum::Conquer) {
		simulation->AddPopup(playerId, "Unlocked Province Conquering"
												"<space>"
												"You can now conquer opponent's provinces with Influence Points <img id=\"Influence\"/>.");
	}
	if (techEnum == TechEnum::Vassalize) 
	{
		simulation->AddPopup(playerId, "Unlocked Vassalization"
			"<space>"
			"You can now vassalize another city with Influence Points <img id=\"Influence\"/>.\n"
			"To vassalize another city, click on the townhall "
			"Your vassal city keep their control, but must pay vassal tax to you.");
	}

	if (techEnum == TechEnum::Combo)
	{
		for (int32 i = 0; i < BuildingEnumCount; i++) {
			const std::vector<int32>& bldIds = simulation->buildingIds(playerId, static_cast<CardEnum>(i));
			for (int32 bldId : bldIds) {
				simulation->building(bldId).CheckCombo();
			}
		}
		
		simulation->AddPopup(playerId, "Unlocked Building Combo"
			"<space>"
			"You can gain Building Combo by constructing multiple Buildings of the same type."
			"<bullet>Combo Level 1: 2 same-type buildings</>"
			"<bullet>Combo Level 2: 4 same-type buildings</>"
			"<bullet>Combo Level 3: 8 same-type buildings</>");
	}
}