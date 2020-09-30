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

	PlayerInputComponent->BindAxis("Pitch", this, &ACameraPawn::PitchCamera); // Semicolon (;) and Slash (/)
	PlayerInputComponent->BindAxis("Yaw", this, &ACameraPawn::YawCamera);

	PlayerInputComponent->BindAction("KeyPressed_R", IE_Pressed, this, &ACameraPawn::KeyPressed_R);
	PlayerInputComponent->BindAction("KeyPressed_CtrlT", IE_Pressed, this, &ACameraPawn::KeyPressed_CtrlT);
	PlayerInputComponent->BindAction("KeyPressed_M", IE_Pressed, this, &ACameraPawn::KeyPressed_M);
	PlayerInputComponent->BindAction("KeyPressed_H", IE_Pressed, this, &ACameraPawn::KeyPressed_H);
	PlayerInputComponent->BindAction("KeyPressed_F", IE_Pressed, this, &ACameraPawn::KeyPressed_F);
	
	PlayerInputComponent->BindAction("LeftMouseButton", IE_Pressed, this, &ACameraPawn::LeftMouseDown);
	PlayerInputComponent->BindAction("LeftMouseButton", IE_Released, this, &ACameraPawn::LeftMouseUp);

	PlayerInputComponent->BindAction("RightMouseButton", IE_Pressed, this, &ACameraPawn::RightMouseDown);
	PlayerInputComponent->BindAction("RightMouseButton", IE_Released, this, &ACameraPawn::RightMouseUp);

	PlayerInputComponent->BindAction("MiddleMouseButton", IE_Pressed, this, &ACameraPawn::MiddleMouseDown);
	PlayerInputComponent->BindAction("MiddleMouseButton", IE_Released, this, &ACameraPawn::MiddleMouseUp);

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

void ACameraPawn::KeyPressed_CtrlT()
{
	if (SimSettings::IsOn("CheatHouseLevelKey")) {
		int32 oldLvl = SimSettings::Get("CheatHouseLevel");
		int32 newLvl = (oldLvl + 1) % (House::GetMaxHouseLvl() + 1);
		SimSettings::Set("CheatHouseLevel", newLvl);
		auto command = make_shared<FSendChat>();
		command->isSystemMessage = true;
		command->message = "House Level " + FString::FromInt(newLvl);
		_networkInterface->SendNetworkCommand(command);
	}
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


void ACameraPawn::LeftMouseDown()
{
	PUN_LOG("LeftMouseDown");
	// Don't accept click before game actually start
	if (Time::Ticks() < Time::TicksPerSecond) { // 1.5 sec is when HideLoadingScreen triggers
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

	if (_networkInterface) 
	{
		//bool isNotPlacingBuilding = buildingPlacementSystem->placementState() == PlacementType::None;
		buildingPlacementSystem->LeftClickUp(_networkInterface);

		//if (isNotPlacingBuilding) {
		//	_networkInterface->GetPunHUD()->LeftMouseUp();
		//}
	}
}

void ACameraPawn::RightMouseDown()
{
#if !UI_ALL
	return;
#endif
	
	PUN_LOG("RightMouseDown");

	
	//if (_networkInterface) {
	//	_networkInterface->GetPunHUD()->RightMouseUp();
	//}

	isRightMouseDown = true;
	rightMouseDragStartAtom = _networkInterface->GetMouseGroundAtom();
	rightMouseDragCamStartAtom = cameraAtom();
}

void ACameraPawn::RightMouseUp()
{
#if !UI_ALL
	return;
#endif
	// Building placement cancel if not doing camera drag
	// If cam moved more than some amount, don't cancel placement
	if (WorldAtom2::Distance(cameraAtom(), rightMouseDragCamStartAtom) < CoordinateConstants::AtomsPerTile)
	{
		if (buildingPlacementSystem->placementState() != PlacementType::None) {
			buildingPlacementSystem->CancelPlacement();
			_gameInterface->Spawn2DSound("UI", "CancelPlacement");
		}

		if (_networkInterface) {
			_networkInterface->GetPunHUD()->RightMouseUp();
		}
	}

	
	isRightMouseDown = false;
}

void ACameraPawn::MiddleMouseDown()
{
	// FrostPunk: if right mouse down, don't activate middle mouse when clicked
	if (!isRightMouseDown)
	{
		isMiddleMouseDown = true;
		middleDragStartMousePosition = _networkInterface->GetMousePositionPun();
	}
}

void ACameraPawn::MiddleMouseUp()
{
	isMiddleMouseDown = false;
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


void ACameraPawn::MoveCameraTo(WorldAtom2 atom, float zoomAmount, float timeLength, FString lerpType)
{
	_systemLerpType = lerpType;
	
	_systemMoveTimeSinceStart = 0.0f;
	_systemMoveTimeLength = timeLength;

	_systemCamLocationStart = _camShiftLocation;
	_systemCamLocationTarget = FVector(atom.x, atom.y, 0) / CoordinateConstants::AtomPerDisplayUnit;

	_systemZoomAmountStart = smoothZoomDistance();
	_systemZoomAmountTarget = GetCameraZoomAmount(GetZoomStepFromAmount(zoomAmount)); // Go to new zoom step

	_LOG(PunDisplay, "MoveCameraTo time:%f", _systemMoveTimeLength);
}

void ACameraPawn::MoveCameraTo(WorldAtom2 atom, float zoomAmount, FRotator rotation, float lightAngle, float timeLength, FString lerpType)
{
	_trailerAccumulatedMoveTime += timeLength;
	float newTimeLength = fmax(timeLength * 0.5f, _trailerAccumulatedMoveTime - GetTrailerTime());
	
	_LOG(PunTrailer, "CAM Trailer MoveCameraTo time:%.2f zoom:%d..%.2f->%.2f rotation:%s  lightAngle:%.2f timeLength:%.2f new:%.2f", 
		GetTrailerTime(), _cameraZoomStep, smoothZoomDistance(), zoomAmount, *rotation.ToCompactString(), lightAngle, timeLength, newTimeLength);
	
	MoveCameraTo(atom, zoomAmount, newTimeLength, lerpType);

	_systemIsRotating = true;
	_systemRotatorStart = GetActorRotation();
	_systemRotatorTarget = rotation;

	if (lightAngle > 0)
	{
		_systemIsRotatingLight = true;
		_systemLightAngleStart = fmod(_networkInterface->GetLightAngle() + 360.0f, 360.0f);
		_systemLightAngleTarget = lightAngle;
	}
}

void ACameraPawn::AdjustCameraZoomTilt()
{
	if (!PunSettings::TrailerMode()) 
	{
		// Always point the camera at 0.X MinZoomAmount Height
		float camHeight = CameraComponent->GetComponentLocation().Z;
		float degreeFromHorizontal = UKismetMathLibrary::DegAtan((camHeight - MinZoomLookAtHeight) / _smoothZoomDistance);
		CameraComponent->SetRelativeRotation(FRotator(41.7 - degreeFromHorizontal, 0, 0));

		//PUN_LOG("Cam Trailer AdjustCameraZoomTilt:%s", *CameraComponent->GetRelativeRotation().ToCompactString());
	}
}

void ACameraPawn::TickInputSystem(AGameManager* gameInterface, float DeltaTime, MouseInfo mouseInfo)
{
	Super::Tick(DeltaTime);

	/*
	 * System movement
	 */
	if (isSystemMovingCamera())
	{
		if (PunSettings::TrailerMode()) {
			_systemMoveTimeSinceStart += fmin(0.02, UGameplayStatics::GetWorldDeltaSeconds(GetWorld()));
			//PUN_LOG("Cam Trail shot:%.2f/%.2f time:%.2f", _systemMoveTimeSinceStart, _systemMoveTimeLength, _systemTrailerTimeSinceStart);
		}
		else {
			_systemMoveTimeSinceStart += fmin(0.02, UGameplayStatics::GetWorldDeltaSeconds(GetWorld()));
		}

		float lastSmoothZoomDistance = _smoothZoomDistance;
		float lastPitch = CameraComponent->GetRelativeRotation().Pitch;
		
		float lerpFraction = Clamp01(_systemMoveTimeSinceStart / _systemMoveTimeLength);

		// Lerp functions
		float zoomLerpFraction = lerpFraction;
		float locationLerpFraction = lerpFraction;
		float rotationLerpFraction = lerpFraction;
		if (_systemLerpType == "ChooseLocation") {
			locationLerpFraction = 1 - exp(-lerpFraction * 20);
		}
		else if (_systemLerpType == "Instant") {
			zoomLerpFraction = 1;
			locationLerpFraction = 1;
		}

		// Lerped Amount
		float lerpedZoomAmount = _systemZoomAmountStart + (_systemZoomAmountTarget - _systemZoomAmountStart) * zoomLerpFraction;
		_smoothZoomDistance = lerpedZoomAmount;

		FVector lastCamShiftLocation = _camShiftLocation;
		_camShiftLocation = _systemCamLocationStart + (_systemCamLocationTarget - _systemCamLocationStart) * locationLerpFraction;


		// Get Old Value to check for jump
		//GetActorRotation()
		
		FRotator rotator;
		FRotator lastRotator = GetActorRotation();
		if (_systemIsRotating) {
			rotator = FMath::Lerp(_systemRotatorStart, _systemRotatorTarget, rotationLerpFraction);
			SetActorRotation(rotator);
		}
		if (_systemIsRotatingLight) {
			float lightAngle = _systemLightAngleStart + (_systemLightAngleTarget - _systemLightAngleStart) * lerpFraction;
			//PUN_LOG("Cam Trailer Update light:%f", lightAngle);
			_networkInterface->SetLightAngle(lightAngle);
		}
		
		//PUN_LOG("System Move Camera %f, %f", zoomLerpFraction, locationLerpFraction);

		// Zoom Adjust
		_cameraZoomStep = GetZoomStepFromAmount(lerpedZoomAmount); // Update this so that the surrounding gets updated...
		CameraComponent->SetRelativeLocation(FVector(-lerpedZoomAmount, 0, 0));

		float zoomDiff = lerpedZoomAmount - lastSmoothZoomDistance;
		float yawDiff = rotator.Yaw - lastRotator.Yaw;
		float locationDiff = FVector::Distance(lastCamShiftLocation, _camShiftLocation);
		if (zoomDiff > 1.0f) _LOG(PunTrailer, "Cam Trailer Zoom Step:%d zoom:%f diff:%f target:%f, rotator:%s", _cameraZoomStep, lerpedZoomAmount, zoomDiff, _systemZoomAmountTarget);
		if (yawDiff > 0.5f) _LOG(PunTrailer, "Cam Trailer Rotation last:%s now:%s diff:%f", *lastRotator.ToCompactString(), *rotator.ToCompactString(), yawDiff);
		if (locationDiff > 1.0f) _LOG(PunTrailer, "Cam Trailer Location last:%s now:%s diff:%f", *lastCamShiftLocation.ToCompactString(), *_camShiftLocation.ToCompactString(), locationDiff);

		// Adjust tilt up if this isn't trailer
		AdjustCameraZoomTilt();

		float tiltDiff = lastPitch - CameraComponent->GetRelativeRotation().Pitch;
		if (tiltDiff > 0.5f) _LOG(PunTrailer, "Cam Trailer Tilt last:%f now:%f diff:%f", lastPitch, CameraComponent->GetRelativeRotation().Pitch, tiltDiff);
		
		if (lerpedZoomAmount < WorldToMapZoomAmount) {
			_state = DisplayState::World;
		}

		// Finish moving camera..
		if (_systemMoveTimeSinceStart >= _systemMoveTimeLength) 
		{
			_cameraZoomStep = GetZoomStepFromAmount(_systemZoomAmountTarget);
			_smoothZoomDistance = _systemZoomAmountTarget;
			
			_camShiftLocation = _systemCamLocationTarget;

			CameraComponent->SetRelativeLocation(FVector(-_systemZoomAmountTarget, 0, 0));

			if (_systemIsRotating) {
				SetActorRotation(_systemRotatorTarget);
				_systemIsRotating = false;
			}
			//PUN_LOG("!!! Cam Trailer Finish zoomStep:%d %f, rotator:%s", _cameraZoomStep, _systemZoomAmountTarget, *_systemRotatorTarget.ToCompactString());
			
			if (_systemIsRotatingLight) {
				//PUN_LOG("Cam Trailer Finish light:%f", _systemLightAngleTarget);
				_networkInterface->SetLightAngle(_systemLightAngleTarget);
				_systemIsRotatingLight = false;
			}
			
			_systemMoveTimeSinceStart = -1.0f;

			if (PunSettings::TrailerMode()) {
				_LOG(PunTrailer, "CAM Trailer Move Done time:%f", GetTrailerTime());
			}
		}

		//PUN_LOG("isSystemMovingCamera lerp:%f step:%d", lerpedZoomAmount, _cameraZoomStep);
		
		return;
	}

	/*
	 * Trailer sequence
	 */
	if (_cameraSequence.size() > 0)
	{
		TrailerCameraRecord cameraRecord = _cameraSequence[0];
		_cameraSequence.erase(_cameraSequence.begin());

		// Spawn Trailer Music on second cam sequence
		if (!_trailerSoundStarted &&
			GetTrailerTime() > 3.0f)
		{
			_trailerSoundStarted = true;
			_gameInterface->Spawn2DSound("UI", "TrailerMusic");
			_trailerStartTime = UGameplayStatics::GetAudioTimeSeconds(this);
			_trailerAccumulatedMoveTime = 0.0f;

			_networkInterface->ExecuteInitialCloudFade();
		}

		if (fabs(cameraRecord.lightAngle - 800.0f) < 0.1f) {
			FTimerHandle UnusedHandle;
			GetWorldTimerManager().SetTimer(UnusedHandle, this, &ACameraPawn::StartForceSnow, PunSettings::Get("ForceSnowDelay"));
			//PunSettings::Set("ForceSnow", 1);
			cameraRecord.lightAngle = 225.0f;
		}
		if (fabs(cameraRecord.lightAngle - 1000.0f)  < 0.1f) {
			// Arrive at Snow
			_networkInterface->TrailerShipStart();
			PunSettings::Set("TrailerNoTreeRefresh", 0);
			cameraRecord.lightAngle = 225.0f;
		}
		if (fabs(cameraRecord.lightAngle - 1200.0f) < 0.1f) {
			// Leave Jungle
			PunSettings::Set("TrailerNoTreeRefresh", 1);
			SimSettings::Set("ToggleRain", 0);
			cameraRecord.lightAngle = 225.0f;
		}
		if (fabs(cameraRecord.lightAngle - 1400.0f) < 0.1f) {
			PunSettings::Set("ForceAutumn", 1);
			cameraRecord.lightAngle = 225.0f;
		}
		if (fabs(cameraRecord.lightAngle - 1500.0f) < 0.1f) {
			PunSettings::Set("ForceAutumn", 0);
			cameraRecord.lightAngle = 225.0f;
		}
		if (fabs(cameraRecord.lightAngle - 2000.0f) < 0.1f) {
			PunSettings::Set("ForceSnow", 0);
			cameraRecord.lightAngle = 225.0f;
		}

		if (fabs(cameraRecord.lightAngle - 3000.0f) < 0.1f) {
			// Arrive at Jungle
			SimSettings::Set("ToggleRain", 1);
			cameraRecord.lightAngle = 45.0f;
		}

		MoveCameraTo(cameraRecord.cameraAtom, cameraRecord.zoomDistance, cameraRecord.rotator, cameraRecord.lightAngle, cameraRecord.transitionTime, cameraRecord.transition);

		if (cameraRecord.isCameraReplayUnpause) {
			_networkInterface->TrailerCityReplayUnpause();
		}

		return;
	}
	
	if (PunSettings::TrailerMode())
	{
		if (_trailerStartTime != -1) {
			_LOG(PunTrailer, "Cam Trail ended time:%.2f", GetTrailerTime());
			_systemMoveTimeSinceStart = -1.0f;
			_systemMoveTimeLength = 0.0f;
			_trailerStartTime = -1;
			PunSettings::SetTrailerMode(false);
		}
	}

	/*
	 * MANUAL CONTROL BEYOND
	 *  In FrostPunk:
	 *  - Keyboard precedes mouseControl. If keyboard control is activated, don't update mouse control
	 *  - If right or middle mouse down, don't edge pan
	 *  
	 */


	/*
	 * Keyboard Rotate
	 */
	// Rotate Yaw
	FRotator rotation;
	{
		rotation = GetActorRotation();
		rotation.Yaw += _cameraRotateInput.X;
		SetActorRotation(rotation);
	}
	bool isKeyboardRotating = !_cameraRotateInput.Equals(FVector2D::ZeroVector);

	// Rotate Pitch
	if (PunSettings::IsOn("AllowCameraPitch"))
	{
		rotation = GetActorRotation();
		rotation.Pitch += _cameraRotateInput.Y;
		rotation.Pitch = fmin(-10.0f, fmax(-89.0f, rotation.Pitch));
		SetActorRotation(rotation);
		//if (_cameraRotateInput.Y != 0) PUN_LOG("Pitch %f", rotation.Pitch);
	}
	

	/*
	 * Scroll Zoom
	 * - this set the target _cameraZoomStep
	 *
	 * Note: FrostPunk scroll zoom is disabled when the middle mouse is down
	 */
	if (!isMiddleMouseDown)
	{	
		int32 zoomStepSkip = static_cast<int32>(MinZoomSkipSteps + zoomSpeedFraction * (MaxZoomSkipSteps - MinZoomSkipSteps));
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
		if (_cameraZoomInputAxis == 1.0f) 
		{
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

	

	/*
	 * Mouse Zoom Updates
	 * - Use _cameraZoomStep to calculate the smooth camera zoom movement
	 */
	float lastZoomAmount = zoomDistance();
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
		
		AdjustCameraZoomTilt();

		if (Time::Ticks() % 30 == 0)
		{
			//PUN_LOG("Zoom %f, %f, %f", camHeight, smoothZoomAmount, degreeFromHorizontal);
			//PUN_LOG("Zoom %d, %f, %f", _cameraZoomStep, cameraZoomAmount(), zoomAmount);
		}

	}


	//PUN_LOG("Cam Update zoomStep:%d %f, rotator:%s", _cameraZoomStep, _smoothZoomDistance, *rotation.ToCompactString());
	

	/*
	 * Keyboard Movement
	 */
	auto moveCamera = [&](FVector2D movementInput)
	{
		float moveSpeed = moveSpeedAtMinZoomAmount * lastZoomAmount / MinZoomAmount;
		
		FVector actorHorizontalForward = GetActorForwardVector();
		actorHorizontalForward.Z = 0.0f;
		
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
	};
	
	{
		moveCamera(_cameraMovementInput);
		
		//float moveSpeed = moveSpeedAtMinZoomAmount * lastZoomAmount / MinZoomAmount;

		//FVector2D movementInput = _cameraMovementInput + _cameraEdgeMovementInput;
		
		//FVector actorHorizontalForward = GetActorForwardVector();
		//actorHorizontalForward.Z = 0.0f;

		//FVector2D movementInput = _cameraMovementInput + _cameraEdgeMovementInput;
		//
		//FVector locationShift = actorHorizontalForward * movementInput.X * DeltaTime * moveSpeed;
		//locationShift += GetActorRightVector() * movementInput.Y * DeltaTime * moveSpeed;

		//_camShiftLocation += locationShift;

		//if (_camShiftLocation.X < 0) {
		//	_camShiftLocation.X = 0;
		//}
		//if (_camShiftLocation.Y < 0) {
		//	_camShiftLocation.Y = 0;
		//}
		//
		//float maxX = GameMapConstants::TilesPerWorldX * CoordinateConstants::DisplayUnitPerTile;
		//if (_camShiftLocation.X > maxX) {
		//	_camShiftLocation.X = maxX;
		//}
		//float maxY = GameMapConstants::TilesPerWorldY * CoordinateConstants::DisplayUnitPerTile;
		//if (_camShiftLocation.Y > maxY) {
		//	_camShiftLocation.Y = maxY;
		//}

		////if (_cameraMovementInput.X > 0 || _cameraMovementInput.Y) {
		////	PUN_LOG("Movement %s", *_camShiftLocation.ToString());
		////}
	}
	bool isKeyboardMoving = !_cameraMovementInput.Equals(FVector2D::ZeroVector, 0.01);


	/*
	 * Right Mouse Down Drag Pan
	 *  - FrostPunk: Keyboard precedes mouseControl. If keyboard control is activated, don't update mouse control
	 */
	FVector2D currentMousePosition = _networkInterface->GetMousePositionPun();
	
	if (isRightMouseDown &&
		!isKeyboardMoving && !isKeyboardRotating)
	{
		WorldAtom2 currentGroundAtom = _networkInterface->GetMouseGroundAtom();

		// Special case out of bound return mouse to the initial position
		if (currentGroundAtom == WorldAtom2::Invalid) {
			SetCameraAtom(rightMouseDrag_LastWorkingAtom);
		}
		else
		{
			WorldAtom2 camAtomMoved = cameraAtom() - rightMouseDragCamStartAtom; // camera moved from its drag start pos

			// if camAtom doesn't move, the shift from right mouse drag would be  camAtom - (currentMouseGroundAtom - rightMouseDragStartAtom)
			//  but camAtom will move and must be corrected with camAtomMoved

			// currentGroundAtom is affected by cameraAtom's adjustment and must be readjusted
			WorldAtom2 currentMouseGroundAtomShiftedback = currentGroundAtom - camAtomMoved;

			WorldAtom2 rightMouseMovedAtom = currentMouseGroundAtomShiftedback - rightMouseDragStartAtom;
			WorldAtom2 newCameraAtom = rightMouseDragCamStartAtom - rightMouseMovedAtom;
			
			rightMouseDrag_LastWorkingAtom = newCameraAtom;
			SetCameraAtom(newCameraAtom);
		}
	}

	/*
	 * Middle Mouse Down Drag Rotate
	 */
	if (isMiddleMouseDown &&
		!isKeyboardMoving && !isKeyboardRotating)
	{
		float mousePositionDiffX = currentMousePosition.X - middleDragStartMousePosition.X;
		float halfViewportSizeX = _networkInterface->GetViewportSizePun().X / 2.0f;

		float anglePerHalfScreen = MinMouseRotateAnglePerHalfScreenMove + (MaxMouseRotateAnglePerHalfScreenMove - MinMouseRotateAnglePerHalfScreenMove) * mouseRotateSpeedFraction;
		float diffAngle = mousePositionDiffX * anglePerHalfScreen / halfViewportSizeX;

		rotation = GetActorRotation();
		rotation.Yaw += diffAngle;
		SetActorRotation(rotation);
		
		// Mouse always stay in the same spot when doing drag Rotate
		_networkInterface->SetMouseLocationPun(middleDragStartMousePosition);
	}

	/*
	 * Input Mouse edge pan
	 */
	if (!isRightMouseDown && !isMiddleMouseDown && 
		!isKeyboardMoving && !isKeyboardRotating)
	{
		FVector2D mouseLocation = _networkInterface->GetMousePositionPun();
		FVector2D viewportSize = _networkInterface->GetViewportSizePun();

		//float closestX = std::min(mouseLocation.X, viewportSize.X - mouseLocation.X);
		//float closestY = std::min(mouseLocation.Y, viewportSize.Y - mouseLocation.Y);
		//PUN_LOG("Mouse: %f %f", closestX, closestY);

		_cameraEdgeMovementInput = FVector2D::ZeroVector;

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

		if (!_cameraEdgeMovementInput.Equals(FVector2D::ZeroVector)) {
			moveCamera(_cameraEdgeMovementInput);
		}
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