// Fill out your copyright notice in the Description page of Project Settings.


#include "TechTreeUI.h"
#include "TechEraUI.h"

using namespace std;

#define LOCTEXT_NAMESPACE "TechUI"


void UTechTreeUI::SetupTechBoxColumn(const std::vector<TechEnum>& techEnums, UVerticalBox* columnBox)
{
	UnlockSystem* unlockSys = simulation().unlockSystem(playerId());
	
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

				if (UOverlay* specialLine = GetTechSpecialLine(techEnum)) {
					techBoxChild->lineChild2 = specialLine;
				}
			}
		}
	}

	check(techIndex == techEnums.size());
}



void UTechTreeUI::TickUI()
{
	auto& sim = simulation();
	const auto& unlockSys = sim.unlockSystem(playerId());

	if (GetVisibility() == ESlateVisibility::Collapsed) {
		// Open tech UI if there is no more queue...
		if (GetShouldOpenUI()) {
			SetShowUI(true);
			SetShouldOpenUI(false);
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
	 * Science Points
	 */
	{
		TArray<FText> args;
		unlockSys->SetDisplaySciencePoint(args);
		ADDTEXT_(INVTEXT(" {0}"), LOCTEXT("Science Points", "Science Points"));
		ScienceAmountText->SetText(JOINTEXT(args));
	}

	/*
	 * Tech Box Unlocked
	 */

	// Update highlight to highlight the tech queue
	const auto& techQueue = unlockSys->techQueue();
	if (techQueue != _lastTechQueue || GetNeedDisplayUpdate())
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
			if (techEnumToTechBox.Contains(static_cast<int>(techEnum))) {
				techEnumToTechBox[static_cast<int>(techEnum)]->SetTechState(tech->state, false, true, tech);
			}
		}

		_lastTechQueue = techQueue;
		SetNeedDisplayUpdate(false);
	}

	// Move research fraction...
	if (techQueue.size() > 0) {
		auto tech = unlockSys->GetTechInfo(techQueue.back());
		if (techEnumToTechBox.Contains(static_cast<int>(tech->techEnum))) {
			techEnumToTechBox[static_cast<int>(tech->techEnum)]->SetTechState(tech->state, false, true, tech);
		}
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
	if (unlockSys->IsLocked(techBox->techEnum)) {
		dataSource()->Spawn2DSound("UI", "UIIncrementalError");
		return;
	}
	if (!unlockSys->IsResearchable(techBox->techEnum)) {
		dataSource()->Spawn2DSound("UI", "UIIncrementalError");
		return;
	}

	// Disallow clicking with requirements not met
	if (!unlockSys->IsRequirementMetForTech(techBox->techEnum)) 
	{
		simulation().AddPopupToFront(playerId(),
			unlockSys->GetTechRequirementPopupText(techBox->techEnum),
			GetExclusiveUIEnum(), "PopupCannot"
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