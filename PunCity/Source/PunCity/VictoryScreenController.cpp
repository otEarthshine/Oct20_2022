// Pun Dumnernchanvanit's


#include "VictoryScreenController.h"

AVictoryScreenController::AVictoryScreenController() : APunBasePlayerController()
{
	PrimaryActorTick.bCanEverTick = true;
	bAllowTickBeforeBeginPlay = false;

	// Set player controller's statuses
	bShowMouseCursor = true;
	bEnableClickEvents = true;
	bEnableMouseOverEvents = true;
	FViewTargetTransitionParams params;
	SetViewTarget(this, params);

	
}