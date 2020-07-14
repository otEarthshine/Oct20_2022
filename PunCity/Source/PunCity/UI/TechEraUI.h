// Pun Dumnernchanvanit's

#pragma once

#include "CoreMinimal.h"
#include "PunCity/UI/PunWidget.h"
#include "TechEraUI.generated.h"

/**
 * 
 */
UCLASS()
class PROTOTYPECITY_API UTechEraUI : public UPunWidget
{
	GENERATED_BODY()
public:
	UPROPERTY(meta = (BindWidget)) UTextBlock* EraText;
	UPROPERTY(meta = (BindWidget)) UTextBlock* EraUnlockText;
	UPROPERTY(meta = (BindWidget)) UVerticalBox* TechList;
};
