// Stub header for Rider indexing only. NOT a real Apparatus implementation.
#pragma once

#include "CoreMinimal.h"
#include "Machine.h"
#include "SubjectHandle.h"
#include "SubjectRecord.generated.h"

USTRUCT(BlueprintType)
struct APPARATUSRUNTIME_API FSubjectRecord
{
    GENERATED_BODY()

    FSubjectRecord() = default;

    void SetFlag(EFlagmarkBit /*Bit*/, bool /*bValue*/ = true) {}

    template <class T>
    void SetTrait(const T& /*Trait*/) {}

    template <class T>
    T& GetTraitRef() { return ApparatusStub::StubTraitRef<T>(); }

    template <class T>
    const T& GetTraitRef() const { return ApparatusStub::StubTraitRef<T>(); }

    template <class T>
    bool HasTrait() const { return false; }

    template <class T>
    T GetTrait() const { return T{}; }
};
