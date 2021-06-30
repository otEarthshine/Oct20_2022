// Pun Dumnernchanvanit's

#pragma once

#include "CoreMinimal.h"
#include "PunCity/UI/PunWidget.h"
#include "ExclamationIcon.generated.h"

/**
 * 
 */

UCLASS()
class PROTOTYPECITY_API UExclamationIcon : public UPunWidget
{
	GENERATED_BODY()
public:
	void SetShow(bool active)
	{
		SetVisibility(active ? ESlateVisibility::HitTestInvisible : ESlateVisibility::Collapsed);
		
		//if (active && GetVisibility() == ESlateVisibility::Collapsed) {
		//	SetVisibility(ESlateVisibility::HitTestInvisible);

		//	if (Animations.Num() == 0) {
		//		GetAnimations(Animations);
		//	}
		//	TryPlayAnimation("Pulse");
		//}
		//else if (!active && GetVisibility() != ESlateVisibility::Collapsed) {
		//	SetVisibility(ESlateVisibility::Collapsed);
		//}
	}

};
