// Pun Dumnernchanvanit's

#pragma once

#include "PunButtonImage.h"
#include "PunButtonImageWithGlow.generated.h"

/**
 * 
 */
UCLASS()
class PROTOTYPECITY_API UPunButtonImageWithGlow : public UPunButtonImage
{
	GENERATED_BODY()
public:

	UPROPERTY(meta = (BindWidget)) UImage* HoverGlowImage;

	virtual void OnInit() override {
		UPunButtonImage::OnInit();
		
		HoverGlowImage->SetVisibility(ESlateVisibility::Collapsed);
		Button1->OnHovered.Clear();
		Button1->OnHovered.AddUniqueDynamic(this, &UPunButtonImageWithGlow::OnHover);
		Button1->OnUnhovered.Clear();
		Button1->OnUnhovered.AddUniqueDynamic(this, &UPunButtonImageWithGlow::OnUnhover);
	}

	UFUNCTION() void OnHover() {
		HoverGlowImage->SetVisibility(ESlateVisibility::SelfHitTestInvisible);
	}
	UFUNCTION() void OnUnhover() {
		HoverGlowImage->SetVisibility(ESlateVisibility::Collapsed);
	}

	
};
