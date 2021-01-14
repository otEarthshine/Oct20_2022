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

	void UpdateUI(bool isMini)
	{
		auto& sim = simulation();
		TownHall& townhall = GetTownhall();
		auto& townhallPlayerOwned = sim.playerOwned(townhall.playerId());

		/*
		 * Intercity Trade / Trade Route
		 */
		if (sim.HasTownhall(playerId())) // Need to have townhall, otherwise don't show anything
		{
			if (townhall.ownedBy(playerId()))
			{
				// Intercity Trade / Trade Route
				if (sim.unlockSystem(playerId())->unlockedSetTradeAmount) {
					SetText(TradeButtonText, "Set Trade Offer");
					BUTTON_ON_CLICK(TradeButton, this, &UTownhallHoverInfo::OnClickSetTradeOfferButton);
					TradeButton->SetVisibility(ESlateVisibility::Visible);
				}
				else {
					TradeButton->SetVisibility(ESlateVisibility::Collapsed);
				}

				
				// Gift
				GiftButton->SetVisibility(ESlateVisibility::Collapsed);

				// Diplomacy
				DiplomacyButton->SetVisibility(ESlateVisibility::Collapsed);

				// Vassalize
				// (Declare Independence)
				if (townhallPlayerOwned.lordPlayerId() != -1)
				{
					SetText(VassalizeButtonRichText, "Declare Independence");
					BUTTON_ON_CLICK(VassalizeButton, this, &UTownhallHoverInfo::OnClickDeclareIndependenceButton);
					
					VassalizeButton->SetVisibility(ESlateVisibility::Visible);
				}
				else {
					VassalizeButton->SetVisibility(ESlateVisibility::Collapsed);
				}
				LiberationButton->SetVisibility(ESlateVisibility::Collapsed);
				

				// Buffs
				{
					std::vector<CardEnum> buffs = townhallPlayerOwned.GetBuffs();
					
					int32 index = 0;
					for (size_t i = 0; i < buffs.size(); i++)
					{
						auto buffIcon = GetBoxChild<UBuffIcon>(BuffRow, index, UIEnum::BuffIcon, true);
						buffIcon->SetBuff(buffs[i]);
					}
					BoxAfterAdd(BuffRow, index);
					
					BuffRow->SetVisibility(ESlateVisibility::SelfHitTestInvisible);
				}
			}
			else 
			{
				// Intercity Trade / Trade Route
				if (sim.IsResearched(playerId(), TechEnum::IntercityRoad))
				{
					// Other ppl's town, show trade route button instead
					std::vector<int32> tradePartners = simulation().worldTradeSystem().GetTradePartners(playerId());
					if (CppUtils::Contains(tradePartners, townhall.playerId())) {
						SetText(TradeButtonText, "Cancel Trade Route");
						BUTTON_ON_CLICK(TradeButton, this, &UTownhallHoverInfo::OnClickCancelTradeRouteButton);
					}
					else {
						SetText(TradeButtonText, "Establish Trade Route");
						BUTTON_ON_CLICK(TradeButton, this, &UTownhallHoverInfo::OnClickEstablishTradeRouteButton);
					}
					TradeButton->SetVisibility(ESlateVisibility::Visible);
				}
				else
				{
					TradeButton->SetVisibility(ESlateVisibility::Collapsed);
				}

				
				// Gift
				GiftButton->SetVisibility(ESlateVisibility::Visible);
				BUTTON_ON_CLICK(GiftButton, this, &UTownhallHoverInfo::OnClickGiftButton);

				// Diplomacy
				DiplomacyButton->SetVisibility(ESlateVisibility::Visible);
				BUTTON_ON_CLICK(DiplomacyButton, this, &UTownhallHoverInfo::OnClickDiplomacyButton);

				// Not already a vassal?
				if (!sim.playerOwned(playerId()).IsVassal(townhall.buildingId()))
				{
					// Vassalize
					if (sim.CanVassalizeOtherPlayers(playerId()) &&
						!townhallPlayerOwned.GetDefendingClaimProgress(townhall.provinceId()).isValid())
					{
						SetText(VassalizeButtonRichText, "Conquer (Vassalize)\n<img id=\"Influence\"/>" + std::to_string(sim.GetProvinceVassalizeStartPrice(townhall.provinceId())));
						BUTTON_ON_CLICK(VassalizeButton, this, &UTownhallHoverInfo::OnClickVassalizeButton);
						VassalizeButton->SetVisibility(ESlateVisibility::Visible);

						// Can also liberate if there is an existing conquerer
						if (townhallPlayerOwned.lordPlayerId() != -1) {
							SetText(LiberationButtonRichText, "Liberation\n<img id=\"Influence\"/>" + std::to_string(BattleInfluencePrice));
							LiberationButton->SetVisibility(ESlateVisibility::Visible);
							BUTTON_ON_CLICK(LiberationButton, this, &UTownhallHoverInfo::OnClickLiberateButton);
						}
						else {
							LiberationButton->SetVisibility(ESlateVisibility::Collapsed);
						}
					}
					else {
						VassalizeButton->SetVisibility(ESlateVisibility::Collapsed);
						LiberationButton->SetVisibility(ESlateVisibility::Collapsed);
					}
				}
				
				// Buffs
				BuffRow->SetVisibility(ESlateVisibility::Collapsed);
			}
		}
		else {
			GiftButton->SetVisibility(ESlateVisibility::Collapsed);
			DiplomacyButton->SetVisibility(ESlateVisibility::Collapsed);
			
			TradeButton->SetVisibility(ESlateVisibility::Collapsed);
			
			VassalizeButton->SetVisibility(ESlateVisibility::Collapsed);
			LiberationButton->SetVisibility(ESlateVisibility::Collapsed);
			
			BuffRow->SetVisibility(ESlateVisibility::Collapsed);
		}
		
		
		// Update population
		int population = townhallPlayerOwned.population();
		TownHoverPopulationText->SetText(FText::FromString(FString::FromInt(population)));

		// Don't show Laborer info if it isMini or not owned by player
		if (isMini || townhall.playerId() != playerId()) {
			LaborerBuilderBox->SetVisibility(ESlateVisibility::Collapsed);
		}
		else 
		{
			//// Sync to simulation only 3 sec after input (prevent fight)
			//if (UGameplayStatics::GetTimeSeconds(this) > _lastPriorityInputTime + 3.0f) {
			//	SyncState();
			//}
			int32 playerId = simulation().building(_buildingId).playerId();
			_laborerPriorityState.TrySyncToSimulation(&simulation(), playerId, this);
			RefreshUI();
			LaborerBuilderBox->SetVisibility(ESlateVisibility::SelfHitTestInvisible);
		}


		// Make sure town's name is up to date with sim (only for non player's town)
		if (townhall.playerId() == playerId()) {
			CityNameText->SetColorAndOpacity(FLinearColor(0.7, 0.85, 1.0)); // Capital
		}
		else 
		{
			if (simulation().playerOwned(playerId()).IsVassal(townhall.buildingId())) {
				CityNameText->SetColorAndOpacity(FLinearColor(0.5, 0.6, 0.7)); // Vassal
			}
			else if (simulation().playerOwned(playerId()).IsAlly(townhall.playerId())) {
				CityNameText->SetColorAndOpacity(FLinearColor(.7, 1, .7));
			}
			else {
				CityNameText->SetColorAndOpacity(FLinearColor(1, 1, .7));
			}
		}

		// Townhall name
		FString displayedName = CityNameText->GetText().ToString();
		FString newDisplayName = townhall.townFName();

		if (displayedName != newDisplayName) {
			CityNameText->SetText(FText::FromString(newDisplayName));
		}
		

		// PlayerColorImage
		PlayerColorCircle->GetDynamicMaterial()->SetVectorParameterValue("PlayerColor1", PlayerColor1(townhallPlayerOwned.playerIdForColor()));
		PlayerColorCircle->GetDynamicMaterial()->SetVectorParameterValue("PlayerColor2", PlayerColor2(townhallPlayerOwned.playerIdForColor()));


		/*
		 * Update TradeInfoOverlay
		 */
		{
			const std::vector<IntercityTradeOffer>& offers = simulation().worldTradeSystem().GetIntercityTradeOffers(townhall.playerId());
			TradeInfoOverlay->SetVisibility(offers.size() > 0 ? ESlateVisibility::Visible : ESlateVisibility::Collapsed);
			CallbackEnum callbackEnum = (townhall.playerId() == playerId()) ? CallbackEnum::None : CallbackEnum::IntercityTrade;
			
			for (const auto& offer : offers) {
				int32 inventory = simulation().resourceCount(townhall.playerId(), offer.resourceEnum);
				if (offer.offerEnum == IntercityTradeOfferEnum::BuyWhenBelow) {
					if (offer.targetInventory > inventory) {
						BuyingBox->AddChooseResourceElement2(offer.resourceEnum, TEXT_NUM(offer.targetInventory - inventory), this, callbackEnum);
					}
				} else {
					if (inventory > offer.targetInventory) {
						SellingBox->AddChooseResourceElement2(offer.resourceEnum, TEXT_NUM(inventory - offer.targetInventory), this, callbackEnum);
					}
				}
			}

			BuyingBox->AfterAdd();
			SellingBox->AfterAdd();
		}


		//

		if (isMini) {
			SetRenderScale(FVector2D(0.5, 0.5));
		}


		// No more military
		ArmyFightBox->SetVisibility(ESlateVisibility::Collapsed);
		MilitaryButtons->SetVisibility(ESlateVisibility::Collapsed);
		LeftArmyBox->SetVisibility(ESlateVisibility::Collapsed);
		RightArmyBox->SetVisibility(ESlateVisibility::Collapsed);
		return;

		/*
		 * Battle
		 */
		//ArmyNode& armyNode = townhall.armyNode;

		//const DescriptionUIState& uiState = simulation().descriptionUIState();

		//bool isMilitaryBuilding = false;
		//if (uiState.objectType == ObjectTypeEnum::Building) {
		//	CardEnum buildingEnum = simulation().buildingEnum(uiState.objectId);
		//	isMilitaryBuilding = buildingEnum == CardEnum::Townhall || IsBarrack(buildingEnum);
		//}

		//// Don't show if player is placing building or looking at jobUI
		//if (inputSystemInterface()->placementState() != PlacementType::None ||
		//	(uiState.objectType == ObjectTypeEnum::Building && !isMilitaryBuilding)) // Is building, and not military, don't show army...
		//{
		//	// Only display if there is battle with FightIcon in world map...
		//	ArmyFightBox->SetVisibility(ESlateVisibility::Collapsed);
		//	MilitaryButtons->SetVisibility(ESlateVisibility::Collapsed);
		//	LeftArmyBox->SetVisibility(ESlateVisibility::Collapsed);
		//	RightArmyBox->SetVisibility(ESlateVisibility::Collapsed);
		//}
		//else
		//{
		//	ArmyFightBox->SetVisibility(ESlateVisibility::SelfHitTestInvisible);

		//	if (armyNode.isBattling() || armyNode.hasArrivingAttackers()) {
		//		FightIcon->SetVisibility(ESlateVisibility::SelfHitTestInvisible);
		//		//UImage* image = CastChecked<UImage>(FightIcon->GetChildAt(0));
		//		//image->SetColorAndOpacity(armyNode.hasArrivingAttackers() ? FLinearColor(1, 1, 1, 0.3) : FLinearColor::White);
		//	} else {
		//		FightIcon->SetVisibility(ESlateVisibility::Collapsed);
		//	}
		//	
		//	MilitaryButtons->SetVisibility(ESlateVisibility::SelfHitTestInvisible);

		//	auto displayArmyUI = [&](ArmyGroup& armyGroup, UArmyLinesUI* armyLines, UIEnum armyUnitUIEnum, int32 countDownSeconds)
		//	{
		//		UHorizontalBox* backLine = armyLines->BackLine;
		//		UHorizontalBox* frontLine = armyLines->FrontLine;

		//		int32 frontIndex = 0;
		//		int32 backIndex = 0;

		//		auto loopFunc = [&](int32 i)
		//		{
		//			int32 troopCount = armyGroup.TroopCount(i);
		//			if (troopCount > 0) {
		//				bool isFrontline = i < FrontLineArmyEnumCount;
		//				auto unitUI = GetBoxChild<UArmyUnitUI>(isFrontline ? frontLine : backLine, isFrontline ? frontIndex : backIndex, armyUnitUIEnum, true);
		//				ArmyEnum armyEnum = static_cast<ArmyEnum>(i);
		//				
		//				if (armyEnum == ArmyEnum::Tower) {
		//					SetText(unitUI->ArmyUnitCount, "lv." + to_string(GetArmyInfo(ArmyEnum::Tower).wallLvl(armyGroup.initialHPs[i])));
		//					unitUI->ArmyUnitCount->SetVisibility(ESlateVisibility::SelfHitTestInvisible);
		//				}
		//				else {
		//					SetText(unitUI->ArmyUnitCount, to_string(troopCount));
		//					unitUI->ArmyUnitCount->SetVisibility(troopCount > 1 ? ESlateVisibility::SelfHitTestInvisible : ESlateVisibility::Collapsed);
		//				}
		//				
		//				unitUI->ArmyUnitIcon->SetBrushFromTexture(assetLoader()->GetArmyIcon(static_cast<ArmyEnum>(i)));

		//				// HP
		//				if (armyGroup.HPs[i] != armyGroup.initialHPs[i]) {
		//					float hpFraction = static_cast<float>(armyGroup.HPs[i]) / armyGroup.initialHPs[i];
		//					unitUI->ArmyUnitHP->GetDynamicMaterial()->SetScalarParameterValue("Fraction", hpFraction);
		//					unitUI->ArmyUnitHP->SetVisibility(ESlateVisibility::SelfHitTestInvisible);
		//				}
		//				else {
		//					unitUI->ArmyUnitHP->SetVisibility(ESlateVisibility::Collapsed);
		//				}

		//				unitUI->ArmyUnitBackground->SetBrushTintColor(PlayerColor2(armyGroup.playerId));
		//				unitUI->ArmyUnitIcon->SetBrushTintColor(PlayerColor1(armyGroup.playerId));

		//				bool showFade = countDownSeconds >= 0 || countDownSeconds == -2;
		//				unitUI->ArmyUnitBackground->SetColorAndOpacity(showFade ? FLinearColor(1, 1, 1, 0.3) : FLinearColor::White);
		//				unitUI->ArmyUnitIcon->SetColorAndOpacity(showFade ? FLinearColor(1, 1, 1, 0.3) : FLinearColor::White);

		//				// Spawn damage and "GotHit" animation if needed
		//				// 
		//				if (armyGroup.lastDamageTaken[i] > 0 && 
		//					armyGroup.lastDamageTick[i] != -1 &&
		//					armyGroup.lastDamageTick[i] - Time::Ticks() < Time::TicksPerSecond * 3)
		//				{
		//					// Clear any existing damage that expired
		//					for (int32 j = unitUI->DamageOverlay->GetChildrenCount(); j-- > 0;) {
		//						auto curFloat = CastChecked<UDamageFloatupUI>(unitUI->DamageOverlay->GetChildAt(j));
		//						if (UGameplayStatics::GetTimeSeconds(this) - curFloat->startTime > 10.0f) {
		//							unitUI->DamageOverlay->RemoveChildAt(i);
		//						}
		//					}
		//					
		//					UDamageFloatupUI* damageFloatup = AddWidget<UDamageFloatupUI>(UIEnum::DamageFloatup);
		//					damageFloatup->startTime = UGameplayStatics::GetTimeSeconds(this);
		//					
		//					unitUI->DamageOverlay->AddChild(damageFloatup);
		//					SetText(damageFloatup->DamageText, to_string(armyGroup.lastDamageTaken[i]));
		//					armyGroup.lastDamageTaken[i] = -1;

		//					auto animation = GetAnimation(damageFloatup, FString("Floatup"));
		//					damageFloatup->PlayAnimation(animation);

		//					//PUN_LOG("DamageOverlay: %d", unitUI->DamageOverlay->GetChildrenCount());
		//				}

		//				// Do
		//				if (i != 0 && // not tower
		//					armyGroup.lastAttackedTick[i] != unitUI->lastAttackTick) 
		//				{
		//					unitUI->PlayAnimation(GetAnimation(unitUI, FString("Hit")));
		//					unitUI->lastAttackTick = armyGroup.lastAttackedTick[i];
		//				}

		//				// Tooltip
		//				AddArmyTooltip(unitUI->ArmyUnitBackground, i, armyGroup, armyNode);
		//			}
		//		};

		//		// Left Army Sort backward
		//		//if (armyUnitUIEnum == UIEnum::ArmyUnitRight) {
		//		//	for (int32 i = 0; i < ArmyEnumCount; i++) {
		//		//		loopFunc(i);
		//		//	}
		//		//}
		//		//else {
		//			for (int32 i = ArmyEnumCount; i-- > 0;) {
		//				loopFunc(i);
		//			}
		//		//}
		//		
		//		BoxAfterAdd(backLine, backIndex);
		//		BoxAfterAdd(frontLine, frontIndex);


		//		// Faded UI with countdown
		//		if (countDownSeconds == -2) {
		//			armyLines->ArrivalText->SetVisibility(ESlateVisibility::SelfHitTestInvisible);
		//			SetText(armyLines->ArrivalText, "Hidden");
		//		} else if (countDownSeconds == -1) {
		//			armyLines->ArrivalText->SetVisibility(ESlateVisibility::Collapsed);
		//		} else {
		//			armyLines->ArrivalText->SetVisibility(ESlateVisibility::SelfHitTestInvisible);
		//			SetText(armyLines->ArrivalText, std::to_string(countDownSeconds) + "s");
		//		}
		//	};

		//	auto setArmyLines = [&](std::vector<ArmyGroup>& groups, UIEnum armyUnitUIEnum, bool showRebel)
		//	{
		//		bool isRight = (armyUnitUIEnum == UIEnum::ArmyUnitRight);
		//		UVerticalBox* armyBox = isRight ? RightArmyBox : LeftArmyBox;
		//		UIEnum armyLineUIEnum = isRight ? UIEnum::ArmyLinesUIRight : UIEnum::ArmyLinesUILeft;
		//		int32 armyIndex = 0;

		//		// Arriving Attacker or Liberator
		//		if (armyLineUIEnum == UIEnum::ArmyLinesUILeft)
		//		{
		//			for (int32 i = 0; i < armyNode.arrivingGroups.size(); i++)
		//			{
		//				bool isConquerer = (armyNode.arrivingGroups[i].helpPlayerId == -1);
		//				bool isLiberator = (armyNode.lordPlayerId != armyNode.originalPlayerId) && armyNode.arrivingGroups[i].helpPlayerId == armyNode.originalPlayerId;
		//				
		//				if (isConquerer || isLiberator) {
		//					UArmyLinesUI* armyLines = GetBoxChild<UArmyLinesUI>(armyBox, armyIndex, armyLineUIEnum, true);
		//					int32 marchCountdown = armyNode.arrivingGroups[i].SecsUntilMarchComplete();
		//					displayArmyUI(armyNode.arrivingGroups[i], armyLines, armyUnitUIEnum, marchCountdown);
		//				}
		//			}
		//		}
		//		
		//		for (int32 i = 0; i < groups.size(); i++) {
		//			UArmyLinesUI* armyLines = GetBoxChild<UArmyLinesUI>(armyBox, armyIndex, armyLineUIEnum, true);
		//			displayArmyUI(groups[i], armyLines, armyUnitUIEnum, -1);
		//		}

		//		// Arriving helpers
		//		for (int32 i = 0; i < armyNode.arrivingGroups.size(); i++) 
		//		{
		//			// if helping existing group, add as faded UI in this group
		//			for (int32 j = 0; j < groups.size(); j++) {
		//				if (armyNode.arrivingGroups[i].helpPlayerId == groups[j].playerId)
		//				{
		//					UArmyLinesUI* armyLines = GetBoxChild<UArmyLinesUI>(armyBox, armyIndex, armyLineUIEnum, true);
		//					int32 marchCountdown = armyNode.arrivingGroups[i].SecsUntilMarchComplete();
		//					displayArmyUI(armyNode.arrivingGroups[i], armyLines, armyUnitUIEnum, marchCountdown);
		//					break;
		//				}
		//			}
		//		}

		//		if (showRebel) {
		//			if (armyNode.rebelGroups.size() > 0) {
		//				// Rebel is only shown on its owner
		//				if (armyNode.rebelGroups[0].playerId == playerId()) {
		//					UArmyLinesUI* armyLines = GetBoxChild<UArmyLinesUI>(armyBox, armyIndex, armyLineUIEnum, true);
		//					displayArmyUI(armyNode.rebelGroups[0], armyLines, armyUnitUIEnum, -2);
		//				}
		//			}
		//		}
		//		
		//		BoxAfterAdd(armyBox, armyIndex);
		//		armyBox->SetVisibility(armyIndex > 0 ? ESlateVisibility::SelfHitTestInvisible : ESlateVisibility::Collapsed);
		//	};

		//	// In battle, defender is on the right (defender should feels awkward...)
		//	if (armyNode.isBattling() || armyNode.hasArrivingAttackers()) {
		//		setArmyLines(armyNode.defendGroups, UIEnum::ArmyUnitRight, true);
		//		setArmyLines(armyNode.attackGroups, UIEnum::ArmyUnitLeft, false);
		//	}
		//	// Otherwise, defender default to the left
		//	else {
		//		setArmyLines(armyNode.defendGroups, UIEnum::ArmyUnitLeft, true);
		//		setArmyLines(armyNode.attackGroups,  UIEnum::ArmyUnitRight, false);
		//	}

		//	_lastArmyNode = armyNode;

		//	// First
		//	// ArmyBox
		//	auto leftSlot = CastChecked<UCanvasPanelSlot>(LeftArmyBox->Slot);
		//	bool rightLineVisible = RightArmyBox->GetChildrenCount() > 0 && RightArmyBox->GetChildAt(0)->GetVisibility() != ESlateVisibility::Collapsed;

		//	if (rightLineVisible) {
		//		leftSlot->SetAlignment(FVector2D(1, 0.5));
		//		leftSlot->SetPosition(FVector2D(-20, 0));
		//	} else {
		//		leftSlot->SetAlignment(FVector2D(0.5, 0.5));
		//		leftSlot->SetPosition(FVector2D(0, 0));
		//	}
		//}

		///*
		// * Army Order Buttons
		// */
		//if (isMini)
		//{
		//	// No display for mini...
		//	int32 armyButtonIndex = 0;
		//	BoxAfterAdd(MilitaryButtons, armyButtonIndex);
		//}
		//else
		//{
		//	int32 armyButtonIndex = 0;

		//	auto getButton = [&](std::string buttonStr) {
		//		auto button = GetBoxChild<UPunSimpleButton>(MilitaryButtons, armyButtonIndex, UIEnum::ArmyDeployButton, true);
		//		SetText(button->Text, buttonStr);
		//		return button;
		//	};

		//	bool isAlliedWithLord = simulation().playerOwned(playerId()).IsAlly(armyNode.lordPlayerId);
		//	
		//	// Already have army deployed (existing/arriving), can reinforce or recall them
		//	// Lord would already have army deployed 
		//	if (armyNode.IsPlayerInvolved(playerId())) 
		//	{
		//		// Recall - Army is arriving, cancel it
		//		// Reinforce - add army
		//		// Move - 
		//		
		//		// Recall/Reinforce, if player isn't the original owner
		//		if (armyNode.lordPlayerId == playerId()) 
		//		{
		//			// Get armyNodes without the current node
		//			std::vector<int32> armyNodeIds = simulation().GetArmyNodeIds(playerId());
		//			CppUtils::TryRemove(armyNodeIds, armyNode.nodeId);
		//			
		//			// Has a place we can move the army to, and has army to move, show the button
		//			ArmyGroup* moveGroup = armyNode.GetArmyGroup(playerId());
		//			if (armyNodeIds.size() > 0 && moveGroup && moveGroup->HasMovableArmy()) {
		//				getButton("Move Army")->Init(CallbackEnum::ArmyMoveBetweenNode, this);
		//			}

		//			// Owner retreat into hiding instead of recall
		//			if (armyNode.originalPlayerId == playerId() && 
		//				armyNode.isBattling()) 
		//			{
		//				getButton("Retreat")->Init(CallbackEnum::ArmyRetreat, this);
		//			}
		//		}
		//		else {
		//			// Rebel retreat instead of recall
		//			if (armyNode.originalPlayerId == playerId()) {
		//				getButton("Retreat")->Init(CallbackEnum::ArmyRetreat, this);
		//			} else {
		//				getButton("Recall Army")->Init(CallbackEnum::ArmyRecall, this);
		//			}

		//			// If there is army elsewhere show reinforce
		//			std::vector<int32> nodeIds = simulation().GetArmyNodeIds(playerId());
		//			bool hasReinforcableArmy = false;
		//			for (int32 nodeId : nodeIds) {
		//				if (armyNode.nodeId != nodeId) {
		//					ArmyGroup* armyGroup = simulation().GetArmyNode(nodeId).GetArmyGroup(playerId());
		//					if (armyGroup && armyGroup->HasMovableArmy()) {
		//						hasReinforcableArmy = true;
		//						break;
		//					}
		//				}
		//			}

		//			if (hasReinforcableArmy) {
		//				getButton("Reinforce Army")->Init(CallbackEnum::ArmyReinforce, this);
		//			}

		//			// TODO: Change Army Stance...
		//		}
		//	}
		//	// If we are not original owner, and our capital is still ours...
		//	else if (armyNode.originalPlayerId != playerId())
		//	{
		//		if (isAlliedWithLord) {
		//			// Ally's capital
		//			if (armyNode.originalPlayerId == armyNode.lordPlayerId) {
		//				getButton("Betray Ally")->Init(CallbackEnum::AllyBetray, this);
		//			}
		//		}
		//		else {
		//			getButton("Conquer")->Init(CallbackEnum::ArmyConquer, this);
		//			getButton("Propose Alliance")->Init(CallbackEnum::AllyRequest, this);
		//		}

		//		// Reinforce any involved ally
		//		std::unordered_set<int32> involvedPlayerIds;
		//		armyNode.ExecuteOnAllGroups([&](const ArmyGroup& group) {
		//			involvedPlayerIds.insert(group.playerId);
		//		});

		//		for (int32 involvedPlayerId : involvedPlayerIds) {
		//			if (simulation().playerOwned(playerId()).IsAlly(involvedPlayerId)) {
		//				auto button = getButton("Reinforce " + simulation().playerName(involvedPlayerId));
		//				button->Init(CallbackEnum::ArmyHelp, this);
		//				button->callbackVar1 = involvedPlayerId;
		//			}
		//		}
		//	}

		//	// Govern by someone else(not you), can liberate, conquer or help defend...
		//	if (armyNode.lordPlayerId != playerId() &&
		//		armyNode.lordPlayerId != armyNode.originalPlayerId) 
		//	{
		//		if (armyNode.originalPlayerId == playerId()) {
		//			getButton("Rebel")->Init(CallbackEnum::ArmyRebel, this);
		//		} else {
		//			if (!isAlliedWithLord) {
		//				getButton("Liberate")->Init(CallbackEnum::ArmyLiberate, this);
		//			}
		//		}
		//	}

		//	BoxAfterAdd(MilitaryButtons, armyButtonIndex);
		//}
	}

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
		
		
		auto command = std::make_shared<FAttack>();
		command->armyOrderEnum = callbackEnum;
		
		// Recall or move
		// - chosen node is the origin node
		// - OpenArmyMoveUI to choose target node to recall/move to
		if (callbackEnum == CallbackEnum::ArmyRecall ||
			callbackEnum == CallbackEnum::ArmyMoveBetweenNode)
		{
			command->originNodeId = _buildingId;
			GetPunHUD()->OpenArmyMoveUI(command);
			return;
		}

		// Rebel uses army at the current node
		if (callbackEnum == CallbackEnum::ArmyRebel) {
			// Get rebel army in target node
			ArmyGroup* rebelArmy = sim.GetArmyNode(buildingId()).GetRebelGroup(playerId());
			
			// Make sure there is army to rebel...
			if (!rebelArmy) {
				//simulation().AddPopupToFront(playerId(), "Need an army to rebel.", ExclusiveUIEnum::ArmyMoveUI, "PopupCannot");
				return;
			}

			command->targetNodeId = _buildingId;
			command->originNodeId = _buildingId;
			command->armyCounts = CppUtils::VecToArray(rebelArmy->GetArmyCounts());
			networkInterface()->SendNetworkCommand(command);
			return;
		}

		// Capital not controlled by player, can't do any other actions
		if (sim.townhall(playerId()).armyNode.originalPlayerId != playerId()) {
			//simulation().AddPopupToFront(playerId(), "Need to regain control of the capital before dispatching armies outside.", ExclusiveUIEnum::ArmyMoveUI, "PopupCannot");
			return;
		}

		// Options that must take army from other cities
		if (callbackEnum == CallbackEnum::ArmyConquer ||
			callbackEnum == CallbackEnum::ArmyHelp ||
			callbackEnum == CallbackEnum::ArmyReinforce ||
			callbackEnum == CallbackEnum::ArmyLiberate)
		{
			if (callbackEnum == CallbackEnum::ArmyHelp) {
				command->helpPlayerId = punWidgetCaller->callbackVar1;
			}
			
			command->targetNodeId = _buildingId;
			GetPunHUD()->OpenArmyMoveUI(command);
			return;
		}

		if (callbackEnum == CallbackEnum::AllyRequest ||
			callbackEnum == CallbackEnum::AllyBetray ||
			callbackEnum == CallbackEnum::ArmyRetreat)
		{
			if (callbackEnum == CallbackEnum::AllyRequest) {
				if (UGameplayStatics::GetTimeSeconds(this) - _lastAllyRequestTick < 5.0f) {
					//simulation().AddPopupToFront(playerId(), "Please wait a bit for another player to reply.", ExclusiveUIEnum::ArmyMoveUI, "PopupCannot");
					return;
				}
				_lastAllyRequestTick = UGameplayStatics::GetTimeSeconds(this);
			}
			
			command->originNodeId = simulation().townhall(playerId()).buildingId(); // Player's capital
			command->targetNodeId = _buildingId; // Target's capital...
			networkInterface()->SendNetworkCommand(command);
			return;
		}

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
		int32 playerId = simulation().building(_buildingId).subclass<TownHall>(CardEnum::Townhall).playerId();
		
		_laborerPriorityState.RefreshUI(
			this,
			&simulation(),
			playerId,

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

		
		//// Employed
		//auto& sim = simulation();
		//int32 playerId = sim.building(_buildingId).subclass<TownHall>(CardEnum::Townhall).playerId();
		//{
		//	auto& playerOwned = sim.playerOwned(playerId);

		//	int32 totalJobSlots = 0;
		//	const std::vector<std::vector<int32>>& jobBuildingEnumToIds = playerOwned.jobBuildingEnumToIds();
		//	for (const std::vector<int32>& buildingIds : jobBuildingEnumToIds) {
		//		for (int32 buildingId : buildingIds) {
		//			totalJobSlots += sim.building(buildingId).allowedOccupants();
		//		}
		//	}
		//	
		//	std::stringstream ss;
		//	ss << playerOwned.employedCount_WithoutBuilder() << "/" << totalJobSlots;
		//	SetText(Employed, ss.str());

		//	AddToolTip(EmployedBox, "People assigned to buildings\n/ Total buildings' job slots");
		//}
		
		
		//// Laborer
		//FString laborerString = FString::FromInt(_laborerCount);
		//if (_townPriorityState.laborerPriority) {
		//	laborerString += FString("/") + FString::FromInt(_townPriorityState.targetLaborerCount);
		//}
		//SetPriorityButtons(LaborerPriorityButton, LaborerNonPriorityButton, LaborerArrowOverlay, _townPriorityState.laborerPriority);

		//if (_laborerCount == 0) {
		//	LaborerRed->SetText(FText::FromString(laborerString));
		//	LaborerRed->SetVisibility(ESlateVisibility::HitTestInvisible);
		//	Laborer->SetVisibility(ESlateVisibility::Collapsed);

		//	AddToolTip(LaborerRed, "People not assigned to buildings become laborers. Laborers haul goods.");
		//}
		//else {
		//	Laborer->SetText(FText::FromString(laborerString));
		//	Laborer->SetVisibility(ESlateVisibility::HitTestInvisible);
		//	LaborerRed->SetVisibility(ESlateVisibility::Collapsed);

		//	AddToolTip(LaborerBox, "People not assigned to buildings become laborers. Laborers haul goods.");
		//}

		//
		//// Builder
		//FString builderString = FString::FromInt(_builderCount);
		//if (_townPriorityState.builderPriority) {
		//	builderString += FString("/") + FString::FromInt(_townPriorityState.targetBuilderCount);
		//}
		//SetPriorityButtons(BuilderPriorityButton, BuilderNonPriorityButton, BuilderArrowOverlay, _townPriorityState.builderPriority);
		//Builder->SetText(FText::FromString(builderString));

		//AddToolTip(BuilderBox, "People assigned to buildings under construction.");


		
		// RoadMaker
		// TODO: Remove?
		FString roadMakerString = FString::FromInt(_laborerPriorityState.roadMakerCount);
		if (_laborerPriorityState.townPriorityState.roadMakerPriority) {
			roadMakerString += FString("/") + FString::FromInt(_laborerPriorityState.townPriorityState.targetRoadMakerCount);
		}
		_laborerPriorityState.SetPriorityButtons(RoadMakerPriorityButton, RoadMakerNonPriorityButton, RoadMakerArrowOverlay, _laborerPriorityState.townPriorityState.roadMakerPriority);
		RoadMaker->SetText(FText::FromString(roadMakerString));

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

	UPROPERTY(meta = (BindWidget)) UButton* GiftButton;
	UPROPERTY(meta = (BindWidget)) UButton* DiplomacyButton;
	
	UPROPERTY(meta = (BindWidget)) UButton* VassalizeButton;
	UPROPERTY(meta = (BindWidget)) URichTextBlock* VassalizeButtonRichText;
	UPROPERTY(meta = (BindWidget)) UButton* LiberationButton;
	UPROPERTY(meta = (BindWidget)) URichTextBlock* LiberationButtonRichText;

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
