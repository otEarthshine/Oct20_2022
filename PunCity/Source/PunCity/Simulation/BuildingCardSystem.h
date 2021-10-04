// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "IGameSimulationCore.h"
#include "PunCity/PunUtils.h"
#include "PunCity/CppUtils.h"

//struct BuildingCardStack
//{
//	CardEnum buildingEnum = CardEnum::None;
//	int32 stackSize = -1;
//	int32 cardStateValue = 0;
//
//	bool operator==(const BuildingCardStack& a) const {
//		return buildingEnum == a.buildingEnum && stackSize == a.stackSize;
//	}
//
//	void operator>>(FArchive& Ar)
//	{
//		Ar << buildingEnum;
//		Ar << stackSize;
//		Ar << cardStateValue;
//	}
//};

static const int32 CardCountForLvl[] = { 1, 2, 4, 8 };
static const int32 CardLvlCount =_countof(CardCountForLvl);

enum class ConverterCardUseState : uint8
{
	None,
	JustUsed,
	SubmittedUI,
};

/**
 * 
 */
class BuildingCardSystem
{
public:
	BuildingCardSystem(int32 playerId, IGameSimulationCore* simulation)
	{
		_simulation = simulation;
		_playerId = playerId;

		if (!simulation->isLoadingFromFile())
		{
			AddDrawCards(CardEnum::HuntingLodge, 2);
			AddDrawCards(CardEnum::FruitGatherer, 2);
			AddDrawCards(CardEnum::Fisher, 2);
			AddDrawCards(CardEnum::MushroomFarm, 2);
			AddDrawCards(CardEnum::RanchPig, 1);
			AddDrawCards(CardEnum::Forester, 1);

			AddDrawCards(CardEnum::Tavern, 1);

			AddDrawCards(CardEnum::CharcoalMaker, 1);
			AddDrawCards(CardEnum::ImmigrationOffice, 1);

			AddDrawCards(CardEnum::Quarry, 1);
			//AddCards(BuildingEnum::Tavern, 1);

			//AddDrawCards(CardEnum::EmergencyRations, 1);

			//AddDrawCards(CardEnum::ShrineWisdom, 1);

			RollHand(handSize());
		}
	}

	int32 GetCardPrice(CardEnum buildingEnum) {
		int32 cardPrice = GetBuildingInfo(buildingEnum).baseCardPrice;
		if (_simulation->TownhallCardCountAll(_playerId, CardEnum::IndustrialRevolution) && 
			IsIndustrialBuilding(buildingEnum)) 
		{
			cardPrice /= 2;
		}

		return cardPrice;
	}

	int32 GetRerollPrice()
	{
		// Note: count round default reroll as a reroll, hence (_rerollCountThisRound - 1)
		int32 rerollsFromButton = _rerollCountThisRound - 1;

		// If didn't already bought card this round, we get a free reroll the first time.
		if (!_alreadyBoughtCardThisRound && rerollsFromButton <= 0) {
			return 0;
		}
		
		return std::max(0, _rerollCountThisRound) * (_simulation->IsResearched(_playerId, TechEnum::CheapReroll) ? 5 : 10);
	}
	

	static const int32 InitialHandSize = 5;

	int32 handSize() {
		int32 handSize = InitialHandSize;
		if (_simulation->IsResearched(_playerId, TechEnum::RerollCardsPlus1)) {
			handSize += 1;
		}
		if (_simulation->townBuildingFinishedCount(_playerId, CardEnum::CensorshipInstitute)) {
			handSize -= 1;
		}
		//if (_simulation->IsResearched(_playerId, TechEnum::MoreRerollCards2)) handSize++;
		return handSize;
	}

	void TickRound();

	int32 cardHandQueueCount() { return _cardHandQueueCount; }
	void UseCardHandQueue()
	{
		_cardHandQueueCount = std::max(0, _cardHandQueueCount - 1);

		if (_cardHandQueueCount > 0) {
			_rerollCountThisRound = 0;
			_alreadyBoughtCardThisRound = false;
			RollHand(handSize(), true);
		}
		
		if (_cardHandQueueCount == 0) {
			SetCardStackBlank(true);
		}
	}
	

	void TryRefreshRareHand()
	{
		if (_rareHandsDataQueued.size() > 0 && _cardsRareHand.size() == 0) {
			_rareHandData = _rareHandsDataQueued[0];
			_rareHandsDataQueued.erase(_rareHandsDataQueued.begin());
			RollRareHandExecute();
		}
	}

	static int32_t GetRollCountdown() {
		return Time::SecondsPerRound - Time::Seconds() % Time::SecondsPerRound;
	}

	std::vector<CardEnum> GetDrawPile() { return _cardsDrawPile; }
	std::vector<CardEnum> GetDiscardPile() { return _cardsDiscardPile; }
	std::vector<CardEnum> GetAllPiles()
	{
		std::vector<CardEnum> pile;
		pile.insert(pile.end(), _cardsHand.begin(), _cardsHand.end());
		pile.insert(pile.end(), _cardsDrawPile.begin(), _cardsDrawPile.end());
		pile.insert(pile.end(), _cardsDiscardPile.begin(), _cardsDiscardPile.end());
		pile.insert(pile.end(), _cardsRemovedPile.begin(), _cardsRemovedPile.end());

		return pile;
	}
	std::vector<CardEnum> GetUnremovedPiles()
	{
		std::vector<CardEnum> pile;
		pile.insert(pile.end(), _cardsHand.begin(), _cardsHand.end());
		pile.insert(pile.end(), _cardsDrawPile.begin(), _cardsDrawPile.end());
		pile.insert(pile.end(), _cardsDiscardPile.begin(), _cardsDiscardPile.end());
		return pile;
	}

	bool HasCardInAnyPile(CardEnum cardEnumIn)
	{
		std::vector<CardEnum> pile = GetAllPiles();
		for (CardEnum cardEnum : pile) {
			if (cardEnum == cardEnumIn) {
				return true;
			}
		}
		return false;
	}

	void RollHand(int drawCount, bool rerollButton = false, bool isInitialHand = false) 
	{
		_isCardStackBlank = false;
		
		// Clear old hand
		_cardsDiscardPile.insert(_cardsDiscardPile.end(), _cardsHand.begin(), _cardsHand.end());
		_cardsHand.clear();

		//if (isInitialHand)
		//{
		//	const std::vector<CardEnum> initialHand {
		//		CardEnum::HuntingLodge,
		//		CardEnum::BerryGatherer,
		//		CardEnum::MushroomHut,
		//		CardEnum::RanchPig,
		//		CardEnum::CharcoalMaker,
		//	};

		//	for (CardEnum cardEnum : initialHand) {
		//		CppUtils::Remove(_cardsDrawPile, cardEnum);
		//		_cardsHand.push_back(cardEnum);
		//	}
		//}
		//else

		auto cardConditionNotMet = [&](CardEnum cardEnum)
		{
			// Check Wheat/Herb etc. for duplicates
			std::vector<SeedInfo> seedInfos = CommonSeedCards;
			for (SeedInfo seedInfo : seedInfos) {
				if (cardEnum == seedInfo.cardEnum) {
					return HasBoughtCard(seedInfo.cardEnum) || _simulation->HasSeed(_playerId, seedInfo.cardEnum);
				}
			}
			
			switch(cardEnum)
			{
			case CardEnum::EmergencyRations: return _simulation->foodCount(_playerId) > 30;
			default: return false;
			}
		};
		
		{
			// Randomly draw new hand..
			for (int i = 0; i < drawCount; i++)
			{
				int loop;
				for (loop = 10000; loop-- > 0;)
				{
					if (_cardsDrawPile.size() == 0) {
						// No more cards to draw, put discardPile into drawPile
						_cardsDrawPile = _cardsDiscardPile;
						_cardsDiscardPile.clear();
					}

					int32 drawIndex = GameRand::Rand() % _cardsDrawPile.size();
					CardEnum cardEnum = _cardsDrawPile[drawIndex];
					_cardsDrawPile.erase(_cardsDrawPile.begin() + drawIndex);

					// If it is not a bought unique card, discard it
					if (IsTownSlotCard(cardEnum) &&
						(HasBoughtCard(cardEnum) || _simulation->TownhallCardCountAll(_playerId, cardEnum) > 0))
					{
						_cardsDiscardPile.push_back(cardEnum);
					}
					// If condition isn't met for this card, discard it
					else if (cardConditionNotMet(cardEnum))
					{
						_cardsDiscardPile.push_back(cardEnum);
					}
					else {
						_cardsHand.push_back(cardEnum);
						break;
					}
				}

				PUN_CHECK(loop > 1);
			}
		}

		PUN_CHECK(_cardsHand.size() == drawCount);

		_cardsHand1Reserved.clear();
		_cardsHand1Reserved.resize(_cardsHand.size(), false);

//#if WITH_EDITOR
//		auto printCards = [&](std::vector<BuildingEnum> cards) {
//			PUN_LOG("Pile");
//			for (int i = 0; i < cards.size(); i++) {
//				PUN_LOG("Card:%d building:%s", i, *ToFString(GetBuildingInfo(cards[i]).name));
//			}
//		};
//		printCards(_cardsDrawPile);
//		printCards(_cardsHand);
//		printCards(_cardsDiscardPile);
//#endif

		if (rerollButton) {
			justRerollButtonPressed = true;
		}
		needHand1Refresh = true;

		_rerollCountThisRound++;
	}

	/*
	 * Rare cards prize
	 */
	void RollRareHand(RareHandEnum rareHandEnum, FText rareHandMessage, int32 objectId)
	{
		_rareHandsDataQueued.push_back({ rareHandEnum, rareHandMessage, objectId });
		
		TryRefreshRareHand();
	}
	bool CanSelectRareCardPrize(CardEnum cardEnum) {
		for (int i = 0; i < _cardsRareHand.size(); i++) {
			if (_cardsRareHand[i] == cardEnum) {
				return true;
			}
		}
		return false;
	}
	void DoneSelectRareHand() {
		_cardsRareHand.clear();
		_cardsRareHandReserved.clear();
	}

	FText rareHandMessage() {
		return _rareHandData.message;
	}
	FText rareHandMessage2();


	bool IsCardStackBlank() { return _isCardStackBlank; }
	void SetCardStackBlank(bool isCardStackBlank) { _isCardStackBlank = isCardStackBlank; }

	// while command is being verified across the network, don't allow further actions
	bool IsPendingCommand() { return _isPendingCommand; }
	void SetPendingCommand(bool isPendingCommand) { _isPendingCommand = isPendingCommand; }

	const std::vector<CardEnum>& GetHand() { return _cardsHand; }
	CardEnum GetCardEnumFromHand(int32_t cardIndex) { return _cardsHand[cardIndex]; }

	const std::vector<CardEnum>& GetRareHand() {
		return _cardsRareHand;
	}
	
	RareHandEnum GetRareHandEnum() { return _rareHandData.rareHandEnum; }
	int32 GetRareHandObjectId() { return _rareHandData.objectId; }
	
	/*
	 * CardStatusTracking... (!!!Not over network)
	 */
	// Card status tracking can't be done in UI since it is refreshed.
	// So it is done here...
	std::vector<bool> GetHand1ReserveStatus() { return _cardsHand1Reserved; }
	bool GetHand1ReserveStatus(int32 cardIndex) { return _cardsHand1Reserved[cardIndex]; }
	void SetHand1CardReservation(int32 cardIndex, bool isReserved) {
		PUN_CHECK(cardIndex < _cardsHand1Reserved.size());
		_cardsHand1Reserved[cardIndex] = isReserved;
	}
	void UnreserveIfRanOutOfCash(int32 negativeTentativeMoney)
	{
		for (size_t i = 0; i < _cardsHand1Reserved.size(); i++) {
			if (_cardsHand1Reserved[i]) {
				_cardsHand1Reserved[i] = false;
				negativeTentativeMoney += GetCardPrice(_cardsHand[i]);
				if (negativeTentativeMoney >= 0) {
					break;
				}
			}
		}
	}
	void ClearHand1CardReservation() {
		for (size_t i = _cardsHand1Reserved.size(); i-- > 0;) {
			_cardsHand1Reserved[i] = false;
		}
	}
	int32 GetUnboughtFreeMoney()
	{
		int32 money = 0;
		for (size_t i = _cardsHand1Reserved.size(); i-- > 0;) {
			if (!_cardsHand1Reserved[i]) {
				money += 5;
			}
		}
		return money;
	}

	 /*
	  * Rare cards prize reservation (!!!Not over network)
	  */
	std::vector<bool> GetRareHandReserveStatus() { return _cardsRareHandReserved; }
	bool GetRareHandReserveStatus(int32_t cardIndex) { return _cardsRareHandReserved[cardIndex]; }
	void SetRareHandCardReservation(int32_t cardIndex, bool isReserved) {
		PUN_CHECK(cardIndex < _cardsRareHandReserved.size());
		_cardsRareHandReserved[cardIndex] = isReserved;
	}


	

	/*
	 * Buy card from bottom row...
	 */
	void AddCardToHand2(CardEnum buildingEnum, bool isFromBuying = false)
	{
		int32 cardBoughtIndex = -1;
		for (size_t i = 0; i < _cardsBought.size(); i++) {
			if (_cardsBought[i].cardEnum == buildingEnum) {
				cardBoughtIndex = i;
				_cardsBought[i].stackSize++;
			}
		}

		// Didn't find card, add card
		if (cardBoughtIndex == -1) {
			cardBoughtIndex = _cardsBought.size();

			CardStatus cardStatus;
			cardStatus.cardEnum = buildingEnum;
			cardStatus.cardBirthTicks = Time::Ticks();
			
			_cardsBought.push_back(cardStatus);
		}

		// Card combining
		if (buildingEnum == CardEnum::ShrineGreedPiece) {
			if (_cardsBought[cardBoughtIndex].stackSize >= 3) {
				UseBoughtCard(buildingEnum, 3);
				AddCardToHand2(CardEnum::ShrineGreed);
			}
		}

		if (isFromBuying) {
			_alreadyBoughtCardThisRound = true;
		}
	}

	const std::vector<CardStatus>& GetCardsBought() {
		return _cardsBought;
	}
	std::vector<CardStatus> GetCardsBoughtAndInventory()
	{
		std::vector<CardStatus> cards = GetCardsBought();
		cards.insert(cards.begin(), _cardsInventory.begin(), _cardsInventory.end());
		return cards;
	}

	std::vector<CardStatus> GetCardsBought_Display() {
		std::vector<CardStatus> cardsBought = _cardsBought;
		RemoveCards(cardsBought, pendingMilitarySlotCards);
		return cardsBought;
	}

	static const int32 maxCardsBought = 7;
	bool CanAddCardToBoughtHand(CardEnum buildingEnum, int32 additionalCards, bool checkHand1Reserved = false)
	{
		// Get the current stack
		std::vector<CardStatus> cardStacksFinal = GetCardsBought();

		// Draw Hand case: include all reserved cards
		if (checkHand1Reserved)
		{
			// Add Reserved Cards for final cardStacks calculation
			check(_cardsHand1Reserved.size() <= _cardsHand.size());
			for (size_t i = 0; i < _cardsHand1Reserved.size(); i++) {
				if (_cardsHand1Reserved[i]) {
					AddToCardStacksHelper(_cardsHand[i], 1, cardStacksFinal);
				}
			}
		}

		// Normal case, we just try to add cards to the cardStacksFinal
		AddToCardStacksHelper(buildingEnum, additionalCards, cardStacksFinal);
		
		return cardStacksFinal.size() <= maxCardsBought;
	}

	void AddToCardStacksHelper(CardEnum buildingEnum, int32 additionalCards, std::vector<CardStatus>& cardStacks)
	{
		for (int32 i = cardStacks.size(); i-- > 0;) {
			if (buildingEnum == cardStacks[i].cardEnum) {
				cardStacks[i].stackSize += additionalCards;
				return;
			}
		}
		
		CardStatus cardStatus;
		cardStatus.cardEnum = buildingEnum;
		cardStatus.stackSize = additionalCards;
		
		cardStacks.push_back(cardStatus);
	}
	

	bool TryAddCardToBoughtHand(CardEnum cardEnum, int32 cardCount = 1)
	{
		if (CanAddCardToBoughtHand(cardEnum, cardCount)) {
			AddCardToHand2(cardEnum);
			return true;
		}
		return false;
	}

	//bool TryAddCardStatusToBoughtHand(CardStatus cardStatus)
	//{
	//	if (maxCardsBought > _cardsBought.size()) {
	//		_cardsBought.push_back(cardStatus);
	//		return true;
	//	}
	//	return false;
	//}

	CardStatus GetActualBoughtCardStatus(const CardStatus& cardStatusIn)
	{
		for (size_t i = _cardsBought.size(); i-- > 0;) {
			if (_cardsBought[i].cardEnum == cardStatusIn.cardEnum &&
				_cardsBought[i].cardBirthTicks == cardStatusIn.cardBirthTicks) {
				return _cardsBought[i];
			}
		}
		return CardStatus::None;
	}

	int32 BoughtCardCount(CardEnum cardEnum, bool includeInventory = true)
	{
		for (size_t i = _cardsBought.size(); i-- > 0;) {
			if (_cardsBought[i].cardEnum == cardEnum) {
				return _cardsBought[i].stackSize;
			}
		}

		if (includeInventory) {
			for (size_t i = _cardsInventory.size(); i-- > 0;) {
				if (_cardsInventory[i].cardEnum == cardEnum) {
					return _cardsInventory[i].stackSize;
				}
			}
		}
		
		return 0;
	}
	bool HasBoughtCard(CardEnum cardEnum) { return BoughtCardCount(cardEnum) > 0; }

	bool HasBoughtMilitaryCard()
	{
		for (int32 i = static_cast<int>(MilitaryCardEnumMin); i < static_cast<int>(MilitaryCardEnumMax); i++) {
			if (HasBoughtCard(static_cast<CardEnum>(i))) {
				return true;
			}
		}
		return false;
	}

	

	int32 DisplayedBoughtCardCount(CardEnum cardEnum, bool includeInventory = true)
	{
		int32 boughtCardCount = BoughtCardCount(cardEnum, includeInventory);
		for (int32 i = 0; i < pendingMilitarySlotCards.size(); i++) {
			if (pendingMilitarySlotCards[i].cardEnum == cardEnum) {
				return boughtCardCount - pendingMilitarySlotCards[i].stackSize;
			}
		}
		return boughtCardCount;
	}
	

	bool CanUseBoughtCard(CardEnum buildingEnum, int32 numberOfCards = 1)
	{
		for (size_t i = _cardsBought.size(); i-- > 0;) {
			if (_cardsBought[i].cardEnum == buildingEnum &&
				_cardsBought[i].stackSize >= numberOfCards) 
			{
				return true;
			}
		}
		return false;
	}
	void UseBoughtCard(CardEnum buildingEnum, int32 numberOfCards) {
		for (size_t i = _cardsBought.size(); i-- > 0;) {
			if (_cardsBought[i].cardEnum == buildingEnum) {
				_cardsBought[i].stackSize -= numberOfCards;
				PUN_CHECK(_cardsBought[i].stackSize >= 0);
				if (_cardsBought[i].stackSize == 0) {
					_cardsBought.erase(_cardsBought.begin() + i);
				}
				return;
			}
		}
		UE_DEBUG_BREAK();
	}

	
	int32 RemoveCardsOld(CardEnum buildingEnum, int32 sellStackSize)
	{
		for (size_t i = _cardsBought.size(); i-- > 0;) 
		{
			if (_cardsBought[i].cardEnum == buildingEnum) {
				int32_t actualSellStackSize = std::min(_cardsBought[i].stackSize,  sellStackSize);
				_cardsBought[i].stackSize -= actualSellStackSize;
				if (_cardsBought[i].stackSize == 0) {
					_cardsBought.erase(_cardsBought.begin() + i);
				}
				return actualSellStackSize;
			}
		}

		// Sell command could be a duplicate, in that case return 1
		return -1;
	}

	//! Check cardBirthTicks
	int32 RemoveCardsFromBoughtHand(CardStatus cardStatus, int32 removeCount)
	{
		for (size_t i = _cardsBought.size(); i-- > 0;)
		{
			if (_cardsBought[i].cardEnum == cardStatus.cardEnum) // &&
				//_cardsBought[i].cardBirthTicks == cardStatus.cardBirthTicks)
			{
				int32_t actualSellStackSize = std::min(_cardsBought[i].stackSize, removeCount);
				_cardsBought[i].stackSize -= actualSellStackSize;
				if (_cardsBought[i].stackSize == 0) {
					_cardsBought.erase(_cardsBought.begin() + i);
				}
				return actualSellStackSize;
			}
		}

		// Removal failed
		return -1;
	}


	void AddCards_BoughtHandAndInventory(CardStatus cardStatus)
	{
		auto tryAddCards = [&](std::vector<CardStatus>& cardStatuses, int32 maxCardCount)
		{
			for (size_t i = cardStatuses.size(); i-- > 0;) {
				if (cardStatuses[i].cardEnum == cardStatus.cardEnum) {
					cardStatuses[i].stackSize += cardStatus.stackSize;
					cardStatus.stackSize = 0;
					return true;
				}
			}

			if (cardStatuses.size() < maxCardCount) {
				cardStatuses.push_back(cardStatus);
			}
			
			return false;
		};

		if (tryAddCards(_cardsBought, maxCardsBought)) {
			return;
		}
		tryAddCards(_cardsInventory, maxCardInventorySlots());
	}
	
	void RemoveCards_BoughtHandAndInventory(CardStatus cardStatus)
	{
		auto removeCards = [&](std::vector<CardStatus>& cardStatuses)
		{
			for (size_t i = cardStatuses.size(); i-- > 0;)
			{
				if (cardStatuses[i].cardEnum == cardStatus.cardEnum)
				{
					int32_t actualSellStackSize = std::min(cardStatuses[i].stackSize, cardStatus.stackSize);
					cardStatuses[i].stackSize -= actualSellStackSize;
					cardStatus.stackSize -= actualSellStackSize;
					if (cardStatuses[i].stackSize == 0) {
						cardStatuses.erase(cardStatuses.begin() + i);
					}
				}

				if (cardStatus.stackSize <= 0) {
					return;
				}
			}
		};
		
		removeCards(_cardsBought);
		removeCards(_cardsInventory);
	}

	static void RemoveCards(CardStatus cardStatus, std::vector<CardStatus>& cardStatuses)
	{
		for (size_t i = cardStatuses.size(); i-- > 0;)
		{
			if (cardStatuses[i].cardEnum == cardStatus.cardEnum)
			{
				int32_t actualSellStackSize = std::min(cardStatuses[i].stackSize, cardStatus.stackSize);
				cardStatuses[i].stackSize -= actualSellStackSize;
				if (cardStatuses[i].stackSize == 0) {
					cardStatuses.erase(cardStatuses.begin() + i);
				}
				return;
			}
		}
	}

	
	void ClearBoughtCards() {
		_cardsBought.clear();
	}
	void ClearRareHands() {
		_rareHandsDataQueued.clear();
		TryRefreshRareHand();
	}

	void AddDrawCards(CardEnum buildingEnum, int cardCount = 1) {
		PUN_CHECK(cardCount > 0);
		for (int i = 0; i < cardCount; i++) {
			_cardsDrawPile.push_back(buildingEnum);
		}
	}

	/*
	 * Card Inventory
	 */
	int32 maxCardInventorySlots()
	{
		if (_simulation->IsResearched(_playerId, TechEnum::CardInventory2)) {
			return 14;
		}
		if (_simulation->IsResearched(_playerId, TechEnum::CardInventory1)) {
			return 7;
		}
		return 0;
	}

	bool CanAddCardsToCardInventory(CardEnum cardEnum)
	{
		if (_cardsInventory.size() < maxCardInventorySlots()) {
			return true;
		}
		for (size_t i = _cardsInventory.size(); i-- > 0;) 
		{
			if (_cardsInventory[i].cardEnum == cardEnum) {
				return true;
			}
		}
		return false;
	}

	const std::vector<CardStatus>& cardInventory() { return _cardsInventory; }
	
	void MoveHandToCardInventory(CardStatus cardStatus, int32 targetCount)
	{
		int32 removedCount = RemoveCardsFromBoughtHand(cardStatus, targetCount);

		if (removedCount > 0)
		{
			// Put into existing stack
			for (size_t i = _cardsInventory.size(); i-- > 0;)
			{
				if (_cardsInventory[i].cardEnum == cardStatus.cardEnum) {
					_cardsInventory[i].stackSize += removedCount;
					return;
				}
			}

			cardStatus.stackSize = removedCount;
			_cardsInventory.push_back(cardStatus);
		}
	}

	void MoveCardInventoryToHand(CardStatus cardStatus, int32 targetCount)
	{
		if (!CanAddCardToBoughtHand(cardStatus.cardEnum, targetCount)) {
			_simulation->AddPopupToFront(_playerId,
				NSLOCTEXT("CardSys", "ReachedHandLimit_InventoryToHand_Pop", "Reached hand limit."),
				ExclusiveUIEnum::CardInventory, "PopupCannot"
			);
			return;
		}

		int32 actualCount = targetCount;

		for (size_t i = _cardsInventory.size(); i-- > 0;)
		{
			if (_cardsInventory[i].cardEnum == cardStatus.cardEnum)
			{
				actualCount = std::min(_cardsInventory[i].stackSize, actualCount);

				_cardsInventory[i].stackSize -= actualCount;
				if (_cardsInventory[i].stackSize <= 0) {
					_cardsInventory.erase(_cardsInventory.begin() + i);
				}
				break;
			}
		}

		if (actualCount > 0) {
			TryAddCardToBoughtHand(cardStatus.cardEnum, actualCount);
		}
	}

	/*
	 * Card Removal
	 */
	void RemoveDrawCards(CardEnum cardEnum)
	{
		auto loop = [&](std::vector<CardEnum>& cards) {
			for (size_t i = cards.size(); i-- > 0;) {
				if (cards[i] == cardEnum) {
					cards.erase(cards.begin() + i);
					_cardsRemovedPile.push_back(cardEnum);
				}
			}
		};
		
		loop(_cardsHand);
		loop(_cardsDrawPile);
		loop(_cardsDiscardPile);
	}


	bool ReturnBuildingSlotCardsToHand(const std::vector<CardStatus>& slotCards)
	{
		// Note: Still need to ResetCardSlots after
		// Return cards to hand
		std::vector<CardEnum> addedCards;

		for (CardStatus card : slotCards)
		{
			if (CanAddCardToBoughtHand(card.cardEnum, 1))
			{
				addedCards.push_back(card.cardEnum);
				AddCardToHand2(card.cardEnum);
			}
			else {
				// Remove the added cards from the Hand (added on previous iterations)
				for (CardEnum addedCard : addedCards) {
					RemoveCardsOld(addedCard, 1);
				}

				return false;
			}
		}

		return true;
	}

	/*
	 * 
	 */
	static void RemoveCards(std::vector<CardStatus>& cardStatus, const std::vector<CardStatus>& cardsToRemove)
	{
		for (int32 i = 0; i < cardStatus.size(); i++) {
			for (int32 j = 0; j < cardsToRemove.size(); j++) {
				if (cardStatus[i].cardEnum == cardsToRemove[j].cardEnum) {
					cardStatus[i].stackSize -= cardsToRemove[j].stackSize;
					check(cardStatus[i].stackSize >= 0);
				}
			}
		}
	}
	

	/*
	 * Serialize
	 */
	void Serialize(FArchive& Ar)
	{
		Ar << justRerollButtonPressed;

		Ar << justRerolledRareHand;

		Ar << converterCardState;
		Ar << wildCardEnumUsed;

		/*
		 * Private
		 */
		SerializeVecBool(Ar, _cardsHand1Reserved);
		SerializeVecBool(Ar, _cardsRareHandReserved);

		SerializeVecValue(Ar, _cardsDrawPile);
		SerializeVecValue(Ar, _cardsHand);
		SerializeVecValue(Ar, _cardsDiscardPile);

		SerializeVecValue(Ar, _cardsRemovedPile);

		SerializeVecObj(Ar, _cardsBought);

		SerializeVecObj(Ar, _cardsInventory);

		
		SerializeVecValue(Ar, _cardsRareHand);
		_rareHandData >> Ar;
		SerializeVecObj(Ar, _rareHandsDataQueued);

		Ar << _rerollCountThisRound;
		Ar << _alreadyBoughtCardThisRound;

		Ar << _isCardStackBlank;
		Ar << _isPendingCommand;

		Ar << _cardHandQueueCount;

		PUN_CHECK(_cardsRareHand.size() == _cardsRareHandReserved.size());
	}

	struct RareHandData
	{
		RareHandEnum rareHandEnum;
		FText message;
		int32 objectId = -1;
		
		void operator>>(FArchive &Ar) {
			Ar << rareHandEnum;
			Ar << message;
			Ar << objectId;
		}
	};

private:

	void RollRareHandExecute();

	void RandomInsertToRareHand(const std::vector<CardEnum>& cardEnums, int32 cardCount = 3)
	{
		LOOP_CHECK_START();
		while (_cardsRareHand.size() < cardCount)
		{
			CardEnum cardEnum = cardEnums[GameRand::Rand() % cardEnums.size()];
			if (!CppUtils::Contains(_cardsRareHand, cardEnum)) {
				_cardsRareHand.push_back(cardEnum);
			}

			LOOP_CHECK_END();
		}
	}

public:
	/*
	 * Serialize
	 */
	bool justRerollButtonPressed = false;

	bool justRerolledRareHand = false;
	bool justRerolledClaimLandHand = false;

	ConverterCardUseState converterCardState = ConverterCardUseState::None;
	CardEnum wildCardEnumUsed = CardEnum::None;

	/*
	 * Non-Serialize
	 */
	bool allowMaxCardHandQueuePopup = true;

	bool needHand1Refresh = false;

	std::vector<CardStatus> pendingMilitarySlotCards;
	
private:
	IGameSimulationCore* _simulation = nullptr;
	int32 _playerId = -1;

	/*
	 * Serialize
	 */
	// Keep track of the card bought (so it can be hidden)
	std::vector<bool> _cardsHand1Reserved;
	std::vector<bool> _cardsRareHandReserved;

	std::vector<CardEnum> _cardsDrawPile;
	std::vector<CardEnum> _cardsHand;
	std::vector<CardEnum> _cardsDiscardPile;

	std::vector<CardEnum> _cardsRemovedPile;

	std::vector<CardStatus> _cardsBought;

	std::vector<CardStatus> _cardsInventory;
	

	std::vector<CardEnum> _cardsRareHand;
	RareHandData _rareHandData;
	std::vector<RareHandData> _rareHandsDataQueued;

	int32 _rerollCountThisRound = 0;
	bool _alreadyBoughtCardThisRound = false; // If cards was not bought yet this round, gain 1 free reroll

	bool _isCardStackBlank;
	bool _isPendingCommand = false;

	int32 _cardHandQueueCount = 0;

};
