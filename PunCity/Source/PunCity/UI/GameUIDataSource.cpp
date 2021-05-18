// Fill out your copyright notice in the Description page of Project Settings.

#include "GameUIDataSource.h"
#include "PunWidget.h"

// Add default functionality here for any IGameUIDataSource functions that are not pure virtual.

#define LOCTEXT_NAMESPACE "GameUIDataSource"

void LaborerPriorityState::RefreshUILaborerPriority(
	class UPunWidget* widget,
	IGameSimulationCore* sim,
	int32 townId,

	UHorizontalBox* EmployedBox,
	UTextBlock* Employed,

	UHorizontalBox* LaborerBox,
	UCheckBox* LaborerManualCheckBox, USizeBox* LaborerArrowOverlay,
	UTextBlock* Laborer,
	UTextBlock* LaborerRed,

	UHorizontalBox* BuilderBox,
	UCheckBox* BuilderManualCheckBox,
	UTextBlock* Builder,
	USizeBox* BuilderArrowOverlay
	)
{
	// Employed
	{
		auto& townManager = sim->townManager(townId);

		int32 totalJobSlots = 0;
		const std::vector<std::vector<int32>>& jobBuildingEnumToIds = townManager.jobBuildingEnumToIds();
		for (const std::vector<int32>& buildingIds : jobBuildingEnumToIds) {
			for (int32 buildingId : buildingIds) {
				totalJobSlots += sim->building(buildingId).allowedOccupants();
			}
		}

		TArray<FText> args;

		if (EmployedBox) {
			widget->AddToolTip(EmployedBox, 
				LOCTEXT("EmployedDesc", "People assigned to buildings\n/ Total buildings' job slots")
			);
			ADDTEXT_(INVTEXT("{0}/{1}"), TEXT_NUM(townManager.employedCount_WithoutBuilder()), TEXT_NUM(totalJobSlots));
		} else {
			ADDTEXT_(LOCTEXT("Employed: X/Y", "Employed: {0}/{1}"), TEXT_NUM(townManager.employedCount_WithoutBuilder()), TEXT_NUM(totalJobSlots));
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
		LaborerManualCheckBox->SetIsChecked(townPriorityState.laborerPriority);
		LaborerArrowOverlay->SetVisibility(townPriorityState.laborerPriority ? ESlateVisibility::Visible : ESlateVisibility::Collapsed);
		
		if (laborerCount == 0) {
			LaborerRed->SetText(JOINTEXT(args));
			LaborerRed->SetVisibility(ESlateVisibility::HitTestInvisible);
			Laborer->SetVisibility(ESlateVisibility::Collapsed);

			widget->AddToolTip(LaborerRed, 
				LOCTEXT("LaborerPriorityButtonAt0_Tip", "People not assigned to buildings become laborers. Laborers haul goods and gather trees/stone.")
			);
		}
		else {
			Laborer->SetText(JOINTEXT(args));
			Laborer->SetVisibility(ESlateVisibility::HitTestInvisible);
			LaborerRed->SetVisibility(ESlateVisibility::Collapsed);

			widget->AddToolTip(LaborerBox, 
				LOCTEXT("LaborerPriorityButton_Tip", "People not assigned to buildings become laborers. Laborers haul goods and gather trees/stone.")
			);
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
		BuilderManualCheckBox->SetIsChecked(townPriorityState.builderPriority);
		BuilderArrowOverlay->SetVisibility(townPriorityState.builderPriority ? ESlateVisibility::Visible : ESlateVisibility::Collapsed);
		
		Builder->SetText(JOINTEXT(args));

		widget->AddToolTip(BuilderBox, 
			LOCTEXT("BuilderBox_Tip", "People assigned to buildings under construction.")
		);
	}
}

#undef LOCTEXT_NAMESPACE 