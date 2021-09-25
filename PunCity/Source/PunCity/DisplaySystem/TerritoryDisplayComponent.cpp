// Fill out your copyright notice in the Description page of Project Settings.

#include "TerritoryDisplayComponent.h"
#include "../MapUtil.h"
#include "PunCity/GameConstants.h"
#include "../Simulation/ProvinceInfoSystem.h"

using namespace std;

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

void UTerritoryDisplayComponent::Display(std::vector<int>& sampleProvinceIds)
{
	auto& sim = simulation();
	auto& provinceSys = sim.provinceSystem();
	auto& provinceInfoSys = sim.provinceInfoSystem();
	
	/*
	 * Show province connections
	 */
	{
		int32 index = 0;

		if (gameManager()->isShowingDefenseOverlay() &&
			_gameManager->ZoomDistanceBelow(WorldZoomTransition_GameToMap))
		{
			auto spawnMesh = [&]()
			{
				UStaticMeshComponent* meshComp;
				if (index == _defenseOverlayMeshes.Num()) {
					meshComp = NewObject<UStaticMeshComponent>(this);
					meshComp->AttachToComponent(this, FAttachmentTransformRules::KeepRelativeTransform);
					meshComp->RegisterComponent();
					meshComp->SetReceivesDecals(false);
					meshComp->SetCollisionEnabled(ECollisionEnabled::NoCollision);
					_defenseOverlayMeshes.Add(meshComp);
					check(index < _defenseOverlayMeshes.Num());
				}
				else {
					meshComp = _defenseOverlayMeshes[index];
				}

				meshComp->SetVisibility(true);
				index++;

				return meshComp;
			};


			const float maxScaling = 3.0f;
			const float minZoomDistance = 500;
			const float maxZoomDistance = 2000;
			float lerp01 = fabs(gameManager()->zoomDistance() - minZoomDistance) / (maxZoomDistance - minZoomDistance);
			float displayScaling = 1.0f + lerp01 * (maxScaling - 1.0f);

			for (int32 i = 0; i < sampleProvinceIds.size(); i++)
			{
				int32 provinceId = sampleProvinceIds[i];

				DefenseOverlayEnum defenseOverlayEnum;
				FTransform nodeTransform;
				TArray<FTransform> lineTransforms;
				GetDefenseNodeDisplayInfo(provinceId, displayScaling, defenseOverlayEnum, nodeTransform, lineTransforms);


				// Node
				UStaticMeshComponent* nodeMesh = spawnMesh();
				if (defenseOverlayEnum == DefenseOverlayEnum::CityNode) {
					nodeMesh->SetStaticMesh(_assetLoader->DefenseOverlay_CityNode);
				}
				else if (defenseOverlayEnum == DefenseOverlayEnum::FortNode) {
					nodeMesh->SetStaticMesh(_assetLoader->DefenseOverlay_FortNode);
				}
				else {
					nodeMesh->SetStaticMesh(_assetLoader->DefenseOverlay_Node);
				}
				nodeMesh->SetRelativeTransform(nodeTransform);
				nodeMesh->TranslucencySortPriority = 5000;


				// Lines
				for (const FTransform& lineTransform : lineTransforms)
				{
					UStaticMeshComponent* lineMesh = spawnMesh();
					lineMesh->SetStaticMesh(_assetLoader->DefenseOverlay_Line);

					lineMesh->SetRelativeTransform(lineTransform);
					lineMesh->TranslucencySortPriority = 4000;
				}

			}
		}
		
		
		for (int32 i = index; i < _defenseOverlayMeshes.Num(); i++)
		{
			_defenseOverlayMeshes[i]->SetVisibility(false);
		}
	}

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
			if (simulation().provinceOwnerTown_Major(provinceId) == -1) {
				sampleProvinceIdsNonPlayer.push_back(provinceId);
			}
		}
	}
	
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

		bool isFade = false; // Fade when province has no owner, and province is not next to player
		FLinearColor minorCityColor = FLinearColor::Black; // 
		
		int32 minorProvinceTownId = simulation().provinceOwnerTown_Minor(comp->provinceId);
		if (minorProvinceTownId != -1)
		{
			minorCityColor = FLinearColor::MakeFromHSV8(GameRand::Rand(minorProvinceTownId) % 255, 255, 255);;
		}
		else
		{
			// Fade when it is not near our territory
			int32 provinceTownId = simulation().provinceOwnerTown_Major(comp->provinceId);
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
		}
		
		material->SetScalarParameterValue("IsFade", isFade);
		//material->SetScalarParameterValue("IsFade", (owner == -1) && !simulation().IsProvinceNextToPlayer(comp->provinceId, playerId()));
		material->SetVectorParameterValue("MinorCityColor", minorCityColor);

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
				int32 townId = simulation().provinceOwnerTown_Major(provinceId);
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

						FPlayerInfo playerInfo = gameManager()->playerInfo(townManage.playerIdForLogo());

						auto material = _gameManager->ZoomDistanceBelow(WorldZoomTransition_GameToMap) ? comp->MaterialInstance : comp->MaterialInstance_Top;
						material->SetVectorParameterValue("PlayerColor1", playerInfo.logoColorBackground);
						material->SetVectorParameterValue("PlayerColor2", playerInfo.logoColorForeground);
						material->SetTextureParameterValue("Logo", _assetLoader->GetPlayerLogo(playerInfo.logoIndex));

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

