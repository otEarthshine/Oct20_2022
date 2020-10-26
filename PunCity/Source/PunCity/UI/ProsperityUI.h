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
		SetVisibility(ESlateVisibility::Collapsed);
		
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

		// TickUI for Boxes
		const std::vector<std::vector<TechEnum>>& houseLvlToProsperityTechEnum = unlockSys->houseLvlToProsperityTechEnum();
		const std::vector<std::vector<int32>>& houseLvlToUnlockCounts = unlockSys->houseLvlToUnlockCounts();

		for (size_t i = 1; i < houseLvlToProsperityTechEnum.size(); i++)
		{
			std::vector<TechEnum> techEnums = houseLvlToProsperityTechEnum[i];;
			for (size_t j = techEnums.size(); j-- > 0;) {
				techEnumToProsperityBox[static_cast<int>(techEnums[j])]->TickUI();
			}
		}
	}

private:
	void SetupTechBoxUIs()
	{
		// Setup all the House Upgrade Techs
		ProsperityScrollBox->ClearChildren();
		
		UnlockSystem* unlockSys = simulation().unlockSystem(playerId());
		const std::vector<std::vector<TechEnum>>& houseLvlToProsperityTechEnum = unlockSys->houseLvlToProsperityTechEnum();
		const std::vector<std::vector<int32>>& houseLvlToUnlockCounts = unlockSys->houseLvlToUnlockCounts();
		
		for (size_t i = 1; i < houseLvlToProsperityTechEnum.size(); i++)
		{
			UProsperityColumnUI* prosperityColumnUI = AddWidget<UProsperityColumnUI>(UIEnum::ProsperityColumnUI);

			prosperityColumnUI->HouseLevelText->SetText(ToFText("House Lvl " + std::to_string(i) + "+"));
			{
				std::stringstream tip;
				tip << "Unlock technologies in this column by increasing the number of houses with lvl " << i << " and above.";
				tip << "<space>";
				
				tip << "<bullet>Houses lvl " << i << ": " << simulation().GetHouseLvlCount(playerId(), i, false) << "</>";
				tip << "<bullet>Houses lvl " << i << "+: " << simulation().GetHouseLvlCount(playerId(), i, true) << "</>";
				
				AddToolTip(prosperityColumnUI->HouseLevelText, tip);
			}
			
			prosperityColumnUI->ProsperityTechList->ClearChildren();
			ProsperityScrollBox->AddChild(prosperityColumnUI);
			
			const std::vector<TechEnum>& techEnums = houseLvlToProsperityTechEnum[i];
			
			for (size_t j = techEnums.size(); j-- > 0;) // Prosperity Techs are arrange upward...
			{
				UProsperityBoxUI* prosperityBox = AddWidget<UProsperityBoxUI>(UIEnum::ProsperityBoxUI);
				prosperityColumnUI->ProsperityTechList->AddChild(prosperityBox);

				techEnumToProsperityBox.Add(static_cast<int32>(techEnums[j]), prosperityBox);
				PUN_CHECK(prosperityBox->uiTechEnum == TechEnum::None);

				prosperityBox->Init(this, techEnums[j], i, houseLvlToUnlockCounts[i][j], j);
				prosperityBox->TechName->SetText(ToFText(unlockSys->GetTechInfo(techEnums[j])->GetName()));
			}
		}

		isInitialized = true;
	}

	
	UFUNCTION() void CloseUI() {
		SetShowUI(false);
	}
};
