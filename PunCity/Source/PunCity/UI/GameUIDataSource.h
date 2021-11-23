// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "UObject/Interface.h"
#include "../Simulation/GameSimulationInfo.h"
#include "../DisplaySystem/GameDisplayInfo.h"
#include "PunCity/NetworkStructs.h"
#include "Kismet/GameplayStatics.h"
#include "PunCity/Simulation/Buildings/TownHall.h"
#include "Components/Widget.h"
#include "Components/Button.h"
#include "Components/CheckBox.h"
#include "Components/SizeBox.h"
#include "Components/TextBlock.h"
#include "Components/HorizontalBox.h"
#include "Components/Image.h"

#include "GameUIDataSource.generated.h"

class Building;

enum class UIEnum
{
	None,
	MainGame,
	EscMenu,
	Popup,
	Quest,

	Chat,
	TopLayerUI,
	Policy,
	Trade,
	TradeIntercity,
	TargetConfirm,
	InitialResourceUI,

	SendImmigrantsUI,
	DiplomacyUI,
	TrainUnitsUI,
	GiftResourceUI,

	TownAutoTradeUI,
	
	DragCardSlot,
	DragCard,

	BuildingPlacementButton,
	CardMini,
	CardSlot,
	//ResearchTypeButton,

	ToolTip,
	TradeRow,
	TradeRowIntercity,
	QuestElement,
	PlayerCompareElement,
	PlayerCompareDetailedElement,
	ResourceStatTableRow,
	BuildingStatTableRow,

	TownAutoTradeRow,
	TownTradeOffersRow,

	//ArmyUI,
	//ArmyRow,

	WorldSpaceUI,
	HoverIcon,
	HoverBuildingJob,
	JobHumanIcon,
	HoverTownhall,
	BuffIcon,
	
	ResourceCompletionIcon,
	BuildingReadyIcon,
	BuildingNeedSetupIcon,
	BuildingNoStorageIcon,
	RegionHoverUI,

	ObjectDescription,
	ObjectDescriptionSystem,
	ChooseResourceElement,
	ManageStorageElement,
	StatisticsUI,

	IconTextPair,
	BuildingResourceChain,
	PunTextWidget,
	PunSpacerWidget,
	PunLineSpacerWidget,
	PunThinLineSpacerWidget,
	PunScrollBoxWidget,
	PunButton,
	PunDropdown,
	PunBudgetAdjuster,
	PunEditableNumberBox,
	PunEditableNumberBoxHorizontal,
	PunGraph,
	PunThinGraph,
	PunTutorialLink,

	AboveBuildingText,
	HoverTextIconPair,
	HoverTextIconPair3Lines,
	
	ComboComplete,
	BuildingComplete,
	HouseUpgrade,
	HouseDowngrade,
	TownhallUpgrade,

	PunRichText,
	PunRichTextTooltipWrap,
	PunRichTextQuestWrap,
	PunRichTextCenter,
	PunRichText_Chat,
	PunRichText_Popup,
	PunRichTextTwoSided,
	PunRichTextBullet_QuestWrap,
	//PunEventText,

	PunItemIcon,
	PunItemSelectionChoice,
	
	TechTreeUI,
	TechEraUI,
	TechBox,

	ProsperityUI,
	ProsperityColumnUI,
	ProsperityBoxUI,

	JobPriorityRow,
	
	SaveSelection,

	PlayerListElement,
	LobbyListElement,

	HiddenSettingsRow,

	//ArmyDeployButton,
	//ArmyLinesUILeft,
	//ArmyLinesUIRight,
	//ArmyUnitLeft,
	//ArmyUnitRight,
	//ArmyMoveRow,
	//ArmyMoveUI,
	//ArmyChooseNodeButton,
	DamageFloatup,

	PunMidRowText,
	TradeDealResourceRow,

	ChooseCharacterElement,
	ChooseLogoElement,
	ChooseColorElement,

	WG_MinorTownWorldUI,
	WG_BattlefieldUI,
	WG_BattlefieldArmyUI,
	WG_BattlefieldUnitIcon,

	WGT_Button,

	WGT_ObjectFocus_Divider,
	WGT_ObjectFocus_FlavorText, // PunTextWidget cpp
	WGT_ObjectFocus_ProvinceTitle, // WGT_ObjectFocus_Title_Cpp
	WGT_ObjectFocus_SectionTitle, // PunTextWidget cpp
	WGT_ObjectFocus_Subheader, // PunTextWidget cpp
	WGT_ObjectFocus_TextLeft, // PunTextWidget cpp
	WGT_ObjectFocus_Title,
	WGT_ObjectFocus_TextRow, // WGT_ObjectFocus_TextRow cpp
	WGT_ObjectFocus_InventoryEntry, // WGT_ObjectFocus_TextRow cpp
	WGT_Focus_EditableTextRow,

	// Unused:
	// WGT_ObjectFocus_ProductionTarget

	Count,
};
static const int UIEnumCount = static_cast<int>(UIEnum::Count);


USTRUCT()
struct FPlayerInfo
{
	GENERATED_BODY();

	static const uint64 InvalidSteamId64 = MAX_uint64;

	UPROPERTY() FText name;
	UPROPERTY() uint64 steamId64 = InvalidSteamId64;
	int32 playerId = -1;

	UPROPERTY() int32 logoIndex = 0;
	UPROPERTY() FLinearColor logoColorBackground = FLinearColor::Black;
	UPROPERTY() FLinearColor logoColorForeground = FLinearColor::Yellow;
	UPROPERTY() FString portraitName = "DefaultWhiteMale";
	UPROPERTY() int32 factionIndex = 0;
	
	bool IsEmpty() const { return steamId64 == InvalidSteamId64; }
	

	FactionEnum factionEnum() const { return static_cast<FactionEnum>(factionIndex); }

	friend FArchive& operator<<(FArchive& Ar, FPlayerInfo& object)
	{
		Ar << object.name;
		Ar << object.steamId64;
		Ar << object.playerId;
		
		Ar << object.logoIndex;
		Ar << object.logoColorBackground;
		Ar << object.logoColorForeground;
		Ar << object.portraitName;
		Ar << object.factionIndex;
		return Ar;
	}
};


// This class does not need to be modified.
UINTERFACE(MinimalAPI)
class UGameUIDataSource : public UInterface
{
	GENERATED_BODY()
};


class LaborerPriorityState
{
public:
	float lastPriorityInputTime = -999.0f;

	FSetTownPriority townPriorityState;
	int32 laborerCount = -1;
	int32 builderCount = -1;
	int32 roadMakerCount = -1;

	void TrySyncToSimulation(IGameSimulationCore* sim, int32 townId, UWidget* widget)
	{
		if (UGameplayStatics::GetTimeSeconds(widget) > lastPriorityInputTime + 3.0f) {
			SyncState(sim, townId);
		}
	}

	void SyncState(IGameSimulationCore* sim, int32 townId)
	{
		auto& townManager = sim->townManager(townId);
		townPriorityState = *(townManager.CreateTownPriorityCommand());
		laborerCount = std::max(townManager.laborerCount(), 0); // Requires clamp since laborerCount() may be negative when someone died
		builderCount = std::max(townManager.builderCount(), 0);
		roadMakerCount = std::max(townManager.roadMakerCount(), 0);
	}

	void RefreshUILaborerPriority(
		class UPunWidget* widget,
		IGameSimulationCore* sim,
		int32 townId,

		UHorizontalBox* EmployedBox,
		UTextBlock* Employed,

		UHorizontalBox* LaborerBox,
		UCheckBox* LaborerManualCheckBox, USizeBox* LaborerArrowOverlay,
		UTextBlock* Laborer,
		UTextBlock* LaborerRed,

		UHorizontalBox* BuilderBox,
		UCheckBox* BuilderManualCheckBox,
		UTextBlock* Builder,
		USizeBox* BuilderArrowOverlay
	);
	
};


/**
 * 
 */
class IGameUIDataSource
{
	GENERATED_BODY()

	// Add interface functions to this class. This is the class that will be inherited to implement this interface.
public:
	virtual int32 playerId() = 0;
	virtual FPlayerInfo playerInfo(int32 playerId) = 0;
	virtual FString playerNameF(int32 playerId) = 0;

	virtual class FMapSettings GetMapSettings() = 0;
	
	virtual int GetResourceCount(int32 playerId, ResourceEnum resourceEnum) = 0;
	virtual int GetResourceCount(int32 playerId, const std::vector<ResourceEnum>& resourceEnums) = 0;

	//virtual int GetBuildingResourceCount(int32_t playerId, ResourceEnum resourceEnum, )

	virtual FVector GetBuildingTrueCenterDisplayLocation(int objectId, WorldAtom2 cameraAtom) = 0;
	virtual class Building& GetBuilding(int objectId) = 0;

	virtual const std::vector<int32>& sampleRegionIds() = 0;
	virtual const std::vector<int32>& sampleNearRegionIds() = 0;

	virtual FVector GetUnitDisplayLocation(int objectId, WorldAtom2 cameraAtom) = 0;
	virtual class UnitStateAI& GetUnitStateAI(int32 unitId) = 0;
	virtual bool IsUnitValid(int32 unitId) = 0;

	virtual class GameSimulationCore& simulation() = 0;
	virtual class GameSimulationCore* simulationPtr() = 0;
	virtual bool HasSimulation() = 0;
	
	virtual class IUnitDataSource& unitDataSource() = 0;

	virtual const class GameDisplayInfo& displayInfo() = 0;
	virtual UnitDisplayState GetUnitTransformAndVariation(UnitStateAI& unit, FTransform& transform) = 0;

	// For making mesh UI
	virtual AActor* actorToAttach() = 0;
	virtual USceneComponent* componentToAttach() = 0;
	virtual class UAssetLoaderComponent* assetLoader() = 0;

	virtual FVector DisplayLocation(WorldAtom2 atom) = 0;
	virtual FVector DisplayLocationTrueCenter(Building& building, bool withHeight = false) = 0;
	//virtual FVector DisplayLocationMapMode(WorldAtom2 atom) = 0;

	virtual void SetOverlayType(OverlayType overlayType, OverlaySetterType setterType) = 0;
	virtual WorldTile2 GetOverlayTile() = 0;
	virtual OverlayType GetOverlayType() = 0;

	virtual void SetOverlayHideTree(bool isHiding) = 0;
	virtual void SetOverlayProvince(bool showProvinceOverlay) = 0;
	//virtual void SetOverlayDefense(bool showOverlay) = 0;

	virtual bool isCtrlDown() = 0;
	virtual bool isShiftDown() = 0;

	virtual const std::vector<int32>& sampleProvinceIds() = 0;

	virtual bool isPhotoMode() = 0;

	virtual ResourceEnum tipResourceEnum() = 0;
	virtual void SetTipResourceEnum(ResourceEnum resourceEnum) = 0;

	virtual bool alwaysShowProvinceHover() = 0;

	virtual bool isHidingTree() = 0;
	virtual bool isShowingProvinceOverlay() = 0;
	virtual bool isShowingDefenseNodes() = 0;
	
	virtual float zoomDistance() = 0;
	virtual bool ZoomDistanceBelow(float threshold) = 0;
	bool ZoomDistanceAbove(float threshold) { return !ZoomDistanceBelow(threshold); }
	
	virtual WorldAtom2 cameraAtom() = 0;
	virtual FVector cameraLocation() = 0;

	virtual class USoundSystemComponent* soundSystem() = 0;
	virtual float NearestSeaDistance() = 0;

	virtual int32 GetBuildingDisplayObjectId(int32 meshId, FString protoName, int32 instanceIndex) = 0;
	virtual int32 GetTileObjDisplayObjectId(int32 meshId, FString protoName, int32 instanceIndex) = 0;

	//virtual void SetObjectFocusSound(bool active, USoundCue* soundCue = nullptr, FVector location = FVector::ZeroVector) = 0;

	virtual void ShowBuildingMesh(Building& building, int customDepth = 0, bool receiveDecal = true) = 0;
	virtual void ShowBuildingMesh(WorldTile2 tile, Direction faceDirection, const std::vector<ModuleTransform>& modules, int32 customDepthIndex, bool receiveDecal = true) = 0;
	virtual void ShowStorageMesh(TileArea area, WorldTile2 centerTile, int customDepth = 0) = 0;
	
	virtual class UDecalComponent* ShowDecal(TileArea area, UMaterial* material) = 0; // TODO: remove?
	virtual class UDecalComponent* ShowDecal(TileArea area, UMaterial* material, TArray<UDecalComponent*>& decals, int32& decalCount, bool useMaterialInstance = false) = 0;

	virtual void ShowDeliveryArrow(FVector start, FVector end, bool isYellow = false, bool isSolid = false) = 0;

	virtual bool IsInSampleRange(WorldTile2 tile) = 0;

	// Audio
	virtual void SpawnAnimalSound(UnitEnum unitEnum, bool isAngry, WorldAtom2 worldAtom, bool usePlayProbability = false) = 0;
	virtual void Spawn3DSound(std::string groupName, std::string soundName, WorldAtom2 worldAtom, float height) = 0;
	//virtual void Spawn2DSound(std::string groupName, std::string soundName, int32 playerId, WorldTile2 tile = WorldTile2::Invalid) = 0; // UI's triggered sound don't need filter
	virtual void Spawn2DSound(std::string groupName, std::string soundName) = 0;
};


USTRUCT()
struct FSaveThreadResults
{
	GENERATED_BODY();
	UPROPERTY() int32 checksum = -1;
	UPROPERTY() int32 compressedDataSize = -1;
	UPROPERTY() bool succeed = false;
};


class PunUIUtils
{
public:
	static void SetTownSwapText(int32 townId, IGameSimulationCore* sim, UTextBlock* TownSwapText, UHorizontalBox* TownSwapHorizontalBox)
	{
		if (townId != -1) {
			int32 townPlayerId = sim->townPlayerId(townId);
			if (sim->GetTownIds(townPlayerId).size() > 1) {
				TownSwapText->SetText(sim->townNameT(townId));
				TownSwapHorizontalBox->SetVisibility(ESlateVisibility::SelfHitTestInvisible);
			}
			else {
				TownSwapHorizontalBox->SetVisibility(ESlateVisibility::Collapsed);
			}
		}
		else {
			TownSwapHorizontalBox->SetVisibility(ESlateVisibility::Collapsed);
		}
	}

	static void SetPlayerLogo(UMaterialInstanceDynamic* material, const FPlayerInfo& playerInfo, UAssetLoaderComponent* assetLoader)
	{
		material->SetVectorParameterValue("ColorBackground", playerInfo.logoColorBackground);
		material->SetVectorParameterValue("ColorForeground", playerInfo.logoColorForeground);
		material->SetTextureParameterValue("Logo", assetLoader->GetPlayerLogo(playerInfo.logoIndex));
	}

	static void SetPlayerLogo(UImage* foregroundImage, UImage* backgroundImage, const FPlayerInfo& playerInfo, UAssetLoaderComponent* assetLoader)
	{
		backgroundImage->GetDynamicMaterial()->SetVectorParameterValue("ColorBackground", playerInfo.logoColorBackground);
		foregroundImage->GetDynamicMaterial()->SetVectorParameterValue("ColorForeground", playerInfo.logoColorForeground);
		foregroundImage->GetDynamicMaterial()->SetTextureParameterValue("Logo", assetLoader->GetPlayerLogo(playerInfo.logoIndex));
	}
};