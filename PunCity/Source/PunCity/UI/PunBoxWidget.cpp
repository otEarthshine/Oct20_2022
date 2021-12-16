// Fill out your copyright notice in the Description page of Project Settings.


#include "PunBoxWidget.h"

#define LOCTEXT_NAMESPACE "PunBoxWidget"

void UPunBoxWidget::AddBuildingTooltip(UWidget* widget, CardEnum buildingEnum, UPunWidget* punWidgetSupport, bool isPermanent)
{
	// TODO: why this not work?

	BldInfo info = GetBuildingInfo(buildingEnum);

	// Tooltip
	UToolTipWidgetBase* tooltip = AddToolTip(widget, punWidgetSupport);

	auto tooltipBox = tooltip->TooltipPunBoxWidget;
	tooltipBox->AfterAdd();

	tooltipBox->AddRichText(TEXT_TAG("<TipHeader>", info.GetName()));
	if (buildingEnum == CardEnum::DirtRoad) {
		tooltipBox->AddRichText(INVTEXT("<Orange>[Z]</>"));
	}


	//! Description
	tooltipBox->AddSpacer();
	
	if (IsMilitaryCardEnum(buildingEnum)) {
		tooltipBox->AddRichTextParsed(GetMilitaryInfoDescription(buildingEnum));
	}
	else {
		tooltipBox->AddRichTextWrap(info.GetDescription());
	}
	
	tooltipBox->AddLineSpacer(12);

	// Card type
	if (IsIndustrialBuilding(buildingEnum)) {
		tooltipBox->AddRichText(LOCTEXT("Type: Industry", "Type: Industry"));
	}
	else if (IsAgricultureBuilding(buildingEnum)) {
		tooltipBox->AddRichText(LOCTEXT("Type: Agriculture", "Type: Agriculture"));
	}
	else if (IsMountainMine(buildingEnum)) {
		tooltipBox->AddRichText(LOCTEXT("Type: Mine", "Type: Mine"));
	}
	else if (IsServiceBuilding(buildingEnum)) {
		tooltipBox->AddRichText(LOCTEXT("Type: Service", "Type: Service"));
	}
	else if (IsSpecialProducer(buildingEnum)) {
		tooltipBox->AddRichText(LOCTEXT("Type: Special", "Type: Special"));
	}
	else if (IsTownSlotCard(buildingEnum)) {
		tooltipBox->AddRichText(LOCTEXT("Type: Town-Slot", "Type: Town-Slot"));
	}
	else if (IsBuildingSlotCard(buildingEnum)) {
		tooltipBox->AddRichText(LOCTEXT("Type: Building-Slot", "Type: Building-Slot"));
	}
	else if (IsActionCard(buildingEnum)) {
		tooltipBox->AddRichText(LOCTEXT("Type: Action", "Type: Action"));
	}
	else if (IsMilitaryCardEnum(buildingEnum)) {
		tooltipBox->AddRichText(LOCTEXT("Type: Military", "Type: Military"));
	}
	//else {
	//	UE_DEBUG_BREAK();
	//}

	auto& sim = punWidgetSupport->dataSource()->simulation();
	auto& cardSys = sim.cardSystem(punWidgetSupport->playerId());
	
	ResourceEnum tokenEnum = cardSys.GetCardPriceTokenEnum(buildingEnum);
	int32 cardPrice = cardSys.GetCardPrice(buildingEnum, tokenEnum);

	if (cardPrice > 0) {
		const FText cardPriceText = LOCTEXT("Card price:", "Card price:");
		tooltipBox->AddRichText(FText::Format(
			INVTEXT("{0} {1}{2}"), 
			cardPriceText,
			sim.GetTokenIconRichText(tokenEnum),
			TEXT_NUM(cardPrice))
		);
	}


	if (IsBuildingCard(buildingEnum))
	{
		int32 upkeep = Building::BaseUpkeep(buildingEnum);
		int32 workerCount = GetBuildingInfo(buildingEnum).workerCount;
		if (upkeep > 0 || workerCount > 0)
		{
			tooltipBox->AddSpacer(8);
			if (upkeep > 0) {
				tooltipBox->AddRichText(FText::Format(LOCTEXT("Upkeep Tip", "Upkeep: <img id=\"Coin\"/>{0}<Gray>(base)</>"), TEXT_NUM(upkeep)));
			}
			if (workerCount > 0) {
				const FText workerCountText = LOCTEXT("Workers:", "Workers:");
				tooltipBox->AddRichText(FText::Format(INVTEXT("{0} {1}"), workerCountText, TEXT_NUM(workerCount)));
			}
		}
		
		if (!isPermanent) {
			tooltipBox->AddLineSpacer(12);
		}

		tooltipBox->AddRichText(TEXT_TAG("<Subheader>", LOCTEXT("Required resources:", "Required resources:")));

		if (buildingEnum == CardEnum::Farm)
		{
			tooltipBox->AddIconPair(INVTEXT(" "), ResourceEnum::Wood, LOCTEXT("FarmWoodCost", " 1/2 x Area"));
		}
		else if (buildingEnum == CardEnum::StorageYard)
		{
			tooltipBox->AddIconPair(INVTEXT(" "), 
				sim.playerFactionEnum(punWidgetSupport->playerId()) == FactionEnum::Arab ? ResourceEnum::Clay : ResourceEnum::Wood, 
				LOCTEXT("StorageWoodCost", " 2 x Storage Space")
			);
		}
		else
		{
			FactionEnum factionEnum = punWidgetSupport->simulation().playerOwned(punWidgetSupport->playerId()).factionEnum();
			auto resourcesNeeded = GetBuildingInfo(buildingEnum).GetConstructionResources(factionEnum);
			bool needResource = false;
			for (int i = 0; i < resourcesNeeded.size(); i++) {
				if (resourcesNeeded[i] > 0) {
					tooltipBox->AddIconPair(INVTEXT(" "), ConstructionResources[i], TEXT_NUM(resourcesNeeded[i]));
					needResource = true;
				}
			}
			if (!needResource) {
				tooltipBox->AddRichText(LOCTEXT(" none", " none"));
			}
		}
	}
}


#undef LOCTEXT_NAMESPACE