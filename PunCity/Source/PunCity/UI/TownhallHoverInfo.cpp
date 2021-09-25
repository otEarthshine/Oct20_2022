// Fill out your copyright notice in the Description page of Project Settings.

#include "TownhallHoverInfo.h"
#include "Components/EditableText.h"

using namespace std;

#define LOCTEXT_NAMESPACE "TownhallHoverInfo"

void UTownhallHoverInfo::PunInit(int32 townIdIn)
{
	uiTownId = townIdIn;
	
	LaborerManualCheckBox->OnCheckStateChanged.Clear();
	LaborerManualCheckBox->OnCheckStateChanged.AddUniqueDynamic(this, &UTownhallHoverInfo::OnCheckManualLaborer);
	AddToolTip(LaborerManualCheckBox,
		LOCTEXT("LaborerManualCheckBox_Tip", "Check this box to manually adjust the number of Laborers.<space>If unchecked, Laborers will be assigned after all Building Jobs (Employed) are already filled.<space>If checked, Laborers will be assigned before the Normal-Priority Building Jobs are filled (according to the number you manually set).")
	);
	
	BUTTON_ON_CLICK(LaborerArrowUp, this, &UTownhallHoverInfo::IncreaseLaborers);
	BUTTON_ON_CLICK(LaborerArrowDown, this, &UTownhallHoverInfo::DecreaseLaborers);

	BuilderManualCheckBox->OnCheckStateChanged.Clear();
	BuilderManualCheckBox->OnCheckStateChanged.AddUniqueDynamic(this, &UTownhallHoverInfo::OnCheckManualBuilder);
	AddToolTip(BuilderManualCheckBox,
		LOCTEXT("BuilderManualCheckBox_Tip", "Check this box to manually adjust the number of Builders.<space>If unchecked, Builders will be assigned after all Building Jobs (Employed) are already filled.<space>If checked, Builders will be assigned before the Normal-Priority Building Jobs are filled (according to the number you manually set).")
	);
	
	BUTTON_ON_CLICK(BuilderArrowUp, this, &UTownhallHoverInfo::IncreaseBuilders);
	BUTTON_ON_CLICK(BuilderArrowDown, this, &UTownhallHoverInfo::DecreaseBuilders);

	BUTTON_ON_CLICK(RoadMakerNonPriorityButton, this, &UTownhallHoverInfo::OnClickRoadMakerNonPriorityButton);
	BUTTON_ON_CLICK(RoadMakerPriorityButton, this, &UTownhallHoverInfo::OnClickRoadMakerPriorityButton);
	BUTTON_ON_CLICK(RoadMakerArrowUp, this, &UTownhallHoverInfo::IncreaseRoadMakers);
	BUTTON_ON_CLICK(RoadMakerArrowDown, this, &UTownhallHoverInfo::DecreaseRoadMakers);


	_laborerPriorityState.lastPriorityInputTime = -999.0f;


	BuffRow->ClearChildren();
}

void UTownhallHoverInfo::ChangedCityName(const FText& Text, ETextCommit::Type CommitMethod)
{
	if (InterfacesInvalid()) return;

	auto command = make_shared<FChangeName>();
	command->name = Text.ToString();
	command->objectId = townBuildingId();
	//PUN_LOG("ChangedCityName: %s", *command->name);
	networkInterface()->SendNetworkCommand(command);
}


void UTownhallHoverInfo::UpdateUI(bool isMini)
{
	auto& sim = simulation();
	TownManagerBase* uiTownManagerBase = sim.townManagerBase(townId());

	UpdateUIBase(isMini);

	/*
	 * 
	 */
	if (sim.HasTownhall(playerId())) // Need to have townhall, otherwise don't show anything
	{
		/*
		 * Our Town
		 */
		if (townPlayerId() == playerId())
		{
			// SendImmigrants
			const auto& townIds = sim.GetTownIds(playerId());
			if (townIds.size() > 1) {
				SendImmigrantsButton->SetVisibility(ESlateVisibility::Visible);
				BUTTON_ON_CLICK(SendImmigrantsButton, this, &UTownhallHoverInfo::OnClickSendImmigrantsButton);
			}
			else {
				SendImmigrantsButton->SetVisibility(ESlateVisibility::Collapsed);
			}

			// Gift
			GiftButton->SetVisibility(ESlateVisibility::Collapsed);
			TradeButton->SetVisibility(ESlateVisibility::Collapsed);

			// Diplomacy
			DiplomacyButton->SetVisibility(ESlateVisibility::Collapsed);

			// TrainUnits
			TrainUnitsRow->SetVisibility(ESlateVisibility::Visible);
			BUTTON_ON_CLICK(TrainUnitsButton, this, &UTownhallHoverInfo::OnClickTrainUnitsButton);

			TownManager& townMgr = sim.townManager(townId());
			std::vector<CardStatus> trainCards = townMgr.trainUnitsQueueDisplay();
			if (trainCards.size() > 0) {
				UMaterialInstanceDynamic* material = TrainUnitsClock->GetDynamicMaterial();
				material->SetScalarParameterValue(FName("Fraction"), townMgr.GetTrainingFraction());
				material->SetTextureParameterValue(FName("UnitIcon"), assetLoader()->GetCardIconNullable(trainCards[0].cardEnum));
				TrainUnitsClock->SetVisibility(ESlateVisibility::Visible);
			}
			else {
				TrainUnitsClock->SetVisibility(ESlateVisibility::Collapsed);
			}

			// Vassalize
			// (Declare Independence)
			if (uiTownManagerBase->lordPlayerId() != -1)
			{
				SetText(AttackButton1RichText, LOCTEXT("Declare Independence", "Declare Independence"));
				BUTTON_ON_CLICK(AttackButton1, this, &UTownhallHoverInfo::OnClickDeclareIndependenceButton);

				AttackButton1->SetVisibility(ESlateVisibility::Visible);
			}
			else {
				AttackButton1->SetVisibility(ESlateVisibility::Collapsed);
			}
			AttackButton2->SetVisibility(ESlateVisibility::Collapsed);


			// Buffs
			{
				std::vector<CardEnum> buffs = sim.playerOwned(townPlayerId()).GetBuffs();

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
		/*
		 * Foreign Town
		 */
		else
		{
			// SendImmigrants
			SendImmigrantsButton->SetVisibility(ESlateVisibility::Collapsed);

			// Gift
			GiftButton->SetVisibility(ESlateVisibility::Visible);
			TradeButton->SetVisibility(ESlateVisibility::Visible);
			BUTTON_ON_CLICK(GiftButton, this, &UMinorTownWorldUI::OnClickGiftButton);
			BUTTON_ON_CLICK(TradeButton, this, &UTownhallHoverInfo::OnClickTradeDealButton);

			// Diplomacy
			DiplomacyButton->SetVisibility(ESlateVisibility::Visible);
			BUTTON_ON_CLICK(DiplomacyButton, this, &UMinorTownWorldUI::OnClickDiplomacyButton);

			// Train Units
			TrainUnitsRow->SetVisibility(ESlateVisibility::Collapsed);

			if (uiTownManagerBase->isCapital())
			{
				// Not already a vassal? Might be able to attack
				if (!sim.townManagerBase(playerId())->IsVassal(townId()))
				{
					// Vassalize
					if (sim.CanVassalizeOtherPlayers(playerId()) &&
						!uiTownManagerBase->GetDefendingClaimProgress(townProvinceId()).isValid())
					{
						// Vassalize (AttackButton1)
						//SetText(AttackButton1RichText, FText::Format(
						//	LOCTEXT("VassalizeButtonRichText_Text", "Conquer (Vassalize)\n<img id=\"Influence\"/>{0}"),
						//	TEXT_NUM(sim.GetProvinceVassalizeStartPrice(townhall.provinceId()))
						//));
						SetText(AttackButton1RichText, FText::Format(
							LOCTEXT("VassalizeButtonRichText_Text", "Conquer (Vassalize)"),
							TEXT_NUM(sim.GetProvinceVassalizeStartPrice(townProvinceId()))
						));
						
						BUTTON_ON_CLICK(AttackButton1, this, &UTownhallHoverInfo::OnClickVassalizeButton);
						AttackButton1->SetVisibility(ESlateVisibility::Visible);

						// Can also liberate if there is an existing conquerer
						if (uiTownManagerBase->lordPlayerId() != -1) {
							SetText(AttackButton2RichText, FText::Format(
								LOCTEXT("LiberationButtonRichText_Text", "Liberation\n<img id=\"Influence\"/>{0}"),
								TEXT_NUM(BattleInfluencePrice)
							));
							AttackButton2->SetVisibility(ESlateVisibility::Visible);
							BUTTON_ON_CLICK(AttackButton2, this, &UTownhallHoverInfo::OnClickLiberateButton);
						}
						else {
							AttackButton2->SetVisibility(ESlateVisibility::Collapsed);
						}
					}
					else {
						AttackButton1->SetVisibility(ESlateVisibility::Collapsed);
						AttackButton2->SetVisibility(ESlateVisibility::Collapsed);
					}
				}
				else {
					AttackButton1->SetVisibility(ESlateVisibility::Collapsed);
					AttackButton2->SetVisibility(ESlateVisibility::Collapsed);
				}
			}
			// Not capital
			else
			{
				if (sim.CanVassalizeOtherPlayers(playerId()) &&
					!uiTownManagerBase->GetDefendingClaimProgress(townProvinceId()).isValid())
				{
					// Vassalize (AttackButton1)
					SetText(AttackButton1RichText, FText::Format(
						LOCTEXT("ConquerColonyButtonRichText_Text", "Conquer (Annex)\n<img id=\"Influence\"/>{0}"),
						TEXT_NUM(sim.GetProvinceConquerColonyStartPrice(townProvinceId()))
					));
					BUTTON_ON_CLICK(AttackButton1, this, &UTownhallHoverInfo::OnClickVassalizeButton);
					AttackButton1->SetVisibility(ESlateVisibility::Visible);

					AttackButton2->SetVisibility(ESlateVisibility::Collapsed);
				}
				else {
					AttackButton1->SetVisibility(ESlateVisibility::Collapsed);
					AttackButton2->SetVisibility(ESlateVisibility::Collapsed);
				}
			}

			// Buffs
			BuffRow->SetVisibility(ESlateVisibility::Collapsed);
		}
	}
	else 
	{
		SendImmigrantsButton->SetVisibility(ESlateVisibility::Collapsed);
		
		GiftButton->SetVisibility(ESlateVisibility::Collapsed);
		DiplomacyButton->SetVisibility(ESlateVisibility::Collapsed);

		TradeButton->SetVisibility(ESlateVisibility::Collapsed);
		
		TrainUnitsRow->SetVisibility(ESlateVisibility::Collapsed);

		AttackButton1->SetVisibility(ESlateVisibility::Collapsed);
		AttackButton2->SetVisibility(ESlateVisibility::Collapsed);

		BuffRow->SetVisibility(ESlateVisibility::Collapsed);
	}


	// Update population
	int population = sim.populationTown(townId());
	TownHoverPopulationText->SetText(FText::FromString(FString::FromInt(population)));

	// Don't show Laborer info if it isMini or not owned by player
	if (isMini || townPlayerId() != playerId()) {
		LaborerBuilderBox->SetVisibility(ESlateVisibility::Collapsed);
	}
	else
	{
		_laborerPriorityState.TrySyncToSimulation(&simulation(), townId(), this);
		RefreshUI();

		//// Hide unless focused on Townhall or Workplace
		//const DescriptionUIState& uiState = sim.descriptionUIState();
		//bool shouldHide = true;
		//if (uiState.objectType == ObjectTypeEnum::Building) {
		//	Building& bld = sim.building(uiState.objectId);
		//	if ((bld.maxOccupants() > 0 && !bld.isEnum(CardEnum::House)) || 
		//		bld.isEnum(CardEnum::Townhall))
		//	{
		//		shouldHide = false;
		//	}
		//}
		//LaborerBuilderBox->SetVisibility(shouldHide ? ESlateVisibility::Hidden : ESlateVisibility::SelfHitTestInvisible);

		LaborerBuilderBox->SetVisibility(ESlateVisibility::SelfHitTestInvisible);
	}



	// PlayerColorImage
	// TODO: move to Minor Town
	int32 playerIdForLogo = uiTownManagerBase->playerIdForLogo();

	if (playerIdForLogo != -1)
	{
		PunUIUtils::SetPlayerLogo(PlayerColorCircle->GetDynamicMaterial(), dataSource()->playerInfo(playerIdForLogo), assetLoader());
		PlayerColorCircle->SetVisibility(ESlateVisibility::SelfHitTestInvisible);
	}
	else {
		PlayerColorCircle->SetVisibility(ESlateVisibility::Collapsed);
	}


	//

	if (isMini) {
		SetRenderScale(FVector2D(0.5, 0.5));
	}


}


#undef LOCTEXT_NAMESPACE