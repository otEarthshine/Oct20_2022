// Pun Dumnernchanvanit's

#pragma once

#include "IGameSimulationCore.h"
#include "PunCity/CppUtils.h"

struct ArmyGroup
{
public:
	int32 playerId = -1;

	int32 marchEndNodeId = -1;
	int32 marchStartNodeId = -1;
	int32 marchStartTick = -1;
	int32 marchEndTick = -1;
	//ArrivalIntentionEnum intentionEnum = ArrivalIntentionEnum::None;

	// helpPlayerId == originalOwner ... liberate or help defend
	// helpPlayerId == -1 ... Conquer or move to merge with self
	// helpPlayerId == currentOwner ... help currentOwner defend or attack...
	int32 helpPlayerId = -1;

	std::vector<int32> HPs;
	std::vector<int32> initialHPs;
	std::vector<int32> lastAttackedTick;
	std::vector<int32> lastDamageTaken; // For UI Display
	std::vector<int32> lastDamageTick;

	ArmyGroup() {}

	ArmyGroup(int32 playerIdIn, int32 nodeId, std::vector<int32> armyCounts, int32 helpPlayerIdIn) {
		playerId = playerIdIn;
		marchEndNodeId = nodeId;
		helpPlayerId = helpPlayerIdIn;
		
		for (int32 i = 0; i < ArmyEnumCount; i++) {
			HPs.push_back(armyCounts[i] * GetArmyInfoInt(i).maxHp);
			lastAttackedTick.push_back(0);
			lastDamageTaken.push_back(-1);
			lastDamageTick.push_back(0);
		}
		initialHPs = HPs;
	}
	ArmyGroup(int32 playerIdIn, int32 nodeId, ArmyEnum armyEnum, int32 count, int32 helpPlayerIdIn = -1) {
		playerId = playerIdIn;
		marchEndNodeId = nodeId;
		helpPlayerId = helpPlayerIdIn;
		
		HPs.resize(ArmyEnumCount, 0);
		HPs[static_cast<int>(armyEnum)] = count * GetArmyInfo(armyEnum).maxHp;
		
		lastAttackedTick.resize(ArmyEnumCount, 0);
		lastDamageTaken.resize(ArmyEnumCount, -1);
		lastDamageTick.resize(ArmyEnumCount, 0);
		initialHPs = HPs;
	}

	// After battle initialHP must be reset
	void ResetUnitHP()
	{
		for (int32 i = TowerArmyEnumCount; i < ArmyEnumCount; i++) {
			// Round up HPs[i] to full unit hps
			HPs[i] = TroopCount(i) * GetArmyInfoInt(i).maxHp;
			initialHPs[i] = HPs[i];
		}
	}

	// Ensure wall exist, if not create a 5% hp one
	void EnsureWall(int32 wallLvl)
	{
		// if already a wall, make sure hp is at least 5%
		if (initialHPs[0] > 0) {
			HPs[0] = std::max(HPs[0], initialHPs[0] * 5 / 100);
			return;
		}
		initialHPs[0] = GetArmyInfo(ArmyEnum::Tower).maxWallHP(wallLvl);
		HPs[0] = initialHPs[0];
	}
	void UpgradeWallLvl(int32 wallLvl) {
		initialHPs[0] = GetArmyInfo(ArmyEnum::Tower).maxWallHP(wallLvl);
	}

	void TakeDamage(int32 armyEnumInt, int32 attackDamage)
	{
		int32 lastHP = HPs[armyEnumInt];
		
		PUN_LOG("-- Before TakeDamage: %s", ToTChar(ToString()));
		HPs[armyEnumInt] = std::max(0, HPs[armyEnumInt] - attackDamage / GetArmyInfoInt(armyEnumInt).defense);
		PUN_LOG("-- After TakeDamage: %s", ToTChar(ToString()));
		
		lastDamageTaken[armyEnumInt] = lastHP - HPs[armyEnumInt];
		lastDamageTick[armyEnumInt] = Time::Ticks();
	}

	bool IsMarching() const { return marchStartTick != -1; }
	int32 TicksUntilMarchComplete() const {
		return IsMarching() ? std::max(0, marchEndTick - Time::Ticks()) : -1;
	}
	int32 SecsUntilMarchComplete() const {
		return TicksUntilMarchComplete() / Time::TicksPerSecond;
	}
	
	/*
	 * Add/Remove Army
	 */
	void AddArmy(std::vector<int32> armyCounts)
	{
		PUN_LOG("AddArmy");
		PUN_LOG(" BEFORE %s", ToTChar(ToString()));
		
		for (int32 i = 0; i < ArmyEnumCount; i++) {
			HPs[i] += armyCounts[i] * GetArmyInfoInt(i).maxHp;
			initialHPs[i] += armyCounts[i] * GetArmyInfoInt(i).maxHp;
		}
		PUN_LOG(" AFTER %s", ToTChar(ToString()));
	}
	void AddArmy(ArmyEnum armyEnum, int32 count, int32 hpPercent = 100) {
		PUN_LOG("AddArmy: %d count:%d hp:%d", armyEnum, count, hpPercent);
		PUN_LOG(" BEFORE %s", ToTChar(ToString()));
		
		int32 i = static_cast<int32>(armyEnum);
		HPs[i] += count * GetArmyInfoInt(i).maxHp * hpPercent / 100;
		initialHPs[i] += count * GetArmyInfoInt(i).maxHp;

		PUN_LOG(" AFTER %s", ToTChar(ToString()));
	}
	std::vector<int32> RemoveArmyPartial(std::vector<int32> removalCounts)
	{
		PUN_LOG("RemoveArmyPartial");
		PUN_LOG(" BEFORE %s", ToTChar(ToString()));
		
		std::vector<int32> actualRemoved(removalCounts.size(), 0);
		for (int32 i = 0; i < ArmyEnumCount; i++) 
		{
			actualRemoved[i] = std::min(TroopCount(i), removalCounts[i]);
			
			HPs[i] = HPs[i] - actualRemoved[i] * GetArmyInfoInt(i).maxHp;
			HPs[i] = std::max(HPs[i], 0);
			
			initialHPs[i] -= actualRemoved[i] * GetArmyInfoInt(i).maxHp;
		}
		
		PUN_LOG(" AFTER %s", ToTChar(ToString()));
		return actualRemoved;
	}
	int32 RemoveArmyPartial(int32 armyEnumInt, int32 removeCount)
	{
		PUN_LOG("RemoveArmyPartial2");
		PUN_LOG(" BEFORE %s", ToTChar(ToString()));

		int32 actualRemoved = std::min(TroopCount(armyEnumInt), removeCount);

		HPs[armyEnumInt] = HPs[armyEnumInt] - actualRemoved * GetArmyInfoInt(armyEnumInt).maxHp;
		HPs[armyEnumInt] = std::max(HPs[armyEnumInt], 0);

		initialHPs[armyEnumInt] -= actualRemoved * GetArmyInfoInt(armyEnumInt).maxHp;

		PUN_LOG(" AFTER %s", ToTChar(ToString()));
		return actualRemoved;
	}
	

	void ClearArmy()
	{
		for (int32 i = 0; i < ArmyEnumCount; i++) {
			HPs[i] = 0;
			initialHPs[i] = 0;
		}
	}

	/*
	 * Get information
	 */
	bool isValid() const { return playerId != -1; }

	bool isDead() {
		for (int32 i = 0; i < ArmyEnumCount; i++) {
			if (HPs[i] > 0) return false;
		}
		return true;
	}

	int32 TicksUntilMarchComplete() { return std::max(0, marchEndTick - Time::Ticks()); }
	int32 TotalMarchTicks() { return marchEndTick - marchStartTick; }

	bool HasTroops() {
		return CppUtils::Sum(HPs) > 0;
	}

	int32 TroopCount(int32 armyEnumInt) const {
		int32 maxHp = GetArmyInfoInt(armyEnumInt).maxHp;
		return (maxHp - 1 + HPs[armyEnumInt]) / maxHp;
	}

	int32 TotalTroopCount()
	{
		int32 total = 0;
		for (int32 i = 0; i < ArmyEnumCount; i++) {
			total += TroopCount(i);
		}
		return total;
	}

	std::vector<int32> GetArmyCounts(bool skipWall = false)
	{
		std::vector<int32> armyCounts(ArmyEnumCount, 0);
		int32 startIndex = skipWall ? TowerArmyEnumCount : 0;
		for (int32 i = startIndex; i < ArmyEnumCount; i++) {
			armyCounts[i] = TroopCount(i);
		}
		return armyCounts;
	}
	bool HasMovableArmy()
	{
		for (int32 i = TowerArmyEnumCount; i < ArmyEnumCount; i++) {
			if (TroopCount(i) > 0) return true;
		}
		return false;
	}

	// Add to old array
	void AddArmyToGivenArray(std::vector<int32>& armyCounts, bool skipWall = false) const
	{
		for (int32 i = (skipWall? TowerArmyEnumCount : 0); i < ArmyEnumCount; i++) {
			armyCounts[i] += TroopCount(i);
		}
	}

	/*
	 * Attack
	 */
	template <typename Func>
	bool IsReadyToHit(int32 dealerEnumInt, Func getAttackDelayPenalty) const {
		// Delay Randomized using lastAttackedTick
		int32 delayTicks = GetArmyInfoInt(dealerEnumInt).attackDelayTicks();
		if (delayTicks == 0) {
			return false;
		}
		delayTicks = delayTicks * (100 + getAttackDelayPenalty()) / 100;
		return Time::Ticks() - lastAttackedTick[dealerEnumInt] >= delayTicks;
	}


	int32 AttackDamage(int32 dealerEnumInt)
	{
		ArmyInfo dealerInfo = GetArmyInfoInt(dealerEnumInt);
		
		int32 troopCount = TroopCount(dealerEnumInt);
		int32 attack = troopCount * dealerInfo.attack * BaseDamagePerAttack;

		// Randomize attack
		attack = attack * (GameRand::Rand() % 100 + 50) / 100;

		if (SimSettings::IsOn("CheatFastBuild")) {
			attack *= 3;
		}
		
		return attack;
	}

	std::string ToString() const
	{
		std::stringstream ss;
		ss << "[pid:" << playerId << " marchEnd:" << marchEndNodeId << " marchStart:" << marchStartNodeId;
		ss << " startTick:" << marchStartTick << " endTick:" << marchEndTick;
		ss << " helpId:" << helpPlayerId;
		ss << "|";
		for (int32 i = 0; i < ArmyEnumCount; i++) {
			ss << HPs[i] << "/" << initialHPs[i] << " ";
		}
		ss << "]";
		return ss.str();
	}

	void operator>>(FArchive& Ar)
	{
		Ar << playerId;

		Ar << marchEndNodeId;
		Ar << marchStartNodeId;
		Ar << marchStartTick;
		Ar << marchEndTick;
		Ar << helpPlayerId;

		SerializeVecValue(Ar, HPs);
		SerializeVecValue(Ar, initialHPs);
		SerializeVecValue(Ar, lastAttackedTick);
		SerializeVecValue(Ar, lastDamageTaken);
		SerializeVecValue(Ar, lastDamageTick);
	}
};

/**
 * 
 */
class PROTOTYPECITY_API ArmyNode
{
public:
	int32 nodeId = -1;
	int32 originalPlayerId = -1;
	int32 lastLordPlayerId = -1; // Used by PlayerOwned to check if there is new lord...
	int32 lordPlayerId = -1;

	std::vector<ArmyGroup> defendGroups; // [0] group is the owner
	std::vector<ArmyGroup> attackGroups;
	std::vector<ArmyGroup> arrivingGroups;

	std::vector<ArmyGroup> rebelGroups;

	void Init(int32 playerId, int32 buildingId, IGameSimulationCore* simulation)
	{
		_simulation = simulation;

		nodeId = buildingId;
		originalPlayerId = playerId;
		lastLordPlayerId = playerId;
		lordPlayerId = playerId;

		// Start with a tower
		std::vector<int32> armyCounts(ArmyEnumCount, 0);
		armyCounts[static_cast<int>(ArmyEnum::Tower)] = 1;
		//armyCounts[static_cast<int>(ArmyEnum::Clubman)] = 5;
		ArmyGroup group(playerId, nodeId, armyCounts, -1);
		defendGroups.push_back(group);
	}
	void InitAfterLoad(IGameSimulationCore* simulation) {
		_simulation = simulation;
	}
	

	bool isBattling() const { return attackGroups.size() > 0; }
	bool hasArrivingAttackers() const {
		for (const ArmyGroup& group : arrivingGroups) {
			if (group.helpPlayerId == -1) {
				return true;
			}
		}
		return false;
	}

	bool IsPlayerInvolved(int32 playerId) const {
		bool isInvolved = false;
		ExecuteOnAllGroups([&](const ArmyGroup& group) {
			if (group.playerId == playerId) {
				isInvolved = true;
			}
		});
		return isInvolved;
	}

	template <typename Func>
	void ExecuteOnAllGroups(Func func, bool includeRebel = false) const {
		for (const ArmyGroup& group : defendGroups) {
			func(group);
		}
		for (const ArmyGroup& group : attackGroups) {
			func(group);
		}
		for (const ArmyGroup& group : arrivingGroups) {
			func(group);
		}
		if (includeRebel) {
			for (const ArmyGroup& group : rebelGroups) {
				func(group);
			}
		}
	}

	void Tick()
	{
		// Arrival at target
		for (int32 i = arrivingGroups.size(); i-- > 0;) 
		{
			if (arrivingGroups[i].TicksUntilMarchComplete() <= 0)
			{
				ArmyGroup group = arrivingGroups[i];
				arrivingGroups.erase(arrivingGroups.begin() + i);
				group.marchStartNodeId = -1;
				group.marchStartTick = -1;
				group.marchEndTick = -1;

				DetermineArrivingGroupDestination(group);
			}
		}
		
		if (isBattling()) {
			//PUN_LOG("Before DealDamage:");
			//for (const ArmyGroup& group : attackGroups) {
			//	PUN_LOG("atkGrp: %s", ToTChar(group.ToString()));
			//}
			//for (const ArmyGroup& group : defendGroups) {
			//	PUN_LOG("defGrp: %s", ToTChar(group.ToString()));
			//}
			
			DealDamage(attackGroups, defendGroups);

			//PUN_LOG("After DealDamage:");
			//for (const ArmyGroup& group : attackGroups) {
			//	PUN_LOG("atkGrp: %s", ToTChar(group.ToString()));
			//}
			//for (const ArmyGroup& group : defendGroups) {
			//	PUN_LOG("defGrp: %s", ToTChar(group.ToString()));
			//}
			//PUN_LOG("---");
			
			RefreshNodeState();
		}

		// HP recovery for wall
		if (Time::Ticks() % Time::TicksPerSecond == 0) 
		{
			if (defendGroups.size() > 0) {
				ArmyGroup& group = defendGroups[0];
				
				for (int32 i = 0; i < TowerArmyEnumCount; i++) 
				{
					if (group.HPs[i] > 0) // Don't recover a destroyed wall
					{
						int32 hpRecovery = group.initialHPs[i] / 200;  // 0.5 percent every 1 sec
						if (isBattling()) {
							hpRecovery /= 2; // half HP recovery during battle
						}
						group.HPs[i] = std::min(group.initialHPs[i], group.HPs[i] + hpRecovery);
					}
				}
			}
		}
	}

	void DetermineArrivingGroupDestination(ArmyGroup& group)
	{
		// Check first if merging with self is possible
		for (size_t j = 0; j < defendGroups.size(); j++) {
			if (defendGroups[j].playerId == group.playerId) {
				defendGroups[j].AddArmy(group.GetArmyCounts());
				return;
			}
		}
		for (size_t j = 0; j < attackGroups.size(); j++) {
			if (attackGroups[j].playerId == group.playerId) {
				attackGroups[j].AddArmy(group.GetArmyCounts());
				return;
			}
		}

		// Player is original owner and is in hiding, hide with the rest
		for (size_t j = 0; j < rebelGroups.size(); j++) {
			if (rebelGroups[j].playerId == group.playerId) {
				rebelGroups[j].AddArmy(group.GetArmyCounts());
				return;
			}
		}

		// Army of node owner should've been merged with defendGroups
		PUN_CHECK(group.playerId != lordPlayerId);

		
		// Conquer
		if (group.helpPlayerId == -1) {
			attackGroups.push_back(group);
		}
		// Side with current lord, help defend
		else if (group.helpPlayerId == lordPlayerId) {
			defendGroups.push_back(group);
		}
		// Side with original owner, determine which side the guy is on, and help, or attack current lord
		else if (group.helpPlayerId == originalPlayerId)
		{
			// original playerId is defender, help defend
			for (size_t j = 0; j < defendGroups.size(); j++) {
				if (defendGroups[j].playerId == originalPlayerId) {
					defendGroups.push_back(group);
					return;
				}
			}

			// for these 2 conditions, add to attackGroup
			// - original playerId is revolting help them
			// - original playerId no longer has army, liberate
			attackGroups.push_back(group);
		}
	}

	void MarchStart(ArmyGroup group, const ArmyNode& originNode, WorldTile2 startTile = WorldTile2::Invalid);

	ArmyGroup* GetArmyGroup(int32 playerId)
	{
		for (ArmyGroup& group : defendGroups) {
			if (group.playerId == playerId) return &group;
		}
		for (ArmyGroup& group : attackGroups) {
			if (group.playerId == playerId) return &group;
		}
		return nullptr;
	}
	ArmyGroup* GetArrivingGroup(int32 playerId)
	{
		for (ArmyGroup& group : arrivingGroups) {
			if (group.playerId == playerId) return &group;
		}
		return nullptr;
	}
	ArmyGroup* GetRebelGroup(int32 playerId) 	{
		if (rebelGroups.size() > 0) return &(rebelGroups[0]);
		return nullptr;
	}

	void AddArmyToCapital(int32 playerId, std::vector<int32> armyCounts)
	{
		ArmyGroup* group = GetArmyGroup(playerId);

		if (group) {
			group->AddArmy(armyCounts);
			return;
		}

		// no army in our own capital, join the rebel...
		if (rebelGroups.size() > 0) {
			rebelGroups[0].AddArmy(armyCounts);
		} else {
			rebelGroups.push_back(ArmyGroup(playerId, nodeId, armyCounts, playerId));
		}
	}
	
	std::vector<ArmyGroup> RemoveArrivalArmyGroups(int32 playerId)
	{
		std::vector<ArmyGroup> groups;
		for (size_t i = arrivingGroups.size(); i-- > 0;) {
			if (arrivingGroups[i].playerId == playerId) {
				groups.push_back(arrivingGroups[i]);
				arrivingGroups.erase(arrivingGroups.begin() + i);
			}
		}
		return groups;
	}

	//int32 DistanceToPlayer(int32 playerId)
	//{
	//	std::vector<int32> armyNodeIds = _simulation->GetArmyNodeIds(playerId);
	//	WorldTile2 currentCenter = _simulation->buildingCenter(nodeId);
	//	
	//	int32 closestDist = INT_MAX;
	//	for (int32 armyNodeId : armyNodeIds) {
	//		int32 dist = WorldTile2::Distance(_simulation->buildingCenter(armyNodeId), currentCenter);
	//		if (dist < closestDist) {
	//			closestDist = dist;
	//		}
	//	}
	//	PUN_CHECK(closestDist != INT_MAX);
	//	return closestDist;
	//}

	//int32 PlayerAttackDelayPenaltyPercent(int32 playerId)
	//{
	//	int32 distanceToBase = DistanceToPlayer(playerId);
	//	int32 delayPenalty = 0;
	//	if (distanceToBase > ArmyPenalty_MinDistance) {
	//		delayPenalty = ArmyAttackDelayPenaltyPercent_AtMaxDistance * (distanceToBase - ArmyPenalty_MinDistance) / (ArmyPenalty_MaxDistance - ArmyPenalty_MinDistance);
	//		delayPenalty = std::min(delayPenalty, ArmyAttackDelayPenaltyPercent_AtMaxDistance);
	//	}
	//	return delayPenalty;
	//}

	void operator>>(FArchive& Ar)
	{
		Ar << nodeId;
		Ar << originalPlayerId;
		Ar << lastLordPlayerId;
		Ar << lordPlayerId;

		SerializeVecObj(Ar, defendGroups);
		SerializeVecObj(Ar, attackGroups);
		SerializeVecObj(Ar, arrivingGroups);
		SerializeVecObj(Ar, rebelGroups);
	}

private:
	// Attacker here is the damage dealer (as oppose to city's attacker)
	void DealDamage(std::vector<ArmyGroup>& group_Attacker, std::vector<ArmyGroup>& group_Defender);

	void RefreshNodeState()
	{
		bool hadAttackGroup = attackGroups.size() > 0;

		// Remove dead groups
		CppUtils::RemoveIf(defendGroups, [&](ArmyGroup& group) { return group.isDead(); });
		CppUtils::RemoveIf(attackGroups, [&](ArmyGroup& group) { return group.isDead(); });


		// If only attackers survive, swap strongest army with land Conquer-Mode to be defender[0]
		if (defendGroups.size() == 0 && attackGroups.size() > 0)
		{
			// attackingGroups[0] becomes the owner
			defendGroups = attackGroups;
			attackGroups.clear();
			lastLordPlayerId = lordPlayerId;

			// Reset all defender's HPs
			for (int32 i = 0; i < defendGroups.size(); i++) {
				defendGroups[i].ResetUnitHP();
			}

			// The strongest army with Conquer_Mode assume control
			int32 strongestIndex = -1;
			int32 strongestCount = 10;
			for (int32 i = 0; i < defendGroups.size(); i++)
			{
				if (defendGroups[i].helpPlayerId == -1 ||
					defendGroups[i].playerId == originalPlayerId)
				{
					int32 count = defendGroups[i].TotalTroopCount();
					if (count > strongestCount) {
						strongestCount = count;
						strongestIndex = i;
					}
				}
			}

			// Swap the strongest index to front
			if (strongestIndex > 0) {
				std::swap(defendGroups[0], defendGroups[strongestIndex]);
			}

			// If there is no army with claim, the real owner assumes control with 0 army (will get a tower free after)
			if (strongestIndex == -1) {
				defendGroups.insert(defendGroups.begin(), ArmyGroup(originalPlayerId, nodeId, ArmyEnum::Tower, 0, originalPlayerId));
			}
			defendGroups[0].AddArmy(ArmyEnum::Tower, 1, 5);

			lordPlayerId = defendGroups[0].playerId;

			_simulation->SetNeedDisplayUpdate(DisplayGlobalEnum::Province, true);
		}

		// had attack group earlier but now gone, refresh unit's State
		if (hadAttackGroup && attackGroups.size() == 0)
		{
			// Reset all defender's HPs
			for (int32 i = 0; i < defendGroups.size(); i++) {
				defendGroups[i].ResetUnitHP();
			}
			//defendGroups[0].EnsureWall(originalPlayerId != -1 ? _simulation->townLvl(originalPlayerId) : 1);
		}
	}

private:
	IGameSimulationCore* _simulation = nullptr;
};

class ArmyUtils
{
public:
	static WorldTile2 GetArmyTile(ArmyGroup& armyGroup, IGameSimulationCore* simulation)
	{
		WorldTile2 endTile = simulation->buildingCenter(armyGroup.marchEndNodeId);
		if (armyGroup.marchStartNodeId == -1) {
			return endTile;
		}
		WorldTile2 startTile = simulation->buildingCenter(armyGroup.marchStartNodeId);

		return WorldTile2((endTile.x - startTile.x) * armyGroup.TicksUntilMarchComplete() / armyGroup.TotalMarchTicks() + startTile.x,
							(endTile.y - startTile.y) * armyGroup.TicksUntilMarchComplete() / armyGroup.TotalMarchTicks() + startTile.y);
	}
};