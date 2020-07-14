// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "DragCardSlot.generated.h"

/**
 * 
 */
UCLASS()
class UDragCardSlot : public UUserWidget
{
	GENERATED_BODY()
public:
	class UPolicyDragCard* policyDragCard;
	
	UPROPERTY(BlueprintReadWrite, meta = (BindWidget)) class UOverlay* SlotOverlay;
	UPROPERTY(meta = (BindWidget)) class UImage* SlotImage;
	UPROPERTY(meta = (BindWidget)) class UTextBlock* DragDropText;
protected:
	bool NativeOnDrop(const FGeometry& InGeometry, const FDragDropEvent& InDragDropEvent, UDragDropOperation* InOperation) override;

};
