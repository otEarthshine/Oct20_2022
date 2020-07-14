// Pun Dumnernchanvanit's

#pragma once

#include "PunWidget.h"
#include "PopupUI.generated.h"

/**
 * 
 */
UCLASS()
class UPopupUI : public UPunWidget
{
	GENERATED_BODY()
public:
	void PunInit();
	void Tick();
	
	void CheckChildrenPointerOnUI()
	{
		CheckPointerOnUI(PopupOverlay);
	}

	UPROPERTY(meta = (BindWidget)) UOverlay* PopupOverlay;
	UPROPERTY(meta = (BindWidget)) UPunBoxWidget* PopupBox;
	UPROPERTY(meta = (BindWidget)) UButton* PopupButton;
	UPROPERTY(meta = (BindWidget)) UTextBlock* PopupButton1Text;
	UPROPERTY(meta = (BindWidget)) UButton* PopupButton2;
	UPROPERTY(meta = (BindWidget)) UTextBlock* PopupButton2Text;
	UPROPERTY(meta = (BindWidget)) UButton* PopupButton3;
	UPROPERTY(meta = (BindWidget)) UTextBlock* PopupButton3Text;
	UPROPERTY(meta = (BindWidget)) UButton* PopupButton4;
	UPROPERTY(meta = (BindWidget)) UTextBlock* PopupButton4Text;
	UPROPERTY(meta = (BindWidget)) UButton* PopupButton5;
	UPROPERTY(meta = (BindWidget)) UTextBlock* PopupButton5Text;

private:
	UFUNCTION() void ClickPopupButton1();
	UFUNCTION() void ClickPopupButton2();
	UFUNCTION() void ClickPopupButton3();
	UFUNCTION() void ClickPopupButton4();
	UFUNCTION() void ClickPopupButton5();

	void ClickPopupButton(int32_t choiceIndex);

private:
	PopupInfo currentPopup;
};
