// Pun Dumnernchanvanit's


#include "TopLayerGameUI.h"

#define LOCTEXT_NAMESPACE "TopLayerGameUI"

void UTopLayerGameUI::Tick()
{
	LEAN_PROFILING_UI(TickTopLayerGameUI);
	
	// Game Pause
	int32 gameSpeed = simulation().gameSpeed();
	if (networkInterface()->IsHost()) {
		gameSpeed = networkInterface()->hostGameSpeed();

		if (!simulation().HasChosenLocation(playerId())) {
			gameSpeed = 0;
		}
	}


	{
		auto& playerOwned = simulation().playerOwned(playerId());

		bool showPause = false;
		if (!playerOwned.hasChosenLocation()) {
			SetText(GamePauseText, LOCTEXT("Choose a starting location", "Choose a starting location"));
			showPause = true;
		}
		else if (!playerOwned.hasCapitalTownhall()) {
			if (playerOwned.initialResources.isValid()) {
				SetText(GamePauseText, LOCTEXT("Place the townhall", "Place the townhall"));
			}
			else {
				SetText(GamePauseText, LOCTEXT("Confirm initial resources", "Confirm initial resources"));
			}
			showPause = true;
		}
		else if (GetPunHUD()->ShouldPauseGameFromUI())
		{
			SetText(GamePauseText, LOCTEXT("Game Paused", "Game Paused"));
			showPause = true;
		}
		// If not all players chose location, warn so 
		else {
			if (simulation().AllPlayerHasTownhallAfterInitialTicks())
			{
				if (gameSpeed == 0) {
					SetText(GamePauseText, LOCTEXT("Game Paused", "Game Paused"));
					showPause = true;
				}
			}
			else {
				SetText(GamePauseText, LOCTEXT("WaitForOthersToChooseLoc", "Waiting for other players\n to choose location..."));
				showPause = true;
			}
		}

		GamePauseOverlay->SetVisibility(showPause ? ESlateVisibility::HitTestInvisible : ESlateVisibility::Collapsed);
	}


	// Midscreen message (AlwaysOn
	{
		PlacementInfo placementInfo = inputSystemInterface()->PlacementBuildingInfo();

		auto setMidscreenText = [&](FText text) {
			MidScreenMessagePermanent->SetVisibility(ESlateVisibility::Visible);
			SetText(MidScreenMessageTextPermanent, text);
		};

		OverlayType overlayType = dataSource()->GetOverlayType();

		if (dataSource()->ZoomDistanceBelow(WorldToMapZoomAmount))
		{
			if (overlayType == OverlayType::Appeal) {
				setMidscreenText(LOCTEXT("Overlay: Appeal", "Overlay: Appeal"));
			}
			else if (overlayType == OverlayType::Farm) {
				setMidscreenText(LOCTEXT("Overlay: Fertility", "Overlay: Fertility"));
			}
			else if (overlayType == OverlayType::Fish) {
				setMidscreenText(LOCTEXT("Overlay: Appeal", "Overlay: Appeal"));
			}
			else if (overlayType == OverlayType::Hunter) {
				if (dataSource()->ZoomDistanceBelow(WorldZoomTransition_Unit)) {
					setMidscreenText(LOCTEXT("Overlay: Animals", "Overlay: Animals"));
				}
				else {
					setMidscreenText(LOCTEXT("Please zoom in to see overlay", "Please zoom in to see overlay"));
				}
			}
			else {
				MidScreenMessagePermanent->SetVisibility(ESlateVisibility::Collapsed);
			}
		}
		else
		{
			if (overlayType != OverlayType::None) {
				setMidscreenText(LOCTEXT("Please zoom in to see overlay", "Please zoom in to see overlay"));
			}
			else {
				MidScreenMessagePermanent->SetVisibility(ESlateVisibility::Collapsed);
			}
		}
	}
}


#undef LOCTEXT_NAMESPACE