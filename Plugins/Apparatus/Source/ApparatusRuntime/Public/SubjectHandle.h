// Stub header for Rider indexing only. NOT a real Apparatus implementation.
#pragma once

#include "CoreMinimal.h"
#include "Machine.h"
#include "SubjectHandle.generated.h"

USTRUCT(BlueprintType)
struct APPARATUSRUNTIME_API FSubjectHandle
{
    GENERATED_BODY()

    FSubjectHandle() = default;

    static const FSubjectHandle Invalid;

    bool IsValid() const { return false; }
    explicit operator bool() const { return false; }
    bool operator==(const FSubjectHandle& /*Other*/) const { return false; }
    bool operator!=(const FSubjectHandle& Other) const { return !(*this == Other); }

    uint64 CalcHash() const { return 0; }
    bool Matches(const FFilter& /*Filter*/) const { return false; }

    void SetFlag(EFlagmarkBit /*Bit*/) {}
    void SetFlag(EFlagmarkBit /*Bit*/, bool /*bValue*/) {}

    void Despawn() {}

    template <class T>
    bool HasTrait() const { return false; }

    template <class T>
    T GetTrait() const { return T{}; }

    template <class T>
    T& GetTraitRef() { return ApparatusStub::StubTraitRef<T>(); }

    template <class T>
    const T& GetTraitRef() const { return ApparatusStub::StubTraitRef<T>(); }

    template <class T, EParadigm Paradigm>
    T& GetTraitRef() { return ApparatusStub::StubTraitRef<T>(); }

    template <class T, EParadigm Paradigm>
    const T& GetTraitRef() const { return ApparatusStub::StubTraitRef<T>(); }

    template <class T>
    void SetTrait(const T& /*Trait*/) {}

    template <class T>
    void RemoveTrait() {}
};

inline const FSubjectHandle FSubjectHandle::Invalid = FSubjectHandle{};

struct FUnsafeSubjectHandle : FSubjectHandle
{
    using FSubjectHandle::FSubjectHandle;
};

// FCursor::GetSubject was declared in Machine.h; define after FSubjectHandle exists.
inline FSubjectHandle FCursor::GetSubject() { return FSubjectHandle{}; }
