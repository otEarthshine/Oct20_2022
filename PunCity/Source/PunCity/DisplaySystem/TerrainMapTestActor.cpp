// Fill out your copyright notice in the Description page of Project Settings.

#include "TerrainMapTestActor.h"
#include "TerrainMapComponent.h"
#include "ProceduralMeshComponent.h"

#include "Materials/Material.h"

using namespace std;

// Sets default values
ATerrainMapTestActor::ATerrainMapTestActor()
{
 	// Set this actor to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = true;
}

// Called when the game starts or when spawned
void ATerrainMapTestActor::BeginPlay()
{
	Super::BeginPlay();
	
}

// Called every frame
void ATerrainMapTestActor::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);
	//UE_LOG(LogTemp, Error, TEXT("sdfklj"));
	if (terrainMap) {
		terrainMap->SetRelativeLocation(FVector::ZeroVector);
	}
}

