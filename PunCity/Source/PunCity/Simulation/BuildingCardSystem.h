// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "IGameSimulationCore.h"
#include "PunCity/PunUtils.h"
#include "PunCity/CppUtils.h"

struct BuildingCardStack
{
	CardEnum buildingEnum = CardEnum::None;
	int32 stackSize = -1;
	int32 cardStateValue = 0;

	bool operator==(const BuildingCardStack& a) const {
		return buildingEnum == a.buildingEnum && stackSize == a.stackSize;
	}

	void operator>>(FArchive& Ar)
	{
		Ar << buildingEnum;
		Ar << stackSize;
		Ar << cardStateValue;
	}
};

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

			AddDrawCards(CardEnum::CoalMine, 1);

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
		if (_simulation->buildingFinishedCount(_playerId, CardEnum::CensorshipInstitute)) {
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
		if (_rareHandsEnumQueued.size() > 0 && _cardsRareHand.size() == 0) {
			_rareHandEnum = _rareHandsEnumQueued[0];
			_rareHandMessage = _rareHandsMessageQueued[0];
			_rareHandsEnumQueued.erase(_rareHandsEnumQueued.begin());
			_rareHandsMessageQueued.erase(_rareHandsMessageQueued.begin());
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
					if (IsGlobalSlotCard(cardEnum) &&
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
	void RollRareHand(RareHandEnum rareHandEnum, FText rareHandMessage)
	{
		_rareHandsEnumQueued.push_back(rareHandEnum);
		_rareHandsMessageQueued.push_back(rareHandMessage);
		
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
		return _rareHandMessage;
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
			if (_cardsBought[i].buildingEnum == buildingEnum) {
				cardBoughtIndex = i;
				_cardsBought[i].stackSize++;
			}
		}

		// Didn't find card, add card
		if (cardBoughtIndex == -1) {
			cardBoughtIndex = _cardsBought.size();
			_cardsBought.push_back({ buildingEnum, 1 });
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

	std::vector<BuildingCardStack> GetCardsBought() {
		return _cardsBought;
	}

	// TODO: wtf need to sort this out
	bool CanAddCardToBoughtHand(CardEnum buildingEnum, int32 additionalCards, bool checkHand1Reserved = false)
	{
		const int32 maxCardsBought = 7;

		std::vector<BuildingCardStack> cardStacksFinal = GetCardsBought();

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

		AddToCardStacksHelper(buildingEnum, additionalCards, cardStacksFinal);
		
		return cardStacksFinal.size() <= maxCardsBought;
	}

	void AddToCardStacksHelper(CardEnum buildingEnum, int32 additionalCards, std::vector<BuildingCardStack>& cardStacks)
	{
		for (int32 i = cardStacks.size(); i-- > 0;) {
			if (buildingEnum == cardStacks[i].buildingEnum) {
				cardStacks[i].stackSize += additionalCards;
				return;
			}
		}
		cardStacks.push_back({ buildingEnum, additionalCards, 0 });
	}
	

	bool TryAddCardToBoughtHand(CardEnum cardEnum, int32 cardCount = 1)
	{
		if (CanAddCardToBoughtHand(cardEnum, cardCount)) {
			AddCardToHand2(cardEnum);
			return true;
		}
		return false;
	}
	

	int32 BoughtCardCount(CardEnum cardEnum) {
		for (size_t i = _cardsBought.size(); i-- > 0;) {
			if (_cardsBought[i].buildingEnum == cardEnum) {
				return _cardsBought[i].stackSize;
			}
		}
		return 0;
	}
	bool HasBoughtCard(CardEnum cardEnum) { return BoughtCardCount(cardEnum) > 0; }

	bool CanUseBoughtCard(CardEnum buildingEnum, int32 numberOfCards = 1)
	{
		for (size_t i = _cardsBought.size(); i-- > 0;) {
			if (_cardsBought[i].buildingEnum == buildingEnum &&
				_cardsBought[i].stackSize >= numberOfCards) 
			{
				return true;
			}
		}
		return false;
	}
	void UseBoughtCard(CardEnum buildingEnum, int32 numberOfCards) {
		for (size_t i = _cardsBought.size(); i-- > 0;) {
			if (_cardsBought[i].buildingEnum == buildingEnum) {
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

	int32 RemoveCards(CardEnum buildingEnum, int32 sellStackSize)
	{
		for (size_t i = _cardsBought.size(); i-- > 0;) {
			if (_cardsBought[i].buildingEnum == buildingEnum) {
				int32_t actualSellStackSize = std::min(_cardsBought[i].stackSize,  sellStackSize);
				_cardsBought[i].stackSize -= actualSellStackSize;
				if (_cardsBought[i].stackSize == 0) {
					_cardsBought.erase(_cardsBought.begin() + i);
				}
				return actualSellStackSize * GetCardPrice(buildingEnum);
			}
		}

		// Sell command could be a duplicate, in that case return 1
		return -1;
	}
	void ClearBoughtCards() {
		_cardsBought.clear();
	}
	void ClearRareHands() {
		_rareHandsEnumQueued.clear();
		_rareHandsMessageQueued.clear();
		TryRefreshRareHand();
	}

	void AddDrawCards(CardEnum buildingEnum, int cardCount = 1) {
		PUN_CHECK(cardCount > 0);
		for (int i = 0; i < cardCount; i++) {
			_cardsDrawPile.push_back(buildingEnum);
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
	

	/*
	 * Serialize
	 */
	void Serialize(FArchive& Ar)
	{
		Ar << justRerollButtonPressed;
		Ar << needHand1Refresh;

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


		
		SerializeVecValue(Ar, _cardsRareHand);
		Ar << _rareHandMessage;
		Ar << _rareHandEnum;
		SerializeVecLoop(Ar, _rareHandsMessageQueued, [&](FText& text) {
			Ar << text;
		});
		SerializeVecValue(Ar, _rareHandsEnumQueued);

		Ar << _rerollCountThisRound;
		Ar << _alreadyBoughtCardThisRound;

		Ar << _isCardStackBlank;
		Ar << _isPendingCommand;

		Ar << _cardHandQueueCount;

		PUN_CHECK(_cardsRareHand.size() == _cardsRareHandReserved.size());
	}

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
	bool needHand1Refresh = false;

	bool justRerolledRareHand = false;
	bool justRerolledClaimLandHand = false;

	ConverterCardUseState converterCardState = ConverterCardUseState::None;
	CardEnum wildCardEnumUsed = CardEnum::None;

	/*
	 * Non-Serialize
	 */
	bool allowMaxCardHandQueuePopup = true;
	
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

	std::vector<BuildingCardStack> _cardsBought;

	std::vector<CardEnum> _cardsRareHand;
	FText _rareHandMessage;
	RareHandEnum _rareHandEnum;
	std::vector<FText> _rareHandsMessageQueued;
	std::vector<RareHandEnum> _rareHandsEnumQueued;

	int32 _rerollCountThisRound = 0;
	bool _alreadyBoughtCardThisRound = false; // If cards was not bought yet this round, gain 1 free reroll

	bool _isCardStackBlank;
	bool _isPendingCommand = false;

	int32 _cardHandQueueCount = 0;

};
