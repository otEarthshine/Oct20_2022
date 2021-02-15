// Pun Dumnernchanvanit's

#pragma once

#include "CoreMinimal.h"
#include "PunCity/UI/PunWidget.h"
#include "Components/Overlay.h"
#include "PunBoxWidget.h"
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

	UPROPERTY(meta = (BindWidget)) UPunBoxWidget* BuyDisplayBox;
	UPROPERTY(meta = (BindWidget)) UPunBoxWidget* SellDisplayBox;

	UPROPERTY(meta = (BindWidget)) UButton* ConfirmButton;
	UPROPERTY(meta = (BindWidget)) UButton* DismissButton;

	int32 townId() {
		return simulation().building(punId).subclass<TownHall>(CardEnum::Townhall).townId();
	}

	void PunInit()
	{
		ConfirmButton->OnClicked.AddDynamic(this, &UIntercityTradeUI::ClickedConfirmButton);
		DismissButton->OnClicked.AddDynamic(this, &UIntercityTradeUI::ClickedDismissButton);

		SetVisibility(ESlateVisibility::Collapsed);

		SetChildHUD(BuyDisplayBox);
		SetChildHUD(SellDisplayBox);
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

			// Fill Buy/SellDisplayBox with proper amount of goods
			{
				IntercityTradeOfferEnum offerEnum = tradeRow->GetOfferEnum();
				int32 targetInventory = tradeRow->TargetInventory->amount;

				
				if (offerEnum == IntercityTradeOfferEnum::BuyWhenBelow) {
					int32 buyCount = targetInventory - resourceCount;
					if (buyCount > 0) {
						BuyDisplayBox->AddIconPair(TEXT_NUM(buyCount), tradeRow->resourceEnum(), FText());
					}
				}
				else if (offerEnum == IntercityTradeOfferEnum::SellWhenAbove) {
					int32 sellCount = resourceCount - targetInventory;
					if (sellCount > 0) {
						SellDisplayBox->AddIconPair(TEXT_NUM(sellCount), tradeRow->resourceEnum(), FText());
					}
				}

				// Hide TargetInventory
				tradeRow->TargetInventory->SetVisibility(offerEnum != IntercityTradeOfferEnum::None ? ESlateVisibility::Visible : ESlateVisibility::Hidden);
			}
			
		}

		BuyDisplayBox->AfterAdd();
		SellDisplayBox->AfterAdd();
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
			tradeRow->TargetInventory->minAmount = 0;
			TradeRowBox->AddChild(tradeRow);
		}

		{
			// Second loop, all other resources
			for (const ResourceInfo& info : SortedNameResourceInfo)
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
		// Take UI Info and input them into sim

		auto command = make_shared<FSetIntercityTrade>();
		command->townId = townId();
		if (command->townId != -1)
		{
			TArray<UWidget*> tradeRows = TradeRowBox->GetAllChildren();
			for (int32 i = 0; i < tradeRows.Num(); i++)
			{
				auto tradeRow = CastChecked<UIntercityTradeRow>(tradeRows[i]);

				IntercityTradeOfferEnum offerEnum = tradeRow->GetOfferEnum();
				int32 targetInventory = tradeRow->TargetInventory->amount;
				
				if ((offerEnum == IntercityTradeOfferEnum::BuyWhenBelow && targetInventory > 0) ||
					(offerEnum == IntercityTradeOfferEnum::SellWhenAbove && targetInventory >= 0))
				{
					command->resourceEnums.Add(static_cast<uint8>(tradeRow->resourceEnum()));
					command->intercityTradeOfferEnum.Add(static_cast<uint8>(offerEnum));
					command->targetInventories.Add(targetInventory);
				}
			}

			networkInterface()->SendNetworkCommand(command);

			dataSource()->Spawn2DSound("UI", "TradeAction");
			SetVisibility(ESlateVisibility::Collapsed);
		}
	}
	UFUNCTION() void ClickedDismissButton() {
		CloseUI();
	}
};
