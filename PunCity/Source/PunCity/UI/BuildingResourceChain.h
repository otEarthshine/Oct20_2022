// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "IconTextPairWidget.h"
#include "PunWidget.h"
#include "BuildingResourceChain.generated.h"

/**
 * 
 */
UCLASS()
class UBuildingResourceChain : public UPunWidget
{
	GENERATED_BODY()
public:
	UPROPERTY(meta = (BindWidget)) UIconTextPairWidget* IconPair1;
	UPROPERTY(meta = (BindWidget)) UIconTextPairWidget* IconPair2;
	UPROPERTY(meta = (BindWidget)) UIconTextPairWidget* IconPair3;

	UPROPERTY(meta = (BindWidget)) UHorizontalBox* ProductionArrowBox;

	UPROPERTY(meta = (BindWidget)) UImage* ProductImage;
	UPROPERTY(meta = (BindWidget)) UTextBlock* ProductText;

	void SetResource(ResourcePair input1, ResourcePair input2, ResourcePair output, UTexture2D* productTexture = nullptr, std::string productStr = "") 
	{
		bool hasInput1 = input1.resourceEnum != ResourceEnum::None;
		bool hasInput2 = input2.resourceEnum != ResourceEnum::None;
		ProductionArrowBox->SetVisibility((hasInput1 || hasInput2) ? ESlateVisibility::SelfHitTestInvisible : ESlateVisibility::Collapsed);

		if (hasInput1) {
			IconPair1->SetText("", std::to_string(input1.count));
			SetChildHUD(IconPair1);
			IconPair1->SetImage(input1.resourceEnum, dataSource()->assetLoader(), true);
		}
		IconPair1->SetVisibility(hasInput1 ? ESlateVisibility::SelfHitTestInvisible : ESlateVisibility::Collapsed);
		
		if (hasInput2) {
			IconPair2->SetText("", std::to_string(input2.count));
			SetChildHUD(IconPair2);
			IconPair2->SetImage(input2.resourceEnum, dataSource()->assetLoader(), true);
		}
		IconPair2->SetVisibility(hasInput2 ? ESlateVisibility::SelfHitTestInvisible : ESlateVisibility::Collapsed);

		if (output.resourceEnum != ResourceEnum::None)
		{
			IconPair3->SetText("", std::to_string(output.count));
			SetChildHUD(IconPair3);
			IconPair3->SetImage(output.resourceEnum, dataSource()->assetLoader(), true);

			IconPair3->SetVisibility(ESlateVisibility::SelfHitTestInvisible);
			ProductImage->SetVisibility(ESlateVisibility::Collapsed);
			ProductText->SetVisibility(ESlateVisibility::Collapsed);
		}
		else
		{
			ProductImage->SetBrushFromTexture(productTexture);
			SetText(ProductText, productStr);

			IconPair3->SetVisibility(ESlateVisibility::Collapsed);
			ProductImage->SetVisibility(ESlateVisibility::SelfHitTestInvisible);
			ProductText->SetVisibility(ESlateVisibility::SelfHitTestInvisible);
		}
	}
};
