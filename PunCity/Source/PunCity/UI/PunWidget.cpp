// Fill out your copyright notice in the Description page of Project Settings.

#include "PunWidget.h"
#include "PunBoxWidget.h"
#include "EngineFontServices.h"
#include "GraphDataSource.h"
#include "PunGraph.h"

int32 UPunWidget::kPointerOnUI = 0;

void UPunWidget::ResetTooltip(UWidget* widget) {
	if (widget->ToolTipWidget) {
		auto tooltip = Cast<UToolTipWidgetBase>(widget->ToolTipWidget);
		if (tooltip) {
			tooltip->TooltipPunBoxWidget->AfterAdd();
		}
	}
}

UToolTipWidgetBase* UPunWidget::AddToolTip(UWidget* widget)
{
	// TODO: why this not work?
	//if (!widget->IsHovered()) {
	//	ResetTooltip(widget);
	//	return nullptr;
	//}
	
	DEBUG_AI_VAR(AddToolTip);
	
	// Detect if tooltip was already created, if so don't recreate
	if (widget->ToolTipWidget) {
		auto tooltip = Cast<UToolTipWidgetBase>(widget->ToolTipWidget);
		if (tooltip) {
			return tooltip;
		}
	}
	
	auto tooltip = AddWidget<UToolTipWidgetBase>(UIEnum::ToolTip);
	SetChildHUD(tooltip->TooltipPunBoxWidget);
	widget->SetToolTip(tooltip);
	return tooltip;
}

UToolTipWidgetBase* UPunWidget::AddToolTip(UWidget* widget, std::string message)
{
	UToolTipWidgetBase* tooltip = AddToolTip(widget);
	if (tooltip) {
		tooltip->TooltipPunBoxWidget->AfterAdd(); // Ensure reused tooltip gets its AfterAdd called
		tooltip->TooltipPunBoxWidget->AddRichTextParsed(message);
	}
	return tooltip;
}
UToolTipWidgetBase* UPunWidget::AddToolTip(UWidget* widget, std::wstring message)
{
	UToolTipWidgetBase* tooltip = AddToolTip(widget);
	if (tooltip) {
		tooltip->TooltipPunBoxWidget->AfterAdd(); // Ensure reused tooltip gets its AfterAdd called
		tooltip->TooltipPunBoxWidget->AddRichText(message);
	}
	return tooltip;
}

void UPunWidget::AddResourceTooltip(UWidget* widget, ResourceEnum resourceEnum, bool skipWidgetHoverCheck)
{
	if (IsSimulationInvalid()) return;

	if (!widget->IsHovered() ||
		skipWidgetHoverCheck) 
	{
		ResetTooltip(widget);
		return;
	}

	std::stringstream ss;
	ss << "<Bold>" << GetResourceInfo(resourceEnum).name << "</>";
	ss << "<space>";
	ss << "<SlightGray>" << GetResourceInfo(resourceEnum).description << "</>";
	ss << "<space>";

	auto& statSys = simulation().playerStatSystem(playerId());
	ss << "Production (yearly): <FaintGreen>" << statSys.GetYearlyResourceStat(ResourceSeasonStatEnum::Production, resourceEnum) << "</>\n";
	ss << "Consumption (yearly): <FaintRed>" << statSys.GetYearlyResourceStat(ResourceSeasonStatEnum::Consumption, resourceEnum) << "</>\n";
	ss << "<space>";

	int32 price100 = simulation().price100(resourceEnum);
	int32 basePrice100 = GetResourceInfo(resourceEnum).basePrice100();
	ss << "Price: " << (price100 / 100.0f) << "<img id=\"Coin\"/> <Gray>(base " << basePrice100 / 100 << ")</>";
	ss << "<space>";
	
	auto tooltip = AddToolTip(widget, ss.str());
	ss.str("");

	auto punGraph = tooltip->TooltipPunBoxWidget->AddGraph();
	
	// Price Graph
	if (bIsHoveredButGraphNotSetup || tooltip->TooltipPunBoxWidget->DidElementResetThisRound())
	{
		bIsHoveredButGraphNotSetup = false;
		
		auto graph = punGraph->Graph;
		std::vector<GraphSeries> seriesList = { {FString("Current Price"), PlotStatEnum::TipMarketPrice, FLinearColor(0.7, 1, 0.7) }};
		
		UGraphDataSource* graphDataSource = NewObject<UGraphDataSource>(this);
		//UGraphDataSource* graphDataSource = GetPunHUD()->SpawnGraphDataSource();
		graphDataSource->Init(playerId(), dataSource());
		graphDataSource->resourceEnum = resourceEnum;
		
		graphDataSource->AddSeries(seriesList);

		for (int32 i = 0; i < seriesList.size(); i++)
		{
			FName seriesId = FName(*seriesList[i].seriesId);
			graph->EnableSeries(seriesId, true);
			graph->ConfigureSeries(seriesId, false, true);
			graph->AddSeriesStyleOverride(seriesId, nullptr, seriesList[i].color);
			graph->SetDatasource(graphDataSource);
		}
	}

	std::vector<std::pair<int32, int32>> mainImporters = simulation().worldTradeSystem().GetTwoMainTraders(resourceEnum, true);
	if (mainImporters.size() > 0) {
		ss << "major importers: " << simulation().playerName(mainImporters[0].first);
		if (mainImporters.size() > 1) {
			ss << ", " << simulation().playerName(mainImporters[1].first);
		}
		ss << "<space>";
	}
	std::vector<std::pair<int32, int32>> mainExporters = simulation().worldTradeSystem().GetTwoMainTraders(resourceEnum, false);
	if (mainExporters.size() > 0) {
		ss << "major exporters: " << simulation().playerName(mainExporters[0].first);
		if (mainExporters.size() > 1) {
			ss << ", " << simulation().playerName(mainExporters[1].first);
		}
		ss << "<space>";
	}

	tooltip->TooltipPunBoxWidget->AddRichTextParsed(ss.str());
}

std::string UPunWidget::WrapString(std::string str, int32 wrapSize)
{
	FSlateFontInfo fontInfo = GetPunHUD()->defaultFont();

	FString fString = ToFString(str);

	TSharedPtr<FSlateFontCache> FontCache = FEngineFontServices::Get().GetFontCache();
	FCharacterList& CharacterList = FontCache->GetCharacterList(fontInfo, 1);

	int32 CurrentX = 0;
	TCHAR PreviousChar = 0;
	int32 wordCharCount = 0; // Collect wordCharCount to add \n to the correct spot

	for (size_t i = 0; i < fString.Len(); i++)
	{
		if (CurrentX == 0 && fString[i] == ' ') {
			fString.RemoveAt(i);
		}
		
		//PUN_LOG("%c  - CurrentX:%d .. i:%d wordCharCount:%d %s", fString[i], CurrentX, i, wordCharCount, *fString.Replace(TEXT("\n"), TEXT("|")));
		
		if (fString.Mid(i, 2) == "\n") { // Reset count with line end
			i++;
			CurrentX = 0;
			wordCharCount = 0;

			//PUN_LOG("Existing Endline");
		}
		else if (CurrentX > wrapSize)
		{
			int32 insertSpot = i - wordCharCount;

			fString.InsertAt(insertSpot, "\n");

			//PUN_LOG("End line %d", insertSpot);

			i = insertSpot;
			//i++; // \n is 2 chars, loop already has 1 increment, hence only 1 extra needed
			CurrentX = 0;
			wordCharCount = 0;
		}
		else 
		{
			TCHAR CurrentChar = fString[i];

			const FCharacterEntry& Entry = CharacterList.GetCharacter(CurrentChar, fontInfo.FontFallback);

			int32 Kerning = 0;
			if (PreviousChar != 0) {
				Kerning = CharacterList.GetKerning(CharacterList.GetCharacter(PreviousChar, fontInfo.FontFallback), Entry);
			}
			PreviousChar = CurrentChar;

			//const int32 TotalCharSpacing =
			//				Kerning + Entry.HorizontalOffset +		// Width is any kerning plus how much to advance the position when drawing a new character
			//				Entry.XAdvance;	// How far we advance

			CurrentX += Kerning + Entry.XAdvance;

			// Collect wordCharCount to add \n to the correct spot
			if (str[i] == ' ') {
				//PUN_LOG("Space Reset");
				wordCharCount = 0;
			}
			else {
				wordCharCount++;
			}
		}
	}
	

	

	//int32 count = 0;
	//int32 wordCharCount = 0;

	////FSlateFontMeasure::Measure(fString, )

	//for (size_t i = 0; i < str.size(); i++)
	//{
	//	if (str.substr(i, 2) == "\n") { // Reset count with line end
	//		i++;
	//		count = 0;
	//		wordCharCount = 0;
	//	}
	//	else if (count > wrapSize)
	//	{
	//		str.insert(i - wordCharCount, "\n");

	//		i++; // \n is 2 chars, loop already has 1 increment, hence only 1 extra needed
	//		count = 0;
	//		wordCharCount = 0;
	//	}
	//	else {
	//		count++;

	//		if (str[i] == ' ') {
	//			wordCharCount = 0;
	//		}
	//		else {
	//			wordCharCount++;
	//		}
	//	}
	//}

	
	return ToStdString(fString);
}