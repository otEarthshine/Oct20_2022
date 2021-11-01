// Pun Dumnernchanvanit's


#include "ArmyNode.h"
#include "Building.h"

using namespace std;

void ArmyNode::MarchStart(ArmyGroup group, const ArmyNode& originNode, WorldTile2 startTile)
{
	if (!startTile.isValid()) {
		startTile = _simulation->buildingCenter(originNode.nodeId);
	}
	WorldTile2 endTile = _simulation->buildingCenter(nodeId);

	int32 marchTimeTicks = WorldTile2::Distance(startTile, endTile) * Time::TicksPerSecond;

	group.marchStartNodeId = originNode.nodeId;
	group.marchEndNodeId = nodeId;
	group.marchStartTick = Time::Ticks();
	group.marchEndTick = Time::Ticks() + marchTimeTicks;
	arrivingGroups.push_back(group);
}

void ArmyNode::DealDamage(std::vector<ArmyGroup>& group_Attacker, std::vector<ArmyGroup>& group_Defender)
{
	auto getGroupIdArmyEnumPairs = [&](const std::vector<ArmyGroup>& groups, 
										std::vector<pair<int32, int32>>& groupIdToArmyEnum_Front, 
										std::vector<pair<int32, int32>>& groupIdToArmyEnum_Back)
	{
		for (int32 groupId = 0; groupId < groups.size(); groupId++)
		{
			std::vector<int32> armyHps = groups[groupId].HPs;
			for (int32 enumInt = 0; enumInt < ArmyEnumCount; enumInt++) {
				if (armyHps[enumInt] > 0) {
					if (IsFrontline(enumInt)) {
						groupIdToArmyEnum_Front.push_back(make_pair(groupId, enumInt));
					} else {
						groupIdToArmyEnum_Back.push_back(make_pair(groupId, enumInt));
					}
				}
			}
		}
	};

	std::vector<pair<int32, int32>> groupIdToArmyEnum_Front_Attacker;
	std::vector<pair<int32, int32>> groupIdToArmyEnum_Back_Attacker;
	getGroupIdArmyEnumPairs(group_Attacker, groupIdToArmyEnum_Front_Attacker, groupIdToArmyEnum_Back_Attacker);
	
	std::vector<pair<int32, int32>> groupIdToArmyEnum_Front_Defender;
	std::vector<pair<int32, int32>> groupIdToArmyEnum_Back_Defender;
	getGroupIdArmyEnumPairs(group_Defender, groupIdToArmyEnum_Front_Defender, groupIdToArmyEnum_Back_Defender);

	auto dealDamage = [&](std::vector<ArmyGroup>& group_Dealer, std::vector<ArmyGroup>& group_Receiver,
		std::vector<pair<int32, int32>>& groupIdToArmyEnum_Dealer,
		std::vector<pair<int32, int32>> groupIdToArmyEnum_Front_Receiver,
		std::vector<pair<int32, int32>> groupIdToArmyEnum_Back_Receiver)
	{
		
		//// Check each damage dealer if it can attack...
		//for (int32 i = 0; i < groupIdToArmyEnum_Dealer.size(); i++)
		//{
		//	auto pairDealer = groupIdToArmyEnum_Dealer[i];
		//	ArmyGroup& dealerGroup = group_Dealer[pairDealer.first];
		//	int32 dealerEnumInt = pairDealer.second;

		//	auto getAttackDelayPenalty = [&]() {
		//		return PlayerAttackDelayPenaltyPercent(dealerGroup.playerId);
		//	};

		//	if (dealerGroup.IsReadyToHit(dealerEnumInt, getAttackDelayPenalty))
		//	{
		//		dealerGroup.lastAttackedTick[dealerEnumInt] = Time::Ticks();
		//		
		//		//int32 troopCount = dealerGroup.TroopCount(dealerEnumInt);
		//		//int32 attack = troopCount * dealerInfo.attack * BaseDamagePerAttack;

		//		//// Randomize attack
		//		//attack = attack * (GameRand::Rand() % 100 + 50) / 100;
		//		
		//		int32 attack = dealerGroup.AttackDamage(dealerEnumInt);

		//		if (_simulation->IsResearched(dealerGroup.playerId, TechEnum::MilitaryLastEra)) 	{
		//			attack = attack * 120 / 100;
		//		}

		//		// Damage frontline first
		//		pair<int32, int32> pairReceiver;
		//		if (groupIdToArmyEnum_Front_Receiver.size() > 0) {
		//			pairReceiver = groupIdToArmyEnum_Front_Receiver[GameRand::Rand() % groupIdToArmyEnum_Front_Receiver.size()];
		//		} else if (groupIdToArmyEnum_Back_Receiver.size() > 0) {
		//			pairReceiver = groupIdToArmyEnum_Back_Receiver[GameRand::Rand() % groupIdToArmyEnum_Back_Receiver.size()];
		//		}
		//		else {
		//			break; // all dead...
		//		}
		//		
		//		ArmyGroup& receiverGroup = group_Receiver[pairReceiver.first];
		//		int32 receiverEnumInt = pairReceiver.second;
		//		ArmyInfo receiverInfo = GetArmyInfoInt(receiverEnumInt);

		//		receiverGroup.TakeDamage(receiverEnumInt, attack);
		//	}
		//}
	};

	// Defender deal damage first
	// Front line, then Back line
	dealDamage(group_Defender, group_Attacker,
		groupIdToArmyEnum_Front_Defender,
		groupIdToArmyEnum_Front_Attacker,
		groupIdToArmyEnum_Back_Attacker);
	dealDamage(group_Defender, group_Attacker,
		groupIdToArmyEnum_Back_Defender,
		groupIdToArmyEnum_Front_Attacker,
		groupIdToArmyEnum_Back_Attacker);

	// Attacker deal damage
	dealDamage(group_Attacker, group_Defender,
		groupIdToArmyEnum_Front_Attacker,
		groupIdToArmyEnum_Front_Defender,
		groupIdToArmyEnum_Back_Defender);
	dealDamage(group_Attacker, group_Defender,
		groupIdToArmyEnum_Back_Attacker,
		groupIdToArmyEnum_Front_Defender,
		groupIdToArmyEnum_Back_Defender);
	
}