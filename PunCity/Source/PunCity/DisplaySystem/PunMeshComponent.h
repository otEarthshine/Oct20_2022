// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "UObject/ObjectMacros.h"
#include "Components/MeshComponent.h"

#include "ProceduralMeshComponent.h"

#include "PunMeshComponent.generated.h"

/**
 * 
 */
UCLASS()
class UPunMeshComponent : public UMeshComponent
{
	GENERATED_BODY()
public:
	void CreateMeshSection(int32 SectionIndex, const TArray<FVector>& Vertices, const TArray<int32>& Triangles, const TArray<FVector>& Normals, 
							const TArray<FVector2D>& UV0, const TArray<FColor>& VertexColors, const TArray<FProcMeshTangent>& Tangents);

	/** Update LocalBounds member from the local box of each section */
	void UpdateLocalBounds();

private:
	/** Array of sections of mesh */
	UPROPERTY()
	TArray<FProcMeshSection> ProcMeshSections;

	/** Local space bounds of mesh */
	UPROPERTY()
	FBoxSphereBounds LocalBounds;
};
