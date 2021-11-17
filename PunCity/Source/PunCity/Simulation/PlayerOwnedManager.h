// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "TownManager.h"

/**
 * 
 */
class PlayerOwnedManager
{
public:
	PlayerOwnedManager(int32 playerId, FactionEnum factionEnum, IGameSimulationCore* simulation)
	{
		_playerId = playerId;
		_factionEnum = factionEnum;
		_simulation = simulation;

		_buffTicksLeft.resize(NonBuildingCardEnumCount, -1);

		_spTicks = 0;
		_maxSPTicks = MaxSP * ticksPerSP();
		_currentSkill = CardEnum::SpeedBoost;

		_isInDarkAge = false;

		_uniqueBuildingIds.resize(BuildingEnumCount, -1);
	}

	void Tick()
	{
		// Tick Mana
		int32 spIncrement = _isInDarkAge ? 2 : 1;
		_spTicks = std::min(_spTicks + spIncrement, _maxSPTicks);

		// Remove skill buff once timer ran out
		for (size_t i = _usedSkillTicks.size(); i-- > 0;) {
			if (Time::Ticks() - _usedSkillTicks[i] > Time::TicksPerSecond * 50) {
				_usedSkillTicks.erase(_usedSkillTicks.begin() + i);
				_usedSkillBuildingIds.erase(_usedSkillBuildingIds.begin() + i);
				_usedSkillEnums.erase(_usedSkillEnums.begin() + i);
			}
		}

		//TickArmyRegionClaim();
	}

	void Tick1Sec();

	void TickRound();

	/*
	 * Initial Stages
	 */
	bool hasChosenLocation() {
		return _simulation->GetProvinceCountPlayer(_playerId) > 0;
		//return _provincesClaimed.size() > 0;
	}
	bool hasChosenInitialResources() { return initialResources.isValid(); }
	bool hasCapitalTownhall() { return capitalTownhallId != -1; }

	/*
	 * 
	 */
	const std::vector<int32>& townIds() { return _townIds; }
	void AddTownId(int32 townId) { _townIds.push_back(townId); }

	int32 playerTownNumber(int32 townId) {
		for (int32 i = 0; i < _townIds.size(); i++) {
			if (townId == _townIds[i]) {
				return i;
			}
		}
		return 0;
	}

	FactionEnum factionEnum() const { return _factionEnum; }

	/*
	 * Variables
	 */
	int32 totalInfluenceIncome100() {
		int32 result = 0;
		for (int32 townId : _townIds) {
			result += _simulation->townManager(townId).totalInfluenceIncome100();
		}
		return result;
	}
	
	int32 totalRevenue100() {
		int32 result = 0;
		for (int32 townId : _townIds) {
			result += _simulation->townManager(townId).totalRevenue100();
		}
		return result;
	}
	int32 totalIncome100() {
		int32 result = 0;
		for (int32 townId : _townIds) {
			result += _simulation->townManager(townId).totalIncome100();
		}
		return result;
	}

	int64 science100PerRound() {
		int64 result = 0;
		for (int32 townId : _townIds) {
			result += _simulation->townManager(townId).science100PerRound();
		}
		return result;
	}
	
	int32 GetPlayerLandTileCount(bool includeMountain)
	{
		int32 result = 0;
		for (int32 townId : _townIds) {
			result += _simulation->townManager(townId).GetPlayerLandTileCount(includeMountain);
		}
		return result;
	}


	void AddTaxIncomeToString(TArray<FText>& args);
	void AddInfluenceIncomeToString(TArray<FText>& args);


	// TODO: rethink max stored influence, needs to be more intentional, allow for more choices
	//int32 storedToInfluenceRevenue = 20;
	//int32 maxStoredInfluence100()
	//{
	//	return 1000000;
	//	//return (influenceIncomes100[static_cast<int>(InfluenceIncomeEnum::Townhall)] +
	//	//	influenceIncomes100[static_cast<int>(InfluenceIncomeEnum::Population)] +
	//	//	influenceIncomes100[static_cast<int>(InfluenceIncomeEnum::Luxury)]) * storedToInfluenceRevenue;
	//}

	std::vector<int32> GetPlotStatVec(PlotStatEnum roundStatEnum) const
	{
		std::vector<int32> statVecResult;
		for (int32 townId : _townIds) {
			const std::vector<int32>& statVec = _simulation->townManager(townId).GetPlotStatVec(roundStatEnum);
			if (statVecResult.size() == 0) {
				statVecResult.resize(statVec.size(), 0);
			}
			for (size_t i = 0; i < statVec.size(); i++) {
				statVecResult[i] += statVec[i];
			}
		}
		return statVecResult;
	}
	

	

	//bool IsProvinceEasilyConnected(int32 provinceId) {
	//	PUN_CHECK(_claimedProvinceConnected.find(provinceId) != _claimedProvinceConnected.end());
	//	return _claimedProvinceConnected[provinceId];
	//}

public:
	//int32 GetProvinceUpkeep100(int32 provinceId)
	//{
	//	// Upkeep is half of the income in ideal
	//	// Upkeep suffers from the same penalty as claim price
	//	int32 baseUpkeep100 = _simulation->GetProvinceIncome100(provinceId) / 2;
	//	return baseUpkeep100;
	//}




	/*
	 * Buffs
	 */
	void AddBuff(CardEnum cardEnum) {
		_buffTicksLeft[static_cast<int>(cardEnum) - BuildingEnumCount] = Time::TicksPerYear * 2;
	}
	bool HasBuff(CardEnum cardEnum) {
		return _buffTicksLeft[static_cast<int>(cardEnum) - BuildingEnumCount] > 0;
	}
	std::vector<CardEnum> GetBuffs() {
		std::vector<CardEnum> buffEnums;
		for (size_t i = 0; i < _buffTicksLeft.size(); i++) {
			if (_buffTicksLeft[i] > 0) {
				buffEnums.push_back(static_cast<CardEnum>(i + BuildingEnumCount));
			}
		}
		return buffEnums;
	}
	int32 GetBuffTicksLeft(CardEnum cardEnum) {
		return _buffTicksLeft[static_cast<int>(cardEnum) - BuildingEnumCount];
	}

	void TryApplyBuff(CardEnum cardEnum);

	/*
	 * Permanent Global Bonuses
	 */
	void AddGlobalBonus(CardEnum cardEnum) {
		_globalBonuses.push_back(cardEnum);
	}
	const std::vector<CardEnum>& globalBonuses() {
		return _globalBonuses;
	}
	bool HasGlobalBonus(CardEnum cardEnumIn) {
		for (CardEnum cardEnum : _globalBonuses) {
			if (cardEnum == cardEnumIn) {
				return true;
			}
		}
		return false;
	}

	/*
	 * SP Skill
	 */
	float spFloat() { return static_cast<float>(_spTicks) / ticksPerSP(); }
	int32 GetSP() { return _spTicks / ticksPerSP(); }
	int32 maxSP() { return _maxSPTicks/ ticksPerSP(); }
	CardEnum currentSkill() { return _currentSkill; }

	void UseSkill(int32 buildingId)
	{
		int32 manaCost = GetSkillManaCost(_currentSkill);
		int32 manaTicks = manaCost * ticksPerSP();
		
		if (_spTicks >= manaTicks) {
			_spTicks = _spTicks - manaTicks;

			_usedSkillEnums.push_back(_currentSkill);
			_usedSkillBuildingIds.push_back(buildingId);
			_usedSkillTicks.push_back(Time::Ticks());
		}
	}
	bool HasSpeedBoost(int32 buildingId) {
		for (size_t i = 0; i < _usedSkillEnums.size(); i++) {
			if (_usedSkillEnums[i] == CardEnum::SpeedBoost && _usedSkillBuildingIds[i] == buildingId)  {
				return true;
			}
		}
		return false;
	}

	bool IsInDarkAge() { return _isInDarkAge; }

	/*
	 * Diplomacy
	 */
	const std::vector<int32>& GetDiplomaticBuilding() { return _diplomaticBuildingIds; }
	
	void AddDiplomaticBuilding(int32 buildingId) { _diplomaticBuildingIds.push_back(buildingId); }
	void RemoveDiplomaticBuilding(int32 buildingId) { CppUtils::TryRemove(_diplomaticBuildingIds, buildingId); }

	const std::vector<int32>& GetSpyNestIds() { return _spyNestIds; }
	void AddSpyNestId(int32 buildingId) { _spyNestIds.push_back(buildingId); }
	void RemoveSpyNestId(int32 buildingId) { CppUtils::TryRemove(_spyNestIds, buildingId); }

	int32 GetUniqueBuildingId(CardEnum buildingEnum) { return _uniqueBuildingIds[static_cast<int32>(buildingEnum)]; }
	void AddUniqueBuildingId(CardEnum buildingEnum, int32 buildingId) { _uniqueBuildingIds[static_cast<int32>(buildingEnum)] = buildingId; }
	void RemoveUniqueBuildingId(CardEnum buildingEnum, int32 buildingId)
	{
		check(_uniqueBuildingIds[static_cast<int32>(buildingEnum)] == buildingId);
		_uniqueBuildingIds[static_cast<int32>(buildingEnum)] = -1;
	}

	/*
	 * Serialize
	 */
	void Serialize(FArchive& Ar)
	{
		//_LOG(PunPlayerOwned, "Serialize[%d] Before isSaving:%d, %d %d %d", _playerId, Ar.IsSaving(), _roadConstructionIds.size(), _constructionIds.size(), _jobBuildingEnumToIds.size());
		
		/*
		 * Public
		 */
		Ar << needChooseLocation;
		Ar << justChoseLocation;
		initialResources.Serialize(Ar);

		Ar << capitalTownhallId;

		Ar << economicVictoryPhase;
		
		// Influence
		//Ar << influencePerRound;

		// Tax Level
		Ar << vassalTaxLevel;

		Ar << alreadyDidGatherMark;
		Ar << alreadyBoughtFirstCard;

		/*
		 * Private
		 */
		SerializeVecValue(Ar, _townIds);
		Ar << _factionEnum;

		// SP
		Ar << _spTicks;
		Ar << _maxSPTicks;
		Ar << _currentSkill;

		Ar << _isInDarkAge;
		
		SerializeVecValue(Ar, _usedSkillEnums);
		SerializeVecValue(Ar, _usedSkillBuildingIds);
		SerializeVecValue(Ar, _usedSkillTicks);

		// Buffs
		SerializeVecValue(Ar, _buffTicksLeft);

		SerializeVecValue(Ar, _globalBonuses);

		// 
		SerializeVecValue(Ar, _diplomaticBuildingIds);
		SerializeVecValue(Ar, _spyNestIds);
		SerializeVecValue(Ar, _uniqueBuildingIds);

		Ar << spyNestInfluenceStolen;

		//_LOG(PunPlayerOwned, "Serialize[%d] After isSaving:%d, %d %d %d", _playerId, Ar.IsSaving(), _roadConstructionIds.size(), _constructionIds.size(), _jobBuildingEnumToIds.size());
	}
	
public:
	//! Serialized
	bool needChooseLocation = true;
	bool justChoseLocation = false;
	FChooseInitialResources initialResources;

	int32 capitalTownhallId = -1;

	int32 economicVictoryPhase = 0;

	int32 vassalTaxLevel = 2;


	bool alreadyDidGatherMark = false;
	bool alreadyBoughtFirstCard = false;

private:
	std::vector<int32> _townIds;
	
	// FactionEnum
	FactionEnum _factionEnum = FactionEnum::None;
	
	// SP
	static int32 ticksPerSP() { return Time::TicksPerSecond * 2; }
	int32 _spTicks;
	int32 _maxSPTicks;
	CardEnum _currentSkill;

	static const int32 MaxSP = 240;

	std::vector<CardEnum> _usedSkillEnums;
	std::vector<int32> _usedSkillBuildingIds;
	std::vector<int32> _usedSkillTicks;

	// Buffs
	std::vector<int32> _buffTicksLeft;

	std::vector<CardEnum> _globalBonuses;

	std::vector<int32> _diplomaticBuildingIds;
	std::vector<int32> _spyNestIds;
	std::vector<int32> _uniqueBuildingIds; // unique buildings from buildingEnum

	//
	int32 spyNestInfluenceStolen = 0;

	// Dark Age
	bool _isInDarkAge;

private:
	IGameSimulationCore* _simulation;
	int32 _playerId;
};
