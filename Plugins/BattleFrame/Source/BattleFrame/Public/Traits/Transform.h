#pragma once

#include "CoreMinimal.h"
#include "Transform.generated.h"

USTRUCT(BlueprintType, Category = "Basic")
struct BATTLEFRAME_API FLocated
{
	GENERATED_BODY()

public:

	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Data")
	FVector Location = FVector::ZeroVector;

	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Data")
	FVector PreLocation = FVector::ZeroVector;

	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Data")
	FVector InitialLocation = FVector::ZeroVector;

};

USTRUCT(BlueprintType, Category = "Basic")
struct BATTLEFRAME_API FRotated
{
	GENERATED_BODY()

public:

	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Data")
	FQuat Rotation = FQuat{ FQuat::Identity };

};

USTRUCT(BlueprintType)
struct BATTLEFRAME_API FDirected
{
	GENERATED_BODY()

public:

	UPROPERTY(BlueprintReadWrite, EditAnywhere, meta = (Tooltip = "当钱朝向"))
	FVector Direction = FVector::ForwardVector;

	UPROPERTY(BlueprintReadWrite, EditAnywhere, meta = (Tooltip = "目标朝向"))
	FVector DesiredDirection = FVector::ForwardVector;

};

USTRUCT(BlueprintType, Category = "Basic")
struct BATTLEFRAME_API FScaled
{
	GENERATED_BODY()

public:

	UPROPERTY(BlueprintReadWrite, EditAnywhere, meta = (Tooltip = "实际尺寸"))
	float Scale = 1;

	UPROPERTY(BlueprintReadWrite, EditAnywhere, meta = (Tooltip = "模型渲染尺寸,实际尺寸为RenderScale * JiggleMultiplier"))
	FVector RenderScale = FVector::OneVector;

	UPROPERTY(BlueprintReadWrite, EditAnywhere, meta = (Tooltip = "Jiggle乘数"))
	FVector JiggleMultiplier = FVector::OneVector;

};