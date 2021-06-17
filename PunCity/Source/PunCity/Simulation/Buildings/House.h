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

			_simulation->ResetTechTreeDisplay(_playerId);
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
	
	int32 housingQuality();
	int32 luxuryHappiness() {
		int32 happiness = 50 + luxuryCount() * 5;
		if (_simulation->TownhallCardCountTown(_townId, CardEnum::BlingBling) > 0 &&
			resourceCount(ResourceEnum::Jewelry)) 
		{
			happiness += 20;
		}
		
		return happiness;
	} // ~16 types


	int32 luxuryCount();
	void CalculateConsumptions(bool consumeLuxury = false);

	int32 occupancyFactor(int32 value) const {
		return value * occupantCount() / houseBaseOccupants; // Higher house lvl with more occupancy
	}

	// Income
	//  Note: tax recalculation refreshed through RecalculateTaxDelayed()
	int32 GetIncome100Int(int32 incomeEnumInt) { return GetIncome100(static_cast<IncomeEnum>(incomeEnumInt)); }
	int32 GetIncome100(IncomeEnum incomeEnum);

	int32 GetInfluenceIncome100() { return _roundLuxuryConsumption100 * 2 / 10;} // 20% of Luxury goes to influence
	
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
	int64 GetScience100(ScienceEnum scienceEnum, int64 cumulative100) const;
	
	int64 science100PerRound()
	{
		int64 result = 0;
		for (int32 i = 0; i < HouseScienceEnums.size(); i++) {
			result += GetScience100(HouseScienceEnums[i], 0);
		}

		int64 cumulative100 = result;
		for (int32 i = 0; i < HouseScienceModifierEnums.size(); i++) {
			result += GetScience100(HouseScienceModifierEnums[i], cumulative100);
		}
		
		return result;
	}

	void OnPickupResource(int32 objectId) override;
	void OnDropoffResource(int32 objectId, ResourceHolderInfo holderInfo, int32 amount) override;

	static const int32 houseTypesPerLevel = 5;
	
	int32 displayVariationIndex() override
	{
		if (_simulation->GetBiomeEnum(centerTile()) == BiomeEnum::Desert &&
			(2 <= _houseLvl && _houseLvl <= 4))
		{
			const int32 desertShift = 4;
			return (_houseLvl - 1) * houseTypesPerLevel + desertShift;
		}
		int32 maxLocalIndex = 2;
		switch(_houseLvl) {
		case 1: maxLocalIndex = 3; break;
		case 2: maxLocalIndex = 3; break;
		case 3: maxLocalIndex = 4; break;
		case 4: maxLocalIndex = 4; break;
		default: break;
		}
		
		// Checker board like pattern so houses doesn't look the same next to each other...
		int32 localIndex = (((centerTile().x / 6) % 2) + ((centerTile().y / 6) % 2) + 1) % maxLocalIndex;
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
		if (_simulation->HasTownBonus(_townId, CardEnum::BorealWinterResist)) {
			heatEfficiency += 30;
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
		//for (int32 x = _area.minX; x <= _area.maxX; x++) {
		//	_simulation->SetWalkable(WorldTile2(x, _area.minY), false);
		//	_simulation->SetWalkable(WorldTile2(x, _area.maxY), false);
		//}
		//for (int32 y = _area.minY; y <= _area.maxY; y++) {
		//	_simulation->SetWalkable(WorldTile2(_area.minX, y), false);
		//	_simulation->SetWalkable(WorldTile2(_area.maxX, y), false);
		//}

		// y is the front(shorter) direction
		static const std::vector<int32> buildingMark
		{
			// y -->
			// A
			// |
			// x
			1, 1, 1, 1, 1, 1,	1, 1, 1, 1, 1, 1,
			1, 0, 0, 1, 1, 1,	1, 1, 1, 1, 1, 1,
			1, 0, 0, 1, 1, 1,	1, 1, 1, 1, 1, 1,
			1, 0, 0, 1, 1, 1,	1, 1, 1, 1, 1, 1,

			1, 0, 0, 0, 0, 0,	0, 0, 0, 1, 1, 1,
			1, 0, 0, 0, 0, 0,	0, 0, 0, 1, 1, 1,
			1, 0, 0, 0, 0, 0,	0, 0, 0, 1, 1, 1,

			1, 0, 0, 0, 1, 0,	0, 0, 0, 1, 1, 1,
			1, 0, 0, 0, 1, 1,	1, 1, 1, 1, 1, 1,

			1, 0, 0, 0, 0, 0,	0, 0, 0, 0, 0, 1,
			1, 0, 0, 0, 0, 0,	0, 0, 0, 0, 0, 1,
			1, 0, 0, 0, 0, 0,	0, 0, 0, 0, 0, 1,
			1, 0, 0, 0, 0, 0,	0, 0, 0, 0, 0, 1,
			1, 0, 0, 0, 0, 0,	0, 0, 0, 0, 0, 1,
			
			1, 0, 0, 0, 0, 0,	0, 0, 0, 0, 0, 1,
			1, 0, 0, 0, 0, 0,	0, 0, 0, 0, 0, 1,
			1, 0, 0, 0, 0, 0,	0, 0, 0, 0, 0, 1,
			1, 1, 1, 1, 1, 1,	1, 0, 0, 0, 1, 1,
		};
		int32 centerYShift = 5;
		int32 centerXShift = -8;

		WorldTile2 size = buildingInfo().size;
		check(size.x * size.y == buildingMark.size());
		for (int32 xIndex = 0; xIndex < size.x; xIndex++) {
			for (int32 yIndex = 0; yIndex < size.y; yIndex++) {
				int32 xIndexInv = size.x - 1 - xIndex;
				SetLocalWalkable_WithDirection(
					WorldTile2(xIndex + centerXShift, yIndex - centerYShift), buildingMark[yIndex + xIndexInv * size.y] == 0
				);
			}
		}

		// Barn area...s
		// From y: 3 to 6
		// From x: -4 to -7
		//for (int32 y = 3; y <= 6; y++) {
		//	for (int32 x = -7; x <= -4; x++) {
		//		bool isWalkable = (y == 5); // Hole through middle of barn so people can get in...
		//		SetLocalWalkable_WithDirection(WorldTile2(x, y), isWalkable);
		//	}
		//}

		// Prevent non-human units from going into barn
		//_simulation->SetWalkableNonIntelligent(gateTile(), false);

		_simulation->ResetUnitActionsInArea(_area);

		_didSetWalkable = true;
	}
	WorldTile2 gateTile() override {
		WorldTile2 rotatedTile = WorldTile2::RotateTileVector(WorldTile2(-7, 5), _faceDirection);
		return rotatedTile + _centerTile;
	}

	std::vector<BonusPair> GetBonuses() override {
		std::vector<BonusPair> bonuses = Building::GetBonuses();
		if (_simulation->HasTownBonus(_townId, CardEnum::SavannaRanch)) {
			bonuses.push_back({ NSLOCTEXT("Ranch", "Grass Fed Bonus", "Grass Fed"), 35 });
		}
		if (_simulation->HasTownBonus(_townId, CardEnum::SavannaGrasslandHerder)) {

			if (IsGrassDominant(centerBiomeEnum())) {
				bonuses.push_back({ NSLOCTEXT("Ranch", "Grassland Herder Bonus", "Grassland Herder"), 50 });
			}
		}
		return bonuses;
	}


	/*
	 * Occupants
	 */
	
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

	UnitEnum GetAnimalEnum()
	{
		switch (buildingEnum()) {
		case CardEnum::RanchPig: return UnitEnum::Pig;
		case CardEnum::RanchSheep:return UnitEnum::Sheep;
		case CardEnum::RanchCow:return UnitEnum::Cow;
		default:
			UE_DEBUG_BREAK();
			return UnitEnum::Pig;
		}
	}

	int32 animalCost() {
		return buildingInfo().constructionCostAsMoney() / 3;
	}
	
public:
	//10 animals ... 5 slaugther per season.. 100 per season... (half year adult growth... )
	const int32 maxAnimals = 15;

private:
	std::vector<int32> _animalOccupants;
};