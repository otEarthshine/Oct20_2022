// Pun Dumnernchanvanit's

#pragma once

#include "CoreMinimal.h"
#include "PunCity/UI/PunWidget.h"

#include "ChooseResourceElement.generated.h"

/**
 * 
 */
UCLASS()
class PROTOTYPECITY_API UChooseResourceElement : public UPunWidget
{
	GENERATED_BODY()
public:

	UPROPERTY(meta = (BindWidget)) UButton* ResourceButton;
	UPROPERTY(meta = (BindWidget)) UImage* ResourceIcon;
	UPROPERTY(meta = (BindWidget)) UTextBlock* ResourceText;
	
	UPROPERTY(meta = (BindWidget)) UTextBlock* MoneyText;
	UPROPERTY(meta = (BindWidget)) UImage* MoneyIcon;

	void OnInit() override {
		ResourceButton->OnClicked.Clear();
		ResourceButton->OnClicked.AddDynamic(this, &UChooseResourceElement::OnClickResourceButton);
	}

	void PunInit(UPunWidget* callbackParent, CallbackEnum callbackEnum, ResourceEnum resourceEnumIn)
	{
		_callbackParent = callbackParent;
		_callbackEnum = callbackEnum;
		resourceEnum = resourceEnumIn;

		if (resourceEnum == ResourceEnum::None) {
			ResourceText->SetText(FText::FromString(FString("Choose resource")));
			ResourceIcon->GetDynamicMaterial()->SetTextureParameterValue("ColorTexture", assetLoader()->BlackIcon);
			ResourceIcon->GetDynamicMaterial()->SetTextureParameterValue("DepthTexture", assetLoader()->BlackIcon);
			
			MoneyText->SetVisibility(ESlateVisibility::Collapsed);
			MoneyIcon->SetVisibility(ESlateVisibility::Collapsed);

			AddToolTip(ResourceButton, "Click to choose a resource.");
		}
		else {
			ResourceText->SetText(ToFText(ResourceName(resourceEnum)));
			ResourceIcon->GetDynamicMaterial()->SetTextureParameterValue("ColorTexture", assetLoader()->GetResourceIcon(resourceEnum));
			ResourceIcon->GetDynamicMaterial()->SetTextureParameterValue("DepthTexture", assetLoader()->GetResourceIconAlpha(resourceEnum));

			int32 price100 = simulation().price100(resourceEnum);
			int32 basePrice100 = GetResourceInfo(resourceEnum).basePrice100();

			std::stringstream ss;
			ss << fixed << setprecision(2);
			ss << price100 / 100.0f;
			SetText(MoneyText, ss);
			//MoneyText->SetText(FText::FromString(FString::SanitizeFloat(price100 / 100.0f)));
			SetPriceColor(MoneyText, price100, basePrice100);

			//ss << "Current price: " << (price100 / 100.0f) << "\n";
			//ss << "Base price: " << (basePrice100 / 100.0f);
			//AddToolTip(ResourceButton, ss.str());
			AddResourceTooltip(ResourceButton, resourceEnum);
			
			MoneyText->SetVisibility(ESlateVisibility::HitTestInvisible);
			MoneyIcon->SetVisibility(ESlateVisibility::HitTestInvisible);
		}
	}

	// For Intercity Trade
	void PunInit2(ResourceEnum resourceEnumIn, std::string resourceStr, UPunWidget* callbackParent, CallbackEnum callbackEnum = CallbackEnum::None)
	{
		_callbackParent = callbackParent;
		_callbackEnum = callbackEnum;
		resourceEnum = resourceEnumIn;

		PUN_CHECK(resourceEnum != ResourceEnum::None);

		ResourceButton->SetVisibility(callbackEnum != CallbackEnum::None ? ESlateVisibility::Visible : ESlateVisibility::Collapsed);
		
		ResourceText->SetText(ToFText(resourceStr));
		ResourceIcon->GetDynamicMaterial()->SetTextureParameterValue("ColorTexture", assetLoader()->GetResourceIcon(resourceEnum));
		ResourceIcon->GetDynamicMaterial()->SetTextureParameterValue("DepthTexture", assetLoader()->GetResourceIconAlpha(resourceEnum));

		int32 price100 = simulation().price100(resourceEnum);
		int32 basePrice100 = GetResourceInfo(resourceEnum).basePrice100();

		std::stringstream ss;
		ss << fixed << setprecision(2);
		ss << price100 / 100.0f;
		SetText(MoneyText, ss);
		SetPriceColor(MoneyText, price100, basePrice100);

		AddResourceTooltip(ResourceButton, resourceEnum);

		MoneyText->SetVisibility(ESlateVisibility::HitTestInvisible);
		MoneyIcon->SetVisibility(ESlateVisibility::HitTestInvisible);
	}

	UFUNCTION() void OnClickResourceButton()
	{
		PUN_CHECK(_callbackParent);
		_callbackParent->CallBack1(this, _callbackEnum);

		dataSource()->Spawn2DSound("UI", "ButtonClick");
	}

	//UFUNCTION() void SearchCommitted(const FText& Text, ETextCommit::Type CommitMethod)
	//{


	//	
	//	//if (CommitMethod == ETextCommit::OnEnter)
	//	//{

	//	//}
	//}

	ResourceEnum resourceEnum = ResourceEnum::None;
	
private:
	UPROPERTY() UPunWidget* _callbackParent = nullptr;
	CallbackEnum _callbackEnum = CallbackEnum::None;
};
