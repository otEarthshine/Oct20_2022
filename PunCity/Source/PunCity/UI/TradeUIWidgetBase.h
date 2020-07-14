// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "GameUIDataSource.h"
#include "../GameNetworkInterface.h"
#include "PunWidget.h"

#include <unordered_map>

class UTradeRowUIWidgetBase;

#include "TradeUIWidgetBase.generated.h"
/**
 * DEPRECATED
 */
UCLASS()
class UTradeUIWidgetBase : public UPunWidget
{
	GENERATED_BODY()
public:
//	void PunInit();
//
//	void OpenUI(int32_t objectId = -1);
//	void TickUI();
//	void UpdateTotal();
//	UFUNCTION() void ClickedTradeButton();
//	UFUNCTION() void ClickedDismissButton();
//
//	UPROPERTY(meta = (BindWidget)) class UScrollBox* BuyRows;
//	UPROPERTY(meta = (BindWidget)) class UScrollBox* SellRows;
//	
//	UPROPERTY(meta = (BindWidget)) class UButton* TradeButton;
//	UPROPERTY(meta = (BindWidget)) class UButton* DismissButton;
//
//	UPROPERTY(meta = (BindWidget)) class UTextBlock* Total;
//
//private:
//	//void UpdateBuyRow(ResourceEnum resourceEnum, class ResourceSystem& resourceSystem);
//	//void UpdateSellRow(ResourceEnum resourceEnum, class ResourceSystem& resourceSystem);
//
//private:
//	//! TODO: refactor out of unordered_map
//	std::unordered_map<ResourceEnum, UTradeRowUIWidgetBase*> _buyPairs;
//	std::unordered_map<ResourceEnum, UTradeRowUIWidgetBase*> _sellPairs;
//	int _totalGain;
};
