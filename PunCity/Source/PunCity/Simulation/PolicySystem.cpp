//// Fill out your copyright notice in the Description page of Project Settings.
//
//#include "PolicySystem.h"
//#include "UnrealEngine.h"
//
//using namespace std;
//
////void PolicySystem::SyncPolicy(FPoliciesSelects policies)
////{
////	TArray<uint8> policyArray = policies.policyArray;
////	
////	// Add policies to in-use map
////	for (int i = 0; i < policyArray.Num(); i++) {
////		// Not yet in-use, add the card
////		PolicyEnum policyEnum = (PolicyEnum)policyArray[i];
////		if (_policiesInUsed.find(policyEnum) == _policiesInUsed.end()) {
////			MoveCardFromAvailableToInUse(policyEnum);
////			PUN_DEBUG(FString::Printf(TEXT("MoveCardFromAvailableToInUse: %s pid:%d"), *FString(_policiesInUsed[policyEnum]->name().c_str()), _playerId));
////		}
////	}
////
////	// Remove policies from in-use map
////	for (auto it = _policiesInUsed.begin(); it != _policiesInUsed.end();) {
////		// Was in-use but was put back to available
////		// Since SyncPolicy is chosen by people, cards will always bounce back to policiesAvailable
////		if (!policyArray.Contains((uint8)it->first)) {
////			check(_policiesAvailable.find(it->first) == _policiesAvailable.end())
////			_policiesInUsed[it->first]->OnSetActive(false);
////			_policiesAvailable[it->first] = _policiesInUsed[it->first];
////			//PUN_DEBUG(FString::Printf(TEXT("SyncSetCardInactive: %s pid:%d"), *FString(it->second->name().c_str()), _playerId)); // Here just in case it was deleted
////			it = _policiesInUsed.erase(it);
////		} else {
////			it++;
////		}
////	}
////
////}