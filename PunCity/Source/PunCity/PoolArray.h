// Pun Dumnernchanvanit's

#pragma once

#include "CoreMinimal.h"
#include "UObject/NoExportTypes.h"
#include "PoolArray.generated.h"

/**
 * 
 */
UCLASS()
class PROTOTYPECITY_API UPoolArray : public UObject
{
	GENERATED_BODY()
public:
	bool bInUse = false; // bInUse, true, means it is out of pool
	UPROPERTY() TArray<uint8> data;
};
