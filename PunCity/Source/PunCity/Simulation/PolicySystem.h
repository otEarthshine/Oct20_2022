//// Fill out your copyright notice in the Description page of Project Settings.
//
//#pragma once
//
//#include "Policy.h"
//#include "GameSimulationInfo.h"
//#include "IGameSimulationCore.h"
//#include "PunCity/NetworkStructs.h"
//
//#include <unordered_map>
//#include <memory>
//
///**
// * 
// */
//class PolicySystem : public IPolicySystem
//{
//public:
//	PolicySystem(int32_t playerId, IGameSimulationCore* simulation) {
//		_playerId = playerId;
//		_simulation = simulation;
//
//		//AddLockedCard(PolicyEnum::CutItAllDown, std::make_shared<CutItAllDown>());
//		//AddLockedCard(PolicyEnum::ChildMarriage, std::make_shared<ChildMarriage>());
//		//AddLockedCard(PolicyEnum::Trade, std::make_shared<TradeCard>());
//		//AddLockedCard(PolicyEnum::SeedExtraction, std::make_shared<SeedExtraction>());
//
//		//AddLockedCard(PolicyEnum::Forage, std::make_shared<ForageCard>());
//		//AddLockedCard(PolicyEnum::ColdResist, std::make_shared<ColdResist>());
//		//AddLockedCard(PolicyEnum::MoreTax, std::make_shared<MoreTax>());
//		//AddLockedCard(PolicyEnum::SmelterBonus, std::make_shared<SmelterBonus>());
//		//AddLockedCard(PolicyEnum::FarmBonus, std::make_shared<FarmBonus>());
//
//		MoveCardFromLockedToAvailable(PolicyEnum::ChildMarriage);
//		MoveCardFromLockedToAvailable(PolicyEnum::CutItAllDown);
//		MoveCardFromLockedToAvailable(PolicyEnum::Forage);
//		MoveCardFromLockedToAvailable(PolicyEnum::ColdResist);
//		MoveCardFromLockedToAvailable(PolicyEnum::SeedExtraction);
//
//		//MoveCardFromLockedToAvailable(PolicyEnum::Trade);
//	}
//	virtual ~PolicySystem() {}
//	
//	//! SyncPolicy comes from player through network
//	//void SyncPolicy(FPoliciesSelects policies);
//
//	inline const std::unordered_map<PolicyEnum, std::shared_ptr<Policy>>& cardsAvailable() { return _policiesAvailable; }
//	inline std::unordered_map<PolicyEnum, std::shared_ptr<Policy>>& cardsInUse() { return _policiesInUsed; }
//
//	void MoveCardFromLockedToAvailable(PolicyEnum policyEnum) override {
//		check(_policiesLocked.find(policyEnum) != _policiesLocked.end())
//		check(_policiesAvailable.find(policyEnum) == _policiesAvailable.end())
//		_policiesAvailable[policyEnum] = _policiesLocked[policyEnum];
//		_policiesLocked.erase(policyEnum);
//	}
//	void MoveCardFromAvailableToInUse(PolicyEnum policyEnum) override {
//		check(_policiesAvailable.find(policyEnum) != _policiesAvailable.end())
//		check(_policiesInUsed.find(policyEnum) == _policiesInUsed.end())
//		_policiesInUsed[policyEnum] = _policiesAvailable[policyEnum];
//		_policiesAvailable.erase(policyEnum);
//
//		_policiesInUsed[policyEnum]->OnSetActive(true);
//
//		UE_LOG(LogTemp, Error, TEXT("MoveCardFromAvailableToInUse %s"), *FString(_policiesInUsed[policyEnum]->name().c_str()));
//	}
//	void MoveCardFromInUseToBin(PolicyEnum policyEnum) override {
//		check(_policiesInUsed.find(policyEnum) != _policiesInUsed.end());
//		UE_LOG(LogTemp, Error, TEXT("MoveCardFromInUseToBin %s"), *FString(_policiesInUsed[policyEnum]->name().c_str()));
//		_policiesInUsed.erase(policyEnum);
//	}
//
//private:
//	void AddLockedCard(PolicyEnum policyEnum, std::shared_ptr<Policy> policy) 
//	{
//		policy->policyEnum = policyEnum;
//		policy->playerId = _playerId;
//		policy->simulation = _simulation;
//		_policiesLocked[policyEnum] = policy;
//	}
//
//private:
//	int8_t _playerId;
//	IGameSimulationCore* _simulation;
//
//	std::unordered_map<PolicyEnum, std::shared_ptr<Policy>> _policiesLocked;
//	std::unordered_map<PolicyEnum, std::shared_ptr<Policy>> _policiesAvailable;
//	std::unordered_map<PolicyEnum, std::shared_ptr<Policy>> _policiesInUsed;
//
//	//std::vector<std::string> _popupsToDisplay;
//};
