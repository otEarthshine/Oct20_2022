#pragma once

#include "GameSimulationInfo.h"

#include "UnitSystem.h"
#include "BuildingSystem.h"
#include "TreeSystem.h"
#include "GameRegionSystem.h"
#include "GeoresourceSystem.h"
#include "OverlaySystem.h"
#include "StatSystem.h"
#include "ResourceDropSystem.h"
#include "../GameMapFlood.h"

#include "Resource/ResourceSystem.h"
#include "PolicySystem.h"
#include "QuestSystem.h"
#include "UnlockSystem.h"
#include "PlayerOwnedManager.h"
#include "PlayerParameters.h"
#include "PopupSystem.h"

#include "../DisplaySystem/PunTerrainGenerator.h"

#include "DebugLineSystem.h"

#include "Buildings/Townhall.h"

#include "IGameSimulationCore.h"
#include "PunCity/GameManagerInterface.h"
#include "../DisplaySystem/TerrainGeneratorDataSource.h"
#include "PunCity/NetworkStructs.h"

#include "UnitStateAI.h"
#include "BuildingCardSystem.h"
#include "ReplaySystem.h"
#include "ChatSystem.h"
#include "EventLogSystem.h"
#include "WorldTradeSystem.h"

#include "PunCity/AlgorithmUtils.h"
#include "PunTerrainChanges.h"
#include "AIPlayerSystem.h"

#include "ProvinceSystem.h"

#include "PunCity/PunTimer.h"

#include "Misc/Crc.h"
#include "Buildings/StorageYard.h"
#include "HumanStateAI.h"
#include "Dom/JsonObject.h"


DECLARE_LOG_CATEGORY_EXTERN(LogNetworkInput, All, All);
// log mylog NoLogging
// DrawDebugString()!!!!


/**
 * 
 */
class GameSimulationCore : public IGameSimulationCore, public IPlayerSimulationInterface
{
public:
	virtual ~GameSimulationCore() {}

	void Init(IGameManagerInterface* gameManager, IGameSoundInterface* soundInterface, IGameUIInterface* uiInterface, bool isLoadingFromFile);

	int tickCount() { return _tickCount; }

	/**
	 * Tick and sample for render
	 */

	void Tick(int bufferCount, NetworkTickInfo& tickInfo);

	TArray<int32> GetTickHashes(int32 startTick) {
		TArray<int32> tickHashesFromStartTick;
		for (int i = startTick; i < _tickHashes.size(); i++) {
			tickHashesFromStartTick.Add(_tickHashes[i]);
		}
		return tickHashesFromStartTick;
	}
	void AppendAndCompareServerHashes(int32 insertIndex, TArray<int32> newServerTickHashes)
	{
		_LOG(PunTickHash, "AppendAndCompareServerHashes_Before %d %d", _tickHashes.size(), _serverTickHashes.size());
		
		PUN_CHECK(insertIndex <= _serverTickHashes.size());
		
		int32 index = insertIndex;
		for (int32 i = 0; i < newServerTickHashes.Num(); i++) {
			if (index < _serverTickHashes.size()) {
				_serverTickHashes[index] = newServerTickHashes[i];
			} else {
				_serverTickHashes.push_back(newServerTickHashes[i]);
			}
			index++;
		}
		int32 checkSize = std::min(_tickHashes.size(), _serverTickHashes.size());
		for (int32 i = 0; i < checkSize; i++) {
			PUN_CHECK(_tickHashes[i] == _serverTickHashes[i]);
		}

		_LOG(PunTickHash, "AppendAndCompareServerHashes_After %d %d", _tickHashes.size(), _serverTickHashes.size());
	}

	/*
	 * Subsystems
	 */

	UnitSystem& unitSystem() final { return *_unitSystem; }
	BuildingSystem& buildingSystem() final { return *_buildingSystem; }
	TreeSystem& treeSystem() final { return *_treeSystem; }
	GeoresourceSystem& georesourceSystem() final { return *_georesourceSystem; }
	OverlaySystem& overlaySystem() final { return _overlaySystem; }
	StatSystem& statSystem() final { return _statSystem; }
	PunTerrainGenerator& terrainGenerator() final { return *_terrainGenerator; }
	GameMapFlood& floodSystem() final { return _floodSystem; }
	GameMapFlood& floodSystemHuman() final { return _floodSystemHuman; }
	ProvinceSystem& provinceSystem() final { return _provinceSystem; }
	GameRegionSystem& regionSystem() final { return *_regionSystem; }
	
	ResourceDropSystem& dropSystem() final { return _dropSystem; }
	//GameEventSource& eventSource(EventSourceEnum eventEnum) final { return _gameEventSystem.source(eventEnum); }

	PlayerOwnedManager& playerOwned(int32 playerId) final { return _playerOwnedManagers[playerId]; }
	ResourceSystem& resourceSystem(int32 playerId) final { return _resourceSystems[playerId]; }
	QuestSystem* questSystem(int32 playerId) final { return playerId < _questSystems.size() ? &_questSystems[playerId] : nullptr; }
	UnlockSystem* unlockSystem(int32 playerId) final { return playerId < _unlockSystems.size() ? &_unlockSystems[playerId] : nullptr; }
	PlayerParameters* parameters(int32 playerId) final { return playerId < _unlockSystems.size() ? &_playerParameters[playerId] : nullptr; }
	SubStatSystem& statSystem(int32 playerId) final { return _statSystem.playerStatSystem(playerId); }

	BuildingCardSystem& cardSystem(int32 playerId) final { return _cardSystem[playerId]; }

	ChatSystem& chatSystem() { return _chatSystem; }
	EventLogSystem& eventLogSystem() { return _eventLogSystem; }

	WorldTradeSystem& worldTradeSystem() final { return _worldTradeSystem; }

	ReplaySystem& replaySystem() { return _replaySystem; }

	PunTerrainChanges& terrainChanges() final { return _terrainChanges; }

	SubStatSystem& playerStatSystem(int32 playerId) {
		return statSystem().playerStatSystem(playerId);
	}

	//inline IUnitDataSource& unitDataSource() final { return *static_cast<IUnitDataSource*>(_unitSystem.get()); }

	IGameSoundInterface* soundInterface() final { return _soundInterface; }
	IGameUIInterface* uiInterface() final { return _uiInterface; }
	IGameManagerInterface* gameManagerInterface() final { return _gameManager; }

	DebugLineSystem& debugLineSystem() final { return _debugLineSystem; }

	AIPlayerSystem& aiPlayerSystem(int32_t playerId) { return _aiPlayerSystem[playerId]; }

	int32 playerCount() final {
		return _gameManager->playerCount();
	}
	
	int32 population(int32 playerId) final {
		return _playerOwnedManagers[playerId].population();
	}
	int32 worldPlayerPopulation() final
	{
		int32 result = 0;
		ExecuteOnPlayersAndAI([&](int32 playerId) {
			result += population(playerId);
		});
		return result;
	}
	
	int32 resourceCount(int32 playerId, ResourceEnum resourceEnum) final {
		return _resourceSystems[playerId].resourceCount(resourceEnum);
	}
	int32 resourceCountWithPop(int32 playerId, ResourceEnum resourceEnum) final {
		return _resourceSystems[playerId].resourceCountWithPop(resourceEnum);
	}
	int32 resourceCountWithDrops(int32 playerId, ResourceEnum resourceEnum) final {
		return _resourceSystems[playerId].resourceCountWithDrops(resourceEnum);
	}

	bool HasEnoughResource(int32 playerId, const std::vector<ResourcePair> pairs) {
		for (const ResourcePair& pair : pairs) 
		{
			if (pair.resourceEnum == ResourceEnum::Food) {
				if (foodCount(playerId) < pair.count) {
					return false;
				}
			}
			else {
				if (resourceCount(playerId, pair.resourceEnum) < pair.count) {
					return false;
				}
			}
		}
		return true;
	}

	void AddResourceGlobal(int32 playerId, ResourceEnum resourceEnum, int32 amount) final {
		resourceSystem(playerId).AddResourceGlobal(resourceEnum, amount, *this);
	}

	bool IsOutputTargetReached(int32 playerId, ResourceEnum resourceEnum) final {
		if (resourceEnum == ResourceEnum::None) {
			return false;
		}
		int32 outputTarget = playerOwned(playerId).GetOutputTarget(resourceEnum);
		return outputTarget != -1 && resourceSystem(playerId).resourceCountWithPop(resourceEnum) >= outputTarget;
	}
	bool IsFarBelowOutputTarget(int32 playerId, ResourceEnum resourceEnum) {
		if (resourceEnum == ResourceEnum::None) {
			return false;
		}
		int32 outputTarget = playerOwned(playerId).GetOutputTarget(resourceEnum);
		if (outputTarget == -1) {
			return false;
		}
		int32 belowThreshold = std::min(outputTarget * 8 / 10, std::max(outputTarget - 20, 0));
		return resourceSystem(playerId).resourceCountWithPop(resourceEnum) < belowThreshold;
	}
	

	int32 aiStartIndex() final { return GameConstants::MaxPlayers; }
	int32 aiEndIndex() final { return GameConstants::MaxPlayersAndAI - 1; }
	bool IsAIPlayer(int32 playerId) final { return playerId >= GameConstants::MaxPlayers; }
	//bool isAIPlayer(int32_t playerId) { return aiPlayerStartIndex() <= playerId && playerId <= aiPlayerEndIndex(); }

	TCHAR* AIPrintPrefix(int32 aiPlayerId) final
	{
		std::stringstream ss;
		ss << "[" << aiPlayerId << "_" << playerName(aiPlayerId) << "]";
		return ToTChar(ss.str());
	}
	

	void ChangeRelationshipModifier(int32 aiPlayerId, int32 towardPlayerId, RelationshipModifierEnum modifierEnum, int32 amount) final
	{
		if (IsAIPlayer(aiPlayerId)) {
			aiPlayerSystem(aiPlayerId).ChangeRelationshipModifier(towardPlayerId, RelationshipModifierEnum::YouGaveUsGifts, amount / GoldToRelationship);
		}
	}
	
	//template<typename Func>
	//void ExecuteOnPlayersAndAI(Func func) {
	//	std::vector<int32> allHumanPlayerIds = _gameManager->allHumanPlayerIds();
	//	for (int32 playerId : allHumanPlayerIds) {
	//		func(playerId);
	//	}
	//	for (int32 playerId = aiStartIndex(); playerId <= aiEndIndex(); playerId++) {
	//		func(playerId);
	//	}
	//}
	int32 playerAndAICount() { return playerCount() + _mapSettings.aiCount; }


	//template<typename Func>
	//void ExecuteOnConnectedPlayers(Func func) {
	//	std::vector<int32> connectedPlayerIds = _gameManager->connectedPlayerIds();
	//	for (int32 playerId : connectedPlayerIds) {
	//		func(playerId);
	//	}
	//}
	

	int32 GetHouseLvlCount(int32 playerId, int32 houseLvl, bool includeHigherLvl) final {
		return _buildingSystem->GetHouseLvlCount(playerId, houseLvl, includeHigherLvl);
	}

	std::pair<int32, int32> GetStorageCapacity(int32 playerId, bool includeUnderConstruction = false) final
	{
		std::vector<int32> storageIds = buildingIds(playerId, CardEnum::StorageYard);
		CppUtils::AppendVec(storageIds, buildingIds(playerId, CardEnum::Warehouse));

		int32 totalSlots = 0;
		int32 usedSlots = 0;
		for (int32 storageId : storageIds) {
			Building& bld = building(storageId);
			if (bld.isConstructed()) {
				usedSlots += bld.subclass<StorageYard>().tilesOccupied();
				totalSlots += bld.storageSlotCount();
			}
			else {
				if (includeUnderConstruction) {
					totalSlots += bld.storageSlotCount();
				}
			}
		}
		return std::pair<int32, int32>(usedSlots, totalSlots);
	}

	

	int foodCount(int32 playerId) final {
		int count = 0;
		for (ResourceEnum foodEnum : StaticData::FoodEnums) {
			count += _resourceSystems[playerId].resourceCount(foodEnum);
		}
		return count;
	}
	int32 GetResourceCount(int32 playerId, const std::vector<ResourceEnum>& resourceEnums) final {
		int32 result = 0;
		for (ResourceEnum resourceEnum : resourceEnums) {
			result += resourceCount(playerId, resourceEnum);
		}
		return result;
	}
	bool HasSeed(int32 playerId, CardEnum seedCardEnum) final {
		return resourceSystem(playerId).HasSeed(seedCardEnum);
	}

	int32 influence(int32 playerId) final {
		return resourceSystem(playerId).influence();
	}
	int32 influence100(int32 playerId) final {
		return resourceSystem(playerId).influence100();
	}
	
	int32 money(int32 playerId) final {
		return resourceSystem(playerId).money();
	}
	void ChangeMoney(int32 playerId, int32 moneyChange) final {
		resourceSystem(playerId).ChangeMoney(moneyChange);
	}

	int32 price100(ResourceEnum resourceEnum) final {
		return _worldTradeSystem.price100(resourceEnum);
	}
	int32 price(ResourceEnum resourceEnum) final {
		return _worldTradeSystem.price100(resourceEnum) / 100;
	}

	TownHall& townhall(int32 playerId) final {
		return building(playerOwned(playerId).townHallId).subclass<TownHall>(CardEnum::Townhall);
	}
	int32 townLvl(int32 playerId) final {
		return townhall(playerId).townhallLvl;
	}
	
	WorldTile2 townhallGateTile(int32 playerId) final {
		return townhall(playerId).gateTile();
	}
	std::string townName(int32 playerId) final {
		return townhall(playerId).townName();
	}
	std::string townSuffix(int32 playerId) final {
		return FTextToStd(playerOwned(playerId).GetTownSizeSuffix());
	}
	std::string townSizeName(int32 playerId) final {
		return FTextToStd(playerOwned(playerId).GetTownSizeName());
	}
	int32 townAgeTicks(int32 playerId) final {
		return townhall(playerId).townAgeTicks();
	}


	void AddImmigrants(int32 playerId, int32 count, WorldTile2 tile) final {
		if (playerOwned(playerId).hasTownhall()) {
			townhall(playerId).AddImmigrants(count, tile);
		}
	}

	int32 lordPlayerId(int32 playerId) final {
		return playerOwned(playerId).lordPlayerId();
	}

	ArmyNode& GetArmyNode(int32 buildingId) final {
		return building(buildingId).subclass<ArmyNodeBuilding>().GetArmyNode();
	}
	std::vector<int32> GetArmyNodeIds(int32 playerId) final {
		std::vector<int32> nodes = playerOwned(playerId).vassalBuildingIds();
		nodes.push_back(townhall(playerId).armyNode.nodeId);
		return nodes;
	}

	WorldAtom2 homeAtom(int32 playerId) final {
		if (playerOwned(playerId).hasChosenLocation()) {
			return GetProvinceCenterTile(playerOwned(playerId).provincesClaimed().front()).worldAtom2();
		}
		return WorldAtom2::Zero;
	}

	bool IsPermanentBuilding(int32 playerId, CardEnum cardEnum) final {
		return unlockSystem(playerId)->IsPermanentBuilding(cardEnum);
	}

	TerrainTileType terraintileType(int32 tileId) final { return _terrainGenerator->terrainTileType(tileId); }
	bool IsWater(WorldTile2 tile) final {
		return IsWaterTileType(terraintileType(tile.tileId()));
	}
	bool IsMountain(WorldTile2 tile) final { return terraintileType(tile.tileId()) == TerrainTileType::Mountain; }
	bool IsWaterOrMountain(WorldTile2 tile) {
		auto tileType = terraintileType(tile.tileId());
		return IsWaterTileType(tileType) || tileType == TerrainTileType::Mountain;
	}
	GeoresourceNode georesource(int32 provinceId) final {
		return _georesourceSystem->georesourceNode(provinceId);
	}

	FloatDet Celsius(WorldTile2 tile) final {
		return Time::CelsiusHelper(MinCelsius(tile), MaxCelsius(tile));
	}
	FloatDet MinCelsius(WorldTile2 tile) final {
		int32 temperatureFraction10000 = _terrainGenerator->GetTemperatureFraction10000(tile.x, _terrainGenerator->GetRainfall100(tile));
		return ModifyCelsiusByBiome(temperatureFraction10000, Time::MinCelsiusBase());
	}
	FloatDet MaxCelsius(WorldTile2 tile) final {
		int32 temperatureFraction10000 = _terrainGenerator->GetTemperatureFraction10000(tile.x, _terrainGenerator->GetRainfall100(tile));
		return ModifyCelsiusByBiome(temperatureFraction10000, Time::MaxCelsiusBase(), maxCelsiusDivider); // Max celsius in general change less drastically than MinCelsius (maxCelsiusDivider)
	}

	FloatDet ModifyCelsiusByBiome(int32 temperatureFraction10000, FloatDet celsius, int32 divider = 1)
	{
		const FloatDet tundraModifierMax = FDOne * 10; // Tundra has no divider, summer temperature is still low
		const FloatDet borealModifierMax = FDOne * 5 / divider;
		const FloatDet jungleModifierMax = FDOne * 3 / divider; // This increases the temperature

		// Tundra
		if (temperatureFraction10000 > tundraTemperatureStart10000) {
			FloatDet modifier = tundraModifierMax * (temperatureFraction10000 - tundraTemperatureStart10000) / (10000 - tundraTemperatureStart10000);
			modifier *= 3; // reach modifierMax 3 times faster
			
			return celsius - borealModifierMax - std::min(modifier, tundraModifierMax);
		}
		// Boreal
		if (temperatureFraction10000 > borealTemperatureStart10000) {
			FloatDet modifier = borealModifierMax * (temperatureFraction10000 - borealTemperatureStart10000) / (tundraTemperatureStart10000 - borealTemperatureStart10000);
			modifier *= 3; // reach modifierMax 3 times faster
			
			return celsius - std::min(modifier, borealModifierMax);
		}
		if (temperatureFraction10000 > forestTemperatureStart10000) {
			return celsius;
		}

		// Jungle
		FloatDet modifier = jungleModifierMax * (forestTemperatureStart10000 - temperatureFraction10000) / forestTemperatureStart10000;
		modifier *= 3;
		return celsius + std::min(modifier, jungleModifierMax);
	}
	
	

	int32 tileOwner(WorldTile2 tile) final
	{
		int32 provinceId = _provinceSystem.GetProvinceIdClean(tile);
		if (provinceId == -1) {
			return -1;
		}
		return _regionSystem->provinceOwner(provinceId);
	}

	bool HasBuilding(int32 tileId) final {
		return _buildingSystem->HasBuilding(tileId);
	}
	
	bool IsBuildable(WorldTile2 tile) final {
		int32 tileId = tile.tileId();
		return terraintileType(tileId) == TerrainTileType::None &&
				_buildingSystem->HasNoBuildingOrFront(tileId) &&
				!overlaySystem().IsRoad(tile);
	}

	bool IsFrontBuildable(WorldTile2 tile) final
	{
		if (!tile.isValid()) {
			return false;
		}
		
		int32 tileId = tile.tileId();
		if (terraintileType(tileId) != TerrainTileType::None) {
			return false;
		}

		CardEnum bldEnum = buildingEnumAtTile(tile);
		return bldEnum == CardEnum::None ||
			IsRoad(bldEnum) ||
			IsRoadOverlapBuilding(bldEnum);
	}
	bool IsRoadOverlapBuildable(WorldTile2 tile) final {
		return IsFrontBuildable(tile) && !IsRoadOverlapBuilding(buildingEnumAtTile(tile));
	}
	bool IsRoadTile(WorldTile2 tile) final {
		return _pathAIHuman->isRoad(tile.x, tile.y);
	}
	
	
	bool IsBuildable(WorldTile2 tile, int32 playerId) final {
		return IsBuildable(tile) &&
			(playerId == -1 || playerId == tileOwner(tile));
	}
	bool IsFrontBuildable(WorldTile2 tile, int32 playerId) {
		return IsFrontBuildable(tile) &&
			(playerId == -1 || playerId == tileOwner(tile));
	}

	// Doesn't check for IsBuildable etc.
	bool IsTileBuildableForPlayer(WorldTile2 tile, int32 playerId)
	{
		if (playerId == -1) return true;
		return tileOwner(tile) == playerId;
	}

	/*
	 * 
	 */

	CardEnum buildingEnumAtTile(WorldTile2 tile) final {
		Building* bld = buildingAtTile(tile);
		return bld ? bld->buildingEnum() : CardEnum::None;
	}


	bool IsCritterBuilding(WorldTile2 tile) {
		int32 bldId = buildingIdAtTile(tile);
		return bldId != -1 && IsCritterBuildingEnum(building(bldId).buildingEnum());
	}
	bool IsCritterBuildingIncludeFronts(WorldTile2 tile) {
		return GetCritterBuildingsIncludeFronts(tile).size() > 0;
	}

	// Can return multiple buildings when two critter building occupy the same tile
	std::vector<int32> GetCritterBuildingsIncludeFronts(WorldTile2 tile) {
		// Has building
		int32 bldId = buildingIdAtTile(tile);
		if (bldId != -1) {
			if (IsCritterBuildingEnum(building(bldId).buildingEnum())) {
				return { bldId };
			}
			return {};
		}

		// Has fronts
		std::vector<int32> frontBuildings = _buildingSystem->frontBuildingIds(tile);
		for (int i = frontBuildings.size(); i-- > 0;) {
			// Trim non critter buildings
			if (!IsCritterBuildingEnum(building(frontBuildings[i]).buildingEnum())) {
				frontBuildings.erase(frontBuildings.begin() + i);
			}
		}
		return frontBuildings;
	}

	/**
	 * IGameSimulationCore
	 */

	bool isLoadingFromFile() final { return _isLoadingFromFile; }
	bool isLoadingForMainMenu() final { return _isLoadingForMainMenu; }

	int32 playerId() final { return _gameManager->playerId(); }

	void ResetUnitActions(int id, int32 waitTicks = 1) final;
	int AddUnit(UnitEnum unitEnum, int32_t playerId, WorldAtom2 location, int32_t ageTicks) final;
	void RemoveUnit(int id) final;
	void ResetUnitActionsInArea(TileArea area) final {
		_unitSystem->ResetUnitActionsInArea(area);
	}

	int unitCount() final { return _unitSystem->unitCount(); }
	bool unitAlive(UnitFullId fullId) final { return _unitSystem->alive(fullId); }

	Building& building(int32 id) final {
		return _buildingSystem->building(id); 
	}
	bool IsValidBuilding(int32 id) final {
		bool isValid = (0 <= id && id < _buildingSystem->buildingCount()) && _buildingSystem->alive(id);
		if (!isValid) {
			LOG_ERROR(LogNetworkInput, "IsValidBuilding: !isValid id:%d bldCount:%d alive:%d", id, _buildingSystem->buildingCount(), isValid ? _buildingSystem->alive(id) : 0);
		}
		return isValid;
	}
	Building& buildingChecked(int32 id) final {
		check(IsValidBuilding(id));
		return _buildingSystem->building(id);
	}

	
	static bool IsValidPlayer(int32 playerId) {
		return 0 <= playerId && playerId <= GameConstants::MaxPlayersAndAI;
	}
	
	CardEnum buildingEnum(int32 id) final {
		return _buildingSystem->buildingEnum(id);
	}

	bool isValidBuildingId(int32 id) final {
		return _buildingSystem->isValidBuildingId(id);
	}

	template <typename T>
	T& building(int32 id) {
		return _buildingSystem->building(id).subclass<T>();
	}

	WorldTile2 buildingCenter(int32 id) final {
		return building(id).centerTile();
	}
	
	Building& building(ResourceHolderInfo holderInfo, int32_t playerId) final {
		int32_t id = resourceSystem(playerId).holder(holderInfo).objectId;
		PUN_CHECK(_buildingSystem->alive(id));
		return building(id);
	}
	Building* buildingAtTile(WorldTile2 tile) final {
		int32 buildingId = buildingIdAtTile(tile);
		return (buildingId == -1) ? nullptr : &building(buildingId);
	}
	
	int32 buildingIdAtTile(WorldTile2 tile) final {
		return _buildingSystem->buildingIdAtTile(tile);
	}
	int32 buildingIdAtTileSafe(WorldTile2 tile) {
		if (_buildingSystem) {
			return _buildingSystem->buildingIdAtTile(tile);
		}
		return -1;
	}
	
	bool tileHasBuilding(WorldTile2 tile) final {
		return buildingIdAtTile(tile) >= 0;
	}

	std::vector<int32> frontBuildingIds(WorldTile2 tile) final {
		return _buildingSystem->frontBuildingIds(tile);
	}

	const std::vector<int32>& buildingIds(int32 playerId, CardEnum buildingEnum) final {
		return _buildingSystem->buildingIds(playerId, buildingEnum);
	}
	int32 buildingCount(int32 playerId, CardEnum buildingEnum) final {
		return _buildingSystem->buildingIds(playerId, buildingEnum).size();
	}
	int32 buildingFinishedCount(int32 playerId, CardEnum cardEnum) final {
		const std::vector<int32>& bldIds = buildingIds(playerId, cardEnum);
		int32 finishedCount = 0;
		for (int32 buildingId : bldIds) {
			if (building(buildingId).isConstructed()) {
				finishedCount++;
			}
		}
		return finishedCount;
	}


	int32 buildingFinishedCount(int32 playerId, int32 provinceId)
	{
		int32 count = 0;
		for (int32 i = 0; i < BuildingEnumCount; i++) {
			count += buildingFinishedCount(playerId, provinceId, static_cast<CardEnum>(i));
		}
		return count;
	}
	int32 buildingFinishedCount(int32 playerId, int32 provinceId, CardEnum buildingEnum)
	{
		const std::vector<int32>& bldIds = buildingIds(playerId, buildingEnum);
		int32 finishedCount = 0;
		for (int32 buildingId : bldIds) {
			Building& bld = building(buildingId);
			if (bld.provinceId() == provinceId && bld.isConstructed()) {
				finishedCount++;
			}
		}
		return finishedCount;
	}

	

	int32 jobBuildingCount(int32 playerId) final {
		return playerOwned(playerId).jobBuildingCount();
	}
	
	const SubregionLists<int32>& buildingSubregionList() final {
		return _buildingSystem->buildingSubregionList();
	}

	// TODO: Optimize to use provinces?
	bool HasBuildingWithinRadius(WorldTile2 tileIn, int32 radius, int32 playerId, CardEnum buildingEnum) final
	{
		const std::vector<int32>& bldIds = buildingIds(playerId, buildingEnum);
		for (int32 bldId : bldIds) {
			if (building(bldId).DistanceTo(tileIn) <= radius) {
				return true;
			}
		}
		return false;
	}
	std::vector<int32> GetBuildingsWithinRadius(WorldTile2 tileIn, int32 radius, int32 playerId, CardEnum buildingEnum) final
	{
		const std::vector<int32>& bldIds = buildingIds(playerId, buildingEnum);
		std::vector<int32> resultIds;
		for (int32 bldId : bldIds) {
			if (building(bldId).DistanceTo(tileIn) <= radius) {
				resultIds.push_back(bldId);
			}
		}
		return resultIds;
	}
	std::vector<int32> GetBuildingsWithinRadiusMultiple(WorldTile2 tileIn, int32 radius, int32 playerId, std::vector<CardEnum> buildingEnums) final
	{
		std::vector<int32> resultIds;
		for (CardEnum buildingEnum : buildingEnums)
		{
			const std::vector<int32>& bldIds = buildingIds(playerId, buildingEnum);
			for (int32 bldId : bldIds) {
				if (building(bldId).DistanceTo(tileIn) <= radius) {
					resultIds.push_back(bldId);
				}
			}
		}
		return resultIds;
	}
	
	

	void RemoveBuilding(int32 buildingId) final {
		_buildingSystem->RemoveBuilding(buildingId);
	}

	bool buildingIsAlive(int32 id) final { return _buildingSystem->alive(id); }

	WorldTile2 gateTile(int32 id) final { return _buildingSystem->building(id).gateTile(); }

	void AddTickBuilding(int buildingId) final {
		_buildingSystem->AddTickBuilding(buildingId);
	}
	void ScheduleTickBuilding(int32 buildingId, int32 scheduleTick) final {
		_buildingSystem->ScheduleTickBuilding(buildingId, scheduleTick);
	}
	void RemoveScheduleTickBuilding(int32 buildingId) final {
		_buildingSystem->RemoveScheduleTickBuilding(buildingId);
	}

	std::vector<int32> GetConstructionResourceCost(CardEnum cardEnum, TileArea area) final
	{
		if (cardEnum == CardEnum::Farm) {
			return { area.tileCount() / 2, 0, 0 };
		}
		if (cardEnum == CardEnum::StorageYard) {
			return { area.tileCount() / 4 * 5, 0, 0 };
		}
		return GetBuildingInfo(cardEnum).constructionResources;
	}

	/*
	 * Only valid for area smaller than a region
	 */
	bool IsLandCleared_SmallOnly(int32 playerId, TileArea area) final
	{
		// TileObj
		auto& tileObjSys = treeSystem();
		bool hasNonEmptyTile = area.ExecuteOnAreaWithExit_WorldTile2([&](WorldTile2 tile) {
			return !tileObjSys.IsEmptyTile_WaterAsEmpty(tile.tileId());
		});
		if (hasNonEmptyTile) {
			return false;
		}

		// Drops
		if (resourceSystem(playerId).GetDropFromSmallArea_Any(area).isValid()) {
			return false;
		}

		return true;
	}

	void SetNeedDisplayUpdate(DisplayClusterEnum displayEnum, int32 regionId, bool needUpdate = true) final {
		if (isInitialized()) {
			_displayEnumToRegionToNeedUpdate[static_cast<int>(displayEnum)][regionId] = needUpdate;
		}
	}
	void SetNeedDisplayUpdate(DisplayClusterEnum displayEnum, const std::vector<int32>& regionIds, bool needUpdate = true) {
		if (isInitialized()) {
			for (int32 regionId : regionIds) {
				_displayEnumToRegionToNeedUpdate[static_cast<int>(displayEnum)][regionId] = needUpdate;
			}
		}
	}
	void SetNeedDisplayUpdate(DisplayClusterEnum displayEnum, TileArea area, bool isSmallArea, bool needUpdate = true) final {
		std::vector<int32> regionIds = WorldRegion2::WorldRegionsToRegionIds(area.GetOverlapRegions(isSmallArea));
		SetNeedDisplayUpdate(displayEnum, regionIds, needUpdate);
	}
	
	bool NeedDisplayUpdate(DisplayClusterEnum displayEnum, int32 regionId) final {
		return isInitialized() && _displayEnumToRegionToNeedUpdate[static_cast<int>(displayEnum)][regionId];
	}

	void SetNeedDisplayUpdate(DisplayGlobalEnum displayEnum, bool needUpdate = true) final {
		if (isInitialized()) {
			_displayEnumToNeedUpdate[static_cast<int>(displayEnum)] = needUpdate;
		}
	}
	bool NeedDisplayUpdate(DisplayGlobalEnum displayEnum) final {
		return isInitialized() && _displayEnumToNeedUpdate[static_cast<int>(displayEnum)];
	}

	void AddNeedDisplayUpdateId(DisplayGlobalEnum displayEnum, int32 id, bool updateBeforeInitialized = false) final {
		if (isInitialized() || updateBeforeInitialized) {
			_displayEnumToNeedUpdateIds[static_cast<int>(displayEnum)].push_back(id);
		}
	}
	std::vector<int32> GetNeedDisplayUpdateIds(DisplayGlobalEnum displayEnum) final {
		if (isInitialized()) {
			std::vector<int32> updateIds = _displayEnumToNeedUpdateIds[static_cast<int>(displayEnum)];
			_displayEnumToNeedUpdateIds[static_cast<int>(displayEnum)].clear();
			return updateIds;
		}
		return {};
	}

	IQuestSystem* iquestSystem(int32 playerId) final { return playerId < _questSystems.size() ? &_questSystems[playerId] : nullptr; }

	UnitStateAI& unitAI(int32 id) final { return _unitSystem->unitStateAI(id); }
	WorldAtom2 unitAtom(int32 id) final { return _unitSystem->atomLocation(id); }

	int32 animalInitialCount(UnitEnum unitEnum) final { return _unitSystem->animalInitialCount(unitEnum); }
	int32 unitEnumCount(UnitEnum unitEnum) final { return _unitSystem->unitCount(unitEnum); }
	
	std::string unitdebugStr(int id) final;
	void unitAddDebugSpeech(int32 id, std::string message) final;

	void SetWalkableSkipFlood(WorldTile2 tile, bool isWalkable) final {
		_pathAI->SetWalkable(tile.x, tile.y, isWalkable);
		_pathAIHuman->SetWalkable(tile.x, tile.y, isWalkable);
	}
	void SetWalkable(WorldTile2 tile, bool isWalkable) final {
		_pathAI->SetWalkable(tile.x, tile.y, isWalkable);
		_pathAIHuman->SetWalkable(tile.x, tile.y, isWalkable);

		_floodSystem.ResetRegionFloodDelayed(TileToRegion64Id(tile));
		_floodSystemHuman.ResetRegionFloodDelayed(TileToRegion64Id(tile));
	}
	void SetWalkableNonIntelligent(WorldTile2 tile, bool isWalkable) final {
		_pathAI->SetWalkable(tile.x, tile.y, isWalkable);
		_floodSystem.ResetRegionFloodDelayed(TileToRegion64Id(tile));
	}
	PunAStar128x256* pathAI(bool canPassGate) final {
		return canPassGate ? _pathAIHuman.get() : _pathAI.get();
	}

	void SetRoadPathAI(WorldTile2 tile, bool isRoad) final {
		//_pathAI->SetRoad(tile.x, tile.y, isRoad);
		//_pathAIHuman->SetRoad(tile.x, tile.y, isRoad); // TODO: setting road is static
		PunAStar128x256::SetRoad(tile.x, tile.y, isRoad);
	}

	int16 GetFloodId(WorldTile2 tile) final { return _floodSystem.GetFloodId(tile); }
	bool IsConnected(WorldTile2 start, WorldTile2 end, int maxRegionDistance, bool canPassGate) final {
		if (canPassGate) {
			return _floodSystemHuman.IsConnected(start, end, maxRegionDistance);
		}
		return _floodSystem.IsConnected(start, end, maxRegionDistance);
	}

	// Cached Connected for human
	bool IsConnectedBuilding(int32 buildingId, int32 playerId) final
	{
		PUN_CHECK(playerId != -1);
		PUN_CHECK(buildingId != -1);

		int8 isConnected = _buildingSystem->IsConnectedBuilding(buildingId);
		PUN_CHECK(isConnected != -1);
		if (isConnected == -1) {
			return false;
		}
		return static_cast<bool>(isConnected);
	}

	void OnRefreshFloodGrid(WorldRegion2 region) override {
		_buildingSystem->OnRefreshFloodGrid(region);
	}
	

	/*
	 * Province
	 */
	int32 GetProvinceIdRaw(WorldTile2 tile) final {
		return _provinceSystem.GetProvinceIdRaw(tile);
	}
	int32 GetProvinceIdClean(WorldTile2 tile) final {
		return _provinceSystem.GetProvinceIdClean(tile);
	}

	const std::vector<int32>& GetProvinceIdsFromRegionId(int32 regionId) final {
		return _provinceSystem.GetProvinceIdsFromRegionId(regionId);
	}
	std::vector<int32> GetProvinceIdsFromArea(TileArea area, bool smallArea) final {
		return _provinceSystem.GetProvinceIdsFromArea(area, smallArea);
	}
	
	bool AreAdjacentProvinces(int32 provinceId1, int32 provinceId2) final {
		return _provinceSystem.AreAdjacentProvinces(provinceId1, provinceId2);
	}
	bool IsProvinceValid(int32 provinceId) final {
		return _provinceSystem.IsProvinceValid(provinceId);
	}

	WorldTile2 GetProvinceCenterTile(int32 provinceId) final {
		return _provinceSystem.GetProvinceCenterTile(provinceId);
	}
	WorldTile2 GetProvinceRandomTile(int32 provinceId, WorldTile2 floodOrigin, int32 maxRegionDist = 1, bool isIntelligent = false, int32 tries = 100) final {
		return _provinceSystem.GetProvinceRandomTile(provinceId, floodOrigin, maxRegionDist, isIntelligent, tries);
	}

	TileArea GetProvinceRectArea(int32 provinceId) final {
		return _provinceSystem.GetProvinceRectArea(provinceId);
	}

	int32 GetTreeCount(int32 provinceId) final
	{
		int32 treeCount = _regionSystem->provinceToTreeCountCache[provinceId];
		if (treeCount == -1) {
			// !!! Very Expensive...
			treeCount = 0;
			_provinceSystem.ExecuteOnProvinceTiles(provinceId, [&](WorldTile2 tile) {
				if (_treeSystem->tileInfo(tile.tileId()).type == ResourceTileType::Tree) {
					treeCount++;
				}
			});
			_regionSystem->provinceToTreeCountCache[provinceId] = treeCount;
		}
		
		return treeCount;
	}


	/*
	 * Province Income/Upkeep/Costs
	 */

	int32 GetProvinceIncome100(int32 provinceId) final {
		// Sample fertility, and take that into province income's calculation
		TileArea area = _provinceSystem.GetProvinceRectArea(provinceId);

		int32 fertilityPercentTotal = 100;
		int32 tilesExamined = 1; // 1 to prevent /0
		for (int32 y = area.minY; y <= area.maxY; y += 4) {
			for (int32 x = area.minX; x <= area.maxX; x += 4) {
				WorldTile2 tile(x, y);
				if (_provinceSystem.GetProvinceIdClean(tile) == provinceId &&
					_terrainGenerator->terrainTileType(tile) == TerrainTileType::None) 
				{
					fertilityPercentTotal += _terrainGenerator->GetFertilityPercent(WorldTile2(x, y));
					tilesExamined++;
				}
			}
		}

		int32 provinceIncome100 = fertilityPercentTotal * Income100PerTiles * _provinceSystem.provinceFlatTileCount(provinceId) / (tilesExamined * 100);
		return std::max(100, provinceIncome100);
	}
	int32 GetProvinceBaseUpkeep100(int32 provinceId)
	{
		return GetProvinceIncome100(provinceId) / 2; // Upkeep half the income as base
	}
	int32 GetProvinceUpkeep100(int32 provinceId, int32 playerId) final
	{
		int32 income100 = GetProvinceBaseUpkeep100(provinceId); // Upkeep half the income as base
		
		if (IsOverseaProvince(provinceId, playerId)) {
			income100 = income100 * 3; // +200%
		}
		
		return income100;
	}


	int32 GetProvinceBaseClaimPrice(int32 provinceId)
	{
		int32 baseClaimPrice100 = GetProvinceIncome100(provinceId) * ClaimToIncomeRatio;
		return (baseClaimPrice100 / 100);
	}

	int32 GetProvinceClaimPrice(int32 provinceId, int32 playerId) {
		return GetProvinceClaimPrice(provinceId, playerId, GetProvinceClaimConnectionEnum(provinceId, playerId));
	}
	int32 GetProvinceClaimPrice(int32 provinceId, int32 playerId, ClaimConnectionEnum claimConnectionEnum)
	{
		int32 percent = 100;
		percent += GetProvinceSpecialClaimCostPercent(provinceId, playerId);
		percent += GetProvinceTerrainClaimCostPenalty(claimConnectionEnum);
		
		return GetProvinceBaseClaimPrice(provinceId) * percent / 100;
	}
	
	int32 GetProvinceAttackStartPrice(int32 provinceId, ClaimConnectionEnum claimConnectionEnum)
	{
		// Province base claim price: pay for acquiring the land
		return GetProvinceBaseClaimPrice(provinceId) * 2 + GetProvinceAttackReinforcePrice(provinceId, claimConnectionEnum);
	}
	int32 GetProvinceAttackReinforcePrice(int32 provinceId, ClaimConnectionEnum claimConnectionEnum)
	{
		return BattleInfluencePrice * GetTotalAttackPercent(provinceId, claimConnectionEnum) / 100;
	}
	int32 GetTotalAttackPercent(int32 provinceId, ClaimConnectionEnum claimConnectionEnum)
	{
		int32 percent = 100;
		percent += GetProvinceTerrainClaimCostPenalty(claimConnectionEnum);
		percent += GetProvinceAttackCostPercent(provinceId);
		return percent;
	}

	// Vassalize Cost increase with:
	// - population (initial)
	// - oversea: double reinforce price
	int32 GetProvinceVassalizeStartPrice(int32 provinceId)
	{
		int32 provincePlayerId = provinceOwner(provinceId);
		PUN_CHECK(homeProvinceId(provincePlayerId) == provinceId);
		
		return BattleInfluencePrice + population(provincePlayerId) * 10; // 100 pop = 1k influence to take over
	}
	int32 GetProvinceVassalizeReinforcePrice(int32 provinceId)
	{
		return BattleInfluencePrice * GetVassalizeTotalAttackPercent(provinceId) / 100;
	}
	// TODO: Campaign Plan
	// You can conquer through Annexation or Vassalization.
	// Annexing a province will annihilate all buildings in the province. Annexed Province become your land.
	// Annexing is costly because your military must take care of erecting a new local governing body.
	// Alternatively, you can also conquer by Vassalizing which is a less costly when taking over a large patch of land.
	// When you force Vassalize a state, your military head straight for the city center to force its ruler to submit to you.
	// The ruler will retain management control of his/her land, but must pay you vassal tax.
	//
	// DefenseBonus = 20% per province + max provincial defense.
	//
	// To vassalize, you must plan out the attack path to the target city center.
	// This will determine the opponent's Defense Bonus in the upcoming Battle.
	// Your path must start from a province under your control.
	//  - [Set Path] above valid provinces click
	//  - Step 1, 2, 3... with [Cancel Path] button
	//  - [Finish Path] once, done, prompt confirm, and start battle
	int32 GetVassalizeTotalAttackPercent(int32 provinceId)
	{
		int32 percent = 100;
		percent += GetProvinceVassalizeDefenseBonus(provinceId);
		return percent;
	}
	int32 GetProvinceVassalizeDefenseBonus(int32 provinceId)
	{
		return 100;
	}
	std::string GetProvinceVassalizeDefenseBonusTip(int32 provinceId)
	{
		return "Defense Bonus against Vassalize Military Campaign: %100";
	}

	//  Vassalize/Liberation can only be done if
	//  - Vassalize is researched
	//  - you are not a vassal yourself
	//  - no battle at home
	bool CanVassalizeOtherPlayers(int32 playerIdIn)
	{
		if (IsAIPlayer(playerIdIn)) { //TODO: this is for testing for now..
			return true;
		}
		return IsResearched(playerIdIn, TechEnum::Vassalize) && 
				CanConquerOtherPlayers_Base(playerIdIn);
	}
	bool CanConquerProvinceOtherPlayers(int32 playerIdIn)
	{
		if (IsAIPlayer(playerIdIn)) { //TODO: this is for testing for now..
			return true;
		}
		return IsResearched(playerIdIn, TechEnum::Conquer) &&
				CanConquerOtherPlayers_Base(playerIdIn);
	}
	bool CanConquerOtherPlayers_Base(int32 playerIdIn) {
		return lordPlayerId(playerIdIn) == -1 &&
				!playerOwned(playerIdIn).GetDefendingClaimProgress(homeProvinceId(playerIdIn)).isValid();
	}

	void LoseVassalHelper(int32 oldLordPlayerId, int32 formerVassalPlayerId)
	{
		if (!HasTownhall(formerVassalPlayerId)) {
			return;
		}
		int32 formerVassalBuildingId = townhall(formerVassalPlayerId).buildingId();
		
		_playerOwnedManagers[oldLordPlayerId].LoseVassal(formerVassalBuildingId);
		_playerOwnedManagers[formerVassalPlayerId].SetLordPlayerId(-1);

		_playerOwnedManagers[oldLordPlayerId].RecalculateTaxDelayed();
		_playerOwnedManagers[formerVassalPlayerId].RecalculateTaxDelayed();
	}

	

	// Claim/Attack Cost Percent
	//  Base Bonuses
	int32 GetProvinceTerrainClaimCostPenalty(ClaimConnectionEnum claimConnectionEnum)
	{
		int32 penalty = 0;

		// Attacking over shallow water has attack cost penalty of 100%
		if (claimConnectionEnum == ClaimConnectionEnum::ShallowWater) {
			penalty += 100;
		}
		// Attacking deepsea has attack cost penalty of 200%
		else if (claimConnectionEnum == ClaimConnectionEnum::Deepwater) {
			penalty += 200;
		}
		
		return penalty;
	}
	//  Claim Bonuses
	int32 GetProvinceSpecialClaimCostPercent(int32 provinceId, int32 playerId)
	{
		int32 percent = 0;

		BiomeEnum biomeEnum = GetBiomeProvince(provinceId);
		if ((biomeEnum == BiomeEnum::Tundra || biomeEnum == BiomeEnum::BorealForest) &&
			IsResearched(playerId, TechEnum::BorealLandCost))
		{
			percent -= 50;
		}

		return percent;
	}
	//  Attack Bonuses
	int32 GetProvinceAttackCostPercent(int32 provinceId)
	{
		int32 percent = 0;
		
		int32 provinceOwnerId = provinceOwner(provinceId);
		if (provinceOwnerId != -1) 
		{
			// Fort
			percent += GetFortDefenseBonus_Helper(provinceId, provinceOwnerId);

			// Buildings
			percent += GetBuildingDefenseBonus_Helper(provinceId, provinceOwnerId);
		}
		return percent;
	}
	std::string GetProvinceDefenseBonusTip(int32 provinceId)
	{
		std::stringstream ss;
		ss << "Defense Bonus: " << GetProvinceAttackCostPercent(provinceId);
		int32 provinceOwnerId = provinceOwner(provinceId);
		if (provinceOwnerId != -1) {
			ss << "<bullet>fort " << GetFortDefenseBonus_Helper(provinceId, provinceOwnerId) << "%</>";
			ss << "<bullet>buildings " << GetBuildingDefenseBonus_Helper(provinceId, provinceOwnerId) << "%</>";
		}
		return ss.str();
	}
	
	int32 GetFortDefenseBonus_Helper(int32 provinceId, int32 provinceOwnerId)
	{
		const std::vector<int32>& fortIds = buildingIds(provinceOwnerId, CardEnum::Fort);
		for (int32 fortId : fortIds) {
			if (building(fortId).provinceId() == provinceId) {
				return 100;
			}
		}
		return 0;
	}
	int32 GetBuildingDefenseBonus_Helper(int32 provinceId, int32 provinceOwnerId)
	{
		return 10 * buildingFinishedCount(provinceOwnerId, provinceId);
	}
	

	//bool HasOutpostAt(int32 playerId, int32 provinceId) final {
	//	return playerOwned(playerId).HasOutpostAt(provinceId);
	//}
	bool IsProvinceNextToPlayer(int32 provinceId, int32 playerId) final
	{
		return _provinceSystem.ExecuteAdjacentProvincesWithExitTrue(provinceId, [&](ProvinceConnection connection) 
		{
			bool isValidConnectionType = connection.tileType == TerrainTileType::None || 
											connection.tileType == TerrainTileType::River;

			return isValidConnectionType && 
					provinceOwner(connection.provinceId) == playerId;
		});
	}
	bool IsProvinceNextToPlayerByShallowWater(int32 provinceId, int32 playerId) final {
		if (!unlockSystem(playerId)->IsResearched(TechEnum::ShallowWaterEmbark)) {
			return false;
		}
		return _provinceSystem.ExecuteAdjacentProvincesWithExitTrue(provinceId, [&](ProvinceConnection connection)
		{
			bool isValidConnectionType = (connection.tileType == TerrainTileType::Ocean);

			return isValidConnectionType &&
				provinceOwner(connection.provinceId) == playerId;
		});
	}
	bool IsProvinceOverseaClaimableByPlayer(int32 provinceId, int32 playerId)
	{
		// TODO: have military harbor that do flood check..
		return unlockSystem(playerId)->IsResearched(TechEnum::DeepWaterEmbark) &&
			_provinceSystem.provinceIsCoastal(provinceId);
	}
	ClaimConnectionEnum GetProvinceClaimConnectionEnum(int32 provinceId, int32 playerId)
	{
		if (IsProvinceNextToPlayer(provinceId, playerId))
		{
			return ClaimConnectionEnum::Flat;
		}
		if (IsProvinceNextToPlayerByShallowWater(provinceId, playerId))
		{
			return ClaimConnectionEnum::ShallowWater;
		}
		if (IsProvinceOverseaClaimableByPlayer(provinceId, playerId))
		{
			return ClaimConnectionEnum::Deepwater;
		}
		return ClaimConnectionEnum::None;
	}
	bool IsOverseaProvince(int32 provinceIdIn, int32 playerId)
	{
		if (!HasTownhall(playerId)) {
			return false;
		}
		
		int32 townhallProvinceId = townhall(playerId).provinceId();
		bool isConnectedToTownhall = FloodProvinces(provinceIdIn, [&](int32 provinceId)
		{
			return townhallProvinceId == provinceId;
		});

		return !isConnectedToTownhall;
	}
	
	template <typename Func>
	bool FloodProvinces(int32 provinceId, Func shouldExitFunc)
	{
		TSet<int32> visited;
		std::vector<int32> floodQueue;
		floodQueue.push_back(provinceId);
		visited.Add(provinceId);
		
		while (floodQueue.size() > 0)
		{
			int32 curProvinceId = floodQueue.back();
			floodQueue.pop_back();
			
			if (shouldExitFunc(curProvinceId)) {
				return true;
			}

			const auto& connections = GetProvinceConnections(curProvinceId);
			for (const ProvinceConnection& connection : connections) {
				if (!visited.Contains(connection.provinceId)) {
					floodQueue.push_back(connection.provinceId);
					visited.Add(connection.provinceId);
				}
			}
		}
		return false;
	}
	
	
	bool IsProvinceNextToPlayerIncludingNonFlatLand(int32 provinceId, int32 playerId) {
		return _provinceSystem.ExecuteAdjacentProvincesWithExitTrue(provinceId, [&](ProvinceConnection connection) {
			return provinceOwner(connection.provinceId) == playerId;
		});
	}
	

	void RefreshTerritoryEdge(int32 playerId) final {
		return _provinceSystem.RefreshTerritoryEdge(playerId, playerOwned(playerId).provincesClaimed());
	}

	bool IsBorderProvince(int32 provinceId) final
	{
		int32 playerId = provinceOwner(provinceId);
		check(playerId != -1);
		const std::vector<ProvinceConnection>& connections = _provinceSystem.GetProvinceConnections(provinceId);
		for (const ProvinceConnection& connection : connections)
		{
			if (connection.tileType == TerrainTileType::None) {
				if (provinceOwner(connection.provinceId) != playerId) {
					return true;
				}
			}
		}
		return false;
	}

	const std::vector<ProvinceConnection>& GetProvinceConnections(int32 provinceId) final {
		return _provinceSystem.GetProvinceConnections(provinceId);
	}

	void AddTunnelProvinceConnections(int32 provinceId1, int32 provinceId2) final
	{
		_provinceSystem.AddTunnelProvinceConnection(provinceId1, provinceId2, TerrainTileType::None);
	}
	
	/*
	 * 
	 */

	NonWalkableTileAccessInfo TryAccessNonWalkableTile(WorldTile2 start, WorldTile2 nonwalkableTile, int maxRegionDistance, bool canPassGate) final {
		if (canPassGate) {
			return _floodSystemHuman.TryAccessNonWalkableTile(start, nonwalkableTile, maxRegionDistance);
		}
		return _floodSystem.TryAccessNonWalkableTile(start, nonwalkableTile, maxRegionDistance);
	}


	//WorldTile2 FindNearbyAvailableTile(WorldTile2 start) final {
	//	return AlgorithmUtils::FindNearbyAvailableTile(start, [&](WorldTile2 tile) {
	//		return GameMap::PathAI->isWalkable(tile.x, tile.y);;
	//	});
	//}

	WorldTile2 FindNearbyDroppableTile(WorldTile2 start, TileArea excludeArea) final {
		return AlgorithmUtils::FindNearbyAvailableTile(start, [&](WorldTile2 tile) {
			return !excludeArea.HasTile(tile) && 
					(IsBuildable(tile) || overlaySystem().IsRoad(tile)) &&
					_treeSystem->IsEmptyLandTile(tile.tileId()) &&
					_dropSystem.GetDrops(tile).size() == 0; // walkable tile outside of the area without existing drop...
		});
	}

	void PlayerAddHouse(int32 playerId, int objectId) override { _playerOwnedManagers[playerId].PlayerAddHouse(objectId); }
	void PlayerRemoveHouse(int32 playerId, int objectId) override { _playerOwnedManagers[playerId].PlayerRemoveHouse(objectId); }
	void PlayerAddJobBuilding(int32 playerId, Building& building, bool isConstructed) override { _playerOwnedManagers[playerId].PlayerAddJobBuilding(building, isConstructed); }
	void PlayerRemoveJobBuilding(int32 playerId, Building& building, bool isConstructed) override { _playerOwnedManagers[playerId].PlayerRemoveJobBuilding(building, isConstructed); }
	void RefreshJobDelayed(int32 playerId) override {
		_playerOwnedManagers[playerId].RefreshJobDelayed();
	}
	
	bool IsInDarkAge(int32 playerId) final { return _playerOwnedManagers[playerId].IsInDarkAge(); }
	
	void RecalculateTaxDelayed(int32 playerId) override {
		_playerOwnedManagers[playerId].RecalculateTaxDelayed();
	}

	const std::vector<int32>& boarBurrows(int32 provinceId) override { return _regionSystem->boarBurrows(provinceId); }
	void AddBoarBurrow(int32 provinceId, int32 buildingId) override {
		_regionSystem->AddBoarBurrow(provinceId, buildingId);
	}
	void RemoveBoarBurrow(int32 provinceId, int32 buildingId) override {
		_regionSystem->RemoveBoarBurrow(provinceId, buildingId);
	}

	const std::vector<int32>& provinceAnimals(int32 regionId) override { return _regionSystem->provinceAnimals(regionId); }
	void AddProvinceAnimals(int32 provinceId, int32 animalId) override {
		_regionSystem->AddProvinceAnimals(provinceId, animalId);
	}
	void RemoveProvinceAnimals(int32 provinceId, int32 animalId) override {
		_regionSystem->RemoveProvinceAnimals(provinceId, animalId);
	}

	int HousingCapacity(int32 playerId) override { return _playerOwnedManagers[playerId].housingCapacity(); }

	void RemoveJobsFrom(int32 buildingId, bool isRefreshJob) override
	{
		Building& bld = building(buildingId);
		std::vector<int>& occupantIds = bld.occupants();
		for (int occupantId : occupantIds) {
			PUN_DEBUG_EXPR(unitAI(occupantId).AddDebugSpeech(" RemoveJobsFrom buildingId:" + std::to_string(buildingId) + " isRefresh:" + std::to_string(isRefreshJob)));
			
			unitAI(occupantId).SetWorkplaceId(-1);
		}
		bld.ClearOccupant();
	}
	
	void RemoveTenantFrom(int32 buildingId) override
	{
		Building& bld = building(buildingId);
		check(IsHouse(bld.buildingEnum()));

		std::vector<int>& occupantIds = bld.occupants();
		for (int occupantId : occupantIds) {
			unitAI(occupantId).SetHouseId(-1);
		}
		bld.ClearOccupant();
	}

	void ClearProvinceBuildings(int32 provinceId)
	{
		PUN_CHECK(_provinceSystem.IsProvinceValid(provinceId));
		
		_provinceSystem.ExecuteOnProvinceTiles(provinceId, [&](WorldTile2 tile) {
			// Destroy any building already here
			int32 bldId = buildingIdAtTile(tile);
			if (bldId != -1) {
				Building& bld = building(bldId);

				soundInterface()->TryStopBuildingWorkSound(bld);
				_buildingSystem->RemoveBuilding(bldId);

				AddDemolishDisplayInfo(tile, { bld.buildingEnum(), bld.area(), Time::Ticks() });
				//_regionToDemolishDisplayInfos[tile.regionId()].push_back({ bld.buildingEnum(), bld.area(), Time::Ticks() });
			}
		});
	}

	void SetProvinceOwner(int32 provinceId, int32 playerId, bool lightMode = false)
	{
		// When transfering land if it is oversea, warn of 200% influence upkeep
		if (playerId != -1 &&
			GetProvinceClaimConnectionEnum(provinceId, playerId) == ClaimConnectionEnum::Deepwater) 
		{
			AddPopup(playerId, "You have claimed a Province oversea.<space>Oversea provinces have upkeep penalty of +200%");
		}
		
		int32 oldPlayerId =  provinceOwner(provinceId);
		if (oldPlayerId != -1) {
			playerOwned(oldPlayerId).TryRemoveProvinceClaim(provinceId, lightMode); // TODO: Try moving this below???
		}
		
		_regionSystem->SetProvinceOwner(provinceId, playerId, lightMode);
		
		if (playerId != -1) {
			playerOwned(playerId).ClaimProvince(provinceId, lightMode);

			if (!lightMode) {
				RefreshTerritoryEdge(playerId);
			}
		}
		if (oldPlayerId != -1) {
			if (!lightMode) {
				RefreshTerritoryEdge(oldPlayerId);
			}
		}
	}
	void SetProvinceOwnerFull(int32 provinceId, int32 playerId) final;

	int32 provinceOwner(int32 provinceId) final { return _regionSystem->provinceOwner(provinceId); }


	/*
	 * Terran Gen
	 */
	int32 GetFertilityPercent(WorldTile2 tile) override {
		return terrainGenerator().GetFertilityPercent(tile);
	}
	BiomeEnum GetBiomeEnum(WorldTile2 tile) override {
		return terrainGenerator().GetBiome(tile);
	}
	BiomeEnum GetBiomeProvince(int32 provinceId) override {
		return terrainGenerator().GetBiome(_provinceSystem.GetProvinceCenterTile(provinceId));
	}
	

	void DespawnResourceHolder(ResourceHolderInfo info, int32 playerId) override {
		resourceSystem(playerId).DespawnHolder(info);
	}

	//! Popup displays
	void AddPopup(int32 playerId, std::string popupBody, std::string popupSound = "") final {
		PopupInfo info(playerId, popupBody);
		info.popupSound = popupSound;
		AddPopup(info);
	}
	void AddPopup(PopupInfo popupInfo) final {
		PUN_CHECK(0 <= popupInfo.playerId);
		PUN_CHECK(popupInfo.playerId < _popupSystems.size());
		_popupSystems[popupInfo.playerId].AddPopup(popupInfo);
	}

	void AddPopupToFront(int32 playerId, std::string popupBody) final {
		AddPopupToFront(PopupInfo(playerId, popupBody));
	}
	void AddPopupToFront(PopupInfo popupInfo) final {
		_popupSystems[popupInfo.playerId].AddPopupToFront(popupInfo);
	}
	void AddPopupToFront(int32 playerId, std::string popupBody, ExclusiveUIEnum exclusiveEnum, std::string popupSound) final {
		PopupInfo popup(playerId, { popupBody }, popupSound);
		popup.warningForExclusiveUI = exclusiveEnum;
		AddPopupToFront(popup);
	}

	void AddPopupAll(PopupInfo popupInfo, int32 playerToSkip) final {
		PUN_LOG("AddPopupToDisplayAll %s", ToTChar(popupInfo.body));
		ExecuteOnConnectedPlayers([&](int32 playerId) {
			if (playerId != playerToSkip) {
				popupInfo.playerId = playerId;
				_popupSystems[playerId].AddPopup(popupInfo);
			}
		});
	}

	void AddPopupNonDuplicate(PopupInfo popupInfo) final
	{
		PUN_CHECK(0 <= popupInfo.playerId);
		PUN_CHECK(popupInfo.playerId < _popupSystems.size());
		auto& popupSys = _popupSystems[popupInfo.playerId];
		if (!popupSys.HasPopup(popupInfo)) {
			popupSys.AddPopup(popupInfo);
		}
	}


	
	PopupInfo* PopupToDisplay(int32 playerId) final {
		return _popupSystems[playerId].PopupToDisplay();
	}


	void TryRemovePopups(int32 playerId, PopupReceiverEnum receiverEnum) final {
		_popupSystems[playerId].TryRemovePopups(receiverEnum);
	}
	
	void CloseCurrentPopup(int32 playerId) final {
		_popupSystems[playerId].ClosePopupToDisplay();
	}
	void WaitForReply(int32 playerId) {
		_popupSystems[playerId].waitingForReply = true;
	}
	void ClearPopups(int32 playerId) {
		_popupSystems[playerId].ClearPopups();
	}
	

	//
	void AddEventLog(int32 playerId, std::string eventMessage, bool isImportant) final {
		_eventLogSystem.AddEventLog(playerId, ToFString(eventMessage), isImportant);
	}
	void AddEventLogF(int32 playerId, FString eventMessage, bool isImportant) final {
		_eventLogSystem.AddEventLog(playerId, eventMessage, isImportant);
	}
	void AddEventLogToAllExcept(int32 playerIdException, std::string eventMessage, bool isImportant) final {
		ExecuteOnConnectedPlayers([&](int32 playerId) {
			if (playerId != playerIdException) {
				AddEventLog(playerId, eventMessage, isImportant);
			}
		});
	}

	bool HasQuest(int32 playerId, QuestEnum questEnum) final {
		return questSystem(playerId)->GetQuest(questEnum) != nullptr;
	}
	bool WasQuestStarted(int32 playerId, QuestEnum questEnum) final {
		return questSystem(playerId)->WasQuestStarted(questEnum);
	}
	void AddQuest(int32 playerId, std::shared_ptr<Quest> quest) final
	{
		if (!WasQuestStarted(playerId, quest->classEnum())) {
			questSystem(playerId)->AddQuest(quest);
		}
	}
	void TryAddQuest(int32 playerId, std::shared_ptr<Quest> quest) final
	{
		// Set quest to use needSatisfied()
		quest->simulation = this;
		quest->playerId = playerId;
		
		if (!questSystem(playerId)->WasQuestStarted(quest->classEnum()) &&
			!quest->needSatisfied()) // This is for quests that are reused
		{
			questSystem(playerId)->AddQuest(quest);
		}
	}
	bool NeedQuestExclamation(int32 playerId, QuestEnum questEnum) final {
		auto quest = questSystem(playerId)->GetQuest(questEnum);
		return quest != nullptr && quest->NeedExclamation();
	}
	void QuestUpdateStatus(int32 playerId, QuestEnum questEnum, int32 value = -1) final {
		auto quest = questSystem(playerId)->GetQuest(questEnum);
		if (quest != nullptr) {
			quest->UpdateStatus(value);
		}
	}


	bool TryDoNonRepeatAction(int32 playerId, NonRepeatActionEnum actionEnum, int32 nonRepeatTicks) final {
		if (Time::Ticks() > _playerIdToNonRepeatActionToAvailableTick[playerId][static_cast<int>(actionEnum)]) {
			_playerIdToNonRepeatActionToAvailableTick[playerId][static_cast<int>(actionEnum)] = Time::Ticks() + nonRepeatTicks;
			return true;
		}
		return false;
	}

	//

	void RefreshHeightForestColorTexture(TileArea area, bool isInstant) override {
		_gameManager->RefreshHeightForestColorTexture(area, isInstant);
	}
	void SetRoadWorldTexture(WorldTile2 tile, bool isRoad, bool isDirtRoad) override {
		_gameManager->SetRoadWorldTexture(tile, isRoad, isDirtRoad);
	}
	void RefreshHeightForestRoadTexture() override {
		_gameManager->RefreshHeightForestRoadTexture();
	}

	// Tech

	bool IsResearched(int32 playerId, TechEnum techEnum) final {
		if (!IsValidPlayer(playerId)) { // - Guard against IsResearched() crash in unlockedInfluence()
			return false;
		}
		return unlockSystem(playerId)->IsResearched(techEnum);
	}
	bool HasTargetResearch(int32 playerId) final { return unlockSystem(playerId)->hasTargetResearch(); }
	int32 sciTechsCompleted(int32 playerId) final { return unlockSystem(playerId)->techsCompleted(); }


	bool IsBuildingUnlocked(int32 playerId, CardEnum cardEnumIn) final {
		return unlockSystem(playerId)->isUnlocked(cardEnumIn);
	}

	// Prosperity
	void UpdateProsperityHouseCount(int32 playerId) final {
		unlockSystem(playerId)->UpdateProsperityHouseCount();
	}

	

	//! Debug
	void DrawLine(WorldAtom2 atom, FVector startShift, WorldAtom2 endAtom, FVector endShift, FLinearColor Color,
					float Thickness = 1.0f, float LifeTime = 10000) final 
	{
#if WITH_EDITOR
		_debugLineSystem.DrawLine(atom, startShift, endAtom, endShift, Color, Thickness, LifeTime);
#endif
	}

	void DrawArea(TileArea area, FLinearColor color, float tilt) final
	{
#if WITH_EDITOR
		//PUN_LOG("DrawArea %s", *ToFString(area.ToString()));

		area.ExecuteOnArea_WorldTile2([&](WorldTile2 tile) {
			DrawLine(tile.worldAtom2(), FVector::ZeroVector, tile.worldAtom2(), FVector(0, tilt, 10), color, 1.0f);
		});
#endif
	}

	void AddDemolishDisplayInfo(WorldTile2 tile, DemolishDisplayInfo demolishInfo)
	{
		if (_gameManager->IsInSampleRange(tile) &&
			_gameManager->zoomDistance() < WorldZoomTransition_Buildings) 
		{
			_regionToDemolishDisplayInfos[tile.regionId()].push_back(demolishInfo);
		}
	}

	std::vector<DemolishDisplayInfo>& GetDemolishDisplayInfo(int32 regionId)
	{
		std::vector<DemolishDisplayInfo>& demolishInfos = _regionToDemolishDisplayInfos[regionId];
		
		// Filter out info that was too old...
		for (size_t i = demolishInfos.size(); i-- > 0;) {
			if (demolishInfos[i].animationTicks() > Time::TicksPerSecond * 10) {
				demolishInfos.erase(demolishInfos.begin() + i);
			}
		}
		return demolishInfos;
	}

	// If half pop dies from starve/cold, happiness would -100
	int32 GetAverageHappiness(int32 playerId) final {
		return playerOwned(playerId).aveHappiness();
	}
	int32 taxHappinessModifier(int32 playerId) final { return playerOwned(playerId).taxHappinessModifier(); }
	int32 cannibalismHappinessModifier(int32 playerId) final {
		return TownhallCardCount(playerId, CardEnum::Cannibalism) > 0 ? -10 : 0;
	}
	int32 citizenDeathHappinessModifier(int32 playerId, SeasonStatEnum statEnum) final {
		SubStatSystem& statSystem = _statSystem.playerStatSystem(playerId);
		int32 deaths = statSystem.GetYearlyStat(statEnum);
		// unhappiness from deaths depends on the ratio (death):(pop+death)
		// unhappiness from death cap at 50, where (death):(pop+death) is 1:1
		int32 happinessModifier = -51 * deaths / (deaths + population(playerId) + 1);

		PUN_CHECK(happinessModifier < 1000 && happinessModifier > -1000);
		return happinessModifier;
	}

	void AddMigrationPendingCount(int32 playerId, int32 migrationCount) final
	{
		playerOwned(playerId).AddMigrationPendingCount(migrationCount);
	}

	void ImmigrationEvent(int32 playerId, int32 migrationCount, std::string message, PopupReceiverEnum receiverEnum) final
	{
		townhall(playerId).ImmigrationEvent(migrationCount, message, receiverEnum);
	}
	
	/*
	 * Cards
	 */

	bool CanBuyCard(int32 playerId, CardEnum buildingEnum)
	{
		// Not enough money
		int32 money = resourceSystem(playerId).money();
		if (money < cardSystem(playerId).GetCardPrice(buildingEnum)) {
			AddPopupToFront(playerId, "Not enough money to purchase the card.", ExclusiveUIEnum::CardHand1, "PopupCannot");
			return false;
		}

		if (!cardSystem(playerId).CanAddCardToBoughtHand(buildingEnum, 1)) {
			AddPopupToFront(playerId, "Reached hand limit for bought cards.", ExclusiveUIEnum::CardHand1, "PopupCannot");
			return false;
		}
		return true;
	}
	int32 BoughtCardCount(int32 playerId, CardEnum buildingEnum) final {
		return cardSystem(playerId).BoughtCardCount(buildingEnum);
	}
	int32 TownhallCardCount(int32 playerId, CardEnum cardEnum) final {
		return cardSystem(playerId).TownhallCardCount(cardEnum);
	}
	bool HasCardInAnyPile(int32 playerId, CardEnum cardEnum) final {
		return cardSystem(playerId).HasCardInAnyPile(cardEnum);
	}
	void AddDrawCards(int32 playerId, CardEnum cardEnum, int32 count) final {
		return cardSystem(playerId).AddDrawCards(cardEnum, count);
	}

	bool TryAddCardToBoughtHand(int32 playerId, CardEnum cardEnum, int32 cardCount = 1) final {
		return cardSystem(playerId).TryAddCardToBoughtHand(cardEnum, cardCount);
	}



	/*
	 * Storage
	 */

	bool isStorageAllFull(int32 playerId) final
	{
		std::vector<int32> storageIds = buildingIds(playerId, CardEnum::StorageYard);

		// Add warehouses
		std::vector<int32> warehouseIds = buildingIds(playerId, CardEnum::Warehouse);
		storageIds.insert(storageIds.end(), warehouseIds.begin(), warehouseIds.end());

		// Add markets
		std::vector<int32> marketIds = buildingIds(playerId, CardEnum::Market);
		storageIds.insert(storageIds.end(), marketIds.begin(), marketIds.end());
		
		for (int32 storageId : storageIds) {
			if (!building(storageId).subclass<StorageYard>().isFull()) {
				return false;
			}
		}
		return true;
	}
	int32 SpaceLeftFor(ResourceEnum resourceEnum, int32 storageId) final {
		Building& bld = building(storageId);
		return bld.subclass<StorageYard>().SpaceLeftFor(resourceEnum);
	}
	void RefreshStorageStatus(int32 storageId) final {
		Building& bld = building(storageId);
		bld.subclass<StorageYard>().RefreshStorage();
	}



	
public:
	const DescriptionUIState& descriptionUIState() { return _descriptionUIState; }
	void SetDescriptionUIState(DescriptionUIState uiState) {
		if (_descriptionUIState.objectType == ObjectTypeEnum::Building) {
			SetNeedDisplayUpdate(DisplayClusterEnum::Building, building(_descriptionUIState.objectId).centerTile().regionId());
		}
		if (uiState.objectType == ObjectTypeEnum::Building) {
			SetNeedDisplayUpdate(DisplayClusterEnum::Building, building(uiState.objectId).centerTile().regionId());
		}

		// On opening unit UI, play sound
		if (_descriptionUIState.objectType != ObjectTypeEnum::Unit && uiState.objectType == ObjectTypeEnum::Unit) {
			UnitEnum unitEnum = _unitSystem->unitEnum(uiState.objectId);
			if (IsAnimal(unitEnum)) {
				soundInterface()->SpawnAnimalSound(unitEnum, false, unitAtom(uiState.objectId), false);
			}
		}

		
		
		_descriptionUIState = uiState;
	}
	void TryRemoveDescriptionUI(ObjectTypeEnum type, int32_t objectId) override {
		_descriptionUIState.TryRemove(type, objectId);
	}

	bool isInitialized() { return _tickCount > 0; }

	int32 gameSpeed() { return _lastGameSpeed; }
	int32 gameSpeedMultiplier() final {
		if (_lastGameSpeed == -12) {
			return 1;
		}
		return _lastGameSpeed;
	}
	float gameSpeedFloat() {
		if (_lastGameSpeed == -12) {
			return 0.5f;
		}
		return _lastGameSpeed;
	}

	FString playerNameF(int32 playerId) final {
		return _gameManager->playerNameF(playerId);
	}
	FText playerNameT(int32 playerId) final {
		return FText::FromString(_gameManager->playerNameF(playerId));
	}
	std::string playerName(int32 playerId) final {
		return ToStdString(_gameManager->playerNameF(playerId));
	}

	
	std::vector<int32> allHumanPlayerIds() final {
		return _gameManager->allHumanPlayerIds();
	}
	std::vector<int32> connectedPlayerIds() final {
		return _gameManager->connectedPlayerIds();
	}
	

	std::string armyNodeName(int32 nodeId) {
		return playerName(building(nodeId).playerId());
	}

	std::vector<int32> GetCapitalArmyCounts(int32 playerId, bool skipWall = false) final
	{
		ArmyGroup* group = townhall(playerId).armyNode.GetArmyGroup(playerId);
		if (group) {
			return group->GetArmyCounts(skipWall);
		}
		return std::vector<int32>(ArmyEnumCount, 0);
	}

	std::vector<int32> GetTotalArmyCounts(int32 playerId, bool skipWall = false) final
	{
		std::vector<int32> nodeIdsVisited = playerOwned(playerId).nodeIdsVisited();
		std::vector<int32> armyCounts(ArmyEnumCount, 0);
		
		for (int32 nodeId : nodeIdsVisited) {
			CardEnum nodeEnum = building(nodeId).buildingEnum();
			if (nodeEnum == CardEnum::Townhall) {
				building<TownHall>(nodeId).armyNode.ExecuteOnAllGroups([&](const ArmyGroup& group) 	{
					if (group.playerId == playerId) {
						group.AddArmyToGivenArray(armyCounts, skipWall);
					}
				}, true);
			}
		}
		return armyCounts;
	}

	void GainAlly(int32 playerId1, int32 playerId2)
	{
		playerOwned(playerId1).GainAlly(playerId2);
		playerOwned(playerId2).GainAlly(playerId1);
	}
	void LoseAlly(int32 playerId1, int32 playerId2)
	{
		playerOwned(playerId1).LoseAlly(playerId2);
		playerOwned(playerId2).LoseAlly(playerId1);
	}

	/*
	 * Lobby
	 */
	MapSizeEnum mapSizeEnum() final {
		return _mapSettings.mapSizeEnum();
	}
	int32 difficultyConsumptionAdjustment(int32 playerId) final {
		if (GameConstants::IsHumanPlayer(playerId)) {
			return DifficultyConsumptionAdjustment[static_cast<int>(_mapSettings.difficultyLevel)];
		}
		return 0;
	}
	
	FMapSettings mapSettings() final {
		return _mapSettings;
	}

	/*
	 * Victory
	 */
	FGameEndStatus endStatus() final {
		return _endStatus;
	}
	void CheckDominationVictory(int32 playerIdToWin)
	{
		std::vector<int32> vassalBuildingIds = _playerOwnedManagers[playerIdToWin].vassalBuildingIds();

		int32 capitalsOwned = 1;
		for (int32 nodeId : vassalBuildingIds) {
			if (building(nodeId).isEnum(CardEnum::Townhall)) {
				capitalsOwned++;
			}
		}

		if (capitalsOwned == playerAndAICount() - 1) {
			AddPopup(playerIdToWin, "You have captured all but one capital. If you captured all capitals, you will achieve the domination victory.");
			AddPopupAll(PopupInfo(-1, playerName(playerIdToWin) + " have captured all but one capital. You will be defeated if " + playerName(playerIdToWin) + " manages to capture all the capitals."), playerIdToWin);
		}
		else if (capitalsOwned == playerAndAICount()) {
			_endStatus.victoriousPlayerId = playerIdToWin;
			_endStatus.gameEndEnum = GameEndEnum::DominationVictory;
		}
	}
	void ExecuteScienceVictory(int32 playerIdToWin) final {
		_endStatus.victoriousPlayerId = playerIdToWin;
		_endStatus.gameEndEnum = GameEndEnum::ScienceVictory;
	}

	bool HasTownhall(int32 playerId) final {
		return playerOwned(playerId).hasTownhall();
	}
	int32 homeProvinceId(int32 playerId) final
	{
		if (!HasTownhall(playerId)) {
			return -1;
		}
		return townhall(playerId).provinceId();
	}

	int32 TownAge(int32 playerId) {
		if (!HasTownhall(playerId)) {
			return 0;
		}
		return townhall(playerId).buildingAge();
	}
	

	bool IsReplayPlayer(int32 playerId) {
		return replaySystem().replayPlayers[playerId].isInitialize();
	}

	void AbandonTown(int32 playerId);

	/*
	 * Settings
	 */

	/*
	 * Snow
	 */
	bool IsSnowStart() {
		return _lastTickSnowAccumulation2 == 0 && _snowAccumulation2 > 0;
	}

	float snowHeightTundraStart() {
		if (PunSettings::Get("ForceSnowPercent") > 0) {
			return PunSettings::Get("ForceSnowPercent") / 100.0f;
		}
		if (PunSettings::TrailerMode()) {
			float minSnowFraction = PunSettings::Get("TrailerTundraMinSnowPercent") / 100.0f;
			return FDToFloat(_snowAccumulation3) * (1.0f - minSnowFraction) + minSnowFraction;
		}
		return FDToFloat(_snowAccumulation3);
	}
	float snowHeightBorealStart() {
		//return 1.0f;
		return FDToFloat(_snowAccumulation2);
	}
	float snowHeightForestStart() { // 0 to 1
		
		return FDToFloat(_snowAccumulation1);
	}
	

	bool playerChoseLocation(int32 playerId) { return playerOwned(playerId).hasChosenLocation(); }

	// Used to pause the game until the last player chose location
	bool AllPlayerHasTownhallAfterInitialTicks() final
	{
		// Below 2 sec, don't pause
		if (Time::Ticks() < Time::TicksPerSecond * 2) {
			return true;
		}

		std::vector<int32> connectedPlayerIds = _gameManager->connectedPlayerIds(false);
		for (int32 playerId : connectedPlayerIds) {
			if (!playerOwned(playerId).hasTownhall() &&
				!IsReplayPlayer(playerId))  // ReplayPlayer shouldn't pause the game
			{
				return false;
			}
		};
		return true;
	}

	void GenerateRareCardSelection(int32 playerId, RareHandEnum rareHandEnum, std::string rareHandMessage) final
	{
		return cardSystem(playerId).RollRareHand(rareHandEnum, rareHandMessage);
	}

	void CheckGetSeedCard(int32 playerId) final
	{
		auto checkGetSeed = [&](CardEnum seedCardEnum, GeoresourceEnum georesourceEnum)
		{
			if (!resourceSystem(playerId).HasSeed(seedCardEnum) &&
				!cardSystem(playerId).HasBoughtCard(seedCardEnum))
			{
				if (unlockSystem(playerId)->IsResearched(TechEnum::Plantation))
				{
					auto& provincesClaimed = playerOwned(playerId).provincesClaimed();
					for (int32 provinceId : provincesClaimed)
					{
						if (georesourceSystem().georesourceNode(provinceId).georesourceEnum == georesourceEnum)
						{
							AddPopup(playerId, "You found " + GetBuildingInfo(seedCardEnum).name + "!");
							cardSystem(playerId).AddCardToHand2(seedCardEnum);
							break;
						}
					}
				}
			}
		};

		checkGetSeed(CardEnum::CannabisSeeds, GeoresourceEnum::CannabisFarm);
		checkGetSeed(CardEnum::GrapeSeeds, GeoresourceEnum::GrapeFarm);
		checkGetSeed(CardEnum::CocoaSeeds, GeoresourceEnum::CocoaFarm);
		checkGetSeed(CardEnum::CottonSeeds, GeoresourceEnum::CottonFarm);
		checkGetSeed(CardEnum::DyeSeeds, GeoresourceEnum::DyeFarm);
		checkGetSeed(CardEnum::CoffeeSeeds, GeoresourceEnum::CoffeeFarm);
		checkGetSeed(CardEnum::TulipSeeds, GeoresourceEnum::TulipFarm);
	}

	void PopupInstantReply(int32 playerId, PopupReceiverEnum replyReceiver, int32 choiceIndex)
	{
		if (replyReceiver == PopupReceiverEnum::StartGame_Story) 
		{
			std::stringstream ss;
			ss << "Tutorials can be opened using the top-left \"?\" button.";
			ss << TutorialLinkString(TutorialLinkEnum::TutorialButton);

			ss << "\n";
			ss << "Camera control:";
			ss << "<bullet>W, A, S, D keys to pan</>";
			ss << "<bullet>Mouse wheel to zoom</>";
			ss << "<bullet>Q, E keys to rotate</>";
			ss << TutorialLinkString(TutorialLinkEnum::CameraControl);
			
			AddPopup(playerId, ss.str());
		}
		else if (replyReceiver == PopupReceiverEnum::DoneResearchEvent_ShowTree)
		{
			if (choiceIndex == 0) {
				unlockSystem(playerId)->shouldOpenTechUI = true;
			}
		}
		else if (replyReceiver == PopupReceiverEnum::UnlockedHouseTree_ShowProsperityUI)
		{
			if (choiceIndex == 0) {
				unlockSystem(playerId)->shouldOpenProsperityUI = true;
			}
		}
		else if (replyReceiver == PopupReceiverEnum::Approve_AbandonTown1)
		{
			if (choiceIndex == 0) {
				std::stringstream ss;
				ss << "Are you sure you want to abandon this settlement? This settlement will be destroyed as a result.";
				AddPopup(PopupInfo(playerId, ss.str(),
					{
						"Yes",
						"No"
					}, PopupReceiverEnum::Approve_AbandonTown2, true, "PopupBad")
				);
			}
		}
	}


	/*
	 * Serialize
	 */

	void Serialize(FArchive& Ar, TArray<uint8>& data, GameSaveChunkEnum saveChunkEnum, std::vector<int32>* crcs = nullptr, std::vector<std::string>* crcLabels = nullptr)
	{
		PUN_LLM(PunSimLLMTag::Simulation);
		
		PUN_CHECK(Ar.IsSaving() || Ar.IsLoading());

		auto checkChunkEnum = [&](GameSaveChunkEnum saveChunkEnumScope) {
			return saveChunkEnum == saveChunkEnumScope ||
					saveChunkEnum == GameSaveChunkEnum::All;
		};

		if (checkChunkEnum(GameSaveChunkEnum::Terrain))
		{
			_isLoadingForMainMenu = false;
			if (Ar.IsLoading()) {
				_gameEventSystem.ClearSubscriptions();
			}

			Ar << _tickCount;

			_mapSettings.Serialize(Ar);
			_endStatus.Serialize(Ar);


			Time::Serialize(Ar, data);

			{
				SERIALIZE_TIMER("Terrain", data, crcs, crcLabels);
				_terrainGenerator->Serialize(Ar);
			}
		}

		if (checkChunkEnum(GameSaveChunkEnum::Trees))
		{
			SERIALIZE_TIMER("Trees", data, crcs, crcLabels);
			treeSystem().Serialize(Ar, data, crcs, crcLabels);
		}

		if (checkChunkEnum(GameSaveChunkEnum::Flood1))
		{
			SERIALIZE_TIMER("Flood", data, crcs, crcLabels);
			_floodSystem.Serialize(Ar);
		}
		
		if (checkChunkEnum(GameSaveChunkEnum::Flood2))
		{
			SERIALIZE_TIMER("Flood2", data, crcs, crcLabels);
			_floodSystemHuman.Serialize(Ar);
		}

		
		if (checkChunkEnum(GameSaveChunkEnum::Others))
		{
			{
				SERIALIZE_TIMER("Regions", data, crcs, crcLabels);

				_provinceSystem.Serialize(Ar);

				_regionSystem->Serialize(Ar);
				_georesourceSystem->Serialize(Ar);
				_terrainChanges.Serialize(Ar);

			}

			{
				SERIALIZE_TIMER("PathAI", data, crcs, crcLabels);
				_pathAI->Serialize(Ar);

				//// Check flood system
				//for (int32 i = 0; i < GameMapConstants::TilesPerWorld; i++) {
				//	if (_terrainGenerator->terrainTileType(i) != TerrainTileType::None) {
				//		PUN_CHECK(_floodSystem.GetFloodId(WorldTile2(i)) == -1);
				//	}
				//}
			}
			{
				SERIALIZE_TIMER("PathAI2", data, crcs, crcLabels);
				_pathAIHuman->Serialize(Ar);
			}

			{
				SERIALIZE_TIMER("Unit", data, crcs, crcLabels);
				_unitSystem->Serialize(Ar);
			}

			{
				//SERIALIZE_TIMER("Building", data, crcs, crcLabels);
				_buildingSystem->Serialize(Ar, data, crcs, crcLabels, false);
			}

			{
				SERIALIZE_TIMER("GlobalSys", data, crcs, crcLabels);
				_overlaySystem.Serialize(Ar);

				_statSystem.Serialize(Ar);
				_dropSystem.Serialize(Ar);
				_worldTradeSystem.Serialize(Ar);

				_replaySystem.Serialize(Ar);
				_eventLogSystem.Serialize(Ar);
			}

			{
				//SERIALIZE_TIMER("PlayerSys", data, crcs, crcLabels);

#define LOOP(sysName, func) {\
								SERIALIZE_TIMER(sysName, data, crcs, crcLabels)\
								for (size_t i = 0; i < GameConstants::MaxPlayersAndAI; i++) { func; }\
							}

				LOOP("Resource", _resourceSystems[i].Serialize(Ar));
				LOOP("Quest", _questSystems[i].Serialize(Ar));

				LOOP("Unlock", _unlockSystems[i].Serialize(Ar));
				LOOP("PlayerParam", _playerParameters[i].Serialize(Ar));
				LOOP("PlayerOwned", _playerOwnedManagers[i].Serialize(Ar));
				LOOP("Popup", _popupSystems[i].Serialize(Ar));
				LOOP("Cards", _cardSystem[i].Serialize(Ar));

				LOOP("AI", _aiPlayerSystem[i].Serialize(Ar));
				LOOP("NonRepeatAction", SerializeVecValue(Ar, _playerIdToNonRepeatActionToAvailableTick[i]));

#undef LOOP
			}

			//SerializeVecValue(Ar, _tickHashes);
			//SerializeVecValue(Ar, _serverTickHashes);

			// Snow
			Ar << _snowAccumulation3;
			Ar << _snowAccumulation2;
			Ar << _snowAccumulation1;

			Ar << _lastTickSnowAccumulation3;
			Ar << _lastTickSnowAccumulation2;
			Ar << _lastTickSnowAccumulation1;

			// Others
			

			// Refresh Territory Display
			for (size_t playerId = 0; playerId < _playerOwnedManagers.size(); playerId++) {
				if (_playerOwnedManagers[playerId].hasChosenLocation()) {
					AddNeedDisplayUpdateId(DisplayGlobalEnum::Territory, playerId);
				}
			}
		}

		//PUN_LOG("UnitSystem Tick unitCount():%d _unitLeans:%d", unitSystem().unitCount(), unitSystem().unitCount() - unitSystem().deadCount());
	}

	void MainMenuDisplayInit()
	{
		_treeSystem = std::make_unique<TreeSystem>();
		_terrainGenerator = std::make_unique<PunTerrainGenerator>();
		
		_georesourceSystem = std::make_unique<GeoresourceSystem>();
		_georesourceSystem->InitGeoresourceSystem(this, false);
		
		_terrainChanges.Init(this);
		
		_buildingSystem = std::make_unique<BuildingSystem>();
		_buildingSystem->Init(this);
		
		_overlaySystem.Init(this);

		//while (_resourceSystems.size() < GameConstants::MaxPlayersAndAI) {
		for (int32 i = _resourceSystems.size(); i < GameConstants::MaxPlayersAndAI; i++) {
			int32 playerId = _resourceSystems.size();
			_resourceSystems.push_back(ResourceSystem(playerId, this));
		}
	}
	void SerializeForMainMenu(FArchive &Ar, const std::vector<int32>& sampleRegionIds)
	{
		_isLoadingForMainMenu = true;
		
		TArray<uint8> data;
		_treeSystem->SerializeForMainMenu(Ar, sampleRegionIds, data);
		_terrainGenerator->SerializeForMainMenu(Ar, sampleRegionIds);

		_buildingSystem->Serialize(Ar, data, nullptr, nullptr, true);
		
		_overlaySystem.Serialize(Ar); // For road
		
		for (size_t i = 0; i < GameConstants::MaxPlayersAndAI; i++) {
			_resourceSystems[i].Serialize(Ar);
		}
	}
	void SaveBuildingsToJSON(const TSharedRef<FJsonObject>& jsonObject, const std::vector<int32>& sampleRegionIds)
	{
		auto& buildingSubregionList = _buildingSystem->buildingSubregionList();

		TArray<TSharedPtr<FJsonValue>> buildingListJson;
		
		for (int32 sampleId : sampleRegionIds)
		{
			buildingSubregionList.ExecuteRegion(WorldRegion2(sampleId), [&](int32 buildingId)
			{	
				Building& bld = building(buildingId);

				if (!IsRoad(bld.buildingEnum())) 
				{
					TSharedPtr<FJsonObject> buildingJson = MakeShared<FJsonObject>();

					buildingJson->SetNumberField("buildingEnum", bld.buildingEnumInt());
					buildingJson->SetNumberField("centerX", bld.centerTile().x);
					buildingJson->SetNumberField("centerY", bld.centerTile().y);
					buildingJson->SetNumberField("faceDirection", static_cast<int>(bld.faceDirection()));

					TSharedPtr<FJsonValue> jsonValue = MakeShared<FJsonValueObject>(buildingJson);
					buildingListJson.Add(jsonValue);
				}
			});
		}

		jsonObject->SetArrayField("BuildingList", buildingListJson);
	}
	void LoadBuildingsFromJSON(const TSharedRef<FJsonObject>& jsonObject)
	{
		// Choose location
		WorldRegion2 centerRegion;
		const TArray<TSharedPtr<FJsonValue>>& buildingListJson = jsonObject->GetArrayField("BuildingList");
		for (const TSharedPtr<FJsonValue>& buildingJsonValue : buildingListJson)
		{
			TSharedPtr<FJsonObject> buildingJson = buildingJsonValue->AsObject();
			if (buildingJson->GetIntegerField("buildingEnum") == static_cast<int>(CardEnum::Townhall))
			{
				WorldTile2 center(buildingJson->GetIntegerField("centerX"), buildingJson->GetIntegerField("centerY"));
				centerRegion = center.region();

				//georesourceSystem().TakeSpot(playerId(), centerRegion.regionId());

				// TODO: proper province
				int32 provinceId = centerRegion.regionId();
				
				// Clear any regional building
				ClearProvinceBuildings(provinceId);

				// Claim Land
				SetProvinceOwnerFull(provinceId, playerId());

				FPlaceBuilding params;
				params.buildingEnum = static_cast<uint8>(CardEnum::Townhall);
				params.faceDirection = static_cast<uint8>(Direction::S);
				params.center = center;
				params.area = BuildingArea(params.center, GetBuildingInfo(CardEnum::Townhall).size, static_cast<Direction>(params.faceDirection));
				params.playerId = playerId();
				int32 townhallId = PlaceBuilding(params);

				building(townhallId).InstantClearArea();
				building(townhallId).FinishConstruction();
				params.area.ExecuteOnArea_WorldTile2([&](WorldTile2 tile) {
					SetWalkable(tile, true);
				});

				// PlayerOwnedManager
				_playerOwnedManagers[playerId()].townHallId = townhallId;
				_playerOwnedManagers[playerId()].justChoseLocation = true;
				_playerOwnedManagers[playerId()].needChooseLocation = false;

				break;
			}
		}

		resourceSystem(playerId()).ChangeMoney(1000000);

		// Claim Land
		// TODO: bring back?
		//for (int32 y = centerRegion.y - 2; y <= centerRegion.y + 2; y++) {
		//	for (int32 x = centerRegion.x - 2; x <= centerRegion.x + 2; x++)
		//	{
		//		FClaimLand command;
		//		command.playerId = playerId();
		//		command.regionId = WorldRegion2(x, y).regionId();
		//		command.claimEnum = CallbackEnum::ClaimLandMoney;
		//		ClaimLand(command);
		//	}
		//}

		SimSettings::Set("CheatFastBuild", 1);

		// Build buildings
		for (const TSharedPtr<FJsonValue>& buildingJsonValue : buildingListJson)
		{
			TSharedPtr<FJsonObject> buildingJson = buildingJsonValue->AsObject();
			FPlaceBuilding command;
			command.buildingEnum = buildingJson->GetIntegerField("buildingEnum");
			command.center = WorldTile2(buildingJson->GetIntegerField("centerX"), buildingJson->GetIntegerField("centerY"));
			command.faceDirection = buildingJson->GetIntegerField("faceDirection");

			command.area = BuildingArea(command.center, GetBuildingInfoInt(command.buildingEnum).size, static_cast<Direction>(command.faceDirection));
			command.playerId = playerId();
			
			PlaceBuilding(command);
		}
	}


	//! Interaction condition helper
	//bool CanTrainUnit(int32 buildingId);

private:
	/**
	 * Player interactions
	 */
public:
	void ExecuteNetworkCommands(std::vector<std::shared_ptr<FNetworkCommand>>& commands) final
	{
		for (auto& command : commands) {
			ExecuteNetworkCommand(command);
		}
	}
	bool ExecuteNetworkCommand(std::shared_ptr<FNetworkCommand> command) final;
	
	int32 PlaceBuilding(FPlaceBuilding parameters) final;
	void PlaceDrag(FPlaceDrag parameters) final;
	void JobSlotChange(FJobSlotChange command) final;
	void SetAllowResource(FSetAllowResource command) final;
	void SetPriority(FSetPriority command) final;
	void SetTownPriority(FSetTownPriority command) final;
	void SetGlobalJobPriority(FSetGlobalJobPriority command) final;
	void GenericCommand(FGenericCommand command) final;

	void TradeResource(FTradeResource command) final;
	void SetIntercityTrade(FSetIntercityTrade command) final;
	void UpgradeBuilding(FUpgradeBuilding command) final;
	void ChangeWorkMode(FChangeWorkMode command) final;
	void ChooseLocation(FChooseLocation command) final;
	void ChooseInitialResources(FChooseInitialResources command) final;
	void PopupDecision(FPopupDecision command) final;
	void RerollCards(FRerollCards command) final;
	
	void SelectRareCard(FSelectRareCard command) final;
	void BuyCards(FBuyCard command) final;
	void SellCards(FSellCards command) final;
	void UseCard(FUseCard command) final;
	void UnslotCard(FUnslotCard command) final;

	void Attack(FAttack command) final;
	//void TrainUnit(FTrainUnit command) final;

	void ClaimLand(FClaimLand command) final;
	void ChooseResearch(FChooseResearch command) final;

	void ChangeName(FChangeName command) final;
	void SendChat(FSendChat command) final;

	void Cheat(FCheat command) final;


private:
	void PlaceInitialTownhallHelper(FPlaceBuilding command, int32 townhallId);
	
	void InitRegionalBuildings();

	void DemolishCritterBuildingsIncludingFronts(WorldTile2 tile, int32 playerId);

	void CheckIntegrity()
	{
		ExecuteOnPlayersAndAI([&](int32_t playerId) {
			_resourceSystems[playerId].CheckIntegrity_ResourceSys();
		});
	}

public:
	DescriptionUIState _descriptionUIState;
	
private:
	int32 _tickCount = 0;

	std::unique_ptr<PunTerrainGenerator> _terrainGenerator;
	std::unique_ptr<TreeSystem> _treeSystem;
	std::unique_ptr<GameRegionSystem> _regionSystem;
	std::unique_ptr<GeoresourceSystem> _georesourceSystem;
	PunTerrainChanges _terrainChanges;

	GameMapFlood _floodSystem;
	GameMapFlood _floodSystemHuman;
	ProvinceSystem _provinceSystem;
	//GameMapFloodShortDistance _floodSystem2;
	//GameMapFloodShortDistance _floodSystem2Human;
	
	std::unique_ptr<PunAStar128x256> _pathAI;
	std::unique_ptr<PunAStar128x256> _pathAIHuman;
	GameEventSystem _gameEventSystem;
	
	std::unique_ptr<UnitSystem> _unitSystem;
	std::unique_ptr<BuildingSystem> _buildingSystem;
	OverlaySystem _overlaySystem;

	StatSystem _statSystem;
	ResourceDropSystem _dropSystem;
	WorldTradeSystem _worldTradeSystem;

	ReplaySystem _replaySystem;
	ChatSystem _chatSystem;
	EventLogSystem _eventLogSystem;

	std::vector<ResourceSystem> _resourceSystems;
	std::vector<QuestSystem> _questSystems;
	std::vector<UnlockSystem> _unlockSystems;
	std::vector<PlayerParameters> _playerParameters;
	std::vector<PlayerOwnedManager> _playerOwnedManagers;
	std::vector<PopupSystem> _popupSystems;
	std::vector<BuildingCardSystem> _cardSystem;

	std::vector<AIPlayerSystem> _aiPlayerSystem;
	
	std::vector<std::vector<int32>> _playerIdToNonRepeatActionToAvailableTick;

	// TODO: Remove
	// int32 _playerCount = 0;
	// std::vector<FString> _playerNames;

	FMapSettings _mapSettings;
	FGameEndStatus _endStatus;
	
	std::vector<int32> _tickHashes;
	std::vector<int32> _serverTickHashes;

	// Snow (Season * Celsius) 
	FloatDet _snowAccumulation3 = 0;
	FloatDet _snowAccumulation2 = 0;
	FloatDet _snowAccumulation1 = 0;
	FloatDet _lastTickSnowAccumulation3 = 0;
	FloatDet _lastTickSnowAccumulation2 = 0;
	FloatDet _lastTickSnowAccumulation1 = 0;
	
private:
	//! Not serialized
	IGameManagerInterface* _gameManager = nullptr;
	IGameSoundInterface* _soundInterface = nullptr;
	IGameUIInterface* _uiInterface = nullptr;
	
	bool _isLoadingFromFile = false;
	bool _isLoadingForMainMenu = false;

	int32 _lastGameSpeed = 1;

	//std::string _loadingText;
	
	std::vector<std::vector<bool>> _displayEnumToRegionToNeedUpdate;
	std::vector<bool> _displayEnumToNeedUpdate;
	std::vector<std::vector<int32>> _displayEnumToNeedUpdateIds;

	std::vector<std::vector<DemolishDisplayInfo>> _regionToDemolishDisplayInfos;
	
	DebugLineSystem _debugLineSystem;

public:
	// Save Check
	SaveCheckState saveCheckState = SaveCheckState::None;
	int32 saveCheckStartTick = -1;
	std::vector<std::vector<int32>> saveCheckCrcsToTick;
	std::vector<std::string> saveCheckCrcLabels;

	bool saveCheckActive() { return saveCheckStartTick != -1; }
};

