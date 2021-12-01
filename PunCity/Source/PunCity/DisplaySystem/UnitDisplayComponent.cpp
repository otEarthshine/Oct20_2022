#include "UnitDisplayComponent.h"

#include "../Simulation/UnitStateAI.h"
#include "../Simulation/ProjectileSystem.h"
#include "Components/LineBatchComponent.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "Components/StaticMeshComponent.h"
#include "PunCity/PunGameSettings.h"
#include "Kismet/KismetMathLibrary.h"

struct UnitPoseInfo {
	WorldAtom2 actualLocation;
	WorldAtom2 facingLocation;
	//UnitPoseInfo(WorldAtom2 atomLocation, WorldAtom2 facingLocation) : actualLocation(atomLocation), facingLocation(facingLocation) {}
	//UnitPoseInfo() : actualLocation(WorldAtom2::Invalid), facingLocation(WorldAtom2::Invalid) {}
};

using namespace std;

UnitDisplayState UUnitDisplayComponent::GetUnitTransformAndVariation(UnitStateAI& unitAI, FTransform& transform)
{
	SCOPE_CYCLE_COUNTER(STAT_PunDisplayUnitTransform);
	
	LLM_SCOPE_(EPunSimLLMTag::PUN_DisplayUnit);

	int32 unitId = unitAI.id();
	UnitEnum unitEnum = unitAI.unitEnum();
	WorldAtom2 cameraAtom = gameManager()->cameraAtom();

	auto& sim = simulation();
	UnitSystem& unitSystem = sim.unitSystem();
	UnitStateAI& unit = unitSystem.unitStateAI(unitId);

	// Get Transform info
	UnitPoseInfo pose;
	pose.actualLocation = unitSystem.actualAtomLocation(unitId);
	pose.facingLocation = unitSystem.targetLocation(unitId);

	FVector displayLocation = MapUtil::DisplayLocation(cameraAtom, pose.actualLocation);
	int32 zoomDistance = _gameManager->zoomDistance();
	
	// No Animation
	if (zoomDistance > WorldZoomTransition_UnitAnimate)
	{
		transform = FTransform(FRotator::ZeroRotator, displayLocation, FVector::OneVector);
		return UnitDisplayState();
	}
	

	
	FTransform lastTransform = FTransform::Identity;
	if (_lastTransforms.Contains(unitId)) {
		lastTransform = _lastTransforms[unitId];
	}
	float lastAnimationTime = lastTransform.GetScale3D().Z;
	
	float scale = 1.0f;// unit.age() >= (float)unit.unitInfo().adultTicks ? 1.0f : 0.7f;

	// Calculate rotation
	FRotator rotator = lastTransform.Rotator();

	FVector targetDisplayLocation = MapUtil::DisplayLocation(cameraAtom, pose.facingLocation);
	FVector targetDirection = targetDisplayLocation - displayLocation;
	FRotator targetRotation = targetDirection.Rotation();

	//! TODO: !!! hack around vertex animate laziness
	targetRotation.Yaw -= 90;

	if (targetDirection.Size() > 0.01f) {
		targetDirection.Normalize();
		rotator = FMath::Lerp(rotator, targetRotation, 0.3f);
	}

	// Special case walk on bridge
	WorldTile2 unitTile = pose.actualLocation.worldTile2();
	CardEnum buildingEnumAtTile = sim.buildingEnumAtTile(unitTile);
	if (IsBridgeOrTunnel(buildingEnumAtTile))
	{
		Building& bridge = *sim.buildingAtTile(unitTile); // simulation().building(GameMap::buildingId(unitTile));

		TileArea area = bridge.area();
		int32_t areaLength = std::max(area.sizeX(), area.sizeY());
		bool isXDirection = area.sizeX() > area.sizeY();
		WorldTile2 start = area.min();
		WorldTile2 end = isXDirection ? WorldTile2(start.x + areaLength - 1, start.y) : WorldTile2(start.x, start.y + areaLength - 1);

		float tileFraction;
		if (isXDirection) {
			tileFraction = static_cast<float>(pose.actualLocation.x - unitTile.worldAtom2().x) / CoordinateConstants::AtomsPerTile + 0.5f;
		} else {
			tileFraction = static_cast<float>(pose.actualLocation.y - unitTile.worldAtom2().y) / CoordinateConstants::AtomsPerTile + 0.5f;
		}

		if (unitTile == start) {
			// Looking N, E
			displayLocation.Z = tileFraction * 6;
		}
		else if (unitTile == end) {
			// Looking S, W
			displayLocation.Z = (1.0f - tileFraction) * 6;
		}
		else {
			displayLocation.Z = 6;
		}

		// Tunnel goes underground instead
		if (buildingEnumAtTile == CardEnum::Tunnel) {
			displayLocation.Z = -displayLocation.Z;
		}

		//PUN_LOG("tileFraction %f, Z %f", tileFraction, displayLocation.Z);
	}


	transform = FTransform(rotator, displayLocation, FVector(scale, scale, scale));

	//! TODO: !!! hack around vertex animate laziness
	//if (IsAnimal(unit.unitEnum())) {
		// Don't exceed the scale of 300 to prevent stuttering animation from inaccurate float

	//if (IsUsingVertexAnimation(unitEnum)) {
	//	float gameSpeed = sim.gameSpeedFloat();
	//	scale = std::fmodf(lastAnimationTime + (unitSystem.isMoving(unitId) ? (GetWorld()->GetDeltaSeconds() * 2.0f * gameSpeed) : 0.0f), 2.0f);
	//}


	//! Display State
	bool isFocusedUnit = IsFocusedUnit(unitId);
	bool justFocusedUnit = JustFocusedUnit();
	bool disableFlashPrevention = isFocusedUnit && justFocusedUnit;
	
	UnitAnimationEnum animationEnum = unit.GetDisplayAnimationEnum(!disableFlashPrevention);
	
	check(animationEnum != UnitAnimationEnum::None);
	//		if (PunSettings::IsOn("UseFullSkelAnim")) {
//			return true;
//		}
//		// Walk animation should use Vertex Animation
//		if (IsCitizenWalkAnimation(animationEnum)) {
//			return true;
//			//return zoomDistance > WorldZoomTransition_HumanWalkAnimate;
//		}

	// Walking Human uses VertexAnimation
	if (unitEnum == UnitEnum::Human)
	{
		//if (animationEnum != UnitAnimationEnum::Ship &&
		//	!IsHorseAnimation(animationEnum) &&
		//	animationEnum != UnitAnimationEnum::ImmigrationCart &&
		//	animationEnum != UnitAnimationEnum::HaulingCart)
		//{
			if (ShouldHumanUseVertexAnimation(animationEnum, zoomDistance) &&
				!isFocusedUnit)
			{
				float gameSpeed = sim.gameSpeedFloat();
				scale = std::fmodf(lastAnimationTime + (unitSystem.isMoving(unitId) ? (GetWorld()->GetDeltaSeconds() * 2.0f * gameSpeed) : 0.0f), 2.0f);
			}
		//}
	}
	else if (unitEnum == UnitEnum::WildMan) {
		
	}
	else {
		float gameSpeed = sim.gameSpeedFloat();
		scale = std::fmodf(lastAnimationTime + (unitSystem.isMoving(unitId) ? (GetWorld()->GetDeltaSeconds() * 2.0f * gameSpeed) : 0.0f), 2.0f);
	}
	
	
	transform.SetScale3D(FVector(scale, scale, scale)); // Need same scale x,y,z to make lighting work properly...


	// Human
	if (unitEnum == UnitEnum::Human) 
	{	
		// Special case: Horse for caravan
		if (animationEnum == UnitAnimationEnum::HorseCaravan) {
			return  { UnitEnum::HorseCaravan, UnitAnimationEnum::Walk, 0 };
		}
		if (animationEnum == UnitAnimationEnum::HorseMarket) {
			return  { UnitEnum::HorseMarket, UnitAnimationEnum::Walk, 0 };
		}
		if (animationEnum == UnitAnimationEnum::HorseLogistics) {
			return  { UnitEnum::HorseLogistics, UnitAnimationEnum::Walk, 0 };
		}
		if (animationEnum == UnitAnimationEnum::Ship) {
			return  { UnitEnum::SmallShip, UnitAnimationEnum::Walk, 0 };
		}

		int32 humanVariation = static_cast<int>(GetHumanVariationEnum(unit.isChild(), unit.isMale(), unit.GetDisplayAnimationEnum(!isFocusedUnit)));
		
		if (animationEnum == UnitAnimationEnum::ImmigrationCart ||
			animationEnum == UnitAnimationEnum::HaulingCart) {
			return  { UnitEnum::Human, UnitAnimationEnum::Walk, humanVariation };
		}
		return  { unitEnum, animationEnum, humanVariation };
	}

	//if (IsUsingSkeletalMesh(unitEnum, animationEnum, _gameManager->zoomDistance())) {
	if (unitEnum == UnitEnum::WildMan) {
		return { unitEnum, animationEnum, 0 };
	}

	// Animals with children
	if (_assetLoader->unitMeshCount(unitEnum) >= 2) {
		return { unitEnum, animationEnum, unit.isChild() ? 1 : 0 };
	}
	
	return { unitEnum, animationEnum, 0 };
}

void UUnitDisplayComponent::UpdateDisplay(int regionId, int meshId, WorldAtom2 cameraAtom, bool justSpawned, bool justCreated)
{
	LLM_SCOPE_(EPunSimLLMTag::PUN_DisplayUnit);
	
	UnitSystem& unitSystem = simulation().unitSystem();
	auto& unitList = simulation().unitSystem().unitSubregionLists();

	//PUN_LOG("ExecuteRegionStart ticks:%d regionId:%d", TimeDisplay::Ticks(), regionId)

	// Overlay highlights
	{
		OverlayType overlayType = gameManager()->GetOverlayType();
		if (overlayType == OverlayType::Hunter) {
			for (UnitEnum unitEnum : WildAnimalNoHomeEnums) {
				SetHighlight(unitEnum, 1);
			}
			for (UnitEnum unitEnum : WildAnimalWithHomeEnums) {
				SetHighlight(unitEnum, 1);
			}
		}
		else if (overlayType == OverlayType::Human)
		{
			SetHighlight(UnitEnum::Human, 1);
		}
		else
		{
			for (int32 i = 0; i < UnitEnumCount; i++) {
				UnitEnum unitEnum = static_cast<UnitEnum>(i);
				SetHighlight(unitEnum, 0);
			}
		}
	}

	WorldRegion2 region(regionId);


	

	float zoomDistance = _gameManager->zoomDistance();
	bool displayOnlyLargeUnits = (zoomDistance > WorldZoomTransition_UnitSmall);

	unitList.ExecuteRegion(region, [&](int32 unitId)
	{
		if (!unitSystem.aliveUnsafe(unitId)) {
			return;
		}

		UnitStateAI& unit = unitSystem.unitStateAI(unitId);
		UnitEnum unitEnum = unit.unitEnum();

		/*
		 *
		 */

		if (displayOnlyLargeUnits && unitEnum != UnitEnum::Hippo) {
			return;
		}

		/*
		 * Special case Human/WildMan
		 *
		 *  Decided when to use Skeletal Mesh VS Vertex Animation
		 */
		bool isFocusedUnit = IsFocusedUnit(unitId);
		bool justFocusedUnit = JustFocusedUnit();
		bool disableFlashPrevention = isFocusedUnit && justFocusedUnit;
		{
			// Zoomed out human should always use instancedStaticMesh
			UnitAnimationEnum animationEnum = unit.GetDisplayAnimationEnum(!disableFlashPrevention);
			
			if (unitEnum == UnitEnum::Human &&
				animationEnum != UnitAnimationEnum::Ship)
			{
				if (!ShouldHumanUseVertexAnimation(animationEnum, zoomDistance) ||
					isFocusedUnit)
				{
					FTransform transform;
					AddSkelMesh(unit, transform);
					UpdateResourceDisplay(unitId, unit, transform);
					return;
				}
			}
			if (unitEnum == UnitEnum::WildMan)
			{
				FTransform transform;
				AddSkelMesh(unit, transform);
				UpdateResourceDisplay(unitId, unit, transform);
				return;
			}
		}




		
		/*
		 * Vertex Animate
		 */

		FTransform transform;
		_currentDisplayState = { unitEnum, unit.GetDisplayAnimationEnum(!disableFlashPrevention), 0 };
		
		//PUN_LOG("UnitAddStart ticks:%d id:%d regionId:%d", TimeDisplay::Ticks(), unitId, regionId);

		// Special case projectile
		if (IsProjectile(unit.unitEnum())) 
		{
			SCOPE_CYCLE_COUNTER(STAT_PunDisplayUnitProjectile);
			
			auto arrow = static_cast<ProjectileArrow*>(&unit);
			ProjectileDisplayInfo projectileInfo = arrow->GetProjectileDisplayInfo();
			FVector projectileLocation = MapUtil::DisplayLocation(cameraAtom, projectileInfo.location);

			// Height is parabola with a fixed max height
			const float maxProjectileHeight = projectileInfo.groundAtomDistance / CoordinateConstants::AtomPerDisplayUnit * 0.5f;
			float fractionTime = projectileInfo.fraction100000 / 100000.0f;

			// Quad eqn with root at 0 and 1:
			// 0 = t * (t + 1)
			// Convert it to positive t and y
			// y = -t^2 + t
			// This eqn max at t = 0.5 or y = 0.25
			// y = (-t^2 + t) * A
			// introduce "A" to scale y to maxProjectileHeight
			float factorA = maxProjectileHeight / 0.25f;
			projectileLocation.Z = (-fractionTime * fractionTime + fractionTime) * factorA;

			// Get ySpeed and xSpeed for determining pitch rotation
			// dy/dt = (-2t + 1) * A
			float ySpeed_distToFrac = (-2.0f * fractionTime + 1.0f) * factorA;
			float ySpeed_distToTick = ySpeed_distToFrac / projectileInfo.totalTicks;
			float xSpeed_distToTick = projectileInfo.groundAtomSpeed / CoordinateConstants::AtomPerDisplayUnit;

			FVector projectileTargetLocation = MapUtil::DisplayLocation(cameraAtom, projectileInfo.targetLocation);
			FVector projectileGroundLocation = projectileLocation;
			projectileGroundLocation.Z = 0;
			FVector projectileTargetDirection = projectileTargetLocation - projectileGroundLocation;
			FRotator projectileRotation = projectileTargetDirection.Rotation();
			check(projectileRotation.Pitch < 0.01 && projectileRotation.Roll < 0.01);
			projectileRotation.Pitch = UKismetMathLibrary::DegAtan(ySpeed_distToTick / xSpeed_distToTick);

			//PUN_LOG("projectileLocationAtom %s tile:%s", *ToFString(projectileInfo.location.ToString()), *projectileInfo.location.worldTile2().To_FString());
			//PUN_LOG("projectileRotation: %s , pitch: %f", *projectileRotation.ToString(), UKismetMathLibrary::DegAtan(ySpeed_distToTick / xSpeed_distToTick));
			//PUN_LOG("projectileLocation: %s", *projectileLocation.ToString());

			transform.SetTranslation(projectileLocation);
			transform.SetRotation(projectileRotation.Quaternion());
			transform.SetScale3D(FVector::OneVector);
		}
		else
		{
			_currentDisplayState = GetUnitTransformAndVariation(unit, transform);
		}


		_thisTransform.Add(unitId, transform);

		{
			LEAN_PROFILING_D(TickUnitDisplay_Unit);
			SCOPE_CYCLE_COUNTER(STAT_PunDisplayUnitAddInst);
			//_unitMeshes->Add(GetMeshName(_currentDisplayState.unitEnum, _currentDisplayState.variationIndex), unitId, transform, 0, unitId);
			_unitMeshes1->Add(_currentDisplayState.unitEnum, _currentDisplayState.variationIndex, unitId, transform);
		}
		

		// No Animation
		if (_gameManager->zoomDistance() > WorldZoomTransition_UnitAnimate) {
			return;
		}

		UpdateResourceDisplay(unitId, unit, transform);
		

		// DEBUG
		if (PunSettings::Settings["PathLines"])
		{
			ULineBatchComponent* line = lineBatch();

			std::vector<WorldTile2>& waypoint = unitSystem.waypoint(unitId);
			for (int i = 1; i < waypoint.size(); i++)
			{
				FVector start = MapUtil::DisplayLocation(cameraAtom, waypoint[i - 1].worldAtom2());
				FVector end = MapUtil::DisplayLocation(cameraAtom, waypoint[i].worldAtom2());
				start.Z = 1.0f;
				end.Z = 1.0f;
				line->DrawLine(start, end, FLinearColor::Blue, 100.0f, 1.0f, 10000);
			}
		}

		
	});

	//PUN_LOG("ExecuteRegionEnd ticks:%d regionId:%d", TimeDisplay::Ticks(), regionId);



	// Rotators
	{
		BuildingSystem& buildingSystem = simulation().buildingSystem();
		auto& buildingList = buildingSystem.buildingSubregionList();
		const GameDisplayInfo& displayInfo = gameManager()->displayInfo();

		buildingList.ExecuteRegion(WorldRegion2(regionId), [&](int32 buildingId)
		{
			Building& building = buildingSystem.building(buildingId);
			CardEnum buildingEnum = building.buildingEnum();
			FactionEnum factionEnum = building.factionEnum();

			if (displayInfo.GetVariationCount(factionEnum, buildingEnum) == 0) {
				return;
			}
			
			// Building mesh
			int32_t displayVariationIndex = building.displayVariationIndex();
			const ModuleTransformGroup& modulePrototype = displayInfo.GetDisplayModules(factionEnum, buildingEnum, displayVariationIndex);
			std::vector<ModuleTransform> modules = modulePrototype.animTransforms;

			if (modules.size() == 0) {
				return;
			}
			
			WorldTile2 centerTile = building.displayCenterTile();
			float buildingRotation = RotationFromDirection(building.displayFaceDirection());

			FVector displayLocation = MapUtil::DisplayLocation(cameraAtom, centerTile.worldAtom2());
			
			FTransform transform(FRotator(0, buildingRotation, 0), displayLocation);

			/*
			 * Moving parts
			 */

			if (building.isConstructed())
			{
				for (int i = 0; i < modules.size(); i++) 
				{
					int32 instanceKey = centerTile.tileId() + i * GameMapConstants::TilesPerWorld;

					ModuleTypeEnum moduleTypeEnum = modules[i].moduleTypeEnum;

					// Rotation
					FRotationInfo rotation;
					rotation.moduleTypeEnum = moduleTypeEnum;
					{
						// Windmill default degree special case
						if (buildingEnum == CardEnum::Windmill) {
							rotation.rotationFloat = 45;
						}

						int32 moduleHash = static_cast<int32>(moduleTypeEnum) * buildingId;
						if (_lastWorkRotatorRotation.Contains(moduleHash)) {
							rotation = _lastWorkRotatorRotation[moduleHash];
						}
						else {
							_lastWorkRotatorRotation.Add(moduleHash, rotation);
						}
						
						bool shouldRotate = (moduleTypeEnum == ModuleTypeEnum::RotateRoll || 
											moduleTypeEnum == ModuleTypeEnum::RotateRollMine ||
											moduleTypeEnum == ModuleTypeEnum::RotateRollMine2 ||
											moduleTypeEnum == ModuleTypeEnum::RotateRollQuarry ||
											moduleTypeEnum == ModuleTypeEnum::RotateRollFurniture ||
											moduleTypeEnum == ModuleTypeEnum::RotateZAxis) &&
											building.shouldDisplayParticles();

						float targetDegreePerSec = 90;

						// Roll Speed
						if (moduleTypeEnum == ModuleTypeEnum::RotateRollMine) {
							targetDegreePerSec = 45;
						}
						else if (moduleTypeEnum == ModuleTypeEnum::RotateRollMine2) {
							targetDegreePerSec = 15;
						}
						else if (moduleTypeEnum == ModuleTypeEnum::RotateRollQuarry) {
							targetDegreePerSec = 45;
						}
						
						
						const float rotationAcceleration = 90; // 1 sec to reach full speed
						float deltaTime = UGameplayStatics::GetWorldDeltaSeconds(GetWorld());
						if (shouldRotate)
						{
							if (rotation.rotationSpeed < targetDegreePerSec) {
								rotation.rotationSpeed += rotationAcceleration * deltaTime;
							}
							rotation.rotationFloat = rotation.rotationFloat - deltaTime * rotation.rotationSpeed;
							//rotationFloat = UGameplayStatics::GetTimeSeconds(GetWorld()) * 180;
						}
						else
						{
							rotation.rotationSpeed = fmax(rotation.rotationSpeed - rotationAcceleration * deltaTime, 0);
							rotation.rotationFloat = rotation.rotationFloat - deltaTime * rotation.rotationSpeed;
						}
						_lastWorkRotatorRotation[moduleHash] = rotation;
					}
					
					// WorkShaderAnimate
					// - Mushroom
					FVector scale = modules[i].transform.GetScale3D();
					if (modules[i].moduleTypeEnum == ModuleTypeEnum::ShaderAnimate) {
						// Don't show if not work hasn't started
						if (building.workDone100() == 0) {
							continue;
						}
						
						// Use scale to animate individual instance...
						scale = FVector(building.workFraction() * 0.5f + 0.5f);
					}

					// WorkShaderOnOff
					// - Beer Brewery
					if (moduleTypeEnum == ModuleTypeEnum::ShaderOnOff)
					{
						rotation.rotationFloat = 0.0f;
						scale = building.shouldDisplayParticles() ? FVector(1.0f) :FVector(0.5f);
					}

					FRotator rotator = FRotator(0, 0, rotation.rotationFloat);
					if (moduleTypeEnum == ModuleTypeEnum::RotateRollMine ||
						moduleTypeEnum == ModuleTypeEnum::RotateRollMine2 ||
						moduleTypeEnum == ModuleTypeEnum::RotateRollFurniture) {
						rotator = FRotator(rotation.rotationFloat, 0, 0);
					}
					else if (moduleTypeEnum == ModuleTypeEnum::RotateZAxis) {
						rotator = FRotator(0, rotation.rotationFloat, 0);
					}
					
					FTransform localTransform = FTransform(rotator, modules[i].transform.GetTranslation(), scale);

					FTransform moduleTransform;
					FTransform::Multiply(&moduleTransform, &localTransform, &transform);

					//PUN_LOG("ModuleDisplay %s, %s", *moduleTransform.GetRotation().Rotator().ToString(), *modules[i].transform.GetRotation().Rotator().ToString());

					_animatingModuleMeshes->Add(modules[i].moduleName, instanceKey, moduleTransform, 0, buildingId);
				}

			}
		});
	}

}


void UUnitDisplayComponent::UpdateResourceDisplay(int32 unitId, UnitStateAI& unit, FTransform& transformIn)
{
	LEAN_PROFILING_D(TickUnitDisplay_Resource);
	
	SCOPE_CYCLE_COUNTER(STAT_PunDisplayUnitResource);

	/*
	 * Unit aux display
	 */
	auto getAuxTransform = [&]() {
		FRotator rotator = transformIn.Rotator();
		rotator.Yaw -= 90;
		FTransform transform(rotator, transformIn.GetTranslation(), transformIn.GetScale3D());
		return transform;
	};

	auto displayResource = [&](float inventoryShift, float inventoryHeight = 0.0f)
	{
		//! Resource Display
		ResourceEnum heldEnum = unit.inventory().Display();
		if (heldEnum != ResourceEnum::None)
		{
			FTransform transform = transformIn;

			//! TODO: !!! hack around vertex animate laziness
			//if (IsAnimal(unit.unitEnum())) {
			FRotator rotator = transform.Rotator();
			transform.SetScale3D(FVector(1, 1, 1));
			rotator.Yaw += 90;
			//}


			// Display smaller resource on units
			transform.SetScale3D(FVector(0.7, 0.7, 0.7));

			FVector horizontalShift = rotator.RotateVector(FVector(inventoryShift, 0, inventoryHeight));
			transform.SetTranslation(transform.GetTranslation() + horizontalShift + FVector(0, 0, 10));

			// Zeros the rotation
			FRotator resourceRotator = transform.Rotator();
			resourceRotator.Yaw -= 90;
			FTransform resourceTransform(resourceRotator, transform.GetTranslation(), transform.GetScale3D());

			_resourceMeshes->Add(ResourceDisplaySystemName(heldEnum), unitId, resourceTransform, 0, unitId);
		}
	};
	
	if (_currentDisplayState.unitEnum == UnitEnum::HorseCaravan) 
	{
		_auxMeshes->Add("HorseCaravan", unitId, getAuxTransform(), 0);
	}
	else if (_currentDisplayState.unitEnum == UnitEnum::HorseLogistics ||
			_currentDisplayState.unitEnum == UnitEnum::HorseMarket)
	{
		FName meshName = _currentDisplayState.unitEnum == UnitEnum::HorseLogistics ? "HorseLogistics" : "HorseMarket";
		_auxMeshes->Add(meshName, unitId, getAuxTransform(), 0);

		displayResource(-15, 5);
	}
	else
	{
		//! Cart Display
		if (_currentDisplayState.unitEnum == UnitEnum::Human) 
		{
			bool isFocusedUnit = IsFocusedUnit(unitId);
			bool justFocusedUnit = JustFocusedUnit();
			bool disableFlashPrevention = isFocusedUnit && justFocusedUnit;
			
			if (unit.GetDisplayAnimationEnum(!disableFlashPrevention) == UnitAnimationEnum::ImmigrationCart) {
				_auxMeshes->Add("Immigration", unitId, getAuxTransform(), 0);
			}
			if (unit.GetDisplayAnimationEnum(!disableFlashPrevention) == UnitAnimationEnum::HaulingCart) {
				_auxMeshes->Add("HaulingCart", unitId, getAuxTransform(), 0);
				displayResource(7);
				return;
			}
		}

		displayResource(unit.isEnum(UnitEnum::Human) ? 4 : 8);
		
		////! Resource Display
		//ResourceEnum heldEnum = unit.inventory().Display();
		//if (heldEnum != ResourceEnum::None)
		//{
		//	FTransform transform = transformIn;

		//	//! TODO: !!! hack around vertex animate laziness
		//	//if (IsAnimal(unit.unitEnum())) {
		//	FRotator rotator = transform.Rotator();
		//	transform.SetScale3D(FVector(1, 1, 1));
		//	rotator.Yaw += 90;
		//	//}


		//	// Display smaller resource on units
		//	transform.SetScale3D(FVector(0.7, 0.7, 0.7));

		//	float inventoryShift = unit.isEnum(UnitEnum::Human) ? 4 : 8;

		//	FVector horizontalShift = rotator.RotateVector(FVector(inventoryShift, 0, 0));
		//	transform.SetTranslation(transform.GetTranslation() + horizontalShift + FVector(0, 0, 10));

		//	// Zeros the rotation
		//	FRotator resourceRotator = transform.Rotator();
		//	resourceRotator.Yaw -= 90;
		//	FTransform resourceTransform(resourceRotator, transform.GetTranslation(), transform.GetScale3D());

		//	_resourceMeshes->Add(ResourceDisplayNameF(heldEnum), unitId, resourceTransform, 0, unitId);
		//}
	}
}