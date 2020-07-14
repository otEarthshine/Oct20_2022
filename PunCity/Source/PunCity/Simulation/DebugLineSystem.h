// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "GameSimulationInfo.h"

struct PunDebugLine
{
	WorldAtom2 startAtom;
	FVector startShift;
	WorldAtom2 endAtom;
	FVector endShift;
	FLinearColor Color;
	float thickness;
	float endTick;
};

/**
 * Keep debug line information to be displayed.
 * Allows drawing lines from simulation code
 * RegionDisplay grab info to render out the lines
 */
class DebugLineSystem
{
public:
	void Init() {
		_regionToDebugLines.resize(GameMapConstants::TotalRegions);
	}

	void DrawLine(WorldAtom2 startAtom, FVector startShift, WorldAtom2 endAtom, FVector endShift, FLinearColor Color,
		float Thickness = 1.0f, float LifeTime = Time::TicksPerSecond)
	{
		PunDebugLine line;
		line.startAtom = startAtom;
		line.startShift = startShift;
		line.endAtom = endAtom;
		line.endShift = endShift;
		line.Color = Color;
		line.thickness = Thickness;
		line.endTick = Time::Ticks() + LifeTime;

		// Add twice, but don't care, just debugging
		_regionToDebugLines[startAtom.region().regionId()].push_back(line);
		_regionToDebugLines[endAtom.region().regionId()].push_back(line);
	}

	std::vector<PunDebugLine>& GetLines(int32 regionId) {
		// Clean out lines with high 
		auto& debugLines = _regionToDebugLines[regionId];
		for (int32 i = debugLines.size(); i-- > 0;) {
			if (Time::Ticks() >= debugLines[i].endTick) {
				debugLines.erase(debugLines.begin() + i);
			}
		}
		return debugLines;
	}

private:
	std::vector<std::vector<PunDebugLine>> _regionToDebugLines;
};
