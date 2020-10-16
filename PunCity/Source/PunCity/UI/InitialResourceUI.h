// Pun Dumnernchanvanit's

#pragma once

#include "PunBoxWidget.h"
#include "PunCity/Simulation/PlayerOwnedManager.h"
#include "Kismet/GameplayStatics.h"

#include "InitialResourceUI.generated.h"

/**
 * 
 */
UCLASS()
class UInitialResourceUI : public UPunWidget
{
	GENERATED_BODY()
public:
	UPROPERTY(meta = (BindWidget)) UImage* FoodIcon;
	UPROPERTY(meta = (BindWidget)) UTextBlock* FoodText;
	UPROPERTY(meta = (BindWidget)) UTextBlock* InitialFoodPriceText;
	UPROPERTY(meta = (BindWidget)) UTextBlock* FoodInventoryText;
	UPROPERTY(meta = (BindWidget)) UButton* FoodArrowUpButton;
	UPROPERTY(meta = (BindWidget)) UButton* FoodArrowDownButton;

	UPROPERTY(meta = (BindWidget)) UTextBlock* WoodPriceText;
	UPROPERTY(meta = (BindWidget)) UTextBlock* WoodInventoryText;
	UPROPERTY(meta = (BindWidget)) UButton* WoodArrowUpButton;
	UPROPERTY(meta = (BindWidget)) UButton* WoodArrowDownButton;

	UPROPERTY(meta = (BindWidget)) UTextBlock* MedicinePriceText;
	UPROPERTY(meta = (BindWidget)) UTextBlock* MedicineInventoryText;
	UPROPERTY(meta = (BindWidget)) UButton* MedicineArrowUpButton;
	UPROPERTY(meta = (BindWidget)) UButton* MedicineArrowDownButton;

	UPROPERTY(meta = (BindWidget)) UTextBlock* ToolsPriceText;
	UPROPERTY(meta = (BindWidget)) UTextBlock* ToolsInventoryText;
	UPROPERTY(meta = (BindWidget)) UButton* ToolsArrowUpButton;
	UPROPERTY(meta = (BindWidget)) UButton* ToolsArrowDownButton;

	UPROPERTY(meta = (BindWidget)) UTextBlock* StonePriceText;
	UPROPERTY(meta = (BindWidget)) UTextBlock* StoneInventoryText;
	UPROPERTY(meta = (BindWidget)) UButton* StoneArrowUpButton;
	UPROPERTY(meta = (BindWidget)) UButton* StoneArrowDownButton;

	UPROPERTY(meta = (BindWidget)) UButton* ConfirmButton;
	UPROPERTY(meta = (BindWidget)) UTextBlock* InitialMoneyText;
	UPROPERTY(meta = (BindWidget)) UTextBlock* InitialStorageSpaceText;

	UPROPERTY(meta = (BindWidget)) UOverlay* InitialResourceUI;

	static const int32 InitialStorageSpace = 12;

	void PunInit()
	{
		BUTTON_ON_CLICK(FoodArrowUpButton, this, &UInitialResourceUI::OnClickFoodArrowUpButton);
		BUTTON_ON_CLICK(FoodArrowDownButton, this, &UInitialResourceUI::OnClickFoodArrowDownButton);

		BUTTON_ON_CLICK(WoodArrowUpButton, this, &UInitialResourceUI::OnClickWoodArrowUpButton);
		BUTTON_ON_CLICK(WoodArrowDownButton, this, &UInitialResourceUI::OnClickWoodArrowDownButton);

		BUTTON_ON_CLICK(MedicineArrowUpButton, this, &UInitialResourceUI::OnClickMedicineArrowUpButton);
		BUTTON_ON_CLICK(MedicineArrowDownButton, this, &UInitialResourceUI::OnClickMedicineArrowDownButton);

		BUTTON_ON_CLICK(ToolsArrowUpButton, this, &UInitialResourceUI::OnClickToolsArrowUpButton);
		BUTTON_ON_CLICK(ToolsArrowDownButton, this, &UInitialResourceUI::OnClickToolsArrowDownButton);

		BUTTON_ON_CLICK(StoneArrowUpButton, this, &UInitialResourceUI::OnClickStoneArrowUpButton);
		BUTTON_ON_CLICK(StoneArrowDownButton, this, &UInitialResourceUI::OnClickStoneArrowDownButton);

		BUTTON_ON_CLICK(ConfirmButton, this, &UInitialResourceUI::OnClickConfirmButton);

		InitialResourceUI->SetVisibility(ESlateVisibility::Collapsed);

		AddToolTip(FoodArrowUpButton, "<Orange>Shift-click</> to increment by 100.");
		AddToolTip(FoodArrowDownButton, "<Orange>Shift-click</> to increment by 100.");

		AddToolTip(WoodArrowUpButton, "<Orange>Shift-click</> to increment by 100.");
		AddToolTip(WoodArrowDownButton, "<Orange>Shift-click</> to increment by 100.");

		AddToolTip(MedicineArrowUpButton, "<Orange>Shift-click</> to increment by 100.");
		AddToolTip(MedicineArrowDownButton, "<Orange>Shift-click</> to increment by 100.");

		AddToolTip(ToolsArrowUpButton, "<Orange>Shift-click</> to increment by 100.");
		AddToolTip(ToolsArrowDownButton, "<Orange>Shift-click</> to increment by 100.");

		AddToolTip(StoneArrowUpButton, "<Orange>Shift-click</> to increment by 100.");
		AddToolTip(StoneArrowDownButton, "<Orange>Shift-click</> to increment by 100.");
	}

	void TickUI()
	{
		if (UGameplayStatics::GetTimeSeconds(this) - lastConfirmTime < 3.0f) { // Delayed confirm
			InitialResourceUI->SetVisibility(ESlateVisibility::Collapsed);
			return;
		}
		
		auto& playerOwned = simulation().playerOwned(playerId());
		if (playerOwned.hasChosenLocation() &&
			!playerOwned.hasChosenInitialResources() &&
			!playerOwned.hasTownhall())
		{
			// Opening UI
			if (InitialResourceUI->GetVisibility() == ESlateVisibility::Collapsed) {
				initialResources = FChooseInitialResources::GetDefault();
			}
			InitialResourceUI->SetVisibility(ESlateVisibility::Visible);

			// Food
			std::vector<int32> provincesClaimed = playerOwned.provincesClaimed();
			PUN_CHECK(provincesClaimed.size() > 0);
			if (simulation().GetBiomeProvince(provincesClaimed[0]) == BiomeEnum::Jungle) {
				FoodIcon->SetBrushFromMaterial(assetLoader()->GetResourceIconMaterial(ResourceEnum::Papaya));
				SetText(FoodText, "Papaya (food)");
			}
			else {
				FoodIcon->SetBrushFromMaterial(assetLoader()->GetResourceIconMaterial(ResourceEnum::Orange));
				SetText(FoodText, "Orange (food)");
			}
			SetText(InitialFoodPriceText, std::to_string(FoodCost));
			SetText(FoodInventoryText, std::to_string(initialResources.foodAmount));

			// Wood
			SetText(WoodPriceText, std::to_string(GetResourceInfo(ResourceEnum::Wood).basePrice));
			SetText(WoodInventoryText, std::to_string(initialResources.woodAmount));

			// Stone
			SetText(StonePriceText, std::to_string(GetResourceInfo(ResourceEnum::Stone).basePrice));
			SetText(StoneInventoryText, std::to_string(initialResources.stoneAmount));

			// Medicine
			SetText(MedicinePriceText, std::to_string(GetResourceInfo(ResourceEnum::Medicine).basePrice));
			SetText(MedicineInventoryText, std::to_string(initialResources.medicineAmount));

			// Tools
			SetText(ToolsPriceText, std::to_string(GetResourceInfo(ResourceEnum::SteelTools).basePrice));
			SetText(ToolsInventoryText, std::to_string(initialResources.toolsAmount));

			int32 resourceValueIncrease = initialResources.totalCost() - FChooseInitialResources::GetDefault().totalCost();
			SetText(InitialMoneyText, std::to_string(GetMoney() - resourceValueIncrease));
			auto resourceMap = initialResources.resourceMap();
			SetText(InitialStorageSpaceText, std::to_string(StorageTilesOccupied(resourceMap)) + "/" + std::to_string(InitialStorageSpace));
			return;
		}
		
		InitialResourceUI->SetVisibility(ESlateVisibility::Collapsed);
	}

	FChooseInitialResources lastInitialResources;
	FChooseInitialResources initialResources;
	float lastConfirmTime = -1;

private:
	int32 GetMoney() { return simulation().money(playerId()); }

	bool CheckEnoughMoneyAndStorage()
	{
		int32 valueIncrease = initialResources.totalCost() - FChooseInitialResources::GetDefault().totalCost();
		if (valueIncrease > GetMoney()) {
			// Not enough money... revert the change
			initialResources = lastInitialResources;
			simulation().AddPopupToFront(playerId(), "Not enough money", ExclusiveUIEnum::InitialResourceUI, "PopupCannot");
			return false;
		}
		auto resourceMap = initialResources.resourceMap();
		if (StorageTilesOccupied(resourceMap) > InitialStorageSpace) {
			// Not enough space
			initialResources = lastInitialResources;
			simulation().AddPopupToFront(playerId(), "Not enough storage space", ExclusiveUIEnum::InitialResourceUI, "PopupCannot");
			return false;
		}
		return true;
	}
	
private:
	bool TryIncrement10(int32& amount) {
		lastInitialResources = initialResources;
		amount += 10;
		return CheckEnoughMoneyAndStorage();
	}
	void TryIncrement(int32& amount) {
		int32 loopCount = dataSource()->isShiftDown() ? 10 : 1;
		for (int32 i = 0; i < loopCount; i++) {
			if (!TryIncrement10(amount)) {
				break;
			}
		}
	}
	void TryDecrement(int32& amount)
	{
		int32 changeAmount = dataSource()->isShiftDown() ? 100 : 10;;
		amount = std::max(0, amount - changeAmount);
	}

	UFUNCTION() void OnClickFoodArrowUpButton() { TryIncrement(initialResources.foodAmount); }
	UFUNCTION() void OnClickFoodArrowDownButton() { TryDecrement(initialResources.foodAmount); }

	UFUNCTION() void OnClickWoodArrowUpButton() { TryIncrement(initialResources.woodAmount); }
	UFUNCTION() void OnClickWoodArrowDownButton() { TryDecrement(initialResources.woodAmount); }

	UFUNCTION() void OnClickMedicineArrowUpButton() { TryIncrement(initialResources.medicineAmount); }
	UFUNCTION() void OnClickMedicineArrowDownButton() { TryDecrement(initialResources.medicineAmount); }

	UFUNCTION() void OnClickToolsArrowUpButton() { TryIncrement(initialResources.toolsAmount); }
	UFUNCTION() void OnClickToolsArrowDownButton() { TryDecrement(initialResources.toolsAmount); }

	UFUNCTION() void OnClickStoneArrowUpButton() { TryIncrement(initialResources.stoneAmount); }
	UFUNCTION() void OnClickStoneArrowDownButton() { TryDecrement(initialResources.stoneAmount); }

	UFUNCTION() void OnClickConfirmButton() {
		auto command = std::make_shared<FChooseInitialResources>();
		*command = initialResources;
		networkInterface()->SendNetworkCommand(command);
		lastConfirmTime = UGameplayStatics::GetTimeSeconds(this);
	}
};