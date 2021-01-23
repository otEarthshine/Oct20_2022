// Fill out your copyright notice in the Description page of Project Settings.

#include "TownhallHoverInfo.h"
#include "Components/EditableText.h"

using namespace std;

#define LOCTEXT_NAMESPACE "TownhallHoverInfo"

void UTownhallHoverInfo::PunInit(int buildingId)
{
	_buildingId = buildingId;
	//CityNameEditableText->OnTextCommitted.AddDynamic(this, &UTownhallHoverInfo::ChangedCityName);

	BUTTON_ON_CLICK(LaborerNonPriorityButton, this, &UTownhallHoverInfo::OnClickLaborerNonPriorityButton);
	BUTTON_ON_CLICK(LaborerPriorityButton, this, &UTownhallHoverInfo::OnClickLaborerPriorityButton);
	BUTTON_ON_CLICK(LaborerArrowUp, this, &UTownhallHoverInfo::IncreaseLaborers);
	BUTTON_ON_CLICK(LaborerArrowDown, this, &UTownhallHoverInfo::DecreaseLaborers);

	BUTTON_ON_CLICK(BuilderNonPriorityButton, this, &UTownhallHoverInfo::OnClickBuilderNonPriorityButton);
	BUTTON_ON_CLICK(BuilderPriorityButton, this, &UTownhallHoverInfo::OnClickBuilderPriorityButton);
	BUTTON_ON_CLICK(BuilderArrowUp, this, &UTownhallHoverInfo::IncreaseBuilders);
	BUTTON_ON_CLICK(BuilderArrowDown, this, &UTownhallHoverInfo::DecreaseBuilders);

	BUTTON_ON_CLICK(RoadMakerNonPriorityButton, this, &UTownhallHoverInfo::OnClickRoadMakerNonPriorityButton);
	BUTTON_ON_CLICK(RoadMakerPriorityButton, this, &UTownhallHoverInfo::OnClickRoadMakerPriorityButton);
	BUTTON_ON_CLICK(RoadMakerArrowUp, this, &UTownhallHoverInfo::IncreaseRoadMakers);
	BUTTON_ON_CLICK(RoadMakerArrowDown, this, &UTownhallHoverInfo::DecreaseRoadMakers);


	LeftArmyBox->ClearChildren();
	RightArmyBox->ClearChildren();
	MilitaryButtons->ClearChildren();

	SetChildHUD(BuyingBox);
	SetChildHUD(SellingBox);

	_laborerPriorityState.lastPriorityInputTime = -999.0f;
	//_laborerPriorityState.PunInit(&simulation(), simulation().building(_buildingId).playerId());c
	//_lastPriorityInputTime = -999.0f;
	//SyncState();

	BuffRow->ClearChildren();
}

void UTownhallHoverInfo::ChangedCityName(const FText& Text, ETextCommit::Type CommitMethod)
{
	if (InterfacesInvalid()) return;

	auto command = make_shared<FChangeName>();
	command->name = Text.ToString();
	command->objectId = _buildingId;
	//PUN_LOG("ChangedCityName: %s", *command->name);
	networkInterface()->SendNetworkCommand(command);
}


void UTownhallHoverInfo::UpdateUI(bool isMini)
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
				SetText(TradeButtonText, LOCTEXT("Set Trade Offer", "Set Trade Offer"));
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
				SetText(VassalizeButtonRichText, LOCTEXT("Declare Independence", "Declare Independence"));
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
					SetText(TradeButtonText, LOCTEXT("Cancel Trade Route", "Cancel Trade Route"));
					BUTTON_ON_CLICK(TradeButton, this, &UTownhallHoverInfo::OnClickCancelTradeRouteButton);
				}
				else {
					SetText(TradeButtonText, LOCTEXT("Establish Trade Route", "Establish Trade Route"));
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
					SetText(VassalizeButtonRichText, FText::Format(
						LOCTEXT("VassalizeButtonRichText_Text", "Conquer (Vassalize)\n<img id=\"Influence\"/>{0}"),
						TEXT_NUM(sim.GetProvinceVassalizeStartPrice(townhall.provinceId()))
					));
					BUTTON_ON_CLICK(VassalizeButton, this, &UTownhallHoverInfo::OnClickVassalizeButton);
					VassalizeButton->SetVisibility(ESlateVisibility::Visible);

					// Can also liberate if there is an existing conquerer
					if (townhallPlayerOwned.lordPlayerId() != -1) {
						SetText(LiberationButtonRichText, FText::Format(
							LOCTEXT("LiberationButtonRichText_Text", "Liberation\n<img id=\"Influence\"/>{0}"),
							TEXT_NUM(BattleInfluencePrice)
						));
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
			}
			else {
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


#undef LOCTEXT_NAMESPACE