// Pun Dumnernchanvanit's

#pragma once

#include "CoreMinimal.h"
#include "PunCity/UI/TechTreeUI.h"
#include "AuxTechTreeUI.generated.h"

/**
 * 
 */
UCLASS()
class PROTOTYPECITY_API UAuxTechTreeUI : public UTechTreeUI
{
	GENERATED_BODY()
public:

	virtual ExclusiveUIEnum GetExclusiveUIEnum() override { return ExclusiveUIEnum::ProsperityUI; }
	virtual bool GetShouldOpenUI() override { return  unlockSys()->shouldOpenProsperityUI; }
	virtual void SetShouldOpenUI(bool value) override { unlockSys()->shouldOpenProsperityUI = value; }
	
	virtual bool GetNeedDisplayUpdate() override { return unlockSys()->needProsperityDisplayUpdate; }
	virtual void SetNeedDisplayUpdate(bool value) override { unlockSys()->needProsperityDisplayUpdate = value; }


	virtual void SetupTechBoxUIs() override
	{
		UnlockSystem* unlockSys = simulation().unlockSystem(playerId());
		const std::vector<std::vector<TechEnum>>& columnToUpgradeTechEnums = unlockSys->columnToUpgradeTechEnums();


		for (int32 i = 1; i < TechScrollBox->GetChildrenCount(); i++)
		{
			UVerticalBox* ColumnBoxes = CastChecked<UVerticalBox>(TechScrollBox->GetChildAt(i));

			// Column Title
			USizeBox* ColumnTitleSizeBox = CastChecked<USizeBox>(ColumnBoxes->GetChildAt(0));
			UTextBlock* columnTitleText = CastChecked<UTextBlock>(ColumnTitleSizeBox->GetChildAt(0));

			auto setColumnTitle = [&](int32 era) {
				FText eraText = unlockSys->GetEraText(era);
				columnTitleText->SetText(FText::Format(
					NSLOCTEXT("UpgradeTreeUI", "Requires X Age", "Requires {0}"),
					eraText
				));
				AddToolTip(columnTitleText, FText::Format(
					NSLOCTEXT("UpgradeTreeUI", "UpgradeTreeText_Tip", "Unlock Upgrades in this column by unlocking {0} on the Technology Tree"),
					eraText
				));
			};

			if (i == 3 && !unlockSys->IsResearched(TechEnum::MiddleAge)) { setColumnTitle(2); }
			else if (i == 6 && !unlockSys->IsResearched(TechEnum::EnlightenmentAge)) { setColumnTitle(3); }
			else if (i == 9 && !unlockSys->IsResearched(TechEnum::IndustrialAge)) { setColumnTitle(4); }
			else {
				columnTitleText->SetText(FText());
			}


			// Column Tech List
			UVerticalBox* columnTechList = CastChecked<UVerticalBox>(ColumnBoxes->GetChildAt(1));

			SetupTechBoxColumn(columnToUpgradeTechEnums[i], columnTechList);
		}


		isInitialized = true;
	}
	
};
