// Pun Dumnernchanvanit's

#pragma once

#include "CoreMinimal.h"
#include "PunCity/UI/PunWidget.h"
#include "ArmyChooseNodeButton.generated.h"

/**
 * 
 */
UCLASS()
class PROTOTYPECITY_API UArmyChooseNodeButton : public UPunWidget
{
	GENERATED_BODY()
public:
	UPROPERTY(meta = (BindWidget)) UButton* ChooseNodeButton;
	UPROPERTY(meta = (BindWidget)) UTextBlock* NodeText;

	void OnInit() override {
		ChooseNodeButton->IsFocusable = false;

		ChooseNodeButton->OnClicked.Clear();
		ChooseNodeButton->OnClicked.AddDynamic(this, &UArmyChooseNodeButton::OnClickButton);
	}

	UPROPERTY() UPunWidget* parent;
	int32 nodeId = -1;

	void PunInit(UPunWidget* parentIn, int32 nodeIdIn)
	{
		parent = parentIn;
		nodeId = nodeIdIn;
	}

	UFUNCTION() void OnClickButton() {
		parent->CallBack2(this, CallbackEnum::None);
	}
};
