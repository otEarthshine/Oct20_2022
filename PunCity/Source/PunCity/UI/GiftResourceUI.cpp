// Pun Dumnernchanvanit's


#include "GiftResourceUI.h"

#define LOCTEXT_NAMESPACE "GiftResourceUI"

void UGiftResourceUI::OnClickConfirmButton()
{
	if (targetTownId != -1)
	{
		//FString resourceName = GiftTypeDropdown->GetSelectedOption();
		//ResourceEnum resourceEnum;
		//int32 amount = GiftTargetAmount->amount;

		//if (resourceName == MoneyText.ToString()) {
		//	resourceEnum = ResourceEnum::Money;
		//	if (amount > simulation().moneyCap32(playerId())) {
		//		simulation().AddPopupToFront(playerId(),
		//			LOCTEXT("Not enough money to give.", "Not enough money to give."),
		//			ExclusiveUIEnum::GiftResourceUI, "PopupCannot"
		//		);
		//		return;
		//	}
		//}
		//else {
		//	// TODO: right now resources are given from the capital, later make it from any town
		//	resourceEnum = FindResourceEnumByName(ToWString(resourceName));
		//	if (amount > simulation().resourceCountTown(playerId(), resourceEnum)) {
		//		simulation().AddPopupToFront(playerId(), 
		//			LOCTEXT("Not enough resource to give.", "Not enough resource to give."),
		//			ExclusiveUIEnum::GiftResourceUI, "PopupCannot"
		//		);
		//		return;
		//	}
		
		auto command = make_shared<FGenericCommand>();
		command->genericCommandType = FGenericCommand::Type::SendGift;
		command->intVar1 = sourceTownId;
		command->intVar2 = targetTownId;
		command->intVar3 = LeftMoneyTargetAmount->amount;
		command->intVar4 = RightMoneyTargetAmount->amount;

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
		command->intVar5 = static_cast<int32>(nextStage);
		

		auto fillResourceValues = [&](UVerticalBox* tradeResourceRows, TArray<int32>& arrayResourceEnum, TArray<int32>& arrayResourceCount)
		{
			for (int32 i = 0; i < tradeResourceRows->GetChildrenCount(); i++)
			{
				auto row = CastChecked<UTradeDealResourceRow>(tradeResourceRows->GetChildAt(i));
				arrayResourceEnum.Add(static_cast<int32>(row->resourceEnum));
				arrayResourceCount.Add(row->TargetAmount->amount);
			}
		};
		fillResourceValues(LeftTradeResourceRows, command->array1, command->array2);
		fillResourceValues(RightTradeResourceRows, command->array3, command->array4);


		auto fillCards = [&](UWrapBox* cardBox, TArray<int32>& arrayCardEnum, TArray<int32>& arrayCardCount)
		{
			for (int32 i = 0; i < cardBox->GetChildrenCount(); i++)
			{
				auto cardButton = CastChecked<UBuildingPlacementButton>(cardBox->GetChildAt(i));

				arrayCardEnum.Add(static_cast<int32>(cardButton->cardStatus.cardEnum));
				arrayCardCount.Add(cardButton->cardStatus.stackSize);
			}
		};
		fillCards(LeftCardBox, command->array5, command->array6);
		fillCards(RightCardBox, command->array7, command->array8);
		
		networkInterface()->SendNetworkCommand(command);
	}

	SetVisibility(ESlateVisibility::Collapsed);
}

#undef LOCTEXT_NAMESPACE