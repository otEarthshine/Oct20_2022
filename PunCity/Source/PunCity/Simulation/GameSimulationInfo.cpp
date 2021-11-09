#include "PunCity/Simulation/GameSimulationInfo.h"
#include <algorithm>
#include "Kismet/GameplayStatics.h"
#include "PunCity/CppUtils.h"
//#include "PunCity/PunUtils.h"


#define LOCTEXT_NAMESPACE "GameSimulationInfo"

int32 TimeDisplay::_Ticks = 0;

float Time::kForcedFallSeason = 0.0f;

std::vector<int32> BldInfo::GetConstructionResources(FactionEnum factionEnum) const
{
	if (factionEnum == FactionEnum::Arab) 
	{
		if (cardEnum == CardEnum::House) {
			return GetConstructionResourceListFromResourcePairs({ ResourcePair(ResourceEnum::Clay, 20) });
		}

		auto convertWoodToOtherResource = [&](std::vector<int32>& result, ResourceEnum resourceEnum)
		{
			// Convert wood in construction to stone
			if (constructionResources.size() > 0)
			{
				int32 totalWoodPrice = constructionResources[0] * GetResourceInfo(ResourceEnum::Wood).basePrice;
				result[0] = 0;

				for (int32 i = 0; i < constructionResources.size(); i++) {
					if (ConstructionResources[i] == resourceEnum) {
						result[i] = constructionResources[i] + totalWoodPrice / GetResourceInfo(resourceEnum).basePrice;
						result[i] = result[i] / 10 * 10;
					}
				}
			}
		};

		// Trading Post/Port converted to stone
		if (cardEnum == CardEnum::TradingPort ||
			cardEnum == CardEnum::TradingPost)
		{
			std::vector<int32> result = constructionResources;
			convertWoodToOtherResource(result, ResourceEnum::Stone);
			return result;
		}
		
		// Most building's wood is converted to clay
		if (cardEnum != CardEnum::Fisher &&
			cardEnum != CardEnum::ClayPit &&
			!IsMountainMine(cardEnum))
		{
			std::vector<int32> result = constructionResources;
			convertWoodToOtherResource(result, ResourceEnum::Clay);
			return result;
		}
	}
	return constructionResources;
}

TileArea BuildingArea(WorldTile2 centerTile, WorldTile2 size, Direction faceDirection)
{
	check(size.isValid());

	// If rotated 90 or 270, size must be reversed
	if (faceDirection == Direction::W || faceDirection == Direction::E) {
		size = WorldTile2(size.y, size.x);
	}

	int16 xShiftDown = 0;
	int16 yShiftDown = 0;

	if (faceDirection == Direction::E) {
		yShiftDown = 1;
	}
	else if (faceDirection == Direction::N) {
		xShiftDown = 1;
		yShiftDown = 1;
	}
	else if (faceDirection == Direction::W) {
		xShiftDown = 1;
	}

	int16 minX = centerTile.x - (size.x - 1 + xShiftDown) / 2;
	int16 maxX = centerTile.x + (size.x - xShiftDown) / 2;
	int16 minY = centerTile.y - (size.y - 1 + yShiftDown) / 2;
	int16 maxY = centerTile.y + (size.y - yShiftDown) / 2;

	return TileArea(minX, minY, maxX, maxY);
}
WorldTile2 GetBuildingCenter(TileArea area, Direction faceDirection)
{
	//if (faceDirection == Direction::S) {
	//	
	//}
	return WorldTile2::Invalid;
}



/**
 * Map
 */

void GameMap::SetRegionsPerWorld(int regionPerWorldX, int regionPerWorldY)
{
	// TODO: get rid of GameMap
	GameMapConstants::SetRegionsPerWorld(regionPerWorldX, regionPerWorldY);
}

int32 Time::_Ticks = 0;

int32 Time::_Seconds = 0;
int32 Time::_Minutes = 0;
int32 Time::_Rounds = 0;
int32 Time::_Seasons = 0;
int32 Time::_Years = 0;

FloatDet Time::_MinCelsiusBase = 0;
FloatDet Time::_MaxCelsiusBase = 0;


void Time::ResetTicks()
{
	SetTickCount(0);

	_MinCelsiusBase = IntToFD(-10);
	_MaxCelsiusBase = IntToFD(28);
}

void Time::SetTickCount(int32 tickCount)
{
	_Ticks = tickCount;
	_Seconds = _Ticks / 60;
	_Minutes = _Seconds / 60;
	_Rounds = _Seconds / 150;
	_Seasons = _Minutes / MinutesPerSeason;
	_Years = _Seasons / 4;

}



std::vector<ResourceEnum> StaticData::FoodEnums;
int32 StaticData::FoodEnumCount = 0;




/*
 * Relationship
 */
static TArray<FText> RelationshipModifierName
{
	LOCTEXT("RelationGaveGifts", "You gave us gifts"),
	LOCTEXT("RelationStrong", "You are strong"),
	LOCTEXT("RelationBefriended", "You befriended us"),
	LOCTEXT("RelationFamily", "We are family"),
	LOCTEXT("RelationGoodTradeDeal", "Good Trade Deal"),
	LOCTEXT("Diplomatic Buildings", "Diplomatic Buildings"),

	LOCTEXT("RelationAdjacentBorders", "Adjacent borders spark tensions"),
	LOCTEXT("RelationProximity", "Townhalls proximity spark tensions"),
	LOCTEXT("RelationWeakling", "Weaklings don't deserve our respect"),
	LOCTEXT("RelationStole", "You stole from us"),
	LOCTEXT("RelationKidnapped", "You kidnapped our citizens"),
	LOCTEXT("RelationTerrorized", "Terrorism"),
	LOCTEXT("RelationAttacked", "You attacked us"),
	LOCTEXT("RelationCannibals", "We fear cannibals"),
};

FText RelationshipModifierNameInt(int32 index) {
	return RelationshipModifierName[index];
}
int32 RelationshipModifierCount() { return RelationshipModifierName.Num(); }



void RelationshipModifiers::GetAIRelationshipText(TArray<FText>& args, int32 playerId) const
{
	ADDTEXT_(LOCTEXT("Overall: ", "Overall: {0}\n"), FText::AsNumber(GetTotalRelationship(playerId)));

	const std::vector<int32>& modifiers = _relationshipModifiers[playerId];
	for (int32 i = 0; i < modifiers.size(); i++) {
		if (modifiers[i] != 0) {
			ADDTEXT_(INVTEXT("{0} {1}\n"), TEXT_NUMSIGNED(modifiers[i]), RelationshipModifierNameInt(i));
		}
	}
}

int32 RelationshipModifiers::GetTotalRelationship(int32 towardPlayerId) const
{
	return CppUtils::Sum(_relationshipModifiers[towardPlayerId]);
}



/*
 * Inventory
 */
FText UnitInventory::ToText() const {
	TArray<FText> args;
	FText inventoryText = LOCTEXT("Inventory:", "Inventory:");
	ADDTEXT_(INVTEXT("\n{0}\n"), inventoryText);
	for (const ResourcePair& pair : _resourcePairs) {
		ADDTEXT_(INVTEXT(" {0} {1}\n"), ResourceNameT(pair.resourceEnum), TEXT_NUM(pair.count));
	}
	ADDTEXT_INV_("\n");
	return JOINTEXT(args);
}

/*
 * CardStatus
 */
const CardStatus CardStatus::None = CardStatus(CardEnum::None, 0);

FArchive& CardStatus::operator>>(FArchive &Ar)
{
	if (Ar.IsSaving()) {
		ResetAnimation();
	}

	Ar << cardEnum;

	Ar << cardBirthTicks;
	Ar << stackSize;

	Ar << cardStateValue1;
	Ar << cardStateValue2;
	Ar << cardStateValue3;

	Ar << lastPositionX100;
	Ar << lastPositionY100;
	Ar << animationStartTime100;

	return Ar;
}

void CardStatus::Serialize(class PunSerializedData& blob)
{
	blob << cardEnum;

	blob << cardBirthTicks;
	blob << stackSize;

	blob << cardStateValue1;
	blob << cardStateValue2;
	blob << cardStateValue3;

	blob << lastPositionX100;
	blob << lastPositionY100;
	blob << animationStartTime100;
}

/*
 * Claim
 */

void AppendClaimConnectionString(TArray<FText>& args, ClaimConnectionEnum claimConnectionEnum)
{
	if (claimConnectionEnum == ClaimConnectionEnum::River) {
		ADDTEXT_LOCTEXT("river_claim", " <Orange>(cross river)</>");
	}
	else if (claimConnectionEnum == ClaimConnectionEnum::ShallowWater) {
		ADDTEXT_LOCTEXT("shallow_water", " (shallow water)");
	}
}


const std::vector<PermanentBonus::RareCardHand> PermanentBonus::PermanentBonusRareCardHands
{
	{ RareHandEnum::BorealCards, { CardEnum::BorealWinterFishing, CardEnum::BorealWinterResist }},
	{ RareHandEnum::BorealCards2, { CardEnum::BorealGoldOil, CardEnum::BorealPineForesting }},

	{ RareHandEnum::DesertCards, { CardEnum::DesertTradeForALiving, CardEnum::DesertOreTrade }},
	{ RareHandEnum::DesertCards2, { CardEnum::DesertGem, CardEnum::DesertIndustry }},

	{ RareHandEnum::SavannaCards, { CardEnum::SavannaHunt, CardEnum::SavannaRanch }},
	{ RareHandEnum::SavannaCards2, { CardEnum::SavannaGrasslandHerder, CardEnum::SavannaGrasslandRoamer }},

	{ RareHandEnum::JungleCards, { CardEnum::JungleGatherer, CardEnum::JungleMushroom }},
	{ RareHandEnum::JungleCards2, { CardEnum::JungleHerbFarm, CardEnum::JungleTree }},

	{ RareHandEnum::ForestCards, { CardEnum::ForestFarm, CardEnum::ForestCharcoal }},
	{ RareHandEnum::ForestCards2, { CardEnum::ForestBeer, CardEnum::ForestTools }},

	{ RareHandEnum::Era2_1_Cards, { CardEnum::Agriculturalist, CardEnum::Geologist }},
	{ RareHandEnum::Era2_2_Cards, { CardEnum::AlcoholAppreciation, CardEnum::Craftmanship }},

	{ RareHandEnum::Era3_1_Cards, { CardEnum::Rationalism, CardEnum::Romanticism }},
	{ RareHandEnum::Era3_2_Cards, { CardEnum::Protectionism, CardEnum::FreeThoughts }},

	{ RareHandEnum::Era4_1_Cards, { CardEnum::Capitalism, CardEnum::Communism }},
	{ RareHandEnum::Era4_2_Cards, { CardEnum::WondersScoreMultiplier, CardEnum::PopulationScoreMultiplier }},
};


#undef LOCTEXT_NAMESPACE