#include "CameraPawn.h"
#include "Camera/CameraComponent.h"
#include "Components/InputComponent.h"
#include "PunCity/Simulation/GameSimulationCore.h"
#include "Kismet/GameplayStatics.h"
#include "GameManager.h"

#include <algorithm>
#include "Kismet/KismetMathLibrary.h"

using namespace std;

ACameraPawn::ACameraPawn()
{
	PrimaryActorTick.bCanEverTick = true;
	bReplicates = false;
	bOnlyRelevantToOwner = true;

	RootComponent = CreateDefaultSubobject<USceneComponent>(TEXT("RootComponent"));

	CameraComponent = CreateDefaultSubobject<UCameraComponent>(TEXT("GameCamera"));
	CameraComponent->SetupAttachment(RootComponent);
}

void ACameraPawn::InitBuildingPlacementSystem(UAssetLoaderComponent* assetLoader)
{
	buildingPlacementSystem = GetWorld()->SpawnActor<ABuildingPlacementSystem>(ABuildingPlacementSystem::StaticClass());
	buildingPlacementSystem->Init(assetLoader);
}

// Called to bind functionality to input
void ACameraPawn::SetupPlayerInputComponent(UInputComponent* PlayerInputComponent)
{
	Super::SetupPlayerInputComponent(PlayerInputComponent);

	if (isMainMenuDisplay) {
		return;
	}

	PlayerInputComponent->BindAxis("MoveForward", this, &ACameraPawn::MoveForward);
	PlayerInputComponent->BindAxis("MoveRight", this, &ACameraPawn::MoveRight);
	PlayerInputComponent->BindAxis("Zoom", this, &ACameraPawn::Zoom);

	PlayerInputComponent->BindAxis("Pitch", this, &ACameraPawn::PitchCamera);
	PlayerInputComponent->BindAxis("Yaw", this, &ACameraPawn::YawCamera);

	PlayerInputComponent->BindAction("KeyPressed_R", IE_Pressed, this, &ACameraPawn::KeyPressed_R);
	PlayerInputComponent->BindAction("KeyPressed_M", IE_Pressed, this, &ACameraPawn::KeyPressed_M);
	PlayerInputComponent->BindAction("KeyPressed_H", IE_Pressed, this, &ACameraPawn::KeyPressed_H);
	PlayerInputComponent->BindAction("LeftMouseButton", IE_Pressed, this, &ACameraPawn::LeftMouseDown);
	PlayerInputComponent->BindAction("LeftMouseButton", IE_Released, this, &ACameraPawn::LeftMouseUp);

	PlayerInputComponent->BindAction("RightMouseButton", IE_Pressed, this, &ACameraPawn::RightMouseDown);

	PlayerInputComponent->BindAction("KeyPressed_Escape", IE_Pressed, this, &ACameraPawn::KeyPressed_Escape);

	PlayerInputComponent->BindAction("SpaceBar", IE_Pressed, this, &ACameraPawn::SpaceBarDown);
	PlayerInputComponent->BindAction("Num_0", IE_Pressed, this, &ACameraPawn::Num0Down);
	PlayerInputComponent->BindAction("Num_1", IE_Pressed, this, &ACameraPawn::Num1Down);
	PlayerInputComponent->BindAction("Num_2", IE_Pressed, this, &ACameraPawn::Num2Down);
	PlayerInputComponent->BindAction("Num_3", IE_Pressed, this, &ACameraPawn::Num3Down);
	PlayerInputComponent->BindAction("Num_4", IE_Pressed, this, &ACameraPawn::Num4Down);
	PlayerInputComponent->BindAction("Num_5", IE_Pressed, this, &ACameraPawn::Num5Down);

	PlayerInputComponent->BindAction("HideGameUI", IE_Pressed, this, &ACameraPawn::Input_HideGameUI);
	PlayerInputComponent->BindAction("HideJobUI", IE_Pressed, this, &ACameraPawn::Input_HideJobUI);
	PlayerInputComponent->BindAction("Enter", IE_Pressed, this, &ACameraPawn::Input_Enter);

	PlayerInputComponent->BindAction("Shift", IE_Pressed, this, &ACameraPawn::Input_ShiftDown);
	PlayerInputComponent->BindAction("Shift", IE_Released, this, &ACameraPawn::Input_ShiftUp);
	PlayerInputComponent->BindAction("Ctrl", IE_Pressed, this, &ACameraPawn::Input_CtrlDown);
	PlayerInputComponent->BindAction("Ctrl", IE_Released, this, &ACameraPawn::Input_CtrlUp);

	PlayerInputComponent->BindAction("PhotoMode", IE_Pressed, this, &ACameraPawn::TogglePhotoMode);

	PlayerInputComponent->BindAction("TestCrash", IE_Pressed, this, &ACameraPawn::TestCrash);

	PlayerInputComponent->BindAction("Build", IE_Pressed, this, &ACameraPawn::KeyPressed_Build);
	PlayerInputComponent->BindAction("Gather", IE_Pressed, this, &ACameraPawn::KeyPressed_Gather);
	PlayerInputComponent->BindAction("Cards", IE_Pressed, this, &ACameraPawn::KeyPressed_Cards);
	PlayerInputComponent->BindAction("Tech", IE_Pressed, this, &ACameraPawn::KeyPressed_Tech);

	PlayerInputComponent->BindAction("DirtRoad", IE_Pressed, this, &ACameraPawn::KeyPressed_DirtRoad);
	PlayerInputComponent->BindAction("Demolish", IE_Pressed, this, &ACameraPawn::KeyPressed_Demolish);

	PlayerInputComponent->BindAction("PlayerDetails", IE_Pressed, this, &ACameraPawn::KeyPressed_PlayerDetails);
	PlayerInputComponent->BindAction("Skill", IE_Pressed, this, &ACameraPawn::KeyPressed_LeaderSkill);
}

void ACameraPawn::MoveForward(float axisValue)
{
	_cameraMovementInput.X = FMath::Clamp<float>(axisValue, -1.0f, 1.0f);
}

void ACameraPawn::MoveRight(float axisValue)
{
	_cameraMovementInput.Y = FMath::Clamp<float>(axisValue, -1.0f, 1.0f);
}

void ACameraPawn::PitchCamera(float axisValue)
{
	_cameraRotateInput.Y = FMath::Clamp<float>(axisValue, -1.0f, 1.0f);
}

void ACameraPawn::YawCamera(float axisValue)
{
	_cameraRotateInput.X = FMath::Clamp<float>(axisValue, -1.0f, 1.0f);
}

void ACameraPawn::Zoom(float axisValue)
{
	// Don't mouse scroll zoom if IsHoveredOnScrollUI()
	if (!_networkInterface->IsHoveredOnScrollUI()) {
		_cameraZoomInputAxis = FMath::Clamp<float>(axisValue, -1.0f, 1.0f);
	}
}

void ACameraPawn::KeyPressed_R()
{
	buildingPlacementSystem->RotatePlacement();
	if (GEngine) GEngine->AddOnScreenDebugMessage(-1, 10.0f, FColor::White, FString::Printf(TEXT("KeyPressed_R")));
}

void ACameraPawn::TogglePhotoMode() {
	isPhotoMode = !isPhotoMode;
}

void ACameraPawn::TestCrash() {
	FDebug::DumpStackTraceToLog();
	checkNoEntry();
}

void ACameraPawn::KeyPressed_M()
{
	//_isWorldMap = !_isWorldMap;

	//// Swap rotator out from cache
	//FRotator currentRotator = GetActorRotation();
	//SetActorRotation(_cachedRotator);
	//_cachedRotator = currentRotator;

	//if (_isWorldMap) {
	//	_gameInterface->RefreshWorldMap();
	//}

	//_gameNetworkInterface->SetMainGameUIActive(!_isWorldMap);
}

void ACameraPawn::KeyPressed_H()
{
	WorldAtom2 lookAtAtom = _gameInterface->simulation().homeAtom(_networkInterface->playerId());
	if (lookAtAtom == WorldAtom2::Zero) {
		return;
	}
	
	SetCameraAtom(lookAtAtom);

	FRotator rotation = GetActorRotation();
	rotation.Yaw = 0;
	SetActorRotation(rotation);
}

void ACameraPawn::LeftMouseDown()
{
	PUN_LOG("LeftMouseDown");
	// Don't accept click before game actually start
	if (Time::Ticks() < 5) {
		return;
	}

#if !UI_ALL
	return;
#endif

	if (_networkInterface) {
		bool isNotPlacingBuilding = buildingPlacementSystem->placementState() == PlacementType::None;
		buildingPlacementSystem->LeftClickDown(_networkInterface);

		if (isNotPlacingBuilding) {
			_networkInterface->GetPunHUD()->LeftMouseDown();
		}
	}
}

void ACameraPawn::LeftMouseUp()
{
	PUN_LOG("LeftMouseUp");

#if !UI_ALL
	return;
#endif

	if (_networkInterface) {
		buildingPlacementSystem->LeftClickUp(_networkInterface);

		//if (buildingPlacementSystem->placementState() == PlacementType::None) {
		//	_gameNetworkInterface->GetPunHUD()->LeftMouseUp();
		//}
	}
}

void ACameraPawn::RightMouseDown()
{
#if !UI_ALL
	return;
#endif
	
	PUN_LOG("RightMouseDown");

	if (buildingPlacementSystem->placementState() != PlacementType::None) {
		buildingPlacementSystem->CancelPlacement();
		_gameInterface->Spawn2DSound("UI", "CancelPlacement");
	}

	
	if (_networkInterface) {
		_networkInterface->GetPunHUD()->RightMouseDown();
	}
}

void ACameraPawn::KeyPressed_Escape()
{
#if !UI_ALL
	return;
#endif
	
	if (buildingPlacementSystem->placementState() != PlacementType::None) {
		buildingPlacementSystem->CancelPlacement();
		_gameInterface->Spawn2DSound("UI", "CancelPlacement");
	}
	
	if (_networkInterface) {
		_networkInterface->GetPunHUD()->KeyPressed_Escape();
	}
}


void ACameraPawn::MoveCameraTo(WorldAtom2 atom, float zoomAmount, float timeLength)
{
	_systemMoveTimeSinceStart = 0.0f;
	_systemMoveTimeLength = timeLength;

	_systemCamLocationStart = _camShiftLocation;
	_systemCamLocationTarget = FVector(atom.x, atom.y, 0) / CoordinateConstants::AtomPerDisplayUnit;

	_systemZoomAmountStart = zoomDistance();
	_systemZoomAmountTarget = GetCameraZoomAmount(GetZoomStepFromAmount(zoomAmount)); // Go to new zoom step
}

void ACameraPawn::TickInputSystem(AGameManager* gameInterface, float DeltaTime, MouseInfo mouseInfo)
{
	Super::Tick(DeltaTime);

	// Input Mouse edge pan
	{
		FVector2D mouseLocation = _networkInterface->GetMousePositionPun();
		FVector2D viewportSize = _networkInterface->GetViewportSizePun();

		//float closestX = std::min(mouseLocation.X, viewportSize.X - mouseLocation.X);
		//float closestY = std::min(mouseLocation.Y, viewportSize.Y - mouseLocation.Y);
		//PUN_LOG("Mouse: %f %f", closestX, closestY);

		_cameraEdgeMovementInput = FVector2D(0, 0);

		if (mouseLocation.X >= 0.0f && mouseLocation.Y >= 0.0f)  // ensure validity
		{
			if (mouseLocation.X < 10) {
				_cameraEdgeMovementInput.Y = -1.0f;
			}
			else if (viewportSize.X - mouseLocation.X < 10) {
				_cameraEdgeMovementInput.Y = 1.0f;
			}

			if (mouseLocation.Y < 10) {
				_cameraEdgeMovementInput.X = 1.0f;
			}
			else if (viewportSize.Y - mouseLocation.Y < 10) {
				_cameraEdgeMovementInput.X = -1.0f;
			}
		}

	}

	// Rotate Yaw
	{
		FRotator rotation = GetActorRotation();
		rotation.Yaw += _cameraRotateInput.X;
		SetActorRotation(rotation);
	}

	// Rotate Pitch
	{
		FRotator rotation = GetActorRotation();
		rotation.Pitch += _cameraRotateInput.Y;
		rotation.Pitch = fmin(-10.0f, fmax(-89.0f, rotation.Pitch));
		SetActorRotation(rotation);
		//if (_cameraRotateInput.Y != 0) PUN_LOG("Pitch %f", rotation.Pitch);
	}


	// System movement
	if (isSystemMovingCamera())
	{
		
		_systemMoveTimeSinceStart += fmin(0.02, UGameplayStatics::GetWorldDeltaSeconds(GetWorld()));
		float lerpFraction = _systemMoveTimeSinceStart / _systemMoveTimeLength;
		float zoomLerpFraction = lerpFraction;
		float locationLerpFraction = 1 - exp(-lerpFraction * 20);
		float lerpedZoomAmount = _systemZoomAmountStart + (_systemZoomAmountTarget - _systemZoomAmountStart) * zoomLerpFraction;
		_camShiftLocation = _systemCamLocationStart + (_systemCamLocationTarget - _systemCamLocationStart) * locationLerpFraction;
		//PUN_LOG("System Move Camera %f, %f", zoomLerpFraction, locationLerpFraction);

		_cameraZoomStep = GetZoomStepFromAmount(lerpedZoomAmount); // Update this so that the surrounding gets updated...

		CameraComponent->SetRelativeLocation(FVector(-lerpedZoomAmount, 0, 0));
		if (lerpedZoomAmount < WorldToMapZoomAmount) {
			_state = DisplayState::World;
		}

		// Finish moving camera..
		if (_systemMoveTimeSinceStart > _systemMoveTimeLength) {
			_cameraZoomStep = GetZoomStepFromAmount(_systemZoomAmountTarget);
			_camShiftLocation = _systemCamLocationTarget;
			_systemMoveTimeSinceStart = -1.0f;
			
			//_gameNetworkInterface->SetMainGameUIActive(_state == DisplayState::World);
		}

		_smoothZoomDistance = lerpedZoomAmount;

		//PUN_LOG("isSystemMovingCamera lerp:%f step:%d", lerpedZoomAmount, _cameraZoomStep);
		
		return;
	}

	// Zoom
	{	
		int32 zoomStepSkip = static_cast<int32>(MinZoomSkipSteps + _zoomSpeedFraction * (MaxZoomSkipSteps - MinZoomSkipSteps));
		if (_cameraZoomInputAxis == -1.0f) 
		{
			int32 preferredZoomStep = min(MaxZoomStep, _cameraZoomStep + zoomStepSkip);
			//_cameraZoomStep = min(MaxZoomStep, _cameraZoomStep + zoomStepSkip);

			// Bounce to zoomStepAfterBounce
			if (!SimSettings::IsOn("NoCameraSnap") &&
				zoomStepBounceLow < preferredZoomStep && 
				preferredZoomStep < zoomStepBounceHigh)
			{
				if (_zoomSnapThresholdCount > 0) {
					_zoomSnapThresholdCount = 0;
				}
				
				// scrolling within 2 sec counts toward snapthreshold
				if (Time::Ticks() - _lastTryZoomSnapTick < Time::TicksPerSecond * 2) {
					_zoomSnapThresholdCount--;
				} else {
					_zoomSnapThresholdCount = 0;
				}

				PUN_LOG("_zoomSnapThresholdCount %d", _zoomSnapThresholdCount);

				if (_zoomSnapThresholdCount < -3) {
					_cameraZoomStep = zoomStepBounceHigh;
					_zoomSnapThresholdCount = 0;

					PUN_LOG("SNAP UP");
					//_isBounceZooming = true;
					//PUN_LOG("_isBounceZooming %d _zoomSnapThresholdCount:%d", _isBounceZooming, _zoomSnapThresholdCount);
				}
				//else {
				//	_isBounceZooming = false;
				//	PUN_LOG("_isBounceZooming %d _zoomSnapThresholdCount:%d", _isBounceZooming, _zoomSnapThresholdCount);
				//}

				_lastTryZoomSnapTick = Time::Ticks();
			}
			else {
				_cameraZoomStep = preferredZoomStep;
			}

			
		}
		if (_cameraZoomInputAxis == 1.0f) {
			int32 preferredZoomStep = max(MinZoomStep, _cameraZoomStep - zoomStepSkip);
			//_cameraZoomStep = max(MinZoomStep, _cameraZoomStep - zoomStepSkip);

			// Bounce to zoomStepBeforeBounce
			if (zoomStepBounceLow < preferredZoomStep && preferredZoomStep < zoomStepBounceHigh)
			{
				if (_zoomSnapThresholdCount < 0) {
					_zoomSnapThresholdCount = 0;
				}
				
				// scrolling within 2 sec counts toward snapthreshold
				// Once threshold reached, start snapping..
				if (Time::Ticks() - _lastTryZoomSnapTick < Time::TicksPerSecond * 2) {
					_zoomSnapThresholdCount++;
				} else {
					_zoomSnapThresholdCount = 0;
				}

				PUN_LOG("_zoomSnapThresholdCount %d", _zoomSnapThresholdCount);

				if (_zoomSnapThresholdCount > 3) {
					_cameraZoomStep = zoomStepBounceLow;
					_zoomSnapThresholdCount = 0;

					PUN_LOG("SNAP DOWN");
					//_isBounceZooming = true;
					//PUN_LOG("_isBounceZooming %d _zoomSnapThresholdCount:%d", _isBounceZooming, _zoomSnapThresholdCount);
				}
				//else {
				//	_isBounceZooming = false;
				//	PUN_LOG("_isBounceZooming %d _zoomSnapThresholdCount:%d", _isBounceZooming, _zoomSnapThresholdCount);
				//}

				_lastTryZoomSnapTick = Time::Ticks();
			}
			else {
				_cameraZoomStep = preferredZoomStep;
			}
		}
		//PUN_LOG("_cameraZoomStep:%d _isBounceZooming %d _zoomSnapThresholdCount:%d", _cameraZoomStep, _isBounceZooming, _zoomSnapThresholdCount);
	}

	// Switch between world/map mode...
	DisplayState newState = zoomDistance() > WorldToMapZoomAmount ? DisplayState::Map : DisplayState::World;
	//DisplayState oldState = _state;
	//if (oldState == DisplayState::Transition) {
	//	// Was transitioning... fully transition + refreshing UI state
	//	_state = newState;
	//	_gameNetworkInterface->SetMainGameUIActive(_state == DisplayState::World);
	//}
	//else if (oldState == DisplayState::World && newState == DisplayState::Map) {
	//	_state = DisplayState::Transition;
	//	_gameInterface->RefreshWorldMap();
	//}
	//else if (oldState == DisplayState::Map && newState == DisplayState::World) {
	//	_state = DisplayState::Transition;
	//}

	if (_state == DisplayState::World && newState == DisplayState::Map) {
		_state = DisplayState::Map;
		//_gameNetworkInterface->SetMainGameUIActive(false);
	}
	else if (_state == DisplayState::Map && newState == DisplayState::World) {
		_state = DisplayState::World;
		//_gameNetworkInterface->SetMainGameUIActive(true);
	}

	

	float lastZoomAmount = zoomDistance();

	// Set camera component zoom
	{
		// During transition state, isMapZoom should use the old state
		// This prevent zoom flash while the display system catch up
		//DisplayState stateForMapZoom = (_state == DisplayState::Transition) ? oldState : _state;
		//float zoomAmount = (stateForMapZoom == DisplayState::Map) ? (lastZoomAmount / 32) : lastZoomAmount; // map is x32 smaller than world
		float zoomAmount = lastZoomAmount;

		// Smooth camera zoom
		//  larger diff = faster zoom
		const float zoomSmoothSpeed = 5.0f; //20.0f
		float lastComponentZoomLocation = -CameraComponent->GetRelativeLocation().X;
		float diff = zoomAmount - lastComponentZoomLocation;
		_smoothZoomDistance = lastComponentZoomLocation + diff * DeltaTime * zoomSmoothSpeed;

		// Already at targetPosition, stop...
		if (diff > 0 && _smoothZoomDistance > zoomAmount) {
			_smoothZoomDistance = zoomAmount;
		}
		else if (diff < 0 && _smoothZoomDistance < zoomAmount) {
			_smoothZoomDistance = zoomAmount;
		}

		if (abs(zoomAmount - WorldToMapZoomAmount) < 200.0f) {
			_smoothZoomDistance = zoomAmount;
		}
		
		CameraComponent->SetRelativeLocation(FVector(-_smoothZoomDistance, 0, 0));

		// Always point the camera at 0.5 MinZoomAmount Height
		float camHeight = CameraComponent->GetComponentLocation().Z;
		float degreeFromHorizontal =  UKismetMathLibrary::DegAtan((camHeight - MinZoomLookAtHeight) / _smoothZoomDistance);
		CameraComponent->SetRelativeRotation(FRotator(41.7 - degreeFromHorizontal, 0, 0));

		if (Time::Ticks() % 30 == 0)
		{
			//PUN_LOG("Zoom %f, %f, %f", camHeight, smoothZoomAmount, degreeFromHorizontal);
			//PUN_LOG("Zoom %d, %f, %f", _cameraZoomStep, cameraZoomAmount(), zoomAmount);
		}
	}

	// Movement
	{
		float moveSpeed = moveSpeedAtMinZoomAmount * lastZoomAmount / MinZoomAmount;
		FVector actorHorizontalForward = GetActorForwardVector();
		actorHorizontalForward.Z = 0.0f;

		FVector2D movementInput = _cameraMovementInput + _cameraEdgeMovementInput;
		
		FVector locationShift = actorHorizontalForward * movementInput.X * DeltaTime * moveSpeed;
		locationShift += GetActorRightVector() * movementInput.Y * DeltaTime * moveSpeed;

		_camShiftLocation += locationShift;

		if (_camShiftLocation.X < 0) {
			_camShiftLocation.X = 0;
		}
		if (_camShiftLocation.Y < 0) {
			_camShiftLocation.Y = 0;
		}
		
		float maxX = GameMapConstants::TilesPerWorldX * CoordinateConstants::DisplayUnitPerTile;
		if (_camShiftLocation.X > maxX) {
			_camShiftLocation.X = maxX;
		}
		float maxY = GameMapConstants::TilesPerWorldY * CoordinateConstants::DisplayUnitPerTile;
		if (_camShiftLocation.Y > maxY) {
			_camShiftLocation.Y = maxY;
		}

		//if (_cameraMovementInput.X > 0 || _cameraMovementInput.Y) {
		//	PUN_LOG("Movement %s", *_camShiftLocation.ToString());
		//}
	}

	// Testing
	//auto viewport = GEngine->GameViewport->Viewport;
	//PUN_LOG("viewport %s ... %s", *viewport->GetInitialPositionXY().ToString(), *viewport->GetSizeXY().ToString());

	//FVector2D screenPosition = _networkInterface->GetMousePositionPun();
	//PUN_LOG("mouse %s", *screenPosition.ToString());

	_gameInterface = gameInterface; // TODO: Where should we set gameInterface???

	//// Get mouse hit (ground position)
	//// 0.0f = mouseDirection.Z * t + mouseLocation.Z
	//float t = -mouseLocation.Z / mouseDirection.Z;
	//FVector mouseHitLocation = mouseLocation + mouseDirection * t;

	buildingPlacementSystem->TickPlacement(gameInterface, _networkInterface, mouseInfo, cameraAtom());
}