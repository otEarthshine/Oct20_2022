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
	int32 playerCount = 0;
	int32 aiCount = 15;
	DifficultyLevel difficultyLevel = DifficultyLevel::Normal;

	bool isSinglePlayer = false;

	bool isMultiplayer() { return !isSinglePlayer; }

	bool isInitialized() { return playerCount > 0; }

	bool operator==(const FMapSettings& a)
	{
		return mapSeed == a.mapSeed &&
			mapSizeEnumInt == a.mapSizeEnumInt &&
			playerCount == a.playerCount &&
			aiCount == a.aiCount &&
			difficultyLevel == a.difficultyLevel;
	}
	bool operator!=(const FMapSettings& a)
	{
		return !(*this == a);
	}


	std::string mapSeedStd() { return ToStdString(mapSeed); }

	void SerializeAndAppendToBlob(TArray<int32>& blob)
	{
		FString_SerializeAndAppendToBlob(mapSeed, blob);
		blob.Add(mapSizeEnumInt);
		blob.Add(playerCount);
		blob.Add(aiCount);
		blob.Add(static_cast<int32>(difficultyLevel));
		blob.Add(isSinglePlayer);
	}

	void DeserializeFromBlob(const TArray<int32>& blob, int32& index)
	{
		mapSeed = FString_DeserializeFromBlob(blob, index);
		mapSizeEnumInt = blob[index++];
		playerCount = blob[index++];
		aiCount = blob[index++];
		difficultyLevel = static_cast<DifficultyLevel>(blob[index++]);
		isSinglePlayer = blob[index++];
	}

	MapSizeEnum mapSizeEnum() { return static_cast<MapSizeEnum>(mapSizeEnumInt); }

	std::string ToString()
	{
		std::stringstream ss;
		ss << "[seed:" << ToStdString(mapSeed);
		ss << ", size:" << ToStdString(MapSizeNames[mapSizeEnumInt]);
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

	void SerializeAndAppendToBlob(TArray<int32>& blob)
	{
		blob.Add(playerId);
		blob.Add(victoriousPlayerId);
		blob.Add(static_cast<int32>(gameEndEnum));
	}

	void DeserializeFromBlob(const TArray<int32>& blob, int32& index)
	{
		playerId = blob[index++];
		victoriousPlayerId = blob[index++];
		gameEndEnum = static_cast<GameEndEnum>(blob[index++]);
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

enum class NetworkCommandEnum
{
	None,
	AddPlayer,

	PlaceBuilding,
	PlaceGather,
	JobSlotChange,
	SetAllowResource,
	SetPriority,
	SetTownPriority,

	TradeResource,
	SetIntercityTrade,
	UpgradeBuilding,
	ChangeWorkMode,
	PopupDecision,
	ChooseLocation,
	Cheat,
	RerollCards,
	SelectRareCard,
	BuyCard,
	SellCards,
	UseCard,
	UnslotCard,

	TrainUnit,

	ClaimLand,
	Attack,
	ChooseResearch,

	ChangeName,
	SendChat,
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

	"TradeResource",
	"SetIntercityTrade",
	"UpgradeBuilding",
	"ChangeWorkMode",
	"PopupDecision",
	"ChooseLocation",
	"Cheat",
	"RerollCards",
	"SelectRareCard",
	"BuyCards",
	"SellCards",
	"UseCard",
	"UnslotCard",

	"TrainUnit",

	"ClaimLand",
	"Attack",
	"ChooseResearch",

	"ChangeName",
	"SendChat",
};

class FNetworkCommand
{
public:
	virtual ~FNetworkCommand() {}

	int32 playerId = -1; // Note!!! This is filled after being sent on network

	virtual NetworkCommandEnum commandType() { return NetworkCommandEnum::None; }

	//! Append NetworkCommand onto the blob
	virtual void SerializeAndAppendToBlob(TArray<int32>& blob) {
		blob.Add((int32)commandType());
		blob.Add(playerId);
	}
	
	//! Read blob using index
	virtual void DeserializeFromBlob(const TArray<int32>& blob, int32& index) {
		blob[index++]; // Skip commandType
		playerId = blob[index++];
	}

	static NetworkCommandEnum GetCommandTypeFromBlob(const TArray<int32>& blob, int32& index) {
		return (NetworkCommandEnum)blob[index];
	}
};

//class FAddPlayer final : public FNetworkCommand
//{
//public:
//	virtual ~FAddPlayer() {}
//
//	int8 isAIPlayer;
//	//int32 playerId;
//	FString playerName;
//	FString replayFileName;
//
//	NetworkCommandEnum commandType() final { return NetworkCommandEnum::AddPlayer; }
//
//	void SerializeAndAppendToBlob(TArray<int32>& blob) final
//	{
//		FNetworkCommand::SerializeAndAppendToBlob(blob);
//
//		blob.Add(isAIPlayer);
//		//blob.Add(playerId);
//		FString_SerializeAndAppendToBlob(playerName, blob);
//		FString_SerializeAndAppendToBlob(replayFileName, blob);
//	}
//
//	void DeserializeFromBlob(const TArray<int32>& blob, int32& index) final
//	{
//		FNetworkCommand::DeserializeFromBlob(blob, index);
//
//		isAIPlayer = blob[index++];
//		//playerId = blob[index++];
//		playerName = FString_DeserializeFromBlob(blob, index);
//		replayFileName = FString_DeserializeFromBlob(blob, index);
//	}
//};


class FPlaceBuildingParameters final : public FNetworkCommand
{
public:
	virtual ~FPlaceBuildingParameters() {}

	uint8 buildingEnum;
	int32 buildingLevel = 0;

	TileArea area; // Needed in the case for manipulable area
	TileArea area2;
	WorldTile2 center;
	uint8 faceDirection;
	bool useBoughtCard = false;
	CardEnum useWildCard = CardEnum::None;

	NetworkCommandEnum commandType() override { return NetworkCommandEnum::PlaceBuilding; }

	void SerializeAndAppendToBlob(TArray<int32>& blob) override
	{
		FNetworkCommand::SerializeAndAppendToBlob(blob);
		blob.Add(buildingEnum);
		blob.Add(buildingLevel);
		area.SerializeAndAppendToBlob(blob);
		area2.SerializeAndAppendToBlob(blob);
		center.SerializeAndAppendToBlob(blob);
		blob.Add(faceDirection);
		blob.Add(useBoughtCard);
		blob.Add(static_cast<int32>(useWildCard));
	}

	void DeserializeFromBlob(const TArray<int32>& blob, int32& index) override 
	{
		FNetworkCommand::DeserializeFromBlob(blob, index);
		buildingEnum = blob[index++];
		buildingLevel = blob[index++];
		area.DeserializeFromBlob(blob, index);
		area2.DeserializeFromBlob(blob, index);
		center.DeserializeFromBlob(blob, index);
		faceDirection = blob[index++];
		useBoughtCard = blob[index++];
		useWildCard = static_cast<CardEnum>(blob[index++]);
	}
};


class FPlaceGatherParameters final : public FNetworkCommand
{
public:
	virtual ~FPlaceGatherParameters() {}
	TileArea area = TileArea::Invalid; // Needed in the case for manipulable area
	TileArea area2 = TileArea::Invalid;
	TArray<int32> path;
	int8 placementType;
	ResourceEnum harvestResourceEnum;

	NetworkCommandEnum commandType() override { return NetworkCommandEnum::PlaceGather; }

	void SerializeAndAppendToBlob(TArray<int32>& blob) override
	{
		FNetworkCommand::SerializeAndAppendToBlob(blob);
		area.SerializeAndAppendToBlob(blob);
		area2.SerializeAndAppendToBlob(blob);
		SerializeArray(blob, path);

		blob.Add(placementType);
		blob.Add(static_cast<int32>(harvestResourceEnum));
	}

	void DeserializeFromBlob(const TArray<int32>& blob, int32& index) override
	{
		FNetworkCommand::DeserializeFromBlob(blob, index);
		area.DeserializeFromBlob(blob, index);
		area2.DeserializeFromBlob(blob, index);
		path = DeserializeArray(blob, index);

		placementType = blob[index++];
		harvestResourceEnum = static_cast<ResourceEnum>(blob[index++]);
	}
};

class FJobSlotChange final : public FNetworkCommand
{
public:
	virtual ~FJobSlotChange() {}

	int32 allowedOccupants;
	int32 buildingId;

	NetworkCommandEnum commandType() final { return NetworkCommandEnum::JobSlotChange; }

	void SerializeAndAppendToBlob(TArray<int32>& blob) final
	{
		FNetworkCommand::SerializeAndAppendToBlob(blob);
		blob.Add(allowedOccupants);
		blob.Add(buildingId);
	}

	void DeserializeFromBlob(const TArray<int32>& blob, int32& index) final
	{
		FNetworkCommand::DeserializeFromBlob(blob, index);
		allowedOccupants = blob[index++];
		buildingId = blob[index++];
	}
};

class FSetAllowResource final : public FNetworkCommand
{
public:
	virtual ~FSetAllowResource() {}

	int32 buildingId = -1;
	ResourceEnum resourceEnum = ResourceEnum::None;
	bool allowed = false;
	int32 target = -1;

	NetworkCommandEnum commandType() final { return NetworkCommandEnum::SetAllowResource; }

	void SerializeAndAppendToBlob(TArray<int32>& blob) final
	{
		FNetworkCommand::SerializeAndAppendToBlob(blob);
		blob.Add(buildingId);
		blob.Add(static_cast<int32>(resourceEnum));
		blob.Add(allowed);
		blob.Add(target);
	}

	void DeserializeFromBlob(const TArray<int32>& blob, int32& index) final
	{
		FNetworkCommand::DeserializeFromBlob(blob, index);
		buildingId = blob[index++];
		resourceEnum = static_cast<ResourceEnum>(blob[index++]);
		allowed = blob[index++];
		target = blob[index++];
	}
};

class FSetPriority final : public FNetworkCommand
{
public:
	virtual ~FSetPriority() {}

	int32 priority;
	int32 buildingId;

	NetworkCommandEnum commandType() final { return NetworkCommandEnum::SetPriority; }

	void SerializeAndAppendToBlob(TArray<int32>& blob) final
	{
		FNetworkCommand::SerializeAndAppendToBlob(blob);
		blob.Add(priority);
		blob.Add(buildingId);
	}

	void DeserializeFromBlob(const TArray<int32>& blob, int32& index) final
	{
		FNetworkCommand::DeserializeFromBlob(blob, index);
		priority = blob[index++];
		buildingId = blob[index++];
	}
};

class FSetTownPriority final : public FNetworkCommand
{
public:
	virtual ~FSetTownPriority() {}

	bool laborerPriority;
	bool builderPriority;
	bool roadMakerPriority;
	int32 targetLaborerCount;
	int32 targetBuilderCount;
	int32 targetRoadMakerCount;

	NetworkCommandEnum commandType() override { return NetworkCommandEnum::SetTownPriority; }

	void SerializeAndAppendToBlob(TArray<int32>& blob) override
	{
		FNetworkCommand::SerializeAndAppendToBlob(blob);
		blob.Add(laborerPriority);
		blob.Add(builderPriority);
		blob.Add(roadMakerPriority);
		
		blob.Add(targetLaborerCount);
		blob.Add(targetBuilderCount);
		blob.Add(targetRoadMakerCount);
	}

	void DeserializeFromBlob(const TArray<int32>& blob, int32& index) override
	{
		FNetworkCommand::DeserializeFromBlob(blob, index);
		laborerPriority = blob[index++];
		builderPriority = blob[index++];
		roadMakerPriority = blob[index++];
		
		targetLaborerCount = blob[index++];
		targetBuilderCount = blob[index++];
		targetRoadMakerCount = blob[index++];
	}
};

class FChangeName final : public FNetworkCommand
{
public:
	virtual ~FChangeName() {}

	FString name;
	int32 objectId;

	NetworkCommandEnum commandType() final { return NetworkCommandEnum::ChangeName; }

	void SerializeAndAppendToBlob(TArray<int32>& blob) final
	{
		FNetworkCommand::SerializeAndAppendToBlob(blob);
		FString_SerializeAndAppendToBlob(name, blob);
		blob.Add(objectId);
	}

	void DeserializeFromBlob(const TArray<int32>& blob, int32& index) final
	{
		FNetworkCommand::DeserializeFromBlob(blob, index);
		name = FString_DeserializeFromBlob(blob, index);
		objectId = blob[index++];
	}
};

class FSendChat final : public FNetworkCommand
{
public:
	virtual ~FSendChat() {}

	uint8 isSystemMessage = false;
	FString message;

	NetworkCommandEnum commandType() final { return NetworkCommandEnum::SendChat; }

	void SerializeAndAppendToBlob(TArray<int32>& blob) final
	{
		FNetworkCommand::SerializeAndAppendToBlob(blob);
		blob.Add(isSystemMessage);
		FString_SerializeAndAppendToBlob(message, blob);
	}

	void DeserializeFromBlob(const TArray<int32>& blob, int32& index) final
	{
		FNetworkCommand::DeserializeFromBlob(blob, index);
		isSystemMessage = blob[index++];
		message = FString_DeserializeFromBlob(blob, index);
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

	void SerializeAndAppendToBlob(TArray<int32>& blob) final
	{
		FNetworkCommand::SerializeAndAppendToBlob(blob);
		SerializeArray(blob, buyEnums);
		SerializeArray(blob, buyAmounts);
		blob.Add(totalGain);
		blob.Add(objectId);
		blob.Add(isIntercityTrade);
	}

	void DeserializeFromBlob(const TArray<int32>& blob, int32& index) final
	{
		FNetworkCommand::DeserializeFromBlob(blob, index);
		DeserializeArray(blob, index, buyEnums);
		buyAmounts = DeserializeArray(blob, index);
		totalGain = blob[index++];
		objectId = blob[index++];
		isIntercityTrade = blob[index++];
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

	void SerializeAndAppendToBlob(TArray<int32>& blob) final
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
	}
};

class FUpgradeBuilding : public FNetworkCommand
{
public:
	virtual ~FUpgradeBuilding() {}

	int32 buildingId;
	int32 upgradeLevel = -1; // use -1 if not needed
	int32 upgradeType = -1;

	NetworkCommandEnum commandType() override { return NetworkCommandEnum::UpgradeBuilding; }

	void SerializeAndAppendToBlob(TArray<int32>& blob) override
	{
		FNetworkCommand::SerializeAndAppendToBlob(blob);
		blob.Add(buildingId);
		blob.Add(upgradeLevel);
		blob.Add(upgradeType);
	}

	void DeserializeFromBlob(const TArray<int32>& blob, int32& index) override
	{
		FNetworkCommand::DeserializeFromBlob(blob, index);
		buildingId = blob[index++];
		upgradeLevel = blob[index++];
		upgradeType = blob[index++];
	}
};

// TODO: Generalize this for changing product type for any...
class FChangeWorkMode : public FNetworkCommand
{
public:
	virtual ~FChangeWorkMode() {}

	int32 buildingId;
	int32 enumInt;

	int32 intVar1 = -1;
	int32 intVar2 = -1;
	int32 intVar3 = -1;

	NetworkCommandEnum commandType() override { return NetworkCommandEnum::ChangeWorkMode; }

	void SerializeAndAppendToBlob(TArray<int32>& blob) override
	{
		FNetworkCommand::SerializeAndAppendToBlob(blob);
		blob.Add(buildingId);
		blob.Add(enumInt);
		
		blob.Add(intVar1);
		blob.Add(intVar2);
		blob.Add(intVar3);
	}

	void DeserializeFromBlob(const TArray<int32>& blob, int32& index) override
	{
		FNetworkCommand::DeserializeFromBlob(blob, index);
		buildingId = blob[index++];
		enumInt = blob[index++];
		
		intVar1 = blob[index++];
		intVar2 = blob[index++];
		intVar3 = blob[index++];
	}
};

class FPopupDecision final : public FNetworkCommand
{
public:
	virtual ~FPopupDecision() {}
	NetworkCommandEnum commandType() final { return NetworkCommandEnum::PopupDecision; }

	int32 replyReceiverIndex;
	int8 choiceIndex;
	int32 replyVar1;

	void SerializeAndAppendToBlob(TArray<int32>& blob) final
	{
		FNetworkCommand::SerializeAndAppendToBlob(blob);
		blob.Add(replyReceiverIndex);
		blob.Add(choiceIndex);
		blob.Add(replyVar1);
	}

	void DeserializeFromBlob(const TArray<int32>& blob, int32& index) final
	{
		FNetworkCommand::DeserializeFromBlob(blob, index);
		replyReceiverIndex = blob[index++];
		choiceIndex = blob[index++];
		replyVar1 = blob[index++];
	}
};

class FRerollCards final : public FNetworkCommand
{
public:
	virtual ~FRerollCards() {}
	NetworkCommandEnum commandType() final { return NetworkCommandEnum::RerollCards; }

	void SerializeAndAppendToBlob(TArray<int32>& blob) final {
		FNetworkCommand::SerializeAndAppendToBlob(blob);
	}

	void DeserializeFromBlob(const TArray<int32>& blob, int32& index) final {
		FNetworkCommand::DeserializeFromBlob(blob, index);
	}
};

class FSelectRareCard final : public FNetworkCommand
{
public:
	virtual ~FSelectRareCard() {}
	NetworkCommandEnum commandType() override { return NetworkCommandEnum::SelectRareCard; }

	CardEnum cardEnum = CardEnum::None;

	void SerializeAndAppendToBlob(TArray<int32>& blob) override {
		FNetworkCommand::SerializeAndAppendToBlob(blob);
		blob.Add(static_cast<int32>(cardEnum));
	}

	void DeserializeFromBlob(const TArray<int32>& blob, int32& index) override {
		FNetworkCommand::DeserializeFromBlob(blob, index);
		cardEnum = static_cast<CardEnum>(blob[index++]);
	}
};

class FBuyCard final : public FNetworkCommand
{
public:
	virtual ~FBuyCard() {}
	NetworkCommandEnum commandType() override { return NetworkCommandEnum::BuyCard; }

	TArray<int32> cardHandBuyIndices;

	void SerializeAndAppendToBlob(TArray<int32>& blob) override {
		FNetworkCommand::SerializeAndAppendToBlob(blob);
		SerializeArray(blob, cardHandBuyIndices);
	}

	void DeserializeFromBlob(const TArray<int32>& blob, int32& index) override {
		FNetworkCommand::DeserializeFromBlob(blob, index);
		cardHandBuyIndices = DeserializeArray(blob, index);
	}
};

class FSellCards final : public FNetworkCommand
{
public:
	virtual ~FSellCards() {}
	NetworkCommandEnum commandType() final { return NetworkCommandEnum::SellCards; }

	CardEnum buildingEnum = CardEnum::None;
	int32_t cardCount = 0;

	void SerializeAndAppendToBlob(TArray<int32>& blob) final {
		FNetworkCommand::SerializeAndAppendToBlob(blob);
		blob.Add(static_cast<int32>(buildingEnum));
		blob.Add(cardCount);
	}

	void DeserializeFromBlob(const TArray<int32>& blob, int32& index) final {
		FNetworkCommand::DeserializeFromBlob(blob, index);
		buildingEnum = static_cast<CardEnum>(blob[index++]);
		cardCount = blob[index++];
	}
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

	void SerializeAndAppendToBlob(TArray<int32>& blob) final {
		FNetworkCommand::SerializeAndAppendToBlob(blob);
		blob.Add(static_cast<int32>(cardEnum));
		blob.Add(variable1);
		blob.Add(variable2);

		blob.Add(positionX100);
		blob.Add(positionY100);

		//blob.Add(animationStartTime100);
	}

	void DeserializeFromBlob(const TArray<int32>& blob, int32& index) final {
		FNetworkCommand::DeserializeFromBlob(blob, index);
		cardEnum = static_cast<CardEnum>(blob[index++]);
		variable1 = blob[index++];
		variable2 = blob[index++];

		positionX100 = blob[index++];
		positionY100 = blob[index++];

		//animationStartTime100 = blob[index++];
	}

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

	void SerializeAndAppendToBlob(TArray<int32>& blob) final {
		FNetworkCommand::SerializeAndAppendToBlob(blob);
		blob.Add(static_cast<int32>(techEnum));
	}

	void DeserializeFromBlob(const TArray<int32>& blob, int32& index) final {
		FNetworkCommand::DeserializeFromBlob(blob, index);
		techEnum = static_cast<TechEnum>(blob[index++]);
	}
};

class FClaimLand final : public FNetworkCommand
{
public:
	virtual ~FClaimLand() {}
	NetworkCommandEnum commandType() final { return NetworkCommandEnum::ClaimLand; }

	int32 provinceId = -1;
	CallbackEnum claimEnum = CallbackEnum::ClaimLandMoney;

	void SerializeAndAppendToBlob(TArray<int32>& blob) final {
		FNetworkCommand::SerializeAndAppendToBlob(blob);
		blob.Add(provinceId);
		blob.Add(static_cast<int32>(claimEnum));
	}

	void DeserializeFromBlob(const TArray<int32>& blob, int32& index) final {
		FNetworkCommand::DeserializeFromBlob(blob, index);
		provinceId = blob[index++];
		claimEnum = static_cast<CallbackEnum>(blob[index++]);
	}
};

class FUnslotCard final : public FNetworkCommand
{
public:
	virtual ~FUnslotCard() {}
	NetworkCommandEnum commandType() final { return NetworkCommandEnum::UnslotCard; }

	int32 buildingId = -1;
	int32 unslotIndex = -1;

	void SerializeAndAppendToBlob(TArray<int32>& blob) final {
		FNetworkCommand::SerializeAndAppendToBlob(blob);
		blob.Add(buildingId);
		blob.Add(unslotIndex);
	}

	void DeserializeFromBlob(const TArray<int32>& blob, int32& index) final {
		FNetworkCommand::DeserializeFromBlob(blob, index);
		buildingId = blob[index++];
		unslotIndex = blob[index++];
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

	void SerializeAndAppendToBlob(TArray<int32>& blob) final {
		FNetworkCommand::SerializeAndAppendToBlob(blob);
		blob.Add(originNodeId);
		blob.Add(targetNodeId);
		blob.Add(helpPlayerId);
		SerializeArray(blob, armyCounts);
		blob.Add(static_cast<int32>(armyOrderEnum));
	}

	void DeserializeFromBlob(const TArray<int32>& blob, int32& index) final {
		FNetworkCommand::DeserializeFromBlob(blob, index);
		originNodeId = blob[index++];
		targetNodeId = blob[index++];
		helpPlayerId = blob[index++];
		armyCounts = DeserializeArray(blob, index);
		armyOrderEnum = static_cast<CallbackEnum>(blob[index++]);
	}
};

class FTrainUnit final : public FNetworkCommand
{
public:
	virtual ~FTrainUnit() {}
	NetworkCommandEnum commandType() final { return NetworkCommandEnum::TrainUnit; }

	int32 buildingId = -1;
	bool isCancel = false;

	void SerializeAndAppendToBlob(TArray<int32>& blob) final {
		FNetworkCommand::SerializeAndAppendToBlob(blob);
		blob.Add(buildingId);
		blob.Add(isCancel);
	}

	void DeserializeFromBlob(const TArray<int32>& blob, int32& index) final {
		FNetworkCommand::DeserializeFromBlob(blob, index);
		buildingId = blob[index++];
		isCancel = blob[index++];
	}
};

class FChooseLocation final : public FNetworkCommand
{
public:
	virtual ~FChooseLocation() {}
	NetworkCommandEnum commandType() override { return NetworkCommandEnum::ChooseLocation; }

	int32 provinceId;
	int8 isChoosingOrReserving;

	void SerializeAndAppendToBlob(TArray<int32>& blob) override 
	{
		FNetworkCommand::SerializeAndAppendToBlob(blob);
		blob.Add(provinceId);
		blob.Add(isChoosingOrReserving);
	}

	void DeserializeFromBlob(const TArray<int32>& blob, int32& index) override
	{
		FNetworkCommand::DeserializeFromBlob(blob, index);
		provinceId = blob[index++];
		isChoosingOrReserving = blob[index++];
	}
};

class FCheat : public FNetworkCommand
{
public:
	virtual ~FCheat() {}
	int32 cheatEnum;
	int32 var1;
	int32 var2;

	NetworkCommandEnum commandType() override { return NetworkCommandEnum::Cheat; }

	void SerializeAndAppendToBlob(TArray<int32>& blob) override
	{
		FNetworkCommand::SerializeAndAppendToBlob(blob);
		blob.Add(cheatEnum);
		blob.Add(var1);
		blob.Add(var2);
	}

	void DeserializeFromBlob(const TArray<int32>& blob, int32& index) override
	{
		FNetworkCommand::DeserializeFromBlob(blob, index);
		cheatEnum = blob[index++];
		var1 = blob[index++];
		var2 = blob[index++];
	}
};


//! Abstract away the conversion to network blob
class NetworkHelper
{
public:
	static void SerializeAndAppendToBlob(std::shared_ptr<FNetworkCommand> command, TArray<int32>& blob) 
	{
		command->SerializeAndAppendToBlob(blob);
	}

	//! This is needed since the class of command is not determined in advance.
#define CASE_COMMAND(commandEnum, classType) case commandEnum: command = std::make_shared<classType>(); break;
	static std::shared_ptr<FNetworkCommand> DeserializeFromBlob(const TArray<int32>& blob, int32& index) 
	{
		check(index < blob.Num());

		NetworkCommandEnum commandEnum = FNetworkCommand::GetCommandTypeFromBlob(blob, index);
		std::shared_ptr<FNetworkCommand> command;
		switch (commandEnum)
		{
			//CASE_COMMAND(NetworkCommandEnum::AddPlayer, FAddPlayer);

			CASE_COMMAND(NetworkCommandEnum::PlaceBuilding, FPlaceBuildingParameters);
			CASE_COMMAND(NetworkCommandEnum::PlaceGather, FPlaceGatherParameters);
			CASE_COMMAND(NetworkCommandEnum::JobSlotChange, FJobSlotChange);
			CASE_COMMAND(NetworkCommandEnum::SetAllowResource, FSetAllowResource);
			CASE_COMMAND(NetworkCommandEnum::SetPriority, FSetPriority);
			CASE_COMMAND(NetworkCommandEnum::SetTownPriority, FSetTownPriority);

			CASE_COMMAND(NetworkCommandEnum::TradeResource, FTradeResource);
			CASE_COMMAND(NetworkCommandEnum::SetIntercityTrade, FSetIntercityTrade);
			
			CASE_COMMAND(NetworkCommandEnum::UpgradeBuilding, FUpgradeBuilding);
			CASE_COMMAND(NetworkCommandEnum::ChangeWorkMode, FChangeWorkMode);
			CASE_COMMAND(NetworkCommandEnum::ChooseLocation, FChooseLocation);
			CASE_COMMAND(NetworkCommandEnum::Cheat, FCheat);
			CASE_COMMAND(NetworkCommandEnum::PopupDecision, FPopupDecision);
			CASE_COMMAND(NetworkCommandEnum::RerollCards, FRerollCards);
			CASE_COMMAND(NetworkCommandEnum::SelectRareCard, FSelectRareCard);
			CASE_COMMAND(NetworkCommandEnum::BuyCard, FBuyCard);
			CASE_COMMAND(NetworkCommandEnum::SellCards, FSellCards);
			CASE_COMMAND(NetworkCommandEnum::UseCard, FUseCard);
			CASE_COMMAND(NetworkCommandEnum::UnslotCard, FUnslotCard);

			CASE_COMMAND(NetworkCommandEnum::Attack,	FAttack);
			CASE_COMMAND(NetworkCommandEnum::TrainUnit, FTrainUnit);

			CASE_COMMAND(NetworkCommandEnum::ClaimLand, FClaimLand);
			CASE_COMMAND(NetworkCommandEnum::ChooseResearch, FChooseResearch);

			CASE_COMMAND(NetworkCommandEnum::ChangeName, FChangeName);
			CASE_COMMAND(NetworkCommandEnum::SendChat, FSendChat);

			

			default: UE_DEBUG_BREAK();
		}
		command->DeserializeFromBlob(blob, index);

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
		//blob.Add(playerCount);
		blob.Add(gameSpeed);
		blob.Add(tickCountSim);

		for (size_t i = 0; i < commands.size(); i++) {
			commands[i]->SerializeAndAppendToBlob(blob);
		}
	}

	void DeserializeFromBlob(const TArray<int32>& blob, int index = 0)
	{
		tickCount = blob[index++];
		//playerCount = blob[index++];
		gameSpeed = blob[index++];
		tickCountSim = blob[index++];

		LOOP_CHECK_START();
		
		while (index < blob.Num()) {
			commands.push_back(NetworkHelper::DeserializeFromBlob(blob, index));

			LOOP_CHECK_END();
		}

		check(index == blob.Num());
	}

	FString ToString() {
		FString string = FString::Printf(TEXT("TickInfo serverTick:%d simTick:%d commands:%d, "), tickCount, tickCountSim, commands.size());
		for (size_t i = 0; i < commands.size(); i++) {
			string.Append(FString("|") + ToFString(NetworkCommandNames[static_cast<int>(commands[i]->commandType())]));
		}
		return string;
	}

};