// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "TownManager.h"

/**
 * 
 */
class PlayerOwnedManager
{
public:
	PlayerOwnedManager(int32 playerId, IGameSimulationCore* simulation)
	{
		_playerId = playerId;
		_simulation = simulation;

		_buffTicksLeft.resize(NonBuildingCardEnumCount, -1);

		_spTicks = 0;
		_maxSPTicks = MaxSP * ticksPerSP();
		_currentSkill = CardEnum::SpeedBoost;

		_isInDarkAge = false;
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
	int32 maxStoredInfluence100()
	{
		return 1000000;
		//return (influenceIncomes100[static_cast<int>(InfluenceIncomeEnum::Townhall)] +
		//	influenceIncomes100[static_cast<int>(InfluenceIncomeEnum::Population)] +
		//	influenceIncomes100[static_cast<int>(InfluenceIncomeEnum::Luxury)]) * storedToInfluenceRevenue;
	}

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
	
	/*
	 * Claim Province Attack
	 */
	void StartConquerProvince(int32 attackerPlayerId, int32 provinceId)
	{
		ProvinceClaimProgress claimProgress;
		claimProgress.provinceId = provinceId;
		claimProgress.attackerPlayerId = attackerPlayerId;
		claimProgress.committedInfluencesAttacker = BattleInfluencePrice;
		claimProgress.committedInfluencesDefender = 0;
		claimProgress.ticksElapsed = BattleClaimTicks / 4; // Start at 25% for attacker
		
		_defendingClaimProgress.push_back(claimProgress);
	}
	void StartConquerProvince_Attacker(int32 provinceId) {
		_attackingProvinceIds.push_back(provinceId);
	}
	void ReinforceAttacker(int32 provinceId, int32 influenceAmount)
	{
		for (auto& claimProgress : _defendingClaimProgress) {
			if (claimProgress.provinceId == provinceId) {
				claimProgress.committedInfluencesAttacker += influenceAmount;
				break;
			}
		}
	}
	void ReinforceDefender(int32 provinceId, int32 influenceAmount)
	{
		for (auto& claimProgress : _defendingClaimProgress) {
			if (claimProgress.provinceId == provinceId) {
				claimProgress.committedInfluencesDefender += influenceAmount;
				break;
			}
		}
	}
	const std::vector<ProvinceClaimProgress>& defendingClaimProgress() { return _defendingClaimProgress; }

	ProvinceClaimProgress GetDefendingClaimProgress(int32 provinceId) const {
		for (const ProvinceClaimProgress& claimProgress : _defendingClaimProgress) {
			if (claimProgress.provinceId == provinceId) {
				return claimProgress;
			}
		}
		return ProvinceClaimProgress();
	}
	
	
	void EndConquer(int32 provinceId) {
		CppUtils::RemoveOneIf(_defendingClaimProgress, [&](ProvinceClaimProgress& claimProgress) { return claimProgress.provinceId == provinceId; });
	}
	void EndConquer_Attacker(int32 provinceId) {
		CppUtils::Remove(_attackingProvinceIds, provinceId);
	}

	// provinceId and attackerPlayerId is needed because this maybe called before the attack started
	ProvinceAttackEnum GetProvinceAttackEnum(int32 provinceId, int32 attackerPlayerId)
	{
		int32 provincePlayerId = _simulation->provinceOwnerPlayer(provinceId);

		if (_simulation->homeProvinceId(provincePlayerId) == provinceId) 
		{
			if (lordPlayerId() != -1)
			{
				// The attacker is the townhall owner, trying to expel the lord
				if (attackerPlayerId == _playerId) {
					return ProvinceAttackEnum::DeclareIndependence;
				}
				return ProvinceAttackEnum::VassalCompetition;
			}
			return ProvinceAttackEnum::Vassalize;
		}

		// Town Conquer
		int32 townId = _simulation->provinceOwnerTown(provinceId);
		if (townId != -1 &&
			_simulation->IsTownhallOverlapProvince(provinceId, provincePlayerId))
		{
			return ProvinceAttackEnum::ConquerColony;
		}
		
		return ProvinceAttackEnum::ConquerProvince;
	}
	

	//bool IsProvinceEasilyConnected(int32 provinceId) {
	//	PUN_CHECK(_claimedProvinceConnected.find(provinceId) != _claimedProvinceConnected.end());
	//	return _claimedProvinceConnected[provinceId];
	//}

public:
	int32 GetProvinceUpkeep100(int32 provinceId)
	{
		// Upkeep is half of the income in ideal
		// Upkeep suffers from the same penalty as claim price
		int32 baseUpkeep100 = _simulation->GetProvinceIncome100(provinceId) / 2;
		return baseUpkeep100;
	}


	/*
	 * Vassal
	 */
	int32 lordPlayerId() { return _lordPlayerId; }
	void SetLordPlayerId(int32 lordPlayerId) {
		_lordPlayerId = lordPlayerId;
	}

	int32 playerIdForColor() {
		if (_lordPlayerId != -1) {
			return _lordPlayerId;
		}
		return _playerId;
	}
	
	const std::vector<int32>& vassalBuildingIds() const { return _vassalBuildingIds; }
	void GainVassal(int32 vassalBuildingId) {
		_vassalBuildingIds.push_back(vassalBuildingId);
	}
	void LoseVassal(int32 vassalBuildingId) {
		CppUtils::TryRemove(_vassalBuildingIds, vassalBuildingId);
	}
	bool IsVassal(int32 vassalBuildingId) {
		return CppUtils::Contains(_vassalBuildingIds, vassalBuildingId);
	}

	/*
	 * Ally
	 */
	const std::vector<int32>& allyPlayerIds() { return _allyPlayerIds; }
	void GainAlly(int32 allyPlayerId) {
		_allyPlayerIds.push_back(allyPlayerId);
	}
	void LoseAlly(int32 allyPlayerId) {
		CppUtils::TryRemove(_allyPlayerIds, allyPlayerId);
	}
	bool IsAlly(int32 playerId) {
		return CppUtils::Contains(_allyPlayerIds, playerId);
	}


	int32 vassalTaxPercent() {
		return 5;
		//switch (taxLevel)
		//{
		//case 0: return 0;
		//case 1: return 10;
		//case 2: return 20;
		//case 3: return 30;
		//case 4: return 40;
		//default:
		//	UE_DEBUG_BREAK();
		//}
		//return -1;
	}

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
	bool HasGlobalBonuses(CardEnum cardEnumIn) {
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

		// Vassal/Annex
		Ar << _lordPlayerId;
		SerializeVecValue(Ar, _vassalBuildingIds);
		SerializeVecValue(Ar, _allyPlayerIds);
		//SerializeVecValue(Ar, _armyNodesVisited);
		//_battle.Serialize(Ar);

		SerializeVecValue(Ar, _attackingProvinceIds);
		SerializeVecObj(Ar, _defendingClaimProgress);

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

		//_LOG(PunPlayerOwned, "Serialize[%d] After isSaving:%d, %d %d %d", _playerId, Ar.IsSaving(), _roadConstructionIds.size(), _constructionIds.size(), _jobBuildingEnumToIds.size());
	}

	
public:
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
	// 
	// Vassal/Annex
	int32 _lordPlayerId = -1;
	std::vector<int32> _vassalBuildingIds;
	std::vector<int32> _allyPlayerIds;

	std::vector<int32> _attackingProvinceIds;
	std::vector<ProvinceClaimProgress> _defendingClaimProgress;

	
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

	// Dark Age
	bool _isInDarkAge;

private:
	IGameSimulationCore* _simulation;
	int32 _playerId;
};
