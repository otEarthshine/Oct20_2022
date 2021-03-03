// Pun Dumnernchanvanit's

#pragma once

#include "CoreMinimal.h"
#include "PunCity/UI/PunWidget.h"
#include "PunTutorialLink.generated.h"

/**
 * 
 */
UCLASS()
class PROTOTYPECITY_API UPunTutorialLink : public UPunWidget
{
	GENERATED_BODY()
public:
	void OnInit() override {
		LinkButton->OnClicked.Clear();
		LinkButton->OnClicked.AddDynamic(this, &UPunTutorialLink::OnButtonDown);
	}

	UPROPERTY(meta = (BindWidget)) UButton* LinkButton;
	UPROPERTY(meta = (BindWidget)) UTextBlock* LinkText;

	
	void SetLink(TutorialLinkEnum linkEnum) {
		_linkEnum = linkEnum;
		
		switch (_linkEnum) {
#define LOCTEXT_NAMESPACE "PunTutorialLink"
#define CASE(LinkEnum, LinkString) case TutorialLinkEnum::LinkEnum: SetText(LinkText, LinkString); break;

		CASE(TutorialButton, LOCTEXT("ShowButton_Link", "(Show me the button..)"));
		CASE(CameraControl, LOCTEXT("ShowHow_Link)", "(Show me how..)"));
		CASE(Happiness, LOCTEXT("HappinessInfo_Link", "(more detail on Happiness..)"));

#undef CASE
#undef LOCTEXT_NAMESPACE
		default:
			break;
		}
	}

private:
	TutorialLinkEnum _linkEnum = TutorialLinkEnum::None;
	
	UFUNCTION() void OnButtonDown() {
		GetPunHUD()->ShowTutorialUI(_linkEnum);
	}
};
