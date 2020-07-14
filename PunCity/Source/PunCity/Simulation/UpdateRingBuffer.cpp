// Fill out your copyright notice in the Description page of Project Settings.


#include "UpdateRingBuffer.h"
#include "PunCity/PunUtils.h"
#include "UnrealEngine.h"

DECLARE_CYCLE_STAT(TEXT("PUN: Unit.Update.Add [0.1]"), STAT_PunUnitUpdateAdd, STATGROUP_Game);
DECLARE_CYCLE_STAT(TEXT("PUN: Unit.Update.After [0.1]"), STAT_PunUnitUpdateAfter, STATGROUP_Game);


void UpdateRingBuffer::AfterProcess() 
{
	SCOPE_CYCLE_COUNTER(STAT_PunUnitUpdateAfter);

	// Clear the last index's used infos and insert updateInfosOnSameTick
	auto& updateInfos = _tickToUpdateInfos[_currentIndex];
	updateInfos.clear();
	//updateInfos.insert(updateInfos.end(), _updateInfosOnSameTick.begin(), _updateInfosOnSameTick.end());
	updateInfos = _updateInfosOnSameTick;
	_updateInfosOnSameTick.clear();

	_currentIndex = (_currentIndex + 1) & tickCountMask;

	//UE_LOG(LogTemp, Error, TEXT("AfterProcess tick:%d index:%d"), Time::Ticks(), _currentIndex);
}