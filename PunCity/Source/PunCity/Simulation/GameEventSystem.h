// Fill out your copyright notice in the Description page of Project Settings.

#pragma once
#include "../PunUtils.h"

#include <vector>
#include <functional>
#include <unordered_map>
#include <memory>

enum class EventSourceEnum
{
	BuiltHouse,
	Dropoff,
	HouseUpgrade,
	PopulationChange,
	
	Count,
	
	None,
};



class GameEventSource
{
public:
	EventSourceEnum eventEnum() { return _eventEnum; }
	
	void Init(EventSourceEnum eventEnum) {
		_eventEnum = eventEnum;
	}

	bool IsSubscribed(int32_t playerId, std::string tag)
	{
		PUN_CHECK(playerId != -1);
		bool hasSubscription = _playerIdToTagToEventHandler[playerId].find(tag) != _playerIdToTagToEventHandler[playerId].end();
		return hasSubscription;
	}

	void Subscribe(int32_t playerId, std::string tag, std::function<void(int)> eventHandler) {
		PUN_CHECK(playerId != -1);
		PUN_CHECK(_playerIdToTagToEventHandler[playerId].find(tag) == _playerIdToTagToEventHandler[playerId].end());
		
		_playerIdToTagToEventHandler[playerId][tag] = eventHandler;
		//PUN_LOG("Subscribing: player:%d tag: %s", playerId, ToTChar(tag));
		//PUN_DEBUG(FString::Printf(TEXT("Subscribing: %s player:%d tag: %s"), *eventName(), playerId, *FString(tag.c_str())));
	}

	void Unsubscribe(int32_t playerId, std::string tag) {
		PUN_CHECK(playerId != -1);
		PUN_CHECK(_playerIdToTagToEventHandler[playerId].find(tag) != _playerIdToTagToEventHandler[playerId].end());
		//_removeList[playerId] = tag;
		//_playerIdToTagToEventHandler[playerId].erase(tag);
		//PUN_DEBUG(FString::Printf(TEXT("Unsubscribing: %s player:%d tag: %s"), *eventName(), playerId, *FString(tag.c_str())));

		_playerIdToTagToEventHandler[playerId].erase(tag);
	}

	// TODO: Need to find a way to unsubscribe and not break this shit
	void Publish(int32_t playerId, int objectId) 
	{
		//// Remove on the next publish to prevent publish->unsubscribe call from clashing (tagToEventHandler editing)
		//for (auto it : _removeList) {
		//	check(_playerIdToTagToEventHandler[it.first].find(it.second) != _playerIdToTagToEventHandler[playerId].end());
		//	_playerIdToTagToEventHandler[it.first].erase(it.second);
		//}
		//_removeList.clear();

		auto tagToEventHandlerIt = _playerIdToTagToEventHandler.find(playerId);
		if (tagToEventHandlerIt != _playerIdToTagToEventHandler.end())
		{
			std::unordered_map<std::string, std::function<void(int)>> tagToEventHandler = tagToEventHandlerIt->second;
			//PUN_LOG("tagToEventHandler %d", tagToEventHandler.size());
			
			for (const auto& it : tagToEventHandler) {
				//PUN_LOG(" iterate size:%d it: %s", tagToEventHandler.size(), ToTChar(it.first));
				it.second(objectId);
			}
		}
	}

	void ClearSubscriptions()
	{
		for (auto& tagToEventHandlerIt :_playerIdToTagToEventHandler) {
			tagToEventHandlerIt.second.clear();
		}
	}

private:
	EventSourceEnum _eventEnum = EventSourceEnum::None;

	std::unordered_map<int32, std::unordered_map<std::string, std::function<void(int)>>> _playerIdToTagToEventHandler;

	//std::unordered_map<int32, std::string> _removeList;
};

/**
 * 
 */
class GameEventSystem
{
public:
	void Init() {
		for (size_t i = 0; i < static_cast<int32>(EventSourceEnum::Count); i++) {
			_eventSources.push_back(GameEventSource());
			_eventSources[i].Init(static_cast<EventSourceEnum>(i));
		}
	}

	GameEventSource& source(EventSourceEnum eventEnum) {
		return _eventSources[static_cast<int32>(eventEnum)];
	}

	void ClearSubscriptions() {
		for (auto& source : _eventSources) {
			source.ClearSubscriptions();
		}
	}

private:
	std::vector<GameEventSource> _eventSources;
};
