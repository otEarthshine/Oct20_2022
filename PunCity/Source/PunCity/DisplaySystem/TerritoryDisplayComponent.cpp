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
	/*
	 * Show province connections
	 */

	

	/*
	 * Territory
	 */

	// Filter out player territory
	std::vector<int32> sampleProvinceIdsNonPlayer;

	if (gameManager()->isShowingProvinceOverlay()) {
		// ProvinceOverlay skip removing province meshes in player's territory
		sampleProvinceIdsNonPlayer = sampleProvinceIds;
	}
	else {
		// Show non-player's province only
		for (int32 provinceId : sampleProvinceIds) {
			if (simulation().provinceOwnerTown(provinceId) == -1) {
				sampleProvinceIdsNonPlayer.push_back(provinceId);
			}
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

				//PUN_LOG("Create territory %d pool:%d", territoryMeshes.Num(), pool.Num());
			}

			territoryMeshes.Last()->UpdateMesh(true, provinceId, -1, -1, isInnerMesh, &simulation());
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
		int32 provinceTownId = simulation().provinceOwnerTown(comp->provinceId);
		bool isFade = false; // Fade when province has no owner, and province is not next to player
		if (provinceTownId == -1) {
			// Is Province next to player???
			const auto& townIds = simulation().GetTownIds(playerId());
			bool isNextToPlayer = false;
			for (int32 townId : townIds) {
				if (simulation().IsProvinceNextToTown(comp->provinceId, townId)) {
					isNextToPlayer = true;
					break;
				}
			}
			if (!isNextToPlayer) {
				isFade = true;
			}
		}
		material->SetScalarParameterValue("IsFade", isFade);
		//material->SetScalarParameterValue("IsFade", (owner == -1) && !simulation().IsProvinceNextToPlayer(comp->provinceId, playerId()));

		comp->SetMaterial(0, material);
	}


	/*
	 * Territory Update
	 */
	{
		// Above WorldToMapZoom, where sampleIds are 0, we show all player territory
		std::vector<int32> sampleTerritoryTownIds;
		
		if (_gameManager->ZoomDistanceBelow(WorldToMapZoomAmount))
		{
			for (int32 provinceId : sampleProvinceIds)
			{
				int32 townId = simulation().provinceOwnerTown(provinceId);
				if (townId != -1) {
					CppUtils::TryAdd(sampleTerritoryTownIds, townId);
				}
			}
		}
		else
		{
			for (int32 i = 0; i < _townIdToTerritoryMesh.Num(); i++) {
				sampleTerritoryTownIds.push_back(i);
			}
		}

		/*
		 * Show Sample Territory Mesh
		 */
		for (int32 townId = 0; townId < _townIdToTerritoryMesh.Num(); townId++)
		{
			auto& sim = simulation();
			auto& playerOwned = sim.playerOwnedFromTownId(townId);
			auto& townManage = sim.townManager(townId);
			
			if (playerOwned.hasChosenLocation())
			{
				PUN_CHECK(townManage.provincesClaimed().size() > 0);
				WorldTile2 centerTile = simulation().GetProvinceCenterTile(townManage.provincesClaimed()[0]);
				FVector displayLocation = MapUtil::DisplayLocation(cameraAtom, centerTile.worldAtom2());

				TArray<UTerritoryMeshComponent*>& comps = _townIdToTerritoryMesh[townId].TerritoryMeshes;
				for (auto& comp : comps)
				{
					if (!PunSettings::TrailerSession &&
						CppUtils::Contains(sampleTerritoryTownIds, townId)) 
					{
						comp->SetVisibility(true);
						comp->SetRelativeLocation(displayLocation);

						// If there is a lord, use lord's color instead
						int32 paintPlayerId = townManage.playerId();
						if (playerOwned.lordPlayerId() != -1) {
							paintPlayerId = playerOwned.lordPlayerId();
						}

						auto material = _gameManager->ZoomDistanceBelow(WorldZoomTransition_GameToMap) ? comp->MaterialInstance : comp->MaterialInstance_Top;
						material->SetVectorParameterValue("PlayerColor1", paintPlayerId != -1 ? PlayerColor1(paintPlayerId) : FLinearColor(0, 0, 0, 0));
						material->SetVectorParameterValue("PlayerColor2", paintPlayerId != -1 ? PlayerColor2(paintPlayerId) : FLinearColor(0, 0, 0, 0));
						comp->SetMaterial(0, material);
					}
					else {
						comp->SetVisibility(false);
					}
				}
			}
		}


		/*
		 * Update territory meshes
		 */
		std::vector<int32> townIds = simulation().GetNeedDisplayUpdateIds(DisplayGlobalEnum::Territory);
		for (int32 townId : townIds)
		{
			//PUN_LOG("Update TerritoryMesh playerId:%d", playerId);
			
			if (_townIdToTerritoryMesh.Num() <= townId) {
				_townIdToTerritoryMesh.SetNum(townId + 1);
			}

			TArray<UTerritoryMeshComponent*>& territoryMeshes = _townIdToTerritoryMesh[townId].TerritoryMeshes;

			if (simulation().HasChosenLocation(simulation().townPlayerId(townId)))
			{
				const auto& clusters = simulation().provinceSystem().GetTerritoryClusters(townId);
				for (int32 i = 0; i < clusters.size(); i++)
				{
					if (territoryMeshes.Num() < clusters.size()) {
						territoryMeshes.Add(CreateTerritoryMeshComponent(false));
					}

					territoryMeshes[i]->UpdateMesh(true, -1, townId, i, false, &simulation(), 50);
					territoryMeshes[i]->SetVisibility(false); // Hide initially until the colors get updated.
				}

				for (int32 j = clusters.size(); j < territoryMeshes.Num(); j++) {
					territoryMeshes[j]->SetVisibility(false);
				}
			}
			else 
			{
				for (int32 i = 0; i < territoryMeshes.Num(); i++) {
					territoryMeshes[i]->SetVisibility(false);
				}
			}
		}
	}
	

}

