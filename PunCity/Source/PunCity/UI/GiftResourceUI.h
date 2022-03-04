// Pun Dumnernchanvanit's

#pragma once

#include "BuildingPlacementButton.h"
#include "ChooseResourceElement.h"
#include "PunBoxWidget.h"
#include "PunEditableNumberBox.h"
#include "TradeDealResourceRow.h"

#include "GiftResourceUI.generated.h"

/**
 * 
 */
UCLASS()
class PROTOTYPECITY_API UGiftResourceUI : public UPunWidget
{
	GENERATED_BODY()
public:
	UPROPERTY(meta = (BindWidget)) UButton* MakeOfferButton;
	UPROPERTY(meta = (BindWidget)) UTextBlock* MakeOfferButtonText;

	UPROPERTY(meta = (BindWidget)) UButton* AcceptOfferButton;
	
	UPROPERTY(meta = (BindWidget)) UButton* CancelButton;
	UPROPERTY(meta = (BindWidget)) UWGT_ButtonCpp* CloseXButton;

	UPROPERTY(meta = (BindWidget)) UTextBlock* GiftTitleText;

	//! Trade
	// Left
	UPROPERTY(meta = (BindWidget)) UHorizontalBox* LeftPlayerNameTextOuterBox;
	UPROPERTY(meta = (BindWidget)) UTextBlock* LeftPlayerNameText;
	
	UPROPERTY(meta = (BindWidget)) UButton* LeftAddMoneyButton;
	UPROPERTY(meta = (BindWidget)) UButton* LeftAddResourceButton;
	UPROPERTY(meta = (BindWidget)) UButton* LeftAddCardButton;

	UPROPERTY(meta = (BindWidget)) UHorizontalBox* LeftMoneyTargetBox;
	UPROPERTY(meta = (BindWidget)) UPunEditableNumberBox* LeftMoneyTargetAmount;
	UPROPERTY(meta = (BindWidget)) UWGT_ButtonCpp* LeftMoneyCloseXButton;
	
	UPROPERTY(meta = (BindWidget)) UVerticalBox* LeftResourceValueBox;
	UPROPERTY(meta = (BindWidget)) URichTextBlock* LeftResourceValueText;
	UPROPERTY(meta = (BindWidget)) UVerticalBox* LeftTradeResourceRows;
	
	UPROPERTY(meta = (BindWidget)) UWrapBox* LeftCardBox;

	// Right
	UPROPERTY(meta = (BindWidget)) USizeBox* RightPlayerNameTextOuterSizeBox;
	UPROPERTY(meta = (BindWidget)) UTextBlock* RightPlayerNameText;
	
	UPROPERTY(meta = (BindWidget)) UHorizontalBox* RightDealBox;
	UPROPERTY(meta = (BindWidget)) UButton* RightAddMoneyButton;
	UPROPERTY(meta = (BindWidget)) UButton* RightAddResourceButton;
	UPROPERTY(meta = (BindWidget)) UButton* RightAddCardButton;

	UPROPERTY(meta = (BindWidget)) UHorizontalBox* RightMoneyTargetBox;
	UPROPERTY(meta = (BindWidget)) UPunEditableNumberBox* RightMoneyTargetAmount;
	UPROPERTY(meta = (BindWidget)) UWGT_ButtonCpp* RightMoneyCloseXButton;

	UPROPERTY(meta = (BindWidget)) UVerticalBox* RightResourceValueBox;
	UPROPERTY(meta = (BindWidget)) URichTextBlock* RightResourceValueText;
	UPROPERTY(meta = (BindWidget)) UVerticalBox* RightTradeResourceRows;

	UPROPERTY(meta = (BindWidget)) UWrapBox* RightCardBox;


	
	

	UPROPERTY(meta = (BindWidget)) UOverlay* ChooseResourceOverlay;
	UPROPERTY(meta = (BindWidget)) UPunBoxWidget* ChooseResourceBox;
	UPROPERTY(meta = (BindWidget)) UButton* ChooseResourceCloseButton;
	UPROPERTY(meta = (BindWidget)) UEditableTextBox* ChooseResourceSearchBox;
	UPROPERTY(meta = (BindWidget)) UWGT_ButtonCpp* ChooseResourceCloseXButton;
	bool isChoosingLeftResource = false;


	UPROPERTY(meta = (BindWidget)) UOverlay* CardChooseOverlay;
	UPROPERTY(meta = (BindWidget)) UTextBlock* CardChooseText;
	UPROPERTY(meta = (BindWidget)) UWrapBox* CardChooseBox;
	UPROPERTY(meta = (BindWidget)) UButton* CardChooseCloseButton;
	UPROPERTY(meta = (BindWidget)) UWGT_ButtonCpp* CardChooseCloseXButton;
	bool isChoosingLeftCards = false;
	
	int32 sourceTownId = -1; // Left
	int32 targetTownId = -1;
	TradeDealStageEnum dealStageEnum = TradeDealStageEnum::None;
	
	const FText MoneyText = NSLOCTEXT("UGiftResourceUI", "Money", "Money");
	
	void PunInit()
	{
		BUTTON_ON_CLICK(MakeOfferButton, this, &UGiftResourceUI::OnClickConfirmButton);
		BUTTON_ON_CLICK(AcceptOfferButton, this, &UGiftResourceUI::OnClickConfirmButton);
		BUTTON_ON_CLICK(CancelButton, this, &UGiftResourceUI::OnClickCloseButton);
		BUTTON_ON_CLICK(CloseXButton->CoreButton, this, &UGiftResourceUI::OnClickCloseButton);

		//GiftTypeDropdown->OnSelectionChanged.Clear();
		//GiftTypeDropdown->OnSelectionChanged.AddDynamic(this, &UGiftResourceUI::OnDropDownChanged);

		//SetChildHUD(GiftTargetAmount);
		//GiftTargetAmount->minAmount = 0;

		//! Left
		SetChildHUD(LeftMoneyTargetAmount);
		LeftMoneyTargetAmount->minAmount = 0;

		BUTTON_ON_CLICK(LeftAddMoneyButton, this, &UGiftResourceUI::OnClickLeftAddMoneyButton);
		BUTTON_ON_CLICK(LeftAddResourceButton, this, &UGiftResourceUI::OnClickLeftAddResourceButton);
		BUTTON_ON_CLICK(LeftAddCardButton, this, &UGiftResourceUI::OnClickLeftAddCardButton);

		BUTTON_ON_CLICK(LeftMoneyCloseXButton->CoreButton, this, &UGiftResourceUI::OnClickAddMoneyCloseXButton);

		//! Right
		SetChildHUD(RightMoneyTargetAmount);
		RightMoneyTargetAmount->minAmount = 0;

		BUTTON_ON_CLICK(RightAddMoneyButton, this, &UGiftResourceUI::OnClickRightAddMoneyButton);
		BUTTON_ON_CLICK(RightAddResourceButton, this, &UGiftResourceUI::OnClickRightAddResourceButton);
		BUTTON_ON_CLICK(RightAddCardButton, this, &UGiftResourceUI::OnClickRightAddCardButton);

		BUTTON_ON_CLICK(RightMoneyCloseXButton->CoreButton, this, &UGiftResourceUI::OnClickAddMoneyCloseXButton);
		

		SetChildHUD(ChooseResourceBox);
		BUTTON_ON_CLICK(ChooseResourceCloseButton, this, &UGiftResourceUI::CloseChooseResourceUI);
		BUTTON_ON_CLICK(ChooseResourceCloseXButton->CoreButton, this, &UGiftResourceUI::CloseChooseResourceUI);


		BUTTON_ON_CLICK(CardChooseCloseButton, this, &UGiftResourceUI::CloseCardChooseOverlay);
		BUTTON_ON_CLICK(CardChooseCloseXButton->CoreButton, this, &UGiftResourceUI::CloseCardChooseOverlay);
		

		SetVisibility(ESlateVisibility::Collapsed);
	}

	void OpenGiftUI(int32 sourcePlayerIdIn, int32 targetTownIdIn, TradeDealStageEnum dealStageEnumIn)
	{
		sourceTownId = sourcePlayerIdIn;
		targetTownId = targetTownIdIn; // use targetTownIdIn to handle Minor Town Id

		dealStageEnum = dealStageEnumIn;

		auto& sim = simulation();
		
		if (dealStageEnum == TradeDealStageEnum::Gifting) 
		{
			SetText(GiftTitleText, FText::Format(
				NSLOCTEXT("GiftResourceUI", "GiftToX", "Gift to {0}"),
				sim.townNameT(targetTownId)
			));
			RightDealBox->SetVisibility(ESlateVisibility::Collapsed);

			LeftPlayerNameTextOuterBox->SetVisibility(ESlateVisibility::Collapsed);
			RightPlayerNameTextOuterSizeBox->SetVisibility(ESlateVisibility::Collapsed);
		}
		else {
			SetText(GiftTitleText, NSLOCTEXT("TradeDealUI", "TradeDealTitle", "Trade Deal"));
			RightDealBox->SetVisibility(ESlateVisibility::SelfHitTestInvisible);

			LeftPlayerNameTextOuterBox->SetVisibility(ESlateVisibility::SelfHitTestInvisible);
			RightPlayerNameTextOuterSizeBox->SetVisibility(ESlateVisibility::SelfHitTestInvisible);

			LeftPlayerNameText->SetText(sim.playerNameT(sim.townPlayerId(sourceTownId)));
			RightPlayerNameText->SetText(sim.playerNameT(sim.townPlayerId(targetTownId)));
		}

		// Left and Right
		LeftMoneyTargetBox->SetVisibility(ESlateVisibility::Collapsed);
		LeftResourceValueBox->SetVisibility(ESlateVisibility::Collapsed);
		LeftCardBox->SetVisibility(ESlateVisibility::Collapsed);

		LeftTradeResourceRows->ClearChildren();
		LeftCardBox->ClearChildren();

		RightAddMoneyButton->SetVisibility(ESlateVisibility::Visible);
		RightMoneyTargetBox->SetVisibility(ESlateVisibility::Collapsed);
		RightResourceValueBox->SetVisibility(ESlateVisibility::Collapsed);
		RightCardBox->SetVisibility(ESlateVisibility::Collapsed);

		RightTradeResourceRows->ClearChildren();
		RightCardBox->ClearChildren();

		//! Minor Town shouldn't display Resource/Card
		LeftAddMoneyButton->SetVisibility(ESlateVisibility::Visible);
		LeftAddResourceButton->SetVisibility(IsMinorTown(targetTownId) ? ESlateVisibility::Collapsed : ESlateVisibility::Visible);
		LeftAddCardButton->SetVisibility(IsMinorTown(targetTownId) ? ESlateVisibility::Collapsed : ESlateVisibility::Visible);

		//! Shared
		ChooseResourceOverlay->SetVisibility(ESlateVisibility::Collapsed);
		
		CardChooseOverlay->SetVisibility(ESlateVisibility::Collapsed);
		CardChooseBox->ClearChildren();
		

		
		SetVisibility(ESlateVisibility::SelfHitTestInvisible);
	}

	void FillDealInfo(const TradeDealSideInfo& sourceDealInfo, const TradeDealSideInfo& targetDealInfo)
	{
		auto fillTradeDealSide = [&](
			bool isLeft,
			const TradeDealSideInfo& dealInfo,
			UHorizontalBox* moneyTargetBox,
			UPunEditableNumberBox* moneyTargetAmount
			)
		{
			if (dealInfo.moneyAmount > 0) {
				moneyTargetBox->SetVisibility(ESlateVisibility::SelfHitTestInvisible);
				moneyTargetAmount->amount = dealInfo.moneyAmount;
				moneyTargetAmount->UpdateText();
			}

			for (const ResourcePair& resourcePair : dealInfo.resourcePairs) {
				AddTradeDealResource(isLeft, resourcePair.resourceEnum, resourcePair.count);
			}

			for (const CardStatus& cardStatus : dealInfo.cardStatuses) {
				AddTradeDealCard(isLeft, cardStatus.cardEnum, 1);
			}
		};
		
		fillTradeDealSide(true, sourceDealInfo, LeftMoneyTargetBox, LeftMoneyTargetAmount);
		fillTradeDealSide(false, targetDealInfo, RightMoneyTargetBox, RightMoneyTargetAmount);
		
	}

	void CloseUI() {
		SetVisibility(ESlateVisibility::Collapsed);
	}

	void TickUI()
	{
		if (!IsVisible()) {
			return;
		}
		
		auto& sim = simulation();

		auto showButton = [&](bool isGreen, const FText& text)
		{
			MakeOfferButtonText->SetText(text);
			MakeOfferButton->SetVisibility(isGreen ? ESlateVisibility::Visible : ESlateVisibility::Collapsed);
			AcceptOfferButton->SetVisibility(isGreen ? ESlateVisibility::Collapsed : ESlateVisibility::Visible);
		};

		switch(dealStageEnum)
		{
		case TradeDealStageEnum::Gifting: showButton(true, NSLOCTEXT("UGiftResourceUI", "Confirm Gift", "Confirm Gift")); break;
		case TradeDealStageEnum::CreateDeal: showButton(true, NSLOCTEXT("UGiftResourceUI", "Make Offer", "Make Offer")); break;

		case TradeDealStageEnum::ExamineDeal:
		case TradeDealStageEnum::ExamineCounterOfferDeal: showButton(false, FText()); break;

		case TradeDealStageEnum::PrepareCounterOfferDeal: showButton(true, NSLOCTEXT("UGiftResourceUI", "Counter Offer", "Counter Offer")); break;

		default:
			UE_DEBUG_BREAK();
			break;
		}
		
		
		/*
		 * Fill choose resource box
		 */
		if (ChooseResourceOverlay->IsVisible())
		{
			FString searchString = ChooseResourceSearchBox->GetText().ToString();

			for (const ResourceInfo& info : SortedNameResourceInfo)
			{
				FString name = info.name.ToString();

				if (IsTradeResource(info.resourceEnum))
				{
					if (searchString.IsEmpty() ||
						name.Find(searchString, ESearchCase::Type::IgnoreCase, ESearchDir::FromStart) != INDEX_NONE)
					{
						int32 townId = isChoosingLeftResource ? sourceTownId : targetTownId;
						if (sim.resourceCountTown(townId, info.resourceEnum) > 0) 
						{
							// Remove Added Resource
							bool resourceAdded = false;
							UVerticalBox* resourceRows = isChoosingLeftResource ? LeftTradeResourceRows : RightTradeResourceRows;
							for (int32 i = 0; i < resourceRows->GetChildrenCount(); i++) {
								auto tradeRow = CastChecked<UTradeDealResourceRow>(resourceRows->GetChildAt(i));
								if (tradeRow->resourceEnum == info.resourceEnum) {
									resourceAdded = true;
									break;
								}
							}

							if (!resourceAdded) {
								ChooseResourceBox->AddChooseResourceElement(info.resourceEnum, this, CallbackEnum::AddTradeDealResource);
							}
						}
					}
				}
			}

			ChooseResourceBox->AfterAdd();
		}

		/*
		 * Trade Deals
		 */
		auto updateResourceValue = [&](UVerticalBox* resourceValueBox, URichTextBlock* resourceValueText, UVerticalBox* tradeResourceRows, int32 townId)
		{
			// Close resourceValueBox if there is no more tradeResourceRows
			if (tradeResourceRows->GetChildrenCount() == 0) {
				resourceValueBox->SetVisibility(ESlateVisibility::Collapsed);
			}
			
			if (resourceValueBox->IsVisible())
			{
				int32 totalValue100 = 0;
				for (int32 i = 0; i < tradeResourceRows->GetChildrenCount(); i++)
				{
					auto row = CastChecked<UTradeDealResourceRow>(tradeResourceRows->GetChildAt(i));
					totalValue100 += sim.price100(row->resourceEnum) * row->TargetAmount->amount;

					row->InventoryRichText->SetText(FText::Format(INVTEXT("/{0}"), sim.resourceCountPlayer(sim.townPlayerId(townId), row->resourceEnum)));
				}
				resourceValueText->SetText(FText::Format(INVTEXT("<img id=\"Coin\"/>{0}"), TEXT_100(totalValue100)));
			}
		};
		updateResourceValue(LeftResourceValueBox, LeftResourceValueText, LeftTradeResourceRows, sourceTownId);
		updateResourceValue(RightResourceValueBox, RightResourceValueText, RightTradeResourceRows, targetTownId);


		if (CardChooseOverlay->IsVisible()) 
		{
			if (isChoosingLeftCards) {
				CardChooseText->SetText(NSLOCTEXT("UGiftResourceUI", "Choose Your Cards", "Choose Your Cards"));
			} else {
				CardChooseText->SetText(FText::Format(NSLOCTEXT("UGiftResourceUI", "Choose Counterparty Cards", "Choose {0}'s Cards"), 
					targetTownId != -1 ? sim.playerNameT(sim.townPlayerId(targetTownId)) : FText()
				));
			}
		}		

		
	}

	// Add Column
	UFUNCTION() void OnClickLeftAddMoneyButton() {
		LeftMoneyTargetBox->SetVisibility(ESlateVisibility::SelfHitTestInvisible);
		LeftAddMoneyButton->SetVisibility(ESlateVisibility::Hidden);
		LeftMoneyTargetAmount->amount = 0;
		LeftMoneyTargetAmount->UpdateText();

		RightMoneyTargetAmount->amount = 0;
		RightMoneyTargetBox->SetVisibility(ESlateVisibility::Collapsed);
		RightAddMoneyButton->SetVisibility(ESlateVisibility::Visible);
	}
	UFUNCTION() void OnClickLeftAddResourceButton() {
		ChooseResourceOverlay->SetVisibility(ESlateVisibility::SelfHitTestInvisible);
		isChoosingLeftResource = true;
	}
	UFUNCTION() void OnClickLeftAddCardButton()
	{
		isChoosingLeftCards = true;
		OpenCardChooseUI();
	}

	UFUNCTION() void OnClickRightAddMoneyButton() {
		RightMoneyTargetBox->SetVisibility(ESlateVisibility::SelfHitTestInvisible);
		RightAddMoneyButton->SetVisibility(ESlateVisibility::Hidden);
		RightMoneyTargetAmount->amount = 0;
		RightMoneyTargetAmount->UpdateText();

		LeftMoneyTargetAmount->amount = 0;
		LeftMoneyTargetBox->SetVisibility(ESlateVisibility::Collapsed);
		LeftAddMoneyButton->SetVisibility(ESlateVisibility::Visible);
	}
	UFUNCTION() void OnClickRightAddResourceButton() {
		ChooseResourceOverlay->SetVisibility(ESlateVisibility::SelfHitTestInvisible);
		isChoosingLeftResource = false;
	}
	UFUNCTION() void OnClickRightAddCardButton()
	{
		isChoosingLeftCards = false;
		OpenCardChooseUI();
	}

	UFUNCTION() void OnClickAddMoneyCloseXButton()
	{
		LeftMoneyTargetAmount->amount = 0;
		LeftMoneyTargetBox->SetVisibility(ESlateVisibility::Collapsed);
		LeftAddMoneyButton->SetVisibility(ESlateVisibility::Visible);

		RightMoneyTargetAmount->amount = 0;
		RightMoneyTargetBox->SetVisibility(ESlateVisibility::Collapsed);
		RightAddMoneyButton->SetVisibility(ESlateVisibility::Visible);
	}
	

	void OpenCardChooseUI()
	{
		auto& sim = simulation();

		int32 cardOwnerId = isChoosingLeftCards ? sim.townPlayerId(sourceTownId) : sim.townPlayerId(targetTownId);
		auto& cardSys = sim.cardSystem(cardOwnerId);


		std::vector<CardStatus> cards = cardSys.GetCardsBoughtAndInventory();

		// Remove Added Card
		{
			UWrapBox* cardBox = isChoosingLeftCards ? LeftCardBox : RightCardBox;
			for (int32 i = 0; i < cardBox->GetChildrenCount(); i++)
			{
				auto cardButton = CastChecked<UBuildingPlacementButton>(cardBox->GetChildAt(i));
				BuildingCardSystem::RemoveCards(cardButton->cardStatus, cards);
			}
		}
		

		CardChooseBox->ClearChildren();

		for (CardStatus cardStatus : cards)
		{
			UBuildingPlacementButton* cardButton = AddWidget<UBuildingPlacementButton>(UIEnum::CardMini);
			cardButton->PunInit(
				cardStatus,
				0, this, CallbackEnum::AddTradeDealCard, CardHandEnum::TradeDealOffer
			);

			cardButton->RefreshBuildingIcon(dataSource()->assetLoader());
			cardButton->SetCardStatus(CardHandEnum::TradeDealSelect, false, false, false);

			CardChooseBox->AddChild(cardButton);
		}

		CardChooseOverlay->SetVisibility(ESlateVisibility::SelfHitTestInvisible);
	}

	// Choose Resource
	UFUNCTION() void CloseChooseResourceUI() {
		ChooseResourceOverlay->SetVisibility(ESlateVisibility::Collapsed);
	}

	// Choose Cards
	UFUNCTION() void CloseCardChooseOverlay() {
		CardChooseOverlay->SetVisibility(ESlateVisibility::Collapsed);
	}
	
	

	virtual void CallBack1(UPunWidget* punWidgetCaller, CallbackEnum callbackEnum) override
	{
		// Trigger Counter offer if the deal was changed
		if (dealStageEnum == TradeDealStageEnum::ExamineDeal ||
			dealStageEnum == TradeDealStageEnum::ExamineCounterOfferDeal)
		{
			dealStageEnum = TradeDealStageEnum::PrepareCounterOfferDeal;
		}
		
		if (callbackEnum == CallbackEnum::AddTradeDealResource)
		{
			auto chooseButton = CastChecked<UChooseResourceElement>(punWidgetCaller);

			AddTradeDealResource(isChoosingLeftResource, chooseButton->resourceEnum);

			CloseChooseResourceUI();
		}
		else if (callbackEnum == CallbackEnum::RemoveTradeDealResource)
		{
			auto callerTradeRow = CastChecked<UTradeDealResourceRow>(punWidgetCaller);
			ResourceEnum resourceEnum = callerTradeRow->resourceEnum;
			
			UVerticalBox* cardBox = isChoosingLeftResource ? LeftTradeResourceRows : LeftResourceValueBox;
			for (int32 i = cardBox->GetChildrenCount(); i-- > 0;) {
				auto tradeRow = CastChecked<UTradeDealResourceRow>(cardBox->GetChildAt(i));
				if (tradeRow->resourceEnum == resourceEnum) {
					cardBox->RemoveChildAt(i);
					break;
				}
			}
		}
		
		else if (callbackEnum == CallbackEnum::AddTradeDealCard)
		{
			auto selectedCardButton = CastChecked<UBuildingPlacementButton>(punWidgetCaller);
			
			AddTradeDealCard(isChoosingLeftCards, selectedCardButton->cardStatus.cardEnum, 1);

			CloseCardChooseOverlay();
		}
		else if (callbackEnum == CallbackEnum::RemoveTradeDealCard)
		{
			auto selectedCardButton = CastChecked<UBuildingPlacementButton>(punWidgetCaller);
			bool isLeftCard = selectedCardButton->cardHandIndex >= 0;
			CardEnum cardEnum = selectedCardButton->cardStatus.cardEnum;

			UWrapBox* cardBox = isLeftCard ? LeftCardBox : RightCardBox;
			for (int32 i = cardBox->GetChildrenCount(); i-- > 0;) {
				auto cardButton = CastChecked<UBuildingPlacementButton>(cardBox->GetChildAt(i));
				if (cardButton->cardStatus.cardEnum == cardEnum) {
					cardBox->RemoveChildAt(i);
					break;
				}
			}
		}
	}

	void AddTradeDealResource(bool isLeft, ResourceEnum resourceEnum, int32 resourceCount = 0)
	{
		auto tradeRow = AddWidget<UTradeDealResourceRow>(UIEnum::TradeDealResourceRow);
		tradeRow->ResourcePair->SetResource(resourceEnum, assetLoader());
		tradeRow->isLeft = isLeft;
		tradeRow->resourceEnum = resourceEnum;
		tradeRow->TargetAmount->Set(this, CallbackEnum::UpdateTradeDealResource);
		tradeRow->TargetAmount->amount = resourceCount;
		tradeRow->TargetAmount->UpdateText();
		tradeRow->callbackParent = this;

		UVerticalBox* resourceRows = isLeft ? LeftTradeResourceRows : RightTradeResourceRows;
		resourceRows->AddChild(tradeRow);

		UVerticalBox* resourceBox = isLeft ? LeftResourceValueBox : RightResourceValueBox;
		resourceBox->SetVisibility(ESlateVisibility::SelfHitTestInvisible);
	}

	void AddTradeDealCard(bool isLeft, CardEnum cardEnum, int32 stackSize = 1)
	{
		UBuildingPlacementButton* cardButton = AddWidget<UBuildingPlacementButton>(UIEnum::CardMini);

		// put isLeft into cardHandIndex
		cardButton->PunInit(
			CardStatus(cardEnum, stackSize),
			isLeft, this, CallbackEnum::RemoveTradeDealCard, CardHandEnum::TradeDealOffer
		);

		cardButton->RefreshBuildingIcon(dataSource()->assetLoader());
		cardButton->SetCardStatus(CardHandEnum::TradeDealOffer, false, false, false);

		UWrapBox* cardBox = isLeft ? LeftCardBox : RightCardBox;
		cardBox->AddChild(cardButton);

		cardBox->SetVisibility(ESlateVisibility::SelfHitTestInvisible);
	}

	
	

	//UFUNCTION() void OnDropDownChanged(FString sItem, ESelectInfo::Type seltype)
	//{
	//	FString resourceName = GiftTypeDropdown->GetSelectedOption();
	//	if (resourceName.IsEmpty()) {
	//		return;
	//	}
	//	
	//	if (resourceName == MoneyText.ToString()) {
	//		GiftIcon->SetBrushFromTexture(assetLoader()->CoinIcon);
	//	}
	//	else {
	//		ResourceEnum resourceEnum = FindResourceEnumByName(ToWString(resourceName));
	//		GiftIcon->SetBrushFromMaterial(assetLoader()->GetResourceIconMaterial(resourceEnum));
	//	}
	//}

	UFUNCTION() void OnClickConfirmButton();

	UFUNCTION() void OnClickCloseButton() {
		SetVisibility(ESlateVisibility::Collapsed);
	}
	
};
