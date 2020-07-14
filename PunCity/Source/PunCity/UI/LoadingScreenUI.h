// Pun Dumnernchanvanit's

#pragma once

#include "Blueprint/UserWidget.h"
#include "Components/RichTextBlock.h"
#include "Components/Image.h"
#include "Components/VerticalBox.h"

#include "LoadingScreenUI.generated.h"

/**
 * 
 */
UCLASS()
class PROTOTYPECITY_API ULoadingScreenUI : public UUserWidget
{
	GENERATED_BODY()
public:
	UPROPERTY(meta = (BindWidget)) URichTextBlock* LoadingText;
	UPROPERTY(meta = (BindWidget)) UVerticalBox* LoadingTextBox;
	UPROPERTY(meta = (BindWidget)) UImage* Logo;
};
