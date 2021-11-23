// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "DisplaySystemComponent.h"
#include "../AssetLoaderComponent.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "Components/DecalComponent.h"
#include "Engine/Texture2D.h"
#include "TerritoryMeshComponent.h"

#include "TerritoryDisplayComponent.generated.h"

USTRUCT()
struct FTerritoryMeshes
{
	GENERATED_BODY();
	UPROPERTY() TArray<UTerritoryMeshComponent*> TerritoryMeshes;
};

USTRUCT()
struct FTerritoryDecals
{
	GENERATED_BODY()

	UPROPERTY() TArray<UDecalComponent*> decals;
	UPROPERTY() TArray<UMaterialInstanceDynamic*> decalMaterials;

	UPROPERTY() TMap<int32, int32> playerIdToDecalIndex;

	void AddNewDecal(int32_t playerId, USceneComponent* parent, UAssetLoaderComponent* assetLoader);

	int32 Contains(int32_t playerId) {
		return playerIdToDecalIndex.Contains(playerId);
	}

	UDecalComponent* GetDecal(int32_t playerId) { return decals[playerIdToDecalIndex[playerId]]; }
	UMaterialInstanceDynamic* GetDecalMaterial(int32_t playerId) { return decalMaterials[playerIdToDecalIndex[playerId]]; }
	UTexture2D* adjacentTerritoriesTexture(int32_t playerId) { return _adjacentTerritoriesTextures[playerIdToDecalIndex[playerId]]; }
	UTexture2D* adjacentTerritoriesTextures2(int32_t playerId) { return _adjacentTerritoriesTextures2[playerIdToDecalIndex[playerId]]; }

	void Deinit() {
		for (UTexture2D* texture : _adjacentTerritoriesTextures) {
			PunUnrealUtils::DestroyTexture2D(texture);
		}
		for (UTexture2D* texture : _adjacentTerritoriesTextures2) {
			PunUnrealUtils::DestroyTexture2D(texture);
		}
		_adjacentTerritoriesTextures.Empty();
		_adjacentTerritoriesTextures2.Empty();
	}
	
private:
	UPROPERTY() TArray<UTexture2D*> _adjacentTerritoriesTextures;
	UPROPERTY() TArray<UTexture2D*> _adjacentTerritoriesTextures2;
};

UCLASS( ClassGroup=(Custom), meta=(BlueprintSpawnableComponent) )
class UTerritoryDisplayComponent : public UDisplaySystemComponent
{
	GENERATED_BODY()
public:
	//void Init(int size, TScriptInterface<IDisplaySystemDataSource> gameManager, UAssetLoaderComponent* assetLoader, int32 initialPoolSize) final
	//{
	//	UDisplaySystemComponent::Init(size, gameManager, assetLoader, initialPoolSize);
	//}

	void Display(std::vector<int>& sampleProvinceIds) override;

	void Deinit() {
		// TODO: Remove This
		//playerDecals.Deinit();
	}

	void GetDefenseNodeDisplayInfo(int32 provinceId, float displayScaling, 
									DefenseOverlayEnum& defenseOverlayEnum_Out, DefenseColorEnum& defenseColorEnum_Out,
									FTransform& nodeTransform_Out, TArray<FTransform>& lineTransforms_Out, TArray<DefenseColorEnum>& lineDefenseColorEnums_Out, bool isMap = false)
	{
		auto& sim = simulation();
		auto& provinceSys = sim.provinceSystem();
		auto& provinceInfoSys = sim.provinceInfoSystem();
		
		WorldTile2 outerProvinceCenterTile = provinceSys.GetProvinceCenterTile(provinceId);
		const ProvinceOwnerInfo& outerProvinceOwnerInfo = provinceInfoSys.provinceOwnerInfo(provinceId);
		check(outerProvinceOwnerInfo.provinceId == provinceId);
		check(outerProvinceCenterTile.isValid());

		// Node
		{
			defenseColorEnum_Out = DefenseColorEnum::Empty;
			
			// Main City Node
			if (outerProvinceOwnerInfo.townId != -1 &&
				sim.GetTownProvinceId(outerProvinceOwnerInfo.townId) == provinceId)
			{
				defenseOverlayEnum_Out = DefenseOverlayEnum::CityNode;
			}
			// Minor City Node
			else if (sim.IsValidMinorTown(outerProvinceOwnerInfo.townId)) {
				defenseOverlayEnum_Out = DefenseOverlayEnum::CityNode;
				defenseColorEnum_Out = DefenseColorEnum::EnemyUnprotected;
			}
			else if (outerProvinceOwnerInfo.fortIds.size() > 0) {
				defenseOverlayEnum_Out = DefenseOverlayEnum::FortNode;
			}
			else {
				defenseOverlayEnum_Out = DefenseOverlayEnum::Node;
			}

			FVector displayLocation;
			if (isMap) {
				displayLocation = MapUtil::DisplayLocation_Map(outerProvinceCenterTile);
			} else {
				displayLocation = MapUtil::DisplayLocation(gameManager()->cameraAtom(), outerProvinceCenterTile.worldAtom2());
			}

			int32 townPlayerId = sim.townPlayerId(outerProvinceOwnerInfo.townId);
			if (townPlayerId == playerId()) {
				defenseColorEnum_Out = outerProvinceOwnerInfo.isSafe ? DefenseColorEnum::Protected : DefenseColorEnum::Unprotected;
			}
			else if (townPlayerId != -1) {
				defenseColorEnum_Out = outerProvinceOwnerInfo.isSafe ? DefenseColorEnum::EnemyProtected : DefenseColorEnum::EnemyUnprotected;
			}

			nodeTransform_Out = FTransform(FRotator::ZeroRotator, displayLocation, FVector(displayScaling, displayScaling, displayScaling));
		}

		// Lines
		{
			float lineDisplayScalingZ = displayScaling * 0.67f;
			
			const std::vector<ProvinceConnection>& connections = provinceSys.GetProvinceConnections(provinceId);

			TSet<int32> flatConnectedProvinceIds;
			for (const ProvinceConnection& connection : connections) {
				if (connection.tileType == TerrainTileType::None) {
					flatConnectedProvinceIds.Add(connection.provinceId);
				}
			}

			for (const ProvinceConnection& connection : connections)
			{
				bool shouldShowConnection = false;
				if (connection.tileType == TerrainTileType::None) {
					shouldShowConnection = true;
				}
				else if (connection.tileType == TerrainTileType::River &&
					!flatConnectedProvinceIds.Contains(connection.provinceId))
				{
					shouldShowConnection = true; // Has River, and has no flat connection
				}

				if (shouldShowConnection)
				{
					WorldTile2 innerProvinceCenterTile = provinceSys.GetProvinceCenterTile(connection.provinceId);
					check(innerProvinceCenterTile.isValid());;
					const ProvinceOwnerInfo& innerProvinceOwnerInfo = provinceInfoSys.provinceOwnerInfo(connection.provinceId);

					DefenseColorEnum colorEnum = DefenseColorEnum::Empty;

					bool useSelfAsOrigin = false;
					if (outerProvinceCenterTile.x > innerProvinceCenterTile.x) {
						useSelfAsOrigin = true;
					}
					else if (outerProvinceCenterTile.x == innerProvinceCenterTile.x) {
						useSelfAsOrigin = outerProvinceCenterTile.y > innerProvinceCenterTile.y;
					}

					WorldTile2 sourceTile = useSelfAsOrigin ? outerProvinceCenterTile : innerProvinceCenterTile;
					WorldTile2 targetTile = useSelfAsOrigin ? innerProvinceCenterTile : outerProvinceCenterTile;

					ProvinceOwnerInfo sourceOwnerInfo = useSelfAsOrigin ? outerProvinceOwnerInfo : innerProvinceOwnerInfo;
					ProvinceOwnerInfo targetOwnerInfo = useSelfAsOrigin ? innerProvinceOwnerInfo : outerProvinceOwnerInfo;

					FVector sourceVec;
					FVector targetVec;
					if (isMap) {
						sourceVec = MapUtil::DisplayLocation_Map(sourceTile);
						targetVec = MapUtil::DisplayLocation_Map(targetTile);
					}
					else {
						sourceVec = MapUtil::DisplayLocation(gameManager()->cameraAtom(), sourceTile.worldAtom2());
						targetVec = MapUtil::DisplayLocation(gameManager()->cameraAtom(), targetTile.worldAtom2());
					}
					
					FVector diffVec = targetVec - sourceVec;

					float lineDisplayScalingY = lineDisplayScalingZ;

					// Coloring
					if (connection.tileType == TerrainTileType::River) {
						//sourceVec.Z += 4; // Dotted River
						colorEnum = DefenseColorEnum::River;
					}
					else {
						int32 sourcePlayerId = sim.townPlayerId(sourceOwnerInfo.townId);
						int32 targetPlayerId = sim.townPlayerId(targetOwnerInfo.townId);
						
						int32 sourceIsOurs = sourcePlayerId == playerId();
						int32 targetIsOurs = targetPlayerId == playerId();

						int32 sourceIsEnemy = !sourceIsOurs && sourcePlayerId != -1;
						int32 targetIsEnemy = !targetIsOurs && targetPlayerId != -1;

						if (sourceIsOurs && targetIsOurs) 
						{
							if (!sourceOwnerInfo.isSafe || !targetOwnerInfo.isSafe) {
								//sourceVec.Z += 2; // Unsafe ColorlineDefenseColorEnums_Out
								colorEnum = DefenseColorEnum::Unprotected;
							}
							else {
								colorEnum = DefenseColorEnum::Protected;
							}

							// Thicker line for Source/Target is ours
							lineDisplayScalingY *= 2.0f;
						}
						else if (sourceIsEnemy && targetIsEnemy)
						{
							if (!sourceOwnerInfo.isSafe || !targetOwnerInfo.isSafe) {
								colorEnum = DefenseColorEnum::EnemyUnprotected;
							}
							else {
								colorEnum = DefenseColorEnum::EnemyProtected;
							}

							lineDisplayScalingY *= 2.0f;
						}
						else
						{
							//sourceVec.Z += 1; // Enemy
						}
					}
					

					lineTransforms_Out.Add(FTransform(diffVec.Rotation(), sourceVec, FVector(diffVec.Size() * 0.1f, lineDisplayScalingY, lineDisplayScalingZ)));
					lineDefenseColorEnums_Out.Add(colorEnum);
				}
			}
		}

		
	}

private:
	UTerritoryMeshComponent* CreateTerritoryMeshComponent(bool isProvince)
	{
		auto comp = NewObject<UTerritoryMeshComponent>(this);
		comp->Rename(*FString(("TerritoryChunk" + to_string(territoryNameCounter++)).c_str()));
		comp->AttachToComponent(this, FAttachmentTransformRules::KeepRelativeTransform, NAME_None);
		comp->SetCollisionEnabled(ECollisionEnabled::NoCollision);
		comp->SetGenerateOverlapEvents(false);
		comp->bAffectDistanceFieldLighting = false;
		comp->SetReceivesDecals(false);
		comp->SetCastShadow(false);
		comp->RegisterComponent();
		if (isProvince) {
			comp->SetTerritoryMaterial(_assetLoader->M_Province, _assetLoader->M_Province_Top);
		} else {
			comp->SetTerritoryMaterial(_assetLoader->M_Territory, _assetLoader->M_Territory_Top);
		}
		return comp;
	}
	
private:
	UPROPERTY() TArray<UTerritoryMeshComponent*> _provinceMeshes;
	UPROPERTY() TArray<UTerritoryMeshComponent*> _territoryMeshesInner;
	UPROPERTY() TArray<UTerritoryMeshComponent*> _provinceMeshesPool;

	UPROPERTY() TArray<FTerritoryMeshes> _townIdToTerritoryMesh;

	UPROPERTY() TArray<UStaticMeshComponent*> _defenseOverlayMeshes;
	
	int32 territoryNameCounter = 0;
};
