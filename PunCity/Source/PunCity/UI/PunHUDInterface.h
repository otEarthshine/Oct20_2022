// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "UObject/Interface.h"
#include "UISystemBase.h"

#include "PunHUDInterface.generated.h"

// This class does not need to be modified.
UINTERFACE(MinimalAPI)
class UPunHUDInterface : public UInterface
{
	GENERATED_BODY()
};

/**
 * For UI classes to communicate with HUD
 */
class IPunHUDInterface
{
	GENERATED_BODY()

	// Add interface functions to this class. This is the class that will be inherited to implement this interface.
public:
	virtual UUserWidget* AddWidgetToHUD(UIEnum uiEnum) = 0;
	virtual UUserWidget* AddWidget(UIEnum uiEnum) = 0;

	virtual UWidget* SpawnWidget(UIEnum uiEnum) = 0;
	virtual void DespawnWidget(UIEnum uiEnum, UWidget* widget) = 0;

	//virtual class UGraphDataSource* SpawnGraphDataSource() = 0;
	//virtual void DespawnGraphDataSourcet(UGraphDataSource* widget) = 0;

	virtual TSubclassOf<UUserWidget> GetPunWidgetClass(UIEnum uiEnum) = 0;

	virtual IGameUIDataSource* dataSource() = 0;
	virtual IGameUIInputSystemInterface* inputSystemInterface() = 0;
	virtual IGameNetworkInterface* networkInterface() = 0;

	virtual int32 playerId() = 0;
	virtual int32 currentTownId() = 0;
	
	virtual UWorld* GetWorldPun() = 0;

	virtual bool ShouldPauseGameFromUI() = 0;

	virtual void OpenTradeUI(int32 objectId) = 0;
	virtual void OpenIntercityTradeUI(int32 objectId) = 0;
	virtual void OpenTargetConfirmUI_IntercityTrade(int32 townhallId, ResourceEnum resourceEnum) = 0;

	virtual void OpenSendImmigrantsUI(int32 townIdIn) = 0;
	virtual void OpenGiftUI(int32 sourcePlayerIdIn, int32 targetTownIdIn, TradeDealStageEnum dealStageEnumIn) = 0;
	virtual void OpenDiplomacyUI(int32 targetPlayerId) = 0;
	virtual void OpenTrainUnitsUI(int32 townIdIn) = 0;

	virtual void OpenTownAutoTradeUI(int32 townIdIn) = 0;

	virtual void ToggleTechUI() = 0;
	virtual void ToggleProsperityUI() = 0;

	virtual void TogglePlayerDetails() = 0;
	virtual void SwitchToNextBuildingUI() = 0;

	virtual void CloseDescriptionUI() = 0;

	virtual bool IsStatisticsUIOpened() = 0;
	virtual void OpenStatisticsUI(int32 townIdIn) = 0;
	virtual void OpenJobPriorityUI(int32 townIdIn) = 0;
	virtual void OpenReinforcementUI(int32 provinceId, CallbackEnum callbackEnum) = 0;
	
	virtual bool IsResourcePriceUIOpened(ResourceEnum resourceEnum) = 0;
	virtual void OpenResourcePriceUI(ResourceEnum resourceEnum) = 0;

	virtual bool IsFoodFuelGraphUIOpened() = 0;
	virtual void OpenFoodFuelGraphUI() = 0;

	virtual void ToggleGraphUI(int32 widgetIndex) = 0;
	
	virtual void CloseStatisticsUI() = 0;

	virtual void ShowTutorialUI(TutorialLinkEnum linkEnum) = 0;

	//virtual void OpenArmyMoveUI(std::shared_ptr<FAttack> armyCommand) = 0;

	virtual bool IsInvalid() = 0;
	virtual bool IsSimulationInvalid() = 0;

	virtual void QueueStickyExclusiveUI(ExclusiveUIEnum exclusiveUIEnum) = 0;
	virtual bool IsShowingExclusiveUI(ExclusiveUIEnum exclusiveUIEnum) = 0;
	virtual ExclusiveUIEnum GetCurrentExclusiveUIDisplay() = 0;
	//virtual int32 NumberOfUITasks() = 0;
	virtual bool IsDoingUITask() = 0;

	virtual TArray<UWidget*> GetEmptyCardSlot() = 0;

	// Actually just ResearchingText's font for now
	virtual FSlateFontInfo defaultFont() = 0;
};

#define BUTTON_ON_CLICK(button, UserObject, FuncName) \
			button->OnClicked.Clear();\
			button->OnClicked.AddUniqueDynamic(UserObject, FuncName);
