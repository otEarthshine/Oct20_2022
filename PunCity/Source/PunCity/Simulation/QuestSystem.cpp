// Fill out your copyright notice in the Description page of Project Settings.

#include "QuestSystem.h"
#include "PunCity/UI/PunBoxWidget.h"
#include "BuildingCardSystem.h"
#include "Building.h"

using namespace std;

bool Quest::CanGetRewardCard() {
	return simulation->cardSystem(playerId).CanAddCardToBoughtHand(rewardCardEnum(), 1);
}
void Quest::GetRewardCard() {
	simulation->cardSystem(playerId).AddCardToHand2(rewardCardEnum());
}