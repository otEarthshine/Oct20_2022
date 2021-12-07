// Pun Dumnernchanvanit's

#pragma once

#include "PunSpineWidget.h"
#include "PunWidget.h"

#include "BattleFieldUnitIcon.generated.h"

/**
 * 
 */
UCLASS()
class PROTOTYPECITY_API UBattleFieldUnitIcon : public UPunWidget
{
	GENERATED_BODY()
public:
	UPROPERTY(meta = (BindWidget)) UPunSpineWidget* UnitImage;
	
	UPROPERTY(meta = (BindWidget)) UImage* BackgroundImage;
	
	UPROPERTY(meta = (BindWidget)) UOverlay* DamageFloatupOverlay;

	UPROPERTY(meta = (BindWidget)) UTextBlock* UnitCountText;

	UPROPERTY(meta = (BindWidget)) UOverlay* FXOverlay;
	UPROPERTY() TArray<UPunSpineWidget*> FXSpines;

	int32 lastDamageTick = -1;
	int32 lastAttackTick = -1;
	int32 lastDeathTick = -1;


	void TickFXSpine()
	{
		for (int32 j = FXSpines.Num(); j-- > 0;)
		{
			if (FXSpines[j]->IsVisible())
			{
				if (GetWorld()->GetTimeSeconds() < FXSpines[j]->animationDoneSec)
				{
					FXSpines[j]->PunTick();
				}
				else {
					FXSpines[j]->SetVisibility(ESlateVisibility::Collapsed);
					FXSpines[j]->animationDoneSec = -1;
				}
			}
		}
	}

	void AddFXSpine(USpineAtlasAsset* atlas_fx, USpineSkeletonDataAsset* skeletonData_fx)
	{
		//UDamageFloatupUI* damageFloatup = AddWidget<UPunSpineWidget>(UIEnum::Pun);
		//
		//unitIcon->FXImage->Atlas = attackerSpineAsset.atlas_fx;
		//unitIcon->FXImage->SkeletonData = attackerSpineAsset.skeletonData_fx;
		//unitIcon->FXImage->SetAnimation(0, "Attack", false);
		//unitIcon->FXImage->SetTimeScale(sim.gameSpeedFloat());

		//unitIcon->FXCompleteTime = GetWorld()->GetTimeSeconds() + ProvinceClaimProgress::AnimationLengthSecs / sim.gameSpeedFloat();
	}

};
