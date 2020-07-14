// Pun Dumnernchanvanit's


#include "PunSelectButton.h"

void UPunSelectButton::OnButtonClicked() {
	PUN_CHECK(_callbackParent);
	_callbackParent->CallBack1(this, _callbackEnum);
}