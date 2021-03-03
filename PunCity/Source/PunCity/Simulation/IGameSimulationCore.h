#pragma once

#include "CoreMinimal.h"
#include "GameSimulationInfo.h"
#include "PunCity/PunContainers.h"
#include "GameEventSystem.h"
#include "PunCity/NetworkStructs.h"


enum class QuestEnum : unsigned char;

class IGameSoundInterface
{
public:
	// Audio
	virtual void SpawnResourceDropoffAudio(ResourceEnum resourceEnum, WorldAtom2 worldAtom) = 0;
	virtual void SpawnAnimalSound(UnitEnum unitEnum, bool isAngry, WorldAtom2 worldAtom, bool usePlayProbability = true) = 0;
	virtual void Spawn3DSound(std::string groupName, std::string soundName, WorldAtom2 worldAtom, float height = 3.0f) = 0;
	virtual void Spawn2DSound(std::string groupName, std::string soundName, int32 playerId, WorldTile2 tile = WorldTile2::Invalid) = 0;
	virtual void Spawn2DSoundAllPlayers(std::string groupName, std::string soundName, WorldTile2 tile) = 0;
	//virtual void Spawn2DSound(std::string groupName, std::string soundName) = 0;

	//virtual void SpawnBuildingWorkSound(class Building& building) = 0;
	virtual void TryStartBuildingWorkSound(class Building& building) = 0;
	virtual void TryStopBuildingWorkSound(class Building& building) = 0;


	virtual float GetDisplayTime() = 0;
	virtual float GetTrailerTime() = 0;
};

class IGameUIInterface
{
public:
	virtual void ShowFloatupInfo(FloatupInfo floatupInfo) = 0;
	virtual void ShowFloatupInfo(FloatupEnum floatupEnum, WorldTile2 tile, FText text, ResourceEnum resourceEnum = ResourceEnum::None, FText text2 = FText()) = 0;
	
	virtual void PunLog(std::string text) = 0;

	virtual void OpenTradeUI(int32 objectId) = 0;
	virtual void OpenIntercityTradeUI(int32 objectId) = 0;

	virtual void SetLoadingText(FText message) = 0;

	virtual void AutosaveGame() = 0;
};

// TODO: move this???
class IPlayerSoundSettings
{
public:
	virtual float masterVolume() = 0;
	virtual float musicVolume() = 0;
	virtual float ambientVolume() = 0;
	virtual float soundEffectsVolume() = 0;
};

/**
 * Interface for simulation systems to communicate with each other
 */
class IGameSimulationCore
{
public:
	virtual ~IGameSimulationCore() {}

	virtual bool isLoadingFromFile() = 0;
	virtual bool isLoadingForMainMenu() = 0;
	virtual int32 playerId() = 0;
	
	virtual class TreeSystem& treeSystem() = 0;
	virtual class OverlaySystem& overlaySystem() = 0;
	virtual class GeoresourceSystem& georesourceSystem() = 0;
	virtual class BuildingSystem& buildingSystem() = 0;
	virtual class UnitSystem& unitSystem() = 0;
	virtual class StatSystem& statSystem() = 0;
	virtual class PunTerrainGenerator& terrainGenerator() = 0;
	virtual class GameMapFlood& floodSystem() = 0;
	//virtual class GameMapFlood& floodSystemHuman() = 0;
	virtual class ProvinceSystem& provinceSystem() = 0;
	virtual class GameRegionSystem& regionSystem() = 0;
	
	virtual class ResourceDropSystem& dropSystem() = 0;
	//virtual GameEventSource& eventSource(EventSourceEnum eventEnum) = 0;

	virtual class WorldTradeSystem& worldTradeSystem() = 0;
	virtual class PunTerrainChanges& terrainChanges() = 0;

	virtual class TownManager& townManager(int32 townId) = 0;
	virtual class ResourceSystem& resourceSystem(int32 townId) = 0;

	virtual class GlobalResourceSystem& globalResourceSystem(int32 playerId) = 0;
	virtual class IQuestSystem* iquestSystem(int32 playerId) = 0;
	virtual class QuestSystem* questSystem(int32 playerId) = 0;
	virtual class UnlockSystem* unlockSystem(int32 playerId) = 0;
	virtual class PlayerParameters* parameters(int32 playerId) = 0;
	virtual class PlayerOwnedManager& playerOwned(int32 playerId) = 0;
	virtual class PlayerOwnedManager& playerOwnedFromTownId(int32 townId) = 0;
	virtual class SubStatSystem& statSystem(int32 townId) = 0;

	virtual class DebugLineSystem& debugLineSystem() = 0;

	virtual class BuildingCardSystem& cardSystem(int32 playerId) = 0;

	virtual class IGameSoundInterface* soundInterface() = 0;
	virtual class IGameUIInterface* uiInterface() = 0;
	virtual class IGameManagerInterface* gameManagerInterface() = 0;

	virtual void ExecuteNetworkCommands(std::vector<std::shared_ptr<FNetworkCommand>>& commands) = 0;
	virtual bool ExecuteNetworkCommand(std::shared_ptr<FNetworkCommand> command) = 0;
	
	//virtual class IUnitDataSource& unitDataSource() = 0;

	virtual int32 playerCount() = 0;
	virtual int32 townCount() = 0;
	virtual const std::vector<int32>& GetTownIds(int32 playerId) = 0;
	virtual int32 townPlayerId(int32 townId) = 0;
	virtual int32 buildingTownId(int32 buildingId) = 0;
	virtual int32 GetNextTown(bool forward, int32 currentTownId, int32 playerId) = 0;
	
	virtual bool IsTownOwnedByPlayer(int32 townIdIn, int32 playerId) = 0;
	virtual bool IsTownhallOverlapProvince(int32 provinceId, int32 provincePlayerId) = 0;

	virtual FString playerNameF(int32 playerId) = 0;
	virtual FText playerNameT(int32 playerId) = 0;
	virtual std::string playerName(int32 playerId) = 0;
	virtual std::vector<int32> allHumanPlayerIds() = 0;
	virtual std::vector<int32> connectedPlayerIds() = 0;

	virtual MapSizeEnum mapSizeEnum() = 0;
	virtual int32 difficultyConsumptionAdjustment(int32 playerId) = 0;
	virtual FMapSettings mapSettings() = 0;

	virtual bool AllPlayerHasTownhallAfterInitialTicks() = 0;

	// Unit
	virtual void ResetUnitActions(int id, int32 waitTicks = 1) = 0;
	virtual int32 AddUnit(UnitEnum unitEnum, int32 townId, WorldAtom2 location, int32_t ageTicks) = 0;
	virtual void RemoveUnit(int id) = 0;
	virtual void ResetUnitActionsInArea(TileArea area) = 0;
	virtual int unitCount() = 0;
	virtual bool unitAlive(UnitFullId fullId) = 0;

	virtual void AddImmigrants(int32 townId, int32 count, WorldTile2 tile = WorldTile2::Invalid) = 0;

	// Building
	virtual class TownHall& GetTownhallCapital(int32 playerId) = 0;
	virtual class TownHall& GetTownhall(int32 townId) = 0;
	virtual class TownHall* GetTownhallPtr(int32 townId) = 0;
	virtual int32 GetTownhallId(int32 townId) = 0;
	virtual bool IsValidTown(int32 townId) = 0;
	
	virtual int32 GetTownLvl(int32 townId) = 0; // Needed because Townhall. might not be included all the time
	virtual int32 GetTownLvlMax(int32 playerId) = 0;
	
	virtual WorldTile2 GetTownhallGateCapital(int32 playerId) = 0;
	virtual WorldTile2 GetTownhallGateFast(int32 townId) = 0;
	virtual WorldTile2 GetTownhallGate(int32 townId) = 0;
	virtual FText townNameT(int32 townId) = 0;

	virtual int32 GetTownAgeTicks(int32 townId) = 0;

	virtual bool IsConnectedToTowns(WorldTile2 tile, int32 playerId, std::vector<uint32>& path) = 0;

	bool unlockedInfluence(int32 playerId) {
		return IsResearched(playerId, TechEnum::InfluencePoints);
	}

	virtual class Building& building(int32 id) = 0;
	virtual class Building& buildingChecked(int32 id) = 0;
	virtual class Building& building(ResourceHolderInfo holderInfo, int32 townId) = 0;
	virtual CardEnum buildingEnum(int32 id) = 0;
	virtual bool isValidBuildingId(int32 id) = 0;

	virtual WorldTile2 buildingCenter(int32 id) = 0;
	
	virtual class Building* buildingAtTile(WorldTile2 tile) = 0;
	virtual CardEnum buildingEnumAtTile(WorldTile2 tile) = 0;
	virtual int32 buildingIdAtTile(WorldTile2 tile) = 0;
	virtual bool tileHasBuilding(WorldTile2 tile) = 0;

	virtual bool IsValidBuilding(int32 id) = 0;

	virtual std::vector<int32> frontBuildingIds(WorldTile2 tile) = 0;

	template<class T>
	T& building(int id, CardEnum buildingEnum) {
		return building(id).subclass<T>(buildingEnum);
	}
	template<class T>
	T& building(int id) {
		return building(id).subclass<T>();
	}

	virtual bool IsPermanentBuilding(int32 playerId, CardEnum cardEnum) = 0;

	virtual void RemoveBuilding(int32 buildingId) = 0;

	virtual const std::vector<int32>& buildingIds(int32 townId, CardEnum buildingEnum) = 0;
	virtual int32 buildingCount(int32 townId, CardEnum buildingEnum) = 0;
	virtual int32 buildingFinishedCount(int32 townId, CardEnum cardEnum) = 0;

	virtual int32 jobBuildingCount(int32 townId) = 0;
	
	virtual const SubregionLists<int32>& buildingSubregionList() = 0;

	virtual bool HasBuildingWithinRadius(WorldTile2 tileIn, int32 radius, int32 townId, CardEnum buildingEnum) = 0;
	virtual std::vector<int32> GetBuildingsWithinRadius(WorldTile2 tileIn, int32 radius, int32 townId, CardEnum buildingEnum) = 0;
	virtual std::vector<int32> GetBuildingsWithinRadiusMultiple(WorldTile2 tileIn, int32 radius, int32 townId, std::vector<CardEnum> buildingEnums) = 0;
	
	template<typename Func>
	std::vector<int32> buildingIdsFiltered(int32 playerId, CardEnum cardEnum, Func shouldRemove) {
		std::vector<int32> results = buildingIds(playerId, cardEnum);
		for (int32 i = results.size(); i-- > 0;) {
			Building& bld = building(results[i]);
			if (shouldRemove(bld)) {
				results.erase(results.begin() + i);
			}
		}
		return results;
	}
	template<typename Func>
	bool hasBuildingWithCondition(int32 playerId, CardEnum cardEnum, Func condition) {
		std::vector<int32> bldIds = buildingIds(playerId, cardEnum);
		for (int32 i = bldIds.size(); i-- > 0;) {
			Building& bld = building(bldIds[i]);
			if (condition(bld)) {
				return true;
			}
		}
		return false;
	}

	virtual bool buildingIsAlive(int32_t id) = 0;
	virtual WorldTile2 gateTile(int id) = 0;
	virtual void AddTickBuilding(int32 buildingId) = 0;
	virtual void ScheduleTickBuilding(int32 buildingId, int32 scheduleTick) = 0;
	virtual void RemoveScheduleTickBuilding(int32 buildingId) = 0;

	virtual std::vector<int32> GetConstructionResourceCost(CardEnum cardEnum, TileArea area) = 0;
	
	virtual bool IsLandCleared_SmallOnly(int32 townId, TileArea area) = 0;

	// Military
	virtual int32 lordPlayerId(int32 playerId) = 0;
	//virtual std::vector<int32> GetArmyNodeIds(int32 playerId) = 0;
	//virtual class ArmyNode& GetArmyNode(int32 buildingId) = 0;

	//virtual std::vector<int32> GetCapitalArmyCounts(int32 playerId, bool skipWall = false) = 0;
	//virtual std::vector<int32> GetTotalArmyCounts(int32 playerId, bool skipWall = false) = 0;

	// Display
	virtual void SetNeedDisplayUpdate(DisplayClusterEnum displayEnum, int32 regionId, bool needUpdate = true) = 0;
	virtual void SetNeedDisplayUpdate(DisplayClusterEnum displayEnum, TileArea area, bool isSmallArea, bool needUpdate = true) = 0;
	virtual bool NeedDisplayUpdate(DisplayClusterEnum displayEnum, int32 regionId) = 0;
	virtual void SetNeedDisplayUpdate(DisplayGlobalEnum displayEnum, bool needUpdate = true) = 0;
	virtual bool NeedDisplayUpdate(DisplayGlobalEnum displayEnum) = 0;
	virtual void AddNeedDisplayUpdateId(DisplayGlobalEnum displayEnum, int32 id, bool updateBeforeInitialized = false) = 0;
	virtual std::vector<int32> GetNeedDisplayUpdateIds(DisplayGlobalEnum displayEnum) = 0;

	virtual class UnitStateAI& unitAI(int32 id) = 0;
	virtual WorldAtom2 unitAtom(int32 id) = 0;

	virtual int32 animalInitialCount(UnitEnum unitEnum) = 0;
	virtual int32 unitEnumCount(UnitEnum unitEnum) = 0;

	virtual void MoveUnitInstantly(int32 unitId, WorldAtom2 atom) = 0;
	

	virtual int16_t GetFloodId(WorldTile2 tile) = 0;
	virtual void SetWalkable(WorldTile2 tile, bool isWalkable) = 0;
	virtual void SetWalkableSkipFlood(WorldTile2 tile, bool isWalkable) = 0;
	//virtual void SetWalkableNonIntelligent(WorldTile2 tile, bool isWalkable) = 0;
	virtual class PunAStar128x256* pathAI() = 0;

	virtual void SetRoadPathAI(WorldTile2 tile, bool isRoad) = 0;

	virtual int32 FindNearestBuildingId(WorldTile2 tile, CardEnum buildingEnum, int32 townId, int32& minBuildingDist) = 0;
	virtual bool FindPathWater(int32 startPortId, int32 endPortId, std::vector<WorldTile2>& resultPath) = 0;
	virtual std::vector<int32> GetPortIds(int32 townId) = 0;
	virtual bool FindBestPathWater(int32 startTownId, int32 endTownId, WorldTile2 startLand, int32& startPortId, int32& endPortId) = 0;
	virtual bool FindBestPathWater(int32 startPortId, int32 endTownId, int32& endPortId) = 0;
	
	virtual TerrainTileType terraintileType(int32 tileId) = 0;
	virtual bool IsWater(WorldTile2 tile) = 0;
	virtual bool IsMountain(WorldTile2 tile) = 0;
	virtual GeoresourceNode georesource(int32 provinceId) = 0;

	virtual FloatDet Celsius(WorldTile2 tile) = 0;
	virtual FloatDet MinCelsius(WorldTile2 tile) = 0;
	virtual FloatDet MaxCelsius(WorldTile2 tile) = 0;

	virtual int32 tileOwnerTown(WorldTile2 tile) = 0;
	virtual int32 tileOwnerPlayer(WorldTile2 tile) = 0;

	virtual bool IsFrontBuildable(WorldTile2 tile) = 0;
	virtual bool IsRoadOverlapBuildable(WorldTile2 tile) = 0;
	virtual bool IsRoadTile(WorldTile2 tile) = 0;

	virtual bool HasBuilding(int32 tileId) = 0;
	
	virtual bool IsBuildable(WorldTile2 tile) = 0;

	virtual bool IsPlayerBuildable(WorldTile2 tile, int32 playerId) = 0;
	virtual bool IsBuildableForPlayer(WorldTile2 tile, int32 playerId) = 0;

	virtual bool IsCritterBuildingIncludeFronts(WorldTile2 tile) = 0;

	virtual bool IsConnected(WorldTile2 start, WorldTile2 end, int maxRegionDistance, bool canPassGate) = 0;

	virtual bool IsConnectedBuilding(int32 buildingId) = 0;

	virtual void OnRefreshFloodGrid(WorldRegion2 region) = 0;
	
	/*
	 * Province
	 */
	virtual int32 GetProvinceIdRaw(WorldTile2 tile) = 0;
	virtual int32 GetProvinceIdClean(WorldTile2 tile) = 0;
	virtual const std::vector<int32>& GetProvinceIdsFromRegionId(int32 regionId) = 0;
	virtual std::vector<int32> GetProvinceIdsFromArea(TileArea area, bool smallArea) = 0;
	
	virtual bool AreAdjacentProvinces(int32 provinceId1, int32 provinceId2) = 0;
	virtual bool IsProvinceValid(int32 provinceId) = 0;
	
	virtual WorldTile2 GetProvinceCenterTile(int32 provinceId) = 0;
	virtual WorldTile2 GetProvinceRandomTile(int32 provinceId, WorldTile2 floodOrigin, int32 maxRegionDist = 1, bool isIntelligent = false, int32 tries = 100) = 0;
	
	virtual TileArea GetProvinceRectArea(int32 provinceId) = 0;

	template <typename Func>
	void ExecuteOnProvinceTiles(int32 provinceId, Func func)
	{
		PUN_CHECK(IsProvinceValid(provinceId));
		TileArea area = GetProvinceRectArea(provinceId);
		area.ExecuteOnArea_WorldTile2([&](WorldTile2 tile) {
			if (GetProvinceIdRaw(tile) == provinceId) {
				func(tile);
			}
		});
	}

	virtual int32 GetTreeCount(int32 provinceId) = 0;

	virtual int32 GetProvinceCountPlayer(int32 playerId) = 0;
	virtual std::vector<int32> GetProvincesPlayer(int32 playerId) = 0;
	virtual const std::vector<int32>& GetProvincesTown(int32 townId) = 0;

	virtual int32 GetProvinceIncome100(int32 provinceId) = 0;
	virtual int32 GetProvinceUpkeep100(int32 provinceId, int32 playerId) = 0;

	virtual int32 GetProvinceClaimPrice(int32 provinceId, int32 playerId) = 0;
	
	//virtual bool HasOutpostAt(int32 playerId, int32 provinceId) = 0;
	virtual bool IsProvinceNextToTown(int32 provinceId, int32 townId) = 0;
	virtual bool IsProvinceNextToTownByShallowWater(int32 provinceId, int32 townId) = 0;

	virtual void RefreshTerritoryEdge(int32 playerId) = 0;

	virtual bool IsBorderProvince(int32 provinceId) = 0;

	virtual const std::vector<ProvinceConnection>& GetProvinceConnections(int32 provinceId) = 0;
	virtual void AddTunnelProvinceConnections(int32 provinceId1, int32 provinceId2) = 0;
	
	/*
	 * 
	 */

	virtual struct NonWalkableTileAccessInfo TryAccessNonWalkableTile(WorldTile2 start, WorldTile2 nonwalkableTile, int maxRegionDistance, bool canPassGate) = 0;
	//virtual WorldTile2 FindNearbyAvailableTile(WorldTile2 start) = 0;
	virtual WorldTile2 FindNearbyDroppableTile(WorldTile2 start, TileArea excludeArea) = 0;

	virtual std::string unitdebugStr(int id) = 0;
	virtual void unitAddDebugSpeech(int32_t id, std::string message) = 0;


	virtual void AddPopup(int32 playerId, FText popupBody, std::string popupSound = "") = 0;
	virtual void AddPopup(int32 playerId, TArray<FText> popupBody, std::string popupSound = "") = 0;
	
	virtual void AddPopupToFront(int32 playerId, FText popupBody) = 0;
	virtual void AddPopupToFront(int32 playerId, FText popupBody, ExclusiveUIEnum exclusiveEnum, std::string popupSound) = 0;
	
	virtual void AddPopup(PopupInfo popupInfo) = 0;
	virtual void AddPopupToFront(PopupInfo popupInfo) = 0;
	virtual void AddPopupAll(PopupInfo popupInfo, int32 playerToSkip) = 0;

	virtual void AddPopupNonDuplicate(PopupInfo popupInfo) = 0;

	virtual PopupInfo* PopupToDisplay(int32 playerId) = 0;
	virtual void CloseCurrentPopup(int32 playerId) = 0;
	virtual void TryRemovePopups(int32 playerId, PopupReceiverEnum receiverEnum) = 0;

	virtual void AddEventLog(int32 playerId, FText eventMessage, bool isImportant) = 0;
	virtual void AddEventLogToAllExcept(int32 playerId, FText eventMessage, bool isImportant) = 0;

	virtual bool HasQuest(int32 playerId, QuestEnum questEnum) = 0;
	virtual bool WasQuestStarted(int32 playerId, QuestEnum questEnum) = 0;
	virtual void AddQuest(int32 playerId, std::shared_ptr<struct Quest> quest) = 0;
	virtual void TryAddQuest(int32 playerId, std::shared_ptr<struct Quest> quest) = 0;
	virtual bool NeedQuestExclamation(int32 playerId, QuestEnum questEnum) = 0;
	virtual void QuestUpdateStatus(int32 playerId, QuestEnum questEnum, int32 value = -1) = 0;

	virtual bool TryDoNonRepeatAction(int32 playerId, NonRepeatActionEnum actionEnum, int32 nonRepeatTicks) = 0;

	virtual int32 gameSpeedMultiplier() = 0;

	// Science
	virtual int32 GetScience100PerRound(int32 playerId) = 0;
	virtual bool IsResearched(int32 playerId, TechEnum techEnum) = 0;
	virtual bool HasTargetResearch(int32 playerId) = 0;;
	virtual int32 sciTechsCompleted(int32 playerId) = 0;

	virtual bool IsBuildingUnlocked(int32 playerId, CardEnum cardEnumIn) = 0;

	virtual void UpdateProsperityHouseCount(int32 playerId) = 0;

	// Buildings associated with Players
	virtual void PlayerAddHouse(int32 townId, int objectId) = 0;
	virtual void PlayerRemoveHouse(int32 townId, int objectId) = 0;
	virtual void PlayerAddJobBuilding(int32 townId, Building& building, bool isConstructed) = 0;
	virtual void PlayerRemoveJobBuilding(int32 townId, Building& building, bool isConstructed) = 0;
	virtual void RefreshJobDelayed(int32 townId) = 0;

	virtual bool IsInDarkAge(int32 playerId) = 0;

	virtual void RecalculateTaxDelayedPlayer(int32 playerId) = 0;
	virtual void RecalculateTaxDelayedTown(int32 townId) = 0;

	virtual const std::vector<int32>& boarBurrows(int32 provinceId) = 0;
	virtual void AddBoarBurrow(int32 provinceId, int32 buildingId) = 0;
	virtual void RemoveBoarBurrow(int32 provinceId, int32 buildingId) = 0;

	virtual const std::vector<int32>& provinceAnimals(int32 provinceId) = 0;
	virtual void AddProvinceAnimals(int32 provinceId, int32 animalId) = 0;
	virtual void RemoveProvinceAnimals(int32 provinceId, int32 animalId) = 0;

	virtual int32 populationTown(int32 townId) = 0;
	virtual int32 populationPlayer(int32 playerId) = 0;
	virtual int32 worldPlayerPopulation() = 0;
	
	virtual int HousingCapacity(int32 townId) = 0;
	virtual int32 GetHouseLvlCount(int32 playerId, int32 houseLvl, bool includeHigherLvl = false) = 0;

	virtual std::pair<int32, int32> GetStorageCapacity(int32 townId, bool includeUnderConstruction = false) = 0;

	virtual void RemoveJobsFrom(int32 buildingId, bool isRefreshJob) = 0;
	
	virtual void RemoveTenantFrom(int32 buildingId) = 0;

	virtual int foodCount(int32 townId) = 0;
	virtual int32 GetResourceCount(int32 townId, const std::vector<ResourceEnum>& resourceEnums) = 0;
	
	virtual bool HasSeed(int32 playerId, CardEnum seedCardEnum) = 0;

	virtual int32 influence(int32 playerId) = 0;
	virtual int32 influence100(int32 playerId) = 0;
	
	virtual int32 money(int32 playerId) = 0;
	virtual void ChangeMoney(int32 playerId, int32 moneyChange) = 0;
	virtual void ChangeMoney100(int32 playerId, int32 moneyChange100) = 0;
	virtual void ChangeInfluence(int32 playerId, int32 influenceChange) = 0;

	virtual int32 price100(ResourceEnum resourceEnum) = 0;
	virtual int32 price(ResourceEnum resourceEnum) = 0;
	
	virtual void DespawnResourceHolder(ResourceHolderInfo info, int32 playerId) = 0;
	
	virtual int32 resourceCountTown(int32 townId, ResourceEnum resourceEnum) = 0;
	virtual int32 resourceCountPlayer(int32 playerId, ResourceEnum resourceEnum) = 0;

	virtual void AddResourceGlobal(int32 townId, ResourceEnum resourceEnum, int32 amount) = 0;

	virtual bool IsOutputTargetReached(int32 townId, ResourceEnum resourceEnum) = 0;
	

	virtual void SetProvinceOwnerFull(int32 provinceId, int32 townId) = 0;
	virtual int32 provinceOwnerTown(int32 provinceId) = 0;
	virtual int32 provinceOwnerPlayer(int32 provinceId) = 0;

	virtual int PlaceBuilding(class FPlaceBuilding parameters) = 0;

	//virtual void SetLoadingText(std::string loadingText) = 0;

	//! Happiness
	virtual int32 GetAverageHappiness(int32 townId) = 0;
	virtual int32 taxHappinessModifier(int32 townId) = 0;
	virtual int32 citizenDeathHappinessModifier(int32 townId, SeasonStatEnum seasonStatEnum) = 0;
	
	//! Immigration
	virtual void AddMigrationPendingCount(int32 townId, int32 migrationCount) = 0;
	virtual void ImmigrationEvent(int32 townId, int32 migrationCount, FText message, PopupReceiverEnum receiverEnum) = 0;
	
	//! Card system
	virtual int32 BoughtCardCount(int32 playerId, CardEnum buildingEnum) = 0;
	virtual int32 TownhallCardCountTown(int32 playerId, CardEnum cardEnum) = 0;
	virtual int32 TownhallCardCountAll(int32 playerId, CardEnum cardEnum) = 0;
	
	virtual bool HasCardInAnyPile(int32 playerId, CardEnum cardEnum) = 0;
	
	virtual void AddDrawCards(int32 playerId, CardEnum cardEnum, int32 count = 1) = 0;

	virtual bool TryAddCardToBoughtHand(int32 playerId, CardEnum cardEnum, int32 cardCount = 1) = 0;
	
	virtual void GenerateRareCardSelection(int32 playerId, RareHandEnum rareHandEnum, FText rareHandMessage) = 0;

	virtual void CheckGetSeedCard(int32 playerId) = 0;

	virtual bool isStorageAllFull(int32 townId) = 0;
	virtual int32 SpaceLeftFor(ResourceEnum resourceEnum, int32 storageId) = 0;
	virtual void RefreshStorageStatus(int32 storageId) = 0;
	
	//virtual void Just

	virtual void DrawLine(WorldAtom2 atom, FVector startShift, WorldAtom2 endAtom, FVector endShift, FLinearColor Color,
							float Thickness = 1.0f, float LifeTime = 10000) = 0;
	virtual void DrawLine(WorldTile2 tile, FVector startShift, WorldTile2 endTile , FVector endShift, FLinearColor Color,
							float Thickness = 1.0f, float LifeTime = 10000) = 0;
	virtual void DrawLine(WorldTile2 tile, FLinearColor Color, float Thickness = 1.0f, float LifeTime = 10000) = 0;
	virtual void DrawLinePath(WorldTile2 start, WorldTile2 end, FLinearColor Color, float Thickness = 1.0f, float LifeTime = 10000) = 0;
	virtual void DrawArea(TileArea area, FLinearColor color = FLinearColor::Yellow, float tilt = 0) = 0;

	//! Terrain
	virtual int32 GetFertilityPercent(WorldTile2 tile) = 0;
	virtual BiomeEnum GetBiomeEnum(WorldTile2 tile) = 0;
	virtual BiomeEnum GetBiomeProvince(int32 provinceId) = 0;

	virtual void RefreshHeightForestColorTexture(TileArea area, bool isInstant) = 0;
	virtual void SetRoadWorldTexture(WorldTile2 tile, bool isRoad, bool isDirtRoad) = 0;
	virtual void RefreshHeightForestRoadTexture() = 0;

	virtual void TryRemoveDescriptionUI(ObjectTypeEnum type, int32_t objectId) = 0;

	virtual int32 aiStartIndex() = 0;
	virtual int32 aiEndIndex() = 0;
	virtual bool IsAIPlayer(int32 playerId) = 0;

	virtual TCHAR* AIPrintPrefix(int32 aiPlayerId) = 0;

	virtual void ChangeRelationshipModifier(int32 aiPlayerId, int32 towardPlayerId, RelationshipModifierEnum modifierEnum, int32 amount) = 0;

	virtual WorldAtom2 homeAtom(int32 townId) = 0;

	//! Snow

	//! Victory Condition
	virtual FGameEndStatus endStatus() = 0;
	virtual void ExecuteScienceVictory(int32 playerId) = 0;


	//! Players
	virtual bool HasTownhall(int32 playerId) = 0;
	virtual bool HasChosenLocation(int32 playerId) = 0;

	virtual int32 homeProvinceId(int32 playerId) = 0;

	template<typename Func>
	void ExecuteOnPlayersAndAI(Func func) {
		std::vector<int32> allHumanPlayerIdsLocal = allHumanPlayerIds();
		for (int32 playerId : allHumanPlayerIdsLocal) {
			func(playerId);
		}
		for (int32 playerId = aiStartIndex(); playerId <= aiEndIndex(); playerId++) {
			func(playerId);
		}
	}
	std::vector<int32> GetAllPlayersAndAI()
	{
		std::vector<int32> playersAndAIIds = allHumanPlayerIds();
		for (int32 playerId = aiStartIndex(); playerId <= aiEndIndex(); playerId++) {
			playersAndAIIds.push_back(playerId);
		}
		return playersAndAIIds;
	}
	
	template<typename Func>
	void ExecuteOnAI(Func func) {
		for (int32 playerId = aiStartIndex(); playerId <= aiEndIndex(); playerId++) {
			func(playerId);
		}
	}

	template<typename Func>
	void ExecuteOnConnectedPlayers(Func func) {
		std::vector<int32> connectedPlayerIdsLocal = connectedPlayerIds();
		for (int32 playerId : connectedPlayerIdsLocal) {
			func(playerId);
		}
	}

	template<typename Func>
	void ExecuteOnAllHumanPlayers(Func func) {
		std::vector<int32> allHumanPlayerIdsLocal = allHumanPlayerIds();
		for (int32 playerId : allHumanPlayerIdsLocal) {
			func(playerId);
		}
	}

	bool IsValidPlayer(int32 playerIdIn) {
		std::vector<int32> allHumanPlayerIdsLocal = allHumanPlayerIds();
		for (int32 playerId : allHumanPlayerIdsLocal) {
			if (playerId == playerIdIn) {
				return true;
			}
		}
		for (int32 playerId = aiStartIndex(); playerId <= aiEndIndex(); playerId++) {
			if (playerId == playerIdIn) {
				return true;
			}
		}
		return false;
	}
};

class IPlayerSimulationInterface
{
public:
	virtual ~IPlayerSimulationInterface() {}
	
	//virtual void AddPlayer(class FAddPlayer command) = 0;

	//virtual void PoliciesSelected(class FPoliciesSelects policies) = 0;
	virtual int32 PlaceBuilding(FPlaceBuilding parameters) = 0;
	virtual void PlaceDrag(class FPlaceDrag parameters) = 0;
	virtual void JobSlotChange(class FJobSlotChange jobSlotChange) = 0;
	virtual void SetAllowResource(FSetAllowResource command) = 0;
	virtual void SetPriority(class FSetPriority command) = 0;
	virtual void SetTownPriority(class FSetTownPriority command) = 0;
	virtual void SetGlobalJobPriority(class FSetGlobalJobPriority command) = 0;
	virtual void GenericCommand(class FGenericCommand command) = 0;

	virtual void TradeResource(class FTradeResource tradeResource) = 0;
	virtual void SetIntercityTrade(class FSetIntercityTrade command) = 0;
	virtual void UpgradeBuilding(class FUpgradeBuilding upgradeCommand) = 0;
	virtual void ChangeWorkMode(class FChangeWorkMode command) = 0;
	virtual void ChooseLocation(class FChooseLocation command) = 0;
	virtual void ChooseInitialResources(class FChooseInitialResources command) = 0;
	
	virtual void PopupDecision(class FPopupDecision command) = 0;
	virtual void RerollCards(class FRerollCards command) = 0;

	virtual void SelectRareCard(class FSelectRareCard command) = 0;
	virtual void BuyCards(class FBuyCard command) = 0;
	virtual void SellCards(class FSellCards command) = 0;
	virtual void UseCard(class FUseCard command) = 0;
	virtual void UnslotCard(class FUnslotCard command) = 0;

	virtual void Attack(class FAttack command) = 0;
	//virtual void TrainUnit(class FTrainUnit command) = 0;

	virtual void ClaimLand(class FClaimLand command) = 0;
	virtual void ChooseResearch(class FChooseResearch command) = 0;

	virtual void ChangeName(class FChangeName command) = 0;
	virtual void SendChat(class FSendChat command) = 0;

	virtual void Cheat(class FCheat command) = 0;
};