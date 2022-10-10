// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "../Simulation/GameSimulationInfo.h"
#include "Components/LineBatchComponent.h"
#include "TerrainGeneratorDataSource.h"

#include "PunCity/Simulation/IGameSimulationCore.h"
#include "PunCity/Simulation/Perlin.h"
#include "PunCity/CppUtils.h"

/**
 * 
 */
class PunTerrainGenerator
{
public:
	void Init(IGameSimulationCore* simulation, int tileDimX, int tileDimY, float displayUnitPerTile, MapSizeEnum mapSize, ULineBatchComponent* _lineBatch);

	void CalculateWorldTerrain(const FMapSettings& mapSettings);

	bool isGeneratingTerrain() { return _initStage > 0; }
	int32 percentLoaded() {
		return _initStage * 100 / 6;
	}
	uint8 NextInitStage();
	uint8 Init1();
	uint8 Init2();
	uint8 Init3();
	uint8 Init4();
	uint8 Init5();

	
	bool SaveOrLoad(bool isSaving);

	void SetGameMap(bool isFullSetup = true);
	void SetTerrainTileType_UnprocessedLakeOnly();
	void SetTerrainTileType_FloodFillOcean();
	
	void SetGameMap2_Walkable();

	// Set some tiles to terrain (such as those without provinceId)
	void SetMountainTile(WorldTile2 tile);
	void SetFreshWaterTile(WorldTile2 tile);
	void SetImpassableFlatTile(WorldTile2 tile);

	const std::vector<int16>& GetHeightMap() { return heightMap; }

	void QuadrantMirror(std::vector<int16>& lastOctave, std::vector<int16>& mirrored, int octaves, FloatDet persistence, FloatDet curPersistence = FDOne);
	void QuadrantMirror2(std::vector<int16>& lastOctave, std::vector<int16>& mirrored);

	void DryUpLargeLake();
	void GenerateMoisture();
	void Erode(std::vector<int16>& heightMapBeforeFlatten, std::vector<int16>& riverGuideMap, std::vector<int16>& roughPerlin_n1to1);
	void MarkLakeAsFreshWater();

	void CalculateAppeal();
	int32 GetFertilityPercentBase(WorldTile2 tile);

	int32 GetRegionFertility(WorldRegion2 region);

	int32 GetAppealPercent(WorldTile2 tile);

	/*
	 * Region info
	 */
	
	//int16 regionMountainTileCount(int32 regionId) const { return _regionMountainTileCount[regionId]; }
	//int16 regionWaterTileCount(int32 regionId) const { return _regionWaterTileCount[regionId]; }
	//int32 regionFlatTileCount(int32 regionId) const { return CoordinateConstants::TileIdsPerRegion - regionMountainTileCount(regionId) - regionWaterTileCount(regionId); };
	//bool regionIsMountain(int32 regionId) const { return regionMountainTileCount(regionId) > CoordinateConstants::TileIdsPerRegion / 3; }
	//bool regionIsOcean(int32 regionId) const { return regionWaterTileCount(regionId) > CoordinateConstants::TileIdsPerRegion * 3 / 4; }


	const std::vector<uint8>& river4x4Map() { return _river4x4Map; }
	const std::vector<uint8>& rainfall4x4Map() { return _rainfall4x4Map; }
	const std::vector<uint8>& isMountain4x4Map() { return _isMountain4x4Map; }

	int32 riverFraction100(WorldTile2 tile) {
		return WorldTile4x4::Get4x4Lerped(tile, GameMapConstants::Tiles4x4PerWorldX, GameMapConstants::Tiles4x4PerWorldY,
			[&](int32_t tile4x4Id)
		{
			return _river4x4Map[tile4x4Id];
		});
	}
	float riverFraction(WorldTile2 tile) {
		return riverFraction100(tile) / 100.0f;
	}

	// TODO: for Desert Palm ???
	//int32 riverMoisture100(WorldTile2 tile) {
	//	return WorldTile4x4::Get4x4Lerped(tile, GameMapConstants::Tiles4x4PerWorldX, GameMapConstants::Tiles4x4PerWorldY,
	//		[&](int32_t tile4x4Id)
	//	{
	//		return std::min(255, static_cast<int32>(_river4x4Map[tile4x4Id]) * 5) * 100 / 255; // Oct 5: * 9
	//	});
	//}

	
	float rainfall255(WorldTile2 tile)
	{
		int32 lerped= WorldTile4x4::Get4x4Lerped(tile, GameMapConstants::Tiles4x4PerWorldX, GameMapConstants::Tiles4x4PerWorldY,
			[&](int32 tile4x4Id)
		{
			return _rainfall4x4Map[tile4x4Id];
		});
		return lerped;
	}

	int32 GetRainfall100(WorldTile2 tile) const {
		// Guard Crash
		int32 index = (tile.x / 4) + (tile.y / 4) * _tile4x4DimX;
		if (index < 0) return 100;
		if (index >= _rainfall4x4Map.size()) return 100;
		
		int32 rainfall255 = _rainfall4x4Map[index];
		return rainfall255 * 100 / 255;
	}

	// Higher is colder...
	int32 GetTemperatureFraction10000(int32 tileX, int32 rainfall100) const
	{
		int32 latitudeFraction10000 = tileX * 10000 / GameMapConstants::TilesPerWorldX;
		latitudeFraction10000 = abs(latitudeFraction10000 - 5000) * 2; // Make equator 0
		latitudeFraction10000 = latitudeFraction10000 - ((rainfall100 - 50) * 1 / 10) * 100; // rainfall affect temperature with 0.1 factor
		return std::max(0, latitudeFraction10000);
	}

	BiomeEnum GetBiome(WorldRegion2 region) const {
		return GetBiome(region.centerTile());
	}

	BiomeEnum GetBiome(WorldTile2 tile) const
	{
		int32 rainfall100 = GetRainfall100(tile);

		int32 temperatureFraction10000 = GetTemperatureFraction10000(tile.x, rainfall100);

		if (_mapSettings.mapTemperature == MapTemperatureEnum::VeryLow) {
			return BiomeEnum::Tundra;
		}

		if (temperatureFraction10000 >= GameMapConstants::TundraTemperatureStart10000) {
			return BiomeEnum::Tundra;
		}
		if (temperatureFraction10000 > GameMapConstants::BorealTemperatureStart10000) {
			if (rainfall100 > taigaStartRainfall100) {
				return BiomeEnum::BorealForest;
			}
			return BiomeEnum::Tundra;
		}
		if (temperatureFraction10000 > GameMapConstants::ForestTemperatureStart10000) {
			if (rainfall100 > forestRainfallStart100) {
				return BiomeEnum::Forest;
			}
			if (rainfall100 > grasslandRainfallStart100) {
				return BiomeEnum::GrassLand;
			}
			return BiomeEnum::Desert;
		}
		if (rainfall100 > jungleRainfallStart100) {
			return BiomeEnum::Jungle;
		}
		return BiomeEnum::Savanna;
	}
	BiomeInfo GetBiomeInfoFromTile(WorldTile2 tile) const {
		return GetBiomeInfo(GetBiome(tile));
	}

	int32 GetHeight(WorldTile2 tile) const {
		return heightMap[tile.tileId()];
	}
	float GetTerrainDisplayHeight(WorldTile2 tile) const
	{
		const float flatHeight = FlatLandHeightFloat;// FDToFloat(FlatLandHeight);
		float height = FDToFloat(heightMap[tile.tileId()]);
		height = (height - flatHeight) / (1.0f - flatHeight) * CoordinateConstants::AboveFlatDisplayUnitHeight;
		return height;
	}

	std::string GetBiomeName(WorldTile2 tile) {
		return ToStdString(GetBiomeNameT(tile).ToString());
	}
	FText GetBiomeNameT(WorldTile2 tile) {
		int height = GetHeight(tile);
		if (height < BeachToWaterHeight - FD0_XXX(010)) {
			return NSLOCTEXT("BiomeInfoName", "Water", "Water");
		}
		return GetBiomeInfoFromTile(tile).name;
	}

	//bool IsRegionWater(WorldRegion2 region) {
	//	return regionWaterTileCount(region.regionId()) > CoordinateConstants::TileIdsPerRegion * 3 / 4;
	//}

	int32 IsOceanCoast(WorldTile2 tile) const {
		int32 tile4x4Id = (tile.x / 4) + (tile.y / 4) * _tile4x4DimX;
		int32 coastal = _isCoastal4x4Map[tile4x4Id];
		coastal = (coastal * (255 - static_cast<int32>(_river4x4Map[tile4x4Id]))) / 255;
		return coastal;
	}

	FloatDet resourcePerlin(WorldTile2 tile) {
		return _perlinMap[tile.tileId()];
	}

	TerrainTileType terrainTileType(WorldTile2 tile) const { return _terrainMap[tile.tileId()]; }

	const std::vector<TerrainTileType>& terrainMap() { return _terrainMap; }
	

	void Serialize(FArchive &Ar, bool withSimulation = true)
	{
		SerializeVecValue(Ar, heightMap);
		SerializeCrc(Ar, heightMap);
		

		SerializeVecValue(Ar, _river4x4Map);
		SerializeVecValue(Ar, _rainfall4x4Map);

		SerializeVecValue(Ar, _isCoastal4x4Map);
		SerializeVecValue(Ar, _isMountain4x4Map);

		SerializeVecValue(Ar, _appealMap);

		perlinGenerator >> Ar;
		SerializeVecValue(Ar, _perlinMap);

		if (withSimulation) {
			SerializeVecValue(Ar, _terrainMap);
			SerializeCrc(Ar, _terrainMap);
		}

		//// SetGameMap
		//if (withSimulation)
		//{
		//	if (Ar.IsLoading()) {
		//		SetGameMap(false);
		//	}
		//	SerializeCrc(Ar, _terrainMap);
		//}
	}

	void SerializeForMainMenu(FArchive &Ar, const std::vector<int32>& sampleRegionIds)
	{
		// Only save sample regions
		heightMap.resize(GameMapConstants::TilesPerWorld);
		_terrainMap.resize(GameMapConstants::TilesPerWorld);
		for (int32 sampleId : sampleRegionIds) {
			WorldRegion2(sampleId).ExecuteOnRegion_WorldTile([&](WorldTile2 tile) {
				int32 tileId = tile.tileId();
				Ar << heightMap[tileId];
				Ar << _terrainMap[tileId];
			});
		}

		SerializeVecValue(Ar, _river4x4Map);
		SerializeVecValue(Ar, _rainfall4x4Map);
		SerializeVecValue(Ar, _isMountain4x4Map);
	}

public:
	static const int timerMultiplier = 1;

	// TODO: proper settings for 	128, 256
	static const float worldMapSizeX;
	static const float worldMapSizeZ;

private:
	static void Save4x4Map(std::vector<uint8>& map4x4, const char* path);
	static void Load4x4Map(std::vector<uint8>& map4x4, const char* path);

private:
	IGameSimulationCore* _simulation = nullptr;
	ULineBatchComponent* _lineBatch = nullptr;

	int _tileDimX = 0;
	int _tileDimY = 0;
	float _displayUnitPerTile = 0;
	int _tile4x4DimX = 0;
	int _tile4x4DimY = 0;

	/*
	 * Serialized Variables
	 */
public:
	std::vector<int16> heightMap;

private:
	//FString _mapSeed;
	//MapSizeEnum _mapSize = MapSizeEnum::Medium;
	FMapSettings _mapSettings;
	
	std::vector<uint8> _river4x4Map; // Note: River is now freshwater (includes lake)
	std::vector<uint8> _rainfall4x4Map;
	
	// For Appeal map
	std::vector<uint8> _isCoastal4x4Map;
	std::vector<uint8> _isMountain4x4Map;

	std::vector<uint16> _appealMap;

	Perlin perlinGenerator;
	Perlin perlinGenerator2;
	Perlin perlinGenerator3;
	
	std::vector<uint16> _perlinMap; // Nice perlin map for trees/stones/farming/appeals etc.

	/*
	 * Non-Serialize
	 */
	std::vector<TerrainTileType> _terrainMap;

private:
	/*
	 * Temp variables
	 */
	int32 _initStage = 0;
	std::vector<int16> continentPerlin1;
	std::vector<int16> continentPerlin2;
	std::vector<int16> subcontinentVoronois;
	
	std::vector<int16> roughnessPerlin;
	//std::vector<int16> continentPerlinOctave;
	std::vector<int16> continentPerlinBeforeMirror;
	std::vector<int16> mirroredRoughness;
};
