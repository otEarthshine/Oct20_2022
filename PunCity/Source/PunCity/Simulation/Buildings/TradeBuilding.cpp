// Pun Dumnernchanvanit's


#include "TradeBuilding.h"
#include "../WorldTradeSystem.h"

void TradeBuilding::UsedTrade(FTradeResource tradeCommand)
{
	_lastCheckTick = Time::Ticks();
	_tradeCommand = tradeCommand;

	// Sold stuff executes right away...
	// Bought stuff will arrive in 1 season
	ExecuteTradeSell(tradeCommand, tradingFeePercent(), centerTile(), _simulation);

	_hasPendingTrade = true;
	_ticksAccumulated = 0;
	_lastCheckTick = Time::Ticks();
}

void TradeBuilding::ExecuteTradeSell(FTradeResource tradeCommand, int32 tradingFeePercent, WorldTile2 tile, IGameSimulationCore* simulation)
{
	// Sold stuff executes on the way right away...
	int32 playerId = tradeCommand.playerId;
	ResourceSystem& system = simulation->resourceSystem(playerId);

	int32 totalTradeMoney100 = 0;
	for (int i = 0; i < tradeCommand.buyEnums.Num(); i++) {
		PUN_CHECK(tradeCommand.buyAmounts[i] != 0);
		ResourceEnum resourceEnum = static_cast<ResourceEnum>(tradeCommand.buyEnums[i]);
		int32 buyAmount = tradeCommand.buyAmounts[i];

		// This is used just for counting quest's total trade amount (otherwise use tradeCommand.totalGain)
		totalTradeMoney100 += abs(tradeCommand.buyAmounts[i] * simulation->price100(resourceEnum));

		if (tradeCommand.buyAmounts[i] < 0) {
			// TODO: this is cheating the system, getting paid more than actually sold, calculate trade gain here?
			buyAmount = max(buyAmount, -system.resourceCount(resourceEnum));

			system.RemoveResourceGlobal(resourceEnum, -buyAmount);
		}
		simulation->worldTradeSystem().ChangeSupply(playerId, resourceEnum, -buyAmount); // Buying is decreasing world supply
	}

	// Change money from trade's Total
	system.ChangeMoney(tradeCommand.totalGain);

	totalTradeMoney100 += totalTradeMoney100 * tradingFeePercent / 100;
	simulation->QuestUpdateStatus(playerId, QuestEnum::TradeQuest, totalTradeMoney100 / 100);

	simulation->uiInterface()->ShowFloatupInfo(FloatupEnum::GainMoney, tile, ToSignedNumber(tradeCommand.totalGain));
}


void TradeBuilding::Tick1Sec()
{
	if (!isConstructed()) {
		return;
	}


	if (_hasPendingTrade)
	{
		if (actualTicksLeft() <= 0)
		{
			unordered_map<ResourceEnum, int32> resourceToCount;
			for (ResourceInfo info : ResourceInfos) {
				int32 resourceCount = GetResourceCount(info.resourceEnum);
				if (resourceCount > 0) {
					resourceToCount[info.resourceEnum] = resourceCount;
				}
			}

			// Try add resources bought once it arrives
			FTradeResource& tradeCommand = _tradeCommand;

			_isTradingPostFull = false;

			// Check if there is enough space to add resource
			for (int32 i = 0; i < tradeCommand.buyEnums.Num(); i++) 
			{
				ResourceEnum resourceEnum = static_cast<ResourceEnum>(tradeCommand.buyEnums[i]);
				int32 buyAmount = tradeCommand.buyAmounts[i];

				if (buyAmount > 0) 
				{
					int32 spaceLeftForResource = StorageLeftForResource(resourceToCount, resourceEnum, storageSlotCount());
					if (buyAmount > spaceLeftForResource) {
						_isTradingPostFull = true;
					}
					int32 unloadAmount = min(buyAmount, spaceLeftForResource);
					if (unloadAmount > 0) {
						resourceToCount[resourceEnum] += unloadAmount;
						tradeCommand.buyAmounts[i] -= unloadAmount;
						AddResource(resourceEnum, unloadAmount);
					}
				}
			}

			//_isTradingPostFull = StorageTilesOccupied(resourceToCount) > storageSlotCount();

			if (!_isTradingPostFull) {
				_hasPendingTrade = false;
			}

			return;
		}

		// Increment
		_ticksAccumulated += ticksFromLastCheck();
		_lastCheckTick = Time::Ticks();
	}
}

void TradingCompany::Tick1Sec()
{
	TradeBuilding::Tick1Sec();

	if (!isConstructed()) {
		return;
	}

	if (_hasPendingTrade) {
		ResetTradeRetryTick();
		return;
	}

	// Show trade failed for 3 sec...
	if (tradeRetryDelayTicks - TradeRetryCountDownTicks() > Time::TicksPerSecond * 3) {
		_tradeFailed = false;
	}

	if (TradeRetryCountDownTicks() > 0) {
		return;
	}

	//if (TradeRetryCountDownTicks() < 0) {
	//	ResetTradeRetryTick();
	//	return;
	//}

	if (activeResourceEnum == ResourceEnum::None) {
		return;
	}

	int32 currentResourceCount = _simulation->resourceCount(_playerId, activeResourceEnum);

	//auto changeMoney = [&](int32 profit) {
	//	resourceSystem().ChangeMoney(profit);
	//	_profitLast2 = _profitLast1;
	//	_profitLast1 = profit;
	//};

	auto issueTradeCommand = [&](int32 buyAmount)
	{
		int32 tradeMoneyBeforeFee100 = buyAmount * _simulation->price100(activeResourceEnum);
		int32 tradeFee100 = abs(tradeMoneyBeforeFee100) * tradingFeePercent() / 100;
		int32 tradeMoney100 = tradeMoneyBeforeFee100 + tradeFee100;

		int32 coinGain = -tradeMoney100 / 100;
		_tradeCommand.playerId = _playerId;
		_tradeCommand.buyEnums.Empty();
		_tradeCommand.buyAmounts.Empty();
		_tradeCommand.buyEnums.Add(static_cast<uint8>(activeResourceEnum));
		_tradeCommand.buyAmounts.Add(buyAmount);
		_tradeCommand.totalGain = coinGain;
		_tradeCommand.objectId = _objectId;

		UsedTrade(_tradeCommand);
	};

	if (isImport)
	{
		if (targetAmount > currentResourceCount) {
			int32 buyAmount = std::min(targetAmount - currentResourceCount, tradeMaximumPerRound());
			int32 moneyLeftToBuyAfterFee = resourceSystem().money() * 100 / (100 + tradingFeePercent());
			int32 maxAmountMoneyCanBuy = moneyLeftToBuyAfterFee / GetResourceInfo(activeResourceEnum).basePrice;
			buyAmount = std::min(maxAmountMoneyCanBuy, buyAmount);

			if (buyAmount > 0) {
				issueTradeCommand(buyAmount);
				ResetTradeRetryTick();
				return;
			}
		}
	}
	else
	{
		if (currentResourceCount > targetAmount) {
			int32 sellAmount = std::min(currentResourceCount - targetAmount, tradeMaximumPerRound());

			if (sellAmount > 0) {
				issueTradeCommand(-sellAmount);
				ResetTradeRetryTick();
				return;
			}
		}
	}

	_tradeFailed = true;
	ResetTradeRetryTick();
}

void OreSupplier::TickRound()
{

}
