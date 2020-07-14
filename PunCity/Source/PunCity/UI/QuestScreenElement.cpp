// Pun Dumnernchanvanit's


#include "QuestScreenElement.h"

FReply UQuestScreenElement::NativeOnMouseButtonDown(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent)
{
	PUN_CHECK(_callbackParent);
	if (InMouseEvent.GetEffectingButton() == EKeys::LeftMouseButton) {
		_callbackParent->CallBack1(this, _callbackEnum);
		_timesClicked++;
	}
	return FReply::Handled();
}
FReply UQuestScreenElement::NativeOnMouseButtonDoubleClick(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent)
{
	if (InMouseEvent.GetEffectingButton() == EKeys::LeftMouseButton) {
		_callbackParent->CallBack1(this, _callbackEnum);
		_timesClicked++;
	}
	return FReply::Handled();
}