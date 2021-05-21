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
#include "../UnitStateAI.h"

using namespace std;

#define LOCTEXT_NAMESPACE "TownHall"

static const TArray<FText> TownhallLvlToUpgradeBonusText =
{
	FText(),
	FText(),

	LOCTEXT("TownhallLvl2UpgradeBonus", "Unlocked Cards:<bullet>Wheat seeds</><bullet>Cabbage seeds</><bullet>Snatch</><bullet>Buy Wood</><bullet>Sell Food</>"), // Lvl 2

	LOCTEXT("TownhallLvl3UpgradeBonus", "<bullet>+10% mine/quarry production.</><space>Unlocked Cards:<bullet>Immigrants</><bullet>Kidnap</>"), // 3

	LOCTEXT("TownhallLvl4UpgradeBonus", "<space>Unlocked Cards:<bullet>Sharing is caring</>"), // 4

	//LOCTEXT("TownhallLvl5UpgradeBonus", "<bullet>+10% industrial production.</>"), // Lvl 5
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
	if (_simulation->populationTown(_townId) == 0) {
		AddInitialImmigrants();
	}

	// Townhall name
	if (_simulation->IsAIPlayer(_playerId)) {
		SetTownName(GetAITownName(_playerId));
	} else {
		FString suffix = FString(" Town");
		if (!isCapital()) {
			suffix = FString(" Colony ") + FString::FromInt(_simulation->playerOwned(_playerId).playerTownNumber(_townId));
		}
		SetTownName(_simulation->playerNameF(_playerId) + suffix);
	}

	_simulation->RecalculateTaxDelayedTown(_townId);

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
	1000, // Lvl 2
	7000,
	50000,
	//30000, // Lvl 5
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
	return globalResourceSystem().money() >= upgradeMoney;
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
		globalResourceSystem().ChangeMoney(-upgradeMoney);
	}

	// Upgrade
	townhallLvl++;
	
	ResetDisplay();
	_simulation->AddFireOnceParticleInfo(ParticleEnum::OnUpgrade, _area);
	
	// Reset all autoupgrade building's display
	for (int32 i = 0; i < BuildingEnumCount; i++) {
		CardEnum buildingEnum = static_cast<CardEnum>(i);
		if (IsAutoEraUpgrade(buildingEnum)) {
			const std::vector<int32>& buildingIds = _simulation->buildingIds(_townId, buildingEnum);
			for (int32 buildingId : buildingIds) {
				_simulation->building(buildingId).ResetDisplay();
			}
		}
	}

	// Biome Card
	if (townhallLvl == 2)
	{
		if (!alreadyGotBiomeCards2) {
			alreadyGotBiomeCards2 = true;

			switch (_simulation->GetBiomeEnum(centerTile()))
			{
			case BiomeEnum::BorealForest:
			case BiomeEnum::Tundra:
				_simulation->GenerateRareCardSelection(_playerId, RareHandEnum::BorealCards2, FText());
				break;
			case BiomeEnum::Desert:
				_simulation->GenerateRareCardSelection(_playerId, RareHandEnum::DesertCards2, FText());
				break;
			case BiomeEnum::Savanna:
			case BiomeEnum::GrassLand:
				_simulation->GenerateRareCardSelection(_playerId, RareHandEnum::SavannaCards2, FText());
				break;
			case BiomeEnum::Jungle:
				_simulation->GenerateRareCardSelection(_playerId, RareHandEnum::JungleCards2, FText());
				break;
			case BiomeEnum::Forest:
				_simulation->GenerateRareCardSelection(_playerId, RareHandEnum::ForestCards2, FText());
				break;
			default:
				UE_DEBUG_BREAK();
				break;
			}
		}
	}
	

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
	}
	else if (townhallLvl == 3) {
		cardSys.AddDrawCards(CardEnum::Immigration, 1);
		cardSys.AddDrawCards(CardEnum::Kidnap, 1);

		{
			_simulation->AddPopup(_playerId, {
				LOCTEXT("UnlockedSetDeliveryTarget1_Pop", "Unlocked ability to Set Delivery Target!<space>You can set the storage/market where the building's output will be stored.<space>"),
				LOCTEXT("UnlockedSetDeliveryTarget2_Pop", "To set the delivery target:<bullet>Click on a production building to bring up its panel</><bullet>Click the [Set Delivery Target] button</><bullet>Select the target you wish to deliver to</>")
				});
			unlockSys->unlockedSetDeliveryTarget = true;
		}
	}
	else if (townhallLvl == 4) {
		cardSys.AddDrawCards(CardEnum::SharingIsCaring, 1);

		{
			_simulation->AddPopup(_playerId,
				LOCTEXT("UnlockedSetTradeOffer_Pop", "Unlocked \"Set Trade Offer\" Button.<space>Use it to put up Trade Offers at the Townhall.\nOther players can examine your Trade Offers, and directly trade with you (0% Fee).")
			);
			unlockSys->unlockedSetTradeAmount = true;
		}

		//{
		//	_simulation->AddPopup(
		//		PopupInfo(_playerId, 
		//			FText::Format(LOCTEXT("BuyCardTownhallUpgrade_Pop", "Would you like to buy a {0} card for {1} <img id=\"Coin\"/>."),
		//				GetBuildingInfo(CardEnum::Warehouse).name,
		//				TEXT_NUM(_simulation->cardSystem(_playerId).GetCardPrice(CardEnum::Warehouse))
		//			), 
		//			{ LOCTEXT("Buy", "Buy"),
		//				LOCTEXT("Refuse", "Refuse") },
		//			PopupReceiverEnum::DoneResearchBuyCardEvent, false, "ResearchComplete", static_cast<int>(CardEnum::Warehouse)
		//		)
		//	);
		//}
	}

	_simulation->QuestUpdateStatus(_playerId, QuestEnum::TownhallUpgradeQuest);

	_simulation->RecalculateTaxDelayedTown(_townId);

	
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
	int32 happiness = _simulation->GetAverageHappiness(_townId);
	int32 population = _simulation->populationTown(_townId);
	int32 housingCapacity = _simulation->HousingCapacity(_townId);

	// Population Factor
	migrationPull_populationSize = 5 + _simulation->populationTown(_townId) / 18; // 5 + X% of population

	
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

	if (_simulation->townBuildingFinishedCount(_townId, CardEnum::ImmigrationPropagandaOffice)) {
		migrationPull_bonuses += migrationPullSoFar * 30 / 100; // 30% increase
	}
}

std::vector<FText> TownHall::getImmigrationEventChoices() {
	std::vector<FText> choices = {
		LOCTEXT("Accept", "Accept"),
		LOCTEXT("Refuse", "Refuse")
	};
	if (_simulation->TownhallCardCountTown(_townId, CardEnum::Cannibalism)) {
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
			FText::Format(LOCTEXT("ImmigrantsDesperateAskToJoin_Pop", "{0} desparate immigrants asked to join your colony. Low happiness lessen immigration."), 
				TEXT_NUM(askedMigration)
			)
		);
	}
	else if (migrationType == 2) {
		ImmigrationEvent(askedMigration,
			FText::Format(LOCTEXT("ImmigrantsSpaceAskToJoin_Pop", "{0} immigrants asked to join your colony. They heard that your town has plenty of available living space. Would you let them join your town?"), 
				TEXT_NUM(askedMigration)
			)
		);
	}
	else if (migrationType == 0) {
		ImmigrationEvent(askedMigration,
			FText::Format(LOCTEXT("ImmigrantsGreatRumorsAskToJoin_Pop", "{0} immigrants asked to join your colony. They came from faraway land, filled with hope, after hearing great rumors about your town. Would you let them join your town?"),
				TEXT_NUM(askedMigration)
			)
		);
	}
	else {
		UE_DEBUG_BREAK();
	}
}

void TownHall::AddInitialImmigrants()
{
	int32 beginAdultTick = _simulation->parameters(_playerId)->BeginBreedingAgeTicks();

#if TRAILER_MODE
	int32 adultCount = 3;
	int32 childrenCount = 2;
#else
	int32 adultCount = 14;
	int32 childrenCount = 4;
	if (!isCapital()) {
		const int32 targetImmigrantCount = 10;
		auto& townManager = _simulation->townManager(_playerId);
		adultCount = targetImmigrantCount * townManager.adultIds().size() / townManager.population();
		childrenCount = targetImmigrantCount - adultCount;

		const auto& adultIds = townManager.adultIds();
		const auto& childIds = townManager.childIds();

		adultCount = std::min(adultCount, static_cast<int32>(adultIds.size()));
		childrenCount = std::min(childrenCount, static_cast<int32>(childIds.size()));

		// Remove citizens from the old town
		for (int32 i = 0; i < adultCount; i++) {
			_simulation->unitAI(adultIds[i]).Die();
		}
		for (int32 i = 0; i < childrenCount; i++) {
			_simulation->unitAI(childIds[i]).Die();
		}
	}
#endif

	auto getRandomTile = [&]() -> WorldTile2 {
		if (GameRand::Rand() % 2 == 0) {
			return WorldTile2(GameRand::Rand() % 2 == 0 ? _area.minX - 1 : _area.maxX + 1, (GameRand::Rand() % _area.sizeY()) + _area.minY);
		}
		else {
			return WorldTile2((GameRand::Rand() % _area.sizeX()) + _area.minX, GameRand::Rand() % 2 == 0 ? _area.minY - 1 : _area.maxY + 1);
		}
	};

	for (int i = 0; i < adultCount; i++) {
		int32 ageTicks = beginAdultTick + i * Time::TicksPerYear / 3;
		_simulation->AddUnit(UnitEnum::Human, _townId, getRandomTile().worldAtom2(), ageTicks);
	}

	// 2 years max for children
	for (int i = 0; i < childrenCount; i++) {
		int32 ageTicks = i * beginAdultTick / childrenCount;
		_simulation->AddUnit(UnitEnum::Human, _townId, getRandomTile().worldAtom2(), ageTicks);
	}
}


#undef LOCTEXT_NAMESPACE