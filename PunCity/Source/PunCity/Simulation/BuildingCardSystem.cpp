// Fill out your copyright notice in the Description page of Project Settings.


#include "BuildingCardSystem.h"

#include "TownManagerBase.h"

#define LOCTEXT_NAMESPACE "BuildingCardSystem"

void BuildingCardSystem::TickRound()
{
	const int32 maxQueueCount = 5;

	_cardHandQueueCount++;

	if (_cardHandQueueCount > maxQueueCount)
	{
		if (allowMaxCardHandQueuePopup)
		{
			_simulation->AddPopupNonDuplicate(PopupInfo(_playerId,
				LOCTEXT("MaxCardsQueued", "You have reached the maximum of 5 queued card hands. 5 queued hands were converted to 1 wild card."),
				{ LOCTEXT("Close", "Close"), LOCTEXT("Do not show this again", "Do not show this again") },
				PopupReceiverEnum::MaxCardHandQueuePopup)
			);
		}

		_cardHandQueueCount = 1; // Combine to create wild card
		AddCardToHand2(CardEnum::WildCard);
	}

	// Show hand if not already done so
	if (_isCardStackBlank) {
		_rerollCountThisRound = 0;
		_alreadyBoughtCardThisRound = false;
		RollHand(handSize());
	}
}

FText BuildingCardSystem::rareHandMessage2()
{
	switch(_rareHandData.rareHandEnum)
	{
	case RareHandEnum::InitialCards1: return LOCTEXT("ChooseAStartingCard", "Choose a Starting Card");

	case RareHandEnum::TownhallLvl2Cards:
	case RareHandEnum::TownhallLvl3Cards:
	case RareHandEnum::TownhallLvl4Cards:
	case RareHandEnum::TownhallLvl5Cards:
		return LOCTEXT("TownhallLvlChooseCards", "Choose a Townhall Slot Card");
		
	case RareHandEnum::BorealCards:
	case RareHandEnum::DesertCards:
	case RareHandEnum::SavannaCards:
	case RareHandEnum::JungleCards:
	case RareHandEnum::ForestCards:
		return LOCTEXT("Biome Choose Card", "Choose a Biome Bonus");
	case RareHandEnum::BorealCards2:
	case RareHandEnum::DesertCards2:
	case RareHandEnum::SavannaCards2:
	case RareHandEnum::JungleCards2:
	case RareHandEnum::ForestCards2:
		return LOCTEXT("Biome Choose Another Card", "Choose another Biome Bonus");

		
	case RareHandEnum::Era2_1_Cards:
	case RareHandEnum::Era3_1_Cards:
	case RareHandEnum::Era4_1_Cards:
		return LOCTEXT("Era Choose Card", "Choose an Era Bonus");
	case RareHandEnum::Era2_2_Cards:
	case RareHandEnum::Era3_2_Cards:
	case RareHandEnum::Era4_2_Cards:
		return LOCTEXT("Era Choose Another Card", "Choose another Era Bonus");

	case RareHandEnum::BuildingSlotCards: return LOCTEXT("ChoosePrize", "CHOOSE YOUR PRIZE !");

	default:
		return LOCTEXT("ChooseRareCard", "CHOOSE YOUR RARE CARD PRIZE !");
	}
}

void BuildingCardSystem::RollRareHandExecute()
{
	int drawCount = 3;// _simulation->buildingFinishedCount(_playerId, CardEnum::Oracle) > 0 ? 4 : 3;

	// Randomly draw new hand..
	auto shouldDraw = [&](CardEnum cardEnum)
	{
		// Don't draw if already bought
		for (size_t j = 0; j < _cardsBought.size(); j++) {
			if (_cardsBought[j].cardEnum == cardEnum) {
				return false;
			}
		}

		// Don't draw if already has building
		if (IsBuildingCard(cardEnum) && _simulation->buildingCount(_playerId, cardEnum)) {
			return false;
		}

		if (_simulation->TownhallCardCountAll(_playerId, cardEnum) > 0) {
			return false;
		}

		return true;
	};

	// Clear old hand

	_cardsRareHand.clear();

	uint8 rareHandEnumInt = static_cast<uint8>(_rareHandData.rareHandEnum);

	// Initial Cards
	if (_rareHandData.rareHandEnum == RareHandEnum::InitialCards1)
	{
		_cardsRareHand = {
			CardEnum::WheatSeed,
			CardEnum::TradingPost,
			CardEnum::Investment,
		};

		PopupInfo popup(_playerId,
			LOCTEXT("GuideAsk_Pop", "Would you like some guidance?"),
			{ LOCTEXT("ChoiceGuide", "Yes, guide me"),
				LOCTEXT("ChoiceNoGuide", "No, I already know what to do") },
			PopupReceiverEnum::StartGame_AskAboutAdvice, true
		);
		_simulation->AddPopup(popup);
	}
	

	//
	else if (_rareHandData.rareHandEnum == RareHandEnum::BuildingSlotCards)
	{
		RandomInsertToRareHand(BuildingSlotCards_NoUpgrade);
	}
	else if (_rareHandData.rareHandEnum == RareHandEnum::CratesCards)
	{
		RandomInsertToRareHand(CrateCards);
	}
	//! Townhall Lvl
	else if (static_cast<uint8>(RareHandEnum::TownhallLvl2Cards) <= rareHandEnumInt && rareHandEnumInt <= static_cast<uint8>(RareHandEnum::TownhallLvl5Cards))
	{

		//// Lvl 2 upgrade, mostly encouragement to upgrade houses
		//_cardsRareHand = {
		//	CardEnum::BeerTax,
		//	CardEnum::HomeBrew,
		//	CardEnum::ChimneyRestrictor,
		//};
		
	}
	//! Population Quests
	else if (static_cast<uint8>(RareHandEnum::PopulationQuestCards1) <= rareHandEnumInt && rareHandEnumInt <= static_cast<uint8>(RareHandEnum::PopulationQuestCards7))
	{
		int32 population = _simulation->populationPlayer(_playerId);
		_rareHandData.message = FText::Format(LOCTEXT("PopulationMilestone_Pop", "{0} people now call your city home!"), TEXT_NUM(population));

		std::vector<CardEnum> cardEnums;
		if (_rareHandData.rareHandEnum == RareHandEnum::PopulationQuestCards1) {
			cardEnums = {
				CardEnum::ProductivityBook,
				CardEnum::SustainabilityBook,
				CardEnum::FrugalityBook,
			};
		}
		else if (_rareHandData.rareHandEnum == RareHandEnum::PopulationQuestCards2) {
			cardEnums = {
				CardEnum::ProductivityBook,
				CardEnum::SustainabilityBook,
				CardEnum::FrugalityBook,
			};
		}
		else if (_rareHandData.rareHandEnum == RareHandEnum::PopulationQuestCards3) {
			cardEnums = {
				CardEnum::ProductivityBook,
				CardEnum::SustainabilityBook,
				CardEnum::FrugalityBook,
			};
		}
		else if (_rareHandData.rareHandEnum == RareHandEnum::PopulationQuestCards4) {
			cardEnums = {
				CardEnum::ProductivityBook,
				CardEnum::SustainabilityBook,
				CardEnum::Motivation,
			};
		}
		else if (_rareHandData.rareHandEnum == RareHandEnum::PopulationQuestCards5) {
			cardEnums = {
				CardEnum::ProductivityBook,
				CardEnum::Motivation,
				CardEnum::Passion,
			};
		}
		else if (_rareHandData.rareHandEnum == RareHandEnum::PopulationQuestCards6) {
			cardEnums = {
				CardEnum::SustainabilityBook,
				CardEnum::Motivation,
				CardEnum::Passion,
			};
		}
		else if (_rareHandData.rareHandEnum == RareHandEnum::PopulationQuestCards7) {
			cardEnums = {
				CardEnum::ProductivityBook,
				CardEnum::Motivation,
				CardEnum::Passion,
			};
		}
		//else if (_rareHandEnum == RareHandEnum::PopulationQuestCards8) {
		//	cardEnums = {
		//		CardEnum::SocialWelfare,
		//		CardEnum::Motivation,
		//		CardEnum::Passion,
		//	};
		//}

		// Remove card if we already have it
		//for (size_t i = cardEnums.size(); i-- > 0;) {
		//	if (_simulation->TownhallCardCountAll(_playerId, cardEnums[i]) > 0 || BoughtCardCount(cardEnums[i]) > 0) {
		//		cardEnums.erase(cardEnums.begin() + i);
		//	}
		//}


		RandomInsertToRareHand(cardEnums, 3);
	}
	else
	{
		// Biomes/Era Bonuses
		std::vector<CardEnum> bonusCardEnums = PermanentBonus::BonusHandEnumToCardEnums(_rareHandData.rareHandEnum);
		if (bonusCardEnums.size() > 0) {
			_cardsRareHand = bonusCardEnums;
		}
		else
		{
			std::vector<CardEnum> cardEnums = {
				CardEnum::ProductivityBook,
				CardEnum::SustainabilityBook,
				CardEnum::FrugalityBook,
				CardEnum::Motivation,
				CardEnum::Passion,
			};

			RandomInsertToRareHand(cardEnums, 3);
			PUN_CHECK(_cardsRareHand.size() <= drawCount);
		}
	}

	_cardsRareHandReserved.clear();
	_cardsRareHandReserved.resize(_cardsRareHand.size(), false);

	justRerolledRareHand = true;
}


#undef LOCTEXT_NAMESPACE