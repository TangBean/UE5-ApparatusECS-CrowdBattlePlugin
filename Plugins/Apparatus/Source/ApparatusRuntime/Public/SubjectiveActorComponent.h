// Stub header for Rider indexing only. NOT a real Apparatus implementation.
#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "SubjectHandle.h"
#include "SubjectiveActorComponent.generated.h"

UCLASS()
class APPARATUSRUNTIME_API USubjectiveActorComponent : public UActorComponent
{
    GENERATED_BODY()

public:
    FSubjectHandle GetHandle() const { return FSubjectHandle{}; }

    template <class T>
    T GetTrait() const { return T{}; }

    template <class T>
    T& GetTraitRef() { return ApparatusStub::StubTraitRef<T>(); }

    template <class T>
    const T& GetTraitRef() const { return ApparatusStub::StubTraitRef<T>(); }

    template <class T>
    bool HasTrait() const { return false; }

    template <class T>
    void SetTrait(const T& /*Trait*/) {}

    template <class T>
    void RemoveTrait() {}
};
