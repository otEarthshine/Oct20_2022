// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "UnrealEngine.h"
#include "UISystemBase.h"
#include "Styling/SlateStyle.h"

#include "PunWidget.h"

#include <unordered_map>
#include <functional>
#include "Components/WidgetComponent.h"
#include "PunCity/MapUtil.h"
#include "BuildingJobUI.h"

#include "WorldSpaceUI.generated.h"

USTRUCT()
struct FWidgetComponentArray
{
	GENERATED_BODY();

	UPROPERTY() TArray<UWidgetComponent*> widgetArray;
};

USTRUCT()
struct FHoverUIs
{
	GENERATED_BODY()
	
	UPROPERTY() TArray<FWidgetComponentArray> uiEnumToHoverWidgetComps;
	UPROPERTY() TArray<FWidgetComponentArray> uiEnumToWidgetPool;

	void Init()
	{
		uiEnumToHoverWidgetComps.SetNum(UIEnumCount);
		uiEnumToWidgetPool.SetNum(UIEnumCount);
	}

	// TODO: automatic despawn when nothing does GetHoverUI...

	template<typename T>
	T* GetHoverUI(int objectId, UIEnum uiEnum, UPunWidget* punWidget, USceneComponent* parent, FVector worldLocation, float zoomAmount, 
					std::function<void(T*)> onInit, float zoomThreshold = WorldZoomTransition_WorldSpaceUIShrink, float scale = 1.0f)
	{
		PUN_CHECK(objectId != -1);
		_displayIds.push_back(objectId);

		if (zoomAmount > zoomThreshold) {
			scale = zoomThreshold / zoomAmount;
		}

		int32 uiEnumInt = static_cast<int32>(uiEnum);
		TArray<UWidgetComponent*>& hoverWidgetComps = uiEnumToHoverWidgetComps[uiEnumInt].widgetArray;

		// Use the same widget if possible
		for (int i = 0; i < hoverWidgetComps.Num(); i++) {
			T* hoverWidget = CastChecked<T>(hoverWidgetComps[i]->GetUserWidgetObject());
			if (hoverWidget->punId == objectId) {
				hoverWidgetComps[i]->GetUserWidgetObject()->SetRenderScale(FVector2D(scale, scale));
				hoverWidgetComps[i]->SetWorldLocation(worldLocation);
				return hoverWidget;
			}
		}

		/*
		 * Spawn
		 */
		TArray<UWidgetComponent*>& poolArray = uiEnumToWidgetPool[uiEnumInt].widgetArray;

		UWidgetComponent* widgetComp = nullptr;
		if (poolArray.Num() > 0)
		{
			widgetComp = poolArray.Pop(false);
			widgetComp->SetVisibility(true);
		}
		else
		{
			// Add new widget if needed
			widgetComp = NewObject<UWidgetComponent>(parent);
			//widgetComp->SetInitialLayerZOrder(zOrder);
			widgetComp->AttachToComponent(parent, FAttachmentTransformRules::KeepRelativeTransform, NAME_None);
			widgetComp->RegisterComponent();
			widgetComp->SetGenerateOverlapEvents(false);
			widgetComp->SetMobility(EComponentMobility::Movable);
			widgetComp->SetWidgetSpace(EWidgetSpace::Screen);

			T* widget = punWidget->AddWidget<T>(uiEnum);
			widgetComp->SetWidget(widget);
		}
		
		widgetComp->SetWorldLocation(worldLocation);

		auto widget = CastChecked<T>(widgetComp->GetUserWidgetObject());
		widget->punId = objectId;
		onInit(widget);
		widget->SetRenderScale(FVector2D(scale, scale));

		DEBUG_AI_VAR(WorldSpaceUICreate);

		hoverWidgetComps.Add(widgetComp);

		return widget;
	}

	void AfterAdd()
	{
		DespawnUnusedUIs();
		_displayIds.clear();
	}

private:
	// Used to determine unused UI to despawn
	std::vector<int32> _displayIds;

	void DespawnUnusedUIs()//, std::function<bool(int)> shouldRemove)
	{
		for (int32 uiEnumInt = 0; uiEnumInt < uiEnumToHoverWidgetComps.Num(); uiEnumInt++)
		{
			TArray<UWidgetComponent*>& hoverWidgetComps = uiEnumToHoverWidgetComps[uiEnumInt].widgetArray;
			
			for (int i = hoverWidgetComps.Num(); i--;)
			{
				UPunWidget* widget = CastChecked<UPunWidget>(hoverWidgetComps[i]->GetUserWidgetObject());
				int objectId = widget->punId;
				check(objectId != -1);

				// Remove display if the unit is out of range
				if (find(_displayIds.begin(), _displayIds.end(), widget->punId) == _displayIds.end()) {
					//hoverWidgetComps[i]->DestroyComponent();

					hoverWidgetComps[i]->SetVisibility(false);
					uiEnumToWidgetPool[uiEnumInt].widgetArray.Add(hoverWidgetComps[i]);
					hoverWidgetComps.RemoveAt(i);
				}
			}
		}
	}

};

// Right now it is hardcoded for building...
USTRUCT()
struct FFloatupUIs
{
	GENERATED_BODY();

	void Init(UPunWidget* basePunWidget, USceneComponent* parent) {
		_basePunWidget = basePunWidget;
		_parent = parent;

		_uiEnumToAliveWidgetComps.SetNum(UIEnumCount);
		_uiEnumToObjectAtoms.resize(UIEnumCount);
		_uiEnumToWidgetComponentPool.SetNum(UIEnumCount);
	}

	template<typename T>
	T* AddHoverUI(UIEnum uiEnum, WorldAtom2 objectAtom, std::function<void(T*)> onInit = [&](T* ui) {})
	{
		int32 uiEnumInt = static_cast<int32>(uiEnum);
		
		TArray<UWidgetComponent*>& poolArray = _uiEnumToWidgetComponentPool[uiEnumInt].widgetArray;

		UWidgetComponent* widgetComp = nullptr;
		if (poolArray.Num() > 0)
		{
			widgetComp = poolArray.Pop(false);
			widgetComp->SetVisibility(true);
		}
		else
		{
			// Add new widget if needed
			widgetComp = NewObject<UWidgetComponent>(_parent);
			widgetComp->AttachToComponent(_parent, FAttachmentTransformRules::KeepRelativeTransform, NAME_None);
			widgetComp->RegisterComponent();
			widgetComp->SetWidgetSpace(EWidgetSpace::Screen);

			T* widget = _basePunWidget->AddWidget<T>(uiEnum);
			widgetComp->SetWidget(widget);
		}
		check(widgetComp);

		SetHoverLocation(widgetComp, objectAtom);

		auto widget = CastChecked<T>(widgetComp->GetUserWidgetObject());
		onInit(widget);
		
		// Animation
		{
			UWidgetAnimation* animation = widget->GetAnimation("Floatup");
			PUN_CHECK(animation);
			widget->PlayAnimation(animation);
			widget->startTime = UGameplayStatics::GetTimeSeconds(widget);
		}

		_uiEnumToAliveWidgetComps[uiEnumInt].widgetArray.Add(widgetComp);
		_uiEnumToObjectAtoms[uiEnumInt].push_back(objectAtom);

		return widget;
	}

	void Tick()
	{
		for (int32 uiEnumInt = 0; uiEnumInt < _uiEnumToAliveWidgetComps.Num(); uiEnumInt++)
		{
			TArray<UWidgetComponent*>& aliveWidgetComps = _uiEnumToAliveWidgetComps[uiEnumInt].widgetArray;
			std::vector<WorldAtom2>& objectAtoms = _uiEnumToObjectAtoms[uiEnumInt];
			
			for (int i = aliveWidgetComps.Num(); i--;) {
				UPunWidget* hoverWidget = CastChecked<UPunWidget>(aliveWidgetComps[i]->GetUserWidgetObject());

				// Update location
				SetHoverLocation(aliveWidgetComps[i], objectAtoms[i]);

				// Remove if needed
				if (hoverWidget->GetAge() > 5.0f) 
				{
					aliveWidgetComps[i]->SetVisibility(false);
					_uiEnumToWidgetComponentPool[uiEnumInt].widgetArray.Add(aliveWidgetComps[i]);
					
					aliveWidgetComps.RemoveAt(i);
					objectAtoms.erase(objectAtoms.begin() + i);
				}

			}
		}
	}

private:
	void SetHoverLocation(UWidgetComponent* hoverWidgetComp, WorldAtom2 objectAtom)
	{
		auto punHUD = _basePunWidget->GetPunHUD();
		FVector displayLocation = MapUtil::DisplayLocation(punHUD->inputSystemInterface()->cameraAtom(), objectAtom, 10.0f);
		hoverWidgetComp->SetWorldLocation(displayLocation);
	}

private:
	UPROPERTY() UPunWidget* _basePunWidget = nullptr;
	UPROPERTY() USceneComponent* _parent = nullptr;

	UPROPERTY() TArray<FWidgetComponentArray> _uiEnumToAliveWidgetComps;
	std::vector<std::vector<WorldAtom2>> _uiEnumToObjectAtoms;
	
	UPROPERTY() TArray<FWidgetComponentArray> _uiEnumToWidgetComponentPool;
};

/**
 * 
 */
UCLASS()
class UWorldSpaceUI : public UPunWidget
{
	GENERATED_BODY()
public:
	void SetupClasses(TSharedPtr<class FSlateStyleSet> style, USceneComponent* worldWidgetParent);
	
	void TickWorldSpaceUI();

	void AddFloatupInfo(FloatupInfo floatupInfo) {
		_floatupInfos.push_back(floatupInfo);
	}

	// Call this, then stop ticking to hide UI
	void StartHideAllUI()
	{
		_buildingJobUIs.AfterAdd();
		
		_townhallHoverInfos.AfterAdd();
		_townhallHoverInfos.AfterAdd(); // Double times to ensure there townhallHover is gone.
		
		_regionHoverUIs.AfterAdd();
		
		_buildingHoverIcons.AfterAdd();
		_iconTextHoverIcons.AfterAdd();

		_unitHoverIcons.AfterAdd();
		_mapHoverIcons.AfterAdd();
	}

public:
	enum class JobUIState {
		None,
		Job,
		Home,
	};
	JobUIState jobUIState = JobUIState::None;

	bool townhallUIActive = true;

private:
	void TickBuildings();
	void TickJobUI(int buildingId);
	void TickTownhallInfo(int buildingId, bool isMini = false);
	void TickRegionUIs();

	void TickUnits();

	void TickMap();

	void TickPlacementInstructions();

	FVector2D buildingScreenLocation(int buildingId);
	FVector GetBuildingTrueCenterDisplayLocation(int buildingId, float height = 50.0f);

	UBuildingJobUI* GetJobUI(int32 buildingId, int32 height);

private:

	//! Show building's job status
	UPROPERTY() FHoverUIs _buildingJobUIs;
	UPROPERTY() FHoverUIs _townhallHoverInfos;
	UPROPERTY() FHoverUIs _regionHoverUIs; // Claim Land

	//! Show house/starving/etc. warning icons
	TSharedPtr<FSlateStyleSet> _style;
	TMap<FString, const FSlateBrush*> _brushes;

	UPROPERTY() FHoverUIs _buildingHoverIcons;
	UPROPERTY() FHoverUIs _iconTextHoverIcons;
	UPROPERTY() FHoverUIs _unitHoverIcons;
	UPROPERTY() FHoverUIs _mapHoverIcons;

	UPROPERTY() FFloatupUIs _floatupUIs;


	UPROPERTY() USceneComponent* _worldWidgetParent;
	UPROPERTY() UWidgetComponent* _aboveBuildingTextWidget;
	float _aboveBuildingTextSpawnTime = -1.0f;

	std::vector<FloatupInfo> _floatupInfos;
};
