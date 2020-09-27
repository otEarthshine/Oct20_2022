// Fill out your copyright notice in the Description page of Project Settings.


#include "PopupSystem.h"
#include "PunCity/GameManagerInterface.h"

void PopupSystem::AddPopup(PopupInfo popup)
{
	popup.startDisplayTick = static_cast<int32>(_simulation->gameManagerInterface()->GetDisplayWorldTime() * 60);
	
	_popups.push_back(popup);
	if (_popups.size() > maxPopupsBeforeDelete) {
		_popups.erase(_popups.begin());
	}
}

void PopupSystem::AddPopupToFront(PopupInfo popup)
{
	popup.startDisplayTick = static_cast<int32>(_simulation->gameManagerInterface()->GetDisplayWorldTime() * 60);

	// Things that doesn't need reply shouldn't stack...
	bool shouldNotRepeat = popup.popupSound == "PopupCannot" || 
							popup.replyReceiver == PopupReceiverEnum::None ||
							popup.replyReceiver == PopupReceiverEnum::DoneResearchEvent_ShowTree;
	
	if (shouldNotRepeat && 
		_popups.size() > 0 &&
		_popups.front() == popup)
	{
		return; // No popup stacks for PopupCannot warning...
	}
	_popups.insert(_popups.begin(), popup);
}