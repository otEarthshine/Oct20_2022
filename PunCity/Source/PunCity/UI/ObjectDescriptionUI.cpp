// Fill out your copyright notice in the Description page of Project Settings.

#include "ObjectDescriptionUI.h"
#include "PunCity/PunUtils.h"
#include "PunCity/NetworkStructs.h"
#include "../Simulation/GameSimulationCore.h"
#include "../Simulation/Buildings/TownHall.h"
#include "../Simulation/Buildings/GathererHut.h"
#include <string>
#include "PunCity/Simulation/Buildings/House.h"

using namespace std;

void UObjectDescriptionUI::Setup() 
{
	ToolCheckBox->OnCheckStateChanged.AddDynamic(this, &UObjectDescriptionUI::ToolCheckBoxChanged);
	TradeButton->OnClicked.AddDynamic(this, &UObjectDescriptionUI::ClickedTradeButton);

	ObjectDropDownBox->OnSelectionChanged.AddDynamic(this, &UObjectDescriptionUI::OnDropDownChanged);

	BuildingsStatOpener->OnClicked.AddDynamic(this, &UObjectDescriptionUI::OnClickBuildingsStatOpener);
	
	NameEditButton->OnClicked.AddDynamic(this, &UObjectDescriptionUI::OnClickNameEditButton);
	NameEditTextBox->OnTextCommitted.AddDynamic(this, &UObjectDescriptionUI::NameEditCommitted);
	
	ChooseResourceCloseButton->OnClicked.AddDynamic(this, &UObjectDescriptionUI::ClickedChooseResource);
	SearchBox->SelectAllTextWhenFocused = true;

	AllowAllButton->OnClicked.AddDynamic(this, &UObjectDescriptionUI::ClickAllowAll);
	DisallowAllButton->OnClicked.AddDynamic(this, &UObjectDescriptionUI::ClickDisallowAll);

	CloseButton->OnClicked.AddDynamic(this, &UObjectDescriptionUI::OnClickCloseButton);

	ManageStorageCloseButton->OnClicked.AddDynamic(this, &UObjectDescriptionUI::ClickCloseManageStorageOverlay);

	SetChildHUD(DescriptionPunBox);
	SetChildHUD(ChooseResourceBox);
	SetChildHUD(EditableNumberBox);

	SetChildHUD(ManageStorageBox);

	CardSlots->ClearChildren();

}

int32 modeIntFromString(const TArray<FString>& options, FString modeName) {
	for (size_t i = 0; i < options.Num(); i++) {
		if (options[i].Equals(modeName)) {
			return i;
		}
	}
	UE_DEBUG_BREAK();
	return -1;
}



void UObjectDescriptionUI::SetDropDown(int id)
{
	if (ObjectDropDownBox->GetVisibility() == ESlateVisibility::Visible) {
		return;
	}

	ObjectDropDownBox->ClearOptions();
	auto& simulation = dataSource()->simulation();
	Building& bld = simulation.building(id);

	if (bld.isEnum(CardEnum::Farm)) {
		auto seedsOwned = simulation.resourceSystem(playerId()).seedsPlantOwned();

		// Remove from drop down if georesourceEnum is invalid.
		if (!SimSettings::IsOn("GeoresourceAnywhere")) 
		{
			GeoresourceEnum georesourceEnum = simulation.georesource(bld.provinceId()).georesourceEnum;
			for (size_t i = seedsOwned.size(); i-- > 0;) {
				if (seedsOwned[i].georesourceEnum != GeoresourceEnum::None &&
					seedsOwned[i].georesourceEnum != georesourceEnum)
				{
					seedsOwned.erase(seedsOwned.begin() + i);
				}
			}
		}

		for (int i = 0; i < seedsOwned.size(); i++) {
			ObjectDropDownBox->AddOption(ToFString(GetTileObjInfo(seedsOwned[i].tileObjEnum).name));
		}
		Farm* farm = static_cast<Farm*>(&bld);
		ObjectDropDownBox->SetSelectedOption(ToFString(TreeInfos[static_cast<int>(farm->currentPlantEnum)].name));
		ObjectDropDownBox->SetVisibility(ESlateVisibility::Visible);
	}
	else if (bld.isEnum(CardEnum::Townhall))
	{
		// Need TaxAdjustment research
		if (!simulation.IsResearched(playerId(), TechEnum::TaxAdjustment)) {
			return;
		}
		
		TownHall& townhall = bld.subclass<TownHall>();
		auto& townhallPlayerOwned = simulation.playerOwned(townhall.playerId());

		// Is owner ... show normal tax
		if (townhall.playerId() == playerId())
		{
			ObjectDropDownBox->AddOption(TaxOptions[0]);
			ObjectDropDownBox->AddOption(TaxOptions[1]);
			ObjectDropDownBox->AddOption(TaxOptions[2]);
			ObjectDropDownBox->AddOption(TaxOptions[3]);
			ObjectDropDownBox->AddOption(TaxOptions[4]);

			ObjectDropDownBox->SetSelectedOption(TaxOptions[townhallPlayerOwned.taxLevel]);
			ObjectDropDownBox->SetVisibility(ESlateVisibility::Visible);
			return;
		}

		//const std::vector<int32_t>& vassalPlayerIds = townhallPlayerOwned.vassalBuildingIds();
		//for (int32 vassalPlayerId : vassalPlayerIds)
		//{
		//	if (townhall.playerId() == vassalPlayerId)
		//	{
		//		ObjectDropDownBox->AddOption(VassalTaxOptions[0]);
		//		ObjectDropDownBox->AddOption(VassalTaxOptions[1]);
		//		ObjectDropDownBox->AddOption(VassalTaxOptions[2]);
		//		ObjectDropDownBox->AddOption(VassalTaxOptions[3]);
		//		ObjectDropDownBox->AddOption(VassalTaxOptions[4]);

		//		ObjectDropDownBox->SetSelectedOption(VassalTaxOptions[townhallPlayerOwned.vassalTaxLevel]);
		//		ObjectDropDownBox->SetVisibility(ESlateVisibility::Visible);
		//		return;
		//	}
		//}
	}
}

void UObjectDescriptionUI::SetEditableNumberBox(int32 id, UPunWidget* callbackParent, CallbackEnum callbackEnum)
{
	if (EditableNumberBox->GetVisibility() == ESlateVisibility::Visible) {
		return;
	}

	auto& simulation = dataSource()->simulation();
	Building& bld = simulation.building(id);

	if (bld.isEnum(CardEnum::TradingCompany))
	{
		EditableNumberBox->Set(callbackParent, callbackEnum);
		EditableNumberBox->amount = bld.subclass<TradingCompany>().targetAmount;
		EditableNumberBox->UpdateText();
		EditableNumberBox->punId = id;
		EditableNumberBox->SetVisibility(ESlateVisibility::Visible);
	}
}

void UObjectDescriptionUI::OnDropDownChanged(FString sItem, ESelectInfo::Type seltype)
{
	if (seltype == ESelectInfo::Type::Direct) return;

	PUN_LOG("OnDropDownChanged: %s ... %d", *sItem, (int)seltype);
	if (sItem.IsEmpty()) return;

	Building& building = dataSource()->simulation().building(state.objectId);

	if (building.isEnum(CardEnum::Farm)) {
		auto command = make_shared<FChangeWorkMode>();
		command->buildingId = state.objectId;
		command->enumInt = TileObjEnumFromName(sItem);
		networkInterface()->SendNetworkCommand(command);
	}
	//else if (building.isEnum(CardEnum::RanchBarn)) 
	//{
	//	auto command = make_shared<FChangeWorkMode>();
	//	command->buildingId = state.objectId;
	//	command->enumInt = UnitEnumIntFromName(sItem);
	//	networkInterface()->SendNetworkCommand(command);
	//}
	else if (building.isEnum(CardEnum::Townhall))
	{
		TownHall& townhall = building.subclass<TownHall>();
		auto& townhallPlayerOwned = simulation().playerOwned(townhall.playerId());

		// Is owner ... show normal tax
		if (townhall.playerId() == playerId()) {
			auto command = make_shared<FChangeWorkMode>();
			command->buildingId = state.objectId;
			command->enumInt = modeIntFromString(TaxOptions, sItem);
			networkInterface()->SendNetworkCommand(command);
			return;
		}

		//const std::vector<int32_t>& vassalPlayerIds = townhallPlayerOwned.vassalPlayerIds();
		//for (int32_t vassalPlayerId : vassalPlayerIds) {
		//	if (townhall.playerId() == vassalPlayerId) {
		//		auto command = make_shared<FChangeWorkMode>();
		//		command->buildingId = state.objectId;
		//		command->enumInt = modeIntFromString(VassalTaxOptions, sItem);
		//		networkInterface()->SendNetworkCommand(command);
		//		return;
		//	}
		//}
	}
}

void UObjectDescriptionUI::ToolCheckBoxChanged(bool active)
{
	if (InterfacesInvalid()) return;

	//PUN_DEBUG(FString(("ToolCheckBoxChanged: " + to_string(active)).c_str()));
	//UE_LOG(LogTemp, Error, TEXT("ToolCheckBoxChanged: "));
}

void UObjectDescriptionUI::ClickedTradeButton()
{
	if (InterfacesInvalid()) return;

	Building& building = dataSource()->simulation().building(state.objectId);
	PUN_CHECK(IsTradingPostLike(building.buildingEnum()));
	
	if (static_cast<TradingPost*>(&building)->CanTrade()) {
		GetPunHUD()->OpenTradeUI(state.objectId);
	}
}