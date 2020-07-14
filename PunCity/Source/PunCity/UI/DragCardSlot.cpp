// Fill out your copyright notice in the Description page of Project Settings.

#include "DragCardSlot.h"
#include "PolicyDragCard.h"
#include "Blueprint/DragDropOperation.h"


bool UDragCardSlot::NativeOnDrop(const FGeometry& InGeometry, const FDragDropEvent& InDragDropEvent, UDragDropOperation* InOperation)
{
	Super::NativeOnDrop(InGeometry, InDragDropEvent, InOperation);

	if (InOperation->Payload) UE_LOG(LogTemp, Error, TEXT("NativeOnDrop: %s"), *(InOperation->Payload->GetClass()->GetName()));

	policyDragCard = Cast<UPolicyDragCard>(InOperation->Payload);

	// Remove from old parent
	// TODO: find a way with just widgets
	if (policyDragCard->cardSlot != nullptr) {
		policyDragCard->cardSlot->policyDragCard = nullptr;
	}

	UE_LOG(LogTemp, Error, TEXT("NativeOnDrop End %d"), policyDragCard != nullptr);
	return true;
}