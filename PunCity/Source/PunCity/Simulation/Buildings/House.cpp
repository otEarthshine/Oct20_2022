// Fill out your copyright notice in the Description page of Project Settings.

#include "House.h"
#include "../GameEventSystem.h"
#include "../OverlaySystem.h"
#include "../Resource/ResourceSystem.h"
#include "../StatSystem.h"
#include "../PlayerParameters.h"
#include "../UnitStateAI.h"
#include "../UnlockSystem.h"
#include "PunCity/CppUtils.h"
#include "PunCity/DisplaySystem/PunTerrainGenerator.h"
#include "GathererHut.h"

#include <functional>
#include <algorithm>

using namespace std;
using namespace std::placeholders;

#define LOCTEXT_NAMESPACE "House"


//int32_t House::GetRadiusBonus(BuildingEnum buildingEnum, int32_t radius, const int32_t bonusByLvl[])
//{
//	// If within radius of a library, get the highest possible bonus;
//	const std::vector<int32_t>& buildingIds = _simulation->buildingIds(_playerId, buildingEnum);
//	int32_t bonus = 0;
//	for (int32_t buildingId : buildingIds) {
//		Building& building = _simulation->building(buildingId);
//		if (building.isConstructed() && building.DistanceTo(_centerTile) <= radius) {
//			bonus = max(bonus, bonusByLvl[building.level()]);
//		}
//	}
//	return bonus;
//}

void House::GetHeatingEfficiencyTip(TArray<FText>& args, ResourceEnum resourceEnum)
{
	ADDTEXT_(LOCTEXT("HeatingEfficiency", "{0} Heating Efficiency: {1}%"),
		ResourceNameT(resourceEnum),
		FText::AsNumber(GetHeatingEfficiency(resourceEnum))
	);
	ADDTEXT_INV_("<space>");

	if (_simulation->TownhallCardCount(_playerId, CardEnum::ChimneyRestrictor)) {
		ADDTEXT_LOCTEXT("ChimneyRestrictor", " +15% Chimney Restrictor\n");
	}
	if (IsUpgraded(0)) {
		ADDTEXT_LOCTEXT("StoneInsulation", " +20% Stone Insulation\n");
	}
	if (IsUpgraded(1)) {
		ADDTEXT_LOCTEXT("BrickInsulation", " +30% Brick Insulation\n");
	}

	if (resourceEnum == ResourceEnum::Coal) {
		if (_simulation->TownhallCardCount(_playerId, CardEnum::CoalTreatment)) {
			ADDTEXT_LOCTEXT("CoalTreatment", " +20% Coal Treatment\n");
		}
		ADDTEXT_LOCTEXT("CoalUsage", " x2 Coal Usage\n");
	}

	//ss << ResourceName(resourceEnum) << " Heating Efficiency: " << GetHeatingEfficiency(resourceEnum) << "%<space>";
	//if (_simulation->TownhallCardCount(_playerId, CardEnum::ChimneyRestrictor)) {
	//	ss << " +15% Chimney Restrictor\n";
	//}
	//if (IsUpgraded(0)) {
	//	ss << " +20% Stone Insulation\n";
	//}
	//if (IsUpgraded(1)) {
	//	ss << " +30% Brick Insulation\n";
	//}

	//if (resourceEnum == ResourceEnum::Coal) {
	//	if (_simulation->TownhallCardCount(_playerId, CardEnum::CoalTreatment)) {
	//		ss << " +20% Coal Treatment\n";
	//	}
	//	ss << " x2 Coal Usage\n";
	//}
}


void House::OnDeinit()
{
	if (isConstructed() && !isBurnedRuin()) {

		//for (int i = _occupantIds.size(); i-- > 0;) {
		//	_simulation->ResetUnitActions(_occupantIds[i]);
		//}
		//_simulation->RemoveTenantFrom(_objectId);
		ResetOccupants(); // Reset occupant's actions that may tie with the house
		_simulation->RemoveTenantFrom(_objectId); // Remove occupants so that new action associated with this house won't be queued
		_simulation->PlayerRemoveHouse(playerId(), _objectId); // Remove from house list so that new tenants won't be assigned to this house
		
		//UpdateSubscription();

		_simulation->RecalculateTaxDelayed(_playerId); // Recalculate sci
	}
}

void House::FinishConstruction()
{
	Building::FinishConstruction();

	_allowedOccupants = houseBaseOccupants;
	_maxOccupants = houseMaxOccupants;

	_simulation->PlayerAddHouse(_playerId, _objectId);

	auto& playerOwned = _simulation->playerOwned(_playerId);
	auto addHolder = [&](ResourceEnum resourceEnum)
	{
		bool allowed = playerOwned.GetHouseResourceAllow(resourceEnum);
		ResourceHolderType type = allowed ? ResourceHolderType::Manual : ResourceHolderType::Provider;
		int32 target = allowed ? 10 : 0;
		AddResourceHolder(resourceEnum, type, target);
	};

	addHolder(ResourceEnum::Wood);
	addHolder(ResourceEnum::Coal);

	ExecuteOnLuxuryResources([&](ResourceEnum resourceEnum) {
		addHolder(resourceEnum);
	});

	if (PunSettings::TrailerMode()) {
		// Add more population in trailer mode
		_simulation->AddImmigrants(_playerId, 2, gateTile());
	} else {
		ForceSetHouseLevel(SimSettings::Get("CheatHouseLevel"));
	}

	//if (SimSettings::Get("CheatHouseLevel") > 1) {
	//	switch(SimSettings::Get("CheatHouseLevel"))
	//	{
	//	case 7: AddResource(ResourceEnum::Jewelry, 10);
	//	case 6: AddResource(ResourceEnum::Book, 10);
	//	case 5: AddResource(ResourceEnum::Chocolate, 10);
	//	case 4: AddResource(ResourceEnum::Wine, 10);
	//	case 3: AddResource(ResourceEnum::Beer, 10);
	//	case 2: AddResource(ResourceEnum::Pottery, 10);
	//		break;
	//	default:
	//		break;
	//	}
	//	CheckHouseLvl();
	//}
	
	//AddResourceHolder(ResourceEnum::Wood, ResourceHolderType::Manual, 10);
	//AddResourceHolder(ResourceEnum::Coal, ResourceHolderType::Manual, 10);

	//ExecuteOnLuxuryResources([&](ResourceEnum resourceEnum) {
	//	AddResourceHolder(resourceEnum, ResourceHolderType::Manual, 10);
	//});

	// Research enabled after building first house
	if (_simulation->unlockSystem(_playerId)->researchEnabled != true) {
		_simulation->unlockSystem(_playerId)->researchEnabled = true;

		PopupInfo popupInfo(_playerId,
			"Unlocked research.\n"
			"Acquire science points <img id=\"Science\"/> by increasing population, upgrading houses, or building libraries.",
			{ "Show tech tree", "Close" }, PopupReceiverEnum::DoneResearchEvent_ShowTree);
		popupInfo.forcedSkipNetworking = true;
		_simulation->AddPopup(popupInfo);
	}

	auto woodCoalHeatText = [](int32 percent) {
		return FText::Format(LOCTEXT("woodCoalHeatText", "Wood/coal gives {0}% more heat"), TEXT_NUM(percent));
	};
	
	_upgrades = {		
		BuildingUpgrade(LOCTEXT("Stone Insulation", "Stone Insulation"), woodCoalHeatText(20), ResourcePair(ResourceEnum::Stone, 20)),
		BuildingUpgrade(LOCTEXT("Hearth Fireplace", "Hearth Fireplace"), woodCoalHeatText(30), ResourcePair(ResourceEnum::Brick, 30)),
	};

	

	auto unlockSys = _simulation->unlockSystem(_playerId);
	unlockSys->UpdateProsperityHouseCount();
	

	// Quest
	//_simulation->eventSource(EventSourceEnum::BuiltHouse).Publish(_playerId, _objectId);
	_simulation->QuestUpdateStatus(_playerId, QuestEnum::BuildHousesQuest, 0);

	//UpdateSubscription();

	_simulation->RecalculateTaxDelayed(_playerId); // Recalculate sci


}

int32 House::GetAppealPercent() {
	int32 appeal = _simulation->overlaySystem().GetAppealPercent(_centerTile);
	if (_simulation->buildingFinishedCount(_playerId, CardEnum::ArchitectStudio)) {
		appeal += 5;
	}
	if (_simulation->buildingFinishedCount(_playerId, CardEnum::EnvironmentalistGuild)) {
		appeal += 15;
	}
	return appeal;
}
int32 House::housingHappiness() {
	int32 happiness = GetAppealPercent();
	return happiness;
}

int32 House::luxuryCount()
{
	int count = 0;
	ExecuteOnLuxuryResources([&](ResourceEnum resourceEnum) {
		if (resourceCount(resourceEnum) > 0) count++;
	});
	//for (ResourceEnum resourceEnum : LuxuryResources) {
	//	if (resourceCount(resourceEnum) > 0) count++;
	//}
	return count;
}
void House::CalculateConsumptions(bool consumeLuxury)
{
	/*
	 * Calculate luxury and food consumption
	 *
	 * food:
	 *  10% to income
	 *  10% to sci
	 *
	 * luxury:
	 *  30% to science
	 *  70% to income
	 */
	
	_roundLuxuryConsumption100 = 0;
	ExecuteOnLuxuryResources([&](ResourceEnum resourceEnum) {
		//for (ResourceEnum resourceEnum : LuxuryResources) {
		if (resourceCount(resourceEnum) > 0 &&
			occupantCount() > 0)
		{
			int32 price100 = _simulation->price100(resourceEnum);

			// Target tax taking into account Science/Culture benefit of lux resources
			int32 luxuryConsumption100_perRound = HumanLuxuryCost100PerRound_ForEachType * occupantCount();

			int32 actualConsumption = GameRand::RandRound(luxuryConsumption100_perRound, price100, Time::Rounds());
			//actualConsumption = max(1, actualConsumption);
			actualConsumption = min(resourceCount(resourceEnum), actualConsumption); // Can't be more than available resource

			if (consumeLuxury) {
				RemoveResource(resourceEnum, actualConsumption);
				_simulation->statSystem(_playerId).AddResourceStat(ResourceSeasonStatEnum::Consumption, resourceEnum, actualConsumption);
			}

			_roundLuxuryConsumption100 += luxuryConsumption100_perRound;
		}
		//}
	});

	// Food consumption
	const int32 foodCost100PerRound_perMan = BaseHumanFoodCost100PerYear / Time::RoundsPerYear;
	int32 foodCost100PerRound_house = occupancyFactor(foodCost100PerRound_perMan * houseBaseOccupants);

	_roundFoodConsumption100 = foodCost100PerRound_house;
}

int32 House::GetIncome100(IncomeEnum incomeEnum)
{
	switch (incomeEnum)
	{
	// Only convert 10% of food cost to tax since most is assumed to be converted to work...
	case IncomeEnum::Base:
		return _roundFoodConsumption100 / 10;

	case IncomeEnum::Tech_MoreGoldPerHouse:
		return occupancyFactor(_simulation->IsResearched(_playerId, TechEnum::MoreGoldPerHouse) ? 300 : 0);

	case IncomeEnum::Tech_HouseLvl6Income:
		return occupancyFactor(_simulation->IsResearched(_playerId, TechEnum::HouseLvl6Income) && (_houseLvl >= 6 ? 1000 : 0));

	case IncomeEnum::Appeal: {
		int32 percentIncomeIncrease = GetAppealPercent() / 5;
		return GetIncome100(IncomeEnum::Base) * percentIncomeIncrease / 100; // 10 appeal convert to 2% income ...	
	}

	case IncomeEnum::Adjacency:
		return occupancyFactor(adjacentBonusCount() * 100);

	case IncomeEnum::Luxury:
		return _roundLuxuryConsumption100 * 4 / 10; // 40% of lux goes to income

	case IncomeEnum::Card_MiddleClass:
		if (_houseLvl >= 2) {
			int32 cardCount = _simulation->TownhallCardCount(_playerId, CardEnum::MiddleClassTax);
			return occupancyFactor(cardCount * 200);
		}
		return 0;

	case IncomeEnum::Card_BeerTax: {
		if (_simulation->TownhallCardCount(_playerId, CardEnum::BeerTax) > 0) {
			return resourceCount(ResourceEnum::Beer) > 0 ? occupancyFactor(500) : 0;
		}
		return 0;
	}

	case IncomeEnum::Card_DesertPilgrim:
		if (_simulation->TownhallCardCount(_playerId, CardEnum::DesertPilgrim) > 0 &&
			_simulation->terrainGenerator().GetBiome(_centerTile) == BiomeEnum::Desert) 
		{
			return  occupancyFactor(500);
		} else {
			return 0;
		}

	default:
		UE_DEBUG_BREAK();
		return 0;
	};
}

int32 House::GetScience100(ScienceEnum scienceEnum)
{
	switch (scienceEnum)
	{
	case ScienceEnum::Base:
		return _roundFoodConsumption100 / 30;

	case ScienceEnum::Luxury:
		return _roundLuxuryConsumption100 * 4 / 10; // 40% lux goes to science
		
	case ScienceEnum::Library: {
		if (_houseLvl < Library::MinHouseLvl) {
			return 0;
		}
		int32 radiusBonus = GetRadiusBonus(CardEnum::Library, Library::Radius, [&](int32 bonus, Building& building) {
			return max(bonus, Library::SciencePerHouse);
		});
		return occupancyFactor(radiusBonus * 100);
	}

	case ScienceEnum::School: {
		if (_houseLvl < School::MinHouseLvl) {
			return 0;
		}
		int32 radiusBonus = GetRadiusBonus(CardEnum::School, Library::Radius, [&](int32 bonus, Building& building) {
			return max(bonus, School::SciencePerHouse);
		});
		return occupancyFactor(radiusBonus * 100);
	}

	case ScienceEnum::HomeBrew: {
		if (_simulation->TownhallCardCount(_playerId, CardEnum::HomeBrew) == 0) {
			return 0;
		}
		return resourceCount(ResourceEnum::Pottery) > 0 ? occupancyFactor(400) : 0;
	}

	default:
		UE_DEBUG_BREAK();
		return 0;
	};
}

void House::OnPickupResource(int32 objectId)
{
	PUN_CHECK(_buildingEnum == CardEnum::House);
	if (!_simulation->buildingIsAlive(_objectId) || isDisabled() || !isConstructed() || _playerId == -1) {
		return;
	}

	CheckHouseLvl();
}
void House::OnDropoffResource(int32 objectId, ResourceHolderInfo holderInfo, int32 amount)
{
	PUN_CHECK(_buildingEnum == CardEnum::House);
	if (!_simulation->buildingIsAlive(_objectId) || isDisabled() || !isConstructed() || _playerId == -1) {
		return;
	}

	// Stone drop weirdness..
	PUN_CHECK(holderInfo.resourceEnum != ResourceEnum::Stone);
	
	CheckHouseLvl();
}

void House::UpgradeHouse(int32 lvl)
{
	int32& maxAchivedLevel = _simulation->parameters(_playerId)->MaxAchievedHouseLvl;
	if (lvl > maxAchivedLevel) 
	{
		std::stringstream ss;
		ss << "Well done! First house level " << lvl << ". ";
		ss << "<space>";

		auto unlockSys = _simulation->unlockSystem(_playerId);

		if (lvl == 2)
		{
			// TODO: use this for something else
			//ss << "Unlocked:";
			//ss << "<bullet>Flower bed</>";

			//unlockSys->UnlockBuilding(CardEnum::FlowerBed);
		}
		else if (lvl == 3)
		{
			//ss << "Unlocked:";
			//ss << "<bullet>Shrubbery</>";

			//unlockSys->UnlockBuilding(CardEnum::GardenShrubbery1);
		}
		else if (lvl == 4)
		{
			//ss << "Unlocked:";
			//ss << "<bullet>Garden cypress</>";

			//unlockSys->UnlockBuilding(CardEnum::GardenCypress);
		}

		_simulation->AddPopup(_playerId, ss.str());


		maxAchivedLevel = lvl;
	}

	_simulation->uiInterface()->ShowFloatupInfo(FloatupEnum::HouseUpgrade, _centerTile, "");
	_houseLvl = lvl;

	//_simulation->QuestUpdateStatus(_playerId, QuestEnum::HouseUpgradeQuest);


	_simulation->UpdateProsperityHouseCount(_playerId);
	
	
	_simulation->soundInterface()->Spawn2DSound("UI", "UpgradeHouse", -1, _centerTile);

	_allowedOccupants = houseBaseOccupants + (lvl - 1) / 2;
	_simulation->RecalculateTaxDelayed(_playerId); // Recalculate sci
	ResetDisplay();
}

int32 House::GetMaxHouseLvl() {
	return 7;
}
std::string House::HouseNeedDescription()
{
	switch(_houseLvl)
	{
	case 1: return "Need 1 type of luxury tier 1";
	case 2: return "Need 3 types of luxury tier 1";
	case 3: return "Need 1 type of luxury tier 2";
	case 4: return "Need 3 types of luxury tier 2";
	case 5: return "Need 1 types of luxury tier 3";
	case 6: return "Need 3 types of luxury tier 3";
	default:
		UE_DEBUG_BREAK();
		return "";
	}
	
	//const std::vector<ResourceEnum>& upgradeNeeds = HouseLvlToUpgradeNeeds[_houseLvl];
	//if (upgradeNeeds.size() > 0) {
	//	stringstream ss;
	//	for (size_t i = 0; i < upgradeNeeds.size(); i++) {
	//		if (i > 0) {
	//			ss << " or ";
	//		}
	//		ss << ResourceName(upgradeNeeds[i]);
	//	}

	//	std::string result = ss.str();
	//	transform(result.begin(), result.end(), result.begin(), ::tolower);
	//	return result;
	//}

	//UE_DEBUG_BREAK();
	//return "";
}
std::string House::HouseNeedTooltipDescription()
{
	switch (_houseLvl)
	{
	case 1: return LuxuryResourceTip(1);
	case 2: return LuxuryResourceTip(1);
	case 3: return LuxuryResourceTip(2);
	case 4: return LuxuryResourceTip(2);
	case 5: return LuxuryResourceTip(3);
	case 6: return LuxuryResourceTip(3);
	default:
		UE_DEBUG_BREAK();
		return "";
	}
}
int32 House::CalculateHouseLevel()
{
	switch(LuxuryTypeCount(1))
	{
	case 0:			return 1;
	case 1: case 2: return 2;
	default: break;
	}

	switch (LuxuryTypeCount(2))
	{
	case 0:			return 3;
	case 1: case 2: return 4;
	default: break;
	}

	switch (LuxuryTypeCount(3))
	{
	case 0:			return 5;
	case 1: case 2: return 6;
	case 3:			return 7;

	default: return 7;
	}
	
}

// Boar

void BoarBurrow::FinishConstruction()
{
	Building::FinishConstruction();
	_simulation->AddBoarBurrow(_simulation->GetProvinceIdClean(_centerTile), _objectId);

	_allowedOccupants = 4;
	_maxOccupants = 4;
}

void BoarBurrow::OnDeinit()
{
	ResetOccupants();
	
	_simulation->RemoveTenantFrom(_objectId);
	_simulation->RemoveBoarBurrow(_simulation->GetProvinceIdClean(_centerTile), _objectId);
}

//void RanchBarn::FinishConstruction()
//{
//	Building::FinishConstruction();
//
//	AddResourceHolder(ResourceEnum::Hay, ResourceHolderType::Requester, 20);
//}
//
//void RanchBarn::OnTick1Sec()
//{
//	if (!isConstructed()) {
//		return;
//	}
//}
//
//void RanchBarn::AddAnimalOccupant(UnitEnum animalEnum, int32 age) 
//{
//	PUN_CHECK(_animalEnum == animalEnum);
//	int32 newAnimalId = _simulation->AddUnit(animalEnum, _playerId, gateTile().worldAtom2(), age);
//	PUN_CHECK(_animalOccupants.size() < maxAnimals);
//	_animalOccupants.push_back(newAnimalId);
//	_simulation->unitAI(newAnimalId).SetHouseId(buildingId());
//}
//
//void RanchBarn::RemoveAnimalOccupant(int32 animalId)
//{
//	CppUtils::Remove(_animalOccupants, animalId);
//}
//
//void RanchBarn::OnDeinit()
//{
//	// Release animals into the wild
//	for (int i = 0; i < _animalOccupants.size(); i++) {
//		int32_t id = _animalOccupants[i];
//		auto& unit = _simulation->unitAI(id);
//		unit.SetHouseId(-1);
//		unit.SetPlayerId(-1);
//		_simulation->ResetUnitActions(id);
//	}
//	_animalOccupants.clear();
//}

/*
 * Ranch
 */
void Ranch::FinishConstruction()
{
	Building::FinishConstruction();

	auto addInitialAnimals = [&](UnitEnum unitEnum) {
		for (int32 i = 0; i < 3; i++) {
			AddAnimalOccupant(unitEnum, GetUnitInfo(unitEnum).minBreedingAgeTicks);
		}
	};

	switch (buildingEnum()) {
	case CardEnum::RanchPig: addInitialAnimals(UnitEnum::Pig); break;
	case CardEnum::RanchSheep: addInitialAnimals(UnitEnum::Sheep); break;
	case CardEnum::RanchCow: addInitialAnimals(UnitEnum::Cow); break;
	default:
		UE_DEBUG_BREAK();
		break;
	}

	AddResourceHolder(ResourceEnum::Hay, ResourceHolderType::Requester, 20);
	AddResourceHolder(ResourceEnum::Milk, ResourceHolderType::Provider, 0);

	workModes = {
		{ LOCTEXT("Kill when reached full capacity", "Kill when reached full capacity"), ResourceEnum::None, ResourceEnum::None, 0},
		{ LOCTEXT("Kill when above half capacity", "Kill when above half capacity"), ResourceEnum::None, ResourceEnum::None, 0},
		{ LOCTEXT("Kill all", "Kill all"), ResourceEnum::None, ResourceEnum::None, 0},
	};
	_workMode = workModes[0];
}

void Ranch::AddAnimalOccupant(UnitEnum animalEnum, int32_t age)
{
	int32 newAnimalId = _simulation->AddUnit(animalEnum, _playerId, centerTile().worldAtom2(), age);
	PUN_CHECK(_animalOccupants.size() < maxAnimals);
	_animalOccupants.push_back(newAnimalId);

	auto& unitAI = _simulation->unitAI(newAnimalId);
	unitAI.SetHouseId(buildingId());

	PUN_LOG("AddAnimalOccupant %s", ToTChar(unitAI.compactStr()));

	PUN_CHECK(_simulation->unitAI(newAnimalId).houseId() != -1);
}

void Ranch::OnDeinit()
{
	// Kill all animals
	for (int i = _animalOccupants.size(); i-- > 0;) {
		int32 id = _animalOccupants[i];
		auto& unit = _simulation->unitAI(id);

		unit.Die();
		
		//_simulation->ResetUnitActions(id); // Must reset before SetPlayerId(-1) or CancelReservation will crash
		//
		//unit.SetHouseId(-1);
		//unit.SetPlayerId(-1);
	}
	_animalOccupants.clear();
}


#undef LOCTEXT_NAMESPACE