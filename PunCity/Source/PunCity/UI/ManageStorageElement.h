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

	UPROPERTY(meta = (BindWidget)) UButton* ExpandArrow;

	UPROPERTY(meta = (BindWidget)) UPunEditableNumberBox* TargetAmount;

	bool IsExpanded() {
		return FMath::IsNearlyEqual(ExpandArrow->GetRenderTransformAngle(), 90.0f);
	}

	void PunInit(ResourceEnum resourceEnumIn, std::string sectionNameIn, int32 buildingIdIn, ECheckBoxState checkBoxState, bool isSection)
	{
		sectionName = sectionNameIn;
		buildingId = buildingIdIn;
		uiResourceEnum = resourceEnumIn;
		
		AcceptBox->SetVisibility(ESlateVisibility::Visible);
		if (AcceptBox->OnCheckStateChanged.GetAllObjects().Num() == 0) {
			AcceptBox->OnCheckStateChanged.AddDynamic(this, &UManageStorageElement::OnCheckAllowResource);
		}

		if (ExpandArrow->OnClicked.GetAllObjects().Num() == 0) {
			ExpandArrow->OnClicked.AddDynamic(this, &UManageStorageElement::OnClickExpandArrow);
		}

		IndentationSpacer->SetVisibility(isSection ? ESlateVisibility::Collapsed : ESlateVisibility::Visible);

		AcceptBox->SetCheckedState(HasDelayOverride() ? checkStateOverride : checkBoxState);
		
		if (uiResourceEnum == ResourceEnum::Food ||
			uiResourceEnum == ResourceEnum::Luxury)
		{
			ResourceIcon->SetVisibility(ESlateVisibility::Collapsed);
			SetText(ResourceText, sectionName);

			ExpandArrow->SetVisibility(ESlateVisibility::Visible);

			if (HasDelayOverride()) {
				ExpandArrow->SetRenderTransformAngle(expandedOverride ? 90.0f : 0.0f);
			}
			else {
				auto& storage = simulation().building<StorageYard>(buildingId);
				if (uiResourceEnum == ResourceEnum::Food) {
					ExpandArrow->SetRenderTransformAngle(storage.expandedFood ? 90.0f : 0.0f);
				}
				else if (uiResourceEnum == ResourceEnum::Luxury) {
					ExpandArrow->SetRenderTransformAngle(storage.expandedLuxury ? 90.0f : 0.0f);
				}
			}
		}
		else 
		{
			ResourceIcon->SetVisibility(ESlateVisibility::Visible);
			SetResourceImage(ResourceIcon, resourceEnumIn, assetLoader());
			SetText(ResourceText, ResourceName(resourceEnumIn));

			ExpandArrow->SetVisibility(ESlateVisibility::Collapsed);
		}

		TargetAmount->SetVisibility(ESlateVisibility::Collapsed);
		SetChildHUD(TargetAmount);
		TargetAmount->OnInit();
		TargetAmount->Set(this, CallbackEnum::SetMarketTarget, buildingIdIn);
	}

	void SetTargetAmount(int32 amount)
	{
		TargetAmount->SetVisibility(ESlateVisibility::Visible);
		TargetAmount->amount = amount;
		TargetAmount->UpdateText();
	}

	void CallBack1(UPunWidget* punWidgetCaller, CallbackEnum callbackEnum) override
	{
		PUN_CHECK(callbackEnum == CallbackEnum::SetMarketTarget);
		
		auto command = std::make_shared<FSetAllowResource>();
		command->buildingId = buildingId;
		command->resourceEnum = uiResourceEnum;
		command->allowed = true;
		command->target = CastChecked<UPunEditableNumberBox>(punWidgetCaller)->amount;

		Market& market = simulation().building(buildingId).subclass<Market>(CardEnum::Market);
		market.lastUIResourceTargets[static_cast<int>(uiResourceEnum)] = command->target;

		networkInterface()->SendNetworkCommand(command);
	}

	UFUNCTION() void OnCheckAllowResource(bool active)
	{
		auto command = std::make_shared<FSetAllowResource>();
		command->buildingId = buildingId;
		command->resourceEnum = uiResourceEnum;
		command->allowed = active;

		networkInterface()->SendNetworkCommand(command);

		delayOverrideStartTime = UGameplayStatics::GetTimeSeconds(this);
		expandedOverride = IsExpanded();
		checkStateOverride = active ? ECheckBoxState::Checked : ECheckBoxState::Unchecked;
	}

	UFUNCTION() void OnClickExpandArrow()
	{
		auto command = std::make_shared<FSetAllowResource>();
		command->buildingId = buildingId;
		command->resourceEnum = uiResourceEnum;

		command->isExpansionCommand = true;
		command->expanded = !IsExpanded();
		
		networkInterface()->SendNetworkCommand(command);

		delayOverrideStartTime = UGameplayStatics::GetTimeSeconds(this);
		expandedOverride = command->expanded;
		checkStateOverride = AcceptBox->GetCheckedState();
	}

	bool HasDelayOverride() {
		return UGameplayStatics::GetTimeSeconds(this) - delayOverrideStartTime < 2.0f;
	}

public:
	int32 buildingId = -1;
	ResourceEnum uiResourceEnum = ResourceEnum::None;

	std::string sectionName;


	float delayOverrideStartTime = 0.0f;
	bool expandedOverride = false;
	ECheckBoxState checkStateOverride = ECheckBoxState::Unchecked;
};
