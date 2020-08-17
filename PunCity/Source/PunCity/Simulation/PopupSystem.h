// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"

#include "IGameSimulationCore.h"
#include "PunCity/NetworkStructs.h"
#include "PunCity/PunUtils.h"

/**
 * 
 * Note that GameSimulationCore should store popups of all players so that, when saved, we can rebuild popup of all players (in case they have no made a reply on it)
 * !!!... Right now no regard is made to saving though...
 */
class PopupSystem
{
public:
	PopupSystem(int32 playerId, IGameSimulationCore* simulation) {
		_playerId = playerId;
		_simulation = simulation;
	}

	void AddPopup(PopupInfo popup) {
		_popups.push_back(popup);
		if (_popups.size() > maxPopupsBeforeDelete) {
			_popups.erase(_popups.begin());
		}
	}

	void AddPopupToFront(PopupInfo popup) {
		if (popup.popupSound == "PopupCannot" &&
			_popups.size() > 0 &&
			_popups.front() == popup) 
		{
			return; // No popup stacks for PopupCannot warning...
		}
		_popups.insert(_popups.begin(), popup);
	}

	PopupInfo* PopupToDisplay()
	{
		if (waitingForReply) {
			return nullptr;
		}

		if (PunSettings::TrailerMode()) {
			ClearPopups();
			return nullptr;
		}
		
		return _popups.empty() ? nullptr : &_popups[0];
	}

	void ClosePopupToDisplay() {
		if (_popups.size() > 0) {
			_popups.erase(_popups.begin());
		}
		waitingForReply = false;
	}

	void ClearPopups() {
		_popups.clear();
		waitingForReply = false;
	}

	void TickRound() {
		for (size_t i = _popups.size(); i-- > 0;) {
			if (Time::Ticks() - _popups[i].startTick > Time::TicksPerSeason) {
				_popups.erase(_popups.begin() + i);
			}
		}
	}

	void Serialize(FArchive& Ar)
	{
		SerializeVecObj(Ar, _popups);
	}

public:
	bool waitingForReply = false;

private:
	static const int32 maxPopupsBeforeDelete = 5;
	
	int32 _playerId;
	IGameSimulationCore* _simulation;

	/*
	 * Serialize
	 */
	std::vector<PopupInfo> _popups; //PlayerId to popups
};
