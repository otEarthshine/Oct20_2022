// Pun Dumnernchanvanit's

#pragma once

#include "PunWidget.h"
#include "WGT_ObjectFocus_TextRow_Cpp.generated.h"

/**
 * 
 */
UCLASS()
class PROTOTYPECITY_API UWGT_ObjectFocus_TextRow_Cpp : public UPunWidget
{
	GENERATED_BODY()
public:

	UPROPERTY(meta = (BindWidget)) UTextBlock* TextLeft;
	UPROPERTY(meta = (BindWidget)) UTextBlock* TextRight;
	UPROPERTY(meta = (BindWidget)) UImage* Icon;

	UPROPERTY(meta = (BindWidget)) UHorizontalBox* LeftBox;
	UPROPERTY(meta = (BindWidget)) UHorizontalBox* RightBox;

	
};
