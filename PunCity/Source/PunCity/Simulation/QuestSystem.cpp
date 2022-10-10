// Fill out your copyright notice in the Description page of Project Settings.

#include "QuestSystem.h"
#include "PunCity/UI/PunBoxWidget.h"
#include "BuildingCardSystem.h"
#include "UnlockSystem.h"
#include "Building.h"

using namespace std;

#define LOCTEXT_NAMESPACE "QuestSystem"

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

void BuildHousesQuest::OnFinishQuest()
{
	PUN_CHECK(simulation);
	auto unlockSys = simulation->unlockSystem(playerId);
	unlockSys->townhallUpgradeUnlocked = true;

	AddEndPopup(
		LOCTEXT("BuildHouses_Finish", "Well done! Your people are happy that they finally have houses to live in.<space>Providing enough housing is important. Homeless people can migrate away, or die during winter. On the other hand, having extra houses can attract immigrants.")
	);

	if (simulation->GetTownLvl(playerId) == 1) {
		simulation->parameters(playerId)->NeedTownhallUpgradeNoticed = true;
	}
}

void TownhallUpgradeQuest::OnFinishQuest()
{
	PUN_CHECK(simulation);
	auto unlockSys = simulation->unlockSystem(playerId);
	unlockSys->townhallUpgradeUnlocked = true;

	AddEndPopup(
		LOCTEXT("UpgradeTownhall_Finish", "Great! Your townhall is now level 2.<space>Upgrading the Townhall, Researching Technology, and Upgrading Houses are ways you progress through the game and unlock new gameplay elements."
	));

}


#undef LOCTEXT_NAMESPACE