// Pun Dumnernchanvanit's


#include "PopupUI.h"
#include "PunCity/UI/PunBoxWidget.h"

using namespace std;

#define LOCTEXT_NAMESPACE "PopupUI"

void UPopupUI::PunInit()
{
	PopupOverlay->SetVisibility(ESlateVisibility::Collapsed);
	PopupButton->OnClicked.AddDynamic(this, &UPopupUI::ClickPopupButton1);
	PopupButton2->OnClicked.AddDynamic(this, &UPopupUI::ClickPopupButton2);
	PopupButton3->OnClicked.AddDynamic(this, &UPopupUI::ClickPopupButton3);
	PopupButton4->OnClicked.AddDynamic(this, &UPopupUI::ClickPopupButton4);
	PopupButton5->OnClicked.AddDynamic(this, &UPopupUI::ClickPopupButton5);

	EraPopupOverlay->SetVisibility(ESlateVisibility::Collapsed);
	EraPopupButton->OnClicked.AddDynamic(this, &UPopupUI::ClickPopupButton1);

	SetChildHUD(PopupBox);
}

void UPopupUI::Tick()
{
	LEAN_PROFILING_UI(TickPopupUI);
	
	PopupInfo* popupToDisplay = dataSource()->simulation().PopupToDisplay(playerId());
	auto punHUD = GetPunHUD();
	bool shouldPopup = popupToDisplay != nullptr;
	
	// Do not popup during card choosing/trading
	if (punHUD->IsDoingUITask() && popupToDisplay != nullptr) 
	{
		shouldPopup = false;

		auto shouldShowExclusive = [&](ExclusiveUIEnum exclusiveUIEnum) {
			return popupToDisplay->warningForExclusiveUI == exclusiveUIEnum &&
					GetPunHUD()->IsShowingExclusiveUI(exclusiveUIEnum);
		};
		
		// Should still display Tech popup in front of Tech UI
		// Shouldn't show the "Show tech tree" in this case
		if (shouldShowExclusive(ExclusiveUIEnum::TechTreeUI))
		{
			shouldPopup = true;
			popupToDisplay->choices = { LOCTEXT("Close", "Close") };
			//popupToDisplay->replyReceiver = PopupReceiverEnum::None; // Why is this here?
		}
		if (shouldShowExclusive(ExclusiveUIEnum::ProsperityUI)) {
			shouldPopup = true;
		}
		else if (shouldShowExclusive(ExclusiveUIEnum::Trading)) {
			shouldPopup = true;
		}
		else if (shouldShowExclusive(ExclusiveUIEnum::TargetConfirm)) {
			shouldPopup = true;
		}
		else if (shouldShowExclusive(ExclusiveUIEnum::CardHand1)) {
			shouldPopup = true;
		}
		else if (shouldShowExclusive(ExclusiveUIEnum::ConverterCardHand)) {
			shouldPopup = true;
		}
		else if (shouldShowExclusive(ExclusiveUIEnum::RareCardHand)) {
			shouldPopup = true;
		}
		//else if (shouldShowExclusive(ExclusiveUIEnum::ArmyMoveUI)) {
		//	shouldPopup = true;
		//}
		else if (shouldShowExclusive(ExclusiveUIEnum::BuildMenu)) {
			shouldPopup = true;
		}
		else if (shouldShowExclusive(ExclusiveUIEnum::InitialResourceUI)) {
			shouldPopup = true;
		}
		else if (shouldShowExclusive(ExclusiveUIEnum::DiplomacyUI)) {
			shouldPopup = true;
		}
		else if (shouldShowExclusive(ExclusiveUIEnum::GiftResourceUI)) {
			shouldPopup = true;
		}
		else if (shouldShowExclusive(ExclusiveUIEnum::SendImmigrantsUI)) {
			shouldPopup = true;
		}
	}
	
	if (!shouldPopup) {
		PopupOverlay->SetVisibility(ESlateVisibility::Collapsed);
		return;
	}
	
	if (popupToDisplay) 
	{
		// Special case: Era Popups
		PopupReceiverEnum receiverEnum = popupToDisplay->replyReceiver;
		if (receiverEnum == PopupReceiverEnum::StartGame_Story ||
			receiverEnum == PopupReceiverEnum::EraPopup_MiddleAge ||
			receiverEnum == PopupReceiverEnum::EraPopup_EnlightenmentAge ||
			receiverEnum == PopupReceiverEnum::EraPopup_IndustrialAge)
		{
			PopupBox->AfterAdd();
			PopupOverlay->SetVisibility(ESlateVisibility::Collapsed);
			EraPopupOverlay->SetVisibility(ESlateVisibility::Visible);

			currentPopup = *popupToDisplay;
			// Sound for this is played from when popup executes

			// EraSwitcher
			int32 activeWidgetIndex = 0;
			switch(receiverEnum) {
				case PopupReceiverEnum::StartGame_Story:		activeWidgetIndex = 0; break;
				case PopupReceiverEnum::EraPopup_MiddleAge:		activeWidgetIndex = 1; break;
				case PopupReceiverEnum::EraPopup_EnlightenmentAge: activeWidgetIndex = 2; break;
				case PopupReceiverEnum::EraPopup_IndustrialAge: activeWidgetIndex = 3; break;
				default: break;
			}
			EraSwitcher->SetActiveWidgetIndex(activeWidgetIndex);
			
			EraPopupButtonText->SetText((receiverEnum == PopupReceiverEnum::StartGame_Story) ? LOCTEXT("CheersBeginning", "Cheers to the new beginning!") : LOCTEXT("Continue", "Continue"));
		}
		else
		{
			// Popup changed, play sound...
			if (!TextEquals(currentPopup.body, popupToDisplay->body) ||
				currentPopup.startTick != popupToDisplay->startTick ||
				currentPopup.startDisplayTick != popupToDisplay->startDisplayTick)
			{
				currentPopup = *popupToDisplay;
				dataSource()->Spawn2DSound("UI", currentPopup.popupSound == "" ? "PopupNeutral" : currentPopup.popupSound);
			}

			PopupBox->AddRichTextParsed(popupToDisplay->body, UIEnum::PunRichText_Popup);
			PopupBox->AfterAdd();


			PopupOverlay->SetVisibility(ESlateVisibility::Visible);
			EraPopupOverlay->SetVisibility(ESlateVisibility::Collapsed);


			auto& choices = currentPopup.choices;
			bool hasChoice1 = choices.size() >= 1;
			PopupButton1Text->SetText(hasChoice1 ? choices[0] : LOCTEXT("Close", "Close"));

			bool hasChoice2 = choices.size() >= 2;
			PopupButton2->SetVisibility(hasChoice2 ? ESlateVisibility::Visible : ESlateVisibility::Collapsed);
			if (hasChoice2) {
				PopupButton2Text->SetText(choices[1]);
			}

			bool hasChoice3 = choices.size() >= 3;
			PopupButton3->SetVisibility(hasChoice3 ? ESlateVisibility::Visible : ESlateVisibility::Collapsed);
			if (hasChoice3) {
				PopupButton3Text->SetText(choices[2]);
			}

			bool hasChoice4 = choices.size() >= 4;
			PopupButton4->SetVisibility(hasChoice4 ? ESlateVisibility::Visible : ESlateVisibility::Collapsed);
			if (hasChoice4) {
				PopupButton4Text->SetText(choices[3]);
			}

			bool hasChoice5 = choices.size() >= 5;
			PopupButton5->SetVisibility(hasChoice5 ? ESlateVisibility::Visible : ESlateVisibility::Collapsed);
			if (hasChoice5) {
				PopupButton5Text->SetText(choices[4]);
			}
		}
	}
}


void UPopupUI::ClickPopupButton1() {
	ClickPopupButton(0);
}
void UPopupUI::ClickPopupButton2() {
	ClickPopupButton(1);
}
void UPopupUI::ClickPopupButton3() {
	ClickPopupButton(2);
}
void UPopupUI::ClickPopupButton4() {
	ClickPopupButton(3);
}
void UPopupUI::ClickPopupButton5() {
	ClickPopupButton(4);
}

void UPopupUI::ClickPopupButton(int32 choiceIndex)
{
	if (InterfacesInvalid()) return;
	check(currentPopup.playerId == playerId());

	PopupOverlay->SetVisibility(ESlateVisibility::Collapsed);
	EraPopupOverlay->SetVisibility(ESlateVisibility::Collapsed);

	dataSource()->Spawn2DSound("UI", "UIWindowClose");

	if (currentPopup.needNetworking()) {
		dataSource()->simulation().WaitForReply(playerId());

		auto popupDecision = make_shared<FPopupDecision>();
		popupDecision->replyReceiverIndex = static_cast<int32>(currentPopup.replyReceiver);
		popupDecision->choiceIndex = choiceIndex;
		popupDecision->replyVar1 = currentPopup.replyVar1;
		popupDecision->replyVar2 = currentPopup.replyVar2;
		popupDecision->replyVar3 = currentPopup.replyVar3;
		networkInterface()->SendNetworkCommand(popupDecision);
	}
	else {
		// Instant reply for popup that has only 1 choice etc.
		if (currentPopup.replyReceiver != PopupReceiverEnum::None) {
			dataSource()->simulation().PopupInstantReply(playerId(), currentPopup.replyReceiver, choiceIndex);
		}
		
		dataSource()->simulation().CloseCurrentPopup(playerId());
	}
}

#undef LOCTEXT_NAMESPACE