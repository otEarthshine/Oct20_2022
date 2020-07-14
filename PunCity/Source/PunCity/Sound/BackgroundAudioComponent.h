// Pun Dumnernchanvanit's

#pragma once

#include "CoreMinimal.h"
#include "Components/AudioComponent.h"
#include "BackgroundAudioComponent.generated.h"

/**
 * 
 */
UCLASS()
class UBackgroundAudioComponent : public UAudioComponent
{
	GENERATED_BODY()
public:

	UPROPERTY(EditAnywhere, BlueprintReadWrite) float FractionFreezingPopulation;
	UPROPERTY(EditAnywhere, BlueprintReadWrite) float FractionStarvingPopulation;
	UPROPERTY(EditAnywhere, BlueprintReadWrite) float Temperature;
};
