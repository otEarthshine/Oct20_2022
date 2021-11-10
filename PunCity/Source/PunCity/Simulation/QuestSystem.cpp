// Fill out your copyright notice in the Description page of Project Settings.

#include "QuestSystem.h"
#include "PunCity/UI/PunBoxWidget.h"
#include "BuildingCardSystem.h"
#include "Building.h"

using namespace std;

bool Quest::CanGetRewardCard() {
	return simulation->cardSystem(playerId).CanAddCardsToBoughtHandOrInventory(rewardCardEnum());
}
void Quest::GetRewardCard() {
	simulation->cardSystem(playerId).AddCards_BoughtHandAndInventory(rewardCardEnum());
}


bool GatherMarkQuest::ShouldSkipToNextQuest()
{
	if (simulation->playerOwned(playerId).alreadyDidGatherMark) {
		return true;
	}
	BiomeEnum biomeEnum = simulation->GetBiomeEnum(simulation->GetTownhallMajor(playerId).centerTile());
	bool isForestBiome = (biomeEnum == BiomeEnum::Forest) || 
						(biomeEnum == BiomeEnum::BorealForest) || 
						(biomeEnum == BiomeEnum::Jungle);
	return !isForestBiome;
}