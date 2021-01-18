// Fill out your copyright notice in the Description page of Project Settings.

#include "Temples.h"
#include "../Resource/ResourceSystem.h"
#include "../TreeSystem.h"
#include "../FateLinkSystem.h"

using namespace std;

//! Blossom Shrine

void BlossomShrine::OnTick1Sec()
{
	// Grow nearby trees
	TreeSystem& treeSystem = _simulation->treeSystem();

	TileArea area = _area;
	area.minX -= 12;
	area.minY -= 12;
	area.maxX += 12;
	area.maxY += 12;
	area.EnforceWorldLimit();
	area.ExecuteOnArea_WorldTile2([&](WorldTile2 tile) {
		treeSystem.TickTile(tile.tileId());
	});
}


void HellPortal::TickRound()
{
	resourceSystem().ChangeMoney(200);
	_simulation->uiInterface()->ShowFloatupInfo(FloatupEnum::GainMoney, centerTile(), "+200");
}


void AdventurersGuild::TickRound()
{
	if (isConstructed() && Time::IsAutumnStart()) {
		_simulation->GenerateRareCardSelection(_playerId, RareHandEnum::RareCards, NSLOCTEXT("SuccessfulVenture", "A successful venture!", "A successful venture!"));
	}
}