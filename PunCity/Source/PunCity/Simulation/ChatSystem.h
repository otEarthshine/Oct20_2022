// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "PunCity/NetworkStructs.h"

/**
 * 
 */
class ChatSystem
{
public:
	void AddMessage(FSendChat chatMessage) {
		_messages.Add(chatMessage);
		//if (_messages.Num() > 20) {
		//	_messages.RemoveAt(0);
		//}
		needRefreshChatUI = true;
	}

	const TArray<FSendChat>& messages() const {
		return _messages;
	}

public:
	bool needRefreshChatUI = true;

private:
	TArray<FSendChat> _messages;
};
