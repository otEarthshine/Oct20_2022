#pragma once

#include "UnrealEngine.h"
#include "BuildingPlacementSystem.h"
#include "PunCity/MapUtil.h"
#include "PunCity/GameNetworkInterface.h"
#include "PunCity/UI/GameUIInputSystemInterface.h"
#include "PunCity/UI/PunHUD.h"

#include "GameFramework/Pawn.h"
#include "Camera/CameraComponent.h"

struct TrailerCameraRecord {
	WorldAtom2 cameraAtom;
	float zoomDistance;
	FRotator rotator;
	FString transition;
	float transitionTime;
};

#include "CameraPawn.generated.h"

/*
 * Manages Cameras and Inputs
 */
UCLASS()
class ACameraPawn : public APawn, public IGameUIInputSystemInterface
{
	GENERATED_BODY()
public:
	// Sets default values for this pawn's properties
	ACameraPawn();
	
	void InitBuildingPlacementSystem(UAssetLoaderComponent* assetLoader);

	// Called to bind functionality to input
	void SetupPlayerInputComponent(class UInputComponent* PlayerInputComponent) final;

	// Called after SetupCamera
	void SetGameInterfaces(IGameManagerInterface* gameInterface, IGameNetworkInterface* gameNetworkInterface) {
		_gameInterface = gameInterface;
		_networkInterface = gameNetworkInterface;

		// Start at the middle of the map
		WorldRegion2 mapSizeRegion = GetMapSize(gameInterface->GetMapSettings().mapSizeEnum());
		WorldAtom2 initialCamAtom = WorldRegion2(mapSizeRegion.x / 2, mapSizeRegion.y / 2).centerTile().worldAtom2();
		_camShiftLocation = FVector(initialCamAtom.x, initialCamAtom.y, 0) / CoordinateConstants::AtomPerDisplayUnit;
		//_cameraZoomStep = 30;
		_cameraZoomStep = 95;

		_state = DisplayState::World;
		
		_cachedRotator = FRotator(-60.0f, 0.0f, 0.0f);
	}

	void TickInputSystem(class AGameManager* gameInterface, float DeltaTime, MouseInfo mouseInfo);

	void MoveForward(float axisValue);
	void MoveRight(float axisValue);
	void PitchCamera(float axisValue);
	void YawCamera(float axisValue);
	void Zoom(float axisValue);

	void KeyPressed_R();
	void KeyPressed_M();
	void KeyPressed_H();
	void LeftMouseDown();
	void LeftMouseUp();
	void RightMouseDown();

	void KeyPressed_Build() {
		ExecuteUsingMainGameUI([&](auto ui) { ui->ToggleBuildingMenu(); });
	}
	void KeyPressed_Gather() {
		ExecuteUsingMainGameUI([&](auto ui) { ui->ToggleGatherButton(); });
	}
	void KeyPressed_Tech() {
		ExecuteUsingMainGameUI([&](auto ui) { ui->ToggleResearchMenu(); });
	}
	void KeyPressed_Cards() {
		ExecuteUsingMainGameUI([&](auto ui) { ui->ClickCardStackButton(); });
	}
	void KeyPressed_PlayerDetails() {
		ExecuteUsingHUD([&](APunHUD* hud) { hud->TogglePlayerDetails(); });
	}

	void KeyPressed_DirtRoad() {
		ExecuteUsingMainGameUI([&](auto ui) { ui->ToggleDirtRoad(); });
	}
	void KeyPressed_Demolish() {
		ExecuteUsingMainGameUI([&](auto ui) { ui->ToggleDemolishButton(); });
	}

	void KeyPressed_LeaderSkill() {
		ExecuteUsingMainGameUI([&](auto ui) { ui->OnClickLeaderSkillButton();  });
		//_networkInterface->GetPunHUD()->mainGameUI()->OnClickLeaderSkillButton();
	}

	template<typename Func>
	void ExecuteUsingHUD(Func func) {
#if UI_ALL
		func(_networkInterface->GetPunHUD());
#endif
	}
	template<typename Func>
	void ExecuteUsingMainGameUI(Func func) {
#if UI_ALL
		func(_networkInterface->GetPunHUD()->mainGameUI());
#endif
	}
	

	void TogglePhotoMode();

	void TestCrash();

	void KeyPressed_Escape();


	void SpaceBarDown() {
		if (_networkInterface->hostGameSpeed() == 0) {
			_networkInterface->Resume();
		} else {
			_networkInterface->Pause();
		}
	}
	
	void Num0Down() { _networkInterface->Pause(); PUN_LOG("Num0Down Pause"); }
	void Num1Down() { _networkInterface->SetGameSpeed(-12); }
	void Num2Down() { _networkInterface->SetGameSpeed(1); }
	void Num3Down() { _networkInterface->SetGameSpeed(2); }
	void Num4Down() { _networkInterface->SetGameSpeed(5); }
	void Num5Down() { _networkInterface->SetGameSpeed(10); }
	void Input_HideGameUI() { _networkInterface->GetPunHUD()->ToggleMainGameUIActive(); }
	void Input_HideJobUI() { /* _gameNetworkInterface->GetPunHUD()->ToggleJobUI();*/ }

	void Input_CtrlDown() { _gameInterface->SetCtrl(true); }
	void Input_CtrlUp() { _gameInterface->SetCtrl(false); }
	void Input_ShiftDown() { _gameInterface->SetShift(true); }
	void Input_ShiftUp() { _gameInterface->SetShift(false); }

	void Input_Enter() {
		if (_networkInterface->IsChatFocus()) {
			_networkInterface->SetFocusGame();
		} else {
			_networkInterface->SetFocusChat();
		}
	}

	UCameraComponent* camera() { return CameraComponent; }

	//! This where displaySystem gets its camera position. 
	WorldAtom2 cameraAtom() final {
		return WorldAtom2(static_cast<int32_t>(_camShiftLocation.X * CoordinateConstants::AtomPerDisplayUnit),
							static_cast<int32_t>(_camShiftLocation.Y * CoordinateConstants::AtomPerDisplayUnit));
	}

	/*
	 * Move Camera
	 */
	void SetCameraAtom(WorldAtom2 atom) {
		_camShiftLocation.X = atom.x / CoordinateConstants::AtomPerDisplayUnit;
		_camShiftLocation.Y = atom.y / CoordinateConstants::AtomPerDisplayUnit;
	}
	void SetZoomAmount(float zoomAmount) {
		_cameraZoomStep = GetZoomStepFromAmount(zoomAmount);
	}
	
	void MoveCameraTo(WorldAtom2 atom, float zoomAmount, float timeLength = 3.0f, FString lerpType = "ChooseLocation");

	void SetCameraSequence(std::vector<TrailerCameraRecord> cameraSequence) {
		_cameraSequence = cameraSequence;
	}
	void ClearCameraSequence() {
		_cameraSequence.clear();
	}
	

	/*
	 * Building placement
	 */
	void StartBuildingPlacement(CardEnum buildingEnum, int32_t buildingLvl, bool useBoughtCard, CardEnum useWildCard) final {
		_networkInterface->ResetGameUI();
		buildingPlacementSystem->StartBuildingPlacement(buildingEnum, buildingLvl, useBoughtCard, useWildCard);
	}
	void StartHarvestPlacement(bool isRemoving, ResourceEnum resourceEnum) final {
		_networkInterface->ResetGameUI();
		buildingPlacementSystem->StartHarvestPlacement(isRemoving, resourceEnum);
	}
	void StartDemolish() final {
		_networkInterface->ResetGameUI();
		buildingPlacementSystem->StartDemolish();
	}
	void StartRoadPlacement(bool isStoneRoad, bool isIntercity) final {
		_networkInterface->ResetGameUI();
		buildingPlacementSystem->StartRoad(isStoneRoad, isIntercity);
	}
	void StartFencePlacement() final {
		_networkInterface->ResetGameUI();
		buildingPlacementSystem->StartFence();
	}
	void StartBridgePlacement() final {
		_networkInterface->ResetGameUI();
		buildingPlacementSystem->StartBridge();
	}

	//int32_t GetCardHandIndexBeingPlaced() final { return buildingPlacementSystem->GetCardHandIndexBeingPlaced(); }
	CardEnum GetBuildingEnumBeingPlaced() final { return buildingPlacementSystem->GetBuildingEnumBeingPlaced(); }

	PlacementInfo PlacementBuildingInfo() final {
		return buildingPlacementSystem->PlacementBuildingInfo();
	}
	PlacementType placementState() final { return buildingPlacementSystem->placementState(); }
	void CancelPlacement() final { buildingPlacementSystem->CancelPlacement(); }


	//bool isMapMode() {
	//	return _state == DisplayState::Map;
	//}

	float zoomDistance() {
		return GetCameraZoomAmount(_cameraZoomStep);
	}
	float smoothZoomDistance() {
		return _smoothZoomDistance;
	}

	bool isBounceZooming() {
		return _isBounceZooming;
	}

	//FVector GetMapCameraLocation() {
	//	// camera always remains at (0,0,0) spot
	//	// Switching from world to map is actually changing the cameraShiftLocation.
	//	// But _camShiftLocation gives the actual location in the world where the camera is.
	//	// This function get the actual camera location taking into account the display mode (world/map)
	//	// Map's mesh is 128x256 size in blender with 1280x2560 in UE4 size, 
	//	// 1 tile-size in the world = 1 region-size in map = 10 displayUnit
	//	// Map is x32 smaller than the world.
	//	// TODO: refactor this out MapToWorldMultiple is 1 now...
	//	return _camShiftLocation / CoordinateConstants::MapToWorldMultiple;
	//}

	float GetMouseZoomSpeedFraction() final {
		return _zoomSpeedFraction;
	}
	void SetMouseZoomSpeedFraction(float fraction) final {
		_zoomSpeedFraction = fraction;
	}

	
	
public:
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Building Placement")
	ABuildingPlacementSystem* buildingPlacementSystem;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Camera Pawn")
	UCameraComponent* CameraComponent;

	bool isPhotoMode = false;

	bool isMainMenuDisplay = false;
	
private:
	bool isSystemMovingCamera() final { return _systemMoveTimeSinceStart != -1.0f; }

	// Move speed changes as we zoom in/out
	const float moveSpeedAtMinZoomAmount = 100;
	
private:
	// TODO: save zoom speed fraction to Settings
	float _zoomSpeedFraction = 0.5f;


	std::vector<TrailerCameraRecord> _cameraSequence;
	FString _systemLerpType = "ChooseLocation";
	
	float _systemMoveTimeSinceStart = -1.0f;
	float _systemMoveTimeLength = 0.0f;
	FVector _systemCamLocationStart;
	FVector _systemCamLocationTarget;
	float _systemZoomAmountStart = 0.0f;
	float _systemZoomAmountTarget = 0.0f;
	
	enum class DisplayState : uint8
	{
		World,
		Transition,
		Map,
	};

	DisplayState _state;

	FVector _camShiftLocation; // This isn't used directly, but is converted to cameraAtom() that is used by display system to move the world while the camera remains in place.

	//! Input changes
	FVector2D _cameraMovementInput;
	FVector2D _cameraEdgeMovementInput;
	
	FVector2D _cameraRotateInput;
	float _cameraZoomInputAxis = 0;
	
	int32 _cameraZoomStep = 0;
	float _smoothZoomDistance = 0;
	bool _isBounceZooming = false;

	FRotator _cachedRotator;

	IGameManagerInterface* _gameInterface;
	IGameNetworkInterface* _networkInterface;


	int32 _zoomSnapThresholdCount = 0;
	int32 _lastTryZoomSnapTick = 0;

public:
	void Serialize(FArchive& archive) final
	{
		archive << isPhotoMode;
		
		archive << _systemMoveTimeSinceStart;
		archive << _systemMoveTimeLength;
		archive << _systemCamLocationStart;
		archive << _systemCamLocationTarget;
		archive << _systemZoomAmountStart;
		archive << _systemZoomAmountTarget;
		
		archive << _state;
		
		archive << _camShiftLocation;
		
		archive << _cameraMovementInput;
		archive << _cameraEdgeMovementInput;
		archive << _cameraRotateInput;
		archive << _cameraZoomInputAxis;
		
		archive << _cameraZoomStep;
		archive << _cachedRotator;
	}
};
