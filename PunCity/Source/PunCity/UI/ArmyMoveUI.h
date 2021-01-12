// Pun Dumnernchanvanit's

#pragma once

#include "CoreMinimal.h"
#include "PunCity/UI/PunWidget.h"
#include "ArmyMoveRow.h"
#include "Components/Overlay.h"
#include "ArmyChooseNodeButton.h"
#include "ArmyMoveUI.generated.h"

/**
 * 
 */
UCLASS()
class PROTOTYPECITY_API UArmyMoveUI : public UPunWidget
{
	GENERATED_BODY()
public:
	// Army Move
	bool showArmyCountUI = false;
	UPROPERTY(meta = (BindWidget)) UOverlay* ArmyMoveOverlay;
	UPROPERTY(meta = (BindWidget)) UTextBlock* ArmyMoveTitle;
	UPROPERTY(meta = (BindWidget)) UTextBlock* ArmyMoveTitle2;
	UPROPERTY(meta = (BindWidget)) UScrollBox* ArmyMoveRowBox;

	UPROPERTY(meta = (BindWidget)) UTextBlock* FromText;
	//UPROPERTY(meta = (BindWidget)) UTextBlock* ToText;

	UPROPERTY(meta = (BindWidget)) UButton* ConfirmButton;
	UPROPERTY(meta = (BindWidget)) UButton* CancelButton;

	// Choose Node
	bool showArmyChooseNode = false;
	UPROPERTY(meta = (BindWidget)) UOverlay* ArmyChooseNodeOverlay;
	UPROPERTY(meta = (BindWidget)) UTextBlock* ArmyChooseNodeTitle;
	UPROPERTY(meta = (BindWidget)) UVerticalBox* ArmyChooseNodeBox;
	UPROPERTY(meta = (BindWidget)) UButton* ArmyChooseNodeCancelButton;

	// Confirm intention
	// Later on use CallbackEnum and put this with popup???
	UPROPERTY(meta = (BindWidget)) UOverlay* ConfirmUI;
	UPROPERTY(meta = (BindWidget)) URichTextBlock* ConfirmText;
	UPROPERTY(meta = (BindWidget)) UButton* ConfirmYesButton;
	UPROPERTY(meta = (BindWidget)) UButton* ConfirmNoButton;

	std::shared_ptr<FAttack> armyCommand;

	void PunInit()
	{
		ArmyMoveRowBox->ClearChildren();
		ConfirmButton->OnClicked.AddDynamic(this, &UArmyMoveUI::OnClickConfirmButton);
		CancelButton->OnClicked.AddDynamic(this, &UArmyMoveUI::OnClickCancelButton);
		ArmyMoveOverlay->SetVisibility(ESlateVisibility::Collapsed);

		ArmyChooseNodeOverlay->SetVisibility(ESlateVisibility::Collapsed);
		ArmyChooseNodeBox->ClearChildren();
		ArmyChooseNodeCancelButton->OnClicked.AddDynamic(this, &UArmyMoveUI::OnClickCancelButton);

		ConfirmUI->SetVisibility(ESlateVisibility::Collapsed);
		ConfirmYesButton->OnClicked.AddDynamic(this, &UArmyMoveUI::OnClickYesButton);
		ConfirmNoButton->OnClicked.AddDynamic(this, &UArmyMoveUI::OnClickNoButton);

		SetVisibility(ESlateVisibility::Collapsed);
	}

	void TickUI()
	{
		
		if (showArmyChooseNode)
		{	
			CallbackEnum orderEnum = armyCommand->armyOrderEnum;
			int32 skipNode = -1;
			
			// Recall, choose a city to recall to
			// Can choose all nodes except origin
			if (orderEnum == CallbackEnum::ArmyRecall)  {
				skipNode = armyCommand->originNodeId;
			}

			if (orderEnum == CallbackEnum::ArmyConquer ||
				orderEnum == CallbackEnum::ArmyHelp ||
				orderEnum == CallbackEnum::ArmyReinforce ||
				orderEnum == CallbackEnum::ArmyLiberate)
			{
				skipNode = armyCommand->targetNodeId;
			}

			if (orderEnum == CallbackEnum::ArmyMoveBetweenNode) {
				skipNode = armyCommand->originNodeId;
			}

			std::vector<int32> armyNodeIds = simulation().GetArmyNodeIds(playerId());

			int32 rowIndex = 0;
			for (int32 nodeId : armyNodeIds) {
				if (nodeId != skipNode) {
					//auto row = GetBoxChild<UArmyChooseNodeButton>(ArmyChooseNodeBox, rowIndex, UIEnum::ArmyChooseNodeButton, true);
					//SetText(row->NodeText, simulation().townName(simulation().GetArmyNode(nodeId).originalPlayerId));
					//row->PunInit(this, nodeId);
				}
			}
			BoxAfterAdd(ArmyChooseNodeBox, rowIndex);

			// Only one choice, just choose right away
			// However, if we are moving army between city, show 1 row anyway to clarify
			if (rowIndex == 1 && orderEnum != CallbackEnum::ArmyMoveBetweenNode) {
				ArmyChoseNode(CastChecked<UArmyChooseNodeButton>(ArmyChooseNodeBox->GetChildAt(0))->nodeId);
				return;
			}
			
			if (rowIndex == 0) {
				simulation().AddPopupToFront(playerId(), "No available node.", ExclusiveUIEnum::ArmyMoveUI, "PopupCannot");
				CloseArmyMoveUI();
				return;
			}
			ArmyChooseNodeOverlay->SetVisibility(ESlateVisibility::Visible);
		}
		else {
			ArmyChooseNodeOverlay->SetVisibility(ESlateVisibility::Collapsed);
		}
		
		if (showArmyCountUI)
		{
			ArmyNode& originNode = simulation().GetArmyNode(armyCommand->originNodeId);
			ArmyNode& targetNode = simulation().GetArmyNode(armyCommand->targetNodeId);
			ArmyGroup* fromGroup = originNode.GetArmyGroup(playerId());
			ArmyGroup* toGroup = targetNode.GetArmyGroup(playerId());

			auto getTitle = [&]() -> std::string
			{
				switch (armyCommand->armyOrderEnum)
				{
				case CallbackEnum::ArmyConquer: return "Conquering " + simulation().armyNodeName(targetNode.nodeId);
				case CallbackEnum::ArmyLiberate: return "Liberating " + simulation().armyNodeName(targetNode.nodeId);
				case CallbackEnum::ArmyHelp: return "Reinforce " + simulation().playerName(armyCommand->helpPlayerId);
				case CallbackEnum::ArmyReinforce: return "Reinforce";
				case CallbackEnum::ArmyMoveBetweenNode: return "Moving army to " + simulation().armyNodeName(targetNode.nodeId);
				default:
					UE_DEBUG_BREAK();
					return "";
				}
			};
			SetText(ArmyMoveTitle, getTitle());

			SetText(ArmyMoveTitle2, "Dispatching from " + simulation().armyNodeName(originNode.nodeId) + " to " + simulation().armyNodeName(targetNode.nodeId));

			SetTextShorten(FromText, "Available Army");

			// Show ArmyMoveRows
			int32 rowIndex = 0;
			for (int32 i = TowerArmyEnumCount; i < ArmyEnumCount; i++)
			{
				PUN_CHECK(fromGroup);
				int32 troopCount = fromGroup->TroopCount(i);
				
				if (troopCount > 0) {
					//UArmyMoveRow* row = GetBoxChild<UArmyMoveRow>(ArmyMoveRowBox, rowIndex, UIEnum::ArmyMoveRow, true);
					//row->armyEnumInt = i;
					//
					//row->ArmyUnitBackground->SetBrushTintColor(PlayerColor2(fromGroup->playerId));
					//row->ArmyUnitIcon->SetBrushTintColor(PlayerColor1(fromGroup->playerId));
					//row->ArmyUnitIcon->SetBrushFromTexture(assetLoader()->GetArmyIcon(static_cast<ArmyEnum>(i)));

					//SetText(row->FromText, std::to_string(troopCount));

					//row->ArmyCount->Set(this, CallbackEnum::IncrementArmyCount);
					//row->ArmyCount->incrementMultiplier = 1;
					//row->ArmyCount->callbackVar1 = i;
				}
			}

			if (rowIndex == 0) {
				simulation().AddPopupToFront(playerId(), "No available army at " + simulation().armyNodeName(originNode.nodeId) + ".", ExclusiveUIEnum::ArmyMoveUI, "PopupCannot");
				CloseArmyMoveUI();
				return;
			}
			
			BoxAfterAdd(ArmyMoveRowBox, rowIndex);

			ArmyMoveOverlay->SetVisibility(ESlateVisibility::Visible);
		}
		else {
			ArmyMoveOverlay->SetVisibility(ESlateVisibility::Collapsed);
		}
	}

	/*
	 * Callbacks
	 */
	// Army count UI change
	void CallBack1(UPunWidget* punWidgetCaller, CallbackEnum callbackEnum) override
	{
		if (callbackEnum == CallbackEnum::IncrementArmyCount)
		{
			ArmyNode& originNode = simulation().GetArmyNode(armyCommand->originNodeId);
			ArmyGroup* fromGroup = originNode.GetArmyGroup(playerId());

			auto numberBox = CastChecked<UPunEditableNumberBox>(punWidgetCaller);
			
			// Limit the amount 
			int32 armyEnumInt = punWidgetCaller->callbackVar1;
			numberBox->amount = std::min(std::max(numberBox->amount, 0), fromGroup->TroopCount(armyEnumInt));
			numberBox->UpdateText();
		}
	}

	// Army Choose Node
	void CallBack2(UPunWidget* punWidgetCaller, CallbackEnum callbackEnum) override
	{
		ArmyChoseNode(CastChecked<UArmyChooseNodeButton>(punWidgetCaller)->nodeId);
	}

	void ArmyChoseNode(int32 nodeId)
	{
		showArmyChooseNode = false;
		ArmyChooseNodeOverlay->SetVisibility(ESlateVisibility::Collapsed);
		
		CallbackEnum orderEnum = armyCommand->armyOrderEnum;

		if (orderEnum == CallbackEnum::ArmyRecall)
		{
			armyCommand->targetNodeId = nodeId;
			networkInterface()->SendNetworkCommand(armyCommand);
			return;
		}

		if (orderEnum == CallbackEnum::ArmyConquer ||
			orderEnum == CallbackEnum::ArmyLiberate)
		{
			armyCommand->originNodeId = nodeId;
			OpenArmyCountUI();
			return;
		}

		// For help 
		if (orderEnum == CallbackEnum::ArmyHelp ||
			orderEnum == CallbackEnum::ArmyReinforce)
		{
			armyCommand->originNodeId = nodeId;
			OpenArmyCountUI();
			return;
		}

		if (orderEnum == CallbackEnum::ArmyMoveBetweenNode)
		{
			armyCommand->targetNodeId = nodeId;
			OpenArmyCountUI();
			return;
		}

		UE_DEBUG_BREAK();
	}

	/*
	 * Open/Close
	 */

	void OpenArmyMoveUI(std::shared_ptr<FAttack> armyCommandIn)
	{
		networkInterface()->ResetGameUI();
		dataSource()->Spawn2DSound("UI", "UIWindowOpen");
		
		armyCommand = armyCommandIn;
		CallbackEnum orderEnum = armyCommand->armyOrderEnum;

		// Recall: Choose node to recall to then send command...
		if (orderEnum == CallbackEnum::ArmyRecall ||
			orderEnum == CallbackEnum::ArmyMoveBetweenNode ||
			orderEnum == CallbackEnum::ArmyConquer ||
			orderEnum == CallbackEnum::ArmyHelp ||
			orderEnum == CallbackEnum::ArmyReinforce ||
			orderEnum == CallbackEnum::ArmyLiberate) 
		{
			// Special case: warn about being too far
			if (orderEnum == CallbackEnum::ArmyConquer ||
				orderEnum == CallbackEnum::ArmyHelp ||
				orderEnum == CallbackEnum::ArmyLiberate)
			{
				ArmyNode& targetNode = simulation().GetArmyNode(armyCommand->targetNodeId);
				int32 attackDelayPenalty = targetNode.PlayerAttackDelayPenaltyPercent(playerId());

				if (attackDelayPenalty > 0)
				{
					ConfirmUI->SetVisibility(ESlateVisibility::Visible);
					int32 attackSpeedPenalty = 100 - 100 * 100 / (100 + attackDelayPenalty);
					SetText(ConfirmText,  simulation().armyNodeName(targetNode.nodeId) + 
						" is very far from cities and colonies you own. As a result, your army will incur an attack speed penalty of " + std::to_string(attackSpeedPenalty) + "%. "+
						"Will you still execute the move?");

					SetVisibility(ESlateVisibility::SelfHitTestInvisible);
					return;
				}
			}
			
			showArmyChooseNode = true;
			SetVisibility(ESlateVisibility::SelfHitTestInvisible);
			
			TickUI();
			return;
		}

		UE_DEBUG_BREAK();
	}

	void OpenArmyCountUI()
	{
		GetArmyCountsAndClear();
		showArmyCountUI = true;
	}

	void CloseArmyMoveUI()
	{
		if (ArmyMoveOverlay->GetVisibility() != ESlateVisibility::Collapsed ||
			ArmyChooseNodeOverlay->GetVisibility() != ESlateVisibility::Collapsed)
		{
			dataSource()->Spawn2DSound("UI", "UIWindowClose");
		}
		showArmyChooseNode = false;
		showArmyCountUI = false;
		ArmyMoveOverlay->SetVisibility(ESlateVisibility::Collapsed);
		ArmyChooseNodeOverlay->SetVisibility(ESlateVisibility::Collapsed);
		SetVisibility(ESlateVisibility::Collapsed);
	}

	UFUNCTION() void OnClickConfirmButton()
	{
		// Pun info from rows into armyCommand and send it off...
		TArray<int32> armyCounts = GetArmyCountsAndClear();

		if (CppUtils::Sum(armyCounts) == 0) {
			simulation().AddPopupToFront(playerId(), "Please assign some units before confirming.", ExclusiveUIEnum::ArmyMoveUI, "PopupCannot");
			return;
		}
		
		armyCommand->armyCounts = armyCounts;
		
		networkInterface()->SendNetworkCommand(armyCommand);
		CloseArmyMoveUI();
	}

	UFUNCTION() void OnClickCancelButton()
	{
		GetArmyCountsAndClear();
		CloseArmyMoveUI();
	}

	// Confirm in case the attack could incur penalty
	UFUNCTION() void OnClickYesButton() {
		ConfirmUI->SetVisibility(ESlateVisibility::Collapsed);
		
		showArmyChooseNode = true;
		TickUI();
	}
	UFUNCTION() void OnClickNoButton() {
		ConfirmUI->SetVisibility(ESlateVisibility::Collapsed);
		CloseArmyMoveUI();
	}
	

	TArray<int32> GetArmyCountsAndClear()
	{
		TArray<int32> armyCounts;
		armyCounts.SetNumZeroed(ArmyEnumCount);
		TArray<UWidget*> rows = ArmyMoveRowBox->GetAllChildren();
		for (UWidget* rowWidget : rows) 
		{
			if (rowWidget->GetVisibility() != ESlateVisibility::Collapsed) 
			{
				auto row = CastChecked<UArmyMoveRow>(rowWidget);
				armyCounts[row->armyEnumInt] = row->ArmyCount->amount;
				row->ArmyCount->amount = 0;
				row->ArmyCount->UpdateText();
			}
		}
		return armyCounts;
	}
};
