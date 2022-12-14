// Pun Dumnernchanvanit's

#pragma once

#include "QuestScreenElement.h"
#include "PunCity/UI/PunBoxWidget.h"
#include "PlayerCompareElement.h"
#include "PlayerCompareDetailedElement.h"
#include "Components/WidgetSwitcher.h"
#include "QuestUI.generated.h"

/**
 * 
 */
UCLASS()
class UQuestUI : public UPunWidget
{
	GENERATED_BODY()
public:
	UPROPERTY(meta = (BindWidget)) UOverlay* QuestOverlay;
	UPROPERTY(meta = (BindWidget)) UVerticalBox* QuestBox;

	UPROPERTY(meta = (BindWidget)) UOverlay* QuestDescriptionOverlay;
	UPROPERTY(meta = (BindWidget)) UPunBoxWidget* QuestDescriptionBox;
	UPROPERTY(meta = (BindWidget)) UButton* QuestDescriptionCloseButton;

	UPROPERTY(meta = (BindWidget)) UOverlay* PlayerCompareOverlay;
	UPROPERTY(meta = (BindWidget)) UOverlay* PlayerCompareInnerOverlay;
	UPROPERTY(meta = (BindWidget)) UVerticalBox* PlayerCompareBox;
	
	void CheckChildrenPointerOnUI() {
		CheckPointerOnUI(QuestOverlay);
		CheckPointerOnUI(PlayerDetailsOverlay);
		CheckPointerOnUI(PlayerCompareInnerOverlay);
	}

	void PunInit() {
		QuestBox->ClearChildren();

		QuestDescriptionOverlay->SetVisibility(ESlateVisibility::Collapsed);
		QuestDescriptionCloseButton->OnClicked.AddDynamic(this, &UQuestUI::OnQuestDescriptionCloseButtonClick);

		SetChildHUD(QuestDescriptionBox);

		PunInit_PlayerDetails();
	}

	void Tick()
	{
#if !UI_QUEST
		return;
#endif
		if (!PunSettings::IsOn("UIQuest")) {
			return;
		}
		
		QuestSystem* questSystem = dataSource()->simulation().questSystem(playerId());
		if (questSystem) 
		{
			/*
			 * Quest List
			 */
			const auto& quests = questSystem->quests();
			QuestOverlay->SetVisibility(quests.size() > 0 ? ESlateVisibility::SelfHitTestInvisible : ESlateVisibility::Collapsed);

			//size_t showCount = std::min(quests.size(), static_cast<size_t>(3));
			size_t shownIndex = 0;

			auto showQuest = [&](std::shared_ptr<Quest> quest)
			{
				if (shownIndex >= QuestBox->GetChildrenCount()) {
					auto widget = AddWidget<UQuestScreenElement>(UIEnum::QuestElement);
					QuestBox->AddChild(widget);
				}

				auto questElement = CastChecked<UQuestScreenElement>(QuestBox->GetChildAt(shownIndex));
				questElement->Setup(quest, this, CallbackEnum::QuestOpenDescription);
				questElement->SetVisibility(ESlateVisibility::Visible);

				shownIndex++;
			};

			// Show important quests first
			for (size_t i = 0; i < quests.size(); i++) {
				if (IsImportantQuest(quests[i]->classEnum())) 	{
					showQuest(quests[i]);
				}
			}
			for (size_t i = 0; i < quests.size(); i++) {
				if (!IsImportantQuest(quests[i]->classEnum())) {
					showQuest(quests[i]);
				}
			}
			
			for (size_t i = shownIndex; i < QuestBox->GetChildrenCount(); i++) {
				QuestBox->GetChildAt(i)->SetVisibility(ESlateVisibility::Collapsed);
			}

			/*
			 * Quest Description
			 */
			if (QuestDescriptionOverlay->GetVisibility() != ESlateVisibility::Collapsed)
			{
				auto quest = questSystem->GetQuest(questEnumToDisplay);
				if (quest)
				{
					QuestDescriptionBox->AfterAdd();

					QuestDescriptionBox->AddRichText("<Subheader>" + quest->questTitle() + "</>");
					QuestDescriptionBox->AddLineSpacer();
					if (quest->numberDescription() != "") {
						QuestDescriptionBox->AddRichText("Progress: " + quest->numberDescription());
						QuestDescriptionBox->AddSpacer(10);
					}
					QuestDescriptionBox->AddRichTextParsed(quest->questDescription());
				}
			}
			
		}

		/*
		 * Player Compare
		 */
		if (simulation().HasTownhall(playerId()))
		{
			std::vector<int32> playerIds = GetPlayersSortedByPopulation();

			for (int32 i = 0; i < playerIds.size(); i++)
			{
				int32 curId = playerIds[i];
				if (i >= PlayerCompareBox->GetChildrenCount()) {
					PlayerCompareBox->AddChild(AddWidget<UPlayerCompareElement>(UIEnum::PlayerCompareElement));
				}

				int32 pop = simulation().population(curId);
				FString name = dataSource()->playerNameF(curId);

				auto element = CastChecked<UPlayerCompareElement>(PlayerCompareBox->GetChildAt(i));
				if (element->PlayerNameText->GetText().ToString() != name ||
					element->PopulationText->GetText().ToString() != FString::FromInt(pop))
				{
					SetChildHUD(element);
					element->PunInit(curId);

					auto setHomeTownIcon = [&]() {
						element->HomeTownIcon->SetVisibility(ESlateVisibility::SelfHitTestInvisible);
						element->HomeTownIcon->SetRenderOpacity(gameInstance()->IsPlayerConnected(curId) ? 1.0f : 0.2f);
					};

					bool isPlayer = (curId == playerId());
					if (isPlayer) {
						SetTextF(element->PlayerNameBoldText, TrimStringF_Dots(name, 17));
						SetText(element->PopulationBoldText, std::to_string(pop));
						setHomeTownIcon();

						AddToolTip(element->PlayerZoomButton, "Click to travel home <Orange>[H]</>");
					}
					else {
						SetTextF(element->PlayerNameText, TrimStringF_Dots(name, 17));
						SetText(element->PopulationText, std::to_string(pop));
						setHomeTownIcon();

						std::string nameStd = ToStdString(name);
						AddToolTip(element->PlayerZoomButton, "Click to travel to " + nameStd);
					}
					element->PlayerNameBoldText->SetVisibility(isPlayer ? ESlateVisibility::SelfHitTestInvisible : ESlateVisibility::Collapsed);
					element->PopulationBoldText->SetVisibility(isPlayer ? ESlateVisibility::SelfHitTestInvisible : ESlateVisibility::Collapsed);
					element->PlayerNameText->SetVisibility(isPlayer ? ESlateVisibility::Collapsed : ESlateVisibility::SelfHitTestInvisible);
					element->PopulationText->SetVisibility(isPlayer ? ESlateVisibility::Collapsed : ESlateVisibility::SelfHitTestInvisible);
				}

				// Element visible if it has a valid name
				element->SetVisibility(name.Len() > 0 ? ESlateVisibility::Visible : ESlateVisibility::Collapsed);
			}

			for (size_t i = playerIds.size(); i < PlayerCompareBox->GetChildrenCount(); i++) {
				PlayerCompareBox->GetChildAt(i)->SetVisibility(ESlateVisibility::Collapsed);
			}
			
			PlayerCompareOverlay->SetVisibility(ESlateVisibility::SelfHitTestInvisible);
		}
		else
		{
			PlayerCompareOverlay->SetVisibility(ESlateVisibility::Hidden);
		}


		TickPlayerDetails();
	}

	void CallBack1(UPunWidget* punWidgetCaller, CallbackEnum callbackEnum) override
	{
		bool wasQuestDescVisible = QuestDescriptionOverlay->GetVisibility() == ESlateVisibility::Visible;
		
		networkInterface()->ResetGameUI();
		
		PUN_CHECK(callbackEnum == CallbackEnum::QuestOpenDescription);
		auto questElement = CastChecked<UQuestScreenElement>(punWidgetCaller);

		questEnumToDisplay = questElement->questEnum;

		if (wasQuestDescVisible) {
			QuestDescriptionOverlay->SetVisibility(ESlateVisibility::Collapsed);
			Spawn2DSound("UI", "UIWindowClose");
		}
		else {
			QuestDescriptionOverlay->SetVisibility(ESlateVisibility::Visible);
			Spawn2DSound("UI", "UIWindowOpen");
		}
	}

	UFUNCTION() void OnQuestDescriptionCloseButtonClick() {
		QuestDescriptionOverlay->SetVisibility(ESlateVisibility::Collapsed);
		Spawn2DSound("UI", "UIWindowClose");
	}

public:
	/*
	 * PlayerDetails
	 */
	UPROPERTY(meta = (BindWidget)) UButton* PlayerDetailsToggleButton;
	UPROPERTY(meta = (BindWidget)) UOverlay* PlayerDetailsOverlay;
	UPROPERTY(meta = (BindWidget)) UScrollBox* PlayerDetailsBox;
	UPROPERTY(meta = (BindWidget)) UButton* PlayerDetailsCloseButton;
	UPROPERTY(meta = (BindWidget)) UButton* PlayerDetailsXButton;

	UPROPERTY(meta = (BindWidget)) UButton* TableTabButton;
	UPROPERTY(meta = (BindWidget)) UButton* GraphTabButton;
	UPROPERTY(meta = (BindWidget)) UWidgetSwitcher* PlayerDetailsSwitcher;
	
	UPROPERTY(meta = (BindWidget)) UComboBoxString* PlayersGraphDropdown;
	UPROPERTY(meta = (BindWidget)) UWidgetSwitcher* PlayersGraphSwitcher;
	UPROPERTY(meta = (BindWidget)) UTimeSeriesPlot* PlayersPopulationGraph;
	UPROPERTY(meta = (BindWidget)) UTimeSeriesPlot* PlayersRevenueGraph;
	UPROPERTY(meta = (BindWidget)) UTimeSeriesPlot* PlayersTechnologiesGraph;
	UPROPERTY(meta = (BindWidget)) UTimeSeriesPlot* PlayersInfluenceGraph;

	bool isInitializedWithSim = false;

	void PunInit_PlayerDetails()
	{
		PlayerDetailsBox->ClearChildren();
		PlayerDetailsToggleButton->OnClicked.AddDynamic(this, &UQuestUI::TogglePlayerDetails);
		PlayerDetailsCloseButton->OnClicked.AddDynamic(this, &UQuestUI::TogglePlayerDetails);
		PlayerDetailsXButton->OnClicked.AddDynamic(this, &UQuestUI::TogglePlayerDetails);

		SetTabSelection(TableTabButton);
		PlayerDetailsSwitcher->SetActiveWidgetIndex(0);

		TableTabButton->OnClicked.AddDynamic(this, &UQuestUI::OnTableTabButtonClick);
		GraphTabButton->OnClicked.AddDynamic(this, &UQuestUI::OnGraphTabButtonClick);

		PlayersGraphDropdown->ClearOptions();
		PlayersGraphDropdown->AddOption("Population");
		PlayersGraphDropdown->AddOption("Revenue");
		PlayersGraphDropdown->AddOption("Technologies");
		PlayersGraphDropdown->AddOption("Influence");
		PlayersGraphDropdown->SetSelectedOption("Population");
		PlayersGraphDropdown->OnSelectionChanged.AddDynamic(this, &UQuestUI::OnGraphDropDownChanged);
		PlayersGraphSwitcher->SetActiveWidgetIndex(0);

		isInitializedWithSim = false;
	}
	
	UFUNCTION() void TogglePlayerDetails()
	{
		if (PlayerDetailsOverlay->GetVisibility() != ESlateVisibility::Collapsed) {
			PlayerDetailsOverlay->SetVisibility(ESlateVisibility::Collapsed);
			Spawn2DSound("UI", "UIWindowClose");
		} else {
			networkInterface()->ResetGameUI();
			PlayerDetailsOverlay->SetVisibility(ESlateVisibility::Visible);
			Spawn2DSound("UI", "UIWindowOpen");
		}
	}

	void TickPlayerDetails()
	{
		std::vector<int32> playerIds = GetPlayersSortedByPopulation();

		for (size_t k = 0; k < playerIds.size(); k++)
		{
			int32 curId = playerIds[k];
			
			if (k >= PlayerDetailsBox->GetChildrenCount()) {
				PlayerDetailsBox->AddChild(AddWidget<UPlayerCompareDetailedElement>(UIEnum::PlayerCompareDetailedElement));
			}

			auto element = CastChecked<UPlayerCompareDetailedElement>(PlayerDetailsBox->GetChildAt(k));
			SetChildHUD(element);

			auto& playerOwned = simulation().playerOwned(curId);

			bool isUIPlayer = (curId == playerId());
			auto setText = [&](URichTextBlock* textBlock, std::string message) {
				if (isUIPlayer) {
					SetText(textBlock, "<Bold>" + message + "</>");
				} else {
					SetText(textBlock, "<SlightGray>" + message + "</>");
				}
			};

			element->HighlightImage->SetVisibility(isUIPlayer ? ESlateVisibility::SelfHitTestInvisible : ESlateVisibility::Collapsed);

			setText(element->PlayerNameText, TrimString_Dots(simulation().playerName(curId), 14));
			setText(element->PopulationText, std::to_string(simulation().population(curId)));
			setText(element->TechnologyText, std::to_string(simulation().sciTechsCompleted(curId)));
			setText(element->RevenueText, std::to_string(playerOwned.totalRevenue100() / 100));
			setText(element->MilitarySizeText, std::to_string(simulation().influence(curId)));
			setText(element->LandText, std::to_string(playerOwned.GetPlayerLandTileCount(true)));

			auto& statSystem = simulation().statSystem(curId);
			const std::vector<std::vector<int32>>& productionStats = statSystem.GetResourceStat(ResourceSeasonStatEnum::Production);
			const std::vector<std::vector<int32>>& consumptionStats = statSystem.GetResourceStat(ResourceSeasonStatEnum::Consumption);
			

			int32 gdp100 = 0;
			std::vector<int32> productionValue100s;
			for (int enumInt = 0; enumInt < ResourceEnumCount; enumInt++)
			{
				check(productionStats[enumInt].size() == 4);
				int32 productionCount = productionStats[enumInt][0] +
												productionStats[enumInt][1] +
												productionStats[enumInt][2] +
												productionStats[enumInt][3];
				int32 productionValue100 = productionCount * simulation().price100(static_cast<ResourceEnum>(enumInt));
				
				productionValue100s.push_back(productionValue100);

				check(consumptionStats[enumInt].size() == 4);
				int32 consumptionCount = consumptionStats[enumInt][0] +
										consumptionStats[enumInt][1] +
										consumptionStats[enumInt][2] +
										consumptionStats[enumInt][3];
				int32 consumptionValue100 = consumptionCount * simulation().price100(static_cast<ResourceEnum>(enumInt));
				
				gdp100 += productionValue100 - consumptionValue100;
			}

			// Mint included in GDP
			{
				gdp100 += 100 * statSystem.GetYearlyStat(SeasonStatEnum::Money);
			}

			

			// Calculate main production
			std::vector<std::pair<ResourceEnum, int32>> mainProductions;
			for (size_t i = 0; i < ResourceEnumCount; i++) 
			{
				if (productionValue100s[i] > 0)
				{
					bool added = false;
					for (size_t j = 0; j < mainProductions.size(); j++)
					{
						if (productionValue100s[i] > mainProductions[j].second) {
							mainProductions.insert(mainProductions.begin() + j, std::make_pair(static_cast<ResourceEnum>(i), productionValue100s[i]));
							added = true;
							break;
						}
					}
					if (!added) {
						mainProductions.push_back(std::make_pair(static_cast<ResourceEnum>(i), productionValue100s[i]));
					}
					if (mainProductions.size() > 2) {
						mainProductions.pop_back();
					}
				}
			}

			setText(element->GDPText, std::to_string(gdp100 / 100));


			auto addSS = [&](std::stringstream& ss, std::string message) {
				if (isUIPlayer) {
					ss << "<Bold>" << message << "</>";
				} else {
					ss << "<SlightGray>" << message << "</>";
				}
			};
			
			{
				std::stringstream ss;
				if (mainProductions.size() > 0) {
					addSS(ss, ResourceName(mainProductions[0].first));
				}
				if (mainProductions.size() > 1) {
					ss << "\n";
					addSS(ss, ResourceName(mainProductions[1].first));
				}
				SetText(element->MainProductionText, ss.str());
			}

			//// Allies and vassals
			//{
			//	std::stringstream ss;
			//	const std::vector<int32> allyIds = playerOwned.allyPlayerIds();
			//	for (int32 i = 0; i < allyIds.size(); i++) {
			//		if (i > 0) {
			//			ss << "\n";
			//		}
			//		addSS(ss, TrimString_Dots(simulation().playerName(allyIds[i]), 10));
			//	}
			//	SetText(element->AlliesText, ss.str());
			//}
			//{
			//	std::stringstream ss;
			//	const std::vector<int32> vassalBuildingIds = playerOwned.vassalBuildingIds();
			//	int32 i = 0;
			//	for (int32 nodeId : vassalBuildingIds) {
			//		Building& bld = simulation().building(nodeId);
			//		if (bld.isEnum(CardEnum::Townhall)) {
			//			if (i > 0) {
			//				ss << "\n";
			//			}
			//			addSS(ss, TrimString_Dots(simulation().playerName(bld.playerId()), 10));
			//			i++;
			//		}
			//	}
			//	SetText(element->VassalsText, ss.str());
			//}
			
			//element->AlliesText
			
			element->SetVisibility(ESlateVisibility::Visible);
		}

		for (size_t i = playerIds.size(); i < PlayerDetailsBox->GetChildrenCount(); i++) {
			PlayerDetailsBox->GetChildAt(i)->SetVisibility(ESlateVisibility::Collapsed);
		}

		// Initialize Graphs later
		if (!isInitializedWithSim) {
			isInitializedWithSim = true;
			
			std::vector<int32> allHumanIds = simulation().allHumanPlayerIds();
			auto addSeriesAll = [&](FString graphName, UTimeSeriesPlot* plot, PlotStatEnum plotEnum) {
				std::vector<GraphSeries> series;
				for (int32 i = 0; i < allHumanIds.size(); i++) {
					FLinearColor color = PlayerColor1(allHumanIds[i]);
					//color = FLinearColor::LerpUsingHSV(color, FLinearColor::White, 0.3); // Mix in some white to make the graph easier to read
					series.push_back({ ToFString(simulation().playerName(allHumanIds[i])), plotEnum, color, allHumanIds[i] });
				}
				AddSeries(plot, series);
			};

			addSeriesAll("Population", PlayersPopulationGraph, PlotStatEnum::Population);
			addSeriesAll("Revenue", PlayersRevenueGraph, PlotStatEnum::Income);
			addSeriesAll("Technologies", PlayersTechnologiesGraph, PlotStatEnum::Technologies);
			addSeriesAll("Influence", PlayersInfluenceGraph, PlotStatEnum::InfluencePoints);
		}
	}

	std::vector<int32> GetPlayersSortedByPopulation()
	{
		std::vector<int32> playerIds = simulation().allHumanPlayerIds();
		
		std::sort(playerIds.begin(), playerIds.end(), [&](int32 playerA, int32 playerB) {
			return simulation().population(playerA) > simulation().population(playerB);
		});
		return playerIds;
	}

	/*
	 * Table/Graph Tab
	 */

	void SetTabSelection(UButton* button) {
		auto setHighlight = [&](UButton* buttonLocal) {
			SetButtonHighlight(buttonLocal, buttonLocal == button);
		};

		setHighlight(TableTabButton);
		setHighlight(GraphTabButton);
	}

	UFUNCTION() void OnTableTabButtonClick() {
		SetTabSelection(TableTabButton);
		PlayerDetailsSwitcher->SetActiveWidgetIndex(0);
		dataSource()->Spawn2DSound("UI", "ButtonClick");
	}
	UFUNCTION() void OnGraphTabButtonClick() {
		SetTabSelection(GraphTabButton);
		PlayerDetailsSwitcher->SetActiveWidgetIndex(1);
		dataSource()->Spawn2DSound("UI", "ButtonClick");
	}

	/*
	 * Graph
	 */
	UFUNCTION() void OnGraphDropDownChanged(FString sItem, ESelectInfo::Type seltype)
	{
		if (seltype == ESelectInfo::Type::Direct) {
			return;
		}

		int32 index = 0;
		auto dropDownCheck = [&](const FString& name) {
			if (sItem == name) {
				PlayersGraphDropdown->SetSelectedOption(sItem);
				PlayersGraphSwitcher->SetActiveWidgetIndex(index);
			}
			index++;
		};

		dropDownCheck("Population");
		dropDownCheck("Revenue");
		dropDownCheck("Technologies");
		dropDownCheck("Influence");
	}

private:
	QuestEnum questEnumToDisplay = QuestEnum::None;
};
