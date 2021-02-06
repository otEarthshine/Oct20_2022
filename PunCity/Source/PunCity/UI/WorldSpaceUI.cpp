// Fill out your copyright notice in the Description page of Project Settings.

#include "WorldSpaceUI.h"

#include "HoverIconWidgetBase.h"

#include "BuildingJobUI.h"
#include "TownhallHoverInfo.h"

#include "../Simulation/PlayerOwnedManager.h"
#include "../Simulation/BuildingSystem.h"
#include "../Simulation/UnitStateAI.h"
#include "../Simulation/Buildings/TownHall.h"
#include "../Simulation/GameSimulationCore.h"

#include <string>
#include "Components/WidgetComponent.h"
#include "AboveBuildingText.h"
#include "PunCity/Simulation/Buildings/GathererHut.h"
#include "IconTextPairWidget.h"
#include "IconTextPair2Lines.h"
#include "PunTextWidget.h"
#include "PunCity/Simulation/HumanStateAI.h"
#include "PunBoxWidget.h"
#include "RegionHoverUI.h"

using namespace std;

#define LOCTEXT_NAMESPACE "PunBoxWidget"

void UWorldSpaceUI::SetupClasses(TSharedPtr<FSlateStyleSet> style, USceneComponent* worldWidgetParent)
{
	_style = style;
	_worldWidgetParent = worldWidgetParent;

	TArray<const FSlateBrush*> outResources;
	_style->GetResources(outResources);
	//UE_LOG(LogTemp, Warning, TEXT("PUN: Styles: %lu"), outResources.Num());
	_brushes.Empty();
	for (int i = 0; i < outResources.Num(); i++) {
		FString name = outResources[i]->GetResourceName().ToString();
		//UE_LOG(LogTemp, Warning, TEXT("PUN: --- Slate Style: %s"), *name);

		if (name.Equals(FString("HouseIcon_diffuse"))) {
			_brushes.Add("HouseIcon_diffuse") = outResources[i];
			//UE_LOG(LogTemp, Warning, TEXT("PUN: 2 Diff Style: %s"), *name);
		}
		else if (name.Equals(FString("StarvingIcon_diffuse"))) {
			_brushes.Add("StarvingIcon_diffuse") = outResources[i];
			//UE_LOG(LogTemp, Warning, TEXT("PUN: 3 Diff Style: %s"), *name);
		}
		else if (name.Equals(FString("SnowIcon_diffuse"))) {
			_brushes.Add("SnowIcon_diffuse") = outResources[i];
			//UE_LOG(LogTemp, Warning, TEXT("PUN: 4 Diff Style: %s"), *name);
		}
		else if (name.Equals(FString("NeedInputExport"))) {
			_brushes.Add("NeedInputExport") = outResources[i];
			//UE_LOG(LogTemp, Warning, TEXT("PUN: 5 Diff Style: %s"), *name);
		}
		else if (name.Equals(FString("NeedRoad"))) {
			_brushes.Add("NeedRoad") = outResources[i];
			//UE_LOG(LogTemp, Warning, TEXT("PUN: 6 Diff Style: %s"), *name);
		}
		else if (name.Equals(FString("Idling"))) {
			_brushes.Add("Idling") = outResources[i];
			//UE_LOG(LogTemp, Warning, TEXT("PUN: 7 Diff Style: %s"), *name);
		}
		else {
			//UE_LOG(LogTemp, Warning, TEXT("PUN: !!!!!!!!!!!! unknown style"), *name);
		}
	}

	_aboveBuildingTextWidget = NewObject<UWidgetComponent>(_worldWidgetParent);
	_aboveBuildingTextWidget->AttachToComponent(_worldWidgetParent, FAttachmentTransformRules::KeepRelativeTransform, NAME_None);
	_aboveBuildingTextWidget->RegisterComponent();
	_aboveBuildingTextWidget->SetWidgetSpace(EWidgetSpace::Screen);
	_aboveBuildingTextWidget->SetWidget(AddWidget<UAboveBuildingText>(UIEnum::AboveBuildingText));

	_floatupUIs.Init(this, _worldWidgetParent);

	auto punBox = CastChecked<UAboveBuildingText>(_aboveBuildingTextWidget->GetUserWidgetObject())->PunBox;
	SetChildHUD(punBox);


	_buildingJobUIs.Init();
	_townhallHoverInfos.Init();
	_regionHoverUIs.Init();
	_iconTextHoverIcons.Init();
	_unitHoverIcons.Init();
	_mapHoverIcons.Init();

	check(HoverWarningString.size() == HoverWarningDescription.size());
}

FVector2D UWorldSpaceUI::buildingScreenLocation(int buildingId)
{
	auto punHUD = GetPunHUD();
	if (punHUD->IsInvalid()) return FVector2D();

	FVector displayLocation = punHUD->dataSource()->GetBuildingTrueCenterDisplayLocation(buildingId, punHUD->inputSystemInterface()->cameraAtom());
	displayLocation += FVector(0, 0, 60);
	FVector2D screenLocation;
	punHUD->networkInterface()->WorldLocationToScreen(displayLocation, screenLocation);
	return screenLocation;
}

FVector UWorldSpaceUI::GetBuildingTrueCenterDisplayLocation(int buildingId, float height)
{
	auto punHUD = GetPunHUD();
	FVector displayLocation = punHUD->dataSource()->GetBuildingTrueCenterDisplayLocation(buildingId, punHUD->inputSystemInterface()->cameraAtom());
	displayLocation += FVector(0, 0, height);
	return displayLocation;
}

void UWorldSpaceUI::TickWorldSpaceUI()
{
#if !UI_WORLDSPACE
	return;
#endif
	if (!PunSettings::IsOn("UIWorldSpace")) {
		return;
	}
	
	TickBuildings();
	TickUnits();
	TickMap();

	auto& simulation = dataSource()->simulation();

	TickPlacementInstructions();

	// Hide jobUIs unless clicked
	DescriptionUIState uiState =  simulation.descriptionUIState();
	jobUIState = JobUIState::None;
	if (uiState.objectType == ObjectTypeEnum::Building && uiState.objectId >= 0)
	{
		Building& building = dataSource()->GetBuilding(uiState.objectId);
		if (!building.isConstructed()) {
			jobUIState = JobUIState::Job;
		}
		else if (IsHumanHouse(building.buildingEnum())) {
			jobUIState = JobUIState::Home;
		}
		else if (building.isEnum(CardEnum::StorageYard) ||
				building.isEnum(CardEnum::Warehouse)) 
		{
			jobUIState = JobUIState::Storage;
		}
		else {
			jobUIState = JobUIState::Job;
		}
	}
}

void UWorldSpaceUI::TickBuildings()
{
	if (InterfacesInvalid()) return;

	IGameUIDataSource* data = dataSource();
	float zoomDistance = dataSource()->zoomDistance();

	std::vector<int32> buildingIdsToDisplay;
	const std::vector<int32>& sampleRegionIds = data->sampleRegionIds();

	auto& sim = data->simulation();
	auto& buildingSystem = sim.buildingSystem();
	auto& buildingList = buildingSystem.buildingSubregionList();

	/*
	 * Fill buildingIdsToDisplay
	 */
	for (int32 sampleRegionId : sampleRegionIds)
	{
		WorldRegion2 curRegion(sampleRegionId);

		buildingList.ExecuteRegion(curRegion, [&](int32 buildingId)
		{
			Building& building = dataSource()->GetBuilding(buildingId);
			if (IsRoad(building.buildingEnum()) ||
				building.isEnum(CardEnum::Fence))
			{
				if (building.workReservers().size() > 0) {
					buildingIdsToDisplay.push_back(buildingId);
				}
			}
			else
			{
				buildingIdsToDisplay.push_back(buildingId);
			}

		});
	}

	auto& playerOwned = sim.playerOwned(playerId());
	auto& provinceSys = sim.provinceSystem();

	const std::vector<int32>& sampleProvinceIds = dataSource()->sampleProvinceIds();

	/*
	 * Province UI
	 */
	{
		for (int32 provinceId : sampleProvinceIds)
		{
			int32 provincePlayerId = sim.provinceOwnerPlayer(provinceId);
			bool showingBattle = false;

			auto getRegionHoverUI = [&]() {
				WorldTile2 provinceCenter = provinceSys.GetProvinceCenterTile(provinceId);
				FVector displayLocation = MapUtil::DisplayLocation(data->cameraAtom(), provinceCenter.worldAtom2(), 50.0f);
				return _regionHoverUIs.GetHoverUI<URegionHoverUI>(provinceId, UIEnum::RegionHoverUI, this, _worldWidgetParent, displayLocation, dataSource()->zoomDistance(),
					[&](URegionHoverUI* ui) 
					{
						ui->IconImage->SetBrushFromMaterial(assetLoader()->M_GeoresourceIcon); // SetBrushFromMaterial must be here since doing it every tick causes leak
					}, 
					WorldZoomTransition_Region4x4ToMap
				);
			};
			
			if (provincePlayerId != -1)
			{	
				ProvinceClaimProgress claimProgress = sim.playerOwned(provincePlayerId).GetDefendingClaimProgress(provinceId);
				if (claimProgress.isValid())
				{
					URegionHoverUI* regionHoverUI = getRegionHoverUI();

					bool isUIPlayerAttacker = claimProgress.attackerPlayerId == playerId();

					//std::stringstream ss;
					//std::string textType = (isUIPlayerAttacker ? "<Large>" : "<LargeRed>");
					//ss << textType << "Attacker: " << simulation().playerName(claimProgress.attackerPlayerId) << "</>\n";
					//ss << textType << claimProgress.committedInfluences << "</><img id=\"Influence\"/>";
					//SetText(regionHoverUI->ClaimingText, ss.str());
					//regionHoverUI->AutoChoseText->SetVisibility(ESlateVisibility::Collapsed);

					// Battle Bar
					float fraction = static_cast<float>(claimProgress.ticksElapsed) / BattleClaimTicks;
					regionHoverUI->BattleBarImage->GetDynamicMaterial()->SetScalarParameterValue("Fraction", 1.0f - fraction);
					regionHoverUI->BattleBarImage->GetDynamicMaterial()->SetScalarParameterValue("IsGreenLeft", isUIPlayerAttacker);
					regionHoverUI->BattleBarImage->SetVisibility(ESlateVisibility::SelfHitTestInvisible);

					
					// Player Logo
					regionHoverUI->PlayerLogoLeft->GetDynamicMaterial()->SetVectorParameterValue("PlayerColor1", PlayerColor1(claimProgress.attackerPlayerId));
					AddToolTip(regionHoverUI->PlayerLogoLeft, FText::Format(
						LOCTEXT("AttackPlayerLogo_Tip", "Attacker: {0}"),
						sim.playerNameT(claimProgress.attackerPlayerId)
					));

					int32 defenderPlayerId = provincePlayerId;
					bool isDeclaringIndependence = (claimProgress.attackerPlayerId == provincePlayerId);
					if (isDeclaringIndependence) {
						defenderPlayerId = sim.playerOwned(provincePlayerId).lordPlayerId(); // Declare Independence
					} // TODO: Declare Independence should init attack from the Lord
					regionHoverUI->PlayerLogoRight->GetDynamicMaterial()->SetVectorParameterValue("PlayerColor2", PlayerColor2(defenderPlayerId));
					AddToolTip(regionHoverUI->PlayerLogoRight, FText::Format(
						LOCTEXT("DefenderPlayerLogo_Tip", "Defender: {0}"),
						sim.playerNameT(defenderPlayerId)
					));
					

					// 
					SetText(regionHoverUI->BattleInfluenceLeft, FText::Format(INVTEXT("<img id=\"Influence\"/><Shadowed>{0}</>"), TEXT_NUM(claimProgress.committedInfluencesAttacker)));
					SetText(regionHoverUI->BattleInfluenceRight, FText::Format(INVTEXT("<img id=\"Influence\"/><Shadowed>{0}</>"), TEXT_NUM(claimProgress.committedInfluencesDefender)));
					SetText(regionHoverUI->DefenseBonusLeft, FText::Format(INVTEXT("<img id=\"Shield\"/><Shadowed>{0}</>"), TEXT_NUM(0)));

					AddToolTip(regionHoverUI->DefenseBonusLeft, 
						LOCTEXT("Attack Bonus: 0%", "Attack Bonus: 0%")
					);
					
					std::string defenderDefenseBonus = (isDeclaringIndependence ? to_string(0) : to_string(sim.GetProvinceAttackCostPercent(provinceId))) + "%</>";
					SetText(regionHoverUI->DefenseBonusRight, "<img id=\"Shield\"/><Shadowed>" + defenderDefenseBonus);
					AddToolTip(regionHoverUI->DefenseBonusLeft, sim.GetProvinceDefenseBonusTip(provinceId));

					// Fight at home province = Vassalize
					if (sim.homeProvinceId(provincePlayerId) == provinceId) {
						SetText(regionHoverUI->BattleText, TEXT_TAG("<Shadowed>", LOCTEXT("Vassalize", "Vassalize")));
					}
					else {
						SetText(regionHoverUI->BattleText, TEXT_TAG("<Shadowed>", LOCTEXT("Annex Province", "Annex Province")));
					}
					

					// UI-Player is Attacker
					if (claimProgress.attackerPlayerId == playerId()) 
					{
						int32 provincePlayerIdTemp = sim.provinceOwnerPlayer(claimProgress.provinceId); //TODO: does it needs claimProgress.provinceId?
						auto& provincePlayerOwner = sim.playerOwned(provincePlayerIdTemp);
						
						ClaimConnectionEnum claimConnectionEnum = sim.GetProvinceClaimConnectionEnumPlayer(provinceId, claimProgress.attackerPlayerId);
						ProvinceAttackEnum attackEnum = provincePlayerOwner.GetProvinceAttackEnum(provinceId, claimProgress.attackerPlayerId);
						
						int32 reinforcePrice = (attackEnum == ProvinceAttackEnum::DeclareIndependence) ? BattleInfluencePrice : sim.GetProvinceAttackReinforcePrice(provinceId, claimConnectionEnum);
						bool hasEnoughInfluence = sim.influence(playerId()) >= reinforcePrice;

						SetText(regionHoverUI->ReinforceLeftButtonText, 
							FText::Format(INVTEXT("{0}\n<img id=\"Influence\"/>{1}"), LOCTEXT("Reinforce", "Reinforce"), TextRed(TEXT_NUM(reinforcePrice), !hasEnoughInfluence))
						);
						regionHoverUI->ReinforceLeftButton->SetIsEnabled(hasEnoughInfluence);
						
						regionHoverUI->ReinforceLeftButtonText->SetVisibility(ESlateVisibility::Visible);
						regionHoverUI->ReinforceRightButtonText->SetVisibility(ESlateVisibility::Collapsed);
						regionHoverUI->ReinforceMoneyRightButtonText->SetVisibility(ESlateVisibility::Collapsed);
					}
					// UI-Player is Defender
					else if (provincePlayerId == playerId())
					{
						int32 hasEnoughInfluence = sim.influence(playerId()) >= BattleInfluencePrice;
						SetText(regionHoverUI->ReinforceRightButtonText, 
							FText::Format(INVTEXT("{0}\n<img id=\"Influence\"/>{1}"), LOCTEXT("Reinforce", "Reinforce"), TextRed(TEXT_NUM(BattleInfluencePrice), !hasEnoughInfluence))
						);
						regionHoverUI->ReinforceRightButton->SetIsEnabled(hasEnoughInfluence);

						int32 hasEnoughMoney = sim.money(playerId()) >= BattleInfluencePrice;
						SetText(regionHoverUI->ReinforceMoneyRightButtonText,
							FText::Format(INVTEXT("{0}\n<img id=\"Coin\"/>{1}"), LOCTEXT("Reinforce", "Reinforce"), TextRed(TEXT_NUM(BattleInfluencePrice), !hasEnoughInfluence))
						);
						regionHoverUI->ReinforceMoneyRightButton->SetIsEnabled(hasEnoughMoney);
						
						regionHoverUI->ReinforceLeftButtonText->SetVisibility(ESlateVisibility::Collapsed);
						regionHoverUI->ReinforceRightButtonText->SetVisibility(ESlateVisibility::Visible);
						regionHoverUI->ReinforceMoneyRightButtonText->SetVisibility(ESlateVisibility::Visible);
					}
					else {
						regionHoverUI->ReinforceLeftButtonText->SetVisibility(ESlateVisibility::Collapsed);
						regionHoverUI->ReinforceRightButtonText->SetVisibility(ESlateVisibility::Collapsed);
						regionHoverUI->ReinforceMoneyRightButtonText->SetVisibility(ESlateVisibility::Collapsed);
					}
					
					regionHoverUI->UpdateUI(provinceId);

					regionHoverUI->ProvinceOverlay->SetVisibility(ESlateVisibility::Collapsed);
					regionHoverUI->BattleOverlay->SetVisibility(ESlateVisibility::SelfHitTestInvisible);

					showingBattle = true;
				}
			}

			if (!showingBattle &&
				zoomDistance < WorldZoomTransition_Region4x4ToMap &&
				dataSource()->isShowingProvinceOverlay())
			{
				URegionHoverUI* regionHoverUI = getRegionHoverUI();
				regionHoverUI->UpdateProvinceOverlayInfo(provinceId);

				regionHoverUI->ProvinceOverlay->SetVisibility(ESlateVisibility::SelfHitTestInvisible);
				regionHoverUI->BattleOverlay->SetVisibility(ESlateVisibility::Collapsed);
			}
		}
	}
	

	OverlayType overlayType = data->GetOverlayType();
	WorldTile2 overlayTile = dataSource()->GetOverlayTile();

	for (int buildingId : buildingIdsToDisplay) 
	{
		if (buildingId == -1) continue; // displayedBuilding list is from BuildingDisplaySystem, invalid objectId is possible

		Building& building = dataSource()->GetBuilding(buildingId);

		if (building.isEnum(CardEnum::Townhall)) 
		{
			if (townhallUIActive) {
				TickTownhallInfo(buildingId);

				// Townhall Upgrade notification...
				if (simulation().parameters(playerId())->NeedTownhallUpgradeNoticed)
				{
					bool isTownhallUIOpened = simulation().descriptionUIState().objectType == ObjectTypeEnum::Building &&
												simulation().building(simulation().descriptionUIState().objectId).isEnum(CardEnum::Townhall);

					if (!isTownhallUIOpened)
					{
						UIconTextPairWidget* hoverIcon = _iconTextHoverIcons.GetHoverUI<UIconTextPairWidget>(buildingId, UIEnum::HoverTextIconPair, this,
							_worldWidgetParent, GetBuildingTrueCenterDisplayLocation(buildingId), zoomDistance, [&](UIconTextPairWidget* ui) {},
							WorldZoomTransition_WorldSpaceUIShrink, 2.0f);

						hoverIcon->SetImage(assetLoader()->ExclamationIcon);
						hoverIcon->SetText(FText(), FText());
					}
				}
			}
			
		} else {
			// Don't show jobUI at all beyond some zoom
			//  (Zoom distance... 455 to 541)
			if (zoomDistance < WorldZoomTransition_WorldSpaceUIHide) {
				TickJobUI(buildingId);
			}
		}


		auto isInOverlayRadiusHouse = [&](OverlayType overlayTypeCurrent, int32 minimumHouseLvl, int32 radius)
		{
			return overlayType == overlayTypeCurrent &&
				building.isEnum(CardEnum::House) &&
				building.subclass<House>().houseLvl() >= minimumHouseLvl &&
				WorldTile2::Distance(building.centerTile(), overlayTile) < radius &&
				building.playerId() == playerId();
		};

		auto isInOverlayRadius = [&](OverlayType overlayTypeCurrent, CardEnum buildingEnum, int32 radius)
		{
			return overlayType == overlayTypeCurrent &&
				building.isEnum(buildingEnum) &&
				WorldTile2::Distance(building.centerTile(), overlayTile) < radius &&
				building.playerId() == playerId();
		};
		
		/*
		 * Science
		 */
		
		if (isInOverlayRadiusHouse(OverlayType::Library, Library::MinHouseLvl, Library::Radius)) 
		{
			UIconTextPairWidget* hoverIcon = _iconTextHoverIcons.GetHoverUI<UIconTextPairWidget>(buildingId, UIEnum::HoverTextIconPair, this,
																								_worldWidgetParent, GetBuildingTrueCenterDisplayLocation(buildingId), zoomDistance, [&](UIconTextPairWidget* ui) {});

			hoverIcon->SetImage(assetLoader()->ScienceIcon);

			bool alreadyHasLibrary = false; ;
			if (building.subclass<House>().GetScience100(ScienceEnum::Library) > 0) {
				alreadyHasLibrary = true;
			}
			else if (building.occupantCount() == 0) {
				int32 radiusBonus = building.GetRadiusBonus(CardEnum::Library, Library::Radius, [&](int32 bonus, Building& buildingScope) {
					return max(bonus, Library::SciencePerHouse);
				});
				if (radiusBonus > 0) {
					alreadyHasLibrary = true;
				}
			}
			
			hoverIcon->SetText("", "+" + to_string(alreadyHasLibrary ? 0 : Library::SciencePerHouse));
			hoverIcon->SetTextColor(alreadyHasLibrary ? FLinearColor(0.38, 0.38, 0.38, 0.5) : FLinearColor::White);
		}
		else if (isInOverlayRadiusHouse(OverlayType::School, School::MinHouseLvl, School::Radius))
		{
			UIconTextPairWidget* hoverIcon = _iconTextHoverIcons.GetHoverUI<UIconTextPairWidget>(buildingId, UIEnum::HoverTextIconPair, this,
																								_worldWidgetParent, GetBuildingTrueCenterDisplayLocation(buildingId), zoomDistance, [&](UIconTextPairWidget* ui) {});

			hoverIcon->SetImage(assetLoader()->ScienceIcon);
			hoverIcon->SetText("", "+" + to_string(School::SciencePerHouse));

			bool alreadyHasSchool = building.subclass<House>().GetScience100(ScienceEnum::School) > 0;
			hoverIcon->SetTextColor(alreadyHasSchool ? FLinearColor(0.38, 0.38, 0.38) : FLinearColor::White);
		}
		/*
		 * Entertainment
		 */
		else if (isInOverlayRadiusHouse(OverlayType::Tavern, 1, Tavern::Radius))
		{
			UIconTextPairWidget* hoverIcon = _iconTextHoverIcons.GetHoverUI<UIconTextPairWidget>(buildingId, UIEnum::HoverTextIconPair, this,
				_worldWidgetParent, GetBuildingTrueCenterDisplayLocation(buildingId), zoomDistance, [&](UIconTextPairWidget* ui) {});

			hoverIcon->SetImage(assetLoader()->SmileIcon);
			hoverIcon->SetText(FText(), FText());
		}
		else if (isInOverlayRadiusHouse(OverlayType::Theatre, Theatre::MinHouseLvl, Theatre::Radius))
		{
			UIconTextPairWidget* hoverIcon = _iconTextHoverIcons.GetHoverUI<UIconTextPairWidget>(buildingId, UIEnum::HoverTextIconPair, this,
				_worldWidgetParent, GetBuildingTrueCenterDisplayLocation(buildingId), zoomDistance, [&](UIconTextPairWidget* ui) {});

			hoverIcon->SetImage(assetLoader()->SmileIcon);
			hoverIcon->SetText(FText(), FText());
		}

		
		/*
		 * Others
		 */
		else if (isInOverlayRadiusHouse(OverlayType::Bank, Bank::MinHouseLvl, Bank::Radius))
		{
			UIconTextPairWidget* hoverIcon = _iconTextHoverIcons.GetHoverUI<UIconTextPairWidget>(buildingId, UIEnum::HoverTextIconPair, this,
				_worldWidgetParent, GetBuildingTrueCenterDisplayLocation(buildingId), zoomDistance, [&](UIconTextPairWidget* ui) {});

			hoverIcon->SetImage(assetLoader()->CoinIcon);
			hoverIcon->SetText(FText(), TEXT_NUMSIGNED(Bank::ProfitPerHouse));
		}
		// Bad Appeal
		else if (overlayType == OverlayType::BadAppeal && IsHumanHouse(building.buildingEnum()) && 
				WorldTile2::Distance(building.centerTile(), overlayTile) < BadAppealRadius)
		{
			UIconTextPairWidget* hoverIcon = _iconTextHoverIcons.GetHoverUI<UIconTextPairWidget>(buildingId, UIEnum::HoverTextIconPair, this,
				_worldWidgetParent, GetBuildingTrueCenterDisplayLocation(buildingId), zoomDistance, [&](UIconTextPairWidget* ui) {},
			WorldZoomTransition_WorldSpaceUIShrink, 1.5);

			hoverIcon->SetImage(assetLoader()->UnhappyIcon);
			hoverIcon->SetText(FText(), FText());
		}

		/*
		 * Workplace
		 */
		else if (isInOverlayRadius(OverlayType::Windmill, CardEnum::Farm, Windmill::Radius))
		{
			UIconTextPairWidget* hoverIcon = _iconTextHoverIcons.GetHoverUI<UIconTextPairWidget>(buildingId, UIEnum::HoverTextIconPair, this,
				_worldWidgetParent, GetBuildingTrueCenterDisplayLocation(buildingId), zoomDistance, [&](UIconTextPairWidget* ui) {});

			//hoverIcon->SetImage(assetLoader()->ScienceIcon);
			hoverIcon->IconImage->SetVisibility(ESlateVisibility::Collapsed);
			hoverIcon->SetText(FText(), INVTEXT("+10%"));
			hoverIcon->SetTextColor(FLinearColor::White);
		}


		
		
		//else if (overlayType == OverlayType::Theatre && IsHumanHouse(building.buildingEnum()) &&
		//		WorldTile2::Distance(building.centerTile(), overlayTile) < Theatre::Radius) 
		//{
		//	UIconTextPairWidget* hoverIcon = _iconTextHoverIcons.GetHoverUI<UIconTextPairWidget>(buildingId, UIEnum::HoverTextIconPair, -1000, this,
		//		_worldWidgetParent, GetBuildingTrueCenterDisplayLocation(buildingId), [&](UIconTextPairWidget* ui) {});
		//	hoverIcon->SetImage(assetLoader()->CultureIcon);
		//	hoverIcon->SetText("", "+" + to_string(Theatre::BaseCultureByLvl[0]));
		//}
		//else if (overlayType == OverlayType::Tavern && IsHumanHouse(building.buildingEnum()) &&
		//		WorldTile2::Distance(building.centerTile(), overlayTile) < Tavern::Radius) 
		//{
		//	UIconTextPairWidget* hoverIcon = _iconTextHoverIcons.GetHoverUI<UIconTextPairWidget>(buildingId, UIEnum::HoverTextIconPair, this,
		//																						_worldWidgetParent, GetBuildingTrueCenterDisplayLocation(buildingId), zoomDistance, [&](UIconTextPairWidget* ui) {});
		//	hoverIcon->SetImage(assetLoader()->SmileIcon);
		//	hoverIcon->SetText("", "+" + to_string(Tavern::BaseHappinessByLvl[0]));
		//}
	}

	//! Remove unused UIs
	_buildingJobUIs.AfterAdd();
	_townhallHoverInfos.AfterAdd();
	_regionHoverUIs.AfterAdd();

	_buildingHoverIcons.AfterAdd();

	_iconTextHoverIcons.AfterAdd();

	_floatupUIs.Tick();

	/*
	 * FloatupInfo
	 */
	if (zoomDistance < WorldZoomTransition_WorldSpaceUIHide)
	{
		for (const FloatupInfo& floatupInfo : _floatupInfos)
		{
			if (Time::Ticks() - floatupInfo.startTick < Time::TicksPerSecond * 10)
			{
				WorldAtom2 worldAtom = floatupInfo.tile.worldAtom2();

				if (floatupInfo.floatupEnum == FloatupEnum::ComboComplete) {
					UPunTextWidget* comboText = _floatupUIs.AddHoverUI<UPunTextWidget>(UIEnum::ComboComplete, worldAtom);
					comboText->SetText(floatupInfo.text);
				}
				if (floatupInfo.floatupEnum == FloatupEnum::BuildingComplete) {
					_floatupUIs.AddHoverUI<UPunTextWidget>(UIEnum::BuildingComplete, worldAtom);
				}
				if (floatupInfo.floatupEnum == FloatupEnum::HouseUpgrade) {
					_floatupUIs.AddHoverUI<UPunTextWidget>(UIEnum::HouseUpgrade, worldAtom);
				}
				if (floatupInfo.floatupEnum == FloatupEnum::HouseDowngrade) {
					_floatupUIs.AddHoverUI<UPunTextWidget>(UIEnum::HouseDowngrade, worldAtom);
				}
				if (floatupInfo.floatupEnum == FloatupEnum::TownhallUpgrade) {
					_floatupUIs.AddHoverUI<UPunTextWidget>(UIEnum::TownhallUpgrade, worldAtom);
				}

				//else if (floatupInfo.floatupEnum == FloatupEnum::GainResource) {
				//	//UIconTextPairWidget* iconTextPair = _floatupUIs.AddHoverUI<UIconTextPairWidget>(UIEnum::HoverTextIconPair, worldAtom);
				//	//iconTextPair->SetText(floatupInfo.text, "");
				//	//iconTextPair->SetImage(floatupInfo.resourceEnum, dataSource()->assetLoader());
				//}
				else if (floatupInfo.floatupEnum == FloatupEnum::GainMoney ||
						floatupInfo.floatupEnum == FloatupEnum::GainScience ||
						floatupInfo.floatupEnum == FloatupEnum::GainInfluence ||
						floatupInfo.floatupEnum == FloatupEnum::GainResource)
				{
					auto assetLoader = dataSource()->assetLoader();
					
					UIconTextPair2Lines* iconTextPair3Lines = _floatupUIs.AddHoverUI<UIconTextPair2Lines>(UIEnum::HoverTextIconPair3Lines, worldAtom);
					if (!floatupInfo.text.IsEmpty() && 
						floatupInfo.text.ToString() != TEXT("0") && 
						floatupInfo.text.ToString() != TEXT("+0"))
					{
						iconTextPair3Lines->IconPair1->SetText(floatupInfo.text, FText());

						if (floatupInfo.floatupEnum == FloatupEnum::GainMoney) {
							iconTextPair3Lines->IconPair1->SetImage(assetLoader->CoinIcon);
						} else if (floatupInfo.floatupEnum == FloatupEnum::GainScience) {
							iconTextPair3Lines->IconPair1->SetImage(assetLoader->ScienceIcon);
						} else if (floatupInfo.floatupEnum == FloatupEnum::GainInfluence) {
							iconTextPair3Lines->IconPair1->SetImage(assetLoader->InfluenceIcon);
						} else {
							iconTextPair3Lines->IconPair1->SetImage(floatupInfo.resourceEnum, assetLoader);
						}
						iconTextPair3Lines->IconPair1->SetVisibility(ESlateVisibility::SelfHitTestInvisible);
					}
					else {
						iconTextPair3Lines->IconPair1->SetVisibility(ESlateVisibility::Collapsed);
					}
					if (!floatupInfo.text2.IsEmpty() &&
						floatupInfo.text2.ToString() != TEXT("0") &&
						floatupInfo.text.ToString() != TEXT("+0"))
					{
						iconTextPair3Lines->IconPair2->SetText(floatupInfo.text2, FText());

						if (floatupInfo.floatupEnum == FloatupEnum::GainMoney) {
							iconTextPair3Lines->IconPair2->SetImage(assetLoader->InfluenceIcon); //TODO: this is not used?
						} else {
							iconTextPair3Lines->IconPair2->SetImage(floatupInfo.resourceEnum2, assetLoader);
						}
						iconTextPair3Lines->IconPair2->SetVisibility(ESlateVisibility::SelfHitTestInvisible);
					}
					else {
						iconTextPair3Lines->IconPair2->SetVisibility(ESlateVisibility::Collapsed);
					}

					iconTextPair3Lines->IconPair3->SetVisibility(ESlateVisibility::Collapsed);
				}
			}
		}
	}
	
	_floatupInfos.clear();
}

UBuildingJobUI* UWorldSpaceUI::GetJobUI(int32 buildingId, int32 height)
{
	Building& building = dataSource()->GetBuilding(buildingId);

	UBuildingJobUI* buildingJobUI = _buildingJobUIs.GetHoverUI<UBuildingJobUI>(buildingId, UIEnum::HoverBuildingJob, this, _worldWidgetParent, GetBuildingTrueCenterDisplayLocation(buildingId, height), dataSource()->zoomDistance(),
		[&](UBuildingJobUI* ui) {
			ui->PunInit(buildingId, IsHouse(building.buildingEnum()));
		}
	);
	return buildingJobUI;
}

void UWorldSpaceUI::TickJobUI(int buildingId)
{
	if (InterfacesInvalid()) return;

	Building& building = dataSource()->GetBuilding(buildingId);

	if (building.isFireDisabled()) {
		return;
	}

	FLinearColor brown(1, .75, .5);

	bool isTileBld = IsRoad(building.buildingEnum()) || building.isEnum(CardEnum::Fence);
	if (isTileBld)
	{
		UBuildingJobUI* buildingJobUI = GetJobUI(buildingId, 10);
		
		//if (jobUIState == JobUIState::Job)
		//{
			buildingJobUI->SetShowHumanSlots(true, false, true);
			buildingJobUI->SetSlots(1, 1, 1, brown);
		//}
		//else {
		//	buildingJobUI->SetShowHumanSlots(false, false);
		//}
		
		buildingJobUI->SetShowBar(false);
		//buildingJobUI->SetStars(0);
		return;
	}

	int32 jobUIHeight = 25;
	if (building.isEnum(CardEnum::Windmill)) {
		jobUIHeight = 50;
	}

	UBuildingJobUI* buildingJobUI = GetJobUI(buildingId, jobUIHeight);


	// Special case hide when constructed
	if (building.isConstructed())
	{
		//switch(building.buildingEnum())
		//{
		//case CardEnum::StorageYard:
		//	buildingJobUI->SetVisibility(ESlateVisibility::Collapsed);
		//	return;
		//default:
		//	break;
		//}
	}
	buildingJobUI->SetVisibility(ESlateVisibility::SelfHitTestInvisible);


	// Under construction
	if (!building.isConstructed())
	{
		auto showAlwaysOnPart = [&]()
		{
			buildingJobUI->SetShowBar(true);
			buildingJobUI->SetBarFraction(building.constructionFraction());

			std::vector<int32> constructionResourceCounts;
			std::vector<int32> constructionResourceRequired = building.GetConstructionResourceCost();
			
			for (size_t i = 0; i < constructionResourceRequired.size(); i++) {
				constructionResourceCounts.push_back(0);
				if (constructionResourceRequired[i] > 0) {
					constructionResourceCounts[i] = building.resourceCount(ConstructionResources[i]);
				}
			}
			buildingJobUI->SetConstructionResource(constructionResourceCounts, building);

			buildingJobUI->SetHoverWarning(building);
		};
		
		// Full UI
		if (jobUIState == JobUIState::Job)
		{
			bool canManipulateOccupant = playerId() == building.playerId();
			buildingJobUI->SetShowHumanSlots(true, canManipulateOccupant);
			buildingJobUI->SetSlots(building.occupantCount(), building.allowedOccupants(), building.maxOccupants(), brown);

			showAlwaysOnPart();
			return;
		}
		// Only progress bar and star
		buildingJobUI->SetShowHumanSlots(false, false);
		buildingJobUI->SetSlots(0, 0, 0, brown);

		showAlwaysOnPart();

		buildingJobUI->SetSpeedBoost(building);

		return;
	}
	// After construction is done, shouldn't be any ResourceCompletion circle left
	buildingJobUI->ClearResourceCompletionBox();

	// Houses
	if (IsHouse(building.buildingEnum()))
	{
		if (IsHumanHouse(building.buildingEnum()))
		{
			FLinearColor lightGreen(0.7, 1, 0.7);

			// Show house UI only when clicked
			if (jobUIState == JobUIState::Home)
			{
				buildingJobUI->SetShowHumanSlots(true, false);
				buildingJobUI->SetSlots(building.occupantCount(), building.allowedOccupants(), building.allowedOccupants(), lightGreen);
			}
			else
			{
				buildingJobUI->SetShowHumanSlots(false, false);
			}

			// Show bar if this house is being upgraded
			auto& house = building.subclass<House>(CardEnum::House);
			if (house.isUpgrading())
			{
				buildingJobUI->SetShowBar(false, true);
				buildingJobUI->SetHouseBarFraction(house.upgradeProgressPercent() / 100.0f);
			}
			else {
				buildingJobUI->SetShowBar(false);
			}

			buildingJobUI->SetSpeedBoost(building);
			
			return;
		}
		
		// Don't show animal houses
		//buildingJobUI->SetStars(0);
		return;
	}

	// Non-house finished buildings beyond this
	bool showOccupants = building.maxOccupants() > 0;

	// Processor/Trader ... show progress bar
	bool showProgress = IsProducerProcessor(building.buildingEnum()) ||
						IsSpecialProducer(building.buildingEnum()) ||
						IsTradingPostLike(building.buildingEnum()) ||
						building.isEnum(CardEnum::TradingCompany) ||
						IsBarrack(building.buildingEnum()) ||

						building.isEnum(CardEnum::HuntingLodge) ||
						building.isEnum(CardEnum::FruitGatherer) ||
						building.isEnum(CardEnum::ShippingDepot) ||
						IsStorage(building.buildingEnum()) || // TODO: may be this is for all buildings??

						building.isEnum(CardEnum::JobManagementBureau) ||
						building.isEnum(CardEnum::StatisticsBureau);

	// Every buildings can be boosted
	buildingJobUI->SetSpeedBoost(building);
	
	if (jobUIState == JobUIState::Job)
	{
		if (showOccupants || showProgress)
		{
			bool canManipulateOccupant = showOccupants && playerId() == building.playerId();
			buildingJobUI->SetShowHumanSlots(showOccupants, canManipulateOccupant);
			buildingJobUI->SetShowBar(false);

			if (showOccupants) {
				buildingJobUI->SetSlots(building.occupantCount(), building.allowedOccupants(), building.maxOccupants(), FLinearColor::White);
			} else {
				buildingJobUI->SetSlots(0, 0, 0, FLinearColor::White);
			}

			if (showProgress) {
				buildingJobUI->SetBuildingStatus(building, jobUIState);
				buildingJobUI->SetHoverWarning(building);
			}

			//buildingJobUI->SetStars(building.level());
			return;
		}
	}
	else
	{
		// Always show the progress
		buildingJobUI->SetShowHumanSlots(false, false);
		buildingJobUI->SetShowBar(false);

		if (showProgress) {
			buildingJobUI->SetBuildingStatus(building, jobUIState);
			buildingJobUI->SetHoverWarning(building);
		}

		//buildingJobUI->SetStars(building.level());
		return;
	}
	

	// Always show stars
	if (building.level() > 0)
	{
		buildingJobUI->SetShowHumanSlots(false, false);
		buildingJobUI->SetShowBar(false);

		buildingJobUI->SetSlots(0, 0, 0, FLinearColor::White);
		//buildingJobUI->SetStars(building.level());
		return;
	}

}

void UWorldSpaceUI::TickTownhallInfo(int buildingId, bool isMini)
{
	if (InterfacesInvalid()) return;

	IGameUIDataSource* data = dataSource();
	Building& building = data->GetBuilding(buildingId);

	// Townhall special case
	if (building.isEnum(CardEnum::Townhall))
	{
		TownHall* townhall = static_cast<TownHall*>(&building);

		UTownhallHoverInfo* townhallInfo = _townhallHoverInfos.GetHoverUI<UTownhallHoverInfo>(buildingId, UIEnum::HoverTownhall, this, _worldWidgetParent, GetBuildingTrueCenterDisplayLocation(buildingId), dataSource()->zoomDistance(),
			[&](UTownhallHoverInfo* ui) {
				ui->CityNameText->SetText(townhall->townNameT());
				ui->CityNameText->SetVisibility(ESlateVisibility::SelfHitTestInvisible);
				//ui->CityNameEditableText->SetVisibility(ESlateVisibility::Collapsed);
				AddToolTip(ui->Laborer, 
					LOCTEXT("TownhallLaborerUI_Tip", "Citizens without assigned workplaces become laborers. Laborers helps with hauling, gathering, and land clearing.")
				);
				ui->PunInit(buildingId);
			}
		);

		// Do not accept input during placement
		//bool isPlacing = inputSystemInterface()->placementState() != PlacementType::None;
		//townhallInfo->SetVisibility(isPlacing ? ESlateVisibility::Collapsed : ESlateVisibility::Visible);

		townhallInfo->UpdateUI(isMini);
	}
}

void UWorldSpaceUI::TickUnits()
{
	if (InterfacesInvalid()) return;

	IGameUIDataSource* data = dataSource();


	std::vector<int> unitIdsToDisplay;
	const std::vector<int32>& sampleRegionIds = data->sampleRegionIds();

	auto& unitLists = data->simulation().unitSystem().unitSubregionLists();

	// TODO: don't do accumulation and use ExecuteRegion directly??
	for (int32_t sampleRegionId : sampleRegionIds) {
		unitLists.ExecuteRegion(WorldRegion2(sampleRegionId), [&](int32_t unitId) {
			unitIdsToDisplay.push_back(unitId);
		});
	}

	for (int unitId : unitIdsToDisplay) {
		if (unitId == -1) continue;

		UnitStateAI& unitAI = data->GetUnitStateAI(unitId);

		if (unitAI.unitEnum() != UnitEnum::Human) {
			continue;
		}
		HumanStateAI& human = unitAI.subclass<HumanStateAI>();

		bool needHousing = unitAI.needHouse();
		bool needFood = unitAI.showNeedFood();
		bool needHeat = unitAI.showNeedHeat();
		bool needHappiness = human.needHappiness();
		bool needHealthcare = human.needHealthcare();
		bool needTools = human.needTools();
		bool idling = false;// unitAI.unitState() == UnitState::Idle;
		// Don't forget to add this in DespawnUnusedUIs too...

		if (needHousing || 
			needFood || 
			needHeat || 
			needHappiness ||
			needHealthcare ||
			needTools ||
			idling)
		{
			FVector displayLocation = data->GetUnitDisplayLocation(unitId, inputSystemInterface()->cameraAtom());
			displayLocation += FVector(0, 0, 25);

			UHoverIconWidgetBase* hoverIcon = _unitHoverIcons.GetHoverUI<UHoverIconWidgetBase>(unitId, UIEnum::HoverIcon, this, _worldWidgetParent, displayLocation, dataSource()->zoomDistance(), 
																								[&](UHoverIconWidgetBase* ui) {}, WorldZoomTransition_WorldSpaceUIShrink, 1.25f);
			hoverIcon->IconImage->SetBrushFromMaterial(assetLoader()->M_HoverWarning);
			
			// Set the right image
			if (needHousing) {
				hoverIcon->IconImage->GetDynamicMaterial()->SetTextureParameterValue("IconImage", assetLoader()->WarningHouse);
				//hoverIcon->IconImage->SetBrush(*_brushes["HouseIcon_diffuse"]);
			}
			else if (needFood) {
				hoverIcon->IconImage->GetDynamicMaterial()->SetTextureParameterValue("IconImage", assetLoader()->WarningStarving);
				//hoverIcon->IconImage->SetBrush(*_brushes["StarvingIcon_diffuse"]);
			}
			else if (needHeat) {
				hoverIcon->IconImage->GetDynamicMaterial()->SetTextureParameterValue("IconImage", assetLoader()->WarningSnow);
				//hoverIcon->IconImage->SetBrush(*_brushes["SnowIcon_diffuse"]);
			}
			else if (needHealthcare) {
				hoverIcon->IconImage->GetDynamicMaterial()->SetTextureParameterValue("IconImage", assetLoader()->WarningHealthcare);
			}
			else if (needTools) {
				hoverIcon->IconImage->GetDynamicMaterial()->SetTextureParameterValue("IconImage", assetLoader()->WarningTools);
			}
			else if (needHappiness) {
				hoverIcon->IconImage->GetDynamicMaterial()->SetTextureParameterValue("IconImage", assetLoader()->UnhappyHoverIcon);
			}
			else if (idling) {
				//PUN_LOG("PUN: idling Style");
				hoverIcon->IconImage->SetBrush(*_brushes["Idling"]);
			}
		}
	}

	_unitHoverIcons.AfterAdd();
}

void UWorldSpaceUI::TickMap()
{
	if (InterfacesInvalid()) return;

	if (!PunSettings::IsOn("DisplayMapUI")) {
		_mapHoverIcons.AfterAdd();
		return;
	}

	IGameUIDataSource* data = dataSource();
	bool shouldShowMapIcon;
	if (dataSource()->isShowingProvinceOverlay()) {
		shouldShowMapIcon = data->ZoomDistanceAbove(WorldZoomTransition_Region4x4ToMap);
	} else {
		shouldShowMapIcon = data->ZoomDistanceAbove(WorldZoomAmountStage3);
	}
	
	if (shouldShowMapIcon &&
		PunSettings::IsOn("MapIcon"))
	{
		auto& simulation = data->simulation();
		auto& georesourceSys = simulation.georesourceSystem();
		auto& provinceSys = simulation.provinceSystem();
		
		const std::vector<int32>& georesourceRegions = georesourceSys.georesourceRegions();
		
		/*
		 * Georesource
		 */
		for (int32 regionId : georesourceRegions)
		{
			GeoresourceNode node = georesourceSys.georesourceNode(regionId);

			
			FVector displayLocation = data->DisplayLocation(node.centerTile.worldAtom2());
			displayLocation += FVector(0, 0, 30);

			//displayLocation = FVector(0, 0, 30); // TODO:

			UHoverIconWidgetBase* hoverIcon = _mapHoverIcons.GetHoverUI<UHoverIconWidgetBase>(regionId, UIEnum::HoverIcon, this, _worldWidgetParent, displayLocation, dataSource()->zoomDistance(),
				[&](UHoverIconWidgetBase* ui) 
			{
				ui->IconImage->SetBrushFromMaterial(assetLoader()->M_GeoresourceIcon);
				ui->IconSizeBox->SetHeightOverride(36);
				ui->IconSizeBox->SetWidthOverride(36);
				ui->IconImage->SetBrushFromMaterial(assetLoader()->M_GeoresourceIcon); // SetBrushFromMaterial must be here since doing it every tick causes leak
			}, 
			MapIconShrinkZoomAmount);

			SetGeoresourceImage(hoverIcon->IconImage, node.info().resourceEnum, assetLoader(), this);
		

			///*
			// * Other resources
			// */
			//case GeoresourceEnum::GiantMushroom: {
			//	stringstream ssTip;
			//	ssTip << "Weird giant mushrooms.";
			//	AddToolTip(hoverIcon->IconImage, ssTip.str());

			//	GeoresourceEnum georesourceEnum = assetLoader()->HasGeoIcon(node.georesourceEnum) ? node.georesourceEnum : GeoresourceEnum::Ruin;
			//	material->SetTextureParameterValue("ColorTexture", assetLoader()->GetGeoIcon(georesourceEnum));
			//	material->SetTextureParameterValue("DepthTexture", assetLoader()->GetGeoIconAlpha(georesourceEnum));
			//	break;
			//}
				
			//default:
			//	GeoresourceEnum georesourceEnum = assetLoader()->HasGeoIcon(node.georesourceEnum) ? node.georesourceEnum : GeoresourceEnum::Ruin;
			//	material->SetTextureParameterValue("ColorTexture", assetLoader()->GetGeoIcon(georesourceEnum));
			//	material->SetTextureParameterValue("DepthTexture", assetLoader()->GetGeoIconAlpha(georesourceEnum));
			//	break;
			//}
			
		}

		/*
		 * Townhall
		 */
		simulation.ExecuteOnPlayersAndAI([&](int32 playerId) 
		{
			const auto& townIds = simulation.playerOwned(playerId).townIds();

			for (int32 townId : townIds)
			{
				int32 townhallId = simulation.townManager(townId).townHallId;
				if (townhallId != -1)
				{
					Building& building = simulation.building(townhallId);

					PUN_CHECK(building.isEnum(CardEnum::Townhall));

					if (townhallUIActive) {
						TickTownhallInfo(townhallId, true);
					}
				}
			}
		});

	}
	
	_mapHoverIcons.AfterAdd();
}

void UWorldSpaceUI::TickPlacementInstructions()
{
	// Above Cursor Placement Text
	PlacementInfo placementInfo = inputSystemInterface()->PlacementBuildingInfo();
	FVector displayLocation = dataSource()->DisplayLocation(placementInfo.mouseOnTile.worldAtom2());

	int32 townId = simulation().tileOwnerTown(placementInfo.mouseOnTile);

	if (IsPointerOnUI() || 
		placementInfo.placementType == PlacementType::None) 
	{
		_aboveBuildingTextWidget->SetVisibility(false);
		_aboveBuildingTextSpawnTime = -1.0f;
		return;
	}

	// Just spawned, set the spawn time
	float time = UGameplayStatics::GetTimeSeconds(this);
	if (_aboveBuildingTextSpawnTime < 0) {
		_aboveBuildingTextSpawnTime = time;
	}

	// Don't show the widget right away so there is no flash
	if (time - _aboveBuildingTextSpawnTime < 0.1f) {
		_aboveBuildingTextWidget->SetVisibility(false);
		return;
	}

	_aboveBuildingTextWidget->SetWorldLocation(displayLocation + FVector(0, 0, 30));
	auto punBox = CastChecked<UAboveBuildingText>(_aboveBuildingTextWidget->GetUserWidgetObject())->PunBox;
	std::stringstream ss;

	auto getInstruction = [&](PlacementInstructionEnum instructionEnum) {
		return placementInfo.requiredInstructions[static_cast<int>(instructionEnum)];
	};
	auto needInstruction = [&](PlacementInstructionEnum instructionEnum) {
		return getInstruction(instructionEnum).shouldShow;
	};
	
	auto endInstruction = [&]() {
		bool shouldBeVisible = punBox->ChildrenCount() > 0;
		_aboveBuildingTextWidget->SetVisibility(shouldBeVisible, true);
		punBox->AfterAdd();
	};

	/*
	 * Single instruction...
	 */
	if (needInstruction(PlacementInstructionEnum::OutsideTerritory)) {
		punBox->AddRichTextCenter(LOCTEXT("OutsideTerritory_Instruct", "<Red>Must be inside our territory</>"));
	}
	// Drag
	else if (needInstruction(PlacementInstructionEnum::DragGather)) {
		punBox->AddRichTextCenter(LOCTEXT("DragGather1_Instruct", "Click and drag cursor"))->SetAutoWrapText(false);
		punBox->AddRichTextCenter(LOCTEXT("DragGather2_Instruct", "to specify the area to harvest"))->SetAutoWrapText(false);
	}
	else if (needInstruction(PlacementInstructionEnum::DragRoad1)) {
		punBox->AddRichTextCenter(LOCTEXT("DragRoad11_Instruct", "Click and drag cursor"))->SetAutoWrapText(false);
		punBox->AddRichTextCenter(LOCTEXT("DragRoad12_Instruct", "to build"))->SetAutoWrapText(false);
	}
	//else if (needInstruction(PlacementInstructionEnum::DragRoad2)) { // Not used yet???
	//	//punBox->AddRichText("Shift-click to repeat")->SetJustification(ETextJustify::Type::Center)->SetAutoWrapText(false);
	//	//punBox->AddRichText("to build");
	//}
	else if (needInstruction(PlacementInstructionEnum::DragRoadStone)) {
		int32 stoneNeeded = getInstruction(PlacementInstructionEnum::DragRoadStone).intVar1;
		int32 resourceCount = 0;
		if (townId != -1) {
			resourceCount = simulation().resourceCountTown(townId, ResourceEnum::Stone);
		}
		punBox->AddRichText(
			TextRed(to_string(stoneNeeded), resourceCount < stoneNeeded) + "<img id=\"Stone\"/>"
		);
	}
	else if (needInstruction(PlacementInstructionEnum::DragRoadIntercity)) {
		int32 goldNeeded = getInstruction(PlacementInstructionEnum::DragRoadIntercity).intVar1;
		punBox->AddRichText(
			TextRed(to_string(goldNeeded), simulation().money(playerId()) < goldNeeded) + "<img id=\"Coin\"/>"
		);
	}
	
	else if (needInstruction(PlacementInstructionEnum::DragStorageYard)) {
		punBox->AddRichTextParsed(getInstruction(PlacementInstructionEnum::DragStorageYard).instruction);
	}
	else if (needInstruction(PlacementInstructionEnum::DragFarm)) {
		punBox->AddRichTextParsed(getInstruction(PlacementInstructionEnum::DragFarm).instruction);
	}
	else if (needInstruction(PlacementInstructionEnum::Kidnap)) {
		punBox->AddRichTextParsed(getInstruction(PlacementInstructionEnum::Kidnap).instruction);
	}
	
	else if (needInstruction(PlacementInstructionEnum::DragDemolish)) {
		punBox->AddRichTextCenter(LOCTEXT("DragDemolish1_Instruct", "Click and drag cursor"))->SetAutoWrapText(false);
		punBox->AddRichTextCenter(LOCTEXT("DragDemolish2_Instruct", "to specify the area to demolish"))->SetAutoWrapText(false);
	}

	else if (needInstruction(PlacementInstructionEnum::DeliveryPointInstruction)) {
		punBox->AddRichTextCenter(LOCTEXT("DeliveryPointInstruction1_Instruct", "Choose storage/market"))->SetAutoWrapText(false);
		punBox->AddRichTextCenter(LOCTEXT("DeliveryPointInstruction2_Instruct", "to set the delivery point."))->SetAutoWrapText(false);
	}
	else if (needInstruction(PlacementInstructionEnum::DeliveryPointMustBeStorage)) {
		punBox->AddRichTextCenter(LOCTEXT("DeliveryPointMustBeStorage1_Instruct", "<Red>Delivery Point must be</>"));
		punBox->AddRichTextCenter(LOCTEXT("DeliveryPointMustBeStorage2_Instruct", "<Red>Storage Yard or Warehouse</>"));
	}
	else if (needInstruction(PlacementInstructionEnum::ShipperDeliveryPointShouldBeOutOfRadius)) {
		punBox->AddRichTextCenter(LOCTEXT("DeliveryPointShouldBeOutOfRadius1_Instruct", "<Red>Delivery Point must be</>"));
		punBox->AddRichTextCenter(LOCTEXT("DeliveryPointShouldBeOutOfRadius2_Instruct", "<Red>outside Shipping Depot's Radius</>"));
	}
	
	else if (needInstruction(PlacementInstructionEnum::Dock)) {
		punBox->AddRichTextCenter(LOCTEXT("Dock_Instruct", "<Red>Dock must face water</>"));
	}
	else if (needInstruction(PlacementInstructionEnum::NeedGeoresource)) {
		GeoresourceEnum geoEnum = static_cast<GeoresourceEnum>(getInstruction(PlacementInstructionEnum::NeedGeoresource).intVar1);
		punBox->AddRichTextCenter(FText::Format(LOCTEXT("NeedGeoresource_Instruct", "<Red>Need region with {0}</>"), GetGeoresourceInfo(geoEnum).name));
	}
	else if (needInstruction(PlacementInstructionEnum::MountainMine)) {
		punBox->AddRichTextCenter(LOCTEXT("MountainMine_Instruct", "<Red>Mine's back must face mountain</>"));
	}
	else if (needInstruction(PlacementInstructionEnum::ForeignBuilding)) {
		punBox->AddRichTextCenter(LOCTEXT("ForeignBuilding_Instruct", "<Red>Must be placed in foreign land</>"));
	}
	else if (needInstruction(PlacementInstructionEnum::MustBeNearRiver))
	{
		punBox->AddRichTextCenter(LOCTEXT("MustBeNearRiver_Instruct", "<Red>Must be built near a river</>"));
	}
	else if (needInstruction(PlacementInstructionEnum::LogisticsOffice))
	{
		punBox->AddRichTextCenter(LOCTEXT("LogisticsOffice1_Instruct", "Take resources from within radius"))->SetAutoWrapText(false);
		punBox->AddRichTextCenter(LOCTEXT("LogisticsOffice2_Instruct", "Ship them to faraway destination"))->SetAutoWrapText(false);
	}
	
	else if (needInstruction(PlacementInstructionEnum::FarmAndRanch)) {
		punBox->AddRichTextCenter(LOCTEXT("FarmAndRanch_Instruct", "<Red>Cannot be built on desert</>"));
	}
	else if (needInstruction(PlacementInstructionEnum::FarmNoValidSeedForRegion)) {
		punBox->AddRichTextCenter(LOCTEXT("FarmNoValidSeedForRegion_Instruct", "<Red>None of your seeds can be planted in this region.</>"));
	}
	else if (needInstruction(PlacementInstructionEnum::FireOnTownhall)) {
		int32 razeInfluence = getInstruction(PlacementInstructionEnum::FireOnTownhall).intVar1;
		punBox->AddRichTextCenter(FText::Format(LOCTEXT("FireOnTownhall_Instruct", "Requires {0}<img id=\"Influence\"/> to set townhall on fire"), TEXT_NUM(razeInfluence)));
	}
	else if (needInstruction(PlacementInstructionEnum::TownhallTarget)) {
		punBox->AddRichTextCenter(LOCTEXT("TownhallTarget_Instruct", "<Red>Must be used on target city's townhall</>"));
	}
	else if (needInstruction(PlacementInstructionEnum::YourBuildingTarget)) {
		punBox->AddRichTextCenter(LOCTEXT("YourBuildingTarget_Instruct", "<Red>Must be used on your building.</>"));
	}
	else if (needInstruction(PlacementInstructionEnum::NotThisBuildingTarget)) {
		punBox->AddRichTextCenter(LOCTEXT("NotThisBuildingTarget_Instruct", "<Red>Cannot be used on this building.</>"));
	}
	else if (needInstruction(PlacementInstructionEnum::ColonyNoGeoresource)) {
		punBox->AddRichTextCenter(LOCTEXT("ColonyNoGeoresource_Instruct", "<Red>Must be built on a province with georesource</>"));
	}
	else if (needInstruction(PlacementInstructionEnum::Colony)) {
		int32 provinceId = simulation().GetProvinceIdClean(placementInfo.mouseOnTile);
		PUN_CHECK(provinceId != -1);
		ResourceEnum resourceEnum = simulation().georesource(provinceId).info().resourceEnum;
		if (resourceEnum != ResourceEnum::None) {
			int32 resourceCount = ResourceOutpost::GetColonyResourceIncome(resourceEnum);
			punBox->AddIconPair(TEXT_NUMSIGNED(resourceCount), resourceEnum, FText::Format(INVTEXT(" {0}"), LOCTEXT("per round", "per round")));
		}
	}
	else if (needInstruction(PlacementInstructionEnum::Fort)) {
		punBox->AddRichTextCenter(LOCTEXT("Fort_Instruct", "Fort does not require citizens nearby to build."));
	}
// Colony
	else if (needInstruction(PlacementInstructionEnum::ColonyNeedsEmptyProvinces)) {
		punBox->AddRichTextCenter(LOCTEXT("NeedEmptyProvinces_Instruct", "<Red>Must be placed on unowned provinces.</>"));
	}
	else if (needInstruction(PlacementInstructionEnum::ColonyNeedsPopulation)) {
		punBox->AddRichTextCenter(LOCTEXT("ColonyNeedsPopulation_Instruct", "<Red>Require population of at least 50 in the capital.</>"));
	}
	else if (needInstruction(PlacementInstructionEnum::ColonyNextToIntercityRoad)) {
		punBox->AddRichTextCenter(LOCTEXT("MustBeNextToIntercityRoad_Instruct", "<Red>Must be placed next to intercity road connected to our Townhall.</>"));
	}
	else if (needInstruction(PlacementInstructionEnum::ColonyClaimCost)) {
		int32 claimCost = getInstruction(PlacementInstructionEnum::ColonyClaimCost).intVar1;
		int32 money = simulation().money(playerId());
		punBox->AddRichTextCenter(FText::Format(
			LOCTEXT("ClaimProvincesCost_Instruct", "Provinces Claim Cost <img id=\"Coin\"/>{0}"),
			TextRed(TEXT_NUM(claimCost), money < claimCost)
		));
	}
	

	bool hasPrimaryInstruction = false;
	for (int32 i = 0; i < static_cast<int>(PlacementInstructionEnum::Rotate); i++) {
		if (needInstruction(static_cast<PlacementInstructionEnum>(i))) {
			hasPrimaryInstruction = true;
			break;
		}
	}

	if (hasPrimaryInstruction) {
		punBox->AddSpacer(12);
	}

	/*
	 * Secondary instructions
	 */

	bool showResource = placementInfo.buildingEnum != CardEnum::None && !hasPrimaryInstruction;

	if (!hasPrimaryInstruction && IsBuildingCard(placementInfo.buildingEnum)) 
	{
		if (needInstruction(PlacementInstructionEnum::Rotate)) {
			punBox->AddRichTextCenter(LOCTEXT("Rotate_Instruct", "Press R to rotate"))->SetAutoWrapText(false);
			punBox->AddSpacer(12);

			showResource = false;
		}
		else if (needInstruction(PlacementInstructionEnum::Shift))
		{
			punBox->AddRichTextCenter(LOCTEXT("Shift1_Instruct", "Shift-click for"))->SetAutoWrapText(false);
			punBox->AddRichTextCenter(LOCTEXT("Shift2_Instruct", "multiple placement"))->SetAutoWrapText(false);
			punBox->AddSpacer(12);

			showResource = false;
		}
	}

	/*
	 * Building info...
	 */
	auto addEfficiencyText = [&](int32 efficiency) {
		punBox->AddRichTextCenter(TextRedOrange(FText::Format(LOCTEXT("PlaceInfo_Efficiency", "Efficiency: {0}%"), TEXT_NUM(efficiency)), efficiency, 80, 60));
	};

	if (IsHouse(placementInfo.buildingEnum) || 
		IsDecorativeBuilding(placementInfo.buildingEnum))
	{
		//int appealPercent = simulation.overlaySystem().appealPercent(placementInfo.mouseOnTile);

		int32 appealPercent = simulation().overlaySystem().GetAppealPercent(placementInfo.mouseOnTile);
		//ss << "Appeal: " << appealPercent << "%";
		punBox->AddRichTextCenter(TextRedOrange(FText::Format(LOCTEXT("PlaceInfo_Appeal", "Appeal: {0}%"), TEXT_NUM(appealPercent)), appealPercent, 80, 60));
		punBox->AddSpacer(12);

	}
	else if (placementInfo.buildingEnum == CardEnum::Fisher)
	{
		int32 efficiency = Fisher::FisherAreaEfficiency(placementInfo.mouseOnTile, false, WorldTile2::Invalid, &simulation());
		//ss << "Efficiency: " << efficiency << "%";
		addEfficiencyText(efficiency);
		punBox->AddSpacer(12);
	}
	else if (placementInfo.buildingEnum == CardEnum::Windmill)
	{
		int32 efficiency = Windmill::WindmillBaseEfficiency(townId, placementInfo.mouseOnTile, &simulation());
		//ss << "Efficiency: " << efficiency << "%";
		addEfficiencyText(efficiency);
		punBox->AddSpacer(12);
	}
	else if (placementInfo.buildingEnum == CardEnum::Beekeeper)
	{
		int32 efficiency = Beekeeper::BeekeeperBaseEfficiency(townId, placementInfo.mouseOnTile, &simulation());
		//ss << "Efficiency: " << efficiency << "%";
		addEfficiencyText(efficiency);
		punBox->AddSpacer(12);
	}
	else if (placementInfo.buildingEnum == CardEnum::FruitGatherer)
	{
		auto& treeSystem = simulation().treeSystem();
		
		TileArea area(placementInfo.mouseOnTile, GathererHut::Radius);
		int32 fruitTreeCount = 0;
		area.ExecuteOnArea_WorldTile2([&](WorldTile2 tile) {
			if (WorldTile2::Distance(placementInfo.mouseOnTile, tile) <= GathererHut::Radius) {
				if (treeSystem.tileInfo(tile.tileId()).IsFruitBearer()) {
					fruitTreeCount++;
				}
			}
		});
		// Less than 12 (=24*24 / 4 / 4 / 3), need to warn red there is too little fruit trees
		// Less than 18, need to warn orange
		//ss << "Fruit Tree Count: " << fruitTreeCount;
		punBox->AddRichTextCenter(TextRedOrange(
			FText::Format(LOCTEXT("PlaceInfo_Appeal", "Fruit Tree Count: {0}"), TEXT_NUM(fruitTreeCount)),
			fruitTreeCount, 30, 20
		));
		punBox->AddSpacer(12);
	}

	

	// Show building resource need...
	if (showResource)
	{
		std::vector<int32> constructionResources = GetBuildingInfo(placementInfo.buildingEnum).constructionResources;
		for (int32 i = 0; i < constructionResources.size(); i++) {
			if (constructionResources[i] > 0) {
				ResourceEnum resourceEnum = ConstructionResources[i];
				int32 neededCount = constructionResources[i];
				
				bool isRed = true;
				if (townId != -1) {
					isRed = simulation().resourceSystem(townId).resourceCountWithDrops(resourceEnum) < neededCount;
				}
				punBox->AddIconPair(FText(), resourceEnum, TEXT_NUM(neededCount), isRed);
			}
		}
	}

	endInstruction();
}


#undef LOCTEXT_NAMESPACE