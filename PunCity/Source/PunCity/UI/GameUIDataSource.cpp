// Fill out your copyright notice in the Description page of Project Settings.

#include "GameUIDataSource.h"
#include "PunWidget.h"

// Add default functionality here for any IGameUIDataSource functions that are not pure virtual.

#define LOCTEXT_NAMESPACE "GameUIDataSource"

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

		TArray<FText> args;

		if (EmployedBox) {
			widget->AddToolTip(EmployedBox, LOCTEXT("EmployedDesc", "People assigned to buildings\n/ Total buildings' job slots"));
			ADDTEXT_(INVTEXT("{0}/{1}"), TEXT_NUM(playerOwned.employedCount_WithoutBuilder()), TEXT_NUM(totalJobSlots));
		} else {
			ADDTEXT_(LOCTEXT("Employed: X/Y", "Employed: {0}/{1}"), TEXT_NUM(playerOwned.employedCount_WithoutBuilder()), TEXT_NUM(totalJobSlots));
		}

		Employed->SetText(JOINTEXT(args));
	}

	
	// Laborer
	{
		TArray<FText> args;
		ADDTEXT_NUM_(laborerCount);
		if (townPriorityState.laborerPriority) {
			ADDTEXT_INV_("/");
			ADDTEXT_NUM_(townPriorityState.targetLaborerCount);
		}
		SetPriorityButtons(LaborerPriorityButton, LaborerNonPriorityButton, LaborerArrowOverlay, townPriorityState.laborerPriority);

		if (laborerCount == 0) {
			LaborerRed->SetText(JOINTEXT(args));
			LaborerRed->SetVisibility(ESlateVisibility::HitTestInvisible);
			Laborer->SetVisibility(ESlateVisibility::Collapsed);

			widget->AddToolTip(LaborerRed, "People not assigned to buildings become laborers. Laborers haul goods and gather trees/stone.");
		}
		else {
			Laborer->SetText(JOINTEXT(args));
			Laborer->SetVisibility(ESlateVisibility::HitTestInvisible);
			LaborerRed->SetVisibility(ESlateVisibility::Collapsed);

			widget->AddToolTip(LaborerBox, "People not assigned to buildings become laborers. Laborers haul goods and gather trees/stone.");
		}
	}


	// Builder
	{
		TArray<FText> args;
		ADDTEXT_NUM_(builderCount);
		if (townPriorityState.builderPriority) {
			ADDTEXT_INV_("/");
			ADDTEXT_NUM_(townPriorityState.targetBuilderCount);
		}
		SetPriorityButtons(BuilderPriorityButton, BuilderNonPriorityButton, BuilderArrowOverlay, townPriorityState.builderPriority);
		Builder->SetText(JOINTEXT(args));

		widget->AddToolTip(BuilderBox, "People assigned to buildings under construction.");
	}
}

#undef LOCTEXT_NAMESPACE 