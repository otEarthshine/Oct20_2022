// Pun Dumnernchanvanit's


#include "ResourceDropDisplayComponent.h"

using namespace std;

static const int32 maxStack = 1;

static std::vector<bool> _tileIdToInUse;

//void UResourceDropDisplayComponent::UpdateDisplay(int provinceId, int meshId, WorldAtom2 cameraAtom)
//{
//	auto& dropSystem = simulation().dropSystem();
//
//	// Set region location
//	WorldRegion2 region(provinceId);
//	FVector regionDisplayLocation = MapUtil::DisplayLocation(cameraAtom, region.worldAtom2());
//	_meshes[meshId]->SetRelativeLocation(regionDisplayLocation + FVector(5, 5, 0));
//
//	if (simulation().NeedDisplayUpdate(DisplayClusterEnum::ResourceDrop, provinceId) || isMainMenuDisplay)
//	{
//		//! Drops
//		if (!isMainMenuDisplay)
//		{
//			SCOPE_CYCLE_COUNTER(STAT_PunDisplayDropTick);
//
//			const std::vector<DropInfo>& drops = dropSystem.Drops(provinceId);
//
//			_tileIdToInUse.resize(maxStack * CoordinateConstants::TileIdsPerRegion);
//			fill(_tileIdToInUse.begin(), _tileIdToInUse.end(), false);
//
//			for (int j = 0; j < drops.size(); j++) {
//				DropInfo dropInfo = drops[j];
//
//				WorldTile2 tile = dropInfo.tile;
//				LocalTile2 localTile = tile.localTile();
//
//				// Increment the tileId if it was already in used
//				for (int k = 0; k < maxStack; k++) {
//					int32_t tileIdWithStack = localTile.tileId() + k * CoordinateConstants::TileIdsPerRegion;
//					if (!_tileIdToInUse[tileIdWithStack])
//					{
//						FTransform transform(FRotator::ZeroRotator, localTile.localDisplayLocation() + FVector(0, 0, 4 * k + 2), FVector(0.8f, 0.8f, 0.8f));
//						_meshes[meshId]->Add(ResourceNameF(dropInfo.holderInfo.resourceEnum) + FString("Hand"), tileIdWithStack, transform, 0);
//						_tileIdToInUse[tileIdWithStack] = true;
//						break;
//					}
//				}
//			}
//		}
//
//		// After
//		_meshes[meshId]->AfterAdd();
//
//		simulation().SetNeedDisplayUpdate(DisplayClusterEnum::ResourceDrop, provinceId, false);
//	}
//
//	if (isMainMenuDisplay) {
//		return;
//	}
//
//	// Debug lines
//	if (PunSettings::Settings["DropLines"])
//	{
//		const std::vector<DropInfo>& drops = dropSystem.Drops(provinceId);
//		for (int j = 0; j < drops.size(); j++) {
//			DropInfo dropInfo = drops[j];
//
//			WorldTile2 tile = dropInfo.tile;
//			FVector start = MapUtil::DisplayLocation(cameraAtom, tile.worldAtom2());
//			lineBatch()->DrawLine(start, start + FVector(0, 0, 100), FLinearColor::White, 100.0f, 1.0f, 10000);
//		}
//	}
//}