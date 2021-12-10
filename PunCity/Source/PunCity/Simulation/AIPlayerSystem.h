// Pun Dumnernchanvanit's

#pragma once

#include "IGameSimulationCore.h"
#include "PlayerOwnedManager.h"
#include "GeoresourceSystem.h"
#include "PunCity/NetworkStructs.h"
#include "Resource/ResourceSystem.h"
#include "GlobalResourceSystem.h"
#include "PunCity/DisplaySystem/PunTerrainGenerator.h"
#include "TreeSystem.h"
#include "BuildingSystem.h"
#include "Buildings/House.h"
#include "Buildings/TownHall.h"
#include "PunCity/CppUtils.h"
#include "SimUtils.h"
#include "Buildings/GathererHut.h"

DECLARE_CYCLE_STAT(TEXT("PUN: [AI]Tick1Sec"), STAT_PunAITick1Sec, STATGROUP_Game);

DECLARE_CYCLE_STAT(TEXT("PUN: [AI]PlaceCityBlock"), STAT_PunAIPlaceCityBlock, STATGROUP_Game);
DECLARE_CYCLE_STAT(TEXT("PUN: [AI]PlaceForestBlock"), STAT_PunAIPlaceForestBlock, STATGROUP_Game);
DECLARE_CYCLE_STAT(TEXT("PUN: [AI]PlaceFarm"), STAT_PunAIPlaceFarm, STATGROUP_Game);

DECLARE_CYCLE_STAT(TEXT("PUN: [AI]UpgradeTownhall"), STAT_PunAIUpgradeTownhall, STATGROUP_Game);
DECLARE_CYCLE_STAT(TEXT("PUN: [AI]Trade"), STAT_PunAITrade, STATGROUP_Game);
DECLARE_CYCLE_STAT(TEXT("PUN: [AI]ClaimProvince"), STAT_PunAIClaimProvince, STATGROUP_Game);

class AIRegionStatus
{
public:
	bool currentPurposeDetermined() {
		return currentPurpose != AIRegionPurposeEnum::None;
	}

	void operator>>(FArchive& Ar)
	{
		Ar << provinceId;
		Ar << proposedPurpose;
		Ar << currentPurpose;

		proposedBlock >> Ar;
		Ar << blockPlaced;

		Ar << updatedThisRound;
	}

public:
	int32 provinceId = -1;

	// ProposedPurpose is determined at Tick1Sec
	//  It is more general
	AIRegionProposedPurposeEnum proposedPurpose = AIRegionProposedPurposeEnum::None;

	// currentPurpose is determined later
	AIRegionPurposeEnum currentPurpose = AIRegionPurposeEnum::None;

	AICityBlock proposedBlock;
	bool blockPlaced = false;
	
	bool updatedThisRound = true;
};

/**
 * 
 */
class AIPlayerSystem
{
public:
	AIPlayerSystem(int32 playerId, IGameSimulationCore* simulation, IPlayerSimulationInterface* playerInterface)
	{
		_aiPlayerId = playerId;
		_simulation = simulation;
		_playerInterface = playerInterface;

		_aiArchetypeEnum = static_cast<AIArchetypeEnum>(_aiPlayerId % AIArchetypeCount);
	}

	AIArchetypeEnum aiArchetypeEnum() { return _aiArchetypeEnum; }
	const AIArchetypeInfo& aiArchetypeInfo() {
		return GetAIArchetypeInfo(_aiArchetypeEnum);
	}

	FactionEnum factionEnum() { return aiArchetypeInfo().factionEnum(); }

	void SetActive(bool active) {
		_active = active;
	}
	bool active() { return _active; }

	AIRegionStatus* regionStatus(int32 provinceId) {
		auto it = std::find_if(_regionStatuses.begin(), _regionStatuses.end(), [&](AIRegionStatus& status) {
			return status.provinceId == provinceId;
		});
		if (it == _regionStatuses.end()) {
			return nullptr;
		}
		return &(*it);
	}


	/*
	 * 
	 */
	void Tick1Sec();

	void TryChooseLocation();
	void TryBuildTownhall();
	void Tick1Sec_DoAction();

	void TryUpgradeBuildings();

	void TryTrade();

	void TryProvinceClaim();

	//
	
	/*
	 * System
	 */

	void Serialize(FArchive& Ar)
	{
		Ar << _aiArchetypeEnum;
		
		SerializeVecObj(Ar, _regionStatuses);
		Ar << _active;

		Ar << _needMoreTradeBuilding;
		Ar << _totalCheatedMoney;
	}

	void AIDebugString(std::stringstream& ss) {
		ss << "-- AI Regions " << _simulation->playerName(_aiPlayerId) << "\n";
		for (AIRegionStatus& status : _regionStatuses) {
			ss << " Region " << status.provinceId << " purpose:" << AIRegionPurposeName[static_cast<int>(status.currentPurpose)] << " proposed:" << static_cast<int>(status.proposedPurpose);
		}
	}

	int32 totalCheatedMoney() { return _totalCheatedMoney; }

private:
	template<class T>
	std::shared_ptr<T> MakeCommand() {
		auto command = std::make_shared<T>();
		std::static_pointer_cast<FNetworkCommand>(command)->playerId = _aiPlayerId;
		return command;
	}

	AIRegionProposedPurposeEnum DetermineProvincePurpose(int32 provinceId)
	{
		auto& terrainGenerator = _simulation->terrainGenerator();
		auto& provinceSys = _simulation->provinceSystem();
		
		int32 treeCount = _simulation->GetTreeCount(provinceId);
		int32 fertility = terrainGenerator.GetRegionFertility(provinceSys.GetProvinceCenterTile(provinceId).region()); // Rough fertility calculation
		int32 flatLandCount = provinceSys.provinceFlatTileCount(provinceId);
		
		if (fertility > 70) {
			return AIRegionProposedPurposeEnum::Fertility;
		}
		if (treeCount > CoordinateConstants::TileIdsPerRegion / 4 / 2) {
			return AIRegionProposedPurposeEnum::Tree;
		}
		if (fertility > 60) {
			return AIRegionProposedPurposeEnum::Fertility;
		}
		//if (fertility > 30 && flatLandCount > CoordinateConstants::TileIdsPerRegion * 7/8 ) {
		//	return AIRegionProposedPurposeEnum::Ranch;
		//}
		//if (treeCount > CoordinateConstants::TileIdsPerRegion / 4 / 4) {
		//	return AIRegionProposedPurposeEnum::Tree;
		//}
		//if (fertility > 50) {
		//	return AIRegionProposedPurposeEnum::Fertility;
		//}
		if (fertility > 30) {
			return AIRegionProposedPurposeEnum::Ranch;
		}

		return AIRegionProposedPurposeEnum::None;
	}

	//! Helpers
	void TryPlaceCityBlock(CardEnum buildingEnumToBeNear, AICityBlock& block);
	void PlaceBuilding(CardEnum cardEnum, WorldTile2 centerTile, Direction faceDirection);
	bool TryPlaceSingleCoastalBuilding(CardEnum cardEnum);
	void TryPlaceMines();

	void TryPlaceCityBlock(std::vector<CardEnum> buildingEnumToBeNear, AICityBlock& block)
	{
		for (CardEnum buildingEnum : buildingEnumToBeNear)
		{
			if (!block.PlacementSucceed()) {
				TryPlaceCityBlock(buildingEnum, block);
			}
		}
	}

	void TryPlaceNormalBuildings_ScaleToPopCount(AICityBlock& block, int32 era, int32 population);
	void TryPlaceNormalBuildings_Single(AICityBlock& block, int32 era);

	TCHAR* AIPrintPrefix() {
		return _simulation->AIPrintPrefix(_aiPlayerId);
	}
	
private:
	int32 _aiPlayerId;
	IGameSimulationCore* _simulation;
	IPlayerSimulationInterface* _playerInterface;

	/*
	 * Serialize
	 */
	AIArchetypeEnum _aiArchetypeEnum;
	
	std::vector<AIRegionStatus> _regionStatuses;
	bool _active = false;

	bool _needMoreTradeBuilding = false;
	int32 _totalCheatedMoney = 0;
};
