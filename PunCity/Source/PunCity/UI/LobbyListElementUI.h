// Pun Dumnernchanvanit's

#pragma once

#include "PunWidget.h"
#include "PunCity/PunGameInstance.h"
#include "LobbyListElementUI.generated.h"

/**
 * 
 */
UCLASS()
class PROTOTYPECITY_API ULobbyListElementUI : public UPunWidget
{
	GENERATED_BODY()
public:
	void PunInit(UPunWidget* callbackParent) {
		_callbackParent = callbackParent;
		ElementButton->OnClicked.AddDynamic(this, &ULobbyListElementUI::OnClickButton);
	}
	
	UPROPERTY(meta = (BindWidget)) UImage* ElementActiveImage;
	UPROPERTY(meta = (BindWidget)) UButton* ElementButton;
	
	UPROPERTY(meta = (BindWidget)) UTextBlock* SessionName;
	UPROPERTY(meta = (BindWidget)) UTextBlock* PlayerCount;
	//UPROPERTY(meta = (BindWidget)) UTextBlock* Ping;

	UPROPERTY(meta = (BindWidget)) UTextBlock* DifferentVersionWarning;
	int32 buildUniqueId = -1;
	
	FOnlineSessionSearchResult sessionSearchResult;

private:
	UFUNCTION() void OnClickButton() {
		_callbackParent->CallBack2(this, CallbackEnum::LobbyListElementSelect);
	};

private:
	UPROPERTY() UPunWidget* _callbackParent = nullptr;
};
