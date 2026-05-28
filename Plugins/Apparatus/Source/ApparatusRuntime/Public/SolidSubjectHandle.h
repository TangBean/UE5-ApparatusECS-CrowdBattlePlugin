// Stub header for Rider indexing only. NOT a real Apparatus implementation.
#pragma once

#include "CoreMinimal.h"
#include "Machine.h"
#include "SubjectHandle.h"

struct FSolidSubjectHandle
{
    FSolidSubjectHandle() = default;

    bool IsValid() const { return false; }
    explicit operator bool() const { return false; }

    void SetFlag(EFlagmarkBit /*Bit*/) {}
    void SetFlag(EFlagmarkBit /*Bit*/, bool /*bValue*/) {}

    template <class T>
    bool HasTrait() const { return false; }

    template <class T>
    T& GetTraitRef() { return ApparatusStub::StubTraitRef<T>(); }

    template <class T>
    const T& GetTraitRef() const { return ApparatusStub::StubTraitRef<T>(); }

    template <class T, EParadigm Paradigm>
    T& GetTraitRef() { return ApparatusStub::StubTraitRef<T>(); }

    template <class T, EParadigm Paradigm>
    const T& GetTraitRef() const { return ApparatusStub::StubTraitRef<T>(); }

    template <class T>
    void SetTraitDeferred(const T& /*Trait*/) {}

    template <class T>
    void RemoveTraitDeferred() {}
};
