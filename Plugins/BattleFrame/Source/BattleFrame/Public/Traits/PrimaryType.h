#pragma once

#include "CoreMinimal.h"
#include "SubjectHandle.h"
#include "RVOVector2.h"
#include "PrimaryType.generated.h"

class UNeighborGridComponent;

USTRUCT(BlueprintType)
struct BATTLEFRAME_API FAgent
{
	GENERATED_BODY()

public:

	UPROPERTY(BlueprintReadWrite, EditAnywhere)
	int32 Score = 1;
};

USTRUCT(BlueprintType)
struct BATTLEFRAME_API FHero
{
	GENERATED_BODY()

public:

};

USTRUCT(BlueprintType)
struct BATTLEFRAME_API FProjectile
{
	GENERATED_BODY()

public:

};

USTRUCT(BlueprintType)
struct BATTLEFRAME_API FProp
{
	GENERATED_BODY()

public:

};

USTRUCT(BlueprintType)
struct BATTLEFRAME_API FTower
{
	GENERATED_BODY()
public:

};