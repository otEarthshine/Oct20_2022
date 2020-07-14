// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "UObject/Interface.h"
#include "TerrainGeneratorDataSource.generated.h"

// This class does not need to be modified.
UINTERFACE(MinimalAPI)
class UTerrainGeneratorDataSource : public UInterface
{
	GENERATED_BODY()
};

/**
 * 
 */
class ITerrainGeneratorDataSource
{
	GENERATED_BODY()
public:
	virtual class ULineBatchComponent* lineBatch() = 0;
};
