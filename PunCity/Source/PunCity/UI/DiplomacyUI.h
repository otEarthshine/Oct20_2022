// Pun Dumnernchanvanit's

#pragma once

#include "PunBoxWidget.h"
#include "DiplomacyUI.generated.h"

/**
 * 
 */
UCLASS()
class PROTOTYPECITY_API UDiplomacyUI : public UPunWidget
{
	GENERATED_BODY()
public:
	UPROPERTY(meta = (BindWidget)) UImage* LogoPreviewImage;
	UPROPERTY(meta = (BindWidget)) UImage* CharacterPreviewImage;

	UPROPERTY(meta = (BindWidget)) UTextBlock* PlayerNameText;
	UPROPERTY(meta = (BindWidget)) URichTextBlock* RelationshipText;
	UPROPERTY(meta = (BindWidget)) UPunBoxWidget* InteractionBox;
	UPROPERTY(meta = (BindWidget)) UButton* CloseButton;
	UPROPERTY(meta = (BindWidget)) UWGT_ButtonCpp* CloseXButton;

	int32 targetTownId = -1;

	void PunInit()
	{
		BUTTON_ON_CLICK(CloseButton, this, &UDiplomacyUI::CloseUI);
		BUTTON_ON_CLICK(CloseXButton->CoreButton, this, &UDiplomacyUI::CloseUI);

		SetChildHUD(InteractionBox);
		
		CloseUI();
	}

	void OpenDiplomacyUI(int32 targetTownIdIn)
	{
		targetTownId = targetTownIdIn;
		SetVisibility(ESlateVisibility::SelfHitTestInvisible);
	}

	UFUNCTION() void CloseUI() {
		SetVisibility(ESlateVisibility::Collapsed);
	}

	void TickUI();

	virtual void CallBack1(UPunWidget* punWidgetCaller, CallbackEnum callbackEnum) override
	{
		auto command = make_shared<FGenericCommand>();
		command->callbackEnum = callbackEnum;
		command->intVar1 = static_cast<int>(targetTownId);

		networkInterface()->SendNetworkCommand(command);
	}
};
