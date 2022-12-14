// Pun Dumnernchanvanit's

#pragma once

#include "CoreMinimal.h"
#include "PunCity/UI/PunWidget.h"

#include "PunRichTextTwoSided.generated.h"

/**
 * 
 */
UCLASS()
class UPunRichTextTwoSided : public UPunWidget
{
	GENERATED_BODY()
public:
	UPROPERTY(meta = (BindWidget)) URichTextBlock* PunRichText;
	UPROPERTY(meta = (BindWidget)) URichTextBlock* PunRichTextRight;

	// Note, thiis gets swapped to the left..
	UPROPERTY(meta = (BindWidget)) USizeBox* RightImageSizeBox;
	UPROPERTY(meta = (BindWidget)) UImage* RightImage;

	UPROPERTY(meta = (BindWidget)) USizeBox* ExpanderBox;
	UPROPERTY(meta = (BindWidget)) UButton* ExpanderButton;
	UPROPERTY(meta = (BindWidget)) UCanvasPanel* ExpandedCanvas;
	UPROPERTY(meta = (BindWidget)) UTextBlock* ExpandedText;

	void SetText(std::string leftText, std::string rightText, ResourceEnum resourceEnum = ResourceEnum::None, std::string expandedText = "") {
		SetText(ToFText(leftText), ToFText(rightText), resourceEnum, ToFText(expandedText));
	}

	void SetText(FText leftText, FText rightText, ResourceEnum resourceEnum = ResourceEnum::None, FText expandedText = FText())
	{
		PunRichText->SetText(leftText);
		PunRichTextRight->SetText(rightText);

		if (resourceEnum == ResourceEnum::None) {
			RightImageSizeBox->SetVisibility(ESlateVisibility::Collapsed);
		}
		else {
			RightImageSizeBox->SetVisibility(ESlateVisibility::HitTestInvisible);

			UMaterialInstanceDynamic* materialInstance = UMaterialInstanceDynamic::Create(assetLoader()->ResourceIconMaterial, this);
			materialInstance->SetTextureParameterValue("ColorTexture", assetLoader()->GetResourceIcon(resourceEnum));
			materialInstance->SetTextureParameterValue("DepthTexture", assetLoader()->GetResourceIconAlpha(resourceEnum));
			RightImage->SetBrushFromMaterial(materialInstance);
		}

		if (!expandedText.IsEmpty())
		{
			ExpanderBox->SetVisibility(ESlateVisibility::SelfHitTestInvisible);
			ExpanderButton->OnClicked.Clear();
			ExpanderButton->OnClicked.AddDynamic(this, &UPunRichTextTwoSided::OnClickExpand);

			if (!isInitialized) {
				ExpandedCanvas->SetVisibility(ESlateVisibility::Collapsed);
			}
			ExpandedText->SetText(expandedText);
		}
		else {
			ExpanderBox->SetVisibility(ESlateVisibility::Collapsed);
			ExpandedCanvas->SetVisibility(ESlateVisibility::Collapsed);
		}

		isInitialized = true;
	}
	

	UFUNCTION() void OnClickExpand() {
		ExpandedCanvas->SetVisibility(ExpandedCanvas->GetVisibility() == ESlateVisibility::Collapsed ? ESlateVisibility::SelfHitTestInvisible : ESlateVisibility::Collapsed);
	}

	bool isInitialized = false;
};
