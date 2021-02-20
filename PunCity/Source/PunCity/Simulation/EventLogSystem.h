// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "GameSimulationInfo.h"
#include "PunCity/CppUtils.h"
#include "PunCity/PunUtils.h"

struct EventLog
{
	FString message;
	bool isImportant;
	int32 startTick;

	FText messageT() const { return FText::FromString(message); }

	//! Serialize
	FArchive& operator>>(FArchive &Ar) {
		Ar << message;
		Ar << isImportant;
		Ar << startTick;
		return Ar;
	}
};

/**
 * 
 */
class EventLogSystem
{
public:
	void AddPlayer() {
		//PUN_LOG("EventLogSystem AddPlayer:%d", _playerIdToEvents.size());
		_playerIdToEvents.push_back(std::vector<EventLog>());
		needRefreshEventLog.push_back(true);
	}

	void ResetPlayer(int32 playerId) {
		//PUN_LOG("EventLogSystem ResetPlayer:%d", playerId);
		_playerIdToEvents[playerId] = std::vector<EventLog>();
		needRefreshEventLog[playerId] = true;
	}

	void AddEventLog(int32 playerId, FText eventMessage, bool isImportant)
	{
		if (playerId == -1) {
			return;
		}

		std::vector<EventLog>& events = _playerIdToEvents[playerId];
		events.push_back({ eventMessage.ToString(), isImportant, Time::Ticks() });
		if (events.size() > 5) {
			events.erase(events.begin());
		}
		needRefreshEventLog[playerId] = true;
	}

	void Tick1Sec()
	{
		for (size_t id = 0; id < _playerIdToEvents.size(); id++) {
			std::vector<EventLog>& events = _playerIdToEvents[id];
			for (int32 i = events.size(); i-- > 0;) {
				if (Time::Ticks() > events[i].startTick + _logLifeSpanSeconds * Time::TicksPerSecond) {
					events.erase(events.begin() + i);
					needRefreshEventLog[id] = true;
				}
			}
		}
	}

	const std::vector<EventLog>& events(int32 playerId) const
	{
		return _playerIdToEvents[playerId];
	}

	void Serialize(FArchive& Ar)
	{
		//PUN_LOG("---------------");
		//PUN_LOG("_playerIdToEvents: %d", _playerIdToEvents.size());
		//for (std::vector<EventLog>& eventLogs : _playerIdToEvents) {
		//	PUN_LOG("- eventLogs: %d", eventLogs.size());
		//	for (EventLog& log : eventLogs) {
		//		PUN_LOG("-- EventLog: %s %d %d", *log.message, log.isImportant, log.startTick);
		//	}
		//}

		//if (Ar.IsSaving()) {
		//	FBufferArchive SaveArchive;
		//	SaveArchive.SetIsSaving(true);
		//	SaveArchive.SetIsLoading(false);
		//	
		//	int32 checksum = FCrc::MemCrc32(SaveArchive.GetData(), SaveArchive.Num());
		//	PUN_LOG("-- checksum_BEFORE size:%d check:%d", SaveArchive.Num(), checksum);

		//	SerializeVecVecObj(SaveArchive, _playerIdToEvents);
		//	checksum = FCrc::MemCrc32(SaveArchive.GetData(), SaveArchive.Num());
		//	PUN_LOG("-- checksum_AFTER size:%d check:%d", SaveArchive.Num(), checksum);
		//	
		//}

		SerializeVecVecObj(Ar, _playerIdToEvents);

		// Event log must be refresh after load. Serialize is just to resize...
		if (Ar.IsLoading()) {
			VecReset(needRefreshEventLog, true);
		}
	}

public:
	std::vector<bool> needRefreshEventLog;

private:
	const int32 _logLifeSpanSeconds = 30;

	std::vector<std::vector<EventLog>> _playerIdToEvents;
};
