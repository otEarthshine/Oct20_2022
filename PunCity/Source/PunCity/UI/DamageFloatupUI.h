// Pun Dumnernchanvanit's

#pragma once

#include "CoreMinimal.h"
#include "PunCity/UI/PunWidget.h"
#include "DamageFloatupUI.generated.h"

/**
 * 
 */
UCLASS()
class PROTOTYPECITY_API UDamageFloatupUI : public UPunWidget
{
	GENERATED_BODY()
public:
	UPROPERTY(meta = (BindWidget)) UTextBlock* DamageText;

	float startTime = 0.0f;
};
