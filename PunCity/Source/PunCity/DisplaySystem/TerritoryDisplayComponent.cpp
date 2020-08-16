// Fill out your copyright notice in the Description page of Project Settings.

#include "TerritoryDisplayComponent.h"
#include "../MapUtil.h"
#include "PunCity/GameConstants.h"
#include "../Simulation/GameRegionSystem.h"

using namespace std;

enum class TerritoryCornerEnum
{
	None,
	Convex,
	Concave,
	BendoutVertical,
	BendoutHorizontal,
	StraightVertical,
	StraightHorizontal,
};


static float TerritoryCornerEnumToFloat(TerritoryCornerEnum cornerEnum)
{
	static const float shift = 0.02;
	if (cornerEnum == TerritoryCornerEnum::Convex) {
		return 0.7 + shift;
	}
	if (cornerEnum == TerritoryCornerEnum::Concave) {
		return 0.6 + shift;
	}
	if (cornerEnum == TerritoryCornerEnum::BendoutVertical) {
		return 0.5 + shift;
	}
	if (cornerEnum == TerritoryCornerEnum::BendoutHorizontal) {
		return 0.4 + shift;
	}
	if (cornerEnum == TerritoryCornerEnum::StraightVertical) {
		return 0.3 + shift;
	}
	if (cornerEnum == TerritoryCornerEnum::StraightHorizontal) {
		return 0.2 + shift;
	}
	return 0.0;
}

void FTerritoryDecals::AddNewDecal(int32_t playerId, USceneComponent* parent, UAssetLoaderComponent* assetLoader)
{
	playerIdToDecalIndex.Add(playerId, decals.Num());
	
	// TODO: refactor to use PunUnrealUtils
	UDecalComponent* decal = NewObject<UDecalComponent>(parent);
	decal->AttachToComponent(parent, FAttachmentTransformRules::KeepRelativeTransform, NAME_None);
	decal->RegisterComponent();
	decals.Add(decal);

	UMaterialInstanceDynamic* material = UMaterialInstanceDynamic::Create(assetLoader->TerritoryMaterial, parent);
	decalMaterials.Add(material);

	decal->SetMaterial(0, material);
	decal->DecalSize = FVector(512, 1024, 1024);
	decal->SetRelativeRotation(FVector(0, 0, -1).Rotation()); // Rotate to project the decal down

	{
		UTexture2D* texture = UTexture2D::CreateTransient(GameMapConstants::RegionsPerWorldX, GameMapConstants::RegionsPerWorldY, EPixelFormat::PF_B8G8R8A8);
		texture->Filter = TextureFilter::TF_Nearest;
		texture->AddToRoot();
		texture->UpdateResource();
		_adjacentTerritoriesTextures.Add(texture);
		material->SetTextureParameterValue(TEXT("TerritoryTexture"), texture);
	}
	{
		UTexture2D* texture = UTexture2D::CreateTransient(GameMapConstants::RegionsPerWorldX, GameMapConstants::RegionsPerWorldY, EPixelFormat::PF_B8G8R8A8);
		texture->Filter = TextureFilter::TF_Nearest;
		texture->AddToRoot();
		texture->UpdateResource();
		_adjacentTerritoriesTextures2.Add(texture);
		material->SetTextureParameterValue(TEXT("TerritoryTexture2"), texture);
	}

	//PUN_LOG("_adjacentTerritoriesTextures size:%d, %d", texture->GetSizeX(), texture->GetSizeY());

	material->SetScalarParameterValue(TEXT("DisplayUnitPerRegion"), CoordinateConstants::DisplayUnitPerRegion);
	material->SetScalarParameterValue(TEXT("DisplayUnitPerWorldX"), GameMapConstants::RegionsPerWorldX * CoordinateConstants::DisplayUnitPerRegion);
	material->SetScalarParameterValue(TEXT("DisplayUnitPerWorldY"), GameMapConstants::RegionsPerWorldY * CoordinateConstants::DisplayUnitPerRegion);
}

static std::vector<uint32> territoryAdjacencyEncoding;
static std::vector<uint32> territoryAdjacencyEncoding2;

void UTerritoryDisplayComponent::Display(std::vector<int>& sampleProvinceIds)
{
	if (!PunSettings::IsOn("DisplayTerritory")) {
		for (int i = 0; i < playerDecals.decalMaterials.Num(); i++) {
			playerDecals.decals[i]->SetVisibility(false);
		}
		return;
	}
	
	// Update
	for (int i = 0; i < playerDecals.decalMaterials.Num(); i++) {
		playerDecals.decals[i]->SetWorldScale3D(FVector::OneVector * MapUtil::GlobalDecalZoomFactor(gameManager()->zoomDistance()));
	}
	
	//_regionBorderDecal->SetVisibility(gameManager()->ZoomDistanceBelow(WorldToMapZoomAmount));

	// TODO: refactor... decal should be resized according to region extent?? May be not anymore??

	// Hide any decal that is not shown
	TileArea sampleArea = gameManager()->sampleArea();
	bool isMapMode = !gameManager()->ZoomDistanceBelow(WorldToMapZoomAmount);
	//int decalCount = 0;
	simulation().ExecuteOnPlayersAndAI([&](int32_t playerId) {
		if (playerDecals.Contains(playerId)) {
			bool shouldDisplay = isMapMode || sampleArea.HasOverlap(simulation().playerOwned(playerId).territoryBoxExtent());
			playerDecals.GetDecal(playerId)->SetVisibility(shouldDisplay);
			//decalCount += shouldDisplay;
		}
	});
	//PUN_LOG("decalCount %d", decalCount);



	/*
	 * Territory
	 */

	// Filter out player territory
	std::vector<int32> sampleProvinceIdsNonPlayer;
	for (int32 provinceId : sampleProvinceIds) {
		if (simulation().provinceOwner(provinceId) == -1) {
			sampleProvinceIdsNonPlayer.push_back(provinceId);
		}
	}
	
	auto& provinceSys = simulation().provinceSystem();
	
	//std::vector<int32> provinceIdsToDisplayInner;
	//for (int32 provinceId : sampleProvinceIdsNonPlayer) {
	//	provinceIdsToDisplayInner.push_back(provinceId);
	//}

	// Helper Func
	auto showTerritoryMeshes = [&](TArray<UTerritoryMeshComponent*>& territoryMeshes, TArray<UTerritoryMeshComponent*>& pool, std::vector<int32>& provinceIds, bool isInnerMesh)
	{
		// Find ones we can reuse from last tick
		TArray<UTerritoryMeshComponent*> reuseMeshes;
		for (UTerritoryMeshComponent* territoryMesh : territoryMeshes)
		{
			if (CppUtils::TryRemove(provinceIds, territoryMesh->provinceId)) {
				reuseMeshes.Add(territoryMesh);
			}
			else {
				territoryMesh->provinceId = -1;
				territoryMesh->SetVisibility(false, true);
				pool.Add(territoryMesh);

				//PUN_LOG("Despawn territory %d pool:%d", territoryMesh->provinceId, pool.Num());
			}
		}

		territoryMeshes = reuseMeshes;

		// Add any additional meshes
		for (int32 provinceId : provinceIds)
		{
			if (pool.Num() > 0)
			{
				auto comp = pool.Pop();
				comp->SetVisibility(true, true);
				territoryMeshes.Add(comp);

				//PUN_LOG("Spawn territory %d pool:%d", territoryMeshes.Num(), pool.Num());
			}
			else
			{
				auto comp = CreateTerritoryMeshComponent(true);
				territoryMeshes.Add(comp);

				PUN_LOG("Create territory %d pool:%d", territoryMeshes.Num(), pool.Num());
			}

			territoryMeshes.Last()->UpdateMesh(true, provinceId, -1, isInnerMesh, &simulation());
		}
	};

	/*
	 * Outer Display
	 */
	showTerritoryMeshes(_provinceMeshes, _provinceMeshesPool, sampleProvinceIdsNonPlayer, false);

	WorldAtom2 cameraAtom = gameManager()->cameraAtom();
	for (UTerritoryMeshComponent* comp : _provinceMeshes)
	{
		WorldTile2 centerTile = provinceSys.GetProvinceCenter(comp->provinceId).worldTile2();
		FVector displayLocation = MapUtil::DisplayLocation(cameraAtom, centerTile.worldAtom2());
		comp->SetRelativeLocation(displayLocation);

		auto material = _gameManager->ZoomDistanceBelow(WorldZoomTransition_GameToMap) ? comp->MaterialInstance : comp->MaterialInstance_Top;

		// Fade when it is not near our territory
		int32 owner = simulation().provinceOwner(comp->provinceId);
		material->SetScalarParameterValue("IsFade", (owner == -1) && !simulation().IsProvinceNextToPlayer(comp->provinceId, playerId()));

		comp->SetMaterial(0, material);
	}

	/*
	 * Territory Update
	 */
	{
		// Above WorldToMapZoom, where sampleIds are 0, we show all player territory
		std::vector<int32> sampleTerritoryPlayerIds;
		
		if (_gameManager->ZoomDistanceBelow(WorldToMapZoomAmount))
		{
			for (int32 provinceId : sampleProvinceIds)
			{
				int32 playerId = simulation().provinceOwner(provinceId);
				if (playerId != -1) {
					CppUtils::TryAdd(sampleTerritoryPlayerIds, playerId);
				}
			}
		}
		else
		{
			for (int32 i = 0; i < _playerIdToTerritoryMesh.Num(); i++) {
				sampleTerritoryPlayerIds.push_back(i);
			}
		}

		/*
		 * Show Sample Territory Mesh
		 */
		for (int32 playerId = 0; playerId < _playerIdToTerritoryMesh.Num(); playerId++)
		{
			auto& playerOwned = simulation().playerOwned(playerId);
			if (playerOwned.hasChosenLocation())
			{
				PUN_CHECK(playerOwned.provincesClaimed().size() > 0);
				WorldTile2 centerTile = simulation().GetProvinceCenterTile(playerOwned.provincesClaimed()[0]);
				FVector displayLocation = MapUtil::DisplayLocation(cameraAtom, centerTile.worldAtom2());

				auto comp = _playerIdToTerritoryMesh[playerId];
				if (comp)
				{
					if (!SimSettings::IsOn("TrailerSession") &&
						CppUtils::Contains(sampleTerritoryPlayerIds, playerId)) 
					{
						comp->SetVisibility(true);
						comp->SetRelativeLocation(displayLocation);

						auto material = _gameManager->ZoomDistanceBelow(WorldZoomTransition_GameToMap) ? comp->MaterialInstance : comp->MaterialInstance_Top;
						material->SetVectorParameterValue("PlayerColor1", playerId != -1 ? PlayerColor1(playerId) : FLinearColor(0, 0, 0, 0));
						material->SetVectorParameterValue("PlayerColor2", playerId != -1 ? PlayerColor2(playerId) : FLinearColor(0, 0, 0, 0));
						comp->SetMaterial(0, material);
					}
					else {
						comp->SetVisibility(false);
					}
				}
			}
		}

		/*
		 * Update meshes
		 */
		std::vector<int32> playerIds = simulation().GetNeedDisplayUpdateIds(DisplayGlobalEnum::Territory);
		for (int32 playerId : playerIds) 
		{
			PUN_LOG("Update TerritoryMesh playerId:%d", playerId);
			
			if (_playerIdToTerritoryMesh.Num() <= playerId) {
				_playerIdToTerritoryMesh.SetNum(playerId + 1);
			}

			if (simulation().playerOwned(playerId).hasChosenLocation()) 
			{
				if (!_playerIdToTerritoryMesh[playerId]) {
					_playerIdToTerritoryMesh[playerId] = CreateTerritoryMeshComponent(false);
				}
				
				_playerIdToTerritoryMesh[playerId]->UpdateMesh(true, -1, playerId, false, &simulation(), 50);
				_playerIdToTerritoryMesh[playerId]->SetVisibility(true);
			} else {
				if (_playerIdToTerritoryMesh[playerId]) {
					_playerIdToTerritoryMesh[playerId]->SetVisibility(false);
				}
			}
		}
	}
	
	// TODO: remove don't need territory decal anymore?
	//// After this point, update on dirty only
	//if (!simulation().NeedDisplayUpdate(DisplayGlobalEnum::Territory)) {
	//	return;
	//}
	//PUN_LOG("Need Display Territory update");
	//simulation().SetNeedDisplayUpdate(DisplayGlobalEnum::Territory, false);

	//
	//GameRegionSystem& regionSystem = simulation().regionSystem();

	//std::vector<int32>& territoryOwnerMap = regionSystem.territoryOwnerMap();
	//territoryAdjacencyEncoding.resize(GameMapConstants::TotalRegions);
	//territoryAdjacencyEncoding2.resize(GameMapConstants::TotalRegions);


	//int maxRegionX = GameMapConstants::RegionsPerWorldX - 1;
	//int maxRegionY = GameMapConstants::RegionsPerWorldY - 1;

	//// Display for each player
	//simulation().ExecuteOnPlayersAndAI([&](int32 playerId) {
	//	DisplayPlayerId(playerId, territoryOwnerMap, playerDecals);
	//});

}

// TODO: Remove this
void UTerritoryDisplayComponent::DisplayPlayerId(int32 playerId, std::vector<int32>& territoryOwnerMap, FTerritoryDecals& decals)
{
	//PUN_LOG("DisplayPlayerId %d", playerId);
	
	if (!decals.Contains(playerId)) {
		decals.AddNewDecal(playerId, this, _assetLoader);
	}

	for (int yy = 0; yy < GameMapConstants::RegionsPerWorldY; yy++)
	{
		for (int xx = 0; xx < GameMapConstants::RegionsPerWorldX; xx++)
		{
			int32_t xx_p = xx + 1;
			int32_t xx_n = xx - 1;
			int32_t yy_p = yy + 1;
			int32_t yy_n = yy - 1;

			WorldRegion2 region(xx, yy);
			bool isPlayer = territoryOwnerMap[region.regionId()] == playerId;

			WorldRegion2 regionN(xx + 1, yy);
			WorldRegion2 regionS(xx - 1, yy);
			WorldRegion2 regionE(xx, yy + 1);
			WorldRegion2 regionW(xx, yy - 1);

			WorldRegion2 regionNE(xx + 1, yy + 1);
			WorldRegion2 regionNW(xx + 1, yy - 1);
			WorldRegion2 regionSE(xx - 1, yy + 1);
			WorldRegion2 regionSW(xx - 1, yy - 1);

			bool isPlayer_N = regionN.IsValid() && territoryOwnerMap[regionN.regionId()] == playerId;
			bool isPlayer_S = regionS.IsValid() && territoryOwnerMap[regionS.regionId()] == playerId;
			bool isPlayer_E = regionE.IsValid() && territoryOwnerMap[regionE.regionId()] == playerId;
			bool isPlayer_W = regionW.IsValid() && territoryOwnerMap[regionW.regionId()] == playerId;

			bool isPlayer_NE = regionNE.IsValid() && territoryOwnerMap[regionNE.regionId()] == playerId;
			bool isPlayer_NW = regionNW.IsValid() && territoryOwnerMap[regionNW.regionId()] == playerId;
			bool isPlayer_SE = regionSE.IsValid() && territoryOwnerMap[regionSE.regionId()] == playerId;
			bool isPlayer_SW = regionSW.IsValid() && territoryOwnerMap[regionSW.regionId()] == playerId;

			// Fill in the corner types
			TerritoryCornerEnum corner_NE = TerritoryCornerEnum::None;
			if (isPlayer)
			{
				if (!isPlayer_N && !isPlayer_E) {
					corner_NE = TerritoryCornerEnum::Convex;
				}
				if (isPlayer_N && isPlayer_NE && !isPlayer_E) {
					corner_NE = TerritoryCornerEnum::BendoutVertical;
				}
				if (!isPlayer_N && isPlayer_NE && isPlayer_E) {
					corner_NE = TerritoryCornerEnum::BendoutHorizontal;
				}
				if (isPlayer_N && !isPlayer_NE && !isPlayer_E) {
					corner_NE = TerritoryCornerEnum::StraightVertical;
				}
				if (!isPlayer_N && !isPlayer_NE && isPlayer_E) {
					corner_NE = TerritoryCornerEnum::StraightHorizontal;
				}
			}
			else
			{
				if (isPlayer_N && isPlayer_NE && isPlayer_E) {
					corner_NE = TerritoryCornerEnum::Concave;
				}
			}

			TerritoryCornerEnum corner_NW = TerritoryCornerEnum::None;
			if (isPlayer)
			{
				if (!isPlayer_N && !isPlayer_W) {
					corner_NW = TerritoryCornerEnum::Convex;
				}
				if (isPlayer_N && isPlayer_NW && !isPlayer_W) {
					corner_NW = TerritoryCornerEnum::BendoutVertical;
				}
				if (!isPlayer_N && isPlayer_NW && isPlayer_W) {
					corner_NW = TerritoryCornerEnum::BendoutHorizontal;
				}
				if (isPlayer_N && !isPlayer_NW && !isPlayer_W) {
					corner_NW = TerritoryCornerEnum::StraightVertical;
				}
				if (!isPlayer_N && !isPlayer_NW && isPlayer_W) {
					corner_NW = TerritoryCornerEnum::StraightHorizontal;
				}
			}
			else
			{
				if (isPlayer_N && isPlayer_NW && isPlayer_W) {
					corner_NW = TerritoryCornerEnum::Concave;
				}
			}

			TerritoryCornerEnum corner_SE = TerritoryCornerEnum::None;
			if (isPlayer)
			{
				if (!isPlayer_S && !isPlayer_E) {
					corner_SE = TerritoryCornerEnum::Convex;
				}
				if (isPlayer_S && isPlayer_SE && !isPlayer_E) {
					corner_SE = TerritoryCornerEnum::BendoutVertical;
				}
				if (!isPlayer_S && isPlayer_SE && isPlayer_E) {
					corner_SE = TerritoryCornerEnum::BendoutHorizontal;
				}
				if (isPlayer_S && !isPlayer_SE && !isPlayer_E) {
					corner_SE = TerritoryCornerEnum::StraightVertical;
				}
				if (!isPlayer_S && !isPlayer_SE && isPlayer_E) {
					corner_SE = TerritoryCornerEnum::StraightHorizontal;
				}
			}
			else
			{
				if (isPlayer_S && isPlayer_SE && isPlayer_E) {
					corner_SE = TerritoryCornerEnum::Concave;
				}
			}

			TerritoryCornerEnum corner_SW = TerritoryCornerEnum::None;
			if (isPlayer)
			{
				if (!isPlayer_S && !isPlayer_W) {
					corner_SW = TerritoryCornerEnum::Convex;
				}
				if (isPlayer_S && isPlayer_SW && !isPlayer_W) {
					corner_SW = TerritoryCornerEnum::BendoutVertical;
				}
				if (!isPlayer_S && isPlayer_SW && isPlayer_W) {
					corner_SW = TerritoryCornerEnum::BendoutHorizontal;
				}
				if (isPlayer_S && !isPlayer_SW && !isPlayer_W) {
					corner_SW = TerritoryCornerEnum::StraightVertical;
				}
				if (!isPlayer_S && !isPlayer_SW && isPlayer_W) {
					corner_SW = TerritoryCornerEnum::StraightHorizontal;
				}
			}
			else
			{
				if (isPlayer_S && isPlayer_SW && isPlayer_W) {
					corner_SW = TerritoryCornerEnum::Concave;
				}
			}

			bool hasEdge_N = isPlayer && !isPlayer_N;
			bool hasEdge_S = isPlayer && !isPlayer_S;
			bool hasEdge_E = isPlayer && !isPlayer_E;
			bool hasEdge_W = isPlayer && !isPlayer_W;

			float packedEdge_NS = (hasEdge_N ? 0.53f : 0.0f) + (hasEdge_S ? 0.253f : 0.0f);
			float packedEdge_EW = (hasEdge_E ? 0.53f : 0.0f) + (hasEdge_W ? 0.253f : 0.0f);

			FLinearColor color(TerritoryCornerEnumToFloat(corner_NE),
				TerritoryCornerEnumToFloat(corner_NW),
				packedEdge_NS,
				0);

			FLinearColor color2(TerritoryCornerEnumToFloat(corner_SE),
				TerritoryCornerEnumToFloat(corner_SW),
				packedEdge_EW,
				0);

			//FColor color(0, 255, 0, 255);
			territoryAdjacencyEncoding[region.regionId()] = color.ToFColor(true).ToPackedARGB();
			territoryAdjacencyEncoding2[region.regionId()] = color2.ToFColor(true).ToPackedARGB();
		}
	}

	// Refresh _territoryTexture for shader
	// Note: Only refreshes when territory changed
	{
		UTexture2D* texture = decals.adjacentTerritoriesTexture(playerId);
		uint32* textureData = reinterpret_cast<uint32*>(texture->PlatformData->Mips[0].BulkData.Lock(LOCK_READ_WRITE));

		const size_t Size = texture->GetSizeX() * texture->GetSizeY() * sizeof(uint32);
		FMemory::Memcpy(textureData, territoryAdjacencyEncoding.data(), Size);

		texture->PlatformData->Mips[0].BulkData.Unlock();
		texture->UpdateResource();
		decals.GetDecalMaterial(playerId)->SetTextureParameterValue(TEXT("TerritoryTexture"), texture);
	}
	{
		UTexture2D* texture = decals.adjacentTerritoriesTextures2(playerId);
		uint32* textureData = reinterpret_cast<uint32*>(texture->PlatformData->Mips[0].BulkData.Lock(LOCK_READ_WRITE));

		const size_t Size = texture->GetSizeX() * texture->GetSizeY() * sizeof(uint32);
		FMemory::Memcpy(textureData, territoryAdjacencyEncoding2.data(), Size);

		texture->PlatformData->Mips[0].BulkData.Unlock();
		texture->UpdateResource();
		decals.GetDecalMaterial(playerId)->SetTextureParameterValue(TEXT("TerritoryTexture2"), texture);
	}


	{
		// Reset color... Color may be changed if lord changed
		if (simulation().playerChoseLocation(playerId))
		{
			int32 lordPlayerId = simulation().townhall(playerId).armyNode.lordPlayerId;

			auto material = decals.GetDecalMaterial(playerId);
			material->SetVectorParameterValue(TEXT("PlayerColor1"), PlayerColor1(lordPlayerId));
			material->SetVectorParameterValue(TEXT("PlayerColor2"), PlayerColor2(lordPlayerId));
		}
	}
}