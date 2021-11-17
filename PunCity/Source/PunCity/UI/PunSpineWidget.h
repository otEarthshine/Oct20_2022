// Pun Dumnernchanvanit's

#pragma once

#include "PunWidget.h"
#include "SpineWidget.h"
#include "PunSpineWidget.generated.h"

/**
 * 
 */
UCLASS()
class PROTOTYPECITY_API UPunSpineWidget : public USpineWidget
{
	GENERATED_BODY()
public:
	void PunTick()
	{
		Tick(GetWorld()->GetDeltaSeconds(), false);

		
		//if (UTrackEntry* track = GetCurrent(0)) 
		//{
		//	//PUN_LOG("SpineTrack: %s, duration%f trackTime:%f", *track->getAnimationName(), track->getAnimationDuration(), track->GetTrackTime());
		//	if (track->GetTrackEntry() == nullptr) {
		//		SetAnimation(0, "Idle", true);
		//		PUN_LOG("SpineTrack: PLAY %s", *track->getAnimationName());
		//	}

		//	if (track->getAnimationName().Len() > 0) {
		//		Tick(GetWorld()->GetDeltaSeconds(), false);
		//	}
		//} else {
		//	PUN_LOG("SpineTrack: None");
		//}

		//if (AnimationComplete.GetAllObjects().Num() == 0) {
		//	AnimationComplete.AddUniqueDynamic(this, &UPunSpineWidget::OnAnimationComplete);
		//	SetAnimation(0, "Idle", true);
		//}
	}

	//UFUNCTION() void OnAnimationComplete(UTrackEntry* entry) {
	//	SetAnimation(0, "Idle", true);
	//}
	
};
