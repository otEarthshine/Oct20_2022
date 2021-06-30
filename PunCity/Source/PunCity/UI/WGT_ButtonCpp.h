// Pun Dumnernchanvanit's

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "Components/Button.h"

#include "WGT_ButtonCpp.generated.h"

/**
 * 
 */
UCLASS()
class PROTOTYPECITY_API UWGT_ButtonCpp : public UUserWidget
{
	GENERATED_BODY()
public:

	UPROPERTY(BlueprintReadWrite, meta = (BindWidget)) UButton* CoreButton;
};
