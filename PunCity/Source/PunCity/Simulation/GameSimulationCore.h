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
#include "GlobalResourceSystem.h"

//#include "PolicySystem.h"
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

enum class TickHashEnum
{
	Unit,
	Building,

	Count,
};

struct TickHashes
{
	TArray<int32> allTickHashes;

	void AddTickHash(int32 tickCount, TickHashEnum tickHashEnum, int32 tickHash) {
		check(allTickHashes.Num() == GetTickIndex(tickCount, tickHashEnum));
		allTickHashes.Add(tickHash);
	}

	int32 GetTickHashes(int32 tickCount, TickHashEnum tickHashEnum) {
		return allTickHashes[GetTickIndex(tickCount, tickHashEnum)];
	}

	int32 TickCount() const {
		return allTickHashes.Num() / TickHashEnumCount();
	}

	void operator>>(FArchive &Ar) {
		Ar << allTickHashes;
	}

	
	static int32 TickHashEnumCount() { return static_cast<int32>(TickHashEnum::Count); }

	static int32 GetTickIndex(int32 tickCount, TickHashEnum tickHashEnum) {
		return tickCount * TickHashEnumCount() + static_cast<int32>(tickHashEnum);
	}
	
};

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


	/*
	 * Desync Check
	 */
	void GetUnsentTickHashes(int32 startTick, TArray<int32>& tickHashes) {
#if WITH_EDITOR
		for (int32 i = startTick * TickHashes::TickHashEnumCount(); i < _tickHashes.allTickHashes.Num(); i++) {
			tickHashes.Add(_tickHashes.allTickHashes[i]);
		}
#endif
	}
	void AppendAndCompareServerHashes(int32 hashSendTick, const TArray<int32>& newAllTickHashes)
	{
#if WITH_EDITOR
		_LOG(PunTickHash, "AppendAndCompareServerHashes_Before newAllTickHashes:%d _serverTickHashes.TickCount:%d", newAllTickHashes.Num(), _serverTickHashes.TickCount());

		PUN_CHECK(hashSendTick <= _serverTickHashes.TickCount());
		
		int32 index = hashSendTick * TickHashes::TickHashEnumCount();
		for (int32 i = 0; i < newAllTickHashes.Num(); i++) {
			if (index < _serverTickHashes.allTickHashes.Num()) {
				_serverTickHashes.allTickHashes[index] = newAllTickHashes[i];
			} else {
				_serverTickHashes.allTickHashes.Add(newAllTickHashes[i]);
			}
			index++;
		}
		int32 checkTickCount = std::min(_tickHashes.TickCount(), _serverTickHashes.TickCount());
		for (int32 i = 0; i < checkTickCount; i++) {
			int32 hashIndex0 = i * TickHashes::TickHashEnumCount();

			auto compareHashes = [&](TickHashEnum tickHashEnum) {
				return _tickHashes.allTickHashes[hashIndex0 + static_cast<int32>(tickHashEnum)] == _serverTickHashes.allTickHashes[hashIndex0 + static_cast<int32>(tickHashEnum)];
			};
			
			check(compareHashes(TickHashEnum::Unit));
			check(compareHashes(TickHashEnum::Building));
		}

		_LOG(PunTickHash, "AppendAndCompareServerHashes_After _tickHashes.TickCount:%d _serverTickHashes.TickCount:%d", _tickHashes.TickCount(), _serverTickHashes.TickCount());
#endif
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
	//GameMapFlood& floodSystemHuman() final { return _floodSystemHuman; }
	ProvinceSystem& provinceSystem() final { return _provinceSystem; }
	GameRegionSystem& regionSystem() final { return *_regionSystem; }
	
	ResourceDropSystem& dropSystem() final { return _dropSystem; }
	//GameEventSource& eventSource(EventSourceEnum eventEnum) final { return _gameEventSystem.source(eventEnum); }

	TownManager& townManager(int32 townId) final {
		check(_townManagers[townId]);
		return *_townManagers[townId];
	}
	ResourceSystem& resourceSystem(int32 townId) final { return _resourceSystems[townId]; }

	PlayerOwnedManager& playerOwned(int32 playerId) final { return _playerOwnedManagers[playerId]; }
	PlayerOwnedManager& playerOwnedFromTownId(int32 townId) final { return _playerOwnedManagers[townManager(townId).playerId()]; }
	
	GlobalResourceSystem& globalResourceSystem(int32 playerId) final { return _globalResourceSystems[playerId]; }
	QuestSystem* questSystem(int32 playerId) final { return playerId < _questSystems.size() ? &_questSystems[playerId] : nullptr; }
	UnlockSystem* unlockSystem(int32 playerId) final { return playerId < _unlockSystems.size() ? &_unlockSystems[playerId] : nullptr; }
	PlayerParameters* parameters(int32 playerId) final { return playerId < _unlockSystems.size() ? &_playerParameters[playerId] : nullptr; }
	SubStatSystem& statSystem(int32 townId) final { return _statSystem.townStatSystem(townId); }

	BuildingCardSystem& cardSystem(int32 playerId) final { return _cardSystem[playerId]; }

	ChatSystem& chatSystem() { return _chatSystem; }
	EventLogSystem& eventLogSystem() { return _eventLogSystem; }

	WorldTradeSystem& worldTradeSystem() final { return _worldTradeSystem; }

	ReplaySystem& replaySystem() { return _replaySystem; }

	PunTerrainChanges& terrainChanges() final { return _terrainChanges; }

	//inline IUnitDataSource& unitDataSource() final { return *static_cast<IUnitDataSource*>(_unitSystem.get()); }

	IGameSoundInterface* soundInterface() final { return _soundInterface; }
	IGameUIInterface* uiInterface() final { return _uiInterface; }
	IGameManagerInterface* gameManagerInterface() final { return _gameManager; }

	DebugLineSystem& debugLineSystem() final { return _debugLineSystem; }

	AIPlayerSystem& aiPlayerSystem(int32_t playerId) { return _aiPlayerSystem[playerId]; }

	int32 playerCount() final {
		return _gameManager->playerCount();
	}

	int32 townCount() final {
		return _resourceSystems.size();
	}
	const std::vector<int32>& GetTownIds(int32 playerId) final {
		return _playerOwnedManagers[playerId].townIds();
	}
	int32 townPlayerId(int32 townId) final {
		if (townId == -1) {
			return -1;
		}
		return townManager(townId).playerId();
	}
	int32 buildingTownId(int32 buildingId) final {
		if (buildingId == -1) {
			return -1;
		}
		return building(buildingId).townId();
	}

	int32 GetNextTown(bool forward, int32 currentTownId, int32 playerId) final
	{
		if (currentTownId == -1) {
			return -1;
		}
		int32 shift = forward ? 1 : -1;
		int32 oldTownId = currentTownId;
		const auto& townIds = GetTownIds(playerId);
		for (int32 i = 0; i < townIds.size(); i++) {
			if (oldTownId == townIds[i]) {
				return townIds[(i + shift + townIds.size()) % townIds.size()];
			}
		}
		UE_DEBUG_BREAK();
		return playerId;
	}

	
	
	bool IsTownOwnedByPlayer(int32 townIdIn, int32 playerId) final {
		const auto& townIds = _playerOwnedManagers[playerId].townIds();
		for (int32 townId : townIds) {
			if (townIdIn == townId) {
				return true;
			}
		}
		return false;
	}

	bool IsTownhallOverlapProvince(int32 provinceId, int32 provincePlayerId) final
	{
		const auto& townIds = GetTownIds(provincePlayerId);
		for (int32 townId : townIds)
		{
			TileArea townhallArea = GetTownhall(townId).area();
			vector<int32> townhallOverlapProvinceIds = provinceSystem().GetProvinceIdsFromArea(townhallArea, true);
			for (int32 townhallOverlapProvinceId : townhallOverlapProvinceIds) {
				if (townhallOverlapProvinceId == provinceId) {
					return true;
				}
			}
		}
		return false;
	}
	
	int32 populationTown(int32 townId) final {
		return _townManagers[townId]->population();
	}
	int32 populationPlayer(int32 playerId) final {
		const auto& townIds = _playerOwnedManagers[playerId].townIds();
		int32 population = 0;
		for (int32 townId : townIds) {
			population += _townManagers[townId]->population();
		}
		return population;
	}
	int32 worldPlayerPopulation() final
	{
		int32 result = 0;
		ExecuteOnPlayersAndAI([&](int32 playerId) {
			result += populationTown(playerId);
		});
		return result;
	}
	
	int32 resourceCountTown(int32 townId, ResourceEnum resourceEnum) final {
		return _resourceSystems[townId].resourceCount(resourceEnum);
	}
	int32 resourceCountTownSafe(int32 townId, ResourceEnum resourceEnum) final {
		PUN_ENSURE(IsValidTown(townId), return 0);
		return _resourceSystems[townId].resourceCount(resourceEnum);
	}
	
	int32 resourceCountPlayer(int32 playerId, ResourceEnum resourceEnum) final {
		const auto& townIds = GetTownIds(playerId);
		int32 count = 0;
		for (int32 townId : townIds) {
			count += _resourceSystems[townId].resourceCount(resourceEnum);
		}
		return count;
	}

	bool HasEnoughResource(int32 townId, const std::vector<ResourcePair> pairs) {
		for (const ResourcePair& pair : pairs) 
		{
			if (pair.resourceEnum == ResourceEnum::Food) {
				if (foodCount(townId) < pair.count) {
					return false;
				}
			}
			else {
				if (resourceCountTown(townId, pair.resourceEnum) < pair.count) {
					return false;
				}
			}
		}
		return true;
	}

	void AddResourceGlobal(int32 townId, ResourceEnum resourceEnum, int32 amount) final {
		resourceSystem(townId).AddResourceGlobal(resourceEnum, amount, *this);
	}

	bool IsOutputTargetReached(int32 townId, ResourceEnum resourceEnum) final {
		if (resourceEnum == ResourceEnum::None) {
			return false;
		}
		int32 outputTarget = townManager(townId).GetOutputTarget(resourceEnum);
		return outputTarget != -1 && resourceSystem(townId).resourceCountWithPop(resourceEnum) >= outputTarget;
	}
	bool IsFarBelowOutputTarget(int32 townId, ResourceEnum resourceEnum) {
		if (resourceEnum == ResourceEnum::None) {
			return false;
		}
		int32 outputTarget = townManager(townId).GetOutputTarget(resourceEnum);
		if (outputTarget == -1) {
			return false;
		}
		int32 belowThreshold = std::min(outputTarget * 8 / 10, std::max(outputTarget - 20, 0));
		return resourceSystem(townId).resourceCountWithPop(resourceEnum) < belowThreshold;
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

	std::pair<int32, int32> GetStorageCapacity(int32 townId, bool includeUnderConstruction = false) final
	{
		std::vector<int32> storageIds = buildingIds(townId, CardEnum::StorageYard);
		CppUtils::AppendVec(storageIds, buildingIds(townId, CardEnum::Warehouse));

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

	

	int32 foodCount(int32 townId) final {
		int count = 0;
		for (ResourceEnum foodEnum : StaticData::FoodEnums) {
			count += _resourceSystems[townId].resourceCount(foodEnum);
		}
		return count;
	}
	int32 GetResourceCount(int32 townId, const std::vector<ResourceEnum>& resourceEnums) final {
		int32 result = 0;
		for (ResourceEnum resourceEnum : resourceEnums) {
			result += resourceCountTown(townId, resourceEnum);
		}
		return result;
	}
	bool HasSeed(int32 playerId, CardEnum seedCardEnum) final {
		return globalResourceSystem(playerId).HasSeed(seedCardEnum);
	}

	int32 influence(int32 playerId) final {
		return globalResourceSystem(playerId).influence();
	}
	int32 influence100(int32 playerId) final {
		return globalResourceSystem(playerId).influence100();
	}
	
	int32 money(int32 playerId) final {
		return globalResourceSystem(playerId).money();
	}
	void ChangeMoney(int32 playerId, int32 moneyChange) final {
		globalResourceSystem(playerId).ChangeMoney(moneyChange);
	}
	void ChangeMoney100(int32 playerId, int32 moneyChange100) final {
		globalResourceSystem(playerId).ChangeMoney100(moneyChange100);
	}

	void ChangeInfluence(int32 playerId, int32 influenceChange) final {
		globalResourceSystem(playerId).ChangeInfluence(influenceChange);
	}

	int32 price100(ResourceEnum resourceEnum) final {
		return _worldTradeSystem.price100(resourceEnum);
	}
	int32 price(ResourceEnum resourceEnum) final {
		return _worldTradeSystem.price100(resourceEnum) / 100;
	}

	TownHall& GetTownhallCapital(int32 playerId) final {
		check(playerId != -1);
		int32 townhallId = playerOwned(playerId).capitalTownhallId;
		check(townhallId != -1);
		return building(townhallId).subclass<TownHall>(CardEnum::Townhall);
	}
	TownHall& GetTownhall(int32 townId) final {
		int32 townhallId = townManager(townId).townHallId;
		check(townhallId != -1);
		return building(townhallId).subclass<TownHall>(CardEnum::Townhall);
	}
	TownHall* GetTownhallPtr(int32 townId) final {
		int32 townHallId = GetTownhallId(townId);
		if (townHallId == -1) return nullptr;
		return &(building(townHallId).subclass<TownHall>(CardEnum::Townhall));
	}
	int32 GetTownhallId(int32 townId) override {
		if (townId == -1) return -1;
		if (townId >= _townManagers.size()) return -1;
		return townManager(townId).townHallId;
	}

	bool IsValidTown(int32 townId) override {
		if (townId < 0) return false;
		if (townId >= _townManagers.size()) return false;
		return townManager(townId).townHallId != -1;
	}
	int32 FindTownIdFromName(int32 playerId, FString townName)
	{
		const auto& townIds = GetTownIds(playerId);
		for (int32 townId : townIds) {
			if (townName == townNameT(townId).ToString()) {
				return townId;
			}
		}
		return -1;
	}

	int32 GetTownLvl(int32 townId) final {
		return GetTownhall(townId).townhallLvl;
	}
	int32 GetTownLvlMax(int32 playerId) final {
		const auto& townIds = playerOwned(playerId).townIds();
		int32 maxLvl = 1;
		for (int32 townId : townIds) {
			maxLvl = std::max(maxLvl, GetTownLvl(townId));
		}
		return maxLvl;
	}
	
	WorldTile2 GetTownhallGateCapital(int32 playerId) final {
		return GetTownhallCapital(playerId).gateTile();
	}
	WorldTile2 GetTownhallGateFast(int32 townId) final {
		return GetTownhall(townId).gateTile();
	}
	WorldTile2 GetTownhallGate(int32 townId) final {
		if (townId == -1) return WorldTile2::Invalid;
		int32 townhallId = townManager(townId).townHallId;
		if (townhallId == -1) return WorldTile2::Invalid;
		return building(townhallId).gateTile();
	}
	FText townNameT(int32 townId) final {
		if (townId == -1) {
			return NSLOCTEXT("GameSimulationCore", "None", "None");
		}
		int32 townhallId = townManager(townId).townHallId;
		if (townhallId == -1) {
			return NSLOCTEXT("GameSimulationCore", "None", "None");
		}
		TownHall& townhall = building(townhallId).subclass<TownHall>(CardEnum::Townhall);
		return townhall.townNameT();
		//return GetTownhall(townId).townNameT();
	}

	int32 GetTownAgeTicks(int32 townId) final {
		return GetTownhall(townId).townAgeTicks();
	}

	bool IsConnectedToTowns(WorldTile2 tile, int32 playerId, std::vector<uint32>& path) final
	{
		const auto& townIds = GetTownIds(playerId);
		for (int32 townIdTemp : townIds) {
			WorldTile2 gateTile = GetTownhallGate(townIdTemp);
			if (gateTile.isValid() &&
				pathAI()->FindPathRoadOnly(tile.x, tile.y, gateTile.x, gateTile.y, path))
			{
				return true;
			}
		}
		return false;
	}


	void AddImmigrants(int32 townId, int32 count, WorldTile2 tile) final {
		if (townId != -1) {
			GetTownhall(townId).AddImmigrants(count, tile);
		}
	}

	int32 lordPlayerId(int32 playerId) final {
		return playerOwned(playerId).lordPlayerId();
	}

	//ArmyNode& GetArmyNode(int32 buildingId) final {
	//	return building(buildingId).subclass<ArmyNodeBuilding>().GetArmyNode();
	//}
	//std::vector<int32> GetArmyNodeIds(int32 playerId) final {
	//	std::vector<int32> nodes = playerOwned(playerId).vassalBuildingIds();
	//	nodes.push_back(townhall(playerId).armyNode.nodeId);
	//	return nodes;
	//}

	WorldAtom2 homeAtom(int32 townId) final
	{
		// Go to townhall center if there is townhall
		if (HasTownhall(townPlayerId(townId))) {
			return GetTownhall(townId).centerTile().worldAtom2();
		}
		// No townhall case, use province center
		if (townManager(townId).provincesClaimed().size() > 0) {
			return GetProvinceCenterTile(townManager(townId).provincesClaimed().front()).worldAtom2();
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
	FloatDet MaxCelsius(WorldTile2 tile) final
	{
		// Guard Crash
		if (_terrainGenerator == nullptr) {
			return Time::MaxCelsiusBase();
		}
		int32 temperatureFraction10000 = _terrainGenerator->GetTemperatureFraction10000(tile.x, _terrainGenerator->GetRainfall100(tile));
		return ModifyCelsiusByBiome(temperatureFraction10000, Time::MaxCelsiusBase(), maxCelsiusDivider); // maxCelsiusDivider: Max celsius in general change less drastically than MinCelsius (maxCelsiusDivider)
	}

	FloatDet ModifyCelsiusByBiome(int32 temperatureFraction10000, FloatDet celsius, int32 divider = 1)
	{
		const FloatDet tundraModifierMax = FDOne * 10; // Tundra has no divider, summer temperature is still low
		const FloatDet borealModifierMax = FDOne * 15 / divider;
		const FloatDet jungleModifierMax = FDOne * 5 / divider; // This increases the temperature

		// Tundra
		if (temperatureFraction10000 > tundraTemperatureStart10000) {
			FloatDet modifier = tundraModifierMax * (temperatureFraction10000 - tundraTemperatureStart10000) / (10000 - tundraTemperatureStart10000);
			modifier *= 3; // reach modifierMax 3 times faster (1/3 of band is the increment)
			
			return celsius - borealModifierMax - std::min(modifier, tundraModifierMax);
		}
		// Boreal
		if (temperatureFraction10000 > borealTemperatureStart10000) {
			FloatDet modifier = borealModifierMax * (temperatureFraction10000 - borealTemperatureStart10000) / (tundraTemperatureStart10000 - borealTemperatureStart10000);
			modifier *= 3; // reach modifierMax 3 times faster (1/3 of band is the increment)
			
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
	
	

	int32 tileOwnerTown(WorldTile2 tile) final
	{
		int32 provinceId = _provinceSystem.GetProvinceIdClean(tile);
		if (provinceId == -1) {
			return -1;
		}
		return _regionSystem->provinceOwner(provinceId);
	}
	int32 tileOwnerPlayer(WorldTile2 tile) final {
		int32 townId = tileOwnerTown(tile);
		if (townId == -1) {
			return -1;
		}
		return townManager(townId).playerId();
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
		return _pathAI->isRoad(tile.x, tile.y);
	}
	//bool IsStoneRoadTile(WorldTile2 tile) final {
	//	return _overlaySystem->;
	//}

	bool IsPlayerBuildable(WorldTile2 tile, int32 playerId) final
	{
		if (!GameMap::IsInGrid(tile)) return false;
		if (tileOwnerPlayer(tile) != playerId) return false;

		return IsBuildable(tile) || IsCritterBuildingIncludeFronts(tile);
	}

	// TODO: merge with IsPlayerBuildable??
	bool IsBuildableForPlayer(WorldTile2 tile, int32 playerId) final {
		return IsBuildable(tile) &&
			(playerId == -1 || playerId == tileOwnerPlayer(tile));
	}
	bool IsFrontBuildableForPlayer(WorldTile2 tile, int32 playerId) {
		return IsFrontBuildable(tile) &&
			(playerId == -1 || playerId == tileOwnerPlayer(tile));
	}

	// Doesn't check for IsBuildable etc.
	bool IsTileBuildableForPlayer(WorldTile2 tile, int32 playerId)
	{
		if (playerId == -1) return true;
		return tileOwnerPlayer(tile) == playerId;
	}

	/*
	 * 
	 */

	CardEnum buildingEnumAtTile(WorldTile2 tile) final {
		Building* bld = buildingAtTile(tile);
		return bld ? bld->buildingEnum() : CardEnum::None;
	}


	bool IsCritterBuilding(WorldTile2 tile) {
		if (!tile.isValid()) {
			return false;
		}
		int32 bldId = buildingIdAtTile(tile);
		return IsValidBuilding(bldId) && IsCritterBuildingEnum(building(bldId).buildingEnum());
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
	int32 AddUnit(UnitEnum unitEnum, int32 townId, WorldAtom2 location, int32 ageTicks) final;
	void RemoveUnit(int id) final;
	void ResetUnitActionsInArea(TileArea area) final {
		_unitSystem->ResetUnitActionsInArea(area);
	}

	int unitCount() final { return _unitSystem->unitCount(); }
	bool unitAlive(UnitFullId fullId) final { return _unitSystem->alive(fullId); }

	const UnitEnum& unitEnum(int32 id) final { return _unitSystem->unitEnum(id); }

	const SubregionLists<int32>& unitSubregionLists() final {
		return _unitSystem->unitSubregionLists();
	}

	// Building
	Building& building(int32 id) final {
		return _buildingSystem->building(id); 
	}
	Building* buildingPtr(int32 id) final {
		if (IsValidBuilding(id)) {
			return &(_buildingSystem->building(id));
		}
		return nullptr;
	}
	bool IsValidBuilding(int32 id) final {
		bool isValid = (0 <= id && id < _buildingSystem->buildingCount()) && _buildingSystem->alive(id);
		if (!isValid) {
			//LOG_ERROR(LogNetworkInput, "IsValidBuilding: !isValid id:%d bldCount:%d alive:%d", id, _buildingSystem->buildingCount(), isValid ? _buildingSystem->alive(id) : 0);
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
	
	Building& building(ResourceHolderInfo holderInfo, int32 townId) final {
		int32_t id = resourceSystem(townId).holder(holderInfo).objectId;
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

	const std::vector<int32>& buildingIds(int32 townId, CardEnum buildingEnum) final {
		return _buildingSystem->buildingIds(townId, buildingEnum);
	}
	int32 buildingCount(int32 townId, CardEnum buildingEnum) final {
		return _buildingSystem->buildingIds(townId, buildingEnum).size();
	}
	int32 buildingFinishedCount(int32 townId, CardEnum cardEnum) final {
		const std::vector<int32>& bldIds = buildingIds(townId, cardEnum);
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

	

	int32 jobBuildingCount(int32 townId) final {
		return townManager(townId).jobBuildingCount();
	}
	
	const SubregionLists<int32>& buildingSubregionList() final {
		return _buildingSystem->buildingSubregionList();
	}

	// TODO: Optimize to use provinces?
	bool HasBuildingWithinRadius(WorldTile2 tileIn, int32 radius, int32 townId, CardEnum buildingEnum) final
	{
		const std::vector<int32>& bldIds = buildingIds(townId, buildingEnum);
		for (int32 bldId : bldIds) {
			if (building(bldId).DistanceTo(tileIn) <= radius) {
				return true;
			}
		}
		return false;
	}
	std::vector<int32> GetBuildingsWithinRadius(WorldTile2 tileIn, int32 radius, int32 townId, CardEnum buildingEnum) final
	{
		const std::vector<int32>& bldIds = buildingIds(townId, buildingEnum);
		std::vector<int32> resultIds;
		for (int32 bldId : bldIds) {
			Building& bld = building(bldId);
			if (bld.DistanceTo(tileIn) <= radius) {
				resultIds.push_back(bldId);
			}
		}
		return resultIds;
	}
	std::vector<int32> GetConstructedBuildingsWithinRadius(WorldTile2 tileIn, int32 radius, int32 townId, CardEnum buildingEnum) final
	{
		const std::vector<int32>& bldIds = buildingIds(townId, buildingEnum);
		std::vector<int32> resultIds;
		for (int32 bldId : bldIds) {
			Building& bld = building(bldId);
			if (bld.isConstructed() &&
				bld.DistanceTo(tileIn) <= radius) 
			{
				resultIds.push_back(bldId);
			}
		}
		return resultIds;
	}
	std::vector<int32> GetBuildingsWithinRadiusMultiple(WorldTile2 tileIn, int32 radius, int32 townId, std::vector<CardEnum> buildingEnums) final
	{
		std::vector<int32> resultIds;
		for (CardEnum buildingEnum : buildingEnums)
		{
			const std::vector<int32>& bldIds = buildingIds(townId, buildingEnum);
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

	//bool buildingIsAlive(int32 id, CardEnum buildingEnum) final {
	//	bool isAlive = _buildingSystem->alive(id);
	//	if (isAlive) {
	//		return _buildingSystem->buildingEnum(id) == buildingEnum;
	//	}
	//	return false;
	//}

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

	bool IsQuickBuild(int32 buildingId) final {
		return _buildingSystem->IsQuickBuild(buildingId);
	}

	static int32 StorageCostPerTile() { return 2; }

	std::vector<int32> GetConstructionResourceCost(CardEnum cardEnum, TileArea area) final
	{
		if (cardEnum == CardEnum::Farm) {
			return { area.tileCount() / 2, 0, 0 };
		}
		if (cardEnum == CardEnum::StorageYard) {
			return { area.tileCount() / 4 * StorageCostPerTile(), 0, 0 };
		}
		return GetBuildingInfo(cardEnum).constructionResources;
	}

	/*
	 * Only valid for area smaller than a region
	 */
	bool IsLandCleared_SmallOnly(int32 townId, TileArea area) final
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
		if (resourceSystem(townId).GetDropFromSmallArea_Any(area).isValid()) {
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

	void MoveUnitInstantly(int32 unitId, WorldAtom2 atom) final {
		_unitSystem->MoveUnitInstantly(unitId, atom);
	}
	
	
	std::string unitdebugStr(int id) final;
	void unitAddDebugSpeech(int32 id, std::string message) final;
	
	FString GetTileBuildingDescription(WorldTile2 tile) final;

	void SetWalkableSkipFlood(WorldTile2 tile, bool isWalkable) final {
		_pathAI->SetWalkable(tile.x, tile.y, isWalkable);
		//_pathAIHuman->SetWalkable(tile.x, tile.y, isWalkable);
	}
	void SetWalkable(WorldTile2 tile, bool isWalkable) final {
		_pathAI->SetWalkable(tile.x, tile.y, isWalkable);
		//_pathAIHuman->SetWalkable(tile.x, tile.y, isWalkable);

		_floodSystem.ResetRegionFloodDelayed(TileToRegion64Id(tile));
		//_floodSystemHuman.ResetRegionFloodDelayed(TileToRegion64Id(tile));
	}
	//void SetWalkableNonIntelligent(WorldTile2 tile, bool isWalkable) final {
	//	_pathAI->SetWalkable(tile.x, tile.y, isWalkable);
	//	_floodSystem.ResetRegionFloodDelayed(TileToRegion64Id(tile));
	//}
	PunAStar128x256* pathAI() final {
		return _pathAI.get();
	}

	void SetRoadPathAI(WorldTile2 tile, bool isRoad) final {
		//_pathAI->SetRoad(tile.x, tile.y, isRoad);
		//_pathAIHuman->SetRoad(tile.x, tile.y, isRoad); // TODO: setting road is static
		PunAStar128x256::SetRoad(tile.x, tile.y, isRoad);
	}

	int16 GetFloodId(WorldTile2 tile) final { return _floodSystem.GetFloodId(tile); }
	bool IsConnected(WorldTile2 start, WorldTile2 end, int maxRegionDistance) final {
		return _floodSystem.IsConnected(start, end, maxRegionDistance);
	}

	// Cached Connected for human
	bool IsConnectedBuilding(int32 buildingId) final
	{
		PUN_CHECK(buildingId != -1);

		int8 isConnected = _buildingSystem->IsConnectedBuilding(buildingId);
		//PUN_CHECK(isConnected != -1);
		if (isConnected == -1) {
			return false;
		}
		return static_cast<bool>(isConnected);
	}

	void OnRefreshFloodGrid(WorldRegion2 region) override {
		_buildingSystem->OnRefreshFloodGrid(region);
	}

	int32 FindNearestBuildingId(WorldTile2 tile, CardEnum buildingEnum, int32 townId, int32& minBuildingDist) override
	{
		minBuildingDist = MAX_int32;
		int32 nearestBuildingId = -1;

		const std::vector<int32>& bldIds = buildingIds(townId, buildingEnum);
		for (int32 bldId : bldIds) {
			int32 dist = WorldTile2::Distance(gateTile(bldId), tile);
			if (dist < minBuildingDist) {
				minBuildingDist = dist;
				nearestBuildingId = bldId;
			}
		}
		return nearestBuildingId;
	}
	bool FindPathWater(int32 startPortId, int32 endPortId, std::vector<WorldTile2>& resultPath) override
	{	
		Building& startPort = building(startPortId);
		Building& endPort = building(endPortId);

		// Look in the cache first
		const std::vector<int32>& cachedPortIds = startPort.cachedWaterDestinationPortIds();
		for (int32 i = 0; i < cachedPortIds.size(); i++) {
			if (cachedPortIds[i] == endPortId) {
				resultPath = startPort.cachedWaterRoute(i);
				return resultPath.size() > 0;
			}
		}
		
		WorldTile2 start = startPort.GetPortTile(); // port tile
		WorldTile2 end = endPort.GetPortTile(); // port tile

		resultPath.clear();
		
		int32 nearestDistance = MAX_int32;
		WorldTile2 nearestWaterTile;
		
		auto checkTile = [&](WorldTile2 tile)
		{
			if (pathAI()->isWater(tile.x / 4, tile.y / 4))
			{
				int32 dist = WorldTile2::Distance(start, tile) + WorldTile2::Distance(tile, end);
				if (dist < nearestDistance) {
					nearestDistance = dist;
					nearestWaterTile = tile;
				}
			}
		};
		auto checkSurroundingTiles = [&](WorldTile2 tile)
		{
			WorldTile2 localMin((tile.x / 4) * 4, (tile.y / 4) * 4);
			checkTile(localMin);
			checkTile(localMin + WorldTile2(4, 0));
			checkTile(localMin + WorldTile2(0, 4));
			checkTile(localMin + WorldTile2(4, 4));
		};

		// Find start
		nearestDistance = MAX_int32;
		checkSurroundingTiles(start);
		WorldTile2 waterStart = nearestWaterTile;

		// Find end
		nearestDistance = MAX_int32;
		checkSurroundingTiles(end);
		WorldTile2 waterEnd = nearestWaterTile;

		// Debug
		//DrawLinePath(waterStart, waterEnd, FLinearColor::Black, 1, 1000);
		
		{
			SCOPE_TIMER("FindPathShip");
			std::vector<uint32_t> rawPath;
			bool succeed = pathAI()->FindPathWater(waterStart.x / 4, waterStart.y / 4, waterEnd.x / 4, waterEnd.y / 4, rawPath, 1, 200000);
			if (succeed) {
				MapUtil::UnpackAStarPath_4x4(rawPath, resultPath);

				// Add beginning and end of path (since rawPath is the rounded path)
				resultPath.insert(resultPath.begin(), end);
				resultPath.push_back(start);

				// Cache
				if (PunSettings::IsOn("CacheWaterRoutes")) {
					startPort.AddWaterRoute(endPortId, resultPath);
				}
				return true;
			}
		}
		return false;
	}
	std::vector<int32> GetPortIds(int32 townId) final
	{
		std::vector<int32> portIds = buildingIds(townId, CardEnum::TradingPort);
		const std::vector<int32>& portIds2 = buildingIds(townId, CardEnum::IntercityLogisticsPort);
		portIds.insert(portIds.end(), portIds2.begin(), portIds2.end());
		
		return portIds;
	}
	bool FindBestPathWater(int32 startTownId, int32 endTownId, WorldTile2 startLand, int32& startPortId, int32& endPortId) final
	{
		startPortId = -1;
		endPortId = -1;

		WorldTile2 startTownGate = GetTownhallGate(startTownId);
		if (!startTownGate.isValid()) { return false; }

		// Rank port by how close it is to origin
		std::vector<int32> startPortIds = GetPortIds(startTownId);
		std::sort(startPortIds.begin(), startPortIds.end(), [&](int32 a, int32 b) {
			int32 distA = WorldTile2::Distance(building(a).centerTile(), startLand);
			int32 distB = WorldTile2::Distance(building(b).centerTile(), startLand);
			return distA < distB;
		});
		// Rank port by how close it is to another town
		std::vector<int32> endPortIds = GetPortIds(endTownId);
		std::sort(endPortIds.begin(), endPortIds.end(), [&](int32 a, int32 b) {
			int32 distA = WorldTile2::Distance(building(a).centerTile(), startTownGate);
			int32 distB = WorldTile2::Distance(building(b).centerTile(), startTownGate);
			return distA < distB;
		});

		std::vector<WorldTile2> resultPath;
		for (int32 curEndPortId : endPortIds) { // End first since 
			for (int32 curStartPortId : startPortIds)
			{
				if (FindPathWater(curStartPortId, curEndPortId, resultPath)) {
					startPortId = curStartPortId;
					endPortId = curEndPortId;
					return true;
				}
			}
		}

		return false;
	}
	bool FindBestPathWater(int32 startPortId, int32 endTownId, int32& endPortId) final
	{
		endPortId = -1;

		WorldTile2 startPortGate = building(startPortId).gateTile();
		
		// Rank port by how close it is to another town
		std::vector<int32> endPortIds = GetPortIds(endTownId);
		std::sort(endPortIds.begin(), endPortIds.end(), [&](int32 a, int32 b) {
			int32 distA = WorldTile2::Distance(building(a).centerTile(), startPortGate);
			int32 distB = WorldTile2::Distance(building(b).centerTile(), startPortGate);
			return distA < distB;
		});

		std::vector<WorldTile2> resultPath;
		for (int32 curEndPortId : endPortIds) { // End first since 
			if (FindPathWater(startPortId, curEndPortId, resultPath)) {
				endPortId = curEndPortId;
				return true;
			}
		}

		return false;
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
	WorldTile2 GetProvinceRandomTile_NoFlood(int32 provinceId, int32 tries) final {
		return _provinceSystem.GetProvinceRandomTile_NoFlood(provinceId, tries);
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

	int32 GetProvinceCountPlayer(int32 playerId) final {
		return GetProvincesPlayer(playerId).size();
	}
	std::vector<int32> GetProvincesPlayer(int32 playerId) final
	{
		const auto& townIds = playerOwned(playerId).townIds();
		std::vector<int32> provinces;
		for (int32 townId : townIds) {
			const auto& provincesClaimed = townManager(townId).provincesClaimed();
			for (int32 provinceId : provincesClaimed) {
				provinces.push_back(provinceId);
			}
		}
		return provinces;
	}
	const std::vector<int32>& GetProvincesTown(int32 townId) final {
		return townManager(townId).provincesClaimed();
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

	int32 GetProvinceClaimPrice(int32 provinceId, int32 playerId) final {
		return GetProvinceClaimPrice(provinceId, playerId, GetProvinceClaimConnectionEnumPlayer(provinceId, playerId));
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
		int32 provincePlayerId = provinceOwnerPlayer(provinceId);
		
		return BattleInfluencePrice + populationPlayer(provincePlayerId) * 30; // 33 pop = 1k influence to take over
	}
	int32 GetProvinceConquerColonyStartPrice(int32 provinceId)
	{
		int32 townId = provinceOwnerTown(provinceId);

		return BattleInfluencePrice + populationTown(townId) * 100; // 33 pop = 3.3k influence to take over
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
	FText GetProvinceVassalizeDefenseBonusTip(int32 provinceId)
	{
		return NSLOCTEXT("SimCore", "DefenseAgainstVassalizeBonus", "Defense Bonus against Vassalize Military Campaign: %100");
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

	bool CanConquerColony(int32 townId, int32 conquererPlayerId)
	{
		if (IsAIPlayer(townId)) { //TODO: this is for testing for now..
			return true;
		}
		return IsResearched(conquererPlayerId, TechEnum::Vassalize) &&
			CanConquerOtherPlayers_Base(conquererPlayerId);
	}

	void LoseVassalHelper(int32 oldLordPlayerId, int32 formerVassalPlayerId)
	{
		if (!HasTownhall(formerVassalPlayerId)) {
			return;
		}
		int32 formerVassalBuildingId = GetTownhallCapital(formerVassalPlayerId).buildingId();
		
		_playerOwnedManagers[oldLordPlayerId].LoseVassal(formerVassalBuildingId);
		_playerOwnedManagers[formerVassalPlayerId].SetLordPlayerId(-1);

		RecalculateTaxDelayedPlayer(oldLordPlayerId);
		RecalculateTaxDelayedPlayer(formerVassalPlayerId);
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
		
		int32 townId = provinceOwnerTown(provinceId);
		if (townId != -1)
		{
			// Fort
			percent += GetFortDefenseBonus_Helper(provinceId, townId);

			// Buildings
			percent += GetBuildingDefenseBonus_Helper(provinceId, townId);
		}
		return percent;
	}
	FText GetProvinceDefenseBonusTip(int32 provinceId)
	{
		TArray<FText> args;
		ADDTEXT_(
			NSLOCTEXT("SimCore", "DefenseBonus_TipTitle","Defense Bonus: {0}"), 
			TEXT_PERCENT(GetProvinceAttackCostPercent(provinceId))
		);
		int32 townId = provinceOwnerTown(provinceId);
		if (townId != -1) {
			const FText bulletText = INVTEXT("<bullet>{0} {1}%</>");
			ADDTEXT_(bulletText, NSLOCTEXT("SimCore", "fort", "fort"), TEXT_NUM(GetFortDefenseBonus_Helper(provinceId, townId)));
			ADDTEXT_(bulletText, NSLOCTEXT("SimCore", "buildings", "buildings"), TEXT_NUM(GetBuildingDefenseBonus_Helper(provinceId, townId)));
		}
		return JOINTEXT(args);
	}
	
	int32 GetFortDefenseBonus_Helper(int32 provinceId, int32 townId)
	{
		const std::vector<int32>& fortIds = buildingIds(townId, CardEnum::Fort);
		for (int32 fortId : fortIds) {
			if (building(fortId).provinceId() == provinceId) {
				return 100;
			}
		}
		return 0;
	}
	int32 GetBuildingDefenseBonus_Helper(int32 provinceId, int32 townId)
	{
		return 10 * buildingFinishedCount(townId, provinceId);
	}

	void ChangeTownOwningPlayer(int32 townId, int32 newPlayerId)
	{
		const std::vector<int32>& provinceIds = _townManagers[townId]->provincesClaimed();
		for (int32 provinceId : provinceIds) {
			SetProvinceOwner(provinceId, townId);
		}
		
		_buildingSystem->ChangeTownOwningPlayer(townId, newPlayerId);
		_townManagers[townId]->ChangeTownOwningPlayer(newPlayerId);
	}

	// NOT NEEDED YET
	//ProvinceAttackEnum GetProvinceAttackEnum(int32 provinceId)
	//{
	//	auto& provincePlayerOwner = playerOwned(provinceOwnerPlayer(provinceId));
	//	ProvinceClaimProgress claimProgress = provincePlayerOwner.GetDefendingClaimProgress(provinceId);
	//	if (claimProgress.isValid()) {
	//		return provincePlayerOwner.GetProvinceAttackEnum(provinceId, claimProgress.attackerPlayerId);
	//	}
	//	return ProvinceAttackEnum::None;
	//}
	//

	//bool HasOutpostAt(int32 playerId, int32 provinceId) final {
	//	return playerOwned(playerId).HasOutpostAt(provinceId);
	//}
	bool IsProvinceNextToTown(int32 provinceId, int32 townId) final
	{
		return _provinceSystem.ExecuteAdjacentProvincesWithExitTrue(provinceId, [&](ProvinceConnection connection) 
		{
			return connection.isConnectedTileType() &&
					provinceOwnerTown(connection.provinceId) == townId;
		});
	}
	bool IsProvinceNextToTownByShallowWater(int32 provinceId, int32 townId) final {
		if (!unlockSystem(townPlayerId(townId))->IsResearched(TechEnum::ShallowWaterEmbark)) {
			return false;
		}
		return _provinceSystem.ExecuteAdjacentProvincesWithExitTrue(provinceId, [&](ProvinceConnection connection)
		{
			bool isValidConnectionType = (connection.tileType == TerrainTileType::Ocean);

			return isValidConnectionType &&
				provinceOwnerTown(connection.provinceId) == townId;
		});
	}
	//bool IsProvinceOverseaClaimableByPlayer(int32 provinceId, int32 playerId, int32 townId)
	//{
	//	// TODO: have military harbor that do flood check..
	//	return unlockSystem(playerId)->IsResearched(TechEnum::DeepWaterEmbark) &&
	//		_provinceSystem.provinceIsCoastal(provinceId);
	//}

	ClaimConnectionEnum GetProvinceClaimConnectionEnumPlayer(int32 provinceId, int32 playerId)
	{
		const auto& townIds = GetTownIds(playerId);
		
		for (int32 townId : townIds) {
			if (IsProvinceNextToTown(provinceId, townId)) {
				return ClaimConnectionEnum::Flat;
			}
		}
		for (int32 townId : townIds) {
			if (IsProvinceNextToTownByShallowWater(provinceId, townId)) {
				return ClaimConnectionEnum::ShallowWater;
			}
		}
		//if (IsProvinceOverseaClaimableByPlayer(provinceId, playerId, -1)) {
		//	return ClaimConnectionEnum::Deepwater;
		//}
		return ClaimConnectionEnum::None;
	}
	// TODO: IS this needed?
	ClaimConnectionEnum GetProvinceClaimConnectionEnumTown(int32 provinceId, int32 townId)
	{
		if (IsProvinceNextToTown(provinceId, townId)) {
			return ClaimConnectionEnum::Flat;
		}
		if (IsProvinceNextToTownByShallowWater(provinceId, townId)) {
			return ClaimConnectionEnum::ShallowWater;
		}
		return ClaimConnectionEnum::None;
	}
	
	bool IsOverseaProvince(int32 provinceIdIn, int32 playerId)
	{
		if (!HasTownhall(playerId)) {
			return false;
		}
		
		int32 townhallProvinceId = GetTownhallCapital(playerId).provinceId();
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
			return provinceOwnerPlayer(connection.provinceId) == playerId;
		});
	}
	

	void RefreshTerritoryEdge(int32 townId) final {
		return _provinceSystem.RefreshTerritoryEdge(townId, GetProvincesTown(townId));
	}

	bool IsBorderProvince(int32 provinceId) final
	{
		int32 playerId = provinceOwnerPlayer(provinceId);
		check(playerId != -1);
		const std::vector<ProvinceConnection>& connections = _provinceSystem.GetProvinceConnections(provinceId);
		for (const ProvinceConnection& connection : connections)
		{
			if (connection.tileType == TerrainTileType::None) {
				if (provinceOwnerPlayer(connection.provinceId) != playerId) {
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
		//if (canPassGate) {
		//	return _floodSystemHuman.TryAccessNonWalkableTile(start, nonwalkableTile, maxRegionDistance);
		//}
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

	void PlayerAddHouse(int32 townId, int objectId) override { _townManagers[townId]->PlayerAddHouse(objectId); }
	void PlayerRemoveHouse(int32 townId, int objectId) override { _townManagers[townId]->PlayerRemoveHouse(objectId); }
	void PlayerAddJobBuilding(int32 townId, Building& building, bool isConstructed) override { _townManagers[townId]->PlayerAddJobBuilding(building, isConstructed); }
	void PlayerRemoveJobBuilding(int32 townId, Building& building, bool isConstructed) override { _townManagers[townId]->PlayerRemoveJobBuilding(building, isConstructed); }
	void RefreshJobDelayed(int32 townId) override {
		_townManagers[townId]->RefreshJobDelayed();
	}
	
	bool IsInDarkAge(int32 playerId) final { return _playerOwnedManagers[playerId].IsInDarkAge(); }
	
	void RecalculateTaxDelayedPlayer(int32 playerId) override {
		const auto& townIds = _playerOwnedManagers[playerId].townIds();
		for (int32 townId : townIds) {
			_townManagers[townId]->RecalculateTaxDelayed();
		}
	}
	void RecalculateTaxDelayedTown(int32 townId) override {
		PUN_ENSURE(townId >= 0, return);
		PUN_ENSURE(townId < _townManagers.size(), return);
		_townManagers[townId]->RecalculateTaxDelayed();
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

	int32 HousingCapacity(int32 playerId) override { return _townManagers[playerId]->housingCapacity(); }

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

	void SetProvinceOwner(int32 provinceId, int32 townId, bool lightMode = false);
	void SetProvinceOwnerFull(int32 provinceId, int32 townId) final;

	void SetProvinceOwner_Popup(int32 provinceId, int32 attackerPlayerId, bool isFull);

	int32 provinceOwnerTown(int32 provinceId) final { return _regionSystem->provinceOwner(provinceId); }
	int32 provinceOwnerPlayer(int32 provinceId) final {
		int32 townId = provinceOwnerTown(provinceId);
		if (townId == -1) {
			return -1;
		}
		return townManager(townId).playerId();
	}

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
	void AddPopup(PopupInfo popupInfo) final {
		PUN_CHECK(0 <= popupInfo.playerId);
		PUN_CHECK(popupInfo.playerId < _popupSystems.size());
		_popupSystems[popupInfo.playerId].AddPopup(popupInfo);
	}
	void AddPopup(int32 playerId, FText popupBody, std::string popupSound = "") final {
		PopupInfo info(playerId, popupBody);
		info.popupSound = popupSound;
		AddPopup(info);
	}
	void AddPopup(int32 playerId, TArray<FText> popupBody, std::string popupSound = "") final {
		PopupInfo info(playerId, JOINTEXT(popupBody));
		info.popupSound = popupSound;
		AddPopup(info);
	}

	
	void AddPopupToFront(PopupInfo popupInfo) final {
		_popupSystems[popupInfo.playerId].AddPopupToFront(popupInfo);
	}
	void AddPopupToFront(int32 playerId, FText popupBody) final {
		AddPopupToFront(PopupInfo(playerId, popupBody));
	}
	void AddPopupToFront(int32 playerId, FText popupBody, ExclusiveUIEnum exclusiveEnum, std::string popupSound) final {
		PopupInfo popup(playerId, { popupBody }, popupSound);
		popup.warningForExclusiveUI = exclusiveEnum;
		AddPopupToFront(popup);
	}

	void AddPopupAll(PopupInfo popupInfo, int32 playerToSkip) final {
		PUN_LOG("AddPopupToDisplayAll %s", *(popupInfo.body.ToString()));
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
	void AddEventLog(int32 playerId, FText eventMessage, bool isImportant) final {
		_eventLogSystem.AddEventLog(playerId, eventMessage, isImportant);
	}
	void AddEventLogToAllExcept(int32 playerIdException, FText eventMessage, bool isImportant) final {
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
	int32 GetScience100PerRound(int32 playerId) final {
		const auto& townIds = playerOwned(playerId).townIds();
		int32 result = 0;
		for (int32 townId : townIds) {
			result += townManager(townId).science100PerRound();
		}
		return result;
	}

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
	// NOTE!!! Y is up
	void DrawLine(WorldTile2 tile, FVector startShift, WorldTile2 endTile, FVector endShift, FLinearColor Color,
					float Thickness = 1.0f, float LifeTime = 10000) final {
		DrawLine(tile.worldAtom2(), startShift, endTile.worldAtom2(), endShift, Color, Thickness, LifeTime);
	}
	void DrawLine(WorldTile2 tile, FLinearColor Color, float Thickness = 1.0f, float LifeTime = 10000) final
	{
		DrawLine(tile, FVector::ZeroVector, tile, FVector(0, 10, 10), Color, Thickness, LifeTime);
	}
	void DrawLinePath(WorldTile2 start, WorldTile2 end, FLinearColor Color, float Thickness = 1.0f, float LifeTime = 10000) final
	{
		DrawLine(start, FVector::ZeroVector, start, FVector(0, 0, 10), FLinearColor::Green, Thickness, LifeTime);
		DrawLine(start, FVector(0, 0, 10), end, FVector(0, 0, 10), Color, Thickness, LifeTime);
		DrawLine(end, FVector::ZeroVector, end, FVector(0, 0, 10), FLinearColor::Red, Thickness, LifeTime);
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
	int32 GetAverageHappiness(int32 townId) final {
		return townManager(townId).aveOverallHappiness();
	}
	int32 GetAverageHappinessPlayer(int32 playerId) {
		int32 totalPopulation = 0;
		int32 totalHappiness = 0;
		const auto& townIds = _playerOwnedManagers[playerId].townIds();
		for (int32 townId : townIds) {
			totalPopulation += _townManagers[townId]->population();
			totalHappiness += _townManagers[townId]->aveOverallHappiness();
		}
		return totalHappiness / totalPopulation;
	}
	
	int32 GetAverageHappinessByType(int32 townId, HappinessEnum happinessEnum) final {
		return townManager(townId).aveHappinessByType(happinessEnum);
	}
	int32 taxHappinessModifier(int32 townId) final { return townManager(townId).taxHappinessModifier(); }
	int32 citizenDeathHappinessModifier(int32 townId, SeasonStatEnum statEnum) final {
		SubStatSystem& statSystem = _statSystem.townStatSystem(townId);
		int32 deaths = statSystem.GetYearlyStat(statEnum);
		// unhappiness from deaths depends on the ratio (death):(pop+death)
		// unhappiness from death cap at 50, where (death):(pop+death) is 1:1
		int32 happinessModifier = -51 * deaths / (deaths + populationTown(townId) + 1);

		PUN_CHECK(happinessModifier < 1000 && happinessModifier > -1000);
		return happinessModifier;
	}

	void AddMigrationPendingCount(int32 townId, int32 migrationCount) final
	{
		townManager(townId).AddMigrationPendingCount(migrationCount);
	}

	void ImmigrationEvent(int32 townId, int32 migrationCount, FText message, PopupReceiverEnum receiverEnum) final
	{
		TownHall* townhall = GetTownhallPtr(townId);
		if (townhall) {
			townhall->ImmigrationEvent(migrationCount, message, receiverEnum);
		}
	}
	
	/*
	 * Cards
	 */

	bool CanBuyCard(int32 playerId, CardEnum buildingEnum)
	{
		// Not enough money
		int32 money = globalResourceSystem(playerId).money();
		if (money < cardSystem(playerId).GetCardPrice(buildingEnum)) {
			AddPopupToFront(playerId, 
				NSLOCTEXT("SimCore", "NotEnoughMoneyPurchaseCard", "Not enough money to purchase the card."), 
				ExclusiveUIEnum::CardHand1, "PopupCannot"
			);
			return false;
		}

		if (!cardSystem(playerId).CanAddCardToBoughtHand(buildingEnum, 1)) {
			AddPopupToFront(playerId, 
				NSLOCTEXT("SimCore", "ReachedHandLimit_Pop", "Reached hand limit for bought cards."),
				ExclusiveUIEnum::CardHand1, "PopupCannot"
			);
			return false;
		}
		return true;
	}
	int32 BoughtCardCount(int32 playerId, CardEnum buildingEnum) final {
		return cardSystem(playerId).BoughtCardCount(buildingEnum);
	}
	int32 TownhallCardCountTown(int32 townId, CardEnum cardEnum) final {
		return townManager(townId).TownhallCardCount(cardEnum);
	}
	int32 TownhallCardCountAll(int32 playerId, CardEnum cardEnum) final {
		const auto& townIds = GetTownIds(playerId);
		int32 count = 0;
		for (int32 townId : townIds) {
			count += townManager(townId).TownhallCardCount(cardEnum);
		}
		return count;
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

	bool isStorageAllFull(int32 townId) final
	{
		std::vector<int32> storageIds = buildingIds(townId, CardEnum::StorageYard);

		// Add warehouses
		std::vector<int32> warehouseIds = buildingIds(townId, CardEnum::Warehouse);
		storageIds.insert(storageIds.end(), warehouseIds.begin(), warehouseIds.end());

		// Add markets
		std::vector<int32> marketIds = buildingIds(townId, CardEnum::Market);
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

	//std::vector<int32> GetCapitalArmyCounts(int32 playerId, bool skipWall = false) final
	//{
	//	ArmyGroup* group = townhall(playerId).armyNode.GetArmyGroup(playerId);
	//	if (group) {
	//		return group->GetArmyCounts(skipWall);
	//	}
	//	return std::vector<int32>(ArmyEnumCount, 0);
	//}

	//std::vector<int32> GetTotalArmyCounts(int32 playerId, bool skipWall = false) final
	//{
	//	std::vector<int32> nodeIdsVisited = playerOwned(playerId).nodeIdsVisited();
	//	std::vector<int32> armyCounts(ArmyEnumCount, 0);
	//	
	//	for (int32 nodeId : nodeIdsVisited) {
	//		CardEnum nodeEnum = building(nodeId).buildingEnum();
	//		if (nodeEnum == CardEnum::Townhall) {
	//			building<TownHall>(nodeId).armyNode.ExecuteOnAllGroups([&](const ArmyGroup& group) 	{
	//				if (group.playerId == playerId) {
	//					group.AddArmyToGivenArray(armyCounts, skipWall);
	//				}
	//			}, true);
	//		}
	//	}
	//	return armyCounts;
	//}

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
			AddPopup(playerIdToWin, 
				NSLOCTEXT("SimCore", "WarnDominationVictory_Pop", "You have captured all but one capital. If you captured all capitals, you will achieve the domination victory.")
			);
			AddPopupAll(PopupInfo(-1,
				FText::Format(
					NSLOCTEXT("SimCore", "WarnDominationVictoryAll_Pop", "{0} have captured all but one capital. You will be defeated if {0} manages to capture all the capitals."),
					playerNameT(playerIdToWin)
				)
			), playerIdToWin);
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
	void ExecuteTimeVictory() {
		_endStatus.victoriousPlayerId = 0;
		_endStatus.gameEndEnum = GameEndEnum::TimeVictory;
	}

	int32 populationScore(int32 playerId) { return populationPlayer(playerId); }
	int32 happinessScore(int32 playerId) { return GetAverageHappinessPlayer(playerId) * 3; }
	int32 moneyScore(int32 playerId) { return money(playerId) / 10000; }
	int32 technologyScore(int32 playerId) { return unlockSystem(playerId)->techsFinished * 10; }
	int32 wonderScore(int32 playerId) { return 0; }
	int32 totalScore(int32 playerId) {
		return populationScore(playerId) + 
			happinessScore(playerId) + 
			moneyScore(playerId) + 
			technologyScore(playerId) + 
			wonderScore(playerId);
	}

	bool HasTownhall(int32 playerId) final {
		return playerOwned(playerId).hasCapitalTownhall();
	}
	bool HasChosenLocation(int32 playerId) final {
		return playerOwned(playerId).hasChosenLocation();
	}
	
	int32 homeProvinceId(int32 playerId) final
	{
		if (!HasTownhall(playerId)) {
			return -1;
		}
		return GetTownhallCapital(playerId).provinceId();
	}

	int32 TownAge(int32 playerId) {
		if (!HasTownhall(playerId)) {
			return 0;
		}
		return GetTownhallCapital(playerId).buildingAge();
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
	

	// Used to pause the game until the last player chose location
	bool AllPlayerHasTownhallAfterInitialTicks() final
	{
		// Below 2 sec, don't pause
		if (Time::Ticks() < Time::TicksPerSecond * 2) {
			return true;
		}

		std::vector<int32> connectedPlayerIds = _gameManager->connectedPlayerIds(false);
		for (int32 playerId : connectedPlayerIds) {
			if (!playerOwned(playerId).hasCapitalTownhall() &&
				!IsReplayPlayer(playerId))  // ReplayPlayer shouldn't pause the game
			{
				return false;
			}
		};
		return true;
	}

	void GenerateRareCardSelection(int32 playerId, RareHandEnum rareHandEnum, FText rareHandMessage) final
	{
		return cardSystem(playerId).RollRareHand(rareHandEnum, rareHandMessage);
	}

	void CheckGetSeedCard(int32 playerId) final
	{
		auto checkGetSeed = [&](CardEnum seedCardEnum, GeoresourceEnum georesourceEnum)
		{
			if (!globalResourceSystem(playerId).HasSeed(seedCardEnum) &&
				!cardSystem(playerId).HasBoughtCard(seedCardEnum))
			{
				if (unlockSystem(playerId)->IsResearched(TechEnum::Plantation))
				{
					std::vector<int32> provincesClaimed = GetProvincesPlayer(playerId);
					for (int32 provinceId : provincesClaimed)
					{
						if (georesourceSystem().georesourceNode(provinceId).georesourceEnum == georesourceEnum)
						{
							AddPopup(playerId, 
								FText::Format(NSLOCTEXT("SimCore", "Found Seed", "You found {0}!"), GetBuildingInfo(seedCardEnum).name)
							);
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

	void PopupInstantReply(int32 playerId, PopupReceiverEnum replyReceiver, int32 choiceIndex);


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

		//if (checkChunkEnum(GameSaveChunkEnum::Debug))
		//{}

		//if (checkChunkEnum(GameSaveChunkEnum::Debug2))
		//{}
		
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

			// Per Town
			{
				//SERIALIZE_TIMER("PlayerSys", data, crcs, crcLabels);

				int32 townCountAr;
				if (Ar.IsSaving()) {
					townCountAr = townCount();
				}
				Ar << townCountAr;

				if (Ar.IsLoading()) {
					// Ensure same town count as before
					for (int32 townId = _resourceSystems.size(); townId < townCountAr; townId++) {
						_resourceSystems.push_back(ResourceSystem(townId, this));
						_townManagers.push_back(make_unique<TownManager>(-1, townId, this));
					}
				}

#define LOOP(sysName, func) {\
								SERIALIZE_TIMER(sysName, data, crcs, crcLabels)\
								for (size_t i = 0; i < townCountAr; i++) { func; }\
							}

				LOOP("Resource", _resourceSystems[i].Serialize(Ar));
				LOOP("TownManager", _townManagers[i]->Serialize(Ar));
#undef LOOP
			}


			//! Per Player
			{
#define LOOP(sysName, func) {\
								SERIALIZE_TIMER(sysName, data, crcs, crcLabels)\
								for (size_t i = 0; i < GameConstants::MaxPlayersAndAI; i++) { func; }\
							}

				LOOP("PlayerOwned", _playerOwnedManagers[i].Serialize(Ar));
				LOOP("GlobalResource", _globalResourceSystems[i].Serialize(Ar));
				LOOP("Quest", _questSystems[i].Serialize(Ar));

				LOOP("Unlock", _unlockSystems[i].Serialize(Ar));
				LOOP("PlayerParam", _playerParameters[i].Serialize(Ar));
				LOOP("Popup", _popupSystems[i].Serialize(Ar));
				LOOP("Cards", _cardSystem[i].Serialize(Ar));

				LOOP("AI", _aiPlayerSystem[i].Serialize(Ar));
				LOOP("NonRepeatAction", SerializeVecValue(Ar, _playerIdToNonRepeatActionToAvailableTick[i]));

#undef LOOP
			}

			//SerializeVecValue(Ar, _tickHashes);
#if WITH_EDITOR
			_tickHashes >> Ar;
#endif
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
		GameRand::SetRandUsageValid(true);
		
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

		GameRand::SetRandUsageValid(false);
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
				_playerOwnedManagers[playerId()].capitalTownhallId = townhallId;
				_playerOwnedManagers[playerId()].justChoseLocation = true;
				_playerOwnedManagers[playerId()].needChooseLocation = false;

				break;
			}
		}

		ChangeMoney(playerId(), 1000000);

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

	// NetworkCommand Helper
	std::vector<int32> GetProvinceAttackerTownIds(int32 attackerPlayerId, int32 provinceId)
	{
		// Get town with adjacent province
		const auto& connections = GetProvinceConnections(provinceId);

		// Get the town to attack with (claim/conquer province)
		const auto& attackerTownIds = GetTownIds(attackerPlayerId);
		
		std::vector<int32> adjacentAttackerTownIds;
		for (const ProvinceConnection& connection : connections) {
			int32 adjacentOwnerTownId = provinceOwnerTown(connection.provinceId);
			
			if (adjacentOwnerTownId != -1 && 
				CppUtils::Contains(attackerTownIds, adjacentOwnerTownId)) 
			{
				CppUtils::TryAdd(adjacentAttackerTownIds, adjacentOwnerTownId);
			}
		}

		if (adjacentAttackerTownIds.size() > 3) {
			adjacentAttackerTownIds.resize(3);
		}

		return adjacentAttackerTownIds;
	}
	
	void CheckPortArea(TileArea area, Direction faceDirection, CardEnum buildingEnum, std::vector<PlacementGridInfo>& grids, 
						bool& setDockInstruction, int32 playerId = -1) // player == -1 means no player check
	{
		auto extraInfoPair = DockPlacementExtraInfo(buildingEnum);
		int32 indexLandEnd = extraInfoPair.first;
		int32 minWaterCount = extraInfoPair.second;

		// Need to face water with overlapping at least 5 water tiles 
		int32 waterCount = 0;

		area.ExecuteOnArea_WorldTile2([&](WorldTile2 tile) {
			if (GameMap::IsInGrid(tile.x, tile.y) && IsTileBuildableForPlayer(tile, playerId))
			{
				int steps = GameMap::GetFacingStep(faceDirection, area, tile);
				if (steps <= indexLandEnd) { // 0,1
					bool isGreen = IsPlayerBuildable(tile, playerId);
					grids.push_back({ isGreen ? PlacementGridEnum::Green : PlacementGridEnum::Red, tile });
				}
				else {
					if (IsWater(tile)) {
						waterCount++;
					}
				}
			}
		});

		// When there isn't enough water tiles, make the part facing water red...
		// Otherwise make it green on water, and gray on non-water
		area.ExecuteOnArea_WorldTile2([&](WorldTile2 tile) {
			int steps = GameMap::GetFacingStep(faceDirection, area, tile);
			if (steps > indexLandEnd) {
				if (waterCount < minWaterCount) {
					grids.push_back({ PlacementGridEnum::Red, tile });

					setDockInstruction = true;
					//SetInstruction(PlacementInstructionEnum::Dock, true);
				}
				else {
					if (tileHasBuilding(tile)) { // Maybe bridge
						grids.push_back({ PlacementGridEnum::Red, tile });
					}
					else {
						bool isGreen = IsWater(tile);
						grids.push_back({ isGreen ? PlacementGridEnum::Green : PlacementGridEnum::Gray, tile });
					}
				}
			}
		});
	}

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
	//GameMapFlood _floodSystemHuman;  // TODO: Remove once save is ok
	ProvinceSystem _provinceSystem;
	//GameMapFloodShortDistance _floodSystem2;
	//GameMapFloodShortDistance _floodSystem2Human;
	
	std::unique_ptr<PunAStar128x256> _pathAI;
	//std::unique_ptr<PunAStar128x256> _pathAIHuman;
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

	// Per Town
	std::vector<ResourceSystem> _resourceSystems;
	std::vector<std::unique_ptr<TownManager>> _townManagers;

	// Per Player
	std::vector<PlayerOwnedManager> _playerOwnedManagers;
	std::vector<GlobalResourceSystem> _globalResourceSystems;
	std::vector<UnlockSystem> _unlockSystems;
	std::vector<QuestSystem> _questSystems;
	std::vector<PlayerParameters> _playerParameters;
	std::vector<PopupSystem> _popupSystems;
	std::vector<BuildingCardSystem> _cardSystem;

	std::vector<AIPlayerSystem> _aiPlayerSystem;
	
	std::vector<std::vector<int32>> _playerIdToNonRepeatActionToAvailableTick;

	// TODO: Remove
	// int32 _playerCount = 0;
	// std::vector<FString> _playerNames;

	FMapSettings _mapSettings;
	FGameEndStatus _endStatus;
	
	TickHashes _tickHashes;
	TickHashes _serverTickHashes;

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

