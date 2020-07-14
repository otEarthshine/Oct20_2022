// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "PunWidget.h"

class UTradeUIWidgetBase;

#include "TradeRowUIWidgetBase.generated.h"
/**
 * DEPRECATED
 */
UCLASS()
class UTradeRowUIWidgetBase : public UUserWidget
{
	GENERATED_BODY()
public:
//	void SetupInterfaces(IGameUIDataSource* dataSource, IGameNetworkInterface* networkInterface, UTradeUIWidgetBase* tradeUIWidget);
//	void UpdateNonTradeAmountTexts();
//
//	UPROPERTY(meta = (BindWidget)) class UButton* ArrowDownButton;
//	UPROPERTY(meta = (BindWidget)) class UButton* ArrowUpButton;
//
//	UPROPERTY(meta = (BindWidget)) class UTextBlock* ResourceNameText;
//	UPROPERTY(meta = (BindWidget)) class UTextBlock* ResourceCostText;
//	UPROPERTY(meta = (BindWidget)) class UTextBlock* MaxAmountText;
//
//
//	UPROPERTY(meta = (BindWidget)) class UEditableTextBox* TradeAmount;
//
//	//! Set by parent
//	ResourceEnum resourceEnum;
//	int32 maxTradeAmount;
//	int32 tradeAmount;
//
//	void CommitTradeAmount(bool setText);
//
//private:
//	UFUNCTION() void ClickArrowDownButton();
//	UFUNCTION() void ClickArrowUpButton();
//	UFUNCTION() void TradeAmountTextChanged(const FText& Text, ETextCommit::Type CommitMethod);
//	
//private:
//	IGameUIDataSource* _dataSource;
//	IGameNetworkInterface* _networkInterface;
//	UTradeUIWidgetBase* _tradeWidget;
};
