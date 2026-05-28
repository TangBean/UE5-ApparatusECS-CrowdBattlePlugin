// Stub header for Rider indexing only. NOT a real Apparatus implementation.
#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Machine.h"
#include "SubjectHandle.h"
#include "SubjectRecord.h"

class AMechanism : public AActor
{
public:
    FSubjectHandle SpawnSubject(const FSubjectRecord& /*Record*/) { return FSubjectHandle{}; }

    FChain* Enchain(const FFilter& /*Filter*/)
    {
        static FChain Chain;
        return &Chain;
    }

    FSolidChain* EnchainSolid(const FFilter& /*Filter*/)
    {
        static FSolidChain Chain;
        return &Chain;
    }

    template <class Chain, class Lambda>
    void Operate(const FFilter& /*Filter*/, Lambda&& /*Op*/) {}

    template <class Chain, class Lambda>
    void OperateConcurrently(const FFilter& /*Filter*/, Lambda&& /*Op*/, int32 /*Threads*/ = 1, int32 /*Batch*/ = 0) {}
};
