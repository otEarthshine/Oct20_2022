#pragma once

#include "CoreMinimal.h"
#include "../Building.h"
#include "PunCity/CppUtils.h"


/**
 * 
 */
class House : public Building
{
public:
	void FinishConstruction() override;
	void OnDeinit() override;

	int32 houseLvl() { return _houseLvl; }

	int32 trailerTargetHouseLvl = 0;

	bool TrailerCheckHouseLvl()
	{
		if (isConstructed() &&
			trailerTargetHouseLvl > _houseLvl)
		{
			if (_houseLvl < GetMaxHouseLvl()) {
				_houseLvl++;
			}

			_allowedOccupants = houseBaseOccupants + (_houseLvl - 1) / 2;
			_simulation->RecalculateTaxDelayedTown(_townId); // Recalculate sci
			ResetDisplay();

			//PUN_LOG("TrailerHouseUpgrade houseLvl:%d trailerTargetHouseLvl:%d", _houseLvl, trailerTargetHouseLvl);
			return true;
		}
		return false;
	}
	
	void CheckHouseLvl()
	{
		// Trailer mode house upgrade 1 level at a time using TrailerCheckHouseLvl()
		if (PunSettings::TrailerMode()) {
			return;
		}
		
		
		int32 lvl = CalculateHouseLevel();
		
		// Note that houseDowngrade happens right away, but houseUpgrade is fake delayed by 5s
		if (lvl > _houseLvl) 
		{
			if (_lastHouseUpgradeTick == -1) {
				_lastHouseUpgradeTick = Time::Ticks();
				// Delayed upgrade
#if TRAILER_MODE
				_simulation->ScheduleTickBuilding(buildingId(), _lastHouseUpgradeTick + 1);
#else
				_simulation->ScheduleTickBuilding(buildingId(), _lastHouseUpgradeTick + houseUpgradeDelayTicks);
#endif
			}
		}
		else if (lvl < _houseLvl) 
		{
			_simulation->uiInterface()->ShowFloatupInfo(FloatupEnum::HouseDowngrade, _centerTile, FText());
			_houseLvl = lvl;
			//_simulation->QuestUpdateStatus(_playerId, QuestEnum::HouseUpgradeQuest);

			_allowedOccupants = houseBaseOccupants + (lvl - 1) / 2;
			_simulation->RecalculateTaxDelayedTown(_townId); // Recalculate sci
			ResetDisplay();
		}
	}
	void UpgradeHouse(int32 lvl);

	void ForceSetHouseLevel(int32 lvl)
	{
		if (lvl > 1) {
			switch (lvl)
			{
			case 7:
				AddResource(ResourceEnum::Jewelry, 10);
				AddResource(ResourceEnum::Chocolate, 10);
			case 6: AddResource(ResourceEnum::Book, 10);
				
			case 5:
				AddResource(ResourceEnum::Vodka, 10);
				AddResource(ResourceEnum::Candle, 10);
			case 4: AddResource(ResourceEnum::Wine, 10);
				
			case 3:
				AddResource(ResourceEnum::Tulip, 10);
				AddResource(ResourceEnum::Beer, 10);
			case 2: AddResource(ResourceEnum::Pottery, 10);
				break;
			default:
				break;
			}
			CheckHouseLvl();
		}
	}

	
	
	static int32 GetMaxHouseLvl();

	int32 GetAppealPercent();
	
	int32 housingHappiness();
	int32 luxuryHappiness() { return luxuryCount() * 5; }


	int32 luxuryCount();
	void CalculateConsumptions(bool consumeLuxury = false);

	int32 occupancyFactor(int32 value) {
		return value * occupantCount() / houseBaseOccupants; // Higher house lvl with more occupancy
	}

	// Income
	//  Note: tax recalculation refreshed through RecalculateTaxDelayed()
	int32 GetIncome100Int(int32 incomeEnumInt) { return GetIncome100(static_cast<IncomeEnum>(incomeEnumInt)); }
	int32 GetIncome100(IncomeEnum incomeEnum);

	int32 GetInfluenceIncome100() { return _roundLuxuryConsumption100 * 2 / 10; } // 20% of Luxury goes to influence

	// Note: these two includes occupancy factor built in...
	int32 _roundLuxuryConsumption100 = 0;
	int32 _roundFoodConsumption100 = 0;


	int32 totalHouseIncome100() {
		int32 result = 0;
		for (int32 i = 0; i < HouseIncomeEnumCount; i++) {
			result += GetIncome100Int(i);
		}
		return result;
	}

	

	bool HasAdjacencyBonus() final {
		return _simulation->IsResearched(_playerId, TechEnum::HouseAdjacency);
	}

	// Science
	int32 GetScience100(ScienceEnum scienceEnum);
	
	int32 science100PerRound()
	{
		int32 result = 0;
		for (int32 i = 0; i < HouseScienceEnums.size(); i++) {
			result += GetScience100(HouseScienceEnums[i]);
		}
		return result;
	}

	void OnPickupResource(int32 objectId) override;
	void OnDropoffResource(int32 objectId, ResourceHolderInfo holderInfo, int32 amount) override;

	static const int32 houseTypesPerLevel = 3;
	
	int32 displayVariationIndex() override
	{
		if (_simulation->GetBiomeEnum(centerTile()) == BiomeEnum::Desert &&
			_houseLvl <= 3) 
		{
			return (_houseLvl - 1) * houseTypesPerLevel + 2;
		}
		// Checker board like pattern so houses doesn't look the same next to each other...
		int32 localIndex = (((centerTile().x / 6) % 2) + ((centerTile().y / 6) % 2) + 1) % 2;
		return (_houseLvl - 1) * houseTypesPerLevel + localIndex;
	}

	int32 GetBuildingSelectorHeight() override
	{
		switch(_houseLvl) {
		case 3:
		case 4:
		case 5:
			return 45;
		case 6:
			return 50;
		case 7:
			return 60;
		default:
			return 35;
		}
	}
	

	static int32 GetLuxuryCountUpgradeRequirement(int32 houseLvl) {
		return houseLvl - 1; // houseLvl 2 requires 1 lux
	}
	static int32 GetAppealUpgradeRequirement(int32 houseLvl) {
		return 20 + houseLvl * 10;
	}



	void GetHeatingEfficiencyTip(TArray<FText>& args, ResourceEnum resourceEnum);

	int32 GetHeatingEfficiency(ResourceEnum resourceEnum)
	{
		int32 heatEfficiency = 100;
		if (_simulation->TownhallCardCountTown(_townId, CardEnum::ChimneyRestrictor)) {
			heatEfficiency += 15;
		}

		if (IsUpgraded(0)) {
			heatEfficiency +=  20;
		}
		if (IsUpgraded(1)) {
			heatEfficiency += 30;
		}

		
		if (resourceEnum == ResourceEnum::Coal) 
		{
			if (_simulation->TownhallCardCountTown(_townId, CardEnum::CoalTreatment)) {
				heatEfficiency += 20;
			}
			heatEfficiency *= 2;
		}

		return heatEfficiency;
	}

//private:
//	int32_t GetRadiusBonus(BuildingEnum buildingEnum, int32_t radius, const int32_t bonusByLvl[]);

	void Serialize(FArchive& Ar) override {
		Building::Serialize(Ar);
		Ar << _houseLvl;
		Ar << _lastHouseUpgradeTick;
	}

	void ScheduleTick() override {
		_lastHouseUpgradeTick = -1;
		UpgradeHouse(CalculateHouseLevel());
	}

	bool isUpgrading() {
		return _lastHouseUpgradeTick != -1;
	}
	int32 upgradeProgressPercent() {
		return (Time::Ticks() - _lastHouseUpgradeTick) * 100 / houseUpgradeDelayTicks;
	}

	int32 maxCardSlots() override { return 0; }

	/*
	 * House Upgrade
	 */
	FText HouseNeedDescription();
	FText HouseNeedTooltipDescription();
private:
	int32 CalculateHouseLevel();

	int32 LuxuryTypeCount(int32 luxuryTier)
	{
		int32 typeCount = 0;
		for (ResourceEnum resourceEnum : TierToLuxuryEnums[luxuryTier]) {
			if (resourceCount(resourceEnum) > 0) {
				typeCount++;
			}
		}
		return typeCount;
	}
	
	//void UpdateSubscription();

private:
	int32 _houseLvl = 1;
	int32 _lastHouseUpgradeTick = -1;
	const int32 houseUpgradeDelayTicks = Time::TicksPerSecond * 8;

	const int32 houseBaseOccupants = 4;
	const int32 houseMaxOccupants = 8;
};

class StoneHouse : public House
{
};

class BoarBurrow : public Building
{
public:
	void FinishConstruction() override;
	void OnDeinit() override;

	void Serialize(FArchive& Ar) override {
		Building::Serialize(Ar);
		inventory >> Ar;
	}

	UnitInventory inventory;
};

class RanchBarn final : public BoarBurrow
{
//public:
//	void FinishConstruction() override;
//	void OnDeinit() override;
//
//	void AddAnimalOccupant(UnitEnum animalEnum, int32_t age);
//	void RemoveAnimalOccupant(int32_t animalId);
//
//	ResourceEnum product() override { return ResourceEnum::Pork; }
//
//	void OnTick1Sec() override;
//
//	int32 openAnimalSlots() { return maxAnimals - _animalOccupants.size(); }
//	const std::vector<int32>& animalOccupants() { return _animalOccupants; }
//	
//	UnitEnum animalEnum() { return _animalEnum; }
//	void SetAnimalEnum(UnitEnum animalEnum) { _animalEnum = animalEnum; }
//
//	void Serialize(FArchive& Ar) override {
//		Building::Serialize(Ar);
//		Ar << _animalEnum;
//		SerializeVecValue(Ar, _animalOccupants);
//	}
//
//public:
//	//10 animals ... 5 slaugther per season.. 100 per season... (half year adult growth... )
//	const int32_t maxAnimals = 15;
//
//private:
//	UnitEnum _animalEnum = UnitEnum::Pig;
//	std::vector<int32> _animalOccupants;
};

const FText RanchWorkMode_FullCapacity = NSLOCTEXT("Ranch", "Kill when reached full capacity", "Kill when reached full capacity");
const FText RanchWorkMode_HalfCapacity = NSLOCTEXT("Ranch", "Kill when above half capacity", "Kill when above half capacity");
const FText RanchWorkMode_KillAll = NSLOCTEXT("Ranch", "Kill all", "Kill all");

class Ranch final : public Building
{
public:
	void FinishConstruction() override;
	
	void OnDeinit() override;

	void SetAreaWalkable() override
	{	
		// Fence
		for (int32 x = _area.minX; x <= _area.maxX; x++) {
			_simulation->SetWalkable(WorldTile2(x, _area.minY), false);
			_simulation->SetWalkable(WorldTile2(x, _area.maxY), false);
		}
		for (int32 y = _area.minY; y <= _area.maxY; y++) {
			_simulation->SetWalkable(WorldTile2(_area.minX, y), false);
			_simulation->SetWalkable(WorldTile2(_area.maxX, y), false);
		}

		// Barn area...s
		// From y: 3 to 6
		// From x: -4 to -7
		for (int32 y = 3; y <= 6; y++) {
			for (int32 x = -7; x <= -4; x++) {
				bool isWalkable = (y == 5); // Hole through middle of barn so people can get in...
				SetLocalWalkable_WithDirection(WorldTile2(x, y), isWalkable);
			}
		}

		// Prevent non-human units from going into barn
		_simulation->SetWalkableNonIntelligent(gateTile(), false);

		_simulation->ResetUnitActionsInArea(_area);

		_didSetWalkable = true;
	}
	WorldTile2 gateTile() override {
		WorldTile2 rotatedTile = WorldTile2::RotateTileVector(WorldTile2(-7, 5), _faceDirection);
		return rotatedTile + _centerTile;
	}
	
	void AddAnimalOccupant(UnitEnum animalEnum, int32 age);
	
	void RemoveAnimalOccupant(int32_t animalId) {
		CppUtils::Remove(_animalOccupants, animalId);
	}

	int32 openAnimalSlots() { return maxAnimals - _animalOccupants.size(); }
	const std::vector<int32>& animalOccupants() { return _animalOccupants; }

	void Serialize(FArchive& Ar) override {
		Building::Serialize(Ar);
		SerializeVecValue(Ar, _animalOccupants);
	}

	void TickRound() override {
		// Give 20 milk per round at full occupancy
		if (isEnum(CardEnum::RanchCow)) {
			int32 milkPerRound = 20 * _animalOccupants.size() / maxAnimals;
			if (milkPerRound > 0) {
				AddResource(ResourceEnum::Milk, milkPerRound);
				AddProductionStat(ResourcePair(ResourceEnum::Milk, milkPerRound));
			}
		}
	}

public:
	//10 animals ... 5 slaugther per season.. 100 per season... (half year adult growth... )
	const int32 maxAnimals = 15;

private:
	std::vector<int32> _animalOccupants;
};