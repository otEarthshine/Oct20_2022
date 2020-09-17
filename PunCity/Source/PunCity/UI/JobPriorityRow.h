// Pun Dumnernchanvanit's

#pragma once

#include "PunWidget.h"
#include "JobPriorityRow.generated.h"

/**
 * 
 */
UCLASS()
class PROTOTYPECITY_API UJobPriorityRow : public UUserWidget
{
	GENERATED_BODY()
public:
	UPROPERTY(meta = (BindWidget)) UButton* ArrowUpButton;
	UPROPERTY(meta = (BindWidget)) UButton* ArrowDownButton;
	UPROPERTY(meta = (BindWidget)) UTextBlock* JobNameText;
	UPROPERTY(meta = (BindWidget)) UButton* ArrowFastUpButton;
	UPROPERTY(meta = (BindWidget)) UButton* ArrowFastDownButton;


};
