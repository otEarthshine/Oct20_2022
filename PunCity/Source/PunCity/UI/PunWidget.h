// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "PunHUDInterface.h"
#include "ToolTipWidgetBase.h"
#include "Components/TextBlock.h"
#include "Components/EditableTextBox.h"
#include "Components/Button.h"
#include "Kismet/GameplayStatics.h"
#include "PunCity/Simulation/GameSimulationCore.h"
#include "Components/RichTextBlock.h"
#include "PunCity/PunGameInstance.h"

// Don't remove faded
#include "Components/VerticalBox.h"
#include "Components/HorizontalBox.h" 
#include "Components/ScrollBox.h"
#include "Components/SizeBox.h"
#include "Components/CheckBox.h"
#include "Components/Image.h"
#include "Components/VerticalBoxSlot.h"
#include "Components/WrapBox.h"
#include "Components/Overlay.h"
#include "Components/ComboBoxString.h"
#include "Components/Slider.h"
#include "Components/CanvasPanel.h"
#include "Components/CanvasPanelSlot.h"
#include "Components/Spacer.h"
#include "WGT_ButtonCpp.h"


#include "Blueprint/SlateBlueprintLibrary.h"
#include "Slate/SGameLayerManager.h"


#include "PunWidget.generated.h"

/**
 * 
 */
UCLASS()
class UPunWidget : public UUserWidget
{
	GENERATED_BODY()
public:

	void SetHUD(TScriptInterface<IPunHUDInterface> punHUD, UIEnum uiEnumIn) {
		_punHUD = punHUD;
		punId = -1;
		uiEnum = uiEnumIn;
		startTime = UGameplayStatics::GetTimeSeconds(GetWorld());
		uiHash = -1;
		
		OnInit();
	}

	// For children that are BindWidgets
	void SetChildHUD(UPunWidget* child) {
		child->SetHUD(GetTPunHUD(), UIEnum::None);
	}

	virtual void OnInit() {}

	virtual void OnDespawnWidget() {}
	
	IPunHUDInterface* GetPunHUD() {
		check(_punHUD);
		return CastChecked<IPunHUDInterface>(_punHUD.GetObject());
	}
	TScriptInterface<IPunHUDInterface> GetTPunHUD() {
		check(_punHUD);
		return _punHUD;
	}

	template<typename T>
	T* AddWidgetToHUD(UIEnum uiEnum) {
		return CastChecked<T>(GetPunHUD()->AddWidgetToHUD(uiEnum));
	}
	template<typename T>
	T* AddWidget(UIEnum uiEnum) {
		UUserWidget* userWidget = GetPunHUD()->AddWidget(uiEnum);
		return CastChecked<T>(userWidget);
	}

	template<typename T>
	T* AddWidget(UIEnum uiEnum, UUserWidget* parent) {
		T* widget = CastChecked<T>(GetPunHUD()->AddWidget(uiEnum));
		widget->AddChild(parent);
		return widget;
	}

	/*
	 * Pool
	 */
	template<typename T>
	T* SpawnWidget(UIEnum uiEnumIn) {
		UWidget* widget = GetPunHUD()->SpawnWidget(uiEnumIn);
		PUN_CHECK(Cast<T>(widget));
		return CastChecked<T>(widget);
	}

	void DespawnWidget(UIEnum uiEnumIn, UWidget* widget) {
		GetPunHUD()->DespawnWidget(uiEnumIn, widget);
	}

	/*
	 * Mouse enter
	 */
	bool bIsHoveredButGraphNotSetup = false;
	
	void NativeOnMouseEnter(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent) override {
		bIsHoveredButGraphNotSetup = true;
	}
	void NativeOnMouseLeave(const FPointerEvent& InMouseEvent) override {

	}

	/*
	 * Tooltip
	 */
	void ResetTooltip(UWidget* widget);

	UToolTipWidgetBase* AddToolTip(UWidget* widget);
	UToolTipWidgetBase* AddToolTip(UWidget* widget, FText message);
	UToolTipWidgetBase* AddToolTip(UWidget* widget, TArray<FText>& args, bool emptyArgs = true) {
		auto tooltip = AddToolTip(widget, FText::Join(FText(), args));
		if (emptyArgs) {
			args.Empty();
		}
		return tooltip;
	}
	

	void SetSeries(class UTimeSeriesPlot* graph, std::vector<struct GraphSeries> seriesList);

	

	TSubclassOf<UUserWidget> GetPunWidgetClass(UIEnum uiEnumIn) { return GetPunHUD()->GetPunWidgetClass(uiEnumIn); }

	IGameUIDataSource* dataSource() { return GetPunHUD()->dataSource(); }
	IGameUIInputSystemInterface* inputSystemInterface() { return GetPunHUD()->inputSystemInterface(); }
	IGameNetworkInterface* networkInterface() { return GetPunHUD()->networkInterface(); }

	UAssetLoaderComponent* assetLoader() { return dataSource()->assetLoader(); }
	GameSimulationCore& simulation() { return dataSource()->simulation(); }

	int32 playerId() { return GetPunHUD()->playerId(); }
	int32 currentTownId() { return GetPunHUD()->currentTownId(); }

	FactionEnum playerFactionEnum() { return dataSource()->playerInfo(playerId()).factionEnum(); }
	
	UWorld* GetWorldPun() { return GetPunHUD()->GetWorldPun(); }

	bool InterfacesInvalid() { return GetPunHUD()->IsInvalid(); }
	bool IsSimulationInvalid() { return  GetPunHUD()->IsSimulationInvalid(); }

	void GetAnimations() {
		GetAnimations(Animations);
	}
	void GetAnimations(TMap<FString, UWidgetAnimation*>& OutResults) {
		return GetAnimations(this, OutResults);
	}
	UWidgetAnimation* GetAnimation(FString name) {
		TMap<FString, UWidgetAnimation*> OutResults;
		GetAnimations(OutResults);
		return OutResults[name];
	}
	bool HasAnimation() { return Animations.Num() > 0; }

	static void GetAnimations(UWidget* widget, TMap<FString, UWidgetAnimation*>& OutResults)
	{
		OutResults.Empty();

		FProperty* Property = widget->GetClass()->PropertyLink;

		int32 loop;
		for (loop = 10000; loop-- > 0;)
		{
			if (Property == nullptr) {
				break;
			}
			
			if (Property->GetClass() == FObjectProperty::StaticClass())
			{
				FObjectProperty* ObjectProperty = CastField<FObjectProperty>(Property);

				if (ObjectProperty->PropertyClass == UWidgetAnimation::StaticClass())
				{
					UObject* Object = ObjectProperty->GetObjectPropertyValue_InContainer(widget);
					UWidgetAnimation* WidgetAnimation = Cast<UWidgetAnimation>(Object);

					if (WidgetAnimation != nullptr)
					{
						FString Name = WidgetAnimation->GetMovieScene()->GetFName().ToString();
						OutResults.Add(Name, WidgetAnimation);
					}
				}
			}

			Property = Property->PropertyLinkNext;
		}

		PUN_CHECK(loop > 1);
	}
	static UWidgetAnimation* GetAnimation(UWidget* widget, FString name) {
		TMap<FString, UWidgetAnimation*> OutResults;
		GetAnimations(widget, OutResults);
		return OutResults[name];
	}
	

	float GetAge() { return UGameplayStatics::GetTimeSeconds(GetWorld()) - startTime;  }

	static void SetText(UTextBlock* textBlock, const std::string& str) {
		textBlock->SetText(FText::FromString(ToFString(str)));
	}
	static void SetText(UTextBlock* textBlock, std::stringstream& ss) {
		textBlock->SetText(FText::FromString(ToFString(ss.str())));
		ss.str(std::string());
	}
	static void SetTextF(UTextBlock* textBlock, const FString& str) {
		textBlock->SetText(FText::FromString(str));
	}
	static void SetText(UTextBlock* textBlock, TArray<FText>& texts) {
		textBlock->SetText(JOINTEXT(texts));
		texts.Empty();
	}

	static void SetText(UTextBlock* textBlock, FText str) {
		textBlock->SetText(str);
	}
	
	static void SetTextNumber(UTextBlock* textBlock, float value, int32 precision) {
		std::stringstream ss;
		ss.precision(precision);
		ss << std::fixed << value;
		textBlock->SetText(ToFText(ss.str()));
	}

	
	static void SetTextShorten(UTextBlock* textBlock, std::string str) {
		if (str.size() > 12) {
			str.resize(12);
			str += "..";
		}
		textBlock->SetText(FText::FromString(ToFString(str)));
	}
	static std::string TrimString_Dots(std::string str, int32 trimSize) {
		if (str.size() > trimSize) {
			str.resize(trimSize);
			str += "..";
		}
		return str;
	}
	static FString TrimStringF_Dots(FString str, int32 trimSize) {
		if (str.Len() > trimSize) {
			str = str.Left(trimSize) + "..";
		}
		return str;
	}
	static FString TrimStringF(FString str, int32 trimSize) {
		if (str.Len() > trimSize) {
			str = str.Left(trimSize);
		}
		return str;
	}

	static const int32 ObjectFocusUIWrapSize = 280; // 330
	std::string WrapString(std::string str, int32 wrapSize = ObjectFocusUIWrapSize, FSlateFontInfo* fontInfoPtr = nullptr);

	// Bugged for localization
	FString WrapStringF(FString fString, int32 wrapSize = ObjectFocusUIWrapSize, FSlateFontInfo* fontInfoPtr = nullptr);
	
	static void SetText(URichTextBlock* textBlock, std::string str) {
		textBlock->SetText(FText::FromString(ToFString(str)));
	}
	static void SetText(URichTextBlock* textBlock, std::stringstream& ss) {
		textBlock->SetText(FText::FromString(ToFString(ss.str())));
		ss.str(std::string());
	}

	static void SetText(URichTextBlock* textBlock, std::wstring str) {
		textBlock->SetText(FText::FromString(ToFString(str)));
	}

	static void SetText(URichTextBlock* textBlock, FText text) {
		textBlock->SetText(text);
	}
	static void SetText(URichTextBlock* textBlock, TArray<FText>& args) {
		textBlock->SetText(JOINTEXT(args));
		args.Empty();
	}

	template <class T>
	static void SetPriceColor(T* textBlock, int32 price100, int32 basePrice100)
	{
		if (price100 > basePrice100 * 120 / 100) { // Expensive
			textBlock->SetColorAndOpacity(FLinearColor(1, 0.7, 0.7));
		}
		else if (price100 < basePrice100 * 80 / 100) { // Cheap
			textBlock->SetColorAndOpacity(FLinearColor(0.7, 1, 0.7));
		}
		else {
			textBlock->SetColorAndOpacity(FLinearColor::White);
		}
	}
	
	static void SetFString(UTextBlock* textBlock, FString str) {
		textBlock->SetText(FText::FromString(str));
	}
	static void SetFString(UEditableTextBox* textBlock, FString str) {
		textBlock->SetText(FText::FromString(str));
	}

	static void SetResourceImage(UImage* image, ResourceEnum resourceEnum, UAssetLoaderComponent* assetLoader)
	{
		image->SetBrushFromMaterial(assetLoader->ResourceIconMaterial);
		image->GetDynamicMaterial()->SetTextureParameterValue("ColorTexture", assetLoader->GetResourceIcon(resourceEnum));
		image->GetDynamicMaterial()->SetTextureParameterValue("DepthTexture", assetLoader->GetResourceIconAlpha(resourceEnum));
	}

	static void SetGeoresourceImage(UImage* image, ResourceEnum resourceEnum, UAssetLoaderComponent* assetLoader, UPunWidget* punWidget);
	

	//! Mouse hovering away for 2 ticks counts as IsPointerOnUI false
	static int32 kPointerOnUI;
	static TArray<FString> kPointerOnUINames;
	static bool IsPointerOnUI()
	{
		if (PunSettings::IsOn("ForceClickthrough")) {
			return false;
		}
		return kPointerOnUI > 0;
	}
	void CheckPointerOnUI() { CheckPointerOnUI(CastChecked<UWidget>(this)); }
	void CheckPointerOnUI(UWidget* widget) {
		if (widget->IsHovered()) {
			kPointerOnUI = std::min(kPointerOnUI + 2, 2);

			kPointerOnUINames.Add(
				widget->GetName() + "\n" + widget->GetPathName() + "\n"
			);
		}
	}
	static void TickIsHovered() {
		kPointerOnUI = std::max(kPointerOnUI - 1, 0);
	}

	void TryPlayAnimation(FString key) {
		if (!IsAnimationPlaying(Animations[key])) {
			PlayAnimation(Animations[key], 0, 0); // Always play the flash	
		}
	}
	void TryStopAnimation(FString key) {
		if (IsAnimationPlaying(Animations[key])) {
			StopAnimation(Animations[key]); // Always play the flash	
		}
	}
	void PlayAnimationIf(FString key, bool shouldPlay) {
		if (shouldPlay) {
			TryPlayAnimation(key);
		} else {
			TryStopAnimation(key);
		}
	}

	UPunGameInstance* gameInstance() { return CastChecked<UPunGameInstance>(GetGameInstance()); }

	/*
	 * Settings Tab Button Highlight
	 */
	template<class UType, class UParam>
	UType* GetFellowChild(UParam* self, int32 startIndex = 0) {
		TArray<UWidget*> childrens = self->GetParent()->GetAllChildren();
		for (int32 i = startIndex; i < childrens.Num(); i++) { // Skip the background image at 0 index
			if (UType* image = Cast<UType>(childrens[i])) {
				return image;
			}
		}
		UE_DEBUG_BREAK();
		return nullptr;
	}

	void SetButtonHighlight(UButton* button, bool highlighted) {
		GetFellowChild<UImage>(button, 1)->SetVisibility(highlighted ? ESlateVisibility::SelfHitTestInvisible : ESlateVisibility::Hidden);
	}

	/*
	 * Children Pooling
	 */
	template<class TChild, class TBox>
	TChild* GetBoxChild(TBox* box, int32& index, UIEnum uiEnum, bool isHitTestable = false)
	{
		if (index >= box->GetChildrenCount()) {
			box->AddChild(AddWidget<TChild>(uiEnum));
		}
		auto completionIcon = CastChecked<TChild>(box->GetChildAt(index));
		completionIcon->SetVisibility(isHitTestable ? ESlateVisibility::Visible : ESlateVisibility::HitTestInvisible);
		index++;
		return completionIcon;
	}

	template<class TBox>
	static void BoxAfterAdd(TBox* box, const int32& index)
	{
		// Set the rest invisible
		for (int32 i = index; i < box->GetChildrenCount(); i++) {
			box->GetChildAt(i)->SetVisibility(ESlateVisibility::Collapsed);
		}
		box->SetVisibility(index > 0 ? ESlateVisibility::SelfHitTestInvisible : ESlateVisibility::Collapsed);
	}

	/*
	 * Helpers
	 */

	FVector2D GetViewportPosition(const FGeometry& MyGeometry)
	{
		FVector2D pixelPos;
		FVector2D viewportPos;
		USlateBlueprintLibrary::AbsoluteToViewport(this, MyGeometry.GetAbsolutePosition(), pixelPos, viewportPos);
		return viewportPos;
	}

	const FGeometry& GetViewportGeometry()
	{
		UWorld* World = GetWorld();
		check(World && World->IsGameWorld());

		UGameViewportClient* ViewportClient = World->GetGameViewport();
		check(ViewportClient);

		TSharedPtr<IGameLayerManager> GameLayerManager = ViewportClient->GetGameLayerManager();
		check(GameLayerManager.IsValid());

		FVector2D ViewportSize;
		ViewportClient->GetViewportSize(ViewportSize);

		return GameLayerManager->GetViewportWidgetHostGeometry();
	}

	float _lastOpened = 0.0f;
	void Spawn2DSound(std::string groupName, std::string soundName) {
		if (UGameplayStatics::GetTimeSeconds(this) - _lastOpened < 0.5f) {
			return;
		}
		if (dataSource()) {
			dataSource()->Spawn2DSound(groupName, soundName);
		}
		else {
			gameInstance()->Spawn2DSound(groupName, soundName);
		}
	}


	void AddResourceTooltip(UWidget* widget, ResourceEnum resourceEnum, bool skipWidgetHoverCheck = false);

	void AddTradeOfferTooltip(UWidget* widget, bool isImport, ResourceEnum resourceEnum, int32 resourceCount, int32 orderFulfilled);

	template<typename Func>
	void ExecuteFillScoreBreakdownText(Func func, int32 playerIdIn)
	{
		auto& sim = simulation();

		func(NSLOCTEXT("ScoreBreakdown", "Difficulty:", "Difficulty:"), DifficultyLevelNames[static_cast<int>(simulation().mapSettings().difficultyLevel)]);
		
		auto populationScoreWidget = func(NSLOCTEXT("ScoreBreakdown", "Population Score:", "Population Score:"), TEXT_NUM(sim.populationScore(playerIdIn)));
		auto happinessScoreWidget = func(NSLOCTEXT("ScoreBreakdown", "Happiness Score:", "Happiness Score:"), TEXT_NUM(sim.happinessScore(playerIdIn)));
		auto moneyScoreWidget = func(NSLOCTEXT("ScoreBreakdown", "Money Score:", "Money Score:"), TEXT_NUM(sim.moneyScore(playerIdIn)));
		auto technologyScoreWidget = func(NSLOCTEXT("ScoreBreakdown", "Technology Score:", "Technology Score:"), TEXT_NUM(sim.technologyScore(playerIdIn)));
		auto wonderScoreWidget = func(NSLOCTEXT("ScoreBreakdown", "Wonder Score:", "Wonder Score:"), TEXT_NUM(sim.wonderScore(playerIdIn)));

		AddToolTip(populationScoreWidget, NSLOCTEXT("ScoreBreakdown", "Population Score tip", "Population Score = Population X 1"));
		AddToolTip(happinessScoreWidget, NSLOCTEXT("ScoreBreakdown", "Happiness Score tip", "Happiness Score = (Average Happiness above 80%) X Population / 10"));
		AddToolTip(moneyScoreWidget, NSLOCTEXT("ScoreBreakdown", "Money Score tip", "Money Score = Money / 1000"));
		AddToolTip(technologyScoreWidget, NSLOCTEXT("ScoreBreakdown", "Technology Score tip", "Technology Score = Technology X 10"));
		AddToolTip(wonderScoreWidget, NSLOCTEXT("ScoreBreakdown", "Wonders Score tip", "Wonders Score = Sum of score from all Wonders"));
	}
	

	template<typename T>
	static T* FindChildRecursive(UPanelWidget* widget)
	{
		for (int32 i = 0; i < widget->GetChildrenCount(); i++)
		{
			if (T* result = Cast<T>(widget->GetChildAt(i))) {
				return result;
			}

			if (UPanelWidget* panelWidget = Cast<UPanelWidget>(widget->GetChildAt(i))) {
				if (T* result = FindChildRecursive<T>(panelWidget)) {
					return result;
				}
			}
		}
		
		return nullptr;
	}

	template<typename T>
	static void FindChildrenRecursive(UPanelWidget* widget, TArray<T*>& results)
	{
		for (int32 i = 0; i < widget->GetChildrenCount(); i++)
		{
			if (T* result = Cast<T>(widget->GetChildAt(i))) {
				results.Add(result);
			}

			if (UPanelWidget* panelWidget = Cast<UPanelWidget>(widget->GetChildAt(i))) {
				FindChildrenRecursive<T>(panelWidget, results);
			}
		}
	}
	

public:

	virtual void CallBack1(UPunWidget* punWidgetCaller, CallbackEnum callbackEnum) {}
	virtual void CallBack2(UPunWidget* punWidgetCaller, CallbackEnum callbackEnum) {}

public:
	int32 punId = -1; // Any attached objectId... optional
	int32 callbackVar1 = -1;
	int32 callbackVar2 = -1;
	int32 callbackVar3 = -1;
	int32 callbackVar4 = -1;

	UIEnum uiEnum = UIEnum::MainGame;
	float startTime = -1;

	int32 uiHash = -1;

	bool justInitializedUI = false;

protected:
	UPROPERTY() TMap<FString, UWidgetAnimation*> Animations;
	
private:
	UPROPERTY() TScriptInterface<IPunHUDInterface> _punHUD;
};