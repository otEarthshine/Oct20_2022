// Fill out your copyright notice in the Description page of Project Settings.

#include "GameUIDataSource.h"
#include "PunWidget.h"

// Add default functionality here for any IGameUIDataSource functions that are not pure virtual.



void LaborerPriorityState::RefreshUI(
	class UPunWidget* widget,
	IGameSimulationCore* sim,
	int32 playerId,

	UHorizontalBox* EmployedBox,
	UTextBlock* Employed,

	UHorizontalBox* LaborerBox,
	UButton* LaborerPriorityButton, UButton* LaborerNonPriorityButton, USizeBox* LaborerArrowOverlay,
	UTextBlock* Laborer,
	UTextBlock* LaborerRed,

	UHorizontalBox* BuilderBox,
	UButton* BuilderNonPriorityButton,
	UButton* BuilderPriorityButton,
	UTextBlock* Builder,
	USizeBox* BuilderArrowOverlay
	)
{
	// Employed
	{
		auto& playerOwned = sim->playerOwned(playerId);

		int32 totalJobSlots = 0;
		const std::vector<std::vector<int32>>& jobBuildingEnumToIds = playerOwned.jobBuildingEnumToIds();
		for (const std::vector<int32>& buildingIds : jobBuildingEnumToIds) {
			for (int32 buildingId : buildingIds) {
				totalJobSlots += sim->building(buildingId).allowedOccupants();
			}
		}

		std::stringstream ss;

		if (EmployedBox) {
			widget->AddToolTip(EmployedBox, "People assigned to buildings\n/ Total buildings' job slots");
			ss << playerOwned.employedCount_WithoutBuilder() << "/" << totalJobSlots;
		} else {
			ss << "Employed: " << playerOwned.employedCount_WithoutBuilder() << "/" << totalJobSlots;
		}

		widget->SetText(Employed, ss.str());
	}

	
	// Laborer
	FString laborerString = FString::FromInt(laborerCount);
	if (townPriorityState.laborerPriority) {
		laborerString += FString("/") + FString::FromInt(townPriorityState.targetLaborerCount);
	}
	SetPriorityButtons(LaborerPriorityButton, LaborerNonPriorityButton, LaborerArrowOverlay, townPriorityState.laborerPriority);

	if (laborerCount == 0) {
		LaborerRed->SetText(FText::FromString(laborerString));
		LaborerRed->SetVisibility(ESlateVisibility::HitTestInvisible);
		Laborer->SetVisibility(ESlateVisibility::Collapsed);

		widget->AddToolTip(LaborerRed, "People not assigned to buildings become laborers. Laborers haul goods and gather trees/stone.");
	}
	else {
		Laborer->SetText(FText::FromString(laborerString));
		Laborer->SetVisibility(ESlateVisibility::HitTestInvisible);
		LaborerRed->SetVisibility(ESlateVisibility::Collapsed);

		widget->AddToolTip(LaborerBox, "People not assigned to buildings become laborers. Laborers haul goods and gather trees/stone.");
	}


	// Builder
	FString builderString = FString::FromInt(builderCount);
	if (townPriorityState.builderPriority) {
		builderString += FString("/") + FString::FromInt(townPriorityState.targetBuilderCount);
	}
	SetPriorityButtons(BuilderPriorityButton, BuilderNonPriorityButton, BuilderArrowOverlay, townPriorityState.builderPriority);
	Builder->SetText(FText::FromString(builderString));

	widget->AddToolTip(BuilderBox, "People assigned to buildings under construction.");
}