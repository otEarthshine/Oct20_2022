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

	//if (zoomDistance < WorldZoomTransition_Tree)
	{
		for (int32 provinceId : sampleProvinceIds)
		{
			int32 provinceOwnerId = sim.provinceOwner(provinceId);
			if (provinceOwnerId != -1)
			{
				ProvinceClaimProgress claimProgress = sim.playerOwned(provinceOwnerId).GetDefendingClaimProgress(provinceId);
				if (claimProgress.isValid())
				{
					WorldTile2 provinceCenter = provinceSys.GetProvinceCenterTile(provinceId);
					FVector displayLocation = MapUtil::DisplayLocation(data->cameraAtom(), provinceCenter.worldAtom2(), 50.0f);
					URegionHoverUI* regionHoverUI = _regionHoverUIs.GetHoverUI<URegionHoverUI>(provinceId, UIEnum::RegionHoverUI, this, _worldWidgetParent, displayLocation, dataSource()->zoomDistance(),
						[&](URegionHoverUI* ui) {}
					);

					bool isAttacker = claimProgress.attackerPlayerId == playerId();

					std::stringstream ss;
					std::string textType = (isAttacker ? "<Large>" : "<LargeRed>");
					ss << textType << "Attacker: " << simulation().playerName(claimProgress.attackerPlayerId) << "</>\n";
					ss << textType << claimProgress.committedInfluences << "</><img id=\"Influence\"/>";
					SetText(regionHoverUI->ClaimingText, ss.str());
					regionHoverUI->AutoChoseText->SetVisibility(ESlateVisibility::Collapsed);

					float fraction = static_cast<float>(claimProgress.ticksElapsed) / BattleClaimTicks;
					regionHoverUI->ClockImage->GetDynamicMaterial()->SetScalarParameterValue("Fraction", fraction);
					regionHoverUI->ClockImage->GetDynamicMaterial()->SetScalarParameterValue("IsRed", !isAttacker);
					regionHoverUI->ClockBox->SetVisibility(ESlateVisibility::SelfHitTestInvisible);
				}
			}
		}
	}
	
	//std::vector<RegionClaimProgress> claimProgresses = playerOwned.armyRegionsClaimQueue();

	//for (int32 provinceId : sampleProvinceIds)
	//{
	//	// RegionHoverUI
	//	if (zoomDistance < WorldZoomTransition_Tree)
	//	{
	//		int32 claimIndex = -1;
	//		for (size_t i = 0; i < claimProgresses.size(); i++) {
	//			if (claimProgresses[i].provinceId == provinceId) {
	//				claimIndex = i;
	//				break;
	//			}
	//		}

	//		WorldTile2 provinceCenter = provinceSys.GetProvinceCenterTile(provinceId);
	//		
	//		if (claimIndex != -1)
	//		{
	//			FVector displayLocation = MapUtil::DisplayLocation(data->cameraAtom(), provinceCenter.worldAtom2(), 50.0f);
	//			URegionHoverUI* regionHoverUI = _regionHoverUIs.GetHoverUI<URegionHoverUI>(provinceId, UIEnum::RegionHoverUI, this, _worldWidgetParent, displayLocation, dataSource()->zoomDistance(),
	//				[&](URegionHoverUI* ui) {}
	//			);

	//			if (claimIndex == 0) {
	//				SetText(regionHoverUI->ClaimingText, "Claiming");
	//				regionHoverUI->AutoChoseText->SetVisibility(ESlateVisibility::Collapsed);

	//				float fraction = static_cast<float>(claimProgresses[claimIndex].claimValue100Inputted / 100) / playerOwned.GetBaseProvinceClaimPrice(provinceId);
	//				regionHoverUI->ClockImage->GetDynamicMaterial()->SetScalarParameterValue("Fraction", fraction);
	//				regionHoverUI->ClockBox->SetVisibility(ESlateVisibility::SelfHitTestInvisible);
	//			}
	//			else
	//			{
	//				SetText(regionHoverUI->ClaimingText, "Queued for Claiming (" + to_string(claimIndex) + ")");
	//				regionHoverUI->AutoChoseText->SetVisibility(ESlateVisibility::Collapsed);
	//				regionHoverUI->ClockBox->SetVisibility(ESlateVisibility::Collapsed);
	//			}
	//		}
	//		else if (claimProgresses.size() == 0 && 
	//				playerOwned.armyAutoClaimProgress().provinceId == provinceId)
	//		{
	//			RegionClaimProgress claimProgress = playerOwned.armyAutoClaimProgress();
	//			
	//			FVector displayLocation = MapUtil::DisplayLocation(data->cameraAtom(), provinceCenter.worldAtom2(), 50.0f);
	//			URegionHoverUI* regionHoverUI = _regionHoverUIs.GetHoverUI<URegionHoverUI>(provinceId, UIEnum::RegionHoverUI, this, _worldWidgetParent, displayLocation, dataSource()->zoomDistance(),
	//				[&](URegionHoverUI* ui) {}
	//			);
	//			
	//			SetText(regionHoverUI->ClaimingText, "Claiming");
	//			regionHoverUI->AutoChoseText->SetVisibility(ESlateVisibility::SelfHitTestInvisible);
	//			SetText(regionHoverUI->AutoChoseText, "(Auto)");

	//			float fraction = static_cast<float>(claimProgress.claimValue100Inputted / 100) / playerOwned.GetBaseProvinceClaimPrice(provinceId);
	//			regionHoverUI->ClockImage->GetDynamicMaterial()->SetScalarParameterValue("Fraction", fraction);
	//			regionHoverUI->ClockBox->SetVisibility(ESlateVisibility::SelfHitTestInvisible);
	//		}
	//		else
	//		{
	//			// Not being claimed, might show georesource
	//			//FVector displayLocation = MapUtil::DisplayLocation(data->cameraAtom(), curRegion.centerTile().worldAtom2(), 50.0f);
	//			//URegionHoverUI* regionHoverUI = _regionHoverUIs.GetHoverUI<URegionHoverUI>(sampleRegionId, UIEnum::RegionHoverUI, this, _worldWidgetParent, displayLocation, dataSource()->zoomDistance(),
	//			//	[&](URegionHoverUI* ui) {}
	//			//);
	//		}
	//	}
	//}

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
						hoverIcon->SetText("", "");
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
			hoverIcon->SetText("", "+" + to_string(Library::SciencePerHouse));

			bool alreadyHasLibrary = building.subclass<House>().GetScience100(ScienceEnum::Library) > 0;
			hoverIcon->SetTextColor(alreadyHasLibrary ? FLinearColor(0.38, 0.38, 0.38) : FLinearColor::White);
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
			hoverIcon->SetText("", "");
		}
		else if (isInOverlayRadiusHouse(OverlayType::Theatre, Theatre::MinHouseLvl, Theatre::Radius))
		{
			UIconTextPairWidget* hoverIcon = _iconTextHoverIcons.GetHoverUI<UIconTextPairWidget>(buildingId, UIEnum::HoverTextIconPair, this,
				_worldWidgetParent, GetBuildingTrueCenterDisplayLocation(buildingId), zoomDistance, [&](UIconTextPairWidget* ui) {});

			hoverIcon->SetImage(assetLoader()->SmileIcon);
			hoverIcon->SetText("", "");
		}

		
		/*
		 * Others
		 */
		else if (isInOverlayRadiusHouse(OverlayType::Bank, Bank::MinHouseLvl, Bank::Radius))
		{
			UIconTextPairWidget* hoverIcon = _iconTextHoverIcons.GetHoverUI<UIconTextPairWidget>(buildingId, UIEnum::HoverTextIconPair, this,
				_worldWidgetParent, GetBuildingTrueCenterDisplayLocation(buildingId), zoomDistance, [&](UIconTextPairWidget* ui) {});

			hoverIcon->SetImage(assetLoader()->CoinIcon);
			hoverIcon->SetText("", "+" + to_string(Bank::ProfitPerHouse));
		}
		// Bad Appeal
		else if (overlayType == OverlayType::BadAppeal && IsHumanHouse(building.buildingEnum()) && 
				WorldTile2::Distance(building.centerTile(), overlayTile) < BadAppealRadius)
		{
			UIconTextPairWidget* hoverIcon = _iconTextHoverIcons.GetHoverUI<UIconTextPairWidget>(buildingId, UIEnum::HoverTextIconPair, this,
				_worldWidgetParent, GetBuildingTrueCenterDisplayLocation(buildingId), zoomDistance, [&](UIconTextPairWidget* ui) {},
			WorldZoomTransition_WorldSpaceUIShrink, 1.5);

			hoverIcon->SetImage(assetLoader()->UnhappyIcon);
			hoverIcon->SetText("", "");
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
			hoverIcon->SetText("", "+10%");
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
					if (floatupInfo.text != "" && floatupInfo.text != "0" && floatupInfo.text != "+0") {
						iconTextPair3Lines->IconPair1->SetText(floatupInfo.text, "");

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
					if (floatupInfo.text2 != "" && floatupInfo.text2 != "0" && floatupInfo.text != "+0") {
						iconTextPair3Lines->IconPair2->SetText(floatupInfo.text2, "");

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
		switch(building.buildingEnum())
		{
		case CardEnum::StorageYard:
			buildingJobUI->SetVisibility(ESlateVisibility::Collapsed);
			return;
		default:
			break;
		}
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
				buildingJobUI->SetBuildingStatus(building);
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
			buildingJobUI->SetBuildingStatus(building);
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
				ui->CityNameText->SetText(FText::FromString(townhall->townFName()));
				ui->CityNameText->SetVisibility(ESlateVisibility::SelfHitTestInvisible);
				//ui->CityNameEditableText->SetVisibility(ESlateVisibility::Collapsed);
				AddToolTip(ui->Laborer, "Citizens without assigned workplaces become laborers. Laborers helps with hauling, gathering, and land clearing.");
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
		bool needFood = unitAI.needFood();
		bool needHeat = unitAI.needHeat();
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
	if (!data->ZoomDistanceBelow(WorldZoomAmountStage3) &&
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
			WorldRegion2 region(regionId);
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
			}, 
			MapIconShrinkZoomAmount);
			
			auto material = hoverIcon->IconImage->GetDynamicMaterial();
			auto setResourceInfo = [&](ResourceEnum resourceEnum, bool isOre) {
				material->SetTextureParameterValue("ColorTexture", assetLoader()->GetResourceIcon(resourceEnum));
				material->SetTextureParameterValue("DepthTexture", assetLoader()->GetResourceIconAlpha(resourceEnum));

				if (isOre) {
					switch(resourceEnum) {
						case ResourceEnum::Iron: resourceEnum = ResourceEnum::IronOre; break;
						case ResourceEnum::GoldBar: resourceEnum = ResourceEnum::GoldOre; break;
						case ResourceEnum::Gemstone: resourceEnum = ResourceEnum::Gemstone; break;
					}
					
					stringstream ssTip;
					ssTip << ResourceName(resourceEnum) << " Deposit in this region that can be mined.";
					AddToolTip(hoverIcon->IconImage, ssTip.str());
				}
				else {
					stringstream ssTip;
					ssTip << "This region is suitable for " << ResourceName(resourceEnum) << " Farming. ";
					AddToolTip(hoverIcon->IconImage, ssTip.str());
				}
			};

			switch(node.georesourceEnum)
			{
			case GeoresourceEnum::CoalOre: setResourceInfo(ResourceEnum::Coal, true); break;
			case GeoresourceEnum::IronOre: setResourceInfo(ResourceEnum::Iron, true); break;
			case GeoresourceEnum::GoldOre: setResourceInfo(ResourceEnum::GoldBar, true); break;
			case GeoresourceEnum::Gemstone: setResourceInfo(ResourceEnum::Gemstone, true); break;
				
			case GeoresourceEnum::CannabisFarm: setResourceInfo(ResourceEnum::Cannabis, false); break;
			case GeoresourceEnum::GrapeFarm: setResourceInfo(ResourceEnum::Grape, false); break;
			case GeoresourceEnum::CocoaFarm: setResourceInfo(ResourceEnum::Cocoa, false); break;

			case GeoresourceEnum::CottonFarm: setResourceInfo(ResourceEnum::Cotton, false); break;
			case GeoresourceEnum::DyeFarm: setResourceInfo(ResourceEnum::Dye, false); break;

			/*
			 * Other resources
			 */
			case GeoresourceEnum::GiantMushroom: {
				stringstream ssTip;
				ssTip << "Weird giant mushrooms.";
				AddToolTip(hoverIcon->IconImage, ssTip.str());

				GeoresourceEnum georesourceEnum = assetLoader()->HasGeoIcon(node.georesourceEnum) ? node.georesourceEnum : GeoresourceEnum::Ruin;
				material->SetTextureParameterValue("ColorTexture", assetLoader()->GetGeoIcon(georesourceEnum));
				material->SetTextureParameterValue("DepthTexture", assetLoader()->GetGeoIconAlpha(georesourceEnum));
				break;
			}
				
			default:
				GeoresourceEnum georesourceEnum = assetLoader()->HasGeoIcon(node.georesourceEnum) ? node.georesourceEnum : GeoresourceEnum::Ruin;
				material->SetTextureParameterValue("ColorTexture", assetLoader()->GetGeoIcon(georesourceEnum));
				material->SetTextureParameterValue("DepthTexture", assetLoader()->GetGeoIconAlpha(georesourceEnum));
				break;
			}
			
		}

		/*
		 * Townhall
		 */
		simulation.ExecuteOnPlayersAndAI([&](int32 playerId) {
			int32 townhallId = simulation.playerOwned(playerId).townHallId;
			if (townhallId != -1)
			{
				Building& building = simulation.building(townhallId);

				PUN_CHECK(building.isEnum(CardEnum::Townhall));
				
				if (townhallUIActive) {
					TickTownhallInfo(townhallId, true);
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
		punBox->AddRichText("<Red>Must be inside our territory</>")->SetJustification(ETextJustify::Type::Center);
	}
	// Drag
	else if (needInstruction(PlacementInstructionEnum::DragGather)) {
		punBox->AddRichText("Click and drag cursor")->SetJustification(ETextJustify::Type::Center)->SetAutoWrapText(false);
		punBox->AddRichText("to specify the area to harvest")->SetJustification(ETextJustify::Type::Center)->SetAutoWrapText(false);
	}
	else if (needInstruction(PlacementInstructionEnum::DragRoad1)) {
		punBox->AddRichText("Click and drag cursor")->SetJustification(ETextJustify::Type::Center)->SetAutoWrapText(false);
		punBox->AddRichText("to build")->SetJustification(ETextJustify::Type::Center)->SetAutoWrapText(false);
	}
	else if (needInstruction(PlacementInstructionEnum::DragRoad2)) {
		punBox->AddRichText("Shift-click to repeat")->SetJustification(ETextJustify::Type::Center)->SetAutoWrapText(false);
		punBox->AddRichText("to build");
	}
	else if (needInstruction(PlacementInstructionEnum::DragRoadStone)) {
		int32 stoneNeeded = getInstruction(PlacementInstructionEnum::DragRoadStone).intVar1;
		punBox->AddRichText(TextRed(to_string(stoneNeeded), simulation().resourceCount(playerId(), ResourceEnum::Stone) < stoneNeeded) + "<img id=\"Stone\"/>");
	}
	else if (needInstruction(PlacementInstructionEnum::DragRoadIntercity)) {
		int32 goldNeeded = getInstruction(PlacementInstructionEnum::DragRoadIntercity).intVar1;
		punBox->AddRichText(TextRed(to_string(goldNeeded), simulation().money(playerId()) < goldNeeded) + "<img id=\"Coin\"/>");
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
		punBox->AddRichText("Click and drag cursor")->SetJustification(ETextJustify::Type::Center)->SetAutoWrapText(false);
		punBox->AddRichText("to specify the area to demolish")->SetJustification(ETextJustify::Type::Center)->SetAutoWrapText(false);
	}
	
	else if (needInstruction(PlacementInstructionEnum::Dock)) {
		punBox->AddRichText("<Red>Dock must face water.</>")->SetJustification(ETextJustify::Type::Center);
	}
	else if (needInstruction(PlacementInstructionEnum::NeedGeoresource)) {
		GeoresourceEnum geoEnum = static_cast<GeoresourceEnum>(getInstruction(PlacementInstructionEnum::NeedGeoresource).intVar1);
		punBox->AddRichText("<Red>Need region with " + GetGeoresourceInfo(geoEnum).name + "</>")->SetJustification(ETextJustify::Type::Center);
	}
	else if (needInstruction(PlacementInstructionEnum::MountainMine)) {
		punBox->AddRichText("<Red>Mine's back must face mountain.</>")->SetJustification(ETextJustify::Type::Center);
	}
	else if (needInstruction(PlacementInstructionEnum::ForeignBuilding)) {
		punBox->AddRichText("<Red>Must be placed in foreign land.</>")->SetJustification(ETextJustify::Type::Center);
	}
	else if (needInstruction(PlacementInstructionEnum::ClayPit)) {
		punBox->AddRichText("<Red>Must be built near a river</>")->SetJustification(ETextJustify::Type::Center);
	}
	else if (needInstruction(PlacementInstructionEnum::FarmAndRanch)) {
		punBox->AddRichText("<Red>Cannot be built on desert</>")->SetJustification(ETextJustify::Type::Center);
	}
	else if (needInstruction(PlacementInstructionEnum::FarmNoValidSeedForRegion)) {
		punBox->AddRichText("<Red>None of your seeds can be planted in this region.</>")->SetJustification(ETextJustify::Type::Center);
	}
	else if (needInstruction(PlacementInstructionEnum::FireOnTownhall)) {
		int32 razeInfluence = getInstruction(PlacementInstructionEnum::FireOnTownhall).intVar1;
		punBox->AddRichText("Requires " + to_string(razeInfluence) + "<img id=\"Influence\"/> to set townhall on fire")->SetJustification(ETextJustify::Type::Center);
	}
	else if (needInstruction(PlacementInstructionEnum::TownhallTarget)) {
		punBox->AddRichText("<Red>Must be used on target city's townhall</>")->SetJustification(ETextJustify::Type::Center);
	}
	else if (needInstruction(PlacementInstructionEnum::YourBuildingTarget)) {
		punBox->AddRichText("<Red>Must be used on your building.</>")->SetJustification(ETextJustify::Type::Center);
	}
	else if (needInstruction(PlacementInstructionEnum::NotThisBuildingTarget)) {
		punBox->AddRichText("<Red>Cannot be used on this building.</>")->SetJustification(ETextJustify::Type::Center);
	}
	else if (needInstruction(PlacementInstructionEnum::ColonyNoGeoresource)) {
		punBox->AddRichText("<Red>Must be built on a province with georesource</>")->SetJustification(ETextJustify::Type::Center);
	}
	else if (needInstruction(PlacementInstructionEnum::Colony)) {
		int32 provinceId = simulation().GetProvinceIdClean(placementInfo.mouseOnTile);
		PUN_CHECK(provinceId != -1);
		ResourceEnum resourceEnum = simulation().georesource(provinceId).info().resourceEnum;
		if (resourceEnum != ResourceEnum::None) {
			int32 resourceCount = Colony::GetColonyResourceIncome(resourceEnum);
			punBox->AddIconPair("+" + to_string(resourceCount), resourceEnum, " per round");
		}
	}
	else if (needInstruction(PlacementInstructionEnum::Fort)) {
		punBox->AddRichText("Fort does not require citizens nearby to build.")->SetJustification(ETextJustify::Type::Center);
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
			punBox->AddRichText("Press R to rotate")
				->SetJustification(ETextJustify::Type::Center)
				->SetAutoWrapText(false);
			punBox->AddSpacer(12);

			showResource = false;
		}
		else if (needInstruction(PlacementInstructionEnum::Shift))
		{
			punBox->AddRichText("Shift-click for")->SetJustification(ETextJustify::Type::Center)->SetAutoWrapText(false);
			punBox->AddRichText("multiple placement")->SetJustification(ETextJustify::Type::Center)->SetAutoWrapText(false);
			punBox->AddSpacer(12);

			showResource = false;
		}
	}

	/*
	 * Building info...
	 */

	if (IsHouse(placementInfo.buildingEnum) || 
		IsDecorativeBuilding(placementInfo.buildingEnum))
	{
		//int appealPercent = simulation.overlaySystem().appealPercent(placementInfo.mouseOnTile);

		int32 appealPercent = simulation().overlaySystem().GetAppealPercent(placementInfo.mouseOnTile);
		ss << "Appeal: " << appealPercent << "%";
		punBox->AddRichText(TextRed(ss.str(), appealPercent < 60))->SetJustification(ETextJustify::Type::Center);
		punBox->AddSpacer(12);

	}
	else if (placementInfo.buildingEnum == CardEnum::Fisher)
	{
		int32 efficiency = Fisher::FisherAreaEfficiency(placementInfo.mouseOnTile, false, WorldTile2::Invalid, &simulation());
		ss << "Efficiency: " << efficiency << "%";
		punBox->AddRichText(TextRed(ss.str(), efficiency < 60))->SetJustification(ETextJustify::Type::Center);
		punBox->AddSpacer(12);
	}
	else if (placementInfo.buildingEnum == CardEnum::Windmill)
	{
		int32 efficiency = Windmill::WindmillBaseEfficiency(playerId(), placementInfo.mouseOnTile, &simulation());
		ss << "Efficiency: " << efficiency << "%";
		punBox->AddRichText(TextRed(ss.str(), efficiency < 60))->SetJustification(ETextJustify::Type::Center);
		punBox->AddSpacer(12);
	}
	else if (placementInfo.buildingEnum == CardEnum::Beekeeper)
	{
		int32 efficiency = Beekeeper::BeekeeperBaseEfficiency(playerId(), placementInfo.mouseOnTile, &simulation());
		ss << "Efficiency: " << efficiency << "%";
		punBox->AddRichText(TextRed(ss.str(), efficiency < 60))->SetJustification(ETextJustify::Type::Center);
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
				bool isRed = simulation().resourceCountWithDrops(playerId(), resourceEnum) < neededCount;
				punBox->AddIconPair("", resourceEnum, to_string(neededCount), isRed);
			}
		}
	}

	endInstruction();
}