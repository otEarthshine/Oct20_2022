// Pun Dumnernchanvanit's

#pragma once

#include "CoreMinimal.h"
#include "UObject/NoExportTypes.h"
#include "SoundAsset.h"
#include "SoundAssets.generated.h"

/**
 * 
 */
UCLASS()
class PROTOTYPECITY_API USoundAssets : public UObject
{
	GENERATED_BODY()
public:
	UPROPERTY() TArray<USoundAsset*> assets;
	
};
