// Pun Dumnernchanvanit's

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "PunWidget.h"
#include "CardSlot.generated.h"

/**
 * 
 */
UCLASS()
class PROTOTYPECITY_API UCardSlot : public UPunWidget
{
	GENERATED_BODY()
public:
	UPROPERTY(meta = (BindWidget)) UImage* CardSlot;
};
