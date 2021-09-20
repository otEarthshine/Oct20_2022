// Pun Dumnernchanvanit's


#include "GiftResourceUI.h"

#define LOCTEXT_NAMESPACE "GiftResourceUI"

void UGiftResourceUI::OnClickConfirmButton()
{
	if (targetTownId != -1)
	{
		TradeDealSideInfo sourceDealInfo;
		TradeDealSideInfo targetDealInfo;
		
		sourceDealInfo.playerId = sourceTownId;
		targetDealInfo.playerId = targetTownId;
		sourceDealInfo.moneyAmount = LeftMoneyTargetBox->IsVisible() ? LeftMoneyTargetAmount->amount : 0;
		targetDealInfo.moneyAmount = RightMoneyTargetBox->IsVisible() ? RightMoneyTargetAmount->amount : 0;

		auto fillResourceValues = [&](UVerticalBox* tradeResourceRows, std::vector<ResourcePair>& resourcePairs)
		{
			for (int32 i = 0; i < tradeResourceRows->GetChildrenCount(); i++)
			{
				auto row = CastChecked<UTradeDealResourceRow>(tradeResourceRows->GetChildAt(i));
				resourcePairs.push_back(ResourcePair(row->resourceEnum, row->TargetAmount->amount));
			}
		};
		fillResourceValues(LeftTradeResourceRows, sourceDealInfo.resourcePairs);
		fillResourceValues(RightTradeResourceRows, targetDealInfo.resourcePairs);


		auto fillCards = [&](UWrapBox* cardBox, std::vector<CardStatus>& cardStatuses)
		{
			for (int32 i = 0; i < cardBox->GetChildrenCount(); i++)
			{
				auto cardButton = CastChecked<UBuildingPlacementButton>(cardBox->GetChildAt(i));
				cardStatuses.push_back(cardButton->cardStatus);
			}
		};
		fillCards(LeftCardBox, sourceDealInfo.cardStatuses);
		fillCards(RightCardBox, targetDealInfo.cardStatuses);

		
		TradeDealStageEnum nextStage = TradeDealStageEnum::None;
		switch (dealStageEnum)
		{
		case TradeDealStageEnum::Gifting: nextStage = TradeDealStageEnum::Gifting; break;
		case TradeDealStageEnum::CreateDeal: nextStage = TradeDealStageEnum::ExamineDeal; break;

		case TradeDealStageEnum::ExamineDeal:
		case TradeDealStageEnum::ExamineCounterOfferDeal: nextStage = TradeDealStageEnum::AcceptDeal; break;

		case TradeDealStageEnum::PrepareCounterOfferDeal: nextStage = TradeDealStageEnum::ExamineCounterOfferDeal; break;

		default:
			UE_DEBUG_BREAK();
			break;
		}
		

		std::shared_ptr<FGenericCommand> command = simulation().PackTradeDealInfoToCommand(sourceDealInfo, targetDealInfo, nextStage);
		
		networkInterface()->SendNetworkCommand(command);
	}

	SetVisibility(ESlateVisibility::Collapsed);
}

#undef LOCTEXT_NAMESPACE