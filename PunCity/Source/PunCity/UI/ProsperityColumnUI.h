// Pun Dumnernchanvanit's

#pragma once

#include "CoreMinimal.h"
#include "PunWidget.h"
#include "ProsperityColumnUI.generated.h"

/**
 * 
 */
UCLASS()
class PROTOTYPECITY_API UProsperityColumnUI : public UUserWidget
{
	GENERATED_BODY()
public:
	UPROPERTY(meta = (BindWidget)) UTextBlock* HouseLevelText;
	UPROPERTY(meta = (BindWidget)) UVerticalBox* ProsperityTechList;
};
