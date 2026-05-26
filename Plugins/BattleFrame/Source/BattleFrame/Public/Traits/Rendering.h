
#pragma once

#include "CoreMinimal.h"
#include "SubjectHandle.h"
#include "Rendering.generated.h"


USTRUCT(BlueprintType)
struct BATTLEFRAME_API FRendering
{
    GENERATED_BODY()

public:

    UPROPERTY(BlueprintReadWrite, EditAnywhere)
    int32 InstanceId = -1;

    UPROPERTY(BlueprintReadWrite, EditAnywhere)
    FSubjectHandle Renderer = FSubjectHandle();

};
