#include "UnitStateAI.h"

#include "IGameSimulationCore.h"
#include "TreeSystem.h"
#include "PunCity/MapUtil.h"
#include "../GameConstants.h"
#include "../GameRand.h"
#include "PlayerParameters.h"
#include "PunCity/NetworkStructs.h"
#include "PunCity/PunUtils.h"
#include "Buildings/House.h"
#include "Buildings/GathererHut.h"
#include "Buildings/Townhall.h"
#include "StatSystem.h"

#include "UnlockSystem.h"


DECLARE_CYCLE_STAT(TEXT("PUN: Unit.FoodAgeAI [3]"), STAT_PunUnitFoodAgeAI, STATGROUP_Game);
DECLARE_CYCLE_STAT(TEXT("PUN: Unit.CalcAction [4]"), STAT_PunUnitCalcAction, STATGROUP_Game);
DECLARE_CYCLE_STAT(TEXT("PUN: Unit.DoAction [5]"), STAT_PunUnitDoAction, STATGROUP_Game);

DECLARE_CYCLE_STAT(TEXT("PUN: Unit.Do.MoveRandomly [5.1]"), STAT_PunUnitDoMoveRandomly, STATGROUP_Game);
DECLARE_CYCLE_STAT(TEXT("PUN: Unit.Do.MoveTo [5.2]"), STAT_PunUnitDoMoveTo, STATGROUP_Game);

DECLARE_CYCLE_STAT(TEXT("PUN: Unit.CalcAnimal [4.1]"), STAT_PunUnitCalcAnimal, STATGROUP_Game);
DECLARE_CYCLE_STAT(TEXT("PUN: Unit.CalcWildNoHome [4.1.1]"), STAT_PunUnitCalcWildNoHome, STATGROUP_Game);
DECLARE_CYCLE_STAT(TEXT("PUN: Unit.CalcWildHome [4.1.2]"), STAT_PunUnitCalcWildHome, STATGROUP_Game);
DECLARE_CYCLE_STAT(TEXT("PUN: Unit.FindWildFood [4...]"), STAT_PunUnitCalcFindWildFood, STATGROUP_Game);

DECLARE_CYCLE_STAT(TEXT("PUN: Unit.CalcBoarGetHome [4.1.2.1]"), STAT_PunUnitCalcBoarGetHome, STATGROUP_Game);
DECLARE_CYCLE_STAT(TEXT("PUN: Unit.CalcBoarHomeFood [4.1.2.2]"), STAT_PunUnitCalcBoarHomeFood, STATGROUP_Game);

DECLARE_CYCLE_STAT(TEXT("PUN: Unit.AddDebugSpeech [-]"), STAT_PunUnitAddDebugSpeech, STATGROUP_Game);


#define USE_DEBUG_SPEECH 0

using namespace std;

#define LOCTEXT_NAMESPACE "UnitStateAI"


static std::vector<uint32_t> rawWaypoint;

int32 UnitStateAI::debugFindFullBushSuccessCount = 0;
int32 UnitStateAI::debugFindFullBushFailCount = 0;

void UnitStateAI::AddUnit(UnitEnum unitEnum, int32 playerId, UnitFullId fullId, IUnitDataSource* unitData, IGameSimulationCore* simulation)
{
	_unitData = unitData;
	_simulation = simulation;
	_id = fullId.id;
	_fullId = fullId;

	_unitEnum = unitEnum;
	_unitState = UnitState::Idle;

	_food = maxFood() / 2;
	_health = 0;
	_heat = unitInfo().maxHeatCelsiusTicks;
	_lastTickCelsiusRate = 0;

	_funTicks = FunTicksAt100Percent * 80 / 100;

	//_birthTick = Time::Ticks() - ageTicks;
	_lastUpdateTick = Time::Ticks();
	_lastPregnant = -1;

	_nextPregnantTick = Time::TicksPerMinute + GameRand::Rand() % (Time::TicksPerYear / 2); // For those immigrants, they could be pregnant anytime now to +half year

	_playerId = playerId;

	_inventory = UnitInventory();
	_workplaceId = -1;
	_houseId = -1;
	_homeProvinceId = -1;
	_lastFindWildFoodTick = 0;

	_hp100 = 10000;
	_isSick = false;
	_nextToolNeedTick = 0;

	_justDidResetActions = false;

	_animationEnum = UnitAnimationEnum::Wait;


	if (IsWildAnimal(unitEnum)) 
	{
		WorldTile2 tile = _unitData->atomLocation(_id).worldTile2();
		_homeProvinceId = _simulation->GetProvinceIdClean(tile);
		PUN_CHECK(_homeProvinceId != -1);
		PUN_CHECK(_simulation->IsProvinceValid(_homeProvinceId));

		// Start off in parent's province
		_simulation->AddProvinceAnimals(_homeProvinceId, _id);
	}

	PUN_CHECK2(reservations.empty(), debugStr());
	AddDebugSpeech("Birth");

	//PUN_LOG("AddUnit %s", ToTChar(compactStr()));
}

int32 UnitStateAI::birthChance()
{
	int32 cap, population;

	switch (unitEnum())
	{
		// If it is human, we need to cap it by house
		// the more empty space (houseCap - pop) the faster growth
		// At pop of 0, slow factor is 1
		case UnitEnum::Human: {
			cap = _simulation->HousingCapacity(_playerId);
			population = _simulation->population(_playerId);
			break;
		} 
		default: {
			cap = GameConstants::IdealWorldPop;
			population = _simulation->unitCount();

			// Ranch's animal reproduce at max..
			if (_playerId != -1 && IsAnimal(_unitData->unitEnum(_id))) {
				cap = 1;
				population = 0;
			}
		}
	}
	int32 baseBirthChance = isEnum(UnitEnum::Human) ? HumanBaseBirthChance : UnitBaseBirthChance;

	if (population >= cap) return 0;
	return cap / (cap - population) * baseBirthChance;
}

void UnitStateAI::Update()
{
	UnitInfo uInfo = unitInfo();

	if (uInfo.IsAgingUnit())
	{
		SCOPE_CYCLE_COUNTER(STAT_PunUnitFoodAgeAI);

		// Check status
		int ticksPassed = Time::Ticks() - _lastUpdateTick;

		int32 adjustedTicksPassed = ticksPassed;
		if (GameConstants::IsAI(_playerId)) {
			adjustedTicksPassed /= 2; // AI cheat
		}

		
		_food -= adjustedTicksPassed;

		_heat += adjustedTicksPassed * _lastTickCelsiusRate; // use TickCelsius rate from last update. This so that the changes between updates are constant and predictable. (and can be displayed in UI)
		int32 tickCelsiusRate = FDToInt(_simulation->Celsius(unitTile())) - FDToInt(Time::ColdCelsius()); // from 5 Celsius down, it is considered cold...???
		if (tickCelsiusRate >= 0) 
		{
			tickCelsiusRate *= 8; // Heat recover faster once above critical temperature
			if (_heat >= uInfo.maxHeatCelsiusTicks) {
				tickCelsiusRate = 0; // Don't increase heat once the max is reached.	
			}
			// Special case: Forced normal heat once spring arrive. No cold ppl during spring
			if (_heat < minWarnHeat()) {
				_heat = minWarnHeat();
			}
		}
		_lastTickCelsiusRate = tickCelsiusRate;

		_funTicks = std::max(0, _funTicks - ticksPassed);
		
		//int32_t heatChangeThisTick = ticksPassed * (tickCelsiusFromX > 0 ? tickCelsiusFromX * 3 : tickCelsiusFromX); // Heat recover 3 times faster once above critical temperature

		//// Frostbite fate
		//if (_playerId != -1) {
		//	heatChangeThisTick = GameRand::Rand100RoundTo1(heatChangeThisTick * (100 + _simulation->fateLinkSystem().GetFateEffectFor(_playerId, FateEnum::Frostbite)));
		//}

		// Sickness decreases health
		// Ppl with sickness will die in 2 seasons without medicine...
		if (_isSick)
		{
			// 1-3 season to death (2 ave)
			int32 ticksToDeath = Time::TicksPerSeason + GameRand::Rand() % (Time::TicksPerSeason * 2);
			_hp100 -= (ticksPassed * maxHP100()) / ticksToDeath;

			if (_hp100 <= 0)
			{
				_simulation->AddEventLog(_playerId, 
					FText::Format(LOCTEXT("DiedSick_Event", "{0} died from sickness"), GetUnitNameT()), 
					true
				);
				_simulation->statSystem(_playerId).AddStat(SeasonStatEnum::DeathSick);
				_simulation->soundInterface()->Spawn2DSound("UI", "DeathBell", _playerId);

				if (_simulation->IsAIPlayer(_playerId)) {
					_LOG(PunAI, "%s Death Sickness", _simulation->AIPrintPrefix(_playerId));
				}
				
				Die();
				return;
			}
		}

	
		_lastUpdateTick = Time::Ticks();

		if (isEnum(UnitEnum::Human))
		{
			auto& resourceSys = _simulation->resourceSystem(_playerId);
			
			// Food cheat: if there is food in storage, ppl won't go into starvation...
			if (_food < minWarnFood() + 1 && resourceSys.HasAvailableFood()) {
				_food = minWarnFood() + 1;
			}

			// Cold cheat:
			//  Need to take into account GetAllowedResource
			//  Anything ppl check anything off don't check it for cheat
			if (_heat < minWarnHeat()) {
				auto& playerOwned = _simulation->playerOwned(_playerId);
				bool isCoalAvailableForHeating = playerOwned.GetHouseResourceAllow(ResourceEnum::Coal) && resourceSys.resourceCountWithPop(ResourceEnum::Coal);
				bool isWoodAvailableForHeating = playerOwned.GetHouseResourceAllow(ResourceEnum::Wood) && resourceSys.resourceCountWithPop(ResourceEnum::Wood);

				if (isCoalAvailableForHeating || isWoodAvailableForHeating) {
					_heat = minWarnHeat();
				}
			}


			// Sickness cheat:
			if (_isSick && _hp100 < maxHP100() / 3 && resourceSys.HasAvailableMedicine())
			{
				_hp100 = maxHP100() / 3;
			}

			// Homeless people could leave your town
			// This only happens beyond 20 population
			// Random chance to leave within 1 season
			if (_houseId == -1 && _playerId != -1 &&
				_simulation->population(_playerId) > 30 &&
				GameRand::Rand() % (Time::TicksPerRound * 2) < ticksPassed)
			{
				_simulation->AddMigrationPendingCount(_playerId, 1);
				_simulation->AddEventLog(_playerId, 
					FText::Format(LOCTEXT("XLeftTownHomeless_Event", "{0} left your town (homeless)."), GetUnitNameT()),
					true
				);

				_simulation->soundInterface()->Spawn2DSound("UI", "DeathBell", _playerId);

				if (_simulation->IsAIPlayer(_playerId)) {
					_LOG(PunAI, "%s Left Homeless", _simulation->AIPrintPrefix(_playerId));
				}
				
				Die();
				return;
			}
		}

		// TODO: Domestic animal food/cold cheat...
		if (IsDomesticatedAnimal(unitEnum()))
		{
			// If there is a ranch, make sure it doesn't die.
			if (_houseId != -1) {
				// Note: half, instead of minWarnFood() etc. to prevent starving warning...
				if (_food < maxFood() / 2) {
					_food = maxFood() / 2;
				}
				if (_heat < maxHeat() / 2) {
					_heat = maxHeat() / 2;
				}
			}
			else {
 				PUN_LOG("Animal without home %s", ToTChar(compactStr()));
				//UE_DEBUG_BREAK();
			}
		}

		if (IsWildAnimal(unitEnum()))
		{
			if (_heat < minWarnHeat()) {
				_heat = minWarnHeat();
			}
		}

		/*
		 * Starve/Cold
		 */
		if (_food < 0 || _heat < 0) 
		{
			if (!SimSettings::IsOn("CheatUndead") &&
				!IsWildAnimalWithColony(unitEnum()))
			{
				// Roll the dice...
				// If ticksPassed == 1.5 round, certain death
				if (GameRand::Rand() % (Time::TicksPerRound * 3 / 2) < ticksPassed) 
				{
					//if (_playerId != GameInfo::PlayerIdNone) {
					//	PUN_LOG("DeathFoodOrHeat ID:%d reservations:%d food:%d heat:%d", _id, reservations.size(), _food, _heat);
					//}

					if (_food < 0) 
					{
						/*
						 * Human migration when low on food
						 *  half chance to migrate given good place
						 */
						if (isEnum(UnitEnum::Human) && _playerId != -1 &&
							GameRand::Rand() % 3 == 0) 
						{
							_simulation->AddMigrationPendingCount(_playerId, 1);
							_simulation->AddEventLog(_playerId, 
								FText::Format(LOCTEXT("XLeftTownStarve_Event", "{0} left your town (starving)."), GetUnitNameT()), 
								true
							);
						}
						else
						{
							_simulation->AddEventLog(_playerId, 
								FText::Format(LOCTEXT("XDiedStarve_Event", "{0} died from starvation"), GetUnitNameT()),
								true
							);
							_simulation->statSystem(_playerId).AddStat(SeasonStatEnum::DeathStarve);
						}

						if (_simulation->IsAIPlayer(_playerId)) {
							_LOG(PunAI, "%s Died Starving", _simulation->AIPrintPrefix(_playerId));
						}
					}
					else 
					{
						_simulation->AddEventLog(_playerId, 
							FText::Format(LOCTEXT("XDiedCold_Event", "{0} died from cold"), GetUnitNameT()),
							true
						);
						_simulation->statSystem(_playerId).AddStat(SeasonStatEnum::DeathCold);

						if (_simulation->IsAIPlayer(_playerId)) {
							_LOG(PunAI, "%s Died Cold", _simulation->AIPrintPrefix(_playerId));
						}
					}
					
					_simulation->soundInterface()->Spawn2DSound("UI", "DeathBell", _playerId);

					Die();
					return;
				}
			}
		}


		// Pregnancy
		// -- Pregnant
		if (unitEnum() == UnitEnum::Human)
		{
			auto parameters = _simulation->parameters(_playerId);
			
			if (!isMale() &&
				Time::Ticks() >= _nextPregnantTick &&
				age() > parameters->BeginBreedingAgeTicks() &&
				age() < parameters->EndBreedingAgeTicks())
			{
				// Play BabyBornBell
				//if (_simulation->TryDoNonRepeatAction(_playerId, NonRepeatActionEnum::BabyBornBell, Time::TicksPerSecond * 30 * _simulation->gameSpeedMultiplier())) {
				//	_simulation->soundInterface()->Spawn2DSound("UI", "BabyBornBell", _playerId);
				//}

				bool isBirthControlActivated = _simulation->TownhallCardCount(_playerId, CardEnum::BirthControl) > 0 &&
												_simulation->population(_playerId) >= _simulation->HousingCapacity(_playerId);

				if (!isBirthControlActivated) {
					_simulation->AddUnit(unitEnum(), _playerId, _unitData->atomLocation(_id), 0);
				}

				_nextPregnantTick = Time::Ticks() + parameters->TicksBetweenPregnancy() - GameRand::Rand() % parameters->TicksBetweenPregnancyRange();
			}
		}
		else
		{
			if (_lastPregnant > 0) // breeded before
			{
				bool canGiveBirth = Time::Ticks() - _lastPregnant > uInfo.gestationTicks &&
									food() > minWarnFood() && 
									_simulation->GetProvinceIdClean(unitTile()) != -1;


				// Domesticated animal can't reproduce outside
				if (IsDomesticatedAnimal(unitEnum())) {
					if (houseId() == -1) {
						canGiveBirth = false;
					}
				}
				else
				{
					// Wild Animals does not reproduce beyond province's max
					if (_homeProvinceId != -1) {
						const int32 provinceMaxAnimal = 5;
						if (_simulation->provinceAnimals(_homeProvinceId).size() >= provinceMaxAnimal) {
							canGiveBirth = false;
						}
					}
					else {
						canGiveBirth = false;
					}
				}

				if (canGiveBirth)
				{
					// TODO: Boar put out multiple units and raise their young

					int32 chance = birthChance();
					if (chance > 0 && GameRand::Rand() % chance == 0)
					{
						// if farm animal... live in the same farm...
						if (_playerId != -1 && 
							IsAnimal(unitEnum()) && 
							houseId() != -1) 
						{
							Building& building = _simulation->building(houseId());

							// TODO: Weird that sometimes building isn't barn, barn problem destroyed???
							PUN_CHECK(IsRanch(building.buildingEnum()));
							if (IsRanch(building.buildingEnum()))
							{
								Ranch& ranch = building.subclass<Ranch>();
								if (ranch.openAnimalSlots() > 0) {
									ranch.AddAnimalOccupant(unitEnum(), 0);
								}
							}
						}
						else
						{
							int32 newUnitId = _simulation->AddUnit(unitEnum(), _playerId, _unitData->atomLocation(_id), 0);

							if (IsDomesticatedAnimal(unitEnum())) {
								PUN_LOG("NonRanch Stray Animal %s", ToTChar(compactStr()));
								PUN_CHECK(_simulation->unitAI(newUnitId).houseId() != -1);
							}
						}

						_simulation->statSystem(_playerId).AddStat(SeasonStatEnum::Birth);
					}
					_lastPregnant = -1;
				}
			}
			// -- Give Birth?
			else if (age() > uInfo.minBreedingAgeTicks) // Never breed before, prepare to breeed when ready
			{
				_lastPregnant = Time::Ticks();
			}
		}

		// Aging Death
		if (age() > maxAge()) 
		{
			if (!SimSettings::IsOn("CheatUndead") &&
				!IsWildAnimalWithColony(unitEnum()))
			{
				if (_playerId != GameInfo::PlayerIdNone) {
					PUN_LOG("DeathAge: %d reservations:%d", _id, reservations.size());
					PUN_LOG("DeathAge AddEventLogF:  %s", ToTChar(compactStr()));

					_simulation->AddEventLog(_playerId,
						FText::Format(LOCTEXT("DiedOld_Event", "{0} died of old age"), GetUnitNameT()),
						false
					);
				}

				_simulation->statSystem(_playerId).AddStat(SeasonStatEnum::DeathAge);
				
				Die();
				return;
			}
		}
	}


	ResetAnimation();
	//PUN_LOG("Anim UnitStateAI::Update() ResetAnimation id:%d", _id);

	// Action
	if (_actions.empty())
	{
		SCOPE_CYCLE_COUNTER(STAT_PunUnitCalcAction);
		
		// Do the action next tick
		NextAction(UnitUpdateCallerEnum::EmptyActions);
		CalculateActions();
	}
	else
	{
		SCOPE_CYCLE_COUNTER(STAT_PunUnitDoAction);

		//if (transform.id < 10) {
		//	UE_LOG(LogTemp, Error, TEXT("Update2: %d, %s"), transform.id, *FString(transform.actualLocation.worldTile2().ToString().c_str()));
		//}

		// Do action after pop. In case the action replaces itself with several actions (such as TryStoreResources)
		_currentAction = _actions.back();
		_actions.pop_back();

		_actionHistory.push_back(_currentAction);

		AddDebugSpeech("[POP]: size:" + to_string(_actions.size()));

		ExecuteAction();
	}
}

void UnitStateAI::Die() 
{
	_simulation->ResetUnitActions(_id);
	AddDebugSpeech("Dead");

	// Animals
	// TODO: House-Unit link system.. one place to store data..
	if (IsWildAnimalWithHome(unitEnum()) && _houseId != -1) {
		_simulation->building(_houseId).RemoveOccupant(_id);
		_houseId = -1;
	}
	// Has owner
	if (IsAnimal(unitEnum()) && _playerId != -1) 
	{
		// TODO: why is building sometimes BoarBurrow when it should always be RanchBarn
		//PUN_CHECK2(_houseId != -1, debugStr());

		// Note: need this check to combat (_houseId == -1) weirdness
		if (_houseId != -1)
		{
			Building& bld = _simulation->building(_houseId);
			if (IsRanch(bld.buildingEnum())) {
				_simulation->building(_houseId).subclass<Ranch>().RemoveAnimalOccupant(_id);
			}
			_houseId = -1;
		}
	}

	//! Human
	if (unitEnum() == UnitEnum::Human)
	{
		if (_simulation->TownhallCardCount(_playerId, CardEnum::Cannibalism) > 0) {
			_simulation->resourceSystem(_playerId).SpawnDrop(ResourceEnum::Pork, 20, unitTile());
		}

		// Cannibalism discovery
		int32 deathCount = _simulation->statSystem(_playerId).GetYearlyStat(SeasonStatEnum::DeathCold);
		deathCount += _simulation->statSystem(_playerId).GetYearlyStat(SeasonStatEnum::DeathStarve);
		if (deathCount > 5 && !_simulation->parameters(_playerId)->CannibalismOffered)
		{
			_simulation->AddPopup(PopupInfo(_playerId, 
				LOCTEXT("CannibalismAsk_Pop", "People are dying left and right. Suddenly, someone asked us to consider eating the corpses for survival. Will you allow Cannibalism?"), 
				{ LOCTEXT("Yes", "Yes"),
					LOCTEXT("No", "No") },
				PopupReceiverEnum::Approve_Cannibalism, true, "PopupBad"
			));
			
			_simulation->parameters(_playerId)->CannibalismOffered = true;
		}
		
	}

	// Animal's workplaceId is regionId
	if (IsWildAnimal(_unitEnum) && _simulation->IsProvinceValid(_homeProvinceId)) {
		_simulation->RemoveProvinceAnimals(_homeProvinceId, _id);
	}

	PUN_CHECK2(_unitData->unitLean(_id).alive(), debugStr());

	//PUN_LOG("Unit Died (Removing) %s", ToTChar(compactStr()));

	_simulation->RemoveUnit(_id);

	//PUN_LOG("Unit Died (Removed) %s", ToTChar(compactStr()));
}

void UnitStateAI::AttackIncoming(UnitFullId attacker, int32 ownerWorkplaceId, int32 damage, int32 attackPlayerId, UnitFullId defenderId)
{
	//PUN_LOG("Unit AttackIncoming id:%d defenderAlive:%d", _id, _unitData->unitLean(_id).alive());
	//PUN_CHECK2(_unitData->unitLean(_id).alive(), debugStr());

	// Unit might already died before arrow arrive...
	// In that case ignorethis...
	//const UnitLean& unitLean = _unitData->unitLean(_id);
	//if (!unitLean.alive()) {
	//	return;
	//}
	//if (defenderId.isValid() && defenderId.birthTicks != unitLean.birthTicks()) {
	//	return;
	//}

	// Unit might already died before arrow arrive...
	if (!_unitData->alive(defenderId)) {
		return;
	}
	if (!_unitData->alive(attacker)) {
		return;
	}


	_hp100 -= damage * 100;

	// Play Sound
	_simulation->soundInterface()->Spawn3DSound("CitizenAction", "BowImpactFlesh", unitAtom());

	if (_hp100 <= 0)
	{
		_simulation->statSystem(_playerId).AddStat(SeasonStatEnum::DeathKilled);

		_simulation->soundInterface()->SpawnAnimalSound(unitEnum(), true, unitAtom());

		// TODO: drop to nonplayer too if player not available??
		if (attacker.isValid() && _simulation->unitAlive(attacker)) {
			auto& unitAI = _simulation->unitAI(attacker.id);
			PUN_CHECK2(unitAI.isEnum(UnitEnum::Human), debugStr());

			// Drops
			std::vector<ResourcePair> drops = unitInfo().resourceDrops100;

			// Random 100 to 1, add efficiency
			for (size_t i = 0; i < drops.size(); i++) 
			{
				PUN_CHECK(drops[i].count >= 100);
				
				Building* workplace = unitAI.workplace();
				int32 efficiency = workplace ? workplace->efficiency() : 0;
				drops[i].count = GameRand::Rand100RoundTo1(drops[i].count * efficiency / 100);
			}
			
			for (ResourcePair& drop : drops)  {
				_simulation->resourceSystem(unitAI.playerId()).SpawnDrop(drop.resourceEnum, drop.count, unitTile());
			}

			// Workplace stat
			if (ownerWorkplaceId != -1 && _simulation->buildingIsAlive(ownerWorkplaceId)) {
				for (ResourcePair& drop : drops) {
					_simulation->building(ownerWorkplaceId).AddProductionStat(drop);
				}
			}
		}

		// TODO: Why this here???
		//if (attackPlayerId != -1) {
		//	std::vector<ResourcePair> drops100 = unitInfo().resourceDrops100;
		//	for (ResourcePair& drop100 : drops100) {
		//		int32 dropCount = GameRand::Rand100RoundTo1(drop100.count);
		//		_simulation->resourceSystem(attackPlayerId).SpawnDrop(drop100.resourceEnum, dropCount, unitTile());
		//	}
		//}

		Die();
	}
}


/*
 * Animals AI
 */

void UnitStateAI::CalculateActions()
{
	SCOPE_CYCLE_COUNTER(STAT_PunUnitCalcAnimal);

	PUN_DEBUG_EXPR(CheckIntegrity());

	PushLastDebugSpeech();

	AddDebugSpeech("CalculateActions (Animals)");

	// Special case:
	if (IsWildAnimalWithColony(unitEnum()))
	{
		_unitState = UnitState::Idle;
		int32 waitTicks = 60 * (GameRand::Rand() % 3 + 4);
		Add_Wait(waitTicks);
		//Add_MoveRandomlyPerlin();
		AddDebugSpeech("(Success)Idle MoveRandomly");
		return;
	}
	

	if (TryCheckBadTile()) return;

	if (TryStoreAnimalInventory()) return;

	// Wild Animal: No Home
	if (IsWildAnimalNoHome(unitEnum())) 
	{
		SCOPE_CYCLE_COUNTER(STAT_PunUnitCalcWildNoHome);

		if (TryGoHomeProvince()) return;

		if (TryFindWildFood()) return;

		_unitState = UnitState::Idle;
		int32 waitTicks = Time::TicksPerSecond * (GameRand::Rand() % 3 + 4);
		Add_Wait(waitTicks);
		Add_MoveRandomlyAnimal(); // This will bring the animal to home province...
		
		AddDebugSpeech("(Success)Idle MoveRandomly");
	}
	else if (IsDomesticatedAnimal(unitEnum()))
	{
		// Only find food within 16 radius since ranch is 16x16
		//if (TryFindWildFood(false, 16)) {
		//	return;
		//}
		
		_unitState = UnitState::Idle;
		int32 waitTicks = 60 * (GameRand::Rand() % 3 + 4);
		Add_Wait(waitTicks);

		if (_houseId != -1) {
			// Move within ranch
			Add_MoveRandomly(_simulation->building(_houseId).area());
		} else {
			Add_MoveRandomly();
		}

		AddDebugSpeech("(Success)Idle MoveRandomly");
	}
	// Wild Animal: With Home
	else if (IsAnimalWithHome(unitEnum()))
	{
		SCOPE_CYCLE_COUNTER(STAT_PunUnitCalcWildHome);

		// Walk home if too far
		if (_houseId != -1 &&
			_simulation->building(_houseId).DistanceTo(unitTile()) > 50)
		{
			if (TryGoNearbyHome()) {
				return;
			}
		}

		// Find Food if needed
		if (TryFindWildFood()) {
			return;
		}
		
		// Has burrow already:
		//  Try Get or Stock Food
		if (_houseId != -1)
		{
			SCOPE_CYCLE_COUNTER(STAT_PunUnitCalcBoarHomeFood);

			if (_food < maxFood() / 2 && TryGetBurrowFood()) {
				return;
			}
			if (TryStockBurrowFood()) {
				return;
			}
		}
		else
		{
			SCOPE_CYCLE_COUNTER(STAT_PunUnitCalcBoarGetHome);

			// Get an existing burrow to live
			const std::vector<int32>& burrowIds = _simulation->boarBurrows(_homeProvinceId);
			for (int32 burrowId : burrowIds)
			{
				// If Burrow is accessible and isn't full. add this as an occupant
				Building& burrow = _simulation->building(burrowId);
				if (burrow.CanAddOccupant() && IsMoveValid(burrow.gateTile()))
				{
					static_cast<BoarBurrow*>(&burrow)->AddOccupant(_id);
					_houseId = burrowId;
					
					_unitState = UnitState::Idle;
					int32 waitTicks = 60 * (GameRand::Rand() % 3 + 4);
					Add_Wait(waitTicks);
					AddDebugSpeech("(Success)Idle Just Moved Home");
					return;
				}
			}

			// No existing burrow:
			if (burrowIds.size() >= 2)
			{
				// Too many burrows in homeProvince, move to a new province
				if (TryChangeProvince_NoAction()) {
					if (TryGoHomeProvince()) {
						return;
					}
				}
			}
			else
			{
				// Try to build a burrow on homeProvince
				const int tries = 10;
				WorldTile2 uTile = unitTile();
				WorldTile2 centerTile = _simulation->GetProvinceRandomTile(_homeProvinceId, uTile, GameConstants::MaxFloodDistance_AnimalFar, false, tries);

				Direction faceDirection = Direction::S;
				TileArea area = BuildingArea(centerTile, BuildingInfo[(int)CardEnum::BoarBurrow].size, faceDirection);

				if (IsBurrowBuildable(area, faceDirection))
				{
					FPlaceBuilding parameters;
					parameters.buildingEnum = static_cast<uint8>(CardEnum::BoarBurrow);
					parameters.faceDirection = static_cast<uint8>(faceDirection);
					parameters.area = area;
					parameters.center = centerTile;
					parameters.playerId = -1;

					int32_t burrowId = _simulation->PlaceBuilding(parameters);
					Building& burrow = _simulation->building(burrowId);
					burrow.FinishConstruction();

					static_cast<BoarBurrow*>(&burrow)->AddOccupant(_id);
					_houseId = burrowId;
				}
			}

		
		}

		if (TryGoNearbyHome()) {
			return;
		}

		// Idle, nothing else to do
		_unitState = UnitState::Idle;
		int32 waitTicks = 60 * (GameRand::Rand() % 3 + 4);
		Add_Wait(waitTicks);
		Add_MoveRandomly();
		AddDebugSpeech("(Success)Idle MoveRandomly");
	}
}

bool UnitStateAI::IsBurrowBuildable(TileArea area, Direction faceDirection)
{
	bool canPlace = true;
	if (area.isInMap()) {
		area.ExecuteOnArea_WorldTile2([&](WorldTile2 areaTile) {
			if (!_simulation->IsBuildable(areaTile)) canPlace = false;
		});
	}
	else {
		canPlace = false;
	}

	// Front Grid
	TileArea frontArea = area.GetFrontArea(faceDirection);
	if (frontArea.isInMap()) {
		frontArea.ExecuteOnArea_WorldTile2([&](WorldTile2 areaTile) {
			if (!_simulation->IsBuildable(areaTile)) canPlace = false;
		});
	}
	else {
		canPlace = false;
	}
	return canPlace;
}

bool UnitStateAI::TryCheckBadTile()
{
	WorldTile2 tile = unitTile();
	PunAStar128x256* pathAI = _simulation->pathAI(IsIntelligentUnit(unitEnum()));

	// Force move in case the unit is on bad tile
	if (!pathAI->isWalkable(tile.x, tile.y))
	{
		// Just spiral out trying to find isWalkable tile...
		int32 x = 0;
		int32 y = 0;
		int32 dx = 0;
		int32 dy = -1;
		WorldTile2 end;

		int32 loop;
		for (loop = 10000; loop-- > 0;) 
		{
			WorldTile2 curTile(x + tile.x, y + tile.y);
			if (curTile.isValid() && pathAI->isWalkable(curTile.x, curTile.y)) {
				end = curTile;
				break;
			}
			if ((x == y) || ((x < 0) && (x == -y)) || ((x > 0) && (x == 1 - y))) {
				int32_t temp = dx;
				dx = -dy;
				dy = temp;
			}
			x += dx;
			y += dy;

			//i++;
			//if (i > 10000) {
			//	UE_DEBUG_BREAK();
			//}
		}

		check(loop > 1);

		// move there ignoring obstacles
		Add_MoveToRobust(end);
		AddDebugSpeech("(Succeed)TryCheckBadTile:");
		return true;
	}
	AddDebugSpeech("(Failed)TryCheckBadTile:");
	return false;
}

bool UnitStateAI::TryGoNearbyHome()
{
	int32 homeId = houseId();
	if (homeId == -1) {
		// Human has townhall as backup
		if (isEnum(UnitEnum::Human)) {
			homeId = _simulation->townhall(_playerId).buildingId();
		}
		else {
			AddDebugSpeech("(Failed)TryGoNearbyHome: No Home");
			return false;
		}
	}

	WorldTile2 houseGate = _simulation->gateTile(homeId);

	DEBUG_ISCONNECTED_VAR(TryGoNearbyHome);
	if (_simulation->IsConnected(unitTile(), houseGate, 0, IsIntelligentUnit(unitEnum()))) {
		AddDebugSpeech("(Failed)TryGoNearbyHome: already same flood region as home");
		return false;
	}

	// If too far from home humans will force themself to go home...
	if (isEnum(UnitEnum::Human))
	{
		// Trailer
		if (PunSettings::TrailerSession) {
			// Too far, in another city, don't walk there and walk randomly instead
			if (WorldTile2::Distance(unitTile(), houseGate) > 80) {
				MoveRandomly();
				return true;
			}
		}


		MoveTo_NoFail(houseGate, GameConstants::MaxFloodDistance_HumanLogistics);
		AddDebugSpeech("(Succeed)TryGoNearbyHome:");
		return true;
	}
	// In this case, animal will just abandon home
	else if (IsWildAnimalWithHome(unitEnum())) 
	{
		if (!IsMoveValid(houseGate))
		{
			// Abandon home only if the home is not ranch..
			if (_simulation->building(homeId).isEnum(CardEnum::BoarBurrow)) {
				_simulation->building(homeId).subclass<BoarBurrow>().RemoveOccupant(_id);
				_houseId = -1;
			}
			AddDebugSpeech("(Failed)TryGoNearbyHome: Abandon home");
			return false;
		}
	}
	else if (IsAnimal(unitEnum())) {
		// Don't break...
	} else {
		UE_DEBUG_BREAK();
	}

	Add_MoveTo(houseGate);
	AddDebugSpeech("(Succeed)TryGoNearbyHome:");
	return true;
}

bool UnitStateAI::TryGetBurrowFood()
{
	PUN_CHECK(_houseId != -1);

	// TODO: Central system to manage all house-occupant relationship...
	BoarBurrow& boarBurrow = _simulation->building(_houseId).subclass<BoarBurrow>();

	ResourceEnum availableFoodEnum = ResourceEnum::None;

	boarBurrow.inventory.Execute_WithBreak([&](ResourcePair& resource) {
		if (resource.count > 0) {
			availableFoodEnum = resource.resourceEnum;
			return true;
		}
		return false;
	});

	if (availableFoodEnum == ResourceEnum::None) {
		AddDebugSpeech("(Failed)TryGetBurrowFood: No Food");
		return false;
	}

	// Lost Home
	if (!IsMoveValid(boarBurrow.gateTile())) {
		if (boarBurrow.isEnum(CardEnum::BoarBurrow)) {
			AddDebugSpeech("(Failed)TryGetBurrowFood: burrow move invalid ... lost home");
			boarBurrow.RemoveOccupant(_id);
			_houseId = -1;
		} else {
			AddDebugSpeech("(Failed)TryGetBurrowFood: ranch move invalid ...");
		}
		return false;
	}

	Add_Eat(availableFoodEnum);
	Add_Wait(60);
	Add_PickupFoodAnimal();
	Add_MoveTo(boarBurrow.gateTile());
	_unitState = UnitState::GetFood;

	AddDebugSpeech("(Succeed)TryGetBurrowFood:");
	return true;
}

bool UnitStateAI::TryStockBurrowFood()
{
	if (_houseId == -1) {
		AddDebugSpeech("(Failed)TryStockBurrowFood: No Home");
		return false;
	}

	// Don't store too much
	BoarBurrow& boarBurrow = _simulation->building(_houseId).subclass<BoarBurrow>();
	if (boarBurrow.inventory.resourceCountAll() > 100) {
		AddDebugSpeech("(Failed)TryStockBurrowFood: No Need");
		return false;
	}

	// Lost Home
	if (!IsMoveValid(boarBurrow.gateTile())) {
		if (boarBurrow.isEnum(CardEnum::BoarBurrow)) {
			AddDebugSpeech("(Failed)TryStockBurrowFood: burrow move invalid ... lost home");
			boarBurrow.RemoveOccupant(_id);
			_houseId = -1;
		}
		else {
			AddDebugSpeech("(Failed)TryStockBurrowFood: ranch move invalid ...");
		}
		return false;
	}

	

	auto& provinceSys = _simulation->provinceSystem();
	
	WorldTile2 fullBushTile = treeSystem().FindNearestUnreservedFullBush(unitTile(), provinceSys.GetRegionOverlaps(_homeProvinceId), GameConstants::MaxFloodDistance_AnimalFar, false);
	if (fullBushTile.isValid()) 
	{
		PUN_CHECK2(treeSystem().isBushReadyForHarvest(fullBushTile.tileId()), "readyForHarvest:" + to_string(treeSystem().isBushReadyForHarvest(fullBushTile.tileId())) + 
																			" fullBushTile:" + fullBushTile.ToString() + "\n" + debugStr());

		ReserveTreeTile(fullBushTile.tileId());

		WorldTile2 gateTile = _simulation->building(_houseId).gateTile();

		Add_DropoffFoodAnimal();

		// Need this since we only checked unit-tree, not burrow-tree
		// MoveToHarder if IsConnected isn't true 
		// TODO: resolve this without MoveToRobust...
		DEBUG_ISCONNECTED_VAR(TryStockBurrowFood);
		if (_simulation->IsConnected(fullBushTile, gateTile, GameConstants::MaxFloodDistance_AnimalFar, IsIntelligentUnit(unitEnum()))) {
			Add_MoveTo(gateTile);
		} else {
			Add_MoveToRobust(gateTile);
		}

		int32 trimWaitTicks = 120;
		if (IsGrass(treeSystem().tileObjEnum(fullBushTile.tileId()))) {
			trimWaitTicks /= GrassToBushValue;
		}

		Add_TrimFullBush(fullBushTile);
		Add_Wait(trimWaitTicks);
		Add_MoveToward(fullBushTile.worldAtom2(), 4000); // 40000 or 0.4 fraction so that the unit won't step into unwalkable tile TODO: check this as 40000?
		Add_MoveTo(fullBushTile);

		_unitState = UnitState::GatherBush;
		AddDebugSpeech("(Success)TryStockBurrowFood:");
		debugFindFullBushSuccessCount++;
		return true;
	}

	Add_Wait(Time::TicksPerSecond * 5);
	
	AddDebugSpeech("(Failed)TryStockBurrowFood: nearestTree invalid");
	debugFindFullBushFailCount++;
	return false;
}

bool UnitStateAI::TryStoreAnimalInventory()
{
	if (_houseId == -1) {
		AddDebugSpeech("(Failed)TryStoreAnimalInventory: No Home");
		return false;
	}
	
	// Free inventory
	if (!_inventory.hasResource()) {
		AddDebugSpeech("(Failed)TryStoreAnimalInventory: No Inventory");
		return false;
	}

	WorldTile2 houseTile = _simulation->building(_houseId).gateTile();

	// May be home is no longer valid, in that case abandon it...
	if (!IsMoveValid(houseTile)) 
	{
		// Abandon home if it is burrow.. (don't abandon ranch)
		if (_simulation->building(_houseId).isEnum(CardEnum::BoarBurrow)) {
			_simulation->building(_houseId).subclass<BoarBurrow>().RemoveOccupant(_id);
			_houseId = -1;
		}
		AddDebugSpeech("(Failed)TryStoreAnimalInventory: Home not connected.. Abandon home");
		return false;
	}

	Add_DropoffFoodAnimal();
	Add_MoveTo(houseTile);

	_simulation->soundInterface()->SpawnAnimalSound(unitEnum(), false, unitAtom());

	AddDebugSpeech("(Success)TryStoreAnimalInventory:");
	return true;
}

bool UnitStateAI::TryGoHomeProvince()
{
	if (_homeProvinceId == -1) {
		AddDebugSpeech("(Failed)TryGoHomeProvince: No Home province");
		return false;
	}

	if (_simulation->GetProvinceIdClean(unitTile()) == _homeProvinceId)
	{
		AddDebugSpeech("(Failed)TryGoHomeProvince: Already at home province");
		return false;
	}

	const int tries = 10;
	WorldTile2 uTile = unitTile();
	WorldTile2 end = _simulation->GetProvinceRandomTile(_homeProvinceId, uTile, GameConstants::MaxFloodDistance_AnimalFar, false, tries);

	// Whatever, just wait a bit to try again..
	if (end == WorldTile2::Invalid) {
		AddDebugSpeech("(Failed)TryGoHomeProvince: Can't find walkable spot in 10 tiles");
		return false;
	}

	AddDebugSpeech("(Success)TryGoHomeProvince: Transfer to MoveTo " + uTile.ToString() + end.ToString());
	Add_MoveTo(end, GameConstants::MaxFloodDistance_AnimalFar);
	return true;
}

void UnitStateAI::Add_DropoffFoodAnimal() {
	AddAction(ActionEnum::DropoffFoodAnimal);
}
void UnitStateAI::DropoffFoodAnimal()
{
	PUN_CHECK2(_inventory.hasResource(), debugStr());
	BoarBurrow& boarBurrow = _simulation->building(_houseId).subclass<BoarBurrow>();

	// TODO: resolve why Unit didn't get reset before this, when building dies
	if (!_simulation->buildingIsAlive(_houseId)) {
		AddDebugSpeech("(Failed)DropoffFoodAnimal: Building died somehow...");
		_simulation->ResetUnitActions(_id);

		PUN_LOG("DropoffFoodAnimal: Building died somehow...");
		return;
	}

	_inventory.Execute([&](ResourcePair& resource) {
		boarBurrow.inventory.Add(resource);
	});
	_inventory.Clear();

	PUN_CHECK2(boarBurrow.inventory.CheckIngrity(), debugStr());

	//int32_t amount = _inventory.Amount(ResourceEnum::Orange);

	////PUN_CHECK2(_simulation->buildingIsAlive(_houseId), debugStr());
	//boarBurrow->resources[ResourceEnum::Orange] += amount;
	//_inventory.Remove(ResourcePair(ResourceEnum::Orange, amount));
	
	NextAction(UnitUpdateCallerEnum::DropoffFoodAnimal);
	AddDebugSpeech("(Done)DropoffFoodAnimal:");
}

void UnitStateAI::Add_PickupFoodAnimal() {
	AddAction(ActionEnum::PickupFoodAnimal);
}
void UnitStateAI::PickupFoodAnimal()
{
	check(_houseId != -1);
	BoarBurrow& boarBurrow = _simulation->building(_houseId).subclass<BoarBurrow>();

	boarBurrow.inventory.Execute_WithBreak([&](ResourcePair& resource) 
	{
		if (resource.count > 0) {
			int32_t consumption = min(10, resource.count);
			ResourcePair resourceConsumed(resource.resourceEnum, consumption);

			boarBurrow.inventory.Remove(resourceConsumed);
			_inventory.Add(resourceConsumed);
			return true;
		}
		return false;
	});

	PUN_CHECK2(boarBurrow.inventory.CheckIngrity(), debugStr());

	//for (int i = 0; i < boarBurrow.resources.size(); i++) {
	//	if (boarBurrow.resources[i].count > 0) {
	//		ResourcePair& resource = boarBurrow.resources[i];
	//		PUN_CHECK2(resource.count >= 0, debugStr());

	//		int32_t consumption = min(10, resource.count);
	//		resource.count -= consumption;

	//		_inventory.Add(ResourcePair(resource.resourceEnum, consumption));
	//		break;
	//	}
	//}

	NextAction(UnitUpdateCallerEnum::PickupFoodAnimal);
	AddDebugSpeech("(Done)PickupFoodAnimal:");
}

bool UnitStateAI::TryAvoidOthers()
{
	const int32 unitCountToAvoid = 3;
	int32 subregionUnitCount = _unitData->unitSubregionLists().SubregionCount(unitTile());
	if (subregionUnitCount >= unitCountToAvoid) 
	{
		_unitState = UnitState::AvoidOthers;
		Add_MoveRandomly();
		AddDebugSpeech("(Success)AvoidOthers MoveRandomly");
		return true;
	}
	return false;
}

bool UnitStateAI::TryFindWildFood()
{
	SCOPE_CYCLE_COUNTER(STAT_PunUnitCalcFindWildFood);
	
	// Go get food only if 3/4 of stomach full
	if (_food >= maxFood() * 3 / 4) {
		AddDebugSpeech("(Failed)TryFindWildFood: Not needed");
		return false;
	}

	auto& provinceSys = _simulation->provinceSystem();

	/*
	 * Find Wild Food
	 */
	if (_homeProvinceId != -1)
	{
		TreeSystem& treeSys = treeSystem();
		const std::vector<WorldRegion2>& regions = provinceSys.GetRegionOverlaps(_homeProvinceId);

		bool shouldFindWildFoodAgain = false;
		for (WorldRegion2 region : regions) {
			if (treeSys.regionLastBushUpdateTick(region) > _lastFindWildFoodTick) {
				shouldFindWildFoodAgain = true;
				break;
			}
		}

		if (shouldFindWildFoodAgain)
		{
			WorldTile2 bushTile = treeSys.FindNearestUnreservedFullBush(unitTile(), regions, GameConstants::MaxFloodDistance_Animal, IsIntelligentUnit(unitEnum()));
			_lastFindWildFoodTick = Time::Ticks();

			if (bushTile.isValid())
			{
				ReserveTreeTile(bushTile.tileId());

				int32 trimWaitTicks = 360;
				if (IsGrass(treeSys.tileObjEnum(bushTile.tileId()))) {
					trimWaitTicks /= GrassToBushValue;
				}

				Add_Eat(treeSys.tileInfo(bushTile.tileId()).harvestResourceEnum());
				Add_Wait(30);
				Add_TrimFullBush(bushTile);
				Add_Wait(trimWaitTicks);
				Add_MoveToward(bushTile.worldAtom2(), 5000);
				Add_MoveTo(bushTile);

				_unitState = UnitState::GetWildFood;

				_simulation->soundInterface()->SpawnAnimalSound(unitEnum(), false, unitAtom());

				AddDebugSpeech("(Success)TryFindWildFood: Bush");
				debugFindFullBushSuccessCount++;
				return true;
			}
		}
	}

	/*
	 * Migration
	 * Can't find food, look to migrate to the province with least animals
	 */
	if (IsWildAnimalNoHome(_unitEnum)) {
		TryChangeProvince_NoAction();
	}

	Add_Wait(Time::TicksPerSecond * 5);

	AddDebugSpeech("(Failed)TryFindWildFood: nearestTree invalid");
	debugFindFullBushFailCount++;
	return false;
}

bool UnitStateAI::TryChangeProvince_NoAction()
{
	if (_homeProvinceId != -1)
	{
		int32 leastAnimalProvinceId = -1;
		int32 leastAnimalCount = _simulation->provinceAnimals(_homeProvinceId).size();

		const std::vector<ProvinceConnection>& connections = _simulation->GetProvinceConnections(_homeProvinceId);
		for (ProvinceConnection connection : connections)
		{
			if (connection.tileType == TerrainTileType::None &&
				_simulation->IsProvinceValid(connection.provinceId))
			{
				int32 animalCount = _simulation->provinceAnimals(connection.provinceId).size();
				if (animalCount < leastAnimalCount) {
					leastAnimalCount = animalCount;
					leastAnimalProvinceId = connection.provinceId;
				}
			}
		}

		if (leastAnimalProvinceId != -1)
		{
			_simulation->RemoveProvinceAnimals(_homeProvinceId, _id);
			_homeProvinceId = leastAnimalProvinceId;
			_simulation->AddProvinceAnimals(_homeProvinceId, _id);

			_lastFindWildFoodTick = 0;
			return true;
		}
	}
	return false;
}

/*
 * Actions
 */

void UnitStateAI::ResetActions_UnitPart(int32 waitTicks)
{
	// Last reset ticks debug
	DEBUG_AI_VAR(ResetCountPerSec);
	//static int32 _lastSec_AA = 0;
	//static int32 _lastSecCount_AA = 0;
	//if (Time::Seconds() > _lastSec_AA) {
	//	_lastSec_AA = Time::Seconds();
	//	Debug_ResetCountPerSec = _lastSecCount_AA;
	//	_lastSecCount_AA = 0;
	//}
	//_lastSecCount_AA++;
	

	// Reset
	CancelReservations();
	_actions.clear();
;
	// TODO: note setting waitTicks for nextTickState break the game..
	_unitData->SetNextTickState(_id, TransformState::NeedActionUpdate, UnitUpdateCallerEnum::ResetActions, 1, true);

	
	Add_Wait(waitTicks);

	_justDidResetActions = true;
	
	AddDebugSpeech("[RESET] ... workplace:" + to_string(workplaceId()));
}

void UnitStateAI::Add_Wait(int32 tickCount, UnitAnimationEnum animationEnum) {
	_actions.push_back(Action(ActionEnum::Wait, tickCount, static_cast<int32>(animationEnum), 0));
}
void UnitStateAI::Wait()
{
	int32 tickCount = action().int32val1;
	_animationEnum = static_cast<UnitAnimationEnum>(action().int32val2);
	
	PUN_DEBUG_EXPR(CheckIntegrity());
	PUN_CHECK2(tickCount < Time::TicksPerMinute, ("badTickCount:" + to_string(tickCount) + debugStr()));

	// TODO: proper repeating cut sound
	// Play Sound
	if (_unitState == UnitState::GatherTree || _unitState == UnitState::ClearLandCutTree) {
		_simulation->soundInterface()->Spawn3DSound("CitizenAction", "TreeChopping", unitAtom());
	}
	else if (_unitState == UnitState::GatherStone || _unitState == UnitState::ClearLandCutStone) {
		_simulation->soundInterface()->Spawn3DSound("CitizenAction", "StonePicking", unitAtom());
	}
	

	NextAction(tickCount, UnitUpdateCallerEnum::Wait);
	AddDebugSpeech("Wait: Done tickCount:" + to_string(tickCount));
}


void UnitStateAI::Add_GatherFruit(WorldTile2 targetTile) {
	_actions.push_back(Action(ActionEnum::GatherFruit, targetTile.tileId(), 0, 0));
}
void UnitStateAI::GatherFruit()
{
	WorldTile2 targetTile(action().int32val1);
	
	UnitReservation reservation = PopReservation(ReservationType::TreeTile);
	PUN_CHECK2(reservation.reserveTileId == targetTile.tileId(), debugStr());

	ResourcePair resourcePair100 = _simulation->treeSystem().UnitGatherFruit100(targetTile);

	// Tree dead by the time arrived
	if (!resourcePair100.isValid()) {
		_simulation->ResetUnitActions(id());
		return;
	}

	// Berry Gatherer's bonus
	if (workplace() && workplace()->isEnum(CardEnum::FruitGatherer)) 
	{
		resourcePair100.count = GameRand::Rand100RoundTo1(resourcePair100.count * workplace()->efficiency());

		if (workplace()->workMode().name.IdenticalTo(MeticulousWorkModeText)) {
			resourcePair100.count = GameRand::Rand100RoundTo1(resourcePair100.count * 130);
		}
	}

	ResourcePair resourcePair = resourcePair100;
	resourcePair.count /= 100;

	_inventory.Add(resourcePair);


	if (resourcePair.isValid()) {
		_simulation->uiInterface()->ShowFloatupInfo(FloatupEnum::GainResource, targetTile, TEXT_NUMSIGNED(resourcePair.count), resourcePair.resourceEnum);
	}

	// AddStatistics done on drop-poff

	NextAction(UnitUpdateCallerEnum::GatherFruit);
	AddDebugSpeech("(Done)GatherFruit:");
}

void UnitStateAI::Add_TrimFullBush(WorldTile2 targetTile) {
	_actions.push_back(Action(ActionEnum::TrimFullBush, targetTile.tileId(), 0, 0));
}
void UnitStateAI::TrimFullBush()
{
	/*
	 * Animal trimming bush..
	 */
	WorldTile2 targetTile(action().int32val1);
	
	UnitReservation reservation = PopReservation(ReservationType::TreeTile);
	PUN_CHECK2(reservation.reserveTileId == targetTile.tileId(), debugStr());

	auto& treeSystem = _simulation->treeSystem();

	PUN_DEBUG_EXPR(std::string beforeCut = treeSystem.tileObjdebugStr(targetTile.tileId()));

	int32 trimEfficiency = 70; // Animals are not efficient at gathering...
	ResourcePair resourcePair = treeSystem.AnimalTrimBush(targetTile, trimEfficiency);

	PUN_CHECK2_EDITOR(resourcePair.count > 0, beforeCut + treeSystem.tileObjdebugStr(targetTile.tileId()) + debugStr());

	_inventory.Add(resourcePair);

	// If this is human's farm, warn them...
	int32 tileBuildingId = _simulation->buildingIdAtTile(targetTile);
	if (tileBuildingId != -1)
	{
		Building& tileBuilding = _simulation->building(tileBuildingId);
		if (tileBuilding.isConstructed(CardEnum::Farm))
		{
			int32 tilePlayerId = tileBuilding.playerId();
			if (_simulation->TryDoNonRepeatAction(tilePlayerId, NonRepeatActionEnum::AnimalsEatingCrop, Time::TicksPerSecond * 30)) {
				_simulation->AddEventLog(tilePlayerId, 
					LOCTEXT("AnimalEatCrop_Event", "Animals are eating your crop..."), 
					true
				);

				auto unlockSys = _simulation->unlockSystem(tilePlayerId);
				if (!unlockSys->didFirstTimeAnimalRavage) {
					unlockSys->didFirstTimeAnimalRavage = true;
					_simulation->AddPopup(tilePlayerId, 
						LOCTEXT("AnimalsEatCrop_Pop", "Animals are eating your crops.<space>One way to deal with this is to build a Hunting Lodge near Farms.<space>For faster animal killing, change Hunting Lodge's Workmode to \"Poison Arrow\".")
					);
				}
			}
		}
	}

	NextAction(UnitUpdateCallerEnum::TrimFullBush);
	AddDebugSpeech("(Done)TrimFullBush:");
}

void UnitStateAI::Add_HarvestTileObj(WorldTile2 targetTile) {
	_actions.push_back(Action(ActionEnum::HarvestTileObj, targetTile.tileId()));
}
void UnitStateAI::HarvestTileObj()
{
	WorldTile2 targetTile(action().int32val1);
	
	UnitReservation reservation = PopReservation(ReservationType::TreeTile);
	PUN_CHECK2(reservation.reserveTileId == targetTile.tileId(), debugStr());

	// Get fruit first
	ResourcePair resourcePairFruit;
	if (_simulation->treeSystem().hasFruit(targetTile.tileId())) {
		resourcePairFruit = _simulation->treeSystem().UnitGatherFruit100(targetTile);
		resourcePairFruit.count /= 200; // Gather by chopping tree lead to less fruit /2, and also get rid of 100 with /100
	}

	ResourcePair resourcePair = _simulation->treeSystem().HumanHarvest(targetTile);

	// Discard hay
	if (resourcePair.resourceEnum != ResourceEnum::Hay)
	{
		// Wood Cutting Bonus
		if (_playerId != -1 && resourcePair.isEnum(ResourceEnum::Wood))
		{
			auto unlockSys = _simulation->unlockSystem(_playerId);
			int32 efficiency = 100;
			if (unlockSys->IsResearched(TechEnum::ImprovedWoodCutting2)) {
				efficiency += 20;
			}
			else if (unlockSys->IsResearched(TechEnum::ImprovedWoodCutting)) {
				efficiency += 20;
			}

			if (workplace() && workplace()->isEnum(CardEnum::Forester)) {
				efficiency += std::max(0, workplace()->efficiency() - 100);
			}

			resourcePair.count = GameRand::Rand100RoundTo1(resourcePair.count * efficiency);
		}

		_inventory.Add(resourcePair);

		if (resourcePairFruit.isValid()) {
			_inventory.Add(resourcePairFruit);
		}

		// Wood cutting stat
		if (workplace() && workplace()->isEnum(CardEnum::Forester)) {
			workplace()->subclass<Forester>().AddProductionStat(resourcePair.count);
		}
		else {
			_simulation->statSystem(_playerId).AddResourceStat(ResourceSeasonStatEnum::Production, resourcePair.resourceEnum, resourcePair.count);

			if (resourcePairFruit.isValid()) {
				_simulation->statSystem(_playerId).AddResourceStat(ResourceSeasonStatEnum::Production, resourcePairFruit.resourceEnum, resourcePairFruit.count);
			}
		}

		FloatupInfo floatupInfo(FloatupEnum::GainResource, Time::Ticks(), targetTile, TEXT_NUMSIGNED(resourcePair.count), resourcePair.resourceEnum);

		if (resourcePairFruit.isValid()) {
			floatupInfo.resourceEnum2 = resourcePairFruit.resourceEnum;
			floatupInfo.text2 = TEXT_NUMSIGNED(resourcePairFruit.count);
		}
		
		_simulation->uiInterface()->ShowFloatupInfo(floatupInfo);
	}

	NextAction(UnitUpdateCallerEnum::HarvestTileObj);
	AddDebugSpeech("(Done)HarvestTileObj: enum:" + ResourceName(resourcePair.resourceEnum) + " amount:" + to_string(_inventory.Amount(resourcePair.resourceEnum)));
}

void UnitStateAI::Add_PlantTree(WorldTile2 targetTile, TileObjEnum tileObjEnum) {
	_actions.push_back(Action(ActionEnum::PlantTree, targetTile.tileId(), static_cast<int32>(tileObjEnum)));
}
void UnitStateAI::PlantTree()
{
	WorldTile2 targetTile(action().int32val1);
	TileObjEnum tileObjEnum = static_cast<TileObjEnum>(action().int32val2);
	
	UnitReservation reservation = PopReservation(0);
	PUN_CHECK2(reservation.reserveTileId == targetTile.tileId(), debugStr());

	// Pop all other reservations
	for (int i = reservations.size(); i-- > 0;) {
		UnitReservation reservationTemp = PopReservation(i);
		PUN_CHECK2(reservationTemp.reservationType == ReservationType::TreeTile, debugStr());
	}

	if (treeSystem().CanPlantTreeOrBush(targetTile)) {
		treeSystem().PlantTree(targetTile.x, targetTile.y, tileObjEnum);
	}

	NextAction(UnitUpdateCallerEnum::PlantTree);
	AddDebugSpeech("(Done)PlantTree: enum:" + GetTileObjInfo(tileObjEnum).nameStr());
}

void UnitStateAI::Add_NourishTree(WorldTile2 targetTile) {
	_actions.push_back(Action(ActionEnum::NourishTree, targetTile.tileId()));
}
void UnitStateAI::NourishTree() {
	WorldTile2 targetTile(action().int32val1);
	treeSystem().UnitNourishTree(targetTile);

	NextAction(UnitUpdateCallerEnum::NourishTree);
	AddDebugSpeech("(Done)NourishTree: enum:" + targetTile.ToString());
}

void UnitStateAI::Add_Eat(ResourceEnum resourceEnum) {
	AddAction(ActionEnum::Eat, static_cast<int32>(resourceEnum));
}
void UnitStateAI::Eat()
{
	ResourceEnum resourceEnum = static_cast<ResourceEnum>(action().int32val1);
	
	// Animal has no reservation and there may not be any food left...
	if (_inventory.Has(resourceEnum)) 
	{
		int32 amount = _inventory.Amount(resourceEnum);
		int32 foodIncrement = amount * unitInfo().foodTicksPerResource;

		// Food consumption changes
		if (isEnum(UnitEnum::Human)) {
			int32 foodConsumptionModifier = _simulation->difficultyConsumptionAdjustment(_playerId);

			foodIncrement = foodIncrement * 100 / (100 + foodConsumptionModifier);

			// Special case:
			if (resourceEnum == ResourceEnum::Melon) {
				_simulation->ChangeMoney(_playerId, 3);
				_simulation->uiInterface()->ShowFloatupInfo(FloatupEnum::GainMoney, unitTile(), INVTEXT("+3"), resourceEnum);
			}
		}
		else
		{	
			// Animals with biome gets eating penalty out of its own biome
			std::vector<BiomeEnum> biomes = unitInfo().biomes;
			if (biomes.size() > 0)
			{
				BiomeEnum biomeEnum = _simulation->GetBiomeEnum(unitTile());
				if (find(biomes.begin(), biomes.end(), biomeEnum) == biomes.end())  {
					foodIncrement /= 2; // half food intake...
				}
			}

			static int32 animalNoHome = 0;
			static int32 animaWithHome = 0;

			//if (IsWildAnimalNoHome(_unitEnum)) {
			//	animalNoHome++;
			//	if (animalNoHome % 10 == 0) PUN_LOG("NoHome animal eat:%d", animalNoHome);
			//} else {
			//	animaWithHome++;
			//	if (animaWithHome % 10 == 0) PUN_LOG("animal - WithHome eat:%d", animaWithHome);
			//}

			// Animals with low count 
			int32 animalCountPercent = _simulation->unitEnumCount(_unitEnum) * 100 / max(1, _simulation->animalInitialCount(_unitEnum));
			if (animalCountPercent > 150) {
				foodIncrement /= 7;
			}
			if (animalCountPercent > 120) {
				foodIncrement /= 3;
			}
			if (animalCountPercent < 90) {
				foodIncrement *= 3;
			}
			if (animalCountPercent < 60) {
				foodIncrement *= 7;
			}
		}
		
		_food += foodIncrement;
		
		_simulation->statSystem(_playerId).AddResourceStat(ResourceSeasonStatEnum::Consumption, resourceEnum, amount);
		_inventory.Remove(ResourcePair(resourceEnum, amount));

		NextAction(UnitUpdateCallerEnum::Eat_Done);
		AddDebugSpeech("(Done)Eat:");
		return;
	}

	NextAction(UnitUpdateCallerEnum::Eat_SomeoneElseTookFood);
	AddDebugSpeech("(Failed)Eat: Someone else took the food.");
}

void UnitStateAI::Add_Heat() {
	AddAction(ActionEnum::Heat);
}
void UnitStateAI::Heat()
{
	//ResourceEnum resourceEnum = _inventory.Has(ResourceEnum::Wood) ? ResourceEnum::Wood : ResourceEnum::Coal;
	PUN_CHECK2(_inventory.Has(ResourceEnum::Wood) || _inventory.Has(ResourceEnum::Coal), debugStr());

	auto tryHeatWith = [&](ResourceEnum resourceEnum)
	{
		if (_inventory.Has(resourceEnum))
		{
			int32 amount = _inventory.Amount(resourceEnum);
			statSystem().AddResourceStat(ResourceSeasonStatEnum::Consumption, resourceEnum, amount);
			_inventory.Remove(ResourcePair(resourceEnum, amount));
			
			int32 heatToAdd = amount * HeatPerResource_CelsiusTicks();
			if (_playerId == 0) {
				PUN_LOG("amount:%d heatToAdd:%d celsiusSec:%d", amount, heatToAdd, heatToAdd/Time::TicksPerSecond);
			}

			heatToAdd = heatToAdd * 100 / (100 + _simulation->difficultyConsumptionAdjustment(_playerId));
		
			
			if (_houseId != -1) {
				House& house = _simulation->building<House>(_houseId, CardEnum::House);
				int32 heatingEfficiency = house.GetHeatingEfficiency(resourceEnum);
				heatToAdd += heatToAdd * heatingEfficiency / 100;
			} else {
				if (resourceEnum == ResourceEnum::Coal) {
					heatToAdd *= 2;
				}
			}

			_heat += heatToAdd;
		}
	};

	tryHeatWith(ResourceEnum::Wood);
	tryHeatWith(ResourceEnum::Coal);

	NextAction(UnitUpdateCallerEnum::Heat);
	AddDebugSpeech("(Done)Heat:");
}

void UnitStateAI::Add_UseMedicine(ResourceEnum resourceEnum) {
	AddAction(ActionEnum::UseMedicine, static_cast<int32>(resourceEnum));
}
void UnitStateAI::UseMedicine()
{
	ResourceEnum resourceEnum = static_cast<ResourceEnum>(action().int32val1);
	
	PUN_CHECK2(_inventory.Has(resourceEnum), debugStr());
	PUN_CHECK(resourceEnum == ResourceEnum::Herb || resourceEnum == ResourceEnum::Medicine);

	int32 amount = resourceEnum == ResourceEnum::Herb ? 2 : 1;
	amount = min(_inventory.Amount(resourceEnum), amount);
	
	_isSick = false;
	_hp100 = 10000;

	statSystem().AddResourceStat(ResourceSeasonStatEnum::Consumption, resourceEnum, amount);
	
	_inventory.Remove(ResourcePair(resourceEnum, amount));

	NextAction(UnitUpdateCallerEnum::Heal);
	AddDebugSpeech("(Done)Heal:");
}
void UnitStateAI::Add_UseTools(ResourceEnum resourceEnum) {
	AddAction(ActionEnum::UseTools, static_cast<int32>(resourceEnum));
}
void UnitStateAI::UseTools()
{
	ResourceEnum resourceEnum = static_cast<ResourceEnum>(action().int32val1);

	PUN_CHECK2(_inventory.Has(resourceEnum), debugStr());
	PUN_CHECK(resourceEnum == ResourceEnum::SteelTools);

	/*
	 * Stone 8 + wood 5 + 2 ... 15 ... half year
		Iron ore 5 + wood 5 + 5 ... 15 ... 1 year
		Iron 18 + wood 5 + 7 ... 30 ... 3 years
	 */

	int32 amount = 1;
	//if (resourceEnum == ResourceEnum::StoneTools) {
	//	_nextToolNeedTick = Time::Ticks() + Time::TicksPerYear / 2; // Stone tool last half a year
	//}
	//else if (resourceEnum == ResourceEnum::CrudeIronTools) {
	//	_nextToolNeedTick = Time::Ticks() + Time::TicksPerYear; // Crude iron tool a year
	//}
	//else 
	if (resourceEnum == ResourceEnum::SteelTools) {
		int32 ticksUntilNextToolNeed = (Time::TicksPerYear * 2) + (GameRand::Rand() % Time::TicksPerYear * 2); // Steel tool 3 year;

		ticksUntilNextToolNeed = ticksUntilNextToolNeed * 100 / (100 + _simulation->difficultyConsumptionAdjustment(_playerId));
		
		_nextToolNeedTick = Time::Ticks() + ticksUntilNextToolNeed;
		//_nextToolNeedTick = Time::Ticks() + (Time::TicksPerYear * 2) + (GameRand::Rand() % Time::TicksPerYear * 2); // Steel tool 3 year
	}
	else {
		UE_DEBUG_BREAK();
	}

	statSystem().AddResourceStat(ResourceSeasonStatEnum::Consumption, resourceEnum, amount);
	_inventory.Remove(ResourcePair(resourceEnum, amount));

	NextAction(UnitUpdateCallerEnum::Tool);
	AddDebugSpeech("(Done)Tool:");
}


void UnitStateAI::Add_HaveFun(int32 funBuildingId) {
	AddAction(ActionEnum::HaveFun, funBuildingId);
}
void UnitStateAI::HaveFun()
{
	int32 funBuildingId = action().int32val1;
	
	Building& building = _simulation->building(funBuildingId);
	
	if (building.isEnum(CardEnum::Theatre) ||
		building.isEnum(CardEnum::Tavern)) 
	{
		FunBuilding& funBuilding = building.subclass<FunBuilding>();
		funBuilding.UseService();
		_funTicks = funBuilding.serviceQuality() * FunTicksAt100Percent / 100;
		
		AddDebugSpeech("(Done)HaveFun ... Theatre or Tavern");
	}
	else {
		// In some case, building may have been despawned or reused...
		// In rare case that this building is reuse as funBuilding, funBuilding will just gain extra reservation which will reset anyway. (won't cause issue)
		AddDebugSpeech("(Done)HaveFun ... Building Gone");
	}

	NextAction(UnitUpdateCallerEnum::HaveFun);
}

void UnitStateAI::Add_MoveRandomlyAnimal() {
	_actions.push_back(Action(ActionEnum::MoveRandomly));
}
void UnitStateAI::MoveRandomlyAnimal()
{
	SCOPE_CYCLE_COUNTER(STAT_PunUnitDoMoveRandomly);

	// TODO: make animals with home also use _homeProvinceId
	PUN_CHECK(_homeProvinceId != -1);
	
	const int tries = 10;
	WorldTile2 uTile = unitTile();
	WorldTile2 end = _simulation->GetProvinceRandomTile(_homeProvinceId, uTile, 1, false, tries);

	// Whatever, just wait a bit to try again..
	if (end == WorldTile2::Invalid) {
		// TODO: waitTicks doesn't work ?? Should work now?
		_simulation->ResetUnitActions(_id, Time::TicksPerSecond * 5);  // Can't walk, (No need for extra wait since there is already Wait() before MoveRandomly()
		AddDebugSpeech("(Bad)MoveRandomly: Can't find walkable spot in 10 tiles");
		return;
	}

	check(_simulation->IsConnected(uTile, end, 1, IsIntelligentUnit(unitEnum())));

	AddDebugSpeech("(Transfer)MoveRandomly: Transfer to MoveTo " + uTile.ToString() + end.ToString());
	MoveTo(end);
}

void UnitStateAI::Add_MoveRandomly(TileArea area) {
	_actions.push_back(Action(ActionEnum::MoveRandomly, area.minX, area.minY, area.maxX, area.maxY));
}
void UnitStateAI::MoveRandomly()
{
	SCOPE_CYCLE_COUNTER(STAT_PunUnitDoMoveRandomly);

	//UE_LOG(LogTemp, Error, TEXT("at %s"), *FString(pathEnd.ToString().c_str()));
	WorldTile2 tile = unitTile();
	PUN_CHECK2(tile.isValid(), debugStr());

	TileArea area;
	area.minX = action().int32val1;
	area.minY = action().int32val2;
	area.maxX = action().int32val3;
	area.maxY = action().int32val4;

	// No area specified
	if (!area.isValid()) 
	{
		// walk 10 tiles away..
		area = TileArea(tile, 10);
		area.EnforceWorldLimit();
	}

	const int tries = 10;
	WorldTile2 end = WorldTile2::Invalid;
	bool canWalk = false;
	for (int i = 0; i < tries; i++) 
	{
		DEBUG_ISCONNECTED_VAR(MoveRandomly);
		
		end = area.RandomTile();
		if (_simulation->IsConnected(tile, end, 1, IsIntelligentUnit(unitEnum()))) { // MoveRandomly is not critical, so give it just 1 flood range to make it less likely to overload PathAI
			AddDebugSpeech("(-)MoveRandomly: Found on first loop" + tile.ToString() + end.ToString());
			canWalk = true;
			break;
		}
	}

	// Just brute it
	if (!canWalk) {
		[&]{
			for (int16_t y = area.minY; y <= area.maxY; y++) { // Faster loop
				for (int16_t x = area.minX; x <= area.maxX; x++)
				{
					DEBUG_ISCONNECTED_VAR(JustBruteIt);
					
					end = WorldTile2(x, y);
					if (_simulation->IsConnected(tile, end, 1, IsIntelligentUnit(unitEnum()))) {
						AddDebugSpeech("(-)MoveRandomly: Can't find walkable spot in 10 tiles" + tile.ToString() + end.ToString());
						canWalk = true;
						return;
					}
				}
			}
		}();
	}

	// Whatever, just wait a bit to try again..
	if (!canWalk) {
		// TODO: waitTicks doesn't work.. 
		_simulation->ResetUnitActions(_id, Time::TicksPerSecond * 5);  // Can't walk, (No need for extra wait since there is already Wait() before MoveRandomly()
		AddDebugSpeech("(Bad)MoveRandomly: Can't find walkable spot in 10 tiles");
		return;
	}

	check(_simulation->IsConnected(tile, end, 1, IsIntelligentUnit(unitEnum())));

	AddDebugSpeech("(Transfer)MoveRandomly: Transfer to MoveTo " + tile.ToString() + end.ToString());
	MoveTo(end);
}

void UnitStateAI::Add_MoveRandomlyPerlin(TileArea area)
{
	_actions.push_back(Action(ActionEnum::MoveRandomlyPerlin, area.minX, area.minY, area.maxX, area.maxY));
}
void UnitStateAI::MoveRandomlyPerlin()
{
	TileArea area;
	area.minX = action().int32val1;
	area.minY = action().int32val2;
	area.maxX = action().int32val3;
	area.maxY = action().int32val4;

	WorldTile2 tile = unitTile();
	PUN_CHECK(tile.isValid());

	const int tries = 10;
	WorldTile2 end = WorldTile2::Invalid;
	bool canWalk = false;
	for (int i = 0; i < tries; i++) {
		end = area.RandomTile();

		DEBUG_ISCONNECTED_VAR(MoveRandomlyPerlin);
		
		if (_simulation->IsConnected(tile, end, 1, IsIntelligentUnit(unitEnum()))) { // MoveRandomly is not critical, so give it just 1 flood range to make it less likely to overload PathAI
			AddDebugSpeech("(-)MoveRandomlyPerlin: Found on first loop" + tile.ToString() + end.ToString());
			canWalk = true;
			break;
		}
	}

	if (canWalk) {
		AddDebugSpeech("(Transfer)MoveRandomlyPerlin: Transfer to MoveTo() " + tile.ToString() + end.ToString());
		MoveTo(end);
	} else {
		NextAction(UnitUpdateCallerEnum::MoveRandomlyPerlin_Failed);
	}
}


void UnitStateAI::Add_MoveTo(WorldTile2 end, int32 customFloodDistance) {
	_actions.push_back(Action(ActionEnum::MoveTo, end.tileId(), customFloodDistance));
}
void UnitStateAI::MoveTo() {
	MoveTo(WorldTile2(action().int32val1), action().int32val2);
}
bool UnitStateAI::MoveTo(WorldTile2 end, int32 customFloodDistance)
{
	SCOPE_CYCLE_COUNTER(STAT_PunUnitDoMoveTo);
	WorldTile2 tile = unitTile();

	// Trailer
	if (PunSettings::TrailerSession)
	{
		// Too far, in another city, don't walk there and walk randomly instead
		if (WorldTile2::Distance(tile, end) > 80) {
			Add_Wait(Time::TicksPerSecond);
			Add_MoveRandomly();
			return true;
		}
	}

	// Possibly something got in the way before this was called...
	// This should also guard against unit's tile becoming invalid (construction built on top)
	if (!IsMoveValid(end, customFloodDistance)) 
	{
		_simulation->ResetUnitActions(_id, 60);
		AddDebugSpeech("(Bad)MoveTo: IsConnected " + tile.ToString() + end.ToString());

		//PUN_LOG("MoveTo IsConnected failed %s", *ToFString(debugStr()));

		_simulation->DrawLine(tile.worldAtom2(), FVector(0, 0, 0), tile.worldAtom2(), FVector(0, 0, 20), FLinearColor(1, 0.9, 0.9));
		_simulation->DrawLine(tile.worldAtom2(), FVector(0, 0, 20), end.worldAtom2(), FVector(0, 0, 20), FLinearColor(1, 0, 1));
		_simulation->DrawLine(end.worldAtom2(), FVector(0, 0, 0), end.worldAtom2(), FVector(0, 0, 20), FLinearColor(1, 0, 1));
		return false;
	}

	//int32 

	bool isIntelligent = IsIntelligentUnit(unitEnum());

	int64 customCalculationCount = 30000;
	int64 distance = WorldTile2::Distance(tile, end);
	const int64 baseDistance = 70;
	if (distance > baseDistance) {
		customCalculationCount = 30000 * distance * distance / (baseDistance * baseDistance);
	}
	
	bool succeed = _simulation->pathAI(isIntelligent)->FindPath(tile.x, tile.y, end.x, end.y, rawWaypoint, isEnum(UnitEnum::Human), isIntelligent, customCalculationCount);

	if (!succeed) 
	{
		DEBUG_AI_VAR(FailedToFindPath);
		
		_simulation->ResetUnitActions(_id, 60);
		AddDebugSpeech("(Bad)MoveTo: " + tile.ToString() + end.ToString());

		//PUN_LOG("MoveTo FindPath failed %s", *ToFString(debugStr()));

		_simulation->DrawLine(tile.worldAtom2(), FVector(0, 0, 0), tile.worldAtom2(), FVector(0, 0, 40), FLinearColor(1, 0.9, 0.9));
		_simulation->DrawLine(tile.worldAtom2(), FVector(0, 0, 40), end.worldAtom2(), FVector(0, 0, 40), FLinearColor::Red);
		_simulation->DrawLine(end.worldAtom2(), FVector(0, 0, 0), end.worldAtom2(), FVector(0, 0, 40), FLinearColor::Red);
		return false;
	}

	//PUN_LOG("MoveTo() UnitAnimationEnum::Walk id:%d", _id);

	_animationEnum = UnitAnimationEnum::Walk;

	MapUtil::UnpackAStarPath(rawWaypoint, _unitData->waypoint(_id));
	_unitData->SetForceMove(_id, false);

	// Convert waypoint to targetTile next tick.
	_unitData->SetNextTickState(_id, TransformState::NeedTargetAtom, UnitUpdateCallerEnum::MoveTo_Done);
	PUN_CHECK2(nextActiveTick() > Time::Ticks(), debugStr());
	AddDebugSpeech("---DONE]MoveTo:" + end.ToString());
	return true;
}

void UnitStateAI::Add_MoveToResource(ResourceHolderInfo holderInfo, int32 customFloodDistance) {
	_actions.push_back(Action(ActionEnum::MoveToResource, static_cast<int32>(holderInfo.resourceEnum), holderInfo.holderId, customFloodDistance));
}
void UnitStateAI::MoveToResource() {
	MoveToResource(ResourceHolderInfo(static_cast<ResourceEnum>(action().int32val1), action().int32val2), action().int32val3);
}
bool UnitStateAI::MoveToResource(ResourceHolderInfo holderInfo, int32 customFloodDistance) {
	const ResourceHolder& holder = resourceSystem().holder(holderInfo);
	
	bool succeed = MoveTo(holder.tile, customFloodDistance);

	// For Storages, trim off excess waypoint
	if (succeed && 
		holder.objectId != -1 && 
		_simulation->building(holder.objectId).isEnum(CardEnum::StorageYard)) 
	{
		_unitData->TrimWaypoint(holder.objectId, _id);
	}
	
	return succeed;
}

// For moving army across long distance.
void UnitStateAI::Add_MoveToForceLongDistance(WorldTile2 end) {
	_actions.push_back(Action(ActionEnum::MoveToForceLongDistance, end.tileId()));
}
void UnitStateAI::MoveToForceLongDistance()
{
	WorldTile2 end(action().int32val1);
	
	SCOPE_CYCLE_COUNTER(STAT_PunUnitDoMoveTo);
	WorldTile2 tile = unitTile();

	bool isIntelligent = IsIntelligentUnit(unitEnum());
	
	const int32 customCalculationCount = 200000;
	bool succeed = _simulation->pathAI(isIntelligent)->FindPath(tile.x, tile.y, end.x, end.y, rawWaypoint, true, isIntelligent, customCalculationCount);

	if (!succeed) 
	{
		DEBUG_AI_VAR(FailedToFindPathLongDist);
		_simulation->ResetUnitActions(_id, 60);
		
		AddDebugSpeech("(Bad)MoveToForceLongDistance: " + tile.ToString() + end.ToString());
		MoveToRobust(end);
		return;
	}

	_animationEnum = UnitAnimationEnum::Walk;

	MapUtil::UnpackAStarPath(rawWaypoint, _unitData->waypoint(_id));
	_unitData->SetForceMove(_id, true);

	// Convert waypoint to targetTile next tick.
	_unitData->SetNextTickState(_id, TransformState::NeedTargetAtom, UnitUpdateCallerEnum::MoveToForceLongDistance_Done);
	AddDebugSpeech("(Done)MoveToForceLongDistance:" + end.ToString());
}

void UnitStateAI::Add_MoveInRange(WorldTile2 end, int32_t range) {
	_actions.push_back(Action(ActionEnum::MoveInRange, end.tileId(), range));
}
// Typical MoveTo that trims off the rest of the stack once the unit moves close enough
void UnitStateAI::MoveInRange()
{
	WorldTile2 end(action().int32val1);
	int32 range = action().int32val2;
	
	if (!MoveTo(end)) {
		AddDebugSpeech("(Bad)MoveInRange: ");
		return;
	}

	std::vector<WorldTile2>& waypoints = _unitData->waypoint(_id);

#if TRAILER_MODE
	if (waypoints.size() == 0) {
		_simulation->ResetUnitActions(_id, 60);
		return;
	}
#endif
	
	check(waypoints.size() > 0);

	// TODO: handle case with less than 2
	for (int i = waypoints.size(); i-- > 1;) {
		if (WorldTile2::ManDistance(waypoints[i], end) < range) {
			waypoints.erase(waypoints.begin(), waypoints.begin() + i);
			break;
		}
	}

	//PUN_LOG("MoveInRange");
	NextAction(UnitUpdateCallerEnum::MoveInRange);
}

// TODO: MoveToRobust should eventually use special PathAI map that only has terrain + bridge + tunnels
// Should also have MoveToHard that will exceed typical max tiles tries for longer range...
void UnitStateAI::Add_MoveToRobust(WorldTile2 end) {
	_actions.push_back(Action(ActionEnum::MoveToRobust, end.tileId()));
}
void UnitStateAI::MoveToRobust() {
	MoveToRobust(WorldTile2(action().int32val1));
}
void UnitStateAI::MoveToRobust(WorldTile2 end)
{
	WorldTile2 tile = unitTile();
	check(tile.isValid());

	bool succeed = _simulation->pathAI(true)->FindPathRobust(tile.x, tile.y, end.x, end.y, rawWaypoint);
	check(succeed);

	MapUtil::UnpackAStarPath(rawWaypoint, _unitData->waypoint(_id));
	_unitData->SetForceMove(_id, true);

	// Convert waypoint to targetTile next tick.
	_unitData->SetNextTickState(_id, TransformState::NeedTargetAtom, UnitUpdateCallerEnum::MoveToRobust_Done);
	AddDebugSpeech("(Done)MoveToRobust:" + end.ToString());
}

void UnitStateAI::MoveTo_NoFail(WorldTile2 end, int32 customFloodDistance)
{
	if (IsMoveValid(end, customFloodDistance)) {
		Add_MoveTo(end, customFloodDistance);
		AddDebugSpeech("  MoveTo_NoFail: Normal");
	}
	else {
		Add_MoveToRobust(end);
		AddDebugSpeech("  MoveTo_NoFail: Force");
	}
}

// Move toward some atom ignoring walkability
void UnitStateAI::Add_MoveToward(WorldAtom2 end, int32 fraction100000) {
	_actions.push_back(Action(ActionEnum::MoveToward, end.x, end.y, fraction100000));
}
void UnitStateAI::MoveToward()
{
	WorldAtom2 end(action().int32val1, action().int32val2);
	int64 fraction100000 = action().int32val3;
	
	WorldAtom2 targetLocation = WorldAtom2::Lerp(_unitData->atomLocation(_id), end, fraction100000);
	_unitData->SetTargetLocation(_id, targetLocation);

	// Find how long it takes to get to target tile and set the nextActiveFrame to that point
	int32_t distance = WorldAtom2::Distance(targetLocation, _unitData->atomLocation(_id));
	int32_t ticksNeeded = distance / HumanGlobalInfo::MoveAtomsPerTick;

	// Make sure that tick is more than 0
	ticksNeeded = ticksNeeded > 0 ? ticksNeeded : 1;

	//PUN_LOG("MoveToward() UnitAnimationEnum::Walk id:%d", _id);

	_animationEnum = UnitAnimationEnum::Walk;

	// Be in the moving state until we arrived at destination.
	_unitData->SetNextTickState(_id, TransformState::Moving, UnitUpdateCallerEnum::MoveTowards_Done, ticksNeeded);
	AddDebugSpeech("(Done)MoveToward: " + end.ToString());
}


void UnitStateAI::Add_PickupResource(ResourceHolderInfo info, int amount) {
	AddAction(ActionEnum::PickupResource, static_cast<int32>(info.resourceEnum), info.holderId, amount);
}
void UnitStateAI::PickupResource()
{
	ResourceHolderInfo info(static_cast<ResourceEnum>(action().int32val1), action().int32val2);
	int amount = action().int32val3;
	
	AddDebugSpeech("PickupResource(Start): amount:" + to_string(amount) + " inv:" + to_string(_inventory.Amount(info.resourceEnum)));

	UnitReservation reservation = PopReservation(ReservationType::Pop);
	PUN_CHECK2(reservation.reserveHolder == info, debugStr());
	PUN_CHECK2(reservation.amount >= amount, debugStr());
	PUN_CHECK2(resourceSystem().holder(info).current() >= amount, debugStr());

	resourceSystem().RemoveResource(info, amount);
	_inventory.Add(ResourcePair(info.resourceEnum, amount));

	PUN_CHECK2(resourceSystem().resourceCount(info) >= 0, debugStr());

	// OnPickupResource
	int32 buildingId = _simulation->resourceSystem(_playerId).holder(info).objectId;
	if (buildingId != -1) {
		Building& building = _simulation->building(buildingId);

		PUN_CHECK(building.holderInfo(info.resourceEnum) == info);
		
		building.OnPickupResource(_id);
	}

	_simulation->soundInterface()->Spawn3DSound("ResourceDropoffPickup", "Pickup", unitAtom());
	
	NextAction(UnitUpdateCallerEnum::PickupResource);
}

void UnitStateAI::Add_DropoffResource(ResourceHolderInfo info, int amount) {
	AddAction(ActionEnum::DropoffResource, static_cast<int32>(info.resourceEnum), info.holderId, amount);
}
void UnitStateAI::DropoffResource()
{
	ResourceHolderInfo info(static_cast<ResourceEnum>(action().int32val1), action().int32val2);
	int amount = action().int32val3;
	
	AddDebugSpeech("(Start)DropoffResource: amount:" + to_string(amount) + " inv:" + to_string(_inventory.Amount(info.resourceEnum)));

	UnitReservation reservation = PopReservation(ReservationType::Push);
	PUN_CHECK2(reservation.reserveHolder == info, debugStr());
	PUN_CHECK2(_inventory.Amount(info.resourceEnum) >= amount, debugStr());

	resourceSystem().AddResource(info, amount);
	PUN_CHECK2(_inventory.Has(info.resourceEnum), debugStr());
	_inventory.Remove(ResourcePair(info.resourceEnum, amount));

	PUN_CHECK2(resourceSystem().resourceCount(info) >= 0, debugStr());
	
	// Statistics
	PUN_CHECK2(_simulation->buildingIsAlive(resourceSystem().holder(info).objectId), debugStr());
	Building& building = _simulation->building(info, _playerId);
	if (building.buildingEnum() == CardEnum::FruitGatherer && building.isConstructed()) {
		building.AddProductionStat(ResourcePair(info.resourceEnum, amount));
	}

	PUN_CHECK(building.holderInfo(info.resourceEnum) == info);

	building.OnDropoffResource(_id, info, amount);

	_simulation->soundInterface()->SpawnResourceDropoffAudio(info.resourceEnum, unitAtom());

	NextAction(UnitUpdateCallerEnum::DropoffResource);
}

void UnitStateAI::Add_DropInventoryAction() {
	AddAction(ActionEnum::DropInventoryAction);
}
void UnitStateAI::DropInventoryAction()
{
	//std::vector<ResourcePair> drops = unitInfo().resourceDrops;
	_inventory.ForEachResource([&](ResourcePair pair) {
		resourceSystem().SpawnDrop(pair.resourceEnum, pair.count, unitTile());

		_simulation->soundInterface()->SpawnResourceDropoffAudio(pair.resourceEnum, unitAtom());
	});
	_inventory.Clear();

	AddDebugSpeech("(Done)DropInventoryAction");
	NextAction(UnitUpdateCallerEnum::DropInventoryAction);
}

void UnitStateAI::Add_StoreGatheredAtWorkplace() {
	AddAction(ActionEnum::StoreGatheredAtWorkplace);
}
void UnitStateAI::StoreGatheredAtWorkplace()
{
	// TODO: right now only fruit, later on will need to be able to clear the whole inventory
	PUN_CHECK2(_inventory.Has(ResourceEnum::Orange) 
			|| _inventory.Has(ResourceEnum::Papaya)
			|| _inventory.Has(ResourceEnum::Coconut), debugStr());

	// End workplace reservation, and switch to drop-off resource reservation instead
	UnitReservation workplaceReservation = PopReservation(ReservationType::Workplace);
	Building& dropoffBld = _simulation->building(workplaceReservation.reserveWorkplaceId);

	if (!IsMoveValid(dropoffBld.gateTile())) {
		AddDebugSpeech("(Failed)StoreGatheredAtWorkplace: move invalid");
		_simulation->ResetUnitActions(_id);
		return;
	}

	ResourceEnum resourceEnum = _inventory.resourcePairs()[0].resourceEnum;
	ResourceHolderInfo dropoffInfo = dropoffBld.holderInfo(resourceEnum);
	PUN_CHECK2(dropoffInfo.isValid(), debugStr());

	// Reserve
	int32 amount = _inventory.Amount(resourceEnum);
	ReserveResource(ReservationType::Push, dropoffInfo, amount);

	// Set Actions
	Add_DropoffResource(dropoffInfo, amount);
	Add_MoveToResource(dropoffInfo);
	//Add_MoveTo(dropoffBld.gateTile());

	NextAction(UnitUpdateCallerEnum::StoreGatheredAtWorkplace);
	AddDebugSpeech("(Done)StoreGatheredAtWorkplace");
}

void UnitStateAI::Add_Produce(int32 workManSec100, int32 waitTicks, int32 timesLeft, int32 workplaceId) {
	_actions.push_back(Action(ActionEnum::Produce, workManSec100, waitTicks, timesLeft, workplaceId));
}
void UnitStateAI::Produce()
{
	int workManSec100 = action().int32val1;
	int waitTicks = action().int32val2;
	int timesLeft = action().int32val3;
	int32 workplaceId = action().int32val4;
	
	// Has to be reserveWorkplaceId because _workplaceId can change anytime
	UnitReservation reservation = GetReservation(workplaceId); //GetReservation(ReservationType::Workplace);
	Building& workplace = _simulation->building(reservation.reserveWorkplaceId);
	PUN_UNIT_CHECK(IsProducer(workplace.buildingEnum()) ||
					IsSpecialProducer(workplace.buildingEnum()));

	if (workplace.filledInputs()) {
		DoWork(workManSec100);

		if (workplace.filledInputs() && timesLeft > 0) {
			ReserveWork(workManSec100, workplace.buildingId());

			Add_Produce(workManSec100, waitTicks, timesLeft - 1, workplaceId);
		}
	}
	else {
		PopReservationWorkplace(workplaceId);
	}

	NextAction(waitTicks, UnitUpdateCallerEnum::Produce_Done);
	AddDebugSpeech("(Done)Produce:" + ReservationsToString());
}

void UnitStateAI::Add_Construct(int32 workManSec100, int32 waitTicks, int32 timesLeft, int32 workplaceId) {
	_actions.push_back(Action(ActionEnum::Construct, workManSec100, waitTicks, timesLeft, workplaceId));
}
void UnitStateAI::Construct()
{
	int32 workManSec100 = action().int32val1;
	int32 waitTicks = action().int32val2;
	int32 timesLeft = action().int32val3;
	int32 workplaceId = action().int32val4;

	//PUN_LOG("Construct() UnitAnimationEnum::Build id:%d timesLeft:%d", _id, timesLeft);
	
	//UnitReservation reservation = GetReservation(workplaceId);

	Building& workplace = _simulation->building(workplaceId);

	if (!workplace.isConstructed()) 
	{
		DoWork(workManSec100, workplaceId);

		AddDebugSpeech(" Not done constructing");
		
		if (!justReset() && !workplace.isConstructed() && timesLeft > 0) {
			ReserveWork(workManSec100, workplaceId);
			Add_Construct(workManSec100, waitTicks, timesLeft - 1, workplaceId);

			//PUN_LOG("Add_Construct tick:%d id:%d timesLeft:%d", Time::Ticks(), _id, timesLeft - 1);
			
			AddDebugSpeech(" Push another Construct() timesLeft:" + to_string(timesLeft - 1));
		}
	}
	else {
		PopReservationWorkplace(workplaceId);
		AddDebugSpeech(" Already constructed");
	}

	_animationEnum = UnitAnimationEnum::Build;

	NextAction(waitTicks, UnitUpdateCallerEnum::Construct_Done);
	AddDebugSpeech("(Done)Construct: workAmount:" + to_string(workManSec100) + ", tickCount:" + to_string(waitTicks));
}

//! Resource reservations
void UnitStateAI::ReserveResource(ReservationType type, ResourceHolderInfo info, int32 amount)
{
	AddDebugSpeech("(Start)ReserveResource:" + ReservationTypeName(type) + ", " + info.ToString() + ", amount:" + to_string(amount));
	PUN_UNIT_CHECK(info.isValid());

	_simulation->resourceSystem(_playerId).AddReservation(type, info, _id, amount);

	UnitReservation reservation;
	reservation.unitId = _id;
	reservation.reservationType = type;
	reservation.reserveHolder = info;
	reservation.amount = amount;
	reservations.push_back(reservation);

	AddDebugSpeech("(Done)ReserveResource:" + ReservationsToString());
}

void UnitStateAI::Add_FillInputs(int32 workplaceId) {
	AddAction(ActionEnum::FillInputs, workplaceId);
}
void UnitStateAI::FillInputs()
{
	int32 workplaceId = action().int32val1;
	
	UnitReservation workplaceReservation = PopReservationWorkplace(workplaceId);
	Building& workplace = _simulation->building(workplaceReservation.reserveWorkplaceId);
	
	// If accidently have 2 ppl coming here, don't double fill and lose resource
	if (workplace.filledInputs()) {
		_simulation->ResetUnitActions(_id);
		return;
	}
	
	BldInfo info = workplace.buildingInfo();
	ResourceSystem& resourceSystem = _simulation->resourceSystem(_playerId);

	PUN_UNIT_CHECK(reservations.size() <= 2); // Production building has max 2 inputs.

	// Remove resources from the building and considered it filled
	for (int i = reservations.size(); i-- > 0;) 
	{
		UnitReservation reservation = PopReservation(i);
		PUN_UNIT_CHECK(reservation.reservationType == ReservationType::Pop);

		ResourceEnum resourceEnum = reservation.reserveHolder.resourceEnum;
		PUN_UNIT_CHECK(resourceEnum == workplace.input1() ||
						resourceEnum == workplace.input2());

		PUN_UNIT_CHECK(workplace.resourceCount(resourceEnum) >= reservation.amount);
		workplace.RemoveResource(resourceEnum, reservation.amount);

		//if (i == 0) {
		//	workplace.AddConsumption1Stat(ResourcePair(resourceEnum, reservation.amount));
		//} else if (i == 1) {
		//	workplace.AddConsumption2Stat(ResourcePair(resourceEnum, reservation.amount));
		//}
	}

	// Set fillInput to true in workplace
	workplace.FillInputs();

	NextAction(UnitUpdateCallerEnum::FillInputs_Done);
	AddDebugSpeech("(Done)FillInputs:" + ReservationsToString());
}

//! Work reservations
void UnitStateAI::ReserveWork(int32 amount, int32 workplaceId)
{
	if (workplaceId == -1) {
		workplaceId = _workplaceId;
	}
	_simulation->building(workplaceId).ReserveWork(_id, amount);
	UnitReservation reservation;
	reservation.unitId = _id;
	reservation.reservationType = ReservationType::Workplace;
	reservation.reserveWorkplaceId = workplaceId;
	reservation.amount = amount;
	reservations.push_back(reservation);

	AddDebugSpeech("ReserveWork: " + ReservationsToString());
}
void UnitStateAI::DoWork(int32 workAmount, int32 workplaceId)
{
	UnitReservation unitReservation = workplaceId != -1 ? PopReservationWorkplace(workplaceId) : PopReservation(ReservationType::Workplace);
	PUN_UNIT_CHECK(unitReservation.amount == workAmount);

#if !TRAILER_MODE
	_simulation->building(unitReservation.reserveWorkplaceId).DoWork(_id, unitReservation.amount);
#endif
	//PUN_CHECK2(reservations.empty(), debugStr());

	AddDebugSpeech("DoWork: " + ReservationsToString());
}

void UnitStateAI::ReserveTreeTile(int32_t tileId)
{
	UnitReservation reservation;
	reservation.unitId = _id;
	reservation.reservationType = ReservationType::TreeTile;
	reservation.reserveTileId = tileId;
	reservations.push_back(reservation);

	_simulation->treeSystem().SetReserved(tileId, reservation);

	AddDebugSpeech("ReserveTreeTile: " + ReservationsToString());
}

void UnitStateAI::ReserveFarmTile(int32_t tileId)
{
	UnitReservation reservation;
	reservation.unitId = _id;
	reservation.reserveWorkplaceId = workplaceId();
	reservation.reservationType = ReservationType::FarmTile;
	reservation.reserveTileId = tileId;
	reservations.push_back(reservation);

	workplace()->subclass<Farm>(CardEnum::Farm).ReserveFarmTile(_id, tileId);

	AddDebugSpeech("ReserveFarmTile: " + ReservationsToString());
}

UnitReservation UnitStateAI::PopReservation(ReservationType reservationType) 
{
	for (int i = reservations.size(); i-- > 0;) {
		if (reservations[i].reservationType == reservationType) {
			UnitReservation reservation = PopReservation(i);

			AddDebugSpeech("PopReservation(Done): type:" + ReservationTypeName(reservationType) + ", " + ReservationsToString());
			PUN_UNIT_CHECK(reservation.unitId == _id);
			return reservation;
		}
	}
	PUN_NOENTRY_LOG(debugStr());
	return UnitReservation();
}

UnitReservation UnitStateAI::PopReservationWorkplace(int32 workplaceId)
{
	for (int i = reservations.size(); i-- > 0;) {
		if (reservations[i].reserveWorkplaceId == workplaceId) {
			UnitReservation reservation = PopReservation(i);

			AddDebugSpeech("PopReservation(Done): workplaceId:" + to_string(workplaceId) + ", " + ReservationsToString());
			PUN_UNIT_CHECK(reservation.unitId == _id);
			PUN_UNIT_CHECK(reservation.reservationType == ReservationType::Workplace);
			return reservation;
		}
	}
	PUN_NOENTRY_LOG(debugStr());
	return UnitReservation();
}

UnitReservation UnitStateAI::PopReservation(int index)
{
	UnitReservation reservation = reservations[index];
	PUN_UNIT_CHECK(reservation.unitId == _id);

	reservations.erase(reservations.begin() + index);
	switch (reservation.reservationType) {
		case ReservationType::Workplace: {
			_simulation->building(reservation.reserveWorkplaceId).UnreserveWork(reservation.unitId);
			break;
		}
		case ReservationType::Pop: 
		case ReservationType::Push: {
			int32_t amount = _simulation->resourceSystem(_playerId).RemoveReservation(reservation.reservationType, reservation.reserveHolder, _id);
			PUN_UNIT_CHECK(reservation.amount == amount);
			break;
		}
		case ReservationType::TreeTile: {
			auto& treeSystem = _simulation->treeSystem();
			PUN_UNIT_CHECK(treeSystem.Reservation(reservation.reserveTileId).unitId == _id);

			treeSystem.Unreserve(reservation.reserveTileId, reservation);
			break;
		}
		case ReservationType::FarmTile: {
			Farm& farm = _simulation->building(reservation.reserveWorkplaceId).subclass<Farm>(CardEnum::Farm);
			WorldTile2 tile(reservation.reserveTileId);
			PUN_UNIT_CHECK(tile.isValid());
			farm.UnreserveFarmTile(_id, tile);
			break;
		}
		default:
			PUN_NOENTRY();
	}
	return reservation;
}

void UnitStateAI::AddDebugSpeech(std::string message) 
{
#if USE_DEBUG_SPEECH
	SCOPE_CYCLE_COUNTER(STAT_PunUnitAddDebugSpeech);
	_debugSpeech << message << " Time:" << Time::Ticks() << ", " << nextActiveTick() << " act:" << _actions.size();

	if (!IsWildAnimal(_unitEnum) && _workplaceId != -1) 
	{
		_debugSpeech << ", workType:" << workplace()->buildingInfo().name
			<< ", isConstructed:" << to_string(workplace()->isConstructed())
			<< ", isAlive:" << to_string(_simulation->buildingIsAlive(_workplaceId));
	}

	_debugSpeech << "\n";
#endif
}

FText UnitStateAI::GetTypeName()
{
	if (isChild()) {
		const FText littleText = LOCTEXT("Little", "Little");
		return FText::Format(INVTEXT("{0} {1}"), littleText, unitInfo().name);
	}
	return unitInfo().name;
}


#undef LOCTEXT_NAMESPACE