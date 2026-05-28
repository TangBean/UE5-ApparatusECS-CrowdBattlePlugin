// Stub header for Rider indexing only. NOT a real Apparatus implementation.
#pragma once

#include "CoreMinimal.h"

// ---- Forward declarations -----------------------------------------------
struct FFilter;
struct FSubjectHandle;
struct FUnsafeSubjectHandle;
struct FSubjectRecord;
class AMechanism;

// ---- Enums --------------------------------------------------------------
enum class EParadigm : uint8
{
    Safe = 0,
    Unsafe = 1,
    SafeHard = 2,
    UnsafeHard = 3,
    Default = Safe,
};

enum class EFlagmarkBit : uint32
{
    None = 0,
    A = 1,
    B = 2,
    C = 3,
    D = 4,
    E = 5,
    F = 6,
    G = 7,
    H = 8,
};

// ---- Trait API mixin used by handle / record types ----------------------
namespace ApparatusStub
{
    template <class T>
    inline T& StubTraitRef()
    {
        static thread_local T Instance{};
        return Instance;
    }
}

// ---- FFilter ------------------------------------------------------------
struct FFilter
{
    FFilter() = default;

    template <class... Ts>
    static FFilter Make() { return FFilter{}; }

    template <class T>
    FFilter& Include() { return *this; }

    template <class T>
    FFilter& Exclude() { return *this; }
};

// ---- Chain / Cursor stubs ----------------------------------------------
struct FCursor
{
    bool Provide() { return false; }
    FSubjectHandle GetSubject();
    template <class T>
    T GetTrait() { return T{}; }
};

struct FChain
{
    template <class Lambda>
    void Operate(Lambda&&) {}

    template <class Lambda>
    void OperateConcurrently(Lambda&&, int32 /*Threads*/ = 1, int32 /*Batch*/ = 0) {}

    FCursor Iterate(int32 /*Offset*/ = 0, int32 /*Count*/ = -1) { return FCursor{}; }
    int32 IterableNum() const { return 0; }
};

using FUnsafeChain = FChain;
using FSolidChain  = FChain;

// ---- UMachine (stub UCLASS-like; no reflection) -------------------------
class UMachine
{
public:
    static AMechanism* ObtainMechanism(class UWorld* /*World*/) { return nullptr; }
};
