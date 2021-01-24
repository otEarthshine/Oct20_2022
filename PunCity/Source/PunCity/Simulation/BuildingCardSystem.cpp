// Fill out your copyright notice in the Description page of Project Settings.


#include "BuildingCardSystem.h"

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
				LOCTEXT("MaxCardsQueued", "You have reached the maximum of 5 queued card hards. 5 queued hands were converted to 1 wild card."),
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
	if (_rareHandEnum == RareHandEnum::InitialCards1 ||
		_rareHandEnum == RareHandEnum::InitialCards2)
	{
		return LOCTEXT("ChooseACard", "CHOOSE A CARD");
	}
	if (_rareHandEnum == RareHandEnum::BuildingSlotCards) {
		return LOCTEXT("ChoosePrize", "CHOOSE YOUR PRIZE !");
	}
	return LOCTEXT("ChooseRareCard", "CHOOSE YOUR RARE CARD PRIZE !");
}

void BuildingCardSystem::RollRareHandExecute()
{
	int drawCount = _simulation->buildingFinishedCount(_playerId, CardEnum::Oracle) > 0 ? 4 : 3;

	// Randomly draw new hand..
	auto shouldDraw = [&](CardEnum cardEnum)
	{
		// Don't draw if already bought
		for (size_t j = 0; j < _cardsBought.size(); j++) {
			if (_cardsBought[j].buildingEnum == cardEnum) {
				return false;
			}
		}

		// Don't draw if already has building
		if (IsBuildingCard(cardEnum) && _simulation->buildingCount(_playerId, cardEnum)) {
			return false;
		}

		if (TownhallCardCount(cardEnum) > 0) {
			return false;
		}

		return true;
	};

	// Clear old hand
	_cardsRareHand.clear();

	if (_rareHandEnum == RareHandEnum::InitialCards1)
	{
		std::vector<CardEnum> cardEnums
		{
			CardEnum::WheatSeed,
			CardEnum::Investment,
			CardEnum::TradingPost,
		};
		for (CardEnum cardEnum : cardEnums) {
			_cardsRareHand.push_back(cardEnum);
		}
		//RandomInsertToRareHand(cardEnums, 3);
	}
	else if (_rareHandEnum == RareHandEnum::InitialCards2)
	{
		std::vector<CardEnum> cardEnums
		{
			CardEnum::ProductivityBook,
			CardEnum::CabbageSeed,
			CardEnum::IronSmelter,
		};
		for (CardEnum cardEnum : cardEnums) {
			_cardsRareHand.push_back(cardEnum);
		}
		//RandomInsertToRareHand(cardEnums, 3);

		PopupInfo popup(_playerId,
			LOCTEXT("GuideAsk_Pop", "Would you like some guidance?"),
			{ LOCTEXT("ChoiceGuide", "Yes, guide me"),
				LOCTEXT("ChoiceNoGuide", "No, I already know what to do") },
			PopupReceiverEnum::StartGame_AskAboutAdvice, true
		);
		_simulation->AddPopup(popup);
	}
	else if (_rareHandEnum == RareHandEnum::BuildingSlotCards)
	{
		RandomInsertToRareHand(BuildingSlotCards);
	}
	else if (_rareHandEnum == RareHandEnum::CratesCards)
	{
		RandomInsertToRareHand(CrateCards);
	}
	else if (_rareHandEnum == RareHandEnum::PopulationQuestCards)
	{
		int32 population = _simulation->population(_playerId);
		_rareHandMessage = FText::Format(LOCTEXT("PopulationMilestone_Pop", "{0} people now call your {1} home!"), TEXT_NUM(population), _simulation->townSizeNameT(_playerId));

		std::vector<CardEnum> cardEnums
		{
			CardEnum::CoalPipeline,
			CardEnum::MiningEquipment,
			CardEnum::Conglomerate,
			CardEnum::BeerTax,
			CardEnum::ProductivityBook,
			CardEnum::SustainabilityBook,
			CardEnum::FrugalityBook,
			CardEnum::GoldRush,
		};

		if (_simulation->GetHouseLvlCount(_playerId, 2, true) > 0) {
			cardEnums.push_back(CardEnum::HomeBrew);
			cardEnums.push_back(CardEnum::HappyBreadDay);
			cardEnums.push_back(CardEnum::BlingBling);
		}

		// Remove card if we already have it
		for (size_t i = cardEnums.size(); i-- > 0;) {
			if (TownhallCardCount(cardEnums[i]) > 0 || BoughtCardCount(cardEnums[i]) > 0) {
				cardEnums.erase(cardEnums.begin() + i);
			}
		}


		RandomInsertToRareHand(cardEnums, 3);
	}
	else
	{

		std::vector<CardEnum> drawableCards;
		for (int32_t i = 0; i < RareCardsCount; i++) {
			if (shouldDraw(RareCards[i])) {
				drawableCards.push_back(RareCards[i]);
			}
		}

		for (int i = 0; i < drawCount; i++)
		{
			int32_t rareCardIndex = GameRand::Rand() % drawableCards.size();
			_cardsRareHand.push_back(drawableCards[rareCardIndex]);
			drawableCards.erase(drawableCards.begin() + rareCardIndex);

			if (drawableCards.size() == 0) {
				break;
			}
		}

		PUN_CHECK(_cardsRareHand.size() <= drawCount);
	}

	_cardsRareHandReserved.clear();
	_cardsRareHandReserved.resize(_cardsRareHand.size(), false);

	justRerolledRareHand = true;
}


#undef LOCTEXT_NAMESPACE