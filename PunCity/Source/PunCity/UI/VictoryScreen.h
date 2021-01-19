// Pun Dumnernchanvanit's

#pragma once

#include "CoreMinimal.h"
#include "PunCity/UI/PunWidget.h"
#include "Components/WidgetSwitcher.h"
#include "TimeSeriesPlot.h"
#include "PunCity/VictoryScreenController.h"
#include "VictoryScreen.generated.h"

/**
 * 
 */
UCLASS()
class PROTOTYPECITY_API UVictoryScreen : public UPunWidget
{
	GENERATED_BODY()
public:
	UPROPERTY(meta = (BindWidget)) UTextBlock* VictoryText;
	UPROPERTY(meta = (BindWidget)) UImage* VictoryBackgroundColor;
	UPROPERTY(meta = (BindWidget)) UImage* DefeatBackgroundColor;

	UPROPERTY(meta = (BindWidget)) UButton* PopulationStatButton;
	UPROPERTY(meta = (BindWidget)) UButton* IncomeStatButton;
	UPROPERTY(meta = (BindWidget)) UButton* ScienceStatButton;

	UPROPERTY(meta = (BindWidget)) UWidgetSwitcher* StatSwitcher;
	UPROPERTY(meta = (BindWidget)) UTimeSeriesPlot* PopulationGraph;
	UPROPERTY(meta = (BindWidget)) UTimeSeriesPlot* IncomeGraph;
	UPROPERTY(meta = (BindWidget)) UTimeSeriesPlot* ScienceGraph;

	UPROPERTY(meta = (BindWidget)) UButton* GoToMainMenuButton;

	void Init()
	{
		GoToMainMenuButton->OnClicked.AddDynamic(this, &UVictoryScreen::OnClickGoToMainMenuButton);
	}

	void Tick();


	UFUNCTION() void OnClickGoToMainMenuButton() {
		//auto firstController = Cast<AVictoryScreenController>(GetWorld()->GetFirstPlayerController());
		//check(UGameplayStatics::GetPlayerControllerID(firstController) == 0);
		GetWorld()->GetFirstPlayerController()->ClientTravel("/Game/Maps/MainMenu", TRAVEL_Absolute);
	}
};
