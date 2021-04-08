// Fill out your copyright notice in the Description page of Project Settings.


#include "TechTreeUI.h"
#include "TechEraUI.h"

using namespace std;

#define LOCTEXT_NAMESPACE "TechUI"

void UTechTreeUI::SetupTechBoxUIs()
{
	UnlockSystem* unlockSys = simulation().unlockSystem(playerId());
	const std::vector<std::vector<TechEnum>>& columnToTechEnums = unlockSys->columnToTechEnums();


	auto setupTechBoxColumn = [&](int32 columnNumber, UVerticalBox* columnBox)
	{
		const std::vector<TechEnum>& techEnums = columnToTechEnums[columnNumber];
		int32 techIndex = 0;
		
		for (int32 i = 0; i < columnBox->GetChildrenCount(); i++)
		{
			UOverlay* techOverlayChild = Cast<UOverlay>(columnBox->GetChildAt(i));
			if (techOverlayChild)
			{
				UOverlay* lineChild = nullptr;
				UTechBoxUI* techBoxChild = nullptr;
				for (int32 j = 0; j < techOverlayChild->GetChildrenCount(); j++) {
					if (lineChild == nullptr) {
						lineChild = Cast<UOverlay>(techOverlayChild->GetChildAt(j));
					}
					if (techBoxChild == nullptr) {
						techBoxChild = Cast<UTechBoxUI>(techOverlayChild->GetChildAt(j));
					}
				}

				// Set TechBoxUI
				if (techBoxChild)
				{
					// Get TechEnum
					TechEnum techEnum = techEnums[techIndex];
					techIndex++;

					auto tech = unlockSys->GetTechInfo(techEnum);

					techEnumToTechBox.Add(static_cast<int32>(tech->techEnum), techBoxChild);
					PUN_CHECK(techBoxChild->techEnum == TechEnum::None);

					SetChildHUD(techBoxChild);
					techBoxChild->Init(this, tech->techEnum);
					techBoxChild->TechName->SetText(tech->GetName());
					techBoxChild->lineChild = lineChild;
					if (techEnum == TechEnum::Theatre) {
						techBoxChild->lineChild2 = TheatreLine;
					}
				}
			}
		}
	};

	setupTechBoxColumn(1, TechList_DarkAge1);
	setupTechBoxColumn(2, TechList_DarkAge2);

	setupTechBoxColumn(3, TechList_MiddleAge1);
	setupTechBoxColumn(4, TechList_MiddleAge2);
	setupTechBoxColumn(5, TechList_MiddleAge3);

	setupTechBoxColumn(6, TechList_EnlightenmentAge1);
	setupTechBoxColumn(7, TechList_EnlightenmentAge2);
	setupTechBoxColumn(8, TechList_EnlightenmentAge3);

	setupTechBoxColumn(9, TechList_IndustrialAge1);
	setupTechBoxColumn(10, TechList_IndustrialAge2);
	setupTechBoxColumn(11, TechList_IndustrialAge3);

	check(techEnumToTechBox.Num() == unlockSys->GetTechCount());

	

	//for (int32 i = 1; i < eraToTechEnums.size(); i++)
	//{
	//	UTechEraUI* techEraUI = AddWidget<UTechEraUI>(UIEnum::TechEraUI);

	//	{ // EraText
	//		techEraUI->EraText->SetText(FText::Format(LOCTEXT("EraX", "Era {0}"), eraNumberToText[i]));

	//		TArray<FText> args;
	//		ADDTEXT_(LOCTEXT("EraX", "Era {0}"), eraNumberToText[i]);

	//		if (i < eraToTechEnums.size() - 1)
	//		{
	//			ADDTEXT_INV_("<space>");
	//			ADDTEXT_(
	//				LOCTEXT("UnlockTechToUnlockEra", "Unlock {0} Technologies in Era {1} to unlock Era {2}."),
	//				unlockSys->techsToUnlockedNextEra(i),
	//				eraNumberToText[i],
	//				eraNumberToText[i + 1]
	//			);

	//			TArray<FText> args2;
	//			UnlockSystem::EraUnlockedDescription(args2, i + 1, true);

	//			if (args2.Num() > 0) {
	//				ADDTEXT_INV_("<space>");
	//				ADDTEXT_(LOCTEXT("RewardForUnlockEra", "Rewards for Unlocking Era {0}:"), eraNumberToText[i + 1]);
	//				ADDTEXT_INV_("<space>");
	//				ADDTEXT__(JOINTEXT(args2));
	//			}
	//		}

	//		AddToolTip(techEraUI->EraText, args);
	//	}

	//	techEraUI->TechList->ClearChildren();
	//	TechScrollBox->AddChild(techEraUI);
	//	std::vector<TechEnum> techEnums = eraToTechEnums[i];

	//	for (TechEnum techEnum : techEnums)
	//	{
	//		auto tech = unlockSys->GetTechInfo(techEnum);

	//		UTechBoxUI* techBox = AddWidget<UTechBoxUI>(UIEnum::TechBox);
	//		techEraUI->TechList->AddChild(techBox);

	//		techEnumToTechBox.Add(static_cast<int32>(tech->techEnum), techBox);
	//		PUN_CHECK(techBox->techEnum == TechEnum::None);

	//		techBox->Init(this, tech->techEnum);

	//		techBox->TechName->SetText(tech->GetName());
	//	}
	//}

	isInitialized = true;
}

void UTechTreeUI::TickUI()
{
	auto& sim = simulation();
	const auto& unlockSys = sim.unlockSystem(playerId());

	if (GetVisibility() == ESlateVisibility::Collapsed) {
		// Open tech UI if there is no more queue...
		if (unlockSys->shouldOpenTechUI) {
			SetShowUI(true);
			unlockSys->shouldOpenTechUI = false;
		}
		return;
	}

	// Try initialize
	if (!isInitialized) {
		if (simulation().isInitialized()) {
			SetupTechBoxUIs();
		}
		else {
			return;
		}
	}

	/*
	 *
	 */

	if (_isMouseDownScrolling)
	{
		//PUN_LOG("||||||||");
		FVector2D mouseOffset;
		GetWorld()->GetGameViewport()->GetMousePosition(mouseOffset);
		//PUN_LOG("GetMousePosition %f %f", mouseOffset.X, mouseOffset.Y);
		mouseOffset -= _initialMousePosition;
		//PUN_LOG("NativeOnMouseMove1 _initialMousePosition:%f mouseOffset:%f", _initialMousePosition.X, mouseOffset.X);

		float scrollOffset = _initialScrollOffset - mouseOffset.X;
		//PUN_LOG("NativeOnMouseMove2 _initialScrollOffset:%f -> scrollOffset:%f", _initialScrollOffset, scrollOffset);
		scrollOffset = FMath::Clamp(scrollOffset, 0.0f, TechScrollBox->GetScrollOffsetOfEnd());
		//PUN_LOG("NativeOnMouseMove3 %f mouseOffset:%f _initialScrollOffset:%f", scrollOffset, mouseOffset.X, _initialScrollOffset);
		//PUN_LOG("NativeOnMouseMove FINAL:%f", scrollOffset);

		TechScrollBox->SetScrollOffset(scrollOffset);
	}


	/*
	 *
	 */

	int32 currentEra = unlockSys->currentEra();

	{
		TArray<FText> args;
		unlockSys->SetDisplaySciencePoint(args);
		ADDTEXT_(INVTEXT(" {0}"), LOCTEXT("Science Points", "Science Points"));
		ScienceAmountText->SetText(JOINTEXT(args));
	}

	/*
	 * Era
	 */

	FLinearColor lockedColor(0.5, 0.5, 0.5, 1);
	FLinearColor unlockedColor(1, 1, 1, 1);
	
	Title_MiddleAge->SetColorAndOpacity(sim.IsResearched(playerId(), TechEnum::MiddleAge) ? unlockedColor : lockedColor);
	Title_EnlightenmentAge->SetColorAndOpacity(sim.IsResearched(playerId(), TechEnum::EnlightenmentAge) ? unlockedColor : lockedColor);
	Title_IndustrialAge->SetColorAndOpacity(sim.IsResearched(playerId(), TechEnum::IndustrialAge) ? unlockedColor : lockedColor);

	/*
	 * Tech Box Unlocked
	 */



	
	/*
	 * Old Era Status
	 */
	
	//// Set Era status...
	//for (int32 i = 0; i < TechScrollBox->GetChildrenCount(); i++)
	//{
	//	int32 era = i + 1;
	//	auto techEraUI = CastChecked<UTechEraUI>(TechScrollBox->GetChildAt(i));
	//	techEraUI->EraText->SetColorAndOpacity(era <= currentEra ? FLinearColor::White : FLinearColor(.2, .2, .2));

	//	if (era == currentEra + 1)
	//	{
	//		techEraUI->EraUnlockText->SetText(
	//			FText::Format(LOCTEXT("Unlock: X/Y Era Z Techs", "Unlock: {0}/{1} Era {2} Techs"),
	//				TEXT_NUM(unlockSys->techsUnlockedInEra(currentEra)),
	//				TEXT_NUM(unlockSys->techsToUnlockedNextEra(currentEra)),
	//				eraNumberToText[i])
	//		);
	//	}
	//	else {
	//		techEraUI->EraUnlockText->SetText(FText());
	//	}
	//}


	// Update highlight to highlight the tech queue
	const auto& techQueue = unlockSys->techQueue();
	if (techQueue != _lastTechQueue || unlockSys->needTechDisplayUpdate)
	{
		// First pass: isResearched or isLocked
		for (const auto& pair : techEnumToTechBox) {
			UTechBoxUI* techBox = pair.Value;
			auto tech = unlockSys->GetTechInfo(techBox->techEnum);
			bool isLocked = unlockSys->IsLocked(tech->techEnum);

			pair.Value->SetTechState(tech->state, isLocked, false, tech);
		}

		// Second pass: active
		for (TechEnum techEnum : techQueue) {
			auto tech = unlockSys->GetTechInfo(techEnum);
			techEnumToTechBox[static_cast<int>(techEnum)]->SetTechState(tech->state, false, true, tech);
		}

		_lastTechQueue = techQueue;
		unlockSys->needTechDisplayUpdate = false;
	}

	// Move research fraction...
	if (techQueue.size() > 0) {
		auto tech = unlockSys->GetTechInfo(techQueue.back());
		techEnumToTechBox[static_cast<int>(tech->techEnum)]->SetTechState(tech->state, false, true, tech);
	}


	/*
	 * Update tooltip
	 */
	for (const auto& pair : techEnumToTechBox) {
		pair.Value->UpdateTooltip();
	}
}

void UTechTreeUI::CallBack1(UPunWidget* punWidgetCaller, CallbackEnum callBackEnum)
{
	UTechBoxUI* techBox = CastChecked<UTechBoxUI>(punWidgetCaller);

	// Make sure this tech is not locked or researched
	auto unlockSys = simulation().unlockSystem(playerId());
	if (unlockSys->IsLocked(techBox->techEnum) ||
		unlockSys->IsResearched(techBox->techEnum))
	{
		dataSource()->Spawn2DSound("UI", "UIIncrementalError");
		return;
	}

	// Disallow clicking with requirements not met
	if (!unlockSys->IsRequirementMetForTech(techBox->techEnum)) 
	{
		simulation().AddPopupToFront(playerId(),
			unlockSys->GetTechRequirementPopupText(techBox->techEnum),
			ExclusiveUIEnum::TechTreeUI, "PopupCannot"
		);
		return;
	}


	PUN_LOG("Chose Research: %d", (int)techBox->techEnum);

	auto command = make_shared<FChooseResearch>();
	command->techEnum = techBox->techEnum;
	networkInterface()->SendNetworkCommand(command);

	// Play Sound
	dataSource()->Spawn2DSound("UI", "ResearchInitiated");
}


#undef LOCTEXT_NAMESPACE