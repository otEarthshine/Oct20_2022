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

	// Note, this gets swapped to the left. ???
	UPROPERTY(meta = (BindWidget)) URichTextBlock* PunRichTextRight1;
	//UPROPERTY(meta = (BindWidget)) USizeBox* RightImageSizeBox1;
	UPROPERTY(meta = (BindWidget)) UImage* RightImage1;

	UPROPERTY(meta = (BindWidget)) URichTextBlock* PunRichTextRight2;
	//UPROPERTY(meta = (BindWidget)) USizeBox* RightImageSizeBox2;
	UPROPERTY(meta = (BindWidget)) UImage* RightImage2;

	UPROPERTY(meta = (BindWidget)) USizeBox* ExpanderBox;
	UPROPERTY(meta = (BindWidget)) UButton* ExpanderButton;
	UPROPERTY(meta = (BindWidget)) UCanvasPanel* ExpandedCanvas;
	UPROPERTY(meta = (BindWidget)) UTextBlock* ExpandedText;

	void SetText(std::string leftText, std::string rightText, ResourceEnum resourceEnum = ResourceEnum::None, std::string expandedText = "") {
		SetText(ToFText(leftText), ToFText(rightText), resourceEnum, ToFText(expandedText));
	}

	void SetText(FText leftText, 
		FText rightText1, ResourceEnum resourceEnum1 = ResourceEnum::None, FText expandedText = FText(), 
		FText rightText2 = FText(), ResourceEnum resourceEnum2 = ResourceEnum::None)
	{
		PunRichText->SetText(leftText);

		auto setImage = [&](FText text, URichTextBlock* textBlock, ResourceEnum resourceEnum, UImage* image)
		{
			textBlock->SetVisibility(text.IsEmpty() ? ESlateVisibility::Collapsed : ESlateVisibility::HitTestInvisible);
			textBlock->SetText(text);
			
			if (resourceEnum == ResourceEnum::None) {
				image->SetVisibility(ESlateVisibility::Collapsed);
			}
			else {
				image->SetVisibility(ESlateVisibility::HitTestInvisible);
				image->GetDynamicMaterial()->SetTextureParameterValue("ColorTexture", assetLoader()->GetResourceIcon(resourceEnum));
				image->GetDynamicMaterial()->SetTextureParameterValue("DepthTexture", assetLoader()->GetResourceIconAlpha(resourceEnum));
			}
		};

		PunRichTextRight1->SetText(rightText1);

		setImage(rightText1, PunRichTextRight1, resourceEnum1, RightImage1);
		setImage(rightText2, PunRichTextRight2, resourceEnum2, RightImage2);

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
