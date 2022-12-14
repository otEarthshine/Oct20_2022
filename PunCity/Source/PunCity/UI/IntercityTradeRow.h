// Pun Dumnernchanvanit's

#pragma once

#include "PunCity/UI/PunWidget.h"
#include "IconTextPairWidget.h"

#include "PunEditableNumberBox.h"
#include "TradeRowBase.h"

#include "IntercityTradeRow.generated.h"

/**
 * 
 */
UCLASS()
class PROTOTYPECITY_API UIntercityTradeRow : public UTradeRowBase
{
	GENERATED_BODY()
public:
	
	UPROPERTY(meta = (BindWidget)) UComboBoxString* OfferTypeDropdown;
	UPROPERTY(meta = (BindWidget)) UPunEditableNumberBox* TargetInventory;


	void Init(class UIntercityTradeUI* intercityTradeUI, ResourceEnum resourceEnumIn)
	{
		_intercityTradeUI = intercityTradeUI;
		_resourceEnum = resourceEnumIn;

		OfferTypeDropdown->OnSelectionChanged.RemoveAll(this);
		OfferTypeDropdown->OnSelectionChanged.AddDynamic(this, &UIntercityTradeRow::OnOfferTypeDropdownChanged);

		if (OfferTypeDropdown->GetOptionCount() == 0) {
			for (std::string name : IntercityTradeOfferEnumName) {
				OfferTypeDropdown->AddOption(ToFString(name));
			}
		}

		SetChildHUD(TargetInventory);

		IntercityTradeOffer offer = dataSource()->simulation().worldTradeSystem().GetIntercityTradeOffer(playerId(), _resourceEnum);
		OfferTypeDropdown->SetSelectedOption(ToFString(GetIntercityTradeOfferEnumName(offer.offerEnum)));
		
		TargetInventory->Set(this, CallbackEnum::None);
		TargetInventory->amount = offer.targetInventory;
		TargetInventory->UpdateText();

		UpdateTexts();
	}

	void UpdateTexts()
	{
		UpdateTextsBase();

		FString inventoryCount = FString::FromInt(simulation().resourceCount(playerId(), _resourceEnum));

		SetFString(InventoryText, inventoryCount);
	}

	IntercityTradeOfferEnum GetOfferEnum() {
		std::string name = ToStdString(OfferTypeDropdown->GetSelectedOption());
		return GetIntercityTradeOfferEnumFromName(name);
	}



	// Callback for PunEditableNumberBox
	void CallBack1(UPunWidget* punWidgetCaller, CallbackEnum callBackEnum) final
	{
		
	}

	UFUNCTION() void OnOfferTypeDropdownChanged(FString sItem, ESelectInfo::Type seltype)
	{
		PUN_LOG("OnOfferTypeDropdownChanged %s", *sItem);
	}

private:
	
	UIntercityTradeUI* _intercityTradeUI;
};
