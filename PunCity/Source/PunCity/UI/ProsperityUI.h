// Pun Dumnernchanvanit's

#pragma once

#include "PunWidget.h"
#include "TechBoxUI.h"
//#include "ProsperityBoxUI.h"
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

	UPROPERTY() TMap<int32, UTechBoxUI*> techEnumToProsperityBox;

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
			simulation().TryRemovePopups(playerId(), PopupReceiverEnum::DoneResearchEvent_ShowAllTrees);
			
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
		const std::vector<std::vector<TechEnum>>& columnToProsperityTechEnum = unlockSys->columnToUpgradeTechEnums();
		for (size_t i = 1; i < columnToProsperityTechEnum.size(); i++)
		{
			std::vector<TechEnum> techEnums = columnToProsperityTechEnum[i];;
			for (size_t j = techEnums.size(); j-- > 0;) {
				//techEnumToProsperityBox[static_cast<int>(techEnums[j])]->TickUI();
			}
		}
	}

public:
	virtual void CallBack1(UPunWidget* punWidgetCaller, CallbackEnum callBackEnum) override;
	

private:
	UTextBlock* GetColumnText(UVerticalBox* columnBox) {
		auto parent = columnBox->GetParent();
		USizeBox* textSizeBox = CastChecked<USizeBox>(parent->GetChildAt(0));
		return CastChecked<UTextBlock>(textSizeBox->GetChildAt(0));
	}
	
	
	void SetupTechBoxUIs()
	{
		// Setup all the House Upgrade Techs
		ProsperityScrollBox->ClearChildren();
		
		UnlockSystem* unlockSys = simulation().unlockSystem(playerId());
		const std::vector<std::vector<TechEnum>>& columnToProsperityTechEnum = unlockSys->columnToUpgradeTechEnums();


		for (int32 i = 1; i < ProsperityScrollBox->GetChildrenCount(); i++)
		{
			UVerticalBox* ColumnBoxes = CastChecked<UVerticalBox>(ProsperityScrollBox->GetChildAt(i));

			// Column Title
			USizeBox* ColumnTitleSizeBox = CastChecked<USizeBox>(ColumnBoxes->GetChildAt(0));
			UTextBlock* columnTitleText = CastChecked<UTextBlock>(ColumnTitleSizeBox->GetChildAt(0));

			auto setColumnTitle = [&](int32 era) {
				FText eraText = unlockSys->GetEraText(era);
				columnTitleText->SetText(FText::Format(
					NSLOCTEXT("ProsperityUI", "Requires X Age", "Requires {0}"),
					eraText
				));
				AddToolTip(columnTitleText, FText::Format(
					NSLOCTEXT("ProsperityUI", "UpgradeTechText_Tip", "Unlock Upgrades in this column by unlocking {0} on the Technology Tree"),
					eraText
				));
			};
			
			if (i == 3) { setColumnTitle(2); }
			if (i == 6) { setColumnTitle(3); }
			if (i == 9) { setColumnTitle(4); }
			else {
				columnTitleText->SetText(FText());
			}


			// Column Tech List
			UVerticalBox* columnTechList = CastChecked<UVerticalBox>(ColumnBoxes->GetChildAt(1));

			const std::vector<TechEnum>& techEnums = columnToProsperityTechEnum[i];
			int32 techEnumLocalIndex = 0;
			
			for (int32 j = 0; j < columnTechList->GetChildrenCount(); j++)
			{
				UWidget* techOrSpace = columnTechList->GetChildAt(j);
				if (Cast<USpacer>(techOrSpace)) {
					continue;
				}
				UTechBoxUI* prosperityBoxUI = Cast<UTechBoxUI>(techOrSpace);
				if (!prosperityBoxUI) {
					UWidget* prosperityBoxUIWidget = CastChecked<UOverlay>(techOrSpace)->GetChildAt(0);
					prosperityBoxUI = CastChecked<UTechBoxUI>(prosperityBoxUIWidget);
				}

				techEnumToProsperityBox.Add(static_cast<int32>(techEnums[j]), prosperityBoxUI);
				//PUN_CHECK(prosperityBoxUI->uiTechEnum == TechEnum::None);

				std::shared_ptr<ResearchInfo> techInfo = unlockSys->GetTechInfo(techEnums[j]);

				prosperityBoxUI->Init(this, techEnums[j]);
				prosperityBoxUI->TechName->SetText(unlockSys->GetTechInfo(techEnums[j])->GetName());
			}

			check(techEnumLocalIndex == techEnums.size());
		}
		
		//for (size_t i = 1; i < columnToProsperityTechEnum.size(); i++)
		//{
		//	UProsperityColumnUI* prosperityColumnUI = AddWidget<UProsperityColumnUI>(UIEnum::ProsperityColumnUI);

		//	prosperityColumnUI->HouseLevelText->SetText(FText::Format(
		//		NSLOCTEXT("ProsperityUI", "HouseLevelText", "House Lvl {0}+"),
		//		TEXT_NUM(i)
		//	));
		//	{	
		//		AddToolTip(prosperityColumnUI->HouseLevelText, FText::Format(
		//			NSLOCTEXT("ProsperityUI", "UpgradeTechText_Tip", "Unlock Upgrades in this column by increasing the number of houses with lvl {0} and above.<space><bullet>Houses lvl {0}: {1}</><bullet>Houses lvl {0}+: {2}</>"),
		//			TEXT_NUM(i),
		//			TEXT_NUM(simulation().GetHouseLvlCount(playerId(), i, false)),
		//			TEXT_NUM(simulation().GetHouseLvlCount(playerId(), i, true))
		//		));
		//	}
		//	
		//	prosperityColumnUI->ProsperityTechList->ClearChildren();
		//	ProsperityScrollBox->AddChild(prosperityColumnUI);
		//	
		//	const std::vector<TechEnum>& techEnums = columnToProsperityTechEnum[i];
		//	
		//	for (size_t j = techEnums.size(); j-- > 0;) // Prosperity Techs are arrange upward...
		//	{
		//		UProsperityBoxUI* prosperityBox = AddWidget<UProsperityBoxUI>(UIEnum::ProsperityBoxUI);
		//		prosperityColumnUI->ProsperityTechList->AddChild(prosperityBox);

		//		techEnumToProsperityBox.Add(static_cast<int32>(techEnums[j]), prosperityBox);
		//		PUN_CHECK(prosperityBox->uiTechEnum == TechEnum::None);

		//		prosperityBox->Init(this, techEnums[j], i, j);
		//		prosperityBox->TechName->SetText(unlockSys->GetTechInfo(techEnums[j])->GetName());
		//	}
		//}

		isInitialized = true;
	}

	
	UFUNCTION() void CloseUI() {
		SetShowUI(false);
	}
};
