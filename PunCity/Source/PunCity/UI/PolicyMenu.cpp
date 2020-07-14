// Fill out your copyright notice in the Description page of Project Settings.

#include "PolicyMenu.h"
#include "UObject/ConstructorHelpers.h"
#include "PolicyDragCard.h"
#include "DragCardSlot.h"
#include "Components/WrapBox.h"
#include "Components/Button.h"
#include "Components/TextBlock.h"
#include "Components/Overlay.h"
#include "Components/Image.h"
#include "../Simulation/GameSimulationCore.h"
#include "../Simulation/PolicySystem.h"
#include "../Simulation/UnlockSystem.h"
#include "TradeUIWidgetBase.h"
#include "PunCity/PunUtils.h"

#include <unordered_set>

//#include "DragCardSlot.h"

using namespace std;

void UPolicyMenu::PunInit(UTradeUIWidgetBase* tradeWidget)
{
	UE_LOG(LogTemp, Error, TEXT("PolicyMenu SetupInterfaces"));

	_tradeWidget = tradeWidget;

	DoneButton->OnClicked.AddDynamic(this, &UPolicyMenu::DoneButtonDown);
	SetVisibility(ESlateVisibility::Collapsed);
}

void UPolicyMenu::TickUI()
{
	// TODO: decide what to do with policy system...
	return;

	//if (policyMenuState != PolicyMenuStateEnum::None) 
	//{
	//	PolicyMenuTitleText->SetText(FText::FromString(policyMenuState == PolicyMenuStateEnum::Spring ?
	//									FString("Choose Policies (Spring/Summer)") : FString("Choose Policies (Autumn/Winter)")));

	//	// Refresh and show menu
	//	
	//	// Remove all old cards/slots
	//	PolicySlotList->ClearChildren();
	//	PolicyCardList->ClearChildren();
	//	
	//	auto& simulation = dataSource()->simulation();

	//	// PolicySlotList
	//	for (int i = 0; i < simulation.unlockSystem(playerId())->policySlotCount; i++) {
	//		UDragCardSlot* slot = AddWidget<UDragCardSlot>(UIEnum::DragCardSlot);
	//		PolicySlotList->AddChild(slot);
	//	}

	//	const auto& cardsInUse = simulation.policySystem(playerId())->cardsInUse();
	//	//PUN_DEBUG(FString::Printf(TEXT("NeedShowPolicyMenu cardsInUse: %lu"), cardsInUse.size()));

	//	int32_t slotIndex = 0;
	//	for (const auto& it : cardsInUse) {
	//		auto slot = Cast<UDragCardSlot>(PolicySlotList->GetChildAt(slotIndex));
	//		UPolicyDragCard* dragCard = CreatePolicyCard(it.second);

	//		slot->SlotOverlay->AddChild(dragCard);
	//		slot->policyDragCard = dragCard;
	//		dragCard->cardSlot = slot;
	//		slotIndex++;
	//		//PUN_DEBUG(FString("Add to PolicySlotList") + FString(dragCard->policy->name().c_str()));
	//	}

	//	// PolicyCardList
	//	for (int i = 0; i < 10; i++) {
	//		UDragCardSlot* slot = AddWidget<UDragCardSlot>(UIEnum::DragCardSlot);
	//		slot->SlotImage->SetColorAndOpacity(FLinearColor(.1, .1, .1));
	//		slot->DragDropText->SetVisibility(ESlateVisibility::Hidden);
	//		PolicyCardList->AddChild(slot);
	//	}

	//	const auto& cardsAvailable = simulation.policySystem(playerId())->cardsAvailable();
	//	//PUN_DEBUG(FString::Printf(TEXT("NeedShowPolicyMenu cardsAvailable: %lu"), cardsAvailable.size()));

	//	slotIndex = 0;
	//	for (const auto& it : cardsAvailable) {
	//		auto slot = Cast<UDragCardSlot>(PolicyCardList->GetChildAt(slotIndex));
	//		UPolicyDragCard* dragCard = CreatePolicyCard(it.second);

	//		slot->SlotOverlay->AddChild(dragCard);
	//		slot->policyDragCard = dragCard;
	//		dragCard->cardSlot = slot;
	//		slotIndex++;

	//		//PUN_DEBUG(FString("Add to PolicyCardList: %s") + FString(dragCard->policy->name().c_str()));
	//	}

	//	SetVisibility(ESlateVisibility::Visible);
	//}
}

UPolicyDragCard* UPolicyMenu::CreatePolicyCard(shared_ptr<Policy> policy)
{
	auto dragCard = AddWidget<UPolicyDragCard>(UIEnum::DragCard);
	dragCard->policy = policy;

	FString policyText((policy->name() + ":\n\n" + policy->description()).c_str());
	dragCard->CardText->SetText(FText::Format(NSLOCTEXT("Policy","Policy", "{0}"), FText::FromString(policyText)));
	//UE_LOG(LogTemp, Error, TEXT("AddCard: %s"), *name);
	return dragCard;
}

void UPolicyMenu::DoneButtonDown()
{
	//bool isTrading = false;
	//TArray<uint8> policyInts;

	//for (int i = 0; i < PolicySlotList->GetChildrenCount(); i++) {
	//	if (auto card = Cast<UDragCardSlot>(PolicySlotList->GetChildAt(i))->policyDragCard) {
	//		PolicyEnum policyEnum = card->policy->policyEnum;
	//		policyInts.Add((uint8)policyEnum);

	//		if (policyEnum == PolicyEnum::Trade) isTrading = true;
	//	}
	//}

	//auto policiesSelectedCommand = make_shared<FPoliciesSelects>();
	//policiesSelectedCommand->policyArray = policyInts;
	//networkInterface()->SendNetworkCommand(policiesSelectedCommand);

	//UE_LOG(LogTemp, Error, TEXT("DoneButtonDown"));
	//SetVisibility(ESlateVisibility::Hidden);

	//if (isTrading) {
	//	_tradeWidget->OpenUI();
	//}
}