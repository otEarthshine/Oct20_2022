#pragma once

#include "CoreMinimal.h"
#include "../Building.h"
#include "TradeBuilding.h"
#include "PunCity/Simulation/ArmyNode.h"

const FText& GetTownhallLvlToUpgradeBonusText(int32 townhallLvl);


class TownHall : public Building
{
public:
	virtual void FinishConstruction() override;

	virtual int32 displayVariationIndex() override {
		if (!isConstructed()) {
			return 1;
		}
		return townhallLvl - 1;
	}

	//int32 townhallIncome()
	//{
	//	switch(townhallLvl)
	//	{
	//	case 1: return 10;
	//	case 2: return 20;
	//	case 3: return 30;
	//	case 4: return 50;
	//	case 5: return 80;
	//	default:
	//		UE_DEBUG_BREAK();
	//		return 0;
	//	}
	//}

	FString townNameF() { return _townName; }
	FText townNameT() { return FText::FromString(_townName); }

	bool isCapital() { return _simulation->townManager(_townId).isCapital(); }

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
	void UpgradeWall();

	//std::string UpgradeButtonString();
	//std::string UpgradeButtonTooltip();

	std::vector<FText> getImmigrationEventChoices();
	void ImmigrationEvent(int32 exactAmount = -1);
	void ImmigrationEvent(int32 exactAmount, FText message, PopupReceiverEnum replyReceiver = PopupReceiverEnum::ImmigrationEvent);
	//void AnimalVendorEvent();

	void AddImmigrants(int32 immigrantCount, WorldTile2 tile = WorldTile2::Invalid)
	{
		if (tile == WorldTile2::Invalid) {
			tile = gateTile();
		}

		for (int i = 0; i < immigrantCount; i++) {
			int32 ageTicks = GameRand::Rand() % _simulation->parameters(_playerId)->DeathAgeTicks();
			_simulation->AddUnit(UnitEnum::Human, _townId, tile.worldAtom2(), ageTicks);
		}
	}
	void AddInitialImmigrants();

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

	//ArmyNode& GetArmyNode() final { return armyNode; }

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
		Ar << alreadyGotBiomeCards1;
		Ar << alreadyGotBiomeCards2;

		SerializeVecLoop(Ar, animalChoicesForBuy, [&](std::pair<UnitEnum, int32_t>& pair) {
			Ar << pair.first;
			Ar << pair.second;
		});

		//armyNode >> Ar;
		//if (Ar.IsLoading()) {
		//	armyNode.InitAfterLoad(_simulation);
		//}
	}

	
	void OnTick1Sec() override;

	int32 maxTradeQuatity() override { return 80; }

	bool shouldDisplayParticles() override { return true; }
	
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

	//ArmyNode armyNode;
	
	// Town lvl
	int32 townhallLvl = 0;
	int32 wallLvl = 1;


	int32 townAgeTicks() {
		return Time::Ticks() - _townStartTick;
	}
	int32 alreadyGotInitialCard = false;
	bool alreadyGotBiomeCards1 = false;
	bool alreadyGotBiomeCards2 = false;
	
private:
	FString _townName = FString("None");

	int32 _townStartTick = -1;
	

	std::vector<std::pair<UnitEnum, int32_t>> animalChoicesForBuy;
};
