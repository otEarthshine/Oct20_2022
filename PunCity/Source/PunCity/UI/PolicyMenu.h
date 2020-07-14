// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "../Simulation/Policy.h"
#include "PunWidget.h"
#include "TradeUIWidgetBase.h"

#include <memory>

#include "PolicyMenu.generated.h"

/**
 * 
 */

UCLASS()
class UPolicyMenu : public UPunWidget
{
	GENERATED_BODY()
public: 
	//UFUNCTION(BlueprintCallable, Category = UPolicyMenu) void DropCardOnSlot();

	UPROPERTY(meta = (BindWidget)) class UWrapBox* PolicySlotList;
	UPROPERTY(meta = (BindWidget)) class UWrapBox* PolicyCardList;
	UPROPERTY(meta = (BindWidget)) class UButton* DoneButton;
	UPROPERTY(meta = (BindWidget)) class UTextBlock* PolicyMenuTitleText;

	void PunInit(UTradeUIWidgetBase* tradeWidget);
	void TickUI();
private:
	class UPolicyDragCard* CreatePolicyCard(std::shared_ptr<Policy> policy);

	UFUNCTION() void DoneButtonDown();
	
private:
	UTradeUIWidgetBase* _tradeWidget;
};
