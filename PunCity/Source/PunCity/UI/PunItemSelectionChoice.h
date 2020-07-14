// Pun Dumnernchanvanit's

#pragma once

#include "CoreMinimal.h"
#include "PunCity/UI/PunWidget.h"
#include "PunItemIcon.h"
#include "PunItemSelectionChoice.generated.h"

/**
 * 
 */
UCLASS()
class UPunItemSelectionChoice : public UPunWidget
{
	GENERATED_BODY()
public:
	UPROPERTY(meta = (BindWidget)) UPunItemIcon* ItemIconWidget;

	UPROPERTY(meta = (BindWidget)) UTextBlock* ItemName;
	UPROPERTY(meta = (BindWidget)) UTextBlock* ItemText;

	UPROPERTY(meta = (BindWidget)) UButton* ItemSelectButton;

	UPROPERTY() UWidget* parentWidget;

	ItemEnum itemEnum = ItemEnum::None;

	void PunInit();
	UFUNCTION() void ClickItemSelect();
};
