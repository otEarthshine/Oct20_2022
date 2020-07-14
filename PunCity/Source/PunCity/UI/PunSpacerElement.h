// Pun Dumnernchanvanit's

#pragma once

#include "PunCity/UI/PunWidget.h"

#include "PunSpacerElement.generated.h"

/**
 * 
 */
UCLASS()
class UPunSpacerElement : public UPunWidget
{
	GENERATED_BODY()
public:
	UPROPERTY(meta = (BindWidget)) USpacer* Spacer;
};
