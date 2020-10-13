// Pun Dumnernchanvanit's

#pragma once

#include "PunBoxWidget.h"
#include "DiplomacyUI.generated.h"

/**
 * 
 */
UCLASS()
class PROTOTYPECITY_API UDiplomacyUI : public UPunWidget
{
	GENERATED_BODY()
public:

	UPROPERTY(meta = (BindWidget)) UTextBlock* PlayerNameText;
	UPROPERTY(meta = (BindWidget)) URichTextBlock* RelationshipText;
	UPROPERTY(meta = (BindWidget)) UPunBoxWidget* InteractionBox;
	UPROPERTY(meta = (BindWidget)) UButton* CloseButton;
	UPROPERTY(meta = (BindWidget)) UButton* CloseButton2;

	int32 aiPlayerId = -1;

	void PunInit()
	{
		BUTTON_ON_CLICK(CloseButton, this, &UDiplomacyUI::CloseUI);
		BUTTON_ON_CLICK(CloseButton2, this, &UDiplomacyUI::CloseUI);
		
		CloseUI();
	}

	void OpenUI(int32 playerIdIn)
	{
		aiPlayerId = playerIdIn;
	}

	UFUNCTION() void CloseUI() {
		SetVisibility(ESlateVisibility::Collapsed);
	}

	void TickUI()
	{
		auto& sim = simulation();
		
		if (IsVisible() && 
			aiPlayerId != -1 && 
			sim.IsAI(aiPlayerId))
		{
			// Name
			SetText(PlayerNameText, sim.playerName(aiPlayerId));

			// Relationship
			auto& aiPlayerSys = sim.aiPlayerSystem(aiPlayerId);
			std::stringstream ss;
			aiPlayerSys.GetAIRelationshipText(ss, playerId());
			SetText(RelationshipText, ss);

			// Interactions
			if (aiPlayerSys.shouldShow_DeclareFriendship()) 
			{
				bool isRed = sim.money(aiPlayerId) < aiPlayerSys.friendshipPrice();
				ss << "Declare Friendship\n";
				ss << "<img id=\"Coin\"/>" << TextRed(to_string(aiPlayerSys.friendshipPrice()), isRed);
				InteractionBox->AddButton2Lines(ss.str(), this, CallbackEnum::DeclareFriendship, !isRed, false);
			}

			if (aiPlayerSys.shouldShow_MarryOut())
			{
				bool isRed = sim.money(aiPlayerId) < aiPlayerSys.marryOutPrice();
				ss << "Marry out daughter or son\n";
				ss << "<img id=\"Coin\"/>" << TextRed(to_string(aiPlayerSys.marryOutPrice()), isRed);
				InteractionBox->AddButton2Lines(ss.str(), this, CallbackEnum::MarryOut, !isRed, false);
			}
		}
	}

	void CallBack1(UPunWidget* punWidgetCaller, CallbackEnum callbackEnum) override
	{
		auto command = make_shared<FGenericCommand>();
		command->callbackEnum = callbackEnum;
		command->intVar1 = static_cast<int>(aiPlayerId);

		networkInterface()->SendNetworkCommand(command);
	}
};
