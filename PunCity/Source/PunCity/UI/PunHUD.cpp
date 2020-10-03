// Fill out your copyright notice in the Description page of Project Settings.

#include "PunHUD.h"
#include "Slate/SlateGameResources.h"
#include "Styling/SlateStyleRegistry.h"

using namespace std;

APunHUD::APunHUD()
{
	INIT_LOG("Construct APunHUD")
	
	_uiClasses.SetNum(UIEnumCount);

	LoadClass(UIEnum::MainGame, "MainGameUIWidget");
	LoadClass(UIEnum::EscMenu, "EscMenu/EscMenuWidget");
	LoadClass(UIEnum::Popup, "PopupUIWidget");
	LoadClass(UIEnum::Quest, "QuestUIWidget");

	LoadClass(UIEnum::Chat, "ChatUIWidget");
	LoadClass(UIEnum::TopLayerUI, "TopLayerUIWidget");
	LoadClass(UIEnum::Policy, "PolicyMenuWidget");
	LoadClass(UIEnum::Trade, "WorldTradeUIWidget");
	LoadClass(UIEnum::TradeIntercity, "IntercityTradeUIWidget");
	LoadClass(UIEnum::TargetConfirm, "TargetConfirmUI");

	LoadClass(UIEnum::InitialResourceUI, "InitialResourceWidget");
	LoadClass(UIEnum::GiftResourceUI, "GiftResourceWidget");

	LoadClass(UIEnum::DragCardSlot, "Drag/DragCardSlotWidget");
	LoadClass(UIEnum::DragCard, "Drag/DragCardWidget");

	LoadClass(UIEnum::BuildingPlacementButton, "BuildingPlacementButtonWidget");
	LoadClass(UIEnum::CardMini, "CardMiniWidget");
	LoadClass(UIEnum::CardSlot, "CardSlotWidget");
	LoadClass(UIEnum::ResearchTypeButton, "ResearchTypeButtonWidget");

	LoadClass(UIEnum::ToolTip, "TooltipWidget");
	LoadClass(UIEnum::TradeRow, "WorldTradeRowWidget");
	LoadClass(UIEnum::TradeRowIntercity, "IntercityTradeRowWidget");
	
	LoadClass(UIEnum::QuestElement, "QuestMiniElementWidget");
	LoadClass(UIEnum::PlayerCompareElement, "PlayerCompareElement");
	LoadClass(UIEnum::PlayerCompareDetailedElement, "PlayerDetailElement");
	LoadClass(UIEnum::ResourceStatTableRow, "StatisticsUI/ResourceStatTableRow");
	LoadClass(UIEnum::BuildingStatTableRow, "StatisticsUI/BuildingStatTableRow");

	LoadClass(UIEnum::ArmyUI, "ArmyUI");
	LoadClass(UIEnum::ArmyRow, "ArmyRow");

	LoadClass(UIEnum::WorldSpaceUI, "WorldSpaceUI/WorldSpaceUIWidget");
	LoadClass(UIEnum::HoverIcon, "WorldSpaceUI/HoverIconWidget");
	LoadClass(UIEnum::HoverBuildingJob, "WorldSpaceUI/BuildingJobUIWidget");
	LoadClass(UIEnum::JobHumanIcon, "WorldSpaceUI/HumanIcon");
	LoadClass(UIEnum::HoverTownhall, "WorldSpaceUI/TownhallHoverInfoWidget");
	LoadClass(UIEnum::ResourceCompletionIcon, "WorldSpaceUI/ResourceCompletionIcon");
	LoadClass(UIEnum::BuildingReadyIcon, "WorldSpaceUI/ReadyIcon");
	LoadClass(UIEnum::BuildingNeedSetupIcon, "WorldSpaceUI/NeedSetupIcon");
	LoadClass(UIEnum::BuildingNoStorageIcon, "WorldSpaceUI/NoStorageSpaceIcon");
	LoadClass(UIEnum::RegionHoverUI, "WorldSpaceUI/RegionHoverUI");

	LoadClass(UIEnum::ObjectDescription, "ObjectDescriptionUI");
	LoadClass(UIEnum::ObjectDescriptionSystem, "ObjectDescriptionUISystem");
	LoadClass(UIEnum::ChooseResourceElement, "ChooseResourceElement");
	LoadClass(UIEnum::ManageStorageElement, "ManageStorageElement");
	LoadClass(UIEnum::StatisticsUI, "StatisticsUI/StatisticsUI");

	LoadClass(UIEnum::IconTextPair, "IconTextPair");
	LoadClass(UIEnum::BuildingResourceChain, "BuildingResourceChainWidget");
	LoadClass(UIEnum::PunTextWidget, "PunTextWidget");
	LoadClass(UIEnum::PunSpacerWidget, "PunSpacerWidget");
	LoadClass(UIEnum::PunLineSpacerWidget, "PunLineSpacerWidget");
	LoadClass(UIEnum::PunThinLineSpacerWidget, "PunThinLineSpacerWidget");
	LoadClass(UIEnum::PunScrollBoxWidget, "PunScrollBoxWidget");
	LoadClass(UIEnum::PunButton, "PunButtonWidget");
	LoadClass(UIEnum::PunDropdown, "PunDropdownWidget");
	LoadClass(UIEnum::PunEditableNumberBox, "PunEditableNumberBoxWidget");
	LoadClass(UIEnum::PunEditableNumberBoxHorizontal, "PunEditableNumberBoxHorizontalWidget");
	LoadClass(UIEnum::PunGraph, "PunGraphWidget");
	LoadClass(UIEnum::PunTutorialLink, "PunTutorialLinkWidget");

	LoadClass(UIEnum::AboveBuildingText, "WorldSpaceUI/AboveBuildingTextWidget");
	LoadClass(UIEnum::HoverTextIconPair, "WorldSpaceUI/HoverTextIconPairWidget");
	LoadClass(UIEnum::HoverTextIconPair3Lines, "WorldSpaceUI/HoverTextIconPair3LinesWidget");
	
	LoadClass(UIEnum::ComboComplete, "WorldSpaceUI/ComboCompleteWidget");
	LoadClass(UIEnum::BuildingComplete, "WorldSpaceUI/BuildingCompleteWidget");
	LoadClass(UIEnum::HouseUpgrade, "WorldSpaceUI/HouseUpgradeWidget");
	LoadClass(UIEnum::HouseDowngrade, "WorldSpaceUI/HouseDowngradeWidget");
	LoadClass(UIEnum::TownhallUpgrade, "WorldSpaceUI/TownhallUpgradeWidget");

	LoadClass(UIEnum::PunRichText, "PunRichTextWidget");
	LoadClass(UIEnum::PunRichText_Chat, "PunRichTextWidget_Chat");
	LoadClass(UIEnum::PunRichTextTwoSided, "PunRichTextTwoSidedWidget");
	LoadClass(UIEnum::PunRichTextBullet, "PunRichTextBulletWidget");
	//LoadClass(UIEnum::PunEventText, "PunEventTextWidget");

	LoadClass(UIEnum::PunItemIcon, "PunItemIconWidget");
	LoadClass(UIEnum::PunItemSelectionChoice, "PunItemSelectionChoiceWidget");

	LoadClass(UIEnum::TechTree, "TechTree/TechTreeUIWidget");
	LoadClass(UIEnum::TechUI, "TechTree/TechUIWidget");
	LoadClass(UIEnum::TechEraUI, "TechTree/TechEraUIWidget");
	LoadClass(UIEnum::TechBox, "TechTree/TechBoxUIWidget");
	
	LoadClass(UIEnum::ProsperityUI, "TechTree/ProsperityUIWidget");
	LoadClass(UIEnum::ProsperityColumnUI, "TechTree/ProsperityColumnUIWidget");
	LoadClass(UIEnum::ProsperityBoxUI, "TechTree/ProsperityBoxUIWidget");

	LoadClass(UIEnum::JobPriorityRow, "JobPriorityRowWidget");
	
	
	LoadClass(UIEnum::SaveSelection, "EscMenu/SaveSelectionWidget");

	LoadClass(UIEnum::PlayerListElement, "MainMenu/PlayerListElement");
	LoadClass(UIEnum::LobbyListElement, "MainMenu/LobbyListElement");

	LoadClass(UIEnum::HiddenSettingsRow, "HiddenSettingsRow");

	LoadClass(UIEnum::ArmyDeployButton, "WorldSpaceUI/ArmyDeployButton");
	LoadClass(UIEnum::ArmyLinesUILeft, "WorldSpaceUI/ArmyLinesUILeft");
	LoadClass(UIEnum::ArmyLinesUIRight, "WorldSpaceUI/ArmyLinesUIRight");
	LoadClass(UIEnum::ArmyUnitLeft, "WorldSpaceUI/ArmyUnitLeft");
	LoadClass(UIEnum::ArmyUnitRight, "WorldSpaceUI/ArmyUnitRight");
	LoadClass(UIEnum::ArmyMoveRow, "ArmyMoveRow");
	LoadClass(UIEnum::ArmyMoveUI, "ArmyMoveUI");
	LoadClass(UIEnum::ArmyChooseNodeButton, "ArmyChooseNodeButton");
	LoadClass(UIEnum::DamageFloatup, "WorldSpaceUI/DamageFloatup");

	// Slate Style
	//TSharedPtr<FSlateStyleSet> style = MakeShareable(new FSlateStyleSet("PunStyle"));
	_style = FSlateGameResources::New("PunStyle1", "/Game/UI/Styles", "/Game/UI/Styles");
	//_style = MakeShareable(&resources);
	//FSlateStyleRegistry::RegisterSlateStyle(_style.Get());
	//style->SetContentRoot(FPaths::GameContentDir() / "PunCity/UI/Styles");

	//const FSlateBrush* slate_spell_heal = style.Get().GetBrush(FName("HouseIconBrush"));
	//
	//style->Set("HouseIcon", new FSlateImageBrush("HouseIconBrush", FVector2D(512, 512)));
	//style->Set("StarvingIcon", new FSlateImageBrush("StarvingIconBrush", FVector2D(512, 512)));

	// Print styles
	TArray<const FSlateBrush*> outResources;
	_style->GetResources(outResources);
	//PUN_LOG("PUN: Styles: %lu", outResources.Num());
	for (int i = 0; i < outResources.Num(); i++) {
		//PUN_LOG("PUN: Test Slate Style: %s", *outResources[i]->GetResourceName().ToString());
	}


	uiEnumToWidgetPool.SetNum(UIEnumCount);
}

void APunHUD::PunTick(bool isPhotoMode)
{
#if !UI_ALL
	return;
#endif
	
	if (!_isInitialized) return;

	// Check photo mode...
	if (isPhotoMode) {
		// Hide UI if not already done
		if (!_isUIHideMode) 
		{
			uiVisibilityBeforeUIHideMode.clear();
			uiVisibilityBeforeUIHideMode.push_back(_mainGameUI->GetVisibility());
			uiVisibilityBeforeUIHideMode.push_back(_questUI->GetVisibility());
			uiVisibilityBeforeUIHideMode.push_back(_chatUI->GetVisibility());
			uiVisibilityBeforeUIHideMode.push_back(_topLayerGameUI->GetVisibility());
			uiVisibilityBeforeUIHideMode.push_back(_escMenuUI->GetVisibility());
			uiVisibilityBeforeUIHideMode.push_back(_worldSpaceUI->GetVisibility());

			_mainGameUI->SetVisibility(ESlateVisibility::Collapsed);
			_questUI->SetVisibility(ESlateVisibility::Collapsed);
			_chatUI->SetVisibility(ESlateVisibility::Collapsed);
			_topLayerGameUI->SetVisibility(ESlateVisibility::Collapsed);

			//_escMenuUI->SetVisibility(ESlateVisibility::Collapsed);
			_escMenuUI->TopLeftBoxWithSpacer->SetVisibility(ESlateVisibility::Collapsed);
			
			_worldSpaceUI->SetVisibility(ESlateVisibility::Collapsed);

			_worldSpaceUI->StartHideAllUI();

			_descriptionUISystem->HideForPhotoMode();
			
			_isUIHideMode = true;
		}

		_escMenuUI->TickLoadingScreen();
		return;
	}

	// Put UI's visibility back if it was hidden...
	if (_isUIHideMode) {
		_mainGameUI->SetVisibility(uiVisibilityBeforeUIHideMode[0]);
		_questUI->SetVisibility(uiVisibilityBeforeUIHideMode[1]);
		_chatUI->SetVisibility(uiVisibilityBeforeUIHideMode[2]);
		_topLayerGameUI->SetVisibility(uiVisibilityBeforeUIHideMode[3]);
		_escMenuUI->TopLeftBoxWithSpacer->SetVisibility(uiVisibilityBeforeUIHideMode[4]);
		_worldSpaceUI->SetVisibility(uiVisibilityBeforeUIHideMode[5]);
		
		_isUIHideMode = false;
	}

	//PUN_LOG("TickingHUD %f", DeltaSeconds);
	_mainGameUI->Tick();
	_popupUI->Tick();
	_questUI->Tick();

	_escMenuUI->Tick();
	
	_chatUI->Tick();
	_topLayerGameUI->Tick();
	
	_worldSpaceUI->TickWorldSpaceUI();
	_descriptionUISystem->Tick();
	_policyMenu->TickUI();
	_worldTradeUI->TickUI();
	_intercityTradeUI->TickUI();
	_targetConfirmUI->TickUI();
	_initialResourceUI->TickUI();
	_giftResourceUI->TickUI();
	
	_techUI->TickUI();
	_prosperityUI->TickUI();
	
	_statisticsUI->TickUI();
	_armyMoveUI->TickUI();

	// Raycast blocking
	_mainGameUI->CheckChildrenPointerOnUI();
	_popupUI->CheckChildrenPointerOnUI();
	_questUI->CheckChildrenPointerOnUI();
	_chatUI->CheckPointerOnUI();
	_statisticsUI->CheckPointerOnUI();
	
	_worldTradeUI->CheckPointerOnUI();
	_intercityTradeUI->CheckPointerOnUI();
	_targetConfirmUI->CheckPointerOnUI();
	_initialResourceUI->CheckPointerOnUI();
	_giftResourceUI->CheckPointerOnUI();
	
	_techUI->CheckPointerOnUI();
	_prosperityUI->CheckPointerOnUI();
	
	_escMenuUI->CheckPointerOnUI();
	_armyMoveUI->CheckPointerOnUI();
	UPunWidget::TickIsHovered();

	UpdateExclusiveUIDisplay();

	//// TODO: keeping this loading screen unhide since serverTick() doesn't work on client...
	//// Hide loading screen after ticking for 1 sec.
	//auto gameInstance = CastChecked<UPunGameInstance>(GetGameInstance());
	//GameSaveInfo saveInfo = gameInstance->saveSystem().GetLastSyncSaveInfo();
	//int32 mapStartTick = saveInfo.IsValid() ? saveInfo.gameTicks : 0;
	//
	//if (Time::Ticks() > Time::TicksPerSecond * 3 / 2 + mapStartTick) {
	//	_escMenuUI->HideLoadingScreen();
	//}

	// Hide the loading screen after server started ticking for 1.5 secs
	if (networkInterface()->serverTick() > Time::TicksPerSecond * 3 / 2) {
		_escMenuUI->HideLoadingScreen();
	}
}

void APunHUD::InitBeforeGameManager()
{
#if !UI_ALL
	return;
#endif
	
	PUN_DEBUG2("APunHUD::InitBeforeGameManager");
	
	_escMenuUI = AddWidgetToHUDCast<UEscMenuUI>(UIEnum::EscMenu);
	_escMenuUI->InitBeforeGameManager();
}

void APunHUD::Setup(IPunPlayerController* controller, USceneComponent* worldWidgetParent)
{
#if !UI_ALL
	return;
#endif
	
	PUN_DEBUG2("APunHUD::Setup");
	
	_controllerInterface = controller;

	_worldSpaceUI = AddWidgetToHUDCast<UWorldSpaceUI>(UIEnum::WorldSpaceUI);
	_worldSpaceUI->SetupClasses(_style, worldWidgetParent);
	
	_questUI = AddWidgetToHUDCast<UQuestUI>(UIEnum::Quest);
	_questUI->PunInit();

	_chatUI = AddWidgetToHUDCast<UChatUI>(UIEnum::Chat);
	_chatUI->PunInit();

	// description UI should be behind choose card UI (_mainGameUI)
	_descriptionUISystem = AddWidgetToHUDCast<UObjectDescriptionUISystem>(UIEnum::ObjectDescriptionSystem);
	_descriptionUISystem->CreateWidgets();

	_statisticsUI = AddWidgetToHUDCast<UStatisticsUI>(UIEnum::StatisticsUI);
	_statisticsUI->InitStatisticsUI();

	_mainGameUI = AddWidgetToHUDCast<UMainGameUI>(UIEnum::MainGame);
	_mainGameUI->PunInit();

	_worldTradeUI = AddWidgetToHUDCast<UWorldTradeUI>(UIEnum::Trade);
	_worldTradeUI->PunInit();

	_intercityTradeUI = AddWidgetToHUDCast<UIntercityTradeUI>(UIEnum::TradeIntercity);
	_intercityTradeUI->PunInit();

	_targetConfirmUI = AddWidgetToHUDCast<UTargetConfirmUI>(UIEnum::TargetConfirm);
	_targetConfirmUI->PunInit();

	_initialResourceUI = AddWidgetToHUDCast<UInitialResourceUI>(UIEnum::InitialResourceUI);
	_initialResourceUI->PunInit();

	_giftResourceUI = AddWidgetToHUDCast<UGiftResourceUI>(UIEnum::GiftResourceUI);
	_giftResourceUI->PunInit();

	_techUI = AddWidgetToHUDCast<UTechUI>(UIEnum::TechUI);
	_techUI->PunInit();

	_prosperityUI = AddWidgetToHUDCast<UProsperityUI>(UIEnum::ProsperityUI);
	_prosperityUI->PunInit();

	_armyMoveUI = AddWidgetToHUDCast<UArmyMoveUI>(UIEnum::ArmyMoveUI);
	_armyMoveUI->PunInit();

	// 
	_popupUI = AddWidgetToHUDCast<UPopupUI>(UIEnum::Popup);
	_popupUI->PunInit();

	// Top layer comes later
	_topLayerGameUI = AddWidgetToHUDCast<UTopLayerGameUI>(UIEnum::TopLayerUI);
	_topLayerGameUI->PunInit();
	
	//_policyMenu = AddWidgetToHUDCast<UPolicyMenu>(UIEnum::Policy);
	//_policyMenu->PunInit(_tradeUIWidget);

	PUN_DEBUG2("Setup EscMenu");
	_escMenuUI->RemoveFromViewport();
	_escMenuUI->AddToViewport(); // Re-add to viewport so it becomes the top UI
	_escMenuUI->PunInit();

	_isInitialized = true;
}


void APunHUD::KeyPressed_Escape()
{
#if !UI_ALL
	return;
#endif
	
	ExclusiveUIEnum uiEnum = GetCurrentExclusiveUIDisplay();

	_mainGameUI->EscDown();

	_worldTradeUI->CloseUI();
	_intercityTradeUI->CloseUI();
	_targetConfirmUI->CloseUI();
	
	_techUI->SetShowUI(false);
	_prosperityUI->SetShowUI(false);

	//if (uiEnum == ExclusiveUIEnum::None &&
	//	!_descriptionUISystem->IsShowingDescriptionUI())
	{
		_escMenuUI->KeyPressed_Escape();
	}

	_descriptionUISystem->CloseDescriptionUI();

	CloseStatisticsUI();

	_questUI->OnQuestDescriptionCloseButtonClick();

	_armyMoveUI->CloseArmyMoveUI();
}