#include "TileObjectDisplayComponent.h"
#include "../MapUtil.h"

#include "Materials/MaterialInstanceDynamic.h"
#include "Components/HierarchicalInstancedStaticMeshComponent.h"
#include "Components/LineBatchComponent.h"
#include "Engine/StaticMesh.h"
#include "PunCity/PunGameSettings.h"
#include "PunCity/GameRand.h"

#include "PunCity/PunUtils.h"

#include <string>
#include "PunCity/PunTimer.h"
#include "PunCity/Simulation/Buildings/GathererHut.h"

using namespace std;

DECLARE_CYCLE_STAT(TEXT("PUN: [Display]TileObjects_Move"), STAT_PunDisplayTreeTickMove, STATGROUP_Game);


int UTileObjectDisplayComponent::CreateNewDisplay(int32 objectId)
{
	LLM_SCOPE_(EPunSimLLMTag::PUN_DisplayTileObj);
	
	int meshId = _meshIdToMeshes.Num();

#if TILE_OBJ_CACHE
	return meshId;
#endif

	_meshIdToMeshes.Add(NewObject<UStaticFastInstancedMeshesComp>(this));
	_meshIdToMeshData.Add(FastInstancedMeshesData());
	
	//_meshIdToMeshes[meshId]->Init("TileObjects" + to_string(meshId) + "_", this, CoordinateConstants::TileIdsPerRegion / 16, "TileObjects", meshId, false);
	_meshIdToMeshes[meshId]->Init("TileObjects" + to_string(meshId) + "_", this, CoordinateConstants::TileIdsPerRegion / 16, "", meshId, false);

	for (int i = 0; i < TileObjEnumSize; i++)
	{
		TileObjInfo info = GetTileObjInfoInt(i);
		TArray<UStaticMesh*> meshAssets = _assetLoader->tileMeshAsset(info.treeEnum).assets;
		TArray<UMaterialInstanceDynamic*> dynamicMaterials = _assetLoader->tileMeshAsset(info.treeEnum).materials;

		if (dynamicMaterials.Num() == 0) {
			for (int j = 0; j < meshAssets.Num(); j++) {
				// Set rainfall texture
				dynamicMaterials.Add(UMaterialInstanceDynamic::Create(meshAssets[j]->GetMaterial(0), this));
				PUN_CHECK(_assetLoader->biomeTexture);
				dynamicMaterials[j]->SetTextureParameterValue("BiomeMap", _assetLoader->biomeTexture);
				_assetLoader->tileMeshAsset(info.treeEnum).materials = dynamicMaterials;
			}
		}
		PUN_CHECK(dynamicMaterials.Num() == meshAssets.Num());

		for (int j = 0; j < meshAssets.Num(); j++) {
			FString meshName = ToFString(info.name) + FString::FromInt(j);
			
			_meshIdToMeshes[meshId]->AddProtoMesh(meshName, meshAssets[j], dynamicMaterials[j], false, false);
			//_meshIdToMeshes[meshId]->AddProtoMesh(meshName, meshAssets[j], dynamicMaterials[j], true, false, castShadow);

			//auto mesh = _meshIdToMeshes[meshId]->GetMesh(meshName);
			//mesh->SetReceivesDecals(false);
		}
	}

	// Gather Mark
	_meshIdToMeshes[meshId]->AddProtoMesh("GatherMark", _assetLoader->GatherMarkMeshMaterial, nullptr, false, false);
	
	//_meshIdToGatherMarks.Add(NewObject<UStaticFastInstancedMesh>(this, UStaticFastInstancedMesh::StaticClass()));
	//_meshIdToGatherMarks[meshId]->Init(this, CoordinateConstants::TileIdsPerRegion);
	//_meshIdToGatherMarks[meshId]->SetMeshState({ FString(("GatherMark" + to_string(meshId)).c_str()), _assetLoader->GatherMarkMeshMaterial, nullptr, false, false});

	return meshId;
}

void UTileObjectDisplayComponent::OnSpawnDisplay(int32 regionId, int32 meshId, WorldAtom2 cameraAtom)
{
	LLM_SCOPE_(EPunSimLLMTag::PUN_DisplayTileObj);

#if TILE_OBJ_CACHE
	if (_objectIdToMeshes.Num() == 0) {
		_objectIdToMeshes.SetNum(GameMapConstants::TotalRegions);
	}
	if (!_objectIdToMeshes[regionId]) // Start caching during TrailerSession
	{
		if (PunSettings::TrailerSession)
		{
			static int32 count = 0;
			count++;
			PUN_LOG("TileObj Cache:%d", count);

			_objectIdToMeshes[regionId] = NewObject<UStaticFastInstancedMeshesComp>(this);
			_objectIdToMeshes[regionId]->Init("TileObjects" + to_string(regionId) + "_", this, CoordinateConstants::TileIdsPerRegion / 16, "", regionId, false);

			for (int i = 0; i < TileObjEnumSize; i++)
			{
				TileObjInfo info = GetTileObjInfoInt(i);
				TArray<UStaticMesh*> meshAssets = _assetLoader->tileMeshAsset(info.treeEnum).assets;
				TArray<UMaterialInstanceDynamic*> dynamicMaterials = _assetLoader->tileMeshAsset(info.treeEnum).materials;

				if (dynamicMaterials.Num() == 0) {
					for (int j = 0; j < meshAssets.Num(); j++) {
						// Set rainfall texture
						dynamicMaterials.Add(UMaterialInstanceDynamic::Create(meshAssets[j]->GetMaterial(0), this));
						PUN_CHECK(_assetLoader->biomeTexture);
						dynamicMaterials[j]->SetTextureParameterValue("BiomeMap", _assetLoader->biomeTexture);
						_assetLoader->tileMeshAsset(info.treeEnum).materials = dynamicMaterials;
					}
				}
				PUN_CHECK(dynamicMaterials.Num() == meshAssets.Num());

				for (int j = 0; j < meshAssets.Num(); j++) {
					FString meshName = ToFString(info.name) + FString::FromInt(j);
					_objectIdToMeshes[regionId]->AddProtoMesh(meshName, meshAssets[j], dynamicMaterials[j], false, false);
				}
			}
		}
	}
	else {
		_objectIdToMeshes[regionId]->SetVisibilityQuick(true);
	}
	
#else
	_meshIdToMeshes[meshId]->SetActive(true);
	//_meshIdToGatherMarks[meshId]->SetActive(true);


	// Refresh
	simulation().SetNeedDisplayUpdate(DisplayClusterEnum::Trees, regionId, true);
#endif
}

void UTileObjectDisplayComponent::UpdateDisplay(int32 regionId, int32 meshId, WorldAtom2 cameraAtom, bool justSpawned, bool justCreated)
{
	LLM_SCOPE_(EPunSimLLMTag::PUN_DisplayTileObj);

	//SCOPE_TIMER_("Tick TileObj");

	// TODO: don't need this anymore?
	//// Skip if this Tick already used too many microseconds
	//int32 nanoSecSinceTickStart = gameManager()->timeSinceTickStart();
	//if (nanoSecSinceTickStart > 1000 * 100) {
	//	_noRegionSkipThisTickYet = false;
	//	return;
	//}


	

	auto& sim = simulation();
	TreeSystem& treeSystem = sim.treeSystem();
	WorldRegion2 region(regionId);

#if TILE_OBJ_CACHE
	if (_objectIdToMeshes.Num() == 0) {
		return;
	}
	UStaticFastInstancedMeshesComp* meshes = _objectIdToMeshes[regionId];
	if (!meshes) {
		return;
	}
#else
	UStaticFastInstancedMeshesComp* meshes = _meshIdToMeshes[meshId];
#endif


	// Set mesh parent's location
	{
		SCOPE_CYCLE_COUNTER(STAT_PunDisplayTreeTickMove);
		//SCOPE_TIMER_("Tick TileObj Move inUse:%d total:%d", _meshIdToMeshes[meshId]->poolInUseNumber(), _meshIdToMeshes[meshId]->meshPool.Num());

		WorldAtom2 regionAtomLocation(region.x * CoordinateConstants::AtomsPerRegion,
			region.y * CoordinateConstants::AtomsPerRegion);
		FVector regionDisplayLocation = MapUtil::DisplayLocation(cameraAtom, regionAtomLocation) + FVector(5, 5, 0);

		meshes->SetRelativeLocation(regionDisplayLocation);
#if !TILE_OBJ_CACHE
		//_meshIdToGatherMarks[meshId]->SetRelativeLocation(regionDisplayLocation);
#endif
	}

	// Overlay highlights
	{
		OverlayType overlayType = gameManager()->GetOverlayType();
		int32 customDepth = overlayType == OverlayType::Gatherer ? 1 : 0;
		FString orangeMeshName = ToFString(GetTileObjInfo(TileObjEnum::Orange).name) + FString::FromInt(2);
		FString papayaMeshName = ToFString(GetTileObjInfo(TileObjEnum::Papaya).name) + FString::FromInt(2);
		FString cococutMeshName = ToFString(GetTileObjInfo(TileObjEnum::Coconut).name) + FString::FromInt(2);
		meshes->SetCustomDepth(orangeMeshName, customDepth);
		meshes->SetCustomDepth(papayaMeshName, customDepth);
		meshes->SetCustomDepth(cococutMeshName, customDepth);
	}


	TileArea area(region);

	/*
	 * Debug
	 */
	if (PunSettings::Settings["TreeGrid"] ||
		PunSettings::Settings["ShowPerlinLines"])
	{
		ULineBatchComponent* line = lineBatch();

		area.ExecuteOnArea_Tile([&](int16_t x, int16_t y)
		{
			WorldTile2 worldTile(x, y);
			int worldTileId = worldTile.tileId();

			if (PunSettings::Settings["TreeGrid"])
			{
				FVector start = MapUtil::DisplayLocation(cameraAtom, WorldTile2(x, y).worldAtom2());

				if (treeSystem.treeShade(worldTileId)) {
					line->DrawLine(start, start + FVector(0, 5, 10), FLinearColor::Gray, 100.0f, 1.0f, 10000);
				}

				if (treeSystem.tileObjEnum(worldTileId) != TileObjEnum::None)
				{
					TileObjInfo tileObjInfo = treeSystem.tileInfo(worldTileId);
					if (tileObjInfo.type == ResourceTileType::Tree) {
						float height = treeSystem.growthPercent(worldTileId) / 100.0f * 15 + 5;
						line->DrawLine(start, start + FVector(0, 0, height), FLinearColor(0, 0.5, 0), 100.0f, 1.0f, 10000);
					}
					// TODO: this is for debug
					else if (tileObjInfo.treeEnum == TileObjEnum::WheatBush) {
						float height = treeSystem.growthPercent(worldTileId) / 100.0f * 15 + 5;
						line->DrawLine(start, start + FVector(0, 0, height), FLinearColor::Yellow, 100.0f, 1.0f, 10000);
					}
					else if (tileObjInfo.type == ResourceTileType::Bush) {
						float height = treeSystem.growthPercent(worldTileId) / 100.0f * 15 + 5;
						line->DrawLine(start, start + FVector(0, 0, height), FLinearColor::Green, 100.0f, 1.0f, 10000);
					}
				}

				if (treeSystem.isOnReadyBushesList(worldTile)) {
					line->DrawLine(start, start + FVector(4, 4, 8), FLinearColor(1, 0, 1), 100.0f, 1.0f, 10000);
				}
				if (treeSystem.hasReservation(worldTile)) {
					line->DrawLine(start, start + FVector(-4, 4, 8), FLinearColor::Black, 100.0f, 1.0f, 10000);
				}

				if (treeSystem.hasFruit(worldTileId)) {
					line->DrawLine(start, start + FVector(0, 2, 20), FLinearColor::Red, 100.0f, 1.0f, 10000);
				}
			}

			if (PunSettings::Settings["ShowPerlinLines"])
			{
				//FVector start = MapUtil::DisplayLocation(cameraAtom, WorldTile2(x, y).worldAtom2());
				//float perlin = treeSystem.testPerlin[worldTileId];
				//FLinearColor color;
				//if (perlin < 0.6) color = FLinearColor::Green;
				//else if (perlin < 0.7) color = FLinearColor::Yellow;
				//else color = FLinearColor::Red;
				//lineBatch->DrawLine(start, start + FVector(0, 0, 50) * perlin, color, 100.0f, 1.0f, 10000);
			}
		});
	}

	/*
	 * Fallen Trees
	 */

	 //PUN_LOG("Start loop regionId:%d Tick:%d", regionId, Time::Ticks());
	if (_isFullDisplay)
	{
		//SCOPE_TIMER("Tick TileObj Fallen");

		unordered_map<int16_t, FallenTreeInfo>& localTileIdToFallenTree = treeSystem.localTileIdToFallenTree(regionId);
		for (auto& it : localTileIdToFallenTree) {
			int localTileId = it.first;
			int worldTileId = region.worldTile2(LocalTile2(localTileId)).tileId();

			FallenTreeInfo fallenInfo = it.second;
			int32_t fellTickElapsed = Time::Ticks() - fallenInfo.fellTick;
			TileObjInfo info = fallenInfo.tileInfo();
			int32_t ageTick = fallenInfo.tileObjAgeTick;
			check(info.treeEnum != TileObjEnum::None);

			FVector displayLocation = MapUtil::DisplayLocation(cameraAtom, WorldTile2(worldTileId).worldAtom2());
			if (fellTickElapsed >= TileObjInfo::FellRotateTicks) {
				displayLocation.Z -= (float)(fellTickElapsed - TileObjInfo::FellRotateTicks) / (TileObjInfo::FellAnimationTicks - TileObjInfo::FellRotateTicks) * 30.0f;
			}

			float xRotation = ((float)min(TileObjInfo::FellRotateTicks, fellTickElapsed) / TileObjInfo::FellRotateTicks) * 85;

			FTransform transform = GameDisplayUtils::GetTreeTransform(displayLocation, xRotation, worldTileId, ageTick, info);
			//PUN_LOG("regionId:%d tileId:%d fellTickElapsed %d", regionId, localTileId, fellTickElapsed);

			//PUN_LOG("localTileIdToFellTicks scale: %f, %f, %f, pos:%f,%f,%f", 
			//	transform.GetScale3D().X, transform.GetScale3D().Y, transform.GetScale3D().Z,
			//	transform.GetTranslation().X, transform.GetTranslation().Y, transform.GetTranslation().Z);

			// Note: Crash here might be forgetting to change TreeEnumSize
			_fallingMeshes->Add(ToFString(TileSubmeshName[(int32)TileSubmeshEnum::Trunk] + info.name), worldTileId, transform, 0);
			_fallingMeshes->Add(ToFString(TileSubmeshName[(int32)TileSubmeshEnum::Leaf] + info.name), worldTileId + 1 * GameMapConstants::TilesPerWorld, transform, 0);
		}
	}

	//PUN_LOG("End loop regionId:%d Tick:%d", regionId, Time::Ticks());

	// Update tree in the case plant should wither away
	if (sim.IsSnowStart()) {
		sim.SetNeedDisplayUpdate(DisplayClusterEnum::Trees, regionId, true);
	}

	
	/*
	 * Alive Trees
	 */
	 // Only update alive tree if needed
	if (sim.NeedDisplayUpdate(DisplayClusterEnum::Trees, regionId) || _displayStateChanged)
	{
		// Don't need to update anymore until something else change
		sim.SetNeedDisplayUpdate(DisplayClusterEnum::Trees, regionId, false);

		if (PunSettings::IsOn("TrailerNoTreeRefresh")) {
			return;
		}

		_chunkInfosToUpdate.push_back({ meshId, regionId, false, justSpawned });
	}
	
	_chunkInfosAll.push_back({ meshId, regionId, false, justSpawned });

	//// Hide meshes without clearing instances
	//meshes->SetVisibilityQuick(!_isHiddenDisplay);
}

void UTileObjectDisplayComponent::UpdateDisplay_PrepareReset(MeshChunkInfo& chunkInfo)
{
	//SCOPE_TIMER("Tick TileObj Alive");
		//SCOPE_TIMER_("TileObj - Trees %d %d full:%d", region.x, region.y, _isFullDisplay);

	//PUN_LOG("Mesh Start TileObj Alive");

	int32 regionId = chunkInfo.regionId;
	int32 meshId = chunkInfo.meshId;
	UStaticFastInstancedMeshesComp* meshes = _meshIdToMeshes[meshId];
	//meshes->shouldBatchAdd = chunkInfo.justSpawned;
	//meshes->BeforeAdd();
	//FastInstancedMeshesData* meshes = &(_meshIdToMeshData[meshId]);
	
	auto& sim = simulation();
	TreeSystem& treeSystem = sim.treeSystem();
	
	int32 playId = playerId();

	//meshes->ClearAddQueue();
	//_meshIdToGatherMarks[meshId]->ClearAddQueue();

	//PUN_ALOG_ALL("FastMesh", "ExecuteTileRegionStart ticks:%d regionId:%d", TimeDisplay::Ticks(), regionId);

	PunTerrainGenerator& terrainGenerator = sim.terrainGenerator();
	const std::vector<int16_t>& heightMap = terrainGenerator.GetHeightMap();
	const float flatHeight = FDToFloat(FlatLandHeight);

	GeoresourceNode georesourceNode = sim.georesourceSystem().georesourceNode(regionId);

	bool isHidingTree = gameManager()->isHidingTree();

	auto showBush = [&](TileObjInfo info, int32 worldTileId, FTransform transform, LocalTile2 localTile, int32 ageState)
	{
		// Plant uses the mesh array for multiple meshes (for example flower + its leaves)
		int32 submeshCount = _assetLoader->tileMeshAsset(info.treeEnum).assets.Num();
		for (int32 i = 0; i < submeshCount; i++)
		{
			// Special case grape frame
			// TODO: proper toggleInfo for this
			if (info.treeEnum == TileObjEnum::Grapevines) {
				if (i == 0) {
					meshes->Add(ToFString(info.name) + FString::FromInt(i), worldTileId, FTransform(FRotator::ZeroRotator, localTile.localDisplayLocation()), ageState, worldTileId);
				}
				else {
					FTransform grapeTransform = transform;
					grapeTransform.SetLocation(localTile.localDisplayLocation());
					grapeTransform.SetRotation(FQuat::Identity);
					meshes->Add(ToFString(info.name) + FString::FromInt(i), worldTileId, grapeTransform, ageState, worldTileId);
				}
				continue;
			}

			bool castShadow = (info.treeEnum != TileObjEnum::GrassGreen);

			meshes->Add(ToFString(info.name) + FString::FromInt(i), worldTileId, transform, ageState, worldTileId, castShadow);
		}
	};

	WorldRegion2 region(regionId);
	TileArea area(region);
	
	area.ExecuteOnArea_Tile([&](int16_t x, int16_t y)
	{
		WorldTile2 worldTile(x, y);
		int32 worldTileId = worldTile.tileId();
		LocalTile2 localTile = worldTile.localTile();
		int32 localTileId = localTile.tileId();
		TileObjInfo info = treeSystem.tileInfo(worldTileId);

		// Note: If hit here, Main Menu needs updating
		PUN_CHECK(IsTileObjEnumValid(info.treeEnum) || info.treeEnum == TileObjEnum::Fish);

		//PUN_LOG("ExecuteOnArea_Tile %d %d", x, y, localTileId);

		/*
		 * Show full farm
		 */
		Building* bld = sim.buildingAtTile(worldTile);
		if (PunSettings::CheatFullFarmRoad() &&
			bld && bld->isEnum(CardEnum::Farm))
		{
			Farm& farm = bld->subclass<Farm>(CardEnum::Farm);
			info = GetTileObjInfo(farm.currentPlantEnum);

			int32 ageTick = info.maxGrowthTick;
			int32 ageState = ageTick / TileObjInfo::TicksPerCycle();
			FTransform transform = GameDisplayUtils::GetBushTransform(localTile.localDisplayLocation(), 0, worldTileId, ageTick, info, terrainGenerator.GetBiome(worldTile));

			showBush(info, worldTileId, transform, localTile, ageState);
		}
		/*
		 * Show growing farm in trailer mode
		 */
		else if (PunSettings::TrailerMode() &&
			bld && bld->isEnum(CardEnum::Farm))
		{
			Farm& farm = bld->subclass<Farm>(CardEnum::Farm);
			info = GetTileObjInfo(farm.currentPlantEnum);

			// Age according to buildingAge 
			const int32 ticksToFullGrown = Time::TicksPerSecond * 10; // when buildingAge == ticksToFullGrown, 
			int32 ageTick = info.maxGrowthTick * farm.buildingAge() / ticksToFullGrown;
			int32 ageState = ageTick;
			FTransform transform = GameDisplayUtils::GetBushTransform(localTile.localDisplayLocation(), 0, worldTileId, ageTick, info, terrainGenerator.GetBiome(worldTile), false);

			showBush(info, worldTileId, transform, localTile, ageState);
		}
#if !TILE_OBJ_CACHE
		// Ores
		else if (terrainGenerator.terrainTileType(worldTileId) == TerrainTileType::Mountain && _isFullDisplay)
		{
			if (georesourceNode.HasResource() && georesourceNode.info().isMountainOre())
			{
				FloatDet heightFD = heightMap[worldTileId];
				float height = FDToFloat(heightFD);

				//float displayHeight = (height - flatHeight) / (1.0f - flatHeight) * CoordinateConstants::AboveFlatDisplayUnitHeight;
				float displayHeight = gameManager()->GetTerrainDisplayHeight(worldTile);

				if (displayHeight < 30.0f)
				{
					int32 rand = GameRand::DisplayRand(worldTileId);
					rand = GameRand::DisplayRand(worldTileId); // first rand might not be random enough

					if (rand % 100 > 50)
					{
						rand = GameRand::DisplayRand(worldTileId);
						float scale = 0.5f + 0.5f * static_cast<float>(rand % 100) / 100.0f;

						rand = GameRand::DisplayRand(rand);
						FRotator rotator(0, static_cast<float>(rand % 360), 0);

						//float scale = 1.0f;
						//FRotator rotator = FRotator::ZeroRotator;

						FVector displayLocation = localTile.localDisplayLocation();
						displayLocation.Z += displayHeight;

						// Random pos
						rand = GameRand::DisplayRand(rand);
						displayLocation.X += rand % 5;
						rand = GameRand::DisplayRand(rand);
						displayLocation.Y += rand % 5;

						//
						FString name = ToFString(GetTileObjInfo(georesourceNode.info().mountainOreEnum).name);

						FTransform transform(rotator, displayLocation, FVector(scale, scale, scale));
						meshes->Add(name + FString::FromInt(0), worldTileId, transform, 10, worldTileId);
					}
				}
			}
		}
#endif

		// Tree
		else if (info.type == ResourceTileType::Tree)
		{
			// Don't show tree if marked
			if (PunSettings::MarkedTreesNoDisplay &&
				treeSystem.HasMark(playId, worldTileId)) {
				return;
			}

			//FVector displayLocation = FVector(localTile.x * 10, localTile.y * 10, 0);
			int32 ageTick = treeSystem.tileObjAge(worldTileId);
			int32 ageState = ageTick / TileObjInfo::TicksPerCycle();

			FString tileObjectName = ToFString(info.name);


			FTransform transform = GameDisplayUtils::GetTreeTransform(localTile.localDisplayLocation(), 0, worldTileId, ageTick, info);

			// Show only stump
			if (isHidingTree) {
				meshes->Add(tileObjectName + FString::FromInt(static_cast<int32>(TileSubmeshEnum::Stump)), worldTileId + 1 * GameMapConstants::TilesPerWorld, transform, ageState, worldTileId);
				return;
			}


			if (!_isFullDisplay && !PunSettings::TrailerSession) {
				// Low Poly Leaf
				meshes->Add(tileObjectName + FString::FromInt(static_cast<int32>(TileSubmeshEnum::LeafLowPoly)), worldTileId, transform, ageState, worldTileId, false, false);
				meshes->Add(tileObjectName + FString::FromInt(static_cast<int32>(TileSubmeshEnum::LeafShadow)), worldTileId, transform, ageState, worldTileId, true, true);

				meshes->Add(tileObjectName + FString::FromInt(static_cast<int32>(TileSubmeshEnum::Trunk)), worldTileId + 1 * GameMapConstants::TilesPerWorld, transform, ageState, worldTileId);
				return;
			}

			// Trunk/Leaves
			meshes->Add(tileObjectName + FString::FromInt(static_cast<int32>(TileSubmeshEnum::Leaf)), worldTileId, transform, ageState, worldTileId, false, false);
			meshes->Add(tileObjectName + FString::FromInt(static_cast<int32>(TileSubmeshEnum::LeafShadow)), worldTileId, transform, ageState, worldTileId, true, true);

			meshes->Add(tileObjectName + FString::FromInt(static_cast<int32>(TileSubmeshEnum::Trunk)), worldTileId + 1 * GameMapConstants::TilesPerWorld, transform, ageState, worldTileId);


			// Fruit
			if (treeSystem.hasFruit(worldTileId)) {
				//PUN_ALOG("FastMesh", worldTileId, "[%s]FruitAdd.Begin ticks:%d id:%d", ToTChar(TileSubmeshName[int(TileSubmeshEnum::Fruit)]), TimeDisplay::Ticks(), worldTileId);
				meshes->Add(tileObjectName + FString::FromInt(static_cast<int32>(TileSubmeshEnum::Fruit)), worldTileId + 2 * GameMapConstants::TilesPerWorld, transform, ageState, worldTileId);
			}

#if !TILE_OBJ_CACHE
			// See if it is marked for gather
			//  Don't show mark if within building area
			if (!isMainMenuDisplay &&
				treeSystem.HasMark(playId, worldTileId) &&
				!sim.tileHasBuilding(worldTile))
			{
				float hoverHeight = GameDisplayUtils::TileObjHoverMeshHeight(info, worldTileId, ageTick);
				FVector translation = transform.GetTranslation();
				translation.Z += hoverHeight;
				transform.SetTranslation(translation);

				meshes->Add("GatherMark", worldTileId + 3 * GameMapConstants::TilesPerWorld, transform, ageState, worldTileId);
				//_meshIdToGatherMarks[meshId]->QueueAdd(worldTileId, transform, ageState);
			}
#endif
		}

		// Deposit
		else if (info.type == ResourceTileType::Deposit)
		{
			FVector displayLocation = localTile.localDisplayLocation();

			int32 variationIndex;
			int32 variationCount = _assetLoader->tileMeshAsset(info.treeEnum).assets.Num();
			FTransform transform = GameDisplayUtils::GetDepositTransform(worldTileId, displayLocation, variationCount, variationIndex);

			//// Stone uses the mesh array for random variations..
			//rand = GameRand::DisplayRand(rand);
			//int32 variationIndex = rand % variationCount;
			//
			meshes->Add(ToFString(info.name) + FString::FromInt(variationIndex), worldTileId, transform, 0, worldTileId);

#if !TILE_OBJ_CACHE
			// See if it is marked for gather
			if (!isMainMenuDisplay &&
				treeSystem.HasMark(playId, worldTileId))
			{
				float hoverHeight = GameDisplayUtils::TileObjHoverMeshHeight(info, worldTileId, 1);
				FVector translation = transform.GetTranslation();
				translation.Z += hoverHeight;
				transform.SetTranslation(translation);

				meshes->Add("GatherMark", worldTileId + 3 * GameMapConstants::TilesPerWorld, transform, 0, worldTileId);
				//_meshIdToGatherMarks[meshId]->QueueAdd(worldTileId, transform, 0);
			}
#endif
		}

		// Bush
		else if (info.type == ResourceTileType::Bush &&
			_isFullDisplay)
		{
			if (isHidingTree && sim.buildingIdAtTile(worldTile) == -1) {
				return;
			}

			// Don't show grass on road construction
			if (sim.isInitialized() &&
				sim.IsRoadTile(worldTile)) {
				return;
			}

			int32 ageTick = treeSystem.tileObjAge(worldTileId);
			int32 ageState = ageTick / TileObjInfo::TicksPerCycle();
			FTransform transform = GameDisplayUtils::GetBushTransform(localTile.localDisplayLocation(), 0, worldTileId, ageTick, info, terrainGenerator.GetBiome(worldTile));


			showBush(info, worldTileId, transform, localTile, ageState);
		}

	});

	//meshes->AfterAdd();
	
	//PUN_ALOG_ALL("FastMesh", "ExecuteTileRegionEnd ticks:%d regionId:%d", TimeDisplay::Ticks(), regionId);
}

void UTileObjectDisplayComponent::UpdateDisplay_UpdateMesh(MeshChunkInfo& chunkInfo)
{
	int32 meshId = chunkInfo.meshId;
	UStaticFastInstancedMeshesComp* meshes = _meshIdToMeshes[meshId];

	// Execute
	//meshes->ExecuteAddQueue(_meshIdToMeshData[meshId].meshesToAdd);
	
	// After Add
	meshes->AfterAdd();
	
//#if !TILE_OBJ_CACHE
//	_meshIdToGatherMarks[meshId]->ExecuteAddQueue();
//	_meshIdToGatherMarks[meshId]->AfterAdd();
//#endif
}

void UTileObjectDisplayComponent::UpdateDisplay_FinishAll(MeshChunkInfo& chunkInfo)
{
	UStaticFastInstancedMeshesComp* meshes = _meshIdToMeshes[chunkInfo.meshId];

	// Hide meshes without clearing instances
	meshes->SetVisibilityQuick(!_isHiddenDisplay);
}


void UTileObjectDisplayComponent::HideDisplay(int32 meshId, int32 regionId)
{
	LLM_SCOPE_(EPunSimLLMTag::PUN_DisplayTileObj);
#if TILE_OBJ_CACHE
	if (_objectIdToMeshes.Num() > 0 && _objectIdToMeshes[regionId]) {
		_objectIdToMeshes[regionId]->SetVisibilityQuick(false);
	}
#else
	
	_meshIdToMeshes[meshId]->SetActive(false);
	//_meshIdToGatherMarks[meshId]->SetActive(false);
#endif
}