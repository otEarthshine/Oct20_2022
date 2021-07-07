// Pun Dumnernchanvanit's

#pragma once

#include "PunWidget.h"
#include "WGT_ObjectFocus_Title_Cpp.generated.h"

/**
 * 
 */
UCLASS()
class PROTOTYPECITY_API UWGT_ObjectFocus_Title_Cpp : public UPunWidget
{
	GENERATED_BODY()
public:
	UPROPERTY(meta = (BindWidget)) UTextBlock* Title;
	UPROPERTY(meta = (BindWidget)) UTextBlock* Subtitle;
	UPROPERTY(meta = (BindWidget)) UImage* Image;


	UPROPERTY(meta = (BindWidget)) USizeBox* BuildingSwapArrows;
	UPROPERTY(meta = (BindWidget)) UButton* BuildingSwapArrowLeftButton;
	UPROPERTY(meta = (BindWidget)) UButton* BuildingSwapArrowRightButton;

	UPROPERTY() UPunWidget* callbackParent = nullptr;
	CallbackEnum callbackEnum = CallbackEnum::None;
	

	virtual void OnInit() override
	{
		BuildingSwapArrowLeftButton->OnClicked.AddDynamic(this, &UWGT_ObjectFocus_Title_Cpp::OnClickBuildingSwapArrowLeftButton);
		BuildingSwapArrowRightButton->OnClicked.AddDynamic(this, &UWGT_ObjectFocus_Title_Cpp::OnClickBuildingSwapArrowRightButton);
		const FText buildingSwapTip = NSLOCTEXT("ObjectDescriptionUI", "BuildingSwapArrow_Tip", "Click to switch between buildings of the same type <Orange>[TAB]</>");
		AddToolTip(BuildingSwapArrowLeftButton, buildingSwapTip);
		AddToolTip(BuildingSwapArrowRightButton, buildingSwapTip);
	}

	UFUNCTION() void OnClickBuildingSwapArrowLeftButton() {
		if (callbackParent) {
			callbackVar1 = true;
			callbackParent->CallBack1(this, callbackEnum);
		}
	}
	UFUNCTION() void OnClickBuildingSwapArrowRightButton() {
		if (callbackParent) {
			callbackVar1 = false;
			callbackParent->CallBack1(this, callbackEnum);
		}
	}
	
};
