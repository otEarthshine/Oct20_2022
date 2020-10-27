// Pun Dumnernchanvanit's


#include "TradeBuilding.h"
#include "../WorldTradeSystem.h"

void TradeBuilding::UsedTrade(FTradeResource tradeCommand)
{
	_lastCheckTick = Time::Ticks();
	_tradeCommand = tradeCommand;

	// Sold stuff executes right away...
	// Bought stuff will arrive in 1 season
	int32 exportMoney100 = 0;
	int32 importMoney100 = 0;
	ExecuteTrade(tradeCommand, tradingFeePercent(), centerTile(), _simulation, false, exportMoney100, importMoney100);

	_exportGainLast1Round += exportMoney100;
	_importLossLast1Round += importMoney100;

	_hasPendingTrade = true;
	_ticksAccumulated = 0;
	_lastCheckTick = Time::Ticks();
}

void TradeBuilding::ExecuteTrade(FTradeResource tradeCommand, int32 tradingFeePercent, WorldTile2 tile, IGameSimulationCore* simulation, bool isInstantBuy, int32& exportMoney100, int32& importMoney100)
{
	// Sold stuff executes on the way right away...
	int32 playerId = tradeCommand.playerId;
	ResourceSystem& resourceSys = simulation->resourceSystem(playerId);

	// This value is used in fee calculation
	//int32 totalTradeMoney100 = 0;
	int32 totalImportMoney100 = 0;
	int32 totalExportMoney100 = 0;
	
	for (int i = 0; i < tradeCommand.buyEnums.Num(); i++) {
		PUN_CHECK(tradeCommand.buyAmounts[i] != 0);
		ResourceEnum resourceEnum = static_cast<ResourceEnum>(tradeCommand.buyEnums[i]);
		int32 buyAmount = tradeCommand.buyAmounts[i];

		// This is used just for counting quest's total trade amount (otherwise use tradeCommand.totalGain)

		if (buyAmount < 0) // Sell
		{
			// TODO: ?? this is cheating the system, getting paid more than actually sold, calculate trade gain here?

			int32 sellAmount = abs(buyAmount);
			sellAmount = min(sellAmount, resourceSys.resourceCount(resourceEnum));

			int32 tradeMoney100 = sellAmount * simulation->price100(resourceEnum);
			totalExportMoney100 += tradeMoney100;

			resourceSys.RemoveResourceGlobal(resourceEnum, sellAmount);
			resourceSys.ChangeMoney100(tradeMoney100);
		}
		else if (buyAmount > 0) // Buy
		{
			int32 tradeMoney100 = buyAmount * simulation->price100(resourceEnum);
			totalImportMoney100 += tradeMoney100;

			if (isInstantBuy) {
				resourceSys.AddResourceGlobal(resourceEnum, buyAmount, *simulation);
			}
			resourceSys.ChangeMoney100(-tradeMoney100);
		}
		
		simulation->worldTradeSystem().ChangeSupply(playerId, resourceEnum, -buyAmount); // Buying is decreasing world supply
	}

	// Change money from trade's Total
	//resourceSys.ChangeMoney(tradeCommand.totalGain);

	int32 totalTradeMoney100 = totalImportMoney100 + totalExportMoney100;

	int32 fee100 = totalTradeMoney100 * tradingFeePercent / 100;
	resourceSys.ChangeMoney100(-fee100);
	
	simulation->QuestUpdateStatus(playerId, QuestEnum::TradeQuest, abs(totalTradeMoney100 / 100 + fee100 / 100));

	simulation->uiInterface()->ShowFloatupInfo(FloatupEnum::GainMoney, tile, ToSignedNumber(tradeCommand.totalGain));

	exportMoney100 = totalExportMoney100;
	importMoney100 = totalImportMoney100;
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

	// Tick move round 1 sec after the last round refresh
	if (Time::Seconds() % Time::SecondsPerRound == 1)
	{
		_exportGainLast2Round = _exportGainLast1Round;
		_exportGainLast1Round = 0;

		_importLossLast2Round = _importLossLast1Round;
		_importLossLast1Round = 0;
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

	// TODO: WTF??
	// Show trade failed for 5 sec...
	//int32 ticksSinceLastRetry = tradeRetryDelayTicks - TradeRetryCountDownTicks();
	//if (ticksSinceLastRetry > Time::TicksPerSecond * 5) {
	//	hoverWarning = HoverWarning::None;
	//}

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

	hoverWarning = HoverWarning::None;

	if (isImport)
	{
		if (targetAmount > currentResourceCount) {
			int32 buyAmountFromTarget = std::min(targetAmount - currentResourceCount, tradeMaximumPerRound());
			int32 moneyLeftToBuyAfterFee = resourceSystem().money() * 100 / (100 + tradingFeePercent());
			moneyLeftToBuyAfterFee = std::max(0, moneyLeftToBuyAfterFee);
			
			int32 maxAmountMoneyCanBuy = moneyLeftToBuyAfterFee / GetResourceInfo(activeResourceEnum).basePrice;
			int32 buyAmount = std::min(maxAmountMoneyCanBuy, buyAmountFromTarget);

			if (buyAmount > 0) 
			{
				issueTradeCommand(buyAmount);
				ResetTradeRetryTick();
				return;
			}
			if (maxAmountMoneyCanBuy == 0) {
				hoverWarning = HoverWarning::NotEnoughMoney;
			} else {
				hoverWarning = HoverWarning::AlreadyReachedTarget;
			}
		}
		else {
			hoverWarning = HoverWarning::AlreadyReachedTarget;
		}
	}
	else
	{
		if (currentResourceCount > targetAmount) {
			int32 sellAmount = std::min(currentResourceCount - targetAmount, tradeMaximumPerRound());

			if (sellAmount > 0) 
			{
				issueTradeCommand(-sellAmount);
				ResetTradeRetryTick();
				return;
			}
		}

		hoverWarning = HoverWarning::ResourcesBelowTarget;
	}

	ResetTradeRetryTick();
}

void OreSupplier::TickRound()
{

}
