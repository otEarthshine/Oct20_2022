#pragma once

#include "CoreMinimal.h"
#include "../Building.h"
#include "TradeBuilding.h"
#include "PunCity/Simulation/ArmyNode.h"

//#include "Garrisons.h"

static const std::vector<std::string> TownhallLvlToUpgradeBonusText =
{
	"",
	"",

	"Unlocked Cards:"
	"<bullet>Wheat seeds</>"
	"<bullet>Cabbage seeds</>"
	"<bullet>Snatch</>"
	"<bullet>Buy Wood</>"
	"<bullet>Sell Food</>", // Lvl 2

	"<bullet>Unlocked Influence Points used to claim land.</>"
	"<bullet>+10% mine/quarry production.</>"
	"<space>"
	"Unlocked Cards:"
	"<bullet>Fort</>"
	"<bullet>Inventor's Workshop</>"
	"<bullet>Archer Barrack</>"
	"<bullet>Immigration Advertisement</>"
	"<bullet>Kidnap</>"
	"<bullet>Stone Road</>"
	"<bullet>Intercity Road</>", // 3

	"<space>"
	"Unlocked Cards:"
	"<bullet>Warehouse</>"
	"<bullet>Colony</>"
	"<bullet>Swordman Barrack</>"
	"<bullet>Sharing is caring</>",// 4

	"<bullet>+10% industrial production.</>", // Lvl 5
};
static const std::string& GetTownhallLvlToUpgradeBonusText(int32 townhallLvl) {
	return TownhallLvlToUpgradeBonusText[townhallLvl];
}

class ArmyNodeBuilding : public Building
{
public:
	virtual ArmyNode& GetArmyNode() = 0;
};

class TownHall : public ArmyNodeBuilding
{
public:
	void FinishConstruction() override;

	int32 displayVariationIndex() override {
		if (!isConstructed()) {
			return 1;
		}
		return townhallLvl - 1;
	}

	int32 townhallIncome()
	{
		switch(townhallLvl)
		{
		case 1: return 10;
		case 2: return 20;
		case 3: return 30;
		case 4: return 50;
		case 5: return 80;
		default:
			UE_DEBUG_BREAK();
			return 0;
		}
	}

	FString townFName() { return _townName; }
	std::string townName() { return ToStdString(_townName); }

	void SetTownName(FString _townNameIn) {
		_townName = _townNameIn;
	}

	bool HasEnoughUpgradeMoney();
	void UpgradeTownhall();

	int32 GetUpgradeStones() {
		int32 upgradeStones = GetUpgradeMoney(wallLvl + 1) / GetResourceInfo(ResourceEnum::Stone).basePrice;
		upgradeStones = (upgradeStones / 10) * 10; // round to nearest 10
		return upgradeStones;
	}
	bool HasEnoughStoneToUpgradeWall() 	{
		return resourceSystem().resourceCount(ResourceEnum::Stone) >= GetUpgradeStones();
	}
	void UpgradeWall() {
		resourceSystem().RemoveResourceGlobal(ResourceEnum::Stone, GetUpgradeStones());
		wallLvl++;
		_simulation->AddPopup(_playerId, townName() + "'s wall has been upgraded to level " + std::to_string(wallLvl) + ".");
	}

	//std::string UpgradeButtonString();
	//std::string UpgradeButtonTooltip();

	void ImmigrationEvent(int32 exactAmount = -1);
	void ImmigrationEvent(int32 exactAmount, std::string message, PopupReceiverEnum replyReceiver = PopupReceiverEnum::ImmigrationEvent);
	//void AnimalVendorEvent();

	void AddImmigrants(int32 immigrantCount, WorldTile2 tile = WorldTile2::Invalid)
	{
		if (tile == WorldTile2::Invalid) {
			tile = gateTile();
		}

		for (int i = 0; i < immigrantCount; i++) {
			int32 ageTicks = GameRand::Rand() % _simulation->parameters(_playerId)->DeathAgeTicks();
			_simulation->AddUnit(UnitEnum::Human, _playerId, tile.worldAtom2(), ageTicks);
		}
	}
	void AddInitialImmigrants()
	{
		int32 beginAdultTick = _simulation->parameters(_playerId)->BeginBreedingAgeTicks();

#if TRAILER_MODE
		int32 adultCount = 3;
		int32 childrenCount = 2;
#else
		int32 adultCount = 14;
		int32 childrenCount = 4;
#endif

		auto getRandomTile = [&]() -> WorldTile2 {
			if (GameRand::Rand() % 2 == 0) {
				return WorldTile2(GameRand::Rand() % 2 == 0 ? _area.minX - 1 : _area.maxX + 1, (GameRand::Rand() % _area.sizeY()) + _area.minY);
			} else {
				return WorldTile2((GameRand::Rand() % _area.sizeX()) + _area.minX, GameRand::Rand() % 2 == 0 ? _area.minY - 1 : _area.maxY + 1);
			}
		};
		
		for (int i = 0; i < adultCount; i++) {
			int32 ageTicks = beginAdultTick + i * Time::TicksPerYear / 3;
			_simulation->AddUnit(UnitEnum::Human, _playerId, getRandomTile().worldAtom2(), ageTicks);
		}

		// 2 years max for children
		for (int i = 0; i < childrenCount; i++) {
			int32 ageTicks = i * beginAdultTick / childrenCount;
			_simulation->AddUnit(UnitEnum::Human, _playerId, getRandomTile().worldAtom2(), ageTicks);
		}
	}

	void AddRequestedImmigrants() {
		AddImmigrants(askedMigration);
	}
	//void SeedChoicesReply(int32 choiceIndex);
	//void AnimalChoicesReply(int32 choiceIndex);

	static int32 GetMaxUpgradeLvl();
	//std::string GetUpgradeRequirementText();
	//static const std::vector<int32_t>& GetUpgradeResources(int32_t lvl);

	int32 GetUpgradeMoney() {
		return GetUpgradeMoney(townhallLvl + 1);
	}
	static int32 GetUpgradeMoney(int32 lvl);

	ArmyNode& GetArmyNode() final { return armyNode; }

	void Serialize(FArchive& Ar) override
	{
		Building::Serialize(Ar);
		Ar << migrationPull_populationSize;
		Ar << migrationPull_freeLivingSpace;
		Ar << migrationPull_happiness;
		Ar << migrationPull_bonuses;
		
		Ar << migrationType;
		
		Ar << askedMigration;
		Ar << townhallLvl;
		Ar << wallLvl;
		Ar << _townName;

		Ar << _townStartTick;
		Ar << alreadyGotInitialCard;

		SerializeVecLoop(Ar, animalChoicesForBuy, [&](std::pair<UnitEnum, int32_t>& pair) {
			Ar << pair.first;
			Ar << pair.second;
		});

		armyNode >> Ar;
		if (Ar.IsLoading()) {
			armyNode.InitAfterLoad(_simulation);
		}
	}

	
	void Tick1Sec() override;

	int32 maxTradeQuatity() override { return 80; }
	
public:
	// Migration
	int32 migrationPull_populationSize = 0;
	int32 migrationPull_freeLivingSpace = 0;
	int32 migrationPull_happiness = 0;
	int32 migrationPull_bonuses = 0;
	
	int32 migrationPull()
	{
		int32 migration = migrationPull_populationSize +
							migrationPull_freeLivingSpace +
							migrationPull_happiness +
							migrationPull_bonuses;
		migration = std::max(0, migration);
		return migration;
	}
	
	int32 migrationType = 0;
	
	int32 askedMigration = 0;

	ArmyNode armyNode;
	
	// Town lvl
	int32 townhallLvl = 0;
	int32 wallLvl = 1;


	int32 townAgeTicks() {
		return Time::Ticks() - _townStartTick;
	}
	int32 alreadyGotInitialCard = false;
	
private:
	FString _townName;

	int32 _townStartTick = -1;
	

	std::vector<std::pair<UnitEnum, int32_t>> animalChoicesForBuy;
};
