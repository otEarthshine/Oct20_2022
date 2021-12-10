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
	_minorTownHoverInfos.Init();
	
	_regionHoverUIs.Init();
	_raidHoverIcons.Init();
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
	LEAN_PROFILING_UI(TickWorldSpaceUI);
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
				building.isEnum(CardEnum::Warehouse) ||
				building.isEnum(CardEnum::Granary) ||
				building.isEnum(CardEnum::IntercityLogisticsHub)||
				building.isEnum(CardEnum::IntercityLogisticsPort))
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

	auto& playerOwned = sim.playerOwned(playerId());
	auto& provinceSys = sim.provinceSystem();

	const std::vector<int32>& sampleProvinceIds = dataSource()->sampleProvinceIds();

	/*
	 * Province UI
	 */
	{
		LEAN_PROFILING_UI(TickWorldSpaceUI_Province);

		int32 provinceMeshIndex = 0;
		
		for (int32 provinceId : sampleProvinceIds)
		{
			int32 provinceTownId = sim.provinceOwnerTownSafe(provinceId);

			ProvinceClaimProgress claimProgress;
			if (IsValidMajorTown(provinceTownId))
			{
				TownHall* townhall = sim.GetTownhallPtr(provinceTownId);
				if (townhall && townhall->provinceId() != provinceId) { // Only show non-capital provinces
					claimProgress = sim.townManagerBase(provinceTownId)->GetDefendingClaimProgressDisplay(provinceId);
				}
			}

			WorldTile2 provinceCenter = provinceSys.GetProvinceCenterTile(provinceId);
			FVector displayLocation = MapUtil::DisplayLocation(data->cameraAtom(), provinceCenter.worldAtom2(), claimProgress.isValid() ? 0.0f : 50.0f);
			
			
			auto getRegionHoverUI = [&]() {
				return _regionHoverUIs.GetHoverUI<URegionHoverUI>(provinceId, UIEnum::RegionHoverUI, this, _worldWidgetParent, displayLocation, dataSource()->zoomDistance(),
					[&](URegionHoverUI* ui) 
					{
						ui->IconImage->SetBrushFromMaterial(assetLoader()->M_GeoresourceIcon); // SetBrushFromMaterial must be here since doing it every tick causes leak
					}, 
					WorldZoomTransition_Region4x4ToMap
				);
			};

			if (zoomDistance < WorldZoomTransition_Region4x4ToMap)
			{
				if (claimProgress.isValid())
				{
					URegionHoverUI* regionHoverUI = getRegionHoverUI();

					regionHoverUI->UpdateBattlefieldUI(provinceId, claimProgress);

					regionHoverUI->ProvinceOverlay->SetVisibility(ESlateVisibility::Collapsed);
					regionHoverUI->BattlefieldUI->SetVisibility(ESlateVisibility::SelfHitTestInvisible);


					/*
					 * Battle Flash
					 */
					UTerritoryMeshComponent* mesh = nullptr;
					if (provinceMeshIndex < _provinceMeshes.Num())
					{
						mesh = _provinceMeshes[provinceMeshIndex];
					}
					else
					{
						auto assetLoader = dataSource()->assetLoader();
						auto meshMaterial = assetLoader->M_TerritoryBattleHighlight;

						auto comp = NewObject<UTerritoryMeshComponent>(dataSource()->componentToAttach());
						comp->Rename(*(FString("TerritoryChunkBattleFlash") + FString::FromInt(_provinceMeshes.Num())));
						comp->AttachToComponent(dataSource()->componentToAttach(), FAttachmentTransformRules::KeepRelativeTransform, NAME_None);
						comp->SetCollisionEnabled(ECollisionEnabled::NoCollision);
						comp->SetGenerateOverlapEvents(false);
						comp->bAffectDistanceFieldLighting = false;
						comp->SetReceivesDecals(false);
						comp->SetCastShadow(false);
						comp->RegisterComponent();
						comp->SetTerritoryMaterial(meshMaterial, meshMaterial);
						_provinceMeshes.Add(comp);
						mesh = comp;
					}
					provinceMeshIndex++;

					if (mesh->provinceId != provinceId) {
						mesh->UpdateMesh(true, provinceId, -1, -1, false, &simulation(), 50);
					}

					FVector territoryDisplayLocation = dataSource()->DisplayLocation(provinceCenter.worldAtom2());

					mesh->SetWorldLocation(territoryDisplayLocation + FVector(0, 0, 1));
					mesh->SetVisibility(true);
				}
				else
				{
					if (dataSource()->GetOverlayType() == OverlayType::Raid)
					{

						int32 originTownId = sim.provinceOwnerTownSafe(provinceId);
						if (originTownId == -1)
						{
							//hoverIcon->SetPair(hoverIcon->IconPair1, LOCTEXT("Empty Province Not Raidable", "Empty Province\nNot Raidable"));
							//hoverIcon->SetPair(hoverIcon->IconPair2);
							//hoverIcon->SetPair(hoverIcon->IconPair3);
						}
						else if (sim.townPlayerId(originTownId) == playerId())
						{
							//hoverIcon->SetPair(hoverIcon->IconPair1, LOCTEXT("Our Province Not Raidable", "Our Province\nNot Raidable"));
							//hoverIcon->SetPair(hoverIcon->IconPair2);
							//hoverIcon->SetPair(hoverIcon->IconPair3);
						}
						else if (sim.IsValidMinorTown(originTownId) ||
							sim.IsBorderProvince(provinceId)) // Border Province Only
						{
							UIconTextPair2Lines* hoverIcon = _raidHoverIcons.GetHoverUI<UIconTextPair2Lines>(provinceId, UIEnum::HoverTextIconPair3Lines, this,
								_worldWidgetParent, displayLocation, dataSource()->zoomDistance(), [&](UIconTextPair2Lines* ui) {},
								WorldZoomTransition_Region4x4ToMap
								);

							int32 raidMoney100 = sim.GetProvinceRaidMoney100(provinceId);
							int32 raidInfluence100 = sim.GetProvinceRaidInfluence100(provinceId);

							hoverIcon->SetPair(hoverIcon->IconPair1, FText(), assetLoader()->CoinIcon, TEXT_100(raidMoney100));
							hoverIcon->SetPair(hoverIcon->IconPair2, FText(), assetLoader()->InfluenceIcon, TEXT_100(raidInfluence100));
							hoverIcon->SetPair(hoverIcon->IconPair3);
						}
					}
					else if (dataSource()->isShowingProvinceOverlay())
					{
						URegionHoverUI* regionHoverUI = getRegionHoverUI();
						regionHoverUI->UpdateProvinceOverlayInfo(provinceId);

						regionHoverUI->ProvinceOverlay->SetVisibility(ESlateVisibility::SelfHitTestInvisible);
						regionHoverUI->BattlefieldUI->SetVisibility(ESlateVisibility::Collapsed);
						//regionHoverUI->BattlefieldUI->provinceId = -1;
					}
				}

			}
		}

		//! Despawn the unused province Meshes
		for (int32 i = provinceMeshIndex; i < _provinceMeshes.Num(); i++) {
			_provinceMeshes[i]->SetVisibility(false);
			_provinceMeshes[i]->SetActive(false);
		}
	}


	{
		LeanProfiler leanProfilerOuter(LeanProfilerEnum::TickWorldSpaceUI_Building);

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

		OverlayType overlayType = data->GetOverlayType();
		WorldTile2 overlayTile = dataSource()->GetOverlayTile(); // Overlay tile is where the mouse is during Building Placement

		/*
		 * Go through Each building and display Hover Icons as needed
		 */
		for (int buildingId : buildingIdsToDisplay)
		{
			if (buildingId == -1) continue; // displayedBuilding list is from BuildingDisplaySystem, invalid objectId is possible

			Building& building = dataSource()->GetBuilding(buildingId);

			if (building.isEnum(CardEnum::Townhall))
			{
				if (townhallUIActive) 
				{
					LEAN_PROFILING_UI(TickWorldSpaceUI_Townhall);
					
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

			}
			else if (building.isEnum(CardEnum::MinorCity) ||
					building.isEnum(CardEnum::ResourceOutpost))
			{
				if (townhallUIActive)
				{
					LEAN_PROFILING_UI(TickWorldSpaceUI_Townhall);
					if (sim.townManagerBase(building.townId())->GetMinorCityLevel()) {
						TickMinorTownInfo(building.townId());
					}
				}
			}
			else {
				// Don't show jobUI at all beyond some zoom
				//  (Zoom distance... 455 to 541)
				if (zoomDistance < WorldZoomTransition_WorldSpaceUIHide) 
				{
					LEAN_PROFILING_UI(TickWorldSpaceUI_BldJob);
					
					TickJobUI(buildingId);
				}
				else {
					building.SetBuildingResourceUIDirty(true);
				}
			}

			LEAN_PROFILING_UI(TickWorldSpaceUI_BldOverlay);

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

			auto isInOverlayRadiusFoodBuilding = [&](OverlayType overlayTypeCurrent, int32 radius)
			{
				return overlayType == overlayTypeCurrent &&
					IsFoodBuilding(building.buildingEnum()) &&
					WorldTile2::Distance(building.centerTile(), overlayTile) < radius &&
					building.playerId() == playerId();
			};

			auto showHoverIcon = [&](UTexture2D* image, const FText& prefix, const FText& suffix)
			{
				UIconTextPairWidget* hoverIcon = _iconTextHoverIcons.GetHoverUI<UIconTextPairWidget>(buildingId, UIEnum::HoverTextIconPair, this,
					_worldWidgetParent, GetBuildingTrueCenterDisplayLocation(buildingId), zoomDistance, [&](UIconTextPairWidget* ui) {});

				hoverIcon->SetImage(image);
				hoverIcon->SetText(prefix, suffix);

				return hoverIcon;
			};

			/*
			 * Science
			 */

			if (isInOverlayRadiusHouse(OverlayType::Library, Library::MinHouseLvl, Library::Radius))
			{
				UIconTextPairWidget* hoverIcon = showHoverIcon(assetLoader()->ScienceIcon, INVTEXT("+"), INVTEXT(""));

				bool alreadyHasLibrary = false; ;
				int32 radiusBonus = building.GetRadiusBonus(CardEnum::Library, Library::Radius, [&](int32 bonus, Building& buildingScope) {
					return max(bonus, 1);
				});
				if (radiusBonus > 0) {
					alreadyHasLibrary = true;
				}

				hoverIcon->SetTextColor(alreadyHasLibrary ? FLinearColor(0.38, 0.38, 0.38, 0.5) : FLinearColor::White);
			}
			else if (isInOverlayRadiusHouse(OverlayType::School, School::MinHouseLvl, School::Radius))
			{
				UIconTextPairWidget* hoverIcon = showHoverIcon(assetLoader()->ScienceIcon, INVTEXT("+"), INVTEXT(""));

				bool alreadyHasSchool = false; ;
				int32 radiusBonus = building.GetRadiusBonus(CardEnum::School, School::Radius, [&](int32 bonus, Building& buildingScope) {
					return max(bonus, 1);
				});
				if (radiusBonus > 0) {
					alreadyHasSchool = true;
				}

				hoverIcon->SetTextColor(alreadyHasSchool ? FLinearColor(0.38, 0.38, 0.38) : FLinearColor::White);
			}
			/*
			 * Entertainment
			 */
			else if (isInOverlayRadiusHouse(OverlayType::Tavern, 1, Tavern::Radius))
			{
				showHoverIcon(assetLoader()->SmileIcon, FText(), FText());
			}
			else if (isInOverlayRadiusHouse(OverlayType::Theatre, Theatre::MinHouseLvl, Theatre::Radius))
			{
				showHoverIcon(assetLoader()->SmileIcon, FText(), FText());
			}
			//else if (isInOverlayRadiusHouse(OverlayType::Zoo, 1, Zoo::Radius))
			//{
			//	showHoverIcon(assetLoader()->SmileIcon, FText(), FText());
			//}
			//else if (isInOverlayRadiusHouse(OverlayType::Museum, 1, Museum::Radius))
			//{
			//	showHoverIcon(assetLoader()->SmileIcon, FText(), FText());
			//}


			/*
			 * Others
			 */
			else if (isInOverlayRadiusHouse(OverlayType::Bank, Bank::MinHouseLvl, Bank::Radius))
			{
				showHoverIcon(assetLoader()->CoinIcon, INVTEXT("+"), FText());
			}
			// Bad Appeal
			else if (overlayType == OverlayType::BadAppeal &&
				(IsHumanHouse(building.buildingEnum()) || IsFunServiceBuilding(building.buildingEnum())) &&
				WorldTile2::Distance(building.centerTile(), overlayTile) < BadAppealRadius)
			{
				showHoverIcon(assetLoader()->UnhappyIcon, FText(), FText());
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

			else if (isInOverlayRadiusFoodBuilding(OverlayType::Granary, Granary::Radius))
			{
				UIconTextPairWidget* hoverIcon = _iconTextHoverIcons.GetHoverUI<UIconTextPairWidget>(buildingId, UIEnum::HoverTextIconPair, this,
					_worldWidgetParent, GetBuildingTrueCenterDisplayLocation(buildingId), zoomDistance, [&](UIconTextPairWidget* ui) {});

				//hoverIcon->SetImage(assetLoader()->ScienceIcon);
				hoverIcon->IconImage->SetVisibility(ESlateVisibility::Collapsed);
				hoverIcon->SetText(FText(), INVTEXT("+25%"));
				hoverIcon->SetTextColor(FLinearColor::White);
			}


		}

		//! Remove unused UIs
		_buildingJobUIs.AfterAdd();
		_townhallHoverInfos.AfterAdd();
		_minorTownHoverInfos.AfterAdd();
		
		_regionHoverUIs.AfterAdd();
		_raidHoverIcons.AfterAdd();

		_buildingHoverIcons.AfterAdd();

		_iconTextHoverIcons.AfterAdd();

	} // End TickWorldSpaceUI_Building

	

	_floatupUIs.Tick();

	/*
	 * FloatupInfo
	 */
	if (zoomDistance < WorldZoomTransition_WorldSpaceUIHide)
	{
		LEAN_PROFILING_UI(TickWorldSpaceUI_BldFloatup);
		
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
					floatupInfo.floatupEnum == FloatupEnum::GainInfluence_With0 ||
					floatupInfo.floatupEnum == FloatupEnum::GainResource)
				{
					auto assetLoader = dataSource()->assetLoader();

					UIconTextPair2Lines* iconTextPair3Lines = _floatupUIs.AddHoverUI<UIconTextPair2Lines>(UIEnum::HoverTextIconPair3Lines, worldAtom);


					auto showFloatup = [&](FText text)
					{
						if (text.IsEmpty()) return false;
						
						if (floatupInfo.floatupEnum != FloatupEnum::GainInfluence_With0)
						{
							if (text.ToString() == TEXT("0") ||
								text.ToString() == TEXT("+0")) {
								return false;
							}
						}
						return true;
					};
					
					if (showFloatup(floatupInfo.text))
					{
						iconTextPair3Lines->IconPair1->SetText(floatupInfo.text, FText());

						if (floatupInfo.floatupEnum == FloatupEnum::GainMoney) {
							iconTextPair3Lines->IconPair1->SetImage(assetLoader->CoinIcon);
						} else if (floatupInfo.floatupEnum == FloatupEnum::GainScience) {
							iconTextPair3Lines->IconPair1->SetImage(assetLoader->ScienceIcon);
						}
						else if (floatupInfo.floatupEnum == FloatupEnum::GainInfluence ||
								floatupInfo.floatupEnum == FloatupEnum::GainInfluence_With0) {
							iconTextPair3Lines->IconPair1->SetImage(assetLoader->InfluenceIcon);
						} else {
							iconTextPair3Lines->IconPair1->SetImage(floatupInfo.resourceEnum, assetLoader);
						}
						iconTextPair3Lines->IconPair1->SetVisibility(ESlateVisibility::SelfHitTestInvisible);
					}
					else {
						iconTextPair3Lines->IconPair1->SetVisibility(ESlateVisibility::Collapsed);
					}
					if (showFloatup(floatupInfo.text2))
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


	auto setForeignLogo = [&](UBuildingJobUI* buildingJobUI)
	{
		int32 foreignBuilderId = building.foreignBuilderId();
		if (foreignBuilderId != -1)
		{
			buildingJobUI->ForeignLogo->SetVisibility(ESlateVisibility::Visible);

			FPlayerInfo playerInfo = dataSource()->playerInfo(foreignBuilderId);

			UMaterialInstanceDynamic* material = buildingJobUI->ForeignLogo->GetDynamicMaterial();
			material->SetVectorParameterValue("ColorBackground", playerInfo.logoColorBackground);
			material->SetVectorParameterValue("ColorForeground", playerInfo.logoColorForeground);
			material->SetTextureParameterValue("Logo", assetLoader()->GetPlayerLogo(playerInfo.logoIndex));

			AddToolTip(buildingJobUI->ForeignLogo, FText::Format(
				LOCTEXT("ForeignLogo_Tip", "This building was built by {0}."),
				simulation().playerNameT(foreignBuilderId)
			));

			bool shouldShowForeignBuildingAllow = building.playerId() == playerId() &&
				!building.isForeignBuildingApproved();

			buildingJobUI->ForeignAllowBox->SetVisibility(shouldShowForeignBuildingAllow ? ESlateVisibility::SelfHitTestInvisible : ESlateVisibility::Collapsed);
		}
		else {
			buildingJobUI->ForeignLogo->SetVisibility(ESlateVisibility::Collapsed);
			buildingJobUI->ForeignAllowBox->SetVisibility(ESlateVisibility::Collapsed);
		}
	};
	

	FLinearColor brown(1, .75, .5);

	bool isTileBld = IsRoad(building.buildingEnum()) || building.isEnum(CardEnum::Fence);
	if (isTileBld)
	{
		LEAN_PROFILING_UI(TickWorldSpaceUI_BldJobTile);
		
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

		setForeignLogo(buildingJobUI);
		
		return;
	}

	int32 jobUIHeight = 25;
	if (building.isEnum(CardEnum::Windmill)) {
		jobUIHeight = 50;
	}

	UBuildingJobUI* buildingJobUI = GetJobUI(buildingId, jobUIHeight);

	DescriptionUIState uiState = simulation().descriptionUIState();
	_uiStateDirty = (uiState != _lastUIState);
	_lastUIState = uiState;

	//bool shouldUpdateUI = uiStateDirty || buildingJobUI->justInitializedUI || building.isBuildingUIDirty();
	//if (!shouldUpdateUI) {
	//	return;
	//}
	//building.SetBuildingUIDirty(false);
	

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

	buildingJobUI->LargeWhiteText->SetVisibility(ESlateVisibility::Collapsed);
	buildingJobUI->MediumGrayText->SetVisibility(ESlateVisibility::Collapsed);

	
	setForeignLogo(buildingJobUI);
	

	// Under construction
	if (!building.isConstructed())
	{
		LEAN_PROFILING_UI(TickWorldSpaceUI_BldJobUC);
		
		if (building.shouldDisplayConstructionUI())
		{
			auto showAlwaysOnPart = [&]()
			{
				buildingJobUI->SetShowBar(true);
				buildingJobUI->SetBarFraction(building.constructionFraction());

				if (building.foreignBuilderId() == -1)
				{
					std::vector<int32> constructionResourceCounts;
					std::vector<int32> constructionResourceRequired = building.GetConstructionResourceCost();
					
					for (size_t i = 0; i < constructionResourceRequired.size(); i++) {
						constructionResourceCounts.push_back(0);
						if (constructionResourceRequired[i] > 0) {
							constructionResourceCounts[i] = building.resourceCountSafe(ConstructionResources[i]);
						}
					}

					buildingJobUI->SetConstructionResource(constructionResourceCounts, building);
				}
				else {
					BoxAfterAdd(buildingJobUI->ResourceCompletionIconBox, 0);
				}

				buildingJobUI->SetHoverWarning(building);
			};

			// Full UI
			if (jobUIState == JobUIState::Job)
			{
				bool canManipulateOccupant = (playerId() == building.playerId()) && building.maxOccupants() > 0;
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
		}
		else {
			buildingJobUI->SetVisibility(ESlateVisibility::Collapsed);
		}
		return;
	}
	
	// After construction is done, shouldn't be any ResourceCompletion circle left
	if (building.justFinishedConstructionJobUIDirty()) {
		building.SetJustFinishedConstructionJobUIDirty(false);
		buildingJobUI->ClearResourceCompletionBox();
	}

	//! Houses
	if (IsHouse(building.buildingEnum()))
	{
		if (IsHumanHouse(building.buildingEnum()))
		{
			LEAN_PROFILING_UI(TickWorldSpaceUI_BldJobHouse);
			
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

			// Spy
			if (house.spyPlayerId() == playerId())
			{
				buildingJobUI->LargeWhiteText->SetVisibility(ESlateVisibility::Visible);
				buildingJobUI->MediumGrayText->SetVisibility(ESlateVisibility::Visible);

				buildingJobUI->LargeWhiteText->SetText(LOCTEXT("Spy Nest", "Spy Nest"));

				if (simulation().influence(house.playerId()) > 0)
				{
					buildingJobUI->MediumGrayText->SetText(FText::Format(
						LOCTEXT("+{0}<img id=\"Influence\"/> per round", "+{0}<img id=\"Influence\"/> per round"),
						TEXT_NUM(simulation().GetSpyNestInfluenceGainPerRound(house.spyPlayerId(), house.townId()))
					));
				}
				else {
					buildingJobUI->MediumGrayText->SetText(LOCTEXT("Target has 0 influence", "Target has 0<img id=\"Influence\"/>"));
				}
			}
			
			return;
		}
		
		// Don't show animal houses
		//buildingJobUI->SetStars(0);
		return;
	}

	//! DiplomaticBuilding
	if (IsForeignOnlyBuilding(building.buildingEnum()))
	{
		if (building.foreignBuilderId() == playerId() ||
			building.playerId() == playerId())
		{
			buildingJobUI->MediumGrayText->SetVisibility(ESlateVisibility::Visible);

			int32 influenceIncome100 = building.subclass<DiplomaticBuilding>().influenceIncome100(playerId());

			buildingJobUI->MediumGrayText->SetText(FText::Format(
				LOCTEXT("+{0}<img id=\"Influence\"/> per round", "+{0}<img id=\"Influence\"/> per round"),
				TEXT_NUM(influenceIncome100 / 100)
			));
		}

		return;
	}

	//! Fort
	if (building.isEnum(CardEnum::Fort))
	{
		if (building.playerId() == playerId())
		{
			buildingJobUI->MediumGrayText->SetVisibility(ESlateVisibility::Visible);

			// Show provinces protected, and income increase
			int32 protectedProvinces = 0;
			int32 protectionIncome100 = 0;
			const std::vector<int32>& provinceIds = simulation().townManager(building.playerId()).provincesClaimed();
			for (int32 provinceId : provinceIds) {
				if (simulation().provinceInfoSystem().provinceOwnerInfo(provinceId).isSafe) {
					protectedProvinces++;
					protectionIncome100 += simulation().GetProvinceIncome100(provinceId) * 2 / 3;
				}
			}

			buildingJobUI->MediumGrayText->SetText(FText::Format(
				LOCTEXT("Fort Hover Text", "Protected Provinces: {0}/{1}\nProtection Income: +{2}<img id=\"Coin\"/>"),
				TEXT_NUM(protectedProvinces),
				TEXT_NUM(provinceIds.size()),
				TEXT_100(protectionIncome100)
			));
		}
		else if (!simulation().townManager(building.townId()).GetDefendingClaimProgress(building.provinceId()).isValid()) // no battle here
		{
			buildingJobUI->SetHoverButton(LOCTEXT("Destroy Fort", "Destroy Fort"), UBuildingJobUI::GenericButtonEnum::RazeFort);
		}

		return;
	}
	
	

	LeanProfiler leanProfilerOuter(LeanProfilerEnum::TickWorldSpaceUI_BldJobWork);

	// Non-house finished buildings beyond this
	bool showOccupants = building.maxOccupants() > 0;

	// Processor/Trader ... show progress bar
	bool showJobUI = IsProducerProcessor(building.buildingEnum()) ||
						IsSpecialProducer(building.buildingEnum()) ||
						IsTradingPostLike(building.buildingEnum()) ||
						building.isEnum(CardEnum::TradingCompany) ||
						building.isEnum(CardEnum::SpyCenter) ||
						building.isEnum(CardEnum::Zoo) ||
						building.isEnum(CardEnum::Museum) ||
						building.isEnum(CardEnum::CardCombiner) ||
						building.isEnum(CardEnum::Caravansary) ||
						building.isEnum(CardEnum::Hotel) ||
						//IsBarrack(building.buildingEnum()) ||

						building.isEnum(CardEnum::Farm) ||
						building.isEnum(CardEnum::HuntingLodge) ||
						building.isEnum(CardEnum::FruitGatherer) ||
						building.isEnum(CardEnum::ShippingDepot) ||
						building.isEnum(CardEnum::IntercityLogisticsHub) ||
						building.isEnum(CardEnum::IntercityLogisticsPort) ||
						IsStorage(building.buildingEnum()) || // TODO: may be this is for all buildings??

						building.isEnum(CardEnum::JobManagementBureau) ||
						building.isEnum(CardEnum::StatisticsBureau);

	// Every buildings can be boosted
	buildingJobUI->SetSpeedBoost(building);
	
	if (jobUIState == JobUIState::Job)
	{
		if (showOccupants || showJobUI)
		{
			bool canManipulateOccupant = showOccupants && playerId() == building.playerId();
			buildingJobUI->SetShowHumanSlots(showOccupants, canManipulateOccupant);
			buildingJobUI->SetShowBar(false);

			if (showOccupants) {
				buildingJobUI->SetSlots(building.occupantCount(), building.allowedOccupants(), building.maxOccupants(), FLinearColor::White);
			} else {
				buildingJobUI->SetSlots(0, 0, 0, FLinearColor::White);
			}

			if (showJobUI) {
				buildingJobUI->SetBuildingStatus(building, jobUIState);
				buildingJobUI->SetHoverWarning(building);
			}

			return;
		}
	}
	else
	{
		// Always show the progress
		buildingJobUI->SetShowHumanSlots(false, false);
		buildingJobUI->SetShowBar(false);

		if (showJobUI) {
			buildingJobUI->SetBuildingStatus(building, jobUIState);
			buildingJobUI->SetHoverWarning(building);
		}

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

		UTownhallHoverInfo* townhallInfo = _townhallHoverInfos.GetHoverUI<UTownhallHoverInfo>(townhall->townId(), UIEnum::HoverTownhall, this, _worldWidgetParent, GetBuildingTrueCenterDisplayLocation(buildingId), dataSource()->zoomDistance(),
			[&](UTownhallHoverInfo* ui) {
				ui->CityNameText->SetText(townhall->townNameT());
				ui->CityNameText->SetVisibility(ESlateVisibility::SelfHitTestInvisible);
				//ui->CityNameEditableText->SetVisibility(ESlateVisibility::Collapsed);
				AddToolTip(ui->Laborer, 
					LOCTEXT("TownhallLaborerUI_Tip", "Citizens without assigned workplaces become laborers. Laborers helps with hauling, gathering, and land clearing.")
				);
				ui->PunInit(townhall->townId());
			}
		);

		// Do not accept input during placement
		//bool isPlacing = inputSystemInterface()->placementState() != PlacementType::None;
		//townhallInfo->SetVisibility(isPlacing ? ESlateVisibility::Collapsed : ESlateVisibility::Visible);

		townhallInfo->UpdateTownhallHoverInfo(isMini);
	}
}

void UWorldSpaceUI::TickMinorTownInfo(int32 townId, bool isMini)
{
	if (InterfacesInvalid()) return;

	auto& sim = dataSource()->simulation();
	TownManagerBase* townManagerBase = sim.townManagerBase(townId);

	UMinorTownWorldUI* minorTownInfo = _minorTownHoverInfos.GetHoverUI<UMinorTownWorldUI>(townId, UIEnum::WG_MinorTownWorldUI, this, _worldWidgetParent, 
		GetBuildingTrueCenterDisplayLocation(townManagerBase->townhallId), dataSource()->zoomDistance(),
		[&](UMinorTownWorldUI* ui) {}
	);

	minorTownInfo->uiTownId = townId;
	minorTownInfo->UpdateMinorTownUI(isMini);
}

void UWorldSpaceUI::TickUnits()
{
	
	if (InterfacesInvalid()) return;

	IGameUIDataSource* data = dataSource();
	auto& sim = data->simulation();

	std::unordered_map<int32, std::vector<int32>> townIdToUnitIdsToDisplay;

	
	const std::vector<int32>& sampleRegionIds = data->sampleRegionIds();

	auto& unitLists = sim.unitSystem().unitSubregionLists();

	{
		LEAN_PROFILING_UI(TickWorldSpaceUI_Unit);
		
		// TODO: don't do accumulation and use ExecuteRegion directly??
		for (int32_t sampleRegionId : sampleRegionIds) {
			unitLists.ExecuteRegion(WorldRegion2(sampleRegionId), [&](int32_t unitId) {
				UnitStateAI& unitAI = data->GetUnitStateAI(unitId);
				int32 townId = unitAI.townId();
				if (IsValidMajorTown(townId)) {
					townIdToUnitIdsToDisplay[townId].push_back(unitId);
				}
			});
		}
	}

	int32 countLeft = 100; // Don't show more than 100 hover icons

	if (!PunSettings::IsOn("SuppressHoverIcon"))
	{
		LEAN_PROFILING_UI(TickWorldSpaceUI_Unit2);

		for (auto it : townIdToUnitIdsToDisplay)
		{
			auto& resourceSys = sim.resourceSystem(it.first);
			int32 population = sim.populationTown(it.first);

			// Already have enough food/medicine/tools in storage? Just don't show the warning...
			bool hasLotsOfFood = resourceSys.foodCount() > std::min(1000, population);

			auto& townManager = sim.townManager(it.first);
			bool hasLotsOfFuel = false;
			if (townManager.GetHouseResourceAllow(ResourceEnum::Coal) &&
				resourceSys.resourceCount(ResourceEnum::Coal) > std::min(500, population / 2)) 
			{
				hasLotsOfFuel = true;
			}
			if (townManager.GetHouseResourceAllow(ResourceEnum::Wood) &&
				resourceSys.resourceCount(ResourceEnum::Wood) > std::min(500, population / 2)) {
				hasLotsOfFuel = true;
			}
			//bool hasLotsOfFuel = resourceSys.fuelCount() > std::min(500, population / 2);
			
			bool hasLotsOfMedicine = resourceSys.resourceCountWithPop(ResourceEnum::Medicine) + resourceSys.resourceCountWithPop(ResourceEnum::Herb) > std::min(300, population / 2);
			bool hasLotsOfTools = resourceSys.resourceCountWithPop(ResourceEnum::SteelTools) + resourceSys.resourceCountWithPop(ResourceEnum::StoneTools) > std::min(300, population / 4);


			const std::vector<int32>& unitIdsToDisplay = it.second;
			for (int unitId : unitIdsToDisplay)
			{
				if (!data->IsUnitValid(unitId)) {
					continue;
				}

				UnitStateAI& unitAI = data->GetUnitStateAI(unitId);

				if (unitAI.unitEnum() != UnitEnum::Human) {
					continue;
				}
				if (unitAI.animationEnum() == UnitAnimationEnum::Invisible) {
					continue;
				}
				HumanStateAI& human = unitAI.subclass<HumanStateAI>();

				std::vector<bool> warningToShow(static_cast<int32>(UAssetLoaderComponent::HoverWarningEnum::Count), false);

				warningToShow[static_cast<int>(UAssetLoaderComponent::HoverWarningEnum::Housing)] = unitAI.needHouse();
				warningToShow[static_cast<int>(UAssetLoaderComponent::HoverWarningEnum::Starving)] = unitAI.showNeedFood() && !hasLotsOfFood;
				warningToShow[static_cast<int>(UAssetLoaderComponent::HoverWarningEnum::Freezing)] = unitAI.showNeedHeat() && !hasLotsOfFuel;
				if (human.needHappiness()) {
					if (human.happinessOverall() < human.happinessLeaveTownThreshold()) {
						warningToShow[static_cast<int>(UAssetLoaderComponent::HoverWarningEnum::UnhappyRed)] = true;
					} else {
						warningToShow[static_cast<int>(UAssetLoaderComponent::HoverWarningEnum::UnhappyOrange)] = true;
					}
				}
				warningToShow[static_cast<int>(UAssetLoaderComponent::HoverWarningEnum::Sick)] = human.needHealthcare() && !hasLotsOfMedicine;
				warningToShow[static_cast<int>(UAssetLoaderComponent::HoverWarningEnum::Tools)] = human.needTools() && !hasLotsOfTools;

				// Foreign Caravan
				if (human.playerId() != playerId() &&
					human.workplace() && 
					human.workplace()->buildingEnum() == CardEnum::Caravansary) 
				{
					warningToShow[static_cast<int>(UAssetLoaderComponent::HoverWarningEnum::Logo)] = true;
				}
				
				// Don't forget to add this in DespawnUnusedUIs too... ???

			
				for (int32 i = 0; i < warningToShow.size(); i++)
				{
					if (countLeft-- <= 0) {
						break;
					}

					if (warningToShow[i])
					{
						FVector displayLocation = data->GetUnitDisplayLocation(unitId, inputSystemInterface()->cameraAtom());
						displayLocation += FVector(0, 0, 25);
						
						UHoverIconWidgetBase* hoverIcon = _unitHoverIcons.GetHoverUI<UHoverIconWidgetBase>(unitId, UIEnum::HoverIcon, this, _worldWidgetParent, displayLocation, dataSource()->zoomDistance(),
							[&](UHoverIconWidgetBase* ui) {}, WorldZoomTransition_WorldSpaceUIShrink, 1.25f);

						// Special case: Logo
						if (static_cast<UAssetLoaderComponent::HoverWarningEnum>(i) == UAssetLoaderComponent::HoverWarningEnum::Logo) {
							FPlayerInfo playerInfo = dataSource()->playerInfo(human.playerId());
							hoverIcon->IconImage->SetBrushFromMaterial(assetLoader()->M_LogoHover);
							hoverIcon->IconImage->GetDynamicMaterial()->SetVectorParameterValue("ColorBackground", playerInfo.logoColorBackground);
							hoverIcon->IconImage->GetDynamicMaterial()->SetVectorParameterValue("ColorForeground", playerInfo.logoColorForeground);
							hoverIcon->IconImage->GetDynamicMaterial()->SetTextureParameterValue("Logo", assetLoader()->GetPlayerLogo(playerInfo.logoIndex));
						}
						else {
							hoverIcon->IconImage->SetBrushFromMaterial(assetLoader()->GetHoverWarningMaterial(static_cast<UAssetLoaderComponent::HoverWarningEnum>(i)));
						}
						break;
					}

					//// Set the right image
					//if (needHousing) {
					//	setMaterial(UAssetLoaderComponent::HoverWarningEnum::Housing);
					//}
					//else if (needFood) {
					//	setMaterial(UAssetLoaderComponent::HoverWarningEnum::Starving);
					//}
					//else if (needHeat) {
					//	setMaterial(UAssetLoaderComponent::HoverWarningEnum::Freezing);
					//}
					//else if (needHealthcare) {
					//	setMaterial(UAssetLoaderComponent::HoverWarningEnum::Sick);
					//}
					//else if (needTools) {
					//	setMaterial(UAssetLoaderComponent::HoverWarningEnum::Tools);
					//}
					//else if (needHappiness) {
					//	if (human.happinessOverall() < human.happinessLeaveTownThreshold()) {
					//		setMaterial(UAssetLoaderComponent::HoverWarningEnum::UnhappyRed);
					//	}
					//	else {
					//		setMaterial(UAssetLoaderComponent::HoverWarningEnum::UnhappyOrange);
					//	}
					//}
				}
				
			}

		}

	}

	_unitHoverIcons.AfterAdd();
}

void UWorldSpaceUI::TickMap()
{
	LEAN_PROFILING_UI(TickWorldSpaceUI_Map);
	
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

			// Oil Invisible before research
			if (node.georesourceEnum == GeoresourceEnum::Oil) {
				if (!simulation.IsResearched(playerId(), TechEnum::Petroleum)) {
					if (!PunSettings::IsOn("ShowAllResourceNodes")) {
						continue;
					}
				}
			}
			
			FVector displayLocation = data->DisplayLocation(node.centerTile.worldAtom2());
			//displayLocation += FVector(0, 0, 30);

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
				int32 townhallId = simulation.townManager(townId).townhallId;
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

		/*
		 * Minor Towns
		 */
		simulation.ExecuteOnMinorTowns([&](int32 townId) {
			TickMinorTownInfo(townId, true);
		});



		/*
		 * Battle field
		 */
		if (dataSource()->ZoomDistanceAbove(WorldZoomTransition_Region4x4ToMap))
		{
			std::vector<ProvinceClaimProgress> battles = simulation.GetAllBattles();

			for (const ProvinceClaimProgress& battle : battles)
			{
				int32 provinceTownId = simulation.provinceOwnerTown_Major(battle.provinceId);
				if (provinceTownId != -1)
				{
					TownHall* townhall = simulation.GetTownhallPtr(provinceTownId);
					if (townhall && townhall->provinceId() != battle.provinceId) // Only show non-capital provinces
					{
						FVector displayLocation = data->DisplayLocation(provinceSys.GetProvinceCenterTile(battle.provinceId).worldAtom2());
						
						URegionHoverUI* regionHoverUI = _regionHoverUIs.GetHoverUI<URegionHoverUI>(battle.provinceId, UIEnum::RegionHoverUI, this, _worldWidgetParent, displayLocation, dataSource()->zoomDistance(),
							[&](URegionHoverUI* ui)
							{
								ui->IconImage->SetBrushFromMaterial(assetLoader()->M_GeoresourceIcon); // SetBrushFromMaterial must be here since doing it every tick causes leak
							},
							WorldZoomTransition_Region4x4ToMap
						);

						regionHoverUI->UpdateBattlefieldUI(battle.provinceId, battle);
						regionHoverUI->ProvinceOverlay->SetVisibility(ESlateVisibility::Collapsed);
						regionHoverUI->BattlefieldUI->SetVisibility(ESlateVisibility::SelfHitTestInvisible);
					}
				}
			}
		}
	}
	
	_mapHoverIcons.AfterAdd();
}

void UWorldSpaceUI::TickPlacementInstructions()
{
	// Above Cursor Placement Text
	PlacementInfo placementInfo = inputSystemInterface()->PlacementBuildingInfo();
	FVector displayLocation = dataSource()->DisplayLocation(placementInfo.mouseOnTile.worldAtom2());

	int32 tileTownId = simulation().tileOwnerTown(placementInfo.mouseOnTile);

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
		punBox->AddRichTextCenter(LOCTEXT("DragGather1_Instruct", "Click and drag cursor"));
		punBox->AddRichTextCenter(LOCTEXT("DragGather2_Instruct", "to specify the area to harvest"));
	}
	else if (needInstruction(PlacementInstructionEnum::DragRoad1)) {
		punBox->AddRichTextCenter(LOCTEXT("DragRoad11_Instruct", "Click and drag cursor"));
		punBox->AddRichTextCenter(LOCTEXT("DragRoad12_Instruct", "to build"));
	}
	//else if (needInstruction(PlacementInstructionEnum::DragRoad2)) { // Not used yet???
	//	//punBox->AddRichText("Shift-click to repeat")->SetJustification(ETextJustify::Type::Center)->SetAutoWrapText(false);
	//	//punBox->AddRichText("to build");
	//}
	else if (needInstruction(PlacementInstructionEnum::DragRoadStone)) {
		int32 stoneNeeded = getInstruction(PlacementInstructionEnum::DragRoadStone).intVar1;
		int32 resourceCount = 0;
		if (IsValidMajorTown(tileTownId)) {
			resourceCount = simulation().resourceCountTown(tileTownId, ResourceEnum::Stone);
		}
		punBox->AddRichText(
			TextRed(to_string(stoneNeeded), resourceCount < stoneNeeded) + "<img id=\"Stone\"/>"
		);
	}
	else if (needInstruction(PlacementInstructionEnum::DragRoadIntercity)) {
		int32 goldNeeded = getInstruction(PlacementInstructionEnum::DragRoadIntercity).intVar1;
		punBox->AddRichText(
			TextRed(to_string(goldNeeded), simulation().moneyCap32(playerId()) < goldNeeded) + "<img id=\"Coin\"/>"
		);
	}
	
	else if (needInstruction(PlacementInstructionEnum::DragStorageYard)) {
		punBox->AddRichTextParsed(getInstruction(PlacementInstructionEnum::DragStorageYard).instruction);
	}

	else if (needInstruction(PlacementInstructionEnum::FarmAndRanch)) {
		punBox->AddRichTextCenter(LOCTEXT("FarmAndRanch_Instruct", "<Red>Cannot be built on desert</>"));
	}
	else if (needInstruction(PlacementInstructionEnum::FarmNoValidSeedForRegion)) {
		punBox->AddRichTextCenter(LOCTEXT("FarmNoValidSeedForRegion_Instruct", "<Red>None of your seeds can be planted in this province.</>"));
	}
	else if (needInstruction(PlacementInstructionEnum::DragFarm)) {
		punBox->AddRichTextParsed(getInstruction(PlacementInstructionEnum::DragFarm).instruction);
	}
	
	//else if (needInstruction(PlacementInstructionEnum::Kidnap)) {
	//	punBox->AddRichTextParsed(getInstruction(PlacementInstructionEnum::Kidnap).instruction);
	//}
	else if (needInstruction(PlacementInstructionEnum::Generic)) {
		LOCTEXT("Terrorism_PlaceInstruction", "Use on opponent's Townhall.");
		punBox->AddRichTextParsed(getInstruction(PlacementInstructionEnum::Generic).instruction);
	}
	
	else if (needInstruction(PlacementInstructionEnum::DragDemolish)) {
		punBox->AddRichTextCenter(LOCTEXT("DragDemolish1_Instruct", "Click and drag cursor"));
		punBox->AddRichTextCenter(LOCTEXT("DragDemolish2_Instruct", "to specify the area to demolish"));
	}

	else if (needInstruction(PlacementInstructionEnum::DeliveryPointInstruction)) {
		punBox->AddRichTextCenter(LOCTEXT("DeliveryPointInstruction1_Instruct", "Choose storage/market"));
		punBox->AddRichTextCenter(LOCTEXT("DeliveryPointInstruction2_Instruct", "to set the delivery point."));
	}
	else if (needInstruction(PlacementInstructionEnum::DeliveryPointMustBeStorage)) {
		punBox->AddRichTextCenter(LOCTEXT("DeliveryPointMustBeStorage1_Instruct", "<Red>Delivery Point must be</>"));
		punBox->AddRichTextCenter(LOCTEXT("DeliveryPointMustBeStorage2_Instruct", "<Red>Storage Building or Market</>"));
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
		punBox->AddRichTextCenter(LOCTEXT("MustBeNearRiver_Instruct", "<Red>Must be built near river or oasis</>"));
	}
	else if (needInstruction(PlacementInstructionEnum::LogisticsOffice))
	{
		punBox->AddRichTextCenter(LOCTEXT("LogisticsOffice1_Instruct", "Take resources from within radius"));
		punBox->AddRichTextCenter(LOCTEXT("LogisticsOffice2_Instruct", "Ship them to faraway destination"));
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
	else if (needInstruction(PlacementInstructionEnum::ResourceOutpostNoGeoresource)) {
		punBox->AddRichTextCenter(LOCTEXT("ColonyNoGeoresource_Instruct", "<Red>Must be built on a province with georesource</>"));
	}
	else if (needInstruction(PlacementInstructionEnum::ResourceOutpost)) {
		int32 provinceId = simulation().GetProvinceIdClean(placementInfo.mouseOnTile);
		if (provinceId != -1)
		{
			ResourceEnum resourceEnum = simulation().georesource(provinceId).info().resourceEnum;
			if (resourceEnum != ResourceEnum::None) {
				int32 resourceCount = ResourceOutpost::GetColonyResourceIncome(resourceEnum);
				punBox->AddIconPair(TEXT_NUMSIGNED(resourceCount), resourceEnum, FText::Format(INVTEXT(" {0}"), LOCTEXT("per round", "per round")));
			}
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
		punBox->AddRichTextCenter(LOCTEXT("MustBeNextToIntercityRoad_Instruct", "<Red>Colony's Townhall must be connected to Capital's Townhall through Intercity Road.</>"));
	}
	else if (needInstruction(PlacementInstructionEnum::PortColonyNeedsPort)) {
		punBox->AddRichTextCenter(LOCTEXT("PortColonyNeedsPort_Instruct", "<Red>Port Colony requires a Trading Port in the Capital.</>"));
	}
	else if (needInstruction(PlacementInstructionEnum::ColonyTooFar)) {
		punBox->AddRichTextCenter(LOCTEXT("ColonyTooFar_Instruct", "<Red>Too far from Townhall</>\n<Red>(max 500 tiles)</>"));
	}
	else if (needInstruction(PlacementInstructionEnum::PortColonyTooFar)) {
		punBox->AddRichTextCenter(LOCTEXT("ColonyTooFar_Instruct", "<Red>Too far from Trading Port</>\n<Red>(max 1000 tiles)</>"));
	}
	else if (needInstruction(PlacementInstructionEnum::ColonyClaimCost)) {
		int32 claimCost = getInstruction(PlacementInstructionEnum::ColonyClaimCost).intVar1;
		int32 moneyCap32 = simulation().moneyCap32(playerId());
		punBox->AddRichTextCenter(FText::Format(
			LOCTEXT("ClaimProvincesCost_Instruct", "Provinces Claim Cost <img id=\"Coin\"/>{0}"),
			TextRed(TEXT_NUM(claimCost), moneyCap32 < claimCost)
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

	bool showResource = placementInfo.buildingEnum != CardEnum::None && !hasPrimaryInstruction && IsValidMajorTown(tileTownId);

	if (!hasPrimaryInstruction && IsBuildingCard(placementInfo.buildingEnum)) 
	{
		if (needInstruction(PlacementInstructionEnum::Rotate)) {
			punBox->AddRichTextCenter(LOCTEXT("Rotate_Instruct", "Press R to rotate"));
			punBox->AddSpacer(12);

			showResource = false;
		}
		else if (needInstruction(PlacementInstructionEnum::Shift))
		{
			punBox->AddRichTextCenter(LOCTEXT("Shift1_Instruct", "Shift-click for"));
			punBox->AddRichTextCenter(LOCTEXT("Shift2_Instruct", "multiple placement"));
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
		int32 efficiency = Building::GetDistanceBasedEfficiency(tileTownId, placementInfo.mouseOnTile, placementInfo.buildingEnum, Windmill::Radius, &simulation());
		addEfficiencyText(efficiency);
		punBox->AddSpacer(12);
	}
	else if (placementInfo.buildingEnum == CardEnum::IrrigationPump)
	{
		int32 efficiency = Building::GetDistanceBasedEfficiency(tileTownId, placementInfo.mouseOnTile, placementInfo.buildingEnum, IrrigationPump::Radius, &simulation());
		addEfficiencyText(efficiency);
		punBox->AddSpacer(12);
	}
	else if (placementInfo.buildingEnum == CardEnum::Beekeeper)
	{
		int32 efficiency = Beekeeper::BeekeeperBaseEfficiency(tileTownId, placementInfo.mouseOnTile, &simulation());
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
		FText textTag = INVTEXT("<NotFlashing>");
		if (fruitTreeCount < 20) {
			textTag = INVTEXT("<FlashingRed>");
		} else if (fruitTreeCount < 30) {
			textTag = INVTEXT("<AlmostFlashing>");
		}
		
		punBox->AddRichTextCenter(
			FText::Format(LOCTEXT("PlaceInfo_Appeal", "{0}Fruit Tree Count: {1}</>"),
				textTag, TEXT_NUM(fruitTreeCount)
			)
		);
		punBox->AddSpacer(12);
	}
	// Building with Instructions
	else if (IsFunServiceBuilding(placementInfo.buildingEnum))
	{
		int32 appealPercent = simulation().overlaySystem().GetAppealPercent(placementInfo.mouseOnTile);
		int32 serviceQuality = FunBuilding::ServiceQuality(placementInfo.buildingEnum, appealPercent);
		punBox->AddRichTextCenter(TextRedOrange(FText::Format(LOCTEXT("PlaceInfo_ServiceQuality", "Service Quality: {0}%"), TEXT_NUM(serviceQuality)), appealPercent, 80, 60));
		punBox->AddSpacer(12);
	}
	else if (placementInfo.buildingEnum == CardEnum::Hotel)
	{
		int32 appealPercent = simulation().overlaySystem().GetAppealPercent(placementInfo.mouseOnTile);
		int32 serviceQuality = Hotel::GetServiceQualityFromAppeal(appealPercent);
		punBox->AddRichTextCenter(TextRedOrange(FText::Format(LOCTEXT("PlaceInfo_ServiceQuality", "Base Service Quality: {0}%"), TEXT_NUM(serviceQuality)), appealPercent, 80, 60));
		punBox->AddSpacer(12);
	}
	

	// Show building resource need...
	if (showResource)
	{
		FactionEnum factionEnum = simulation().playerOwned(playerId()).factionEnum();
		
		std::vector<int32> constructionResources = GetBuildingInfo(placementInfo.buildingEnum).GetConstructionResources(factionEnum);
		for (int32 i = 0; i < constructionResources.size(); i++) {
			if (constructionResources[i] > 0) {
				ResourceEnum resourceEnum = ConstructionResources[i];
				int32 neededCount = constructionResources[i];
				
				bool isRed = simulation().resourceSystem(tileTownId).resourceCountWithDrops(resourceEnum) < neededCount;
				
				punBox->AddIconPair(FText(), resourceEnum, TEXT_NUM(neededCount), isRed);
			}
		}
	}

	endInstruction();
}


#undef LOCTEXT_NAMESPACE