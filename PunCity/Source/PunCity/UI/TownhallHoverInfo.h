// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "PunWidget.h"
#include "PunCity/Simulation/Buildings/GathererHut.h"
#include "ArmyUnitUI.h"
#include "DamageFloatupUI.h"
#include "ArmyLinesUI.h"
#include "PunSimpleButton.h"
#include "PunBoxWidget.h"
#include "BuffIcon.h"
#include "MinorTownWorldUI.h"

#include "TownhallHoverInfo.generated.h"


/**
 * 
 */
UCLASS()
class UTownhallHoverInfo : public UMinorTownWorldUI
{
	GENERATED_BODY()
public:
	void PunInit(int townIdIn);

	void UpdateUI(bool isMini);

	//ArmyNode _lastArmyNode; // Used to compare to the current armyNode and show damage...
	//TArray<UWidget*> _damageFloatups;targetTownId

	/*
	 * Buttons callback
	 */
	float _lastAllyRequestTick = 0;
	
	void CallBack1(UPunWidget* punWidgetCaller, CallbackEnum callbackEnum) override
	{
		auto& sim = simulation();

		if (callbackEnum == CallbackEnum::IntercityTrade)
		{
			ResourceEnum resourceEnum = CastChecked<UChooseResourceElement>(punWidgetCaller)->resourceEnum;
			_LOG(LogNetworkInput, "Callback1 IntercityTrade %s", ToTChar(ResourceName(resourceEnum)));
			
			GetPunHUD()->OpenTargetConfirmUI_IntercityTrade(townBuildingId(), resourceEnum);
			return;
		}

		UE_DEBUG_BREAK();
	}
	
	void RefreshUI()
	{
		//int32 townId = simulation().building(_buildingId).subclass<TownHall>(CardEnum::Townhall).townId();
		
		_laborerPriorityState.RefreshUILaborerPriority(
			this,
			&simulation(),
			townId(),

			EmployedBox,
			Employed,

			LaborerBox,
			LaborerManualCheckBox, LaborerArrowOverlay,
			Laborer,
			LaborerRed,

			BuilderBox,
			BuilderManualCheckBox,
			Builder,
			BuilderArrowOverlay
		);

		
		// RoadMaker
		// TODO: Remove?
		//FString roadMakerString = FString::FromInt(_laborerPriorityState.roadMakerCount);
		//if (_laborerPriorityState.townPriorityState.roadMakerPriority) {
		//	roadMakerString += FString("/") + FString::FromInt(_laborerPriorityState.townPriorityState.targetRoadMakerCount);
		//}
		//_laborerPriorityState.SetPriorityButtons(RoadMakerPriorityButton, RoadMakerNonPriorityButton, RoadMakerArrowOverlay, _laborerPriorityState.townPriorityState.roadMakerPriority);
		//RoadMaker->SetText(FText::FromString(roadMakerString));

	}

public:
	//UPROPERTY(meta = (BindWidget)) UVerticalBox* BattleBox;
	//UPROPERTY(meta = (BindWidget)) UCanvasPanel* ArmyFightBox;
	//UPROPERTY(meta = (BindWidget)) UVerticalBox* LeftArmyBox;
	//UPROPERTY(meta = (BindWidget)) UVerticalBox* RightArmyBox;
	//UPROPERTY(meta = (BindWidget)) USizeBox* FightIcon;
	
	//UPROPERTY(meta = (BindWidget)) UVerticalBox* MilitaryButtons;

	UPROPERTY(meta = (BindWidget)) UButton* TradeButton;
	UPROPERTY(meta = (BindWidget)) UTextBlock* TradeButtonText;
	//UPROPERTY(meta = (BindWidget)) UOverlay* TradeInfoOverlay;
	//UPROPERTY(meta = (BindWidget)) UPunBoxWidget* BuyingBox;
	//UPROPERTY(meta = (BindWidget)) UPunBoxWidget* SellingBox;

	UPROPERTY(meta = (BindWidget)) UButton* SendImmigrantsButton;
	
	
	

	UPROPERTY(meta = (BindWidget)) UHorizontalBox* TrainUnitsRow;
	UPROPERTY(meta = (BindWidget)) UButton* TrainUnitsButton;
	UPROPERTY(meta = (BindWidget)) UImage* TrainUnitsClock;
	

	UPROPERTY(meta = (BindWidget)) UHorizontalBox* BuffRow;
	
	UPROPERTY(meta = (BindWidget)) UTextBlock* TownHoverPopulationText;
	UPROPERTY(meta = (BindWidget)) UImage* PlayerColorCircle;

	UPROPERTY(meta = (BindWidget)) UVerticalBox* LaborerBuilderBox;

	UPROPERTY(meta = (BindWidget)) UHorizontalBox* EmployedBox;
	UPROPERTY(meta = (BindWidget)) UTextBlock* Employed;

	UPROPERTY(meta = (BindWidget)) UHorizontalBox* LaborerBox;
	UPROPERTY(meta = (BindWidget)) UCheckBox* LaborerManualCheckBox;
	UPROPERTY(meta = (BindWidget)) UTextBlock* Laborer;
	UPROPERTY(meta = (BindWidget)) UTextBlock* LaborerRed;
	UPROPERTY(meta = (BindWidget)) UButton* LaborerArrowUp;
	UPROPERTY(meta = (BindWidget)) UButton* LaborerArrowDown;
	UPROPERTY(meta = (BindWidget)) USizeBox* LaborerArrowOverlay;

	UPROPERTY(meta = (BindWidget)) UHorizontalBox* BuilderBox;
	UPROPERTY(meta = (BindWidget)) UCheckBox* BuilderManualCheckBox;
	UPROPERTY(meta = (BindWidget)) UTextBlock* Builder;
	UPROPERTY(meta = (BindWidget)) UButton* BuilderArrowUp;
	UPROPERTY(meta = (BindWidget)) UButton* BuilderArrowDown;
	UPROPERTY(meta = (BindWidget)) USizeBox* BuilderArrowOverlay;

	UPROPERTY(meta = (BindWidget)) UButton* RoadMakerNonPriorityButton;
	UPROPERTY(meta = (BindWidget)) UButton* RoadMakerPriorityButton;
	UPROPERTY(meta = (BindWidget)) UTextBlock* RoadMaker;
	UPROPERTY(meta = (BindWidget)) UButton* RoadMakerArrowUp;
	UPROPERTY(meta = (BindWidget)) UButton* RoadMakerArrowDown;
	UPROPERTY(meta = (BindWidget)) USizeBox* RoadMakerArrowOverlay;

private:
	UFUNCTION() void ChangedCityName(const FText& Text, ETextCommit::Type CommitMethod);

	void SendNetworkPriority()
	{
		auto command = std::make_shared<FSetTownPriority>();
		*command = townPriorityState();
		command->townId = townId();
		networkInterface()->SendNetworkCommand(command);

		_laborerPriorityState.lastPriorityInputTime = UGameplayStatics::GetTimeSeconds(this);
	}


	UFUNCTION() void OnClickSetTradeOfferButton() {
		GetPunHUD()->OpenIntercityTradeUI(townBuildingId());
	}
	UFUNCTION() void OnClickEstablishTradeRouteButton()
	{
		auto command = make_shared<FSetIntercityTrade>();
		//command->townId = townId();
		//if (command->townId != -1) {
			command->buildingIdToEstablishTradeRoute = townBuildingId();
			networkInterface()->SendNetworkCommand(command);
		//}
	}
	UFUNCTION() void OnClickCancelTradeRouteButton()
	{
		auto command = make_shared<FSetIntercityTrade>();
		//command->townId = townId();
		//if (command->townId != -1) {
			command->buildingIdToEstablishTradeRoute = townBuildingId();
			command->isCancelingTradeRoute = 1;
			networkInterface()->SendNetworkCommand(command);
		//}
	}

	UFUNCTION() void OnClickSendImmigrantsButton() {
		int32 townId = simulation().building(townBuildingId()).townId();
		GetPunHUD()->OpenSendImmigrantsUI(townId);
	}
	
	//UFUNCTION() void OnClickGiftButton() {
	//	int32 targetTownId = simulation().building(townBuildingId()).townId();
	//	GetPunHUD()->OpenGiftUI(playerId(), targetTownId, TradeDealStageEnum::Gifting);
	//}
	UFUNCTION() void OnClickTradeDealButton() {
		int32 targetTownId = simulation().building(townBuildingId()).townId();
		GetPunHUD()->OpenGiftUI(playerId(), targetTownId, TradeDealStageEnum::CreateDeal);
	}
	
	//UFUNCTION() void OnClickDiplomacyButton() {
	//	int32 targetPlayerId = simulation().building(townBuildingId()).playerId();
	//	GetPunHUD()->OpenDiplomacyUI(targetPlayerId);
	//}
	UFUNCTION() void OnClickTrainUnitsButton() {
		int32 townId = simulation().building(townBuildingId()).townId();
		GetPunHUD()->OpenTrainUnitsUI(townId);
	}


	// Note: vassalize/independence/conquerColony are all CallbackEnum::StartAttackProvince for now
	// later on, there will be
	UFUNCTION() void OnClickVassalizeButton()
	{
		check(playerId() != townPlayerId());
		GetPunHUD()->OpenReinforcementUI(townProvinceId(), CallbackEnum::StartAttackProvince);
	}
	
	UFUNCTION() void OnClickDeclareIndependenceButton()
	{
		check(playerId() == townPlayerId());
		GetPunHUD()->OpenReinforcementUI(townProvinceId(), CallbackEnum::StartAttackProvince);
	}

	UFUNCTION() void OnClickConquerColonyButton()
	{
		check(playerId() == townPlayerId());
		GetPunHUD()->OpenReinforcementUI(townProvinceId(), CallbackEnum::StartAttackProvince);
	}

	UFUNCTION() void OnClickLiberateButton()
	{
		check(playerId() != townPlayerId());
		GetPunHUD()->OpenReinforcementUI(townProvinceId(), CallbackEnum::Liberate);
	}

	/*
	 * Laborer
	 */
	
	//UFUNCTION() void OnClickLaborerNonPriorityButton() {
	//	townPriorityState().laborerPriority = true;
	//	RefreshUI();
	//	SendNetworkPriority();
	//}
	//UFUNCTION() void OnClickLaborerPriorityButton() {
	//	townPriorityState().laborerPriority = false;
	//	RefreshUI();
	//	SendNetworkPriority();
	//}
	UFUNCTION() void OnCheckManualLaborer(bool isChecked) {
		townPriorityState().laborerPriority = isChecked;
		RefreshUI();
		SendNetworkPriority();
	}

	
	UFUNCTION() void IncreaseLaborers() {
		townPriorityState().targetLaborerCount++;
		RefreshUI();
		SendNetworkPriority();
		
		dataSource()->Spawn2DSound("UI", "UIIncrementalChange");
	}
	UFUNCTION() void DecreaseLaborers() {
		townPriorityState().targetLaborerCount = std::max(0, townPriorityState().targetLaborerCount - 1);
		RefreshUI();
		SendNetworkPriority();

		dataSource()->Spawn2DSound("UI", "UIIncrementalChange");
	}

	/*
	 * Builder
	 */
	//UFUNCTION() void OnClickBuilderNonPriorityButton() {
	//	townPriorityState().builderPriority = true;
	//	RefreshUI();
	//	SendNetworkPriority();
	//}
	//UFUNCTION() void OnClickBuilderPriorityButton() {
	//	townPriorityState().builderPriority = false;
	//	RefreshUI();
	//	SendNetworkPriority();
	//}
	UFUNCTION() void OnCheckManualBuilder(bool isChecked) {
		townPriorityState().builderPriority = isChecked;
		RefreshUI();
		SendNetworkPriority();
	}
	
	UFUNCTION() void IncreaseBuilders() {
		townPriorityState().targetBuilderCount++;
		RefreshUI();
		SendNetworkPriority();

		dataSource()->Spawn2DSound("UI", "UIIncrementalChange");
	}
	UFUNCTION() void DecreaseBuilders() {
		townPriorityState().targetBuilderCount = std::max(0, townPriorityState().targetBuilderCount - 1);
		RefreshUI();
		SendNetworkPriority();

		dataSource()->Spawn2DSound("UI", "UIIncrementalChange");
	}

	/*
	 * RoadMaker
	 */
	UFUNCTION() void OnClickRoadMakerNonPriorityButton() {
		townPriorityState().roadMakerPriority = true;
		RefreshUI();
		SendNetworkPriority();
	}
	UFUNCTION() void OnClickRoadMakerPriorityButton() {
		townPriorityState().roadMakerPriority = false;
		RefreshUI();
		SendNetworkPriority();
	}
	UFUNCTION() void IncreaseRoadMakers() {
		townPriorityState().targetRoadMakerCount++;
		RefreshUI();
		SendNetworkPriority();

		dataSource()->Spawn2DSound("UI", "UIIncrementalChange");
	}
	UFUNCTION() void DecreaseRoadMakers() {
		townPriorityState().targetRoadMakerCount = std::max(0, townPriorityState().targetRoadMakerCount - 1);
		RefreshUI();
		SendNetworkPriority();

		dataSource()->Spawn2DSound("UI", "UIIncrementalChange");
	}

	//UFUNCTION() void OnClickConquerButton()
	//{
	//	TownHall& townhall = simulation().building(_buildingId).subclass<TownHall>();

	//	int32 attackingPlayerId = playerId();
	//	int32 defendingPlayerId = townhall.playerId();
	//	
	//	PUN_CHECK(attackingPlayerId != defendingPlayerId);

	//	// TODO: menu to allow allocating attackers
	//	ArmyGroup& group = simulation().townhall(attackingPlayerId).armyNode.defendGroups[0];
	//	
	//	if (group.playerId != attackingPlayerId)
	//	{
	//		simulation().AddPopupToFront(playerId(), "Cannot attack when your capital is not controlled by you.", ExclusiveUIEnum::None, "PopupCannot");
	//		return;
	//	}

	//	std::vector<int32> armyCounts = group.GetArmyCounts();
	//	if (CppUtils::Sum(armyCounts) <= 0)
	//	{
	//		simulation().AddPopupToFront(playerId(), "Need an army to attack.", ExclusiveUIEnum::None, "PopupCannot");
	//		return;
	//	}

	//	auto command = std::make_shared<FAttack>();
	//	command->defendingPlayerId = defendingPlayerId;
	//	command->armyCounts = CppUtils::VecToArray(armyCounts);
	//	networkInterface()->SendNetworkCommand(command);
	//}

	FSetTownPriority& townPriorityState() { return _laborerPriorityState.townPriorityState; }
	LaborerPriorityState _laborerPriorityState;

	//float _lastPriorityInputTime = -999.0f;
	//
	//FSetTownPriority _townPriorityState;
	//int32 _laborerCount = -1;
	//int32 _builderCount = -1;
	//int32 _roadMakerCount = -1;
	
};
