// Pun Dumnernchanvanit's

#pragma once

#include "CoreMinimal.h"
#include "PunWidget.h"
#include "BattleFieldMiniUI.generated.h"

/**
 * 
 */
UCLASS()
class PROTOTYPECITY_API UBattleFieldMiniUI : public UPunWidget
{
	GENERATED_BODY()
public:
	UPROPERTY(meta = (BindWidget)) UOverlay* BattleOverlay;

	UPROPERTY(meta = (BindWidget)) URichTextBlock* BattleText;

	UPROPERTY(meta = (BindWidget)) UImage* PlayerLogoLeft;
	UPROPERTY(meta = (BindWidget)) UImage* PlayerLogoRight;
	UPROPERTY(meta = (BindWidget)) UImage* BattleBarImage;

	UPROPERTY(meta = (BindWidget)) URichTextBlock* LeftArmyStrength;
	UPROPERTY(meta = (BindWidget)) URichTextBlock* RightArmyStrength;

	virtual bool IsMiniUI() { return true; }

	void UpdateUIBase(int32 provinceIdIn, const ProvinceClaimProgress& claimProgress);

	
};
