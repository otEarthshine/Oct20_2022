// Pun Dumnernchanvanit's

#pragma once

#include "WorldTradeSystem.h"

DECLARE_CYCLE_STAT(TEXT("PUN: [TM]UpdateRelationship"), STAT_PunAIUpdateRelationship, STATGROUP_Game);

struct ProvinceClaimProgress
{
	ProvinceAttackEnum attackEnum = ProvinceAttackEnum::None;

	int32 provinceId = -1;
	int32 attackerPlayerId = -1;
	int32 defenderTownId = -1;

	std::vector<CardStatus> attackerFrontLine;
	std::vector<CardStatus> attackerBackLine;
	std::vector<CardStatus> defenderFrontLine;
	std::vector<CardStatus> defenderBackLine;

	std::vector<CardStatus> defenderWall;
	std::vector<CardStatus> defenderTreasure;

	std::vector<CardStatus> damageHitQueue; // cardStateValue2 as activation tick

	bool isCrossingRiver = false;
	int32 raidMoney = 0;

	int32 attacker_attackBonus = 0;
	int32 attacker_defenseBonus = 0;
	int32 defender_attackBonus = 0;
	int32 defender_defenseBonus = 0;

	int32 battleFinishCountdownTicks = -1; // Countdown after Finishing battle

	// Non-Serialized
	std::vector<CardStatus> attackerFrontLine_pendingRemoval;
	std::vector<CardStatus> attackerBackLine_pendingRemoval;
	std::vector<CardStatus> defenderFrontLine_pendingRemoval;
	std::vector<CardStatus> defenderBackLine_pendingRemoval;

	std::vector<CardStatus> defenderWall_pendingRemoval;
	std::vector<CardStatus> defenderTreasure_pendingRemoval;

	static const int32 AnimationLengthSecs = 3;
	static const int32 AnimationLengthTicks = AnimationLengthSecs * Time::TicksPerSecond;
	

	bool attackerWon() const {
		return battleFinishCountdownTicks == 0 && 
			defenderFrontLine.size() == 0 && 
			defenderBackLine.size() == 0 && 
			defenderWall.size() == 0 &&
			defenderTreasure.size() == 0;
	}
	bool attackerLost() const {
		return attackerFrontLine.size() == 0 && attackerBackLine.size() == 0;
	}
	bool isWaitingForBattleFinishCountdown() const {
		return (defenderFrontLine.size() == 0 && defenderBackLine.size() == 0) || 
				(attackerFrontLine.size() == 0 && attackerBackLine.size() == 0);
	}

	bool defenderStillAlive() const {
		return defenderFrontLine.size() > 0 ||
			defenderBackLine.size() > 0 ||
			defenderWall.size() > 0 ||
			defenderTreasure.size() > 0;
	}

	bool IsPlayerReinforcingAttacker(int32 playerId)
	{
		return IsPlayerReinforcingLine(playerId, attackerFrontLine) ||
				IsPlayerReinforcingLine(playerId, attackerBackLine);
	}
	bool IsPlayerReinforcingDefender(int32 playerId)
	{
		return IsPlayerReinforcingLine(playerId, defenderFrontLine) ||
			IsPlayerReinforcingLine(playerId, defenderBackLine);
	}

	static bool IsPlayerReinforcingLine(int32 playerId, const std::vector<CardStatus>& line)
	{
		for (const CardStatus& lineUnit : line) {
			if (lineUnit.cardStateValue1 == playerId) {
				return true;
			}
		}
		return false;
	}
	

	bool isValid() { return provinceId != -1; }


	static void PrepareCardStatusForMilitary(int32 playerId, std::vector<CardStatus>& militaryCards)
	{
		// cardStateValue1 = playerId, cardStateValue2 = HP
		for (CardStatus& cardStatus : militaryCards) {
			cardStatus.cardStateValue1 = playerId;
			cardStatus.cardStateValue2 = GetMilitaryInfo(cardStatus.cardEnum).hp100;
		}
	}

	void Reinforce(std::vector<CardStatus>& militaryCards, bool isReinforcingAttacker, int32 unitPlayerId);

	static void AddMilitaryCards(std::vector<CardStatus>& militaryCards, CardStatus militaryCard)
	{
		for (int32 i = 0; i < militaryCards.size(); i++) {
			if (militaryCards[i].cardEnum == militaryCard.cardEnum &&
				GetCardPlayer(militaryCards[i]) == GetCardPlayer(militaryCard))
			{
				militaryCards[i].stackSize += militaryCard.stackSize;
				return;
			}
		}
		militaryCards.push_back(militaryCard);
	}

	static int32 GetCardPlayer(const CardStatus& card) {
		return card.cardStateValue1;
	}
	

	void Retreat_DeleteUnits(int32 unitPlayerId);

	void Tick(IGameSimulationCore* simulation);

	template<typename Func>
	void ExecuteOnAllUnits(Func func) const
	{
		for (const CardStatus& card : attackerFrontLine) func(card);
		for (const CardStatus& card : attackerBackLine) func(card);
		for (const CardStatus& card : defenderFrontLine) func(card);
		for (const CardStatus& card : defenderBackLine) func(card);
	}

	template<typename Func>
	void ExecuteOnAll_PendingRemoval(Func func) const
	{
		for (const CardStatus& card : attackerFrontLine_pendingRemoval) func(card);
		for (const CardStatus& card : attackerBackLine_pendingRemoval) func(card);
		for (const CardStatus& card : defenderFrontLine_pendingRemoval) func(card);
		for (const CardStatus& card : defenderBackLine_pendingRemoval) func(card);
		
		for (const CardStatus& card : defenderWall_pendingRemoval) func(card);
		for (const CardStatus& card : defenderTreasure_pendingRemoval) func(card);
	}

	void RemoveExpired_PendingRemovalUnits()
	{
		auto removedExpired = [&](std::vector<CardStatus>& cards)
		{
			for (int32 i = cards.size(); i-- > 0;) {
				if (Time::Ticks() - cards[i].displayCardStateValue3 > AnimationLengthTicks) {
					cards.erase(cards.begin() + i);
				}
			}
		};
		removedExpired(attackerFrontLine_pendingRemoval);
		removedExpired(attackerBackLine_pendingRemoval);
		removedExpired(defenderFrontLine_pendingRemoval);
		removedExpired(defenderBackLine_pendingRemoval);

		removedExpired(defenderWall_pendingRemoval);
		removedExpired(defenderTreasure_pendingRemoval);
	}


	//! Serialize
	FArchive& operator>>(FArchive &Ar)
	{
		Ar << attackEnum;

		Ar << provinceId;
		Ar << attackerPlayerId;
		Ar << defenderTownId;

		SerializeVecObj(Ar, attackerFrontLine);
		SerializeVecObj(Ar, attackerBackLine);
		SerializeVecObj(Ar, defenderFrontLine);
		SerializeVecObj(Ar, defenderBackLine);

		SerializeVecObj(Ar, defenderWall);
		SerializeVecObj(Ar, defenderTreasure);

		SerializeVecObj(Ar, damageHitQueue);

		Ar << isCrossingRiver;
		Ar << raidMoney;
		
		Ar << attacker_attackBonus;
		Ar << attacker_defenseBonus;
		Ar << defender_attackBonus;
		Ar << defender_defenseBonus;

		Ar << battleFinishCountdownTicks;

		return Ar;
	}
};

struct AutoTradeElement
{
	bool isImport = true;
	ResourceEnum resourceEnum = ResourceEnum::None;
	int32 targetInventory = 0;
	int32 maxTradeAmount = 0;

	int32 calculatedTradeAmountNextRound = 0;
	int32 calculatedFulfilledTradeAmountNextRound = 0;

	int32 price100 = 0;
	int32 directTradeAmount = 0;
	int32 feePercent = 0;

	int32 calculatedMoney = 0;
	int32 calculatedFeeDiscount = 0;
	int32 calculatedFee = 0;

	int32 calculatedFulfillmentLeft() const {
		return calculatedTradeAmountNextRound - calculatedFulfilledTradeAmountNextRound;
	}

	// Import may not complete successfully due to not having enough storage a
	int64 GetImportLeftoverRefund100(int32 leftoverAmount) const
	{
		check(leftoverAmount >= 0);
		int64 previousNetMoney = calculatedMoney + calculatedFee;

		int64 newTradeAmount = calculatedTradeAmountNextRound - leftoverAmount;
		int64 newImportMoney = newTradeAmount * price100;
		int64 newFeeDiscount = std::min(static_cast<int64>(directTradeAmount), newTradeAmount) * price100 * feePercent / 100;
		int64 newFee = (newImportMoney * feePercent / 100) - newFeeDiscount;

		int64 newNetMoney = newImportMoney + newFee;

		return previousNetMoney - newNetMoney;
	}

	void CalculateMoney(int32 price100In, int32 directTradeAmountIn, int32 feePercentIn)
	{
		price100 = price100In;
		directTradeAmount = directTradeAmountIn;
		feePercent = feePercentIn;

		calculatedMoney = calculatedTradeAmountNextRound * price100;

		calculatedFeeDiscount = directTradeAmount * price100 * feePercent / 100;
		calculatedFee = (calculatedMoney * feePercent / 100) - calculatedFeeDiscount;
	}


	//! Serialize
	FArchive& operator>>(FArchive &Ar) {
		Ar << isImport;
		Ar << resourceEnum;
		Ar << targetInventory;
		Ar << maxTradeAmount;

		Ar << calculatedTradeAmountNextRound;
		Ar << calculatedFulfilledTradeAmountNextRound;

		Ar << price100;
		Ar << directTradeAmount;
		Ar << feePercent;

		Ar << calculatedMoney;
		Ar << calculatedFeeDiscount;
		Ar << calculatedFee;
		return Ar;
	}

	int32 GetHash() const
	{
		return FastHashCombineMany({
			isImport,
			static_cast<int32>(resourceEnum),
			targetInventory,
			maxTradeAmount,
			calculatedTradeAmountNextRound,
			calculatedFulfilledTradeAmountNextRound,
			});
	}
};

/*
 *
 */
class TownManagerBase
{
public:
	bool isValid() { return _townId != -1; }

	TownManagerBase(int32 playerId, int32 townId, FactionEnum factionEnum, IGameSimulationCore* simulation) :
		_playerId(playerId),
		_townId(townId),
		_factionEnum(factionEnum),
		_simulation(simulation)
	{
		_minorCityWealth = 500;
		//_minorCityTargetWealth = 0;
	}

	virtual ~TownManagerBase() {}

	virtual void Tick()
	{
		// Defense
		for (ProvinceClaimProgress& claimProgress : _defendingClaimProgress) {
			claimProgress.Tick(_simulation);
		}

		for (int32 i = _defendingClaimProgress_pendingRemoval.size(); i-- > 0;) {
			_defendingClaimProgress_pendingRemoval[i].battleFinishCountdownTicks--;
			if (_defendingClaimProgress_pendingRemoval[i].battleFinishCountdownTicks <= 0) {
				_defendingClaimProgress_pendingRemoval.erase(_defendingClaimProgress_pendingRemoval.begin() + i);
			}
		}
	}

	/*
	 * Get
	 */

	RelationshipModifiers& relationship() { return _relationships; }

	int32 playerId() { return _playerId; }

	int32 townId() { return _townId; }

	bool hasTownhall() { return townhallId != -1; }

	bool isCapital() { return _townId == _playerId; }

	FactionEnum factionEnum() const { return _factionEnum; }

	const std::vector<int32>& provincesClaimed() { return _provincesClaimed; }

	virtual int32 baseDefenderUnits() {
		return 3 * GetMinorCityLevel();
	}

	/*
	 *
	 */
	virtual void RecalculateTaxDelayed() {}

	void Tick1Sec_TownBase();

	/*
	 * Auto Trade
	 */

	std::vector<AutoTradeElement>& autoExportElements() { return _autoExportElements; }
	std::vector<AutoTradeElement>& autoImportElements() { return _autoImportElements; }

	const std::vector<AutoTradeElement>& autoExportElementsConst() { return _autoExportElements; }
	const std::vector<AutoTradeElement>& autoImportElementsConst() { return _autoImportElements; }

	void InitAutoTrade(int32 provinceId)
	{
		GeoresourceEnum georesourceEnum = _simulation->georesource(provinceId).georesourceEnum;

		AutoTradeElement element;
		int32 tradeAmount = 100;
		element.targetInventory = tradeAmount;
		element.maxTradeAmount = tradeAmount;
		element.calculatedTradeAmountNextRound = tradeAmount;

		// Usually export food, fuel, raw material
		{
			ResourceEnum chosenResourceEnum;

			if (georesourceEnum != GeoresourceEnum::None) {
				chosenResourceEnum = GeoresourceEnumToResourceEnum(georesourceEnum);
			}
			else {
				std::vector<ResourceEnum> exportEnums = FoodEnums_Input;
				exportEnums.push_back(ResourceEnum::Coal);
				exportEnums.push_back(ResourceEnum::Wood);
				exportEnums.push_back(ResourceEnum::Stone);
				exportEnums.push_back(ResourceEnum::Clay);
				chosenResourceEnum = exportEnums[GameRand::Rand() % exportEnums.size()];
			}

			element.isImport = false;
			element.resourceEnum = chosenResourceEnum;

			_autoExportElements.push_back(element);
		}



		// Usually import processed luxury
		{
			std::vector<ResourceEnum> importEnums = {
				ResourceEnum::Beer,
				ResourceEnum::Furniture,
				ResourceEnum::Pottery,
			};

			element.isImport = true;
			element.resourceEnum = importEnums[GameRand::Rand() % importEnums.size()];

			_autoImportElements.push_back(element);
		}
	}

	int32 lastRoundAutoTradeProfit() { return _lastRoundAutoTradeProfit; }
	void SetLastRoundAutoTradeProfit(int32 lastRoundAutoTradeProfit) {
		_lastRoundAutoTradeProfit = lastRoundAutoTradeProfit;
	}

	void CalculateAutoTradeProfit(
		int32& exportMoneyTotal,
		int32& importMoneyTotal,
		int32& feeTotal,
		int32& feeDiscountTotal,
		TArray<FText>& exportTooltipText,
		TArray<FText>& importTooltipText,
		TArray<FText>& feeTooltipText,
		TArray<FText>& feeDiscountTooltipText,
		bool shouldFillTipText
	);


	/*
	 * Claim Province
	 */
	void TryRemoveProvinceClaim(int32 provinceId, bool lightMode)
	{
		CppUtils::TryRemove(_provincesClaimed, provinceId);
		//_claimedProvinceConnected.erase(provinceId);

		if (!lightMode) {
			RecalculateTax(false);
		}
	}

	void ClaimProvince(int32 provinceIdIn, bool lightMode = false)
	{
		//PUN_LOG("ClaimProvince province:%d pid:%d", provinceId, _playerId);

		_provincesClaimed.push_back(provinceIdIn);

		RefreshProvinceInfo();


		if (lightMode) {
			return;
		}

		RecalculateTax(false);
	}

	/*
	 * Claim Province Attack
	 */

	void StartAttack_Defender(int32 attackerPlayerId, int32 provinceId, ProvinceAttackEnum provinceAttackEnum, std::vector<CardStatus>& initialMilitaryCards);
	void StartAttack_Attacker(int32 provinceId) {
		_attackingProvinceIds.push_back(provinceId);
	}

	const std::vector<ProvinceClaimProgress>& defendingClaimProgress() { return _defendingClaimProgress; }

	const std::vector<ProvinceClaimProgress>& defendingClaimProgress_pendingRemoval() { return _defendingClaimProgress_pendingRemoval; }

	ProvinceClaimProgress GetDefendingClaimProgress(int32 provinceId) const {
		for (const ProvinceClaimProgress& claimProgress : _defendingClaimProgress) {
			if (claimProgress.provinceId == provinceId) {
				return claimProgress;
			}
		}
		return ProvinceClaimProgress();
	}

	ProvinceClaimProgress GetDefendingClaimProgressDisplay(int32 provinceId) const {
		for (const ProvinceClaimProgress& claimProgress : _defendingClaimProgress) {
			if (claimProgress.provinceId == provinceId) {
				return claimProgress;
			}
		}
		for (const ProvinceClaimProgress& claimProgress : _defendingClaimProgress_pendingRemoval) {
			if (claimProgress.provinceId == provinceId) {
				return claimProgress;
			}
		}
		return ProvinceClaimProgress();
	}

	ProvinceClaimProgress* GetDefendingClaimProgressPtr(int32 provinceId) {
		for (ProvinceClaimProgress& claimProgress : _defendingClaimProgress) {
			if (claimProgress.provinceId == provinceId) {
				return &claimProgress;
			}
		}
		return nullptr;
	}


	void ReturnMilitaryUnitCards(std::vector<CardStatus>& cards);
	void RetreatMilitaryUnitCards(std::vector<CardStatus>& cards, int32 playerId);
	
	void EndConquer(int32 provinceId)
	{
		// Return Military Units
		for (int32 i = 0; i < _defendingClaimProgress.size(); i++)
		{
			ProvinceClaimProgress& claimProgress = _defendingClaimProgress[i];
			if (claimProgress.provinceId == provinceId) 
			{
				ReturnMilitaryUnitCards(claimProgress.attackerFrontLine);
				ReturnMilitaryUnitCards(claimProgress.attackerBackLine);
				ReturnMilitaryUnitCards(claimProgress.defenderFrontLine);
				ReturnMilitaryUnitCards(claimProgress.defenderBackLine);
				break;
			}
		}

		for (int32 i = _defendingClaimProgress.size(); i-- > 0;)
		{
			ProvinceClaimProgress& claimProgress = _defendingClaimProgress[i];
			if (claimProgress.provinceId == provinceId)
			{
				int32 lastDeadCompleteTick = Time::Ticks();
				claimProgress.ExecuteOnAll_PendingRemoval([&](const CardStatus& card)
				{
					lastDeadCompleteTick = std::max(lastDeadCompleteTick, card.displayCardStateValue3 + ProvinceClaimProgress::AnimationLengthTicks);
				});
				claimProgress.battleFinishCountdownTicks = lastDeadCompleteTick - Time::Ticks();
				_defendingClaimProgress_pendingRemoval.push_back(claimProgress);
				_defendingClaimProgress.erase(_defendingClaimProgress.begin() + i);
				break;
			}
		}

		//CppUtils::RemoveOneIf(_defendingClaimProgress, [&](ProvinceClaimProgress& claimProgress) { return claimProgress.provinceId == provinceId; });
	}
	void EndConquer_Attacker(int32 provinceId)
	{
		CppUtils::Remove(_attackingProvinceIds, provinceId);
	}



	// provinceId and attackerPlayerId is needed because this maybe called before the attack started
	ProvinceAttackEnum GetProvinceAttackEnum(int32 provinceId, int32 attackerPlayerId)
	{
		auto getMainCityAttackEnum = [&]()
		{
			if (lordPlayerId() != -1)
			{
				// The attacker is the townhall owner, trying to expel the lord
				if (attackerPlayerId == _playerId) {
					return ProvinceAttackEnum::DeclareIndependence;
				}
				return ProvinceAttackEnum::VassalCompetition;
			}
			return ProvinceAttackEnum::Vassalize;
		};

		int32 provincePlayerId = _simulation->provinceOwnerPlayer(provinceId);

		//! Minor Town
		if (provincePlayerId == -1)
		{
			return getMainCityAttackEnum();
		}

		//! Capital
		if (_simulation->homeProvinceId(provincePlayerId) == provinceId)
		{
			return getMainCityAttackEnum();
		}

		//! Colony
		int32 townId = _simulation->provinceOwnerTown_Major(provinceId);
		if (townId != -1 &&
			_simulation->IsTownhallOverlapProvince(provinceId, townId))
		{
			return ProvinceAttackEnum::ConquerColony;
		}

		return ProvinceAttackEnum::ConquerProvince;
	}

	/*
	 * Vassal
	 */
	int32 lordPlayerId() { return _lordPlayerId; }
	void SetLordPlayerId(int32 lordPlayerId) {
		_lordPlayerId = lordPlayerId;
	}

	int32 playerIdForLogo() {
		if (_lordPlayerId != -1) {
			return _lordPlayerId;
		}
		if (IsMinorTown(_townId)) {
			int32 mainAllyId = _relationships.GetMainAllyId();
			if (mainAllyId != -1) {
				return mainAllyId;
			}
		}
		return _playerId;
	}

	int32 defenderPlayerId() {
		return _playerId;
	}

	const std::vector<int32>& vassalTownIds() const { return _vassalTownIds; }
	void GainVassal(int32 vassalTownId) {
		_vassalTownIds.push_back(vassalTownId);
	}
	void LoseVassal(int32 vassalTownId) {
		CppUtils::TryRemove(_vassalTownIds, vassalTownId);
	}
	bool IsVassal(int32 vassalTownId) {
		return CppUtils::Contains(_vassalTownIds, vassalTownId);
	}

	/*
	 * Ally
	 */
	// TODO: deprecate this???
	const std::vector<int32>& allyPlayerIds() { return _allyPlayerIds; }
	void GainAlly(int32 allyPlayerId) {
		_allyPlayerIds.push_back(allyPlayerId);
	}
	void LoseAlly(int32 allyPlayerId) {
		CppUtils::TryRemove(_allyPlayerIds, allyPlayerId);
	}
	bool IsAlly(int32 playerId) {
		return CppUtils::Contains(_allyPlayerIds, playerId);
	}


	int64 vassalTaxPercent() {
		return 5;
		//switch (taxLevel)
		//{
		//case 0: return 0;
		//case 1: return 10;
		//case 2: return 20;
		//case 3: return 30;
		//case 4: return 40;
		//default:
		//	UE_DEBUG_BREAK();
		//}
		//return -1;
	}
	int32 vassalInfluencePercent() {
		return 50;
	}

	int32 GetMinorCityAllyInfluenceReward(int32 playerId)
	{
		if (_relationships.isAlly(playerId)) {
			return 150;
		}
		int32 totalRelationship = _relationships.GetTotalRelationship(playerId);
		if (totalRelationship >= 60) {
			return 60;
		}
		if (totalRelationship >= 30) {
			return 30;
		}
		return 0;
	}

	/*
	 * Minor City
	 * - target wealth largely mimics nearby X cities's revenue
	 * - income bounce to catch up with target wealth
	 * - trade increases with wealth.. Assumption is that high pop cities will deal with more minor cities, so increase doesn't have to be as significant
	 * - Giving it money will speedup its growth
	 *
	 * - Raiding X city will give roughly the cities' revenue.. X varies with Era.. (assuming later era has more military influence)
	 * - Raiding Card is more expensive, the more frequent the raid.
	 * - Razing city gives a bit more money/influence, but is a one time thing..
	 */
	//void RefreshMinorCityTargetWealth();

	// 500*1.3^20 = 95k .. (starts with 500)
	// start around 20 per round, ends at around 4000 per round
	
	int32 GetMinorCityMoneyIncome() {
		int32 incomeBefore1000 = _minorCityWealth * 30 / 100 / Time::RoundsPerYear;
		if (incomeBefore1000 <= 1000) {
			return incomeBefore1000;
		}
		return static_cast<int32>(sqrt(static_cast<double>(incomeBefore1000) / 1000) * 1000);
	}
	
	void GetMinorCityLevelAndCurrentMoney(int32& level, int32& currentMoney, int32& moneyToNextLevel)
	{
		const int32 firstLevelRequirement = 1000;

		// TODO: use mod instead???

		level = 1;
		currentMoney = _minorCityWealth;
		
		moneyToNextLevel = firstLevelRequirement;

		while (currentMoney >= moneyToNextLevel) {
			level++;
			currentMoney -= moneyToNextLevel;
			moneyToNextLevel = moneyToNextLevel * 200 / 100;
		}
	}

	int32 GetMinorCityLevel()
	{
		int32 level, currentMoney, moneyToNextLevel;
		GetMinorCityLevelAndCurrentMoney(level, currentMoney, moneyToNextLevel);
		return level;
	}

	bool CanBeClaimed()
	{
		return townhallId != -1 && 
			_simulation->buildingEnum(townhallId) == CardEnum::MinorCity && 
			GetMinorCityLevel() <= 1;
	}

	int32 GetTourismPopulation() {
		return GetMinorCityLevel() * 10;
	}
	

	int32 totalWealth_MinorCity() { return _minorCityWealth; }
	void ChangeWealth_MinorCity(int32 value) {
		_minorCityWealth += value;
	}

	// Revenue depends on minorCityWealth
	virtual int64 totalRevenue100() { return GetMinorCityMoneyIncome() * 100; }
	virtual int32 totalInfluenceIncome100() { return totalRevenue100() / 2; } // TODO: Not used yet??

	/*
	 * Children
	 */

	void AddChildBuilding(Building& child);

	const std::vector<int32>& childBuildingIds() {
		return _childBuildingIds;
	}

	std::vector<int32> GetPortIds()
	{
		for (int32 childBuildingId : _childBuildingIds) {
			if (_simulation->building(childBuildingId).isEnum(CardEnum::MinorCityPort)) {
				return _childBuildingIds;
			}
		}
		return {};
	}

	// 


	void SetRelationshipToWar(int32 askingPlayerId)
	{
		if (_simulation->IsAIPlayer(_playerId) || IsMinorTown(_townId)) {
			_relationships.SetModifier(askingPlayerId, RelationshipModifierEnum::YouAttackedUs, -100);
			_relationships.SetModifier(askingPlayerId, RelationshipModifierEnum::YouBefriendedUs, 0);
		}
	}

	void Tick1SecRelationship();

	void ProposeAlliance(int32 askingPlayerId);


	/*
	 * Helpers
	 */
	virtual void RefreshProvinceInfo() {}

	virtual void RecalculateTax(bool showFloatup) {}

	virtual const std::vector<class Hotel*> GetConnectedHotels();
	
	/*
	 * Serialize
	 */
	void Serialize(FArchive& Ar)
	{
		//! Public
		Ar << _playerId;
		Ar << _townId;
		Ar << _factionEnum;
		
		Ar << townhallId;

		SerializeVecValue(Ar, _provincesClaimed);

		SerializeVecValue(Ar, _autoExportElements);
		SerializeVecValue(Ar, _autoImportElements);

		Ar << _lastRoundAutoTradeProfit;

		Ar << _relationships;


		//! Vassal/Annex
		Ar << _lordPlayerId;
		SerializeVecValue(Ar, _vassalTownIds);
		SerializeVecValue(Ar, _allyPlayerIds);

		SerializeVecValue(Ar, _attackingProvinceIds);
		SerializeVecObj(Ar, _defendingClaimProgress);

		//! Minor City
		Ar << _minorCityWealth;
		//Ar << _minorCityTargetWealth;
	}

public:
	int32 _playerId = -1;
	int32 _townId = -1;
	FactionEnum _factionEnum = FactionEnum::None;
	
	int32 townhallId = -1;

	static const int32 BaseAutoTradeFeePercent = 40;

protected:
	// FactionEnum
	//FactionEnum _factionEnum = FactionEnum::Arab;
	
	//
	std::vector<int32> _provincesClaimed;

	// Auto Trade
	std::vector<AutoTradeElement> _autoExportElements;
	std::vector<AutoTradeElement> _autoImportElements;

	int32 _lastRoundAutoTradeProfit = 0;

	// Relationship
	// 1 relationship should cost around 20 gold
	RelationshipModifiers _relationships;

	// Vassal/Annex
	int32 _lordPlayerId = -1;
	std::vector<int32> _vassalTownIds;
	std::vector<int32> _allyPlayerIds;

	std::vector<int32> _attackingProvinceIds;
	std::vector<ProvinceClaimProgress> _defendingClaimProgress;

	// Minor City
	int32 _minorCityWealth = -1;
	//int32 _minorCityTargetWealth = -1;
	std::vector<int32> _childBuildingIds;

	/*
	 * Non-Serialize
	 */
	IGameSimulationCore* _simulation = nullptr;
	
	std::vector<ProvinceClaimProgress> _defendingClaimProgress_pendingRemoval;
};