// Pun Dumnernchanvanit's


#include "TradeBuilding.h"
#include "../WorldTradeSystem.h"

#define LOCTEXT_NAMESPACE "TradeBuilding"

void TradeBuilding::UsedTrade(FTradeResource tradeCommand)
{
	_lastCheckTick = Time::Ticks();
	_tradeCommand = tradeCommand;

	// Sold stuff executes right away...
	// Bought stuff will arrive in 1 season
	int32 exportMoney100 = 0;
	int32 importMoney100 = 0;
	ExecuteTrade(tradeCommand, tradingFeePercent(), centerTile(), _simulation, false, exportMoney100, importMoney100);

	_exportGain100Last1Round += exportMoney100;
	_importLoss100Last1Round += importMoney100;

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

		if (buyAmount < 0) // Sell (Export)
		{
			// TODO: ?? this is cheating the system, getting paid more than actually sold, calculate trade gain here?

			int32 sellAmount = abs(buyAmount);
			sellAmount = min(sellAmount, resourceSys.resourceCount(resourceEnum));

			int32 tradeMoney100 = sellAmount * simulation->price100(resourceEnum);
			totalExportMoney100 += tradeMoney100;

			resourceSys.RemoveResourceGlobal(resourceEnum, sellAmount);
			resourceSys.ChangeMoney100(tradeMoney100);
		}
		else if (buyAmount > 0) // Buy (Import)
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

	//int32 totalTradeMoney100 = totalImportMoney100 + totalExportMoney100;

	int32 exportFee100 = totalExportMoney100 * tradingFeePercent / 100;
	int32 importFee100 = totalImportMoney100 * tradingFeePercent / 100;
	int32 fee100 = exportFee100 + importFee100;
	//int32 fee100 = totalTradeMoney100 * tradingFeePercent / 100;
	resourceSys.ChangeMoney100(-fee100);
	
	simulation->QuestUpdateStatus(playerId, QuestEnum::TradeQuest, abs((totalExportMoney100 + totalImportMoney100 + fee100) / 100));

	//simulation->uiInterface()->ShowFloatupInfo(FloatupEnum::GainMoney, tile, ToSignedNumber(tradeCommand.totalGain));

	exportMoney100 = totalExportMoney100 - exportFee100;
	importMoney100 = totalImportMoney100 + importFee100;

	simulation->uiInterface()->ShowFloatupInfo(FloatupEnum::GainMoney, tile, ToSignedNumber((exportMoney100 - importMoney100) / 100));
}


void TradeBuilding::OnTick1Sec()
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
		_exportGain100Last2Round = _exportGain100Last1Round;
		_exportGain100Last1Round = 0;

		_importLoss100Last2Round = _importLoss100Last1Round;
		_importLoss100Last1Round = 0;
	}
}

/*
 * TradingPost/Port
 */
const FText FeeDiscountText = LOCTEXT("Fee Discount", "Fee Discount");
const FText FastDeliveryText = LOCTEXT("Fast Delivery", "Fast Delivery");
const FText IncreaseLoadText = LOCTEXT("Increased Load", "Increased Load");
const FText SpeedBoostText = LOCTEXT("Speed Boost", "Speed Boost");

void TradingPost::FinishConstruction() {
	TradeBuilding::FinishConstruction();

	for (ResourceInfo info : ResourceInfos) {
		AddResourceHolder(info.resourceEnum, ResourceHolderType::Provider, 0);
	}

	_upgrades = {
		BuildingUpgrade(FeeDiscountText, LOCTEXT("Fee Discount Desc1", "Decrease trading fee by 5%."), 250),
		BuildingUpgrade(FastDeliveryText, LOCTEXT("Fast Delivery Desc1", "-30% the time it takes, for traders to arrive (+50% efficiency)."), 300),
		BuildingUpgrade(IncreaseLoadText, LOCTEXT("Increased Load Desc1", "+120 goods quantity per trade."), 200),
	};

	_simulation->TryAddQuest(_playerId, std::make_shared<TradeQuest>());

	TrailerAddResource();
}

std::vector<BonusPair> TradingPost::GetBonuses()
{
	std::vector<BonusPair> result;
	if (IsUpgraded(1)) {
		result.push_back({ FastDeliveryText, 50 });
	}
	if (_simulation->playerOwned(_playerId).HasSpeedBoost(buildingId())) {
		result.push_back({ SpeedBoostText, 50 });
	}

	return result;
}



/*
 * TradingPort
 */
void TradingPort::FinishConstruction() {
	TradeBuilding::FinishConstruction();

	for (ResourceInfo info : ResourceInfos) {
		AddResourceHolder(info.resourceEnum, ResourceHolderType::Provider, 0);
	}

	_upgrades = {
		BuildingUpgrade(FeeDiscountText, LOCTEXT("Fee Discount Desc2", "Decrease trading fee by 5%."), 250),
		BuildingUpgrade(FastDeliveryText, LOCTEXT("Fast Delivery Desc2", "Halve the time it takes, for traders to arrive (+100% efficiency)."), 500),
		BuildingUpgrade(IncreaseLoadText, LOCTEXT("Increased Load Desc2", "+240 goods quantity per trade."), 300),
	};

	_simulation->TryAddQuest(_playerId, std::make_shared<TradeQuest>());

	TrailerAddResource();
}

std::vector<BonusPair> TradingPort::GetBonuses() 
{
	std::vector<BonusPair> result;
	if (IsUpgraded(1)) {
		result.push_back({ FastDeliveryText, 100 });
	}
	if (_simulation->playerOwned(_playerId).HasSpeedBoost(buildingId())) {
		result.push_back({ SpeedBoostText, 50 });
	}

	return result;
}




/*
 * TradingCompany
 */
void TradingCompany::FinishConstruction() {
	TradeBuilding::FinishConstruction();

	for (ResourceInfo info : ResourceInfos) {
		AddResourceHolder(info.resourceEnum, ResourceHolderType::Provider, 0);
	}

	_upgrades = {
		BuildingUpgrade(FeeDiscountText,								LOCTEXT("Fee Discount Desc3", "Decrease trading fee by 5%."), 250),
		BuildingUpgrade(LOCTEXT("Efficient Hauling", "Efficient Hauling"), LOCTEXT("Efficient Hauling Desc3", "+60 goods quantity per trade."), 350),
		BuildingUpgrade(LOCTEXT("Marine Trade", "Marine Trade"),		LOCTEXT("Marine Trade Desc3", "+60 goods quantity per trade, if adjacent to a trading port."), 200),
	};

	_simulation->TryAddQuest(_playerId, std::make_shared<TradeQuest>());

	needTradingCompanySetup = true;

	ResetTradeRetryTick();
}

std::vector<BonusPair> TradingCompany::GetBonuses()
{
	std::vector<BonusPair> result;
	if (_simulation->playerOwned(_playerId).HasSpeedBoost(buildingId())) {
		result.push_back({ SpeedBoostText, 50 });
	}
	return result;
}

void TradingCompany::OnTick1Sec()
{
	TradeBuilding::OnTick1Sec();

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

/*
 * OreSupplier
 */
void OreSupplier::FinishConstruction() {
	Building::FinishConstruction();

	_upgrades = {
		BuildingUpgrade(INVTEXT("More Ore"), INVTEXT("Buy 10 more ore each round"), 200),
		BuildingUpgrade(INVTEXT("Even More Ore"), INVTEXT("Buy 20 more ore each round"), 500),
	};
}
void OreSupplier::TickRound()
{

}


#undef LOCTEXT_NAMESPACE