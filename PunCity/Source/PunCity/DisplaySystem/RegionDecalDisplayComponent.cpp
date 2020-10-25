// Pun Dumnernchanvanit's


#include "RegionDecalDisplayComponent.h"
#include "Components/DecalComponent.h"
#include "PunCity/GameNetworkInterface.h"
#include "../Simulation/OverlaySystem.h"

using namespace std;

int URegionDecalDisplayComponent::CreateNewDisplay(int objectId)
{
	int meshId = _roadDecals.Num();
	WorldRegion2 region(objectId);

	UDecalComponent* roadDecal = NewObject<UDecalComponent>(this);

	roadTextureDim = CoordinateConstants::TilesPerRegion * 2 + 4;

	// Road Decal
	{
		roadDecal->Rename(UTF8_TO_TCHAR(("RoadDecal" + to_string(meshId)).c_str()));
		roadDecal->AttachToComponent(this, FAttachmentTransformRules::KeepRelativeTransform, NAME_None);
		roadDecal->SetRelativeLocationAndRotation(FVector(0, 0, 0), FVector(0, 0, -1).Rotation()); // Rotate to project the decal down
		roadDecal->DecalSize = FVector(30, 160, 160); // 32 tiles = 160 
		roadDecal->RegisterComponent();
		roadDecal->SetActive(false);
		roadDecal->SetVisibility(false);
		roadDecal->SortOrder = 100;

		UMaterialInstanceDynamic* roadMaterial = UMaterialInstanceDynamic::Create(_assetLoader->RoadMaterial, roadDecal);
		roadDecal->SetMaterial(0, roadMaterial);

		UTexture2D* texture = UTexture2D::CreateTransient(roadTextureDim, roadTextureDim, PF_B8G8R8A8);
		texture->Filter = TF_Bilinear;
		//texture->W
		texture->AddToRoot();
		texture->UpdateResource();
		roadMaterial->SetTextureParameterValue(TEXT("RoadMask"), texture);

		_roadDecals.Add(FPunDecal(roadDecal, roadMaterial, texture));
	}

	// Overlay Decal
	{
		UDecalComponent* decal = NewObject<UDecalComponent>(roadDecal);
		decal->Rename(UTF8_TO_TCHAR(("OverlayDecal" + to_string(meshId)).c_str()));
		decal->AttachToComponent(roadDecal, FAttachmentTransformRules::KeepRelativeTransform, NAME_None);
		//decal->SetRelativeLocationAndRotation(FVector(0, 0, 0), FVector(0, 0, -1).Rotation()); // Rotate to project the decal down
		decal->DecalSize = FVector(30, 160, 160);
		decal->RegisterComponent();
		decal->SetActive(false);
		decal->SetVisibility(false);

		UMaterialInstanceDynamic* material = UMaterialInstanceDynamic::Create(_assetLoader->OverlayMaterial, decal);

		decal->SetMaterial(0, material);

		UTexture2D* texture = UTexture2D::CreateTransient(CoordinateConstants::TilesPerRegion, CoordinateConstants::TilesPerRegion, PF_B8G8R8A8); // PF_A8
		texture->Filter = TF_Nearest;
		texture->AddToRoot();
		texture->UpdateResource();
		material->SetTextureParameterValue(TEXT("OverlayMap"), texture);

		_overlayDecals.Add(FPunDecal(decal, material, texture));
	}

	// Ore Decal
	{
		UDecalComponent* decal = NewObject<UDecalComponent>(roadDecal);
		decal->Rename(UTF8_TO_TCHAR(("OreDecal" + to_string(meshId)).c_str()));
		decal->AttachToComponent(roadDecal, FAttachmentTransformRules::KeepRelativeTransform, NAME_None);
		decal->DecalSize = FVector(160, 160, 160); // 32 height... lift Z by 16 
		decal->RegisterComponent();
		decal->SetActive(false);
		decal->SetVisibility(false);

		decal->SetRelativeLocation(FVector(-161, 0, 0));

		decal->SetDecalMaterial(_assetLoader->M_CoalOreDecal);

		_oreDecals.Add(decal);
	}

	// Wetness
	{
		//UDecalComponent* decal = NewObject<UDecalComponent>(roadDecal);
		//decal->Rename(UTF8_TO_TCHAR(("WetnessDecal" + to_string(meshId)).c_str()));
		//decal->AttachToComponent(roadDecal, FAttachmentTransformRules::KeepRelativeTransform, NAME_None);
		////decal->SetRelativeLocationAndRotation(FVector(0, 0, -35), FVector(0, 0, -1).Rotation()); // Rotate to project the decal down
		////decal->SetRelativeLocationAndRotation(FVector(0, 0, 0), FVector(0, 0, -1).Rotation()); // Rotate to project the decal down
		//decal->SetWorldLocation(FVector(0, 0, -35)); // Rotate to project the decal down

		//decal->DecalSize = FVector(32, 160, 160);
		//decal->RegisterComponent();
		//decal->SetActive(false);
		//decal->SetVisibility(false);

		//decal->SetMaterial(0, _assetLoader->WetnessMaterial);

		//_wetnessDecals.Add(decal);
	}

	return meshId;
}

void URegionDecalDisplayComponent::OnSpawnDisplay(int regionId, int meshId, WorldAtom2 cameraAtom)
{
	auto& overlaySystem = simulation().overlaySystem();
	UpdateRoadDisplay(regionId, meshId, overlaySystem);
	
	//_wetnessDecals[meshId]->SetVisibility(true);
	_oreDecals[meshId]->SetVisibility(true);
}


static int32_t updateCount = 0;

void URegionDecalDisplayComponent::UpdateDisplay(int regionId, int meshId, WorldAtom2 cameraAtom)
{
	const float centerShift = CoordinateConstants::RegionCenterDisplayUnits + 5;
	
	WorldRegion2 region(regionId);
	WorldAtom2 objectAtomLocation(region.x * CoordinateConstants::AtomsPerRegion, 
									region.y * CoordinateConstants::AtomsPerRegion);
	FVector displayLocation = MapUtil::DisplayLocation(cameraAtom, objectAtomLocation);
	displayLocation += FVector(centerShift, centerShift, 0);
	
	TreeSystem& treeSystem = simulation().treeSystem();
	OverlayType overlayType = gameManager()->GetOverlayType();
	
	bool overlayWasVisible = _overlayDecals[meshId].decal->IsVisible();
	bool overlayActive = overlayType != OverlayType::None;

	_roadDecals[meshId].decal->SetRelativeLocation(displayLocation);


	/*
	 * Ore
	 */
	GeoresourceEnum georesourceEnum = simulation().georesourceSystem().georesourceNode(regionId).georesourceEnum;
	UMaterial* decalMaterial = nullptr;
	switch(georesourceEnum)
	{
	case GeoresourceEnum::GoldOre: decalMaterial = _assetLoader->M_GoldOreDecal; break;
	case GeoresourceEnum::IronOre: decalMaterial = _assetLoader->M_IronOreDecal; break;
	case GeoresourceEnum::CoalOre: decalMaterial = _assetLoader->M_CoalOreDecal; break;
	case GeoresourceEnum::Gemstone: decalMaterial = _assetLoader->M_GemstoneDecal; break;
	default:
		break;
	}
	
	if (decalMaterial) {
		_oreDecals[meshId]->SetDecalMaterial(decalMaterial);
		_oreDecals[meshId]->SetVisibility(true);
	} else {
		_oreDecals[meshId]->SetVisibility(false);
	}

	/*
	 * Overlay
	 */
	// Some overlay doesn't need grid...
	if (!IsGridOverlay(overlayType)) {
		overlayActive = false;
	}

	_overlayDecals[meshId].decal->SetVisibility(overlayActive);
	if (overlayActive)
	{
		if (!overlayWasVisible ||
			//_lastOverlayType != overlayType ||
			simulation().NeedDisplayUpdate(DisplayClusterEnum::Overlay, regionId) ||
			updateCount++ % 60 == 0)
		{
			//SCOPE_TIMER("Tick Region Overlay");

			static std::vector<uint32> overlayData;
			overlayData.resize(CoordinateConstants::TilesPerRegion * CoordinateConstants::TilesPerRegion, 230);

			// TODO: make this a ExecuteOnRegion function?
			int32 regionMinTileX = region.x * CoordinateConstants::TilesPerRegion;
			int32 regionMinTileY = region.y * CoordinateConstants::TilesPerRegion;

			OverlaySystem& overlaySys = simulation().overlaySystem();

			PunTerrainGenerator& generator = simulation().terrainGenerator();

			for (int y = 0; y < CoordinateConstants::TilesPerRegion; y++) {
				for (int x = 0; x < CoordinateConstants::TilesPerRegion; x++)
				{
					WorldTile2 worldTile(x + regionMinTileX, y + regionMinTileY);
					int32_t localTileId = x + y * CoordinateConstants::TilesPerRegion;
					uint8_t overlayColor = 0;

					if (overlayType == OverlayType::Appeal)
					{
						if (!simulation().IsWaterOrMountain(worldTile)) {
							int32 appeal = overlaySys.GetAppealPercent(worldTile);
							overlayColor = min(appeal * 254 / 100 + 1, 255); // 0 overlayData is blank tile
						}
					}
					else if (overlayType == OverlayType::Fish)
					{
						overlayColor = 255 * treeSystem.fish100Count(worldTile.tileId()) / 100;
					}
					else if (overlayType == OverlayType::Farm ||
							overlayType == OverlayType::IrrigationReservoir)
					{
						/*WorldTile2 tile(x + regionMinTileX, y + regionMinTileY);*/
						if (!simulation().IsWaterOrMountain(worldTile)) {
							// Color range... 50% - 100%
							int32 fertilityShaved = max(0, generator.GetFertilityPercent(worldTile) - OverlayFertilityMaxRed);
							overlayColor = fertilityShaved * 254 / (MaxFertility - OverlayFertilityMaxRed) + 1; // 0 overlayData is blank tile
						}
					}
					else {
						UE_DEBUG_BREAK();
					}

					bool highOpacity = overlayType == OverlayType::Fish || simulation().IsBuildable(worldTile);
					FColor color = FColor(overlayColor, 0, 0, highOpacity ? 255 : 0);
					overlayData[localTileId] = color.ToPackedARGB();
				}
			}

			UTexture2D* texture = _overlayDecals[meshId].texture;
			uint32* textureData = reinterpret_cast<uint32*>(texture->PlatformData->Mips[0].BulkData.Lock(LOCK_READ_WRITE));

			const size_t dataSize = texture->GetSizeX() * texture->GetSizeY() * sizeof(uint32);
			FMemory::Memcpy(textureData, overlayData.data(), dataSize);

			texture->PlatformData->Mips[0].BulkData.Unlock();
			texture->UpdateResource();
			_overlayDecals[meshId].material->SetTextureParameterValue(TEXT("OverlayMap"), texture);

			bool isSolid = overlayType != OverlayType::Appeal; // Only Appeal is noto solid
			_overlayDecals[meshId].material->SetScalarParameterValue(TEXT("IsSolid"), isSolid);

			simulation().SetNeedDisplayUpdate(DisplayClusterEnum::Overlay, regionId, false);
		}
	}

	//_lastOverlayType = overlayType;



	//! Refresh road
	auto& overlaySystem = simulation().overlaySystem();
	
	TileArea demolishArea;
	if (!isMainMenuDisplay) {
		demolishArea = gameManager()->networkInterface()->GetDemolishHighlightArea();
	}

	
	if (simulation().NeedDisplayUpdate(DisplayClusterEnum::Road, regionId) 
		|| isMainMenuDisplay || _lastDemolishArea != demolishArea) 
	{
		_lastDemolishArea = demolishArea;
		
		UpdateRoadDisplay(regionId, meshId, overlaySystem);
	}

}


// TODO: generalize..
static FUpdateTextureRegion2D texRegion;
static std::vector<uint8> roadData2;

static std::vector<uint32> roadData; // TODO: Use same as overlayData?

void URegionDecalDisplayComponent::UpdateRoadDisplay(int32 regionId, int meshId, OverlaySystem& overlaySystem)
{
	WorldRegion2 region(regionId);
	
	// Special case region on edge shouldn't display road anyway
	if (region.x == 0 || region.x == GameMapConstants::RegionsPerWorldX - 1||
		region.y == 0 || region.y == GameMapConstants::RegionsPerWorldY - 1) 
	{
		return;
	}
	
	const std::vector<RoadTile>& roads = overlaySystem.roads(regionId);

	// if there is no road, just don't display decal
	if (roads.size() == 0) {
		_roadDecals[meshId].decal->SetVisibility(false);
		return;
	}
	
	// Fill in the rest of the region
	if (roadData.size() == 0) {
		roadData.resize(roadTextureDim * roadTextureDim);
	}
	std::fill(roadData.begin(), roadData.end(), 0);

	SCOPE_TIMER_FILTER(1000, "Tick RegionDecal: Road");

	int32 roadDataDim = CoordinateConstants::TilesPerRegion + 2;
	std::vector<bool> roadDataBool(roadDataDim * roadDataDim, false);


	WorldTile2 roadDataDimStartTile = region.minTile() - WorldTile2(1, 1);

	auto fillRoadTile = [&](const RoadTile& roadTile)
	{
		WorldTile2 tile = roadTile.tile;

		WorldTile2 localTile = tile - roadDataDimStartTile;

		// A > 0.95 is stone... A > 0.2 is road
		int alpha = roadTile.isConstructed ? 0 : 255;

		if (PunSettings::CheatFullFarmRoad()) {
			alpha = 0;
		}

		if (isMainMenuDisplay) {
			alpha = 0; // Main menu's road is always constructed...
		}

		int blue = 0;// _lastDemolishArea.HasTile(roadTile.tile) ? 255 : 0;

		FColor color;
		if (roadTile.isDirt) {
			color = FColor(0, 255, blue, alpha);
		}
		else {
			color = FColor(255, 0, blue, alpha);
		}

		//roadData[localTile.x + localTile.y * CoordinateConstants::TilesPerRegion] = color.ToPackedARGB();

		auto fillRoadData = [&](WorldTile2& localTileIn)
		{
			roadData[(localTileIn.x * 2) + (localTileIn.y * 2) * roadTextureDim] = color.ToPackedARGB();
			roadData[(localTileIn.x * 2 + 1) + (localTileIn.y * 2) * roadTextureDim] = color.ToPackedARGB();
			roadData[(localTileIn.x * 2) + (localTileIn.y * 2 + 1) * roadTextureDim] = color.ToPackedARGB();
			roadData[(localTileIn.x * 2 + 1) + (localTileIn.y * 2 + 1) * roadTextureDim] = color.ToPackedARGB();
		};

		fillRoadData(localTile);

		roadDataBool[localTile.x + localTile.y * roadDataDim] = true;
	};
	
	for (const RoadTile& roadTile : roads) 
	{
		fillRoadTile(roadTile);

		auto getAndFillRoadTile = [&](WorldTile2 tile) {
			RoadTile curRoadTile = overlaySystem.GetRoad(tile);
			if (curRoadTile.isValid()) {
				fillRoadTile(curRoadTile);
			}
		};

		LocalTile2 localTile = roadTile.tile.localTile();

		// If this is on the edge, check for connection
		if (localTile.x == 0) {
			getAndFillRoadTile(roadTile.tile + WorldTile2(-1, -1));
			getAndFillRoadTile(roadTile.tile + WorldTile2(-1, 0));
			getAndFillRoadTile(roadTile.tile + WorldTile2(-1, 1));
		}
		if (localTile.x == CoordinateConstants::TilesPerRegion - 1) {
			getAndFillRoadTile(roadTile.tile + WorldTile2(1, -1));
			getAndFillRoadTile(roadTile.tile + WorldTile2(1, 0));
			getAndFillRoadTile(roadTile.tile + WorldTile2(1, 1));
		}
		if (localTile.y == 0) {
			getAndFillRoadTile(roadTile.tile + WorldTile2(-1, -1));
			getAndFillRoadTile(roadTile.tile + WorldTile2(0, -1));
			getAndFillRoadTile(roadTile.tile + WorldTile2(1, -1));
		}
		if (localTile.y == CoordinateConstants::TilesPerRegion - 1) {
			getAndFillRoadTile(roadTile.tile + WorldTile2(-1, 1));
			getAndFillRoadTile(roadTile.tile + WorldTile2(0, 1));
			getAndFillRoadTile(roadTile.tile + WorldTile2(1, 1));
		}
		
		//LocalTile2 localTile = roadTile.tile.localTile();

		//// Add 1 to localTile since the texture is +2 on the edge
		//localTile = LocalTile2(localTile.x + 1, localTile.y + 1);
		//
		//// A > 0.95 is stone... A > 0.2 is road
		//int alpha = roadTile.isConstructed ? 0 : 255;

		//if (PunSettings::CheatFullFarmRoad()) {
		//	alpha = 0;
		//}

		//if (isMainMenuDisplay) {
		//	alpha = 0; // Main menu's road is always constructed...
		//}

		//int blue = 0;// _lastDemolishArea.HasTile(roadTile.tile) ? 255 : 0;

		//FColor color;
		//if (roadTile.isDirt) {
		//	color = FColor(0, 255, blue, alpha);
		//}
		//else {
		//	color = FColor(255, 0, blue, alpha);
		//}
		//
		////roadData[localTile.x + localTile.y * CoordinateConstants::TilesPerRegion] = color.ToPackedARGB();

		//auto fillRoadData = [&](LocalTile2& localTileIn)
		//{
		//	roadData[(localTileIn.x * 2) + (localTileIn.y * 2) * roadTextureDim] = color.ToPackedARGB();
		//	roadData[(localTileIn.x * 2 + 1) + (localTileIn.y * 2) * roadTextureDim] = color.ToPackedARGB();
		//	roadData[(localTileIn.x * 2) + (localTileIn.y * 2 + 1) * roadTextureDim] = color.ToPackedARGB();
		//	roadData[(localTileIn.x * 2 + 1) + (localTileIn.y * 2 + 1) * roadTextureDim] = color.ToPackedARGB();
		//};

		//fillRoadData(localTile);

		//roadDataBool[localTile.x + localTile.y * roadDataDim] = true;

	}

	/*
	 * Check for diagonals
	 */
	for (int32 y = 0; y < roadDataDim; y++) {
		for (int32 x = 0; x < roadDataDim; x++)
		{
			if (!roadDataBool[x + y * roadDataDim])
			{
				//FColor colorBottom(roadData[(x * 2 - 2) + (y * 2) * roadTextureDim]);
				//FColor colorLeft(roadData[(x * 2) + (y * 2 - 2) * roadTextureDim]);
				//FColor colorTop(roadData[(x * 2 + 2) + (y * 2) * roadTextureDim]);
				//FColor colorRight(roadData[(x * 2) + (y * 2 + 2) * roadTextureDim]);

				bool hasBottom = (x > 0) && roadDataBool[(x - 1) + y * roadDataDim];
				bool hasLeft = (y > 0) && roadDataBool[x + (y - 1) * roadDataDim];
				bool hasTop = (x < roadDataDim - 1) && roadDataBool[(x + 1) + y * roadDataDim];
				bool hasRight = (y < roadDataDim - 1) && roadDataBool[x + (y + 1) * roadDataDim];

				if (hasBottom && hasLeft) {
					roadData[(x * 2) + (y * 2) * roadTextureDim] = FColor(roadData[(x * 2 - 2) + (y * 2) * roadTextureDim]).ToPackedARGB();
				}
				if (hasLeft && hasTop) {
					roadData[(x * 2 + 1) + (y * 2) * roadTextureDim] = FColor(roadData[(x * 2 + 2) + (y * 2) * roadTextureDim]).ToPackedARGB();
				}
				if (hasTop && hasRight) {
					roadData[(x * 2 + 1) + (y * 2 + 1) * roadTextureDim] = FColor(roadData[(x * 2 + 2) + (y * 2) * roadTextureDim]).ToPackedARGB();
				}
				if (hasRight && hasBottom) {
					roadData[(x * 2) + (y * 2 + 1) * roadTextureDim] = FColor(roadData[(x * 2 - 2) + (y * 2) * roadTextureDim]).ToPackedARGB();
				}
			}
			
			//if (FColor(roadData[(x * 2) + (y * 2) * roadTextureDim]).R == 0)
			//{
			//	FColor colorBottom(roadData[(x * 2 - 2) + (y * 2) * roadTextureDim]);
			//	FColor colorLeft(roadData[(x * 2) + (y * 2 - 2) * roadTextureDim]);
			//	FColor colorTop(roadData[(x * 2 + 2) + (y * 2) * roadTextureDim]);
			//	FColor colorRight(roadData[(x * 2) + (y * 2 + 2) * roadTextureDim]);
			//	
			//	bool hasBottom = colorBottom.R;
			//	bool hasLeft = colorLeft.R;
			//	bool hasTop = colorTop.R;
			//	bool hasRight = colorRight.R;

			//	if (hasBottom && hasLeft) {
			//		roadData[(x * 2) + (y * 2) * roadTextureDim] = colorBottom.ToPackedARGB();
			//	}
			//	if (hasLeft && hasTop) {
			//		roadData[(x * 2 + 1) + (y * 2) * roadTextureDim] = colorLeft.ToPackedARGB();
			//	}
			//	if (hasTop && hasRight) {
			//		roadData[(x * 2 + 1) + (y * 2 + 1) * roadTextureDim] = colorTop.ToPackedARGB();
			//	}
			//	if (hasRight && hasBottom) {
			//		roadData[(x * 2) + (y * 2 + 1) * roadTextureDim] = colorRight.ToPackedARGB();
			//	}
			//}
		}
	}


	UTexture2D* texture = _roadDecals[meshId].texture;
	const size_t dataSize = texture->GetSizeX() * texture->GetSizeY() * sizeof(uint32);
	
	uint32* textureData = reinterpret_cast<uint32*>(texture->PlatformData->Mips[0].BulkData.Lock(LOCK_READ_WRITE));

	FMemory::Memcpy(textureData, roadData.data(), dataSize);

	texture->PlatformData->Mips[0].BulkData.Unlock();
	texture->UpdateResource();

	//{
	//	texRegion = FUpdateTextureRegion2D(0, 0, 0, 0, CoordinateConstants::TilesPerRegion, CoordinateConstants::TilesPerRegion);
	//	texture->UpdateResource();
	//	texture->UpdateTextureRegions(0, 1, &texRegion, CoordinateConstants::TilesPerRegion, 4, roadData2.data());
	//	texture->UpdateResource();
	//}

	_roadDecals[meshId].material->SetTextureParameterValue(TEXT("RoadMask"), texture);

	_roadDecals[meshId].decal->SetVisibility(true);

	simulation().SetNeedDisplayUpdate(DisplayClusterEnum::Road, regionId, false);
}

void URegionDecalDisplayComponent::HideDisplay(int meshId, int32 regionId)
{
	_overlayDecals[meshId].decal->SetVisibility(false);

	_roadDecals[meshId].decal->SetVisibility(false);
	//_wetnessDecals[meshId]->SetVisibility(false);

	_oreDecals[meshId]->SetVisibility(false);
}