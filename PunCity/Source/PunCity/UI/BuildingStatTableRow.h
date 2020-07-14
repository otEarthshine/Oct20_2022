// Pun Dumnernchanvanit's

#pragma once

#include "CoreMinimal.h"
#include "PunCity/UI/PunWidget.h"
#include "IconTextPairWidget.h"
#include "BuildingStatTableRow.generated.h"

/**
 * 
 */
UCLASS()
class PROTOTYPECITY_API UBuildingStatTableRow : public UPunWidget
{
	GENERATED_BODY()
public:
	UPROPERTY(meta = (BindWidget)) URichTextBlock* BuildingName;
	UPROPERTY(meta = (BindWidget)) URichTextBlock* WorkForce; // Yellow denominator is for allowed < max occupants
	UPROPERTY(meta = (BindWidget)) URichTextBlock* Upkeep;
	UPROPERTY(meta = (BindWidget)) UIconTextPairWidget* Production1;
	UPROPERTY(meta = (BindWidget)) UIconTextPairWidget* Production2;
	UPROPERTY(meta = (BindWidget)) UIconTextPairWidget* Consumption1;
	UPROPERTY(meta = (BindWidget)) UIconTextPairWidget* Consumption2;
	UPROPERTY(meta = (BindWidget)) UButton * GotoButton;

	void OnInit() override {
		GotoButton->OnClicked.Clear();
		GotoButton->OnClicked.AddDynamic(this, &UBuildingStatTableRow::OnButtonDown);
	}

	void SetBuildingId(int32 buildingId) {
		_buildingId = buildingId;
	}

private:
	int32 _buildingId = -1;
	
	UFUNCTION() void OnButtonDown() {
		Building& building = simulation().building(_buildingId);
		networkInterface()->SetCameraAtom(building.centerTile().worldAtom2());
		DescriptionUIState uiState(ObjectTypeEnum::Building, _buildingId);
		uiState.shouldCloseStatUI = false;
		simulation().SetDescriptionUIState(uiState);
	}
};
