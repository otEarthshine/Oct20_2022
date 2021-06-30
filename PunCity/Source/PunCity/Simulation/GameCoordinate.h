#pragma once

#include "GameSimulationConstants.h"
#include "PunCity/GameRand.h"
#include "PunCity/PunSerialize.h"
#include <vector>
#include <string>
#include <algorithm>


enum class Direction : uint8
{
	S,
	E,
	N,
	W,
};

static const std::string DirectionName[] = {
	"South",
	"East",
	"North",
	"West",
};

inline Direction OppositeDirection(Direction direction) {
	switch (direction) {
	case Direction::N: return Direction::S;
	case Direction::S: return Direction::N;
	case Direction::E: return Direction::W;
	case Direction::W: return Direction::E;
	}
	UE_DEBUG_BREAK();
	return Direction::N; // TODO: change to +2
}

static const int32 DirectionCount = _countof(DirectionName);

inline Direction RotateDirection(Direction direction) {
	return static_cast<Direction>((static_cast<int>(direction) - 1 + 4) % 4); // -1 to make it clock-wise
}

inline Direction RotateDirection(Direction direction, Direction directionRotator) {
	return static_cast<Direction>((static_cast<int>(direction) + static_cast<int>(directionRotator)) % 4);
}

inline float RotationFromDirection(Direction direction) 
{
	if (direction == Direction::S) {
		return 0.0f;
	}
	if (direction == Direction::W) {
		return 90.0f;
	}
	if (direction == Direction::N) {
		return 180.0f;
	}
	else {
		return 270.0f;
	}
}


struct Float2
{
	float x, y;
};

struct Int2
{
	int x, y;
};

struct WorldTile2;
struct WorldRegion2;

struct WorldAtom2
{
	int32 x, y;

	WorldAtom2() : x(-1), y(-1) { }
	WorldAtom2(int32_t x, int32_t y) : x(x), y(y) { }

	static WorldAtom2 Zero;
	static WorldAtom2 Invalid;

	static int32_t Distance(WorldAtom2 start, WorldAtom2 end);

	static bool DistanceLessThan(WorldTile2 start, WorldTile2 end, int32_t tileRadius);

	//! amount in 1/100000 (0.001%)
	static WorldAtom2 Lerp(WorldAtom2 start, WorldAtom2 end, int64_t fraction100000);

	//! NOT USED? Return if we arrived at the destination
	bool MoveTowards(WorldAtom2 target, int64_t atomPerTick) {
		int64_t diffX = static_cast<int64_t>(target.x - x);
		int64_t diffY = static_cast<int64_t>(target.y - y);

		// arrived at destination
		if (abs(diffX) < atomPerTick && abs(diffY) < atomPerTick) {
			x = target.x;
			y = target.y;
			return true;
		}

		int64_t sumSquare = diffX * diffX + diffY * diffY;
		int64_t length = static_cast<int64_t>(sqrt(static_cast<double>(sumSquare)));

		//! Can do a while() check for sqrt result if this results in divergence
		int64_t moveX = (diffX * atomPerTick) / length; // diffX * atomPerTick first for precision
		int64_t moveY = (diffY * atomPerTick) / length;

		x += moveX;
		y += moveY;

		return false;
	}

	std::string ToString() { 
		return "(" + std::to_string(float(x) / GameMapConstants::TilesPerWorldX) + "," 
					+ std::to_string(float(y) / GameMapConstants::TilesPerWorldY) + ")";
	}


	WorldTile2 worldTile2() const;
	WorldRegion2 region() const;

	bool operator==(const WorldAtom2& a) {
		return x == a.x && y == a.y;
	}

	bool operator!=(const WorldAtom2& a) {
		return x != a.x || y != a.y;
	}

	WorldAtom2 operator+(const WorldAtom2& a) {
		return WorldAtom2(a.x + x, a.y + y);
	}

	WorldAtom2 operator+(const int32_t& a) {
		return WorldAtom2(a + x, a + y);
	}

	void operator+=(const WorldAtom2& a) {
		x += a.x;
		y += a.y;
	}

	WorldAtom2 operator*(const int& a) {
		return WorldAtom2(x * a, y * a);
	}

	WorldAtom2 operator/(const int& a) {
		return WorldAtom2(x / a, y / a);
	}

	WorldAtom2 operator-(const WorldAtom2& a) {
		return WorldAtom2(x - a.x, y - a.y);
	}

	//! Serialize
	FArchive& operator>>(FArchive &Ar) {
		Ar << x << y;
		return Ar;
	}
};

struct LocalTile2
{
	int8_t x, y;

	LocalTile2() : x(0), y(0) { }
	LocalTile2(int8_t x, int8_t y) : x(x), y(y) { }
	LocalTile2(int localTileId) {
		x = localTileId % CoordinateConstants::TilesPerRegion;
		y = localTileId / CoordinateConstants::TilesPerRegion;
	}

	int16_t tileId() { return x + y * CoordinateConstants::TilesPerRegion; }

	WorldTile2 worldTile2(WorldRegion2 region);

	bool isValid() {
		return x >= 0 && y >= 0 && x < CoordinateConstants::TilesPerRegion && y < CoordinateConstants::TilesPerRegion;
	}

	FVector localDisplayLocation() {
		return FVector(x * CoordinateConstants::DisplayUnitPerTile, y * CoordinateConstants::DisplayUnitPerTile, 0.0f);
	}

	LocalTile2 operator+(const LocalTile2& a) {
		return LocalTile2(a.x + x, a.y + y);
	}
};

// Faster version?
struct WorldTile2Int
{
	int32_t xy;
	WorldTile2Int() : xy(0) { }
	WorldTile2Int(int32_t xy) : xy(xy) { }
	WorldTile2Int(int32_t x, int32_t y, int32_t dimX) : xy(x + y * dimX) { }
	inline int32_t x(int32_t dimX) { return xy % dimX; }
	inline int32_t y(int32_t dimX) { return xy / dimX; }
	inline WorldTile2Int add(const WorldTile2Int& a) const { return WorldTile2Int(xy + a.xy); }
	inline WorldTile2Int subtract(const WorldTile2Int& a) const { return WorldTile2Int(xy - a.xy); }
	inline WorldTile2Int divide(int a, int32_t dimX) {
		return WorldTile2Int(x(dimX) + a, y(dimX) + a, dimX);
	}
	static inline int32_t Distance(const WorldTile2Int& start, const WorldTile2Int& end, int32_t dimX) {
		WorldTile2Int v = start.subtract(end);
		return (int32_t)sqrt(v.x(dimX) * v.x(dimX) + v.y(dimX) * v.y(dimX));
	}
};

// TODO: found that int is faster than WorldTile2??
//#pragma pack(push, 1)
struct WorldTile2
{
	int16_t x, y;

	static const WorldTile2 Zero;
	static const WorldTile2 Invalid;

	WorldTile2() : x(0), y(0) { }
	WorldTile2(int16_t x, int16_t y) : x(x), y(y) { }
	WorldTile2(int32_t tileId);

	WorldAtom2 worldAtom2() const { 
		return WorldAtom2(x, y) * CoordinateConstants::AtomsPerTile + CoordinateConstants::HalfAtomsPerTile;
	}
	WorldRegion2 region() const;
	int32_t regionId() const;

	LocalTile2 localTile() const;
	int16_t localTileId() const;

	LocalTile2 localTile(WorldRegion2 region) const; // Local tile can be negative for display placement

	FVector displayLocation() {
		return FVector(x * CoordinateConstants::DisplayUnitPerTile, y * CoordinateConstants::DisplayUnitPerTile, 0);
	}

	std::vector<WorldRegion2> nearby4Regions() const;

	std::string ToString() { return "(" + std::to_string(x) + ", " + std::to_string(y) + ")"; }
	FString To_FString() { return  FString(ToString().c_str()); }
	FText ToText() { return FText::FromString(FString(ToString().c_str())); }

	int32_t tileId() const;
	bool isValid() const { return x >= 0 && x < GameMapConstants::TilesPerWorldX &&
							y >= 0 && y < GameMapConstants::TilesPerWorldY; }

	static int32_t ManDistance(WorldTile2 start, WorldTile2 end) {
		WorldTile2 v = end - start;
		return abs(v.x) + abs(v.y);
	}

	static int32_t ManDiagDistance(WorldTile2 start, WorldTile2 end) {
		WorldTile2 v = end - start;
		int16_t vx = abs(v.x);
		int16_t vy = abs(v.y);
		return vx > vy ? vx : vy;
	}

	// TODO: use ref?
	static int32_t Distance(WorldTile2 start, WorldTile2 end) {
		WorldTile2 v = end - start;
		return static_cast<int32_t>(sqrt(v.x * v.x + v.y * v.y));
	}

	WorldTile2 operator+(const WorldTile2& a) const {
		return WorldTile2(x + a.x, y + a.y);
	}

	WorldTile2 operator-(const WorldTile2& a) const {
		return WorldTile2(x - a.x, y - a.y);
	}

	WorldTile2 operator+(const int16_t& a) const {
		return WorldTile2(x + a, y + a);
	}

	WorldTile2 operator-(const int16_t& a) const {
		return WorldTile2(x - a, y - a);
	}

	WorldTile2 operator*(const WorldTile2& a) const {
		return WorldTile2(a.x * x, a.y * y);
	}

	WorldTile2 operator*(int a) const {
		return WorldTile2(a * x, a * y);
	}

	WorldTile2 operator/(int a) const {
		return WorldTile2(x / a, y / a);
	}

	void operator+=(const WorldTile2& a) {
		x += a.x;
		y += a.y;
	}

	bool operator==(const WorldTile2& a) const {
		return x == a.x && y == a.y;
	}
	bool operator!=(const WorldTile2& a) const {
		return x != a.x || y != a.y;
	}

	////! Append onto the network blob
	//void SerializeAndAppendToBlob(TArray<int32>& blob) {
	//	blob.Add(x);
	//	blob.Add(y);
	//}

	////! Read from network blob using index
	//void DeserializeFromBlob(const TArray<int32>& blob, int32& index) {
	//	x = blob[index++];
	//	y = blob[index++];
	//}

	static WorldTile2 DirectionTile(Direction direction) {
		switch (direction) {
		case Direction::N: return WorldTile2(1, 0);
		case Direction::S: return WorldTile2(-1, 0);
		case Direction::E: return WorldTile2(0, 1);
		case Direction::W: return WorldTile2(0, -1);
		default:
			UE_DEBUG_BREAK();
		}
		return WorldTile2();
	}

	// For buildings, centerTile always stay in the same spot.
	// When building rotates x,y swap and its sign changed
	// Direction::S means 0 rotation
	static WorldTile2 RotateTileVector(WorldTile2 tile, Direction direction) {
		switch (direction) {
		case Direction::S: return tile;
		case Direction::E: return WorldTile2(tile.y, -tile.x);
		case Direction::N: return WorldTile2(-tile.x, -tile.y);
		case Direction::W: return WorldTile2(-tile.y, tile.x);
		default:
			UE_DEBUG_BREAK();
		}
		return WorldTile2();
	}
	// This is needed if we use _area.centerTile() when there are actually rotations
	static WorldTile2 EvenSizeRotationCenterShift(WorldTile2 tile, Direction direction) {
		if (direction == Direction::E) return tile + WorldTile2(0, 1);
		if (direction == Direction::N) return tile + WorldTile2(1, 1);
		if (direction == Direction::W) return tile + WorldTile2(1, 0);
		return tile;
	}

	bool IsAdjacentTo(WorldTile2 tile) const {
		WorldTile2 diffTile = (*this - tile);
		return diffTile == WorldTile2(1, 0) ||
			diffTile == WorldTile2(-1, 0) ||
			diffTile == WorldTile2(0, 1) ||
			diffTile == WorldTile2(0, -1);
	}

	bool Is8AdjacentTo(WorldTile2 tile) const {
		WorldTile2 diffTile = (*this - tile);
		return diffTile == WorldTile2(1, 0) ||
			diffTile == WorldTile2(-1, 0) ||
			diffTile == WorldTile2(0, 1) ||
			diffTile == WorldTile2(0, -1) ||

			diffTile == WorldTile2(1, 1) ||
			diffTile == WorldTile2(-1, 1) ||
			diffTile == WorldTile2(1, -1) ||
			diffTile == WorldTile2(-1, -1);
	}


	FArchive& operator>>(FArchive &Ar) {
		Ar << x << y;
		return Ar;
	}

	int32 GetHash() const {
		return x + y;
	}


	int32 maxElement() const { return std::max(x, y); }
};
//#pragma pack(pop)

struct WorldTile4x4
{
	//int16_t x, y;

	//WorldTile4x4() : x(0), y(0) { }
	//WorldTile4x4(int16_t x, int16_t y) : x(x), y(y) { }
	//WorldTile4x4(WorldTile2 tile) : x(tile.x / 4), y(tile.y / 4) { }

	//int32_t tile4x4Id();

	// Add tile in 3x3 around tile4x4 map
	template<typename T>
	static void AddTileTo4x4(int32_t tileX, int32_t tileY, std::vector<T>& v, int32_t dimX, int32_t dimY)
	{
		int32_t tile4x4_X = tileX / 4;
		int32_t tile4x4_Y = tileY / 4;

		int32_t tile4x4_X_neg = std::max(0, tile4x4_X - 1);
		int32_t tile4x4_Y_neg = std::max(0, tile4x4_Y - 1);
		int32_t tile4x4_X_pos = std::min(dimX - 1, tile4x4_X + 1);
		int32_t tile4x4_Y_pos = std::min(dimY - 1, tile4x4_Y + 1);

		for (int32_t yy = tile4x4_Y_neg; yy <= tile4x4_Y_pos; yy++) {
			for (int32_t xx = tile4x4_X_neg; xx <= tile4x4_X_pos; xx++) {
				v[xx + yy * dimX] = 255; // 200 for smooth...
			}
		}
	}

	template<typename Func>
	static int32 Get4x4Lerped(WorldTile2 tile, int32 dim4x4_X, int32 dim4x4_Y, Func get4x4Value)
	{
		int32 mapX0 = tile.x / 4;
		int32 mapY0 = tile.y / 4;
		int32 mapX1 = std::min(mapX0 + 1, dim4x4_X - 1);
		int32 mapY1 = std::min(mapY0 + 1, dim4x4_Y - 1);

		int32 value00 = get4x4Value(mapX0 + mapY0 * dim4x4_X);
		int32 value01 = get4x4Value(mapX0 + mapY1 * dim4x4_X);
		int32 value10 = get4x4Value(mapX1 + mapY0 * dim4x4_X);
		int32 value11 = get4x4Value(mapX1 + mapY1 * dim4x4_X);

		int32 xMod = tile.x % 4;
		int32 value_0 = ((4 - xMod) * value00 + xMod * value10) / 4;
		int32 value_1 = ((4 - xMod) * value01 + xMod * value11) / 4;

		int32 yMod = tile.y % 4;
		return ((4 - yMod) * value_0 + yMod * value_1) / 4;
	}

	template<typename T>
	static void Blur(int smoothRadius, std::vector<T>& v, int32_t dimX, int32_t dimY)
	{
		std::vector<T> tempMap(v.size());

		//UE_LOG(LogTemp, Error, TEXT("WTFFFF dimX: %d dimY: %d"), dimX, dimY);

		// Horizontal Blur
		// TODO: Do sum move...
		for (int y = 0; y < dimY; y++) {
			for (int x = 0; x < dimX; x++)
			{
				int minX = std::max(x - smoothRadius, 0);
				int maxX = std::min(x + smoothRadius, dimX - 1);
				int32_t sum = 0;
				for (int xx = minX; xx <= maxX; xx++) {
					sum += v[xx + y * dimX];
				}
				tempMap[x + y * dimX] = sum / (maxX - minX + 1);
			}
		}

		// Vertical blur
		for (int y = 0; y < dimY; y++) {
			for (int x = 0; x < dimX; x++)
			{
				int minY = std::max(y - smoothRadius, 0);
				int maxY = std::min(y + smoothRadius, dimY - 1);
				int32_t sum = 0;
				for (int yy = minY; yy <= maxY; yy++) {
					sum += tempMap[x + yy * dimX];
				}
				v[x + y * dimX] = sum / (maxY - minY + 1);
			}
		}
	}
};

struct WorldRegion2
{
	int32_t x, y;

	WorldRegion2() : x(-1), y(-1) {}
	WorldRegion2(int32_t regionId);
	WorldRegion2(int32_t x, int32_t y) : x(x), y(y) {}

	int32_t regionId() const;

	WorldTile2 worldTile2(LocalTile2 localTile2) {
		return WorldTile2(localTile2.x + minXTile(), localTile2.y + minYTile());
	}

	WorldAtom2 worldAtom2() {
		return WorldAtom2(x * CoordinateConstants::AtomsPerRegion,
						  y * CoordinateConstants::AtomsPerRegion);
	}

	WorldTile2 centerTile() const {
		return WorldTile2(CoordinateConstants::TilesPerRegion / 2 - 1 + minXTile(), CoordinateConstants::TilesPerRegion / 2 - 1 + minYTile());
	}

	int32_t minXTile() const { return x * CoordinateConstants::TilesPerRegion; }
	int32_t minYTile() const { return y * CoordinateConstants::TilesPerRegion; }
	WorldTile2 minTile() { return WorldTile2(x, y) * CoordinateConstants::TilesPerRegion; }

	int32_t maxXTile() { return (x + 1) * CoordinateConstants::TilesPerRegion - 1; }
	int32_t maxYTile() { return (y + 1) * CoordinateConstants::TilesPerRegion - 1; }
	WorldTile2 maxTile() { return WorldTile2(maxXTile(), maxYTile()); }

	bool IsInRegion(WorldAtom2 location)  { 
		return location.x / CoordinateConstants::AtomsPerRegion == x && 
				location.y / CoordinateConstants::AtomsPerRegion == y;
	}

	bool IsInRegion(WorldTile2 tile) {
		return tile.x / CoordinateConstants::TilesPerRegion == x &&
			tile.y / CoordinateConstants::TilesPerRegion == y;
	}

	static WorldRegion2 Invalid() { return WorldRegion2(-1, -1); }

	bool IsValid() const {
		return x >= 0 && x < GameMapConstants::RegionsPerWorldX &&
				y >= 0 && y < GameMapConstants::RegionsPerWorldY;
	}

	template<typename Functor>
	void ExecuteOnRegion_Tile(Functor functor) {
		const int32_t lastLocal = CoordinateConstants::TilesPerRegion - 1;
		for (int16_t yy = minYTile(); yy <= minYTile() + lastLocal; yy++) { // Faster loop
			for (int16_t xx = minXTile(); xx <= minXTile() + lastLocal; xx++) {
				functor(xx, yy);
			}
		}
	}

	template<typename Functor>
	void ExecuteOnRegion_WorldTile(Functor functor) {
		const int32 lastLocal = CoordinateConstants::TilesPerRegion - 1;
		for (int16 yy = minYTile(); yy <= minYTile() + lastLocal; yy++) { // Faster loop
			for (int16 xx = minXTile(); xx <= minXTile() + lastLocal; xx++) {
				functor(WorldTile2(xx, yy));
			}
		}
	}

	std::string ToString() { return "(" + std::to_string(x) + "," + std::to_string(y) + ")"; }
	FString ToFString() { return "(" + FString::FromInt(x) + "," + FString::FromInt(y) + ")"; }

	bool operator==(const WorldRegion2& a) const {
		return x == a.x && y == a.y;
	}

	WorldRegion2 neighbor(Direction direction) {
		WorldRegion2 neighborRegion(x, y);
		switch (direction) {
		case Direction::N: neighborRegion.x = neighborRegion.x + 1; break;
		case Direction::S: neighborRegion.x = neighborRegion.x - 1; break;
		case Direction::E: neighborRegion.y = neighborRegion.y + 1; break;
		case Direction::W: neighborRegion.y = neighborRegion.y - 1; break;
		default:
			UE_DEBUG_BREAK();
		}
		return neighborRegion;
	}

	template <typename Func>
	void ExecuteOnNearbyRegions(Func func)
	{
		for (int32 rY = y - 1; rY <= y + 1; rY++) {
			for (int32 rX = x - 1; rX <= x + 1; rX++) {
				WorldRegion2 nearbyRegion(rX, rY);
				if (nearbyRegion.IsValid()) {
					func(nearbyRegion);
				}
			}
		}
	}

	template <typename Func>
	void ExecuteOnAdjacentRegions(Func func)
	{
		func(north());
		func(south());
		func(east());
		func(west());
	}

	bool IsAdjacent(WorldRegion2 region) const {
		return region == north() ||
			region == south() ||
			region == east() ||
			region == west();
	}

	WorldRegion2 north() const { return WorldRegion2(x + 1, y); }
	WorldRegion2 south() const { return WorldRegion2(x - 1, y); }
	WorldRegion2 east() const { return WorldRegion2(x, y + 1); }
	WorldRegion2 west() const { return WorldRegion2(x, y - 1); }

	template<typename Func>
	int32 GetAverage4x4Value(int32 dim4x4_X, int32 dim4x4_Y, Func get4x4Value)
	{
		int32 minX4 = minXTile() / 4;
		int32 minY4 = minYTile() / 4;

		int32 total = 0;
		for (int32 y4 = 0; y4 < 7; y4++) {
			for (int32 x4 = 0; x4 < 7; x4++) 
			{
				int32 worldX4 = std::min(minX4 + x4, dim4x4_X - 1);
				int32 worldY4 = std::min(minY4 + y4, dim4x4_Y - 1);
				total += get4x4Value(worldX4 + worldY4 * dim4x4_X);
			}
		}

		return total / 8 / 8;
	}


	bool IsInnerRegion() const
	{
		return x >= GameMapConstants::MinInnerRegionX && x < GameMapConstants::MaxInnerRegionX &&
			y >= GameMapConstants::MinInnerRegionY && y < GameMapConstants::MaxInnerRegionY;
	}

	static std::vector<int32> WorldRegionsToRegionIds(const std::vector<WorldRegion2>& regions)
	{
		std::vector<int32> regionIds;
		for (WorldRegion2 region : regions) {
			regionIds.push_back(region.regionId());
		}
		return regionIds;
	}
	

	//! Serialize
	FArchive& operator>>(FArchive &Ar) {
		Ar << x;
		Ar << y;
		return Ar;
	}
};

struct TileArea
{
	// TODO: change all int to int32???
	int16_t minX, minY;
	int16_t maxX, maxY; // max inclusive

	int16_t sizeX() const { return maxX - minX + 1; }
	int16_t sizeY() const { return maxY - minY + 1; }
	WorldTile2 size() { return WorldTile2(sizeX(), sizeY()); }
	int32 tileCount() { return static_cast<int32>(sizeX()) * sizeY(); }

	WorldTile2 min() { return WorldTile2(minX, minY); }
	WorldTile2 max() { return WorldTile2(maxX, maxY); }
	bool isValid() { return min().isValid() && max().isValid(); }

	WorldTile2 corner00() const { return WorldTile2(minX, minY); }
	WorldTile2 corner01() const { return WorldTile2(minX, maxY); }
	WorldTile2 corner10() const { return WorldTile2(maxX, minY); }
	WorldTile2 corner11() const { return WorldTile2(maxX, maxY); }

	WorldTile2 corner(int32 index)
	{
		switch(index)
		{
		case 0: return corner00();
		case 1: return corner01();
		case 2: return corner10();
		default: return corner11();
		}
	}
	
	TileArea(WorldTile2 start, int16_t distanceAway);
	
	TileArea() : minX(0), minY(0), maxX(0), maxY(0) { }
	TileArea(int16_t minX, int16_t minY, int16_t maxX, int16_t maxY ) : minX(minX), minY(minY), maxX(maxX), maxY(maxY) { }
	TileArea(WorldTile2 start, WorldTile2 size) : minX(start.x), minY(start.y), 
												maxX(start.x + size.x - 1), maxY(start.y + size.y - 1) {}
	TileArea(WorldRegion2 region) {
		minX = region.minXTile();
		minY = region.minYTile();
		maxX = minX + CoordinateConstants::TilesPerRegion - 1;
		maxY = minY + CoordinateConstants::TilesPerRegion - 1;
	}

	static TileArea GetLoopMinMaxInitial() {
		return TileArea(INT16_MAX, INT16_MAX, INT16_MIN, INT16_MIN);
	}

	bool isInvalid() { return minX == -1 && minY == -1 && maxX == -1 && maxY == -1; }
	static TileArea Invalid;

	bool isInMap() { return minX >= 0 && minY >= 0 && maxX < GameMapConstants::TilesPerWorldX && maxY < GameMapConstants::TilesPerWorldY; }

	bool HasTile(WorldTile2 tile) {
		return minX <= tile.x && tile.x <= maxX &&
				minY <= tile.y && tile.y <= maxY;
	}

	TileArea Move(int16 x, int16 y) {
		return TileArea(minX + x, minY + y, maxX + x, maxY + y);
	}

	WorldAtom2 trueCenterAtom() {
		return (min() + max()).worldAtom2() / 2;
	}

	WorldTile2 centerTile() {
		return (min() + max()) / 2;
	}

	WorldTile2 centerTile(Direction faceDirection) {
		return WorldTile2::EvenSizeRotationCenterShift(centerTile(), faceDirection);
	}
	
	
	std::string ToString() {
		return "[Area: min(" + std::to_string(minX) + "," + std::to_string(minY) + ") max(" + std::to_string(maxX) + "," + std::to_string(maxY) + ")]";
	}

	void EnforceWorldLimit();
	

	template<typename Functor>
	void ExecuteOnArea_Zero(Functor functor) {
		for (int y = 0; y < sizeY(); y++) {
			for (int x = 0; x < sizeX(); x++) {
				functor(x, y);
			}
		}
	}

	template<typename Functor>
	void ExecuteOnAreaWithExit_Zero(Functor functor) {
		for (int16_t y = 0; y < sizeY(); y++) {
			for (int16_t x = 0; x < sizeX(); x++) {
				if (functor(x, y)) {
					return;
				}
			}
		}
	}

	template<typename Functor>
	void ExecuteOnArea_Tile(Functor functor) const {
		for (int16_t y = minY; y <= maxY; y++) { // Faster loop
			for (int16_t x = minX; x <= maxX; x++) {
				functor(x, y);
			}
		}
	}

	template<typename Functor>
	void ExecuteOnArea_WorldTile2(Functor functor) const {
		for (int16_t y = minY; y <= maxY; y++) { // Faster loop
			for (int16_t x = minX; x <= maxX; x++) {
				functor(WorldTile2(x, y));
			}
		}
	}

	template<typename Functor>
	void ExecuteOnAreaSkip4_WorldTile2(Functor functor) const {
		for (int16 y = minY; y <= maxY; y += 4) { // Faster loop
			for (int16 x = minX; x <= maxX; x += 4) {
				functor(WorldTile2(x, y));
			}
		}
	}
	

	template<class Functor>
	bool ExecuteOnAreaWithExit_WorldTile2(Functor shouldExitFunctor) {
		for (int16_t y = minY; y <= maxY; y++) { // Faster loop
			for (int16_t x = minX; x <= maxX; x++) {
				if (shouldExitFunctor(WorldTile2(x, y))) {
					return true;
				}
			}
		}
		return false;
	}

	template<typename Functor>
	void ExecuteOnArea_WorldTile2x2(Functor functor) const {
		for (int16_t y2 = (minY / 2); y2 <= (maxY / 2); y2++) { // Faster loop
			for (int16_t x2 = (minX / 2); x2 <= (maxX / 2); x2++) {
				functor(x2, y2);
			}
		}
	}
	template<typename Functor>
	bool ExecuteOnAreaWithExit_WorldTile2x2(Functor shouldExitFunctor) const {
		for (int16_t y2 = (minY / 2); y2 <= (maxY / 2); y2++) { // Faster loop
			for (int16_t x2 = (minX / 2); x2 <= (maxX / 2); x2++) {
				if (shouldExitFunctor(x2, y2)) {
					return true;
				}
			}
		}
		return false;
	}

	// Border
	template<typename Functor>
	bool ExecuteOnBorderWithExit_WorldTile2(Functor functor) const {
		for (int16 y = minY + 1; y <= maxY - 1; y++) { // Faster loop
			if (functor(WorldTile2(minX, y))) {
				return true;
			}
			if (functor(WorldTile2(maxX, y))) {
				return true;
			}
		}
		for (int16 x = minX; x <= maxX; x++) {
			if (functor(WorldTile2(x, minY))) {
				return true;
			}
			if (functor(WorldTile2(x, maxY))) {
				return true;
			}
		}
		return false;
	}

	template<typename Functor>
	void ExecuteOnBorder_WorldTile2(Functor functor) const {
		for (int16 y = minY + 1; y <= maxY - 1; y++) { // Faster loop
			functor(WorldTile2(minX, y));
			functor(WorldTile2(maxX, y));
		}
		for (int16 x = minX; x <= maxX; x++) {
			functor(WorldTile2(x, minY));
			functor(WorldTile2(x, maxY));
		}
	}
	

	std::vector<WorldRegion2> GetOverlapRegions(bool isSmallArea) const {
		return isSmallArea ? GetOverlapRegionsSmallArea() : GetOverlapRegions();
	}

	std::vector<WorldRegion2> GetOverlapRegions() const
	{
		std::vector<WorldRegion2> resultRegions;
		
		//for (int16_t y = minY; y <= maxY; y++) { // Faster loop
		//	for (int16_t x = minX; x <= maxX; x++) {
		//		WorldRegion2 region = WorldTile2(x, y).region();
		//		if (std::find(resultRegions.begin(), resultRegions.end(), region) == resultRegions.end()) {
		//			resultRegions.push_back(region);
		//		}
		//	}
		//}

		for (int16_t y = minY; y <= maxY; y += CoordinateConstants::TilesPerRegion) { // Faster loop
			for (int16_t x = minX; x <= maxX; x += CoordinateConstants::TilesPerRegion) 
			{
				WorldRegion2 region = WorldTile2(x, y).region();
				check(std::find(resultRegions.begin(), resultRegions.end(), region) == resultRegions.end());
				
				resultRegions.push_back(region);
			}
		}
		
		return resultRegions;
	}

	std::vector<WorldRegion2> GetOverlapRegionsSmallArea() const
	{
		std::vector<WorldRegion2> regions;
		auto tryAddRegion = [&](WorldRegion2 region) {
			if (std::find(regions.begin(), regions.end(), region) == regions.end()) {
				regions.push_back(region);
			}
		};
		
		tryAddRegion(corner00().region());
		tryAddRegion(corner01().region());
		tryAddRegion(corner10().region());
		tryAddRegion(corner11().region());
		return regions;
	}

	bool HasOverlap(TileArea area2) const
	{
		return maxX >= area2.minX && area2.maxX >= minX &&
			maxY >= area2.minY && area2.maxY >= minY;
	}

	WorldTile2 RandomTile() {
		return WorldTile2(GameRand::Rand() % sizeX() + minX, GameRand::Rand() % sizeY() + minY);
	}

	TileArea tileArea2x2() { return TileArea(minX / 2, minY / 2, maxX / 2, maxY / 2); }


	TileArea RotateArea(WorldTile2 centerTile, Direction direction) // S as no rotation
	{
		WorldTile2 tile1(minX, minY);
		WorldTile2 tile2(minX, maxY);
		WorldTile2 tile3(maxX, minY);
		WorldTile2 tile4(maxX, maxY);

		WorldTile2 tileShift1 = tile1 - centerTile;
		WorldTile2 tileShift2 = tile2 - centerTile;
		WorldTile2 tileShift3 = tile3 - centerTile;
		WorldTile2 tileShift4 = tile4 - centerTile;
		
		WorldTile2 rotatedTileShift1 = WorldTile2::RotateTileVector(tileShift1, direction);
		WorldTile2 rotatedTileShift2 = WorldTile2::RotateTileVector(tileShift2, direction);
		WorldTile2 rotatedTileShift3 = WorldTile2::RotateTileVector(tileShift3, direction);
		WorldTile2 rotatedTileShift4 = WorldTile2::RotateTileVector(tileShift4, direction);

		WorldTile2 rotatedTile1 = rotatedTileShift1 + centerTile;
		WorldTile2 rotatedTile2 = rotatedTileShift2 + centerTile;
		WorldTile2 rotatedTile3 = rotatedTileShift3 + centerTile;
		WorldTile2 rotatedTile4 = rotatedTileShift4 + centerTile;

		TileArea area;
		area.minX = std::min(std::min(rotatedTile1.x, rotatedTile2.x), std::min(rotatedTile3.x, rotatedTile4.x));
		area.maxX = std::max(std::max(rotatedTile1.x, rotatedTile2.x), std::max(rotatedTile3.x, rotatedTile4.x));
		area.minY = std::min(std::min(rotatedTile1.y, rotatedTile2.y), std::min(rotatedTile3.y, rotatedTile4.y));
		area.maxY = std::max(std::max(rotatedTile1.y, rotatedTile2.y), std::max(rotatedTile3.y, rotatedTile4.y));
		
		return area;
	}

	////! Append onto the network blob
	//void SerializeAndAppendToBlob(TArray<int32>& blob) {
	//	blob.Add(minX);
	//	blob.Add(minY);
	//	blob.Add(maxX);
	//	blob.Add(maxY);
	//}

	////! Read from network blob using index
	//void DeserializeFromBlob(const TArray<int32>& blob, int32& index) {
	//	minX = blob[index++];
	//	minY = blob[index++];
	//	maxX = blob[index++];
	//	maxY = blob[index++];
	//}

	TileArea GetFrontArea(Direction faceDirection) const {
		switch (faceDirection) {
			case Direction::N: return TileArea(maxX + 1, minY, maxX + 1, maxY);
			case Direction::S: return TileArea(minX - 1, minY, minX - 1, maxY);
			case Direction::E: return TileArea(minX, maxY + 1, maxX, maxY + 1);
			case Direction::W: return TileArea(minX, minY - 1, maxX, minY - 1);
			UE_DEBUG_BREAK();
		}
		return TileArea::Invalid;
	}

	void ExpandArea(int32 tiles)
	{
		minX -= tiles;
		minY -= tiles;
		maxX += tiles;
		maxY += tiles;
	}
	void EnsureTileInsideArea(WorldTile2 tile)
	{
		if (tile.x < minX) { minX = tile.x; }
		if (tile.y < minY) { minY = tile.y; }
		if (tile.x > maxX) { maxX = tile.x; }
		if (tile.y > maxY) { maxY = tile.y; }
	}

	static TileArea CombineAreas(const TileArea& a, const TileArea& b)
	{
		return TileArea(std::min(a.minX, b.minX),
						std::min(a.minY, b.minY),
						std::max(a.maxX, b.maxX),
						std::max(a.maxY, b.maxY));
	}

	bool operator==(const TileArea& a) const
	{
		return minX == a.minX && minY == a.minY && 
				maxX == a.maxX && maxY == a.maxY;
	}

	bool operator!=(const TileArea& a) const
	{
		return minX != a.minX || minY != a.minY ||
			maxX != a.maxX || maxY != a.maxY;
	}

	FArchive& operator>>(FArchive &Ar) {
		Ar << minX << minY;
		Ar << maxX << maxY;
		return Ar;
	}

	int32 GetHash() {
		return minX + minY + maxX + maxY;
	}
};

struct WorldTile2x2
{
	int16 x;
	int16 y;

	WorldTile2x2() : x(-1), y(-1) {}
	WorldTile2x2(int32 tile2x2Id) : x(tile2x2Id % (GameMapConstants::TilesPerWorldX / 2)), y(tile2x2Id / (GameMapConstants::TilesPerWorldX / 2)) {}
	WorldTile2x2(WorldTile2 tile) : x(tile.x / 2), y(tile.y / 2) {}
	WorldTile2x2(int16 x, int16 y) : x(x), y(y) {}

	WorldTile2 worldTile2() const { return WorldTile2(x * 2, y * 2); }

	int32 tile2x2Id() const { return x + y * (GameMapConstants::TilesPerWorldX / 2); }

	bool operator==(const WorldTile2x2& tile2x2) const {
		return x == tile2x2.x && y == tile2x2.y;
	}

	WorldTile2x2 operator+(const WorldTile2x2& tile2x2) const {
		return WorldTile2x2(x + tile2x2.x, y + tile2x2.y);
	}

	WorldTile2x2 operator-(const WorldTile2x2& tile2x2) const {
		return WorldTile2x2(x - tile2x2.x, y - tile2x2.y);
	}

	bool IsInvalid() {
		return x == -1 && y == -1;
	}

	//! Serialize
	FArchive& operator>>(FArchive &Ar) {
		Ar << x;
		Ar << y;
		return Ar;
	}
};


// Utils
class MathUtils
{
public:
	static int32_t SumVector(std::vector<int32_t> v) {
		int32_t sum = 0;
		const size_t size = v.size();
		for (size_t i = 0; i < size; i++) {
			sum += v[i];
		}
		return sum;
	}

	static float LogisticFunction01(float x, float strengthFactor = 10) {
		return 1 / (1 + exp(-(x - 0.5f) * strengthFactor));
	}
};