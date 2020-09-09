// Pun Dumnernchanvanit's

#pragma once

#include "CoreMinimal.h"
#include "PunCity/UI/PunWidget.h"
#include "Components/Overlay.h"
#include "TopLayerGameUI.generated.h"

/**
 * 
 */
UCLASS()
class PROTOTYPECITY_API UTopLayerGameUI : public UPunWidget
{
	GENERATED_BODY()
public:
	/*
	 * GamePause
	 */
	UPROPERTY(meta = (BindWidget)) UOverlay* GamePauseOverlay;
	UPROPERTY(meta = (BindWidget)) UTextBlock* GamePauseText;


	UPROPERTY(meta = (BindWidget)) UOverlay* MidScreenMessagePermanent;
	UPROPERTY(meta = (BindWidget)) UTextBlock* MidScreenMessageTextPermanent;


	void PunInit()
	{
		GamePauseOverlay->SetVisibility(ESlateVisibility::Collapsed);
		MidScreenMessagePermanent->SetVisibility(ESlateVisibility::Collapsed);
	}

	void Tick()
	{
		// Game Pause
		int32 gameSpeed = simulation().gameSpeed();
		if (networkInterface()->IsHost()) {
			gameSpeed = networkInterface()->hostGameSpeed();

			if (!simulation().playerOwned(playerId()).hasChosenLocation()) {
				gameSpeed = 0;
			}
		}


		{
			auto& playerOwned = simulation().playerOwned(playerId());
			
			bool showPause = false;
			if (!playerOwned.hasChosenLocation()) {
				SetText(GamePauseText, "Choose a starting location");
				showPause = true;
			}
			else if (!playerOwned.hasTownhall()) {
				if (playerOwned.initialResources.isValid()) {
					SetText(GamePauseText, "Place the townhall");
				} else {
					SetText(GamePauseText, "Confirm initial resources");
				}
				showPause = true;
			}
			// If not all players chose location, warn so 
			else {
				if (simulation().AllPlayerChoseLocationAfterInitialTicks())
				{
					if (gameSpeed == 0) {
						SetText(GamePauseText, "Game Paused");
						showPause = true;
					}
				}
				else {
					SetText(GamePauseText, "Waiting for other players\n to choose location...");
					showPause = true;
				}
			}
			
			GamePauseOverlay->SetVisibility(showPause ? ESlateVisibility::HitTestInvisible : ESlateVisibility::Collapsed);
		}


		// Midscreen message (AlwaysOn
		{
			PlacementInfo placementInfo = inputSystemInterface()->PlacementBuildingInfo();

			auto setMidscreenText = [&](std::string str) {
				MidScreenMessagePermanent->SetVisibility(ESlateVisibility::Visible);
				SetText(MidScreenMessageTextPermanent, str);
			};

			OverlayType overlayType = dataSource()->GetOverlayType();

			if (dataSource()->ZoomDistanceBelow(WorldToMapZoomAmount))
			{
				if (overlayType == OverlayType::Appeal) {
					setMidscreenText("Overlay: Appeal");
				}
				else if (overlayType == OverlayType::Farm) {
					setMidscreenText("Overlay: Fertility");
				}
				else if (overlayType == OverlayType::Fish) {
					setMidscreenText("Overlay: Appeal");
				}
				else if (overlayType == OverlayType::Hunter) {
					if (dataSource()->ZoomDistanceBelow(WorldZoomTransition_Unit)) {
						setMidscreenText("Overlay: Animals");
					}
					else {
						setMidscreenText("Please zoom in to see overlay");
					}
				}
				else {
					MidScreenMessagePermanent->SetVisibility(ESlateVisibility::Collapsed);
				}
			}
			else
			{
				if (overlayType != OverlayType::None) {
					setMidscreenText("Please zoom in to see overlay");
				}
				else {
					MidScreenMessagePermanent->SetVisibility(ESlateVisibility::Collapsed);
				}
			}
		}
	}
	
};
