// Pun Dumnernchanvanit's

#pragma once

#include "PunWidget.h"
#include "SpineWidget.h"
#include "WG_PunSpine.generated.h"

/**
 * 
 */
UCLASS()
class PROTOTYPECITY_API UWG_PunSpine : public UPunWidget
{
	GENERATED_BODY()
public:
	UPROPERTY(meta = (BindWidget)) USpineWidget* Spine;


	void PunTick()
	{
		Spine->Tick(GetWorld()->GetDeltaSeconds(), false);
	}

	float animationDoneSec = -1;

	
};
