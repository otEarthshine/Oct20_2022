// Fill out your copyright notice in the Description page of Project Settings.

#include "TownHall.h"
#include "PunCity/GameRand.h"
#include "PunCity/GameConstants.h"
#include "../OverlaySystem.h"
#include "../Resource/ResourceSystem.h"
#include "../PlayerOwnedManager.h"
#include "../ArmyStateAI.h"
#include "PunCity/CppUtils.h"
#include "../QuestSystem.h"
#include "../BuildingCardSystem.h"
#include "../UnlockSystem.h"

using namespace std;

#define LOCTEXT_NAMESPACE "TownHall"

static const TArray<FText> TownhallLvlToUpgradeBonusText =
{
	FText(),
	FText(),

	LOCTEXT("TownhallLvl2UpgradeBonus", 
		"Unlocked Cards:"
		"<bullet>Wheat seeds</>"
		"<bullet>Cabbage seeds</>"
		"<bullet>Snatch</>"
		"<bullet>Buy Wood</>"
		"<bullet>Sell Food</>"
	), // Lvl 2

	LOCTEXT("TownhallLvl3UpgradeBonus",
		"<bullet>+10% mine/quarry production.</>"
		"<space>"
		"Unlocked Cards:"
		"<bullet>Immigrants</>"
		"<bullet>Kidnap</>"
	), // 3

	LOCTEXT("TownhallLvl4UpgradeBonus",
		"<space>"
		"Unlocked Cards:"
		"<bullet>Warehouse</>"
		"<bullet>Sharing is caring</>"
	), // 4

	LOCTEXT("TownhallLvl5UpgradeBonus",
		"<bullet>+10% industrial production.</>"
	), // Lvl 5
};
const FText& GetTownhallLvlToUpgradeBonusText(int32 townhallLvl) {
	return TownhallLvlToUpgradeBonusText[townhallLvl];
}

void TownHall::FinishConstruction()
{
	Building::FinishConstruction();
	townhallLvl = 1;
	wallLvl = 1;

	// Need this check because Cheat FastBuild may activate FinishConstruction two times
	if (_simulation->population(_playerId) == 0) {
		AddInitialImmigrants();
	}

	// Townhall name
	if (_simulation->IsAIPlayer(_playerId)) {
		SetTownName(GetAITownName(_playerId));
	} else {
		SetTownName(_simulation->playerNameF(_playerId) + FString(" Town"));
	}

	//garrisons.Init(_simulation, _playerId);

	_simulation->RecalculateTaxDelayed(_playerId);

	armyNode.Init(_playerId, buildingId(), _simulation);

	_townStartTick = Time::Ticks();


	// Trailer Mode and Editor show no rare card
	if (PunSettings::TrailerMode()) {
		alreadyGotInitialCard = true;
	}
#if WITH_EDITOR
	//alreadyGotInitialCard = true;
#endif
}

static const std::vector<int32_t> townhallLvlToUpgradeMoney =
{
	0,
	0,
	200, // Lvl 2
	1000,
	5000,
	30000, // Lvl 5
};

int32 TownHall::GetMaxUpgradeLvl() {
	return townhallLvlToUpgradeMoney.size() - 1;
}

int32 TownHall::GetUpgradeMoney(int32 lvl) {
	return townhallLvlToUpgradeMoney[lvl];
}

// TODO: take out??
bool TownHall::HasEnoughUpgradeMoney()
{
	PUN_CHECK(townhallLvl < GetMaxUpgradeLvl());

	int32 upgradeMoney = GetUpgradeMoney(townhallLvl + 1);
	return resourceSystem().money() >= upgradeMoney;
}

void TownHall::UpgradeTownhall()
{	
	if (townhallLvl >= GetMaxUpgradeLvl()) {
		return;
	}
	
	// Remove Upgrade cost
	//const std::vector<int32_t>& upgradeResources = GetUpgradeResources(townhallLvl + 1);
	//for (int32_t j = 0; j < upgradeResources.size(); j++) {
	//	if (upgradeResources[j] > 0) {
	//		resourceSystem().RemoveResourceGlobal(ConstructionResources[j], upgradeResources[j]);
	//	}
	//}
	int32 upgradeMoney = GetUpgradeMoney(townhallLvl + 1);
	if (upgradeMoney > 0) {
		resourceSystem().ChangeMoney(-upgradeMoney);
	}

	// Upgrade
	townhallLvl++;
	ResetDisplay();

	// Unlock Popup
	{
		TArray<FText> args;
		ADDTEXT_LOCTEXT("Congratulation!", "Congratulation!");
		ADDTEXT_INV_("<space>");
		ADDTEXT_(LOCTEXT("TownNowLvlX", "Your townhall is now lvl {0}."), TEXT_NUM(townhallLvl));
		ADDTEXT_INV_("<space>");
		ADDTEXT__(TownhallLvlToUpgradeBonusText[townhallLvl]);

		_simulation->AddPopup(_playerId, 
			JOINTEXT(args),
			"UpgradeTownhall"
		);

		_simulation->AddPopupAll(PopupInfo(_playerId, 
			FText::Format(LOCTEXT("TownhallUpgradeToLvl_Pop", "{0} has been upgraded to level {1}"), townNameT(), TEXT_NUM(townhallLvl))
		), _playerId);
	}

	auto& cardSys = _simulation->cardSystem(_playerId);
	auto unlockSys = _simulation->unlockSystem(_playerId);

	if (townhallLvl == 2) {
		cardSys.AddDrawCards(CardEnum::Snatch, 1);
		cardSys.AddDrawCards(CardEnum::WheatSeed, 1);
		cardSys.AddDrawCards(CardEnum::CabbageSeed, 1);
		cardSys.AddDrawCards(CardEnum::SellFood, 1);
		cardSys.AddDrawCards(CardEnum::BuyWood, 1);
		//cardSys.AddDrawCards(CardEnum::BarrackClubman, 1);

		_simulation->AddPopup(_playerId, LOCTEXT("UnlockedPriorityButton_Pop",
				"Unlocked Priority Button <img id=\"NonPriorityStar\"/>!"
				"<space>"
				"Click it to switch building's priority between 3 states:\n"
				"  <img id=\"NonPriorityStar\"/> Default worker allocation\n"
				"  <img id=\"PriorityStar\"/> Prioritize working here\n"
				"  <img id=\"PriorityStop\"/> Don't allow working here"
		));
		unlockSys->unlockedPriorityStar = true;
	}
	else if (townhallLvl == 3) {
		cardSys.AddDrawCards(CardEnum::Immigration, 1);
		cardSys.AddDrawCards(CardEnum::Kidnap, 1);

		_simulation->AddPopup(_playerId, LOCTEXT("UnlockedSetTradeOffer_Pop",
			"Unlocked \"Set Trade Offer\" Button."
			"<space>"
			"Use it to put up Trade Offers at the Townhall.\n"
			"Other players can examine your Trade Offers, and directly trade with you (0% Fee)."
		));
		unlockSys->unlockedSetTradeAmount = true;
	}
	else if (townhallLvl == 4) {
		cardSys.AddDrawCards(CardEnum::Warehouse, 1);
		cardSys.AddDrawCards(CardEnum::SharingIsCaring, 1);

		{
			_simulation->AddPopup(_playerId, LOCTEXT("UnlockedSetDeliveryTarget_Pop",
				"Unlocked ability to Set Delivery Target!"
				"<space>"
				"You can set the storage/market where the building's output will be stored."
				"<space>"
				"To set the delivery target:"
				"<bullet>Click on a production building to bring up its panel</>"
				"<bullet>Click the [Set Delivery Target] button</>"
				"<bullet>Select the target you wish to deliver to</>"
			));
			unlockSys->unlockedSetDeliveryTarget = true;
		}

		{
			_simulation->AddPopup(
				PopupInfo(_playerId, 
					FText::Format(LOCTEXT("BuyCardTownhallUpgrade_Pop",
						"Would you like to buy a {0} card for {1} <img id=\"Coin\"/>."
						),
						GetBuildingInfo(CardEnum::Warehouse).name,
						TEXT_NUM(_simulation->cardSystem(_playerId).GetCardPrice(CardEnum::Warehouse))
					), 
					{ LOCTEXT("Buy", "Buy"),
						LOCTEXT("Refuse", "Refuse") },
					PopupReceiverEnum::DoneResearchBuyCardEvent, false, "ResearchComplete", static_cast<int>(CardEnum::Warehouse)
				)
			);
		}
	}

	_simulation->QuestUpdateStatus(_playerId, QuestEnum::TownhallUpgradeQuest);

	_simulation->RecalculateTaxDelayed(_playerId);

	
	_simulation->uiInterface()->ShowFloatupInfo(FloatupEnum::TownhallUpgrade, _centerTile, FText());

	//_simulation->soundInterface()->Spawn2DSoundAllPlayers("UI", "UpgradeTownhall", _centerTile);
}

void TownHall::UpgradeWall() {
	resourceSystem().RemoveResourceGlobal(ResourceEnum::Stone, GetUpgradeStones());
	wallLvl++;
	_simulation->AddPopup(_playerId,
		FText::Format(LOCTEXT("WallUpgraded_Pop", "{0}'s wall has been upgraded to level {1}."), townNameT(), TEXT_NUM(wallLvl))
	);
}

void TownHall::OnTick1Sec()
{
	migrationType = 0;
	
	// Update migrationPull
	int32 happiness = _simulation->GetAverageHappiness(_playerId);
	int32 population = _simulation->population(_playerId);
	int32 housingCapacity = _simulation->HousingCapacity(_playerId);

	// Population Factor
	migrationPull_populationSize = 5 + _simulation->population(_playerId) / 18; // 5 + X% of population

	
	// Free living space factor
	if (housingCapacity >= population) {
		// 40% of living space as migration pull
		migrationPull_freeLivingSpace = (housingCapacity - population) * 4 / 10; 
	} else {
		// 40% of homeless deters immigration, with
		migrationPull_freeLivingSpace = (housingCapacity - population) * 4 / 10; // 40% of living space as migration pull
		migrationPull_freeLivingSpace = max(-migrationPull_populationSize * 7 / 10, migrationPull_freeLivingSpace); // cannot decrease migrationPull more than 70%
	}

	if (migrationPull_freeLivingSpace > migrationPull_populationSize) {
		migrationType = 2;
	}

	int32 migrationPullSoFar = migrationPull_populationSize + migrationPull_freeLivingSpace;

	// Unemployment (Number of laborers)
	//int32 laborerCount = _simulation->playerOwned(_playerId).laborerCount();
	//if (population > 0) {
	//	migrationPull_unemployment = -migrationPullSoFar * laborerCount / population; // if everyone is laborer, don't migrate here...	
	//} else {
	//	migrationPull_unemployment = 0;
	//}
	//migrationPullSoFar += migrationPull_unemployment;

	
	// Happiness Factor
	if (happiness >= 50) {
		migrationPull_happiness = migrationPullSoFar * (happiness - 50) / 2 / 100; // At 100 happiness, migration is +25% (/2)
	} else {
		// Unhappiness affects more
		migrationPull_happiness = migrationPullSoFar * (happiness - 50) * 2 / 100; // At 0 happiness, there is no migration...
		migrationType = 1;
	}

	// Bonuses
	migrationPullSoFar += migrationPull_happiness;
	migrationPull_bonuses = 0;
	
	if (_simulation->IsResearched(_playerId, TechEnum::TradingPost)) {
		migrationPull_bonuses += migrationPullSoFar * 20 / 100; // 20% immigration increase from foreign trade...
	}

	if (_simulation->buildingFinishedCount(_playerId, CardEnum::ImmigrationPropagandaOffice)) {
		migrationPull_bonuses += migrationPullSoFar * 30 / 100; // 30% increase
	}
}

std::vector<FText> TownHall::getImmigrationEventChoices() {
	std::vector<FText> choices = {
		LOCTEXT("Accept", "Accept"),
		LOCTEXT("Refuse", "Refuse")
	};
	if (_simulation->TownhallCardCount(_playerId, CardEnum::Cannibalism)) {
		choices.push_back(LOCTEXT("KillStealCanni", "kill, steal, and eat (Cannibalism)"));
	}
	return choices;
}

void TownHall::ImmigrationEvent(int32 exactAmount, FText message, PopupReceiverEnum replyReceiver)
{
	askedMigration = exactAmount;
	_simulation->AddPopup(PopupInfo(_playerId, 
		message, 
		getImmigrationEventChoices(), replyReceiver
	));
}

void TownHall::ImmigrationEvent(int32 exactAmount)
{
	//PUN_LOG("ImmigrationEvent");
	askedMigration = migrationPull();
	
	// force add
	if (exactAmount != -1) {
		askedMigration = exactAmount;
		_simulation->AddPopup(PopupInfo(_playerId, 
			FText::Format(LOCTEXT("ImmigrantsAskedToJoin_Pop", "{0} immigrants asked to join your colony."), TEXT_NUM(askedMigration)),
			getImmigrationEventChoices(), PopupReceiverEnum::ImmigrationEvent
		));
		return;
	}

	if (askedMigration == 0) {
		_simulation->AddPopup(_playerId, 
			LOCTEXT("NoOneWantsToJoin_Pop", "Not a single soul wants to join your town.")
		);
	}

	if (migrationType == 1) {
		// Low happiness... less immigration
		ImmigrationEvent(askedMigration,
			FText::Format(LOCTEXT("ImmigrantsDesperateAskToJoin_Pop", 
				"{0} desparate immigrants asked to join your colony. Low happiness lessen immigration."), 
				TEXT_NUM(askedMigration)
			)
		);
	}
	else if (migrationType == 2) {
		ImmigrationEvent(askedMigration,
			FText::Format(LOCTEXT("ImmigrantsSpaceAskToJoin_Pop", 
				"{0} immigrants asked to join your colony. They heard that your town has plenty of available living space. Would you let them join your town?"), 
				TEXT_NUM(askedMigration)
			)
		);
	}
	else if (migrationType == 0) {
		ImmigrationEvent(askedMigration,
			FText::Format(LOCTEXT("ImmigrantsGreatRumorsAskToJoin_Pop", 
				"{0} immigrants asked to join your colony. They came from faraway land, filled with hope, after hearing great rumors about your town. Would you let them join your town?"),
				TEXT_NUM(askedMigration)
			)
		);
	}
	else {
		UE_DEBUG_BREAK();
	}
}


#undef LOCTEXT_NAMESPACE