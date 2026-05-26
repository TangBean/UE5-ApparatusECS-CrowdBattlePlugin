#pragma once

#include "CoreMinimal.h"
#include "SubjectHandle.h"
#include "GridData.generated.h"

// these values are cached for cache optimizaiton, 
// this is exactly 32 bits
// 1 cpu cache line can contain 2 of this struct

USTRUCT(BlueprintType, meta = (ForceAlignment = 4))
struct BATTLEFRAME_API FGridData
{
    GENERATED_BODY()

public:

    uint32 SubjectHash = 0;
    FVector3f Location{0,0,0};
    float Radius = 0;
    FSubjectHandle SubjectHandle = FSubjectHandle();
    float DistSqr = 0;

    // 匹配Handle
    bool operator==(const FGridData& Other) const
    {
        return SubjectHandle == Other.SubjectHandle;
    }

    // 匹配Hash
    bool operator==(uint32 Hash) const
    {
        return SubjectHash == Hash;
    }

    // 查询Hash
    friend uint32 GetTypeHash(const FGridData& Data)
    {
        return Data.SubjectHash;
    }
};
