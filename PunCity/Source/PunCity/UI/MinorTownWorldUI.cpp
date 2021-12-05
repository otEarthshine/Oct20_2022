// Pun Dumnernchanvanit's


#include "MinorTownWorldUI.h"

#include "BattleFieldUnitIcon.h"

#define LOCTEXT_NAMESPACE "MinorTownWorldUI"



void UMinorTownWorldUI::UpdateUIBase(bool isMini)
{
	if (isMini) {
		SetRenderScale(FVector2D(0.5, 0.5));
	}
	
	/*
	 * City Name
	 */
	int32 uiPlayerId = townPlayerId();
	 // Make sure town's name is up to date with sim (only for non player's town)
	if (uiPlayerId == playerId()) {
		CityNameText->SetColorAndOpacity(FLinearColor(0.7, 0.85, 1.0)); // Our Capital
	}
	else
	{
		if (simulation().townManagerBase(playerId())->IsVassal(townId())) {
			CityNameText->SetColorAndOpacity(FLinearColor(0.5, 0.6, 0.7)); // Vassal
		}
		else if (simulation().townManagerBase(playerId())->IsAlly(uiPlayerId)) {
			CityNameText->SetColorAndOpacity(FLinearColor(.7, 1, .7));
		}
		else {
			CityNameText->SetColorAndOpacity(FLinearColor(1, 1, .7));
		}
	}


	// Townhall name
	FText displayedName = CityNameText->GetText();
	FText newDisplayName = simulation().townNameT(townId());

	if (!TextEquals(displayedName, newDisplayName)) {
		CityNameText->SetText(newDisplayName);
	}

	
	// PlayerColorImage
	// TODO: move to Minor Town
	int32 playerIdForLogo = simulation().townManagerBase(uiTownId)->playerIdForLogo();
	if (playerIdForLogo != -1)
	{
		PunUIUtils::SetPlayerLogo(PlayerColorCircle->GetDynamicMaterial(), dataSource()->playerInfo(playerIdForLogo), assetLoader());
		PlayerColorCircle->SetVisibility(ESlateVisibility::SelfHitTestInvisible);
	}
	else {
		PlayerColorCircle->SetVisibility(ESlateVisibility::Collapsed);
	}


	//! BottomCaptionText
	auto& sim = simulation();
	TownManagerBase* uiTownManagerBase = sim.townManagerBase(uiTownId);
	
	TArray<FText> args;
	auto tryAddNewLine = [&]() {
		if (args.Num() > 0) {
			ADDTEXT_INV_("\n");
		}
	};

	//! Steal Placement
	PlacementInfo placementInfo = inputSystemInterface()->PlacementBuildingInfo();
	if (placementInfo.buildingEnum == CardEnum::Steal &&
		uiPlayerId != -1)
	{
		int32 militaryPenalty;
		int32 actualSteal = sim.GetStealIncome(playerId(), uiPlayerId, militaryPenalty);
		
		FText militaryPenaltyText;
		if (militaryPenalty > 0) {
			militaryPenaltyText = FText::Format(
				LOCTEXT("StealMilitaryPenalty", "\n<Red>-{0}% effectiveness from military units</>"),
				TEXT_NUM(militaryPenalty)
			);
		}
		
		ADDTEXT_(
			LOCTEXT("MinorTownBottomCaption_Steal", "Steal Reward: +{0}<img id=\"Coin\"/>"),
			TEXT_NUM(actualSteal),
			militaryPenaltyText
		);
	}
	else
	{
		if (uiTownManagerBase->lordPlayerId() == playerId()) {
			ADDTEXT_(
				LOCTEXT("MinorTownBottomCaption_VassalTax", "Vassal Tax: +{0}<img id=\"Coin\"/>, +{1}<img id=\"Influence\"/>"),
				TEXT_100(uiTownManagerBase->totalRevenue100() * uiTownManagerBase->vassalTaxPercent() / 100),
				TEXT_100(uiTownManagerBase->totalInfluenceIncome100() * uiTownManagerBase->vassalInfluencePercent() / 100)
			);
		}

		//! Trade Route Income
		const std::vector<int32>& townIds = sim.GetTownIds(playerId());
		int32 tradeIncome = 0;
		for (int32 townId : townIds) {
			if (sim.worldTradeSystem().HasTradeRoute(townId, uiTownId)) {
				tradeIncome += sim.GetTradeRouteIncome();
			}
		}
		if (tradeIncome > 0) {
			tryAddNewLine();
			ADDTEXT_(
				LOCTEXT("MinorTownBottomCaption_tradeRoute", "Trade Route: +{0}<img id=\"Coin\"/>"),
				TEXT_NUM(tradeIncome)
			);
		}

		//! Good Relation Income
		if (IsMinorTown(uiTownId))
		{
			int32 relationshipInfluenceIncome = uiTownManagerBase->GetMinorCityAllyInfluenceReward(playerId());
			if (relationshipInfluenceIncome > 0) {
				tryAddNewLine();
				ADDTEXT_(
					LOCTEXT("MinorTownBottomCaption_relationship", "Good Relation: +{0}<img id=\"Influence\"/>"),
					TEXT_NUM(relationshipInfluenceIncome)
				);
			}
		}
	}

	if (args.Num() > 0) {
		BottomCaptionText->SetVisibility(ESlateVisibility::SelfHitTestInvisible);
		BottomCaptionText->SetText(JOINTEXT(args));
	}
	else {
		BottomCaptionText->SetVisibility(ESlateVisibility::Collapsed);
	}


	/*
	 * Battle
	 */
	ProvinceClaimProgress claimProgress = sim.townManagerBase(uiTownId)->GetDefendingClaimProgressDisplay(townProvinceId());
	if (claimProgress.isValid())
	{
		SetChildHUD(BattlefieldUI);
		BattlefieldUI->SetVisibility(ESlateVisibility::SelfHitTestInvisible);
		BattlefieldUI->UpdateBattleFieldUI(townProvinceId(), claimProgress, false);
	}
	else 
	{
		BattlefieldUI->SetVisibility(ESlateVisibility::Collapsed);
		//BattlefieldUI->provinceId = -1;
	}

	/*
	 * Trade Route
	 */
	if (!claimProgress.isValid() &&
		uiPlayerId != playerId() &&
		simulation().IsResearched(playerId(), TechEnum::TradeRoute) &&
		!simulation().worldTradeSystem().HasTradeRoute(playerId(), uiTownId))
	{
		ConnectTradeRouteButton->SetVisibility(ESlateVisibility::Visible);
		BUTTON_ON_CLICK(ConnectTradeRouteButton, this, &UMinorTownWorldUI::OnClickConnectTradeButton);
	}
	else {
		ConnectTradeRouteButton->SetVisibility(ESlateVisibility::Collapsed);
	}

	/*
	 * Show Foreign Army
	 */
	if (uiPlayerId != -1)
	//if (sim.townPlayerId(uiTownId) != playerId())
	{
		std::vector<CardStatus> militaryCards = sim.cardSystem(uiPlayerId).GetMilitaryCards();
		int32 index = 0;
		for (int32 i = 0; i < militaryCards.size(); i++)
		{
			auto unitIcon = GetBoxChild<UBattleFieldUnitIcon>(ArmyRow, index, UIEnum::WG_BattlefieldUnitIcon, true);
			unitIcon->UnitCountText->SetText(TEXT_NUM(militaryCards[i].stackSize));

			FSpineAsset spineAsset = assetLoader()->GetSpine(militaryCards[i].cardEnum);
			unitIcon->UnitImage->Atlas = spineAsset.atlas;
			unitIcon->UnitImage->SkeletonData = spineAsset.skeletonData;

			unitIcon->BackgroundImage->GetDynamicMaterial()->SetVectorParameterValue("ColorBackground", dataSource()->playerInfo(uiPlayerId).logoColorBackground);
			unitIcon->UnitImage->PunTick();

			MilitaryCardInfo militaryInfo = GetMilitaryInfo(militaryCards[i].cardEnum);

			int32 unitHP = militaryInfo.hp100 / 100;

			AddToolTip(unitIcon, FText::Format(
				LOCTEXT("UnitIcon_Tip", "{0}<space>HP: {1}\nDefense: {2}\nAttack: {3}<space>Unit Count: {4}"),
				GetBuildingInfo(militaryCards[i].cardEnum).name,
				TEXT_NUM(unitHP),
				TEXT_NUM(militaryInfo.defenseDisplay()),
				TEXT_NUM(militaryInfo.attackDisplay()),
				TEXT_NUM(militaryCards[i].stackSize)
			));
		}

		BoxAfterAdd(ArmyRow, index);

		ArmyRow->SetVisibility(ArmyRow->GetChildrenCount() > 0 ? ESlateVisibility::SelfHitTestInvisible : ESlateVisibility::Collapsed);
	}
	else {
		ArmyRow->SetVisibility(ESlateVisibility::Collapsed);
	}
}


void UMinorTownWorldUI::UpdateMinorTownUI(bool isMini)
{
	auto& sim = simulation();
	TownManagerBase* uiTownManagerBase = sim.townManagerBase(uiTownId);

	bool hasNoBattle = !uiTownManagerBase->GetDefendingClaimProgressDisplay(townProvinceId()).isValid();
	bool canRaze = sim.IsUnlocked(playerId(), UnlockStateEnum::Raze);
	bool canVassalize = sim.CanVassalizeOtherPlayers(playerId());
	bool canDiplo = sim.IsResearched(playerId(), TechEnum::ForeignRelation);

	if (!canRaze && !canVassalize && !canDiplo &&
		uiTownManagerBase->GetMinorCityLevel() == 1) 
	{
		SetVisibility(ESlateVisibility::Collapsed);
		return;
	}
	SetVisibility(ESlateVisibility::Visible);
	
	UpdateUIBase(isMini);



	GiftButton->SetVisibility(ESlateVisibility::Collapsed);
	DiplomacyButton->SetVisibility(ESlateVisibility::Collapsed);
	
	if (canDiplo && hasNoBattle)
	{
		// Gift
		GiftButton->SetVisibility(ESlateVisibility::Visible);
		BUTTON_ON_CLICK(GiftButton, this, &UMinorTownWorldUI::OnClickGiftButton);

		// Diplomacy
		DiplomacyButton->SetVisibility(ESlateVisibility::Visible);
		BUTTON_ON_CLICK(DiplomacyButton, this, &UMinorTownWorldUI::OnClickDiplomacyButton);
	}
	

	/*
	 * AttackButtons
	 */
	AttackButton1->SetVisibility(ESlateVisibility::Collapsed);
	AttackButton2->SetVisibility(ESlateVisibility::Collapsed);
	AttackButton3->SetVisibility(ESlateVisibility::Collapsed);

	// Not already a vassal? Might be able to attack
	if (!sim.townManagerBase(playerId())->IsVassal(townId()) &&
		hasNoBattle)
	{
		// Vassalize
		if (canVassalize)
		{
			//! Vassalize (AttackButton1)
			SetText(AttackButton1RichText, 
				LOCTEXT("VassalizeButtonRichText_Text", "Conquer (Vassalize)")
			);

			BUTTON_ON_CLICK(AttackButton1, this, &UMinorTownWorldUI::OnClickVassalizeButton);
			AttackButton1->SetVisibility(ESlateVisibility::Visible);

			// Can also liberate if there is an existing conquerer
			if (uiTownManagerBase->lordPlayerId() != -1) {
				SetText(AttackButton2RichText, 
					LOCTEXT("LiberationButtonRichText_Text", "Liberation")
				);
				AttackButton2->SetVisibility(ESlateVisibility::Visible);
				BUTTON_ON_CLICK(AttackButton2, this, &UMinorTownWorldUI::OnClickLiberateButton);
			}
		}

		//! Raze
		if (canRaze)
		{
			AttackButton3->SetVisibility(ESlateVisibility::Visible);
			BUTTON_ON_CLICK(AttackButton3, this, &UMinorTownWorldUI::OnClickRazeButton);
		}
	}


	// Update population
	TownHoverPopulationText->SetText(FText::Format(LOCTEXT("Lv {0}", "Lv {0}"), TEXT_NUM(uiTownManagerBase->GetMinorCityLevel())));

	
}





















#undef LOCTEXT_NAMESPACE