// Pun Dumnernchanvanit's

#pragma once

#include "CoreMinimal.h"
#include "PunCity/UI/PunWidget.h"
#include "Components/WidgetSwitcher.h"
#include "MediaAssets/Public/MediaPlayer.h"

#include "TutorialUI.generated.h"

/**
 * 
 */
UCLASS()
class PROTOTYPECITY_API UTutorialUI : public UPunWidget
{
	GENERATED_BODY()
public:
	void PunInit()
	{
#define INIT_TAB(TabName) SwitchButtons.Add(TabName##Switch); (TabName##Switch)->OnClicked.AddDynamic(this, &UTutorialUI::OnClick##TabName);

		INIT_TAB(Overview);
		
		INIT_TAB(CameraControl);
		INIT_TAB(Resources);
		INIT_TAB(WoodAndStone);
		INIT_TAB(PlacingBuildings);
		INIT_TAB(BuildingCards);
		INIT_TAB(ClaimingTerritory);
		
		INIT_TAB(Housing);
		INIT_TAB(Food);
		INIT_TAB(SurvivingWinter);
		INIT_TAB(Money);

		INIT_TAB(Research);
		INIT_TAB(AssigningJobs);
		INIT_TAB(LeaderSkill);

		INIT_TAB(Happiness);
		
#undef INIT_TAB
		TutorialCloseButton->OnClicked.AddDynamic(this, &UTutorialUI::OnClickCloseButton);
		TutorialXButton->CoreButton->OnClicked.AddDynamic(this, &UTutorialUI::OnClickCloseButton);

		// Other links
		MoreHousingButton->OnClicked.AddDynamic(this, &UTutorialUI::OnClickHousing);
		MoreFoodButton->OnClicked.AddDynamic(this, &UTutorialUI::OnClickFood);
		MoreHeatButton->OnClicked.AddDynamic(this, &UTutorialUI::OnClickSurvivingWinter);

		MoreBuildingHouses->OnClicked.AddDynamic(this, &UTutorialUI::OnClickPlacingBuildings);
		MoreBuildingCards->OnClicked.AddDynamic(this, &UTutorialUI::OnClickBuildingCards);

		MoreGatheringButton->OnClicked.AddDynamic(this, &UTutorialUI::OnClickWoodAndStone);
		MoreFoodButton2->OnClicked.AddDynamic(this, &UTutorialUI::OnClickFood);
		
		// Start with viewing Overview
		OnClickSwitchButton(OverviewSwitch, OverviewPage);
	}

	void ShowTutorialUI(TutorialLinkEnum linkEnum) {
		switch (linkEnum)
		{
		case TutorialLinkEnum::CameraControl: OnClickCameraControl(); break;
		case TutorialLinkEnum::Happiness: OnClickHappiness(); break;
		default:
			break;
		}
	}

	UPROPERTY(meta = (BindWidget)) UWidgetSwitcher* TutorialSwitcher;
	UPROPERTY(meta = (BindWidget)) UButton* TutorialCloseButton;
	UPROPERTY(meta = (BindWidget)) UWGT_ButtonCpp* TutorialXButton;

	UPROPERTY() TArray<UButton*> SwitchButtons;
private:

	UPROPERTY(meta = (BindWidget)) UButton* OverviewSwitch; //
	
	UPROPERTY(meta = (BindWidget)) UButton* CameraControlSwitch;
	UPROPERTY(meta = (BindWidget)) UButton* ResourcesSwitch;
	UPROPERTY(meta = (BindWidget)) UButton* WoodAndStoneSwitch;
	UPROPERTY(meta = (BindWidget)) UButton* PlacingBuildingsSwitch;
	UPROPERTY(meta = (BindWidget)) UButton* BuildingCardsSwitch;
	UPROPERTY(meta = (BindWidget)) UButton* ClaimingTerritorySwitch;
	
	UPROPERTY(meta = (BindWidget)) UButton* HousingSwitch;
	UPROPERTY(meta = (BindWidget)) UButton* FoodSwitch;
	UPROPERTY(meta = (BindWidget)) UButton* SurvivingWinterSwitch;
	UPROPERTY(meta = (BindWidget)) UButton* MoneySwitch;
	
	UPROPERTY(meta = (BindWidget)) UButton* ResearchSwitch;
	UPROPERTY(meta = (BindWidget)) UButton* AssigningJobsSwitch; //
	UPROPERTY(meta = (BindWidget)) UButton* LeaderSkillSwitch;

	UPROPERTY(meta = (BindWidget)) UButton* HappinessSwitch;

	/*
	 * Pages
	 */
	UPROPERTY(meta = (BindWidget)) UVerticalBox* OverviewPage; //
	
	UPROPERTY(meta = (BindWidget)) UVerticalBox* CameraControl;
	UPROPERTY(meta = (BindWidget)) UVerticalBox* Resources;
	UPROPERTY(meta = (BindWidget)) UVerticalBox* WoodAndStone;
	UPROPERTY(meta = (BindWidget)) UVerticalBox* PlacingBuildings;
	UPROPERTY(meta = (BindWidget)) UVerticalBox* BuildingCards;
	UPROPERTY(meta = (BindWidget)) UVerticalBox* ClaimingTerritory;
	
	UPROPERTY(meta = (BindWidget)) UVerticalBox* Housing;
	UPROPERTY(meta = (BindWidget)) UVerticalBox* Food;
	UPROPERTY(meta = (BindWidget)) UVerticalBox* SurvivingWinter;
	UPROPERTY(meta = (BindWidget)) UVerticalBox* Money;
	
	UPROPERTY(meta = (BindWidget)) UVerticalBox* Research;
	UPROPERTY(meta = (BindWidget)) UVerticalBox* AssigningJobs;
	UPROPERTY(meta = (BindWidget)) UVerticalBox* LeaderSkill;

	UPROPERTY(meta = (BindWidget)) UVerticalBox* Happiness;

	//
	UPROPERTY(meta = (BindWidget)) UButton* MoreHousingButton;
	UPROPERTY(meta = (BindWidget)) UButton* MoreFoodButton;
	UPROPERTY(meta = (BindWidget)) UButton* MoreHeatButton;
	
	UPROPERTY(meta = (BindWidget)) UButton* MoreBuildingHouses;
	UPROPERTY(meta = (BindWidget)) UButton* MoreBuildingCards;

	UPROPERTY(meta = (BindWidget)) UButton* MoreGatheringButton;
	UPROPERTY(meta = (BindWidget)) UButton* MoreFoodButton2;


	UFUNCTION() void OnClickOverview() { OnClickSwitchButton(OverviewSwitch, OverviewPage); }
	
	UFUNCTION() void OnClickCameraControl() { OnClickSwitchButton(CameraControlSwitch, CameraControl); PlayMedia_CameraControl();  }
	UFUNCTION() void OnClickResources() { OnClickSwitchButton(ResourcesSwitch, Resources); }
	UFUNCTION() void OnClickWoodAndStone() { OnClickSwitchButton(WoodAndStoneSwitch, WoodAndStone); PlayMedia_WoodAndStone();}
	UFUNCTION() void OnClickPlacingBuildings() { OnClickSwitchButton(PlacingBuildingsSwitch, PlacingBuildings); PlayMedia_PlacingBuildings(); }
	UFUNCTION() void OnClickBuildingCards() { OnClickSwitchButton(BuildingCardsSwitch, BuildingCards); PlayMedia_BuildingCards(); }
	UFUNCTION() void OnClickClaimingTerritory() { OnClickSwitchButton(ClaimingTerritorySwitch, ClaimingTerritory); PlayMedia_ClaimingTerritory(); }
	
	UFUNCTION() void OnClickHousing() { OnClickSwitchButton(HousingSwitch, Housing); PlayMedia_Housing(); }
	UFUNCTION() void OnClickFood() { OnClickSwitchButton(FoodSwitch, Food); }
	UFUNCTION() void OnClickSurvivingWinter() { OnClickSwitchButton(SurvivingWinterSwitch, SurvivingWinter); }
	UFUNCTION() void OnClickMoney() { OnClickSwitchButton(MoneySwitch, Money); }
	
	UFUNCTION() void OnClickResearch() { OnClickSwitchButton(ResearchSwitch, Research); PlayMedia_Research();  }
	UFUNCTION() void OnClickAssigningJobs() { OnClickSwitchButton(AssigningJobsSwitch, AssigningJobs); PlayMedia_AssigningJobs(); }
	UFUNCTION() void OnClickLeaderSkill() { OnClickSwitchButton(LeaderSkillSwitch, LeaderSkill); PlayMedia_LeaderSkill(); }

	UFUNCTION() void OnClickHappiness() { OnClickSwitchButton(HappinessSwitch, Happiness); }

public:
	UFUNCTION(BlueprintImplementableEvent) void PlayMedia_CameraControl();
	UFUNCTION(BlueprintImplementableEvent) void PlayMedia_WoodAndStone();
	UFUNCTION(BlueprintImplementableEvent) void PlayMedia_PlacingBuildings();
	UFUNCTION(BlueprintImplementableEvent) void PlayMedia_BuildingCards();
	UFUNCTION(BlueprintImplementableEvent) void PlayMedia_ClaimingTerritory();

	UFUNCTION(BlueprintImplementableEvent) void PlayMedia_Housing();

	UFUNCTION(BlueprintImplementableEvent) void PlayMedia_Research();
	UFUNCTION(BlueprintImplementableEvent) void PlayMedia_AssigningJobs();
	UFUNCTION(BlueprintImplementableEvent) void PlayMedia_LeaderSkill();

	UFUNCTION(BlueprintImplementableEvent) void CloseMedia();

private:
	//
	UFUNCTION() void OnClickCloseButton() {
		CloseMedia();
		
		SetVisibility(ESlateVisibility::Collapsed);
		Spawn2DSound("UI", "UIWindowClose");
	}
	
	void ResetTabSelection()
	{
		for (int32 i = 0; i < SwitchButtons.Num(); i++) {
			SetButtonHighlight(SwitchButtons[i], false);
		}
	}

	void OnClickSwitchButton(UButton* tabButton, UVerticalBox* tabUI)
	{
		CloseMedia();
		
		ResetTabSelection();
		SetButtonHighlight(tabButton, true);
		TutorialSwitcher->SetActiveWidget(tabUI);

		Spawn2DSound("UI", "ButtonClick");
	}
	
};
