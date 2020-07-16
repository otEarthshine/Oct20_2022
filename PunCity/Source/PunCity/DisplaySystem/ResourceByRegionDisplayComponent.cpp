// Pun Dumnernchanvanit's


#include "ResourceByRegionDisplayComponent.h"

//DECLARE_CYCLE_STAT(TEXT("PUN: [Display]Building_Resource"), STAT_PunDisplayBuilding_Resource, STATGROUP_Game);

using namespace std;

//void UResourceByRegionDisplayComponent::UpdateDisplay(int regionId, int meshId, WorldAtom2 cameraAtom)
//{
//	auto& dropSystem = simulation().dropSystem();
//	auto& buildingSystem = simulation().buildingSystem();
//
//	// Set region location
//	WorldRegion2 region(regionId);
//	FVector regionDisplayLocation = MapUtil::DisplayLocation(cameraAtom, region.worldAtom2());
//	_meshes[meshId]->SetRelativeLocation(regionDisplayLocation + FVector(5, 5, 0));
//
//	if (simulation().NeedDisplayUpdate(DisplayClusterEnum::Resource, regionId) || isMainMenuDisplay)
//	{
//		//! Building's stacks
//		{
//			SCOPE_CYCLE_COUNTER(STAT_PunDisplayBuilding_Resource);
//
//			auto& buildingList = buildingSystem.buildingSubregionList();
//
//			buildingList.ExecuteRegion(region, [&](int32 buildingId)
//			{
//				Building& building = buildingSystem.building(buildingId);
//				std::vector<ResourceHolderInfo>& holderInfos = building.holderInfos();
//				TileArea area = building.area();
//				WorldTile2 minTile = area.min();
//
//				// relativeMinTile can be negative since minTile can be out of region..
//				WorldTile2 relativeMinTileWorld = minTile - region.minTile();
//				LocalTile2 relativeMinTile(relativeMinTileWorld.x, relativeMinTileWorld.y);
//
//				LocalTile2 localCenter = building.centerTile().localTile();
//				check(localCenter.isValid());
//				ResourceSystem& resourceSystem = simulation().resourceSystem(building.playerId());
//
//				const int resourcePerDisplay = 10;
//
//				if (building.isEnum(CardEnum::StorageYard) ||
//					building.isEnum(CardEnum::TradingPost) ||
//					building.isEnum(CardEnum::TradingPort) ||
//					building.isEnum(CardEnum::HumanitarianAidCamp))
//				{
//					int32 currentTileIndex = 0;
//
//					FVector resourceShift = FVector::ZeroVector;
//					if (building.isEnum(CardEnum::TradingPost)) {
//						switch (building.faceDirection()) {
//						case Direction::S: resourceShift = FVector(0, 20, 0); break;
//						case Direction::N: resourceShift = FVector(40, 20, 0); break;
//						case Direction::E: resourceShift = FVector(20, 40, 0); break;
//						case Direction::W: resourceShift = FVector(20, 0, 0); break;
//						default: break;
//						}
//						//resourceShift = FVector(0, 20, 0);
//						//resourceShift = resourceShift.RotateAngleAxis(RotationFromDirection(building.faceDirection()), FVector::UpVector);
//					}
//					else if (building.isEnum(CardEnum::TradingPort)) {
//						switch (building.faceDirection()) {
//						case Direction::S: resourceShift = FVector(0, 0, 0); break;
//						case Direction::N: resourceShift = FVector(60, 50, 0); break;
//						case Direction::E: resourceShift = FVector(0, 60, 0); break;
//						case Direction::W: resourceShift = FVector(50, 0, 0); break;
//						default: break;
//						}
//						//resourceShift = FVector(0, 20, 0);
//						//resourceShift = resourceShift.RotateAngleAxis(RotationFromDirection(building.faceDirection()), FVector::UpVector);
//					}
//
//					for (size_t j = 0; j < holderInfos.size(); j++)
//					{
//						check(holderInfos[j].isValid());
//
//						int resourceCount = resourceSystem.resourceCount(holderInfos[j]);
//						int tileCount = (resourceCount + GameConstants::StorageCountPerTile - 1) / GameConstants::StorageCountPerTile;
//						check(tileCount >= 0);
//						check(tileCount <= building.storageSlotCount());
//
//						FString resourceName = ToFString(holderInfos[j].resourceName());
//
//						const int stacksPerSide = building.stackPerSide();
//
//						for (int i = 0; i < tileCount; i++)
//						{
//							int x = currentTileIndex % stacksPerSide;
//							int y = currentTileIndex / stacksPerSide;
//							currentTileIndex++;
//
//							FVector resourceLocation = FVector(x * 20 + 5, y * 20 + 5, 0) + relativeMinTile.localDisplayLocation() + resourceShift;
//
//							// Scale is used to show resource fraction.
//							int resourceInStack = std::min(GameConstants::StorageCountPerTile, resourceCount - i * GameConstants::StorageCountPerTile);
//							int unitsInStack = (resourceInStack + resourcePerDisplay - 1) / resourcePerDisplay;
//							PUN_CHECK(unitsInStack > 0);
//							float scale = unitsInStack * 0.05f + 0.01;
//
//							FTransform transform(FRotator(0, 0, 0), resourceLocation, FVector(1, 1, scale));
//
//							// TODO: resourceTile could be out of range...
//							//  Handle by having putting it in tileId beyond max reserved for this?? reserve.. complex..
//							//  Or just be lazy and 5000 * 16 tileIds per region, too much???
//							// Or use TileIdHashMap for this?? unlimited tileIds... less code!
//
//							_meshes[meshId]->Add(resourceName, localCenter.tileId() + i * CoordinateConstants::TileIdsPerRegion, transform, 0);
//						}
//					}
//				}
//
//			});
//		}
//
//
//		// After
//		_meshes[meshId]->AfterAdd();
//
//		simulation().SetNeedDisplayUpdate(DisplayClusterEnum::Resource, regionId, false);
//	}
//	
//}