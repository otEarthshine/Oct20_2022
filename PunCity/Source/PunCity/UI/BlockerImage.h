// Pun Dumnernchanvanit's

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "PunCity/PunUtils.h"
#include "BlockerImage.generated.h"

/**
 * 
 */
UCLASS()

class UBlockerImage : public UUserWidget
{
	GENERATED_BODY()
public:
	FReply NativeOnMouseButtonDown(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent) override {
		PUN_LOG("NativeOnMouseButtonDown");
		if (InMouseEvent.GetEffectingButton() == EKeys::LeftMouseButton) {
			return FReply::Handled();
		}
		return FReply::Unhandled();
	}
	FReply NativeOnMouseButtonDoubleClick(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent) override {
		return FReply::Handled();
	}

};
