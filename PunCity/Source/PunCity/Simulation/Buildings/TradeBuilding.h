// Pun Dumnernchanvanit's

#pragma once

#include "../Building.h"
#include "PunCity/NetworkStructs.h"
#include "../QuestSystem.h"

/**
 * 
 */
class TradeBuilding : public Building
{
public:
	// TODO: use this to spawn trader and have it come towards us...
	void FinishConstruction() override {
		Building::FinishConstruction();

		_hasPendingTrade = false;
		_lastCheckTick = -Time::TicksPerSeason;
		_ticksAccumulated = 0;
	}

	int32 storageSlotCount() override {
		return 4;
	}
	int32 stackPerSide() override {
		return 2;
	}

	/*
	 * Variables
	 */

	bool IsTradeBuildingFull() const { return _isTradingPostFull; }

	//int32 totalTicks() {
	//	if (isEnum(CardEnum::TradingCompany)) {
	//		return Time::TicksPerSeason * 100 / efficiency() / 2; // Trading company requires half the time to trade
	//	}
	//	return Time::TicksPerSeason * 100 / efficiency();
	//}
	int32 totalTicks() {
		int32 result = Time::TicksPerSeason / 2;
		//if (isEnum(CardEnum::TradingCompany)) {
		//	result /= 2;
		//}
		return result;
	}

	int32 ticksFromLastCheck() {
		int32 ticksFromLastCheck = Time::Ticks() - _lastCheckTick;
		return ticksFromLastCheck * efficiency() / 100;
	}
	int32 actualTicksLeft() {
		int32 ticksLeftAtLastCheck = totalTicks() - _ticksAccumulated;
		return max(0, ticksLeftAtLastCheck - ticksFromLastCheck()); // include lerp from last check
	}
	float CountdownSecondsDisplay() {
		return actualTicksLeft() / Time::TicksPerSecond;
	}
	int32 CountdownSecondsDisplayInt() {
		return actualTicksLeft() / Time::TicksPerSecond;
	}
	float barFraction() override {
		if (_hasPendingTrade) {
			return 1.0f - CountdownSecondsDisplay() / (totalTicks() / Time::TicksPerSecond);
		}
		return 0.0f;
	}

	int32 lastCheckTick() const { return _lastCheckTick; }


	

	std::vector<ResourcePair> tradeResourcePairs() const
	{
		std::vector<ResourcePair> resourcePairs;
		if (_hasPendingTrade) {
			for (int32 i = 0; i < _tradeCommand.buyEnums.Num(); i++) {
				resourcePairs.push_back(ResourcePair(static_cast<ResourceEnum>(_tradeCommand.buyEnums[i]), _tradeCommand.buyAmounts[i]));
			}
		}
		return resourcePairs;
	}


	bool CanTrade() const { return !_hasPendingTrade; }
	bool HasPendingTrade() const { return _hasPendingTrade; }


	/*
	 * Trade
	 */

	void UsedTrade(FTradeResource tradeCommand);

	static void ExecuteTrade(FTradeResource tradeCommand, int32 baseTradingFeePercent, WorldTile2 tile, IGameSimulationCore* simulation, bool isInstantBuy, int32& exportMoney100, int32& importMoney100);

	void OnTick1Sec() override;

	
	int32 exportMoney100() {
		return std::max(0, _exportGain100Last1Round + _exportGain100Last2Round);
	}
	int32 importMoney100() {
		return std::max(0, _importLoss100Last1Round + _importLoss100Last2Round);
	}
	

	/*
	 * Serialize
	 */
	void Serialize(FArchive& Ar) override {
		Building::Serialize(Ar);
		Ar << _lastCheckTick;
		Ar << _ticksAccumulated;
		
		Ar << _hasPendingTrade;
		_tradeCommand >> Ar;
		Ar << _isTradingPostFull;

		Ar << _importLoss100Last1Round;
		Ar << _importLoss100Last2Round;
		Ar << _exportGain100Last1Round;
		Ar << _exportGain100Last2Round;
	}

protected:
	FTradeResource _tradeCommand;

	int32 _lastCheckTick = 0; // TODO: use this as last checked tick for now for lerp...
	int32 _ticksAccumulated = 0;
	
	
	bool _hasPendingTrade = false;

	bool _isTradingPostFull = false;

	int32 _importLoss100Last1Round = 0;
	int32 _importLoss100Last2Round = 0;
	int32 _exportGain100Last1Round = 0;
	int32 _exportGain100Last2Round = 0;
};

class TradingPost final : public TradeBuilding
{
public:
	void FinishConstruction() final;
	std::vector<BonusPair> GetBonuses() final;

	int32 maxTradeQuatity() override {
		int32 quantity = 180;
		if (IsUpgraded(2)) {
			quantity += 120;
		}
		return quantity;
	}
};

class TradingPort final : public TradeBuilding
{
public:
	void FinishConstruction() final;
	std::vector<BonusPair> GetBonuses() final;

	int32 maxTradeQuatity() override {
		int32 quantity = 240;
		if (IsUpgraded(2)) {
			quantity += 240;
		}
		return quantity;
	}
};

class TradingCompany final : public TradeBuilding
{
public:
	void FinishConstruction() final;
	std::vector<BonusPair> GetBonuses() final;

	int32 tradeMaximumPerRound()
	{
		int32 quantity = 120;
		if (IsUpgraded(1)) {
			quantity += 60;
		}
		if (IsUpgraded(2)) {
			bool hasNearbyPort = _simulation->hasBuildingWithCondition(_playerId, CardEnum::TradingPort, [&](Building& bld) {
				return WorldTile2::ManDistance(bld.centerTile(), centerTile()) <= 18;
			});
			if (hasNearbyPort) {
				quantity += 60;
			}
		}
		return quantity;
	}

	int32 baseUpkeep() override {
		if (_simulation->townBuildingFinishedCount(_playerId, CardEnum::StockMarket)) {
			return 1;
		}
		if (_simulation->TownhallCardCountTown(_playerId, CardEnum::Conglomerate)) {
			return 1;
		}
		return Building::baseUpkeep();
	}

	void OnTick1Sec() override;

	void Serialize(FArchive& Ar) override {
		TradeBuilding::Serialize(Ar);
		Ar << isImport;
		Ar << activeResourceEnum;
		Ar << targetAmount;

		Ar << _ticksToStartTrade;

		Ar << needTradingCompanySetup;
	}

	void ResetTradeRetryTick() {
		_ticksToStartTrade = Time::Ticks() + tradeRetryDelayTicks;
	}

	int32 TradeRetryCountDownTicks() {
		return std::max(0, _ticksToStartTrade - Time::Ticks());
	}

	float barFraction() override {
		if (_hasPendingTrade) {
			return 1.0f - static_cast<float>(actualTicksLeft()) / totalTicks();
		}
		else {
			return 1.0f - static_cast<float>(TradeRetryCountDownTicks()) / tradeRetryDelayTicks;
		}
	}

public:
	bool isImport = true;
	ResourceEnum activeResourceEnum = ResourceEnum::None;
	int32 targetAmount = 0;

	bool needTradingCompanySetup = true;

	// Not Serialized
	int32 lastTargetAmountSet = -1;

private:
	const int32 tradeRetryDelayTicks = 20 * Time::TicksPerSecond;

	int32 _ticksToStartTrade = -1;

};

class HumanitarianAidCamp final : public Building
{
	void FinishConstruction() final {
		Building::FinishConstruction();

		AddResourceHolder(ResourceEnum::Wheat, ResourceHolderType::Provider, 0);
		AddResource(ResourceEnum::Wheat, 100);
	}
};

class OreSupplier final : public Building
{
public:
	void FinishConstruction() override;

	void TickRound() override;

	int32_t maxBuyAmount() {
		int32_t buyAmount = 10;
		if (IsUpgraded(0)) {
			buyAmount += 10;
		}
		if (IsUpgraded(1)) {
			buyAmount += 20;
		}
		return buyAmount;
	}
};