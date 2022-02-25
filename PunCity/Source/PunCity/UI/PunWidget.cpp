// Fill out your copyright notice in the Description page of Project Settings.

#include "PunWidget.h"
#include "PunBoxWidget.h"
#include "EngineFontServices.h"
#include "GraphDataSource.h"
#include "PunGraph.h"
#include "KantanChartLegend.h"

int32 UPunWidget::kPointerOnUI = 0;
TArray<FString> UPunWidget::kPointerOnUINames;

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

UToolTipWidgetBase* UPunWidget::AddToolTip(UWidget* widget, FText message)
{
	UToolTipWidgetBase* tooltip = AddToolTip(widget);
	if (tooltip) {
		tooltip->TooltipPunBoxWidget->AfterAdd(); // Ensure reused tooltip gets its AfterAdd called
		tooltip->TooltipPunBoxWidget->AddRichTextParsed(message);
	}
	return tooltip;
}

#define LOCTEXT_NAMESPACE "ResourceTooltip"

// Note: This is spammable... It won't trigger before hover... Graphs will only trigger on mouse hover
void UPunWidget::AddResourceTooltip(UWidget* widget, ResourceEnum resourceEnum, bool skipWidgetHoverCheck)
{
	if (IsSimulationInvalid()) return;

	if (!widget->IsHovered() ||
		skipWidgetHoverCheck) 
	{
		ResetTooltip(widget);
		return;
	}

	TArray<FText> args;
	ADDTEXT_TAG_("<Bold>", GetResourceInfo(resourceEnum).name);
	ADDTEXT_INV_("<space>");

	std::wstring envelopString = StringEnvelopImgTag(FTextToW(GetResourceInfo(resourceEnum).description), TEXT("<SlightGray>"));
	ADDTEXT_(INVTEXT("<SlightGray>{0}</>"), FText::FromString(FString(envelopString.c_str())));
	ADDTEXT_INV_("<space>");

	auto& statSys = simulation().statSystem(playerId());
	ADDTEXT_(LOCTEXT("ResourceTipProduction", "Production (yearly): <FaintGreen>{0}</>\n"), TEXT_NUM(statSys.GetYearlyResourceStat(ResourceSeasonStatEnum::Production, resourceEnum)));
	ADDTEXT_(LOCTEXT("ResourceTipConsumption", "Consumption (yearly): <FaintRed>{0}</>\n"), TEXT_NUM(statSys.GetYearlyResourceStat(ResourceSeasonStatEnum::Consumption, resourceEnum)));
	ADDTEXT_INV_("<space>");

	int32 price100 = simulation().price100(resourceEnum);
	int32 basePrice100 = GetResourceInfo(resourceEnum).basePrice100();
	ADDTEXT_(LOCTEXT("ResourceTipPrice", "Price: {0}<img id=\"Coin\"/> <Gray>(base {1})</>"), TEXT_NUM(price100 / 100.0f), TEXT_NUM(basePrice100 / 100));
	ADDTEXT_INV_("<space>");
	
	auto tooltip = AddToolTip(widget, args);
	args.Empty();

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
		
		graphDataSource->SetSeries(seriesList);

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
		ADDTEXT_(LOCTEXT("major importers: {0}", "major importers: {0}"), simulation().playerNameT(mainImporters[0].first));
		if (mainImporters.size() > 1) {
			ADDTEXT_(INVTEXT(", {0}"), simulation().playerNameT(mainImporters[1].first));
		}
		ADDTEXT_INV_("<space>");
	}
	std::vector<std::pair<int32, int32>> mainExporters = simulation().worldTradeSystem().GetTwoMainTraders(resourceEnum, false);
	if (mainExporters.size() > 0) {
		ADDTEXT_(LOCTEXT("major exporters: {0}", "major exporters: {0}"), simulation().playerNameT(mainExporters[0].first));
		if (mainExporters.size() > 1) {
			ADDTEXT_(INVTEXT(", {0}"), simulation().playerNameT(mainExporters[1].first));
		}
		ADDTEXT_INV_("<space>");
	}

	tooltip->TooltipPunBoxWidget->AddRichTextParsed(args);
}

void UPunWidget::AddTradeOfferTooltip(UWidget* widget, bool isImport, ResourceEnum resourceEnum, int32 resourceCount, int32 orderFulfilled)
{
	AddToolTip(widget, FText::Format(
		// Export 100 Furniture
		// Order fulfilled: 100
		isImport ? LOCTEXT("TradeOfferImport_Tip", "This city wants to import {0} {1}<space>Order fulfilled: {2}<space>To trade with this city, use the Trading Company.") :
					LOCTEXT("TradeOfferExport_Tip", "This city wants to export {0} {1}<space>Order fulfilled: {2}<space>To trade with this city, use the Trading Company."),
		TEXT_NUM(resourceCount),
		ResourceNameT(resourceEnum),
		TEXT_NUM(orderFulfilled)
	));
}

#undef LOCTEXT_NAMESPACE


void UPunWidget::SetSeries(UTimeSeriesPlot* graph, std::vector<GraphSeries> seriesList)
{
	// Set the legend
	GetFellowChild<UKantanChartLegend>(graph)->SetChart(graph);

	UGraphDataSource* graphDataSource = NewObject<UGraphDataSource>(this);
	//dataSources.Add(graphDataSource);
	graphDataSource->Init(playerId(), dataSource());
	graphDataSource->SetSeries(seriesList);

	for (int32 i = 0; i < seriesList.size(); i++)
	{
		FName seriesId = FName(*seriesList[i].seriesId);
		graph->EnableSeries(seriesId, true);
		graph->ConfigureSeries(seriesId, false, true);
		graph->AddSeriesStyleOverride(seriesId, nullptr, seriesList[i].color);
		graph->SetDatasource(graphDataSource);
	}
}



std::string UPunWidget::WrapString(std::string str, int32 wrapSize, FSlateFontInfo* fontInfoPtr)
{
	FSlateFontInfo fontInfo = fontInfoPtr ? *fontInfoPtr : GetPunHUD()->defaultFont();

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

	
	return ToStdString(fString);
}

FString UPunWidget::WrapStringF(FString fString, int32 wrapSize, FSlateFontInfo* fontInfoPtr)
{
	FSlateFontInfo fontInfo = fontInfoPtr ? *fontInfoPtr : GetPunHUD()->defaultFont();

	TSharedPtr<FSlateFontCache> FontCache = FEngineFontServices::Get().GetFontCache();
	FCharacterList& CharacterList = FontCache->GetCharacterList(fontInfo, 1);

	int32 CurrentX = 0;
	TCHAR PreviousChar = 0;
	int32 wordCharCount = 0; // Collect wordCharCount to add \n to the correct spot in front of word
	int32 lastInsertSpot = 0;
	
	int32 initialLength = fString.Len();

	for (size_t i = 0; i < fString.Len(); i++)
	{
		// Prevent freeze if the text is too long
		if (fString.Len() > 2 * initialLength) {
			UE_DEBUG_BREAK();
			break;
		}
		
		if (CurrentX == 0 && fString[i] == TEXT(' ')) {
			fString.RemoveAt(i);
		}

		//PUN_LOG("%c  - CurrentX:%d .. i:%d wordCharCount:%d %s", fString[i], CurrentX, i, wordCharCount, *fString.Replace(TEXT("\n"), TEXT("|")));

		if (fString.Mid(i, 2) == TEXT("\n")) { // Reset count with line end
			i++;
			CurrentX = 0;
			wordCharCount = 0;

			//PUN_LOG("Existing Endline");
		}
		else if (CurrentX > wrapSize)
		{
			int32 insertSpot = i - wordCharCount;

			// TODO: If the whole line has no space, put the "\n" at the end of the line
			if (insertSpot - lastInsertSpot == 0) {
				insertSpot = i - 1;
			}
			
			fString.InsertAt(insertSpot, TEXT("\n"));

			//PUN_LOG("End line %d", insertSpot);

			i = insertSpot;
			lastInsertSpot = insertSpot;
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
			if (fString[i] == TEXT(' ')) {
				//PUN_LOG("Space Reset");
				wordCharCount = 0;
			}
			else {
				wordCharCount++;
			}
		}
	}


	return fString;
}


#define LOCTEXT_NAMESPACE "SetGeoresourceImage"

void UPunWidget::SetGeoresourceImage(UImage* image, ResourceEnum resourceEnum, UAssetLoaderComponent* assetLoader, UPunWidget* punWidget)
{
	auto material = image->GetDynamicMaterial();

	if (IsOreEnum(resourceEnum)) {
		punWidget->AddToolTip(image, FText::Format(
			LOCTEXT("MineGeoresourceImage_Tip", "{0} Deposit in this region that can be mined."),
			ResourceNameT(resourceEnum)
		));
	}
	else if (resourceEnum == ResourceEnum::Oil)
	{
		punWidget->AddToolTip(image, FText::Format(
			LOCTEXT("FarmGeoresourceImage_Tip", "Oil-rich region."),
			ResourceNameT(resourceEnum)
		));
	}
	else {
		punWidget->AddToolTip(image, FText::Format(
			LOCTEXT("FarmGeoresourceImage_Tip", "This region is suitable for {0} Farming."),
			ResourceNameT(resourceEnum)
		));
	}


	

	switch (resourceEnum) {
	case ResourceEnum::IronOre: resourceEnum = ResourceEnum::Iron; break;
	case ResourceEnum::GoldOre: resourceEnum = ResourceEnum::GoldBar; break;
	}

	UTexture2D* worldGeoresourceIcon = assetLoader->GetGeoresourceIcon(resourceEnum);
	if (worldGeoresourceIcon)
	{
		material->SetScalarParameterValue("IsAlphaOpacity", 1);
		material->SetTextureParameterValue("ColorTexture", worldGeoresourceIcon);
	}
	else {
		material->SetScalarParameterValue("IsAlphaOpacity", 0);
		material->SetTextureParameterValue("ColorTexture", assetLoader->GetResourceIcon(resourceEnum));
		material->SetTextureParameterValue("DepthTexture", assetLoader->GetResourceIconAlpha(resourceEnum));
	}
};


#undef LOCTEXT_NAMESPACE