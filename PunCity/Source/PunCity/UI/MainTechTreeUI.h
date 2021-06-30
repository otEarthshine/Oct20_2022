// Pun Dumnernchanvanit's

#pragma once

#include "CoreMinimal.h"
#include "PunCity/UI/TechTreeUI.h"
#include "MainTechTreeUI.generated.h"

/**
 * 
 */
UCLASS()
class PROTOTYPECITY_API UMainTechTreeUI : public UTechTreeUI
{
	GENERATED_BODY()
public:
	UPROPERTY(meta = (BindWidget)) UTextBlock* Title_DarkAge;
	UPROPERTY(meta = (BindWidget)) UVerticalBox* TechList_DarkAge1;
	UPROPERTY(meta = (BindWidget)) UVerticalBox* TechList_DarkAge2;

	UPROPERTY(meta = (BindWidget)) UTextBlock* Title_MiddleAge;
	UPROPERTY(meta = (BindWidget)) UVerticalBox* TechList_MiddleAge1;
	UPROPERTY(meta = (BindWidget)) UVerticalBox* TechList_MiddleAge2;
	UPROPERTY(meta = (BindWidget)) UVerticalBox* TechList_MiddleAge3;

	UPROPERTY(meta = (BindWidget)) UOverlay* TheatreLine;

	UPROPERTY(meta = (BindWidget)) UTextBlock* Title_EnlightenmentAge;
	UPROPERTY(meta = (BindWidget)) UVerticalBox* TechList_EnlightenmentAge1;
	UPROPERTY(meta = (BindWidget)) UVerticalBox* TechList_EnlightenmentAge2;
	UPROPERTY(meta = (BindWidget)) UVerticalBox* TechList_EnlightenmentAge3;

	UPROPERTY(meta = (BindWidget)) UTextBlock* Title_IndustrialAge;
	UPROPERTY(meta = (BindWidget)) UVerticalBox* TechList_IndustrialAge1;
	UPROPERTY(meta = (BindWidget)) UVerticalBox* TechList_IndustrialAge2;
	UPROPERTY(meta = (BindWidget)) UVerticalBox* TechList_IndustrialAge3;

	
	virtual UOverlay* GetTechSpecialLine(TechEnum techEnum) override {
		if (techEnum == TechEnum::Theatre) {
			return TheatreLine;
		}
		return nullptr;
	}
	
	virtual void SetupTechBoxUIs() override;

	virtual void TickUI() override
	{
		UTechTreeUI::TickUI();
		
		auto& sim = simulation();
		/*
		 * Era
		 */
		FLinearColor lockedColor(0.5, 0.5, 0.5, 1);
		FLinearColor unlockedColor(1, 1, 1, 1);

		Title_MiddleAge->SetColorAndOpacity(sim.IsResearched(playerId(), TechEnum::MiddleAge) ? unlockedColor : lockedColor);
		Title_EnlightenmentAge->SetColorAndOpacity(sim.IsResearched(playerId(), TechEnum::EnlightenmentAge) ? unlockedColor : lockedColor);
		Title_IndustrialAge->SetColorAndOpacity(sim.IsResearched(playerId(), TechEnum::IndustrialAge) ? unlockedColor : lockedColor);

	}
	
};
