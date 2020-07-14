// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "PunTerrainGenerator.h"
#include <memory>
#include "Components/LineBatchComponent.h"
#include "TerrainGeneratorDataSource.h"

#include "TerrainMapTestActor.generated.h"

UCLASS()
class ATerrainMapTestActor : public AActor
{
	GENERATED_BODY()
	
public:	
	// Sets default values for this actor's properties
	ATerrainMapTestActor();

protected:
	// Called when the game starts or when spawned
	virtual void BeginPlay() override;

public:	
	// Called every frame
	virtual void Tick(float DeltaTime) override;

	std::unique_ptr<PunTerrainGenerator> terrainGenerator;
	class UTerrainMapComponent* terrainMap;
	ULineBatchComponent* _lineBatch;
};
