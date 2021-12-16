// Pun Dumnernchanvanit's

#pragma once

#include "CoreMinimal.h"
#include "PunWidget.h"
#include "ImportPlayerColorList.generated.h"

/**
 * 
 */
UCLASS()
class PROTOTYPECITY_API UImportPlayerColorList : public UPunWidget
{
	GENERATED_BODY()
public:
	UPROPERTY(meta = (BindWidget)) UWrapBox* ImportPlayerColorWrapBox;

	void ImportPlayerColors()
	{
		AIArchetypeInfo::PlayerBackgroundColors.Empty();
		AIArchetypeInfo::PlayerForegroundColors.Empty();
		
		for (int32 i = 0; i < ImportPlayerColorWrapBox->GetChildrenCount(); i++)
		{
			TArray<UImage*> images;
			FindChildrenRecursive<UImage>(CastChecked<UPanelWidget>(ImportPlayerColorWrapBox->GetChildAt(i)), images);
			check(images.Num() == 2);

			AIArchetypeInfo::PlayerBackgroundColors.Add(images[0]->ColorAndOpacity);
			AIArchetypeInfo::PlayerForegroundColors.Add(images[1]->ColorAndOpacity);
		}
	}


	
};
