// Pun Dumnernchanvanit's


#include "TechUI.h"
#include "TechEraUI.h"

using namespace std;

#define LOCTEXT_NAMESPACE "TechUI"

void UTechUI::SetupTechBoxUIs()
{
	UnlockSystem* unlockSys = simulation().unlockSystem(playerId());
	const std::vector<std::vector<TechEnum>>& eraToTechEnums = unlockSys->columnToTechEnums();

	TechScrollBox->ClearChildren();

	for (int32 i = 1; i < eraToTechEnums.size(); i++)
	{
		UTechEraUI* techEraUI = AddWidget<UTechEraUI>(UIEnum::TechEraUI);

		{ // EraText
			techEraUI->EraText->SetText(FText::Format(LOCTEXT("EraX", "Era {0}"), eraNumberToText[i]));

			TArray<FText> args;
			ADDTEXT_(LOCTEXT("EraX", "Era {0}"), eraNumberToText[i]);

			if (i < eraToTechEnums.size() - 1)
			{
				ADDTEXT_INV_("<space>");
				ADDTEXT_(
					LOCTEXT("UnlockTechToUnlockEra", "Unlock {0} Technologies in Era {1} to unlock Era {2}."),
					unlockSys->techsToUnlockedNextEra(i),
					eraNumberToText[i],
					eraNumberToText[i + 1]
				);

				TArray<FText> args2;
				UnlockSystem::EraUnlockedDescription(args2, i + 1, true);

				if (args2.Num() > 0) {
					ADDTEXT_INV_("<space>");
					ADDTEXT_(LOCTEXT("RewardForUnlockEra", "Rewards for Unlocking Era {0}:"), eraNumberToText[i + 1]);
					ADDTEXT_INV_("<space>");
					ADDTEXT__(JOINTEXT(args2));
				}
			}

			AddToolTip(techEraUI->EraText, args);
		}
		
		techEraUI->TechList->ClearChildren();
		TechScrollBox->AddChild(techEraUI);
		std::vector<TechEnum> techEnums = eraToTechEnums[i];
		
		for (TechEnum techEnum : techEnums)
		{
			auto tech = unlockSys->GetTechInfo(techEnum);
			
			UTechBoxUI* techBox = AddWidget<UTechBoxUI>(UIEnum::TechBox);
			techEraUI->TechList->AddChild(techBox);
			techBox->SetPadding(FMargin(0, 0, 0, 12));
			
			techEnumToTechBox.Add(static_cast<int32>(tech->techEnum), techBox);
			PUN_CHECK(techBox->techEnum == TechEnum::None);

			techBox->Init(this, tech->techEnum);

			techBox->TechName->SetText(tech->GetName());
		}
	}

	isInitialized = true;
}

void UTechUI::TickUI()
{
	const auto& unlockSys = simulation().unlockSystem(playerId());

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
		} else {
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

	int32 currentEra = unlockSys->currentTechColumn();

	{
		TArray<FText> args;
		unlockSys->SetDisplaySciencePoint(args);
		ADDTEXT_(INVTEXT(" {0}"), LOCTEXT("Science Points", "Science Points"));
		ScienceAmountText->SetText(JOINTEXT(args));
	}

	// Set next tech sci need
				//ss << techInfo->scienceNeeded(unlockSys->techsFinished) << "<img id=\"Science\"/> Science";
			//tooltipBox->AddRichText(ss);
			//tooltipBox->AddLineSpacer(12);

	// Set Era status...
	for (int32 i = 0; i < TechScrollBox->GetChildrenCount(); i++)
	{
		int32 era = i + 1;
		auto techEraUI = CastChecked<UTechEraUI>(TechScrollBox->GetChildAt(i));
		techEraUI->EraText->SetColorAndOpacity(era <= currentEra ? FLinearColor::White : FLinearColor(.2, .2, .2));

		if (era == currentEra + 1) 
		{
			techEraUI->EraUnlockText->SetText(
				FText::Format(LOCTEXT("Unlock: X/Y Era Z Techs", "Unlock: {0}/{1} Era {2} Techs"),
					TEXT_NUM(unlockSys->techsUnlockedInEra(currentEra)), 
					TEXT_NUM(unlockSys->techsToUnlockedNextEra(currentEra)), 
					eraNumberToText[i])
			);
		}
		else {
			techEraUI->EraUnlockText->SetText(FText());
		}
	}
	

	// Update highlight to highlight the tech queue
	const auto& techQueue = unlockSys->techQueue();
	if (techQueue != _lastTechQueue || unlockSys->needTechDisplayUpdate)
	{
		// First pass: isResearched or isLocked
		for (const auto& pair : techEnumToTechBox) {
			UTechBoxUI* techBox = pair.Value;
			auto tech = unlockSys->GetTechInfo(techBox->techEnum);
			bool isLocked = tech->column > currentEra;
			
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

void UTechUI::CallBack1(UPunWidget* punWidgetCaller, CallbackEnum callBackEnum)
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
	if (!unlockSys->IsRequirementMetForTech(techBox->techEnum)) {
		auto tech = unlockSys->GetTechInfo(techBox->techEnum);
		
		simulation().AddPopupToFront(playerId(), 
			FText::Format(
				LOCTEXT("NeedSatisfyTechPrereq_Pop", "Technology's Prerequisites not met.<space>Satisfy the Prerequisites by producing {0} {1}"), 
				TEXT_NUM(tech->requiredResourceCount), 
				GetResourceInfo(tech->requiredResourceEnum).name
			),
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