// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "PunWidget.h"
#include "PunCity/Simulation/IGameSimulationCore.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "PunBoxWidget.h"
#include "Blueprint/WidgetLayoutLibrary.h"

enum class CardAnimationEnum
{
	None,
	ToBuildingSlot,
	ToGlobalSlot,
};

#include "BuildingPlacementButton.generated.h"

/**
 * 
 */
UCLASS()
class UBuildingPlacementButton : public UPunWidget
{
	GENERATED_BODY()
public:
	CardEnum buildingEnum;
	int32 cardHandIndex = -1;
	int32 buildingLvl = 0;
	
	FVector2D position;

	// Make card snap to slot (0,0)
	FVector2D cardAnimationOrigin;
	float cardAnimationStartTime = -1;
	float animationLength = 0.2f; // 0.2s default

	// Animate card in viewport
	bool isManuallyAnimating = false;
	CardAnimationEnum animationEnum = CardAnimationEnum::None;
	FVector2D cardAnimationTarget;

	float initTime = 0;
	bool needExclamation = false;

	bool IsPermanentCard() { return cardHandIndex == -1; }
	int32 IsUnboughtCard() { return cardHandIndex >= 0 && buildingLvl == -1; }

	void PunInit(CardEnum buildingEnumIn, int32 cardHandIndexIn, int32 buildingLvlIn, int32 stackSize, UPunWidget* callbackParent, CallbackEnum callbackEnum)
	{
		buildingEnum = buildingEnumIn;
		cardHandIndex = cardHandIndexIn;
		buildingLvl = buildingLvlIn;
		_callbackParent = callbackParent;
		_callbackEnum = callbackEnum;

		BldInfo info = GetBuildingInfo(buildingEnum);
		std::string description = info.miniDescription;
		if (IsGlobalSlotCard(buildingEnum)) {
			description += "\n<Gray>(global slot)</>";
		}
		if (IsBuildingSlotCard(buildingEnum)) {
			description += "\n<Gray>(building slot)</>";
		}
		
		DescriptionRichText->SetText(ToFText(description));
		BuildingNameText->SetText(ToFText(info.name));

		if (buildingEnum == CardEnum::House) {
			BuildingHotkeyText->SetVisibility(ESlateVisibility::SelfHitTestInvisible);
			SetText(BuildingHotkeyText, "[H]");
		} else if(buildingEnum == CardEnum::Farm) {
			BuildingHotkeyText->SetVisibility(ESlateVisibility::SelfHitTestInvisible);
			SetText(BuildingHotkeyText, "[F]");
		} else {
			BuildingHotkeyText->SetVisibility(ESlateVisibility::Collapsed);
		}

		CardGlow->SetVisibility(ESlateVisibility::Hidden);

		if (stackSize > 0) {
			cardCount = stackSize;
			
			Count->SetText(FText::FromString(FString::FromInt(stackSize)));

			//// 1/2, 2/4, 3/4, 4/8
			//if (stackSize >= CardCountForLvl[3]) {
			//	Count->SetText(FText::FromString(FString::FromInt(stackSize)));
			//}
			//else if (stackSize >= CardCountForLvl[2]) {
			//	Count->SetText(FText::FromString(FString::FromInt(stackSize) + "/8"));
			//}
			//else if (stackSize >= CardCountForLvl[1]) {
			//	Count->SetText(FText::FromString(FString::FromInt(stackSize) + "/4"));
			//}
			//else if (stackSize >= CardCountForLvl[0]) {
			//	Count->SetText(FText::FromString(FString::FromInt(stackSize) + "/2"));
			//}
			
			Count->SetVisibility(ESlateVisibility::HitTestInvisible);
		} else {
			Count->SetVisibility(ESlateVisibility::Collapsed);
		}

		// 
		if (IsUnboughtCard() || IsPermanentCard()) {
			SetStars(0);
			SellButton->SetVisibility(ESlateVisibility::Collapsed);
		} else {
			SetStars(buildingLvl);
			SellButton->SetVisibility(ESlateVisibility::Visible);
			SellButton->OnClicked.AddDynamic(this, &UBuildingPlacementButton::OnSellButtonClicked);
		}

		if (IsGlobalSlotCard(buildingEnum)) {
			CardSlotUnderneath->GetDynamicMaterial()->SetTextureParameterValue("CardSlot", assetLoader()->CardSlotRound);
			CardSlotUnderneath->GetDynamicMaterial()->SetScalarParameterValue("IsBuildingSlotCard", false);
			CardSlotUnderneath->SetVisibility(ESlateVisibility::HitTestInvisible);
		}
		else if (IsBuildingSlotCard(buildingEnum)) {
			CardSlotUnderneath->GetDynamicMaterial()->SetTextureParameterValue("CardSlot", assetLoader()->CardSlotBevel);
			CardSlotUnderneath->GetDynamicMaterial()->SetScalarParameterValue("IsBuildingSlotCard", true);
			CardSlotUnderneath->SetVisibility(ESlateVisibility::HitTestInvisible);
		}
		else {
			CardSlotUnderneath->SetVisibility(ESlateVisibility::Collapsed);
		}

		// Tooltip
		UPunBoxWidget::AddBuildingTooltip(this, buildingEnum, this, IsPermanentCard());

		// Close pricing/combi
		PriceTextBox->SetVisibility(ESlateVisibility::Collapsed);

		NeedResourcesText->SetVisibility(ESlateVisibility::Collapsed);
		
		BuyText->SetVisibility(ESlateVisibility::Collapsed);

		GetAnimations();
		//ExclamationIcon->SetVisibility(ESlateVisibility::Collapsed);

		initTime = 0;
	}

	FReply NativeOnMouseButtonDown(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent) override;
	FReply NativeOnMouseButtonDoubleClick(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent) override;
	void NativeOnMouseEnter(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent) override {
		UPunWidget::NativeOnMouseEnter(InGeometry, InMouseEvent);
		CardGlow->SetVisibility(ESlateVisibility::SelfHitTestInvisible);
		dataSource()->Spawn2DSound("UI", "CardHover");
	}
	void NativeOnMouseLeave(const FPointerEvent& InMouseEvent) override {
		UPunWidget::NativeOnMouseLeave(InMouseEvent);
		CardGlow->SetVisibility(ESlateVisibility::Hidden);
	}

	void SetupMaterials(UAssetLoaderComponent* assetLoader)
	{
		if (IsBuildingCard(buildingEnum)) {
			colorMaterial = UMaterialInstanceDynamic::Create(assetLoader->BuildingIconMaterial, this);
			colorMaterial->SetTextureParameterValue("ColorTexture", assetLoader->GetBuildingIcon(buildingEnum));
			colorMaterial->SetTextureParameterValue("DepthTexture", assetLoader->GetBuildingIconAlpha(buildingEnum));

			if (assetLoader->IsBuildingUsingSpecialIcon(buildingEnum)) {
				colorMaterial->SetScalarParameterValue("IsSpecial", 1.0f);
			} else {
				colorMaterial->SetScalarParameterValue("IsSpecial", 0.0f);
			}
			
			//switch (buildingEnum) {
			//case CardEnum::DirtRoad:
			//case CardEnum::StoneRoad:
			//case CardEnum::Fence:
			//case CardEnum::FenceGate:
			//case CardEnum::TrapSpike:
			//case CardEnum::Bridge:
			//	colorMaterial->SetScalarParameterValue("IsSpecial", 1.0f);
			//	break;
			//default:
			//	colorMaterial->SetScalarParameterValue("IsSpecial", 0.0f);
			//	break;
			//}

			//grayMaterial = UMaterialInstanceDynamic::Create(assetLoader->BuildingIconGrayMaterial, this);
			//grayMaterial->SetTextureParameterValue("ColorTexture", assetLoader->GetBuildingIcon(buildingEnum));
			//grayMaterial->SetTextureParameterValue("DepthTexture", assetLoader->GetBuildingIconAlpha(buildingEnum));
		} else {
			colorMaterial = UMaterialInstanceDynamic::Create(assetLoader->CardIconMaterial, this);
			colorMaterial->SetTextureParameterValue("ColorTexture", assetLoader->GetCardIcon(buildingEnum));
			
			//grayMaterial = UMaterialInstanceDynamic::Create(assetLoader->CardIconGrayMaterial, this);
		}

		BuildingIcon->SetBrushFromMaterial(colorMaterial);
	}

	void SetCardStatus(CardHandEnum cardHandEnum, bool isReservedForBuying, bool needResource, bool isRareCardHand = false, bool tryShowBuyText = true)
	{
		if (tryShowBuyText) {
			BuyText->SetVisibility(isReservedForBuying && !isRareCardHand ? ESlateVisibility::SelfHitTestInvisible : ESlateVisibility::Collapsed);
		} else {
			BuyText->SetVisibility(ESlateVisibility::Collapsed);
		}

		auto material = CardBackgroundImage->GetDynamicMaterial();
		//material->SetScalarParameterValue("IsRare", isRareCardHand ? 1.0f : 0.0f);
		material->SetScalarParameterValue("Highlight", isReservedForBuying ? 1.0f : 0.0f);

		bool isGlobalSlotCard = IsGlobalSlotCard(buildingEnum);
		bool isBuildingSlotCard = IsBuildingSlotCard(buildingEnum);
		
		material->SetScalarParameterValue("IsBuildingCard", IsBuildingCard(buildingEnum) ? 1.0f : 0.0f);
		material->SetScalarParameterValue("IsActionCard", IsActionCard(buildingEnum) ? 1.0f : 0.0f);
		material->SetScalarParameterValue("IsGlobalSlotCard", isGlobalSlotCard ? 1.0f : 0.0f);
		material->SetScalarParameterValue("IsBuildingSlotCard", isBuildingSlotCard ? 1.0f : 0.0f);

		// Don't show 
		bool showNeedResourceUI = !isReservedForBuying && needResource;


		//BuildingIcon->SetBrushFromMaterial(showNeedResourceUI ? grayMaterial : colorMaterial);
		BuildingIcon->GetDynamicMaterial()->SetScalarParameterValue("IsGray", showNeedResourceUI ? 1.0f : 0.0f);
		
		NeedResourcesText->SetColorAndOpacity(FLinearColor(.2, 0, 0));
		NeedResourcesText->SetVisibility(showNeedResourceUI ? ESlateVisibility::HitTestInvisible : ESlateVisibility::Collapsed);
		NeedResourcesText->SetText(FText::FromString("Need money"));

		//
		if (isGlobalSlotCard) {
			material->SetTextureParameterValue("CardTexture", assetLoader()->CardFrontRound);
		} else if (isBuildingSlotCard) {
			material->SetTextureParameterValue("CardTexture", assetLoader()->CardFrontBevel);
		} else {
			material->SetTextureParameterValue("CardTexture", assetLoader()->CardFront);
		}

		// Notify of the first time seeing permanent card..
		needExclamation = false;
		if (IsPermanentCard())
		{
			if (buildingEnum == CardEnum::Farm && !simulation().parameters(playerId())->FarmNoticed) {
				needExclamation = true;
			}
			else if (buildingEnum == CardEnum::Bridge && !simulation().parameters(playerId())->BridgeNoticed) {
				needExclamation = true;
			}
			else if (buildingEnum == CardEnum::House && simulation().NeedQuestExclamation(playerId(), QuestEnum::BuildHousesQuest)) {
				needExclamation = true;
			}
			else if (buildingEnum == CardEnum::StorageYard && simulation().HasQuest(playerId(), QuestEnum::BuildStorageQuest) && 
				simulation().buildingCount(playerId(), CardEnum::StorageYard) <= 2) {
				needExclamation = true;
			}
		}
		else
		{
			if (buildingEnum == CardEnum::Townhall) {
				needExclamation = true;
			}
			else if (simulation().HasQuest(playerId(), QuestEnum::FoodBuildingQuest) && 
					(IsAgricultureBuilding(buildingEnum) && buildingEnum != CardEnum::Forester)) 
			{
				needExclamation = true;
			}

			// First Seed Card
			if (cardHandEnum == CardHandEnum::BoughtHand &&
				IsSeedCard(buildingEnum) && 
				!simulation().unlockSystem(playerId())->isUnlocked(CardEnum::Farm)) 
			{
				needExclamation = true;
			}
		}

		//PlayAnimationIf("Flash", needExclamation);
		//ExclamationIcon->SetShow(needExclamation);
	}
	
	void SetBoughtCardNeedResource(bool needResource) {
		// Bought card need resource is just a warning, and is not grayed out.
		//NeedResourcesText->SetColorAndOpacity(FLinearColor(.5, .1, 0));
		//NeedResourcesText->SetVisibility(needResource ? ESlateVisibility::HitTestInvisible : ESlateVisibility::Collapsed);
		//NeedResourcesText->SetText(FText::FromString("Need resources"));

		NeedResourcesText->SetVisibility(ESlateVisibility::Collapsed);
	}

	void SetStars(int32 count) {
		Star1->SetVisibility(count >= 1 ? ESlateVisibility::HitTestInvisible : ESlateVisibility::Collapsed);
		Star2->SetVisibility(count >= 2 ? ESlateVisibility::HitTestInvisible : ESlateVisibility::Collapsed);
		Star3->SetVisibility(count >= 3 ? ESlateVisibility::HitTestInvisible : ESlateVisibility::Collapsed);
		//PUN_LOG("SetStars1 %d", Star1->GetVisibility() == ESlateVisibility::HitTestInvisible);
	}

	/*
	 * Animation
	 */

	void FlyTowardsWidgetAndDespawn(UWidget* targetWidget)
	{
		//targetWidget->GetCa
	}

	void StartAnimation(FVector2D origin, FVector2D target, CardAnimationEnum animationEnumIn, float animationLengthIn = 0.2f)
	{
		isManuallyAnimating = true;
		cardAnimationOrigin = origin;
		cardAnimationTarget = target;
		cardAnimationStartTime = UGameplayStatics::GetTimeSeconds(this);
		animationLength = animationLengthIn;
		animationEnum = animationEnumIn;
	}
	
	void NativeTick(const FGeometry& MyGeometry, float InDeltaTime) override
	{
		float time = UGameplayStatics::GetTimeSeconds(this);
		
		if (isManuallyAnimating)
		{
			const FGeometry& viewportGeometry = GetViewportGeometry();
			
			FVector2D targetAbsolutePos = viewportGeometry.LocalToAbsolute(cardAnimationTarget);
			FVector2D targetLocalPos = MyGeometry.AbsoluteToLocal(targetAbsolutePos);
			
			if (time - cardAnimationStartTime < animationLength)
			{	
				FVector2D lastAbsolutePos = viewportGeometry.LocalToAbsolute(cardAnimationOrigin);
				FVector2D lastLocalPos = MyGeometry.AbsoluteToLocal(lastAbsolutePos);

				float animationProgress = static_cast<float>(time - cardAnimationStartTime) / animationLength;
				ParentOverlay->SetRenderTranslation(animationProgress * (targetLocalPos - lastLocalPos) + lastLocalPos);
			}
			else {
				ParentOverlay->SetRenderTranslation(targetLocalPos);
			}
			
			return;
		}

		/*
		 * Animate to 0 local pos
		 * !!! Main?
		 */
		if (time - cardAnimationStartTime < animationLength)
		{
			const FGeometry& viewportGeometry = GetViewportGeometry();
			FVector2D absolutePos = viewportGeometry.LocalToAbsolute(cardAnimationOrigin);
			FVector2D localPos = MyGeometry.AbsoluteToLocal(absolutePos);
			
			float animationProgress = static_cast<float>(time - cardAnimationStartTime) / animationLength;
			ParentOverlay->SetRenderTranslation((1.0f - animationProgress) * localPos);
		}
		else {
			ParentOverlay->SetRenderTranslation(FVector2D::ZeroVector);
		}

		if (needExclamation) {
			float timeSinceInit = UGameplayStatics::GetTimeSeconds(this) - initTime;
			float fraction = 1.0f - 2.0f * abs(fmod(timeSinceInit, 1.0f) - 0.5f);
			CardBackgroundImage->GetDynamicMaterial()->SetScalarParameterValue("FlashFraction", fraction);
		}
		else {
			CardBackgroundImage->GetDynamicMaterial()->SetScalarParameterValue("FlashFraction", 0);
		}
	}

	bool isAnimating() {
		float time = UGameplayStatics::GetTimeSeconds(this);
		return isManuallyAnimating || time - cardAnimationStartTime < animationLength;
	}


	void SetPrice(int32 price)
	{
		SetText(PriceText, to_string(price));
		PriceTextBox->SetVisibility(price > 0 ? ESlateVisibility::HitTestInvisible : ESlateVisibility::Collapsed);
	}

	UPROPERTY(meta = (BindWidget)) UOverlay* ParentOverlay;

	UPROPERTY(meta = (BindWidget)) UTextBlock* BuildingNameText;
	UPROPERTY(meta = (BindWidget)) UTextBlock* BuildingHotkeyText;
	UPROPERTY(meta = (BindWidget)) URichTextBlock* DescriptionRichText;
	UPROPERTY(meta = (BindWidget)) UImage* BuildingIcon;

	UPROPERTY(meta = (BindWidget)) UTextBlock* PriceText;
	UPROPERTY(meta = (BindWidget)) UHorizontalBox* PriceTextBox;

	UPROPERTY(meta = (BindWidget)) USizeBox* Star1;
	UPROPERTY(meta = (BindWidget)) USizeBox* Star2;
	UPROPERTY(meta = (BindWidget)) USizeBox* Star3;
	UPROPERTY(meta = (BindWidget)) UTextBlock* Count;
	int32 cardCount = 0;

	UPROPERTY(meta = (BindWidget)) UImage* CardBackgroundImage;
	UPROPERTY(meta = (BindWidget)) UImage* CardGlow;

	UPROPERTY(meta = (BindWidget)) UTextBlock* NeedResourcesText;
	UPROPERTY(meta = (BindWidget)) UTextBlock* BuyText;
	
	UPROPERTY(meta = (BindWidget)) UButton* SellButton;

	UPROPERTY(meta = (BindWidget)) UImage* CardSlotUnderneath;

	//UPROPERTY(meta = (BindWidget)) UExclamationIcon* ExclamationIcon;

private:
	UPROPERTY() UPunWidget* _callbackParent;
	CallbackEnum _callbackEnum;

	UPROPERTY() UMaterialInstanceDynamic* colorMaterial;
	//UPROPERTY() UMaterialInstanceDynamic* grayMaterial;

	UFUNCTION() void OnSellButtonClicked();
};
