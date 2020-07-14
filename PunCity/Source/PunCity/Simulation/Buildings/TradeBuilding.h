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
		int32 result = Time::TicksPerSeason;
		if (isEnum(CardEnum::TradingCompany)) {
			result /= 2;
		}
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

	static void ExecuteTradeSell(FTradeResource tradeCommand, int32 tradingFeePercent, WorldTile2 tile, IGameSimulationCore* simulation);

	void Tick1Sec() override;

	void Serialize(FArchive& Ar) override {
		Building::Serialize(Ar);
		Ar << _lastCheckTick;
		Ar << _ticksAccumulated;
		
		Ar << _hasPendingTrade;
		_tradeCommand >> Ar;
		Ar << _isTradingPostFull;
	}

protected:
	FTradeResource _tradeCommand;

	int32 _lastCheckTick = 0; // TODO: use this as last checked tick for now for lerp...
	int32 _ticksAccumulated = 0;
	
	
	bool _hasPendingTrade = false;

	bool _isTradingPostFull = false;
};

class TradingPost final : public TradeBuilding
{
public:
	void FinishConstruction() final {
		TradeBuilding::FinishConstruction();

		for (ResourceInfo info : ResourceInfos) {
			AddResourceHolder(info.resourceEnum, ResourceHolderType::Provider, 0);
		}

		_upgrades = {
			BuildingUpgrade("Fee discount", "Decrease trading fee by 5%.", 250),
			BuildingUpgrade("Fast delivery", "-30% the time it takes, for traders to arrive (+50% efficiency).", 300),
			BuildingUpgrade("Increased load", "+120 goods quantity per trade.", 200),
		};

		_simulation->TryAddQuest(_playerId, std::make_shared<TradeQuest>());
	}

	int32 maxTradeQuatity() override {
		int32 quantity = 180;
		if (IsUpgraded(2)) {
			quantity += 120;
		}
		return quantity;
	}

	std::vector<BonusPair> GetBonuses() final {
		std::vector<BonusPair> result;
		if (IsUpgraded(1)) {
			result.push_back({"Fast delivery", 50});
		}
		if (_simulation->playerOwned(_playerId).HasSpeedBoost(buildingId())) {
			result.push_back({ "Speed Boost", 50 });
		}
		
		return result;
	}

};

class TradingPort final : public TradeBuilding
{
public:
	void FinishConstruction() final {
		TradeBuilding::FinishConstruction();

		for (ResourceInfo info : ResourceInfos) {
			AddResourceHolder(info.resourceEnum, ResourceHolderType::Provider, 0);
		}

		_upgrades = {
			BuildingUpgrade("Fee discount", "Decrease trading fee by 5%.", 250),
			BuildingUpgrade("Fast delivery", "Halve the time it takes, for traders to arrive (+100% efficiency).", 500),
			BuildingUpgrade("Increased load", "+240 goods quantity per trade.", 300),
		};

		_simulation->TryAddQuest(_playerId, std::make_shared<TradeQuest>());
	}

	int32 maxTradeQuatity() override {
		int32 quantity = 240;
		if (IsUpgraded(2)) {
			quantity += 240;
		}
		return quantity;
	}

	std::vector<BonusPair> GetBonuses() final {
		std::vector<BonusPair> result;
		if (IsUpgraded(1)) {
			result.push_back({"Fast delivery", 100});
		}
		if (_simulation->playerOwned(_playerId).HasSpeedBoost(buildingId())) {
			result.push_back({ "Speed Boost", 50 });
		}
		
		return result;
	}

};

class TradingCompany final : public TradeBuilding
{
public:
	void FinishConstruction() final
	{
		TradeBuilding::FinishConstruction();

		for (ResourceInfo info : ResourceInfos) {
			AddResourceHolder(info.resourceEnum, ResourceHolderType::Provider, 0);
		}

		_upgrades = {
			BuildingUpgrade("Fee discount", "Decrease trading fee by 5%.", 250),
			BuildingUpgrade("Efficient Hauling", "+60 goods quantity per trade.", 350),
			BuildingUpgrade("Marine trade", "+60 goods quantity per trade, if adjacent to a trading port.", 200),
		};

		_simulation->TryAddQuest(_playerId, std::make_shared<TradeQuest>());

		needTradingCompanySetup = true;

		ResetTradeRetryTick();
	}

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

	int32 upkeep() override {
		if (_simulation->buildingFinishedCount(_playerId, CardEnum::StockMarket)) {
			return 1;
		}
		return Building::upkeep();
	}

	int32 exportMoney() {
		return std::max(0, _profitLast1);
		//+ std::max(0, _profitLast2);
	}
	int32 importMoney() {
		return std::min(0, _profitLast1);
		//+ std::min(0, _profitLast2);
	}

	std::vector<BonusPair> GetBonuses() final {
		std::vector<BonusPair> result;
		if (_simulation->playerOwned(_playerId).HasSpeedBoost(buildingId())) {
			result.push_back({ "Speed Boost", 50 });
		}
		return result;
	}

	void Tick1Sec() override;

	void Serialize(FArchive& Ar) override {
		TradeBuilding::Serialize(Ar);
		Ar << isImport;
		Ar << activeResourceEnum;
		Ar << targetAmount;

		Ar << _ticksToStartTrade;
		Ar << _tradeFailed;

		Ar << _profitLast1;
		Ar << _profitLast2;

		Ar << needTradingCompanySetup;
	}

	void ResetTradeRetryTick() {
		_ticksToStartTrade = Time::Ticks() + tradeRetryDelayTicks;
	}

	int32 TradeRetryCountDownTicks() {
		return std::max(0, _ticksToStartTrade - Time::Ticks());
	}

	bool lastTradeFailed() { return _tradeFailed; }

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

private:
	const int32 tradeRetryDelayTicks = 20 * Time::TicksPerSecond;

	int32 _ticksToStartTrade = -1;
	bool _tradeFailed = false;

	int32 _profitLast1 = 0;
	int32 _profitLast2 = 0;
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
	void FinishConstruction() override {
		Building::FinishConstruction();

		_upgrades = {
			BuildingUpgrade("More Ore", "Buy 10 more ore each round", 200),
			BuildingUpgrade("Even More Ore", "Buy 20 more ore each round", 500),
		};
	}

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