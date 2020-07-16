// Pun Dumnernchanvanit's

#pragma once

#include "PunWidget.h"

#include "PunEditableNumberBox.h"
#include "ManageStorageElement.generated.h"

/**
 * 
 */
UCLASS()
class PROTOTYPECITY_API UManageStorageElement : public UPunWidget
{
	GENERATED_BODY()
public:
	UPROPERTY(meta = (BindWidget)) USpacer* IndentationSpacer;
	UPROPERTY(meta = (BindWidget)) UCheckBox* AcceptBox;
	
	UPROPERTY(meta = (BindWidget)) UImage* ResourceIcon;
	UPROPERTY(meta = (BindWidget)) UTextBlock* ResourceText;

	//UPROPERTY(meta = (BindWidget)) UPunEditableNumberBox* TargetAmount;

	void PunInit(ResourceEnum resourceEnumIn, std::string sectionName, int32 buildingIdIn)
	{
		buildingId = buildingIdIn;
		uiResourceEnum = resourceEnumIn;
		AcceptBox->SetVisibility(ESlateVisibility::Visible);
		if (AcceptBox->OnCheckStateChanged.GetAllObjects().Num() == 0) {
			AcceptBox->OnCheckStateChanged.AddDynamic(this, &UManageStorageElement::OnCheckAllowResource);
		}

		auto& sim = simulation();
		bool allowed = sim.building(buildingId).holder(uiResourceEnum).type != ResourceHolderType::Provider;
		AcceptBox->SetCheckedState(allowed ? ECheckBoxState::Checked : ECheckBoxState::Unchecked);

		SetResourceImage(ResourceIcon, resourceEnumIn, assetLoader());
		SetText(ResourceText, ResourceName(resourceEnumIn));

		//SetTargetBox->SetCheckedState(allowed, )
	}

	UFUNCTION() void OnCheckAllowResource(bool active)
	{
		auto command = std::make_shared<FSetAllowResource>();
		command->buildingId = buildingId;
		command->resourceEnum = uiResourceEnum;
		command->allowed = active;
		networkInterface()->SendNetworkCommand(command);
	}


public:
	int32 buildingId = -1;
	ResourceEnum uiResourceEnum = ResourceEnum::None;
};
