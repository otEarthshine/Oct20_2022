#pragma once

#include "CoreMinimal.h"
#include "PunCity/Simulation/GameSimulationInfo.h"
#include "PunUnrealUtils.h"



/*
 * Map settings sent across network in the lobby
 */
class FMapSettings
{
public:
	FString mapSeed = FString("seed");
	int32 mapSizeEnumInt = static_cast<int>(MapSizeEnum::Medium);
	MapSeaLevelEnum mapSeaLevel = MapSeaLevelEnum::Medium;
	MapMoistureEnum mapMoisture = MapMoistureEnum::Medium;
	MapTemperatureEnum mapTemperature = MapTemperatureEnum::Medium;
	MapMountainDensityEnum mapMountainDensity = MapMountainDensityEnum::Medium;
	
	int32 playerCount = 0;
	int32 aiCount = 15;
	DifficultyLevel difficultyLevel = DifficultyLevel::Normal;

	bool isSinglePlayer = false;

	bool isMultiplayer() { return !isSinglePlayer; }

	bool isInitialized() { return playerCount > 0; }

	static FMapSettings GetDefault(bool isSinglePlayerIn) {
		FMapSettings mapSettings;
		mapSettings.playerCount = 6;
		mapSettings.isSinglePlayer = isSinglePlayerIn;
		return mapSettings;
	}

	bool operator==(const FMapSettings& a)
	{
		return mapSeed == a.mapSeed &&
			mapSizeEnumInt == a.mapSizeEnumInt &&
			mapSeaLevel == a.mapSeaLevel &&
			mapMoisture == a.mapMoisture &&
			mapTemperature == a.mapTemperature &&
			mapMountainDensity == a.mapMountainDensity &&

			playerCount == a.playerCount &&
			aiCount == a.aiCount &&
			difficultyLevel == a.difficultyLevel;
	}
	bool operator!=(const FMapSettings& a)
	{
		return !(*this == a);
	}

	bool MapEquals(const FMapSettings& a)
	{
		return mapSeed == a.mapSeed &&
			mapSizeEnumInt == a.mapSizeEnumInt &&
			mapSeaLevel == a.mapSeaLevel &&
			mapMoisture == a.mapMoisture &&
			mapTemperature == a.mapTemperature &&
			mapMountainDensity == a.mapMountainDensity;
	}


	std::string mapSeedStd() { return ToStdString(mapSeed); }


	void Serialize(PunSerializedData& Ar) {
		Ar << mapSeed;
		Ar << mapSizeEnumInt;
		Ar << mapSeaLevel;
		Ar << mapMoisture;
		Ar << mapTemperature;
		Ar << mapMountainDensity;
		
		Ar << playerCount;
		Ar << aiCount;
		Ar << difficultyLevel;
		Ar << isSinglePlayer;
	}

	MapSizeEnum mapSizeEnum() { return static_cast<MapSizeEnum>(mapSizeEnumInt); }

	std::string ToString()
	{
		std::stringstream ss;
		ss << "[seed:" << ToStdString(mapSeed);
		ss << ", size:" << FTextToStd(MapSizeNames[mapSizeEnumInt]);
		ss << ", seaLevel:" << FTextToStd(MapSettingsLevelNames[static_cast<int>(mapSeaLevel)]);
		ss << ", moisture:" << FTextToStd(MapMoistureNames[static_cast<int>(mapMoisture)]);
		ss << ", temperature:" << FTextToStd(MapSettingsLevelNames[static_cast<int>(mapTemperature)]);
		ss << ", mountain:" << FTextToStd(MapSettingsLevelNames[static_cast<int>(mapMountainDensity)]);
		
		ss << ", playerCount:" << playerCount;
		ss << ", ai:" << aiCount;
		ss << ", mode:" << FTextToStd(DifficultyLevelNames[static_cast<int>(difficultyLevel)]);
		ss << ", singlePlayer:" << isSinglePlayer;
		ss << "]";
		return ss.str();
	}

	void Serialize(FArchive &Ar)
	{
		Ar << mapSeed;
		Ar << mapSizeEnumInt;
		Ar << mapSeaLevel;
		Ar << mapMoisture;
		Ar << mapTemperature;
		Ar << mapMountainDensity;
		
		Ar << playerCount;
		Ar << aiCount;
		Ar << difficultyLevel;
		Ar << isSinglePlayer;
	}
};

/*
 * Victory
 */
enum class GameEndEnum : uint8
{
	None,
	ScienceVictory,
	EconomicVictory,
	DominationVictory,
	ScoreVictory,
};

struct FGameEndStatus
{
public:
	int32 playerId = -1;
	int32 victoriousPlayerId = -1;
	GameEndEnum gameEndEnum = GameEndEnum::None;

	void Serialize(PunSerializedData& blob)
	{
		blob << playerId;
		blob << victoriousPlayerId;
		blob << gameEndEnum;
	}

	void Serialize(FArchive &Ar)
	{
		Ar << playerId;
		Ar << victoriousPlayerId;
		Ar << gameEndEnum;
	}
};

/*
 * Network command
 */

enum class NetworkCommandEnum : uint8
{
	None,
	AddPlayer,

	PlaceBuilding,
	PlaceDrag,
	JobSlotChange,
	SetAllowResource,
	SetPriority,
	SetTownPriority,
	SetGlobalJobPriority,
	GenericCommand,

	TradeResource,
	SetIntercityTrade,
	UpgradeBuilding,
	ChangeWorkMode,
	PopupDecision,
	ChooseLocation,
	ChooseInitialResources,
	
	Cheat,
	RerollCards,
	SelectRareCard,
	BuyCard,
	SellCards,
	UseCard,
	UnslotCard,

	//TrainUnit,

	ClaimLand,
	Attack,
	ChooseResearch,

	ChangeName,
	SendChat,

	ReplayPause,

	Count,
};

static const std::vector<std::string> NetworkCommandNames
{
	"None",
	"AddPlayer",

	"PlaceBuilding",
	"PlaceDrag",
	"JobSlotChanged",
	"SetAllowResource",
	"SetPriority",
	"SetTownPriority",
	"SetGlobalJobPriority",
	"GenericCommand",

	"TradeResource",
	"SetIntercityTrade",
	"UpgradeBuilding",
	"ChangeWorkMode",
	"PopupDecision",
	"ChooseLocation",
	"ChooseInitialResources",
	
	"Cheat",
	"RerollCards",
	"SelectRareCard",
	"BuyCards",
	"SellCards",
	"UseCard",
	"UnslotCard",

	//"TrainUnit",

	"ClaimLand",
	"Attack",
	"ChooseResearch",

	"ChangeName",
	"SendChat",

	"ReplayPause",
};
static std::string GetNetworkCommandName(NetworkCommandEnum commandEnum) {
	check(static_cast<int32>(NetworkCommandEnum::Count) == NetworkCommandNames.size());
	return NetworkCommandNames[static_cast<int>(commandEnum)];
}


class FNetworkCommand
{
public:
	virtual ~FNetworkCommand() {}

	int32 playerId = -1; // Note!!! This is filled after being sent on network
	int32 townId = -1;
	int32 proxyControllerTick = -1;

	virtual NetworkCommandEnum commandType() { return NetworkCommandEnum::None; }

	virtual void Serialize(PunSerializedData& Ar) {
		int32 commandInt = 0;
		if (Ar.isSaving()) {
			commandInt = static_cast<int32>(commandType());
		}
		
		Ar << commandInt;
		Ar << playerId;
		Ar << townId;
		Ar << proxyControllerTick;
	}

	static NetworkCommandEnum GetCommandTypeFromBlob(PunSerializedData& blob) {
		return static_cast<NetworkCommandEnum>(blob[blob.readIndex]);
	}

	virtual FString ToCompactString() {
		return ToFString(GetNetworkCommandName(commandType())) + " [" + FString::FromInt(playerId) + "] pT:" + FString::FromInt(proxyControllerTick) + " ";
	}

	virtual int32 GetTickHash() {
		return playerId + townId + static_cast<int32>(commandType()) + (Time::Ticks() % Time::TicksPerYear);
	}
};

// 
class FBuildingCommand : public FNetworkCommand
{
public:
	virtual ~FBuildingCommand() {}

	int32 buildingId = -1;
	int32 buildingTileId = -1; // Replay Only
	CardEnum buildingEnum = CardEnum::None; // Replay Only

	void Serialize(PunSerializedData& blob) override {
		FNetworkCommand::Serialize(blob);
		
		blob << buildingId;
		blob << buildingTileId;
		blob << buildingEnum;
	}

	virtual int32 GetTickHash() override {
		return FNetworkCommand::GetTickHash() + buildingId + buildingTileId + static_cast<int32>(buildingEnum);
	}

	virtual FString ToCompactString() override {
		return FNetworkCommand::ToCompactString() + " bldId:" + FString::FromInt(buildingId);
	}
};


class FPlaceBuilding final : public FNetworkCommand
{
public:
	virtual ~FPlaceBuilding() {}

	PlacementType placementType = PlacementType::Building;
	int32 buildingEnum = static_cast<int32>(CardEnum::None);
	int32 intVar1 = 0; // Note TrailerMode - Farm: use buildingLevel to specify plant type (need after-edit..)

	TileArea area; // Needed in the case for manipulable area
	TileArea area2;
	WorldTile2 center = WorldTile2::Invalid;
	int32 faceDirection = -1;
	
	bool useBoughtCard = false; //  Note TrailerMode - use useBoughtCard to specify if it is prebuilt
	CardEnum useWildCard = CardEnum::None;
	int32 buildingIdToSetDelivery = -1;

	bool isTrailerPreBuilt() { return static_cast<bool>(area2.minX); }

	NetworkCommandEnum commandType() override { return NetworkCommandEnum::PlaceBuilding; }

	void Serialize(PunSerializedData& blob) override {
		FNetworkCommand::Serialize(blob);

		blob << placementType;
		blob << buildingEnum;
		blob << intVar1;
		
		blob << area;
		blob << area2;
		blob << center;
		blob << faceDirection;
		
		blob << useBoughtCard;
		blob << useWildCard;
		blob << buildingIdToSetDelivery;
	}

	FString ToCompactString() override {
		return ToFString(GetNetworkCommandName(commandType())) + "-" + GetBuildingInfoInt(buildingEnum).nameF();
	}

	virtual int32 GetTickHash() override {
		return FNetworkCommand::GetTickHash()
			+ area.GetHash()
			+ area2.GetHash()
			+ center.GetHash()
			+ static_cast<int32>(faceDirection)
			+ static_cast<int32>(buildingEnum)
			+ static_cast<int32>(useBoughtCard)
			+ static_cast<int32>(useWildCard) + buildingIdToSetDelivery;
	}
};


class FPlaceDrag final : public FNetworkCommand
{
public:
	virtual ~FPlaceDrag() {}
	TileArea area = TileArea::Invalid; // Needed in the case for manipulable area
	TileArea area2 = TileArea::Invalid;
	TArray<int32> path;
	int32 placementType = -1;
	ResourceEnum harvestResourceEnum = ResourceEnum::None;

	bool isTrailerPreBuilt() { return static_cast<bool>(area2.minX); }

	NetworkCommandEnum commandType() override { return NetworkCommandEnum::PlaceDrag; }

	void Serialize(PunSerializedData& blob) override
	{
		FNetworkCommand::Serialize(blob);
		blob << area;
		blob << area2;
		blob << path;
		blob << placementType;
		blob << harvestResourceEnum;
	}
	

	virtual FString ToCompactString() override {
		return FNetworkCommand::ToCompactString() + " placementType:" + FString::FromInt(placementType);
	}
};

class FJobSlotChange final : public FBuildingCommand
{
public:
	virtual ~FJobSlotChange() {}

	int32 allowedOccupants = 0;

	NetworkCommandEnum commandType() final { return NetworkCommandEnum::JobSlotChange; }

	void Serialize(PunSerializedData& blob) final {
		FBuildingCommand::Serialize(blob);
		
		blob << allowedOccupants;
	}
};

class FSetAllowResource final : public FBuildingCommand
{
public:
	virtual ~FSetAllowResource() {}

	ResourceEnum resourceEnum = ResourceEnum::None;
	bool allowed = false;
	int32 target = -1;

	bool isExpansionCommand = false;
	bool expanded = false;

	NetworkCommandEnum commandType() final { return NetworkCommandEnum::SetAllowResource; }

	void Serialize(PunSerializedData& blob) final {
		FBuildingCommand::Serialize(blob);
		
		blob << resourceEnum;
		blob << allowed;
		blob << target;

		blob << isExpansionCommand;
		blob << expanded;
	}
};

class FSetPriority final : public FBuildingCommand
{
public:
	virtual ~FSetPriority() {}

	int32 priority = -1;

	NetworkCommandEnum commandType() final { return NetworkCommandEnum::SetPriority; }
	

	void Serialize(PunSerializedData& blob) final {
		FBuildingCommand::Serialize(blob);
		
		blob << priority;
	}
	

	virtual FString ToCompactString() override {
		return FBuildingCommand::ToCompactString() + " prior:" + FString::FromInt(priority);
	}
};

class FSetGlobalJobPriority final : public FNetworkCommand
{
public:
	virtual ~FSetGlobalJobPriority() {}
	NetworkCommandEnum commandType() final { return NetworkCommandEnum::SetGlobalJobPriority; }

	TArray<int32> jobPriorityList;

	void Serialize(PunSerializedData& blob) override
	{
		FNetworkCommand::Serialize(blob);
		blob << jobPriorityList;
	}
};

class FGenericCommand final : public FNetworkCommand
{
public:
	virtual ~FGenericCommand() {}
	NetworkCommandEnum commandType() final { return NetworkCommandEnum::GenericCommand; }

	CallbackEnum callbackEnum = CallbackEnum::None;
	
	enum class Type : uint8 {
		SendGift,
		SetProduceUntil,
		SendImmigrants,
	} genericCommandType = Type::SendGift;

	int32 intVar1 = -1;
	int32 intVar2 = -1;
	int32 intVar3 = -1;
	int32 intVar4 = -1;
	int32 intVar5 = -1;
	int32 intVar6 = -1;
	int32 intVar7 = -1;
	int32 intVar8 = -1;
	
	TArray<int32> array1;
	TArray<int32> array2;
	TArray<int32> array3;
	TArray<int32> array4;
	TArray<int32> array5;
	TArray<int32> array6;
	TArray<int32> array7;
	TArray<int32> array8;

	void Serialize(PunSerializedData& blob) override
	{
		FNetworkCommand::Serialize(blob);
		blob << callbackEnum;
		blob << genericCommandType;
		blob << intVar1;
		blob << intVar2;
		blob << intVar3;
		blob << intVar4;
		blob << intVar5;
		blob << intVar6;
		blob << intVar7;
		blob << intVar8;

		blob << array1;
		blob << array2;
		blob << array3;
		blob << array4;
		blob << array5;
		blob << array6;
		blob << array7;
		blob << array8;
	}

	bool operator==(const FGenericCommand& a)
	{
		return callbackEnum == a.callbackEnum ||
			genericCommandType == a.genericCommandType ||
			intVar1 == a.intVar1 ||
			intVar2 == a.intVar2 ||
			intVar3 == a.intVar3 ||
			intVar4 == a.intVar4 ||
			intVar5 == a.intVar5 ||
			intVar6 == a.intVar6 ||
			intVar7 == a.intVar7 ||
			intVar8 == a.intVar8 ||

			array1 == a.array1 ||
			array2 == a.array2 ||
			array3 == a.array3 ||
			array4 == a.array4 ||
			array5 == a.array5 ||
			array6 == a.array6 ||
			array7 == a.array7 ||
			array8 == a.array8;
	}
};

class FSetTownPriority final : public FNetworkCommand
{
public:
	virtual ~FSetTownPriority() {}

	bool laborerPriority = false;
	bool builderPriority = false;
	bool roadMakerPriority = false;
	int32 targetLaborerCount = -1;
	int32 targetBuilderCount = -1;
	int32 targetRoadMakerCount = -1;

	virtual NetworkCommandEnum commandType() override { return NetworkCommandEnum::SetTownPriority; }

	virtual int32 GetTickHash() override {
		return FNetworkCommand::GetTickHash() + laborerPriority + builderPriority + roadMakerPriority + 
													targetLaborerCount + targetBuilderCount + targetRoadMakerCount;
	}

	void Serialize(PunSerializedData& blob) override
	{
		FNetworkCommand::Serialize(blob);
		
		blob << laborerPriority;
		blob << builderPriority;
		blob << roadMakerPriority;

		blob << targetLaborerCount;
		blob << targetBuilderCount;
		blob << targetRoadMakerCount;
	}
};

class FChangeName final : public FNetworkCommand
{
public:
	virtual ~FChangeName() {}

	FString name;
	int32 objectId = -1;

	NetworkCommandEnum commandType() final { return NetworkCommandEnum::ChangeName; }

	void Serialize(PunSerializedData& blob) final {
		FNetworkCommand::Serialize(blob);
		blob << name;
		blob << objectId;
	}
};

class FSendChat final : public FNetworkCommand
{
public:
	virtual ~FSendChat() {}

	uint8 isSystemMessage = false;
	FString message;

	NetworkCommandEnum commandType() final { return NetworkCommandEnum::SendChat; }

	void Serialize(PunSerializedData& blob) final {
		FNetworkCommand::Serialize(blob);
		blob << isSystemMessage;
		blob << message;
	}

	static std::shared_ptr<FSendChat> SystemMessage(FString message) {
		auto chat = std::make_shared<FSendChat>();
		chat->isSystemMessage = true;
		chat->message = message;
		return chat;
	}
};

class FTradeResource final : public FNetworkCommand
{
public:
	virtual ~FTradeResource() {}

	TArray<uint8> buyEnums;
	TArray<int32> buyAmounts;
	int32 totalGain = -1;
	int32 objectId = -1;
	uint8 isIntercityTrade = 0;

	NetworkCommandEnum commandType() final { return NetworkCommandEnum::TradeResource; }

	void Serialize(PunSerializedData& blob) override {
		FNetworkCommand::Serialize(blob);
		blob << buyEnums;
		blob << buyAmounts;
		blob << totalGain;
		blob << objectId;
		blob << isIntercityTrade;
	}

	void operator>>(FArchive& Ar)
	{
		Ar << buyEnums;
		Ar << buyAmounts;
		Ar << totalGain;
		Ar << objectId;
		Ar << isIntercityTrade;
	}

	virtual FString ToCompactString() override {
		return FNetworkCommand::ToCompactString() + " buyEnums:" + FString::FromInt(buyEnums.Num()) + " totalGain:" + FString::FromInt(totalGain) + " objId:" + FString::FromInt(objectId) + " intercity:" + FString::FromInt(isIntercityTrade);
	}
};


class FSetIntercityTrade final : public FNetworkCommand
{
public:
	virtual ~FSetIntercityTrade() {}

	int32 buildingIdToEstablishTradeRoute = -1;
	uint8 isCancelingTradeRoute = 0;
	TArray<uint8> resourceEnums;
	TArray<uint8> intercityTradeOfferEnum;
	TArray<int32> targetInventories;

	NetworkCommandEnum commandType() final { return NetworkCommandEnum::SetIntercityTrade; }

	void Serialize(PunSerializedData& blob) final
	{
		FNetworkCommand::Serialize(blob);
		blob << buildingIdToEstablishTradeRoute;
		blob << isCancelingTradeRoute;
		blob << resourceEnums;
		blob << intercityTradeOfferEnum;
		blob << targetInventories;
	}

	/*void SerializeAndAppendToBlob(TArray<int32>& blob) final
	{
		FNetworkCommand::SerializeAndAppendToBlob(blob);
		blob.Add(buildingIdToEstablishTradeRoute);
		blob.Add(isCancelingTradeRoute);
		SerializeArray(blob, resourceEnums);
		SerializeArray(blob, intercityTradeOfferEnum);
		SerializeArray(blob, targetInventories);
	}

	void DeserializeFromBlob(const TArray<int32>& blob, int32& index) final
	{
		FNetworkCommand::DeserializeFromBlob(blob, index);
		buildingIdToEstablishTradeRoute = blob[index++];
		isCancelingTradeRoute = blob[index++];
		DeserializeArray(blob, index, resourceEnums);
		DeserializeArray(blob, index, intercityTradeOfferEnum);
		targetInventories = DeserializeArray(blob, index);
	}*/
};

class FUpgradeBuilding : public FNetworkCommand
{
public:
	virtual ~FUpgradeBuilding() {}

#if TRAILER_MODE
	int32 tileId = -1;
#endif
	
	int32 buildingId = -1;
	int32 upgradeLevel = -1; // use -1 if not needed
	int32 upgradeType = -1;
	int32 isShiftDown = false;

	NetworkCommandEnum commandType() override { return NetworkCommandEnum::UpgradeBuilding; }

	void Serialize(PunSerializedData& blob) override {
		FNetworkCommand::Serialize(blob);

#if TRAILER_MODE
		blob << tileId;
#endif
		blob << buildingId;
		blob << upgradeLevel;
		blob << upgradeType;
		blob << isShiftDown;
	}

	virtual int32 GetTickHash() override {
		return FNetworkCommand::GetTickHash() + buildingId + upgradeLevel + upgradeType + isShiftDown;
	}
};

// TODO: Generalize this for changing product type for any...
class FChangeWorkMode : public FBuildingCommand
{
public:
	virtual ~FChangeWorkMode() {}

	int32 enumInt = -1;

	int32 intVar1 = -1;
	int32 intVar2 = -1;
	int32 intVar3 = -1;

	NetworkCommandEnum commandType() override { return NetworkCommandEnum::ChangeWorkMode; }

	void Serialize(PunSerializedData& blob) override
	{
		FBuildingCommand::Serialize(blob);
		blob << enumInt;

		blob << intVar1;
		blob << intVar2;
		blob << intVar3;
	}

	virtual int32 GetTickHash() override {
		return FBuildingCommand::GetTickHash() + enumInt + intVar1 + intVar2 + intVar3;
	}

	virtual FString ToCompactString() override {
		return FNetworkCommand::ToCompactString() + " enumInt:" + FString::FromInt(enumInt) + " int1:" + FString::FromInt(intVar1) + " int2:" + FString::FromInt(intVar2) + " int3:" + FString::FromInt(intVar3);
	}
};

class FPopupDecision final : public FNetworkCommand
{
public:
	virtual ~FPopupDecision() {}
	NetworkCommandEnum commandType() final { return NetworkCommandEnum::PopupDecision; }

	int32 replyReceiverIndex = -1;
	int8 choiceIndex = -1;
	int32 replyVar1 = -1;
	int32 replyVar2 = -1;
	int32 replyVar3 = -1;

	void Serialize(PunSerializedData& blob) final
	{
		FNetworkCommand::Serialize(blob);
		blob << replyReceiverIndex;
		blob << choiceIndex;
		
		blob << replyVar1;
		blob << replyVar2;
		blob << replyVar3;
	}
};

class FRerollCards final : public FNetworkCommand
{
public:
	virtual ~FRerollCards() {}
	NetworkCommandEnum commandType() final { return NetworkCommandEnum::RerollCards; }

};

class FSelectRareCard final : public FNetworkCommand
{
public:
	virtual ~FSelectRareCard() {}
	NetworkCommandEnum commandType() override { return NetworkCommandEnum::SelectRareCard; }

	CardEnum cardEnum = CardEnum::None;
	int32 objectId = -1;

	void Serialize(PunSerializedData& blob) override {
		FNetworkCommand::Serialize(blob);
		blob << cardEnum;
		blob << objectId;
	}
};

class FBuyCard final : public FNetworkCommand
{
public:
	virtual ~FBuyCard() {}
	NetworkCommandEnum commandType() override { return NetworkCommandEnum::BuyCard; }

	TArray<int32> cardHandBuyIndices;

	void Serialize(PunSerializedData& blob) override {
		FNetworkCommand::Serialize(blob);
		blob << cardHandBuyIndices;
	}
};

class FSellCards final : public FNetworkCommand
{
public:
	virtual ~FSellCards() {}
	NetworkCommandEnum commandType() final { return NetworkCommandEnum::SellCards; }

	CardStatus cardStatus;
	int32 isShiftDown = false;

	void Serialize(PunSerializedData& blob) final {
		FNetworkCommand::Serialize(blob);
		cardStatus.Serialize(blob);
		blob << isShiftDown;
	}
};

class FUseCard final : public FNetworkCommand
{
public:
	virtual ~FUseCard() {}
	NetworkCommandEnum commandType() final { return NetworkCommandEnum::UseCard; }

	CardStatus cardStatus;
	int32 variable1 = -1;
	int32 variable2 = -1;

	CallbackEnum callbackEnum = CallbackEnum::None;

	int32 positionX100 = -1;
	int32 positionY100 = -1;

	//int32 animationStartTime100 = -1;

	void SetPosition(FVector2D position) {
		positionX100 = static_cast<int32>(position.X * 100);
		positionY100 = static_cast<int32>(position.Y * 100);
	}

	void Serialize(PunSerializedData& blob) final {
		FNetworkCommand::Serialize(blob);

		cardStatus.Serialize(blob);
		
		blob << variable1;
		blob << variable2;

		blob << callbackEnum;

		blob << positionX100;
		blob << positionY100;

		//blob.Add(animationStartTime100);
	}

	bool operator==(const FUseCard& a)
	{
		return cardStatus == a.cardStatus ||
			variable1 == a.variable1 ||
			variable2 == a.variable2 ||
			callbackEnum == a.callbackEnum ||
			positionX100 == a.positionX100 ||
			positionY100 == a.positionY100;
	}
	
	CardStatus GetCardStatus_AnimationOnly(int32 animationStartTime100)
	{
		CardStatus cardStatusOut = cardStatus;
		cardStatusOut.lastPositionX100 = positionX100;
		cardStatusOut.lastPositionY100 = positionY100;
		cardStatusOut.animationStartTime100 = animationStartTime100;
		
		return cardStatusOut;
	}
};

class FChooseResearch final : public FNetworkCommand
{
public:
	virtual ~FChooseResearch() {}
	NetworkCommandEnum commandType() final { return NetworkCommandEnum::ChooseResearch; }

	TechEnum techEnum = TechEnum::None;

	void Serialize(PunSerializedData& blob) final {
		FNetworkCommand::Serialize(blob);
		blob << techEnum;
	}

	virtual FString ToCompactString() override {
		return FNetworkCommand::ToCompactString() + " techEnum:" + FString::FromInt(static_cast<int32>(techEnum));
	}
};

class FClaimLand final : public FNetworkCommand
{
public:
	virtual ~FClaimLand() {}
	NetworkCommandEnum commandType() final { return NetworkCommandEnum::ClaimLand; }

	int32 provinceId = -1;
	CallbackEnum claimEnum = CallbackEnum::ClaimLandMoney;
	TArray<int32> cardEnums;
	TArray<int32> cardCount;

	void Serialize(PunSerializedData& blob) final {
		FNetworkCommand::Serialize(blob);
		blob << provinceId;
		blob << claimEnum;

		blob << cardEnums;
		blob << cardCount;
	}
};

class FUnslotCard final : public FNetworkCommand
{
public:
	virtual ~FUnslotCard() {}
	NetworkCommandEnum commandType() final { return NetworkCommandEnum::UnslotCard; }

	int32 buildingId = -1;
	int32 unslotIndex = -1;

	void Serialize(PunSerializedData& blob) final {
		FNetworkCommand::Serialize(blob);
		blob << buildingId;
		blob << unslotIndex;
	}
};

class FAttack final : public FNetworkCommand
{
public:
	virtual ~FAttack() {}
	NetworkCommandEnum commandType() final { return NetworkCommandEnum::Attack; }

	int32 originNodeId = -1;
	int32 targetNodeId = -1;
	int32 helpPlayerId = -1;
	TArray<int32> armyCounts;
	CallbackEnum armyOrderEnum = CallbackEnum::None;

	//void SerializeAndAppendToBlob(TArray<int32>& blob) final {
	//	FNetworkCommand::SerializeAndAppendToBlob(blob);
	//	blob.Add(originNodeId);
	//	blob.Add(targetNodeId);
	//	blob.Add(helpPlayerId);
	//	SerializeArray(blob, armyCounts);
	//	blob.Add(static_cast<int32>(armyOrderEnum));
	//}

	//void DeserializeFromBlob(const TArray<int32>& blob, int32& index) final {
	//	FNetworkCommand::DeserializeFromBlob(blob, index);
	//	originNodeId = blob[index++];
	//	targetNodeId = blob[index++];
	//	helpPlayerId = blob[index++];
	//	armyCounts = DeserializeArray(blob, index);
	//	armyOrderEnum = static_cast<CallbackEnum>(blob[index++]);
	//}
};

class FChooseLocation final : public FNetworkCommand
{
public:
	virtual ~FChooseLocation() {}
	NetworkCommandEnum commandType() override { return NetworkCommandEnum::ChooseLocation; }

	int32 provinceId = -1;
	int8 isChoosingOrReserving = -1;

	void Serialize(PunSerializedData& blob) override {
		FNetworkCommand::Serialize(blob);
		
		blob << provinceId;
		blob << isChoosingOrReserving;
	}
};

class FChooseInitialResources final : public FNetworkCommand
{
public:
	virtual ~FChooseInitialResources() {}
	NetworkCommandEnum commandType() override { return NetworkCommandEnum::ChooseInitialResources; }

	int32 foodAmount = -1;
	int32 woodAmount = -1;
	int32 medicineAmount = -1;
	int32 toolsAmount = -1;
	int32 stoneAmount = -1;
	int32 clayAmount = -1;

	bool isValid() { return foodAmount >= 0; }

	int32 totalCost()
	{
		return foodAmount * FoodCost +
			woodAmount * GetResourceInfo(ResourceEnum::Wood).basePrice +
			medicineAmount * GetResourceInfo(ResourceEnum::Medicine).basePrice +
			toolsAmount * GetResourceInfo(ResourceEnum::IronTools).basePrice +
			stoneAmount * GetResourceInfo(ResourceEnum::Stone).basePrice +
			clayAmount * GetResourceInfo(ResourceEnum::Clay).basePrice;
	}
	std::unordered_map<ResourceEnum, int32> resourceMap()
	{
		std::unordered_map<ResourceEnum, int32> resourceMap;
		resourceMap[ResourceEnum::Orange] = foodAmount;
		resourceMap[ResourceEnum::Wood] = woodAmount;
		resourceMap[ResourceEnum::Medicine] = medicineAmount;
		resourceMap[ResourceEnum::IronTools] = toolsAmount;
		resourceMap[ResourceEnum::Stone] = stoneAmount;
		resourceMap[ResourceEnum::Clay] = clayAmount;
		return resourceMap;
	}

	static FChooseInitialResources GetDefault(FactionEnum factionEnum)
	{
		FChooseInitialResources command;
		command.foodAmount = 300;
		command.woodAmount = 120;
		command.stoneAmount = 120;
		command.clayAmount = factionEnum == FactionEnum::Arab ? 240 : 0;
		
		command.medicineAmount = 240;
		command.toolsAmount = 180;
		return command;
	}

	void Serialize(PunSerializedData& blob) override {
		FNetworkCommand::Serialize(blob);

		blob << foodAmount;
		blob << woodAmount;
		blob << medicineAmount;
		blob << toolsAmount;
		blob << stoneAmount;
		blob << clayAmount;
	}

	void Serialize(FArchive &Ar)
	{
		Ar << foodAmount;
		Ar << woodAmount;
		Ar << medicineAmount;
		Ar << toolsAmount;
		Ar << stoneAmount;
		Ar << clayAmount;
	}
};

class FCheat : public FNetworkCommand
{
public:
	virtual ~FCheat() {}
	CheatEnum cheatEnum = CheatEnum::Money;
	int32 var1 = -1;
	int32 var2 = -1;
	int32 var3 = -1;
	int32 var4 = -1;
	int32 var5 = -1;
	FString stringVar1;

	NetworkCommandEnum commandType() override { return NetworkCommandEnum::Cheat; }

	void Serialize(PunSerializedData& blob) override {
		FNetworkCommand::Serialize(blob);
		
		blob << cheatEnum;
		blob << var1;
		blob << var2;
		blob << var3;
		blob << var4;
		blob << var5;
		
		blob << stringVar1;
	}
};



//! Abstract away the conversion to network blob
class NetworkHelper
{
public:
	static void SerializeAndAppendToBlob(std::shared_ptr<FNetworkCommand> command, PunSerializedData& blob)
	{
		command->Serialize(blob);
	}

	//! This is needed since the class of command is not determined in advance.
#define CASE_COMMAND(commandEnum, classType) case commandEnum: command = std::make_shared<classType>(); break;
	static std::shared_ptr<FNetworkCommand> DeserializeFromBlob(PunSerializedData& blob)
	{
		PUN_CHECK(!blob.isSaving());
		check(blob.readIndex < blob.Num());

		NetworkCommandEnum commandEnum = FNetworkCommand::GetCommandTypeFromBlob(blob);
		std::shared_ptr<FNetworkCommand> command;
		switch (commandEnum)
		{
			//CASE_COMMAND(NetworkCommandEnum::AddPlayer, FAddPlayer);

			CASE_COMMAND(NetworkCommandEnum::PlaceBuilding, FPlaceBuilding);
			CASE_COMMAND(NetworkCommandEnum::PlaceDrag, FPlaceDrag);
			CASE_COMMAND(NetworkCommandEnum::JobSlotChange, FJobSlotChange);
			CASE_COMMAND(NetworkCommandEnum::SetAllowResource, FSetAllowResource);
			CASE_COMMAND(NetworkCommandEnum::SetPriority, FSetPriority);
			CASE_COMMAND(NetworkCommandEnum::SetTownPriority, FSetTownPriority);

			CASE_COMMAND(NetworkCommandEnum::TradeResource, FTradeResource);
			CASE_COMMAND(NetworkCommandEnum::SetIntercityTrade, FSetIntercityTrade);
			
			CASE_COMMAND(NetworkCommandEnum::UpgradeBuilding, FUpgradeBuilding);
			CASE_COMMAND(NetworkCommandEnum::ChangeWorkMode, FChangeWorkMode);
			CASE_COMMAND(NetworkCommandEnum::ChooseLocation, FChooseLocation);
			CASE_COMMAND(NetworkCommandEnum::ChooseInitialResources, FChooseInitialResources);
			
			CASE_COMMAND(NetworkCommandEnum::Cheat, FCheat);
			CASE_COMMAND(NetworkCommandEnum::PopupDecision, FPopupDecision);
			CASE_COMMAND(NetworkCommandEnum::RerollCards, FRerollCards);
			CASE_COMMAND(NetworkCommandEnum::SelectRareCard, FSelectRareCard);
			CASE_COMMAND(NetworkCommandEnum::BuyCard, FBuyCard);
			CASE_COMMAND(NetworkCommandEnum::SellCards, FSellCards);
			CASE_COMMAND(NetworkCommandEnum::UseCard, FUseCard);
			CASE_COMMAND(NetworkCommandEnum::UnslotCard, FUnslotCard);

			CASE_COMMAND(NetworkCommandEnum::Attack,	FAttack);

			CASE_COMMAND(NetworkCommandEnum::ClaimLand, FClaimLand);
			CASE_COMMAND(NetworkCommandEnum::ChooseResearch, FChooseResearch);

			CASE_COMMAND(NetworkCommandEnum::ChangeName, FChangeName);
			CASE_COMMAND(NetworkCommandEnum::SendChat, FSendChat);

			CASE_COMMAND(NetworkCommandEnum::SetGlobalJobPriority, FSetGlobalJobPriority);
			CASE_COMMAND(NetworkCommandEnum::GenericCommand, FGenericCommand);

			default: UE_DEBUG_BREAK();
		}
		command->Serialize(blob);

		return command;
	}
#undef CASE_COMMAND

};

struct NetworkTickInfo
{
	int32 proxyControllerTick = 0; // This is server tick... Not simulation tick. These may be different depending on gameSpeed.
	//int32 playerCount = 0;
	int32 gameSpeed = 1;
	int32 tickCountSim = 0;

	std::vector<std::shared_ptr<FNetworkCommand>> commands;

	bool hasCommand() { return commands.size() > 0; }

	void SerializeToBlob(TArray<int32>& blob)
	{
		blob.Add(proxyControllerTick);
		blob.Add(gameSpeed);
		blob.Add(tickCountSim);

		PunSerializedData punBlob(true);
		for (size_t i = 0; i < commands.size(); i++) {
			commands[i]->Serialize(punBlob);
		}
		blob.Append(punBlob);

		uint32 checksum = BSDChecksum(0, blob);
		int32* checksumInt = reinterpret_cast<int32*>(&checksum);
		blob.Insert(*checksumInt, 0);

		//_LOG(PunTickHash, "<<< NetworkTickInfo tick:%d gameSpeed:%d tickSim:%d commands:%d", proxyControllerTick, gameSpeed, tickCountSim, commands.size());
	}

	uint32 BSDChecksum(int32 shift, const TArray<int32>& blob)
	{
		uint32 checksum = 0;
		for (int32 i = shift; i < blob.Num(); i++) {
			checksum = (checksum >> 1) + ((checksum & 1) << 15);
			checksum += static_cast<uint32>(abs(blob[i]));
			checksum &= 0xffff;       /* Keep it within bounds. */
		}
		return checksum;
	}

	void DeserializeFromBlob(const TArray<int32>& blob, int index = 0)
	{
		int32 sourceChecksumInt = blob[index++];
		uint32 sourceChecksum = *reinterpret_cast<uint32*>(&sourceChecksumInt);
		uint32 checksum = BSDChecksum(1, blob);
		check(sourceChecksum == checksum);
		
		proxyControllerTick = blob[index++];
		gameSpeed = blob[index++];
		tickCountSim = blob[index++];

		LOOP_CHECK_START();

		PunSerializedData punBlob(false, blob, index);
		
		while (punBlob.readIndex < punBlob.Num()) {
			commands.push_back(NetworkHelper::DeserializeFromBlob(punBlob));
			LOOP_CHECK_END();
		}

		check(punBlob.readIndex == punBlob.Num()); // Ensure there is no leftover data

		_LOG(PunTickHash, ">>> NetworkTickInfo tick:%d gameSpeed:%d tickSim:%d commands:%d", proxyControllerTick, gameSpeed, tickCountSim, commands.size());
	}

	FString ToString() {
		FString string = FString::Printf(TEXT("TickInfo serverTick:%d simTick:%d commands:%d, "), proxyControllerTick, tickCountSim, commands.size());
		for (size_t i = 0; i < commands.size(); i++) {
			string.Append(FString("|") + ToFString(NetworkCommandNames[static_cast<int>(commands[i]->commandType())]));
		}
		return string;
	}

};