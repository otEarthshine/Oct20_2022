#include "TradeRowUIWidgetBase.h"
#include "Components/Button.h"
#include "Components/TextBlock.h"
#include "Components/EditableTextBox.h"
#include "TradeUIWidgetBase.h"
#include "../Simulation/GameSimulationInfo.h"
#include "../Simulation/GameSimulationCore.h"
#include "../Simulation/Resource/ResourceSystem.h"
#include <string>
#include <sstream>

using namespace std;

//void UTradeRowUIWidgetBase::SetupInterfaces(IGameUIDataSource* dataSource, IGameNetworkInterface* networkInterface, UTradeUIWidgetBase* tradeUIWidget)
//{
//	_dataSource = dataSource;
//	_networkInterface = networkInterface;
//	_tradeWidget = tradeUIWidget;
//	ArrowDownButton->OnClicked.AddDynamic(this, &UTradeRowUIWidgetBase::ClickArrowDownButton);
//	ArrowUpButton->OnClicked.AddDynamic(this, &UTradeRowUIWidgetBase::ClickArrowUpButton);
//	TradeAmount->OnTextCommitted.AddDynamic(this, &UTradeRowUIWidgetBase::TradeAmountTextChanged);
//
//	check(_tradeWidget);
//}
//
//void UTradeRowUIWidgetBase::UpdateNonTradeAmountTexts()
//{
//	ResourceNameText->SetText(FText::FromString(FString(ResourceName(resourceEnum).c_str())));
//	ResourceCostText->SetText(FText::FromString(FString::FromInt(ResourceInfos[(int)resourceEnum].basePrice)));
//	MaxAmountText->SetText(FText::FromString(FString::FromInt(maxTradeAmount)));
//}
//
//void UTradeRowUIWidgetBase::ClickArrowDownButton()
//{
//	tradeAmount -= 10;
//	CommitTradeAmount(true);
//}
//void UTradeRowUIWidgetBase::ClickArrowUpButton()
//{
//	tradeAmount += 10;
//	CommitTradeAmount(true);
//}
//
//void UTradeRowUIWidgetBase::TradeAmountTextChanged(const FText& Text, ETextCommit::Type CommitMethod) 
//{
//	if (CommitMethod == ETextCommit::OnEnter || CommitMethod == ETextCommit::OnUserMovedFocus) {
//		//PUN_DEBUG(FString("TradeAmountTextChanged Inside"));
//		FString tradeAmountString = Text.ToString();
//		tradeAmount = tradeAmountString.IsNumeric() ? FCString::Atoi(*tradeAmountString) : 0;
//		CommitTradeAmount(true);
//	}
//}
//
//void UTradeRowUIWidgetBase::CommitTradeAmount(bool setText)
//{
//	// Cap trade amount
//	tradeAmount = max(0, min(maxTradeAmount, tradeAmount));
//
//	// Update the total sum
//	_tradeWidget->UpdateTotal();
//
//	if (setText) {
//		TradeAmount->SetText(FText::FromString(FString::FromInt(tradeAmount)));
//	}
//}