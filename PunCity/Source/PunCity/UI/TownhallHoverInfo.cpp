// Fill out your copyright notice in the Description page of Project Settings.

#include "TownhallHoverInfo.h"
#include "Components/EditableText.h"

using namespace std;


void UTownhallHoverInfo::PunInit(int buildingId)
{
	_buildingId = buildingId;
	//CityNameEditableText->OnTextCommitted.AddDynamic(this, &UTownhallHoverInfo::ChangedCityName);

	auto townhall = simulation().building(_buildingId).subclass<TownHall>(CardEnum::Townhall);
	if (townhall.playerId() == playerId()) {
		SetText(TradeButtonText, "Set Trade Offer");
		BUTTON_ON_CLICK(TradeButton, this, &UTownhallHoverInfo::OnClickSetTradeOfferButton);
	} else {
		SetText(TradeButtonText, "Establish Trade Route");
		BUTTON_ON_CLICK(TradeButton, this, &UTownhallHoverInfo::OnClickEstablishTradeRouteButton);
	}

	BUTTON_ON_CLICK(LaborerNonPriorityButton, this, &UTownhallHoverInfo::OnClickLaborerNonPriorityButton);
	BUTTON_ON_CLICK(LaborerPriorityButton, this, &UTownhallHoverInfo::OnClickLaborerPriorityButton);
	BUTTON_ON_CLICK(LaborerArrowUp, this, &UTownhallHoverInfo::IncreaseLaborers);
	BUTTON_ON_CLICK(LaborerArrowDown, this, &UTownhallHoverInfo::DecreaseLaborers);

	BUTTON_ON_CLICK(BuilderNonPriorityButton, this, &UTownhallHoverInfo::OnClickBuilderNonPriorityButton);
	BUTTON_ON_CLICK(BuilderPriorityButton, this, &UTownhallHoverInfo::OnClickBuilderPriorityButton);
	BUTTON_ON_CLICK(BuilderArrowUp, this, &UTownhallHoverInfo::IncreaseBuilders);
	BUTTON_ON_CLICK(BuilderArrowDown, this, &UTownhallHoverInfo::DecreaseBuilders);

	BUTTON_ON_CLICK(RoadMakerNonPriorityButton, this, &UTownhallHoverInfo::OnClickRoadMakerNonPriorityButton);
	BUTTON_ON_CLICK(RoadMakerPriorityButton, this, &UTownhallHoverInfo::OnClickRoadMakerPriorityButton);
	BUTTON_ON_CLICK(RoadMakerArrowUp, this, &UTownhallHoverInfo::IncreaseRoadMakers);
	BUTTON_ON_CLICK(RoadMakerArrowDown, this, &UTownhallHoverInfo::DecreaseRoadMakers);


	LeftArmyBox->ClearChildren();
	RightArmyBox->ClearChildren();
	MilitaryButtons->ClearChildren();

	_lastPriorityInputTime = -999.0f;
	SyncState();
}

void UTownhallHoverInfo::ChangedCityName(const FText& Text, ETextCommit::Type CommitMethod)
{
	if (InterfacesInvalid()) return;

	auto command = make_shared<FChangeName>();
	command->name = Text.ToString();
	command->objectId = _buildingId;
	//PUN_LOG("ChangedCityName: %s", *command->name);
	networkInterface()->SendNetworkCommand(command);
}