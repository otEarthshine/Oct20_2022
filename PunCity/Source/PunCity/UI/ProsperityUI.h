// Pun Dumnernchanvanit's

#pragma once

#include "PunWidget.h"
#include "ProsperityColumnUI.h"
#include "ProsperityBoxUI.h"
#include "ProsperityUI.generated.h"

/**
 * 
 */
UCLASS()
class PROTOTYPECITY_API UProsperityUI : public UPunWidget
{
	GENERATED_BODY()
public:
	UPROPERTY(meta = (BindWidget)) UScrollBox* ProsperityScrollBox;
	
	UPROPERTY(meta = (BindWidget)) UButton* CloseButton;
	UPROPERTY(meta = (BindWidget)) UButton* CloseButton2;

	UPROPERTY() TMap<int32, UProsperityBoxUI*> techEnumToProsperityBox;

	bool isInitialized = false;

	void PunInit()
	{
		CloseButton->OnClicked.AddDynamic(this, &UProsperityUI::CloseUI);
		CloseButton2->OnClicked.AddDynamic(this, &UProsperityUI::CloseUI);
	}

	void SetShowUI(bool show)
	{
		if (show) {
			networkInterface()->ResetGameUI();
			simulation().TryRemovePopups(playerId(), PopupReceiverEnum::UnlockedHouseTree_ShowProsperityUI);
			
			dataSource()->Spawn2DSound("UI", "UIWindowOpen");
		}
		else {
			dataSource()->Spawn2DSound("UI", "UIWindowClose");
		}

		ProsperityScrollBox->SetScrollOffset(0);

		SetVisibility(show ? ESlateVisibility::SelfHitTestInvisible : ESlateVisibility::Collapsed);
		TickUI();
	}

	void TickUI()
	{
		const auto& unlockSys = simulation().unlockSystem(playerId());
		
		if (GetVisibility() == ESlateVisibility::Collapsed) {
			// Open tech UI if there is no more queue...
			if (unlockSys->shouldOpenProsperityUI) {
				SetShowUI(true);
				unlockSys->shouldOpenProsperityUI = false;
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
	}

private:
	void SetupTechBoxUIs()
	{
		// Setup all the House Upgrade Techs
		ProsperityScrollBox->ClearChildren();
		
		UnlockSystem* unlockSys = simulation().unlockSystem(playerId());
		const std::vector<std::vector<std::shared_ptr<ResearchInfo>>>& houseLvlToProsperityTech = unlockSys->houseLvlToProsperityTech();
		const std::vector<std::vector<int32>>& houseLvlToUnlockCount = unlockSys->houseLvlToUnlockCount();
		
		for (size_t i = 1; i < houseLvlToProsperityTech.size(); i++)
		{
			UProsperityColumnUI* prosperityColumnUI = AddWidget<UProsperityColumnUI>(UIEnum::ProsperityColumnUI);
			prosperityColumnUI->HouseLevelText->SetText(ToFText("House Lvl " + std::to_string(i)));
			prosperityColumnUI->ProsperityTechList->ClearChildren();
			ProsperityScrollBox->AddChild(prosperityColumnUI);
			
			auto& techs = houseLvlToProsperityTech[i];
			for (const auto& tech : techs)
			{
				UProsperityBoxUI* prosperityBox = AddWidget<UProsperityBoxUI>(UIEnum::ProsperityBoxUI);
				prosperityColumnUI->ProsperityTechList->AddChild(prosperityBox);

				techEnumToProsperityBox.Add(static_cast<int32>(tech->techEnum), prosperityBox);
				PUN_CHECK(prosperityBox->techEnum == TechEnum::None);

				prosperityBox->Init(this, tech->techEnum);

				prosperityBox->TechName->SetText(ToFText(tech->GetName()));
			}
		}

		isInitialized = true;
	}

	
	UFUNCTION() void CloseUI() {
		SetShowUI(false);
	}
};
