// Pun Dumnernchanvanit's

#pragma once

#include "CoreMinimal.h"
#include "PunCity/UI/PunWidget.h"
#include "IconTextPairWidget.h"
#include "IconTextPair2Lines.generated.h"

/**
 * 
 */
UCLASS()
class UIconTextPair2Lines : public UPunWidget
{
	GENERATED_BODY()
public:
	UPROPERTY(meta = (BindWidget)) UIconTextPairWidget* IconPair1;
	UPROPERTY(meta = (BindWidget)) UIconTextPairWidget* IconPair2;
	UPROPERTY(meta = (BindWidget)) UIconTextPairWidget* IconPair3;

};
