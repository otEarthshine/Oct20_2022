// Pun Dumnernchanvanit's

#pragma once

#include "CoreMinimal.h"
#include "PunWidget.h"
#include "BuffIcon.generated.h"

/**
 * 
 */
UCLASS()
class PROTOTYPECITY_API UBuffIcon : public UPunWidget
{
	GENERATED_BODY()
public:
	UPROPERTY(meta = (BindWidget)) UImage* BuffIconInner;
	UPROPERTY(meta = (BindWidget)) UImage* BuffIconOuter;
	UPROPERTY(meta = (BindWidget)) UTextBlock* BuffCountDown;

	void SetBuff(CardEnum buffEnum)
	{
		// Set Text
		{
			int32 ticksLeft = simulation().playerOwned(playerId()).GetBuffTicksLeft(buffEnum);
			int32 secsLeft = ticksLeft / Time::TicksPerSecond;
			std::stringstream ss;
			if (secsLeft < 100) {
				ss << secsLeft << "s";
			} else {
				ss << secsLeft / Time::SecondsPerMinute << "m";
			}

			SetText(BuffCountDown, ss.str());
		}
		
		if (buffEnum == CardEnum::KidnapGuard)
		{
			BuffIconInner->SetBrushFromTexture(assetLoader()->AdultIcon);
			BuffIconInner->SetBrushSize(FVector2D(15, 25));
			BuffIconInner->SetColorAndOpacity(FLinearColor(0.5, 0.483, 0.45));
		}
		else if (buffEnum == CardEnum::TreasuryGuard)
		{
			BuffIconInner->SetBrushFromTexture(assetLoader()->CoinIcon);
			BuffIconInner->SetBrushSize(FVector2D(18, 18));
			BuffIconInner->SetColorAndOpacity(FLinearColor(1, 1, 1));
		}
	}
	
};
