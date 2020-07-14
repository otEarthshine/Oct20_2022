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
		if (TownhallCardCount(CardEnum::IndustrialRevolution) && 
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

	void TickRound() {
		_rerollCountThisRound = 0;
		_alreadyBoughtCardThisRound = false;
		
		RollHand(handSize());

		//if (Time::IsSpringStart() && Time::Years() > 0) {
		//	RollRareHand();
		//}
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
		//pile.insert(pile.end(), _cardsInTownhall.begin(), _cardsInTownhall.end());
		// TODO: eventually has _cardsDisabled for those hidden by buildings
		return pile;
	}

	bool HasCardInPile(CardEnum cardEnumIn)
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
						(HasBoughtCard(cardEnum) || TownhallCardCount(cardEnum) > 0))
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
	 * Townhall cards
	 */
	
	bool CanAddCardToTownhall() {
		return _cardsInTownhall.size() < _simulation->townLvl(_playerId);
	}

	void AddCardToTownhall(CardStatus card) {
		PUN_CHECK(!IsActionCard(card.cardEnum) && !IsBuildingCard(card.cardEnum));
		_cardsInTownhall.push_back(card);

		// Some cards may trigger refresh etc...
		if (card.cardEnum == CardEnum::IndustrialRevolution) {
			needHand1Refresh = true;
		}
	}

	CardEnum RemoveCardFromTownhall(int32 slotIndex) {
		if (slotIndex < _cardsInTownhall.size()) {
			CardEnum cardEnum = _cardsInTownhall[slotIndex].cardEnum;
			_cardsInTownhall.erase(_cardsInTownhall.begin() + slotIndex);
			return cardEnum;
		}
		return CardEnum::None;
	}

	bool TownhallHasCard(CardEnum cardEnum)
	{
		for (size_t i = 0; i < _cardsInTownhall.size(); i++) {
			if (_cardsInTownhall[i].cardEnum == cardEnum) {
				return true;
			}
		}
		return false;
	}

	std::vector<CardStatus> cardsInTownhall() {
		return _cardsInTownhall;
	}

	int32 maxTownhallCards() {
		return _simulation->townLvl(_playerId);
	}

	int32 TownhallCardCount(CardEnum cardEnum)
	{
		int32 count = 0;
		for (size_t i = 0; i < _cardsInTownhall.size(); i++) {
			if (_cardsInTownhall[i].cardEnum == cardEnum) {
				count++;
			}
		}
		return count;
	}

	/*
	 * Rare cards prize
	 */
	void RollRareHand(RareHandEnum rareHandEnum, std::string rareHandMessage)
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

	std::string rareHandMessage() {
		return _rareHandMessage;
	}
	std::string rareHandMessage2() {
		if (_rareHandEnum == RareHandEnum::InitialCards1 ||
			_rareHandEnum == RareHandEnum::InitialCards2) 
		{
			return "CHOOSE A CARD";
		}
		if (_rareHandEnum == RareHandEnum::BuildingSlotCards) {
			return "CHOOSE YOUR PRIZE !";
		}
		return "CHOOSE YOUR RARE CARD PRIZE !";
	}



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

	bool CanAddCardToBoughtHand(CardEnum buildingEnum, int32 additionalCards)
	{
		const int32 maxCardsBought = 7;
		if (GetCardsBought().size() + additionalCards > maxCardsBought)
		{
			for (size_t i = _cardsBought.size(); i-- > 0;) {
				// TODO: properly work this out... CardSystem needs to know about how many stack of each lvl...
				// Can still add to 1/3, 2/3, not 3/3, 4/3, 5/3, not 6/3, 7/3, 8/3, not 9/3
				// Not 3, 6, 9, 12, 15
				if (_cardsBought[i].buildingEnum == buildingEnum) {
					if (_cardsBought[i].stackSize != 3 &&
						_cardsBought[i].stackSize != 6 && 
						_cardsBought[i].stackSize != 9 && 
						_cardsBought[i].stackSize != 12 && 
						_cardsBought[i].stackSize != 15) {
						return true;
					}
				}
			}
			return false;
		}
		return true;
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

	void AddDrawCards(CardEnum buildingEnum, int cardCount = 1) {
		PUN_CHECK(cardCount > 0);
		for (int i = 0; i < cardCount; i++) {
			_cardsDrawPile.push_back(buildingEnum);
		}
	}

	/*
	 * Card Removal
	 */
	void RemoveDrawCards(CardEnum buildingEnum) {
		CppUtils::RemoveAll(_cardsHand, buildingEnum);
		CppUtils::RemoveAll(_cardsDrawPile, buildingEnum);
		CppUtils::RemoveAll(_cardsDiscardPile, buildingEnum);
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

		SerializeVecObj(Ar, _cardsBought);

		// Ensure cards are not animated
		//if (Ar.IsSaving()) {
		//	for (CardStatus& card : _cardsInTownhall) {
		//		card.ResetAnimation();
		//	}
		//}
		SerializeVecObj(Ar, _cardsInTownhall);

		//PUN_LOG("_cardsInTownhall id:%d size:%d", _playerId, _cardsInTownhall.size());
		//for (CardStatus& card : _cardsInTownhall) {
		//	PUN_LOG(" -- card:%d anim:%d last:%d,%d", static_cast<int>(card.cardEnum), card.animationStartTime100, card.lastPositionX100, card.lastPositionY100);
		//}
		
		SerializeVecValue(Ar, _cardsRareHand);
		SerializeStr(Ar, _rareHandMessage);
		Ar << _rareHandEnum;
		SerializeVecLoop(Ar, _rareHandsMessageQueued, [&](std::string& str) {
			SerializeStr(Ar, str);
		});
		SerializeVecValue(Ar, _rareHandsEnumQueued);

		Ar << _rerollCountThisRound;
		Ar << _alreadyBoughtCardThisRound;

		Ar << _isCardStackBlank;
		Ar << _isPendingCommand;

		PUN_CHECK(_cardsRareHand.size() == _cardsRareHandReserved.size());
	}

private:

	void RollRareHandExecute()
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
				CardEnum::HerbSeed,
				CardEnum::ProductivityBook,
				CardEnum::IronSmelter,
			};
			for (CardEnum cardEnum : cardEnums) {
				_cardsRareHand.push_back(cardEnum);
			}
			//RandomInsertToRareHand(cardEnums, 3);

			std::stringstream ss;
			ss << "Would you advices?";
			PopupInfo popup(_playerId, ss.str(), { "Please advise me", "I already know what to do" }, PopupReceiverEnum::StartGame_AskAboutAdvice, true);
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
			_rareHandMessage = std::to_string(population) + " people now call your " + _simulation->townSizeName(_playerId) + " home!";

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

	std::vector<BuildingCardStack> _cardsBought;
	std::vector<CardStatus> _cardsInTownhall;

	std::vector<CardEnum> _cardsRareHand;
	std::string _rareHandMessage;
	RareHandEnum _rareHandEnum;
	std::vector<std::string> _rareHandsMessageQueued;
	std::vector<RareHandEnum> _rareHandsEnumQueued;

	int32 _rerollCountThisRound = 0;
	bool _alreadyBoughtCardThisRound = false; // If cards was not bought yet this round, gain 1 free reroll

	bool _isCardStackBlank;
	bool _isPendingCommand = false;
};
