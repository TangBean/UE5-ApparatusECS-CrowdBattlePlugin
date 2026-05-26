#pragma once
 
#include "CoreMinimal.h"
#include "Traits/Damage.h"
#include "SubjectHandle.h"
#include "Slow.generated.h" 

USTRUCT(BlueprintType)
struct BATTLEFRAME_API FSlow
{
	GENERATED_BODY()

public:

	FSubjectHandle SlowTarget = FSubjectHandle();

	float SlowTimeout = 4.f;
	float SlowStrength = 1.f;
	EDmgType DmgType = EDmgType::Normal;
	bool bJustSpawned = true;
};

USTRUCT(BlueprintType)
struct BATTLEFRAME_API FSlowing
{
	GENERATED_BODY()

private:

	mutable std::atomic<bool> LockFlag{ false };

public:

	void Lock() const
	{
		while (LockFlag.exchange(true, std::memory_order_acquire));
	}

	void Unlock() const
	{
		LockFlag.store(false, std::memory_order_release);
	}

	TSet<FSubjectHandle> Slows;
	float CombinedSlowMult = 1;

	FSlowing() {};

	FSlowing(const FSlowing& Slowing)
	{
		LockFlag.store(Slowing.LockFlag.load());
		Slows = Slowing.Slows;
		CombinedSlowMult = Slowing.CombinedSlowMult;
	}

	FSlowing& operator=(const FSlowing& Slowing)
	{
		LockFlag.store(Slowing.LockFlag.load());
		Slows = Slowing.Slows;
		CombinedSlowMult = Slowing.CombinedSlowMult;
		return *this;
	}

};