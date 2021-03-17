#pragma once

#include "Building.h"
#include "PunCity/NetworkStructs.h"
#include "PunCity/PunContainers.h"
#include "PunCity/CppUtils.h"
#include "PunCity/PunTimer.h"

class Building;

/**
 * 
 */
class BuildingSystem
{
public:
	void Init(IGameSimulationCore* simulation) {
		_simulation = simulation;
		_buildingSubregionList.Init();

		_townIdPlus1ToEnumToBuildingIds.resize(GameConstants::MaxPlayersAndAI + 1, std::vector<std::vector<int32>>(BuildingEnumCount));

		_buildingIdMap.resize(GameMapConstants::TilesPerWorld, -1);
		_buildingFrontIdMap.resize(GameMapConstants::TilesPerWorld * 4, -1);
	}

	void Tick();

    //! For Rendering
	const std::vector<WorldAtom2>& atomLocation() { return _atomLocation; }
	const std::vector<bool>& alive() { return _alive; }
	const std::vector<uint8>& buildingEnums() { return _buildingEnum; }

	CardEnum buildingEnum(int32 id) { return static_cast<CardEnum>(_buildingEnum[id]); }

	/*
	 * Map
	 */
	//CardEnum buildingEnumAtTile(WorldTile2 tile) { return _buildingEnumMap[tile.tileId()]; }
	int32 buildingIdAtTile(WorldTile2 tile) { return _buildingIdMap[tile.tileId()]; }

	bool HasBuilding(int32 tileId) {
		return _buildingIdMap[tileId] != -1;
	}
	
	bool HasNoBuildingOrFront(int32 tileId)
	{
		return _buildingIdMap[tileId] == -1 &&
			_buildingFrontIdMap[tileId * 4] == -1 &&
			_buildingFrontIdMap[tileId * 4 + 1] == -1 &&
			_buildingFrontIdMap[tileId * 4 + 2] == -1 &&
			_buildingFrontIdMap[tileId * 4 + 3] == -1;
	}

	std::vector<int32> frontBuildingIds(WorldTile2 tile) {
		int32 tileId = tile.tileId();
		std::vector<int32> buildingIds;
		if (_buildingFrontIdMap[tileId * 4] != -1) buildingIds.push_back(_buildingFrontIdMap[tileId * 4]);
		if (_buildingFrontIdMap[tileId * 4 + 1] != -1) buildingIds.push_back(_buildingFrontIdMap[tileId * 4 + 1]);
		if (_buildingFrontIdMap[tileId * 4 + 2] != -1) buildingIds.push_back(_buildingFrontIdMap[tileId * 4 + 2]);
		if (_buildingFrontIdMap[tileId * 4 + 3] != -1) buildingIds.push_back(_buildingFrontIdMap[tileId * 4 + 3]);
		return buildingIds;
	}

	/*
	 * Buildings
	 */

	WorldAtom2 actualAtomLocation(int id) const { return _atomLocation[id]; }

	Building& building(int32 id) { 
		PUN_CHECK2(id < _buildings.size() && id >= 0, "id:" + std::to_string(id) + " size:" + std::to_string(_buildings.size()));
		PUN_CHECK(_buildings[id] != nullptr);
		return *_buildings[id]; 
	}
	int32 buildingCount() { return _buildings.size(); }

	int32 isValidBuildingId(int32 id) {
		return _buildings.size() > id && id >= 0;
	}
	
	std::vector<std::unique_ptr<Building>>& buildings() { return _buildings; }

	int AddTileBuilding(WorldTile2 tile, CardEnum buildingEnum, int32 playerId);
	int AddBuilding(FPlaceBuilding parameters);
	void RemoveBuilding(int buildingId);

	void AddTickBuilding(int32 buildingId) {
		_buildingsToTick.push_back(buildingId);
	}
	void ScheduleTickBuilding(int32 buildingId, int32 scheduleTick) {
		_scheduleBuildingIds.push_back(buildingId);
		_scheduleTicks.push_back(scheduleTick);
	}
	void RemoveScheduleTickBuilding(int32 buildingId) {
		for (size_t i = _scheduleBuildingIds.size(); i-- > 0;) {
			if (_scheduleBuildingIds[i] == buildingId) {
				_scheduleBuildingIds.erase(_scheduleBuildingIds.begin() + i);
				_scheduleTicks.erase(_scheduleTicks.begin() + i);
				return;
			}
		}
		UE_DEBUG_BREAK();
	}

	void AddQuickBuild(int32 buildingId) {
		_quickBuildList.push_back(buildingId);
	}
	void RemoveQuickBuild(int32 buildingId) {
		CppUtils::TryRemove(_quickBuildList, buildingId);
	}
	bool IsQuickBuild(int32 buildingId) {
		for (int32 quickBuildId : _quickBuildList) {
			if (quickBuildId == buildingId) {
				return true;
			}
		}
		return false;
	}
	

	bool alive(int id) { return _alive[id]; }

	const SubregionLists<int32>& buildingSubregionList() { return _buildingSubregionList; }

	const std::vector<int32>& buildingIds(int32 townId, CardEnum buildingEnum) {
		return _townIdPlus1ToEnumToBuildingIds[townId + 1][static_cast<int>(buildingEnum)];
	}
	size_t GetBuildingCount(int32 townId, CardEnum buildingEnum) {
		return buildingIds(townId, buildingEnum).size();
	}
	void AddTown(int32 townId) {
		_townIdPlus1ToEnumToBuildingIds.push_back(std::vector<std::vector<int32>>(BuildingEnumCount));
		check(_townIdPlus1ToEnumToBuildingIds.size() == townId + 2);
	}

	// Change all _playerId in buildings
	void ChangeTownOwningPlayer(int32 townId, int32 newPlayerId)
	{
		std::vector<std::vector<int32>>& enumToBuildingIds = _townIdPlus1ToEnumToBuildingIds[townId + 1];
		for (std::vector<int32>& bldIds : enumToBuildingIds) {
			for (int32 bldId : bldIds) {
				building(bldId).ChangeTownOwningPlayer(newPlayerId);
			}
		}
	}
	

	int32 GetHouseLvlCount(int32 playerId, int32 houseLvl, bool includeHigherLvl);
	

	void StartFire(int32 buildingId) {
		building(buildingId).StartFire();
		CppUtils::TryAdd(_buildingsOnFire, buildingId);
	}
	void StopFire(int32 buildingId) {
		building(buildingId).StopFire();
		CppUtils::TryRemove(_buildingsOnFire, buildingId);
	}

	int8 IsConnectedBuilding(int32 buildingId) {
		PUN_CHECK(buildingId < _isBuildingIdConnected.size());
		return _isBuildingIdConnected[buildingId];
	}
	void OnRefreshFloodGrid(WorldRegion2 region)
	{
		_buildingSubregionList.ExecuteRegion(region, [&](int32 buildingId)
		{
			Building& bld = building(buildingId);
			int32 townId = bld.townId();
			if (townId != -1) {
				RefreshIsBuildingConnected(townId, buildingId, bld.gateTile());
			}
		});
	}
	

	int32 GetSyncHash() {
		int32 hash = 0;
		for (size_t i = _atomLocation.size(); i-- > 0;) {
			hash += _atomLocation[i].x;
			hash += _atomLocation[i].y;
			hash += static_cast<int32>(_buildingEnum[i]);
			hash += static_cast<int32>(_alive[i]);
		}

		hash += _buildingsToTick.size();
		hash += _buildingsOnFire.size();
		hash += _quickBuildList.size();
		hash += _scheduleBuildingIds.size();
		hash += _scheduleTicks.size();
		return hash;
	}

	/*
	 * Serialize
	 */

	void Serialize(FArchive& Ar, TArray<uint8>& data, std::vector<int32>* crcs, std::vector<std::string>* crcLabels, bool isForMainMenu)
	{
		{
			SERIALIZE_TIMER("Building - _atomLocation", data, crcs, crcLabels);
			SerializeVecObj(Ar, _atomLocation);
		}

		{
			SERIALIZE_TIMER("Building - _buildingSubregionList", data, crcs, crcLabels);
			_buildingSubregionList.Serialize(Ar);
		}

		{
			SERIALIZE_TIMER("Building - _buildingEnum", data, crcs, crcLabels);
			SerializeVecValue(Ar, _buildingEnum);
		}
		
		{
			SERIALIZE_TIMER("Building - BeforeLoop", data, crcs, crcLabels);

			SerializeVecBool(Ar, _alive);

			//PUN_CRC_LOG("Building - BeforeLoop", Ar, data);
		}

		{
			SERIALIZE_TIMER("Building - Loop", data, crcs, crcLabels);
			
			SerializeVecLoop(Ar, _buildings, [&](std::unique_ptr<Building>& building) {
				SerializePtr<std::unique_ptr<Building>, CardEnum>(Ar, building, [&](CardEnum buildingEnum) {
					//PUN_LOG("CreateBuilding %d", static_cast<int>(buildingEnum));
					CreateBuilding(buildingEnum, building);
					if (Ar.IsLoading()) {
						building->LoadInit(_simulation);
					}
				});
			});

			//PUN_CRC_LOG("Building - Loop", Ar, data);
		}

		{
			SERIALIZE_TIMER("Building - AfterLoop", data, crcs, crcLabels);
			
			SerializeVecLoop(Ar, _townIdPlus1ToEnumToBuildingIds, [&](std::vector<std::vector<int32>>& vecVecValue) {
				SerializeVecVecValue(Ar, vecVecValue);
			});

			SerializeVecValue(Ar, _buildingsToTick);
			SerializeVecValue(Ar, _buildingsOnFire);
			SerializeVecValue(Ar, _quickBuildList);

			SerializeVecValue(Ar, _scheduleBuildingIds);
			SerializeVecValue(Ar, _scheduleTicks);

			SerializeVecValue(Ar, _isBuildingIdConnected);

			// Check
			for (size_t i = 0; i < _buildings.size(); i++) {
				int32 id = _buildings[i]->buildingId();
				PUN_CHECK(id == i);
			}

			//PUN_CRC_LOG("Building - AfterLoop", Ar, data);
		}

		{	
			SCOPE_TIMER("Building - Map");
			// Loop through all buildings putting them onto maps
			if (Ar.IsLoading()) {
				VecReset(_buildingIdMap, -1);
				VecReset(_buildingFrontIdMap, -1);
				
				for (size_t i = 0; i < _buildings.size(); i++) {
					if (_alive[i]) {
						PlaceBuildingOnMap(i, false);
					}
				}
			}

			// Main menu doesn't need CRC check since not all buildings are loaded.
			if (!isForMainMenu)
			{
				SerializeCrc(Ar, _buildingIdMap);
				SerializeCrc(Ar, _buildingFrontIdMap);
			}
		}
	}
	
private:
	void CreateBuilding(CardEnum buildingEnum, std::unique_ptr<Building>& building);
	
	//void RemoveBuildingFromBuildingSystem(int32 buildingId);

	void SetBuildingTile(WorldTile2 tile, int32 buildingId) {
		_buildingIdMap[tile.tileId()] = buildingId;
	}

	void SetFrontTile(WorldTile2 tile, Direction faceDirection, int32 buildingId) {
		_buildingFrontIdMap[tile.tileId() * 4 + static_cast<int32>(faceDirection)] = buildingId;
	}

	void PlaceBuildingOnMap(int32 buildingIdIn, bool isBuildingInitialAdd, bool isAdding = true);

	void RefreshIsBuildingConnected(int32 townId, int32 buildingId, WorldTile2 tile)
	{
		if (townId != -1)
		{
			PUN_CHECK(buildingId < _isBuildingIdConnected.size());
			WorldTile2 townGate = _simulation->GetTownhallGate(townId);

			if (townGate.isValid()) {
				DEBUG_ISCONNECTED_VAR(RefreshIsBuildingConnected);
				_isBuildingIdConnected[buildingId] = _simulation->IsConnected(townGate, tile, GameConstants::MaxFloodDistance_HumanLogistics, true);
				return;
			}
		}
		_isBuildingIdConnected[buildingId] = false;
	}

private:
	IGameSimulationCore* _simulation = nullptr;

	//! Lookup
	SubregionLists<int32> _buildingSubregionList;

	//! Per Building
	std::vector<WorldAtom2> _atomLocation;
	std::vector<uint8> _buildingEnum;

	std::vector<bool> _alive;
	std::vector<std::unique_ptr<Building>> _buildings;

	std::vector<std::vector<std::vector<int32>>> _townIdPlus1ToEnumToBuildingIds;

	//
	std::vector<int32> _buildingsToTick;
	std::vector<int32> _buildingsOnFire;
	std::vector<int32> _quickBuildList;

	std::vector<int32> _scheduleBuildingIds;
	std::vector<int32> _scheduleTicks;

	std::vector<int8> _isBuildingIdConnected;// IsConnected for buildings


private:
	/*
	 * Calculate on load
	 */
	std::vector<int32> _buildingIdMap;
	std::vector<int32> _buildingFrontIdMap; // x4 size of total tiles... 4 represents each facing directions of buildings using this tile..
};