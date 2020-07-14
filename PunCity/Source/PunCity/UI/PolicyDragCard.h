// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "GameUIDataSource.h"
#include <memory>
#include "../Simulation/Policy.h"

#include "PolicyDragCard.generated.h"

/**
 * 
 */
UCLASS()
class UPolicyDragCard : public UUserWidget
{
	GENERATED_BODY()
public:
	UPROPERTY(meta = (BindWidget)) class UTextBlock* CardText;
	std::shared_ptr<Policy> policy;

	class UDragCardSlot* cardSlot;
};
