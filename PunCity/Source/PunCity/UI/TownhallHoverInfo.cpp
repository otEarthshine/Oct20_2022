// Fill out your copyright notice in the Description page of Project Settings.

#include "TownhallHoverInfo.h"
#include "Components/EditableText.h"

using namespace std;


void UTownhallHoverInfo::PunInit(int buildingId)
{
	_buildingId = buildingId;
	//CityNameEditableText->OnTextCommitted.AddDynamic(this, &UTownhallHoverInfo::ChangedCityName);

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


	
	//LaborerNonPriorityButton->OnClicked.AddDynamic(this, &UTownhallHoverInfo::OnClickLaborerNonPriorityButton);
	//LaborerPriorityButton->OnClicked.AddDynamic(this, &UTownhallHoverInfo::OnClickLaborerPriorityButton);
	//LaborerArrowUp->OnClicked.AddDynamic(this, &UTownhallHoverInfo::IncreaseLaborers);
	//LaborerArrowDown->OnClicked.AddDynamic(this, &UTownhallHoverInfo::DecreaseLaborers);
	//
	//BuilderNonPriorityButton->OnClicked.AddDynamic(this, &UTownhallHoverInfo::OnClickBuilderNonPriorityButton);
	//BuilderPriorityButton->OnClicked.AddDynamic(this, &UTownhallHoverInfo::OnClickBuilderPriorityButton);
	//BuilderArrowUp->OnClicked.AddDynamic(this, &UTownhallHoverInfo::IncreaseBuilders);
	//BuilderArrowDown->OnClicked.AddDynamic(this, &UTownhallHoverInfo::DecreaseBuilders);

	//RoadMakerNonPriorityButton->OnClicked.AddDynamic(this, &UTownhallHoverInfo::OnClickRoadMakerNonPriorityButton);
	//RoadMakerPriorityButton->OnClicked.AddDynamic(this, &UTownhallHoverInfo::OnClickRoadMakerPriorityButton);
	//RoadMakerArrowUp->OnClicked.AddDynamic(this, &UTownhallHoverInfo::IncreaseRoadMakers);
	//RoadMakerArrowDown->OnClicked.AddDynamic(this, &UTownhallHoverInfo::DecreaseRoadMakers);

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