// Pun Dumnernchanvanit's

#pragma once

#include "PunSpineWidget.h"
#include "PunWidget.h"
#include "WG_PunSpine.h"

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
	UPROPERTY() TArray<UWG_PunSpine*> FXSpines;

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

	void AddFXSpine(USpineAtlasAsset* atlas_fx, USpineSkeletonDataAsset* skeletonData_fx, float gameSpeed)
	{
		UWG_PunSpine* fxSpine = nullptr;
		for (int32 i = 0; i < FXSpines.Num(); i++) {
			if (!FXSpines[i]->IsVisible()) {
				fxSpine = FXSpines[i];
				break;
			}
		}

		if (!fxSpine)
		{
			fxSpine = AddWidget<UWG_PunSpine>(UIEnum::WG_PunSpine);
			FXSpines.Add(fxSpine);
			FXOverlay->AddChildToOverlay(fxSpine);
		}

		fxSpine->Spine->Atlas = atlas_fx;
		fxSpine->Spine->SkeletonData = skeletonData_fx;
		fxSpine->Spine->SetAnimation(0, "Attack", false);
		fxSpine->Spine->SetTimeScale(gameSpeed);
		fxSpine->animationDoneSec = GetWorld()->GetTimeSeconds() + ProvinceClaimProgress::AnimationLengthSecs / gameSpeed;
		fxSpine->SetVisibility(ESlateVisibility::HitTestInvisible);
	}

};
