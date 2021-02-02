// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "PunCity/AssetLoaderComponent.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "PunWidget.h"

#include "IconTextPairWidget.generated.h"

/**
 * 
 */
UCLASS()
class UIconTextPairWidget : public UPunWidget
{
	GENERATED_BODY()
public:

	
	void SetText(std::string prefix, std::string suffix) {
		SetFString(ToFString(prefix), ToFString(suffix));
	}
	void SetFString(const FString& prefix, const FString& suffix) {
		PrefixText->SetText(FText::FromString(prefix));
		SuffixText->SetText(FText::FromString(suffix));
	}
	void SetText(const FText& prefix, const FText& suffix) {
		PrefixText->SetText(prefix);
		SuffixText->SetText(suffix);
	}

	void SetTextRed() {
		SetTextColor(FLinearColor(1, 0, 0));
	}
	void SetTextColorCoin() {
		SetTextColor(FLinearColor(1, 1, 0.7));
	}
	void SetTextColorScience() {
		SetTextColor(FLinearColor(0.7, 0.8, 1));
	}
	void SetTextColorCulture() {
		SetTextColor(FLinearColor(0.85, 0.7, 1));
	}
	void SetTextColor(FLinearColor color) {
		PrefixText->SetColorAndOpacity(color);
		SuffixText->SetColorAndOpacity(color);
	}

	void SetTextShadow() {
		PrefixText->SetShadowOffset(FVector2D(1, 1));
		PrefixText->SetShadowColorAndOpacity(FLinearColor(0, 0, 0, 0.5));
		SuffixText->SetShadowOffset(FVector2D(1, 1));
		SuffixText->SetShadowColorAndOpacity(FLinearColor(0, 0, 0, 0.5));
	}
	
	void SetImage(UTexture2D* texture)
	{
		IconImage->SetVisibility(ESlateVisibility::SelfHitTestInvisible);
		IconImage->SetBrushFromTexture(texture);
	}

	
	void SetImage(ResourceEnum resourceEnum, UAssetLoaderComponent* assetLoader, bool autoAddToolTip = false)
	{
		// TODO: LEAK possible source??
		//UMaterialInstanceDynamic* materialInstance = UMaterialInstanceDynamic::Create(assetLoader->ResourceIconMaterial, this);
		//materialInstance->SetTextureParameterValue("ColorTexture", assetLoader->GetResourceIcon(resourceEnum));
		//materialInstance->SetTextureParameterValue("DepthTexture", assetLoader->GetResourceIconAlpha(resourceEnum));
		//IconImage->SetBrushFromMaterial(materialInstance);

		IconImage->SetVisibility(ESlateVisibility::SelfHitTestInvisible);
		IconImage->SetBrushFromMaterial(assetLoader->GetResourceIconMaterial(resourceEnum));

		if (autoAddToolTip) {
			AddResourceTooltip(this, resourceEnum);
		}
	}

	void InitBackgroundButton(ResourceEnum resourceEnumIn) {
		uiResourceEnum = resourceEnumIn;
		BackgroundButton->SetVisibility(ESlateVisibility::Visible);
		if (BackgroundButton->OnClicked.GetAllObjects().Num() == 0) {
			BackgroundButton->OnClicked.AddDynamic(this, &UIconTextPairWidget::OnClickBackgroundButton);
		}
	}
	
	UFUNCTION() void OnClickBackgroundButton() {
		if (GetPunHUD()->IsResourcePriceUIOpened(uiResourceEnum)) {
			GetPunHUD()->CloseStatisticsUI();
		} else {
			GetPunHUD()->OpenResourcePriceUI(uiResourceEnum);
		}
	}

	void UpdateAllowCheckBox(ResourceEnum resourceEnumIn)
	{
		if (uiResourceEnum == ResourceEnum::None)
		{
			uiResourceEnum = resourceEnumIn;
			FrontCheckBox->SetVisibility(ESlateVisibility::Visible);
			if (FrontCheckBox->OnCheckStateChanged.GetAllObjects().Num() == 0) {
				FrontCheckBox->OnCheckStateChanged.AddDynamic(this, &UIconTextPairWidget::OnCheckAllowResource);
			}
			bool allowed = simulation().townManager(simulation().buildingTownId(ObjectId)).GetHouseResourceAllow(uiResourceEnum);
			FrontCheckBox->SetCheckedState(allowed ? ECheckBoxState::Checked : ECheckBoxState::Unchecked);
		}
	}
	UFUNCTION() void OnCheckAllowResource(bool active)
	{
		auto command = std::make_shared<FSetAllowResource>();
		command->buildingId = ObjectId;
		command->resourceEnum = uiResourceEnum;
		command->allowed = active;
		networkInterface()->SendNetworkCommand(command);
		//simulation().playerOwned(playerId()).SetHouseResourceAllow(uiResourceEnum, active);
	}

public:
	UPROPERTY(meta = (BindWidget)) UButton* BackgroundButton;
	
	ResourceEnum uiResourceEnum = ResourceEnum::None;
	
public:
	UPROPERTY(meta = (BindWidget)) UCheckBox* FrontCheckBox;
	
	UPROPERTY(meta = (BindWidget)) UImage* IconImage;
	UPROPERTY(meta = (BindWidget)) UTextBlock* PrefixText;
	UPROPERTY(meta = (BindWidget)) UTextBlock* SuffixText;

	int32 ObjectId = -1;
};
