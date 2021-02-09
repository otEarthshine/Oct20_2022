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

#include "TownhallHoverInfo.generated.h"


/**
 * 
 */
UCLASS()
class UTownhallHoverInfo : public UPunWidget
{
	GENERATED_BODY()
public:
	void PunInit(int buildingId);

	int buildingId() { return _buildingId; }
	TownHall& GetTownhall() {
		return simulation().building(_buildingId).subclass<TownHall>(CardEnum::Townhall);
	}

	void UpdateUI(bool isMini);

	ArmyNode _lastArmyNode; // Used to compare to the current armyNode and show damage...
	TArray<UWidget*> _damageFloatups;

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
			
			GetPunHUD()->OpenTargetConfirmUI_IntercityTrade(_buildingId, resourceEnum);
			return;
		}
		
		
		//auto command = std::make_shared<FAttack>();
		//command->armyOrderEnum = callbackEnum;
		//
		//// Recall or move
		//// - chosen node is the origin node
		//// - OpenArmyMoveUI to choose target node to recall/move to
		//if (callbackEnum == CallbackEnum::ArmyRecall ||
		//	callbackEnum == CallbackEnum::ArmyMoveBetweenNode)
		//{
		//	command->originNodeId = _buildingId;
		//	GetPunHUD()->OpenArmyMoveUI(command);
		//	return;
		//}

		//// Rebel uses army at the current node
		//if (callbackEnum == CallbackEnum::ArmyRebel) {
		//	// Get rebel army in target node
		//	ArmyGroup* rebelArmy = sim.GetArmyNode(buildingId()).GetRebelGroup(playerId());
		//	
		//	// Make sure there is army to rebel...
		//	if (!rebelArmy) {
		//		//simulation().AddPopupToFront(playerId(), "Need an army to rebel.", ExclusiveUIEnum::ArmyMoveUI, "PopupCannot");
		//		return;
		//	}

		//	command->targetNodeId = _buildingId;
		//	command->originNodeId = _buildingId;
		//	command->armyCounts = CppUtils::VecToArray(rebelArmy->GetArmyCounts());
		//	networkInterface()->SendNetworkCommand(command);
		//	return;
		//}

		//// Capital not controlled by player, can't do any other actions
		//if (sim.townhall(playerId()).armyNode.originalPlayerId != playerId()) {
		//	//simulation().AddPopupToFront(playerId(), "Need to regain control of the capital before dispatching armies outside.", ExclusiveUIEnum::ArmyMoveUI, "PopupCannot");
		//	return;
		//}

		//// Options that must take army from other cities
		//if (callbackEnum == CallbackEnum::ArmyConquer ||
		//	callbackEnum == CallbackEnum::ArmyHelp ||
		//	callbackEnum == CallbackEnum::ArmyReinforce ||
		//	callbackEnum == CallbackEnum::ArmyLiberate)
		//{
		//	if (callbackEnum == CallbackEnum::ArmyHelp) {
		//		command->helpPlayerId = punWidgetCaller->callbackVar1;
		//	}
		//	
		//	command->targetNodeId = _buildingId;
		//	GetPunHUD()->OpenArmyMoveUI(command);
		//	return;
		//}

		//if (callbackEnum == CallbackEnum::AllyRequest ||
		//	callbackEnum == CallbackEnum::AllyBetray ||
		//	callbackEnum == CallbackEnum::ArmyRetreat)
		//{
		//	if (callbackEnum == CallbackEnum::AllyRequest) {
		//		if (UGameplayStatics::GetTimeSeconds(this) - _lastAllyRequestTick < 5.0f) {
		//			//simulation().AddPopupToFront(playerId(), "Please wait a bit for another player to reply.", ExclusiveUIEnum::ArmyMoveUI, "PopupCannot");
		//			return;
		//		}
		//		_lastAllyRequestTick = UGameplayStatics::GetTimeSeconds(this);
		//	}
		//	
		//	command->originNodeId = simulation().townhall(playerId()).buildingId(); // Player's capital
		//	command->targetNodeId = _buildingId; // Target's capital...
		//	networkInterface()->SendNetworkCommand(command);
		//	return;
		//}

		UE_DEBUG_BREAK();
	}
	

	//void SyncState()
	//{
	//	int32 playerId = simulation().building(_buildingId).subclass<TownHall>(CardEnum::Townhall).playerId();
	//	auto& playerOwned = simulation().playerOwned(playerId);
	//	_townPriorityState = *(playerOwned.CreateTownPriorityCommand());
	//	_laborerCount = std::max(playerOwned.laborerCount(), 0); // Requires clamp since laborerCount() may be negative when someone died
	//	_builderCount = std::max(playerOwned.builderCount(), 0);
	//	_roadMakerCount = std::max(playerOwned.roadMakerCount(), 0);
	//}
	
	void RefreshUI()
	{
		int32 townId = simulation().building(_buildingId).subclass<TownHall>(CardEnum::Townhall).townId();
		
		_laborerPriorityState.RefreshUILaborerPriority(
			this,
			&simulation(),
			townId,

			EmployedBox,
			Employed,

			LaborerBox,
			LaborerPriorityButton, LaborerNonPriorityButton, LaborerArrowOverlay,
			Laborer,
			LaborerRed,

			BuilderBox,
			BuilderNonPriorityButton,
			BuilderPriorityButton,
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
	UPROPERTY(meta = (BindWidget)) UCanvasPanel* ArmyFightBox;
	UPROPERTY(meta = (BindWidget)) UVerticalBox* LeftArmyBox;
	UPROPERTY(meta = (BindWidget)) UVerticalBox* RightArmyBox;
	UPROPERTY(meta = (BindWidget)) USizeBox* FightIcon;
	
	UPROPERTY(meta = (BindWidget)) UVerticalBox* MilitaryButtons;

	UPROPERTY(meta = (BindWidget)) UButton* TradeButton;
	UPROPERTY(meta = (BindWidget)) UTextBlock* TradeButtonText;
	UPROPERTY(meta = (BindWidget)) UOverlay* TradeInfoOverlay;
	UPROPERTY(meta = (BindWidget)) UPunBoxWidget* BuyingBox;
	UPROPERTY(meta = (BindWidget)) UPunBoxWidget* SellingBox;

	UPROPERTY(meta = (BindWidget)) UButton* SendImmigrantsButton;
	UPROPERTY(meta = (BindWidget)) UButton* GiftButton;
	UPROPERTY(meta = (BindWidget)) UButton* DiplomacyButton;
	
	UPROPERTY(meta = (BindWidget)) UButton* AttackButton1;
	UPROPERTY(meta = (BindWidget)) URichTextBlock* AttackButton1RichText;
	UPROPERTY(meta = (BindWidget)) UButton* AttackButton2;
	UPROPERTY(meta = (BindWidget)) URichTextBlock* AttackButton2RichText;

	UPROPERTY(meta = (BindWidget)) UHorizontalBox* BuffRow;
	
	UPROPERTY(meta = (BindWidget)) UTextBlock* TownHoverPopulationText;
	UPROPERTY(meta = (BindWidget)) UTextBlock* CityNameText;
	UPROPERTY(meta = (BindWidget)) UImage* PlayerColorCircle;

	UPROPERTY(meta = (BindWidget)) UVerticalBox* LaborerBuilderBox;

	UPROPERTY(meta = (BindWidget)) UHorizontalBox* EmployedBox;
	UPROPERTY(meta = (BindWidget)) UTextBlock* Employed;

	UPROPERTY(meta = (BindWidget)) UHorizontalBox* LaborerBox;
	UPROPERTY(meta = (BindWidget)) UButton* LaborerNonPriorityButton;
	UPROPERTY(meta = (BindWidget)) UButton* LaborerPriorityButton;
	UPROPERTY(meta = (BindWidget)) UTextBlock* Laborer;
	UPROPERTY(meta = (BindWidget)) UTextBlock* LaborerRed;
	UPROPERTY(meta = (BindWidget)) UButton* LaborerArrowUp;
	UPROPERTY(meta = (BindWidget)) UButton* LaborerArrowDown;
	UPROPERTY(meta = (BindWidget)) USizeBox* LaborerArrowOverlay;

	UPROPERTY(meta = (BindWidget)) UHorizontalBox* BuilderBox;
	UPROPERTY(meta = (BindWidget)) UButton* BuilderNonPriorityButton;
	UPROPERTY(meta = (BindWidget)) UButton* BuilderPriorityButton;
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
		networkInterface()->SendNetworkCommand(command);

		_laborerPriorityState.lastPriorityInputTime = UGameplayStatics::GetTimeSeconds(this);
	}

	//static void SetPriorityButtons(UButton* PriorityButton, UButton* NonPriorityButton, USizeBox* ArrowOverlay, bool priority)
	//{
	//	PriorityButton->SetVisibility(priority ? ESlateVisibility::Visible : ESlateVisibility::Collapsed);
	//	NonPriorityButton->SetVisibility(priority ? ESlateVisibility::Collapsed : ESlateVisibility::Visible);
	//	ArrowOverlay->SetVisibility(priority ? ESlateVisibility::Visible : ESlateVisibility::Collapsed);
	//}


	UFUNCTION() void OnClickSetTradeOfferButton() {
		GetPunHUD()->OpenIntercityTradeUI(_buildingId);
	}
	UFUNCTION() void OnClickEstablishTradeRouteButton()
	{
		auto command = make_shared<FSetIntercityTrade>();
		command->buildingIdToEstablishTradeRoute = _buildingId;
		networkInterface()->SendNetworkCommand(command);
	}
	UFUNCTION() void OnClickCancelTradeRouteButton()
	{
		auto command = make_shared<FSetIntercityTrade>();
		command->buildingIdToEstablishTradeRoute = _buildingId;
		command->isCancelingTradeRoute = 1;
		networkInterface()->SendNetworkCommand(command);
	}

	UFUNCTION() void OnClickSendImmigrantsButton() {
		int32 townId = simulation().building(_buildingId).townId();
		GetPunHUD()->OpenSendImmigrantsUI(townId);
	}
	UFUNCTION() void OnClickGiftButton() {
		int32 targetPlayerId = simulation().building(_buildingId).playerId();
		GetPunHUD()->OpenGiftUI(targetPlayerId);
	}
	UFUNCTION() void OnClickDiplomacyButton() {
		int32 targetPlayerId = simulation().building(_buildingId).playerId();
		GetPunHUD()->OpenDiplomacyUI(targetPlayerId);
	}

	void AttackDefenseHelper(CallbackEnum claimEnum)
	{
		auto command = make_shared<FClaimLand>();
		command->claimEnum = claimEnum;
		command->provinceId = GetTownhall().provinceId();
		PUN_CHECK(command->provinceId != -1);

		networkInterface()->SendNetworkCommand(command);
	}
	// Note: vassalize/independence/conquerColony are all CallbackEnum::StartAttackProvince for now
	// later on, there will be
	UFUNCTION() void OnClickVassalizeButton()
	{
		check(playerId() != GetTownhall().playerId());
		AttackDefenseHelper(CallbackEnum::StartAttackProvince);
	}
	
	UFUNCTION() void OnClickVassalizeReinforceButton()
	{
		check(playerId() != GetTownhall().playerId());
		AttackDefenseHelper(CallbackEnum::ReinforceAttackProvince);
	}
	
	UFUNCTION() void OnClickDeclareIndependenceButton()
	{
		check(playerId() == GetTownhall().playerId());
		AttackDefenseHelper(CallbackEnum::StartAttackProvince);
	}

	UFUNCTION() void OnClickConquerColonyButton()
	{
		check(playerId() == GetTownhall().playerId());
		AttackDefenseHelper(CallbackEnum::StartAttackProvince);
	}

	UFUNCTION() void OnClickLiberateButton()
	{
		check(playerId() != GetTownhall().playerId());
		AttackDefenseHelper(CallbackEnum::Liberate);
	}

	/*
	 * Laborer
	 */
	
	UFUNCTION() void OnClickLaborerNonPriorityButton() {
		townPriorityState().laborerPriority = true;
		RefreshUI();
		SendNetworkPriority();
	}
	UFUNCTION() void OnClickLaborerPriorityButton() {
		townPriorityState().laborerPriority = false;
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
	UFUNCTION() void OnClickBuilderNonPriorityButton() {
		townPriorityState().builderPriority = true;
		RefreshUI();
		SendNetworkPriority();
	}
	UFUNCTION() void OnClickBuilderPriorityButton() {
		townPriorityState().builderPriority = false;
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

private:
	int _buildingId;
};
