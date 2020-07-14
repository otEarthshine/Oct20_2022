// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "PunCity/UI/PunWidget.h"
#include "PunCity/Simulation/UnlockSystem.h"

#include "TechBoxUI.h"

#include "TechTreeUI.generated.h"

/**
 * 
 */
UCLASS()
class UTechTreeUI : public UPunWidget
{
	GENERATED_BODY()
public:
	void PunInit()
	{
		AddColumn(1, { TechBox1_1, TechBox1_2 , TechBox1_3 });
		AddColumn(2, { TechBox2_1, TechBox2_2 , TechBox2_3, TechBox2_4, TechBox2_5 , TechBox2_6 });
		AddColumn(3, { TechBox3_1, TechBox3_2 , TechBox3_3, TechBox3_4, TechBox3_5 , TechBox3_6,  TechBox3_7 });
		AddColumn(4, { TechBox4_1, TechBox4_2 , TechBox4_3, TechBox4_4, TechBox4_5 , TechBox4_6,  TechBox4_7 });
		AddColumn(5, { TechBox5_1, TechBox5_2 , TechBox5_3, TechBox5_4, TechBox5_5 , TechBox5_6,  TechBox5_7 });
		AddColumn(6, { TechBox6_1, TechBox6_2 , TechBox6_3, TechBox6_4, TechBox6_5 , TechBox6_6,  TechBox6_7 });

		isInitialized = false;
		SetVisibility(ESlateVisibility::Collapsed);

		CloseButton->OnClicked.AddDynamic(this, &UTechTreeUI::CloseUI);
		CloseButton2->OnClicked.AddDynamic(this, &UTechTreeUI::CloseUI);

		TechScrollSizeBox->SetWidthOverride(1200);
		TechScrollBox->SetScrollbarThickness(FVector2D(10, 10));
	}

	void SetShowUI(bool show) {
		if (show) {
			networkInterface()->ResetGameUI();

			dataSource()->Spawn2DSound("UI", "UIWindowOpen");
		} else {
			dataSource()->Spawn2DSound("UI", "UIWindowClose");
		}
		
		SetVisibility(show ? ESlateVisibility::SelfHitTestInvisible : ESlateVisibility::Collapsed);
		TickUI();
	}

	void TickUI();

	//UTechBoxUI* GetTechBoxAtALocation(TechBoxLocation location) {
	//	return techBoxLocationToTechBox[location];
	//}

public:
	UPROPERTY(meta = (BindWidget)) USizeBox* TechScrollSizeBox;
	UPROPERTY(meta = (BindWidget)) UScrollBox* TechScrollBox;

	UPROPERTY(meta = (BindWidget)) UTechBoxUI* TechBox1_1;
	UPROPERTY(meta = (BindWidget)) UTechBoxUI* TechBox1_2;
	UPROPERTY(meta = (BindWidget)) UTechBoxUI* TechBox1_3;

	UPROPERTY(meta = (BindWidget)) UTechBoxUI* TechBox2_1;
	UPROPERTY(meta = (BindWidget)) UTechBoxUI* TechBox2_2;
	UPROPERTY(meta = (BindWidget)) UTechBoxUI* TechBox2_3;
	UPROPERTY(meta = (BindWidget)) UTechBoxUI* TechBox2_4;
	UPROPERTY(meta = (BindWidget)) UTechBoxUI* TechBox2_5;
	UPROPERTY(meta = (BindWidget)) UTechBoxUI* TechBox2_6;

	UPROPERTY(meta = (BindWidget)) UTechBoxUI* TechBox3_1;
	UPROPERTY(meta = (BindWidget)) UTechBoxUI* TechBox3_2;
	UPROPERTY(meta = (BindWidget)) UTechBoxUI* TechBox3_3;
	UPROPERTY(meta = (BindWidget)) UTechBoxUI* TechBox3_4;
	UPROPERTY(meta = (BindWidget)) UTechBoxUI* TechBox3_5;
	UPROPERTY(meta = (BindWidget)) UTechBoxUI* TechBox3_6;
	UPROPERTY(meta = (BindWidget)) UTechBoxUI* TechBox3_7;

	UPROPERTY(meta = (BindWidget)) UTechBoxUI* TechBox4_1;
	UPROPERTY(meta = (BindWidget)) UTechBoxUI* TechBox4_2;
	UPROPERTY(meta = (BindWidget)) UTechBoxUI* TechBox4_3;
	UPROPERTY(meta = (BindWidget)) UTechBoxUI* TechBox4_4;
	UPROPERTY(meta = (BindWidget)) UTechBoxUI* TechBox4_5;
	UPROPERTY(meta = (BindWidget)) UTechBoxUI* TechBox4_6;
	UPROPERTY(meta = (BindWidget)) UTechBoxUI* TechBox4_7;

	UPROPERTY(meta = (BindWidget)) UTechBoxUI* TechBox5_1;
	UPROPERTY(meta = (BindWidget)) UTechBoxUI* TechBox5_2;
	UPROPERTY(meta = (BindWidget)) UTechBoxUI* TechBox5_3;
	UPROPERTY(meta = (BindWidget)) UTechBoxUI* TechBox5_4;
	UPROPERTY(meta = (BindWidget)) UTechBoxUI* TechBox5_5;
	UPROPERTY(meta = (BindWidget)) UTechBoxUI* TechBox5_6;
	UPROPERTY(meta = (BindWidget)) UTechBoxUI* TechBox5_7;

	UPROPERTY(meta = (BindWidget)) UTechBoxUI* TechBox6_1;
	UPROPERTY(meta = (BindWidget)) UTechBoxUI* TechBox6_2;
	UPROPERTY(meta = (BindWidget)) UTechBoxUI* TechBox6_3;
	UPROPERTY(meta = (BindWidget)) UTechBoxUI* TechBox6_4;
	UPROPERTY(meta = (BindWidget)) UTechBoxUI* TechBox6_5;
	UPROPERTY(meta = (BindWidget)) UTechBoxUI* TechBox6_6;
	UPROPERTY(meta = (BindWidget)) UTechBoxUI* TechBox6_7;

	UPROPERTY(meta = (BindWidget)) UButton* CloseButton;
	UPROPERTY(meta = (BindWidget)) UButton* CloseButton2;

	UPROPERTY() TMap<int32, UTechBoxUI*> techBoxLocationToTechBox;

	bool isInitialized = false;

private:
	// Use ResearchInfo's TechBoxLocation to link the techEnum to proper TechBoxUI
	void SetupTechBoxUIs();

	void CallBack1(UPunWidget* punWidgetCaller, CallbackEnum callBackEnum) final;

	void AddColumn(int32_t column, TArray<UTechBoxUI*> techBoxes) {
		for (int32 i = 0; i < techBoxes.Num(); i++) {
			techBoxLocationToTechBox.Add(TechBoxLocation(column, i + 1).id(), techBoxes[i]);
		}
	}

	UFUNCTION() void CloseUI();

private:
	std::vector<std::shared_ptr<ResearchInfo>> _lastTechQueue;
};
