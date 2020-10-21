// Pun Dumnernchanvanit's

#pragma once

#include "IGameSimulationCore.h"
#include "PlayerOwnedManager.h"
#include "GeoresourceSystem.h"
#include "PunCity/NetworkStructs.h"
#include "Resource/ResourceSystem.h"
#include "PunCity/DisplaySystem/PunTerrainGenerator.h"
#include "TreeSystem.h"
#include "BuildingSystem.h"
#include "Buildings/House.h"
#include "Buildings/TownHall.h"
#include "PunCity/CppUtils.h"
#include "SimUtils.h"


class AIRegionStatus
{
public:
	bool useDetermined() {
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
	AIRegionProposedPurposeEnum proposedPurpose = AIRegionProposedPurposeEnum::None;
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

		_relationshipModifiers.resize(GameConstants::MaxPlayersAndAI);
		for (size_t i = 0; i < _relationshipModifiers.size(); i++) {
			_relationshipModifiers[i].resize(RelationshipModifierCount);
		}
	}

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

	void Tick1Sec()
	{
		if (!_active) {
			return;
		}
		
		auto& playerOwned = _simulation->playerOwned(_aiPlayerId);
		auto& resourceSystem = _simulation->resourceSystem(_aiPlayerId);
		auto& terrainGenerator = _simulation->terrainGenerator();
		auto& treeSystem = _simulation->treeSystem();
		auto& buildingSystem = _simulation->buildingSystem();
		auto& provinceSys = _simulation->provinceSystem();
		
		const std::vector<int32>& provincesClaimed = playerOwned.provincesClaimed();

		// Sync _regionStatuses
		{
			// Reset to false
			for (AIRegionStatus& status : _regionStatuses) {
				status.updatedThisRound = false;
			}
			// Create status for region we don't have yet
			for (int32 provinceId : provincesClaimed) {
				auto it = std::find_if(_regionStatuses.begin(), _regionStatuses.end(), [&](AIRegionStatus& status) {
					return status.provinceId == provinceId;
				});
				
				if (it == _regionStatuses.end()) 
				{
					AIRegionProposedPurposeEnum purpose = DetermineProvincePurpose(provinceId);
					_regionStatuses.push_back({ provinceId , purpose });
				}
				else {
					it->updatedThisRound = true;
				}
			}
			// Delete the regions we don't use
			for (size_t i = _regionStatuses.size(); i-- > 0;) {
				if (!_regionStatuses[i].updatedThisRound) {
					_regionStatuses.erase(_regionStatuses.begin() + i);
				}
			}
		}

		

		/*
		 * Choose Location (Initialize)
		 */
		if (!playerOwned.hasChosenLocation())
		{
			for (int count = 10000; count-- > 0;) 
			{
				//int32 regionX = GameRand::Rand() % GameMapConstants::RegionsPerWorldX_Inner + GameMapConstants::MinInnerRegionX;
				//int32 regionY = GameRand::Rand() % GameMapConstants::RegionsPerWorldY_Inner + GameMapConstants::MinInnerRegionY;
				//WorldRegion2 region(regionX, regionY);

				int32 provinceId = GameRand::Rand() % GameMapConstants::TotalRegions;

				if (provinceSys.IsProvinceValid(provinceId))
				{
					//BiomeEnum biomeEnum = terrainGenerator.GetBiome(region);
					AIRegionProposedPurposeEnum purposeEnum = DetermineProvincePurpose(provinceId);

					if (purposeEnum != AIRegionProposedPurposeEnum::None &&
						SimUtils::CanReserveSpot_NotTooCloseToAnother(provinceId, _simulation, 5))
					{
						// Note, FindStartSpot isn't always valid
						TileArea finalArea = SimUtils::FindStartSpot(provinceId, _simulation);

						if (finalArea.isValid())
						{
							auto command = MakeCommand<FChooseLocation>();
							command->provinceId = provinceId;
							command->isChoosingOrReserving = true;
							_playerInterface->ChooseLocation(*command);
							break;
						}
					}
				}
			}
			
			return;
		}

		/*
		 * Choose initial resources (Initialize)
		 */
		if (!playerOwned.hasChosenInitialResources()) {
			FChooseInitialResources command = FChooseInitialResources::GetDefault();
			command.playerId = _aiPlayerId;
			_playerInterface->ChooseInitialResources(command);
			return;
		}

		/*
		 * Build townhall (Initialize)
		 */
		if (!playerOwned.hasTownhall())
		{
			_LOG(PunAI, "%s Try Build Townhall", ToTChar(AIPrintPrefix()));
			
			PUN_CHECK(playerOwned.provincesClaimed().size() == 1);
			int32 provinceId = playerOwned.provincesClaimed()[0];
			TileArea provinceRectArea = provinceSys.GetProvinceRectArea(provinceId);

			WorldTile2 townhallSize = GetBuildingInfo(CardEnum::Townhall).size;
			TileArea area = BuildingArea(provinceRectArea.centerTile(), townhallSize, Direction::S);

			// Add Road...
			area.minX = area.minX - 1;
			area.minY = area.minY - 1;
			area.maxX = area.maxX + 1;
			area.maxY = area.maxY + 1;

			// Add Storages
			area.minY = area.minY - 5;
			area.maxY = area.maxY + 5; // Ensure area center is also townhall center

			PUN_CHECK(area.isValid());
			
			TileArea buildableArea = SimUtils::SpiralFindAvailableBuildArea(area, [&](WorldTile2 tile)
			{
				// Out of province, can't build...
				if (provinceSys.GetProvinceIdClean(tile) != provinceId) {
					return false;
				}

				// Not buildable and not critter building
				if (!_simulation->IsBuildable(tile) &&
					!_simulation->IsCritterBuildingIncludeFronts(tile))
				{
					std::vector<int32> frontIds = _simulation->frontBuildingIds(tile);
					if (frontIds.size() > 0)
					{
						// If any front isn't regional building, we can't plant
						for (int32 frontId : frontIds) {
							if (!IsRegionalBuilding(_simulation->buildingEnum(frontId))) {
								return false;
							}
						}
						return true;
					}

					CardEnum buildingEnum = _simulation->buildingEnumAtTile(tile);
					return IsRegionalBuilding(buildingEnum);
				}
				return true;
			},
			[&](WorldTile2 tile) {
				return !provinceRectArea.HasTile(tile);
			});

			if (buildableArea.isValid())
			{
				_LOG(PunAI, "%s Build Townhall: buildableArea isValid", ToTChar(AIPrintPrefix()));
				
				auto command = MakeCommand<FPlaceBuilding>();
				command->buildingEnum = static_cast<uint8>(CardEnum::Townhall);
				command->playerId = _aiPlayerId;
				command->center = buildableArea.centerTile(); // area center is also townhall center
				command->faceDirection = static_cast<uint8>(Direction::S);

				BldInfo buildingInfo = GetBuildingInfoInt(command->buildingEnum);
				command->area = BuildingArea(command->center, buildingInfo.size, Direction::S);
				
				_playerInterface->PlaceBuilding(*command);
			}
			
			return;
		}


		/*
		 * Update Relationship
		 */
		std::vector<int32> allPlayersAndAI = _simulation->GetAllPlayersAndAI();
		for (int32 playerId = 0; playerId < allPlayersAndAI.size(); playerId++)
		{
			if (_simulation->HasTownhall(playerId))
			{
#define MODIFIER(EnumName) _relationshipModifiers[playerId][static_cast<int>(RelationshipModifierEnum::EnumName)]

				// Decaying modifiers (decay every season)
				if (Time::Ticks() % Time::TicksPerSeason == 0)
				{
					DecreaseToZero(MODIFIER(YouGaveUsGifts));
					IncreaseToZero(MODIFIER(YouStealFromUs));
					IncreaseToZero(MODIFIER(YouKidnapFromUs));
				}

				//if (Time::Ticks() % Time::TicksPerMinute == 0)
				{
					// Calculated modifiers
					auto calculateStrength = [&](int32 playerIdScope) {
						return _simulation->influence(playerIdScope) + _simulation->playerOwned(playerIdScope).totalInfluenceIncome100() * Time::RoundsPerYear;
					};
					int32 aiStrength = calculateStrength(_aiPlayerId);
					int32 counterPartyStrength = calculateStrength(playerId);

					MODIFIER(YouAreStrong) = Clamp((counterPartyStrength - aiStrength) / 50, 0, 20);
					MODIFIER(YouAreWeak) = Clamp((aiStrength - counterPartyStrength) / 50, 0, 20);

					const std::vector<int32>& provinceIds = _simulation->playerOwned(_aiPlayerId).provincesClaimed();
					int32 borderCount = 0;
					for (int32 provinceId : provinceIds) {
						const std::vector<ProvinceConnection>& connections = _simulation->GetProvinceConnections(provinceId);
						for (const ProvinceConnection& connection : connections) {
							if (_simulation->provinceOwner(connection.provinceId) == playerId) {
								borderCount++;
							}
						}
					}
					MODIFIER(AdjacentBordersSparkTensions) = std::max(-borderCount * 5, -20);

					// townhall nearer 500 tiles will cause tensions
					int32 townhallDistance = WorldTile2::Distance(_simulation->townhallGateTile(_aiPlayerId), _simulation->townhallGateTile(playerId));
					if (townhallDistance <= 500) {
						MODIFIER(TownhallProximitySparkTensions) = -20 * (500 - townhallDistance) / 500;
					}
					else {
						MODIFIER(TownhallProximitySparkTensions) = 0;
					}
				}
#undef MODIFIER
			}
		}

		/*
		 * Do good/bad act once every year
		 */
		int32 secondToAct = GameRand::Rand(Time::Years() * _aiPlayerId) % Time::SecondsPerYear;
		if (Time::Seconds() % Time::SecondsPerYear == secondToAct)
		{
			PUN_LOG("[AIPlayer] Act pid:%d second:%d", _aiPlayerId, secondToAct);

			int32 maxRelationshipPlayerId = -1;
			int32 maxRelationship = 0;

			int32 minRelationshipPlayerId = -1;
			int32 minRelationship = 0;
			_simulation->ExecuteOnPlayersAndAI([&](int32 playerId)
			{
				int32 relationship = GetTotalRelationship(playerId);
				if (relationship > maxRelationship) {
					maxRelationship = relationship;
					maxRelationshipPlayerId = playerId;
				}
				if (relationship <= minRelationship)
				{
					minRelationship = relationship;
					minRelationshipPlayerId = playerId;
				}
			});

			// Gifting
			if (maxRelationshipPlayerId != -1 &&
				_simulation->HasTownhall(maxRelationshipPlayerId) &&
				_simulation->money(_aiPlayerId) > 500)
			{
				PUN_LOG("[AIPlayer] gift pid:%d target:%d second:%d", _aiPlayerId, maxRelationshipPlayerId, secondToAct);
				
				auto command = make_shared<FGenericCommand>();
				command->genericCommandType = FGenericCommand::Type::SendGift;
				command->playerId = _aiPlayerId;
				command->intVar1 = maxRelationshipPlayerId;
				command->intVar2 = static_cast<int>(ResourceEnum::Money);
				command->intVar3 = std::min(100, _simulation->money(_aiPlayerId) - 500);

				_playerInterface->GenericCommand(*command);
			}

			// TODO: ...
			//// Steal/Kidnap
			//if (minRelationshipPlayerId != -1 &&
			//	_simulation->HasTownhall(minRelationshipPlayerId))
			//{
			//	// Ideally 20 money per pop
			//	//  Target also need to have more than 1k
			//	if (_simulation->money(minRelationshipPlayerId) > 1000 &&
			//		_simulation->money(_aiPlayerId) < _simulation->population(_aiPlayerId) * 20)
			//	{
			//		PUN_LOG("[AIPlayer] Steal pid:%d target:%d second:%d", _aiPlayerId, minRelationshipPlayerId, secondToAct);

			//		auto command = make_shared<FPlaceBuilding>();
			//		command->playerId = _aiPlayerId;
			//		command->center = _simulation->townhall(minRelationshipPlayerId).centerTile();
			//		command->buildingEnum = static_cast<uint8>(CardEnum::Steal);

			//		_playerInterface->PlaceBuilding(*command);
			//	}
			//	else
			//	{
			//		PUN_LOG("AI Kidnap pid:%d second:%d", _aiPlayerId, secondToAct);
			//		
			//		auto command = make_shared<FPlaceBuilding>();
			//		command->playerId = _aiPlayerId;
			//		command->center = _simulation->townhall(minRelationshipPlayerId).centerTile();
			//		command->buildingEnum = static_cast<uint8>(CardEnum::Kidnap);

			//		_playerInterface->PlaceBuilding(*command);
			//	}
			//}
		}


		

		/*
		 * Prepare stats
		 */
		TownHall& townhall = _simulation->townhall(_aiPlayerId);

		// Make sure townhall is marked as a city block...
		int32 townhallProvinceId = townhall.provinceId();
		for (AIRegionStatus& status : _regionStatuses) 
		{
			if (status.provinceId == townhallProvinceId) {
				status.currentPurpose = AIRegionPurposeEnum::City;
				treeSystem.MarkArea(_aiPlayerId, provinceSys.GetProvinceRectArea(status.provinceId), false, ResourceEnum::None);
				break;
			}
		}

		int32 population = playerOwned.population();
		const std::vector<std::vector<int32>>& jobBuildingEnumToIds = playerOwned.jobBuildingEnumToIds();

		int32 jobSlots = 0;
		for (const std::vector<int32>& jobBuildingIds : jobBuildingEnumToIds) {
			for (int32 buildingId : jobBuildingIds) 
			{
				Building& building = buildingSystem.building(buildingId);
				jobSlots += building.occupantCount();
			}
		}


		const std::vector<int32> constructionIds = playerOwned.constructionIds();
		int32 housesUnderConstruction = 0;
		for (int32 constructionId : constructionIds) {
			if (_simulation->buildingEnum(constructionId) == CardEnum::House) {
				housesUnderConstruction++;
			}
		}
		int32 otherBuildingsUnderConstruction = constructionIds.size() - housesUnderConstruction;
		
		bool notTooManyHouseUnderConstruction = housesUnderConstruction <= 1;
		bool notTooManyUnderConstruction = otherBuildingsUnderConstruction <= 1;

		// Need Food 1 month in advance
		bool hasEnoughFood = _simulation->foodCount(_aiPlayerId) >= (population * BaseHumanFoodCost100PerYear / 2 / 100);
		bool outOfFood = _simulation->foodCount(_aiPlayerId) <= (population * BaseHumanFoodCost100PerYear / 8 / 100);

		// Need heat to survive winter
		int32 heatResources = resourceSystem.resourceCount(ResourceEnum::Coal) + resourceSystem.resourceCount(ResourceEnum::Wood);
		bool hasEnoughHeat = heatResources >= population * HumanHeatResourcePerYear * 2;

		// Place houses
		if (notTooManyHouseUnderConstruction)
		{
			int32 housingCapacity = _simulation->HousingCapacity(_aiPlayerId);
			const int32 maxHouseCapacity = 4 * 30; // 120 max pop
			
			if (housingCapacity < maxHouseCapacity && population > housingCapacity)
			{
				// Group 4 houses into a block...
				AICityBlock block;

				block.topBuildingEnums = { CardEnum::House, CardEnum::House };
				block.bottomBuildingEnums = { CardEnum::House, CardEnum::House };
				block.CalculateSize();

				//block.SetMinTile(
				//	AlgorithmUtils::FindNearbyAvailableTile(townhall.centerTile(), [&](const WorldTile2& tile) {
				//		return block.CanPlaceCityBlock(tile, _playerId, _simulation);
				//	})
				//);
				block.TryFindArea(townhall.centerTile(), _aiPlayerId, _simulation);

				if (block.HasArea()) {
					std::vector<std::shared_ptr<FNetworkCommand>> commands;
					SimUtils::PlaceCityBlock(block, _aiPlayerId, commands);
					_simulation->ExecuteNetworkCommands(commands);
				}
			}
		}

		// Put down forest cluster... trying to keep 20% laborer count
		{
			if (population * 8 / 10 > jobSlots)
			{
				AIRegionStatus* bestStatus = nullptr;
				int32 bestScore = -1;
				AICityBlock bestBlock;

				bool alreadyHaveForester = false;
				for (size_t i = 0; i < _regionStatuses.size(); i++) {
					if (_regionStatuses[i].currentPurpose == AIRegionPurposeEnum::Forester) {
						alreadyHaveForester = true;
					}
				}
				
				for (size_t i = 0; i < _regionStatuses.size(); i++) 
				{
					AIRegionStatus& status = _regionStatuses[i];

					// Already placed foresting cluster on this tile..
					if (status.useDetermined()) {
						continue;
					}

					int32 provinceId = status.provinceId;
					WorldTile2 provinceCenter = provinceSys.GetProvinceCenterTile(provinceId);

					// Make sure we can place this block
					AICityBlock block;

					if (status.proposedPurpose == AIRegionProposedPurposeEnum::Fertility) {
						block.topBuildingEnums = { CardEnum::Farm, CardEnum::Farm , CardEnum::StorageYard };
						block.bottomBuildingEnums = { CardEnum::Farm, CardEnum::Farm, CardEnum::StorageYard };
						
						status.currentPurpose = AIRegionPurposeEnum::Farm;
					}
					else if (status.proposedPurpose == AIRegionProposedPurposeEnum::Ranch) {
						block.topBuildingEnums = {};
						block.bottomBuildingEnums = { CardEnum::RanchPig };

						status.currentPurpose = AIRegionPurposeEnum::Ranch;
					}
					else if (status.proposedPurpose == AIRegionProposedPurposeEnum::Tree) {
						if (!alreadyHaveForester) {
							block.topBuildingEnums = { CardEnum::Forester, CardEnum::CharcoalMaker, CardEnum::StorageYard };
							block.bottomBuildingEnums = { CardEnum::HuntingLodge, CardEnum::StorageYard, CardEnum::StorageYard };

							status.currentPurpose = AIRegionPurposeEnum::Forester;
						}
						else {
							block.topBuildingEnums = { CardEnum::FruitGatherer, CardEnum::StorageYard, CardEnum::House };
							block.bottomBuildingEnums = { CardEnum::HuntingLodge, CardEnum::StorageYard, CardEnum::StorageYard };

							status.currentPurpose = AIRegionPurposeEnum::FruitGather;
						}
					}
					else {
						status.currentPurpose = AIRegionPurposeEnum::City;
					}
					
					block.CalculateSize();
					block.TryFindArea(provinceCenter, _aiPlayerId, _simulation);

					if (!block.HasArea()) {
						continue;
					}

					int32 treeCount = _simulation->GetTreeCount(provinceId);
					
					int32 distance = WorldTile2::ManDistance(provinceCenter, townhall.centerTile());

					int32 score = std::max(0, 1000 + treeCount - distance * 3);

					// too far.. score 0
					if (distance > CoordinateConstants::TilesPerRegion * 2) {
						score = 0;
					}

					if (score > bestScore) {
						bestStatus = &status;
						bestScore = score;
						bestBlock = block;
					}
				}

				if (bestStatus) 
				{
					TileArea bestProvinceRectArea = provinceSys.GetProvinceRectArea(bestStatus->provinceId);
					
					// Remove gather marks, since this will be used for sustainable gathering...
					if (bestStatus->currentPurpose == AIRegionPurposeEnum::City ||
						bestStatus->currentPurpose == AIRegionPurposeEnum::Farm ||
						bestStatus->currentPurpose == AIRegionPurposeEnum::Ranch) 
					{
						treeSystem.MarkArea(_aiPlayerId, bestProvinceRectArea, false, ResourceEnum::None);
					}
					else {
						treeSystem.MarkArea(_aiPlayerId, bestProvinceRectArea, false, ResourceEnum::Stone);
					}

					bestStatus->proposedBlock = bestBlock;
				}

				
			}

			if (notTooManyUnderConstruction)
			{
				for (size_t i = 0; i < _regionStatuses.size(); i++) 
				{
					if (_regionStatuses[i].currentPurpose != AIRegionPurposeEnum::None &&
						!_regionStatuses[i].blockPlaced) 
					{
						if (_regionStatuses[i].proposedBlock.IsValid()) {
							std::vector<std::shared_ptr<FNetworkCommand>> commands;
							SimUtils::PlaceForestBlock(_regionStatuses[i].proposedBlock, _aiPlayerId, commands);
							_simulation->ExecuteNetworkCommands(commands);
							
							//PlaceForestBlock(_regionStatuses[i].proposedBlock);
						}
						_regionStatuses[i].blockPlaced = true;
						break;
					}
				}
			}
		}

		/*
		 * Accept immigrants
		 */
		PopupInfo* popup = _simulation->PopupToDisplay(_aiPlayerId);
		if (popup && popup->replyReceiver == PopupReceiverEnum::ImmigrationEvent) {
			if (hasEnoughFood && hasEnoughHeat && population < 50) {
				townhall.AddRequestedImmigrants();
			}
			_simulation->CloseCurrentPopup(_aiPlayerId);
		}

		/*
		 * Find nearby region to claim
		 */
		if (provincesClaimed.size() < 5)
		{
			// Look adjacent to provincesClaimed for something to claim.
			int32 bestClaimProvinceId = -1;
			int32 bestClaimPoints = -99999;
			
			auto checkAdjacent = [&] (ProvinceConnection connection)
			{
				if (_simulation->IsProvinceValid(connection.provinceId) && 
					_simulation->provinceOwner(connection.provinceId) == -1)
				{
					// best place to claim is the one with resource or flat land
					int32 flatLandPoints = provinceSys.provinceFlatTileCount(connection.provinceId) / 2;
					int32 treePoints = _simulation->GetTreeCount(connection.provinceId) * 2;
					//int32 randomPoints = GameRand::Rand() % std::max(treePoints, 1);

					WorldTile2 provinceCenter = provinceSys.GetProvinceCenterTile(connection.provinceId);

					const int32 distanceFactor = 70;
					int32 distance = WorldTile2::ManDistance(provinceCenter, townhall.centerTile());
					int32 distancePoints = distance * (flatLandPoints + treePoints) / CoordinateConstants::TilesPerRegion * distanceFactor / 100;
					//int32_t resourcePoints =

					AIRegionProposedPurposeEnum purpose = DetermineProvincePurpose(connection.provinceId);

					if (purpose != AIRegionProposedPurposeEnum::None)
					{
						int32 points = flatLandPoints + treePoints - distancePoints;

						if (points > bestClaimPoints) {
							bestClaimProvinceId = connection.provinceId;
							bestClaimPoints = points;
						}
					}
				}
			};
			
			for (int32 provinceId : provincesClaimed) {
				provinceSys.ExecuteAdjacentProvinces(provinceId, checkAdjacent);
			}

			if (bestClaimProvinceId != -1)
			{
				int32 regionPrice = playerOwned.GetBaseProvinceClaimPrice(bestClaimProvinceId);
				
				if (resourceSystem.money() >= regionPrice) 
				{
					// AI claim at half price...
					resourceSystem.ChangeMoney(regionPrice / 2);
					
					auto command = MakeCommand<FClaimLand>();
					command->provinceId = bestClaimProvinceId;
					_playerInterface->ClaimLand(*command);
				}
			}
		}
	}

	/*
	 * UI Interface
	 */

	void GetAIRelationshipText(std::stringstream& ss, int32 playerId)
	{
		ss << "Overall: " << GetTotalRelationship(playerId) << "\n";
		
		const std::vector<int32>& modifiers = _relationshipModifiers[playerId];
		for (int32 i = 0; i < modifiers.size(); i++) {
			if (modifiers[i] != 0) {
				ss << ToSignedNumber(modifiers[i]) << " " << RelationshipModifierName[i] << "\n";
			}
		}
	}

	int32 GetTotalRelationship(int32 towardPlayerId)
	{
		return CppUtils::Sum(_relationshipModifiers[towardPlayerId]);
	}

	// Friendship
	bool shouldShow_DeclareFriendship(int32 askingPlayerId) {
		if (GetRelationshipModifier(askingPlayerId, RelationshipModifierEnum::YouAttackedUs) > 0) {
			return false;
		}
		return GetRelationshipModifier(askingPlayerId, RelationshipModifierEnum::YouBefriendedUs) == 0;
	}
	int32 friendshipPrice() { return 200; }
	void DeclareFriendship(int32 askingPlayerId) {
		SetRelationshipModifier(askingPlayerId, RelationshipModifierEnum::YouBefriendedUs, friendshipPrice() / GoldToRelationship);
		_simulation->ChangeMoney(askingPlayerId, -friendshipPrice());
	}

	// Marriage
	bool shouldShow_MarryOut(int32 askingPlayerId) {
		return GetRelationshipModifier(askingPlayerId, RelationshipModifierEnum::WeAreFamily) == 0;
	}
	int32 marryOutPrice() { return 1000; }
	void MarryOut(int32 askingPlayerId) {
		SetRelationshipModifier(askingPlayerId, RelationshipModifierEnum::WeAreFamily, marryOutPrice() / GoldToRelationship);
		_simulation->ChangeMoney(askingPlayerId, -marryOutPrice());
	}

	// 

	
	void DeclareWar(int32 askingPlayerId)
	{
		SetRelationshipModifier(askingPlayerId, RelationshipModifierEnum::YouAttackedUs, -100);
		SetRelationshipModifier(askingPlayerId, RelationshipModifierEnum::YouBefriendedUs, 0);
	}

	//
	void SetRelationshipModifier(int32 askingPlayerId, RelationshipModifierEnum modifier, int32 value) {
		_relationshipModifiers[askingPlayerId][static_cast<int>(modifier)] = value;
	}
	void ChangeRelationshipModifier(int32 askingPlayerId, RelationshipModifierEnum modifier, int32 value) {
		_relationshipModifiers[askingPlayerId][static_cast<int>(modifier)] += value;
	}
	int32 GetRelationshipModifier(int32 askingPlayerId, RelationshipModifierEnum modifier) {
		return _relationshipModifiers[askingPlayerId][static_cast<int>(modifier)];
	}

	void ClearRelationshipModifiers(int32 towardPlayerId) {
		std::vector<int32>& modifiers = _relationshipModifiers[towardPlayerId];
		std::fill(modifiers.begin(), modifiers.end(), 0);
	}
	
	/*
	 * System
	 */

	void Serialize(FArchive& Ar)
	{
		SerializeVecObj(Ar, _regionStatuses);
		Ar << _active;

		SerializeVecVecValue(Ar, _relationshipModifiers);
	}

	void AIDebugString(std::stringstream& ss) {
		ss << "-- AI Regions " << _simulation->playerName(_aiPlayerId) << "\n";
		for (AIRegionStatus& status : _regionStatuses) {
			ss << " Region " << status.provinceId << " purpose:" << AIRegionPurposeName[static_cast<int>(status.currentPurpose)] << " proposed:" << static_cast<int>(status.proposedPurpose);
		}
	}

private:
	template<class T>
	std::shared_ptr<T> MakeCommand() {
		auto command = std::make_shared<T>();
		std::static_pointer_cast<FNetworkCommand>(command)->playerId = _aiPlayerId;
		return command;
	}

	//void PlaceBuildingRow(const std::vector<CardEnum>& buildingEnums, WorldTile2 start, bool isFaceSouth)
	//{
	//	int32 currentY = start.y;
	//	int32 currentX = start.x;

	//	auto placeRow = [&](CardEnum buildingEnum, int32_t sign)
	//	{
	//		WorldTile2 size = GetBuildingInfo(buildingEnum).size;

	//		// Storage yard always 4x4
	//		if (buildingEnum == CardEnum::StorageYard) {
	//			size = WorldTile2(4, 4);
	//		}
	//		
	//		// 1 shift 1 ... 2 shift 1 .. 3 shift 2 ... 4 shift 2 ... 5 shift 3
	//		int32_t yShift = (size.y + 1) / 2;
	//		int32_t xShift = (size.x + 1) / 2;
	//		WorldTile2 centerTile(currentX + sign * xShift, currentY + sign * yShift);
	//		Direction faceDirection = isFaceSouth ? Direction::S : Direction::N;

	//		auto command = MakeCommand<FPlaceBuilding>();
	//		command->buildingEnum = static_cast<uint8>(buildingEnum);
	//		command->area = BuildingArea(centerTile, size, faceDirection);
	//		command->center = centerTile;
	//		command->faceDirection = uint8(faceDirection);

	//		
	//		_playerInterface->PlaceBuilding(*command);

	//		//PUN_LOG("AI Build %s", *centerTile.To_FString());

	//		currentY += sign * size.y;
	//	};

	//	if (isFaceSouth) {
	//		for (CardEnum buildingEnum : buildingEnums) {
	//			placeRow(buildingEnum, 1);
	//		}
	//	} else {
	//		for (size_t i = buildingEnums.size(); i-- > 0;) {
	//			placeRow(buildingEnums[i], -1);
	//		}
	//	}
	//}

	//void PlaceCityBlock(AICityBlock& block)
	//{
	//	TileArea blockArea = block.area();

	//	// Build surrounding road...
	//	{
	//		TArray<int32> path;
	//		for (int32_t x = blockArea.minX; x <= blockArea.maxX; x++) {
	//			path.Add(WorldTile2(x, blockArea.minY).tileId());
	//			path.Add(WorldTile2(x, blockArea.maxY).tileId());
	//		}
	//		for (int32_t y = blockArea.minY + 1; y <= blockArea.maxY - 1; y++) {
	//			path.Add(WorldTile2(blockArea.minX, y).tileId());
	//			path.Add(WorldTile2(blockArea.maxX, y).tileId());
	//		}

	//		auto command = MakeCommand<FPlaceDrag>();
	//		command->path = path;
	//		command->placementType = static_cast<int8>(PlacementType::DirtRoad);
	//		_playerInterface->PlaceDrag(*command);
	//	}

	//	// Build buildings
	//	{
	//		PlaceBuildingRow(block.topBuildingEnums, blockArea.max(), false);
	//		PlaceBuildingRow(block.bottomBuildingEnums, blockArea.min(), true);
	//	}
	//}

	//void PlaceForestBlock(AICityBlock& block)
	//{
	//	PUN_CHECK(block.IsValid());
	//	
	//	TileArea blockArea = block.area();
	//	int32 roadTileX = block.midRoadTileX();

	//	//PUN_LOG("PlaceForestBlock %s", *ToFString(blockArea.ToString()));

	//	// Face up row
	//	PlaceBuildingRow(block.topBuildingEnums, WorldTile2(roadTileX, blockArea.maxY), false);
	//	PlaceBuildingRow(block.bottomBuildingEnums, WorldTile2(roadTileX, blockArea.minY), true);
	//}

	AIRegionProposedPurposeEnum DetermineProvincePurpose(int32 provinceId)
	{
		auto& terrainGenerator = _simulation->terrainGenerator();
		auto& provinceSys = _simulation->provinceSystem();
		
		int32 treeCount = _simulation->GetTreeCount(provinceId);
		int32 fertility = terrainGenerator.GetRegionFertility(provinceSys.GetProvinceCenterTile(provinceId).region()); // Rough fertility calculation
		int32 flatLandCount = provinceSys.provinceFlatTileCount(provinceId);
		
		if (fertility > 75) {
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
		if (treeCount > CoordinateConstants::TileIdsPerRegion / 4 / 4) {
			return AIRegionProposedPurposeEnum::Tree;
		}
		if (fertility > 50) {
			return AIRegionProposedPurposeEnum::Fertility;
		}

		return AIRegionProposedPurposeEnum::None;
	}

	std::string AIPrintPrefix()
	{
		std::stringstream ss;
		ss << "[" << _aiPlayerId << "_" << _simulation->playerName(_aiPlayerId) << "]";
		return ss.str();
	}
	
private:
	int32 _aiPlayerId;
	IGameSimulationCore* _simulation;
	IPlayerSimulationInterface* _playerInterface;

	/*
	 * Serialize
	 */
	std::vector<AIRegionStatus> _regionStatuses;
	bool _active = false;

	// 1 relationship should cost around 20 gold
	std::vector<std::vector<int32>> _relationshipModifiers;
};
