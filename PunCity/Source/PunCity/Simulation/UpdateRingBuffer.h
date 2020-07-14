// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "UnitBase.h"
#include <array>
#include "PunCity/PunUtils.h"

//struct UnitUpdateInfo
//{
//	int32_t unitId = -1;
//	int32_t nextUpdateTick = 0; // pack state into this??
//	TransformState state;
//
//	//! Debug
//	int32_t queuedTick;
//	std::string caller = "";
//};

struct UpdateRingInfo
{
	int32_t unitId = -1;
	int32_t nextUpdateTick = 0;

	//! Serialize
	FArchive& operator>>(FArchive &Ar) {
		Ar << unitId << nextUpdateTick;
		return Ar;
	}
};

class UpdateRingBuffer
{
public:
	void Init(int32_t tickCountShiftIn) {
		tickCountShift = tickCountShiftIn;

		size_t size = 1ULL << tickCountShift;
		tickCountMask = size - 1;

		_currentIndex = 0;
		_tickToUpdateInfos.resize(size);
	}

	// (next - tick - 1) / size
	// For example, tick = 5, size = 10
	// next = 15: (15 - 5 - 1) / 10 = 0
	// next = 16: (16 - 5 - 1) / 10 = 1
	void AddUpdateInfo(int32_t unitId, int32_t nextUpdateTick, TransformState state, std::string& caller)
	{
		//SCOPE_CYCLE_COUNTER(STAT_PunUnitUpdateAdd);

		//if (unitId == 8849) {
		//	PUN_LOG("Unit state %d, caller:%s", state, *ToFString(caller));
		//}

		UpdateRingInfo info = { unitId, nextUpdateTick };

		int32_t targetIndex = nextUpdateTick & tickCountMask;
		// if this is on the same index, cache it to be added after this tick ends
		if (targetIndex == _currentIndex) {
			_updateInfosOnSameTick.push_back(info);
			//PUN_LOG("AddUpdateInfo2 targetIndex:%d _currentIndex:%d", targetIndex, _currentIndex);
		}
		else {
			_tickToUpdateInfos[targetIndex].push_back(info);
			//PUN_LOG("AddUpdateInfo3 targetIndex:%d _currentIndex:%d", targetIndex, _currentIndex);
		}
	}

	void AddBackUnripeTick(UpdateRingInfo info) {
		_updateInfosOnSameTick.push_back(info);
	}

	// Increment current index after processing it
	void AfterProcess();

	const std::vector<UpdateRingInfo>& unitsToUpdate() const {
		check((Time::Ticks() & tickCountMask) == _currentIndex);
		return _tickToUpdateInfos[_currentIndex];
	}

	void RemoveUpdateInfo(int32_t unitId, int32_t nextUpdateTick)
	{
		int32_t targetIndex = nextUpdateTick & tickCountMask;
		std::vector<UpdateRingInfo>& sameTickInfos = _tickToUpdateInfos[targetIndex];
		for (int i = sameTickInfos.size(); i-- > 0;) {
			if (sameTickInfos[i].unitId == unitId) {
				sameTickInfos.erase(sameTickInfos.begin() + i);
				return;
			}
		}
		//UE_DEBUG_BREAK();
	}

#if WITH_EDITOR
	void CheckNoUpdateInfo(int32_t unitId, int32_t ignoreTick) {
		int32_t ignoreIndex = ignoreTick & tickCountMask;
		for (size_t i = 0; i < _tickToUpdateInfos.size(); i++) {
			if (i != ignoreIndex) {
				std::vector<UpdateRingInfo>& infos = _tickToUpdateInfos[i];
				for (UpdateRingInfo& info : infos) {
					if (info.unitId == unitId) {
						UE_DEBUG_BREAK();
					}
				}
			}
		}
	}
#endif

	void Serialize(FArchive& Ar)
	{
		Ar << tickCountShift;
		Ar << tickCountMask;
		Ar << _currentIndex;
		SerializeVecVecObj(Ar, _tickToUpdateInfos);
		SerializeVecObj(Ar, _updateInfosOnSameTick);
	}

public:
	int32_t tickCountShift;
	int32_t tickCountMask;

private:
	int32_t _currentIndex;

	std::vector<std::vector<UpdateRingInfo>> _tickToUpdateInfos; // unitId*2 is lastUpdate... unitId*2+1 is nextUpdate

	std::vector<UpdateRingInfo> _updateInfosOnSameTick;
};