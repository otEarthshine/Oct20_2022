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

	UPROPERTY(meta = (BindWidget)) UTextBlock* PlayerNameText;
	UPROPERTY(meta = (BindWidget)) URichTextBlock* RelationshipText;
	UPROPERTY(meta = (BindWidget)) UPunBoxWidget* InteractionBox;
	UPROPERTY(meta = (BindWidget)) UButton* CloseButton;
	UPROPERTY(meta = (BindWidget)) UButton* CloseButton2;

	int32 aiPlayerId = -1;

	void PunInit()
	{
		BUTTON_ON_CLICK(CloseButton, this, &UDiplomacyUI::CloseUI);
		BUTTON_ON_CLICK(CloseButton2, this, &UDiplomacyUI::CloseUI);

		SetChildHUD(InteractionBox);
		
		CloseUI();
	}

	void OpenUI(int32 playerIdIn)
	{
		aiPlayerId = playerIdIn;
		SetVisibility(ESlateVisibility::SelfHitTestInvisible);
	}

	UFUNCTION() void CloseUI() {
		SetVisibility(ESlateVisibility::Collapsed);
	}

	void TickUI();

	void CallBack1(UPunWidget* punWidgetCaller, CallbackEnum callbackEnum) override
	{
		auto command = make_shared<FGenericCommand>();
		command->callbackEnum = callbackEnum;
		command->intVar1 = static_cast<int>(aiPlayerId);

		networkInterface()->SendNetworkCommand(command);
	}
};
