// Fill out your copyright notice in the Description page of Project Settings.

#include "PunTerrainGenerator.h"
#include "../Simulation/Perlin.h"
#include "../GameRand.h"
#include <fstream>
#include "PunCity/PunTimer.h"
#include <functional>
#include "PunCity/PunUtils.h"
#include "PunCity/PunFileUtils.h"
#include "PunCity/Simulation/Buildings/GathererHut.h"

using namespace std;

// Used for displaying lines
static FVector _mapOrigin;

void PunTerrainGenerator::Init(IGameSimulationCore* simulation, int tileDimX, int tileDimY, float displayUnitPerTile, MapSizeEnum mapSize, ULineBatchComponent* lineBatch)
{
	_simulation = simulation;
	_lineBatch = lineBatch;

	//_mapSize = mapSize;
	_mapSettings.mapSizeEnumInt = static_cast<int32>(mapSize);

	PUN_CHECK(tileDimX > 0 && tileDimX > 0);

	_tileDimX = tileDimX;
	_tileDimY = tileDimY;
	_displayUnitPerTile = displayUnitPerTile;

	_tile4x4DimX = _tileDimX / 4;
	_tile4x4DimY = _tileDimY / 4;

	perlinGenerator = Perlin();

	heightMap.resize(tileDimX * tileDimY);

	_LOG(PunTerrain, "Init tileDimX:%d tileDimY:%d displayUnitPerTile:%f", _tileDimX, _tileDimY, _displayUnitPerTile);
	
	//_regionMountainTileCount.resize(GameMapConstants::TotalRegions);
	//_regionWaterTileCount.resize(GameMapConstants::TotalRegions);

	_river4x4Map.clear();
	_river4x4Map.resize(_tile4x4DimX * _tile4x4DimY, 0);

	_rainfall4x4Map.clear();
	_rainfall4x4Map.resize(_tile4x4DimX * _tile4x4DimY, 0);

	_isCoastal4x4Map.clear();
	_isCoastal4x4Map.resize(_tile4x4DimX * _tile4x4DimY, 0);

	_isMountain4x4Map.clear();
	_isMountain4x4Map.resize(_tile4x4DimX * _tile4x4DimY, 0);

	_mapOrigin = FVector(0, 0, 0);

	_terrainMap.resize(tileDimX * tileDimY);

	/*
	 * Perlin
	 */
	perlinGenerator = Perlin();
	_perlinMap.resize(tileDimX * tileDimY);

	//UE_LOG(LogTemp, Error, TEXT("Init displayUnitPerTile:%f _tileDimX:%d, _tileDimY:%d"), displayUnitPerTile, _tileDimX, _tileDimY);
	if (_lineBatch) {
		_lineBatch->DrawLine(FVector(0, 50, 0), FVector(_tileDimX * _displayUnitPerTile, _tileDimY * _displayUnitPerTile + 50, 0), FLinearColor::Red, 0, 2, 120);
	}
}

struct CircleTile2
{
	WorldTile2 center;
	int32_t radius;

	bool IsInMap(int dimX, int dimY) {
		return center.x >= 0 && center.y >= 0 && center.x < dimX && center.y < dimY;
	}
	bool IsInArea(TileArea area) {
		return center.x >= area.minX && center.y >= area.minY && center.x <= area.maxX && center.y <= area.maxY;
	}
};

static double LineSlope(WorldTile2 p1, WorldTile2 p2) {
	return double(p2.y - p1.y) / (p2.x - p1.x);
}

struct LineEquation
{
	double m;
	double b;

	//LineEquation(WorldTile2 p1, WorldTile2 p2) {
	//	m = float(p2.y - p1.y) / (p2.x - p1.x);
	//	b = p1.y - m * p1.x; // y = mx + b .... b = y - mx
	//}
	LineEquation(double m, WorldTile2 p1) : m(m) {
		b = p1.y - m * p1.x; // y = mx + b .... b = y - mx
	}
	WorldTile2 intersect(LineEquation eq2) {
		// y = m1*x + b1, y = m2*x + b2
		// m1*x + b1 = m2*x + b2
		// x = (b2 - b1) / (m1 - m2)
		double x = double(eq2.b - b) / (m - eq2.m);
		return WorldTile2(x, int(m * x + b));
	}
};

static CircleTile2 CircumCircle(WorldTile2 pointA, WorldTile2 pointB, WorldTile2 pointC)
{
	WorldTile2 midpointAB = (pointA + pointB) / 2;
	WorldTile2 midpointAC = (pointA + pointC) / 2;

	LineEquation midExtensionAB(-1.0 / LineSlope(pointA, pointB), midpointAB);
	LineEquation midExtensionAC(-1.0 / LineSlope(pointA, pointC), midpointAC);

	CircleTile2 circle;
	circle.center = midExtensionAB.intersect(midExtensionAC);
	circle.radius = WorldTile2::Distance(circle.center, pointA);
	return circle;
}

// Find sets of trisIndices/circles
// trisIndices's int value represents index of a point in points
// each 3 trisIndices corresponds to a circle which the triangle is circumscribed
static void Delaunay(const std::vector<WorldTile2>& points, std::vector<int>& trisIndices, std::vector<CircleTile2>& circles, int dimX, int dimY, TileArea pointSampleArea, float displayUnitPerTile)
{
	SCOPE_TIMER("Delaunay");
	trisIndices.clear();

	// Loop through all set of 3 points to find valid delaunay
	for (int i = 0; i < points.size(); i++) {
		for (int j = 0; j < i; j++) {
			for (int k = 0; k < j; k++)
			{
				CircleTile2 circle = CircumCircle(points[i], points[j], points[k]);
				// If circle is beyond the point sample area, don't use it
				if (circle.IsInArea(pointSampleArea))
				{
					bool isTriangle = true;
					for (int l = 0; l < points.size(); l++) {
						if (l == i || l == j || l == k) continue;

						WorldTile2 p_l = points[l];// (points[l] % dimX, points[l] / dimX);
						if (WorldTile2::Distance(p_l, circle.center) < circle.radius) {
							isTriangle = false;
							break;
						}
					}
					if (isTriangle) {
						trisIndices.push_back(i);
						trisIndices.push_back(j);
						trisIndices.push_back(k);
						circles.push_back(circle);
						//UE_LOG(LogTemp, Error, TEXT("tris push %d, %d, %d"), i, j, k);

						//if (_lineBatch) {
						//	FVector p1 = _mapOrigin + FVector(circle.center.x * displayUnitPerTile, circle.center.y * displayUnitPerTile + 50, 0); // + 50 for weird shift
						//	FVector p2 = p1 + FVector(0, 0, 100);
						//	FVector pi(p_i.x * displayUnitPerTile, p_i.y * displayUnitPerTile + 50, 100);
						//	FVector pj(p_j.x * displayUnitPerTile, p_j.y * displayUnitPerTile + 50, 100);
						//	FVector pk(p_k.x * displayUnitPerTile, p_k.y * displayUnitPerTile + 50, 100);
						//	lineBatch->DrawLine(p1, p2, FLinearColor::Blue, 0, 2, 120);
						//	lineBatch->DrawLine(pi, p2, FLinearColor::Black, 0, 2, 120);
						//	lineBatch->DrawLine(pj, p2, FLinearColor::Black, 0, 2, 120);
						//	lineBatch->DrawLine(pk, p2, FLinearColor::Black, 0, 2, 120);
						//}
					}
				}
			}
		}
	}
}

static void FindAdjacentTrisPairs(std::vector<CircleTile2>& circles, std::vector<int>& trisIndices, int dimX, int dimY, std::vector<pair<int, int>>& adjacentTrisPairs)
{
	SCOPE_TIMER("Delaunay FindAdjacentTrisPairs");

	// Go through all pairs of circles
	for (int i = circles.size(); i-- > 0;) {
		bool circle_i_inMap = circles[i].IsInMap(dimX, dimY);

		for (int j = i; j-- > 0;) 
		{
			// if both circles are outside of the map, we don't need to draw the edge between them
			if (circle_i_inMap || circles[j].IsInMap(dimX, dimY))
			{
				// compare tris corners, 2 duplicates means these tris are adjacent
				// ii, jj loops through 3 corners of each tris
				int duplicates = 0;
				for (int ii = 0; ii < 3; ii++) {
					for (int jj = 0; jj < 3; jj++) {
						if (trisIndices[i * 3 + ii] == trisIndices[j * 3 + jj]) {
							++duplicates;
						}
					}
				}
				check(duplicates <= 2);
				if (duplicates == 2) adjacentTrisPairs.push_back(pair<int, int>(i, j));
			}
		}
	}
}

static void DrawDebugTris(const std::vector<int>& points, std::vector<int>& tris, int dimX, ULineBatchComponent* lineBatch, float displayUnitPerTile)
{
	PUN_LOG("DrawDebugTris points:%llu, tris:%llu", points.size(), tris.size());

	for (int i = 0; i < tris.size(); i += 3) {
		const float height = 100;
		FVector p1 = _mapOrigin + FVector((points[tris[i]] % dimX)	  * displayUnitPerTile, (points[tris[i]] / dimX)	* displayUnitPerTile + 50, height);
		FVector p2 = _mapOrigin + FVector((points[tris[i + 1]] % dimX) * displayUnitPerTile, (points[tris[i + 1]] / dimX)* displayUnitPerTile + 50, height);
		FVector p3 = _mapOrigin + FVector((points[tris[i + 2]] % dimX) * displayUnitPerTile, (points[tris[i + 2]] / dimX)* displayUnitPerTile + 50, height);
		lineBatch->DrawLine(p1, p2, FLinearColor::Green, 0, 2, 120);
		lineBatch->DrawLine(p2, p3, FLinearColor::Green, 0, 2, 120);
		lineBatch->DrawLine(p3, p1, FLinearColor::Green, 0, 2, 120);
		//UE_LOG(LogTemp, Error, TEXT("point:%f,%f,%f"), p1.X, p1.Y, p1.Z);
	}
}

static void DrawAdjacentTrisPairs(std::vector<CircleTile2>& circles, std::vector<pair<int, int>>& adjacentTrisPairs, ULineBatchComponent* lineBatch, float displayUnitPerTile)
{
	PUN_LOG("adjacentTrisPairs size: %llu", adjacentTrisPairs.size());

	for (int i = 0; i < adjacentTrisPairs.size(); i++) {
		auto center1 = circles[adjacentTrisPairs[i].first].center;
		auto center2 = circles[adjacentTrisPairs[i].second].center;
		FVector p1 = _mapOrigin + FVector(center1.x * displayUnitPerTile, center1.y * displayUnitPerTile + 50, 50);
		FVector p2 = _mapOrigin + FVector(center2.x * displayUnitPerTile, center2.y * displayUnitPerTile + 50, 50);
		lineBatch->DrawLine(p1, p2, FLinearColor::Blue, 0, 2, 120);
	}
}

void PunTerrainGenerator::GenerateMoisture()
{
	/*
	 * River
	 */
	WorldTile4x4::Blur(2, _river4x4Map, _tile4x4DimX, _tile4x4DimY);

	/*
	 * Ocean
	 */
	std::vector<uint8> isOcean4x4Map(_tile4x4DimX * _tile4x4DimY, 0);
	
	{
		SCOPE_TIMER("GenerateMoisture1");

		auto allNearbyWater = [&](int x4, int y4)
		{
			// rain cloud shift left (since left should be desert for desert latitude)
			int minX4 = x4;
			int minY4 = y4;
			int maxX4 = x4;// min(_tile4x4DimX, x4 + 2);
			int maxY4 = min(_tile4x4DimY - 1, y4 + 2);

			for (int yy = minY4; yy <= maxY4; yy++) {
				for (int xx = minX4; xx <= maxX4; xx++)
				{
					int nodeX = xx * 4;
					int nodeY = yy * 4;
					PUN_CHECK(nodeX + nodeY * _tileDimX < heightMap.size());
					if (heightMap[nodeX + nodeY * _tileDimX] > BeachToWaterHeight) {
						return false;
					}
				}
			}
			return true;
		};

		// Fill 4x4rainfall in water body with all surrounding 4x4 tile being water
		for (int y4 = 0; y4 < _tile4x4DimY; y4++) {
			for (int x4 = 0; x4 < _tile4x4DimX; x4++)
			{
				if (allNearbyWater(x4, y4)) {
					isOcean4x4Map[x4 + y4 * _tile4x4DimX] = 255;
				}

				int x = x4 * 4;
				int y = y4 * 4;
				int height = heightMap[x + y * _tileDimX];
				if (height > MountainHeight) {
					WorldTile4x4::AddTileTo4x4(x, y, _isMountain4x4Map, _tile4x4DimX, _tile4x4DimY);
				}
				else if (height < BeachToWaterHeight) {
					WorldTile4x4::AddTileTo4x4(x, y, _isCoastal4x4Map, _tile4x4DimX, _tile4x4DimY);
				}
			}
		}

		// Blur mountain for blocking moisture..
		WorldTile4x4::Blur(2, _isMountain4x4Map, _tile4x4DimX, _tile4x4DimY);

		WorldTile4x4::Blur(2, _isCoastal4x4Map, _tile4x4DimX, _tile4x4DimY);
	}

	{
		SCOPE_TIMER("GenerateMoisture Rainfall");
		/*
		 * Rainfall
		 */

		std::vector<int32> cloudLeftVec(_tile4x4DimX, 0);
		std::vector<int32> cloudLeftSmoothVec(_tile4x4DimX, 0);
		
		auto wind = [&](int centerId, int32& cloudLeft, int32 startCloudSize)
		{
			int32 currentOcean = isOcean4x4Map[centerId];
			if (currentOcean == 255) {
				cloudLeft = startCloudSize;
			}
			if (cloudLeft <= 100) {
				return;
			}

			// Calculate rainfall ... Affected by mountain block
			const int32 maxMountainRainFactor1000 = 124;
			int32 rainFactor1000 = 20 + maxMountainRainFactor1000 * _isMountain4x4Map[centerId] / 255;
			int32 rainAmount = cloudLeft * rainFactor1000 / 1000;

			// Rain down increasing rainfall
			_rainfall4x4Map[centerId] = min(255, _rainfall4x4Map[centerId] + rainAmount * 5);
			cloudLeft -= rainAmount;
		};
		
		const int half4x4DimX = _tile4x4DimX / 2;

		// At desert latitude, we get less rain...
		auto isDesertLatitude = [&] (int x4) {
			int32 latitudeFraction100 = abs(x4 - half4x4DimX) * 100 / half4x4DimX;
			return forestTemperatureStart100 < latitudeFraction100 && latitudeFraction100 < borealTemperatureStart100;
		};

		/*
		 * Parameters
		 */
		// Note: 5800,3000 is for Trailer
		const int32 normalCloudStartSize_MediumWet = 5800;// +2000; // 12000 // 20000;
		const int32 desertCloudStartSize_MediumWet = 3000;// +1000; // 5000;

		int32 moistureInt = static_cast<int>(_mapSettings.mapMoisture);
		int32 normalCloudStartSize = moistureInt * normalCloudStartSize_MediumWet / 2;
		int32 desertCloudStartSize = moistureInt * desertCloudStartSize_MediumWet / 2;

		// Scale moisture with map size
		switch(_mapSettings.mapSizeEnum())
		{
		case MapSizeEnum::Large: normalCloudStartSize *= 2; desertCloudStartSize *= 2; break;
		case MapSizeEnum::Small: normalCloudStartSize /= 2; desertCloudStartSize /= 2; break;
		default: break;
		}

		if (_mapSettings.mapMoisture == MapMoistureEnum::VeryHigh) {
			normalCloudStartSize = normalCloudStartSize * 20;
			desertCloudStartSize = desertCloudStartSize * 20;
		}
		

#define WESTWARD_WIND
#ifdef WESTWARD_WIND
		// Westward wind
		// Note: Westward wind is exclusively for desert latitude wind.
		for (int y4 = _tile4x4DimY - 2; y4-- > 0;) 
		{
			// Calculate all tiles perpendicular to the wind
			for (int x4 = 0; x4 < _tile4x4DimX; x4++) 
			{
				int32 startCloudSize = isDesertLatitude(x4) ? desertCloudStartSize : 0;
				wind(x4 + y4 * _tile4x4DimX, cloudLeftVec[x4], startCloudSize);
			}

			// Smooth cloud
			int smoothWidth = 11;
			int smoothSum = 0;
			for (int x4 = 0; x4 < smoothWidth; x4++) {
				smoothSum += cloudLeftVec[x4];
			}
			
			for (int x4 = smoothWidth; x4 < _tile4x4DimX; x4++)
			{
				cloudLeftSmoothVec[x4 - (smoothWidth + 1) / 2] = smoothSum / smoothWidth;
				smoothSum -= cloudLeftVec[x4 - smoothWidth];
				smoothSum += cloudLeftVec[x4];
			}
			swap(cloudLeftSmoothVec, cloudLeftVec);
		}
#endif

#define EASTWARD_WIND
#ifdef EASTWARD_WIND
		// Eastward wind
		for (int y4 = 0; y4 < _tile4x4DimY; y4++) 
		{
			for (int x4 = 0; x4 < _tile4x4DimX; x4++) {
				int32 startCloudSize = isDesertLatitude(x4) ? 0 : normalCloudStartSize;
				wind(x4 + y4 * _tile4x4DimX, cloudLeftVec[x4], startCloudSize);
			}

			// Smooth cloud
			int smoothWidth = 11;
			int smoothSum = 0;
			for (int x4 = 0; x4 < smoothWidth; x4++) {
				smoothSum += cloudLeftVec[x4];
			}

			for (int x4 = smoothWidth; x4 < _tile4x4DimX; x4++)
			{
				cloudLeftSmoothVec[x4 - (smoothWidth + 1) / 2] = smoothSum / smoothWidth;
				smoothSum -= cloudLeftVec[x4 - smoothWidth];
				smoothSum += cloudLeftVec[x4];
			}
			swap(cloudLeftSmoothVec, cloudLeftVec);
		}
#endif

		// Adjust rainfall by latitude... so that Tundra have little rain
		const int32 tundraStartNorth4x4 = tundraTemperatureStart100 * half4x4DimX / 100 + half4x4DimX;
		const int32 tundraStartSouth4x4 = half4x4DimX - tundraTemperatureStart100 * half4x4DimX / 100;
		
		for (int y4 = 0; y4 < _tile4x4DimY; y4++) {
			// South rainfall deduction
			const int32_t lerpTiles = 64;
			int32_t lerp = lerpTiles;
			for (int x4 = tundraStartSouth4x4; x4-- > 0;) {
				_rainfall4x4Map[x4 + y4 * _tile4x4DimX] = _rainfall4x4Map[x4 + y4 * _tile4x4DimX] * lerp / lerpTiles;
				lerp = max(0, lerp - 1);
			}

			// North rainfall deduction
			lerp = lerpTiles;
			for (int x4 = tundraStartNorth4x4; x4 < _tile4x4DimX; x4++) {
				_rainfall4x4Map[x4 + y4 * _tile4x4DimX] = _rainfall4x4Map[x4 + y4 * _tile4x4DimX] * lerp / lerpTiles;
				lerp = max(0, lerp - 1);
			}
		}

		// Rainfall perlin random
		PUN_DEBUG_EXPR(int32 riverCount = 0;
					   int32 riverOverride = 0;);
		
		for (int y4 = 0; y4 < _tile4x4DimY; y4++) {
			for (int x4 = 0; x4 < _tile4x4DimX; x4++) 
			{
				int32 initialRainfall = _rainfall4x4Map[x4 + y4 * _tile4x4DimX];
				int32 rainfall = initialRainfall;

				rainfall = min(rainfall, 255);
				
				// Fertility gets manipulated by perlin...
				// - We only use 0.5x0.5 of the whole perlin map
				FloatDet perlin_fd = _perlinMap[(x4 * 2) + (y4 * 4) * _tileDimX];
				perlin_fd = (perlin_fd - FDHalf) * 10 + FDHalf; // Enhance diff by X times
				perlin_fd = max(0, min(FDOne, perlin_fd)); // Clamp

				// Perlin only works within the biome
				int32 rainfall100 = rainfall * 100 / 255;
				if (rainfall100 < forestRainfallStart100) {
					rainfall100 = grasslandRainfallStart100 + FDSafeMul(rainfall100 - grasslandRainfallStart100, perlin_fd); // Grass
				}
				else {
					int32 stablePart = forestRainfallStart100 + 5; // The +10 offset rainfall's 90% max, so trees will still grow at min parts
					rainfall100 = stablePart + FDSafeMul(rainfall100 - stablePart, perlin_fd); // Forest
				}
				rainfall = rainfall100 * 255 / 100;
				//rainfall = rainfall * 90 / 100; // non-river fertility caps at 90%

				// - Can get pushed down or up, but never to less than TreePerlinMaxCutoff to prevent tree baldness
				//   (Only for area with rainFertility initially suitable for trees)
				//if (initialRainfall > RandomFertilityMinCutoffPercent) {
				//	rainfall = max(rainfall, RandomFertilityMinCutoffPercent);
				//}

				int32 riverMoisture = min(255, _river4x4Map[x4 + y4 * _tile4x4DimX] * 9);

				PUN_DEBUG_EXPR(if (riverMoisture > 0) riverCount++;);

				// river is always a fertile...
				if (riverMoisture > rainfall) {
					PUN_DEBUG_EXPR(riverOverride++;);
					rainfall = riverMoisture;
				}

				// Taiga's fertility is 80% normal (at 90% ... taiga will be 72%)
				if (GetTemperatureFraction10000(x4 * 4, rainfall100) > (borealTemperatureStart100 * 100) &&
					(rainfall * 100 / 255) > 59) // above 59, rainfall gets pushed down
				{
					rainfall = (rainfall - 59) * 80 / 100 + 59;
				}

				_rainfall4x4Map[x4 + y4 * _tile4x4DimX] = rainfall;
			}
		}

		PUN_LOG("riverCount %d, %d", riverCount, riverOverride);
	}

}

// River
void PunTerrainGenerator::Erode(std::vector<int16_t>& heightMapBeforeFlatten, std::vector<int16_t>& riverGuideMap, std::vector<int16_t>& roughPerlin_n1to1)
{
	SCOPE_TIMER("Erode");
	// TODO: make river source slimmer... the closer to river source, the more likely it is to dig less dirt?

	const double inertia = 0.005; // This gives just the right inertia to make river curves
	const int maxLifetime = 10000;

	std::vector<uint8> passedRivers(_tile4x4DimX * _tile4x4DimY, 0);

	int32 totalSourceCount = 700;
	switch(_mapSettings.mapSizeEnum()) {
		case MapSizeEnum::Small: totalSourceCount = totalSourceCount / 9; break;
		case MapSizeEnum::Medium: totalSourceCount = totalSourceCount / 3; break;
		case MapSizeEnum::Large: break;
		default: break;
	}

	// Dry map should have less river
	int32 mapMoistureInt = static_cast<int>(_mapSettings.mapMoisture);
	if (mapMoistureInt < static_cast<int>(MapMoistureEnum::Medium)) {
		totalSourceCount = totalSourceCount * (mapMoistureInt + 1) / 3;
	}
	

	int32 riverSucceeded = 0;
	
	for (int sourceCount = 0; sourceCount < totalSourceCount; sourceCount++)
	{
		int sourceX, sourceY;
		int loopCount = 0;

		// Don't start beyond tundra
		int32 tundraBandSize = _tileDimX / 2 * (100 - tundraTemperatureStart100) / 100;
		
		while (true) 
		{
			sourceX = GameRand::Rand() % (_tileDimX - tundraBandSize * 2) + tundraBandSize; // do not start river in tundra
			sourceY = GameRand::Rand() % _tileDimY;
			
			loopCount++;
			if (loopCount > 10000) {
				PUN_LOG("River Error");
				return;
			}

			FloatDet height = heightMap[sourceX + sourceY * _tileDimX];

			// Start on flat land
			if (FD0_XXX(124) <= height && height <= FD0_XXX(130)) 
			{
				// also shouldn't start near other river...
				bool nearOtherRiver = false;

				int32_t x4mid = (sourceX / 4);
				int32_t y4mid = (sourceY / 4);
				int32_t span = 4; // 5
				for (int32_t y4 = y4mid - span; y4 <= y4mid + span; y4++) {
					for (int32_t x4 = x4mid - span; x4 <= x4mid + span; x4++) {
						if (passedRivers[x4 + y4 * _tile4x4DimX] > 0) {
							nearOtherRiver = true;
						}
					}
				}

				if (!nearOtherRiver) {
					break;
				}
			}
			
		};
		// PUN_LOG("source: %d, %d height:%f displayUnitPerTile:%f", sourceX, sourceY, heightMap[sourceX + sourceY * tileDimX], displayUnitPerTile);

		//bool doneBeforeMaxDroplets = false;

		uint8_t riverId = GameRand::Rand() % 254 + 1;

		std::vector<int32_t> riverTileIndices;

		//for (int i = 0; i < maxDroplets; i++) 
		{
			FloatDet posX = IntToFD(sourceX);
			FloatDet posY = IntToFD(sourceY);
			double dirX = 0; // Requires double since dirX can be too small
			double dirY = 0;

			riverTileIndices.clear();
			bool riverFoundExit = false;
			bool riverOutOfBound = false;

			int lifeTime;
			for (lifeTime = 0; lifeTime < maxLifetime; lifeTime++)
			{
				int nodeX = FDToInt(posX);
				int nodeY = FDToInt(posY);

				int dropIndex = nodeX + nodeY * _tileDimX;

				// droplet's offset within a tile
				FloatDet localX = posX - IntToFD(nodeX);
				FloatDet localY = posY - IntToFD(nodeY);

				FloatDet height00 = heightMap[dropIndex];
				FloatDet height10 = heightMap[dropIndex + 1];
				FloatDet height01 = heightMap[dropIndex + _tileDimX];
				FloatDet height11 = heightMap[dropIndex + 1 + _tileDimX];
				FloatDet height = FDMul(FDMul(height00, FDOne - localX), FDOne - localY) +
									FDMul(FDMul(height10, localX), FDOne - localY) +
									FDMul(FDMul(height01, FDOne - localX), localY) +
									FDMul(FDMul(height11, localX), localY);

				// Out into the ocean
				if (height <= FD0_XXX(110)) {
					//PUN_LOG("river reached ocean");
					riverFoundExit = true;
					break;
				}

				// Hit another river.. stop
				uint8& passedRiver = passedRivers[(nodeX / 4) + (nodeY / 4) * _tile4x4DimX];
				if (passedRiver != 0 && passedRiver != riverId) {
					//PUN_LOG("river reached another river");
					riverFoundExit = true;
					break;
				}

				auto getGradient = [&](const std::vector<int16_t>& map, FloatDet& gradientX_Out, FloatDet& gradientY_Out)
				{
					FloatDet heightAux00 = map[dropIndex];
					FloatDet heightAux10 = map[dropIndex + 1];
					FloatDet heightAux01 = map[dropIndex + _tileDimX];
					FloatDet heightAux11 = map[dropIndex + 1 + _tileDimX];

					gradientX_Out = FDMul(heightAux10 - heightAux00, FDOne - localY) + FDMul(heightAux11 - heightAux01, localY);
					gradientY_Out = FDMul(heightAux01 - heightAux00, FDOne - localX) + FDMul(heightAux11 - heightAux10, localX);
				};

				FloatDet gradientX = 0;
				FloatDet gradientY = 0;
				getGradient(heightMapBeforeFlatten, gradientX, gradientY);
				
				FloatDet guideGradientX = 0;
				FloatDet guideGradientY = 0;
				getGradient(riverGuideMap, guideGradientX, guideGradientY);
				
				// Move the droplet
				dirX = dirX * inertia - FDToFloat(gradientX + guideGradientX / 2) * (1.0 - inertia);
				dirY = dirY * inertia - FDToFloat(gradientY + guideGradientY / 2) * (1.0 - inertia);

				// Solve the straight river problem
				if (dirY / dirX > 10.0 || dirX / dirY > 10.0) {
					FloatDet roughGradientX = 0;
					FloatDet roughGradientY = 0;
					getGradient(roughPerlin_n1to1, roughGradientX, roughGradientY);
					
					dirX += -FDToFloat(roughGradientX / 4);
					dirY += -FDToFloat(roughGradientX / 4);
				}

				double length = sqrt(dirX * dirX + dirY * dirY);
				if (length != 0) {
					dirX /= length;
					dirY /= length;
				}

				//if (auto lineBatch = lineBatch()) {
					//FVector p1(posX * displayUnitPerTile, posY * displayUnitPerTile + 50, 20);
					//FVector p2((posX + dirX) * displayUnitPerTile, (posY + dirY) * displayUnitPerTile + 50, 20 + 1);
					//lineBatch->DrawLine(p1, p2, FLinearColor(1, 0, 1 * (float(i) / maxDroplets)), 0, 0.5, 120);
				//}

				posX += FloatToFD(dirX);
				posY += FloatToFD(dirY);

				// new height
				nodeX = FDToInt(posX);
				nodeY = FDToInt(posY);
				
				if (nodeX <= 0 || nodeX + 1 >= _tileDimX || 
					nodeY <= 0 || nodeY + 1 >= _tileDimY) {
					//PUN_LOG("river out of bound");
					riverOutOfBound = true;
					break;
				}

				riverTileIndices.push_back(dropIndex);
			}

			//PUN_LOG("Done Lifetime %d foundExit:%d", lifeTime, riverFoundExit);

			if (riverFoundExit && riverTileIndices.size() > 64)
			{
				const FloatDet erodeAmount = FD0_XXX(007); // old:005 ... 0.0005
				const FloatDet maxDepth = RiverMaxDepth;// FD0_XXX(110); // old 115 or -10 ... 125
				
				const auto erodeTile = [&](int16_t& heightTile, int16_t curErodeAmount) {
					heightTile = max(heightTile - curErodeAmount, maxDepth);
				};

				for (int32_t dropIndex : riverTileIndices)
				{
					erodeTile(heightMap[dropIndex], erodeAmount);

					erodeTile(heightMap[dropIndex - 1], erodeAmount);
					erodeTile(heightMap[dropIndex + 1], erodeAmount);
					erodeTile(heightMap[dropIndex - _tileDimX], erodeAmount);
					erodeTile(heightMap[dropIndex + _tileDimX], erodeAmount);

					erodeTile(heightMap[dropIndex - 1 - _tileDimX], erodeAmount);
					erodeTile(heightMap[dropIndex + 1 - _tileDimX], erodeAmount);
					erodeTile(heightMap[dropIndex - 1 + _tileDimX], erodeAmount);
					erodeTile(heightMap[dropIndex + 1 + _tileDimX], erodeAmount);

					FloatDet fd0_293 = FD0_XXX(293);

					erodeTile(heightMap[dropIndex + _tileDimX * 2 + -2], FDMul(fd0_293, erodeAmount));
					erodeTile(heightMap[dropIndex + _tileDimX * 2 + -1], erodeAmount / 2);
					erodeTile(heightMap[dropIndex + _tileDimX * 2 + 0], erodeAmount / 2);
					erodeTile(heightMap[dropIndex + _tileDimX * 2 + 1], erodeAmount / 2);
					erodeTile(heightMap[dropIndex + _tileDimX * 2 + 2], FDMul(fd0_293, erodeAmount));

					erodeTile(heightMap[dropIndex - _tileDimX * 2 + -2], FDMul(fd0_293, erodeAmount));
					erodeTile(heightMap[dropIndex - _tileDimX * 2 + -1], erodeAmount / 2);
					erodeTile(heightMap[dropIndex - _tileDimX * 2 + 0], erodeAmount / 2);
					erodeTile(heightMap[dropIndex - _tileDimX * 2 + 1], erodeAmount / 2);
					erodeTile(heightMap[dropIndex - _tileDimX * 2 + 2], FDMul(fd0_293, erodeAmount));

					erodeTile(heightMap[dropIndex - 2 - _tileDimX], erodeAmount / 2);
					erodeTile(heightMap[dropIndex - 2], erodeAmount / 2);
					erodeTile(heightMap[dropIndex - 2 + _tileDimX], erodeAmount / 2);

					erodeTile(heightMap[dropIndex + 2 - _tileDimX], erodeAmount / 2);
					erodeTile(heightMap[dropIndex + 2], erodeAmount / 2);
					erodeTile(heightMap[dropIndex + 2 + _tileDimX], erodeAmount / 2);

					// 3rd layer
					erodeTile(heightMap[dropIndex + _tileDimX * 3 + -2], FDMul(fd0_293, erodeAmount) / 4);
					erodeTile(heightMap[dropIndex + _tileDimX * 3 + -1], erodeAmount / 8);
					erodeTile(heightMap[dropIndex + _tileDimX * 3 + 0], erodeAmount / 8);
					erodeTile(heightMap[dropIndex + _tileDimX * 3 + 1], erodeAmount / 8);
					erodeTile(heightMap[dropIndex + _tileDimX * 3 + 2], FDMul(fd0_293, erodeAmount) / 4);

					erodeTile(heightMap[dropIndex - _tileDimX * 3 + -2], FDMul(fd0_293, erodeAmount) / 4);
					erodeTile(heightMap[dropIndex - _tileDimX * 3 + -1], erodeAmount / 8);
					erodeTile(heightMap[dropIndex - _tileDimX * 3 + 0], erodeAmount / 8);
					erodeTile(heightMap[dropIndex - _tileDimX * 3 + 1], erodeAmount / 8);
					erodeTile(heightMap[dropIndex - _tileDimX * 3 + 2], FDMul(fd0_293, erodeAmount) / 4);

					erodeTile(heightMap[dropIndex - 3 - _tileDimX * 2], FDMul(fd0_293, erodeAmount) / 4);
					erodeTile(heightMap[dropIndex - 3 - _tileDimX], erodeAmount / 8);
					erodeTile(heightMap[dropIndex - 3], erodeAmount / 8);
					erodeTile(heightMap[dropIndex - 3 + _tileDimX], erodeAmount / 8);
					erodeTile(heightMap[dropIndex - 3 + _tileDimX * 2], FDMul(fd0_293, erodeAmount) / 4);

					erodeTile(heightMap[dropIndex + 3 - _tileDimX * 2], FDMul(fd0_293, erodeAmount) / 4);
					erodeTile(heightMap[dropIndex + 3 - _tileDimX], erodeAmount / 8);
					erodeTile(heightMap[dropIndex + 3], erodeAmount / 8);
					erodeTile(heightMap[dropIndex + 3 + _tileDimX], erodeAmount / 8);
					erodeTile(heightMap[dropIndex + 3 + _tileDimX * 2], FDMul(fd0_293, erodeAmount) / 4);

					// Record the river for farming
					WorldTile2 dropTile(dropIndex);

					uint8& passedRiver = passedRivers[(dropTile.x / 4) + (dropTile.y / 4) * _tile4x4DimX];
					passedRiver = riverId;
					

					WorldTile4x4::AddTileTo4x4(dropTile.x, dropTile.y, _river4x4Map, _tile4x4DimX, _tile4x4DimY);
				}

				riverSucceeded++;
				//PUN_LOG("Completed River, droplets: %llu lifeTime: %d", riverTileIndices.size(), lifeTime);
			}

			if (_lineBatch) {
				FVector sourcePoint = _mapOrigin + FVector(sourceX * _displayUnitPerTile, sourceY * _displayUnitPerTile + 50, 0);
				_lineBatch->DrawLine(sourcePoint, sourcePoint + FVector(0, 0, 200),
					riverFoundExit ? FLinearColor(0, 1, 0) : FLinearColor(1, 1, 0)
					, 0, 0.5, 120);
				//if (invalidRiver) _lineBatch->DrawLine(sourcePoint, sourcePoint + FVector(0, 10, 200), FLinearColor(1, 0.2, 0), 0, 0.5, 120);
			}
		}
	}

	PUN_LOG("riverSourceCount:%d riverSucceeded:%d", totalSourceCount, riverSucceeded)

	// Debug river
	PUN_DEBUG_EXPR(
		int32 riverCountAfterCreate = 0;
		for (size_t i = _river4x4Map.size(); i-- > 0;) {
			if (_river4x4Map[i] > 0) riverCountAfterCreate++;
		}
	);
	PUN_LOG("riverCountAfterCreate size:%d, count:%d", _river4x4Map.size(), riverCountAfterCreate);
}

void PunTerrainGenerator::Save4x4Map(std::vector<uint8_t>& map4x4, const char* path)
{
	ofstream file;
	file.open(path, ios::out | ios::binary);

	check(map4x4.size() > 0);

	int size = sizeof(map4x4[0]) * map4x4.size();
	file.write(reinterpret_cast<char*>(&map4x4[0]), size);
	file.close();
}
void PunTerrainGenerator::Load4x4Map(std::vector<uint8_t>& map4x4, const char* path)
{
	ifstream file;
	file.open(path, ios::in | ios::binary);

	int size = sizeof(map4x4[0]) * map4x4.size();
	file.read(reinterpret_cast<char*>(&map4x4[0]), size);
	file.close();

	check(map4x4.size() > 0);
}


bool PunTerrainGenerator::SaveOrLoad(bool isSaving)
{
	FString savedFolderPath = FPaths::ConvertRelativePathToFull(FPaths::ProjectSavedDir());
	FString saveDataPath = savedFolderPath + "TerrainData.dat";
	FString saveMetaDataPath = savedFolderPath + "TerrainMetaData.dat";

	auto serialize = [&](FArchive& Ar) {
		Serialize(Ar, false);
	};

	if (isSaving) {
		SCOPE_TIMER_("SaveMap %s", *saveDataPath);
		
		// Metadata
		PunFileUtils::SaveFile(saveMetaDataPath, [&](FArchive& Ar) {
			_mapSettings.Serialize(Ar);
		});
		return PunFileUtils::SaveFile(saveDataPath, serialize);
	}

	SCOPE_TIMER_("LoadMap %s", *saveDataPath);
	
	// Metadata
	PunFileUtils::LoadFile(saveMetaDataPath, [&](FArchive& Ar) {
		_mapSettings.Serialize(Ar);
	});
	return PunFileUtils::LoadFile(saveDataPath, serialize);
}

void PunTerrainGenerator::SetGameMap(bool isMapSetup)
{
	{
		SCOPE_TIMER("SetGameMap");

		_LOG(PunTerrain, "SetGameMap: isMapSetup:%d tileDimX:%d tileDimY:%d", isMapSetup, _tileDimX, _tileDimY);

		VecReset(_terrainMap, TerrainTileType::None);

		// Borders are all Ocean
		auto paintBorder = [&](WorldTile2 tile) {
			if (isMapSetup) {
				_simulation->SetWalkableSkipFlood(tile, false);
			}
			_terrainMap[tile.tileId()] = TerrainTileType::Ocean;
			//_regionWaterTileCount[tile.regionId()]++;
		};
		for (int y = 0; y < _tileDimY; y++) {
			paintBorder(WorldTile2(0, y));
			paintBorder(WorldTile2(_tileDimX - 1, y));
		}
		for (int x = 0; x < _tileDimX; x++) {
			paintBorder(WorldTile2(x, 0));
			paintBorder(WorldTile2(x, _tileDimY - 1));
		}

		// Ignore border
		for (int y = 1; y < _tileDimY - 1; y++) {
			for (int x = 1; x < _tileDimX - 1; x++)
			{
				WorldTile2 tile(x, y);

				FloatDet heightFD = heightMap[x + y * _tileDimX];

				if (heightMap[(x - 1) + (y - 1) * _tileDimX] > MountainHeight ||
					heightMap[(x)	  + (y - 1) * _tileDimX] > MountainHeight ||
					heightMap[(x + 1) + (y - 1) * _tileDimX] > MountainHeight ||
					
					heightMap[(x - 1) + (y) * _tileDimX] > MountainHeight ||
					heightFD > MountainHeight ||
					heightMap[(x + 1) + (y) * _tileDimX] > MountainHeight ||
					
					heightMap[(x - 1) + (y + 1) * _tileDimX] > MountainHeight ||
					heightMap[(x)	  + (y + 1) * _tileDimX] > MountainHeight ||
					heightMap[(x + 1) + (y + 1) * _tileDimX] > MountainHeight)
				{
					if (isMapSetup) {
						_simulation->SetWalkableSkipFlood(tile, false);
					}
					
					_terrainMap[tile.tileId()] = TerrainTileType::Mountain;
					//_regionMountainTileCount[tile.regionId()]++;
				}
				else if (heightMap[(x - 1) + (y - 1) * _tileDimX] < BeachToWaterHeight ||
						heightMap[(x)	  +  (y - 1) * _tileDimX] < BeachToWaterHeight ||
						heightMap[(x + 1) +  (y - 1) * _tileDimX] < BeachToWaterHeight ||

						heightMap[(x - 1) + (y)* _tileDimX] < BeachToWaterHeight ||
						heightFD < BeachToWaterHeight ||
						heightMap[(x + 1) + (y)* _tileDimX] < BeachToWaterHeight ||

						heightMap[(x - 1) + (y + 1) * _tileDimX] < BeachToWaterHeight ||
						heightMap[(x)	  + (y + 1) * _tileDimX] < BeachToWaterHeight ||
						heightMap[(x + 1) + (y + 1) * _tileDimX] < BeachToWaterHeight)
				{
					if (isMapSetup) {
						_simulation->SetWalkableSkipFlood(tile, false);
					}

					// Resolves River vs Ocean
					if (heightFD >= RiverMaxDepth - FD0_XXX(003) && riverFraction100(tile) > 90) {
						_terrainMap[tile.tileId()] = TerrainTileType::River;
					} else {
						_terrainMap[tile.tileId()] = TerrainTileType::Ocean;
					}
					
					//_terrainMap[tile.tileId()] = TerrainTileType::Water;
					//_regionWaterTileCount[tile.regionId()]++;
				}
			}
		}

	}

	// Ocean region originate water.. water will flood
	if (isMapSetup) {
		SCOPE_TIMER("CalculateFertility");
		CalculateAppeal();
	}
}

void PunTerrainGenerator::SetMountainTile(WorldTile2 tile)
{
	_terrainMap[tile.tileId()] = TerrainTileType::Mountain;
	_simulation->SetWalkable(tile, false);
}
void PunTerrainGenerator::SetWaterTile(WorldTile2 tile)
{
	_terrainMap[tile.tileId()] = TerrainTileType::Ocean;
	_simulation->SetWalkable(tile, false);
}
void PunTerrainGenerator::SetImpassableFlatTile(WorldTile2 tile)
{
	_terrainMap[tile.tileId()] = TerrainTileType::ImpassableFlat;
	_simulation->SetWalkable(tile, false);
}


static std::vector<int16_t> newOctave;
void PunTerrainGenerator::QuadrantMirror(std::vector<int16_t>& lastOctave, std::vector<int16_t>& mirrored, int octaves, FloatDet persistence, FloatDet curPersistence)
{
	SCOPE_TIMER_MUL("Perlin Mirroring", PunTerrainGenerator::timerMultiplier);

	const int tileDimXHalf = _tileDimX / 2;
	const int tileDimYHalf = _tileDimY / 2;
	const int lastX = _tileDimX - 1;
	const int lastY = _tileDimY - 1;

	// 4 quadrants mirroring seam
	newOctave.resize(lastOctave.size());
	for (int y = 0; y < tileDimYHalf; y++) {
		for (int x = 0; x < tileDimXHalf; x++) {
			int xx0 = x * 2;
			int yy0 = y * 2;
			int xx1 = lastX - x * 2;
			int yy1 = lastY - y * 2;

			int shiftedX = x + tileDimXHalf;
			int shiftedY = y + tileDimYHalf;

			// condense
			newOctave[x + y * _tileDimX] =					FDMul(lastOctave[xx0 + yy0 * _tileDimX], curPersistence);
			newOctave[shiftedX + y * _tileDimX] =			FDMul(lastOctave[xx1 + yy0 * _tileDimX], curPersistence);
			newOctave[x + shiftedY * _tileDimX] =			FDMul(lastOctave[xx0 + yy1 * _tileDimX], curPersistence);
			newOctave[shiftedX + shiftedY * _tileDimX] =	FDMul(lastOctave[xx1 + yy1 * _tileDimX], curPersistence);

			mirrored[x + y * _tileDimX] += newOctave[x + y * _tileDimX];
			mirrored[shiftedX + y * _tileDimX] += newOctave[shiftedX + y * _tileDimX];
			mirrored[x + shiftedY * _tileDimX] += newOctave[x + shiftedY * _tileDimX];
			mirrored[shiftedX + shiftedY * _tileDimX] += newOctave[shiftedX + shiftedY * _tileDimX];
		}
	}
	swap(newOctave, lastOctave);

	if (octaves > 1) {
		QuadrantMirror(lastOctave, mirrored, octaves - 1, persistence, FDMul(curPersistence, persistence));
	}
}

// Double the freq
void PunTerrainGenerator::QuadrantMirror2(std::vector<int16_t>& lastOctave, std::vector<int16_t>& mirrored)
{
	SCOPE_TIMER_MUL("Perlin Mirroring", PunTerrainGenerator::timerMultiplier);

	const int tileDimXHalf = _tileDimX / 2;
	const int tileDimYHalf = _tileDimY / 2;
	const int lastX = _tileDimX - 1;
	const int lastY = _tileDimY - 1;

	// 4 quadrants mirroring seam
	for (int y = 0; y < tileDimYHalf; y++) {
		for (int x = 0; x < tileDimXHalf; x++) {
			int xx0 = x * 2;
			int yy0 = y * 2;
			int xx1 = lastX - x * 2;
			int yy1 = lastY - y * 2;

			int shiftedX = x + tileDimXHalf;
			int shiftedY = y + tileDimYHalf;

			// condense
			mirrored[x + y * _tileDimX] = lastOctave[xx0 + yy0 * _tileDimX];
			mirrored[shiftedX + y * _tileDimX] = lastOctave[xx1 + yy0 * _tileDimX];
			mirrored[x + shiftedY * _tileDimX] = lastOctave[xx0 + yy1 * _tileDimX];
			mirrored[shiftedX + shiftedY * _tileDimX] = lastOctave[xx1 + yy1 * _tileDimX];
		}
	}
}

void PunTerrainGenerator::CalculateWorldTerrain(const FMapSettings& mapSettings)
{
	_mapSettings = mapSettings;

	PUN_LOG("CalculateWorldTerrain: %s", *_mapSettings.mapSeed);

	string mapSeed = ToStdString(mapSettings.mapSeed);

	// Special "seed"
	if (mapSeed == "seed") {
		mapSeed = "PunSeed";
	}

	
	std::hash<std::string> hasher;
	int32 seedNumber = hasher(mapSeed);
	
	GameRand::ResetStateToTickCount(seedNumber);
	perlinGenerator.Init(seedNumber);
	perlinGenerator2.Init(GameRand::Rand(seedNumber));
	perlinGenerator3.Init(GameRand::Rand(GameRand::Rand(seedNumber)));

	_initStage = 1;

	PUN_LOG("RandState Done %d", GameRand::RandState());
}

uint8 PunTerrainGenerator::NextInitStage()
{
	switch(_initStage)
	{
	case 1: return Init1();
	case 2: return Init2();
	case 3: return Init3();
	case 4: return Init4();
	case 5: return Init5();
	default:
		UE_DEBUG_BREAK();
		return false;
	}
}
uint8 PunTerrainGenerator::Init1()
{
	INIT_LOG("WorldInit1: %d, %d .. %d", _tileDimX, _tileDimY, GameMapConstants::TilesPerWorldX);

	auto modifyPerlinFreq = [&](int32& freq)
	{
		if (_mapSettings.mapSizeEnum() == MapSizeEnum::Large) {
			freq *= 2;
		}
		if (_mapSettings.mapSizeEnum() == MapSizeEnum::Small) {
			freq /= 2;
		}
	};
	
	// Perlin
	{
		FloatDet freq = FD0_XX(8); // 4
		modifyPerlinFreq(freq);

		for (int i = 0; i < _tileDimX * _tileDimY; i++) {
			int x = i % _tileDimX;
			int y = i / _tileDimY;

			_perlinMap[i] = perlinGenerator.noise01(x * freq, y * freq);
		}
	}

	//VecReset(_regionMountainTileCount);
	//VecReset(_regionWaterTileCount);

	// TODO: tileDimY from norm var below

	// This is int16 but it is actually compressed FloatDet
	continentPerlin1 = std::vector<int16>(_tileDimY * _tileDimX);
	continentPerlin2 = std::vector<int16>(_tileDimY * _tileDimX);
	roughnessPerlin = std::vector<int16>(_tileDimY * _tileDimX);
	{
		SCOPE_TIMER_MUL("Generate base perlin", timerMultiplier);
		// Perlins
		int32 roughnessPerlinFreq = 32;
		int32 continentPerlinFreq1 = 2;
		int32 continentPerlinFreq2 = 3;
		int32 continentPerlinFreq3 = 5;

		modifyPerlinFreq(roughnessPerlinFreq);

		for (int y = 0; y < _tileDimY; y++) {
			for (int x = 0; x < _tileDimX; x++)
			{
				FloatDet normX = x * FDOne / _tileDimX;
				FloatDet normY = y * FDOne / _tileDimX; // This is to prevent perlin's stretch

				int index = x + y * _tileDimX;
				roughnessPerlin[index] = perlinGenerator.noise(normX * roughnessPerlinFreq, normY * roughnessPerlinFreq);

				// Multiple perlins since QuadrantMirror starts to look weird after multiple octaves
				// Solution is to do only 1 QuadrantMirror, to get the octaves between the initial 3 octaves
				// 2, 3, 5 each has around 0.8^2 spacing (frequency decreases 0.8 each octave)
				//
				//const int32 initialFrequency = 2;
				//const FloatDet frequencyPercentStep = 70;
				//const FloatDet persistent1000 = 850;
				//FloatDet strength1000 = 1000;

				//FloatDet perlinTotal = 0;
				//FloatDet perlinX = normX * initialFrequency;
				//FloatDet perlinY = normY * initialFrequency;
				//for (int32 i = 0; i < 7; i++) {
				//	perlinTotal += perlinGenerator.noise(perlinX, perlinY) * strength1000 / 1000;
				//	
				//	perlinX = perlinX * frequencyPercentStep / 100;
				//	perlinY = perlinY * frequencyPercentStep / 100;
				//	strength1000 = strength1000 * persistent1000 / 1000;
				//}
				
				//continentPerlin[index] = perlinTotal;

				
				
				
				//// .85 persistent (.9 is too much, too scattered)
				//continentPerlin[index] = perlinGenerator.noise(normX * 2, normY * 2) +
				//						perlinGenerator2.noise(normX * 3, normY * 3) * 85 / 100 + // * 13 / 20 +
				//						perlinGenerator3.noise(normX * 5, normY * 5) * 72 / 100 + // * 3 / 4 + // .85^2 ~= 3/4
				//						perlinGenerator.noise(normX * 7, normY * 7) * 61 / 100 + // * 3 / 5 + // .85^3 = 3/5
				//						perlinGenerator2.noise(normX * 10, normY * 10) * 52 / 100 + // / 2 + // .85^4 ~= 1/2
				//						
				//						perlinGenerator3.noise(normX * 15, normY * 15) * 44 / 100 +
				//						perlinGenerator.noise(normX * 23, normY * 23) * 38 / 100 +
				//						perlinGenerator.noise(normX * 35, normY * 35) * 32 / 100 +
				//						perlinGenerator.noise(normX * 53, normY * 53) * 27 / 100 +
				//						perlinGenerator.noise(normX * 80, normY * 80) * 23 / 100;

				// 1) less detail for river...
				continentPerlin1[index] =
					perlinGenerator.noise(normX * 3, normY * 3) +
					perlinGenerator2.noise(normX * 5, normY * 5) * 80 / 100;

				// 2) details
				continentPerlin2[index] =
					perlinGenerator3.noise(normX * 7, normY * 7) * 67 / 100 +
					perlinGenerator.noise(normX * 10, normY * 10) * 55 / 100 +
					perlinGenerator2.noise(normX * 15, normY * 15) * 45 / 100 +

					perlinGenerator3.noise(normX * 23, normY * 23) * 37 / 100 +
					perlinGenerator.noise(normX * 35, normY * 35) * 30 / 100;
					//perlinGenerator.noise(normX * 52, normY * 52) * 25 / 100;

					//perlinGenerator.noise(normX * continentPerlinFreq1, normY * continentPerlinFreq1) +
					//perlinGenerator.noise(normX * continentPerlinFreq2, normY * continentPerlinFreq2) * 3 / 4 +
					//perlinGenerator.noise(normX * continentPerlinFreq3, normY * continentPerlinFreq3) * 9 / 16; // 8 / 5 * 8 / 10 = 32 / 25

				//continentPerlin2[index] = perlinGenerator2.noise(normX * continentPerlinFreq2, normY * continentPerlinFreq2);

				// Use 2 perlins since doing mirroring too much create artifact
			}
		}

		// TODO: make perlin map scaler to scale maps up (better performance)
	}

	_initStage = 2;
	return false;
}
uint8 PunTerrainGenerator::Init2()
{
	INIT_LOG("WorldInit2: %d, %d .. %d", _tileDimX, _tileDimY, GameMapConstants::TilesPerWorldX);
	
	// Note that having low initial persistence (curPersistence) lead to a less pronounced mirroring.
	//continentPerlinOctave = std::vector<int16>(continentPerlin1);
	
	continentPerlinBeforeMirror = std::vector<int16>(continentPerlin1);
	mirroredRoughness = std::vector<int16>(roughnessPerlin);


	// TODO: has quality settings for octave?
	//QuadrantMirror(continentPerlin, continentPerlinOctave, 1, FD0_XX(85), FD0_XX(85)); // 5 octave

	//QuadrantMirror(continentPerlin, continentPerlinOctave, 3, FD0_XX(80), FD0_XX(50));
	QuadrantMirror(roughnessPerlin, mirroredRoughness, 7, FD0_XX(90), FD0_XX(50));

	

	_initStage = 3;
	return false;
}
uint8 PunTerrainGenerator::Init3()
{
	INIT_LOG("WorldInit3: %d, %d .. %d", _tileDimX, _tileDimY, GameMapConstants::TilesPerWorldX);
	/*
	 * Voronoi
	 */
	int32 subContinentPointsCount = 300;

	// Mountain Density
	switch (_mapSettings.mapMountainDensity) {
	case MapMountainDensityEnum::VeryLow: subContinentPointsCount = 20; break;
	case MapMountainDensityEnum::Low: subContinentPointsCount = 80; break;
	case MapMountainDensityEnum::Medium: subContinentPointsCount = 170; break;
	case MapMountainDensityEnum::High: subContinentPointsCount = 300; break;
	case MapMountainDensityEnum::VeryHigh: subContinentPointsCount = 500; break;
	default: break;
	}

	// Vary with map size
	switch (_mapSettings.mapSizeEnum()) {
		case MapSizeEnum::Small: subContinentPointsCount /= 2; break; // Note we skip large map adjustment since it will look weird
		default: break;
	}

	
	// Only Sample non deep sea points ??
	std::vector<WorldTile2> subContinentPoints;
	TileArea subContinentSampleArea(WorldTile2(-_tileDimX / 4, -_tileDimY / 4), WorldTile2(_tileDimX * 6 / 4, _tileDimY * 6 / 4));
	for (int i = 0; i < subContinentPointsCount; i++) {
		int x = GameRand::Rand() % subContinentSampleArea.sizeX() + subContinentSampleArea.minX; // 6/4 - 1/4
		int y = GameRand::Rand() % subContinentSampleArea.sizeY() + subContinentSampleArea.minY; // TDOO: after making a cylinder world, get rid of this in y direction?
		subContinentPoints.push_back(WorldTile2(x, y));
	}

	// Delaunay
	std::vector<int> delaunayTris;
	std::vector<CircleTile2> delaunayCircles;
	std::vector<pair<int, int>> adjacentTrisPairs;
	Delaunay(subContinentPoints, delaunayTris, delaunayCircles, _tileDimX, _tileDimY, subContinentSampleArea, _displayUnitPerTile);
	FindAdjacentTrisPairs(delaunayCircles, delaunayTris, _tileDimX, _tileDimY, adjacentTrisPairs);

	// Draw Delaunay
	if (_lineBatch)
	{
		for (int i = 0; i < subContinentPoints.size(); i++) {
			//FVector p1 = _mapOrigin + FVector((subContinentPoints[i] % _tileDimX) * _displayUnitPerTile, (subContinentPoints[i] / _tileDimX)* _displayUnitPerTile + 50, 0); // + 50 for weird shift
			FVector p1 = _mapOrigin + FVector(subContinentPoints[i].x * _displayUnitPerTile, subContinentPoints[i].y * _displayUnitPerTile + 50, 0); // + 50 for weird shift
			FVector p2 = p1 + FVector(0, 0, 100);
			_lineBatch->DrawLine(p1, p2, FLinearColor::Red, 0, 2, 120);
		}

		//DrawDebugTris(subContinentPoints, delaunayTris, _tileDimX, _lineBatch, _displayUnitPerTile);
		DrawAdjacentTrisPairs(delaunayCircles, adjacentTrisPairs, _lineBatch, _displayUnitPerTile);
	}

	//std::vector<std::vector<pair<WorldTile2, WorldTile2>>> regionToCenterPairs(GameMapConstants::TotalRegions);
	std::vector<bool> regionToCenterPairs(GameMapConstants::TotalRegions);
	{
		SCOPE_TIMER_MUL("Filter Voronoi by region", timerMultiplier);
		// For each region, go through all adjacentTrisPairs and determine if the pair should be placed within the region
		// regionToCenterPairs can be queried later when we loop through all tileX*tileY. This is for performance.
		for (int regionId = 0; regionId < regionToCenterPairs.size(); regionId++) {
			for (int i = adjacentTrisPairs.size(); i-- > 0;)
			{
				WorldTile2 center1 = delaunayCircles[adjacentTrisPairs[i].first].center;
				WorldTile2 center2 = delaunayCircles[adjacentTrisPairs[i].second].center;
				WorldTile2 regionCenter = WorldRegion2(regionId).centerTile();

				WorldTile2 center1toCurrent = regionCenter - center1;
				// 1 + before sqrt prevent div by 0
				//int center1toCurrentDist = 1 + (int)sqrtf(center1toCurrent.x*center1toCurrent.x + center1toCurrent.y * center1toCurrent.y);

				WorldTile2 center1toCenter2 = center2 - center1;
				int center1toCenter2Dist = 1 + (int)sqrtf(center1toCenter2.x*center1toCenter2.x + center1toCenter2.y * center1toCenter2.y);

				FloatDet dirX = center1toCenter2.x * FDOne / center1toCenter2Dist;
				FloatDet dirY = center1toCenter2.y * FDOne / center1toCenter2Dist;

				// dot is for projection... center1toCurrent gets projected onto center1toCenter2
				//  use int and not FloatDot to prevent overflow
				int dot = FDToInt(dirX * center1toCurrent.x + dirY * center1toCurrent.y);

				// Shave off regions in direction parallel to the edge.
				// dot needs be between 0 and center1toCenter2Dist for the point to be between edge's two points
				if (dot > center1toCenter2Dist || dot <= 0) continue;

				int pointAlongLineX = FDToInt(dot * dirX) + center1.x;
				int pointAlongLineY = FDToInt(dot * dirY) + center1.y;

				int perpendicularX = regionCenter.x - pointAlongLineX;
				int perpendicularY = regionCenter.y - pointAlongLineY;

				// 1 + before sqrt prevent div by 0
				int perpendicularDist = 1 + (int)sqrtf(perpendicularX * perpendicularX + perpendicularY * perpendicularY);

				// Shave off regions in direction perpendicular the the edge.
				// TODO: tileDimX / 4 is temporary
				if (perpendicularDist < _tileDimX / 32) {
					//regionToCenterPairs[regionId].push_back(pair<WorldTile2, WorldTile2>(center1, center2));
					regionToCenterPairs[regionId] = true;
				}
			}
		}
	}

	// map that has tiles 4 units apart... used to smooth the jagged subcontinent voronoi.
	const int32 tile4x4PerWorldX = GameMapConstants::TilesPerWorldX / 4;
	const int32 tile4x4PerWorldY = GameMapConstants::TilesPerWorldY / 4;
	const int32 tile4x4PerRegion = CoordinateConstants::TilesPerRegion / 4;
	const int32 totalTile4x4 = tile4x4PerWorldX * tile4x4PerWorldY;

	std::vector<FloatDet> tile4x4ToMountainRangeHeights(totalTile4x4);
	std::vector<FloatDet> tile4x4ToMountainRangeHeightsTemp(totalTile4x4);
	{
		SCOPE_TIMER_MUL("Mountain Range", timerMultiplier);

		// fill tile4x4 map.
		const int32 regionPerWorldX = GameMapConstants::RegionsPerWorldX;
		const int32 regionPerWorldY = GameMapConstants::RegionsPerWorldY;

		for (int regionX = 0; regionX < regionPerWorldX; regionX++) {
			for (int regionY = 0; regionY < regionPerWorldY; regionY++)
			{
				int32 regionX1 = min(regionX + 1, regionPerWorldX - 1);
				int32 regionY1 = min(regionY + 1, regionPerWorldY - 1);

				FloatDet height_x0_y0 = regionToCenterPairs[regionX + regionY * regionPerWorldX] ? FDOne : 0;
				FloatDet height_x0_y1 = regionToCenterPairs[regionX + regionY1 * regionPerWorldX] ? FDOne : 0;
				FloatDet height_x1_y0 = regionToCenterPairs[regionX1 + regionY * regionPerWorldX] ? FDOne : 0;
				FloatDet height_x1_y1 = regionToCenterPairs[regionX1 + regionY1 * regionPerWorldX] ? FDOne : 0;

				for (int y = 0; y < tile4x4PerRegion; y++) {
					for (int x = 0; x < tile4x4PerRegion; x++)
					{
						FloatDet xAt0Lerp = (height_x0_y0 * (tile4x4PerRegion - y) + height_x0_y1 * y) / tile4x4PerRegion;
						FloatDet xAt1Lerp = (height_x1_y0 * (tile4x4PerRegion - y) + height_x1_y1 * y) / tile4x4PerRegion;
						int32_t tile4x4_X = x + regionX * tile4x4PerRegion;
						int32_t tile4x4_Y = y + regionY * tile4x4PerRegion;
						int tile4x4Id = tile4x4_X + tile4x4_Y * tile4x4PerWorldX;
						tile4x4ToMountainRangeHeights[tile4x4Id] = (xAt0Lerp * (tile4x4PerRegion - x) + xAt1Lerp * x) / tile4x4PerRegion;
					}
				}
			}
		}

		// 2 Pass Smooth ..
		// TODO: optimize with moving sum???
		const int32_t kernelSize = 4;
		const int32_t kernelSize2 = 1 + kernelSize * 2;
		for (int tile4x4_Y = 0; tile4x4_Y < tile4x4PerWorldY; tile4x4_Y++) {
			for (int tile4x4_X = kernelSize; tile4x4_X < tile4x4PerWorldX - kernelSize; tile4x4_X++)
			{
				FloatDet sum = 0;
				for (int xx = -kernelSize; xx < kernelSize + 1; xx++) {
					sum += tile4x4ToMountainRangeHeights[(tile4x4_X + xx) + tile4x4_Y * tile4x4PerWorldX];
				}
				tile4x4ToMountainRangeHeightsTemp[tile4x4_X + tile4x4_Y * tile4x4PerWorldX] = sum / kernelSize2;
			}
		}
		for (int tile4x4_Y = kernelSize; tile4x4_Y < tile4x4PerWorldY - kernelSize; tile4x4_Y++) {
			for (int tile4x4_X = 0; tile4x4_X < tile4x4PerWorldX; tile4x4_X++)
			{
				FloatDet sum = 0;
				for (int yy = -kernelSize; yy < kernelSize + 1; yy++) {
					sum += tile4x4ToMountainRangeHeightsTemp[tile4x4_X + (tile4x4_Y + yy) * tile4x4PerWorldX];
				}
				tile4x4ToMountainRangeHeights[tile4x4_X + tile4x4_Y * tile4x4PerWorldX] = sum / kernelSize2;
			}
		}

		// subcontinentVoronoi lerp
		subcontinentVoronois = std::vector<int16>(_tileDimY * _tileDimX);
		for (int tile4x4_Y = 0; tile4x4_Y < tile4x4PerWorldY; tile4x4_Y++) {
			for (int tile4x4_X = 0; tile4x4_X < tile4x4PerWorldX; tile4x4_X++)
			{
				int32_t tile4x4_X_1 = min(tile4x4_X + 1, tile4x4PerWorldX - 1);
				int32_t tile4x4_Y_1 = min(tile4x4_Y + 1, tile4x4PerWorldY - 1);

				FloatDet height_x0_y0 = tile4x4ToMountainRangeHeights[tile4x4_X + tile4x4_Y * tile4x4PerWorldX];
				FloatDet height_x0_y1 = tile4x4ToMountainRangeHeights[tile4x4_X + tile4x4_Y_1 * tile4x4PerWorldX];
				FloatDet height_x1_y0 = tile4x4ToMountainRangeHeights[tile4x4_X_1 + tile4x4_Y * tile4x4PerWorldX];
				FloatDet height_x1_y1 = tile4x4ToMountainRangeHeights[tile4x4_X_1 + tile4x4_Y_1 * tile4x4PerWorldX];

				for (int y = 0; y < 4; y++) {
					for (int x = 0; x < 4; x++)
					{
						FloatDet x0Lerp = (height_x0_y0 * (4 - y) + height_x0_y1 * y) / 4;
						FloatDet x1Lerp = (height_x1_y0 * (4 - y) + height_x1_y1 * y) / 4;
						int32_t tileX = x + tile4x4_X * 4;
						int32_t tileY = y + tile4x4_Y * 4;
						int tileId = tileX + tileY * _tileDimX;
						subcontinentVoronois[tileId] = (x0Lerp * (4 - x) + x1Lerp * x) / 4;
					}
				}
			}
		}

	}

	_initStage = 4;
	return false;
}
uint8 PunTerrainGenerator::Init4()
{
	INIT_LOG("WorldInit4: %d, %d .. %d", _tileDimX, _tileDimY, GameMapConstants::TilesPerWorldX);

	FloatDet continentPerlinFactor = FD0_XX(35); // The higher factor, the less land mass???

	FloatDet continentPerlinFactorVariation = FD0_XX(07);
	continentPerlinFactor += (static_cast<int>(_mapSettings.mapSeaLevel) - 2) * continentPerlinFactorVariation;
	
	/*
	 * Assemble HeightMap
	 */
	{
		SCOPE_TIMER_MUL("Assemble HeightMap", timerMultiplier);
		int count = 0;

		for (int y = 0; y < _tileDimY; y++) {
			for (int x = 0; x < _tileDimX; x++)
			{
				FloatDet normX = IntToFD(x) / _tileDimX;
				FloatDet normY = IntToFD(y) / _tileDimY;

				int tileId = x + y * _tileDimX;
				/*
				 * Parabola Tapering
				 */
				PUN_CHECK(0 <= normX && normX < FDOne);
				FloatDet parabolaX2 = (normX - FDHalf) * 2; // Turn [0 to 1] -> [-1 to 1]
				//parabolaX2 = FDQuarter(abs(parabolaX2)) + FDQuarter(FDMul(parabolaX2, parabolaX2)) * 3;
				parabolaX2 = abs(parabolaX2); // Less cold land, but more organic looking shape towards poles
				//parabolaX2 = FDMul(parabolaX2, parabolaX2);
				FloatDet parabolaTaperingX = FDOne - parabolaX2;

				FloatDet parabolaY2 = (normY - FDHalf) * 2;
				//parabolaY2 = abs(parabolaY2);
				parabolaY2 = FDMul(parabolaY2, parabolaY2);
				FloatDet parabolaTaperingY = FDOne - parabolaY2;

				FloatDet parabolaTapering = FDMin(parabolaTaperingX, parabolaTaperingY);

				// Perlin
				FloatDet roughPerlin_n1to1 = mirroredRoughness[tileId];
				FloatDet continentPerlinFinal = continentPerlin1[tileId] + continentPerlin2[tileId] +roughPerlin_n1to1 / 16 * 3;
				//continentPerlinFinal = FDHalf(continentPerlinFinal) + FDHalf;
				continentPerlinFinal = FDHalf(continentPerlinFinal) + FDHalf;
				
				FloatDet continentPerlinTapered = FDMul(continentPerlinFinal - continentPerlinFactor, parabolaTapering);
				FloatDet continents = FDMin(FD0_XXX(125), continentPerlinTapered); // Flat land + ocean

				FloatDet subcontinentVoronoi = subcontinentVoronois[tileId];

				// The more continentLandFactor, the closer an ocean floor is to land. at continents=0.125f, this is 1.0f which means mountains have full strength
				FloatDet continentLandFactor = FDDiv(continents - FD0_X(1), FD0_XXX(025));
				continentLandFactor = FDMax(continentLandFactor, 0);

				// Make mountain range
				FloatDet roughPerlinMountain = FDMax(0, roughPerlin_n1to1 - FDOne / 8);
				FloatDet roughPerlinMountainWithVoronoiPlain = FDMul(subcontinentVoronoi, roughPerlinMountain);
				FloatDet mountainRange = FDMul(roughPerlinMountainWithVoronoiPlain, continentLandFactor);

				// Main!!!
				FloatDet finalZ = continents;
				finalZ += mountainRange;

				heightMap[x + y * _tileDimX] = FDMax(0, FDMin(FDOne, finalZ));

				// Reuse continentPerlin as unflatted continent for river calcation
				// We don't use continentLandFactor directly because that is a flat land
				FloatDet subcontinentVoronoiAdjustedForRiver = max(0, (int)FDMul(subcontinentVoronoi, roughPerlin_n1to1)); // only half of perlin protrude up
				continentPerlin1[tileId] = (FDHalf(continentPerlinBeforeMirror[tileId]) + FDHalf) * 2 + subcontinentVoronoiAdjustedForRiver / 4 + roughPerlin_n1to1 / 16;// continentPerlinTapered / 3 + (subcontinentVoronoiAdjustedForRiver / 32);
				//continentPerlin[tileId] = (continentPerlinBeforeMirror[tileId]) * 2;// +subcontinentVoronoiAdjustedForRiver / 4 + roughPerlin_n1to1 / 16;// continentPerlinTapered / 3 + (subcontinentVoronoiAdjustedForRiver / 32);

			}
		}
	}

	_initStage = 5;
	return false;
}
uint8 PunTerrainGenerator::Init5()
{
	INIT_LOG("WorldInit5: %d, %d .. %d", _tileDimX, _tileDimY, GameMapConstants::TilesPerWorldX);
	
	Erode(continentPerlin1, continentPerlin2, mirroredRoughness);

	GenerateMoisture(); // rainfall4x4map

	SaveOrLoad(true);
	
	// Clean
	_initStage = 0;
	continentPerlin1.clear(); continentPerlin1.shrink_to_fit();
	continentPerlin2.clear(); continentPerlin2.shrink_to_fit();
	
	roughnessPerlin.clear(); roughnessPerlin.shrink_to_fit();
	//continentPerlinOctave = std::vector<int16>();
	continentPerlinBeforeMirror.clear(); continentPerlinBeforeMirror.shrink_to_fit();
	mirroredRoughness.clear(); mirroredRoughness.shrink_to_fit();

	return true;
}

// Generate fertility. 
// !!! Must be done after SetGameMap
// Do horizontal blur using baseFertility and riverBaseFertility (river_region + water)
// Fertility map is calculated for every 4 tiles and the real one is lerped between...
void PunTerrainGenerator::CalculateAppeal()
{
	int mapSize = _tile4x4DimX * _tile4x4DimY;

	// Mountain Coastal
	WorldTile4x4::Blur(2, _isCoastal4x4Map, _tile4x4DimX, _tile4x4DimY);

	// Appeal
	_appealMap.resize(mapSize);

	for (int y = 0; y < _tile4x4DimY; y++) {
		for (int x = 0; x < _tile4x4DimX; x++) {
			int32 terrainAppeal = max(_isCoastal4x4Map[x + y * _tile4x4DimX], _isMountain4x4Map[x + y * _tile4x4DimX]);

			// Appeal gets manipulated by perlin...
			FloatDet perlin_fd = _perlinMap[(x * 2) + (y * 4) * _tileDimX];
			perlin_fd = (perlin_fd - FDHalf) * 10 + FDHalf; // Enhance diff by X times
			perlin_fd = max(0, min(FDOne, perlin_fd)); // Clamp
			perlin_fd = perlin_fd * 12 / 10; // Max fertility is 120%

			int32 perlinAppeal = FDSafeMul(perlin_fd, 100);
			int32 appeal = max(perlinAppeal, terrainAppeal);

			_appealMap[x + y * _tile4x4DimX] = appeal;
		}
	}
}

// Sample fertility from _rainfall4x4Map using bilinear interpolation.
int32 PunTerrainGenerator::GetFertilityPercent(WorldTile2 tile)
{
	if (!tile.isValid()) return 0;

	int32 fertility = WorldTile4x4::Get4x4Lerped(tile, _tile4x4DimX, _tile4x4DimY, [&](int32_t tile4x4Id) {
		return static_cast<int32>(_rainfall4x4Map[tile4x4Id]) * 100 / 255;
	});

	// beyond tundra decrease fertility
	int32 temperatureFraction10000 = GetTemperatureFraction10000(tile.x, fertility);
	if (temperatureFraction10000 > tundraTemperatureStart10000) {
		int32 tundraFertilityZeroBandFraction10000 = 200;
		int32 fertilityDeduction = (temperatureFraction10000 - tundraTemperatureStart10000) / tundraFertilityZeroBandFraction10000;
		fertility = max(0, fertility - fertilityDeduction);
	}
	// Within equator
	// Note that we must lerp under savanna band since desert is forced to have no plant
	else if (temperatureFraction10000 < forestTemperatureStart10000)
	{
		int32 equatorFertilityFullBandFraction10000 = 200;
		
		int32 fertilityMin = (forestTemperatureStart10000 - temperatureFraction10000) * 30 / equatorFertilityFullBandFraction10000; // minimum 30 fertility at full...
		fertilityMin = min(fertilityMin, 30);
		fertility = max(fertilityMin, fertility);
	}
	

	int32 provinceId = _simulation->GetProvinceIdClean(tile);
	if (provinceId != -1)
	{
		GeoresourceEnum georesourceEnum = _simulation->georesource(provinceId).georesourceEnum;
		if (fertility <= 100)
		{
			if (IsFarmGeoresource(georesourceEnum))
			{
				int32 fertilityBuff100 = max(0, 20 - WorldTile2::Distance(tile.region().centerTile(), tile));
				fertilityBuff100 = fertilityBuff100 * 50 / 16;
				fertility = fertility + fertilityBuff100;

				fertility = min(100, fertility);
			}
		}

		// nearby
		int32 provinceTownId = _simulation->provinceOwnerTown(provinceId);
		if (provinceTownId != -1)
		{
			const std::vector<int32>& buildingIds = _simulation->buildingIds(provinceTownId, CardEnum::IrrigationReservoir);
			for (int32 buildingId: buildingIds)
			{
				Building& building = _simulation->building(buildingId);
				if (building.isConstructed())
				{
					int32 distance = building.DistanceTo(tile);
					const int32 bandSize = 3;
					if (
						distance <= IrrigationReservoir::Radius + bandSize)
					{
						int32 fertilityIrrigated = 100;
						if (distance > IrrigationReservoir::Radius) {
							int32 bandFraction100 = 100 - (distance - IrrigationReservoir::Radius) * 100 / bandSize;
							fertilityIrrigated = bandFraction100 * 100 / 100;
						}

						fertility = max(fertility, fertilityIrrigated);
					}
				}
			}
		}

	}


	return max(0, min(fertility, MaxFertility));
}

int32 PunTerrainGenerator::GetRegionFertility(WorldRegion2 region)
{
	return region.GetAverage4x4Value(_tile4x4DimX, _tile4x4DimY, [&](int32_t tile4x4Id) {
		return static_cast<int32>(_rainfall4x4Map[tile4x4Id]) * 100 / 255;
	});
}

int32 PunTerrainGenerator::GetAppealPercent(WorldTile2 tile)
{
	if (!tile.isValid()) return 0;

	int32 appeal = WorldTile4x4::Get4x4Lerped(tile, _tile4x4DimX, _tile4x4DimY, [&](int32_t tile4x4Id) {
		return _appealMap[tile4x4Id];
	});

	appeal = min(appeal, 100); // coast/mountain appeal can't be above 100;
	appeal = 70 + appeal * 30 / 100; // Minimum natural appeal is 70;
	return appeal;
}