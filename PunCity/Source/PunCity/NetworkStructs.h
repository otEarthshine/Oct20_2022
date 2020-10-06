#pragma once

#include "CoreMinimal.h"
#include "PunCity/Simulation/GameSimulationInfo.h"
#include "PunUnrealUtils.h"

/*
 * Light weight alternative to UE4's FArchive FMemoryReader etc.
 */
class PunSerializedData : public TArray<int32>
{
public:
	PunSerializedData(bool isSaving) {
		readIndex = isSaving ? -1 : 0;
	}

	PunSerializedData(bool isSaving, const TArray<int32>& blob, int32 index = 0)
	{
		readIndex = isSaving ? -1 : index;
		Append(blob);
	}

	int32 readIndex = -1;
	
	bool isSaving() { return readIndex == -1; }

	
	void operator<<(int32& value) {
		if (isSaving()) {
			Add(value);
		} else {
			value = (*this)[readIndex++];
		}
	}
	void operator<<(int16& value) {
		if (isSaving()) {
			Add(value);
		}
		else {
			value = (*this)[readIndex++];
		}
	}
	void operator<<(int8& value) {
		if (isSaving()) {
			Add(value);
		} else {
			value = (*this)[readIndex++];
		}
	}
	void operator<<(uint16& value) {
		if (isSaving()) {
			Add(value);
		}
		else {
			value = (*this)[readIndex++];
		}
	}
	void operator<<(uint8& value) {
		if (isSaving()) {
			Add(value);
		} else {
			value = (*this)[readIndex++];
		}
	}
	void operator<<(bool& value) {
		if (isSaving()) {
			Add(value);
		} else {
			value = (*this)[readIndex++];
		}
	}

	// Enum
	template <
		typename EnumType,
		typename = typename TEnableIf<TIsEnumClass<EnumType>::Value>::Type
	>
	void operator<<(EnumType& Value) {
		return (*this) << (__underlying_type(EnumType)&)Value;
	}

	void operator<<(FString& value) {
		if (isSaving()) {
			FString_SerializeAndAppendToBlob(value, *this);
		} else {
			value = FString_DeserializeFromBlob(*this, readIndex);
		}
	}

	void operator<<(TArray<int32>& inArray) {
		if (isSaving()) {
			Add(inArray.Num());
			Append(inArray);
		} else {
			int32 count = (*this)[readIndex++];
			for (int i = 0; i < count; i++) {
				inArray.Add((*this)[readIndex++]);
			}
		}
	}
	void operator<<(TArray<uint8>& inArray) {
		if (isSaving()) {
			Add(inArray.Num());
			Append(inArray);
		}
		else {
			int32 count = (*this)[readIndex++];
			for (int i = 0; i < count; i++) {
				inArray.Add((*this)[readIndex++]);
			}
		}
	}

	void operator<<(WorldTile2& value) {
		(*this) << value.x;
		(*this) << value.y;
	}
	void operator<<(TileArea& value) {
		(*this) << value.minX;
		(*this) << value.minY;
		(*this) << value.maxX;
		(*this) << value.maxY;
	}
};

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
		ss << ", size:" << ToStdString(MapSizeNames[mapSizeEnumInt]);
		ss << ", seaLevel:" << ToStdString(MapSettingsLevelNames[static_cast<int>(mapSeaLevel)]);
		ss << ", moisture:" << ToStdString(MapMoistureNames[static_cast<int>(mapMoisture)]);
		ss << ", temperature:" << ToStdString(MapSettingsLevelNames[static_cast<int>(mapTemperature)]);
		ss << ", mountain:" << ToStdString(MapSettingsLevelNames[static_cast<int>(mapMountainDensity)]);
		
		ss << ", playerCount:" << playerCount;
		ss << ", ai:" << aiCount;
		ss << ", mode:" << ToStdString(DifficultyLevelNames[static_cast<int>(difficultyLevel)]);
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
};

static const std::string NetworkCommandNames[] =
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
	return NetworkCommandNames[static_cast<int>(commandEnum)];
}


class FNetworkCommand
{
public:
	virtual ~FNetworkCommand() {}

	int32 playerId = -1; // Note!!! This is filled after being sent on network

	virtual NetworkCommandEnum commandType() { return NetworkCommandEnum::None; }

	virtual void Serialize(PunSerializedData& Ar) {
		int32 commandInt = 0;
		if (Ar.isSaving()) {
			commandInt = static_cast<int32>(commandType());
		}
		
		Ar << commandInt;
		Ar << playerId;
	}

	static NetworkCommandEnum GetCommandTypeFromBlob(PunSerializedData& blob) {
		return static_cast<NetworkCommandEnum>(blob[blob.readIndex]);
	}

	virtual FString ToCompactString() {
		return ToFString(GetNetworkCommandName(commandType()));
	}
};

// 
class FBuildingCommand : public FNetworkCommand
{
public:
	virtual ~FBuildingCommand() {}

	int32 buildingId;
	int32 buildingTileId = -1; // Replay Only
	CardEnum buildingEnum = CardEnum::None; // Replay Only

	void Serialize(PunSerializedData& blob) override {
		FNetworkCommand::Serialize(blob);
		
		blob << buildingId;
		blob << buildingTileId;
		blob << buildingEnum;
	}
};


class FPlaceBuilding final : public FNetworkCommand
{
public:
	virtual ~FPlaceBuilding() {}

	uint8 buildingEnum;
	int32 buildingLevel = 0; // Note TrailerMode - Farm: use buildingLevel to specify plant type (need after-edit..)

	TileArea area; // Needed in the case for manipulable area
	TileArea area2;
	WorldTile2 center;
	uint8 faceDirection;
	bool useBoughtCard = false; //  Note TrailerMode - use useBoughtCard to specify if it is prebuilt
	CardEnum useWildCard = CardEnum::None;

	bool isTrailerPreBuilt() { return area2.minX; }

	NetworkCommandEnum commandType() override { return NetworkCommandEnum::PlaceBuilding; }

	void Serialize(PunSerializedData& blob) override {
		FNetworkCommand::Serialize(blob);

		blob << buildingEnum;
		blob << buildingLevel;
		
		blob << area;
		blob << area2;
		blob << center;
		blob << faceDirection;
		blob << useBoughtCard;
		blob << useWildCard;
	}

	FString ToCompactString() override {
		return ToFString(GetNetworkCommandName(commandType()) + "-" + GetBuildingInfoInt(buildingEnum).name);
	}
};


class FPlaceDrag final : public FNetworkCommand
{
public:
	virtual ~FPlaceDrag() {}
	TileArea area = TileArea::Invalid; // Needed in the case for manipulable area
	TileArea area2 = TileArea::Invalid;
	TArray<int32> path;
	int32 placementType;
	ResourceEnum harvestResourceEnum;

	bool isTrailerPreBuilt() { return area2.minX; }

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
};

class FJobSlotChange final : public FBuildingCommand
{
public:
	virtual ~FJobSlotChange() {}

	int32 allowedOccupants;

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

	enum class Type : uint8 {
		SendGift,
	} genericCommandType;

	int32 intVar1 = -1;
	int32 intVar2 = -1;
	int32 intVar3 = -1;

	void Serialize(PunSerializedData& blob) override
	{
		FNetworkCommand::Serialize(blob);
		blob << genericCommandType;
		blob << intVar1;
		blob << intVar2;
		blob << intVar3;
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

	NetworkCommandEnum commandType() override { return NetworkCommandEnum::SetTownPriority; }

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
	int32 objectId;

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
};

class FTradeResource final : public FNetworkCommand
{
public:
	virtual ~FTradeResource() {}

	TArray<uint8> buyEnums;
	TArray<int32> buyAmounts;
	int32 totalGain;
	int32 objectId;
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
	
	int32 buildingId;
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

};

// TODO: Generalize this for changing product type for any...
class FChangeWorkMode : public FBuildingCommand
{
public:
	virtual ~FChangeWorkMode() {}

	int32 enumInt;

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
	
	//void SerializeAndAppendToBlob(TArray<int32>& blob) override
	//{
	//	FBuildingCommand::SerializeAndAppendToBlob(blob);
	//	blob.Add(enumInt);
	//	
	//	blob.Add(intVar1);
	//	blob.Add(intVar2);
	//	blob.Add(intVar3);
	//}

	//void DeserializeFromBlob(const TArray<int32>& blob, int32& index) override
	//{
	//	FBuildingCommand::DeserializeFromBlob(blob, index);
	//	enumInt = blob[index++];
	//	
	//	intVar1 = blob[index++];
	//	intVar2 = blob[index++];
	//	intVar3 = blob[index++];
	//}
};

class FPopupDecision final : public FNetworkCommand
{
public:
	virtual ~FPopupDecision() {}
	NetworkCommandEnum commandType() final { return NetworkCommandEnum::PopupDecision; }

	int32 replyReceiverIndex;
	int8 choiceIndex;
	int32 replyVar1;

	void Serialize(PunSerializedData& blob) final
	{
		FNetworkCommand::Serialize(blob);
		blob << replyReceiverIndex;
		blob << choiceIndex;
		blob << replyVar1;
	}
	
	//void SerializeAndAppendToBlob(TArray<int32>& blob) final
	//{
	//	FNetworkCommand::SerializeAndAppendToBlob(blob);
	//	blob.Add(replyReceiverIndex);
	//	blob.Add(choiceIndex);
	//	blob.Add(replyVar1);
	//}

	//void DeserializeFromBlob(const TArray<int32>& blob, int32& index) final
	//{
	//	FNetworkCommand::DeserializeFromBlob(blob, index);
	//	replyReceiverIndex = blob[index++];
	//	choiceIndex = blob[index++];
	//	replyVar1 = blob[index++];
	//}
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

	void Serialize(PunSerializedData& blob) override {
		FNetworkCommand::Serialize(blob);
		blob << cardEnum;
	}
	
	//void SerializeAndAppendToBlob(TArray<int32>& blob) override {
	//	FNetworkCommand::SerializeAndAppendToBlob(blob);
	//	blob.Add(static_cast<int32>(cardEnum));
	//}

	//void DeserializeFromBlob(const TArray<int32>& blob, int32& index) override {
	//	FNetworkCommand::DeserializeFromBlob(blob, index);
	//	cardEnum = static_cast<CardEnum>(blob[index++]);
	//}
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
	//void SerializeAndAppendToBlob(TArray<int32>& blob) override {
	//	FNetworkCommand::SerializeAndAppendToBlob(blob);
	//	SerializeArray(blob, cardHandBuyIndices);
	//}

	//void DeserializeFromBlob(const TArray<int32>& blob, int32& index) override {
	//	FNetworkCommand::DeserializeFromBlob(blob, index);
	//	cardHandBuyIndices = DeserializeArray(blob, index);
	//}
};

class FSellCards final : public FNetworkCommand
{
public:
	virtual ~FSellCards() {}
	NetworkCommandEnum commandType() final { return NetworkCommandEnum::SellCards; }

	CardEnum buildingEnum = CardEnum::None;
	int32 cardCount = 0;

	void Serialize(PunSerializedData& blob) final {
		FNetworkCommand::Serialize(blob);
		blob << buildingEnum;
		blob << cardCount;
	}

	//void SerializeAndAppendToBlob(TArray<int32>& blob) final {
	//	FNetworkCommand::SerializeAndAppendToBlob(blob);
	//	blob.Add(static_cast<int32>(buildingEnum));
	//	blob.Add(cardCount);
	//}

	//void DeserializeFromBlob(const TArray<int32>& blob, int32& index) final {
	//	FNetworkCommand::DeserializeFromBlob(blob, index);
	//	buildingEnum = static_cast<CardEnum>(blob[index++]);
	//	cardCount = blob[index++];
	//}
};

class FUseCard final : public FNetworkCommand
{
public:
	virtual ~FUseCard() {}
	NetworkCommandEnum commandType() final { return NetworkCommandEnum::UseCard; }

	CardEnum cardEnum = CardEnum::None;
	int32 variable1 = -1;
	int32 variable2 = -1;

	int32 positionX100 = -1;
	int32 positionY100 = -1;

	//int32 animationStartTime100 = -1;

	void SetPosition(FVector2D position) {
		positionX100 = static_cast<int32>(position.X * 100);
		positionY100 = static_cast<int32>(position.Y * 100);
	}

	void Serialize(PunSerializedData& blob) final {
		FNetworkCommand::Serialize(blob);
		blob << cardEnum;
		blob << variable1;
		blob << variable2;

		blob << positionX100;
		blob << positionY100;

		//blob.Add(animationStartTime100);
	}
	
	//void SerializeAndAppendToBlob(TArray<int32>& blob) final {
	//	FNetworkCommand::SerializeAndAppendToBlob(blob);
	//	blob.Add(static_cast<int32>(cardEnum));
	//	blob.Add(variable1);
	//	blob.Add(variable2);

	//	blob.Add(positionX100);
	//	blob.Add(positionY100);

	//	//blob.Add(animationStartTime100);
	//}

	//void DeserializeFromBlob(const TArray<int32>& blob, int32& index) final {
	//	FNetworkCommand::DeserializeFromBlob(blob, index);
	//	cardEnum = static_cast<CardEnum>(blob[index++]);
	//	variable1 = blob[index++];
	//	variable2 = blob[index++];

	//	positionX100 = blob[index++];
	//	positionY100 = blob[index++];

	//	//animationStartTime100 = blob[index++];
	//}

	CardStatus GetCardStatus(int32 animationStartTime100) {
		return { cardEnum, positionX100, positionY100, animationStartTime100 };
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
};

class FClaimLand final : public FNetworkCommand
{
public:
	virtual ~FClaimLand() {}
	NetworkCommandEnum commandType() final { return NetworkCommandEnum::ClaimLand; }

	int32 provinceId = -1;
	CallbackEnum claimEnum = CallbackEnum::ClaimLandMoney;

	void Serialize(PunSerializedData& blob) final {
		FNetworkCommand::Serialize(blob);
		blob << provinceId;
		blob << claimEnum;
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

	int32 provinceId;
	int8 isChoosingOrReserving;

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

	bool isValid() { return foodAmount >= 0; }

	int32 totalCost()
	{
		return foodAmount * FoodCost +
			woodAmount * GetResourceInfo(ResourceEnum::Wood).basePrice +
			medicineAmount * GetResourceInfo(ResourceEnum::Medicine).basePrice +
			toolsAmount * GetResourceInfo(ResourceEnum::SteelTools).basePrice +
			stoneAmount * GetResourceInfo(ResourceEnum::Stone).basePrice;
	}
	std::unordered_map<ResourceEnum, int32> resourceMap()
	{
		std::unordered_map<ResourceEnum, int32> resourceMap;
		resourceMap[ResourceEnum::Orange] = foodAmount;
		resourceMap[ResourceEnum::Wood] = woodAmount;
		resourceMap[ResourceEnum::Medicine] = medicineAmount;
		resourceMap[ResourceEnum::SteelTools] = toolsAmount;
		resourceMap[ResourceEnum::Stone] = stoneAmount;
		return resourceMap;
	}

	static FChooseInitialResources GetDefault()
	{
		FChooseInitialResources command;
		command.foodAmount = 240;
		command.woodAmount = 120;
		command.stoneAmount = 120;
		
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
	}

	void Serialize(FArchive &Ar)
	{
		Ar << foodAmount;
		Ar << woodAmount;
		Ar << medicineAmount;
		Ar << toolsAmount;
		Ar << stoneAmount;
	}
};

class FCheat : public FNetworkCommand
{
public:
	virtual ~FCheat() {}
	CheatEnum cheatEnum;
	int32 var1;
	int32 var2;

	NetworkCommandEnum commandType() override { return NetworkCommandEnum::Cheat; }

	void Serialize(PunSerializedData& blob) override {
		FNetworkCommand::Serialize(blob);
		
		blob << cheatEnum;
		blob << var1;
		blob << var2;
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
	int32 tickCount = 0; // This is server tick... Not simulation tick. These may be different depending on gameSpeed.
	//int32 playerCount = 0;
	int32 gameSpeed = 1;
	int32 tickCountSim = 0;

	std::vector<std::shared_ptr<FNetworkCommand>> commands;

	bool hasCommand() { return commands.size() > 0; }

	void SerializeToBlob(TArray<int32>& blob)
	{
		blob.Add(tickCount);
		blob.Add(gameSpeed);
		blob.Add(tickCountSim);

		PunSerializedData punBlob(true);
		for (size_t i = 0; i < commands.size(); i++) {
			commands[i]->Serialize(punBlob);
		}
		blob.Append(punBlob);
	}

	void DeserializeFromBlob(const TArray<int32>& blob, int index = 0)
	{
		tickCount = blob[index++];
		gameSpeed = blob[index++];
		tickCountSim = blob[index++];

		LOOP_CHECK_START();

		PunSerializedData punBlob(false, blob, index);
		
		while (punBlob.readIndex < punBlob.Num()) {
			commands.push_back(NetworkHelper::DeserializeFromBlob(punBlob));

			LOOP_CHECK_END();
		}

		check(punBlob.readIndex == punBlob.Num()); // Ensure there is no leftover data
	}

	FString ToString() {
		FString string = FString::Printf(TEXT("TickInfo serverTick:%d simTick:%d commands:%d, "), tickCount, tickCountSim, commands.size());
		for (size_t i = 0; i < commands.size(); i++) {
			string.Append(FString("|") + ToFString(NetworkCommandNames[static_cast<int>(commands[i]->commandType())]));
		}
		return string;
	}

};