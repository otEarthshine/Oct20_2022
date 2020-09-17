// Pun Dumnernchanvanit's


#include "PopupUI.h"
#include "PunCity/UI/PunBoxWidget.h"

using namespace std;

void UPopupUI::PunInit()
{
	PopupOverlay->SetVisibility(ESlateVisibility::Collapsed);
	PopupButton->OnClicked.AddDynamic(this, &UPopupUI::ClickPopupButton1);
	PopupButton2->OnClicked.AddDynamic(this, &UPopupUI::ClickPopupButton2);
	PopupButton3->OnClicked.AddDynamic(this, &UPopupUI::ClickPopupButton3);
	PopupButton4->OnClicked.AddDynamic(this, &UPopupUI::ClickPopupButton4);
	PopupButton5->OnClicked.AddDynamic(this, &UPopupUI::ClickPopupButton5);

	SetChildHUD(PopupBox);
}

void UPopupUI::Tick()
{
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
		if (shouldShowExclusive(ExclusiveUIEnum::TechUI))
		{
			shouldPopup = true;
			popupToDisplay->choices = { "Close" };
			popupToDisplay->replyReceiver = PopupReceiverEnum::None;
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
		else if (shouldShowExclusive(ExclusiveUIEnum::ArmyMoveUI)) {
			shouldPopup = true;
		}
		else if (shouldShowExclusive(ExclusiveUIEnum::BuildMenu)) {
			shouldPopup = true;
		}
	}
	
	if (!shouldPopup) {
		PopupOverlay->SetVisibility(ESlateVisibility::Collapsed);
		return;
	}
	
	if (popupToDisplay) 
	{
		// Popup changed, play sound...
		if (currentPopup.body != popupToDisplay->body ||
			currentPopup.startTick != popupToDisplay->startTick ||
			currentPopup.startDisplayTick != popupToDisplay->startDisplayTick)
		{
			currentPopup = *popupToDisplay;
			dataSource()->Spawn2DSound("UI", currentPopup.popupSound == "" ? "PopupNeutral" : currentPopup.popupSound);
		}

		PopupBox->AddRichTextParsed(popupToDisplay->body);
		PopupBox->AfterAdd();

		
		PopupOverlay->SetVisibility(ESlateVisibility::Visible);
		

		auto& choices = currentPopup.choices;
		bool hasChoice1 = choices.size() >= 1;
		PopupButton1Text->SetText(hasChoice1 ? ToFText(choices[0]) : FText::FromString("Close"));

		bool hasChoice2 = choices.size() >= 2;
		PopupButton2->SetVisibility(hasChoice2 ? ESlateVisibility::Visible : ESlateVisibility::Collapsed);
		if (hasChoice2) {
			PopupButton2Text->SetText(ToFText(choices[1]));
		}

		bool hasChoice3 = choices.size() >= 3;
		PopupButton3->SetVisibility(hasChoice3 ? ESlateVisibility::Visible : ESlateVisibility::Collapsed);
		if (hasChoice3) {
			PopupButton3Text->SetText(ToFText(choices[2]));
		}

		bool hasChoice4 = choices.size() >= 4;
		PopupButton4->SetVisibility(hasChoice4 ? ESlateVisibility::Visible : ESlateVisibility::Collapsed);
		if (hasChoice4) {
			PopupButton4Text->SetText(ToFText(choices[3]));
		}

		bool hasChoice5 = choices.size() >= 5;
		PopupButton5->SetVisibility(hasChoice5 ? ESlateVisibility::Visible : ESlateVisibility::Collapsed);
		if (hasChoice5) {
			PopupButton5Text->SetText(ToFText(choices[4]));
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

	dataSource()->Spawn2DSound("UI", "UIWindowClose");

	if (currentPopup.needNetworking()) {
		dataSource()->simulation().WaitForReply(playerId());

		auto popupDecision = make_shared<FPopupDecision>();
		popupDecision->replyReceiverIndex = static_cast<int32>(currentPopup.replyReceiver);
		popupDecision->choiceIndex = choiceIndex;
		popupDecision->replyVar1 = currentPopup.replyVar1;
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