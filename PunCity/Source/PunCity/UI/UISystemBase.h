// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "UnrealEngine.h"
#include "Blueprint/UserWidget.h"
#include "Components/Button.h"
#include "GameUIDataSource.h"
#include "GameUIInputSystemInterface.h"
#include "PunCity/GameNetworkInterface.h"


class IPunPlayerController
{
public:
	virtual IGameUIDataSource* dataSource() = 0;
	virtual IGameUIInputSystemInterface* inputSystemInterface() = 0;
	virtual IGameNetworkInterface* networkInterface() = 0;
};

// Correction so we can transfer value from UMG editor directly
static FLinearColor UMGColor(float r, float g, float b, float a = 1.0f) {
	return FLinearColor(FMath::Pow(r,  1.0f / 2.2f), 
							FMath::Pow(g, 1.0f / 2.2f),
							FMath::Pow(b, 1.0f / 2.2f),
								FMath::Pow(a, 1.0f / 2.2f));
}

static const FLinearColor PunUIGreen = FLinearColor(0.3f, 0.6f, 0.15f, 1.0f);
static const FLinearColor PunUIGreen2 = FLinearColor(0.3f, 0.6f, 0.15f, 1.0f);
static const FLinearColor PunUIDisableGray(.4f, .4f, .4f, 1.0f);

static const FLinearColor PunUIButtonRed = FLinearColor(.6f, 0.3f, 0.3f, 1.0f);

static const FLinearColor PunUIDoneGold(.8f, .7f, .1f, 1.0f);

static const FLinearColor PunUITechResearchedOuter = FLinearColor(.5, .5, .5, 1.0f);
static const FLinearColor PunUITechResearchedInner = FLinearColor(1.0, 1.0, 1.0, 1.0f);
static const FLinearColor PunUITechAvailableOuter = FLinearColor(.05, .05, .10, 1.0f);
static const FLinearColor PunUITechAvailableInner = FLinearColor(.6, .6, .6, 1.0f);
static const FLinearColor PunUITechSelectedOuter = FLinearColor(1.0f, 0.6f, 0.2f, 1.0f);
static const FLinearColor PunUITechSelectedInner = FLinearColor(1.0f, 1.0, 1.0, 1.0f);

enum class ButtonStateEnum
{
	Disabled,
	Enabled,
	Hidden,
	RedEnabled,
};

static void SetButtonEnabled(UButton* button, ButtonStateEnum state) 
{
	button->SetIsEnabled(state == ButtonStateEnum::Enabled || state == ButtonStateEnum::RedEnabled);

	
	if (state == ButtonStateEnum::Enabled) {
		button->SetBackgroundColor(PunUIGreen);
	}
	else if (state == ButtonStateEnum::RedEnabled) {
		button->SetBackgroundColor(PunUIButtonRed);
	}
	else {
		button->SetBackgroundColor(PunUIDisableGray);
	}
	
	button->SetVisibility(state != ButtonStateEnum::Hidden ? ESlateVisibility::Visible : ESlateVisibility::Collapsed);
}