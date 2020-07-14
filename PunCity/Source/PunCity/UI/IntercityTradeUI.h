// Pun Dumnernchanvanit's

#pragma once

#include "CoreMinimal.h"
#include "PunCity/UI/PunWidget.h"
#include "Components/Overlay.h"
#include "IntercityTradeRow.h"
#include "IntercityTradeUI.generated.h"

/**
 * 
 */
UCLASS()
class PROTOTYPECITY_API UIntercityTradeUI : public UPunWidget
{
	GENERATED_BODY()
public:
	UPROPERTY(meta = (BindWidget)) UOverlay* TradeOverlay;

	UPROPERTY(meta = (BindWidget)) UEditableTextBox* SearchBox;

	UPROPERTY(meta = (BindWidget)) UScrollBox* TradeRowBox;

	UPROPERTY(meta = (BindWidget)) UTextBlock* BuyDisplayText;
	UPROPERTY(meta = (BindWidget)) UTextBlock* SellDisplayText;
	UPROPERTY(meta = (BindWidget)) UPunBoxWidget* BuyDisplayBox;
	UPROPERTY(meta = (BindWidget)) UPunBoxWidget* SellDisplayBox;

	UPROPERTY(meta = (BindWidget)) UButton* ConfirmButton;
	UPROPERTY(meta = (BindWidget)) UButton* DismissButton;

	void PunInit()
	{
		ConfirmButton->OnClicked.AddDynamic(this, &UIntercityTradeUI::ClickedConfirmButton);
		DismissButton->OnClicked.AddDynamic(this, &UIntercityTradeUI::ClickedDismissButton);

		SetVisibility(ESlateVisibility::Collapsed);
	}

	void TickUI()
	{
		if (GetVisibility() != ESlateVisibility::SelfHitTestInvisible) {
			return;
		}

		ResourceSystem& resourceSystem = dataSource()->simulation().resourceSystem(playerId());

		FString searchString = SearchBox->GetText().ToString();

		// Recalculate trade amount as resourceCount changes
		TArray<UWidget*> tradeRows = TradeRowBox->GetAllChildren();

		for (int32 i = 0; i < tradeRows.Num(); i++)
		{
			auto tradeRow = CastChecked<UIntercityTradeRow>(tradeRows[i]);
			int32 resourceCount = resourceSystem.resourceCount(tradeRow->resourceEnum());

			// update text if inventory changed
			if (resourceCount != tradeRow->inventory) {
				tradeRow->inventory = resourceCount;
				tradeRow->UpdateTexts();
			}

			// Hide if it is not searched...
			FString resourceName = ResourceNameF(tradeRow->resourceEnum());

			if (searchString.IsEmpty() ||
				resourceName.Find(searchString, ESearchCase::Type::IgnoreCase, ESearchDir::FromStart) != INDEX_NONE)
			{
				tradeRow->SetVisibility(ESlateVisibility::Visible);
			}
			else {
				tradeRow->SetVisibility(ESlateVisibility::Collapsed);
			}
		}
	
	}

	void OpenUI(int32 objectId)
	{
		TradeRowBox->ClearChildren();

		ResourceSystem& resourceSystem = simulation().resourceSystem(playerId());

		punId = objectId;

		// First loop, only the resources we have for selling, sorted by amount...
		std::vector<ResourcePair> availableForSellingPairs;
		for (ResourceInfo info : ResourceInfos) {
			int32 count = resourceSystem.resourceCount(info.resourceEnum);
			if (IsTradeResource(info.resourceEnum) && count > 0)
			{
				availableForSellingPairs.push_back(ResourcePair(info.resourceEnum, count));
			}
		}

		std::sort(availableForSellingPairs.begin(), availableForSellingPairs.end(), [&](ResourcePair a, ResourcePair b) {
			return a.count > b.count;
		});

		for (ResourcePair pair : availableForSellingPairs) {
			auto tradeRow = AddWidget<UIntercityTradeRow>(UIEnum::TradeRowIntercity);
			tradeRow->Init(this, pair.resourceEnum);
			TradeRowBox->AddChild(tradeRow);
		}


		{
			// Second loop, all other resources
			for (const ResourceInfo& info : SortedNameResourceEnum)
			{
				if (IsTradeResource(info.resourceEnum) &&
					resourceSystem.resourceCount(info.resourceEnum) == 0)
				{
					auto tradeRow = AddWidget<UIntercityTradeRow>(UIEnum::TradeRowIntercity);
					tradeRow->Init(this, info.resourceEnum);
					TradeRowBox->AddChild(tradeRow);
				}
			}
		}

		SetVisibility(ESlateVisibility::SelfHitTestInvisible);
		TickUI();
		//UpdateTotal();

		dataSource()->Spawn2DSound("UI", "UIWindowOpen");
	}

	void CloseUI() {
		if (GetVisibility() != ESlateVisibility::Collapsed) {
			dataSource()->Spawn2DSound("UI", "UIWindowClose");
		}
		SetVisibility(ESlateVisibility::Collapsed);
	}


	UFUNCTION() void ClickedConfirmButton()
	{
		
	}
	UFUNCTION() void ClickedDismissButton() {
		CloseUI();
	}
};
